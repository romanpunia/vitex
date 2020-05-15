#include "rest.h"

namespace Tomahawk
{
    namespace Wrapper
    {
		namespace Rest
		{
			void Enable(Script::VMManager* Manager)
			{
				Manager->Namespace("Rest", [](Script::VMGlobal* Global)
				{
					/*
						TODO: Register types for VM from Tomahawk::Rest
							enum EventState;
							enum EventWorkflow;
							enum EventType;
							enum NodeType;
							enum NodePretty;
							enum FileMode;
							enum FileSeek;
							value FileState;
							ref EventArgs;
							ref EventBase;
							ref EventTimer;
							ref EventListener;
							value Resource;
							value ResourceEntry;
							ref DirectoryEntry;
							ref ChildProcess;
							value DateTime;
							value TickTimer;
							ref Console;
							ref Timer;
							ref FileStream;
							ref FileLogger;
							ref FileTree;
							static OS;
							ref EventWorker;
							ref EventQueue;
							ref Document;
					*/

					return 0;
				});
			}
		}
    }
}