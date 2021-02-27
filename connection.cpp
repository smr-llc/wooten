#include "connection.h"
#include "portbroker.h"

#include <string.h>

Connection::Connection() {

}

Connection::~Connection() {
    m_tx.stop();
}

int Connection::connect(const JoinedData &data, const JoinedData &sessionData) {
    m_connId = std::string(data.connId, 6);
    struct in_addr host;

    if (memcmp(data.connId, sessionData.connId, 6) == 0) {
        return -1;
    }

    if (memcmp(&data.publicAddr, &sessionData.publicAddr, sizeof(struct in_addr)) == 0) {
        if (memcmp(&data.privateAddr, &sessionData.privateAddr, sizeof(struct in_addr)) == 0) {
            return -1;
        }
        memcpy(&host, &data.privateAddr, sizeof(struct in_addr));
    }
    else {
        memcpy(&host, &data.publicAddr, sizeof(struct in_addr));
    }
    uint16_t portNum = ntohs(data.port);
    m_host = inet_ntoa(host);

	printf("Attempting to create new peer connection with %s on port %d...\n", m_host.c_str(), portNum);
    fflush(stdout);

    std::string sessionConnId = std::string(sessionData.connId, 6);
    int result = m_tx.init(host, data.port, sessionConnId);
    if (result != 0) {
		printf("ERROR: Failed to initialize connection tx component\n");
        return result;
    }

	printf("Finished setting up peer connection with %s\n", m_host.c_str());
    fflush(stdout);

	Bela_runAuxiliaryTask(WootTx::txUdp, 50, &m_tx);

    return 0;
}

void Connection::handleFrame(const WootPkt &pkt) {
    m_rx.handleFrame(pkt);
}

void Connection::processFrame(BelaContext *context, Mixer &mixer) {
    m_tx.sendFrame(context);
    m_rx.writeReceivedFrame(context, mixer);
}

int Connection::writeToGuiBuffer(Gui &gui, int offset) {
    std::vector<char> nameVec(m_host.begin(), m_host.end()); 
    gui.sendBuffer(offset, nameVec);
    m_rx.levelMeter()->writeToGuiBuffer(gui, offset+1);
    return 2;
}

void Connection::readFromGuiBuffer(Gui &gui, unsigned int bufferId) {
    DataBuffer& buffer = gui.getDataBuffer(bufferId);
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

std::string Connection::connId() const {
	return m_connId;
}