# Pilot Light

一个把电脑端工作状态转换成实体红黄绿灯光的 USB RGB 状态信号灯项目。它面向 Codex 等本地开发助手：主机代理读取本地状态，把“工作中、等待用户、已就绪、出错、离线”发送给灯，灯通过不同节奏的闪烁、常亮和熄灭直观提示当前状态。

当前设计版本为 **SL25 Rev B**：通过 CH552G 控制一颗位于正面中心的 WS2812B-V5，使用单个直径 24 mm 的扩散灯窗显示状态。

![SL25 Rev B assembly preview](enclosure/sl25_traffic_rev_b/preview/assembly.png)

> **2026-11-25 实物进展**　SL25 Rev B 样机已完成装配并成功显示灯光。当前处于样机验证阶段。

## 项目用途

该灯不是交通信号灯，而是放在显示器旁或工作台上的“外置状态栏”：不看屏幕也能知道本地 Codex 是否正在运行、是否需要你批准/输入、是否已经完成，或是否发生错误。灯光含义如下：

| 状态 | 灯光 | 含义 |
| --- | --- | --- |
| 工作中 | 黄色常亮 | Codex 正在执行回合 |
| 等待用户 | 黄色双闪 | 需要输入、审批或任务被中断 |
| 已就绪 | 暗绿色常亮 | Codex 在线且当前空闲/已完成 |
| 出错 | 红色快闪 | 最近回合失败或状态读取异常 |
| 离线 | 熄灭 | Codex 进程不可用 |

主机程序只读取本机的 Codex 状态数据库，不读取屏幕，也不会把认证信息发送到网络。状态读取和报文映射可以用 `--dry-run` 在没有硬件时测试。

## 当前状态

- 实物样机已完成装配并点亮。外壳修正版与 PCB 对应的两孔在 CAD 中同轴，孔距为 19.6 mm。
- 仓库包含 KiCad、FreeCAD、Gerber、STEP、STL 和裁切模板。
- 固件：`firmware/ch552_codex_status/` 提供 USB HID 状态灯应用，`firmware/ch552_led_diagnostic/` 提供 RGB 循环与 GPIO 探针。
- 主机代理：`host/codex_status_agent.mjs` 已实现 Codex 本地状态读取、状态映射、HID 报文发送、状态文件覆盖和 dry-run 测试。
- 自动化测试：主机代理的状态映射和报文格式测试位于 `host/codex_status_agent.test.mjs`。

## 当前设计

| 项目 | SL25 Rev B 规格 |
| --- | --- |
| PCB | 25 x 25 mm、双层、1.6 mm 厚 |
| 光源 | 正面中心 WS2812B-V5 RGB LED |
| 控制器 | CH552G，LED 数据输出为 P1.5 |
| 接口 | 背面 USB-C，USB 2.0 全速原型电路 |
| 外壳 | 30 x 30 x 40 mm |
| 灯窗 | 正面中心直径 24 mm，PET 扩散膜与透明 PC 保护片 |
| 遮光檐 | 顶部前伸 15 mm，两侧连续收短 |

该设计的光学结构从外到内依次为：黑色遮光帽、透明 PC 保护片、PET 扩散膜、泡棉压圈、白色混光腔与中心 RGB LED。白色混光腔是获得较均匀光效的必要零件，不能以黑色打印件替代。

## 仓库结构

| 路径 | 内容 |
| --- | --- |
| [`hardware/sl25_rev_b/`](hardware/sl25_rev_b/) | 当前 Rev B KiCad 工程、BOM、Gerber、贴片坐标、3D 模型与检查报告 |
| [`enclosure/sl25_traffic_rev_b/`](enclosure/sl25_traffic_rev_b/) | 当前 Rev B FreeCAD 工程、STEP、STL、光学片裁切模板与装配说明 |
| [`firmware/ch552_led_diagnostic/`](firmware/ch552_led_diagnostic/) | RGB 循环诊断和 P1.5 GPIO 探针 |
| [`firmware/ch552_ws2812_demo/`](firmware/ch552_ws2812_demo/) | 实验用全白光程序 |
| [`firmware/ch552_codex_status/`](firmware/ch552_codex_status/) | 接收 USB HID 状态报文并显示 Codex 状态的应用固件 |
| [`host/`](host/) | 读取本地 Codex 状态并发送 HID 报文的 Node.js 主机代理 |
| [`hardware/reference/`](hardware/reference/) | 器件数据手册和参考图纸 |
| [`SL25-RevB-complete.zip`](SL25-RevB-complete.zip) | Rev B 孔位修正版模型、PCB 制造文件、已编译固件、源代码和主机代理 |
| [`hardware/SL25-RevB-gerbers.zip`](hardware/SL25-RevB-gerbers.zip) | Rev B PCB 制板文件包 |
| [`enclosure/SL25-Traffic-STL-RevB.zip`](enclosure/SL25-Traffic-STL-RevB.zip) | Rev B 3D 打印和膜片裁切文件包 |

