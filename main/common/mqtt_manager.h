#pragma once

/**
 * MQTT 客户端管理器
 *
 * 职责：
 *   - 连接指定 MQTT Broker，断线自动重连
 *   - 封装订阅、发布、JSON 发布接口
 *   - 通过回调将收到的消息传递给业务层
 *
 * 用法：
 *   MqttManager mqtt;
 *   mqtt.begin(host, port, clientId, onMessage);
 *   // loop 中
 *   mqtt.loop();
 *   // 重连成功后手动订阅
 *   if (mqtt.isConnected()) mqtt.subscribe(topic);
 */

#include "WString.h"
#include <Arduino.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

/// 消息回调签名：topic、payload（已转为 C 字符串）、长度
using MqttCallback = void (*)(const char* topic, const char* payload, unsigned int length);

class MqttManager {
public:
    /// 初始化 MQTT 连接参数并注册消息回调（无认证）
    void begin(const char* host, uint16_t port, const char* clientId, MqttCallback cb) {
        begin(host, port, clientId, nullptr, nullptr, cb);
    }

    /// 初始化 MQTT 连接参数并注册消息回调（带认证）
    void begin(const char* host, uint16_t port, const char* clientId,
               const char* user, const char* pass, MqttCallback cb) {
        host_ = host;
        port_ = port;
        clientId_ = clientId;
        user_ = user;
        pass_ = pass;
        callback_ = cb;

        client_.setClient(wifiClient_);
        client_.setServer(host, port);
        client_.setCallback([](char* topic, byte* payload, unsigned int length) {
            if (instance_ && instance_->callback_) {
                char msg[512];
                unsigned int len = min(length, (unsigned int)(sizeof(msg) - 1));
                memcpy(msg, payload, len);
                msg[len] = '\0';
                instance_->callback_(topic, msg, len);
            }
        });
        instance_ = this;
    }

    /// 当前是否已连接 Broker
    bool isConnected() {
        return client_.connected();
    }

    /// 非阻塞轮询，断线自动重连，驱动 MQTT 内部循环
    void loop() {
        if (!client_.connected()) {
            reconnect();
        }
        client_.loop();
    }

    /// 订阅指定主题
    bool subscribe(const char* topic) {
        return client_.subscribe(topic);
    }

    /// 发布文本消息，可选 retained 标记
    bool publish(const char* topic, const String& payload, bool retained = false) {
        return client_.publish(topic, payload.c_str(), retained);
    }

    /// 发布 JSON 消息，内部序列化后发送
    bool publishJson(const char* topic, JsonDocument& doc, bool retained = false) {
        char buf[512];
        serializeJson(doc, buf);
        return publish(topic, buf, retained);
    }

private:
    /// 尝试连接 Broker，最多等待 10 秒
    void reconnect() {
        unsigned long start = millis();
        while (!client_.connected() && millis() - start < 10000) {
            bool ok;
            if (user_ && pass_) {
                ok = client_.connect(clientId_, user_, pass_);
            } else {
                ok = client_.connect(clientId_);
            }
            if (ok) return;
            delay(500);
        }
    }

    WiFiClient wifiClient_;
    PubSubClient client_{wifiClient_};
    const char* host_ = nullptr;
    uint16_t port_ = 0;
    const char* clientId_ = nullptr;
    const char* user_ = nullptr;
    const char* pass_ = nullptr;
    MqttCallback callback_ = nullptr;

    /// 静态实例指针，供 PubSubClient lambda 回调访问
    inline static MqttManager* instance_ = nullptr;
};
