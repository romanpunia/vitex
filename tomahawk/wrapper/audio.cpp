#include "audio.h"
#include "../audio.h"

namespace Tomahawk
{
    namespace Wrapper
    {
		namespace Audio
		{
			void Enable(Script::VMManager* Manager)
			{
				Manager->Namespace("Audio", [](Script::VMGlobal* Global)
				{
					/*
						TODO: Register types for VM from Tomahawk::Audio
							enum SoundDistanceModel;
							enum SoundEx;
							namespace AudioContext;
							ref AudioClip;
							ref AudioSource;
							ref AudioDevice;
					*/

					return 0;
				});
			}
		}
    }
}