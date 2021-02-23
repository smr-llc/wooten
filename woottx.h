#pragma once

#include <sys/socket.h>
#include <arpa/inet.h>

#include "config.h"
#include "levelmeter.h"

class WootTx {
public:
    WootTx();

    int init(struct in_addr addr, in_port_t port, std::string connId);
    void sendFrame(BelaContext *context);

    static void txUdp(void*);

    int levelMeterCount();
    LevelMeter * const levelMeter(int channel);

private:
    LevelMeter m_meters[2];
    int m_sock;
    int m_readPos;
    int m_writePos;
    std::string m_connId;
    struct sockaddr_in m_peerAddr;
	socklen_t m_peerAddrLen;
    float m_buf[RINGBUFF_SAMPLES];
};
