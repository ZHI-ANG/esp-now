#ifndef APP_PRIV_H
#define APP_PRIV_H

#include <sdkconfig.h>
#include <esp_err.h>
#include <esp_types.h>
#include <hal/gpio_types.h>

void app_driver_gpio_init(gpio_num_t *gpio_list, size_t len);
void app_driver_gpio_set_level(int pin_num, int level);

#endif /* APP_PRIV_H */