/**
 * 智能音箱 - 按键说话（Push-to-Talk）
 *
 * 流程：按住按钮录音 → MiMo ASR → MiMo Chat → MiMo TTS → 播放
 *
 * 硬件：
 *   ESP32-S3 + MAX98357A 喇叭 + INMP441 麦克风
 *   按钮 → GPIO 11
 *   麦克风 → SCK=GPIO40, WS=GPIO41, SD=GPIO39, L/R=GPIO42
 *   喇叭 → BCLK=GPIO21, LRC=GPIO47, DIN=GPIO20
 */
#include <Arduino.h>
#include <WiFi.h>
#include "common/speaker.h"
#include "common/mic_recorder.h"
#include "common/stt_client.h"
#include "common/ai_chat.h"
#include "common/tts_client.h"

// ─── 硬件引脚 ───
static constexpr uint8_t PIN_BUTTON = 11;
static constexpr uint8_t PIN_SPEAKER_BCLK = 21;
static constexpr uint8_t PIN_SPEAKER_LRC = 47;
static constexpr uint8_t PIN_SPEAKER_DIN = 20;

// ─── 录音缓冲（PSRAM 8MB，可录 10 秒） ───
static constexpr size_t REC_BUF_SIZE = 16000 * 2 * MIC_MAX_RECORD_SEC;
static uint8_t *recBuf = nullptr;

// ─── 模块实例 ───
static MicRecorder mic(PIN_BUTTON, {40, 41, 39, 42});
static Speaker speaker(PIN_SPEAKER_BCLK, PIN_SPEAKER_LRC, PIN_SPEAKER_DIN, I2S_NUM_0, "speaker");
static SttClient stt;
static AiChat aiChat;
static TtsClient tts;

// ─── WiFi ───
void connectWiFi() {
    Serial.printf("[WiFi] 连接 %s ", WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    while (WiFi.status() != WL_CONNECTED) {
        delay(300);
        Serial.print(".");
    }
    Serial.printf("\n[WiFi] 已连接, IP: %s\n", WiFi.localIP().toString().c_str());
}

// ─── 处理一次对话 ───
void handleConversation() {
    Serial.println("\n=== 按住按钮开始说话 ===");

    // 1. 等待按钮按下
    while (!mic.isPressed()) { delay(10); }
    delay(50);

    // 2. 录音
    size_t audioLen = mic.record(recBuf, REC_BUF_SIZE);
    if (audioLen < 3200) {
        Serial.println("[对话] 录音太短，已跳过");
        return;
    }

    // 3. MiMo ASR：语音 → 文字
    String userText;
    Serial.println("[对话] 正在识别语音...");
    if (!stt.recognize(recBuf, audioLen, userText)) {
        Serial.println("[对话] 语音识别失败");
        return;
    }
    Serial.printf("[对话] 你说: %s\n", userText.c_str());

    // 4. MiMo Chat：文字 → AI 回答
    String aiAnswer;
    Serial.println("[对话] AI 正在思考...");
    if (!aiChat.ask(userText, aiAnswer)) {
        Serial.println("[对话] AI 问答失败");
        return;
    }
    Serial.printf("[对话] AI: %s\n", aiAnswer.c_str());

    // 5. MiMo TTS：文字 → 语音播放
    Serial.println("[对话] 正在合成语音...");
    String audioUrl;
    if (!tts.synthesize(aiAnswer, audioUrl)) {
        Serial.println("[对话] TTS 合成失败");
        return;
    }

    // 播放
    if (audioUrl.startsWith("base64:")) {
        // TTS 返回 base64 数据，ESP32 无法直接播放
        // 需要解码后通过 I2S 输出
        Serial.println("[对话] TTS 返回 base64 数据，暂不支持直接播放");
        Serial.println("[对话] 请确认 MiMo TTS 是否支持 URL 输出模式");
    } else {
        Serial.printf("[对话] 播放音频: %s\n", audioUrl.c_str());
        speaker.playUrl(audioUrl);
        while (speaker.isUrlPlaying()) {
            speaker.loopUrl();
            delay(1);
        }
        Serial.println("[对话] 播放完成");
    }
}

void setup() {
    Serial.begin(115200);
    delay(2000);

    Serial.println("\n========================================");
    Serial.println("  智能音箱 v2.0 MiMo");
    Serial.println("========================================");
    Serial.printf("[BOOT] CPU freq: %d MHz\n", getCpuFrequencyMhz());
    Serial.printf("[BOOT] Free heap: %u bytes\n", ESP.getFreeHeap());
    Serial.printf("[BOOT] Flash size: %u bytes\n", ESP.getFlashChipSize());

    // 检查 PSRAM
    Serial.printf("[BOOT] PSRAM size: %u bytes\n", ESP.getPsramSize());
    Serial.printf("[BOOT] Free PSRAM: %u bytes\n", ESP.getFreePsram());

    // 测试 PSRAM 分配
    Serial.printf("[INIT] 需要 PSRAM: %u bytes\n", REC_BUF_SIZE);
    recBuf = (uint8_t *)ps_malloc(REC_BUF_SIZE);
    if (!recBuf) {
        Serial.println("[ERROR] PSRAM 分配失败！尝试 malloc...");
        recBuf = (uint8_t *)malloc(REC_BUF_SIZE);
    }
    if (!recBuf) {
        Serial.println("[ERROR] 内存分配全部失败！");
        return;
    }
    Serial.printf("[INIT] 缓冲区分配成功: %u bytes\n", REC_BUF_SIZE);

    // 测试 WiFi
    Serial.printf("[INIT] WiFi SSID: %s\n", WIFI_SSID);
    connectWiFi();

    // 测试 I2S 喇叭
    Serial.println("[INIT] 初始化喇叭...");
    speaker.begin();
    Serial.println("[INIT] 喇叭 OK");

    // 测试麦克风
    Serial.println("[INIT] 初始化麦克风...");
    mic.begin();
    Serial.println("[INIT] 麦克风 OK");

    Serial.println("[INIT] ====== 全部初始化完成 ======");
    Serial.printf("[INIT] API Key: %s...\n", String(MIMO_API_KEY).substring(0, 8).c_str());
    Serial.println("按住 GPIO 11 按钮说话...");
}

void loop() {
    if (mic.isPressed()) {
        handleConversation();
        Serial.println("\n按住按钮继续对话...");
    }
    delay(50);
}
