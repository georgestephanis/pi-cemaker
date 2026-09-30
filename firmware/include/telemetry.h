#ifndef TELEMETRY_H_
#define TELEMETRY_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Hardware pin definitions for RP2040 ADC
#define ADC_PIN_VBAT            26 // ADC0 (GPIO 26) - Battery voltage divider
#define ADC_PIN_VBUS_IN         27 // ADC1 (GPIO 27) - Input voltage divider
#define ADC_PIN_VOUT            28 // ADC2 (GPIO 28) - 5.1V output divider
#define ADC_PIN_TEMP            29 // ADC3 (GPIO 29) - 10k NTC Thermistor

// Resistor divider ratios (scaled for 0 - 3.0V ADC input range with 3.3V reference)
// V_BAT (max 8.4V): 100k / 33k divider -> scale factor ~ 4.030
#define VBAT_DIVIDER_RATIO      4.030f
// V_BUS_IN (max 21V): 100k / 15k divider -> scale factor ~ 7.667
#define VBUS_DIVIDER_RATIO      7.667f
// V_OUT (nominal 5.1V, max 6V): 100k / 100k divider -> scale factor ~ 2.000
#define VOUT_DIVIDER_RATIO      2.000f

// Battery limits for 2S Li-ion
#define BATTERY_MAX_MV          8400 // 4.20V per cell
#define BATTERY_NOMINAL_MV      7400 // 3.70V per cell
#define BATTERY_WARN_MV         6800 // ~3.40V per cell (~20% SoC)
#define BATTERY_CRITICAL_MV     6200 // ~3.10V per cell (~5% SoC, initiates shutdown)
#define BATTERY_CUTOFF_MV       6000 // 3.00V per cell (hardware cut-off limit)
#define BATTERY_DESIGN_CAP_MAH  3000 // 2S1P of 3000 mAh cells: series cells share charge, so pack capacity is one cell's (about 22 Wh)

// Telemetry state structure
typedef struct {
    uint16_t v_bat_mv;          // Filtered battery voltage in mV
    uint16_t v_bus_in_mv;       // Filtered USB-C input voltage in mV
    uint16_t v_out_mv;          // Filtered 5.1V output voltage in mV
    int16_t  temp_c;            // Cell temperature in degrees C
    int16_t  current_bat_ma;    // Current into/out of battery (+ charging, - discharging)
    uint16_t current_out_ma;    // Current delivered to Pi 5 (mA)
    uint8_t  soc_pct;           // State of charge (0 - 100%)
    uint16_t runtime_secs;      // Estimated seconds remaining until cutoff
    
    // Status flags
    bool ac_present;
    bool charging;
    bool discharging;
    bool fully_charged;
    bool low_battery_warn;
    bool shutdown_imminent;
    bool fault_overtemp;
    bool fault_undervoltage;

    // Simulation mode
    bool sim_mode_active;
} telemetry_data_t;

// API functions
void telemetry_init(void);
void telemetry_sample_tick(void); // Called periodically (e.g. 100 Hz or 10 Hz)
const telemetry_data_t* telemetry_get(void);

// Simulation controls (for testing host OS reaction without cutting physical wires)
void telemetry_sim_enable(bool enable);
void telemetry_sim_set_ac(bool ac_connected);
void telemetry_sim_set_soc(uint8_t soc_pct);
void telemetry_sim_step(uint32_t delta_ms);

#ifdef __cplusplus
}
#endif

#endif /* TELEMETRY_H_ */
