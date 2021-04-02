#ifndef TH_NETWORK_POSTGRESQL_H
#define TH_NETWORK_POSTGRESQL_H

#include "../core/network.h"

struct pg_conn;

namespace Tomahawk
{
	namespace Network
	{
		namespace PDB
		{
			typedef pg_conn TConnection;

			class Queue;

			enum AddressOp
			{
				AddressOp_Host,
				AddressOp_Ip,
				AddressOp_Port,
				AddressOp_Database,
				AddressOp_User,
				AddressOp_Password,
				AddressOp_Timeout,
				AddressOp_Encoding,
				AddressOp_Options,
				AddressOp_Profile,
				AddressOp_Fallback_Profile,
				AddressOp_KeepAlive,
				AddressOp_KeepAlive_Idle,
				AddressOp_KeepAlive_Interval,
				AddressOp_KeepAlive_Count,
				AddressOp_TTY,
				AddressOp_SSL,
				AddressOp_SSL_Compression,
				AddressOp_SSL_Cert,
				AddressOp_SSL_Root_Cert,
				AddressOp_SSL_Key,
				AddressOp_SSL_CRL,
				AddressOp_Require_Peer,
				AddressOp_Require_SSL,
				AddressOp_KRB_Server_Name,
				AddressOp_Service
			};

			class TH_OUT Address
			{
			private:
				std::unordered_map<std::string, std::string> Params;

			public:
				Address();
				Address(const std::string& URI);
				void Override(const std::string& Key, const std::string& Value);
				bool Set(AddressOp Key, const std::string& Value);
				std::string Get(AddressOp Key) const;
				const std::unordered_map<std::string, std::string>& Get() const;
				const char** CreateKeys() const;
				const char** CreateValues() const;

			private:
				static std::string GetKeyName(AddressOp Key);
			};

			class TH_OUT Connection : public Core::Object
			{
				friend Queue;

			private:
				TConnection* Base;
				Queue* Master;
				bool Connected;

			public:
				Connection();
				virtual ~Connection() override;
				Core::Async<bool> Connect(const Address& URI);
				Core::Async<bool> Disconnect();
				TConnection* Get() const;
				bool IsConnected() const;
			};

			class TH_OUT Queue : public Core::Object
			{
				friend Connection;

			private:
				std::unordered_set<Connection*> Active;
				std::unordered_set<Connection*> Inactive;
				std::mutex Safe;
				Address Source;
				bool Connected;

			public:
				Queue();
				virtual ~Queue() override;
				bool Connect(const Address& URI);
				bool Disconnect();
				bool Push(Connection** Client);
				Core::Async<Connection*> Pop();

			private:
				void Clear(Connection* Client);
			};

			class TH_OUT Driver
			{
			private:
				struct Pose
				{
					std::string Key;
					size_t Offset;
					bool Escape;
				};

				struct Sequence
				{
					std::vector<Pose> Positions;
					std::string Request;
					std::string Cache;
				};

			private:
				static std::unordered_map<std::string, Sequence>* Queries;
				static std::mutex* Safe;
				static int State;

			public:
				static void Create();
				static void Release();
				static bool AddQuery(const std::string& Name, const char* Buffer, size_t Size);
				static bool AddDirectory(const std::string& Directory, const std::string& Origin = "");
				static bool RemoveQuery(const std::string& Name);
				static std::string GetQuery(Connection* Base, const std::string& Name, Core::DocumentArgs* Map, bool Once = true);
				static std::vector<std::string> GetQueries();

			private:
				static std::string GetCharArray(Connection* Base, const std::string& Src);
				static std::string GetByteArray(Connection* Base, const char* Src, size_t Size);
				static std::string GetSQL(Connection* Base, Core::Document* Source, bool Escape);
			};
		}
	}
}
#endif