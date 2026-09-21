#include "EventQueue.h"

MONITOR_EVENT ProcessBuffer[PROCESS_QUEUE_SIZE];
MONITOR_EVENT ImageBuffer[IMAGE_QUEUE_SIZE];
MONITOR_EVENT ThreadBuffer[THREAD_QUEUE_SIZE];


VOID QueueInit(PEVENT_QUEUE Queue, PMONITOR_EVENT Buffer, ULONG Size)
{
	Queue->buffer = Buffer;
	Queue->size = Size;
	Queue->readIndex = 0;
	Queue->writeIndex = 0;
	Queue->count = 0;
	RtlZeroMemory(Buffer,sizeof(MONITOR_EVENT) * Size);
}

BOOLEAN QueueIsEmpty(PEVENT_QUEUE Queue)
{
	return Queue->count == 0;
}

//判断事件队列是否满
BOOLEAN QueueIsFull(PEVENT_QUEUE Queue)
{
	return Queue->count >= Queue->size;
}

//将事件加入事件队列
BOOLEAN Enqueue(PEVENT_QUEUE Queue, MONITOR_EVENT value)
{
	if (QueueIsFull(Queue))
	{
		return FALSE;
	}
	Queue->buffer[Queue->writeIndex] = value;
	Queue->writeIndex++;
	if (Queue->writeIndex >= Queue->size)
	{
		Queue->writeIndex = 0;
	}
	Queue->count++;
	return TRUE;
}

//将事件移除事件队列
BOOLEAN Dequeue(PEVENT_QUEUE Queue, PMONITOR_EVENT value)
{
	if (QueueIsEmpty(Queue))
	{
		return FALSE;
	}
	*value = Queue->buffer[Queue->readIndex];
	Queue->readIndex++;
	if (Queue->readIndex >= Queue->size)
	{
		Queue->readIndex = 0;
	}
	Queue->count--;
	return TRUE;
}

VOID EventManagerInit()
{
	RtlZeroMemory(&g_EventManager, sizeof(EVENT_MANAGER));
	QueueInit(&g_EventManager, ProcessBuffer, PROCESS_QUEUE_SIZE);
	QueueInit(&g_EventManager, ThreadBuffer, IMAGE_QUEUE_SIZE);
	QueueInit(&g_EventManager, ImageBuffer, IMAGE_QUEUE_SIZE);
}