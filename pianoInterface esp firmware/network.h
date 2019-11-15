#ifndef NETWORK_H
#define NETWORK_H

#include "m_constants.h"

#ifndef _PIANOSIZE
#define _PIANOSIZE 88
#endif

//#include <music.h>

enum class PlayMode{
    Idle = 1,
    Indicate,
    Waiting,
    SongLoading
};

extern PlayMode globalMode;

namespace network
{
    void beginConnection();
    bool waitForConnection();
    void startServer();
    bool isConnected();

    void pollEvents();

}

#endif