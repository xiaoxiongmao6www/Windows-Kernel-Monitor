#include "Monitor.h"

//打印
void PrintStatisticEvent(PMONITOR_STATISTICS StatisticEvent)
{
    ULONG TotalEvents = StatisticEvent->ProcessCount + StatisticEvent->ThreadCount + StatisticEvent->ImageCount + StatisticEvent->DroppedEvents;
    ULONG DropRate = 0;
    DropRate = StatisticEvent->DroppedEvents * 100 / TotalEvents;
    printf("\n");
    printf("xxxxxxxxxxxxx事件统计xxxxxxxxxxxxxxxx\n");
    printf("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n");
    printf("进程数量           : %lu\n", StatisticEvent->ProcessCount);
    printf("线程数量           : %lu\n", StatisticEvent->ThreadCount);
    printf("模块数量           : %lu\n", StatisticEvent->ImageCount);
    printf("进程丢失数量       : %lu\n", StatisticEvent->ProcessDropped);
    printf("线程丢失数量       : %lu\n", StatisticEvent->ThreadDropped);
    printf("模块丢失数量       : %lu\n", StatisticEvent->ImageDropped);
    printf("事件总丢失数量     : %lu\n", StatisticEvent->DroppedEvents);
    printf("事件丢失率         : %lu%%\n", DropRate);
    printf("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n");

}

//打印
void PrintEvent(PMONITOR_EVENT Event)
{
    printf("\n");
    printf("=============事件详情===============\n");
    printf("====================================\n");
    if (Event->Type == EVENT_PROCESS) {
        printf("事件类型        : EVENT_PROCESS\n");
    }
    else if (Event->Type == EVENT_THREAD) {
        printf("事件类型        : EVENT_THREAD\n");
    }
    else if (Event->Type == EVENT_IMAGE) {
        printf("事件类型        : EVENT_IMAGE\n");
    }
    printf("进程ID          : %lu\n", Event->ProcessId);
    printf("父进程ID        : %lu\n", Event->ParentProcessId);
    printf("线程ID          : %lu\n", Event->ThreadId);
    printf("进程名          : %s\n", Event->ProcessName);

    if (Event->ImagePath[0] == L'\0')
    {
        printf("模块加载路径    : NULL\n");
    }
    else
    {
        wprintf(L"ImagePath       : %ls\n", Event->ImagePath);
    }
    printf("====================================\n");

}