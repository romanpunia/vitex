#include "engine.h"
#include "../engine.h"

namespace Tomahawk
{
    namespace Wrapper
    {
		namespace Engine
		{
			void Enable(Script::VMManager* Manager)
			{
				Manager->Namespace("Engine", [](Script::VMGlobal* Global)
				{
					/*
						TODO: Register types for VM from Tomahawk::Engine

					*/

					return 0;
				});
			}
		}
    }
}