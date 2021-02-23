#include "Bela.h"
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>

#include "config.h"
#include "woottx.h"
#include "wootpkt.h"

WootTx::WootTx() :
	m_sock(0),
	m_readPos(0),
	m_writePos(0)
{
}

int WootTx::init(struct in_addr addr, in_port_t port, std::string connId) {
	if ( (m_sock = socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, IPPROTO_UDP)) == -1)
	{
		printf("Failed to create outgoing udp socket!\n");
		fflush(stdout);
		return 1;	
	}

	m_peerAddrLen = sizeof(m_peerAddr);
	memset((char *) &m_peerAddr, 0, m_peerAddrLen);
	m_peerAddr.sin_family = AF_INET;
	m_peerAddr.sin_port = port;
	memcpy(&m_peerAddr.sin_addr, &addr, sizeof(struct in_addr));

	m_connId = connId;

	return 0;
}

int WootTx::levelMeterCount() {
	return 2;
}

LevelMeter * const WootTx::levelMeter(int channel) {
	if (channel >= 0 && channel < levelMeterCount()) {
		return &m_meters[channel];
	}
	return nullptr;
}

void WootTx::txUdp(void* txArg) {
	WootTx *tx = (WootTx*)txArg;

	if (tx->m_connId.size() == 0) {
		printf("Cannot start txUdp without connection ID...\n");
		fflush(stdout);
		return;
	}

	printf("Starting txUdp for connection %s...\n", tx->m_connId.c_str());
	fflush(stdout);

	WootPkt pkt;
	pkt.header.magic = WOOT_PKT_MAGIC;
	memcpy(pkt.header.connId, tx->m_connId.c_str(), 6);
	uint16_t seq = 0;
	
	while(!Bela_stopRequested())
	{
		int diff = tx->m_writePos - tx->m_readPos;
		if (diff >= NETBUFF_SAMPLES || diff < 0) {
			pkt.header.seq = seq++;

			for (int i = 0; i < NETBUFF_SAMPLES; i++) {
				pkt.samples[i] = (int16_t)(tx->m_buf[tx->m_readPos + i] * 32767.0);
			}
			tx->m_readPos += NETBUFF_SAMPLES;
			tx->m_readPos %= RINGBUFF_SAMPLES;

			sendto(tx->m_sock,
					(char*)&pkt,
					sizeof(WootPkt),
					0,
					(struct sockaddr *) &tx->m_peerAddr,
					tx->m_peerAddrLen);
		}
		else {
			usleep(250);
		}
	}

	printf("Ending txUdp\n");
	fflush(stdout);
}

void WootTx::sendFrame(BelaContext *context) {
	for(unsigned int n = 0; n < NETBUFF_SAMPLES; n++) {
		float val = 0.0;
		for(unsigned int ch = 0; ch < 2; ch++){
			// downsample to 22.05
			float sample = audioRead(context, n * 2, ch);
			val += sample;
			m_meters[ch].feedSample(sample);
		}
		m_buf[m_writePos + n] = val / 2.0f;
	}
	m_writePos += NETBUFF_SAMPLES;
	m_writePos %= RINGBUFF_SAMPLES;
}