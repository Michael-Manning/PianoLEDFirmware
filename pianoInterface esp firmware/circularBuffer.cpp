#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "circularBuffer.h"
#include "m_error.h"


namespace{
    Event eventBuffer[eventBufferSize];
    unsigned int head = 0; // always positioned on the next free index. One after actual data
    unsigned int tail = 0; // always positioned on the first index of actual data
    unsigned int eventCount = 0;

    bool bufferInit = false;
    SemaphoreHandle_t xMutex;
}

void InitBuffer(){
    xMutex = xSemaphoreCreateMutex();
    ::bufferInit = true;
}

int EventQueLength(){
    assert_fatal(bufferInit, ErrorCode::BUFFER_OVERRUN);
    return eventCount;
}

void PushEvent(Event e){
    if(!assert_fatal(bufferInit, ErrorCode::BUFFER_OVERRUN)){
        return;
    }
    xSemaphoreTake(xMutex, portMAX_DELAY);

    if(!assert_fatal(eventCount < eventBufferSize, ErrorCode::BUFFER_OVERRUN)){
        return;
    }

    if(!assert_fatal(head < eventBufferSize, ErrorCode::IMPOSSIBLE_INTERNAL)){
        return;
    }

    eventBuffer[head] = e;
    head++;
    eventCount++;

    // End of array reached. Cirle back to the front
    if(head == eventBufferSize){
        head = 0;
    }

    xSemaphoreGive(xMutex);
}
bool PopEvent(Event * e){
    assert_fatal(bufferInit, ErrorCode::BUFFER_OVERRUN);

    if(eventCount == 0){
        return false;
    }
    xSemaphoreTake(xMutex, portMAX_DELAY);

    if(!assert_fatal(tail != eventBufferSize, ErrorCode::IMPOSSIBLE_INTERNAL)){
        return false;
    }

    // Data actually get coppied. Don't want to worry about it randomy being
    // deallocated when the buffer circles back around.
    *e = eventBuffer[tail];
    tail++;
    eventCount --;

    if(tail == eventBufferSize){
        tail = 0;
    }

    xSemaphoreGive(xMutex);

    return true;
}