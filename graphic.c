#include "bootpack.h"
/*初始化颜色设定*/
void init_palette(void)
{
	static unsigned char table_rgb[16 * 3] = {
		0x00, 0x00, 0x00,	/*0:黑*/
		0xff, 0x00, 0x00,	/*1:亮红*/
		0x00, 0xff, 0x00,	/*2:亮绿*/
		0xff, 0xff, 0x00,	/*3:亮黄*/
		0x00, 0x00, 0xff,	/*4:亮蓝*/
		0xff, 0x00, 0xff,	/*5:亮紫*/
		0x00, 0xff, 0xff,	/*6:浅蓝色*/
		0xff, 0xff, 0xff,	/*7:白*/
		0xc6, 0xc6, 0xc6,	/*8:亮灰*/
		0x84, 0x00, 0x00,	/*9:暗红*/
		0x00, 0x84, 0x00,	/*10:暗绿*/
		0x84, 0x84, 0x00,	/*11:暗黄*/
		0x00, 0x00, 0x84,	/*12:暗蓝*/
		0x84, 0x00, 0x84,	/*13:暗紫*/
		0x00, 0x84, 0x84,	/*14:浅暗蓝*/
		0x84, 0x84, 0x84	/*15:暗灰*/
	};
	set_palette(0, 15, table_rgb);
	return;
}

void set_palette(int start, int end, unsigned char *rgb)
{
	int i, eflags;
	/*保存EFLAGES的值，并屏蔽中断*/
	eflags = io_load_eflags();	
	io_cli(); 	
	/*颜色设定*/
	io_out8(0x03c8, start);
	for (i = start; i <= end; i++) {
		io_out8(0x03c9, rgb[0] / 4);
		io_out8(0x03c9, rgb[1] / 4);
		io_out8(0x03c9, rgb[2] / 4);
		rgb += 3;
	}
	/*恢复EFLAGES的值*/
	io_store_eflags(eflags);	
	return;
}

/*
功能：绘制一个矩形
参数：vram首地址，屏幕宽，颜色，左上点横纵坐标，右下点横纵坐标
*/
void boxfill8(unsigned char *vram, int xsize, unsigned char c, int x0, int y0, int x1, int y1)
{
	int x, y;
	for (y = y0; y <= y1; y++) {
		for (x = x0; x <= x1; x++)
			vram[y * xsize + x] = c;
	}
	return;
}

/*
初始化屏幕
参数：vram首地址，屏幕宽、高
*/
void init_screen8(char *vram, int x, int y)
{
	boxfill8(vram, x, COL8_008484,  0,     0,      x -  1, y - 29);
	boxfill8(vram, x, COL8_C6C6C6,  0,     y - 28, x -  1, y - 28);
	boxfill8(vram, x, COL8_FFFFFF,  0,     y - 27, x -  1, y - 27);
	boxfill8(vram, x, COL8_C6C6C6,  0,     y - 26, x -  1, y -  1);

	boxfill8(vram, x, COL8_FFFFFF,  3,     y - 24, 59,     y - 24);
	boxfill8(vram, x, COL8_FFFFFF,  2,     y - 24,  2,     y -  4);
	boxfill8(vram, x, COL8_848484,  3,     y -  4, 59,     y -  4);
	boxfill8(vram, x, COL8_848484, 59,     y - 23, 59,     y -  5);
	boxfill8(vram, x, COL8_000000,  2,     y -  3, 59,     y -  3);
	boxfill8(vram, x, COL8_000000, 60,     y - 24, 60,     y -  3);

	boxfill8(vram, x, COL8_848484, x - 47, y - 24, x -  4, y - 24);
	boxfill8(vram, x, COL8_848484, x - 47, y - 23, x - 47, y -  4);
	boxfill8(vram, x, COL8_FFFFFF, x - 47, y -  3, x -  4, y -  3);
	boxfill8(vram, x, COL8_FFFFFF, x -  3, y - 24, x -  3, y -  3);
	return;
}

/*
功能：用for语句把8个像素的程序循环16遍，从而显示一个字符
参数：vram首地址，屏幕宽，起始横纵坐标，颜色，要显示的字符
*/
void putfont8(char *vram, int xsize, int x, int y, char c, char *font)
{
	int i;
	char *p, d /* data */;
	for (i = 0; i < 16; i++) {
		p = vram + (y + i) * xsize + x;
		d = font[i];
		if ((d & 0x80) != 0) { p[0] = c; }
		if ((d & 0x40) != 0) { p[1] = c; }
		if ((d & 0x20) != 0) { p[2] = c; }
		if ((d & 0x10) != 0) { p[3] = c; }
		if ((d & 0x08) != 0) { p[4] = c; }
		if ((d & 0x04) != 0) { p[5] = c; }
		if ((d & 0x02) != 0) { p[6] = c; }
		if ((d & 0x01) != 0) { p[7] = c; }
	}
	return;
}

/*
功能：这个函数用来显示字符串，字符编码使用了ASCII
参数：vram首地址，屏幕宽，起始横纵坐标，颜色，要显示的字符串
*/
void putfonts8_asc(char *vram, int xsize, int x, int y, char c, unsigned char *s)
{
	extern char hankaku[4096];/*在C语言中使用hankaku字体，extern是外部数据*/
	/*依次显示每个字符*/
	for (; *s != 0x00; s++) {
		putfont8(vram, xsize, x, y, c, hankaku + *s * 16);
		x += 8;
	}
	return;
}

/*
功能：初始化鼠标指针的颜色数组
参数：数组首地址，背景颜色
*/
void init_mouse_cursor8(char *mouse, char bc)
{
	static char cursor[16][16] = {
		"**************..",
		"*OOOOOOOOOOO*...",
		"*OOOOOOOOOO*....",
		"*OOOOOOOOO*.....",
		"*OOOOOOOO*......",
		"*OOOOOOO*.......",
		"*OOOOOOO*.......",
		"*OOOOOOOO*......",
		"*OOOO**OOO*.....",
		"*OOO*..*OOO*....",
		"*OO*....*OOO*...",
		"*O*......*OOO*..",
		"**........*OOO*.",
		"*..........*OOO*",
		"............*OO*",
		".............***"
	};
	int x, y;

	for (y = 0; y < 16; y++) {
		for (x = 0; x < 16; x++) {
			if (cursor[y][x] == '*') {
				mouse[y * 16 + x] = COL8_000000;
			}
			if (cursor[y][x] == 'O') {
				mouse[y * 16 + x] = COL8_FFFFFF;
			}
			if (cursor[y][x] == '.') {
				mouse[y * 16 + x] = bc;
			}
		}
	}
	return;
}

/*
功能：显示一个矩形图形（主要来显示鼠标）
参数：vram首地址，屏幕宽，图片宽度，图片高度，图片起始横纵坐标，图片颜色数组，显示宽度
*/
void putblock8_8(char *vram, int vxsize, int pxsize,
	int pysize, int px0, int py0, char *buf, int bxsize)
{
	int x, y;
	for (y = 0; y < pysize; y++) {
		for (x = 0; x < pxsize; x++) {
			vram[(py0 + y) * vxsize + (px0 + x)] = buf[y * bxsize + x];
		}
	}
	return;
}
