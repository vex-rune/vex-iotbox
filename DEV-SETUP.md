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
