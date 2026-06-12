#pragma once
// ═══════════════════════════════════════
// 按钮（上拉接地，按下 LOW）
// ═══════════════════════════════════════
//
// 接线：按钮一端接 GPIO，另一端接 GND（利用内部上拉电阻）
//
// 用法：
//   // 1. 创建实例（引脚, 名称）
//   Button btn(11, "door_open");
//
//   // 2. setup 中设置上拉输入
//   pinMode(11, INPUT_PULLUP);
//
//   // 3. loop 中检测（需自行做消抖）
//   static bool lastState = false;
//   bool pressed = btn.isPressed();
//   if (pressed && !lastState) {
//       Serial.println("按钮按下");
//   }
//   lastState = pressed;
//
// 注意：isPressed() 不含消抖，需在外层自行用 millis() 实现

#include <Arduino.h>

class Button {
public:
  Button() {}
  Button(int pin, String name) : pin_(pin), name(name) {}

  /// 按下返回 true（LOW），松开返回 false（HIGH）
  bool isPressed() { return digitalRead(pin_) == LOW; }

  /// 状态 JSON，可直接传给 mqtt.publish()
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
