#pragma once

/**
 * WiFi 连接管理器
 *
 * 职责：
 *   - STA 模式连接路由器
 *   - 断线自动重连（loop 中调用，非阻塞）
 *
 * 用法：
 *   WifiManager wifi;
 *   wifi.begin(WIFI_SSID, WIFI_PASS);
 *   // loop 中
 *   wifi.loop();
 */

#include <Arduino.h>
#include <WiFi.h>

class WifiManager {
public:
    /// 初始化 WiFi，设置 STA 模式并开始连接
    void begin(const char* ssid, const char* pass) {
        ssid_ = ssid;
        pass_ = pass;
        WiFi.mode(WIFI_STA);
        WiFi.begin(ssid, pass);
    }

    /// 当前是否已连接
    bool isConnected() {
        return WiFi.status() == WL_CONNECTED;
    }

    /// 非阻塞轮询，断线时自动重连（最多等待 10 秒）
    void loop() {
        if (WiFi.status() != WL_CONNECTED) {
            WiFi.reconnect();
            unsigned long start = millis();
            while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
                delay(100);
            }
        }
    }

    /// 获取本机局域网 IP
    IPAddress localIP() {
        return WiFi.localIP();
    }

private:
    const char* ssid_ = nullptr;
    const char* pass_ = nullptr;
};
