# vex-iotbox

物联网小盒子，基于 ESP32 的 IoT 设备系统，支持设备自组网通信、移动端控制、云端采集与数据可视化。

## 系统架构

三个模块各自独立运行，通过网络协议互联互通：

```
┌─────────────────┐         ┌─────────────────┐         ┌─────────────────┐
│                 │  MQTT   │                 │  HTTP   │                 │
│    硬件 ESP32   │◄───────►│   Java 服务端   │◄───────►│  Flutter App    │
│   (独立运行)    │         │   (独立运行)    │         │  (独立运行)     │
│                 │         │                 │         │                 │
└─────────────────┘         └─────────────────┘         └─────────────────┘
        ▲                                                   │
        │           WiFi / 蓝牙 (近场直连)                    │
        └───────────────────────────────────────────────────┘
                                     │
                                     │ JDBC
                                     ▼
                            ┌─────────────────┐
                            │    Metabase     │
                            │    报表平台     │
                            └─────────────────┘
```

- **硬件 ESP32** — 本地 MQTT 组网，主控作为网关透传至云端；可脱离云端独立运行
- **Flutter App** — 可通过蓝牙/WiFi 直连 ESP32，无需服务端参与
- **Java 服务端** — 通过云端 MQTT 采集和控制设备，提供 REST API；Metabase 接入数据库生成报表

> 本仓库仅包含硬件固件部分，服务端与 App 分别在独立仓库。

## 设备单元

| 单元 | 标识 | 目录 | 说明 |
|------|------|------|------|
| 主控 & 语音 | 0001 | `main_ctrl/` | DHT 温湿度、本地开门按键、喇叭直驱、状态屏幕、云端网关 |
| 灯光控制 | 01XX | `light_ctrl/` | 光耦继电器开关控制 |
| 门禁 | 0201 | `door_access/` | NFC 读卡、电磁锁、门铃、PIR 感应 |
| 电表采集 | 03XX | `power_meter/` | 电表数据采集（规划中） |

## GPIO 分配

### 主控 0001

| GPIO | 用途 |
|------|------|
| - | DHT 温湿度传感器 |
| - | 本机开门物理按键 |
| - | 喇叭（ESP32-S3 直驱） |

### 灯光 01XX

| GPIO | 用途 |
|------|------|
| 12 | 光耦继电器控制引脚 |
| ADC | 光敏电阻（环境光照采集） |

### 门禁 0201

| GPIO | 用途 |
|------|------|
| SPI | NFC 读卡器 |
| - | 电磁门禁锁继电器 |
| - | 门锁状态检测（上锁/解锁反馈） |
| - | 门铃触发按键 |
| - | 人体红外感应器（PIR） |

## MQTT 协议

### 主题结构

**本地 MQTT**（设备间通信）：

```
vex/{unit_id}/cmd       本地下发指令（主控/云端 → 设备）
vex/{unit_id}/status    设备上报状态（变更时 + 定时）
vex/{unit_id}/event     设备上报瞬时事件
```

**云端 MQTT**（主控 ↔ 服务端）：

```
cloud/vex/{unit_id}/status   主控透传至云端（retained）
cloud/vex/{unit_id}/event    主控透传至云端
cloud/vex/{unit_id}/cmd      云端下发指令 → 主控转发至本地
```

- `unit_id` 即设备标识：`0001`、`0101`~`0105`、`0201`、`03XX`
- 本地 `status` 主题设为 retained，新订阅者立即获取最新状态
- 所有消息体为 JSON，UTF-8 编码

### 指令（cmd）

| type | 参数 | 说明 |
|------|------|------|
| `light.on` | — | 手动开灯 |
| `light.off` | — | 手动关灯 |
| `light.auto` | `enabled`, `low`, `high` | 自动模式开关及光感阈值设置 |
| `door.unlock` | `duration: 秒` | 开门（自动回落） |
| `env.query` | — | 主控主动查询环境数据 |
| `status.get` | — | 查询任意单元当前状态 |

示例：

```json
// 开灯
{"type": "light.on"}

// 开门 5 秒
{"type": "door.unlock", "duration": 5}
```

### 状态上报（status）

| 单元 | type | 字段 |
|------|------|------|
| 灯光 | `light.status` | `on`, `auto`, `lux`, `low`, `high` |
| 门禁 | `door.status` | `locked`, `bell`, `lux` |
| 主控 | `env.status` | `temp`, `humidity` |

示例：

```json
{"type": "light.status", "on": true, "auto": true, "lux": 350, "low": 200, "high": 800}
{"type": "env.status", "temp": 25.3, "humidity": 60}
```

### 事件上报（event）

| 单元 | type | 字段 | 说明 |
|------|------|------|------|
| 门禁 | `door.card` | `uid` | NFC 刷卡 |
| 门禁 | `door.bell` | — | 门铃按下 |
| 门禁 | `door.pir` | `detected` | 人体感应 |
| 主控 | `door.open` | — | 本地按键开门 |

