# udp&tcp
1. udp
   1. 什么是udp
      + UDP（UserDatagramProtocol）是一个简单的面向消息的传输层协议，尽管UDP提供标头和有效负载的完整性验证（通过校验和），但它不保证向上层协议提供消息传递，并且UDP层在发送后不会保留UDP 消息的状态。因此，UDP有时被称为不可靠的数据报协议。如果需要传输可靠性，则必须在用户应用程序中实现。
      + UDP使用具有最小协议机制的简单无连接通信模型。UDP提供数据完整性的校验和，以及用于在数据报的源和目标寻址不同函数的端口号。它没有握手对话，因此将用户的程序暴露在底层网络的任何不可靠的方面。如果在网络接口级别需要纠错功能，应用程序可以使用为此目的设计的传输控制协议(TCP)
      + 特点:
        - UDP是基于IP的简单协议，不可靠的协议。
        - UDP的优点：简单，轻量化。
        - UDP的缺点：没有流控制，没有应答确认机制，不能解决丢包、重发、错序问题。
        - 使用UDP 协议最大的特点就是速度快。
   2. 端口号
      + 端口号是16位的非负整数,范围是0~65535,这个范围会分为三种不同的端口号段，由端口号是由互联网分配号码管理局（IANA）进行分配
        - 周知/标准端口号，它的范围是 0 - 1023。在Unix的操作系统上，使用这些端口之一需要超级用户操作权限
        - 注册端口号，范围是 1024 - 49151。是用于IANA 注册服务的注册端口。
        - 私有端口号，范围是 49152 - 6553。未正式指定用于任何特定服务，可用于任何目的。这些端口也可以用作临时端口，在主机上运行的软件可以使用这些端口根据需要动态创建通信终结点。
      + 端口的作用，简单说就是为了区分不同应用程序的，当电脑接收到一个数据报，将根据不同的端口将数据送给不同的应用程序。所以上面说到互联网分配号码管理局（IANA）分配。
        - 具体分配的细则，大家可以看这个[网站](https://www.iana.org/assignments/service-names-port-numbers/service-names-port-numbers.xhtml)  
        - 以80端口为例，80端口是为HTTP（HyperTextTransport Protocol)即超文本传输协议开放的，此为上网冲浪使用次数最多的协议，主要用于WWW（WorldWide Web）即万维网传输信息的协议
        - 当然端口并不是唯一用来区分不同应用程序的因素，假如来到达服务器的两个80端口的数据报，但实际上，这两个数据报需要送给不同的应用程序。所以仅凭端口号来确定某一条报文显然是不够的。互联网上一般使用 源IP 地址、目标IP地址、源端口号、目标端口号来进行区分。如果其中的某一项不同，就被认为是不同的报文段。这些也是多路分解和多路复用的基础
   3. UDP和ARP之间的交互
      + 一个细节，这是针对一些系统地实现来说的。当ARP(Address Resolution Protocol)缓存还是空的时候。UDP在被发送之前一定要发送一个ARP请求来获得目的主机的MAC地址   
      + `arp -a`可以查看局域网所有IP信息
   4. UDP适用场景及优点
      + UDP协议一般作为流媒体应用、语音交流、视频会议所使用的传输层协议，还有许多基于互联网的电话服务使用的VOIP（基于IP的语音）也是基于UDP运行的，实时视频和音频流协议旨在处理偶尔丢失的数据包，因此，如果重新传输丢失的数据包，则只会发生质量略有下降，而不是出现较大的延迟 
      + 速度快，采用 UDP 协议时，只要应用进程将数据传给 UDP，UDP 就会将此数据打包进 UDP 报文段并立刻传递给网络层，然而TCP有拥塞控制的功能，它会在发送前判断互联网的拥堵情况，如果互联网极度阻塞，那么就会抑制 TCP 的发送方。使用 UDP 的目的就是希望实时性。
      + 无须建立连接，TCP 在数据传输之前需要经过三次握手的操作，而 UDP 则无须任何准备即可进行数据传输。因此 UDP 没有建立连接的时延。
      + 无连接状态，TCP 需要在端系统中维护连接状态，连接状态包括接收和发送缓存、拥塞控制参数以及序号和确认号的参数，在 UDP 中没有这些参数，也没有发送缓存和接受缓存。因此，某些专门用于某种特定应用的服务器当应用程序运行在 UDP 上，一般能支持更多的活跃用户
      + 分组首部开销小，每个 TCP 报文段都有 20 字节的首部开销，而 UDP 仅仅只有 8 字节的开销。 
   5. TCP/IP体系结构 
      + 简单了解一下OSI参考模型，OSI将网络分为七层，自下而上分别是物理层、数据链路层、网络层、传输层、会话层、表示层、应用层，而TCP/IP体系结构则将网络分为四层，自下而上分别是网络接口层、网络层、传输层、应用层。为了将每一层讲明白，我们讲网络接口层拆分为物理层和数据链路层，以下是在《图解TCP/IP》上面找了一张OSI参考模型和TCP/IP体系结构的对照图
      + <img src="img/tcp_ip_struct.png">
      + 介绍了了TCP/IP有哪几层后，再来说一说每一层的大概功能。计算机的世界很奇妙，它里面有很多东西和现实世界都是一一对应的，这也可能是计算机设计者们有意而为之的吧。我先来说一下一个数据包在网络中的传输过程，再来用物流的例子对照着解释一遍，你就应该能够明白每一层的作用了
      + <img src="img/tcp_ip_struct1.png" width=120%>
      + 看上面的图，发送端想要发送数据到接收端。首先应用层准备好要发送的数据，然后给了传输层，传输层的主要作用就是为发送端和接收端提供可靠的连接服务，传输层将数据处理完后就给了网络层。网络层的功能就是管理网络，其中一个核心的功能就是路径的选择(路由)，从发送端到接收端有很多条路，网络层就负责管理下一步数据应该到哪个路由器。选择好了路径之后，数据就来到了数据链路层，这一层就是负责将数据从一个路由器送到另一个路由器。然后就是物理层了，可以简单的理解，物理层就是网线一类的最基础的设备
      + 可能有很多同学看到我上面的一段话后还是一知半解，没关系，我再用物流的例子解释一遍你就明白了
      + <img src="img/tcp_ip_struct2.png" width=80%>
   6. socket通讯模型
      + 什么是socket
        - socket 的原意是“插座”，在计算机通信领域，socket 被翻译为“套接字”，它是计算机之间进行通信的一种约定或一种方式。
        - 通过 socket 这种约定，一台计算机可以接收其他计算机的数据，也可以向其他计算机发送数据。
        - socket 的典型应用就是 Web 服务器和浏览器：浏览器获取用户输入的 URL，向服务器发起请求，服务器分析接收到的 URL，将对应的网页内容返回给浏览器，浏览器再经过解析和渲染，就将文字、图片、视频等元素呈现给用户。
        - 例如我们每天浏览网页、QQ 聊天、收发 email 等等
        - <img src="img/socket.png" width=50%>
      + socket 套接字的分类
        - 通过 socket() 函数创建连接时，必须告诉它使用哪种数据传输方式
        - 1. 流格式套接字（SOCK_STREAM） 
          - 流格式套接字（Stream Sockets）也叫“面向连接的套接字”，在代码中使用 SOCK_STREAM 表示。
          - SOCK_STREAM 是一种可靠的、双向的通信数据流，数据可以准确无误地到达另一台计算机，如果损坏或丢失，可以重新发送。
          - SOCK_STREAM 有以下几个特征：
            - 数据在传输过程中不会消失；
            - 数据是按照顺序传输的；
            - 数据的发送和接收不是同步的（有的说法也称“不存在数据边界”）。
          - 可以将 SOCK_STREAM 想象成一条传输带，只要传输带本身没有问题（不会断网），就能保证数据不丢失；同时，较晚传送的数据不会先到达，较早传送的数据不会晚到达，这就保证了数据是按照顺序传递的。
        - 2. 数据报格式套接字（SOCK_DGRAM）
          - 数据报格式套接字（Datagram Sockets）也叫“无连接的套接字”，在代码中使用 SOCK_DGRAM 表示。
          - 计算机只管传输数据，不作数据校验，如果数据在传输中损坏，或者没有到达另一台计算机，是没有办法补救的。也就是说，数据错了就错了，无法重传。
          - 因为数据报套接字所做的校验工作少，所以在传输效率方面比流格式套接字要高。
          - 可以将 SOCK_DGRAM 比喻成高速移动的摩托车快递，它有以下特征：
            - 强调快速传输而非传输顺序；
            - 传输的数据可能丢失也可能损毁；
            - 限制每次传输的数据大小；
            - 数据的发送和接收是同步的（也称为“存在数据边界”）。
          - 总之，数据报套接字是一种不可靠的、不按顺序传递的、以追求速度为目的的套接字。
          - 视频聊天和语音聊天通常使用SOCK_DGRAM 来传输数据，因为首先要保证通信的效率，尽量减小延迟，而数据的正确性是次要的，即使丢失很小的一部分数据，视频和音频也可以正常解析，最多出现噪点或杂音，不会对通信质量有实质的影响 
   7. udp编程模型
      1. udp编程模型<br><img src="img/udp_flow.png">
      2. socket:创建一个套接字
        + https://www.man7.org/linux/man-pages/man7/socket.7@@man-pages.html
        + int sockfd = socket(int domain, int socket_type, int protocol);
          - domain:用于设置网络通信的域，函数根据这个参数选择通信协议的族。通信协议族在文件sys/socket.h中
            - 定义可以选择 AF_INET（用于 Internet 进程间通信） 或者 AF_UNIX（用于同一台机器进程间通信）,实际编程中常用AF_INET 
          - socket_type：套接字类型用于设置套接字通信的类型，主要有SOCKET_STREAM（流式套接字）、SOCK_DGRAM(数据包套接字)
          - protocol:用于某个协议的特定类型，即type类型中的某个类型。通常某协议中只有一种特定类型，这样protocol参数仅能设置为0；但是有些协议有多种特定的类型，就需要设置这个参数来选择特定的类型。
        + 创建一个udp `sockfd = socket(AF_INET, SOCKET_DGRAM, 0)` 
      3. sendto:发送数据
         + https://www.man7.org/linux/man-pages/man2/sendto.2.html
         + ssize_t sendto(int sockfd, const void *buf, size_t len, int flags,
                          const struct sockaddr *dest_addr, socklen_t addrlen);
           - sockfd:用socket创建的套接字 
           - buf:指向包含要传输数据的缓冲区的指针。
           - len:buf 参数指向的数据长度（以字节为单位）
           - flags:一组标志，用于指定调用的方式。通常设为0
           - dest_addr:指向包含目标套接字地址的 sockaddr 结构的指针
             - 通常不会直接使用`struct sockaddr`,而是用`struct sockaddr_in`类型,具体见[5. ip地址结构]()
           - addrlen:参数指向的地址的大小（以字节为单位）,实际上就是dest_addr的大小
           - 返回值:未发生错误,sendto 将返回发送的字节总数，这可能小于len指示的数字.否则，将返回SOCKET_ERROR值
      4. recvfrom:接收数据
         + https://www.man7.org/linux/man-pages/man2/recvfrom.2.html
         + ssize_t recvfrom(int sockfd, void *restrict buf, size_t len, int flags,
                        struct sockaddr *restrict src_addr,
                        socklen_t *restrict addrlen);
           - sockfd:用socket创建的套接字 
           - buf:传入数据的缓冲区。
           - len:buf 参数指向的缓冲区的长度（以字节为单位）
           - flags:一组标志，用于指定调用的方式。通常设为0
           - src_addr:指向 sockaddr 结构中缓冲区的可选指针，该缓冲区将在返回时保存源地址
           - addrlen:指向参数指向的缓冲区大小（以字节为单位 ）,实际上就是dest_addr的大小
           - 返回值:如果未发生错误,recvfrom 将返回收到的字节数.连接已正常关闭，则返回零.否则,将返回SOCKET_ERROR值
      5. ip地址结构 
         + `struct sockaddr`和`struct sockaddr_in`这两个结构体用来处理网络通信的地址
         + sockaddr:
           - sockaddr在头文件#include <sys/socket.h>中定义，sockaddr的缺陷是：sa_data把目标地址和端口信息混在一起了，如下
           - ```c
                struct sockaddr {  
                sa_family_t sin_family;//地址族
            　　  char sa_data[14]; //14字节，包含套接字中的目标地址和端口信息               
            　　 }; 
             ```
        + sockaddr_in
          - sockaddr_in在头文件#include<netinet/in.h>或#include <arpa/inet.h>中定义，该结构体解决了sockaddr的缺陷，把port和addr 分开储存在两个变量中，如下：
            ```c
                struct sockaddr_in {
                    u8_t            sin_len;
                    sa_family_t     sin_family; //地址簇
                    in_port_t       sin_port;   //16位端口号
                    struct in_addr  sin_addr;   //32位IP地址的结构体,实际上就是一个u32_t
                    char            sin_zero[8]; //不使用
                };
                
                struct in_addr {
                    u32_t s_addr;
                };
            ``` 
          - sin_port和sin_addr都必须是网络字节序（NBO），一般可视化的数字都是主机字节序（HBO）
        + 总结:
          - 二者长度一样，都是16个字节，即占用的内存大小是一致的，因此可以互相转化。二者是并列结构，指向sockaddr_in结构的指针也可以指向sockaddr。
          - sockaddr常用于bind、connect、recvfrom、sendto等函数的参数，指明地址信息，是一种通用的套接字地址。
          - sockaddr_in 是internet环境下套接字的地址形式。所以在网络编程中我们会对sockaddr_in结构体进行操作，使用sockaddr_in来建立所需的信息，最后使用类型转化就可以了。一般先把sockaddr_in变量赋值后，强制类型转换后传入用sockaddr做参数的函数：sockaddr_in用于socket定义和赋值；sockaddr用于函数参数

   8. API汇总
      * 以下 BSD Socket 接口位于 lwip/lwip/src/include/lwip/sockets.h。
      * socket()
      * bind()
      * accept()
      * shutdown()
      * getpeername()
      * getsockopt()＆setsockopt()（请参阅套接字选项）
      * close()（通过虚拟文件系统组件）
      * read()，readv()，write()，writev()（经由虚拟文件系统部件）
      * recv()，recvmsg()，recvfrom()
      * send()，sendmsg()，sendto()
      * select()（通过虚拟文件系统组件）
      * poll()（注意：在ESP-IDF上，poll()是通过内部调用select来实现的，因此，select()如果有可用的方法选择，建议直接使用。）
      * fcntl()（请参阅fcntl）
      * ioctl()（请参阅ioctls）
   9. 常用的转换宏
      1. 字节序转换宏:
        - htons(x) 
        - ntohs(x) 
        - htonl(x) 
        - ntohl(x) 
      2. IP地址转换函数:  
         1. `in_addr_t inet_addr(const char *cp);` //点分十进制地址的地址转成网络字节序
            `dest_addr.sin_addr.s_addr = inet_addr("127.0.0.1");`
         2. int inet_aton(const char *cp, struct in_addr *inp);
            + 参数  cp：传入的ip地址；inp   指向转换后存储到struct in_addr结构体的s_addr；
            ```c
                int ip_addr;
                struct in_addr inet_ip_addr;
                ip_addr = inet_aton("192.168.2.125", &inet_ip_addr);
                printf("%d\n",inet_ip_addr.s_addr);   
            ```
         3. `char *inet_ntoa(struct in_addr in);` //网络字节序转给点分十进制
            ```c
                struct in_addr network;
                network.s_addr=2097326272;    //为s_addr赋值--网络字节序
                printf("IP : %s\n", inet_ntoa(network));
            ``` 
   10. udp客户端步骤
      1. 新建socket
         * 
         ```c
            int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
            if(sockfd < 0) 
            {
                ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
            }
                    
         ```
      2. 配置将要连接的服务器信息（端口和IP）
         ```c
            #define HOST_IP_ADDR    "192.168.110.239"    // 要连接UDP服务器地址
            #define UDP_PORT        9000                 // 要连接UDP服务器端口号

            struct sockaddr_in dest_addr;
            dest_addr.sin_family = AF_INET;
            dest_addr.sin_addr.s_addr = inet_addr(HOST_IP_ADDR);
            dest_addr.sin_port = htons(UDP_PORT);
         ```
      3. 发送数据
         ```c
            char *msg = "Message from ESP32";
            //以下语句有个小bug
            int err = sendto(sockfd, msg, sizeof(msg), 0, (struct sockaddr *) &dest_addr, sizeof(dest_addr));
            if (err < 0) {
                ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
                close(sockfd);
            }
         ```
      4. 接收数据
         ```c
            while (1){
                char buf[100];
                struct sockaddr from;
                uint32_t fromlen = sizeof(from);
                int len = recvfrom(sockfd, (void *)buf, sizeof(buf), 0, &from, &fromlen);
                if(len > 0){
                    printf("receive:%d\n",len);
                    printf("%s\n",buf);

                    struct sockaddr_in *src_addr = (struct sockaddr_in *)&from;
                    printf("from:%s\n",inet_ntoa(src_addr->sin_addr.s_addr));
                }
            }
         ```
      5. 实例:
      ```c
        #include <stdio.h>

        #include "esp_wifi.h"
        #include "freertos/FreeRTOS.h"
        #include "freertos/task.h"
        #include "nvs_flash.h"
        #include "string.h"
        #include "esp_log.h"
        #include "esp_smartconfig.h"
        #include "freertos/event_groups.h"
        #include "lwip/sockets.h"
        #define TAG "main"
        
        void udp(void){
            printf("udp start\n");
            int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
            if(sockfd < 0){
                ESP_LOGE("", "Unable to create socket: errno %d", errno);
            }

            struct sockaddr_in dest_addr;
            dest_addr.sin_family = AF_INET;
            dest_addr.sin_addr.s_addr = inet_addr("192.168.110.239");
            dest_addr.sin_port = htons(9000);

            char *msg = "Message from ESP32";
            int err = sendto(sockfd, msg, sizeof(msg), 0, (struct sockaddr *) &dest_addr, sizeof(dest_addr));
            if (err < 0) {
                ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
                close(sockfd);
            }
            while (1){
                char buf[100];
                struct sockaddr from;
                uint32_t fromlen = sizeof(from);
                int len = recvfrom(sockfd, (void *)buf, sizeof(buf), 0, &from, &fromlen);
                if(len > 0){
                    printf("receive:%d\n",len);
                    printf("%s\n",buf);

                    struct sockaddr_in *src_addr = (struct sockaddr_in *)&from;
                    printf("from:%s\n",inet_ntoa(src_addr->sin_addr.s_addr));
                }
            }
        }

        static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id,void* event_data)
        {
            ESP_LOGI("", "event_base:%s， event_id：%d\r\n", event_base, event_id);
            if (event_base == WIFI_EVENT) {
                switch (event_id) {
                case WIFI_EVENT_STA_START:  // STA模式启动
                    esp_wifi_connect();
                    break;
                case WIFI_EVENT_STA_DISCONNECTED:
                    
                    break;
                }
            }
            if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP){
                ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
                ESP_LOGI("", "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
                xTaskCreate(udp,"",1028 * 4,NULL,5,NULL);
            }
            
        }

        void app_main(void)
        {
            nvs_flash_init();
            esp_netif_init();
            
            esp_event_loop_create_default();  // 创建一个默认的事件循环
            
            esp_netif_create_default_wifi_sta();//必须放在esp_event_loop_create_default()后面,否则收不到IP_EVENT_STA_GOT_IP

            wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
            ESP_ERROR_CHECK(esp_wifi_init(&cfg));
            esp_wifi_set_mode(WIFI_MODE_STA);

            esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, NULL);
            esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP,&event_handler, NULL, NULL);

            wifi_config_t conf={.sta.ssid="YQ2103",
                                .sta.password="abcd12345",
                                .sta.threshold.authmode= WIFI_AUTH_WPA2_PSK,};
            esp_wifi_set_config(WIFI_IF_STA, &conf);
            esp_wifi_start();
            
        }

      ```  
   11. udp服务器步骤
      1. 新建socket
            ```c
                int addr_family = 0;
                int ip_protocol = 0;

                addr_family = AF_INET;
                ip_protocol = IPPROTO_IP;

                int sock =  socket(addr_family, SOCK_DGRAM, ip_protocol);
                if(sock < 0) 
                {
                    ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
                }
            ```
       2. 配置服务器信息
            ```c
                #define UDP_PORT        3333                 // UDP服务器端口号

                struct sockaddr_in dest_addr;
                dest_addr.sin_family = AF_INET;
                dest_addr.sin_addr.s_addr = htonl(INADDR_ANY);
                dest_addr.sin_port = htons(UDP_PORT);
            ```
       3. 绑定地址
            ```c
                int err = bind(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
                if(err < 0)
                {
                    ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
                    close(sock);
                }
                ESP_LOGI(TAG, "Socket bound, port %d", PORT);
            ```
       4. 接收数据
            ```c
                char rx_buffer[128];
                char host_ip[] = HOST_IP_ADDR;

                while(1)
                {
                    ······
                    // 清空缓存
                    memset(rx_buffer, 0, sizeof(rx_buffer));

                    struct sockaddr_in source_addr; // Large enough for both IPv4 or IPv6
                    socklen_t socklen = sizeof(source_addr);
                    int len = recvfrom(sock, rx_buffer, sizeof(rx_buffer), 0, (struct sockaddr *)&source_addr, &socklen);

                    // Error occurred during receiving
                    if(len < 0) 
                    {
                        ESP_LOGE(TAG, "recvfrom failed: errno %d", errno);
                        break;
                    }
                    // Data received
                    else 
                    {
                        ESP_LOGI(TAG, "Received %d bytes from %s:", len, host_ip);
                        ESP_LOGI(TAG, "%s", rx_buffer);
                    }
                }
                close(sock);
            ```
       5. 发送数据
            ```c
                static const char *payload = "Message from ESP32 ";

                int err = sendto(sock, payload, strlen(payload), 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
                if(err < 0) 
                {
                    ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
                    close(sock);
                }
            ```
