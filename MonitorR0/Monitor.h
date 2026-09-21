#pragma once
#include <ntifs.h>
#include <ntddk.h>

// 设备名称
#define DEVICE_NAME L"\\Device\\Monitor"

// 用户态访问的符号链接
#define SYM_NAME L"\\??\\Monitor"



// 是否已经注册进程回调
extern BOOLEAN g_ProcessMonitorEnabled;
//是否已经注册线程回调
extern BOOLEAN g_ThreadMonitorEnabled;
//是否已经注册模块加载回调
extern BOOLEAN g_ImageMonitorEnabled;
// 保护事件队列和 WaitingIrp
extern KSPIN_LOCK g_EventLock;
// 当前正在等待事件的 IRP
extern PIRP g_WaitingIrp;
// 因为队列满而丢失的事件数量
extern ULONG g_DroppedEvents;
//全局卸载标志
extern BOOLEAN g_DriverUnloading;

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
}MONITOR_STATISTICS,* PMONITOR_STATISTICS;
