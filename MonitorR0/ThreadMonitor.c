#include "Monitor.h"
#include "EventQueue.h"

// 获取进程镜像名称
PUCHAR PsGetProcessImageFileName(PEPROCESS Process);
// 统一事件分发
BOOLEAN PushEvent(PMONITOR_EVENT Event);

VOID CreateThreadMonitor(_In_ HANDLE ProcessId,_In_ HANDLE ThreadId,_In_ BOOLEAN Create)
{
	PEPROCESS Process = NULL;
	//通过进程id获取进程名
	NTSTATUS status = PsLookupProcessByProcessId(ProcessId, &Process);
	if (!NT_SUCCESS(status))
	{
		return;
	}
	//进程名
	PUCHAR ProcessName = PsGetProcessImageFileName(Process);

	//记录新产生的事件到结构体中
	MONITOR_EVENT event = { 0 };
	event.Type = EVENT_THREAD;
	event.ProcessId = HandleToULong(ProcessId);
	event.ThreadId = HandleToULong(ThreadId);
	event.ParentProcessId = 0;
	//event.Create = Create;
	if (ProcessName != NULL)
	{
		RtlCopyMemory(event.ProcessName, ProcessName, sizeof(event.ProcessName) - 1);
		event.ProcessName[sizeof(event.ProcessName) - 1] = '\0';
	}

	PushEvent(&event);
	//上面的PsLookupProcessByProcessId会给对象增加引用计数，所以这里要减少
	ObDereferenceObject(Process);
}

