# Station模式连接到AP
## 步骤及API简介
1. 初始化nvs_flash:初始化nvs系统,因为wifi需要用到nfs,如果不初始化nvs,调用esp_wifi_init会出错
    `#include "nvs_flash.h"`
    `nvs_flash_init();`
2. 创建esp_netif: Initialize the underlying TCP/IP stack
    `#include "esp_wifi.h"`
    `esp_netif_init();`
3. 创建一个默认的事件循环:必须先创建,否则会收不到`IP_EVENT_STA_GOT_IP`
   `esp_event_loop_create_default();`
4. 创建默认的wifi sta: 必须放在`esp_event_loop_create_default()`后面
    `esp_netif_create_default_wifi_sta();`
5. 初始化WiFi,并把wifi置为station模式
       ```c
        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        ESP_ERROR_CHECK(esp_wifi_init(&cfg));
        esp_wifi_set_mode(WIFI_MODE_STA);
       ```
6. 注册事件并处理事件:
    ```c
        esp_event_handler_instance_t wifi_handler,ip_handler;
        esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, &wifi_handler);
        esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP,&event_handler, NULL, &ip_handler);
    ``` 
    ```c 新建处理事件函数
        static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id,void* event_data)
        {
            ESP_LOGI("", "event_base:%s， event_id：%d\r\n", event_base, event_id);
            if (event_base == WIFI_EVENT) {
                ...
            }
            if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP){
                ...
            }
        }
    ```
7. 设置WIFI参数,并启动WIFI.主要是SSID,密码
   ```c
    wifi_config_t conf={.sta.ssid="test",
                        .sta.password="123012345",
                        .sta.threshold.authmode= WIFI_AUTH_WPA2_PSK,};
    esp_wifi_set_config(WIFI_IF_STA, &conf);
    esp_wifi_start();
   ```
8. 启动完wifi后,wifi会产生`WIFI_EVENT_STA_START`事件,在该事件中调用`esp_wifi_connect();`连接路由器
   ```c
   if (event_base == WIFI_EVENT) {
        switch (event_id) {
        case WIFI_EVENT_STA_START:  // STA模式启动
            esp_wifi_connect();
            break;
   ```
9.  当连接成功后,会产生`IP_EVENT_STA_GOT_IP`事件,在该事件中可以获取到IP
    IP放在event_data中,要转成ip_event_got_ip_t类型
    ```c
    if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP){
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI("", "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
    }
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

static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id,void* event_data)
{
    ESP_LOGI("", "event_base:%s， event_id：%d\r\n", event_base, event_id);
    if (event_base == WIFI_EVENT) {
        switch (event_id) {
        case WIFI_EVENT_STA_START:  // STA模式启动
            esp_wifi_connect();
            break;
        case WIFI_EVENT_STA_CONNECTED:
            printf("WIFI_EVENT_STA_CONNECTED\n");
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
    
    esp_netif_create_default_wifi_sta();//必须放在esp_event_loop_create_default()后面,否则收不到IP_EVENT_STA_GOT_IP

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    esp_wifi_set_mode(WIFI_MODE_STA);

    esp_event_handler_instance_t wifi_handler,ip_handler;
    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, &wifi_handler);
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP,&event_handler, NULL, &ip_handler);

    

    wifi_config_t conf={.sta.ssid="test",
                        .sta.password="123012345",
                        .sta.threshold.authmode= WIFI_AUTH_WPA2_PSK,};
    esp_wifi_set_config(WIFI_IF_STA, &conf);
    esp_wifi_start();
    
}
```