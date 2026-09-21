#include <stdio.h>
#include <Windows.h>
#include <winioctl.h>

#define DEVICE_NAME L"\\device\\Monitor" //R0
#define SYM_NAME L"\\??\\Monitor"        //R3


//进程处理IOCT
#define IOCTL_START_PROCESS_MONITOR CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_STOP_PROCESS_MONITOR CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)
//进程处理IOCT
#define IOCTL_START_THREAD_MONITOR CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_STOP_THREAD_MONITOR CTL_CODE(FILE_DEVICE_UNKNOWN, 0x803, METHOD_BUFFERED, FILE_ANY_ACCESS)
//处理IOCT
#define IOCTL_START_IMAGE_MONITOR CTL_CODE(FILE_DEVICE_UNKNOWN, 0x804, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_STOP_IMAGE_MONITOR CTL_CODE(FILE_DEVICE_UNKNOWN, 0x805, METHOD_BUFFERED, FILE_ANY_ACCESS)
//事件处理IOCT
#define IOCTL_GET_EVENT_QUEUE CTL_CODE(FILE_DEVICE_UNKNOWN, 0x806, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_CLEAR_EVENT_QUEUE CTL_CODE(FILE_DEVICE_UNKNOWN, 0x807, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_GET_STATISTICS CTL_CODE(FILE_DEVICE_UNKNOWN, 0x808, METHOD_BUFFERED, FILE_ANY_ACCESS)

//事件结构体
typedef struct _MONITOR_EVENT
{
    ULONG ProcessId;
    ULONG ParentProcessId;
    BOOLEAN Create;
    CHAR ProcessName[16];

} MONITOR_EVENT, * PMONITOR_EVENT;
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


// 获取一个进程事件
BOOL GetProcessEvent(HANDLE hDevice, PMONITOR_EVENT Event
)
{
    DWORD bytesReturned = 0;
    ZeroMemory(Event, sizeof(MONITOR_EVENT));

    BOOL success = DeviceIoControl(hDevice, IOCTL_GET_EVENT_QUEUE, NULL, 0, Event, sizeof(MONITOR_EVENT), &bytesReturned, NULL);

    if (!success)
    {
        DWORD error = GetLastError();
        // 没有事件
        if (error == ERROR_NO_MORE_ITEMS)
        {
            return FALSE;
        }
        printf(
            "[-] IOCTL_GET_EVENT failed, error = %lu\n",
            error
        );
        return FALSE;
    }

    if (bytesReturned != sizeof(MONITOR_EVENT))
    {
        printf("[-] Invalid event size: %lu\n", bytesReturned);
        return FALSE;
    }
    return TRUE;
}

//打印
void PrintProcessEvent(PMONITOR_EVENT Event)
{
    printf("\n");
    printf("====================================\n");
    printf("ProcessId       : %lu\n", Event->ProcessId);
    printf("ParentProcessId : %lu\n", Event->ParentProcessId);
    printf("Create          : %s\n", Event->Create ? "TRUE" : "FALSE");
    printf("ProcessName     : %s\n", Event->ProcessName);
    printf("====================================\n");
}

//
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
int main()
{
    printf("====================================\n");
    printf("       Kernel Process Monitor\n");
    printf("====================================\n\n");
    //打开驱动
    printf("[*] 打开驱动: %ls\n", SYM_NAME);
    HANDLE hDevice = CreateFileW(
        SYM_NAME,                                //要创建或打开的文件或设备的名称
        GENERIC_READ | GENERIC_WRITE,            //权限
        FILE_SHARE_READ | FILE_SHARE_WRITE,      //请求的文件或设备的共享模式
        NULL,
        OPEN_EXISTING,                           //对存在或不存在的文件或设备采取的操作。
        0,                   //文件或设备属性和标志，FILE_ATTRIBUTE_NORMAL是文件最常见的默认值。
        NULL
    );
    if (hDevice == INVALID_HANDLE_VALUE)
    {
        printf("驱动打开失败.\n");
        return 1;
    }
    printf("驱动打开成功.\n");
    //开启进程监控
    if (!StartProcessMonitor(hDevice))
    {
        CloseHandle(hDevice);
        return 1;
    }
    // 等待一下，让系统产生一些进程事件
    printf("\n[*] Monitoring process events...\n");
    printf("[*] Press Enter to stop monitoring.\n\n");
    // 这里简单轮询
    while (1)
    {
        MONITOR_EVENT Event;
        if (GetProcessEvent(hDevice, &Event))
        {
            PrintProcessEvent(&Event);
        }
        // 每 100ms 检查一次
        Sleep(100);
        // 检查用户是否按下 Enter
        if (GetAsyncKeyState(VK_RETURN) & 0x8000)
        {
            break;
        }
    }
    // 停止进程监控
    StopProcessMonitor(hDevice);

    // 关闭设备
    CloseHandle(hDevice);

    printf("\n[*] 设备关闭.\n");

    system("pause");
    return 0;
}

