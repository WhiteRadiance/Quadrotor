//#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "oled.h"
#include "ff.h"
#include "fmt_jpeg.h"


#if defined SSD1306_C128_R64
	extern u8	Frame_Buffer[128][8];
#elif defined SSD1351_C128_R128
	extern u16	Frame_Buffer[128][128];
#elif defined SSD1351_C128_R96
	extern u16	Frame_Buffer[96][128];
#endif


extern FRESULT	f_res;
extern UINT		bw,br;



inline u16 rgb888_to_565(u8 R8, u8 G8, u8 B8)
{
	u16 R5 = ((u16)R8*249 + 1014) >> 11;
	u16 G6 = ((u16)G8*253 +  505) >> 10;
	u16 B5 = ((u16)B8*249 + 1014) >> 11;
	
//	u16 R5 = (u16)R8 >> 3;
//	u16 G6 = (u16)G8 >> 2;
//	u16 B5 = (u16)B8 >> 3;
	return ((R5<<11) | (G6<<5) | (B5));
}


//double数据的四舍五入
inline int round_double(double val)
{
	double res = (val > 0.0) ? (val + 0.5) : (val - 0.5);
	return (int)res;
}


//2Byte小端转换为大端
u16 rd_BigEnd16(u16 val)
{
	u8	b1 = (u8)(val);
	u8	b2 = (u8)(val >> 8);
	u16 temp = (u16)b2 + ((u16)b1 << 8);
	return temp;
}


//对数组全部元素进行求和,用于求DC/AC_NRCodes数组的sum来确定DC/AC_Values数组的长度
static u16 sum_arr(u8* arr, u8 len_arr)
{
	u16 sum = 0;
	for (u8 i = 0; i < len_arr; i++)
		sum += arr[i];
	return sum;
}


//为了方便对齐8/16pixel的MCU, 预读取SOF0的clrY_sample用于判断jpeg的采样比率
//为了方便开辟空间, 预读取SOF0的height/width
void rd_SOF0_2get_size(FIL* fp, u8* pY_sample, u16* pimw, u16* pimh)
{
	u16 seg_label_t = 0;
	u16 seg_size_t = 0;
	u8  dump = 0;						//占位字节,用于保存不需管的字节

	f_read(fp, &seg_label_t, 2, &br);	//SOI

	f_read(fp, &seg_label_t, 2, &br);	//find SOF0 label
	while (seg_label_t != 0xC0FF)
	{
		if (seg_label_t == 0x01FF)		//TEM
			f_read(fp, &seg_label_t, 2, &br);
		else {
			f_read(fp, &seg_size_t, 2, &br);
			f_lseek(fp, f_tell(fp) + rd_BigEnd16(seg_size_t) - 2);
			f_read(fp, &seg_label_t, 2, &br);
		}
	}
	f_read(fp, &seg_size_t, 2, &br);	//seg_len
	f_read(fp, &dump, 1, &br);			//acc
	f_read(fp, pimh, 2, &br);
	*pimh = rd_BigEnd16(*pimh);			//imheight
	f_read(fp, pimw, 2, &br);
	*pimw = rd_BigEnd16(*pimw);			//imwidth
	f_read(fp, &dump, 1, &br);			//color_component
	f_read(fp, &dump, 1, &br);			//clrY_id = 0x01
	f_read(fp, pY_sample, 1, &br);		//0x11(444) or 0x22(420)

	f_rewind(fp);						//rewind to the start of the file
	return;
}




//======================== 解码核心函数 ======================== 解码核心函数 =========================




void Inverse_DCT(double res[][8], double block[][8])
{
	double temp[8][8] = { 0.0 };
	for (u8 i = 0; i < 8; i++) {
		for (u8 j = 0; j < 8; j++) {
			res[i][j] = 0.0;
			for (u8 t = 0; t < 8; t++) {
				temp[i][j] += DCT_T[i][t] * block[t][j];
			}
		}
	}
	for (u8 i = 0; i < 8; i++) {
		for (u8 j = 0; j < 8; j++) {
			for (u8 t = 0; t < 8; t++) {
				res[i][j] += temp[i][t] * DCT[t][j];
			}
		}
	}
}




//将Huffman表写入SD卡,生成DHT的时候顺便计算相同码字长度下的(最大码字+1),生成由码长定位码字的映射表len_tbl
//LuChroDCAC: 0x00=Y_DC, 0x01=UV_DC, 0x10=Y_AC, 0x11=UV_AV
void Build_Huffman_Table_SD(const u8* nr_codes, const u8* std_tbl, u16* max_inThisLen, u8* huff_len_tbl[], u8 LuChroDCAC, short presize)
{
	u8 pos_in_tbl = 0;
	u16 code_val = 0;
	FIL fp;
	if(LuChroDCAC == 0x00)
		f_res = f_open(&fp, "0:/PICTURE/jpg/YdcHuffTbl.dat", FA_WRITE | FA_CREATE_ALWAYS);
	else if(LuChroDCAC == 0x01)
		f_res = f_open(&fp, "0:/PICTURE/jpg/UVdcHuffTbl.dat", FA_WRITE | FA_CREATE_ALWAYS);
	else if(LuChroDCAC == 0x10)
		f_res = f_open(&fp, "0:/PICTURE/jpg/YacHuffTbl.dat", FA_WRITE | FA_CREATE_ALWAYS);
	else if(LuChroDCAC == 0x11)
		f_res = f_open(&fp, "0:/PICTURE/jpg/UVacHuffTbl.dat", FA_WRITE | FA_CREATE_ALWAYS);
	else;
	if(f_res==FR_OK)
	{
		for(short i=0; i<presize; i++)
		{
			int izero = 0;
			u8  uzero = 0;
			f_write(&fp, &izero, 4, &bw);
			f_write(&fp, &uzero, 1, &bw);
		}
		f_rewind(&fp);
		for(short k = 1; k <= 16; k++)
		{
			u8 num = 0;
			for(short j = 1; j <= nr_codes[k - 1]; j++)
			{
				huff_len_tbl[k][num] = std_tbl[pos_in_tbl];
				num++;
				//huffman_tbl[std_tbl[pos_in_tbl]].code = code_val;
				//huffman_tbl[std_tbl[pos_in_tbl]].len = k;
				f_lseek(&fp, std_tbl[pos_in_tbl] * 5);
				f_write(&fp, &code_val, 4, &bw);
				f_write(&fp, &k, 1, &bw);
				pos_in_tbl++;
				code_val++;
			}
			max_inThisLen[k] = code_val;
			code_val <<= 1;
		}
		max_inThisLen[0] = 0;
		/* Close the file */
		f_close(&fp);
	}
	else
	{
		oled_show_string(0, 0, (u8*)"can't create dat.", 12, BLACK, CYAN);
		oled_show_num_every0(0, 20, f_res, 2, 12, BLACK, CYAN);
		oled_refresh();
	}
}



