#include "mdb.h"
extern "C"
{
#ifdef TH_HAS_MONGOC
#include <mongoc.h>
#define BS(X) (X != nullptr ? *X : nullptr)
#endif
}

namespace Tomahawk
{
	namespace Network
	{
		namespace MDB
		{
			Property::~Property()
			{
				Release();
			}
			void Property::Release()
			{
				Document::Release(&Document);
				Document::Release(&Array);
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

			bool Util::ParseDecimal(const char* Value, int64_t* High, int64_t* Low)
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
			bool Util::GenerateId(unsigned char* Id12)
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
			std::string Util::OIdToString(unsigned char* Id12)
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
			std::string Util::StringToOId(const std::string& Id24)
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

			TDocument* Document::Create(bool Array)
			{
#ifdef TH_HAS_MONGOC
				if (!Array)
					return bson_new();

				return BCON_NEW("pipeline", "[", "]");
#else
				return nullptr;
#endif
			}
			TDocument* Document::Create(Rest::Document* Document)
			{
#ifdef TH_HAS_MONGOC
				if (!Document || !Document->Value.IsObject())
					return nullptr;

				TDocument* New = bson_new();
				TDocument* Replica = nullptr;
				uint64_t Index = 0;

				bool Array = (Document->Value.GetType() == Rest::VarType_Array);
				for (auto&& Node : *Document->GetNodes())
				{
					switch (Node->Value.GetType())
					{
						case Rest::VarType_Object:
							Replica = Create(Node);
							SetDocument(New, Array ? nullptr : Node->Key.c_str(), &Replica, Index);
							break;
						case Rest::VarType_Array:
							Replica = Create(Node);
							SetArray(New, Array ? nullptr : Node->Key.c_str(), &Replica, Index);
							break;
						case Rest::VarType_String:
							SetStringBuffer(New, Array ? nullptr : Node->Key.c_str(), Node->Value.GetString(), Node->Value.GetSize(), Index);
							break;
						case Rest::VarType_Boolean:
							SetBoolean(New, Array ? nullptr : Node->Key.c_str(), Node->Value.GetBoolean(), Index);
							break;
						case Rest::VarType_Decimal:
							SetDecimalString(New, Array ? nullptr : Node->Key.c_str(), Node->Value.GetDecimal(), Index);
							break;
						case Rest::VarType_Number:
							SetNumber(New, Array ? nullptr : Node->Key.c_str(), Node->Value.GetNumber(), Index);
							break;
						case Rest::VarType_Integer:
							SetInteger(New, Array ? nullptr : Node->Key.c_str(), Node->Value.GetInteger(), Index);
							break;
						case Rest::VarType_Null:
							SetNull(New, Array ? nullptr : Node->Key.c_str(), Index);
							break;
						case Rest::VarType_Base64:
						{
							if (Node->Value.GetSize() != 12)
							{
								std::string Base = Compute::Common::Base64Encode(Node->Value.GetBlob());
								SetStringBuffer(New, Array ? nullptr : Node->Key.c_str(), Base.c_str(), Base.size(), Index);
							}
							else
								SetObjectId(New, Array ? nullptr : Node->Key.c_str(), Node->Value.GetBase64(), Index);
							break;
						}
						default:
							break;
					}
				}

				return New;
#else
				return nullptr;
#endif
			}
			TDocument* Document::Create(const std::string& JSON)
			{
#ifdef TH_HAS_MONGOC
				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				TDocument* Document = bson_new_from_json((unsigned char*)JSON.c_str(), (ssize_t)JSON.size(), &Error);
				if (!Document || Error.code != 0)
				{
					if (Document != nullptr)
						bson_destroy(Document);

					TH_ERROR("[json] %s", Error.message);
					return nullptr;
				}

				return Document;
#else
				return nullptr;
#endif
			}
			TDocument* Document::Create(const unsigned char* Buffer, uint64_t Length)
			{
#ifdef TH_HAS_MONGOC
				return bson_new_from_data(Buffer, (size_t)Length);
#else
				return nullptr;
#endif
			}
			void Document::Release(TDocument** Document)
			{
#ifdef TH_HAS_MONGOC
				if (Document == nullptr || *Document == nullptr)
					return;

				if ((*Document)->flags == 2)
				{
					bson_destroy(*Document);
					bson_free(*Document);
				}
				else
					bson_destroy(*Document);
#endif
				*Document = nullptr;
			}
			void Document::Loop(TDocument* Document, void* Context, const std::function<bool(TDocument*, Property*, void*)>& Callback)
			{
				if (!Callback || !Document)
					return;

#ifdef TH_HAS_MONGOC
				bson_iter_t It;
				if (!bson_iter_init(&It, Document))
					return;

				Property Output;
				while (bson_iter_next(&It))
				{
					Clone(&It, &Output);
					bool Continue = Callback(Document, &Output, Context);
					Output.Release();

					if (!Continue)
						break;
				}
#endif
			}
			void Document::Join(TDocument* Document, TDocument* Value)
			{
#ifdef TH_HAS_MONGOC
				if (Document != nullptr && Value != nullptr)
					bson_concat(Document, Value);
#endif
			}
			bool Document::SetDocument(TDocument* Document, const char* Key, TDocument** Value, uint64_t ArrayId)
			{
#ifdef TH_HAS_MONGOC
				if (!Value || !*Value || !Document)
					return false;

				char Index[16];
				if (Key == nullptr)
					bson_uint32_to_string((uint32_t)ArrayId, &Key, Index, sizeof(Index));

				bool Result = bson_append_document(Document, Key, -1, *Value);
				bson_destroy(*Value);
				*Value = nullptr;

				return Result;
#else
				return false;
#endif
			}
			bool Document::SetArray(TDocument* Document, const char* Key, TDocument** Value, uint64_t ArrayId)
			{
#ifdef TH_HAS_MONGOC
				if (!Value || !*Value || !Document)
					return false;

				char Index[16];
				if (Key == nullptr)
					bson_uint32_to_string((uint32_t)ArrayId, &Key, Index, sizeof(Index));

				bool Result = bson_append_array(Document, Key, -1, *Value);
				bson_destroy(*Value);
				*Value = nullptr;

				return Result;
#else
				return false;
#endif
			}
			bool Document::SetString(TDocument* Document, const char* Key, const char* Value, uint64_t ArrayId)
			{
#ifdef TH_HAS_MONGOC
				if (!Document)
					return false;

				char Index[16];
				if (Key == nullptr)
					bson_uint32_to_string((uint32_t)ArrayId, &Key, Index, sizeof(Index));

				return bson_append_utf8(Document, Key, -1, Value, -1);
#else
				return false;
#endif
			}
			bool Document::SetStringBuffer(TDocument* Document, const char* Key, const char* Value, uint64_t Length, uint64_t ArrayId)
			{
#ifdef TH_HAS_MONGOC
				if (!Document)
					return false;

				char Index[16];
				if (Key == nullptr)
					bson_uint32_to_string((uint32_t)ArrayId, &Key, Index, sizeof(Index));

				return bson_append_utf8(Document, Key, -1, Value, (int)Length);
#else
				return false;
#endif
			}
			bool Document::SetInteger(TDocument* Document, const char* Key, int64_t Value, uint64_t ArrayId)
			{
#ifdef TH_HAS_MONGOC
				if (!Document)
					return false;

				char Index[16];
				if (Key == nullptr)
					bson_uint32_to_string((uint32_t)ArrayId, &Key, Index, sizeof(Index));

				return bson_append_int64(Document, Key, -1, Value);
#else
				return false;
#endif
			}
			bool Document::SetNumber(TDocument* Document, const char* Key, double Value, uint64_t ArrayId)
			{
#ifdef TH_HAS_MONGOC
				if (!Document)
					return false;

				char Index[16];
				if (Key == nullptr)
					bson_uint32_to_string((uint32_t)ArrayId, &Key, Index, sizeof(Index));

				return bson_append_double(Document, Key, -1, Value);
#else
				return false;
#endif
			}
			bool Document::SetDecimal(TDocument* Document, const char* Key, uint64_t High, uint64_t Low, uint64_t ArrayId)
			{
#ifdef TH_HAS_MONGOC
				if (!Document)
					return false;

				char Index[16];
				if (Key == nullptr)
					bson_uint32_to_string((uint32_t)ArrayId, &Key, Index, sizeof(Index));

				bson_decimal128_t Decimal;
				Decimal.high = (uint64_t)High;
				Decimal.low = (uint64_t)Low;

				return bson_append_decimal128(Document, Key, -1, &Decimal);
#else
				return false;
#endif
			}
			bool Document::SetDecimalString(TDocument* Document, const char* Key, const std::string& Value, uint64_t ArrayId)
			{
#ifdef TH_HAS_MONGOC
				if (!Document)
					return false;

				char Index[16];
				if (Key == nullptr)
					bson_uint32_to_string((uint32_t)ArrayId, &Key, Index, sizeof(Index));

				bson_decimal128_t Decimal;
				bson_decimal128_from_string(Value.c_str(), &Decimal);

				return bson_append_decimal128(Document, Key, -1, &Decimal);
#else
				return false;
#endif
			}
			bool Document::SetDecimalInteger(TDocument* Document, const char* Key, int64_t Value, uint64_t ArrayId)
			{
#ifdef TH_HAS_MONGOC
				if (!Document)
					return false;

				char Index[16];
				if (Key == nullptr)
					bson_uint32_to_string((uint32_t)ArrayId, &Key, Index, sizeof(Index));

				char Data[64];
				sprintf(Data, "%lld", Value);

				bson_decimal128_t Decimal;
				bson_decimal128_from_string(Data, &Decimal);

				return bson_append_decimal128(Document, Key, -1, &Decimal);
#else
				return false;
#endif
			}
			bool Document::SetDecimalNumber(TDocument* Document, const char* Key, double Value, uint64_t ArrayId)
			{
#ifdef TH_HAS_MONGOC
				if (!Document)
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

				return bson_append_decimal128(Document, Key, -1, &Decimal);
#else
				return false;
#endif
			}
			bool Document::SetBoolean(TDocument* Document, const char* Key, bool Value, uint64_t ArrayId)
			{
#ifdef TH_HAS_MONGOC
				if (!Document)
					return false;

				char Index[16];
				if (Key == nullptr)
					bson_uint32_to_string((uint32_t)ArrayId, &Key, Index, sizeof(Index));

				return bson_append_bool(Document, Key, -1, Value);
#else
				return false;
#endif
			}
			bool Document::SetObjectId(TDocument* Document, const char* Key, unsigned char Value[12], uint64_t ArrayId)
			{
#ifdef TH_HAS_MONGOC
				if (!Document)
					return false;

				bson_oid_t ObjectId;
				memcpy(ObjectId.bytes, Value, sizeof(unsigned char) * 12);

				char Index[16];
				if (Key == nullptr)
					bson_uint32_to_string((uint32_t)ArrayId, &Key, Index, sizeof(Index));

				return bson_append_oid(Document, Key, -1, &ObjectId);
#else
				return false;
#endif
			}
			bool Document::SetNull(TDocument* Document, const char* Key, uint64_t ArrayId)
			{
#ifdef TH_HAS_MONGOC
				if (!Document)
					return false;

				char Index[16];
				if (Key == nullptr)
					bson_uint32_to_string((uint32_t)ArrayId, &Key, Index, sizeof(Index));

				return bson_append_null(Document, Key, -1);
#else
				return false;
#endif
			}
			bool Document::Set(TDocument* Document, const char* Key, Property* Value, uint64_t ArrayId)
			{
#ifdef TH_HAS_MONGOC
				if (!Document || !Value)
					return false;

				char Index[16];
				if (Key == nullptr)
					bson_uint32_to_string((uint32_t)ArrayId, &Key, Index, sizeof(Index));

				TDocument* Object = nullptr;
				switch (Value->Mod)
				{
					case Type_Document:
						Object = Copy(Value->Document);
						return SetDocument(Document, Key, &Object);
					case Type_Array:
						Object = Copy(Value->Document);
						return SetArray(Document, Key, &Object);
					case Type_String:
						return SetString(Document, Key, Value->String.c_str());
					case Type_Boolean:
						return SetBoolean(Document, Key, Value->Boolean);
					case Type_Number:
						return SetNumber(Document, Key, Value->Number);
					case Type_Integer:
						return SetInteger(Document, Key, Value->Integer);
					case Type_Decimal:
						return SetDecimal(Document, Key, Value->High, Value->Low);
					case Type_ObjectId:
						return SetObjectId(Document, Key, Value->ObjectId);
					default:
						break;
				}

				return false;
#else
				return false;
#endif
			}
			bool Document::Has(TDocument* Document, const char* Key)
			{
#ifdef TH_HAS_MONGOC
				if (!Document)
					return false;

				return bson_has_field(Document, Key);
#else
				return false;
#endif
			}
			bool Document::Get(TDocument* Document, const char* Key, Property* Output)
			{
#ifdef TH_HAS_MONGOC
				bson_iter_t It;
				if (!Document || !bson_iter_init_find(&It, Document, Key))
					return false;

				return Clone(&It, Output);
#else
				return false;
#endif
			}
			bool Document::Find(TDocument* Document, const char* Key, Property* Output)
			{
#ifdef TH_HAS_MONGOC
				bson_iter_t It, Value;
				if (!Document || !bson_iter_init(&It, Document) || !bson_iter_find_descendant(&It, Key, &Value))
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
						Output->Document = Create((const unsigned char*)Value->value.v_doc.data, (uint64_t)Value->value.v_doc.data_len);
						break;
					case BSON_TYPE_ARRAY:
						Output->Mod = Type_Array;
						Output->Array = Create((const unsigned char*)Value->value.v_doc.data, (uint64_t)Value->value.v_doc.data_len);
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
			uint64_t Document::Count(TDocument* Document)
			{
#ifdef TH_HAS_MONGOC
				if (!Document)
					return 0;

				return bson_count_keys(Document);
#else
				return 0;
#endif
			}
			std::string Document::ToRelaxedJSON(TDocument* Document)
			{
#ifdef TH_HAS_MONGOC
				if (!Document)
					return std::string();

				size_t Length = 0;
				char* Value = bson_as_relaxed_extended_json(Document, &Length);

				std::string Output;
				Output.assign(Value, (uint64_t)Length);
				bson_free(Value);

				return Output;
#else
				return "";
#endif
			}
			std::string Document::ToExtendedJSON(TDocument* Document)
			{
#ifdef TH_HAS_MONGOC
				if (!Document)
					return std::string();

				size_t Length = 0;
				char* Value = bson_as_canonical_extended_json(Document, &Length);

				std::string Output;
				Output.assign(Value, (uint64_t)Length);
				bson_free(Value);

				return Output;
#else
				return "";
#endif
			}
			std::string Document::ToJSON(TDocument* Document)
			{
#ifdef TH_HAS_MONGOC
				if (!Document)
					return std::string();

				size_t Length = 0;
				char* Value = bson_as_json(Document, &Length);

				std::string Output;
				Output.assign(Value, (uint64_t)Length);
				bson_free(Value);

				return Output;
#else
				return "";
#endif
			}
			Rest::Document* Document::ToDocument(TDocument* Document, bool IsArray)
			{
#ifdef TH_HAS_MONGOC
				if (!Document)
					return nullptr;

				Rest::Document* New = (IsArray ? Rest::Document::Array() : Rest::Document::Object());
				Loop(Document, New, [](TDocument* Base, Property* Key, void* UserData) -> bool
				{
					Rest::Document* Node = (Rest::Document*)UserData;
					std::string Name = (Node->Value.GetType() == Rest::VarType_Array ? "" : Key->Name);

					switch (Key->Mod)
					{
						case Type_Document:
						{
							Node->Set(Name, ToDocument(Key->Document, false));
							break;
						}
						case Type_Array:
						{
							Node->Set(Name, ToDocument(Key->Array, true));
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

				return New;
#else
				return nullptr;
#endif
			}
			TDocument* Document::Copy(TDocument* Document)
			{
#ifdef TH_HAS_MONGOC
				if (!Document)
					return nullptr;

				return bson_copy(Document);
#else
				return nullptr;
#endif
			}

			TAddress* Address::Create(const char* Uri)
			{
#ifdef TH_HAS_MONGOC
				return mongoc_uri_new(Uri);
#else
				return nullptr;
#endif
			}
			void Address::Release(TAddress** URI)
			{
#ifdef TH_HAS_MONGOC
				if (!URI || !*URI)
					return;

				mongoc_uri_destroy(*URI);
				*URI = nullptr;
#endif
			}
			void Address::SetOption(TAddress* URI, const char* Name, int64_t Value)
			{
#ifdef TH_HAS_MONGOC
				if (URI != nullptr)
					mongoc_uri_set_option_as_int32(URI, Name, (int32_t)Value);
#endif
			}
			void Address::SetOption(TAddress* URI, const char* Name, bool Value)
			{
#ifdef TH_HAS_MONGOC
				if (URI != nullptr)
					mongoc_uri_set_option_as_bool(URI, Name, Value);
#endif
			}
			void Address::SetOption(TAddress* URI, const char* Name, const char* Value)
			{
#ifdef TH_HAS_MONGOC
				if (URI != nullptr)
					mongoc_uri_set_option_as_utf8(URI, Name, Value);
#endif
			}
			void Address::SetAuthMechanism(TAddress* URI, const char* Value)
			{
#ifdef TH_HAS_MONGOC
				if (URI != nullptr)
					mongoc_uri_set_auth_mechanism(URI, Value);
#endif
			}
			void Address::SetAuthSource(TAddress* URI, const char* Value)
			{
#ifdef TH_HAS_MONGOC
				if (URI != nullptr)
					mongoc_uri_set_auth_source(URI, Value);
#endif
			}
			void Address::SetCompressors(TAddress* URI, const char* Value)
			{
#ifdef TH_HAS_MONGOC
				if (URI != nullptr)
					mongoc_uri_set_compressors(URI, Value);
#endif
			}
			void Address::SetDatabase(TAddress* URI, const char* Value)
			{
#ifdef TH_HAS_MONGOC
				if (URI != nullptr)
					mongoc_uri_set_database(URI, Value);
#endif
			}
			void Address::SetUsername(TAddress* URI, const char* Value)
			{
#ifdef TH_HAS_MONGOC
				if (URI != nullptr)
					mongoc_uri_set_username(URI, Value);
#endif
			}
			void Address::SetPassword(TAddress* URI, const char* Value)
			{
#ifdef TH_HAS_MONGOC
				if (URI != nullptr)
					mongoc_uri_set_password(URI, Value);
#endif
			}

			TStream* Stream::Create(bool IsOrdered)
			{
#ifdef TH_HAS_MONGOC
				return mongoc_bulk_operation_new(IsOrdered);
#else
				return nullptr;
#endif
			}
			void Stream::Release(TStream** Operation)
			{
#ifdef TH_HAS_MONGOC
				if (!Operation || !*Operation)
					return;

				mongoc_bulk_operation_destroy(*Operation);
				*Operation = nullptr;
#endif
			}
			bool Stream::RemoveMany(TStream* Operation, TDocument** Selector, TDocument** Options)
			{
#ifdef TH_HAS_MONGOC
				if (!Operation)
				{
					Document::Release(Selector);
					Document::Release(Options);
					return false;
				}

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!mongoc_bulk_operation_remove_many_with_opts(Operation, BS(Selector), BS(Options), &Error))
				{
					Document::Release(Selector);
					Document::Release(Options);
					TH_ERROR("[mongoc] %s", Error.message);
					return false;
				}

				Document::Release(Selector);
				Document::Release(Options);
				return true;
#else
				return false;
#endif
			}
			bool Stream::RemoveOne(TStream* Operation, TDocument** Selector, TDocument** Options)
			{
#ifdef TH_HAS_MONGOC
				if (!Operation)
				{
					Document::Release(Selector);
					Document::Release(Options);
					return false;
				}

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!mongoc_bulk_operation_remove_one_with_opts(Operation, BS(Selector), BS(Options), &Error))
				{
					Document::Release(Selector);
					Document::Release(Options);
					TH_ERROR("[mongoc] %s", Error.message);
					return false;
				}

				Document::Release(Selector);
				Document::Release(Options);
				return true;
#else
				return false;
#endif
			}
			bool Stream::ReplaceOne(TStream* Operation, TDocument** Selector, TDocument** Replacement, TDocument** Options)
			{
#ifdef TH_HAS_MONGOC
				if (!Operation)
				{
					Document::Release(Selector);
					Document::Release(Replacement);
					Document::Release(Options);
					return false;
				}

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!mongoc_bulk_operation_replace_one_with_opts(Operation, BS(Selector), BS(Replacement), BS(Options), &Error))
				{
					Document::Release(Selector);
					Document::Release(Replacement);
					Document::Release(Options);
					TH_ERROR("[mongoc] %s", Error.message);
					return false;
				}

				Document::Release(Selector);
				Document::Release(Replacement);
				Document::Release(Options);
				return true;
#else
				return false;
#endif
			}
			bool Stream::Insert(TStream* Operation, TDocument** Document, TDocument** Options)
			{
#ifdef TH_HAS_MONGOC
				if (!Operation)
				{
					Document::Release(Document);
					Document::Release(Options);
					return false;
				}

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!mongoc_bulk_operation_insert_with_opts(Operation, BS(Document), BS(Options), &Error))
				{
					Document::Release(Document);
					Document::Release(Options);
					TH_ERROR("[mongoc] %s", Error.message);
					return false;
				}

				Document::Release(Document);
				Document::Release(Options);
				return true;
#else
				return false;
#endif
			}
			bool Stream::UpdateOne(TStream* Operation, TDocument** Selector, TDocument** Document, TDocument** Options)
			{
#ifdef TH_HAS_MONGOC
				if (!Operation)
				{
					Document::Release(Selector);
					Document::Release(Document);
					Document::Release(Options);
					return false;
				}

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!mongoc_bulk_operation_update_one_with_opts(Operation, BS(Selector), BS(Document), BS(Options), &Error))
				{
					Document::Release(Selector);
					Document::Release(Document);
					Document::Release(Options);
					TH_ERROR("[mongoc] %s", Error.message);
					return false;
				}

				Document::Release(Selector);
				Document::Release(Document);
				Document::Release(Options);
				return true;
#else
				return false;
#endif
			}
			bool Stream::UpdateMany(TStream* Operation, TDocument** Selector, TDocument** Document, TDocument** Options)
			{
#ifdef TH_HAS_MONGOC
				if (!Operation)
				{
					Document::Release(Selector);
					Document::Release(Document);
					Document::Release(Options);
					return false;
				}

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!mongoc_bulk_operation_update_many_with_opts(Operation, BS(Selector), BS(Document), BS(Options), &Error))
				{
					Document::Release(Selector);
					Document::Release(Document);
					Document::Release(Options);
					TH_ERROR("[mongoc] %s", Error.message);
					return false;
				}

				Document::Release(Selector);
				Document::Release(Document);
				Document::Release(Options);
				return true;
#else
				return false;
#endif
			}
			bool Stream::RemoveMany(TStream* Operation, TDocument* Selector, TDocument* Options)
			{
#ifdef TH_HAS_MONGOC
				if (!Operation)
					return false;

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!mongoc_bulk_operation_remove_many_with_opts(Operation, Selector, Options, &Error))
				{
					TH_ERROR("[mongoc] %s", Error.message);
					return false;
				}

