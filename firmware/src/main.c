#include <stdio.h>
#include <string.h>

#if defined(PICO_ON_DEVICE) && PICO_ON_DEVICE
#include "pico/stdlib.h"
#include "pico/multicore.h"
#endif

#include "tusb.h"
#include "usb_descriptors.h"
#include "telemetry.h"
#include "power_mgr.h"
#include "cli.h"

// Track last sent status to avoid redundant USB HID packets
static hid_ups_status_report_t s_last_report = {0};
static uint32_t s_last_report_time_ms = 0;

//--------------------------------------------------------------------+
// Core 1 Entry: Telemetry Sampling & Power Management State Machine
//--------------------------------------------------------------------+
void core1_entry(void) {
    while (1) {
        telemetry_sample_tick();
        telemetry_sim_step(10);
        power_mgr_tick(10);
#if defined(PICO_ON_DEVICE) && PICO_ON_DEVICE
        sleep_ms(10);
#endif
    }
}

//--------------------------------------------------------------------+
// Core 0: USB Device Task, HID Reporting, and CDC CLI
//--------------------------------------------------------------------+

static void send_hid_ups_report(void) {
    if (!tud_hid_ready()) return;

    const telemetry_data_t* t = telemetry_get();
    power_state_t pstate = power_mgr_get_state();

    hid_ups_status_report_t report;
    report.report_id = REPORT_ID_UPS_STATUS;
    report.present_status = 0;

    if (t->ac_present)           report.present_status |= UPS_STATUS_AC_PRESENT;
    if (t->charging)             report.present_status |= UPS_STATUS_CHARGING;
    if (t->discharging)          report.present_status |= UPS_STATUS_DISCHARGING;
    if (t->fully_charged)        report.present_status |= UPS_STATUS_FULLY_CHARGED;
    if (t->low_battery_warn)     report.present_status |= UPS_STATUS_BELOW_LIMIT;
    if (t->shutdown_imminent || pstate == PWR_STATE_SHUTDOWN_PENDING) {
        report.present_status |= UPS_STATUS_SHUTDOWN_IMMINENT;
    }
    report.present_status |= UPS_STATUS_BATTERY_PRESENT;
    if (t->fault_overtemp || t->fault_undervoltage) {
        report.present_status |= UPS_STATUS_FAULT;
    }

    report.remaining_capacity = t->soc_pct;
    report.run_time_to_empty  = t->runtime_secs;
    report.voltage_mv         = t->v_bat_mv;

#if defined(PICO_ON_DEVICE) && PICO_ON_DEVICE
    uint32_t now = to_ms_since_boot(get_absolute_time());
#else
    static uint32_t now = 0;
    now += 10;
#endif

    // Send immediately if status flags or capacity changed, or at least every 1000ms
    bool changed = (memcmp(&report, &s_last_report, sizeof(report)) != 0);
    bool heartbeat = (now - s_last_report_time_ms >= 1000);

    if (changed || heartbeat) {
        if (tud_hid_report(REPORT_ID_UPS_STATUS, &report.present_status, sizeof(report) - 1)) {
            memcpy(&s_last_report, &report, sizeof(report));
            s_last_report_time_ms = now;
        }
    }
}

static void cdc_task(void) {
    if (tud_cdc_available()) {
        char buf[64];
        uint32_t count = tud_cdc_read(buf, sizeof(buf));
        for (uint32_t i = 0; i < count; i++) {
            cli_process_char(buf[i]);
        }
    }
}

int main(void) {
#if defined(PICO_ON_DEVICE) && PICO_ON_DEVICE
    stdio_init_all();
#endif

    // Initialize subsystems
    telemetry_init();
    power_mgr_init();

#if defined(PICO_ON_DEVICE) && PICO_ON_DEVICE
    // Launch Core 1 for real-time ADC and power state processing
    multicore_launch_core1(core1_entry);
#endif

    // Initialize TinyUSB stack
    tusb_init();
    cli_init();

    // Main Core 0 loop
    while (1) {
        tud_task(); // TinyUSB device task
        cdc_task(); // Process incoming CDC characters
        send_hid_ups_report(); // Send periodic UPS telemetry over HID
    }

    return 0;
}

//--------------------------------------------------------------------+
// TinyUSB HID Callbacks
//--------------------------------------------------------------------+

// Invoked when received GET_REPORT control request
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id,
                               hid_report_type_t report_type,
                               uint8_t* buffer, uint16_t reqlen) {
    (void) instance;
    (void) report_type;

    if (report_id == REPORT_ID_UPS_STATUS) {
        const telemetry_data_t* t = telemetry_get();
        hid_ups_status_report_t report;
        report.report_id = REPORT_ID_UPS_STATUS;
        report.present_status = 0;
        if (t->ac_present)           report.present_status |= UPS_STATUS_AC_PRESENT;
        if (t->charging)             report.present_status |= UPS_STATUS_CHARGING;
        if (t->discharging)          report.present_status |= UPS_STATUS_DISCHARGING;
        if (t->fully_charged)        report.present_status |= UPS_STATUS_FULLY_CHARGED;
        if (t->low_battery_warn)     report.present_status |= UPS_STATUS_BELOW_LIMIT;
        if (t->shutdown_imminent)    report.present_status |= UPS_STATUS_SHUTDOWN_IMMINENT;
        report.present_status |= UPS_STATUS_BATTERY_PRESENT;
        report.remaining_capacity = t->soc_pct;
        report.run_time_to_empty  = t->runtime_secs;
        report.voltage_mv         = t->v_bat_mv;

        uint16_t copy_len = (reqlen < sizeof(report)) ? reqlen : sizeof(report);
        memcpy(buffer, &report, copy_len);
        return copy_len;
    } else if (report_id == REPORT_ID_UPS_CONFIG) {
        hid_ups_config_report_t config;
        config.report_id = REPORT_ID_UPS_CONFIG;
        config.config_voltage_mv = BATTERY_NOMINAL_MV; // 7400 mV
        config.design_capacity_mah = BATTERY_DESIGN_CAP_MAH; // 6000 mAh

        uint16_t copy_len = (reqlen < sizeof(config)) ? reqlen : sizeof(config);
        memcpy(buffer, &config, copy_len);
        return copy_len;
    }

    return 0;
}

// Invoked when received SET_REPORT control request or output report
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id,
                           hid_report_type_t report_type,
                           uint8_t const* buffer, uint16_t bufsize) {
    (void) instance;
    (void) report_id;
    (void) report_type;
    (void) buffer;
    (void) bufsize;
}
