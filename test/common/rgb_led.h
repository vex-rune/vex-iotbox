#pragma once
// ═══════════════════════════════════════
// 板载 RGB LED（WS2812 / NeoPixel）
// ═══════════════════════════════════════
//
// 接线：单线协议，GPIO 直驱 LED DIN 引脚
//
// 用法：
//   // 1. 创建实例（引脚）
//   RgbLed rgb(48);
//
//   // 2. setup 中初始化（亮度 0-255）
//   rgb.begin(50);
//
//   // 3. 设置颜色
//   rgb.setColor(255, 0, 0);    // 红色
//   rgb.setColor(0, 255, 0);    // 绿色
//   rgb.off();                   // 关闭
//
//   // 4. 彩虹轮转（非阻塞，每次调用前进一帧，需在 loop 中持续调用）
//   rgb.rainbow();
//
//   // 5. 状态 JSON
//   String json = rgb.getStatusJson();
//   // → {"pin":48,"r":255,"g":0,"b":0}
//
// 依赖：adafruit/Adafruit NeoPixel

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

class RgbLed {
public:
  RgbLed(int pin) : pin_(pin), strip_(1, pin, NEO_GRB + NEO_KHZ800) {}

  /// 初始化，设置亮度 0-255，必须在 setup() 中调用
  void begin(uint8_t brightness = 50) {
    strip_.begin();
    strip_.setBrightness(brightness);
    strip_.show();
  }

  /// 设置颜色（R, G, B）
  void setColor(uint8_t r, uint8_t g, uint8_t b) {
    strip_.setPixelColor(0, strip_.Color(r, g, b));
    strip_.show();
  }

  /// 关闭
  void off() { setColor(0, 0, 0); }

  /// 彩虹轮转（非阻塞，每次调用前进一帧）
  void rainbow() {
    static uint8_t hue = 0;
    strip_.setPixelColor(0, wheel(hue++));
    strip_.show();
  }

  /// 获取当前颜色的 JSON
  String getStatusJson() {
    uint32_t c = strip_.getPixelColor(0);
    char buf[64];
    snprintf(buf, sizeof(buf), "{\"pin\":%d,\"r\":%d,\"g\":%d,\"b\":%d}",
             pin_, (c >> 16) & 0xFF, (c >> 8) & 0xFF, c & 0xFF);
    return String(buf);
  }

private:
  int pin_;
  Adafruit_NeoPixel strip_;

  /// 色轮：0-255 映射到彩虹色
  uint32_t wheel(byte pos) {
    pos = 255 - pos;
    if (pos < 85) return strip_.Color(255 - pos * 3, 0, pos * 3);
    if (pos < 170) { pos -= 85; return strip_.Color(0, pos * 3, 255 - pos * 3); }
    pos -= 170;
    return strip_.Color(pos * 3, 255 - pos * 3, 0);
  }
};
