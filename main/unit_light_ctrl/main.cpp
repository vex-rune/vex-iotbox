/**
 * 灯光控制单元 (01XX)
 *
 * 功能：
 *   - GPIO 12 控制光耦继电器开关
 *   - 接收 MQTT 指令：light.on / light.off / status.get
 *   - 状态变化时上报 light.status（retained）
 *   - 上电默认继电器关闭
 */

// 引入头文件
#include <Arduino.h>
#include "common/wifi_manager.h"
#include "common/mqtt_manager.h"
#include "common/protocol.h"
#include "common/message.h"

constexpr uint8_t PIN_RELAY = 12;

// ── 全局对象 ──
// WifiManager 和 MqttManager 是我们自己写的类，封装了 WiFi 和 MQTT 的连接逻辑
WifiManager wifi;   // WiFi 管理器实例
MqttManager mqtt;   // MQTT 客户端实例

// String：Arduino 字符串类，支持 + 拼接，这里用来存 MQTT 主题路径
String topicCmd;      // 指令接收主题，例如 "vex/0101/cmd"
String topicStatus;   // 状态上报主题，例如 "vex/0101/status"

// bool：布尔类型，true 或 false
bool lightOn = false; // 当前灯光状态，false = 关，true = 开

/**
 * reportStatus() — 上报当前灯光状态到 MQTT
 *
 * JsonDocument：ArduinoJson 库的 JSON 文档对象，用来构建 JSON 消息
 * doc["type"] = xxx：向 JSON 对象添加键值对
 * mqtt.publishJson()：将 JSON 序列化后发布到指定主题
 * 第三个参数 true 表示 retained（保留消息），新订阅者能立即收到最新状态
 */
void reportStatus() {
    Message msg(TYPE_LIGHT_STATUS);
    msg.set("on", lightOn);
    mqtt.publishJson(topicStatus.c_str(), msg.doc, true);
}

/**
 * setRelay() — 控制继电器开关并上报状态
 *
 * digitalWrite(pin, HIGH/LOW)：Arduino 函数，设置引脚输出高/低电平
 * HIGH = 3.3V（开），LOW = 0V（关）
 * on ? HIGH : LOW：三元运算符，on 为 true 时取 HIGH，否则取 LOW
 */
void setRelay(bool on) {
    lightOn = on;                                    // 更新状态变量
    digitalWrite(PIN_RELAY, on ? HIGH : LOW);        // 控制引脚电平
    reportStatus();                                  // 状态变化后上报
}

/**
 * onMqttMessage() — 收到 MQTT 消息时的回调函数
 *
 * 参数说明：
 *   topic   — 消息来自哪个主题
 *   payload — 消息内容（JSON 字符串）
 *   length  — 消息长度
 *
 * deserializeJson(doc, payload)：将 JSON 字符串解析到 doc 对象，失败返回非零值
 * doc["type"]：从 JSON 中取 "type" 字段的值，返回 const char*（C 字符串指针）
 * strcmp(a, b)：C 语言函数，比较两个字符串是否相等，相等返回 0
 * TYPE_LIGHT_ON 等：在 protocol.h 中定义的字符串常量，避免硬编码
 */
void onMqttMessage(const char* topic, const char* payload, unsigned int length) {
    Message msg;
    if (!msg.parse(payload)) return;

    if (strcmp(msg.type(), TYPE_LIGHT_ON) == 0) {
        setRelay(true);
    } else if (strcmp(msg.type(), TYPE_LIGHT_OFF) == 0) {
        setRelay(false);
    } else if (strcmp(msg.type(), TYPE_STATUS_GET) == 0) {
        reportStatus();
    }
}

/**
 * setup() — Arduino 程序入口，上电后执行一次
 *
 * pinMode(pin, OUTPUT)：将引脚设为输出模式
 * digitalWrite(pin, LOW)：初始化时将继电器设为关闭
 * topic::cmd(UNIT_ID)：调用 protocol.h 中的函数，生成 "vex/0101/cmd"
 * wifi.begin()：初始化 WiFi 连接
 * mqtt.begin()：初始化 MQTT 连接，传入 broker 地址、端口、客户端 ID、消息回调
 *   String("light-") + UNIT_ID：拼接客户端 ID，例如 "light-0101"
 *   .c_str()：将 Arduino String 转为 C 字符串（const char*），MQTT 库需要这种格式
 */
void setup() {
    pinMode(PIN_RELAY, OUTPUT);       // 设置 GPIO 12 为输出模式
    digitalWrite(PIN_RELAY, LOW);     // 上电默认关闭继电器

    topicCmd = topic::cmd(UNIT_ID);       // 生成指令主题
    topicStatus = topic::status(UNIT_ID); // 生成状态主题

    wifi.begin(WIFI_SSID, WIFI_PASS);     // 连接 WiFi
    String clientId = String("light-") + UNIT_ID;  // 拼接客户端 ID
    mqtt.begin(MQTT_HOST, MQTT_PORT, clientId.c_str(), onMqttMessage); // 连接 MQTT
}

/**
 * loop() — Arduino 主循环，setup() 之后反复执行
 *
 * wifi.loop()：检查 WiFi 连接，断线自动重连
 * mqtt.loop()：检查 MQTT 连接，断线自动重连，处理收发消息
 *
 * static bool subscribed = false：
 *   static 表示变量只初始化一次，loop() 反复调用时保留上次的值
 *   用来记住是否已经订阅过主题，避免重复订阅
 *
 * mqtt.subscribe(topicCmd.c_str())：订阅指令主题，收到消息后会触发 onMqttMessage
 * reportStatus()：首次连接成功后上报一次当前状态
 *
 * 断线时将 subscribed 重置为 false，下次重连成功后会重新订阅
 */
void loop() {
    wifi.loop();   // 驱动 WiFi（含自动重连）
    mqtt.loop();   // 驱动 MQTT（含自动重连 + 消息收发）

    static bool subscribed = false;  // 标记是否已订阅
    if (mqtt.isConnected() && !subscribed) {  // MQTT 连接成功且未订阅
        mqtt.subscribe(topicCmd.c_str());     // 订阅指令主题
        reportStatus();                       // 上报初始状态
        subscribed = true;                    // 标记为已订阅
    } else if (!mqtt.isConnected()) {         // MQTT 断开连接
        subscribed = false;                   // 重置标记，重连后重新订阅
    }
}
