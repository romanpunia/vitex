#include "mdb.h"
extern "C"
{
#ifdef TH_HAS_MONGOC
#include <mongoc.h>
#endif
}

namespace Tomahawk
{
	namespace Network
	{
		namespace MDB
		{
#ifdef TH_HAS_MONGOC
			template <typename R, typename T, typename... Args>
			bool MDB_EXEC(R&& Function, T* Base, Args&&... Data)
			{
				if (!Base)
					return false;

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				bool Result = Function(Base, Data..., &Error);
				if (!Result && Error.code != 0)
					TH_ERROR("[mongoc:%i] %s", (int)Error.code, Error.message);

				return Result;
			}
			template <typename R, typename T, typename... Args>
			Cursor MDB_EXEC_CUR(R&& Function, T* Base, Args&&... Data)
			{
				if (!Base)
					return nullptr;

				TCursor* Result = Function(Base, Data...);
				if (!Result)
					TH_ERROR("[mongoc] cursor cannot be fetched");

				return Result;
			}
#endif
			Property::Property() : Object(nullptr), Array(nullptr), Mod(Type::Unknown), Integer(0), High(0), Low(0), Number(0.0), Boolean(false), IsValid(false)
			{
			}
			Property::~Property()
			{
				Release();
			}
			void Property::Release()
			{
				Document(Object).Release();
				Object = nullptr;

				Document(Array).Release();
				Array = nullptr;

				Name.clear();
				String.clear();
				Integer = 0;
				Number = 0;
				Boolean = false;
				Mod = Type::Unknown;
				IsValid = false;
			}
			std::string& Property::ToString()
			{
				switch (Mod)
				{
					case Type::Document:
						return String.assign("{}");
					case Type::Array:
						return String.assign("[]");
					case Type::String:
						return String;
					case Type::Boolean:
						return String.assign(Boolean ? "true" : "false");
					case Type::Number:
						return String.assign(std::to_string(Number));
					case Type::Integer:
						return String.assign(std::to_string(Integer));
					case Type::ObjectId:
						return String.assign(Compute::Common::Base64Encode((const char*)ObjectId));
					case Type::Null:
						return String.assign("null");
					case Type::Unknown:
					case Type::Uncastable:
						return String.assign("undefined");
					case Type::Decimal:
					{
#ifdef TH_HAS_MONGOC
						bson_decimal128_t Decimal;
						Decimal.high = (uint64_t)High;
						Decimal.low = (uint64_t)Low;

						char Buffer[BSON_DECIMAL128_STRING];
						bson_decimal128_to_string(&Decimal, Buffer);
						return String.assign(Buffer);
#else
						break;
#endif
					}
				}

				return String;
			}

			bool Util::GetId(unsigned char* Id12)
			{
#ifdef TH_HAS_MONGOC
				if (Id12 == nullptr)
					return false;

				bson_oid_t ObjectId;
				bson_oid_init(&ObjectId, nullptr);

				memcpy((void*)Id12, (void*)ObjectId.bytes, sizeof(char) * 12);
				return true;
#else
				return false;
#endif
			}
			bool Util::GetDecimal(const char* Value, int64_t* High, int64_t* Low)
			{
				if (!Value || !High || !Low)
					return false;

#ifdef TH_HAS_MONGOC
				bson_decimal128_t Decimal;
				if (!bson_decimal128_from_string(Value, &Decimal))
					return false;

				*High = Decimal.high;
				*Low = Decimal.low;
				return true;
#else
				return false;
#endif
			}
			unsigned int Util::GetHashId(unsigned char* Id12)
			{
#ifdef TH_HAS_MONGOC
				if (!Id12 || strlen((const char*)Id12) != 12)
					return 0;

				bson_oid_t Id;
				memcpy((void*)Id.bytes, (void*)Id12, sizeof(char) * 12);

				return bson_oid_hash(&Id);
#else
				return 0;
#endif
			}
			int64_t Util::GetTimeId(unsigned char* Id12)
			{
#ifdef TH_HAS_MONGOC
				if (!Id12 || strlen((const char*)Id12) != 12)
					return 0;

				bson_oid_t Id;
				memcpy((void*)Id.bytes, (void*)Id12, sizeof(char) * 12);

				return bson_oid_get_time_t(&Id);
#else
				return 0;
#endif
			}
			std::string Util::IdToString(unsigned char* Id12)
			{
#ifdef TH_HAS_MONGOC
				if (!Id12)
					return 0;

				bson_oid_t Id;
				memcpy(Id.bytes, Id12, sizeof(unsigned char) * 12);

				char Result[25];
				bson_oid_to_string(&Id, Result);

				return std::string(Result, 24);
#else
				return "";
#endif
			}
			std::string Util::StringToId(const std::string& Id24)
			{
				if (Id24.size() != 24)
					return "";

#ifdef TH_HAS_MONGOC
				bson_oid_t Id;
				bson_oid_init_from_string(&Id, Id24.c_str());

				return std::string((const char*)Id.bytes, 12);
#else
				return "";
#endif
			}

