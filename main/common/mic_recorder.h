/**
 * I2S 麦克风录音模块
 * 基于 INMP441 数字麦克风，使用 I2S_NUM_1 采集 PCM 数据
 */
#pragma once

#include <Arduino.h>
#include <driver/i2s.h>

/// 录音参数
static constexpr i2s_port_t MIC_I2S_PORT = I2S_NUM_1;
static constexpr int MIC_SAMPLE_RATE = 16000;
static constexpr i2s_bits_per_sample_t MIC_BITS = I2S_BITS_PER_SAMPLE_16BIT;
static constexpr i2s_channel_fmt_t MIC_FORMAT = I2S_CHANNEL_FMT_ONLY_LEFT;
static constexpr int MIC_MAX_RECORD_SEC = 10;
static constexpr size_t MIC_BUF_SIZE = 4096;

/// 引脚配置 (INMP441)
struct MicPins {
    uint8_t sck;
    uint8_t ws;
    uint8_t sd;
    uint8_t lr;  // 接 GND = 左声道，接 3.3V = 右声道
};

class MicRecorder {
public:
    MicRecorder(uint8_t buttonPin, const MicPins &pins)
        : buttonPin_(buttonPin), pins_(pins) {}

    void begin() {
        pinMode(buttonPin_, INPUT_PULLUP);

        i2s_config_t cfg{};
        cfg.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX);
        cfg.sample_rate = MIC_SAMPLE_RATE;
        cfg.bits_per_sample = MIC_BITS;
        cfg.channel_format = MIC_FORMAT;
        cfg.communication_format = I2S_COMM_FORMAT_STAND_I2S;
        cfg.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1;
        cfg.dma_buf_count = 8;
        cfg.dma_buf_len = 256;
        cfg.use_apll = false;
        cfg.tx_desc_auto_clear = false;
        cfg.fixed_mclk = 0;
        i2s_driver_install(MIC_I2S_PORT, &cfg, 0, nullptr);

        i2s_pin_config_t pinCfg{};
        pinCfg.bck_io_num = pins_.sck;
        pinCfg.ws_io_num = pins_.ws;
        pinCfg.data_out_num = I2S_PIN_NO_CHANGE;
        pinCfg.data_in_num = pins_.sd;
        i2s_set_pin(MIC_I2S_PORT, &pinCfg);
        i2s_zero_dma_buffer(MIC_I2S_PORT);
    }

    /// 检测按钮是否按下
    bool isPressed() const { return digitalRead(buttonPin_) == LOW; }

    /**
     * 开始录音，按住按钮期间持续采集
     * @param outBuf   输出缓冲区（调用者负责分配）
     * @param maxBytes 缓冲区最大字节数
     * @return 实际写入的字节数
     */
    size_t record(uint8_t *outBuf, size_t maxBytes) {
        size_t totalRead = 0;
        i2s_zero_dma_buffer(MIC_I2S_PORT);

        Serial.println("[MIC] 开始录音...");
        while (isPressed() && totalRead < maxBytes) {
            size_t bytesRead = 0;
            esp_err_t err = i2s_read(MIC_I2S_PORT, outBuf + totalRead,
                                     min(MIC_BUF_SIZE, maxBytes - totalRead),
                                     &bytesRead, pdMS_TO_TICKS(100));
            if (err != ESP_OK || bytesRead == 0) break;
            totalRead += bytesRead;
        }
        Serial.printf("[MIC] 录音完成: %u bytes (%.1f 秒)\n",
                      totalRead, (float)totalRead / (MIC_SAMPLE_RATE * 2));
        return totalRead;
    }

private:
    uint8_t buttonPin_;
    MicPins pins_;
};
