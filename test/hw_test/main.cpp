/**
 * 硬件测试固件 (hw_test)
 *
 * 通过 MQTT 命令测试各组件：
 *   topic: iotbox_test/cmd
 *   1 - RGB LED（GPIO48，彩虹轮转）
 *   2 - LED 红/黄/绿（GPIO15/16/17，依次闪烁）
 *   3 - 按钮 1/2/3（GPIO11/12/13，等待按下）
 *   4 - DHT11 温湿度（GPIO4）
 *   5 - OLED 显示器（GPIO8/9）
 *   6 - 喇叭 MAX98357A（I2S，播放 440Hz）
 *   7 - 打印所有引脚状态
 *   8 - 全部测试
 */

#include <Arduino.h>
#include <stdexcept>

// ── 公共组件 ──
#include "button.h"
#include "display.h"
#include "indicator.h"
#include "mqtt_manager.h"
#include "rgb_led.h"
#include "speaker.h"
#include "temp_hum.h"
#include "wifi_manager.h"

// ═══════════════════════════════════════
// 引脚定义
// ═══════════════════════════════════════
#define PIN_LED_RED 15
#define PIN_LED_YEL 16
#define PIN_LED_GRN 17
#define PIN_BTN_1 11
#define PIN_BTN_2 12
#define PIN_BTN_3 13
#define PIN_DHT 4
#define PIN_OLED_SCL 2
#define PIN_OLED_SDA 1
#define PIN_I2S_BCLK 21
#define PIN_I2S_LRC 47
#define PIN_I2S_DOUT 20

// ═══════════════════════════════════════
// MQTT 主题
// ═══════════════════════════════════════
#define MQTT_PUB "iotbox_test/state"
#define MQTT_CMD "iotbox_test/cmd"

// ═══════════════════════════════════════
// 全局实例
// ═══════════════════════════════════════
RgbLed rgb(48);
Indicator ledRed(PIN_LED_RED, "ledRed");
Indicator ledYel(PIN_LED_YEL, "ledYel");
Indicator ledGrn(PIN_LED_GRN, "ledGrn");
Button btn1(PIN_BTN_1, "btn1");
Button btn2(PIN_BTN_2, "btn2");
Button btn3(PIN_BTN_3, "btn3");
TempHum dht11(PIN_DHT, DHT11, "dht11");
Display oled(PIN_OLED_SDA, PIN_OLED_SCL, 128, 32, "oled");
Speaker speaker(PIN_I2S_BCLK, PIN_I2S_LRC, PIN_I2S_DOUT, I2S_NUM_0, "speaker");
WifiManager wifi;
MqttManager mqtt;

// ═══════════════════════════════════════
// 测试函数
// ═══════════════════════════════════════

/// 1: RGB LED 彩虹轮转
void testRgb() {
  Serial.println("[1] RGB LED test start (GPIO48)");
  for (int i = 0; i < 256; i++) {
    rgb.rainbow();
    if (i % 64 == 0) {
      Serial.printf("  rainbow step %d/256\n", i);
      mqtt.publish(MQTT_PUB, rgb.getStatusJson(), true);
      delay(100);
    }
  }
  rgb.off();
  Serial.println("[1] RGB LED test done");
}