2. tcp
   1. TCP与UDP优缺点
      1. TCP面向连接（如打电话要先拨号建立连接）;UDP是无连接的，即发送数据之前不需要建立连接。
      2. TCP提供可靠的服务。也就是说，通过TCP连接传送的数据，无差错，不丢失，不重复，且按序到达
      3. UDP尽最大努力交付，即不保证可靠交付。
      4. TCP通过校验和，重传控制，序号标识，滑动窗口、确认应答实现可靠传输。如丢包时的重发控制，还可以对次序乱掉的分包进行顺序控制
      5. UDP具有较好的实时性，工作效率比TCP高，适用于对高速传输和实时性有较高的通信或广播通信。
      6. 每一条TCP连接只能是点到点的;UDP支持一对一，一对多，多对一和多对多的交互通信。
      7. TCP对系统资源要求较多，UDP对系统资源要求较少
3. tcp client
   1. 主要流程<br><img src="tcp.png">
   2. 新建socket
        ```c
            int addr_family = 0;
            int ip_protocol = 0;

            addr_family = AF_INET;
            ip_protocol = IPPROTO_IP;

            int sock =  socket(addr_family, SOCK_STREAM, ip_protocol);
            if(sock < 0) 
            {
                ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
                // 新建失败后，关闭新建的socket，等待下次新建
                close(sock);
            }
        ```
   3. 配置将要连接的服务器信息（端口和IP）
        ```c
            #define TCP_SERVER_ADRESS    "192.168.61.217"    // 要连接TCP服务器地址
            #define TCP_PORT             3333                // 要连接TCP服务器端口号

            struct sockaddr_in dest_addr;
            dest_addr.sin_family = AF_INET;
            dest_addr.sin_addr.s_addr = inet_addr(TCP_SERVER_ADRESS);
            dest_addr.sin_port = htons(TCP_PORT);
        ```
   4. 连接服务器
        ```c
            int err = connect(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr);
            if(err != 0) 
            {
                ESP_LOGE(TAG, "Socket unable to connect: errno %d", errno);
                // 连接失败后，关闭之前新建的socket，等待下次新建
                close(sock);
            }

        ```
   5. 接收数据
        ```c
            char rx_buffer[128];

            while (1) 
            {
                ······
                int len = recv(sock, rx_buffer, sizeof(rx_buffer), 0);
                // Error occurred during receiving
                if (len < 0) 
                {
                    ESP_LOGE(TAG, "recv failed: errno %d", errno);
                    break;
                }
                // Data received
                else 
                {
                    memset(rx_buffer, 0, sizeof(rx_buffer));
                    ESP_LOGI(TAG, "Received %d bytes from %s:", len, TCP_SERVER_ADRESS);
                    ESP_LOGI(TAG, "%s", rx_buffer);
                }
            }

        ```
   6. 发送数据
        ```c
            static const char *payload = "Message from ESP32 ";

            int err = send(sock, payload, strlen(payload), 0);
            if (err < 0) 
            {
                ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
            }

        ```
