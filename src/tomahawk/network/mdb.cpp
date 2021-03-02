#include "mdb.h"
extern "C"
{
#ifdef TH_HAS_MONGOC
#include <mongoc.h>
#define MDB_POP(V) (V ? V->Get() : nullptr)
#define MDB_FREE(V) { if (V != nullptr) V->Release(); }
#endif
}

namespace Tomahawk
{
	namespace Network
	{
		namespace MDB
		{
			Property::Property() : Object(nullptr), Array(nullptr), Mod(Type_Unknown), Integer(0), High(0), Low(0), Number(0.0), Boolean(false), IsValid(false)
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
				Mod = Type_Unknown;
				IsValid = false;
			}
			std::string& Property::ToString()
			{
				switch (Mod)
				{
					case Type_Document:
						return String.assign("{}");
					case Type_Array:
						return String.assign("[]");
					case Type_String:
						return String;
					case Type_Boolean:
						return String.assign(Boolean ? "true" : "false");
					case Type_Number:
						return String.assign(std::to_string(Number));
					case Type_Integer:
						return String.assign(std::to_string(Integer));
					case Type_ObjectId:
						return String.assign(Compute::Common::Base64Encode((const char*)ObjectId));
					case Type_Null:
						return String.assign("null");
					case Type_Unknown:
					case Type_Uncastable:
						return String.assign("undefined");
					case Type_Decimal:
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

			Document::Document(TDocument* NewBase) : Base(NewBase)
			{
			}
			void Document::Release()
			{
#ifdef TH_HAS_MONGOC
				if (!Base)
					return;

				if (Base->flags == 2)
				{
					bson_destroy(Base);
					bson_free(Base);
				}
				else
					bson_destroy(Base);
				Base = nullptr;
#endif
			}
			void Document::Join(const Document& Value)
			{
#ifdef TH_HAS_MONGOC
				if (Base != nullptr && Value.Base != nullptr)
					bson_concat(Base, Value.Base);
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
			bool Document::SetDocument(const char* Key, Document* Value, uint64_t ArrayId)
			{
#ifdef TH_HAS_MONGOC
				if (!Value || !Value->Base || !Base)
					return false;

				char Index[16];
				if (Key == nullptr)
					bson_uint32_to_string((uint32_t)ArrayId, &Key, Index, sizeof(Index));

				bool Result = bson_append_document(Base, Key, -1, Value->Base);
				Value->Release();

				return Result;
#else
				return false;
#endif
			}
			bool Document::SetArray(const char* Key, Document* Value, uint64_t ArrayId)
			{
#ifdef TH_HAS_MONGOC
				if (!Value || !Value->Base || !Base)
					return false;

				char Index[16];
				if (Key == nullptr)
					bson_uint32_to_string((uint32_t)ArrayId, &Key, Index, sizeof(Index));

				bool Result = bson_append_array(Base, Key, -1, Value->Base);
				Value->Release();

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

				Document Object(nullptr);
				switch (Value->Mod)
				{
					case Type_Document:
						Object = Document(Value->Object).Copy();
						return SetDocument(Key, &Object);
					case Type_Array:
						Object = Document(Value->Array).Copy();
						return SetArray(Key, &Object);
					case Type_String:
						return SetString(Key, Value->String.c_str());
					case Type_Boolean:
						return SetBoolean(Key, Value->Boolean);
					case Type_Number:
						return SetNumber(Key, Value->Number);
					case Type_Integer:
						return SetInteger(Key, Value->Integer);
					case Type_Decimal:
						return SetDecimal(Key, Value->High, Value->Low);
					case Type_ObjectId:
						return SetObjectId(Key, Value->ObjectId);
					default:
						break;
				}

				return false;
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
						Output->Mod = Type_Document;
						Output->Object = Document::FromBuffer((const unsigned char*)Value->value.v_doc.data, (uint64_t)Value->value.v_doc.data_len).Get();
						break;
					case BSON_TYPE_ARRAY:
						Output->Mod = Type_Array;
						Output->Array = Document::FromBuffer((const unsigned char*)Value->value.v_doc.data, (uint64_t)Value->value.v_doc.data_len).Get();
						break;
					case BSON_TYPE_BOOL:
						Output->Mod = Type_Boolean;
						Output->Boolean = Value->value.v_bool;
						break;
					case BSON_TYPE_INT32:
						Output->Mod = Type_Integer;
						Output->Integer = Value->value.v_int32;
						break;
					case BSON_TYPE_INT64:
						Output->Mod = Type_Integer;
						Output->Integer = Value->value.v_int64;
						break;
					case BSON_TYPE_DOUBLE:
						Output->Mod = Type_Number;
						Output->Number = Value->value.v_double;
						break;
					case BSON_TYPE_DECIMAL128:
						Output->Mod = Type_Decimal;
						Output->High = (uint64_t)Value->value.v_decimal128.high;
						Output->Low = (uint64_t)Value->value.v_decimal128.low;
						break;
					case BSON_TYPE_UTF8:
						Output->Mod = Type_String;
						Output->String.assign(Value->value.v_utf8.str, (uint64_t)Value->value.v_utf8.len);
						break;
					case BSON_TYPE_TIMESTAMP:
						Output->Mod = Type_Integer;
						Output->Integer = (int64_t)Value->value.v_timestamp.timestamp;
						Output->Number = (double)Value->value.v_timestamp.increment;
						break;
					case BSON_TYPE_DATE_TIME:
						Output->Mod = Type_Integer;
						Output->Integer = Value->value.v_datetime;
						break;
					case BSON_TYPE_REGEX:
						Output->Mod = Type_String;
						Output->String.assign(Value->value.v_regex.regex).append(1, '\n').append(Value->value.v_regex.options);
						break;
					case BSON_TYPE_CODE:
						Output->Mod = Type_String;
						Output->String.assign(Value->value.v_code.code, (uint64_t)Value->value.v_code.code_len);
						break;
					case BSON_TYPE_SYMBOL:
						Output->Mod = Type_String;
						Output->String.assign(Value->value.v_symbol.symbol, (uint64_t)Value->value.v_symbol.len);
						break;
					case BSON_TYPE_CODEWSCOPE:
						Output->Mod = Type_String;
						Output->String.assign(Value->value.v_codewscope.code, (uint64_t)Value->value.v_codewscope.code_len);
						break;
					case BSON_TYPE_UNDEFINED:
					case BSON_TYPE_NULL:
						Output->Mod = Type_Null;
						break;
					case BSON_TYPE_OID:
						Output->Mod = Type_ObjectId;
						memcpy(Output->ObjectId, Value->value.v_oid.bytes, sizeof(unsigned char) * 12);
						break;
					case BSON_TYPE_EOD:
					case BSON_TYPE_BINARY:
					case BSON_TYPE_DBPOINTER:
					case BSON_TYPE_MAXKEY:
					case BSON_TYPE_MINKEY:
						Output->Mod = Type_Uncastable;
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
			Rest::Document* Document::ToDocument(bool IsArray) const
			{
#ifdef TH_HAS_MONGOC
				if (!Base)
					return nullptr;

				Rest::Document* Node = (IsArray ? Rest::Document::Array() : Rest::Document::Object());
				Loop([Node](Property* Key) -> bool
				{
					std::string Name = (Node->Value.GetType() == Rest::VarType_Array ? "" : Key->Name);
					switch (Key->Mod)
					{
						case Type_Document:
						{
							Node->Set(Name, Document(Key->Object).ToDocument(false));
							break;
						}
						case Type_Array:
						{
							Node->Set(Name, Document(Key->Array).ToDocument(true));
							break;
						}
						case Type_String:
							Node->Set(Name, Rest::Var::String(Key->String));
							break;
						case Type_Boolean:
							Node->Set(Name, Rest::Var::Boolean(Key->Boolean));
							break;
						case Type_Number:
							Node->Set(Name, Rest::Var::Number(Key->Number));
							break;
						case Type_Decimal:
						{
							double Decimal = Rest::Stroke(&Key->ToString()).ToDouble();
							if (Decimal != (int64_t)Decimal)
								Node->Set(Name, Rest::Var::Number(Decimal));
							else
								Node->Set(Name, Rest::Var::Integer((int64_t)Decimal));
							break;
						}
						case Type_Integer:
							Node->Set(Name, Rest::Var::Integer(Key->Integer));
							break;
						case Type_ObjectId:
							Node->Set(Name, Rest::Var::Base64(Key->ObjectId, 12));
							break;
						case Type_Null:
							Node->Set(Name, Rest::Var::Null());
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
			Document Document::FromDocument(Rest::Document* Src)
			{
#ifdef TH_HAS_MONGOC
				if (!Src || !Src->Value.IsObject())
					return nullptr;

				bool Array = (Src->Value.GetType() == Rest::VarType_Array);
				Document Result = bson_new();
				Document Replica = nullptr;
				uint64_t Index = 0;

				for (auto&& Node : *Src->GetNodes())
				{
					switch (Node->Value.GetType())
					{
						case Rest::VarType_Object:
							Replica = Document::FromDocument(Node);
							Result.SetDocument(Array ? nullptr : Node->Key.c_str(), &Replica, Index);
							break;
						case Rest::VarType_Array:
							Replica = Document::FromDocument(Node);
							Result.SetArray(Array ? nullptr : Node->Key.c_str(), &Replica, Index);
							break;
						case Rest::VarType_String:
							Result.SetBlob(Array ? nullptr : Node->Key.c_str(), Node->Value.GetString(), Node->Value.GetSize(), Index);
							break;
						case Rest::VarType_Boolean:
							Result.SetBoolean(Array ? nullptr : Node->Key.c_str(), Node->Value.GetBoolean(), Index);
							break;
						case Rest::VarType_Decimal:
							Result.SetDecimalString(Array ? nullptr : Node->Key.c_str(), Node->Value.GetDecimal(), Index);
							break;
						case Rest::VarType_Number:
							Result.SetNumber(Array ? nullptr : Node->Key.c_str(), Node->Value.GetNumber(), Index);
							break;
						case Rest::VarType_Integer:
							Result.SetInteger(Array ? nullptr : Node->Key.c_str(), Node->Value.GetInteger(), Index);
							break;
						case Rest::VarType_Null:
							Result.SetNull(Array ? nullptr : Node->Key.c_str(), Index);
							break;
						case Rest::VarType_Base64:
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
				}

				return Result;
#else
				return nullptr;
#endif
			}
			Document Document::FromEmpty(bool Array)
			{
#ifdef TH_HAS_MONGOC
				if (Array)
					return BCON_NEW("pipeline", "[", "]");
				
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
			bool Stream::RemoveMany(Document* Selector, Document* Options)
			{
#ifdef TH_HAS_MONGOC
				if (!Base)
				{
					MDB_FREE(Selector);
					MDB_FREE(Options);
					return false;
				}

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!mongoc_bulk_operation_remove_many_with_opts(Base, MDB_POP(Selector), MDB_POP(Options), &Error))
				{
					MDB_FREE(Selector);
					MDB_FREE(Options);
					TH_ERROR("[mongoc] %s", Error.message);
					return false;
				}

				MDB_FREE(Selector);
				MDB_FREE(Options);
				return true;
#else
				return false;
#endif
			}
			bool Stream::RemoveOne(Document* Selector, Document* Options)
			{
#ifdef TH_HAS_MONGOC
				if (!Base)
				{
					MDB_FREE(Selector);
					MDB_FREE(Options);
					return false;
				}

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!mongoc_bulk_operation_remove_one_with_opts(Base, MDB_POP(Selector), MDB_POP(Options), &Error))
				{
					MDB_FREE(Selector);
					MDB_FREE(Options);
					TH_ERROR("[mongoc] %s", Error.message);
					return false;
				}

				MDB_FREE(Selector);
				MDB_FREE(Options);
				return true;
#else
				return false;
#endif
			}
			bool Stream::ReplaceOne(Document* Selector, Document* Replacement, Document* Options)
			{
#ifdef TH_HAS_MONGOC
				if (!Base)
				{
					MDB_FREE(Selector);
					MDB_FREE(Replacement);
					MDB_FREE(Options);
					return false;
				}

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!mongoc_bulk_operation_replace_one_with_opts(Base, MDB_POP(Selector), MDB_POP(Replacement), MDB_POP(Options), &Error))
				{
					MDB_FREE(Selector);
					MDB_FREE(Replacement);
					MDB_FREE(Options);
					TH_ERROR("[mongoc] %s", Error.message);
					return false;
				}

				MDB_FREE(Selector);
				MDB_FREE(Replacement);
				MDB_FREE(Options);
				return true;
#else
				return false;
#endif
			}
			bool Stream::Insert(Document* Result, Document* Options)
			{
#ifdef TH_HAS_MONGOC
				if (!Base)
				{
					MDB_FREE(Result);
					MDB_FREE(Options);
					return false;
				}

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!mongoc_bulk_operation_insert_with_opts(Base, MDB_POP(Result), MDB_POP(Options), &Error))
				{
					MDB_FREE(Result);
					MDB_FREE(Options);
					TH_ERROR("[mongoc] %s", Error.message);
					return false;
				}

				MDB_FREE(Result);
				MDB_FREE(Options);
				return true;
#else
				return false;
#endif
			}
			bool Stream::UpdateOne(Document* Selector, Document* Result, Document* Options)
			{
#ifdef TH_HAS_MONGOC
				if (!Base)
				{
					MDB_FREE(Selector);
					MDB_FREE(Result);
					MDB_FREE(Options);
					return false;
				}

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!mongoc_bulk_operation_update_one_with_opts(Base, MDB_POP(Selector), MDB_POP(Result), MDB_POP(Options), &Error))
				{
					MDB_FREE(Selector);
					MDB_FREE(Result);
					MDB_FREE(Options);
					TH_ERROR("[mongoc] %s", Error.message);
					return false;
				}

				MDB_FREE(Selector);
				MDB_FREE(Result);
				MDB_FREE(Options);
				return true;
#else
				return false;
#endif
			}
			bool Stream::UpdateMany(Document* Selector, Document* Result, Document* Options)
			{
#ifdef TH_HAS_MONGOC
				if (!Base)
				{
					MDB_FREE(Selector);
					MDB_FREE(Result);
					MDB_FREE(Options);
					return false;
				}

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!mongoc_bulk_operation_update_many_with_opts(Base, MDB_POP(Selector), MDB_POP(Result), MDB_POP(Options), &Error))
				{
					MDB_FREE(Selector);
					MDB_FREE(Result);
					MDB_FREE(Options);
					TH_ERROR("[mongoc] %s", Error.message);
					return false;
				}

				MDB_FREE(Selector);
				MDB_FREE(Result);
				MDB_FREE(Options);
				return true;
#else
				return false;
#endif
			}
			bool Stream::RemoveMany(const Document& Selector, const Document& Options)
			{
#ifdef TH_HAS_MONGOC
				if (!Base)
					return false;

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!mongoc_bulk_operation_remove_many_with_opts(Base, Selector.Get(), Options.Get(), &Error))
				{
					TH_ERROR("[mongoc] %s", Error.message);
					return false;
				}

				return true;
#else
				return false;
#endif
			}
			bool Stream::RemoveOne(const Document& Selector, const Document& Options)
			{
#ifdef TH_HAS_MONGOC
				if (!Base)
					return false;

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!mongoc_bulk_operation_remove_one_with_opts(Base, Selector.Get(), Options.Get(), &Error))
				{
					TH_ERROR("[mongoc] %s", Error.message);
					return false;
				}

				return true;
#else
				return false;
#endif
			}
			bool Stream::ReplaceOne(const Document& Selector, const Document& Replacement, const Document& Options)
			{
#ifdef TH_HAS_MONGOC
				if (!Base)
					return false;

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!mongoc_bulk_operation_replace_one_with_opts(Base, Selector.Get(), Replacement.Get(), Options.Get(), &Error))
				{
					TH_ERROR("[mongoc] %s", Error.message);
					return false;
				}

				return true;
#else
				return false;
#endif
			}
			bool Stream::Insert(const Document& Result, const Document& Options)
			{
#ifdef TH_HAS_MONGOC
				if (!Base)
					return false;

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!mongoc_bulk_operation_insert_with_opts(Base, Result.Get(), Options.Get(), &Error))
				{
					TH_ERROR("[mongoc] %s", Error.message);
					return false;
				}

				return true;
#else
				return false;
#endif
			}
			bool Stream::UpdateOne(const Document& Selector, const Document& Result, const Document& Options)
			{
#ifdef TH_HAS_MONGOC
				if (!Base)
					return false;

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!mongoc_bulk_operation_update_one_with_opts(Base, Selector.Get(), Result.Get(), Options.Get(), &Error))
				{
					TH_ERROR("[mongoc] %s", Error.message);
					return false;
				}