			Document::Document() : Base(nullptr), Store(false)
			{
			}
			Document::Document(TDocument* NewBase) : Base(NewBase), Store(false)
			{
			}
			void Document::Release() const
			{
#ifdef TH_HAS_MONGOC
				if (!Base || Store)
					return;

				if (Base->flags == 2)
				{
					bson_destroy(Base);
					bson_free(Base);
				}
				else
					bson_destroy(Base);
				*((TDocument**)&Base) = nullptr;
#endif
			}
			void Document::Join(const Document& Value)
			{
#ifdef TH_HAS_MONGOC
				if (Base != nullptr && Value.Base != nullptr)
				{
					bson_concat(Base, Value.Base);
					Value.Release();
				}
#endif
			}
			void Document::Loop(const std::function<bool(Property*)>& Callback) const
			{
				if (!Callback || !Base)
					return;

#ifdef TH_HAS_MONGOC
				bson_iter_t It;
				if (!bson_iter_init(&It, Base))
					return;

				Property Output;
				while (bson_iter_next(&It))
				{
					Clone(&It, &Output);
					bool Continue = Callback(&Output);
					Output.Release();

					if (!Continue)
						break;
				}
#endif
			}
			bool Document::SetDocument(const char* Key, const Document& Value, uint64_t ArrayId)
			{
#ifdef TH_HAS_MONGOC
				if (!Value.Base || !Base)
					return false;

				char Index[16];
				if (Key == nullptr)
					bson_uint32_to_string((uint32_t)ArrayId, &Key, Index, sizeof(Index));

				bool Result = bson_append_document(Base, Key, -1, Value.Base);
				Value.Release();

				return Result;
#else
				return false;
#endif
			}
			bool Document::SetArray(const char* Key, const Document& Value, uint64_t ArrayId)
			{
#ifdef TH_HAS_MONGOC
				if (!Value.Base || !Base)
					return false;

				char Index[16];
				if (Key == nullptr)
					bson_uint32_to_string((uint32_t)ArrayId, &Key, Index, sizeof(Index));

				bool Result = bson_append_array(Base, Key, -1, Value.Base);
				Value.Release();

				return Result;
#else
				return false;
#endif
			}
			bool Document::SetString(const char* Key, const char* Value, uint64_t ArrayId)
			{
#ifdef TH_HAS_MONGOC
				if (!Base)
					return false;

				char Index[16];
				if (Key == nullptr)
					bson_uint32_to_string((uint32_t)ArrayId, &Key, Index, sizeof(Index));

				return bson_append_utf8(Base, Key, -1, Value, -1);
#else
				return false;
#endif
			}
			bool Document::SetBlob(const char* Key, const char* Value, uint64_t Length, uint64_t ArrayId)
			{
#ifdef TH_HAS_MONGOC
				if (!Base)
					return false;

				char Index[16];
				if (Key == nullptr)
					bson_uint32_to_string((uint32_t)ArrayId, &Key, Index, sizeof(Index));

				return bson_append_utf8(Base, Key, -1, Value, (int)Length);
#else
				return false;
#endif
			}
			bool Document::SetInteger(const char* Key, int64_t Value, uint64_t ArrayId)
			{
#ifdef TH_HAS_MONGOC
				if (!Base)
					return false;

				char Index[16];
				if (Key == nullptr)
					bson_uint32_to_string((uint32_t)ArrayId, &Key, Index, sizeof(Index));

				return bson_append_int64(Base, Key, -1, Value);
#else
				return false;
#endif
			}
			bool Document::SetNumber(const char* Key, double Value, uint64_t ArrayId)
			{
#ifdef TH_HAS_MONGOC
				if (!Base)
					return false;

				char Index[16];
				if (Key == nullptr)
					bson_uint32_to_string((uint32_t)ArrayId, &Key, Index, sizeof(Index));

				return bson_append_double(Base, Key, -1, Value);
#else
				return false;
#endif
			}
			bool Document::SetDecimal(const char* Key, uint64_t High, uint64_t Low, uint64_t ArrayId)
			{
#ifdef TH_HAS_MONGOC
				if (!Base)
					return false;

				char Index[16];
				if (Key == nullptr)
					bson_uint32_to_string((uint32_t)ArrayId, &Key, Index, sizeof(Index));

				bson_decimal128_t Decimal;
				Decimal.high = (uint64_t)High;
				Decimal.low = (uint64_t)Low;

				return bson_append_decimal128(Base, Key, -1, &Decimal);
#else
				return false;
#endif
			}
			bool Document::SetDecimalString(const char* Key, const std::string& Value, uint64_t ArrayId)
			{
#ifdef TH_HAS_MONGOC
				if (!Base)
					return false;

				char Index[16];
				if (Key == nullptr)
					bson_uint32_to_string((uint32_t)ArrayId, &Key, Index, sizeof(Index));

				bson_decimal128_t Decimal;
				bson_decimal128_from_string(Value.c_str(), &Decimal);

				return bson_append_decimal128(Base, Key, -1, &Decimal);
#else
				return false;
#endif
			}
			bool Document::SetDecimalInteger(const char* Key, int64_t Value, uint64_t ArrayId)
			{
#ifdef TH_HAS_MONGOC
				if (!Base)
					return false;

				char Index[16];
				if (Key == nullptr)
					bson_uint32_to_string((uint32_t)ArrayId, &Key, Index, sizeof(Index));

				char Data[64];
				sprintf(Data, "%lld", Value);

				bson_decimal128_t Decimal;
				bson_decimal128_from_string(Data, &Decimal);

				return bson_append_decimal128(Base, Key, -1, &Decimal);
#else
				return false;
#endif
			}
			bool Document::SetDecimalNumber(const char* Key, double Value, uint64_t ArrayId)
			{
#ifdef TH_HAS_MONGOC
				if (!Base)
					return false;

				char Index[16];
				if (Key == nullptr)
					bson_uint32_to_string((uint32_t)ArrayId, &Key, Index, sizeof(Index));

				char Data[64];
				sprintf(Data, "%f", Value);

				for (size_t i = 0; i < 64; i++)
					Data[i] = (Data[i] == ',' ? '.' : Data[i]);

				bson_decimal128_t Decimal;
				bson_decimal128_from_string(Data, &Decimal);

				return bson_append_decimal128(Base, Key, -1, &Decimal);
#else
				return false;
#endif
			}
			bool Document::SetBoolean(const char* Key, bool Value, uint64_t ArrayId)
			{
#ifdef TH_HAS_MONGOC
				if (!Base)
					return false;

				char Index[16];
				if (Key == nullptr)
					bson_uint32_to_string((uint32_t)ArrayId, &Key, Index, sizeof(Index));

				return bson_append_bool(Base, Key, -1, Value);
#else
				return false;
#endif
			}
			bool Document::SetObjectId(const char* Key, unsigned char Value[12], uint64_t ArrayId)
			{
#ifdef TH_HAS_MONGOC
				if (!Base)
					return false;

				bson_oid_t ObjectId;
				memcpy(ObjectId.bytes, Value, sizeof(unsigned char) * 12);

				char Index[16];
				if (Key == nullptr)
					bson_uint32_to_string((uint32_t)ArrayId, &Key, Index, sizeof(Index));

				return bson_append_oid(Base, Key, -1, &ObjectId);
#else
				return false;
#endif
			}
			bool Document::SetNull(const char* Key, uint64_t ArrayId)
			{
#ifdef TH_HAS_MONGOC
				if (!Base)
					return false;

				char Index[16];
				if (Key == nullptr)
					bson_uint32_to_string((uint32_t)ArrayId, &Key, Index, sizeof(Index));

				return bson_append_null(Base, Key, -1);
#else
				return false;
#endif
			}
			bool Document::Set(const char* Key, Property* Value, uint64_t ArrayId)
			{
#ifdef TH_HAS_MONGOC
				if (!Base || !Value)
					return false;

				char Index[16];
				if (Key == nullptr)
					bson_uint32_to_string((uint32_t)ArrayId, &Key, Index, sizeof(Index));

				switch (Value->Mod)
				{
					case Type::Document:
						return SetDocument(Key, Document(Value->Object).Copy());
					case Type::Array:
						return SetArray(Key, Document(Value->Array).Copy());
					case Type::String:
						return SetString(Key, Value->String.c_str());
					case Type::Boolean:
						return SetBoolean(Key, Value->Boolean);
					case Type::Number:
						return SetNumber(Key, Value->Number);
					case Type::Integer:
						return SetInteger(Key, Value->Integer);
					case Type::Decimal:
						return SetDecimal(Key, Value->High, Value->Low);
					case Type::ObjectId:
						return SetObjectId(Key, Value->ObjectId);
					default:
						return false;
				}
#else
				return false;
#endif
			}
			bool Document::Has(const char* Key) const
			{
#ifdef TH_HAS_MONGOC
				if (!Base)
					return false;

				return bson_has_field(Base, Key);
#else
				return false;
#endif
			}
			bool Document::Get(const char* Key, Property* Output) const
			{
#ifdef TH_HAS_MONGOC
				bson_iter_t It;
				if (!Base || !bson_iter_init_find(&It, Base, Key))
					return false;

				return Clone(&It, Output);
#else
				return false;
#endif
			}
			bool Document::Find(const char* Key, Property* Output) const
			{
#ifdef TH_HAS_MONGOC
				bson_iter_t It, Value;
				if (!Base || !bson_iter_init(&It, Base) || !bson_iter_find_descendant(&It, Key, &Value))
					return false;

				return Clone(&Value, Output);
#else
				return false;
#endif
			}
			bool Document::Clone(void* It, Property* Output)
			{
#ifdef TH_HAS_MONGOC
				const bson_value_t* Value = bson_iter_value((bson_iter_t*)It);
				if (!Value || !Output)
					return false;

				Output->IsValid = false;
				Output->Release();

				switch (Value->value_type)
				{
					case BSON_TYPE_DOCUMENT:
						Output->Mod = Type::Document;
						Output->Object = Document::FromBuffer((const unsigned char*)Value->value.v_doc.data, (uint64_t)Value->value.v_doc.data_len).Get();
						break;
					case BSON_TYPE_ARRAY:
						Output->Mod = Type::Array;
						Output->Array = Document::FromBuffer((const unsigned char*)Value->value.v_doc.data, (uint64_t)Value->value.v_doc.data_len).Get();
						break;
					case BSON_TYPE_BOOL:
						Output->Mod = Type::Boolean;
						Output->Boolean = Value->value.v_bool;
						break;
					case BSON_TYPE_INT32:
						Output->Mod = Type::Integer;
						Output->Integer = Value->value.v_int32;
						break;
					case BSON_TYPE_INT64:
						Output->Mod = Type::Integer;
						Output->Integer = Value->value.v_int64;
						break;
					case BSON_TYPE_DOUBLE:
						Output->Mod = Type::Number;
						Output->Number = Value->value.v_double;
						break;
					case BSON_TYPE_DECIMAL128:
						Output->Mod = Type::Decimal;
						Output->High = (uint64_t)Value->value.v_decimal128.high;
						Output->Low = (uint64_t)Value->value.v_decimal128.low;
						break;
					case BSON_TYPE_UTF8:
						Output->Mod = Type::String;
						Output->String.assign(Value->value.v_utf8.str, (uint64_t)Value->value.v_utf8.len);
						break;
					case BSON_TYPE_TIMESTAMP:
						Output->Mod = Type::Integer;
						Output->Integer = (int64_t)Value->value.v_timestamp.timestamp;
						Output->Number = (double)Value->value.v_timestamp.increment;
						break;
					case BSON_TYPE_DATE_TIME:
						Output->Mod = Type::Integer;
						Output->Integer = Value->value.v_datetime;
						break;
					case BSON_TYPE_REGEX:
						Output->Mod = Type::String;
						Output->String.assign(Value->value.v_regex.regex).append(1, '\n').append(Value->value.v_regex.options);
						break;
					case BSON_TYPE_CODE:
						Output->Mod = Type::String;
						Output->String.assign(Value->value.v_code.code, (uint64_t)Value->value.v_code.code_len);
						break;
					case BSON_TYPE_SYMBOL:
						Output->Mod = Type::String;
						Output->String.assign(Value->value.v_symbol.symbol, (uint64_t)Value->value.v_symbol.len);
						break;
					case BSON_TYPE_CODEWSCOPE:
						Output->Mod = Type::String;
						Output->String.assign(Value->value.v_codewscope.code, (uint64_t)Value->value.v_codewscope.code_len);
						break;
					case BSON_TYPE_UNDEFINED:
					case BSON_TYPE_NULL:
						Output->Mod = Type::Null;
						break;
					case BSON_TYPE_OID:
						Output->Mod = Type::ObjectId;
						memcpy(Output->ObjectId, Value->value.v_oid.bytes, sizeof(unsigned char) * 12);
						break;
					case BSON_TYPE_EOD:
					case BSON_TYPE_BINARY:
					case BSON_TYPE_DBPOINTER:
					case BSON_TYPE_MAXKEY:
					case BSON_TYPE_MINKEY:
						Output->Mod = Type::Uncastable;
						break;
					default:
						break;
				}

				Output->Name.assign(bson_iter_key((const bson_iter_t*)It));
				Output->IsValid = true;

				return true;
#else
				return false;
#endif
			}
			uint64_t Document::Count() const
			{
#ifdef TH_HAS_MONGOC
				if (!Base)
					return 0;

				return bson_count_keys(Base);
#else
				return 0;
#endif
			}
			std::string Document::ToRelaxedJSON() const
			{
#ifdef TH_HAS_MONGOC
				if (!Base)
					return std::string();

				size_t Length = 0;
				char* Value = bson_as_relaxed_extended_json(Base, &Length);

				std::string Output;
				Output.assign(Value, (uint64_t)Length);
				bson_free(Value);

				return Output;
#else
				return "";
#endif
			}
			std::string Document::ToExtendedJSON() const
			{
#ifdef TH_HAS_MONGOC
				if (!Base)
					return std::string();

				size_t Length = 0;
				char* Value = bson_as_canonical_extended_json(Base, &Length);

				std::string Output;
				Output.assign(Value, (uint64_t)Length);
				bson_free(Value);

				return Output;
#else
				return "";
#endif
			}
			std::string Document::ToJSON() const
			{
#ifdef TH_HAS_MONGOC
				if (!Base)
					return std::string();

				size_t Length = 0;
				char* Value = bson_as_json(Base, &Length);

				std::string Output;
				Output.assign(Value, (uint64_t)Length);
				bson_free(Value);

				return Output;
#else
				return "";
#endif
			}
			std::string Document::ToIndices() const
			{
#ifdef TH_HAS_MONGOC
				if (!Base)
					return std::string();

				char* Value = mongoc_collection_keys_to_index_string(Base);
				if (!Value)
					return std::string();

				std::string Output(Value);
				bson_free(Value);

				return Output;
#else
				return "";
#endif
			}
			Core::Document* Document::ToDocument(bool IsArray) const
			{
#ifdef TH_HAS_MONGOC
				if (!Base)
					return nullptr;

				Core::Document* Node = (IsArray ? Core::Document::Array() : Core::Document::Object());
				Loop([Node](Property* Key) -> bool
				{
					std::string Name = (Node->Value.GetType() == Core::VarType::Array ? "" : Key->Name);
					switch (Key->Mod)
					{
						case Type::Document:
						{
							Node->Set(Name, Document(Key->Object).ToDocument(false));
							break;
						}
						case Type::Array:
						{
							Node->Set(Name, Document(Key->Array).ToDocument(true));
							break;
						}
						case Type::String:
							Node->Set(Name, Core::Var::String(Key->String));
							break;
						case Type::Boolean:
							Node->Set(Name, Core::Var::Boolean(Key->Boolean));
							break;
						case Type::Number:
							Node->Set(Name, Core::Var::Number(Key->Number));
							break;
						case Type::Decimal:
							Node->Set(Name, Core::Var::DecimalString(Key->ToString()));
							break;
						case Type::Integer:
							Node->Set(Name, Core::Var::Integer(Key->Integer));
							break;
						case Type::ObjectId:
							Node->Set(Name, Core::Var::Base64(Key->ObjectId, 12));
							break;
						case Type::Null:
							Node->Set(Name, Core::Var::Null());
							break;
						default:
							break;
					}

					return true;
				});

				return Node;
#else
				return nullptr;
#endif
			}
			TDocument* Document::Get() const
			{
#ifdef TH_HAS_MONGOC
				return Base;
#else
				return nullptr;
#endif
			}
			Document Document::Copy() const
			{
#ifdef TH_HAS_MONGOC
				if (!Base)
					return nullptr;

				return Document(bson_copy(Base));
#else
				return nullptr;
#endif
			}
			Document& Document::Persist(bool Keep)
			{
				Store = Keep;
				return *this;
			}
			Document Document::FromDocument(Core::Document* Src)
			{
#ifdef TH_HAS_MONGOC
				if (!Src || !Src->Value.IsObject())
					return nullptr;

				bool Array = (Src->Value.GetType() == Core::VarType::Array);
				Document Result = bson_new();
				uint64_t Index = 0;

				for (auto&& Node : *Src->GetNodes())
				{
					switch (Node->Value.GetType())
					{
						case Core::VarType::Object:
							Result.SetDocument(Array ? nullptr : Node->Key.c_str(), Document::FromDocument(Node), Index);
							break;
						case Core::VarType::Array:
							Result.SetArray(Array ? nullptr : Node->Key.c_str(), Document::FromDocument(Node), Index);
							break;
						case Core::VarType::String:
							Result.SetBlob(Array ? nullptr : Node->Key.c_str(), Node->Value.GetString(), Node->Value.GetSize(), Index);
							break;
						case Core::VarType::Boolean:
							Result.SetBoolean(Array ? nullptr : Node->Key.c_str(), Node->Value.GetBoolean(), Index);
							break;
						case Core::VarType::Decimal:
							Result.SetDecimalString(Array ? nullptr : Node->Key.c_str(), Node->Value.GetDecimal().ToString(), Index);
							break;
						case Core::VarType::Number:
							Result.SetNumber(Array ? nullptr : Node->Key.c_str(), Node->Value.GetNumber(), Index);
							break;
						case Core::VarType::Integer:
							Result.SetInteger(Array ? nullptr : Node->Key.c_str(), Node->Value.GetInteger(), Index);
							break;
						case Core::VarType::Null:
							Result.SetNull(Array ? nullptr : Node->Key.c_str(), Index);
							break;
						case Core::VarType::Base64:
						{
							if (Node->Value.GetSize() != 12)
							{
								std::string Base = Compute::Common::Base64Encode(Node->Value.GetBlob());
								Result.SetBlob(Array ? nullptr : Node->Key.c_str(), Base.c_str(), Base.size(), Index);
							}
							else
								Result.SetObjectId(Array ? nullptr : Node->Key.c_str(), Node->Value.GetBase64(), Index);
							break;
						}
						default:
							break;
					}
					Index++;
				}

				return Result;
#else
				return nullptr;
#endif
			}
			Document Document::FromEmpty()
			{
#ifdef TH_HAS_MONGOC
				return bson_new();
#else
				return nullptr;
#endif
			}
			Document Document::FromJSON(const std::string& JSON)
			{
#ifdef TH_HAS_MONGOC
				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				TDocument* Result = bson_new_from_json((unsigned char*)JSON.c_str(), (ssize_t)JSON.size(), &Error);
				if (Result != nullptr && Error.code == 0)
					return Result;

				if (Result != nullptr)
					bson_destroy(Result);

				TH_ERROR("[json] %s", Error.message);
				return nullptr;
#else
				return nullptr;
#endif
			}
			Document Document::FromBuffer(const unsigned char* Buffer, uint64_t Length)
			{
#ifdef TH_HAS_MONGOC
				return bson_new_from_data(Buffer, (size_t)Length);
#else
				return nullptr;
#endif
			}
			Document Document::FromSource(TDocument* Src)
			{
#ifdef TH_HAS_MONGOC
				TDocument* Dest = bson_new();
				bson_steal(Dest, Src);
				return Dest;
#else
				return nullptr;
#endif
			}

