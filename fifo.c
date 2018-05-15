#include "bootpack.h"

#define FLAGS_OVERRUN		0x0001

/*
���ܣ���ʼ��һ������
����������ָ�룬���д�С
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
���ܣ����һ��32λ����
����������ָ�룬�������
����ֵ�����������ʧ��ʱ����-1��������ӷ���0
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
		if (fifo->task->flags != 2) {	/*����δ����*/
			task_run(fifo->task, -1, 0); 	/*����*/
		}
	}
	
	return 0;
}

/*
���ܣ�����һ��32λ����
����������ָ��
����ֵ�����س������ݣ����ӿգ�����ʧ�ܣ�����-1��
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
���ܣ��ж϶���״̬
����������ָ��
����ֵ������0��ʾ����Ϊ��
*/
int fifo32_status(struct FIFO32 *fifo)
{
	return fifo->size - fifo->free;
}