				return true;
#else
				return false;
#endif
			}
			bool Stream::UpdateMany(const Document& Selector, const Document& Result, const Document& Options)
			{
#ifdef TH_HAS_MONGOC
				if (!Base)
					return false;

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!mongoc_bulk_operation_update_many_with_opts(Base, Selector.Get(), Result.Get(), Options.Get(), &Error))
				{
					TH_ERROR("[mongoc] %s", Error.message);
					return false;
				}

				return true;
#else
				return false;
#endif
			}
			bool Stream::Execute()
			{
#ifdef TH_HAS_MONGOC
				if (!Base)
					return false;

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				TDocument Result;
				if (!mongoc_bulk_operation_execute(Base, &Result, &Error))
				{
					Release();
					bson_destroy(&Result);
					return false;
				}

				Release();
				bson_destroy(&Result);
				return true;
#else
				return false;
#endif
			}
			bool Stream::Execute(Document* Reply)
			{
#ifdef TH_HAS_MONGOC
				if (!MDB_POP(Reply))
					return Execute();

				if (!Base)
					return false;

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!mongoc_bulk_operation_execute(Base, MDB_POP(Reply), &Error))
				{
					Release();
					return false;
				}

				Release();
				return true;
#else
				return false;
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
			void Cursor::Receive(const std::function<bool(const Document&)>& Callback) const
			{
#ifdef TH_HAS_MONGOC
				if (!Callback || !Base)
					return;

				TDocument* Query = nullptr;
				while (mongoc_cursor_next(Base, (const TDocument**)&Query))
				{
					if (!Callback(Query))
						break;
				}
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
			bool Cursor::Next() const
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
			TCursor* Cursor::Get() const
			{
#ifdef TH_HAS_MONGOC
				return Base;
#else
				return nullptr;
#endif
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
			bool Collection::UpdateMany(Document* Selector, Document* Update, Document* Options, Document* Reply)
			{
#ifdef TH_HAS_MONGOC
				if (!Base)
				{
					MDB_FREE(Selector);
					MDB_FREE(Update);
					MDB_FREE(Options);
					return false;
				}

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!mongoc_collection_update_many(Base, MDB_POP(Selector), MDB_POP(Update), MDB_POP(Options), MDB_POP(Reply), &Error))
				{
					MDB_FREE(Selector);
					MDB_FREE(Update);
					MDB_FREE(Options);
					return false;
				}

				MDB_FREE(Selector);
				MDB_FREE(Update);
				MDB_FREE(Options);
				return true;
#else
				return false;
#endif
			}
			bool Collection::UpdateOne(Document* Selector, Document* Update, Document* Options, Document* Reply)
			{
#ifdef TH_HAS_MONGOC
				if (!Base)
				{
					MDB_FREE(Selector);
					MDB_FREE(Update);
					MDB_FREE(Options);
					return false;
				}

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!mongoc_collection_update_one(Base, MDB_POP(Selector), MDB_POP(Update), MDB_POP(Options), MDB_POP(Reply), &Error))
				{
					MDB_FREE(Selector);
					MDB_FREE(Update);
					MDB_FREE(Options);
					TH_ERROR("[mongoc] %s", Error.message);
					return false;
				}

				MDB_FREE(Selector);
				MDB_FREE(Update);
				MDB_FREE(Options);
				return true;
#else
				return false;
#endif
			}
			bool Collection::Rename(const char* NewDatabaseName, const char* NewCollectionName)
			{
#ifdef TH_HAS_MONGOC
				if (!Base)
					return false;

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!mongoc_collection_rename(Base, NewDatabaseName, NewCollectionName, false, &Error))
				{
					TH_ERROR("[mongoc] %s", Error.message);
					return false;
				}

				return true;
