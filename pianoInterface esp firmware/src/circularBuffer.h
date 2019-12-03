#ifndef CIRCULARBUFFER_H
#define CIRCULARBUFFER_H

struct Event{
    void (*action)();
};

constexpr unsigned int eventBufferSize = 100;

void InitBuffer();

int EventQueLength();

void PushEvent(Event e);
bool PopEvent(Event * e);

#endif