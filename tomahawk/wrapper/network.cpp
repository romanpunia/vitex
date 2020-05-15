#include "network.h"
#include "../network.h"
#include "../network/http.h"
#include "../network/smtp.h"
#include "../network/bson.h"
#include "../network/mongodb.h"

namespace Tomahawk
{
    namespace Wrapper
    {
		namespace Network
		{
			namespace HTTP
			{
				void Enable(Script::VMManager* Manager)
				{
					Manager->Namespace("HTTP", [](Script::VMGlobal* Global)
					{
						/*
							TODO: Register types for VM from Tomahawk::Network::HTTP
								enum Auth;
								enum Content;
								enum QueryValue;
								enum WebSocketOp;
								enum WebSocketState;
								enum CompressionTune;
								ref GatewayFile;
								ref ErrorFile;
								ref MimeType;
								ref MimeStatic;
								ref Credentials;
								ref Header;
								ref Resource;
								ref Cookie;
								ref RequestFrame;
								ref ResponseFrame;
								ref WebSocketFrame;
								ref GatewayFrame;
								ref ParserFrame;
								ref QueryToken;
								ref RouteEntry;
								ref SiteEntry;
								ref MapRouter;
								ref Connection;
								ref QueryParameter;
								ref Query;
								ref Session;
								ref Parser;
								ref Util;
								ref Server;
								ref Client;
						*/

						return 0;
					});
				}
			}

			namespace SMTP
			{
				void Enable(Script::VMManager* Manager)
				{
					Manager->Namespace("SMTP", [](Script::VMGlobal* Global)
					{
						/*
							TODO: Register types for VM from Tomahawk::Network::SMTP
								enum Priority;
								ref Recipient;
								ref Attachment;
								ref RequestFrame;
								ref Client;
						*/

						return 0;
					});
				}
			}

			namespace BSON
			{
				void Enable(Script::VMManager* Manager)
				{
					Manager->Namespace("BSON", [](Script::VMGlobal* Global)
					{
						/*
							TODO: Register types for VM from Tomahawk::Network::BSON
								enum Type;
								ref KeyPair;
								ref Document;
						*/

						return 0;
					});
				}
			}

			namespace MongoDB
			{
				void Enable(Script::VMManager* Manager)
				{
					Manager->Namespace("MongoDB", [](Script::VMGlobal* Global)
					{
						/*
							TODO: Register types for VM from Tomahawk::Network::MongoDB
								enum ReadMode;
								enum QueryMode;
								enum InsertMode;
								enum UpdateMode;
								enum RemoveMode;
								enum FindAndModifyMode;
								ref HostList;
								ref SSLOptions;
								ref APMCommandStarted;
								ref APMCommandSucceeded;
								ref APMCommandFailed;
								ref APMServerChanged;
								ref APMServerOpened;
								ref APMServerClosed;
								ref APMTopologyChanged;
								ref APMTopologyOpened;
								ref APMTopologyClosed;
								ref APMServerHeartbeatStarted;
								ref APMServerHeartbeatSucceeded;
								ref APMServerHeartbeatFailed;
								ref APMCallbacks;
								namespace Connector;
								ref URI;
								ref FindAndModifyOptions;
								ref ReadConcern;
								ref WriteConcern;
								ref ReadPreferences;
								ref BulkOperation;
								ref ChangeStream;
								ref Cursor;
								ref Collection;
								ref Database;
								ref ServerDescription;
								ref TopologyDescription;
								ref Client;
								ref ClientPool;
						*/

						return 0;
					});
				}
			}

			void Enable(Script::VMManager* Manager)
			{
				Manager->Namespace("Network", [](Script::VMGlobal* Global)
				{
					/*
						TODO: Register types for VM from Tomahawk::Network
							enum Secure;
							enum ServerState;
							enum SocketEvent;
							enum SocketType;
							ref Address;
							ref WriteEvent;
							ref ReadEvent;
							ref Socket;
							ref Host;
							ref Listener;
							ref Certificate;
							ref DataFrame;
							ref SocketCertificate;
							ref SocketConnection;
							ref SocketRouter;
							ref Multiplexer;
							ref SocketServer;
							ref SocketClient;
					*/

					return 0;
				});
			}
		}
    }
}