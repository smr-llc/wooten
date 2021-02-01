#include <Bela.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <string.h>
#include <errno.h>

#include "config.h"
#include "rxudp.h"


WootRx::WootRx() :
	m_sock(0),
	m_currBuf(0)
{
	memset((char *) &m_bufLen, 0, sizeof(int) * RX_BUFFERS);
}


int WootRx::init(int port) {
	m_peerSockLen = sizeof(m_peerAddr);

	if ((m_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
	{
		printf("failed to create udp receive socket, got errno %d\n", errno);
		fflush(stdout);
		return 1;
	}

	struct sockaddr_in addr;
	memset((char *) &addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if( bind(m_sock, (struct sockaddr*)&addr, sizeof(addr) ) == -1)
	{
		printf("failed to bind udp receive socket, got errno %d\n", errno);
		fflush(stdout);
		return 2;
	}

	return 0;
}


void WootRx::rxUdp(void* rxArg) {
	WootRx* rx = (WootRx*)rxArg;

	printf("Starting rxUdp...\n");
	fflush(stdout);

	while (!Bela_stopRequested()) {
		rx->readAllUdp();
		usleep(150);
	}

	printf("Ending rxUdp\n");
	fflush(stdout);
}


int WootRx::readAllUdp() {
	int readNum = 0;
	int nBuf = m_currBuf;
	while (true) {
		int prevBuf = nBuf;
		nBuf = (nBuf + 1) % RX_BUFFERS;
		m_bufLen[nBuf] = 0;
		m_bufLen[nBuf] = recvfrom(m_sock,
										m_buf[nBuf],
										NETBUFF_BYTES,
										MSG_DONTWAIT,
										(struct sockaddr *) &m_peerAddr,
										&m_peerSockLen);
		if (m_bufLen[nBuf] <= 0) {
			m_currBuf = prevBuf;
			return readNum;
		}
		readNum++;
	}
}


void WootRx::writeReceivedFrame(BelaContext *context, int nChan) {
	readAllUdp();
	if (m_bufLen[m_currBuf] != NETBUFF_BYTES) {
		for(unsigned int n = 0; n < context->audioFrames; n++) {
			for(unsigned int ch = 0; ch < nChan; ch++) {
				audioWrite(context, n, ch, 0);
			}
		}
	}
	else {
		for(unsigned int n = 0; n < context->audioFrames; n++) {
			for(unsigned int ch = 0; ch < nChan; ch++) {
				audioWrite(context, n, ch, ((float*)m_buf[m_currBuf])[(n * 2) + ch]);
			}
		}
		m_bufLen[m_currBuf] = 0;
	}
}