				return true;
#else
				return false;
#endif
			}
			bool Stream::RemoveOne(TStream* Operation, TDocument* Selector, TDocument* Options)
			{
#ifdef TH_HAS_MONGOC
				if (!Operation)
					return false;

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!mongoc_bulk_operation_remove_one_with_opts(Operation, Selector, Options, &Error))
				{
					TH_ERROR("[mongoc] %s", Error.message);
					return false;
				}

				return true;
#else
				return false;
#endif
			}
			bool Stream::ReplaceOne(TStream* Operation, TDocument* Selector, TDocument* Replacement, TDocument* Options)
			{
#ifdef TH_HAS_MONGOC
				if (!Operation)
					return false;

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!mongoc_bulk_operation_replace_one_with_opts(Operation, Selector, Replacement, Options, &Error))
				{
					TH_ERROR("[mongoc] %s", Error.message);
					return false;
				}

				return true;
#else
				return false;
#endif
			}
			bool Stream::Insert(TStream* Operation, TDocument* Document, TDocument* Options)
			{
#ifdef TH_HAS_MONGOC
				if (!Operation)
					return false;

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!mongoc_bulk_operation_insert_with_opts(Operation, Document, Options, &Error))
				{
					TH_ERROR("[mongoc] %s", Error.message);
					return false;
				}

				return true;
