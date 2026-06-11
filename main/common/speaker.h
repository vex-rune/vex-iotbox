#pragma once
// ═══════════════════════════════════════
// 喇叭 MAX98357A（I2S 输出）
// ═══════════════════════════════════════

#include <Arduino.h>
#include <driver/i2s.h>
#include "Audio.h"

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
    installDriver_();
    state_ = "ready";
  }

  /// 播放音频数据（PCM）
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

  // ─── URL 流播放 ───

  /// 开始从 URL 播放音频（MP3/WAV 流）
  /// durationSec: 播放时长（秒），0 = 播完为止
  bool playUrl(const String &url, int durationSec = 0) {
    if (url.isEmpty()) return false;

    // 卸载本地 I2S 驱动，让 Audio 库接管
    i2s_driver_uninstall(i2s_num_);

    ensureAudio_();
    audio_->setPinout(bck_, ws_, din_);
    audio_->setVolume(20);
    audio_->setBufsize(64 * 1024, 2048);  // 加大缓冲区，防止卡顿
    audio_->connecttohost(url.c_str());

    urlDuration_ = (unsigned long)durationSec * 1000;
    urlStart_ = millis();
    urlPlaying_ = true;
    state_ = "url_playing";
    return true;
  }

  /// 在 loop() 中调用，驱动 URL 流播放
  /// 返回 true 表示仍在播放
  bool loopUrl() {
    if (!urlPlaying_ || !audio_) return false;

    // 检查是否超时
    if (urlDuration_ > 0 && millis() - urlStart_ >= urlDuration_) {
      stopUrl();
      return false;
    }

    audio_->loop();
    return true;
  }

  /// 停止 URL 播放，恢复本地 I2S 驱动
  void stopUrl() {
    if (audio_) {
      audio_->stopSong();
      delete audio_;
      audio_ = nullptr;
    }
    urlPlaying_ = false;

    // 恢复本地 I2S 驱动
    installDriver_();
    state_ = "ready";
  }

  /// 当前是否在播放 URL 流
  bool isUrlPlaying() const { return urlPlaying_; }

  /// 状态 JSON
  String getStatusJson() {
    char buf[128];
    snprintf(buf, sizeof(buf), "{\"name\": \"%s\", \"state\": %s}",
             name.c_str(), state_);
    return String(buf);
  }

private:
  /// 安装本地 I2S 驱动
  void installDriver_() {
    i2s_conf_ = {
        .mode = i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = 44100,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .dma_buf_count = 16,
        .dma_buf_len = 256,
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
  }

  /// 懒创建 Audio 实例（仅在需要 URL 播放时分配）
  void ensureAudio_() {
    if (!audio_) {
      audio_ = new Audio();
    }
  }

  i2s_port_t i2s_num_;
  i2s_config_t i2s_conf_;
  const char *state_ = "pending";
  int bck_ = 0, ws_ = 0, din_ = 0;
  String name;

  // URL 播放状态
  Audio *audio_ = nullptr;
  bool urlPlaying_ = false;
  unsigned long urlStart_ = 0;
  unsigned long urlDuration_ = 0;
};
