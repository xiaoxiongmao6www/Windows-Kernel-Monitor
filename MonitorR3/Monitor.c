#include "Monitor.h"
#include "MonitorIoctl.h"

//打开进程监控
BOOL StartProcessMonitor(HANDLE hDevice)
{
    DWORD bytesReturned = 0;
    BOOL success = DeviceIoControl(hDevice, IOCTL_START_PROCESS_MONITOR, NULL, 0, NULL, 0, &bytesReturned, NULL);
    if (!success)
    {
        printf("进程监控打开失败");
        return FALSE;
    }
    printf("[+] 开启进程监控\n");
    return TRUE;
}
//关闭进程监控
BOOL StopProcessMonitor(HANDLE hDevice)
{
    DWORD bytesReturned = 0;
    BOOL success = DeviceIoControl(hDevice, IOCTL_STOP_PROCESS_MONITOR, NULL, 0, NULL, 0, &bytesReturned, NULL);
    if (!success)
    {
        printf("关闭进程监控");
        return FALSE;
    }
    printf("[+] 进程监控关闭.\n");
    return TRUE;
}
//打开线程监控
BOOL StartThreadMonitor(HANDLE hDevice)
{
    DWORD bytesReturned = 0;
    BOOL success = DeviceIoControl(hDevice, IOCTL_START_THREAD_MONITOR, NULL, 0, NULL, 0, &bytesReturned, NULL);
    if (!success)
    {
        printf("线程监控打开失败");
        return FALSE;
    }
    printf("[+] 开启线程监控\n");
    return TRUE;
}
//关闭线程监控
BOOL StopThreadMonitor(HANDLE hDevice)
{
    DWORD bytesReturned = 0;
    BOOL success = DeviceIoControl(hDevice, IOCTL_STOP_THREAD_MONITOR, NULL, 0, NULL, 0, &bytesReturned, NULL);
    if (!success)
    {
        printf("关闭线程监控");
        return FALSE;
    }
    printf("[+] 线程监控关闭.\n");
    return TRUE;
}
//打开模块加载监控
BOOL StartImageMonitor(HANDLE hDevice)
{
    DWORD bytesReturned = 0;
    BOOL success = DeviceIoControl(hDevice, IOCTL_START_IMAGE_MONITOR, NULL, 0, NULL, 0, &bytesReturned, NULL);
    if (!success)
    {
        printf("模块监控打开失败");
        return FALSE;
    }
    printf("[+] 开启模块监控\n");
    return TRUE;
}
//关闭线程监控
BOOL StopImageMonitor(HANDLE hDevice)
{
    DWORD bytesReturned = 0;
    BOOL success = DeviceIoControl(hDevice, IOCTL_STOP_IMAGE_MONITOR, NULL, 0, NULL, 0, &bytesReturned, NULL);
    if (!success)
    {
        printf("关闭模块监控");
        return FALSE;
    }
    printf("[+] 模块监控关闭.\n");
    return TRUE;
}




