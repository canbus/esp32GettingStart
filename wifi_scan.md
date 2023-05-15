# WIFI 扫描附近AP/阻塞等待扫描结果/回调通知扫描结果
1. https://docs.espressif.com/projects/esp-idf/zh_CN/latest/esp32/api-reference/network/esp_wifi.html?highlight=esp_wifi_scan_start#_CPPv419esp_wifi_scan_startPK18wifi_scan_config_tb
2. ESP32 WIFI介绍
    Wi-Fi 库支持配置及监控 ESP32 Wi-Fi 连网功能。
    支持配置：
        基站模式（即 STA 模式或 Wi-Fi 客户端模式），此时 ESP32 连接到接入点 (AP)。
        AP 模式（即 Soft-AP 模式或接入点模式），此时基站连接到 ESP32。
        AP-STA 共存模式（ESP32 既是接入点，同时又作为基站连接到另外一个接入点）。
        上述模式的各种安全模式（WPA、WPA2 及 WEP 等）。
        扫描接入点（包括主动扫描及被动扫描）。
        使用混杂模式监控 IEEE802.11 Wi-Fi 数据包。
3. STA模式下扫描外部WiFi
   步骤及API简介
    1. 初始化nvs_flash:初始化nvs系统,因为wifi需要用到nfs,如果不初始化nvs,调用esp_wifi_init会出错
        `#include "nvs_flash.h"`
        `nvs_flash_init();`
    3. 创建esp_netif: Initialize the underlying TCP/IP stack
       `#include "esp_wifi.h"`
       `esp_netif_init();`
    4. 创建默认的wifi sta: Creates default WIFI STA. In case of any init error this API aborts.
       `esp_netif_create_default_wifi_sta();`
    5. 初始化、配置WiFi并启动
       ```c
        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        ESP_ERROR_CHECK(esp_wifi_init(&cfg));

        esp_wifi_set_mode(WIFI_MODE_STA);
        esp_wifi_start();
       ```
    6. 扫描AP,并解析
       相关的API函数
         1. esp_wifi_scan_start(const wifi_scan_config_t *config, bool block) //开始扫描WiFi，扫描附近所有可用的AP。
            *config可以指定SSID扫描某个AP,为NULL表示扫描全部WiFi
            bool；表示是否要阻塞。为true表示此函数将阻塞直到扫描完成。为false表示此函数将要立即返回（扫描结束通过事件循环来传递结果)
         2. esp_wifi_scan_get_ap_num(uint16_t *number) //获取扫描到的AP个数
         3. esp_wifi_scan_get_ap_records(uint16_t *number, wifi_ap_record_t *ap_records) //获取扫描到的AP
            *number类型为：uint16_t；【既可输入又可输出】输入读取扫描记录的期望数量，例如预先分配的数组长度。输出实际扫描到的AP个数
            *ap_records类型为：wifi_ap_record_t指针；表示存放扫描结果的数组
       ```c
        //wifi_scan_config_t scanCfg; 可以指定扫描某一个ssid,不指定的话,传null
        esp_wifi_scan_start(NULL, true);

        uint16_t number = 10;
        wifi_ap_record_t ap_info[10];  // 最多扫描10个
        uint16_t ap_count = 0;
        memset(ap_info, 0, sizeof(ap_info));

        esp_wifi_scan_get_ap_records(&number, ap_info);
        esp_wifi_scan_get_ap_num(&ap_count);
        printf("Aps count:%d\n", ap_count);
        for (int i = 0; i < 10 && i < ap_count; i++) {
            printf("SSID:%s,RSSI:%d ch%d\n", ap_info[i].ssid, ap_info[i].rssi, ap_info[i].primary);
        }
       ```
    7. 示例1:阻塞方式获得ap信息
       ```c
        #include <stdio.h>

        #include "esp_wifi.h"
        #include "freertos/FreeRTOS.h"
        #include "freertos/task.h"
        #include "nvs_flash.h"
        #include "string.h"

        void app_main(void)
        {
            nvs_flash_init();
            esp_netif_init();
            esp_netif_create_default_wifi_sta();

            wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
            ESP_ERROR_CHECK(esp_wifi_init(&cfg));

            esp_wifi_set_mode(WIFI_MODE_STA);
            esp_wifi_start();

            esp_wifi_scan_start(NULL, true);

            uint16_t total_number;
            esp_wifi_scan_get_ap_num(&total_number);
            printf("Total numbers:%d\n", total_number);

            uint16_t number = 10;  // 最多获取10个
            wifi_ap_record_t ap_records[number];
            memset(ap_records, 0, sizeof(ap_records));
            esp_wifi_scan_get_ap_records(&number, &ap_records);
            for (int i = 0; i < number; i++) {
                printf("AP%d\tSSID:%s,RSSI:%d,ch%d\n", i, ap_records[i].ssid, ap_records[i].rssi,
                    ap_records[i].primary);
            }
        }
       ```
    8. 事件处理（如果使用事件循环的话,则用下面步骤）
       1. [ESP32 事件机制](esp32_事件机制.md)
       2. 首先调用`esp_event_loop_create_default()` 创建默认事件循环。
       3. 之后使用`esp_event_handler_register()` 将事件处理程序注册到系统事件循环
          * esp_event_handler_register() 
            用于注册以下各项的处理程序：（1）特定事件，（2）某个事件基础的所有事件，或（3）系统事件循环已知的所有事件。
                特定事件：指定确切的event_base和event_id
                特定基准的所有事件：指定确切的event_base并使用ESP_EVENT_ANY_ID作为event_id
                循环已知的所有事件：将ESP_EVENT_ANY_BASE用作event_base，将ESP_EVENT_ANY_ID用作event_id
            可以将多个处理程序注册到事件.将单个处理程序注册到多个事件也是可以。但将同一处理程序多次注册到同一事件将导致以前的注册被覆盖。
            注意
                事件循环库不维护event_handler_arg的副本，因此用户应确保在调用处理程序时event_handler_arg仍指向有效位置
            返回
                ESP_OK：成功
                ESP_ERR_NO_MEM：无法为处理程序分配内存
                ESP_ERR_INVALID_ARG：事件库和事件ID的无效组合
                其他：失败
            参量
                event_base：要为其注册处理程序的事件的基本ID
                event_id：要为其注册处理程序的事件的ID
                event_handler：在调度事件时调用的处理函数
                event_handler_arg：除事件数据外，在调用时传递给处理程序的数据
          * 下面使用ESP_EVENT_ANY_ID将WIFI事件和IP事件全部注册到响应函数event_handler
       ```c
        include "esp_event.h"

        esp_event_loop_create_default();//创建一个默认的事件循环

        esp_event_handler_instance_t wifi_handler,ip_handler;
        esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, &wifi_handler);
        esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP,&event_handler, NULL, &ip_handler);
        ```
       4. 处理事件
       ```c
        static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
        {
            ESP_LOGI("","event_base:%s， event_id：%d\r\n",event_base, event_id);
            wifi_event_ap_staconnected_t *wifi_event_data;
            if (event_base == WIFI_EVENT){
                case WIFI_EVENT_STA_START://STA模式启动
                break;
            }
        }
       ```
    9. 示例2:非阻塞方式(事件方式)获得ap信息
    ```c
    #include <stdio.h>

    #include "esp_wifi.h"
    #include "freertos/FreeRTOS.h"
    #include "freertos/task.h"
    #include "nvs_flash.h"
    #include "string.h"
    #include "esp_log.h"

    void printAP()
    {
        uint16_t total_number;
        esp_wifi_scan_get_ap_num(&total_number);
        printf("Total numbers:%d\n", total_number);

        uint16_t number = 10;  // 最多获取10个
        wifi_ap_record_t ap_records[number];
        memset(ap_records, 0, sizeof(ap_records));
        esp_wifi_scan_get_ap_records(&number, &ap_records);
        for (int i = 0; i < number; i++) {
            printf("AP%d\tSSID:%s,RSSI:%d,ch%d\n", i, ap_records[i].ssid, ap_records[i].rssi,
                ap_records[i].primary);
        }
    }
    static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id,void* event_data)
    {
        ESP_LOGI("", "event_base:%s， event_id：%d\r\n", event_base, event_id);
        wifi_event_ap_staconnected_t* wifi_event_data;
        if (event_base == WIFI_EVENT) {
            switch (event_id) {
            case WIFI_EVENT_STA_START:  // STA模式启动
                break;
            case WIFI_EVENT_SCAN_DONE:
                printAP();
                break;
            }
        }
    }
    void app_main(void)
    {
        nvs_flash_init();
        esp_netif_init();
        esp_netif_create_default_wifi_sta();

        // 事件
        esp_event_loop_create_default();  // 创建一个默认的事件循环

        esp_event_handler_instance_t wifi_handler,ip_handler;
        esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, &wifi_handler);
        esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP,&event_handler, NULL, &ip_handler);

        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        ESP_ERROR_CHECK(esp_wifi_init(&cfg));

        esp_wifi_set_mode(WIFI_MODE_STA);
        esp_wifi_start();

        esp_wifi_scan_start(NULL, true);

        
    }
    ``` 