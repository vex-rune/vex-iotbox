/**
 * 设备发现测试 (test_dev_discover)
 *
 * 功能：
 *   - 扫描周围 WiFi 热点，自主发现网关 "VEX_GW"
 *   - 连接到网关 AP
 *   - 通过 UDP 发送 HELLO 报文，等待 ACK 确认
 *
 * 协议：
 *   扫描 → 找到 SSID "VEX_GW" → 连接 → UDP HELLO → 收到 ACK
 *
 * 编译时通过 -D DEVICE_ID=\"DEV_001\" 指定设备编号
 */

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>

// ── 配置 ──
static const char* GW_SSID     = "VEX_GW";
static const char* GW_PASS     = "vex12345";
static const uint16_t UDP_PORT = 5000;

#ifndef DEVICE_ID
#define DEVICE_ID "DEV_001"
#endif

// ── 状态 ──
static bool discovered = false;
static bool connected  = false;

// ── UDP ──
WiFiUDP udp;
char respBuf[128];

/// 扫描 WiFi，查找网关 AP
bool scanForGateway() {
    Serial.printf("[DEV] 扫描 WiFi 热点...\n");
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);

    int n = WiFi.scanNetworks();
    Serial.printf("[DEV] 发现 %d 个热点:\n", n);

    for (int i = 0; i < n; i++) {
        Serial.printf("  %-32s  RSSI:%d\n", WiFi.SSID(i).c_str(), WiFi.RSSI(i));
        if (WiFi.SSID(i) == GW_SSID) {
            Serial.printf("[DEV] 找到网关: %s (RSSI:%d)\n", GW_SSID, WiFi.RSSI(i));
            WiFi.scanDelete();
            return true;
        }
    }
    Serial.println("[DEV] 未发现网关");
    WiFi.scanDelete();
    return false;
}

/// 连接到网关 AP
bool connectToGateway() {
    Serial.printf("[DEV] 连接网关 AP: %s\n", GW_SSID);
    WiFi.begin(GW_SSID, GW_PASS);

    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
        delay(500);
        Serial.print(".");
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("[DEV] 已连接, 本机 IP: %s\n", WiFi.localIP().toString().c_str());
        return true;
    }
    Serial.println("[DEV] 连接超时");
    return false;
}

/// 通过 UDP 向网关发送 HELLO，等待 ACK
bool sendHelloAndWaitAck() {
    Serial.printf("[DEV] 发送 HELLO → %s:%d\n", GW_SSID, UDP_PORT);
    udp.begin(UDP_PORT + 1);

    char hello[64];
    snprintf(hello, sizeof(hello), "HELLO:%s", DEVICE_ID);
    udp.beginPacket(WiFi.gatewayIP(), UDP_PORT);
    udp.write((const uint8_t*)hello, strlen(hello));
    udp.endPacket();

    Serial.println("[DEV] 等待 ACK...");
    unsigned long start = millis();
    while (millis() - start < 5000) {
        int size = udp.parsePacket();
        if (size > 0) {
            int len = udp.read(respBuf, sizeof(respBuf) - 1);
            if (len > 0) respBuf[len] = '\0';

            if (strncmp(respBuf, "ACK:", 4) == 0) {
                Serial.printf("[DEV] 收到网关确认: %s\n", respBuf);
                return true;
            }
        }
        delay(100);
    }
    Serial.println("[DEV] 等待 ACK 超时");
    return false;
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.printf("\n=== 设备发现测试 [%s] ===\n\n", DEVICE_ID);

    discovered = scanForGateway();
    if (!discovered) {
        Serial.println("[DEV] 未发现网关, 3 秒后重试...");
        delay(3000);
        discovered = scanForGateway();
    }

    if (!discovered) {
        Serial.println("[DEV] 仍未发现网关, 停止");
        while (true) delay(1000);
    }

    connected = connectToGateway();
    if (!connected) {
        Serial.println("[DEV] 连接失败, 停止");
        while (true) delay(1000);
    }

    if (sendHelloAndWaitAck()) {
        Serial.printf("[DEV] 发现完成! 网关已确认, %s 已就绪\n", DEVICE_ID);
    } else {
        Serial.println("[DEV] 握手失败, 等待网关重试...");
    }

    Serial.println("\n[DEV] 进入主循环, 定期发送心跳...\n");
}

void loop() {
    static unsigned long lastHeartbeat = 0;
    if (millis() - lastHeartbeat > 30000 && WiFi.status() == WL_CONNECTED) {
        lastHeartbeat = millis();
        char hello[64];
        snprintf(hello, sizeof(hello), "HELLO:%s", DEVICE_ID);
        udp.beginPacket(WiFi.gatewayIP(), UDP_PORT);
        udp.write((const uint8_t*)hello, strlen(hello));
        udp.endPacket();
        Serial.printf("[DEV] 心跳发送 (%s)\n", DEVICE_ID);
    }

    int size = udp.parsePacket();
    if (size > 0) {
        int len = udp.read(respBuf, sizeof(respBuf) - 1);
        if (len > 0) respBuf[len] = '\0';
        if (strncmp(respBuf, "ACK:", 4) == 0) {
            Serial.printf("[DEV] 收到 ACK: %s\n", respBuf);
        }
    }

    delay(10);
}
