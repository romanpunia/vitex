#include "mongodb.h"
extern "C"
{
#ifdef THAWK_HAS_MONGOC
#include <mongoc.h>
#define BS(X) (X != nullptr ? *X : nullptr)
#endif
}

namespace Tomahawk
{
	namespace Network
	{
		namespace MongoDB
		{
			QueryCache::QueryCache()
			{
			}
			QueryCache::~QueryCache()
			{
				Safe.lock();
				for (auto& Query : Queries)
					BSON::Document::Release(&Query.second.Cache);
				Safe.unlock();
			}
			bool QueryCache::AddQuery(const std::string& Name, const char* Buffer, size_t Size)
			{
				if (Name.empty() || !Buffer || !Size)
					return false;

				Sequence Result;
				Result.Request.assign(Buffer, Size);

				Rest::Stroke Base(&Result.Request);
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
								Base.Insert('\xFF', Arg);
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
					Result.Cache = BSON::Document::Create(Result.Request);

				Safe.lock();
				Queries[Name] = Result;
				Safe.unlock();

				return true;
			}
			bool QueryCache::AddDirectory(const std::string& Directory, const std::string& Origin)
			{
				std::vector<Rest::ResourceEntry> Entries;
				if (!Rest::OS::ScanDir(Directory, &Entries))
					return false;

				std::string Path = Directory;
				if (Path.back() != '/' && Path.back() != '\\')
					Path.append(1, '/');

				uint64_t Size = 0;
				for (auto& File : Entries)
				{
					Rest::Stroke Base(Path + File.Path);
					if (File.Source.IsDirectory)
					{
						AddDirectory(Base.R(), Origin.empty() ? Directory : Origin);
						continue;
					}

					if (!Base.EndsWith(".json"))
						continue;

					char* Buffer = (char*)Rest::OS::ReadAllBytes(Base.Get(), &Size);
					if (!Buffer)
						continue;

					Base.Replace(Origin.empty() ? Directory : Origin, "").Replace("\\", "/").Replace(".json", "");
					AddQuery(Base.Substring(1).R(), Buffer, Size);
					free(Buffer);
				}

				return true;
			}
			bool QueryCache::RemoveQuery(const std::string& Name)
			{
				Safe.lock();
				auto It = Queries.find(Name);
				if (It == Queries.end())
				{
					Safe.unlock();
					return false;
				}

				BSON::Document::Release(&It->second.Cache);
				Queries.erase(It);
				Safe.unlock();
				return true;
			}
			BSON::TDocument* QueryCache::GetQuery(const std::string& Name, QueryMap* Map, bool Once)
			{
				Safe.lock();
				auto It = Queries.find(Name);
				if (It == Queries.end())
				{
					Safe.unlock();
					return nullptr;
				}

				if (It->second.Cache != nullptr)
				{
					BSON::TDocument* Result = BSON::Document::Copy(It->second.Cache);
					Safe.unlock();

					return Result;
				}

				if (!Map)
				{
					BSON::TDocument* Result = BSON::Document::Create(It->second.Request);
					Safe.unlock();

					return Result;
				}

				Sequence Origin = It->second;
				Safe.unlock();

				Rest::Stroke Result(&Origin.Request);
				for (auto& Sub : *Map)
				{
					std::string Value = GetJSON(Sub.second);
					if (!Value.empty())
						Result.Replace("\xFF<" + Sub.first + '>', Value);

					if (Once)
						delete Sub.second;
				}

				if (Once)
					Map->clear();

				return BSON::Document::Create(Origin.Request);
			}
			std::vector<std::string> QueryCache::GetQueries()
			{
				std::vector<std::string> Result;
				Result.reserve(Queries.size());

				for (auto& Item : Queries)
					Result.push_back(Item.first);

				return Result;
			}
			std::string QueryCache::GetJSON(Rest::Document* Source)
			{
				if (!Source)
					return "";

				switch (Source->Type)
				{
					case Rest::NodeType_Object:
					{
						std::string Result = "{";
						for (auto* Node : *Source->GetNodes())
						{
							Result.append(1, '\"').append(Node->Name).append("\":");
							Result.append(GetJSON(Node)).append(1, ',');
						}

						if (!Source->GetNodes()->empty())
							Result = Result.substr(0, Result.size() - 1);

						return Result + "}";
					}
					case Rest::NodeType_Array:
					{
						std::string Result = "[";
						for (auto* Node : *Source->GetNodes())
							Result.append(GetJSON(Node)).append(1, ',');

						if (!Source->GetNodes()->empty())
							Result = Result.substr(0, Result.size() - 1);

						return Result + "]";
					}
					case Rest::NodeType_String:
						return "\"" + Source->String + "\"";
					case Rest::NodeType_Integer:
						return std::to_string(Source->Integer);
					case Rest::NodeType_Number:
						return std::to_string(Source->Number);
					case Rest::NodeType_Boolean:
						return Source->Boolean ? "true" : "false";
					case Rest::NodeType_Decimal:
					{
#ifdef THAWK_HAS_MONGOC
						Network::BSON::KeyPair Pair;
						Pair.Mod = Network::BSON::Type_Decimal;
						Pair.High = Source->Integer;
						Pair.Low = Source->Low;
						return "{\"$numberDouble\":\"" + Pair.ToString() + "\"}";
#endif
					}
					case Rest::NodeType_Id:
						return "{\"$oid\":\"" + BSON::Document::OIdToString((unsigned char*)Source->String.c_str()) + "\"}";
					case Rest::NodeType_Null:
						return "null";
					case Rest::NodeType_Undefined:
						return "undefined";
					default:
						break;
				}

				return "";
			}

			bool Connector::AddQuery(const std::string& Name, const char* Buffer, size_t Size)
			{
				if (!Cache)
					return false;

				return Cache->AddQuery(Name, Buffer, Size);
			}
			bool Connector::AddDirectory(const std::string& Directory)
			{
				if (!Cache)
					return false;

				return Cache->AddDirectory(Directory);
			}
			bool Connector::RemoveQuery(const std::string& Name)
			{
				if (!Cache)
					return false;

				return Cache->RemoveQuery(Name);
			}
			BSON::TDocument* Connector::GetQuery(const std::string& Name, QueryMap* Map, bool Once)
			{
				if (!Cache)
					return nullptr;

				return Cache->GetQuery(Name, Map, Once);
			}
			std::vector<std::string> Connector::GetQueries()
			{
				if (!Cache)
					return std::vector<std::string>();

				return Cache->GetQueries();
			}
			void Connector::Create()
			{
#ifdef THAWK_HAS_MONGOC
				if (State <= 0)
				{
					Cache = new QueryCache();
					mongoc_init();
					State = 1;
				}
				else
					State++;
#endif
			}
			void Connector::Release()
			{
#ifdef THAWK_HAS_MONGOC
				if (State == 1)
				{
					delete Cache;
					Cache = nullptr;
					mongoc_cleanup();
					State = 0;
				}
				else if (State > 0)
					State--;
#endif
			}
			QueryCache* Connector::Cache = nullptr;
			int Connector::State = 0;

			TURI* URI::Create(const char* Uri)
			{
#ifdef THAWK_HAS_MONGOC
				return mongoc_uri_new(Uri);
#else
				return nullptr;
#endif
			}
			void URI::Release(TURI** URI)
			{
#ifdef THAWK_HAS_MONGOC
				if (!URI || !*URI)
					return;

				mongoc_uri_destroy(*URI);
				*URI = nullptr;
#endif
			}
			void URI::SetOption(TURI* URI, const char* Name, int64_t Value)
			{
#ifdef THAWK_HAS_MONGOC
				if (URI != nullptr)
					mongoc_uri_set_option_as_int32(URI, Name, (int32_t)Value);
#endif
			}
			void URI::SetOption(TURI* URI, const char* Name, bool Value)
			{
#ifdef THAWK_HAS_MONGOC
				if (URI != nullptr)
					mongoc_uri_set_option_as_bool(URI, Name, Value);
#endif
			}
			void URI::SetOption(TURI* URI, const char* Name, const char* Value)
			{
#ifdef THAWK_HAS_MONGOC
				if (URI != nullptr)
					mongoc_uri_set_option_as_utf8(URI, Name, Value);
#endif
			}
			void URI::SetAuthMechanism(TURI* URI, const char* Value)
			{
#ifdef THAWK_HAS_MONGOC
				if (URI != nullptr)
					mongoc_uri_set_auth_mechanism(URI, Value);
#endif
			}
			void URI::SetAuthSource(TURI* URI, const char* Value)
			{
#ifdef THAWK_HAS_MONGOC
				if (URI != nullptr)
					mongoc_uri_set_auth_source(URI, Value);
#endif
			}
			void URI::SetCompressors(TURI* URI, const char* Value)
			{
#ifdef THAWK_HAS_MONGOC
				if (URI != nullptr)
					mongoc_uri_set_compressors(URI, Value);
#endif
			}
			void URI::SetDatabase(TURI* URI, const char* Value)
			{
#ifdef THAWK_HAS_MONGOC
				if (URI != nullptr)
					mongoc_uri_set_database(URI, Value);
#endif
			}
			void URI::SetUsername(TURI* URI, const char* Value)
			{
#ifdef THAWK_HAS_MONGOC
				if (URI != nullptr)
					mongoc_uri_set_username(URI, Value);
#endif
			}
			void URI::SetPassword(TURI* URI, const char* Value)
			{
#ifdef THAWK_HAS_MONGOC
				if (URI != nullptr)
					mongoc_uri_set_password(URI, Value);
#endif
			}

			TFindAndModifyOptions* FindAndModifyOptions::Create()
			{
#ifdef THAWK_HAS_MONGOC
				return mongoc_find_and_modify_opts_new();
#else
				return nullptr;
#endif
			}
			void FindAndModifyOptions::Release(TFindAndModifyOptions** Options)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Options || !*Options)
					return;

				mongoc_find_and_modify_opts_destroy(*Options);
				*Options = nullptr;
#endif
			}
			bool FindAndModifyOptions::SetFlags(TFindAndModifyOptions* Options, FindAndModifyMode Flags)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Options)
					return false;

				return mongoc_find_and_modify_opts_set_flags(Options, (mongoc_find_and_modify_flags_t)Flags);
#else
				return false;
#endif
			}
			bool FindAndModifyOptions::SetMaxTimeMs(TFindAndModifyOptions* Options, uint64_t Time)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Options)
					return false;

				return mongoc_find_and_modify_opts_set_max_time_ms(Options, (uint32_t)Time);
#else
				return false;
#endif
			}
			bool FindAndModifyOptions::SetFields(TFindAndModifyOptions* Options, BSON::TDocument** Fields)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Fields || !Options)
				{
					BSON::Document::Release(Fields);
					return false;
				}

				bool Result = mongoc_find_and_modify_opts_set_fields(Options, *Fields);
				BSON::Document::Release(Fields);

				return Result;
#else
				return false;
#endif
			}
			bool FindAndModifyOptions::SetSort(TFindAndModifyOptions* Options, BSON::TDocument** Sort)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Sort || !Options)
				{
					BSON::Document::Release(Sort);
					return false;
				}

				bool Result = mongoc_find_and_modify_opts_set_sort(Options, *Sort);
				BSON::Document::Release(Sort);

				return Result;
#else
				return false;
#endif
			}
			bool FindAndModifyOptions::SetUpdate(TFindAndModifyOptions* Options, BSON::TDocument** Update)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Update || !Options)
				{
					BSON::Document::Release(Update);
					return false;
				}

				bool Result = mongoc_find_and_modify_opts_set_update(Options, *Update);
				BSON::Document::Release(Update);

				return Result;
#else
				return false;
#endif
			}
			bool FindAndModifyOptions::SetBypassDocumentValidation(TFindAndModifyOptions* Options, bool Bypass)
			{
#ifdef THAWK_HAS_MONGOC
				return mongoc_find_and_modify_opts_set_bypass_document_validation(Options, Bypass);
#else
				return false;
#endif
			}
			bool FindAndModifyOptions::Append(TFindAndModifyOptions* Options, BSON::TDocument** Opts)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Opts || !Options)
				{
					BSON::Document::Release(Opts);
					return false;
				}

				bool Result = mongoc_find_and_modify_opts_append(Options, *Opts);
				BSON::Document::Release(Opts);

				return Result;
#else
				return false;
#endif
			}
			bool FindAndModifyOptions::WillBypassDocumentValidation(TFindAndModifyOptions* Options)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Options)
					return false;

				return mongoc_find_and_modify_opts_get_bypass_document_validation(Options);
#else
				return false;
#endif
			}
			uint64_t FindAndModifyOptions::GetMaxTimeMs(TFindAndModifyOptions* Options)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Options)
					return 0;

				return mongoc_find_and_modify_opts_get_max_time_ms(Options);
#else
				return 0;
#endif
			}
			FindAndModifyMode FindAndModifyOptions::GetFlags(TFindAndModifyOptions* Options)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Options)
					return FindAndModifyMode_None;

				return (FindAndModifyMode)mongoc_find_and_modify_opts_get_flags(Options);
#else
				return FindAndModifyMode_None;
#endif
			}
			BSON::TDocument* FindAndModifyOptions::GetFields(TFindAndModifyOptions* Options)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Options)
					return nullptr;

				BSON::TDocument* Document = bson_new();
				mongoc_find_and_modify_opts_get_fields(Options, Document);
				return Document;
#else
				return nullptr;
