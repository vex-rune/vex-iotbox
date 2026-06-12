#pragma once
// ═══════════════════════════════════════
// 喇叭 MAX98357A（I2S 输出，URL 流播放）
// ═══════════════════════════════════════
//
// 接线：BCLK→GPIO, LRC→GPIO, DIN→GPIO, VIN→3.3V, GND→GND
//       SD 悬空 = 始终启用；GAIN 悬空 = 15dB
//
// 用法：
//   // 1. 创建实例（BCLK, LRC, DIN, 名称）
//   Speaker speaker(21, 47, 20, "speaker");
//
//   // 2. setup 中初始化
//   speaker.begin();
//
//   // 3. 播放 URL 音频（非阻塞）
//   speaker.playUrl("http://example.com/doorbell.mp3", 5);  // 播 5 秒
//
//   // 4. loop 中驱动播放
//   speaker.loopUrl();  // 每次 loop 调一次
//
//   // 5. 主动停止
//   speaker.stopUrl();
//
//   // 6. 查询状态
//   speaker.isPlaying();  // 是否正在播放
//
// 依赖：esphome/ESP32-audioI2S

#include <Arduino.h>
#include "Audio.h"

class Speaker {
public:
  Speaker() {}

  Speaker(int bck, int ws, int din, String name)
      : bck_(bck), ws_(ws), din_(din), name(name) {}

  /// 初始化引脚（必须在 setup 中调用）
  void begin() {
    pinMode(bck_, OUTPUT);
    pinMode(ws_, OUTPUT);
    pinMode(din_, OUTPUT);
    state_ = "ready";
  }

  /// 开始从 URL 播放音频（MP3/WAV 流），非阻塞
  /// durationSec: 播放时长（秒），0 = 播完为止
  bool playUrl(const String &url, int durationSec = 0) {
    if (url.isEmpty()) return false;

    ensureAudio_();
    audio_->setPinout(bck_, ws_, din_);
    audio_->setVolume(50);
    audio_->setBufsize(64 * 1024, 2048);
    audio_->connecttohost(url.c_str());

    urlDuration_ = (unsigned long)durationSec * 1000;
    urlStart_ = millis();
    urlPlaying_ = true;
    state_ = "playing";
    return true;
  }

  /// 在 loop() 中调用，驱动 URL 流播放
  /// 返回 true 表示仍在播放
  bool loopUrl() {
    if (!urlPlaying_ || !audio_) return false;

    if (urlDuration_ > 0 && millis() - urlStart_ >= urlDuration_) {
      stopUrl();
      return false;
    }

    audio_->loop();
    return true;
  }

  /// 停止播放
  void stopUrl() {
    if (audio_) {
      delete audio_;
      audio_ = nullptr;
    }
    urlPlaying_ = false;
    state_ = "ready";
  }

  /// 是否正在播放
  bool isPlaying() const { return urlPlaying_; }

  /// 状态 JSON
  String getStatusJson() {
    char buf[128];
    snprintf(buf, sizeof(buf), "{\"name\": \"%s\", \"state\": \"%s\"}",
             name.c_str(), state_);
    return String(buf);
  }

private:
  /// 懒创建 Audio 实例
  void ensureAudio_() {
    if (!audio_) {
      audio_ = new Audio();
    }
  }

  const char *state_ = "pending";
  int bck_ = 0, ws_ = 0, din_ = 0;
  String name;

  Audio *audio_ = nullptr;
  bool urlPlaying_ = false;
  unsigned long urlStart_ = 0;
  unsigned long urlDuration_ = 0;
};
