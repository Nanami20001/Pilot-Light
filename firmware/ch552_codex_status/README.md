# SL25 Rev B 最终固件

这是可烧录的应用固件：CH552G 通过 USB HID 接收电脑端状态灯报文，再驱动正面 WS2812B-V5。它包含 USB 枚举、HID OUT、红黄绿效果、USB 挂起处理和约 3 秒主机超时关灯。

## 编译

要求：SDCC。寄存器地址已按仓库内 WCH CH552 EVT 头文件核对，源码自身可独立编译。在 PowerShell 中执行：

```powershell
cd firmware/ch552_codex_status
.\Makefile.ps1
```

默认 SDCC 路径是 `E:\SDCC\bin\sdcc.exe`；其他路径可以这样指定：

```powershell
.\Makefile.ps1 -Sdcc C:\SDCC\bin\sdcc.exe
```

输出文件是 `build/sl25_codex_status.hex`。首次编译时先检查生成的 map/ihx 是否正常，再用 WCHISPTool 烧录。

## 烧录

1. 确认 5V/GND 无短路，断开外部电源。
2. 断电时临时短接 PCB 背面的 `JP1/BOOT`。
3. 插入 USB-C，电脑识别 ISP 后立即松开 `JP1`。
4. WCHISPTool 选择“8位 CH55X / CH552 / USB下载”。
5. 选择 `build/sl25_codex_status.hex`，下载并校验。
6. 拔插 USB，确认 JP1 已断开。

## USB 设备参数

当前固件使用原型 VID/PID `F055:2525`。这只适合个人原型测试；正式分发前应替换 `main.c` 顶部的 VID/PID，并同步修改主机代理启动参数。Windows 会将设备识别为 HID，不需要额外串口驱动。

## 报文和颜色

主机端发送 9 字节数组：

```text
[报告ID=0, 状态, 效果, 亮度, R, G, B, 序号, 保留]
```

固件使用后 8 字节有效载荷：

```text
[状态, 效果, 亮度, R, G, B, 序号, 保留]
```

状态：`0=离线/熄灭`、`1=工作中`、`2=等待用户`、`3=就绪`、`4=错误`。

效果：`0=常亮`、`1=呼吸`、`2=闪烁`。

映射：工作中绿色呼吸，等待用户黄色呼吸，错误红色闪烁，就绪暗绿色常亮，离线熄灭。固件按状态覆盖 RGB，避免主机发送错误颜色；亮度仍由报文控制。

## 主机程序

在项目根目录执行：

```powershell
node host/codex_status_agent.mjs --dry-run --verbose
```

确认状态无误并安装 `node-hid` 后，连接设备运行：

```powershell
cd host
npm install
node codex_status_agent.mjs --vid 0xF055 --pid 0x2525 --verbose
```

代理从 `%USERPROFILE%\.codex\thread_history_1.sqlite` 读取最近回合，每秒发送心跳；退出时发送熄灯报文。可以用 `--state-file` 测试指定状态。

## 适用范围

固件已通过 SDCC 语法和链接检查；当前没有实物板，因此 USB 枚举、WS2812 时序和 USB-C 电气性能仍需在第一块样板上验证。若枚举失败，先用 WCHISPTool 重新确认芯片及供电。
