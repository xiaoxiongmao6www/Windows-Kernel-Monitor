#include "Monitor.h"
#include "MonitorIoctl.h"
#include "EventQueue.h"

//创建事件队列
extern EVENT_QUEUE g_EventQueue;
extern MONITOR_STATISTICS g_Statistics;
VOID CreateProcessMonitor(_In_ HANDLE ParentId,_In_ HANDLE ProcessId,_In_ BOOLEAN Create);
VOID CreateThreadMonitor(_In_ HANDLE ProcessId, _In_ HANDLE ThreadId, _In_ BOOLEAN Create);
VOID CreateImageMonitor(_In_opt_ PUNICODE_STRING FullImageName, _In_ HANDLE ProcessId, _In_ PIMAGE_INFO ImageInfo);
VOID CancelRoutine(PDEVICE_OBJECT DeviceObject, PIRP Irp);


//派遣函数具体实现
NTSTATUS IoDispatch(_In_ struct _DEVICE_OBJECT* DeviceObject, _Inout_ struct _IRP* Irp)
{
	//获取IRP数据
	PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
	//获取控制码
	ULONG code = IrpSp->Parameters.DeviceIoControl.IoControlCode;
	//R0发送的数据长度
	ULONG outLen = IrpSp->Parameters.DeviceIoControl.OutputBufferLength;
	NTSTATUS status = STATUS_SUCCESS;
	ULONG information = 0;


	if (IrpSp->MajorFunction == IRP_MJ_DEVICE_CONTROL) {
		switch (code)
		{
		//开始进程监控
		case IOCTL_START_PROCESS_MONITOR:
		{
			
			if (g_ProcessMonitorEnabled)
			{
				status = STATUS_ALREADY_REGISTERED;
				break;
			}
			status = PsSetCreateProcessNotifyRoutine(CreateProcessMonitor, FALSE);
			if (NT_SUCCESS(status))
			{
				g_ProcessMonitorEnabled = TRUE;
			}
			break;

		}
		case IOCTL_STOP_PROCESS_MONITOR:
		{
			if (!g_ProcessMonitorEnabled)
			{
				status = STATUS_NOT_FOUND;
				break;
			}
			status = PsSetCreateProcessNotifyRoutine(CreateProcessMonitor, TRUE);

			if (NT_SUCCESS(status))
			{
				g_ProcessMonitorEnabled = FALSE;
			}
			break;
		}
		//开始线程监控
		case IOCTL_START_THREAD_MONITOR:
		{

			if (g_ThreadMonitorEnabled)
			{
				status = STATUS_ALREADY_REGISTERED;
				break;
			}
			status = PsSetCreateThreadNotifyRoutine(CreateThreadMonitor);
			if (NT_SUCCESS(status))
			{
				g_ThreadMonitorEnabled = TRUE;
			}
			break;

		}
		case IOCTL_STOP_THREAD_MONITOR:
		{
			if (!g_ThreadMonitorEnabled)
			{
				status = STATUS_NOT_FOUND;
				break;
			}
			status = PsRemoveCreateThreadNotifyRoutine(CreateThreadMonitor);

			if (NT_SUCCESS(status))
			{
				g_ThreadMonitorEnabled = FALSE;
			}
			break;
		}
		//开始模块加载监控
		case IOCTL_START_IMAGE_MONITOR:
		{

			if (g_ImageMonitorEnabled)
			{
				status = STATUS_ALREADY_REGISTERED;
				break;
			}
			status = PsSetLoadImageNotifyRoutine(CreateImageMonitor);
			if (NT_SUCCESS(status))
			{
				g_ImageMonitorEnabled = TRUE;
			}
			break;

		}
		case IOCTL_STOP_IMAGE_MONITOR:
		{
			if (!g_ImageMonitorEnabled)
			{
				status = STATUS_NOT_FOUND;
				break;
			}
			status = PsRemoveLoadImageNotifyRoutine(CreateImageMonitor);

			if (NT_SUCCESS(status))
			{
				g_ImageMonitorEnabled = FALSE;
			}
			break;
		}
		//获取事件队列
		/*STATUS_SUCCESS，请求完成，得到结果
		STATUS_DEVICE_BUSY，当前无法接受这个请求，立即得到失败结果
		STATUS_PENDING，请求已经接受，但还没完成，暂时保留
		IoCompleteRequest()，把 IRP 真正完成，完成 IRPI / O 返回*/

		case IOCTL_GET_EVENT_QUEUE:
		{
			if (outLen < sizeof(MONITOR_EVENT))
			{
				status = STATUS_BUFFER_TOO_SMALL;
				break;
			}
			PMONITOR_EVENT outData = (PMONITOR_EVENT)Irp->AssociatedIrp.SystemBuffer;
			MONITOR_EVENT event = { 0 };
			//加锁
			KIRQL oldIrql;
			KeAcquireSpinLock(&g_EventLock, &oldIrql);
			// 第一优先级 Process
			if (Dequeue(&g_EventManager.ProcessQueue,&event))
			{
				goto EVENT_SUCCESS;
			}

			// 第二优先级 Image

			if (Dequeue(&g_EventManager.ImageQueue,&event))
			{
				goto EVENT_SUCCESS;
			}
			// 第三优先级 Thread
			if (Dequeue(&g_EventManager.ThreadQueue,&event))
			{
				goto EVENT_SUCCESS;
			}
			// 队列为空,当前已经存在一个等待 IRP,不允许第二个 R3 请求等待
			if (g_WaitingIrp != NULL)
			{
				KeReleaseSpinLock(&g_EventLock, oldIrql);
				status = STATUS_DEVICE_BUSY;
				break;
			}
			// 告诉 I / O Manager,这个 IRP 现在进入 Pending 状态
			IoMarkIrpPending(Irp);
			//如果I/O Manager发现取消会调用 CancelRoutine
			IoSetCancelRoutine(Irp, CancelRoutine);
			// 队列为空,并且没有等待 IRP,保存当前 IRP
			g_WaitingIrp = Irp;
			//释放锁
			KeReleaseSpinLock(&g_EventLock, oldIrql);
			return STATUS_PENDING;
		EVENT_SUCCESS:
			KeReleaseSpinLock(&g_EventLock,oldIrql);
			// 将事件复制到R3 buffer
			*outData = event;
			information = sizeof(MONITOR_EVENT);
			status = STATUS_SUCCESS;
			break;
		}
		//获取事件统计
		case IOCTL_GET_STATISTICS:
		{
			if (outLen < sizeof(MONITOR_STATISTICS))
			{
				status = STATUS_BUFFER_TOO_SMALL;
				break;
			}
			PMONITOR_STATISTICS outData =(PMONITOR_STATISTICS)Irp->AssociatedIrp.SystemBuffer;
			*outData = g_Statistics;
			information = sizeof(MONITOR_STATISTICS);
			status = STATUS_SUCCESS;
			break;
		}
		default:
		{
			status = STATUS_INVALID_DEVICE_REQUEST;
			break;
		}
		}
	}
	else
	{
		status = STATUS_INVALID_DEVICE_REQUEST;
	}

	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = information;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return status;
}

