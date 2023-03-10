1. RMT
    1.  什么是RMT(https://docs.espressif.com/projects/esp-idf/zh_CN/latest/esp32/api-reference/peripherals/rmt.html)
    2.  示例1: 带载波
        <br><img src="img/MorseCode.png">
        <br><img src="img/MorseCode2.png">
        ```c Morse Code
            #include "sdkconfig.h"
            #include "freertos/FreeRTOS.h"
            #include "freertos/task.h"
            #include "esp_log.h"
            #include "driver/rmt.h"

            static const char *TAG = "example";

            #define CONFIG_EXAMPLE_RMT_TX_GPIO  18
            #define RMT_TX_CHANNEL RMT_CHANNEL_0

            static const rmt_item32_t morse_esp[] = {
                // E : dot
                {{{ 32767, 1, 32767, 0 }}}, // dot 32ms on 32ms off
                {{{ 32767, 0, 32767, 0 }}}, // SPACE
                // S : dot, dot, dot
                {{{ 32767, 1, 32767, 0 }}}, // dot
                {{{ 32767, 1, 32767, 0 }}}, // dot
                {{{ 32767, 1, 32767, 0 }}}, // dot
                {{{ 32767, 0, 32767, 0 }}}, // SPACE
                // P : dot, dash, dash, dot
                {{{ 32767, 1, 32767, 0 }}}, // dot
                {{{ 32767, 1, 32767, 1 }}},
                {{{ 32767, 1, 32767, 0 }}}, // dash
                {{{ 32767, 1, 32767, 1 }}},
                {{{ 32767, 1, 32767, 0 }}}, // dash
                {{{ 32767, 1, 32767, 0 }}}, // dot
                // RMT end marker
                {{{ 0, 1, 0, 0 }}}
            };

            /*
            * Initialize the RMT Tx channel
            */
            static void rmt_tx_init(void)
            {
                rmt_config_t config = RMT_DEFAULT_CONFIG_TX(CONFIG_EXAMPLE_RMT_TX_GPIO, RMT_TX_CHANNEL);
                // enable the carrier to be able to hear the Morse sound
                // if the RMT_TX_GPIO is connected to a speaker
                config.tx_config.carrier_en = true;
                config.tx_config.carrier_duty_percent = 50;
                // with current implementation of the RMT API
                config.tx_config.carrier_freq_hz = 1000;//611;
                // set the maximum clock divider to be able to output
                // RMT pulses in range of about one hundred milliseconds
                config.clk_div = 80;//80->1M ;

                ESP_ERROR_CHECK(rmt_config(&config));
                ESP_ERROR_CHECK(rmt_driver_install(config.channel, 0, 0));
            }

            void app_main(void)
            {
                ESP_LOGI(TAG, "Configuring transmitter");
                rmt_tx_init();

                while (1) {
                    ESP_ERROR_CHECK(rmt_write_items(RMT_TX_CHANNEL, morse_esp, sizeof(morse_esp) / sizeof(morse_esp[0]), true));
                    ESP_LOGI(TAG, "Transmission complete");
                    vTaskDelay(1000 / portTICK_PERIOD_MS);
                }
            }
        ```  
    3.  示例2: 不带载波
        <br><img src="img/rmt_wave.png">
        ```c
            #include "driver/rmt.h"
            #include "esp_log.h"
            #include "freertos/FreeRTOS.h"
            #include "freertos/task.h"

            #define CONFIG_EXAMPLE_RMT_TX_GPIO 18
            #define RMT_TX_CHANNEL RMT_CHANNEL_0

            void app_main(void)
            {
                static const rmt_item32_t wave[] = {
                    {{{100, 1, 200, 0}}},  // 2.5us1 5us0
                    {{{100, 1, 300, 0}}},
                    {{{100, 1, 400, 0}}},
                    {{{0, 1, 0, 0}}}  // // RMT end marker
                };
                rmt_config_t config;
                config.rmt_mode = RMT_MODE_TX;		//发送
                config.channel = RMT_TX_CHANNEL;	//通道
                config.gpio_num = CONFIG_EXAMPLE_RMT_TX_GPIO;			//管脚
                config.mem_block_num = 3;				//3个脉冲
                config.tx_config.loop_en = false;
                config.tx_config.carrier_en = false;
                config.tx_config.idle_output_en = true;
                config.tx_config.idle_level = 0;
                // set counter clock to 40MHz
                config.clk_div = 2;

                ESP_ERROR_CHECK(rmt_config(&config));
                ESP_ERROR_CHECK(rmt_driver_install(RMT_TX_CHANNEL, 0, 0));
                while (1) {
                    vTaskDelay(1000 / portTICK_PERIOD_MS);
                    rmt_write_items(RMT_TX_CHANNEL, wave, sizeof(wave) / sizeof(wave[0]), true);
                    printf("Transmission complete\n");
                }
            }
        ```