#endif
			}
			BSON::TDocument* FindAndModifyOptions::GetSort(TFindAndModifyOptions* Options)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Options)
					return nullptr;

				BSON::TDocument* Document = bson_new();
				mongoc_find_and_modify_opts_get_sort(Options, Document);
				return Document;
#else
				return nullptr;
#endif
			}
			BSON::TDocument* FindAndModifyOptions::GetUpdate(TFindAndModifyOptions* Options)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Options)
					return nullptr;

				BSON::TDocument* Document = bson_new();
				mongoc_find_and_modify_opts_get_update(Options, Document);
				return Document;
#else
				return nullptr;
#endif
			}

			TBulkOperation* BulkOperation::Create(bool IsOrdered)
			{
#ifdef THAWK_HAS_MONGOC
				return mongoc_bulk_operation_new(IsOrdered);
#else
				return nullptr;
#endif
			}
			void BulkOperation::Release(TBulkOperation** Operation)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Operation || !*Operation)
					return;

				mongoc_bulk_operation_destroy(*Operation);
				*Operation = nullptr;
#endif
			}
			bool BulkOperation::RemoveMany(TBulkOperation* Operation, BSON::TDocument** Selector, BSON::TDocument** Options)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Operation)
				{
					BSON::Document::Release(Selector);
					BSON::Document::Release(Options);
					return false;
				}

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!mongoc_bulk_operation_remove_many_with_opts(Operation, BS(Selector), BS(Options), &Error))
				{
					BSON::Document::Release(Selector);
					BSON::Document::Release(Options);
					THAWK_ERROR("couldn't build command -> %s", Error.message);
					return false;
				}

				BSON::Document::Release(Selector);
				BSON::Document::Release(Options);
				return true;
#else
				return false;
#endif
			}
			bool BulkOperation::RemoveOne(TBulkOperation* Operation, BSON::TDocument** Selector, BSON::TDocument** Options)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Operation)
				{
					BSON::Document::Release(Selector);
					BSON::Document::Release(Options);
					return false;
				}

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!mongoc_bulk_operation_remove_one_with_opts(Operation, BS(Selector), BS(Options), &Error))
				{
					BSON::Document::Release(Selector);
					BSON::Document::Release(Options);
					THAWK_ERROR("couldn't build command -> %s", Error.message);
					return false;
				}

				BSON::Document::Release(Selector);
				BSON::Document::Release(Options);
				return true;
#else
				return false;
#endif
			}
			bool BulkOperation::ReplaceOne(TBulkOperation* Operation, BSON::TDocument** Selector, BSON::TDocument** Replacement, BSON::TDocument** Options)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Operation)
				{
					BSON::Document::Release(Selector);
					BSON::Document::Release(Replacement);
					BSON::Document::Release(Options);
					return false;
				}

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!mongoc_bulk_operation_replace_one_with_opts(Operation, BS(Selector), BS(Replacement), BS(Options), &Error))
				{
					BSON::Document::Release(Selector);
					BSON::Document::Release(Replacement);
					BSON::Document::Release(Options);
					THAWK_ERROR("couldn't build command -> %s", Error.message);
					return false;
				}

				BSON::Document::Release(Selector);
				BSON::Document::Release(Replacement);
				BSON::Document::Release(Options);
				return true;
#else
				return false;
#endif
			}
			bool BulkOperation::Insert(TBulkOperation* Operation, BSON::TDocument** Document, BSON::TDocument** Options)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Operation)
				{
					BSON::Document::Release(Document);
					BSON::Document::Release(Options);
					return false;
				}

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!mongoc_bulk_operation_insert_with_opts(Operation, BS(Document), BS(Options), &Error))
				{
					BSON::Document::Release(Document);
					BSON::Document::Release(Options);
					THAWK_ERROR("couldn't build command -> %s", Error.message);
					return false;
				}

				BSON::Document::Release(Document);
				BSON::Document::Release(Options);
				return true;
#else
				return false;
#endif
			}
			bool BulkOperation::UpdateOne(TBulkOperation* Operation, BSON::TDocument** Selector, BSON::TDocument** Document, BSON::TDocument** Options)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Operation)
				{
					BSON::Document::Release(Selector);
					BSON::Document::Release(Document);
					BSON::Document::Release(Options);
					return false;
				}

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!mongoc_bulk_operation_update_one_with_opts(Operation, BS(Selector), BS(Document), BS(Options), &Error))
				{
					BSON::Document::Release(Selector);
					BSON::Document::Release(Document);
					BSON::Document::Release(Options);
					THAWK_ERROR("couldn't build command -> %s", Error.message);
					return false;
				}

				BSON::Document::Release(Selector);
				BSON::Document::Release(Document);
				BSON::Document::Release(Options);
				return true;
#else
				return false;
#endif
			}
			bool BulkOperation::UpdateMany(TBulkOperation* Operation, BSON::TDocument** Selector, BSON::TDocument** Document, BSON::TDocument** Options)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Operation)
				{
					BSON::Document::Release(Selector);
					BSON::Document::Release(Document);
					BSON::Document::Release(Options);
					return false;
				}

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!mongoc_bulk_operation_update_many_with_opts(Operation, BS(Selector), BS(Document), BS(Options), &Error))
				{
					BSON::Document::Release(Selector);
					BSON::Document::Release(Document);
					BSON::Document::Release(Options);
					THAWK_ERROR("couldn't build command -> %s", Error.message);
					return false;
				}

				BSON::Document::Release(Selector);
				BSON::Document::Release(Document);
				BSON::Document::Release(Options);
				return true;
#else
				return false;
#endif
			}
			bool BulkOperation::RemoveMany(TBulkOperation* Operation, BSON::TDocument* Selector, BSON::TDocument* Options)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Operation)
					return false;

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!mongoc_bulk_operation_remove_many_with_opts(Operation, Selector, Options, &Error))
				{
					THAWK_ERROR("couldn't build command -> %s", Error.message);
					return false;
				}

				return true;
#else
				return false;
#endif
			}
			bool BulkOperation::RemoveOne(TBulkOperation* Operation, BSON::TDocument* Selector, BSON::TDocument* Options)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Operation)
					return false;

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!mongoc_bulk_operation_remove_one_with_opts(Operation, Selector, Options, &Error))
				{
					THAWK_ERROR("couldn't build command -> %s", Error.message);
					return false;
				}

				return true;
#else
				return false;
#endif
			}
			bool BulkOperation::ReplaceOne(TBulkOperation* Operation, BSON::TDocument* Selector, BSON::TDocument* Replacement, BSON::TDocument* Options)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Operation)
					return false;

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!mongoc_bulk_operation_replace_one_with_opts(Operation, Selector, Replacement, Options, &Error))
				{
					THAWK_ERROR("couldn't build command -> %s", Error.message);
					return false;
				}

				return true;
#else
				return false;
#endif
			}
			bool BulkOperation::Insert(TBulkOperation* Operation, BSON::TDocument* Document, BSON::TDocument* Options)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Operation)
					return false;

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!mongoc_bulk_operation_insert_with_opts(Operation, Document, Options, &Error))
				{
					THAWK_ERROR("couldn't build command -> %s", Error.message);
					return false;
				}

				return true;
#else
				return false;
#endif
			}
			bool BulkOperation::UpdateOne(TBulkOperation* Operation, BSON::TDocument* Selector, BSON::TDocument* Document, BSON::TDocument* Options)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Operation)
					return false;

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!mongoc_bulk_operation_update_one_with_opts(Operation, Selector, Document, Options, &Error))
				{
					THAWK_ERROR("couldn't build command -> %s", Error.message);
					return false;
				}

				return true;
#else
				return false;
#endif
			}
			bool BulkOperation::UpdateMany(TBulkOperation* Operation, BSON::TDocument* Selector, BSON::TDocument* Document, BSON::TDocument* Options)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Operation)
					return false;

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!mongoc_bulk_operation_update_many_with_opts(Operation, Selector, Document, Options, &Error))
				{
					THAWK_ERROR("couldn't build command -> %s", Error.message);
					return false;
				}

				return true;
#else
				return false;
#endif
			}
			bool BulkOperation::Execute(TBulkOperation** Operation)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Operation || !*Operation)
					return false;

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				BSON::TDocument Result;
				if (!mongoc_bulk_operation_execute(*Operation, &Result, &Error))
				{
					Release(Operation);
					bson_destroy(&Result);
					return false;
				}

				Release(Operation);
				bson_destroy(&Result);
				return true;
#else
				return false;
#endif
			}
			bool BulkOperation::Execute(TBulkOperation** Operation, BSON::TDocument** Reply)
			{
#ifdef THAWK_HAS_MONGOC
				if (Reply == nullptr || *Reply == nullptr)
					return Execute(Operation);

				if (!Operation || !*Operation)
					return false;

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!mongoc_bulk_operation_execute(*Operation, BS(Reply), &Error))
				{
					Release(Operation);
					return false;
				}

				Release(Operation);
				return true;
#else
				return false;
#endif
			}
			uint64_t BulkOperation::GetHint(TBulkOperation* Operation)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Operation)
					return 0;

				return (uint64_t)mongoc_bulk_operation_get_hint(Operation);
#else
				return 0;
#endif
			}
			TWriteConcern* BulkOperation::GetWriteConcern(TBulkOperation* Operation)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Operation)
					return nullptr;

				return (TWriteConcern*)mongoc_bulk_operation_get_write_concern(Operation);
#else
				return nullptr;
#endif
			}

			TChangeStream* ChangeStream::Create(TConnection* Connection, BSON::TDocument** Pipeline, BSON::TDocument** Options)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Connection)
				{
					BSON::Document::Release(Pipeline);
					BSON::Document::Release(Options);
					return nullptr;
				}

				TChangeStream* Stream = mongoc_client_watch(Connection, BS(Pipeline), BS(Options));
				BSON::Document::Release(Pipeline);
				BSON::Document::Release(Options);

				return Stream;
#else
				return nullptr;
#endif
			}
			TChangeStream* ChangeStream::Create(TDatabase* Database, BSON::TDocument** Pipeline, BSON::TDocument** Options)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Database)
				{
					BSON::Document::Release(Pipeline);
					BSON::Document::Release(Options);
					return nullptr;
				}

				TChangeStream* Stream = mongoc_database_watch(Database, BS(Pipeline), BS(Options));
				BSON::Document::Release(Pipeline);
				BSON::Document::Release(Options);

				return Stream;
#else
				return nullptr;
#endif
			}
			TChangeStream* ChangeStream::Create(TCollection* Collection, BSON::TDocument** Pipeline, BSON::TDocument** Options)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Collection)
				{
					BSON::Document::Release(Pipeline);
					BSON::Document::Release(Options);
					return nullptr;
				}

				TChangeStream* Stream = mongoc_collection_watch(Collection, BS(Pipeline), BS(Options));
				BSON::Document::Release(Pipeline);
				BSON::Document::Release(Options);

				return Stream;
#else
				return nullptr;
#endif
			}
			void ChangeStream::Release(TChangeStream** Stream)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Stream || !*Stream)
					return;

				mongoc_change_stream_destroy(*Stream);
				*Stream = nullptr;
#endif
			}
			bool ChangeStream::Next(TChangeStream* Stream, BSON::TDocument** Result)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Stream)
				{
					BSON::Document::Release(Result);
					return false;
				}

				return mongoc_change_stream_next(Stream, (const BSON::TDocument**)Result);
#else
				return false;
#endif
			}
			bool ChangeStream::Error(TChangeStream* Stream, BSON::TDocument** Result)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Stream)
				{
					BSON::Document::Release(Result);
					return false;
				}

				return mongoc_change_stream_error_document(Stream, nullptr, (const BSON::TDocument**)Result);
#else
				return false;
