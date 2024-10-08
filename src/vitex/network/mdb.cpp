#include "mdb.h"
#ifdef VI_MONGOC
#include <mongoc/mongoc.h>
#endif
#define MongoExecuteQuery(Function, Context, ...) ExecuteQuery(#Function, Function, Context, ##__VA_ARGS__)
#define MongoExecuteCursor(Function, Context, ...) ExecuteCursor(#Function, Function, Context, ##__VA_ARGS__)

namespace Vitex
{
	namespace Network
	{
		namespace MDB
		{
#ifdef VI_MONGOC
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
			ExpectsDB<void> ExecuteQuery(const char* Name, R&& Function, T* Base, Args&&... Data)
			{
				VI_ASSERT(Base != nullptr, "context should be set");
				VI_MEASURE(Core::Timings::Intensive);
				VI_DEBUG("[mongoc] execute query schema on 0x%" PRIXPTR ": %s", (uintptr_t)Base, Name + 1);

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				auto Time = Core::Schedule::GetClock();
				if (!Function(Base, Data..., &Error))
					return DatabaseException(Error.code, Error.message);

				VI_DEBUG("[mongoc] OK execute on 0x%" PRIXPTR " (%" PRIu64 " ms)", (uintptr_t)Base, (uint64_t)((Core::Schedule::GetClock() - Time).count() / 1000));
				(void)Time;
				return Core::Expectation::Met;
			}
			template <typename R, typename T, typename... Args>
			ExpectsDB<Cursor> ExecuteCursor(const char* Name, R&& Function, T* Base, Args&&... Data)
			{
				VI_ASSERT(Base != nullptr, "context should be set");
				VI_MEASURE(Core::Timings::Intensive);
				VI_DEBUG("[mongoc] execute query cursor on 0x%" PRIXPTR ": %s", (uintptr_t)Base, Name + 1);

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				auto Time = Core::Schedule::GetClock();
				TCursor* Result = Function(Base, Data...);
				if (!Result || mongoc_cursor_error(Result, &Error))
					return DatabaseException(Error.code, Error.message);

				VI_DEBUG("[mongoc] OK execute on 0x%" PRIXPTR " (%" PRIu64 " ms)", (uintptr_t)Base, (uint64_t)((Core::Schedule::GetClock() - Time).count() / 1000));
				(void)Time;
				return Cursor(Result);
			}
#endif
			Property::Property() noexcept : Source(nullptr), Integer(0), High(0), Low(0), Number(0.0), Boolean(false), Mod(Type::Unknown), IsValid(false)
			{
			}
			Property::Property(Property&& Other) : Name(std::move(Other.Name)), String(std::move(Other.String)), Source(Other.Source), Integer(Other.Integer), High(Other.High), Low(Other.Low), Number(Other.Number), Boolean(Other.Boolean), Mod(Other.Mod), IsValid(Other.IsValid)
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
#ifdef VI_MONGOC
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
			Core::Unique<TDocument> Property::Reset()
			{
				if (!Source)
					return nullptr;

				TDocument* Result = Source;
				Source = nullptr;
				return Result;
			}
			Core::String& Property::ToString()
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
						return String.assign(Core::ToString(Number));
					case Type::Integer:
						return String.assign(Core::ToString(Integer));
					case Type::ObjectId:
						return String.assign(Compute::Codec::Bep45Encode(Core::String((const char*)ObjectId, 12)));
					case Type::Null:
						return String.assign("null");
					case Type::Unknown:
					case Type::Uncastable:
						return String.assign("undefined");
					case Type::Decimal:
					{
#ifdef VI_MONGOC
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
			Core::String Property::ToObjectId()
			{
				return Utils::IdToString(ObjectId);
			}
			Document Property::AsDocument() const
			{
				return Document(Source);
			}
			Property Property::At(const std::string_view& Label) const
			{
				Property Result;
				AsDocument().GetProperty(Label, &Result);
				return Result;
			}
			Property Property::operator [](const std::string_view& Label)
			{
				Property Result;
				AsDocument().GetProperty(Label, &Result);
				return Result;
			}
			Property Property::operator [](const std::string_view& Label) const
			{
				Property Result;
				AsDocument().GetProperty(Label, &Result);
				return Result;
			}

			DatabaseException::DatabaseException(int NewErrorCode, Core::String&& NewMessage) : ErrorCode(NewErrorCode)
			{
				Message = std::move(NewMessage);
				if (ErrorCode == 0)
					return;

				Message += " (error = ";
				Message += Core::ToString(ErrorCode);
				Message += ")";
			}
			const char* DatabaseException::type() const noexcept
			{
				return "mongodb_error";
			}
			int DatabaseException::error_code() const noexcept
			{
				return ErrorCode;
			}

			Document::Document() : Base(nullptr), Store(false)
			{
			}
			Document::Document(TDocument* NewBase) : Base(NewBase), Store(false)
			{
			}
			Document::Document(const Document& Other) : Base(nullptr), Store(false)
			{
#ifdef VI_MONGOC
				if (Other.Base)
					Base = bson_copy(Other.Base);
#endif
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
			Document& Document::operator =(const Document& Other)
			{
#ifdef VI_MONGOC
				if (&Other == this)
					return *this;

				Cleanup();
				Base = Other.Base ? bson_copy(Other.Base) : nullptr;
				Store = false;
				return *this;
#else
				return *this;
#endif
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
#ifdef VI_MONGOC
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
#ifdef VI_MONGOC
				VI_ASSERT(Base != nullptr, "schema should be set");
				VI_ASSERT(Value.Base != nullptr, "other schema should be set");
				bson_concat((bson_t*)Base, (bson_t*)Value.Base);
#endif
			}
			void Document::Loop(const std::function<bool(Property*)>& Callback) const
			{
				VI_ASSERT(Base != nullptr, "schema should be set");
				VI_ASSERT(Callback, "callback should be set");
#ifdef VI_MONGOC
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
			bool Document::SetSchema(const std::string_view& Key, const Document& Value, size_t ArrayId)
			{
#ifdef VI_MONGOC
				VI_ASSERT(Base != nullptr, "schema should be set");
				VI_ASSERT(Value.Base != nullptr, "other schema should be set");

				char Index[16];
				std::string_view ValueKey = Key;
				if (ValueKey.empty())
				{
					const char* IndexKey = Index;
					bson_uint32_to_string((uint32_t)ArrayId, &IndexKey, Index, sizeof(Index));
					ValueKey = IndexKey;
				}

				return bson_append_document((bson_t*)Base, ValueKey.data(), (int)ValueKey.size(), (bson_t*)Value.Base);
#else
				return false;
#endif
			}
			bool Document::SetArray(const std::string_view& Key, const Document& Value, size_t ArrayId)
			{
#ifdef VI_MONGOC
				VI_ASSERT(Base != nullptr, "schema should be set");
				VI_ASSERT(Value.Base != nullptr, "other schema should be set");

				char Index[16];
				std::string_view ValueKey = Key;
				if (ValueKey.empty())
				{
					const char* IndexKey = Index;
					bson_uint32_to_string((uint32_t)ArrayId, &IndexKey, Index, sizeof(Index));
					ValueKey = IndexKey;
				}

				return bson_append_array((bson_t*)Base, ValueKey.data(), (int)ValueKey.size(), (bson_t*)Value.Base);
#else
				return false;
#endif
			}
			bool Document::SetString(const std::string_view& Key, const std::string_view& Value, size_t ArrayId)
			{
#ifdef VI_MONGOC
				VI_ASSERT(Base != nullptr, "schema should be set");

				char Index[16];
				std::string_view ValueKey = Key;
				if (ValueKey.empty())
				{
					const char* IndexKey = Index;
					bson_uint32_to_string((uint32_t)ArrayId, &IndexKey, Index, sizeof(Index));
					ValueKey = IndexKey;
				}

				return bson_append_utf8((bson_t*)Base, ValueKey.data(), (int)ValueKey.size(), Value.data(), (int)Value.size());
#else
				return false;
#endif
			}
			bool Document::SetInteger(const std::string_view& Key, int64_t Value, size_t ArrayId)
			{
#ifdef VI_MONGOC
				VI_ASSERT(Base != nullptr, "schema should be set");

				char Index[16];
				std::string_view ValueKey = Key;
				if (ValueKey.empty())
				{
					const char* IndexKey = Index;
					bson_uint32_to_string((uint32_t)ArrayId, &IndexKey, Index, sizeof(Index));
					ValueKey = IndexKey;
				}

				return bson_append_int64((bson_t*)Base, ValueKey.data(), (int)ValueKey.size(), Value);
#else
				return false;
#endif
			}
			bool Document::SetNumber(const std::string_view& Key, double Value, size_t ArrayId)
			{
#ifdef VI_MONGOC
				VI_ASSERT(Base != nullptr, "schema should be set");

				char Index[16];
				std::string_view ValueKey = Key;
				if (ValueKey.empty())
				{
					const char* IndexKey = Index;
					bson_uint32_to_string((uint32_t)ArrayId, &IndexKey, Index, sizeof(Index));
					ValueKey = IndexKey;
				}

				return bson_append_double((bson_t*)Base, ValueKey.data(), (int)ValueKey.size(), Value);
#else
				return false;
#endif
			}
			bool Document::SetDecimal(const std::string_view& Key, uint64_t High, uint64_t Low, size_t ArrayId)
			{
#ifdef VI_MONGOC
				VI_ASSERT(Base != nullptr, "schema should be set");

				char Index[16];
				std::string_view ValueKey = Key;
				if (ValueKey.empty())
				{
					const char* IndexKey = Index;
					bson_uint32_to_string((uint32_t)ArrayId, &IndexKey, Index, sizeof(Index));
					ValueKey = IndexKey;
				}

				bson_decimal128_t Decimal;
				Decimal.high = (uint64_t)High;
				Decimal.low = (uint64_t)Low;

				return bson_append_decimal128((bson_t*)Base, ValueKey.data(), (int)ValueKey.size(), &Decimal);
#else
				return false;
#endif
			}
			bool Document::SetDecimalString(const std::string_view& Key, const std::string_view& Value, size_t ArrayId)
			{
#ifdef VI_MONGOC
				VI_ASSERT(Base != nullptr, "schema should be set");
				VI_ASSERT(Core::Stringify::IsCString(Value), "value should be set");

				char Index[16];
				std::string_view ValueKey = Key;
				if (ValueKey.empty())
				{
					const char* IndexKey = Index;
					bson_uint32_to_string((uint32_t)ArrayId, &IndexKey, Index, sizeof(Index));
					ValueKey = IndexKey;
				}

				bson_decimal128_t Decimal;
				bson_decimal128_from_string(Value.data(), &Decimal);

				return bson_append_decimal128((bson_t*)Base, ValueKey.data(), (int)ValueKey.size(), &Decimal);
#else
				return false;
#endif
			}
			bool Document::SetDecimalInteger(const std::string_view& Key, int64_t Value, size_t ArrayId)
			{
#ifdef VI_MONGOC
				VI_ASSERT(Base != nullptr, "schema should be set");

				char Index[16];
				std::string_view ValueKey = Key;
				if (ValueKey.empty())
				{
					const char* IndexKey = Index;
					bson_uint32_to_string((uint32_t)ArrayId, &IndexKey, Index, sizeof(Index));
					ValueKey = IndexKey;
				}

				char Data[64];
				snprintf(Data, 64, "%" PRId64 "", Value);

				bson_decimal128_t Decimal;
				bson_decimal128_from_string(Data, &Decimal);

				return bson_append_decimal128((bson_t*)Base, ValueKey.data(), (int)ValueKey.size(), &Decimal);
#else
				return false;
#endif
			}
			bool Document::SetDecimalNumber(const std::string_view& Key, double Value, size_t ArrayId)
			{
#ifdef VI_MONGOC
				VI_ASSERT(Base != nullptr, "schema should be set");

				char Index[16];
				std::string_view ValueKey = Key;
				if (ValueKey.empty())
				{
					const char* IndexKey = Index;
					bson_uint32_to_string((uint32_t)ArrayId, &IndexKey, Index, sizeof(Index));
					ValueKey = IndexKey;
				}

				char Data[64];
				snprintf(Data, 64, "%f", Value);

				for (size_t i = 0; i < 64; i++)
					Data[i] = (Data[i] == ',' ? '.' : Data[i]);

				bson_decimal128_t Decimal;
				bson_decimal128_from_string(Data, &Decimal);

				return bson_append_decimal128((bson_t*)Base, ValueKey.data(), (int)ValueKey.size(), &Decimal);
#else
				return false;
#endif
			}
			bool Document::SetBoolean(const std::string_view& Key, bool Value, size_t ArrayId)
			{
#ifdef VI_MONGOC
				VI_ASSERT(Base != nullptr, "schema should be set");

				char Index[16];
				std::string_view ValueKey = Key;
				if (ValueKey.empty())
				{
					const char* IndexKey = Index;
					bson_uint32_to_string((uint32_t)ArrayId, &IndexKey, Index, sizeof(Index));
					ValueKey = IndexKey;
				}

				return bson_append_bool((bson_t*)Base, ValueKey.data(), (int)ValueKey.size(), Value);
#else
				return false;
#endif
			}
			bool Document::SetObjectId(const std::string_view& Key, uint8_t Value[12], size_t ArrayId)
			{
#ifdef VI_MONGOC
				VI_ASSERT(Base != nullptr, "schema should be set");

				bson_oid_t ObjectId;
				memcpy(ObjectId.bytes, Value, sizeof(uint8_t) * 12);

				char Index[16];
				std::string_view ValueKey = Key;
				if (ValueKey.empty())
				{
					const char* IndexKey = Index;
					bson_uint32_to_string((uint32_t)ArrayId, &IndexKey, Index, sizeof(Index));
					ValueKey = IndexKey;
				}

				return bson_append_oid((bson_t*)Base, ValueKey.data(), (int)ValueKey.size(), &ObjectId);
#else
				return false;
#endif
			}
			bool Document::SetNull(const std::string_view& Key, size_t ArrayId)
			{
#ifdef VI_MONGOC
				VI_ASSERT(Base != nullptr, "schema should be set");

				char Index[16];
				std::string_view ValueKey = Key;
				if (ValueKey.empty())
				{
					const char* IndexKey = Index;
					bson_uint32_to_string((uint32_t)ArrayId, &IndexKey, Index, sizeof(Index));
					ValueKey = IndexKey;
				}

				return bson_append_null((bson_t*)Base, ValueKey.data(), (int)ValueKey.size());
#else
				return false;
#endif
			}
			bool Document::SetProperty(const std::string_view& Key, Property* Value, size_t ArrayId)
			{
#ifdef VI_MONGOC
				VI_ASSERT(Base != nullptr, "schema should be set");
				VI_ASSERT(Value != nullptr, "property should be set");

				char Index[16];
				std::string_view ValueKey = Key;
				if (ValueKey.empty())
				{
					const char* IndexKey = Index;
					bson_uint32_to_string((uint32_t)ArrayId, &IndexKey, Index, sizeof(Index));
					ValueKey = IndexKey;
				}

				switch (Value->Mod)
				{
					case Type::Document:
						return SetSchema(Key, Value->AsDocument().Copy());
					case Type::Array:
						return SetArray(Key, Value->AsDocument().Copy());
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
			bool Document::HasProperty(const std::string_view& Key) const
			{
#ifdef VI_MONGOC
				VI_ASSERT(Base != nullptr, "schema should be set");
				VI_ASSERT(Core::Stringify::IsCString(Key), "key should be set");
				return bson_has_field(Base, Key.data());
#else
				return false;
#endif
			}
			bool Document::GetProperty(const std::string_view& Key, Property* Output) const
			{
#ifdef VI_MONGOC
				VI_ASSERT(Base != nullptr, "schema should be set");
				VI_ASSERT(Core::Stringify::IsCString(Key), "key should be set");
				VI_ASSERT(Output != nullptr, "property should be set");

				bson_iter_t It;
				if (!bson_iter_init_find(&It, Base, Key.data()))
					return false;

				return Clone(&It, Output);
#else
				return false;
#endif
			}
			bool Document::Clone(void* It, Property* Output)
			{
#ifdef VI_MONGOC
				VI_ASSERT(It != nullptr, "iterator should be set");
				VI_ASSERT(Output != nullptr, "property should be set");

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
						memcpy(New.ObjectId, Value->value.v_oid.bytes, sizeof(uint8_t) * 12);
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
#ifdef VI_MONGOC
				if (!Base)
					return 0;

				return bson_count_keys(Base);
#else
				return 0;
#endif
			}
			Core::String Document::ToRelaxedJSON() const
			{
#ifdef VI_MONGOC
				VI_ASSERT(Base != nullptr, "schema should be set");
				size_t Length = 0;
				char* Value = bson_as_relaxed_extended_json(Base, &Length);

				Core::String Output;
				Output.assign(Value, (uint64_t)Length);
				bson_free(Value);

				return Output;
#else
				return "";
#endif
			}
			Core::String Document::ToExtendedJSON() const
			{
#ifdef VI_MONGOC
				VI_ASSERT(Base != nullptr, "schema should be set");
				size_t Length = 0;
				char* Value = bson_as_canonical_extended_json(Base, &Length);

				Core::String Output;
				Output.assign(Value, (uint64_t)Length);
				bson_free(Value);

				return Output;
#else
				return "";
#endif
			}
			Core::String Document::ToJSON() const
			{
#ifdef VI_MONGOC
				VI_ASSERT(Base != nullptr, "schema should be set");
				size_t Length = 0;
				char* Value = bson_as_json(Base, &Length);

				Core::String Output;
				Output.assign(Value, (uint64_t)Length);
				bson_free(Value);

				return Output;
#else
				return "";
#endif
			}
			Core::String Document::ToIndices() const
			{
#ifdef VI_MONGOC
				VI_ASSERT(Base != nullptr, "schema should be set");
				char* Value = mongoc_collection_keys_to_index_string(Base);
				if (!Value)
					return Core::String();

				Core::String Output(Value);
				bson_free(Value);

				return Output;
#else
				return "";
#endif
			}
			Core::Schema* Document::ToSchema(bool IsArray) const
			{
#ifdef VI_MONGOC
				if (!Base)
					return nullptr;

				Core::Schema* Node = (IsArray ? Core::Var::Set::Array() : Core::Var::Set::Object());
				Loop([Node](Property* Key) -> bool
				{
					Core::String Name = (Node->Value.GetType() == Core::VarType::Array ? "" : Key->Name);
					switch (Key->Mod)
					{
						case Type::Document:
						{
							Node->Set(Name, Key->AsDocument().ToSchema(false));
							break;
						}
						case Type::Array:
						{
							Node->Set(Name, Key->AsDocument().ToSchema(true));
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
#ifdef VI_MONGOC
				return Base;
#else
				return nullptr;
#endif
			}
			Document Document::Copy() const
			{
#ifdef VI_MONGOC
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
			Property Document::At(const std::string_view& Name) const
			{
				Property Result;
				GetProperty(Name, &Result);
				return Result;
			}
			Document Document::FromSchema(Core::Schema* Src)
			{
#ifdef VI_MONGOC
				VI_ASSERT(Src != nullptr && Src->Value.IsObject(), "schema should be set");
				bool Array = (Src->Value.GetType() == Core::VarType::Array);
				Document Result = bson_new();
				size_t Index = 0;

				for (auto&& Node : Src->GetChilds())
				{
					switch (Node->Value.GetType())
					{
						case Core::VarType::Object:
							Result.SetSchema(Array ? nullptr : Node->Key.c_str(), Document::FromSchema(Node), Index);
							break;
						case Core::VarType::Array:
							Result.SetArray(Array ? nullptr : Node->Key.c_str(), Document::FromSchema(Node), Index);
							break;
						case Core::VarType::String:
							Result.SetString(Array ? nullptr : Node->Key.c_str(), Node->Value.GetString(), Index);
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
							if (Node->Value.Size() != 12)
							{
								Core::String Base = Compute::Codec::Bep45Encode(Node->Value.GetBlob());
								Result.SetString(Array ? nullptr : Node->Key.c_str(), Base, Index);
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
#ifdef VI_MONGOC
				return bson_new();
#else
				return nullptr;
#endif
			}
			ExpectsDB<Document> Document::FromJSON(const std::string_view& JSON)
			{
#ifdef VI_MONGOC
				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				TDocument* Result = bson_new_from_json((uint8_t*)JSON.data(), (ssize_t)JSON.size(), &Error);
				if (Result != nullptr && Error.code == 0)
					return Document(Result);
				else if (Result != nullptr)
					bson_destroy((bson_t*)Result);

				return DatabaseException(Error.code, Error.message);
#else
				return DatabaseException(0, "not supported");
#endif
			}
			Document Document::FromBuffer(const uint8_t* Buffer, size_t Length)
			{
#ifdef VI_MONGOC
				VI_ASSERT(Buffer != nullptr, "buffer should be set");
				return bson_new_from_data(Buffer, Length);
#else
				return nullptr;
#endif
			}
			Document Document::FromSource(TDocument* Src)
			{
#ifdef VI_MONGOC
				VI_ASSERT(Src != nullptr, "src should be set");
				TDocument* Dest = bson_new();
				bson_steal((bson_t*)Dest, (bson_t*)Src);
				return Dest;
#else
				return nullptr;
#endif
			}

			Address::Address() : Base(nullptr)
			{
			}
			Address::Address(TAddress* NewBase) : Base(NewBase)
			{
			}
			Address::Address(const Address& Other) : Address(std::move(*(Address*)&Other))
			{
			}
			Address::Address(Address&& Other) : Base(Other.Base)
			{
				Other.Base = nullptr;
			}
			Address::~Address()
			{
#ifdef VI_MONGOC
				if (Base != nullptr)
					mongoc_uri_destroy(Base);
				Base = nullptr;
#endif
			}
			Address& Address::operator =(const Address& Other)
			{
				return *this = std::move(*(Address*)&Other);
			}
			Address& Address::operator =(Address&& Other)
			{
				if (&Other == this)
					return *this;
#ifdef VI_MONGOC
				if (Base != nullptr)
					mongoc_uri_destroy(Base);
#endif
				Base = Other.Base;
				Other.Base = nullptr;
				return *this;
			}
			void Address::SetOption(const std::string_view& Name, int64_t Value)
			{
#ifdef VI_MONGOC
				VI_ASSERT(Base != nullptr, "context should be set");
				VI_ASSERT(Core::Stringify::IsCString(Name), "name should be set");
				mongoc_uri_set_option_as_int32(Base, Name.data(), (int32_t)Value);
#endif
			}
			void Address::SetOption(const std::string_view& Name, bool Value)
			{
#ifdef VI_MONGOC
				VI_ASSERT(Base != nullptr, "context should be set");
				VI_ASSERT(Core::Stringify::IsCString(Name), "name should be set");
				mongoc_uri_set_option_as_bool(Base, Name.data(), Value);
#endif
			}
			void Address::SetOption(const std::string_view& Name, const std::string_view& Value)
			{
#ifdef VI_MONGOC
				VI_ASSERT(Base != nullptr, "context should be set");
				VI_ASSERT(Core::Stringify::IsCString(Name), "name should be set");
				VI_ASSERT(Core::Stringify::IsCString(Value), "value should be set");
				mongoc_uri_set_option_as_utf8(Base, Name.data(), Value.data());
#endif
			}
			void Address::SetAuthMechanism(const std::string_view& Value)
			{
#ifdef VI_MONGOC
				VI_ASSERT(Base != nullptr, "context should be set");
				VI_ASSERT(Core::Stringify::IsCString(Value), "value should be set");
				mongoc_uri_set_auth_mechanism(Base, Value.data());
#endif
			}
			void Address::SetAuthSource(const std::string_view& Value)
			{
#ifdef VI_MONGOC
				VI_ASSERT(Base != nullptr, "context should be set");
				VI_ASSERT(Core::Stringify::IsCString(Value), "value should be set");
				mongoc_uri_set_auth_source(Base, Value.data());
#endif
			}
			void Address::SetCompressors(const std::string_view& Value)
			{
#ifdef VI_MONGOC
				VI_ASSERT(Base != nullptr, "context should be set");
				VI_ASSERT(Core::Stringify::IsCString(Value), "value should be set");
				mongoc_uri_set_compressors(Base, Value.data());
#endif
			}
			void Address::SetDatabase(const std::string_view& Value)
			{
#ifdef VI_MONGOC
				VI_ASSERT(Base != nullptr, "context should be set");
				VI_ASSERT(Core::Stringify::IsCString(Value), "value should be set");
				mongoc_uri_set_database(Base, Value.data());
#endif
			}
			void Address::SetUsername(const std::string_view& Value)
			{
#ifdef VI_MONGOC
				VI_ASSERT(Base != nullptr, "context should be set");
				VI_ASSERT(Core::Stringify::IsCString(Value), "value should be set");
				mongoc_uri_set_username(Base, Value.data());
#endif
			}
			void Address::SetPassword(const std::string_view& Value)
			{
#ifdef VI_MONGOC
				VI_ASSERT(Base != nullptr, "context should be set");
				VI_ASSERT(Core::Stringify::IsCString(Value), "value should be set");
				mongoc_uri_set_password(Base, Value.data());
#endif
			}
			TAddress* Address::Get() const
			{
#ifdef VI_MONGOC
				return Base;
#else
				return nullptr;
#endif
			}
			ExpectsDB<Address> Address::FromURL(const std::string_view& Location)
			{
#ifdef VI_MONGOC
				VI_ASSERT(Core::Stringify::IsCString(Location), "location should be set");
				TAddress* Result = mongoc_uri_new(Location.data());
				if (!strstr(Location.data(), MONGOC_URI_SOCKETTIMEOUTMS))
					mongoc_uri_set_option_as_int32(Result, MONGOC_URI_SOCKETTIMEOUTMS, 10000);

				return Address(Result);
#else
				return DatabaseException(0, "not supported");
#endif
			}

			Stream::Stream() : NetOptions(nullptr), Source(nullptr), Base(nullptr), Count(0)
			{
			}
			Stream::Stream(TCollection* NewSource, TStream* NewBase, Document&& NewOptions) : NetOptions(std::move(NewOptions)), Source(NewSource), Base(NewBase), Count(0)
			{
			}
			Stream::Stream(const Stream& Other) : Stream(std::move(*(Stream*)&Other))
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
#ifdef VI_MONGOC
				if (Base != nullptr)
					mongoc_bulk_operation_destroy(Base);

				Source = nullptr;
				Base = nullptr;
				Count = 0;
#endif
			}
			Stream& Stream::operator =(const Stream& Other)
			{
				return *this = std::move(*(Stream*)&Other);
			}
			Stream& Stream::operator =(Stream&& Other)
			{
				if (&Other == this)
					return *this;

#ifdef VI_MONGOC
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
			ExpectsDB<void> Stream::RemoveMany(const Document& Match, const Document& Options)
			{
#ifdef VI_MONGOC
				auto Status = NextOperation();
				if (!Status)
					return Status;

				return MongoExecuteQuery(&mongoc_bulk_operation_remove_many_with_opts, Base, Match.Get(), Options.Get());
#else
				return DatabaseException(0, "not supported");
#endif
			}
			ExpectsDB<void> Stream::RemoveOne(const Document& Match, const Document& Options)
			{
#ifdef VI_MONGOC
				auto Status = NextOperation();
				if (!Status)
					return Status;

				return MongoExecuteQuery(&mongoc_bulk_operation_remove_one_with_opts, Base, Match.Get(), Options.Get());
#else
				return DatabaseException(0, "not supported");
#endif
			}
			ExpectsDB<void> Stream::ReplaceOne(const Document& Match, const Document& Replacement, const Document& Options)
			{
#ifdef VI_MONGOC
				auto Status = NextOperation();
				if (!Status)
					return Status;

				return MongoExecuteQuery(&mongoc_bulk_operation_replace_one_with_opts, Base, Match.Get(), Replacement.Get(), Options.Get());
#else
				return DatabaseException(0, "not supported");
#endif
			}
			ExpectsDB<void> Stream::InsertOne(const Document& Result, const Document& Options)
			{
#ifdef VI_MONGOC
				auto Status = NextOperation();
				if (!Status)
					return Status;

				return MongoExecuteQuery(&mongoc_bulk_operation_insert_with_opts, Base, Result.Get(), Options.Get());
#else
				return DatabaseException(0, "not supported");
#endif
			}
			ExpectsDB<void> Stream::UpdateOne(const Document& Match, const Document& Result, const Document& Options)
			{
#ifdef VI_MONGOC
				auto Status = NextOperation();
				if (!Status)
					return Status;

				return MongoExecuteQuery(&mongoc_bulk_operation_update_one_with_opts, Base, Match.Get(), Result.Get(), Options.Get());
#else
				return DatabaseException(0, "not supported");
#endif
			}
			ExpectsDB<void> Stream::UpdateMany(const Document& Match, const Document& Result, const Document& Options)
			{
#ifdef VI_MONGOC
				auto Status = NextOperation();
				if (!Status)
					return Status;

				return MongoExecuteQuery(&mongoc_bulk_operation_update_many_with_opts, Base, Match.Get(), Result.Get(), Options.Get());
#else
				return DatabaseException(0, "not supported");
#endif
			}
			ExpectsDB<void> Stream::TemplateQuery(const std::string_view& Name, Core::SchemaArgs* Map)
			{
				VI_DEBUG("[mongoc] template query %s", Name.empty() ? "empty-query-name" : Core::String(Name).c_str());
				auto Template = Driver::Get()->GetQuery(Name, Map);
				if (!Template)
					return Template.Error();

				return Query(*Template);
			}
			ExpectsDB<void> Stream::Query(const Document& Command)
			{
#ifdef VI_MONGOC
				if (!Command.Get())
					return DatabaseException(0, "execute query: empty command");

				Property Type;
				if (!Command.GetProperty("type", &Type) || Type.Mod != Type::String)
					return DatabaseException(0, "execute query: no type field");

				if (Type.String == "update")
				{
					Property Match;
					if (!Command.GetProperty("match", &Match) || Match.Mod != Type::Document)
						return DatabaseException(0, "execute query: no match field");

					Property Update;
					if (!Command.GetProperty("update", &Update) || Update.Mod != Type::Document)
						return DatabaseException(0, "execute query: no update field");

					Property Options = Command["options"];
					return UpdateOne(Match.Reset(), Update.Reset(), Options.Reset());
				}
				else if (Type.String == "update-many")
				{
					Property Match;
					if (!Command.GetProperty("match", &Match) || Match.Mod != Type::Document)
						return DatabaseException(0, "execute query: no match field");

					Property Update;
					if (!Command.GetProperty("update", &Update) || Update.Mod != Type::Document)
						return DatabaseException(0, "execute query: no update field");

					Property Options = Command["options"];
					return UpdateMany(Match.Reset(), Update.Reset(), Options.Reset());
				}
				else if (Type.String == "insert")
				{
					Property Value;
					if (!Command.GetProperty("value", &Value) || Value.Mod != Type::Document)
						return DatabaseException(0, "execute query: no value field");

					Property Options = Command["options"];
					return InsertOne(Value.Reset(), Options.Reset());
				}
				else if (Type.String == "replace")
				{
					Property Match;
					if (!Command.GetProperty("match", &Match) || Match.Mod != Type::Document)
						return DatabaseException(0, "execute query: no match field");

					Property Value;
					if (!Command.GetProperty("value", &Value) || Value.Mod != Type::Document)
						return DatabaseException(0, "execute query: no value field");

					Property Options = Command["options"];
					return ReplaceOne(Match.Reset(), Value.Reset(), Options.Reset());
				}
				else if (Type.String == "remove")
				{
					Property Match;
					if (!Command.GetProperty("match", &Match) || Match.Mod != Type::Document)
						return DatabaseException(0, "execute query: no match field");

					Property Options = Command["options"];
					return RemoveOne(Match.Reset(), Options.Reset());
				}
				else if (Type.String == "remove-many")
				{
					Property Match;
					if (!Command.GetProperty("match", &Match) || Match.Mod != Type::Document)
						return DatabaseException(0, "execute query: no match field");

					Property Options = Command["options"];
					return RemoveMany(Match.Reset(), Options.Reset());
				}

				return DatabaseException(0, "invalid query type: " + Type.String);
#else
				return DatabaseException(0, "not supported");
#endif
			}
			ExpectsDB<void> Stream::NextOperation()
			{
#ifdef VI_MONGOC
				if (!Base || !Source)
					return DatabaseException(0, "invalid operation");

				if (Count++ <= 768)
					return Core::Expectation::Met;

				TDocument Result;
				auto Status = MongoExecuteQuery(&mongoc_bulk_operation_execute, Base, &Result);
				bson_destroy((bson_t*)&Result);
				if (Source != nullptr)
				{
					auto Subresult = Collection(Source).CreateStream(NetOptions);
					if (Subresult)
						*this = std::move(*Subresult);
				}
				return Status;
#else
				return DatabaseException(0, "not supported");
#endif
			}
			ExpectsPromiseDB<Document> Stream::ExecuteWithReply()
			{
#ifdef VI_MONGOC
				if (!Base)
					return ExpectsPromiseDB<Document>(DatabaseException(0, "invalid operation"));

				auto* Context = Base;
				return Core::Cotask<ExpectsDB<Document>>([Context]() mutable -> ExpectsDB<Document>
				{
					TDocument Subresult;
					auto Status = MongoExecuteQuery(&mongoc_bulk_operation_execute, Context, &Subresult);
					if (!Status)
					{
						bson_destroy((bson_t*)&Subresult);
						return Status.Error();
					}

					Document Result = Document::FromSource(&Subresult);
					bson_destroy((bson_t*)&Subresult);
					return Result;
				});
#else
				return ExpectsPromiseDB<Document>(DatabaseException(0, "not supported"));
#endif
			}
			ExpectsPromiseDB<void> Stream::Execute()
			{
#ifdef VI_MONGOC
				if (!Base)
					return ExpectsPromiseDB<void>(DatabaseException(0, "invalid operation"));

				auto* Context = Base;
				return Core::Cotask<ExpectsDB<void>>([Context]() mutable -> ExpectsDB<void>
				{
					TDocument Result;
					auto Status = MongoExecuteQuery(&mongoc_bulk_operation_execute, Context, &Result);
					bson_destroy((bson_t*)&Result);
					return Status;
				});
#else
				return ExpectsPromiseDB<void>(DatabaseException(0, "not supported"));
#endif
			}
			size_t Stream::GetHint() const
			{
#ifdef VI_MONGOC
				VI_ASSERT(Base != nullptr, "context should be set");
#if MONGOC_CHECK_VERSION(1, 28, 0)
				return (size_t)mongoc_bulk_operation_get_server_id(Base);
#else
				return (size_t)mongoc_bulk_operation_get_hint(Base);
#endif
#else
				return 0;
#endif
			}
			TStream* Stream::Get() const
			{
#ifdef VI_MONGOC
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
#ifdef VI_MONGOC
				if (!Base)
					return;

				mongoc_cursor_destroy(Base);
				Base = nullptr;
#endif
			}
			void Cursor::SetMaxAwaitTime(size_t MaxAwaitTime)
			{
#ifdef VI_MONGOC
				VI_ASSERT(Base != nullptr, "context should be set");
				mongoc_cursor_set_max_await_time_ms(Base, (uint32_t)MaxAwaitTime);
#endif
			}
			void Cursor::SetBatchSize(size_t BatchSize)
			{
#ifdef VI_MONGOC
				VI_ASSERT(Base != nullptr, "context should be set");
				mongoc_cursor_set_batch_size(Base, (uint32_t)BatchSize);
#endif
			}
			bool Cursor::SetLimit(int64_t Limit)
			{
#ifdef VI_MONGOC
				VI_ASSERT(Base != nullptr, "context should be set");
				return mongoc_cursor_set_limit(Base, Limit);
#else
				return false;
#endif
			}
			bool Cursor::SetHint(size_t Hint)
			{
#ifdef VI_MONGOC
				VI_ASSERT(Base != nullptr, "context should be set");
#if MONGOC_CHECK_VERSION(1, 28, 0)
				return mongoc_cursor_set_server_id(Base, (uint32_t)Hint);
#else
				return mongoc_cursor_set_hint(Base, (uint32_t)Hint);
#endif
#else
				return false;
#endif
			}
			bool Cursor::Empty() const
			{
#ifdef VI_MONGOC
				if (!Base)
					return true;

				return !mongoc_cursor_more(Base);
#else
				return true;
#endif
			}
			Core::Option<DatabaseException> Cursor::Error() const
			{
#ifdef VI_MONGOC
				if (!Base)
					return DatabaseException(0, "invalid operation");

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));
				if (!mongoc_cursor_error(Base, &Error))
					return Core::Optional::None;

				return DatabaseException(Error.code, Error.message);
#else
				return DatabaseException(0, "not supported");
#endif
			}
			ExpectsPromiseDB<void> Cursor::Next()
			{
#ifdef VI_MONGOC
				if (!Base)
					return ExpectsPromiseDB<void>(DatabaseException(0, "invalid operation"));

				auto* Context = Base;
				return Core::Cotask<ExpectsDB<void>>([Context]() -> ExpectsDB<void>
				{
					VI_MEASURE(Core::Timings::Intensive);
					TDocument* Query = nullptr;
					if (mongoc_cursor_next(Context, (const TDocument**)&Query))
						return Core::Expectation::Met;

					bson_error_t Error;
					memset(&Error, 0, sizeof(bson_error_t));
					if (!mongoc_cursor_error(Context, &Error))
						return DatabaseException(0, "end of stream");

					return DatabaseException(Error.code, Error.message);
				});
#else
				return ExpectsPromiseDB<void>(DatabaseException(0, "not supported"));
#endif
			}
			int64_t Cursor::GetId() const
			{
#ifdef VI_MONGOC
				VI_ASSERT(Base != nullptr, "context should be set");
				return (int64_t)mongoc_cursor_get_id(Base);
#else
				return 0;
#endif
			}
			int64_t Cursor::GetLimit() const
			{
#ifdef VI_MONGOC
				VI_ASSERT(Base != nullptr, "context should be set");
				return (int64_t)mongoc_cursor_get_limit(Base);
#else
				return 0;
#endif
			}
			size_t Cursor::GetMaxAwaitTime() const
			{
#ifdef VI_MONGOC
				VI_ASSERT(Base != nullptr, "context should be set");
				return (size_t)mongoc_cursor_get_max_await_time_ms(Base);
#else
				return 0;
#endif
			}
			size_t Cursor::GetBatchSize() const
			{
#ifdef VI_MONGOC
				VI_ASSERT(Base != nullptr, "context should be set");
				return (size_t)mongoc_cursor_get_batch_size(Base);
#else
				return 0;
#endif
			}
			size_t Cursor::GetHint() const
			{
#ifdef VI_MONGOC
				VI_ASSERT(Base != nullptr, "context should be set");
#if MONGOC_CHECK_VERSION(1, 28, 0)
				return (size_t)mongoc_cursor_get_server_id(Base);
#else
				return (size_t)mongoc_cursor_get_hint(Base);
#endif
#else
				return 0;
#endif
			}
			Document Cursor::Current() const
			{
#ifdef VI_MONGOC
				if (!Base)
					return nullptr;

				return (TDocument*)mongoc_cursor_current(Base);
#else
				return nullptr;
#endif
			}
			Cursor Cursor::Clone()
			{
#ifdef VI_MONGOC
				VI_ASSERT(Base != nullptr, "context should be set");
				return mongoc_cursor_clone(Base);
#else
				return nullptr;
#endif
			}
			TCursor* Cursor::Get() const
			{
#ifdef VI_MONGOC
				return Base;
#else
				return nullptr;
#endif
			}

			Response::Response() : NetSuccess(false)
			{
			}
			Response::Response(bool NewSuccess) : NetSuccess(NewSuccess)
			{
			}
			Response::Response(Cursor& NewCursor) : NetCursor(std::move(NewCursor))
			{
				NetSuccess = NetCursor && !NetCursor.ErrorOrEmpty();
			}
			Response::Response(Document& NewDocument) : NetDocument(std::move(NewDocument))
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
			ExpectsPromiseDB<Core::Schema*> Response::Fetch()
			{
				if (NetDocument)
					return ExpectsPromiseDB<Core::Schema*>(NetDocument.ToSchema());

				if (!NetCursor)
					return ExpectsPromiseDB<Core::Schema*>(DatabaseException(0, "fetch: no cursor"));

				return NetCursor.Next().Then<ExpectsDB<Core::Schema*>>([this](ExpectsDB<void>&& Result) -> ExpectsDB<Core::Schema*>
				{
					if (!Result)
						return Result.Error();

					return NetCursor.Current().ToSchema();
				});
			}
			ExpectsPromiseDB<Core::Schema*> Response::FetchAll()
			{
				if (NetDocument)
				{
					Core::Schema* Result = NetDocument.ToSchema();
					return ExpectsPromiseDB<Core::Schema*>(Result ? Result : Core::Var::Set::Array());
				}

				if (!NetCursor)
					return ExpectsPromiseDB<Core::Schema*>(Core::Var::Set::Array());

				return Core::Coasync<ExpectsDB<Core::Schema*>>([this]() -> ExpectsPromiseDB<Core::Schema*>
				{
					Core::UPtr<Core::Schema> Result = Core::Var::Set::Array();
					while (true)
					{
						auto Status = VI_AWAIT(NetCursor.Next());
						if (Status)
						{
							Result->Push(NetCursor.Current().ToSchema());
							continue;
						}
						else if (Status.Error().error_code() != 0 && Result->Empty())
							Coreturn ExpectsDB<Core::Schema*>(std::move(Status.Error()));
						break;
					}
					Coreturn ExpectsDB<Core::Schema*>(Result.Reset());
				});
			}
			ExpectsPromiseDB<Property> Response::GetProperty(const std::string_view& Name)
			{
				if (NetDocument)
				{
					Property Result;
					NetDocument.GetProperty(Name, &Result);
					return ExpectsPromiseDB<Property>(std::move(Result));
				}

				if (!NetCursor)
					return ExpectsPromiseDB<Property>(Property());

				Document Source = NetCursor.Current();
				if (Source)
				{
					Property Result;
					Source.GetProperty(Name, &Result);
					return ExpectsPromiseDB<Property>(std::move(Result));
				}

				Core::String Copy = Core::String(Name);
				return NetCursor.Next().Then<ExpectsDB<Property>>([this, Copy](ExpectsDB<void>&& Status) mutable -> ExpectsDB<Property>
				{
					Property Result;
					if (!Status)
						return Status.Error();

					Document Source = NetCursor.Current();
					if (!Source)
						return DatabaseException(0, "property not found: " + Copy);

					Source.GetProperty(Copy, &Result);
					return Result;
				});
			}
			Cursor&& Response::GetCursor()
			{
				return std::move(NetCursor);
			}
			Document&& Response::GetDocument()
			{
				return std::move(NetDocument);
			}
			bool Response::Success() const
			{
				return NetSuccess;
			}

			Transaction::Transaction() : Base(nullptr)
			{
			}
			Transaction::Transaction(TTransaction* NewBase) : Base(NewBase)
			{
			}
			ExpectsDB<void> Transaction::Push(const Document& QueryOptionsWrapper)
			{
#ifdef VI_MONGOC
				Document& QueryOptions = *(Document*)&QueryOptionsWrapper;
				if (!QueryOptions.Get())
					QueryOptions = bson_new();

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));
				if (!mongoc_client_session_append(Base, (bson_t*)QueryOptions.Get(), &Error) && Error.code != 0)
					return DatabaseException(Error.code, Error.message);

				return Core::Expectation::Met;
#else
				return DatabaseException(0, "not supported");
#endif
			}
			ExpectsDB<void> Transaction::Put(TDocument** QueryOptions) const
			{
#ifdef VI_MONGOC
				VI_ASSERT(QueryOptions != nullptr, "query options should be set");
				if (!*QueryOptions)
					*QueryOptions = bson_new();

				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));
				if (!mongoc_client_session_append(Base, (bson_t*)*QueryOptions, &Error) && Error.code != 0)
					return DatabaseException(Error.code, Error.message);

				return Core::Expectation::Met;
#else
				return DatabaseException(0, "not supported");
#endif
			}
			ExpectsPromiseDB<void> Transaction::Begin()
			{
#ifdef VI_MONGOC
				auto* Context = Base;
				return Core::Cotask<ExpectsDB<void>>([Context]()
				{
					return MongoExecuteQuery(&mongoc_client_session_start_transaction, Context, nullptr);
				});
#else
				return ExpectsPromiseDB<void>(DatabaseException(0, "not supported"));
#endif
			}
			ExpectsPromiseDB<void> Transaction::Rollback()
			{
#ifdef VI_MONGOC
				auto* Context = Base;
				return Core::Cotask<ExpectsDB<void>>([Context]()
				{
					return MongoExecuteQuery(&mongoc_client_session_abort_transaction, Context);
				});
#else
				return ExpectsPromiseDB<void>(DatabaseException(0, "not supported"));
#endif
			}
			ExpectsPromiseDB<Document> Transaction::RemoveMany(Collection& Source, const Document& Match, const Document& Options)
			{
#ifdef VI_MONGOC
				if (!Push(Options))
					return ExpectsPromiseDB<Document>(DatabaseException(0, "invalid operation"));

				return Source.RemoveMany(Match, Options);
#else
				return ExpectsPromiseDB<Document>(DatabaseException(0, "not supported"));
#endif
			}
			ExpectsPromiseDB<Document> Transaction::RemoveOne(Collection& Source, const Document& Match, const Document& Options)
			{
#ifdef VI_MONGOC
				if (!Push(Options))
					return ExpectsPromiseDB<Document>(DatabaseException(0, "invalid operation"));

				return Source.RemoveOne(Match, Options);
#else
				return ExpectsPromiseDB<Document>(DatabaseException(0, "not supported"));
#endif
			}
			ExpectsPromiseDB<Document> Transaction::ReplaceOne(Collection& Source, const Document& Match, const Document& Replacement, const Document& Options)
			{
#ifdef VI_MONGOC
				if (!Push(Options))
					return ExpectsPromiseDB<Document>(DatabaseException(0, "invalid operation"));

				return Source.ReplaceOne(Match, Replacement, Options);
#else
				return ExpectsPromiseDB<Document>(DatabaseException(0, "not supported"));
#endif
			}
			ExpectsPromiseDB<Document> Transaction::InsertMany(Collection& Source, Core::Vector<Document>& List, const Document& Options)
			{
#ifdef VI_MONGOC
				if (!Push(Options))
					return ExpectsPromiseDB<Document>(DatabaseException(0, "invalid operation"));

				return Source.InsertMany(List, Options);
#else
				return ExpectsPromiseDB<Document>(DatabaseException(0, "not supported"));
#endif
			}
			ExpectsPromiseDB<Document> Transaction::InsertOne(Collection& Source, const Document& Result, const Document& Options)
			{
#ifdef VI_MONGOC
				if (!Push(Options))
					return ExpectsPromiseDB<Document>(DatabaseException(0, "invalid operation"));

				return Source.InsertOne(Result, Options);
#else
				return ExpectsPromiseDB<Document>(DatabaseException(0, "not supported"));
#endif
			}
			ExpectsPromiseDB<Document> Transaction::UpdateMany(Collection& Source, const Document& Match, const Document& Update, const Document& Options)
			{
#ifdef VI_MONGOC
				if (!Push(Options))
					return ExpectsPromiseDB<Document>(DatabaseException(0, "invalid operation"));

				return Source.UpdateMany(Match, Update, Options);
#else
				return ExpectsPromiseDB<Document>(DatabaseException(0, "not supported"));
#endif
			}
			ExpectsPromiseDB<Document> Transaction::UpdateOne(Collection& Source, const Document& Match, const Document& Update, const Document& Options)
			{
#ifdef VI_MONGOC
				if (!Push(Options))
					return ExpectsPromiseDB<Document>(DatabaseException(0, "invalid operation"));

				return Source.UpdateOne(Match, Update, Options);
#else
				return ExpectsPromiseDB<Document>(DatabaseException(0, "not supported"));
#endif
			}
			ExpectsPromiseDB<Cursor> Transaction::FindMany(Collection& Source, const Document& Match, const Document& Options)
			{
#ifdef VI_MONGOC
				if (!Push(Options))
					return ExpectsPromiseDB<Cursor>(DatabaseException(0, "invalid operation"));

				return Source.FindMany(Match, Options);
#else
				return ExpectsPromiseDB<Cursor>(DatabaseException(0, "not supported"));
#endif
			}
			ExpectsPromiseDB<Cursor> Transaction::FindOne(Collection& Source, const Document& Match, const Document& Options)
			{
#ifdef VI_MONGOC
				if (!Push(Options))
					return ExpectsPromiseDB<Cursor>(DatabaseException(0, "invalid operation"));

				return Source.FindOne(Match, Options);
#else
				return ExpectsPromiseDB<Cursor>(DatabaseException(0, "not supported"));
#endif
			}
			ExpectsPromiseDB<Cursor> Transaction::Aggregate(Collection& Source, QueryFlags Flags, const Document& Pipeline, const Document& Options)
			{
#ifdef VI_MONGOC
				if (!Push(Options))
					return ExpectsPromiseDB<Cursor>(DatabaseException(0, "invalid operation"));

				return Source.Aggregate(Flags, Pipeline, Options);
#else
				return ExpectsPromiseDB<Cursor>(DatabaseException(0, "not supported"));
#endif
			}
			ExpectsPromiseDB<Response> Transaction::TemplateQuery(Collection& Source, const std::string_view& Name, Core::SchemaArgs* Map)
			{
				auto Template = Driver::Get()->GetQuery(Name, Map);
				if (!Template)
					return ExpectsPromiseDB<Response>(std::move(Template.Error()));

				return Query(Source, *Template);
			}
			ExpectsPromiseDB<Response> Transaction::Query(Collection& Source, const Document& Command)
			{
#ifdef VI_MONGOC
				return Source.Query(Command, *this);
#else
				return ExpectsPromiseDB<Response>(DatabaseException(0, "not supported"));
#endif
			}
			Core::Promise<TransactionState> Transaction::Commit()
			{
#ifdef VI_MONGOC
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
				return Core::Promise<TransactionState>(TransactionState::Fatal);
#endif
			}
			TTransaction* Transaction::Get() const
			{
#ifdef VI_MONGOC
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
			Collection::Collection(Collection&& Other) : Base(Other.Base)
			{
				Other.Base = nullptr;
			}
			Collection::~Collection()
			{
#ifdef VI_MONGOC
				if (Base != nullptr)
					mongoc_collection_destroy(Base);
				Base = nullptr;
#endif
			}
			Collection& Collection::operator =(Collection&& Other)
			{
				if (&Other == this)
					return *this;

#ifdef VI_MONGOC
				if (Base != nullptr)
					mongoc_collection_destroy(Base);
#endif
				Base = Other.Base;
				Other.Base = nullptr;
				return *this;
			}
			ExpectsPromiseDB<void> Collection::Rename(const std::string_view& NewDatabaseName, const std::string_view& NewCollectionName)
			{
#ifdef VI_MONGOC
				auto* Context = Base;
				auto NewDatabaseNameCopy = Core::String(NewDatabaseName);
				auto NewCollectionNameCopy = Core::String(NewDatabaseName);
				return Core::Cotask<ExpectsDB<void>>([Context, NewDatabaseNameCopy = std::move(NewDatabaseNameCopy), NewCollectionNameCopy = std::move(NewCollectionNameCopy)]() mutable
				{
					return MongoExecuteQuery(&mongoc_collection_rename, Context, NewDatabaseNameCopy.data(), NewCollectionNameCopy.data(), false);
				});
#else
				return ExpectsPromiseDB<void>(DatabaseException(0, "not supported"));
#endif
			}
			ExpectsPromiseDB<void> Collection::RenameWithOptions(const std::string_view& NewDatabaseName, const std::string_view& NewCollectionName, const Document& Options)
			{
#ifdef VI_MONGOC
				auto* Context = Base;
				auto NewDatabaseNameCopy = Core::String(NewDatabaseName);
				auto NewCollectionNameCopy = Core::String(NewDatabaseName);
				return Core::Cotask<ExpectsDB<void>>([Context, NewDatabaseNameCopy = std::move(NewDatabaseNameCopy), NewCollectionNameCopy = std::move(NewCollectionNameCopy), &Options]() mutable
				{
					return MongoExecuteQuery(&mongoc_collection_rename_with_opts, Context, NewDatabaseNameCopy.c_str(), NewCollectionNameCopy.c_str(), false, Options.Get());
				});
#else
				return ExpectsPromiseDB<void>(DatabaseException(0, "not supported"));
#endif
			}
			ExpectsPromiseDB<void> Collection::RenameWithRemove(const std::string_view& NewDatabaseName, const std::string_view& NewCollectionName)
			{
#ifdef VI_MONGOC
				auto* Context = Base;
				auto NewDatabaseNameCopy = Core::String(NewDatabaseName);
				auto NewCollectionNameCopy = Core::String(NewDatabaseName);
				return Core::Cotask<ExpectsDB<void>>([Context, NewDatabaseNameCopy = std::move(NewDatabaseNameCopy), NewCollectionNameCopy = std::move(NewCollectionNameCopy)]() mutable
				{
					return MongoExecuteQuery(&mongoc_collection_rename, Context, NewDatabaseNameCopy.c_str(), NewCollectionNameCopy.c_str(), true);
				});
#else
				return ExpectsPromiseDB<void>(DatabaseException(0, "not supported"));
#endif
			}
			ExpectsPromiseDB<void> Collection::RenameWithOptionsAndRemove(const std::string_view& NewDatabaseName, const std::string_view& NewCollectionName, const Document& Options)
			{
#ifdef VI_MONGOC
				auto* Context = Base;
				auto NewDatabaseNameCopy = Core::String(NewDatabaseName);
				auto NewCollectionNameCopy = Core::String(NewDatabaseName);
				return Core::Cotask<ExpectsDB<void>>([Context, NewDatabaseNameCopy = std::move(NewDatabaseNameCopy), NewCollectionNameCopy = std::move(NewCollectionNameCopy), &Options]() mutable
				{
					return MongoExecuteQuery(&mongoc_collection_rename_with_opts, Context, NewDatabaseNameCopy.c_str(), NewCollectionNameCopy.c_str(), true, Options.Get());
				});
#else
				return ExpectsPromiseDB<void>(DatabaseException(0, "not supported"));
#endif
			}
			ExpectsPromiseDB<void> Collection::Remove(const Document& Options)
			{
#ifdef VI_MONGOC
				auto* Context = Base;
				return Core::Cotask<ExpectsDB<void>>([Context, &Options]()
				{
					return MongoExecuteQuery(&mongoc_collection_drop_with_opts, Context, Options.Get());
				});
#else
				return ExpectsPromiseDB<void>(DatabaseException(0, "not supported"));
#endif
			}
			ExpectsPromiseDB<void> Collection::RemoveIndex(const std::string_view& Name, const Document& Options)
			{
#ifdef VI_MONGOC
				auto* Context = Base;
				auto NameCopy = Core::String(Name);
				return Core::Cotask<ExpectsDB<void>>([Context, NameCopy = std::move(NameCopy), &Options]() mutable
				{
					return MongoExecuteQuery(&mongoc_collection_drop_index_with_opts, Context, NameCopy.c_str(), Options.Get());
				});
#else
				return ExpectsPromiseDB<void>(DatabaseException(0, "not supported"));
#endif
			}
			ExpectsPromiseDB<Document> Collection::RemoveMany(const Document& Match, const Document& Options)
			{
#ifdef VI_MONGOC
				auto* Context = Base;
				return Core::Cotask<ExpectsDB<Document>>([Context, &Match, &Options]() -> ExpectsDB<Document>
				{
					TDocument Subresult;
					auto Status = MongoExecuteQuery(&mongoc_collection_delete_many, Context, Match.Get(), Options.Get(), &Subresult);
					if (!Status)
					{
						bson_destroy((bson_t*)&Subresult);
						return Status.Error();
					}
					
					Document Result = Document::FromSource(&Subresult);
					bson_destroy((bson_t*)&Subresult);
					return Result;
				});
#else
				return ExpectsPromiseDB<Document>(DatabaseException(0, "not supported"));
#endif
			}
			ExpectsPromiseDB<Document> Collection::RemoveOne(const Document& Match, const Document& Options)
			{
#ifdef VI_MONGOC
				auto* Context = Base;
				return Core::Cotask<ExpectsDB<Document>>([Context, &Match, &Options]() -> ExpectsDB<Document>
				{
					TDocument Subresult;
					auto Status = MongoExecuteQuery(&mongoc_collection_delete_one, Context, Match.Get(), Options.Get(), &Subresult);
					if (!Status)
					{
						bson_destroy((bson_t*)&Subresult);
						return Status.Error();
					}

					Document Result = Document::FromSource(&Subresult);
					bson_destroy((bson_t*)&Subresult);
					return Result;
				});
#else
				return ExpectsPromiseDB<Document>(DatabaseException(0, "not supported"));
#endif
			}
			ExpectsPromiseDB<Document> Collection::ReplaceOne(const Document& Match, const Document& Replacement, const Document& Options)
			{
#ifdef VI_MONGOC
				auto* Context = Base;
				return Core::Cotask<ExpectsDB<Document>>([Context, &Match, &Replacement, &Options]() -> ExpectsDB<Document>
				{
					TDocument Subresult;
					auto Status = MongoExecuteQuery(&mongoc_collection_replace_one, Context, Match.Get(), Replacement.Get(), Options.Get(), &Subresult);
					if (!Status)
					{
						bson_destroy((bson_t*)&Subresult);
						return Status.Error();
					}

					Document Result = Document::FromSource(&Subresult);
					bson_destroy((bson_t*)&Subresult);
					return Result;
				});
#else
				return ExpectsPromiseDB<Document>(DatabaseException(0, "not supported"));
#endif
			}
			ExpectsPromiseDB<Document> Collection::InsertMany(Core::Vector<Document>& List, const Document& Options)
			{
#ifdef VI_MONGOC
				VI_ASSERT(!List.empty(), "insert array should not be empty");
				Core::Vector<Document> Array(std::move(List));
				auto* Context = Base;

				return Core::Cotask<ExpectsDB<Document>>([Context, &Array, &Options]() -> ExpectsDB<Document>
				{
					Core::Vector<TDocument*> Subarray;
				    Subarray.reserve(Array.size());
				    for (auto& Item : Array)
						Subarray.push_back(Item.Get());

					TDocument Subresult;
					auto Status = MongoExecuteQuery(&mongoc_collection_insert_many, Context, (const TDocument**)Subarray.data(), (size_t)Array.size(), Options.Get(), &Subresult);
					if (!Status)
					{
						bson_destroy((bson_t*)&Subresult);
						return Status.Error();
					}

					Document Result = Document::FromSource(&Subresult);
					bson_destroy((bson_t*)&Subresult);
					return Result;
				});
#else
				return ExpectsPromiseDB<Document>(DatabaseException(0, "not supported"));
#endif
			}
			ExpectsPromiseDB<Document> Collection::InsertOne(const Document& Result, const Document& Options)
			{
#ifdef VI_MONGOC
				auto* Context = Base;
				return Core::Cotask<ExpectsDB<Document>>([Context, &Result, &Options]() -> ExpectsDB<Document>
				{
					TDocument Subresult;
					auto Status = MongoExecuteQuery(&mongoc_collection_insert_one, Context, Result.Get(), Options.Get(), &Subresult);
					if (!Status)
					{
						bson_destroy((bson_t*)&Subresult);
						return Status.Error();
					}

					Document Result = Document::FromSource(&Subresult);
					bson_destroy((bson_t*)&Subresult);
					return Result;
				});
#else
				return ExpectsPromiseDB<Document>(DatabaseException(0, "not supported"));
#endif
			}
			ExpectsPromiseDB<Document> Collection::UpdateMany(const Document& Match, const Document& Update, const Document& Options)
			{
#ifdef VI_MONGOC
				auto* Context = Base;
				return Core::Cotask<ExpectsDB<Document>>([Context, &Match, &Update, &Options]() -> ExpectsDB<Document>
				{
					TDocument Subresult;
					auto Status = MongoExecuteQuery(&mongoc_collection_update_many, Context, Match.Get(), Update.Get(), Options.Get(), &Subresult);
					if (!Status)
					{
						bson_destroy((bson_t*)&Subresult);
						return Status.Error();
					}

					Document Result = Document::FromSource(&Subresult);
					bson_destroy((bson_t*)&Subresult);
					return Result;
				});
#else
				return ExpectsPromiseDB<Document>(DatabaseException(0, "not supported"));
#endif
			}
			ExpectsPromiseDB<Document> Collection::UpdateOne(const Document& Match, const Document& Update, const Document& Options)
			{
#ifdef VI_MONGOC
				auto* Context = Base;
				return Core::Cotask<ExpectsDB<Document>>([Context, &Match, &Update, &Options]() -> ExpectsDB<Document>
				{
					TDocument Subresult;
					auto Status = MongoExecuteQuery(&mongoc_collection_update_one, Context, Match.Get(), Update.Get(), Options.Get(), &Subresult);
					if (!Status)
					{
						bson_destroy((bson_t*)&Subresult);
						return Status.Error();
					}

					Document Result = Document::FromSource(&Subresult);
					bson_destroy((bson_t*)&Subresult);
					return Result;
				});
#else
				return ExpectsPromiseDB<Document>(DatabaseException(0, "not supported"));
#endif
			}
			ExpectsPromiseDB<Document> Collection::FindAndModify(const Document& Query, const Document& Sort, const Document& Update, const Document& Fields, bool RemoveAt, bool Upsert, bool New)
			{
#ifdef VI_MONGOC
				auto* Context = Base;
				return Core::Cotask<ExpectsDB<Document>>([Context, &Query, &Sort, &Update, &Fields, RemoveAt, Upsert, New]() -> ExpectsDB<Document>
				{
					TDocument Subresult;
					auto Status = MongoExecuteQuery(&mongoc_collection_find_and_modify, Context, Query.Get(), Sort.Get(), Update.Get(), Fields.Get(), RemoveAt, Upsert, New, &Subresult);
					if (!Status)
					{
						bson_destroy((bson_t*)&Subresult);
						return Status.Error();
					}

					Document Result = Document::FromSource(&Subresult);
					bson_destroy((bson_t*)&Subresult);
					return Result;
				});
#else
				return ExpectsPromiseDB<Document>(DatabaseException(0, "not supported"));
#endif
			}
			Core::Promise<size_t> Collection::CountDocuments(const Document& Match, const Document& Options)
			{
#ifdef VI_MONGOC
				auto* Context = Base;
				return Core::Cotask<size_t>([Context, &Match, &Options]()
				{
					int64_t Count = mongoc_collection_count_documents(Context, Match.Get(), Options.Get(), nullptr, nullptr, nullptr);
					return Count > 0 ? (size_t)Count : (size_t)0;
				});
#else
				return Core::Promise<size_t>(0);
#endif
			}
			Core::Promise<size_t> Collection::CountDocumentsEstimated(const Document& Options)
			{
#ifdef VI_MONGOC
				auto* Context = Base;
				return Core::Cotask<size_t>([Context, &Options]()
				{
					int64_t Count = mongoc_collection_estimated_document_count(Context, Options.Get(), nullptr, nullptr, nullptr);
					return Count > 0 ? (size_t)Count : (size_t)0;
				});
#else
				return Core::Promise<size_t>(0);
#endif
			}
			ExpectsPromiseDB<Cursor> Collection::FindIndexes(const Document& Options)
			{
#ifdef VI_MONGOC
				auto* Context = Base;
				return Core::Cotask<ExpectsDB<Cursor>>([Context, &Options]()
				{
					return MongoExecuteCursor(&mongoc_collection_find_indexes_with_opts, Context, Options.Get());
				});
#else
				return ExpectsPromiseDB<Cursor>(DatabaseException(0, "not supported"));
#endif
			}
			ExpectsPromiseDB<Cursor> Collection::FindMany(const Document& Match, const Document& Options)
			{
#ifdef VI_MONGOC
				auto* Context = Base;
				return Core::Cotask<ExpectsDB<Cursor>>([Context, &Match, &Options]()
				{
					return MongoExecuteCursor(&mongoc_collection_find_with_opts, Context, Match.Get(), Options.Get(), nullptr);
				});
#else
				return ExpectsPromiseDB<Cursor>(DatabaseException(0, "not supported"));
#endif
			}
			ExpectsPromiseDB<Cursor> Collection::FindOne(const Document& Match, const Document& Options)
			{
#ifdef VI_MONGOC
				auto* Context = Base;
				return Core::Cotask<ExpectsDB<Cursor>>([Context, &Match, &Options]()
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
				return ExpectsPromiseDB<Cursor>(DatabaseException(0, "not supported"));
#endif
			}
			ExpectsPromiseDB<Cursor> Collection::Aggregate(QueryFlags Flags, const Document& Pipeline, const Document& Options)
			{
#ifdef VI_MONGOC
				auto* Context = Base;
				return Core::Cotask<ExpectsDB<Cursor>>([Context, Flags, &Pipeline, &Options]()
				{
					return MongoExecuteCursor(&mongoc_collection_aggregate, Context, (mongoc_query_flags_t)Flags, Pipeline.Get(), Options.Get(), nullptr);
				});
#else
				return ExpectsPromiseDB<Cursor>(DatabaseException(0, "not supported"));
#endif
			}
			ExpectsPromiseDB<Response> Collection::TemplateQuery(const std::string_view& Name, Core::SchemaArgs* Map, const Transaction& Session)
			{
				VI_DEBUG("[mongoc] template query %s", Name.empty() ? "empty-query-name" : Core::String(Name).c_str());
				auto Template = Driver::Get()->GetQuery(Name, Map);
				if (!Template)
					return ExpectsPromiseDB<Response>(Template.Error());

				return Query(*Template, Session);
			}
			ExpectsPromiseDB<Response> Collection::Query(const Document& Command, const Transaction& Session)
			{
#ifdef VI_MONGOC
				if (!Command.Get())
					return ExpectsPromiseDB<Response>(DatabaseException(0, "execute query: empty command"));

				Property Type;
				if (!Command.GetProperty("type", &Type) || Type.Mod != Type::String)
					return ExpectsPromiseDB<Response>(DatabaseException(0, "execute query: no type field"));

				if (Type.String == "aggregate")
				{
					QueryFlags Flags = QueryFlags::None; Property QFlags;
					if (Command.GetProperty("flags", &QFlags) && QFlags.Mod == Type::String)
					{
						for (auto& Item : Core::Stringify::Split(QFlags.String, ','))
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
					if (Session && !Session.Put(&Options.Source))
						return ExpectsPromiseDB<Response>(DatabaseException(0, "execute query: in transaction"));

					Property Pipeline;
					if (!Command.GetProperty("pipeline", &Pipeline) || Pipeline.Mod != Type::Array)
						return ExpectsPromiseDB<Response>(DatabaseException(0, "execute query: no pipeline field"));

					return Aggregate(Flags, Pipeline.Reset(), Options.Reset()).Then<ExpectsDB<Response>>([](ExpectsDB<Cursor>&& Result) -> ExpectsDB<Response>
					{
						if (!Result)
							return Result.Error();

						return Response(*Result);
					});
				}
				else if (Type.String == "find")
				{
					Property Options = Command["options"];
					if (Session && !Session.Put(&Options.Source))
						return ExpectsPromiseDB<Response>(DatabaseException(0, "execute query: in transaction"));

					Property Match;
					if (!Command.GetProperty("match", &Match) || Match.Mod != Type::Document)
						return ExpectsPromiseDB<Response>(DatabaseException(0, "execute query: no match field"));

					return FindOne(Match.Reset(), Options.Reset()).Then<ExpectsDB<Response>>([](ExpectsDB<Cursor>&& Result) -> ExpectsDB<Response>
					{
						if (!Result)
							return Result.Error();

						return Response(*Result);
					});
				}
				else if (Type.String == "find-many")
				{
					Property Options = Command["options"];
					if (Session && !Session.Put(&Options.Source))
						return ExpectsPromiseDB<Response>(DatabaseException(0, "execute query: in transaction"));

					Property Match;
					if (!Command.GetProperty("match", &Match) || Match.Mod != Type::Document)
						return ExpectsPromiseDB<Response>(DatabaseException(0, "execute query: no match field"));

					return FindMany(Match.Reset(), Options.Reset()).Then<ExpectsDB<Response>>([](ExpectsDB<Cursor>&& Result) -> ExpectsDB<Response>
					{
						if (!Result)
							return Result.Error();

						return Response(*Result);
					});
				}
				else if (Type.String == "update")
				{
					Property Options = Command["options"];
					if (Session && !Session.Put(&Options.Source))
						return ExpectsPromiseDB<Response>(DatabaseException(0, "execute query: in transaction"));

					Property Match;
					if (!Command.GetProperty("match", &Match) || Match.Mod != Type::Document)
						return ExpectsPromiseDB<Response>(DatabaseException(0, "execute query: no match field"));

					Property Update;
					if (!Command.GetProperty("update", &Update) || Update.Mod != Type::Document)
						return ExpectsPromiseDB<Response>(DatabaseException(0, "execute query: no update field"));

					return UpdateOne(Match.Reset(), Update.Reset(), Options.Reset()).Then<ExpectsDB<Response>>([](ExpectsDB<Document>&& Result) -> ExpectsDB<Response>
					{
						if (!Result)
							return Result.Error();

						return Response(*Result);
					});
				}
				else if (Type.String == "update-many")
				{
					Property Options = Command["options"];
					if (Session && !Session.Put(&Options.Source))
						return ExpectsPromiseDB<Response>(DatabaseException(0, "execute query: in transaction"));

					Property Match;
					if (!Command.GetProperty("match", &Match) || Match.Mod != Type::Document)
						return ExpectsPromiseDB<Response>(DatabaseException(0, "execute query: no match field"));

					Property Update;
					if (!Command.GetProperty("update", &Update) || Update.Mod != Type::Document)
						return ExpectsPromiseDB<Response>(DatabaseException(0, "execute query: no update field"));

					return UpdateMany(Match.Reset(), Update.Reset(), Options.Reset()).Then<ExpectsDB<Response>>([](ExpectsDB<Document>&& Result) -> ExpectsDB<Response>
					{
						if (!Result)
							return Result.Error();

						return Response(*Result);
					});
				}
				else if (Type.String == "insert")
				{
					Property Options = Command["options"];
					if (Session && !Session.Put(&Options.Source))
						return ExpectsPromiseDB<Response>(DatabaseException(0, "execute query: in transaction"));

					Property Value;
					if (!Command.GetProperty("value", &Value) || Value.Mod != Type::Document)
						return ExpectsPromiseDB<Response>(DatabaseException(0, "execute query: no value field"));

					return InsertOne(Value.Reset(), Options.Reset()).Then<ExpectsDB<Response>>([](ExpectsDB<Document>&& Result) -> ExpectsDB<Response>
					{
						if (!Result)
							return Result.Error();

						return Response(*Result);
					});
				}
				else if (Type.String == "insert-many")
				{
					Property Options = Command["options"];
					if (Session && !Session.Put(&Options.Source))
						return ExpectsPromiseDB<Response>(DatabaseException(0, "execute query: in transaction"));

					Property Values;
					if (!Command.GetProperty("values", &Values) || Values.Mod != Type::Array)
						return ExpectsPromiseDB<Response>(DatabaseException(0, "execute query: no values field"));

					Core::Vector<Document> Data;
					Values.AsDocument().Loop([&Data](Property* Value)
					{
						Data.push_back(Value->Reset());
						return true;
					});

					return InsertMany(Data, Options.Reset()).Then<ExpectsDB<Response>>([](ExpectsDB<Document>&& Result) -> ExpectsDB<Response>
					{
						if (!Result)
							return Result.Error();

						return Response(*Result);
					});
				}
				else if (Type.String == "find-update")
				{
					Property Match;
					if (!Command.GetProperty("match", &Match) || Match.Mod != Type::Document)
						return ExpectsPromiseDB<Response>(DatabaseException(0, "execute query: no match field"));

					Property Sort = Command["sort"];
					Property Update = Command["update"];
					Property Fields = Command["fields"];
					Property Remove = Command["remove"];
					Property Upsert = Command["upsert"];
					Property New = Command["new"];

					return FindAndModify(Match.Reset(), Sort.Reset(), Update.Reset(), Fields.Reset(), Remove.Boolean, Upsert.Boolean, New.Boolean).Then<ExpectsDB<Response>>([](ExpectsDB<Document>&& Result) -> ExpectsDB<Response>
					{
						if (!Result)
							return Result.Error();

						return Response(*Result);
					});
				}
				else if (Type.String == "replace")
				{
					Property Options = Command["options"];
					if (Session && !Session.Put(&Options.Source))
						return ExpectsPromiseDB<Response>(DatabaseException(0, "execute query: in transaction"));

					Property Match;
					if (!Command.GetProperty("match", &Match) || Match.Mod != Type::Document)
						return ExpectsPromiseDB<Response>(DatabaseException(0, "execute query: no match field"));

					Property Value;
					if (!Command.GetProperty("value", &Value) || Value.Mod != Type::Document)
						return ExpectsPromiseDB<Response>(DatabaseException(0, "execute query: no value field"));

					return ReplaceOne(Match.Reset(), Value.Reset(), Options.Reset()).Then<ExpectsDB<Response>>([](ExpectsDB<Document>&& Result) -> ExpectsDB<Response>
					{
						if (!Result)
							return Result.Error();

						return Response(*Result);
					});
				}
				else if (Type.String == "remove")
				{
					Property Options = Command["options"];
					if (Session && !Session.Put(&Options.Source))
						return ExpectsPromiseDB<Response>(DatabaseException(0, "execute query: in transaction"));

					Property Match;
					if (!Command.GetProperty("match", &Match) || Match.Mod != Type::Document)
						return ExpectsPromiseDB<Response>(DatabaseException(0, "execute query: no match field"));

					return RemoveOne(Match.Reset(), Options.Reset()).Then<ExpectsDB<Response>>([](ExpectsDB<Document>&& Result) -> ExpectsDB<Response>
					{
						if (!Result)
							return Result.Error();

						return Response(*Result);
					});
				}
				else if (Type.String == "remove-many")
				{
					Property Options = Command["options"];
					if (Session && !Session.Put(&Options.Source))
						return ExpectsPromiseDB<Response>(DatabaseException(0, "execute query: in transaction"));

					Property Match;
					if (!Command.GetProperty("match", &Match) || Match.Mod != Type::Document)
						return ExpectsPromiseDB<Response>(DatabaseException(0, "execute query: no match field"));

					return RemoveMany(Match.Reset(), Options.Reset()).Then<ExpectsDB<Response>>([](ExpectsDB<Document>&& Result) -> ExpectsDB<Response>
					{
						if (!Result)
							return Result.Error();

						return Response(*Result);
					});
				}

				return ExpectsPromiseDB<Response>(DatabaseException(0, "invalid query type: " + Type.String));
#else
				return ExpectsPromiseDB<Response>(DatabaseException(0, "not supported"));
#endif
			}
			Core::String Collection::GetName() const
			{
#ifdef VI_MONGOC
				VI_ASSERT(Base != nullptr, "context should be set");
				return mongoc_collection_get_name(Base);
#else
				return Core::String();
#endif
			}
			ExpectsDB<Stream> Collection::CreateStream(Document& Options)
			{
#ifdef VI_MONGOC
				if (!Base)
					return DatabaseException(0, "invalid operation");

				TStream* Operation = mongoc_collection_create_bulk_operation_with_opts(Base, Options.Get());
				return Stream(Base, Operation, std::move(Options));
#else
				return DatabaseException(0, "not supported");
#endif
			}
			TCollection* Collection::Get() const
			{
#ifdef VI_MONGOC
				return Base;
#else
				return nullptr;
#endif
			}

			Database::Database() : Base(nullptr)
			{
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
#ifdef VI_MONGOC
				if (Base != nullptr)
					mongoc_database_destroy(Base);
				Base = nullptr;
#endif
			}
			Database& Database::operator =(Database&& Other)
			{
				if (&Other == this)
					return *this;

#ifdef VI_MONGOC
				if (Base != nullptr)
					mongoc_database_destroy(Base);
#endif
				Base = Other.Base;
				Other.Base = nullptr;
				return *this;
			}
			ExpectsPromiseDB<void> Database::RemoveAllUsers()
			{
#ifdef VI_MONGOC
				auto* Context = Base;
				return Core::Cotask<ExpectsDB<void>>([Context]()
				{
					return MongoExecuteQuery(&mongoc_database_remove_all_users, Context);
				});
#else
				return ExpectsPromiseDB<void>(DatabaseException(0, "not supported"));
#endif
			}
			ExpectsPromiseDB<void> Database::RemoveUser(const std::string_view& Name)
			{
#ifdef VI_MONGOC
				auto* Context = Base;
				auto NameCopy = Core::String(Name);
				return Core::Cotask<ExpectsDB<void>>([Context, NameCopy = std::move(NameCopy)]() mutable
				{
					return MongoExecuteQuery(&mongoc_database_remove_user, Context, NameCopy.c_str());
				});
#else
				return ExpectsPromiseDB<void>(DatabaseException(0, "not supported"));
#endif
			}
			ExpectsPromiseDB<void> Database::Remove()
			{
#ifdef VI_MONGOC
				auto* Context = Base;
				return Core::Cotask<ExpectsDB<void>>([Context]()
				{
					return MongoExecuteQuery(&mongoc_database_drop, Context);
				});
#else
				return ExpectsPromiseDB<void>(DatabaseException(0, "not supported"));
#endif
			}
			ExpectsPromiseDB<void> Database::RemoveWithOptions(const Document& Options)
			{
#ifdef VI_MONGOC
				if (!Options.Get())
					return Remove();

				auto* Context = Base;
				return Core::Cotask<ExpectsDB<void>>([Context, &Options]()
				{
					return MongoExecuteQuery(&mongoc_database_drop_with_opts, Context, Options.Get());
				});
#else
				return ExpectsPromiseDB<void>(DatabaseException(0, "not supported"));
#endif
			}
			ExpectsPromiseDB<void> Database::AddUser(const std::string_view& Username, const std::string_view& Password, const Document& Roles, const Document& Custom)
			{
#ifdef VI_MONGOC
				auto* Context = Base;
				auto UsernameCopy = Core::String(Username);
				auto PasswordCopy = Core::String(Password);
				return Core::Cotask<ExpectsDB<void>>([Context, UsernameCopy = std::move(UsernameCopy), PasswordCopy = std::move(PasswordCopy), &Roles, &Custom]() mutable
				{
					return MongoExecuteQuery(&mongoc_database_add_user, Context, UsernameCopy.c_str(), PasswordCopy.c_str(), Roles.Get(), Custom.Get());
				});
#else
				return ExpectsPromiseDB<void>(DatabaseException(0, "not supported"));
#endif
			}
			ExpectsPromiseDB<void> Database::HasCollection(const std::string_view& Name)
			{
#ifdef VI_MONGOC
				auto* Context = Base;
				auto NameCopy = Core::String(Name);
				return Core::Cotask<ExpectsDB<void>>([Context, NameCopy = std::move(NameCopy)]() mutable -> ExpectsDB<void>
				{
					bson_error_t Error;
					memset(&Error, 0, sizeof(bson_error_t));
					if (!mongoc_database_has_collection(Context, NameCopy.c_str(), &Error))
						return DatabaseException(Error.code, Error.message);

					return Core::Expectation::Met;
				});
#else
				return ExpectsPromiseDB<void>(DatabaseException(0, "not supported"));
#endif
			}
			ExpectsPromiseDB<Cursor> Database::FindCollections(const Document& Options)
			{
#ifdef VI_MONGOC
				auto* Context = Base;
				return Core::Cotask<ExpectsDB<Cursor>>([Context, &Options]()
				{
					return MongoExecuteCursor(&mongoc_database_find_collections_with_opts, Context, Options.Get());
				});
#else
				return ExpectsPromiseDB<Cursor>(DatabaseException(0, "not supported"));
#endif
			}
			ExpectsPromiseDB<Collection> Database::CreateCollection(const std::string_view& Name, const Document& Options)
			{
#ifdef VI_MONGOC
				if (!Base)
					return ExpectsPromiseDB<Collection>(DatabaseException(0, "invalid operation"));

				auto* Context = Base;
				auto NameCopy = Core::String(Name);
				return Core::Cotask<ExpectsDB<Collection>>([Context, NameCopy = std::move(NameCopy), &Options]() mutable -> ExpectsDB<Collection>
				{
					bson_error_t Error;
					memset(&Error, 0, sizeof(bson_error_t));
					TCollection* Result = mongoc_database_create_collection(Context, NameCopy.c_str(), Options.Get(), &Error);
					if (!Result)
						return DatabaseException(Error.code, Error.message);
					
					return Collection(Result);
				});
#else
				return ExpectsPromiseDB<Collection>(DatabaseException(0, "not supported"));
#endif
			}
			ExpectsDB<Core::Vector<Core::String>> Database::GetCollectionNames(const Document& Options) const
			{
#ifdef VI_MONGOC
				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));
				if (!Base)
					return DatabaseException(0, "invalid operation");

				char** Names = mongoc_database_get_collection_names_with_opts(Base, Options.Get(), &Error);
				if (!Names)
					return DatabaseException(Error.code, Error.message);

				Core::Vector<Core::String> Output;
				for (unsigned i = 0; Names[i]; i++)
					Output.push_back(Names[i]);

				bson_strfreev(Names);
				return Output;
#else
				return DatabaseException(0, "not supported");
#endif
			}
			Core::String Database::GetName() const
			{
#ifdef VI_MONGOC
				VI_ASSERT(Base != nullptr, "context should be set");
				auto* Name = mongoc_database_get_name(Base);
				return Name ? Name : "";
#else
				return Core::String();
#endif
			}
			Collection Database::GetCollection(const std::string_view& Name)
			{
#ifdef VI_MONGOC
				VI_ASSERT(Base != nullptr, "context should be set");
				VI_ASSERT(Core::Stringify::IsCString(Name), "name should be set");
				return mongoc_database_get_collection(Base, Name.data());
#else
				return nullptr;
#endif
			}
			TDatabase* Database::Get() const
			{
#ifdef VI_MONGOC
				return Base;
#else
				return nullptr;
#endif
			}

			Watcher::Watcher()
			{
			}
			Watcher::Watcher(TWatcher* NewBase) : Base(NewBase)
			{
			}
			Watcher::Watcher(const Watcher& Other) : Watcher(std::move(*(Watcher*)&Other))
			{
			}
			Watcher::Watcher(Watcher&& Other) : Base(Other.Base)
			{
				Other.Base = nullptr;
			}
			Watcher::~Watcher()
			{
#ifdef VI_MONGOC
				if (Base != nullptr)
					mongoc_change_stream_destroy(Base);
				Base = nullptr;
#endif
			}
			Watcher& Watcher::operator =(const Watcher& Other)
			{
				return *this = std::move(*(Watcher*)&Other);
			}
			Watcher& Watcher::operator =(Watcher&& Other)
			{
				if (&Other == this)
					return *this;
#ifdef VI_MONGOC
				if (Base != nullptr)
					mongoc_change_stream_destroy(Base);
#endif
				Base = Other.Base;
				Other.Base = nullptr;
				return *this;
			}
			ExpectsPromiseDB<void> Watcher::Next(Document& Result)
			{
#ifdef VI_MONGOC
				if (!Base || !Result.Get())
					return ExpectsPromiseDB<void>(DatabaseException(0, "invalid operation"));

				auto* Context = Base;
				return Core::Cotask<ExpectsDB<void>>([Context, &Result]() -> ExpectsDB<void>
				{
					TDocument* Ptr = Result.Get();
					if (!mongoc_change_stream_next(Context, (const TDocument**)&Ptr))
						return DatabaseException(0, "end of stream");

					return Core::Expectation::Met;
				});
#else
				return ExpectsPromiseDB<void>(DatabaseException(0, "not supported"));
#endif
			}
			ExpectsPromiseDB<void> Watcher::Error(Document& Result)
			{
#ifdef VI_MONGOC
				if (!Base || !Result.Get())
					return ExpectsPromiseDB<void>(DatabaseException(0, "invalid operation"));

				auto* Context = Base;
				return Core::Cotask<ExpectsDB<void>>([Context, &Result]() -> ExpectsDB<void>
				{
					TDocument* Ptr = Result.Get();
					if (!mongoc_change_stream_error_document(Context, nullptr, (const TDocument**)&Ptr))
						return DatabaseException(0, "end of stream");

					return Core::Expectation::Met;
				});
#else
				return ExpectsPromiseDB<void>(DatabaseException(0, "not supported"));
#endif
			}
			TWatcher* Watcher::Get() const
			{
#ifdef VI_MONGOC
				return Base;
#else
				return nullptr;
#endif
			}
			ExpectsDB<Watcher> Watcher::FromConnection(Connection* Connection, const Document& Pipeline, const Document& Options)
			{
#ifdef VI_MONGOC
				if (!Connection)
					return DatabaseException(0, "invalid operation");

				return Watcher(mongoc_client_watch(Connection->Get(), Pipeline.Get(), Options.Get()));
#else
				return DatabaseException(0, "not supported");
#endif
			}
			ExpectsDB<Watcher> Watcher::FromDatabase(const Database& Database, const Document& Pipeline, const Document& Options)
			{
#ifdef VI_MONGOC
				if (!Database.Get())
					return DatabaseException(0, "invalid operation");

				return Watcher(mongoc_database_watch(Database.Get(), Pipeline.Get(), Options.Get()));
#else
				return DatabaseException(0, "not supported");
#endif
			}
			ExpectsDB<Watcher> Watcher::FromCollection(const Collection& Collection, const Document& Pipeline, const Document& Options)
			{
#ifdef VI_MONGOC
				if (!Collection.Get())
					return DatabaseException(0, "invalid operation");

				return Watcher(mongoc_collection_watch(Collection.Get(), Pipeline.Get(), Options.Get()));
#else
				return DatabaseException(0, "not supported");
#endif
			}

			Connection::Connection() : Connected(false), Session(nullptr), Base(nullptr), Master(nullptr)
			{
			}
			Connection::~Connection() noexcept
			{
#ifdef VI_MONGOC
				TTransaction* Context = Session.Get();
				if (Context != nullptr)
					mongoc_client_session_destroy(Context);
#endif
				if (Connected && Base != nullptr)
					Disconnect();
			}
			ExpectsPromiseDB<void> Connection::Connect(Address* Location)
			{
#ifdef VI_MONGOC
				VI_ASSERT(Master != nullptr, "connection should be created outside of cluster");
				VI_ASSERT(Location && Location->Get(), "location should be valid");
				if (!Core::OS::Control::Has(Core::AccessOption::Net))
					return ExpectsPromiseDB<void>(DatabaseException(-1, "connect failed: permission denied"));
				else if (Connected)
					return Disconnect().Then<ExpectsPromiseDB<void>>([this, Location](ExpectsDB<void>&&) { return this->Connect(Location); });

				TAddress* Address = Location->Get(); *Location = nullptr;
				return Core::Cotask<ExpectsDB<void>>([this, Address]() -> ExpectsDB<void>
				{
					VI_MEASURE(Core::Timings::Intensive);
					Base = mongoc_client_new_from_uri(Address);
					if (!Base)
						return DatabaseException(0, "connect: invalid address");

					Driver::Get()->AttachQueryLog(Base);
					Connected = true;
					return Core::Expectation::Met;
				});
#else
				return ExpectsPromiseDB<void>(DatabaseException(0, "not supported"));
#endif
			}
			ExpectsPromiseDB<void> Connection::Disconnect()
			{
#ifdef VI_MONGOC
				VI_ASSERT(Connected && Base, "connection should be established");
				return Core::Cotask<ExpectsDB<void>>([this]() -> ExpectsDB<void>
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
					return Core::Expectation::Met;
				});
#else
				return ExpectsPromiseDB<void>(DatabaseException(0, "not supported"));
#endif
			}
			ExpectsPromiseDB<void> Connection::MakeTransaction(std::function<Core::Promise<bool>(Transaction*)>&& Callback)
			{
#ifdef VI_MONGOC
				VI_ASSERT(Callback, "callback should not be empty");
				return Core::Coasync<ExpectsDB<void>>([this, Callback = std::move(Callback)]() mutable -> ExpectsPromiseDB<void>
				{
					auto Context = GetSession();
					if (!Context)
						Coreturn ExpectsDB<void>(std::move(Context.Error()));
					else if (!*Context)
						Coreturn ExpectsDB<void>(DatabaseException(0, "make transaction: no session"));

					while (true)
					{
						auto Status = VI_AWAIT(Context->Begin());
						if (!Status)
							Coreturn ExpectsDB<void>(std::move(Status));

						if (!VI_AWAIT(Callback(*Context)))
							break;

						while (true)
						{
							TransactionState State = VI_AWAIT(Context->Commit());
							if (State == TransactionState::Fatal)
								Coreturn ExpectsDB<void>(DatabaseException(0, "make transaction: fatal error"));
							else if (State == TransactionState::OK)
								Coreturn ExpectsDB<void>(Core::Expectation::Met);

							if (State == TransactionState::Retry_Commit)
							{
								VI_DEBUG("[mongoc] retrying transaction commit");
								continue;
							}

							if (State == TransactionState::Retry)
							{
								VI_DEBUG("[mongoc] retrying full transaction");
								break;
							}
						}
					}

					VI_AWAIT(Context->Rollback());
					Coreturn ExpectsDB<void>(DatabaseException(0, "make transaction: aborted"));
				});
#else
				return ExpectsPromiseDB<void>(DatabaseException(0, "not supported"));
#endif
			}
			ExpectsPromiseDB<Cursor> Connection::FindDatabases(const Document& Options)
			{
#ifdef VI_MONGOC
				return Core::Cotask<ExpectsDB<Cursor>>([this, &Options]() -> ExpectsDB<Cursor>
				{
					mongoc_cursor_t* Result = mongoc_client_find_databases_with_opts(Base, Options.Get());
					if (!Result)
						return DatabaseException(0, "databases not found");

					return Cursor(Result);
				});
#else
				return ExpectsPromiseDB<Cursor>(DatabaseException(0, "not supported"));
#endif
			}
			void Connection::SetProfile(const std::string_view& Name)
			{
#ifdef VI_MONGOC
				VI_ASSERT(Core::Stringify::IsCString(Name), "name should be set");
				mongoc_client_set_appname(Base, Name.data());
#endif
			}
			ExpectsDB<void> Connection::SetServer(bool ForWrites)
			{
#ifdef VI_MONGOC
				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));

				mongoc_server_description_t* Server = mongoc_client_select_server(Base, ForWrites, nullptr, &Error);
				if (!Server)
					return DatabaseException(Error.code, Error.message);

				mongoc_server_description_destroy(Server);
				return Core::Expectation::Met;
#else
				return DatabaseException(0, "not supported");
#endif
			}
			ExpectsDB<Transaction*> Connection::GetSession()
			{
#ifdef VI_MONGOC
				if (Session.Get() != nullptr)
					return &Session;

				bson_error_t Error;
				Session = mongoc_client_start_session(Base, nullptr, &Error);
				if (!Session.Get())
					return DatabaseException(Error.code, Error.message);

				return &Session;
#else
				return &Session;
#endif
			}
			Database Connection::GetDatabase(const std::string_view& Name) const
			{
#ifdef VI_MONGOC
				VI_ASSERT(Base != nullptr, "context should be set");
				VI_ASSERT(Core::Stringify::IsCString(Name), "name should be set");
				return mongoc_client_get_database(Base, Name.data());
#else
				return nullptr;
#endif
			}
			Database Connection::GetDefaultDatabase() const
			{
#ifdef VI_MONGOC
				VI_ASSERT(Base != nullptr, "context should be set");
				return mongoc_client_get_default_database(Base);
#else
				return nullptr;
#endif
			}
			Collection Connection::GetCollection(const std::string_view& DatabaseName, const std::string_view& Name) const
			{
#ifdef VI_MONGOC
				VI_ASSERT(Base != nullptr, "context should be set");
				VI_ASSERT(Core::Stringify::IsCString(DatabaseName), "db name should be set");
				VI_ASSERT(Core::Stringify::IsCString(Name), "name should be set");
				return mongoc_client_get_collection(Base, DatabaseName.data(), Name.data());
#else
				return nullptr;
#endif
			}
			Address Connection::GetAddress() const
			{
#ifdef VI_MONGOC
				VI_ASSERT(Base != nullptr, "context should be set");
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
			ExpectsDB<Core::Vector<Core::String>> Connection::GetDatabaseNames(const Document& Options) const
			{
#ifdef VI_MONGOC
				VI_ASSERT(Base != nullptr, "context should be set");
				bson_error_t Error;
				memset(&Error, 0, sizeof(bson_error_t));
				char** Names = mongoc_client_get_database_names_with_opts(Base, Options.Get(), &Error);
				if (!Names)
					return DatabaseException(Error.code, Error.message);

				Core::Vector<Core::String> Output;
				for (unsigned i = 0; Names[i]; i++)
					Output.push_back(Names[i]);

				bson_strfreev(Names);
				return Output;
#else
				return DatabaseException(0, "not supported");
#endif
			}
			bool Connection::IsConnected() const
			{
				return Connected;
			}

			Cluster::Cluster() : Connected(false), Pool(nullptr), SrcAddress(nullptr)
			{
			}
			Cluster::~Cluster() noexcept
			{
#ifdef VI_MONGOC
				if (Pool != nullptr)
				{
					mongoc_client_pool_destroy(Pool);
					Pool = nullptr;
				}
				SrcAddress.~Address();
				Connected = false;
#endif
			}
			ExpectsPromiseDB<void> Cluster::Connect(Address* Location)
			{
#ifdef VI_MONGOC
				VI_ASSERT(Location && Location->Get(), "location should be set");
				if (!Core::OS::Control::Has(Core::AccessOption::Net))
					return ExpectsPromiseDB<void>(DatabaseException(-1, "connect failed: permission denied"));
				else if (Connected)
					return Disconnect().Then<ExpectsPromiseDB<void>>([this, Location](ExpectsDB<void>&&) { return this->Connect(Location); });

				TAddress* Context = Location->Get(); *Location = nullptr;
				return Core::Cotask<ExpectsDB<void>>([this, Context]() -> ExpectsDB<void>
				{
					VI_MEASURE(Core::Timings::Intensive);
					SrcAddress = Context;
					Pool = mongoc_client_pool_new(SrcAddress.Get());
					if (!Pool)
						return DatabaseException(0, "connect: invalid address");

					Driver::Get()->AttachQueryLog(Pool);
					Connected = true;
					return Core::Expectation::Met;
				});
#else
				return ExpectsPromiseDB<void>(DatabaseException(0, "not supported"));
#endif
			}
			ExpectsPromiseDB<void> Cluster::Disconnect()
			{
#ifdef VI_MONGOC
				VI_ASSERT(Connected && Pool, "connection should be established");
				return Core::Cotask<ExpectsDB<void>>([this]() -> ExpectsDB<void>
				{
					if (Pool != nullptr)
					{
						mongoc_client_pool_destroy(Pool);
						Pool = nullptr;
					}

					SrcAddress.~Address();
					Connected = false;
					return Core::Expectation::Met;
				});
#else
				return ExpectsPromiseDB<void>(DatabaseException(0, "not supported"));
#endif
			}
			void Cluster::SetProfile(const std::string_view& Name)
			{
#ifdef VI_MONGOC
				VI_ASSERT(Pool != nullptr, "connection should be established");
				VI_ASSERT(Core::Stringify::IsCString(Name), "name should be set");
				mongoc_client_pool_set_appname(Pool, Name.data());
#endif
			}
			void Cluster::Push(Connection** Client)
			{
#ifdef VI_MONGOC
				VI_ASSERT(Client && *Client, "client should be set");
				mongoc_client_pool_push(Pool, (*Client)->Base);
				(*Client)->Base = nullptr;
				(*Client)->Connected = false;
				(*Client)->Master = nullptr;
				Core::Memory::Release(*Client);
#endif
			}
			Connection* Cluster::Pop()
			{
#ifdef VI_MONGOC
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

			bool Utils::GetId(uint8_t* Id12) noexcept
			{
#ifdef VI_MONGOC
				VI_ASSERT(Id12 != nullptr, "id should be set");
				bson_oid_t ObjectId;
				bson_oid_init(&ObjectId, nullptr);

				memcpy((void*)Id12, (void*)ObjectId.bytes, sizeof(char) * 12);
				return true;
#else
				return false;
#endif
			}
			bool Utils::GetDecimal(const std::string_view& Value, int64_t* High, int64_t* Low) noexcept
			{
				VI_ASSERT(Core::Stringify::IsCString(Value), "value should be set");
				VI_ASSERT(High != nullptr, "high should be set");
				VI_ASSERT(Low != nullptr, "low should be set");
#ifdef VI_MONGOC
				bson_decimal128_t Decimal;
				if (!bson_decimal128_from_string(Value.data(), &Decimal))
					return false;

				*High = Decimal.high;
				*Low = Decimal.low;
				return true;
#else
				return false;
#endif
			}
			uint32_t Utils::GetHashId(uint8_t* Id12) noexcept
			{
#ifdef VI_MONGOC
				VI_ASSERT(Id12 != nullptr, "id should be set");

				bson_oid_t Id;
				memcpy((void*)Id.bytes, (void*)Id12, sizeof(char) * 12);
				return bson_oid_hash(&Id);
#else
				return 0;
#endif
			}
			int64_t Utils::GetTimeId(uint8_t* Id12) noexcept
			{
#ifdef VI_MONGOC
				VI_ASSERT(Id12 != nullptr, "id should be set");

				bson_oid_t Id;
				memcpy((void*)Id.bytes, (void*)Id12, sizeof(char) * 12);

				return bson_oid_get_time_t(&Id);
#else
				return 0;
#endif
			}
			Core::String Utils::IdToString(uint8_t* Id12) noexcept
			{
#ifdef VI_MONGOC
				VI_ASSERT(Id12 != nullptr, "id should be set");

				bson_oid_t Id;
				memcpy(Id.bytes, Id12, sizeof(uint8_t) * 12);

				char Result[25];
				bson_oid_to_string(&Id, Result);

				return Core::String(Result, 24);
#else
				return "";
#endif
			}
			Core::String Utils::StringToId(const std::string_view& Id24) noexcept
			{
				VI_ASSERT(Id24.size() == 24, "id should be 24 chars long");
				VI_ASSERT(Core::Stringify::IsCString(Id24), "id should be set");
#ifdef VI_MONGOC
				bson_oid_t Id;
				bson_oid_init_from_string(&Id, Id24.data());

				return Core::String((const char*)Id.bytes, 12);
#else
				return "";
#endif
			}
			Core::String Utils::GetJSON(Core::Schema* Source, bool Escape) noexcept
			{
				VI_ASSERT(Source != nullptr, "source should be set");
				switch (Source->Value.GetType())
				{
					case Core::VarType::Object:
					{
						Core::String Result = "{";
						for (auto* Node : Source->GetChilds())
						{
							Result.append(1, '\"').append(Node->Key).append("\":");
							Result.append(GetJSON(Node, true)).append(1, ',');
						}

						if (!Source->Empty())
							Result = Result.substr(0, Result.size() - 1);

						return Result + "}";
					}
					case Core::VarType::Array:
					{
						Core::String Result = "[";
						for (auto* Node : Source->GetChilds())
							Result.append(GetJSON(Node, true)).append(1, ',');

						if (!Source->Empty())
							Result = Result.substr(0, Result.size() - 1);

						return Result + "]";
					}
					case Core::VarType::String:
					{
						Core::String Result = Source->Value.GetBlob();
						if (!Escape)
							return Result;

						Result.insert(Result.begin(), '\"');
						Result.append(1, '\"');
						return Result;
					}
					case Core::VarType::Integer:
						return Core::ToString(Source->Value.GetInteger());
					case Core::VarType::Number:
						return Core::ToString(Source->Value.GetNumber());
					case Core::VarType::Boolean:
						return Source->Value.GetBoolean() ? "true" : "false";
					case Core::VarType::Decimal:
						return "{\"$numberDouble\":\"" + Source->Value.GetDecimal().ToString() + "\"}";
					case Core::VarType::Binary:
					{
						if (Source->Value.Size() != 12)
						{
							Core::String Base = Compute::Codec::Bep45Encode(Source->Value.GetBlob());
							return "\"" + Base + "\"";
						}

						return "{\"$oid\":\"" + Utils::IdToString(Source->Value.GetBinary()) + "\"}";
					}
					case Core::VarType::Null:
					case Core::VarType::Undefined:
						return "null";
					default:
						break;
				}

				return "";
			}

			Driver::Sequence::Sequence() : Cache(nullptr)
			{
			}
			Driver::Sequence::Sequence(const Sequence& Other) : Positions(Other.Positions), Request(Other.Request), Cache(Other.Cache.Copy())
			{
			}

			Driver::Driver() noexcept : Logger(nullptr), APM(nullptr)
			{
#ifdef VI_MONGOC
				VI_TRACE("[mdb] OK initialize driver");
				mongoc_log_set_handler([](mongoc_log_level_t Level, const char* Domain, const char* Message, void*)
				{
					switch (Level)
					{
						case MONGOC_LOG_LEVEL_WARNING:
							VI_WARN("[mongoc] %s %s", Domain, Message);
							break;
						case MONGOC_LOG_LEVEL_INFO:
							VI_INFO("[mongoc] %s %s", Domain, Message);
							break;
						case MONGOC_LOG_LEVEL_ERROR:
						case MONGOC_LOG_LEVEL_CRITICAL:
							VI_ERR("[mongoc] %s %s", Domain, Message);
							break;
						case MONGOC_LOG_LEVEL_MESSAGE:
							VI_DEBUG("[mongoc] %s %s", Domain, Message);
							break;
						case MONGOC_LOG_LEVEL_TRACE:
							VI_TRACE("[mongoc] %s %s", Domain, Message);
							break;
						default:
							break;
					}
				}, nullptr);
				mongoc_init();
#endif
			}
			Driver::~Driver() noexcept
			{
#ifdef VI_MONGOC
				VI_TRACE("[mdb] cleanup driver");
				if (APM != nullptr)
				{
					mongoc_apm_callbacks_destroy((mongoc_apm_callbacks_t*)APM);
					APM = nullptr;
				}
				mongoc_cleanup();
#endif
			}
			void Driver::SetQueryLog(const OnQueryLog& Callback) noexcept
			{
				Logger = Callback;
				if (!Logger || APM)
					return;
#ifdef VI_MONGOC
				mongoc_apm_callbacks_t* Callbacks = mongoc_apm_callbacks_new();
				mongoc_apm_set_command_started_cb(Callbacks, [](const mongoc_apm_command_started_t* Event)
				{
					const char* Name = mongoc_apm_command_started_get_command_name(Event);
					char* Command = bson_as_relaxed_extended_json(mongoc_apm_command_started_get_command(Event), nullptr);
					Core::String Buffer = Core::Stringify::Text("%s:\n%s\n", Name, Command);
					bson_free(Command);

					auto* Base = Driver::Get();
					if (Base->Logger)
						Base->Logger(Buffer);
				});
				APM = (void*)Callbacks;
#endif
			}
			void Driver::AttachQueryLog(TConnection* Connection) noexcept
			{
#ifdef VI_MONGOC
				VI_ASSERT(Connection != nullptr, "connection should be set");
				mongoc_client_set_apm_callbacks(Connection, (mongoc_apm_callbacks_t*)APM, nullptr);
#endif
			}
			void Driver::AttachQueryLog(TConnectionPool* Connection) noexcept
			{
#ifdef VI_MONGOC
				VI_ASSERT(Connection != nullptr, "connection pool should be set");
				mongoc_client_pool_set_apm_callbacks(Connection, (mongoc_apm_callbacks_t*)APM, nullptr);
#endif
			}
			void Driver::AddConstant(const std::string_view& Name, const std::string_view& Value) noexcept
			{
				VI_ASSERT(!Name.empty(), "name should not be empty");
				Core::UMutex<std::mutex> Unique(Exclusive);
				Constants[Core::String(Name)] = Value;
			}
			ExpectsDB<void> Driver::AddQuery(const std::string_view& Name, const std::string_view& Buffer)
			{
				VI_ASSERT(!Name.empty(), "name should not be empty");
				if (Buffer.empty())
					return DatabaseException(0, "import empty query error: " + Core::String(Name));

				Sequence Result;
				Result.Request.assign(Buffer);

				Core::String Enums = " \r\n\t\'\"()<>=%&^*/+-,!?:;";
				Core::String Erasable = " \r\n\t\'\"()<>=%&^*/+-,.!?:;";
				Core::String Quotes = "\"'`";

				Core::String& Base = Result.Request;
				Core::Stringify::ReplaceInBetween(Base, "/*", "*/", "", false);
				Core::Stringify::Trim(Base);
				Core::Stringify::Compress(Base, Erasable.c_str(), Quotes.c_str());

				auto Enumerations = Core::Stringify::FindStartsWithEndsOf(Base, "#", Enums.c_str(), Quotes.c_str());
				if (!Enumerations.empty())
				{
					int64_t Offset = 0;
					Core::UMutex<std::mutex> Unique(Exclusive);
					for (auto& Item : Enumerations)
					{
						size_t Size = Item.second.End - Item.second.Start;
						Item.second.Start = (size_t)((int64_t)Item.second.Start + Offset);
						Item.second.End = (size_t)((int64_t)Item.second.End + Offset);

						auto It = Constants.find(Item.first);
						if (It == Constants.end())
							return DatabaseException(0, "query expects @" + Item.first + " constant: " + Core::String(Name));

						Core::Stringify::ReplacePart(Base, Item.second.Start, Item.second.End, It->second);
						size_t NewSize = It->second.size();
						Offset += (int64_t)NewSize - (int64_t)Size;
						Item.second.End = Item.second.Start + NewSize;
					}
				}

				Core::Vector<std::pair<Core::String, Core::TextSettle>> Variables;
				for (auto& Item : Core::Stringify::FindInBetween(Base, "$<", ">", Quotes.c_str()))
				{
					Item.first += ";escape";
					Variables.emplace_back(std::move(Item));
				}

				for (auto& Item : Core::Stringify::FindInBetween(Base, "@<", ">", Quotes.c_str()))
				{
					Item.first += ";unsafe";
					Variables.emplace_back(std::move(Item));
				}

				Core::Stringify::ReplaceParts(Base, Variables, "", [&Erasable](const std::string_view&, char Left, int)
				{
					return Erasable.find(Left) == Core::String::npos ? ' ' : '\0';
				});

				for (auto& Item : Variables)
				{
					Pose Position;
					Position.Escape = Item.first.find(";escape") != Core::String::npos;
					Position.Offset = Item.second.Start;
					Position.Key = Item.first.substr(0, Item.first.find(';'));
					Result.Positions.emplace_back(std::move(Position));
				}

				if (Variables.empty())
				{
					auto Subresult = Document::FromJSON(Result.Request);
					if (!Subresult)
						return Subresult.Error();
					Result.Cache = std::move(*Subresult);
				}

				Core::UMutex<std::mutex> Unique(Exclusive);
				Queries[Core::String(Name)] = std::move(Result);
				return Core::Expectation::Met;
			}
			ExpectsDB<void> Driver::AddDirectory(const std::string_view& Directory, const std::string_view& Origin)
			{
				Core::Vector<std::pair<Core::String, Core::FileEntry>> Entries;
				if (!Core::OS::Directory::Scan(Directory, Entries))
					return DatabaseException(0, "import directory scan error: " + Core::String(Directory));

				Core::String Path = Core::String(Directory);
				if (Path.back() != '/' && Path.back() != '\\')
					Path.append(1, '/');

				size_t Size = 0;
				for (auto& File : Entries)
				{
					Core::String Base(Path + File.first);
					if (File.second.IsDirectory)
					{
						auto Status = AddDirectory(Base, Origin.empty() ? Directory : Origin);
						if (!Status)
							return Status;
						else
							continue;
					}

					if (!Core::Stringify::EndsWith(Base, ".json"))
						continue;

					auto Buffer = Core::OS::File::ReadAll(Base, &Size);
					if (!Buffer)
						continue;

					Core::Stringify::Replace(Base, Origin.empty() ? Directory : Origin, "");
					Core::Stringify::Replace(Base, "\\", "/");
					Core::Stringify::Replace(Base, ".json", "");
					if (Core::Stringify::StartsOf(Base, "\\/"))
						Base.erase(0, 1);

					auto Status = AddQuery(Base, std::string_view((char*)*Buffer, Size));
					Core::Memory::Deallocate(*Buffer);
					if (!Status)
						return Status;
				}

				return Core::Expectation::Met;
			}
			bool Driver::RemoveConstant(const std::string_view& Name) noexcept
			{
				Core::UMutex<std::mutex> Unique(Exclusive);
				auto It = Constants.find(Core::KeyLookupCast(Name));
				if (It == Constants.end())
					return false;

				Constants.erase(It);
				return true;
			}
			bool Driver::RemoveQuery(const std::string_view& Name) noexcept
			{
				Core::UMutex<std::mutex> Unique(Exclusive);
				auto It = Queries.find(Core::KeyLookupCast(Name));
				if (It == Queries.end())
					return false;

				Queries.erase(It);
				return true;
			}
			bool Driver::LoadCacheDump(Core::Schema* Dump) noexcept
			{
				VI_ASSERT(Dump != nullptr, "dump should be set");
				size_t Count = 0;
				Core::UMutex<std::mutex> Unique(Exclusive);
				Queries.clear();

				for (auto* Data : Dump->GetChilds())
				{
					Sequence Result;
					Result.Request = Data->GetVar("request").GetBlob();

					auto* Cache = Data->Get("cache");
					if (Cache != nullptr && Cache->Value.IsObject())
					{
						Result.Cache = Document::FromSchema(Cache);
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

					Core::String Name = Data->GetVar("name").GetBlob();
					Queries[Name] = std::move(Result);
					++Count;
				}

				if (Count > 0)
					VI_DEBUG("[mdb] OK load %" PRIu64 " parsed query templates", (uint64_t)Count);

				return Count > 0;
			}
			Core::Schema* Driver::GetCacheDump() noexcept
			{
				Core::UMutex<std::mutex> Unique(Exclusive);
				Core::Schema* Result = Core::Var::Set::Array();
				for (auto& Query : Queries)
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

				VI_DEBUG("[mdb] OK save %" PRIu64 " parsed query templates", (uint64_t)Queries.size());
				return Result;
			}
			ExpectsDB<Document> Driver::GetQuery(const std::string_view& Name, Core::SchemaArgs* Map) noexcept
			{
				Core::UMutex<std::mutex> Unique(Exclusive);
				auto It = Queries.find(Core::KeyLookupCast(Name));
				if (It == Queries.end())
					return DatabaseException(0, "query not found: " + Core::String(Name));

				if (It->second.Cache.Get() != nullptr)
					return It->second.Cache.Copy();

				if (!Map || Map->empty())
					return Document::FromJSON(It->second.Request);

				Sequence Origin = It->second;
				size_t Offset = 0;
				Unique.Negate();

				Core::String& Result = Origin.Request;
				for (auto& Word : Origin.Positions)
				{
					auto It = Map->find(Word.Key);
					if (It == Map->end())
						return DatabaseException(0, "query expects @" + Word.Key + " constant: " + Core::String(Name));

					Core::String Value = Utils::GetJSON(*It->second, Word.Escape);
					if (Value.empty())
						continue;

					Result.insert(Word.Offset + Offset, Value);
					Offset += Value.size();
				}

				auto Data = Document::FromJSON(Origin.Request);
				if (!Data)
					return Data.Error();
				else if (!Data->Get())
					return DatabaseException(0, "query construction error: " + Core::String(Name));

				return Data;
			}
			Core::Vector<Core::String> Driver::GetQueries() noexcept
			{
				Core::Vector<Core::String> Result;
				Core::UMutex<std::mutex> Unique(Exclusive);
				Result.reserve(Queries.size());
				for (auto& Item : Queries)
					Result.push_back(Item.first);

				return Result;
			}
		}
	}
}
