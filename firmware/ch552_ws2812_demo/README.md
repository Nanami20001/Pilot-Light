# SL25 Rev B CH552G 固件

这是用于焊后硬件检查的最小测试程序：上电后让中心 WS2812B-V5 持续显示全亮白光（R=G=B=255）。LED 数据脚为 CH552G 的 P1.5（物理脚 3）。它不包含 USB HID 或电脑状态控制协议。程序已按 CH552 的 P1.5 GPIO 寄存器和 24 MHz 内部时钟配置。

该程序循环发送全白帧。以 24 MHz 指令模型计算，帧内逻辑 1 的低电平为 375 ns，低于当前 RGB 诊断采用的 580 ns 下限。首次点灯请用 `../ch552_led_diagnostic/build/sl25_led_diagnostic.hex`。全白程序供台架实验使用，满亮时应观察整机电流和温升。

## 编译

使用 SDCC 编译：

```powershell
cd firmware/ch552_ws2812_demo
.\Makefile.ps1
```

默认输出为 `build-white/sl25_ws2812_white.hex`。也可以手工把 `main.c` 放入 WCH CH55X EVT 工程构建。

## 烧录

用 WCHISPTool 选择“8位 CH55X / CH552 / USB下载”。插 USB 前临时短接 PCB 的 JP1/BOOT；电脑识别后松开 JP1，再选择生成的 HEX 下载并校验。

## Codex 状态灯协议

仓库中的循环点灯程序不包含 USB 应用协议。显示 Codex 状态请烧录 `firmware/ch552_codex_status/` 的完整固件，接收 `host/codex_status_agent.mjs` 发送的 HID 报文：

```text
[报告ID=0, 状态, 效果, 亮度, R, G, B, 序号, 保留]
```

状态值为 `0=熄灭/离线`、`1=工作中`、`2=等待用户`、`3=就绪`、`4=错误`；效果值为 `0=常亮`、`1=呼吸`、`2=闪烁`。固件应实现约 3 秒命令超时自动关灯，并在 USB 挂起或断开时关灯。

## 注意

当前代码只适合台架点灯验证。正式使用前应加入 USB 枚举、主机命令超时关灯、挂起关灯和看门狗，并用示波器确认 WS2812 时序。不要把本文件烧到 CH551 或其他型号而不重新验证引脚和启动文件。
