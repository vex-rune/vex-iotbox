/**
 * 主控单元 (0001) — 网关角色
 *
 * 功能：
 *   1. 开启 WiFi AP（VEX_GW），供子设备自主发现
 *   2. UDP 监听设备 HELLO，回复 ACK
 *   3. STA 连接路由器 + MQTT 云端中继
 *   4. 环境感知 + 本地交互
 */

// 引入头文件
#include <Arduino.h>
#include <WiFiUdp.h>
#include "common/wifi_manager.h"
#include "common/mqtt_manager.h"
#include "common/protocol.h"
#include "common/message.h"
#include "temp_hum.h"
#include "button.h"
#include "display.h"
#include "speaker.h"

// ═══════════════════════════════════════
// 引脚定义
// ═══════════════════════════════════════
#define PIN_DHT       4
#define PIN_BTN_DOOR  11
#define PIN_LED_RED   15
#define PIN_LED_YEL   16
#define PIN_LED_GRN   17
#define PIN_OLED_SDA  1
#define PIN_OLED_SCL  2
#define PIN_SPK_BCLK 21
#define PIN_SPK_LRC  47
#define PIN_SPK_DIN  20

// ═══════════════════════════════════════
// AP + UDP 发现配置
// ═══════════════════════════════════════
static const char* AP_SSID     = "VEX_GW";
static const char* AP_PASS     = "vex12345";
static const uint16_t UDP_PORT = 5000;
static const char* GW_ID       = "GW_001";

// ═══════════════════════════════════════
// 全局对象
// ═══════════════════════════════════════
WifiManager wifi;
MqttManager mqtt;
TempHum dht(PIN_DHT, DHT11, "env");
Button btnDoor(PIN_BTN_DOOR, "btn_door");
Display oled(PIN_OLED_SDA, PIN_OLED_SCL, 128, 32, "oled");
Speaker speaker(PIN_SPK_BCLK, PIN_SPK_LRC, PIN_SPK_DIN, "speaker");
WiFiUDP udp;

// MQTT 主题（直接写地址）
String topicCmd    = "vex/0001/cmd";
String topicStatus = "vex/0001/status";

// ═══════════════════════════════════════
// 已发现设备列表
// ═══════════════════════════════════════
struct DeviceEntry {
    char id[32];
    IPAddress ip;
    unsigned long lastSeen;
};

static DeviceEntry discoveredDevices[8];
static int deviceCount = 0;

int findOrAddDevice(const char* id, IPAddress ip) {
    for (int i = 0; i < deviceCount; i++) {
        if (strcmp(discoveredDevices[i].id, id) == 0) {
            discoveredDevices[i].ip = ip;
            discoveredDevices[i].lastSeen = millis();
            return i;
        }
    }
    if (deviceCount < 8) {
        strncpy(discoveredDevices[deviceCount].id, id, sizeof(discoveredDevices[0].id) - 1);
        discoveredDevices[deviceCount].ip = ip;
        discoveredDevices[deviceCount].lastSeen = millis();
        Serial.printf("[GW] 新设备上线: %s (IP: %s)\n", id, ip.toString().c_str());
        return deviceCount++;
    }
    return -1;
}

void printDeviceList() {
    Serial.printf("[GW] === 已发现 %d 个设备 ===\n", deviceCount);
    for (int i = 0; i < deviceCount; i++) {
        unsigned long ago = (millis() - discoveredDevices[i].lastSeen) / 1000;
        Serial.printf("  [%d] %s  IP=%s  %lus 前\n",
                      i, discoveredDevices[i].id,
                      discoveredDevices[i].ip.toString().c_str(), ago);
    }
}

/// 处理 UDP 设备发现报文
void handleDiscovery() {
    int packetSize = udp.parsePacket();
    if (packetSize <= 0) return;

    char packetBuf[256];
    int len = udp.read(packetBuf, sizeof(packetBuf) - 1);
    if (len > 0) packetBuf[len] = '\0';

    if (strncmp(packetBuf, "HELLO:", 6) == 0) {
        const char* deviceId = packetBuf + 6;
        IPAddress remoteIp = udp.remoteIP();

        Serial.printf("[GW] 收到 HELLO, 设备=%s, IP=%s\n",
                      deviceId, remoteIp.toString().c_str());

        findOrAddDevice(deviceId, remoteIp);

        // 回复 ACK
        udp.beginPacket(udp.remoteIP(), udp.remotePort());
        char ack[64];
        snprintf(ack, sizeof(ack), "ACK:%s", GW_ID);
        udp.write((const uint8_t*)ack, strlen(ack));
        udp.endPacket();

        Serial.printf("[GW] 已回复 ACK → %s\n", remoteIp.toString().c_str());
        printDeviceList();
    }
}

// ═══════════════════════════════════════
// 状态变量
// ═══════════════════════════════════════
static bool subscribed     = false;
static bool bootSoundPlayed = false;
static float lastTemp     = 0;
static float lastHumidity = 0;
static unsigned long lastReportTime = 0;

