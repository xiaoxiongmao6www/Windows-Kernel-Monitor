#include "Monitor.h"
#include "MonitorIoctl.h"

void PrintEvent(PMONITOR_EVENT Event);

DWORD WINAPI StatisticThread(LPVOID lpParam)
{
    HANDLE hDevice = (HANDLE)lpParam;
    printf("[+] 统计事件数量的线程开始运行\n");
    while (g_StatisticRunning)
    {
        MONITOR_STATISTICS Statistics = { 0 };
        DWORD bytesReturned = 0;
        BOOL result = DeviceIoControl(hDevice, IOCTL_GET_STATISTICS, NULL, 0, &Statistics, sizeof(MONITOR_STATISTICS), &bytesReturned, NULL);
        if (result)
        {
            PrintStatisticEvent(&Statistics);

        }
        else
        {
            printf("[-] 获取统计事件数量失败:%lu\n", GetLastError());

        }
        Sleep(1000);
    }
    printf("[+] 统计事件数量结束\n");
    return 0;
}