#else
				return false;
#endif
			}
			bool Collection::RenameWithOptions(const char* NewDatabaseName, const char* NewCollectionName, Document* Options)
			{
#ifdef TH_HAS_MONGOC
				if (!Base)
				{
					MDB_FREE(Options);
					return false;
				}

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!mongoc_collection_rename_with_opts(Base, NewDatabaseName, NewCollectionName, false, MDB_POP(Options), &Error))
				{
					MDB_FREE(Options);
					TH_ERROR("[mongoc] %s", Error.message);
					return false;
				}

				MDB_FREE(Options);
				return true;
#else
				return false;
#endif
			}
			bool Collection::RenameWithRemove(const char* NewDatabaseName, const char* NewCollectionName)
			{
#ifdef TH_HAS_MONGOC
				if (!Base)
					return false;

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!mongoc_collection_rename(Base, NewDatabaseName, NewCollectionName, true, &Error))
				{
					TH_ERROR("[mongoc] %s", Error.message);
					return false;
				}

				return true;
#else
				return false;
#endif
			}
			bool Collection::RenameWithOptionsAndRemove(const char* NewDatabaseName, const char* NewCollectionName, Document* Options)
			{
#ifdef TH_HAS_MONGOC
				if (!Base)
				{
					MDB_FREE(Options);
					return false;
				}

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!mongoc_collection_rename_with_opts(Base, NewDatabaseName, NewCollectionName, true, MDB_POP(Options), &Error))
				{
					MDB_FREE(Options);
					TH_ERROR("[mongoc] %s", Error.message);
					return false;
				}

				MDB_FREE(Options);
				return true;