/// 2: LED 依次闪烁
void testLed() {
  Serial.println("[2] LED test start");
  Serial.printf("  RED ON  (GPIO%d)\n", PIN_LED_RED);
  ledRed.on();
  mqtt.publish(MQTT_PUB, ledRed.getStatusJson(), true);
  delay(1000);
  ledRed.off();
  mqtt.publish(MQTT_PUB, ledRed.getStatusJson(), true);
  Serial.printf("  YEL ON  (GPIO%d)\n", PIN_LED_YEL);
  ledYel.on();
  mqtt.publish(MQTT_PUB, ledYel.getStatusJson(), true);
  delay(1000);
  ledYel.off();
  mqtt.publish(MQTT_PUB, ledYel.getStatusJson(), true);
  Serial.printf("  GRN ON  (GPIO%d)\n", PIN_LED_GRN);
  ledGrn.on();
  mqtt.publish(MQTT_PUB, ledGrn.getStatusJson(), true);
  mqtt.publish(MQTT_PUB, ledGrn.getStatusJson(), true);
  delay(1000);
  ledGrn.off();
  mqtt.publish(MQTT_PUB, ledGrn.getStatusJson(), true);
  Serial.println("[2] LED test done");

  // 全部亮
  ledRed.on();
  ledYel.on();
  ledGrn.on();
  Serial.println("[2] LED test done");
  mqtt.publish(MQTT_PUB, ledRed.getStatusJson(), true);
  mqtt.publish(MQTT_PUB, ledYel.getStatusJson(), true);
  mqtt.publish(MQTT_PUB, ledGrn.getStatusJson(), true);
  delay(1000);
  // 全部灭
  ledRed.off();
  ledYel.off();
  ledGrn.off();
  mqtt.publish(MQTT_PUB, ledRed.getStatusJson(), true);
  mqtt.publish(MQTT_PUB, ledYel.getStatusJson(), true);
  mqtt.publish(MQTT_PUB, ledGrn.getStatusJson(), true);
  Serial.println("[2] LED test done");
}

/// 3: 按钮检测（5 秒超时）
void testButton() {
  Serial.printf("[3] Button test start (GPIO%d/%d/%d, 5s timeout)\n", PIN_BTN_1,
                PIN_BTN_2, PIN_BTN_3);
  unsigned long start = millis();
  while (millis() - start < 5000) {
    if (btn1.isPressed()) {
      Serial.printf("  btn1 PRESSED (GPIO%d)\n", PIN_BTN_1);
      mqtt.publish(MQTT_PUB, btn1.getStatusJson(), true);
    }
    if (btn2.isPressed()) {
      Serial.printf("  btn2 PRESSED (GPIO%d)\n", PIN_BTN_2);
      mqtt.publish(MQTT_PUB, btn2.getStatusJson(), true);
    }
    if (btn3.isPressed()) {
      Serial.printf("  btn3 PRESSED (GPIO%d)\n", PIN_BTN_3);
      mqtt.publish(MQTT_PUB, btn3.getStatusJson(), true);
    }
    delay(50);
  }
  Serial.println("[3] Button test done");
}

/// 4: DHT11 温湿度
void testDht() {
  Serial.printf("[4] DHT11 test start (GPIO%d)\n", PIN_DHT);
  float t = dht11.getTemp();
  float h = dht11.getHumidity();
  if (isnan(t) || isnan(h)) {
    Serial.println("  DHT11 read FAILED (nan)");
    mqtt.publish(MQTT_PUB, dht11.getStatusJson(), true);
  } else {
    Serial.printf("  DHT11 OK: temp=%.2f C, humi=%.2f %%\n", t, h);
    mqtt.publish(MQTT_PUB, dht11.getStatusJson(), true);
  }
  Serial.println("[4] DHT11 test done");
}

/// 5: OLED 显示
void testOled(JsonDocument json) {
  Serial.printf("[5] OLED test start (SDA=%d, SCL=%d)\n", PIN_OLED_SDA,
                PIN_OLED_SCL);

  // 解析文字参数(从json中提取)
  String text = json["text"].as<String>();
  if (text.isEmpty()) {
    text = "HW Test OK";
  }
  
  Serial.printf("  text: %s\n", text.c_str());

  // 获取 size
  int x = 0;
  if (json["x"]) {
    x = json["x"].as<int>();
  }
  int y = 16;
  if (json["y"]) {
    y = json["y"].as<int>();
  }

  Serial.printf("  x: %d, y: %d\n", x, y);

  // 显示文字
  oled.showText(text.c_str(), x, y);
  mqtt.publish(MQTT_PUB, oled.getStatusJson(), true);
  delay(1000);
  Serial.println("[5] OLED test done");
}

