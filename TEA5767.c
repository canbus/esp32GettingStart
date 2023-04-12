#include <driver/gpio.h>
#include <esp_system.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <nvs_flash.h>
#include <soc/rmt_struct.h>
#include <stdio.h>
#include "driver/i2c.h"

#define SCL_IO GPIO_NUM_8
#define SDA_IO GPIO_NUM_7
#define I2C_NUM I2C_NUM_0
#define I2C_FREQ_HZ 100000
#define I2C_TX_BUF_DISABLE 0
#define I2C_RX_BUF_DISABLE 0

#define WRITE_BIT I2C_MASTER_WRITE              /*!< I2C master write */
#define READ_BIT I2C_MASTER_READ                /*!< I2C master read */
#define ACK_CHECK_EN 0x1                        /*!< I2C master will check ack from slave*/
#define ACK_CHECK_DIS 0x0                       /*!< I2C master will not check ack from slave */
#define ACK_VAL 0x0                             /*!< I2C ack value */
#define NACK_VAL 0x1                            /*!< I2C nack value */

//#define SLAVE_ADDR   0x23 // 从机地址
#define SLAVE_ADDR   0xC0 >> 1 // TEA5767

#define TEA5767_MAX_KHZ     108000  // 最高频率 108M
#define TEA5767_MIN_KHZ     87500   // 最低频率 87.5M

uint32_t g_frequency = TEA5767_MIN_KHZ ;
static uint8_t s_radioWriteData[5] = {0x31,0xA0,0x20,0x11,0x00};          // 要写入TEA5767的数据，默认存台的PLL,104.3MHz
static uint8_t s_radioReadData[5] = {0};                                  // TEA5767读出的状态
static uint32_t s_pll = 0;


int I2C_Master_Transmit(uint8_t slaveAddr, uint8_t regAddr, uint8_t *pData, uint16_t dataLen)
{
    int ret;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (slaveAddr << 1) | WRITE_BIT, ACK_CHECK_EN);
    if(NULL != regAddr)
    {
        i2c_master_write_byte(cmd, regAddr, ACK_CHECK_EN);
    }
    i2c_master_write(cmd, pData, dataLen, ACK_CHECK_EN);
    i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(I2C_NUM, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
}

int I2C_Master_Receive(uint8_t slaveAddr, uint8_t regAddr, uint8_t *pData, uint16_t dataLen)
{
    int ret;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (slaveAddr << 1) | READ_BIT, ACK_CHECK_EN);
    if(NULL != regAddr)
    {
        i2c_master_write_byte(cmd, regAddr, ACK_CHECK_EN);
    }
    i2c_master_read(cmd, pData, dataLen, ACK_VAL);
    i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(I2C_NUM, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
}


//由频率计算PLL
void TEA5767_GetPLL(void)
{
    uint8_t hlsi;
    hlsi = s_radioWriteData[2] & 0x10;                                  // HLSI位
    if(hlsi)
    {
        s_pll = (uint32_t)((float)((g_frequency+225)*4)/(float)32.768);   // 频率单位:k
    }
    else
    {
        s_pll = (uint32_t)((float)((g_frequency-225)*4)/(float)32.768);   // 频率单位:k
    }
}

