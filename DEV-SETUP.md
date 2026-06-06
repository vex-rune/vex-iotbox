# 开发环境配置

## ESP32-S3 开发板

- **型号**: ESP32-S3-DevKitC-1-N16R8
- **Flash**: 16MB
- **PSRAM**: 8MB OPI
- **分区表**: `huge_app.csv`

## PlatformIO 环境

- **Platform**: Espressif 32
- **Board**: `esp32-s3-devkitc-1`
- **Framework**: Arduino

## VS Code + clangd 配置

### 安装的扩展

- PlatformIO IDE (pioarduino)
- clangd (随 PlatformIO 自动安装)

### 问题

PlatformIO 内置的 clangd 找不到头文件，导致代码报红、无法 `Ctrl+Click` 跳转。

### 解决方案

#### 1. 生成 compile_commands.json

每次修改 `platformio.ini` 或新增源文件后执行：

```bash
python -m platformio run -e hw_test -t compiledb
```

生成的文件在项目根目录：`compile_commands.json`

#### 2. 配置 .vscode/settings.json

```json
{
    "C_Cpp.intelliSenseEngine": "disabled",
    "clangd.arguments": [
        "--compile-commands-dir=${workspaceFolder}",
        "--query-driver=${env:USERPROFILE}/.platformio/packages/toolchain-xtensa-esp32s3/bin/xtensa-esp32s3-elf-*"
    ]
}
```

- `C_Cpp.intelliSenseEngine: disabled` — 禁用 Microsoft C/C++ 扩展的 IntelliSense（避免和 clangd 冲突）
- `--compile-commands-dir` — 指向 compile_commands.json 所在目录
- `--query-driver` — 指向 ESP32 交叉编译器，让 clangd 能解析 ESP32 头文件

#### 3. 配置 .clangd（可选）

项目根目录创建 `.clangd` 文件，让 clangd 能自动找到编译数据库：

```yaml
CompileFlags:
  CompilationDatabase: .
```

#### 4. 重启 Language Server

按 `Ctrl+Shift+P` → 输入 `clangd` → 选择 `Restart Language Server`

### 验证

- 代码无红色波浪线
- `Ctrl+Click` 可以跳转到定义
- 悬停显示类型信息

### 注意事项

- 每次修改 `platformio.ini` 后需要重新运行 `pio run -t compiledb`
- clangd 和 Microsoft C/C++ 扩展会冲突，必须禁用其中一个的 IntelliSense
- `xtensa-esp32s3-elf-*` 是通配符，匹配所有 ESP32-S3 交叉编译器

## ESP32-S3-N16R8 引脚限制

### PSRAM 占用引脚（GPIO33-37）

N16R8 的 8MB Octal PSRAM 固定使用 GPIO33-40。这些引脚不能用于普通 GPIO 功能：

| 引脚 | 用途 | 状态 |
|------|------|------|
| GPIO33-35 | PSRAM 数据线 | 不可用 |
| GPIO36 | PSRAM / OLED SCL | **与 WiFi 冲突** |
| GPIO37 | PSRAM / OLED SDA | **与 WiFi 冲突** |
| GPIO38-40 | PSRAM 数据线 | 不可用 |

### OLED + WiFi 冲突

**现象**：OLED 接在 GPIO36/37 时，设备在 WiFi 连接阶段崩溃（`TG1WDT_SYS_RST` 看门狗超时）。

**原因**：GPIO36/37 是 PSRAM 引脚。`Wire.begin(37, 36)` 初始化 I2C 后干扰了 PSRAM 总线，WiFi 内部使用 PSRAM 做缓冲区，访问时崩溃。

**解决方案**：OLED 必须使用其他空闲引脚，推荐 **GPIO1(SCL) + GPIO2(SDA)**。

### 推荐引脚分配

| 功能 | 推荐引脚 | 备注 |
|------|----------|------|
| OLED I2C | GPIO1(SCL) + GPIO2(SDA) | 空闲，安全 |
| LED 红/黄/绿 | GPIO15/16/17 | 已验证 |
| 按钮 1/2/3 | GPIO11/12/13 | 已验证 |
| DHT11 | GPIO4 | 已验证 |
| MAX98357A 喇叭 | GPIO20/47/21 | I2S，已验证 |
| INMP441 麦克风 | GPIO39/40/41/42 | I2S，已验证 |
| RGB LED | GPIO48 | WS2812，已验证 |
| USB Serial | GPIO43/44 | 不要占用 |
| Flash | GPIO26-32 | 不可用 |
| PSRAM | GPIO33-40 | 不可用 |