#else
				return false;
#endif
			}
			bool Collection::Remove(Document* Options)
			{
#ifdef TH_HAS_MONGOC
				if (!Base)
				{
					MDB_FREE(Options);
					return false;
				}

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!mongoc_collection_drop_with_opts(Base, MDB_POP(Options), &Error))
				{
					MDB_FREE(Options);
					TH_ERROR("[mongoc] %s", Error.message);
					return false;
				}

				MDB_FREE(Options);
				return true;
#else
				return false;
#endif
			}
			bool Collection::RemoveMany(Document* Selector, Document* Options, Document* Reply)
			{
#ifdef TH_HAS_MONGOC
				if (!Base)
				{
					MDB_FREE(Selector);
					MDB_FREE(Options);
					return false;
				}

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!mongoc_collection_delete_many(Base, MDB_POP(Selector), MDB_POP(Options), MDB_POP(Reply), &Error))
				{
					MDB_FREE(Selector);
					MDB_FREE(Options);
					TH_ERROR("[mongoc] %s", Error.message);
					return false;
				}

				MDB_FREE(Selector);
				MDB_FREE(Options);
				return true;
#else
				return false;
#endif
			}
			bool Collection::RemoveOne(Document* Selector, Document* Options, Document* Reply)
			{
#ifdef TH_HAS_MONGOC
				if (!Base)
				{
					MDB_FREE(Selector);
					MDB_FREE(Options);
					return false;
				}

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!mongoc_collection_delete_one(Base, MDB_POP(Selector), MDB_POP(Options), MDB_POP(Reply), &Error))
				{
					MDB_FREE(Selector);
					MDB_FREE(Options);
					TH_ERROR("[mongoc] %s", Error.message);
					return false;
				}

				MDB_FREE(Selector);
				MDB_FREE(Options);
				return true;
#else
				return false;
#endif
			}
			bool Collection::RemoveIndex(const char* Name, Document* Options)
			{
#ifdef TH_HAS_MONGOC
				if (!Base)
				{
					MDB_FREE(Options);
					return false;
				}

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!mongoc_collection_drop_index_with_opts(Base, Name, MDB_POP(Options), &Error))
				{
					MDB_FREE(Options);
					TH_ERROR("[mongoc] %s", Error.message);
					return false;
				}

				MDB_FREE(Options);
				return true;
