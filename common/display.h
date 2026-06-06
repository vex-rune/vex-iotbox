#pragma once
// ═══════════════════════════════════════
// OLED 显示器（SSD1306，I2C，U8g2）
// ═══════════════════════════════════════

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