#else
				return false;
#endif
			}
			bool Stream::UpdateOne(TStream* Operation, TDocument* Selector, TDocument* Document, TDocument* Options)
			{
#ifdef TH_HAS_MONGOC
				if (!Operation)
					return false;

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!mongoc_bulk_operation_update_one_with_opts(Operation, Selector, Document, Options, &Error))
				{
					TH_ERROR("[mongoc] %s", Error.message);
					return false;
				}

				return true;
#else
				return false;
#endif
			}
			bool Stream::UpdateMany(TStream* Operation, TDocument* Selector, TDocument* Document, TDocument* Options)
			{
#ifdef TH_HAS_MONGOC
				if (!Operation)
					return false;

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!mongoc_bulk_operation_update_many_with_opts(Operation, Selector, Document, Options, &Error))
				{
					TH_ERROR("[mongoc] %s", Error.message);
					return false;
				}

				return true;
#else
				return false;
#endif
			}
			bool Stream::Execute(TStream** Operation)
			{
#ifdef TH_HAS_MONGOC
				if (!Operation || !*Operation)
					return false;

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				TDocument Result;
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
			bool Stream::Execute(TStream** Operation, TDocument** Reply)
			{
#ifdef TH_HAS_MONGOC
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
			uint64_t Stream::GetHint(TStream* Operation)
			{
#ifdef TH_HAS_MONGOC
				if (!Operation)
					return 0;

				return (uint64_t)mongoc_bulk_operation_get_hint(Operation);
#else
				return 0;
#endif
			}

			TWatcher* Watcher::Create(TConnection* Connection, TDocument** Pipeline, TDocument** Options)
			{
#ifdef TH_HAS_MONGOC
				if (!Connection)
				{
					Document::Release(Pipeline);
					Document::Release(Options);
					return nullptr;
				}

				TWatcher* Stream = mongoc_client_watch(Connection, BS(Pipeline), BS(Options));
				Document::Release(Pipeline);
				Document::Release(Options);

				return Stream;
#else
				return nullptr;
#endif
			}
			TWatcher* Watcher::Create(TDatabase* Database, TDocument** Pipeline, TDocument** Options)
			{
#ifdef TH_HAS_MONGOC
				if (!Database)
				{
					Document::Release(Pipeline);
					Document::Release(Options);
					return nullptr;
				}

				TWatcher* Stream = mongoc_database_watch(Database, BS(Pipeline), BS(Options));
				Document::Release(Pipeline);
				Document::Release(Options);

				return Stream;
#else
				return nullptr;
#endif
			}
			TWatcher* Watcher::Create(TCollection* Collection, TDocument** Pipeline, TDocument** Options)
			{
#ifdef TH_HAS_MONGOC
				if (!Collection)
				{
					Document::Release(Pipeline);
					Document::Release(Options);
					return nullptr;
				}

				TWatcher* Stream = mongoc_collection_watch(Collection, BS(Pipeline), BS(Options));
				Document::Release(Pipeline);
				Document::Release(Options);

				return Stream;
#else
				return nullptr;
#endif
			}
			void Watcher::Release(TWatcher** Stream)
			{
#ifdef TH_HAS_MONGOC
				if (!Stream || !*Stream)
					return;

				mongoc_change_stream_destroy(*Stream);
				*Stream = nullptr;
#endif
			}
			bool Watcher::Next(TWatcher* Stream, TDocument** Result)
			{
#ifdef TH_HAS_MONGOC
				if (!Stream)
				{
					Document::Release(Result);
					return false;
				}

				return mongoc_change_stream_next(Stream, (const TDocument**)Result);
#else
				return false;
#endif
			}
			bool Watcher::Error(TWatcher* Stream, TDocument** Result)
			{
#ifdef TH_HAS_MONGOC
				if (!Stream)
				{
					Document::Release(Result);
					return false;
				}

				return mongoc_change_stream_error_document(Stream, nullptr, (const TDocument**)Result);
#else
				return false;
#endif
			}

			void Cursor::Release(TCursor** Cursor)
			{
#ifdef TH_HAS_MONGOC
				if (!Cursor || !*Cursor)
					return;

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (mongoc_cursor_error(*Cursor, &Error))
					TH_ERROR("[mongoc] %s", Error.message);

				mongoc_cursor_destroy(*Cursor);
				*Cursor = nullptr;
#endif
			}
			void Cursor::Receive(TCursor* Cursor, void* Context, bool(* Next)(TCursor*, TDocument*, void*))
			{
#ifdef TH_HAS_MONGOC
				if (!Next || !Cursor)
					return;

				TDocument* Query = nullptr;
				while (mongoc_cursor_next(Cursor, (const TDocument**)&Query))
				{
					if (!Next(Cursor, Query, Context))
						break;
				}
#endif
			}
			void Cursor::SetMaxAwaitTime(TCursor* Cursor, uint64_t MaxAwaitTime)
			{
#ifdef TH_HAS_MONGOC
				if (Cursor != nullptr)
					mongoc_cursor_set_max_await_time_ms(Cursor, (uint32_t)MaxAwaitTime);
#endif
			}
			void Cursor::SetBatchSize(TCursor* Cursor, uint64_t BatchSize)
			{
#ifdef TH_HAS_MONGOC
				if (Cursor != nullptr)
					mongoc_cursor_set_batch_size(Cursor, (uint32_t)BatchSize);
#endif
			}
			bool Cursor::Next(TCursor* Cursor)
			{
#ifdef TH_HAS_MONGOC
				if (!Cursor)
					return false;

				TDocument* Query = nullptr;
				return mongoc_cursor_next(Cursor, (const TDocument**)&Query);
#else
				return false;
#endif
			}
			bool Cursor::SetLimit(TCursor* Cursor, int64_t Limit)
			{
#ifdef TH_HAS_MONGOC
				if (!Cursor)
					return false;

				return mongoc_cursor_set_limit(Cursor, Limit);
#else
				return false;
#endif
			}
			bool Cursor::SetHint(TCursor* Cursor, uint64_t Hint)
			{
#ifdef TH_HAS_MONGOC
				if (!Cursor)
					return false;

				return mongoc_cursor_set_hint(Cursor, (uint32_t)Hint);
#else
				return false;
#endif
			}
			bool Cursor::HasError(TCursor* Cursor)
			{
#ifdef TH_HAS_MONGOC
				if (!Cursor)
					return false;

				return mongoc_cursor_error(Cursor, nullptr);
#else
				return false;
#endif
			}
			bool Cursor::HasMoreData(TCursor* Cursor)
			{
#ifdef TH_HAS_MONGOC
				if (!Cursor)
					return false;

				return mongoc_cursor_more(Cursor);
#else
				return false;
#endif
			}
			int64_t Cursor::GetId(TCursor* Cursor)
			{
#ifdef TH_HAS_MONGOC
				if (!Cursor)
					return 0;

				return (int64_t)mongoc_cursor_get_id(Cursor);
#else
				return 0;
#endif
			}
			int64_t Cursor::GetLimit(TCursor* Cursor)
			{
#ifdef TH_HAS_MONGOC
				if (!Cursor)
					return 0;

				return (int64_t)mongoc_cursor_get_limit(Cursor);
#else
				return 0;
#endif
			}
			uint64_t Cursor::GetMaxAwaitTime(TCursor* Cursor)
			{
#ifdef TH_HAS_MONGOC
				if (!Cursor)
					return 0;

				return (uint64_t)mongoc_cursor_get_max_await_time_ms(Cursor);
#else
				return 0;
#endif
			}
			uint64_t Cursor::GetBatchSize(TCursor* Cursor)
			{
#ifdef TH_HAS_MONGOC
				if (!Cursor)
					return 0;

				return (uint64_t)mongoc_cursor_get_batch_size(Cursor);
#else
				return 0;
#endif
			}
			uint64_t Cursor::GetHint(TCursor* Cursor)
			{
#ifdef TH_HAS_MONGOC
				if (!Cursor)
					return 0;

				return (uint64_t)mongoc_cursor_get_hint(Cursor);
#else
				return 0;
#endif
			}
			TDocument* Cursor::GetCurrent(TCursor* Cursor)
			{
#ifdef TH_HAS_MONGOC
				if (!Cursor)
					return nullptr;

				return (TDocument*)mongoc_cursor_current(Cursor);
#else
				return nullptr;
#endif
			}

			void Collection::Release(TCollection** Collection)
			{
#ifdef TH_HAS_MONGOC
				if (!Collection || !*Collection)
					return;

				mongoc_collection_destroy(*Collection);
				*Collection = nullptr;
#endif
			}
			bool Collection::UpdateMany(TCollection* Collection, TDocument** Selector, TDocument** Update, TDocument** Options, TDocument** Reply)
			{
#ifdef TH_HAS_MONGOC
				if (!Collection)
				{
					Document::Release(Selector);
					Document::Release(Update);
					Document::Release(Options);
					return false;
				}

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!mongoc_collection_update_many(Collection, BS(Selector), BS(Update), BS(Options), BS(Reply), &Error))
				{
					Document::Release(Selector);
					Document::Release(Update);
					Document::Release(Options);
					return false;
				}

				Document::Release(Selector);
				Document::Release(Update);
				Document::Release(Options);
				return true;
#else
				return false;
#endif
			}
			bool Collection::UpdateOne(TCollection* Collection, TDocument** Selector, TDocument** Update, TDocument** Options, TDocument** Reply)
			{
#ifdef TH_HAS_MONGOC
				if (!Collection)
				{
					Document::Release(Selector);
					Document::Release(Update);
					Document::Release(Options);
					return false;
				}

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!mongoc_collection_update_one(Collection, BS(Selector), BS(Update), BS(Options), BS(Reply), &Error))
				{
					Document::Release(Selector);
					Document::Release(Update);
					Document::Release(Options);
					TH_ERROR("[mongoc] %s", Error.message);
					return false;
				}

				Document::Release(Selector);
				Document::Release(Update);
				Document::Release(Options);
				return true;
#else
				return false;
#endif
			}
			bool Collection::Rename(TCollection* Collection, const char* NewDatabaseName, const char* NewCollectionName)
			{
#ifdef TH_HAS_MONGOC
				if (!Collection)
					return false;

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!mongoc_collection_rename(Collection, NewDatabaseName, NewCollectionName, false, &Error))
				{
					TH_ERROR("[mongoc] %s", Error.message);
					return false;
				}

				return true;
