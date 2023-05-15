# AP模式
## 步骤及API简介
1. 初始化nvs_flash:初始化nvs系统,因为wifi需要用到nfs,如果不初始化nvs,调用esp_wifi_init会出错
    `#include "nvs_flash.h"`
    `nvs_flash_init();`
2. 创建esp_netif: Initialize the underlying TCP/IP stack
    `#include "esp_wifi.h"`
    `esp_netif_init();`
3. 创建一个默认的事件循环:
   `esp_event_loop_create_default();`
4. 创建默认的wifi ap:
    `esp_netif_create_default_wifi_ap();`
5. 初始化WiFi,并把wifi置为ap模式
       ```c
        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        ESP_ERROR_CHECK(esp_wifi_init(&cfg));
        esp_wifi_set_mode(WIFI_MODE_AP);
       ```
6. 注册事件并处理事件:
    ```c
        esp_event_handler_instance_t wifi_handler;
        esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, &wifi_handler);
    ``` 
    ```c 新建处理事件函数
        static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id,void* event_data)
        {
            ESP_LOGI("", "event_base:%s， event_id：%d\r\n", event_base, event_id);
            if (event_base == WIFI_EVENT) {
                ...
            }
        }
    ```
7. 设置WIFI参数,并启动WIFI.主要是SSID,密码
   ```c
    wifi_config_t conf={.ap.ssid="esp32",
                        .ap.ssid_len = strlen("esp32"),
                        .ap.channel = 6,
                        .ap.max_connection = 3,
                        .ap.password="12345678",
                        .ap.authmode= WIFI_AUTH_WPA2_PSK,};
    esp_wifi_set_mode(WIFI_MODE_AP);
    esp_wifi_set_config(WIFI_IF_AP, &conf);
    esp_wifi_start();
   ```
8. 启动完wifi后,当有station接入时产生`WIFI_EVENT_AP_STACONNECTED`事件,可以获取到station的相关信息
   ```c
        case WIFI_EVENT_AP_STACONNECTED:
            { 
            wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
            ESP_LOGI("", "station "MACSTR" join, AID=%d", MAC2STR(event->mac), event->aid);
            }
            break;
   ```
9.  当station掉线后,会产生`WIFI_EVENT_AP_STADISCONNECTED`事件
    ```c
        case WIFI_EVENT_AP_STADISCONNECTED:
            {
            wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
            ESP_LOGI("", "station "MACSTR" leave, AID=%d", MAC2STR(event->mac), event->aid);
            }
            break;
    ```
10. 示例
```c
#include <stdio.h>

#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "string.h"
#include "esp_log.h"
#include "esp_wifi_types.h"

static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id,void* event_data)
{
    ESP_LOGI("", "event_base:%s， event_id：%d\r\n", event_base, event_id);
    if (event_base == WIFI_EVENT) {
        switch (event_id) {
        case WIFI_EVENT_AP_STACONNECTED:
            { 
            wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
            ESP_LOGI("", "station "MACSTR" join, AID=%d", MAC2STR(event->mac), event->aid);
            }
            break;
        case WIFI_EVENT_AP_STADISCONNECTED:
            {
            wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
            ESP_LOGI("", "station "MACSTR" leave, AID=%d", MAC2STR(event->mac), event->aid);
            }
            break;
        }
    }
    if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP){
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI("", "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
    }
}
void app_main(void)
{
    nvs_flash_init();
    esp_netif_init();
    
    esp_event_loop_create_default();  // 创建一个默认的事件循环
    
    esp_netif_create_default_wifi_ap();//

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    
    esp_event_handler_instance_t wifi_handler;
    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, NULL);


    wifi_config_t conf={.ap.ssid="esp32",
                        .ap.ssid_len = strlen("esp32"),
                        .ap.channel = 6,
                        .ap.max_connection = 3,
                        .ap.password="12345678",
                        .ap.authmode= WIFI_AUTH_WPA2_PSK,};
    esp_wifi_set_mode(WIFI_MODE_AP);
    esp_wifi_set_config(WIFI_IF_AP, &conf);
    esp_wifi_start();
    
}
```