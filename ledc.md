# LEDC
## 基于LEDC(LED Control)实现PWM调光(占空比切换/渐变/频率切换)
1. LED PWM控制器基础介绍
    1. 基本介绍(https://docs.espressif.com/projects/esp-idf/zh_CN/v4.3.2/esp32c3/api-reference/peripherals/ledc.html)
        LED PWM 控制器主要用于控制 LED，也可产生 PWM 信号用于其他设备的控制。该控制器有 8 路高速通道和 8 路低速通道，可以产生独立的波形来驱动 RGB LED 设备等。

        LED PWM 控制器的高速通道和低速通道均支持硬件渐变功能，可在无需 CPU 干预的情况下自动改变 PWM 信号的占空比，也可由软件改变 PWM 信号的占空比，实现亮度和颜色渐变。此外，低速通道在 Sleep 模式下仍可运行。
    2. PWM的基础知识
        <br><img src="img\pwm.png"><br>
        关于PWM的渐变，是通过改变Lpointn的值，开启了占空比渐变，Lpointn的值会在计数器溢出固定次数后（可配置）递增或递减（可配置），渐变值也可以配置：
        <img src="img\pwm2.png"><br>
2. 使用步骤
    1. 要让指定 LED PWM 控制器 高速模式或低速模式 通道运行，需进行如下配置：
        1. 配置定时器,指定PWM信号的频率和占空比分辨率。
            使用ledc_timer_config函数,主要是ledc_timer_config_t结构体
            ```c
            typedef struct {
                ledc_mode_t speed_mode;                /*!< LEDC speed speed_mode, high-speed mode or low-speed mode */
                ledc_timer_bit_t duty_resolution;      /*!< LEDC channel duty resolution */
                ledc_timer_t  timer_num;               /*!< The timer source of channel (0 - 3) */
                uint32_t freq_hz;                      /*!< LEDC timer frequency (Hz) */
                ledc_clk_cfg_t clk_cfg;                /*!< Configure LEDC source clock.
                                                            For low speed channels and high speed channels, you can specify the source clock using LEDC_USE_REF_TICK, LEDC_USE_APB_CLK or LEDC_AUTO_CLK.
                                                            For low speed channels, you can also specify the source clock using LEDC_USE_RTC8M_CLK, in this case, all low speed channel's source clock must be RTC8M_CLK*/
            } ledc_timer_config_t;
            ```
            注意：PWM 频率越高，占空比分辨率越低，反之则越高。
            ```c
            ledc_timer_config_t ledc_timer;
            ledc_timer.duty_resolution = LEDC_TIMER_13_BIT;
            ledc_timer.freq_hz = 5000; // PWM信号频率,5kHz,占空比分辨率最大为13位
            ledc_timer.speed_mode = LEDC_LOW_SPEED_MODE;    // 定时器模式
            ledc_timer.timer_num = LEDC_HS_TIMER;            // 定时器序号
            ledc_timer.clk_cfg = LEDC_AUTO_CLK;

            ledc_timer_config(&ledc_timer);
            ```
        2. 配置通道 绑定定时器和输出 PWM 信号的 GPIO。
            使用ledc_channel_config，参数为ledc_channel_config_t结构体
            ```c
            typedef struct {
                int gpio_num;                   /*!< the LEDC output gpio_num, if you want to use gpio16, gpio_num = 16 */
                ledc_mode_t speed_mode;         /*!< LEDC speed speed_mode, high-speed mode or low-speed mode */
                ledc_channel_t channel;         /*!< LEDC channel (0 - 7) */
                ledc_intr_type_t intr_type;     /*!< configure interrupt, Fade interrupt enable  or Fade interrupt disable */
                ledc_timer_t timer_sel;         /*!< Select the timer source of channel (0 - 3) */
                uint32_t duty;                  /*!< LEDC channel duty, range is [0, (2**duty_resolution)] */
                int hpoint;                     /*!< 一般设为0, the max value is 0xfffff */
            } ledc_channel_config_t;

            ```
            ```c
                ledc_channel_config_t ledc_channel[2] = {
                    {
                        .channel    = 1,
                        .duty       = 0,
                        .gpio_num   = 3,
                        .speed_mode = LEDC_LOW_SPEED_MODE,
                        .hpoint     = 0,
                        .timer_sel  = LEDC_TIMER_0
                    },
                    {
                        .channel    = 2,
                        .duty       = 0,
                        .gpio_num   = 4,
                        .speed_mode = LEDC_LOW_SPEED_MODE,
                        .hpoint     = 0,
                        .timer_sel  = LEDC_TIMER_0
                    }
                };
                ledc_channel_config(&ledc_channel[0]);
                ledc_channel_config(&ledc_channel[1]);
            ```
            注意：ESP32-C3只有(0-5)6个通道
        3. 改变 PWM 信号 输出 PWM 信号来驱动 LED。可通过软件控制或使用硬件渐变功能来改变 LED 的亮度。
            ledc_set_duty(ledc_mode_t speed_mode, ledc_channel_t channel, uint32_t duty);  设置占空比
            ledc_update_duty(ledc_mode_t speed_mode, ledc_channel_t channel); 配置生效
            ledc_get_duty();    查看当前占空比
            硬件渐变：
                ledc_fade_func_install(0);使能硬件渐变
                ledc_set_fade_with_time()
                ledc_set_fade_with_step()
                ledc_set_fade();
                ledc_fade_start(); 开启渐变
    2. 步骤总结：
            1. #include "driver/ledc.h"
            2. 用ledc_timer_config配置定时器(duty_resolution/freq_hz/timer_num/clk_cfg/speed_mode)
            注意: duty_resolution和freq_hz具有相关性,典型:频率为5kHz,占空比分辨率最大可为13位
            3. 用ledc_channel_config配置通道(channel/duty/gpio_num/speed_mode/timer_sel)
            4. ledc_set_duty
            5. ledc_update_duty
3. PWM基础测试
    ```c 测试代码
        void app_main(void)
        {
            ledc_timer_config_t ledc_timer;
            ledc_timer.duty_resolution = LEDC_TIMER_13_BIT;
            ledc_timer.freq_hz = 5000; // PWM信号频率,5kHz,占空比分辨率最大为13位
            ledc_timer.speed_mode = LEDC_LOW_SPEED_MODE;    // 定时器模式
            ledc_timer.timer_num = LEDC_HS_TIMER;            // 定时器序号
            ledc_timer.clk_cfg = LEDC_AUTO_CLK;
            ledc_timer_config(&ledc_timer);

            ledc_channel_config_t ledc_channel[1] = {
                {
                    .channel    = 1,
                    .duty       = 0,
                    .gpio_num   = 3,
                    .speed_mode = LEDC_LOW_SPEED_MODE,
                    .hpoint     = 0,
                    .timer_sel  = LEDC_TIMER_0
                },
            };
            ledc_channel_config(&ledc_channel[0]);
            ledc_set_duty(LEDC_LOW_SPEED_MODE,1,4000);
            ledc_update_duty(LEDC_LOW_SPEED_MODE,1);

            while(1) vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
    ```
    * 效果
    上面的例子是一个通道输出，一个LED灯，初始化的时候占空比为0，通过ledc_set_duty函数把占空比设4000.    
3. 作业1：用LEDC，驱动一个LED灯，初始化的时候占空比为0，通过ledc_set_duty函数把占空比为10%，让LED微微亮
4. 练习2：完成的一个呼吸灯程序(LED 灯在5s内由亮变暗,再由暗变亮,一直重复)
5. 练习3：在作业1的基础上：试着用二个按键控制灯渐变的速度。按键1，加快渐变速度，按键2，减慢渐变的速度
6. 练习4: RGB三色灯驱动：完成一个函数,rgb(u8 r,u8 g,u8 b),实现输入RGB值,可以得到对应的颜色
    ```c 参考代码
        ledc_timer_config_t ledc_timer;
        ledc_timer.duty_resolution = LEDC_TIMER_13_BIT;
        ledc_timer.freq_hz = 5000; // PWM信号频率,5kHz,占空比分辨率最大为13位
        ledc_timer.speed_mode = LEDC_LOW_SPEED_MODE;    // 定时器模式
        ledc_timer.timer_num = LEDC_HS_TIMER;            // 定时器序号
        ledc_timer.clk_cfg = LEDC_AUTO_CLK;
        ledc_timer_config(&ledc_timer);

        ledc_channel_config_t ledc_channel[3] = {
            {
                .channel    = 1,
                .duty       = 0,
                .gpio_num   = 3,
                .speed_mode = LEDC_LOW_SPEED_MODE,
                .hpoint     = 0,
                .timer_sel  = LEDC_TIMER_0
            },
            {
                .channel    = 2,
                .duty       = 0,
                .gpio_num   = 4,
                .speed_mode = LEDC_LOW_SPEED_MODE,
                .hpoint     = 0,
                .timer_sel  = LEDC_TIMER_0
            },
            {
                .channel    = 3,
                .duty       = 0,
                .gpio_num   = 5,
                .speed_mode = LEDC_LOW_SPEED_MODE,
                .hpoint     = 0,
                .timer_sel  = LEDC_TIMER_0
            },
        };
        ledc_channel_config(&ledc_channel[0]);
        ledc_channel_config(&ledc_channel[1]);
        ledc_channel_config(&ledc_channel[2]);
        
        // ledc_set_duty(LEDC_LOW_SPEED_MODE, 1, 0);
        // ledc_update_duty(LEDC_LOW_SPEED_MODE, 1);

        uint8_t r=255,g=255,b=10;
        {
            ledc_set_duty(LEDC_LOW_SPEED_MODE, 1, 0x1FFF * r / 255);
            ledc_set_duty(LEDC_LOW_SPEED_MODE, 2, 0x1FFF * g / 255);
            ledc_set_duty(LEDC_LOW_SPEED_MODE, 3, 0x1FFF * b / 255);
            ledc_update_duty(LEDC_LOW_SPEED_MODE, 1);
            ledc_update_duty(LEDC_LOW_SPEED_MODE, 2);
            ledc_update_duty(LEDC_LOW_SPEED_MODE, 3);
        }

        while(1) vTaskDelay(1000 / portTICK_PERIOD_MS);
    ```
7. PWM硬件渐变
    * 上面的例子是简单的PWM控制,如果需要实现渐变的效果,则需要使用以下几个函数
    ```c
        ledc_fade_func_install(0);    // 注册LEDC服务，在调用前使用，参数是作为是否允许中断
        ledc_set_fade_with_time(speed_mode, channel, target_duty, max_fade_time_ms);
        ledc_fade_start(speed_mode, channel, ledc_fade_mode_t fade_mode);
    ``` 
8. PWM硬件渐变示例
9. ```c 下面实例GPIO3的pwm由0渐变到4000,用时3秒
        ledc_timer_config_t ledc_timer;
        ledc_timer.duty_resolution = LEDC_TIMER_13_BIT;
        ledc_timer.freq_hz = 5000; // PWM信号频率,5kHz,占空比分辨率最大为13位
        ledc_timer.speed_mode = LEDC_LOW_SPEED_MODE;    // 定时器模式
        ledc_timer.timer_num = LEDC_HS_TIMER;            // 定时器序号
        ledc_timer.clk_cfg = LEDC_AUTO_CLK;
        ledc_timer_config(&ledc_timer);

        ledc_channel_config_t ledc_channel[1] = {
            {
                .channel    = 1,
                .duty       = 0,
                .gpio_num   = 3,
                .speed_mode = LEDC_LOW_SPEED_MODE,
                .hpoint     = 0,
                .timer_sel  = LEDC_TIMER_0
            },
        };
        ledc_channel_config(&ledc_channel[0]);
        // ledc_set_duty(LEDC_LOW_SPEED_MODE,1,4000);
        // ledc_update_duty(LEDC_LOW_SPEED_MODE,1);

        //渐变控制器
        ledc_fade_func_install(0);    // 注册LEDC服务，在调用前使用，参数是作为是否允许中断
        ledc_set_fade_with_time(LEDC_LOW_SPEED_MODE, 1, 4000, 3000); //3000,3秒
        ledc_fade_start(LEDC_LOW_SPEED_MODE, 1, LEDC_FADE_NO_WAIT);
    ``` 
10. 练习4(PWM硬件渐变):在RGB三色灯驱动练习的基础上,添加硬件渐变.函数修改成rgb(u8 r,u8 g,u8 b,u8 speed),speed为渐变秒数.
11. 使用中断:
    - 要处理中断,可调用函数 ledc_isr_register()
    - ledc_channel_config_t中的参数 ledc_intr_type_t,可以不用设置
    - 中断的产生条件:变化量超过1023就会产生一次中断或者渐变完成时也会产生中断.
        /*Because the max duty change times for once fade operation is 1023, if the duty change times is more than 1023, we will separate the fade operation as several times, so maybe several times interrupt will be generated. If current duty is equal to target duty, that means it is the fade end interrupt for whole fade operation.In ESP-IDF, please see the LEDC interrupt handle function ledc_fade_isr*/
    - 使用步骤:
        - ledc_fade_func_install(ESP_INTR_FLAG_IRAM | ESP_INTR_FLAG_SHARED);
        - ledc_isr_register(ledc_isr, (void *)100, ESP_INTR_FLAG_IRAM | ESP_INTR_FLAG_SHARED, NULL);
        - 实现中断函数
        - ```c
            void IRAM_ATTR ledc_isr(void *arg)
            {
                int param = (int)arg;
                xQueueSendFromISR(ledcQueue, &param, 0);
            }
            ```
    - 完整示例
    - ```c
        #include "driver/ledc.h"
        QueueHandle_t ledcQueue;
        void IRAM_ATTR ledc_isr(void *arg)
        {
            static int isr_count;
            isr_count++;
            int param = (int)arg;
            xQueueSendFromISR(ledcQueue, &isr_count, 0);
        }

        void app_main(void)
        {
            ledc_timer_config_t ledc_timer;
            ledc_timer.duty_resolution = LEDC_TIMER_13_BIT;
            ledc_timer.freq_hz = 5000;  // PWM信号频率,5kHz,占空比分辨率最大为13位
            ledc_timer.speed_mode = LEDC_LOW_SPEED_MODE;  // 定时器模式
            ledc_timer.timer_num = 0;                     // 定时器序号
            ledc_timer.clk_cfg = LEDC_AUTO_CLK;
            ledc_timer_config(&ledc_timer);

            ledc_channel_config_t ledc_channel[3] = {
                {.channel = 1,
                .duty = 0,
                .gpio_num = 16,
                .speed_mode = LEDC_LOW_SPEED_MODE,
                .hpoint = 0,
                .timer_sel = LEDC_TIMER_0,
                },
                {.channel = 2,
                .duty = 500,
                .gpio_num = 17,
                .speed_mode = LEDC_LOW_SPEED_MODE,
                .hpoint = 0,
                .timer_sel = LEDC_TIMER_0,
                },
                {.channel = 3,
                .duty = 1000,
                .gpio_num = 5,
                .speed_mode = LEDC_LOW_SPEED_MODE,
                .hpoint = 0,
                .timer_sel = LEDC_TIMER_0,
                .intr_type = LEDC_INTR_DISABLE},
                };
            for (int i = 0; i < 3; i++) ledc_channel_config(&ledc_channel[i]);

            // ledc_set_duty(LEDC_LOW_SPEED_MODE,1,4000);
            // ledc_update_duty(LEDC_LOW_SPEED_MODE,1);

            ledcQueue = xQueueCreate(10, sizeof(int));

            // 渐变控制器
            ledc_fade_func_install(ESP_INTR_FLAG_IRAM | ESP_INTR_FLAG_SHARED);
            // ledc_fade_func_install(0);  // 注册LEDC服务，在调用前使用，参数是作为是否允许中断

            ledc_isr_register(ledc_isr, (void *)100, ESP_INTR_FLAG_IRAM | ESP_INTR_FLAG_SHARED, NULL);
            /*Because the max duty change times for once fade operation is 1023, if the duty change times is more than 1023, we will separate the fade operation as several times, so maybe several times interrupt will be generated. If current duty is equal to target duty, that means it is the fade end interrupt for whole fade operation.
        In ESP-IDF, please see the LEDC interrupt handle function ledc_fade_isr*/


            ESP_LOGI("ledc", "fade_start");
            ledc_set_fade_with_time(LEDC_LOW_SPEED_MODE, 1, 4000, 3000);  // 4000,3秒
            ledc_fade_start(LEDC_LOW_SPEED_MODE, 1, LEDC_FADE_NO_WAIT);

            while (1) {
                int val;
                xQueueReceive(ledcQueue, &val, portMAX_DELAY);
                ESP_LOGI("ledc", "%d", val);
                if (val == 4) {
                    ledc_set_fade_with_time(LEDC_LOW_SPEED_MODE, 2, 4000, 5000);  // 
                    ledc_fade_start(LEDC_LOW_SPEED_MODE, 2, LEDC_FADE_NO_WAIT);
                    ESP_LOGI("ledc", "ledc_fade_start 2");
                }
            }
        }
        ```
12. 练习5:呼吸流水灯的实现,用ledc驱动8个led灯,开始是全灭,第1个灯3秒内渐变到最亮,完成后,第2个灯4秒内渐变到最亮...