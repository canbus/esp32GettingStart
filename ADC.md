
# ADC
 1. Analog to Digital Converter(esp32)
    1. 概述(https://docs.espressif.com/projects/esp-idf/zh_CN/v4.3.2/esp32/api-reference/peripherals/adc.html)
       * ESP32 集成了 2 个 12-bit SAR ADC，共支持 18 个模拟通道输入
            * 2个模数转换器指的是 ADC1 和 ADC2，12 bit 即为 ADC 的最高精度 2^12 = 4096。精度可配置 12 位、11 位、10 位、9 位多种分辨率，参见 《ESP32 技术参考手册》 的第 583 页
    2. ADC 精度和通道:18个通道,最高精度:12位,4096 
        * ADC1：8 个通道，GPIO32~GPIO39
        * ADC2：10 个通道，GPIO0、GPIO2、GPIO4、GPIO12-GPIO15、GPIO25-GPIO27.(Wi-Fi驱动程序使用 ADC2)
        * ADC1 通道定义
         ```c
            typedef enum {
                ADC1_CHANNEL_0 = 0, /*!< ADC1 channel 0 is GPIO36 */
                ADC1_CHANNEL_1,     /*!< ADC1 channel 1 is GPIO37 */
                ADC1_CHANNEL_2,     /*!< ADC1 channel 2 is GPIO38 */
                ADC1_CHANNEL_3,     /*!< ADC1 channel 3 is GPIO39 */
                ADC1_CHANNEL_4,     /*!< ADC1 channel 4 is GPIO32 */
                ADC1_CHANNEL_5,     /*!< ADC1 channel 5 is GPIO33 */
                ADC1_CHANNEL_6,     /*!< ADC1 channel 6 is GPIO34 */
                ADC1_CHANNEL_7,     /*!< ADC1 channel 7 is GPIO35 */
                ADC1_CHANNEL_MAX,
            } adc1_channel_t;
         ```
    3. 注意事项:
       1. 一些ADC2引脚用于Strapping引脚（GPIO 0、2、15）,因此无法自由使用
       2. 由于Wi-Fi也使用ADC2模块，因此在一起使用时，只有其中一个模块可以获得优先权，这意味着ADC2_get_raw（）可能会被堵塞，直到Wi-Fi停止，反之亦然
    4. ADC 工作模式:
       1. 每个ADC单元支持两种工作模式，即ADC单读模式和ADC连续（DMA）模式。ADC单读模式适用于低频采样操作。ADC连续（DMA）读取模式适用于高频连续采样动作
            * ADC 单次读取模式：适用于低频采样，采样频率要求 1 kHz
            * ADC 连续（DMA）模式：适用于高频连续采样，以 mKz 为单位的频率
    5. ADC 衰减配置
         ```c
            typedef enum {
                ADC_ATTEN_DB_0   = 0,  /*!<No input attenumation, ADC can measure up to approx. 800 mV. */
                ADC_ATTEN_DB_2_5 = 1,  /*!< extending the range of measurement to up to approx. 1100 mV. */
                ADC_ATTEN_DB_6   = 2,  /*!< extending the range of measurement to up to  approx. 1350 mV. */
                ADC_ATTEN_DB_11  = 3,  /*!< extending the range of measurement to up to  approx. 2600 mV. */
                ADC_ATTEN_MAX,
            } adc_atten_t
         ```
    6. 单次读取模式
       1. 配置衰减及ADC位数
          1. 对于ADC1,用以下二个函数配置:
             adc1_config_width()   配置ADC位数
             adc1_config_channel_atten() 配置ADC的衰减
          2. 对于ADC2,用adc2_config_channel_atten() 配置衰减,数据宽度在每次读取的时候再配置
       2. 读取:adc1_get_raw()及adc2_get_raw()读取.对于ADC2,读取时还要设置ADC的位数 
       3. ADC编程步骤
          1. #include <driver/adc.h>
          2. adc1_config_width(ADC_WIDTH_BIT_12); //配置所需的精度
          3. adc1_config_channel_atten(ADC_CHANNEL_6, ADC_ATTEN_DB_0); //配置所需的衰减
          4. adc1_get_raw(ADC_CHANNEL_6);  //读取adc
       4. 示例
             ```c
               adc1_config_width(ADC_WIDTH_BIT_12);
               adc1_config_channel_atten(ADC_CHANNEL_6, ADC_ATTEN_DB_11);//GPIO34

               while(1){
                   int rawVal = adc1_get_raw(ADC_CHANNEL_6);
                   printf("%d ",rawVal);
               }
             ``` 
       5. 实际电压的计算
               * 对于实际电压的计算，有如下计算公式：
                   Vout = Dout * Vmax / Dmax 
       6. 练习:实现同时从从GPIO0,GPIO32,GPIO35上读取电压值.如下:
           ```
               GPIO0: raw:0 Vol:0.00V
               GPIO32: raw:0 Vol:0.00V
               GPIO35: raw:0 Vol:0.00V
              
               GPIO0: raw:571 Vol:0.36V
               GPIO32: raw:571 Vol:0.36V
               GPIO35: raw:0 Vol:0.00V

               GPIO0: raw:511 Vol:2.59V
               GPIO32: raw:4095 Vol:2.60V
               GPIO35: raw:4095 Vol:2.60V
           ```
    7. 噪声影响
       ESP32 ADC对噪声非常敏感,会导致ADC的读数差异比较大.为了最小化噪声，可以将0.1µF电容器连接到使用中的ADC输入焊盘。多次采样也会进一步减轻噪声的影响
    8. ADC校准
    9. 霍尔传感器读取
       1. 简介
        霍尔效应传感器是一种磁场传感器，当在磁场下施加时，其输出电压的变化做出响应。
        霍尔效应是导体中的电流发生时发生的情况其路径因磁场的作用而偏离。通过适当的格式，此偏差会导致产生霍尔电压，并且可以通过外部电路加以利用，这就是霍尔传感器的作用。霍尔电压可以由外部电路测量或用于感应目的，因为它与它产生的电场强度成正比。
       2. 使用函数hall_sensor_read()读取霍尔传感器。读取到的值是 12 位宽 (范围0-4095)。因此 霍尔传感器使用前（即调用hall_sensor_read()之前）必须通过调用adc1_config_width()来将ADC1配置位12位宽。
         注意：霍尔传感器采用 ADC1 的通道 0 和 3。不要将这些通道配置为ADC通道。同样，也不要将其他任何东西连接到这两个通道对应的 GPIO 上（GPIO36 和 GPIO39），也不要对这两个IO口做任何配置。否则可能会影响传感器对低值信号的测量
       3. 练习:当磁场达到一定强度时,打开LED,否则LED指示灯将熄灭
    10. 虚拟示波器的实现:https://pan.baidu.com/s/1XzLB602CVKOSQgRMTRrUXw?pwd=27hu 
    11. 连续读取模式(esp32的连续模式只能用要i2s中.)
 