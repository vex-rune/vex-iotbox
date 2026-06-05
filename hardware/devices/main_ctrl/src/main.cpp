#include <Arduino.h>
#include <ArduinoJson.h>
#include "wifi_manager.h"
#include "mqtt_client.h"
#include "device_config.h"

static void on_mqtt_msg(const char *topic, const char *payload, int len)
{
    Serial.printf("收到 [%s]: %.*s\n", topic, len, payload);
    // TODO: 解析子设备上报
    // - 门禁事件 → 驱动小米音箱
    // - 电表数据 → 存储/转发
}

void setup()
{
    Serial.begin(115200);
    Serial.printf("主控单元启动，ID: %s\n", DEVICE_ID);

    wifi_manager_init("vex-iotbox", "vex123456");
    mqtt_client_init("mqtt://192.168.1.100:1883", DEVICE_ID, on_mqtt_msg);
}

void loop()
{
    mqtt_client_loop();
    // TODO: 读取 DHT 温湿度、检测开门按键
    delay(5000);
}
