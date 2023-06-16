#ifndef VI_NETWORK_SMTP_H
#define VI_NETWORK_SMTP_H
#include "../core/network.h"

namespace Mavi
{
	namespace Network
	{
		namespace SMTP
		{
			typedef std::function<void()> ReplyCallback;

			enum class Priority
			{
				High = 2,
				Normal = 3,
				Low = 4
			};

			struct VI_OUT Recipient
			{
				Core::String Name;
				Core::String Address;
			};

			struct VI_OUT Attachment
			{
				Core::String Path;
				size_t Length = 0;
			};

			struct VI_OUT RequestFrame
			{
				Core::UnorderedMap<Core::String, Core::String> Headers;
				Core::Vector<Recipient> Recipients;
				Core::Vector<Recipient> CCRecipients;
				Core::Vector<Recipient> BCCRecipients;
				Core::Vector<Attachment> Attachments;
				Core::Vector<Core::String> Messages;
				Core::String SenderName;
				Core::String SenderAddress;
				Core::String Subject;
				Core::String Charset = "utf-8";
				Core::String Mailer;
				Core::String Receiver;
				Core::String Login;
				Core::String Password;
				Priority Prior = Priority::Normal;
				bool Authenticate = true;
				bool NoNotification = false;
				bool AllowHTML = false;
				char RemoteAddress[48] = { };
			};

			class VI_OUT Client final : public SocketClient
			{
			private:
				FILE * AttachmentFile;
				Core::String Buffer;
				Core::String Command;
				Core::String Boundary;
				Core::String Hoster;
				RequestFrame Request;
				int32_t Pending;
				bool Authorized;

			public:
				Client(const Core::String& Domain, int64_t ReadTimeout);
				~Client() override = default;
				Core::ExpectedPromise<void> Send(RequestFrame&& Root);
				RequestFrame* GetRequest();

			private:
				bool OnResolveHost(RemoteHost* Address) override;
				bool OnConnect() override;
				bool OnClose() override;
				bool Authorize(const ReplyCallback& Callback);
				bool PrepareAndSend();
				bool SendAttachment();
				bool ProcessAttachment();
				bool ReadResponses(int Code, const ReplyCallback& Callback);
				bool ReadResponse(int Code, const ReplyCallback& Callback);
				bool SendRequest(int Code, const Core::String& Content, const ReplyCallback& Callback);
				bool CanRequest(const char* Keyword);
				unsigned char* Unicode(const char* String);
			};
		}
	}
}
#endif