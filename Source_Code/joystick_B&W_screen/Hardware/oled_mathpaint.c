#include "oled.h"
#include "math.h"
///////////////////////////////////////////////////////////////////////////Plexer//
// 1.���������oled_draw_point()����������ѧ��ͼ
// 2.������ʹ��ʵ�����ص�����, column(0~127)ӳ��ΪX��, row(0-63)ӳ��ΪY��
// 3.���ǵ���ʾ�������к�, (0,0)λ�����Ͻ�, ��X������ָ��, Y������ָ��
// 4.���к�����ֻ���޸�buffer����, ��ʹ���㺯��Ҳֻ�����buffer����
// 5.���ֻ���֡���ݵ�����������������, ֻҪ����ˢ��ÿһ֡(����fps)����
//////////////////////////////////////////////////////////////////////////2020/4/20
//OLED���Դ�GRAM��ʽ����:
// [0] 0 1 2 3 ... 127
// [1] 0 1 2 3 ... 127
// [2] 0 1 2 3 ... 127
// [3] 0 1 2 3 ... 127
// [4] 0 1 2 3 ... 127
// [5] 0 1 2 3 ... 127
// [6] 0 1 2 3 ... 127
// [7] 0 1 2 3 ... 127

//��fram_buffer����GRAM�Ĺ���������������spi��DMA��ʽ, ��������Ŷ
extern u8 Frame_Buffer[128][8];


//��������    /exe=1ִ�л���  0ɾ������
//ʹ�������Ʋ���ɭ��ķ(Bresenham)ֱ���㷨�Ľ��Ʒ�
//���߳����Ǳ�����(�����˵�)
void oled_draw_line(u8 x1,u8 y1,u8 x2,u8 y2,u8 exe)
{
	u16 xerr=1,yerr=1;			//error
	u16 distance;
	int incx,incy;				//�����Լ�(��������)
	u8 nowx = x1,nowy = y1;		//��ǰ��������
	int dx = x2-x1; 			//������������
	int dy = y2-y1; 

	if(dx>0)		incx=1; 	//x�����Լ�
	else if(dx==0)	incx=0;		//x����(����ֱ��)
	else
	{ incx=-1;					//x�����Լ�
		dx = -dx; } 
	if(dy>0)		incy=1;  	//y�����Լ�
	else if(dy==0)	incy=0;		//y����(��ˮƽ��)
	else
	{	incy=-1;				//y�����Լ�
		dy = -dy; }
	
	if(dx > dy)					//���б��С��45��
		distance = dx; 			//ѡȡ��������������x(����ʱx������ƶ�һ��)
	else												//�������45��
		distance = dy;			//ѡȡ��������������y(����ʱy������ƶ�һ��)
	
	for(u8 t=0;t<=distance;t++)	//�������
	{
		oled_draw_point(nowx,nowy,exe);	//����
		xerr += dx ; 					//ÿ������xerr����dx
		yerr += dy ; 					//ÿ������yerr����dy
		if(xerr>distance) 
		{
			xerr -= distance; 
			nowx += incx; 		//����or����or�Լ�
		}
		if(yerr>distance) 
		{
			yerr -= distance; 
			nowy += incy; 		//����or����or�Լ�
		} 
	}   
}

//������		 /exe=1ִ��  0ɾ��
void oled_draw_rectangle(u8 x1,u8 y1,u8 x2,u8 y2,u8 exe)
{
	oled_draw_line(x1,y1,x2,y1,exe);
	oled_draw_line(x1,y1,x1,y2,exe);
	oled_draw_line(x1,y2,x2,y2,exe);
	oled_draw_line(x2,y1,x2,y2,exe);
}

//��Բ(Բ��,�뾶)		  /exe: 1ִ��  0ɾ��
//ʹ��Bresenham�㷨��Բ
void oled_draw_circle(u8 x0,u8 y0,u8 r,u8 exe)
{
	int a,b,p;			//pΪ�ж��¸���λ�õı�־
	a = 0; b = r;		//��һ��Բ��Ϊԭ��ʱ��Բ ��ʼ��Ϊ(0,r)
	p = 3 - (r<<1);     //�ж��¸���λ�õı�־
	for(;a<=b;a++)		//aĬ������,ֻ�迼��bʲôʱ���С
	{
		oled_draw_point(x0+a,y0+b,exe);	//1.(+a,+b)Ϊԭ��Բ�� 90~ 45���ڵ�ĳ�������
		//�Գ� �õ�����7��Բ���ϵĵ�
		oled_draw_point(x0+b,y0+a,exe);	//2.(+b,+a)Ϊԭ��Բ��  0~ 45���ڵ�ĳ�������
		oled_draw_point(x0+b,y0-a,exe);	//3.(+b,-a)Ϊԭ��Բ��360~315���ڵ�ĳ�������
		oled_draw_point(x0+a,y0-b,exe);	//4.(+a,-b)Ϊԭ��Բ��270~315���ڵ�ĳ�������
		oled_draw_point(x0-a,y0-b,exe);	//5.(-a,-b)Ϊԭ��Բ��270~225���ڵ�ĳ�������
		oled_draw_point(x0-b,y0-a,exe);	//6.(-b,-a)Ϊԭ��Բ��180~225���ڵ�ĳ�������
		oled_draw_point(x0-b,y0+a,exe);	//7.(-b,+a)Ϊԭ��Բ��180~135���ڵ�ĳ�������
		oled_draw_point(x0-a,y0+b,exe);	//8.(-a,+b)Ϊԭ��Բ�� 90~135���ڵ�ĳ�������
		
		//Bresenham�㷨: �б�ʽp���ϵ���
		if(p<0)
				p = p + 4*a + 6;	//p<0ʱp=p+4a+6,��y����
		else
		{
				p = p+4*(a-b)+10;	//p>=0,p=p+4(a-b)+10,��y�Լ�
				b--;
		}
	}									  
}