#endif
			}

			TReadConcern* ReadConcern::Create()
			{
#ifdef THAWK_HAS_MONGOC
				return mongoc_read_concern_new();
#else
				return nullptr;
#endif
			}
			void ReadConcern::Release(TReadConcern** ReadConcern)
			{
#ifdef THAWK_HAS_MONGOC
				if (!ReadConcern || !*ReadConcern)
					return;

				mongoc_read_concern_destroy(*ReadConcern);
				*ReadConcern = nullptr;
#endif
			}
			void ReadConcern::Append(TReadConcern* ReadConcern, BSON::TDocument* Options)
			{
#ifdef THAWK_HAS_MONGOC
				if (ReadConcern != nullptr)
					mongoc_read_concern_append(ReadConcern, Options);
#endif
			}
			void ReadConcern::SetLevel(TReadConcern* ReadConcern, const char* Level)
			{
#ifdef THAWK_HAS_MONGOC
				if (ReadConcern != nullptr)
					mongoc_read_concern_set_level(ReadConcern, Level);
#endif
			}
			bool ReadConcern::IsDefault(TReadConcern* ReadConcern)
			{
#ifdef THAWK_HAS_MONGOC
				if (!ReadConcern)
					return false;

				return mongoc_read_concern_is_default(ReadConcern);
#else
				return false;
#endif
			}
			const char* ReadConcern::GetLevel(TReadConcern* ReadConcern)
			{
#ifdef THAWK_HAS_MONGOC
				if (!ReadConcern)
					return nullptr;

				return mongoc_read_concern_get_level(ReadConcern);
#else
				return nullptr;
#endif
			}

			TWriteConcern* WriteConcern::Create()
			{
#ifdef THAWK_HAS_MONGOC
				return mongoc_write_concern_new();
#else
				return nullptr;
#endif
			}
			void WriteConcern::Release(TWriteConcern** WriteConcern)
			{
#ifdef THAWK_HAS_MONGOC
				if (!WriteConcern || !*WriteConcern)
					return;

				mongoc_write_concern_destroy(*WriteConcern);
				*WriteConcern = nullptr;
#endif
			}
			void WriteConcern::Append(TWriteConcern* WriteConcern, BSON::TDocument* Options)
			{
#ifdef THAWK_HAS_MONGOC
				if (WriteConcern != nullptr)
					mongoc_write_concern_append(WriteConcern, Options);
#endif
			}
			void WriteConcern::SetToken(TWriteConcern* WriteConcern, uint64_t W)
			{
#ifdef THAWK_HAS_MONGOC
				if (WriteConcern != nullptr)
					mongoc_write_concern_set_w(WriteConcern, (int32_t)W);
#endif
			}
			void WriteConcern::SetMajority(TWriteConcern* WriteConcern, uint64_t Timeout)
			{
#ifdef THAWK_HAS_MONGOC
				if (WriteConcern != nullptr)
					mongoc_write_concern_set_wmajority(WriteConcern, (int32_t)Timeout);
#endif
			}
			void WriteConcern::SetTimeout(TWriteConcern* WriteConcern, uint64_t Timeout)
			{
#ifdef THAWK_HAS_MONGOC
				if (WriteConcern != nullptr)
					mongoc_write_concern_set_wtimeout(WriteConcern, (int32_t)Timeout);
#endif
			}
			void WriteConcern::SetTag(TWriteConcern* WriteConcern, const char* Tag)
			{
#ifdef THAWK_HAS_MONGOC
				if (WriteConcern != nullptr)
					mongoc_write_concern_set_wtag(WriteConcern, Tag);
#endif
			}
			void WriteConcern::SetJournaled(TWriteConcern* WriteConcern, bool Enabled)
			{
#ifdef THAWK_HAS_MONGOC
				if (WriteConcern != nullptr)
					mongoc_write_concern_set_journal(WriteConcern, Enabled);
#endif
			}
			bool WriteConcern::ShouldBeJournaled(TWriteConcern* WriteConcern)
			{
#ifdef THAWK_HAS_MONGOC
				if (!WriteConcern)
					return false;

				return mongoc_write_concern_get_journal(WriteConcern);
#else
				return false;
#endif
			}
			bool WriteConcern::HasJournal(TWriteConcern* WriteConcern)
			{
#ifdef THAWK_HAS_MONGOC
				if (!WriteConcern)
					return false;

				return mongoc_write_concern_journal_is_set(WriteConcern);
#else
				return false;
#endif
			}
			bool WriteConcern::HasMajority(TWriteConcern* WriteConcern)
			{
#ifdef THAWK_HAS_MONGOC
				if (!WriteConcern)
					return false;

				return mongoc_write_concern_get_wmajority(WriteConcern);
#else
				return false;
#endif
			}
			bool WriteConcern::IsAcknowledged(TWriteConcern* WriteConcern)
			{
#ifdef THAWK_HAS_MONGOC
				if (!WriteConcern)
					return false;

				return mongoc_write_concern_is_acknowledged(WriteConcern);
#else
				return false;
#endif
			}
			bool WriteConcern::IsDefault(TWriteConcern* WriteConcern)
			{
#ifdef THAWK_HAS_MONGOC
				if (!WriteConcern)
					return false;

				return mongoc_write_concern_is_default(WriteConcern);
#else
				return false;
#endif
			}
			bool WriteConcern::IsValid(TWriteConcern* WriteConcern)
			{
#ifdef THAWK_HAS_MONGOC
				if (!WriteConcern)
					return false;

				return mongoc_write_concern_is_valid(WriteConcern);
#else
				return false;
#endif
			}
			int64_t WriteConcern::GetToken(TWriteConcern* WriteConcern)
			{
#ifdef THAWK_HAS_MONGOC
				if (!WriteConcern)
					return 0;

				return mongoc_write_concern_get_w(WriteConcern);
#else
				return 0;
#endif
			}
			int64_t WriteConcern::GetTimeout(TWriteConcern* WriteConcern)
			{
#ifdef THAWK_HAS_MONGOC
				if (!WriteConcern)
					return 0;

				return mongoc_write_concern_get_wtimeout(WriteConcern);
#else
				return 0;
#endif
			}
			const char* WriteConcern::GetTag(TWriteConcern* WriteConcern)
			{
#ifdef THAWK_HAS_MONGOC
				if (!WriteConcern)
					return nullptr;

				return mongoc_write_concern_get_wtag(WriteConcern);
#else
				return nullptr;
#endif
			}

			TReadPreferences* ReadPreferences::Create()
			{
#ifdef THAWK_HAS_MONGOC
				return mongoc_read_prefs_new(MONGOC_READ_PRIMARY);
#else
				return nullptr;
#endif
			}
			void ReadPreferences::Release(TReadPreferences** ReadPreferences)
			{
#ifdef THAWK_HAS_MONGOC
				if (!ReadPreferences || !*ReadPreferences)
					return;

				mongoc_read_prefs_destroy(*ReadPreferences);
				*ReadPreferences = nullptr;
#endif
			}
			void ReadPreferences::SetTags(TReadPreferences* ReadPreferences, BSON::TDocument** Tags)
			{
#ifdef THAWK_HAS_MONGOC
				if (!ReadPreferences)
				{
					BSON::Document::Release(Tags);
					return;
				}

				mongoc_read_prefs_set_tags(ReadPreferences, BS(Tags));
				BSON::Document::Release(Tags);
#endif
			}
			void ReadPreferences::SetMode(TReadPreferences* ReadPreferences, ReadMode Mode)
			{
#ifdef THAWK_HAS_MONGOC
				if (ReadPreferences != nullptr)
					mongoc_read_prefs_set_mode(ReadPreferences, (mongoc_read_mode_t)Mode);
#endif
			}
			void ReadPreferences::SetMaxStalenessSeconds(TReadPreferences* ReadPreferences, uint64_t MaxStalenessSeconds)
			{
#ifdef THAWK_HAS_MONGOC
				if (ReadPreferences != nullptr)
					mongoc_read_prefs_set_max_staleness_seconds(ReadPreferences, (int64_t)MaxStalenessSeconds);
#endif
			}
			bool ReadPreferences::IsValid(TReadPreferences* ReadPreferences)
			{
#ifdef THAWK_HAS_MONGOC
				if (!ReadPreferences)
					return false;

				return mongoc_read_prefs_is_valid(ReadPreferences);
#else
				return false;
#endif
			}
			ReadMode ReadPreferences::GetMode(TReadPreferences* ReadPreferences)
			{
#ifdef THAWK_HAS_MONGOC
				if (!ReadPreferences)
					return ReadMode_Primary;

				return (ReadMode)mongoc_read_prefs_get_mode(ReadPreferences);
#else
				return ReadMode_Primary;
#endif
			}
			uint64_t ReadPreferences::GetMaxStalenessSeconds(TReadPreferences* ReadPreferences)
			{
#ifdef THAWK_HAS_MONGOC
				if (!ReadPreferences)
					return 0;

				return (uint64_t)mongoc_read_prefs_get_max_staleness_seconds(ReadPreferences);
#else
				return 0;
#endif
			}

			void Cursor::Release(TCursor** Cursor)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Cursor || !*Cursor)
					return;

				mongoc_cursor_destroy(*Cursor);
				*Cursor = nullptr;
#endif
			}
			void Cursor::Receive(TCursor* Cursor, void* Context, bool(* Next)(TCursor*, BSON::TDocument*, void*))
			{
#ifdef THAWK_HAS_MONGOC
				if (!Next || !Cursor)
					return;

				BSON::TDocument* Query = nullptr;
				while (mongoc_cursor_next(Cursor, (const BSON::TDocument**)&Query))
				{
					if (!Next(Cursor, Query, Context))
						break;
				}
#endif
			}
			void Cursor::SetMaxAwaitTime(TCursor* Cursor, uint64_t MaxAwaitTime)
			{
#ifdef THAWK_HAS_MONGOC
				if (Cursor != nullptr)
					mongoc_cursor_set_max_await_time_ms(Cursor, (uint32_t)MaxAwaitTime);
#endif
			}
			void Cursor::SetBatchSize(TCursor* Cursor, uint64_t BatchSize)
			{
#ifdef THAWK_HAS_MONGOC
				if (Cursor != nullptr)
					mongoc_cursor_set_batch_size(Cursor, (uint32_t)BatchSize);
#endif
			}
			bool Cursor::Next(TCursor* Cursor)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Cursor)
					return false;

				BSON::TDocument* Query = nullptr;
				return mongoc_cursor_next(Cursor, (const BSON::TDocument**)&Query);
#else
				return false;
#endif
			}
			bool Cursor::SetLimit(TCursor* Cursor, int64_t Limit)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Cursor)
					return false;

				return mongoc_cursor_set_limit(Cursor, Limit);
#else
				return false;
#endif
			}
			bool Cursor::SetHint(TCursor* Cursor, uint64_t Hint)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Cursor)
					return false;

				return mongoc_cursor_set_hint(Cursor, (uint32_t)Hint);
#else
				return false;
#endif
			}
			bool Cursor::HasError(TCursor* Cursor)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Cursor)
					return false;

				return mongoc_cursor_error(Cursor, nullptr);
#else
				return false;
#endif
			}
			bool Cursor::HasMoreData(TCursor* Cursor)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Cursor)
					return false;

				return mongoc_cursor_more(Cursor);
#else
				return false;
#endif
			}
			int64_t Cursor::GetId(TCursor* Cursor)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Cursor)
					return 0;

				return (int64_t)mongoc_cursor_get_id(Cursor);
#else
				return 0;
#endif
			}
			int64_t Cursor::GetLimit(TCursor* Cursor)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Cursor)
					return 0;

				return (int64_t)mongoc_cursor_get_limit(Cursor);
#else
				return 0;
#endif
			}
			uint64_t Cursor::GetMaxAwaitTime(TCursor* Cursor)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Cursor)
					return 0;

				return (uint64_t)mongoc_cursor_get_max_await_time_ms(Cursor);
#else
				return 0;
#endif
			}
			uint64_t Cursor::GetBatchSize(TCursor* Cursor)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Cursor)
					return 0;

				return (uint64_t)mongoc_cursor_get_batch_size(Cursor);
#else
				return 0;
#endif
			}
			uint64_t Cursor::GetHint(TCursor* Cursor)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Cursor)
					return 0;

				return (uint64_t)mongoc_cursor_get_hint(Cursor);
#else
				return 0;
#endif
			}
			HostList Cursor::GetHosts(TCursor* Cursor)
			{
				HostList Hosts;
				if (!Cursor)
					return Hosts;

#ifdef THAWK_HAS_MONGOC
				mongoc_cursor_get_host(Cursor, (mongoc_host_list_t*)&Hosts);
#endif
				return Hosts;
			}
			BSON::TDocument* Cursor::GetCurrent(TCursor* Cursor)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Cursor)
					return nullptr;

				return (BSON::TDocument*)mongoc_cursor_current(Cursor);
#else
				return nullptr;
#endif
			}

			void Collection::Release(TCollection** Collection)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Collection || !*Collection)
					return;

				mongoc_collection_destroy(*Collection);
				*Collection = nullptr;
#endif
			}
			void Collection::SetReadConcern(TCollection* Collection, TReadConcern* Concern)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Collection || !Concern)
					return;

				mongoc_collection_set_read_concern(Collection, Concern);
#endif
			}
			void Collection::SetWriteConcern(TCollection* Collection, TWriteConcern* Concern)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Collection || !Concern)
					return;

				mongoc_collection_set_write_concern(Collection, Concern);
#endif
			}
			void Collection::SetReadPreferences(TCollection* Collection, TReadPreferences* Preferences)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Collection || !Preferences)
					return;

				mongoc_collection_set_read_prefs(Collection, Preferences);
#endif
			}
			bool Collection::UpdateDocument(TCollection* Collection, UpdateMode Flags, BSON::TDocument** Selector, BSON::TDocument** Update, TWriteConcern* Concern)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Collection)
				{
					BSON::Document::Release(Selector);
					BSON::Document::Release(Update);
					return false;
				}

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!mongoc_collection_update(Collection, (mongoc_update_flags_t)Flags, BS(Selector), BS(Update), Concern, &Error))
				{
					BSON::Document::Release(Selector);
					BSON::Document::Release(Update);
					THAWK_ERROR(Error.message);
					return false;
				}

				BSON::Document::Release(Selector);
				BSON::Document::Release(Update);
				return true;
#else
				return false;
#endif
			}
			bool Collection::UpdateMany(TCollection* Collection, BSON::TDocument** Selector, BSON::TDocument** Update, BSON::TDocument** Options, BSON::TDocument** Reply)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Collection)
				{
					BSON::Document::Release(Selector);
					BSON::Document::Release(Update);
					BSON::Document::Release(Options);
					return false;
				}

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!mongoc_collection_update_many(Collection, BS(Selector), BS(Update), BS(Options), BS(Reply), &Error))
				{
					BSON::Document::Release(Selector);
					BSON::Document::Release(Update);
					BSON::Document::Release(Options);
					return false;
				}

				BSON::Document::Release(Selector);
				BSON::Document::Release(Update);
				BSON::Document::Release(Options);
				return true;
#else
				return false;
