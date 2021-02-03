#pragma once

#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdint.h>
#include "config.h"

#define RX_BUFFERS 2

class WootRx {
public:
    WootRx();

    int init(int port);
    void writeReceivedFrame(BelaContext *context, int nChan);

    static void rxUdp(void*);

private:
    int m_sock;
    int m_readPos;
    int m_writePos;
    struct sockaddr_in m_peerAddr;
	socklen_t m_peerSockLen;
    float m_buf[RINGBUFF_SAMPLES];
};
