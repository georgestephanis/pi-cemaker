#ifndef POWER_MGR_H_
#define POWER_MGR_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Hardware GPIO pin definitions
#define PIN_5V_EN               15 // Controls TPS55289 / 5V Buck-Boost Enable (HIGH = ON, LOW = CUT)
#define PIN_PWR_BTN             14 // Open-drain gate to Raspberry Pi 5 PWR_BTN header
#define PIN_LED_PWR             16 // Green: Power indicator
#define PIN_LED_BAT             17 // Amber/Blue: Battery charging / discharging indicator
#define PIN_LED_FAULT           18 // Red: Fault / Over-temp / Low battery warning
#define PIN_USER_BTN            19 // Momentary push button (active LOW with pull-up)

// Power manager timing constants
#define SHUTDOWN_GRACE_PERIOD_MS 45000 // 45 seconds for Linux OS clean halt before 5V rail cut
#define PWR_BTN_PULSE_MS         500   // 500 ms button press pulse for Pi 5 power button

// Power state machine states
typedef enum {
    PWR_STATE_INIT = 0,
    PWR_STATE_MAINS_CHARGING,       // Mains AC present, battery charging, 5V rail ON
    PWR_STATE_MAINS_FULL,           // Mains AC present, battery full, float, 5V rail ON
    PWR_STATE_BATTERY_DISCHARGING,  // Mains AC absent, running on 2S battery, 5V rail ON
    PWR_STATE_BATTERY_LOW,          // Mains absent, battery < 20%, 5V rail ON, warning flag set
    PWR_STATE_SHUTDOWN_PENDING,     // Battery < 5%, ShutdownImminent asserted, 45s countdown active
    PWR_STATE_POWER_CUT,            // 45s expired: 5V rail CUT to kill PMIC "zombie halt" drain (~1.5-2W)
    PWR_STATE_DORMANT_SLEEP,        // Battery < 6.0V cutoff: RP2040 dormant sleep (<100uA) until AC returns
    PWR_STATE_REBOOTING             // AC restored: 5V rail re-energized, cold-booting Pi 5
} power_state_t;

// API functions
void power_mgr_init(void);
void power_mgr_tick(uint32_t delta_ms);
power_state_t power_mgr_get_state(void);
const char* power_mgr_get_state_str(power_state_t state);
uint32_t power_mgr_get_countdown_remaining_ms(void);

// Manual control overrides
void power_mgr_force_5v_enable(bool enable);
void power_mgr_pulse_pi_power_button(void);
void power_mgr_request_clean_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* POWER_MGR_H_ */
