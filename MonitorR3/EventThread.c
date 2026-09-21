#include "Monitor.h"
#include "MonitorIoctl.h"

void PrintEvent(PMONITOR_EVENT Event);

//获取事件线程
DWORD WINAPI EventThread(LPVOID lpParam)
{
    HANDLE hDevice = (HANDLE)lpParam;
    while (g_EventRunning)
    {

        MONITOR_EVENT event = { 0 };
        DWORD bytesReturned = 0;
        BOOL success = DeviceIoControl(hDevice, IOCTL_GET_EVENT_QUEUE, NULL, 0, &event, sizeof(event), &bytesReturned, NULL);
        if (!success)
        {
            DWORD error = GetLastError();
            if (error == ERROR_OPERATION_ABORTED)
            {
                printf("[*] IO 取消\n");
                break;
            }
            printf("获取事件失败:%lu\n", error);
            continue;
        }
        if (bytesReturned == sizeof(MONITOR_EVENT))
        {
            PrintEvent(&event);
        }
    }
    return 0;
}


