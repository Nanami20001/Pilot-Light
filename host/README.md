# SL25 Codex 工作状态灯

`codex_status_agent.mjs` 在 Windows 本机读取 Codex 的本地 SQLite 状态投影，并把状态转换成 USB HID 报文。它不读取屏幕，也不把 Codex 的认证信息发送到网络。

## 红黄绿定义

| 状态 | 灯光 | 含义 |
| --- | --- | --- |
| `working` | 绿色呼吸 | Codex 有一个 `inProgress` 回合 |
| `waiting` | 黄色呼吸 | 回合被中断，或等待用户输入/审批 |
| `error` | 红色闪烁 | 最近回合 `failed`，或本地状态读取出错 |
| `ready` | 暗绿色常亮 | Codex 在线，但当前没有运行中的回合，或最近回合已完成 |
| `offline` | 熄灭 | Codex 进程不在线 |

绿色表示“正常/正在推进”，黄色表示“需要人处理”，红色表示“需要排障”。呼吸和闪烁由 `firmware/ch552_codex_status/` 中的最终固件实现。

## 测试状态读取

Node.js 22 或更高版本自带 `node:sqlite`，不需要额外安装数据库驱动：

```powershell
node host/codex_status_agent.mjs --dry-run --verbose
node host/codex_status_agent.mjs --dry-run --once
```

测试时可以用状态文件覆盖数据库状态：

```powershell
'{"status":"working"}' | Set-Content .\codex-status.json -Encoding utf8
node host/codex_status_agent.mjs --dry-run --state-file .\codex-status.json --once
```

## 连接实物

烧录 `firmware/ch552_codex_status/build/sl25_codex_status.hex` 后，在 `host` 目录安装 HID 依赖：

```powershell
cd host
npm install
node codex_status_agent.mjs --vid 0xF055 --pid 0x2525 --verbose
```

也可以用 `--path` 直接指定 HID 设备路径。node-hid 写入时发送 9 字节，其中报告 ID 为 0，设备收到 8 字节有效载荷：

```text
[报告ID=0, 状态, 效果, 亮度, R, G, B, 序号, 保留]
```

设备端应在没有收到新报文约 3 秒后自动熄灯；USB 挂起、断开和初始化时也应熄灯。不要把 `host` 目录里的 VID/PID 示例直接用于发布产品，正式 VID/PID 需要由固件阶段确定。

## 状态来源的边界

本实现读取 Codex 当前版本写入的 `~/.codex/thread_history_1.sqlite`。该文件属于本地状态投影，版本更新时字段可能变化；读取失败会显示红灯而不会误显示绿色。更长期的集成可以改为连接 Codex app-server 的事件流，再复用同一个 HID 报文格式。
