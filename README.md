# Windows Kernel Monitor Driver

一个基于 Windows Kernel Driver 的系统事件监控项目。

本项目用于学习和实践 Windows Kernel、Kernel Callback、R3/R0 通信、IOCTL、IRP、Pending IRP、内核同步机制以及 Windows Driver Reverse Engineering。

![78996203724](docs/images/1789962037244.png)

![78996212560](docs/images/1789962125603.png)

## 项目简介

本项目实现了一个轻量级 Windows Kernel Monitor Driver。

驱动通过 Windows Kernel Callback 机制捕获系统关键事件，并通过自定义 Event Manager 将事件从 Kernel Mode 传递到 User Mode。

当前支持：

- Process 创建 / 退出监控
- Thread 创建 / 退出监控
- EXE / DLL / Kernel Image 加载监控


整体流程：

```
                        User Mode
                             │
                             │ DeviceIoControl
                             ▼
                  ┌─────────────────────┐
                  │     MonitorR3       │
                  │    Event Thread     │
                  │   Statistics Thread │
                  └──────────┬──────────┘
                             │
                             │ IOCTL
                             ▼
════════════════════════ Kernel Boundary ════════════════════════
                             │
                             ▼
                  ┌─────────────────────┐
                  │    Monitor Driver   │
                  │     IoDispatch      │
                  │     Event Manager   │
                  └──────────┬──────────┘
                             │
              ┌──────────────┼──────────────┐
              │              │              │
              ▼              ▼              ▼
       Process Callback Thread Callback Image Callback
              │              │              │
              └──────────────┼──────────────┘
                             ▼
                         PushEvent()
                             │
                             ▼
                  ┌─────────────────────┐
                  │    Event Queues     │
                  │   Process Queue     │
                  │    Thread Queue     │
                  │     Image Queue     │
                  └──────────┬──────────┘
                             │
                             ▼
                       Pending IRP
                             │
                             ▼
                    IoCompleteRequest
                             │
                             ▼
                         Ring3 返回

```

---

# 项目目录


```
Windows-Kernel-Monitor

MonitorDriver/
│
├── MonitorR0/
│   ├── DriverEntry.c
│   ├── IoDispatch.c
│   ├── Monitor.c
│   ├── ProcessMonitor.c
│   ├── ThreadMonitor.c
│   ├── ImageMonitor.c
│   ├── EventQueue.c
│   ├── EventQueue.h
│   ├── Monitor.h
│   └── MonitorIoctl.h
│
├── MonitorR3/
│   ├── include/
│   │   ├── MonitorIoctl.h
│   │   └── Monitor.h
│   │
│   └── src/
│       ├── Main.c
│       ├── EventThread.c
│       ├── StatisticThread.c
│       └── Print.c
│
└── docs/ 
│    └──image/
│        ├── 1789962037244
│        └── 1789962125603.png  
│ 
└── README.md
  
```

# 核心功能


## 1. Process Monitor


使用：

```c
PsSetCreateProcessNotifyRoutine()
```


监控进程创建和退出。


记录：

- Process ID
- Parent Process ID
- Process Name
- Create / Exit 状态


流程：

```
Process Create / Exit

        ↓

Process Callback

        ↓

CreateProcessMonitor()

        ↓

MONITOR_EVENT

        ↓

PushEvent()
```

---

## 2. Thread Monitor


使用：

```c
PsSetCreateThreadNotifyRoutine()
```


监控线程生命周期。


记录：

- Process ID
- Thread ID
- Create / Exit 状态


---

## 3. Image Monitor


使用：

```c
PsSetLoadImageNotifyRoutine()
```


监控：

- EXE 加载
- DLL 加载
- Kernel Module 加载


记录：

- Process ID
- Image Path
- Image Information


---

# R3 / R0 通信


驱动创建：

```
\Device\Monitor
```


并建立符号链接：

```
\??\Monitor
```


用户态：

```c
CreateFileW("\\\\.\\Monitor")
```


打开设备。


通信方式：

```c
DeviceIoControl()
```


调用链：

```
User API
    ↓
I/O Manager
    ↓
IRP_MJ_DEVICE_CONTROL
    ↓
IoDispatch()
    ↓
Driver Handler
```



