#ifndef CIRCULARBUFFER_H
#define CIRCULARBUFFER_H

#include "lighting.h"
#include <Arduino.h>
#include "network.h"
constexpr unsigned int eventBufferSize = 100;

union EventData
{
    struct animationChange
    {
       lights::AnimationMode mode;
       unsigned int data;
    };
    struct PlayModeChange{
        PlayMode mode;
    }
    struct error{
        byte errorCode;
    };   
};


struct Event{
    void (*action)();
    EventData data;
};

void InitBuffer();
int EventQueLength();
void PushEvent(Event e);
bool PopEvent(Event * e);

#endif