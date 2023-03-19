#ifndef ED_NETWORK_SMTP_H
#define ED_NETWORK_SMTP_H
#include "../core/network.h"

namespace Edge
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

			struct ED_OUT Recipient
			{
				std::string Name;
				std::string Address;
			};

			struct ED_OUT Attachment
			{
				std::string Path;
				size_t Length = 0;
			};

			struct ED_OUT RequestFrame
			{
				std::unordered_map<std::string, std::string> Headers;
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
				Priority Prior = Priority::Normal;
				bool Authenticate = true;
				bool NoNotification = false;
				bool AllowHTML = false;
				char RemoteAddress[48] = { };
			};

			class ED_OUT Client final : public SocketClient
			{
			private:
				FILE * AttachmentFile;
				std::string Buffer;
				std::string Command;
				std::string Boundary;
				std::string Hoster;
				RequestFrame Request;
				int32_t Pending;
				bool Authorized;

			public:
				Client(const std::string& Domain, int64_t ReadTimeout);
				~Client() override;
				Core::Promise<int> Send(RequestFrame&& Root);
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
				bool SendRequest(int Code, const std::string& Content, const ReplyCallback& Callback);
				bool CanRequest(const char* Keyword);
				unsigned char* Unicode(const char* String);
			};
		}
	}
}
#endif