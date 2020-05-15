#ifndef THAWK_WRAPPER_NETWORK_H
#define THAWK_WRAPPER_NETWORK_H

#include "../script.h"

namespace Tomahawk
{
    namespace Wrapper
    {
		namespace Network
		{
			namespace HTTP
			{
				THAWK_OUT void Enable(Script::VMManager* Manager);
			}

			namespace SMTP
			{
				THAWK_OUT void Enable(Script::VMManager* Manager);
			}

			namespace BSON
			{
				THAWK_OUT void Enable(Script::VMManager* Manager);
			}

			namespace MongoDB
			{
				THAWK_OUT void Enable(Script::VMManager* Manager);
			}

			THAWK_OUT void Enable(Script::VMManager* Manager);
		}
    }
}
#endif