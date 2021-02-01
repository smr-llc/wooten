#include "Bela.h"
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <string.h>
#include <errno.h>

#include "config.h"
#include "txudp.h"

WootTx::WootTx() :
	m_sock(0),
	m_bufWritten(0),
	m_bufRead(0)
{
	
}

int WootTx::connect(const char* host, int port) {
	m_peerAddrLen = sizeof(m_peerAddr);

	if ( (m_sock = socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, IPPROTO_UDP)) == -1)
	{
		printf("failed to create transmit udp socket\n");
		fflush(stdout);
		return 1;	
	}

	memset((char *) &m_peerAddr, 0, m_peerAddrLen);
	m_peerAddr.sin_family = AF_INET;
	m_peerAddr.sin_port = htons(port);
	if (inet_aton(host, &m_peerAddr.sin_addr) == 0) 
	{
		printf("failed to resolve server\n");
		fflush(stdout);
		return 2;
	}

	return 0;
}

void WootTx::txUdp(void* txArg) {
	WootTx *tx = (WootTx*)txArg;

	printf("Starting txUdp...\n");
	fflush(stdout);

	char bufBuf[NETBUFF_BYTES];
	
	while(!Bela_stopRequested())
	{
		if (tx->m_bufRead != tx->m_bufWritten) {
			tx->m_bufRead = tx->m_bufWritten;
			memcpy(bufBuf, (char*)tx->m_buf, NETBUFF_BYTES);
			sendto(tx->m_sock,
					bufBuf,
					NETBUFF_BYTES,
					0,
					(struct sockaddr *) &tx->m_peerAddr,
					tx->m_peerAddrLen);
		}
		usleep(150);
	}

	printf("Ending txUdp\n");
	fflush(stdout);
}

void WootTx::sendFrame(BelaContext *context, int nChan) {
	for(unsigned int n = 0; n < context->audioFrames; n++) {
		for(unsigned int ch = 0; ch < nChan; ch++){
			m_buf[(n * nChan) + ch] = audioRead(context, n, ch);
		}
	}
	sendto(m_sock,
		m_buf,
		NETBUFF_BYTES,
		0,
		(struct sockaddr *) &m_peerAddr,
		m_peerAddrLen);
}