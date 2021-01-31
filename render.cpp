#include <Bela.h>
#include <algorithm>

#include "config.h"
#include "rxudp.h"
#include "txudp.h"

int gAudioChannelNum; // number of audio channels to iterate over

float buf[RINGBUFF_SAMPLES];
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

	//Bela_runAuxiliaryTask(rxUdp, 50);
	//Bela_runAuxiliaryTask(txUdp, 50);

	printf("Initializing Tx\n");
	fflush(stdout);
	if (initWootTx(&tx, DEST_HOST, RXPORT) != 0) {
		printf("Failed to initialize Tx\n");
		fflush(stdout);
		return false;
	}

	printf("Initializing Rx\n");
	fflush(stdout);
	if (initWootRx(&rx, RXPORT) != 0) {
		printf("Failed to initialize Rx\n");
		fflush(stdout);
		return false;
	}

	return true;
}

void render(BelaContext *context, void *userData)
{
	// Write incoming samples to output buffer
	// for(unsigned int n = 0; n < context->audioFrames; n++) {
	// 	for(unsigned int ch = 0; ch < gAudioChannelNum; ch++){
	// 		writeTxUdpSample(audioRead(context, n, ch));
	// 	}
	// }

	// Write incoming audio samples to UDP
	writeTxUdpSamples(&tx, context, gAudioChannelNum);

	// Write incoming UDP samples to audio

	// Write available samples from input buffer
	// if (readRxUdpSamples(buf, context->audioFrames * gAudioChannelNum) > 0) {
	// 	for(unsigned int n = 0; n < context->audioFrames; n++) {
	// 		for(unsigned int ch = 0; ch < gAudioChannelNum; ch++){
	// 			audioWrite(context, n, ch, buf[(n * 2) + ch]);
	// 		}
	// 	}
	// }
	writeRxUdpSamples(&rx, context, gAudioChannelNum);

}

void cleanup(BelaContext *context, void *userData)
{
}

