#pragma once

#include <Bela.h>
#include <libraries/Gui/Gui.h>
#include <list>

#include "connection.h"
#include "mixer.h"

class WootBase {
public:
    void setup(BelaContext *context);
    void processFrame(BelaContext *context);

    static void auxProcess(void *selfArg);
    static bool guiControlHandler(JSONObject &json, void *selfArg);


private:
    LevelMeter m_inputMeters[2];
    void auxProcessImpl();
    bool guiControlHandlerImpl(JSONObject &json);
    void addConnection(const char* host);

    Gui m_gui;
    Mixer m_mixer;
    std::list<Connection*> m_connections;

    unsigned int m_coreBuffer;
    
    bool m_monitorSelf;
    float m_monitorSelfLevel;
};