`hardware/sl25/` 与 `enclosure/sl25_traffic/` 为历史 Rev A 文件，仅供参考。Rev A 和 Rev B 的 LED、元件安装面、外壳及控制方式不兼容，不能混用。

当前工作区的源码在根目录各子文件夹中。已有的 `SL25-RevB/` 是早期解包副本，新的交付以根目录 `SL25-RevB-complete.zip` 为准。旧固件构建归档在本地 `firmware/archive/`。

## 制造与装配

1. 使用 `hardware/SL25-RevB-gerbers.zip` 下单 PCB；建议规格为 FR-4、2 层、1.6 mm、1 oz 铜。
2. 根据 [`hardware/sl25_rev_b/BOM.csv`](hardware/sl25_rev_b/BOM.csv) 和 [`MATERIALS.md`](hardware/sl25_rev_b/MATERIALS.md) 采购并装配；所有元件和 USB-C 均在 PCB 背面，只有 WS2812B-V5 位于正面。
3. 打印 Rev B 的四个外壳件。前壳和遮光帽使用黑色材料，混光腔必须使用不透光白色材料；建议 PETG、0.4 mm 喷嘴、0.16-0.20 mm 层高。
4. 按 `cut_templates/` 中 1:1 SVG 裁切直径 26 mm 的透明 PC 保护片、PET 扩散膜和泡棉环；先测量模板中的 10 x 10 mm 校准框。
5. 参考 [外壳装配说明](enclosure/sl25_traffic_rev_b/README.md) 完成装配。需要两颗 M2 x 12、头径不超过 4.0 mm 的 90 度沉头螺丝。
6. 从完整交付包取 `firmware/ch552_led_diagnostic/build/sl25_led_diagnostic.hex`，确认红、绿、蓝、熄灭依次循环，再烧录 `firmware/ch552_codex_status/build/sl25_codex_status.hex` 运行状态灯应用。焊 LED 前可用 `sl25_gpio_probe.hex` 测量 P1.5。
7. 在 Node.js 22 或更高版本下运行 `node host/codex_status_agent.mjs --dry-run --verbose` 验证状态读取。连接硬件后，在 `host/` 中执行 `npm ci`，再按 [主机代理说明](host/README.md) 启动 HID 发送。Windows 也可运行 [`启动状态灯.cmd`](启动状态灯.cmd)。

完整尺寸、材料规格、打印方向、光学叠层和拆装步骤见 [外壳 README](enclosure/sl25_traffic_rev_b/README.md)。电路、USB/LED 引脚、制造规则和检验结果见 [PCB README](hardware/sl25_rev_b/README.md)。

## 开发与重建

- PCB 使用 KiCad 10。更新设计时，先保留手工修改，再依次运行 `hardware/generate_rev_b.py`、`hardware/route_rev_b.py` 和 `hardware/finish_board.py`；这些生成流程会覆盖已有输出。
- 外壳使用 FreeCAD。修改 `enclosure/sl25_traffic_rev_b/parameters.json` 后，依次运行 `build.py`、`finish_cad.py`、`render.py` 和 `cut_templates.py`。
- 固件使用 SDCC。`ch552_codex_status` 可独立编译并输出 `build/sl25_codex_status.hex`。正式分发前应替换原型 VID/PID `F055:2525`。
- 使用 FreeCAD 的 Python 运行 `package_rev_b.py` 可重新编译固件并生成完整交付包。SDCC 路径可用 `--sdcc` 指定。包内的 `firmware/firmware_checks.json` 记录 HEX 和时序检查结果。
- 主机代理要求 Node.js 22 或更高版本；连接实物时使用 `node-hid`，无硬件时使用 `--dry-run` 或 `--state-file`。

## 验证状态与限制

样机已完成装配并点亮。仓库中的 ERC、DRC、几何干涉、HEX 校验、主固件与 RGB 诊断的指令时序模型，以及主机代理的状态映射和报文格式检查均已通过。以下项目仍待验证。

- USB 电气性能、跨主机枚举和 USB-C 兼容性
- 主机到灯的全部状态切换、断线和恢复行为
- LED 的 WS2812 时序、功耗、温升和各颜色亮度
- 扩散膜下的均匀度、色斑和黄色配比
- 不同打印件的尺寸一致性、膜材厚度公差和反复拆装耐久
- Codex 本地 SQLite 状态投影在不同版本中的字段兼容性

本项目不是通过认证的交通信号灯，也不应在安全关键或监管用途部署。

## 许可证

尚未指定许可证。在许可证补充前，请先联系仓库所有者确认使用、修改和分发权限。