//newBytePos初始化为负数,当其为负数时说明进入函数时应该重新读取新的newByte
//回复为rle数组的同时顺便恢复成8×8的矩阵形式
void rd_restore_HuffBit_RLE_Mat(int* rle, int temp[][8], int* pnewByte, int* pnewBytePos, int* prev_DC, const u16* max_first_DC, \
	const u16* max_first_AC, u8* huff_lentbl_DC[], u8* huff_lentbl_AC[], FIL* fp, const u8* dc_nrcodes, const u8* ac_nrcodes, u8 IsLumi)
{
	//首先出现的是RLE编码的Huffman码字,然后根据码字低字节得知下个码字的码长,并读取幅度码字
	static u8 mmask[8] = { 1,2,4,8,16,32,64,128 };
	int value = 0, code_len = 0;
	u8 rle_idx = 0;
	BitString bitcode_1, bitcode_2;										//RLE编码对儿的第一项(修复前nrz/len修复后nrz)和第二项(magnitude)
	bitcode_1.code = 0; bitcode_1.len = 0;
	bitcode_2.code = 0; bitcode_2.len = 0;
	u8 xx = 0, yy = 0;
	
	FIL fp_dat;
	if(IsLumi == 1)  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		f_res = f_open(&fp_dat, "0:/PICTURE/jpg/YdcHuffTbl.dat", FA_READ | FA_OPEN_EXISTING);
	else
		f_res = f_open(&fp_dat, "0:/PICTURE/jpg/UVdcHuffTbl.dat", FA_READ | FA_OPEN_EXISTING);
	if(f_res != FR_OK)
	{
		oled_show_string(0, 40, (u8*)"can't load dat.", 12, BLACK, CYAN);
		oled_refresh();
		return;
	}
	
	//outer-func default newByte=0, newBytePos=-1
	while (value >= (int)max_first_DC[code_len])						//必须是 >=, 因为max指示的值是当前最大码字+1,可能是后面码字的前缀
	{
		if (*pnewBytePos < 0) {
			if (*pnewByte == 0xFF)	f_read(fp, pnewByte, 1, &br);		//0xFF之后会紧跟一个无意义的0x00(忽略掉)
			f_read(fp, pnewByte, 1, &br);
			*pnewBytePos = 7;
		}
		value <<= 1;
		value = value + ((*pnewByte & mmask[*pnewBytePos]) ? 1 : 0);
		(*pnewBytePos)--;
		code_len++;
	}

	for (int p = 0; p < dc_nrcodes[code_len - 1]; p++) {				//DC映射表其实不会太长(远小于130)
		int huffcode;
		f_lseek(&fp_dat, huff_lentbl_DC[code_len][p] * 5);
		f_read(&fp_dat, &huffcode, 4, &br);
		//if (rd_hufftbl_DC[huff_lentbl_DC[code_len][p]].code == value) {	//在码长映射表中寻找与value相符合的码字
		if(huffcode == value) {
			bitcode_1.code = huff_lentbl_DC[code_len][p];				//这里没有存码字(如110b),而是直接存(如0x5)
			bitcode_1.len = code_len;									//注意DC部分的nrz肯定是0,故省略0x05成0x5
			rle[rle_idx] = bitcode_1.code;								//DC的nrz肯定是0,不过为了兼容AC还是保存了0
			rle_idx++;
			value = 0;
			code_len = 0;
			break;
		}
	}
	f_close(&fp_dat);
	
	if (bitcode_1.code == 0) {
		bitcode_2.code = 0;
		bitcode_2.len = 0;
		rle[0] = 0;														//将第一项从Bit编码修复为RLE编码的DC_nrz=0
		rle[rle_idx] = bitcode_2.code + *prev_DC;						//差分编码是0时表示当前实际DC值等于prev_DC
		temp[0][0] = rle[rle_idx];
		yy++;
		rle_idx++;
	}
	else {
		bitcode_2.len = bitcode_1.code & 0x0F;
		rle[0] = 0;														//修复第一项
		for (int p = 0; p < bitcode_2.len; p++) {						//第一项指明了幅度码字的码长
			if (*pnewBytePos < 0) {
				if (*pnewByte == 0xFF)	f_read(fp, pnewByte, 1, &br);		//0xFF之后会紧跟一个无意义的0x00(忽略掉)
				f_read(fp, pnewByte, 1, &br);
				*pnewBytePos = 7;
			}
			value <<= 1;
			//value |= (*newByte & mmask[*newBytePos]);
			value = value + ((*pnewByte & mmask[*pnewBytePos]) ? 1 : 0);
			(*pnewBytePos)--;
			code_len++;
			if ((p == 0) && (value == 0))
				bitcode_2.code = -1;									//暂时把code置负,用于指示首个bit不是1(在幅度编码中代表负数)
		}
		if (bitcode_2.code < 0) {
			int tt = (-1) * (1 << bitcode_2.len) + 1;
			bitcode_2.code = value + tt + *prev_DC;
			rle[rle_idx] = bitcode_2.code;
			temp[0][0] = rle[rle_idx];
			yy++;
			rle_idx++;
		}
		else {
			bitcode_2.code = value + *prev_DC;
			rle[rle_idx] = bitcode_2.code;
			temp[0][0] = rle[rle_idx];
			yy++;
			rle_idx++;
		}
		*prev_DC = bitcode_2.code;										//更新prev_DC
		value = 0;
		code_len = 0;
		bitcode_1.code = 0;	bitcode_1.len = 0;
		bitcode_2.code = 0;	bitcode_2.len = 0;
	}
	
	/*if(IsLumi==2)
	{
		oled_show_num_every0(0, 110, (u8)temp[0][0], 6, 12, BLACK, CYAN);
		oled_show_num_every0(50, 110, (u8)*pnewByte, 6, 12, BLACK, CYAN);
		oled_show_num_every0(100, 110, (u8)*pnewBytePos, 4, 12, BLACK, CYAN);
		oled_refresh();
		while(1);
	}*/
	
	if(IsLumi == 1)  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		f_res = f_open(&fp_dat, "0:/PICTURE/jpg/YacHuffTbl.dat", FA_READ | FA_OPEN_EXISTING);
	else
		f_res = f_open(&fp_dat, "0:/PICTURE/jpg/UVacHuffTbl.dat", FA_READ | FA_OPEN_EXISTING);
	if(f_res != FR_OK)
	{
		oled_show_string(0, 40, (u8*)"can't load dat.", 12, BLACK, CYAN);
		oled_refresh();
		return;
	}
	
	//AC part
	int nums = 0;
	//记录RLE编码的原始数据中AC部分的长度(63),注意连零的个数也会记录...
	//(不然长度无法自增到63,另外结尾不出现0时也是不存在EOB的)
	for (nums = 0; nums < 63; nums++)
	{
		while (value >= (int)max_first_AC[code_len])
		{
			if (*pnewBytePos < 0) {
				if (*pnewByte == 0xFF)	f_read(fp, pnewByte, 1, &br);		//0xFF之后会紧跟一个无意义的0x00(忽略掉)
				f_read(fp, pnewByte, 1, &br);
				*pnewBytePos = 7;
			}
			value <<= 1;
			value = value + ((*pnewByte & mmask[*pnewBytePos]) ? 1 : 0);
			(*pnewBytePos)--;
			code_len++;
		}
		for (int p = 0; p < ac_nrcodes[code_len - 1]; p++) {
			int huffcode;
			f_lseek(&fp_dat, huff_lentbl_AC[code_len][p] * 5);
			f_read(&fp_dat, &huffcode, 4, &br);
			//if (rd_hufftbl_AC[huff_lentbl_AC[code_len][p]].code == value) {	//在码长映射表中寻找与value相符合的码字
			if(huffcode == value) {
				bitcode_1.code = huff_lentbl_AC[code_len][p];			//这里是直接存坐标映射(也就是码字对应的值)
				bitcode_1.len = code_len;
				rle[rle_idx] = bitcode_1.code;
				rle_idx++;
				value = 0;
				code_len = 0;
				break;
			}
		}
		if (bitcode_1.code == 0xF0) {									//处理AC出现的(15,0)
			bitcode_2.code = 0;
			bitcode_2.len = 0;
			rle[rle_idx - 1] = 15;
			rle[rle_idx] = bitcode_2.code;
			rle_idx++;
			for (u8 z = 0; z < 16; z++) {								//（15,0）实际上是16个0
				temp[xx][yy] = 0;
				yy++;
				if (yy >= 8) { xx++; yy = 0; }
			}
			nums += 15;
		}
		else if (bitcode_1.code == 0x00) {								//EOB:处理AC出现的(0,0)
			bitcode_2.code = 0;
			bitcode_2.len = 0;
			rle[rle_idx - 1] = 0;
			rle[rle_idx] = bitcode_2.code;
			rle_idx++;
			while (xx <= 7) {
				temp[xx][yy] = 0;
				yy++;
				if (yy >= 8) { xx++; yy = 0; }
			}
			nums = 63;													//auto-break the loop
			//break;
		}
		else {
			bitcode_2.len = bitcode_1.code & 0x0F;
			rle[rle_idx - 1] >>= 4;										//将第一项从Bit编码(如0x25)修复为RLE编码(如0x02)
			for (u8 z = 0; z < rle[rle_idx - 1]; z++) {
				temp[xx][yy] = 0;
				yy++;
				if (yy >= 8) { xx++; yy = 0; }
			}
			nums += rle[rle_idx - 1];
			for (int p = 0; p < bitcode_2.len; p++) {					//第一项指明了幅度码字的码长
				if (*pnewBytePos < 0) {
					if (*pnewByte == 0xFF)	f_read(fp, pnewByte, 1, &br);	//0xFF之后会紧跟一个无意义的0x00(忽略掉)
					f_read(fp, pnewByte, 1, &br);
					*pnewBytePos = 7;
				}
				value <<= 1;
				value = value + ((*pnewByte & mmask[*pnewBytePos]) ? 1 : 0);
				(*pnewBytePos)--;
				code_len++;
				if ((p == 0) && (value == 0))
					bitcode_2.code = -1;								//暂时把code置负,用于指示首个bit不是1(在幅度编码中代表负数)
			}
			if (bitcode_2.code < 0) {
				int tt = (-1) * (1 << bitcode_2.len) + 1;
				bitcode_2.code = value + tt;
				rle[rle_idx] = bitcode_2.code;
				rle_idx++;
				temp[xx][yy] = bitcode_2.code;
				yy++;
				if (yy >= 8) { xx++; yy = 0; }
				value = 0;
				code_len = 0;
			}
			else {
				bitcode_2.code = value;
				rle[rle_idx] = bitcode_2.code;
				rle_idx++;
				temp[xx][yy] = bitcode_2.code;
				yy++;
				if (yy >= 8) { xx++; yy = 0; }
				value = 0;
				code_len = 0;
			}
		}
		bitcode_1.code = 0;	bitcode_1.len = 0;
		bitcode_2.code = 0;	bitcode_2.len = 0;
	}
	f_close(&fp_dat);
}




