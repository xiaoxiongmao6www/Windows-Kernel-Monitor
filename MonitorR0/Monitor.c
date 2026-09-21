#include "Monitor.h"
#include "EventQueue.h"

// EVENT_QUEUE g_EventQueue;
extern EVENT_MANAGER g_EventManager;
extern MONITOR_STATISTICS g_Statistics;


BOOLEAN PushEvent(PMONITOR_EVENT Event)
{
   //判断是属于那个类型的队列
    PEVENT_QUEUE Queue = NULL;
    
    switch (Event->Type)
    {
    case EVENT_PROCESS:
        Queue = &g_EventManager.ProcessQueue;
        //防止出现竞争问题
        InterlockedIncrement(&g_Statistics.ProcessCount);
        break;

    case EVENT_THREAD:
        Queue = &g_EventManager.ThreadQueue;
        InterlockedIncrement(&g_Statistics.ThreadCount);
        break;

    case EVENT_IMAGE:
        Queue = &g_EventManager.ImageQueue;
        InterlockedIncrement(&g_Statistics.ImageCount);
        break;

    default:
        return FALSE;
    }
    //进入锁
    PIRP irp = NULL;
    KIRQL oldIrql;
    KeAcquireSpinLock(&g_EventLock, &oldIrql);
    if (g_WaitingIrp)
    {
        //把IRP从公共区域摘走从这里开始:CancelRoutine不能再拿它
        irp = g_WaitingIrp;
        g_WaitingIrp = NULL;
    }
    else 
    {
        //统计总丢失量
        if (!Enqueue(Queue, *Event)) {
            InterlockedIncrement(&g_Statistics.DroppedEvents);
            switch (Event->Type)
            {
            case EVENT_PROCESS:
                InterlockedIncrement(&g_Statistics.ProcessDropped);
                break;
            case EVENT_THREAD:
                InterlockedIncrement(&g_Statistics.ThreadDropped);
                break;
            case EVENT_IMAGE:
                InterlockedIncrement(&g_Statistics.ImageDropped);
                break;
            }
        }
        KeReleaseSpinLock(&g_EventLock, oldIrql);
        return TRUE;
    }
    KeReleaseSpinLock(&g_EventLock, oldIrql);
    if (irp) {
        //让I/O Manager 后续取消这个IRP时，不再进入你的 CancelRoutine
        //取消CancelRoutine
        KIRQL cancelIrql;
        IoAcquireCancelSpinLock(&cancelIrql);
        IoSetCancelRoutine(irp, NULL);
        IoReleaseCancelSpinLock(cancelIrql);
        PMONITOR_EVENT outEvent = (PMONITOR_EVENT)irp->AssociatedIrp.SystemBuffer;
        *outEvent = *Event;
        irp->IoStatus.Status = STATUS_SUCCESS;
        irp->IoStatus.Information = sizeof(MONITOR_EVENT);
        IoCompleteRequest(irp, IO_NO_INCREMENT);
    }
    return TRUE;
}

//irp取消处理回调函数
VOID CancelRoutine(PDEVICE_OBJECT DeviceObject,PIRP Irp)
{
    UNREFERENCED_PARAMETER(DeviceObject);
    KIRQL oldIrql;
    //CancelRoutine 是 I/O Manager 调用的特殊回调；I/O Manager调用CancelRoutine时Cancel Spin Lock已经持有所以先释放
    IoReleaseCancelSpinLock(Irp->CancelIrql);
    KeAcquireSpinLock(&g_EventLock,&oldIrql);
    if (g_WaitingIrp == Irp)
    {
        g_WaitingIrp = NULL;
    }
    // 已经被PushEvent或者Unload拿走，直接释放锁
    KeReleaseSpinLock(&g_EventLock, oldIrql);
    Irp->IoStatus.Status = STATUS_CANCELLED;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
}