4. tcp server
   1. 主要流程<br><img src="tcp.png">
   2. 新建socket
    ```c
        int addr_family = 0;
        int ip_protocol = 0;

        addr_family = AF_INET;
        ip_protocol = IPPROTO_IP;

        int listen_sock =  socket(addr_family, SOCK_STREAM, ip_protocol);
        if(listen_sock < 0) 
        {
            ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
        }
    ```
   3. 配置服务器信息
    ```c
        #define TCP_PORT             3333                // TCP服务器端口号

        struct sockaddr_in dest_addr;
        dest_addr.sin_family = AF_INET;
        dest_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        dest_addr.sin_port = htons(TCP_PORT);
    ```
   4. 绑定地址
    ```c
        int err = bind(listen_sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
        if(err != 0) 
        {
            ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
            ESP_LOGE(TAG, "IPPROTO: %d", addr_family);
            close(listen_sock);
        }
        ESP_LOGI(TAG, "Socket bound, port %d", PORT);
    ```
   5. 开始监听
    ```c
        err = listen(listen_sock, 1);    // 这里为啥是1，网上大多数是5
        if(err != 0) 
        {
            ESP_LOGE(TAG, "Error occurred during listen: errno %d", errno);
            close(listen_sock);
        }
    ```
   6.  等待客户端连接
    ```c
        while(1)
        {
            struct sockaddr_in6 source_addr; // Large enough for both IPv4 or IPv6
            uint addr_len = sizeof(source_addr);
            int connect_sock = accept(listen_sock, (struct sockaddr *)&source_addr, &addr_len);
            if(connect_sock < 0) 
            {
                ESP_LOGE(TAG, "Unable to accept connection: errno %d", errno);
                close(listen_sock);
            }    
        }
    ```
   7.  接收数据
    ```c
        int len;
        char rx_buffer[128];

        while(1)
        {
            memset(rx_buffer, 0, sizeof(rx_buffer));    // 清空缓存        
            len = recv(connect_sock, rx_buffer, sizeof(rx_buffer), 0);  // 读取接收数据
            if(len < 0) 
            {
                ESP_LOGE(TAG, "Error occurred during receiving: errno %d", errno);
            } 
            else if (len == 0) 
            {
                ESP_LOGW(TAG, "Connection closed");
            } 
            else 
            {
                ESP_LOGI(TAG, "Received %d bytes: %s", len, rx_buffer);
            }
        }
    ```
   8.  发送数据
    ```c
        send(connect_socket, rx_buffer, sizeof(rx_buffer, 0);
    ```
