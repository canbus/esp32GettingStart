# [任务通知](https://www.freertos.org/zh-cn-cmn-s/RTOS-task-notification-API.html)
## 概述
 + FreeRTOS 从 V8.2.0 版本开始提供任务通知这个功能，每个任务都有 一个 32 位 的通知值，在大多数情况下，任务通知可以 替代二值信号量、计数信号量、事件组，也可以替代长度为 1 的队列（可以保存一个 32 位整数或指针值）

 + 相对于以前使用 FreeRTOS 内核通信的资源，必须创建队列、二进制信号量、计数信号量或事件组的情况，使用任务通知显然更灵活。按照 FreeRTOS 官方的说法，使用任务通知比通过信号量等 ICP 通信方式解除阻塞的任务要快 45%，并且更加省 RAM 内存空间（使用 GCC 编译器，-o2 优化级别），任务通知的使用无需创建队列。 想要使用任务通知，必须将 FreeRTOSConfig.h 中的宏定义 configUSE_TASK_NOTIFICATIONS 设置为 1，其实FreeRTOS 默认是为 1 的，所以任务通知是默认使能的。
  
 + FreeRTOS 提供以下几种方式发送通知给任务 ：
    - 发送通知给任务，如果有通知未读，不覆盖通知值。
    - 发送通知给任务，直接覆盖通知值。
    - 发送通知给任务，设置通知值的一个或者多个位 ，可以当做事件组来使用。
    - 发送通知给任务，递增通知值，可以当做计数信号量使用。
    - 通过对以上任务通知方式的合理使用，可以在一定场合下替代 FreeRTOS 的信号量，队列、事件组等。
     
 + 当然，凡是都有利弊，不然的话 FreeRTOS 还要内核的 IPC 通信机制干嘛，消息通知虽然处理更快，RAM 开销更小，但也有以下限制 ：
    - 只能有一个任务接收通知消息，因为必须指定接收通知的任务。
    - 只有等待通知的任务可以被阻塞，发送通知的任务，在任何情况下都不会因为发送失败而进入阻塞态
## API 汇总
  - xTaskNotifyGive() / xTaskNotifyGiveIndexed()
  - vTaskNotifyGiveFromISR() / vTaskNotifyGiveIndexedFromISR()
  - ulTaskNotifyTake() / ulTaskNotifyTakeIndexed()
  - xTaskNotify() / xTaskNotifyIndexed()
  - xTaskNotifyAndQuery() / xTaskNotifyAndQueryIndexed()
  - xTaskNotifyAndQueryFromISR / xTaskNotifyAndQueryFromISRIndexed()/
  - xTaskNotifyFromISR() / xTaskNotifyFromISRIndexed()
  - xTaskNotifyWait() / xTaskNotifyWaitIndexed()
  - xTaskNotifyStateClear() / xTaskNotifyStateClearIndexed()
  - ulTasknotifyValueClear() / ulTasknotifyValueClearIndexed()
