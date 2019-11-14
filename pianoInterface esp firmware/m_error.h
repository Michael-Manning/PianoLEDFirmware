#ifndef MERROR_H
#define MERROR_H

#include <Arduino.h>
#include "lighting.h"

extern bool errorLock;

enum class ErrorCode{
    IMPOSSIBLE_INTERNAL = 255,
    INVALID_LED_INDEX = 129, // INTERNAL: Function call in lighting was missused
    WIFI_NO_SSID_AVAIL = 130, // WIFI was unable to connect
    WIFI_CONNECTION_FAILED = 131,
    WIFI_CONNECTION_LOST = 132,
    WIFI_DISCONNECTED = 133,
    WIFI_CONNECTION_TIMEOUT = 134
};

bool fatalError(ErrorCode errorCode, bool exec = false);

#define assert_fatal(expr, errorCode) fatalError(errorCode, expr)


#endif