#endif
			}
			bool Collection::UpdateOne(TCollection* Collection, BSON::TDocument** Selector, BSON::TDocument** Update, BSON::TDocument** Options, BSON::TDocument** Reply)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Collection)
				{
					BSON::Document::Release(Selector);
					BSON::Document::Release(Update);
					BSON::Document::Release(Options);
					return false;
				}

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!mongoc_collection_update_one(Collection, BS(Selector), BS(Update), BS(Options), BS(Reply), &Error))
				{
					BSON::Document::Release(Selector);
					BSON::Document::Release(Update);
					BSON::Document::Release(Options);
					THAWK_ERROR(Error.message);
					return false;
				}

				BSON::Document::Release(Selector);
				BSON::Document::Release(Update);
				BSON::Document::Release(Options);
				return true;
#else
				return false;
#endif
			}
			bool Collection::Rename(TCollection* Collection, const char* NewDatabaseName, const char* NewCollectionName)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Collection)
					return false;

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!mongoc_collection_rename(Collection, NewDatabaseName, NewCollectionName, false, &Error))
				{
					THAWK_ERROR(Error.message);
					return false;
				}

				return true;
#else
				return false;
#endif
			}
			bool Collection::RenameWithOptions(TCollection* Collection, const char* NewDatabaseName, const char* NewCollectionName, BSON::TDocument** Options)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Collection)
				{
					BSON::Document::Release(Options);
					return false;
				}

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!mongoc_collection_rename_with_opts(Collection, NewDatabaseName, NewCollectionName, false, BS(Options), &Error))
				{
					BSON::Document::Release(Options);
					THAWK_ERROR(Error.message);
					return false;
				}

				BSON::Document::Release(Options);
				return true;
#else
				return false;
#endif
			}
			bool Collection::RenameWithRemove(TCollection* Collection, const char* NewDatabaseName, const char* NewCollectionName)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Collection)
					return false;

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!mongoc_collection_rename(Collection, NewDatabaseName, NewCollectionName, true, &Error))
				{
					THAWK_ERROR(Error.message);
					return false;
				}

				return true;
#else
				return false;
#endif
			}
			bool Collection::RenameWithOptionsAndRemove(TCollection* Collection, const char* NewDatabaseName, const char* NewCollectionName, BSON::TDocument** Options)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Collection)
				{
					BSON::Document::Release(Options);
					return false;
				}

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!mongoc_collection_rename_with_opts(Collection, NewDatabaseName, NewCollectionName, true, BS(Options), &Error))
				{
					BSON::Document::Release(Options);
					THAWK_ERROR(Error.message);
					return false;
				}

				BSON::Document::Release(Options);
				return true;
#else
				return false;
#endif
			}
			bool Collection::Remove(TCollection* Collection, BSON::TDocument** Options)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Collection)
				{
					BSON::Document::Release(Options);
					return false;
				}

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!mongoc_collection_drop_with_opts(Collection, BS(Options), &Error))
				{
					BSON::Document::Release(Options);
					THAWK_ERROR(Error.message);
					return false;
				}

				BSON::Document::Release(Options);
				return true;
#else
				return false;
#endif
			}
			bool Collection::RemoveDocument(TCollection* Collection, RemoveMode Flags, BSON::TDocument** Selector, TWriteConcern* Concern)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Collection)
				{
					BSON::Document::Release(Selector);
					return false;
				}

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!mongoc_collection_remove(Collection, (mongoc_remove_flags_t)Flags, BS(Selector), Concern, &Error))
				{
					BSON::Document::Release(Selector);
					THAWK_ERROR(Error.message);
					return false;
				}

				BSON::Document::Release(Selector);
				return true;
#else
				return false;
#endif
			}
			bool Collection::RemoveMany(TCollection* Collection, BSON::TDocument** Selector, BSON::TDocument** Options, BSON::TDocument** Reply)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Collection)
				{
					BSON::Document::Release(Selector);
					BSON::Document::Release(Options);
					return false;
				}

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!mongoc_collection_delete_many(Collection, BS(Selector), BS(Options), BS(Reply), &Error))
				{
					BSON::Document::Release(Selector);
					BSON::Document::Release(Options);
					THAWK_ERROR(Error.message);
					return false;
				}

				BSON::Document::Release(Selector);
				BSON::Document::Release(Options);
				return true;
#else
				return false;
#endif
			}
			bool Collection::RemoveOne(TCollection* Collection, BSON::TDocument** Selector, BSON::TDocument** Options, BSON::TDocument** Reply)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Collection)
				{
					BSON::Document::Release(Selector);
					BSON::Document::Release(Options);
					return false;
				}

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!mongoc_collection_delete_one(Collection, BS(Selector), BS(Options), BS(Reply), &Error))
				{
					BSON::Document::Release(Selector);
					BSON::Document::Release(Options);
					THAWK_ERROR(Error.message);
					return false;
				}

				BSON::Document::Release(Selector);
				BSON::Document::Release(Options);
				return true;
#else
				return false;
#endif
			}
			bool Collection::RemoveIndex(TCollection* Collection, const char* Name, BSON::TDocument** Options)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Collection)
				{
					BSON::Document::Release(Options);
					return false;
				}

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!mongoc_collection_drop_index_with_opts(Collection, Name, BS(Options), &Error))
				{
					BSON::Document::Release(Options);
					THAWK_ERROR(Error.message);
					return false;
				}

				BSON::Document::Release(Options);
				return true;
#else
				return false;
#endif
			}
			bool Collection::ReplaceOne(TCollection* Collection, BSON::TDocument** Selector, BSON::TDocument** Replacement, BSON::TDocument** Options, BSON::TDocument** Reply)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Collection)
				{
					BSON::Document::Release(Selector);
					BSON::Document::Release(Replacement);
					BSON::Document::Release(Options);
					return false;
				}

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!mongoc_collection_replace_one(Collection, BS(Selector), BS(Replacement), BS(Options), BS(Reply), &Error))
				{
					BSON::Document::Release(Selector);
					BSON::Document::Release(Replacement);
					BSON::Document::Release(Options);
					THAWK_ERROR(Error.message);
					return false;
				}

				BSON::Document::Release(Selector);
				BSON::Document::Release(Replacement);
				BSON::Document::Release(Options);
				return true;
#else
				return false;
#endif
			}
			bool Collection::InsertDocument(TCollection* Collection, InsertMode Flags, BSON::TDocument** Document, TWriteConcern* Concern)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Collection)
				{
					BSON::Document::Release(Document);
					return false;
				}

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!mongoc_collection_insert(Collection, (mongoc_insert_flags_t)Flags, BS(Document), Concern, &Error))
				{
					BSON::Document::Release(Document);
					THAWK_ERROR(Error.message);
					return false;
				}

				BSON::Document::Release(Document);
				return true;
#else
				return false;
#endif
			}
			bool Collection::InsertMany(TCollection* Collection, BSON::TDocument** Documents, uint64_t Length, BSON::TDocument** Options, BSON::TDocument** Reply)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Collection || !Documents || !Length)
				{
					BSON::Document::Release(Options);
					return false;
				}

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!mongoc_collection_insert_many(Collection, (const BSON::TDocument**)Documents, (size_t)Length, BS(Options), BS(Reply), &Error))
				{
					BSON::Document::Release(Options);
					THAWK_ERROR(Error.message);
					return false;
				}

				BSON::Document::Release(Options);
				return true;
#else
				return false;
#endif
			}
			bool Collection::InsertMany(TCollection* Collection, const std::vector<BSON::TDocument*>& Documents, BSON::TDocument** Options, BSON::TDocument** Reply)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Collection || !Documents.size())
				{
					BSON::Document::Release(Options);
					return false;
				}

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!mongoc_collection_insert_many(Collection, (const BSON::TDocument**)Documents.data(), (size_t)Documents.size(), BS(Options), BS(Reply), &Error))
				{
					BSON::Document::Release(Options);
					THAWK_ERROR(Error.message);
					return false;
				}

				BSON::Document::Release(Options);
				return true;
#else
				return false;
#endif
			}
			bool Collection::InsertOne(TCollection* Collection, BSON::TDocument** Document, BSON::TDocument** Options, BSON::TDocument** Reply)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Collection)
				{
					BSON::Document::Release(Options);
					BSON::Document::Release(Document);
					return false;
				}

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!mongoc_collection_insert_one(Collection, BS(Document), BS(Options), BS(Reply), &Error))
				{
					BSON::Document::Release(Options);
					BSON::Document::Release(Document);
					THAWK_ERROR(Error.message);
					return false;
				}

				BSON::Document::Release(Options);
				BSON::Document::Release(Document);
				return true;
#else
				return false;
#endif
			}
			bool Collection::ExecuteCommandWithReply(TCollection* Collection, BSON::TDocument** Command, BSON::TDocument** Reply, TReadPreferences* Preferences)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Collection)
				{
					BSON::Document::Release(Command);
					return false;
				}

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!mongoc_collection_command_simple(Collection, BS(Command), Preferences, BS(Reply), &Error))
				{
					BSON::Document::Release(Command);
					THAWK_ERROR(Error.message);
					return false;
				}

				BSON::Document::Release(Command);
				return true;
#else
				return false;
#endif
			}
			bool Collection::ExecuteCommandWithOptions(TCollection* Collection, BSON::TDocument** Command, BSON::TDocument** Options, BSON::TDocument** Reply, TReadPreferences* Preferences)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Collection)
				{
					BSON::Document::Release(Options);
					BSON::Document::Release(Command);
					return false;
				}

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!mongoc_collection_command_with_opts(Collection, BS(Command), Preferences, BS(Options), BS(Reply), &Error))
				{
					BSON::Document::Release(Options);
					BSON::Document::Release(Command);
					THAWK_ERROR(Error.message);
					return false;
				}

				BSON::Document::Release(Options);
				BSON::Document::Release(Command);
				return true;
#else
				return false;
#endif
			}
			bool Collection::ExecuteReadCommandWithOptions(TCollection* Collection, BSON::TDocument** Command, BSON::TDocument** Options, BSON::TDocument** Reply, TReadPreferences* Preferences)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Collection)
				{
					BSON::Document::Release(Command);
					BSON::Document::Release(Options);
					return false;
				}

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!mongoc_collection_read_command_with_opts(Collection, BS(Command), Preferences, BS(Options), BS(Reply), &Error))
				{
					BSON::Document::Release(Command);
					BSON::Document::Release(Options);
					THAWK_ERROR(Error.message);
					return false;
				}

				BSON::Document::Release(Command);
				BSON::Document::Release(Options);
				return true;
#else
				return false;
#endif
			}
			bool Collection::ExecuteReadWriteCommandWithOptions(TCollection* Collection, BSON::TDocument** Command, BSON::TDocument** Options, BSON::TDocument** Reply)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Collection)
				{
					BSON::Document::Release(Command);
					BSON::Document::Release(Options);
					return false;
				}

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!mongoc_collection_read_write_command_with_opts(Collection, BS(Command), nullptr, BS(Options), BS(Reply), &Error))
				{
					BSON::Document::Release(Command);
					BSON::Document::Release(Options);
					THAWK_ERROR(Error.message);
					return false;
				}

				BSON::Document::Release(Command);
				BSON::Document::Release(Options);
				return true;
#else
				return false;
#endif
			}
			bool Collection::ExecuteWriteCommandWithOptions(TCollection* Collection, BSON::TDocument** Command, BSON::TDocument** Options, BSON::TDocument** Reply)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Collection)
				{
					BSON::Document::Release(Command);
					BSON::Document::Release(Options);
					return false;
				}

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!mongoc_collection_write_command_with_opts(Collection, BS(Command), BS(Options), BS(Reply), &Error))
				{
					BSON::Document::Release(Command);
					BSON::Document::Release(Options);
					THAWK_ERROR(Error.message);
					return false;
				}

				BSON::Document::Release(Command);
				BSON::Document::Release(Options);
				return true;
#else
				return false;
#endif
			}
			bool Collection::FindAndModify(TCollection* Collection, BSON::TDocument** Query, BSON::TDocument** Sort, BSON::TDocument** Update, BSON::TDocument** Fields, BSON::TDocument** Reply, bool RemoveAt, bool Upsert, bool New)
			{
#ifdef THAWK_HAS_MONGOC
				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!Collection)
				{
					BSON::Document::Release(Query);
					BSON::Document::Release(Sort);
					BSON::Document::Release(Update);
					BSON::Document::Release(Fields);
					return false;
				}

				if (!mongoc_collection_find_and_modify(Collection, BS(Query), BS(Sort), BS(Update), BS(Fields), RemoveAt, Upsert, New, BS(Reply), &Error))
				{
					BSON::Document::Release(Query);
					BSON::Document::Release(Sort);
					BSON::Document::Release(Update);
					BSON::Document::Release(Fields);
					THAWK_ERROR(Error.message);
					return false;
				}

				BSON::Document::Release(Query);
				BSON::Document::Release(Sort);
				BSON::Document::Release(Update);
				BSON::Document::Release(Fields);
				return true;
#else
				return false;
#endif
			}
			bool Collection::FindAndModifyWithOptions(TCollection* Collection, BSON::TDocument** Query, BSON::TDocument** Reply, TFindAndModifyOptions** Options)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Collection)
				{
					BSON::Document::Release(Query);
					FindAndModifyOptions::Release(Options);
					return false;
				}

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!mongoc_collection_find_and_modify_with_opts(Collection, BS(Query), BS(Options), BS(Reply), &Error))
				{
					BSON::Document::Release(Query);
					FindAndModifyOptions::Release(Options);
					THAWK_ERROR(Error.message);
					return false;
				}

				BSON::Document::Release(Query);
				FindAndModifyOptions::Release(Options);
				return true;
#else
				return false;