			Address::Address(TAddress* NewBase) : Base(NewBase)
			{
			}
			void Address::Release()
			{
#ifdef TH_HAS_MONGOC
				if (!Base)
					return;

				mongoc_uri_destroy(Base);
				Base = nullptr;
#endif
			}
			void Address::SetOption(const char* Name, int64_t Value)
			{
#ifdef TH_HAS_MONGOC
				if (Base != nullptr)
					mongoc_uri_set_option_as_int32(Base, Name, (int32_t)Value);
#endif
			}
			void Address::SetOption(const char* Name, bool Value)
			{
#ifdef TH_HAS_MONGOC
				if (Base != nullptr)
					mongoc_uri_set_option_as_bool(Base, Name, Value);
#endif
			}
			void Address::SetOption(const char* Name, const char* Value)
			{
#ifdef TH_HAS_MONGOC
				if (Base != nullptr)
					mongoc_uri_set_option_as_utf8(Base, Name, Value);
#endif
			}
			void Address::SetAuthMechanism(const char* Value)
			{
#ifdef TH_HAS_MONGOC
				if (Base != nullptr)
					mongoc_uri_set_auth_mechanism(Base, Value);
#endif
			}
			void Address::SetAuthSource(const char* Value)
			{
#ifdef TH_HAS_MONGOC
				if (Base != nullptr)
					mongoc_uri_set_auth_source(Base, Value);
#endif
			}
			void Address::SetCompressors(const char* Value)
			{
#ifdef TH_HAS_MONGOC
				if (Base != nullptr)
					mongoc_uri_set_compressors(Base, Value);
#endif
			}
			void Address::SetDatabase(const char* Value)
			{
#ifdef TH_HAS_MONGOC
				if (Base != nullptr)
					mongoc_uri_set_database(Base, Value);
#endif
			}
			void Address::SetUsername(const char* Value)
			{
#ifdef TH_HAS_MONGOC
				if (Base != nullptr)
					mongoc_uri_set_username(Base, Value);
#endif
			}
			void Address::SetPassword(const char* Value)
			{
#ifdef TH_HAS_MONGOC
				if (Base != nullptr)
					mongoc_uri_set_password(Base, Value);
#endif
			}
			TAddress* Address::Get() const
			{
#ifdef TH_HAS_MONGOC
				return Base;
#else
				return nullptr;
#endif
			}
			Address Address::FromURI(const char* Value)
			{
#ifdef TH_HAS_MONGOC
				return mongoc_uri_new(Value);
#else
				return nullptr;
#endif
			}

			Stream::Stream(TStream* NewBase) : Base(NewBase)
			{
			}
			void Stream::Release()
			{
#ifdef TH_HAS_MONGOC
				if (!Base)
					return;

				mongoc_bulk_operation_destroy(Base);
				Base = nullptr;
#endif
			}
			bool Stream::RemoveMany(const Document& Select, const Document& Options)
			{
#ifdef TH_HAS_MONGOC
				bool Subresult = MDB_EXEC(&mongoc_bulk_operation_remove_many_with_opts, Base, Select.Get(), Options.Get());
				Select.Release();
				Options.Release();

				return Subresult;
#else
				return false;
#endif
			}
			bool Stream::RemoveOne(const Document& Select, const Document& Options)
			{
#ifdef TH_HAS_MONGOC
				bool Subresult = MDB_EXEC(&mongoc_bulk_operation_remove_one_with_opts, Base, Select.Get(), Options.Get());
				Select.Release();
				Options.Release();

				return Subresult;
#else
				return false;
#endif
			}
			bool Stream::ReplaceOne(const Document& Select, const Document& Replacement, const Document& Options)
			{
#ifdef TH_HAS_MONGOC
				bool Subresult = MDB_EXEC(&mongoc_bulk_operation_replace_one_with_opts, Base, Select.Get(), Replacement.Get(), Options.Get());
				Select.Release();
				Replacement.Release();
				Options.Release();

				return Subresult;
#else
				return false;
#endif
			}
			bool Stream::Insert(const Document& Result, const Document& Options)
			{
#ifdef TH_HAS_MONGOC
				bool Subresult = MDB_EXEC(&mongoc_bulk_operation_insert_with_opts, Base, Result.Get(), Options.Get());
				Result.Release();
				Options.Release();

				return Subresult;
#else
				return false;
#endif
			}
			bool Stream::UpdateOne(const Document& Select, const Document& Result, const Document& Options)
			{
#ifdef TH_HAS_MONGOC
				bool Subresult = MDB_EXEC(&mongoc_bulk_operation_update_one_with_opts, Base, Select.Get(), Result.Get(), Options.Get());
				Select.Release();
				Result.Release();
				Options.Release();

				return Subresult;
#else
				return false;
#endif
			}
			bool Stream::UpdateMany(const Document& Select, const Document& Result, const Document& Options)
			{
#ifdef TH_HAS_MONGOC
				bool Subresult = MDB_EXEC(&mongoc_bulk_operation_update_many_with_opts, Base, Select.Get(), Result.Get(), Options.Get());
				Select.Release();
				Result.Release();
				Options.Release();

				return Subresult;
#else
				return false;
#endif
			}
			Core::Async<Document> Stream::ExecuteWithReply()
			{
#ifdef TH_HAS_MONGOC
				if (!Base)
					return Core::Async<Document>::Store(nullptr);

				Stream Context = *this; Base = nullptr;
				return Core::Async<Document>([Context](Core::Async<Document>& Future) mutable
				{
					TDocument Subresult;
					bool fResult = MDB_EXEC(&mongoc_bulk_operation_execute, Context.Get(), &Subresult);
					Context.Release();

					if (!fResult)
					{
						bson_free(&Subresult);
						Future.Set(Document(nullptr));
					}
					else
						Future.Set(Document::FromSource(&Subresult));
				});
#else
				return Core::Async<Document>::Store(nullptr);
#endif
			}
			Core::Async<bool> Stream::Execute()
			{
#ifdef TH_HAS_MONGOC
				if (!Base)
					return Core::Async<bool>::Store(true);

				Stream Context = *this; Base = nullptr;
				return Core::Async<bool>([Context](Core::Async<bool>& Future) mutable
				{
					TDocument Result;
					bool Subresult = MDB_EXEC(&mongoc_bulk_operation_execute, Context.Get(), &Result);
					bson_destroy(&Result);
					Context.Release();

					Future.Set(Subresult);
				});
#else
				return Core::Async<bool>::Store(false);
#endif
			}
			uint64_t Stream::GetHint() const
			{
#ifdef TH_HAS_MONGOC
				if (!Base)
					return 0;

				return (uint64_t)mongoc_bulk_operation_get_hint(Base);
#else
				return 0;
#endif
			}
			TStream* Stream::Get() const
			{
#ifdef TH_HAS_MONGOC
				return Base;
#else
				return nullptr;
#endif
			}
			Stream Stream::FromEmpty(bool IsOrdered)
			{
#ifdef TH_HAS_MONGOC
				return mongoc_bulk_operation_new(IsOrdered);
#else
				return nullptr;
#endif
			}

