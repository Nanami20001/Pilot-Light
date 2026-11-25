# SL25 Rev B 最终固件

这是可烧录的应用固件：CH552G 通过 USB HID 接收电脑端状态灯报文，再驱动正面 WS2812B-V5。它包含标准 HID IN/OUT 端点、红黄绿效果、USB 挂起处理和约 3 秒主机超时关灯。

当前烧录文件为 `build/sl25_codex_status.hex`，由完整交付包重新编译，包含 WS2812 发送时序和控制端点 `SET_REPORT` 接收修复。建议先用 `../ch552_led_diagnostic/build/sl25_led_diagnostic.hex` 验证红绿蓝循环，再测试主机状态控制。详见 [`../FIRMWARE_DIAGNOSIS.md`](../FIRMWARE_DIAGNOSIS.md)。

程序启动时短暂白光自检，随后等待主机报文；未收到报文时熄灭是预期行为。下载校验后须松开 JP1/BOOT，再断电重启进入应用。

## 编译

需要 SDCC 和 `../common/ws2812_ch552.h`。在 PowerShell 中执行以下命令。

```powershell
cd firmware/ch552_codex_status
.\Makefile.ps1
```

默认 SDCC 路径是 `E:\SDCC\bin\sdcc.exe`；其他路径可以这样指定：

```powershell
.\Makefile.ps1 -Sdcc C:\SDCC\bin\sdcc.exe
```

输出文件是 `build/sl25_codex_status.hex`。首次编译时先检查生成的 map/ihx 是否正常，再用 WCHISPTool 烧录。

可用 `.\Makefile.ps1 -OutDir .\build-local` 将本次编译写到单独目录。

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

主机端通过 node-hid 发送 9 字节数组。第 1 字节是 node-hid 要求的报告 ID，设备没有定义报告 ID，因此固定为 0：

```text
[报告ID=0, 状态, 效果, 亮度, R, G, B, 序号, 保留]
```

固件使用后 8 字节有效载荷：

```text
[状态, 效果, 亮度, R, G, B, 序号, 保留]
```

状态：`0=离线/熄灭`、`1=工作中`、`2=等待用户`、`3=就绪`、`4=错误`。

效果：`0=常亮`、`1=呼吸`、`2=闪烁`。

固件按报文状态码设置 RGB，状态码 `1` 对应绿色，`2` 对应黄色。随附主机代理用 `2` 配合不同的亮度节奏显示工作中的黄灯常亮和等待时的黄灯双闪。就绪为暗绿色，错误为红色，离线熄灭。

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

固件已通过 SDCC 语法和链接检查；启动时将 CH552 时钟配置为 USB 所需的 24 MHz，并显式打开 P3.6/P3.7 的 USB 复用，HID 接口使用 EP1 中断 IN 与 EP2 中断 OUT，USB 中断标志按寄存器状态判断，以满足 Windows HID 枚举和输出报告要求。WS2812 时序和 USB-C 电气性能仍需在实物板上验证。若枚举失败，先确认 JP1 已断开、USB 为数据线并重新插拔。
