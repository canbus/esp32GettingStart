# smartConfig
1. 概述
   SmartConfig是TI开发的一种配置技术，用于将新的Wi-Fi设备连接到Wi-Fi网络。它使用移动应用程序将网络凭据从智能手机或平板电脑广播到未配置的Wi-Fi设备。
   该技术的优点是设备不需要直接知道接入点（AP）的SSID或密码。此信息是使用智能手机提供的。
2. app:https://github.com/EspressifApp/EsptouchForAndroid/releases/tag/v2.3.2 
   airKiss二微码 <br><img src="img\airkissQrcode.png">
2. API说明
    SmartConfig 接口位于 esp_wifi/include/esp_smartconfig.h
    esp_smartconfig_set_type
    esp_smartconfig_start
    esp_smartconfig_stop
3. 大致流程<br><img src="img\smatconfigflow.png" >
4. 编程步骤
   1. 参照(wifi_station)中的介绍把wifi置成station模式.并添加`SC_EVENT`事件注册
       `esp_event_handler_register(SC_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL);`
   2. 建立event_handler用于处理WIFI事件
      1. 在`WIFI_EVENT_STA_START`事件中启动smartconfig任务
        ```c
            case WIFI_EVENT_STA_START:  // STA模式启动
                xTaskCreate(smartconfig_task, "smartconfig_task", 4096, NULL, 3, NULL);
                break;
        ``` 
      2. 在`SC_EVENT`的`SC_EVENT_GOT_SSID_PSWD`事件中保存密码,并连接到ap
         ```c
            if (event_base == SC_EVENT && event_id == SC_EVENT_GOT_SSID_PSWD) {
            ESP_LOGI(TAG, "Got SSID and password");

            smartconfig_event_got_ssid_pswd_t *evt = (smartconfig_event_got_ssid_pswd_t *)event_data;
            wifi_config_t wifi_config;
            uint8_t ssid[33] = { 0 };
            uint8_t password[65] = { 0 };
            uint8_t rvd_data[33] = { 0 };

            bzero(&wifi_config, sizeof(wifi_config_t));
            memcpy(wifi_config.sta.ssid, evt->ssid, sizeof(wifi_config.sta.ssid));
            memcpy(wifi_config.sta.password, evt->password, sizeof(wifi_config.sta.password));
            wifi_config.sta.bssid_set = evt->bssid_set;
            if (wifi_config.sta.bssid_set == true) {
                memcpy(wifi_config.sta.bssid, evt->bssid, sizeof(wifi_config.sta.bssid));
            }

            memcpy(ssid, evt->ssid, sizeof(evt->ssid));
            memcpy(password, evt->password, sizeof(evt->password));
            ESP_LOGI(TAG, "SSID:%s", ssid);
            ESP_LOGI(TAG, "PASSWORD:%s", password);
            
            ESP_ERROR_CHECK( esp_wifi_disconnect() );
            ESP_ERROR_CHECK( esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
            esp_wifi_connect();
         ```
   3. 新建一个任务,用于处理smartconfig
      ```c
      static void smartconfig_task(void * parm)
        {
            EventBits_t uxBits;
            ESP_ERROR_CHECK( esp_smartconfig_set_type(SC_TYPE_ESPTOUCH_AIRKISS) );
            smartconfig_start_config_t cfg = SMARTCONFIG_START_CONFIG_DEFAULT();
            ESP_ERROR_CHECK( esp_smartconfig_start(&cfg) );
            while (1) {
                uxBits = xEventGroupWaitBits(s_wifi_event_group, CONNECTED_BIT | ESPTOUCH_DONE_BIT, true, false, portMAX_DELAY);
                if(uxBits & CONNECTED_BIT) {
                    ESP_LOGI(TAG, "WiFi Connected to ap");
                }
                if(uxBits & ESPTOUCH_DONE_BIT) {
                    ESP_LOGI(TAG, "smartconfig over");
                    esp_smartconfig_stop();
                    vTaskDelete(NULL);
                }
            }
        }
      ```
