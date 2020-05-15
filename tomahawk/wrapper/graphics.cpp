#include "graphics.h"
#include "../graphics.h"

namespace Tomahawk
{
    namespace Wrapper
    {
		namespace Graphics
		{
			void Enable(Script::VMManager* Manager)
			{
				Manager->Namespace("Graphics", [](Script::VMGlobal* Global)
				{
					/*
						TODO: Register types for VM from Tomahawk::Graphics
							
					*/

					return 0;
				});
			}
		}
    }
}