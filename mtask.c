#include "bootpack.h"

struct TASKCTL *taskctl;
struct TIMER *task_timer;

/*返回正在运行的任务的指针*/
struct TASK *task_now(void)
{
	struct TASKLEVEL *tl = &taskctl->level[taskctl->now_lv];
	
	return tl->tasks[tl->now];
}

/*添加一个任务*/
void task_add(struct TASK *task)
{
	struct TASKLEVEL *tl = &taskctl->level[task->level];/*该任务的等级*/
	tl->tasks[tl->running] = task;	/*插入数组末尾*/
	tl->running++;
	task->flags = 2; 	/*标为正在运行*/
	
	return;
}

/*移除一个任务*/
void task_remove(struct TASK *task)
{
	int i;
	struct TASKLEVEL *tl = &taskctl->level[task->level];/*该任务的等级*/

	/*寻找该任务*/
	for (i = 0; i < tl->running; i++) {
		if (tl->tasks[i] == task) {
			/*找到则退出*/
			break;
		}
	}

	tl->running--;	/*任务减少一个*/
	if (i < tl->now) {
		tl->now--; /*调整正在运行任务的“指针”*/
	}
	if (tl->now >= tl->running) {
		/*若now异常则修正*/
		tl->now = 0;
	}
	task->flags = 1; /*标为休眠*/

	/*将task后面的任务向前移*/
	for (; i < tl->running; i++) {
		tl->tasks[i] = tl->tasks[i + 1];
	}

	return;
}

/*切换至等级最高的任务运行*/
void task_switchsub(void)
{
	int i;
	/*遍历寻找最高的等级*/
	for (i = 0; i < MAX_TASKLEVELS; i++) {
		if (taskctl->level[i].running > 0) {
			/*找到则退出*/
			break;
		}
	}
	
	/*修改任务控制器*/
	taskctl->now_lv = i;	/*正在运行的等级置为i*/
	taskctl->lv_change = 0;	/*已切换至最高等级*/
	
	return;
}
/*卫兵思路，执行htl*/
void task_idle(void)
{
	for (;;)
	{
		io_hlt();
	}
}

/*初始任务管理器*/
struct TASK *task_init(struct MEMMAN *memman)
{
	int i;
	struct TASK *task,*idle;
	/*指向GDT首地址的GDT指针*/
	struct SEGMENT_DESCRIPTOR *gdt = (struct SEGMENT_DESCRIPTOR *) ADR_GDT;
	/*申请内存*/
	taskctl = (struct TASKCTL *) memman_alloc_4k(memman, sizeof (struct TASKCTL));
	/*初始化所有任务*/
	for (i = 0; i < MAX_TASKS; i++) {
		taskctl->tasks0[i].flags = 0;	/*未使用*/
		taskctl->tasks0[i].sel = (TASK_GDT0 + i) * 8;	/*段号*/
		/*将TSS段注册至GDT中*/
		set_segmdesc(gdt + TASK_GDT0 + i, 103, (int) &taskctl->tasks0[i].tss, AR_TSS32);
	}
	/*初始化任务等级*/
	for (i = 0; i < MAX_TASKLEVELS; i++) {
		/*无任务正在运行*/
		taskctl->level[i].running = 0;	
		taskctl->level[i].now = 0;
	}
	
	/*将调用这个函数的程序注册为一个任务*/
	idle = task_alloc();	/*申请一个任务*/
	idle->tss.esp = memman_alloc_4k(memman, 64 * 1024) + 64 * 1024;
	idle->tss.eip = (int)&task_idle;/*将闲置任务HTL放到最下层*/
	idle->tss.es = 1 * 8;
	idle->tss.cs = 2 * 8;
	idle->tss.ss = 1 * 8;
	idle->tss.ds = 1 * 8;
	idle->tss.fs = 1 * 8;
	idle->tss.gs = 1 * 8;
	task_run(idle, MAX_TASKLEVELS - 1, 1);
	
	return task;
}

/*向任务管理器申请一个任务*/
struct TASK *task_alloc(void)
{
	int i;
	struct TASK *task;
	
	/*遍历寻找未使用的任务*/
	for (i = 0; i < MAX_TASKS; i++) {
		if (taskctl->tasks0[i].flags == 0) {
			task = &taskctl->tasks0[i];
			task->flags = 1; 	/*休眠*/
			task->tss.eflags = 0x00000202; /*IF = 1*/
			task->tss.eax = 0; 
			task->tss.ecx = 0;
			task->tss.edx = 0;
			task->tss.ebx = 0;
			task->tss.ebp = 0;
			task->tss.esi = 0;
			task->tss.edi = 0;
			task->tss.es = 0;
			task->tss.ds = 0;
			task->tss.fs = 0;
			task->tss.gs = 0;
			task->tss.ldtr = 0;
			task->tss.iomap = 0x40000000;
			
			return task;
		}
	}
	return 0; /*全部都在使用*/
}

/*唤醒一个任务，并设置它的等级和优先级*/
void task_run(struct TASK *task, int level, int priority)
{
	if (level < 0) {
		level = task->level; /*不作调整*/
	}
	
	if (priority > 0) {
		task->priority = priority;
	}

	/*调整等级，首先从当前等级中删去*/
	if (task->flags == 2 && task->level != level) { 
		task_remove(task); /*flags被置为1*/
	}
	/*任务添加*/
	if (task->flags != 2) {
		task->level = level;
		task_add(task);
	}

	taskctl->lv_change = 1; /*最高等级可能发生了改变*/
	
	return;
}

/*使任务休眠*/
void task_sleep(struct TASK *task)
{
	struct TASK *now_task;
	
	if (task->flags == 2) {
		/*任务运行中*/
		now_task = task_now();	/*取得当前正在运行的任务指针*/
		task_remove(task); 		/*移除该任务*/
		if (task == now_task) {
			/*若是自己休眠自己，则需要进行任务跳转*/
			task_switchsub();		/*最高等级可能改变*/
			now_task = task_now();  /*取得当前正在运行的任务指针*/
			farjmp(0, now_task->sel);/*跳转*/
		}
	}
	
	return;
}

/*任务切换*/
void task_switch(void)
{
	struct TASKLEVEL *tl = &taskctl->level[taskctl->now_lv];	/*当前等级*/
	struct TASK *new_task, *now_task = tl->tasks[tl->now];		/*当前任务*/
	
	/*指向下一个任务*/
	tl->now++;
	if (tl->now == tl->running) {
		tl->now = 0;
	}
	
	if (taskctl->lv_change != 0) {
		/*可能需要调整正在运行的等级*/
		task_switchsub();
		tl = &taskctl->level[taskctl->now_lv];
	}
	
	/*跳转至下一个任务*/
	new_task = tl->tasks[tl->now];
	timer_settime(task_timer, new_task->priority);	/*设定计时器*/
	if (new_task != now_task) {
		/*跳转*/
		farjmp(0, new_task->sel);
	}
	
	return;
}
