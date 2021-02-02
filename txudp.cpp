#include "Bela.h"
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>

#include "config.h"
#include "txudp.h"

WootTx::WootTx() :
	m_sock(0),
	m_readPos(0),
	m_writePos(0)
{
	
}

int WootTx::connect(const char* host, int port) {
	m_peerAddrLen = sizeof(m_peerAddr);

	if ( (m_sock = socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, IPPROTO_UDP)) == -1)
	{
		printf("Failed to create outgoing udp socket!\n");
		fflush(stdout);
		return 1;	
	}

	memset((char *) &m_peerAddr, 0, m_peerAddrLen);
	m_peerAddr.sin_family = AF_INET;
	m_peerAddr.sin_port = htons(port);
	if (inet_aton(host, &m_peerAddr.sin_addr) == 0) 
	{
		printf("Failed to resolve peer address!\n");
		fflush(stdout);
		return 2;
	}

	return 0;
}

void WootTx::txUdp(void* txArg) {
	WootTx *tx = (WootTx*)txArg;

	printf("Starting txUdp...\n");
	fflush(stdout);

	int16_t buf16[NETBUFF_SAMPLES];
	
	while(!Bela_stopRequested())
	{
		int diff = tx->m_writePos - tx->m_readPos;
		if (diff >= NETBUFF_SAMPLES || diff < 0) {
			for (int i = 0; i < NETBUFF_SAMPLES; i++) {
				buf16[i] = (int16_t)(tx->m_buf[tx->m_readPos + i] * 32767.0);
			}
			tx->m_readPos += NETBUFF_SAMPLES;
			tx->m_readPos %= RINGBUFF_SAMPLES;

			sendto(tx->m_sock,
					(char*)buf16,
					NETBUFF_BYTES,
					0,
					(struct sockaddr *) &tx->m_peerAddr,
					tx->m_peerAddrLen);
		}
		else {
			usleep(150);
		}
	}

	printf("Ending txUdp\n");
	fflush(stdout);
}

void WootTx::sendFrame(BelaContext *context, int nChan) {
	for(unsigned int n = 0; n < context->audioFrames; n += 2) {
		double val = 0.0;
		for(unsigned int ch = 0; ch < nChan; ch++){
			val += audioRead(context, n, ch);
		}
		if (nChan > 0) {
			val /= nChan;
		}
		m_buf[m_writePos + n] = (float)val;
	}
	m_writePos += (context->audioFrames / 2);
	m_writePos %= RINGBUFF_SAMPLES;
}