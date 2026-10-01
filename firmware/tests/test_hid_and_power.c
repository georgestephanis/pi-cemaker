#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "usb_descriptors.h"
#include "telemetry.h"
#include "power_mgr.h"
#include "cli.h"

static int test_count = 0;
static int pass_count = 0;

#define TEST_ASSERT(cond, msg) do { \
    test_count++; \
    if (cond) { \
        pass_count++; \
        printf("  [PASS] %s\n", msg); \
    } else { \
        printf("  [FAIL] %s (Line %d)\n", msg, __LINE__); \
    } \
} while(0)

void test_descriptor_structure(void) {
    printf("\n=== Test 1: HID Report Descriptor & Struct Alignment ===\n");

    // Check size of packed report structs
    TEST_ASSERT(sizeof(hid_ups_status_report_t) == 7, "hid_ups_status_report_t is exactly 7 bytes packed");
    TEST_ASSERT(sizeof(hid_ups_config_report_t) == 5, "hid_ups_config_report_t is exactly 5 bytes packed");
    TEST_ASSERT(hid_report_descriptor_len > 0, "HID report descriptor is populated");
    TEST_ASSERT(hid_report_descriptor[0] == 0x05 && hid_report_descriptor[1] == 0x84,
                "Usage Page is 0x84 (Power Device)");
}

void test_telemetry_and_soc(void) {
    printf("\n=== Test 2: Telemetry & State of Charge (SoC) Calculation ===\n");

    telemetry_init();
    telemetry_sim_enable(true);

    // Test 100% capacity at 8.4V
    telemetry_sim_set_soc(100);
    telemetry_sample_tick();
    const telemetry_data_t* t = telemetry_get();
    TEST_ASSERT(t->soc_pct == 100, "SoC is 100%");
    TEST_ASSERT(t->v_bat_mv >= 8350, "Battery voltage at 100% is >= 8.35V");
    TEST_ASSERT(!t->low_battery_warn, "No low battery warning at 100%");
    TEST_ASSERT(!t->shutdown_imminent, "No shutdown imminent at 100%");

    // Test 20% warning threshold
    telemetry_sim_set_ac(false);
    telemetry_sim_set_soc(20);
    telemetry_sample_tick();
    t = telemetry_get();
    TEST_ASSERT(t->discharging, "Discharging flag set when AC is false");
    TEST_ASSERT(t->low_battery_warn, "low_battery_warn asserted at 20% SoC");
    TEST_ASSERT(!t->shutdown_imminent, "shutdown_imminent NOT yet asserted at 20% SoC");

    // Test 5% critical threshold
    telemetry_sim_set_soc(5);
    telemetry_sample_tick();
    t = telemetry_get();
    TEST_ASSERT(t->low_battery_warn, "low_battery_warn asserted at 5% SoC");
    TEST_ASSERT(t->shutdown_imminent, "shutdown_imminent asserted at <= 5% SoC");
}

void test_power_manager_state_machine(void) {
    printf("\n=== Test 3: Power Management State Machine & Zombie Halt Prevention ===\n");

    telemetry_init();
    power_mgr_init();
    telemetry_sim_enable(true);

    // Initial state: AC present
    telemetry_sim_set_ac(true);
    telemetry_sim_set_soc(80);
    telemetry_sample_tick();
    power_mgr_tick(100);
    TEST_ASSERT(power_mgr_get_state() == PWR_STATE_MAINS_CHARGING,
                "State is MAINS_CHARGING when AC is present");

    // Simulate AC unplug
    telemetry_sim_set_ac(false);
    telemetry_sample_tick();
    power_mgr_tick(100);
    TEST_ASSERT(power_mgr_get_state() == PWR_STATE_BATTERY_DISCHARGING,
                "State transitions to BATTERY_DISCHARGING on AC loss");

    // Battery drops to 15% (warning)
    telemetry_sim_set_soc(15);
    telemetry_sample_tick();
    power_mgr_tick(100);
    TEST_ASSERT(power_mgr_get_state() == PWR_STATE_BATTERY_LOW,
                "State transitions to BATTERY_LOW at 15%");

    // Battery drops to critical 4% -> triggers shutdown countdown
    telemetry_sim_set_soc(4);
    telemetry_sample_tick();
    power_mgr_tick(100);
    TEST_ASSERT(power_mgr_get_state() == PWR_STATE_SHUTDOWN_PENDING,
                "State transitions to SHUTDOWN_PENDING at <= 5%");
    TEST_ASSERT(power_mgr_get_countdown_remaining_ms() > 40000,
                "Grace period starts at ~45 seconds");

    // Advance timer by 30 seconds: 5V rail should STILL BE ON while Linux halts
    power_mgr_tick(30000);
    TEST_ASSERT(power_mgr_get_state() == PWR_STATE_SHUTDOWN_PENDING,
                "State remains SHUTDOWN_PENDING during 45s countdown (5V rail preserved)");

    // Advance timer past 45 seconds: 5V rail must be CUT to prevent zombie halt
    power_mgr_tick(16000);
    TEST_ASSERT(power_mgr_get_state() == PWR_STATE_POWER_CUT,
                "State transitions to POWER_CUT after 45s (5V rail turned off)");

    // Mains power returns! Pi-cemaker must restore 5V rail and trigger auto cold-boot
    telemetry_sim_set_ac(true);
    telemetry_sample_tick();
    power_mgr_tick(100);
    TEST_ASSERT(power_mgr_get_state() == PWR_STATE_REBOOTING,
                "State transitions to REBOOTING when wall power is reconnected");

    power_mgr_tick(100);
    TEST_ASSERT(power_mgr_get_state() == PWR_STATE_MAINS_CHARGING,
                "State returns to MAINS_CHARGING after cold-boot power cycle");
}

void test_manual_override(void) {
    printf("\n=== Test 4: Manual 5V cut is not overwritten by the state machine ===\n");

    telemetry_init();
    telemetry_sim_enable(true);
    telemetry_sim_set_ac(true);
    power_mgr_init();

    power_mgr_force_5v_enable(false);
    for (int i = 0; i < 10; i++) power_mgr_tick(10);
    TEST_ASSERT(power_mgr_manual_cut_active(), "Manual cut persists across state machine ticks");

    power_mgr_force_5v_enable(true);
    TEST_ASSERT(!power_mgr_manual_cut_active(), "Re-enabling returns control to the state machine");

    power_mgr_force_5v_enable(false);
    power_mgr_init();
    TEST_ASSERT(!power_mgr_manual_cut_active(), "power_mgr_init clears a manual cut");
}

int main(void) {
    printf("====================================================\n");
    printf("  Running Pi-cemaker Firmware & HID Test Suite      \n");
    printf("====================================================\n");

    test_descriptor_structure();
    test_telemetry_and_soc();
    test_power_manager_state_machine();
    test_manual_override();

    printf("\n----------------------------------------------------\n");
    printf("Test Results: %d / %d tests passed (%.1f%%)\n",
           pass_count, test_count, (float)pass_count * 100.0f / (float)test_count);
    printf("====================================================\n");

    return (pass_count == test_count) ? 0 : 1;
}
