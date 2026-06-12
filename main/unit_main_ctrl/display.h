#pragma once
// ═══════════════════════════════════════
// OLED 显示器（SSD1306，I2C，U8g2）
// ═══════════════════════════════════════
//
// 接线：GND→GND, VCC→3.3V, SDA→GPIO, SCL→GPIO
//
// 用法：
//   // 1. 创建实例（SDA引脚, SCL引脚, 宽, 高, 名称）
//   Display oled(1, 2, 128, 32, "oled");
//
//   // 2. setup 中初始化（屏幕显示 "OLED Ready"）
//   oled.begin();
//
//   // 3. 显示文字（128x32 屏幕，中文字体每行约 8 个汉字）
//   oled.showText("25.3C 60%");     // 默认位置 (0, 16)
//   oled.showText("你好", 0, 16);    // 指定位置
//
//   // 4. 关闭/唤醒屏幕
//   oled.close();    // 省电模式
//   oled.open();     // 恢复显示
//
// 注意：showText() 每次调用会清屏并重绘，不支持多行叠加
// 依赖：olikraus/U8g2

#include "clib/u8g2.h"
#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>

class Display {
public:
  Display() : display(nullptr) {}

  Display(int sda, int scl, int width, int height, String name)
      : sda_(sda), scl_(scl), name_(name),
        display(new U8G2_SSD1306_128X32_UNIVISION_F_SW_I2C(U8G2_R0, scl, sda,
                                                           U8X8_PIN_NONE)) {}

  void begin() {
    if (!display)
      return;
    display->begin();
    display->clearBuffer();
    display->setFont(u8g2_font_6x10_tf);
    display->drawStr(0, 12, "OLED Ready");
    display->sendBuffer();
    state_ = "ready";
  }

  void showText(const char *text, int x = 0, int y = 16) {
    if (!display)
      return;
    display->clearBuffer();
    display->setFont(u8g2_font_unifont_t_chinese3);
    display->drawStr(x, y, text);
    display->sendBuffer();
    state_ = "show";
  }

  void close() {
    if (!display)
      return;
    display->setPowerSave(1);
    state_ = "closed";
  }

  void open() {
    if (!display)
      return;
    display->setPowerSave(0);
    state_ = "open";
  }

  bool isReady() const { return state_ == "ready"; }

  String getStatusJson() {
    char buf[128];
    snprintf(buf, sizeof(buf), "{\"name\":\"%s\",\"state\":\"%s\"}",
             name_.c_str(), state_);
    return String(buf);
  }

private:
  int sda_ = 0, scl_ = 0;
  String name_;
  U8G2_SSD1306_128X32_UNIVISION_F_SW_I2C *display = nullptr;
  const char *state_ = "init";
};
