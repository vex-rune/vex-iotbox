#pragma once

/**
 * MQTT 协议定义
 *
 * 职责：
 *   - 统一构建本地 / 云端 MQTT 主题路径
 *   - 定义所有消息 type 常量，避免各单元硬编码字符串
 *
 * 主题格式：
 *   本地：vex/{unit_id}/cmd | status | event
 *   云端：cloud/vex/{unit_id}/cmd | status | event
 */

#include <Arduino.h>

// ── 主题构建 ──

namespace topic {

// ─── 本地 MQTT 主题（设备间通信）───

/// 本地下发指令：vex/{unit_id}/cmd
inline String cmd(const char* unitId) {
    return String("vex/") + unitId + "/cmd";
}

/// 设备状态上报：vex/{unit_id}/status
inline String status(const char* unitId) {
    return String("vex/") + unitId + "/status";
}

/// 设备事件上报：vex/{unit_id}/event
inline String event(const char* unitId) {
    return String("vex/") + unitId + "/event";
}

// ─── 云端 MQTT 主题（主控透传用）───

/// 云端下发指令：cloud/vex/{unit_id}/cmd
inline String cloudCmd(const char* unitId) {
    return String("cloud/vex/") + unitId + "/cmd";
}

/// 云端状态透传：cloud/vex/{unit_id}/status
inline String cloudStatus(const char* unitId) {
    return String("cloud/vex/") + unitId + "/status";
}

/// 云端事件透传：cloud/vex/{unit_id}/event
inline String cloudEvent(const char* unitId) {
    return String("cloud/vex/") + unitId + "/event";
}

} // namespace topic

// ── 消息类型常量 ──

// ─── 灯光单元 ───
constexpr const char* TYPE_LIGHT_ON        = "light.on";      // 开灯指令
constexpr const char* TYPE_LIGHT_OFF       = "light.off";     // 关灯指令
constexpr const char* TYPE_LIGHT_AUTO      = "light.auto";    // 自动模式配置
constexpr const char* TYPE_LIGHT_STATUS    = "light.status";  // 灯光状态上报

// ─── 门禁单元 ───
constexpr const char* TYPE_DOOR_UNLOCK     = "door.unlock";   // 开门指令
constexpr const char* TYPE_DOOR_STATUS     = "door.status";   // 门禁状态上报
constexpr const char* TYPE_DOOR_CARD       = "door.card";     // NFC 刷卡事件
constexpr const char* TYPE_DOOR_BELL       = "door.bell";     // 门铃按下事件
constexpr const char* TYPE_DOOR_PIR        = "door.pir";      // PIR 人体感应事件

// ─── 主控单元 ───
constexpr const char* TYPE_ENV_STATUS      = "env.status";    // 环境数据上报
constexpr const char* TYPE_ENV_QUERY       = "env.query";     // 环境数据查询指令
constexpr const char* TYPE_DOOR_OPEN       = "door.open";     // 本地按键开门事件

// ─── 通用 ───
constexpr const char* TYPE_STATUS_GET      = "status.get";    // 查询任意单元当前状态
