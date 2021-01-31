#pragma once

#include <sys/socket.h>
#include <arpa/inet.h>
#include "config.h"

typedef struct WootRx WootRx;
struct WootRx {
    int sock;
    struct sockaddr_in peerAddr;
	socklen_t peerSockLen;
    int rxLen[2];
    char buf[2][NETBUFF_BYTES];
};

int initWootRx(WootRx* rx, int port);
// int rxBytes(WootRx* rx);

// void rxUdp(void*);
// int readRxUdpSamples(float* buf, int num);

void writeRxUdpSamples(WootRx* rx, BelaContext *context, int nChan);