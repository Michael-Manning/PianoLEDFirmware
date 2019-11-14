#ifndef NETWORK_H
#define NETWORK_H

#include "m_constants.h"

#ifndef _PIANOSIZE
#define _PIANOSIZE 88
#endif

//#include <music.h>

enum class PlayMode{
    indicate,
    waiting,
    fade
};

extern PlayMode mode;

namespace network
{
    bool connect();

    void pollEvents();

}

#endif