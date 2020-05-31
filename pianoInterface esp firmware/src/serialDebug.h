#ifndef SERIALDEBUG_H
#define SERIALDEBUG_H

#define ENABLE_SERIAL

namespace debug
{
    void print(const char * message);
    void println(const char * message);
    void printf(char *fmt, ...);
} 

#endif

