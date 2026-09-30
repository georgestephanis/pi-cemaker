#include "telemetry.h"
#include <string.h>

#if defined(PICO_ON_DEVICE) && PICO_ON_DEVICE
#include "hardware/adc.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"
#endif

// Internal state storage
static telemetry_data_t s_telemetry = {
    .v_bat_mv = BATTERY_NOMINAL_MV,
    .v_bus_in_mv = 12000, // Typical 12V PD
    .v_out_mv = 5100,     // 5.10V output
    .temp_c = 25,
    .current_bat_ma = 500, // Charging at 500mA
    .current_out_ma = 2000,// 2A load
    .soc_pct = 75,
    .runtime_secs = 7200,
    .ac_present = true,
    .charging = true,
    .discharging = false,
    .fully_charged = false,
    .low_battery_warn = false,
    .shutdown_imminent = false,
    .fault_overtemp = false,
    .fault_undervoltage = false,
    .sim_mode_active = false
};

// Moving average filters (stored in float for precision)
static float s_filt_vbat = 7400.0f;
static float s_filt_vbus = 12000.0f;
static float s_filt_vout = 5100.0f;
static float s_filt_temp = 25.0f;
static const float EMA_ALPHA = 0.05f; // Low-pass filter coefficient

// State of Charge lookup table for 2S Li-ion (Pack mV -> SoC %)
typedef struct {
    uint16_t mv;
    uint8_t soc;
} soc_point_t;

static const soc_point_t SOC_TABLE[] = {
    { 8400, 100 },
    { 8200,  95 },
    { 8000,  85 },
    { 7800,  75 },
    { 7600,  60 },
    { 7400,  45 },
    { 7200,  30 },
    { 6900,  20 }, // 20% Low battery warning threshold
    { 6600,  12 },
    { 6300,   6 },
    { 6200,   4 }, // <= 5% Critical shutdown threshold
    { 6000,   0 }
};
static const size_t SOC_TABLE_SIZE = sizeof(SOC_TABLE) / sizeof(SOC_TABLE[0]);

static uint8_t calculate_soc(uint16_t pack_mv) {
    if (pack_mv >= SOC_TABLE[0].mv) return 100;
    if (pack_mv <= SOC_TABLE[SOC_TABLE_SIZE - 1].mv) return 0;

    for (size_t i = 0; i < SOC_TABLE_SIZE - 1; i++) {
        if (pack_mv <= SOC_TABLE[i].mv && pack_mv >= SOC_TABLE[i + 1].mv) {
            uint16_t span_mv = SOC_TABLE[i].mv - SOC_TABLE[i + 1].mv;
            uint16_t delta_mv = pack_mv - SOC_TABLE[i + 1].mv;
            uint8_t span_soc = SOC_TABLE[i].soc - SOC_TABLE[i + 1].soc;
            return SOC_TABLE[i + 1].soc + (uint8_t)((delta_mv * span_soc) / span_mv);
        }
    }
    return 0;
}

void telemetry_init(void) {
#if defined(PICO_ON_DEVICE) && PICO_ON_DEVICE
    adc_init();
    adc_gpio_init(ADC_PIN_VBAT);
    adc_gpio_init(ADC_PIN_VBUS_IN);
    adc_gpio_init(ADC_PIN_VOUT);
    adc_gpio_init(ADC_PIN_TEMP);
#endif
    s_filt_vbat = BATTERY_NOMINAL_MV;
    s_filt_vbus = 12000.0f;
    s_filt_vout = 5100.0f;
    s_filt_temp = 25.0f;
}

#if defined(PICO_ON_DEVICE) && PICO_ON_DEVICE
static uint16_t read_adc_mv(uint8_t channel, float divider_ratio) {
    adc_select_input(channel);
    // Average 16 samples for hardware noise immunity
    uint32_t raw_sum = 0;
    for (int i = 0; i < 16; i++) {
        raw_sum += adc_read();
    }
    float raw_avg = (float)raw_sum / 16.0f;
    // RP2040 ADC: 12-bit (4096 counts), reference ~ 3.3V
    float pin_voltage = (raw_avg * 3300.0f) / 4096.0f;
    return (uint16_t)(pin_voltage * divider_ratio);
}
#endif