// ═══════════════════════════════════════
// MQTT 消息回调
// ═══════════════════════════════════════
void onMqttMessage(const char* topic, const char* payload, unsigned int length) {
    Message msg;
    if (!msg.parse(payload)) return;

    // 门铃事件 → 播放叮咚
    if (strcmp(msg.type(), TYPE_DOOR_BELL) == 0) {
        Serial.println("收到门铃，播放叮咚");
        speaker.playUrl("https://vex-static.oss-cn-beijing.aliyuncs.com/dingdong.mp3", 5);
    }
}

// ═══════════════════════════════════════
// 环境上报（30 秒间隔，变化超阈值才发）
// ═══════════════════════════════════════
void reportEnv() {
    if (millis() - lastReportTime < 30000) return;

    float temp = dht.getTemp();
    float hum  = dht.getHumidity();

    bool tempChanged  = abs(temp - lastTemp) >= 0.1;
    bool humChanged   = abs(hum - lastHumidity) >= 1;
    bool isFirstRead  = (lastReportTime == 0);

    if (!isFirstRead && !tempChanged && !humChanged) {
        lastReportTime = millis();
        return;
    }

    lastTemp = temp;
    lastHumidity = hum;
    lastReportTime = millis();

    Serial.printf("temp: %.1f, hum: %.0f\n", temp, hum);
    oled.showText(("T:" + String(temp, 1) + "C H:" + String(hum, 0) + "%").c_str(), 0, 16);

    Message msg(TYPE_ENV_STATUS);
    msg.set("temp", temp).set("humidity", hum);
    mqtt.publishJson(topicStatus.c_str(), msg.doc);
}

// ═══════════════════════════════════════
// setup — 上电后执行一次
// ═══════════════════════════════════════
void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n=== 主控 0001 启动（网关模式） ===");

    // ── WiFi AP_STA 模式：同时开 AP + 连路由器 ──
    WiFi.mode(WIFI_AP_STA);
    Serial.printf("[GW] 开启热点: SSID=%s\n", AP_SSID);
    if (!WiFi.softAP(AP_SSID, AP_PASS)) {
        Serial.println("[GW] AP 开启失败!");
    } else {
        Serial.printf("[GW] AP 就绪, IP: %s\n", WiFi.softAPIP().toString().c_str());
    }

    // UDP 监听
    udp.begin(UDP_PORT);
    Serial.printf("[GW] UDP 监听端口 %d\n", UDP_PORT);

    // STA 连接路由器 + MQTT
    Serial.println("连接 WiFi 路由器...");
    wifi.begin(WIFI_SSID, WIFI_PASS);
    Serial.println("连接 MQTT...");
    mqtt.begin(MQTT_HOST, MQTT_PORT, "ctrl-0001", MQTT_USER, MQTT_PASS, onMqttMessage);

    // 传感器
    dht.begin();

    // LED
    Serial.println("初始化 LED...");
    pinMode(PIN_LED_RED, OUTPUT);
    pinMode(PIN_LED_YEL, OUTPUT);
    pinMode(PIN_LED_GRN, OUTPUT);
    digitalWrite(PIN_LED_RED, HIGH);
    digitalWrite(PIN_LED_YEL, HIGH);
    digitalWrite(PIN_LED_GRN, HIGH);
    delay(500);
    digitalWrite(PIN_LED_RED, LOW);
    digitalWrite(PIN_LED_YEL, LOW);
    digitalWrite(PIN_LED_GRN, LOW);

    // 外设
    speaker.begin();
    oled.begin();
}

// ═══════════════════════════════════════
// loop — 主循环
// ═══════════════════════════════════════
void loop() {
    wifi.loop();
    mqtt.loop();

    // 处理设备 UDP 发现
    handleDiscovery();

    // MQTT 连上后订阅指令
    if (mqtt.isConnected() && !subscribed) {
        mqtt.subscribe(topicCmd.c_str());       // 本机指令
        mqtt.subscribe("vex/+/event");          // 所有单元事件（门铃、刷卡等）
        subscribed = true;
    } else if (!mqtt.isConnected()) {
        subscribed = false;
    }

    // 定时上报温湿度
    reportEnv();

    // 喇叭流播放驱动
    speaker.loopUrl();

    // 开机提示音（WiFi 连上后只播一次）
    if (!bootSoundPlayed && wifi.isConnected()) {
        bootSoundPlayed = true;
        Serial.println("WiFi 连上，播放开机提示音");

        speaker.playUrl("https://vex-static.oss-cn-beijing.aliyuncs.com/hi.mp3", 5);
        oled.showText("VEX IoTBox", 0, 16);

        Message msg("status.ready");
        mqtt.publishJson(topicStatus.c_str(), msg.doc);
        Serial.println("开机完成");
    }
}
