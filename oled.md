# OLED屏幕定位
  + OLED显示是点阵式显示，，最常见的规格有128x64,126x96等。OLED放大了看就如下图一样，每一个点就是一个像素，通过很多个点的点亮来组成字符。
    <br><img src="img\oled.png">
    - ssd1306内部ram和行列的映射图 
    <br><img src="img\ssd1306_ram.png" width = 50%>
    - ssd1306内部ram和行列的映射放大图
    <br><img src="img\ssd1306_ram_enlargetment.png" width = 50%>
  + 那么在MCU中在这么大片的点阵中是如何告诉哪些点阵点亮来实现字符的显示呢
    - ssd1306有三种寻址方式:页寻址，水平寻址和垂直寻址
      <br><img src="img\ssd1306_address_mode.png" width = 70%>
  + 页寻址:
    + 下面讨论最流行的页寻址，简单、高效，多了记不住。现在市面上流行OLED驱动芯片都是SSD系列芯片核心驱动，访问方式类似。它的页寻址方式如下图箭头所示，最大共11页(对于ssd1036最大是8页)，128列。一页是8行。
    <br><img src="img\ssd1306_page_mode.png" width = 70%><br>
    + 页寻址流程(文档中的原文如下:)
      - Set the page start address of the target display location by command B0h to B7h. 
      - Set the lower start column address of pointer by command 00h~0Fh.  
      - Set the upper start column address of pointer by command 10h~1Fh 
  + SSD1036芯片手册中对于页地址解释如下oo
    <br><img src="img\ssd1036_page.png" width = 50%><br>
  + B0~BB是代表页寻址方式，X2，X1，X0就是需要访问的页码，在程序中直接将0xb0加上y,即`ssd1306_writeCmd(0xb0 + y);`，对于芯片来说就是页寻址及页码地址设置。对于程序来说y就是第几行字的意思.<br><br>
  上面的程序代码实际上就是显示在屏幕上第几行的意思,接下来需要关心第几列<br>
  + 列地址,在SSD中叫" Column Start Address for Page"<br>
   <img src="img\ssd1306_column_addr.png" width= 50%><br>
  + ssd1306有128列,ssd用二条命令分别设置.分成了lower column和higher column.用程序代码就是` ssd1306_writeCmd((x & 0x0f) | 0x00);`和`ssd1306_writeCmd(((x & 0xf0) >> 4) | 0x10);`
  + 最终的定位程序如下
  ```c
    void ssd1306_set_pos(unsigned char x, unsigned char y)
    {
        ssd1306_writeCmd(0xb0 + y); //写页码,就是第几行.一行8个点
        ssd1306_writeCmd((x & 0x0f) | 0x00 );//写lower column,就是行地址的低4位. 每列1个点
        ssd1306_writeCmd(((x & 0xf0) >> 4) | 0x10); //写higher column,就是行地址的高4位.
    }
  ```