			Cursor::Cursor() : Base(nullptr)
			{
			}
			Cursor::Cursor(TCursor* NewBase) : Base(NewBase)
			{
			}
			void Cursor::Release()
			{
#ifdef TH_HAS_MONGOC
				if (!Base)
					return;

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (mongoc_cursor_error(Base, &Error))
					TH_ERROR("[mongoc] %s", Error.message);

				mongoc_cursor_destroy(Base);
				Base = nullptr;
#endif
			}
			void Cursor::SetMaxAwaitTime(uint64_t MaxAwaitTime)
			{
#ifdef TH_HAS_MONGOC
				if (Base != nullptr)
					mongoc_cursor_set_max_await_time_ms(Base, (uint32_t)MaxAwaitTime);
#endif
			}
			void Cursor::SetBatchSize(uint64_t BatchSize)
			{
#ifdef TH_HAS_MONGOC
				if (Base != nullptr)
					mongoc_cursor_set_batch_size(Base, (uint32_t)BatchSize);
#endif
			}
			bool Cursor::SetLimit(int64_t Limit)
			{
#ifdef TH_HAS_MONGOC
				if (!Base)
					return false;

				return mongoc_cursor_set_limit(Base, Limit);
#else
				return false;
#endif
			}
			bool Cursor::SetHint(uint64_t Hint)
			{
#ifdef TH_HAS_MONGOC
				if (!Base)
					return false;

				return mongoc_cursor_set_hint(Base, (uint32_t)Hint);
#else
				return false;
#endif
			}
			bool Cursor::HasError() const
			{
#ifdef TH_HAS_MONGOC
				if (!Base)
					return false;

				return mongoc_cursor_error(Base, nullptr);
#else
				return false;
#endif
			}
			bool Cursor::HasMoreData() const
			{
#ifdef TH_HAS_MONGOC
				if (!Base)
					return false;

				return mongoc_cursor_more(Base);
#else
				return false;
#endif
			}
			bool Cursor::NextSync() const
			{
#ifdef TH_HAS_MONGOC
				if (!Base)
					return false;

				TDocument* Query = nullptr;
				return mongoc_cursor_next(Base, (const TDocument**)&Query);
#else
				return false;
#endif
			}
			Core::Async<bool> Cursor::Next() const
			{
#ifdef TH_HAS_MONGOC
				if (!Base)
					return Core::Async<bool>::Store(false);

				auto* Context = Base;
				return Core::Async<bool>([Context](Core::Async<bool>& Future)
				{
					TDocument* Query = nullptr;
					Future.Set(mongoc_cursor_next(Context, (const TDocument**)&Query));
				});
#else
				return Core::Async<bool>::Store(false);
#endif
			}
			int64_t Cursor::GetId() const
			{
#ifdef TH_HAS_MONGOC
				if (!Base)
					return 0;

				return (int64_t)mongoc_cursor_get_id(Base);
#else
				return 0;
#endif
			}
			int64_t Cursor::GetLimit() const
			{
#ifdef TH_HAS_MONGOC
				if (!Base)
					return 0;

				return (int64_t)mongoc_cursor_get_limit(Base);
#else
				return 0;
#endif
			}
			uint64_t Cursor::GetMaxAwaitTime() const
			{
#ifdef TH_HAS_MONGOC
				if (!Base)
					return 0;

				return (uint64_t)mongoc_cursor_get_max_await_time_ms(Base);
#else
				return 0;
#endif
			}
			uint64_t Cursor::GetBatchSize() const
			{
#ifdef TH_HAS_MONGOC
				if (!Base)
					return 0;

				return (uint64_t)mongoc_cursor_get_batch_size(Base);
#else
				return 0;
#endif
			}
			uint64_t Cursor::GetHint() const
			{
#ifdef TH_HAS_MONGOC
				if (!Base)
					return 0;

				return (uint64_t)mongoc_cursor_get_hint(Base);
#else
				return 0;
#endif
			}
			Document Cursor::GetCurrent() const
			{
#ifdef TH_HAS_MONGOC
				if (!Base)
					return nullptr;

				return (TDocument*)mongoc_cursor_current(Base);
#else
				return nullptr;
#endif
			}
			Cursor Cursor::Clone()
			{
#ifdef TH_HAS_MONGOC
				if (!Base)
					return nullptr;

				return mongoc_cursor_clone(Base);
#else
				return nullptr;
#endif
			}
			TCursor* Cursor::Get() const
			{
#ifdef TH_HAS_MONGOC
				return Base;
#else
				return nullptr;
#endif
			}

