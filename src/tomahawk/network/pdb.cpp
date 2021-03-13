#include "pdb.h"
#ifdef TH_HAS_POSTGRESQL
#include <libpq-fe.h>
#endif

namespace Tomahawk
{
	namespace Network
	{
		namespace PDB
		{
			Connection::Connection() : Base(nullptr), Master(nullptr), Connected(false)
			{
			}
			Connection::~Connection()
			{
			}
			TConnection* Connection::Get() const
			{
				return Base;
			}
			bool Connection::IsConnected() const
			{
				return Connected;
			}

			Queue::Queue() : Connected(false)
			{
			}
			Queue::~Queue()
			{
			}

			void Driver::Create()
			{
#ifdef TH_HAS_POSTGRESQL
				if (State <= 0)
				{
					Queries = new std::unordered_map<std::string, Sequence>();
					Safe = new std::mutex();
					State = 1;
				}
				else
					State++;
#endif
			}
			void Driver::Release()
			{
#ifdef TH_HAS_POSTGRESQL
				if (State == 1)
				{
					if (Safe != nullptr)
						Safe->lock();

					State = 0;
					if (Queries != nullptr)
					{
						delete Queries;
						Queries = nullptr;
					}

					if (Safe != nullptr)
					{
						Safe->unlock();
						delete Safe;
						Safe = nullptr;
					}
				}
				else if (State > 0)
					State--;
#endif
			}
			bool Driver::AddQuery(const std::string& Name, const char* Buffer, size_t Size)
			{
				if (!Queries || !Safe || Name.empty() || !Buffer || !Size)
					return false;

				Sequence Result;
				Result.Request.assign(Buffer, Size);

				Core::Parser Base(&Result.Request);
				Base.Trim();

				uint64_t Args = 0;
				uint64_t Index = 0;
				int64_t Arg = -1;
				bool Spec = false;
				bool Lock = false;

				while (Index < Base.Size())
				{
					char V = Base.R()[Index];
					if (V == '"')
					{
						if (Lock)
						{
							if (Spec)
								Spec = false;
							else
								Lock = false;
						}
						else if (Spec)
							Spec = false;
						else
							Lock = true;
						Index++;
					}
					else if (V == '>')
					{
						if (!Spec && Arg >= 0)
						{
							if (Arg < Base.Size())
							{
								Base.Insert(TH_PREFIX_CHAR, Arg);
								Args++; Index++;
							}

							Spec = false;
							Index++;
							Arg = -1;
						}
						else if (Spec)
						{
							Spec = false;
							Base.Erase(Index - 1, 1);
						}
						else
							Index++;
					}
					else if (V == '<')
					{
						if (!Spec && Arg < 0)
						{
							Arg = Index;
							Index++;
						}
						else if (Spec)
						{
							Spec = false;
							Base.Erase(Index - 1, 1);
						}
						else
							Index++;
					}
					else if (Lock && V == '\\')
					{
						Spec = true;
						Index++;
					}
					else if (!Lock && (V == '\n' || V == '\r' || V == '\t' || V == ' '))
					{
						Base.Erase(Index, 1);
					}
					else
					{
						Spec = false;
						Index++;
					}
				}

				if (Args < 1)
					Result.Cache = Result.Request;

				Safe->lock();
				(*Queries)[Name] = Result;
				Safe->unlock();

				return true;
			}
			bool Driver::AddDirectory(const std::string& Directory, const std::string& Origin)
			{
				std::vector<Core::ResourceEntry> Entries;
				if (!Core::OS::Directory::Scan(Directory, &Entries))
					return false;

				std::string Path = Directory;
				if (Path.back() != '/' && Path.back() != '\\')
					Path.append(1, '/');

				uint64_t Size = 0;
				for (auto& File : Entries)
				{
					Core::Parser Base(Path + File.Path);
					if (File.Source.IsDirectory)
					{
						AddDirectory(Base.R(), Origin.empty() ? Directory : Origin);
						continue;
					}

					if (!Base.EndsWith(".sql"))
						continue;

					char* Buffer = (char*)Core::OS::File::ReadAll(Base.Get(), &Size);
					if (!Buffer)
						continue;

					Base.Replace(Origin.empty() ? Directory : Origin, "").Replace("\\", "/").Replace(".sql", "");
					AddQuery(Base.Substring(1).R(), Buffer, Size);
					TH_FREE(Buffer);
				}

				return true;
			}
			bool Driver::RemoveQuery(const std::string& Name)
			{
				if (!Queries || !Safe)
					return false;

				Safe->lock();
				auto It = Queries->find(Name);
				if (It == Queries->end())
				{
					Safe->unlock();
					return false;
				}

				Queries->erase(It);
				Safe->unlock();
				return true;
			}
			std::string Driver::GetQuery(Connection* Base, const std::string& Name, Core::DocumentArgs* Map, bool Once)
			{
				if (!Queries || !Safe)
					return nullptr;

				Safe->lock();
				auto It = Queries->find(Name);
				if (It == Queries->end())
				{
					Safe->unlock();
					if (Once && Map != nullptr)
					{
						for (auto& Item : *Map)
							TH_RELEASE(Item.second);
						Map->clear();
					}

					return nullptr;
				}

				if (!It->second.Cache.empty())
				{
					std::string Result = It->second.Cache;
					Safe->unlock();

					if (Once && Map != nullptr)
					{
						for (auto& Item : *Map)
							TH_RELEASE(Item.second);
						Map->clear();
					}

					return Result;
				}

				if (!Map || Map->empty())
				{
					std::string Result = It->second.Request;
					Safe->unlock();

					if (Once && Map != nullptr)
					{
						for (auto& Item : *Map)
							TH_RELEASE(Item.second);
						Map->clear();
					}

					return Result;
				}

				Sequence Origin = It->second;
				Safe->unlock();

				Core::Parser Result(&Origin.Request);
				for (auto& Context : *Map)
				{
					std::string Value = GetSQL(Base, Context.second);
					if (!Value.empty())
						Result.Replace(TH_PREFIX_STR "<" + Context.first + '>', Value);

					if (Once)
						TH_RELEASE(Context.second);
				}

				if (Once)
					Map->clear();

				std::string Data = Origin.Request;
				if (Data.empty())
					TH_ERROR("could not construct query: \"%s\"", Name.c_str());

				return Data;
			}
			std::string Driver::GetSubquery(Connection* Base, const char* Buffer, Core::DocumentArgs* Map, bool Once)
			{
				if (!Buffer || Buffer[0] == '\0')
				{
					if (Once && Map != nullptr)
					{
						for (auto& Item : *Map)
							TH_RELEASE(Item.second);
						Map->clear();
					}

					return "";
				}

				if (!Map || Map->empty())
				{
					if (Once && Map != nullptr)
					{
						for (auto& Item : *Map)
							TH_RELEASE(Item.second);
						Map->clear();
					}

					return Buffer;
				}

				Core::Parser Result(Buffer, strlen(Buffer));
				for (auto& Context : *Map)
				{
					std::string Value = GetSQL(Base, Context.second);
					if (!Value.empty())
						Result.Replace(TH_PREFIX_STR "<" + Context.first + '>', Value);

					if (Once)
						TH_RELEASE(Context.second);
				}

				if (Once)
					Map->clear();

				std::string Data = Result.R();
				if (Data.empty())
					TH_ERROR("could not construct subquery:\n%s", Result.Get());

				return Data;
			}
			std::vector<std::string> Driver::GetQueries()
			{
				std::vector<std::string> Result;
				if (!Queries || !Safe)
					return Result;

				Safe->lock();
				Result.reserve(Queries->size());
				for (auto& Item : *Queries)
					Result.push_back(Item.first);
				Safe->unlock();

				return Result;
			}
			std::string Driver::GetCharArray(Connection* Base, const std::string& Src)
			{
#ifdef TH_HAS_POSTGRESQL
				if (Src.empty())
					return "''";

				if (!Base || !Base->Get())
					return "'" + Src + "'";

				char* Subresult = PQescapeLiteral(Base->Get(), Src.c_str(), Src.size());
				std::string Result(Subresult);
				PQfreemem(Subresult);

				return Result;
#else
				return "'" + Src + "'";
#endif
			}
			std::string Driver::GetByteArray(Connection* Base, const char* Src, size_t Size)
			{
#ifdef TH_HAS_POSTGRESQL
				if (!Src || !Size)
					return "''";

				if (!Base || !Base->Get())
					return "'\\x" + Compute::Common::BinToHex(Src, Size) + "'::bytea";

				size_t Length = 0;
				char* Subresult = (char*)PQescapeByteaConn(Base->Get(), (unsigned char*)Src, Size, &Length);
				std::string Result(Subresult, Length);
				PQfreemem((unsigned char*)Subresult);

				return Result;
#else
				return "'\\x" + Compute::Common::BinToHex(Src, Size) + "'::bytea";
#endif
			}
			std::string Driver::GetSQL(Connection* Base, Core::Document* Source)
			{
				if (!Source)
					return "NULL";

				Core::Document* Parent = Source->GetParent();
				bool Array = (Parent->Value.GetType() == Core::VarType_Array);

				switch (Source->Value.GetType())
				{
					case Core::VarType_Object:
					{
						if (Array)
							return "";

						std::string Result;
						Core::Document::WriteJSON(Source, [&Result](Core::VarForm, const char* Buffer, int64_t Length)
						{
							if (Buffer != nullptr && Length > 0)
								Result.append(Buffer, Length);
						});

						return GetCharArray(Base, Result);
					}
					case Core::VarType_Array:
					{
						std::string Result = (Array ? "[" : "ARRAY[");
						for (auto* Node : *Source->GetNodes())
							Result.append(GetSQL(Base, Node)).append(1, ',');

						if (!Source->GetNodes()->empty())
							Result = Result.substr(0, Result.size() - 1);

						return Result + "]";
					}
					case Core::VarType_String:
						return GetCharArray(Base, Source->Value.GetBlob());
					case Core::VarType_Integer:
						return std::to_string(Source->Value.GetInteger());
					case Core::VarType_Number:
						return std::to_string(Source->Value.GetNumber());
					case Core::VarType_Boolean:
						return Source->Value.GetBoolean() ? "TRUE" : "FALSE";
					case Core::VarType_Decimal:
					{
						std::string Result(std::move(GetCharArray(Base, Source->Value.GetDecimal())));
						return (Result.size() >= 2 ? Result.substr(1, Result.size() - 2) : Result);
					}
					case Core::VarType_Base64:
						return GetByteArray(Base, Source->Value.GetString(), Source->Value.GetSize());
					case Core::VarType_Null:
					case Core::VarType_Undefined:
						return "NULL";
					default:
						break;
				}

				return "NULL";
			}
			std::unordered_map<std::string, Driver::Sequence>* Driver::Queries = nullptr;
			std::mutex* Driver::Safe = nullptr;
			int Driver::State = 0;
		}
	}
}