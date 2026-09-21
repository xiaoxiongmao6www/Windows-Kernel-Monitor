#include "Monitor.h"
#include "EventQueue.h"

// 获取进程镜像名称
PUCHAR PsGetProcessImageFileName(PEPROCESS Process);
// 统一事件分发
BOOLEAN PushEvent(PMONITOR_EVENT Event);

VOID CreateImageMonitor(_In_opt_ PUNICODE_STRING FullImageName, _In_ HANDLE ProcessId, _In_ PIMAGE_INFO ImageInfo)
{
	//如果已经卸载就不会继续执行
	if (g_DriverUnloading)
	{
		return;
	}
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
	event.Type = EVENT_IMAGE;
	event.ProcessId = HandleToULong(ProcessId);
	if (ProcessName != NULL)
	{
		RtlCopyMemory(event.ProcessName, ProcessName, sizeof(event.ProcessName) - 1);
		event.ProcessName[sizeof(event.ProcessName) - 1] = '\0';
	}
	//判断长度是否超过260字节，超过就不符合
	if (FullImageName != NULL && FullImageName->Buffer != NULL)
	{
		USHORT copyLength = FullImageName->Length;
		if (copyLength >= sizeof(event.ImagePath))
		{
			copyLength = sizeof(event.ImagePath) - sizeof(WCHAR);
		}

		RtlCopyMemory(event.ImagePath,FullImageName->Buffer,copyLength);
		event.ImagePath[copyLength / sizeof(WCHAR)] =L'\0';

	}

	PushEvent(&event);
	//上面的PsLookupProcessByProcessId会给对象增加引用计数，所以这里要减少
	ObDereferenceObject(Process);
}
