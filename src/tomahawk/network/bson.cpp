#include "bson.h"
#include "../core/compute.h"
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
		namespace BSON
		{
			KeyPair::~KeyPair()
			{
				Release();
			}
			void KeyPair::Release()
			{
				BSON::Document::Release(&Document);
				BSON::Document::Release(&Array);
				Name.clear();
				String.clear();

				Integer = 0;
				Number = 0;
				Boolean = false;
				Mod = Type_Unknown;
				IsValid = false;
			}
			std::string& KeyPair::ToString()
			{
				switch (Mod)
				{
					case BSON::Type_Document:
						return String.assign("{}");
					case BSON::Type_Array:
						return String.assign("[]");
					case BSON::Type_String:
						return String;
					case BSON::Type_Boolean:
						return String.assign(Boolean ? "true" : "false");
					case BSON::Type_Number:
						return String.assign(std::to_string(Number));
					case BSON::Type_Integer:
						return String.assign(std::to_string(Integer));
					case BSON::Type_ObjectId:
						return String.assign(Compute::MathCommon::Base64Encode((const char*)ObjectId));
					case BSON::Type_Null:
						return String.assign("null");
					case BSON::Type_Unknown:
					case BSON::Type_Uncastable:
						return String.assign("undefined");
					case BSON::Type_Decimal:
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
				if (!Document || !Document->IsObject())
					return nullptr;

				TDocument* New = bson_new();
				TDocument* Replica = nullptr;
				uint64_t Index = 0;

				bool Array = (Document->Type == Rest::NodeType_Array);
				for (auto&& Node : *Document->GetNodes())
				{
					switch (Node->Type)
					{
						case Rest::NodeType_Object:
							Replica = Create(Node);
							AddKeyDocument(New, Array ? nullptr : Node->Name.c_str(), &Replica, Index);
							break;
						case Rest::NodeType_Array:
							Replica = Create(Node);
							AddKeyArray(New, Array ? nullptr : Node->Name.c_str(), &Replica, Index);
							break;
						case Rest::NodeType_String:
							AddKeyString(New, Array ? nullptr : Node->Name.c_str(), Node->String.c_str(), Index);
							break;
						case Rest::NodeType_Boolean:
							AddKeyBoolean(New, Array ? nullptr : Node->Name.c_str(), Node->Boolean, Index);
							break;
						case Rest::NodeType_Decimal:
							AddKeyDecimal(New, Array ? nullptr : Node->Name.c_str(), Node->Integer, Node->Low, Index);
							break;
						case Rest::NodeType_Number:
							AddKeyNumber(New, Array ? nullptr : Node->Name.c_str(), Node->Number, Index);
							break;
						case Rest::NodeType_Integer:
							AddKeyInteger(New, Array ? nullptr : Node->Name.c_str(), Node->Integer, Index);
							break;
						case Rest::NodeType_Null:
							AddKeyNull(New, Array ? nullptr : Node->Name.c_str(), Index);
							break;
						case Rest::NodeType_Id:
							AddKeyObjectId(New, Array ? nullptr : Node->Name.c_str(), (unsigned char*)Node->String.c_str(), Index);
							break;
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
			void Document::Loop(TDocument* Document, void* Context, const std::function<bool(TDocument*, KeyPair*, void*)>& Callback)
			{
				if (!Callback || !Document)
					return;

#ifdef TH_HAS_MONGOC
				bson_iter_t It;
				if (!bson_iter_init(&It, Document))
					return;

				KeyPair Output;
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
			bool Document::GenerateId(unsigned char* Id12)
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
			bool Document::AddKeyDocument(TDocument* Document, const char* Key, TDocument** Value, uint64_t ArrayId)
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
			bool Document::AddKeyArray(TDocument* Document, const char* Key, TDocument** Value, uint64_t ArrayId)
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
			bool Document::AddKeyString(TDocument* Document, const char* Key, const char* Value, uint64_t ArrayId)
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
			bool Document::AddKeyStringBuffer(TDocument* Document, const char* Key, const char* Value, uint64_t Length, uint64_t ArrayId)
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
			bool Document::AddKeyInteger(TDocument* Document, const char* Key, int64_t Value, uint64_t ArrayId)
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
			bool Document::AddKeyNumber(TDocument* Document, const char* Key, double Value, uint64_t ArrayId)
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
			bool Document::AddKeyDecimal(TDocument* Document, const char* Key, uint64_t High, uint64_t Low, uint64_t ArrayId)
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
			bool Document::AddKeyDecimalString(TDocument* Document, const char* Key, const std::string& Value, uint64_t ArrayId)
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
			bool Document::AddKeyDecimalInteger(TDocument* Document, const char* Key, int64_t Value, uint64_t ArrayId)
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
			bool Document::AddKeyDecimalNumber(TDocument* Document, const char* Key, double Value, uint64_t ArrayId)
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
			bool Document::AddKeyBoolean(TDocument* Document, const char* Key, bool Value, uint64_t ArrayId)
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
			bool Document::AddKeyObjectId(TDocument* Document, const char* Key, unsigned char Value[12], uint64_t ArrayId)
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
			bool Document::AddKeyNull(TDocument* Document, const char* Key, uint64_t ArrayId)
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
			bool Document::AddKey(TDocument* Document, const char* Key, KeyPair* Value, uint64_t ArrayId)
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
					case BSON::Type_Document:
						Object = Copy(Value->Document);
						return AddKeyDocument(Document, Key, &Object);
					case BSON::Type_Array:
						Object = Copy(Value->Document);
						return AddKeyArray(Document, Key, &Object);
					case BSON::Type_String:
						return AddKeyString(Document, Key, Value->String.c_str());
					case BSON::Type_Boolean:
						return AddKeyBoolean(Document, Key, Value->Boolean);
					case BSON::Type_Number:
						return AddKeyNumber(Document, Key, Value->Number);
					case BSON::Type_Integer:
						return AddKeyInteger(Document, Key, Value->Integer);
					case BSON::Type_Decimal:
						return AddKeyDecimal(Document, Key, Value->High, Value->Low);
					case BSON::Type_ObjectId:
						return AddKeyObjectId(Document, Key, Value->ObjectId);
					default:
						break;
				}

				return false;
#else
				return false;
#endif
			}
			bool Document::HasKey(TDocument* Document, const char* Key)
			{
#ifdef TH_HAS_MONGOC
				if (!Document)
					return false;

				return bson_has_field(Document, Key);
#else
				return false;
#endif
			}
			bool Document::GetKey(TDocument* Document, const char* Key, KeyPair* Output)
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
			bool Document::GetKeyWithNotation(TDocument* Document, const char* Key, KeyPair* Output)
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
			bool Document::ParseDecimal(const char* Value, int64_t* High, int64_t* Low)
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
			bool Document::Clone(void* It, KeyPair* Output)
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
			unsigned int Document::GetHashId(unsigned char* Id12)
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
			int64_t Document::GetTimeId(unsigned char* Id12)
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
			uint64_t Document::CountKeys(TDocument* Document)
			{
#ifdef TH_HAS_MONGOC
				if (!Document)
					return 0;

				return bson_count_keys(Document);
#else
				return 0;
#endif
			}
			std::string Document::OIdToString(unsigned char* Id12)
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
			std::string Document::StringToOId(const std::string& Id24)
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
			std::string Document::ToRelaxedAndExtendedJSON(TDocument* Document)
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
			std::string Document::ToClassicJSON(TDocument* Document)
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

				Rest::Document* New = new Rest::Document();
				if (IsArray)
					New->Type = Rest::NodeType_Array;

				Loop(Document, New, [](TDocument* Base, KeyPair* Key, void* UserData) -> bool
				{
					Rest::Document* Node = (Rest::Document*)UserData;
					std::string Name = (Node->Type == Rest::NodeType_Array ? "" : Key->Name);
					double Decimal;

					switch (Key->Mod)
					{
						case BSON::Type_Document:
						{
							Node->SetDocument(Name, ToDocument(Key->Document, false));
							break;
						}
						case BSON::Type_Array:
						{
							Node->SetArray(Name, ToDocument(Key->Array, true));
							break;
						}
						case BSON::Type_String:
							Node->SetString(Name, Key->String.c_str());
							break;
						case BSON::Type_Boolean:
							Node->SetBoolean(Name, Key->Boolean);
							break;
						case BSON::Type_Number:
							Node->SetNumber(Name, Key->Number);
							break;
						case BSON::Type_Decimal:
							Decimal = Rest::Stroke(&Key->ToString()).ToFloat64();
							if (Decimal != (int64_t)Decimal)
								Node->SetNumber(Name, Decimal);
							else
								Node->SetInteger(Name, (int64_t)Decimal);
							break;
						case BSON::Type_Integer:
							Node->SetInteger(Name, Key->Integer);
							break;
						case BSON::Type_ObjectId:
							Node->SetId(Name, Key->ObjectId);
							break;
						case BSON::Type_Null:
							Node->SetNull(Name);
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
		}
	}
}