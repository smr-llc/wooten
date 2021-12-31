#pragma once

#include <Bela.h>
#include <libraries/Gui/Gui.h>
#include <map>
#include <string>

#include "session.h"
#include "mixer.h"
#include "levelmeter.h"

class WootBase {
public:
    int setup(BelaContext *context);
    void processFrame(BelaContext *context);

    static void auxProcess(void *selfArg);
    static bool guiControlHandler(JSONObject &json, void *selfArg);

    Gui& gui();

private:
    LevelMeter m_inputMeters[2];
    void auxProcessImpl();
    bool guiControlHandlerImpl(JSONObject &json);
    void joinSession(std::string sessionId);
    void leaveSession();
    void setRecording(bool record);

    Gui m_gui;
    Mixer m_mixer;
    Session m_session;
    Recorder m_recorder;

    unsigned int m_coreBuffer;
    
    bool m_monitorSelf;
    float m_monitorSelfLevel;
    int m_outputGainLevel;
    int m_lastOutputGainLevel;
    int m_inputGainLevel;
    int m_lastInputGainLevel;
};