#endif
			}
			uint64_t Collection::CountElementsInArray(TCollection* Collection, BSON::TDocument** Match, BSON::TDocument** Filter, BSON::TDocument** Options, TReadPreferences* Preferences)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Collection || !Filter || !*Filter || !Match || !*Match)
				{
					BSON::Document::Release(Filter);
					BSON::Document::Release(Match);
					BSON::Document::Release(Options);
					return 0;
				}

				BSON::TDocument* Pipeline = BCON_NEW("pipeline", "[", "{", "$match", BCON_DOCUMENT(*Match), "}", "{", "$project", "{", "Count", "{", "$Count", "{", "$filter", BCON_DOCUMENT(*Filter), "}", "}", "}", "}", "]");
				if (!Pipeline)
				{
					BSON::Document::Release(Filter);
					BSON::Document::Release(Match);
					BSON::Document::Release(Options);
					return 0;
				}

				TCursor* Cursor = mongoc_collection_aggregate(Collection, MONGOC_QUERY_NONE, Pipeline, BS(Options), Preferences);
				BSON::Document::Release(&Pipeline);
				BSON::Document::Release(Filter);
				BSON::Document::Release(Match);
				BSON::Document::Release(Options);

				if (!Cursor || !Cursor::Next(Cursor))
				{
					Cursor::Release(&Cursor);
					return 0;
				}

				BSON::TDocument* Result = Cursor::GetCurrent(Cursor);
				if (!Result)
				{
					Cursor::Release(&Cursor);
					return 0;
				}

				BSON::KeyPair Count;
				BSON::Document::GetKey(Result, "Count", &Count);
				Cursor::Release(&Cursor);

				return (uint64_t)Count.Integer;
#else
				return 0;
#endif
			}
			uint64_t Collection::CountDocuments(TCollection* Collection, BSON::TDocument** Filter, BSON::TDocument** Options, BSON::TDocument** Reply, TReadPreferences* Preferences)
			{
#ifdef THAWK_HAS_MONGOC
				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!Collection)
				{
					BSON::Document::Release(Filter);
					BSON::Document::Release(Options);
					return 0;
				}

				uint64_t Count = (uint64_t)mongoc_collection_count_documents(Collection, BS(Filter), BS(Options), Preferences, BS(Reply), &Error);
				if (Error.code != 0)
				{
					BSON::Document::Release(Filter);
					BSON::Document::Release(Options);
					THAWK_ERROR(Error.message);
					return false;
				}

				BSON::Document::Release(Filter);
				BSON::Document::Release(Options);
				return Count;
#else
				return 0;
#endif
			}
			uint64_t Collection::CountDocumentsEstimated(TCollection* Collection, BSON::TDocument** Options, BSON::TDocument** Reply, TReadPreferences* Preferences)
			{
#ifdef THAWK_HAS_MONGOC
				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!Collection)
				{
					BSON::Document::Release(Options);
					return 0;
				}

				uint64_t Count = (uint64_t)mongoc_collection_estimated_document_count(Collection, BS(Options), Preferences, BS(Reply), &Error);
				if (Error.code != 0)
				{
					BSON::Document::Release(Options);
					THAWK_ERROR(Error.message);
					return false;
				}

				BSON::Document::Release(Options);
				return Count;
#else
				return 0;
#endif
			}
			std::string Collection::StringifyKeyIndexes(TCollection* Collection, BSON::TDocument** Keys)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Collection || !*Keys)
					return std::string();

				char* Value = mongoc_collection_keys_to_index_string(*Keys);
				std::string Output = Value;
				bson_free(Value);

				BSON::Document::Release(Keys);
				return Output;
#else
				return "";
#endif
			}
			const char* Collection::GetName(TCollection* Collection)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Collection)
					return nullptr;

				return mongoc_collection_get_name(Collection);
#else
				return nullptr;
#endif
			}
			TReadConcern* Collection::GetReadConcern(TCollection* Collection)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Collection)
					return nullptr;

				return (TReadConcern*)mongoc_collection_get_read_concern(Collection);
#else
				return nullptr;
#endif
			}
			TReadPreferences* Collection::GetReadPreferences(TCollection* Collection)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Collection)
					return nullptr;

				return (TReadPreferences*)mongoc_collection_get_read_prefs(Collection);
#else
				return nullptr;
#endif
			}
			TWriteConcern* Collection::GetWriteConcern(TCollection* Collection)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Collection)
					return nullptr;

				return (TWriteConcern*)mongoc_collection_get_write_concern(Collection);
#else
				return nullptr;
#endif
			}
			TCursor* Collection::FindIndexes(TCollection* Collection, BSON::TDocument** Options)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Collection)
				{
					BSON::Document::Release(Options);
					return nullptr;
				}

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				TCursor* Cursor = mongoc_collection_find_indexes_with_opts(Collection, BS(Options));
				if (Cursor == nullptr || Error.code != 0)
				{
					BSON::Document::Release(Options);
					THAWK_ERROR("couldn't fetch the cursor -> %s", Error.message);
					return nullptr;
				}

				BSON::Document::Release(Options);
				return Cursor;
#else
				return nullptr;
#endif
			}
			TCursor* Collection::FindMany(TCollection* Collection, BSON::TDocument** Filter, BSON::TDocument** Options, TReadPreferences* Preferences)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Collection)
				{
					BSON::Document::Release(Options);
					return nullptr;
				}

				TCursor* Result = mongoc_collection_find_with_opts(Collection, BS(Filter), BS(Options), Preferences);
				BSON::Document::Release(Filter);
				BSON::Document::Release(Options);

				return Result;
#else
				return nullptr;
#endif
			}
			TCursor* Collection::FindOne(TCollection* Collection, BSON::TDocument** Filter, BSON::TDocument** Options, TReadPreferences* Preferences)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Collection)
				{
					BSON::Document::Release(Filter);
					BSON::Document::Release(Options);
					return nullptr;
				}

				BSON::TDocument* Settings = (Options ? *Options : nullptr);
				if (Settings != nullptr)
					BSON::Document::AddKeyInteger(Settings, "limit", 1);
				else
					Settings = BCON_NEW("limit", BCON_INT32(1));


				TCursor* Cursor = mongoc_collection_find_with_opts(Collection, BS(Filter), Settings, Preferences);
				if (!Options && Settings)
					bson_destroy(Settings);

				BSON::Document::Release(Filter);
				BSON::Document::Release(Options);
				return Cursor;
#else
				return nullptr;
#endif
			}
			TCursor* Collection::Aggregate(TCollection* Collection, QueryMode Flags, BSON::TDocument** Pipeline, BSON::TDocument** Options, TReadPreferences* Preferences)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Collection)
				{
					BSON::Document::Release(Pipeline);
					BSON::Document::Release(Options);
					return nullptr;
				}

				TCursor* Result = mongoc_collection_aggregate(Collection, (mongoc_query_flags_t)Flags, BS(Pipeline), BS(Options), Preferences);

				BSON::Document::Release(Pipeline);
				BSON::Document::Release(Options);

				return Result;
#else
				return nullptr;
#endif
			}
			TCursor* Collection::ExecuteCommand(TCollection* Collection, QueryMode Flags, uint64_t Skip, uint64_t Limit, uint64_t BatchSize, BSON::TDocument** Command, BSON::TDocument** Fields, TReadPreferences* Preferences)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Collection)
				{
					BSON::Document::Release(Command);
					BSON::Document::Release(Fields);
					return nullptr;
				}

				TCursor* Result = mongoc_collection_command(Collection, (mongoc_query_flags_t)Flags, (uint32_t)Skip, (uint32_t)Limit, (uint32_t)BatchSize, BS(Command), BS(Fields), Preferences);

				BSON::Document::Release(Command);
				BSON::Document::Release(Fields);

				return Result;
#else
				return nullptr;
#endif
			}
			TBulkOperation* Collection::CreateBulkOperation(TCollection* Collection, BSON::TDocument** Options)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Collection)
				{
					BSON::Document::Release(Options);
					return nullptr;
				}

				TBulkOperation* Operation = mongoc_collection_create_bulk_operation_with_opts(Collection, BS(Options));
				BSON::Document::Release(Options);

				return Operation;
#else
				return nullptr;
#endif
			}

			void Database::Release(TDatabase** Database)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Database || !*Database)
					return;

				mongoc_database_destroy(*Database);
				*Database = nullptr;
#endif
			}
			void Database::SetReadConcern(TDatabase* Database, TReadConcern* Concern)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Database || !Concern)
					return;

				mongoc_database_set_read_concern(Database, Concern);
#endif
			}
			void Database::SetWriteConcern(TDatabase* Database, TWriteConcern* Concern)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Database || !Concern)
					return;

				mongoc_database_set_write_concern(Database, Concern);
#endif
			}
			void Database::SetReadPreferences(TDatabase* Database, TReadPreferences* Preferences)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Database || !Preferences)
					return;

				mongoc_database_set_read_prefs(Database, Preferences);
#endif
			}
			bool Database::HasCollection(TDatabase* Database, const char* Name)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Database)
					return false;

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!mongoc_database_has_collection(Database, Name, &Error))
				{
					THAWK_ERROR(Error.message);
					return false;
				}

				return true;
#else
				return false;
#endif
			}
			bool Database::RemoveAllUsers(TDatabase* Database)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Database)
					return false;

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!mongoc_database_remove_all_users(Database, &Error))
				{
					THAWK_ERROR(Error.message);
					return false;
				}

				return true;
#else
				return false;
#endif
			}
			bool Database::RemoveUser(TDatabase* Database, const char* Name)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Database)
					return false;

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!mongoc_database_remove_user(Database, Name, &Error))
				{
					THAWK_ERROR(Error.message);
					return false;
				}

				return true;
#else
				return false;
#endif
			}
			bool Database::Remove(TDatabase* Database)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Database)
					return false;

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!mongoc_database_drop(Database, &Error))
				{
					THAWK_ERROR(Error.message);
					return false;
				}

				return true;
#else
				return false;
#endif
			}
			bool Database::RemoveWithOptions(TDatabase* Database, BSON::TDocument** Options)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Database)
				{
					BSON::Document::Release(Options);
					return false;
				}

				if (!Options)
					return Remove(Database);

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!mongoc_database_drop_with_opts(Database, BS(Options), &Error))
				{
					BSON::Document::Release(Options);
					THAWK_ERROR(Error.message);
					return false;
				}

				BSON::Document::Release(Options);
				return true;
#else
				return false;
#endif
			}
			bool Database::AddUser(TDatabase* Database, const char* Username, const char* Password, BSON::TDocument** Roles, BSON::TDocument** Custom)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Database)
				{
					BSON::Document::Release(Roles);
					BSON::Document::Release(Custom);
					return false;
				}

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!mongoc_database_add_user(Database, Username, Password, BS(Roles), BS(Custom), &Error))
				{
					BSON::Document::Release(Roles);
					BSON::Document::Release(Custom);
					THAWK_ERROR(Error.message);
					return false;
				}

				BSON::Document::Release(Roles);
				BSON::Document::Release(Custom);
				return true;
#else
				return false;
#endif
			}
			bool Database::ExecuteCommandWithReply(TDatabase* Database, BSON::TDocument** Command, BSON::TDocument** Reply, TReadPreferences* Preferences)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Database)
				{
					BSON::Document::Release(Command);
					return false;
				}

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!mongoc_database_command_simple(Database, BS(Command), Preferences, BS(Reply), &Error))
				{
					BSON::Document::Release(Command);
					THAWK_ERROR(Error.message);
					return false;
				}

				BSON::Document::Release(Command);
				return true;
#else
				return false;
#endif
			}
			bool Database::ExecuteCommandWithOptions(TDatabase* Database, BSON::TDocument** Command, BSON::TDocument** Options, BSON::TDocument** Reply, TReadPreferences* Preferences)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Database)
				{
					BSON::Document::Release(Command);
					BSON::Document::Release(Options);
					return false;
				}

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!mongoc_database_command_with_opts(Database, BS(Command), Preferences, BS(Options), BS(Reply), &Error))
				{
					BSON::Document::Release(Command);
					BSON::Document::Release(Options);
					THAWK_ERROR(Error.message);
					return false;
				}

				BSON::Document::Release(Command);
				BSON::Document::Release(Options);
				return true;
#else
				return false;
#endif
			}
			bool Database::ExecuteReadCommandWithOptions(TDatabase* Database, BSON::TDocument** Command, BSON::TDocument** Options, BSON::TDocument** Reply, TReadPreferences* Preferences)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Database)
				{
					BSON::Document::Release(Command);
					BSON::Document::Release(Options);
					return false;
				}

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!mongoc_database_read_command_with_opts(Database, BS(Command), Preferences, BS(Options), BS(Reply), &Error))
				{
					BSON::Document::Release(Command);
					BSON::Document::Release(Options);
					THAWK_ERROR(Error.message);
					return false;
				}

				BSON::Document::Release(Command);
				BSON::Document::Release(Options);
				return true;
#else
				return false;
#endif
			}
			bool Database::ExecuteReadWriteCommandWithOptions(TDatabase* Database, BSON::TDocument** Command, BSON::TDocument** Options, BSON::TDocument** Reply)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Database)
				{
					BSON::Document::Release(Command);
					BSON::Document::Release(Options);
					return false;
				}

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!mongoc_database_read_write_command_with_opts(Database, BS(Command), nullptr, BS(Options), BS(Reply), &Error))
				{
					BSON::Document::Release(Command);
					BSON::Document::Release(Options);
					THAWK_ERROR(Error.message);
					return false;
				}

				BSON::Document::Release(Command);
				BSON::Document::Release(Options);
				return true;
