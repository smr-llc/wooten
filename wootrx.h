#pragma once

#include <Bela.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdint.h>

#include "config.h"
#include "levelmeter.h"
#include "mixer.h"
#include "wootpkt.h"

class WootRx {
public:
    WootRx();

    void writeReceivedFrame(BelaContext *context, Mixer &mixer);

    LevelMeter * const levelMeter();

    void setRxQueueSize(int size);
    int rxQueueSize() const;

    void setRxLevel(float level);
    float rxLevel() const;

    void handleFrame(const WootPkt &pkt);

private:
    LevelMeter m_meter;
    int m_readPos;
    int m_writePos;
    float m_buf[RINGBUFF_SAMPLES];
    int m_rxQueueSize;
    float m_rxLevel;
    uint16_t m_lastSeq;
};
