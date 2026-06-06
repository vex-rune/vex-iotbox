#pragma once
// ═══════════════════════════════════════
// 按钮（上拉接地，按下 LOW）
// ═══════════════════════════════════════

#include <Arduino.h>

class Button {
public:
  Button() {}
  Button(int pin, String name) : pin_(pin), name(name) {}

  /// 按下返回 true
  bool isPressed() { return digitalRead(pin_) == LOW; }

  /// 状态 JSON
  String getStatusJson() {
    char buf[128];
    snprintf(buf, sizeof(buf),
             "{\"name\": \"%s\", \"pin\": %d, \"pressed\": %s}",
             name.c_str(), pin_, isPressed() ? "true" : "false");
    return String(buf);
  }

private:
  int pin_ = 0;
  String name;
};
