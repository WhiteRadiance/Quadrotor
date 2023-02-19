只要启用长文件名_LFN_就必须选择一种_CODE_PAGE_

cc936.c:	包含三个东西  1.Unicode-GBK  2.GBK-Unicode  3.英语字母大小写互换(如.TXT-.txt)
ccsbc.c:	除去  932(日文), 936(简体), 949(韩文), 950(繁体)  外其他拉丁语系的转换集
syscall.c:	若启用  1)_FS_ReEntrant_   2)使用heap(堆)保存LFN时  其一时才需要添加进工程

【注】不同变换码表的存储空间占用：
CODE_PAGE=437(U.S.):	code=32744, RO-data=6872,    RW-data=140, ZI-data=8268
CODE_PAGE=936(简体):	code=32892, Ro-data=180832, RW-data=140, ZI-data=8268
可见：cc437(英文长文件名)下占用39KByte Flash，cc936(GBK长文件名)下占用213KByte Flash
提示：	STM32F103-RC为 64pin, 256KBFlash, 48KBRam
	STM32F103-C8为 48pin,   64KBFlash, 20KBRam
	STM32F103-RB为 64pin, 128KBFlash, 20KBRam
	STM32F103-ZE为144pin, 512KBFlash, 64KBRam

GBK:		2Byte表示一个汉字, 高字节第一位必是1(>0x7F), 剔除0xXX7F一条线
		完全包含GB2312, 中文标点, 日本汉字, 繁体汉字, 西文&标点(同ASCII)

GB18030:	1 or 2 or 4字节构成一个汉字(变长码), 1字节与ASCII一致, 多字节的首字节的最高位是1
		基本包含GBK, 完全包含GB2312, 收录日韩汉字, 收录中国少数民族的文字
