#include "mdb.h"
#include <inttypes.h>
extern "C"
{
#ifdef ED_HAS_MONGOC
#include <mongoc.h>
#endif
}
#define MongoExecuteQuery(Function, Context, ...) ExecuteQuery(ED_STRINGIFY(Function), Function, Context, ##__VA_ARGS__)
#define MongoExecuteCursor(Function, Context, ...) ExecuteCursor(ED_STRINGIFY(Function), Function, Context, ##__VA_ARGS__)

namespace Edge
{
	namespace Network
	{
		namespace MDB
		{
#ifdef ED_HAS_MONGOC
			typedef enum
			{
				BSON_FLAG_NONE = 0,
				BSON_FLAG_INLINE = (1 << 0),
				BSON_FLAG_STATIC = (1 << 1),
				BSON_FLAG_RDONLY = (1 << 2),
				BSON_FLAG_CHILD = (1 << 3),
				BSON_FLAG_IN_CHILD = (1 << 4),
				BSON_FLAG_NO_FREE = (1 << 5),
			} BSON_FLAG;

			template <typename R, typename T, typename... Args>
			bool ExecuteQuery(const char* Name, R&& Function, T* Base, Args&&... Data)
			{
				ED_ASSERT(Base != nullptr, false, "context should be set");
				ED_MEASURE(ED_TIMING_MAX);
				ED_DEBUG("[mongoc] execute query schema on 0x%" PRIXPTR "\n\t%s", (uintptr_t)Base, Name + 1);

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				bool Result = Function(Base, Data..., &Error);
				if (!Result && Error.code != 0)
					ED_ERR("[mongoc:%i] %s", (int)Error.code, Error.message);

				if (Result || Error.code == 0)
					ED_DEBUG("[mongoc] OK execute on 0x%" PRIXPTR, (uintptr_t)Base);

				return Result;
			}
			template <typename R, typename T, typename... Args>
			Cursor ExecuteCursor(const char* Name, R&& Function, T* Base, Args&&... Data)
			{
				ED_ASSERT(Base != nullptr, nullptr, "context should be set");
				ED_MEASURE(ED_TIMING_MAX);
				ED_DEBUG("[mongoc] execute query cursor on 0x%" PRIXPTR "\n\t%s", (uintptr_t)Base, Name + 1);

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				TCursor* Result = Function(Base, Data...);
				if (!Result || mongoc_cursor_error(Result, &Error))
					ED_ERR("[mongoc:%i] %s", (int)Error.code, Error.message);
					
				if (Result || Error.code == 0)
					ED_DEBUG("[mongoc] OK execute on 0x%" PRIXPTR, (uintptr_t)Base);
				
				return Result;
			}
#endif
			Property::Property() noexcept : Source(nullptr), Mod(Type::Unknown), Integer(0), High(0), Low(0), Number(0.0), Boolean(false), IsValid(false)
			{
			}
			Property::Property(Property&& Other) : Name(std::move(Other.Name)), String(std::move(Other.String)), Source(Other.Source), Mod(Other.Mod), Integer(Other.Integer), High(Other.High), Low(Other.Low), Number(Other.Number), Boolean(Other.Boolean), IsValid(Other.IsValid)
			{
				if (Mod == Type::ObjectId)
					memcpy(ObjectId, Other.ObjectId, sizeof(ObjectId));

				Other.Source = nullptr;
			}
			Property::~Property()
			{
				Document Deleted(Source);
				Source = nullptr;
				Name.clear();
				String.clear();
				Integer = 0;
				Number = 0;
				Boolean = false;
				Mod = Type::Unknown;
				IsValid = false;
			}
			Property& Property::operator =(const Property& Other)
			{
				if (&Other == this)
					return *this;

				Document Deleted(Source);
				Name = std::move(Other.Name);
				String = std::move(Other.String);
				Mod = Other.Mod;
				Integer = Other.Integer;
				High = Other.High;
				Low = Other.Low;
				Number = Other.Number;
				Boolean = Other.Boolean;
				IsValid = Other.IsValid;
#ifdef ED_HAS_MONGOC
				if (Other.Source != nullptr)
					Source = bson_copy(Other.Source);
				else
					Source = nullptr;
#endif
				if (Mod == Type::ObjectId)
					memcpy(ObjectId, Other.ObjectId, sizeof(ObjectId));

				return *this;
			}
			Property& Property::operator =(Property&& Other)
			{
				if (&Other == this)
					return *this;

				Document Deleted(Source);
				Name = std::move(Other.Name);
				String = std::move(Other.String);
				Source = Other.Source;
				Mod = Other.Mod;
				Integer = Other.Integer;
				High = Other.High;
				Low = Other.Low;
				Number = Other.Number;
				Boolean = Other.Boolean;
				IsValid = Other.IsValid;
				Other.Source = nullptr;

				if (Mod == Type::ObjectId)
					memcpy(ObjectId, Other.ObjectId, sizeof(ObjectId));

				return *this;
			}
			Core::Unique<TDocument> Property::LoseOwnership()
			{
				if (!Source)
					return nullptr;

				TDocument* Result = Source;
				Source = nullptr;
				return Result;
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
						return String.assign(Compute::Codec::Bep45Encode((const char*)ObjectId));
					case Type::Null:
						return String.assign("null");
					case Type::Unknown:
					case Type::Uncastable:
						return String.assign("undefined");
					case Type::Decimal:
					{
#ifdef ED_HAS_MONGOC
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
			Document Property::Get() const
			{
				return Document(Source);
			}
			Property Property::operator [](const char* Label)
			{
				Property Result;
				Get().GetProperty(Label, &Result);

				return Result;
			}
			Property Property::operator [](const char* Label) const
			{
				Property Result;
				Get().GetProperty(Label, &Result);

				return Result;
			}

			bool Util::GetId(unsigned char* Id12)
			{
#ifdef ED_HAS_MONGOC
				ED_ASSERT(Id12 != nullptr, false, "id should be set");
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
				ED_ASSERT(Value != nullptr, false, "value should be set");
				ED_ASSERT(High != nullptr, false, "high should be set");
				ED_ASSERT(Low != nullptr, false, "low should be set");
#ifdef ED_HAS_MONGOC
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
#ifdef ED_HAS_MONGOC
				ED_ASSERT(Id12 != nullptr, 0, "id should be set");

				bson_oid_t Id;
				memcpy((void*)Id.bytes, (void*)Id12, sizeof(char) * 12);
				return bson_oid_hash(&Id);
#else
				return 0;
#endif
			}
			int64_t Util::GetTimeId(unsigned char* Id12)
			{
#ifdef ED_HAS_MONGOC
				ED_ASSERT(Id12 != nullptr, 0, "id should be set");

				bson_oid_t Id;
				memcpy((void*)Id.bytes, (void*)Id12, sizeof(char) * 12);

				return bson_oid_get_time_t(&Id);
#else
				return 0;
#endif
			}
			std::string Util::IdToString(unsigned char* Id12)
			{
#ifdef ED_HAS_MONGOC
				ED_ASSERT(Id12 != nullptr, std::string(), "id should be set");

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
				ED_ASSERT(Id24.size() == 24, std::string(), "id should be 24 chars long");
#ifdef ED_HAS_MONGOC
				bson_oid_t Id;
				bson_oid_init_from_string(&Id, Id24.c_str());

				return std::string((const char*)Id.bytes, 12);
#else
				return "";
#endif
			}

			Document::Document(TDocument* NewBase) : Base(NewBase), Store(false)
			{
			}
			Document::Document(Document&& Other) : Base(Other.Base), Store(Other.Store)
			{
				Other.Base = nullptr;
				Other.Store = false;
			}
			Document::~Document()
			{
				Cleanup();
			}
			Document& Document::operator =(Document&& Other)
			{
				if (&Other == this)
					return *this;

				Cleanup();
				Base = Other.Base;
				Store = Other.Store;
				Other.Base = nullptr;
				Other.Store = false;
				return *this;
			}
			void Document::Cleanup()
			{
#ifdef ED_HAS_MONGOC
				if (!Base || Store)
					return;

				if (!(Base->flags & BSON_FLAG_STATIC) && !(Base->flags & BSON_FLAG_RDONLY) && !(Base->flags & BSON_FLAG_INLINE) && !(Base->flags & BSON_FLAG_NO_FREE))
					bson_destroy((bson_t*)Base);

				Base = nullptr;
				Store = false;
#endif
			}
			void Document::Join(const Document& Value)
			{
#ifdef ED_HAS_MONGOC
				ED_ASSERT_V(Base != nullptr, "schema should be set");
				ED_ASSERT_V(Value.Base != nullptr, "other schema should be set");
				bson_concat((bson_t*)Base, (bson_t*)Value.Base);
#endif
			}
			void Document::Loop(const std::function<bool(Property*)>& Callback) const
			{
				ED_ASSERT_V(Base != nullptr, "schema should be set");
				ED_ASSERT_V(Callback, "callback should be set");
#ifdef ED_HAS_MONGOC
				bson_iter_t It;
				if (!bson_iter_init(&It, Base))
					return;

				Property Output;
				while (bson_iter_next(&It))
				{
					Clone(&It, &Output);
					if (!Callback(&Output))
						break;
				}
#endif
			}
			bool Document::SetSchema(const char* Key, const Document& Value, size_t ArrayId)
			{
#ifdef ED_HAS_MONGOC
				ED_ASSERT(Base != nullptr, false, "schema should be set");
				ED_ASSERT(Value.Base != nullptr, false, "other schema should be set");

				char Index[16];
				if (Key == nullptr)
					bson_uint32_to_string((uint32_t)ArrayId, &Key, Index, sizeof(Index));

				return bson_append_document((bson_t*)Base, Key, -1, (bson_t*)Value.Base);
#else
				return false;
#endif
			}
			bool Document::SetArray(const char* Key, const Document& Value, size_t ArrayId)
			{
#ifdef ED_HAS_MONGOC
				ED_ASSERT(Base != nullptr, false, "schema should be set");
				ED_ASSERT(Value.Base != nullptr, false, "other schema should be set");

				char Index[16];
				if (Key == nullptr)
					bson_uint32_to_string((uint32_t)ArrayId, &Key, Index, sizeof(Index));

				return bson_append_array((bson_t*)Base, Key, -1, (bson_t*)Value.Base);
#else
				return false;
#endif
			}
			bool Document::SetString(const char* Key, const char* Value, size_t ArrayId)
			{
#ifdef ED_HAS_MONGOC
				ED_ASSERT(Base != nullptr, false, "schema should be set");
				ED_ASSERT(Value != nullptr, false, "value should be set");

				char Index[16];
				if (Key == nullptr)
					bson_uint32_to_string((uint32_t)ArrayId, &Key, Index, sizeof(Index));

				return bson_append_utf8((bson_t*)Base, Key, -1, Value, -1);
#else
				return false;
#endif
			}
			bool Document::SetBlob(const char* Key, const char* Value, size_t Length, size_t ArrayId)
			{
#ifdef ED_HAS_MONGOC
				ED_ASSERT(Base != nullptr, false, "schema should be set");
				ED_ASSERT(Value != nullptr, false, "value should be set");

				char Index[16];
				if (Key == nullptr)
					bson_uint32_to_string((uint32_t)ArrayId, &Key, Index, sizeof(Index));

				return bson_append_utf8((bson_t*)Base, Key, -1, Value, (int)Length);
#else
				return false;
#endif
			}
			bool Document::SetInteger(const char* Key, int64_t Value, size_t ArrayId)
			{
#ifdef ED_HAS_MONGOC
				ED_ASSERT(Base != nullptr, false, "schema should be set");

				char Index[16];
				if (Key == nullptr)
					bson_uint32_to_string((uint32_t)ArrayId, &Key, Index, sizeof(Index));

				return bson_append_int64((bson_t*)Base, Key, -1, Value);
#else
				return false;
#endif
			}
			bool Document::SetNumber(const char* Key, double Value, size_t ArrayId)
			{
#ifdef ED_HAS_MONGOC
				ED_ASSERT(Base != nullptr, false, "schema should be set");

				char Index[16];
				if (Key == nullptr)
					bson_uint32_to_string((uint32_t)ArrayId, &Key, Index, sizeof(Index));

				return bson_append_double((bson_t*)Base, Key, -1, Value);
#else
				return false;
#endif
			}
			bool Document::SetDecimal(const char* Key, uint64_t High, uint64_t Low, size_t ArrayId)
			{
#ifdef ED_HAS_MONGOC
				ED_ASSERT(Base != nullptr, false, "schema should be set");

				char Index[16];
				if (Key == nullptr)
					bson_uint32_to_string((uint32_t)ArrayId, &Key, Index, sizeof(Index));

				bson_decimal128_t Decimal;
				Decimal.high = (uint64_t)High;
				Decimal.low = (uint64_t)Low;

				return bson_append_decimal128((bson_t*)Base, Key, -1, &Decimal);
#else
				return false;
#endif
			}
			bool Document::SetDecimalString(const char* Key, const std::string& Value, size_t ArrayId)
			{
#ifdef ED_HAS_MONGOC
				ED_ASSERT(Base != nullptr, false, "schema should be set");

				char Index[16];
				if (Key == nullptr)
					bson_uint32_to_string((uint32_t)ArrayId, &Key, Index, sizeof(Index));

				bson_decimal128_t Decimal;
				bson_decimal128_from_string(Value.c_str(), &Decimal);

				return bson_append_decimal128((bson_t*)Base, Key, -1, &Decimal);
#else
				return false;
#endif
			}
			bool Document::SetDecimalInteger(const char* Key, int64_t Value, size_t ArrayId)
			{
#ifdef ED_HAS_MONGOC
				ED_ASSERT(Base != nullptr, false, "schema should be set");

				char Index[16];
				if (Key == nullptr)
					bson_uint32_to_string((uint32_t)ArrayId, &Key, Index, sizeof(Index));

				char Data[64];
				snprintf(Data, 64, "%lld", Value);

				bson_decimal128_t Decimal;
				bson_decimal128_from_string(Data, &Decimal);

				return bson_append_decimal128((bson_t*)Base, Key, -1, &Decimal);
#else
				return false;
#endif
			}
			bool Document::SetDecimalNumber(const char* Key, double Value, size_t ArrayId)
			{
#ifdef ED_HAS_MONGOC
				ED_ASSERT(Base != nullptr, false, "schema should be set");

				char Index[16];
				if (Key == nullptr)
					bson_uint32_to_string((uint32_t)ArrayId, &Key, Index, sizeof(Index));

				char Data[64];
				snprintf(Data, 64, "%f", Value);

				for (size_t i = 0; i < 64; i++)
					Data[i] = (Data[i] == ',' ? '.' : Data[i]);

				bson_decimal128_t Decimal;
				bson_decimal128_from_string(Data, &Decimal);

				return bson_append_decimal128((bson_t*)Base, Key, -1, &Decimal);
#else
				return false;
#endif
			}
			bool Document::SetBoolean(const char* Key, bool Value, size_t ArrayId)
			{
#ifdef ED_HAS_MONGOC
				ED_ASSERT(Base != nullptr, false, "schema should be set");

				char Index[16];
				if (Key == nullptr)
					bson_uint32_to_string((uint32_t)ArrayId, &Key, Index, sizeof(Index));

				return bson_append_bool((bson_t*)Base, Key, -1, Value);
#else
				return false;
#endif
			}
			bool Document::SetObjectId(const char* Key, unsigned char Value[12], size_t ArrayId)
			{
#ifdef ED_HAS_MONGOC
				ED_ASSERT(Base != nullptr, false, "schema should be set");

				bson_oid_t ObjectId;
				memcpy(ObjectId.bytes, Value, sizeof(unsigned char) * 12);

				char Index[16];
				if (Key == nullptr)
					bson_uint32_to_string((uint32_t)ArrayId, &Key, Index, sizeof(Index));

				return bson_append_oid((bson_t*)Base, Key, -1, &ObjectId);
#else
				return false;
#endif
			}
			bool Document::SetNull(const char* Key, size_t ArrayId)
			{
#ifdef ED_HAS_MONGOC
				ED_ASSERT(Base != nullptr, false, "schema should be set");

				char Index[16];
				if (Key == nullptr)
					bson_uint32_to_string((uint32_t)ArrayId, &Key, Index, sizeof(Index));

				return bson_append_null((bson_t*)Base, Key, -1);
#else
				return false;
#endif
			}
			bool Document::SetProperty(const char* Key, Property* Value, size_t ArrayId)
			{
#ifdef ED_HAS_MONGOC
				ED_ASSERT(Base != nullptr, false, "schema should be set");
				ED_ASSERT(Value != nullptr, false, "property should be set");

				char Index[16];
				if (Key == nullptr)
					bson_uint32_to_string((uint32_t)ArrayId, &Key, Index, sizeof(Index));

				switch (Value->Mod)
				{
					case Type::Document:
						return SetSchema(Key, Value->Get().Copy());
					case Type::Array:
						return SetArray(Key, Value->Get().Copy());
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
			bool Document::HasProperty(const char* Key) const
			{
#ifdef ED_HAS_MONGOC
				ED_ASSERT(Base != nullptr, false, "schema should be set");
				ED_ASSERT(Key != nullptr, false, "key should be set");
				return bson_has_field(Base, Key);
#else
				return false;
#endif
			}
			bool Document::GetProperty(const char* Key, Property* Output) const
			{
#ifdef ED_HAS_MONGOC
				ED_ASSERT(Base != nullptr, false, "schema should be set");
				ED_ASSERT(Key != nullptr, false, "key should be set");
				ED_ASSERT(Output != nullptr, false, "property should be set");

				bson_iter_t It;
				if (!bson_iter_init_find(&It, Base, Key))
					return false;

				return Clone(&It, Output);
#else
				return false;
#endif
			}
			bool Document::Clone(void* It, Property* Output)
			{
#ifdef ED_HAS_MONGOC
				ED_ASSERT(It != nullptr, false, "iterator should be set");
				ED_ASSERT(Output != nullptr, false, "property should be set");

				const bson_value_t* Value = bson_iter_value((bson_iter_t*)It);
				if (!Value)
					return false;

				Property New;
				switch (Value->value_type)
				{
					case BSON_TYPE_DOCUMENT:
					{
						const uint8_t* Buffer; uint32_t Length;
						bson_iter_document((bson_iter_t*)It, &Length, &Buffer);
						New.Mod = Type::Document;
						New.Source = bson_new_from_data(Buffer, Length);
						break;
					}
					case BSON_TYPE_ARRAY:
					{
						const uint8_t* Buffer; uint32_t Length;
						bson_iter_array((bson_iter_t*)It, &Length, &Buffer);
						New.Mod = Type::Array;
						New.Source = bson_new_from_data(Buffer, Length);
						break;
					}
					case BSON_TYPE_BOOL:
						New.Mod = Type::Boolean;
						New.Boolean = Value->value.v_bool;
						break;
					case BSON_TYPE_INT32:
						New.Mod = Type::Integer;
						New.Integer = Value->value.v_int32;
						break;
					case BSON_TYPE_INT64:
						New.Mod = Type::Integer;
						New.Integer = Value->value.v_int64;
						break;
					case BSON_TYPE_DOUBLE:
						New.Mod = Type::Number;
						New.Number = Value->value.v_double;
						break;
					case BSON_TYPE_DECIMAL128:
						New.Mod = Type::Decimal;
						New.High = (uint64_t)Value->value.v_decimal128.high;
						New.Low = (uint64_t)Value->value.v_decimal128.low;
						break;
					case BSON_TYPE_UTF8:
						New.Mod = Type::String;
						New.String.assign(Value->value.v_utf8.str, (uint64_t)Value->value.v_utf8.len);
						break;
					case BSON_TYPE_TIMESTAMP:
						New.Mod = Type::Integer;
						New.Integer = (int64_t)Value->value.v_timestamp.timestamp;
						New.Number = (double)Value->value.v_timestamp.increment;
						break;
					case BSON_TYPE_DATE_TIME:
						New.Mod = Type::Integer;
						New.Integer = Value->value.v_datetime;
						break;
					case BSON_TYPE_REGEX:
						New.Mod = Type::String;
						New.String.assign(Value->value.v_regex.regex).append(1, '\n').append(Value->value.v_regex.options);
						break;
					case BSON_TYPE_CODE:
						New.Mod = Type::String;
						New.String.assign(Value->value.v_code.code, (uint64_t)Value->value.v_code.code_len);
						break;
					case BSON_TYPE_SYMBOL:
						New.Mod = Type::String;
						New.String.assign(Value->value.v_symbol.symbol, (uint64_t)Value->value.v_symbol.len);
						break;
					case BSON_TYPE_CODEWSCOPE:
						New.Mod = Type::String;
						New.String.assign(Value->value.v_codewscope.code, (uint64_t)Value->value.v_codewscope.code_len);
						break;
					case BSON_TYPE_UNDEFINED:
					case BSON_TYPE_NULL:
						New.Mod = Type::Null;
						break;
					case BSON_TYPE_OID:
						New.Mod = Type::ObjectId;
						memcpy(New.ObjectId, Value->value.v_oid.bytes, sizeof(unsigned char) * 12);
						break;
					case BSON_TYPE_EOD:
					case BSON_TYPE_BINARY:
					case BSON_TYPE_DBPOINTER:
					case BSON_TYPE_MAXKEY:
					case BSON_TYPE_MINKEY:
						New.Mod = Type::Uncastable;
						break;
					default:
						break;
				}

				New.Name.assign(bson_iter_key((const bson_iter_t*)It));
				New.IsValid = true;
				*Output = std::move(New);

				return true;
#else
				return false;
#endif
			}
			size_t Document::Count() const
			{
#ifdef ED_HAS_MONGOC
				if (!Base)
					return 0;

				return bson_count_keys(Base);
#else
				return 0;
#endif
			}
			std::string Document::ToRelaxedJSON() const
			{
#ifdef ED_HAS_MONGOC
				ED_ASSERT(Base != nullptr, std::string(), "schema should be set");
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
#ifdef ED_HAS_MONGOC
				ED_ASSERT(Base != nullptr, std::string(), "schema should be set");
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
#ifdef ED_HAS_MONGOC
				ED_ASSERT(Base != nullptr, std::string(), "schema should be set");
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
#ifdef ED_HAS_MONGOC
				ED_ASSERT(Base != nullptr, std::string(), "schema should be set");
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
			Core::Schema* Document::ToSchema(bool IsArray) const
			{
#ifdef ED_HAS_MONGOC
				if (!Base)
					return nullptr;

				Core::Schema* Node = (IsArray ? Core::Var::Set::Array() : Core::Var::Set::Object());
				Loop([Node](Property* Key) -> bool
				{
					std::string Name = (Node->Value.GetType() == Core::VarType::Array ? "" : Key->Name);
					switch (Key->Mod)
					{
						case Type::Document:
						{
							Node->Set(Name, Key->Get().ToSchema(false));
							break;
						}
						case Type::Array:
						{
							Node->Set(Name, Key->Get().ToSchema(true));
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
							Node->Set(Name, Core::Var::Binary(Key->ObjectId, 12));
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
#ifdef ED_HAS_MONGOC
				return Base;
#else
				return nullptr;
#endif
			}
			Document Document::Copy() const
			{
#ifdef ED_HAS_MONGOC
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
			Document Document::FromDocument(Core::Schema* Src)
			{
#ifdef ED_HAS_MONGOC
				ED_ASSERT(Src != nullptr && Src->Value.IsObject(), nullptr, "schema should be set");
				bool Array = (Src->Value.GetType() == Core::VarType::Array);
				Document Result = bson_new();
				size_t Index = 0;

				for (auto&& Node : Src->GetChilds())
				{
					switch (Node->Value.GetType())
					{
						case Core::VarType::Object:
							Result.SetSchema(Array ? nullptr : Node->Key.c_str(), Document::FromDocument(Node), Index);
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
						case Core::VarType::Binary:
						{
							if (Node->Value.GetSize() != 12)
							{
								std::string Base = Compute::Codec::Bep45Encode(Node->Value.GetBlob());
								Result.SetBlob(Array ? nullptr : Node->Key.c_str(), Base.c_str(), Base.size(), Index);
							}
							else
								Result.SetObjectId(Array ? nullptr : Node->Key.c_str(), Node->Value.GetBinary(), Index);
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
#ifdef ED_HAS_MONGOC
				return bson_new();
#else
				return nullptr;
#endif
			}
			Document Document::FromJSON(const std::string& JSON)
			{
#ifdef ED_HAS_MONGOC
				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				TDocument* Result = bson_new_from_json((unsigned char*)JSON.c_str(), (ssize_t)JSON.size(), &Error);
				if (Result != nullptr && Error.code == 0)
					return Result;

				if (Result != nullptr)
					bson_destroy((bson_t*)Result);

				ED_ERR("[json] %s", Error.message);
				return nullptr;
#else
				return nullptr;
#endif
			}
			Document Document::FromBuffer(const unsigned char* Buffer, size_t Length)
			{
#ifdef ED_HAS_MONGOC
				ED_ASSERT(Buffer != nullptr, nullptr, "buffer should be set");
				return bson_new_from_data(Buffer, Length);
#else
				return nullptr;
#endif
			}
			Document Document::FromSource(TDocument* Src)
			{
#ifdef ED_HAS_MONGOC
				ED_ASSERT(Src != nullptr, nullptr, "src should be set");
				TDocument* Dest = bson_new();
				bson_steal((bson_t*)Dest, (bson_t*)Src);
				return Dest;
#else
				return nullptr;
#endif
			}

			Address::Address(TAddress* NewBase) : Base(NewBase)
			{
			}
			Address::Address(Address&& Other) : Base(Other.Base)
			{
				Other.Base = nullptr;
			}
			Address::~Address()
			{
#ifdef ED_HAS_MONGOC
				if (Base != nullptr)
					mongoc_uri_destroy(Base);
				Base = nullptr;
#endif
			}
			Address& Address::operator =(Address&& Other)
			{
				if (&Other == this)
					return *this;

#ifdef ED_HAS_MONGOC
				if (Base != nullptr)
					mongoc_uri_destroy(Base);
#endif
				Base = Other.Base;
				Other.Base = nullptr;
				return *this;
			}
			void Address::SetOption(const char* Name, int64_t Value)
			{
#ifdef ED_HAS_MONGOC
				ED_ASSERT_V(Base != nullptr, "context should be set");
				ED_ASSERT_V(Name != nullptr, "name should be set");

				mongoc_uri_set_option_as_int32(Base, Name, (int32_t)Value);
#endif
			}
			void Address::SetOption(const char* Name, bool Value)
			{
#ifdef ED_HAS_MONGOC
				ED_ASSERT_V(Base != nullptr, "context should be set");
				ED_ASSERT_V(Name != nullptr, "name should be set");

				mongoc_uri_set_option_as_bool(Base, Name, Value);
#endif
			}
			void Address::SetOption(const char* Name, const char* Value)
			{
#ifdef ED_HAS_MONGOC
				ED_ASSERT_V(Base != nullptr, "context should be set");
				ED_ASSERT_V(Name != nullptr, "name should be set");
				ED_ASSERT_V(Value != nullptr, "value should be set");

				mongoc_uri_set_option_as_utf8(Base, Name, Value);
#endif
			}
			void Address::SetAuthMechanism(const char* Value)
			{
#ifdef ED_HAS_MONGOC
				ED_ASSERT_V(Base != nullptr, "context should be set");
				ED_ASSERT_V(Value != nullptr, "value should be set");

				mongoc_uri_set_auth_mechanism(Base, Value);
#endif
			}
			void Address::SetAuthSource(const char* Value)
			{
#ifdef ED_HAS_MONGOC
				ED_ASSERT_V(Base != nullptr, "context should be set");
				ED_ASSERT_V(Value != nullptr, "value should be set");

				mongoc_uri_set_auth_source(Base, Value);
#endif
			}
			void Address::SetCompressors(const char* Value)
			{
#ifdef ED_HAS_MONGOC
				ED_ASSERT_V(Base != nullptr, "context should be set");
				ED_ASSERT_V(Value != nullptr, "value should be set");

				mongoc_uri_set_compressors(Base, Value);
#endif
			}
			void Address::SetDatabase(const char* Value)
			{
#ifdef ED_HAS_MONGOC
				ED_ASSERT_V(Base != nullptr, "context should be set");
				ED_ASSERT_V(Value != nullptr, "value should be set");

				mongoc_uri_set_database(Base, Value);
#endif
			}
			void Address::SetUsername(const char* Value)
			{
#ifdef ED_HAS_MONGOC
				ED_ASSERT_V(Base != nullptr, "context should be set");
				ED_ASSERT_V(Value != nullptr, "value should be set");

				mongoc_uri_set_username(Base, Value);
#endif
			}
			void Address::SetPassword(const char* Value)
			{
#ifdef ED_HAS_MONGOC
				ED_ASSERT_V(Base != nullptr, "context should be set");
				ED_ASSERT_V(Value != nullptr, "value should be set");

				mongoc_uri_set_password(Base, Value);
#endif
			}
			TAddress* Address::Get() const
			{
#ifdef ED_HAS_MONGOC
				return Base;
#else
				return nullptr;
#endif
			}
			Address Address::FromURI(const char* Value)
			{
#ifdef ED_HAS_MONGOC
				ED_ASSERT(Value != nullptr, nullptr, "value should be set");
				TAddress* Result = mongoc_uri_new(Value);
				if (!strstr(Value, MONGOC_URI_SOCKETTIMEOUTMS))
					mongoc_uri_set_option_as_int32(Result, MONGOC_URI_SOCKETTIMEOUTMS, 10000);

				return Result;
#else
				return nullptr;
#endif
			}

			Stream::Stream() : NetOptions(nullptr), Source(nullptr), Base(nullptr), Count(0)
			{
			}
			Stream::Stream(TCollection* NewSource, TStream* NewBase, Document&& NewOptions) : NetOptions(std::move(NewOptions)), Source(NewSource), Base(NewBase), Count(0)
			{
			}
			Stream::Stream(Stream&& Other) : NetOptions(std::move(Other.NetOptions)), Source(Other.Source), Base(Other.Base), Count(Other.Count)
			{
				Other.Source = nullptr;
				Other.Base = nullptr;
				Other.Count = 0;
			}
			Stream::~Stream()
			{
#ifdef ED_HAS_MONGOC
				if (Base != nullptr)
					mongoc_bulk_operation_destroy(Base);

				Source = nullptr;
				Base = nullptr;
				Count = 0;
#endif
			}
			Stream& Stream::operator =(Stream&& Other)
			{
				if (&Other == this)
					return *this;

#ifdef ED_HAS_MONGOC
				if (Base != nullptr)
					mongoc_bulk_operation_destroy(Base);
#endif
				NetOptions = std::move(Other.NetOptions);
				Source = Other.Source;
				Base = Other.Base;
				Count = Other.Count;
				Other.Source = nullptr;
				Other.Base = nullptr;
				Other.Count = 0;
				return *this;
			}
			bool Stream::RemoveMany(const Document& Match, const Document& Options)
			{
#ifdef ED_HAS_MONGOC
				if (!NextOperation())
					return false;

				return MongoExecuteQuery(&mongoc_bulk_operation_remove_many_with_opts, Base, Match.Get(), Options.Get());
#else
				return false;
#endif
			}
			bool Stream::RemoveOne(const Document& Match, const Document& Options)
			{
#ifdef ED_HAS_MONGOC
				if (!NextOperation())
					return false;

				return MongoExecuteQuery(&mongoc_bulk_operation_remove_one_with_opts, Base, Match.Get(), Options.Get());
#else
				return false;
#endif
			}
			bool Stream::ReplaceOne(const Document& Match, const Document& Replacement, const Document& Options)
			{
#ifdef ED_HAS_MONGOC
				if (!NextOperation())
					return false;

				return MongoExecuteQuery(&mongoc_bulk_operation_replace_one_with_opts, Base, Match.Get(), Replacement.Get(), Options.Get());
#else
				return false;
#endif
			}
			bool Stream::InsertOne(const Document& Result, const Document& Options)
			{
#ifdef ED_HAS_MONGOC
				if (!NextOperation())
					return false;

				return MongoExecuteQuery(&mongoc_bulk_operation_insert_with_opts, Base, Result.Get(), Options.Get());
#else
				return false;
#endif
			}
			bool Stream::UpdateOne(const Document& Match, const Document& Result, const Document& Options)
			{
#ifdef ED_HAS_MONGOC
				if (!NextOperation())
					return false;

				return MongoExecuteQuery(&mongoc_bulk_operation_update_one_with_opts, Base, Match.Get(), Result.Get(), Options.Get());
#else
				return false;
#endif
			}
			bool Stream::UpdateMany(const Document& Match, const Document& Result, const Document& Options)
			{
#ifdef ED_HAS_MONGOC
				if (!NextOperation())
					return false;

				return MongoExecuteQuery(&mongoc_bulk_operation_update_many_with_opts, Base, Match.Get(), Result.Get(), Options.Get());
#else
				return false;
#endif
			}
			bool Stream::TemplateQuery(const std::string& Name, Core::SchemaArgs* Map, bool Once)
			{
				ED_DEBUG("[mongoc] template query %s", Name.empty() ? "empty-query-name" : Name.c_str());
				return Query(Driver::GetQuery(Name, Map, Once));
			}
			bool Stream::Query(const Document& Command)
			{
#ifdef ED_HAS_MONGOC
				if (!Command.Get())
				{
					ED_ERR("[mongoc] cannot run empty query");
					return false;
				}

				Property Type;
				if (!Command.GetProperty("type", &Type) || Type.Mod != Type::String)
				{
					ED_ERR("[mongoc] cannot run query without query @type");
					return false;
				}

				if (Type.String == "update")
				{
					Property Match;
					if (!Command.GetProperty("match", &Match) || Match.Mod != Type::Document)
					{
						ED_ERR("[mongoc] cannot run update-one query without @match");
						return false;
					}

					Property Update;
					if (!Command.GetProperty("update", &Update) || Update.Mod != Type::Document)
					{
						ED_ERR("[mongoc] cannot run update-one query without @update");
						return false;
					}

					Property Options = Command["options"];
					return UpdateOne(Match.LoseOwnership(), Update.LoseOwnership(), Options.LoseOwnership());
				}
				else if (Type.String == "update-many")
				{
					Property Match;
					if (!Command.GetProperty("match", &Match) || Match.Mod != Type::Document)
					{
						ED_ERR("[mongoc] cannot run update-many query without @match");
						return false;
					}

					Property Update;
					if (!Command.GetProperty("update", &Update) || Update.Mod != Type::Document)
					{
						ED_ERR("[mongoc] cannot run update-many query without @update");
						return false;
					}

					Property Options = Command["options"];
					return UpdateMany(Match.LoseOwnership(), Update.LoseOwnership(), Options.LoseOwnership());
				}
				else if (Type.String == "insert")
				{
					Property Value;
					if (!Command.GetProperty("value", &Value) || Value.Mod != Type::Document)
					{
						ED_ERR("[mongoc] cannot run insert-one query without @value");
						return false;
					}

					Property Options = Command["options"];
					return InsertOne(Value.LoseOwnership(), Options.LoseOwnership());
				}
				else if (Type.String == "replace")
				{
					Property Match;
					if (!Command.GetProperty("match", &Match) || Match.Mod != Type::Document)
					{
						ED_ERR("[mongoc] cannot run replace-one query without @match");
						return false;
					}

					Property Value;
					if (!Command.GetProperty("value", &Value) || Value.Mod != Type::Document)
					{
						ED_ERR("[mongoc] cannot run replace-one query without @value");
						return false;
					}

					Property Options = Command["options"];
					return ReplaceOne(Match.LoseOwnership(), Value.LoseOwnership(), Options.LoseOwnership());
				}
				else if (Type.String == "remove")
				{
					Property Match;
					if (!Command.GetProperty("match", &Match) || Match.Mod != Type::Document)
					{
						ED_ERR("[mongoc] cannot run remove-one query without @value");
						return false;
					}

					Property Options = Command["options"];
					return RemoveOne(Match.LoseOwnership(), Options.LoseOwnership());
				}
				else if (Type.String == "remove-many")
				{
					Property Match;
					if (!Command.GetProperty("match", &Match) || Match.Mod != Type::Document)
					{
						ED_ERR("[mongoc] cannot run remove-many query without @value");
						return false;
					}

					Property Options = Command["options"];
					return RemoveMany(Match.LoseOwnership(), Options.LoseOwnership());
				}

				ED_ERR("[mongoc] cannot find query of type \"%s\"", Type.String.c_str());
				return false;
#else
				return false;
#endif
			}
			bool Stream::NextOperation()
			{
#ifdef ED_HAS_MONGOC
				if (!Base || !Source)
					return false;

				bool State = true;
				if (Count > 768)
				{
					TDocument Result;
					State = MongoExecuteQuery(&mongoc_bulk_operation_execute, Base, &Result);
					bson_destroy((bson_t*)&Result);

					if (Source != nullptr)
						*this = Collection(Source).CreateStream(std::move(NetOptions));
				}
				else
					Count++;

				return State;
#else
				return false;
#endif
			}
			Core::Promise<Document> Stream::ExecuteWithReply()
			{
#ifdef ED_HAS_MONGOC
				if (!Base)
					return Document(nullptr);

				auto* Context = Base;
				return Core::Cotask<Document>([Context]() mutable
				{
					TDocument Subresult; Document Result;
					if (!MongoExecuteQuery(&mongoc_bulk_operation_execute, Context, &Subresult))
						Result = Document(nullptr);
					else
						Result = Document::FromSource(&Subresult);

					bson_destroy((bson_t*)&Subresult);
					return Result;
				});
#else
				return Document(nullptr);
#endif
			}
			Core::Promise<bool> Stream::Execute()
			{
#ifdef ED_HAS_MONGOC
				if (!Base)
					return true;

				auto* Context = Base;
				return Core::Cotask<bool>([Context]() mutable
				{
					TDocument Result;
					bool Subresult = MongoExecuteQuery(&mongoc_bulk_operation_execute, Context, &Result);
					bson_destroy((bson_t*)&Result);
					return Subresult;
				});
#else
				return false;
#endif
			}
			size_t Stream::GetHint() const
			{
#ifdef ED_HAS_MONGOC
				ED_ASSERT(Base != nullptr, 0, "context should be set");
				return (size_t)mongoc_bulk_operation_get_hint(Base);
#else
				return 0;
#endif
			}
			TStream* Stream::Get() const
			{
#ifdef ED_HAS_MONGOC
				return Base;
#else
				return nullptr;
#endif
			}

			Cursor::Cursor(TCursor* NewBase) : Base(NewBase)
			{
			}
			Cursor::Cursor(Cursor&& Other) : Base(Other.Base)
			{
				Other.Base = nullptr;
			}
			Cursor::~Cursor()
			{
				Cleanup();
			}
			Cursor& Cursor::operator =(Cursor&& Other)
			{
				if (&Other == this)
					return *this;

				Cleanup();
				Base = Other.Base;
				Other.Base = nullptr;
				return *this;
			}
			void Cursor::Cleanup()
			{
#ifdef ED_HAS_MONGOC
				if (!Base)
					return;

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (mongoc_cursor_error(Base, &Error))
					ED_ERR("[mongoc] %s", Error.message);

				mongoc_cursor_destroy(Base);
				Base = nullptr;
#endif
			}
			void Cursor::SetMaxAwaitTime(size_t MaxAwaitTime)
			{
#ifdef ED_HAS_MONGOC
				ED_ASSERT_V(Base != nullptr, "context should be set");
				mongoc_cursor_set_max_await_time_ms(Base, (uint32_t)MaxAwaitTime);
#endif
			}
			void Cursor::SetBatchSize(size_t BatchSize)
			{
#ifdef ED_HAS_MONGOC
				ED_ASSERT_V(Base != nullptr, "context should be set");
				mongoc_cursor_set_batch_size(Base, (uint32_t)BatchSize);
#endif
			}
			bool Cursor::SetLimit(int64_t Limit)
			{
#ifdef ED_HAS_MONGOC
				ED_ASSERT(Base != nullptr, false, "context should be set");
				return mongoc_cursor_set_limit(Base, Limit);
#else
				return false;
#endif
			}
			bool Cursor::SetHint(size_t Hint)
			{
#ifdef ED_HAS_MONGOC
				ED_ASSERT(Base != nullptr, false, "context should be set");
				return mongoc_cursor_set_hint(Base, (uint32_t)Hint);
#else
				return false;
#endif
			}
			bool Cursor::HasError() const
			{
#ifdef ED_HAS_MONGOC
				if (!Base)
					return true;

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!mongoc_cursor_error(Base, &Error))
					return false;

				ED_ERR("[mongoc] %s", Error.message);
				return true;
#else
				return false;
#endif
			}
			bool Cursor::HasMoreData() const
			{
#ifdef ED_HAS_MONGOC
				if (!Base)
					return false;

				return mongoc_cursor_more(Base);
#else
				return false;
#endif
			}
			Core::Promise<bool> Cursor::Next() const
			{
#ifdef ED_HAS_MONGOC
				if (!Base)
					return false;

				auto* Context = Base;
				return Core::Cotask<bool>([Context]()
				{
					ED_MEASURE(ED_TIMING_MAX);
					TDocument* Query = nullptr;
					return mongoc_cursor_next(Context, (const TDocument**)&Query);
				});
#else
				return false;
#endif
			}
			int64_t Cursor::GetId() const
			{
#ifdef ED_HAS_MONGOC
				ED_ASSERT(Base != nullptr, 0, "context should be set");
				return (int64_t)mongoc_cursor_get_id(Base);
#else
				return 0;
#endif
			}
			int64_t Cursor::GetLimit() const
			{
#ifdef ED_HAS_MONGOC
				ED_ASSERT(Base != nullptr, 0, "context should be set");
				return (int64_t)mongoc_cursor_get_limit(Base);
#else
				return 0;
#endif
			}
			size_t Cursor::GetMaxAwaitTime() const
			{
#ifdef ED_HAS_MONGOC
				ED_ASSERT(Base != nullptr, 0, "context should be set");
				return (size_t)mongoc_cursor_get_max_await_time_ms(Base);
#else
				return 0;
#endif
			}
			size_t Cursor::GetBatchSize() const
			{
#ifdef ED_HAS_MONGOC
				ED_ASSERT(Base != nullptr, 0, "context should be set");
				return (size_t)mongoc_cursor_get_batch_size(Base);
#else
				return 0;
#endif
			}
			size_t Cursor::GetHint() const
			{
#ifdef ED_HAS_MONGOC
				ED_ASSERT(Base != nullptr, 0, "context should be set");
				return (size_t)mongoc_cursor_get_hint(Base);
#else
				return 0;
#endif
			}
			Document Cursor::GetCurrent() const
			{
#ifdef ED_HAS_MONGOC
				if (!Base)
					return nullptr;

				return (TDocument*)mongoc_cursor_current(Base);
#else
				return nullptr;
#endif
			}
			Cursor Cursor::Clone()
			{
#ifdef ED_HAS_MONGOC
				ED_ASSERT(Base != nullptr, nullptr, "context should be set");
				return mongoc_cursor_clone(Base);
#else
				return nullptr;
#endif
			}
			TCursor* Cursor::Get() const
			{
#ifdef ED_HAS_MONGOC
				return Base;
#else
				return nullptr;
#endif
			}

			Response::Response(bool NewSuccess) : NetSuccess(NewSuccess)
			{
			}
			Response::Response(Cursor&& NewCursor) : NetCursor(std::move(NewCursor))
			{
				NetSuccess = NetCursor && !NetCursor.HasError() && NetCursor.HasMoreData();
			}
			Response::Response(Document&& NewDocument) : NetDocument(std::move(NewDocument))
			{
				NetSuccess = NetDocument.Get() != nullptr;
			}
			Response::Response(Response&& Other) : NetCursor(std::move(Other.NetCursor)), NetDocument(std::move(Other.NetDocument)), NetSuccess(Other.NetSuccess)
			{
				Other.NetSuccess = false;
			}
			Response& Response::operator =(Response&& Other)
			{
				if (&Other == this)
					return *this;

				NetCursor = std::move(Other.NetCursor);
				NetDocument = std::move(Other.NetDocument);
				NetSuccess = Other.NetSuccess;
				Other.NetSuccess = false;
				return *this;
			}
			Core::Promise<Core::Schema*> Response::Fetch() const
			{
				if (NetDocument)
					return NetDocument.ToSchema();

				if (!NetCursor)
					return (Core::Schema*)nullptr;

				return NetCursor.Next().Then<Core::Schema*>([this](bool&& Result)
				{
					return NetCursor.GetCurrent().ToSchema();
				});
			}
			Core::Promise<Core::Schema*> Response::FetchAll() const
			{
				if (NetDocument)
				{
					Core::Schema* Result = NetDocument.ToSchema();
					return Result ? Result : Core::Var::Set::Array();
				}

				if (!NetCursor)
					return Core::Var::Set::Array();

				return Core::Coasync<Core::Schema*>([this]()
				{
					Core::Schema* Result = Core::Var::Set::Array();
					while (ED_AWAIT(NetCursor.Next()))
						Result->Push(NetCursor.GetCurrent().ToSchema());

					return Result;
				});
			}
			Property Response::GetProperty(const char* Name)
			{
				ED_ASSERT(Name != nullptr, Property(), "context should be set");

				Property Result;
				if (NetDocument)
				{
					NetDocument.GetProperty(Name, &Result);
					return Result;
				}

				if (!NetCursor)
					return Result;

				Document Source = NetCursor.GetCurrent();
				if (Source)
				{
					Source.GetProperty(Name, &Result);
					return Result;
				}

				if (!ED_AWAIT(NetCursor.Next()))
					return Result;

				Source = NetCursor.GetCurrent();
				if (Source)
					Source.GetProperty(Name, &Result);

				return Result;
			}
			Cursor&& Response::GetCursor()
			{
				return std::move(NetCursor);
			}
			Document&& Response::GetDocument()
			{
				return std::move(NetDocument);
			}
			bool Response::IsSuccess() const
			{
				return NetSuccess;
			}

			Collection::Collection(TCollection* NewBase) : Base(NewBase)
			{
			}
			Collection::Collection(Collection&& Other) : Base(Other.Base)
			{
				Other.Base = nullptr;
			}
			Collection::~Collection()
			{
#ifdef ED_HAS_MONGOC
				if (Base != nullptr)
					mongoc_collection_destroy(Base);
				Base = nullptr;
#endif
			}
			Collection& Collection::operator =(Collection&& Other)
			{
				if (&Other == this)
					return *this;

#ifdef ED_HAS_MONGOC
				if (Base != nullptr)
					mongoc_collection_destroy(Base);
#endif
				Base = Other.Base;
				Other.Base = nullptr;
				return *this;
			}
			Core::Promise<bool> Collection::Rename(const std::string& NewDatabaseName, const std::string& NewCollectionName) const
			{
#ifdef ED_HAS_MONGOC
				auto* Context = Base;
				return Core::Cotask<bool>([Context, NewDatabaseName, NewCollectionName]()
				{
					return MongoExecuteQuery(&mongoc_collection_rename, Context, NewDatabaseName.c_str(), NewCollectionName.c_str(), false);
				});
#else
				return false;
#endif
			}
			Core::Promise<bool> Collection::RenameWithOptions(const std::string& NewDatabaseName, const std::string& NewCollectionName, const Document& Options) const
			{
#ifdef ED_HAS_MONGOC
				auto* Context = Base;
				return Core::Cotask<bool>([Context, NewDatabaseName, NewCollectionName, &Options]()
				{
					return MongoExecuteQuery(&mongoc_collection_rename_with_opts, Context, NewDatabaseName.c_str(), NewCollectionName.c_str(), false, Options.Get());
				});
#else
				return false;
#endif
			}
			Core::Promise<bool> Collection::RenameWithRemove(const std::string& NewDatabaseName, const std::string& NewCollectionName) const
			{
#ifdef ED_HAS_MONGOC
				auto* Context = Base;
				return Core::Cotask<bool>([Context, NewDatabaseName, NewCollectionName]()
				{
					return MongoExecuteQuery(&mongoc_collection_rename, Context, NewDatabaseName.c_str(), NewCollectionName.c_str(), true);
				});
#else
				return false;
#endif
			}
			Core::Promise<bool> Collection::RenameWithOptionsAndRemove(const std::string& NewDatabaseName, const std::string& NewCollectionName, const Document& Options) const
			{
#ifdef ED_HAS_MONGOC
				auto* Context = Base;
				return Core::Cotask<bool>([Context, NewDatabaseName, NewCollectionName, &Options]()
				{
					return MongoExecuteQuery(&mongoc_collection_rename_with_opts, Context, NewDatabaseName.c_str(), NewCollectionName.c_str(), true, Options.Get());
				});
#else
				return false;
#endif
			}
			Core::Promise<bool> Collection::Remove(const Document& Options) const
			{
#ifdef ED_HAS_MONGOC
				auto* Context = Base;
				return Core::Cotask<bool>([Context, &Options]()
				{
					return MongoExecuteQuery(&mongoc_collection_drop_with_opts, Context, Options.Get());
				});
#else
				return false;
#endif
			}
			Core::Promise<bool> Collection::RemoveIndex(const std::string& Name, const Document& Options) const
			{
#ifdef ED_HAS_MONGOC
				auto* Context = Base;
				return Core::Cotask<bool>([Context, Name, &Options]()
				{
					return MongoExecuteQuery(&mongoc_collection_drop_index_with_opts, Context, Name.c_str(), Options.Get());
				});
#else
				return false;
#endif
			}
			Core::Promise<Document> Collection::RemoveMany(const Document& Match, const Document& Options) const
			{
#ifdef ED_HAS_MONGOC
				auto* Context = Base;
				return Core::Cotask<Document>([Context, &Match, &Options]()
				{
					TDocument Subresult; Document Result;
					if (!MongoExecuteQuery(&mongoc_collection_delete_many, Context, Match.Get(), Options.Get(), &Subresult))
						Result = Document(nullptr);
					else
						Result = Document::FromSource(&Subresult);

					bson_destroy((bson_t*)&Subresult);
					return Result;
				});
#else
				return Document(nullptr);
#endif
			}
			Core::Promise<Document> Collection::RemoveOne(const Document& Match, const Document& Options) const
			{
#ifdef ED_HAS_MONGOC
				auto* Context = Base;
				return Core::Cotask<Document>([Context, &Match, &Options]()
				{
					TDocument Subresult; Document Result;
					if (!MongoExecuteQuery(&mongoc_collection_delete_one, Context, Match.Get(), Options.Get(), &Subresult))
						Result = Document(nullptr);
					else
						Result = Document::FromSource(&Subresult);

					bson_destroy((bson_t*)&Subresult);
					return Result;
				});
#else
				return Document(nullptr);
#endif
			}
			Core::Promise<Document> Collection::ReplaceOne(const Document& Match, const Document& Replacement, const Document& Options) const
			{
#ifdef ED_HAS_MONGOC
				auto* Context = Base;
				return Core::Cotask<Document>([Context, &Match, &Replacement, &Options]()
				{
					TDocument Subresult; Document Result;
					if (!MongoExecuteQuery(&mongoc_collection_replace_one, Context, Match.Get(), Replacement.Get(), Options.Get(), &Subresult))
						Result = Document(nullptr);
					else
						Result = Document::FromSource(&Subresult);

					bson_destroy((bson_t*)&Subresult);
					return Result;
				});
#else
				return Document(nullptr);
#endif
			}
			Core::Promise<Document> Collection::InsertMany(std::vector<Document>& List, const Document& Options) const
			{
#ifdef ED_HAS_MONGOC
				ED_ASSERT(!List.empty(), Document(nullptr), "insert array should not be empty");
				std::vector<Document> Array(std::move(List));
				auto* Context = Base;

				return Core::Cotask<Document>([Context, &Array, &Options]()
				{
					std::vector<TDocument*> Subarray;
				    Subarray.reserve(Array.size());

				    for (auto& Item : Array)
						Subarray.push_back(Item.Get());

					TDocument Subresult; Document Result;
					if (!MongoExecuteQuery(&mongoc_collection_insert_many, Context, (const TDocument**)Subarray.data(), (size_t)Array.size(), Options.Get(), &Subresult))
						Result = Document(nullptr);
					else
						Result = Document::FromSource(&Subresult);

					bson_destroy((bson_t*)&Subresult);
					return Result;
				});
#else
				return Document(nullptr);
#endif
			}
			Core::Promise<Document> Collection::InsertOne(const Document& Result, const Document& Options) const
			{
#ifdef ED_HAS_MONGOC
				auto* Context = Base;
				return Core::Cotask<Document>([Context, &Result, &Options]()
				{
					TDocument Subresult; Document Returns;
					if (!MongoExecuteQuery(&mongoc_collection_insert_one, Context, Result.Get(), Options.Get(), &Subresult))
						Returns = Document(nullptr);
					else
						Returns = Document::FromSource(&Subresult);

					bson_destroy((bson_t*)&Subresult);
					return Returns;
				});
#else
				return Document(nullptr);
#endif
			}
			Core::Promise<Document> Collection::UpdateMany(const Document& Match, const Document& Update, const Document& Options) const
			{
#ifdef ED_HAS_MONGOC
				auto* Context = Base;
				return Core::Cotask<Document>([Context, &Match, &Update, &Options]()
				{
					TDocument Subresult; Document Result;
					if (!MongoExecuteQuery(&mongoc_collection_update_many, Context, Match.Get(), Update.Get(), Options.Get(), &Subresult))
						Result = Document(nullptr);
					else
						Result = Document::FromSource(&Subresult);

					bson_destroy((bson_t*)&Subresult);
					return Result;
				});
#else
				return Document(nullptr);
#endif
			}
			Core::Promise<Document> Collection::UpdateOne(const Document& Match, const Document& Update, const Document& Options) const
			{
#ifdef ED_HAS_MONGOC
				auto* Context = Base;
				return Core::Cotask<Document>([Context, &Match, &Update, &Options]()
				{
					TDocument Subresult; Document Result;
					if (!MongoExecuteQuery(&mongoc_collection_update_one, Context, Match.Get(), Update.Get(), Options.Get(), &Subresult))
						Result = Document(nullptr);
					else
						Result = Document::FromSource(&Subresult);

					bson_destroy((bson_t*)&Subresult);
					return Result;
				});
#else
				return Document(nullptr);
#endif
			}
			Core::Promise<Document> Collection::FindAndModify(const Document& Query, const Document& Sort, const Document& Update, const Document& Fields, bool RemoveAt, bool Upsert, bool New) const
			{
#ifdef ED_HAS_MONGOC
				auto* Context = Base;
				return Core::Cotask<Document>([Context, &Query, &Sort, &Update, &Fields, RemoveAt, Upsert, New]()
				{
					TDocument Subresult; Document Result;
					if (!MongoExecuteQuery(&mongoc_collection_find_and_modify, Context, Query.Get(), Sort.Get(), Update.Get(), Fields.Get(), RemoveAt, Upsert, New, &Subresult))
						Result = Document(nullptr);
					else
						Result = Document::FromSource(&Subresult);

					bson_destroy((bson_t*)&Subresult);
					return Result;
				});
#else
				return Document(nullptr);
#endif
			}
			Core::Promise<size_t> Collection::CountDocuments(const Document& Match, const Document& Options) const
			{
#ifdef ED_HAS_MONGOC
				auto* Context = Base;
				return Core::Cotask<size_t>([Context, &Match, &Options]()
				{
					return (size_t)MongoExecuteQuery(&mongoc_collection_count_documents, Context, Match.Get(), Options.Get(), nullptr, nullptr);
				});
#else
				return (size_t)0;
#endif
			}
			Core::Promise<size_t> Collection::CountDocumentsEstimated(const Document& Options) const
			{
#ifdef ED_HAS_MONGOC
				auto* Context = Base;
				return Core::Cotask<size_t>([Context, &Options]()
				{
					return (size_t)MongoExecuteQuery(&mongoc_collection_estimated_document_count, Context, Options.Get(), nullptr, nullptr);
				});
#else
				return (size_t)0;
#endif
			}
			Core::Promise<Cursor> Collection::FindIndexes(const Document& Options) const
			{
#ifdef ED_HAS_MONGOC
				auto* Context = Base;
				return Core::Cotask<Cursor>([Context, &Options]()
				{
					return MongoExecuteCursor(&mongoc_collection_find_indexes_with_opts, Context, Options.Get());
				});
#else
				return Cursor(nullptr);
#endif
			}
			Core::Promise<Cursor> Collection::FindMany(const Document& Match, const Document& Options) const
			{
#ifdef ED_HAS_MONGOC
				auto* Context = Base;
				return Core::Cotask<Cursor>([Context, &Match, &Options]()
				{
					return MongoExecuteCursor(&mongoc_collection_find_with_opts, Context, Match.Get(), Options.Get(), nullptr);
				});
#else
				return Cursor(nullptr);
#endif
			}
			Core::Promise<Cursor> Collection::FindOne(const Document& Match, const Document& Options) const
			{
#ifdef ED_HAS_MONGOC
				auto* Context = Base;
				return Core::Cotask<Cursor>([Context, &Match, &Options]()
				{
					Document Settings;
					if (Options.Get() != nullptr)
					{
						Settings = Options.Copy();
						Settings.SetInteger("limit", 1);
					}
					else
						Settings = Document(BCON_NEW("limit", BCON_INT32(1)));

					return MongoExecuteCursor(&mongoc_collection_find_with_opts, Context, Match.Get(), Settings.Get(), nullptr);
				});
#else
				return Cursor(nullptr);
#endif
			}
			Core::Promise<Cursor> Collection::Aggregate(QueryFlags Flags, const Document& Pipeline, const Document& Options) const
			{
#ifdef ED_HAS_MONGOC
				auto* Context = Base;
				return Core::Cotask<Cursor>([Context, Flags, &Pipeline, &Options]()
				{
					return MongoExecuteCursor(&mongoc_collection_aggregate, Context, (mongoc_query_flags_t)Flags, Pipeline.Get(), Options.Get(), nullptr);
				});
#else
				return Cursor(nullptr);
#endif
			}
			Core::Promise<Response> Collection::TemplateQuery(const std::string& Name, Core::SchemaArgs* Map, bool Once, Transaction* Session) const
			{
				ED_DEBUG("[mongoc] template query %s", Name.empty() ? "empty-query-name" : Name.c_str());
				return Query(Driver::GetQuery(Name, Map, Once), Session);
			}
			Core::Promise<Response> Collection::Query(const Document& Command, Transaction* Session) const
			{
#ifdef ED_HAS_MONGOC
				if (!Command.Get())
				{
					ED_ERR("[mongoc] cannot run empty query");
					return Response();
				}

				Property Type;
				if (!Command.GetProperty("type", &Type) || Type.Mod != Type::String)
				{
					ED_ERR("[mongoc] cannot run query without query @type");
					return Response();
				}

				if (Type.String == "aggregate")
				{
					QueryFlags Flags = QueryFlags::None; Property QFlags;
					if (Command.GetProperty("flags", &QFlags) && QFlags.Mod == Type::String)
					{
						for (auto& Item : Core::Parser(&QFlags.String).Split(','))
						{
							if (Item == "tailable-cursor")
								Flags = Flags | QueryFlags::Tailable_Cursor;
							else if (Item == "slave-ok")
								Flags = Flags | QueryFlags::Slave_Ok;
							else if (Item == "oplog-replay")
								Flags = Flags | QueryFlags::Oplog_Replay;
							else if (Item == "no-cursor-timeout")
								Flags = Flags | QueryFlags::No_Cursor_Timeout;
							else if (Item == "await-data")
								Flags = Flags | QueryFlags::Await_Data;
							else if (Item == "exhaust")
								Flags = Flags | QueryFlags::Exhaust;
							else if (Item == "partial")
								Flags = Flags | QueryFlags::Partial;
						}
					}

					Property Options = Command["options"];
					if (Session != nullptr && !Session->Put(&Options.Source))
					{
						ED_ERR("[mongoc] cannot run aggregation query in transaction");
						return Response();
					}

					Property Pipeline;
					if (!Command.GetProperty("pipeline", &Pipeline) || Pipeline.Mod != Type::Array)
					{
						ED_ERR("[mongoc] cannot run aggregation query without @pipeline");
						return Response();
					}

					return Aggregate(Flags, Pipeline.LoseOwnership(), Options.LoseOwnership()).Then<Response>([](Cursor&& Result)
					{
						return Response(Result);
					});
				}
				else if (Type.String == "find")
				{
					Property Options = Command["options"];
					if (Session != nullptr && !Session->Put(&Options.Source))
					{
						ED_ERR("[mongoc] cannot run find-one query in transaction");
						return Response();
					}

					Property Match;
					if (!Command.GetProperty("match", &Match) || Match.Mod != Type::Document)
					{
						ED_ERR("[mongoc] cannot run find-one query without @match");
						return Response();
					}

					return FindOne(Match.LoseOwnership(), Options.LoseOwnership()).Then<Response>([](Cursor&& Result)
					{
						return Response(Result);
					});
				}
				else if (Type.String == "find-many")
				{
					Property Options = Command["options"];
					if (Session != nullptr && !Session->Put(&Options.Source))
					{
						ED_ERR("[mongoc] cannot run find-many query in transaction");
						return Response();
					}

					Property Match;
					if (!Command.GetProperty("match", &Match) || Match.Mod != Type::Document)
					{
						ED_ERR("[mongoc] cannot run find-many query without @match");
						return Response();
					}

					return FindMany(Match.LoseOwnership(), Options.LoseOwnership()).Then<Response>([](Cursor&& Result)
					{
						return Response(Result);
					});
				}
				else if (Type.String == "update")
				{
					Property Options = Command["options"];
					if (Session != nullptr && !Session->Put(&Options.Source))
					{
						ED_ERR("[mongoc] cannot run update-one query in transaction");
						return Response();
					}

					Property Match;
					if (!Command.GetProperty("match", &Match) || Match.Mod != Type::Document)
					{
						ED_ERR("[mongoc] cannot run update-one query without @match");
						return Response();
					}

					Property Update;
					if (!Command.GetProperty("update", &Update) || Update.Mod != Type::Document)
					{
						ED_ERR("[mongoc] cannot run update-one query without @update");
						return Response();
					}

					return UpdateOne(Match.LoseOwnership(), Update.LoseOwnership(), Options.LoseOwnership()).Then<Response>([](Document&& Result)
					{
						return Response(Result);
					});
				}
				else if (Type.String == "update-many")
				{
					Property Options = Command["options"];
					if (Session != nullptr && !Session->Put(&Options.Source))
					{
						ED_ERR("[mongoc] cannot run update-many query in transaction");
						return Response();
					}

					Property Match;
					if (!Command.GetProperty("match", &Match) || Match.Mod != Type::Document)
					{
						ED_ERR("[mongoc] cannot run update-many query without @match");
						return Response();
					}

					Property Update;
					if (!Command.GetProperty("update", &Update) || Update.Mod != Type::Document)
					{
						ED_ERR("[mongoc] cannot run update-many query without @update");
						return Response();
					}

					return UpdateMany(Match.LoseOwnership(), Update.LoseOwnership(), Options.LoseOwnership()).Then<Response>([](Document&& Result)
					{
						return Response(Result);
					});
				}
				else if (Type.String == "insert")
				{
					Property Options = Command["options"];
					if (Session != nullptr && !Session->Put(&Options.Source))
					{
						ED_ERR("[mongoc] cannot run insert-one query in transaction");
						return Response();
					}

					Property Value;
					if (!Command.GetProperty("value", &Value) || Value.Mod != Type::Document)
					{
						ED_ERR("[mongoc] cannot run insert-one query without @value");
						return Response();
					}

					return InsertOne(Value.LoseOwnership(), Options.LoseOwnership()).Then<Response>([](Document&& Result)
					{
						return Response(Result);
					});
				}
				else if (Type.String == "insert-many")
				{
					Property Options = Command["options"];
					if (Session != nullptr && !Session->Put(&Options.Source))
					{
						ED_ERR("[mongoc] cannot run insert-many query in transaction");
						return Response();
					}

					Property Values;
					if (!Command.GetProperty("values", &Values) || Values.Mod != Type::Array)
					{
						ED_ERR("[mongoc] cannot run insert-many query without @values");
						return Response();
					}

					std::vector<Document> Data;
					Values.Get().Loop([&Data](Property* Value)
					{
						Data.push_back(Value->LoseOwnership());
						return true;
					});

					return InsertMany(Data, Options.LoseOwnership()).Then<Response>([](Document&& Result)
					{
						return Response(Result);
					});
				}
				else if (Type.String == "find-update")
				{
					Property Match;
					if (!Command.GetProperty("match", &Match) || Match.Mod != Type::Document)
					{
						ED_ERR("[mongoc] cannot run find-and-modify query without @match");
						return Response();
					}

					Property Sort = Command["sort"];
					Property Update = Command["update"];
					Property Fields = Command["fields"];
					Property Remove = Command["remove"];
					Property Upsert = Command["upsert"];
					Property New = Command["new"];

					return FindAndModify(Match.LoseOwnership(), Sort.LoseOwnership(), Update.LoseOwnership(), Fields.LoseOwnership(), Remove.Boolean, Upsert.Boolean, New.Boolean).Then<Response>([](Document&& Result)
					{
						return Response(Result);
					});
				}
				else if (Type.String == "replace")
				{
					Property Options = Command["options"];
					if (Session != nullptr && !Session->Put(&Options.Source))
					{
						ED_ERR("[mongoc] cannot run replace-one query in transaction");
						return Response();
					}

					Property Match;
					if (!Command.GetProperty("match", &Match) || Match.Mod != Type::Document)
					{
						ED_ERR("[mongoc] cannot run replace-one query without @match");
						return Response();
					}

					Property Value;
					if (!Command.GetProperty("value", &Value) || Value.Mod != Type::Document)
					{
						ED_ERR("[mongoc] cannot run replace-one query without @value");
						return Response();
					}

					return ReplaceOne(Match.LoseOwnership(), Value.LoseOwnership(), Options.LoseOwnership()).Then<Response>([](Document&& Result)
					{
						return Response(Result);
					});
				}
				else if (Type.String == "remove")
				{
					Property Options = Command["options"];
					if (Session != nullptr && !Session->Put(&Options.Source))
					{
						ED_ERR("[mongoc] cannot run remove-one query in transaction");
						return Response();
					}

					Property Match;
					if (!Command.GetProperty("match", &Match) || Match.Mod != Type::Document)
					{
						ED_ERR("[mongoc] cannot run remove-one query without @value");
						return Response();
					}

					return RemoveOne(Match.LoseOwnership(), Options.LoseOwnership()).Then<Response>([](Document&& Result)
					{
						return Response(Result);
					});
				}
				else if (Type.String == "remove-many")
				{
					Property Options = Command["options"];
					if (Session != nullptr && !Session->Put(&Options.Source))
					{
						ED_ERR("[mongoc] cannot run remove-many query in transaction");
						return Response();
					}

					Property Match;
					if (!Command.GetProperty("match", &Match) || Match.Mod != Type::Document)
					{
						ED_ERR("[mongoc] cannot run remove-many query without @value");
						return Response();
					}

					return RemoveMany(Match.LoseOwnership(), Options.LoseOwnership()).Then<Response>([](Document&& Result)
					{
						return Response(Result);
					});
				}

				ED_ERR("[mongoc] cannot find query of type \"%s\"", Type.String.c_str());
				return Response();
#else
				return Response();
#endif
			}
			const char* Collection::GetName() const
			{
#ifdef ED_HAS_MONGOC
				ED_ASSERT(Base != nullptr, nullptr, "context should be set");
				return mongoc_collection_get_name(Base);
#else
				return nullptr;
#endif
			}
			Stream Collection::CreateStream(Document&& Options) const
			{
#ifdef ED_HAS_MONGOC
				if (!Base)
					return Stream(nullptr, nullptr, nullptr);

				TStream* Operation = mongoc_collection_create_bulk_operation_with_opts(Base, Options.Get());
				return Stream(Base, Operation, std::move(Options));
#else
				return Stream(nullptr, nullptr, nullptr);
#endif
			}
			TCollection* Collection::Get() const
			{
#ifdef ED_HAS_MONGOC
				return Base;
#else
				return nullptr;
#endif
			}

			Database::Database(TDatabase* NewBase) : Base(NewBase)
			{
			}
			Database::Database(Database&& Other) : Base(Other.Base)
			{
				Other.Base = nullptr;
			}
			Database::~Database()
			{
#ifdef ED_HAS_MONGOC
				if (Base != nullptr)
					mongoc_database_destroy(Base);
				Base = nullptr;
#endif
			}
			Database& Database::operator =(Database&& Other)
			{
				if (&Other == this)
					return *this;

#ifdef ED_HAS_MONGOC
				if (Base != nullptr)
					mongoc_database_destroy(Base);
#endif
				Base = Other.Base;
				Other.Base = nullptr;
				return *this;
			}
			Core::Promise<bool> Database::RemoveAllUsers()
			{
#ifdef ED_HAS_MONGOC
				auto* Context = Base;
				return Core::Cotask<bool>([Context]()
				{
					return MongoExecuteQuery(&mongoc_database_remove_all_users, Context);
				});
#else
				return false;
#endif
			}
			Core::Promise<bool> Database::RemoveUser(const std::string& Name)
			{
#ifdef ED_HAS_MONGOC
				auto* Context = Base;
				return Core::Cotask<bool>([Context, Name]()
				{
					return MongoExecuteQuery(&mongoc_database_remove_user, Context, Name.c_str());
				});
#else
				return false;
#endif
			}
			Core::Promise<bool> Database::Remove()
			{
#ifdef ED_HAS_MONGOC
				auto* Context = Base;
				return Core::Cotask<bool>([Context]()
				{
					return MongoExecuteQuery(&mongoc_database_drop, Context);
				});
#else
				return false;
#endif
			}
			Core::Promise<bool> Database::RemoveWithOptions(const Document& Options)
			{
#ifdef ED_HAS_MONGOC
				if (!Options.Get())
					return Remove();

				auto* Context = Base;
				return Core::Cotask<bool>([Context, &Options]()
				{
					return MongoExecuteQuery(&mongoc_database_drop_with_opts, Context, Options.Get());
				});
#else
				return false;
#endif
			}
			Core::Promise<bool> Database::AddUser(const std::string& Username, const std::string& Password, const Document& Roles, const Document& Custom)
			{
#ifdef ED_HAS_MONGOC
				auto* Context = Base;
				return Core::Cotask<bool>([Context, Username, Password, &Roles, &Custom]()
				{
					return MongoExecuteQuery(&mongoc_database_add_user, Context, Username.c_str(), Password.c_str(), Roles.Get(), Custom.Get());
				});
#else
				return false;
#endif
			}
			Core::Promise<bool> Database::HasCollection(const std::string& Name) const
			{
#ifdef ED_HAS_MONGOC
				auto* Context = Base;
				return Core::Cotask<bool>([Context, Name]()
				{
					bson_error_t Error;
					memset(&Error, 0, sizeof(bson_error_t));
					bool Subresult = mongoc_database_has_collection(Context, Name.c_str(), &Error);
					if (!Subresult && Error.code != 0)
						ED_ERR("[mongoc:%i] %s", (int)Error.code, Error.message);

					return Subresult;
				});
#else
				return false;
#endif
			}
			Core::Promise<Cursor> Database::FindCollections(const Document& Options) const
			{
#ifdef ED_HAS_MONGOC
				auto* Context = Base;
				return Core::Cotask<Cursor>([Context, &Options]()
				{
					return MongoExecuteCursor(&mongoc_database_find_collections_with_opts, Context, Options.Get());
				});
#else
				return Cursor(nullptr);
#endif
			}
			Core::Promise<Collection> Database::CreateCollection(const std::string& Name, const Document& Options)
			{
#ifdef ED_HAS_MONGOC
				if (!Base)
					return Collection(nullptr);

				auto* Context = Base;
				return Core::Cotask<Collection>([Context, Name, &Options]()
				{
					bson_error_t Error;
					memset(&Error, 0, sizeof(bson_error_t));

					TCollection* Collection = mongoc_database_create_collection(Context, Name.c_str(), Options.Get(), &Error);
					if (Collection == nullptr)
						ED_ERR("[mongoc] %s", Error.message);
					
					return Collection;
				});
#else
				return Collection(nullptr);
#endif
			}
			std::vector<std::string> Database::GetCollectionNames(const Document& Options) const
			{
#ifdef ED_HAS_MONGOC
				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!Base)
					return std::vector<std::string>();

				char** Names = mongoc_database_get_collection_names_with_opts(Base, Options.Get(), &Error);
				if (Names == nullptr)
				{
					ED_ERR("[mongoc] %s", Error.message);
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
#ifdef ED_HAS_MONGOC
				ED_ASSERT(Base != nullptr, nullptr, "context should be set");
				return mongoc_database_get_name(Base);
#else
				return nullptr;
#endif
			}
			Collection Database::GetCollection(const std::string& Name)
			{
#ifdef ED_HAS_MONGOC
				ED_ASSERT(Base != nullptr, nullptr, "context should be set");
				return mongoc_database_get_collection(Base, Name.c_str());
#else
				return nullptr;
#endif
			}
			TDatabase* Database::Get() const
			{
#ifdef ED_HAS_MONGOC
				return Base;
#else
				return nullptr;
#endif
			}

			Watcher::Watcher(TWatcher* NewBase) : Base(NewBase)
			{
			}
			Watcher::Watcher(Watcher&& Other) : Base(Other.Base)
			{
				Other.Base = nullptr;
			}
			Watcher::~Watcher()
			{
#ifdef ED_HAS_MONGOC
				if (Base != nullptr)
					mongoc_change_stream_destroy(Base);
				Base = nullptr;
#endif
			}
			Watcher& Watcher::operator =(Watcher&& Other)
			{
				if (&Other == this)
					return *this;

#ifdef ED_HAS_MONGOC
				if (Base != nullptr)
					mongoc_change_stream_destroy(Base);
#endif
				Base = Other.Base;
				Other.Base = nullptr;
				return *this;
			}
			Core::Promise<bool> Watcher::Next(Document& Result) const
			{
#ifdef ED_HAS_MONGOC
				if (!Base || !Result.Get())
					return false;

				auto* Context = Base;
				return Core::Cotask<bool>([Context, &Result]()
				{
					TDocument* Ptr = Result.Get();
					return mongoc_change_stream_next(Context, (const TDocument**)&Ptr);
				});
#else
				return false;
#endif
			}
			Core::Promise<bool> Watcher::Error(Document& Result) const
			{
#ifdef ED_HAS_MONGOC
				if (!Base || !Result.Get())
					return false;

				auto* Context = Base;
				return Core::Cotask<bool>([Context, &Result]()
				{
					TDocument* Ptr = Result.Get();
					return mongoc_change_stream_error_document(Context, nullptr, (const TDocument**)&Ptr);
				});
#else
				return false;
#endif
			}
			TWatcher* Watcher::Get() const
			{
#ifdef ED_HAS_MONGOC
				return Base;
#else
				return nullptr;
#endif
			}
			Watcher Watcher::FromConnection(Connection* Connection, const Document& Pipeline, const Document& Options)
			{
#ifdef ED_HAS_MONGOC
				if (!Connection)
					return nullptr;

				return mongoc_client_watch(Connection->Get(), Pipeline.Get(), Options.Get());
#else
				return nullptr;
#endif
			}
			Watcher Watcher::FromDatabase(const Database& Database, const Document& Pipeline, const Document& Options)
			{
#ifdef ED_HAS_MONGOC
				if (!Database.Get())
					return nullptr;

				return mongoc_database_watch(Database.Get(), Pipeline.Get(), Options.Get());
#else
				return nullptr;
#endif
			}
			Watcher Watcher::FromCollection(const Collection& Collection, const Document& Pipeline, const Document& Options)
			{
#ifdef ED_HAS_MONGOC
				if (!Collection.Get())
					return nullptr;

				return mongoc_collection_watch(Collection.Get(), Pipeline.Get(), Options.Get());
#else
				return nullptr;
#endif
			}

			Transaction::Transaction(TTransaction* NewBase) : Base(NewBase)
			{
			}
			bool Transaction::Push(Document& QueryOptions) const
			{
#ifdef ED_HAS_MONGOC
				if (!QueryOptions.Get())
					QueryOptions = bson_new();

				bson_error_t Error;
				bool Result = mongoc_client_session_append(Base, (bson_t*)QueryOptions.Get(), &Error);
				if (!Result && Error.code != 0)
					ED_ERR("[mongoc:%i] %s", (int)Error.code, Error.message);

				return Result;
#else
				return false;
#endif
			}
			bool Transaction::Put(TDocument** QueryOptions) const
			{
#ifdef ED_HAS_MONGOC
				ED_ASSERT(QueryOptions != nullptr, false, "query options should be set");
				if (!*QueryOptions)
					*QueryOptions = bson_new();

				bson_error_t Error;
				bool Result = mongoc_client_session_append(Base, (bson_t*)*QueryOptions, &Error);
				if (!Result && Error.code != 0)
					ED_ERR("[mongoc:%i] %s", (int)Error.code, Error.message);

				return Result;
#else
				return false;
#endif
			}
			Core::Promise<bool> Transaction::Start()
			{
#ifdef ED_HAS_MONGOC
				auto* Context = Base;
				return Core::Cotask<bool>([Context]()
				{
					return MongoExecuteQuery(&mongoc_client_session_start_transaction, Context, nullptr);
				});
#else
				return false;
#endif
			}
			Core::Promise<bool> Transaction::Abort()
			{
#ifdef ED_HAS_MONGOC
				auto* Context = Base;
				return Core::Cotask<bool>([Context]()
				{
					return MongoExecuteQuery(&mongoc_client_session_abort_transaction, Context);
				});
#else
				return false;
#endif
			}
			Core::Promise<Document> Transaction::RemoveMany(const Collection& Source, const Document& Match, Document&& Options)
			{
#ifdef ED_HAS_MONGOC
				if (!Push(Options))
					return Document(nullptr);

				return Source.RemoveMany(Match, Options);
#else
				return Document(nullptr);
#endif
			}
			Core::Promise<Document> Transaction::RemoveOne(const Collection& Source, const Document& Match, Document&& Options)
			{
#ifdef ED_HAS_MONGOC
				if (!Push(Options))
					return Document(nullptr);

				return Source.RemoveOne(Match, Options);
#else
				return Document(nullptr);
#endif
			}
			Core::Promise<Document> Transaction::ReplaceOne(const Collection& Source, const Document& Match, const Document& Replacement, Document&& Options)
			{
#ifdef ED_HAS_MONGOC
				if (!Push(Options))
					return Document(nullptr);

				return Source.ReplaceOne(Match, Replacement, Options);
#else
				return Document(nullptr);
#endif
			}
			Core::Promise<Document> Transaction::InsertMany(const Collection& Source, std::vector<Document>& List, Document&& Options)
			{
#ifdef ED_HAS_MONGOC
				if (!Push(Options))
					return Document(nullptr);

				return Source.InsertMany(List, Options);
#else
				return Document(nullptr);
#endif
			}
			Core::Promise<Document> Transaction::InsertOne(const Collection& Source, const Document& Result, Document&& Options)
			{
#ifdef ED_HAS_MONGOC
				if (!Push(Options))
					return Document(nullptr);

				return Source.InsertOne(Result, Options);
#else
				return Document(nullptr);
#endif
			}
			Core::Promise<Document> Transaction::UpdateMany(const Collection& Source, const Document& Match, const Document& Update, Document&& Options)
			{
#ifdef ED_HAS_MONGOC
				if (!Push(Options))
					return Document(nullptr);

				return Source.UpdateMany(Match, Update, Options);
#else
				return Document(nullptr);
#endif
			}
			Core::Promise<Document> Transaction::UpdateOne(const Collection& Source, const Document& Match, const Document& Update, Document&& Options)
			{
#ifdef ED_HAS_MONGOC
				if (!Push(Options))
					return Document(nullptr);

				return Source.UpdateOne(Match, Update, Options);
#else
				return Document(nullptr);
#endif
			}
			Core::Promise<Cursor> Transaction::FindMany(const Collection& Source, const Document& Match, Document&& Options) const
			{
#ifdef ED_HAS_MONGOC
				if (!Push(Options))
					return Cursor(nullptr);

				return Source.FindMany(Match, Options);
#else
				return Cursor(nullptr);
#endif
			}
			Core::Promise<Cursor> Transaction::FindOne(const Collection& Source, const Document& Match, Document&& Options) const
			{
#ifdef ED_HAS_MONGOC
				if (!Push(Options))
					return Cursor(nullptr);

				return Source.FindOne(Match, Options);
#else
				return Cursor(nullptr);
#endif
			}
			Core::Promise<Cursor> Transaction::Aggregate(const Collection& Source, QueryFlags Flags, const Document& Pipeline, Document&& Options) const
			{
#ifdef ED_HAS_MONGOC
				if (!Push(Options))
					return Cursor(nullptr);

				return Source.Aggregate(Flags, Pipeline, Options);
#else
				return Cursor(nullptr);
#endif
			}
			Core::Promise<Response> Transaction::TemplateQuery(const Collection& Source, const std::string& Name, Core::SchemaArgs* Map, bool Once)
			{
				return Query(Source, Driver::GetQuery(Name, Map, Once));
			}
			Core::Promise<Response> Transaction::Query(const Collection& Source, const Document& Command)
			{
#ifdef ED_HAS_MONGOC
				return Source.Query(Command, this);
#else
				return Response();
#endif
			}
			Core::Promise<TransactionState> Transaction::Commit()
			{
#ifdef ED_HAS_MONGOC
				auto* Context = Base;
				return Core::Cotask<TransactionState>([Context]()
				{
					TDocument Subresult; TransactionState Result;
					if (MongoExecuteQuery(&mongoc_client_session_commit_transaction, Context, &Subresult))
						Result = TransactionState::OK;
					else if (mongoc_error_has_label(&Subresult, "TransientTransactionError"))
						Result = TransactionState::Retry;
					else if (mongoc_error_has_label(&Subresult, "UnknownTransactionCommitResult"))
						Result = TransactionState::Retry_Commit;
					else
						Result = TransactionState::Fatal;

					bson_free(&Subresult);
					return Result;
				});
#else
				return TransactionState::Fatal;
#endif
			}
			TTransaction* Transaction::Get() const
			{
#ifdef ED_HAS_MONGOC
				return Base;
#else
				return nullptr;
#endif
			}

			Connection::Connection() : Connected(false), Session(nullptr), Base(nullptr), Master(nullptr)
			{
				Driver::Create();
			}
			Connection::~Connection()
			{
#ifdef ED_HAS_MONGOC
				TTransaction* Context = Session.Get();
				if (Context != nullptr)
					mongoc_client_session_destroy(Context);
#endif
				if (Connected && Base != nullptr)
					Disconnect();
				Driver::Release();
			}
			Core::Promise<bool> Connection::Connect(const std::string& Address)
			{
#ifdef ED_HAS_MONGOC
				ED_ASSERT(Master != nullptr, false, "connection should be created outside of cluster");
				if (Connected)
				{
					return Disconnect().Then<Core::Promise<bool>>([this, Address](bool)
					{
						return this->Connect(Address);
					});
				}

				return Core::Cotask<bool>([this, Address]()
				{
					ED_MEASURE(ED_TIMING_MAX);
					bson_error_t Error;
					memset(&Error, 0, sizeof(bson_error_t));

					TAddress* URI = mongoc_uri_new_with_error(Address.c_str(), &Error);
					if (!URI)
					{
						ED_ERR("[urierr] %s", Error.message);
						return false;
					}

					Base = mongoc_client_new_from_uri(URI);
					if (!Base)
					{
						ED_ERR("[mongoc] couldn't connect to requested URI");
						return false;
					}

					Driver::AttachQueryLog(Base);
					Connected = true;
					return true;
				});
#else
				return false;
#endif
			}
			Core::Promise<bool> Connection::Connect(Address* URL)
			{
#ifdef ED_HAS_MONGOC
				ED_ASSERT(Master != nullptr, false, "connection should be created outside of cluster");
				ED_ASSERT(URL && URL->Get(), false, "url should be valid");

				if (Connected)
				{
					return Disconnect().Then<Core::Promise<bool>>([this, URL](bool)
					{
						return this->Connect(URL);
					});
				}

				TAddress* URI = URL->Get();
				return Core::Cotask<bool>([this, URI]()
				{
					ED_MEASURE(ED_TIMING_MAX);
					Base = mongoc_client_new_from_uri(URI);
					if (!Base)
					{
						ED_ERR("[mongoc] couldn't connect to requested URI");
						return false;
					}

					Driver::AttachQueryLog(Base);
					Connected = true;
					return true;
				});
#else
				return false;
#endif
			}
			Core::Promise<bool> Connection::Disconnect()
			{
#ifdef ED_HAS_MONGOC
				ED_ASSERT(Connected && Base, false, "connection should be established");
				return Core::Cotask<bool>([this]()
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

					return true;
				});
#else
				return false;
#endif
			}
			Core::Promise<bool> Connection::MakeTransaction(const std::function<Core::Promise<bool>(Transaction&)>& Callback)
			{
#ifdef ED_HAS_MONGOC
				ED_ASSERT(Callback, false, "callback should not be empty");
				return Core::Coasync<bool>([this, Callback]()
				{
					Transaction Context = GetSession();
					if (!Context)
						return false;

					while (true)
					{
						if (!ED_AWAIT(Context.Start()))
							return false;

						if (!ED_AWAIT(Callback(Context)))
							break;

						while (true)
						{
							TransactionState State = ED_AWAIT(Context.Commit());
							if (State == TransactionState::OK || State == TransactionState::Fatal)
								return State == TransactionState::OK;

							if (State == TransactionState::Retry_Commit)
							{
								ED_WARN("[mongoc] retrying transaction commit");
								continue;
							}

							if (State == TransactionState::Retry)
							{
								ED_WARN("[mongoc] retrying full transaction");
								break;
							}
						}
					}

					ED_AWAIT(Context.Abort());
					return false;
				});
#else
				return false;
#endif
			}
			Core::Promise<bool> Connection::MakeCotransaction(const std::function<bool(Transaction&)>& Callback)
			{
#ifdef ED_HAS_MONGOC
				ED_ASSERT(Callback, false, "callback should not be empty");
				return Core::Coasync<bool>([this, Callback]()
				{
					Transaction Context = GetSession();
					if (!Context)
						return false;

					while (true)
					{
						if (!ED_AWAIT(Context.Start()))
							return false;

						if (!Callback(Context))
							break;

						while (true)
						{
							TransactionState State = ED_AWAIT(Context.Commit());
							if (State == TransactionState::OK || State == TransactionState::Fatal)
								return State == TransactionState::OK;

							if (State == TransactionState::Retry_Commit)
							{
								ED_WARN("[mongoc] retrying transaction commit");
								continue;
							}

							if (State == TransactionState::Retry)
							{
								ED_WARN("[mongoc] retrying full transaction");
								break;
							}
						}
					}

					ED_AWAIT(Context.Abort());
					return false;
				});
#else
				return false;
#endif
			}
			Core::Promise<Cursor> Connection::FindDatabases(const Document& Options) const
			{
#ifdef ED_HAS_MONGOC
				return Core::Cotask<Cursor>([this, &Options]()
				{
					return mongoc_client_find_databases_with_opts(Base, Options.Get());
				});
#else
				return Cursor(nullptr);
#endif
			}
			void Connection::SetProfile(const std::string& Name)
			{
#ifdef ED_HAS_MONGOC
				mongoc_client_set_appname(Base, Name.c_str());
#endif
			}
			bool Connection::SetServer(bool ForWrites)
			{
#ifdef ED_HAS_MONGOC
				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				mongoc_server_description_t* Server = mongoc_client_select_server(Base, ForWrites, nullptr, &Error);
				if (Server == nullptr)
				{
					ED_ERR("[mongoc] command fail: %s", Error.message);
					return false;
				}

				mongoc_server_description_destroy(Server);
				return true;
#else
				return false;
#endif
			}
			Transaction& Connection::GetSession()
			{
#ifdef ED_HAS_MONGOC
				if (Session.Get() != nullptr)
					return Session;

				bson_error_t Error;
				Session = mongoc_client_start_session(Base, nullptr, &Error);
				if (!Session.Get())
					ED_ERR("[mongoc] couldn't create transaction\n\t%s", Error.message);

				return Session;
#else
				return Session;
#endif
			}
			Database Connection::GetDatabase(const std::string& Name) const
			{
#ifdef ED_HAS_MONGOC
				ED_ASSERT(Base != nullptr, nullptr, "context should be set");
				return mongoc_client_get_database(Base, Name.c_str());
#else
				return nullptr;
#endif
			}
			Database Connection::GetDefaultDatabase() const
			{
#ifdef ED_HAS_MONGOC
				ED_ASSERT(Base != nullptr, nullptr, "context should be set");
				return mongoc_client_get_default_database(Base);
#else
				return nullptr;
#endif
			}
			Collection Connection::GetCollection(const char* DatabaseName, const char* Name) const
			{
#ifdef ED_HAS_MONGOC
				ED_ASSERT(Base != nullptr, nullptr, "context should be set");
				return mongoc_client_get_collection(Base, DatabaseName, Name);
#else
				return nullptr;
#endif
			}
			Address Connection::GetAddress() const
			{
#ifdef ED_HAS_MONGOC
				ED_ASSERT(Base != nullptr, nullptr, "context should be set");
				return (TAddress*)mongoc_client_get_uri(Base);
#else
				return nullptr;
#endif
			}
			Cluster* Connection::GetMaster() const
			{
				return Master;
			}
			TConnection* Connection::Get() const
			{
				return Base;
			}
			std::vector<std::string> Connection::GetDatabaseNames(const Document& Options) const
			{
#ifdef ED_HAS_MONGOC
				ED_ASSERT(Base != nullptr, std::vector<std::string>(), "context should be set");

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));
				char** Names = mongoc_client_get_database_names_with_opts(Base, Options.Get(), &Error);
				if (Names == nullptr)
				{
					ED_ERR("[mongoc] %s", Error.message);
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

			Cluster::Cluster() : Connected(false), Pool(nullptr), SrcAddress(nullptr)
			{
				Driver::Create();
			}
			Cluster::~Cluster()
			{
				Disconnect();
				Driver::Release();
			}
			Core::Promise<bool> Cluster::Connect(const std::string& URI)
			{
#ifdef ED_HAS_MONGOC
				if (Connected)
				{
					return Disconnect().Then<Core::Promise<bool>>([this, URI](bool)
					{
						return this->Connect(URI);
					});
				}

				return Core::Cotask<bool>([this, URI]()
				{
					ED_MEASURE(ED_TIMING_MAX);
					bson_error_t Error;
					memset(&Error, 0, sizeof(bson_error_t));

					SrcAddress = mongoc_uri_new_with_error(URI.c_str(), &Error);
					if (!SrcAddress.Get())
					{
						ED_ERR("[urierr] %s", Error.message);
						return false;
					}

					Pool = mongoc_client_pool_new(SrcAddress.Get());
					if (!Pool)
					{
						ED_ERR("[mongoc] couldn't connect to requested URI");
						return false;
					}

					Driver::AttachQueryLog(Pool);
					Connected = true;
					return true;
				});
#else
				return false;
#endif
			}
			Core::Promise<bool> Cluster::Connect(Address* URI)
			{
#ifdef ED_HAS_MONGOC
				ED_ASSERT(URI && URI->Get(), false, "url should be set");
				if (Connected)
				{
					return Disconnect().Then<Core::Promise<bool>>([this, URI](bool)
					{
						return this->Connect(URI);
					});
				}

				TAddress* Context = URI->Get();
				*URI = nullptr;

				return Core::Cotask<bool>([this, Context]()
				{
					ED_MEASURE(ED_TIMING_MAX);
					SrcAddress = Context;
					Pool = mongoc_client_pool_new(SrcAddress.Get());
					if (!Pool)
					{
						ED_ERR("[mongoc] couldn't connect to requested URI");
						return false;
					}

					Driver::AttachQueryLog(Pool);
					Connected = true;
					return true;
				});
#else
				return false;
#endif
			}
			Core::Promise<bool> Cluster::Disconnect()
			{
#ifdef ED_HAS_MONGOC
				ED_ASSERT(Connected && Pool, false, "connection should be established");
				return Core::Cotask<bool>([this]()
				{
					if (Pool != nullptr)
					{
						mongoc_client_pool_destroy(Pool);
						Pool = nullptr;
					}

					SrcAddress.~Address();
					Connected = false;
					return true;
				});
#else
				return false;
#endif
			}
			void Cluster::SetProfile(const char* Name)
			{
#ifdef ED_HAS_MONGOC
				ED_ASSERT_V(Pool != nullptr, "connection should be established");
				ED_ASSERT_V(Name != nullptr, "name should be set");
				mongoc_client_pool_set_appname(Pool, Name);
#endif
			}
			void Cluster::Push(Connection** Client)
			{
#ifdef ED_HAS_MONGOC
				ED_ASSERT_V(Client && *Client, "client should be set");
				mongoc_client_pool_push(Pool, (*Client)->Base);
				(*Client)->Base = nullptr;
				(*Client)->Connected = false;
				(*Client)->Master = nullptr;

				ED_RELEASE(*Client);
				*Client = nullptr;
#endif
			}
			Connection* Cluster::Pop()
			{
#ifdef ED_HAS_MONGOC
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
			TConnectionPool* Cluster::Get() const
			{
				return Pool;
			}
			Address& Cluster::GetAddress()
			{
				return SrcAddress;
			}

			Driver::Sequence::Sequence() : Cache(nullptr)
			{
			}
			Driver::Sequence::Sequence(const Sequence& Other) : Positions(Other.Positions), Request(Other.Request), Cache(Other.Cache.Copy())
			{
			}

			void Driver::Create()
			{
#ifdef ED_HAS_MONGOC
				if (State <= 0)
				{
					using Map1 = Core::Mapping<std::unordered_map<std::string, Sequence>>;
					using Map2 = Core::Mapping<std::unordered_map<std::string, std::string>>;
					Network::Driver::SetActive(true);

					Queries = ED_NEW(Map1);
					Constants = ED_NEW(Map2);
					Safe = ED_NEW(std::mutex);
					mongoc_log_set_handler([](mongoc_log_level_t Level, const char* Domain, const char* Message, void*)
					{
						switch (Level)
						{
							case MONGOC_LOG_LEVEL_ERROR:
								ED_ERR("[mongoc] [%s] %s", Domain, Message);
								break;
							case MONGOC_LOG_LEVEL_WARNING:
								ED_WARN("[mongoc] [%s] %s", Domain, Message);
								break;
							case MONGOC_LOG_LEVEL_INFO:
								ED_INFO("[mongoc] [%s] %s", Domain, Message);
								break;
							case MONGOC_LOG_LEVEL_CRITICAL:
								ED_ERR("[mongocerr] [%s] %s", Domain, Message);
								break;
							case MONGOC_LOG_LEVEL_MESSAGE:
								ED_DEBUG("[mongoc] [%s] %s", Domain, Message);
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
#ifdef ED_HAS_MONGOC
				if (State == 1)
				{
					Network::Driver::SetActive(false);
					if (Safe != nullptr)
						Safe->lock();

					if (APM != nullptr)
					{
						mongoc_apm_callbacks_destroy((mongoc_apm_callbacks_t*)APM);
						APM = nullptr;
					}

					State = 0;
					if (Queries != nullptr)
					{
						ED_DELETE(Mapping, Queries);
						Queries = nullptr;
					}

					if (Constants != nullptr)
					{
						ED_DELETE(Mapping, Constants);
						Constants = nullptr;
					}

					if (Safe != nullptr)
					{
						Safe->unlock();
						ED_DELETE(mutex, Safe);
						Safe = nullptr;
					}

					mongoc_cleanup();
				}
				else if (State > 0)
					State--;
#endif
			}
			void Driver::SetQueryLog(const OnQueryLog& Callback)
			{
				Logger = Callback;
				if (!Logger || APM)
					return;
#ifdef ED_HAS_MONGOC
				mongoc_apm_callbacks_t* Callbacks = mongoc_apm_callbacks_new();
				mongoc_apm_set_command_started_cb(Callbacks, [](const mongoc_apm_command_started_t* Event)
				{
					const char* Name = mongoc_apm_command_started_get_command_name(Event);
					char* Command = bson_as_relaxed_extended_json(mongoc_apm_command_started_get_command(Event), nullptr);
					std::string Buffer = Core::Form("%s:\n%s\n", Name, Command).R();
					bson_free(Command);

					if (Logger)
						Logger(Buffer);
				});
				APM = (void*)Callbacks;
#endif
			}
			void Driver::AttachQueryLog(TConnection* Connection)
			{
#ifdef ED_HAS_MONGOC
				ED_ASSERT_V(Connection != nullptr, "connection should be set");
				mongoc_client_set_apm_callbacks(Connection, (mongoc_apm_callbacks_t*)APM, nullptr);
#endif
			}
			void Driver::AttachQueryLog(TConnectionPool* Connection)
			{
#ifdef ED_HAS_MONGOC
				ED_ASSERT_V(Connection != nullptr, "connection pool should be set");
				mongoc_client_pool_set_apm_callbacks(Connection, (mongoc_apm_callbacks_t*)APM, nullptr);
#endif
			}
			bool Driver::AddConstant(const std::string& Name, const std::string& Value)
			{
				ED_ASSERT(Constants && Safe, false, "driver should be initialized");
				ED_ASSERT(!Name.empty(), false, "name should not be empty");

				Safe->lock();
				Constants->Map[Name] = Value;
				Safe->unlock();
				return true;
			}
			bool Driver::AddQuery(const std::string& Name, const char* Buffer, size_t Size)
			{
				ED_ASSERT(Queries && Safe, false, "driver should be initialized");
				ED_ASSERT(!Name.empty(), false, "name should not be empty");
				ED_ASSERT(Buffer, false, "buffer should be set");

				if (!Size)
					return false;

				Sequence Result;
				Result.Request.assign(Buffer, Size);

				std::string Enums = " \r\n\t\'\"()<>=%&^*/+-,!?:;";
				std::string Erasable = " \r\n\t\'\"()<>=%&^*/+-,.!?:;";
				std::string Quotes = "\"'`";

				Core::Parser Base(&Result.Request);
				Base.ReplaceInBetween("/*", "*/", "", false);
				Base.Trim().Compress(Erasable.c_str(), Quotes.c_str());

				auto Enumerations = Base.FindStartsWithEndsOf("#", Enums.c_str(), Quotes.c_str());
				if (!Enumerations.empty())
				{
					int64_t Offset = 0;
					std::unique_lock<std::mutex> Unique(*Safe);
					for (auto& Item : Enumerations)
					{
						size_t Size = Item.second.End - Item.second.Start, NewSize = 0;
						Item.second.Start = (size_t)((int64_t)Item.second.Start + Offset);
						Item.second.End = (size_t)((int64_t)Item.second.End + Offset);

						auto It = Constants->Map.find(Item.first);
						if (It == Constants->Map.end())
						{
							ED_ERR("[mongoc] template query %s\n\texpects constant: %s", Name.c_str(), Item.first.c_str());
							Base.ReplacePart(Item.second.Start, Item.second.End, "");
						}
						else
						{
							Base.ReplacePart(Item.second.Start, Item.second.End, It->second);
							NewSize = It->second.size();
						}

						Offset += (int64_t)NewSize - (int64_t)Size;
						Item.second.End = Item.second.Start + NewSize;
					}
				}

				std::vector<std::pair<std::string, Core::Parser::Settle>> Variables;
				for (auto& Item : Base.FindInBetween("$<", ">", Quotes.c_str()))
				{
					Item.first += "__1";
					Variables.emplace_back(std::move(Item));
				}

				for (auto& Item : Base.FindInBetween("@<", ">", Quotes.c_str()))
				{
					Item.first += "__2";
					Variables.emplace_back(std::move(Item));
				}

				Base.ReplaceParts(Variables, "", [&Erasable](char Left)
				{
					return Erasable.find(Left) == std::string::npos ? ' ' : '\0';
				});

				for (auto& Item : Variables)
				{
					Pose Position;
					Position.Escape = Item.first.find("__1") != std::string::npos;
					Position.Offset = Item.second.Start;
					Position.Key = Item.first.substr(0, Item.first.size() - 3);
					Result.Positions.emplace_back(std::move(Position));
				}

				if (Variables.empty())
					Result.Cache = Document::FromJSON(Result.Request);

				Safe->lock();
				Queries->Map[Name] = std::move(Result);
				Safe->unlock();

				return true;
			}
			bool Driver::AddDirectory(const std::string& Directory, const std::string& Origin)
			{
				std::vector<Core::FileEntry> Entries;
				if (!Core::OS::Directory::Scan(Directory, &Entries))
					return false;

				std::string Path = Directory;
				if (Path.back() != '/' && Path.back() != '\\')
					Path.append(1, '/');

				size_t Size = 0;
				for (auto& File : Entries)
				{
					Core::Parser Base(Path + File.Path);
					if (File.IsDirectory)
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
					ED_FREE(Buffer);
				}

				return true;
			}
			bool Driver::RemoveConstant(const std::string& Name)
			{
				ED_ASSERT(Constants && Safe, false, "driver should be initialized");
				Safe->lock();
				auto It = Constants->Map.find(Name);
				if (It == Constants->Map.end())
				{
					Safe->unlock();
					return false;
				}

				Constants->Map.erase(It);
				Safe->unlock();
				return true;
			}
			bool Driver::RemoveQuery(const std::string& Name)
			{
				ED_ASSERT(Queries && Safe, false, "driver should be initialized");
				Safe->lock();
				auto It = Queries->Map.find(Name);
				if (It == Queries->Map.end())
				{
					Safe->unlock();
					return false;
				}

				Queries->Map.erase(It);
				Safe->unlock();
				return true;
			}
			bool Driver::LoadCacheDump(Core::Schema* Dump)
			{
				ED_ASSERT(Queries && Safe, false, "driver should be initialized");
				ED_ASSERT(Dump != nullptr, false, "dump should be set");

				size_t Count = 0;
				std::unique_lock<std::mutex> Unique(*Safe);
				Queries->Map.clear();

				for (auto* Data : Dump->GetChilds())
				{
					Sequence Result;
					Result.Request = Data->GetVar("request").GetBlob();

					auto* Cache = Data->Get("cache");
					if (Cache != nullptr && Cache->Value.IsObject())
					{
						Result.Cache = Document::FromDocument(Cache);
						Result.Request = Result.Cache.ToJSON();
					}

					Core::Schema* Positions = Data->Get("positions");
					if (Positions != nullptr)
					{
						for (auto* Position : Positions->GetChilds())
						{
							Pose Next;
							Next.Key = Position->GetVar(0).GetBlob();
							Next.Offset = (size_t)Position->GetVar(1).GetInteger();
							Next.Escape = Position->GetVar(2).GetBoolean();
							Result.Positions.emplace_back(std::move(Next));
						}
					}

					std::string Name = Data->GetVar("name").GetBlob();
					Queries->Map[Name] = std::move(Result);
					++Count;
				}

				if (Count > 0)
					ED_DEBUG("[pq] OK load %llu parsed query templates", (uint64_t)Count);

				return Count > 0;
			}
			Core::Schema* Driver::GetCacheDump()
			{
				ED_ASSERT(Queries && Safe, false, "driver should be initialized");
				std::unique_lock<std::mutex> Unique(*Safe);
				Core::Schema* Result = Core::Var::Set::Array();
				for (auto& Query : Queries->Map)
				{
					Core::Schema* Data = Result->Push(Core::Var::Set::Object());
					Data->Set("name", Core::Var::String(Query.first));

					auto* Cache = Query.second.Cache.ToSchema();
					if (Cache != nullptr)
						Data->Set("cache", Cache);
					else
						Data->Set("request", Core::Var::String(Query.second.Request));

					auto* Positions = Data->Set("positions", Core::Var::Set::Array());
					for (auto& Position : Query.second.Positions)
					{
						auto* Next = Positions->Push(Core::Var::Set::Array());
						Next->Push(Core::Var::String(Position.Key));
						Next->Push(Core::Var::Integer(Position.Offset));
						Next->Push(Core::Var::Boolean(Position.Escape));
					}
				}

				ED_DEBUG("[pq] OK save %llu parsed query templates", (uint64_t)Queries->Map.size());
				return Result;
			}
			Document Driver::GetQuery(const std::string& Name, Core::SchemaArgs* Map, bool Once)
			{
				ED_ASSERT(Queries && Safe, nullptr, "driver should be initialized");
				Safe->lock();
				auto It = Queries->Map.find(Name);
				if (It == Queries->Map.end())
				{
					Safe->unlock();
					if (Once && Map != nullptr)
					{
						for (auto& Item : *Map)
							ED_RELEASE(Item.second);
						Map->clear();
					}

					ED_ERR("[mongoc] template query %s does not exist", Name.c_str());
					return nullptr;
				}

				if (It->second.Cache.Get() != nullptr)
				{
					Document Result = It->second.Cache.Copy();
					Safe->unlock();

					if (Once && Map != nullptr)
					{
						for (auto& Item : *Map)
							ED_RELEASE(Item.second);
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
							ED_RELEASE(Item.second);
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
					{
						ED_ERR("[mongoc] template query %s\n\texpects parameter: %s", Name.c_str(), Word.Key.c_str());
						continue;
					}

					std::string Value = GetJSON(It->second, Word.Escape);
					if (Value.empty())
						continue;

					Result.Insert(Value, Word.Offset + Offset);
					Offset += Value.size();
				}

				if (Once)
				{
					for (auto& Item : *Map)
						ED_RELEASE(Item.second);
					Map->clear();
				}

				Document Data = Document::FromJSON(Origin.Request);
				if (!Data.Get())
					ED_ERR("[mongoc] could not construct query: \"%s\"", Name.c_str());

				return Data;
			}
			std::vector<std::string> Driver::GetQueries()
			{
				std::vector<std::string> Result;
				ED_ASSERT(Queries && Safe, Result, "driver should be initialized");

				Safe->lock();
				Result.reserve(Queries->Map.size());
				for (auto& Item : Queries->Map)
					Result.push_back(Item.first);
				Safe->unlock();

				return Result;
			}
			std::string Driver::GetJSON(Core::Schema* Source, bool Escape)
			{
				ED_ASSERT(Source != nullptr, std::string(), "source should be set");
				switch (Source->Value.GetType())
				{
					case Core::VarType::Object:
					{
						std::string Result = "{";
						for (auto* Node : Source->GetChilds())
						{
							Result.append(1, '\"').append(Node->Key).append("\":");
							Result.append(GetJSON(Node, true)).append(1, ',');
						}

						if (!Source->IsEmpty())
							Result = Result.substr(0, Result.size() - 1);

						return Result + "}";
					}
					case Core::VarType::Array:
					{
						std::string Result = "[";
						for (auto* Node : Source->GetChilds())
							Result.append(GetJSON(Node, true)).append(1, ',');

						if (!Source->IsEmpty())
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
					case Core::VarType::Binary:
					{
						if (Source->Value.GetSize() != 12)
						{
							std::string Base = Compute::Codec::Bep45Encode(Source->Value.GetBlob());
							return "\"" + Base + "\"";
						}

						return "{\"$oid\":\"" + Util::IdToString(Source->Value.GetBinary()) + "\"}";
					}
					case Core::VarType::Null:
					case Core::VarType::Undefined:
						return "null";
					default:
						break;
				}

				return "";
			}
			Core::Mapping<std::unordered_map<std::string, Driver::Sequence>>* Driver::Queries = nullptr;
			Core::Mapping<std::unordered_map<std::string, std::string>>* Driver::Constants = nullptr;
			std::mutex* Driver::Safe = nullptr;
			std::atomic<int> Driver::State(0);
			OnQueryLog Driver::Logger = nullptr;
			void* Driver::APM = nullptr;
		}
	}
}
