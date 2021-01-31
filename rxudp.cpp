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

void rxUdp(void*) {
	memset((char*)inBuff, 0, RINGBUFF_BYTES);
	inIdxW = 0;
	inIdxR = 0;

	printf("Starting rxUdp...\n");
	fflush(stdout);

	struct sockaddr_in si_me, si_other;
	socklen_t slen = sizeof(si_other);
	int s, recv_len;
	if ((s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
	{
		printf("failed to create udp receive socket, got errno %d\n", errno);
		fflush(stdout);
		sleep(5);
		return;
	}
	memset((char *) &si_me, 0, sizeof(si_me));
	si_me.sin_family = AF_INET;
	si_me.sin_port = htons(RXPORT);
	si_me.sin_addr.s_addr = htonl(INADDR_ANY);

	if( bind(s , (struct sockaddr*)&si_me, sizeof(si_me) ) == -1)
	{
		printf("failed to bind udp receive socket, got errno %d\n", errno);
		fflush(stdout);
		sleep(5);
		return;
	}

	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 500000;
	if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
		    printf("failed to set timeout on receive socket, got errno %d\n", errno);
  		    fflush(stdout);
			sleep(5);
		    return;
	}


	char netInBuff[NETBUFF_BYTES];
	while (!Bela_stopRequested()) {
		if ((recv_len = recvfrom(s, netInBuff, NETBUFF_BYTES, 0, (struct sockaddr *) &si_other, &slen)) == -1)
		{
			if (errno == ETIMEDOUT) {
				printf("rxUdp timed out while receiving...\n");
				fflush(stdout);
				continue;
			}
			else if (errno == EAGAIN) {
				printf("rxUdp got 'try again' error while receiving...\n");
				fflush(stdout);
				sleep(1);
				continue;
			}
			printf("failed to receive on udp socket, got errno %d\n", errno);
			fflush(stdout);
			sleep(5);
			return;
		}
		if (recv_len == NETBUFF_BYTES) {
			memcpy((char*)(&inBuff[inIdxW]), netInBuff, NETBUFF_BYTES);
			inIdxW += NETBUFF_SAMPLES;
			inIdxW %= RINGBUFF_SAMPLES;
		}	
	}
	printf("Ending rxUdp\n");
	fflush(stdout);
}

int readRxUdpSamples(float* buf, int num) {
	int availableSamples = inIdxW - inIdxR;
	if (availableSamples < 0) {
        	availableSamples += RINGBUFF_SAMPLES;
	}
	if (availableSamples < num) {
		return 0;
	}
	memcpy(buf, &inBuff[inIdxR], num * 4);
	inIdxR += num;
	inIdxR %= RINGBUFF_SAMPLES;
	return num;
}