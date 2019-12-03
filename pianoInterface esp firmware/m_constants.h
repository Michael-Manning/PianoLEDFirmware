#ifndef CONSTANTS_H
#define CONSTANTS_H

//#define MICROBRUTE_DEBUG

#ifdef MICROBRUTE_DEBUG
#define _KEYCOUNT 53
#define _PIANOSIZE 88
#else
constexpr uint8_t _KEYCOUNT = 53;
constexpr uint8_t _PIANOSIZE =   88;
#endif
constexpr uint8_t _PIXELPIN = 13; 
#define _SSID "Dennis"
#define _NETWORKKEY "7804663459"
#define _DEVICE_NETWORK_NAME "PianoESP"

#endif