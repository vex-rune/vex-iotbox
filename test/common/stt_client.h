/**
 * MiMo 语音识别客户端（ASR）
 *
 * 调用小米 MiMo-V2.5-ASR 将 PCM 音频转为文字
 * API: https://api.xiaomimimo.com/v1/audio/transcriptions
 *
 * 用法：
 *   SttClient stt;
 *
 *   // pcmData：MicRecorder.record() 返回的 PCM 数据
 *   // pcmLen：数据字节数
 *   String text;
 *   if (stt.recognize(pcmData, pcmLen, text)) {
 *       Serial.println(text);  // 识别结果，如 "打开客厅灯"
 *   }
 *
 * 注意：
 *   - 需要 WiFi 已连接
 *   - PCM 格式：16kHz, 16bit, 单声道（左声道）
 *   - 编译时需传入 -DMIMO_API_KEY=\"your_key\"
 * 依赖：WiFiClientSecure（ESP32 内置）
 */
#pragma once

#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

#ifndef MIMO_API_KEY
#define MIMO_API_KEY "YOUR_MIMO_KEY"
#endif

static constexpr const char *ASR_HOST = "api.xiaomimimo.com";
static constexpr const char *ASR_PATH = "/v1/audio/transcriptions";
static constexpr const char *ASR_MODEL = "mimo-v2.5-asr";
static constexpr int ASR_SAMPLE_RATE = 16000;

class SttClient {
public:
    /**
     * 识别 PCM 音频
     * @param pcmData   PCM 数据指针
     * @param pcmLen    数据长度（字节）
     * @param text      输出：识别结果文本
     * @return true 成功
     */
    bool recognize(const uint8_t *pcmData, size_t pcmLen, String &text) {
        if (!WiFi.isConnected()) {
            Serial.println("[STT] WiFi 未连接");
            return false;
        }

        // 构建 multipart/form-data 请求
        String boundary = "----ESP32Boundary";
        String body;

        // model 字段
        body += "--" + boundary + "\r\n";
        body += "Content-Disposition: form-data; name=\"model\"\r\n\r\n";
        body += String(ASR_MODEL) + "\r\n";

        // language 字段
        body += "--" + boundary + "\r\n";
        body += "Content-Disposition: form-data; name=\"language\"\r\n\r\n";
        body += "zh\r\n";

        // file 字段（WAV 头 + PCM 数据）
        // 构建一个最小 WAV 头
        uint32_t dataSize = pcmLen;
        uint32_t fileSize = 44 + dataSize;
        uint8_t wavHeader[44];
        // RIFF header
        memcpy(wavHeader, "RIFF", 4);
        memcpy(wavHeader + 4, &fileSize, 4);
        memcpy(wavHeader + 8, "WAVE", 4);
        // fmt sub-chunk
        memcpy(wavHeader + 12, "fmt ", 4);
        uint32_t fmtSize = 16;
        memcpy(wavHeader + 16, &fmtSize, 4);
        uint16_t audioFormat = 1;  // PCM
        memcpy(wavHeader + 20, &audioFormat, 2);
        uint16_t channels = 1;
        memcpy(wavHeader + 22, &channels, 2);
        uint32_t sampleRate = ASR_SAMPLE_RATE;
        memcpy(wavHeader + 24, &sampleRate, 4);
        uint32_t byteRate = ASR_SAMPLE_RATE * 1 * 2;
        memcpy(wavHeader + 28, &byteRate, 4);
        uint16_t blockAlign = 2;
        memcpy(wavHeader + 32, &blockAlign, 2);
        uint16_t bitsPerSample = 16;
        memcpy(wavHeader + 34, &bitsPerSample, 2);
        // data sub-chunk
        memcpy(wavHeader + 36, "data", 4);
        memcpy(wavHeader + 40, &dataSize, 4);

        body += "--" + boundary + "\r\n";
        body += "Content-Disposition: form-data; name=\"file\"; filename=\"audio.wav\"\r\n";
        body += "Content-Type: audio/wav\r\n\r\n";

        // 计算总长度：body前半 + wavHeader + pcmData + 尾部
        size_t headerLen = body.length();
        String tail = "\r\n--" + boundary + "--\r\n";
        size_t totalLen = headerLen + 44 + dataSize + tail.length();

        // 发送请求
        WiFiClientSecure client;
        client.setInsecure();

        Serial.printf("[STT] 连接 %s ...\n", ASR_HOST);
        if (!client.connect(ASR_HOST, 443)) {
            Serial.println("[STT] 连接失败");
            return false;
        }

        client.printf("POST %s HTTP/1.1\r\n", ASR_PATH);
        client.printf("Host: %s\r\n", ASR_HOST);
        client.printf("Authorization: Bearer %s\r\n", MIMO_API_KEY);
        client.println("Content-Type: multipart/form-data; boundary=----ESP32Boundary");
        client.printf("Content-Length: %d\r\n", totalLen);
        client.println("Connection: close");
        client.println();
        client.print(body);

        // 写入 WAV 头
        client.write(wavHeader, 44);

        // 写入 PCM 数据
        size_t written = 0;
        while (written < pcmLen) {
            size_t chunk = min((size_t)4096, pcmLen - written);
            client.write(pcmData + written, chunk);
            written += chunk;
        }

        // 写入尾部
        client.print(tail);

        // 读取响应
        String resp = readHttpResponse(client);
        client.stop();

        if (resp.isEmpty()) {
            Serial.println("[STT] 响应为空");
            return false;
        }

        JsonDocument respDoc;
        if (deserializeJson(respDoc, resp)) {
            Serial.printf("[STT] JSON 解析失败, 响应前200字: %s\n",
                          resp.substring(0, 200).c_str());
            return false;
        }

        // OpenAI 兼容格式: {"text": "..."}
        if (respDoc["text"].is<String>()) {
            text = respDoc["text"].as<String>();
        } else {
            Serial.println("[STT] 响应中未找到 text 字段");
            return false;
        }

        Serial.printf("[STT] 识别结果: %s\n", text.c_str());
        return true;
    }

private:
    String readHttpResponse(WiFiClient &client) {
        String response;
        unsigned long timeout = millis();
        while (!client.available()) {
            if (millis() - timeout > 30000) {
                Serial.println("[STT] 响应超时");
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
