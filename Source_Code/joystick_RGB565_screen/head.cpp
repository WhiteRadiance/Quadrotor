/*	本程序用于给Image2Lcd生成的图片或视频(多张bin图片)添加6或8字节的文件头
 *	其实软件本身可以添加文件头的，本程序用于后期手动加入文件头
 *	因为视频不可能每一张bin图片都加上文件头，只需在第一张(即视频的头部)加入文件头就行
 *	文件头包含扫描方式，大小端，色彩深度，图片长宽，RGB格式 等信息，格式如下：
 *		Byte 1:		Scan	-> 0x10(高位在前,水平扫描); 0x11(高位在前,垂直扫描)
 *		Byte 2:		Depth	-> 0x01(ibit); 0x10(16bit); 0x12(18bit); 0x18(24bit)
 *		Byte 3,4:	Width	-> 0x0080(128); 0x0780(1920)
 *		Byte 5,6:	Height 	-> 0x0060(96); 0x0438(1080)
 *		Byte 7:	   16-Form	-> 0x01(565); 0x00(555)
 *		Byte 8:	   16-Order	-> 0x1B(R-G-B order); 0x39(B-G-R order)
 *	单色bin文件头只有前6字节，16bit或更高色深时拥有全部8字节文件头
**/

#include <stdio.h>


const unsigned char HEAD_SIZE = 6;	//文件头包含几个字节

int main()
{
	FILE* fp;
	fp = fopen("head.bin", "wb+");	//新建二进制文件，清空内容，允许读写

	unsigned char head[HEAD_SIZE] = {0};
	head[0] = 0x11;	//高位在前，垂直扫描
	head[1] = 0x01;	//1bit色深（单色）
	head[2] = 0x00;
	head[3] = 0x6A;	//width = 106
	head[4] = 0x00;
	head[5] = 0x4F;	//height= 79

	// head[6] = ;
	// head[7] = ;

	for(unsigned char i=0; i<HEAD_SIZE; i++)
		//fprintf(fp, "%u", head[i]);
		fwrite(head+i, 1, 1, fp);

	printf("\n%d\n", sizeof(char));
	fclose(fp);
	return 0;
}