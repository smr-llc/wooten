#pragma once

#include <sys/socket.h>
#include <arpa/inet.h>
#include "config.h"

#define RX_BUFFERS 2

class WootRx {
public:
    WootRx();

    int init(int port);
    void writeReceivedFrame(BelaContext *context, int nChan);

    static void rxUdp(void*);

private:
    int readAllUdp();

    int m_sock;
    struct sockaddr_in m_peerAddr;
	socklen_t m_peerSockLen;
    int m_currBuf;
    int m_bufLen[RX_BUFFERS];
    char m_buf[RX_BUFFERS][NETBUFF_BYTES];
};
