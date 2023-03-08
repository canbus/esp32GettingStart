# mutex互斥量
## 概述
    怎么独享厕所？自己开门上锁，完事了自己开锁。
    你当然可以进去后，让别人帮你把门：但是，命运就掌握在别人手上了。

    使用队列、信号量，都可以实现互斥访问，以信号量为例：
        信号量初始值为1
        任务A想上厕所，"take"信号量成功，它进入厕所
        任务B也想上厕所，"take"信号量不成功，等待
        任务A用完厕所，"give"信号量；轮到任务B使用

    这需要有2个前提：
        任务B很老实，不撬门(一开始不"give"信号量)
        没有坏人：别的任务不会"give"信号量

    可以看到，使用信号量确实也可以实现互斥访问，但是不完美。

    使用互斥量可以解决这个问题，互斥量的名字取得很好：
        量：值为0、1
        互斥：用来实现互斥访问

    它的核心在于：谁上锁，就只能由谁开锁。
    很奇怪的是，FreeRTOS的互斥锁，并没有在代码上实现这点：  
        即使任务A获得了互斥锁，任务B竟然也可以释放互斥锁。
        谁上锁、谁释放：只是约定。

    本节涉及如下内容：
        为什么要实现互斥操作
        怎么使用互斥量
        互斥量导致的优先级反转、优先级继承
## 互斥量的使用场合
    在多任务系统中，任务A正在使用某个资源，还没用完的情况下任务B也来使用的话，就可能导致问题。

    比如对于串口，任务A正使用它来打印，在打印过程中任务B也来打印，客户看到的结果就是A、B的信息混杂在一起

    这种现象很常见：
        1. 访问外设：串口例子
        2. 读、修改、写操作导致的问题
            对于同一个变量，比如int a，如果有两个任务同时写它就有可能导致问题。
            对于变量的修改，C代码只有一条语句，比如fun(){a=a+8;}，它的内部实现分为3步：读出原值、修改、写入。
                任务A、B同时执行a=a+8的函数，最终结果可能会是1+8+8=17
        3. 对变量的非原子化访问
            修改变量、设置结构体、在16位的机器上写32位的变量，这些操作都是非原子的。也就是它们的操作过程都可能被打断，如果被打断的过程有其他任务来操作这些变量，就可能导致冲突
        4. 函数重入
            "可重入的函数"是指：多个任务同时调用它、任务和中断同时调用它，函数的运行也是安全的。可重入的函数也被称为"线程安全”(thread safe)。
            每个任务都维持自己的栈、自己的CPU寄存器，如果一个函数只使用局部变量，那么它就是线程安全的。
            函数中一旦使用了全局变量、静态变量、其他外设，它就不是"可重入的"，如果该函数正在被调用，就必须阻止其他任务、中断再次调用它。

    上述问题的解决方法是：任务A访问这些全局变量、函数代码时，独占它，就是上个锁。这些全局变量、函数代码必须被独占地使用，它们被称为临界资源

    互斥量也被称为互斥锁，使用过程如下：
        互斥量初始值为1
        任务A想访问临界资源，先获得并占有互斥量，然后开始访问
        任务B也想访问临界资源，也要先获得互斥量：被别人占有了，于是阻塞
        任务A使用完毕，释放互斥量；任务B被唤醒、得到并占有互斥量，然后开始访问临界资源
        任务B使用完毕，释放互斥量
## 互斥量函数
### 创建
  + 互斥量是一种特殊的二进制信号量。
  + 使用互斥量时，先创建、然后去获得、释放它。使用句柄来表示一个互斥量。
  + 创建互斥量的函数有2种：动态分配内存，静态分配内存，函数原型如下：
    ```c
    /* 创建一个互斥量，返回它的句柄。
    * 此函数内部会分配互斥量结构体 
    * 返回值: 返回句柄，非NULL表示成功
    */
    SemaphoreHandle_t xSemaphoreCreateMutex( void );

    /* 创建一个静态互斥量，返回它的句柄。
    * 此函数无需动态分配内存，所以需要先有一个StaticSemaphore_t结构体，并传入它的指针
    * 返回值: 返回句柄，非NULL表示成功
    */
    SemaphoreHandle_t xSemaphoreCreateMutexStatic( StaticSemaphore_t *pxMutexBuffer );
    ```
  + 要想使用互斥量，需要在配置文件FreeRTOSConfig.h中定义: `#define configUSE_MUTEXES 1`
