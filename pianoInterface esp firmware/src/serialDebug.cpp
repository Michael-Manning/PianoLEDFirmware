#include "serialDebug.h"
#include <Arduino.h>

namespace debug
{

    #ifdef ENABLE_SERIAL
void print(const char * message){
    Serial.print(message);
}
void println(const char * message ){
    Serial.println(message);
}

void printf(char *fmt, ...){
    Serial.printf(fmt);
}
#else
void print(const char * message){
}
void println(const char * message ){
}
void printf(char *fmt, ...){
}
#endif
}