示例：

```json
{"type": "door.card", "uid": "AA:BB:CC:DD"}
{"type": "door.pir", "detected": true}
```

## 单元详细设计

### 主控 0001（main_ctrl）

**职责**：环境感知 + 本地交互 + 网关中继

| 功能 | 逻辑 |
|------|------|
| DHT 温湿度 | 每 30 秒采集一次，变化超过阈值（±0.5°C / ±2%）时上报 `env.status` |
| 本地开门按键 | 检测到按下后，向门禁单元发送 `door.unlock` 指令 |
| 喇叭驱动 | 接收事件后播放提示音（门铃响、刷卡成功/失败等） |
| 状态屏幕 | 订阅本地所有单元 status，实时展示温湿度、灯光开关、门锁状态 |
| 云端网关 | 订阅本地所有单元的 status/event，透传至云端 MQTT |
| 云端下发 | 接收云端 MQTT 指令，转发至本地对应单元 |

**MQTT 订阅（本地）**：
- `vex/+/status` — 监听所有单元状态，转发至云端
- `vex/+/event` — 监听所有单元事件，转发至云端 + 驱动喇叭

**MQTT 订阅（云端）**：
- `cloud/vex/+/cmd` — 接收云端指令，转发至本地对应 `vex/{unit_id}/cmd`

**本地规则**：
- 门禁单元离线时，主控仍可本地开门（按键直接触发喇叭确认音）
- 云端离线时，本地控制不受影响，恢复后补发缓存状态

---

### 灯光 01XX（light_ctrl）

**职责**：单路继电器开关 + 光感自动控制

| 功能 | 逻辑 |
|------|------|
| 继电器开关 | 接收 `light.on` / `light.off`，立即切换 GPIO 12 电平 |
| 自动模式 | 订阅门禁单元 `door.status.lux`，低于 `low` 自动开灯，高于 `high` 自动关灯 |
| 阈值设置 | 接收 `light.auto` 远程设置高低限位及开关自动模式 |
| 状态上报 | 状态变化时发布 `light.status`（retained） |
| 启动恢复 | 上电后继电器关闭，自动模式关闭，阈值从 NVS 恢复 |

**自动模式逻辑**：

```
光照 < low  → 开灯
光照 > high → 关灯
low ≤ 光照 ≤ high → 保持当前状态（迟滞区间，防抖动）
```

**多实例**：`0101`~`0105`，每个实例编译时通过 `UNIT_ID` 区分，固件相同。

---

### 门禁 0201（door_access）

**职责**：出入管控

| 功能 | 逻辑 |
|------|------|
| NFC 读卡 | 持续轮询，读到卡后上报 `door.card` 事件（携带 UID） |
| 电磁锁控制 | 收到 `door.unlock` 后吸合继电器，`duration` 秒后自动回落上锁 |
| 门锁状态 | 实时检测锁体反馈，状态变化时更新 `door.status.locked` |
| 门铃按键 | 按下即发 `door.bell` 事件 |
| PIR 人体感应 | 状态变化时上报 `door.pir` 事件，带 `detected` 字段 |

**安全策略**：
- NFC 刷卡后上报事件，由服务端鉴权后下发 `door.unlock`
- 本地不存储卡号白名单，离线时仅支持本地按键开门
- 连续刷卡失败 3 次，触发喇叭告警音

---

### 电表 03XX（power_meter）

**职责**：电表数据采集（规划中）

预留接口，后续通过 RS485/Modbus 读取电表数据，定时上报用电量。

---

## 项目结构

```
vex-iotbox/
├── platformio.ini               # 多环境配置（每个单元一个 env）
├── common/                      # 公共库（MQTT、WiFi 管理、OTA）
├── main_ctrl/                   # 主控 & 语音 (0001)
├── light_ctrl/                  # 灯光控制 (01XX)
├── door_access/                 # 门禁 (0201)
├── power_meter/                 # 电表采集 (03XX)
└── README.md
```

### 单元职责

| 单元 | 目录 | 职责 |
|------|------|------|
| 主控 0001 | `main_ctrl/` | DHT 温湿度采集、本地开门按键、喇叭驱动、云端网关透传 |
| 灯光 01XX | `light_ctrl/` | 光耦继电器开关控制、MQTT 下发执行 |
| 门禁 0201 | `door_access/` | NFC 读卡鉴权、电磁锁控制、门铃、PIR 人体感应 |
| 电表 03XX | `power_meter/` | 电表数据采集（规划中） |

## 开发环境

- 主控芯片：ESP32-S3
- 开发框架：PlatformIO + Arduino
- 通信协议：WiFi + MQTT



```shell
python -m platformio run -e hw_test -t upload --upload-port COM8  # 编译并烧录
python -m platformio device monitor -e hw_test           # 查看串口日志
```