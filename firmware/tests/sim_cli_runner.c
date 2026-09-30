#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include "telemetry.h"
#include "power_mgr.h"
#include "cli.h"

int main(void) {
    telemetry_init();
    power_mgr_init();
    cli_init();

    // Set non-blocking or standard line input for CLI simulation
    char line[128];
    while (fgets(line, sizeof(line), stdin) != NULL) {
        for (size_t i = 0; line[i] != '\0'; i++) {
            cli_process_char(line[i]);
        }
    }
    return 0;
}
