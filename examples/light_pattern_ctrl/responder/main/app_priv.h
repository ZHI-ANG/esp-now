#ifndef APP_PRIV_H
#define APP_PRIV_H

#include <sdkconfig.h>
#include <esp_err.h>
#include <esp_types.h>
#include <hal/gpio_types.h>
#include "esp_eth_spec.h"
#include "hal/gpio_types.h"

#define RESPONDER_PREFIX                CONFIG_RESPONDER_PREFIX
#define INITIATOR_PREFIX                CONFIG_INITIATOR_PREFIX
#define GROUP_NAME                      CONFIG_GROUP_NAME

#define RESPONDER_NAME RESPONDER_PREFIX GROUP_NAME
#define INITIATOR_NAME INITIATOR_PREFIX GROUP_NAME

/* the binding table of each initiator and corresponding gpio pin */
typedef struct {
    gpio_num_t gpio_pin;
    uint8_t mac_addr[ETH_ADDR_LEN];
} gpio_bind_t;

typedef enum {
    OFF,
    ON,
    INVALID,
    NUM_CMD,
} command_t;

void app_driver_gpio_init(gpio_num_t *gpio_list, size_t len);
void app_driver_gpio_set_onoff(int pin_num, command_t cmd);

#endif /* APP_PRIV_H */