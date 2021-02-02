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
	m_readPos(0),
	m_writePos(0)
{
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


void WootRx::readAllUdp() {
	while (true) {
		int nBytes = recvfrom(m_sock,
								(char*)m_netBuf,
								NETBUFF_BYTES,
								MSG_DONTWAIT,
								(struct sockaddr *) &m_peerAddr,
								&m_peerSockLen);
		if (nBytes == NETBUFF_BYTES) {
			for (int i = 0; i < NETBUFF_SAMPLES; i++) {
				m_buf[m_writePos + i] = ((float)m_netBuf[i]) / 32768.0f;
			}
			m_writePos += NETBUFF_SAMPLES;
			m_writePos %= RINGBUFF_SAMPLES;
		}
		else {
			return;
		}
	}
}


void WootRx::writeReceivedFrame(BelaContext *context, int nChan) {
	int bufSamples = m_writePos - m_readPos;
	if (bufSamples < 0) {
		bufSamples += RINGBUFF_SAMPLES;
	}
	if (bufSamples > (NETBUFF_SAMPLES * RX_QUEUE_SIZE)) {
		m_readPos += (RINGBUFF_SAMPLES + bufSamples) - (NETBUFF_SAMPLES * RX_QUEUE_SIZE);
		m_readPos %= RINGBUFF_SAMPLES;
	}

	if (bufSamples < NETBUFF_SAMPLES) {
		for(unsigned int n = 0; n < context->audioFrames; n++) {
			for(unsigned int ch = 0; ch < nChan; ch++) {
				audioWrite(context, n, ch, 0);
			}
		}
	}
	else {
		for(unsigned int n = 0; n < NETBUFF_SAMPLES; n++) {
			for(unsigned int ch = 0; ch < nChan; ch++) {
				audioWrite(context, n * DOWNSAMPLE_FACTOR, ch, m_buf[m_readPos + n]);
				audioWrite(context, n * DOWNSAMPLE_FACTOR + 1, ch, m_buf[m_readPos + n]);
			}
		}
		m_readPos += NETBUFF_SAMPLES;
		m_readPos %= RINGBUFF_SAMPLES;
	}
}
