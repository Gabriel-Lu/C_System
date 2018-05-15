#include "bootpack.h"

#define PIT_CTRL	0x0043
#define PIT_CNT0	0x0040

struct TIMERCTL timerctl;

#define TIMER_FLAGS_ALLOC		1	
#define TIMER_FLAGS_USING		2	

/*n»PIT*/
void init_pit(void)
{
	int i;
	struct TIMER *t;
	
	io_out8(PIT_CTRL, 0x34);
	io_out8(PIT_CNT0, 0x9c);
	io_out8(PIT_CNT0, 0x2e);
	
	timerctl.count = 0;
	for (i = 0; i < MAX_TIMER; i++) {
		timerctl.timers0[i].flags = 0; /*??’gp*/
	}
	
	t = timer_alloc(); /*\?κ’??νμ?£Ί*/
	t->timeout = 0xffffffff;
	t->flags = TIMER_FLAGS_USING;
	t->next = 0; 		/*vLΊκ’??ν*/
	timerctl.t0 = t; 	/*?\I?*/
	timerctl.next = 0xffffffff; 
	
	return;
}

/*ό??νΗν\?κ’??ν*/
struct TIMER *timer_alloc(void)
{
	int i;
	/*Υ??Q’νgpI??ν*/
	for (i = 0; i < MAX_TIMER; i++) {
		if (timerctl.timers0[i].flags == 0) {
			timerctl.timers0[i].flags = TIMER_FLAGS_ALLOC;
			return &timerctl.timers0[i];
		}
	}
	
	return 0; /*vLQ*/
}

/*?ϊgp?I??ν*/
void timer_free(struct TIMER *timer)
{
	timer->flags = 0; /*gpσ?u0*/
	
	return;
}

/*n»??νi?uΚόI?tζaΚόI?j*/
void timer_init(struct TIMER *timer, struct FIFO32 *fifo, int data)
{
	timer->fifo = fifo;
	timer->data = data;
	
	return;
}

/*???ν?θ??*/
void timer_settime(struct TIMER *timer, unsigned int timeout)
{
	int e;
	struct TIMER *t, *s;
	
	timer->timeout = timeout + timerctl.count;	/*???©I?*/
	timer->flags = TIMER_FLAGS_USING;
	
	e = io_load_eflags();
	io_cli();
	
	t = timerctl.t0;
	if (timer->timeout <= t->timeout) {
		/*όέ?\?*/
		timerctl.t0 = timer;
		timer->next = t; 
		timerctl.next = timer->timeout;
		
		io_store_eflags(e);
		
		return;
	}
	/*Ϋ?ό?\?iφ?L£ΊIΆέΘsοόφj*/
	while(1){
		s = t;
		t = t->next;
		/*?QόΚu*/
		if (timer->timeout <= t->timeout) {
			/*όέ*sa*tV?*/
			s->next = timer; 
			timer->next = t;
			
			io_store_eflags(e);
			return;
		}
	}
}

/*20i??νjf?*/
void inthandler20(int *esp)
{
	struct TIMER *timer;
	char ts = 0;
	
	io_out8(PIC0_OCW2, 0x60);
	timerctl.count++;
	
	/*αΩ΄???ν*/
	if (timerctl.next > timerctl.count) {
		return;
	}
	
	/*L??ν΄?*/
	timer = timerctl.t0; 
	while(1){
		if (timer->timeout > timerctl.count) {
			break;	/*Q’΄?I??ν?΅oCέ?VOIs₯΄?I??ν*/
		}
		/*?΄???νI?*/
		timer->flags = TIMER_FLAGS_ALLOC;
		if (timer != task_timer) {	/*κΚI??ν*/
			/*ό?tζΚό*/
			fifo32_put(timer->fifo, timer->data);
		}
		else {		/*C?Ψ???ν*/
			ts = 1; /*΄??uu1*/
		}
		
		timer = timer->next; /*wόΊκ’??ν*/
	}
	
	/*dVwθ?\?*/
	timerctl.t0 = timer;	
	/*dV?θΊκ’΄??*/
	timerctl.next = timer->timeout;
	
	/*C?Ψ???ν΄?*/
	if (ts != 0)
	{
		task_switch();	/*C?΅?*/
	}
	return;
}