---

# IOCTL设计


当前使用：

```
METHOD_BUFFERED
```


主要控制：

```
START_PROCESS_MONITOR
STOP_PROCESS_MONITOR

START_THREAD_MONITOR
STOP_THREAD_MONITOR

START_IMAGE_MONITOR
STOP_IMAGE_MONITOR

GET_EVENT_QUEUE
GET_STATISTICS
```

---

# Event Manager


由于 Kernel Callback 不适合直接进行复杂用户态通信，因此设计 Event Manager 层。


结构：


```
Process Callback ──┐
                   │
Thread Callback ───┼──→ PushEvent()
                   │
Image Callback ────┘
                         │
                         ▼
                    Event Manager
                         │
              ┌──────────┼──────────┐
              ▼          ▼          ▼
        Process Queue Thread Queue Image Queue
```

事件统一封装：


```c
typedef enum
{
    EVENT_PROCESS,
    EVENT_THREAD,
    EVENT_IMAGE

}EVENT_TYPE;
```


不同来源的 Kernel Event 最终转换成统一结构：

```c
MONITOR_EVENT
```


方便 Ring3 统一处理。

---

# Ring Buffer（环形队列）


项目使用 Ring Buffer 保存 Kernel Event。


核心字段：

```c
buffer
readIndex
writeIndex
count
capacity
```


用于处理：

- Producer / Consumer
- Event Overflow
- Event Drop


当生产速度超过消费速度时：

```
Queue Full
    ↓
Event Drop
    ↓
Statistics Record
```

---

# Kernel Synchronization（内核同步）


由于多个 Callback 可能同时产生事件：

- Process Callback
- Thread Callback
- Image Callback


因此使用：

```c
KSPIN_LOCK
```


保护共享 Event Queue。


流程：

```
Callback
   ↓
PushEvent()
   ↓
Acquire Spin Lock
   ↓
检查 Queue
   ↓
写入 Event / 获取等待 IRP
   ↓
更新 Queue State
   ↓
Release Spin Lock
```

---

# Pending IRP

当 Ring3 请求获取事件，而 Kernel Queue 暂时为空，驱动不会立即完成该 IRP，而是保存等待中的请求。等待新的 Kernel Event 到达（因此用户态不用循环轮询）


模型：

```
没有事件
    ↓
等待

有事件
    ↓
完成 IRP
    ↓
Ring3 被唤醒
```

---

# User Mode Architecture


Ring3 使用多线程结构：


```
       MonitorR3
           |
           |
     -----------------
     |               |
Event Thread   Statistic Thread
     |               |
 GET_EVENT     GET_STATISTICS

```


Event Thread：

负责：

- 等待 Kernel Event
- 获取事件
- 输出监控信息


Statistic Thread：

负责：

- 获取事件统计
- 输出 Event Count
- 输出 Drop Count


---



---

# Reverse Engineering Practice


本项目同时作为 Windows Driver Reverse Engineering 学习样本。


重点分析：

- User API → Kernel Transition

- DeviceIoControl 调用链

- IRP 创建与处理

- IOCTL 分发流程

- Ring Buffer 实现

- Kernel Synchronization

- Callback 注册机制


---



# Current Limitations


当前项目定位为学习研究项目。


限制：

- Event Queue 容量有限
- 高并发情况下可能丢失事件
- 当前主要支持单 Ring3 Client
- Cancel IRP 场景仍需进一步完善
- Driver Unload 生命周期仍需增强
- 未实现完整 EDR 行为分析


---



# Future Research


后续计划研究：


## Driver

- 多 Pending IRP
- IRP Cancel Race
- 多客户端通信
- FileObject Context
- 更完善 Event Manager


## Windows Security

- MiniFilter
- ETW
- EDR Architecture
- Kernel Object Monitoring


---

# Learning Purpose


这个项目不是为了实现一个商业安全产品。

主要目标：通过实际代码理解 Windows Kernel 内部机制，建立从 **Windows API → I/O Manager → IRP → Kernel → Callback → 内核数据结构 → 汇编** 的完整技术链路。

欢迎指出实现、设计和理解上的问题。