            Collection::Collection() : Base(nullptr)
            {
            }
			Collection::Collection(TCollection* NewBase) : Base(NewBase)
			{
			}
			void Collection::Release()
			{
#ifdef TH_HAS_MONGOC
				if (!Base)
					return;

				mongoc_collection_destroy(Base);
				Base = nullptr;
#endif
			}
			Core::Async<bool> Collection::Rename(const std::string& NewDatabaseName, const std::string& NewCollectionName)
			{
#ifdef TH_HAS_MONGOC
				auto* Context = Base;
				return Core::Async<bool>([Context, NewDatabaseName, NewCollectionName](Core::Async<bool>& Future)
				{
					Future.Set(MDB_EXEC(&mongoc_collection_rename, Context, NewDatabaseName.c_str(), NewCollectionName.c_str(), false));
				});
#else
				return Core::Async<bool>::Store(false);
#endif
			}
			Core::Async<bool> Collection::RenameWithOptions(const std::string& NewDatabaseName, const std::string& NewCollectionName, const Document& Options)
			{
#ifdef TH_HAS_MONGOC
				auto* Context = Base;
				return Core::Async<bool>([Context, NewDatabaseName, NewCollectionName, Options](Core::Async<bool>& Future)
				{
					bool Subresult = MDB_EXEC(&mongoc_collection_rename_with_opts, Context, NewDatabaseName.c_str(), NewCollectionName.c_str(), false, Options.Get());
					Options.Release();

					Future.Set(Subresult);
				});
#else
				return Core::Async<bool>::Store(false);
#endif
			}
			Core::Async<bool> Collection::RenameWithRemove(const std::string& NewDatabaseName, const std::string& NewCollectionName)
			{
#ifdef TH_HAS_MONGOC
				auto* Context = Base;
				return Core::Async<bool>([Context, NewDatabaseName, NewCollectionName](Core::Async<bool>& Future)
				{
					Future.Set(MDB_EXEC(&mongoc_collection_rename, Context, NewDatabaseName.c_str(), NewCollectionName.c_str(), true));
				});
#else
				return Core::Async<bool>::Store(false);
#endif
			}
			Core::Async<bool> Collection::RenameWithOptionsAndRemove(const std::string& NewDatabaseName, const std::string& NewCollectionName, const Document& Options)
			{
#ifdef TH_HAS_MONGOC
				auto* Context = Base;
				return Core::Async<bool>([Context, NewDatabaseName, NewCollectionName, Options](Core::Async<bool>& Future)
				{
					bool Subresult = MDB_EXEC(&mongoc_collection_rename_with_opts, Context, NewDatabaseName.c_str(), NewCollectionName.c_str(), true, Options.Get());
					Options.Release();

					Future.Set(Subresult);
				});
#else
				return Core::Async<bool>::Store(false);
#endif
			}
			Core::Async<bool> Collection::Remove(const Document& Options)
			{
#ifdef TH_HAS_MONGOC
				auto* Context = Base;
				return Core::Async<bool>([Context, Options](Core::Async<bool>& Future)
				{
					bool Subresult = MDB_EXEC(&mongoc_collection_drop_with_opts, Context, Options.Get());
					Options.Release();

					Future.Set(Subresult);
				});
#else
				return Core::Async<bool>::Store(false);
#endif
			}
			Core::Async<bool> Collection::RemoveIndex(const std::string& Name, const Document& Options)
			{
#ifdef TH_HAS_MONGOC
				auto* Context = Base;
				return Core::Async<bool>([Context, Name, Options](Core::Async<bool>& Future)
				{
					bool Subresult = MDB_EXEC(&mongoc_collection_drop_index_with_opts, Context, Name.c_str(), Options.Get());
					Options.Release();

					Future.Set(Subresult);
				});
#else
				return Core::Async<bool>::Store(false);
#endif
			}
			Core::Async<Document> Collection::RemoveMany(const Document& Select, const Document& Options)
			{
#ifdef TH_HAS_MONGOC
				auto* Context = Base;
				return Core::Async<Document>([Context, Select, Options](Core::Async<Document>& Future)
				{
					TDocument Subresult;
					bool fResult = MDB_EXEC(&mongoc_collection_delete_many, Context, Select.Get(), Options.Get(), &Subresult);
					Select.Release();
					Options.Release();

					if (!fResult)
					{
						bson_free(&Subresult);
						Future.Set(Document(nullptr));
					}
					else
						Future.Set(Document::FromSource(&Subresult));
				});
#else
				return Core::Async<Document>::Store(nullptr);
#endif
			}
			Core::Async<Document> Collection::RemoveOne(const Document& Select, const Document& Options)
			{
#ifdef TH_HAS_MONGOC
				auto* Context = Base;
				return Core::Async<Document>([Context, Select, Options](Core::Async<Document>& Future)
				{
					TDocument Subresult;
					bool fResult = MDB_EXEC(&mongoc_collection_delete_one, Context, Select.Get(), Options.Get(), &Subresult);
					Select.Release();
					Options.Release();

					if (!fResult)
					{
						bson_free(&Subresult);
						Future.Set(Document(nullptr));
					}
					else
						Future.Set(Document::FromSource(&Subresult));
				});
#else
				return Core::Async<Document>::Store(nullptr);
#endif
			}
			Core::Async<Document> Collection::ReplaceOne(const Document& Select, const Document& Replacement, const Document& Options)
			{
#ifdef TH_HAS_MONGOC
				auto* Context = Base;
				return Core::Async<Document>([Context, Select, Replacement, Options](Core::Async<Document>& Future)
				{
					TDocument Subresult;
					bool fResult = MDB_EXEC(&mongoc_collection_replace_one, Context, Select.Get(), Replacement.Get(), Options.Get(), &Subresult);
					Select.Release();
					Replacement.Release();
					Options.Release();

					if (!fResult)
					{
						bson_free(&Subresult);
						Future.Set(Document(nullptr));
					}
					else
						Future.Set(Document::FromSource(&Subresult));
				});
#else
				return Core::Async<Document>::Store(nullptr);
#endif
			}
			Core::Async<Document> Collection::InsertMany(std::vector<Document>& List, const Document& Options)
			{
#ifdef TH_HAS_MONGOC
				if (List.empty())
					return Core::Async<Document>::Store(nullptr);

				std::vector<Document> Array(std::move(List));
				auto* Context = Base;

				return Core::Async<Document>([Context, Array = std::move(Array), Options](Core::Async<Document>& Future) mutable
				{
					TDocument** Subarray = (TDocument**)TH_MALLOC(sizeof(TDocument*) * Array.size());
					for (size_t i = 0; i < Array.size(); i++)
						Subarray[i] = Array[i].Get();

					TDocument Subresult;
					bool fResult = MDB_EXEC(&mongoc_collection_insert_many, Context, (const TDocument**)Subarray, (size_t)Array.size(), Options.Get(), &Subresult);
					for (auto& Item : Array)
						Item.Release();
					Options.Release();
					TH_FREE(Subarray);

					if (!fResult)
					{
						bson_free(&Subresult);
						Future.Set(Document(nullptr));
					}
					else
						Future.Set(Document::FromSource(&Subresult));
				});
#else
				return Core::Async<Document>::Store(nullptr);
#endif
			}
			Core::Async<Document> Collection::InsertOne(const Document& Result, const Document& Options)
			{
#ifdef TH_HAS_MONGOC
				auto* Context = Base;
				return Core::Async<Document>([Context, Result, Options](Core::Async<Document>& Future)
				{
					TDocument Subresult;
					bool fResult = MDB_EXEC(&mongoc_collection_insert_one, Context, Result.Get(), Options.Get(), &Subresult);
					Options.Release();
					Result.Release();

					if (!fResult)
					{
						bson_free(&Subresult);
						Future.Set(Document(nullptr));
					}
					else
						Future.Set(Document::FromSource(&Subresult));
				});
#else
				return Core::Async<Document>::Store(nullptr);
#endif
			}
			Core::Async<Document> Collection::UpdateMany(const Document& Select, const Document& Update, const Document& Options)
			{
#ifdef TH_HAS_MONGOC
				auto* Context = Base;
				return Core::Async<Document>([Context, Select, Update, Options](Core::Async<Document>& Future)
				{
					TDocument Subresult;
					bool fResult = MDB_EXEC(&mongoc_collection_update_many, Context, Select.Get(), Update.Get(), Options.Get(), &Subresult);
					Select.Release();
					Update.Release();
					Options.Release();

					if (!fResult)
					{
						bson_free(&Subresult);
						Future.Set(Document(nullptr));
					}
					else
						Future.Set(Document::FromSource(&Subresult));
				});
#else
				return Core::Async<Document>::Store(nullptr);
#endif
			}
			Core::Async<Document> Collection::UpdateOne(const Document& Select, const Document& Update, const Document& Options)
			{
#ifdef TH_HAS_MONGOC
				auto* Context = Base;
				return Core::Async<Document>([Context, Select, Update, Options](Core::Async<Document>& Future)
				{
					TDocument Subresult;
					bool fResult = MDB_EXEC(&mongoc_collection_update_one, Context, Select.Get(), Update.Get(), Options.Get(), &Subresult);
					Select.Release();
					Update.Release();
					Options.Release();

					if (!fResult)
					{
						bson_free(&Subresult);
						Future.Set(Document(nullptr));
					}
					else
						Future.Set(Document::FromSource(&Subresult));
				});
#else
				return Core::Async<Document>::Store(nullptr);
#endif
			}
			Core::Async<Document> Collection::FindAndModify(const Document& Query, const Document& Sort, const Document& Update, const Document& Fields, bool RemoveAt, bool Upsert, bool New)
			{
#ifdef TH_HAS_MONGOC
				auto* Context = Base;
				return Core::Async<Document>([Context, Query, Sort, Update, Fields, RemoveAt, Upsert, New](Core::Async<Document>& Future)
				{
					TDocument Subresult;
					bool fResult = MDB_EXEC(&mongoc_collection_find_and_modify, Context, Query.Get(), Sort.Get(), Update.Get(), Fields.Get(), RemoveAt, Upsert, New, &Subresult);
					Query.Release();
					Sort.Release();
					Update.Release();
					Fields.Release();

					if (!fResult)
					{
						bson_free(&Subresult);
						Future.Set(Document(nullptr));
					}
					else
						Future.Set(Document::FromSource(&Subresult));
				});
#else
				return Core::Async<Document>::Store(nullptr);
#endif
			}
			Core::Async<uint64_t> Collection::CountElementsInArray(const Document& Match, const Document& Filter, const Document& Options) const
			{
#ifdef TH_HAS_MONGOC
				if (!Base || !Filter.Get() || !Match.Get())
				{
					Filter.Release();
					Match.Release();
					Options.Release();
					return Core::Async<uint64_t>::Store(0);
				}

				auto* Context = Base;
				return Core::Async<uint64_t>([Context, Match, Filter, Options](Core::Async<uint64_t>& Future)
				{
					Document Pipeline = BCON_NEW("pipeline", "[", "{", "$match", BCON_DOCUMENT(Match.Get()), "}", "{", "$project", "{", "Count", "{", "$Count", "{", "$filter", BCON_DOCUMENT(Filter.Get()), "}", "}", "}", "}", "]");
					Cursor Subresult = MDB_EXEC_CUR(&mongoc_collection_aggregate, Context, MONGOC_QUERY_NONE, Pipeline.Get(), Options.Get(), nullptr);
					Property Count;

					if (Subresult.NextSync())
					{
						Document Result = Subresult.GetCurrent();
						if (Result.Get())
							Result.Get("Count", &Count);
					}

					Subresult.Release();
					Pipeline.Release();
					Filter.Release();
					Match.Release();
					Options.Release();

					Future.Set((uint64_t)Count.Integer);
				});
#else
				return Core::Async<uint64_t>::Store(0);
#endif
			}
			Core::Async<uint64_t> Collection::CountDocuments(const Document& Select, const Document& Options) const
			{
#ifdef TH_HAS_MONGOC
				auto* Context = Base;
				return Core::Async<uint64_t>([Context, Select, Options](Core::Async<uint64_t>& Future)
				{
					uint64_t Subresult = (uint64_t)MDB_EXEC(&mongoc_collection_count_documents, Context, Select.Get(), Options.Get(), nullptr, nullptr);
					Select.Release();
					Options.Release();

					Future.Set(Subresult);
				});
#else
				return Core::Async<uint64_t>::Store(0);
#endif
			}
			Core::Async<uint64_t> Collection::CountDocumentsEstimated(const Document& Options) const
			{
#ifdef TH_HAS_MONGOC
				auto* Context = Base;
				return Core::Async<uint64_t>([Context, Options](Core::Async<uint64_t>& Future)
				{
					uint64_t Subresult = (uint64_t)MDB_EXEC(&mongoc_collection_estimated_document_count, Context, Options.Get(), nullptr, nullptr);
					Options.Release();

					Future.Set(Subresult);
				});
#else
				return Core::Async<uint64_t>::Store(0);
#endif
			}
			Core::Async<Cursor> Collection::FindIndexes(const Document& Options) const
			{
#ifdef TH_HAS_MONGOC
				auto* Context = Base;
				return Core::Async<Cursor>([Context, Options](Core::Async<Cursor>& Future)
				{
					Cursor Subresult = MDB_EXEC_CUR(&mongoc_collection_find_indexes_with_opts, Context, Options.Get());
					Options.Release();

					Future.Sync(true).Set(Subresult);
				});
#else
				return Core::Async<Cursor>::Store(nullptr);
#endif
			}
			Core::Async<Cursor> Collection::FindMany(const Document& Select, const Document& Options) const
			{
#ifdef TH_HAS_MONGOC
				auto* Context = Base;
				return Core::Async<Cursor>([Context, Select, Options](Core::Async<Cursor>& Future)
				{
					Cursor Subresult = MDB_EXEC_CUR(&mongoc_collection_find_with_opts, Context, Select.Get(), Options.Get(), nullptr);
					Select.Release();
					Options.Release();

					Future.Sync(true).Set(Subresult);
				});
#else
				return Core::Async<Cursor>::Store(nullptr);
#endif
			}
			Core::Async<Cursor> Collection::FindOne(const Document& Select, const Document& Options) const
			{
#ifdef TH_HAS_MONGOC
				auto* Context = Base;
				return Core::Async<Cursor>([Context, Select, Options](Core::Async<Cursor>& Future)
				{
					Document Settings = Options;
					if (Settings.Get() != nullptr)
						Settings.SetInteger("limit", 1);
					else
						Settings = Document(BCON_NEW("limit", BCON_INT32(1)));

					Cursor Subresult = MDB_EXEC_CUR(&mongoc_collection_find_with_opts, Context, Select.Get(), Settings.Get(), nullptr);
					if (!Options.Get() && Settings.Get())
						Settings.Release();
					Select.Release();
					Options.Release();

					Future.Sync(true).Set(Subresult);
				});
#else
				return Core::Async<Cursor>::Store(nullptr);
#endif
			}
			Core::Async<Cursor> Collection::Aggregate(Query Flags, const Document& Pipeline, const Document& Options) const
			{
#ifdef TH_HAS_MONGOC
				auto* Context = Base;
				return Core::Async<Cursor>([Context, Flags, Pipeline, Options](Core::Async<Cursor>& Future)
				{
					Cursor Subresult = MDB_EXEC_CUR(&mongoc_collection_aggregate, Context, (mongoc_query_flags_t)Flags, Pipeline.Get(), Options.Get(), nullptr);
					Pipeline.Release();
					Options.Release();

					Future.Sync(true).Set(Subresult);
				});
#else
				return Core::Async<Cursor>::Store(nullptr);
#endif
			}
			const char* Collection::GetName() const
			{
#ifdef TH_HAS_MONGOC
				if (!Base)
					return nullptr;

				return mongoc_collection_get_name(Base);
#else
				return nullptr;
#endif
			}
			Stream Collection::CreateStream(const Document& Options)
			{
#ifdef TH_HAS_MONGOC
				if (!Base)
				{
					Options.Release();
					return nullptr;
				}

				TStream* Operation = mongoc_collection_create_bulk_operation_with_opts(Base, Options.Get());
				Options.Release();

				return Operation;
#else
				return nullptr;
#endif
			}
			TCollection* Collection::Get() const
			{
#ifdef TH_HAS_MONGOC
				return Base;
#else
				return nullptr;
#endif
			}

