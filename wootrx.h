#pragma once

#include <Bela.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdint.h>
#include <liquid/liquid.h>

#include "config.h"
#include "levelmeter.h"
#include "mixer.h"

class WootRx {
public:
    WootRx();
    ~WootRx();

    int init(in_port_t port);
    void writeReceivedFrame(BelaContext *context, Mixer &mixer);

    static void rxUdp(void*);

    LevelMeter * const levelMeter();

    void setRxQueueSize(int size);
    int rxQueueSize() const;

private:
    LevelMeter m_meter;
    resamp2_crcf m_resampler;
    int m_sock;
    int m_readPos;
    int m_writePos;
    struct sockaddr_in m_peerAddr;
	socklen_t m_peerSockLen;
    float m_buf[RINGBUFF_SAMPLES];
    int m_rxQueueSize;
};
