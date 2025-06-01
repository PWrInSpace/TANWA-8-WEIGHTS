#ifndef PWRINSPACE_CONSOLE_H_
#define PWRINSPACE_CONSOLE_H_

#include "esp_console.h"

// For some serial monitors this lib is crashing
// Works with cutecom ...
// Print giga good message to default stream
#define CONSOLE_WRITE_G(format, ...) printf(" # G: "format"\n", ##__VA_ARGS__)

// Print error message to default stream
#define CONSOLE_WRITE_E(format, ...) printf(" # E: "format"\n", ##__VA_ARGS__)

// Print to default stream
#define CONSOLE_WRITE(format, ...) printf(" # "format"\n", ##__VA_ARGS__)

esp_err_t console_init(void);
esp_err_t console_register_commands(esp_console_cmd_t *commands, size_t number_of_cmd);
esp_err_t console_deinit(void);

#endif /* PWRINSPACE_CONSOLE_H_ */