/**
 @brief 读TEA5767状态
 @param 无
 @return 无
*/
void TEA5767_Read(void)
{
    uint8_t i;
    uint8_t tempLow;
    uint8_t tempHigh;
    s_pll = 0;
    
    I2C_Master_Receive(SLAVE_ADDR, NULL , &s_radioReadData, 5);
    tempLow = s_radioReadData[1];                                       // 得到s_pll低8位 
    tempHigh = s_radioReadData[0];                                      // 得到s_pll高6位
    tempHigh &= 0x3f;
    s_pll = tempHigh * 256 + tempLow;                                   // PLL值 
}
/**
 @brief 手动搜索电台，不用考虑TEA5767用于搜台的相关位:SM,SUD
 @param mode -[in] 搜索方式，mode=1，向上搜索，频率值+0.1MHz；mode=0，向下搜索，频率值-0.1MHz
 @return 无
*/
void TEA5767_Search(uint8_t mode)
{
    TEA5767_Read();                                                     // 读取当前频率值 
    if(mode)                                                            // 向上搜索
    {
        g_frequency += 100;
        if(g_frequency > TEA5767_MAX_KHZ)                               // 频率达到最大值
        {
            g_frequency = TEA5767_MIN_KHZ;
        }
    }
    else
    {
        g_frequency -= 100;
        if(g_frequency < TEA5767_MIN_KHZ)
        {
            g_frequency = TEA5767_MAX_KHZ;
        }
    }          
    TEA5767_GetPLL();                                                   // 计算PLL值
    s_radioWriteData[0] = s_pll / 256;
    s_radioWriteData[1] = s_pll % 256;
    s_radioWriteData[2] = 0x20;
    s_radioWriteData[3] = 0x11;
    s_radioWriteData[4] = 0x00;
    I2C_Master_Transmit(SLAVE_ADDR, NULL, &s_radioWriteData, 5);
    TEA5767_Read();
    if(s_radioReadData[0] & 0x80)										// 搜台成功，RF=1，可保存其频率值待用
    {
        printf(" frequency=%d\n", (int)g_frequency);
    } else {
        printf("s_radioReadData[0]:%x\n ",s_radioReadData[0]);
    }
}

void autoSearch(uint8_t mode)
{
    TEA5767_Read(); 
    if(mode){
        s_radioWriteData[2] = 0xa0;
    }else{
        s_radioWriteData[2] = 0x20;
    }          
    TEA5767_GetPLL();                                                   // 计算PLL值
    s_radioWriteData[0] = s_pll / 256 + 0x40;
    s_radioWriteData[1] = s_pll % 256;
    //s_radioWriteData[2] = 0x20;
    s_radioWriteData[3] = 0x11;
    s_radioWriteData[4] = 0x00;
    I2C_Master_Transmit(SLAVE_ADDR, NULL, &s_radioWriteData, 5);
    TEA5767_Read();
    if(s_radioReadData[0] & 0x80)										// 搜台成功，RF=1，可保存其频率值待用
    {
        printf(" frequency=%d\n", (int)g_frequency);
    } else {
        printf("s_radioReadData[0]:%x\n ",s_radioReadData[0]);
    }   
}

void TEA5767_Mute(uint8_t mode)
{		
    if(mode == 1)
    {
        s_radioWriteData[0] = s_radioWriteData[0] | 0x80;//静音
    }
    else
    {
        s_radioWriteData[0] = s_radioWriteData[0] & 0x7F;
    }
    
    I2C_Master_Transmit(SLAVE_ADDR, NULL, &s_radioWriteData, 5);
}


void TEA5767_SetFrequency(uint32_t frequency) //单位:KHz
{
    g_frequency = frequency;
    TEA5767_GetPLL();
    s_radioWriteData[0] = s_pll / 256;
    s_radioWriteData[1] = s_pll % 256;
    s_radioWriteData[2] = 0x20;
    s_radioWriteData[3] = 0x11;
    s_radioWriteData[4] = 0x00;
    
    I2C_Master_Transmit(SLAVE_ADDR, NULL, &s_radioWriteData, 5);
}

int I2C_Init(void)
{
    int i2c_master_port = I2C_NUM;
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = SDA_IO, // select GPIO specific to your project
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = SCL_IO, // select GPIO specific to your project
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_FREQ_HZ, // select frequency specific to your project
        // .clk_flags = 0,          /*!< Optional, you can use I2C_SCLK_SRC_FLAG_* flags to choose i2c source clock here. */
    };
    i2c_param_config(i2c_master_port, &conf);
    return i2c_driver_install(i2c_master_port, conf.mode, I2C_RX_BUF_DISABLE, I2C_TX_BUF_DISABLE, 0);
}



void Init_TEA5767(void)
{
    uint8_t data;
    I2C_Init();
    TEA5767_SetFrequency(98700);
 
    // data = 0x01;
    // I2C_Master_Transmit(SLAVE_ADDR, NULL, &data, 1);
}

void app_main(void)
{
    Init_TEA5767();
    printf("Init_BH1750\n");
}