### Give/Take
  + 要注意的是，互斥量不能在ISR中使用。
  + 操作函数，比如删除、give/take，跟一般是信号量是一样的
    ```c
    /*
    * xSemaphore: 信号量句柄，你要删除哪个信号量, 互斥量也是一种信号量
    */
    void vSemaphoreDelete( SemaphoreHandle_t xSemaphore );

    /* 释放 */
    BaseType_t xSemaphoreGive( SemaphoreHandle_t xSemaphore );

    /* 释放(ISR版本) */
    BaseType_t xSemaphoreGiveFromISR(SemaphoreHandle_t xSemaphore,BaseType_t *pxHigherPriorityTaskWoken);

    /* 获得 */
    BaseType_t xSemaphoreTake(SemaphoreHandle_t xSemaphore,TickType_t xTicksToWait);
    /* 获得(ISR版本) */
    xSemaphoreGiveFromISR(SemaphoreHandle_t xSemaphore,BaseType_t *pxHigherPriorityTaskWoken);
    ```
## 示例15: 互斥量基本使用
  + 使用互斥量时有如下特点：
    - 刚创建的互斥量可以被成功"take"
    - “take"互斥量成功的任务，被称为"holder”，只能由它"give"互斥量；别的任务"give"不成功
    - 在ISR中不能使用互斥量
  + 下面程序创建2个发送任务：故意发送大量的字符。做2个实验：
    - 不使用互斥量：任务1、任务2打印的字符串混杂在一起
     ```c
        void task1(void *arg)
        {
            while (1) {
                for(int i=0;i<10;i++)
                    printf("this is task1 %d\n",i);
            }
        }
        void task2(void *arg)
        {
            while (1) {
                for(int i=0;i<10;i++)
                    printf("this is task2 %d\n",i);
            }
        }

        void app_main(void)
        {
            xTaskCreate(task2, "task2", 1024 * 2, (void *)"recv", 3, NULL);
            xTaskCreate(task1, "task1", 1024 * 2, (void *)"send", 3, NULL);
        }
     ```
    - 使用互斥量：可以看到任务1、任务2打印的字符串没有混杂在一起
    ```c
        void task1(void *arg)
        {
            while (1) {
                xSemaphoreTake(mutex, portMAX_DELAY);
                for(int i=0;i<10;i++)
                    printf("this is task1 %d\n",i);
                xSemaphoreGive(mutex);
            }
        }
        void task2(void *arg)
        {
            while (1) {
                xSemaphoreTake(mutex, portMAX_DELAY);
                for(int i=0;i<10;i++)
                    printf("this is task2 %d\n",i);
                xSemaphoreGive(mutex);
            }
        }

        void app_main(void)
        {
            mutex = xSemaphoreCreateMutex();
            xTaskCreate(task2, "task2", 1024 * 2, (void *)"recv", 3, NULL);
            xTaskCreate(task1, "task1", 1024 * 2, (void *)"send", 3, NULL);
        }
     ```
