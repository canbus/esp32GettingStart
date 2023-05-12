# 硬件定时器
注意:本API将在 ESP-IDF-v5.0 之后被弃用，届时你可能仍然可以使用本Timer API，但是建议迁移至新的 GPTimer API
ESP32 硬件定时器涉及的内容较多，理解起来可能会有点困难
## 概述
https://docs.espressif.com/projects/esp-idf/zh_CN/v4.3.2/esp32/api-reference/peripherals/timer.html
## 配置定时器
ESP32内置了两个定时器组 Timer Group，每个定时器组都有两个64位定时器Timer。支持向上、向下两个方向计数。支持设置警报阈值
### 配置和操作定时器的常规步骤
1. 定时器初始化 - 启动定时器前应设置的参数，以及每个设置提供的具体功能。
    两个 ESP32 定时器组中，每组都有两个定时器，两组共有四个定时器供使用。
    ESP32 定时器组的类型为 timer_group_t，每组中的个体定时器类型为 timer_idx_t。
    首先调用 timer_init() 函数，并将 timer_config_t 结构体传递给此函数，用于定义定时器的工作方式，实现定时器初始化。
    特别注意以下定时器参数可设置为：
        时钟源: 选择时钟源，它同时钟分频器一起决定了定时器的分辨率。默认的时钟源是 APB_CLK_FREQ(一般80 MHz,宏定义:TIMER_BASE_CLK)。
        分频器: 设置定时器中计数器计数的速度，divider 的设置将用作输入时钟源的除数。
        模式: 设置计数器是递增还是递减。可通过从 timer_count_dir_t 中选取一个值，后使用 counter_dir 来选择模式。
        计数器使能: 如果计数器已使能，则在调用 timer_init() 后计数器将立即开始递增/递减。您可通过从 timer_start_t 中选取一个值，后使用 counter_en 改变此行为。
        报警使能: 可使用 alarm_en 设置。
        自动重载: 设置计数器是否应该在定时器警报上使用 auto_reload 自动重载首个计数值，还是继续递增或递减。
        要获取定时器设置的当前值，请使用函数 timer_get_config()。
    定时器的频率计算: f0 = Fclk / K (其中f0​ 为定时器的计数频率(HZ),Fclk为定时器时钟频率,k为分频比
    定时值的计算: t = N / f0 (N为定时器计数值)
    ```c
    struct timer_config_t{
         timer_alarm_t 			alarm_en;//定时器中断开(1)关(0)
         timer_start_t 			counter_en;//定时器运行开(1)关(0)
         timer_intr_mode_t		intr_type;//中断类型（一般不需要改动）
         timer_count_dir_t		counter_dir;//向上计数(1)/向下计数(0)
         timer_autoreload_t		auto_reload;//是(1)否(0)自动重装载
         uint32_t 				divider;//分频系数
    }
    ```
    以下代码把定时器时钟为设为1M,向上计数
    ```c
        timer_config_t timerCfg;
        timerCfg.alarm_en = TIMER_ALARM_EN;
        timerCfg.auto_reload = TIMER_AUTORELOAD_EN;
        timerCfg.counter_dir = TIMER_COUNT_UP;
        timerCfg.divider = 80; //80M/80 = 1M
        timer_init(0, 0, &timerCfg);
    ```
2. 定时器控制 - 如何读取定时器的值，如何暂停/启动定时器以及如何改变定时器的操作方式。
   定时器使能后便开始计数。要使能定时器，可首先设置 counter_en 为 true，然后调用函数 timer_init()，或者直接调用函数 timer_start()。
   可通过调用函数 timer_set_counter_value() 来指定定时器的首个计数值。
   要检查定时器的当前值，调用函数 timer_get_counter_value() 或 timer_get_counter_time_sec()。
   可通过调用函数 timer_pause() 随时暂停定时器。要再次启动它，调用函数 timer_start()。
   要重新配置定时器，可调用函数 timer_init()
   除此之外，还可通过通过以下一组函数更改个别设置来重新配置定时器
    timer_set_divider() 更改计数频率
    timer_set_counter_mode() 设置计数器应递增还是递减
    timer_set_auto_reload() 设置是否应在定时器警报上重载首个计数值
  ```c
  timer_set_counter_value(0, 0, 0); //0组0号定时器的初始化值设为0,reload时也用这个值
  ``` 
3. 警报 - 如何设置和使用警报。
   要设置警报，先调用函数 timer_set_alarm_value()，然后使用 timer_set_alarm() 使能警报。
   当调用函数 timer_init() 时，也可以在定时器初始化阶段使能警报。
   警报已使能且定时器达到警报值后，根据配置，可能会出现以下两种行为：
    如果已配置中断，此时将触发中断。
    如 auto_reload 已使能，定时器的计数器将重新加载，从先前配置好的值开始再次计数。应使用函数 timer_set_counter_value() 预先设置该值。
  ```c //当定时器频率是1M时,则计数1M次时,就是1秒
  timer_set_alarm_value(0,0, 1000 * 1000 * 1); //1秒
  ```
4. 处理中断事务- 如何使用中断提供的回调函数。
   调用 timer_isr_callback_add() 函数可以给某个定时器注册一个中断回调函数，该函数会在中断上下文中被执行
   用户不能在回调函数中调用任何会阻塞 CPU 的 API。 
   在中断函数中用户无需检测和处理中断的状态位，这些操作会由默认的中断处理程序替我们完成。
   timer_isr_callback_add(timer_group_t group_num,  //Timer group number
                          timer_idx_t timer_num,    //Timer index of timer group
                          timer_isr_t isr_handler,  // Interrupt handler function, it is a callback function.
                          void *args,               //Parameter for handler function
                          int intr_alloc_flags);    //Flags used to allocate the interrupt. One or multiple (ORred) 
                                                    //ESP_INTR_FLAG_* values. See esp_intr_alloc.h for more info
                        //If the intr_alloc_flags value ESP_INTR_FLAG_IRAM is set, the handler function must be declared with IRAM_ATTR attribute and can only call functions in IRAM or ROM

  ```c
    IRAM_ATTR bool timerIsr(void *arg) 
    {
        gpio_set_level(16, 0);
        gpio_set_level(16, 1);
        return true;
    }
    timer_isr_callback_add(0, 0, timerIsr, NULL, ESP_INTR_FLAG_IRAM);
  ```
5. 完整示例
```c
#include "driver/gpio.h"
#include "driver/timer.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

uint8_t gpioLevel = 1;

IRAM_ATTR bool timerIsr(void *arg)
{
    gpio_set_level(16, gpioLevel);
    gpioLevel = gpioLevel == 0 ? 1 : 0;
    return true;
}
void initTimer()
{
    gpio_reset_pin(16);
    gpio_set_direction(16, GPIO_MODE_OUTPUT);

    timer_config_t timerCfg;
    timerCfg.alarm_en = TIMER_ALARM_EN;
    timerCfg.auto_reload = TIMER_AUTORELOAD_EN;
    timerCfg.counter_dir = TIMER_COUNT_UP;
    timerCfg.divider = 80;  // 80M/80 = 1M
    timer_init(0, 0, &timerCfg);
    timer_set_counter_value(0, 0, 0);
    timer_set_alarm_value(0, 0, 1000 * 1000 * 1);  // 1秒
    timer_isr_callback_add(0, 0, timerIsr, NULL, ESP_INTR_FLAG_IRAM);
}

void app_main(void) 
{ 
    initTimer(); 
}
```
## 配置回调法
```c
#include <stdio.h>

#include "driver/timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#define TIMER_DIVIDER 16                             //  硬件定时器分频器
#define TIMER_FREQ (TIMER_BASE_CLK / TIMER_DIVIDER)  // 定时器计数频率

// 时间(s) = (double) val / TIMER_FREQ  //定时器的时间


static xQueueHandle queue;

static bool timer_callback(void *args)
{
    uint64_t val;
    BaseType_t pxHigherPriorityTaskWoken = pdFALSE;

    val = timer_group_get_counter_value_in_isr(0, 0);
    // 取定时器值函数的使用【必须调用带有_in_isr的函数】

    // 通过队列将 val 传给任务
    xQueueSendFromISR(queue, (void *)&val, &pxHigherPriorityTaskWoken);

    return pxHigherPriorityTaskWoken;
}

static void task(void *arg)
{
    uint64_t counts = 0ull;
    while (1) {
        counts++;
        uint64_t val;
        if (xQueueReceive(queue, &val, portMAX_DELAY)) {
            printf("定时器第 %llu 次中断,%lld\n", counts, val);
        }
    }
}
void app_main(void)
{
    queue = xQueueCreate(10, sizeof(uint64_t));
    timer_config_t config = {
        .alarm_en = 1,
        .counter_en = 0,
        .counter_dir = TIMER_COUNT_UP,
        .auto_reload = 1,
        .divider = 16,
    };
    timer_init(0, 0, &config);
    timer_set_counter_value(0, 0, 0x00ull);
    timer_set_alarm_value(0, 0, TIMER_FREQ * 3);
    timer_enable_intr(0, 0);

    timer_isr_callback_add(0, 0, timer_callback, NULL, ESP_INTR_FLAG_IRAM);

    xTaskCreate(task, "timer_test_task", 2048, NULL, 5, NULL);
    timer_start(0, 0);
    printf("定时器启动成功！");
}

```
## 自定义 ISR
```c
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/timer.h"
#include "freertos/queue.h"

#define TIMER_DIVIDER         16  //  Hardware timer clock divider
#define TIMER_FREQ           (TIMER_BASE_CLK / TIMER_DIVIDER)  // convert counter value to seconds
//转换公式
//count = TIMER_SCALE * time(second);
//time(second) = count / TIMER_SCALE;

static xQueueHandle queue;
//回调函数
static void IRAM_ATTR timerIsr(void *arg){
    /**
     1. 获取自旋锁timer_spinlock_take(参数：定时器组索引)
     2. 显式清除中断状态timer_group_clr_intr_status_in_isr(group_idx, index)
     3. 再次使能中断timer_group_enable_alarm_in_isr(group_idx, index);
     4. 释放自旋锁timer_spinlock_take(参数：定时器组索引)
     */
    timer_spinlock_take(0);    

    uint64_t val = 0;
	val = timer_group_get_counter_value_in_isr(0, 0);
	    
    timer_group_clr_intr_status_in_isr(0, 0);
    timer_group_enable_alarm_in_isr(0, 0);
    
    xQueueSendFromISR(queue, &val, NULL);
    
    timer_spinlock_give(0);
}

static void task(void *arg){
    uint64_t counts = 0ull;
    while(1){
        counts ++;
        uint64_t val;
        if(xQueueReceive(queue, &val, portMAX_DELAY)){
            printf("定时器第 %llu 次中断\n", counts);
        }
    }
}
void app_main(void)
{
    queue = xQueueCreate(10, sizeof(uint64_t));
    timer_config_t config = {
        .alarm_en = 1,
        .counter_en = 0,
        .counter_dir = TIMER_COUNT_UP,
        .auto_reload = 1,
        .divider = 16,
    };
    timer_init(0, 0, &config);
    timer_set_counter_value(0, 0, 0x00ull);
    timer_set_alarm_value(0, 0, TIMER_FREQ * 3);
    timer_enable_intr(0, 0);
    timer_isr_register(0, 0, timerIsr, &config, ESP_INTR_FLAG_IRAM, NULL);
    xTaskCreate(task, "timer_test_task", 2048, NULL, 5, NULL);
    timer_start(0, 0);
    printf("定时器启动成功！")
}

```
