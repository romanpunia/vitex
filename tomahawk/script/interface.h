#ifndef THAWK_SCRIPT_INTERFACE_H
#define THAWK_SCRIPT_INTERFACE_H

#include "../script.h"

namespace Tomahawk
{
	namespace Script
	{
		namespace Interface
		{
			THAWK_OUT void EnableRest(VMManager* Manager);
			THAWK_OUT void EnableCompute(VMManager* Manager);
			THAWK_OUT void EnableNetwork(VMManager* Manager);
			THAWK_OUT void EnableHTTP(VMManager* Manager);
			THAWK_OUT void EnableSMTP(VMManager* Manager);
			THAWK_OUT void EnableBSON(VMManager* Manager);
			THAWK_OUT void EnableMongoDB(VMManager* Manager);
			THAWK_OUT void EnableAudio(VMManager* Manager);
			THAWK_OUT void EnableEffects(VMManager* Manager);
			THAWK_OUT void EnableFilters(VMManager* Manager);
			THAWK_OUT void EnableGraphics(VMManager* Manager);
			THAWK_OUT void EnableEngine(VMManager* Manager);
		}
	}
}
#endif