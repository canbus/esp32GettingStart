7. UART通信
    1. UART概览：(https://docs.espressif.com/projects/esp-idf/zh_CN/latest/esp32/api-reference/peripherals/uart.html)
    2. 简介
       * ESP32芯片有三个UART控制器(UART0、UART1和UART2)，具有一组功能相同的寄存器，便于编程和灵活性。每个UART控制器可独立配置波特率、数据位长、位序、停止位数、奇偶校验位等参数。所有的控制器都兼容 UART 支持的设备从各种制造商，也可以支持红外数据关联协议(IrDA)
    3. UART使用步骤
        1. 设置通信参数 - 设置波特率、数据位、停止位等
           1. 一次性配置所有参数
                使用uart_config_t结构体可以统一设置
                ```c
                    typedef struct {
                        int 				  baud_rate;      //波特率
                        uart_word_length_t    data_bits;      /*!< UART byte size*/
                        uart_parity_t 		  parity;         //奇偶校验方式
                        uart_stop_bits_t 	  stop_bits;      //停止位数
                        uart_hw_flowcontrol_t flow_ctrl;    //硬件流控方式
                        uint8_t 			  rx_flow_ctrl_thresh;        //硬件流控阈值
                        uart_sclk_t 	    source_clk;     //时钟源
                    } uart_config_t;
                ```
                ```c
                    uart_config_t uartConfig = {
                        .baud_rate   = 115200,
                        .data_bits   = UART_DATA_8_BITS,
                        .flow_ctrl   = UART_HW_FLOWCTRL_DISABLE,
                        .parity 	 = UART_PARITY_DISABLE,
                        .stop_bits   = UART_STOP_BITS_1,
                        .source_clk  = UART_SCLK_APB,
                    };
                    uart_param_config(UART_NUM_2, &uartConfig);
                ```
           2. 分步依次配置每个参数
                调用下表中的专用函数，能够单独配置特定参数。如需重新配置某个参数，也可使用这些函数
                    配置参数        设置函数                    获取函数
                    波特率          uart_set_baudrate()     uart_get_baudrate()
                    传输位          uart_set_word_length()  uart_get_word_length()
                    奇偶控制        uart_set_parity()       uart_get_parity()
                    停止位          uart_set_stop_bits()    uart_get_stop_bits() 
                    硬件流控模式    uart_set_hw_flow_ctrl()  uart_get_hw_flow_ctrl()
                    通信模式        uart_set_mode()          uart_get_mode()
        2. 设置通信管脚
            通信参数设置完成后，可以配置其他 UART 设备连接的 GPIO 管脚。调用函数 uart_set_pin()，指定配置 Tx、Rx、RTS 和 CTS 信号的 GPIO 管脚编号
            ```c
            // Set UART pins(TX: IO4, RX: IO5, RTS: IO18, CTS: IO19)
            ESP_ERROR_CHECK(uart_set_pin(UART_NUM_2, 4, 5, 18, 19));
            ```
        3. 安装驱动程序
           通信管脚设置完成后，请调用 uart_driver_install() 安装驱动程序并指定以下参数：
            1. Tx 环形缓冲区的大小
            2. Rx 环形缓冲区的大小
            3. 事件队列句柄和大小
            4. 分配中断的标志
            该函数将为 UART 驱动程序分配所需的内部资源。
            ```c
            // Setup UART buffered IO with event queue
            const int uart_buffer_size = (1024 * 2);
            QueueHandle_t uart_queue;
            // Install UART driver using an event queue here
            //uart_driver_install(uart_port_t uart_num, int rx_buffer_size, int tx_buffer_size, 
            //                    int event_queue_size, QueueHandle_t *uart_queue, int intr_alloc_flags)
            ESP_ERROR_CHECK(uart_driver_install(UART_NUM_2, uart_buffer_size, \
                                                    uart_buffer_size, 10, &uart_queue, 0));
            ```
        4. 发数据
            使用uart write bytes()往Tx FIFO buffer里面写数据，就可以发送数据
        5. 收数据
           1. 方式一:轮询(uart_read_bytes())
              * 使用uart_read_bytes()从Rx FIFO buffer里读数据，就是接收数据
              * ```c
                    #include "driver/uart.h"
                    void uartTest()
                    {
                        #define BUF_SIZE (1024)

                        uart_config_t uartConfig = {
                            .baud_rate   = 115200,
                            .data_bits   = UART_DATA_8_BITS,
                            .flow_ctrl   = UART_HW_FLOWCTRL_DISABLE,
                            .parity 	 = UART_PARITY_DISABLE,
                            .stop_bits   = UART_STOP_BITS_1,
                            .source_clk  = UART_SCLK_APB,
                        };
                        uart_param_config(UART_NUM_2, &uartConfig);
                        uart_driver_install(UART_NUM_2, BUF_SIZE * 2, 0, 0, NULL, 0);
                        uart_set_pin(UART_NUM_2,18, 19,  0, 0);

                        uint8_t *data = (uint8_t *) malloc(BUF_SIZE);
                        while (1) {
                            // Read data from the UART
                            int len = uart_read_bytes(UART_NUM_2, data, BUF_SIZE, 20 / portTICK_RATE_MS);
                            if(len > 0)
                            printf("rx:%s\n",data);
                            // Write data back to the UART
                            uart_write_bytes(UART_NUM_2, (const char *) data, len);
                            vTaskDelay(pdMS_TO_TICKS(1000));
                        }
                    }
               ```
           2. 方式二:队列信号(xQueueReceive()UART_DATA事件->uart_read_bytes())
              * 当安装驱动的时候uart_driver_install()函数有一个参数*uart_queue和queue_size。该函数会利用这两个参数创建一个UART 事件的队列。此队列即 FreeRTOS 的 Queue。
              * 该队列使用一个uart_event_t类型的结构体,这个结构体包含了事件类型和UART_DATA事件携带的数据
              ```c
                typedef struct {
                    uart_event_type_t	type; // 事件类型,主要是UART_DATA事件
                    size_t 				size; //UART 数据长度
                    ...
                } uart_event_t;
              ```
              * 步骤: 用xQueueReceive()接收Queue中的UART_DATA事件然后调用uart_read_bytes()读取数据
              * ```c
                    #include "driver/uart.h"
                    QueueHandle_t uart_queue;    
                    void uartInit()
                    {
                        #define BUF_SIZE (100)

                        uart_config_t uartConfig = {
                            .baud_rate   = 115200,
                            .data_bits   = UART_DATA_8_BITS,
                            .flow_ctrl   = UART_HW_FLOWCTRL_DISABLE,
                            .parity 	 = UART_PARITY_DISABLE,
                            .stop_bits   = UART_STOP_BITS_1,
                            .source_clk  = UART_SCLK_APB,
                        };
                        uart_param_config(UART_NUM_2, &uartConfig);
                        uart_driver_install(UART_NUM_2, BUF_SIZE * 2, 0, 10, &uart_queue, 0);
                        uart_set_pin(UART_NUM_2,18, 19,  0, 0);

                        uint8_t *data = (uint8_t *) malloc(BUF_SIZE);
                        uart_write_bytes(UART_NUM_2, (const char *) "hello", 5);
                        while (1) {
                            uart_event_t event;
                            if(xQueueReceive(uart_queue,&event,portMAX_DELAY)){
                                switch (event.type)
                                {
                                case UART_DATA:
                                    printf("rx:%d\n",event.size);
                                    int len = uart_read_bytes(UART_NUM_2, data, BUF_SIZE, 20 / portTICK_RATE_MS);
                                    printf("rx:%d,%s\n",len,data);
                                    break;
                                
                                default:
                                    break;
                                }
                            }
                        }
                    }
              ```
 