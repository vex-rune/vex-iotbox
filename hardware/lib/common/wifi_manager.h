#pragma once

#include <Arduino.h>
#include <WiFi.h>

inline void wifi_manager_init(const char *ssid, const char *password)
{
    WiFi.begin(ssid, password);
    Serial.print("连接 WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println();
    Serial.print("WiFi 已连接，IP: ");
    Serial.println(WiFi.localIP());
}

inline bool wifi_manager_is_connected()
{
    return WiFi.status() == WL_CONNECTED;
}
