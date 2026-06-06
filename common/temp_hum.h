#pragma once
// ═══════════════════════════════════════
// 温湿度传感器 DHT11
// ═══════════════════════════════════════

#include <Arduino.h>
#include <DHT.h>

class TempHum {
public:
  TempHum() {}

  TempHum(int pin, uint8_t type, String name)
      : dht(new DHT(pin, type)), pin(pin), name(name) {}

  /// 初始化 DHT11 硬件
  void begin() { dht->begin(); delay(1000); }

  float getTemp() { return dht->readTemperature(); }
  float getHumidity() { return dht->readHumidity(); }

  /// 状态 JSON
  String getStatusJson() {
    char buf[128];
    snprintf(buf, sizeof(buf),
             "{\"name\": \"%s\", \"temp\": %.2f, \"humidity\": %.2f}",
             name.c_str(), getTemp(), getHumidity());
    return String(buf);
  }

private:
  DHT *dht = nullptr;
  String name;
  int pin;
};
