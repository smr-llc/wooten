#pragma once

#include <sys/socket.h>
#include <arpa/inet.h>
#include <mutex>
#include "config.h"

class WootTx {
public:
    WootTx();

    int connect(const char* host, int port);
    void sendFrame(BelaContext *context, int nChan);

    static void txUdp(void*);

private:
    int m_sock;
    int m_bufWritten;
    int m_bufRead;
    struct sockaddr_in m_peerAddr;
	socklen_t m_peerAddrLen;
    float m_buf[NETBUFF_SAMPLES];
};