5. 示例
```c
#include <stdio.h>

#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "string.h"
#include "esp_log.h"
#include "esp_smartconfig.h"
#include "freertos/event_groups.h"

#define TAG "main"
static EventGroupHandle_t s_wifi_event_group;
static const int CONNECTED_BIT = BIT0;
static const int ESPTOUCH_DONE_BIT = BIT1;

static void smartconfig_task(void * parm)
{
    EventBits_t uxBits;
    ESP_ERROR_CHECK( esp_smartconfig_set_type(SC_TYPE_ESPTOUCH_AIRKISS) );
    smartconfig_start_config_t cfg = SMARTCONFIG_START_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_smartconfig_start(&cfg) );
    while (1) {
        uxBits = xEventGroupWaitBits(s_wifi_event_group, CONNECTED_BIT | ESPTOUCH_DONE_BIT, true, false, portMAX_DELAY);
        if(uxBits & CONNECTED_BIT) {
            ESP_LOGI(TAG, "WiFi Connected to ap");
        }
        if(uxBits & ESPTOUCH_DONE_BIT) {
            ESP_LOGI(TAG, "smartconfig over");
            esp_smartconfig_stop();
            vTaskDelete(NULL);
        }
    }
}
static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id,void* event_data)
{
    ESP_LOGI("", "event_base:%s， event_id：%d\r\n", event_base, event_id);
    if (event_base == WIFI_EVENT) {
        switch (event_id) {
        case WIFI_EVENT_STA_START:  // STA模式启动
            xTaskCreate(smartconfig_task, "smartconfig_task", 4096, NULL, 3, NULL);
            break;
        case WIFI_EVENT_STA_DISCONNECTED:
            esp_wifi_connect();
            xEventGroupClearBits(s_wifi_event_group, CONNECTED_BIT);
            break;
        }
    }
    if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP){
        xEventGroupSetBits(s_wifi_event_group, CONNECTED_BIT);
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI("", "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
    }
    if (event_base == SC_EVENT && event_id == SC_EVENT_SCAN_DONE) {
        ESP_LOGI(TAG, "Scan done");
    } else if (event_base == SC_EVENT && event_id == SC_EVENT_FOUND_CHANNEL) {
        ESP_LOGI(TAG, "Found channel");
    } else if (event_base == SC_EVENT && event_id == SC_EVENT_GOT_SSID_PSWD) {
        ESP_LOGI(TAG, "Got SSID and password");

        smartconfig_event_got_ssid_pswd_t *evt = (smartconfig_event_got_ssid_pswd_t *)event_data;
        wifi_config_t wifi_config;
        uint8_t ssid[33] = { 0 };
        uint8_t password[65] = { 0 };
        uint8_t rvd_data[33] = { 0 };

        bzero(&wifi_config, sizeof(wifi_config_t));
        memcpy(wifi_config.sta.ssid, evt->ssid, sizeof(wifi_config.sta.ssid));
        memcpy(wifi_config.sta.password, evt->password, sizeof(wifi_config.sta.password));
        wifi_config.sta.bssid_set = evt->bssid_set;
        if (wifi_config.sta.bssid_set == true) {
            memcpy(wifi_config.sta.bssid, evt->bssid, sizeof(wifi_config.sta.bssid));
        }

        memcpy(ssid, evt->ssid, sizeof(evt->ssid));
        memcpy(password, evt->password, sizeof(evt->password));
        ESP_LOGI(TAG, "SSID:%s", ssid);
        ESP_LOGI(TAG, "PASSWORD:%s", password);
        if (evt->type == SC_TYPE_ESPTOUCH_V2) {
            ESP_ERROR_CHECK( esp_smartconfig_get_rvd_data(rvd_data, sizeof(rvd_data)) );
            ESP_LOGI(TAG, "RVD_DATA:");
            for (int i=0; i<33; i++) {
                printf("%02x ", rvd_data[i]);
            }
            printf("\n");
        }

        ESP_ERROR_CHECK( esp_wifi_disconnect() );
        ESP_ERROR_CHECK( esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
        esp_wifi_connect();
    } else if (event_base == SC_EVENT && event_id == SC_EVENT_SEND_ACK_DONE) {
        xEventGroupSetBits(s_wifi_event_group, ESPTOUCH_DONE_BIT);
    }
}
void app_main(void)
{
    s_wifi_event_group = xEventGroupCreate();

    nvs_flash_init();
    esp_netif_init();
    
    esp_event_loop_create_default();  // 创建一个默认的事件循环
    
    esp_netif_create_default_wifi_sta();//必须放在esp_event_loop_create_default()后面,否则收不到IP_EVENT_STA_GOT_IP

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    esp_wifi_set_mode(WIFI_MODE_STA);

    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, NULL);
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP,&event_handler, NULL, NULL);
    esp_event_handler_register(SC_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL);
    

    // wifi_config_t conf={.sta.ssid="test",
    //                     .sta.password="123012345",
    //                     .sta.threshold.authmode= WIFI_AUTH_WPA2_PSK,};
    // esp_wifi_set_config(WIFI_IF_STA, &conf);
    esp_wifi_start();
    
}
```