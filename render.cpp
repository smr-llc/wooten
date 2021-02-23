#include <Bela.h>
#include <libraries/Gui/Gui.h>
#include <algorithm>

#include "portbroker.h"
#include "wootbase.h"

WootBase base;

bool setup(BelaContext *context, void *userData)
{
	if(context->audioInChannels < 2
		|| context->audioOutChannels < 2 ) {
		printf("FATAL: Cannot run without 2 input and output audio channels!\n");
		fflush(stdout);
		return false;
	}


	if (base.setup(context) != 0) {
		printf("FATAL: Failed WootBase setup!\n");
		fflush(stdout);
		return false;
	}
	
	Bela_runAuxiliaryTask(PortBroker::assignPorts, 10);
	Bela_runAuxiliaryTask(WootBase::auxProcess, 20, &base);

	return true;
}

void render(BelaContext *context, void *userData)
{
	base.processFrame(context);
}

void cleanup(BelaContext *context, void *userData)
{
}

