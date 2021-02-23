#pragma once

#include <Bela.h>

#include "mixer.h"
#include "wootrx.h"
#include "woottx.h"
#include "sessionprotocol.h"
#include "wootpkt.h"

class Connection {
public:
    Connection();
    ~Connection();

    int connect(const JoinedData &data, const JoinedData &sessionData);
    void handleFrame(const WootPkt &pkt);
    void processFrame(BelaContext *context, Mixer &mixer);

    void initializeReadBuffer(Gui &gui);
    int writeToGuiBuffer(Gui &gui, int offset);
    void readFromGuiBuffer(Gui &gui);

    void setRxQueueSize(int size);
    int rxQueueSize() const;

    std::string connId() const;

private:
    WootRx m_rx;
    WootTx m_tx;
    std::string m_host;
    std::string m_connId;
    int m_guiReadBuffer;
};