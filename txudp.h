#pragma once

#include <sys/socket.h>
#include <arpa/inet.h>
#include "config.h"

typedef struct WootTx WootTx;
struct WootTx {
    int sock;
    struct sockaddr_in peerAddr;
	socklen_t peerAddrLen;
    float buf[NETBUFF_BYTES];
};

int initWootTx(WootTx* rx, const char* host, int port);
int txBytes(WootTx* rx);

void txUdp(void*);
void writeTxUdpSample(float sample);

int writeTxUdpSamples(WootTx* tx, BelaContext *context, int nChan);