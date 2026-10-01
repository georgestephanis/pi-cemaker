#include "power_mgr.h"
#include "telemetry.h"

#if defined(PICO_ON_DEVICE) && PICO_ON_DEVICE
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/sync.h"
#include "hardware/rosc.h"
#include "hardware/clocks.h"
#endif

static power_state_t s_pwr_state = PWR_STATE_INIT;
static uint32_t s_countdown_timer_ms = 0;
static uint32_t s_pwr_btn_pulse_timer_ms = 0;
static uint32_t s_led_blink_timer_ms = 0;
static bool s_manual_cut = false;
static uint32_t s_off_timer_ms = 0; // time the rail has been held off in POWER_CUT
static bool s_led_toggle = false;

static void set_gpio_output(unsigned int pin, bool high) {
#if defined(PICO_ON_DEVICE) && PICO_ON_DEVICE
    gpio_put(pin, high ? 1 : 0);
#else
    (void)pin;
    (void)high;
#endif
}

void power_mgr_init(void) {
#if defined(PICO_ON_DEVICE) && PICO_ON_DEVICE
    // Initialize 5V Regulator Enable (Active HIGH)
    gpio_init(PIN_5V_EN);
    gpio_set_dir(PIN_5V_EN, GPIO_OUT);
    gpio_put(PIN_5V_EN, 1); // Enable 5.1V output to Raspberry Pi 5 by default

    // Initialize Pi 5 Power Button Open-Drain Header
    gpio_init(PIN_PWR_BTN);
    gpio_set_dir(PIN_PWR_BTN, GPIO_OUT);
    gpio_put(PIN_PWR_BTN, 0); // Keep floating / open-drain inactive

    // Status LEDs
    gpio_init(PIN_LED_PWR);
    gpio_set_dir(PIN_LED_PWR, GPIO_OUT);
    gpio_put(PIN_LED_PWR, 1);

    gpio_init(PIN_LED_BAT);
    gpio_set_dir(PIN_LED_BAT, GPIO_OUT);
    gpio_put(PIN_LED_BAT, 0);

    gpio_init(PIN_LED_FAULT);
    gpio_set_dir(PIN_LED_FAULT, GPIO_OUT);
    gpio_put(PIN_LED_FAULT, 0);

    // User push button
    gpio_init(PIN_USER_BTN);
    gpio_set_dir(PIN_USER_BTN, GPIO_IN);
    gpio_pull_up(PIN_USER_BTN);
#endif

    s_manual_cut = false;
    s_pwr_state = PWR_STATE_MAINS_CHARGING;
    s_countdown_timer_ms = 0;
}

static void rail_write(bool enable) {
    set_gpio_output(PIN_5V_EN, enable);
    set_gpio_output(PIN_LED_PWR, enable);
}

// Used by the state machine. Ignored while a manual cut is in force.
static void rail_set(bool enable) {
    if (s_manual_cut) return;
    rail_write(enable);
}

// Manual control (CLI). Cutting the rail holds it off against the state machine
// until the rail is manually re-enabled; enabling returns control to automatic.
void power_mgr_force_5v_enable(bool enable) {
    s_manual_cut = !enable;
    rail_write(enable);
}

bool power_mgr_manual_cut_active(void) {
    return s_manual_cut;
}

void power_mgr_pulse_pi_power_button(void) {
    s_pwr_btn_pulse_timer_ms = PWR_BTN_PULSE_MS;
    set_gpio_output(PIN_PWR_BTN, 1); // Pull open-drain low (active)
}

void power_mgr_request_clean_shutdown(void) {
    if (s_pwr_state != PWR_STATE_SHUTDOWN_PENDING && s_pwr_state != PWR_STATE_POWER_CUT) {
        s_pwr_state = PWR_STATE_SHUTDOWN_PENDING;
        s_countdown_timer_ms = SHUTDOWN_GRACE_PERIOD_MS;
    }
}

power_state_t power_mgr_get_state(void) {
    return s_pwr_state;
}

const char* power_mgr_get_state_str(power_state_t state) {
    switch (state) {
        case PWR_STATE_INIT:                return "INITIALIZING";
        case PWR_STATE_MAINS_CHARGING:      return "MAINS_CHARGING";
        case PWR_STATE_MAINS_FULL:          return "MAINS_FULL";
        case PWR_STATE_BATTERY_DISCHARGING: return "BATTERY_DISCHARGING";
        case PWR_STATE_BATTERY_LOW:         return "BATTERY_LOW";
        case PWR_STATE_SHUTDOWN_PENDING:    return "SHUTDOWN_COUNTDOWN";
        case PWR_STATE_POWER_CUT:           return "POWER_CUT_ZOMBIE_PREVENTION";
        case PWR_STATE_DORMANT_SLEEP:       return "DORMANT_DEEP_SLEEP";
        case PWR_STATE_REBOOTING:           return "AUTO_REBOOTING";
        default:                            return "UNKNOWN";
    }
}

uint32_t power_mgr_get_countdown_remaining_ms(void) {
    return s_countdown_timer_ms;
}

