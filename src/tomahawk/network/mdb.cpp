#include "mdb.h"
#include <inttypes.h>
extern "C"
{
#ifdef TH_HAS_MONGOC
#include <mongoc.h>
#endif
}
#define MongoExecuteQuery(Function, Context, ...) ExecuteQuery(TH_STRINGIFY(Function), Function, Context, ##__VA_ARGS__)
#define MongoExecuteCursor(Function, Context, ...) ExecuteCursor(TH_STRINGIFY(Function), Function, Context, ##__VA_ARGS__)

namespace Tomahawk
{
	namespace Network
	{
		namespace MDB
		{
#ifdef TH_HAS_MONGOC
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
				TH_ASSERT(Base != nullptr, false, "context should be set");
				TH_PPUSH(TH_PERF_MAX);
				TH_DEBUG("[mongoc] execute query schema on 0x%" PRIXPTR "\n\t%s", (uintptr_t)Base, Name + 1);

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				bool Result = Function(Base, Data..., &Error);
				if (!Result && Error.code != 0)
					TH_ERR("[mongoc:%i] %s", (int)Error.code, Error.message);

				if (Result || Error.code == 0)
					TH_DEBUG("[mongoc] OK execute on 0x%" PRIXPTR, (uintptr_t)Base);

				TH_PPOP();
				return Result;
			}
			template <typename R, typename T, typename... Args>
			Cursor ExecuteCursor(const char* Name, R&& Function, T* Base, Args&&... Data)
			{
				TH_ASSERT(Base != nullptr, nullptr, "context should be set");
				TH_PPUSH(TH_PERF_MAX);
				TH_DEBUG("[mongoc] execute query cursor on 0x%" PRIXPTR "\n\t%s", (uintptr_t)Base, Name + 1);

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				TCursor* Result = Function(Base, Data...);
				if (!Result || mongoc_cursor_error(Result, &Error))
					TH_ERR("[mongoc:%i] %s", (int)Error.code, Error.message);
					
				if (Result || Error.code == 0)
					TH_DEBUG("[mongoc] OK execute on 0x%" PRIXPTR, (uintptr_t)Base);
				
				TH_PPOP();
				return Result;
			}
#endif
			Property::Property() noexcept : Source(nullptr), Mod(Type::Unknown), Integer(0), High(0), Low(0), Number(0.0), Boolean(false), IsValid(false)
			{
			}
			Property::Property(const Property& Other) noexcept : Name(Other.Name), String(Other.String), Source(Other.Source), Mod(Other.Mod), Integer(Other.Integer), High(Other.High), Low(Other.Low), Number(Other.Number), Boolean(Other.Boolean), IsValid(Other.IsValid)
			{
				if (Mod == Type::ObjectId)
					memcpy(ObjectId, Other.ObjectId, sizeof(ObjectId));
#ifdef TH_HAS_MONGOC
				if (Other.Source != nullptr)
					Source = bson_copy(Other.Source);
#endif
			}
			Property::Property(Property&& Other) noexcept : Name(std::move(Other.Name)), String(std::move(Other.String)), Source(Other.Source), Mod(Other.Mod), Integer(Other.Integer), High(Other.High), Low(Other.Low), Number(Other.Number), Boolean(Other.Boolean), IsValid(Other.IsValid)
			{
				if (Mod == Type::ObjectId)
					memcpy(ObjectId, Other.ObjectId, sizeof(ObjectId));

				Other.Source = nullptr;
			}
			Property::~Property()
			{
				Release();
			}
			void Property::Release()
			{
				Schema(Source).Release();
				Source = nullptr;
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
					case Type::Schema:
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
			TDocument* Property::GetOwnership()
			{
				if (!Source)
					return nullptr;

				TDocument* Result = Source;
				Source = nullptr;
				return Result;
			}
			Schema Property::Get() const
			{
				return Schema(Source);
			}
			Property& Property::operator= (const Property& Other) noexcept
			{
				if (&Other == this)
					return *this;

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

				if (Mod == Type::ObjectId)
					memcpy(ObjectId, Other.ObjectId, sizeof(ObjectId));
#ifdef TH_HAS_MONGOC
				if (Other.Source != nullptr)
					Source = bson_copy(Other.Source);
#endif
				return *this;
			}
			Property& Property::operator= (Property&& Other) noexcept
			{
				if (&Other == this)
					return *this;

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
#ifdef TH_HAS_MONGOC
				TH_ASSERT(Id12 != nullptr, false, "id should be set");
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
				TH_ASSERT(Value != nullptr, false, "value should be set");
				TH_ASSERT(High != nullptr, false, "high should be set");
				TH_ASSERT(Low != nullptr, false, "low should be set");
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
				TH_ASSERT(Id12 != nullptr, 0, "id should be set");

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
				TH_ASSERT(Id12 != nullptr, 0, "id should be set");

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
				TH_ASSERT(Id12 != nullptr, std::string(), "id should be set");

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
				TH_ASSERT(Id24.size() == 24, std::string(), "id should be 24 chars long");
#ifdef TH_HAS_MONGOC
				bson_oid_t Id;
				bson_oid_init_from_string(&Id, Id24.c_str());

				return std::string((const char*)Id.bytes, 12);
#else
				return "";
#endif
			}

			Schema::Schema() : Base(nullptr), Store(false)
			{
			}
			Schema::Schema(TDocument* NewBase) : Base(NewBase), Store(false)
			{
			}
			void Schema::Release() const
			{
#ifdef TH_HAS_MONGOC
				if (!Base || Store)
					return;

				if (!(Base->flags & BSON_FLAG_STATIC) && !(Base->flags & BSON_FLAG_RDONLY) && !(Base->flags & BSON_FLAG_INLINE) && !(Base->flags & BSON_FLAG_NO_FREE))
					bson_destroy((bson_t*)Base);
#endif
			}
			void Schema::Release()
			{
#ifdef TH_HAS_MONGOC
				if (!Base || Store)
					return;

				if (!(Base->flags & BSON_FLAG_STATIC) && !(Base->flags & BSON_FLAG_RDONLY) && !(Base->flags & BSON_FLAG_INLINE) && !(Base->flags & BSON_FLAG_NO_FREE))
					bson_destroy((bson_t*)Base);
				Base = nullptr;
#endif
			}
			void Schema::Join(const Schema& Value)
			{
#ifdef TH_HAS_MONGOC
				TH_ASSERT_V(Base != nullptr, "schema should be set");
				TH_ASSERT_V(Value.Base != nullptr, "other schema should be set");
				bson_concat((bson_t*)Base, (bson_t*)Value.Base);
				Value.Release();
#endif
			}
			void Schema::Loop(const std::function<bool(Property*)>& Callback) const
			{
				TH_ASSERT_V(Base != nullptr, "schema should be set");
				TH_ASSERT_V(Callback, "callback should be set");
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
			bool Schema::SetSchema(const char* Key, const Schema& Value, uint64_t ArrayId)
			{
#ifdef TH_HAS_MONGOC
				TH_ASSERT(Base != nullptr, false, "schema should be set");
				TH_ASSERT(Value.Base != nullptr, false, "other schema should be set");

				char Index[16];
				if (Key == nullptr)
					bson_uint32_to_string((uint32_t)ArrayId, &Key, Index, sizeof(Index));

				bool Result = bson_append_document((bson_t*)Base, Key, -1, (bson_t*)Value.Base);
				Value.Release();

				return Result;
#else
				return false;
#endif
			}
			bool Schema::SetArray(const char* Key, const Schema& Value, uint64_t ArrayId)
			{
#ifdef TH_HAS_MONGOC
				TH_ASSERT(Base != nullptr, false, "schema should be set");
				TH_ASSERT(Value.Base != nullptr, false, "other schema should be set");

				char Index[16];
				if (Key == nullptr)
					bson_uint32_to_string((uint32_t)ArrayId, &Key, Index, sizeof(Index));

				bool Result = bson_append_array((bson_t*)Base, Key, -1, (bson_t*)Value.Base);
				Value.Release();

				return Result;
#else
				return false;
#endif
			}
			bool Schema::SetString(const char* Key, const char* Value, uint64_t ArrayId)
			{
#ifdef TH_HAS_MONGOC
				TH_ASSERT(Base != nullptr, false, "schema should be set");
				TH_ASSERT(Value != nullptr, false, "value should be set");

				char Index[16];
				if (Key == nullptr)
					bson_uint32_to_string((uint32_t)ArrayId, &Key, Index, sizeof(Index));

				return bson_append_utf8((bson_t*)Base, Key, -1, Value, -1);
#else
				return false;
#endif
			}
			bool Schema::SetBlob(const char* Key, const char* Value, uint64_t Length, uint64_t ArrayId)
			{
#ifdef TH_HAS_MONGOC
				TH_ASSERT(Base != nullptr, false, "schema should be set");
				TH_ASSERT(Value != nullptr, false, "value should be set");

				char Index[16];
				if (Key == nullptr)
					bson_uint32_to_string((uint32_t)ArrayId, &Key, Index, sizeof(Index));

				return bson_append_utf8((bson_t*)Base, Key, -1, Value, (int)Length);
#else
				return false;
#endif
			}
			bool Schema::SetInteger(const char* Key, int64_t Value, uint64_t ArrayId)
			{
#ifdef TH_HAS_MONGOC
				TH_ASSERT(Base != nullptr, false, "schema should be set");

				char Index[16];
				if (Key == nullptr)
					bson_uint32_to_string((uint32_t)ArrayId, &Key, Index, sizeof(Index));

				return bson_append_int64((bson_t*)Base, Key, -1, Value);
#else
				return false;
#endif
			}
			bool Schema::SetNumber(const char* Key, double Value, uint64_t ArrayId)
			{
#ifdef TH_HAS_MONGOC
				TH_ASSERT(Base != nullptr, false, "schema should be set");

				char Index[16];
				if (Key == nullptr)
					bson_uint32_to_string((uint32_t)ArrayId, &Key, Index, sizeof(Index));

				return bson_append_double((bson_t*)Base, Key, -1, Value);
#else
				return false;
#endif
			}
			bool Schema::SetDecimal(const char* Key, uint64_t High, uint64_t Low, uint64_t ArrayId)
			{
#ifdef TH_HAS_MONGOC
				TH_ASSERT(Base != nullptr, false, "schema should be set");

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
			bool Schema::SetDecimalString(const char* Key, const std::string& Value, uint64_t ArrayId)
			{
#ifdef TH_HAS_MONGOC
				TH_ASSERT(Base != nullptr, false, "schema should be set");

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
			bool Schema::SetDecimalInteger(const char* Key, int64_t Value, uint64_t ArrayId)
			{
#ifdef TH_HAS_MONGOC
				TH_ASSERT(Base != nullptr, false, "schema should be set");

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
			bool Schema::SetDecimalNumber(const char* Key, double Value, uint64_t ArrayId)
			{
#ifdef TH_HAS_MONGOC
				TH_ASSERT(Base != nullptr, false, "schema should be set");

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
			bool Schema::SetBoolean(const char* Key, bool Value, uint64_t ArrayId)
			{
#ifdef TH_HAS_MONGOC
				TH_ASSERT(Base != nullptr, false, "schema should be set");

				char Index[16];
				if (Key == nullptr)
					bson_uint32_to_string((uint32_t)ArrayId, &Key, Index, sizeof(Index));

				return bson_append_bool((bson_t*)Base, Key, -1, Value);
#else
				return false;
#endif
			}
			bool Schema::SetObjectId(const char* Key, unsigned char Value[12], uint64_t ArrayId)
			{
#ifdef TH_HAS_MONGOC
				TH_ASSERT(Base != nullptr, false, "schema should be set");

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
			bool Schema::SetNull(const char* Key, uint64_t ArrayId)
			{
#ifdef TH_HAS_MONGOC
				TH_ASSERT(Base != nullptr, false, "schema should be set");

				char Index[16];
				if (Key == nullptr)
					bson_uint32_to_string((uint32_t)ArrayId, &Key, Index, sizeof(Index));

				return bson_append_null((bson_t*)Base, Key, -1);
#else
				return false;
#endif
			}
			bool Schema::SetProperty(const char* Key, Property* Value, uint64_t ArrayId)
			{
#ifdef TH_HAS_MONGOC
				TH_ASSERT(Base != nullptr, false, "schema should be set");
				TH_ASSERT(Value != nullptr, false, "property should be set");

				char Index[16];
				if (Key == nullptr)
					bson_uint32_to_string((uint32_t)ArrayId, &Key, Index, sizeof(Index));

				switch (Value->Mod)
				{
					case Type::Schema:
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
			bool Schema::HasProperty(const char* Key) const
			{
#ifdef TH_HAS_MONGOC
				TH_ASSERT(Base != nullptr, false, "schema should be set");
				TH_ASSERT(Key != nullptr, false, "key should be set");
				return bson_has_field(Base, Key);
#else
				return false;
#endif
			}
			bool Schema::GetProperty(const char* Key, Property* Output) const
			{
#ifdef TH_HAS_MONGOC
				TH_ASSERT(Base != nullptr, false, "schema should be set");
				TH_ASSERT(Key != nullptr, false, "key should be set");
				TH_ASSERT(Output != nullptr, false, "property should be set");

				bson_iter_t It;
				if (!bson_iter_init_find(&It, Base, Key))
					return false;

				return Clone(&It, Output);
#else
				return false;
#endif
			}
			bool Schema::Clone(void* It, Property* Output)
			{
#ifdef TH_HAS_MONGOC
				TH_ASSERT(It != nullptr, false, "iterator should be set");
				TH_ASSERT(Output != nullptr, false, "property should be set");

				const bson_value_t* Value = bson_iter_value((bson_iter_t*)It);
				if (!Value)
					return false;

				Output->IsValid = false;
				Output->Release();

				switch (Value->value_type)
				{
					case BSON_TYPE_DOCUMENT:
					{
						const uint8_t* Buffer; uint32_t Length;
						bson_iter_document((bson_iter_t*)It, &Length, &Buffer);
						Output->Mod = Type::Schema;
						Output->Source = bson_new_from_data(Buffer, Length);
						break;
					}
					case BSON_TYPE_ARRAY:
					{
						const uint8_t* Buffer; uint32_t Length;
						bson_iter_array((bson_iter_t*)It, &Length, &Buffer);
						Output->Mod = Type::Array;
						Output->Source = bson_new_from_data(Buffer, Length);
						break;
					}
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
			uint64_t Schema::Count() const
			{
#ifdef TH_HAS_MONGOC
				if (!Base)
					return 0;

				return bson_count_keys(Base);
#else
				return 0;
#endif
			}
			std::string Schema::ToRelaxedJSON() const
			{
#ifdef TH_HAS_MONGOC
				TH_ASSERT(Base != nullptr, std::string(), "schema should be set");
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
			std::string Schema::ToExtendedJSON() const
			{
#ifdef TH_HAS_MONGOC
				TH_ASSERT(Base != nullptr, std::string(), "schema should be set");
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
			std::string Schema::ToJSON() const
			{
#ifdef TH_HAS_MONGOC
				TH_ASSERT(Base != nullptr, std::string(), "schema should be set");
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
			std::string Schema::ToIndices() const
			{
#ifdef TH_HAS_MONGOC
				TH_ASSERT(Base != nullptr, std::string(), "schema should be set");
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
			Core::Schema* Schema::ToSchema(bool IsArray) const
			{
#ifdef TH_HAS_MONGOC
				if (!Base)
					return nullptr;

				Core::Schema* Node = (IsArray ? Core::Var::Set::Array() : Core::Var::Set::Object());
				Loop([Node](Property* Key) -> bool
				{
					std::string Name = (Node->Value.GetType() == Core::VarType::Array ? "" : Key->Name);
					switch (Key->Mod)
					{
						case Type::Schema:
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
			TDocument* Schema::Get() const
			{
#ifdef TH_HAS_MONGOC
				return Base;
#else
				return nullptr;
#endif
			}
			Schema Schema::Copy() const
			{
#ifdef TH_HAS_MONGOC
				if (!Base)
					return nullptr;

				return Schema(bson_copy(Base));
#else
				return nullptr;
#endif
			}
			Schema& Schema::Persist(bool Keep)
			{
				Store = Keep;
				return *this;
			}
			Schema Schema::FromDocument(Core::Schema* Src)
			{
#ifdef TH_HAS_MONGOC
				TH_ASSERT(Src != nullptr && Src->Value.IsObject(), nullptr, "schema should be set");
				bool Array = (Src->Value.GetType() == Core::VarType::Array);
				Schema Result = bson_new();
				uint64_t Index = 0;

				for (auto&& Node : Src->GetChilds())
				{
					switch (Node->Value.GetType())
					{
						case Core::VarType::Object:
							Result.SetSchema(Array ? nullptr : Node->Key.c_str(), Schema::FromDocument(Node), Index);
							break;
						case Core::VarType::Array:
							Result.SetArray(Array ? nullptr : Node->Key.c_str(), Schema::FromDocument(Node), Index);
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
			Schema Schema::FromEmpty()
			{
#ifdef TH_HAS_MONGOC
				return bson_new();
#else
				return nullptr;
#endif
			}
			Schema Schema::FromJSON(const std::string& JSON)
			{
#ifdef TH_HAS_MONGOC
				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				TDocument* Result = bson_new_from_json((unsigned char*)JSON.c_str(), (ssize_t)JSON.size(), &Error);
				if (Result != nullptr && Error.code == 0)
					return Result;

				if (Result != nullptr)
					bson_destroy((bson_t*)Result);

				TH_ERR("[json] %s", Error.message);
				return nullptr;
#else
				return nullptr;
#endif
			}
			Schema Schema::FromBuffer(const unsigned char* Buffer, uint64_t Length)
			{
#ifdef TH_HAS_MONGOC
				TH_ASSERT(Buffer != nullptr, nullptr, "buffer should be set");
				return bson_new_from_data(Buffer, (size_t)Length);
#else
				return nullptr;
#endif
			}
			Schema Schema::FromSource(TDocument* Src)
			{
#ifdef TH_HAS_MONGOC
				TH_ASSERT(Src != nullptr, nullptr, "src should be set");
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
				TH_ASSERT_V(Base != nullptr, "context should be set");
				TH_ASSERT_V(Name != nullptr, "name should be set");

				mongoc_uri_set_option_as_int32(Base, Name, (int32_t)Value);
#endif
			}
			void Address::SetOption(const char* Name, bool Value)
			{
#ifdef TH_HAS_MONGOC
				TH_ASSERT_V(Base != nullptr, "context should be set");
				TH_ASSERT_V(Name != nullptr, "name should be set");

				mongoc_uri_set_option_as_bool(Base, Name, Value);
#endif
			}
			void Address::SetOption(const char* Name, const char* Value)
			{
#ifdef TH_HAS_MONGOC
				TH_ASSERT_V(Base != nullptr, "context should be set");
				TH_ASSERT_V(Name != nullptr, "name should be set");
				TH_ASSERT_V(Value != nullptr, "value should be set");

				mongoc_uri_set_option_as_utf8(Base, Name, Value);
#endif
			}
			void Address::SetAuthMechanism(const char* Value)
			{
#ifdef TH_HAS_MONGOC
				TH_ASSERT_V(Base != nullptr, "context should be set");
				TH_ASSERT_V(Value != nullptr, "value should be set");

				mongoc_uri_set_auth_mechanism(Base, Value);
#endif
			}
			void Address::SetAuthSource(const char* Value)
			{
#ifdef TH_HAS_MONGOC
				TH_ASSERT_V(Base != nullptr, "context should be set");
				TH_ASSERT_V(Value != nullptr, "value should be set");

				mongoc_uri_set_auth_source(Base, Value);
#endif
			}
			void Address::SetCompressors(const char* Value)
			{
#ifdef TH_HAS_MONGOC
				TH_ASSERT_V(Base != nullptr, "context should be set");
				TH_ASSERT_V(Value != nullptr, "value should be set");

				mongoc_uri_set_compressors(Base, Value);
#endif
			}
			void Address::SetDatabase(const char* Value)
			{
#ifdef TH_HAS_MONGOC
				TH_ASSERT_V(Base != nullptr, "context should be set");
				TH_ASSERT_V(Value != nullptr, "value should be set");

				mongoc_uri_set_database(Base, Value);
#endif
			}
			void Address::SetUsername(const char* Value)
			{
#ifdef TH_HAS_MONGOC
				TH_ASSERT_V(Base != nullptr, "context should be set");
				TH_ASSERT_V(Value != nullptr, "value should be set");

				mongoc_uri_set_username(Base, Value);
#endif
			}
			void Address::SetPassword(const char* Value)
			{
#ifdef TH_HAS_MONGOC
				TH_ASSERT_V(Base != nullptr, "context should be set");
				TH_ASSERT_V(Value != nullptr, "value should be set");

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
				TH_ASSERT(Value != nullptr, nullptr, "value should be set");
				TAddress* Result = mongoc_uri_new(Value);
				if (!strstr(Value, MONGOC_URI_SOCKETTIMEOUTMS))
					mongoc_uri_set_option_as_int32(Result, MONGOC_URI_SOCKETTIMEOUTMS, 10000);

				return Result;
#else
				return nullptr;
#endif
			}

			Stream::Stream() : IOptions(nullptr), Source(nullptr), Base(nullptr), Count(0)
			{
			}
			Stream::Stream(TCollection* NewSource, TStream* NewBase, const Schema& NewOptions) : IOptions(NewOptions), Source(NewSource), Base(NewBase), Count(0)
			{
			}
			void Stream::Release()
			{
#ifdef TH_HAS_MONGOC
				IOptions.Release();
				Source = nullptr;
				if (!Base)
					return;

				mongoc_bulk_operation_destroy(Base);
				Base = nullptr;
#endif
			}
			bool Stream::RemoveMany(const Schema& Match, const Schema& Options)
			{
#ifdef TH_HAS_MONGOC
				if (!NextOperation())
					return false;

				bool Subresult = MongoExecuteQuery(&mongoc_bulk_operation_remove_many_with_opts, Base, Match.Get(), Options.Get());
				Match.Release();
				Options.Release();

				return Subresult;
#else
				return false;
#endif
			}
			bool Stream::RemoveOne(const Schema& Match, const Schema& Options)
			{
#ifdef TH_HAS_MONGOC
				if (!NextOperation())
					return false;

				bool Subresult = MongoExecuteQuery(&mongoc_bulk_operation_remove_one_with_opts, Base, Match.Get(), Options.Get());
				Match.Release();
				Options.Release();

				return Subresult;
#else
				return false;
#endif
			}
			bool Stream::ReplaceOne(const Schema& Match, const Schema& Replacement, const Schema& Options)
			{
#ifdef TH_HAS_MONGOC
				if (!NextOperation())
					return false;

				bool Subresult = MongoExecuteQuery(&mongoc_bulk_operation_replace_one_with_opts, Base, Match.Get(), Replacement.Get(), Options.Get());
				Match.Release();
				Replacement.Release();
				Options.Release();

				return Subresult;
#else
				return false;
#endif
			}
			bool Stream::InsertOne(const Schema& Result, const Schema& Options)
			{
#ifdef TH_HAS_MONGOC
				if (!NextOperation())
					return false;

				bool Subresult = MongoExecuteQuery(&mongoc_bulk_operation_insert_with_opts, Base, Result.Get(), Options.Get());
				Result.Release();
				Options.Release();

				return Subresult;
#else
				return false;
#endif
			}
			bool Stream::UpdateOne(const Schema& Match, const Schema& Result, const Schema& Options)
			{
#ifdef TH_HAS_MONGOC
				if (!NextOperation())
					return false;

				bool Subresult = MongoExecuteQuery(&mongoc_bulk_operation_update_one_with_opts, Base, Match.Get(), Result.Get(), Options.Get());
				Match.Release();
				Result.Release();
				Options.Release();

				return Subresult;
#else
				return false;
#endif
			}
			bool Stream::UpdateMany(const Schema& Match, const Schema& Result, const Schema& Options)
			{
#ifdef TH_HAS_MONGOC
				if (!NextOperation())
					return false;

				bool Subresult = MongoExecuteQuery(&mongoc_bulk_operation_update_many_with_opts, Base, Match.Get(), Result.Get(), Options.Get());
				Match.Release();
				Result.Release();
				Options.Release();

				return Subresult;
#else
				return false;
#endif
			}
			bool Stream::TemplateQuery(const std::string& Name, Core::SchemaArgs* Map, bool Once)
			{
				TH_DEBUG("[mongoc] template query %s", Name.empty() ? "empty-query-name" : Name.c_str());
				return Query(Driver::GetQuery(Name, Map, Once));
			}
			bool Stream::Query(const Schema& Command)
			{
#ifdef TH_HAS_MONGOC
				if (!Command.Get())
				{
					TH_ERR("[mongoc] cannot run empty query");
					return false;
				}

				Property Type;
				if (!Command.GetProperty("type", &Type) || Type.Mod != Type::String)
				{
					Command.Release();
					TH_ERR("[mongoc] cannot run query without query @type");
					return false;
				}

				if (Type.String == "update")
				{
					Property Match;
					if (!Command.GetProperty("match", &Match) || Match.Mod != Type::Schema)
					{
						Command.Release();
						TH_ERR("[mongoc] cannot run update-one query without @match");
						return false;
					}

					Property Update;
					if (!Command.GetProperty("update", &Update) || Update.Mod != Type::Schema)
					{
						Command.Release();
						TH_ERR("[mongoc] cannot run update-one query without @update");
						return false;
					}

					Property Options = Command["options"];
					Command.Release();

					return UpdateOne(Match.GetOwnership(), Update.GetOwnership(), Options.GetOwnership());
				}
				else if (Type.String == "update-many")
				{
					Property Match;
					if (!Command.GetProperty("match", &Match) || Match.Mod != Type::Schema)
					{
						Command.Release();
						TH_ERR("[mongoc] cannot run update-many query without @match");
						return false;
					}

					Property Update;
					if (!Command.GetProperty("update", &Update) || Update.Mod != Type::Schema)
					{
						Command.Release();
						TH_ERR("[mongoc] cannot run update-many query without @update");
						return false;
					}

					Property Options = Command["options"];
					Command.Release();

					return UpdateMany(Match.GetOwnership(), Update.GetOwnership(), Options.GetOwnership());
				}
				else if (Type.String == "insert")
				{
					Property Value;
					if (!Command.GetProperty("value", &Value) || Value.Mod != Type::Schema)
					{
						Command.Release();
						TH_ERR("[mongoc] cannot run insert-one query without @value");
						return false;
					}

					Property Options = Command["options"];
					Command.Release();

					return InsertOne(Value.GetOwnership(), Options.GetOwnership());
				}
				else if (Type.String == "replace")
				{
					Property Match;
					if (!Command.GetProperty("match", &Match) || Match.Mod != Type::Schema)
					{
						Command.Release();
						TH_ERR("[mongoc] cannot run replace-one query without @match");
						return false;
					}

					Property Value;
					if (!Command.GetProperty("value", &Value) || Value.Mod != Type::Schema)
					{
						Command.Release();
						TH_ERR("[mongoc] cannot run replace-one query without @value");
						return false;
					}

					Property Options = Command["options"];
					Command.Release();

					return ReplaceOne(Match.GetOwnership(), Value.GetOwnership(), Options.GetOwnership());
				}
				else if (Type.String == "remove")
				{
					Property Match;
					if (!Command.GetProperty("match", &Match) || Match.Mod != Type::Schema)
					{
						Command.Release();
						TH_ERR("[mongoc] cannot run remove-one query without @value");
						return false;
					}

					Property Options = Command["options"];
					Command.Release();

					return RemoveOne(Match.GetOwnership(), Options.GetOwnership());
				}
				else if (Type.String == "remove-many")
				{
					Property Match;
					if (!Command.GetProperty("match", &Match) || Match.Mod != Type::Schema)
					{
						Command.Release();
						TH_ERR("[mongoc] cannot run remove-many query without @value");
						return false;
					}

					Property Options = Command["options"];
					Command.Release();

					return RemoveMany(Match.GetOwnership(), Options.GetOwnership());
				}

				Command.Release();
				TH_ERR("[mongoc] cannot find query of type \"%s\"", Type.String.c_str());
				return false;
#else
				return false;
#endif
			}
			bool Stream::NextOperation()
			{
#ifdef TH_HAS_MONGOC
				if (!Base || !Source)
					return false;

				bool State = true;
				if (Count > 768)
				{
					TDocument Result;
					State = MongoExecuteQuery(&mongoc_bulk_operation_execute, Base, &Result);
					bson_destroy((bson_t*)&Result);

					if (Source != nullptr)
						*this = Collection(Source).CreateStream(IOptions);
				}
				else
					Count++;

				return State;
#else
				return false;
#endif
			}
			Core::Async<Schema> Stream::ExecuteWithReply()
			{
#ifdef TH_HAS_MONGOC
				if (!Base)
					return Schema(nullptr);

				Stream Context = *this; Base = nullptr;
				return Core::Async<Schema>::Execute([Context](Core::Async<Schema>& Future) mutable
				{
					TDocument Subresult;
					bool fResult = MongoExecuteQuery(&mongoc_bulk_operation_execute, Context.Get(), &Subresult);
					Context.Release();

					if (!fResult)
						Future = Schema(nullptr);
					else
						Future = Schema::FromSource(&Subresult);
					bson_destroy((bson_t*)&Subresult);
				});
#else
				return Schema(nullptr);
#endif
			}
			Core::Async<bool> Stream::Execute()
			{
#ifdef TH_HAS_MONGOC
				if (!Base)
					return true;

				Stream Context = *this; Base = nullptr;
				return Core::Async<bool>::Execute([Context](Core::Async<bool>& Future) mutable
				{
					TDocument Result;
					bool Subresult = MongoExecuteQuery(&mongoc_bulk_operation_execute, Context.Get(), &Result);
					bson_destroy((bson_t*)&Result);
					Context.Release();

					Future = Subresult;
				});
#else
				return false;
#endif
			}
			uint64_t Stream::GetHint() const
			{
#ifdef TH_HAS_MONGOC
				TH_ASSERT(Base != nullptr, 0, "context should be set");
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
					TH_ERR("[mongoc] %s", Error.message);

				mongoc_cursor_destroy(Base);
				Base = nullptr;
#endif
			}
			void Cursor::SetMaxAwaitTime(uint64_t MaxAwaitTime)
			{
#ifdef TH_HAS_MONGOC
				TH_ASSERT_V(Base != nullptr, "context should be set");
				mongoc_cursor_set_max_await_time_ms(Base, (uint32_t)MaxAwaitTime);
#endif
			}
			void Cursor::SetBatchSize(uint64_t BatchSize)
			{
#ifdef TH_HAS_MONGOC
				TH_ASSERT_V(Base != nullptr, "context should be set");
				mongoc_cursor_set_batch_size(Base, (uint32_t)BatchSize);
#endif
			}
			bool Cursor::SetLimit(int64_t Limit)
			{
#ifdef TH_HAS_MONGOC
				TH_ASSERT(Base != nullptr, false, "context should be set");
				return mongoc_cursor_set_limit(Base, Limit);
#else
				return false;
#endif
			}
			bool Cursor::SetHint(uint64_t Hint)
			{
#ifdef TH_HAS_MONGOC
				TH_ASSERT(Base != nullptr, false, "context should be set");
				return mongoc_cursor_set_hint(Base, (uint32_t)Hint);
#else
				return false;
#endif
			}
			bool Cursor::HasError() const
			{
#ifdef TH_HAS_MONGOC
				if (!Base)
					return true;

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				if (!mongoc_cursor_error(Base, &Error))
					return false;

				TH_ERR("[mongoc] %s", Error.message);
				return true;
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
			Core::Async<bool> Cursor::Next() const
			{
#ifdef TH_HAS_MONGOC
				if (!Base)
					return false;

				auto* Context = Base;
				return Core::Async<bool>::Execute([Context](Core::Async<bool>& Future)
				{
					TH_PPUSH(TH_PERF_MAX);
					TDocument* Query = nullptr;
					Future = mongoc_cursor_next(Context, (const TDocument**)&Query);
					TH_PPOP();
				});
#else
				return false;
#endif
			}
			int64_t Cursor::GetId() const
			{
#ifdef TH_HAS_MONGOC
				TH_ASSERT(Base != nullptr, 0, "context should be set");
				return (int64_t)mongoc_cursor_get_id(Base);
#else
				return 0;
#endif
			}
			int64_t Cursor::GetLimit() const
			{
#ifdef TH_HAS_MONGOC
				TH_ASSERT(Base != nullptr, 0, "context should be set");
				return (int64_t)mongoc_cursor_get_limit(Base);
#else
				return 0;
#endif
			}
			uint64_t Cursor::GetMaxAwaitTime() const
			{
#ifdef TH_HAS_MONGOC
				TH_ASSERT(Base != nullptr, 0, "context should be set");
				return (uint64_t)mongoc_cursor_get_max_await_time_ms(Base);
#else
				return 0;
#endif
			}
			uint64_t Cursor::GetBatchSize() const
			{
#ifdef TH_HAS_MONGOC
				TH_ASSERT(Base != nullptr, 0, "context should be set");
				return (uint64_t)mongoc_cursor_get_batch_size(Base);
#else
				return 0;
#endif
			}
			uint64_t Cursor::GetHint() const
			{
#ifdef TH_HAS_MONGOC
				TH_ASSERT(Base != nullptr, 0, "context should be set");
				return (uint64_t)mongoc_cursor_get_hint(Base);
#else
				return 0;
#endif
			}
			Schema Cursor::GetCurrent() const
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
				TH_ASSERT(Base != nullptr, nullptr, "context should be set");
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

			Response::Response() : ISuccess(false)
			{
			}
			Response::Response(const Response& Other) : ICursor(Other.ICursor), IDocument(Other.IDocument), ISuccess(Other.ISuccess)
			{
			}
			Response::Response(const Cursor& _Cursor) : ICursor(_Cursor), ISuccess(_Cursor && !_Cursor.HasError() && _Cursor.HasMoreData())
			{
			}
			Response::Response(const Schema& _Document) : IDocument(_Document), ISuccess(_Document.Get() != nullptr)
			{
			}
			Response::Response(bool _Success) : ISuccess(_Success)
			{
			}
			void Response::Release()
			{
				ICursor.Release();
				IDocument.Release();
			}
			Core::Async<Core::Schema*> Response::Fetch() const
			{
				if (IDocument)
					return IDocument.ToSchema();

				if (!ICursor)
					return (Core::Schema*)nullptr;

				Cursor Context(ICursor);
				return ICursor.Next().Then<Core::Schema*>([Context](bool&& Result)
				{
					return Context.GetCurrent().ToSchema();
				});
			}
			Core::Async<Core::Schema*> Response::FetchAll() const
			{
				if (IDocument)
				{
					Core::Schema* Result = IDocument.ToSchema();
					return Result ? Result : Core::Var::Set::Array();
				}

				if (!ICursor)
					return Core::Var::Set::Array();

				Cursor Context(ICursor);
				return Core::Coasync<Core::Schema*>([Context]()
				{
					Core::Schema* Result = Core::Var::Set::Array();
					while (TH_AWAIT(Context.Next()))
						Result->Push(Context.GetCurrent().ToSchema());

					return Result;
				});
			}
			Property Response::GetProperty(const char* Name)
			{
				TH_ASSERT(Name != nullptr, Property(), "context should be set");

				Property Result;
				if (IDocument)
				{
					IDocument.GetProperty(Name, &Result);
					return Result;
				}

				if (!ICursor)
					return Result;

				Schema Source = ICursor.GetCurrent();
				if (Source)
				{
					Source.GetProperty(Name, &Result);
					return Result;
				}

				if (!TH_AWAIT(ICursor.Next()))
					return Result;

				Source = ICursor.GetCurrent();
				if (Source)
					Source.GetProperty(Name, &Result);

				return Result;
			}
			Cursor Response::GetCursor() const
			{
				return ICursor;
			}
			Schema Response::GetDocument() const
			{
				return IDocument;
			}
			bool Response::IsOK() const
			{
				return ISuccess;
			}
			bool Response::OK()
			{
				bool Success = ISuccess;
				Release();

				return Success;
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
				return Core::Async<bool>::Execute([Context, NewDatabaseName, NewCollectionName](Core::Async<bool>& Future)
				{
					Future = MongoExecuteQuery(&mongoc_collection_rename, Context, NewDatabaseName.c_str(), NewCollectionName.c_str(), false);
				});
#else
				return false;
#endif
			}
			Core::Async<bool> Collection::RenameWithOptions(const std::string& NewDatabaseName, const std::string& NewCollectionName, const Schema& Options)
			{
#ifdef TH_HAS_MONGOC
				auto* Context = Base;
				return Core::Async<bool>::Execute([Context, NewDatabaseName, NewCollectionName, Options](Core::Async<bool>& Future)
				{
					bool Subresult = MongoExecuteQuery(&mongoc_collection_rename_with_opts, Context, NewDatabaseName.c_str(), NewCollectionName.c_str(), false, Options.Get());
					Options.Release();

					Future = Subresult;
				});
#else
				return false;
#endif
			}
			Core::Async<bool> Collection::RenameWithRemove(const std::string& NewDatabaseName, const std::string& NewCollectionName)
			{
#ifdef TH_HAS_MONGOC
				auto* Context = Base;
				return Core::Async<bool>::Execute([Context, NewDatabaseName, NewCollectionName](Core::Async<bool>& Future)
				{
					Future = MongoExecuteQuery(&mongoc_collection_rename, Context, NewDatabaseName.c_str(), NewCollectionName.c_str(), true);
				});
#else
				return false;
#endif
			}
			Core::Async<bool> Collection::RenameWithOptionsAndRemove(const std::string& NewDatabaseName, const std::string& NewCollectionName, const Schema& Options)
			{
#ifdef TH_HAS_MONGOC
				auto* Context = Base;
				return Core::Async<bool>::Execute([Context, NewDatabaseName, NewCollectionName, Options](Core::Async<bool>& Future)
				{
					bool Subresult = MongoExecuteQuery(&mongoc_collection_rename_with_opts, Context, NewDatabaseName.c_str(), NewCollectionName.c_str(), true, Options.Get());
					Options.Release();

					Future = Subresult;
				});
#else
				return false;
#endif
			}
			Core::Async<bool> Collection::Remove(const Schema& Options)
			{
#ifdef TH_HAS_MONGOC
				auto* Context = Base;
				return Core::Async<bool>::Execute([Context, Options](Core::Async<bool>& Future)
				{
					bool Subresult = MongoExecuteQuery(&mongoc_collection_drop_with_opts, Context, Options.Get());
					Options.Release();

					Future = Subresult;
				});
#else
				return false;
#endif
			}
			Core::Async<bool> Collection::RemoveIndex(const std::string& Name, const Schema& Options)
			{
#ifdef TH_HAS_MONGOC
				auto* Context = Base;
				return Core::Async<bool>::Execute([Context, Name, Options](Core::Async<bool>& Future)
				{
					bool Subresult = MongoExecuteQuery(&mongoc_collection_drop_index_with_opts, Context, Name.c_str(), Options.Get());
					Options.Release();

					Future = Subresult;
				});
#else
				return false;
#endif
			}
			Core::Async<Schema> Collection::RemoveMany(const Schema& Match, const Schema& Options)
			{
#ifdef TH_HAS_MONGOC
				auto* Context = Base;
				return Core::Async<Schema>::Execute([Context, Match, Options](Core::Async<Schema>& Future)
				{
					TDocument Subresult;
					bool fResult = MongoExecuteQuery(&mongoc_collection_delete_many, Context, Match.Get(), Options.Get(), &Subresult);
					Match.Release();
					Options.Release();

					if (!fResult)
						Future = Schema(nullptr);
					else
						Future = Schema::FromSource(&Subresult);
					bson_destroy((bson_t*)&Subresult);
				});
#else
				return Schema(nullptr);
#endif
			}
			Core::Async<Schema> Collection::RemoveOne(const Schema& Match, const Schema& Options)
			{
#ifdef TH_HAS_MONGOC
				auto* Context = Base;
				return Core::Async<Schema>::Execute([Context, Match, Options](Core::Async<Schema>& Future)
				{
					TDocument Subresult;
					bool fResult = MongoExecuteQuery(&mongoc_collection_delete_one, Context, Match.Get(), Options.Get(), &Subresult);
					Match.Release();
					Options.Release();

					if (!fResult)
						Future = Schema(nullptr);
					else
						Future = Schema::FromSource(&Subresult);
					bson_destroy((bson_t*)&Subresult);
				});
#else
				return Schema(nullptr);
#endif
			}
			Core::Async<Schema> Collection::ReplaceOne(const Schema& Match, const Schema& Replacement, const Schema& Options)
			{
#ifdef TH_HAS_MONGOC
				auto* Context = Base;
				return Core::Async<Schema>::Execute([Context, Match, Replacement, Options](Core::Async<Schema>& Future)
				{
					TDocument Subresult;
					bool fResult = MongoExecuteQuery(&mongoc_collection_replace_one, Context, Match.Get(), Replacement.Get(), Options.Get(), &Subresult);
					Match.Release();
					Replacement.Release();
					Options.Release();

					if (!fResult)
						Future = Schema(nullptr);
					else
						Future = Schema::FromSource(&Subresult);
					bson_destroy((bson_t*)&Subresult);
				});
#else
				return Schema(nullptr);
#endif
			}
			Core::Async<Schema> Collection::InsertMany(std::vector<Schema>& List, const Schema& Options)
			{
#ifdef TH_HAS_MONGOC
				TH_ASSERT(!List.empty(), Schema(nullptr), "insert array should not be empty");
				std::vector<Schema> Array(std::move(List));
				auto* Context = Base;

				return Core::Async<Schema>::Execute([Context, Array = std::move(Array), Options](Core::Async<Schema>& Future) mutable
				{
					TDocument** Subarray = TH_MALLOC(TDocument*, sizeof(TDocument*) * Array.size());
					for (size_t i = 0; i < Array.size(); i++)
						Subarray[i] = Array[i].Get();

					TDocument Subresult;
					bool fResult = MongoExecuteQuery(&mongoc_collection_insert_many, Context, (const TDocument**)Subarray, (size_t)Array.size(), Options.Get(), &Subresult);
					for (auto& Item : Array)
						Item.Release();
					Options.Release();
					TH_FREE(Subarray);

					if (!fResult)
						Future = Schema(nullptr);
					else
						Future = Schema::FromSource(&Subresult);
					bson_destroy((bson_t*)&Subresult);
				});
#else
				return Schema(nullptr);
#endif
			}
			Core::Async<Schema> Collection::InsertOne(const Schema& Result, const Schema& Options)
			{
#ifdef TH_HAS_MONGOC
				auto* Context = Base;
				return Core::Async<Schema>::Execute([Context, Result, Options](Core::Async<Schema>& Future)
				{
					TDocument Subresult;
					bool fResult = MongoExecuteQuery(&mongoc_collection_insert_one, Context, Result.Get(), Options.Get(), &Subresult);
					Options.Release();
					Result.Release();

					if (!fResult)
						Future = Schema(nullptr);
					else
						Future = Schema::FromSource(&Subresult);
					bson_destroy((bson_t*)&Subresult);
				});
#else
				return Schema(nullptr);
#endif
			}
			Core::Async<Schema> Collection::UpdateMany(const Schema& Match, const Schema& Update, const Schema& Options)
			{
#ifdef TH_HAS_MONGOC
				auto* Context = Base;
				return Core::Async<Schema>::Execute([Context, Match, Update, Options](Core::Async<Schema>& Future)
				{
					TDocument Subresult;
					bool fResult = MongoExecuteQuery(&mongoc_collection_update_many, Context, Match.Get(), Update.Get(), Options.Get(), &Subresult);
					Match.Release();
					Update.Release();
					Options.Release();

					if (!fResult)
						Future = Schema(nullptr);
					else
						Future = Schema::FromSource(&Subresult);
					bson_destroy((bson_t*)&Subresult);
				});
#else
				return Schema(nullptr);
#endif
			}
			Core::Async<Schema> Collection::UpdateOne(const Schema& Match, const Schema& Update, const Schema& Options)
			{
#ifdef TH_HAS_MONGOC
				auto* Context = Base;
				return Core::Async<Schema>::Execute([Context, Match, Update, Options](Core::Async<Schema>& Future)
				{
					TDocument Subresult;
					bool fResult = MongoExecuteQuery(&mongoc_collection_update_one, Context, Match.Get(), Update.Get(), Options.Get(), &Subresult);
					Match.Release();
					Update.Release();
					Options.Release();

					if (!fResult)
						Future = Schema(nullptr);
					else
						Future = Schema::FromSource(&Subresult);
					bson_destroy((bson_t*)&Subresult);
				});
#else
				return Schema(nullptr);
#endif
			}
			Core::Async<Schema> Collection::FindAndModify(const Schema& Query, const Schema& Sort, const Schema& Update, const Schema& Fields, bool RemoveAt, bool Upsert, bool New)
			{
#ifdef TH_HAS_MONGOC
				auto* Context = Base;
				return Core::Async<Schema>::Execute([Context, Query, Sort, Update, Fields, RemoveAt, Upsert, New](Core::Async<Schema>& Future)
				{
					TDocument Subresult;
					bool fResult = MongoExecuteQuery(&mongoc_collection_find_and_modify, Context, Query.Get(), Sort.Get(), Update.Get(), Fields.Get(), RemoveAt, Upsert, New, &Subresult);
					Query.Release();
					Sort.Release();
					Update.Release();
					Fields.Release();

					if (!fResult)
						Future = Schema(nullptr);
					else
						Future = Schema::FromSource(&Subresult);
					bson_destroy((bson_t*)&Subresult);
				});
#else
				return Schema(nullptr);
#endif
			}
			Core::Async<uint64_t> Collection::CountDocuments(const Schema& Match, const Schema& Options) const
			{
#ifdef TH_HAS_MONGOC
				auto* Context = Base;
				return Core::Async<uint64_t>::Execute([Context, Match, Options](Core::Async<uint64_t>& Future)
				{
					uint64_t Subresult = (uint64_t)MongoExecuteQuery(&mongoc_collection_count_documents, Context, Match.Get(), Options.Get(), nullptr, nullptr);
					Match.Release();
					Options.Release();

					Future = Subresult;
				});
#else
				return 0;
#endif
			}
			Core::Async<uint64_t> Collection::CountDocumentsEstimated(const Schema& Options) const
			{
#ifdef TH_HAS_MONGOC
				auto* Context = Base;
				return Core::Async<uint64_t>::Execute([Context, Options](Core::Async<uint64_t>& Future)
				{
					uint64_t Subresult = (uint64_t)MongoExecuteQuery(&mongoc_collection_estimated_document_count, Context, Options.Get(), nullptr, nullptr);
					Options.Release();

					Future = Subresult;
				});
#else
				return 0;
#endif
			}
			Core::Async<Cursor> Collection::FindIndexes(const Schema& Options) const
			{
#ifdef TH_HAS_MONGOC
				auto* Context = Base;
				return Core::Async<Cursor>::Execute([Context, Options](Core::Async<Cursor>& Future)
				{
					Cursor Subresult = MongoExecuteCursor(&mongoc_collection_find_indexes_with_opts, Context, Options.Get());
					Options.Release();

					Future = Subresult;
				});
#else
				return Cursor(nullptr);
#endif
			}
			Core::Async<Cursor> Collection::FindMany(const Schema& Match, const Schema& Options) const
			{
#ifdef TH_HAS_MONGOC
				auto* Context = Base;
				return Core::Async<Cursor>::Execute([Context, Match, Options](Core::Async<Cursor>& Future)
				{
					Cursor Subresult = MongoExecuteCursor(&mongoc_collection_find_with_opts, Context, Match.Get(), Options.Get(), nullptr);
					Match.Release();
					Options.Release();

					Future = Subresult;
				});
#else
				return Cursor(nullptr);
#endif
			}
			Core::Async<Cursor> Collection::FindOne(const Schema& Match, const Schema& Options) const
			{
#ifdef TH_HAS_MONGOC
				auto* Context = Base;
				return Core::Async<Cursor>::Execute([Context, Match, Options](Core::Async<Cursor>& Future)
				{
					Schema Settings = Options;
					if (Settings.Get() != nullptr)
						Settings.SetInteger("limit", 1);
					else
						Settings = Schema(BCON_NEW("limit", BCON_INT32(1)));

					Cursor Subresult = MongoExecuteCursor(&mongoc_collection_find_with_opts, Context, Match.Get(), Settings.Get(), nullptr);
					if (!Options.Get() && Settings.Get())
						Settings.Release();
					Match.Release();
					Options.Release();

					Future = Subresult;
				});
#else
				return Cursor(nullptr);
#endif
			}
			Core::Async<Cursor> Collection::Aggregate(QueryFlags Flags, const Schema& Pipeline, const Schema& Options) const
			{
#ifdef TH_HAS_MONGOC
				auto* Context = Base;
				return Core::Async<Cursor>::Execute([Context, Flags, Pipeline, Options](Core::Async<Cursor>& Future)
				{
					Cursor Subresult = MongoExecuteCursor(&mongoc_collection_aggregate, Context, (mongoc_query_flags_t)Flags, Pipeline.Get(), Options.Get(), nullptr);
					Pipeline.Release();
					Options.Release();

					Future = Subresult;
				});
#else
				return Cursor(nullptr);
#endif
			}
			Core::Async<Response> Collection::TemplateQuery(const std::string& Name, Core::SchemaArgs* Map, bool Once, Transaction* Session)
			{
				TH_DEBUG("[mongoc] template query %s", Name.empty() ? "empty-query-name" : Name.c_str());
				return Query(Driver::GetQuery(Name, Map, Once), Session);
			}
			Core::Async<Response> Collection::Query(const Schema& Command, Transaction* Session)
			{
#ifdef TH_HAS_MONGOC
				if (!Command.Get())
				{
					TH_ERR("[mongoc] cannot run empty query");
					return Response();
				}

				Property Type;
				if (!Command.GetProperty("type", &Type) || Type.Mod != Type::String)
				{
					Command.Release();
					TH_ERR("[mongoc] cannot run query without query @type");
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
						Command.Release();
						TH_ERR("[mongoc] cannot run aggregation query in transaction");
						return Response();
					}

					Property Pipeline;
					if (!Command.GetProperty("pipeline", &Pipeline) || Pipeline.Mod != Type::Array)
					{
						Command.Release();
						TH_ERR("[mongoc] cannot run aggregation query without @pipeline");
						return Response();
					}

					Command.Release();
					return Aggregate(Flags, Pipeline.GetOwnership(), Options.GetOwnership()).Then<Response>([](Cursor&& Result)
					{
						return Response(Result);
					});
				}
				else if (Type.String == "find")
				{
					Property Options = Command["options"];
					if (Session != nullptr && !Session->Put(&Options.Source))
					{
						Command.Release();
						TH_ERR("[mongoc] cannot run find-one query in transaction");
						return Response();
					}

					Property Match;
					if (!Command.GetProperty("match", &Match) || Match.Mod != Type::Schema)
					{
						Command.Release();
						TH_ERR("[mongoc] cannot run find-one query without @match");
						return Response();
					}

					Command.Release();
					return FindOne(Match.GetOwnership(), Options.GetOwnership()).Then<Response>([](Cursor&& Result)
					{
						return Response(Result);
					});
				}
				else if (Type.String == "find-many")
				{
					Property Options = Command["options"];
					if (Session != nullptr && !Session->Put(&Options.Source))
					{
						Command.Release();
						TH_ERR("[mongoc] cannot run find-many query in transaction");
						return Response();
					}

					Property Match;
					if (!Command.GetProperty("match", &Match) || Match.Mod != Type::Schema)
					{
						Command.Release();
						TH_ERR("[mongoc] cannot run find-many query without @match");
						return Response();
					}

					Command.Release();
					return FindMany(Match.GetOwnership(), Options.GetOwnership()).Then<Response>([](Cursor&& Result)
					{
						return Response(Result);
					});
				}
				else if (Type.String == "update")
				{
					Property Options = Command["options"];
					if (Session != nullptr && !Session->Put(&Options.Source))
					{
						Command.Release();
						TH_ERR("[mongoc] cannot run update-one query in transaction");
						return Response();
					}

					Property Match;
					if (!Command.GetProperty("match", &Match) || Match.Mod != Type::Schema)
					{
						Command.Release();
						TH_ERR("[mongoc] cannot run update-one query without @match");
						return Response();
					}

					Property Update;
					if (!Command.GetProperty("update", &Update) || Update.Mod != Type::Schema)
					{
						Command.Release();
						TH_ERR("[mongoc] cannot run update-one query without @update");
						return Response();
					}

					Command.Release();
					return UpdateOne(Match.GetOwnership(), Update.GetOwnership(), Options.GetOwnership()).Then<Response>([](Schema&& Result)
					{
						return Response(Result);
					});
				}
				else if (Type.String == "update-many")
				{
					Property Options = Command["options"];
					if (Session != nullptr && !Session->Put(&Options.Source))
					{
						Command.Release();
						TH_ERR("[mongoc] cannot run update-many query in transaction");
						return Response();
					}

					Property Match;
					if (!Command.GetProperty("match", &Match) || Match.Mod != Type::Schema)
					{
						Command.Release();
						TH_ERR("[mongoc] cannot run update-many query without @match");
						return Response();
					}

					Property Update;
					if (!Command.GetProperty("update", &Update) || Update.Mod != Type::Schema)
					{
						Command.Release();
						TH_ERR("[mongoc] cannot run update-many query without @update");
						return Response();
					}

					Command.Release();
					return UpdateMany(Match.GetOwnership(), Update.GetOwnership(), Options.GetOwnership()).Then<Response>([](Schema&& Result)
					{
						return Response(Result);
					});
				}
				else if (Type.String == "insert")
				{
					Property Options = Command["options"];
					if (Session != nullptr && !Session->Put(&Options.Source))
					{
						Command.Release();
						TH_ERR("[mongoc] cannot run insert-one query in transaction");
						return Response();
					}

					Property Value;
					if (!Command.GetProperty("value", &Value) || Value.Mod != Type::Schema)
					{
						Command.Release();
						TH_ERR("[mongoc] cannot run insert-one query without @value");
						return Response();
					}

					Command.Release();
					return InsertOne(Value.GetOwnership(), Options.GetOwnership()).Then<Response>([](Schema&& Result)
					{
						return Response(Result);
					});
				}
				else if (Type.String == "insert-many")
				{
					Property Options = Command["options"];
					if (Session != nullptr && !Session->Put(&Options.Source))
					{
						Command.Release();
						TH_ERR("[mongoc] cannot run insert-many query in transaction");
						return Response();
					}

					Property Values;
					if (!Command.GetProperty("values", &Values) || Values.Mod != Type::Array)
					{
						Command.Release();
						TH_ERR("[mongoc] cannot run insert-many query without @values");
						return Response();
					}

					std::vector<Schema> Data;
					Values.Get().Loop([&Data](Property* Value)
					{
						Data.push_back(Value->GetOwnership());
						return true;
					});

					Command.Release();
					return InsertMany(Data, Options.GetOwnership()).Then<Response>([](Schema&& Result)
					{
						return Response(Result);
					});
				}
				else if (Type.String == "find-update")
				{
					Property Match;
					if (!Command.GetProperty("match", &Match) || Match.Mod != Type::Schema)
					{
						Command.Release();
						TH_ERR("[mongoc] cannot run find-and-modify query without @match");
						return Response();
					}

					Property Sort = Command["sort"];
					Property Update = Command["update"];
					Property Fields = Command["fields"];
					Property Remove = Command["remove"];
					Property Upsert = Command["upsert"];
					Property New = Command["new"];

					Command.Release();
					return FindAndModify(Match.GetOwnership(), Sort.GetOwnership(), Update.GetOwnership(), Fields.GetOwnership(), Remove.Boolean, Upsert.Boolean, New.Boolean).Then<Response>([](Schema&& Result)
					{
						return Response(Result);
					});
				}
				else if (Type.String == "replace")
				{
					Property Options = Command["options"];
					if (Session != nullptr && !Session->Put(&Options.Source))
					{
						Command.Release();
						TH_ERR("[mongoc] cannot run replace-one query in transaction");
						return Response();
					}

					Property Match;
					if (!Command.GetProperty("match", &Match) || Match.Mod != Type::Schema)
					{
						Command.Release();
						TH_ERR("[mongoc] cannot run replace-one query without @match");
						return Response();
					}

					Property Value;
					if (!Command.GetProperty("value", &Value) || Value.Mod != Type::Schema)
					{
						Command.Release();
						TH_ERR("[mongoc] cannot run replace-one query without @value");
						return Response();
					}

					Command.Release();
					return ReplaceOne(Match.GetOwnership(), Value.GetOwnership(), Options.GetOwnership()).Then<Response>([](Schema&& Result)
					{
						return Response(Result);
					});
				}
				else if (Type.String == "remove")
				{
					Property Options = Command["options"];
					if (Session != nullptr && !Session->Put(&Options.Source))
					{
						Command.Release();
						TH_ERR("[mongoc] cannot run remove-one query in transaction");
						return Response();
					}

					Property Match;
					if (!Command.GetProperty("match", &Match) || Match.Mod != Type::Schema)
					{
						Command.Release();
						TH_ERR("[mongoc] cannot run remove-one query without @value");
						return Response();
					}

					Command.Release();
					return RemoveOne(Match.GetOwnership(), Options.GetOwnership()).Then<Response>([](Schema&& Result)
					{
						return Response(Result);
					});
				}
				else if (Type.String == "remove-many")
				{
					Property Options = Command["options"];
					if (Session != nullptr && !Session->Put(&Options.Source))
					{
						Command.Release();
						TH_ERR("[mongoc] cannot run remove-many query in transaction");
						return Response();
					}

					Property Match;
					if (!Command.GetProperty("match", &Match) || Match.Mod != Type::Schema)
					{
						Command.Release();
						TH_ERR("[mongoc] cannot run remove-many query without @value");
						return Response();
					}

					Command.Release();
					return RemoveMany(Match.GetOwnership(), Options.GetOwnership()).Then<Response>([](Schema&& Result)
					{
						return Response(Result);
					});
				}

				Command.Release();
				TH_ERR("[mongoc] cannot find query of type \"%s\"", Type.String.c_str());
				return Response();
#else
				return Response();
#endif
			}
			const char* Collection::GetName() const
			{
#ifdef TH_HAS_MONGOC
				TH_ASSERT(Base != nullptr, nullptr, "context should be set");
				return mongoc_collection_get_name(Base);
#else
				return nullptr;
#endif
			}
			Stream Collection::CreateStream(const Schema& Options)
			{
#ifdef TH_HAS_MONGOC
				if (!Base)
				{
					Options.Release();
					return Stream(nullptr, nullptr, nullptr);
				}

				TStream* Operation = mongoc_collection_create_bulk_operation_with_opts(Base, Options.Get());
				return Stream(Base, Operation, Options);
#else
				return Stream(nullptr, nullptr, nullptr);
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
				return Core::Async<bool>::Execute([Context](Core::Async<bool>& Future)
				{
					Future = MongoExecuteQuery(&mongoc_database_remove_all_users, Context);
				});
#else
				return false;
#endif
			}
			Core::Async<bool> Database::RemoveUser(const std::string& Name)
			{
#ifdef TH_HAS_MONGOC
				auto* Context = Base;
				return Core::Async<bool>::Execute([Context, Name](Core::Async<bool>& Future)
				{
					Future = MongoExecuteQuery(&mongoc_database_remove_user, Context, Name.c_str());
				});
#else
				return false;
#endif
			}
			Core::Async<bool> Database::Remove()
			{
#ifdef TH_HAS_MONGOC
				auto* Context = Base;
				return Core::Async<bool>::Execute([Context](Core::Async<bool>& Future)
				{
					Future = MongoExecuteQuery(&mongoc_database_drop, Context);
				});
#else
				return false;
#endif
			}
			Core::Async<bool> Database::RemoveWithOptions(const Schema& Options)
			{
#ifdef TH_HAS_MONGOC
				if (!Options.Get())
					return Remove();

				auto* Context = Base;
				return Core::Async<bool>::Execute([Context, Options](Core::Async<bool>& Future)
				{
					bool Subresult = MongoExecuteQuery(&mongoc_database_drop_with_opts, Context, Options.Get());
					Options.Release();

					Future = Subresult;
				});
#else
				return false;
#endif
			}
			Core::Async<bool> Database::AddUser(const std::string& Username, const std::string& Password, const Schema& Roles, const Schema& Custom)
			{
#ifdef TH_HAS_MONGOC
				auto* Context = Base;
				return Core::Async<bool>::Execute([Context, Username, Password, Roles, Custom](Core::Async<bool>& Future)
				{
					bool Subresult = MongoExecuteQuery(&mongoc_database_add_user, Context, Username.c_str(), Password.c_str(), Roles.Get(), Custom.Get());
					Roles.Release();
					Custom.Release();

					Future = Subresult;
				});
#else
				return false;
#endif
			}
			Core::Async<bool> Database::HasCollection(const std::string& Name) const
			{
#ifdef TH_HAS_MONGOC
				auto* Context = Base;
				return Core::Async<bool>::Execute([Context, Name](Core::Async<bool>& Future)
				{
					bson_error_t Error;
					memset(&Error, 0, sizeof(bson_error_t));
					bool Subresult = mongoc_database_has_collection(Context, Name.c_str(), &Error);
					if (!Subresult && Error.code != 0)
						TH_ERR("[mongoc:%i] %s", (int)Error.code, Error.message);

					Future = Subresult;
				});
#else
				return false;
#endif
			}
			Core::Async<Cursor> Database::FindCollections(const Schema& Options) const
			{
#ifdef TH_HAS_MONGOC
				auto* Context = Base;
				return Core::Async<Cursor>::Execute([Context, Options](Core::Async<Cursor>& Future)
				{
					Cursor Subresult = MongoExecuteCursor(&mongoc_database_find_collections_with_opts, Context, Options.Get());
					Options.Release();

					Future = Subresult;
				});
#else
				return Cursor(nullptr);
#endif
			}
			Core::Async<Collection> Database::CreateCollection(const std::string& Name, const Schema& Options)
			{
#ifdef TH_HAS_MONGOC
				if (!Base)
				{
					Options.Release();
					return Collection(nullptr);
				}

				auto* Context = Base;
				return Core::Async<Collection>::Execute([Context, Name, Options](Core::Async<Collection>& Future)
				{
					bson_error_t Error;
					memset(&Error, 0, sizeof(bson_error_t));

					TCollection* Collection = mongoc_database_create_collection(Context, Name.c_str(), Options.Get(), &Error);
					Options.Release();

					if (Collection == nullptr)
						TH_ERR("[mongoc] %s", Error.message);

					Future = Collection;
				});
#else
				return Collection(nullptr);
#endif
			}
			std::vector<std::string> Database::GetCollectionNames(const Schema& Options) const
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
					TH_ERR("[mongoc] %s", Error.message);
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
				TH_ASSERT(Base != nullptr, nullptr, "context should be set");
				return mongoc_database_get_name(Base);
#else
				return nullptr;
#endif
			}
			Collection Database::GetCollection(const std::string& Name)
			{
#ifdef TH_HAS_MONGOC
				TH_ASSERT(Base != nullptr, nullptr, "context should be set");
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
			Core::Async<bool> Watcher::Next(const Schema& Result) const
			{
#ifdef TH_HAS_MONGOC
				if (!Base || !Result.Get())
					return false;

				auto* Context = Base;
				return Core::Async<bool>::Execute([Context, Result](Core::Async<bool>& Future)
				{
					TDocument* Ptr = Result.Get();
					Future = mongoc_change_stream_next(Context, (const TDocument**)&Ptr);
				});
#else
				return false;
#endif
			}
			Core::Async<bool> Watcher::Error(const Schema& Result) const
			{
#ifdef TH_HAS_MONGOC
				if (!Base || !Result.Get())
					return false;

				auto* Context = Base;
				return Core::Async<bool>::Execute([Context, Result](Core::Async<bool>& Future)
				{
					TDocument* Ptr = Result.Get();
					Future = mongoc_change_stream_error_document(Context, nullptr, (const TDocument**)&Ptr);
				});
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
			Watcher Watcher::FromConnection(Connection* Connection, const Schema& Pipeline, const Schema& Options)
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
			Watcher Watcher::FromDatabase(const Database& Database, const Schema& Pipeline, const Schema& Options)
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
			Watcher Watcher::FromCollection(const Collection& Collection, const Schema& Pipeline, const Schema& Options)
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
			bool Transaction::Push(Schema& QueryOptions) const
			{
#ifdef TH_HAS_MONGOC
				if (!QueryOptions.Get())
					QueryOptions = bson_new();

				bson_error_t Error;
				bool Result = mongoc_client_session_append(Base, (bson_t*)QueryOptions.Get(), &Error);
				if (!Result && Error.code != 0)
					TH_ERR("[mongoc:%i] %s", (int)Error.code, Error.message);

				return Result;
#else
				return false;
#endif
			}
			bool Transaction::Put(TDocument** QueryOptions) const
			{
#ifdef TH_HAS_MONGOC
				TH_ASSERT(QueryOptions != nullptr, false, "query options should be set");
				if (!*QueryOptions)
					*QueryOptions = bson_new();

				bson_error_t Error;
				bool Result = mongoc_client_session_append(Base, (bson_t*)*QueryOptions, &Error);
				if (!Result && Error.code != 0)
					TH_ERR("[mongoc:%i] %s", (int)Error.code, Error.message);

				return Result;
#else
				return false;
#endif
			}
			Core::Async<bool> Transaction::Start()
			{
#ifdef TH_HAS_MONGOC
				auto* Context = Base;
				return Core::Async<bool>::Execute([Context](Core::Async<bool>& Future)
				{
					Future = MongoExecuteQuery(&mongoc_client_session_start_transaction, Context, nullptr);
				});
#else
				return false;
#endif
			}
			Core::Async<bool> Transaction::Abort()
			{
#ifdef TH_HAS_MONGOC
				auto* Context = Base;
				return Core::Async<bool>::Execute([Context](Core::Async<bool>& Future)
				{
					Future = MongoExecuteQuery(&mongoc_client_session_abort_transaction, Context);
				});
#else
				return false;
#endif
			}
			Core::Async<Schema> Transaction::RemoveMany(const Collection& fBase, const Schema& Match, const Schema& fOptions)
			{
#ifdef TH_HAS_MONGOC
				Schema Options = fOptions;
				if (!Push(Options))
				{
					Match.Release();
					Options.Release();
					return Schema(nullptr);
				}

				return Collection(fBase.Get()).RemoveMany(Match, Options);
#else
				return Schema(nullptr);
#endif
			}
			Core::Async<Schema> Transaction::RemoveOne(const Collection& fBase, const Schema& Match, const Schema& fOptions)
			{
#ifdef TH_HAS_MONGOC
				Schema Options = fOptions;
				if (!Push(Options))
				{
					Match.Release();
					Options.Release();
					return Schema(nullptr);
				}

				return Collection(fBase.Get()).RemoveOne(Match, Options);
#else
				return Schema(nullptr);
#endif
			}
			Core::Async<Schema> Transaction::ReplaceOne(const Collection& fBase, const Schema& Match, const Schema& Replacement, const Schema& fOptions)
			{
#ifdef TH_HAS_MONGOC
				Schema Options = fOptions;
				if (!Push(Options))
				{
					Match.Release();
					Replacement.Release();
					Options.Release();
					return Schema(nullptr);
				}

				return Collection(fBase.Get()).ReplaceOne(Match, Replacement, Options);
#else
				return Schema(nullptr);
#endif
			}
			Core::Async<Schema> Transaction::InsertMany(const Collection& fBase, std::vector<Schema>& List, const Schema& fOptions)
			{
#ifdef TH_HAS_MONGOC
				Schema Options = fOptions;
				if (!Push(Options))
				{
					Options.Release();
					for (auto& Item : List)
						Item.Release();

					return Schema(nullptr);
				}

				return Collection(fBase.Get()).InsertMany(List, Options);
#else
				return Schema(nullptr);
#endif
			}
			Core::Async<Schema> Transaction::InsertOne(const Collection& fBase, const Schema& Result, const Schema& fOptions)
			{
#ifdef TH_HAS_MONGOC
				Schema Options = fOptions;
				if (!Push(Options))
				{
					Result.Release();
					Options.Release();
					return Schema(nullptr);
				}

				return Collection(fBase.Get()).InsertOne(Result, Options);
#else
				return Schema(nullptr);
#endif
			}
			Core::Async<Schema> Transaction::UpdateMany(const Collection& fBase, const Schema& Match, const Schema& Update, const Schema& fOptions)
			{
#ifdef TH_HAS_MONGOC
				Schema Options = fOptions;
				if (!Push(Options))
				{
					Match.Release();
					Update.Release();
					Options.Release();
					return Schema(nullptr);
				}

				return Collection(fBase.Get()).UpdateMany(Match, Update, Options);
#else
				return Schema(nullptr);
#endif
			}
			Core::Async<Schema> Transaction::UpdateOne(const Collection& fBase, const Schema& Match, const Schema& Update, const Schema& fOptions)
			{
#ifdef TH_HAS_MONGOC
				Schema Options = fOptions;
				if (!Push(Options))
				{
					Match.Release();
					Update.Release();
					Options.Release();
					return Schema(nullptr);
				}

				return Collection(fBase.Get()).UpdateOne(Match, Update, Options);
#else
				return Schema(nullptr);
#endif
			}
			Core::Async<Cursor> Transaction::FindMany(const Collection& fBase, const Schema& Match, const Schema& fOptions) const
			{
#ifdef TH_HAS_MONGOC
				Schema Options = fOptions;
				if (!Push(Options))
				{
					Match.Release();
					Options.Release();
					return Cursor(nullptr);
				}

				return Collection(fBase.Get()).FindMany(Match, Options);
#else
				return Cursor(nullptr);
#endif
			}
			Core::Async<Cursor> Transaction::FindOne(const Collection& fBase, const Schema& Match, const Schema& fOptions) const
			{
#ifdef TH_HAS_MONGOC
				Schema Options = fOptions;
				if (!Push(Options))
				{
					Match.Release();
					Options.Release();
					return Cursor(nullptr);
				}

				return Collection(fBase.Get()).FindOne(Match, Options);
#else
				return Cursor(nullptr);
#endif
			}
			Core::Async<Cursor> Transaction::Aggregate(const Collection& fBase, QueryFlags Flags, const Schema& Pipeline, const Schema& fOptions) const
			{
#ifdef TH_HAS_MONGOC
				Schema Options = fOptions;
				if (!Push(Options))
				{
					Pipeline.Release();
					Options.Release();
					return Cursor(nullptr);
				}

				return Collection(fBase.Get()).Aggregate(Flags, Pipeline, Options);
#else
				return Cursor(nullptr);
#endif
			}
			Core::Async<Response> Transaction::TemplateQuery(const Collection& fBase, const std::string& Name, Core::SchemaArgs* Map, bool Once)
			{
				return Query(fBase, Driver::GetQuery(Name, Map, Once));
			}
			Core::Async<Response> Transaction::Query(const Collection& fBase, const Schema& Command)
			{
#ifdef TH_HAS_MONGOC
				return Collection(fBase.Get()).Query(Command, this);
#else
				return Response();
#endif
			}
			Core::Async<TransactionState> Transaction::Commit()
			{
#ifdef TH_HAS_MONGOC
				auto* Context = Base;
				return Core::Async<TransactionState>::Execute([Context](Core::Async<TransactionState>& Future)
				{
					TDocument Subresult;
					if (MongoExecuteQuery(&mongoc_client_session_commit_transaction, Context, &Subresult))
						Future = TransactionState::OK;
					else if (mongoc_error_has_label(&Subresult, "TransientTransactionError"))
						Future = TransactionState::Retry;
					else if (mongoc_error_has_label(&Subresult, "UnknownTransactionCommitResult"))
						Future = TransactionState::Retry_Commit;
					else
						Future = TransactionState::Fatal;
					bson_free(&Subresult);
				});
#else
				return TransactionState::Fatal;
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

			Connection::Connection() : Connected(false), Session(nullptr), Base(nullptr), Master(nullptr)
			{
				Driver::Create();
			}
			Connection::~Connection()
			{
#ifdef TH_HAS_MONGOC
				TTransaction* Context = Session.Get();
				if (Context != nullptr)
					mongoc_client_session_destroy(Context);
#endif
				if (Connected && Base != nullptr)
					Disconnect();
				Driver::Release();
			}
			Core::Async<bool> Connection::Connect(const std::string& Address)
			{
#ifdef TH_HAS_MONGOC
				TH_ASSERT(Master != nullptr, false, "connection should be created outside of cluster");
				if (Connected)
				{
					return Disconnect().Then<Core::Async<bool>>([this, Address](bool)
					{
						return this->Connect(Address);
					});
				}

				return Core::Async<bool>::Execute([this, Address](Core::Async<bool>& Future)
				{
					TH_PPUSH(TH_PERF_MAX);

					bson_error_t Error;
					memset(&Error, 0, sizeof(bson_error_t));

					TAddress* URI = mongoc_uri_new_with_error(Address.c_str(), &Error);
					if (!URI)
					{
						Future = false;
						TH_ERR("[urierr] %s", Error.message);
						TH_PPOP();
						return;
					}

					Base = mongoc_client_new_from_uri(URI);
					if (!Base)
					{
						Future = false;
						TH_ERR("[mongoc] couldn't connect to requested URI");
						TH_PPOP();
						return;
					}

					Driver::AttachQueryLog(Base);
					Connected = true;
					Future = true;
					TH_PPOP();
				});
#else
				return false;
#endif
			}
			Core::Async<bool> Connection::Connect(Address* URL)
			{
#ifdef TH_HAS_MONGOC
				TH_ASSERT(Master != nullptr, false, "connection should be created outside of cluster");
				TH_ASSERT(URL && URL->Get(), false, "url should be valid");

				if (Connected)
				{
					return Disconnect().Then<Core::Async<bool>>([this, URL](bool)
					{
						return this->Connect(URL);
					});
				}

				TAddress* URI = URL->Get();
				return Core::Async<bool>::Execute([this, URI](Core::Async<bool>& Future)
				{
					TH_PPUSH(TH_PERF_MAX);
					Base = mongoc_client_new_from_uri(URI);
					if (Base != nullptr)
					{
						Driver::AttachQueryLog(Base);
						Future = true;
						Connected = true;
					}
					else
					{
						Future = false;
						TH_ERR("[mongoc] couldn't connect to requested URI");
					}
					TH_PPOP();
				});
#else
				return false;
#endif
			}
			Core::Async<bool> Connection::Disconnect()
			{
#ifdef TH_HAS_MONGOC
				TH_ASSERT(Connected && Base, false, "connection should be established");
				return Core::Async<bool>::Execute([this](Core::Async<bool>& Future)
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

					Future = true;
				});
#else
				return false;
#endif
			}
			Core::Async<bool> Connection::MakeTransaction(const std::function<Core::Async<bool>(Transaction&)>& Callback)
			{
#ifdef TH_HAS_MONGOC
				TH_ASSERT(Callback, false, "callback should not be empty");
				return Core::Coasync<bool>([this, Callback]()
				{
					Transaction Context = GetSession();
					if (!Context)
						return false;

					while (true)
					{
						if (!TH_AWAIT(Context.Start()))
							return false;

						if (!TH_AWAIT(Callback(Context)))
							break;

						while (true)
						{
							TransactionState State = TH_AWAIT(Context.Commit());
							if (State == TransactionState::OK || State == TransactionState::Fatal)
								return State == TransactionState::OK;

							if (State == TransactionState::Retry_Commit)
							{
								TH_WARN("[mongoc] retrying transaction commit");
								continue;
							}

							if (State == TransactionState::Retry)
							{
								TH_WARN("[mongoc] retrying full transaction");
								break;
							}
						}
					}

					TH_AWAIT(Context.Abort());
					return false;
				});
#else
				return false;
#endif
			}
			Core::Async<bool> Connection::MakeCotransaction(const std::function<bool(Transaction&)>& Callback)
			{
#ifdef TH_HAS_MONGOC
				TH_ASSERT(Callback, false, "callback should not be empty");
				return Core::Coasync<bool>([this, Callback]()
				{
					Transaction Context = GetSession();
					if (!Context)
						return false;

					while (true)
					{
						if (!TH_AWAIT(Context.Start()))
							return false;

						if (!Callback(Context))
							break;

						while (true)
						{
							TransactionState State = TH_AWAIT(Context.Commit());
							if (State == TransactionState::OK || State == TransactionState::Fatal)
								return State == TransactionState::OK;

							if (State == TransactionState::Retry_Commit)
							{
								TH_WARN("[mongoc] retrying transaction commit");
								continue;
							}

							if (State == TransactionState::Retry)
							{
								TH_WARN("[mongoc] retrying full transaction");
								break;
							}
						}
					}

					TH_AWAIT(Context.Abort());
					return false;
				});
#else
				return false;
#endif
			}
			Core::Async<Cursor> Connection::FindDatabases(const Schema& Options) const
			{
#ifdef TH_HAS_MONGOC
				return Core::Async<Cursor>::Execute([this, Options](Core::Async<Cursor>& Future)
				{
					TCursor* Subresult = mongoc_client_find_databases_with_opts(Base, Options.Get());
					Options.Release();

					Future = Subresult;
				});
#else
				return Cursor(nullptr);
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
					TH_ERR("[mongoc] command fail: %s", Error.message);
					return false;
				}

				mongoc_server_description_destroy(Server);
				return true;
#else
				return false;
#endif
			}
			Transaction Connection::GetSession()
			{
#ifdef TH_HAS_MONGOC
				bson_error_t Error;
				if (!Session.Get())
				{
					Session = mongoc_client_start_session(Base, nullptr, &Error);
					if (!Session.Get())
					{
						TH_ERR("[mongoc] couldn't create transaction\n\t%s", Error.message);
						return nullptr;
					}
				}

				return Session;
#else
				return nullptr;
#endif
			}
			Database Connection::GetDatabase(const std::string& Name) const
			{
#ifdef TH_HAS_MONGOC
				TH_ASSERT(Base != nullptr, nullptr, "context should be set");
				return mongoc_client_get_database(Base, Name.c_str());
#else
				return nullptr;
#endif
			}
			Database Connection::GetDefaultDatabase() const
			{
#ifdef TH_HAS_MONGOC
				TH_ASSERT(Base != nullptr, nullptr, "context should be set");
				return mongoc_client_get_default_database(Base);
#else
				return nullptr;
#endif
			}
			Collection Connection::GetCollection(const char* DatabaseName, const char* Name) const
			{
#ifdef TH_HAS_MONGOC
				TH_ASSERT(Base != nullptr, nullptr, "context should be set");
				return mongoc_client_get_collection(Base, DatabaseName, Name);
#else
				return nullptr;
#endif
			}
			Address Connection::GetAddress() const
			{
#ifdef TH_HAS_MONGOC
				TH_ASSERT(Base != nullptr, nullptr, "context should be set");
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
			std::vector<std::string> Connection::GetDatabaseNames(const Schema& Options) const
			{
#ifdef TH_HAS_MONGOC
				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				TH_ASSERT(Base != nullptr, std::vector<std::string>(), "context should be set");
				char** Names = mongoc_client_get_database_names_with_opts(Base, Options.Get(), &Error);
				Options.Release();

				if (Names == nullptr)
				{
					TH_ERR("[mongoc] %s", Error.message);
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
			Core::Async<bool> Cluster::Connect(const std::string& URI)
			{
#ifdef TH_HAS_MONGOC
				if (Connected)
				{
					return Disconnect().Then<Core::Async<bool>>([this, URI](bool)
					{
						return this->Connect(URI);
					});
				}

				return Core::Async<bool>::Execute([this, URI](Core::Async<bool>& Future)
				{
					TH_PPUSH(TH_PERF_MAX);
					bson_error_t Error;
					memset(&Error, 0, sizeof(bson_error_t));

					SrcAddress = mongoc_uri_new_with_error(URI.c_str(), &Error);
					if (!SrcAddress.Get())
					{
						Future = false;
						TH_ERR("[urierr] %s", Error.message);
						TH_PPOP();
						return;
					}

					Pool = mongoc_client_pool_new(SrcAddress.Get());
					if (!Pool)
					{
						Future = false;
						TH_ERR("[mongoc] couldn't connect to requested URI");
						TH_PPOP();
						return;
					}

					Driver::AttachQueryLog(Pool);
					Connected = true;
					Future = true;
					TH_PPOP();
				});
#else
				return false;
#endif
			}
			Core::Async<bool> Cluster::Connect(Address* URI)
			{
#ifdef TH_HAS_MONGOC
				TH_ASSERT(URI && URI->Get(), false, "url should be set");
				if (Connected)
				{
					return Disconnect().Then<Core::Async<bool>>([this, URI](bool)
					{
						return this->Connect(URI);
					});
				}

				TAddress* Context = URI->Get();
				*URI = nullptr;

				return Core::Async<bool>::Execute([this, Context](Core::Async<bool>& Future)
				{
					TH_PPUSH(TH_PERF_MAX);

					SrcAddress = Context;
					Pool = mongoc_client_pool_new(SrcAddress.Get());
					if (Pool != nullptr)
					{
						Driver::AttachQueryLog(Pool);
						Future = true;
						Connected = true;
					}
					else
					{
						Future = false;
						TH_ERR("[mongoc] couldn't connect to requested URI");
					}

					TH_PPOP();
				});
#else
				return false;
#endif
			}
			Core::Async<bool> Cluster::Disconnect()
			{
#ifdef TH_HAS_MONGOC
				TH_ASSERT(Connected && Pool, false, "connection should be established");
				return Core::Async<bool>::Execute([this](Core::Async<bool>& Future)
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
				return false;
#endif
			}
			void Cluster::SetProfile(const char* Name)
			{
#ifdef TH_HAS_MONGOC
				TH_ASSERT_V(Pool != nullptr, "connection should be established");
				TH_ASSERT_V(Name != nullptr, "name should be set");
				mongoc_client_pool_set_appname(Pool, Name);
#endif
			}
			void Cluster::Push(Connection** Client)
			{
#ifdef TH_HAS_MONGOC
				TH_ASSERT_V(Client && *Client, "client should be set");
				mongoc_client_pool_push(Pool, (*Client)->Base);
				(*Client)->Base = nullptr;
				(*Client)->Connected = false;
				(*Client)->Master = nullptr;

				TH_RELEASE(*Client);
				*Client = nullptr;
#endif
			}
			Connection* Cluster::Pop()
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
			TConnectionPool* Cluster::Get() const
			{
				return Pool;
			}
			Address Cluster::GetAddress() const
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
								TH_ERR("[mongoc] [%s] %s", Domain, Message);
								break;
							case MONGOC_LOG_LEVEL_WARNING:
								TH_WARN("[mongoc] [%s] %s", Domain, Message);
								break;
							case MONGOC_LOG_LEVEL_INFO:
								TH_INFO("[mongoc] [%s] %s", Domain, Message);
								break;
							case MONGOC_LOG_LEVEL_CRITICAL:
								TH_ERR("[mongocerr] [%s] %s", Domain, Message);
								break;
							case MONGOC_LOG_LEVEL_MESSAGE:
								TH_DEBUG("[mongoc] [%s] %s", Domain, Message);
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

					if (APM != nullptr)
					{
						mongoc_apm_callbacks_destroy((mongoc_apm_callbacks_t*)APM);
						APM = nullptr;
					}

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
			void Driver::SetQueryLog(const OnQueryLog& Callback)
			{
				Logger = Callback;
				if (!Logger || APM)
					return;
#ifdef TH_HAS_MONGOC
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
#ifdef TH_HAS_MONGOC
				TH_ASSERT_V(Connection != nullptr, "connection should be set");
				mongoc_client_set_apm_callbacks(Connection, (mongoc_apm_callbacks_t*)APM, nullptr);
#endif
			}
			void Driver::AttachQueryLog(TConnectionPool* Connection)
			{
#ifdef TH_HAS_MONGOC
				TH_ASSERT_V(Connection != nullptr, "connection pool should be set");
				mongoc_client_pool_set_apm_callbacks(Connection, (mongoc_apm_callbacks_t*)APM, nullptr);
#endif
			}
			bool Driver::AddQuery(const std::string& Name, const char* Buffer, size_t Size)
			{
				TH_ASSERT(Queries && Safe, false, "driver should be initialized");
				TH_ASSERT(!Name.empty(), false, "name should not be empty");
				TH_ASSERT(Buffer, false, "buffer should be set");

				if (!Size)
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
					Result.Cache = Schema::FromJSON(Result.Request);

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
				TH_ASSERT(Queries && Safe, false, "driver should be initialized");
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
			Schema Driver::GetQuery(const std::string& Name, Core::SchemaArgs* Map, bool Once)
			{
				TH_ASSERT(Queries && Safe, nullptr, "driver should be initialized");
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

					TH_ERR("[mongoc] template query %s does not exist", Name.c_str());
					return nullptr;
				}

				if (It->second.Cache.Get() != nullptr)
				{
					Schema Result = It->second.Cache.Copy();
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
					Schema Result = Schema::FromJSON(It->second.Request);
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
					{
						TH_ERR("[mongoc] template query %s\n\texpects parameter: %s", Name.c_str(), Word.Key.c_str());
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
						TH_RELEASE(Item.second);
					Map->clear();
				}

				Schema Data = Schema::FromJSON(Origin.Request);
				if (!Data.Get())
					TH_ERR("[mongoc] could not construct query: \"%s\"", Name.c_str());

				return Data;
			}
			std::vector<std::string> Driver::GetQueries()
			{
				std::vector<std::string> Result;
				TH_ASSERT(Queries && Safe, Result, "driver should be initialized");

				Safe->lock();
				Result.reserve(Queries->size());
				for (auto& Item : *Queries)
					Result.push_back(Item.first);
				Safe->unlock();

				return Result;
			}
			std::string Driver::GetJSON(Core::Schema* Source, bool Escape)
			{
				TH_ASSERT(Source != nullptr, std::string(), "source should be set");
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
			std::unordered_map<std::string, Driver::Sequence>* Driver::Queries = nullptr;
			std::mutex* Driver::Safe = nullptr;
			std::atomic<int> Driver::State(0);
			OnQueryLog Driver::Logger = nullptr;
			void* Driver::APM = nullptr;
		}
	}
}
