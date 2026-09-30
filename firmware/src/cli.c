#include "cli.h"
#include "telemetry.h"
#include "power_mgr.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#if defined(PICO_ON_DEVICE) && PICO_ON_DEVICE
#include "pico/bootrom.h"
#include "tusb.h"
#endif

#define CLI_BUFFER_SIZE 64
static char s_cli_buf[CLI_BUFFER_SIZE];
static uint8_t s_cli_len = 0;

static void cli_puts(const char* str) {
#if defined(PICO_ON_DEVICE) && PICO_ON_DEVICE
    if (tud_cdc_connected()) {
        tud_cdc_write_str(str);
        tud_cdc_write_flush();
    }
#else
    printf("%s", str);
    fflush(stdout);
#endif
}

void cli_print_prompt(void) {
    cli_puts("\r\npi-cemaker> ");
}

void cli_init(void) {
    s_cli_len = 0;
    cli_puts("\r\n========================================\r\n");
    cli_puts("  Pi-cemaker 2S UPS Telemetry CLI\r\n");
    cli_puts("  Type 'help' for a list of commands.\r\n");
    cli_puts("========================================\r\n");
    cli_print_prompt();
}

static void print_status(void) {
    const telemetry_data_t* t = telemetry_get();
    power_state_t state = power_mgr_get_state();
    char buf[128];

    cli_puts("\r\n--- Pi-cemaker UPS Telemetry Status ---\r\n");
    snprintf(buf, sizeof(buf), "State:           %s\r\n", power_mgr_get_state_str(state));
    cli_puts(buf);

    if (state == PWR_STATE_SHUTDOWN_PENDING) {
        snprintf(buf, sizeof(buf), "Shutdown Timer:  %u ms remaining before 5V rail cut\r\n",
                 (unsigned int)power_mgr_get_countdown_remaining_ms());
        cli_puts(buf);
    }

    snprintf(buf, sizeof(buf), "AC Wall Input:   %s (%u mV)\r\n",
             t->ac_present ? "CONNECTED" : "DISCONNECTED", t->v_bus_in_mv);
    cli_puts(buf);

    snprintf(buf, sizeof(buf), "Battery Pack:    %u mV | SoC: %u%% | State: %s\r\n",
             t->v_bat_mv, t->soc_pct,
             t->charging ? "CHARGING" : (t->discharging ? "DISCHARGING" : "IDLE/FULL"));
    cli_puts(buf);

    snprintf(buf, sizeof(buf), "5.1V Pi Output:  %u mV | Pi Current: ~%u mA\r\n",
             t->v_out_mv, t->current_out_ma);
    cli_puts(buf);

    snprintf(buf, sizeof(buf), "Estimated Run:   %u seconds (~%u min)\r\n",
             t->runtime_secs, t->runtime_secs / 60);
    cli_puts(buf);

    snprintf(buf, sizeof(buf), "Cell Temp:       %d C\r\n", t->temp_c);
    cli_puts(buf);

    snprintf(buf, sizeof(buf), "Flags:           LowBatWarn=%d, ShutdownImminent=%d, SimMode=%d\r\n",
             t->low_battery_warn, t->shutdown_imminent, t->sim_mode_active);
    cli_puts(buf);
}

static void handle_command(char* cmd) {
    // Strip leading whitespace
    while (*cmd == ' ') cmd++;
    if (*cmd == '\0') return;

    if (strcmp(cmd, "help") == 0) {
        cli_puts("\r\nAvailable commands:\r\n");
        cli_puts("  status            - Print real-time voltages, SoC, and power state\r\n");
        cli_puts("  sim on            - Enable software simulation mode\r\n");
        cli_puts("  sim off           - Disable simulation mode (return to real ADC)\r\n");
        cli_puts("  sim ac <0|1>      - Simulate AC power loss (0) or connect (1)\r\n");
        cli_puts("  sim soc <0-100>   - Set simulated battery charge percentage\r\n");
        cli_puts("  shutdown          - Initiate clean OS shutdown sequence (45s window)\r\n");
        cli_puts("  powercut          - Force immediate 5V rail cut (test zombie mitigation)\r\n");
        cli_puts("  poweron           - Force 5V rail enable\r\n");
        cli_puts("  pulse-pwr         - Pulse Pi 5 hardware power button line\r\n");
        cli_puts("  reboot-bootloader - Reboot RP2040 into USB mass-storage bootloader\r\n");
    } else if (strcmp(cmd, "status") == 0) {
        print_status();
    } else if (strncmp(cmd, "sim", 3) == 0) {
        char* sub = cmd + 3;
        while (*sub == ' ') sub++;
        if (strcmp(sub, "on") == 0) {
            telemetry_sim_enable(true);
            cli_puts("\r\nSimulation mode ENABLED.\r\n");
        } else if (strcmp(sub, "off") == 0) {
            telemetry_sim_enable(false);
            cli_puts("\r\nSimulation mode DISABLED.\r\n");
        } else if (strncmp(sub, "ac", 2) == 0) {
            int ac = atoi(sub + 2);
            telemetry_sim_set_ac(ac != 0);
            char buf[64];
            snprintf(buf, sizeof(buf), "\r\nSimulated AC set to: %d\r\n", ac != 0);
            cli_puts(buf);
        } else if (strncmp(sub, "soc", 3) == 0) {
            int soc = atoi(sub + 3);
            telemetry_sim_set_soc((uint8_t)soc);
            char buf[64];
            snprintf(buf, sizeof(buf), "\r\nSimulated SoC set to: %d%%\r\n", soc);
            cli_puts(buf);
        } else {
            cli_puts("\r\nUnknown sim command. Usage: sim <on|off|ac 0/1|soc 0-100>\r\n");
        }
    } else if (strcmp(cmd, "shutdown") == 0) {
        cli_puts("\r\nTriggering clean shutdown sequence (45s grace period before 5V cut)...\r\n");
        power_mgr_request_clean_shutdown();
    } else if (strcmp(cmd, "powercut") == 0) {
        cli_puts("\r\nCutting 5.1V power rail to Raspberry Pi 5...\r\n");
        power_mgr_force_5v_enable(false);
    } else if (strcmp(cmd, "poweron") == 0) {
        cli_puts("\r\nRestoring 5.1V power rail to Raspberry Pi 5...\r\n");
        power_mgr_force_5v_enable(true);
    } else if (strcmp(cmd, "pulse-pwr") == 0) {
        cli_puts("\r\nPulsing Pi 5 PWR_BTN open-drain line...\r\n");
        power_mgr_pulse_pi_power_button();
    } else if (strcmp(cmd, "reboot-bootloader") == 0) {
        cli_puts("\r\nRebooting RP2040 into USB Bootloader mode...\r\n");
#if defined(PICO_ON_DEVICE) && PICO_ON_DEVICE
        reset_usb_boot(0, 0);
#endif
    } else {
        cli_puts("\r\nUnknown command. Type 'help' for command list.\r\n");
    }
}

void cli_process_char(char c) {
    if (c == '\r' || c == '\n') {
        cli_puts("\r\n");
        s_cli_buf[s_cli_len] = '\0';
        handle_command(s_cli_buf);
        s_cli_len = 0;
        cli_print_prompt();
    } else if (c == '\b' || c == 0x7F) { // Backspace / Delete
        if (s_cli_len > 0) {
            s_cli_len--;
            cli_puts("\b \b");
        }
    } else if (s_cli_len < CLI_BUFFER_SIZE - 1 && c >= 32 && c <= 126) {
        s_cli_buf[s_cli_len++] = c;
        char echo[2] = { c, '\0' };
        cli_puts(echo);
    }
}
