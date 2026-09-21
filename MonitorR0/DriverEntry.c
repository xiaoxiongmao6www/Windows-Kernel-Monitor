#include "Monitor.h"
#include "MonitorIoctl.h"
#include "EventQueue.h"

// 注意：真正的定义只能在一个 .c 文件出现
BOOLEAN g_ProcessMonitorEnabled = FALSE;
BOOLEAN g_ThreadMonitorEnabled = FALSE;
BOOLEAN g_ImageMonitorEnabled = FALSE;
KSPIN_LOCK g_EventLock;
PIRP g_WaitingIrp = NULL;
ULONG g_DroppedEvents = 0;
EVENT_QUEUE g_EventQueue;
//全局卸载标志
BOOLEAN g_DriverUnloading = FALSE;
MONITOR_STATISTICS g_Statistics = { 0 };
EVENT_MANAGER g_EventManager;



// 派遣函数
NTSTATUS IoCreate(_In_ PDEVICE_OBJECT DeviceObject,_Inout_ PIRP Irp);
NTSTATUS IoDispatch(_In_ PDEVICE_OBJECT DeviceObject,_Inout_ PIRP Irp);
// 进程回调
VOID CreateProcessMonitor(_In_ HANDLE ParentId,_In_ HANDLE ProcessId,_In_ BOOLEAN Create);
//线程回调
VOID CreateThreadMonitor(_In_ HANDLE ProcessId, _In_ HANDLE ThreadId, _In_ BOOLEAN Create);
//模块回调函数
VOID CreateImageMonitor(_In_opt_ PUNICODE_STRING FullImageName, _In_ HANDLE ProcessId, _In_ PIMAGE_INFO ImageInfo);

VOID DriverUnload(PDRIVER_OBJECT pDriver)
{
	//将全局卸载标志设置为TRUE
	g_DriverUnloading = TRUE;
	//如果进程回调没有被关闭则这里关闭
	if (g_ProcessMonitorEnabled)
	{
		PsSetCreateProcessNotifyRoutine(CreateProcessMonitor,TRUE);
		g_ProcessMonitorEnabled = FALSE;
	}
	//如果线程回调没有被关闭则这里关闭
	if (g_ThreadMonitorEnabled) {
		PsRemoveCreateThreadNotifyRoutine(CreateThreadMonitor);
		g_ThreadMonitorEnabled = FALSE;

	}
	//如果模块回调没有被关闭则这里关闭
	if (g_ImageMonitorEnabled) {
		PsRemoveLoadImageNotifyRoutine(CreateImageMonitor);
		g_ImageMonitorEnabled = FALSE;
	}
	//处理IRP
	PIRP irp = NULL;
	KIRQL oldIrql;
	KIRQL cancelIrql;
	KeAcquireSpinLock(&g_EventLock, &oldIrql);
	irp = g_WaitingIrp;
	g_WaitingIrp = NULL;
	
	KeReleaseSpinLock(&g_EventLock,oldIrql);
	if (irp)
	{
		IoAcquireCancelSpinLock(&cancelIrql);
		IoSetCancelRoutine(irp, NULL);
		IoReleaseCancelSpinLock(cancelIrql);
		irp->IoStatus.Status = STATUS_CANCELLED;
		IoCompleteRequest(irp, IO_NO_INCREMENT);
	}
	UNICODE_STRING symName = { 0 };
	RtlInitUnicodeString(&symName, SYM_NAME);
	IoDeleteSymbolicLink(&symName);
	IoDeleteDevice(pDriver->DeviceObject);
}


NTSTATUS IoCreate(_In_ struct _DEVICE_OBJECT* DeviceObject, _Inout_ struct _IRP* Irp)
{
	//DbgBreakPoint();
	Irp->IoStatus.Status = STATUS_SUCCESS;  //返回给3环的状态
	Irp->IoStatus.Information = 0;          //返回的数量，字节
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}



NTSTATUS DriverEntry(PDRIVER_OBJECT pDriver, PUNICODE_STRING pReg)
{
	UNREFERENCED_PARAMETER(pReg);

	// 初始化事件锁
	KeInitializeSpinLock(&g_EventLock);
	//创建设备对象名称
	UNICODE_STRING unDevice;
	RtlInitUnicodeString(&unDevice, DEVICE_NAME);

	PDEVICE_OBJECT pDevice = NULL;

	//创建符号链接名称
	UNICODE_STRING unSymName;
	RtlInitUnicodeString(&unSymName, SYM_NAME);

	//创建设备
	NTSTATUS st = IoCreateDevice(
		pDriver,
		0,
		&unDevice,
		FILE_DEVICE_UNKNOWN,  //我们创建的是虚拟设备，所以要选未知的
		FILE_DEVICE_SECURE_OPEN,
		FALSE,
		&pDevice);
	if (!NT_SUCCESS(st)) {
		DbgPrintEx(77, 0, "[db]:Device Create failed %x\r\n", st);
		return st;
	}

	//创建符号链接
	st = IoCreateSymbolicLink(&unSymName, &unDevice);//符号链接是为了3环可以知道
	if (!NT_SUCCESS(st)) {
		DbgPrintEx(77, 0, "[db]:SymbolicLink Create failed %x\r\n", st);
		IoDeleteDevice(pDevice);
		return st;
	}

	pDevice->Flags &= ~DO_DEVICE_INITIALIZING;//去掉初始化标志

	//设置数据交互方式
	pDevice->Flags |= DO_BUFFERED_IO;

	//设置派遣函数
	pDriver->MajorFunction[IRP_MJ_CREATE] = IoCreate;
	pDriver->MajorFunction[IRP_MJ_CLOSE] = IoCreate;
	pDriver->MajorFunction[IRP_MJ_DEVICE_CONTROL] = IoDispatch;
	//初始化事件队列
	EventManagerInit();
	
	//设置卸载函数
	pDriver->DriverUnload = DriverUnload;
	return STATUS_SUCCESS;
}