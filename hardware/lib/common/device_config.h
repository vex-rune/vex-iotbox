#pragma once

#include <Arduino.h>

// 设备 ID 通过 platformio.ini 中的 -DDEVICE_ID 宏定义
#ifndef DEVICE_ID
#define DEVICE_ID "0001"
#endif

inline String topic_upload(const char *device_id)
{
    return "dev/" + String(device_id) + "/upload";
}

inline String topic_cmd(const char *device_id)
{
    return "dev/" + String(device_id) + "/cmd";
}