#else
				return false;
#endif
			}
			bool Collection::ReplaceOne(Document* Selector, Document* Replacement, Document* Options, Document* Reply)
			{
#ifdef TH_HAS_MONGOC
				if (!Base)
				{
					MDB_FREE(Selector);
					MDB_FREE(Replacement);
					MDB_FREE(Options);
					return false;
				}

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!mongoc_collection_replace_one(Base, MDB_POP(Selector), MDB_POP(Replacement), MDB_POP(Options), MDB_POP(Reply), &Error))
				{
					MDB_FREE(Selector);
					MDB_FREE(Replacement);
					MDB_FREE(Options);
					TH_ERROR("[mongoc] %s", Error.message);
					return false;
				}

				MDB_FREE(Selector);
				MDB_FREE(Replacement);
				MDB_FREE(Options);
				return true;
#else
				return false;
#endif
			}
			bool Collection::InsertMany(std::vector<Document>& List, Document* Options, Document* Reply)
			{
#ifdef TH_HAS_MONGOC
				if (!Base || List.empty())
				{
					MDB_FREE(Options);
					return false;
				}

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				TDocument** Array = (TDocument**)TH_MALLOC(sizeof(TDocument*) * List.size());
				for (size_t i = 0; i < List.size(); i++)
					Array[i] = List[i].Get();

				bool Subresult = mongoc_collection_insert_many(Base, (const TDocument**)Array, (size_t)List.size(), MDB_POP(Options), MDB_POP(Reply), &Error);
				if (!Subresult)
					TH_ERROR("[mongoc] %s", Error.message);

				for (auto& Item : List)
					Item.Release();

				MDB_FREE(Options);
				List.clear();
				return true;
#else
				return false;
#endif
			}
			bool Collection::InsertOne(Document* Result, Document* Options, Document* Reply)
			{
#ifdef TH_HAS_MONGOC
				if (!Base)
				{
					MDB_FREE(Options);
					MDB_FREE(Result);
					return false;
				}

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!mongoc_collection_insert_one(Base, MDB_POP(Result), MDB_POP(Options), MDB_POP(Reply), &Error))
				{
					MDB_FREE(Options);
					MDB_FREE(Result);
					TH_ERROR("[mongoc] %s", Error.message);
					return false;
				}

				MDB_FREE(Options);
				MDB_FREE(Result);
				return true;
#else
				return false;
#endif
			}
			bool Collection::FindAndModify(Document* Query, Document* Sort, Document* Update, Document* Fields, Document* Reply, bool RemoveAt, bool Upsert, bool New)
			{
#ifdef TH_HAS_MONGOC
				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!Base)
				{
					MDB_FREE(Query);
					MDB_FREE(Sort);
					MDB_FREE(Update);
					MDB_FREE(Fields);
					return false;
				}

				if (!mongoc_collection_find_and_modify(Base, MDB_POP(Query), MDB_POP(Sort), MDB_POP(Update), MDB_POP(Fields), RemoveAt, Upsert, New, MDB_POP(Reply), &Error))
				{
					MDB_FREE(Query);
					MDB_FREE(Sort);
					MDB_FREE(Update);
					MDB_FREE(Fields);
					TH_ERROR("[mongoc] %s", Error.message);
					return false;
				}

				MDB_FREE(Query);
				MDB_FREE(Sort);
				MDB_FREE(Update);
				MDB_FREE(Fields);
				return true;
#else
				return false;
#endif
			}
			uint64_t Collection::CountElementsInArray(Document* Match, Document* Filter, Document* Options) const
			{
#ifdef TH_HAS_MONGOC
				if (!Base || !MDB_POP(Filter) || !MDB_POP(Match))
				{
					MDB_FREE(Filter);
					MDB_FREE(Match);
					MDB_FREE(Options);
					return 0;
				}

				Document Pipeline = BCON_NEW("pipeline", "[", "{", "$match", BCON_DOCUMENT(MDB_POP(Match)), "}", "{", "$project", "{", "Count", "{", "$Count", "{", "$filter", BCON_DOCUMENT(MDB_POP(Filter)), "}", "}", "}", "}", "]");
				if (!Pipeline.Get())
				{
					MDB_FREE(Filter);
					MDB_FREE(Match);
					MDB_FREE(Options);
					return 0;
				}

				Cursor Cursor = mongoc_collection_aggregate(Base, MONGOC_QUERY_NONE, Pipeline.Get(), MDB_POP(Options), nullptr);
				Pipeline.Release();
				MDB_FREE(Filter);
				MDB_FREE(Match);
				MDB_FREE(Options);

				if (!Cursor.Get() || !Cursor.Next())
				{
					Cursor.Release();
					return 0;
				}

				Document Result = Cursor.GetCurrent();
				if (!Result.Get())
				{
					Cursor.Release();
					return 0;
				}

				Property Count;
				Result.Get("Count", &Count);
				Cursor.Release();

				return (uint64_t)Count.Integer;
#else
				return 0;
#endif
			}
			uint64_t Collection::CountDocuments(Document* Filter, Document* Options, Document* Reply) const
			{
#ifdef TH_HAS_MONGOC
				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!Base)
				{
					MDB_FREE(Filter);
					MDB_FREE(Options);
					return 0;
				}

				uint64_t Count = (uint64_t)mongoc_collection_count_documents(Base, MDB_POP(Filter), MDB_POP(Options), nullptr, MDB_POP(Reply), &Error);
				if (Error.code != 0)
				{
					MDB_FREE(Filter);
					MDB_FREE(Options);
					TH_ERROR("[mongoc] %s", Error.message);
					return false;
				}

				MDB_FREE(Filter);
				MDB_FREE(Options);
				return Count;
#else
				return 0;
#endif
			}
			uint64_t Collection::CountDocumentsEstimated(Document* Options, Document* Reply) const
			{
#ifdef TH_HAS_MONGOC
				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!Base)
				{
					MDB_FREE(Options);
					return 0;
				}

				uint64_t Count = (uint64_t)mongoc_collection_estimated_document_count(Base, MDB_POP(Options), nullptr, MDB_POP(Reply), &Error);
				if (Error.code != 0)
				{
					MDB_FREE(Options);
					TH_ERROR("[mongoc] %s", Error.message);
					return false;
				}

				MDB_FREE(Options);
				return Count;
