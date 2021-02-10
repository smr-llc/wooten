#include "wootbase.h"
#include <wchar.h>

void WootBase::setup(BelaContext *context) {
	// Setup GUI
	m_gui.setup(context->projectName);
	m_gui.setControlDataCallback(WootBase::guiControlHandler, this);

	m_coreBuffer = m_gui.setBuffer('d', 10);
	printf("Core Buffer ID: %d\n", m_coreBuffer);
}

void WootBase::processFrame(BelaContext *context) {

	// Read input audio into level meters
	for (unsigned int n = 0; n < BLOCK_SIZE; n++) {
		for (unsigned int ch = 0; ch < 2; ch++) {
			m_inputMeters[ch].feedSample(audioRead(context, n, ch));
		}
	}


	// Process input/output audio for each peer connection
	for (Connection* connection : m_connections) {
		connection->processFrame(context, m_mixer);
	}

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

		m_gui.sendBuffer(9, m_connections.size());
		int connectionBufferOffset = 10;
		for (Connection* connection : m_connections) {
			int increment = connection->writeToGuiBuffer(m_gui, connectionBufferOffset);
			connectionBufferOffset += increment;
		}

		// read data from gui
		DataBuffer& buffer = m_gui.getDataBuffer(m_coreBuffer);
		int* guiData = buffer.getAsInt();
		m_monitorSelf = (guiData[0] == 1) ? true : false;
		m_monitorSelfLevel = ((float)guiData[1]) / 1000.0f;
	}


	// if (tx.levelMeter(0)->shouldRead()) {
	// 	if (rx.levelMeter()->shouldRead()) {
	// 		gui.sendBuffer(0, (unsigned int)(rx.levelMeter()->avg() * 10000.0f));
	// 		gui.sendBuffer(1, (unsigned int)(rx.levelMeter()->takePeak() * 10000.0f));
	// 		gui.sendBuffer(2, (unsigned int)(rx.levelMeter()->heldPeak() * 10000.0f));
	// 	}
	// 	gui.sendBuffer(3, (unsigned int)(tx.levelMeter(0)->avg() * 10000.0f));
	// 	gui.sendBuffer(4, (unsigned int)(tx.levelMeter(0)->takePeak() * 10000.0f));
	// 	gui.sendBuffer(5, (unsigned int)(tx.levelMeter(0)->heldPeak() * 10000.0f));
	// 	gui.sendBuffer(6, (unsigned int)(tx.levelMeter(1)->avg() * 10000.0f));
	// 	gui.sendBuffer(7, (unsigned int)(tx.levelMeter(1)->takePeak() * 10000.0f));
	// 	gui.sendBuffer(8, (unsigned int)(tx.levelMeter(1)->heldPeak() * 10000.0f));

	// 	DataBuffer& buffer = gui.getDataBuffer(0);
	// 	int* guiData = buffer.getAsInt();
	// 	rx.setRxQueueSize(guiData[0]);
	// 	rx.setMonitorSelf((guiData[1] == 1) ? true : false);
	// 	rx.setMonitorSelfLevel(((float)guiData[2]) / 1000.0f);
	// }
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

	if (event == "add-connection") {
		addConnection(JSON::ws2s(json[L"host"]->AsString()).c_str());
		return true;
	}

	return false;
}


void WootBase::addConnection(const char* host) {
    Connection *conn = new Connection();
    if (conn->connect(host) != 0) {
        delete conn;
    }
    m_connections.push_back(conn);
}