# ESP_LOG日志
## printf是不可重入函数
    printf不能在中断中被调用的原因是它是一个不可重入函数，而在中断中要避免调用不可重入函数，首先我们先说说什么是不可重入函数。

    简单说来，区分一个函数是否可重入就是看这个函数能否在未返回的时候再次被调用。而造成一个函数不可重入的原因往往是使用了全局变量，如果一个函数未返回再执行一次会导致对全局变量的操作是不安全的。就例如我们常用的printf、malloc、free都是不可重入的函数，printf会引用全局变量stdout，malloc，free会引用全局的内存分配表，在多线程的环境下，如果没有很好的处理数据保护和互斥访问，就会发生错误。
## ESP_LOG日志
  1. 日志记录库提供了两种设置日志详细程度的方法
     + 在编译时：在menuconfig中，使用选项设置详细程度CONFIG_LOG_DEFAULT_LEVEL。详细程度高于的所有日志记录语句CONFIG_LOG_DEFAULT_LEVEL 将被预处理器删除。
     + 在运行时：详细级别低于的所有日志CONFIG_LOG_DEFAULT_LEVEL默认情况下启用。该功能esp_log_level_set()可用于按模块设置日志记录级别。模块由其标签标识，这些标签是可读的ASCII零终止字符串。
     + esp_log_level_set()无法将日志记录级别设置为高于CONFIG_LOG_DEFAULT_LEVEL。要在编译时增加特定文件的日志级别，请使用宏LOG_LOCAL_LEVEL
  2. 日志级别
     + ESP_LOGE -错误（最低）
     + ESP_LOGW - 警告
     + ESP_LOGI -信息
     + ESP_LOGD -调试
     + ESP_LOGV -详细（最高）
  3. 使用
     1. #include "esp_log.h"
     2. 定义TAG变量: #define TAG "mylog"
     3. 输出日志:    ESP_LOGI(TAG, "Total APs scanned = %u", ap_count);
  4. 简化ESP_LOG的用法
     + `#define INFO(format, ...) ESP_LOGI("INFO", format, ##__VA_ARGS__) //printf(format,##__VA_ARGS__) `