#else
				return false;
#endif
			}
			bool Collection::RenameWithOptions(TCollection* Collection, const char* NewDatabaseName, const char* NewCollectionName, TDocument** Options)
			{
#ifdef TH_HAS_MONGOC
				if (!Collection)
				{
					Document::Release(Options);
					return false;
				}

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!mongoc_collection_rename_with_opts(Collection, NewDatabaseName, NewCollectionName, false, BS(Options), &Error))
				{
					Document::Release(Options);
					TH_ERROR("[mongoc] %s", Error.message);
					return false;
				}

				Document::Release(Options);
				return true;
#else
				return false;
#endif
			}
			bool Collection::RenameWithRemove(TCollection* Collection, const char* NewDatabaseName, const char* NewCollectionName)
			{
#ifdef TH_HAS_MONGOC
				if (!Collection)
					return false;

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!mongoc_collection_rename(Collection, NewDatabaseName, NewCollectionName, true, &Error))
				{
					TH_ERROR("[mongoc] %s", Error.message);
					return false;
				}

				return true;
#else
				return false;
#endif
			}
			bool Collection::RenameWithOptionsAndRemove(TCollection* Collection, const char* NewDatabaseName, const char* NewCollectionName, TDocument** Options)
			{
#ifdef TH_HAS_MONGOC
				if (!Collection)
				{
					Document::Release(Options);
					return false;
				}

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!mongoc_collection_rename_with_opts(Collection, NewDatabaseName, NewCollectionName, true, BS(Options), &Error))
				{
					Document::Release(Options);
					TH_ERROR("[mongoc] %s", Error.message);
					return false;
				}

				Document::Release(Options);
				return true;