#else
				return 0;
#endif
			}
			std::string Collection::StringifyKeyIndexes(Document* Keys) const
			{
#ifdef TH_HAS_MONGOC
				if (!Base || !MDB_POP(Keys))
					return std::string();

				char* Value = mongoc_collection_keys_to_index_string(MDB_POP(Keys));
				std::string Output = Value;
				bson_free(Value);

				MDB_FREE(Keys);
				return Output;
#else
				return "";
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
			Cursor Collection::FindIndexes(Document* Options) const
			{
#ifdef TH_HAS_MONGOC
				if (!Base)
				{
					MDB_FREE(Options);
					return nullptr;
				}

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				TCursor* Cursor = mongoc_collection_find_indexes_with_opts(Base, MDB_POP(Options));
				if (Cursor == nullptr || Error.code != 0)
				{
					MDB_FREE(Options);
					TH_ERROR("[mongoc] %s", Error.message);
					return nullptr;
				}

				MDB_FREE(Options);
				return Cursor;
#else
				return nullptr;
#endif
			}
			Cursor Collection::FindMany(Document* Filter, Document* Options) const
			{
#ifdef TH_HAS_MONGOC
				if (!Base)
				{
					MDB_FREE(Options);
					return nullptr;
				}

				TCursor* Result = mongoc_collection_find_with_opts(Base, MDB_POP(Filter), MDB_POP(Options), nullptr);
				MDB_FREE(Filter);
				MDB_FREE(Options);

				return Result;
#else
				return nullptr;
#endif
			}
			Cursor Collection::FindOne(Document* Filter, Document* Options) const
			{
#ifdef TH_HAS_MONGOC
				if (!Base)
				{
					MDB_FREE(Filter);
					MDB_FREE(Options);
					return nullptr;
				}

				Document Settings = (Options ? *Options : nullptr);
				if (Settings.Get() != nullptr)
					Settings.SetInteger("limit", 1);
				else
					Settings = Document(BCON_NEW("limit", BCON_INT32(1)));


				TCursor* Cursor = mongoc_collection_find_with_opts(Base, MDB_POP(Filter), Settings.Get(), nullptr);
				if ((!Options || !Options->Get()) && Settings.Get())
					Settings.Release();

				MDB_FREE(Filter);
				MDB_FREE(Options);
				return Cursor;
#else
				return nullptr;
#endif
			}
			Cursor Collection::Aggregate(Query Flags, Document* Pipeline, Document* Options) const
			{
#ifdef TH_HAS_MONGOC
				if (!Base)
				{
					MDB_FREE(Pipeline);
					MDB_FREE(Options);
					return nullptr;
				}

				TCursor* Result = mongoc_collection_aggregate(Base, (mongoc_query_flags_t)Flags, MDB_POP(Pipeline), MDB_POP(Options), nullptr);

				MDB_FREE(Pipeline);
				MDB_FREE(Options);

				return Result;
#else
				return nullptr;
#endif
			}
			Stream Collection::CreateStream(Document* Options)
			{
#ifdef TH_HAS_MONGOC
				if (!Base)
				{
					MDB_FREE(Options);
					return nullptr;
				}

				TStream* Operation = mongoc_collection_create_bulk_operation_with_opts(Base, MDB_POP(Options));
				MDB_FREE(Options);

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
			bool Database::HasCollection(const char* Name) const
			{
#ifdef TH_HAS_MONGOC
				if (!Base)
					return false;

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!mongoc_database_has_collection(Base, Name, &Error))
				{
					TH_ERROR("[mongoc] %s", Error.message);
					return false;
				}

				return true;
#else
				return false;
#endif
			}
			bool Database::RemoveAllUsers()
			{
#ifdef TH_HAS_MONGOC
				if (!Base)
					return false;

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!mongoc_database_remove_all_users(Base, &Error))
				{
					TH_ERROR("[mongoc] %s", Error.message);
					return false;
				}

				return true;
#else
				return false;
#endif
			}
			bool Database::RemoveUser(const char* Name)
			{
#ifdef TH_HAS_MONGOC
				if (!Base)
					return false;

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!mongoc_database_remove_user(Base, Name, &Error))
				{
					TH_ERROR("[mongoc] %s", Error.message);
					return false;
				}

				return true;
#else
				return false;
#endif
			}
			bool Database::Remove()
			{
#ifdef TH_HAS_MONGOC
				if (!Base)
					return false;

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!mongoc_database_drop(Base, &Error))
				{
					TH_ERROR("[mongoc] %s", Error.message);
					return false;
				}

				return true;
#else
				return false;
#endif
			}
			bool Database::RemoveWithOptions(Document* Options)
			{
#ifdef TH_HAS_MONGOC
				if (!Base)
				{
					MDB_FREE(Options);
					return false;
				}

				if (!MDB_POP(Options))
					return Remove();

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!mongoc_database_drop_with_opts(Base, MDB_POP(Options), &Error))
				{
					MDB_FREE(Options);
					TH_ERROR("[mongoc] %s", Error.message);
					return false;
				}

				MDB_FREE(Options);
				return true;
#else
				return false;
#endif
			}
			bool Database::AddUser(const char* Username, const char* Password, Document* Roles, Document* Custom)
			{
#ifdef TH_HAS_MONGOC
				if (!Base)
				{
					MDB_FREE(Roles);
					MDB_FREE(Custom);
					return false;
				}

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!mongoc_database_add_user(Base, Username, Password, MDB_POP(Roles), MDB_POP(Custom), &Error))
				{
					MDB_FREE(Roles);
					MDB_FREE(Custom);
					TH_ERROR("[mongoc] %s", Error.message);
					return false;
				}

				MDB_FREE(Roles);
				MDB_FREE(Custom);
				return true;
#else
				return false;
