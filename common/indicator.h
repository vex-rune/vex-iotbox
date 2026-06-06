#pragma once
// ═══════════════════════════════════════
// 指示灯（LED）
// ═══════════════════════════════════════

#include <Arduino.h>

class Indicator {
public:
  Indicator() {}
  Indicator(int pin, String name) : pin_(pin), name(name) {}

  void begin() { pinMode(pin_, OUTPUT); }

  void on() { digitalWrite(pin_, HIGH); }
  void off() { digitalWrite(pin_, LOW); }
  bool isOn() { return digitalRead(pin_) == HIGH; }

  /// 状态 JSON
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
