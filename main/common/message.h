#pragma once

/**
 * MQTT 消息实体
 *
 * 职责：
 *   - 统一封装所有 status / event / cmd 消息的 JSON 结构
 *   - 自动附加 eventId（唯一标识）和 timestamp（上电毫秒数）
 *   - 提供链式 API 设置业务字段
 *
 * 标准字段：
 *   type      — 消息类型，对应 protocol.h 中的 TYPE_* 常量
 *   eventId   — 自动生成，格式 "{millis}-{counter}"
 *   timestamp — 上电毫秒数（millis()）
 *
 * 用法：
 *   Message msg(TYPE_ENV_STATUS);
 *   msg.set("temp", 25.3f).set("humidity", 60.0f);
 *   mqtt.publishJson(topicMyStatus, msg.doc, true);
 *
 *   // 解析入站消息
 *   Message::parse(payload, [&](Message& m) {
 *       if (m.type() == TYPE_DOOR_BELL) { ... }
 *   });
 */

#include <Arduino.h>
#include <ArduinoJson.h>

class Message {
public:
    /// 构造：自动填充 type / eventId / timestamp
    explicit Message(const char* type) {
        doc["type"] = type;
        doc["eventId"] = nextEventId();
        doc["timestamp"] = millis();
    }

    /// 空构造：用于解析入站消息
    Message() = default;

    // ── 链式设置业务字段 ──

    Message& set(const char* key, const char* value) {
        doc[key] = value;
        return *this;
    }

    Message& set(const char* key, const String& value) {
        doc[key] = value;
        return *this;
    }

    Message& set(const char* key, int value) {
        doc[key] = value;
        return *this;
    }

    Message& set(const char* key, unsigned int value) {
        doc[key] = value;
        return *this;
    }

    Message& set(const char* key, long value) {
        doc[key] = value;
        return *this;
    }

    Message& set(const char* key, float value) {
        doc[key] = value;
        return *this;
    }

    Message& set(const char* key, double value) {
        doc[key] = value;
        return *this;
    }

    Message& set(const char* key, bool value) {
        doc[key] = value;
        return *this;
    }

    // ── 读取字段 ──

    /// 获取 type 字段
    const char* type() const {
        return doc["type"] | "";
    }

    /// 获取 eventId 字段
    const char* eventId() const {
        return doc["eventId"] | "";
    }

    /// 获取指定字段（字符串）
    const char* getString(const char* key, const char* fallback = "") const {
        return doc[key] | fallback;
    }

    /// 获取指定字段（整数）
    int getInt(const char* key, int fallback = 0) const {
        return doc[key] | fallback;
    }

    /// 获取指定字段（浮点数）
    float getFloat(const char* key, float fallback = 0.0f) const {
        return doc[key] | fallback;
    }

    /// 获取指定字段（布尔值）
    bool getBool(const char* key, bool fallback = false) const {
        return doc[key] | fallback;
    }

    // ── 解析 ──

    /// 解析 JSON payload，成功返回 true
    bool parse(const char* payload) {
        return !deserializeJson(doc, payload);
    }

    // ── 底层访问 ──

    /// 直接操作 JsonDocument（兼容 mqtt.publishJson）
    JsonDocument doc;

private:
    /// 生成唯一 eventId："{millis}-{counter}"
    static String nextEventId() {
        static uint32_t counter = 0;
        char buf[32];
        snprintf(buf, sizeof(buf), "%lu-%lu", millis(), ++counter);
        return String(buf);
    }
};