#else
				return false;
#endif
			}
			bool Collection::Remove(TCollection* Collection, TDocument** Options)
			{
#ifdef TH_HAS_MONGOC
				if (!Collection)
				{
					Document::Release(Options);
					return false;
				}

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!mongoc_collection_drop_with_opts(Collection, BS(Options), &Error))
				{
					Document::Release(Options);
					TH_ERROR("[mongoc] %s", Error.message);
					return false;
				}

				Document::Release(Options);
				return true;
#else
				return false;
#endif
			}
			bool Collection::RemoveMany(TCollection* Collection, TDocument** Selector, TDocument** Options, TDocument** Reply)
			{
#ifdef TH_HAS_MONGOC
				if (!Collection)
				{
					Document::Release(Selector);
					Document::Release(Options);
					return false;
				}

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!mongoc_collection_delete_many(Collection, BS(Selector), BS(Options), BS(Reply), &Error))
				{
					Document::Release(Selector);
					Document::Release(Options);
					TH_ERROR("[mongoc] %s", Error.message);
					return false;
				}

				Document::Release(Selector);
				Document::Release(Options);
				return true;
#else
				return false;
#endif
			}
			bool Collection::RemoveOne(TCollection* Collection, TDocument** Selector, TDocument** Options, TDocument** Reply)
			{
#ifdef TH_HAS_MONGOC
				if (!Collection)
				{
					Document::Release(Selector);
					Document::Release(Options);
					return false;
				}

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!mongoc_collection_delete_one(Collection, BS(Selector), BS(Options), BS(Reply), &Error))
				{
					Document::Release(Selector);
					Document::Release(Options);
					TH_ERROR("[mongoc] %s", Error.message);
					return false;
				}

				Document::Release(Selector);
				Document::Release(Options);
				return true;