## 示例16: 谁上锁就由谁解锁？
    互斥量、互斥锁，本来的概念确实是：谁上锁就得由谁解锁。

    但是FreeRTOS并没有实现这点，只是要求程序员按照这样的惯例写代码。

    但是esp32修改了FreeRTOS,如果去解锁不是自已上的锁,会抛出异常
    assert failed: xQueueGenericSend queue.c:782 (pxQueue->pcHead != ((void *)0) || pxQueue->u.xSemaphore.xMutexHolder == ((void *)0) || pxQueue->u.xSemaphore.xMutexHolder == xTaskGetCurrentTaskHandle())

    main函数创建了2个任务：
        任务1：高优先级，一开始就获得互斥锁，永远不释放。
        任务2：任务1阻塞时它开始执行，它先尝试获得互斥量，失败的话就监守自盗(释放互斥量、开锁)，然后再上锁
   + 执行流程 
     - A：任务1的优先级高，先运行，立刻上锁
     - B：任务1阻塞
     - C：任务2开始执行，尝试获得互斥量(上锁)，超时时间设为0。根据返回值打印出：上锁失败
     - D：任务2监守自盗，开锁，抛异常
   + 代码```c
    void task1(void *arg)
    {
        while (1) {
            xSemaphoreTake(mutex, portMAX_DELAY);
            printf("task1 task mutex\n");
            vTaskDelay(pdMS_TO_TICKS(1000 * 10));
        }
    }
    void task2(void *arg)
    {
        vTaskDelay(pdMS_TO_TICKS(10));
        while (1) {
            if(pdTRUE != xSemaphoreTake(mutex, 0)){
                printf("task2 take mutex fail\n");
                xSemaphoreGive(mutex);
            }
            xSemaphoreTake(mutex, portMAX_DELAY);
            printf("task2 task mutex\n");
            vTaskDelay(pdMS_TO_TICKS(1000 * 10));
        }
    }
    ```    
## 示例17: 优先级反转
  + 假设任务A、B都想使用串口，A优先级比较低：
    - 任务A获得了串口的互斥量
    - 任务B也想使用串口，它将会阻塞、等待A释放互斥量
    - 高优先级的任务，被低优先级的任务延迟，这被称为"优先级反转"(priority inversion)
    - 如果涉及3个任务，可以让"优先级反转"的后果更加恶劣。
  + 代码 
    ```c
        void task1(void *arg)
        {
            while (1) {
                xSemaphoreTake(mutex, portMAX_DELAY);
                printf("task1 take mutex\n");
                vTaskDelay(pdMS_TO_TICKS(1000 * 10));
                printf("task1 give mutex\n");
                xSemaphoreGive(mutex);
            }
        }
        void task2(void *arg)
        {
            vTaskDelay(pdMS_TO_TICKS(10));
            while (1) {
                printf("task2 run\n");
                xSemaphoreTake(mutex, portMAX_DELAY);
                printf("task2 take mutex\n");
                vTaskDelay(pdMS_TO_TICKS(1000 * 10));
                xSemaphoreGive(mutex);
            }
        }
        void app_main(void)
        {
            mutex = xSemaphoreCreateMutex();
            xTaskCreate(task2, "task2", 1024 * 2, (void *)"recv", 4, NULL);
            xTaskCreate(task1, "task1", 1024 * 2, (void *)"send", 3, NULL);
        }
    ```
  + 二值信号量"优先级反转"的恶劣后果 
    + 创建3个任务：vTaskL/vTaskM/vTaskH(低/中/高优先级任务)及二值信号量(xSemaphoreCreateBinary)
    + 运行流程
      - A：vTaskH优先级最高，它最先运行。随后Delay,vTaskH阻塞
      - B：vTaskM开始运行。随后Delay, vTaskM阻塞
      - C：vTaskL开始运行，获得二值信号量，但还没有释放
      - D：vTaskH Delay时间到，vTaskH恢复运行，它无法获得二进制信号量，一直阻塞等待
      - E：vTaskM Delay时间到，vTaskM恢复运行，它比vTaskL优先级高，一直运行。导致vTaskL无法运行，自然无法释放二进制信号量，于是HPTask也无法运行。 
    + ```c
        SemaphoreHandle_t binSem;
        void vTaskH(void *arg)
        {
            vTaskDelay(pdMS_TO_TICKS(20));
            while (1) {
                printf("vTaskH wait binSem\n");
                xSemaphoreTake(binSem, portMAX_DELAY);
                printf("vTaskH take mutex\n");
                vTaskDelay(pdMS_TO_TICKS(1000 * 10));
                xSemaphoreGive(binSem);
            }
        }
        void vTaskM(void *arg)
        {
            vTaskDelay(pdMS_TO_TICKS(10));
            while (1) {
                printf("vTaskM wait binSem\n");
                if(pdTRUE == xSemaphoreTake(binSem, 0)){
                printf("vTaskM take mutex\n");
                vTaskDelay(pdMS_TO_TICKS(1000 * 10));
                    xSemaphoreGive(binSem);
                }
            }
        }
        void vTaskL(void *arg)
        {
            while (1) {
                printf("vTaskL wait binSem\n");
                xSemaphoreTake(binSem, portMAX_DELAY);
                printf("vTaskL take mutex\n");
                for(int i=0;i<100000;i++){
                    printf("vTaskL running..............:%d\n",i);
                }
                xSemaphoreGive(binSem);
            }
        }
        void app_main(void)
        {
            binSem = xSemaphoreCreateBinary();
            xSemaphoreGive(binSem);
            xTaskCreate(vTaskH, "", 1024 * 2, NULL, 5, NULL);
            xTaskCreate(vTaskM, "", 1024 * 2, NULL, 4, NULL);
            xTaskCreate(vTaskL, "", 1024 * 2, NULL, 3, NULL);
        }
      ``` 
