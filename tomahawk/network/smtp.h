#ifndef THAWK_NETWORK_SMTP_H
#define THAWK_NETWORK_SMTP_H

#include "../network.h"

namespace Tomahawk
{
	namespace Network
	{
		namespace SMTP
		{
			typedef std::function<void(class Client*, struct RequestFrame*, int)> ResponseCallback;
			typedef std::function<void()> ReplyCallback;

			enum Priority
			{
				Priority_High = 2,
				Priority_Normal = 3,
				Priority_Low = 4
			};

			struct THAWK_OUT Recipient
			{
				std::string Name;
				std::string Address;
			};

			struct THAWK_OUT Attachment
			{
				std::string Path;
				UInt64 Length = 0;
			};

			struct THAWK_OUT RequestFrame
			{
				std::vector<Recipient> Recipients;
				std::vector<Recipient> CCRecipients;
				std::vector<Recipient> BCCRecipients;
				std::vector<Attachment> Attachments;
				std::vector<std::string> Messages;
				std::string SenderName;
				std::string SenderAddress;
				std::string Subject;
				std::string Charset = "utf-8";
				std::string Mailer;
				std::string Receiver;
				std::string Login;
				std::string Password;
				std::string Hostname;
				Priority Prior = Priority_Normal;
				bool Authenticate = true;
				bool NoNotification = false;
				bool AllowHTML = false;
				char RemoteAddress[48] = { 0 };
			};

			class THAWK_OUT Client : public SocketClient
			{
			private:
				FILE* AttachmentFile;
				std::string Buffer;
				std::string Command;
				std::string Boundary;
				RequestFrame Request;
				Int64 Pending;
				bool Staging;
				bool Authorized;

			public:
				Client(Int64 ReadTimeout);
				virtual ~Client() override;
				bool Send(RequestFrame* Root, const ResponseCallback& Callback);
				RequestFrame* GetRequest();

			private:
				bool OnResolveHost(Host* Address) override;
				bool OnConnect() override;
				bool OnClose() override;
				bool Authorize(const ReplyCallback& Callback);
				bool SendAttachment();
				bool ProcessAttachment();
				bool ReadResponses(int Code, const ReplyCallback& Callback);
				bool ReadResponse(int Code, const ReplyCallback& Callback);
				bool SendRequest(int Code, const std::string& Content, const ReplyCallback& Callback);
				bool CanRequest(const char* Keyword);
				unsigned char* Unicode(const char* String);
			};
		}
	}
}
#endif