#else
				return false;
#endif
			}
			bool Collection::RemoveIndex(TCollection* Collection, const char* Name, TDocument** Options)
			{
#ifdef TH_HAS_MONGOC
				if (!Collection)
				{
					Document::Release(Options);
					return false;
				}

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!mongoc_collection_drop_index_with_opts(Collection, Name, BS(Options), &Error))
				{
					Document::Release(Options);
					TH_ERROR("[mongoc] %s", Error.message);
					return false;
				}

				Document::Release(Options);
				return true;
#else
				return false;
#endif
			}
			bool Collection::ReplaceOne(TCollection* Collection, TDocument** Selector, TDocument** Replacement, TDocument** Options, TDocument** Reply)
			{
#ifdef TH_HAS_MONGOC
				if (!Collection)
				{
					Document::Release(Selector);
					Document::Release(Replacement);
					Document::Release(Options);
					return false;
				}

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!mongoc_collection_replace_one(Collection, BS(Selector), BS(Replacement), BS(Options), BS(Reply), &Error))
				{
					Document::Release(Selector);
					Document::Release(Replacement);
					Document::Release(Options);
					TH_ERROR("[mongoc] %s", Error.message);
					return false;
				}

				Document::Release(Selector);
				Document::Release(Replacement);
				Document::Release(Options);
				return true;
#else
				return false;
#endif
			}
			bool Collection::InsertMany(TCollection* Collection, TDocument** Documents, uint64_t Length, TDocument** Options, TDocument** Reply)
			{
#ifdef TH_HAS_MONGOC
				if (!Collection || !Documents || !Length)
				{
					Document::Release(Options);
					return false;
				}

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!mongoc_collection_insert_many(Collection, (const TDocument**)Documents, (size_t)Length, BS(Options), BS(Reply), &Error))
				{
					Document::Release(Options);
					TH_ERROR("[mongoc] %s", Error.message);
					return false;
				}

				Document::Release(Options);
				return true;
#else
				return false;
#endif
			}
			bool Collection::InsertMany(TCollection* Collection, const std::vector<TDocument*>& Documents, TDocument** Options, TDocument** Reply)
			{
#ifdef TH_HAS_MONGOC
				if (!Collection || !Documents.size())
				{
					Document::Release(Options);
					return false;
				}

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!mongoc_collection_insert_many(Collection, (const TDocument**)Documents.data(), (size_t)Documents.size(), BS(Options), BS(Reply), &Error))
				{
					Document::Release(Options);
					TH_ERROR("[mongoc] %s", Error.message);
					return false;
				}

				Document::Release(Options);
				return true;
#else
				return false;
#endif
			}
			bool Collection::InsertOne(TCollection* Collection, TDocument** Document, TDocument** Options, TDocument** Reply)
			{
#ifdef TH_HAS_MONGOC
				if (!Collection)
				{
					Document::Release(Options);
					Document::Release(Document);
					return false;
				}

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!mongoc_collection_insert_one(Collection, BS(Document), BS(Options), BS(Reply), &Error))
				{
					Document::Release(Options);
					Document::Release(Document);
					TH_ERROR("[mongoc] %s", Error.message);
					return false;
				}

				Document::Release(Options);
				Document::Release(Document);
				return true;
#else
				return false;
#endif
			}
			bool Collection::FindAndModify(TCollection* Collection, TDocument** Query, TDocument** Sort, TDocument** Update, TDocument** Fields, TDocument** Reply, bool RemoveAt, bool Upsert, bool New)
			{
#ifdef TH_HAS_MONGOC
				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!Collection)
				{
					Document::Release(Query);
					Document::Release(Sort);
					Document::Release(Update);
					Document::Release(Fields);
					return false;
				}

				if (!mongoc_collection_find_and_modify(Collection, BS(Query), BS(Sort), BS(Update), BS(Fields), RemoveAt, Upsert, New, BS(Reply), &Error))
				{
					Document::Release(Query);
					Document::Release(Sort);
					Document::Release(Update);
					Document::Release(Fields);
					TH_ERROR("[mongoc] %s", Error.message);
					return false;
				}

				Document::Release(Query);
				Document::Release(Sort);
				Document::Release(Update);
				Document::Release(Fields);
				return true;
#else
				return false;
#endif
			}
			uint64_t Collection::CountElementsInArray(TCollection* Collection, TDocument** Match, TDocument** Filter, TDocument** Options)
			{
#ifdef TH_HAS_MONGOC
				if (!Collection || !Filter || !*Filter || !Match || !*Match)
				{
					Document::Release(Filter);
					Document::Release(Match);
					Document::Release(Options);
					return 0;
				}

				TDocument* Pipeline = BCON_NEW("pipeline", "[", "{", "$match", BCON_DOCUMENT(*Match), "}", "{", "$project", "{", "Count", "{", "$Count", "{", "$filter", BCON_DOCUMENT(*Filter), "}", "}", "}", "}", "]");
				if (!Pipeline)
				{
					Document::Release(Filter);
					Document::Release(Match);
					Document::Release(Options);
					return 0;
				}

				TCursor* Cursor = mongoc_collection_aggregate(Collection, MONGOC_QUERY_NONE, Pipeline, BS(Options), nullptr);
				Document::Release(&Pipeline);
				Document::Release(Filter);
				Document::Release(Match);
				Document::Release(Options);

				if (!Cursor || !Cursor::Next(Cursor))
				{
					Cursor::Release(&Cursor);
					return 0;
				}

				TDocument* Result = Cursor::GetCurrent(Cursor);
				if (!Result)
				{
					Cursor::Release(&Cursor);
					return 0;
				}

				Property Count;
				Document::Get(Result, "Count", &Count);
				Cursor::Release(&Cursor);

				return (uint64_t)Count.Integer;
#else
				return 0;
#endif
			}
			uint64_t Collection::CountDocuments(TCollection* Collection, TDocument** Filter, TDocument** Options, TDocument** Reply)
			{
#ifdef TH_HAS_MONGOC
				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!Collection)
				{
					Document::Release(Filter);
					Document::Release(Options);
					return 0;
				}

				uint64_t Count = (uint64_t)mongoc_collection_count_documents(Collection, BS(Filter), BS(Options), nullptr, BS(Reply), &Error);
				if (Error.code != 0)
				{
					Document::Release(Filter);
					Document::Release(Options);
					TH_ERROR("[mongoc] %s", Error.message);
					return false;
				}

				Document::Release(Filter);
				Document::Release(Options);
				return Count;
