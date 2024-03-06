/* Get Start Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_system.h"
#include "esp_wifi.h"

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 4, 0)
#include "esp_mac.h"
#endif

#include "espnow.h"
#include "espnow_storage.h"
#include "espnow_utils.h"

#include "esp_eth_spec.h"
#include "hal/gpio_types.h"
#include "app_priv.h"

// You can modify these according to your boards.
#define UART_BAUD_RATE 115200
#define UART_PORT_NUM  0
#define UART_TX_IO     UART_PIN_NO_CHANGE
#define UART_RX_IO     UART_PIN_NO_CHANGE

#define RESPONDER_PREFIX                "$RESPONDER/"
#define GROUP_NAME                      "GROUP1"
#define RESPONDER_NAME RESPONDER_PREFIX GROUP_NAME

static const char *TAG = "app_main";

#define GPIO_LIST_LEN 6 /* 18, 19, 20, 21, 22, 23 */

static gpio_num_t gpio_list[GPIO_LIST_LEN] = {
    GPIO_NUM_18,
    GPIO_NUM_19,
    GPIO_NUM_20,
    GPIO_NUM_21,
    GPIO_NUM_22,
    GPIO_NUM_23,
};

static int next_gpio_idx; /* next gpio pin to be bounded to initiator */

/* the control table of each initiator bounded with gpio pin */
typedef struct {
    gpio_num_t gpio_pin;
    uint8_t mac_addr[ETH_ADDR_LEN];
} gpio_bind_t;

static gpio_bind_t s_gpio_bind_table[GPIO_LIST_LEN];

static void app_wifi_init()
{
    esp_event_loop_create_default();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
    ESP_ERROR_CHECK(esp_wifi_start());
}

static esp_err_t espnow_data_recv_cb(uint8_t *src_addr, void *data,
                                       size_t size, wifi_pkt_rx_ctrl_t *rx_ctrl)
{
    ESP_PARAM_CHECK(src_addr);
    ESP_PARAM_CHECK(data);
    ESP_PARAM_CHECK(size);
    ESP_PARAM_CHECK(rx_ctrl);

    static uint32_t count = 0;

    ESP_LOGI(TAG, "espnow_recv, <%" PRIu32 "> [" MACSTR "][%d][%d][%u]: %.*s",
             count++, MAC2STR(src_addr), rx_ctrl->channel, rx_ctrl->rssi, size, size, (char *)data);

    int pin = 0;
    for ( pin = 0; pin < GPIO_LIST_LEN; pin++ ) {
        if (s_gpio_bind_table[pin].mac_addr[0]){
            if (memcmp(s_gpio_bind_table[pin].mac_addr, src_addr, ETH_ADDR_LEN) == 0){
                break;
            }
        }
    }

    /* this incoming initiator is not bounded yet */
    if ( pin == GPIO_LIST_LEN ) {
        /* if we have more qouta in gpio list */
        if (next_gpio_idx < GPIO_LIST_LEN) {
            for ( pin = 0; pin < GPIO_LIST_LEN; pin++ ) {
                /* find a new slot */
                if (!s_gpio_bind_table[pin].mac_addr[0]) {
                    memcpy(s_gpio_bind_table[pin].mac_addr, src_addr, ETH_ADDR_LEN);
                    s_gpio_bind_table[pin].gpio_pin = gpio_list[next_gpio_idx];
                    break;
                }
            }
            next_gpio_idx += 1;
        }
    }

    if (pin == GPIO_LIST_LEN) {
        ESP_LOGE(TAG, "No more entry in GPIO Binding Table. Failed to bind " MACSTR "", MAC2STR(src_addr));
    }

    ESP_LOGI(TAG, "**************** GPIO Binding Table ****************");
    for (int idx=0; idx<GPIO_LIST_LEN; idx++) {
        if (s_gpio_bind_table[idx].mac_addr[0]) {
            ESP_LOGI(TAG, MACSTR"\t%d", MAC2STR(s_gpio_bind_table[idx].mac_addr), s_gpio_bind_table[idx].gpio_pin);
        }
    }
    ESP_LOGI(TAG, "++++++++++++++++ GPIO Binding Table ++++++++++++++++");

    int state = -1;
    if (strncmp(data, "ON", ESPNOW_DATA_LEN) == 0) {
        state = 1;
    }

    if (strncmp(data, "OFF", ESPNOW_DATA_LEN) == 0) {
        state = 0;
    }

    if (state == -1) {
        ESP_LOGE(TAG, "Received unrecognized command");
    } else {
        app_driver_gpio_set_level(s_gpio_bind_table[pin].gpio_pin, state);
    }

    return ESP_OK;
}

static void local_mac_addr_broadcast()
{
    static espnow_frame_head_t frame_head = {
        .retransmit_count = CONFIG_RETRY_NUM,
        .broadcast        = true,
    };

    while (true) {
        ESP_LOGI(TAG, "Broadcasting name %s to initiators", RESPONDER_NAME);
        espnow_send(ESPNOW_DATA_TYPE_DATA, ESPNOW_ADDR_BROADCAST, RESPONDER_NAME, strlen(RESPONDER_NAME)+1, &frame_head, portMAX_DELAY);
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}

void app_main()
{
    espnow_storage_init();

    app_driver_gpio_init(gpio_list, GPIO_LIST_LEN);

    app_wifi_init();

    espnow_config_t espnow_config = ESPNOW_INIT_CONFIG_DEFAULT();

    espnow_init(&espnow_config);

    xTaskCreate(local_mac_addr_broadcast, "mac_broad", 2048, NULL, 4, NULL);

    espnow_set_config_for_data_type(ESPNOW_DATA_TYPE_DATA, true, espnow_data_recv_cb);
}
