#pragma once
// ═══════════════════════════════════════
// 指示灯（LED）
// ═══════════════════════════════════════
//
// 接线：GPIO → 330Ω 限流电阻 → LED 正极 → GND
//
// 用法：
//   // 1. 创建实例（引脚, 名称）
//   Indicator led(15, "led_red");
//
//   // 2. setup 中初始化为输出模式
//   led.begin();
//
//   // 3. 控制开关
//   led.on();               // 亮
//   led.off();              // 灭
//   bool state = led.isOn(); // 查询当前状态
//
//   // 4. 状态 JSON（可直接用于 MQTT 发布）
//   String json = led.getStatusJson();
//   // → {"name": "led_red", "pin": 15, "state": "on"}

#include <Arduino.h>

class Indicator {
public:
  Indicator() {}
  Indicator(int pin, String name) : pin_(pin), name(name) {}

  /// 初始化为输出模式，必须在 setup() 中调用
  void begin() { pinMode(pin_, OUTPUT); }

  void on() { digitalWrite(pin_, HIGH); }
  void off() { digitalWrite(pin_, LOW); }
  bool isOn() { return digitalRead(pin_) == HIGH; }

  /// 状态 JSON，可直接传给 mqtt.publish()
  String getStatusJson() {
    char buf[128];
    snprintf(buf, sizeof(buf),
             "{\"name\": \"%s\", \"pin\": %d, \"state\": %s}",
             name.c_str(), pin_, isOn() ? "on" : "off");
    return String(buf);
  }

private:
  int pin_ = 0;
  String name;
};
