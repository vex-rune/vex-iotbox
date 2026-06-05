#pragma once

#include <Arduino.h>
#include <WiFiClient.h>
#include <PubSubClient.h>

typedef void (*mqtt_msg_handler_t)(const char *topic, const char *payload, int len);

static WiFiClient s_wifi_client;
static PubSubClient s_mqtt_client(s_wifi_client);
static mqtt_msg_handler_t s_msg_handler = nullptr;

static void mqtt_callback(char *topic, byte *payload, unsigned int length)
{
    if (s_msg_handler) {
        s_msg_handler(topic, (const char *)payload, (int)length);
    }
}

inline void mqtt_client_init(const char *broker_uri, const char *device_id,
                             mqtt_msg_handler_t handler)
{
    s_msg_handler = handler;

    // 解析 broker URI：mqtt://192.168.1.100:1883
    String uri(broker_uri);
    String host = uri.substring(6, uri.indexOf(':', 6));
    int port = uri.substring(uri.indexOf(':', 6) + 1).toInt();

    s_mqtt_client.setServer(host.c_str(), port);
    s_mqtt_client.setCallback(mqtt_callback);

    String client_id = "esp32-" + String(device_id);
    while (!s_mqtt_client.connected()) {
        Serial.printf("连接 MQTT Broker %s:%d ...", host.c_str(), port);
        if (s_mqtt_client.connect(client_id.c_str())) {
            Serial.println(" 已连接");
            String cmd_topic = "dev/" + String(device_id) + "/cmd";
            s_mqtt_client.subscribe(cmd_topic.c_str());
            Serial.printf("已订阅: %s\n", cmd_topic.c_str());
        } else {
            Serial.println(" 失败，3秒后重试");
            delay(3000);
        }
    }
}

inline bool mqtt_publish(const char *device_id, const char *payload)
{
    String topic = "dev/" + String(device_id) + "/upload";
    return s_mqtt_client.publish(topic.c_str(), payload);
}

inline void mqtt_client_loop()
{
    s_mqtt_client.loop();
}
