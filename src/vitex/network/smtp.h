#ifndef VI_NETWORK_SMTP_H
#define VI_NETWORK_SMTP_H
#include "../network.h"

namespace vitex
{
	namespace network
	{
		namespace smtp
		{
			typedef std::function<void()> reply_callback;

			enum class priority
			{
				high = 2,
				normal = 3,
				low = 4
			};

			struct recipient
			{
				core::string name;
				core::string address;
			};

			struct attachment
			{
				core::string path;
				size_t length = 0;
			};

			struct request_frame
			{
				core::unordered_map<core::string, core::string> headers;
				core::vector<recipient> recipients;
				core::vector<recipient> cc_recipients;
				core::vector<recipient> bcc_recipients;
				core::vector<attachment> attachments;
				core::vector<core::string> messages;
				core::string sender_name;
				core::string sender_address;
				core::string subject;
				core::string charset = "utf-8";
				core::string mailer;
				core::string receiver;
				core::string login;
				core::string password;
				priority prior = priority::normal;
				bool authenticate = true;
				bool no_notification = false;
				bool allow_html = false;
			};

			class client final : public socket_client
			{
			private:
				FILE* attachment_file;
				core::string buffer;
				core::string command;
				core::string boundary;
				core::string hoster;
				request_frame request;
				int32_t pending;
				bool authorized;

			public:
				client(const std::string_view& domain, int64_t read_timeout);
				~client() override = default;
				core::expects_promise_system<void> send(request_frame&& root);
				request_frame* get_request();

			private:
				core::expects_system<void> on_connect() override;
				core::expects_system<void> on_disconnect() override;
				bool authorize(reply_callback&& callback);
				bool prepare_and_send();
				bool send_attachment();
				bool process_attachment();
				bool read_responses(int code, reply_callback&& callback);
				bool read_response(int code, reply_callback&& callback);
				bool send_request(int code, const std::string_view& content, reply_callback&& callback);
				bool can_request(const std::string_view& keyword);
				uint8_t* unicode(const std::string_view& value);
			};
		}
	}
}
#endif