# 知识点
    这里要用到一个FreeRTOS的知识点，xTaskGetTickCount()函数，用于获取系统当前运行的时钟节拍数。
至于一个时钟节拍数是1ms，2ms，还是10ms，取决于configTICK_RATE_HZ，即CONFIG_FREERTOS_HZ。
CONFIG_FREERTOS_HZ在sdkconfig中定义，默认是100Hz。
则一个时钟节拍数是10ms。可以将其修改为1000Hz，则一个时钟节拍数是1ms，计时更加精确。
不过这样也会增加系统的开销，造成不必要的浪费

# 方式一:中断方式
   1. 设置按键上升沿或者下降沿中断
   2. 进入中断后
        1. 如果是低电平，则直接抛出“按键短按”，并记下当前的tick
        2. 如果是高电平，则判断当前的tick和之前的tick的差值是否大于阈值
            * 如果是，则抛出“按键长按”
   3. 该方式的好处：中断比较省CPU
   4. 该方式的劣处：只能在按键放开的时候，才能抛出“按键长按”，用户体验不是很好
   5. 最后还需要开一个线程，因为中断中不允许做日志打印等比较耗时的操作。我们实际应用中把日志去掉的话，完全在中断中直接做判断
   6. 示例代码
   ```c
        #include "user_key.h"
        #include <esp_log.h>
        #include <freertos/FreeRTOS.h>
        #include "freertos/task.h"
        #include "freertos/queue.h"
        #include "sys/time.h"

        #define BTN                 GPIO_NUM_0
        #define BTN_ACTIVE_LEVEL    0               //低电平有效

        static xQueueHandle gpio_evt_queue = NULL;

        static void gpio_isr_handler(void* arg)
        {
            uint32_t gpio_num = (uint32_t)arg;
            xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
        }

        static void gpio_task_example(void* arg)
        {
            static uint32_t tickCount;
            for (;;) {
                uint32_t io_num;
                if (xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
                    printf("GPIO[%d] intr, val: %d\n", io_num, gpio_get_level(io_num));
                    if(gpio_get_level(BTN) == BTN_ACTIVE_LEVEL){
                        printf("key short press\n");
                    }else{
                        if(xTaskGetTickCount() > tickCount + 500/portTICK_RATE_MS){
                            printf("key long press\n");
                        }
                    }
                    tickCount = xTaskGetTickCount();
                }
            }
        }

        void btn_init(void)
        {
            gpio_config_t cfg = {0};
            cfg.pin_bit_mask = (1ull << BTN);
            cfg.pull_down_en = GPIO_PULLDOWN_DISABLE;
            cfg.pull_up_en = GPIO_PULLUP_ENABLE;
            cfg.mode = GPIO_MODE_INPUT;
            cfg.intr_type = GPIO_INTR_ANYEDGE;
            gpio_config(&cfg);

            gpio_install_isr_service(ESP_INTR_FLAG_LEVEL1);
            gpio_isr_handler_add(BTN, gpio_isr_handler, (void*)BTN);

            gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
            xTaskCreate(gpio_task_example, "gpio_task_example", 2048, NULL, 10, NULL);

            while (1) {
                vTaskDelay(500 / portTICK_RATE_MS);
            }
        }
   ```
   ```运行结果
    GPIO[0] intr, val: 0
    key short press
    GPIO[0] intr, val: 0
    key short press
    GPIO[0] intr, val: 0
    key short press
    GPIO[0] intr, val: 0
    key short press
    GPIO[0] intr, val: 0
    key short press
    GPIO[0] intr, val: 1
    key long press
    GPIO[0] intr, val: 1
   ```
# 方式二:定时扫描
  1. 开一个线程或者定时器，不断的扫描GPIO的输入状态
  2. 该方式的好处：用户体验较好
  3. 该方式的劣处：比较浪费CPU资源
  4. 流程
     1. 把IO配置为输入,置keyPress(按键按下),keyLongPress(按键长按),tick(按键按下时间)
     2. 开启一个线程,隔一段时间就去读取一次IO口
     3. 读取IO,进入判断按键
        1. 读取IO,电平是按下,进入判断按键按下流程
           1. 按键消抖后判断短按,并置相应的标识位(keyPress = true;keyLongPress = false;tick)
           2. 如果已经有键按下,判断是否是长按.是则置标志位(keyLongPress = true)
        2. 没有按下,是否是按键释放?,并置标识位(keyPress = false;)
     4. 按键消抖
        * 那么为什么要消抖呢?这是有历史原因的 (使用触摸就不用消抖了,因为它也没办法抖😏),具体原因如下所示
        * 按键消抖通常的按键所用开关为机械弹性开关，当机械触点断开、闭合时，由于机械触点的弹性作用，一个按键开关在闭合时不会马上稳定地接通，在断开时也不会一下子断开。因而在闭合及断开的瞬间均伴随有一连串的抖动，为了不产生这种现象而作的措施就是按键消抖.(出自<百度百科>) 
        <br><img src="img/key.png">
  5. 示例
  ```c
    #include <esp_log.h>
    #include <freertos/FreeRTOS.h>
    #include "freertos/task.h"
    #include "freertos/queue.h"
    #include "sys/time.h"

    #define BTN                 GPIO_NUM_0
    #define BTN_ACTIVE_LEVEL    0               //低电平有效


    void readButton()
    {
        static uint32_t tick = 0;
        static bool keyPress = false;
        static bool keyLongPress = false;
        if(gpio_get_level(BTN) == BTN_ACTIVE_LEVEL){
            if(keyPress == false){
                vTaskDelay(100/portTICK_RATE_MS); //延时100ms消抖,判断是否识动作
                if(gpio_get_level(BTN) == BTN_ACTIVE_LEVEL){
                    printf("key down\n");
                    keyPress = true;
                    keyLongPress = false;
                    tick = xTaskGetTickCount();
                }  
            }else{
                if(keyLongPress == false && (uint32_t)(xTaskGetTickCount() - tick) > 500 / portTICK_RATE_MS){
                    printf("key long press\n");
                    keyLongPress = true;
                } 
            } 
        }else{
            if(keyPress){
                printf("key release\n");
            }
            keyPress = false; 
        }
    }

    static void gpio_task_example(void* arg)
    {
        for (;;) {
            vTaskDelay(100 / portTICK_RATE_MS);
            readButton();
        }
    }

    void btn_init(void)
    {
        gpio_config_t cfg = {0};
        cfg.pin_bit_mask = (1ull << BTN);
        cfg.pull_down_en = GPIO_PULLDOWN_DISABLE;
        cfg.pull_up_en = GPIO_PULLUP_ENABLE;
        cfg.mode = GPIO_MODE_INPUT;
        cfg.intr_type = GPIO_INTR_ANYEDGE;
        gpio_config(&cfg);

        xTaskCreate(gpio_task_example, "gpio_task_example", 2048, NULL, 10, NULL);

        while (1) {
            vTaskDelay(500 / portTICK_RATE_MS);
        }
    }
  ```
  ```输出
    I (419) gpio: GPIO[0]| InputEn: 1| OutputEn: 0| OpenDrain: 0| Pullup: 1| Pulldown: 0| Intr:3
    key down
    key release
    key down
    key long press
    key release 
  ```  
# 练习:
 * 按键短按，LED为1秒间隔闪烁
 * 按键长按，LED为5秒间隔闪烁