			Database::Database(TDatabase* NewBase) : Base(NewBase)
			{
			}
			void Database::Release()
			{
#ifdef TH_HAS_MONGOC
				if (!Base)
					return;

				mongoc_database_destroy(Base);
				Base = nullptr;
#endif
			}
			Core::Async<bool> Database::RemoveAllUsers()
			{
#ifdef TH_HAS_MONGOC
				auto* Context = Base;
				return Core::Async<bool>([Context](Core::Async<bool>& Future)
				{
					Future.Set(MDB_EXEC(&mongoc_database_remove_all_users, Context));
				});
#else
				return Core::Async<bool>::Store(false);
#endif
			}
			Core::Async<bool> Database::RemoveUser(const std::string& Name)
			{
#ifdef TH_HAS_MONGOC
				auto* Context = Base;
				return Core::Async<bool>([Context, Name](Core::Async<bool>& Future)
				{
					Future.Set(MDB_EXEC(&mongoc_database_remove_user, Context, Name.c_str()));
				});
#else
				return Core::Async<bool>::Store(false);
#endif
			}
			Core::Async<bool> Database::Remove()
			{
#ifdef TH_HAS_MONGOC
				auto* Context = Base;
				return Core::Async<bool>([Context](Core::Async<bool>& Future)
				{
					Future.Set(MDB_EXEC(&mongoc_database_drop, Context));
				});
#else
				return Core::Async<bool>::Store(false);
#endif
			}
			Core::Async<bool> Database::RemoveWithOptions(const Document& Options)
			{
#ifdef TH_HAS_MONGOC
				if (!Options.Get())
					return Remove();

				auto* Context = Base;
				return Core::Async<bool>([Context, Options](Core::Async<bool>& Future)
				{
					bool Subresult = MDB_EXEC(&mongoc_database_drop_with_opts, Context, Options.Get());
					Options.Release();

					Future.Set(Subresult);
				});
#else
				return Core::Async<bool>::Store(false);
#endif
			}
			Core::Async<bool> Database::AddUser(const std::string& Username, const std::string& Password, const Document& Roles, const Document& Custom)
			{
#ifdef TH_HAS_MONGOC
				auto* Context = Base;
				return Core::Async<bool>([Context, Username, Password, Roles, Custom](Core::Async<bool>& Future)
				{
					bool Subresult = MDB_EXEC(&mongoc_database_add_user, Context, Username.c_str(), Password.c_str(), Roles.Get(), Custom.Get());
					Roles.Release();
					Custom.Release();

					Future.Set(Subresult);
				});
#else
				return Core::Async<bool>::Store(false);
#endif
			}
			Core::Async<bool> Database::HasCollection(const std::string& Name) const
			{
#ifdef TH_HAS_MONGOC
				auto* Context = Base;
				return Core::Async<bool>([Context, Name](Core::Async<bool>& Future)
				{
					bson_error_t Error;
					memset(&Error, 0, sizeof(bson_error_t));
					bool Subresult = mongoc_database_has_collection(Context, Name.c_str(), &Error);
					if (!Subresult && Error.code != 0)
						TH_ERROR("[mongoc:%i] %s", (int)Error.code, Error.message);

					Future.Set(Subresult);
				});
#else
				return Core::Async<bool>::Store(false);
#endif
			}
			Core::Async<Cursor> Database::FindCollections(const Document& Options) const
			{
#ifdef TH_HAS_MONGOC
				auto* Context = Base;
				return Core::Async<Cursor>([Context, Options](Core::Async<Cursor>& Future)
				{
					Cursor Subresult = MDB_EXEC_CUR(&mongoc_database_find_collections_with_opts, Context, Options.Get());
					Options.Release();

					Future.Sync(true).Set(Subresult);
				});
#else
				return Core::Async<Cursor>::Store(nullptr);
#endif
			}
			Core::Async<Collection> Database::CreateCollection(const std::string& Name, const Document& Options)
			{
#ifdef TH_HAS_MONGOC
				if (!Base)
				{
					Options.Release();
					return Core::Async<Collection>::Store(nullptr);
				}

				auto* Context = Base;
				return Core::Async<Collection>([Context, Name, Options](Core::Async<Collection>& Future)
				{
					bson_error_t Error;
					memset(&Error, 0, sizeof(bson_error_t));

					TCollection* Collection = mongoc_database_create_collection(Context, Name.c_str(), Options.Get(), &Error);
					Options.Release();

					if (Collection == nullptr)
						TH_ERROR("[mongoc] %s", Error.message);

					Future.Set(Collection);
				});
#else
				return nullptr;
#endif
			}
			std::vector<std::string> Database::GetCollectionNames(const Document& Options) const
			{
#ifdef TH_HAS_MONGOC
				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!Base)
					return std::vector<std::string>();

				char** Names = mongoc_database_get_collection_names_with_opts(Base, Options.Get(), &Error);
				Options.Release();

