#include "bootpack.h"

struct TIMER *mt_timer;
int mt_tr;

/*功能：初始化mt_timer和mt_tr的值*/
void mt_init(void)
{
	mt_timer = timer_alloc();
	/* 这里没有必要使用timer_init */
	timer_settime(mt_timer, 2);
	mt_tr = 3 * 8;
	return;
}

void mt_taskswitch(void)
{
	if (mt_tr == 3 * 8) {
		mt_tr = 4 * 8;
	} else {
		mt_tr = 3 * 8;
	}
	timer_settime(mt_timer, 2); //计时器设置为0.02秒
	farjmp(0, mt_tr); //任务切换
	return;
}
