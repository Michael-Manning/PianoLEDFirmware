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
#else
void print(const char * message){
}
void println(const char * message ){
}
#endif
}

