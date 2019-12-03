#ifndef NETWORK_H
#define NETWORK_H

namespace network
{
    void beginConnection();

    bool waitForConnection();

    void startServer();

    bool isConnected();

    void pollEvents();
}

#endif