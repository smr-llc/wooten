#include "wootbase.h"
#include <wchar.h>

int WootBase::setup(BelaContext *context) {
	// Setup GUI
	m_gui.setup(context->projectName);
	m_gui.setControlDataCallback(WootBase::guiControlHandler, this);

	m_coreBuffer = m_gui.setBuffer('d', 10);
	printf("Core Buffer ID: %d\n", m_coreBuffer);

	if (m_session.setup(m_gui) != 0) {
		return -1;
	}

	return 0;
}

void WootBase::processFrame(BelaContext *context) {

	// Read input audio into level meters
	for (unsigned int n = 0; n < BLOCK_SIZE; n++) {
		for (unsigned int ch = 0; ch < 2; ch++) {
			m_inputMeters[ch].feedSample(audioRead(context, n, ch));
		}
	}

	// Process input/output audio for each peer connection
	m_session.processFrame(context, m_mixer);

	// Monitor input in output, if enabled
	if (m_monitorSelf) {
        m_mixer.addLayer(m_monitorSelfLevel);
        for(unsigned int n = 0; n < context->audioFrames; n++) {
            float sample = audioRead(context, n, 0) / 2.0f;
            sample += audioRead(context, n, 1) / 2.0f;
            m_mixer.writeSample(n, sample);
        }
    }

	// Write summed audio to DAC
	m_mixer.flushToDac(context);


    // Perform GUI buffer updates
	if (m_inputMeters[0].shouldRead()) {
		// write data to gui
		m_inputMeters[0].writeToGuiBuffer(m_gui, 1);
		m_inputMeters[1].writeToGuiBuffer(m_gui, 2);

		if (m_session.isActive()) {
			std::string sessId = m_session.sessionId();
			std::vector<char> sessIdVec(sessId.begin(), sessId.end());
			m_gui.sendBuffer(3, 1);
			m_gui.sendBuffer(4, sessIdVec);
		}
		else {
			m_gui.sendBuffer(3, 0);
			m_gui.sendBuffer(4, std::vector<char>());
		}

		m_session.writeToGuiBuffer();

		// read data from gui
		DataBuffer& buffer = m_gui.getDataBuffer(m_coreBuffer);
		int* guiData = buffer.getAsInt();
		m_monitorSelf = (guiData[0] == 1) ? true : false;
		m_monitorSelfLevel = ((float)guiData[1]) / 1000.0f;
	}
}

void WootBase::auxProcess(void *selfArg) {
    WootBase *base = (WootBase*)selfArg;

	printf("Starting WootBase...\n");
	fflush(stdout);

    base->auxProcessImpl();

	printf("WootBase Finished\n");
	fflush(stdout);
}

void WootBase::auxProcessImpl() {
    while (!Bela_stopRequested()) {
        sleep(1);
    }
}


bool WootBase::guiControlHandler(JSONObject &json, void *selfArg) {
	printf("Calling GUI control handler...\n");
	fflush(stdout);

    WootBase *base = (WootBase*)selfArg;
	return base->guiControlHandlerImpl(json);
}

std::wstring jsonToString(JSONObject &json) {
	std::wstring str = L"{\n";
	for (auto const &pair : json) {
		str += L"  ";
		str += L"\"" + pair.first + L"\": " + JSON::Stringify(pair.second) + L"\n";
	}
	str += L"}\n";
	return str;
}

bool WootBase::guiControlHandlerImpl(JSONObject &json) {
	printf("Got GUI control data: %s\n", JSON::ws2s(jsonToString(json)).c_str());
	fflush(stdout);

	if (json.count(L"event") < 1) {
		return false;
	}

	std::string event = JSON::ws2s(json[L"event"]->AsString());

	if (event == "join-session") {
		joinSession(JSON::ws2s(json[L"sessionId"]->AsString()).c_str());
		return true;
	}
	else if (event == "leave-session") {
		leaveSession();
		return true;
	}

	return false;
}

void WootBase::joinSession(std::string sessionId) {
	m_session.start(sessionId);
}

void WootBase::leaveSession() {
	m_session.stop();
}