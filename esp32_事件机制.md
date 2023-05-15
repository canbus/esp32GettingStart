# ESP32 事件机制
1. 文档链接https://docs.espressif.com/projects/esp-idf/zh_CN/latest/esp32/api-guides/event-handling.html
2. 什么是事件？ 什么是事件处理机制？
    事件和信号其实是同样的道理，来了一个事件（或者信号）去执行一个函数去处理一些事情，类似于单片机机的中断。
    使用事件和事件处理函数解决问题的方式就成为事件处理机制。
    ESP32的一些组件在状态发生改变时使用事件去通知应用程序，比如wifi的连接和断开事件，下面介绍ESP32的事件处理机制。
3. Legacy Event Loop  传统的事件循环
    在esp_event Library事件库引入之前，wifi驱动，以太网，tcp/ip协议栈的事件都是通过传统的事件循环处理的，事件循环可以看作一个while(1) ;循环。
    传统的事件循环只支持系统已经定义好的事件id和事件信息结构，对于用户自定义的事件和蓝牙mesh事件是无法处理的。
    传统事件只支持一个事件处理函数，而且应用程序组件不能独自处理wifi和ip事件，需要应用程序将这些事件抛出。
4. esp_event Library Event Loop esp事件库提供的事件循环
    事件库用来取代传统的事件循环，并提供了一个默认的事件循环来处理wifi和ip事件。
    事件库的能力：
     事件库允许组件声明一个事件到其他组件注册的处理函数中，当事件发生时执行相应的处理函数。这允许松散耦合的组件间不需要通过应用程序的调用在其他组件状态发生变化时另一个组件执行相应的操作。例如，将所有的连接处理放在一个高级的库中，订阅其他组件的连接事件，在其他组件连接状态发生变化时，高级的处理进行相应的操作。这种机制以序列化的方式简化了事件的处理并且在另一个上下文中推迟了代码的执行。简单来说，就是将事件的发生地点和事件的处理地点分来，事件处理地点去订阅事件发生地点要发生的事件，且经整个系统的事件处理放在同一个地点处理，简化了系统的设计。
5. esp_event Library API的使用。
    API位置：esp-idf-v4.1\components\esp_event
6. 创建用户事件循环：
    ```c
    #include "esp_event.h"
    #include "string.h"
    //用户自定义事件
    ESP_EVENT_DECLARE_BASE(s_test_base1);
    ESP_EVENT_DEFINE_BASE(s_test_base1);
    //事件id
    enum {
        TEST_EVENT_BASE1_EV1,
        TEST_EVENT_BASE1_EV2,
        TEST_EVENT_BASE1_MAX
    };
    //事件处理函数
    void handle(void* event_handler_arg, esp_event_base_t event_base,  int32_t event_id, void* event_data)
    {
        printf("event_data: %s\n", (char *)event_data);
    }

    void app_main()
    {
        char buf[] = "test event handle";
        //定义事件循环对象
        esp_event_loop_handle_t loop1;
        //事件循环配置
        esp_event_loop_args_t loop_config = {
            .queue_size = 32, //事件队列，设置可接收的事件数量
            .task_name = "loop", //事件是依靠一个单独的freertos任务，设置任务名
            .task_priority = 1, //任务优先级
            .task_stack_size = 2048, //任务栈大小
            .task_core_id = 0 //任务使用的cpu核id
        };
        //创建事件循环
        esp_event_loop_create(&loop_config, &loop1);
        //向循环中注册事件和事件处理函数
        esp_event_handler_register_with(loop1, s_test_base1, TEST_EVENT_BASE1_EV1, handle, NULL);
        
        while(1) {
            //触发事件，验证处理函数是否被调用
            esp_event_post_to(loop1, s_test_base1, TEST_EVENT_BASE1_EV1, buf, strlen(buf) + 1, portMAX_DELAY);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
    }
    ```c

