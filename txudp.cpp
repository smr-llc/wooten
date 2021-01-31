#include "Bela.h"
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <string.h>
#include <errno.h>

#include "config.h"
#include "txudp.h"

float outBuff[RINGBUFF_SAMPLES];
int outIdxW;
int outIdxR;

int initWootTx(WootTx* tx, const char* host, int port) {
	memset((char *)tx, 0, sizeof(WootTx));

	tx->peerAddrLen = sizeof(tx->peerAddr);

	if ( (tx->sock = socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, IPPROTO_UDP)) == -1)
	{
		printf("failed to create transmit udp socket\n");
		fflush(stdout);
		return 1;	
	}
	memset((char *) &tx->peerAddr, 0, tx->peerAddrLen);
	tx->peerAddr.sin_family = AF_INET;
	tx->peerAddr.sin_port = htons(RXPORT);
	if (inet_aton(host, &tx->peerAddr.sin_addr) == 0) 
	{
		printf("failed to resolve server\n");
		fflush(stdout);
		return 2;
	}

	return 0;
}

int txBytes(WootTx* tx) {
	int result = sendto(tx->sock, (char*)tx->buf, NETBUFF_BYTES , 0 , (struct sockaddr *) &tx->peerAddr, tx->peerAddrLen);
	if (result == -1) {
		printf("failed to send over udp\n");
		fflush(stdout);
	}
	return result;
}

void txUdp(void*) {
	outIdxW = 0;
	outIdxR = 0;
	memset((char*)outBuff, 0, RINGBUFF_BYTES);

	printf("Starting txUdp...\n");
	fflush(stdout);

	WootTx tx;
	if (initWootTx(&tx, DEST_HOST, RXPORT) != 0) {
		return;
	}
	
	while(!Bela_stopRequested())
	{
		int availableSamples = outIdxW - outIdxR;
		if (availableSamples < 0) {
			availableSamples += RINGBUFF_SAMPLES;
		}
		if (availableSamples < NETBUFF_SAMPLES) {
			usleep(125);
			continue;
		}
		memcpy((char*)tx.buf, (char*)(&outBuff[outIdxR]), NETBUFF_BYTES);
		outIdxR += NETBUFF_SAMPLES;
		outIdxR %= RINGBUFF_SAMPLES;
		txBytes(&tx);
	}
	printf("Ending txUdp\n");
	fflush(stdout);
}


void writeTxUdpSample(float sample) {
	outBuff[outIdxW] = sample;
	outIdxW += 1;
	outIdxW %= RINGBUFF_SAMPLES;
}

int writeTxUdpSamples(WootTx* tx, BelaContext *context, int nChan) {
	for(unsigned int n = 0; n < context->audioFrames; n++) {
		for(unsigned int ch = 0; ch < nChan; ch++){
			tx->buf[(n * nChan) + ch] = audioRead(context, n, ch);
		}
	}
	// for (int i = 0; i < NETBUFF_SAMPLES; i++) {
	// 	tx->buf[i] = ((float)(i))/80.0f - 0.9f;
	// }
	return txBytes(tx);
}