## xTaskNotifyGive() / ulTaskNotifyTake()
  + 概述
    - 每个RTOS任务都有一个32位通知值(ulNotifiedValue)，在创建RTOS任务时初始化为零。RTOS
    - 任务通知是直接发送到任务的事件，并可以选择更新接收任务的通知值。
    - xTaskNotifyGive()用于递增任务通知值(ulNotifiedValue).相当于xSemaphoreGive()
    - ulTaskNotifyTake()用于测试通知值(ulNotifiedValue),不为0时，ulTaskNotifyTake()将返回
    - xTaskNotifyGive()/ulTaskNotifyTake() 可以用为轻量级、更快的二进制或计数信号量 替代品  
  + API 
    ```c
    BaseType_t xTaskNotifyGive(TaskHandle_t xTaskToNotify );//通知任务,并使其通知值(ulNotifiedValue)增加
    BaseType_t xTaskNotifyGiveIndexed( TaskHandle_t xTaskToNotify, UBaseType_t uxIndexToNotify );//esp32未实现

    //任务的通知值(ulNotifiedValue)不为0时，ulTaskNotifyTake() 将返回,并在返回之前递减任务的通知值或者清0
    uint32_t ulTaskNotifyTake( BaseType_t xClearCountOnExit,TickType_t xTicksToWait );
    uint32_t ulTaskNotifyTakeIndexed( UBaseType_t uxIndexToWaitOn, BaseType_t xClearCountOnExit,TickType_t );//esp32未实现
    /**
     * xTaskToNotify    目标任务句柄,可以通过 xTaskCreateStatic()/xTaskGetHandle()/ xTaskGetCurrentTaskHandle()获取
     * uxIndexToNotify  目标任务通知值数组中的索引,通知值将发送给该索引。xTaskNotify()默认向索引0发送通知
     * xClearCountOnExit xClearCountOnExit=pdFALSE,则"通知值"在ulTaskNotifyTake()退出之前递减.等同调用xSemaphoreTake将信号量递减
                         xClearCountOnExit=pdTRUE,则通知值(ulNotifiedValue)退出之前重设为0.等同xSemaphoreTake将信号量置为0
     */
    ```
  + 示例1:
    - 创建二个task,task2优先级更快
    - task1用ulTaskNotifyTake (pdTRUE, portMAX_DELAY);等待任务通知,task1 blocked
    - task2延时2秒后,给task1发送通知xTaskNotifyGive()
    - task1恢复运行
    ```c
        #define INFO(format, ...) ESP_LOGI("info", format, ##__VA_ARGS__) //printf(format, ##__VA_ARGS__)
        TaskHandle_t task1,task2;
        void vTask1(void *arg)
        {
            while(1){
                INFO("task 1 waitting for notify");
                ulTaskNotifyTake (pdTRUE, portMAX_DELAY);
                INFO("task 1 get a notify\n");
            }
        }
        void vTask2(void *arg)
        {
            while(1){
                INFO("task2 run...");
                vTaskDelay(pdMS_TO_TICKS(1000 * 1));
                INFO("notify task1");
                xTaskNotifyGive(task1);
                vTaskDelay(pdMS_TO_TICKS(1000 * 5));
            }
        }

        void app_main(void)
        {
            xTaskCreate(vTask1, "task1", 1024 * 2, NULL, 2, &task1);
            xTaskCreate(vTask2, "task2", 1024 * 2, NULL, 3, &task2);
        }
    ```
  + 示例2:notify有可能丢失
    - 在1的基础上,task2一次性发送三次通知
    - 会发现task1只能获得一次通知
    - 要解决这个问题,task1需要用ulTaskNotifyTake (pdFAIL, portMAX_DELAY);获取通知
    ```c
        #define INFO(format, ...) ESP_LOGI("info", format, ##__VA_ARGS__) //printf(format, ##__VA_ARGS__)
        TaskHandle_t task1,task2;
        void vTask1(void *arg)
        {
            while(1){
                INFO("task 1 waitting for notify");
                ulTaskNotifyTake (pdFAIL, portMAX_DELAY); //如果有pdTRUE,则有可能会丢失notify
                INFO("task 1 get a notify\n");
            }
        }
        void vTask2(void *arg)
        {
            while(1){
                INFO("task2 run...");
                vTaskDelay(pdMS_TO_TICKS(1000 * 1));
                INFO("notify task1");
                xTaskNotifyGive(task1); //发送三次notify
                xTaskNotifyGive(task1);
                xTaskNotifyGive(task1);
                vTaskDelay(pdMS_TO_TICKS(1000 * 5));
            }
        }

        void app_main(void)
        {
            xTaskCreate(vTask1, "task1", 1024 * 2, NULL, 2, &task1);
            xTaskCreate(vTask2, "task2", 1024 * 2, NULL, 3, &task2);
        }
    ```   
  + 练习:有二个task,一个task用于接收串口信息.另一个task用于处理串口信息.用taskNotify实现二个task的同步