				if (Names == nullptr)
				{
					TH_ERROR("[mongoc] %s", Error.message);
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
			const char* Database::GetName() const
			{
#ifdef TH_HAS_MONGOC
				if (!Base)
					return nullptr;

				return mongoc_database_get_name(Base);
#else
				return nullptr;
#endif
			}
			Collection Database::GetCollection(const std::string& Name) const
			{
#ifdef TH_HAS_MONGOC
				if (!Base)
					return nullptr;

				return mongoc_database_get_collection(Base, Name.c_str());
#else
				return nullptr;
#endif
			}
			TDatabase* Database::Get() const
			{
#ifdef TH_HAS_MONGOC
				return Base;
#else
				return nullptr;
#endif
			}

			Watcher::Watcher(TWatcher* NewBase) : Base(NewBase)
			{
			}
			void Watcher::Release()
			{
#ifdef TH_HAS_MONGOC
				if (!Base)
					return;

				mongoc_change_stream_destroy(Base);
				Base = nullptr;
#endif
			}
			Core::Async<bool> Watcher::Next(const Document& Result) const
			{
#ifdef TH_HAS_MONGOC
				if (!Base || !Result.Get())
					return Core::Async<bool>::Store(false);

				auto* Context = Base;
				return Core::Async<bool>([Context, Result](Core::Async<bool>& Future)
				{
					TDocument* Ptr = Result.Get();
					Future.Set(mongoc_change_stream_next(Context, (const TDocument**)&Ptr));
				});
#else
				return Core::Async<bool>::Store(false);
#endif
			}
			Core::Async<bool> Watcher::Error(const Document& Result) const
			{
#ifdef TH_HAS_MONGOC
				if (!Base || !Result.Get())
					return Core::Async<bool>::Store(false);

				auto* Context = Base;
				return Core::Async<bool>([Context, Result](Core::Async<bool>& Future)
				{
					TDocument* Ptr = Result.Get();
					Future.Set(mongoc_change_stream_error_document(Context, nullptr, (const TDocument**)&Ptr));
				});
#else
				return Core::Async<bool>::Store(false);
#endif
			}
			bool Watcher::NextSync(const Document& Result) const
			{
#ifdef TH_HAS_MONGOC
				if (!Base || !Result.Get())
					return false;

				TDocument* Ptr = Result.Get();
				return mongoc_change_stream_next(Base, (const TDocument**)&Ptr);
#else
				return false;
#endif
			}
			bool Watcher::ErrorSync(const Document& Result) const
			{
#ifdef TH_HAS_MONGOC
				if (!Base || !Result.Get())
					return false;

				TDocument* Ptr = Result.Get();
				return mongoc_change_stream_error_document(Base, nullptr, (const TDocument**)&Ptr);
#else
				return false;
#endif
			}
			TWatcher* Watcher::Get() const
			{
#ifdef TH_HAS_MONGOC
				return Base;
#else
				return nullptr;
#endif
			}
			Watcher Watcher::FromConnection(Connection* Connection, const Document& Pipeline, const Document& Options)
			{
#ifdef TH_HAS_MONGOC
				if (!Connection)
				{
					Pipeline.Release();
					Options.Release();
					return nullptr;
				}

				TWatcher* Stream = mongoc_client_watch(Connection->Get(), Pipeline.Get(), Options.Get());
				Pipeline.Release();
				Options.Release();

				return Stream;
#else
				return nullptr;
#endif
			}
			Watcher Watcher::FromDatabase(const Database& Database, const Document& Pipeline, const Document& Options)
			{
#ifdef TH_HAS_MONGOC
				if (!Database.Get())
				{
					Pipeline.Release();
					Options.Release();
					return nullptr;
				}

				TWatcher* Stream = mongoc_database_watch(Database.Get(), Pipeline.Get(), Options.Get());
				Pipeline.Release();
				Options.Release();

				return Stream;
#else
				return nullptr;
#endif
			}
			Watcher Watcher::FromCollection(const Collection& Collection, const Document& Pipeline, const Document& Options)
			{
#ifdef TH_HAS_MONGOC
				if (!Collection.Get())
				{
					Pipeline.Release();
					Options.Release();
					return nullptr;
				}

				TWatcher* Stream = mongoc_collection_watch(Collection.Get(), Pipeline.Get(), Options.Get());
				Pipeline.Release();
				Options.Release();

				return Stream;
#else
				return nullptr;
#endif
			}

			Transaction::Transaction(TTransaction* NewBase) : Base(NewBase)
			{
			}
			void Transaction::Release()
			{
#ifdef TH_HAS_MONGOC
				if (!Base)
					return;

				mongoc_client_session_destroy(Base);
				Base = nullptr;
#endif
			}
			bool Transaction::Push(Document& QueryOptions) const
			{
#ifdef TH_HAS_MONGOC
				if (!QueryOptions.Get())
					QueryOptions = bson_new();

				bson_error_t Error;
				bool Result = mongoc_client_session_append(Base, QueryOptions.Get(), &Error);
				if (!Result && Error.code != 0)
					TH_ERROR("[mongoc:%i] %s", (int)Error.code, Error.message);

				return Result;
#else
				return false;
#endif
			}
			Core::Async<bool> Transaction::Start()
			{
#ifdef TH_HAS_MONGOC
				auto* Context = Base;
				return Core::Async<bool>([Context](Core::Async<bool>& Future)
				{
					Future.Set(MDB_EXEC(&mongoc_client_session_start_transaction, Context, nullptr));
				});
#else
                return Core::Async<bool>::Store(false);
#endif
			}
			Core::Async<bool> Transaction::Abort()
			{
#ifdef TH_HAS_MONGOC
				auto* Context = Base;
				return Core::Async<bool>([Context](Core::Async<bool>& Future)
				{
					Future.Set(MDB_EXEC(&mongoc_client_session_abort_transaction, Context));
				});
#else
                return Core::Async<bool>::Store(false);
#endif
			}
			Core::Async<Document> Transaction::RemoveMany(const Collection& fBase, const Document& Select, const Document& fOptions)
			{
#ifdef TH_HAS_MONGOC
				Document Options = fOptions;
				if (!Push(Options))
				{
					Select.Release();
					Options.Release();
					return Core::Async<Document>(nullptr);
				}

				return Collection(fBase.Get()).RemoveMany(Select, Options);
#else
				return Core::Async<Document>(nullptr);
#endif
			}
			Core::Async<Document> Transaction::RemoveOne(const Collection& fBase, const Document& Select, const Document& fOptions)
			{
#ifdef TH_HAS_MONGOC
				Document Options = fOptions;
				if (!Push(Options))
				{
					Select.Release();
					Options.Release();
					return Core::Async<Document>(nullptr);
				}

				return Collection(fBase.Get()).RemoveOne(Select, Options);
#else
				return Core::Async<Document>(nullptr);
#endif
			}
			Core::Async<Document> Transaction::ReplaceOne(const Collection& fBase, const Document& Select, const Document& Replacement, const Document& fOptions)
			{
#ifdef TH_HAS_MONGOC
				Document Options = fOptions;
				if (!Push(Options))
				{
					Select.Release();
					Replacement.Release();
					Options.Release();
					return Core::Async<Document>(nullptr);
				}

				return Collection(fBase.Get()).ReplaceOne(Select, Replacement, Options);
#else
				return Core::Async<Document>(nullptr);
#endif
			}
			Core::Async<Document> Transaction::InsertMany(const Collection& fBase, std::vector<Document>& List, const Document& fOptions)
			{
#ifdef TH_HAS_MONGOC
				Document Options = fOptions;
				if (!Push(Options))
				{
					Options.Release();
					for (auto& Item : List)
						Item.Release();

					return Core::Async<Document>(nullptr);
				}

				return Collection(fBase.Get()).InsertMany(List, Options);
#else
				return Core::Async<Document>(nullptr);
#endif
			}
			Core::Async<Document> Transaction::InsertOne(const Collection& fBase, const Document& Result, const Document& fOptions)
			{
#ifdef TH_HAS_MONGOC
				Document Options = fOptions;
				if (!Push(Options))
				{
					Result.Release();
					Options.Release();
					return Core::Async<Document>(nullptr);
				}

				return Collection(fBase.Get()).InsertOne(Result, Options);
#else
				return Core::Async<Document>(nullptr);
#endif
			}
			Core::Async<Document> Transaction::UpdateMany(const Collection& fBase, const Document& Select, const Document& Update, const Document& fOptions)
			{
#ifdef TH_HAS_MONGOC
				Document Options = fOptions;
				if (!Push(Options))
				{
					Select.Release();
					Update.Release();
					Options.Release();
					return Core::Async<Document>(nullptr);
				}

				return Collection(fBase.Get()).UpdateMany(Select, Update, Options);
#else
				return Core::Async<Document>(nullptr);
#endif
			}
			Core::Async<Document> Transaction::UpdateOne(const Collection& fBase, const Document& Select, const Document& Update, const Document& fOptions)
			{
#ifdef TH_HAS_MONGOC
				Document Options = fOptions;
				if (!Push(Options))
				{
					Select.Release();
					Update.Release();
					Options.Release();
					return Core::Async<Document>(nullptr);
				}

				return Collection(fBase.Get()).UpdateOne(Select, Update, Options);
#else
				return Core::Async<Document>(nullptr);
#endif
			}
			Core::Async<Cursor> Transaction::FindMany(const Collection& fBase, const Document& Select, const Document& fOptions) const
			{
#ifdef TH_HAS_MONGOC
				Document Options = fOptions;
				if (!Push(Options))
				{
					Select.Release();
					Options.Release();
					return Core::Async<Cursor>(nullptr);
				}

				return Collection(fBase.Get()).FindMany(Select, Options);
#else
				return Core::Async<Cursor>(nullptr);
#endif
			}
			Core::Async<Cursor> Transaction::FindOne(const Collection& fBase, const Document& Select, const Document& fOptions) const
			{
#ifdef TH_HAS_MONGOC
				Document Options = fOptions;
				if (!Push(Options))
				{
					Select.Release();
					Options.Release();
					return Core::Async<Cursor>(nullptr);
				}

				return Collection(fBase.Get()).FindOne(Select, Options);
#else
				return Core::Async<Cursor>(nullptr);
#endif
			}
			Core::Async<Cursor> Transaction::Aggregate(const Collection& fBase, Query Flags, const Document& Pipeline, const Document& fOptions) const
			{
#ifdef TH_HAS_MONGOC
				Document Options = fOptions;
				if (!Push(Options))
				{
					Pipeline.Release();
					Options.Release();
					return Core::Async<Cursor>(nullptr);
				}

				return Collection(fBase.Get()).Aggregate(Flags, Pipeline, Options);
#else
				return Core::Async<Cursor>(nullptr);
#endif
			}
			Core::Async<Document> Transaction::Commit()
			{
#ifdef TH_HAS_MONGOC
				auto* Context = Base;
				return Core::Async<Document>([Context](Core::Async<Document>& Future)
				{
					TDocument Subresult;
					if (!MDB_EXEC(&mongoc_client_session_commit_transaction, Context, &Subresult))
					{
						bson_free(&Subresult);
						Future.Set(Document(nullptr));
					}
					else
						Future.Set(Document::FromSource(&Subresult));
				});
#else
				return Core::Async<Document>::Store(nullptr);
#endif
			}
			TTransaction* Transaction::Get() const
			{
#ifdef TH_HAS_MONGOC
				return Base;
#else
				return nullptr;
#endif
			}
			TTransaction* Transaction::FromConnection(Connection* Client)
			{
#ifdef TH_HAS_MONGOC
				if (!Client)
					return nullptr;

				bson_error_t Error;
				if (!Client->Session.Get())
				{
					Client->Session = mongoc_client_start_session(Client->Base, nullptr, &Error);
					if (!Client->Session.Get())
					{
						TH_ERROR("[mongoc] couldn't create transaction\n\t%s", Error.message);
						return nullptr;
					}
				}

				return Client->Session.Get();
#else
				return nullptr;
#endif
			}

			Connection::Connection() : Session(nullptr), Base(nullptr), Master(nullptr), Connected(false)
			{
				Driver::Create();
			}
			Connection::~Connection()
			{
				Session.Release();
				Disconnect();
				Driver::Release();
			}
			Core::Async<bool> Connection::Connect(const std::string& Address)
			{
#ifdef TH_HAS_MONGOC
				if (Master != nullptr)
					return Core::Async<bool>::Store(false);

				if (Connected)
				{
					return Disconnect().Then<Core::Async<bool>>([this, Address](bool)
					{
						return this->Connect(Address);
					});
				}

				return Core::Async<bool>([this, Address](Core::Async<bool>& Future)
				{
					bson_error_t Error;
					memset(&Error, 0, sizeof(bson_error_t));

					TAddress* URI = mongoc_uri_new_with_error(Address.c_str(), &Error);
					if (!URI)
					{
						TH_ERROR("[urierr] %s", Error.message);
						return Future.Set(false);
					}

					Base = mongoc_client_new_from_uri(URI);
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
			Core::Async<bool> Connection::Connect(Address* URL)
			{
#ifdef TH_HAS_MONGOC
				if (Master != nullptr || !URL || !URL->Get())
					return Core::Async<bool>::Store(false);

				if (Connected)
				{
					return Disconnect().Then<Core::Async<bool>>([this, URL](bool)
					{
						return this->Connect(URL);
					});
				}

				TAddress* URI = URL->Get();
				return Core::Async<bool>([this, URI](Core::Async<bool>& Future)
				{
					Base = mongoc_client_new_from_uri(URI);
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
#ifdef TH_HAS_MONGOC
				if (!Connected || !Base)
					return Core::Async<bool>::Store(false);

				return Core::Async<bool>([this](Core::Async<bool>& Future)
				{
					Connected = false;
					if (Master != nullptr)
					{
						if (Base != nullptr)
						{
							mongoc_client_pool_push(Master->Get(), Base);
							Base = nullptr;
						}

						Master = nullptr;
					}
					else if (Base != nullptr)
					{
						mongoc_client_destroy(Base);
						Base = nullptr;
					}

					Future.Set(true);
				});
#else
				return Core::Async<bool>::Store(false);
#endif
			}
			Core::Async<Cursor> Connection::FindDatabases(const Document& Options) const
			{
#ifdef TH_HAS_MONGOC
				return Core::Async<Cursor>([this, Options](Core::Async<Cursor>& Future)
				{
					TCursor* Subresult = mongoc_client_find_databases_with_opts(Base, Options.Get());
					Options.Release();

					Future.Sync(true).Set(Subresult);
				});
#else
				return Core::Async<Cursor>::Store(nullptr);
#endif
			}
			void Connection::SetProfile(const std::string& Name)
			{
#ifdef TH_HAS_MONGOC
				mongoc_client_set_appname(Base, Name.c_str());
#endif
			}
			bool Connection::SetServer(bool ForWrites)
			{
#ifdef TH_HAS_MONGOC
				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				mongoc_server_description_t* Server = mongoc_client_select_server(Base, ForWrites, nullptr, &Error);
				if (Server == nullptr)
				{
					TH_ERROR("command fail: %s", Error.message);
					return false;
				}

				mongoc_server_description_destroy(Server);
				return true;
#else
				return false;
#endif
			}
			Database Connection::GetDatabase(const std::string& Name) const
			{
#ifdef TH_HAS_MONGOC
				return mongoc_client_get_database(Base, Name.c_str());
#else
				return nullptr;
#endif
			}
			Database Connection::GetDefaultDatabase() const
			{
#ifdef TH_HAS_MONGOC
				return mongoc_client_get_default_database(Base);
#else
				return nullptr;
#endif
			}
			Collection Connection::GetCollection(const char* DatabaseName, const char* Name) const
			{
#ifdef TH_HAS_MONGOC
				return mongoc_client_get_collection(Base, DatabaseName, Name);
#else
				return nullptr;
#endif
			}
			Address Connection::GetAddress() const
			{
#ifdef TH_HAS_MONGOC
				return (TAddress*)mongoc_client_get_uri(Base);
#else
				return nullptr;
#endif
			}
			Queue* Connection::GetMaster() const
			{
				return Master;
			}
			TConnection* Connection::Get() const
			{
				return Base;
			}
			std::vector<std::string> Connection::GetDatabaseNames(const Document& Options) const
			{
#ifdef TH_HAS_MONGOC
				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				char** Names = mongoc_client_get_database_names_with_opts(Base, Options.Get(), &Error);
				Options.Release();

				if (Names == nullptr)
				{
					TH_ERROR("[mongoc] %s", Error.message);
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
			bool Connection::IsConnected() const
			{
				return Connected;
			}

			Queue::Queue() : Pool(nullptr), SrcAddress(nullptr), Connected(false)
			{
				Driver::Create();
			}
			Queue::~Queue()
			{
				Disconnect();
				Driver::Release();
			}
			Core::Async<bool> Queue::Connect(const std::string& URI)
			{
#ifdef TH_HAS_MONGOC
				if (Connected)
				{
					return Disconnect().Then<Core::Async<bool>>([this, URI](bool)
					{
						return this->Connect(URI);
					});
				}

				return Core::Async<bool>([this, URI](Core::Async<bool>& Future)
				{
					bson_error_t Error;
					memset(&Error, 0, sizeof(bson_error_t));

					SrcAddress = mongoc_uri_new_with_error(URI.c_str(), &Error);
					if (!SrcAddress.Get())
					{
						TH_ERROR("[urierr] %s", Error.message);
						return Future.Set(false);
					}

					Pool = mongoc_client_pool_new(SrcAddress.Get());
					if (!Pool)
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
			Core::Async<bool> Queue::Connect(Address* URI)
			{
#ifdef TH_HAS_MONGOC
				if (!URI || !URI->Get())
					return Core::Async<bool>::Store(false);

				if (Connected)
				{
					return Disconnect().Then<Core::Async<bool>>([this, URI](bool)
					{
						return this->Connect(URI);
					});
				}

				TAddress* Context = URI->Get();
				*URI = nullptr;

				return Core::Async<bool>([this, Context](Core::Async<bool>& Future)
				{
					SrcAddress = Context;
					Pool = mongoc_client_pool_new(SrcAddress.Get());
					if (!Pool)
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
			Core::Async<bool> Queue::Disconnect()
			{
#ifdef TH_HAS_MONGOC
				if (!Connected || !Pool)
					return Core::Async<bool>::Store(false);

				return Core::Async<bool>([this](Core::Async<bool>& Future)
				{
					if (Pool != nullptr)
					{
						mongoc_client_pool_destroy(Pool);
						Pool = nullptr;
					}

					SrcAddress.Release();
					Connected = false;
				});
#else
				return Core::Async<bool>::Store(false);
#endif
			}
			void Queue::SetProfile(const char* Name)
			{
#ifdef TH_HAS_MONGOC
				mongoc_client_pool_set_appname(Pool, Name);
#endif
			}
			void Queue::Push(Connection** Client)
			{
#ifdef TH_HAS_MONGOC
				if (!Client || !*Client)
					return;

				mongoc_client_pool_push(Pool, (*Client)->Base);
				(*Client)->Base = nullptr;
				(*Client)->Connected = false;
				(*Client)->Master = nullptr;

				TH_RELEASE(*Client);
				*Client = nullptr;
#endif
			}
			Connection* Queue::Pop()
			{
#ifdef TH_HAS_MONGOC
				TConnection* Base = mongoc_client_pool_pop(Pool);
				if (!Base)
					return nullptr;

				Connection* Result = new Connection();
				Result->Base = Base;
				Result->Connected = true;
				Result->Master = this;

				return Result;
#else
				return nullptr;
#endif
			}
			TConnectionPool* Queue::Get() const
			{
				return Pool;
			}
			Address Queue::GetAddress() const
			{
				return SrcAddress;
			}

			Driver::Sequence::Sequence() : Cache(nullptr)
			{
			}

			void Driver::Create()
			{
#ifdef TH_HAS_MONGOC
				if (State <= 0)
				{
					Queries = new std::unordered_map<std::string, Sequence>();
					Safe = TH_NEW(std::mutex);
					mongoc_log_set_handler([](mongoc_log_level_t Level, const char* Domain, const char* Message, void*)
					{
						switch (Level)
						{
							case MONGOC_LOG_LEVEL_ERROR:
								TH_ERROR("[mongoc] [%s] %s", Domain, Message);
								break;
							case MONGOC_LOG_LEVEL_WARNING:
								TH_WARN("[mongoc] [%s] %s", Domain, Message);
								break;
							case MONGOC_LOG_LEVEL_INFO:
								TH_INFO("[mongoc] [%s] %s", Domain, Message);
								break;
							case MONGOC_LOG_LEVEL_CRITICAL:
								TH_ERROR("[mongocerr] [%s] %s", Domain, Message);
								break;
							case MONGOC_LOG_LEVEL_MESSAGE:
								TH_LOG("[mongoc] [%s] %s", Domain, Message);
								break;
							default:
								break;
						}
					}, nullptr);
					mongoc_init();
					State = 1;
				}
				else
					State++;
#endif
			}
			void Driver::Release()
			{
#ifdef TH_HAS_MONGOC
				if (State == 1)
				{
					if (Safe != nullptr)
						Safe->lock();

					State = 0;
					if (Queries != nullptr)
					{
						for (auto& Query : *Queries)
							Query.second.Cache.Release();

						delete Queries;
						Queries = nullptr;
					}

					if (Safe != nullptr)
					{
						Safe->unlock();
						TH_DELETE(mutex, Safe);
						Safe = nullptr;
					}

					mongoc_cleanup();
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
								Index -= Index - Arg + 1; Args++;
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
					Result.Cache = Document::FromJSON(Result.Request);

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

					if (!Base.EndsWith(".json"))
						continue;

					char* Buffer = (char*)Core::OS::File::ReadAll(Base.Get(), &Size);
					if (!Buffer)
						continue;

					Base.Replace(Origin.empty() ? Directory : Origin, "").Replace("\\", "/").Replace(".json", "");
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

				It->second.Cache.Release();
				Queries->erase(It);
				Safe->unlock();
				return true;
			}
			Document Driver::GetQuery(const std::string& Name, Core::DocumentArgs* Map, bool Once)
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

				if (It->second.Cache.Get() != nullptr)
				{
					Document Result = It->second.Cache.Copy();
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
					Document Result = Document::FromJSON(It->second.Request);
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

					std::string Value = GetJSON(It->second, Word.Escape);
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

				Document Data = Document::FromJSON(Origin.Request);
				if (!Data.Get())
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
			std::string Driver::GetJSON(Core::Document* Source, bool Escape)
			{
				if (!Source)
					return "";

				switch (Source->Value.GetType())
				{
					case Core::VarType::Object:
					{
						std::string Result = "{";
						for (auto* Node : *Source->GetNodes())
						{
							Result.append(1, '\"').append(Node->Key).append("\":");
							Result.append(GetJSON(Node, true)).append(1, ',');
						}

						if (!Source->GetNodes()->empty())
							Result = Result.substr(0, Result.size() - 1);

						return Result + "}";
					}
					case Core::VarType::Array:
					{
						std::string Result = "[";
						for (auto* Node : *Source->GetNodes())
							Result.append(GetJSON(Node, true)).append(1, ',');

						if (!Source->GetNodes()->empty())
							Result = Result.substr(0, Result.size() - 1);

						return Result + "]";
					}
					case Core::VarType::String:
					{
						std::string Result = Source->Value.GetBlob();
						if (!Escape)
							return Result;

						Result.insert(Result.begin(), '\"');
						Result.append(1, '\"');
						return Result;
					}
					case Core::VarType::Integer:
						return std::to_string(Source->Value.GetInteger());
					case Core::VarType::Number:
						return std::to_string(Source->Value.GetNumber());
					case Core::VarType::Boolean:
						return Source->Value.GetBoolean() ? "true" : "false";
					case Core::VarType::Decimal:
						return "{\"$numberDouble\":\"" + Source->Value.GetDecimal().ToString() + "\"}";
					case Core::VarType::Base64:
					{
						if (Source->Value.GetSize() != 12)
						{
							std::string Base = Compute::Common::Base64Encode(Source->Value.GetBlob());
							return "\"" + Base + "\"";
						}

						return "{\"$oid\":\"" + Util::IdToString(Source->Value.GetBase64()) + "\"}";
					}
					case Core::VarType::Null:
					case Core::VarType::Undefined:
						return "null";
					default:
						break;
				}

				return "";
			}
			std::unordered_map<std::string, Driver::Sequence>* Driver::Queries = nullptr;
			std::mutex* Driver::Safe = nullptr;
			int Driver::State = 0;
		}
	}
}
