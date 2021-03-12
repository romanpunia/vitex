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
				TConnection* Get() const;
				bool IsConnected() const;
			};

			class TH_OUT Queue : public Core::Object
			{
			private:
				bool Connected;

			public:
				Queue();
				virtual ~Queue() override;
			};

			class TH_OUT Driver
			{
			private:
				struct Sequence
				{
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
				static std::string GetSubquery(Connection* Base, const char* Buffer, Core::DocumentArgs* Map, bool Once = true);
				static std::vector<std::string> GetQueries();

			private:
				static std::string GetCharArray(Connection* Base, const std::string& Src);
				static std::string GetByteArray(Connection* Base, const char* Src, size_t Size);
				static std::string GetSQL(Connection* Base, Core::Document* Source);
			};
		}
	}
}
#endif