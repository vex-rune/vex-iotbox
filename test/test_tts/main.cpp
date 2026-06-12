/**
 * MiniMax TTS URL 播放测试 (tts_test)
 *
 * 功能：
 *   - 连接 WiFi
 *   - 调用 MiniMax T2A API（非流式，返回音频 URL）
 *   - 通过 Speaker 类直接播放 URL 音频
 *
 * 依赖：
 *   - esphome/ESP32-audioI2S（Speaker 内部使用）
 */

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "common/speaker.h"
#include "common/wifi_manager.h"

// ═══════════════════════════════════════
// 配置
// ═══════════════════════════════════════
#ifndef MINIMAX_API_KEY
#define MINIMAX_API_KEY "YOUR_API_KEY_HERE"
#endif

constexpr const char *API_URL = "https://api.minimaxi.com/v1/t2a_v2";
constexpr const char *TTS_TEXT = "你好，这是一个语音合成测试。";

// I2S 引脚（MAX98357A）
constexpr int PIN_BCLK = 21;
constexpr int PIN_LRC  = 47;
constexpr int PIN_DOUT = 20;

// ── 全局对象 ──
Speaker speaker(PIN_BCLK, PIN_LRC, PIN_DOUT, I2S_NUM_0, "speaker");
WifiManager wifi;

// ═══════════════════════════════════════
// TTS：获取音频 URL 并播放
// ═══════════════════════════════════════
void ttsPlay(const String &text) {
    Serial.printf("\n[TTS] Text: %s\n", text.c_str());

    // ── 构造请求体 ──
    JsonDocument req;
    req["model"] = "speech-2.8-hd";
    req["text"] = text;
    req["stream"] = false;

    JsonObject voiceSetting = req["voice_setting"].to<JsonObject>();
    voiceSetting["voice_id"] = "male-qn-qingse";
    voiceSetting["speed"] = 1;
    voiceSetting["vol"] = 1;
    voiceSetting["pitch"] = 0;

    JsonObject audioSetting = req["audio_setting"].to<JsonObject>();
    audioSetting["sample_rate"] = 32000;
    audioSetting["bitrate"] = 128000;
    audioSetting["format"] = "mp3";
    audioSetting["channel"] = 1;

    req["output_format"] = "url";

    String body;
    serializeJson(req, body);

    Serial.printf("[TTS] Request: %d bytes\n", body.length());

    // ── 发送 HTTP POST ──
    HTTPClient http;
    http.begin(API_URL);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Authorization", "Bearer " MINIMAX_API_KEY);

    int httpCode = http.POST(body);
    Serial.printf("[TTS] HTTP status: %d\n", httpCode);

    if (httpCode != 200) {
        String err = http.getString();
        Serial.printf("[TTS] ERROR: %s\n", err.c_str());
        http.end();
        return;
    }

    // ── 解析响应，提取音频 URL ──
    String resp = http.getString();
    http.end();

    JsonDocument doc;
    if (deserializeJson(doc, resp)) {
        Serial.println("[TTS] JSON parse failed");
        return;
    }

    // 检查错误
    int statusCode = doc["base_resp"]["status_code"];
    if (statusCode != 0) {
        Serial.printf("[TTS] API error %d: %s\n",
                      statusCode, doc["base_resp"]["status_msg"].as<const char *>());
        return;
    }

    // 提取音频 URL
    const char *audioUrl = doc["data"]["audio"];
    if (!audioUrl || strlen(audioUrl) == 0) {
        Serial.println("[TTS] No audio URL in response");
        return;
    }

    int audioLen = doc["extra_info"]["audio_length"];
    int sampleRate = doc["extra_info"]["audio_sample_rate"];
    Serial.printf("[TTS] Audio URL: %s\n", audioUrl);
    Serial.printf("[TTS] Length: %d ms, Sample rate: %d Hz\n", audioLen, sampleRate);

    // ── 通过 Speaker 播放 ──
    Serial.println("[TTS] Playing...");
    speaker.playUrl(audioUrl);

    while (speaker.isUrlPlaying()) {
        speaker.loopUrl();
        delay(1);
    }

    Serial.println("[TTS] Done");
}

// ═══════════════════════════════════════
// Arduino 入口
// ═══════════════════════════════════════
void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n=== MiniMax TTS URL Test ===");

    speaker.begin();

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

    ttsPlay(TTS_TEXT);
}

void loop() {
    if (speaker.isUrlPlaying()) {
        speaker.loopUrl();
    }
    delay(1);
}