7. 创建默认事件循环（与用户事件循环相比少了表示事件循环的句柄）
    ```c
    #include "esp_event.h"
    #include "string.h"
    //用户自定义事件
    ESP_EVENT_DECLARE_BASE(s_test_base1);
    ESP_EVENT_DEFINE_BASE(s_test_base1);
    //事件id
    enum {
        TEST_EVENT_BASE1_EV1,
        TEST_EVENT_BASE1_EV2,
        TEST_EVENT_BASE1_MAX
    };

    void handle(void* event_handler_arg, esp_event_base_t event_base,  int32_t event_id, void* event_data)
    {
        printf("event_data: %s\n", (char *)event_data);
    }

    void app_main()
    {
        char buf[] = "te's event handle";
        esp_event_loop_create_default();
        esp_event_handler_register(s_test_base1, TEST_EVENT_BASE1_EV1, handle, NULL);

        while(1) {
            esp_event_post(s_test_base1, TEST_EVENT_BASE1_EV1, buf, strlen(buf) + 1, portMAX_DELAY);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
    }
   ``` 
8. 创建不使用专有任务的事件循环(主要是对esp_event_loop_run()函数本质的解读)
    `esp_err_t esp_event_loop_run(esp_event_loop_handle_t event_loop, TickType_t ticks_to_run)`
    该函数官方的描述是 分配一个事件到事件循环，其实从这个层面理解是很难理解清楚的，从函数的字面意思可以看出是让一个事件循环event_loop运行ticks_to_run个tick数，说白了就是临时运行这个事件循环若干时间，如果将这个函数放在while(1)中就是一直运行咯。这个函数是针对在创建时间循环时没有指定任务名（本质就是没有为事件循环单独开启一个freertos任务），那么只有让事件循环运行起来才能正常的接受事件和调用事件处理函数，所以才有调用esp_event_loop_run函数启动事件循环的操作。
    
    下面我以创建单独任务执行esp_event_loop_run()函数为例进行了测试。
    ```c
    #include "esp_event.h"
    #include "string.h"

    //用户自定义事件
    ESP_EVENT_DECLARE_BASE(s_test_base1);
    ESP_EVENT_DEFINE_BASE(s_test_base1);
    //事件id
    enum {
        TEST_EVENT_BASE1_EV1,
        TEST_EVENT_BASE1_EV2,
        TEST_EVENT_BASE1_MAX
    };

    //事件处理函数

    void handle(void* event_handler_arg, esp_event_base_t event_base,  int32_t event_id, void* event_data)

    {

        printf("event_data: %s\n", (char *)event_data);

    }

    

    static void event_loop(void* args)

    {

        esp_event_loop_handle_t event_loop = (esp_event_loop_handle_t) args;

    

        while(1) {

            esp_event_loop_run(event_loop, portMAX_DELAY);

        }

    }

    

    void app_main()

    {

        char buf[] = "test event handle 3";

        //定义事件循环对象

        esp_event_loop_handle_t loop1;

        //事件循环配置

        esp_event_loop_args_t loop_config = {

            .queue_size = 32, //事件队列，设置可接收的事件数量

            .task_name = "loop", //事件是依靠一个单独的freertos任务，设置任务名

            .task_priority = 1, //任务优先级

            .task_stack_size = 2048, //任务栈大小

            .task_core_id = 0 //任务使用的cpu核id

        };

        loop_config.task_name = NULL;

        //创建时间循环

        esp_event_loop_create(&loop_config, &loop1);

        //向循环中注册事件和事件处理函数

        esp_event_handler_register_with(loop1, s_test_base1, TEST_EVENT_BASE1_EV1, handle, NULL);

    

        TaskHandle_t mtask;

        xTaskCreate(event_loop, "task", 2584, (void*) loop1, 1, &mtask);

        //esp_event_loop_run(event_loop, portMAX_DELAY);

        while(1) {

            //触发事件，验证处理函数是否被调用

            esp_event_post_to(loop1, s_test_base1, TEST_EVENT_BASE1_EV1, buf, strlen(buf) + 1, portMAX_DELAY);

            vTaskDelay(1000 / portTICK_PERIOD_MS);

        }

    }
    ```
9. 总结：
    esp_event Library API使用步骤：
      1.  创建事件循环 ；
      2.  注册事件和事件处理函数 （关于事件的定义和事件处理函数的定义可以参考我的案例）；
      3.  触发事件，执行事件处理函数；
