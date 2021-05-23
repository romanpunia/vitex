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
			Address::Address()
			{
			}
			Address::Address(const std::string& URI)
			{
#ifdef TH_HAS_POSTGRESQL
				char* Message = nullptr;
				PQconninfoOption* Result = PQconninfoParse(URI.c_str(), &Message);
				if (Result != nullptr)
				{
					size_t Index = 0;
					while (Result[Index].keyword != nullptr)
					{
						auto& Info = Result[Index];
						Params[Info.keyword] = Info.val;
						Index++;
					}
					PQconninfoFree(Result);
				}
				else
				{
					if (Message != nullptr)
					{
						TH_ERROR("[pg] couldn't parse URI string\n\t%s", Message);
						PQfreemem(Message);
					}
				}
#endif
			}
			void Address::Override(const std::string& Key, const std::string& Value)
			{
				Params[Key] = Value;
			}
			bool Address::Set(AddressOp Key, const std::string& Value)
			{
				std::string Name = GetKeyName(Key);
				if (Name.empty())
					return false;

				Params[Name] = Value;
				return true;
			}
			std::string Address::Get(AddressOp Key) const
			{
				auto It = Params.find(GetKeyName(Key));
				if (It == Params.end())
					return "";

				return It->second;
			}
			const std::unordered_map<std::string, std::string>& Address::Get() const
			{
				return Params;
			}
			std::string Address::GetKeyName(AddressOp Key)
			{
				switch (Key)
				{
					case AddressOp_Host:
						return "host";
					case AddressOp_Ip:
						return "hostaddr";
					case AddressOp_Port:
						return "port";
					case AddressOp_Database:
						return "dbname";
					case AddressOp_User:
						return "user";
					case AddressOp_Password:
						return "password";
					case AddressOp_Timeout:
						return "connect_timeout";
					case AddressOp_Encoding:
						return "client_encoding";
					case AddressOp_Options:
						return "options";
					case AddressOp_Profile:
						return "application_name";
					case AddressOp_Fallback_Profile:
						return "fallback_application_name";
					case AddressOp_KeepAlive:
						return "keepalives";
					case AddressOp_KeepAlive_Idle:
						return "keepalives_idle";
					case AddressOp_KeepAlive_Interval:
						return "keepalives_interval";
					case AddressOp_KeepAlive_Count:
						return "keepalives_count";
					case AddressOp_TTY:
						return "tty";
					case AddressOp_SSL:
						return "sslmode";
					case AddressOp_SSL_Compression:
						return "sslcompression";
					case AddressOp_SSL_Cert:
						return "sslcert";
					case AddressOp_SSL_Root_Cert:
						return "sslrootcert";
					case AddressOp_SSL_Key:
						return "sslkey";
					case AddressOp_SSL_CRL:
						return "sslcrl";
					case AddressOp_Require_Peer:
						return "requirepeer";
					case AddressOp_Require_SSL:
						return "requiressl";
					case AddressOp_KRB_Server_Name:
						return "krbservname";
					case AddressOp_Service:
						return "service";
					default:
						return "";
				}
			}
			const char** Address::CreateKeys() const
			{
				const char** Result = (const char**)TH_MALLOC(sizeof(const char*) * (Params.size() + 1));
				size_t Index = 0;

				for (auto& Key : Params)
					Result[Index++] = Key.first.c_str();

				Result[Index] = nullptr;
				return Result;
			}
			const char** Address::CreateValues() const
			{
				const char** Result = (const char**)TH_MALLOC(sizeof(const char*) * (Params.size() + 1));
				size_t Index = 0;

				for (auto& Key : Params)
					Result[Index++] = Key.second.c_str();

				Result[Index] = nullptr;
				return Result;
			}

			Connection::Connection() : Base(nullptr), Master(nullptr), Connected(false)
			{
				Driver::Create();
			}
			Connection::~Connection()
			{
				Driver::Release();
			}
			Core::Async<bool> Connection::Connect(const Address& URI)
			{
#ifdef TH_HAS_POSTGRESQL
				if (Master != nullptr)
					return Core::Async<bool>::Store(false);

				if (Connected)
				{
					return Disconnect().Then<Core::Async<bool>>([this, URI](bool)
					{
						return this->Connect(URI);
					});
				}

				return Core::Async<bool>([this, URI](Core::Async<bool>& Future)
				{
					const char** Keys = URI.CreateKeys();
					const char** Values = URI.CreateValues();
					Base = PQconnectdbParams(Keys, Values, 0);
					TH_FREE(Keys);
					TH_FREE(Values);

					if (!Base)
					{
						TH_ERROR("couldn't connect to requested URI");
						return Future.Set(false);
					}

					Connected = true;
					Future.Set(true);
				});
#else
				return Core::Async<bool>::Store(false);
#endif
			}
			Core::Async<bool> Connection::Disconnect()
			{
#ifdef TH_HAS_POSTGRESQL
				if (!Connected || !Base)
					return Core::Async<bool>::Store(false);

				return Core::Async<bool>([this](Core::Async<bool>& Future)
				{
					Connected = false;
					if (!Base)
						return Future.Set(true);

					if (!Master)
					{
						PQfinish(Base);
						Base = nullptr;
					}
					else
						Master->Clear(this);

					Future.Set(true);
				});
#else
				return Core::Async<bool>::Store(false);
#endif
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
				Driver::Create();
			}
			Queue::~Queue()
			{
				Disconnect();
				Driver::Release();
			}
			Core::Async<bool> Queue::Connect(const Address& URI)
			{
#ifdef TH_HAS_POSTGRESQL
				if (Connected || URI.Get().empty())
					return Core::Async<bool>::Store(false);

				return Core::Async<bool>([this, URI](Core::Async<bool>& Future)
				{
					const char** Keys = URI.CreateKeys();
					const char** Values = URI.CreateValues();
					PGPing Result = PQpingParams(Keys, Values, 0);
					TH_FREE(Keys);
					TH_FREE(Values);

					switch (Result)
					{
						case PGPing::PQPING_OK:
							Source = URI;
							Connected = true;
							Future.Set(true);
							break;
						case PGPing::PQPING_REJECT:
							TH_ERROR("[pg] server connection rejected");
							Future.Set(false);
							break;
						case PGPing::PQPING_NO_ATTEMPT:
							TH_ERROR("[pg] invalid params");
							Future.Set(false);
							break;
						case PGPing::PQPING_NO_RESPONSE:
							TH_ERROR("[pg] couldn't connect to server");
							Future.Set(false);
							break;
						default:
							Future.Set(false);
							break;
					}
				});
#else
                return Core::Async<bool>::Store(false);
#endif
			}
			Core::Async<bool> Queue::Disconnect()
			{
				if (!Connected)
					return Core::Async<bool>::Store(false);

				return Core::Async<bool>([this](Core::Async<bool>& Future)
				{
					Safe.lock();
					for (auto& Base : Active)
						Clear(Base);

					for (auto& Base : Inactive)
					{
						Clear(Base);
						TH_RELEASE(Base);
					}

					Connected = false;
					Active.clear();
					Inactive.clear();
					Safe.unlock();
					Future.Set(true);
				});
			}
			void Queue::Clear(Connection* Client)
			{
				if (!Client)
					return;
#ifdef TH_HAS_POSTGRESQL
				if (Client->Base != nullptr)
					PQfinish(Client->Base);
#endif
				Client->Base = nullptr;
				Client->Master = nullptr;
				Client->Connected = false;
			}
			bool Queue::Push(Connection** Client)
			{
				if (!Client || !*Client)
					return false;

				Safe.lock();
				Clear(*Client);
				TH_RELEASE(*Client);
				Safe.unlock();

				return true;
			}
			Core::Async<Connection*> Queue::Pop()
			{
#ifdef TH_HAS_POSTGRESQL
				if (!Connected)
					return Core::Async<Connection*>::Store(nullptr);

				Safe.lock();
				if (!Inactive.empty())
				{
					Connection* Result = *Inactive.begin();
					Inactive.erase(Inactive.begin());
					Active.insert(Result);
					Safe.unlock();

					return Core::Async<Connection*>::Store(Result);
				}

				Connection* Result = new Connection();
				Active.insert(Result);
				Safe.unlock();

				return Result->Connect(Source).Then<Core::Async<Connection*>>([this, Result](bool&& Subresult)
				{
					if (Subresult)
					{
						Result->Master = this;
						return Core::Async<Connection*>::Store(Result);
					}

					this->Safe.lock();
					this->Active.erase(Result);
					this->Safe.unlock();

					TH_RELEASE(Result);
					return Core::Async<Connection*>::Store(nullptr);
				});
#else
				return Core::Async<Connection*>::Store(nullptr);
#endif
			}

			void Driver::Create()
			{
#ifdef TH_HAS_POSTGRESQL
				if (State <= 0)
				{
					Queries = new std::unordered_map<std::string, Sequence>();
					Safe = TH_NEW(std::mutex);
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
						TH_DELETE(mutex, Safe);
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
					char L = Base.R()[!Index ? Index : Index - 1];

					if (V == '\'')
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
								Pose Next;
								Next.Escape = (Base.R()[Arg] == '$');
                                Next.Key = Base.R().substr((size_t)Arg + 2, (size_t)Index - (size_t)Arg - 2);
								Next.Offset = (size_t)Arg;
								Result.Positions.push_back(std::move(Next));
								Base.RemovePart(Arg, Index + 1);
								Index -= Index - Arg - 1; Args++;
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
					else if ((L == '@' || L == '$') && V == '<')
					{
						if (!Spec && Arg < 0)
						{
							Arg = (!Index ? Index : Index - 1);
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
					else if (!Lock && (V == '\n' || V == '\r' || V == '\t' || (L == ' ' && V == ' ')))
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
				size_t Offset = 0;
				Safe->unlock();

				Core::Parser Result(&Origin.Request);
				for (auto& Word : Origin.Positions)
				{
					auto It = Map->find(Word.Key);
					if (It == Map->end())
						continue;

					std::string Value = GetSQL(Base, It->second, Word.Escape);
					if (Value.empty())
						continue;

					Result.Insert(Value, Word.Offset + Offset);
					Offset += Value.size();
				}

				if (Once)
				{
					for (auto& Item : *Map)
						TH_RELEASE(Item.second);
					Map->clear();
				}

				std::string Data = Origin.Request;
				if (Data.empty())
					TH_ERROR("could not construct query: \"%s\"", Name.c_str());

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
				{
					Core::Parser Dest(Src);
					Dest.Replace(TH_PREFIX_STR, TH_PREFIX_STR "\\");
					return Dest.Insert('\'', 0).Append('\'').R();
				}

				char* Subresult = PQescapeLiteral(Base->Get(), Src.c_str(), Src.size());
				std::string Result(Subresult);
				PQfreemem(Subresult);

				Core::Parser(&Result).Replace(TH_PREFIX_STR, TH_PREFIX_STR "\\");
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
					return "'\\x" + Compute::Common::HexEncode(Src, Size) + "'::bytea";

				size_t Length = 0;
				char* Subresult = (char*)PQescapeByteaConn(Base->Get(), (unsigned char*)Src, Size, &Length);
				std::string Result(Subresult, Length);
				PQfreemem((unsigned char*)Subresult);

				return Result;
#else
				return "'\\x" + Compute::Common::HexEncode(Src, Size) + "'::bytea";
#endif
			}
			std::string Driver::GetSQL(Connection* Base, Core::Document* Source, bool Escape)
			{
				if (!Source)
					return "NULL";

				Core::Document* Parent = Source->GetParent();
				bool Array = (Parent && Parent->Value.GetType() == Core::VarType_Array);

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
							Result.append(GetSQL(Base, Node, true)).append(1, ',');

						if (!Source->GetNodes()->empty())
							Result = Result.substr(0, Result.size() - 1);

						return Result + "]";
					}
					case Core::VarType_String:
					{
						std::string Result(GetCharArray(Base, Source->Value.GetBlob()));
						if (Escape)
							return Result;

						if (Result.front() != '\'' || Result.back() != '\'')
							return Result;

						if (Result.size() == 2)
							return "";

                        Result = Result.substr(1, Result.size() - 2);
						return Result;
					}
					case Core::VarType_Integer:
						return std::to_string(Source->Value.GetInteger());
					case Core::VarType_Number:
						return std::to_string(Source->Value.GetNumber());
					case Core::VarType_Boolean:
						return Source->Value.GetBoolean() ? "TRUE" : "FALSE";
					case Core::VarType_Decimal:
					{
                        std::string Result(GetCharArray(Base, Source->Value.GetDecimal().ToString()));
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
