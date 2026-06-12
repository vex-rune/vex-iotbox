#pragma once
// ═══════════════════════════════════════
// 温湿度传感器（DHT11 / DHT22）
// ═══════════════════════════════════════
//
// 接线：传感器 DATA 引脚需外接 10K 上拉电阻至 3.3V
//
// 用法：
//   // 1. 创建实例（引脚, 传感器类型, 名称）
//   TempHum dht(4, DHT11, "env");
//
//   // 2. setup 中初始化硬件
//   dht.begin();
//
//   // 3. 读取数据
//   float temp = dht.getTemp();       // 温度（°C），失败返回 NaN
//   float hum  = dht.getHumidity();   // 湿度（%），失败返回 NaN
//
//   // 4. 获取状态 JSON（可直接用于 MQTT 发布）
//   String json = dht.getStatusJson();
//   // → {"name": "env", "temp": 25.30, "humidity": 60.00}
//
// 依赖：adafruit/DHT sensor library

#include <Arduino.h>
#include <DHT.h>

class TempHum {
public:
  TempHum() {}

  TempHum(int pin, uint8_t type, String name)
      : dht(new DHT(pin, type)), pin(pin), name(name) {}

  /// 初始化 DHT 硬件，必须在 setup() 中调用
  void begin() { dht->begin(); delay(1000); }

  /// 读取温度（°C），传感器无响应或失败时返回 NaN
  float getTemp() { return dht->readTemperature(); }

  /// 读取湿度（%），传感器无响应或失败时返回 NaN
  float getHumidity() { return dht->readHumidity(); }

  /// 状态 JSON，可直接传给 mqtt.publish()
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
