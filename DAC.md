
# DAC:Digital To Analog Converter
 1. Overview
    1. ESP32有两个8位DAC（数模转换器）通道，连接到GPIO25（通道1）和GPIO26（通道2）。
    2. DAC驱动器允许将这些通道设置为任意电压
    3. 也可以通过I2S驱动程序使用DMA模式写入数据到DAC通道,当使用“built-in DAC mode”时
 2. DAC输出电压:
    1. 由于 DAC 输出为 8 位.所以取值为 0-255 正比对应于电源电压（如0 - 3.3V）。即：
        Vout = N / 255 * Vdd
       其中，N为 dac_value 的值，Vdd为电源电压(通常为3.3V)。
       计算得的Vout为输出的DAC电压。
    2. 当 N = 155时，有
         Vout = 155 / 255 *Vdd = 155 / 255 × 3.3 V ≈ 2.00588 V
 3. 编程步骤
    1. 头文件 <driver/dac.h>
    2. 使能dac:dac_output_enable(DAC_CHANNEL_1);
    3. 输出电压:dac_output_voltage(DAC_CHANNEL_1, 155);
    
 4. DAC输出余弦波
    ```c
    dac_cw_config_t config;
    config.en_ch = DAC_CHANNEL_1;
    config.freq = 55000;
    config.scale = DAC_CW_SCALE_1;
    config.phase = DAC_CW_PHASE_0;

	dac_cw_generator_config(&config);
	dac_cw_generator_enable();

    dac_output_enable(DAC_CHANNEL_1);

    ```
    