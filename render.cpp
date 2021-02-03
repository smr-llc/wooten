#include <Bela.h>
#include <algorithm>

#include "config.h"
#include "wootrx.h"
#include "woottx.h"

#define DEST_HOST "192.168.1.202"

int gAudioChannelNum; // number of audio channels to iterate over

WootTx tx;
WootRx rx;

bool setup(BelaContext *context, void *userData)
{
	// Check that we have the same number of inputs and outputs.
	if(context->audioInChannels != context->audioOutChannels ||
			context->analogInChannels != context-> analogOutChannels){
		printf("Different number of outputs and inputs available. Working with what we have.\n");
	}

	// If the amout of audio and analog input and output channels is not the same
	// we will use the minimum between input and output
	gAudioChannelNum = std::min(context->audioInChannels, context->audioOutChannels);

	printf("Initializing Rx...\n");
	if (rx.init(RXPORT) != 0) {
		printf("Failed to initialize Rx!\n");
		fflush(stdout);
		return false;
	}

	printf("Initializing Tx...\n");
	fflush(stdout);
	if (tx.connect(DEST_HOST, RXPORT) != 0) {
		printf("Failed to initialize Tx!\n");
		fflush(stdout);
		return false;
	}

	Bela_runAuxiliaryTask(WootRx::rxUdp, 50, &rx);
	Bela_runAuxiliaryTask(WootTx::txUdp, 50, &tx);

	return true;
}

void render(BelaContext *context, void *userData)
{
	tx.sendFrame(context, gAudioChannelNum);

	rx.writeReceivedFrame(context, gAudioChannelNum);
}

void cleanup(BelaContext *context, void *userData)
{
}