#else
				return false;
#endif
			}
			bool Database::ExecuteWriteCommandWithOptions(TDatabase* Database, BSON::TDocument** Command, BSON::TDocument** Options, BSON::TDocument** Reply)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Database)
				{
					BSON::Document::Release(Command);
					BSON::Document::Release(Options);
					return false;
				}

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!mongoc_database_write_command_with_opts(Database, BS(Command), BS(Options), BS(Reply), &Error))
				{
					BSON::Document::Release(Command);
					BSON::Document::Release(Options);
					THAWK_ERROR(Error.message);
					return false;
				}

				BSON::Document::Release(Command);
				BSON::Document::Release(Options);
				return true;
#else
				return false;
#endif
			}
			std::vector<std::string> Database::GetCollectionNames(TDatabase* Database, BSON::TDocument** Options)
			{
#ifdef THAWK_HAS_MONGOC
				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!Database)
					return std::vector<std::string>();

				char** Names = mongoc_database_get_collection_names_with_opts(Database, BS(Options), &Error);
				BSON::Document::Release(Options);

				if (Names == nullptr)
				{
					THAWK_ERROR(Error.message);
					return std::vector<std::string>();
				}

				std::vector<std::string> Output;
				for (unsigned i = 0; Names[i]; i++)
					Output.push_back(Names[i]);

				bson_strfreev(Names);
				return Output;
#else
				return std::vector<std::string>();
#endif
			}
			const char* Database::GetName(TDatabase* Database)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Database)
					return nullptr;

				return mongoc_database_get_name(Database);
#else
				return nullptr;
#endif
			}
			TCursor* Database::ExecuteCommand(TDatabase* Database, QueryMode Flags, uint64_t Skip, uint64_t Limit, uint64_t BatchSize, BSON::TDocument** Command, BSON::TDocument** Fields, TReadPreferences* Preferences)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Database)
				{
					BSON::Document::Release(Command);
					BSON::Document::Release(Fields);
					return nullptr;
				}


				TCursor* Result = mongoc_database_command(Database, (mongoc_query_flags_t)Flags, (uint32_t)Skip, (uint32_t)Limit, (uint32_t)BatchSize, BS(Command), BS(Fields), Preferences);

				BSON::Document::Release(Command);
				BSON::Document::Release(Fields);

				return Result;
#else
				return nullptr;
#endif
			}
			TCursor* Database::FindCollections(TDatabase* Database, BSON::TDocument** Options)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Database)
				{
					BSON::Document::Release(Options);
					return nullptr;
				}

				TCursor* Cursor = mongoc_database_find_collections_with_opts(Database, BS(Options));
				BSON::Document::Release(Options);

				return Cursor;
#else
				return nullptr;
#endif
			}
			TCollection* Database::CreateCollection(TDatabase* Database, const char* Name, BSON::TDocument** Options)
			{
#ifdef THAWK_HAS_MONGOC
				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!Database)
				{
					BSON::Document::Release(Options);
					return nullptr;
				}

				TCollection* Collection = mongoc_database_create_collection(Database, Name, BS(Options), &Error);
				BSON::Document::Release(Options);

				if (Collection == nullptr)
					THAWK_ERROR(Error.message);

				return Collection;
#else
				return nullptr;
#endif
			}
			TCollection* Database::GetCollection(TDatabase* Database, const char* Name)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Database)
					return nullptr;

				return mongoc_database_get_collection(Database, Name);
#else
				return nullptr;
#endif
			}
			TReadConcern* Database::GetReadConcern(TDatabase* Database)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Database)
					return nullptr;

				return (TReadConcern*)mongoc_database_get_read_concern(Database);
#else
				return nullptr;
#endif
			}
			TReadPreferences* Database::GetReadPreferences(TDatabase* Database)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Database)
					return nullptr;

				return (TReadPreferences*)mongoc_database_get_read_prefs(Database);
#else
				return nullptr;
#endif
			}
			TWriteConcern* Database::GetWriteConcern(TDatabase* Database)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Database)
					return nullptr;

				return (TWriteConcern*)mongoc_database_get_write_concern(Database);
#else
				return nullptr;
#endif
			}

			void ServerDescription::Release(TServerDescription** Description)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Description || !*Description)
					return;

				mongoc_server_description_destroy(*Description);
				*Description = nullptr;
#endif
			}
			uint64_t ServerDescription::GetId(TServerDescription* Description)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Description)
					return 0;

				return mongoc_server_description_id(Description);
#else
				return 0;
#endif
			}
			int64_t ServerDescription::GetTripTime(TServerDescription* Description)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Description)
					return 0;

				return mongoc_server_description_round_trip_time(Description);
#else
				return 0;
#endif
			}
			const char* ServerDescription::GetType(TServerDescription* Description)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Description)
					return nullptr;

				return mongoc_server_description_type(Description);
#else
				return nullptr;
#endif
			}
			BSON::TDocument* ServerDescription::IsMaster(TServerDescription* Description)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Description)
					return nullptr;

				return (BSON::TDocument*)mongoc_server_description_ismaster(Description);
#else
				return nullptr;
#endif
			}
			HostList* ServerDescription::GetHosts(TServerDescription* Description)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Description)
					return nullptr;

				return (HostList*)mongoc_server_description_host(Description);
#else
				return nullptr;
#endif
			}

			bool TopologyDescription::HasReadableServer(TTopologyDescription* Description, TReadPreferences* Preferences)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Description)
					return false;

				return mongoc_topology_description_has_readable_server(Description, Preferences);
#else
				return false;
#endif
			}
			bool TopologyDescription::HasWritableServer(TTopologyDescription* Description)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Description)
					return false;

				return mongoc_topology_description_has_writable_server(Description);
#else
				return false;
#endif
			}
			std::vector<TServerDescription*> TopologyDescription::GetServers(TTopologyDescription* Description)
			{
#ifdef THAWK_HAS_MONGOC
				size_t Length = 0;
				if (!Description)
					return std::vector<TServerDescription*>();

				TServerDescription** Values = mongoc_topology_description_get_servers(Description, &Length);
				if (!Values || !Length)
				{
					if (Values != nullptr)
						bson_free(Values);

					return std::vector<TServerDescription*>();
				}

				std::vector<TServerDescription*> Servers;
				for (uint64_t i = 0; i < Length; i++)
				{
					Servers.push_back(Values[i]);
					Values[i] = nullptr;
				}

				bson_free(Values);
				return Servers;
#else
				return std::vector<TServerDescription*>();
#endif
			}
			const char* TopologyDescription::GetType(TTopologyDescription* Description)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Description)
					return nullptr;

				return mongoc_topology_description_type(Description);
#else
				return nullptr;
