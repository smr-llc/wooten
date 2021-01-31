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

void txUdp(void*) {
	outIdxW = 0;
	outIdxR = 0;
	memset((char*)outBuff, 0, RINGBUFF_BYTES);

	printf("Starting txUdp...\n");
	fflush(stdout);

	struct sockaddr_in si_other;
	int s;
	socklen_t slen = sizeof(si_other);
	if ( (s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
	{
	        printf("failed to create transmit udp socket\n");
	        fflush(stdout);
		return;	
	}
	memset((char *) &si_other, 0, sizeof(si_other));
	si_other.sin_family = AF_INET;
	si_other.sin_port = htons(RXPORT);
	if (inet_aton("127.0.0.1" , &si_other.sin_addr) == 0) 
	{
		printf("failed to resolve server\n");
		fflush(stdout);
		return;
	}

	char netOutBuff[NETBUFF_BYTES];
	while(!Bela_stopRequested())
	{
		int availableSamples = outIdxW - outIdxR;
		if (availableSamples < 0) {
			availableSamples += RINGBUFF_SAMPLES;
		}
		if (availableSamples < NETBUFF_SAMPLES) {
			usleep(500);
			continue;
		}
		memcpy(netOutBuff, (char*)(&outBuff[outIdxR]), NETBUFF_BYTES);
		outIdxR += NETBUFF_SAMPLES;
		outIdxR %= RINGBUFF_SAMPLES;
		if (sendto(s, netOutBuff, NETBUFF_BYTES , 0 , (struct sockaddr *) &si_other, slen) == -1)
		{
			printf("failed to send over udp");
			fflush(stdout);
			return;
		}
	}
	printf("Ending txUdp\n");
	fflush(stdout);
}


void writeTxUdpSample(float sample) {
	outBuff[outIdxW] = sample;
	outIdxW += 1;
	outIdxW %= RINGBUFF_SAMPLES;
}