#else
				return 0;
#endif
			}
			uint64_t Collection::CountDocumentsEstimated(TCollection* Collection, TDocument** Options, TDocument** Reply)
			{
#ifdef TH_HAS_MONGOC
				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!Collection)
				{
					Document::Release(Options);
					return 0;
				}

				uint64_t Count = (uint64_t)mongoc_collection_estimated_document_count(Collection, BS(Options), nullptr, BS(Reply), &Error);
				if (Error.code != 0)
				{
					Document::Release(Options);
					TH_ERROR("[mongoc] %s", Error.message);
					return false;
				}

				Document::Release(Options);
				return Count;
#else
				return 0;
#endif
			}
			std::string Collection::StringifyKeyIndexes(TCollection* Collection, TDocument** Keys)
			{
#ifdef TH_HAS_MONGOC
				if (!Collection || !*Keys)
					return std::string();

				char* Value = mongoc_collection_keys_to_index_string(*Keys);
				std::string Output = Value;
				bson_free(Value);

				Document::Release(Keys);
				return Output;
#else
				return "";
#endif
			}
			const char* Collection::GetName(TCollection* Collection)
			{
#ifdef TH_HAS_MONGOC
				if (!Collection)
					return nullptr;

				return mongoc_collection_get_name(Collection);
#else
				return nullptr;
#endif
			}
			TCursor* Collection::FindIndexes(TCollection* Collection, TDocument** Options)
			{
#ifdef TH_HAS_MONGOC
				if (!Collection)
				{
					Document::Release(Options);
					return nullptr;
				}

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				TCursor* Cursor = mongoc_collection_find_indexes_with_opts(Collection, BS(Options));
				if (Cursor == nullptr || Error.code != 0)
				{
					Document::Release(Options);
					TH_ERROR("[mongoc] %s", Error.message);
					return nullptr;
				}

				Document::Release(Options);
				return Cursor;
#else
				return nullptr;
#endif
			}
			TCursor* Collection::FindMany(TCollection* Collection, TDocument** Filter, TDocument** Options)
			{
#ifdef TH_HAS_MONGOC
				if (!Collection)
				{
					Document::Release(Options);
					return nullptr;
				}

				TCursor* Result = mongoc_collection_find_with_opts(Collection, BS(Filter), BS(Options), nullptr);
				Document::Release(Filter);
				Document::Release(Options);

				return Result;
#else
				return nullptr;
#endif
			}
			TCursor* Collection::FindOne(TCollection* Collection, TDocument** Filter, TDocument** Options)
			{
#ifdef TH_HAS_MONGOC
				if (!Collection)
				{
					Document::Release(Filter);
					Document::Release(Options);
					return nullptr;
				}

				TDocument* Settings = (Options ? *Options : nullptr);
				if (Settings != nullptr)
					Document::SetInteger(Settings, "limit", 1);
				else
					Settings = BCON_NEW("limit", BCON_INT32(1));


				TCursor* Cursor = mongoc_collection_find_with_opts(Collection, BS(Filter), Settings, nullptr);
				if (!Options && Settings)
					bson_destroy(Settings);

				Document::Release(Filter);
				Document::Release(Options);
				return Cursor;
#else
				return nullptr;
#endif
			}
			TCursor* Collection::Aggregate(TCollection* Collection, Query Flags, TDocument** Pipeline, TDocument** Options)
			{
#ifdef TH_HAS_MONGOC
				if (!Collection)
				{
					Document::Release(Pipeline);
					Document::Release(Options);
					return nullptr;
				}

				TCursor* Result = mongoc_collection_aggregate(Collection, (mongoc_query_flags_t)Flags, BS(Pipeline), BS(Options), nullptr);

				Document::Release(Pipeline);
				Document::Release(Options);

				return Result;
#else
				return nullptr;
#endif
			}
			TStream* Collection::CreateStream(TCollection* Collection, TDocument** Options)
			{
#ifdef TH_HAS_MONGOC
				if (!Collection)
				{
					Document::Release(Options);
					return nullptr;
				}

				TStream* Operation = mongoc_collection_create_bulk_operation_with_opts(Collection, BS(Options));
				Document::Release(Options);

				return Operation;
#else
				return nullptr;
#endif
			}

			void Database::Release(TDatabase** Database)
			{
#ifdef TH_HAS_MONGOC
				if (!Database || !*Database)
					return;

				mongoc_database_destroy(*Database);
				*Database = nullptr;
#endif
			}
			bool Database::HasCollection(TDatabase* Database, const char* Name)
			{
#ifdef TH_HAS_MONGOC
				if (!Database)
					return false;

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!mongoc_database_has_collection(Database, Name, &Error))
				{
					TH_ERROR("[mongoc] %s", Error.message);
					return false;
				}

				return true;
#else
				return false;
#endif
			}
			bool Database::RemoveAllUsers(TDatabase* Database)
			{
#ifdef TH_HAS_MONGOC
				if (!Database)
					return false;

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!mongoc_database_remove_all_users(Database, &Error))
				{
					TH_ERROR("[mongoc] %s", Error.message);
					return false;
				}

				return true;
#else
				return false;
#endif
			}
			bool Database::RemoveUser(TDatabase* Database, const char* Name)
			{
#ifdef TH_HAS_MONGOC
				if (!Database)
					return false;

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!mongoc_database_remove_user(Database, Name, &Error))
				{
					TH_ERROR("[mongoc] %s", Error.message);
					return false;
				}

				return true;
#else
				return false;
#endif
			}
			bool Database::Remove(TDatabase* Database)
			{
#ifdef TH_HAS_MONGOC
				if (!Database)
					return false;

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!mongoc_database_drop(Database, &Error))
				{
					TH_ERROR("[mongoc] %s", Error.message);
					return false;
				}

				return true;
#else
				return false;
#endif
			}
			bool Database::RemoveWithOptions(TDatabase* Database, TDocument** Options)
			{
#ifdef TH_HAS_MONGOC
				if (!Database)
				{
					Document::Release(Options);
					return false;
				}

				if (!Options)
					return Remove(Database);

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!mongoc_database_drop_with_opts(Database, BS(Options), &Error))
				{
					Document::Release(Options);
					TH_ERROR("[mongoc] %s", Error.message);
					return false;
				}

				Document::Release(Options);
				return true;
#else
				return false;
#endif
			}
			bool Database::AddUser(TDatabase* Database, const char* Username, const char* Password, TDocument** Roles, TDocument** Custom)
			{
#ifdef TH_HAS_MONGOC
				if (!Database)
				{
					Document::Release(Roles);
					Document::Release(Custom);
					return false;
				}

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!mongoc_database_add_user(Database, Username, Password, BS(Roles), BS(Custom), &Error))
				{
					Document::Release(Roles);
					Document::Release(Custom);
					TH_ERROR("[mongoc] %s", Error.message);
					return false;
				}

				Document::Release(Roles);
				Document::Release(Custom);
				return true;
#else
				return false;
