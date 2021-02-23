#pragma once

#include "sessionprotocol.h"
#include "mixer.h"
#include "config.h"
#include "connection.h"

#include <Bela.h>
#include <libraries/Gui/Gui.h>
#include <mutex>
#include <atomic>
#include <vector>

class Session {
public:
    Session();

    int setup();

    void start(std::string sessId = "");
    void stop();
    void processFrame(BelaContext *context, Mixer &mixer);
    void writeToGuiBuffer(Gui &gui);

    bool isActive() const;
    std::string sessionId() const;

    static void manageSession(void*);
    static void rxUdp(void*);
private:
    void manageSessionImpl();
    void rxUdpImpl();

    std::mutex m_connMutex;
    std::map<std::string, Connection*> m_connections;

    std::string m_sessId;
    int m_rxSock;
    std::atomic<bool> m_udpActive;
    std::atomic<bool> m_managerActive;
    std::atomic<bool> m_terminate;

    AuxiliaryTask m_manageSessionTask;
    AuxiliaryTask m_rxUdpTask;

    JoinedData m_myJoinedData;
};