#include <app_priv.h>
#include <hal/gpio_ll.h>
#include <driver/gpio.h>
#include <esp_log.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define TAG "app_driver"

void app_driver_gpio_init(gpio_num_t *gpio_list, size_t len)
{
    uint64_t pin_mask = 0;
    for ( int idx=0; idx<len; idx++) {
        pin_mask |= 1 << gpio_list[idx];
    }

    ESP_LOGI(TAG, "pin_mask=%08x", (uint32_t)pin_mask);

    gpio_config_t io_conf;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = pin_mask;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.pull_down_en = 1;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);

    gpio_ll_set_level(&GPIO, GPIO_NUM_18, 1);
    gpio_ll_set_level(&GPIO, GPIO_NUM_19, 1);

    vTaskDelay(1000 / portTICK_PERIOD_MS);

    gpio_ll_set_level(&GPIO, GPIO_NUM_18, 0);
    gpio_ll_set_level(&GPIO, GPIO_NUM_19, 0);
}

void app_driver_gpio_set_onoff(int pin_num, command_t command)
{
    int level = 0;

    if (command == ON) {
        level = 1;
    }

    if (command == OFF) {
        level = 0;
    }

    gpio_ll_set_level(&GPIO, pin_num, level);

    ESP_LOGI(TAG, "set GPIO(%d) to %s", pin_num, level?"HIGH":"LOW");
}