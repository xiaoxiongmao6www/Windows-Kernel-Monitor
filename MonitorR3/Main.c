#include "Monitor.h"
HANDLE g_Device = NULL;
//事件线程运行状态
volatile BOOL g_EventRunning = TRUE;
//统计线程运行状态
volatile BOOL g_StatisticRunning = TRUE;
//打开进程监控
BOOL StartProcessMonitor(HANDLE hDevice);
//关闭进程监控
BOOL StopProcessMonitor(HANDLE hDevice);
//打开线程监控
BOOL StartThreadMonitor(HANDLE hDevice);
//关闭线程监控
BOOL StopThreadMonitor(HANDLE hDevice);
//打开模块加载监控
BOOL StartImageMonitor(HANDLE hDevice);
//关闭线程监控
BOOL StopImageMonitor(HANDLE hDevice);
//获取事件线程
DWORD WINAPI EventThread(LPVOID lpParam);
//事件统计线程
DWORD WINAPI StatisticThread(LPVOID lpParam);


int main()
{
    printf("按下Enter开启内核监控\n\n");
    getchar();
    printf("====================================\n");
    printf("          内核事件监控\n");
    printf("====================================\n\n");
    //打开驱动
    printf("[*] 打开驱动: %ls\n", SYM_NAME);
    HANDLE g_Device = CreateFileW(
        SYM_NAME,                                //要创建或打开的文件或设备的名称
        GENERIC_READ | GENERIC_WRITE,            //权限
        FILE_SHARE_READ | FILE_SHARE_WRITE,      //请求的文件或设备的共享模式
        NULL,
        OPEN_EXISTING,                           //对存在或不存在的文件或设备采取的操作。
        0,                   //文件或设备属性和标志，FILE_ATTRIBUTE_NORMAL是文件最常见的默认值。
        NULL
    );
    if (g_Device == INVALID_HANDLE_VALUE)
    {
        printf("驱动打开失败.\n");
        return 1;
    }
    printf("驱动打开成功.\n");
    //开启进程监控
    if (!StartProcessMonitor(g_Device))
    {
        CloseHandle(g_Device);
        return 1;
    }
    //开启线程监控
    if (!StartThreadMonitor(g_Device))
    {
        CloseHandle(g_Device);
        return 1;
    }
    //开启模块监控
    if (!StartImageMonitor(g_Device))
    {
        CloseHandle(g_Device);
        return 1;
    }
    // 创建事件线程
    HANDLE hEventThread =CreateThread(NULL,0,EventThread, g_Device,0,NULL);

    // 创建统计线程
    HANDLE hStatisticThread =CreateThread(NULL,0,StatisticThread, g_Device,0,NULL);

    //退出
    printf("\n按下Enter退出\n");
    getchar();

    //通知线程停止
    g_EventRunning = FALSE;
    g_StatisticRunning = FALSE;
    //取消正在等待的IOCTL_GET_EVENT_QUEUE
    CancelIoEx(g_Device,NULL);

    //等待线程结束
    WaitForSingleObject(hStatisticThread,INFINITE);
    WaitForSingleObject(hEventThread,INFINITE);
    CloseHandle(hStatisticThread);
    CloseHandle(hEventThread);

    // 停止进程监控
    StopProcessMonitor(g_Device);
    // 停止线程监控
    StopThreadMonitor(g_Device);
    // 停止模块监控
    StopImageMonitor(g_Device);
    // 关闭设备
    CloseHandle(g_Device);

    printf("\n[*] 设备关闭.\n");

    system("pause");
    return 0;
}

