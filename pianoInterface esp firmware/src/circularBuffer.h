#ifndef CIRCULARBUFFER_H
#define CIRCULARBUFFER_H

#include "lighting/lighting.h"

// union Event{
//     enum {NO_PARAM, ANIMATION_MODE} eventType = NO_PARAM;
//     lights::AnimationMode mode;
//     union{
//         void (*action)();
//         void (*animationAction)(lights::AnimationMode);
//     };
// };
union Event{
    void (*action)();
};

constexpr unsigned int eventBufferSize = 100;

void InitBuffer();

int EventQueLength();

void PushEvent(Event e);
void PushEvent(void (*Action)());
#define _pushModeSwitchEvent(mode) PushEvent([]() {lights::setAnimationMode(mode);});
bool PopEvent(Event * e);

#endif