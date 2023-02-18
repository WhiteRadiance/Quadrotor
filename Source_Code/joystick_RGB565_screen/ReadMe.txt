*因为keil会报错弹出" __use_no_semihosting was requested, but _ttywrch was referenced "，所以工程里system下的usart.c中第50行(Line50)加入了_ttywrch()函数来进入半主机semihost模式
*不想使用上面的方法，就只能注释掉第38行(Line38)的#pragma import(__use_no_semihosting)，但是很有可能会毁坏重定向至串口输出的的 printf() 函数的运行
*还有一种方法就是勾选魔术棒工具里Target页下的 Use Micro LIB 选项框，不太推荐勾选

当前CORE下的startup_stm32f10x_hd.s：
	里面的stack_size(栈区)已由0x0400扩大至0x0800。
	stack_size已继续扩大至0x0A00，heap_size已由0x0200扩大至0x0400(主要因为jpeg的很多malloc)
	jpeg实在太耗内存了,暂时关闭对中文的支持
	已经启用MicroLIB的C语言库来减少ROM占用空间
当前FatFs：
	支持长文件名但不支持中文【以支持GBK】
	仅支持一个盘符(SD='0:\')
	支持字符串功能，激活f_fets(), f_putc(), f_puts(), f_printf()使用权限
	若设置为只读(当前可读可写)，底层仅三个函数有效：
		disk_status()，disk_initialize()，disk_read()，此时上层FatFs不能写入和擦除。
	MIN_SS=512，MIN_MAX=512
当前sdio_sdcard.c/h中为了节省空间，将CSD_Tab[4]和SID_Tab[4]改为了malloc和free，注意SD_Init()函数末尾			即free掉了上述Tab内存空间，此改动导致SD_InitializeCards(), SD_GetCardInfo(), SD_Erase()三个函数		受到影响，前俩函数的输入参数加入了u32*Tab，而erase函数里使用CSD_Tab[1]的部分已替换为调用		全局变量结构体 SDCardInfo. SD_csd. CardComdClasses 子项。综上，因为在SD_Init里面调用了			SD_InitializeCards和SD_GetCardInfo，所以在sd_Init末尾free内存以后，两个Tab[4]就无法使用了，它			们的信息已经保存在 SDCardInfo. SD_csd/SD_cid中，后面一律使用结构体来访问SD卡信息。
为了适配1206字体，因为6*31=126，所以oled显示字符串时从第三列(column2)开始显示，并且show_string()		也进行了偏移，即x换行后右移2列，y换行后多加1小横排像素。(7*8+8=12*5+4=64)
	【现在支持中文以后，对于某些字号取消了换行后的偏移】
显示08564视频一帧有680字节，但是SD卡读680Byte会乱码，经测试分两次读340Byte显示很流畅。
	【因为不明原因单次680Byte也不卡顿了，所以现在代码是单次，双次只读已被注释】

启用DMA的部分：ADC循环扫描，SDIO读写SD卡，SPI刷新OLED屏幕的GRAM

【注意】stm32f10x_config.h(任意.c下拉都能找到此.h)中为了节省ROM注释掉了一些不用的头文件(如fsmc)