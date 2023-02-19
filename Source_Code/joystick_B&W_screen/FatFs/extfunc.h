#ifndef _EXTFUNC_H_
#define _EXTFUNC_H_
#include "stm32f10x.h"
#include "ff.h"

#ifndef NULL
#define NULL	0
#endif

#define F_TXT		1		// *.txt
#define F_BIN		2		// *.bin	-	(oled̫С��, ���ܼ������зֱ��ʵ�bin�ļ�)
#define F_64_BIN	3		// *64.bin	-	(ֻ����Ϊoled�ߴ�ԭ����ʱ��Ƴ����ĸ�ʽ)
#define F_FON		4		// *.fon	-	(���Ƶ�����ļ�, ����������ʾ������ԭ���Ͻ�ֹ����)
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

void ef_FileList_iteration(char* path);								//�ļ��б�FileList�ĵ���iterate

u8 ef_SD_GetGbkMatrix(u8 GbkCode_H, u8 GbkCode_L, u8 lib, u8* matrix, u8 fast);//����GBK����õ���Ӧ����

#endif