#endif
			}
			std::vector<std::string> Database::GetCollectionNames(Document* Options) const
			{
#ifdef TH_HAS_MONGOC
				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!Base)
					return std::vector<std::string>();

				char** Names = mongoc_database_get_collection_names_with_opts(Base, MDB_POP(Options), &Error);
				MDB_FREE(Options);

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
			Cursor Database::FindCollections(Document* Options) const
			{
#ifdef TH_HAS_MONGOC
				if (!Base)
				{
					MDB_FREE(Options);
					return nullptr;
				}

				TCursor* Cursor = mongoc_database_find_collections_with_opts(Base, MDB_POP(Options));
				MDB_FREE(Options);

				return Cursor;
#else
				return nullptr;
#endif
			}
			Collection Database::CreateCollection(const char* Name, Document* Options)
			{
#ifdef TH_HAS_MONGOC
				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!Base)
				{
					MDB_FREE(Options);
					return nullptr;
				}

				TCollection* Collection = mongoc_database_create_collection(Base, Name, MDB_POP(Options), &Error);
				MDB_FREE(Options);

				if (Collection == nullptr)
					TH_ERROR("[mongoc] %s", Error.message);

				return Collection;
#else
				return nullptr;
#endif
			}
			Collection Database::GetCollection(const char* Name) const
			{
#ifdef TH_HAS_MONGOC
				if (!Base)
					return nullptr;

				return mongoc_database_get_collection(Base, Name);
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
			bool Watcher::Next(Document& Result) const
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
			bool Watcher::Error(Document& Result) const
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
			Watcher Watcher::FromConnection(Connection* Connection, Document* Pipeline, Document* Options)
			{
#ifdef TH_HAS_MONGOC
				if (!Connection)
				{
					MDB_FREE(Pipeline);
					MDB_FREE(Options);
					return nullptr;
				}

				TWatcher* Stream = mongoc_client_watch(Connection->Get(), MDB_POP(Pipeline), MDB_POP(Options));
				MDB_FREE(Pipeline);
				MDB_FREE(Options);

				return Stream;
#else
				return nullptr;
#endif
			}
			Watcher Watcher::FromDatabase(const Database& Database, Document* Pipeline, Document* Options)
			{
#ifdef TH_HAS_MONGOC
				if (!Database.Get())
				{
					MDB_FREE(Pipeline);
					MDB_FREE(Options);
					return nullptr;
				}

				TWatcher* Stream = mongoc_database_watch(Database.Get(), MDB_POP(Pipeline), MDB_POP(Options));
				MDB_FREE(Pipeline);
				MDB_FREE(Options);

				return Stream;
#else
				return nullptr;
#endif
			}
			Watcher Watcher::FromCollection(const Collection& Collection, Document* Pipeline, Document* Options)
			{
#ifdef TH_HAS_MONGOC
				if (!Collection.Get())
				{
					MDB_FREE(Pipeline);
					MDB_FREE(Options);
					return nullptr;
				}

				TWatcher* Stream = mongoc_collection_watch(Collection.Get(), MDB_POP(Pipeline), MDB_POP(Options));
				MDB_FREE(Pipeline);
				MDB_FREE(Options);

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
			bool Transaction::Start()
			{
#ifdef TH_HAS_MONGOC
				if (!Base)
					return false;

				bson_error_t Error;
				if (!mongoc_client_session_start_transaction(Base, nullptr, &Error))
				{
					TH_ERROR("[mongoc] failed to start transaction\n\t%s", Error.message);
					return false;
				}

				return true;
#else
				return false;
#endif
			}
			bool Transaction::Abort()
			{
#ifdef TH_HAS_MONGOC
				if (!Base)
					return false;

				return mongoc_client_session_abort_transaction(Base, nullptr);
#else
				return false;
#endif
			}
			bool Transaction::Commit(Document* Reply)
			{
#ifdef TH_HAS_MONGOC
				if (!Base)
					return false;

				bson_error_t Error;
				if (!mongoc_client_session_commit_transaction(Base, MDB_POP(Reply), &Error))
				{
					TH_ERROR("[mongoc] could not commit transaction\n\t%s", Error.message);
					return false;
				}
				
				return true;
#else
				return false;
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
			TTransaction* Transaction::FromConnection(Connection* Client, Document& Uid)
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

				if (!Uid.Get())
					Uid = bson_new();

				if (!mongoc_client_session_append(Client->Session.Get(), Uid.Get(), &Error))
				{
					TH_ERROR("[mongoc] could not set session uid\n\t%s", Error.message);
					return false;
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
			void Connection::SetProfile(const char* Name)
			{
#ifdef TH_HAS_MONGOC
				mongoc_client_set_appname(Base, Name);
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
			bool Connection::Connect(const std::string& Address)
			{
#ifdef TH_HAS_MONGOC
				if (Master != nullptr)
					return false;

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (Connected)
					Disconnect();

				TAddress* URI = mongoc_uri_new_with_error(Address.c_str(), &Error);
				if (URI == nullptr)
				{
					TH_ERROR("[urierr] %s", Error.message);
					return false;
				}

				Base = mongoc_client_new_from_uri(URI);
				if (Base == nullptr)
				{
					TH_ERROR("couldn't connect to requested URI");
					return false;
				}

				Connected = true;
				return true;
#else
				return false;
#endif
			}
			bool Connection::Connect(Address* URI)
			{
#ifdef TH_HAS_MONGOC
				if (Master != nullptr || !URI || !URI->Get())
					return false;

				if (Connected)
					Disconnect();

				Base = mongoc_client_new_from_uri(URI->Get());
				if (!Base)
				{
					TH_ERROR("couldn't connect to requested URI");
					return false;
				}

				*URI = nullptr;
				Connected = true;
				return true;
#else
				return false;
#endif
			}
			bool Connection::Disconnect()
			{
#ifdef TH_HAS_MONGOC
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

				return true;
#else
				return false;
#endif
			}
			Cursor Connection::FindDatabases(Document* Options) const
			{
#ifdef TH_HAS_MONGOC
				TCursor* Reference = mongoc_client_find_databases_with_opts(Base, MDB_POP(Options));
				MDB_FREE(Options);

				return Reference;
#else
				return nullptr;
#endif
			}
			Database Connection::GetDatabase(const char* Name) const
			{
#ifdef TH_HAS_MONGOC
				return mongoc_client_get_database(Base, Name);
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
			std::vector<std::string> Connection::GetDatabaseNames(Document* Options)
			{
#ifdef TH_HAS_MONGOC
				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				char** Names = mongoc_client_get_database_names_with_opts(Base, MDB_POP(Options), &Error);
				MDB_FREE(Options);

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
			bool Connection::IsConnected()
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
			void Queue::SetProfile(const char* Name)
			{
#ifdef TH_HAS_MONGOC
				mongoc_client_pool_set_appname(Pool, Name);
#endif
			}
			bool Queue::Connect(const std::string& URI)
			{
#ifdef TH_HAS_MONGOC
				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (Connected)
					Disconnect();

				SrcAddress = mongoc_uri_new_with_error(URI.c_str(), &Error);
				if (!SrcAddress.Get())
				{
					TH_ERROR("[urierr] %s", Error.message);
					return false;
				}

				Pool = mongoc_client_pool_new(SrcAddress.Get());
				if (!Pool)
				{
					TH_ERROR("couldn't connect to requested URI");
					return false;
				}

				Connected = true;
				return true;
#else
				return false;
#endif
			}
			bool Queue::Connect(Address* URI)
			{
#ifdef TH_HAS_MONGOC
				if (!URI || !URI->Get())
					return false;

				if (Connected)
					Disconnect();

				Pool = mongoc_client_pool_new(URI->Get());
				if (!Pool)
				{
					TH_ERROR("couldn't connect to requested URI");
					return false;
				}

				SrcAddress = URI->Get();
				*URI = nullptr;
				Connected = true;
				return true;
#else
				return false;
#endif
			}
			bool Queue::Disconnect()
			{
#ifdef TH_HAS_MONGOC
				if (Pool != nullptr)
				{
					mongoc_client_pool_destroy(Pool);
					Pool = nullptr;
				}

				SrcAddress.Release();
				Connected = false;
				return true;
#else
				return false;
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

				delete *Client;
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
					Safe = new std::mutex();

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
						delete Safe;
						Safe = nullptr;
					}

					mongoc_cleanup();
					State = 0;
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
					Result.Cache = Document::FromJSON(Result.Request);

				Safe->lock();
				(*Queries)[Name] = Result;
				Safe->unlock();

				return true;
			}
			bool Driver::AddDirectory(const std::string& Directory, const std::string& Origin)
			{
				std::vector<Rest::ResourceEntry> Entries;
				if (!Rest::OS::Directory::Scan(Directory, &Entries))
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

					char* Buffer = (char*)Rest::OS::File::ReadAll(Base.Get(), &Size);
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
			Document Driver::GetQuery(const std::string& Name, QueryMap* Map, bool Once)
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
				Safe->unlock();

				Rest::Stroke Result(&Origin.Request);
				for (auto& Sub : *Map)
				{
					std::string Value = GetJSON(Sub.second);
					if (!Value.empty())
						Result.Replace(TH_PREFIX_STR "<" + Sub.first + '>', Value);

					if (Once)
						TH_RELEASE(Sub.second);
				}

				if (Once)
					Map->clear();

				Document Data = Document::FromJSON(Origin.Request);
				if (Data.Get())
					TH_ERROR("could not construct query: \"%s\"", Name.c_str());

				return Data;
			}
			Document Driver::GetSubquery(const char* Buffer, QueryMap* Map, bool Once)
			{
				if (!Buffer || Buffer[0] == '\0')
				{
					if (Once && Map != nullptr)
					{
						for (auto& Item : *Map)
							TH_RELEASE(Item.second);
						Map->clear();
					}

					return nullptr;
				}

				if (!Map || Map->empty())
				{
					if (Once && Map != nullptr)
					{
						for (auto& Item : *Map)
							TH_RELEASE(Item.second);
						Map->clear();
					}

					return Document::FromJSON(Buffer);
				}

				Rest::Stroke Result(Buffer, strlen(Buffer));
				for (auto& Sub : *Map)
				{
					std::string Value = GetJSON(Sub.second);
					if (!Value.empty())
						Result.Replace(TH_PREFIX_STR "<" + Sub.first + '>', Value);

					if (Once)
						TH_RELEASE(Sub.second);
				}

				if (Once)
					Map->clear();

				Document Data = Document::FromJSON(Result.R());
				if (!Data.Get())
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
			std::string Driver::GetJSON(Rest::Document* Source)
			{
				if (!Source)
					return "";

				switch (Source->Value.GetType())
				{
					case Rest::VarType_Object:
					{
						std::string Result = "{";
						for (auto* Node : *Source->GetNodes())
						{
							Result.append(1, '\"').append(Node->Key).append("\":");
							Result.append(GetJSON(Node)).append(1, ',');
						}

						if (!Source->GetNodes()->empty())
							Result = Result.substr(0, Result.size() - 1);

						return Result + "}";
					}
					case Rest::VarType_Array:
					{
						std::string Result = "[";
						for (auto* Node : *Source->GetNodes())
							Result.append(GetJSON(Node)).append(1, ',');

						if (!Source->GetNodes()->empty())
							Result = Result.substr(0, Result.size() - 1);

						return Result + "]";
					}
					case Rest::VarType_String:
						return "\"" + Source->Value.GetBlob() + "\"";
					case Rest::VarType_Integer:
						return std::to_string(Source->Value.GetInteger());
					case Rest::VarType_Number:
						return std::to_string(Source->Value.GetNumber());
					case Rest::VarType_Boolean:
						return Source->Value.GetBoolean() ? "true" : "false";
					case Rest::VarType_Decimal:
						return "{\"$numberDouble\":\"" + Source->Value.GetDecimal() + "\"}";
					case Rest::VarType_Base64:
					{
						if (Source->Value.GetSize() != 12)
						{
							std::string Base = Compute::Common::Base64Encode(Source->Value.GetBlob());
							return "\"" + Base + "\"";
						}

						return "{\"$oid\":\"" + Util::IdToString(Source->Value.GetBase64()) + "\"}";
					}
					case Rest::VarType_Null:
					case Rest::VarType_Undefined:
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