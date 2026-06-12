/**
 * 网关发现测试 (gw_test)
 *
 * 功能：
 *   - 开启 WiFi AP（热点），供设备自主发现
 *   - 监听 UDP 广播，接收设备 HELLO 报文
 *   - 回复 ACK 确认，打印已发现的设备列表
 *
 * 协议：
 *   AP SSID:  VEX_GW
 *   AP 密码:  vex12345
 *   UDP 端口: 5000
 *   设备 → 网关: "HELLO:{device_id}"
 *   网关 → 设备: "ACK:{gateway_id}"
 */

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>

// ── 配置 ──
static const char* AP_SSID     = "VEX_GW";
static const char* AP_PASS     = "vex12345";
static const uint16_t UDP_PORT = 5000;
static const char* GW_ID       = "GW_001";

// ── UDP ──
WiFiUDP udp;
char packetBuf[256];

// ── 已发现设备列表 ──
struct DeviceEntry {
    char id[32];
    IPAddress ip;
    unsigned long lastSeen;
};

static DeviceEntry discoveredDevices[8];
static int deviceCount = 0;

/// 查找或添加设备，返回索引
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

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n=== 网关发现测试 (gw_test) ===");

    // 开启 AP
    Serial.printf("[GW] 开启热点: SSID=%s\n", AP_SSID);
    WiFi.mode(WIFI_AP);
    bool ok = WiFi.softAP(AP_SSID, AP_PASS);
    if (!ok) {
        Serial.println("[GW] AP 开启失败!");
        while (true) delay(1000);
    }
    Serial.printf("[GW] AP 已就绪, IP: %s\n", WiFi.softAPIP().toString().c_str());

    // 启动 UDP
    udp.begin(UDP_PORT);
    Serial.printf("[GW] UDP 监听端口 %d\n", UDP_PORT);
    Serial.println("[GW] 等待设备发现...\n");
}

void loop() {
    // 接收 UDP 数据包
    int packetSize = udp.parsePacket();
    if (packetSize > 0) {
        int len = udp.read(packetBuf, sizeof(packetBuf) - 1);
        if (len > 0) {
            packetBuf[len] = '\0';
        }

        // 解析 HELLO 报文: "HELLO:{device_id}"
        if (strncmp(packetBuf, "HELLO:", 6) == 0) {
            const char* deviceId = packetBuf + 6;
            IPAddress remoteIp = udp.remoteIP();

            Serial.printf("[GW] 收到 HELLO, 设备=%s, IP=%s\n",
                          deviceId, remoteIp.toString().c_str());

            // 添加或更新设备
            findOrAddDevice(deviceId, remoteIp);

            // 回复 ACK
            udp.beginPacket(udp.remoteIP(), udp.remotePort());
            char ack[64];
            snprintf(ack, sizeof(ack), "ACK:%s", GW_ID);
            udp.write((const uint8_t*)ack, strlen(ack));
            udp.endPacket();

            Serial.printf("[GW] 已回复 ACK → %s\n", remoteIp.toString().c_str());
            printDeviceList();
            Serial.println();
        }
    }

    // 每 10 秒打印一次设备列表
    static unsigned long lastPrint = 0;
    if (millis() - lastPrint > 10000) {
        lastPrint = millis();
        if (deviceCount > 0) {
            printDeviceList();
        } else {
            Serial.println("[GW] 暂无设备连接\n");
        }
    }

    delay(10);
}
