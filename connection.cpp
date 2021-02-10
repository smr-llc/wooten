#include "connection.h"
#include "portbroker.h"

Connection::Connection() {

}

int Connection::connect(const char * host) {
	printf("Attempting to create new peer connection with %s...\n", host);
    fflush(stdout);

    m_host = host;

    PortAssignment assignment;
    if (PortBroker::getPortAssignmentForHost(host, assignment) != 0) {
		printf("ERROR: Failed to negotiate connection with host\n");
        return -1;
    }

	printf("Got port assignment\n");
    fflush(stdout);

    int result = m_tx.init(assignment.peerAddr, assignment.port);
    if (result != 0) {
		printf("ERROR: Failed to initialize connection tx component\n");
        return result;
    }

	printf("Initialized transmitter\n");
    fflush(stdout);

    result = m_rx.init(assignment.port);
    if (result != 0) {
		printf("ERROR: Failed to initialize connection rx component\n");
        return result;
    }

	printf("Initialized receiver\n");
    fflush(stdout);

	printf("Finished negotiating connection with %s\n", host);
    fflush(stdout);

	Bela_runAuxiliaryTask(WootTx::txUdp, 50, &m_tx);
	Bela_runAuxiliaryTask(WootRx::rxUdp, 50, &m_rx);

    return 0;
}

void Connection::processFrame(BelaContext *context, Mixer &mixer) {
    m_tx.sendFrame(context);
    m_rx.writeReceivedFrame(context, mixer);
}

void Connection::initializeReadBuffer(Gui &gui) {
    m_guiReadBuffer = gui.setBuffer('d', 5);
}

int Connection::writeToGuiBuffer(Gui &gui, int offset) {
    std::vector<char> nameVec(m_host.begin(), m_host.end()); 
    gui.sendBuffer(offset, nameVec);
    m_rx.levelMeter()->writeToGuiBuffer(gui, offset+1);
    return 2;
}

void Connection::readFromGuiBuffer(Gui &gui) {
    DataBuffer& buffer = gui.getDataBuffer(m_guiReadBuffer);
    int* guiData = buffer.getAsInt();
    int rxQSize = guiData[0];
    float rxLevel = ((float)guiData[1]) / 1000.0f;

    if (rxQSize > 0) {
        m_rx.setRxQueueSize(rxQSize);
    }

    if (rxLevel >= 0.0f) {
        m_rx.setRxLevel(rxLevel);
    }
}

void Connection::setRxQueueSize(int size) {
	m_rx.setRxQueueSize(size);
}

int Connection::rxQueueSize() const {
	return m_rx.rxQueueSize();
}