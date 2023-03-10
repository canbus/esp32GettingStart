# cJson
   1. JSON简介 
      1. JSON (JavaScript Object Notation, JS 对象简谱) 是一种轻量级的数据交换格式。它基于 ECMAScript (欧洲计算机协会制定的js规范)的一个子集，采用完全独立于编程语言的文本格式来存储和表示数据。
      2. JSON 语法规则
         在 JS 语言中，一切都是对象。 因此，任何支持的类型都可以通过 JSON 来表示，例如字符串、数字、对象、数组等。但是对象和数组是比较特殊且常用的两种类型：
            ● 对象表示为键值对
            ● 数据由逗号分隔
            ● 花括号保存对象
            ● 方括号保存数组
      3. JSON 键/值对
         JSON 键值对是用来保存 JS 对象的一种方式，键/值对组合中的键名写在前面并用双引号 "" 包裹，使用冒号 : 分隔，然后紧接着值：
            {"firstName": "Json"}
   2. 常用库函数(https://github.com/DaveGamble/cJSON/blob/master/cJSON.h)
      1. 从给定的JSON字符串中得到cJSON对象:cJSON *cJSON_Parse(const char *value)
      2. 从cJSON对象中获取有格式的JSON对象:char *cJSON_Print(cJSON *item)
      3. 删除cJSON对象，释放链表占用的内存空间:void cJSON_Delete(cJSON *c)
      4. 获取cJSON对象数组成员的个数:int cJSON_GetArraySize(cJSON *array)
      5. 根据下标获取cJSON对象数组中的对象:cJSON *cJSON_GetArrayItem(cJSON*array,int item)
      6. 根据键获取对应的值（cJSON对象）:cJSON *cJSON_GetObjectItem(cJSON*object,const char *string)
      7. 创建整型数组类型结构体:cJSON_CreateIntArray(const int *numbers, int count);
         "hex": [51,15,63,22,96]
      8. 创建数组类型结构体:cJSON_CreateArray(void);
         ```c
            cJSON *array = cJSON_CreateArray();
            cJSON *pRelay = cJSON_CreateObject(); 
            cJSON_AddStringToObject(pRelay,"relay","on");
            cJSON_AddItemToArray(array,pRelay);
            cJSON_AddItemToObject(pRoot,"info",array);
            "info": 
            [
                {
                    "relay": "on"        
                }
            ]
         ```
      9.  新增一个字符串类型字段到JSON格式的数据:cJSON_AddStringToObject(object,name,s)
      10. 新增一个数字类型字段到JSON格式的数据:cJSON_AddNumberToObject(object, name, number);
      11. 新增一个新的子节点cJSON到根节点:void cJSON_AddItemToObject(cJSON *object,const char *string,cJSON *item)
   3. 头文件
      文件位于 esp-idf\components\json\cJSON
        使用时包含头文件 cJSON.h
        #include "cJSON.h"
   4. 生成JSON数据
      1. 流程：创建JSON结构体 --> 添加数据 --> 释放内存
      2. 创建JSON结构体
         ```c
            cJSON *pRoot = cJSON_CreateObject();                         // 创建JSON根部结构体
            cJSON *pValue = cJSON_CreateObject();                        // 创建JSON子叶结构体
         ```
      3. 添加字符串类型数据
         ```c
            cJSON_AddStringToObject(pRoot,"mac","65:c6:3a:b2:33:c8");    // 添加字符串类型数据到根部结构体
            cJSON_AddItemToObject(pRoot, "value",pValue);
            cJSON_AddStringToObject(pValue,"day","Sunday");              // 添加字符串类型数据到子叶结构体
         ```
      4. 添加整型数据
         ```c
            cJSON_AddNumberToObject(pRoot,"number",2);                   // 添加整型数据到根部结构体
         ```
      5. 添加数组类型数据
         1. 整型数组
            ```c
                int hex[5]={51,15,63,22,96};
                cJSON *pHex = cJSON_CreateIntArray(hex,5);                   // 创建整型数组类型结构体
                cJSON_AddItemToObject(pRoot,"hex",pHex);                     // 添加整型数组到数组类型结构体
            ```
         2. JSON对象数组
            ```c
                cJSON * pArray = cJSON_CreateArray();                        // 创建数组类型结构体
                cJSON_AddItemToObject(pRoot,"info",pArray);                  // 添加数组到根部结构体
                cJSON * pArray_relay = cJSON_CreateObject();                 // 创建JSON子叶结构体
                cJSON_AddItemToArray(pArray,pArray_relay);                   // 添加子叶结构体到数组结构体            
                cJSON_AddStringToObject(pArray_relay, "relay", "on");        // 添加字符串类型数据到子叶结构体
            ```
      6. 格式化JSON对象
         ```c
            char *sendData == cJSON_Print(pRoot);                        // 从cJSON对象中获取有格式的JSON对象
            os_printf("data:%s\n", sendData);                            // 打印数据
         ```
      7.  释放内存
         ```c
            cJSON_free((void *) sendData);                             // 释放cJSON_Print ()分配出来的内存空间
            cJSON_Delete(pRoot);                                       // 释放cJSON_CreateObject ()分配出来的内存空间
         ```
   5. 解析JSON数据
      1. 流程：判断JSON格式 --> 解析数据 --> 释放内存
      2. 判断是否JSON格式
         ```c
            // receiveData是要剖析的数据
            //首先整体判断是否为一个json格式的数据
            cJSON *pJsonRoot = cJSON_Parse(receiveData);
            //如果是否json格式数据
            if (pJsonRoot !=NULL)
            {
                ···
            }
         ```
      3. 解析字符串类型数据
         ```c
            char bssid[23] = {0};
            cJSON *pMacAdress = cJSON_GetObjectItem(pJsonRoot, "mac");    // 解析mac字段字符串内容
            if (!pMacAdress) return;                                      // 判断mac字段是否json格式
            else
            {
                if (cJSON_IsString(pMacAdress))                           // 判断mac字段是否string类型
                {
                    strcpy(bssid, pMacAdress->valuestring);               // 拷贝内容到字符串数组
                }
            }
         ```
      4. 解析子叶结构体
         ```c
            char strDay[23] = {0};
            cJSON *pValue = cJSON_GetObjectItem(pJsonRoot, "value");      // 解析value字段内容
            if (!pValue) return;                                          // 判断value字段是否json格式
            else
            {
                cJSON *pDay = cJSON_GetObjectItem(pValue, "day");         // 解析子节点pValue的day字段字符串内容
                if (!pDay) return;                                        // 判断day字段是否json格式
                else
                {
                    if (cJSON_IsString(pDay))                             // 判断day字段是否string类型
                    {
                        strcpy(strDay, pDay->valuestring);                // 拷贝内容到字符串数组
                    }
                }
            }
         ```
      5. 解析整型数组数据
         ```c
            cJSON *pArry = cJSON_GetObjectItem(pJsonRoot, "hex");        // 解析hex字段数组内容
            if (!pArry) return;                                          // 判断hex字段是否json格式
            else
            {
                int arryLength = cJSON_GetArraySize(pArry);              // 获取数组长度
                int i;
                for (i = 0; i < arryLength; i++)
                {                                                        // 打印数组内容
                    os_printf("cJSON_GetArrayItem(pArry, %d)= %d\n",i,cJSON_GetArrayItem(pArry, i)->valueint);        
                }
            }
         ```
      6.  解析JSON对象数组数据
         ```c
            cJSON *pArryInfo = cJSON_GetObjectItem(pJsonRoot, "info");   // 解析info字段数组内容
            cJSON *pInfoItem = NULL;
            cJSON *pInfoObj = NULL;
            char strRelay[23] = {0};
            if (!pArryInfo) return;                                      // 判断info字段是否json格式
            else
            {
                int arryLength = cJSON_GetArraySize(pArryInfo);          // 获取数组长度
                int i;
                for (i = 0; i < arryLength; i++)
                {
                    pInfoItem = cJSON_GetArrayItem(pArryInfo, i);        // 获取数组中JSON对象
                    if(NULL != pInfoItem)
                    {
                        pInfoObj = cJSON_GetObjectItem(pInfoItem,"relay");// 解析relay字段字符串内容   
                        if(pInfoObj)
                        {
                            strcpy(strRelay, pInfoObj->valuestring);      // 拷贝内容到字符串数组
                        }
                    }                                                   
                }
            }
            {
                "mac": "65:c6:3a:b2:33:c8",
                "value": 
                {
                    "day": "Sunday"                
                },
                "number": 2,
                "hex": [51,15,63,22,96],
                "info": 
                [
                    {
                        "relay": "on"        
                    }
                ]
            }
         ```
      7.  释放内存
         ```c
            cJSON_Delete(pJsonRoot);                                      // 释放cJSON_Parse()分配出来的内存空间
         ```
