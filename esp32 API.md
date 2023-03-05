
# 编译步骤
    function prompt{"PS >"}
    配置目标芯片：
    idf.py set-target esp32c3
    打开menuconfig配置界面：
    idf.py menuconfig
    编译
    idf.py build
    烧写
    idf.py -p PORT [-b BAUD] flash
    查看串口信息
    idf.py -p PORT monitor 
# 要清除配置:以在命令行执行 `esptool.py -p com16 erase_region 0x9000 0x6000` 
                            或 `esptool.py --port com16 erase_flash`
# CPU相关信息的获取
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    printf("This is %s chip with %d CPU core(s), WiFi%s%s, ",
            CONFIG_IDF_TARGET,
            chip_info.cores,
            (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
            (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");

    printf("silicon revision %d, ", chip_info.revision);

    printf("%dMB %s flash\n", spi_flash_get_chip_size() / (1024 * 1024),
            (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

    printf("Minimum free heap size: %d bytes\n", esp_get_minimum_free_heap_size());
    esp_restart();
## 获取内存使用情况
1. (Top) → Component config → FreeRTOS 启用 [*] Enable FreeRTOS to collect run time stats
2. 创建任务来统计使用情况
   ```c
   //首先创建一个任务单独来做这件事情
    xTaskCreate(state_task, "state_task", 4096, NULL, 6, NULL);

    //回调函数
    static void state_task(void *pvParameters)
    {
        static char InfoBuffer[512] = {0};
        while (1)
        {
            vTaskGetRunTimeStats((char *) &InfoBuffer);
            printf("\r\n任务名       运行计数         使用率\r\n");
            printf("\r\n%s\r\n", InfoBuffer);
            vTaskDelay(2000 / portTICK_PERIOD_MS);
        }
    }
   ```

# 常用API：
    调试:ESP32 的调试工具 coredump(https://blog.csdn.net/tidyjiang/article/details/72123346)
    延时1秒  vTaskDelay(1000 / portTICK_PERIOD_MS);
    重启    longjmp(buf, 0); esp_restart();
    日志级别    esp_log_level_set("*", ESP_LOG_ERROR);        // 设置所有的部件日志详细度为ERROR
               esp_log_level_set("wifi", ESP_LOG_WARN);      // 使能来自WiFi栈的警告日志
  1. IO操作:
     1. 单个IO输出 3:r 4:g 5:b 
        gpio_reset_pin(4);
        gpio_set_direction(4, GPIO_MODE_OUTPUT);//GPIO作为输出
        gpio_set_level(4, 0);//输出低电平
     2. 多个IO输出
        #define GPIO_OUTPUT_IO_0    4
        #define GPIO_OUTPUT_IO_1    5
        #define GPIO_OUTPUT_PIN_SEL  ((1ULL<<GPIO_OUTPUT_IO_0) | (1ULL<<GPIO_OUTPUT_IO_1))
        gpio_config_t io_conf;
        io_conf.intr_type = GPIO_PIN_INTR_DISABLE;//中断
        io_conf.mode = GPIO_MODE_OUTPUT;//输出模式
        io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;//IO4,5设为输出
        io_conf.pull_down_en = 0;//禁止下拉
        io_conf.pull_up_en = 0;//禁止上拉
        gpio_config(&io_conf);//配置使能
        gpio_set_level(4,1);
        gpio_set_level(5,1);
     3. IO输入
        gpio_set_direction(GPIO_NUM_4, GPIO_MODE_INPUT);//GPIO作为输入
        gpio_set_pull_mode(GPIO_NUM_4, GPIO_PULLUP_ONLY);
        int level = gpio_get_level(GPIO_NUM_4); //gpio_get_level()返回0或1
        printf("gpio level:%d\n",level);
     5. 按键输入
        ```c
        #define BUTTON_IO_NUM           9
        #define BUTTON_ACTIVE_LEVEL     0
        static void button_tap_cb(void* arg)
        {
            ESP_LOGI(TAG, "tap cb (%s)", (char *)arg);
            example_ble_mesh_send_gen_onoff_set();
        }

        static void board_button_init(void)
        {
            button_handle_t btn_handle = iot_button_create(BUTTON_IO_NUM, BUTTON_ACTIVE_LEVEL);
            if (btn_handle) {
                iot_button_set_evt_cb(btn_handle, BUTTON_CB_RELEASE, button_tap_cb, "RELEASE");
            }
        }
        ```
     6. 中断
        ```c
            #include <stdio.h>
            #include <stdlib.h>
            #include "freertos/FreeRTOS.h"
            #include "freertos/task.h"
            #include "freertos/queue.h"
            #include "driver/gpio.h"  


            #define KEY1   0

            static xQueueHandle gpio_evt_queue = NULL;


            void gpio_isr_handler(void* arg) 
            {   
                uint32_t gpio_num = (uint32_t) arg;
                xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
            }

            static void gpio_task_example(void* arg)
            {
                for( ;; )
                {
                    uint32_t io_num;
                    if(xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
                        printf("GPIO[%d] intr, val: %d\n", io_num, gpio_get_level(io_num));
                    }
                }
            }

            void app_main(void)
            {
                gpio_config_t io_conf = {};              //新建配置GPIO pad的gpio_config功能参数的结构体  
                io_conf.pin_bit_mask = (1ULL << KEY1);    //设置GPIO4的掩码为1
                io_conf.mode = GPIO_MODE_INPUT;            //设置GPIO4 为输出模式
                io_conf.pull_up_en = 1;                     //上拉
                io_conf.pull_down_en = 0;                   //不下拉  
                io_conf.intr_type = GPIO_INTR_NEGEDGE;      //下升沿触发中断         
                if (gpio_config(&io_conf) != ESP_OK)       //配置 
                    printf("gpio4_config failed \n");   

                if (gpio_install_isr_service(ESP_INTR_FLAG_LEVEL1) != ESP_OK)       //设置中断服务优先级 
                    printf("gpio_install_isr_service failed \n");  

                if (gpio_isr_handler_add(KEY1, gpio_isr_handler, (void*)KEY1 ) != ESP_OK)       //设置中断处理程序
                    printf("gpio_isr_handler_add failed \n");   

                gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
                xTaskCreate(gpio_task_example, "gpio_task_example", 2048, NULL, 10, NULL);

                while(1)
                {
                    vTaskDelay(500 / portTICK_RATE_MS);
                }
            }

        ```
  2. 定时器
        ```
        //================一次性定时器=====================================
        #include "driver/timer.h"
        esp_timer_handle_t timer_Once_handle;//开始,删除等操作会用到
        void timerOnceCB(void *arg)
        {
            printf("timerOnceHandler\n");
            esp_err_t err = esp_timer_delete(timer_Once_handle);
            if(err == ESP_OK){
                printf("delete timer success\n");
            }else{
                printf("delete timer fail\n");
            }
        }
        void startTimerOnce()
        {
            esp_timer_create_args_t timer_args = {
                .callback=&timerOnceCB,  .arg=NULL,  .name="onceTimer"
            };
            esp_timer_create(&timer_args,&timer_Once_handle);
            esp_timer_start_once(timer_Once_handle,1000 * 1000 * 5);//5秒定时
            printf("start a once timer for 5 second\n");
        }
        //================周期性定时器=====================================
        #include "driver/timer.h"
        esp_timer_handle_t timer_Periodic_handle;//开始,删除等操作会用到
        static uint32_t ledStatus = 0;
        void timerPeriodicCB(void *arg)
        {
            ledStatus = ~ledStatus;
            int64_t tick = esp_timer_get_time();
            printf("timerHandler:tick:%d\n",tick);
            if(ledStatus){
                gpio_set_level(LED2, 1); 
            }else{
                gpio_set_level(LED2, 0);
            }
            
        }
        void startTimerPeriodic()
        {
            esp_timer_create_args_t timer_args = {
                .callback=&timerPeriodicCB,  .arg=NULL,  .name="PeriodicTimer"
            };
            esp_timer_create(&timer_args,&timer_Periodic_handle);
            esp_timer_start_periodic(timer_Periodic_handle,1000 * 1000 * 1);//1秒定时
            printf("start a Periodic timer for 1 second\n");
        }
        void TimerPeriodic()
        {
             esp_timer_stop(timer_Periodic_handle);
        }
        ```
  3. PWM
     1. ledc_fade_func_install()：注册LEDC服务
     2. ledc_timer_config(): ledc_timer_config_t：配置定时器参数
        1. ledc_mode_t是指定高速或者低速运行
        2. ledc_timer_t是指哪个定时器，有四个0到3！
        3. 定时器的位数ledc_timer_bit_t
     3. ledc_set_fade_with_time()
        1. ledc_mode_t mode
        2. ledc_channel_t chn
        3. uint32_t target_duty
        4. int max_fade_time_ms)
     4. ledc_fade_start() 开启渐变
     5. demo
        ```
        #include "driver/ledc.h"

        #define GPIO_LED_R 3
        #define GPIO_LED_Y 18

        #define LEDC_HS_TIMER       LEDC_TIMER_0
        #define LEDC_HS_MODE        0  // LEDC_HIGH_SPEED_MODE
        #define LEDC_HS_CH0_GPIO    GPIO_LED_R
        #define LEDC_HS_CH1_GPIO    GPIO_LED_Y
        #define LEDC_HS_CH0_CHANNEL LEDC_CHANNEL_0
        #define LEDC_HS_CH1_CHANNEL LEDC_CHANNEL_1

        #define LEDC_DUTY           5000       //渐变的变大最终目标占空比
        #define LEDC_FADE_TIME      3000  //渐变的时间

        void pwmInit()
        {
            ledc_timer_config_t ledc_timer = {
                .duty_resolution = LEDC_TIMER_13_BIT,  // resolution of PWM duty
                .freq_hz = LEDC_DUTY,                       // frequency of PWM signal
                .speed_mode = LEDC_HS_MODE,            // timer mode
                .timer_num = LEDC_HS_TIMER             // timer index
            };

            ledc_timer_config(&ledc_timer);

            ledc_channel_config_t ledc_channel[] = {
                {.channel = LEDC_HS_CH0_CHANNEL,
                .duty = 0,
                .gpio_num = LEDC_HS_CH0_GPIO,
                .speed_mode = LEDC_HS_MODE,
                .timer_sel = LEDC_HS_TIMER},
                {.channel = LEDC_HS_CH1_CHANNEL,
                .duty = 0,
                .gpio_num = LEDC_HS_CH1_GPIO,
                .speed_mode = LEDC_HS_MODE,
                .timer_sel = LEDC_HS_TIMER},
            };

            ledc_channel_config(&ledc_channel[0]);
            ledc_channel_config(&ledc_channel[1]);

            ledc_fade_func_install(0);

            while (1) {
                for(int ch=0;ch<2;ch++){
                    printf("ch[%d] duty:%d\n",ch,ledc_get_duty(ledc_channel[ch].speed_mode, ledc_channel[ch].channel));
                    ledc_set_fade_with_time(ledc_channel[ch].speed_mode, ledc_channel[ch].channel,LEDC_DUTY, LEDC_FADE_TIME);
                    ledc_fade_start(ledc_channel[ch].speed_mode, ledc_channel[ch].channel, LEDC_FADE_NO_WAIT);

                    vTaskDelay(LEDC_FADE_TIME / portTICK_PERIOD_MS);
                    vTaskDelay(1000 * 1 / portTICK_PERIOD_MS);

                    printf("ch[%d] duty:%d\n",ledc_get_duty(ledc_channel[ch].speed_mode, ledc_channel[ch].channel));
                    ledc_set_fade_with_time(ledc_channel[ch].speed_mode, ledc_channel[ch].channel,0, LEDC_FADE_TIME);
                    ledc_fade_start(ledc_channel[ch].speed_mode, ledc_channel[ch].channel, LEDC_FADE_NO_WAIT);

                    vTaskDelay(LEDC_FADE_TIME / portTICK_PERIOD_MS);
                }
            }
        }
        ```
  4. nvs操作
     1. ```
        void nvs(void)
        {
            esp_err_t err = nvs_flash_init();
            if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
            ESP_ERROR_CHECK(nvs_flash_erase());
            err = nvs_flash_init();
            }
            ESP_ERROR_CHECK(err);
            
            nvs_handle_t my_handle;
            err = nvs_open("storage", NVS_READWRITE, &my_handle);//storage为命名空间.
            if (err != ESP_OK) {
                printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
            } else {
                printf("Done\n");
                //读数据操作
                printf("Reading restart counter from NVS ... ");
                int32_t restart_counter = 0; // value will default to 0, if not set yet in NVS
                err = nvs_get_i32(my_handle, "restart_counter", &restart_counter);//restart_counter为键名
                //处理读的情况
                switch (err) {
                    case ESP_OK:
                        printf("Done\n");
                        printf("Restart counter = %d\n", restart_counter);
                        break;
                    case ESP_ERR_NVS_NOT_FOUND:
                        printf("The value is not initialized yet!\n");
                        break;
                    default :
                        printf("Error (%s) reading!\n", esp_err_to_name(err));
                }
        
                // 写数据操作
                printf("Updating restart counter in NVS ... ");
                restart_counter++;
                err = nvs_set_i32(my_handle, "restart_counter", restart_counter);
                printf((err != ESP_OK) ? "Failed!\n" : "Done\n");
                printf("Committing updates in NVS ... ");
                //提交并保存表的内容
                err = nvs_commit(my_handle);
                printf((err != ESP_OK) ? "Failed!\n" : "Done\n");
        
                //关闭
                nvs_close(my_handle);
            }

        }
        ```
  5. tcp: [tcp demo]  lwip/lwip/src/include/lwip/sockets.h  
```
    // true为创建服务器并且等待连接，false仅仅只是等待连接；返回值是是否成功接收到一个设备的连接
esp_err_t create_tcp_server_(bool isCreatServer, uint32_t TCP_PORT) {
  if (isCreatServer) {
    INFO("server socket....,port=%d\n", TCP_PORT);
    server_socket = socket(AF_INET, SOCK_STREAM, 0);

    if (server_socket < 0) {
      show_socket_error_reason("create_server", server_socket);
      return ESP_FAIL;
    }
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(TCP_PORT);  //指定的端口号
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(server_socket, (struct sockaddr *)&server_addr,
             sizeof(server_addr)) < 0) {
      show_socket_error_reason("bind_server", server_socket);
      close(server_socket);
      if(connect_socket != 0)
        closesocket(connect_socket);
      restartTcpServerTimer();
      return ESP_FAIL;
    }
  }

  if (listen(server_socket, 5) < 0) {
    show_socket_error_reason("listen_server", server_socket);
    close(server_socket);
    return ESP_FAIL;
  }
  int socklen;
  connect_socket =
      accept(server_socket, (struct sockaddr *)&client_addr, &socklen);
  //判断是否连接成功
  if (connect_socket < 0) {
    show_socket_error_reason("accept_server", connect_socket);
    close(server_socket);
    return ESP_FAIL;
  }

  /*connection established，now can send/recv*/
  INFO("tcp connection established!");
  xTaskCreate(recv_data, "recv_data", 4096, NULL, 3, NULL);
  return ESP_OK;
}


void recv_data(void *pvParameters) {
  int len = 0;
  int g_total_data = 0;
  char databuff[1024];
  while (1) {
    memset(databuff, 0x00, sizeof(databuff));
    len = recv(connect_socket, databuff, sizeof(databuff), 0);
    INFO("recvData: %s\n", databuff);
    send(connect_socket, databuff, sizeof(databuff), 0);
    if (len > 0) {
      g_total_data += len;
      g_rxtx_need_restart = false;
    } else {
      show_socket_error_reason("recv_data", connect_socket);
      if (errno == 128) {  // Socket is not connected
        break;
      }
      g_rxtx_need_restart = true;
    }
  }
  restartTcpServerTimer();
  g_rxtx_need_restart = true;
  vTaskDelete(NULL);
  
}
```     
  6. udp
  7. 按键(button)
```
$ENV{IDF_PATH}/examples/bluetooth/esp_ble_mesh/common_components/button
#include "iot_button.h"

#define BUTTON_ON_OFF           9
#define BUTTON_ACTIVE_LEVEL     0
static uint32_t dev_on_btn_num = BUTTON_ON_OFF;

static void button_tap_cb(void* arg)
{
    // example_ble_mesh_send_sensor_message(send_opcode[press_count++]);
    // press_count = press_count % ARRAY_SIZE(send_opcode);
    ESP_LOGD(TAG,"button_tap_cb:%d",*(uint8_t *)arg);
}

void initButton()
{
    button_handle_t dev_on_off_btn = iot_button_create(BUTTON_ON_OFF, BUTTON_ACTIVE_LEVEL);
    if(dev_on_off_btn)
        iot_button_set_evt_cb(dev_on_off_btn, BUTTON_CB_TAP, button_tap_cb, &dev_on_btn_num);
}
```
  8. LED驱动
```
$ENV{IDF_PATH}/examples/bluetooth/esp_ble_mesh/common_components/light_driver
light_driver_config_t driver_config = {
        .gpio_red        = CONFIG_LIGHT_GPIO_RED,
        .gpio_green      = CONFIG_LIGHT_GPIO_GREEN,
        .gpio_blue       = CONFIG_LIGHT_GPIO_BLUE,
        .gpio_cold       = CONFIG_LIGHT_GPIO_COLD,
        .gpio_warm       = CONFIG_LIGHT_GPIO_WARM,
        .fade_period_ms  = CONFIG_LIGHT_FADE_PERIOD_MS,
        .blink_period_ms = CONFIG_LIGHT_BLINK_PERIOD_MS,
    };

light_driver_init(&driver_config);
    light_driver_set_mode(MODE_HSL);
```
  9. 串口驱动
``` c
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/uart.h"
#include "esp_log.h"

#include "main.h"

static char *TAG = "uart";

//extern void parseUartData(unsigned char len,char *dat);

#define EX_UART_NUM UART_NUM_1
#define EX_UART_PIN_RX  8
#define EX_UART_PIN_TX  7
#define PATTERN_CHR_NUM    (3)         /*!< Set the number of consecutive and identical characters received by receiver which defines a UART pattern*/

#define BUF_SIZE (128)
#define RD_BUF_SIZE (BUF_SIZE)
static QueueHandle_t uart_queue;


void uartSendRaw(char *str,int len)
{
   uart_write_bytes(EX_UART_NUM, (const char*) str, len); 
}

void uartSend(char *str)
{
   uart_write_bytes(EX_UART_NUM, (const char*) str, strlen(str)); 
}

static void uart_event_task(void *pvParameters)
{
    uart_event_t event;
    size_t buffered_size;
    uint8_t* dtmp = (uint8_t*) malloc(RD_BUF_SIZE);
    for(;;) {
        //Waiting for UART event.
        if(xQueueReceive(uart_queue, (void * )&event, (portTickType)portMAX_DELAY)) {
            bzero(dtmp, RD_BUF_SIZE);
            ESP_LOGI(TAG, "uart[%d] event:", EX_UART_NUM);
            switch(event.type) {
                //Event of UART receving data
                /*We'd better handler data event fast, there would be much more data events than
                other types of events. If we take too much time on data event, the queue might
                be full.*/
                case UART_DATA:
                    ESP_LOGI(TAG, "[UART DATA]: %d", event.size);
                    uart_read_bytes(EX_UART_NUM, dtmp, event.size, portMAX_DELAY);
                    // parseUartData(event.size,(char *)dtmp);
                    rx_pack_t *p = malloc(sizeof(rx_pack_t));
                    void *payload = malloc(event.size + 1);

                    INFO("===malloc:%p,%p\n", p, payload);
                    if (p != NULL && payload != NULL) {
                        p->kind = RX_PACK_KIND_UART;
                        p->len = event.size;
                        p->payload = payload;
                        memcpy(p->payload, dtmp, event.size);
                        if (g_rxPackQueue != NULL)
                            xQueueSend(g_rxPackQueue, &p, 20);
                        else
                            INFO("g_rxPackQueue == NULL\n");
                    } else {
                        INFO("====malloc fail====\n");
                    }

                    //uart_write_bytes(EX_UART_NUM, (const char*) dtmp, event.size);
                    break;
                //Event of HW FIFO overflow detected
                case UART_FIFO_OVF:
                    ESP_LOGI(TAG, "hw fifo overflow");
                    // If fifo overflow happened, you should consider adding flow control for your application.
                    // The ISR has already reset the rx FIFO,
                    // As an example, we directly flush the rx buffer here in order to read more data.
                    uart_flush_input(EX_UART_NUM);
                    xQueueReset(uart_queue);
                    break;
                //Event of UART ring buffer full
                case UART_BUFFER_FULL:
                    ESP_LOGI(TAG, "ring buffer full");
                    // If buffer full happened, you should consider encreasing your buffer size
                    // As an example, we directly flush the rx buffer here in order to read more data.
                    uart_flush_input(EX_UART_NUM);
                    xQueueReset(uart_queue);
                    break;
                //Event of UART RX break detected
                case UART_BREAK:
                    ESP_LOGI(TAG, "uart rx break");
                    break;
                //Event of UART parity check error
                case UART_PARITY_ERR:
                    ESP_LOGI(TAG, "uart parity error");
                    break;
                //Event of UART frame error
                case UART_FRAME_ERR:
                    ESP_LOGI(TAG, "uart frame error");
                    break;
                //UART_PATTERN_DET
                case UART_PATTERN_DET:
                    uart_get_buffered_data_len(EX_UART_NUM, &buffered_size);
                    int pos = uart_pattern_pop_pos(EX_UART_NUM);
                    ESP_LOGI(TAG, "[UART PATTERN DETECTED] pos: %d, buffered size: %d", pos, buffered_size);
                    if (pos == -1) {
                        // There used to be a UART_PATTERN_DET event, but the pattern position queue is full so that it can not
                        // record the position. We should set a larger queue size.
                        // As an example, we directly flush the rx buffer here.
                        uart_flush_input(EX_UART_NUM);
                    } else {
                        uart_read_bytes(EX_UART_NUM, dtmp, pos, 100 / portTICK_PERIOD_MS);
                        uint8_t pat[PATTERN_CHR_NUM + 1];
                        memset(pat, 0, sizeof(pat));
                        uart_read_bytes(EX_UART_NUM, pat, PATTERN_CHR_NUM, 100 / portTICK_PERIOD_MS);
                        ESP_LOGI(TAG, "read data: %s", dtmp);
                        ESP_LOGI(TAG, "read pat : %s", pat);
                    }
                    break;
                //Others
                default:
                    ESP_LOGI(TAG, "uart event type: %d", event.type);
                    break;
            }
        }
    }
    free(dtmp);
    dtmp = NULL;
    vTaskDelete(NULL);
}

void uartInit(void)
{
    esp_log_level_set(TAG, ESP_LOG_INFO);

    /* Configure parameters of an UART driver,
     * communication pins and install the driver */
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    //Install UART driver, and get the queue.
    uart_driver_install(EX_UART_NUM, BUF_SIZE * 2, BUF_SIZE * 2, 20, &uart_queue, 0);
    uart_param_config(EX_UART_NUM, &uart_config);

    //Set UART log level
    esp_log_level_set(TAG, ESP_LOG_INFO);
    //Set UART pins (using UART0 default pins ie no changes.)
    //uart_set_pin(EX_UART_NUM, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_set_pin(EX_UART_NUM, EX_UART_PIN_TX, EX_UART_PIN_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    //Set uart pattern detect function.
    uart_enable_pattern_det_baud_intr(EX_UART_NUM, '+', PATTERN_CHR_NUM, 9, 0, 0);
  
    //Reset the pattern queue length to record at most 20 pattern positions.
    uart_pattern_queue_reset(EX_UART_NUM, 20);

    //Create a task to handler UART event from ISR
    xTaskCreate(uart_event_task, "uart_event_task", 2048, NULL, priotity_uartRecvTask, NULL);
}

```

# OTA [https://docs.espressif.com/projects/esp-idf/zh_CN/latest/esp32/api-reference/system/ota.html]
## OTA 数据分区
所有使用 OTA 功能项目，其分区表 必须包含一个 OTA 数据分区（类型为 data，子类型为 ota）
工厂启动设置下，OTA 数据分区中应没有数据（所有字节擦写成 0xFF）。如果分区表中有工厂应用程序，ESP-IDF 软件引导加载程序会启动工厂应用程序。如果分区表中没有工厂应用程序，则启动第一个可用的 OTA 分区（通常是 ota_0）
## 应用程序回滚
1. 应用程序运行正常，esp_ota_mark_app_valid_cancel_rollback() 将正在运行的应用程序状态标记为 ESP_OTA_IMG_VALID，启动此应用程序无限制
2. 应用程序出现严重错误，无法继续工作，esp_ota_mark_app_invalid_rollback_and_reboot() 将正在运行的版本标记为 ESP_OTA_IMG_INVALID 然后复位。引导加载程序不会选取此版本，而是启动此前正常运行的版本
3. 如果 CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE 使能，则无需调用函数便可复位，回滚至之前的应用版本


# BLE 
## 频偏
BLE标准规定载波频率的偏差不要超过150KHz,高速晶体的要求的上限是50ppm
## BLE距离估算 https://www.bluetooth.com/learn-about-bluetooth/key-attributes/range/#estimator
## 地址,BLE设备的地址必须是公共地址或者静态地址
 1. 公共地址（Public Address）,即ieee地址,总长度为6个字节,前24bit,ieee分配,后24bit,公司同部分配
 2. 随机地址（Random Address）.分为静态地址（Static Address）和私有地址（Private Address）
    1. 静态地址:48bit,最高2位是0b11,只能在重新上电之后，才能改变静态地址的内容
    2. 私有地址,用于解决隐私问题,分为不可解析私有地址（Non-resolvable private address) 和 可解析私有地址（Resolvable private address）
       1. 可解析私有地址:48bits，最高的2位有效位是0b10,该地址会周期性变化
       2. 不可解析私有地址:48bits，但是最高的2位有效位是0b00,不能与公共地址相同；该地址会周期性变化；
## ibeacon发送自定义数据[https://www.jianshu.com/p/087823702db4]
```
static uint8_t user_data[3] = {0x11, 0x22, 0x33};
static uint8_t user_data2[3] = {0x66, 0x77, 0x88};

static esp_ble_adv_data_t srsp_data1 = {
    .set_scan_rsp = true,
    .manufacturer_len = sizeof(user_data2),
    .p_manufacturer_data = user_data2,
};

static esp_ble_adv_data_t adv_data1 = {
    .manufacturer_len = sizeof(user_data),
    .p_manufacturer_data = user_data,
    .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};

    esp_ble_gap_set_device_name("hello");
    esp_ble_gap_config_adv_data(&srsp_data1);   //广播自定义数据
    esp_ble_gap_config_adv_data(&adv_data1);
```


## GATT 
### GATT（Generic Attributes Profile）的缩写，是已连接的低功耗蓝牙设备之间进行通信的协议
一旦两个设备建立起了连接，GATT 就开始起作用了，这也意味着，你必需完成前面的GAP协议。

GATT使用了 ATT（Attribute Protocol）协议，ATT 协议把 Service，Characteristic 对应的数据保存在一个查找表中，查找表使用 16bit ID 作为每一项的索引。

GATT定义的多层数据结构简要概括起来就是 服务（Service） 可以包含多个 特征（Characteristic），每个特征包含 属性（Properties） 和 值（Value），还可以包含多个 描述（Descriptor）。

### 一个GATT 服务器应用程序架构(由Application Profiles组织起来)如下
┌─────────────────┐         ┌───────────────┐
│ Application     |         │ Client for    |
| Profile A       |---------│ Application   |
|-----------------|         │ Profile A     |
|Server 1         |         └───────────────┘
|-Characteristic 1|
|-Characteristic 2|
┠─────────────────┨
| Application     |         ┌───────────────┐
| Profile B       |         │ Client for    |
|-----------------|---------│ Application   |
|Server 1         |         │ Profile B     |
|-Characteristic 1|         └───────────────┘
|-Characteristic 2|
└─────────────────┘
Application Profile存储在数组中，并分配相应的回调函数gatts_profile_a_event_handler() 和 gatts_profile_b_event_handler()。
结构体如下:
struct gatts_profile_inst {
    esp_gatts_cb_t gatts_cb; //Profile回调函数
    uint16_t gatts_if;       //在GATT客户机上的不同的应用程序使用不同的接口，用gatts_if参数来表示,初始化为 ESP_GATT_IF_NONE
                             //在注册客户端时(如注册profile A的客户端时gatt_if = 3,在注册profile B的客户端时gatt_if = 4)
    uint16_t app_id;         //Application ID是由应用程序分配的用来标识每个Profile。 
                             //通过这种方法，可以在一个Server中run多个Application Profile
                             //esp_ble_gatts_app_register(PROFILE_A_APP_ID);
    uint16_t conn_id;
    uint16_t service_handle;
    esp_gatt_srvc_id_t service_id;
    uint16_t char_handle;
    esp_bt_uuid_t char_uuid;
    esp_gatt_perm_t perm;
    esp_gatt_char_prop_t property;
    uint16_t descr_handle;
    esp_bt_uuid_t descr_uuid;
};

esp_ble_gatts_register_callback(esp_gatts_cb_t callback);向BTA GATTS模块注册应用程序回调
callback回调函数处理从BLE堆栈推送到应用程序的所有事件,参数如下:
event:事件类型(蓝牙的状态),定义在esp_gatts_api.h
gatts_if(uint8_t):GATT访问接口类型,不同的Application Profilec对应不同的gatts_if,如示例中的3和4
param: esp_ble_gatts_cb_param_t 指向回调函数的参数，是个联合体类型，不同的事件类型采用联合体内不同的成员结构体

蓝牙状态转变过程(gatts_server)
- 建立连接之前
    ESP_GATTS_REG_EVT---->ESP_GATTS_CREATE_EVT---->ESP_GATTS_START_EVT---->ESP_GATTS_ADD_CHAR_EVT--->ESP_GATTS_ADD_CHAR_DESCR_EVT
    注册->创建->启动->添加特征->添加特征描述
- Client开始连接之后
    CONNECT_EVT---->ESP_GATTS_MTU_EVT--->GATT_WRITE_EVT--->ESP_GATTS_CONF_EVT-->GATT_READ_EVT


                                                    

### 流程
 每个profile包括GATT interface(GATT 接口)、Application ID(应用程序ID)、 Connection ID(连接ID)、Service Handle(服务句柄)、Service ID(服务ID)、Characteristic handle(特征句柄)、Characteristic UUID(特征UUID)、ATT权限、Characteristic Properties、描述符句柄、描述符UUID
   如果Characteristic 支持通知(notifications)或指示(indicatons)，它就必须是实现CCCD(Client Characteristic  Configuration Descriptor)----这是额外的ATT。描述符有一个句柄和UUID。
### 链接:[https://www.cnblogs.com/smart-mutouren/p/5937990.html]
        [https://blog.csdn.net/zhejfl/article/details/85136102]
### 名词:characteristic(服务属性)
### server
1. Service Definition
    服务定义中的顺序为:Service Declaration ~ Include Definitions(>=0) ~ Characteristic Definitions(>=0)
    Service Declaration如下
    Attribute Handle	Attribute Type	                        Attribute Value	            Attribute Permission
    0xNNNN	            0x2800 – UUID for <Primary Service>     16-bit Bluetooth UUID       Read Only,
                        0x2801 – UUID for <Secondary Service>	128-bit UUID for Service    No Authentication,No Authorization
    规则如下:
    - 当多个服务存在时
        使用16-bit Bluetooth UUID服务定义的服务应该分组(如按顺序排列)
        同理，使用128-bit UUID的服务定义的服务也分组
    - 一个设备或高层协议可能有多个服务定义，同时多个服务定义含有相同的Service UUID
    - 服务端的所有Attributes应该包含一个服务声明或存在一个服务定义
    - 服务端的服务定义可能无序;Client不应该认为服务端的服务定义一定是有序的
2. Include Definition
    一个Include Definition只包含一个Include Declaration
    Include Declaration如下
    Attribute Handle	Attribute Type	    Attribute Value	                                Attribute Permission
    0xNNNN	            0x2802 – UUID       Included Service    End Group   Service UUID    Read Only,
                        for<Include>	    Attribute Handle    Handle                      No Authentication,No Authorization
    其中，仅当UUID是16-bit Bluetooth UUID时才存在
    如果一个Service的Include Definition(A)是引用其他Server的Include Definition(B)
    那么Include Definition(B)不应该引用Include Definition(A)，否则就是循环引用(Circular Reference)
    当一个Client检测到循环引用或detects nested include declarations to a greater level than it expects
    Client应当终止本次通信(ATT Bearer)
3.  Characteristic Definition
    Characteristic Definition包含如下内容
    - Characteristic Declaration                       : First
    - Characteristic Value declaration                 : Second
    - Characteristic Descriptor Declarations(Optional) : Last(含多个时顺序不关紧要)               
    
    Characteristic Definitions在服务端以Attribute Handle排序

以上每个Declaration包含在一个单独的Attribute中
  1. 1 Characteristic Declaration
    Characteristic Declaration如下
    Attribute Handle	Attribute Type	Attribute Value	                                        Attribute Permission
    0xNNNN	            0x2803          Characteristic  Characteristic Value    Characteristic  Read Only,
                        – UUID for      Properties      Attribute Handle        UUID            No Authentication,
                        Characteristic	                                                        No Authorization
  2. Characteristic Value Declaration
    Characteristic Value Declaration如下
    Attribute Handle	Attribute Type	                       Attribute Value	            Attribute Permissions
    0xNNNN	            0xuuuu – 16-bit Bluetooth UUID         Characteristic Value         Higher layer profile
                        or                                                                  or
                        128-bit UUID for Characteristic UUID                                implementation specific
  3. Characteristic Descriptor Declarations
    Characteristic Descriptor Declarations包含了Characteristic Value相关信息

    GATT定义了一系列的标准Characteristic Descriptors供高层协议使用
    高层协议也可以定义协议相关的Characteristic Descriptors

    Characteristic Descriptors在服务端上是无序的，Client不应该理所当然
    Characteristic Descriptors Declarations Permissions由高层协议定义或协议相关的
    Client不应该理所当然地认为是可读的

    Characteristic Descriptor Declarations包括如下内容(详细见规范)

    - Characteristic Extended Properties
    - Characteristic User Description
    - Client Characteristic Configuration
    - Server Characteristic Configuration
    - Characteristic Presentation Format
    - Characteristic Aggregate Format
 
  4. Summary of GATT Profile Attribute Types
    Attribute Type	                        UUID	    Description
    <Primary Service>	                    0x2800	    Primary Service Declaration
    <Secondary Service>	                    0x2801	    Secondary Service Declaration
    <Include>	                            0x2802	    Include Declaration
    <Characteristic>	                    0x2803	    Characteristic Declaration
    <Characteristic Extended Properties>    0x2900	    Characteristic Extended Properties
    <Characteristic User Description>	    0x2901	    Characteristic User Description Descriptor
    <Client Characteristic Configuration>	0x2902	    Client Characteristic Configuration Descriptor
    <Server Characteristic Configuration>	0x2903	    Server Characteristic Configuration Descriptor
    <Characteristic Format>	                0x2904	    Characteristic Format Descriptor
    <Characteristic Aggregate Format>	    0x2905	    Characteristic Aggregate Format Descriptor
	
4. 	GATT Feature Requirements
    GATT中定义了11项Feature,每个Feature都有对应的过程和子过程，这些过程描述了如何使用ATT来实现各自的功能
    序号 Feature                                Procedure
    1  Server Configuration                     Exchange MTU,连接期间只能配置一次，确定连接通信的ATT_MTU大小；否则就用默认的23-3 = 20
    2  Primary Service Discovery                Discover All Primary Services/Discover Primary Services By Service UUID 
       (主服务查找)                              把主服务都找出来，
    3  Relationship Discovery                   查找Included Services
    4  Characteristic Discovery                 Discover All Characteristic of a Service./Discover Characteristic By UUID
        (特征查找)                               这里找到的是Characteristic declaration的ATT Handle 和ATT Value。
                                                这个ATT value包括Characteristic Properties, 特征值的句柄和Characteristic UUID。特征值的句柄在读写特征值时要用到。
    5  Characteristic Descriptor Discovery
    6  Reading a Characteristic Value
    7  Writing a Characteristic Value
    8  Notification of a Characteristic Value
    9  Indication of a Characteristic Value
    10  Reading a Characteristic Descriptor
    11 Writing a Characteristic Descriptor
5. 流程
    1）配置交换(exchanging configuration)
    2）发现一个设备上的服务s和特征s
    3）读取一个特征值(characteristic value)
    4）写入一个特征值
    5）通知一个特征值
    6）指示一个特征值 


