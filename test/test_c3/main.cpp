/**
 * ESP32-C3 SuperMini 综合硬件测试
 *
 * 测试项：
 *   1. 系统信息（芯片型号、CPU 频率、Flash、RAM、MAC 等）
 *   2. ADC 模拟读取（GPIO0-GPIO5，共 6 通道）
 *   3. WiFi 扫描（扫描周围热点）
 *   4. GPIO 数字读写（GPIO2 输出 / GPIO3 输入）
 *   5. Deep Sleep 省电模式（30 秒后自动唤醒）
 */

#include <Arduino.h>
#include <WiFi.h>
#include "common/wifi_manager.h"

// ── ADC 引脚定义（ESP32-C3 ADC1: GPIO0-GPIO5）──
constexpr uint8_t ADC_PINS[] = {0, 1, 2, 3, 4, 5};
constexpr uint8_t ADC_PIN_COUNT = sizeof(ADC_PINS) / sizeof(ADC_PINS[0]);

// ── GPIO 测试引脚 ──
constexpr uint8_t PIN_TEST_OUT = 2;  // 输出测试
constexpr uint8_t PIN_TEST_IN  = 3;  // 输入测试

// ═══════════════════════════════════════════════
// 1. 系统信息
// ═══════════════════════════════════════════════
void printSystemInfo() {
    Serial.println("═══════════════════════════════════════");
    Serial.println("       ESP32-C3 系统信息");
    Serial.println("═══════════════════════════════════════");

    // 芯片型号
    esp_chip_info_t chip;
    esp_chip_info(&chip);
    const char* chipModel;
    switch (chip.model) {
        case CHIP_ESP32:   chipModel = "ESP32";   break;
        case CHIP_ESP32S2: chipModel = "ESP32-S2"; break;
        case CHIP_ESP32S3: chipModel = "ESP32-S3"; break;
        case CHIP_ESP32C3: chipModel = "ESP32-C3"; break;
        case CHIP_ESP32H2: chipModel = "ESP32-H2"; break;
        default:           chipModel = "Unknown";  break;
    }
    Serial.printf("芯片型号：      %s (revision %d)\n", chipModel, chip.revision);
    Serial.printf("核心数：        %d\n", chip.cores);

    // CPU 频率
    Serial.printf("CPU 频率：      %d MHz\n", getCpuFrequencyMhz());

    // Flash 信息
    Serial.printf("Flash 大小：    %d MB\n", ESP.getFlashChipSize() / (1024 * 1024));
    Serial.printf("Flash 频率：    %d MHz\n", ESP.getFlashChipSpeed() / 1000000);

    // RAM 信息
    Serial.printf("总 Heap：       %d bytes\n", ESP.getHeapSize());
    Serial.printf("可用 Heap：     %d bytes\n", ESP.getFreeHeap());
    Serial.printf("Heap 使用率：   %.1f%%\n",
                  (1.0 - (float)ESP.getFreeHeap() / ESP.getHeapSize()) * 100);

    // PSRAM（C3 通常没有）
    if (ESP.getPsramSize() > 0) {
        Serial.printf("PSRAM 总大小：  %d bytes\n", ESP.getPsramSize());
        Serial.printf("PSRAM 可用：    %d bytes\n", ESP.getFreePsram());
    } else {
        Serial.println("PSRAM：         无");
    }

    // MAC 地址
    Serial.printf("WiFi MAC：      %s\n", WiFi.macAddress().c_str());

    // SDK 版本
    Serial.printf("SDK 版本：      %s\n", ESP.getSdkVersion());

    // Reset 原因
    esp_reset_reason_t reason = esp_reset_reason();
    const char* resetReason;
    switch (reason) {
        case ESP_RST_POWERON:  resetReason = "上电复位";   break;
        case ESP_RST_EXT:      resetReason = "外部复位";   break;
        case ESP_RST_SW:       resetReason = "软件复位";   break;
        case ESP_RST_PANIC:    resetReason = "异常崩溃";   break;
        case ESP_RST_INT_WDT:  resetReason = "看门狗超时"; break;
        case ESP_RST_TASK_WDT: resetReason = "任务看门狗"; break;
        case ESP_RST_DEEPSLEEP:resetReason = "Deep Sleep唤醒"; break;
        default:               resetReason = "未知";       break;
    }
    Serial.printf("上次复位原因：  %s\n", resetReason);
    Serial.println();
}