//调用函数之前已经反差分解码过并已写成原始矩阵格式,函数内部反量化并反ZigZag处理
void rd_restore_InvQuantize(double afterInvQ[][8], int temp[][8], const u8* Qm)
{
	//inv-Quantize
	for (u8 i = 0; i < 8; i++) {
		for (u8 j = 0; j < 8; j++) {
			temp[i][j] = temp[i][j] * Qm[i * 8 + j];
		}
	}
	//inv-ZigZag
	for (u8 i = 0; i < 8; i++) {
		for (u8 j = 0; j < 8; j++) {
			afterInvQ[RLE_ZigZag[i * 8 + j][0]][RLE_ZigZag[i * 8 + j][1]] = (double)temp[i][j];
		}
	}
}




//======================== 超级核心函数 ======================== 超级核心函数 =========================




/*
 * intro:	jpeg解码
 *			yuv_blk会转换为rgb565_blk然后赋给Frame_Buffer
 * param:	yuv_Y 为16*16Byte, yuv_U,yuv_V 为8*8Byte
 *			ox/oy 是图片居中以后左上角的坐标点
 * note:	本函数没有使用fclose()来关闭文件
 *			本函数暂只能解码444和420采样率的JPEG图片至YUV,输出固定为yuv444p格式
 *			注意jpeg解码时应该根据文件生成对应的DQT和DHT, 为了兼容性也绝不能使用标准表
 *			注意jpeg文件中除了段头之外其余数据中若出现0xFF后都会紧跟一个无意义的0x00
 *			注意jpeg文件存在包含相机信息的Exif段, 另外jpeg会容忍段头出现的若干个0xFF (函数还不支持此条要求)
 */
