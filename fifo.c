#include "bootpack.h"

#define FLAGS_OVERRUN		0x0001

/*
功能：初始化一个队列
参数：队列指针，队列大小
*/
void fifo32_init(struct FIFO32 *fifo, int size, int *buf, struct TASK *task)
{
	fifo->size = size;
	fifo->buf = buf;
	fifo->free = size; 
	fifo->flags = 0;
	fifo->p = 0; 
	fifo->q = 0; 
	fifo->task = task;
	
	return;
}

/*
功能：入队一个32位数据
参数：队列指针，入队数据
返回值：队满，入队失败时返回-1，正常入队返回0
*/
int fifo32_put(struct FIFO32 *fifo, int data)
{
	if (fifo->free == 0) {

		fifo->flags |= FLAGS_OVERRUN;
		return -1;
	}
	fifo->buf[fifo->p] = data;
	fifo->p++;
	if (fifo->p == fifo->size) {
		fifo->p = 0;
	}
	fifo->free--;
	
	if (fifo->task != 0) {
		if (fifo->task->flags != 2) {	/*任务未运行*/
			task_run(fifo->task, -1, 0); 	/*唤醒*/
		}
	}
	
	return 0;
}

/*
功能：出队一个32位数据
参数：队列指针
返回值：返回出队数据；若队空，出队失败，返回-1；
*/
int fifo32_get(struct FIFO32 *fifo)
{
	int data;
	if (fifo->free == fifo->size) {
		return -1;
	}
	data = fifo->buf[fifo->q];
	fifo->q++;
	if (fifo->q == fifo->size) {
		fifo->q = 0;
	}
	fifo->free++;
	return data;
}

/*
功能：判断队列状态
参数：队列指针
返回值：返回0表示队列为空
*/
int fifo32_status(struct FIFO32 *fifo)
{
	return fifo->size - fifo->free;
}

