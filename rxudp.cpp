#include <Bela.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <string.h>
#include <errno.h>

#include "config.h"
#include "rxudp.h"

float inBuff[RINGBUFF_SAMPLES];
int inIdxW;
int inIdxR;

int initWootRx(WootRx* rx, int port) {
	memset((char *)rx, 0, sizeof(WootRx));

	rx->peerSockLen = sizeof(rx->peerAddr);

	if ((rx->sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
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

	if( bind(rx->sock, (struct sockaddr*)&addr, sizeof(addr) ) == -1)
	{
		printf("failed to bind udp receive socket, got errno %d\n", errno);
		fflush(stdout);
		return 2;
	}

	// struct timeval tv;
	// tv.tv_sec = 1;
	// tv.tv_usec = 500000;
	// if (setsockopt(rx->sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
	// 	    printf("failed to set timeout on receive socket, got errno %d\n", errno);
  	// 	    fflush(stdout);
	// 	    return 3;
	// }

	return 0;
}

// int rxBytes(WootRx* rx) {
// 	rx->rxLen = recvfrom(rx->sock, rx->buf, NETBUFF_BYTES, 0, (struct sockaddr*)&rx->peerAddr, &rx->peerSockLen);
// 	return rx->rxLen;
// }

// void rxUdp(void*) {
// 	memset((char*)inBuff, 0, RINGBUFF_BYTES);
// 	inIdxW = 0;
// 	inIdxR = 0;

// 	printf("Starting rxUdp...\n");
// 	fflush(stdout);

// 	WootRx rx;
// 	if (initWootRx(&rx, RXPORT) != 0) {
// 		return;
// 	}

// 	while (!Bela_stopRequested()) {
// 		if (rxBytes(&rx) == -1)
// 		{
// 			if (errno == ETIMEDOUT || errno == EAGAIN) {
// 				printf("rxUdp timed out while receiving...\n");
// 				fflush(stdout);
// 				//sleep(1);
// 				continue;
// 			}
// 			printf("failed to receive on udp socket, got errno %d\n", errno);
// 			fflush(stdout);
// 			sleep(5);
// 			return;
// 		}
// 		if (rx.rxLen == NETBUFF_BYTES) {
// 			memcpy((char*)(&inBuff[inIdxW]), rx.buf, NETBUFF_BYTES);
// 			inIdxW += NETBUFF_SAMPLES;
// 			inIdxW %= RINGBUFF_SAMPLES;
// 		}
// 		else {
// 			printf("wrong buffer size %d\n", rx.rxLen);
// 			fflush(stdout);
// 			sleep(5);
// 		}
// 	}
// 	printf("Ending rxUdp\n");
// 	fflush(stdout);
// }

// int readRxUdpSamples(float* buf, int num) {
// 	int availableSamples = inIdxW - inIdxR;
// 	if (availableSamples < 0) {
//         	availableSamples += RINGBUFF_SAMPLES;
// 	}
// 	if (availableSamples < num) {
// 		return 0;
// 	}
// 	memcpy(buf, &inBuff[inIdxR], num * 4);
// 	inIdxR += num;
// 	inIdxR %= RINGBUFF_SAMPLES;
// 	return num;
// }

void writeRxUdpSamples(WootRx* rx, BelaContext *context, int nChan) {
	rx->rxLen[0] = 0;
	rx->rxLen[1] = 0;
	int bufNum = 0;
	while (true) {
		rx->rxLen[bufNum] = recv(rx->sock, rx->buf[bufNum], NETBUFF_BYTES, MSG_DONTWAIT);
		if (rx->rxLen[bufNum] <= 0) {
			bufNum = (bufNum + 1) % 2;
			break;
		}
		bufNum = (bufNum + 1) % 2;
	}
	if (rx->rxLen[bufNum] != NETBUFF_BYTES) {
		for(unsigned int n = 0; n < context->audioFrames; n++) {
			for(unsigned int ch = 0; ch < nChan; ch++){
				audioWrite(context, n, ch, 0);
			}
		}
	}
	else {
		for(unsigned int n = 0; n < context->audioFrames; n++) {
			for(unsigned int ch = 0; ch < nChan; ch++){
				audioWrite(context, n, ch, ((float*)rx->buf[bufNum])[(n * 2) + ch]);
			}
		}
	}
}