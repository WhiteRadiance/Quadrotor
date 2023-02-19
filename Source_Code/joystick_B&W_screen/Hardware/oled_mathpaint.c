#include "oled.h"
#include "math.h"
///////////////////////////////////////////////////////////////////////////Plexer//
// 1.本程序基于oled_draw_point()函数进行数学绘图
// 2.本程序使用实际像素点坐标, column(0~127)映射为X轴, row(0-63)映射为Y轴
// 3.考虑到显示器的行列号, (0,0)位于左上角, 即X轴正向指右, Y轴正向指下
// 4.所有函数均只能修改buffer数据, 即使清零函数也只是清空buffer而已
// 5.这种缓冲帧数据的做法导致无需清屏, 只要快速刷新每一帧(类似fps)就行
//////////////////////////////////////////////////////////////////////////2020/4/20
//OLED的显存GRAM格式如下:
// [0] 0 1 2 3 ... 127
// [1] 0 1 2 3 ... 127
// [2] 0 1 2 3 ... 127
// [3] 0 1 2 3 ... 127
// [4] 0 1 2 3 ... 127
// [5] 0 1 2 3 ... 127
// [6] 0 1 2 3 ... 127
// [7] 0 1 2 3 ... 127

//把fram_buffer送入GRAM的过程甚至可以启用spi的DMA方式, 那样更快哦
extern u8 Frame_Buffer[128][8];


//两点连线    /exe=1执行画线  0删除线条
//使用了类似布雷森汉姆(Bresenham)直线算法的近似法
//画线长度是闭区间(包括端点)
void oled_draw_line(u8 x1,u8 y1,u8 x2,u8 y2,u8 exe)
{
	u16 xerr=1,yerr=1;			//error
	u16 distance;
	int incx,incy;				//自增自减(单步方向)
	u8 nowx = x1,nowy = y1;		//当前描点的坐标
	int dx = x2-x1; 			//计算坐标增量
	int dy = y2-y1; 

	if(dx>0)		incx=1; 	//x正向自加
	else if(dx==0)	incx=0;		//x不变(画垂直线)
	else
	{ incx=-1;					//x反向自减
		dx = -dx; } 
	if(dy>0)		incy=1;  	//y正向自加
	else if(dy==0)	incy=0;		//y不变(画水平线)
	else
	{	incy=-1;				//y反向自减
		dy = -dy; }
	
	if(dx > dy)					//如果斜率小于45度
		distance = dx; 			//选取基本增量坐标轴x(画线时x方向必移动一格)
	else												//如果大于45度
		distance = dy;			//选取基本增量坐标轴y(画线时y方向必移动一格)
	
	for(u8 t=0;t<=distance;t++)	//画线输出
	{
		oled_draw_point(nowx,nowy,exe);	//画点
		xerr += dx ; 					//每次运行xerr增加dx
		yerr += dy ; 					//每次运行yerr增加dy
		if(xerr>distance) 
		{
			xerr -= distance; 
			nowx += incx; 		//自增or不变or自减
		}
		if(yerr>distance) 
		{
			yerr -= distance; 
			nowy += incy; 		//自增or不变or自减
		} 
	}   
}

//画矩形		 /exe=1执行  0删除
void oled_draw_rectangle(u8 x1,u8 y1,u8 x2,u8 y2,u8 exe)
{
	oled_draw_line(x1,y1,x2,y1,exe);
	oled_draw_line(x1,y1,x1,y2,exe);
	oled_draw_line(x1,y2,x2,y2,exe);
	oled_draw_line(x2,y1,x2,y2,exe);
}

//画圆(圆心,半径)		  /exe: 1执行  0删除
//使用Bresenham算法画圆
void oled_draw_circle(u8 x0,u8 y0,u8 r,u8 exe)
{
	int a,b,p;			//p为判断下个点位置的标志
	a = 0; b = r;		//归一化圆心为原点时的圆 起始点为(0,r)
	p = 3 - (r<<1);     //判断下个点位置的标志
	for(;a<=b;a++)		//a默认自增,只需考虑b什么时候减小
	{
		oled_draw_point(x0+a,y0+b,exe);	//1.(+a,+b)为原点圆的 90~ 45度内的某点的坐标
		//对称 得到另外7个圆弧上的点
		oled_draw_point(x0+b,y0+a,exe);	//2.(+b,+a)为原点圆的  0~ 45度内的某点的坐标
		oled_draw_point(x0+b,y0-a,exe);	//3.(+b,-a)为原点圆的360~315度内的某点的坐标
		oled_draw_point(x0+a,y0-b,exe);	//4.(+a,-b)为原点圆的270~315度内的某点的坐标
		oled_draw_point(x0-a,y0-b,exe);	//5.(-a,-b)为原点圆的270~225度内的某点的坐标
		oled_draw_point(x0-b,y0-a,exe);	//6.(-b,-a)为原点圆的180~225度内的某点的坐标
		oled_draw_point(x0-b,y0+a,exe);	//7.(-b,+a)为原点圆的180~135度内的某点的坐标
		oled_draw_point(x0-a,y0+b,exe);	//8.(-a,+b)为原点圆的 90~135度内的某点的坐标
		
		//Bresenham算法: 判别式p不断递推
		if(p<0)
				p = p + 4*a + 6;	//p<0时p=p+4a+6,则y不变
		else
		{
				p = p+4*(a-b)+10;	//p>=0,p=p+4(a-b)+10,且y自减
				b--;
		}
	}									  
}



