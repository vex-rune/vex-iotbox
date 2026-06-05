#include <Arduino.h>
#include <ArduinoJson.h>
#include "wifi_manager.h"
#include "mqtt_client.h"
#include "device_config.h"

static void on_mqtt_msg(const char *topic, const char *payload, int len)
{
    Serial.printf("收到 [%s]: %.*s\n", topic, len, payload);
    // TODO: 解析 JSON
    // - unlock: 驱动电磁锁开门
    // - lock:   锁定门禁
}

void setup()
{
    Serial.begin(115200);
    Serial.printf("门禁单元启动，ID: %s\n", DEVICE_ID);

    wifi_manager_init("vex-iotbox", "vex123456");
    mqtt_client_init("mqtt://192.168.1.100:1883", DEVICE_ID, on_mqtt_msg);
}

void loop()
{
    mqtt_client_loop();
    // TODO: 检测 NFC 读卡、门铃按键、PIR 人体感应
    delay(100);
}
