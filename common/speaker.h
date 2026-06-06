#pragma once
// ═══════════════════════════════════════
// 喇叭 MAX98357A（I2S 输出）
// ═══════════════════════════════════════

#include <Arduino.h>
#include <driver/i2s.h>

class Speaker {
public:
  Speaker() {}

  Speaker(int bck, int ws, int din, i2s_port_t port, String name)
      : i2s_num_(port), bck_(bck), ws_(ws), din_(din), name(name) {}

  /// 初始化 I2S 驱动（必须在 setup 中调用）
  void begin() {
    pinMode(bck_, OUTPUT);
    pinMode(ws_, OUTPUT);
    pinMode(din_, OUTPUT);

    i2s_conf_ = {
        .mode = i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = 44100,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .dma_buf_count = 8,
        .dma_buf_len = 64,
        .tx_desc_auto_clear = true,
    };

    i2s_driver_install(i2s_num_, &i2s_conf_, 0, nullptr);

    i2s_pin_config_t pin_conf_ = {
        .bck_io_num = bck_,
        .ws_io_num = ws_,
        .data_out_num = din_,
        .data_in_num = I2S_PIN_NO_CHANGE,
    };
    i2s_set_pin(i2s_num_, &pin_conf_);
    state_ = "ready";
  }

  /// 播放音频数据
  void play(const int16_t *buf, size_t len) {
    size_t w;
    i2s_write(i2s_num_, buf, len * 2, &w, portMAX_DELAY);
    state_ = "playing";
  }

  /// 静音
  void mute() {
    i2s_zero_dma_buffer(i2s_num_);
    state_ = "muted";
  }

  /// 状态 JSON
  String getStatusJson() {
    char buf[128];
    snprintf(buf, sizeof(buf), "{\"name\": \"%s\", \"state\": %s}",
             name.c_str(), state_);
    return String(buf);
  }

private:
  i2s_port_t i2s_num_;
  i2s_config_t i2s_conf_;
  const char *state_ = "pending";
  int bck_ = 0, ws_ = 0, din_ = 0;
  String name;
};