void power_mgr_tick(uint32_t delta_ms) {
    const telemetry_data_t* t = telemetry_get();

    // 1. Handle Pi 5 Power Button pulse timeout
    if (s_pwr_btn_pulse_timer_ms > 0) {
        if (delta_ms >= s_pwr_btn_pulse_timer_ms) {
            s_pwr_btn_pulse_timer_ms = 0;
            set_gpio_output(PIN_PWR_BTN, 0); // Release button
        } else {
            s_pwr_btn_pulse_timer_ms -= delta_ms;
        }
    }

    // 2. LED Animation timer
    s_led_blink_timer_ms += delta_ms;
    if (s_led_blink_timer_ms >= 250) {
        s_led_blink_timer_ms = 0;
        s_led_toggle = !s_led_toggle;
    }

    // 3. State Machine Transitions
    switch (s_pwr_state) {
        case PWR_STATE_INIT:
        case PWR_STATE_MAINS_CHARGING:
        case PWR_STATE_MAINS_FULL:
            rail_set(true);
            if (!t->ac_present) {
                // AC power lost: switchover to battery discharge
                s_pwr_state = t->low_battery_warn ? PWR_STATE_BATTERY_LOW : PWR_STATE_BATTERY_DISCHARGING;
            } else if (t->fully_charged) {
                s_pwr_state = PWR_STATE_MAINS_FULL;
            } else {
                s_pwr_state = PWR_STATE_MAINS_CHARGING;
            }
            break;

        case PWR_STATE_BATTERY_DISCHARGING:
        case PWR_STATE_BATTERY_LOW:
            rail_set(true);
            if (t->ac_present) {
                // Mains restored
                s_pwr_state = PWR_STATE_MAINS_CHARGING;
            } else if (t->shutdown_imminent) {
                // Critical low battery (<5%): Initiate 45-second shutdown countdown
                s_pwr_state = PWR_STATE_SHUTDOWN_PENDING;
                s_countdown_timer_ms = SHUTDOWN_GRACE_PERIOD_MS;
            } else if (t->low_battery_warn) {
                s_pwr_state = PWR_STATE_BATTERY_LOW;
            }
            break;

        case PWR_STATE_SHUTDOWN_PENDING:
            rail_set(true); // Keep 5.1V rail energized while Linux halts
            // The OS has already been told to shut down and may be halted with 5V
            // still applied, so the countdown always completes even if mains returns.
            // The rail is then cycled so the Pi 5 PMIC cold-boots.
            if (delta_ms >= s_countdown_timer_ms) {
                s_countdown_timer_ms = 0;
                s_off_timer_ms = 0;
                s_pwr_state = PWR_STATE_POWER_CUT;
                rail_set(false);
            } else {
                s_countdown_timer_ms -= delta_ms;
            }
            break;

        case PWR_STATE_POWER_CUT:
            // 5V rail is CUT. Pi 5 PMIC is unpowered.
            rail_set(false);
            s_off_timer_ms += delta_ms;

            if (t->ac_present) {
                // Mains is back, but hold the rail off long enough for the output caps
                // to discharge and the PMIC to reset before re-energizing.
                if (s_off_timer_ms >= POWER_CUT_MIN_OFF_MS) {
                    s_pwr_state = PWR_STATE_REBOOTING;
                    rail_set(true);
                }
            } else if (t->v_bat_mv <= BATTERY_CUTOFF_MV) {
                // Battery depleted to cutoff limit (6.0V). Enter dormant mode to prevent cell destruction
                s_pwr_state = PWR_STATE_DORMANT_SLEEP;
            }
            break;

        case PWR_STATE_REBOOTING:
            // Brief stabilization period before returning to normal charging state
            rail_set(true);
            s_pwr_state = PWR_STATE_MAINS_CHARGING;
            break;

        case PWR_STATE_DORMANT_SLEEP:
            rail_set(false);
            set_gpio_output(PIN_LED_PWR, 0);
            set_gpio_output(PIN_LED_BAT, 0);
            set_gpio_output(PIN_LED_FAULT, 0);
#if defined(PICO_ON_DEVICE) && PICO_ON_DEVICE
            // On RP2040, shut down clocks and sleep until VBUS edge or button
            // rosc_set_dormant();
#endif
            if (t->ac_present) {
                s_pwr_state = PWR_STATE_REBOOTING;
            }
            break;
    }

    // 4. Update status LEDs according to state
    if (s_pwr_state != PWR_STATE_DORMANT_SLEEP) {
        if (s_pwr_state == PWR_STATE_MAINS_CHARGING) {
            set_gpio_output(PIN_LED_BAT, s_led_toggle); // Slow blink
            set_gpio_output(PIN_LED_FAULT, 0);
        } else if (s_pwr_state == PWR_STATE_MAINS_FULL) {
            set_gpio_output(PIN_LED_BAT, 1); // Solid on
            set_gpio_output(PIN_LED_FAULT, 0);
        } else if (s_pwr_state == PWR_STATE_BATTERY_DISCHARGING) {
            set_gpio_output(PIN_LED_BAT, s_led_toggle);
            set_gpio_output(PIN_LED_FAULT, 0);
        } else if (s_pwr_state == PWR_STATE_BATTERY_LOW || s_pwr_state == PWR_STATE_SHUTDOWN_PENDING) {
            set_gpio_output(PIN_LED_FAULT, s_led_toggle); // Fast blink warning
        } else if (s_pwr_state == PWR_STATE_POWER_CUT) {
            set_gpio_output(PIN_LED_FAULT, 1); // Solid fault / halted
        }
    }
}