#endif
			}
			std::vector<std::string> Database::GetCollectionNames(TDatabase* Database, TDocument** Options)
			{
#ifdef TH_HAS_MONGOC
				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!Database)
					return std::vector<std::string>();

				char** Names = mongoc_database_get_collection_names_with_opts(Database, BS(Options), &Error);
				Document::Release(Options);

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
			const char* Database::GetName(TDatabase* Database)
			{
#ifdef TH_HAS_MONGOC
				if (!Database)
					return nullptr;

				return mongoc_database_get_name(Database);
#else
				return nullptr;
#endif
			}
			TCursor* Database::FindCollections(TDatabase* Database, TDocument** Options)
			{
#ifdef TH_HAS_MONGOC
				if (!Database)
				{
					Document::Release(Options);
					return nullptr;
				}

				TCursor* Cursor = mongoc_database_find_collections_with_opts(Database, BS(Options));
				Document::Release(Options);

				return Cursor;
#else
				return nullptr;
#endif
			}
			TCollection* Database::CreateCollection(TDatabase* Database, const char* Name, TDocument** Options)
			{
#ifdef TH_HAS_MONGOC
				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!Database)
				{
					Document::Release(Options);
					return nullptr;
				}

				TCollection* Collection = mongoc_database_create_collection(Database, Name, BS(Options), &Error);
				Document::Release(Options);

				if (Collection == nullptr)
					TH_ERROR("[mongoc] %s", Error.message);

				return Collection;
#else
				return nullptr;
#endif
			}
			TCollection* Database::GetCollection(TDatabase* Database, const char* Name)
			{
#ifdef TH_HAS_MONGOC
				if (!Database)
					return nullptr;

				return mongoc_database_get_collection(Database, Name);
#else
				return nullptr;
#endif
			}

			TTransaction* Transaction::Create(Connection* Client, TDocument** Uid)
			{
#ifdef TH_HAS_MONGOC
				if (!Client || !Uid)
					return nullptr;

				bson_error_t Error;
				if (!Client->Session)
				{
					Client->Session = mongoc_client_start_session(Client->Base, nullptr, &Error);
					if (!Client->Session)
					{
						TH_ERROR("[mongoc] couldn't create transaction\n\t%s", Error.message);
						return nullptr;
					}
				}

				if (!*Uid)
					*Uid = bson_new();

				if (!mongoc_client_session_append(Client->Session, *Uid, &Error))
				{
					TH_ERROR("[mongoc] could not set session uid\n\t%s", Error.message);
					return false;
				}

				return Client->Session;
#else
				return nullptr;
#endif
			}
			void Transaction::Release(Connection* Client)
			{
#ifdef TH_HAS_MONGOC
				if (Client != nullptr && Client->Session != nullptr)
				{
					mongoc_client_session_destroy(Client->Session);
					Client->Session = nullptr;
				}
#endif
			}
			bool Transaction::Start(TTransaction* Session)
			{
#ifdef TH_HAS_MONGOC
				if (!Session)
					return false;

				bson_error_t Error;
				if (!mongoc_client_session_start_transaction(Session, nullptr, &Error))
				{
					TH_ERROR("[mongoc] failed to start transaction\n\t%s", Error.message);
					return false;
				}

				return true;
#else
				return false;
#endif
			}
			bool Transaction::Abort(TTransaction* Session)
			{
#ifdef TH_HAS_MONGOC
				if (!Session)
					return false;

				return mongoc_client_session_abort_transaction(Session, nullptr);
#else
				return false;
#endif
			}
			bool Transaction::Commit(TTransaction* Session, TDocument** Reply)
			{
#ifdef TH_HAS_MONGOC
				if (!Session)
					return false;

				bson_error_t Error;
				if (!mongoc_client_session_commit_transaction(Session, BS(Reply), &Error))
				{
					TH_ERROR("[mongoc] could not commit transaction\n\t%s", Error.message);
					return false;
				}
				
				return true;
#else
				return false;
#endif
			}

			Connection::Connection() : Session(nullptr), Base(nullptr), Master(nullptr), Connected(false)
			{
				Driver::Create();
			}
			Connection::~Connection()
			{
				Transaction::Release(this);
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
			bool Connection::Connect(TAddress* URI)
			{
#ifdef TH_HAS_MONGOC
				if (Master != nullptr)
					return false;

				if (!URI)
					return false;

				if (Connected)
					Disconnect();

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
			TCursor* Connection::FindDatabases(TDocument** Options)
			{
#ifdef TH_HAS_MONGOC
				TCursor* Reference = mongoc_client_find_databases_with_opts(Base, BS(Options));
				Document::Release(Options);

				return Reference;
#else
				return nullptr;
#endif
			}
			TDatabase* Connection::GetDatabase(const char* Name)
			{
#ifdef TH_HAS_MONGOC
				return mongoc_client_get_database(Base, Name);
#else
				return nullptr;
#endif
			}
			TDatabase* Connection::GetDefaultDatabase()
			{
#ifdef TH_HAS_MONGOC
				return mongoc_client_get_default_database(Base);
#else
				return nullptr;
#endif
			}
			TCollection* Connection::GetCollection(const char* DatabaseName, const char* Name)
			{
#ifdef TH_HAS_MONGOC
				return mongoc_client_get_collection(Base, DatabaseName, Name);
#else
				return nullptr;
#endif
			}
			TConnection* Connection::Get()
			{
				return Base;
			}
			TAddress* Connection::GetAddress()
			{
#ifdef TH_HAS_MONGOC
				return (TAddress*)mongoc_client_get_uri(Base);
#else
				return nullptr;
#endif
			}
			Queue* Connection::GetMaster()
			{
				return Master;
			}
			std::vector<std::string> Connection::GetDatabaseNames(TDocument** Options)
			{
#ifdef TH_HAS_MONGOC
				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				char** Names = mongoc_client_get_database_names_with_opts(Base, BS(Options), &Error);
				Document::Release(Options);

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

			Queue::Queue() : Pool(nullptr), Address(nullptr), Connected(false)
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

				Address = mongoc_uri_new_with_error(URI.c_str(), &Error);
				if (!Address)
				{
					TH_ERROR("[urierr] %s", Error.message);
					return false;
				}

				Pool = mongoc_client_pool_new(Address);
				if (Pool == nullptr)
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
			bool Queue::Connect(TAddress* URI)
			{
#ifdef TH_HAS_MONGOC
				if (!URI)
					return false;

				if (Connected)
					Disconnect();

				Pool = mongoc_client_pool_new(Address = URI);
				if (Pool == nullptr)
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
			bool Queue::Disconnect()
			{
#ifdef TH_HAS_MONGOC
				if (Pool != nullptr)
				{
					mongoc_client_pool_destroy(Pool);
					Pool = nullptr;
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
			TConnectionPool* Queue::Get()
			{
				return Pool;
			}
			TAddress* Queue::GetAddress()
			{
				return Address;
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
							Document::Release(&Query.second.Cache);

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
					Result.Cache = Document::Create(Result.Request);

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

				Document::Release(&It->second.Cache);
				Queries->erase(It);
				Safe->unlock();
				return true;
			}
			TDocument* Driver::GetQuery(const std::string& Name, QueryMap* Map, bool Once)
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

				if (It->second.Cache != nullptr)
				{
					TDocument* Result = Document::Copy(It->second.Cache);
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
					TDocument* Result = Document::Create(It->second.Request);
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

				TDocument* Data = Document::Create(Origin.Request);
				if (!Data)
					TH_ERROR("could not construct query: \"%s\"", Name.c_str());

				return Data;
			}
			TDocument* Driver::GetSubquery(const char* Buffer, QueryMap* Map, bool Once)
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

					return Document::Create((const unsigned char*)Buffer, strlen(Buffer));
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

				TDocument* Data = Document::Create(Result.R());
				if (!Data)
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

						return "{\"$oid\":\"" + Util::OIdToString(Source->Value.GetBase64()) + "\"}";
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