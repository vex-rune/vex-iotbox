/**
 * MiMo TTS 客户端
 *
 * 调用小米 MiMo-V2.5-TTS 将文字转为语音 URL
 * API: https://api.xiaomimimo.com/v1/chat/completions
 *
 * 用法：
 *   TtsClient tts;
 *
 *   String text = "门已打开";
 *   String audioUrl;
 *   if (tts.synthesize(text, audioUrl)) {
 *       if (!audioUrl.startsWith("base64:")) {
 *           // URL 模式：通过 Speaker 播放
 *           speaker.playUrl(audioUrl);
 *           while (speaker.isUrlPlaying()) {
 *               speaker.loopUrl();
 *               delay(1);
 *           }
 *       }
 *   }
 *
 * 注意：
 *   - 需要 WiFi 已连接
 *   - 输出可能是 URL 或 base64 数据，需根据前缀判断
 *   - 编译时需传入 -DMIMO_API_KEY=\"your_key\"
 * 依赖：WiFiClientSecure（ESP32 内置）, Speaker（用于播放）
 */
#pragma once

#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

#ifndef MIMO_API_KEY
#define MIMO_API_KEY "YOUR_MIMO_KEY"
#endif

static constexpr const char *TTS_HOST = "api.xiaomimimo.com";
static constexpr const char *TTS_PATH = "/v1/chat/completions";
static constexpr const char *TTS_MODEL = "mimo-v2.5-tts";

class TtsClient {
public:
    /**
     * 文字转语音并返回音频 URL
     * @param text      要合成的文字
     * @param audioUrl  输出：音频文件 URL
     * @return true 成功
     */
    bool synthesize(const String &text, String &audioUrl) {
        if (!WiFi.isConnected()) {
            Serial.println("[TTS] WiFi 未连接");
            return false;
        }

        WiFiClientSecure client;
        client.setInsecure();

        Serial.printf("[TTS] 连接 %s ...\n", TTS_HOST);
        if (!client.connect(TTS_HOST, 443)) {
            Serial.println("[TTS] 连接失败");
            return false;
        }

        // MiMo TTS 格式：文本放在 assistant 消息中
        JsonDocument reqDoc;
        reqDoc["model"] = TTS_MODEL;

        JsonArray messages = reqDoc["messages"].to<JsonArray>();
        JsonObject assistant = messages.add<JsonObject>();
        assistant["role"] = "assistant";
        assistant["content"] = text;

        // 音频输出配置
        JsonObject audio = reqDoc["audio"].to<JsonObject>();
        audio["format"] = "mp3";
        audio["voice"] = "mimo_default";

        String body;
        serializeJson(reqDoc, body);

        client.printf("POST %s HTTP/1.1\r\n", TTS_PATH);
        client.printf("Host: %s\r\n", TTS_HOST);
        client.println("Content-Type: application/json");
        client.printf("Authorization: Bearer %s\r\n", MIMO_API_KEY);
        client.printf("Content-Length: %d\r\n", body.length());
        client.println("Connection: close");
        client.println();
        client.print(body);

        String resp = readHttpResponse(client);
        client.stop();

        if (resp.isEmpty()) {
            Serial.println("[TTS] 响应为空");
            return false;
        }

        JsonDocument respDoc;
        if (deserializeJson(respDoc, resp)) {
            Serial.printf("[TTS] JSON 解析失败, 响应前200字: %s\n",
                          resp.substring(0, 200).c_str());
            return false;
        }

        // 提取音频数据（base64）或 URL
        // OpenAI 兼容格式可能返回 audio.data (base64) 或 audio.url
        JsonObject msg = respDoc["choices"][0]["message"];
        if (msg["audio"].is<JsonObject>()) {
            JsonObject audioObj = msg["audio"];
            if (audioObj["url"].is<String>()) {
                audioUrl = audioObj["url"].as<String>();
            } else if (audioObj["data"].is<String>()) {
                audioUrl = "base64:" + audioObj["data"].as<String>();
            }
        }

        if (audioUrl.isEmpty()) {
            Serial.println("[TTS] 未获取到音频数据");
            return false;
        }

        Serial.printf("[TTS] 合成成功, 音频: %s\n",
                      audioUrl.substring(0, 80).c_str());
        return true;
    }

private:
    String readHttpResponse(WiFiClient &client) {
        String response;
        unsigned long timeout = millis();
        while (!client.available()) {
            if (millis() - timeout > 30000) {
                Serial.println("[TTS] 响应超时");
                return "";
            }
            delay(10);
        }
        while (client.available()) {
            String line = client.readStringUntil('\n');
            if (line == "\r" || line.isEmpty()) break;
        }
        while (client.available()) {
            response += client.readString();
        }
        return response;
    }
};
