# 简介
要示例实现蓝牙BLE GATT 服务端。此示例实现两个Application Profiles，用事件处理配置步骤，例如定义广告参数、更新连接参数以及创建services 和 characteristics。此外，此示例处理读取和写入事件，包括写入长特征请求，该请求将传入的数据划分为块，以便数据能够适应属性协议（ATT）消息。

# 主要流程
1. 头文件
    ```c
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include "freertos/FreeRTOS.h"
    #include "freertos/task.h"
    #include "freertos/event_groups.h"
    #include "esp_system.h"
    #include "esp_log.h"
    #include "nvs_flash.h"
    #include "esp_bt.h"
    #include "esp_gap_ble_api.h"
    #include "esp_gatts_api.h"
    #include "esp_bt_defs.h"
    #include "esp_bt_main.h"
    #include "esp_gatt_common_api.h"
    #include "sdkconfig.h"
    ```
    主要的头文件:
    1. esp_bt.h: 主要实现 BT controller 及 VHCI 接口.
    2. esp_bt_main.h: 主要实现BT 协议栈的初始化和启用
    3. esp_gap_ble_api.h: 主要是GAP配置，如广播和连接参数等
    4. esp_gatts_api.h: 主要是GATT配置,如创建services和characteristics.

2. 程序入口
    ```c
     void app_main()
    {
        esp_err_t ret;

        // 初始化 NVS.用于存储数据
        ret = nvs_flash_init();
        if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
            ESP_ERROR_CHECK(nvs_flash_erase());
            ret = nvs_flash_init();
        }
        ESP_ERROR_CHECK(ret);

        //把esp_bt_controller_config_t结构体设为默认参数
        esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();

        //通过esp_bt_controller_init初始化bt控制器
        ret = esp_bt_controller_init(&bt_cfg);
        if (ret) {
            ESP_LOGE(GATTS_TAG, "%s initialize controller failed\n", __func__);
            return;
        }
        //bt控制器置为ble模式,总共可以有4种模式
        //1.ESP_BT_MODE_IDLE: Bluetooth not running
        //2.ESP_BT_MODE_BLE: BLE mode
        //3.ESP_BT_MODE_CLASSIC_BT: BT Classic mode
        //4.ESP_BT_MODE_BTDM: Dual mode (BLE + BT Classic)
        ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
        if (ret) {
            ESP_LOGE(GATTS_TAG, "%s enable controller failed\n", __func__);
            return;
        }
        //初始化bt协议栈
        ret = esp_bluedroid_init();
        if (ret) {
            ESP_LOGE(GATTS_TAG, "%s init bluetooth failed\n", __func__);
            return;
        }
        ret = esp_bluedroid_enable();
        //这句执行完,bt协议栈就已经启动并且开始运行
        if (ret) {
            ESP_LOGE(GATTS_TAG, "%s enable bluetooth failed\n", __func__);
            return;
        }
        //到这里,虽然bt协议栈已经运行,但是蓝牙的具体功能还没有定义.
        //蓝牙功能的定义是通过事件的形式来实现,比如要设置或者读取一个参数
        //事件主要由GAP和GATT组成.我们的程序需要注册一个回调函数来处理这些事件.
        ret = esp_ble_gatts_register_callback(gatts_event_handler);
        if (ret){
            ESP_LOGE(GATTS_TAG, "gatts register error, error code = %x", ret);
            return;
        }
        ret = esp_ble_gap_register_callback(gap_event_handler);
        if (ret){
            ESP_LOGE(GATTS_TAG, "gap register error, error code = %x", ret);
            return;
        }
        ret = esp_ble_gatts_app_register(PROFILE_A_APP_ID);
        if (ret){
            ESP_LOGE(GATTS_TAG, "gatts app register error, error code = %x", ret);
            return;
        }
        ret = esp_ble_gatts_app_register(PROFILE_B_APP_ID);
        if (ret){
            ESP_LOGE(GATTS_TAG, "gatts app register error, error code = %x", ret);
            return;
        }
        esp_err_t local_mtu_ret = esp_ble_gatt_set_local_mtu(512);
        if (local_mtu_ret){
            ESP_LOGE(GATTS_TAG, "set local  MTU failed, error code = %x", local_mtu_ret);
        }
        return;
    }
    ```