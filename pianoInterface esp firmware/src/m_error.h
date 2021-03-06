#ifndef MERROR_H
#define MERROR_H

enum class ErrorCode{
    IMPOSSIBLE_INTERNAL = 255,
    INVALID_LED_INDEX = 129, // INTERNAL: Function call in lighting was missused
    WIFI_NO_SSID_AVAIL = 130, // WIFI was unable to connect
    WIFI_CONNECTION_FAILED = 131,
    WIFI_CONNECTION_LOST = 132,
    WIFI_DISCONNECTED = 133,
    WIFI_CONNECTION_TIMEOUT = 134,
    BUFFER_OVERRUN = 135, // INTERNAL: Exceded the max length of a circular buffer
    TCP_MESSAGE_INCOMPLETE = 136, // A message was recieved of the wrong length or was timed out
    SONG_LOAD_DISCONTINUITY = 137, // Error detected in song loading process
    INVALID_SONG_FRAME_INDEX = 138, // Exceded max song length while loaded, or accsess ilegally
    USB_HOST_INITIALISATION = 139, // Could not communicate with the MAX3421E module
    USB_TIMEOUT = 140,
    NULL_SONG_FRAME = 141,
    INVALID_LOOP_SETTING = 142,
    NOT_IMPLIMENTED = 143, // Unfinished section of code was accessed by accident
    NO_ERROR = 144,
    INVALID_SETTING = 145,
    SONG_LOAD_UNEXPECTED_FRAME_COUNT = 146
};

bool isErrorLocked();

ErrorCode getCurrentError();

bool fatalError(ErrorCode errorCode, bool exec = false);

#define assert_fatal(expr, errorCode) fatalError(errorCode, expr)


#endif