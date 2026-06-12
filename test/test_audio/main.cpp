/**
 * 音频流播放测试 (audio_test)
 *
 * 功能：
 *   - 连接 WiFi
 *   - 通过 Speaker 类播放 URL 音频流（MP3/WAV）
 *   - 播放 30 秒后停止
 *
 * I2S 引脚（MAX98357A）：
 *   BCLK = GPIO21, LRC = GPIO47, DOUT = GPIO20
 */

#include <Arduino.h>
#include "common/speaker.h"
#include "common/wifi_manager.h"

// ── 引脚定义 ──
constexpr uint8_t PIN_I2S_BCLK = 21;
constexpr uint8_t PIN_I2S_LRC  = 47;
constexpr uint8_t PIN_I2S_DOUT = 20;

// ── 测试 URL（可替换为任意 MP3/WAV 直链）──
constexpr const char *TEST_URL =
    "http://m10.music.126.net/20260609152236/b923923577aaa5977ab87cbb800a313d/ymusic/5353/0f0f/0358/d99739615f8e5153d77042092f07fd77.mp3?vuutv=ESDvMedTWPAXtlQYym3LGg3TQ14/3vA/Dj1LNp4ABMf97fPjIacWI9Kv3LSH6PROR6krFBRFljqWNzv801A3RRJV5dXhJSou+tAxghc4E0X6zvEealgGaLKjEfU3ay5PL1L9pjJtMrj/mEf0BwbdqZPWYedE3zN3Da8NOGUfZQBYS5xTwFdDN+g31kTMDCzcpKNs4mHpzVRWzlppUpLJQ7pcML4M6Q57oux0JaxnNBZ9hSVwSfS9emG7Ac5cv/mHv0yP2u9Z8nFOSXCpcuEn7lrK4rdVXSDKR5fvLj3Ie+aHe0EYuvMwzr7CpYy4K6QV0jmVdNFAALtnt4n30EX7yGyZT/ORcFr8p8NjGVCSZADlRycS8gQAeqk4NCG6qiGh&cdntag=bWFyaz1vc193ZWIscXVhbGl0eV9zdGFuZGFyZA";
constexpr int PLAY_DURATION_SEC = 30;

// ── 全局对象 ──
Speaker speaker(PIN_I2S_BCLK, PIN_I2S_LRC, PIN_I2S_DOUT, I2S_NUM_0, "speaker");
WifiManager wifi;

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=== Audio Stream Test ===");

  // 初始化喇叭
  speaker.begin();
  Serial.println("Speaker init OK");

  // 连接 WiFi
  Serial.printf("Connecting to WiFi: %s\n", WIFI_SSID);
  wifi.begin(WIFI_SSID, WIFI_PASS);
  unsigned long start = millis();
  while (!wifi.isConnected() && millis() - start < 15000) {
    delay(100);
    Serial.print(".");
  }
  if (!wifi.isConnected()) {
    Serial.println("\nWiFi FAILED");
    return;
  }
  Serial.printf("\nWiFi OK, IP: %s\n", wifi.localIP().toString().c_str());

  // 开始播放
  Serial.printf("Playing: %s\n", TEST_URL);
  Serial.printf("Duration: %d seconds\n", PLAY_DURATION_SEC);
  speaker.playUrl(TEST_URL, PLAY_DURATION_SEC);
  Serial.println("Streaming...");
}

void loop() {
  if (speaker.isUrlPlaying()) {
    speaker.loopUrl();
  }
}