#endif
			}

			Client::Client()
			{
				Connector::Create();
			}
			Client::~Client()
			{
				Disconnect();
				Connector::Release();
			}
			bool Client::Connect(const char* Address)
			{
#ifdef THAWK_HAS_MONGOC
				if (Master != nullptr)
					return false;

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (Connected)
					Disconnect();

				TURI* URI = mongoc_uri_new_with_error(Address, &Error);
				if (URI == nullptr)
				{
					THAWK_ERROR("couldn't parse URI -> %s", Error.message);
					return false;
				}

				Connection = mongoc_client_new_from_uri(URI);
				if (Connection == nullptr)
				{
					THAWK_ERROR("couldn't connect to requested URI");
					return false;
				}

				Connected = true;
				return true;
#else
				return false;
#endif
			}
			bool Client::ConnectWithURI(TURI* URI)
			{
#ifdef THAWK_HAS_MONGOC
				if (Master != nullptr)
					return false;

				if (!URI)
					return false;

				if (Connected)
					Disconnect();

				Connection = mongoc_client_new_from_uri(URI);
				if (Connection == nullptr)
				{
					THAWK_ERROR("couldn't connect to requested URI");
					return false;
				}

				Connected = true;
				return true;
#else
				return false;
#endif
			}
			bool Client::Disconnect()
			{
#ifdef THAWK_HAS_MONGOC
				Connected = false;
				if (Master != nullptr)
				{
					if (Connection != nullptr)
					{
						mongoc_client_pool_push(Master->Get(), Connection);
						Connection = nullptr;
					}

					Master = nullptr;
				}
				else if (Connection != nullptr)
				{
					mongoc_client_destroy(Connection);
					Connection = nullptr;
				}

				return true;
#else
				return false;
#endif
			}
			bool Client::SelectServer(bool ForWrites)
			{
#ifdef THAWK_HAS_MONGOC
				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				TServerDescription* Server = mongoc_client_select_server(Connection, ForWrites, nullptr, &Error);
				if (Server == nullptr)
				{
					THAWK_ERROR("command fail: %s", Error.message);
					return false;
				}

				mongoc_server_description_destroy(Server);
				return true;
#else
				return false;
#endif
			}
			void Client::SetAppName(const char* Name)
			{
#ifdef THAWK_HAS_MONGOC
				mongoc_client_set_appname(Connection, Name);
#endif
			}
			void Client::SetReadConcern(TReadConcern* Concern)
			{
#ifdef THAWK_HAS_MONGOC
				mongoc_client_set_read_concern(Connection, Concern);
#endif
			}
			void Client::SetReadPreferences(TReadPreferences* Preferences)
			{
#ifdef THAWK_HAS_MONGOC
				mongoc_client_set_read_prefs(Connection, Preferences);
#endif
			}
			void Client::SetWriteConcern(TWriteConcern* Concern)
			{
#ifdef THAWK_HAS_MONGOC
				mongoc_client_set_write_concern(Connection, Concern);
#endif
			}
			void Client::SetSSLOptions(const SSLOptions& Options)
			{
#ifdef THAWK_HAS_MONGOC
				mongoc_client_set_ssl_opts(Connection, (const mongoc_ssl_opt_t*)&Options);
#endif
			}
			void Client::SetAPMCallbacks(const APMCallbacks& Options)
			{
#ifdef THAWK_HAS_MONGOC
				mongoc_apm_callbacks_t* Callbacks = mongoc_apm_callbacks_new();
				if (Options.OnCommandStarted != nullptr)
				{
					mongoc_apm_set_command_started_cb(Callbacks, [](const mongoc_apm_command_started_t* Event) -> void
					{
						Client* Client = (MongoDB::Client*)mongoc_apm_command_started_get_context(Event);
						if (!Client || !Client->ApmCallbacks.OnCommandStarted)
							return;

						APMCommandStarted Command;
						Command.OperationId = mongoc_apm_command_started_get_operation_id(Event);
						Command.RequestId = mongoc_apm_command_started_get_request_id(Event);
						Command.ServerId = mongoc_apm_command_started_get_server_id(Event);
						Command.Hosts = (HostList*)mongoc_apm_command_started_get_host(Event);
						Command.Command = (BSON::TDocument*)mongoc_apm_command_started_get_command(Event);
						Command.CommandName = mongoc_apm_command_started_get_command_name(Event);
						Command.DatabaseName = mongoc_apm_command_started_get_database_name(Event);
						Command.Event = (void*)Event;

						Client->ApmCallbacks.OnCommandStarted(Client, &Command);
					});
				}
				if (Options.OnCommandSucceeded != nullptr)
				{
					mongoc_apm_set_command_succeeded_cb(Callbacks, [](const mongoc_apm_command_succeeded_t* Event) -> void
					{
						Client* Client = (MongoDB::Client*)mongoc_apm_command_succeeded_get_context(Event);
						if (!Client || !Client->ApmCallbacks.OnCommandSucceeded)
							return;

						APMCommandSucceeded Command;
						Command.Duration = mongoc_apm_command_succeeded_get_duration(Event);
						Command.OperationId = mongoc_apm_command_succeeded_get_operation_id(Event);
						Command.RequestId = mongoc_apm_command_succeeded_get_request_id(Event);
						Command.ServerId = mongoc_apm_command_succeeded_get_server_id(Event);
						Command.Hosts = (HostList*)mongoc_apm_command_succeeded_get_host(Event);
						Command.Reply = (BSON::TDocument*)mongoc_apm_command_succeeded_get_reply(Event);
						Command.CommandName = mongoc_apm_command_succeeded_get_command_name((mongoc_apm_command_succeeded_t*)Event);
						Command.Event = (void*)Event;

						Client->ApmCallbacks.OnCommandSucceeded(Client, &Command);
					});
				}
				if (Options.OnCommandFailed != nullptr)
				{
					mongoc_apm_set_command_failed_cb(Callbacks, [](const mongoc_apm_command_failed_t* Event) -> void
					{
						Client* Client = (MongoDB::Client*)mongoc_apm_command_failed_get_context(Event);
						if (!Client || !Client->ApmCallbacks.OnCommandFailed)
							return;

						APMCommandFailed Command;
						Command.Duration = mongoc_apm_command_failed_get_duration(Event);
						Command.OperationId = mongoc_apm_command_failed_get_operation_id(Event);
						Command.RequestId = mongoc_apm_command_failed_get_request_id(Event);
						Command.ServerId = mongoc_apm_command_failed_get_server_id(Event);
						Command.Hosts = (HostList*)mongoc_apm_command_failed_get_host(Event);
						Command.Reply = (BSON::TDocument*)mongoc_apm_command_failed_get_reply(Event);
						Command.CommandName = mongoc_apm_command_failed_get_command_name(Event);
						Command.Event = (void*)Event;

						Client->ApmCallbacks.OnCommandFailed(Client, &Command);
					});
				}
				if (Options.OnServerChanged != nullptr)
				{
					mongoc_apm_set_server_changed_cb(Callbacks, [](const mongoc_apm_server_changed_t* Event) -> void
					{
						Client* Client = (MongoDB::Client*)mongoc_apm_server_changed_get_context(Event);
						if (!Client || !Client->ApmCallbacks.OnServerChanged)
							return;

						APMServerChanged Command;
						Command.Hosts = (HostList*)mongoc_apm_server_changed_get_host(Event);
						Command.New = (TServerDescription*)mongoc_apm_server_changed_get_new_description(Event);
						Command.Previous = (TServerDescription*)mongoc_apm_server_changed_get_previous_description(Event);
						Command.Event = (void*)Event;

						Client->ApmCallbacks.OnServerChanged(Client, &Command);
					});
				}
				if (Options.OnServerOpened != nullptr)
				{
					mongoc_apm_set_server_opening_cb(Callbacks, [](const mongoc_apm_server_opening_t* Event) -> void
					{
						Client* Client = (MongoDB::Client*)mongoc_apm_server_opening_get_context(Event);
						if (!Client || !Client->ApmCallbacks.OnServerOpened)
							return;

						APMServerOpened Command;
						Command.Hosts = (HostList*)mongoc_apm_server_opening_get_host(Event);
						Command.Event = (void*)Event;

						Client->ApmCallbacks.OnServerOpened(Client, &Command);
					});
				}
				if (Options.OnServerClosed != nullptr)
				{
					mongoc_apm_set_server_closed_cb(Callbacks, [](const mongoc_apm_server_closed_t* Event) -> void
					{
						Client* Client = (MongoDB::Client*)mongoc_apm_server_closed_get_context(Event);
						if (!Client || !Client->ApmCallbacks.OnServerClosed)
							return;

						APMServerClosed Command;
						Command.Hosts = (HostList*)mongoc_apm_server_closed_get_host(Event);
						Command.Event = (void*)Event;

						Client->ApmCallbacks.OnServerClosed(Client, &Command);
					});
				}
				if (Options.OnServerHeartbeatFailed != nullptr)
				{
					mongoc_apm_set_server_heartbeat_failed_cb(Callbacks, [](const mongoc_apm_server_heartbeat_failed_t* Event) -> void
					{
						Client* Client = (MongoDB::Client*)mongoc_apm_server_heartbeat_failed_get_context(Event);
						if (!Client || !Client->ApmCallbacks.OnServerHeartbeatFailed)
							return;

						bson_error_t Error;
						memset(&Error, 0, sizeof(bson_error_t));

						mongoc_apm_server_heartbeat_failed_get_error(Event, &Error);

						APMServerHeartbeatFailed Command;
						Command.Duration = mongoc_apm_server_heartbeat_failed_get_duration(Event);
						Command.Message = Error.message;
						Command.ErrorCode = Error.code;
						Command.Domain = Error.domain;
						Command.Hosts = (HostList*)mongoc_apm_server_heartbeat_failed_get_host(Event);
						Command.Event = (void*)Event;

						Client->ApmCallbacks.OnServerHeartbeatFailed(Client, &Command);
					});
				}
				if (Options.OnServerHeartbeatStarted != nullptr)
				{
					mongoc_apm_set_server_heartbeat_started_cb(Callbacks, [](const mongoc_apm_server_heartbeat_started_t* Event) -> void
					{
						Client* Client = (MongoDB::Client*)mongoc_apm_server_heartbeat_started_get_context(Event);
						if (!Client || !Client->ApmCallbacks.OnServerHeartbeatStarted)
							return;

						APMServerHeartbeatStarted Command;
						Command.Hosts = (HostList*)mongoc_apm_server_heartbeat_started_get_host(Event);
						Command.Event = (void*)Event;

						Client->ApmCallbacks.OnServerHeartbeatStarted(Client, &Command);
					});
				}
				if (Options.OnServerHeartbeatSucceeded != nullptr)
				{
					mongoc_apm_set_server_heartbeat_succeeded_cb(Callbacks, [](const mongoc_apm_server_heartbeat_succeeded_t* Event) -> void
					{
						Client* Client = (MongoDB::Client*)mongoc_apm_server_heartbeat_succeeded_get_context(Event);
						if (!Client || !Client->ApmCallbacks.OnServerHeartbeatSucceeded)
							return;

						APMServerHeartbeatSucceeded Command;
						Command.Duration = mongoc_apm_server_heartbeat_succeeded_get_duration(Event);
						Command.Hosts = (HostList*)mongoc_apm_server_heartbeat_succeeded_get_host(Event);
						Command.Reply = (BSON::TDocument*)mongoc_apm_server_heartbeat_succeeded_get_reply(Event);
						Command.Event = (void*)Event;

						Client->ApmCallbacks.OnServerHeartbeatSucceeded(Client, &Command);
					});
				}
				if (Options.OnTopologyChanged != nullptr)
				{
					mongoc_apm_set_topology_changed_cb(Callbacks, [](const mongoc_apm_topology_changed_t* Event) -> void
					{
						Client* Client = (MongoDB::Client*)mongoc_apm_topology_changed_get_context(Event);
						if (!Client || !Client->ApmCallbacks.OnTopologyChanged)
							return;

						APMTopologyChanged Command;
						Command.Event = (void*)Event;
						Command.New = (TTopologyDescription*)mongoc_apm_topology_changed_get_new_description(Event);
						Command.Previous = (TTopologyDescription*)mongoc_apm_topology_changed_get_previous_description(Event);

						Client->ApmCallbacks.OnTopologyChanged(Client, &Command);
					});
				}
				if (Options.OnTopologyClosed != nullptr)
				{
					mongoc_apm_set_topology_closed_cb(Callbacks, [](const mongoc_apm_topology_closed_t* Event) -> void
					{
						Client* Client = (MongoDB::Client*)mongoc_apm_topology_closed_get_context(Event);
						if (!Client || !Client->ApmCallbacks.OnTopologyClosed)
							return;

						APMTopologyClosed Command;
						Command.Event = (void*)Event;

						Client->ApmCallbacks.OnTopologyClosed(Client, &Command);
					});
				}
				if (Options.OnTopologyOpened != nullptr)
				{
					mongoc_apm_set_topology_opening_cb(Callbacks, [](const mongoc_apm_topology_opening_t* Event) -> void
					{
						Client* Client = (MongoDB::Client*)mongoc_apm_topology_opening_get_context(Event);
						if (!Client || !Client->ApmCallbacks.OnTopologyOpened)
							return;

						APMTopologyOpened Command;
						Command.Event = (void*)Event;

						Client->ApmCallbacks.OnTopologyOpened(Client, &Command);
					});
				}

				memcpy(&ApmCallbacks, &Options, sizeof(APMCallbacks));
				mongoc_client_set_apm_callbacks(Connection, Callbacks, this);
				mongoc_apm_callbacks_destroy(Callbacks);
#endif
			}
			TCursor* Client::ExecuteCommand(const char* DatabaseName, BSON::TDocument** Query, TReadPreferences* Preferences)
			{
#ifdef THAWK_HAS_MONGOC
				if (!DatabaseName || !Query)
				{
					BSON::Document::Release(Query);
					return nullptr;
				}

				TCursor* Reference = mongoc_client_command(Connection, DatabaseName, MONGOC_QUERY_NONE, 0, 0, 0, BS(Query), nullptr, Preferences);
				BSON::Document::Release(Query);

				return Reference;
#else
				return nullptr;
#endif
			}
			BSON::TDocument* Client::ExecuteCommandResult(const char* DatabaseName, BSON::TDocument** Query, TReadPreferences* Preferences)
			{
#ifdef THAWK_HAS_MONGOC
				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!DatabaseName || !Query)
				{
					BSON::Document::Release(Query);
					return nullptr;
				}

				BSON::TDocument* Reference = nullptr;
				if (!mongoc_client_command_simple(Connection, DatabaseName, BS(Query), Preferences, Reference, &Error))
				{
					BSON::Document::Release(Query);
					THAWK_ERROR(Error.message);
					return nullptr;
				}

				BSON::Document::Release(Query);
				return Reference;
#else
				return nullptr;
#endif
			}
			BSON::TDocument* Client::ExecuteCommandResult(const char* DatabaseName, BSON::TDocument** Query, BSON::TDocument** Options, TReadPreferences* Preferences)
			{
#ifdef THAWK_HAS_MONGOC
				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!DatabaseName || !Query)
				{
					BSON::Document::Release(Query);
					BSON::Document::Release(Options);
					return nullptr;
				}

				BSON::TDocument* Reference = nullptr;
				if (!mongoc_client_command_with_opts(Connection, DatabaseName, BS(Query), Preferences, BS(Options), Reference, &Error))
				{
					BSON::Document::Release(Query);
					BSON::Document::Release(Options);
					THAWK_ERROR(Error.message);
					return nullptr;
				}

				BSON::Document::Release(Query);
				BSON::Document::Release(Options);
				return Reference;
#else
				return nullptr;
#endif
			}
			BSON::TDocument* Client::ExecuteReadCommandResult(const char* DatabaseName, BSON::TDocument** Query, BSON::TDocument** Options, TReadPreferences* Preferences)
			{
#ifdef THAWK_HAS_MONGOC
				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!DatabaseName || !Query)
				{
					BSON::Document::Release(Query);
					BSON::Document::Release(Options);
					return nullptr;
				}

				BSON::TDocument* Reference = nullptr;
				if (!mongoc_client_read_command_with_opts(Connection, DatabaseName, BS(Query), Preferences, BS(Options), Reference, &Error))
				{
					BSON::Document::Release(Query);
					BSON::Document::Release(Options);
					THAWK_ERROR(Error.message);
					return nullptr;
				}

				BSON::Document::Release(Query);
				BSON::Document::Release(Options);
				return Reference;
#else
				return nullptr;
#endif
			}
			BSON::TDocument* Client::ExecuteWriteCommandResult(const char* DatabaseName, BSON::TDocument** Query, BSON::TDocument** Options)
			{
#ifdef THAWK_HAS_MONGOC
				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!DatabaseName || !Query)
				{
					BSON::Document::Release(Query);
					BSON::Document::Release(Options);
					return nullptr;
				}

				BSON::TDocument* Reference = nullptr;
				if (!mongoc_client_write_command_with_opts(Connection, DatabaseName, BS(Query), BS(Options), Reference, &Error))
				{
					BSON::Document::Release(Query);
					BSON::Document::Release(Options);
					THAWK_ERROR(Error.message);
					return nullptr;
				}

				BSON::Document::Release(Query);
				BSON::Document::Release(Options);
				return Reference;
#else
				return nullptr;
