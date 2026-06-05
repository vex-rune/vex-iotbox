# 硬件固件（ESP32）

基于 PlatformIO + Arduino 框架开发，所有设备通过 WiFi + MQTT 互联互通。

## 设备清单

| 设备类型 | ID | 数量 | 核心功能 |
|---------|-----|------|---------|
| 主控 & 语音单元 | `0001` | 1 | 系统中控、接收子设备上报、下发指令、门铃语音播报 |
| 灯光控制单元 | `0101`~`0105` | 3~5 | 0-10V 调光、光耦继电器开关 |
| 门禁单元 | `0201` | 1 | 电磁锁、NFC 读卡、门铃按键、人体红外感应 |
| 电表采集单元 | `0301`+ | N | 电表数据采集上报 |

## 目录结构

```
hardware/
├── lib/
│   └── common/                     # 公共组件库（各设备共享）
│       ├── wifi_manager.h          # WiFi 连接管理
│       ├── mqtt_client.h           # MQTT 客户端（PubSubClient）
│       └── device_config.h         # 设备 ID / MQTT 主题定义
└── devices/
    ├── main_ctrl/                  # 主控 & 语音单元 (0001)
    │   ├── platformio.ini
    │   └── src/main.cpp
    ├── light_ctrl/                 # 灯光控制单元 (0101~0105)
    │   ├── platformio.ini
    │   └── src/main.cpp
    ├── door_access/                # 门禁单元 (0201)
    │   ├── platformio.ini
    │   └── src/main.cpp
    └── power_meter/                # 电表采集单元 (03XX)
        ├── platformio.ini
        └── src/main.cpp
```

## 平台配置说明

每个设备的 `platformio.ini` 结构相同，差异仅在 `DEVICE_ID`：

```ini
[env:esp32s3]
platform = espressif32          # ESP32 开发平台
board = esp32s3devkitc1         # 目标开发板
framework = arduino             # Arduino 框架（比 ESP-IDF 更易上手）
monitor_speed = 115200          # 串口波特率
lib_deps =                      # 第三方库（自动下载）
    knolleary/PubSubClient      # MQTT 客户端
    bblanchon/ArduinoJson       # JSON 解析
lib_extra_dirs =                # 引用公共组件库
    ../../lib
build_flags =
    -DDEVICE_ID=\"0101\"        # 本机设备 ID
```

**关键点**：`lib_extra_dirs = ../../lib` 让 PlatformIO 扫描 `hardware/lib/common/` 目录，
自动将其中的 `.h` 文件加入编译路径，无需手动配置 `-I` 参数。

## MQTT 通信协议

### 主题规范

```
上报（设备 → MQTT → 主控/后端）：dev/{设备ID}/upload
下发（主控 → MQTT → 设备）：    dev/{设备ID}/cmd
```

### 联动规则

- 门禁事件 → 上报 `dev/0001/upload` → 主控驱动小米音箱
- 主控下发开锁 → `dev/0201/cmd` → 门禁单元执行
- 主控下发调光 → `dev/01XX/cmd` → 灯光单元执行

## 开发环境搭建

### 第一步：安装 PlatformIO

VS Code 扩展商店搜索 **PlatformIO IDE**，安装即可。

首次使用会自动下载 ESP32-S3 编译工具链（约 500MB，只需一次）。

### 第二步：打开工程

VS Code 打开 `vex-iotbox.code-workspace`，左侧出现三个根文件夹：

- **硬件** → 包含 4 个设备子文件夹，每个是独立 PlatformIO 工程
- **App** → Flutter 项目
- **Server** → Java 项目

### 第三步：编译

1. 左侧资源管理器展开 **硬件** → 点击要编译的设备文件夹（如 `light_ctrl`）
2. 底部状态栏点击 **[编译]** 按钮（对勾图标）
3. 或在设备目录终端执行 `pio run`

### 第四步：烧录 & 监视

- 底部状态栏 **[上传]** 按钮（右箭头）→ 烧录到板子
- **[串口监视器]** 按钮（插头图标）→ 查看运行日志
- 快捷键：`Ctrl+Alt+U` 烧录，`Ctrl+Alt+S` 打开监视器

## 依赖库（自动安装）

| 库 | 用途 |
|---|------|
| PubSubClient | MQTT 客户端 |
| ArduinoJson | JSON 解析 |

## GPIO 分配（参考）

### 主控 0001
| GPIO | 用途 |
|------|------|
| - | DHT 温湿度传感器 |
| - | 本机开门物理按键 |
| UART | 小米智能音箱串口通信 |

### 灯光 01XX
| GPIO | 用途 |
|------|------|
| 12 | 光耦继电器控制引脚 |
| 25 | 0-10V 调光输出（可选） |

### 门禁 0201
| GPIO | 用途 |
|------|------|
| SPI | NFC 读卡器 |
| - | 电磁门禁锁继电器 |
| - | 门铃触发按键 |
| - | 人体红外感应器（PIR） |
