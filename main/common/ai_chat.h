/**
 * MiMo Chat 客户端
 * 调用小米 MiMo ChatCompletion API 进行文本问答
 * API: https://api.xiaomimimo.com/v1/chat/completions
 */
#pragma once

#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

#ifndef MIMO_API_KEY
#define MIMO_API_KEY "YOUR_MIMO_KEY"
#endif

static constexpr const char *CHAT_HOST = "api.xiaomimimo.com";
static constexpr const char *CHAT_PATH = "/v1/chat/completions";
static constexpr const char *CHAT_MODEL = "MiMo-V2.5";

static constexpr const char *CHAT_SYSTEM_PROMPT =
    "你是一个简洁的智能助手。请用中文回答用户的问题，回答不超过100字。";

class AiChat {
public:
    bool ask(const String &question, String &response) {
        if (!WiFi.isConnected()) {
            Serial.println("[CHAT] WiFi 未连接");
            return false;
        }

        WiFiClientSecure client;
        client.setInsecure();

        Serial.printf("[CHAT] 连接 %s ...\n", CHAT_HOST);
        if (!client.connect(CHAT_HOST, 443)) {
            Serial.println("[CHAT] 连接失败");
            return false;
        }

        JsonDocument reqDoc;
        reqDoc["model"] = CHAT_MODEL;
        reqDoc["max_tokens"] = 256;
        reqDoc["temperature"] = 0.7;

        JsonArray messages = reqDoc["messages"].to<JsonArray>();
        JsonObject sys = messages.add<JsonObject>();
        sys["role"] = "system";
        sys["content"] = CHAT_SYSTEM_PROMPT;
        JsonObject user = messages.add<JsonObject>();
        user["role"] = "user";
        user["content"] = question;

        String body;
        serializeJson(reqDoc, body);

        client.printf("POST %s HTTP/1.1\r\n", CHAT_PATH);
        client.printf("Host: %s\r\n", CHAT_HOST);
        client.println("Content-Type: application/json");
        client.printf("Authorization: Bearer %s\r\n", MIMO_API_KEY);
        client.printf("Content-Length: %d\r\n", body.length());
        client.println("Connection: close");
        client.println();
        client.print(body);

        String resp = readHttpResponse(client);
        client.stop();

        if (resp.isEmpty()) {
            Serial.println("[CHAT] 响应为空");
            return false;
        }

        JsonDocument respDoc;
        if (deserializeJson(respDoc, resp)) {
            Serial.println("[CHAT] JSON 解析失败");
            return false;
        }

        response = respDoc["choices"][0]["message"]["content"].as<String>();
        Serial.printf("[CHAT] 回答: %s\n", response.c_str());
        return !response.isEmpty();
    }

private:
    String readHttpResponse(WiFiClient &client) {
        String response;
        unsigned long timeout = millis();
        while (!client.available()) {
            if (millis() - timeout > 15000) return "";
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