#endif
			}
			TCursor* Client::FindDatabases(BSON::TDocument** Options)
			{
#ifdef THAWK_HAS_MONGOC
				TCursor* Reference = mongoc_client_find_databases_with_opts(Connection, BS(Options));
				BSON::Document::Release(Options);

				return Reference;
#else
				return nullptr;
#endif
			}
			TServerDescription* Client::GetServerDescription(unsigned int ServerId)
			{
#ifdef THAWK_HAS_MONGOC
				return mongoc_client_get_server_description(Connection, ServerId);
#else
				return nullptr;
#endif
			}
			TDatabase* Client::GetDatabase(const char* Name)
			{
#ifdef THAWK_HAS_MONGOC
				return mongoc_client_get_database(Connection, Name);
#else
				return nullptr;
#endif
			}
			TDatabase* Client::GetDefaultDatabase()
			{
#ifdef THAWK_HAS_MONGOC
				return mongoc_client_get_default_database(Connection);
#else
				return nullptr;
#endif
			}
			TCollection* Client::GetCollection(const char* DatabaseName, const char* Name)
			{
#ifdef THAWK_HAS_MONGOC
				return mongoc_client_get_collection(Connection, DatabaseName, Name);
#else
				return nullptr;
#endif
			}
			TReadConcern* Client::GetReadConcern()
			{
#ifdef THAWK_HAS_MONGOC
				return (TReadConcern*)mongoc_client_get_read_concern(Connection);
#else
				return nullptr;
#endif
			}
			TReadPreferences* Client::GetReadPreferences()
			{
#ifdef THAWK_HAS_MONGOC
				return (TReadPreferences*)mongoc_client_get_read_prefs(Connection);
#else
				return nullptr;
#endif
			}
			TWriteConcern* Client::GetWriteConcern()
			{
#ifdef THAWK_HAS_MONGOC
				return (TWriteConcern*)mongoc_client_get_write_concern(Connection);
#else
				return nullptr;
#endif
			}
			TConnection* Client::Get()
			{
				return Connection;
			}
			TURI* Client::GetURI()
			{
#ifdef THAWK_HAS_MONGOC
				return (TURI*)mongoc_client_get_uri(Connection);
#else
				return nullptr;
#endif
			}
			ClientPool* Client::GetMaster()
			{
				return Master;
			}
			std::vector<std::string> Client::GetDatabaseNames(BSON::TDocument** Options)
			{
#ifdef THAWK_HAS_MONGOC
				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				char** Names = mongoc_client_get_database_names_with_opts(Connection, BS(Options), &Error);
				BSON::Document::Release(Options);

				if (Names == nullptr)
				{
					THAWK_ERROR(Error.message);
					return std::vector<std::string>();
				}

				std::vector<std::string> Output;
				for (unsigned i = 0; Names[i]; i++)
					Output.push_back(Names[i]);

				bson_strfreev(Names);
				return Output;
#else
				return std::vector<std::string>();
#endif
			}
			bool Client::IsConnected()
			{
				return Connected;
			}

			ClientPool::ClientPool()
			{
				Connector::Create();
			}
			ClientPool::~ClientPool()
			{
				Disconnect();
				Connector::Release();
			}
			bool ClientPool::Connect(const char* URI)
			{
#ifdef THAWK_HAS_MONGOC
				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (Connected)
					Disconnect();

				Address = mongoc_uri_new_with_error(URI, &Error);
				if (Address == nullptr)
				{
					THAWK_ERROR("couldn't parse URI -> %s", Error.message);
					return false;
				}

				Clients = mongoc_client_pool_new(Address);
				if (Clients == nullptr)
				{
					THAWK_ERROR("couldn't connect to requested URI");
					return false;
				}

				Connected = true;
				return true;
#else
				return false;
#endif
			}
			bool ClientPool::ConnectWithURI(TURI* URI)
			{
#ifdef THAWK_HAS_MONGOC
				if (!URI)
					return false;

				if (Connected)
					Disconnect();

				Clients = mongoc_client_pool_new(Address = URI);
				if (Clients == nullptr)
				{
					THAWK_ERROR("couldn't connect to requested URI");
					return false;
				}

				Connected = true;
				return true;
#else
				return false;
#endif
			}
			bool ClientPool::Disconnect()
			{
#ifdef THAWK_HAS_MONGOC
				if (Clients != nullptr)
				{
					mongoc_client_pool_destroy(Clients);
					Clients = nullptr;
				}

				if (Address != nullptr)
				{
					mongoc_uri_destroy(Address);
					Address = nullptr;
				}

				Connected = false;
				return true;
#else
				return false;
#endif
			}
			void ClientPool::SetAppName(const char* Name)
			{
#ifdef THAWK_HAS_MONGOC
				mongoc_client_pool_set_appname(Clients, Name);
#endif
			}
			void ClientPool::SetSSLOptions(const SSLOptions& Options)
			{
#ifdef THAWK_HAS_MONGOC
				mongoc_client_pool_set_ssl_opts(Clients, (const mongoc_ssl_opt_t*)&Options);
#endif
			}
			void ClientPool::SetAPMCallbacks(const APMCallbacks& Options)
			{
#ifdef THAWK_HAS_MONGOC
				mongoc_apm_callbacks_t* Callbacks = mongoc_apm_callbacks_new();
				if (Options.OnCommandStarted != nullptr)
				{
					mongoc_apm_set_command_started_cb(Callbacks, [](const mongoc_apm_command_started_t* Event) -> void
					{
						Client* Client = (MongoDB::Client*)mongoc_apm_command_started_get_context(Event);
						if (!Client || !Client->ApmCallbacks.OnCommandStarted)
							return;

						APMCommandStarted Command;
						Command.OperationId = mongoc_apm_command_started_get_operation_id(Event);
						Command.RequestId = mongoc_apm_command_started_get_request_id(Event);
						Command.ServerId = mongoc_apm_command_started_get_server_id(Event);
						Command.Hosts = (HostList*)mongoc_apm_command_started_get_host(Event);
						Command.Command = (BSON::TDocument*)mongoc_apm_command_started_get_command(Event);
						Command.CommandName = mongoc_apm_command_started_get_command_name(Event);
						Command.DatabaseName = mongoc_apm_command_started_get_database_name(Event);
						Command.Event = (void*)Event;

						Client->ApmCallbacks.OnCommandStarted(Client, &Command);
					});
				}
				if (Options.OnCommandSucceeded != nullptr)
				{
					mongoc_apm_set_command_succeeded_cb(Callbacks, [](const mongoc_apm_command_succeeded_t* Event) -> void
					{
						Client* Client = (MongoDB::Client*)mongoc_apm_command_succeeded_get_context(Event);
						if (!Client || !Client->ApmCallbacks.OnCommandSucceeded)
							return;

						APMCommandSucceeded Command;
						Command.Duration = mongoc_apm_command_succeeded_get_duration(Event);
						Command.OperationId = mongoc_apm_command_succeeded_get_operation_id(Event);
						Command.RequestId = mongoc_apm_command_succeeded_get_request_id(Event);
						Command.ServerId = mongoc_apm_command_succeeded_get_server_id(Event);
						Command.Hosts = (HostList*)mongoc_apm_command_succeeded_get_host(Event);
						Command.Reply = (BSON::TDocument*)mongoc_apm_command_succeeded_get_reply(Event);
						Command.CommandName = mongoc_apm_command_succeeded_get_command_name((mongoc_apm_command_succeeded_t*)Event);
						Command.Event = (void*)Event;

						Client->ApmCallbacks.OnCommandSucceeded(Client, &Command);
					});
				}
				if (Options.OnCommandFailed != nullptr)
				{
					mongoc_apm_set_command_failed_cb(Callbacks, [](const mongoc_apm_command_failed_t* Event) -> void
					{
						Client* Client = (MongoDB::Client*)mongoc_apm_command_failed_get_context(Event);
						if (!Client || !Client->ApmCallbacks.OnCommandFailed)
							return;

						APMCommandFailed Command;
						Command.Duration = mongoc_apm_command_failed_get_duration(Event);
						Command.OperationId = mongoc_apm_command_failed_get_operation_id(Event);
						Command.RequestId = mongoc_apm_command_failed_get_request_id(Event);
						Command.ServerId = mongoc_apm_command_failed_get_server_id(Event);
						Command.Hosts = (HostList*)mongoc_apm_command_failed_get_host(Event);
						Command.Reply = (BSON::TDocument*)mongoc_apm_command_failed_get_reply(Event);
						Command.CommandName = mongoc_apm_command_failed_get_command_name(Event);
						Command.Event = (void*)Event;

						Client->ApmCallbacks.OnCommandFailed(Client, &Command);
					});
				}
				if (Options.OnServerChanged != nullptr)
				{
					mongoc_apm_set_server_changed_cb(Callbacks, [](const mongoc_apm_server_changed_t* Event) -> void
					{
						Client* Client = (MongoDB::Client*)mongoc_apm_server_changed_get_context(Event);
						if (!Client || !Client->ApmCallbacks.OnServerChanged)
							return;

						APMServerChanged Command;
						Command.Hosts = (HostList*)mongoc_apm_server_changed_get_host(Event);
						Command.New = (TServerDescription*)mongoc_apm_server_changed_get_new_description(Event);
						Command.Previous = (TServerDescription*)mongoc_apm_server_changed_get_previous_description(Event);
						Command.Event = (void*)Event;

						Client->ApmCallbacks.OnServerChanged(Client, &Command);
					});
				}
				if (Options.OnServerOpened != nullptr)
				{
					mongoc_apm_set_server_opening_cb(Callbacks, [](const mongoc_apm_server_opening_t* Event) -> void
					{
						Client* Client = (MongoDB::Client*)mongoc_apm_server_opening_get_context(Event);
						if (!Client || !Client->ApmCallbacks.OnServerOpened)
							return;

						APMServerOpened Command;
						Command.Hosts = (HostList*)mongoc_apm_server_opening_get_host(Event);
						Command.Event = (void*)Event;

						Client->ApmCallbacks.OnServerOpened(Client, &Command);
					});
				}
				if (Options.OnServerClosed != nullptr)
				{
					mongoc_apm_set_server_closed_cb(Callbacks, [](const mongoc_apm_server_closed_t* Event) -> void
					{
						Client* Client = (MongoDB::Client*)mongoc_apm_server_closed_get_context(Event);
						if (!Client || !Client->ApmCallbacks.OnServerClosed)
							return;

						APMServerClosed Command;
						Command.Hosts = (HostList*)mongoc_apm_server_closed_get_host(Event);
						Command.Event = (void*)Event;

						Client->ApmCallbacks.OnServerClosed(Client, &Command);
					});
				}
				if (Options.OnServerHeartbeatFailed != nullptr)
				{
					mongoc_apm_set_server_heartbeat_failed_cb(Callbacks, [](const mongoc_apm_server_heartbeat_failed_t* Event) -> void
					{
						Client* Client = (MongoDB::Client*)mongoc_apm_server_heartbeat_failed_get_context(Event);
						if (!Client || !Client->ApmCallbacks.OnServerHeartbeatFailed)
							return;

						bson_error_t Error;
						memset(&Error, 0, sizeof(bson_error_t));

						mongoc_apm_server_heartbeat_failed_get_error(Event, &Error);

						APMServerHeartbeatFailed Command;
						Command.Duration = mongoc_apm_server_heartbeat_failed_get_duration(Event);
						Command.Message = Error.message;
						Command.ErrorCode = Error.code;
						Command.Domain = Error.domain;
						Command.Hosts = (HostList*)mongoc_apm_server_heartbeat_failed_get_host(Event);
						Command.Event = (void*)Event;

						Client->ApmCallbacks.OnServerHeartbeatFailed(Client, &Command);
					});
				}
				if (Options.OnServerHeartbeatStarted != nullptr)
				{
					mongoc_apm_set_server_heartbeat_started_cb(Callbacks, [](const mongoc_apm_server_heartbeat_started_t* Event) -> void
					{
						Client* Client = (MongoDB::Client*)mongoc_apm_server_heartbeat_started_get_context(Event);
						if (!Client || !Client->ApmCallbacks.OnServerHeartbeatStarted)
							return;

						APMServerHeartbeatStarted Command;
						Command.Hosts = (HostList*)mongoc_apm_server_heartbeat_started_get_host(Event);
						Command.Event = (void*)Event;

						Client->ApmCallbacks.OnServerHeartbeatStarted(Client, &Command);
					});
				}
				if (Options.OnServerHeartbeatSucceeded != nullptr)
				{
					mongoc_apm_set_server_heartbeat_succeeded_cb(Callbacks, [](const mongoc_apm_server_heartbeat_succeeded_t* Event) -> void
					{
						Client* Client = (MongoDB::Client*)mongoc_apm_server_heartbeat_succeeded_get_context(Event);
						if (!Client || !Client->ApmCallbacks.OnServerHeartbeatSucceeded)
							return;

						APMServerHeartbeatSucceeded Command;
						Command.Duration = mongoc_apm_server_heartbeat_succeeded_get_duration(Event);
						Command.Hosts = (HostList*)mongoc_apm_server_heartbeat_succeeded_get_host(Event);
						Command.Reply = (BSON::TDocument*)mongoc_apm_server_heartbeat_succeeded_get_reply(Event);
						Command.Event = (void*)Event;

						Client->ApmCallbacks.OnServerHeartbeatSucceeded(Client, &Command);
					});
				}
				if (Options.OnTopologyChanged != nullptr)
				{
					mongoc_apm_set_topology_changed_cb(Callbacks, [](const mongoc_apm_topology_changed_t* Event) -> void
					{
						Client* Client = (MongoDB::Client*)mongoc_apm_topology_changed_get_context(Event);
						if (!Client || !Client->ApmCallbacks.OnTopologyChanged)
							return;

						APMTopologyChanged Command;
						Command.Event = (void*)Event;
						Command.New = (TTopologyDescription*)mongoc_apm_topology_changed_get_new_description(Event);
						Command.Previous = (TTopologyDescription*)mongoc_apm_topology_changed_get_previous_description(Event);

						Client->ApmCallbacks.OnTopologyChanged(Client, &Command);
					});
				}
				if (Options.OnTopologyClosed != nullptr)
				{
					mongoc_apm_set_topology_closed_cb(Callbacks, [](const mongoc_apm_topology_closed_t* Event) -> void
					{
						Client* Client = (MongoDB::Client*)mongoc_apm_topology_closed_get_context(Event);
						if (!Client || !Client->ApmCallbacks.OnTopologyClosed)
							return;

						APMTopologyClosed Command;
						Command.Event = (void*)Event;

						Client->ApmCallbacks.OnTopologyClosed(Client, &Command);
					});
				}
				if (Options.OnTopologyOpened != nullptr)
				{
					mongoc_apm_set_topology_opening_cb(Callbacks, [](const mongoc_apm_topology_opening_t* Event) -> void
					{
						Client* Client = (MongoDB::Client*)mongoc_apm_topology_opening_get_context(Event);
						if (!Client || !Client->ApmCallbacks.OnTopologyOpened)
							return;

						APMTopologyOpened Command;
						Command.Event = (void*)Event;

						Client->ApmCallbacks.OnTopologyOpened(Client, &Command);
					});
				}

				memcpy(&ApmCallbacks, &Options, sizeof(APMCallbacks));
				mongoc_client_pool_set_apm_callbacks(Clients, Callbacks, this);
				mongoc_apm_callbacks_destroy(Callbacks);
#endif
			}
			void ClientPool::Push(Client** Client)
			{
#ifdef THAWK_HAS_MONGOC
				if (!Client || !*Client)
					return;

				mongoc_client_pool_push(Clients, (*Client)->Connection);
				(*Client)->Connection = nullptr;
				(*Client)->Connected = false;
				(*Client)->Master = nullptr;

				delete *Client;
				*Client = nullptr;
#endif
			}
			Client* ClientPool::Pop()
			{
#ifdef THAWK_HAS_MONGOC
				TConnection* Connection = mongoc_client_pool_pop(Clients);
				if (!Connection)
					return nullptr;

				Client* Client = new MongoDB::Client();
				Client->Connection = Connection;
				Client->Connected = true;
				Client->Master = this;

				return Client;
#else
				return nullptr;
#endif
			}
			TConnectionPool* ClientPool::Get()
			{
				return Clients;
			}
			TURI* ClientPool::GetURI()
			{
				return Address;
			}
		}
	}
}