void Dec_JPEG_to_YuvRgb(FIL* fp, u8* yuv_Y, u8* yuv_U, u8* yuv_V, u8 ox, u8 oy)
{
	struct s_JPEG_header Jheader;

	u16 height = 0, width = 0;
	u16 seg_label_t = 0;
	u16 seg_size_t = 0;
	u16 res_sync = 0;

	f_read(fp, &Jheader.SOI, 2, &br);
	Jheader.SOI = rd_BigEnd16(Jheader.SOI);
	
	oled_show_string(0, 0, (u8*)"SOI done.", 12, CYAN, BLACK);///////////////////////////////////////////////////
	oled_refresh();

	f_read(fp, &seg_label_t, 2, &br);
	while (seg_label_t != 0xDAFF)
	{
		switch (seg_label_t)
		{
		case 0xE0FF:							// APP0/JFIF (是最常见的APPn)
			f_read(fp, &seg_size_t, 2, &br);
			Jheader.APP0 = rd_BigEnd16(seg_label_t);			Jheader.app0len = rd_BigEnd16(seg_size_t);
			f_read(fp, Jheader.app0id, 5, &br);
			f_read(fp, &Jheader.jfifver, 2, &br);				//Jheader.jfifver = rd_BigEnd16(Jheader.jfifver);
			f_read(fp, &Jheader.xyunit, 1, &br);
			f_read(fp, &Jheader.xden, 2, &br);					//Jheader.xden = rd_BigEnd16(Jheader.xden);
			f_read(fp, &Jheader.yden, 2, &br);					//Jheader.yden = rd_BigEnd16(Jheader.yden);
			f_read(fp, &Jheader.thumbh, 1, &br);
			f_read(fp, &Jheader.thumbv, 1, &br);
		
			oled_show_string(0, 13, (u8*)"APP0 done.", 12, CYAN, BLACK);///////////////////////////////////////////////////
			oled_refresh();
			break;

		case 0xE1FF:							// APP1/Exif (包含相机信息等)
			f_read(fp, &seg_size_t, 2, &br);
			f_lseek(fp, f_tell(fp) + rd_BigEnd16(seg_size_t) - 2);
		
			oled_show_string(0, 13, (u8*)"APP1 skipped.", 12, CYAN, BLACK);///////////////////////////////////////////////////
			oled_refresh();
			break;

		case 0xDBFF:							// DQT
			f_read(fp, &seg_size_t, 2, &br);
		//oled_show_num_every0(50,50,seg_size_t,5,12,CYAN,BLACK);////////////////////////////////////////////////
		//oled_refresh();
			if (seg_size_t == 0x8400)			//0x84 = 132 = 2+(1+64)*2	->	单个label包含两个DQT的情况
			{
				for (int t = 0; t < 2; t++)
				{
					u8	dqt_id_t = 0;
					f_read(fp, &dqt_id_t, 1, &br);
					if (dqt_id_t == 0) {
						Jheader.DQT0.label = rd_BigEnd16(seg_label_t);
						Jheader.DQT0.dqtlen = rd_BigEnd16(seg_size_t);
						Jheader.DQT0.dqtacc_id = dqt_id_t;
						f_read(fp, Jheader.DQT0.QTable, 64, &br);
					}
					else if (dqt_id_t == 1) {
						Jheader.DQT1.label = rd_BigEnd16(seg_label_t);
						Jheader.DQT1.dqtlen = rd_BigEnd16(seg_size_t);
						Jheader.DQT1.dqtacc_id = dqt_id_t;
						f_read(fp, Jheader.DQT1.QTable, 64, &br);
					}
					else;
				}
			}
			else if (seg_size_t == 0x4300)		//0x43 = 67 = 2+1+64		->	每个label各包含一个DQT的情况(更常见)
			{
				f_lseek(fp, f_tell(fp) - 4);	//rewind 2+2 Byte
				for (int t = 0; t < 2; t++)
				{
					u8	dqt_id_t = 0;
					f_read(fp, &seg_label_t, 2, &br);
					f_read(fp, &seg_size_t, 2, &br);
					f_read(fp, &dqt_id_t, 1, &br);
					if (dqt_id_t == 0) {
						Jheader.DQT0.label = rd_BigEnd16(seg_label_t);
						Jheader.DQT0.dqtlen = rd_BigEnd16(seg_size_t);
						Jheader.DQT0.dqtacc_id = dqt_id_t;
						f_read(fp, Jheader.DQT0.QTable, 64, &br);
					}
					else if (dqt_id_t == 1) {
						Jheader.DQT1.label = rd_BigEnd16(seg_label_t);
						Jheader.DQT1.dqtlen = rd_BigEnd16(seg_size_t);
						Jheader.DQT1.dqtacc_id = dqt_id_t;
						f_read(fp, Jheader.DQT1.QTable, 64, &br);
					}
					else;
				}
			}
			else;
			
			oled_show_string(0, 26, (u8*)"DQT done.", 12, CYAN, BLACK);///////////////////////////////////////////////////
			oled_refresh();
			break;

		case 0xC0FF:							// SOF0
			f_read(fp, &seg_size_t, 2, &br);
			Jheader.SOF0 = rd_BigEnd16(seg_label_t);			Jheader.sof0len = rd_BigEnd16(seg_size_t);
			f_read(fp, &Jheader.sof0acc, 1, &br);
			f_read(fp, &Jheader.imheight, 2, &br);				Jheader.imheight = rd_BigEnd16(Jheader.imheight);	//image height/pixel
			f_read(fp, &Jheader.imwidth, 2, &br);				Jheader.imwidth = rd_BigEnd16(Jheader.imwidth);		//image width/pixel
			width = Jheader.imwidth;
			height = Jheader.imheight;
			f_read(fp, &Jheader.clrcomponent, 1, &br);
			for (int t = 0; t < Jheader.clrcomponent; t++) {	//clr_c=3:YCbCr, =4:CMYK, 但是JFIF只支持YCbCr(YUV)
				u8	clr_id_t = 0, clr_hvsample_t = 0, clr_Qid_t = 0;
				f_read(fp, &clr_id_t, 1, &br);
				f_read(fp, &clr_hvsample_t, 1, &br);
				f_read(fp, &clr_Qid_t, 1, &br);
				if (clr_id_t == 0x01) {
					Jheader.clrY_id = clr_id_t;
					Jheader.clrY_sample = clr_hvsample_t;
					Jheader.clrY_QTable = clr_Qid_t;
				}
				else if (clr_id_t == 0x02) {
					Jheader.clrU_id = clr_id_t;
					Jheader.clrU_sample = clr_hvsample_t;
					Jheader.clrU_QTable = clr_Qid_t;
				}
				else if (clr_id_t == 0x03) {
					Jheader.clrV_id = clr_id_t;
					Jheader.clrV_sample = clr_hvsample_t;
					Jheader.clrV_QTable = clr_Qid_t;
				}
				else;
			}
			
			oled_show_string(0, 39, (u8*)"SOF0 done.", 12, CYAN, BLACK);///////////////////////////////////////////////////
			oled_refresh();
			break;

		case 0xC4FF:							// DHT
			f_read(fp, &seg_size_t, 2, &br);
			seg_size_t = rd_BigEnd16(seg_size_t);

			//如果DHT段的容量大于0xE0则暂时认为这是单个label包含4个表的情况
			//大多数情况下都是独立的四个较小的独立的DHT表: 每个label包含一个表(共4个重复DHT段)
			if (seg_size_t < 0x00E0)
				f_lseek(fp, f_tell(fp) - 4);			//rewind 2+2 Byte
			for (u8 t = 0; t < 4; t++)
			{
				u8 dht_id_t = 0;
				if (seg_size_t < 0x00E0) {
					f_read(fp, &seg_label_t, 2, &br);
					f_read(fp, &seg_size_t, 2, &br);
					seg_size_t = rd_BigEnd16(seg_size_t);
				}
				f_read(fp, &dht_id_t, 1, &br);
				if (dht_id_t == 0x00) {			//Lumi DC
					Jheader.DHT_DC0.label = rd_BigEnd16(seg_label_t);
					Jheader.DHT_DC0.hufftype_id = dht_id_t;
					f_read(fp, Jheader.DHT_DC0.DC_NRcodes, 16, &br);
					u16 Values_len = sum_arr(Jheader.DHT_DC0.DC_NRcodes, 16);
					if (Values_len == 0)		return;
					Jheader.DHT_DC0.dhtlen = 16 + Values_len + 1 + 2;	//codes + values + id + self_seg_len
					Jheader.DHT_DC0.DC_Values = (u8*)malloc(sizeof(u8) * Values_len);
					f_read(fp, Jheader.DHT_DC0.DC_Values, Values_len, &br);
				}
				else if (dht_id_t == 0x01) {	//Chromi DC
					Jheader.DHT_DC1.label = rd_BigEnd16(seg_label_t);
					Jheader.DHT_DC1.hufftype_id = dht_id_t;
					f_read(fp, Jheader.DHT_DC1.DC_NRcodes, 16, &br);
					u16 Values_len = sum_arr(Jheader.DHT_DC1.DC_NRcodes, 16);
					if (Values_len == 0)		return;
					Jheader.DHT_DC1.dhtlen = 16 + Values_len + 1 + 2;	//codes + values + id + self_seg_len
					Jheader.DHT_DC1.DC_Values = (u8*)malloc(sizeof(u8) * Values_len);
					f_read(fp, Jheader.DHT_DC1.DC_Values, Values_len, &br);
				}
				else if (dht_id_t == 0x10) {	//Lumi AC
					Jheader.DHT_AC0.label = rd_BigEnd16(seg_label_t);
					Jheader.DHT_AC0.hufftype_id = dht_id_t;
					f_read(fp, Jheader.DHT_AC0.AC_NRcodes, 16, &br);
					u16 Values_len = sum_arr(Jheader.DHT_AC0.AC_NRcodes, 16);
					if (Values_len == 0)		return;
					Jheader.DHT_AC0.dhtlen = 16 + Values_len + 1 + 2;	//codes + values + id + self_seg_len
					Jheader.DHT_AC0.AC_Values = (u8*)malloc(sizeof(u8) * Values_len);
					f_read(fp, Jheader.DHT_AC0.AC_Values, Values_len, &br);
				}
				else if (dht_id_t == 0x11) {	//Chromi AC
					Jheader.DHT_AC1.label = rd_BigEnd16(seg_label_t);
					Jheader.DHT_AC1.hufftype_id = dht_id_t;
					f_read(fp, Jheader.DHT_AC1.AC_NRcodes, 16, &br);
					u16 Values_len = sum_arr(Jheader.DHT_AC1.AC_NRcodes, 16);
					if (Values_len == 0)		return;
					Jheader.DHT_AC1.dhtlen = 16 + Values_len + 1 + 2;	//codes + values + id + self_seg_len
					Jheader.DHT_AC1.AC_Values = (u8*)malloc(sizeof(u8)* Values_len);
					f_read(fp, Jheader.DHT_AC1.AC_Values, Values_len, &br);
				}
				else;
			}
			
			oled_show_string(0, 52, (u8*)"DHT done.", 12, CYAN, BLACK);///////////////////////////////////////////////////
			oled_refresh();
			break;

		case 0xDDFF:							// DRI
			f_read(fp, &seg_size_t, 2, &br);
			f_read(fp, &res_sync, 2, &br);
			res_sync = rd_BigEnd16(res_sync);
			break;

		case 0x01FF:							// TEM (label only)
			break;

		default:								// PhotoShop图片的APP13/14, ICC色彩联盟的APP2, 剩余的APPn 和 罕见的标识符就直接跳过
			f_read(fp, &seg_size_t, 2, &br);
			f_lseek(fp, f_tell(fp) + rd_BigEnd16(seg_size_t) - 2);
			break;
		}

		f_read(fp, &seg_label_t, 2, &br);
	}

	oled_show_string(0, 65, (u8*)"Build Huffman ", 12, CYAN, BLACK);///////////////////////////////////////////////////
	oled_refresh();
	
	//JPEG固定SOS段的后面就是图像的压缩数据
	//读SOS段之前先把前面的DHT处理成Huffman表和Huffman码长查找表
	u16	max_first_Lumi_DC[17] = { 0 }, max_first_Lumi_AC[17] = { 0 }, max_first_Chromi_DC[17] = { 0 }, max_first_Chromi_AC[17] = { 0 };
	//动态申请二级指针的空间(每一项对应的空间大小是不一样的)
	u8* HuffTbl_len_Y_DC[17] = { NULL }, * HuffTbl_len_Y_AC[17] = { NULL }, * HuffTbl_len_UV_DC[17] = { NULL }, * HuffTbl_len_UV_AC[17] = { NULL };
	for (u8 m = 0; m < 16; m++) {
		if (Jheader.DHT_DC0.DC_NRcodes[m] != 0)		HuffTbl_len_Y_DC[m + 1] = (u8*)malloc(sizeof(u8) * Jheader.DHT_DC0.DC_NRcodes[m]);
		if (Jheader.DHT_AC0.AC_NRcodes[m] != 0)		HuffTbl_len_Y_AC[m + 1] = (u8*)malloc(sizeof(u8) * Jheader.DHT_AC0.AC_NRcodes[m]);
		if (Jheader.DHT_DC1.DC_NRcodes[m] != 0)		HuffTbl_len_UV_DC[m + 1] = (u8*)malloc(sizeof(u8) * Jheader.DHT_DC1.DC_NRcodes[m]);
		if (Jheader.DHT_AC1.AC_NRcodes[m] != 0)		HuffTbl_len_UV_AC[m + 1] = (u8*)malloc(sizeof(u8) * Jheader.DHT_AC1.AC_NRcodes[m]);

		if (HuffTbl_len_Y_DC[m + 1] != NULL)	memset(HuffTbl_len_Y_DC[m + 1], 0, Jheader.DHT_DC0.DC_NRcodes[m]);
		if (HuffTbl_len_Y_AC[m + 1] != NULL)	memset(HuffTbl_len_Y_AC[m + 1], 0, Jheader.DHT_AC0.AC_NRcodes[m]);
		if (HuffTbl_len_UV_DC[m + 1] != NULL)	memset(HuffTbl_len_UV_DC[m + 1], 0, Jheader.DHT_DC1.DC_NRcodes[m]);
		if (HuffTbl_len_UV_AC[m + 1] != NULL)	memset(HuffTbl_len_UV_AC[m + 1], 0, Jheader.DHT_AC1.AC_NRcodes[m]);
	}
	//HuffType *rd_HuffTbl_Y_DC = (HuffType*)malloc(sizeof(HuffType) * Jheader.DHT_DC0.dhtlen - 2 - 1 - 16);
	//rd_HuffTbl_Y_AC = (HuffType*)malloc(sizeof(HuffType) * 256);
	//HuffType *rd_HuffTbl_UV_DC = (HuffType*)malloc(sizeof(HuffType) * Jheader.DHT_DC1.dhtlen - 2 - 1 - 16);
	//rd_HuffTbl_UV_AC = (HuffType*)malloc(sizeof(HuffType) * 256);
	/*if (rd_HuffTbl_Y_DC == NULL || rd_HuffTbl_Y_AC == NULL || rd_HuffTbl_UV_DC == NULL || rd_HuffTbl_UV_AC == NULL){
		oled_show_string(84, 65, (u8*)"failed!", 12, CYAN, BLACK);///////////////////////////////////////////////////
		oled_refresh();
		
		for (u8 m = 1; m < 17; m++) {
			if(HuffTbl_len_Y_DC[m] != NULL)		free(HuffTbl_len_Y_DC[m]);
			if(HuffTbl_len_Y_AC[m] != NULL)		free(HuffTbl_len_Y_AC[m]);
			if(HuffTbl_len_UV_DC[m] != NULL)	free(HuffTbl_len_UV_DC[m]);
			if(HuffTbl_len_UV_AC[m] != NULL)	free(HuffTbl_len_UV_AC[m]);
		}
		if(rd_HuffTbl_Y_DC != NULL)		free(rd_HuffTbl_Y_DC);
		if(rd_HuffTbl_UV_DC != NULL)	free(rd_HuffTbl_UV_DC);
		return;
	}*/
	//memset(rd_HuffTbl_Y_DC, 0, sizeof(HuffType)* Jheader.DHT_DC0.dhtlen - 2 - 1 - 16);
	//memset(rd_HuffTbl_Y_AC, 0, sizeof(HuffType) * 256);
	//memset(rd_HuffTbl_UV_DC, 0, sizeof(HuffType)* Jheader.DHT_DC1.dhtlen - 2 - 1 - 16);
	//memset(rd_HuffTbl_UV_AC, 0, sizeof(HuffType) * 256);

	Build_Huffman_Table_SD(Jheader.DHT_DC0.DC_NRcodes, Jheader.DHT_DC0.DC_Values, max_first_Lumi_DC, HuffTbl_len_Y_DC, 0x00, Jheader.DHT_DC0.dhtlen - 19);
	Build_Huffman_Table_SD(Jheader.DHT_AC0.AC_NRcodes, Jheader.DHT_AC0.AC_Values, max_first_Lumi_AC, HuffTbl_len_Y_AC, 0x10, 256);
	Build_Huffman_Table_SD(Jheader.DHT_DC1.DC_NRcodes, Jheader.DHT_DC1.DC_Values, max_first_Chromi_DC, HuffTbl_len_UV_DC, 0x01, Jheader.DHT_DC1.dhtlen - 19);
	Build_Huffman_Table_SD(Jheader.DHT_AC1.AC_NRcodes, Jheader.DHT_AC1.AC_Values, max_first_Chromi_AC, HuffTbl_len_UV_AC, 0x11, 256);	
	
	//DC/AC_Values已经用不到了
	free(Jheader.DHT_DC0.DC_Values);
	free(Jheader.DHT_DC1.DC_Values);
	free(Jheader.DHT_AC0.AC_Values);
	free(Jheader.DHT_AC1.AC_Values);

	oled_show_string(84, 65, (u8*)"done.", 12, CYAN, BLACK);///////////////////////////////////////////////////
	oled_refresh();
	
	// SOS
	f_read(fp, &seg_size_t, 2, &br);
	Jheader.SOS = rd_BigEnd16(seg_label_t);					Jheader.soslen = rd_BigEnd16(seg_size_t);
	f_read(fp, &Jheader.component, 1, &br);
	f_read(fp, &Jheader.Y_id_dht, 2, &br);					//Jheader.Y_id_dht = rd_BigEnd16(Jheader.Y_id_dht);
	f_read(fp, &Jheader.U_id_dht, 2, &br);					//Jheader.U_id_dht = rd_BigEnd16(Jheader.U_id_dht);
	f_read(fp, &Jheader.V_id_dht, 2, &br);					//Jheader.V_id_dht = rd_BigEnd16(Jheader.V_id_dht);
	f_read(fp, &Jheader.SpectrumS, 1, &br);		//the following three items are fixed
	f_read(fp, &Jheader.SpectrumE, 1, &br);
	f_read(fp, &Jheader.SpectrumC, 1, &br);
	
	oled_show_string(0, 78, (u8*)"start de-compresing.", 12, CYAN, BLACK);
	oled_refresh();
	oled_clear();

	// 读取原始压缩数据
	int newByte = 0, newBytePos = -1;
	int	prev_DC_Y = 0, prev_DC_U = 0, prev_DC_V = 0;
	
	double	res_IDCT_YUV[8][8] = { 0.0 };
	double	res_afterInvQ_YUV[8][8] = { 0.0 };
	int		rle_YUV[128] = { 0 };
	int		mat_YUV[8][8] = { 0 };	//由rle直接恢复出的原始矩阵,还没有反量化和反Zigzag
	
	if (Jheader.clrY_sample == 0x11)						//jpeg in yuv444
	{
		u8 res_sync_count = 0;	//DRI段需要的复位计数器
		for (int yPos = 0; yPos < height; yPos += 8) {
			for (int xPos = 0; xPos < width; xPos += 8) {
				res_sync_count++;
				if (res_sync != 0 && res_sync_count > res_sync) {	//DRI复位差分编码和复位Huffman编码
					newByte = 0, newBytePos = -1;
					prev_DC_Y = 0, prev_DC_U = 0, prev_DC_V = 0;
					f_lseek(fp, f_tell(fp) + 2);							//跳过RSTm标签0xFFDm
					res_sync_count = 1;
				}
				//int		MCU_Y[8][8] = { 0 };
				//double	res_IDCT_Y[8][8] = { 0.0 };
				//double	res_afterInvQ_Y[8][8] = { 0.0 };
				//int		rle_Y[128] = { 0 };
				//int		mat_Y[8][8] = { 0 };	//由rle直接恢复出的原始矩阵,还没有反量化和反Zigzag
				
				//Y-channel
				rd_restore_HuffBit_RLE_Mat(rle_YUV, mat_YUV, &newByte, &newBytePos, &prev_DC_Y, max_first_Lumi_DC, max_first_Lumi_AC, HuffTbl_len_Y_DC, \
					HuffTbl_len_Y_AC, fp, Jheader.DHT_DC0.DC_NRcodes, Jheader.DHT_AC0.AC_NRcodes, 1);
				rd_restore_InvQuantize(res_afterInvQ_YUV, mat_YUV, Jheader.DQT0.QTable);
				Inverse_DCT(res_IDCT_YUV, res_afterInvQ_YUV);
				for (int i = 0; i < 8; i++) {
					for (int j = 0; j < 8; j++) {
						MCU_Y[i][j] = round_double(res_IDCT_YUV[i][j] + 128.0);
						if (MCU_Y[i][j] < 0x00)		MCU_Y[i][j] = 0x00;
						if (MCU_Y[i][j] > 0xFF)		MCU_Y[i][j] = 0xFF;
					}
				}
				
				//int		MCU_U[8][8] = { 0 }, MCU_V[8][8] = { 0 };
				//double	res_IDCT_UV[8][8] = { 0.0 };//, res_IDCT_V[8][8] = { 0.0 };
				//double	res_afterInvQ_UV[8][8] = { 0.0 };//, res_afterInvQ_V[8][8] = { 0.0 };
				//int		rle_UV[128] = { 0 };//, rle_V[128] = { 0 };
				//int		mat_UV[8][8] = { 0 };//, mat_V[8][8] = { 0 };	//由rle直接恢复出的原始矩阵,还没有反量化和反Zigzag


				//U-channel
				rd_restore_HuffBit_RLE_Mat(rle_YUV, mat_YUV, &newByte, &newBytePos, &prev_DC_U, max_first_Chromi_DC, max_first_Chromi_AC, HuffTbl_len_UV_DC, \
					HuffTbl_len_UV_AC, fp, Jheader.DHT_DC1.DC_NRcodes, Jheader.DHT_AC1.AC_NRcodes, 0);
				rd_restore_InvQuantize(res_afterInvQ_YUV, mat_YUV, Jheader.DQT1.QTable);
				Inverse_DCT(res_IDCT_YUV, res_afterInvQ_YUV);
				for (int i = 0; i < 8; i++) {
					for (int j = 0; j < 8; j++) {
						MCU_U[i][j] = round_double(res_IDCT_YUV[i][j] + 128.0);
						if (MCU_U[i][j] < 0x00)		MCU_U[i][j] = 0x00;
						if (MCU_U[i][j] > 0xFF)		MCU_U[i][j] = 0xFF;
					}
				}
				/*oled_show_num_every0(0, 110, (u16)res_afterInvQ_YUV[0][0], 5, 12, BLACK, CYAN);
				oled_show_num_every0(40, 110, (u16)res_IDCT_YUV[0][0], 5, 12, BLACK, CYAN);
				oled_show_num_every0(80, 110, (u16)MCU_U[0][0], 5, 12, BLACK, CYAN);
				oled_refresh();
				while(1);*/
				
				//memset(rle_YUV, 0, 128);
				//V-channel
				rd_restore_HuffBit_RLE_Mat(rle_YUV, mat_YUV, &newByte, &newBytePos, &prev_DC_V, max_first_Chromi_DC, max_first_Chromi_AC, HuffTbl_len_UV_DC, \
					HuffTbl_len_UV_AC, fp, Jheader.DHT_DC1.DC_NRcodes, Jheader.DHT_AC1.AC_NRcodes, 0);
				rd_restore_InvQuantize(res_afterInvQ_YUV, mat_YUV, Jheader.DQT1.QTable);
				Inverse_DCT(res_IDCT_YUV, res_afterInvQ_YUV);
				for (int i = 0; i < 8; i++) {
					for (int j = 0; j < 8; j++) {
						MCU_V[i][j] = round_double(res_IDCT_YUV[i][j] + 128.0);
						if (MCU_V[i][j] < 0x00)		MCU_V[i][j] = 0x00;
						if (MCU_V[i][j] > 0xFF)		MCU_V[i][j] = 0xFF;
					}
				}
				
				/*FIL ffpp;
				f_res = f_open(&ffpp, "0:/PICTURE/jpg/mcu.dat", FA_WRITE | FA_CREATE_ALWAYS);
				for (u8 i = 0; i < 8; i++) {
					for (u8 j = 0; j < 8; j++) {
						u16 us = (u16)mat_YUV[i][j];
						f_write(&ffpp, &us, 2, &bw);
					}
				}
				for (u8 i = 0; i < 8; i++) {
					for (u8 j = 0; j < 8; j++) {
						u16 us = (u16)MCU_U[i][j];
						f_write(&ffpp, &us, 2, &bw);
					}
				}
//				for (u8 i = 1; i < 17; i++)
//					f_write(&ffpp, &max_first_Chromi_DC[i], 2, &bw);
//				for (u8 i = 1; i < 17; i++)
//					f_write(&ffpp, &max_first_Chromi_AC[i], 2, &bw);
				f_write(&ffpp, Jheader.DHT_AC1.AC_NRcodes, 16, &bw);
				f_write(&ffpp, Jheader.DHT_AC1.AC_Values, 162, &bw);
				f_close(&ffpp);
				while(1);*/
				

				for (u8 i = 0; i < 8; i++)		//write Y to YUV
				{
					if (yPos + i >= height)							//图片Y平面的下侧边缘
						break;
					for (u8 j = 0; j < 8; j++) {
						if (xPos + j >= width)						//图片Y平面的右侧边缘
							break;
						yuv_Y[i*16 + j] = (u8)MCU_Y[i][j];			//注意yuv中y的大小是16x16
					}
				}
				
				for (u8 i = 0; i < 8; i++)		//(jpeg444)write UV to YUV_444
				{
					if (yPos + i >= height)							//图片UV平面的下侧边缘
						break;
					for (u8 j = 0; j < 8; j++) {
						if (xPos + j >= width)						//图片UV平面的右侧边缘
							break;
						yuv_U[i*8 + j] = (u8)MCU_U[i][j];
						yuv_V[i*8 + j] = (u8)MCU_V[i][j];
						
						//颜色转换
						short R=0, G=0, B=0;	//rgb888
						short u = (short)yuv_U[i*8 + j] - 128;
						short v = (short)yuv_V[i*8 + j] - 128;
						R = (short)yuv_Y[i*16 + j] + v + ((v*103)>>8);
						G = (short)yuv_Y[i*16 + j] - ((u*88)>>8) - ((v*183)>>8);
						B = (short)yuv_Y[i*16 + j] + u + ((u*198)>>8);
//						R = yuv_Y[i*16 + j] + 1.402 * ((short)yuv_V[i*8 + j] - 128);
//						G = yuv_Y[i*16 + j] - 0.344 * ((short)yuv_U[i*8 + j] - 128) - 0.714 * ((short)yuv_V[i*8 + j] - 128);
//						B = yuv_Y[i*16 + j] + 1.772 * ((short)yuv_U[i*8 + j] - 128);
						
						R = (R > 255) ? 255 : R;		R = (R < 0) ? 0 : R;
						G = (G > 255) ? 255 : G;		G = (G < 0) ? 0 : G;
						B = (B > 255) ? 255 : B;		B = (B < 0) ? 0 : B;
						
						Frame_Buffer[oy + yPos + i][ox + xPos + j] = rgb888_to_565((u8)R, (u8)G, (u8)B);
					}
				}
			}
			oled_refresh();
		}
	}
	else if (Jheader.clrY_sample == 0x22)					//jpeg in yuv411(e.g. yuv420p = l420)
	{
		u16 res_sync_count = 0;	//DRI段需要的复位计数器
		for (int yPos = 0; yPos < height; yPos += 16) {
			for (int xPos = 0; xPos < width; xPos += 16) {
				for (int u = 0; u < 4; u++)
				{
					res_sync_count++;
					if (res_sync != 0 && res_sync_count > res_sync) {	//DRI复位差分编码和复位Huffman编码
						newByte = 0, newBytePos = -1;
						prev_DC_Y = 0, prev_DC_U = 0, prev_DC_V = 0;
						f_lseek(fp, f_tell(fp) + 2);					//跳过RSTm标签0xFFDm
						res_sync_count = 1;
					}
					//int		MCU_Y[8][8] = { 0 };
					//double	res_IDCT_Y[8][8] = { 0.0 };
					//double	res_afterInvQ_Y[8][8] = { 0.0 };
					//int		rle_Y[128] = { 0 };
					//int		mat_Y[8][8] = { 0 };	//由rle直接恢复出的原始矩阵,还没有反量化和反Zigzag

					//Y-channel
					rd_restore_HuffBit_RLE_Mat(rle_YUV, mat_YUV, &newByte, &newBytePos, &prev_DC_Y, max_first_Lumi_DC, max_first_Lumi_AC, HuffTbl_len_Y_DC, \
						HuffTbl_len_Y_AC, fp, Jheader.DHT_DC0.DC_NRcodes, Jheader.DHT_AC0.AC_NRcodes, 1);
					rd_restore_InvQuantize(res_afterInvQ_YUV, mat_YUV, Jheader.DQT0.QTable);
					Inverse_DCT(res_IDCT_YUV, res_afterInvQ_YUV);
					for (int i = 0; i < 8; i++) {
						for (int j = 0; j < 8; j++) {
							MCU_Y[i][j] = round_double(res_IDCT_YUV[i][j] + 128.0);
							if (MCU_Y[i][j] < 0x00)		MCU_Y[i][j] = 0x00;
							if (MCU_Y[i][j] > 0xFF)		MCU_Y[i][j] = 0xFF;
						}
					}

					for (u8 i = 0; i < 8; i++) {					//write Y to YUV
						if (yPos + i + (u / 2) * 8 >= height)		//图片Y平面的下侧边缘
							break;
						for (u8 j = 0; j < 8; j++) {
							if (xPos + j + (u % 2) * 8 >= width)	//图片Y平面的右侧边缘
								break;
							yuv_Y[(i + (u / 2) * 8) * 16 + j + (u % 2) * 8] = MCU_Y[i][j];	//注意yuv中y的大小是16x16
						}
					}
				}

				//int		MCU_U[8][8] = { 0 }, MCU_V[8][8] = { 0 };
				//double	res_IDCT_U[8][8] = { 0.0 }, res_IDCT_V[8][8] = { 0.0 };
				//double	res_afterInvQ_U[8][8] = { 0.0 }, res_afterInvQ_V[8][8] = { 0.0 };
				//int		rle_U[128] = { 0 }, rle_V[128] = { 0 };
				//int		mat_U[8][8] = { 0 }, mat_V[8][8] = { 0 };	//由rle直接恢复出的原始矩阵,还没有反量化和反Zigzag

				//U-channel
				rd_restore_HuffBit_RLE_Mat(rle_YUV, mat_YUV, &newByte, &newBytePos, &prev_DC_U, max_first_Chromi_DC, max_first_Chromi_AC, HuffTbl_len_UV_DC, \
					HuffTbl_len_UV_AC, fp, Jheader.DHT_DC1.DC_NRcodes, Jheader.DHT_AC1.AC_NRcodes, 0);
				rd_restore_InvQuantize(res_afterInvQ_YUV, mat_YUV, Jheader.DQT1.QTable);
				Inverse_DCT(res_IDCT_YUV, res_afterInvQ_YUV);
				for (int i = 0; i < 8; i++) {
					for (int j = 0; j < 8; j++) {
						MCU_U[i][j] = round_double(res_IDCT_YUV[i][j] + 128.0);
						if (MCU_U[i][j] < 0x00)		MCU_U[i][j] = 0x00;
						if (MCU_U[i][j] > 0xFF)		MCU_U[i][j] = 0xFF;
					}
				}

				//V-channel
				rd_restore_HuffBit_RLE_Mat(rle_YUV, mat_YUV, &newByte, &newBytePos, &prev_DC_V, max_first_Chromi_DC, max_first_Chromi_AC, HuffTbl_len_UV_DC, \
					HuffTbl_len_UV_AC, fp, Jheader.DHT_DC1.DC_NRcodes, Jheader.DHT_AC1.AC_NRcodes, 0);
				rd_restore_InvQuantize(res_afterInvQ_YUV, mat_YUV, Jheader.DQT1.QTable);
				Inverse_DCT(res_IDCT_YUV, res_afterInvQ_YUV);
				for (int i = 0; i < 8; i++) {
					for (int j = 0; j < 8; j++) {
						MCU_V[i][j] = round_double(res_IDCT_YUV[i][j] + 128.0);
						if (MCU_V[i][j] < 0x00)		MCU_V[i][j] = 0x00;
						if (MCU_V[i][j] > 0xFF)		MCU_V[i][j] = 0xFF;
					}
				}
#if 0
				for (u8 i = 0; i < 8; i++)		//(jpeg420)write UV to YUV_444
				{
					for (u8 j = 0; j < 8; j++)
					{
						if (yPos + i * 2 + u / 2 >= height || xPos + j * 2 + u % 2 >= width)
							break;//continue;
						yuv_U[(i/2)*8 + j/2] = (u8)MCU_U[i/2][j/2];
						yuv_V[(i/2)*8 + j/2] = (u8)MCU_V[i/2][j/2];
						
						//颜色转换
						short R=0, G=0, B=0;	//rgb888
						short u = (short)yuv_U[(i/2)*8 + j/2] - 128;
						short v = (short)yuv_V[(i/2)*8 + j/2] - 128;
						R = (short)yuv_Y[i*16 + j] + v + ((v*103)>>8);
						G = (short)yuv_Y[i*16 + j] - ((u*88)>>8) - ((v*183)>>8);
						B = (short)yuv_Y[i*16 + j] + u + ((u*198)>>8);
//						R = yuv_Y[i*16 + j] + 1.402 * ((short)yuv_V[(i/2)*8 + j/2] - 128);
//						G = yuv_Y[i*16 + j] - 0.344 * ((short)yuv_U[(i/2)*8 + j/2] - 128) - 0.714 * ((short)yuv_V[(i/2)*8 + j/2] - 128);
//						B = yuv_Y[i*16 + j] + 1.772 * ((short)yuv_U[(i/2)*8 + j/2] - 128);
						
						R = (R > 255) ? 255 : R;		R = (R < 0) ? 0 : R;
						G = (G > 255) ? 255 : G;		G = (G < 0) ? 0 : G;
						B = (B > 255) ? 255 : B;		B = (B < 0) ? 0 : B;
						
						Frame_Buffer[oy + yPos + i][ox + xPos + j] = rgb888_to_565((u8)R, (u8)G, (u8)B);
					}
				}
#endif
				for (u8 u=0; u<4; u++)
				{
					for (u8 i=0; i<8; i++)
					{
						for (u8 j=0; j<8; j++)
						{
							yuv_U[i*8 + j] = (u8)MCU_U[(i/2) + (u/2)*4][(j/2) + (u%2)];
							yuv_V[i*8 + j] = (u8)MCU_V[(i/2) + (u/2)*4][(j/2) + (u%2)];
							
							//颜色转换
							short R=0, G=0, B=0;	//rgb888
							short u = (short)yuv_U[(i/2+(u/2)*4)*8 + j/2 + u%2] - 128;
							short v = (short)yuv_V[(i/2+(u/2)*4)*8 + j/2 + u%2] - 128;
							R = (short)yuv_Y[i*16 + j] + v + ((v*103)>>8);
							G = (short)yuv_Y[i*16 + j] - ((u*88)>>8) - ((v*183)>>8);
							B = (short)yuv_Y[i*16 + j] + u + ((u*198)>>8);
//							R = yuv_Y[i*16 + j] + 1.402 * ((short)yuv_V[(i/2)*8 + j/2] - 128);
//							G = yuv_Y[i*16 + j] - 0.344 * ((short)yuv_U[(i/2)*8 + j/2] - 128) - 0.714 * ((short)yuv_V[(i/2)*8 + j/2] - 128);
//							B = yuv_Y[i*16 + j] + 1.772 * ((short)yuv_U[(i/2)*8 + j/2] - 128);
							
							R = (R > 255) ? 255 : R;		R = (R < 0) ? 0 : R;
							G = (G > 255) ? 255 : G;		G = (G < 0) ? 0 : G;
							B = (B > 255) ? 255 : B;		B = (B < 0) ? 0 : B;
							
							Frame_Buffer[oy + yPos + i + (u/2)*8][ox + xPos + j + (u%2)*8] = rgb888_to_565((u8)R, (u8)G, (u8)B);
						}
					}
				}
				
				oled_refresh();
			}
		}
	}
	else if (Jheader.clrY_sample == 0x21)					//yuv422
	{
		//yuv422
	}
	else;

	// EOI
	f_read(fp, &seg_label_t, 2, &br);
	Jheader.EOI = rd_BigEnd16(seg_label_t);

	// free all malloc space
	for (u8 m = 0; m < 17; m++) {
		if(HuffTbl_len_Y_DC[m] != NULL)		free(HuffTbl_len_Y_DC[m]);
		if(HuffTbl_len_Y_AC[m] != NULL)		free(HuffTbl_len_Y_AC[m]);
		if(HuffTbl_len_UV_DC[m] != NULL)	free(HuffTbl_len_UV_DC[m]);
		if(HuffTbl_len_UV_AC[m] != NULL)	free(HuffTbl_len_UV_AC[m]);
		
		HuffTbl_len_Y_DC[m] = NULL;
		HuffTbl_len_Y_AC[m] = NULL;
		HuffTbl_len_UV_DC[m] = NULL;
		HuffTbl_len_UV_AC[m] = NULL;
	}
	//free(rd_HuffTbl_Y_DC);
	//free(rd_HuffTbl_Y_AC);
	//free(rd_HuffTbl_UV_DC);
	//free(rd_HuffTbl_UV_AC);
//	free(Jheader.DHT_DC0.DC_Values);
//	free(Jheader.DHT_DC1.DC_Values);
//	free(Jheader.DHT_AC0.AC_Values);
//	free(Jheader.DHT_AC1.AC_Values);
	return;
}


