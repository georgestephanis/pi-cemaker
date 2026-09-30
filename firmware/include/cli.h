#ifndef CLI_H_
#define CLI_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void cli_init(void);
void cli_process_char(char c);
void cli_print_prompt(void);

#ifdef __cplusplus
}
#endif

#endif /* CLI_H_ */