void telemetry_sample_tick(void) {
    if (s_telemetry.sim_mode_active) {
        // In simulation mode, skip hardware ADC updates
        return;
    }

#if defined(PICO_ON_DEVICE) && PICO_ON_DEVICE
    uint16_t raw_vbat = read_adc_mv(0, VBAT_DIVIDER_RATIO);
    uint16_t raw_vbus = read_adc_mv(1, VBUS_DIVIDER_RATIO);
    uint16_t raw_vout = read_adc_mv(2, VOUT_DIVIDER_RATIO);

    // Apply Exponential Moving Average filter
    s_filt_vbat += EMA_ALPHA * ((float)raw_vbat - s_filt_vbat);
    s_filt_vbus += EMA_ALPHA * ((float)raw_vbus - s_filt_vbus);
    s_filt_vout += EMA_ALPHA * ((float)raw_vout - s_filt_vout);

    s_telemetry.v_bat_mv = (uint16_t)s_filt_vbat;
    s_telemetry.v_bus_in_mv = (uint16_t)s_filt_vbus;
    s_telemetry.v_out_mv = (uint16_t)s_filt_vout;
#endif

    // Update power flags based on voltages
    s_telemetry.ac_present = (s_telemetry.v_bus_in_mv > 7500); // Input PD > 7.5V
    s_telemetry.soc_pct = calculate_soc(s_telemetry.v_bat_mv);

    if (s_telemetry.ac_present) {
        s_telemetry.discharging = false;
        if (s_telemetry.v_bat_mv >= 8350) {
            s_telemetry.charging = false;
            s_telemetry.fully_charged = true;
        } else {
            s_telemetry.charging = true;
            s_telemetry.fully_charged = false;
        }
        s_telemetry.runtime_secs = 65535; // Effectively infinite on AC
    } else {
        s_telemetry.charging = false;
        s_telemetry.fully_charged = false;
        s_telemetry.discharging = true;

        // Estimate runtime: (Remaining mAh / Current mA) * 3600
        // Typical Pi 5 load ~ 2000mA
        uint32_t rem_mah = ((uint32_t)s_telemetry.soc_pct * BATTERY_DESIGN_CAP_MAH) / 100;
        uint32_t load_ma = s_telemetry.current_out_ma > 500 ? s_telemetry.current_out_ma : 2000;
        s_telemetry.runtime_secs = (uint16_t)((rem_mah * 3600) / load_ma);
    }

    // Safety and warning flags
    s_telemetry.low_battery_warn = (!s_telemetry.ac_present && s_telemetry.soc_pct <= 20);
    s_telemetry.shutdown_imminent = (!s_telemetry.ac_present && s_telemetry.soc_pct <= 5);
    s_telemetry.fault_undervoltage = (s_telemetry.v_bat_mv <= BATTERY_CUTOFF_MV);
    s_telemetry.fault_overtemp = (s_telemetry.temp_c >= 55);
}

const telemetry_data_t* telemetry_get(void) {
    return &s_telemetry;
}

//--------------------------------------------------------------------+
// Simulation Engine
//--------------------------------------------------------------------+

void telemetry_sim_enable(bool enable) {
    s_telemetry.sim_mode_active = enable;
}

void telemetry_sim_set_ac(bool ac_connected) {
    s_telemetry.ac_present = ac_connected;
    s_telemetry.v_bus_in_mv = ac_connected ? 15000 : 0;
    if (ac_connected) {
        s_telemetry.discharging = false;
        s_telemetry.charging = (s_telemetry.soc_pct < 100);
    } else {
        s_telemetry.charging = false;
        s_telemetry.discharging = true;
    }
}

void telemetry_sim_set_soc(uint8_t soc_pct) {
    if (soc_pct > 100) soc_pct = 100;
    s_telemetry.soc_pct = soc_pct;
    // Map SoC to approximate pack mV
    s_telemetry.v_bat_mv = 6000 + (uint16_t)(((uint32_t)soc_pct * 2400) / 100);
    s_telemetry.low_battery_warn = (!s_telemetry.ac_present && s_telemetry.soc_pct <= 20);
    s_telemetry.shutdown_imminent = (!s_telemetry.ac_present && s_telemetry.soc_pct <= 5);
}

void telemetry_sim_step(uint32_t delta_ms) {
    if (!s_telemetry.sim_mode_active) return;

    if (!s_telemetry.ac_present) {
        // Simulate discharge: 1% drain per 2000ms
        static uint32_t acc_ms = 0;
        acc_ms += delta_ms;
        if (acc_ms >= 2000 && s_telemetry.soc_pct > 0) {
            acc_ms -= 2000;
            telemetry_sim_set_soc(s_telemetry.soc_pct - 1);
        }
    } else if (s_telemetry.charging && s_telemetry.soc_pct < 100) {
        // Simulate charge: 1% gain per 1000ms
        static uint32_t chg_ms = 0;
        chg_ms += delta_ms;
        if (chg_ms >= 1000) {
            chg_ms -= 1000;
            telemetry_sim_set_soc(s_telemetry.soc_pct + 1);
        }
    }
}
