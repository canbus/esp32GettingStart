# 硬件定时器
## 配置定时器
ESP32内置了两个定时器组 Timer Group，每个定时器组都有两个64位定时器Timer。支持向上、向下两个方向计数。支持设置警报阈值
1. 定时器初始化
定时器初始化需要通过函数timer_init传递一个timer_config_t结构体来完成对定时器的初始化
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
2. 常用的API
    #include "driver/timer.h"
    timer_init(0,0,&config);
    timer_set_counter_value(0,0, 0x00);//装载定时器初值为0
    timer_start(0,0);
    timer_get_counter_value(0,0,&tick);
    timer_deinit(0,0);
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
