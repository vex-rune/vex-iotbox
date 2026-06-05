#include <Arduino.h>
#include <ArduinoJson.h>
#include "wifi_manager.h"
#include "mqtt_client.h"
#include "device_config.h"

/**
 * 硬件规格：
 * - 供电：220V 市电，板载 AC-DC 降压模块为 ESP32 供电
 * - 输出：光耦继电器（隔离驱动，控制灯具通断）
 * - 调光：0-10V 调光信号输出（可选）
 */

#define GPIO_RELAY       12   // 光耦继电器控制引脚
#define GPIO_PWM_DAC     25   // 0-10V 调光输出（可选）

// 光敏传感器主题（由传感器设备发布）
#define TOPIC_LIGHT_SENSOR "dev/sensor/light/upload"

// 灯光工作模式
typedef enum {
    LIGHT_MODE_MANUAL,
    LIGHT_MODE_AUTO,
} light_mode_t;

// 灯光状态
typedef struct {
    light_mode_t mode;
    bool power;
    int brightness;
    int lux_threshold;
    int hysteresis;
    int current_lux;
} light_state_t;

static light_state_t s_state = {
    .mode          = LIGHT_MODE_MANUAL,
    .power         = false,
    .brightness    = 100,
    .lux_threshold = 30,
    .hysteresis    = 5,
    .current_lux   = 0,
};

static void relay_set(bool on)
{
    digitalWrite(GPIO_RELAY, on ? HIGH : LOW);
    s_state.power = on;
    Serial.printf("继电器: %s\n", on ? "开" : "关");
}

static void auto_mode_check(void)
{
    if (s_state.mode != LIGHT_MODE_AUTO) return;

    if (s_state.current_lux < s_state.lux_threshold && !s_state.power) {
        relay_set(true);
    } else if (s_state.current_lux > s_state.lux_threshold + s_state.hysteresis && s_state.power) {
        relay_set(false);
    }
}

// MQTT 指令
// { "cmd": "set_mode",       "value": "manual" | "auto" }
// { "cmd": "switch",         "value": "on" | "off" }
// { "cmd": "set_brightness", "value": 0~100 }
// { "cmd": "set_threshold",  "value": 30 }
// { "cmd": "set_hysteresis", "value": 5 }
// { "cmd": "get_status" }
static void handle_cmd(char *payload, int len)
{
    StaticJsonDocument<256> doc;
    if (deserializeJson(doc, payload, len)) {
        Serial.println("JSON 解析失败");
        return;
    }

    const char *cmd = doc["cmd"];
    if (!cmd) return;

    if (strcmp(cmd, "set_mode") == 0) {
        const char *val = doc["value"];
        if (strcmp(val, "manual") == 0) {
            s_state.mode = LIGHT_MODE_MANUAL;
            Serial.println("切换到：手动模式");
        } else if (strcmp(val, "auto") == 0) {
            s_state.mode = LIGHT_MODE_AUTO;
            Serial.println("切换到：自动模式");
        }
    } else if (strcmp(cmd, "switch") == 0) {
        if (s_state.mode == LIGHT_MODE_MANUAL) {
            const char *val = doc["value"];
            relay_set(strcmp(val, "on") == 0);
        } else {
            Serial.println("自动模式下不接受手动开关");
        }
    } else if (strcmp(cmd, "set_brightness") == 0) {
        if (s_state.mode == LIGHT_MODE_MANUAL) {
            s_state.brightness = doc["value"];
            Serial.printf("亮度设为: %d%%\n", s_state.brightness);
        }
    } else if (strcmp(cmd, "set_threshold") == 0) {
        s_state.lux_threshold = doc["value"];
        Serial.printf("光照阈值设为: %d lux\n", s_state.lux_threshold);
    } else if (strcmp(cmd, "set_hysteresis") == 0) {
        s_state.hysteresis = doc["value"];
        Serial.printf("回差设为: %d lux\n", s_state.hysteresis);
    } else if (strcmp(cmd, "get_status") == 0) {
        StaticJsonDocument<256> status;
        status["mode"] = s_state.mode == LIGHT_MODE_MANUAL ? "manual" : "auto";
        status["power"] = s_state.power;
        status["brightness"] = s_state.brightness;
        status["lux_threshold"] = s_state.lux_threshold;
        status["current_lux"] = s_state.current_lux;
        char buf[256];
        serializeJson(status, buf, sizeof(buf));
        mqtt_publish(DEVICE_ID, buf);
    } else {
        Serial.printf("未知指令: %s\n", cmd);
    }
}

static void on_mqtt_msg(const char *topic, const char *payload, int len)
{
    Serial.printf("收到 [%s]: %.*s\n", topic, len, payload);

    String cmd_topic = topic_cmd(DEVICE_ID);
    if (strcmp(topic, cmd_topic.c_str()) == 0) {
        handle_cmd((char *)payload, len);
    } else if (strcmp(topic, TOPIC_LIGHT_SENSOR) == 0) {
        StaticJsonDocument<128> doc;
        if (!deserializeJson(doc, payload, len)) {
            s_state.current_lux = doc["lux"];
            auto_mode_check();
        }
    }
}

void setup()
{
    Serial.begin(115200);
    Serial.printf("灯光控制单元启动，ID: %s\n", DEVICE_ID);

    pinMode(GPIO_RELAY, OUTPUT);
    digitalWrite(GPIO_RELAY, LOW);

    wifi_manager_init("vex-iotbox", "vex123456");
    mqtt_client_init("mqtt://192.168.1.100:1883", DEVICE_ID, on_mqtt_msg);

    Serial.printf("初始模式: %s\n", s_state.mode == LIGHT_MODE_MANUAL ? "手动" : "自动");
}

void loop()
{
    mqtt_client_loop();
    auto_mode_check();
    delay(5000);
}
