# 当前设计：SL25 Rev B

Rev B 是中心大灯面版本：25×25 mm PCB，正面中心 WS2812B-V5，背面 USB-C；外壳 30×30×40 mm，中心 Ø24 mm 可见扩散膜灯窗。当前遮光檐顶部最长前伸 15 mm（1.5 cm），两侧渐短至安装环，檐长与灯面直径之比为 0.625。

2026-11-25，样机已完成装配并成功显示灯光。外壳修正版的安装孔距为 19.6 mm，后盖、PCB 与前壳的孔轴线已通过 CAD 检查。当前交付包括 CH552 主固件、诊断固件和电脑端状态代理。

- PCB 工程、材料与制造说明：`hardware/sl25_rev_b/README.md`
- 外壳、膜片与装配说明：`enclosure/sl25_traffic_rev_b/README.md`
- 完整交付包：`SL25-RevB-complete.zip`
- 制板文件包：`hardware/SL25-RevB-gerbers.zip`
- 打印与膜片裁切包：`enclosure/SL25-Traffic-STL-RevB.zip`
- 状态灯主固件：`firmware/ch552_codex_status/build/sl25_codex_status.hex`
- RGB 诊断与 GPIO 探针：`firmware/ch552_led_diagnostic/build/`

Rev A 保留为历史版本。两版灯珠、PCB 安装面、灯光控制方式和外壳各有差异，装配请整套使用 Rev B。全状态联调、USB 电气与兼容性、光学均匀度和长期运行仍待验证。
