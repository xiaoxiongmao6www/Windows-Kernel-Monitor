#pragma once

#include "Monitor.h"
#define PROCESS_QUEUE_SIZE 128
#define IMAGE_QUEUE_SIZE   512
#define THREAD_QUEUE_SIZE  1024


//队列
typedef struct _EVENT_QUEUE
{
    PMONITOR_EVENT buffer;
    ULONG size;
    ULONG readIndex;
    ULONG writeIndex;
    ULONG count;
}EVENT_QUEUE,* PEVENT_QUEUE;


//将事件存储到3个不同的队列，线程优先级为1，模块优先级为2，进程优先级为3
typedef struct _EVENT_MANAGER
{
    EVENT_QUEUE ProcessQueue;
    EVENT_QUEUE ImageQueue;
    EVENT_QUEUE ThreadQueue;
}EVENT_MANAGER,* PEVENT_MANAGER;

extern EVENT_MANAGER g_EventManager;

//初始化队列
VOID QueueInit(PEVENT_QUEUE Queue, PMONITOR_EVENT Buffer, ULONG Size);
VOID EventManagerInit();
//判断队列是否为空
BOOLEAN QueueIsEmpty(_In_ PEVENT_QUEUE Queue);
//判断队列是否满
BOOLEAN QueueIsFull(_In_ PEVENT_QUEUE Queue);
//存入队列
BOOLEAN Enqueue(_Inout_ PEVENT_QUEUE Queue,_In_ MONITOR_EVENT Value);
//取出事件
BOOLEAN Dequeue(_Inout_ PEVENT_QUEUE Queue,_Out_ PMONITOR_EVENT Value);