// ═══════════════════════════════════════════════
// 2. ADC 模拟读取
// ═══════════════════════════════════════════════
void testAdc() {
    Serial.println("═══════════════════════════════════════");
    Serial.println("       ADC 模拟读取测试");
    Serial.println("═══════════════════════════════════════");

    // ESP32-C3 ADC 分辨率默认 12 位（0-4095）
    analogReadResolution(12);

    for (uint8_t i = 0; i < ADC_PIN_COUNT; i++) {
        uint8_t pin = ADC_PINS[i];
        // 多次采样取平均，减少噪声
        uint32_t sum = 0;
        constexpr int SAMPLES = 16;
        for (int s = 0; s < SAMPLES; s++) {
            sum += analogRead(pin);
        }
        float avg = (float)sum / SAMPLES;
        float voltage = avg / 4095.0 * 3.3;  // ESP32-C3 ADC 参考电压 3.3V
        Serial.printf("GPIO%d (ADC%d)  raw=%.0f  voltage=%.2fV\n",
                      pin, i, avg, voltage);
    }
    Serial.println();
}

// ═══════════════════════════════════════════════
// 3. WiFi 扫描
// ═══════════════════════════════════════════════
void testWifiScan() {
    Serial.println("═══════════════════════════════════════");
    Serial.println("       WiFi 扫描测试");
    Serial.println("═══════════════════════════════════════");

    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);

    int n = WiFi.scanNetworks();
    Serial.printf("发现 %d 个热点：\n", n);

    for (int i = 0; i < n; i++) {
        Serial.printf("  %-32s  CH:%2d  RSSI:%d  %s\n",
                      WiFi.SSID(i).c_str(),
                      WiFi.channel(i),
                      WiFi.RSSI(i),
                      WiFi.encryptionType(i) == WIFI_AUTH_OPEN ? "[开放]" : "[加密]");
    }
    Serial.println();
}

// ═══════════════════════════════════════════════
// 4. GPIO 数字读写
// ═══════════════════════════════════════════════
void testGpio() {
    Serial.println("═══════════════════════════════════════");
    Serial.println("       GPIO 数字读写测试");
    Serial.println("═══════════════════════════════════════");

    pinMode(PIN_TEST_OUT, OUTPUT);
    pinMode(PIN_TEST_IN, INPUT_PULLUP);

    // 输出翻转测试：高 → 低 → 高
    Serial.printf("GPIO%d 输出 HIGH (3.3V)\n", PIN_TEST_OUT);
    digitalWrite(PIN_TEST_OUT, HIGH);
    delay(500);

    Serial.printf("GPIO%d 读入 = %d\n", PIN_TEST_IN, digitalRead(PIN_TEST_IN));

    Serial.printf("GPIO%d 输出 LOW (0V)\n", PIN_TEST_OUT);
    digitalWrite(PIN_TEST_OUT, LOW);
    delay(500);

    Serial.printf("GPIO%d 读入 = %d\n", PIN_TEST_IN, digitalRead(PIN_TEST_IN));

    Serial.printf("GPIO%d 输出 HIGH\n", PIN_TEST_OUT);
    digitalWrite(PIN_TEST_OUT, HIGH);

    Serial.printf("GPIO%d (BOOT 键) 读入 = %d （按下=LOW）\n", 9, digitalRead(9));
    Serial.println();
}

// ═══════════════════════════════════════════════
// 5. Deep Sleep 测试
// ═══════════════════════════════════════════════
void testDeepSleep() {
    Serial.println("═══════════════════════════════════════════");
    Serial.println("       Deep Sleep 省电测试");
    Serial.println("═══════════════════════════════════════════");
    Serial.println("30 秒后进入 Deep Sleep...");
    Serial.println("唤醒方式：按 RST 键（或拔插 USB）");
    Serial.flush();

    // 配置定时唤醒（微秒）：30 秒
    esp_sleep_enable_timer_wakeup(30ULL * 1000 * 1000);

    // 进入 Deep Sleep（芯片断电，仅 RTC 存活）
    esp_deep_sleep_start();
    // 下面的代码不会执行，唤醒后从 setup() 重新开始
}

// ═══════════════════════════════════════════════
// Arduino 入口
// ═══════════════════════════════════════════════
void setup() {
    Serial.begin(115200);

    // 等待 USB CDC 就绪
    unsigned long start = millis();
    while (!Serial && millis() - start < 3000) {
        delay(100);
    }
    delay(500);

    Serial.println("\n╔═══════════════════════════════════════╗");
    Serial.println("║  ESP32-C3 SuperMini 硬件综合测试       ║");
    Serial.println("╚═══════════════════════════════════════╝\n");

    // 1. 系统信息
    printSystemInfo();

    // 2. ADC 测试
    testAdc();

    // 3. WiFi 扫描
    testWifiScan();

    // 4. GPIO 测试
    testGpio();

    // 5. Deep Sleep 测试
    testDeepSleep();
}

void loop() {
    // Deep Sleep 唤醒后不会走到这里
    // 唤醒后会重新执行 setup()
}