/// 6: 喇叭播放 440Hz
void testSpeaker() {
  Serial.printf("[6] Speaker test start (BCLK=%d, LRC=%d, DIN=%d)\n",
                PIN_I2S_BCLK, PIN_I2S_LRC, PIN_I2S_DOUT);

  const int sampleRate = 44100;
  const int duration = sampleRate / 4;
  int16_t *buf = (int16_t *)malloc(duration * sizeof(int16_t) * 2);
  if (!buf) {
    Serial.println("  malloc FAILED");
    return;
  }
  Serial.printf("  buffer allocated: %d bytes\n",
                duration * (int)sizeof(int16_t) * 2);

  for (int i = 0; i < duration; i++) {
    float t = (float)i / sampleRate;
    int16_t val = (sinf(2.0 * PI * 440.0 * t) > 0) ? 16000 : -16000;
    buf[i * 2] = val;
    buf[i * 2 + 1] = val;
  }

  Serial.println("  playing 440Hz square wave...");
  speaker.play(buf, duration * 2);
  delay(300);
  speaker.mute();
  free(buf);

  Serial.println("[6] Speaker test done");
}

// ═══════════════════════════════════════
// MQTT 消息回调
// ═══════════════════════════════════════
void onMqttMessage(const char *topic, const char *payload,
                   unsigned int length) {

  // 解析为 JSON 字符串
  String cmd = String(payload);
  // 去掉首尾空格
  cmd.trim();
  // 解析 JSON 字符串
  JsonDocument doc;
  deserializeJson(doc, cmd);

  // 提取 cmd 字段
  cmd = doc["cmd"].as<String>();
  // 其他参数
  String param = doc["param"].as<String>();

  Serial.printf("\n>>> MQTT received: topic=%s, payload=%s, len=%d\n", topic,
                cmd.c_str(), length);

  try {
    if (cmd == "1")
      testRgb();
    else if (cmd == "2")
      testLed();
    else if (cmd == "3")
      testButton();
    else if (cmd == "4")
      testDht();
    else if (cmd == "5")
      testOled(doc);
    else if (cmd == "6")
      testSpeaker();
  } catch (const std::exception &e) {
    Serial.printf(">>> EXCEPTION: %s\n", e.what());
    doc["error"] = e.what();
  } catch (...) {
    Serial.println(">>> UNKNOWN EXCEPTION");
    doc["error"] = "unknown exception";
  }

  Serial.printf(">>> Publishing result to %s\n", MQTT_PUB);
  mqtt.publishJson(MQTT_PUB, doc);
  Serial.println(">>> Result published");

  
  Serial.println("\n=== Ready! Send MQTT cmd to iotbox_test/cmd ===");
  Serial.println("  1=RGB  2=LED  3=Btn  4=DHT  5=OLED  6=SPK  8=ALL");
}

// ═══════════════════════════════════════
// setup & loop
// ═══════════════════════════════════════
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=== HW Test Ready ===");

  Serial.println("Initializing hardware...");
  rgb.begin(50);
  Serial.println("  RGB LED (GPIO48) OK");
  ledRed.begin();
  Serial.println("  LED Red (GPIO15) OK");
  ledYel.begin();
  Serial.println("  LED Yel (GPIO16) OK");
  ledGrn.begin();
  Serial.println("  LED Grn (GPIO17) OK");
  dht11.begin();
  Serial.println("  DHT11 (GPIO4) OK");
  speaker.begin();
  Serial.println("  Speaker I2S OK");
  oled.begin();
  Serial.println("  OLED (SDA8/SCL9) OK");
  Serial.println("Hardware init done");

  // ── 排查测试：保留 WiFi，禁用 MQTT ──
  Serial.printf("Connecting to WiFi: SSID=%s\n", WIFI_SSID);
  wifi.begin(WIFI_SSID, WIFI_PASS);
  unsigned long wifiStart = millis();
  while (!wifi.isConnected() && millis() - wifiStart < 15000) {
    delay(100);
    Serial.print(".");
  }
  Serial.printf("\nWiFi connected, IP: %s\n",
                wifi.localIP().toString().c_str());

  Serial.printf("Connecting to MQTT: host=%s, port=%d\n", MQTT_HOST,
                MQTT_PORT);
  mqtt.begin(MQTT_HOST, MQTT_PORT, "hw_test", MQTT_USER, MQTT_PASS,
             onMqttMessage);
  while (!mqtt.isConnected()) {
    mqtt.loop();
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nMQTT connected");
  mqtt.subscribe(MQTT_CMD);

  Serial.println("\n=== HW Test: OLED + WiFi (no MQTT) ===");
}

void loop() {
  mqtt.loop();
  delay(1000);
}