## 示例18: 优先级继承
    示例17的问题在于，LTask低优先级任务获得了锁，但是它优先级太低而无法运行。
    如果能提升LTask任务的优先级，让它能尽快运行、释放锁，"优先级反转"的问题不就解决了吗？
    把LTask任务的优先级提升到什么水平？
    
  + 优先级继承：
     - 假设持有互斥锁的是任务A，如果更高优先级的任务B也尝试获得这个锁
     - 任务B说：你既然持有宝剑，又不给我，那就继承我的愿望吧
     - 于是任务A就继承了任务B的优先级
     - 这就叫：优先级继承
     - 等任务A释放互斥锁时，它就恢复为原来的优先级
     - 互斥锁内部就实现了优先级的提升、恢复
  
  + 运行时序如下所示：
     - A：HTask执行xSemaphoreTake();，它的优先级被LTask继承
     - B：LTask抢占MTask，运行
     - C：LTask执行xSemaphoreGive;，它的优先级恢复为原来值
     - D：HTask得到互斥锁，开始运行
     - 互斥锁的"优先级继承"，可以减小"优先级反转"的影响
  + ```c
        SemaphoreHandle_t mutex;
        void vTaskH(void *arg)
        {
            vTaskDelay(pdMS_TO_TICKS(20));
            while (1) {
                printf("vTaskH wait mutex\n");
                xSemaphoreTake(mutex, portMAX_DELAY);
                printf("vTaskH take mutex\n");
                xSemaphoreGive(mutex);
                vTaskDelay(pdMS_TO_TICKS(100));
            }
        }
        void vTaskM(void *arg)
        {
            vTaskDelay(pdMS_TO_TICKS(10));
            while (1) {
                printf("vTaskM wait binSem\n");
                if(pdTRUE == xSemaphoreTake(mutex, 0)){
                printf("vTaskM take mutex\n");
                vTaskDelay(pdMS_TO_TICKS(1000 ));
                xSemaphoreGive(mutex);
                }
            }
        }
        void vTaskL(void *arg)
        {
            printf("vTaskL pri:%d\n",uxTaskPriorityGet(NULL));
            while (1) {
                printf("vTaskL wait mutex\n");
                xSemaphoreTake(mutex, portMAX_DELAY);
                printf("vTaskL take mutex\n");
                for(int i=0;i<100;i++){
                    printf("vTaskL pri:%d,%d\n",uxTaskPriorityGet(NULL),i);
                }
                xSemaphoreGive(mutex);
            }
        }
        void app_main(void)
        {
            mutex = xSemaphoreCreateMutex();
            xTaskCreate(vTaskH, "", 1024 * 2, NULL, 5, NULL);
            xTaskCreate(vTaskM, "", 1024 * 2, NULL, 4, NULL);
            xTaskCreate(vTaskL, "", 1024 * 2, NULL, 3, NULL);
        }
    ```
## 递归锁
### 死锁的概念
    日常生活的死锁：我们只招有工作经验的人！我没有工作经验怎么办？那你就去找工作啊！

  + 假设有2个互斥量M1、M2，2个任务A、B：
    - A获得了互斥量M1
    - B获得了互斥量M2
    - A还要获得互斥量M2才能运行，结果A阻塞
    - B还要获得互斥量M1才能运行，结果B阻塞
    - A、B都阻塞，再无法释放它们持有的互斥量
    - 死锁发生！