## xTaskNotify() /  xTaskNotifyWait()
  + xTaskNotify() 将事件直接发送任务,并还可选择以下方式更新接收任务的通知值：
    - 将 32 位数字写入通知值
    - 添加一个（递增）通知值
    - 设置通知值中的一个或多个位
    - 保持通知值不变  
  + API
    ```c
    BaseType_t xTaskNotify( TaskHandle_t xTaskToNotify, uint32_t ulValue, eNotifyAction eAction );
    BaseType_t xTaskNotifyIndexed( xTaskToNotify , UBaseType_t uxIndexToNotify,ulValue , eAction);
    BaseType_t xTaskNotifyFromISR( xTaskToNotify, ulValue, eAction,BaseType_t *pxHigherPriorityTaskWoken );
    BaseType_t xTaskNotifyIndexedFromISR( xTaskToNotify, uxIndexToNotify, ulValue, eAction,*pxHigherPriorityTaskWoken );

    BaseType_t xTaskNotifyWait( uint32_t ulBitsToClearOnEntry,uint32_t ulBitsToClearOnExit,
                             uint32_t *pulNotificationValue,TickType_t xTicksToWait );
    BaseType_t xTaskNotifyWaitIndexed( ...);
    /*xTaskToNotify 目标任务句柄,可以通过 xTaskCreateStatic()/xTaskGetHandle()/ xTaskGetCurrentTaskHandle()获取
      ulValue  	    用于更新目标任务的通知值。 请参阅下面 eAction 参数的说明
      eAction  	    枚举类型，可以采用下表中记录的值之一来执行相关操作
            eNoAction	不更新通知值,在这种情况下,不使用 ulValue
            eSetBits	目标任务的通知值使用ulValue按位或运算.如:如果ulValue设为0x01,则将在目标任务的通知值中设置位0
                        同样,如果ulValue为0x04,则将在目标任务的通知值中设置位2.
                        通过这种方式,RTOS任务通知机制可以用作事件组的轻量级替代方案
            eIncrement	目标任务的通知值自增1,使得调用xTaskNotify()相当于调用 xTaskNotifyGive().在这种情况下不使用 ulValue
            eSetValueWithOverwrite	目标任务的通知值无条件设置为 ulValue.因此,任务通知可用作xQueueOverwrite()的轻量级替代方案
            eSetValueWithoutOrwrite	如果目标任务没有挂起的通知，则其通知值将设置为 ulValue。
                                    如果目标任务已经有挂起的通知，则不会更新其通知值,因为这样做会覆盖之前使用的值.
                                    在这种情况下,调用xTaskNotify()失败,返回 pdFALSE。
                                    因此,可在长度为1的队列上用作xQueueSend()在长度为1的轻量级替代方案。  
      uxIndexToNotify 目标任务通知值数组中的索引,通知值将发送给该索引。xTaskNotify()默认向索引0发送通知
      Returns:  除eAction设置为eSetValueWithoutOverwrite且目标任务的通知值,
                因目标任务已有挂起的通知而无法更新之外,在所有情况下均返回 pdPASS

      ulBitsToClearOnEntry      在 xTaskNotifyWait()函数入口,ulBitsToClearOnEntry中设置的任何位都将在调用任务通知值中被清除
                                例如:ulBitsToClearOnEntry=0x01,则任务通知值的位 0 将在函数入口被清除。
                                    ulBitsToClearOnEntry=0xffffffff (ULONG_MAX),将清除任务通知值中的所有位,通知值被清除为0。
      ulBitsToClearOnExit  	    在 xTaskNotifyWait()退出前,ulBitsToClearOnExit 中设置的任何位都将在任务通知值中被清除
                                在任务通知值保存到 *pulNotificationValue 中之后清除位 
                                例如:ulBitsToClearOnExit=0x03,则位 0 和 位 1 将在函数退出之前 清除。
                                     ulBitsToClearOnExit=0xffffffff (ULONG_MAX) 将清除任务通知值中的所有位,通知值被清除为0
      pulNotificationValue  	用于传出任务通知值。如果不需,可以设为NULL
      Returns:   在调用 xTaskNotifyWait() 时，如果收到通知， 或通知已经挂起，则返回 pdTRUE。超时， 则返回 pdFALSE
    */
    ``` 
  + 示例1
    - 建立二个任务
    - 任务1用xTaskNotifyWait等候通知,task1 block
    - 任务2用xTaskNotify发送通知.task1 会收到通知
    ```c
    #define INFO(format, ...) ESP_LOGI("info", format, ##__VA_ARGS__) //printf(format, ##__VA_ARGS__)
    TaskHandle_t task1,task2;
    void vTask1(void *arg)
    {
        uint32_t notifyVal;
        while(1){
            INFO("task 1 waitting for notify");
            xTaskNotifyWait(0x00,       /* Don't clear any notification bits on entry. */   
                            ULONG_MAX,  /* Reset the notification value to 0 on exit. */
                            &notifyVal, /* Notified value pass out in notifyVal */
                            portMAX_DELAY);
            INFO("task 1 get a notify:%d",notifyVal);
        }
    }
    void vTask2(void *arg)
    {
        uint32_t nofityVal = 0;
        while(1){
            INFO("task2 run...");
            vTaskDelay(pdMS_TO_TICKS(1000 * 1));
            INFO("notify task1,%d",nofityVal++);
            xTaskNotify(task1, nofityVal, eSetBits);
            vTaskDelay(pdMS_TO_TICKS(1000 * 2));
        }
    }

    void app_main(void)
    {
        xTaskCreate(vTask1, "task1", 1024 * 2, NULL, 5, &task1);
        xTaskCreate(vTask2, "task2", 1024 * 2, NULL, 3, &task2);
    }
    ```  
  + 示例2:有可能会丢失通知
    - 建立二个任务,task2优先级更高
    - 任务1用xTaskNotifyWait等候通知,task1 block
    - 任务2运行用,延时1秒用xTaskNotify连续发送二次通知
    - task1只会收到通知一次通知
    - 如果需要避免丢失通知,需要把接收任务(task1)的优先级提高
    ```c
    #define INFO(format, ...) ESP_LOGI("info", format, ##__VA_ARGS__) //printf(format, ##__VA_ARGS__)
    TaskHandle_t task1,task2;
    void vTask1(void *arg)
    {
        uint32_t notifyVal;
        while(1){
            INFO("task 1 waitting for notify");
            xTaskNotifyWait(0x00,       /* Don't clear any notification bits on entry. */   
                            0x00,//ULONG_MAX,  /* Reset the notification value to 0 on exit. */
                            &notifyVal, /* Notified value pass out in notifyVal */
                            portMAX_DELAY);
            INFO("task 1 get a notify:%d\n",notifyVal);
        }
    }
    void vTask2(void *arg)
    {
        uint32_t nofityVal = 0;
        while(1){
            INFO("task2 run...");
            vTaskDelay(pdMS_TO_TICKS(1000 * 1));
            INFO("notify task1,%d",nofityVal++);
            xTaskNotify(task1, nofityVal++, eSetBits);
            xTaskNotify(task1, nofityVal, eSetBits);
            vTaskDelay(pdMS_TO_TICKS(1000 * 2));
            vTaskDelete(NULL);
        }
    }

    void app_main(void)
    {
        xTaskCreate(vTask1, "task1", 1024 * 2, NULL, 2, &task1);
        xTaskCreate(vTask2, "task2", 1024 * 2, NULL, 3, &task2);
    } 
    ```
  + 练习1: 
    - 创建三个任务:
    - 一个任务发送任务通知,扫描二个按键并发通知:xTaskNotify(task1, BTN1, eSetValueWithOverwrite)
    - 其他的两个任务是用于接收任务通知(每个任务接受一个按键通知):xTaskNotifyWait(0x00, ULONG_MAX, &notifyVal, portMAX_DELAY);
    - 三个任务独立运行，发送消息任务是通过检测按键的按下情况来发送消息通知，另两个任务获取消息通知，在任务通知中没有可用的通知之前就一直等待消息，一旦获取到消息通知就把消息打印在串口
    - ```c
        #define INFO(format, ...) ESP_LOGI("info", format, ##__VA_ARGS__) //printf(format, ##__VA_ARGS__)
        #define BTN1 16
        #define BTN2 17
        TaskHandle_t task1,task2;
        void vTask1(void *arg)
        {
            uint32_t notifyVal;
            while(1){
                INFO("task 1 waitting for notify");
                xTaskNotifyWait(0x00,       /* Don't clear any notification bits on entry. */   
                                ULONG_MAX,  /* Reset the notification value to 0 on exit. */
                                &notifyVal, /* Notified value pass out in notifyVal */
                                portMAX_DELAY);
                INFO("task 1 get a key:%d\n",notifyVal);
            }
        }
        void vTask2(void *arg)
        {
            uint32_t notifyVal;
            while(1){
                INFO("task 2 waitting for notify");
                xTaskNotifyWait(0x00,       /* Don't clear any notification bits on entry. */   
                                ULONG_MAX,  /* Reset the notification value to 0 on exit. */
                                &notifyVal, /* Notified value pass out in notifyVal */
                                portMAX_DELAY);
                INFO("task 2 get a key:%d\n",notifyVal);
            }
        }

        uint32_t getKey(gpio_num_t btn)
        {
            if(gpio_get_level((gpio_num_t)btn) == 0){
                vTaskDelay(pdMS_TO_TICKS(20));
                if(gpio_get_level((gpio_num_t)btn) == 0){
                    while(1){
                        if(gpio_get_level((gpio_num_t)btn))
                            break;    
                    }
                    INFO("key down");
                    return btn;
                }
            }  
            return 0;
        }
        void vTaskScanKey(void *arg)
        {
            gpio_config_t cfg={0};
            cfg.pin_bit_mask = (1ull << BTN1 | 1ull << BTN2);
            cfg.mode = GPIO_MODE_INPUT;
            cfg.pull_up_en = GPIO_PULLUP_ENABLE;
            cfg.pull_down_en = GPIO_PULLDOWN_DISABLE;
            cfg.intr_type = GPIO_INTR_DISABLE;
            gpio_config(&cfg);

            while (1)
            {
                if(getKey(BTN1) == BTN1){
                    xTaskNotify(task1, BTN1, eSetValueWithOverwrite);
                }else if(getKey(BTN2) == BTN2){
                    xTaskNotify(task2, BTN2, eSetValueWithOverwrite);
                }
                vTaskDelay(100);
            }
        }
        void app_main(void)
        {
            xTaskCreate(vTask1, "task1", 1024 * 2, NULL, 4, &task1);
            xTaskCreate(vTask2, "task2", 1024 * 2, NULL, 4, &task2);
            xTaskCreate(vTaskScanKey,"", 1024 * 2, NULL, 3, NULL);
        }
      ```
  + 练习2:
    - 在练习1的基础上,接收的二个任务改成如下:
    - 接收任务1接受 KEY1 按键 模拟停车场停车操作
    - 接收任务1接受 KEY2 按键 模拟停车场取车操作
    - 并且在串口调试助手输出相应信息
      ```
        车位默认值为 5 个，按下 KEY1 申请车位，按下 KEY2 释放车位！
        KEY1 被按下，成功申请到停车位。当前车位为1
        KEY1 被按下，成功申请到停车位。当前车位为2
        KEY1 被按下，成功申请到停车位。当前车位为3
        KEY1 被按下，成功申请到停车位。当前车位为4
        KEY1 被按下，成功申请到停车位。当前车位为5
        KEY1 被按下，车位已经没有了。请等待释放车位
        KEY2 被按下，释放 1 个停车位
      ``` 