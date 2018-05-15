#include "bootpack.h"

#define PIT_CTRL	0x0043
#define PIT_CNT0	0x0040

struct TIMERCTL timerctl;

#define TIMER_FLAGS_ALLOC		1	
#define TIMER_FLAGS_USING		2	

/*初始化PIT*/
void init_pit(void)
{
	int i;
	struct TIMER *t;
	
	io_out8(PIT_CTRL, 0x34);
	io_out8(PIT_CNT0, 0x9c);
	io_out8(PIT_CNT0, 0x2e);
	
	timerctl.count = 0;
	for (i = 0; i < MAX_TIMER; i++) {
		timerctl.timers0[i].flags = 0; /*??未使用*/
	}
	
	t = timer_alloc(); /*申?一个??器作?哨兵*/
	t->timeout = 0xffffffff;
	t->flags = TIMER_FLAGS_USING;
	t->next = 0; 		/*没有下一个??器*/
	timerctl.t0 = t; 	/*?表的?*/
	timerctl.next = 0xffffffff; 
	
	return;
}

/*向??器管理器申?一个??器*/
struct TIMER *timer_alloc(void)
{
	int i;
	/*遍??找未被使用的??器*/
	for (i = 0; i < MAX_TIMER; i++) {
		if (timerctl.timers0[i].flags == 0) {
			timerctl.timers0[i].flags = TIMER_FLAGS_ALLOC;
			return &timerctl.timers0[i];
		}
	}
	
	return 0; /*没有找到*/
}

/*?放使用?的??器*/
void timer_free(struct TIMER *timer)
{
	timer->flags = 0; /*使用状?置0*/
	
	return;
}

/*初始化??器（?置写入的?冲区和写入的?）*/
void timer_init(struct TIMER *timer, struct FIFO32 *fifo, int data)
{
	timer->fifo = fifo;
	timer->data = data;
	
	return;
}

/*???器?定??*/
void timer_settime(struct TIMER *timer, unsigned int timeout)
{
	int e;
	struct TIMER *t, *s;
	
	timer->timeout = timeout + timerctl.count;	/*???束的?刻*/
	timer->flags = TIMER_FLAGS_USING;
	
	e = io_load_eflags();
	io_cli();
	
	t = timerctl.t0;
	if (timer->timeout <= t->timeout) {
		/*插入在?表?部*/
		timerctl.t0 = timer;
		timer->next = t; 
		timerctl.next = timer->timeout;
		
		io_store_eflags(e);
		
		return;
	}
	/*否?插入到?表中?（因?有哨兵的存在所以不会插入至末尾）*/
	while(1){
		s = t;
		t = t->next;
		/*?找插入位置*/
		if (timer->timeout <= t->timeout) {
			/*插入在*s和*t之?*/
			s->next = timer; 
			timer->next = t;
			
			io_store_eflags(e);
			return;
		}
	}
}

/*20号（??器）中断?理*/
void inthandler20(int *esp)
{
	struct TIMER *timer;
	char ts = 0;
	
	io_out8(PIC0_OCW2, 0x60);
	timerctl.count++;
	
	/*若无超???器*/
	if (timerctl.next > timerctl.count) {
		return;
	}
	
	/*有??器超?*/
	timer = timerctl.t0; 
	while(1){
		if (timer->timeout > timerctl.count) {
			break;	/*找到未超?的??器?跳出，在?之前的都是超?的??器*/
		}
		/*?超???器的?理*/
		timer->flags = TIMER_FLAGS_ALLOC;
		if (timer != task_timer) {	/*一般的??器*/
			/*向?冲区写入数据*/
			fifo32_put(timer->fifo, timer->data);
		}
		else {		/*任?切???器*/
			ts = 1; /*超??志置1*/
		}
		
		timer = timer->next; /*指向下一个??器*/
	}
	
	/*重新指定?表?*/
	timerctl.t0 = timer;	
	/*重新?定下一个超??刻*/
	timerctl.next = timer->timeout;
	
	/*当任?切???器超?*/
	if (ts != 0)
	{
		task_switch();	/*任?跳?*/
	}
	return;
}
