#pragma once
#include <stdio.h>
#include <Windows.h>
#include <winioctl.h>

#define DEVICE_NAME L"\\device\\Monitor" //R0
#define SYM_NAME L"\\??\\Monitor"        //R3

//事件的类型
typedef enum
{
    EVENT_PROCESS,
    EVENT_THREAD,
    EVENT_IMAGE
} EVENT_TYPE;

//事件
typedef struct
{
    EVENT_TYPE Type;
    ULONG ProcessId;
    ULONG ThreadId;
    ULONG ParentProcessId;
    CHAR ProcessName[64];
    WCHAR ImagePath[260];
}MONITOR_EVENT, * PMONITOR_EVENT;

//统计事件量
typedef struct
{
    ULONG ProcessCount;
    ULONG ThreadCount;
    ULONG ImageCount;
    ULONG ProcessDropped;
    ULONG ThreadDropped;
    ULONG ImageDropped;
    ULONG DroppedEvents;
}MONITOR_STATISTICS, * PMONITOR_STATISTICS;


//事件线程运行状态
extern volatile BOOL g_EventRunning;
//统计线程运行状态
extern volatile BOOL g_StatisticRunning;

