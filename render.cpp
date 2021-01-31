#include <Bela.h>
#include <algorithm>

#include "config.h"
#include "rxudp.h"
#include "txudp.h"

int gAudioChannelNum; // number of audio channels to iterate over

float buf[RINGBUFF_SAMPLES];

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

	Bela_runAuxiliaryTask(rxUdp);
	Bela_runAuxiliaryTask(txUdp);

	return true;
}

void render(BelaContext *context, void *userData)
{
	// Write incoming samples to output buffer
	for(unsigned int n = 0; n < context->audioFrames; n++) {
		for(unsigned int ch = 0; ch < gAudioChannelNum; ch++){
			writeTxUdpSample(audioRead(context, n, ch));
		}
	}

	// Write available samples from input buffer
	if (readRxUdpSamples(buf, context->audioFrames * gAudioChannelNum) > 0) {
		for(unsigned int n = 0; n < context->audioFrames; n++) {
			for(unsigned int ch = 0; ch < gAudioChannelNum; ch++){
				audioWrite(context, n, ch, buf[(n * 2) + ch]);
			}
		}
	}
}

void cleanup(BelaContext *context, void *userData)
{
}

