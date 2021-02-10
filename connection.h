#pragma once

#include <Bela.h>

#include "mixer.h"
#include "wootrx.h"
#include "woottx.h"

class Connection {
public:
    Connection();

    int connect(const char * host);
    void processFrame(BelaContext *context, Mixer &mixer);

    int writeToGuiBuffer(Gui &gui, int offset);

    void setRxQueueSize(int size);
    int rxQueueSize() const;

private:
    WootRx m_rx;
    WootTx m_tx;
};