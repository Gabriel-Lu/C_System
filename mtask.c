#include "bootpack.h"

struct TASKCTL *taskctl;
struct TIMER *task_timer;

/*�����������е������ָ��*/
struct TASK *task_now(void)
{
	struct TASKLEVEL *tl = &taskctl->level[taskctl->now_lv];
	
	return tl->tasks[tl->now];
}

/*���һ������*/
void task_add(struct TASK *task)
{
	struct TASKLEVEL *tl = &taskctl->level[task->level];/*������ĵȼ�*/
	tl->tasks[tl->running] = task;	/*��������ĩβ*/
	tl->running++;
	task->flags = 2; 	/*��Ϊ��������*/
	
	return;
}

/*�Ƴ�һ������*/
void task_remove(struct TASK *task)
{
	int i;
	struct TASKLEVEL *tl = &taskctl->level[task->level];/*������ĵȼ�*/

	/*Ѱ�Ҹ�����*/
	for (i = 0; i < tl->running; i++) {
		if (tl->tasks[i] == task) {
			/*�ҵ����˳�*/
			break;
		}
	}

	tl->running--;	/*�������һ��*/
	if (i < tl->now) {
		tl->now--; /*����������������ġ�ָ�롱*/
	}
	if (tl->now >= tl->running) {
		/*��now�쳣������*/
		tl->now = 0;
	}
	task->flags = 1; /*��Ϊ����*/

	/*��task�����������ǰ��*/
	for (; i < tl->running; i++) {
		tl->tasks[i] = tl->tasks[i + 1];
	}

	return;
}

/*�л����ȼ���ߵ���������*/
void task_switchsub(void)
{
	int i;
	/*����Ѱ����ߵĵȼ�*/
	for (i = 0; i < MAX_TASKLEVELS; i++) {
		if (taskctl->level[i].running > 0) {
			/*�ҵ����˳�*/
			break;
		}
	}
	
	/*�޸����������*/
	taskctl->now_lv = i;	/*�������еĵȼ���Ϊi*/
	taskctl->lv_change = 0;	/*���л�����ߵȼ�*/
	
	return;
}
/*����˼·��ִ��htl*/
void task_idle(void)
{
	for (;;)
	{
		io_hlt();
	}
}

/*��ʼ���������*/
struct TASK *task_init(struct MEMMAN *memman)
{
	int i;
	struct TASK *task,*idle;
	/*ָ��GDT�׵�ַ��GDTָ��*/
	struct SEGMENT_DESCRIPTOR *gdt = (struct SEGMENT_DESCRIPTOR *) ADR_GDT;
	/*�����ڴ�*/
	taskctl = (struct TASKCTL *) memman_alloc_4k(memman, sizeof (struct TASKCTL));
	/*��ʼ����������*/
	for (i = 0; i < MAX_TASKS; i++) {
		taskctl->tasks0[i].flags = 0;	/*δʹ��*/
		taskctl->tasks0[i].sel = (TASK_GDT0 + i) * 8;	/*�κ�*/
		/*��TSS��ע����GDT��*/
		set_segmdesc(gdt + TASK_GDT0 + i, 103, (int) &taskctl->tasks0[i].tss, AR_TSS32);
	}
	/*��ʼ������ȼ�*/
	for (i = 0; i < MAX_TASKLEVELS; i++) {
		/*��������������*/
		taskctl->level[i].running = 0;	
		taskctl->level[i].now = 0;
	}
	
	/*��������������ĳ���ע��Ϊһ������*/
	idle = task_alloc();	/*����һ������*/
	idle->tss.esp = memman_alloc_4k(memman, 64 * 1024) + 64 * 1024;
	idle->tss.eip = (int)&task_idle;/*����������HTL�ŵ����²�*/
	idle->tss.es = 1 * 8;
	idle->tss.cs = 2 * 8;
	idle->tss.ss = 1 * 8;
	idle->tss.ds = 1 * 8;
	idle->tss.fs = 1 * 8;
	idle->tss.gs = 1 * 8;
	task_run(idle, MAX_TASKLEVELS - 1, 1);
	
	return task;
}

/*���������������һ������*/
struct TASK *task_alloc(void)
{
	int i;
	struct TASK *task;
	
	/*����Ѱ��δʹ�õ�����*/
	for (i = 0; i < MAX_TASKS; i++) {
		if (taskctl->tasks0[i].flags == 0) {
			task = &taskctl->tasks0[i];
			task->flags = 1; 	/*����*/
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
	return 0; /*ȫ������ʹ��*/
}

/*����һ�����񣬲��������ĵȼ������ȼ�*/
void task_run(struct TASK *task, int level, int priority)
{
	if (level < 0) {
		level = task->level; /*��������*/
	}
	
	if (priority > 0) {
		task->priority = priority;
	}

	/*�����ȼ������ȴӵ�ǰ�ȼ���ɾȥ*/
	if (task->flags == 2 && task->level != level) { 
		task_remove(task); /*flags����Ϊ1*/
	}
	/*�������*/
	if (task->flags != 2) {
		task->level = level;
		task_add(task);
	}

	taskctl->lv_change = 1; /*��ߵȼ����ܷ����˸ı�*/
	
	return;
}

/*ʹ��������*/
void task_sleep(struct TASK *task)
{
	struct TASK *now_task;
	
	if (task->flags == 2) {
		/*����������*/
		now_task = task_now();	/*ȡ�õ�ǰ�������е�����ָ��*/
		task_remove(task); 		/*�Ƴ�������*/
		if (task == now_task) {
			/*�����Լ������Լ�������Ҫ����������ת*/
			task_switchsub();		/*��ߵȼ����ܸı�*/
			now_task = task_now();  /*ȡ�õ�ǰ�������е�����ָ��*/
			farjmp(0, now_task->sel);/*��ת*/
		}
	}
	
	return;
}

/*�����л�*/
void task_switch(void)
{
	struct TASKLEVEL *tl = &taskctl->level[taskctl->now_lv];	/*��ǰ�ȼ�*/
	struct TASK *new_task, *now_task = tl->tasks[tl->now];		/*��ǰ����*/
	
	/*ָ����һ������*/
	tl->now++;
	if (tl->now == tl->running) {
		tl->now = 0;
	}
	
	if (taskctl->lv_change != 0) {
		/*������Ҫ�����������еĵȼ�*/
		task_switchsub();
		tl = &taskctl->level[taskctl->now_lv];
	}
	
	/*��ת����һ������*/
	new_task = tl->tasks[tl->now];
	timer_settime(task_timer, new_task->priority);	/*�趨��ʱ��*/
	if (new_task != now_task) {
		/*��ת*/
		farjmp(0, new_task->sel);
	}
	
	return;
}