### 自我死锁
  + 假设这样的场景：
    - 任务A获得了互斥锁M
    - 它调用一个库函数
    - 库函数要去获取同一个互斥锁M，于是它阻塞：任务A休眠，等待任务A来释放互斥锁！
    - 死锁发生！
### 函数
  + 怎么解决这类问题？可以使用递归锁(Recursive Mutexes)，它的特性如下：
    - 任务A获得递归锁M后，它还可以多次去获得这个锁
    - "take"了N次，要"give"N次，这个锁才会被释放
  + 递归锁的函数根一般互斥量的函数名不一样，参数类型一样，列表如下
  + |      |             递归锁              |     一般互斥量        |
    |------|--------------------------------|-----------------------|
    | 创建 | xSemaphoreCreateRecursiveMutex | xSemaphoreCreateMutex |
    | 获得 | xSemaphoreTakeRecursive        | xSemaphoreTake        |
    | 释放 | xSemaphoreGiveRecursive        | xSemaphoreGive        |
  + ```c
        /* 创建一个递归锁，返回它的句柄。
        * 此函数内部会分配互斥量结构体 
        * 返回值: 返回句柄，非NULL表示成功
        */
        SemaphoreHandle_t xSemaphoreCreateRecursiveMutex( void );

        /* 释放 */
        BaseType_t xSemaphoreGiveRecursive( SemaphoreHandle_t xSemaphore );

        /* 获得 */
        BaseType_t xSemaphoreTakeRecursive(SemaphoreHandle_t xSemaphore,TickType_t xTicksToWait);
    ```
### 示例19: 递归锁
   + main函数里创建了2个任务
     - 任务1：高优先级，一开始就获得递归锁，然后故意等待很长时间，让任务2运行
     - 任务2：低优先级，看看能否操作别人持有的锁 
   + 运行流程如下图所示：
     - A：任务1优先级最高，先运行，获得递归锁
     - B：任务1阻塞，让任务2得以运行
     - C：任务2运行，看看能否获得别人持有的递归锁：不能
     - D：任务2故意执行"give"操作，看看能否释放别人持有的递归锁：不能
     - E：任务2等待递归锁
     - F：任务1阻塞时间到后继续运行，使用循环多次获得、释放递归锁
     - 递归锁在代码上实现了：谁持有递归锁，必须由谁释放
   + ```c
        SemaphoreHandle_t recuMutex;
        void vTaskH(void *arg)
        {
            while (1) {
                ESP_LOGI("vTask","vTaskH wait mutex");
                xSemaphoreTakeRecursive(recuMutex, portMAX_DELAY);
                ESP_LOGI("vTask","vTaskH take mutex");
                // for(int i=0;i<10;i++){
                //     xSemaphoreTakeRecursive(recuMutex, portMAX_DELAY);
                //     ESP_LOGI("vTask","vTaskH take mutex %d",i);
                //     xSemaphoreGiveRecursive(recuMutex);
                // }
                vTaskDelay(pdMS_TO_TICKS(1000 * 5));
                xSemaphoreGiveRecursive(recuMutex);
            }
        }
        void vTaskL(void *arg)
        {
            vTaskDelay(1);
            while (1) {
                ESP_LOGI("vTask","vTaskL wait mutex");
                if(pdTRUE == xSemaphoreTakeRecursive(recuMutex, 0)){
                ESP_LOGI("vTask","vTaskL take mutex"); 
                
                }else{
                    ESP_LOGI("vTask","vTaskL give mutex");
                    xSemaphoreGiveRecursive(recuMutex);
                }
                vTaskDelay(1000);
            }
        }
        void app_main(void)
        {
            recuMutex = xSemaphoreCreateRecursiveMutex();
            xTaskCreate(vTaskH, "", 1024 * 2, NULL, 5, NULL);
            xTaskCreate(vTaskL, "", 1024 * 2, NULL, 3, NULL);
        }
     ``` 