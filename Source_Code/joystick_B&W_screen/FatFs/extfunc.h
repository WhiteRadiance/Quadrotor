#ifndef _EXTFUNC_H_
#define _EXTFUNC_H_
#include "stm32f10x.h"
#include "ff.h"

#ifndef NULL
#define NULL	0
#endif

#define F_TXT		1		// *.txt
#define F_BIN		2		// *.bin	-	(oled太小了, 不能兼容所有分辨率的bin文件)
#define F_64_BIN	3		// *64.bin	-	(只是因为oled尺寸原因暂时设计出来的格式)
#define F_FON		4		// *.fon	-	(自制点阵库文件, 除了用于显示汉字外原则上禁止访问)
#define F_MP3		5		// *.mp3
#define F_WAV		6		// *.wav
#define F_JPEG		7		// *.jpeg
#define F_JPG		7		// *.jpg
#define F_PNG		8		// *.png
#define F_BMP		9		// *.bmp
#define F_MP4		10		// *.mp4
#define F_AVI		11		// *.avi



/* Primary Functions */
FRESULT ef_getcapacity(char* drv_path, u32* tot_size, u32* fre_size, u32* sd_size);
FRESULT ef_scanfiles(char* path, u8* num);
u8 ef_checkfile(char* path);

/* Additional Functions (need oled screen) */
void ef_display_SDcapcity(char* path);								//display FatFs size and SD physical size
void ef_display_filelist(char* path,u8 fnum_t,u8 frame,u8 j,u8 delta,u8* head);	//display SDcard File List in current director
u8	 ef_check_extension(char* f_name);								//check File Extension, like ".txt" "64.bin" ".mp3" ...
void ef_multimedia_txt(char* path_t, u8 lib);						//SDcard's Text Editor
void ef_playvideo_bin08564(char* path_t, u8 x, u8 y, u16 frame);	//play video "mix_all_08564.bin"

void ef_FileList_iteration(char* path);								//文件列表FileList的迭代iterate

u8 ef_SD_GetGbkMatrix(u8 GbkCode_H, u8 GbkCode_L, u8 lib, u8* matrix, u8 fast);//根据GBK内码得到对应点阵

#endif
