#include "bindings.h"
#include "network.h"
#include "../engine/processors.h"
#include "../engine/components.h"
#include "../engine/renderers.h"
#include <new>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <angelscript.h>
#ifndef __psp2__
#include <locale.h>
#endif
#define ARRAY_CACHE 1000
#define MAP_CACHE 1003
#define TYPENAME_ARRAY "array"
#define TYPENAME_STRING "string"
#define TYPENAME_DICTIONARY "dictionary"
#define TYPENAME_ANY "any"
#define TYPENAME_CLOSURE "closure"
#define TYPENAME_THREAD "thread"
#define TYPENAME_REF "ref"
#define TYPENAME_WEAK "weak"
#define TYPENAME_CONSTWEAK "const_weak"
#define TYPENAME_PROMISE "promise"
#define TYPENAME_SCHEMA "schema"
#define TYPENAME_DECIMAL "decimal"
#define TYPENAME_VARIANT "variant"
#define TYPENAME_VERTEX "vertex"
#define TYPENAME_ELEMENTVERTEX "element_vertex"
#define TYPENAME_VECTOR3 "vector3"
#define TYPENAME_JOINT "joint"
#define TYPENAME_RECTANGLE "rectangle"
#define TYPENAME_VIEWPORT "viewport"
#define TYPENAME_ANIMATORKEY "animator_key"
#define TYPENAME_SKINANIMATORCLIP "skin_animator_clip"
#define TYPENAME_PHYSICSHULLSHAPE "physics_hull_shape"
#define TYPENAME_FILEENTRY "file_entry"
#define TYPENAME_REGEXMATCH "regex_match"
#define TYPENAME_INPUTLAYOUTATTRIBUTE "input_layout_attribute"
#define TYPENAME_REMOTEHOST "remote_host"
#define TYPENAME_SOCKETCERTIFICATE "socket_certificate"
#define TYPENAME_AUDIOCLIP "audio_clip"
#define TYPENAME_ELEMENTBUFFER "element_buffer"
#define TYPENAME_MESHBUFFER "mesh_buffer"
#define TYPENAME_SKINMESHBUFFER "skin_mesh_buffer"
#define TYPENAME_INSTANCEBUFFER "instance_buffer"
#define TYPENAME_TEXTURE2D "texture_2d"
#define TYPENAME_SHADER "shader"
#define TYPENAME_MODEL "model"
#define TYPENAME_SKINMODEL "skin_model"
#define TYPENAME_GRAPHICSDEVICE "graphics_device"
#define TYPENAME_SKINANIMATION "skin_animation"
#define TYPENAME_ASSETFILE "asset_file"
#define TYPENAME_MATERIAL "material"
#define TYPENAME_COMPONENT "base_component"
#define TYPENAME_ENTITY "scene_entity"
#define TYPENAME_SCENEGRAPH "scene_graph"
#define TYPENAME_ELEMENTNODE "ui_element"
#define TYPENAME_HTTPSERVER "http_server"

namespace
{
	class StringFactory : public asIStringFactory
	{
	private:
		static StringFactory* Base;

	public:
		Mavi::Core::UnorderedMap<Mavi::Core::String, int> Cache;

	public:
		StringFactory()
		{
		}
		~StringFactory()
		{
			VI_ASSERT_V(Cache.size() == 0, "some strings are still in use");
		}
		const void* GetStringConstant(const char* Buffer, asUINT Length)
		{
			VI_ASSERT(Buffer != nullptr, nullptr, "buffer must not be null");

			asAcquireExclusiveLock();
			Mavi::Core::String Source(Buffer, Length);
			auto It = Cache.find(Source);

			if (It == Cache.end())
				It = Cache.insert(std::make_pair(std::move(Source), 1)).first;
			else
				It->second++;

			asReleaseExclusiveLock();
			return reinterpret_cast<const void*>(&It->first);
		}
		int ReleaseStringConstant(const void* Source)
		{
			VI_ASSERT(Source != nullptr, asERROR, "source must not be null");

			asAcquireExclusiveLock();
			auto It = Cache.find(*reinterpret_cast<const Mavi::Core::String*>(Source));
			if (It == Cache.end())
			{
				asReleaseExclusiveLock();
				return asERROR;
			}

			It->second--;
			if (It->second <= 0)
				Cache.erase(It);

			asReleaseExclusiveLock();
			return asSUCCESS;
		}
		int GetRawStringData(const void* Source, char* Buffer, asUINT* Length) const
		{
			VI_ASSERT(Source != nullptr, asERROR, "source must not be null");
			if (Length != nullptr)
				*Length = (asUINT)reinterpret_cast<const Mavi::Core::String*>(Source)->length();

			if (Buffer != nullptr)
				memcpy(Buffer, reinterpret_cast<const Mavi::Core::String*>(Source)->c_str(), reinterpret_cast<const Mavi::Core::String*>(Source)->length());

			return asSUCCESS;
		}

	public:
		static StringFactory* Get()
		{
			if (!Base)
				Base = VI_NEW(StringFactory);

			return Base;
		}
		static void Free()
		{
			if (Base != nullptr && Base->Cache.empty())
			{
				VI_DELETE(StringFactory, Base);
				Base = nullptr;
			}
		}
	};
	StringFactory* StringFactory::Base = nullptr;
}

namespace Mavi
{
	namespace Scripting
	{
		namespace Bindings
		{
			void PointerToHandleCast(void* From, void** To, int TypeId)
			{
				if (!(TypeId & asTYPEID_OBJHANDLE))
					return;

				if (!(TypeId & asTYPEID_MASK_OBJECT))
					return;

				*To = From;
			}
			void HandleToHandleCast(void* From, void** To, int TypeId)
			{
				if (!(TypeId & asTYPEID_OBJHANDLE))
					return;

				if (!(TypeId & asTYPEID_MASK_OBJECT))
					return;

				asIScriptContext* Context = asGetActiveContext();
				if (!Context)
					return;

				asIScriptEngine* Engine = Context->GetEngine();
				if (!Engine)
					return;

				asITypeInfo* TypeInfo = Engine->GetTypeInfoById(TypeId);
				Engine->RefCastObject(From, TypeInfo, TypeInfo, To);
			}
			void* HandleToPointerCast(void* From, int TypeId)
			{
				if (!(TypeId & asTYPEID_OBJHANDLE))
					return nullptr;

				if (!(TypeId & asTYPEID_MASK_OBJECT))
					return nullptr;

				return *reinterpret_cast<void**>(From);
			}

			void String::Construct(Core::String* Current)
			{
				VI_ASSERT_V(Current != nullptr, "Current should be set");
				new(Current) Core::String();
			}
			void String::CopyConstruct(const Core::String& Other, Core::String* Current)
			{
				VI_ASSERT_V(Current != nullptr, "Current should be set");
				new(Current) Core::String(Other);
			}
			void String::Destruct(Core::String* Current)
			{
				VI_ASSERT_V(Current != nullptr, "Current should be set");
				Current->~basic_string();
			}
			Core::String& String::AddAssignTo(const Core::String& Current, Core::String& Dest)
			{
				Dest += Current;
				return Dest;
			}
			bool String::IsEmpty(const Core::String& Current)
			{
				return Current.empty();
			}
			void* String::ToPtr(const Core::String& Value)
			{
				return (void*)Value.c_str();
			}
			Core::String String::Reverse(const Core::String& Value)
			{
				Core::Stringify Result(Value);
				Result.Reverse();
				return Result.R();
			}
			Core::String& String::AssignUInt64To(as_uint64_t Value, Core::String& Dest)
			{
				Dest = Core::ToString(Value);
				return Dest;
			}
			Core::String& String::AddAssignUInt64To(as_uint64_t Value, Core::String& Dest)
			{
				Dest += Core::ToString(Value);
				return Dest;
			}
			Core::String String::AddUInt641(const Core::String& Current, as_uint64_t Value)
			{
				std::ostringstream Stream;
				Stream << Value;
				Stream << Current;
				return Core::Copy<Core::String>(Stream.str());
			}
			Core::String String::AddInt641(as_int64_t Value, const Core::String& Current)
			{
				std::ostringstream Stream;
				Stream << Current;
				Stream << Value;
				return Core::Copy<Core::String>(Stream.str());
			}
			Core::String& String::AssignInt64To(as_int64_t Value, Core::String& Dest)
			{
				Dest = Core::ToString(Value);
				return Dest;
			}
			Core::String& String::AddAssignInt64To(as_int64_t Value, Core::String& Dest)
			{
				Dest += Core::ToString(Value);
				return Dest;
			}
			Core::String String::AddInt642(const Core::String& Current, as_int64_t Value)
			{
				return Current + Core::ToString(Value);
			}
			Core::String String::AddUInt642(as_uint64_t Value, const Core::String& Current)
			{
				return Core::ToString(Value) + Current;
			}
			Core::String& String::AssignDoubleTo(double Value, Core::String& Dest)
			{
				Dest = Core::ToString(Value);
				return Dest;
			}
			Core::String& String::AddAssignDoubleTo(double Value, Core::String& Dest)
			{
				Dest += Core::ToString(Value);
				return Dest;
			}
			Core::String& String::AssignFloatTo(float Value, Core::String& Dest)
			{
				Dest = Core::ToString(Value);
				return Dest;
			}
			Core::String& String::AddAssignFloatTo(float Value, Core::String& Dest)
			{
				Dest += Core::ToString(Value);
				return Dest;
			}
			Core::String& String::AssignBoolTo(bool Value, Core::String& Dest)
			{
				Dest = (Value ? "true" : "false");
				return Dest;
			}
			Core::String& String::AddAssignBoolTo(bool Value, Core::String& Dest)
			{
				Dest += (Value ? "true" : "false");
				return Dest;
			}
			Core::String String::AddDouble1(const Core::String& Current, double Value)
			{
				return Current + Core::ToString(Value);
			}
			Core::String String::AddDouble2(double Value, const Core::String& Current)
			{
				return Core::ToString(Value) + Current;
			}
			Core::String String::AddFloat1(const Core::String& Current, float Value)
			{
				return Current + Core::ToString(Value);
			}
			Core::String String::AddFloat2(float Value, const Core::String& Current)
			{
				return Core::ToString(Value) + Current;
			}
			Core::String String::AddBool1(const Core::String& Current, bool Value)
			{
				return Current + (Value ? "true" : "false");
			}
			Core::String String::AddBool2(bool Value, const Core::String& Current)
			{
				return (Value ? "true" : "false") + Current;
			}
			char* String::CharAt(size_t Value, Core::String& Current)
			{
				if (Value >= Current.size())
				{
					asIScriptContext* Context = asGetActiveContext();
					if (Context != nullptr)
						Context->SetException("out of range");

					return 0;
				}

				return &Current[Value];
			}
			int String::Cmp(const Core::String& A, const Core::String& B)
			{
				int Result = 0;
				if (A < B)
					Result = -1;
				else if (A > B)
					Result = 1;

				return Result;
			}
			int String::FindFirst(const Core::String& Needle, size_t Start, const Core::String& Current)
			{
				return (int)Current.find(Needle, (size_t)Start);
			}
			int String::FindFirstOf(const Core::String& Needle, size_t Start, const Core::String& Current)
			{
				return (int)Current.find_first_of(Needle, (size_t)Start);
			}
			int String::FindLastOf(const Core::String& Needle, size_t Start, const Core::String& Current)
			{
				return (int)Current.find_last_of(Needle, (size_t)Start);
			}
			int String::FindFirstNotOf(const Core::String& Needle, size_t Start, const Core::String& Current)
			{
				return (int)Current.find_first_not_of(Needle, (size_t)Start);
			}
			int String::FindLastNotOf(const Core::String& Needle, size_t Start, const Core::String& Current)
			{
				return (int)Current.find_last_not_of(Needle, (size_t)Start);
			}
			int String::FindLast(const Core::String& Needle, int Start, const Core::String& Current)
			{
				return (int)Current.rfind(Needle, (size_t)Start);
			}
			void String::Insert(size_t Offset, const Core::String& Other, Core::String& Current)
			{
				Current.insert(Offset, Other);
			}
			void String::Erase(size_t Offset, int Count, Core::String& Current)
			{
				Current.erase(Offset, (size_t)(Count < 0 ? Core::String::npos : Count));
			}
			size_t String::Length(const Core::String& Current)
			{
				return (size_t)Current.length();
			}
			void String::Resize(size_t Size, Core::String& Current)
			{
				Current.resize(Size);
			}
			Core::String String::Replace(const Core::String& A, const Core::String& B, uint64_t Offset, const Core::String& Base)
			{
				return Mavi::Core::Stringify(Base).Replace(A, B, (size_t)Offset).R();
			}
			as_int64_t String::IntStore(const Core::String& Value, size_t Base, size_t* ByteCount)
			{
				if (Base != 10 && Base != 16)
				{
					if (ByteCount)
						*ByteCount = 0;

					return 0;
				}

				const char* End = &Value[0];
				bool Sign = false;
				if (*End == '-')
				{
					Sign = true;
					End++;
				}
				else if (*End == '+')
					End++;

				as_int64_t Result = 0;
				if (Base == 10)
				{
					while (*End >= '0' && *End <= '9')
					{
						Result *= 10;
						Result += *End++ - '0';
					}
				}
				else
				{
					while ((*End >= '0' && *End <= '9') || (*End >= 'a' && *End <= 'f') || (*End >= 'A' && *End <= 'F'))
					{
						Result *= 16;
						if (*End >= '0' && *End <= '9')
							Result += *End++ - '0';
						else if (*End >= 'a' && *End <= 'f')
							Result += *End++ - 'a' + 10;
						else if (*End >= 'A' && *End <= 'F')
							Result += *End++ - 'A' + 10;
					}
				}

				if (ByteCount)
					*ByteCount = size_t(size_t(End - Value.c_str()));

				if (Sign)
					Result = -Result;

				return Result;
			}
			as_uint64_t String::UIntStore(const Core::String& Value, size_t Base, size_t* ByteCount)
			{
				if (Base != 10 && Base != 16)
				{
					if (ByteCount)
						*ByteCount = 0;
					return 0;
				}

				const char* End = &Value[0];
				as_uint64_t Result = 0;

				if (Base == 10)
				{
					while (*End >= '0' && *End <= '9')
					{
						Result *= 10;
						Result += *End++ - '0';
					}
				}
				else
				{
					while ((*End >= '0' && *End <= '9') || (*End >= 'a' && *End <= 'f') || (*End >= 'A' && *End <= 'F'))
					{
						Result *= 16;
						if (*End >= '0' && *End <= '9')
							Result += *End++ - '0';
						else if (*End >= 'a' && *End <= 'f')
							Result += *End++ - 'a' + 10;
						else if (*End >= 'A' && *End <= 'F')
							Result += *End++ - 'A' + 10;
					}
				}

				if (ByteCount)
					*ByteCount = size_t(size_t(End - Value.c_str()));

				return Result;
			}
			double String::FloatStore(const Core::String& Value, size_t* ByteCount)
			{
				char* End;
#if !defined(_WIN32_WCE) && !defined(ANDROID) && !defined(__psp2__)
				char* Temp = setlocale(LC_NUMERIC, 0);
				Core::String Base = Temp ? Temp : "C";
				setlocale(LC_NUMERIC, "C");
#endif

				double res = strtod(Value.c_str(), &End);
#if !defined(_WIN32_WCE) && !defined(ANDROID) && !defined(__psp2__)
				setlocale(LC_NUMERIC, Base.c_str());
#endif
				if (ByteCount)
					*ByteCount = size_t(size_t(End - Value.c_str()));

				return res;
			}
			Core::String String::Sub(size_t Start, int Count, const Core::String& Current)
			{
				Core::String Result;
				if (Start < Current.length() && Count != 0)
					Result = Current.substr(Start, (size_t)(Count < 0 ? Core::String::npos : Count));

				return Result;
			}
			bool String::Equals(const Core::String& Left, const Core::String& Right)
			{
				return Left == Right;
			}
			Core::String String::ToLower(const Core::String& Symbol)
			{
				return Mavi::Core::Stringify(Symbol).ToLower().R();
			}
			Core::String String::ToUpper(const Core::String& Symbol)
			{
				return Mavi::Core::Stringify(Symbol).ToUpper().R();
			}
			Core::String String::ToInt8(char Value)
			{
				return Core::ToString(Value);
			}
			Core::String String::ToInt16(short Value)
			{
				return Core::ToString(Value);
			}
			Core::String String::ToInt(int Value)
			{
				return Core::ToString(Value);
			}
			Core::String String::ToInt64(int64_t Value)
			{
				return Core::ToString(Value);
			}
			Core::String String::ToUInt8(unsigned char Value)
			{
				return Core::ToString(Value);
			}
			Core::String String::ToUInt16(unsigned short Value)
			{
				return Core::ToString(Value);
			}
			Core::String String::ToUInt(unsigned int Value)
			{
				return Core::ToString(Value);
			}
			Core::String String::ToUInt64(uint64_t Value)
			{
				return Core::ToString(Value);
			}
			Core::String String::ToFloat(float Value)
			{
				return Core::ToString(Value);
			}
			Core::String String::ToDouble(double Value)
			{
				return Core::ToString(Value);
			}
			Core::String String::ToPointer(void* Value)
			{
				char* Buffer = (char*)Value;
				if (!Buffer)
					return Core::String();

				size_t Size = strlen(Buffer);
				return Size > 1024 * 1024 * 1024 ? Core::String() : Core::String(Buffer, Size);
			}
			Array* String::Split(const Core::String& Splitter, const Core::String& Current)
			{
				asIScriptContext* Context = asGetActiveContext();
				asIScriptEngine* Engine = Context->GetEngine();
				asITypeInfo* ArrayType = Engine->GetTypeInfoByDecl(TYPENAME_ARRAY "<" TYPENAME_STRING ">@");
				Array* Array = Array::Create(ArrayType);

				int Offset = 0, Prev = 0, Count = 0;
				while ((Offset = (int)Current.find(Splitter, Prev)) != (int)Core::String::npos)
				{
					Array->Resize(Array->GetSize() + 1);
					((Core::String*)Array->At(Count))->assign(&Current[Prev], Offset - Prev);
					Prev = Offset + (int)Splitter.length();
					Count++;
				}

				Array->Resize(Array->GetSize() + 1);
				((Core::String*)Array->At(Count))->assign(&Current[Prev]);
				return Array;
			}
			Core::String String::Join(const Array& Array, const Core::String& Splitter)
			{
				Core::String Current = "";
				if (!Array.GetSize())
					return Current;

				int i;
				for (i = 0; i < (int)Array.GetSize() - 1; i++)
				{
					Current += *(Core::String*)Array.At(i);
					Current += Splitter;
				}

				Current += *(Core::String*)Array.At(i);
				return Current;
			}
			char String::ToChar(const Core::String& Symbol)
			{
				return Symbol.empty() ? '\0' : Symbol[0];
			}

			float Math::FpFromIEEE(uint32_t Raw)
			{
				return *reinterpret_cast<float*>(&Raw);
			}
			uint32_t Math::FpToIEEE(float Value)
			{
				return *reinterpret_cast<uint32_t*>(&Value);
			}
			double Math::FpFromIEEE(as_uint64_t Raw)
			{
				return *reinterpret_cast<double*>(&Raw);
			}
			as_uint64_t Math::FpToIEEE(double Value)
			{
				return *reinterpret_cast<as_uint64_t*>(&Value);
			}
			bool Math::CloseTo(float A, float B, float Epsilon)
			{
				if (A == B)
					return true;

				float diff = fabsf(A - B);
				if ((A == 0 || B == 0) && (diff < Epsilon))
					return true;

				return diff / (fabs(A) + fabs(B)) < Epsilon;
			}
			bool Math::CloseTo(double A, double B, double Epsilon)
			{
				if (A == B)
					return true;

				double diff = fabs(A - B);
				if ((A == 0 || B == 0) && (diff < Epsilon))
					return true;

				return diff / (fabs(A) + fabs(B)) < Epsilon;
			}

			Exception::Pointer::Pointer() : Context(nullptr)
			{
			}
			Exception::Pointer::Pointer(asIScriptContext* NewContext) : Context(NewContext)
			{
				const char* Value = (Context ? Context->GetExceptionString() : nullptr);
				if (Value != nullptr && Value[0] != '\0')
				{
					LoadExceptionData(Context->GetExceptionString());
					Origin = LoadStackHere();
				}
			}
			Exception::Pointer::Pointer(const Core::String& Value) : Context(asGetActiveContext())
			{
				LoadExceptionData(Value);
				Origin = LoadStackHere();
			}
			Exception::Pointer::Pointer(const Core::String& NewType, const Core::String& NewMessage) : Type(NewType), Message(NewMessage), Context(asGetActiveContext())
			{
				Origin = LoadStackHere();
			}
			void Exception::Pointer::LoadExceptionData(const Core::String& Value)
			{
				size_t Offset = Value.find(':');
				if (Offset != std::string::npos)
				{
					Type = Value.substr(0, Offset);
					Message = Value.substr(Offset + 1);
				}
				else if (!Value.empty())
				{
					Type = "std::runtime_error";
					Message = Value;
				}
			}
			const Core::String& Exception::Pointer::GetType() const
			{
				return Type;
			}
			const Core::String& Exception::Pointer::GetMessage() const
			{
				return Message;
			}
			Core::String Exception::Pointer::ToExceptionString() const
			{
				return Empty() ? "" : Type + ":" + Message;
			}
			Core::String Exception::Pointer::What() const
			{
				Core::String Data = Type;
				if (!Message.empty())
				{
					Data.append(": ");
					Data.append(Message);
				}

				Data.append(" ");
				Data.append(Origin.empty() ? LoadStackHere() : Origin);
				return Data;
			}
			Core::String Exception::Pointer::LoadStackHere() const
			{
				Core::String Data;
				if (!Context)
					return Data;

				ImmediateContext* ThisContext = ImmediateContext::Get(Context);
				if (!ThisContext)
					return Data;

				asIScriptFunction* Function = Context->GetExceptionFunction();
				if (!Function)
					return Data;

				const char* Decl = Function->GetDeclaration();
				Data.append("at function ");
				Data.append(Decl ? Decl : "<any>");

				const char* Module = Function->GetModuleName();
				Data.append(", in module ");
				Data.append(Module ? Module : "<anonymous>");

				int LineNumber = Context->GetExceptionLineNumber();
				if (LineNumber > 0)
				{
					Data.append(":");
					Data.append(Core::ToString(LineNumber));
				}

				Data.append(", at location ");
				Data.append(Core::Form("0x%" PRIXPTR, Function).R());
				return Data;
			}
			bool Exception::Pointer::Empty() const
			{
				return Type.empty() && Message.empty();
			}

			void Exception::Throw(const Pointer& Data)
			{
				asIScriptContext* Context = asGetActiveContext();
				if (Context != nullptr)
					Context->SetException(Data.ToExceptionString().c_str());
			}
			void Exception::Rethrow()
			{
				asIScriptContext* Context = asGetActiveContext();
				if (Context != nullptr)
					Context->SetException(Context->GetExceptionString());
			}
			bool Exception::HasException()
			{
				asIScriptContext* Context = asGetActiveContext();
				if (!Context)
					return false;

				const char* Message = Context->GetExceptionString();
				return Message != nullptr && Message[0] != '\0';
			}
			Exception::Pointer Exception::GetException()
			{
				asIScriptContext* Context = asGetActiveContext();
				if (!Context)
					return Pointer();

				const char* Message = Context->GetExceptionString();
				if (!Message)
					return Pointer();

				return Pointer(Core::String(Message));
			}
			bool Exception::GeneratorCallback(const Core::String& Path, Core::String& Code)
			{
				FunctionFactory::ReplacePreconditions("throw", Code, [](const Core::String& Expression)
				{
					return "exception::throw(" + Expression + ")";
				});
				return true;
			}

			Any::Any(asIScriptEngine* _Engine) noexcept : RefCount(1), GCFlag(false), Engine(_Engine) 
			{
				Value.TypeId = 0;
				Value.Integer = 0;
				Engine->NotifyGarbageCollectorOfNewObject(this, Engine->GetTypeInfoByName(TYPENAME_ANY));
			}
			Any::Any(void* Ref, int RefTypeId, asIScriptEngine* _Engine) noexcept : Any(_Engine)
			{
				Store(Ref, RefTypeId);
			}
			Any::Any(const Any& Other) noexcept : Any(Other.Engine)
			{
				if ((Other.Value.TypeId & asTYPEID_MASK_OBJECT))
				{
					asITypeInfo* Info = Engine->GetTypeInfoById(Other.Value.TypeId);
					if (Info != nullptr)
						Info->AddRef();
				}

				Value.TypeId = Other.Value.TypeId;
				if (Value.TypeId & asTYPEID_OBJHANDLE)
				{
					Value.Object = Other.Value.Object;
					Engine->AddRefScriptObject(Value.Object, Engine->GetTypeInfoById(Value.TypeId));
				}
				else if (Value.TypeId & asTYPEID_MASK_OBJECT)
					Value.Object = Engine->CreateScriptObjectCopy(Other.Value.Object, Engine->GetTypeInfoById(Value.TypeId));
				else
					Value.Integer = Other.Value.Integer;
			}
			Any::~Any() noexcept
			{
				FreeObject();
			}
			Any& Any::operator=(const Any& Other) noexcept
			{
				if ((Other.Value.TypeId & asTYPEID_MASK_OBJECT))
				{
					asITypeInfo* Info = Engine->GetTypeInfoById(Other.Value.TypeId);
					if (Info != nullptr)
						Info->AddRef();
				}

				FreeObject();
				Value.TypeId = Other.Value.TypeId;

				if (Value.TypeId & asTYPEID_OBJHANDLE)
				{
					Value.Object = Other.Value.Object;
					Engine->AddRefScriptObject(Value.Object, Engine->GetTypeInfoById(Value.TypeId));
				}
				else if (Value.TypeId & asTYPEID_MASK_OBJECT)
					Value.Object = Engine->CreateScriptObjectCopy(Other.Value.Object, Engine->GetTypeInfoById(Value.TypeId));
				else
					Value.Integer = Other.Value.Integer;

				return *this;
			}
			int Any::CopyFrom(const Any* Other)
			{
				if (Other == 0)
					return asINVALID_ARG;

				*this = *(Any*)Other;
				return 0;
			}
			void Any::Store(void* Ref, int RefTypeId)
			{
				if ((RefTypeId & asTYPEID_MASK_OBJECT))
				{
					asITypeInfo* T = Engine->GetTypeInfoById(RefTypeId);
					if (T)
						T->AddRef();
				}

				FreeObject();
				Value.TypeId = RefTypeId;

				if (Value.TypeId & asTYPEID_OBJHANDLE)
				{
					Value.Object = *(void**)Ref;
					Engine->AddRefScriptObject(Value.Object, Engine->GetTypeInfoById(Value.TypeId));
				}
				else if (Value.TypeId & asTYPEID_MASK_OBJECT)
				{
					Value.Object = Engine->CreateScriptObjectCopy(Ref, Engine->GetTypeInfoById(Value.TypeId));
				}
				else
				{
					Value.Integer = 0;
					int Size = Engine->GetSizeOfPrimitiveType(Value.TypeId);
					memcpy(&Value.Integer, Ref, Size);
				}
			}
			bool Any::Retrieve(void* Ref, int RefTypeId) const
			{
				if (RefTypeId & asTYPEID_OBJHANDLE)
				{
					if ((Value.TypeId & asTYPEID_MASK_OBJECT))
					{
						if ((Value.TypeId & asTYPEID_HANDLETOCONST) && !(RefTypeId & asTYPEID_HANDLETOCONST))
							return false;

						Engine->RefCastObject(Value.Object, Engine->GetTypeInfoById(Value.TypeId), Engine->GetTypeInfoById(RefTypeId), reinterpret_cast<void**>(Ref));
						if (*(asPWORD*)Ref == 0)
							return false;

						return true;
					}
				}
				else if (RefTypeId & asTYPEID_MASK_OBJECT)
				{
					if (Value.TypeId == RefTypeId)
					{
						Engine->AssignScriptObject(Ref, Value.Object, Engine->GetTypeInfoById(Value.TypeId));
						return true;
					}
				}
				else
				{
					int Size1 = Engine->GetSizeOfPrimitiveType(Value.TypeId);
					int Size2 = Engine->GetSizeOfPrimitiveType(RefTypeId);
					if (Size1 == Size2)
					{
						memcpy(Ref, &Value.Integer, Size1);
						return true;
					}
				}

				return false;
			}
			void* Any::GetAddressOfObject()
			{
				if (Value.TypeId & asTYPEID_OBJHANDLE)
					return &Value.Object;
				else if (Value.TypeId & asTYPEID_MASK_OBJECT)
					return Value.Object;
				else if (Value.TypeId <= asTYPEID_DOUBLE || Value.TypeId & asTYPEID_MASK_SEQNBR)
					return &Value.Integer;

				return nullptr;
			}
			int Any::GetTypeId() const
			{
				return Value.TypeId;
			}
			void Any::FreeObject()
			{
				if (Value.TypeId & asTYPEID_MASK_OBJECT)
				{
					asITypeInfo* T = Engine->GetTypeInfoById(Value.TypeId);
					Engine->ReleaseScriptObject(Value.Object, T);

					if (T)
						T->Release();

					Value.Object = 0;
					Value.TypeId = 0;
				}
			}
			void Any::EnumReferences(asIScriptEngine* InEngine)
			{
				if (Value.Object && (Value.TypeId & asTYPEID_MASK_OBJECT))
				{
					asITypeInfo* SubType = Engine->GetTypeInfoById(Value.TypeId);
					if ((SubType->GetFlags() & asOBJ_REF))
						InEngine->GCEnumCallback(Value.Object);
					else if ((SubType->GetFlags() & asOBJ_VALUE) && (SubType->GetFlags() & asOBJ_GC))
						Engine->ForwardGCEnumReferences(Value.Object, SubType);

					asITypeInfo* T = InEngine->GetTypeInfoById(Value.TypeId);
					if (T != nullptr)
						InEngine->GCEnumCallback(T);
				}
			}
			void Any::ReleaseAllHandles(asIScriptEngine*)
			{
				FreeObject();
			}
			int Any::AddRef() const
			{
				GCFlag = false;
				return asAtomicInc(RefCount);
			}
			int Any::Release() const
			{
				GCFlag = false;
				if (asAtomicDec(RefCount) == 0)
				{
					this->~Any();
					asFreeMem((void*)this);
					return 0;
				}

				return RefCount;
			}
			int Any::GetRefCount()
			{
				return RefCount;
			}
			void Any::SetFlag()
			{
				GCFlag = true;
			}
			bool Any::GetFlag()
			{
				return GCFlag;
			}
			Core::Unique<Any> Any::Create()
			{
				asIScriptContext* Context = asGetActiveContext();
				if (!Context)
					return nullptr;

				void* Data = asAllocMem(sizeof(Any));
				return new(Data) Any(Context->GetEngine());
			}
			Any* Any::Create(int TypeId, void* Ref)
			{
				asIScriptContext* Context = asGetActiveContext();
				if (!Context)
					return nullptr;

				void* Data = asAllocMem(sizeof(Any));
				return new(Data) Any(Ref, TypeId, Context->GetEngine());
			}
			Any* Any::Create(const char* Decl, void* Ref)
			{
				asIScriptContext* Context = asGetActiveContext();
				if (!Context)
					return nullptr;

				asIScriptEngine* Engine = Context->GetEngine();
				void* Data = asAllocMem(sizeof(Any));
				return new(Data) Any(Ref, Engine->GetTypeIdByDecl(Decl), Engine);
			}
			void Any::Factory1(asIScriptGeneric* G)
			{
				asIScriptEngine* Engine = G->GetEngine();
				void* Mem = asAllocMem(sizeof(Any));
				*(Any**)G->GetAddressOfReturnLocation() = new(Mem) Any(Engine);
			}
			void Any::Factory2(asIScriptGeneric* G)
			{
				asIScriptEngine* Engine = G->GetEngine();
				void* Ref = (void*)G->GetArgAddress(0);
				void* Mem = asAllocMem(sizeof(Any));
				int RefType = G->GetArgTypeId(0);

				*(Any**)G->GetAddressOfReturnLocation() = new(Mem) Any(Ref, RefType, Engine);
			}
			Any& Any::Assignment(Any* Other, Any* Self)
			{
				return *Self = *Other;
			}

			Array::Array(asITypeInfo* Info, void* BufferPtr) noexcept : RefCount(1), GCFlag(false), ObjType(Info), Buffer(nullptr), ElementSize(0), SubTypeId(-1)
			{
				VI_ASSERT_V(Info && Core::String(Info->GetName()) == TYPENAME_ARRAY, "array type is invalid");
				ObjType->AddRef();
				Precache();

				asIScriptEngine* Engine = Info->GetEngine();
				if (SubTypeId & asTYPEID_MASK_OBJECT)
					ElementSize = sizeof(asPWORD);
				else
					ElementSize = Engine->GetSizeOfPrimitiveType(SubTypeId);

				size_t Length = *(asUINT*)BufferPtr;
				if (!CheckMaxSize(Length))
					return;

				if ((Info->GetSubTypeId() & asTYPEID_MASK_OBJECT) == 0)
				{
					CreateBuffer(&Buffer, Length);
					if (Length > 0)
						memcpy(At(0), (((asUINT*)BufferPtr) + 1), (size_t)Length * (size_t)ElementSize);
				}
				else if (Info->GetSubTypeId() & asTYPEID_OBJHANDLE)
				{
					CreateBuffer(&Buffer, Length);
					if (Length > 0)
						memcpy(At(0), (((asUINT*)BufferPtr) + 1), (size_t)Length * (size_t)ElementSize);

					memset((((asUINT*)BufferPtr) + 1), 0, (size_t)Length * (size_t)ElementSize);
				}
				else if (Info->GetSubType()->GetFlags() & asOBJ_REF)
				{
					SubTypeId |= asTYPEID_OBJHANDLE;
					CreateBuffer(&Buffer, Length);
					SubTypeId &= ~asTYPEID_OBJHANDLE;

					if (Length > 0)
						memcpy(Buffer->Data, (((asUINT*)BufferPtr) + 1), (size_t)Length * (size_t)ElementSize);

					memset((((asUINT*)BufferPtr) + 1), 0, (size_t)Length * (size_t)ElementSize);
				}
				else
				{
					CreateBuffer(&Buffer, Length);
					for (size_t n = 0; n < Length; n++)
					{
						unsigned char* SourceObj = (unsigned char*)BufferPtr;
						SourceObj += 4 + n * Info->GetSubType()->GetSize();
						Engine->AssignScriptObject(At(n), SourceObj, Info->GetSubType());
					}
				}

				if (ObjType->GetFlags() & asOBJ_GC)
					ObjType->GetEngine()->NotifyGarbageCollectorOfNewObject(this, ObjType);
			}
			Array::Array(size_t Length, asITypeInfo* Info) noexcept : RefCount(1), GCFlag(false), ObjType(Info), Buffer(nullptr), ElementSize(0), SubTypeId(-1)
			{
				VI_ASSERT_V(Info && Core::String(Info->GetName()) == TYPENAME_ARRAY, "array type is invalid");
				ObjType->AddRef();
				Precache();

				if (SubTypeId & asTYPEID_MASK_OBJECT)
					ElementSize = sizeof(asPWORD);
				else
					ElementSize = ObjType->GetEngine()->GetSizeOfPrimitiveType(SubTypeId);

				if (!CheckMaxSize(Length))
					return;

				CreateBuffer(&Buffer, Length);
				if (ObjType->GetFlags() & asOBJ_GC)
					ObjType->GetEngine()->NotifyGarbageCollectorOfNewObject(this, ObjType);
			}
			Array::Array(const Array& Other) noexcept : RefCount(1), GCFlag(false), ObjType(Other.ObjType), Buffer(nullptr), ElementSize(0), SubTypeId(-1)
			{
				VI_ASSERT_V(ObjType && Core::String(ObjType->GetName()) == TYPENAME_ARRAY, "array type is invalid");
				ObjType->AddRef();
				Precache();

				ElementSize = Other.ElementSize;
				if (ObjType->GetFlags() & asOBJ_GC)
					ObjType->GetEngine()->NotifyGarbageCollectorOfNewObject(this, ObjType);

				CreateBuffer(&Buffer, 0);
				*this = Other;
			}
			Array::Array(size_t Length, void* DefaultValue, asITypeInfo* Info) noexcept : RefCount(1), GCFlag(false), ObjType(Info), Buffer(nullptr), ElementSize(0), SubTypeId(-1)
			{
				VI_ASSERT_V(Info && Core::String(Info->GetName()) == TYPENAME_ARRAY, "array type is invalid");
				ObjType->AddRef();
				Precache();

				if (SubTypeId & asTYPEID_MASK_OBJECT)
					ElementSize = sizeof(asPWORD);
				else
					ElementSize = ObjType->GetEngine()->GetSizeOfPrimitiveType(SubTypeId);

				if (!CheckMaxSize(Length))
					return;

				CreateBuffer(&Buffer, Length);
				if (ObjType->GetFlags() & asOBJ_GC)
					ObjType->GetEngine()->NotifyGarbageCollectorOfNewObject(this, ObjType);

				for (size_t i = 0; i < GetSize(); i++)
					SetValue(i, DefaultValue);
			}
			Array::~Array() noexcept
			{
				if (Buffer)
				{
					DeleteBuffer(Buffer);
					Buffer = nullptr;
				}
				if (ObjType)
					ObjType->Release();
			}
			Array& Array::operator=(const Array& Other) noexcept
			{
				if (&Other != this && Other.GetArrayObjectType() == GetArrayObjectType())
				{
					Resize(Other.Buffer->NumElements);
					CopyBuffer(Buffer, Other.Buffer);
				}

				return *this;
			}
			void Array::SetValue(size_t Index, void* Value)
			{
				void* Ptr = At(Index);
				if (Ptr == 0)
					return;

				if ((SubTypeId & ~asTYPEID_MASK_SEQNBR) && !(SubTypeId & asTYPEID_OBJHANDLE))
					ObjType->GetEngine()->AssignScriptObject(Ptr, Value, ObjType->GetSubType());
				else if (SubTypeId & asTYPEID_OBJHANDLE)
				{
					void* Swap = *(void**)Ptr;
					*(void**)Ptr = *(void**)Value;
					ObjType->GetEngine()->AddRefScriptObject(*(void**)Value, ObjType->GetSubType());
					if (Swap)
						ObjType->GetEngine()->ReleaseScriptObject(Swap, ObjType->GetSubType());
				}
				else if (SubTypeId == asTYPEID_BOOL || SubTypeId == asTYPEID_INT8 || SubTypeId == asTYPEID_UINT8)
					*(char*)Ptr = *(char*)Value;
				else if (SubTypeId == asTYPEID_INT16 || SubTypeId == asTYPEID_UINT16)
					*(short*)Ptr = *(short*)Value;
				else if (SubTypeId == asTYPEID_INT32 || SubTypeId == asTYPEID_UINT32 || SubTypeId == asTYPEID_FLOAT || SubTypeId > asTYPEID_DOUBLE)
					*(int*)Ptr = *(int*)Value;
				else if (SubTypeId == asTYPEID_INT64 || SubTypeId == asTYPEID_UINT64 || SubTypeId == asTYPEID_DOUBLE)
					*(double*)Ptr = *(double*)Value;
			}
			size_t Array::GetSize() const
			{
				return Buffer->NumElements;
			}
			bool Array::IsEmpty() const
			{
				return Buffer->NumElements == 0;
			}
			void Array::Reserve(size_t MaxElements)
			{
				if (MaxElements <= Buffer->MaxElements)
					return;

				if (!CheckMaxSize(MaxElements))
					return;

				SBuffer* NewBuffer = reinterpret_cast<SBuffer*>(asAllocMem(sizeof(SBuffer) - 1 + (size_t)ElementSize * (size_t)MaxElements));
				if (NewBuffer)
				{
					NewBuffer->NumElements = Buffer->NumElements;
					NewBuffer->MaxElements = MaxElements;
				}
				else
				{
					asIScriptContext* Context = asGetActiveContext();
					if (Context)
						Context->SetException("out of memory");
					return;
				}

				memcpy(NewBuffer->Data, Buffer->Data, (size_t)Buffer->NumElements * (size_t)ElementSize);
				asFreeMem(Buffer);
				Buffer = NewBuffer;
			}
			void Array::Resize(size_t NumElements)
			{
				if (!CheckMaxSize(NumElements))
					return;

				Resize((int)NumElements - (int)Buffer->NumElements, (size_t)-1);
			}
			void Array::RemoveRange(size_t Start, size_t Count)
			{
				if (Count == 0)
					return;

				if (Buffer == 0 || Start > Buffer->NumElements)
				{
					asIScriptContext* Context = asGetActiveContext();
					if (Context)
						Context->SetException("index out of bounds");
					return;
				}

				if (Start + Count > Buffer->NumElements)
					Count = Buffer->NumElements - Start;

				Destruct(Buffer, Start, Start + Count);
				memmove(Buffer->Data + Start * (size_t)ElementSize, Buffer->Data + (Start + Count) * (size_t)ElementSize, (size_t)(Buffer->NumElements - Start - Count) * (size_t)ElementSize);
				Buffer->NumElements -= Count;
			}
			void Array::Resize(int Delta, size_t Where)
			{
				if (Delta < 0)
				{
					if (-Delta > (int)Buffer->NumElements)
						Delta = -(int)Buffer->NumElements;

					if (Where > Buffer->NumElements + Delta)
						Where = Buffer->NumElements + Delta;
				}
				else if (Delta > 0)
				{
					if (!CheckMaxSize(Buffer->NumElements + Delta))
						return;

					if (Where > Buffer->NumElements)
						Where = Buffer->NumElements;
				}

				if (Delta == 0)
					return;

				if (Buffer->MaxElements < Buffer->NumElements + Delta)
				{
					size_t Count = (size_t)Buffer->NumElements + (size_t)Delta, Size = (size_t)ElementSize;
					SBuffer* NewBuffer = reinterpret_cast<SBuffer*>(asAllocMem(sizeof(SBuffer) - 1 + Size * Count));
					if (NewBuffer)
					{
						NewBuffer->NumElements = Buffer->NumElements + Delta;
						NewBuffer->MaxElements = NewBuffer->NumElements;
					}
					else
					{
						asIScriptContext* Context = asGetActiveContext();
						if (Context)
							Context->SetException("out of memory");
						return;
					}

					memcpy(NewBuffer->Data, Buffer->Data, (size_t)Where * (size_t)ElementSize);
					if (Where < Buffer->NumElements)
						memcpy(NewBuffer->Data + (Where + Delta) * (size_t)ElementSize, Buffer->Data + Where * (size_t)ElementSize, (size_t)(Buffer->NumElements - Where) * (size_t)ElementSize);

					Construct(NewBuffer, Where, Where + Delta);
					asFreeMem(Buffer);
					Buffer = NewBuffer;
				}
				else if (Delta < 0)
				{
					Destruct(Buffer, Where, Where - Delta);
					memmove(Buffer->Data + Where * (size_t)ElementSize, Buffer->Data + (Where - Delta) * (size_t)ElementSize, (size_t)(Buffer->NumElements - (Where - Delta)) * (size_t)ElementSize);
					Buffer->NumElements += Delta;
				}
				else
				{
					memmove(Buffer->Data + (Where + Delta) * (size_t)ElementSize, Buffer->Data + Where * (size_t)ElementSize, (size_t)(Buffer->NumElements - Where) * (size_t)ElementSize);
					Construct(Buffer, Where, Where + Delta);
					Buffer->NumElements += Delta;
				}
			}
			bool Array::CheckMaxSize(size_t NumElements)
			{
				size_t MaxSize = 0xFFFFFFFFul - sizeof(SBuffer) + 1;
				if (ElementSize > 0)
					MaxSize /= (size_t)ElementSize;

				if (NumElements <= MaxSize)
					return true;

				asIScriptContext* Context = asGetActiveContext();
				if (Context)
					Context->SetException("too large array size");

				return false;
			}
			asITypeInfo* Array::GetArrayObjectType() const
			{
				return ObjType;
			}
			int Array::GetArrayTypeId() const
			{
				return ObjType->GetTypeId();
			}
			int Array::GetElementTypeId() const
			{
				return SubTypeId;
			}
			void Array::InsertAt(size_t Index, void* Value)
			{
				if (Index > Buffer->NumElements)
				{
					asIScriptContext* Context = asGetActiveContext();
					if (Context)
						Context->SetException("index out of bounds");
				}
				else
				{
					Resize(1, Index);
					SetValue(Index, Value);
				}
			}
			void Array::InsertAt(size_t Index, const Array& Array)
			{
				if (Index > Buffer->NumElements)
				{
					asIScriptContext* Context = asGetActiveContext();
					if (Context)
						Context->SetException("index out of bounds");
					return;
				}

				if (ObjType != Array.ObjType)
				{
					asIScriptContext* Context = asGetActiveContext();
					if (Context)
						Context->SetException("mismatching array types");
					return;
				}

				size_t NewSize = Array.GetSize();
				Resize((int)NewSize, Index);

				if (&Array != this)
				{
					for (size_t i = 0; i < Array.GetSize(); i++)
					{
						void* Value = const_cast<void*>(Array.At(i));
						SetValue(Index + i, Value);
					}
				}
				else
				{
					for (size_t i = 0; i < Index; i++)
					{
						void* Value = const_cast<void*>(Array.At(i));
						SetValue(Index + i, Value);
					}

					for (size_t i = Index + NewSize, k = 0; i < Array.GetSize(); i++, k++)
					{
						void* Value = const_cast<void*>(Array.At(i));
						SetValue(Index + Index + k, Value);
					}
				}
			}
			void Array::InsertLast(void* Value)
			{
				InsertAt(Buffer->NumElements, Value);
			}
			void Array::RemoveAt(size_t Index)
			{
				if (Index >= Buffer->NumElements)
				{
					asIScriptContext* Context = asGetActiveContext();
					if (Context)
						Context->SetException("index out of bounds");
				}
				else
					Resize(-1, Index);
			}
			void Array::RemoveLast()
			{
				RemoveAt(Buffer->NumElements - 1);
			}
			const void* Array::At(size_t Index) const
			{
				if (Buffer == 0 || Index >= Buffer->NumElements)
				{
					asIScriptContext* Context = asGetActiveContext();
					if (Context)
						Context->SetException("index out of bounds");

					return nullptr;
				}
				else if ((SubTypeId & asTYPEID_MASK_OBJECT) && !(SubTypeId & asTYPEID_OBJHANDLE))
					return *(void**)(Buffer->Data + (size_t)ElementSize * Index);

				return Buffer->Data + (size_t)ElementSize * Index;
			}
			void* Array::At(size_t Index)
			{
				return const_cast<void*>(const_cast<const Array*>(this)->At(Index));
			}
			void* Array::GetBuffer()
			{
				return Buffer->Data;
			}
			void Array::CreateBuffer(SBuffer** BufferPtr, size_t NumElements)
			{
				*BufferPtr = reinterpret_cast<SBuffer*>(asAllocMem(sizeof(SBuffer) - 1 + (size_t)ElementSize * (size_t)NumElements));
				if (*BufferPtr)
				{
					(*BufferPtr)->NumElements = NumElements;
					(*BufferPtr)->MaxElements = NumElements;
					Construct(*BufferPtr, 0, NumElements);
				}
				else
				{
					asIScriptContext* Context = asGetActiveContext();
					if (Context)
						Context->SetException("out of memory");
				}
			}
			void Array::DeleteBuffer(SBuffer* BufferPtr)
			{
				Destruct(BufferPtr, 0, BufferPtr->NumElements);
				asFreeMem(BufferPtr);
			}
			void Array::Construct(SBuffer* BufferPtr, size_t Start, size_t End)
			{
				if ((SubTypeId & asTYPEID_MASK_OBJECT) && !(SubTypeId & asTYPEID_OBJHANDLE))
				{
					void** Max = (void**)(BufferPtr->Data + End * sizeof(void*));
					void** D = (void**)(BufferPtr->Data + Start * sizeof(void*));

					asIScriptEngine* Engine = ObjType->GetEngine();
					asITypeInfo* SubType = ObjType->GetSubType();

					for (; D < Max; D++)
					{
						*D = (void*)Engine->CreateScriptObject(SubType);
						if (*D == 0)
						{
							memset(D, 0, sizeof(void*) * (Max - D));
							return;
						}
					}
				}
				else
				{
					void* D = (void*)(BufferPtr->Data + Start * (size_t)ElementSize);
					memset(D, 0, (size_t)(End - Start) * (size_t)ElementSize);
				}
			}
			void Array::Destruct(SBuffer* BufferPtr, size_t Start, size_t End)
			{
				if (SubTypeId & asTYPEID_MASK_OBJECT)
				{
					asIScriptEngine* Engine = ObjType->GetEngine();
					void** Max = (void**)(BufferPtr->Data + End * sizeof(void*));
					void** D = (void**)(BufferPtr->Data + Start * sizeof(void*));

					for (; D < Max; D++)
					{
						if (*D)
							Engine->ReleaseScriptObject(*D, ObjType->GetSubType());
					}
				}
			}
			bool Array::Less(const void* A, const void* B, bool Asc, asIScriptContext* Context, SCache* Cache)
			{
				if (!Asc)
				{
					const void* Temp = A;
					A = B;
					B = Temp;
				}

				if (!(SubTypeId & ~asTYPEID_MASK_SEQNBR))
				{
					switch (SubTypeId)
					{
#define COMPARE(T) *((T*)A) < *((T*)B)
						case asTYPEID_BOOL: return COMPARE(bool);
						case asTYPEID_INT8: return COMPARE(signed char);
						case asTYPEID_UINT8: return COMPARE(unsigned char);
						case asTYPEID_INT16: return COMPARE(signed short);
						case asTYPEID_UINT16: return COMPARE(unsigned short);
						case asTYPEID_INT32: return COMPARE(signed int);
						case asTYPEID_UINT32: return COMPARE(unsigned int);
						case asTYPEID_FLOAT: return COMPARE(float);
						case asTYPEID_DOUBLE: return COMPARE(double);
						default: return COMPARE(signed int);
#undef COMPARE
					}
				}
				else
				{
					if (SubTypeId & asTYPEID_OBJHANDLE)
					{
						if (*(void**)A == 0)
							return true;

						if (*(void**)B == 0)
							return false;
					}

					if (Cache && Cache->CmpFunc)
					{
						Context->Prepare(Cache->CmpFunc);
						if (SubTypeId & asTYPEID_OBJHANDLE)
						{
							Context->SetObject(*((void**)A));
							Context->SetArgObject(0, *((void**)B));
						}
						else
						{
							Context->SetObject((void*)A);
							Context->SetArgObject(0, (void*)B);
						}

						if (ImmediateContext::Get(Context)->ExecuteNext() == asEXECUTION_FINISHED)
							return (int)Context->GetReturnDWord() < 0;
					}
				}

				return false;
			}
			void Array::Reverse()
			{
				size_t Size = GetSize();
				if (Size >= 2)
				{
					unsigned char Temp[16];
					for (size_t i = 0; i < Size / 2; i++)
					{
						Copy(Temp, GetArrayItemPointer((int)i));
						Copy(GetArrayItemPointer((int)i), GetArrayItemPointer((int)(Size - i - 1)));
						Copy(GetArrayItemPointer((int)(Size - i - 1)), Temp);
					}
				}
			}
			bool Array::operator==(const Array& Other) const
			{
				if (ObjType != Other.ObjType)
					return false;

				if (GetSize() != Other.GetSize())
					return false;

				asIScriptContext* CmpContext = 0;
				bool IsNested = false;

				if (SubTypeId & ~asTYPEID_MASK_SEQNBR)
				{
					CmpContext = asGetActiveContext();
					if (CmpContext)
					{
						if (CmpContext->GetEngine() == ObjType->GetEngine() && CmpContext->PushState() >= 0)
							IsNested = true;
						else
							CmpContext = 0;
					}

					if (CmpContext == 0)
						CmpContext = ObjType->GetEngine()->CreateContext();
				}

				bool IsEqual = true;
				SCache* Cache = reinterpret_cast<SCache*>(ObjType->GetUserData(ARRAY_CACHE));
				for (size_t n = 0; n < GetSize(); n++)
				{
					if (!Equals(At(n), Other.At(n), CmpContext, Cache))
					{
						IsEqual = false;
						break;
					}
				}

				if (CmpContext)
				{
					if (IsNested)
					{
						asEContextState State = CmpContext->GetState();
						CmpContext->PopState();
						if (State == asEXECUTION_ABORTED)
							CmpContext->Abort();
					}
					else
						CmpContext->Release();
				}

				return IsEqual;
			}
			bool Array::Equals(const void* A, const void* B, asIScriptContext* Context, SCache* Cache) const
			{
				if (!(SubTypeId & ~asTYPEID_MASK_SEQNBR))
				{
					switch (SubTypeId)
					{
#define COMPARE(T) *((T*)A) == *((T*)B)
						case asTYPEID_BOOL: return COMPARE(bool);
						case asTYPEID_INT8: return COMPARE(signed char);
						case asTYPEID_UINT8: return COMPARE(unsigned char);
						case asTYPEID_INT16: return COMPARE(signed short);
						case asTYPEID_UINT16: return COMPARE(unsigned short);
						case asTYPEID_INT32: return COMPARE(signed int);
						case asTYPEID_UINT32: return COMPARE(unsigned int);
						case asTYPEID_FLOAT: return COMPARE(float);
						case asTYPEID_DOUBLE: return COMPARE(double);
						default: return COMPARE(signed int);
#undef COMPARE
					}
				}
				else
				{
					if (SubTypeId & asTYPEID_OBJHANDLE)
					{
						if (*(void**)A == *(void**)B)
							return true;
					}

					if (Cache && Cache->EqFunc)
					{
						Context->Prepare(Cache->EqFunc);
						if (SubTypeId & asTYPEID_OBJHANDLE)
						{
							Context->SetObject(*((void**)A));
							Context->SetArgObject(0, *((void**)B));
						}
						else
						{
							Context->SetObject((void*)A);
							Context->SetArgObject(0, (void*)B);
						}

						if (ImmediateContext::Get(Context)->ExecuteNext() == asEXECUTION_FINISHED)
							return Context->GetReturnByte() != 0;

						return false;
					}

					if (Cache && Cache->CmpFunc)
					{
						Context->Prepare(Cache->CmpFunc);
						if (SubTypeId & asTYPEID_OBJHANDLE)
						{
							Context->SetObject(*((void**)A));
							Context->SetArgObject(0, *((void**)B));
						}
						else
						{
							Context->SetObject((void*)A);
							Context->SetArgObject(0, (void*)B);
						}

						if (ImmediateContext::Get(Context)->ExecuteNext() == asEXECUTION_FINISHED)
							return (int)Context->GetReturnDWord() == 0;

						return false;
					}
				}

				return false;
			}
			int Array::FindByRef(void* RefPtr) const
			{
				return FindByRef(0, RefPtr);
			}
			int Array::FindByRef(size_t StartAt, void* RefPtr) const
			{
				size_t Size = GetSize();
				if (SubTypeId & asTYPEID_OBJHANDLE)
				{
					RefPtr = *(void**)RefPtr;
					for (size_t i = StartAt; i < Size; i++)
					{
						if (*(void**)At(i) == RefPtr)
							return (int)i;
					}
				}
				else
				{
					for (size_t i = StartAt; i < Size; i++)
					{
						if (At(i) == RefPtr)
							return (int)i;
					}
				}

				return -1;
			}
			int Array::Find(void* Value) const
			{
				return Find(0, Value);
			}
			int Array::Find(size_t StartAt, void* Value) const
			{
				SCache* Cache = 0;
				if (SubTypeId & ~asTYPEID_MASK_SEQNBR)
				{
					Cache = reinterpret_cast<SCache*>(ObjType->GetUserData(ARRAY_CACHE));
					if (!Cache || (Cache->CmpFunc == 0 && Cache->EqFunc == 0))
					{
						asIScriptContext* Context = asGetActiveContext();
						asITypeInfo* SubType = ObjType->GetEngine()->GetTypeInfoById(SubTypeId);
						if (Context)
						{
							char Swap[512];
							if (Cache && Cache->EqFuncReturnCode == asMULTIPLE_FUNCTIONS)
								snprintf(Swap, 512, "Type '%s' has multiple matching opEquals or opCmp methods", SubType->GetName());
							else
								snprintf(Swap, 512, "Type '%s' does not have a matching opEquals or opCmp method", SubType->GetName());
							Context->SetException(Swap);
						}

						return -1;
					}
				}

				asIScriptContext* CmpContext = 0;
				bool IsNested = false;

				if (SubTypeId & ~asTYPEID_MASK_SEQNBR)
				{
					CmpContext = asGetActiveContext();
					if (CmpContext)
					{
						if (CmpContext->GetEngine() == ObjType->GetEngine() && CmpContext->PushState() >= 0)
							IsNested = true;
						else
							CmpContext = 0;
					}

					if (CmpContext == 0)
						CmpContext = ObjType->GetEngine()->CreateContext();
				}

				int Result = -1;
				size_t Size = GetSize();
				for (size_t i = StartAt; i < Size; i++)
				{
					if (Equals(At(i), Value, CmpContext, Cache))
					{
						Result = (int)i;
						break;
					}
				}

				if (CmpContext)
				{
					if (IsNested)
					{
						asEContextState State = CmpContext->GetState();
						CmpContext->PopState();
						if (State == asEXECUTION_ABORTED)
							CmpContext->Abort();
					}
					else
						CmpContext->Release();
				}

				return Result;
			}
			void Array::Copy(void* Dest, void* Src)
			{
				memcpy(Dest, Src, ElementSize);
			}
			void* Array::GetArrayItemPointer(int Index)
			{
				return Buffer->Data + Index * ElementSize;
			}
			void* Array::GetDataPointer(void* BufferPtr)
			{
				if ((SubTypeId & asTYPEID_MASK_OBJECT) && !(SubTypeId & asTYPEID_OBJHANDLE))
					return reinterpret_cast<void*>(*(size_t*)BufferPtr);
				else
					return BufferPtr;
			}
			void Array::SortAsc()
			{
				Sort(0, GetSize(), true);
			}
			void Array::SortAsc(size_t StartAt, size_t Count)
			{
				Sort(StartAt, Count, true);
			}
			void Array::SortDesc()
			{
				Sort(0, GetSize(), false);
			}
			void Array::SortDesc(size_t StartAt, size_t Count)
			{
				Sort(StartAt, Count, false);
			}
			void Array::Sort(size_t StartAt, size_t Count, bool Asc)
			{
				SCache* Cache = reinterpret_cast<SCache*>(ObjType->GetUserData(ARRAY_CACHE));
				if (SubTypeId & ~asTYPEID_MASK_SEQNBR)
				{
					if (!Cache || Cache->CmpFunc == 0)
					{
						asIScriptContext* Context = asGetActiveContext();
						asITypeInfo* SubType = ObjType->GetEngine()->GetTypeInfoById(SubTypeId);
						if (Context)
						{
							char Swap[512];
							if (Cache && Cache->CmpFuncReturnCode == asMULTIPLE_FUNCTIONS)
								snprintf(Swap, 512, "Type '%s' has multiple matching opCmp methods", SubType->GetName());
							else
								snprintf(Swap, 512, "Type '%s' does not have a matching opCmp method", SubType->GetName());
							Context->SetException(Swap);
						}

						return;
					}
				}

				if (Count < 2)
					return;

				int Start = (int)StartAt;
				int End = (int)StartAt + (int)Count;

				if (Start >= (int)Buffer->NumElements || End > (int)Buffer->NumElements)
				{
					asIScriptContext* Context = asGetActiveContext();
					if (Context)
						Context->SetException("index out of bounds");

					return;
				}

				unsigned char Swap[16];
				asIScriptContext* CmpContext = 0;
				bool IsNested = false;

				if (SubTypeId & ~asTYPEID_MASK_SEQNBR)
				{
					CmpContext = asGetActiveContext();
					if (CmpContext)
					{
						if (CmpContext->GetEngine() == ObjType->GetEngine() && CmpContext->PushState() >= 0)
							IsNested = true;
						else
							CmpContext = 0;
					}

					if (CmpContext == 0)
						CmpContext = ObjType->GetEngine()->RequestContext();
				}

				for (int i = Start + 1; i < End; i++)
				{
					Copy(Swap, GetArrayItemPointer(i));
					int j = i - 1;

					while (j >= Start && Less(GetDataPointer(Swap), At(j), Asc, CmpContext, Cache))
					{
						Copy(GetArrayItemPointer(j + 1), GetArrayItemPointer(j));
						j--;
					}

					Copy(GetArrayItemPointer(j + 1), Swap);
				}

				if (CmpContext)
				{
					if (IsNested)
					{
						asEContextState State = CmpContext->GetState();
						CmpContext->PopState();
						if (State == asEXECUTION_ABORTED)
							CmpContext->Abort();
					}
					else
						ObjType->GetEngine()->ReturnContext(CmpContext);
				}
			}
			void Array::Sort(asIScriptFunction* Function, size_t StartAt, size_t Count)
			{
				if (Count < 2)
					return;

				size_t Start = StartAt;
				size_t End = as_uint64_t(StartAt) + as_uint64_t(Count) >= (as_uint64_t(1) << 32) ? 0xFFFFFFFF : StartAt + Count;
				if (End > Buffer->NumElements)
					End = Buffer->NumElements;

				if (Start >= Buffer->NumElements)
				{
					asIScriptContext* Context = asGetActiveContext();
					if (Context)
						Context->SetException("index out of bounds");

					return;
				}

				unsigned char Swap[16];
				asIScriptContext* CmpContext = 0;
				bool IsNested = false;

				CmpContext = asGetActiveContext();
				if (CmpContext)
				{
					if (CmpContext->GetEngine() == ObjType->GetEngine() && CmpContext->PushState() >= 0)
						IsNested = true;
					else
						CmpContext = 0;
				}

				if (CmpContext == 0)
					CmpContext = ObjType->GetEngine()->RequestContext();

				if (!CmpContext)
					return;

				auto* CmpContextVM = ImmediateContext::Get(CmpContext);
				for (size_t i = Start + 1; i < End; i++)
				{
					Copy(Swap, GetArrayItemPointer((int)i));
					size_t j = i - 1;

					while (j != 0xFFFFFFFF && j >= Start)
					{
						CmpContext->Prepare(Function);
						CmpContext->SetArgAddress(0, GetDataPointer(Swap));
						CmpContext->SetArgAddress(1, At(j));
						int Result = CmpContextVM->ExecuteNext();
						if (Result != asEXECUTION_FINISHED)
							break;

						if (*(bool*)(CmpContext->GetAddressOfReturnValue()))
						{
							Copy(GetArrayItemPointer((int)j + 1), GetArrayItemPointer((int)j));
							j--;
						}
						else
							break;
					}

					Copy(GetArrayItemPointer((int)j + 1), Swap);
				}

				if (IsNested)
				{
					asEContextState State = CmpContext->GetState();
					CmpContext->PopState();
					if (State == asEXECUTION_ABORTED)
						CmpContext->Abort();
				}
				else
					ObjType->GetEngine()->ReturnContext(CmpContext);
			}
			void Array::CopyBuffer(SBuffer* Dest, SBuffer* Src)
			{
				asIScriptEngine* Engine = ObjType->GetEngine();
				if (SubTypeId & asTYPEID_OBJHANDLE)
				{
					if (Dest->NumElements > 0 && Src->NumElements > 0)
					{
						int Count = (int)(Dest->NumElements > Src->NumElements ? Src->NumElements : Dest->NumElements);
						void** Max = (void**)(Dest->Data + Count * sizeof(void*));
						void** D = (void**)Dest->Data;
						void** S = (void**)Src->Data;

						for (; D < Max; D++, S++)
						{
							void* Swap = *D;
							*D = *S;

							if (*D)
								Engine->AddRefScriptObject(*D, ObjType->GetSubType());

							if (Swap)
								Engine->ReleaseScriptObject(Swap, ObjType->GetSubType());
						}
					}
				}
				else
				{
					if (Dest->NumElements > 0 && Src->NumElements > 0)
					{
						int Count = (int)(Dest->NumElements > Src->NumElements ? Src->NumElements : Dest->NumElements);
						if (SubTypeId & asTYPEID_MASK_OBJECT)
						{
							void** Max = (void**)(Dest->Data + Count * sizeof(void*));
							void** D = (void**)Dest->Data;
							void** s = (void**)Src->Data;

							asITypeInfo* SubType = ObjType->GetSubType();
							for (; D < Max; D++, s++)
								Engine->AssignScriptObject(*D, *s, SubType);
						}
						else
							memcpy(Dest->Data, Src->Data, (size_t)Count * (size_t)ElementSize);
					}
				}
			}
			void Array::Precache()
			{
				SubTypeId = ObjType->GetSubTypeId();
				if (!(SubTypeId & ~asTYPEID_MASK_SEQNBR))
					return;

				SCache* Cache = reinterpret_cast<SCache*>(ObjType->GetUserData(ARRAY_CACHE));
				if (Cache)
					return;

				asAcquireExclusiveLock();
				Cache = reinterpret_cast<SCache*>(ObjType->GetUserData(ARRAY_CACHE));
				if (Cache)
				{
					asReleaseExclusiveLock();
					return;
				}

				Cache = reinterpret_cast<SCache*>(asAllocMem(sizeof(SCache)));
				if (!Cache)
				{
					asIScriptContext* Context = asGetActiveContext();
					if (Context)
						Context->SetException("out of memory");

					asReleaseExclusiveLock();
					return;
				}

				memset(Cache, 0, sizeof(SCache));
				bool MustBeConst = (SubTypeId & asTYPEID_HANDLETOCONST) ? true : false;

				asITypeInfo* SubType = ObjType->GetEngine()->GetTypeInfoById(SubTypeId);
				if (SubType)
				{
					for (size_t i = 0; i < SubType->GetMethodCount(); i++)
					{
						asIScriptFunction* Function = SubType->GetMethodByIndex((int)i);
						if (Function->GetParamCount() == 1 && (!MustBeConst || Function->IsReadOnly()))
						{
							asDWORD Flags = 0;
							int ReturnTypeId = Function->GetReturnTypeId(&Flags);
							if (Flags != asTM_NONE)
								continue;

							bool IsCmp = false, IsEquals = false;
							if (ReturnTypeId == asTYPEID_INT32 && strcmp(Function->GetName(), "opCmp") == 0)
								IsCmp = true;
							if (ReturnTypeId == asTYPEID_BOOL && strcmp(Function->GetName(), "opEquals") == 0)
								IsEquals = true;

							if (!IsCmp && !IsEquals)
								continue;

							int ParamTypeId;
							Function->GetParam(0, &ParamTypeId, &Flags);

							if ((ParamTypeId & ~(asTYPEID_OBJHANDLE | asTYPEID_HANDLETOCONST)) != (SubTypeId & ~(asTYPEID_OBJHANDLE | asTYPEID_HANDLETOCONST)))
								continue;

							if ((Flags & asTM_INREF))
							{
								if ((ParamTypeId & asTYPEID_OBJHANDLE) || (MustBeConst && !(Flags & asTM_CONST)))
									continue;
							}
							else if (ParamTypeId & asTYPEID_OBJHANDLE)
							{
								if (MustBeConst && !(ParamTypeId & asTYPEID_HANDLETOCONST))
									continue;
							}
							else
								continue;

							if (IsCmp)
							{
								if (Cache->CmpFunc || Cache->CmpFuncReturnCode)
								{
									Cache->CmpFunc = 0;
									Cache->CmpFuncReturnCode = asMULTIPLE_FUNCTIONS;
								}
								else
									Cache->CmpFunc = Function;
							}
							else if (IsEquals)
							{
								if (Cache->EqFunc || Cache->EqFuncReturnCode)
								{
									Cache->EqFunc = 0;
									Cache->EqFuncReturnCode = asMULTIPLE_FUNCTIONS;
								}
								else
									Cache->EqFunc = Function;
							}
						}
					}
				}

				if (Cache->EqFunc == 0 && Cache->EqFuncReturnCode == 0)
					Cache->EqFuncReturnCode = asNO_FUNCTION;
				if (Cache->CmpFunc == 0 && Cache->CmpFuncReturnCode == 0)
					Cache->CmpFuncReturnCode = asNO_FUNCTION;

				ObjType->SetUserData(Cache, ARRAY_CACHE);
				asReleaseExclusiveLock();
			}
			void Array::EnumReferences(asIScriptEngine* Engine)
			{
				if (SubTypeId & asTYPEID_MASK_OBJECT)
				{
					void** D = (void**)Buffer->Data;
					asITypeInfo* SubType = Engine->GetTypeInfoById(SubTypeId);
					if ((SubType->GetFlags() & asOBJ_REF))
					{
						for (size_t n = 0; n < Buffer->NumElements; n++)
						{
							if (D[n])
								Engine->GCEnumCallback(D[n]);
						}
					}
					else if ((SubType->GetFlags() & asOBJ_VALUE) && (SubType->GetFlags() & asOBJ_GC))
					{
						for (size_t n = 0; n < Buffer->NumElements; n++)
						{
							if (D[n])
								Engine->ForwardGCEnumReferences(D[n], SubType);
						}
					}
				}
			}
			void Array::ReleaseAllHandles(asIScriptEngine*)
			{
				Resize(0);
			}
			void Array::AddRef() const
			{
				GCFlag = false;
				asAtomicInc(RefCount);
			}
			void Array::Release() const
			{
				GCFlag = false;
				if (asAtomicDec(RefCount) == 0)
				{
					this->~Array();
					asFreeMem(const_cast<Array*>(this));
				}
			}
			int Array::GetRefCount()
			{
				return RefCount;
			}
			void Array::SetFlag()
			{
				GCFlag = true;
			}
			bool Array::GetFlag()
			{
				return GCFlag;
			}
			Array* Array::Create(asITypeInfo* Info, size_t Length)
			{
				void* Memory = asAllocMem(sizeof(Array));
				if (Memory == 0)
				{
					asIScriptContext* Context = asGetActiveContext();
					if (Context)
						Context->SetException("out of memory");

					return 0;
				}

				Array* a = new(Memory) Array(Length, Info);
				return a;
			}
			Array* Array::Create(asITypeInfo* Info, void* InitList)
			{
				void* Memory = asAllocMem(sizeof(Array));
				if (Memory == 0)
				{
					asIScriptContext* Context = asGetActiveContext();
					if (Context)
						Context->SetException("out of memory");

					return 0;
				}

				Array* a = new(Memory) Array(Info, InitList);
				return a;
			}
			Array* Array::Create(asITypeInfo* Info, size_t length, void* DefaultValue)
			{
				void* Memory = asAllocMem(sizeof(Array));
				if (Memory == 0)
				{
					asIScriptContext* Context = asGetActiveContext();
					if (Context)
						Context->SetException("out of memory");

					return 0;
				}

				Array* a = new(Memory) Array(length, DefaultValue, Info);
				return a;
			}
			Array* Array::Create(asITypeInfo* Info)
			{
				return Array::Create(Info, size_t(0));
			}
			void Array::CleanupTypeInfoCache(asITypeInfo* Type)
			{
				Array::SCache* Cache = reinterpret_cast<Array::SCache*>(Type->GetUserData(ARRAY_CACHE));
				if (Cache != nullptr)
				{
					Cache->~SCache();
					asFreeMem(Cache);
				}
			}
			bool Array::TemplateCallback(asITypeInfo* Info, bool& DontGarbageCollect)
			{
				int TypeId = Info->GetSubTypeId();
				if (TypeId == asTYPEID_VOID)
					return false;

				if ((TypeId & asTYPEID_MASK_OBJECT) && !(TypeId & asTYPEID_OBJHANDLE))
				{
					asIScriptEngine* Engine = Info->GetEngine();
					asITypeInfo* SubType = Engine->GetTypeInfoById(TypeId);
					asDWORD Flags = SubType->GetFlags();

					if ((Flags & asOBJ_VALUE) && !(Flags & asOBJ_POD))
					{
						bool Found = false;
						for (size_t n = 0; n < SubType->GetBehaviourCount(); n++)
						{
							asEBehaviours Beh;
							asIScriptFunction* Func = SubType->GetBehaviourByIndex((int)n, &Beh);
							if (Beh != asBEHAVE_CONSTRUCT)
								continue;

							if (Func->GetParamCount() == 0)
							{
								Found = true;
								break;
							}
						}

						if (!Found)
						{
							Engine->WriteMessage(TYPENAME_ARRAY, 0, 0, asMSGTYPE_ERROR, "The subtype has no default constructor");
							return false;
						}
					}
					else if ((Flags & asOBJ_REF))
					{
						bool Found = false;
						if (!Engine->GetEngineProperty(asEP_DISALLOW_VALUE_ASSIGN_FOR_REF_TYPE))
						{
							for (size_t n = 0; n < SubType->GetFactoryCount(); n++)
							{
								asIScriptFunction* Func = SubType->GetFactoryByIndex((int)n);
								if (Func->GetParamCount() == 0)
								{
									Found = true;
									break;
								}
							}
						}

						if (!Found)
						{
							Engine->WriteMessage(TYPENAME_ARRAY, 0, 0, asMSGTYPE_ERROR, "The subtype has no default factory");
							return false;
						}
					}

					if (!(Flags & asOBJ_GC))
						DontGarbageCollect = true;
				}
				else if (!(TypeId & asTYPEID_OBJHANDLE))
				{
					DontGarbageCollect = true;
				}
				else
				{
					asITypeInfo* SubType = Info->GetEngine()->GetTypeInfoById(TypeId);
					asDWORD Flags = SubType->GetFlags();

					if (!(Flags & asOBJ_GC))
					{
						if ((Flags & asOBJ_SCRIPT_OBJECT))
						{
							if ((Flags & asOBJ_NOINHERIT))
								DontGarbageCollect = true;
						}
						else
							DontGarbageCollect = true;
					}
				}

				return true;
			}

			Storable::Storable() noexcept
			{
			}
			Storable::Storable(asIScriptEngine* Engine, void* Value, int _TypeId) noexcept
			{
				Set(Engine, Value, _TypeId);
			}
			Storable::~Storable() noexcept
			{
				if (Value.Object && Value.TypeId)
				{
					asIScriptContext* Context = asGetActiveContext();
					if (Context != nullptr)
						FreeValue(Context->GetEngine());
				}
			}
			void Storable::FreeValue(asIScriptEngine* Engine)
			{
				if (Value.TypeId & asTYPEID_MASK_OBJECT)
				{
					Engine->ReleaseScriptObject(Value.Object, Engine->GetTypeInfoById(Value.TypeId));
					Value.Clean();
				}
			}
			void Storable::EnumReferences(asIScriptEngine* _Engine)
			{
				if (Value.Object != nullptr)
					_Engine->GCEnumCallback(Value.Object);

				if (Value.TypeId)
					_Engine->GCEnumCallback(_Engine->GetTypeInfoById(Value.TypeId));
			}
			void Storable::Set(asIScriptEngine* Engine, void* Pointer, int _TypeId)
			{
				FreeValue(Engine);
				Value.TypeId = _TypeId;

				if (Value.TypeId & asTYPEID_OBJHANDLE)
				{
					Value.Object = *(void**)Pointer;
					Engine->AddRefScriptObject(Value.Object, Engine->GetTypeInfoById(Value.TypeId));
				}
				else if (Value.TypeId & asTYPEID_MASK_OBJECT)
				{
					Value.Object = Engine->CreateScriptObjectCopy(Pointer, Engine->GetTypeInfoById(Value.TypeId));
					if (Value.Object == 0)
					{
						asIScriptContext* Context = asGetActiveContext();
						if (Context)
							Context->SetException("cannot create copy of object");
					}
				}
				else
				{
					int Size = Engine->GetSizeOfPrimitiveType(Value.TypeId);
					memcpy(&Value.Integer, Pointer, Size);
				}
			}
			void Storable::Set(asIScriptEngine* Engine, Storable& Other)
			{
				if (Other.Value.TypeId & asTYPEID_OBJHANDLE)
					Set(Engine, (void*)&Other.Value.Object, Other.Value.TypeId);
				else if (Other.Value.TypeId & asTYPEID_MASK_OBJECT)
					Set(Engine, (void*)Other.Value.Object, Other.Value.TypeId);
				else
					Set(Engine, (void*)&Other.Value.Integer, Other.Value.TypeId);
			}
			bool Storable::Get(asIScriptEngine* Engine, void* Pointer, int _TypeId) const
			{
				if (_TypeId & asTYPEID_OBJHANDLE)
				{
					if ((_TypeId & asTYPEID_MASK_OBJECT))
					{
						if ((Value.TypeId & asTYPEID_HANDLETOCONST) && !(_TypeId & asTYPEID_HANDLETOCONST))
							return false;

						Engine->RefCastObject(Value.Object, Engine->GetTypeInfoById(Value.TypeId), Engine->GetTypeInfoById(_TypeId), reinterpret_cast<void**>(Pointer));
						return true;
					}
				}
				else if (_TypeId & asTYPEID_MASK_OBJECT)
				{
					bool isCompatible = false;
					if ((Value.TypeId & ~(asTYPEID_OBJHANDLE | asTYPEID_HANDLETOCONST)) == _TypeId && Value.Object != 0)
						isCompatible = true;

					if (isCompatible)
					{
						Engine->AssignScriptObject(Pointer, Value.Object, Engine->GetTypeInfoById(Value.TypeId));
						return true;
					}
				}
				else
				{
					if (Value.TypeId == _TypeId)
					{
						int Size = Engine->GetSizeOfPrimitiveType(_TypeId);
						memcpy(Pointer, &Value.Integer, Size);
						return true;
					}

					if (_TypeId == asTYPEID_DOUBLE)
					{
						if (Value.TypeId == asTYPEID_INT64)
						{
							*(double*)Pointer = double(Value.Integer);
						}
						else if (Value.TypeId == asTYPEID_BOOL)
						{
							char Local;
							memcpy(&Local, &Value.Integer, sizeof(char));
							*(double*)Pointer = Local ? 1.0 : 0.0;
						}
						else if (Value.TypeId > asTYPEID_DOUBLE && (Value.TypeId & asTYPEID_MASK_OBJECT) == 0)
						{
							int Local;
							memcpy(&Local, &Value.Integer, sizeof(int));
							*(double*)Pointer = double(Local);
						}
						else
						{
							*(double*)Pointer = 0;
							return false;
						}

						return true;
					}
					else if (_TypeId == asTYPEID_INT64)
					{
						if (Value.TypeId == asTYPEID_DOUBLE)
						{
							*(as_int64_t*)Pointer = as_int64_t(Value.Number);
						}
						else if (Value.TypeId == asTYPEID_BOOL)
						{
							char Local;
							memcpy(&Local, &Value.Integer, sizeof(char));
							*(as_int64_t*)Pointer = Local ? 1 : 0;
						}
						else if (Value.TypeId > asTYPEID_DOUBLE && (Value.TypeId & asTYPEID_MASK_OBJECT) == 0)
						{
							int Local;
							memcpy(&Local, &Value.Integer, sizeof(int));
							*(as_int64_t*)Pointer = Local;
						}
						else
						{
							*(as_int64_t*)Pointer = 0;
							return false;
						}

						return true;
					}
					else if (_TypeId > asTYPEID_DOUBLE && (Value.TypeId & asTYPEID_MASK_OBJECT) == 0)
					{
						if (Value.TypeId == asTYPEID_DOUBLE)
						{
							*(int*)Pointer = int(Value.Number);
						}
						else if (Value.TypeId == asTYPEID_INT64)
						{
							*(int*)Pointer = int(Value.Integer);
						}
						else if (Value.TypeId == asTYPEID_BOOL)
						{
							char Local;
							memcpy(&Local, &Value.Integer, sizeof(char));
							*(int*)Pointer = Local ? 1 : 0;
						}
						else if (Value.TypeId > asTYPEID_DOUBLE && (Value.TypeId & asTYPEID_MASK_OBJECT) == 0)
						{
							int Local;
							memcpy(&Local, &Value.Integer, sizeof(int));
							*(int*)Pointer = Local;
						}
						else
						{
							*(int*)Pointer = 0;
							return false;
						}

						return true;
					}
					else if (_TypeId == asTYPEID_BOOL)
					{
						if (Value.TypeId & asTYPEID_OBJHANDLE)
						{
							*(bool*)Pointer = Value.Object ? true : false;
						}
						else if (Value.TypeId & asTYPEID_MASK_OBJECT)
						{
							*(bool*)Pointer = true;
						}
						else
						{
							as_uint64_t Zero = 0;
							int Size = Engine->GetSizeOfPrimitiveType(Value.TypeId);
							*(bool*)Pointer = memcmp(&Value.Integer, &Zero, Size) == 0 ? false : true;
						}

						return true;
					}
				}

				return false;
			}
			const void* Storable::GetAddressOfValue() const
			{
				if ((Value.TypeId & asTYPEID_MASK_OBJECT) && !(Value.TypeId & asTYPEID_OBJHANDLE))
					return Value.Object;

				return reinterpret_cast<const void*>(&Value.Object);
			}
			int Storable::GetTypeId() const
			{
				return Value.TypeId;
			}

			Dictionary::LocalIterator::LocalIterator(const Dictionary& From, InternalMap::const_iterator It) noexcept : It(It), Base(From)
			{
			}
			void Dictionary::LocalIterator::operator++()
			{
				++It;
			}
			void Dictionary::LocalIterator::operator++(int)
			{
				++It;
			}
			Dictionary::LocalIterator& Dictionary::LocalIterator::operator*()
			{
				return *this;
			}
			bool Dictionary::LocalIterator::operator==(const LocalIterator& Other) const
			{
				return It == Other.It;
			}
			bool Dictionary::LocalIterator::operator!=(const LocalIterator& Other) const
			{
				return It != Other.It;
			}
			const Core::String& Dictionary::LocalIterator::GetKey() const
			{
				return It->first;
			}
			int Dictionary::LocalIterator::GetTypeId() const
			{
				return It->second.Value.TypeId;
			}
			bool Dictionary::LocalIterator::GetValue(void* Pointer, int TypeId) const
			{
				return It->second.Get(Base.Engine, Pointer, TypeId);
			}
			const void* Dictionary::LocalIterator::GetAddressOfValue() const
			{
				return It->second.GetAddressOfValue();
			}

			Dictionary::Dictionary(asIScriptEngine* _Engine) noexcept
			{
				Init(_Engine);
			}
			Dictionary::Dictionary(unsigned char* Buffer) noexcept
			{
				asIScriptContext* Context = asGetActiveContext();
				Init(Context->GetEngine());

				Dictionary::SCache& Cache = *reinterpret_cast<Dictionary::SCache*>(Engine->GetUserData(MAP_CACHE));
				bool keyAsRef = Cache.KeyType->GetFlags() & asOBJ_REF ? true : false;

				size_t Length = *(asUINT*)Buffer;
				Buffer += 4;

				while (Length--)
				{
					if (asPWORD(Buffer) & 0x3)
						Buffer += 4 - (asPWORD(Buffer) & 0x3);

					Core::String Name;
					if (keyAsRef)
					{
						Name = **(Core::String**)Buffer;
						Buffer += sizeof(Core::String*);
					}
					else
					{
						Name = *(Core::String*)Buffer;
						Buffer += sizeof(Core::String);
					}

					int TypeId = *(int*)Buffer;
					Buffer += sizeof(int);

					void* RefPtr = (void*)Buffer;
					if (TypeId >= asTYPEID_INT8 && TypeId <= asTYPEID_DOUBLE)
					{
						as_int64_t Integer64;
						double Double64;

						switch (TypeId)
						{
							case asTYPEID_INT8:
								Integer64 = *(char*)RefPtr;
								break;
							case asTYPEID_INT16:
								Integer64 = *(short*)RefPtr;
								break;
							case asTYPEID_INT32:
								Integer64 = *(int*)RefPtr;
								break;
							case asTYPEID_UINT8:
								Integer64 = *(unsigned char*)RefPtr;
								break;
							case asTYPEID_UINT16:
								Integer64 = *(unsigned short*)RefPtr;
								break;
							case asTYPEID_UINT32:
								Integer64 = *(unsigned int*)RefPtr;
								break;
							case asTYPEID_INT64:
							case asTYPEID_UINT64:
								Integer64 = *(as_int64_t*)RefPtr;
								break;
							case asTYPEID_FLOAT:
								Double64 = *(float*)RefPtr;
								break;
							case asTYPEID_DOUBLE:
								Double64 = *(double*)RefPtr;
								break;
						}

						if (TypeId >= asTYPEID_FLOAT)
							Set(Name, &Double64, asTYPEID_DOUBLE);
						else
							Set(Name, &Integer64, asTYPEID_INT64);
					}
					else
					{
						if ((TypeId & asTYPEID_MASK_OBJECT) && !(TypeId & asTYPEID_OBJHANDLE) && (Engine->GetTypeInfoById(TypeId)->GetFlags() & asOBJ_REF))
							RefPtr = *(void**)RefPtr;

						Set(Name, RefPtr, VirtualMachine::Get(Engine)->IsNullable(TypeId) ? 0 : TypeId);
					}

					if (TypeId & asTYPEID_MASK_OBJECT)
					{
						asITypeInfo* Info = Engine->GetTypeInfoById(TypeId);
						if (Info->GetFlags() & asOBJ_VALUE)
							Buffer += Info->GetSize();
						else
							Buffer += sizeof(void*);
					}
					else if (TypeId == 0)
						Buffer += sizeof(void*);
					else
						Buffer += Engine->GetSizeOfPrimitiveType(TypeId);
				}
			}
			Dictionary::Dictionary(const Dictionary& Other) noexcept
			{
				Init(Other.Engine);
				for (auto It = Other.Data.begin(); It != Other.Data.end(); ++It)
				{
					auto& Key = It->second;
					if (Key.Value.TypeId & asTYPEID_OBJHANDLE)
						Set(It->first, (void*)&Key.Value.Object, Key.Value.TypeId);
					else if (Key.Value.TypeId & asTYPEID_MASK_OBJECT)
						Set(It->first, (void*)Key.Value.Object, Key.Value.TypeId);
					else
						Set(It->first, (void*)&Key.Value.Integer, Key.Value.TypeId);
				}
			}
			Dictionary::~Dictionary() noexcept
			{
				DeleteAll();
			}
			void Dictionary::AddRef() const
			{
				GCFlag = false;
				asAtomicInc(RefCount);
			}
			void Dictionary::Release() const
			{
				GCFlag = false;
				if (asAtomicDec(RefCount) == 0)
				{
					this->~Dictionary();
					asFreeMem(const_cast<Dictionary*>(this));
				}
			}
			int Dictionary::GetRefCount()
			{
				return RefCount;
			}
			void Dictionary::SetGCFlag()
			{
				GCFlag = true;
			}
			bool Dictionary::GetGCFlag()
			{
				return GCFlag;
			}
			void Dictionary::EnumReferences(asIScriptEngine* _Engine)
			{
				for (auto It = Data.begin(); It != Data.end(); ++It)
				{
					auto& Key = It->second;
					if (Key.Value.TypeId & asTYPEID_MASK_OBJECT)
					{
						asITypeInfo* SubType = Engine->GetTypeInfoById(Key.Value.TypeId);
						if ((SubType->GetFlags() & asOBJ_VALUE) && (SubType->GetFlags() & asOBJ_GC))
							Engine->ForwardGCEnumReferences(Key.Value.Object, SubType);
						else
							_Engine->GCEnumCallback(Key.Value.Object);
					}
				}
			}
			void Dictionary::ReleaseAllReferences(asIScriptEngine*)
			{
				DeleteAll();
			}
			Dictionary& Dictionary::operator =(const Dictionary& Other) noexcept
			{
				DeleteAll();
				for (auto It = Other.Data.begin(); It != Other.Data.end(); ++It)
				{
					auto& Key = It->second;
					if (Key.Value.TypeId & asTYPEID_OBJHANDLE)
						Set(It->first, (void*)&Key.Value.Object, Key.Value.TypeId);
					else if (Key.Value.TypeId & asTYPEID_MASK_OBJECT)
						Set(It->first, (void*)Key.Value.Object, Key.Value.TypeId);
					else
						Set(It->first, (void*)&Key.Value.Integer, Key.Value.TypeId);
				}

				return *this;
			}
			Storable* Dictionary::operator[](const Core::String& Key)
			{
				return &Data[Key];
			}
			const Storable* Dictionary::operator[](const Core::String& Key) const
			{
				auto It = Data.find(Key);
				if (It != Data.end())
					return &It->second;

				asIScriptContext* Context = asGetActiveContext();
				if (Context)
					Context->SetException("invalid access to non-existing value");

				return 0;
			}
			void Dictionary::Set(const Core::String& Key, void* Value, int TypeId)
			{
				auto It = Data.find(Key);
				if (It == Data.end())
					It = Data.insert(InternalMap::value_type(Key, Storable())).first;

				It->second.Set(Engine, Value, TypeId);
			}
			bool Dictionary::Get(const Core::String& Key, void* Value, int TypeId) const
			{
				auto It = Data.find(Key);
				if (It != Data.end())
					return It->second.Get(Engine, Value, TypeId);

				return false;
			}
			bool Dictionary::GetIndex(size_t Index, Core::String* Key, void** Value, int* TypeId) const
			{
				if (Index >= Data.size())
					return false;

				auto It = Begin();
				size_t Offset = 0;

				while (Offset != Index)
				{
					++Offset;
					++It;
				}

				if (Key != nullptr)
					*Key = It.GetKey();

				if (Value != nullptr)
					*Value = (void*)It.GetAddressOfValue();

				if (TypeId != nullptr)
					*TypeId = It.GetTypeId();

				return true;
			}
			int Dictionary::GetTypeId(const Core::String& Key) const
			{
				auto It = Data.find(Key);
				if (It != Data.end())
					return It->second.Value.TypeId;

				return -1;
			}
			bool Dictionary::Exists(const Core::String& Key) const
			{
				auto It = Data.find(Key);
				if (It != Data.end())
					return true;

				return false;
			}
			bool Dictionary::IsEmpty() const
			{
				if (Data.size() == 0)
					return true;

				return false;
			}
			size_t Dictionary::GetSize() const
			{
				return size_t(Data.size());
			}
			bool Dictionary::Delete(const Core::String& Key)
			{
				auto It = Data.find(Key);
				if (It != Data.end())
				{
					It->second.FreeValue(Engine);
					Data.erase(It);
					return true;
				}

				return false;
			}
			void Dictionary::DeleteAll()
			{
				for (auto It = Data.begin(); It != Data.end(); ++It)
					It->second.FreeValue(Engine);
				Data.clear();
			}
			Array* Dictionary::GetKeys() const
			{
				Dictionary::SCache* Cache = reinterpret_cast<Dictionary::SCache*>(Engine->GetUserData(MAP_CACHE));
				asITypeInfo* Info = Cache->ArrayType;

				Array* Array = Array::Create(Info, size_t(Data.size()));
				size_t Current = 0;

				for (auto It = Data.begin(); It != Data.end(); ++It)
					*(Core::String*)Array->At(Current++) = It->first;

				return Array;
			}
			Dictionary::LocalIterator Dictionary::Begin() const
			{
				return LocalIterator(*this, Data.begin());
			}
			Dictionary::LocalIterator Dictionary::End() const
			{
				return LocalIterator(*this, Data.end());
			}
			Dictionary::LocalIterator Dictionary::Find(const Core::String& Key) const
			{
				return LocalIterator(*this, Data.find(Key));
			}
			Dictionary* Dictionary::Create(asIScriptEngine* Engine)
			{
				Dictionary* Obj = (Dictionary*)asAllocMem(sizeof(Dictionary));
				new(Obj) Dictionary(Engine);
				return Obj;
			}
			Dictionary* Dictionary::Create(unsigned char* buffer)
			{
				Dictionary* Obj = (Dictionary*)asAllocMem(sizeof(Dictionary));
				new(Obj) Dictionary(buffer);
				return Obj;
			}
			void Dictionary::Init(asIScriptEngine* Engine_)
			{
				RefCount = 1;
				GCFlag = false;
				Engine = Engine_;

				Dictionary::SCache* Cache = reinterpret_cast<Dictionary::SCache*>(Engine->GetUserData(MAP_CACHE));
				Engine->NotifyGarbageCollectorOfNewObject(this, Cache->DictionaryType);
			}
			void Dictionary::Cleanup(asIScriptEngine* Engine)
			{
				Dictionary::SCache* Cache = reinterpret_cast<Dictionary::SCache*>(Engine->GetUserData(MAP_CACHE));
				VI_DELETE(SCache, Cache);
			}
			void Dictionary::Setup(asIScriptEngine* Engine)
			{
				Dictionary::SCache* Cache = reinterpret_cast<Dictionary::SCache*>(Engine->GetUserData(MAP_CACHE));
				if (Cache == 0)
				{
					Cache = VI_NEW(Dictionary::SCache);
					Engine->SetUserData(Cache, MAP_CACHE);
					Engine->SetEngineUserDataCleanupCallback(Cleanup, MAP_CACHE);

					Cache->DictionaryType = Engine->GetTypeInfoByName(TYPENAME_DICTIONARY);
					Cache->ArrayType = Engine->GetTypeInfoByDecl(TYPENAME_ARRAY "<" TYPENAME_STRING ">");
					Cache->KeyType = Engine->GetTypeInfoByDecl(TYPENAME_STRING);
				}
			}
			void Dictionary::Factory(asIScriptGeneric* Generic)
			{
				*(Dictionary**)Generic->GetAddressOfReturnLocation() = Dictionary::Create(Generic->GetEngine());
			}
			void Dictionary::ListFactory(asIScriptGeneric* Generic)
			{
				unsigned char* buffer = (unsigned char*)Generic->GetArgAddress(0);
				*(Dictionary**)Generic->GetAddressOfReturnLocation() = Dictionary::Create(buffer);
			}
			void Dictionary::KeyConstruct(void* Memory)
			{
				new(Memory) Storable();
			}
			void Dictionary::KeyDestruct(Storable* Obj)
			{
				asIScriptContext* Context = asGetActiveContext();
				if (Context)
				{
					asIScriptEngine* Engine = Context->GetEngine();
					Obj->FreeValue(Engine);
				}

				Obj->~Storable();
			}
			Storable& Dictionary::KeyopAssign(void* RefPtr, int TypeId, Storable* Obj)
			{
				asIScriptContext* Context = asGetActiveContext();
				if (Context)
				{
					asIScriptEngine* Engine = Context->GetEngine();
					Obj->Set(Engine, RefPtr, TypeId);
				}

				return *Obj;
			}
			Storable& Dictionary::KeyopAssign(const Storable& Other, Storable* Obj)
			{
				asIScriptContext* Context = asGetActiveContext();
				if (Context)
				{
					asIScriptEngine* Engine = Context->GetEngine();
					Obj->Set(Engine, const_cast<Storable&>(Other));
				}

				return *Obj;
			}
			Storable& Dictionary::KeyopAssign(double Value, Storable* Obj)
			{
				return KeyopAssign(&Value, asTYPEID_DOUBLE, Obj);
			}
			Storable& Dictionary::KeyopAssign(as_int64_t Value, Storable* Obj)
			{
				return Dictionary::KeyopAssign(&Value, asTYPEID_INT64, Obj);
			}
			void Dictionary::KeyopCast(void* RefPtr, int TypeId, Storable* Obj)
			{
				asIScriptContext* Context = asGetActiveContext();
				if (Context)
				{
					asIScriptEngine* Engine = Context->GetEngine();
					Obj->Get(Engine, RefPtr, TypeId);
				}
			}
			as_int64_t Dictionary::KeyopConvInt(Storable* Obj)
			{
				as_int64_t Value;
				KeyopCast(&Value, asTYPEID_INT64, Obj);
				return Value;
			}
			double Dictionary::KeyopConvDouble(Storable* Obj)
			{
				double Value;
				KeyopCast(&Value, asTYPEID_DOUBLE, Obj);
				return Value;
			}

			Ref::Ref() noexcept : Type(0), Pointer(nullptr)
			{
			}
			Ref::Ref(const Ref& Other) noexcept : Type(Other.Type), Pointer(Other.Pointer)
			{
				AddRefHandle();
			}
			Ref::Ref(void* RefPtr, asITypeInfo* _Type) noexcept : Type(_Type), Pointer(RefPtr)
			{
				AddRefHandle();
			}
			Ref::Ref(void* RefPtr, int TypeId) noexcept : Type(0), Pointer(nullptr)
			{
				Assign(RefPtr, TypeId);
			}
			Ref::~Ref() noexcept
			{
				ReleaseHandle();
			}
			void Ref::ReleaseHandle()
			{
				if (Pointer && Type)
				{
					asIScriptEngine* Engine = Type->GetEngine();
					Engine->ReleaseScriptObject(Pointer, Type);
					Engine->Release();
					Pointer = nullptr;
					Type = 0;
				}
			}
			void Ref::AddRefHandle()
			{
				if (Pointer && Type)
				{
					asIScriptEngine* Engine = Type->GetEngine();
					Engine->AddRefScriptObject(Pointer, Type);
					Engine->AddRef();
				}
			}
			Ref& Ref::operator =(const Ref& Other) noexcept
			{
				Set(Other.Pointer, Other.Type);
				return *this;
			}
			void Ref::Set(void* RefPtr, asITypeInfo* _Type)
			{
				if (Pointer == RefPtr)
					return;

				ReleaseHandle();
				Pointer = RefPtr;
				Type = _Type;
				AddRefHandle();
			}
			void* Ref::GetAddressOfObject()
			{
				return &Pointer;
			}
			asITypeInfo* Ref::GetType() const
			{
				return Type;
			}
			int Ref::GetTypeId() const
			{
				if (Type == 0)
					return 0;

				return Type->GetTypeId() | asTYPEID_OBJHANDLE;
			}
			Ref& Ref::Assign(void* RefPtr, int TypeId)
			{
				if (TypeId == 0)
				{
					Set(0, 0);
					return *this;
				}

				if (TypeId & asTYPEID_OBJHANDLE)
				{
					RefPtr = *(void**)RefPtr;
					TypeId &= ~asTYPEID_OBJHANDLE;
				}

				asIScriptContext* Context = asGetActiveContext();
				asIScriptEngine* Engine = Context->GetEngine();
				asITypeInfo* Info = Engine->GetTypeInfoById(TypeId);

				if (Info && strcmp(Info->GetName(), TYPENAME_REF) == 0)
				{
					Ref* Result = (Ref*)RefPtr;
					RefPtr = Result->Pointer;
					Info = Result->Type;
				}

				Set(RefPtr, Info);
				return *this;
			}
			bool Ref::operator==(const Ref& Other) const
			{
				return Pointer == Other.Pointer && Type == Other.Type;
			}
			bool Ref::operator!=(const Ref& Other) const
			{
				return !(*this == Other);
			}
			bool Ref::Equals(void* RefPtr, int TypeId) const
			{
				if (TypeId == 0)
					RefPtr = 0;

				if (TypeId & asTYPEID_OBJHANDLE)
				{
					void** Sub = (void**)RefPtr;
					if (!Sub)
						return false;

					RefPtr = *Sub;
					TypeId &= ~asTYPEID_OBJHANDLE;
				}

				if (RefPtr == Pointer)
					return true;

				return false;
			}
			void Ref::Cast(void** OutRef, int TypeId)
			{
				if (Type == 0)
				{
					*OutRef = 0;
					return;
				}

				VI_ASSERT_V(TypeId & asTYPEID_OBJHANDLE, "type should be object handle");
				TypeId &= ~asTYPEID_OBJHANDLE;

				asIScriptEngine* Engine = Type->GetEngine();
				asITypeInfo* Info = Engine->GetTypeInfoById(TypeId);
				*OutRef = 0;

				Engine->RefCastObject(Pointer, Type, Info, OutRef);
			}
			void Ref::EnumReferences(asIScriptEngine* _Engine)
			{
				if (Pointer)
					_Engine->GCEnumCallback(Pointer);

				if (Type)
					_Engine->GCEnumCallback(Type);
			}
			void Ref::ReleaseReferences(asIScriptEngine* _Engine)
			{
				Set(0, 0);
			}
			void Ref::Construct(Ref* Base)
			{
				new(Base) Ref();
			}
			void Ref::Construct(Ref* Base, const Ref& Other)
			{
				new(Base) Ref(Other);
			}
			void Ref::Construct(Ref* Base, void* RefPtr, int TypeId)
			{
				new(Base) Ref(RefPtr, TypeId);
			}
			void Ref::Destruct(Ref* Base)
			{
				Base->~Ref();
			}

			Weak::Weak(asITypeInfo* _Type) noexcept : WeakRefFlag(nullptr), Type(_Type), Ref(nullptr)
			{
				VI_ASSERT_V(Type != nullptr, "type should be set");
				Type->AddRef();
			}
			Weak::Weak(const Weak& Other) noexcept
			{
				Ref = Other.Ref;
				Type = Other.Type;
				Type->AddRef();
				WeakRefFlag = Other.WeakRefFlag;
				if (WeakRefFlag)
					WeakRefFlag->AddRef();
			}
			Weak::Weak(void* RefPtr, asITypeInfo* _Type) noexcept
			{
				if (!_Type || !(strcmp(_Type->GetName(), TYPENAME_WEAK) == 0 || strcmp(_Type->GetName(), TYPENAME_CONSTWEAK) == 0))
					return;

				Ref = RefPtr;
				Type = _Type;
				Type->AddRef();

				WeakRefFlag = Type->GetEngine()->GetWeakRefFlagOfScriptObject(Ref, Type->GetSubType());
				if (WeakRefFlag)
					WeakRefFlag->AddRef();
			}
			Weak::~Weak() noexcept
			{
				if (Type)
					Type->Release();

				if (WeakRefFlag)
					WeakRefFlag->Release();
			}
			Weak& Weak::operator =(const Weak& Other) noexcept
			{
				if (Ref == Other.Ref && WeakRefFlag == Other.WeakRefFlag)
					return *this;

				if (Type != Other.Type)
				{
					if (!(strcmp(Type->GetName(), TYPENAME_CONSTWEAK) == 0 && strcmp(Other.Type->GetName(), TYPENAME_WEAK) == 0 && Type->GetSubType() == Other.Type->GetSubType()))
						return *this;
				}

				Ref = Other.Ref;
				if (WeakRefFlag)
					WeakRefFlag->Release();

				WeakRefFlag = Other.WeakRefFlag;
				if (WeakRefFlag)
					WeakRefFlag->AddRef();

				return *this;
			}
			Weak& Weak::Set(void* NewRef)
			{
				if (WeakRefFlag)
					WeakRefFlag->Release();

				Ref = NewRef;
				if (NewRef)
				{
					WeakRefFlag = Type->GetEngine()->GetWeakRefFlagOfScriptObject(NewRef, Type->GetSubType());
					WeakRefFlag->AddRef();
				}
				else
					WeakRefFlag = 0;

				Type->GetEngine()->ReleaseScriptObject(NewRef, Type->GetSubType());
				return *this;
			}
			asITypeInfo* Weak::GetRefType() const
			{
				return Type->GetSubType();
			}
			bool Weak::operator==(const Weak& Other) const
			{
				if (Ref == Other.Ref &&
					WeakRefFlag == Other.WeakRefFlag &&
					Type == Other.Type)
					return true;

				return false;
			}
			bool Weak::operator!=(const Weak& Other) const
			{
				return !(*this == Other);
			}
			int Weak::GetTypeId() const
			{
				if (!Type)
					return asTYPEID_VOID;

				return Type->GetTypeId();
			}
			void* Weak::GetAddressOfObject()
			{
				if (!Type)
					return nullptr;

				return &Ref;
			}
			void* Weak::Get() const
			{
				if (Ref == 0 || WeakRefFlag == 0)
					return 0;

				WeakRefFlag->Lock();
				if (!WeakRefFlag->Get())
				{
					Type->GetEngine()->AddRefScriptObject(Ref, Type->GetSubType());
					WeakRefFlag->Unlock();
					return Ref;
				}
				WeakRefFlag->Unlock();

				return 0;
			}
			bool Weak::Equals(void* RefPtr) const
			{
				if (Ref != RefPtr)
					return false;

				asILockableSharedBool* flag = Type->GetEngine()->GetWeakRefFlagOfScriptObject(RefPtr, Type->GetSubType());
				if (WeakRefFlag != flag)
					return false;

				return true;
			}
			void Weak::Construct(asITypeInfo* Type, void* Memory)
			{
				new(Memory) Weak(Type);
			}
			void Weak::Construct2(asITypeInfo* Type, void* RefPtr, void* Memory)
			{
				new(Memory) Weak(RefPtr, Type);
				asIScriptContext* Context = asGetActiveContext();
				if (Context && Context->GetState() == asEXECUTION_EXCEPTION)
					reinterpret_cast<Weak*>(Memory)->~Weak();
			}
			void Weak::Destruct(Weak* Obj)
			{
				Obj->~Weak();
			}
			bool Weak::TemplateCallback(asITypeInfo* Info, bool&)
			{
				asITypeInfo* SubType = Info->GetSubType();
				if (SubType == 0)
					return false;

				if (!(SubType->GetFlags() & asOBJ_REF))
					return false;

				if (Info->GetSubTypeId() & asTYPEID_OBJHANDLE)
					return false;

				size_t Count = SubType->GetBehaviourCount();
				for (size_t n = 0; n < Count; n++)
				{
					asEBehaviours Beh;
					SubType->GetBehaviourByIndex((int)n, &Beh);
					if (Beh == asBEHAVE_GET_WEAKREF_FLAG)
						return true;
				}

				Info->GetEngine()->WriteMessage(TYPENAME_WEAK, 0, 0, asMSGTYPE_ERROR, "The subtype doesn't support weak references");
				return false;
			}

			Core::String Random::Getb(uint64_t Size)
			{
				return Compute::Codec::HexEncode(Compute::Crypto::RandomBytes((size_t)Size)).substr(0, (size_t)Size);
			}
			double Random::Betweend(double Min, double Max)
			{
				return Compute::Mathd::Random(Min, Max);
			}
			double Random::Magd()
			{
				return Compute::Mathd::RandomMag();
			}
			double Random::Getd()
			{
				return Compute::Mathd::Random();
			}
			float Random::Betweenf(float Min, float Max)
			{
				return Compute::Mathf::Random(Min, Max);
			}
			float Random::Magf()
			{
				return Compute::Mathf::RandomMag();
			}
			float Random::Getf()
			{
				return Compute::Mathf::Random();
			}
			uint64_t Random::Betweeni(uint64_t Min, uint64_t Max)
			{
				return Compute::Math<uint64_t>::Random(Min, Max);
			}

			Promise::Promise(asIScriptContext* NewContext) noexcept : Engine(nullptr), Context(NewContext), Callback(nullptr), Value(-1), RefCount(1)
			{
				VI_ASSERT_V(Context != nullptr, "context should not be null");
				Context->AddRef();
				Engine = Context->GetEngine();
				Engine->NotifyGarbageCollectorOfNewObject(this, Engine->GetTypeInfoByName(TYPENAME_PROMISE));
			}
			void Promise::Release()
			{
				RefCount &= 0x7FFFFFFF;
				if (--RefCount <= 0)
				{
					ReleaseReferences(nullptr);
					this->~Promise();
					asFreeMem((void*)this);
				}
			}
			void Promise::AddRef()
			{
				RefCount = (RefCount & 0x7FFFFFFF) + 1;
			}
			void Promise::EnumReferences(asIScriptEngine* OtherEngine)
			{
				if (Value.Object != nullptr && (Value.TypeId & asTYPEID_MASK_OBJECT))
				{
					asITypeInfo* SubType = Engine->GetTypeInfoById(Value.TypeId);
					if ((SubType->GetFlags() & asOBJ_REF))
						OtherEngine->GCEnumCallback(Value.Object);
					else if ((SubType->GetFlags() & asOBJ_VALUE) && (SubType->GetFlags() & asOBJ_GC))
						Engine->ForwardGCEnumReferences(Value.Object, SubType);

					asITypeInfo* Type = OtherEngine->GetTypeInfoById(Value.TypeId);
					if (Type != nullptr)
						OtherEngine->GCEnumCallback(Type);
				}

				if (Callback != nullptr)
					OtherEngine->GCEnumCallback(Callback);
			}
			void Promise::ReleaseReferences(asIScriptEngine*)
			{
				if (Value.TypeId & asTYPEID_MASK_OBJECT)
				{
					asITypeInfo* Type = Engine->GetTypeInfoById(Value.TypeId);
					Engine->ReleaseScriptObject(Value.Object, Type);
					if (Type != nullptr)
						Type->Release();
					Value.Clean();
				}

				if (Callback != nullptr)
				{
					Callback->Release();
					Callback = nullptr;
				}

				if (Context != nullptr)
				{
					Context->Release();
					Context = nullptr;
				}
			}
			void Promise::SetFlag()
			{
				RefCount |= 0x80000000;
			}
			bool Promise::GetFlag()
			{
				return (RefCount & 0x80000000) ? true : false;
			}
			int Promise::GetRefCount()
			{
				return (RefCount & 0x7FFFFFFF);
			}
			int Promise::GetTypeId()
			{
				return Value.TypeId;
			}
			void* Promise::GetAddressOfObject()
			{
				return Retrieve();
			}
			void Promise::When(asIScriptFunction* NewCallback)
			{
				if (Callback != nullptr)
					Callback->Release();

				Callback = NewCallback;
				if (Callback != nullptr)
					Callback->AddRef();
			}
			void Promise::Store(void* RefPointer, int RefTypeId)
			{
				VI_ASSERT_V(Value.TypeId == PromiseNULL, "promise should be settled only once");
				VI_ASSERT_V(RefPointer != nullptr || RefTypeId == asTYPEID_VOID, "input pointer should not be null");
				VI_ASSERT_V(Engine != nullptr, "promise is malformed (engine is null)");
				VI_ASSERT_V(Context != nullptr, "promise is malformed (context is null)");

				Update.lock();
				if (Value.TypeId == PromiseNULL)
				{
					if ((RefTypeId & asTYPEID_MASK_OBJECT))
					{
						asITypeInfo* Type = Engine->GetTypeInfoById(RefTypeId);
						if (Type != nullptr)
							Type->AddRef();
					}

					Value.TypeId = RefTypeId;
					if (Value.TypeId & asTYPEID_OBJHANDLE)
					{
						Value.Object = *(void**)RefPointer;
					}
					else if (Value.TypeId & asTYPEID_MASK_OBJECT)
					{
						Value.Object = Engine->CreateScriptObjectCopy(RefPointer, Engine->GetTypeInfoById(Value.TypeId));
					}
					else
					{
						Value.Integer = 0;
						int Size = Engine->GetSizeOfPrimitiveType(Value.TypeId);
						memcpy(&Value.Integer, RefPointer, Size);
					}

					bool SuspendOwned = Context->GetUserData(PromiseUD) == (void*)this;
					if (SuspendOwned)
						Context->SetUserData(nullptr, PromiseUD);

					bool WantsResume = (Context->GetState() != asEXECUTION_ACTIVE && SuspendOwned);
					ImmediateContext* Immediate = ImmediateContext::Get(Context);
					Promise* Base = this;
					Update.unlock();

					if (Callback != nullptr)
					{
						AddRef();
						Immediate->Execute(Callback, [Base](ImmediateContext* Context)
						{
							Context->SetArgAddress(0, Base->GetAddressOfObject());
						}).When([Base](int&&)
						{
							Base->Callback->Release();
							Base->Callback = nullptr;
							Base->Release();
						});
					}

					if (WantsResume)
					{
						Core::Schedule::Get()->SetTask([Base, Immediate]()
						{
							Immediate->Resume();
							Base->Release();
						}, Core::Difficulty::Light);
					}
					else if (SuspendOwned)
						Release();
				}
				else
				{
					asIScriptContext* ThisContext = asGetActiveContext();
					if (!ThisContext)
						ThisContext = Context;

					ThisContext->SetException("promise is already fulfilled");
					Update.unlock();
				}
			}
			void Promise::Store(void* RefPointer, const char* TypeName)
			{
				VI_ASSERT_V(Engine != nullptr, "promise is malformed (engine is null)");
				VI_ASSERT_V(TypeName != nullptr, "typename should not be null");
				Store(RefPointer, Engine->GetTypeIdByDecl(TypeName));
			}
			void Promise::StoreVoid()
			{
				Store(nullptr, asTYPEID_VOID);
			}
			bool Promise::Retrieve(void* RefPointer, int RefTypeId)
			{
				VI_ASSERT(Engine != nullptr, false, "promise is malformed (engine is null)");
				VI_ASSERT(RefPointer != nullptr, false, "output pointer should not be null");
				if (Value.TypeId == PromiseNULL)
					return false;

				if (RefTypeId & asTYPEID_OBJHANDLE)
				{
					if ((Value.TypeId & asTYPEID_MASK_OBJECT))
					{
						if ((Value.TypeId & asTYPEID_HANDLETOCONST) && !(RefTypeId & asTYPEID_HANDLETOCONST))
							return false;

						Engine->RefCastObject(Value.Object, Engine->GetTypeInfoById(Value.TypeId), Engine->GetTypeInfoById(RefTypeId), reinterpret_cast<void**>(RefPointer));
						if (*(asPWORD*)RefPointer == 0)
							return false;

						return true;
					}
				}
				else if (RefTypeId & asTYPEID_MASK_OBJECT)
				{
					if (Value.TypeId == RefTypeId)
					{
						Engine->AssignScriptObject(RefPointer, Value.Object, Engine->GetTypeInfoById(Value.TypeId));
						return true;
					}
				}
				else
				{
					int Size1 = Engine->GetSizeOfPrimitiveType(Value.TypeId);
					int Size2 = Engine->GetSizeOfPrimitiveType(RefTypeId);
					VI_ASSERT(Size1 == Size2, false, "cannot map incompatible primitive types");

					if (Size1 == Size2)
					{
						memcpy(RefPointer, &Value.Integer, Size1);
						return true;
					}
				}

				return false;
			}
			void Promise::RetrieveVoid()
			{
			}
			void* Promise::Retrieve()
			{
				if (Value.TypeId == PromiseNULL)
					return nullptr;

				if (Value.TypeId & asTYPEID_OBJHANDLE)
					return &Value.Object;
				else if (Value.TypeId & asTYPEID_MASK_OBJECT)
					return Value.Object;
				else if (Value.TypeId <= asTYPEID_DOUBLE || Value.TypeId & asTYPEID_MASK_SEQNBR)
					return &Value.Integer;

				return nullptr;
			}
			bool Promise::IsPending()
			{
				return Value.TypeId == PromiseNULL;
			}
			Promise* Promise::YieldIf()
			{
				std::unique_lock<std::mutex> Unique(Update);
				if (Value.TypeId == PromiseNULL && Context != nullptr)
				{
					AddRef();
					Context->SetUserData(this, PromiseUD);
					Context->Suspend();
				}

				return this;
			}
			Promise* Promise::Create()
			{
				return new(asAllocMem(sizeof(Promise))) Promise(asGetActiveContext());
			}
			Promise* Promise::CreateFactory(void* _Ref, int TypeId)
			{
				Promise* Future = new(asAllocMem(sizeof(Promise))) Promise(asGetActiveContext());
				if (TypeId != asTYPEID_VOID)
					Future->Store(_Ref, TypeId);
				return Future;
			}
			Promise* Promise::CreateFactoryVoid()
			{
				return Create();
			}
			bool Promise::TemplateCallback(asITypeInfo* Info, bool& DontGarbageCollect)
			{
				int TypeId = Info->GetSubTypeId();
				if (TypeId == asTYPEID_VOID)
					return false;

				if ((TypeId & asTYPEID_MASK_OBJECT) && !(TypeId & asTYPEID_OBJHANDLE))
				{
					asIScriptEngine* Engine = Info->GetEngine();
					asITypeInfo* SubType = Engine->GetTypeInfoById(TypeId);
					asDWORD Flags = SubType->GetFlags();

					if ((Flags & asOBJ_VALUE) && !(Flags & asOBJ_POD))
					{
						bool Found = false;
						for (size_t i = 0; i < SubType->GetBehaviourCount(); i++)
						{
							asEBehaviours Behaviour;
							asIScriptFunction* Func = SubType->GetBehaviourByIndex((int)i, &Behaviour);
							if (Behaviour != asBEHAVE_CONSTRUCT)
								continue;

							if (Func->GetParamCount() == 0)
							{
								Found = true;
								break;
							}
						}

						if (!Found)
						{
							Engine->WriteMessage(TYPENAME_PROMISE, 0, 0, asMSGTYPE_ERROR, "The subtype has no default constructor");
							return false;
						}
					}
					else if ((Flags & asOBJ_REF))
					{
						bool Found = false;
						if (!Engine->GetEngineProperty(asEP_DISALLOW_VALUE_ASSIGN_FOR_REF_TYPE))
						{
							for (size_t i = 0; i < SubType->GetFactoryCount(); i++)
							{
								asIScriptFunction* Function = SubType->GetFactoryByIndex((int)i);
								if (Function->GetParamCount() == 0)
								{
									Found = true;
									break;
								}
							}
						}

						if (!Found)
						{
							Engine->WriteMessage(TYPENAME_PROMISE, 0, 0, asMSGTYPE_ERROR, "The subtype has no default factory");
							return false;
						}
					}

					if (!(Flags & asOBJ_GC))
						DontGarbageCollect = true;
				}
				else if (!(TypeId & asTYPEID_OBJHANDLE))
				{
					DontGarbageCollect = true;
				}
				else
				{
					asITypeInfo* SubType = Info->GetEngine()->GetTypeInfoById(TypeId);
					asDWORD Flags = SubType->GetFlags();

					if (!(Flags & asOBJ_GC))
					{
						if ((Flags & asOBJ_SCRIPT_OBJECT))
						{
							if ((Flags & asOBJ_NOINHERIT))
								DontGarbageCollect = true;
						}
						else
							DontGarbageCollect = true;
					}
				}

				return true;
			}
			bool Promise::GeneratorCallback(const Core::String&, Core::String& Code)
			{
				Core::Stringify(&Code).Replace("promise<void>", "promise_v");
				FunctionFactory::ReplacePreconditions("co_await", Code, [](const Core::String& Expression)
				{
					return Expression + ".yield().unwrap()";
				});
				return true;
			}
			Core::String Promise::GetStatus(ImmediateContext* Context)
			{
				VI_ASSERT(Context != nullptr, Core::String(), "context should be set");

				Core::String Result;
				switch (Context->GetState())
				{
					case Activation::FINISHED:
						Result = "FIN";
						break;
					case Activation::SUSPENDED:
						Result = "SUSP";
						break;
					case Activation::ABORTED:
						Result = "ABRT";
						break;
					case Activation::EXCEPTION:
						Result = "EXCE";
						break;
					case Activation::PREPARED:
						Result = "PREP";
						break;
					case Activation::ACTIVE:
						Result = "ACTV";
						break;
					case Activation::ERR:
						Result = "ERR";
						break;
					default:
						Result = "INIT";
						break;
				}

				Promise* Base = (Promise*)Context->GetUserData(PromiseUD);
				if (Base != nullptr)
				{
					const char* Format = " in pending promise on 0x%" PRIXPTR " %s";
					if (!Base->IsPending())
						Result += Core::Form(Format, (uintptr_t)Base, "that was fulfilled").R();
					else
						Result += Core::Form(Format, (uintptr_t)Base, "that was not fulfilled").R();
				}

				return Result;
			}
			int Promise::PromiseNULL = -1;
			int Promise::PromiseUD = 559;

			Core::Decimal DecimalNegate(Core::Decimal& Base)
			{
				return -Base;
			}
			Core::Decimal& DecimalMulEq(Core::Decimal& Base, const Core::Decimal& V)
			{
				Base *= V;
				return Base;
			}
			Core::Decimal& DecimalDivEq(Core::Decimal& Base, const Core::Decimal& V)
			{
				Base /= V;
				return Base;
			}
			Core::Decimal& DecimalAddEq(Core::Decimal& Base, const Core::Decimal& V)
			{
				Base += V;
				return Base;
			}
			Core::Decimal& DecimalSubEq(Core::Decimal& Base, const Core::Decimal& V)
			{
				Base -= V;
				return Base;
			}
			Core::Decimal& DecimalPP(Core::Decimal& Base)
			{
				Base++;
				return Base;
			}
			Core::Decimal& DecimalMM(Core::Decimal& Base)
			{
				Base--;
				return Base;
			}
			bool DecimalEq(Core::Decimal& Base, const Core::Decimal& Right)
			{
				return Base == Right;
			}
			int DecimalCmp(Core::Decimal& Base, const Core::Decimal& Right)
			{
				if (Base == Right)
					return 0;

				return Base > Right ? 1 : -1;
			}
			Core::Decimal DecimalAdd(const Core::Decimal& Left, const Core::Decimal& Right)
			{
				return Left + Right;
			}
			Core::Decimal DecimalSub(const Core::Decimal& Left, const Core::Decimal& Right)
			{
				return Left - Right;
			}
			Core::Decimal DecimalMul(const Core::Decimal& Left, const Core::Decimal& Right)
			{
				return Left * Right;
			}
			Core::Decimal DecimalDiv(const Core::Decimal& Left, const Core::Decimal& Right)
			{
				return Left / Right;
			}
			Core::Decimal DecimalPer(const Core::Decimal& Left, const Core::Decimal& Right)
			{
				return Left % Right;
			}

			Core::DateTime& DateTimeAddEq(Core::DateTime& Base, const Core::DateTime& V)
			{
				Base += V;
				return Base;
			}
			Core::DateTime& DateTimeSubEq(Core::DateTime& Base, const Core::DateTime& V)
			{
				Base -= V;
				return Base;
			}
			Core::DateTime& DateTimeSet(Core::DateTime& Base, const Core::DateTime& V)
			{
				Base = V;
				return Base;
			}
			bool DateTimeEq(Core::DateTime& Base, const Core::DateTime& Right)
			{
				return Base == Right;
			}
			int DateTimeCmp(Core::DateTime& Base, const Core::DateTime& Right)
			{
				if (Base == Right)
					return 0;

				return Base > Right ? 1 : -1;
			}
			Core::DateTime DateTimeAdd(const Core::DateTime& Left, const Core::DateTime& Right)
			{
				return Left + Right;
			}
			Core::DateTime DateTimeSub(const Core::DateTime& Left, const Core::DateTime& Right)
			{
				return Left - Right;
			}

			size_t VariantGetSize(Core::Variant& Base)
			{
				return Base.GetSize();
			}
			bool VariantEquals(Core::Variant& Base, const Core::Variant& Other)
			{
				return Base == Other;
			}
			bool VariantImplCast(Core::Variant& Base)
			{
				return Base;
			}

			void SchemaNotifyAllReferences(Core::Schema* Base, asIScriptEngine* Engine, asITypeInfo* Type)
			{
				Engine->NotifyGarbageCollectorOfNewObject(Base, Type);
				for (auto* Item : Base->GetChilds())
					SchemaNotifyAllReferences(Item, Engine, Type);
			}
			Core::Schema* SchemaInit(Core::Schema* Base)
			{
				return Base;
			}
			Core::Schema* SchemaConstructBuffer(unsigned char* Buffer)
			{
				if (!Buffer)
					return nullptr;

				Core::Schema* Result = Core::Var::Set::Object();
				asIScriptContext* Context = asGetActiveContext();
				asIScriptEngine* VM = Context->GetEngine();
				size_t Length = *(asUINT*)Buffer;
				Buffer += 4;

				while (Length--)
				{
					if (asPWORD(Buffer) & 0x3)
						Buffer += 4 - (asPWORD(Buffer) & 0x3);

					Core::String Name = *(Core::String*)Buffer;
					Buffer += sizeof(Core::String);

					int TypeId = *(int*)Buffer;
					Buffer += sizeof(int);

					void* Ref = (void*)Buffer;
					if (TypeId >= asTYPEID_BOOL && TypeId <= asTYPEID_DOUBLE)
					{
						switch (TypeId)
						{
							case asTYPEID_BOOL:
								Result->Set(Name, Core::Var::Boolean(*(bool*)Ref));
								break;
							case asTYPEID_INT8:
								Result->Set(Name, Core::Var::Integer(*(char*)Ref));
								break;
							case asTYPEID_INT16:
								Result->Set(Name, Core::Var::Integer(*(short*)Ref));
								break;
							case asTYPEID_INT32:
								Result->Set(Name, Core::Var::Integer(*(int*)Ref));
								break;
							case asTYPEID_UINT8:
								Result->Set(Name, Core::Var::Integer(*(unsigned char*)Ref));
								break;
							case asTYPEID_UINT16:
								Result->Set(Name, Core::Var::Integer(*(unsigned short*)Ref));
								break;
							case asTYPEID_UINT32:
								Result->Set(Name, Core::Var::Integer(*(unsigned int*)Ref));
								break;
							case asTYPEID_INT64:
							case asTYPEID_UINT64:
								Result->Set(Name, Core::Var::Integer(*(asINT64*)Ref));
								break;
							case asTYPEID_FLOAT:
								Result->Set(Name, Core::Var::Number(*(float*)Ref));
								break;
							case asTYPEID_DOUBLE:
								Result->Set(Name, Core::Var::Number(*(double*)Ref));
								break;
						}
					}
					else
					{
						asITypeInfo* Type = VM->GetTypeInfoById(TypeId);
						if ((TypeId & asTYPEID_MASK_OBJECT) && !(TypeId & asTYPEID_OBJHANDLE) && (Type && Type->GetFlags() & asOBJ_REF))
							Ref = *(void**)Ref;

						if (TypeId & asTYPEID_OBJHANDLE)
							Ref = *(void**)Ref;

						if (VirtualMachine::Get(VM)->IsNullable(TypeId) || !Ref)
						{
							Result->Set(Name, Core::Var::Null());
						}
						else if (Type && !strcmp(TYPENAME_SCHEMA, Type->GetName()))
						{
							Core::Schema* Base = (Core::Schema*)Ref;
							if (Base->GetParent() != Result)
								Base->AddRef();

							Result->Set(Name, Base);
						}
						else if (Type && !strcmp(TYPENAME_STRING, Type->GetName()))
							Result->Set(Name, Core::Var::String(*(Core::String*)Ref));
						else if (Type && !strcmp(TYPENAME_DECIMAL, Type->GetName()))
							Result->Set(Name, Core::Var::Decimal(*(Core::Decimal*)Ref));
					}

					if (TypeId & asTYPEID_MASK_OBJECT)
					{
						asITypeInfo* T = VM->GetTypeInfoById(TypeId);
						if (T->GetFlags() & asOBJ_VALUE)
							Buffer += T->GetSize();
						else
							Buffer += sizeof(void*);
					}
					else if (TypeId == 0)
					{
						VI_WARN("[memerr] use nullptr instead of null for initialization lists");
						Buffer += sizeof(void*);
					}
					else
						Buffer += VM->GetSizeOfPrimitiveType(TypeId);
				}

				return Result;
			}
			void SchemaConstruct(asIScriptGeneric* Generic)
			{
				asIScriptEngine* Engine = Generic->GetEngine();
				unsigned char* Buffer = (unsigned char*)Generic->GetArgAddress(0);
				Core::Schema* New = SchemaConstructBuffer(Buffer);
				SchemaNotifyAllReferences(New, Engine, Engine->GetTypeInfoByName(TYPENAME_SCHEMA));
				*(Core::Schema**)Generic->GetAddressOfReturnLocation() = New;
			}
			Core::Schema* SchemaGetIndex(Core::Schema* Base, const Core::String& Name)
			{
				Core::Schema* Result = Base->Fetch(Name);
				if (Result != nullptr)
					return Result;

				return Base->Set(Name, Core::Var::Undefined());
			}
			Core::Schema* SchemaGetIndexOffset(Core::Schema* Base, size_t Offset)
			{
				return Base->Get(Offset);
			}
			Core::Schema* SchemaSet(Core::Schema* Base, const Core::String& Name, Core::Schema* Value)
			{
				if (Value != nullptr && Value->GetParent() != Base)
					Value->AddRef();

				return Base->Set(Name, Value);
			}
			Core::Schema* SchemaPush(Core::Schema* Base, Core::Schema* Value)
			{
				if (Value != nullptr)
					Value->AddRef();

				return Base->Push(Value);
			}
			Core::Schema* SchemaFirst(Core::Schema* Base)
			{
				auto& Childs = Base->GetChilds();
				return Childs.empty() ? nullptr : Childs.front();
			}
			Core::Schema* SchemaLast(Core::Schema* Base)
			{
				auto& Childs = Base->GetChilds();
				return Childs.empty() ? nullptr : Childs.back();
			}
			Core::Variant SchemaFirstVar(Core::Schema* Base)
			{
				auto& Childs = Base->GetChilds();
				return Childs.empty() ? Core::Var::Undefined() : Childs.front()->Value;
			}
			Core::Variant SchemaLastVar(Core::Schema* Base)
			{
				auto& Childs = Base->GetChilds();
				return Childs.empty() ? Core::Var::Undefined() : Childs.back()->Value;
			}
			Array* SchemaGetCollection(Core::Schema* Base, const Core::String& Name, bool Deep)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				if (!Context)
					return nullptr;

				VirtualMachine* VM = Context->GetVM();
				if (!VM)
					return nullptr;

				Core::Vector<Core::Schema*> Nodes = Base->FetchCollection(Name, Deep);
				for (auto& Node : Nodes)
					Node->AddRef();

				TypeInfo Type = VM->GetTypeInfoByDecl(TYPENAME_ARRAY "<" TYPENAME_SCHEMA "@>@");
				return Array::Compose(Type.GetTypeInfo(), Nodes);
			}
			Array* SchemaGetChilds(Core::Schema* Base)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				if (!Context)
					return nullptr;

				VirtualMachine* VM = Context->GetVM();
				if (!VM)
					return nullptr;

				TypeInfo Type = VM->GetTypeInfoByDecl(TYPENAME_ARRAY "<" TYPENAME_SCHEMA "@>@");
				return Array::Compose(Type.GetTypeInfo(), Base->GetChilds());
			}
			Array* SchemaGetAttributes(Core::Schema* Base)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				if (!Context)
					return nullptr;

				VirtualMachine* VM = Context->GetVM();
				if (!VM)
					return nullptr;

				TypeInfo Type = VM->GetTypeInfoByDecl(TYPENAME_ARRAY "<" TYPENAME_SCHEMA "@>@");
				return Array::Compose(Type.GetTypeInfo(), Base->GetAttributes());
			}
			Dictionary* SchemaGetNames(Core::Schema* Base)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				if (!Context)
					return nullptr;

				VirtualMachine* VM = Context->GetVM();
				if (!VM)
					return nullptr;

				Core::UnorderedMap<Core::String, size_t> Mapping = Base->GetNames();
				Dictionary* Map = Dictionary::Create(VM->GetEngine());

				for (auto& Item : Mapping)
				{
					int64_t Index = (int64_t)Item.second;
					Map->Set(Item.first, &Index, (int)TypeId::INT64);
				}

				return Map;
			}
			size_t SchemaGetSize(Core::Schema* Base)
			{
				return Base->Size();
			}
			Core::String SchemaToJSON(Core::Schema* Base)
			{
				Core::String Stream;
				Core::Schema::ConvertToJSON(Base, [&Stream](Core::VarForm, const char* Buffer, size_t Length)
				{
					if (Buffer != nullptr && Length > 0)
						Stream.append(Buffer, Length);
				});

				return Stream;
			}
			Core::String SchemaToXML(Core::Schema* Base)
			{
				Core::String Stream;
				Core::Schema::ConvertToXML(Base, [&Stream](Core::VarForm, const char* Buffer, size_t Length)
				{
					if (Buffer != nullptr && Length > 0)
						Stream.append(Buffer, Length);
				});

				return Stream;
			}
			Core::String SchemaToString(Core::Schema* Base)
			{
				switch (Base->Value.GetType())
				{
					case Core::VarType::Null:
					case Core::VarType::Undefined:
					case Core::VarType::Object:
					case Core::VarType::Array:
					case Core::VarType::Pointer:
						break;
					case Core::VarType::String:
					case Core::VarType::Binary:
						return Base->Value.GetBlob();
					case Core::VarType::Integer:
						return Core::ToString(Base->Value.GetInteger());
					case Core::VarType::Number:
						return Core::ToString(Base->Value.GetNumber());
					case Core::VarType::Decimal:
						return Base->Value.GetDecimal().ToString();
					case Core::VarType::Boolean:
						return Base->Value.GetBoolean() ? "1" : "0";
				}

				return "";
			}
			Core::String SchemaToBinary(Core::Schema* Base)
			{
				return Base->Value.GetBlob();
			}
			int64_t SchemaToInteger(Core::Schema* Base)
			{
				return Base->Value.GetInteger();
			}
			double SchemaToNumber(Core::Schema* Base)
			{
				return Base->Value.GetNumber();
			}
			Core::Decimal SchemaToDecimal(Core::Schema* Base)
			{
				return Base->Value.GetDecimal();
			}
			bool SchemaToBoolean(Core::Schema* Base)
			{
				return Base->Value.GetBoolean();
			}
			Core::Schema* SchemaFromJSON(const Core::String& Value)
			{
				return Core::Schema::ConvertFromJSON(Value.c_str(), Value.size(), false);
			}
			Core::Schema* SchemaFromXML(const Core::String& Value)
			{
				return Core::Schema::ConvertFromXML(Value.c_str(), false);
			}
			Core::Schema* SchemaImport(const Core::String& Value)
			{
				VirtualMachine* VM = VirtualMachine::Get();
				if (!VM)
					return nullptr;

				return VM->ImportJSON(Value);
			}
			Core::Schema* SchemaCopyAssign(Core::Schema* Base, const Core::Variant& Other)
			{
				Base->Value = Other;
				return Base;
			}
			bool SchemaEquals(Core::Schema* Base, Core::Schema* Other)
			{
				if (Other != nullptr)
					return Base->Value == Other->Value;

				Core::VarType Type = Base->Value.GetType();
				return (Type == Core::VarType::Null || Type == Core::VarType::Undefined);
			}
			void SchemaEnumRefs(Core::Schema* Base, asIScriptEngine* Engine)
			{
				Engine->GCEnumCallback(Base->GetParent());
				for (auto* Item : Base->GetChilds())
				{
					Engine->GCEnumCallback(Item);
					SchemaEnumRefs(Item, Engine);
				}
			}
#ifdef VI_HAS_BINDINGS
			template <typename T>
			Core::String GetComponentName(T* Base)
			{
				return Base->GetName();
			}
			template <typename T>
			void PopulateComponent(RefClass& Class)
			{
				Class.SetMethodEx("string get_name() const", &GetComponentName<T>);
				Class.SetMethod("uint64 get_id() const", &T::GetId);
			}
			Core::VariantArgs ToVariantKeys(Core::Schema* Args)
			{
				Core::VariantArgs Keys;
				if (!Args || Args->IsEmpty())
					return Keys;

				Keys.reserve(Args->Size());
				for (auto* Item : Args->GetChilds())
					Keys[Item->Key] = Item->Value;

				return Keys;
			}

			Mutex::Mutex() noexcept : Ref(1)
			{
			}
			void Mutex::Release()
			{
				if (asAtomicDec(Ref) <= 0)
				{
					this->~Mutex();
					asFreeMem((void*)this);
				}
			}
			void Mutex::AddRef()
			{
				asAtomicInc(Ref);
			}
			bool Mutex::TryLock()
			{
				return Base.try_lock();
			}
			void Mutex::Lock()
			{
				Base.lock();
			}
			void Mutex::Unlock()
			{
				Base.unlock();
			}
			Mutex* Mutex::Factory()
			{
				void* Data = asAllocMem(sizeof(Mutex));
				if (!Data)
				{
					asIScriptContext* Context = asGetActiveContext();
					if (Context != nullptr)
						Context->SetException("out of memory");

					return nullptr;
				}

				return new(Data) Mutex();
			}

			Thread::Thread(asIScriptEngine* Engine, asIScriptFunction* Func) noexcept : Function(Func), VM(VirtualMachine::Get(Engine)), Context(nullptr), Flag(false), Sparcing(0), RefCount(1)
			{
				Engine->NotifyGarbageCollectorOfNewObject(this, Engine->GetTypeInfoByName(TYPENAME_THREAD));
			}
			void Thread::InvokeRoutine()
			{
				{
					std::unique_lock<std::recursive_mutex> Unique(Mutex);
					if (!Function)
						return Release();

					if (Context == nullptr)
						Context = VM->CreateContext();

					if (Context == nullptr)
					{
						VM->GetEngine()->WriteMessage(TYPENAME_THREAD, 0, 0, asMSGTYPE_ERROR, "failed to start a thread: no available context");
						return Release();
					}
				}
				Context->Execute(Function, [this](ImmediateContext* Context)
				{
					Context->SetArgObject(0, this);
					Context->SetUserData(this, ContextUD);
				}).When([this](int&& State)
				{
					std::unique_lock<std::recursive_mutex> Unique(Mutex);
					Context->SetUserData(nullptr, ContextUD);
					if (State != asEXECUTION_SUSPENDED)
					{
						Except = Exception::Pointer(Context->GetContext());
						Context->Unprepare();
					}
					if (Sparcing == 0)
						Release();
					else
						Sparcing = 2;
				});
				asThreadCleanup();
			}
			void Thread::ResumeRoutine()
			{
				Mutex.lock();
				Sparcing = 1;

				if (Context && Context->GetState() == Activation::SUSPENDED)
				{
					Mutex.unlock();
					Context->Resume();
					Mutex.lock();
				}

				if (Sparcing == 2)
					Release();

				Mutex.unlock();
				asThreadCleanup();
			}
			void Thread::AddRef()
			{
				Flag = false;
				asAtomicInc(RefCount);
			}
			void Thread::Suspend()
			{
				Mutex.lock();
				Sparcing = 1;

				if (Context && Context->GetState() != Activation::SUSPENDED)
				{
					Mutex.unlock();
					Context->Suspend();
				}
				else
					Mutex.unlock();
			}
			void Thread::Resume()
			{
				Mutex.lock(); Join();
				Procedure = std::thread(&Thread::ResumeRoutine, this);
				VI_DEBUG("[vm] resume thread at %s", Core::OS::Process::GetThreadId(Procedure.get_id()).c_str());
				Mutex.unlock();
			}
			void Thread::Release()
			{
				Flag = false;
				if (asAtomicDec(RefCount) <= 0)
				{
					ReleaseReferences(nullptr);
					this->~Thread();
					asFreeMem((void*)this);
				}
			}
			void Thread::SetGCFlag()
			{
				Flag = true;
			}
			bool Thread::GetGCFlag()
			{
				return Flag;
			}
			int Thread::GetRefCount()
			{
				return RefCount;
			}
			void Thread::EnumReferences(asIScriptEngine* Engine)
			{
				for (int i = 0; i < 2; i++)
				{
					for (auto Any : Pipe[i].Queue)
					{
						if (Any != nullptr)
							Engine->GCEnumCallback(Any);
					}
				}

				Engine->GCEnumCallback(Engine);
				if (Context != nullptr)
					Engine->GCEnumCallback(Context);

				if (Function != nullptr)
					Engine->GCEnumCallback(Function);
			}
			int Thread::Join(uint64_t Timeout)
			{
				if (std::this_thread::get_id() == Procedure.get_id())
					return -1;

				{
					std::unique_lock<std::recursive_mutex> Unique(Mutex);
					if (!Procedure.joinable())
						return -1;

					VI_DEBUG("[vm] join thread %s", Core::OS::Process::GetThreadId(Procedure.get_id()).c_str());
				}
				Procedure.join();

				std::unique_lock<std::recursive_mutex> Unique(Mutex);
				if (!Except.Empty())
					Exception::Throw(Except);

				return 1;
			}
			int Thread::Join()
			{
				if (std::this_thread::get_id() == Procedure.get_id())
					return -1;

				while (true)
				{
					int R = Join(1000);
					if (R == -1 || R == 1)
						return R;
				}

				return 0;
			}
			Core::String Thread::GetId() const
			{
				return Core::OS::Process::GetThreadId(Procedure.get_id());
			}
			void Thread::Push(void* _Ref, int TypeId)
			{
				auto* _Thread = GetThread();
				int Id = (_Thread == this ? 1 : 0);

				void* Data = asAllocMem(sizeof(Any));
				Any* Next = new(Data) Any(_Ref, TypeId, VirtualMachine::Get()->GetEngine());
				Pipe[Id].Mutex.lock();
				Pipe[Id].Queue.push_back(Next);
				Pipe[Id].Mutex.unlock();
				Pipe[Id].CV.notify_one();
			}
			bool Thread::Pop(void* _Ref, int TypeId)
			{
				bool Resolved = false;
				while (!Resolved)
					Resolved = Pop(_Ref, TypeId, 1000);

				return true;
			}
			bool Thread::Pop(void* _Ref, int TypeId, uint64_t Timeout)
			{
				auto* _Thread = GetThread();
				int Id = (_Thread == this ? 0 : 1);

				std::unique_lock<std::mutex> Guard(Pipe[Id].Mutex);
				if (!Pipe[Id].CV.wait_for(Guard, std::chrono::milliseconds(Timeout), [&]
				{
					return Pipe[Id].Queue.size() != 0;
				}))
					return false;

				Any* Result = Pipe[Id].Queue.front();
				if (!Result->Retrieve(_Ref, TypeId))
					return false;

				Pipe[Id].Queue.erase(Pipe[Id].Queue.begin());
				Result->Release();

				return true;
			}
			bool Thread::IsActive()
			{
				std::unique_lock<std::recursive_mutex> Unique(Mutex);
				return (Context && Context->GetState() != Activation::SUSPENDED);
			}
			bool Thread::Start()
			{
				std::unique_lock<std::recursive_mutex> Unique(Mutex);
				if (!Function)
					return false;

				if (Context != nullptr)
				{
					if (Context->GetState() != Activation::SUSPENDED)
						return false;
					Join();
				}
				else if (Procedure.joinable())
				{
					if (std::this_thread::get_id() == Procedure.get_id())
						return false;
					Join();
				}

				AddRef();
				Procedure = std::thread(&Thread::InvokeRoutine, this);
				VI_DEBUG("[vm] spawn thread %s", Core::OS::Process::GetThreadId(Procedure.get_id()).c_str());
				return true;
			}
			void Thread::ReleaseReferences(asIScriptEngine*)
			{
				Join();
				for (int i = 0; i < 2; i++)
				{
					Pipe[i].Mutex.lock();
					for (auto Any : Pipe[i].Queue)
					{
						if (Any != nullptr)
							Any->Release();
					}
					Pipe[i].Queue.clear();
					Pipe[i].Mutex.unlock();
				}

				std::unique_lock<std::recursive_mutex> Unique(Mutex);
				if (Function)
					Function->Release();

				VI_CLEAR(Context);
				VM = nullptr;
				Function = nullptr;
			}
			void Thread::Create(asIScriptGeneric* Generic)
			{
				asIScriptEngine* Engine = Generic->GetEngine();
				asIScriptFunction* Function = *(asIScriptFunction**)Generic->GetAddressOfArg(0);
				void* Data = asAllocMem(sizeof(Thread));
				*(Thread**)Generic->GetAddressOfReturnLocation() = new(Data) Thread(Engine, Function);
			}
			Thread* Thread::GetThread()
			{
				asIScriptContext* Context = asGetActiveContext();
				if (!Context)
					return nullptr;

				return static_cast<Thread*>(Context->GetUserData(ContextUD));
			}
			void Thread::ThreadSleep(uint64_t Timeout)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(Timeout));
			}
			void Thread::ThreadSuspend()
			{
				asIScriptContext* Context = asGetActiveContext();
				if (Context && Context->GetState() != asEXECUTION_SUSPENDED)
					Context->Suspend();
			}
			Core::String Thread::GetThreadId()
			{
				return Core::OS::Process::GetThreadId(std::this_thread::get_id());
			}
			int Thread::ContextUD = 550;
			int Thread::EngineListUD = 551;

			Complex::Complex() noexcept
			{
				R = 0;
				I = 0;
			}
			Complex::Complex(const Complex& Other) noexcept
			{
				R = Other.R;
				I = Other.I;
			}
			Complex::Complex(float _R, float _I) noexcept
			{
				R = _R;
				I = _I;
			}
			bool Complex::operator==(const Complex& Other) const
			{
				return (R == Other.R) && (I == Other.I);
			}
			bool Complex::operator!=(const Complex& Other) const
			{
				return !(*this == Other);
			}
			Complex& Complex::operator=(const Complex& Other) noexcept
			{
				R = Other.R;
				I = Other.I;
				return *this;
			}
			Complex& Complex::operator+=(const Complex& Other)
			{
				R += Other.R;
				I += Other.I;
				return *this;
			}
			Complex& Complex::operator-=(const Complex& Other)
			{
				R -= Other.R;
				I -= Other.I;
				return *this;
			}
			Complex& Complex::operator*=(const Complex& Other)
			{
				*this = *this * Other;
				return *this;
			}
			Complex& Complex::operator/=(const Complex& Other)
			{
				*this = *this / Other;
				return *this;
			}
			float Complex::SquaredLength() const
			{
				return R * R + I * I;
			}
			float Complex::Length() const
			{
				return sqrtf(SquaredLength());
			}
			Complex Complex::operator+(const Complex& Other) const
			{
				return Complex(R + Other.R, I + Other.I);
			}
			Complex Complex::operator-(const Complex& Other) const
			{
				return Complex(R - Other.R, I + Other.I);
			}
			Complex Complex::operator*(const Complex& Other) const
			{
				return Complex(R * Other.R - I * Other.I, R * Other.I + I * Other.R);
			}
			Complex Complex::operator/(const Complex& Other) const
			{
				float Sq = Other.SquaredLength();
				if (Sq == 0)
					return Complex(0, 0);

				return Complex((R * Other.R + I * Other.I) / Sq, (I * Other.R - R * Other.I) / Sq);
			}
			Complex Complex::GetRI() const
			{
				return *this;
			}
			Complex Complex::GetIR() const
			{
				return Complex(R, I);
			}
			void Complex::SetRI(const Complex& Other)
			{
				*this = Other;
			}
			void Complex::SetIR(const Complex& Other)
			{
				R = Other.I;
				I = Other.R;
			}
			void Complex::DefaultConstructor(Complex* Base)
			{
				new(Base) Complex();
			}
			void Complex::CopyConstructor(const Complex& Other, Complex* Base)
			{
				new(Base) Complex(Other);
			}
			void Complex::ConvConstructor(float Result, Complex* Base)
			{
				new(Base) Complex(Result);
			}
			void Complex::InitConstructor(float Result, float _I, Complex* Base)
			{
				new(Base) Complex(Result, _I);
			}
			void Complex::ListConstructor(float* InitList, Complex* Base)
			{
				new(Base) Complex(InitList[0], InitList[1]);
			}

			void ConsoleTrace(Core::Console* Base, uint32_t Frames)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				if (Context != nullptr)
					return Base->WriteLine(Context->GetStackTrace(3, (size_t)Frames));

				VI_ERR("[vm] no active context for stack tracing");
			}
			void ConsoleGetSize(Core::Console* Base, uint32_t& X, uint32_t& Y)
			{
				Base->GetSize(&X, &Y);
			}

			Format::Format() noexcept
			{
			}
			Format::Format(unsigned char* Buffer) noexcept
			{
				ImmediateContext* Context = ImmediateContext::Get();
				if (!Context || !Buffer)
					return;

				VirtualMachine* VM = Context->GetVM();
				unsigned int Length = *(unsigned int*)Buffer;
				Buffer += 4;

				while (Length--)
				{
					if (uintptr_t(Buffer) & 0x3)
						Buffer += 4 - (uintptr_t(Buffer) & 0x3);

					int TypeId = *(int*)Buffer;
					Buffer += sizeof(int);

					Core::Stringify Result; Core::String Offset;
					FormatBuffer(VM, Result, Offset, (void*)Buffer, TypeId);
					Args.push_back(Result.R()[0] == '\n' ? Result.Substring(1).R() : Result.R());

					if (TypeId & (int)TypeId::MASK_OBJECT)
					{
						TypeInfo Type = VM->GetTypeInfoById(TypeId);
						if (Type.IsValid() && Type.GetFlags() & (size_t)ObjectBehaviours::VALUE)
							Buffer += Type.GetSize();
						else
							Buffer += sizeof(void*);
					}
					else if (TypeId == 0)
						Buffer += sizeof(void*);
					else
						Buffer += VM->GetSizeOfPrimitiveType(TypeId);
				}
			}
			Core::String Format::JSON(void* Ref, int TypeId)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				if (!Context)
					return "{}";

				Core::Stringify Result;
				FormatJSON(Context->GetVM(), Result, Ref, TypeId);
				return Result.R();
			}
			Core::String Format::Form(const Core::String& F, const Format& Form)
			{
				Core::Stringify Buffer = F;
				size_t Offset = 0;

				for (auto& Item : Form.Args)
				{
					auto R = Buffer.FindUnescaped('?', Offset);
					if (!R.Found)
						break;

					Buffer.ReplacePart(R.Start, R.End, Item);
					Offset = R.End;
				}

				return Buffer.R();
			}
			void Format::WriteLine(Core::Console* Base, const Core::String& F, Format* Form)
			{
				Core::Stringify Buffer = F;
				size_t Offset = 0;

				if (Form != nullptr)
				{
					for (auto& Item : Form->Args)
					{
						auto R = Buffer.FindUnescaped('?', Offset);
						if (!R.Found)
							break;

						Buffer.ReplacePart(R.Start, R.End, Item);
						Offset = R.End;
					}
				}

				Base->sWriteLine(Buffer.R());
			}
			void Format::Write(Core::Console* Base, const Core::String& F, Format* Form)
			{
				Core::Stringify Buffer = F;
				size_t Offset = 0;

				if (Form != nullptr)
				{
					for (auto& Item : Form->Args)
					{
						auto R = Buffer.FindUnescaped('?', Offset);
						if (!R.Found)
							break;

						Buffer.ReplacePart(R.Start, R.End, Item);
						Offset = R.End;
					}
				}

				Base->sWrite(Buffer.R());
			}
			void Format::FormatBuffer(VirtualMachine* VM, Core::Stringify& Result, Core::String& Offset, void* Ref, int TypeId)
			{
				if (TypeId < (int)TypeId::BOOL || TypeId >(int)TypeId::DOUBLE)
				{
					TypeInfo Type = VM->GetTypeInfoById(TypeId);
					if (!Ref || VM->IsNullable(TypeId))
					{
						Result.Append("null");
						return;
					}

					if (TypeInfo::IsScriptObject(TypeId))
					{
						ScriptObject VObject = *(asIScriptObject**)Ref;
						Core::Stringify Decl;

						Offset += '\t';
						for (unsigned int i = 0; i < VObject.GetPropertiesCount(); i++)
						{
							const char* Name = VObject.GetPropertyName(i);
							Decl.Append(Offset).fAppend("%s: ", Name ? Name : "");
							FormatBuffer(VM, Decl, Offset, VObject.GetAddressOfProperty(i), VObject.GetPropertyTypeId(i));
							Decl.Append(",\n");
						}

						Offset = Offset.substr(0, Offset.size() - 1);
						if (!Decl.Empty())
							Result.fAppend("\n%s{\n%s\n%s}", Offset.c_str(), Decl.Clip(Decl.Size() - 2).Get(), Offset.c_str());
						else
							Result.Append("{}");
					}
					else if (strcmp(Type.GetName(), TYPENAME_DICTIONARY) == 0)
					{
						Dictionary* Base = *(Dictionary**)Ref;
						Core::Stringify Decl; Core::String Name;

						Offset += '\t';
						for (unsigned int i = 0; i < Base->GetSize(); i++)
						{
							void* ElementRef; int ElementTypeId;
							if (!Base->GetIndex(i, &Name, &ElementRef, &ElementTypeId))
								continue;

							Decl.Append(Offset).fAppend("%s: ", Name.c_str());
							FormatBuffer(VM, Decl, Offset, ElementRef, ElementTypeId);
							Decl.Append(",\n");
						}

						Offset = Offset.substr(0, Offset.size() - 1);
						if (!Decl.Empty())
							Result.fAppend("\n%s{\n%s\n%s}", Offset.c_str(), Decl.Clip(Decl.Size() - 2).Get(), Offset.c_str());
						else
							Result.Append("{}");
					}
					else if (strcmp(Type.GetName(), TYPENAME_ARRAY) == 0)
					{
						Array* Base = *(Array**)Ref;
						int ArrayTypeId = Base->GetElementTypeId();
						Core::Stringify Decl;

						Offset += '\t';
						for (unsigned int i = 0; i < Base->GetSize(); i++)
						{
							Decl.Append(Offset);
							FormatBuffer(VM, Decl, Offset, Base->At(i), ArrayTypeId);
							Decl.Append(", ");
						}

						Offset = Offset.substr(0, Offset.size() - 1);
						if (!Decl.Empty())
							Result.fAppend("\n%s[\n%s\n%s]", Offset.c_str(), Decl.Clip(Decl.Size() - 2).Get(), Offset.c_str());
						else
							Result.Append("[]");
					}
					else if (strcmp(Type.GetName(), TYPENAME_STRING) != 0)
					{
						Core::Stringify Decl;
						Offset += '\t';

						Type.ForEachProperty([&Decl, VM, &Offset, Ref, TypeId](TypeInfo* Base, FunctionInfo* Prop)
						{
							Decl.Append(Offset).fAppend("%s: ", Prop->Name ? Prop->Name : "");
							FormatBuffer(VM, Decl, Offset, Base->GetProperty<void>(Ref, Prop->Offset, TypeId), Prop->TypeId);
							Decl.Append(",\n");
						});

						Offset = Offset.substr(0, Offset.size() - 1);
						if (!Decl.Empty())
							Result.fAppend("\n%s{\n%s\n%s}", Offset.c_str(), Decl.Clip(Decl.Size() - 2).Get(), Offset.c_str());
						else
							Result.fAppend("{}\n", Type.GetName());
					}
					else
						Result.Append(*(Core::String*)Ref);
				}
				else
				{
					switch (TypeId)
					{
						case (int)TypeId::BOOL:
							Result.fAppend("%s", *(bool*)Ref ? "true" : "false");
							break;
						case (int)TypeId::INT8:
							Result.fAppend("%i", *(char*)Ref);
							break;
						case (int)TypeId::INT16:
							Result.fAppend("%i", *(short*)Ref);
							break;
						case (int)TypeId::INT32:
							Result.fAppend("%i", *(int*)Ref);
							break;
						case (int)TypeId::INT64:
							Result.fAppend("%ll", *(int64_t*)Ref);
							break;
						case (int)TypeId::UINT8:
							Result.fAppend("%u", *(unsigned char*)Ref);
							break;
						case (int)TypeId::UINT16:
							Result.fAppend("%u", *(unsigned short*)Ref);
							break;
						case (int)TypeId::UINT32:
							Result.fAppend("%u", *(unsigned int*)Ref);
							break;
						case (int)TypeId::UINT64:
							Result.fAppend("%" PRIu64, *(uint64_t*)Ref);
							break;
						case (int)TypeId::FLOAT:
							Result.fAppend("%f", *(float*)Ref);
							break;
						case (int)TypeId::DOUBLE:
							Result.fAppend("%f", *(double*)Ref);
							break;
						default:
							Result.Append("null");
							break;
					}
				}
			}
			void Format::FormatJSON(VirtualMachine* VM, Core::Stringify& Result, void* Ref, int TypeId)
			{
				if (TypeId < (int)TypeId::BOOL || TypeId >(int)TypeId::DOUBLE)
				{
					TypeInfo Type = VM->GetTypeInfoById(TypeId);
					void* Object = Type.GetInstance<void>(Ref, TypeId);

					if (!Object)
					{
						Result.Append("null");
						return;
					}

					if (TypeInfo::IsScriptObject(TypeId))
					{
						ScriptObject VObject = (asIScriptObject*)Object;
						Core::Stringify Decl;

						for (unsigned int i = 0; i < VObject.GetPropertiesCount(); i++)
						{
							const char* Name = VObject.GetPropertyName(i);
							Decl.fAppend("\"%s\":", Name ? Name : "");
							FormatJSON(VM, Decl, VObject.GetAddressOfProperty(i), VObject.GetPropertyTypeId(i));
							Decl.Append(",");
						}

						if (!Decl.Empty())
							Result.fAppend("{%s}", Decl.Clip(Decl.Size() - 1).Get());
						else
							Result.Append("{}");
					}
					else if (strcmp(Type.GetName(), TYPENAME_DICTIONARY) == 0)
					{
						Dictionary* Base = (Dictionary*)Object;
						Core::Stringify Decl; Core::String Name;

						for (unsigned int i = 0; i < Base->GetSize(); i++)
						{
							void* ElementRef; int ElementTypeId;
							if (!Base->GetIndex(i, &Name, &ElementRef, &ElementTypeId))
								continue;

							Decl.fAppend("\"%s\":", Name.c_str());
							FormatJSON(VM, Decl, ElementRef, ElementTypeId);
							Decl.Append(",");
						}

						if (!Decl.Empty())
							Result.fAppend("{%s}", Decl.Clip(Decl.Size() - 1).Get());
						else
							Result.Append("{}");
					}
					else if (strcmp(Type.GetName(), TYPENAME_ARRAY) == 0)
					{
						Array* Base = (Array*)Object;
						int ArrayTypeId = Base->GetElementTypeId();
						Core::Stringify Decl;

						for (unsigned int i = 0; i < Base->GetSize(); i++)
						{
							FormatJSON(VM, Decl, Base->At(i), ArrayTypeId);
							Decl.Append(",");
						}

						if (!Decl.Empty())
							Result.fAppend("[%s]", Decl.Clip(Decl.Size() - 1).Get());
						else
							Result.Append("[]");
					}
					else if (strcmp(Type.GetName(), TYPENAME_STRING) != 0)
					{
						Core::Stringify Decl;
						Type.ForEachProperty([&Decl, VM, Ref, TypeId](TypeInfo* Base, FunctionInfo* Prop)
						{
							Decl.fAppend("\"%s\":", Prop->Name ? Prop->Name : "");
							FormatJSON(VM, Decl, Base->GetProperty<void>(Ref, Prop->Offset, TypeId), Prop->TypeId);
							Decl.Append(",");
						});

						if (!Decl.Empty())
							Result.fAppend("{%s}", Decl.Clip(Decl.Size() - 1).Get());
						else
							Result.fAppend("{}", Type.GetName());
					}
					else
						Result.fAppend("\"%s\"", ((Core::String*)Object)->c_str());
				}
				else
				{
					switch (TypeId)
					{
						case (int)TypeId::BOOL:
							Result.fAppend("%s", *(bool*)Ref ? "true" : "false");
							break;
						case (int)TypeId::INT8:
							Result.fAppend("%i", *(char*)Ref);
							break;
						case (int)TypeId::INT16:
							Result.fAppend("%i", *(short*)Ref);
							break;
						case (int)TypeId::INT32:
							Result.fAppend("%i", *(int*)Ref);
							break;
						case (int)TypeId::INT64:
							Result.fAppend("%ll", *(int64_t*)Ref);
							break;
						case (int)TypeId::UINT8:
							Result.fAppend("%u", *(unsigned char*)Ref);
							break;
						case (int)TypeId::UINT16:
							Result.fAppend("%u", *(unsigned short*)Ref);
							break;
						case (int)TypeId::UINT32:
							Result.fAppend("%u", *(unsigned int*)Ref);
							break;
						case (int)TypeId::UINT64:
							Result.fAppend("%" PRIu64, *(uint64_t*)Ref);
							break;
						case (int)TypeId::FLOAT:
							Result.fAppend("%f", *(float*)Ref);
							break;
						case (int)TypeId::DOUBLE:
							Result.fAppend("%f", *(double*)Ref);
							break;
					}
				}
			}

			Application::Application(Desc& I) noexcept : Engine::Application(&I), Context(ImmediateContext::Get())
			{
				VI_ASSERT_V(Context != nullptr, "virtual machine should be present at this level");
				Context->AddRef();

				if (I.Usage & (size_t)Engine::ApplicationSet::ScriptSet)
					VM = Context->GetVM();

				if (I.Usage & (size_t)Engine::ApplicationSet::ContentSet)
				{
					Content = new Engine::ContentManager(nullptr);
					Content->AddProcessor(new Engine::Processors::AssetProcessor(Content), VI_HASH(TYPENAME_ASSETFILE));
					Content->AddProcessor(new Engine::Processors::MaterialProcessor(Content), VI_HASH(TYPENAME_MATERIAL));
					Content->AddProcessor(new Engine::Processors::SceneGraphProcessor(Content), VI_HASH(TYPENAME_SCENEGRAPH));
					Content->AddProcessor(new Engine::Processors::AudioClipProcessor(Content), VI_HASH(TYPENAME_AUDIOCLIP));
					Content->AddProcessor(new Engine::Processors::Texture2DProcessor(Content), VI_HASH(TYPENAME_TEXTURE2D));
					Content->AddProcessor(new Engine::Processors::ShaderProcessor(Content), VI_HASH(TYPENAME_SHADER));
					Content->AddProcessor(new Engine::Processors::ModelProcessor(Content), VI_HASH(TYPENAME_MODEL));
					Content->AddProcessor(new Engine::Processors::SkinModelProcessor(Content), VI_HASH(TYPENAME_SKINMODEL));
					Content->AddProcessor(new Engine::Processors::SkinAnimationProcessor(Content), VI_HASH(TYPENAME_SKINANIMATION));
					Content->AddProcessor(new Engine::Processors::SchemaProcessor(Content), VI_HASH(TYPENAME_SCHEMA));
					Content->AddProcessor(new Engine::Processors::ServerProcessor(Content), VI_HASH(TYPENAME_HTTPSERVER));
					Content->AddProcessor(new Engine::Processors::HullShapeProcessor(Content), VI_HASH(TYPENAME_PHYSICSHULLSHAPE));
				}
			}
			Application::~Application() noexcept
			{
				VM = nullptr;
				VI_CLEAR(OnScriptHook);
				VI_CLEAR(OnKeyEvent);
				VI_CLEAR(OnInputEvent);
				VI_CLEAR(OnWheelEvent);
				VI_CLEAR(OnWindowEvent);
				VI_CLEAR(OnCloseEvent);
				VI_CLEAR(OnComposeEvent);
				VI_CLEAR(OnDispatch);
				VI_CLEAR(OnPublish);
				VI_CLEAR(OnInitialize);
				VI_CLEAR(OnGetGUI);
				VI_CLEAR(Context);
			}
			void Application::ScriptHook()
			{
				if (!OnScriptHook)
					return;

				Context->ExecuteSubcall(OnScriptHook, nullptr);
			}
			void Application::KeyEvent(Graphics::KeyCode Key, Graphics::KeyMod Mod, int Virtual, int Repeat, bool Pressed)
			{
				if (!OnKeyEvent)
					return;

				Context->ExecuteSubcall(OnKeyEvent, [Key, Mod, Virtual, Repeat, Pressed](ImmediateContext* Context)
				{
					Context->SetArg32(0, (int)Key);
					Context->SetArg32(1, (int)Mod);
					Context->SetArg32(2, Virtual);
					Context->SetArg32(3, Repeat);
					Context->SetArg8(4, (unsigned char)Pressed);
				});
			}
			void Application::InputEvent(char* Buffer, size_t Length)
			{
				if (!OnInputEvent)
					return;

				Core::String Text(Buffer, Length);
				Context->ExecuteSubcall(OnInputEvent, [&Text](ImmediateContext* Context)
				{
					Context->SetArgObject(0, (void*)&Text);
				});
			}
			void Application::WheelEvent(int X, int Y, bool Normal)
			{
				if (!OnWheelEvent)
					return;

				Context->ExecuteSubcall(OnWheelEvent, [X, Y, Normal](ImmediateContext* Context)
				{
					Context->SetArg32(0, X);
					Context->SetArg32(1, Y);
					Context->SetArg8(2, (unsigned char)Normal);
				});
			}
			void Application::WindowEvent(Graphics::WindowState NewState, int X, int Y)
			{
				if (!OnWindowEvent)
					return;

				Context->ExecuteSubcall(OnWindowEvent, [NewState, X, Y](ImmediateContext* Context)
				{
					Context->SetArg32(0, (int)NewState);
					Context->SetArg32(1, X);
					Context->SetArg32(2, Y);
				});
			}
			void Application::CloseEvent()
			{
				if (!OnCloseEvent)
					return;

				Context->ExecuteSubcall(OnCloseEvent, nullptr);
			}
			void Application::ComposeEvent()
			{
				if (!OnComposeEvent)
					return;

				Context->ExecuteSubcall(OnComposeEvent, nullptr);
			}
			void Application::Dispatch(Core::Timer* Time)
			{
				if (!OnDispatch)
					return;

				Context->ExecuteSubcall(OnDispatch, [Time](ImmediateContext* Context)
				{
					Context->SetArgObject(0, (void*)Time);
				});
			}
			void Application::Publish(Core::Timer* Time)
			{
				if (!OnPublish)
					return;

				Context->ExecuteSubcall(OnPublish, [Time](ImmediateContext* Context)
				{
					Context->SetArgObject(0, (void*)Time);
				});
			}
			void Application::Initialize()
			{
				if (!OnInitialize)
					return;

				Context->ExecuteSubcall(OnInitialize, nullptr);
			}
			Engine::GUI::Context* Application::GetGUI() const
			{
				if (!OnGetGUI)
					return Engine::Application::GetGUI();

				Context->ExecuteSubcall(OnGetGUI, nullptr);
				return Context->GetReturnObject<Engine::GUI::Context>();
			}
			bool Application::WantsRestart(int ExitCode)
			{
				return ExitCode == Engine::EXIT_RESTART;
			}

			bool StreamOpen(Core::Stream* Base, const Core::String& Path, Core::FileMode Mode)
			{
				return Base->Open(Path.c_str(), Mode);
			}
			Core::String StreamRead(Core::Stream* Base, size_t Size)
			{
				Core::String Result;
				Result.resize(Size);

				size_t Count = Base->Read((char*)Result.data(), Size);
				if (Count < Size)
					Result.resize(Count);

				return Result;
			}
			size_t StreamWrite(Core::Stream* Base, const Core::String& Data)
			{
				return Base->Write(Data.data(), Data.size());
			}

			bool FileStreamOpen(Core::FileStream* Base, const Core::String& Path, Core::FileMode Mode)
			{
				return Base->Open(Path.c_str(), Mode);
			}
			Core::String FileStreamRead(Core::FileStream* Base, size_t Size)
			{
				Core::String Result;
				Result.resize(Size);

				size_t Count = Base->Read((char*)Result.data(), Size);
				if (Count < Size)
					Result.resize(Count);

				return Result;
			}
			size_t FileStreamWrite(Core::FileStream* Base, const Core::String& Data)
			{
				return Base->Write(Data.data(), Data.size());
			}

			bool GzStreamOpen(Core::GzStream* Base, const Core::String& Path, Core::FileMode Mode)
			{
				return Base->Open(Path.c_str(), Mode);
			}
			Core::String GzStreamRead(Core::GzStream* Base, size_t Size)
			{
				Core::String Result;
				Result.resize(Size);

				size_t Count = Base->Read((char*)Result.data(), Size);
				if (Count < Size)
					Result.resize(Count);

				return Result;
			}
			size_t GzStreamWrite(Core::GzStream* Base, const Core::String& Data)
			{
				return Base->Write(Data.data(), Data.size());
			}

			bool WebStreamOpen(Core::WebStream* Base, const Core::String& Path, Core::FileMode Mode)
			{
				return Base->Open(Path.c_str(), Mode);
			}
			Core::String WebStreamRead(Core::WebStream* Base, size_t Size)
			{
				Core::String Result;
				Result.resize(Size);

				size_t Count = Base->Read((char*)Result.data(), Size);
				if (Count < Size)
					Result.resize(Count);

				return Result;
			}
			size_t WebStreamWrite(Core::WebStream* Base, const Core::String& Data)
			{
				return Base->Write(Data.data(), Data.size());
			}

			Core::TaskId ScheduleSetInterval(Core::Schedule* Base, uint64_t Mills, asIScriptFunction* Callback, Core::Difficulty Type, bool AllowMultithreading)
			{
				if (!Callback)
					return Core::INVALID_TASK_ID;

				ImmediateContext* Context = ImmediateContext::Get();
				if (!Context)
					return Core::INVALID_TASK_ID;

				Callback->AddRef();
				Context->AddRef();
				
				Core::TaskId Task = Base->SetSeqInterval(Mills, [AllowMultithreading, Context, Callback](size_t InvocationId) mutable
				{
					if (AllowMultithreading && Context->IsSuspended())
					{
						auto* VM = Context->GetVM();
						Context->Release();
						Context = VM->CreateContext();
					}

					Callback->AddRef();
					Context->AddRef();

					if (InvocationId == 1)
					{
						Callback->Release();
						Context->Release();
					}

					Context->Execute(Callback, nullptr).When([Context, Callback](int&&)
					{
						Callback->Release();
						Context->Release();
					});
				}, Type);

				if (Task != Core::INVALID_TASK_ID)
					return Task;

				Callback->Release();
				Context->Release();
				return Core::INVALID_TASK_ID;
			}
			Core::TaskId ScheduleSetTimeout(Core::Schedule* Base, uint64_t Mills, asIScriptFunction* Callback, Core::Difficulty Type, bool AllowMultithreading)
			{
				if (!Callback)
					return Core::INVALID_TASK_ID;

				ImmediateContext* Context = ImmediateContext::Get();
				if (!Context)
					return Core::INVALID_TASK_ID;

				Callback->AddRef();
				Context->AddRef();

				Core::TaskId Task = Base->SetTimeout(Mills, [AllowMultithreading, Context, Callback]() mutable
				{
					if (AllowMultithreading && Context->IsSuspended())
					{
						auto* VM = Context->GetVM();
						Context->Release();
						Context = VM->CreateContext();
					}

					Context->Execute(Callback, nullptr).When([Context, Callback](int&&)
					{
						Callback->Release();
						Context->Release();
					});
				}, Type);

				if (Task != Core::INVALID_TASK_ID)
					return Task;

				Callback->Release();
				Context->Release();
				return Core::INVALID_TASK_ID;
			}
			bool ScheduleSetImmediate(Core::Schedule* Base, asIScriptFunction* Callback, Core::Difficulty Type, bool AllowMultithreading)
			{
				if (!Callback)
					return false;

				ImmediateContext* Context = ImmediateContext::Get();
				if (!Context)
					return false;

				Callback->AddRef();
				Context->AddRef();

				bool Queued = Base->SetTask([AllowMultithreading, Context, Callback]() mutable
				{
					if (AllowMultithreading && Context->IsSuspended())
					{
						auto* VM = Context->GetVM();
						Context->Release();
						Context = VM->CreateContext();
					}

					Context->Execute(Callback, nullptr).When([Context, Callback](int&&)
					{
						Callback->Release();
						Context->Release();
					});
				}, Type);

				if (Queued)
					return true;

				Callback->Release();
				Context->Release();
				return false;
			}

			Array* OSDirectoryScan(const Core::String& Path)
			{
				Core::Vector<Core::FileEntry> Entries;
				Core::OS::Directory::Scan(Path, &Entries);

				TypeInfo Type = VirtualMachine::Get()->GetTypeInfoByDecl(TYPENAME_ARRAY "<" TYPENAME_FILEENTRY ">@");
				return Array::Compose<Core::FileEntry>(Type.GetTypeInfo(), Entries);
			}
			Array* OSDirectoryGetMounts(const Core::String& Path)
			{
				TypeInfo Type = VirtualMachine::Get()->GetTypeInfoByDecl(TYPENAME_ARRAY "<" TYPENAME_STRING ">@");
				return Array::Compose<Core::String>(Type.GetTypeInfo(), Core::OS::Directory::GetMounts());
			}
			bool OSDirectoryCreate(const Core::String& Path)
			{
				return Core::OS::Directory::Create(Path.c_str());
			}
			bool OSDirectoryRemove(const Core::String& Path)
			{
				return Core::OS::Directory::Remove(Path.c_str());
			}
			bool OSDirectoryIsExists(const Core::String& Path)
			{
				return Core::OS::Directory::IsExists(Path.c_str());
			}
			void OSDirectorySet(const Core::String& Path)
			{
				return Core::OS::Directory::Set(Path.c_str());
			}

			bool OSFileState(const Core::String& Path, Core::FileEntry& Data)
			{
				return Core::OS::File::State(Path.c_str(), &Data);
			}
			bool OSFileMove(const Core::String& From, const Core::String& To)
			{
				return Core::OS::File::Move(From.c_str(), To.c_str());
			}
			bool OSFileRemove(const Core::String& Path)
			{
				return Core::OS::File::Remove(Path.c_str());
			}
			bool OSFileIsExists(const Core::String& Path)
			{
				return Core::OS::File::IsExists(Path.c_str());
			}
			size_t OSFileJoin(const Core::String& From, Array* Paths)
			{
				return Core::OS::File::Join(From.c_str(), Array::Decompose<Core::String>(Paths));
			}
			Core::FileState OSFileGetProperties(const Core::String& Path)
			{
				return Core::OS::File::GetProperties(Path.c_str());
			}
			Core::Stream* OSFileOpenJoin(const Core::String& From, Array* Paths)
			{
				return Core::OS::File::OpenJoin(From.c_str(), Array::Decompose<Core::String>(Paths));
			}
			Array* OSFileReadAsArray(const Core::String& Path)
			{
				TypeInfo Type = VirtualMachine::Get()->GetTypeInfoByDecl(TYPENAME_ARRAY "<" TYPENAME_STRING ">@");
				return Array::Compose<Core::String>(Type.GetTypeInfo(), Core::OS::File::ReadAsArray(Path));
			}

			bool OSPathIsRemote(const Core::String& Path)
			{
				return Core::OS::Path::IsRemote(Path.c_str());
			}
			Core::String OSPathResolve(const Core::String& Path)
			{
				return Core::OS::Path::Resolve(Path.c_str());
			}
			Core::String OSPathResolveDirectory(const Core::String& Path)
			{
				return Core::OS::Path::ResolveDirectory(Path.c_str());
			}
			Core::String OSPathGetDirectory(const Core::String& Path, size_t Level)
			{
				return Core::OS::Path::GetDirectory(Path.c_str(), Level);
			}
			Core::String OSPathGetFilename(const Core::String& Path)
			{
				return Core::OS::Path::GetFilename(Path.c_str());
			}
			Core::String OSPathGetExtension(const Core::String& Path)
			{
				return Core::OS::Path::GetExtension(Path.c_str());
			}

			int OSProcessExecute(const Core::String& Command)
			{
				return Core::OS::Process::Execute("%s", Command.c_str());
			}
			int OSProcessAwait(void* ProcessPtr)
			{
				int ExitCode = -1;
				if (!Core::OS::Process::Await((Core::ChildProcess*)ProcessPtr, &ExitCode))
					return -1;

				return ExitCode;
			}
			bool OSProcessFree(void* ProcessPtr)
			{
				auto* Result = (Core::ChildProcess*)ProcessPtr;
				bool Success = Core::OS::Process::Free(Result);
				VI_DELETE(ChildProcess, Result);
				return Success;
			}
			void* OSProcessSpawn(const Core::String& Path, Array* Args)
			{
				Core::ChildProcess* Result = VI_NEW(Core::ChildProcess);
				if (!Core::OS::Process::Spawn(Path, Array::Decompose<Core::String>(Args), Result))
				{
					VI_DELETE(ChildProcess, Result);
					return nullptr;
				}

				return (void*)Result;
			}
			
			Compute::Vector2& Vector2MulEq1(Compute::Vector2& A, const Compute::Vector2& B)
			{
				return A *= B;
			}
			Compute::Vector2& Vector2MulEq2(Compute::Vector2& A, float B)
			{
				return A *= B;
			}
			Compute::Vector2& Vector2DivEq1(Compute::Vector2& A, const Compute::Vector2& B)
			{
				return A /= B;
			}
			Compute::Vector2& Vector2DivEq2(Compute::Vector2& A, float B)
			{
				return A /= B;
			}
			Compute::Vector2& Vector2AddEq1(Compute::Vector2& A, const Compute::Vector2& B)
			{
				return A += B;
			}
			Compute::Vector2& Vector2AddEq2(Compute::Vector2& A, float B)
			{
				return A += B;
			}
			Compute::Vector2& Vector2SubEq1(Compute::Vector2& A, const Compute::Vector2& B)
			{
				return A -= B;
			}
			Compute::Vector2& Vector2SubEq2(Compute::Vector2& A, float B)
			{
				return A -= B;
			}
			Compute::Vector2 Vector2Mul1(Compute::Vector2& A, const Compute::Vector2& B)
			{
				return A * B;
			}
			Compute::Vector2 Vector2Mul2(Compute::Vector2& A, float B)
			{
				return A * B;
			}
			Compute::Vector2 Vector2Div1(Compute::Vector2& A, const Compute::Vector2& B)
			{
				return A / B;
			}
			Compute::Vector2 Vector2Div2(Compute::Vector2& A, float B)
			{
				return A / B;
			}
			Compute::Vector2 Vector2Add1(Compute::Vector2& A, const Compute::Vector2& B)
			{
				return A + B;
			}
			Compute::Vector2 Vector2Add2(Compute::Vector2& A, float B)
			{
				return A + B;
			}
			Compute::Vector2 Vector2Sub1(Compute::Vector2& A, const Compute::Vector2& B)
			{
				return A - B;
			}
			Compute::Vector2 Vector2Sub2(Compute::Vector2& A, float B)
			{
				return A - B;
			}
			Compute::Vector2 Vector2Neg(Compute::Vector2& A)
			{
				return -A;
			}
			bool Vector2Eq(Compute::Vector2& A, const Compute::Vector2& B)
			{
				return A == B;
			}
			int Vector2Cmp(Compute::Vector2& A, const Compute::Vector2& B)
			{
				if (A == B)
					return 0;

				return A > B ? 1 : -1;
			}

			Compute::Vector3& Vector3MulEq1(Compute::Vector3& A, const Compute::Vector3& B)
			{
				return A *= B;
			}
			Compute::Vector3& Vector3MulEq2(Compute::Vector3& A, float B)
			{
				return A *= B;
			}
			Compute::Vector3& Vector3DivEq1(Compute::Vector3& A, const Compute::Vector3& B)
			{
				return A /= B;
			}
			Compute::Vector3& Vector3DivEq2(Compute::Vector3& A, float B)
			{
				return A /= B;
			}
			Compute::Vector3& Vector3AddEq1(Compute::Vector3& A, const Compute::Vector3& B)
			{
				return A += B;
			}
			Compute::Vector3& Vector3AddEq2(Compute::Vector3& A, float B)
			{
				return A += B;
			}
			Compute::Vector3& Vector3SubEq1(Compute::Vector3& A, const Compute::Vector3& B)
			{
				return A -= B;
			}
			Compute::Vector3& Vector3SubEq2(Compute::Vector3& A, float B)
			{
				return A -= B;
			}
			Compute::Vector3 Vector3Mul1(Compute::Vector3& A, const Compute::Vector3& B)
			{
				return A * B;
			}
			Compute::Vector3 Vector3Mul2(Compute::Vector3& A, float B)
			{
				return A * B;
			}
			Compute::Vector3 Vector3Div1(Compute::Vector3& A, const Compute::Vector3& B)
			{
				return A / B;
			}
			Compute::Vector3 Vector3Div2(Compute::Vector3& A, float B)
			{
				return A / B;
			}
			Compute::Vector3 Vector3Add1(Compute::Vector3& A, const Compute::Vector3& B)
			{
				return A + B;
			}
			Compute::Vector3 Vector3Add2(Compute::Vector3& A, float B)
			{
				return A + B;
			}
			Compute::Vector3 Vector3Sub1(Compute::Vector3& A, const Compute::Vector3& B)
			{
				return A - B;
			}
			Compute::Vector3 Vector3Sub2(Compute::Vector3& A, float B)
			{
				return A - B;
			}
			Compute::Vector3 Vector3Neg(Compute::Vector3& A)
			{
				return -A;
			}
			bool Vector3Eq(Compute::Vector3& A, const Compute::Vector3& B)
			{
				return A == B;
			}
			int Vector3Cmp(Compute::Vector3& A, const Compute::Vector3& B)
			{
				if (A == B)
					return 0;

				return A > B ? 1 : -1;
			}

			Compute::Vector4& Vector4MulEq1(Compute::Vector4& A, const Compute::Vector4& B)
			{
				return A *= B;
			}
			Compute::Vector4& Vector4MulEq2(Compute::Vector4& A, float B)
			{
				return A *= B;
			}
			Compute::Vector4& Vector4DivEq1(Compute::Vector4& A, const Compute::Vector4& B)
			{
				return A /= B;
			}
			Compute::Vector4& Vector4DivEq2(Compute::Vector4& A, float B)
			{
				return A /= B;
			}
			Compute::Vector4& Vector4AddEq1(Compute::Vector4& A, const Compute::Vector4& B)
			{
				return A += B;
			}
			Compute::Vector4& Vector4AddEq2(Compute::Vector4& A, float B)
			{
				return A += B;
			}
			Compute::Vector4& Vector4SubEq1(Compute::Vector4& A, const Compute::Vector4& B)
			{
				return A -= B;
			}
			Compute::Vector4& Vector4SubEq2(Compute::Vector4& A, float B)
			{
				return A -= B;
			}
			Compute::Vector4 Vector4Mul1(Compute::Vector4& A, const Compute::Vector4& B)
			{
				return A * B;
			}
			Compute::Vector4 Vector4Mul2(Compute::Vector4& A, float B)
			{
				return A * B;
			}
			Compute::Vector4 Vector4Div1(Compute::Vector4& A, const Compute::Vector4& B)
			{
				return A / B;
			}
			Compute::Vector4 Vector4Div2(Compute::Vector4& A, float B)
			{
				return A / B;
			}
			Compute::Vector4 Vector4Add1(Compute::Vector4& A, const Compute::Vector4& B)
			{
				return A + B;
			}
			Compute::Vector4 Vector4Add2(Compute::Vector4& A, float B)
			{
				return A + B;
			}
			Compute::Vector4 Vector4Sub1(Compute::Vector4& A, const Compute::Vector4& B)
			{
				return A - B;
			}
			Compute::Vector4 Vector4Sub2(Compute::Vector4& A, float B)
			{
				return A - B;
			}
			Compute::Vector4 Vector4Neg(Compute::Vector4& A)
			{
				return -A;
			}
			bool Vector4Eq(Compute::Vector4& A, const Compute::Vector4& B)
			{
				return A == B;
			}
			int Vector4Cmp(Compute::Vector4& A, const Compute::Vector4& B)
			{
				if (A == B)
					return 0;

				return A > B ? 1 : -1;
			}

			Compute::Matrix4x4 Matrix4x4Mul1(Compute::Matrix4x4& A, const Compute::Matrix4x4& B)
			{
				return A * B;
			}
			Compute::Vector4 Matrix4x4Mul2(Compute::Matrix4x4& A, const Compute::Vector4& B)
			{
				return A * B;
			}
			bool Matrix4x4Eq(Compute::Matrix4x4& A, const Compute::Matrix4x4& B)
			{
				return A == B;
			}
			float& Matrix4x4GetRow(Compute::Matrix4x4& Base, size_t Index)
			{
				return Base.Row[Index % 16];
			}

			Compute::Vector3 QuaternionMul1(Compute::Quaternion& A, const Compute::Vector3& B)
			{
				return A * B;
			}
			Compute::Quaternion QuaternionMul2(Compute::Quaternion& A, const Compute::Quaternion& B)
			{
				return A * B;
			}
			Compute::Quaternion QuaternionMul3(Compute::Quaternion& A, float B)
			{
				return A * B;
			}
			Compute::Quaternion QuaternionAdd(Compute::Quaternion& A, const Compute::Quaternion& B)
			{
				return A + B;
			}
			Compute::Quaternion QuaternionSub(Compute::Quaternion& A, const Compute::Quaternion& B)
			{
				return A - B;
			}

			Compute::Vector4& Frustum8CGetCorners(Compute::Frustum8C& Base, size_t Index)
			{
				return Base.Corners[Index % 8];
			}

			Compute::Vector4& Frustum6PGetCorners(Compute::Frustum6P& Base, size_t Index)
			{
				return Base.Planes[Index % 6];
			}

			size_t JointSize(Compute::Joint& Base)
			{
				return Base.Childs.size();
			}
			Compute::Joint& JointGetChilds(Compute::Joint& Base, size_t Index)
			{
				if (Base.Childs.empty())
					return Base;

				return Base.Childs[Index % Base.Childs.size()];
			}

			size_t SkinAnimatorKeySize(Compute::SkinAnimatorKey& Base)
			{
				return Base.Pose.size();
			}
			Compute::AnimatorKey& SkinAnimatorKeyGetPose(Compute::SkinAnimatorKey& Base, size_t Index)
			{
				if (Base.Pose.empty())
				{
					Base.Pose.push_back(Compute::AnimatorKey());
					return Base.Pose.front();
				}

				return Base.Pose[Index % Base.Pose.size()];
			}

			size_t SkinAnimatorClipSize(Compute::SkinAnimatorClip& Base)
			{
				return Base.Keys.size();
			}
			Compute::SkinAnimatorKey& SkinAnimatorClipGetKeys(Compute::SkinAnimatorClip& Base, size_t Index)
			{
				if (Base.Keys.empty())
				{
					Base.Keys.push_back(Compute::SkinAnimatorKey());
					return Base.Keys.front();
				}

				return Base.Keys[Index % Base.Keys.size()];
			}

			size_t KeyAnimatorClipSize(Compute::KeyAnimatorClip& Base)
			{
				return Base.Keys.size();
			}
			Compute::AnimatorKey& KeyAnimatorClipGetKeys(Compute::KeyAnimatorClip& Base, size_t Index)
			{
				if (Base.Keys.empty())
				{
					Base.Keys.push_back(Compute::AnimatorKey());
					return Base.Keys.front();
				}

				return Base.Keys[Index % Base.Keys.size()];
			}

			Core::String RegexMatchTarget(Compute::RegexMatch& Base)
			{
				return Base.Pointer ? Base.Pointer : Core::String();
			}

			Array* RegexResultGet(Compute::RegexResult& Base)
			{
				TypeInfo Type = VirtualMachine::Get()->GetTypeInfoByDecl(TYPENAME_ARRAY "<" TYPENAME_REGEXMATCH ">@");
				return Array::Compose<Compute::RegexMatch>(Type.GetTypeInfo(), Base.Get());
			}
			Array* RegexResultToArray(Compute::RegexResult& Base)
			{
				TypeInfo Type = VirtualMachine::Get()->GetTypeInfoByDecl(TYPENAME_ARRAY "<" TYPENAME_STRING ">@");
				return Array::Compose<Core::String>(Type.GetTypeInfo(), Base.ToArray());
			}

			Compute::RegexResult RegexSourceMatch(Compute::RegexSource& Base, const Core::String& Text)
			{
				Compute::RegexResult Result;
				Compute::Regex::Match(&Base, Result, Text);
				return Result;
			}
			Core::String RegexSourceReplace(Compute::RegexSource& Base, const Core::String& Text, const Core::String& To)
			{
				Core::String Copy = Text;
				Compute::RegexResult Result;
				Compute::Regex::Replace(&Base, To, Copy);
				return Copy;
			}

			void WebTokenSetAudience(Compute::WebToken* Base, Array* Data)
			{
				Base->SetAudience(Array::Decompose<Core::String>(Data));
			}

			void CosmosQueryIndex(Compute::Cosmos* Base, asIScriptFunction* Overlaps, asIScriptFunction* Match)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				if (!Context || !Overlaps || !Match)
					return;

				Compute::Cosmos::Iterator Iterator;
				Base->QueryIndex<void>(Iterator, [Context, Overlaps](const Compute::Bounding& Box)
				{
					Context->Execute(Overlaps, [&Box](ImmediateContext* Context)
					{
						Context->SetArgObject(0, (void*)&Box);
					}).Wait();

					return (bool)Context->GetReturnWord();
				}, [Context, Match](void* Item)
				{
					Context->ExecuteSubcall(Match, [Item](ImmediateContext* Context)
					{
						Context->SetArgAddress(0, Item);
					});
				});
			}

			void IncludeDescAddExt(Compute::IncludeDesc& Base, const Core::String& Value)
			{
				Base.Exts.push_back(Value);
			}
			void IncludeDescRemoveExt(Compute::IncludeDesc& Base, const Core::String& Value)
			{
				auto It = std::find(Base.Exts.begin(), Base.Exts.end(), Value);
				if (It != Base.Exts.end())
					Base.Exts.erase(It);
			}

			void PreprocessorSetIncludeCallback(Compute::Preprocessor* Base, asIScriptFunction* Callback)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				if (!Context || !Callback)
					return Base->SetIncludeCallback(nullptr);

				Base->SetIncludeCallback([Context, Callback](Compute::Preprocessor* Base, const Compute::IncludeResult& File, Core::String* Out)
				{
					Context->ExecuteSubcall(Callback, [Base, &File, &Out](ImmediateContext* Context)
					{
						Context->SetArgObject(0, Base);
						Context->SetArgObject(1, (void*)&File);
						Context->SetArgObject(2, Out);
					});

					return (bool)Context->GetReturnWord();
				});
			}
			void PreprocessorSetPragmaCallback(Compute::Preprocessor* Base, asIScriptFunction* Callback)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				if (!Context || !Callback)
					return Base->SetPragmaCallback(nullptr);

				TypeInfo Type = VirtualMachine::Get()->GetTypeInfoByDecl(TYPENAME_ARRAY "<" TYPENAME_STRING ">@");
				Base->SetPragmaCallback([Type, Context, Callback](Compute::Preprocessor* Base, const Core::String& Name, const Core::Vector<Core::String>& Args)
				{
					Array* Params = Array::Compose<Core::String>(Type.GetTypeInfo(), Args);
					Context->ExecuteSubcall(Callback, [Base, &Name, &Params](ImmediateContext* Context)
					{
						Context->SetArgObject(0, Base);
						Context->SetArgObject(1, (void*)&Name);
						Context->SetArgObject(2, Params);
					});

					if (Params != nullptr)
						Params->Release();

					return (bool)Context->GetReturnWord();
				});
			}
			bool PreprocessorIsDefined(Compute::Preprocessor* Base, const Core::String& Name)
			{
				return Base->IsDefined(Name.c_str());
			}

			Array* HullShapeGetVertices(Compute::HullShape* Base)
			{
				TypeInfo Type = VirtualMachine::Get()->GetTypeInfoByDecl(TYPENAME_ARRAY "<" TYPENAME_VERTEX ">@");
				return Array::Compose(Type.GetTypeInfo(), Base->GetVertices());
			}
			Array* HullShapeGetIndices(Compute::HullShape* Base)
			{
				TypeInfo Type = VirtualMachine::Get()->GetTypeInfoByDecl(TYPENAME_ARRAY "<int>@");
				return Array::Compose(Type.GetTypeInfo(), Base->GetIndices());
			}

			Compute::SoftBody::Desc::CV::SConvex& SoftBodySConvexCopy(Compute::SoftBody::Desc::CV::SConvex& Base, Compute::SoftBody::Desc::CV::SConvex& Other)
			{
				if (&Base == &Other)
					return Base;

				VI_RELEASE(Base.Hull);
				Base = Other;
				VI_ASSIGN(Base.Hull, Other.Hull);

				return Base;
			}
			void SoftBodySConvexDestructor(Compute::SoftBody::Desc::CV::SConvex& Base)
			{
				VI_RELEASE(Base.Hull);
			}

			Array* SoftBodyGetIndices(Compute::SoftBody* Base)
			{
				Core::Vector<int> Result;
				Base->GetIndices(&Result);

				TypeInfo Type = VirtualMachine::Get()->GetTypeInfoByDecl(TYPENAME_ARRAY "<int>@");
				return Array::Compose(Type.GetTypeInfo(), Result);
			}
			Array* SoftBodyGetVertices(Compute::SoftBody* Base)
			{
				Core::Vector<Compute::Vertex> Result;
				Base->GetVertices(&Result);

				TypeInfo Type = VirtualMachine::Get()->GetTypeInfoByDecl(TYPENAME_ARRAY "<" TYPENAME_VERTEX ">@");
				return Array::Compose(Type.GetTypeInfo(), Result);
			}

			btCollisionShape* SimulatorCreateConvexHullSkinVertex(Compute::Simulator* Base, Array* Data)
			{
				auto Value = Array::Decompose<Compute::SkinVertex>(Data);
				return Base->CreateConvexHull(Value);
			}
			btCollisionShape* SimulatorCreateConvexHullVertex(Compute::Simulator* Base, Array* Data)
			{
				auto Value = Array::Decompose<Compute::Vertex>(Data);
				return Base->CreateConvexHull(Value);
			}
			btCollisionShape* SimulatorCreateConvexHullVector2(Compute::Simulator* Base, Array* Data)
			{
				auto Value = Array::Decompose<Compute::Vector2>(Data);
				return Base->CreateConvexHull(Value);
			}
			btCollisionShape* SimulatorCreateConvexHullVector3(Compute::Simulator* Base, Array* Data)
			{
				auto Value = Array::Decompose<Compute::Vector3>(Data);
				return Base->CreateConvexHull(Value);
			}
			btCollisionShape* SimulatorCreateConvexHullVector4(Compute::Simulator* Base, Array* Data)
			{
				auto Value = Array::Decompose<Compute::Vector4>(Data);
				return Base->CreateConvexHull(Value);
			}
			Array* SimulatorGetShapeVertices(Compute::Simulator* Base, btCollisionShape* Shape)
			{
				TypeInfo Type = VirtualMachine::Get()->GetTypeInfoByDecl(TYPENAME_ARRAY "<" TYPENAME_VECTOR3 ">@");
				return Array::Compose(Type.GetTypeInfo(), Base->GetShapeVertices(Shape));
			}

			bool AudioEffectSetFilter(Audio::AudioEffect* Base, Audio::AudioFilter* New)
			{
				Audio::AudioFilter* Copy = New;
				return Base->SetFilter(&Copy);
			}

			void AlertResult(Graphics::Alert& Base, asIScriptFunction* Callback)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				if (!Context || !Callback)
					return Base.Result(nullptr);

				Base.Result([Context, Callback](int Id)
				{
					Context->ExecuteSubcall(Callback, [Id](ImmediateContext* Context)
					{
						Context->SetArg32(0, Id);
					});
				});
			}

			void ActivitySetTitle(Graphics::Activity* Base, const Core::String& Value)
			{
				Base->SetTitle(Value.c_str());
			}	
			void ActivitySetAppStateChange(Graphics::Activity* Base, asIScriptFunction* Callback)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				if (Context != nullptr && Callback != nullptr)
				{
					Base->Callbacks.AppStateChange = [Context, Callback](Graphics::AppState Type)
					{
						Context->ExecuteSubcall(Callback, [Type](ImmediateContext* Context)
						{
							Context->SetArg32(0, (int)Type);
						});
					};
				}
				else
					Base->Callbacks.AppStateChange = nullptr;
			}
			void ActivitySetWindowStateChange(Graphics::Activity* Base, asIScriptFunction* Callback)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				if (Context != nullptr && Callback != nullptr)
				{
					Base->Callbacks.WindowStateChange = [Context, Callback](Graphics::WindowState State, int X, int Y)
					{
						Context->ExecuteSubcall(Callback, [State, X, Y](ImmediateContext* Context)
						{
							Context->SetArg32(0, (int)State);
							Context->SetArg32(1, X);
							Context->SetArg32(2, Y);
						});
					};
				}
				else
					Base->Callbacks.WindowStateChange = nullptr;
			}
			void ActivitySetKeyState(Graphics::Activity* Base, asIScriptFunction* Callback)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				if (Context != nullptr && Callback != nullptr)
				{
					Base->Callbacks.KeyState = [Context, Callback](Graphics::KeyCode Code, Graphics::KeyMod Mod, int X, int Y, bool Value)
					{
						Context->ExecuteSubcall(Callback, [Code, Mod, X, Y, Value](ImmediateContext* Context)
						{
							Context->SetArg32(0, (int)Code);
							Context->SetArg32(1, (int)Mod);
							Context->SetArg32(2, X);
							Context->SetArg32(3, Y);
							Context->SetArg8(4, Value);
						});
					};
				}
				else
					Base->Callbacks.KeyState = nullptr;
			}
			void ActivitySetInputEdit(Graphics::Activity* Base, asIScriptFunction* Callback)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				if (Context != nullptr && Callback != nullptr)
				{
					Base->Callbacks.InputEdit = [Context, Callback](char* Buffer, int X, int Y)
					{
						Core::String Text = (Buffer ? Buffer : Core::String());
						Context->ExecuteSubcall(Callback, [Text, X, Y](ImmediateContext* Context)
						{
							Context->SetArgObject(0, (void*)&Text);
							Context->SetArg32(1, X);
							Context->SetArg32(2, Y);
						});
					};
				}
				else
					Base->Callbacks.InputEdit = nullptr;
			}
			void ActivitySetInput(Graphics::Activity* Base, asIScriptFunction* Callback)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				if (Context != nullptr && Callback != nullptr)
				{
					Base->Callbacks.Input = [Context, Callback](char* Buffer, int X)
					{
						Core::String Text = (Buffer ? Buffer : Core::String());
						Context->ExecuteSubcall(Callback, [Text, X](ImmediateContext* Context)
						{
							Context->SetArgObject(0, (void*)&Text);
							Context->SetArg32(1, X);
						});
					};
				}
				else
					Base->Callbacks.Input = nullptr;
			}
			void ActivitySetCursorMove(Graphics::Activity* Base, asIScriptFunction* Callback)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				if (Context != nullptr && Callback != nullptr)
				{
					Base->Callbacks.CursorMove = [Context, Callback](int X, int Y, int Z, int W)
					{
						Context->ExecuteSubcall(Callback, [X, Y, Z, W](ImmediateContext* Context)
						{
							Context->SetArg32(0, X);
							Context->SetArg32(1, Y);
							Context->SetArg32(2, Z);
							Context->SetArg32(3, W);
						});
					};
				}
				else
					Base->Callbacks.CursorMove = nullptr;
			}
			void ActivitySetCursorWheelState(Graphics::Activity* Base, asIScriptFunction* Callback)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				if (Context != nullptr && Callback != nullptr)
				{
					Base->Callbacks.CursorWheelState = [Context, Callback](int X, int Y, bool Z)
					{
						Context->ExecuteSubcall(Callback, [X, Y, Z](ImmediateContext* Context)
						{
							Context->SetArg32(0, X);
							Context->SetArg32(1, Y);
							Context->SetArg8(2, Z);
						});
					};
				}
				else
					Base->Callbacks.CursorWheelState = nullptr;
			}
			void ActivitySetJoyStickAxisMove(Graphics::Activity* Base, asIScriptFunction* Callback)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				if (Context != nullptr && Callback != nullptr)
				{
					Base->Callbacks.JoyStickAxisMove = [Context, Callback](int X, int Y, int Z)
					{
						Context->ExecuteSubcall(Callback, [X, Y, Z](ImmediateContext* Context)
						{
							Context->SetArg32(0, X);
							Context->SetArg32(1, Y);
							Context->SetArg32(2, Z);
						});
					};
				}
				else
					Base->Callbacks.JoyStickAxisMove = nullptr;
			}
			void ActivitySetJoyStickBallMove(Graphics::Activity* Base, asIScriptFunction* Callback)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				if (Context != nullptr && Callback != nullptr)
				{
					Base->Callbacks.JoyStickBallMove = [Context, Callback](int X, int Y, int Z, int W)
					{
						Context->ExecuteSubcall(Callback, [X, Y, Z, W](ImmediateContext* Context)
						{
							Context->SetArg32(0, X);
							Context->SetArg32(1, Y);
							Context->SetArg32(2, Z);
							Context->SetArg32(3, W);
						});
					};
				}
				else
					Base->Callbacks.JoyStickBallMove = nullptr;
			}
			void ActivitySetJoyStickHatMove(Graphics::Activity* Base, asIScriptFunction* Callback)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				if (Context != nullptr && Callback != nullptr)
				{
					Base->Callbacks.JoyStickHatMove = [Context, Callback](Graphics::JoyStickHat Type, int X, int Y)
					{
						Context->ExecuteSubcall(Callback, [Type, X, Y](ImmediateContext* Context)
						{
							Context->SetArg32(0, (int)Type);
							Context->SetArg32(1, X);
							Context->SetArg32(2, Y);
						});
					};
				}
				else
					Base->Callbacks.JoyStickHatMove = nullptr;
			}
			void ActivitySetJoyStickKeyState(Graphics::Activity* Base, asIScriptFunction* Callback)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				if (Context != nullptr && Callback != nullptr)
				{
					Base->Callbacks.JoyStickKeyState = [Context, Callback](int X, int Y, bool Z)
					{
						Context->ExecuteSubcall(Callback, [X, Y, Z](ImmediateContext* Context)
						{
							Context->SetArg32(0, X);
							Context->SetArg32(1, Y);
							Context->SetArg8(2, Z);
						});
					};
				}
				else
					Base->Callbacks.JoyStickKeyState = nullptr;
			}
			void ActivitySetJoyStickState(Graphics::Activity* Base, asIScriptFunction* Callback)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				if (Context != nullptr && Callback != nullptr)
				{
					Base->Callbacks.JoyStickState = [Context, Callback](int X, bool Y)
					{
						Context->ExecuteSubcall(Callback, [X, Y](ImmediateContext* Context)
						{
							Context->SetArg32(0, X);
							Context->SetArg8(1, Y);
						});
					};
				}
				else
					Base->Callbacks.JoyStickState = nullptr;
			}
			void ActivitySetControllerAxisMove(Graphics::Activity* Base, asIScriptFunction* Callback)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				if (Context != nullptr && Callback != nullptr)
				{
					Base->Callbacks.ControllerAxisMove = [Context, Callback](int X, int Y, int Z)
					{
						Context->ExecuteSubcall(Callback, [X, Y, Z](ImmediateContext* Context)
						{
							Context->SetArg32(0, X);
							Context->SetArg32(1, Y);
							Context->SetArg32(2, Z);
						});
					};
				}
				else
					Base->Callbacks.ControllerAxisMove = nullptr;
			}
			void ActivitySetControllerKeyState(Graphics::Activity* Base, asIScriptFunction* Callback)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				if (Context != nullptr && Callback != nullptr)
				{
					Base->Callbacks.ControllerKeyState = [Context, Callback](int X, int Y, bool Z)
					{
						Context->ExecuteSubcall(Callback, [X, Y, Z](ImmediateContext* Context)
						{
							Context->SetArg32(0, X);
							Context->SetArg32(1, Y);
							Context->SetArg8(2, Z);
						});
					};
				}
				else
					Base->Callbacks.ControllerKeyState = nullptr;
			}
			void ActivitySetControllerState(Graphics::Activity* Base, asIScriptFunction* Callback)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				if (Context != nullptr && Callback != nullptr)
				{
					Base->Callbacks.ControllerState = [Context, Callback](int X, int Y)
					{
						Context->ExecuteSubcall(Callback, [X, Y](ImmediateContext* Context)
						{
							Context->SetArg32(0, X);
							Context->SetArg32(1, Y);
						});
					};
				}
				else
					Base->Callbacks.ControllerState = nullptr;
			}
			void ActivitySetTouchMove(Graphics::Activity* Base, asIScriptFunction* Callback)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				if (Context != nullptr && Callback != nullptr)
				{
					Base->Callbacks.TouchMove = [Context, Callback](int X, int Y, float Z, float W, float X1, float Y1, float Z1)
					{
						Context->ExecuteSubcall(Callback, [X, Y, Z, W, X1, Y1, Z1](ImmediateContext* Context)
						{
							Context->SetArg32(0, X);
							Context->SetArg32(1, Y);
							Context->SetArgFloat(2, Z);
							Context->SetArgFloat(3, W);
							Context->SetArgFloat(4, X1);
							Context->SetArgFloat(5, Y1);
							Context->SetArgFloat(6, Z1);
						});
					};
				}
				else
					Base->Callbacks.TouchMove = nullptr;
			}
			void ActivitySetTouchState(Graphics::Activity* Base, asIScriptFunction* Callback)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				if (Context != nullptr && Callback != nullptr)
				{
					Base->Callbacks.TouchState = [Context, Callback](int X, int Y, float Z, float W, float X1, float Y1, float Z1, bool W1)
					{
						Context->ExecuteSubcall(Callback, [X, Y, Z, W, X1, Y1, Z1, W1](ImmediateContext* Context)
						{
							Context->SetArg32(0, X);
							Context->SetArg32(1, Y);
							Context->SetArgFloat(2, Z);
							Context->SetArgFloat(3, W);
							Context->SetArgFloat(4, X1);
							Context->SetArgFloat(5, Y1);
							Context->SetArgFloat(6, Z1);
							Context->SetArg8(7, W1);
						});
					};
				}
				else
					Base->Callbacks.TouchState = nullptr;
			}
			void ActivitySetGestureState(Graphics::Activity* Base, asIScriptFunction* Callback)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				if (Context != nullptr && Callback != nullptr)
				{
					Base->Callbacks.GestureState = [Context, Callback](int X, int Y, int Z, float W, float X1, float Y1, bool Z1)
					{
						Context->ExecuteSubcall(Callback, [X, Y, Z, W, X1, Y1, Z1](ImmediateContext* Context)
						{
							Context->SetArg32(0, X);
							Context->SetArg32(1, Y);
							Context->SetArg32(2, Z);
							Context->SetArgFloat(3, W);
							Context->SetArgFloat(4, X1);
							Context->SetArgFloat(5, Y1);
							Context->SetArg8(6, Z1);
						});
					};
				}
				else
					Base->Callbacks.GestureState = nullptr;
			}
			void ActivitySetMultiGestureState(Graphics::Activity* Base, asIScriptFunction* Callback)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				if (Context != nullptr && Callback != nullptr)
				{
					Base->Callbacks.MultiGestureState = [Context, Callback](int X, int Y, float Z, float W, float X1, float Y1)
					{
						Context->ExecuteSubcall(Callback, [X, Y, Z, W, X1, Y1](ImmediateContext* Context)
						{
							Context->SetArg32(0, X);
							Context->SetArg32(1, Y);
							Context->SetArgFloat(2, Z);
							Context->SetArgFloat(3, W);
							Context->SetArgFloat(4, X1);
							Context->SetArgFloat(5, Y1);
						});
					};
				}
				else
					Base->Callbacks.MultiGestureState = nullptr;
			}
			void ActivitySetDropFile(Graphics::Activity* Base, asIScriptFunction* Callback)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				if (Context != nullptr && Callback != nullptr)
				{
					Base->Callbacks.DropFile = [Context, Callback](const Core::String& Text)
					{
						Context->ExecuteSubcall(Callback, [Text](ImmediateContext* Context)
						{
							Context->SetArgObject(0, (void*)&Text);
						});
					};
				}
				else
					Base->Callbacks.DropFile = nullptr;
			}
			void ActivitySetDropText(Graphics::Activity* Base, asIScriptFunction* Callback)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				if (Context != nullptr && Callback != nullptr)
				{
					Base->Callbacks.DropText = [Context, Callback](const Core::String& Text)
					{
						Context->ExecuteSubcall(Callback, [Text](ImmediateContext* Context)
						{
							Context->SetArgObject(0, (void*)&Text);
						});
					};
				}
				else
					Base->Callbacks.DropText = nullptr;
			}
			
			Graphics::RenderTargetBlendState& BlendStateDescGetRenderTarget(Graphics::BlendState::Desc& Base, size_t Index)
			{
				return Base.RenderTarget[Index % 8];
			}

			float& SamplerStateDescGetBorderColor(Graphics::SamplerState::Desc& Base, size_t Index)
			{
				return Base.BorderColor[Index % 4];
			}

			Graphics::InputLayout::Desc& InputLayoutDescCopy(Graphics::InputLayout::Desc& Base, Graphics::InputLayout::Desc& Other)
			{
				if (&Base == &Other)
					return Base;

				VI_RELEASE(Base.Source);
				Base = Other;
				VI_ASSIGN(Base.Source, Other.Source);

				return Base;
			}
			void InputLayoutDescDestructor(Graphics::InputLayout::Desc& Base)
			{
				VI_RELEASE(Base.Source);
			}
			void InputLayoutDescSetAttributes(Graphics::InputLayout::Desc& Base, Array* Data)
			{
				Base.Attributes = Array::Decompose<Graphics::InputLayout::Attribute>(Data);
			}

			Array* InputLayoutGetAttributes(Graphics::InputLayout* Base)
			{
				TypeInfo Type = VirtualMachine::Get()->GetTypeInfoByDecl(TYPENAME_ARRAY "<" TYPENAME_INPUTLAYOUTATTRIBUTE ">@");
				return Array::Compose(Type.GetTypeInfo(), Base->GetAttributes());
			}

			void ShaderDescSetDefines(Graphics::Shader::Desc& Base, Array* Data)
			{
				Base.Defines = Array::Decompose<Core::String>(Data);
			}

			void MeshBufferDescSetElements(Graphics::MeshBuffer::Desc& Base, Array* Data)
			{
				Base.Elements = Array::Decompose<Compute::Vertex>(Data);
			}
			void MeshBufferDescSetIndices(Graphics::MeshBuffer::Desc& Base, Array* Data)
			{
				Base.Indices = Array::Decompose<int>(Data);
			}

			void SkinMeshBufferDescSetElements(Graphics::SkinMeshBuffer::Desc& Base, Array* Data)
			{
				Base.Elements = Array::Decompose<Compute::SkinVertex>(Data);
			}
			void SkinMeshBufferDescSetIndices(Graphics::SkinMeshBuffer::Desc& Base, Array* Data)
			{
				Base.Indices = Array::Decompose<int>(Data);
			}

			Graphics::InstanceBuffer::Desc& InstanceBufferDescCopy(Graphics::InstanceBuffer::Desc& Base, Graphics::InstanceBuffer::Desc& Other)
			{
				if (&Base == &Other)
					return Base;

				VI_RELEASE(Base.Device);
				Base = Other;
				VI_ASSIGN(Base.Device, Other.Device);

				return Base;
			}
			void InstanceBufferDescDestructor(Graphics::InstanceBuffer::Desc& Base)
			{
				VI_RELEASE(Base.Device);
			}

			void InstanceBufferSetArray(Graphics::InstanceBuffer* Base, Array* Data)
			{
				auto& Source = Base->GetArray();
				Source = Array::Decompose<Compute::ElementVertex>(Data);
			}
			Array* InstanceBufferGetArray(Graphics::InstanceBuffer* Base)
			{
				TypeInfo Type = VirtualMachine::Get()->GetTypeInfoByDecl(TYPENAME_ARRAY "<" TYPENAME_ELEMENTVERTEX ">@");
				return Array::Compose(Type.GetTypeInfo(), Base->GetArray());
			}

			void MultiRenderTarget2DDescSetFormatMode(Graphics::MultiRenderTarget2D::Desc& Base, size_t Index, Graphics::Format Mode)
			{
				Base.FormatMode[Index % 8] = Mode;
			}

			void MultiRenderTargetCubeDescSetFormatMode(Graphics::MultiRenderTargetCube::Desc& Base, size_t Index, Graphics::Format Mode)
			{
				Base.FormatMode[Index % 8] = Mode;
			}

			Graphics::Cubemap::Desc& CubemapDescCopy(Graphics::Cubemap::Desc& Base, Graphics::Cubemap::Desc& Other)
			{
				if (&Base == &Other)
					return Base;

				VI_RELEASE(Base.Source);
				Base = Other;
				VI_ASSIGN(Base.Source, Other.Source);

				return Base;
			}
			void CubemapDescDestructor(Graphics::Cubemap::Desc& Base)
			{
				VI_RELEASE(Base.Source);
			}

			Graphics::GraphicsDevice::Desc& GraphicsDeviceDescCopy(Graphics::GraphicsDevice::Desc& Base, Graphics::GraphicsDevice::Desc& Other)
			{
				if (&Base == &Other)
					return Base;

				VI_RELEASE(Base.Window);
				Base = Other;
				VI_ASSIGN(Base.Window, Other.Window);

				return Base;
			}
			void GraphicsDeviceDescDestructor(Graphics::GraphicsDevice::Desc& Base)
			{
				VI_RELEASE(Base.Window);
			}

			void GraphicsDeviceSetVertexBuffers(Graphics::GraphicsDevice* Base, Array* Data, bool Value)
			{
				Core::Vector<Graphics::ElementBuffer*> Buffer = Array::Decompose<Graphics::ElementBuffer*>(Data);
				Base->SetVertexBuffers(Buffer.data(), (uint32_t)Buffer.size(), Value);
			}
			void GraphicsDeviceSetWriteable1(Graphics::GraphicsDevice* Base, Array* Data, uint32_t Slot, bool Value)
			{
				Core::Vector<Graphics::ElementBuffer*> Buffer = Array::Decompose<Graphics::ElementBuffer*>(Data);
				Base->SetWriteable(Buffer.data(), Slot, (uint32_t)Buffer.size(), Value);
			}
			void GraphicsDeviceSetWriteable2(Graphics::GraphicsDevice* Base, Array* Data, uint32_t Slot, bool Value)
			{
				Core::Vector<Graphics::Texture2D*> Buffer = Array::Decompose<Graphics::Texture2D*>(Data);
				Base->SetWriteable(Buffer.data(), Slot, (uint32_t)Buffer.size(), Value);
			}
			void GraphicsDeviceSetWriteable3(Graphics::GraphicsDevice* Base, Array* Data, uint32_t Slot, bool Value)
			{
				Core::Vector<Graphics::Texture3D*> Buffer = Array::Decompose<Graphics::Texture3D*>(Data);
				Base->SetWriteable(Buffer.data(), Slot, (uint32_t)Buffer.size(), Value);
			}
			void GraphicsDeviceSetWriteable4(Graphics::GraphicsDevice* Base, Array* Data, uint32_t Slot, bool Value)
			{
				Core::Vector<Graphics::TextureCube*> Buffer = Array::Decompose<Graphics::TextureCube*>(Data);
				Base->SetWriteable(Buffer.data(), Slot, (uint32_t)Buffer.size(), Value);
			}
			void GraphicsDeviceSetTargetMap(Graphics::GraphicsDevice* Base, Graphics::RenderTarget* Target, Array* Data)
			{
				Core::Vector<bool> Buffer = Array::Decompose<bool>(Data);
				while (Buffer.size() < 8)
					Buffer.push_back(false);

				bool Map[8];
				for (size_t i = 0; i < 8; i++)
					Map[i] = Buffer[i];

				Base->SetTargetMap(Target, Map);
			}
			void GraphicsDeviceSetViewports(Graphics::GraphicsDevice* Base, Array* Data)
			{
				Core::Vector<Graphics::Viewport> Buffer = Array::Decompose<Graphics::Viewport>(Data);
				Base->SetViewports((uint32_t)Buffer.size(), Buffer.data());
			}
			void GraphicsDeviceSetScissorRects(Graphics::GraphicsDevice* Base, Array* Data)
			{
				Core::Vector<Compute::Rectangle> Buffer = Array::Decompose<Compute::Rectangle>(Data);
				Base->SetScissorRects((uint32_t)Buffer.size(), Buffer.data());
			}
			Array* GraphicsDeviceGetViewports(Graphics::GraphicsDevice* Base)
			{
				Core::Vector<Graphics::Viewport> Viewports;
				Viewports.resize(32);

				uint32_t Count = 0;
				Base->GetViewports(&Count, Viewports.data());
				Viewports.resize((size_t)Count);

				TypeInfo Type = VirtualMachine::Get()->GetTypeInfoByDecl(TYPENAME_ARRAY "<" TYPENAME_VIEWPORT ">@");
				return Array::Compose(Type.GetTypeInfo(), Viewports);
			}
			Array* GraphicsDeviceGetScissorRects(Graphics::GraphicsDevice* Base)
			{
				Core::Vector<Compute::Rectangle> Rects;
				Rects.resize(32);

				uint32_t Count = 0;
				Base->GetScissorRects(&Count, Rects.data());
				Rects.resize((size_t)Count);

				TypeInfo Type = VirtualMachine::Get()->GetTypeInfoByDecl(TYPENAME_ARRAY "<" TYPENAME_RECTANGLE ">@");
				return Array::Compose(Type.GetTypeInfo(), Rects);
			}
			Graphics::MeshBuffer* GraphicsDeviceCreateMeshBuffer1(Graphics::GraphicsDevice* Base, const Graphics::MeshBuffer::Desc& Desc)
			{
				auto* Result = Base->CreateMeshBuffer(Desc);
				FunctionFactory::AtomicNotifyGC(TYPENAME_MESHBUFFER, Result);
				return Result;
			}
			Graphics::MeshBuffer* GraphicsDeviceCreateMeshBuffer2(Graphics::GraphicsDevice* Base, Graphics::ElementBuffer* VertexBuffer, Graphics::ElementBuffer* IndexBuffer)
			{
				if (VertexBuffer != nullptr)
					VertexBuffer->AddRef();

				if (IndexBuffer != nullptr)
					IndexBuffer->AddRef();

				auto* Result = Base->CreateMeshBuffer(VertexBuffer, IndexBuffer);
				FunctionFactory::AtomicNotifyGC(TYPENAME_MESHBUFFER, Result);
				return Result;
			}
			Graphics::SkinMeshBuffer* GraphicsDeviceCreateSkinMeshBuffer1(Graphics::GraphicsDevice* Base, const Graphics::SkinMeshBuffer::Desc& Desc)
			{
				auto* Result = Base->CreateSkinMeshBuffer(Desc);
				FunctionFactory::AtomicNotifyGC(TYPENAME_SKINMESHBUFFER, Result);
				return Result;
			}
			Graphics::SkinMeshBuffer* GraphicsDeviceCreateSkinMeshBuffer2(Graphics::GraphicsDevice* Base, Graphics::ElementBuffer* VertexBuffer, Graphics::ElementBuffer* IndexBuffer)
			{
				if (VertexBuffer != nullptr)
					VertexBuffer->AddRef();

				if (IndexBuffer != nullptr)
					IndexBuffer->AddRef();

				auto* Result = Base->CreateSkinMeshBuffer(VertexBuffer, IndexBuffer);
				FunctionFactory::AtomicNotifyGC(TYPENAME_SKINMESHBUFFER, Result);
				return Result;
			}
			Graphics::InstanceBuffer* GraphicsDeviceCreateInstanceBuffer(Graphics::GraphicsDevice* Base, const Graphics::InstanceBuffer::Desc& Desc)
			{
				auto* Result = Base->CreateInstanceBuffer(Desc);
				FunctionFactory::AtomicNotifyGC(TYPENAME_INSTANCEBUFFER, Result);
				return Result;
			}
			Graphics::TextureCube* GraphicsDeviceCreateTextureCube(Graphics::GraphicsDevice* Base, Array* Data)
			{
				Core::Vector<Graphics::Texture2D*> Buffer = Array::Decompose<Graphics::Texture2D*>(Data);
				while (Buffer.size() < 6)
					Buffer.push_back(nullptr);

				Graphics::Texture2D* Map[6];
				for (size_t i = 0; i < 6; i++)
					Map[i] = Buffer[i];

				return Base->CreateTextureCube(Map);
			}
			Graphics::Texture2D* GraphicsDeviceCopyTexture2D1(Graphics::GraphicsDevice* Base, Graphics::Texture2D* Source)
			{
				Graphics::Texture2D* Result = nullptr;
				Base->CopyTexture2D(Source, &Result);
				return Result;
			}
			Graphics::Texture2D* GraphicsDeviceCopyTexture2D2(Graphics::GraphicsDevice* Base, Graphics::RenderTarget* Source, uint32_t Index)
			{
				Graphics::Texture2D* Result = nullptr;
				Base->CopyTexture2D(Source, Index, &Result);
				return Result;
			}
			Graphics::Texture2D* GraphicsDeviceCopyTexture2D3(Graphics::GraphicsDevice* Base, Graphics::RenderTargetCube* Source, Compute::CubeFace Face)
			{
				Graphics::Texture2D* Result = nullptr;
				Base->CopyTexture2D(Source, Face, &Result);
				return Result;
			}
			Graphics::Texture2D* GraphicsDeviceCopyTexture2D4(Graphics::GraphicsDevice* Base, Graphics::MultiRenderTargetCube* Source, uint32_t Index, Compute::CubeFace Face)
			{
				Graphics::Texture2D* Result = nullptr;
				Base->CopyTexture2D(Source, Index, Face, &Result);
				return Result;
			}
			Graphics::TextureCube* GraphicsDeviceCopyTextureCube1(Graphics::GraphicsDevice* Base, Graphics::RenderTargetCube* Source)
			{
				Graphics::TextureCube* Result = nullptr;
				Base->CopyTextureCube(Source, &Result);
				return Result;
			}
			Graphics::TextureCube* GraphicsDeviceCopyTextureCube2(Graphics::GraphicsDevice* Base, Graphics::MultiRenderTargetCube* Source, uint32_t Index)
			{
				Graphics::TextureCube* Result = nullptr;
				Base->CopyTextureCube(Source, Index, &Result);
				return Result;
			}
			Graphics::Texture2D* GraphicsDeviceCopyBackBuffer(Graphics::GraphicsDevice* Base)
			{
				Graphics::Texture2D* Result = nullptr;
				Base->CopyBackBuffer(&Result);
				return Result;
			}
			Graphics::Texture2D* GraphicsDeviceCopyBackBufferMSAA(Graphics::GraphicsDevice* Base)
			{
				Graphics::Texture2D* Result = nullptr;
				Base->CopyBackBufferMSAA(&Result);
				return Result;
			}
			Graphics::Texture2D* GraphicsDeviceCopyBackBufferNoAA(Graphics::GraphicsDevice* Base)
			{
				Graphics::Texture2D* Result = nullptr;
				Base->CopyBackBufferNoAA(&Result);
				return Result;
			}
			Graphics::GraphicsDevice* GraphicsDeviceCreate(Graphics::GraphicsDevice::Desc& Desc)
			{
				auto* Result = Graphics::GraphicsDevice::Create(Desc);
				FunctionFactory::AtomicNotifyGC(TYPENAME_GRAPHICSDEVICE, Result);
				return Result;
			}
			void GraphicsDeviceCompileBuiltinShaders(Array* Devices)
			{
				Graphics::GraphicsDevice::CompileBuiltinShaders(Array::Decompose<Graphics::GraphicsDevice*>(Devices));
			}

			Compute::Matrix4x4& AnimationBufferGetOffsets(Engine::AnimationBuffer& Base, size_t Index)
			{
				return Base.Offsets[Index % Graphics::JOINTS_SIZE];
			}

			void PoseBufferSetOffset(Engine::PoseBuffer& Base, int64_t Index, const Engine::PoseData& Data)
			{
				Base.Offsets[Index] = Data;
			}
			void PoseBufferSetMatrix(Engine::PoseBuffer& Base, Graphics::SkinMeshBuffer* Mesh, size_t Index, const Compute::Matrix4x4& Data)
			{
				Base.Matrices[Mesh].Data[Index % Graphics::JOINTS_SIZE] = Data;
			}
			Engine::PoseData& PoseBufferGetOffset(Engine::PoseBuffer& Base, int64_t Index)
			{
				return Base.Offsets[Index];
			}
			Compute::Matrix4x4& PoseBufferGetMatrix(Engine::PoseBuffer& Base, Graphics::SkinMeshBuffer* Mesh, size_t Index)
			{
				return Base.Matrices[Mesh].Data[Index % Graphics::JOINTS_SIZE];
			}
			size_t PoseBufferGetOffsetsSize(Engine::PoseBuffer& Base)
			{
				return Base.Offsets.size();
			}
			size_t PoseBufferGetMatricesSize(Engine::PoseBuffer& Base, Graphics::SkinMeshBuffer* Mesh)
			{
				return Graphics::JOINTS_SIZE;
			}

			Array* ModelGetMeshes(Engine::Model* Base)
			{
				TypeInfo Type = VirtualMachine::Get()->GetTypeInfoByDecl(TYPENAME_ARRAY "<" TYPENAME_MESHBUFFER "@>@");
				return Array::Compose(Type.GetTypeInfo(), Base->Meshes);
			}
			void ModelSetMeshes(Engine::Model* Base, Array* Data)
			{
				Base->Meshes = Array::Decompose<Graphics::MeshBuffer*>(Data);
			}
			Array* SkinModelGetMeshes(Engine::SkinModel* Base)
			{
				TypeInfo Type = VirtualMachine::Get()->GetTypeInfoByDecl(TYPENAME_ARRAY "<" TYPENAME_SKINMESHBUFFER "@>@");
				return Array::Compose(Type.GetTypeInfo(), Base->Meshes);
			}
			void SkinModelSetMeshes(Engine::SkinModel* Base, Array* Data)
			{
				Base->Meshes = Array::Decompose<Graphics::SkinMeshBuffer*>(Data);
			}

			Dictionary* LocationGetQuery(Network::Location& Base)
			{
				TypeInfo Type = VirtualMachine::Get()->GetTypeInfoByDecl(TYPENAME_DICTIONARY "<" TYPENAME_STRING ">@");
				return Dictionary::Compose<Core::String>(Type.GetTypeId(), Base.Query);
			}

			int SocketAccept1(Network::Socket* Base, Network::Socket* Fd, Core::String& Address)
			{
				char IpAddress[64];
				int Status = Base->Accept(Fd, IpAddress);
				Address = IpAddress;
				return Status;
			}
			int SocketAccept2(Network::Socket* Base, socket_t& Fd, Core::String& Address)
			{
				char IpAddress[64];
				int Status = Base->Accept(&Fd, IpAddress);
				Address = IpAddress;
				return Status;
			}
			int SocketAcceptAsync(Network::Socket* Base, bool WithAddress, asIScriptFunction* Callback)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				if (!Context || !Callback)
					return -1;

				return Base->AcceptAsync(WithAddress, [Context, Callback](socket_t Fd, char* Address)
				{
					Core::String IpAddress = Address;
					Context->Execute(Callback, [Fd, IpAddress](ImmediateContext* Context)
					{
#ifdef VI_64
						Context->SetArg64(0, (int64_t)Fd);
#else
						Context->SetArg32(0, (int32_t)Fd);
#endif
						Context->SetArgObject(1, (void*)&IpAddress);
					}).Wait();

					return (bool)Context->GetReturnWord();
				});
			}
			int SocketCloseAsync(Network::Socket* Base, bool Graceful, asIScriptFunction* Callback)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				if (!Context || !Callback)
					return -1;

				return Base->CloseAsync(Graceful, [Context, Callback]()
				{
					Context->Execute(Callback, nullptr).Wait();
					return (bool)Context->GetReturnWord();
				});
			}
			int64_t SocketSendFileAsync(Network::Socket* Base, FILE* Stream, int64_t Offset, int64_t Size, asIScriptFunction* Callback)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				if (!Context || !Callback)
					return -1;

				return Base->SendFileAsync(Stream, Offset, Size, [Context, Callback](Network::SocketPoll Poll)
				{
					Context->Execute(Callback, [Poll](ImmediateContext* Context)
					{
						Context->SetArg32(0, (int)Poll);
					}).Wait();
				});
			}
			int SocketWrite(Network::Socket* Base, const Core::String& Data)
			{
				return Base->Write(Data.data(), (int)Data.size());
			}
			int SocketWriteAsync(Network::Socket* Base, const Core::String& Data, asIScriptFunction* Callback)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				if (!Context || !Callback)
					return -1;

				return Base->WriteAsync(Data.data(), Data.size(), [Context, Callback](Network::SocketPoll Poll)
				{
					Context->Execute(Callback, [Poll](ImmediateContext* Context)
					{
						Context->SetArg32(0, (int)Poll);
					}).Wait();
				});
			}
			int SocketRead1(Network::Socket* Base, Core::String& Data, int Size)
			{
				Data.resize(Size);
				return Base->Read((char*)Data.data(), Size);
			}
			int SocketRead2(Network::Socket* Base, Core::String& Data, int Size, asIScriptFunction* Callback)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				if (!Context || !Callback)
					return -1;

				return Base->Read((char*)Data.data(), (int)Data.size(), [Context, Callback](Network::SocketPoll Poll, const char* Data, size_t Size)
				{
					Core::String Source(Data, Size);
					Context->ExecuteSubcall(Callback, [Poll, Source](ImmediateContext* Context)
					{
						Context->SetArg32(0, (int)Poll);
						Context->SetArgObject(1, (void*)&Source);
					});

					return (bool)Context->GetReturnWord();
				});
			}
			int SocketReadAsync(Network::Socket* Base, size_t Size, asIScriptFunction* Callback)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				if (!Context || !Callback)
					return -1;

				return Base->ReadAsync(Size, [Context, Callback](Network::SocketPoll Poll, const char* Data, size_t Size)
				{
					Core::String Source(Data, Size);
					Context->Execute(Callback, [Poll, Source](ImmediateContext* Context)
					{
						Context->SetArg32(0, (int)Poll);
						Context->SetArgObject(1, (void*)&Source);
					}).Wait();

					return (bool)Context->GetReturnWord();
				});
			}
			int SocketReadUntil(Network::Socket* Base, Core::String& Data, asIScriptFunction* Callback)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				if (!Context || !Callback)
					return -1;

				return Base->ReadUntil(Data.c_str(), [Context, Callback](Network::SocketPoll Poll, const char* Data, size_t Size)
				{
					Core::String Source(Data, Size);
					Context->ExecuteSubcall(Callback, [Poll, Source](ImmediateContext* Context)
					{
						Context->SetArg32(0, (int)Poll);
						Context->SetArgObject(1, (void*)&Source);
					});

					return (bool)Context->GetReturnWord();
				});
			}
			int SocketReadUntilAsync(Network::Socket* Base, Core::String& Data, asIScriptFunction* Callback)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				if (!Context || !Callback)
					return -1;

				return Base->ReadUntilAsync(Data.c_str(), [Context, Callback](Network::SocketPoll Poll, const char* Data, size_t Size)
				{
					Core::String Source(Data, Size);
					Context->Execute(Callback, [Poll, Source](ImmediateContext* Context)
					{
						Context->SetArg32(0, (int)Poll);
						Context->SetArgObject(1, (void*)&Source);
					}).Wait();

					return (bool)Context->GetReturnWord();
				});
			}
			int SocketConnectAsync(Network::Socket* Base, Network::SocketAddress* Address, asIScriptFunction* Callback)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				if (!Context || !Callback)
					return -1;

				return Base->ConnectAsync(Address, [Context, Callback](int Code)
				{
					Context->Execute(Callback, [Code](ImmediateContext* Context)
					{
						Context->SetArg32(0, (int)Code);
					}).Wait();

					return (bool)Context->GetReturnWord();
				});
			}
			int SocketSecure(Network::Socket* Base, ssl_ctx_st* Context, const Core::String& Value)
			{
				return Base->Secure(Context, Value.c_str());
			}

			Network::EpollFd& EpollFdCopy(Network::EpollFd& Base, Network::EpollFd& Other)
			{
				if (&Base == &Other)
					return Base;

				VI_RELEASE(Base.Base);
				Base = Other;
				VI_ASSIGN(Base.Base, Other.Base);

				return Base;
			}
			void EpollFdDestructor(Network::EpollFd& Base)
			{
				VI_RELEASE(Base.Base);
			}

			int EpollHandleWait(Network::EpollHandle& Base, Array* Data, uint64_t Timeout)
			{
				Core::Vector<Network::EpollFd> Fds = Array::Decompose<Network::EpollFd>(Data);
				return Base.Wait(Fds.data(), Fds.size(), Timeout);
			}

			bool MultiplexerWhenReadable(Network::Socket* Base, asIScriptFunction* Callback)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				if (!Context || !Callback)
					return false;

				return Network::Multiplexer::WhenReadable(Base, [Base, Context, Callback](Network::SocketPoll Poll)
				{
					Context->Execute(Callback, [Base, Poll](ImmediateContext* Context)
					{
						Context->SetArgObject(0, Base);
						Context->SetArg32(1, (int)Poll);
					}).Wait();
				});
			}
			bool MultiplexerWhenWriteable(Network::Socket* Base, asIScriptFunction* Callback)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				if (!Context || !Callback)
					return false;

				return Network::Multiplexer::WhenWriteable(Base, [Base, Context, Callback](Network::SocketPoll Poll)
				{
					Context->Execute(Callback, [Base, Poll](ImmediateContext* Context)
					{
						Context->SetArgObject(0, Base);
						Context->SetArg32(1, (int)Poll);
					}).Wait();
				});
			}
			
			void SocketRouterSetListeners(Network::SocketRouter* Base, Dictionary* Data)
			{
				TypeInfo Type = VirtualMachine::Get()->GetTypeInfoByDecl(TYPENAME_DICTIONARY "<" TYPENAME_REMOTEHOST ">@");
				Base->Listeners = Dictionary::Decompose<Network::RemoteHost>(Type.GetTypeId(), Data);
			}
			Dictionary* SocketRouterGetListeners(Network::SocketRouter* Base)
			{
				TypeInfo Type = VirtualMachine::Get()->GetTypeInfoByDecl(TYPENAME_DICTIONARY "<" TYPENAME_REMOTEHOST ">@");
				return Dictionary::Compose<Network::RemoteHost>(Type.GetTypeId(), Base->Listeners);
			}
			void SocketRouterSetCertificates(Network::SocketRouter* Base, Dictionary* Data)
			{
				TypeInfo Type = VirtualMachine::Get()->GetTypeInfoByDecl(TYPENAME_DICTIONARY "<" TYPENAME_SOCKETCERTIFICATE ">@");
				Base->Certificates = Dictionary::Decompose<Network::SocketCertificate>(Type.GetTypeId(), Data);
			}
			Dictionary* SocketRouterGetCertificates(Network::SocketRouter* Base)
			{
				TypeInfo Type = VirtualMachine::Get()->GetTypeInfoByDecl(TYPENAME_DICTIONARY "<" TYPENAME_SOCKETCERTIFICATE ">@");
				return Dictionary::Compose<Network::SocketCertificate>(Type.GetTypeId(), Base->Certificates);
			}

			Core::String SocketConnectionGetRemoteAddress(Network::SocketConnection* Base)
			{
				return Base->RemoteAddress;
			}
			bool SocketConnectionError(Network::SocketConnection* Base, int Code, const Core::String& Message)
			{
				return Base->Error(Code, "%s", Message.c_str());
			}

			template <typename T>
			void PopulateAudioFilterBase(RefClass& Class, bool BaseCast = true)
			{
				if (BaseCast)
					Class.SetOperatorEx(Operators::Cast, 0, "void", "?&out", &HandleToHandleCast);

				Class.SetMethod("void synchronize()", &T::Synchronize);
				Class.SetMethod("void deserialize(schema@+)", &T::Deserialize);
				Class.SetMethod("void serialize(schema@+)", &T::Serialize);
				Class.SetMethod("base_audio_filter@ copy()", &T::Copy);
				Class.SetMethod("audio_source@+ get_source()", &T::GetSource);
				PopulateComponent<T>(Class);
			}
			template <typename T, typename... Args>
			void PopulateAudioFilterInterface(RefClass& Class, const char* Constructor)
			{
				Class.SetConstructor<T, Args...>(Constructor);
				Class.SetDynamicCast<T, Audio::AudioFilter>("base_audio_filter@+", true);
				PopulateAudioFilterBase<T>(Class, false);
			}

			template <typename T>
			void PopulateAudioEffectBase(RefClass& Class, bool BaseCast = true)
			{
				if (BaseCast)
					Class.SetOperatorEx(Operators::Cast, 0, "void", "?&out", &HandleToHandleCast);

				Class.SetMethod("void synchronize()", &T::Synchronize);
				Class.SetMethod("void deserialize(schema@+)", &T::Deserialize);
				Class.SetMethod("void serialize(schema@+)", &T::Serialize);
				Class.SetMethodEx("bool set_filter(base_audio_filter@+)", &AudioEffectSetFilter);
				Class.SetMethod("base_audio_effect@ copy()", &T::Copy);
				Class.SetMethod("audio_source@+ get_filter()", &T::GetFilter);
				Class.SetMethod("audio_source@+ get_source()", &T::GetSource);
				PopulateComponent<T>(Class);
			}
			template <typename T, typename... Args>
			void PopulateAudioEffectInterface(RefClass& Class, const char* Constructor)
			{
				Class.SetConstructor<T, Args...>(Constructor);
				Class.SetDynamicCast<T, Audio::AudioEffect>("base_audio_effect@+", true);
				PopulateAudioEffectBase<T>(Class, false);
			}

			void EventSetArgs(Engine::Event& Base, Dictionary* Data)
			{
				TypeInfo Type = VirtualMachine::Get()->GetTypeInfoByDecl(TYPENAME_DICTIONARY "<" TYPENAME_VARIANT ">@");
				Base.Args = Dictionary::Decompose<Core::Variant>(Type.GetTypeId(), Data);
			}
			Dictionary* EventGetArgs(Engine::Event& Base)
			{
				TypeInfo Type = VirtualMachine::Get()->GetTypeInfoByDecl(TYPENAME_DICTIONARY "<" TYPENAME_VARIANT ">@");
				return Dictionary::Compose<Core::Variant>(Type.GetTypeId(), Base.Args);
			}

			Engine::Viewer& ViewerCopy(Engine::Viewer& Base, Engine::Viewer& Other)
			{
				if (&Base == &Other)
					return Base;

				VI_RELEASE(Base.Renderer);
				Base = Other;
				VI_ASSIGN(Base.Renderer, Other.Renderer);

				return Base;
			}
			void ViewerDestructor(Engine::Viewer& Base)
			{
				VI_RELEASE(Base.Renderer);
			}

			Array* SkinAnimationGetClips(Engine::SkinAnimation* Base)
			{
				TypeInfo Type = VirtualMachine::Get()->GetTypeInfoByDecl(TYPENAME_ARRAY "<" TYPENAME_SKINANIMATORCLIP ">@");
				return Array::Compose<Compute::SkinAnimatorClip>(Type.GetTypeInfo(), Base->GetClips());
			}

			size_t SparseIndexGetSize(Engine::SparseIndex& Base)
			{
				return Base.Data.Size();
			}
			Engine::Component* SparseIndexGetData(Engine::SparseIndex& Base, size_t Index)
			{
				if (Index >= Base.Data.Size())
					return nullptr;

				return Base.Data[Index];
			}

			template <typename T>
			void* ProcessorDuplicate(T* Base, Engine::AssetCache* Cache, Core::Schema* Args)
			{
				return Base->Duplicate(Cache, ToVariantKeys(Args));
			}
			template <typename T>
			void* ProcessorDeserialize(T* Base, Core::Stream* Stream, size_t Offset, Core::Schema* Args)
			{
				return Base->Deserialize(Stream, Offset, ToVariantKeys(Args));
			}
			template <typename T>
			bool ProcessorSerialize(T* Base, Core::Stream* Stream, void* Instance, Core::Schema* Args)
			{
				return Base->Serialize(Stream, Instance, ToVariantKeys(Args));
			}
			template <typename T>
			void PopulateProcessorBase(RefClass& Class, bool BaseCast = true)
			{
				if (BaseCast)
					Class.SetOperatorEx(Operators::Cast, 0, "void", "?&out", &HandleToHandleCast);

				Class.SetMethod("void free(uptr@)", &Engine::Processor::Free);
				Class.SetMethodEx("uptr@ duplicate(uptr@, schema@+)", &ProcessorDuplicate<T>);
				Class.SetMethodEx("uptr@ deserialize(base_stream@+, usize, schema@+)", &ProcessorDeserialize<T>);
				Class.SetMethodEx("bool serialize(base_stream@+, uptr@, schema@+)", &ProcessorSerialize<T>);
				Class.SetMethod("content_manager@+ get_content() const", &Engine::Processor::GetContent);
			}
			template <typename T, typename... Args>
			void PopulateProcessorInterface(RefClass& Class, const char* Constructor)
			{
				Class.SetConstructor<T, Args...>(Constructor);
				Class.SetDynamicCast<T, Engine::Processor>("base_processor@+", true);
				PopulateProcessorBase<T>(Class, false);
			}

			template <typename T>
			void ComponentMessage(T* Base, const Core::String& Name, Core::Schema* Args)
			{
				auto Data = ToVariantKeys(Args);
				return Base->Message(Name, Data);
			}
			template <typename T>
			void PopulateComponentBase(RefClass& Class, bool BaseCast = true)
			{
				if (BaseCast)
					Class.SetOperatorEx(Operators::Cast, 0, "void", "?&out", &HandleToHandleCast);

				Class.SetMethod("void serialize(schema@+)", &Engine::Component::Serialize);
				Class.SetMethod("void deserialize(schema@+)", &Engine::Component::Deserialize);
				Class.SetMethod("void activate(base_component@+)", &Engine::Component::Activate);
				Class.SetMethod("void deactivate()", &Engine::Component::Deactivate);
				Class.SetMethod("void synchronize(clock_timer@+)", &Engine::Component::Synchronize);
				Class.SetMethod("void animate(clock_timer@+)", &Engine::Component::Animate);
				Class.SetMethod("void update(clock_timer@+)", &Engine::Component::Update);
				Class.SetMethodEx("void message(const string &in, schema@+)", &ComponentMessage<T>);
				Class.SetMethod("void movement()", &Engine::Component::Movement);
				Class.SetMethod("usize get_unit_bounds(vector3 &out, vector3 &out) const", &Engine::Component::GetUnitBounds);
				Class.SetMethod("float get_visibility(const viewer_t &in, float) const", &Engine::Component::GetVisibility);
				Class.SetMethod("base_component@+ copy(scene_entity@+) const", &Engine::Component::Copy);
				Class.SetMethod("scene_entity@+ get_entity() const", &Engine::Component::GetEntity);
				Class.SetMethod("void set_active(bool)", &Engine::Component::SetActive);
				Class.SetMethod("bool is_drawable() const", &Engine::Component::IsDrawable);
				Class.SetMethod("bool is_cullable() const", &Engine::Component::IsCullable);
				Class.SetMethod("bool is_active() const", &Engine::Component::IsActive);
				PopulateComponent<T>(Class);
			};
			template <typename T, typename... Args>
			void PopulateComponentInterface(RefClass& Class, const char* Constructor)
			{
				Class.SetConstructor<T, Args...>(Constructor);
				Class.SetDynamicCast<T, Engine::Component>("base_component@+", true);
				PopulateComponentBase<T>(Class, false);
			};

			void EntityRemoveComponent(Engine::Entity* Base, Engine::Component* Source)
			{
				if (Source != nullptr)
					Base->RemoveComponent(Source->GetId());
			}
			void EntityRemoveComponentById(Engine::Entity* Base, uint64_t Id)
			{
				Base->RemoveComponent(Id);
			}
			Engine::Component* EntityAddComponent(Engine::Entity* Base, Engine::Component* Source)
			{
				if (!Source)
					return nullptr;

				return Base->AddComponent(Source);
			}
			Engine::Component* EntityGetComponentById(Engine::Entity* Base, uint64_t Id)
			{
				return Base->GetComponent(Id);
			}
			Array* EntityGetComponents(Engine::Entity* Base)
			{
				Core::Vector<Engine::Component*> Components;
				Components.reserve(Base->GetComponentsCount());

				for (auto& Item : *Base)
					Components.push_back(Item.second);

				TypeInfo Type = VirtualMachine::Get()->GetTypeInfoByDecl(TYPENAME_ARRAY "<" TYPENAME_COMPONENT "@>@");
				return Array::Compose<Engine::Component*>(Type.GetTypeInfo(), Components);
			}

			template <typename T>
			void PopulateDrawableBase(RefClass & Class)
			{
				Class.SetProperty<Engine::Drawable>("float overlapping", &Engine::Drawable::Overlapping);
				Class.SetProperty<Engine::Drawable>("bool static", &Engine::Drawable::Static);
				Class.SetMethod("void clear_materials()", &Engine::Drawable::ClearMaterials);
				Class.SetMethod("bool set_category(geo_category)", &Engine::Drawable::SetCategory);
				Class.SetMethod<Engine::Drawable, bool, void*, Engine::Material*>("bool set_material(uptr@, material@+)", &Engine::Drawable::SetMaterial);
				Class.SetMethod<Engine::Drawable, bool, Engine::Material*>("bool set_material(material@+)", &Engine::Drawable::SetMaterial);
				Class.SetMethod("geo_category get_category() const", &Engine::Drawable::GetCategory);
				Class.SetMethod<Engine::Drawable, int64_t, void*>("int64 get_slot(uptr@)", &Engine::Drawable::GetSlot);
				Class.SetMethod<Engine::Drawable, int64_t>("int64 get_slot()", &Engine::Drawable::GetSlot);
				Class.SetMethod<Engine::Drawable, Engine::Material*, void*>("material@+ get_material(uptr@)", &Engine::Drawable::GetMaterial);
				Class.SetMethod<Engine::Drawable, Engine::Material*>("material@+ get_material()", &Engine::Drawable::GetMaterial);
				PopulateComponentBase<T>(Class, false);
			}
			template <typename T, typename... Args>
			void PopulateDrawableInterface(RefClass& Class, const char* Constructor)
			{
				Class.SetConstructor<T, Args...>(Constructor);
				Class.SetDynamicCast<T, Engine::Component>("base_component@+", true);
				PopulateDrawableBase<T>(Class);
			}

			template <typename T>
			void PopulateRendererBase(RefClass& Class, bool BaseCast = true)
			{
				if (BaseCast)
					Class.SetOperatorEx(Operators::Cast, 0, "void", "?&out", &HandleToHandleCast);

				Class.SetProperty<Engine::Renderer>("bool active", &Engine::Renderer::Active);
				Class.SetMethod("void serialize(schema@+)", &Engine::Renderer::Serialize);
				Class.SetMethod("void deserialize(schema@+)", &Engine::Renderer::Deserialize);
				Class.SetMethod("void clear_culling()", &Engine::Renderer::ClearCulling);
				Class.SetMethod("void resize_buffers()", &Engine::Renderer::ResizeBuffers);
				Class.SetMethod("void activate()", &Engine::Renderer::Activate);
				Class.SetMethod("void deactivate()", &Engine::Renderer::Deactivate);
				Class.SetMethod("void begin_pass()", &Engine::Renderer::BeginPass);
				Class.SetMethod("void end_pass()", &Engine::Renderer::EndPass);
				Class.SetMethod("bool has_category(geo_category)", &Engine::Renderer::HasCategory);
				Class.SetMethod("usize render_pass(clock_timer@+)", &Engine::Renderer::RenderPass);
				Class.SetMethod("void set_renderer(render_system@+)", &Engine::Renderer::SetRenderer);
				Class.SetMethod("render_system@+ get_renderer() const", &Engine::Renderer::GetRenderer);
				PopulateComponent<T>(Class);
			}
			template <typename T, typename... Args>
			void PopulateRendererInterface(RefClass& Class, const char* Constructor)
			{
				Class.SetConstructor<T, Args...>(Constructor);
				Class.SetDynamicCast<T, Engine::Renderer>("base_renderer@+", true);
				PopulateRendererBase<T>(Class, false);
			}

			void RenderSystemRestoreViewBuffer(Engine::RenderSystem* Base)
			{
				Base->RestoreViewBuffer(nullptr);
			}
			void RenderSystemMoveRenderer(Engine::RenderSystem* Base, Engine::Renderer* Source, size_t Offset)
			{
				if (Source != nullptr)
					Base->MoveRenderer(Source->GetId(), Offset);
			}
			void RenderSystemRemoveRenderer(Engine::RenderSystem* Base, Engine::Renderer* Source)
			{
				if (Source != nullptr)
					Base->RemoveRenderer(Source->GetId());
			}
			void RenderSystemFreeBuffers1(Engine::RenderSystem* Base, const Core::String& Name, Graphics::ElementBuffer* Buffer1, Graphics::ElementBuffer* Buffer2)
			{
				Graphics::ElementBuffer* Buffers[2];
				Buffers[0] = Buffer1;
				Buffers[1] = Buffer2;
				Base->FreeBuffers(Name, Buffers);
			}
			void RenderSystemFreeBuffers2(Engine::RenderSystem* Base, const Core::String& Name, Graphics::ElementBuffer* Buffer1, Graphics::ElementBuffer* Buffer2)
			{
				Graphics::ElementBuffer* Buffers[2];
				Buffers[0] = Buffer1;
				Buffers[1] = Buffer2;
				Base->FreeBuffers(Buffers);
			}
			Array* RenderSystemCompileBuffers(Engine::RenderSystem* Base, const Core::String& Name, size_t ElementSize, size_t ElementsCount)
			{
				Core::Vector<Graphics::ElementBuffer*> Buffers;
				Buffers.push_back(nullptr);
				Buffers.push_back(nullptr);

				if (!Base->CompileBuffers(Buffers.data(), Name, ElementSize, ElementsCount))
					return nullptr;

				TypeInfo Type = VirtualMachine::Get()->GetTypeInfoByDecl(TYPENAME_ARRAY "<" TYPENAME_ELEMENTBUFFER "@>@");
				return Array::Compose(Type.GetTypeInfo(), Buffers);
			}
			bool RenderSystemAddRenderer(Engine::RenderSystem* Base, Engine::Renderer* Source)
			{
				if (!Source)
					return false;

				return Base->AddRenderer(Source) != nullptr;
			}
			Engine::Renderer* RenderSystemGetRenderer(Engine::RenderSystem* Base, uint64_t Id)
			{
				return Base->GetRenderer(Id);
			}
			Engine::Renderer* RenderSystemGetRendererByIndex(Engine::RenderSystem* Base, size_t Index)
			{
				auto& Data = Base->GetRenderers();
				if (Index >= Data.size())
					return nullptr;

				return Data[Index];
			}
			size_t RenderSystemGetRenderersCount(Engine::RenderSystem* Base)
			{
				return Base->GetRenderers().size();
			}
			void RenderSystemQuerySync(Engine::RenderSystem* Base, uint64_t Id, asIScriptFunction* Callback)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				if (!Callback || !Context)
					return;

				Base->QueryBasicSync(Id, [Context, Callback](Engine::Component* Item)
				{
					Context->ExecuteSubcall(Callback, [Item](ImmediateContext* Context)
					{
						Context->SetArgObject(0, (void*)Item);
					});
				});
			}

			Array* PrimitiveCacheCompile(Engine::PrimitiveCache* Base, const Core::String& Name, size_t ElementSize, size_t ElementsCount)
			{
				Core::Vector<Graphics::ElementBuffer*> Buffers;
				Buffers.push_back(nullptr);
				Buffers.push_back(nullptr);

				if (!Base->Compile(Buffers.data(), Name, ElementSize, ElementsCount))
					return nullptr;

				TypeInfo Type = VirtualMachine::Get()->GetTypeInfoByDecl(TYPENAME_ARRAY "<" TYPENAME_ELEMENTBUFFER "@>@");
				return Array::Compose(Type.GetTypeInfo(), Buffers);
			}
			Array* PrimitiveCacheGet(Engine::PrimitiveCache* Base, const Core::String& Name)
			{
				Core::Vector<Graphics::ElementBuffer*> Buffers;
				Buffers.push_back(nullptr);
				Buffers.push_back(nullptr);

				if (!Base->Get(Buffers.data(), Name))
					return nullptr;

				TypeInfo Type = VirtualMachine::Get()->GetTypeInfoByDecl(TYPENAME_ARRAY "<" TYPENAME_ELEMENTBUFFER "@>@");
				return Array::Compose(Type.GetTypeInfo(), Buffers);
			}
			bool PrimitiveCacheFree(Engine::PrimitiveCache* Base, const Core::String& Name, Graphics::ElementBuffer* Buffer1, Graphics::ElementBuffer* Buffer2)
			{
				Graphics::ElementBuffer* Buffers[2];
				Buffers[0] = Buffer1;
				Buffers[1] = Buffer2;
				return Base->Free(Name, Buffers);
			}
			Core::String PrimitiveCacheFind(Engine::PrimitiveCache* Base, Graphics::ElementBuffer* Buffer1, Graphics::ElementBuffer* Buffer2)
			{
				Graphics::ElementBuffer* Buffers[2];
				Buffers[0] = Buffer1;
				Buffers[1] = Buffer2;
				return Base->Find(Buffers);
			}
			Array* PrimitiveCacheGetSphereBuffers(Engine::PrimitiveCache* Base)
			{
				Core::Vector<Graphics::ElementBuffer*> Buffers;
				Buffers.push_back(nullptr);
				Buffers.push_back(nullptr);
				Base->GetSphereBuffers(Buffers.data());

				TypeInfo Type = VirtualMachine::Get()->GetTypeInfoByDecl(TYPENAME_ARRAY "<" TYPENAME_ELEMENTBUFFER "@>@");
				return Array::Compose(Type.GetTypeInfo(), Buffers);
			}
			Array* PrimitiveCacheGetCubeBuffers(Engine::PrimitiveCache* Base)
			{
				Core::Vector<Graphics::ElementBuffer*> Buffers;
				Buffers.push_back(nullptr);
				Buffers.push_back(nullptr);
				Base->GetCubeBuffers(Buffers.data());

				TypeInfo Type = VirtualMachine::Get()->GetTypeInfoByDecl(TYPENAME_ARRAY "<" TYPENAME_ELEMENTBUFFER "@>@");
				return Array::Compose(Type.GetTypeInfo(), Buffers);
			}
			Array* PrimitiveCacheGetBoxBuffers(Engine::PrimitiveCache* Base)
			{
				Core::Vector<Graphics::ElementBuffer*> Buffers;
				Buffers.push_back(nullptr);
				Buffers.push_back(nullptr);
				Base->GetBoxBuffers(Buffers.data());

				TypeInfo Type = VirtualMachine::Get()->GetTypeInfoByDecl(TYPENAME_ARRAY "<" TYPENAME_ELEMENTBUFFER "@>@");
				return Array::Compose(Type.GetTypeInfo(), Buffers);
			}
			Array* PrimitiveCacheGetSkinBoxBuffers(Engine::PrimitiveCache* Base)
			{
				Core::Vector<Graphics::ElementBuffer*> Buffers;
				Buffers.push_back(nullptr);
				Buffers.push_back(nullptr);
				Base->GetSkinBoxBuffers(Buffers.data());

				TypeInfo Type = VirtualMachine::Get()->GetTypeInfoByDecl(TYPENAME_ARRAY "<" TYPENAME_ELEMENTBUFFER "@>@");
				return Array::Compose(Type.GetTypeInfo(), Buffers);
			}

			void* ContentManagerLoad(Engine::ContentManager* Base, Engine::Processor* Source, const Core::String& Path, Core::Schema* Args)
			{
				return Base->Load(Source, Path, ToVariantKeys(Args));
			}
			bool ContentManagerSave(Engine::ContentManager* Base, Engine::Processor* Source, const Core::String& Path, void* Object, Core::Schema* Args)
			{
				return Base->Save(Source, Path, Object, ToVariantKeys(Args));
			}
			void ContentManagerLoadAsync2(Engine::ContentManager* Base, Engine::Processor* Source, const Core::String& Path, Core::Schema* Args, asIScriptFunction* Callback)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				if (!Context || !Callback)
					return;

				Callback->AddRef();
				Context->AddRef();
				Base->LoadAsync(Source, Path, ToVariantKeys(Args)).When([Context, Callback](void*&& Object)
				{
					Context->Execute(Callback, [Object](ImmediateContext* Context)
					{
						Context->SetArgAddress(0, Object);
					}).When([Context, Callback](int&&)
					{
						Callback->Release();
						Context->Release();
					});
				});
			}
			void ContentManagerLoadAsync1(Engine::ContentManager* Base, Engine::Processor* Source, const Core::String& Path, asIScriptFunction* Callback)
			{
				ContentManagerLoadAsync2(Base, Source, Path, nullptr, Callback);
			}
			void ContentManagerSaveAsync2(Engine::ContentManager* Base, Engine::Processor* Source, const Core::String& Path, void* Object, Core::Schema* Args, asIScriptFunction* Callback)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				if (!Context || !Callback)
					return;

				Callback->AddRef();
				Context->AddRef();
				Base->SaveAsync(Source, Path, Object, ToVariantKeys(Args)).When([Context, Callback](bool&& Success)
				{
					Context->Execute(Callback, [Success](ImmediateContext* Context)
					{
						Context->SetArg8(0, Success);
					}).When([Context, Callback](int&&)
					{
						Callback->Release();
						Context->Release();
					});
				});
			}
			void ContentManagerSaveAsync1(Engine::ContentManager* Base, Engine::Processor* Source, const Core::String& Path, void* Object, asIScriptFunction* Callback)
			{
				ContentManagerSaveAsync2(Base, Source, Path, Object, nullptr, Callback);
			}
			bool ContentManagerFindCache1(Engine::ContentManager* Base, Engine::Processor* Source, Engine::AssetCache& Output, const Core::String& Path)
			{
				auto* Cache = Base->FindCache(Source, Path);
				if (!Cache)
					return false;

				Output = *Cache;
				return true;
			}
			bool ContentManagerFindCache2(Engine::ContentManager* Base, Engine::Processor* Source, Engine::AssetCache& Output, void* Object)
			{
				auto* Cache = Base->FindCache(Source, Object);
				if (!Cache)
					return false;

				Output = *Cache;
				return true;
			}
			Engine::Processor* ContentManagerAddProcessor(Engine::ContentManager* Base, Engine::Processor* Source, uint64_t Id)
			{
				if (!Source)
					return nullptr;

				return Base->AddProcessor(Source, Id);
			}

			Engine::SceneGraph::Desc::Dependencies& SceneGraphSharedDescCopy(Engine::SceneGraph::Desc::Dependencies& Base, Engine::SceneGraph::Desc::Dependencies& Other)
			{
				if (&Base == &Other)
					return Base;

				VI_RELEASE(Base.Device);
				VI_RELEASE(Base.Activity);
				VI_RELEASE(Base.VM);
				VI_RELEASE(Base.Content);
				VI_RELEASE(Base.Primitives);
				VI_RELEASE(Base.Shaders);
				Base = Other;
				VI_ASSIGN(Base.Device, Other.Device);
				VI_ASSIGN(Base.Activity, Other.Activity);
				VI_ASSIGN(Base.VM, Other.VM);
				VI_ASSIGN(Base.Content, Other.Content);
				VI_ASSIGN(Base.Primitives, Other.Primitives);
				VI_ASSIGN(Base.Shaders, Other.Shaders);

				return Base;
			}
			void SceneGraphSharedDescDestructor(Engine::SceneGraph::Desc::Dependencies& Base)
			{
				VI_RELEASE(Base.Device);
				VI_RELEASE(Base.Activity);
				VI_RELEASE(Base.VM);
				VI_RELEASE(Base.Content);
				VI_RELEASE(Base.Primitives);
				VI_RELEASE(Base.Shaders);
			}

			void SceneGraphRayTest(Engine::SceneGraph* Base, uint64_t Id, const Compute::Ray& Ray, asIScriptFunction* Callback)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				if (!Context || !Callback)
					return;

				Base->RayTest(Id, Ray, [Context, Callback](Engine::Component* Source, const Compute::Vector3& Where)
				{
					Context->ExecuteSubcall(Callback, [&Source, &Where](ImmediateContext* Context)
					{
						Context->SetArgObject(0, (void*)Source);
						Context->SetArgObject(1, (void*)&Where);
					});
					return (bool)Context->GetReturnByte();
				});
			}
			void SceneGraphMutate1(Engine::SceneGraph* Base, Engine::Entity* Source, Engine::Entity* Child, const Core::String& Name)
			{
				Base->Mutate(Source, Child, Name.c_str());
			}
			void SceneGraphMutate2(Engine::SceneGraph* Base, Engine::Entity* Source, const Core::String& Name)
			{
				Base->Mutate(Source, Name.c_str());
			}
			void SceneGraphMutate3(Engine::SceneGraph* Base, Engine::Component* Source, const Core::String& Name)
			{
				Base->Mutate(Source, Name.c_str());
			}
			void SceneGraphMutate4(Engine::SceneGraph* Base, Engine::Material* Source, const Core::String& Name)
			{
				Base->Mutate(Source, Name.c_str());
			}
			void SceneGraphTransaction(Engine::SceneGraph* Base, asIScriptFunction* Callback)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				if (!Context || !Callback)
					return;

				Context->AddRef();
				Callback->AddRef();
				Base->Transaction([Context, Callback]()
				{
					Context->Execute(Callback, nullptr).When([Context, Callback](int&&)
					{
						Context->Release();
						Callback->Release();
					});
				});
			}
			void SceneGraphPushEvent1(Engine::SceneGraph* Base, const Core::String& Name, Core::Schema* Args, bool Propagate)
			{
				Base->PushEvent(Name, ToVariantKeys(Args), Propagate);
			}
			void SceneGraphPushEvent2(Engine::SceneGraph* Base, const Core::String& Name, Core::Schema* Args, Engine::Component* Source)
			{
				Base->PushEvent(Name, ToVariantKeys(Args), Source);
			}
			void SceneGraphPushEvent3(Engine::SceneGraph* Base, const Core::String& Name, Core::Schema* Args, Engine::Entity* Source)
			{
				Base->PushEvent(Name, ToVariantKeys(Args), Source);
			}
			void* SceneGraphSetListener(Engine::SceneGraph* Base, const Core::String& Name, asIScriptFunction* Callback)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				if (!Context || !Callback)
					return nullptr;

				return Base->SetListener(Name, [Context, Callback](const Core::String& Name, Core::VariantArgs& BaseArgs)
				{
					Core::Schema* Args = Core::Var::Set::Object();
					Args->Reserve(BaseArgs.size());
					for (auto& Item : BaseArgs)
						Args->Set(Item.first, Item.second);

					Callback->AddRef();
					Context->AddRef();
					Context->Execute(Callback, [Name, Args](ImmediateContext* Context)
					{
						Context->SetArgObject(0, (void*)&Name);
						Context->SetArgObject(1, (void*)Args);
					}).When([Context, Callback, Args](int&&)
					{
						Context->Release();
						Callback->Release();
						VI_RELEASE(Args);
					});
				});
			}
			void SceneGraphLoadResource(Engine::SceneGraph* Base, uint64_t Id, Engine::Component* Source, const Core::String& Path, Core::Schema* Args, asIScriptFunction* Callback)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				if (!Context || !Callback)
					return;

				Context->AddRef();
				Callback->AddRef();
				Base->LoadResource(Id, Source, Path, ToVariantKeys(Args), [Context, Callback](void* Resource)
				{
					Context->Execute(Callback, [Resource](ImmediateContext* Context)
					{
						Context->SetArgAddress(0, Resource);
					}).When([Context, Callback](int&&)
					{
						Context->Release();
						Callback->Release();
					});
				});
			}
			Array* SceneGraphGetComponents(Engine::SceneGraph* Base, uint64_t Id)
			{
				auto& Data = Base->GetComponents(Id);
				Core::Vector<Engine::Component*> Output;
				Output.reserve(Data.Size());

				for (auto* Next : Data)
					Output.push_back(Next);

				TypeInfo Type = VirtualMachine::Get()->GetTypeInfoByDecl(TYPENAME_ARRAY "<" TYPENAME_COMPONENT "@>@");
				return Array::Compose(Type.GetTypeInfo(), Output);
			}
			Array* SceneGraphGetActors(Engine::SceneGraph* Base, Engine::ActorType Source)
			{
				auto& Data = Base->GetActors(Source);
				Core::Vector<Engine::Component*> Output;
				Output.reserve(Data.Size());

				for (auto* Next : Data)
					Output.push_back(Next);

				TypeInfo Type = VirtualMachine::Get()->GetTypeInfoByDecl(TYPENAME_ARRAY "<" TYPENAME_COMPONENT "@>@");
				return Array::Compose(Type.GetTypeInfo(), Output);
			}
			Array* SceneGraphCloneEntityAsArray(Engine::SceneGraph* Base, Engine::Entity* Source)
			{
				TypeInfo Type = VirtualMachine::Get()->GetTypeInfoByDecl(TYPENAME_ARRAY "<" TYPENAME_ENTITY "@>@");
				return Array::Compose(Type.GetTypeInfo(), Base->CloneEntityAsArray(Source));
			}
			Array* SceneGraphQueryByParent(Engine::SceneGraph* Base, Engine::Entity* Source)
			{
				TypeInfo Type = VirtualMachine::Get()->GetTypeInfoByDecl(TYPENAME_ARRAY "<" TYPENAME_ENTITY "@>@");
				return Array::Compose(Type.GetTypeInfo(), Base->QueryByParent(Source));
			}
			Array* SceneGraphQueryByName(Engine::SceneGraph* Base, const Core::String& Source)
			{
				TypeInfo Type = VirtualMachine::Get()->GetTypeInfoByDecl(TYPENAME_ARRAY "<" TYPENAME_ENTITY "@>@");
				return Array::Compose(Type.GetTypeInfo(), Base->QueryByName(Source));
			}
			Array* SceneGraphQueryByPosition(Engine::SceneGraph* Base, uint64_t Id, const Compute::Vector3& Position, float Radius)
			{
				TypeInfo Type = VirtualMachine::Get()->GetTypeInfoByDecl(TYPENAME_ARRAY "<" TYPENAME_COMPONENT "@>@");
				return Array::Compose(Type.GetTypeInfo(), Base->QueryByPosition(Id, Position, Radius));
			}
			Array* SceneGraphQueryByArea(Engine::SceneGraph* Base, uint64_t Id, const Compute::Vector3& Min, const Compute::Vector3& Max)
			{
				TypeInfo Type = VirtualMachine::Get()->GetTypeInfoByDecl(TYPENAME_ARRAY "<" TYPENAME_COMPONENT "@>@");
				return Array::Compose(Type.GetTypeInfo(), Base->QueryByArea(Id, Min, Max));
			}
			Array* SceneGraphQueryByRay(Engine::SceneGraph* Base, uint64_t Id, const Compute::Ray& Ray)
			{
				TypeInfo Type = VirtualMachine::Get()->GetTypeInfoByDecl(TYPENAME_ARRAY "<" TYPENAME_COMPONENT "@>@");
				return Array::Compose(Type.GetTypeInfo(), Base->QueryByRay(Id, Ray));
			}
			Array* SceneGraphQueryByMatch(Engine::SceneGraph* Base, uint64_t Id, asIScriptFunction* Callback)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				if (!Context || !Callback)
					return nullptr;

				TypeInfo Type = VirtualMachine::Get()->GetTypeInfoByDecl(TYPENAME_ARRAY "<" TYPENAME_COMPONENT "@>@");
				return Array::Compose(Type.GetTypeInfo(), Base->QueryByMatch(Id, [Context, Callback](const Compute::Bounding& Source)
				{
					Context->ExecuteSubcall(Callback, [&Source](ImmediateContext* Context)
					{
						Context->SetArgObject(0, (void*)&Source);
					});
					return (bool)Context->GetReturnByte();
				}));
			}

			Application::CacheInfo& ApplicationCacheInfoCopy(Application::CacheInfo& Base, Application::CacheInfo& Other)
			{
				if (&Base == &Other)
					return Base;

				VI_RELEASE(Base.Primitives);
				VI_RELEASE(Base.Shaders);
				Base = Other;
				VI_ASSIGN(Base.Primitives, Other.Primitives);
				VI_ASSIGN(Base.Shaders, Other.Shaders);

				return Base;
			}
			void ApplicationCacheInfoDestructor(Application::CacheInfo& Base)
			{
				VI_RELEASE(Base.Primitives);
				VI_RELEASE(Base.Shaders);
			}

			bool IElementDispatchEvent(Engine::GUI::IElement& Base, const Core::String& Name, Core::Schema* Args)
			{
				Core::VariantArgs Data;
				if (Args != nullptr)
				{
					Data.reserve(Args->Size());
					for (auto Item : Args->GetChilds())
						Data[Item->Key] = Item->Value;
				}

				return Base.DispatchEvent(Name, Data);
			}
			Array* IElementQuerySelectorAll(Engine::GUI::IElement& Base, const Core::String& Value)
			{
				TypeInfo Type = VirtualMachine::Get()->GetTypeInfoByDecl(TYPENAME_ARRAY "<" TYPENAME_ELEMENTNODE ">@");
				return Array::Compose(Type.GetTypeInfo(), Base.QuerySelectorAll(Value));
			}

			bool IElementDocumentDispatchEvent(Engine::GUI::IElementDocument& Base, const Core::String& Name, Core::Schema* Args)
			{
				Core::VariantArgs Data;
				if (Args != nullptr)
				{
					Data.reserve(Args->Size());
					for (auto Item : Args->GetChilds())
						Data[Item->Key] = Item->Value;
				}

				return Base.DispatchEvent(Name, Data);
			}
			Array* IElementDocumentQuerySelectorAll(Engine::GUI::IElementDocument& Base, const Core::String& Value)
			{
				TypeInfo Type = VirtualMachine::Get()->GetTypeInfoByDecl(TYPENAME_ARRAY "<" TYPENAME_ELEMENTNODE ">@");
				return Array::Compose(Type.GetTypeInfo(), Base.QuerySelectorAll(Value));
			}

			bool DataModelSetRecursive(Engine::GUI::DataNode* Node, Core::Schema* Data, size_t Depth)
			{
				size_t Index = 0;
				for (auto& Item : Data->GetChilds())
				{
					auto& Child = Node->Add(Item->Value);
					Child.SetTop((void*)(uintptr_t)Index++, Depth);
					if (Item->Value.IsObject())
						DataModelSetRecursive(&Child, Item, Depth + 1);
				}

				Node->SortTree();
				return true;
			}
			bool DataModelGetRecursive(Engine::GUI::DataNode* Node, Core::Schema* Data)
			{
				size_t Size = Node->GetSize();
				for (size_t i = 0; i < Size; i++)
				{
					auto& Child = Node->At(i);
					DataModelGetRecursive(&Child, Data->Push(Child.Get()));
				}

				return true;
			}
			bool DataModelSet(Engine::GUI::DataModel* Base, const Core::String& Name, Core::Schema* Data)
			{
				if (!Data->Value.IsObject())
					return Base->SetProperty(Name, Data->Value) != nullptr;

				Engine::GUI::DataNode* Node = Base->SetArray(Name);
				if (!Node)
					return false;

				return DataModelSetRecursive(Node, Data, 0);
			}
			bool DataModelSetVar(Engine::GUI::DataModel* Base, const Core::String& Name, const Core::Variant& Data)
			{
				return Base->SetProperty(Name, Data) != nullptr;
			}
			bool DataModelSetString(Engine::GUI::DataModel* Base, const Core::String& Name, const Core::String& Value)
			{
				return Base->SetString(Name, Value) != nullptr;
			}
			bool DataModelSetInteger(Engine::GUI::DataModel* Base, const Core::String& Name, int64_t Value)
			{
				return Base->SetInteger(Name, Value) != nullptr;
			}
			bool DataModelSetFloat(Engine::GUI::DataModel* Base, const Core::String& Name, float Value)
			{
				return Base->SetFloat(Name, Value) != nullptr;
			}
			bool DataModelSetDouble(Engine::GUI::DataModel* Base, const Core::String& Name, double Value)
			{
				return Base->SetDouble(Name, Value) != nullptr;
			}
			bool DataModelSetBoolean(Engine::GUI::DataModel* Base, const Core::String& Name, bool Value)
			{
				return Base->SetBoolean(Name, Value) != nullptr;
			}
			bool DataModelSetPointer(Engine::GUI::DataModel* Base, const Core::String& Name, void* Value)
			{
				return Base->SetPointer(Name, Value) != nullptr;
			}
			bool DataModelSetCallback(Engine::GUI::DataModel* Base, const Core::String& Name, asIScriptFunction* Callback)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				if (!Context)
					return false;

				if (!Callback)
					return false;

				Callback->AddRef();
				Context->AddRef();

				TypeInfo Type = VirtualMachine::Get()->GetTypeInfoByDecl(TYPENAME_ARRAY "<" TYPENAME_VARIANT ">@");
				Base->SetDetachCallback([Callback, Context]()
				{
					Callback->Release();
					Context->Release();
				});

				return Base->SetCallback(Name, [Type, Context, Callback](Engine::GUI::IEvent& Wrapper, const Core::VariantList& Args)
				{
					Engine::GUI::IEvent Event = Wrapper.Copy();
					Array* Data = Array::Compose(Type.GetTypeInfo(), Args);
					Context->Execute(Callback, [Event, &Data](ImmediateContext* Context)
					{
						Engine::GUI::IEvent Copy = Event;
						Context->SetArgObject(0, &Copy);
						Context->SetArgObject(1, &Data);
					}).When([Event](int&&)
					{
						Engine::GUI::IEvent Copy = Event;
						Copy.Release();
					});
				});
			}
			Core::Schema* DataModelGet(Engine::GUI::DataModel* Base, const Core::String& Name)
			{
				Engine::GUI::DataNode* Node = Base->GetProperty(Name);
				if (!Node)
					return nullptr;

				Core::Schema* Result = new Core::Schema(Node->Get());
				if (Result->Value.IsObject())
					DataModelGetRecursive(Node, Result);

				return Result;
			}

			Engine::GUI::IElement ContextGetFocusElement(Engine::GUI::Context* Base, const Compute::Vector2& Value)
			{
				return Base->GetElementAtPoint(Value);
			}

			ModelListener::ModelListener(asIScriptFunction* NewCallback) noexcept : Base(new Engine::GUI::Listener(Bind(NewCallback))), Source(NewCallback), Context(nullptr)
			{
			}
			ModelListener::ModelListener(const Core::String& FunctionName) noexcept : Base(new Engine::GUI::Listener(FunctionName)), Source(nullptr), Context(nullptr)
			{
			}
			ModelListener::~ModelListener() noexcept
			{
				VI_CLEAR(Source);
				VI_CLEAR(Context);
				VI_CLEAR(Base);
			}
			asIScriptFunction* ModelListener::GetCallback()
			{
				return Source;
			}
			Engine::GUI::EventCallback ModelListener::Bind(asIScriptFunction* Callback)
			{
				if (Context != nullptr)
					Context->Release();
				Context = ImmediateContext::Get();

				if (Source != nullptr)
					Source->Release();
				Source = Callback;

				if (!Context || !Source)
					return nullptr;

				Source->AddRef();
				Context->AddRef();

				return [this](Engine::GUI::IEvent& Wrapper)
				{
					Engine::GUI::IEvent Event = Wrapper.Copy();
					Context->Execute(Source, [Event](ImmediateContext* Context)
					{
						Engine::GUI::IEvent Copy = Event;
						Context->SetArgObject(0, &Copy);
					}).When([Event](int&&)
					{
						Engine::GUI::IEvent Copy = Event;
						Copy.Release();
					});
				};
			}

			void ComponentsSoftBodyLoad(Engine::Components::SoftBody* Base, const Core::String& Path, float Ant, asIScriptFunction* Callback)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				if (Context != nullptr && Callback != nullptr)
				{
					Context->AddRef();
					Callback->AddRef();
				}

				Base->Load(Path, Ant, [Context, Callback]()
				{
					if (!Context || !Callback)
						return;

					Context->Execute(Callback, nullptr).When([Context, Callback](int&&)
					{
						Callback->Release();
						Context->Release();
					});
				});
			}

			void ComponentsRigidBodyLoad(Engine::Components::RigidBody* Base, const Core::String& Path, float Mass, float Ant, asIScriptFunction* Callback)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				if (Context != nullptr && Callback != nullptr)
				{
					Context->AddRef();
					Callback->AddRef();
				}

				Base->Load(Path, Mass, Ant, [Context, Callback]()
				{
					if (!Context || !Callback)
						return;

					Context->Execute(Callback, nullptr).When([Context, Callback](int&&)
					{
						Callback->Release();
						Context->Release();
					});
				});
			}

			void ComponentsKeyAnimatorLoadAnimation(Engine::Components::KeyAnimator* Base, const Core::String& Path, asIScriptFunction* Callback)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				if (Context != nullptr && Callback != nullptr)
				{
					Context->AddRef();
					Callback->AddRef();
				}

				Base->LoadAnimation(Path, [Context, Callback](bool)
				{
					if (!Context || !Callback)
						return;

					Context->Execute(Callback, nullptr).When([Context, Callback](int&&)
					{
						Callback->Release();
						Context->Release();
					});
				});
			}
#endif
			bool Registry::LoadCTypes(VirtualMachine* VM)
			{
				VI_ASSERT(VM != nullptr && VM->GetEngine() != nullptr, false, "manager should be set");
				asIScriptEngine* Engine = VM->GetEngine();
#ifdef VI_64
				Engine->RegisterTypedef("usize", "uint64");
#else
				Engine->RegisterTypedef("usize", "uint32");
#endif
				RefClass VPointer(VM, "uptr", Engine->RegisterObjectType("uptr", 0, asOBJ_REF | asOBJ_NOCOUNT));
				Engine->RegisterObjectMethod("uptr", "void opCast(?&out)", asFUNCTIONPR(PointerToHandleCast, (void*, void**, int), void), asCALL_CDECL_OBJFIRST);
				Engine->RegisterGlobalFunction("uptr@ to_ptr(?&in)", asFUNCTION(HandleToPointerCast), asCALL_CDECL);

				return true;
			}
			bool Registry::LoadAny(VirtualMachine* VM)
			{
				VI_ASSERT(VM != nullptr && VM->GetEngine() != nullptr, false, "manager should be set");

				asIScriptEngine* Engine = VM->GetEngine();
				Engine->RegisterObjectType("any", sizeof(Any), asOBJ_REF | asOBJ_GC);
				Engine->RegisterObjectBehaviour("any", asBEHAVE_FACTORY, "any@ f()", asFUNCTION(Any::Factory1), asCALL_GENERIC);
				Engine->RegisterObjectBehaviour("any", asBEHAVE_FACTORY, "any@ f(?&in) explicit", asFUNCTION(Any::Factory2), asCALL_GENERIC);
				Engine->RegisterObjectBehaviour("any", asBEHAVE_FACTORY, "any@ f(const int64&in) explicit", asFUNCTION(Any::Factory2), asCALL_GENERIC);
				Engine->RegisterObjectBehaviour("any", asBEHAVE_FACTORY, "any@ f(const double&in) explicit", asFUNCTION(Any::Factory2), asCALL_GENERIC);
				Engine->RegisterObjectBehaviour("any", asBEHAVE_ADDREF, "void f()", asMETHOD(Any, AddRef), asCALL_THISCALL);
				Engine->RegisterObjectBehaviour("any", asBEHAVE_RELEASE, "void f()", asMETHOD(Any, Release), asCALL_THISCALL);
				Engine->RegisterObjectBehaviour("any", asBEHAVE_GETREFCOUNT, "int f()", asMETHOD(Any, GetRefCount), asCALL_THISCALL);
				Engine->RegisterObjectBehaviour("any", asBEHAVE_SETGCFLAG, "void f()", asMETHOD(Any, SetFlag), asCALL_THISCALL);
				Engine->RegisterObjectBehaviour("any", asBEHAVE_GETGCFLAG, "bool f()", asMETHOD(Any, GetFlag), asCALL_THISCALL);
				Engine->RegisterObjectBehaviour("any", asBEHAVE_ENUMREFS, "void f(int&in)", asMETHOD(Any, EnumReferences), asCALL_THISCALL);
				Engine->RegisterObjectBehaviour("any", asBEHAVE_RELEASEREFS, "void f(int&in)", asMETHOD(Any, ReleaseAllHandles), asCALL_THISCALL);
				Engine->RegisterObjectMethod("any", "any &opAssign(any&in)", asFUNCTION(Any::Assignment), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("any", "void store(?&in)", asMETHODPR(Any, Store, (void*, int), void), asCALL_THISCALL);
				Engine->RegisterObjectMethod("any", "bool retrieve(?&out)", asMETHODPR(Any, Retrieve, (void*, int) const, bool), asCALL_THISCALL);

				return true;
			}
			bool Registry::LoadArray(VirtualMachine* VM)
			{
				VI_ASSERT(VM != nullptr && VM->GetEngine() != nullptr, false, "manager should be set");

				asIScriptEngine* Engine = VM->GetEngine();
				Engine->SetTypeInfoUserDataCleanupCallback(Array::CleanupTypeInfoCache, ARRAY_CACHE);
				Engine->RegisterObjectType("array<class T>", 0, asOBJ_REF | asOBJ_GC | asOBJ_TEMPLATE);
				Engine->RegisterObjectBehaviour("array<T>", asBEHAVE_TEMPLATE_CALLBACK, "bool f(int&in, bool&out)", asFUNCTION(Array::TemplateCallback), asCALL_CDECL);
				Engine->RegisterObjectBehaviour("array<T>", asBEHAVE_FACTORY, "array<T>@ f(int&in)", asFUNCTIONPR(Array::Create, (asITypeInfo*), Array*), asCALL_CDECL);
				Engine->RegisterObjectBehaviour("array<T>", asBEHAVE_FACTORY, "array<T>@ f(int&in, usize length) explicit", asFUNCTIONPR(Array::Create, (asITypeInfo*, size_t), Array*), asCALL_CDECL);
				Engine->RegisterObjectBehaviour("array<T>", asBEHAVE_FACTORY, "array<T>@ f(int&in, usize length, const T &in Value)", asFUNCTIONPR(Array::Create, (asITypeInfo*, size_t, void*), Array*), asCALL_CDECL);
				Engine->RegisterObjectBehaviour("array<T>", asBEHAVE_LIST_FACTORY, "array<T>@ f(int&in Type, int&in InitList) {repeat T}", asFUNCTIONPR(Array::Create, (asITypeInfo*, void*), Array*), asCALL_CDECL);
				Engine->RegisterObjectBehaviour("array<T>", asBEHAVE_ADDREF, "void f()", asMETHOD(Array, AddRef), asCALL_THISCALL);
				Engine->RegisterObjectBehaviour("array<T>", asBEHAVE_RELEASE, "void f()", asMETHOD(Array, Release), asCALL_THISCALL);
				Engine->RegisterObjectMethod("array<T>", "T &opIndex(usize Index)", asMETHODPR(Array, At, (size_t), void*), asCALL_THISCALL);
				Engine->RegisterObjectMethod("array<T>", "const T &opIndex(usize Index) const", asMETHODPR(Array, At, (size_t) const, const void*), asCALL_THISCALL);
				Engine->RegisterObjectMethod("array<T>", "array<T> &opAssign(const array<T>&in)", asMETHOD(Array, operator=), asCALL_THISCALL);
				Engine->RegisterObjectMethod("array<T>", "void insert_at(usize Index, const T&in Value)", asMETHODPR(Array, InsertAt, (size_t, void*), void), asCALL_THISCALL);
				Engine->RegisterObjectMethod("array<T>", "void insert_at(usize Index, const array<T>& Array)", asMETHODPR(Array, InsertAt, (size_t, const Array&), void), asCALL_THISCALL);
				Engine->RegisterObjectMethod("array<T>", "void insert_last(const T&in Value)", asMETHOD(Array, InsertLast), asCALL_THISCALL);
				Engine->RegisterObjectMethod("array<T>", "void push(const T&in Value)", asMETHOD(Array, InsertLast), asCALL_THISCALL);
				Engine->RegisterObjectMethod("array<T>", "void pop()", asMETHOD(Array, RemoveLast), asCALL_THISCALL);
				Engine->RegisterObjectMethod("array<T>", "void remove_at(usize Index)", asMETHOD(Array, RemoveAt), asCALL_THISCALL);
				Engine->RegisterObjectMethod("array<T>", "void remove_last()", asMETHOD(Array, RemoveLast), asCALL_THISCALL);
				Engine->RegisterObjectMethod("array<T>", "void remove_range(usize Start, usize Count)", asMETHOD(Array, RemoveRange), asCALL_THISCALL);
				Engine->RegisterObjectMethod("array<T>", "usize size() const", asMETHOD(Array, GetSize), asCALL_THISCALL);
				Engine->RegisterObjectMethod("array<T>", "void reserve(usize length)", asMETHOD(Array, Reserve), asCALL_THISCALL);
				Engine->RegisterObjectMethod("array<T>", "void resize(usize length)", asMETHODPR(Array, Resize, (size_t), void), asCALL_THISCALL);
				Engine->RegisterObjectMethod("array<T>", "void sort_asc()", asMETHODPR(Array, SortAsc, (), void), asCALL_THISCALL);
				Engine->RegisterObjectMethod("array<T>", "void sort_asc(usize StartAt, usize Count)", asMETHODPR(Array, SortAsc, (size_t, size_t), void), asCALL_THISCALL);
				Engine->RegisterObjectMethod("array<T>", "void sort_desc()", asMETHODPR(Array, SortDesc, (), void), asCALL_THISCALL);
				Engine->RegisterObjectMethod("array<T>", "void sort_desc(usize StartAt, usize Count)", asMETHODPR(Array, SortDesc, (size_t, size_t), void), asCALL_THISCALL);
				Engine->RegisterObjectMethod("array<T>", "void reverse()", asMETHOD(Array, Reverse), asCALL_THISCALL);
				Engine->RegisterObjectMethod("array<T>", "usize find(const T&in if_handle_then_const Value) const", asMETHODPR(Array, Find, (void*) const, int), asCALL_THISCALL);
				Engine->RegisterObjectMethod("array<T>", "usize find(usize StartAt, const T&in if_handle_then_const Value) const", asMETHODPR(Array, Find, (size_t, void*) const, int), asCALL_THISCALL);
				Engine->RegisterObjectMethod("array<T>", "usize find_by_ref(const T&in if_handle_then_const Value) const", asMETHODPR(Array, FindByRef, (void*) const, int), asCALL_THISCALL);
				Engine->RegisterObjectMethod("array<T>", "usize find_by_ref(usize StartAt, const T&in if_handle_then_const Value) const", asMETHODPR(Array, FindByRef, (size_t, void*) const, int), asCALL_THISCALL);
				Engine->RegisterObjectMethod("array<T>", "bool opEquals(const array<T>&in) const", asMETHOD(Array, operator==), asCALL_THISCALL);
				Engine->RegisterObjectMethod("array<T>", "bool empty() const", asMETHOD(Array, IsEmpty), asCALL_THISCALL);
				Engine->RegisterFuncdef("bool array<T>::less(const T&in if_handle_then_const a, const T&in if_handle_then_const b)");
				Engine->RegisterObjectMethod("array<T>", "void sort(const less &in, usize StartAt = 0, usize Count = usize(-1))", asMETHODPR(Array, Sort, (asIScriptFunction*, size_t, size_t), void), asCALL_THISCALL);
				Engine->RegisterObjectBehaviour("array<T>", asBEHAVE_GETREFCOUNT, "int f()", asMETHOD(Array, GetRefCount), asCALL_THISCALL);
				Engine->RegisterObjectBehaviour("array<T>", asBEHAVE_SETGCFLAG, "void f()", asMETHOD(Array, SetFlag), asCALL_THISCALL);
				Engine->RegisterObjectBehaviour("array<T>", asBEHAVE_GETGCFLAG, "bool f()", asMETHOD(Array, GetFlag), asCALL_THISCALL);
				Engine->RegisterObjectBehaviour("array<T>", asBEHAVE_ENUMREFS, "void f(int&in)", asMETHOD(Array, EnumReferences), asCALL_THISCALL);
				Engine->RegisterObjectBehaviour("array<T>", asBEHAVE_RELEASEREFS, "void f(int&in)", asMETHOD(Array, ReleaseAllHandles), asCALL_THISCALL);
				Engine->RegisterDefaultArrayType("array<T>");

				return true;
			}
			bool Registry::LoadComplex(VirtualMachine* VM)
			{
#ifdef VI_HAS_BINDINGS
				VI_ASSERT(VM != nullptr && VM->GetEngine() != nullptr, false, "manager should be set");
				asIScriptEngine* Engine = VM->GetEngine();
				Engine->RegisterObjectType("complex", sizeof(Complex), asOBJ_VALUE | asOBJ_POD | asGetTypeTraits<Complex>() | asOBJ_APP_CLASS_ALLFLOATS);
				Engine->RegisterObjectProperty("complex", "float R", asOFFSET(Complex, R));
				Engine->RegisterObjectProperty("complex", "float I", asOFFSET(Complex, I));
				Engine->RegisterObjectBehaviour("complex", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(Complex::DefaultConstructor), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectBehaviour("complex", asBEHAVE_CONSTRUCT, "void f(const complex &in)", asFUNCTION(Complex::CopyConstructor), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectBehaviour("complex", asBEHAVE_CONSTRUCT, "void f(float)", asFUNCTION(Complex::ConvConstructor), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectBehaviour("complex", asBEHAVE_CONSTRUCT, "void f(float, float)", asFUNCTION(Complex::InitConstructor), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectBehaviour("complex", asBEHAVE_LIST_CONSTRUCT, "void f(const int &in) {float, float}", asFUNCTION(Complex::ListConstructor), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("complex", "complex &opAddAssign(const complex &in)", asMETHODPR(Complex, operator+=, (const Complex&), Complex&), asCALL_THISCALL);
				Engine->RegisterObjectMethod("complex", "complex &opSubAssign(const complex &in)", asMETHODPR(Complex, operator-=, (const Complex&), Complex&), asCALL_THISCALL);
				Engine->RegisterObjectMethod("complex", "complex &opMulAssign(const complex &in)", asMETHODPR(Complex, operator*=, (const Complex&), Complex&), asCALL_THISCALL);
				Engine->RegisterObjectMethod("complex", "complex &opDivAssign(const complex &in)", asMETHODPR(Complex, operator/=, (const Complex&), Complex&), asCALL_THISCALL);
				Engine->RegisterObjectMethod("complex", "bool opEquals(const complex &in) const", asMETHODPR(Complex, operator==, (const Complex&) const, bool), asCALL_THISCALL);
				Engine->RegisterObjectMethod("complex", "complex opAdd(const complex &in) const", asMETHODPR(Complex, operator+, (const Complex&) const, Complex), asCALL_THISCALL);
				Engine->RegisterObjectMethod("complex", "complex opSub(const complex &in) const", asMETHODPR(Complex, operator-, (const Complex&) const, Complex), asCALL_THISCALL);
				Engine->RegisterObjectMethod("complex", "complex opMul(const complex &in) const", asMETHODPR(Complex, operator*, (const Complex&) const, Complex), asCALL_THISCALL);
				Engine->RegisterObjectMethod("complex", "complex opDiv(const complex &in) const", asMETHODPR(Complex, operator/, (const Complex&) const, Complex), asCALL_THISCALL);
				Engine->RegisterObjectMethod("complex", "float abs() const", asMETHOD(Complex, Length), asCALL_THISCALL);
				Engine->RegisterObjectMethod("complex", "complex get_ri() const property", asMETHOD(Complex, GetRI), asCALL_THISCALL);
				Engine->RegisterObjectMethod("complex", "complex get_ir() const property", asMETHOD(Complex, GetIR), asCALL_THISCALL);
				Engine->RegisterObjectMethod("complex", "void set_ri(const complex &in) property", asMETHOD(Complex, SetRI), asCALL_THISCALL);
				Engine->RegisterObjectMethod("complex", "void set_ir(const complex &in) property", asMETHOD(Complex, SetIR), asCALL_THISCALL);
				return true;
#else
				VI_ASSERT(false, false, "<complex> is not loaded");
				return false;
#endif
			}
			bool Registry::LoadDictionary(VirtualMachine* VM)
			{
				VI_ASSERT(VM != nullptr && VM->GetEngine() != nullptr, false, "manager should be set");
				asIScriptEngine* Engine = VM->GetEngine();
				Engine->RegisterObjectType("storable", sizeof(Storable), asOBJ_VALUE | asOBJ_ASHANDLE | asOBJ_GC | asGetTypeTraits<Storable>());
				Engine->RegisterObjectBehaviour("storable", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(Dictionary::KeyConstruct), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectBehaviour("storable", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(Dictionary::KeyDestruct), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectBehaviour("storable", asBEHAVE_ENUMREFS, "void f(int&in)", asMETHOD(Storable, EnumReferences), asCALL_THISCALL);
				Engine->RegisterObjectBehaviour("storable", asBEHAVE_RELEASEREFS, "void f(int&in)", asMETHOD(Storable, FreeValue), asCALL_THISCALL);
				Engine->RegisterObjectMethod("storable", "storable &opAssign(const storable &in)", asFUNCTIONPR(Dictionary::KeyopAssign, (const Storable&, Storable*), Storable&), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("storable", "storable &opHndlAssign(const ?&in)", asFUNCTIONPR(Dictionary::KeyopAssign, (void*, int, Storable*), Storable&), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("storable", "storable &opHndlAssign(const storable &in)", asFUNCTIONPR(Dictionary::KeyopAssign, (const Storable&, Storable*), Storable&), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("storable", "storable &opAssign(const ?&in)", asFUNCTIONPR(Dictionary::KeyopAssign, (void*, int, Storable*), Storable&), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("storable", "storable &opAssign(double)", asFUNCTIONPR(Dictionary::KeyopAssign, (double, Storable*), Storable&), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("storable", "storable &opAssign(int64)", asFUNCTIONPR(Dictionary::KeyopAssign, (as_int64_t, Storable*), Storable&), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("storable", "void opCast(?&out)", asFUNCTIONPR(Dictionary::KeyopCast, (void*, int, Storable*), void), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("storable", "void opConv(?&out)", asFUNCTIONPR(Dictionary::KeyopCast, (void*, int, Storable*), void), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("storable", "int64 opConv()", asFUNCTIONPR(Dictionary::KeyopConvInt, (Storable*), as_int64_t), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("storable", "double opConv()", asFUNCTIONPR(Dictionary::KeyopConvDouble, (Storable*), double), asCALL_CDECL_OBJLAST);

				Engine->RegisterObjectType("dictionary", sizeof(Dictionary), asOBJ_REF | asOBJ_GC);
				Engine->RegisterObjectBehaviour("dictionary", asBEHAVE_FACTORY, "dictionary@ f()", asFUNCTION(Dictionary::Factory), asCALL_GENERIC);
				Engine->RegisterObjectBehaviour("dictionary", asBEHAVE_LIST_FACTORY, "dictionary @f(int &in) {repeat {string, ?}}", asFUNCTION(Dictionary::ListFactory), asCALL_GENERIC);
				Engine->RegisterObjectBehaviour("dictionary", asBEHAVE_ADDREF, "void f()", asMETHOD(Dictionary, AddRef), asCALL_THISCALL);
				Engine->RegisterObjectBehaviour("dictionary", asBEHAVE_RELEASE, "void f()", asMETHOD(Dictionary, Release), asCALL_THISCALL);
				Engine->RegisterObjectMethod("dictionary", "dictionary &opAssign(const dictionary &in)", asMETHODPR(Dictionary, operator=, (const Dictionary&), Dictionary&), asCALL_THISCALL);
				Engine->RegisterObjectMethod("dictionary", "void set(const string &in, const ?&in)", asMETHODPR(Dictionary, Set, (const Core::String&, void*, int), void), asCALL_THISCALL);
				Engine->RegisterObjectMethod("dictionary", "bool get(const string &in, ?&out) const", asMETHODPR(Dictionary, Get, (const Core::String&, void*, int) const, bool), asCALL_THISCALL);
				Engine->RegisterObjectMethod("dictionary", "bool exists(const string &in) const", asMETHOD(Dictionary, Exists), asCALL_THISCALL);
				Engine->RegisterObjectMethod("dictionary", "bool empty() const", asMETHOD(Dictionary, IsEmpty), asCALL_THISCALL);
				Engine->RegisterObjectMethod("dictionary", "usize size() const", asMETHOD(Dictionary, GetSize), asCALL_THISCALL);
				Engine->RegisterObjectMethod("dictionary", "bool delete(const string &in)", asMETHOD(Dictionary, Delete), asCALL_THISCALL);
				Engine->RegisterObjectMethod("dictionary", "void delete_all()", asMETHOD(Dictionary, DeleteAll), asCALL_THISCALL);
				Engine->RegisterObjectMethod("dictionary", "array<string>@ get_keys() const", asMETHOD(Dictionary, GetKeys), asCALL_THISCALL);
				Engine->RegisterObjectMethod("dictionary", "storable &opIndex(const string &in)", asMETHODPR(Dictionary, operator[], (const Core::String&), Storable*), asCALL_THISCALL);
				Engine->RegisterObjectMethod("dictionary", "const storable &opIndex(const string &in) const", asMETHODPR(Dictionary, operator[], (const Core::String&) const, const Storable*), asCALL_THISCALL);
				Engine->RegisterObjectBehaviour("dictionary", asBEHAVE_GETREFCOUNT, "int f()", asMETHOD(Dictionary, GetRefCount), asCALL_THISCALL);
				Engine->RegisterObjectBehaviour("dictionary", asBEHAVE_SETGCFLAG, "void f()", asMETHOD(Dictionary, SetGCFlag), asCALL_THISCALL);
				Engine->RegisterObjectBehaviour("dictionary", asBEHAVE_GETGCFLAG, "bool f()", asMETHOD(Dictionary, GetGCFlag), asCALL_THISCALL);
				Engine->RegisterObjectBehaviour("dictionary", asBEHAVE_ENUMREFS, "void f(int&in)", asMETHOD(Dictionary, EnumReferences), asCALL_THISCALL);
				Engine->RegisterObjectBehaviour("dictionary", asBEHAVE_RELEASEREFS, "void f(int&in)", asMETHOD(Dictionary, ReleaseAllReferences), asCALL_THISCALL);

				Dictionary::Setup(Engine);
				return true;
			}
			bool Registry::LoadRef(VirtualMachine* VM)
			{
				asIScriptEngine* Engine = VM->GetEngine();
				if (!Engine)
					return false;

				Engine->RegisterObjectType("ref", sizeof(Ref), asOBJ_VALUE | asOBJ_ASHANDLE | asOBJ_GC | asGetTypeTraits<Ref>());
				Engine->RegisterObjectBehaviour("ref", asBEHAVE_CONSTRUCT, "void f()", asFUNCTIONPR(Ref::Construct, (Ref*), void), asCALL_CDECL_OBJFIRST);
				Engine->RegisterObjectBehaviour("ref", asBEHAVE_CONSTRUCT, "void f(const ref &in)", asFUNCTIONPR(Ref::Construct, (Ref*, const Ref&), void), asCALL_CDECL_OBJFIRST);
				Engine->RegisterObjectBehaviour("ref", asBEHAVE_CONSTRUCT, "void f(const ?&in)", asFUNCTIONPR(Ref::Construct, (Ref*, void*, int), void), asCALL_CDECL_OBJFIRST);
				Engine->RegisterObjectBehaviour("ref", asBEHAVE_DESTRUCT, "void f()", asFUNCTIONPR(Ref::Destruct, (Ref*), void), asCALL_CDECL_OBJFIRST);
				Engine->RegisterObjectBehaviour("ref", asBEHAVE_ENUMREFS, "void f(int&in)", asMETHOD(Ref, EnumReferences), asCALL_THISCALL);
				Engine->RegisterObjectBehaviour("ref", asBEHAVE_RELEASEREFS, "void f(int&in)", asMETHOD(Ref, ReleaseReferences), asCALL_THISCALL);
				Engine->RegisterObjectMethod("ref", "void opCast(?&out)", asMETHODPR(Ref, Cast, (void**, int), void), asCALL_THISCALL);
				Engine->RegisterObjectMethod("ref", "ref &opHndlAssign(const ref &in)", asMETHOD(Ref, operator=), asCALL_THISCALL);
				Engine->RegisterObjectMethod("ref", "ref &opHndlAssign(const ?&in)", asMETHOD(Ref, Assign), asCALL_THISCALL);
				Engine->RegisterObjectMethod("ref", "bool opEquals(const ref &in) const", asMETHODPR(Ref, operator==, (const Ref&) const, bool), asCALL_THISCALL);
				Engine->RegisterObjectMethod("ref", "bool opEquals(const ?&in) const", asMETHODPR(Ref, Equals, (void*, int) const, bool), asCALL_THISCALL);
				return true;
			}
			bool Registry::LoadWeakRef(VirtualMachine* VM)
			{
				asIScriptEngine* Engine = VM->GetEngine();
				if (!Engine)
					return false;

				Engine->RegisterObjectType("weak<class T>", sizeof(Weak), asOBJ_VALUE | asOBJ_ASHANDLE | asOBJ_TEMPLATE | asOBJ_APP_CLASS_DAK);
				Engine->RegisterObjectBehaviour("weak<T>", asBEHAVE_CONSTRUCT, "void f(int&in)", asFUNCTION(Weak::Construct), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectBehaviour("weak<T>", asBEHAVE_CONSTRUCT, "void f(int&in, T@+)", asFUNCTION(Weak::Construct2), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectBehaviour("weak<T>", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(Weak::Destruct), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectBehaviour("weak<T>", asBEHAVE_TEMPLATE_CALLBACK, "bool f(int&in, bool&out)", asFUNCTION(Weak::TemplateCallback), asCALL_CDECL);
				Engine->RegisterObjectMethod("weak<T>", "T@ opImplCast()", asMETHOD(Weak, Get), asCALL_THISCALL);
				Engine->RegisterObjectMethod("weak<T>", "T@ get() const", asMETHODPR(Weak, Get, () const, void*), asCALL_THISCALL);
				Engine->RegisterObjectMethod("weak<T>", "weak<T> &opHndlAssign(const weak<T> &in)", asMETHOD(Weak, operator=), asCALL_THISCALL);
				Engine->RegisterObjectMethod("weak<T>", "weak<T> &opAssign(const weak<T> &in)", asMETHOD(Weak, operator=), asCALL_THISCALL);
				Engine->RegisterObjectMethod("weak<T>", "bool opEquals(const weak<T> &in) const", asMETHODPR(Weak, operator==, (const Weak&) const, bool), asCALL_THISCALL);
				Engine->RegisterObjectMethod("weak<T>", "weak<T> &opHndlAssign(T@)", asMETHOD(Weak, Set), asCALL_THISCALL);
				Engine->RegisterObjectMethod("weak<T>", "bool opEquals(const T@+) const", asMETHOD(Weak, Equals), asCALL_THISCALL);
				Engine->RegisterObjectType("const_weak<class T>", sizeof(Weak), asOBJ_VALUE | asOBJ_ASHANDLE | asOBJ_TEMPLATE | asOBJ_APP_CLASS_DAK);
				Engine->RegisterObjectBehaviour("const_weak<T>", asBEHAVE_CONSTRUCT, "void f(int&in)", asFUNCTION(Weak::Construct), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectBehaviour("const_weak<T>", asBEHAVE_CONSTRUCT, "void f(int&in, const T@+)", asFUNCTION(Weak::Construct2), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectBehaviour("const_weak<T>", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(Weak::Destruct), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectBehaviour("const_weak<T>", asBEHAVE_TEMPLATE_CALLBACK, "bool f(int&in, bool&out)", asFUNCTION(Weak::TemplateCallback), asCALL_CDECL);
				Engine->RegisterObjectMethod("const_weak<T>", "const T@ opImplCast() const", asMETHOD(Weak, Get), asCALL_THISCALL);
				Engine->RegisterObjectMethod("const_weak<T>", "const T@ get() const", asMETHODPR(Weak, Get, () const, void*), asCALL_THISCALL);
				Engine->RegisterObjectMethod("const_weak<T>", "const_weak<T> &opHndlAssign(const const_weak<T> &in)", asMETHOD(Weak, operator=), asCALL_THISCALL);
				Engine->RegisterObjectMethod("const_weak<T>", "const_weak<T> &opAssign(const const_weak<T> &in)", asMETHOD(Weak, operator=), asCALL_THISCALL);
				Engine->RegisterObjectMethod("const_weak<T>", "bool opEquals(const const_weak<T> &in) const", asMETHODPR(Weak, operator==, (const Weak&) const, bool), asCALL_THISCALL);
				Engine->RegisterObjectMethod("const_weak<T>", "const_weak<T> &opHndlAssign(const T@)", asMETHOD(Weak, Set), asCALL_THISCALL);
				Engine->RegisterObjectMethod("const_weak<T>", "bool opEquals(const T@+) const", asMETHOD(Weak, Equals), asCALL_THISCALL);
				Engine->RegisterObjectMethod("const_weak<T>", "const_weak<T> &opHndlAssign(const weak<T> &in)", asMETHOD(Weak, operator=), asCALL_THISCALL);
				Engine->RegisterObjectMethod("const_weak<T>", "bool opEquals(const weak<T> &in) const", asMETHODPR(Weak, operator==, (const Weak&) const, bool), asCALL_THISCALL);
				
				return true;
			}
			bool Registry::LoadMath(VirtualMachine* VM)
			{
				asIScriptEngine* Engine = VM->GetEngine();
				if (!Engine)
					return false;

				Engine->RegisterGlobalFunction("float fp_from_ieee(uint)", asFUNCTIONPR(Math::FpFromIEEE, (uint32_t), float), asCALL_CDECL);
				Engine->RegisterGlobalFunction("uint fp_to_ieee(float)", asFUNCTIONPR(Math::FpToIEEE, (float), uint32_t), asCALL_CDECL);
				Engine->RegisterGlobalFunction("double fp_from_ieee(uint64)", asFUNCTIONPR(Math::FpFromIEEE, (as_uint64_t), double), asCALL_CDECL);
				Engine->RegisterGlobalFunction("uint64 fp_to_ieee(double)", asFUNCTIONPR(Math::FpToIEEE, (double), as_uint64_t), asCALL_CDECL);
				Engine->RegisterGlobalFunction("bool close_to(float, float, float = 0.00001f)", asFUNCTIONPR(Math::CloseTo, (float, float, float), bool), asCALL_CDECL);
				Engine->RegisterGlobalFunction("bool close_to(double, double, double = 0.0000000001)", asFUNCTIONPR(Math::CloseTo, (double, double, double), bool), asCALL_CDECL);
				Engine->RegisterGlobalFunction("float rad2deg()", asFUNCTIONPR(Compute::Mathf::Rad2Deg, (), float), asCALL_CDECL);
				Engine->RegisterGlobalFunction("float deg2rad()", asFUNCTIONPR(Compute::Mathf::Deg2Rad, (), float), asCALL_CDECL);
				Engine->RegisterGlobalFunction("float pi_value()", asFUNCTIONPR(Compute::Mathf::Pi, (), float), asCALL_CDECL);
				Engine->RegisterGlobalFunction("float clamp(float, float, float)", asFUNCTIONPR(Compute::Mathf::Clamp, (float, float, float), float), asCALL_CDECL);
				Engine->RegisterGlobalFunction("float acotan(float)", asFUNCTIONPR(Compute::Mathf::Acotan, (float), float), asCALL_CDECL);
				Engine->RegisterGlobalFunction("float cotan(float)", asFUNCTIONPR(Compute::Mathf::Cotan, (float), float), asCALL_CDECL);
				Engine->RegisterGlobalFunction("float minf(float, float)", asFUNCTIONPR(Compute::Mathf::Min, (float, float), float), asCALL_CDECL);
				Engine->RegisterGlobalFunction("float maxf(float, float)", asFUNCTIONPR(Compute::Mathf::Max, (float, float), float), asCALL_CDECL);
				Engine->RegisterGlobalFunction("float saturate(float)", asFUNCTIONPR(Compute::Mathf::Saturate, (float), float), asCALL_CDECL);
				Engine->RegisterGlobalFunction("float lerp(float, float, float)", asFUNCTIONPR(Compute::Mathf::Lerp, (float, float, float), float), asCALL_CDECL);
				Engine->RegisterGlobalFunction("float strong_lerp(float, float, float)", asFUNCTIONPR(Compute::Mathf::StrongLerp, (float, float, float), float), asCALL_CDECL);
				Engine->RegisterGlobalFunction("float angle_saturate(float)", asFUNCTIONPR(Compute::Mathf::SaturateAngle, (float), float), asCALL_CDECL);
				Engine->RegisterGlobalFunction("float angle_distance(float, float)", asFUNCTIONPR(Compute::Mathf::AngleDistance, (float, float), float), asCALL_CDECL);
				Engine->RegisterGlobalFunction("float angle_lerp(float, float, float)", asFUNCTIONPR(Compute::Mathf::AngluarLerp, (float, float, float), float), asCALL_CDECL);
				Engine->RegisterGlobalFunction("float randomf()", asFUNCTIONPR(Compute::Mathf::Random, (), float), asCALL_CDECL);
				Engine->RegisterGlobalFunction("float randomf_mag()", asFUNCTIONPR(Compute::Mathf::RandomMag, (), float), asCALL_CDECL);
				Engine->RegisterGlobalFunction("float randomf_range()", asFUNCTIONPR(Compute::Mathf::Random, (float, float), float), asCALL_CDECL);
				Engine->RegisterGlobalFunction("float mapf(float, float, float, float, float)", asFUNCTIONPR(Compute::Mathf::Map, (float, float, float, float, float), float), asCALL_CDECL);
				Engine->RegisterGlobalFunction("float cos(float)", asFUNCTIONPR(Compute::Mathf::Cos, (float), float), asCALL_CDECL);
				Engine->RegisterGlobalFunction("float sin(float)", asFUNCTIONPR(Compute::Mathf::Sin, (float), float), asCALL_CDECL);
				Engine->RegisterGlobalFunction("float tan(float)", asFUNCTIONPR(Compute::Mathf::Tan, (float), float), asCALL_CDECL);
				Engine->RegisterGlobalFunction("float acos(float)", asFUNCTIONPR(Compute::Mathf::Acos, (float), float), asCALL_CDECL);
				Engine->RegisterGlobalFunction("float asin(float)", asFUNCTIONPR(Compute::Mathf::Asin, (float), float), asCALL_CDECL);
				Engine->RegisterGlobalFunction("float atan(float)", asFUNCTIONPR(Compute::Mathf::Atan, (float), float), asCALL_CDECL);
				Engine->RegisterGlobalFunction("float atan2(float, float)", asFUNCTIONPR(Compute::Mathf::Atan2, (float, float), float), asCALL_CDECL);
				Engine->RegisterGlobalFunction("float exp(float)", asFUNCTIONPR(Compute::Mathf::Exp, (float), float), asCALL_CDECL);
				Engine->RegisterGlobalFunction("float log(float)", asFUNCTIONPR(Compute::Mathf::Log, (float), float), asCALL_CDECL);
				Engine->RegisterGlobalFunction("float log2(float)", asFUNCTIONPR(Compute::Mathf::Log2, (float), float), asCALL_CDECL);
				Engine->RegisterGlobalFunction("float log10(float)", asFUNCTIONPR(Compute::Mathf::Log10, (float), float), asCALL_CDECL);
				Engine->RegisterGlobalFunction("float pow(float, float)", asFUNCTIONPR(Compute::Mathf::Pow, (float, float), float), asCALL_CDECL);
				Engine->RegisterGlobalFunction("float sqrt(float)", asFUNCTIONPR(Compute::Mathf::Sqrt, (float), float), asCALL_CDECL);
				Engine->RegisterGlobalFunction("float ceil(float)", asFUNCTIONPR(Compute::Mathf::Ceil, (float), float), asCALL_CDECL);
				Engine->RegisterGlobalFunction("float abs(float)", asFUNCTIONPR(Compute::Mathf::Abs, (float), float), asCALL_CDECL);
				Engine->RegisterGlobalFunction("float floor(float)", asFUNCTIONPR(Compute::Mathf::Floor, (float), float), asCALL_CDECL);
				
				return true;
			}
			bool Registry::LoadString(VirtualMachine* VM)
			{
				asIScriptEngine* Engine = VM->GetEngine();
				if (!Engine)
					return false;

				Engine->RegisterObjectType("string", sizeof(Core::String), asOBJ_VALUE | asGetTypeTraits<Core::String>());
				Engine->RegisterStringFactory("string", StringFactory::Get());
				Engine->RegisterObjectBehaviour("string", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(String::Construct), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectBehaviour("string", asBEHAVE_CONSTRUCT, "void f(const string &in)", asFUNCTION(String::CopyConstruct), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectBehaviour("string", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(String::Destruct), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("string", "string &opAssign(const string &in)", asMETHODPR(Core::String, operator =, (const Core::String&), Core::String&), asCALL_THISCALL);
				Engine->RegisterObjectMethod("string", "string &opAddAssign(const string &in)", asFUNCTION(String::AddAssignTo), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("string", "bool opEquals(const string &in) const", asFUNCTIONPR(String::Equals, (const Core::String&, const Core::String&), bool), asCALL_CDECL_OBJFIRST);
				Engine->RegisterObjectMethod("string", "int opCmp(const string &in) const", asFUNCTION(String::Cmp), asCALL_CDECL_OBJFIRST);
				Engine->RegisterObjectMethod("string", "string opAdd(const string &in) const", asFUNCTIONPR(std::operator +, (const Core::String&, const Core::String&), Core::String), asCALL_CDECL_OBJFIRST);
				Engine->RegisterObjectMethod("string", "usize size() const", asFUNCTION(String::Length), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("string", "void resize(usize)", asFUNCTION(String::Resize), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("string", "bool empty() const", asFUNCTION(String::IsEmpty), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("string", "uint8 &opIndex(usize)", asFUNCTION(String::CharAt), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("string", "const uint8 &opIndex(usize) const", asFUNCTION(String::CharAt), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("string", "string &opAssign(double)", asFUNCTION(String::AssignDoubleTo), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("string", "string &opAddAssign(double)", asFUNCTION(String::AddAssignDoubleTo), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("string", "string opAdd(double) const", asFUNCTION(String::AddDouble1), asCALL_CDECL_OBJFIRST);
				Engine->RegisterObjectMethod("string", "string opAdd_r(double) const", asFUNCTION(String::AddDouble2), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("string", "string &opAssign(float)", asFUNCTION(String::AssignFloatTo), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("string", "string &opAddAssign(float)", asFUNCTION(String::AddAssignFloatTo), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("string", "string opAdd(float) const", asFUNCTION(String::AddFloat1), asCALL_CDECL_OBJFIRST);
				Engine->RegisterObjectMethod("string", "string opAdd_r(float) const", asFUNCTION(String::AddFloat2), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("string", "string &opAssign(int64)", asFUNCTION(String::AssignInt64To), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("string", "string &opAddAssign(int64)", asFUNCTION(String::AddAssignInt64To), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("string", "string opAdd(int64) const", asFUNCTION(String::AddInt641), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("string", "string opAdd_r(int64) const", asFUNCTION(String::AddInt642), asCALL_CDECL_OBJFIRST);
				Engine->RegisterObjectMethod("string", "string &opAssign(uint64)", asFUNCTION(String::AssignUInt64To), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("string", "string &opAddAssign(uint64)", asFUNCTION(String::AddAssignUInt64To), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("string", "string opAdd(uint64) const", asFUNCTION(String::AddUInt641), asCALL_CDECL_OBJFIRST);
				Engine->RegisterObjectMethod("string", "string opAdd_r(uint64) const", asFUNCTION(String::AddUInt642), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("string", "string &opAssign(bool)", asFUNCTION(String::AssignBoolTo), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("string", "string &opAddAssign(bool)", asFUNCTION(String::AddAssignBoolTo), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("string", "string opAdd(bool) const", asFUNCTION(String::AddBool1), asCALL_CDECL_OBJFIRST);
				Engine->RegisterObjectMethod("string", "string opAdd_r(bool) const", asFUNCTION(String::AddBool2), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("string", "string needle(usize = 0, int = -1) const", asFUNCTION(String::Sub), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("string", "int find_first(const string &in, usize = 0) const", asFUNCTION(String::FindFirst), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("string", "int find_first_of(const string &in, usize = 0) const", asFUNCTION(String::FindFirstOf), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("string", "int find_first_not_of(const string &in, usize = 0) const", asFUNCTION(String::FindFirstNotOf), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("string", "int find_last(const string &in, int = -1) const", asFUNCTION(String::FindLast), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("string", "int find_last_of(const string &in, int = -1) const", asFUNCTION(String::FindLastOf), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("string", "int find_last_not_of(const string &in, int = -1) const", asFUNCTION(String::FindLastNotOf), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("string", "void insert(usize, const string &in)", asFUNCTION(String::Insert), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("string", "void erase(usize, int Count = -1)", asFUNCTION(String::Erase), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("string", "string replace(const string &in, const string &in, usize = 0)", asFUNCTION(String::Replace), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("string", "array<string>@ split(const string &in) const", asFUNCTION(String::Split), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("string", "string to_lower() const", asFUNCTION(String::ToLower), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("string", "string to_upper() const", asFUNCTION(String::ToUpper), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("string", "string reverse() const", asFUNCTION(String::Reverse), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("string", "uptr@ get_ptr() const", asFUNCTION(String::ToPtr), asCALL_CDECL_OBJLAST);
				Engine->RegisterGlobalFunction("int64 to_int(const string &in, usize = 10, usize &out = 0)", asFUNCTION(String::IntStore), asCALL_CDECL);
				Engine->RegisterGlobalFunction("uint64 to_uint(const string &in, usize = 10, usize &out = 0)", asFUNCTION(String::UIntStore), asCALL_CDECL);
				Engine->RegisterGlobalFunction("double to_float(const string &in, usize &out = 0)", asFUNCTION(String::FloatStore), asCALL_CDECL);
				Engine->RegisterGlobalFunction("uint8 to_char(const string &in)", asFUNCTION(String::ToChar), asCALL_CDECL);
				Engine->RegisterGlobalFunction("string to_string(const array<string> &in, const string &in)", asFUNCTION(String::Join), asCALL_CDECL);
				Engine->RegisterGlobalFunction("string to_string(int8)", asFUNCTION(String::ToInt8), asCALL_CDECL);
				Engine->RegisterGlobalFunction("string to_string(int16)", asFUNCTION(String::ToInt16), asCALL_CDECL);
				Engine->RegisterGlobalFunction("string to_string(int)", asFUNCTION(String::ToInt), asCALL_CDECL);
				Engine->RegisterGlobalFunction("string to_string(int64)", asFUNCTION(String::ToInt64), asCALL_CDECL);
				Engine->RegisterGlobalFunction("string to_string(uint8)", asFUNCTION(String::ToUInt8), asCALL_CDECL);
				Engine->RegisterGlobalFunction("string to_string(uint16)", asFUNCTION(String::ToUInt16), asCALL_CDECL);
				Engine->RegisterGlobalFunction("string to_string(uint)", asFUNCTION(String::ToUInt), asCALL_CDECL);
				Engine->RegisterGlobalFunction("string to_string(uint64)", asFUNCTION(String::ToUInt64), asCALL_CDECL);
				Engine->RegisterGlobalFunction("string to_string(float)", asFUNCTION(String::ToFloat), asCALL_CDECL);
				Engine->RegisterGlobalFunction("string to_string(double)", asFUNCTION(String::ToDouble), asCALL_CDECL);
				Engine->RegisterGlobalFunction("string to_string(uptr@)", asFUNCTION(String::ToPointer), asCALL_CDECL);

				return true;
			}
			bool Registry::LoadException(VirtualMachine* VM)
			{
				VI_ASSERT(VM != nullptr && VM->GetEngine() != nullptr, false, "manager should be set");

				asIScriptEngine* Engine = VM->GetEngine();
				TypeClass VExceptionData = VM->SetStructTrivial<Exception::Pointer>("exception_ptr");
				VExceptionData.SetProperty("string type", &Exception::Pointer::Type);
				VExceptionData.SetProperty("string message", &Exception::Pointer::Message);
				VExceptionData.SetProperty("string origin", &Exception::Pointer::Origin);
				VExceptionData.SetConstructor<Exception::Pointer>("void f()");
				VExceptionData.SetConstructor<Exception::Pointer, const Core::String&>("void f(const string&in)");
				VExceptionData.SetConstructor<Exception::Pointer, const Core::String&, const Core::String&>("void f(const string&in, const string&in)");
				VExceptionData.SetMethod("const string& get_type() const", &Exception::Pointer::GetType);
				VExceptionData.SetMethod("const string& get_message() const", &Exception::Pointer::GetMessage);
				VExceptionData.SetMethod("string what() const", &Exception::Pointer::What);
				VExceptionData.SetMethod("bool empty() const", &Exception::Pointer::Empty);

				VM->SetCodeGenerator("std/exception", &Exception::GeneratorCallback);
				VM->BeginNamespace("exception");
				Engine->RegisterGlobalFunction("void throw(const exception_ptr&in)", asFUNCTION(Exception::Throw), asCALL_CDECL);
				Engine->RegisterGlobalFunction("void rethrow()", asFUNCTION(Exception::Rethrow), asCALL_CDECL);
				Engine->RegisterGlobalFunction("exception_ptr unwrap()", asFUNCTION(Exception::GetException), asCALL_CDECL);
				VM->EndNamespace();

				return true;
			}
			bool Registry::LoadMutex(VirtualMachine* VM)
			{
#ifdef VI_HAS_BINDINGS
				VI_ASSERT(VM != nullptr && VM->GetEngine() != nullptr, false, "manager should be set");
				asIScriptEngine* Engine = VM->GetEngine();
				Engine->RegisterObjectType("mutex", sizeof(Mutex), asOBJ_REF);
				Engine->RegisterObjectBehaviour("mutex", asBEHAVE_FACTORY, "mutex@ f()", asFUNCTION(Mutex::Factory), asCALL_CDECL);
				Engine->RegisterObjectBehaviour("mutex", asBEHAVE_ADDREF, "void f()", asMETHOD(Mutex, AddRef), asCALL_THISCALL);
				Engine->RegisterObjectBehaviour("mutex", asBEHAVE_RELEASE, "void f()", asMETHOD(Mutex, Release), asCALL_THISCALL);
				Engine->RegisterObjectMethod("mutex", "bool try_lock()", asMETHOD(Mutex, TryLock), asCALL_THISCALL);
				Engine->RegisterObjectMethod("mutex", "void lock()", asMETHOD(Mutex, Lock), asCALL_THISCALL);
				Engine->RegisterObjectMethod("mutex", "void unlock()", asMETHOD(Mutex, Unlock), asCALL_THISCALL);
				return true;
#else
				VI_ASSERT(false, false, "<mutex> is not loaded");
				return false;
#endif
			}
			bool Registry::LoadThread(VirtualMachine* VM)
			{
#ifdef VI_HAS_BINDINGS
				VI_ASSERT(VM != nullptr && VM->GetEngine() != nullptr, false, "manager should be set");
				asIScriptEngine* Engine = VM->GetEngine();
				Engine->RegisterObjectType("thread", 0, asOBJ_REF | asOBJ_GC);
				Engine->RegisterFuncdef("void thread_event(thread@+)");
				Engine->RegisterObjectBehaviour("thread", asBEHAVE_FACTORY, "thread@ f(thread_event@)", asFUNCTION(Thread::Create), asCALL_GENERIC);
				Engine->RegisterObjectBehaviour("thread", asBEHAVE_ADDREF, "void f()", asMETHOD(Thread, AddRef), asCALL_THISCALL);
				Engine->RegisterObjectBehaviour("thread", asBEHAVE_RELEASE, "void f()", asMETHOD(Thread, Release), asCALL_THISCALL);
				Engine->RegisterObjectBehaviour("thread", asBEHAVE_SETGCFLAG, "void f()", asMETHOD(Thread, SetGCFlag), asCALL_THISCALL);
				Engine->RegisterObjectBehaviour("thread", asBEHAVE_GETGCFLAG, "bool f()", asMETHOD(Thread, GetGCFlag), asCALL_THISCALL);
				Engine->RegisterObjectBehaviour("thread", asBEHAVE_GETREFCOUNT, "int f()", asMETHOD(Thread, GetRefCount), asCALL_THISCALL);
				Engine->RegisterObjectBehaviour("thread", asBEHAVE_ENUMREFS, "void f(int&in)", asMETHOD(Thread, EnumReferences), asCALL_THISCALL);
				Engine->RegisterObjectBehaviour("thread", asBEHAVE_RELEASEREFS, "void f(int&in)", asMETHOD(Thread, ReleaseReferences), asCALL_THISCALL);
				Engine->RegisterObjectMethod("thread", "bool is_active()", asMETHOD(Thread, IsActive), asCALL_THISCALL);
				Engine->RegisterObjectMethod("thread", "bool invoke()", asMETHOD(Thread, Start), asCALL_THISCALL);
				Engine->RegisterObjectMethod("thread", "void suspend()", asMETHOD(Thread, Suspend), asCALL_THISCALL);
				Engine->RegisterObjectMethod("thread", "void resume()", asMETHOD(Thread, Resume), asCALL_THISCALL);
				Engine->RegisterObjectMethod("thread", "void push(const ?&in)", asMETHOD(Thread, Push), asCALL_THISCALL);
				Engine->RegisterObjectMethod("thread", "bool pop(?&out)", asMETHODPR(Thread, Pop, (void*, int), bool), asCALL_THISCALL);
				Engine->RegisterObjectMethod("thread", "bool pop(?&out, uint64)", asMETHODPR(Thread, Pop, (void*, int, uint64_t), bool), asCALL_THISCALL);
				Engine->RegisterObjectMethod("thread", "int join(uint64)", asMETHODPR(Thread, Join, (uint64_t), int), asCALL_THISCALL);
				Engine->RegisterObjectMethod("thread", "int join()", asMETHODPR(Thread, Join, (), int), asCALL_THISCALL);
				Engine->RegisterObjectMethod("thread", "string get_id() const", asMETHODPR(Thread, GetId, () const, Core::String), asCALL_THISCALL);

				VM->BeginNamespace("this_thread");
				Engine->RegisterGlobalFunction("thread@+ get_routine()", asFUNCTION(Thread::GetThread), asCALL_CDECL);
				Engine->RegisterGlobalFunction("string get_id()", asFUNCTION(Thread::GetThreadId), asCALL_CDECL);
				Engine->RegisterGlobalFunction("void sleep(uint64)", asFUNCTION(Thread::ThreadSleep), asCALL_CDECL);
				Engine->RegisterGlobalFunction("void suspend()", asFUNCTION(Thread::ThreadSuspend), asCALL_CDECL);
				VM->EndNamespace();
				return true;
#else
				VI_ASSERT(false, false, "<thread> is not loaded");
				return false;
#endif
			}
			bool Registry::LoadRandom(VirtualMachine* VM)
			{
				VI_ASSERT(VM != nullptr && VM->GetEngine() != nullptr, false, "manager should be set");

				VM->BeginNamespace("random");
				VM->SetFunction("string bytes(uint64)", &Random::Getb);
				VM->SetFunction("double betweend(double, double)", &Random::Betweend);
				VM->SetFunction("double magd()", &Random::Magd);
				VM->SetFunction("double getd()", &Random::Getd);
				VM->SetFunction("float betweenf(float, float)", &Random::Betweenf);
				VM->SetFunction("float magf()", &Random::Magf);
				VM->SetFunction("float getf()", &Random::Getf);
				VM->SetFunction("uint64 betweeni(uint64, uint64)", &Random::Betweeni);
				VM->EndNamespace();

				return true;
			}
			bool Registry::LoadPromise(VirtualMachine* VM)
			{
				asIScriptEngine* Engine = VM->GetEngine();
				VI_ASSERT(Engine != nullptr, false, "manager should be set");
				Engine->RegisterObjectType("promise<class T>", 0, asOBJ_REF | asOBJ_GC | asOBJ_TEMPLATE);
				Engine->RegisterObjectBehaviour("promise<T>", asBEHAVE_FACTORY, "promise<T>@ f(?&in)", asFUNCTION(Promise::CreateFactory), asCALL_CDECL);
				Engine->RegisterObjectBehaviour("promise<T>", asBEHAVE_TEMPLATE_CALLBACK, "bool f(int&in, bool&out)", asFUNCTION(Promise::TemplateCallback), asCALL_CDECL);
				Engine->RegisterObjectBehaviour("promise<T>", asBEHAVE_ADDREF, "void f()", asMETHOD(Promise, AddRef), asCALL_THISCALL);
				Engine->RegisterObjectBehaviour("promise<T>", asBEHAVE_RELEASE, "void f()", asMETHOD(Promise, Release), asCALL_THISCALL);
				Engine->RegisterObjectBehaviour("promise<T>", asBEHAVE_SETGCFLAG, "void f()", asMETHOD(Promise, SetFlag), asCALL_THISCALL);
				Engine->RegisterObjectBehaviour("promise<T>", asBEHAVE_GETGCFLAG, "bool f()", asMETHOD(Promise, GetFlag), asCALL_THISCALL);
				Engine->RegisterObjectBehaviour("promise<T>", asBEHAVE_GETREFCOUNT, "int f()", asMETHOD(Promise, GetRefCount), asCALL_THISCALL);
				Engine->RegisterObjectBehaviour("promise<T>", asBEHAVE_ENUMREFS, "void f(int&in)", asMETHOD(Promise, EnumReferences), asCALL_THISCALL);
				Engine->RegisterObjectBehaviour("promise<T>", asBEHAVE_RELEASEREFS, "void f(int&in)", asMETHOD(Promise, ReleaseReferences), asCALL_THISCALL);
				Engine->RegisterObjectMethod("promise<T>", "void wrap(?&in)", asMETHODPR(Promise, Store, (void*, int), void), asCALL_THISCALL);
				Engine->RegisterObjectMethod("promise<T>", "T& unwrap()", asMETHODPR(Promise, Retrieve, (), void*), asCALL_THISCALL);
				Engine->RegisterObjectMethod("promise<T>", "promise<T>@+ yield()", asMETHOD(Promise, YieldIf), asCALL_THISCALL);
				Engine->RegisterObjectMethod("promise<T>", "bool pending()", asMETHOD(Promise, IsPending), asCALL_THISCALL);
				Engine->RegisterObjectType("promise_v", 0, asOBJ_REF | asOBJ_GC);
				Engine->RegisterObjectBehaviour("promise_v", asBEHAVE_FACTORY, "promise_v@ f()", asFUNCTION(Promise::CreateFactoryVoid), asCALL_CDECL);
				Engine->RegisterObjectBehaviour("promise_v", asBEHAVE_ADDREF, "void f()", asMETHOD(Promise, AddRef), asCALL_THISCALL);
				Engine->RegisterObjectBehaviour("promise_v", asBEHAVE_RELEASE, "void f()", asMETHOD(Promise, Release), asCALL_THISCALL);
				Engine->RegisterObjectBehaviour("promise_v", asBEHAVE_SETGCFLAG, "void f()", asMETHOD(Promise, SetFlag), asCALL_THISCALL);
				Engine->RegisterObjectBehaviour("promise_v", asBEHAVE_GETGCFLAG, "bool f()", asMETHOD(Promise, GetFlag), asCALL_THISCALL);
				Engine->RegisterObjectBehaviour("promise_v", asBEHAVE_GETREFCOUNT, "int f()", asMETHOD(Promise, GetRefCount), asCALL_THISCALL);
				Engine->RegisterObjectBehaviour("promise_v", asBEHAVE_ENUMREFS, "void f(int&in)", asMETHOD(Promise, EnumReferences), asCALL_THISCALL);
				Engine->RegisterObjectBehaviour("promise_v", asBEHAVE_RELEASEREFS, "void f(int&in)", asMETHOD(Promise, ReleaseReferences), asCALL_THISCALL);
				Engine->RegisterObjectMethod("promise_v", "void wrap()", asMETHODPR(Promise, StoreVoid, (), void), asCALL_THISCALL);
				Engine->RegisterObjectMethod("promise_v", "void unwrap()", asMETHODPR(Promise, RetrieveVoid, (), void), asCALL_THISCALL);
				Engine->RegisterObjectMethod("promise_v", "promise_v@+ yield()", asMETHOD(Promise, YieldIf), asCALL_THISCALL);
				Engine->RegisterObjectMethod("promise_v", "bool pending()", asMETHOD(Promise, IsPending), asCALL_THISCALL);
				VM->SetCodeGenerator("std/promise", &Promise::GeneratorCallback);

				return true;
			}
			bool Registry::LoadPromiseAsync(VirtualMachine* VM)
			{
				asIScriptEngine* Engine = VM->GetEngine();
				if (Engine->GetTypeInfoByDecl("promise<bool>@") != nullptr)
					return false;

				VI_ASSERT(Engine != nullptr, false, "manager should be set");
				Engine->RegisterObjectType("promise<class T>", 0, asOBJ_REF | asOBJ_GC | asOBJ_TEMPLATE);
				Engine->RegisterObjectBehaviour("promise<T>", asBEHAVE_FACTORY, "promise<T>@ f(?&in)", asFUNCTION(Promise::CreateFactory), asCALL_CDECL);
				Engine->RegisterObjectBehaviour("promise<T>", asBEHAVE_TEMPLATE_CALLBACK, "bool f(int&in, bool&out)", asFUNCTION(Promise::TemplateCallback), asCALL_CDECL);
				Engine->RegisterObjectBehaviour("promise<T>", asBEHAVE_ADDREF, "void f()", asMETHOD(Promise, AddRef), asCALL_THISCALL);
				Engine->RegisterObjectBehaviour("promise<T>", asBEHAVE_RELEASE, "void f()", asMETHOD(Promise, Release), asCALL_THISCALL);
				Engine->RegisterObjectBehaviour("promise<T>", asBEHAVE_SETGCFLAG, "void f()", asMETHOD(Promise, SetFlag), asCALL_THISCALL);
				Engine->RegisterObjectBehaviour("promise<T>", asBEHAVE_GETGCFLAG, "bool f()", asMETHOD(Promise, GetFlag), asCALL_THISCALL);
				Engine->RegisterObjectBehaviour("promise<T>", asBEHAVE_GETREFCOUNT, "int f()", asMETHOD(Promise, GetRefCount), asCALL_THISCALL);
				Engine->RegisterObjectBehaviour("promise<T>", asBEHAVE_ENUMREFS, "void f(int&in)", asMETHOD(Promise, EnumReferences), asCALL_THISCALL);
				Engine->RegisterObjectBehaviour("promise<T>", asBEHAVE_RELEASEREFS, "void f(int&in)", asMETHOD(Promise, ReleaseReferences), asCALL_THISCALL);
				Engine->RegisterFuncdef("void promise<T>::when_callback(T&in)");
				Engine->RegisterObjectMethod("promise<T>", "void when(when_callback@+)", asMETHOD(Promise, When), asCALL_THISCALL);
				Engine->RegisterObjectMethod("promise<T>", "void wrap(?&in)", asMETHODPR(Promise, Store, (void*, int), void), asCALL_THISCALL);
				Engine->RegisterObjectMethod("promise<T>", "T& unwrap()", asMETHODPR(Promise, Retrieve, (), void*), asCALL_THISCALL);
				Engine->RegisterObjectMethod("promise<T>", "promise<T>@+ yield()", asMETHOD(Promise, YieldIf), asCALL_THISCALL);
				Engine->RegisterObjectMethod("promise<T>", "bool pending()", asMETHOD(Promise, IsPending), asCALL_THISCALL);
				Engine->RegisterObjectType("promise_v", 0, asOBJ_REF | asOBJ_GC);
				Engine->RegisterObjectBehaviour("promise_v", asBEHAVE_FACTORY, "promise_v@ f()", asFUNCTION(Promise::CreateFactoryVoid), asCALL_CDECL);
				Engine->RegisterObjectBehaviour("promise_v", asBEHAVE_ADDREF, "void f()", asMETHOD(Promise, AddRef), asCALL_THISCALL);
				Engine->RegisterObjectBehaviour("promise_v", asBEHAVE_RELEASE, "void f()", asMETHOD(Promise, Release), asCALL_THISCALL);
				Engine->RegisterObjectBehaviour("promise_v", asBEHAVE_SETGCFLAG, "void f()", asMETHOD(Promise, SetFlag), asCALL_THISCALL);
				Engine->RegisterObjectBehaviour("promise_v", asBEHAVE_GETGCFLAG, "bool f()", asMETHOD(Promise, GetFlag), asCALL_THISCALL);
				Engine->RegisterObjectBehaviour("promise_v", asBEHAVE_GETREFCOUNT, "int f()", asMETHOD(Promise, GetRefCount), asCALL_THISCALL);
				Engine->RegisterObjectBehaviour("promise_v", asBEHAVE_ENUMREFS, "void f(int&in)", asMETHOD(Promise, EnumReferences), asCALL_THISCALL);
				Engine->RegisterObjectBehaviour("promise_v", asBEHAVE_RELEASEREFS, "void f(int&in)", asMETHOD(Promise, ReleaseReferences), asCALL_THISCALL);
				Engine->RegisterFuncdef("void promise_v::when_callback()");
				Engine->RegisterObjectMethod("promise_v", "void when(when_callback@+)", asMETHOD(Promise, When), asCALL_THISCALL);
				Engine->RegisterObjectMethod("promise_v", "void wrap()", asMETHODPR(Promise, StoreVoid, (), void), asCALL_THISCALL);
				Engine->RegisterObjectMethod("promise_v", "void unwrap()", asMETHODPR(Promise, RetrieveVoid, (), void), asCALL_THISCALL);
				Engine->RegisterObjectMethod("promise_v", "promise_v@+ yield()", asMETHOD(Promise, YieldIf), asCALL_THISCALL);
				Engine->RegisterObjectMethod("promise_v", "bool pending()", asMETHOD(Promise, IsPending), asCALL_THISCALL);
				VM->SetCodeGenerator("std/promise/async", &Promise::GeneratorCallback);

				return true;
			}
			bool Registry::LoadFormat(VirtualMachine* Engine)
			{
#ifdef VI_HAS_BINDINGS
				VI_ASSERT(Engine != nullptr, false, "manager should be set");
				RefClass VFormat = Engine->SetClass<Format>("format", false);
				VFormat.SetConstructor<Format>("format@ f()");
				VFormat.SetConstructorList<Format>("format@ f(int &in) {repeat ?}");
				VFormat.SetMethodStatic("string to_json(const ? &in)", &Format::JSON);

				return true;
#else
				VI_ASSERT(false, false, "<format> is not loaded");
				return false;
#endif
			}
			bool Registry::LoadDecimal(VirtualMachine* Engine)
			{
				VI_ASSERT(Engine != nullptr, false, "manager should be set");

				TypeClass VDecimal = Engine->SetStructTrivial<Core::Decimal>("decimal");
				VDecimal.SetConstructor<Core::Decimal>("void f()");
				VDecimal.SetConstructor<Core::Decimal, int32_t>("void f(int)");
				VDecimal.SetConstructor<Core::Decimal, int32_t>("void f(uint)");
				VDecimal.SetConstructor<Core::Decimal, int64_t>("void f(int64)");
				VDecimal.SetConstructor<Core::Decimal, uint64_t>("void f(uint64)");
				VDecimal.SetConstructor<Core::Decimal, float>("void f(float)");
				VDecimal.SetConstructor<Core::Decimal, double>("void f(double)");
				VDecimal.SetConstructor<Core::Decimal, const Core::String&>("void f(const string &in)");
				VDecimal.SetConstructor<Core::Decimal, const Core::Decimal&>("void f(const decimal &in)");
				VDecimal.SetMethod("decimal& truncate(int)", &Core::Decimal::Truncate);
				VDecimal.SetMethod("decimal& round(int)", &Core::Decimal::Round);
				VDecimal.SetMethod("decimal& trim()", &Core::Decimal::Trim);
				VDecimal.SetMethod("decimal& unlead()", &Core::Decimal::Unlead);
				VDecimal.SetMethod("decimal& untrail()", &Core::Decimal::Untrail);
				VDecimal.SetMethod("bool is_nan() const", &Core::Decimal::IsNaN);
				VDecimal.SetMethod("double to_double() const", &Core::Decimal::ToDouble);
				VDecimal.SetMethod("int64 to_int64() const", &Core::Decimal::ToInt64);
				VDecimal.SetMethod("string to_string() const", &Core::Decimal::ToString);
				VDecimal.SetMethod("string exp() const", &Core::Decimal::Exp);
				VDecimal.SetMethod("string decimals() const", &Core::Decimal::Decimals);
				VDecimal.SetMethod("string ints() const", &Core::Decimal::Ints);
				VDecimal.SetMethod("string size() const", &Core::Decimal::Size);
				VDecimal.SetOperatorEx(Operators::Neg, (uint32_t)Position::Const, "decimal", "", &DecimalNegate);
				VDecimal.SetOperatorEx(Operators::MulAssign, (uint32_t)Position::Left, "decimal&", "const decimal &in", &DecimalMulEq);
				VDecimal.SetOperatorEx(Operators::DivAssign, (uint32_t)Position::Left, "decimal&", "const decimal &in", &DecimalDivEq);
				VDecimal.SetOperatorEx(Operators::AddAssign, (uint32_t)Position::Left, "decimal&", "const decimal &in", &DecimalAddEq);
				VDecimal.SetOperatorEx(Operators::SubAssign, (uint32_t)Position::Left, "decimal&", "const decimal &in", &DecimalSubEq);
				VDecimal.SetOperatorEx(Operators::PostInc, (uint32_t)Position::Left, "decimal&", "", &DecimalPP);
				VDecimal.SetOperatorEx(Operators::PostDec, (uint32_t)Position::Left, "decimal&", "", &DecimalMM);
				VDecimal.SetOperatorEx(Operators::Equals, (uint32_t)Position::Const, "bool", "const decimal &in", &DecimalEq);
				VDecimal.SetOperatorEx(Operators::Cmp, (uint32_t)Position::Const, "int", "const decimal &in", &DecimalCmp);
				VDecimal.SetOperatorEx(Operators::Add, (uint32_t)Position::Const, "decimal", "const decimal &in", &DecimalAdd);
				VDecimal.SetOperatorEx(Operators::Sub, (uint32_t)Position::Const, "decimal", "const decimal &in", &DecimalSub);
				VDecimal.SetOperatorEx(Operators::Mul, (uint32_t)Position::Const, "decimal", "const decimal &in", &DecimalMul);
				VDecimal.SetOperatorEx(Operators::Div, (uint32_t)Position::Const, "decimal", "const decimal &in", &DecimalDiv);
				VDecimal.SetOperatorEx(Operators::Mod, (uint32_t)Position::Const, "decimal", "const decimal &in", &DecimalPer);
				VDecimal.SetMethodStatic("decimal nan()", &Core::Decimal::NaN);

				return true;
			}
			bool Registry::LoadVariant(VirtualMachine* Engine)
			{
				VI_ASSERT(Engine != nullptr, false, "manager should be set");
				Enumeration VVarType = Engine->SetEnum("var_type");
				VVarType.SetValue("null_t", (int)Core::VarType::Null);
				VVarType.SetValue("undefined_t", (int)Core::VarType::Undefined);
				VVarType.SetValue("object_t", (int)Core::VarType::Object);
				VVarType.SetValue("array_t", (int)Core::VarType::Array);
				VVarType.SetValue("string_t", (int)Core::VarType::String);
				VVarType.SetValue("binary_t", (int)Core::VarType::Binary);
				VVarType.SetValue("integer_t", (int)Core::VarType::Integer);
				VVarType.SetValue("number_t", (int)Core::VarType::Number);
				VVarType.SetValue("decimal_t", (int)Core::VarType::Decimal);
				VVarType.SetValue("boolean_t", (int)Core::VarType::Boolean);

				TypeClass VVariant = Engine->SetStructTrivial<Core::Variant>("variant");
				VVariant.SetConstructor<Core::Variant>("void f()");
				VVariant.SetConstructor<Core::Variant, const Core::Variant&>("void f(const variant &in)");
				VVariant.SetMethod("bool deserialize(const string &in, bool = false)", &Core::Variant::Deserialize);
				VVariant.SetMethod("string serialize() const", &Core::Variant::Serialize);
				VVariant.SetMethod("decimal to_decimal() const", &Core::Variant::GetDecimal);
				VVariant.SetMethod("string to_string() const", &Core::Variant::GetBlob);
				VVariant.SetMethod("uptr@ to_pointer() const", &Core::Variant::GetPointer);
				VVariant.SetMethod("int64 to_integer() const", &Core::Variant::GetInteger);
				VVariant.SetMethod("double to_number() const", &Core::Variant::GetNumber);
				VVariant.SetMethod("bool to_boolean() const", &Core::Variant::GetBoolean);
				VVariant.SetMethod("var_type get_type() const", &Core::Variant::GetType);
				VVariant.SetMethod("bool is_object() const", &Core::Variant::IsObject);
				VVariant.SetMethod("bool empty() const", &Core::Variant::IsEmpty);
				VVariant.SetMethodEx("usize size() const", &VariantGetSize);
				VVariant.SetOperatorEx(Operators::Equals, (uint32_t)Position::Const, "bool", "const variant &in", &VariantEquals);
				VVariant.SetOperatorEx(Operators::ImplCast, (uint32_t)Position::Const, "bool", "", &VariantImplCast);

				Engine->BeginNamespace("var");
				Engine->SetFunction("variant auto_t(const string &in, bool = false)", &Core::Var::Auto);
				Engine->SetFunction("variant null_t()", &Core::Var::Null);
				Engine->SetFunction("variant undefined_t()", &Core::Var::Undefined);
				Engine->SetFunction("variant object_t()", &Core::Var::Object);
				Engine->SetFunction("variant array_t()", &Core::Var::Array);
				Engine->SetFunction("variant pointer_t(uptr@)", &Core::Var::Pointer);
				Engine->SetFunction("variant integer_t(int64)", &Core::Var::Integer);
				Engine->SetFunction("variant number_t(double)", &Core::Var::Number);
				Engine->SetFunction("variant boolean_t(bool)", &Core::Var::Boolean);
				Engine->SetFunction<Core::Variant(const Core::String&)>("variant string_t(const string &in)", &Core::Var::String);
				Engine->SetFunction<Core::Variant(const Core::String&)>("variant binary_t(const string &in)", &Core::Var::Binary);
				Engine->SetFunction<Core::Variant(const Core::String&)>("variant decimal_t(const string &in)", &Core::Var::DecimalString);
				Engine->SetFunction<Core::Variant(const Core::Decimal&)>("variant decimal_t(const decimal &in)", &Core::Var::Decimal);
				Engine->EndNamespace();

				return true;
			}
			bool Registry::LoadTimestamp(VirtualMachine* Engine)
			{
				VI_ASSERT(Engine != nullptr, false, "manager should be set");
				TypeClass VDateTime = Engine->SetStructTrivial<Core::DateTime>("timestamp");
				VDateTime.SetConstructor<Core::DateTime>("void f()");
				VDateTime.SetConstructor<Core::DateTime, uint64_t>("void f(uint64)");
				VDateTime.SetConstructor<Core::DateTime, const Core::DateTime&>("void f(const timestamp &in)");
				VDateTime.SetMethod("string format(const string &in)", &Core::DateTime::Format);
				VDateTime.SetMethod("string date(const string &in)", &Core::DateTime::Date);
				VDateTime.SetMethod("string to_iso8601()", &Core::DateTime::Iso8601);
				VDateTime.SetMethod("timestamp now()", &Core::DateTime::Now);
				VDateTime.SetMethod("timestamp from_nanoseconds(uint64)", &Core::DateTime::FromNanoseconds);
				VDateTime.SetMethod("timestamp from_microseconds(uint64)", &Core::DateTime::FromMicroseconds);
				VDateTime.SetMethod("timestamp from_milliseconds(uint64)", &Core::DateTime::FromMilliseconds);
				VDateTime.SetMethod("timestamp from_seconds(uint64)", &Core::DateTime::FromSeconds);
				VDateTime.SetMethod("timestamp from_minutes(uint64)", &Core::DateTime::FromMinutes);
				VDateTime.SetMethod("timestamp from_hours(uint64)", &Core::DateTime::FromHours);
				VDateTime.SetMethod("timestamp from_days(uint64)", &Core::DateTime::FromDays);
				VDateTime.SetMethod("timestamp from_weeks(uint64)", &Core::DateTime::FromWeeks);
				VDateTime.SetMethod("timestamp from_months(uint64)", &Core::DateTime::FromMonths);
				VDateTime.SetMethod("timestamp from_years(uint64)", &Core::DateTime::FromYears);
				VDateTime.SetMethod("timestamp& set_date_seconds(uint64, bool = true)", &Core::DateTime::SetDateSeconds);
				VDateTime.SetMethod("timestamp& set_date_minutes(uint64, bool = true)", &Core::DateTime::SetDateMinutes);
				VDateTime.SetMethod("timestamp& set_date_hours(uint64, bool = true)", &Core::DateTime::SetDateHours);
				VDateTime.SetMethod("timestamp& set_date_day(uint64, bool = true)", &Core::DateTime::SetDateDay);
				VDateTime.SetMethod("timestamp& set_date_week(uint64, bool = true)", &Core::DateTime::SetDateWeek);
				VDateTime.SetMethod("timestamp& set_date_month(uint64, bool = true)", &Core::DateTime::SetDateMonth);
				VDateTime.SetMethod("timestamp& set_date_year(uint64, bool = true)", &Core::DateTime::SetDateYear);
				VDateTime.SetMethod("uint64 date_second()", &Core::DateTime::DateSecond);
				VDateTime.SetMethod("uint64 date_minute()", &Core::DateTime::DateMinute);
				VDateTime.SetMethod("uint64 date_hour()", &Core::DateTime::DateHour);
				VDateTime.SetMethod("uint64 date_day()", &Core::DateTime::DateDay);
				VDateTime.SetMethod("uint64 date_week()", &Core::DateTime::DateWeek);
				VDateTime.SetMethod("uint64 date_month()", &Core::DateTime::DateMonth);
				VDateTime.SetMethod("uint64 date_year()", &Core::DateTime::DateYear);
				VDateTime.SetMethod("uint64 nanoseconds()", &Core::DateTime::Nanoseconds);
				VDateTime.SetMethod("uint64 microseconds()", &Core::DateTime::Microseconds);
				VDateTime.SetMethod("uint64 milliseconds()", &Core::DateTime::Milliseconds);
				VDateTime.SetMethod("uint64 seconds()", &Core::DateTime::Seconds);
				VDateTime.SetMethod("uint64 minutes()", &Core::DateTime::Minutes);
				VDateTime.SetMethod("uint64 hours()", &Core::DateTime::Hours);
				VDateTime.SetMethod("uint64 days()", &Core::DateTime::Days);
				VDateTime.SetMethod("uint64 weeks()", &Core::DateTime::Weeks);
				VDateTime.SetMethod("uint64 months()", &Core::DateTime::Months);
				VDateTime.SetMethod("uint64 years()", &Core::DateTime::Years);
				VDateTime.SetOperatorEx(Operators::AddAssign, (uint32_t)Position::Left, "timestamp&", "const timestamp &in", &DateTimeAddEq);
				VDateTime.SetOperatorEx(Operators::SubAssign, (uint32_t)Position::Left, "timestamp&", "const timestamp &in", &DateTimeSubEq);
				VDateTime.SetOperatorEx(Operators::Equals, (uint32_t)Position::Const, "bool", "const timestamp &in", &DateTimeEq);
				VDateTime.SetOperatorEx(Operators::Cmp, (uint32_t)Position::Const, "int", "const timestamp &in", &DateTimeCmp);
				VDateTime.SetOperatorEx(Operators::Add, (uint32_t)Position::Const, "timestamp", "const timestamp &in", &DateTimeAdd);
				VDateTime.SetOperatorEx(Operators::Sub, (uint32_t)Position::Const, "timestamp", "const timestamp &in", &DateTimeSub);
				VDateTime.SetMethodStatic<Core::String, int64_t>("string get_gmt(int64)", &Core::DateTime::FetchWebDateGMT);
				VDateTime.SetMethodStatic<Core::String, int64_t>("string get_time(int64)", &Core::DateTime::FetchWebDateTime);

				return true;
			}
			bool Registry::LoadConsole(VirtualMachine* Engine)
			{
#ifdef VI_HAS_BINDINGS
				VI_ASSERT(Engine != nullptr, false, "manager should be set");
				Enumeration VStdColor = Engine->SetEnum("std_color");
				VStdColor.SetValue("black", (int)Core::StdColor::Black);
				VStdColor.SetValue("dark_blue", (int)Core::StdColor::DarkBlue);
				VStdColor.SetValue("dark_green", (int)Core::StdColor::DarkGreen);
				VStdColor.SetValue("light_blue", (int)Core::StdColor::LightBlue);
				VStdColor.SetValue("dark_red", (int)Core::StdColor::DarkRed);
				VStdColor.SetValue("magenta", (int)Core::StdColor::Magenta);
				VStdColor.SetValue("orange", (int)Core::StdColor::Orange);
				VStdColor.SetValue("light_gray", (int)Core::StdColor::LightGray);
				VStdColor.SetValue("gray", (int)Core::StdColor::Gray);
				VStdColor.SetValue("blue", (int)Core::StdColor::Blue);
				VStdColor.SetValue("green", (int)Core::StdColor::Green);
				VStdColor.SetValue("cyan", (int)Core::StdColor::Cyan);
				VStdColor.SetValue("red", (int)Core::StdColor::Red);
				VStdColor.SetValue("pink", (int)Core::StdColor::Pink);
				VStdColor.SetValue("yellow", (int)Core::StdColor::Yellow);
				VStdColor.SetValue("white", (int)Core::StdColor::White);

				RefClass VConsole = Engine->SetClass<Core::Console>("console", false);
				VConsole.SetMethod("void begin()", &Core::Console::Begin);
				VConsole.SetMethod("void end()", &Core::Console::End);
				VConsole.SetMethod("void hide()", &Core::Console::Hide);
				VConsole.SetMethod("void show()", &Core::Console::Show);
				VConsole.SetMethod("void clear()", &Core::Console::Clear);
				VConsole.SetMethod("void detach()", &Core::Console::Detach);
				VConsole.SetMethod("void flush()", &Core::Console::Flush);
				VConsole.SetMethod("void flush_write()", &Core::Console::FlushWrite);
				VConsole.SetMethod("void set_cursor(uint32, uint32)", &Core::Console::SetCursor);
				VConsole.SetMethod("void set_coloring(bool)", &Core::Console::SetColoring);
				VConsole.SetMethod("void color_begin(std_color, std_color)", &Core::Console::ColorBegin);
				VConsole.SetMethod("void color_end()", &Core::Console::ColorEnd);
				VConsole.SetMethod("void capture_time()", &Core::Console::CaptureTime);
				VConsole.SetMethod("void write_line(const string &in)", &Core::Console::sWriteLine);
				VConsole.SetMethod("void write_char(uint8)", &Core::Console::WriteChar);
				VConsole.SetMethod("void write(const string &in)", &Core::Console::sWrite);
				VConsole.SetMethod("double get_captured_time()", &Core::Console::GetCapturedTime);
				VConsole.SetMethod("string read(usize)", &Core::Console::Read);
				VConsole.SetMethodStatic("console@+ get()", &Core::Console::Get);
				VConsole.SetMethodEx("void trace(uint32 = 32)", &ConsoleTrace);
				VConsole.SetMethodEx("void get_size(uint32 &out, uint32 &out)", &ConsoleGetSize);
				VConsole.SetMethodEx("void write_line(const string &in, format@+)", &Format::WriteLine);
				VConsole.SetMethodEx("void write(const string &in, format@+)", &Format::Write);

				return true;
#else
				VI_ASSERT(false, false, "<console> is not loaded");
				return false;
#endif
			}
			bool Registry::LoadSchema(VirtualMachine* Engine)
			{
				VI_ASSERT(Engine != nullptr, false, "manager should be set");
				VI_TYPEREF(Schema, "schema");

				RefClass VSchema = Engine->SetClass<Core::Schema>("schema", true);
				VSchema.SetProperty<Core::Schema>("string key", &Core::Schema::Key);
				VSchema.SetProperty<Core::Schema>("variant value", &Core::Schema::Value);
				VSchema.SetGcConstructor<Core::Schema, Schema, const Core::Variant&>("schema@ f(const variant &in)");
				VSchema.SetGcConstructorListEx<Core::Schema>("schema@ f(int &in) { repeat { string, ? } }", &SchemaConstruct);
				VSchema.SetMethod<Core::Schema, Core::Variant, size_t>("variant get_var(usize) const", &Core::Schema::GetVar);
				VSchema.SetMethod<Core::Schema, Core::Variant, const Core::String&>("variant get_var(const string &in) const", &Core::Schema::GetVar);
				VSchema.SetMethod<Core::Schema, Core::Variant, const Core::String&>("variant get_attribute_var(const string &in) const", &Core::Schema::GetAttributeVar);
				VSchema.SetMethod("schema@+ get_parent() const", &Core::Schema::GetParent);
				VSchema.SetMethod("schema@+ get_attribute(const string &in) const", &Core::Schema::GetAttribute);
				VSchema.SetMethod<Core::Schema, Core::Schema*, size_t>("schema@+ get(usize) const", &Core::Schema::Get);
				VSchema.SetMethod<Core::Schema, Core::Schema*, const Core::String&, bool>("schema@+ get(const string &in, bool = false) const", &Core::Schema::Fetch);
				VSchema.SetMethod<Core::Schema, Core::Schema*, const Core::String&>("schema@+ set(const string &in)", &Core::Schema::Set);
				VSchema.SetMethod<Core::Schema, Core::Schema*, const Core::String&, const Core::Variant&>("schema@+ set(const string &in, const variant &in)", &Core::Schema::Set);
				VSchema.SetMethod<Core::Schema, Core::Schema*, const Core::String&, const Core::Variant&>("schema@+ set_attribute(const string& in, const variant &in)", &Core::Schema::SetAttribute);
				VSchema.SetMethod<Core::Schema, Core::Schema*, const Core::Variant&>("schema@+ push(const variant &in)", &Core::Schema::Push);
				VSchema.SetMethod<Core::Schema, Core::Schema*, size_t>("schema@+ pop(usize)", &Core::Schema::Pop);
				VSchema.SetMethod<Core::Schema, Core::Schema*, const Core::String&>("schema@+ pop(const string &in)", &Core::Schema::Pop);
				VSchema.SetMethod("schema@ copy() const", &Core::Schema::Copy);
				VSchema.SetMethod("bool has(const string &in) const", &Core::Schema::Has);
				VSchema.SetMethod("bool has_attribute(const string &in) const", &Core::Schema::HasAttribute);
				VSchema.SetMethod("bool empty() const", &Core::Schema::IsEmpty);
				VSchema.SetMethod("bool is_attribute() const", &Core::Schema::IsAttribute);
				VSchema.SetMethod("bool is_saved() const", &Core::Schema::IsAttribute);
				VSchema.SetMethod("string get_name() const", &Core::Schema::GetName);
				VSchema.SetMethod("void join(schema@+, bool)", &Core::Schema::Join);
				VSchema.SetMethod("void unlink()", &Core::Schema::Unlink);
				VSchema.SetMethod("void clear()", &Core::Schema::Clear);
				VSchema.SetMethod("void save()", &Core::Schema::Save);
				VSchema.SetMethodEx("variant first_var() const", &SchemaFirstVar);
				VSchema.SetMethodEx("variant last_var() const", &SchemaLastVar);
				VSchema.SetMethodEx("schema@+ first() const", &SchemaFirst);
				VSchema.SetMethodEx("schema@+ last() const", &SchemaLast);
				VSchema.SetMethodEx("schema@+ set(const string &in, schema@+)", &SchemaSet);
				VSchema.SetMethodEx("schema@+ push(schema@+)", &SchemaPush);
				VSchema.SetMethodEx("array<schema@>@ get_collection(const string &in, bool = false) const", &SchemaGetCollection);
				VSchema.SetMethodEx("array<schema@>@ get_attributes() const", &SchemaGetAttributes);
				VSchema.SetMethodEx("array<schema@>@ get_childs() const", &SchemaGetChilds);
				VSchema.SetMethodEx("dictionary@ get_names() const", &SchemaGetNames);
				VSchema.SetMethodEx("usize size() const", &SchemaGetSize);
				VSchema.SetMethodEx("string to_json() const", &SchemaToJSON);
				VSchema.SetMethodEx("string to_xml() const", &SchemaToXML);
				VSchema.SetMethodEx("string to_string() const", &SchemaToString);
				VSchema.SetMethodEx("string to_binary() const", &SchemaToBinary);
				VSchema.SetMethodEx("int64 to_integer() const", &SchemaToInteger);
				VSchema.SetMethodEx("double to_number() const", &SchemaToNumber);
				VSchema.SetMethodEx("decimal to_decimal() const", &SchemaToDecimal);
				VSchema.SetMethodEx("bool to_boolean() const", &SchemaToBoolean);
				VSchema.SetMethodStatic("schema@ from_json(const string &in)", &SchemaFromJSON);
				VSchema.SetMethodStatic("schema@ from_xml(const string &in)", &SchemaFromXML);
				VSchema.SetMethodStatic("schema@ import_json(const string &in)", &SchemaImport);
				VSchema.SetOperatorEx(Operators::Assign, (uint32_t)Position::Left, "schema@+", "const variant &in", &SchemaCopyAssign);
				VSchema.SetOperatorEx(Operators::Equals, (uint32_t)(Position::Left | Position::Const), "bool", "schema@+", &SchemaEquals);
				VSchema.SetOperatorEx(Operators::Index, (uint32_t)Position::Left, "schema@+", "const string &in", &SchemaGetIndex);
				VSchema.SetOperatorEx(Operators::Index, (uint32_t)Position::Left, "schema@+", "usize", &SchemaGetIndexOffset);
				VSchema.SetEnumRefsEx<Core::Schema>(&SchemaEnumRefs);
				VSchema.SetReleaseRefsEx<Core::Schema>([](Core::Schema* Base, asIScriptEngine*)
				{
					Base->Unlink();
					Base->Clear();
				});

				Engine->BeginNamespace("var::set");
				Engine->SetFunction("schema@ auto_t(const string &in, bool = false)", &Core::Var::Auto);
				Engine->SetFunction("schema@ null_t()", &Core::Var::Set::Null);
				Engine->SetFunction("schema@ undefined_t()", &Core::Var::Set::Undefined);
				Engine->SetFunction("schema@ object_t()", &Core::Var::Set::Object);
				Engine->SetFunction("schema@ array_t()", &Core::Var::Set::Array);
				Engine->SetFunction("schema@ pointer_t(uptr@)", &Core::Var::Set::Pointer);
				Engine->SetFunction("schema@ integer_t(int64)", &Core::Var::Set::Integer);
				Engine->SetFunction("schema@ number_t(double)", &Core::Var::Set::Number);
				Engine->SetFunction("schema@ boolean_t(bool)", &Core::Var::Set::Boolean);
				Engine->SetFunction<Core::Schema* (const Core::String&)>("schema@ string_t(const string &in)", &Core::Var::Set::String);
				Engine->SetFunction<Core::Schema* (const Core::String&)>("schema@ binary_t(const string &in)", &Core::Var::Set::Binary);
				Engine->SetFunction<Core::Schema* (const Core::String&)>("schema@ decimal_t(const string &in)", &Core::Var::Set::DecimalString);
				Engine->SetFunction<Core::Schema* (const Core::Decimal&)>("schema@ decimal_t(const decimal &in)", &Core::Var::Set::Decimal);
				Engine->SetFunction("schema@+ as(schema@+)", &SchemaInit);
				Engine->EndNamespace();

				return true;
			}
			bool Registry::LoadClockTimer(VirtualMachine* Engine)
			{
#ifdef VI_HAS_BINDINGS
				VI_ASSERT(Engine != nullptr, false, "manager should be set");

				RefClass VTimer = Engine->SetClass<Core::Timer>("clock_timer", false);
				VTimer.SetConstructor<Core::Timer>("clock_timer@ f()");
				VTimer.SetMethod("void set_fixed_frames(float)", &Core::Timer::SetFixedFrames);
				VTimer.SetMethod("void begin()", &Core::Timer::Begin);
				VTimer.SetMethod("void finish()", &Core::Timer::Finish);
				VTimer.SetMethod("usize get_frame_index() const", &Core::Timer::GetFrameIndex);
				VTimer.SetMethod("usize get_fixed_frame_index() const", &Core::Timer::GetFixedFrameIndex);
				VTimer.SetMethod("float get_max_frames() const", &Core::Timer::GetMaxFrames);
				VTimer.SetMethod("float get_min_step() const", &Core::Timer::GetMinStep);
				VTimer.SetMethod("float get_frames() const", &Core::Timer::GetFrames);
				VTimer.SetMethod("float get_elapsed() const", &Core::Timer::GetElapsed);
				VTimer.SetMethod("float get_elapsed_mills() const", &Core::Timer::GetElapsedMills);
				VTimer.SetMethod("float get_step() const", &Core::Timer::GetStep);
				VTimer.SetMethod("float get_fixed_step() const", &Core::Timer::GetFixedStep);
				VTimer.SetMethod("bool is_fixed() const", &Core::Timer::IsFixed);

				return true;
#else
				VI_ASSERT(false, false, "<clock_timer> is not loaded");
				return false;
#endif
			}
			bool Registry::LoadFileSystem(VirtualMachine* Engine)
			{
#ifdef VI_HAS_BINDINGS
				VI_ASSERT(Engine != nullptr, false, "manager should be set");
				Enumeration VFileMode = Engine->SetEnum("file_mode");
				VFileMode.SetValue("read_only", (int)Core::FileMode::Read_Only);
				VFileMode.SetValue("write_only", (int)Core::FileMode::Write_Only);
				VFileMode.SetValue("append_only", (int)Core::FileMode::Append_Only);
				VFileMode.SetValue("read_write", (int)Core::FileMode::Read_Write);
				VFileMode.SetValue("write_read", (int)Core::FileMode::Write_Read);
				VFileMode.SetValue("read_append_write", (int)Core::FileMode::Read_Append_Write);
				VFileMode.SetValue("binary_read_only", (int)Core::FileMode::Binary_Read_Only);
				VFileMode.SetValue("binary_write_only", (int)Core::FileMode::Binary_Write_Only);
				VFileMode.SetValue("binary_append_only", (int)Core::FileMode::Binary_Append_Only);
				VFileMode.SetValue("binary_read_write", (int)Core::FileMode::Binary_Read_Write);
				VFileMode.SetValue("binary_write_read", (int)Core::FileMode::Binary_Write_Read);
				VFileMode.SetValue("binary_read_append_write", (int)Core::FileMode::Binary_Read_Append_Write);

				Enumeration VFileSeek = Engine->SetEnum("file_seek");
				VFileSeek.SetValue("begin", (int)Core::FileSeek::Begin);
				VFileSeek.SetValue("current", (int)Core::FileSeek::Current);
				VFileSeek.SetValue("end", (int)Core::FileSeek::End);

				TypeClass VFileState = Engine->SetPod<Core::FileState>("file_state");
				VFileState.SetProperty("usize size", &Core::FileState::Size);
				VFileState.SetProperty("usize links", &Core::FileState::Links);
				VFileState.SetProperty("usize permissions", &Core::FileState::Permissions);
				VFileState.SetProperty("usize document", &Core::FileState::Document);
				VFileState.SetProperty("usize device", &Core::FileState::Device);
				VFileState.SetProperty("usize userId", &Core::FileState::UserId);
				VFileState.SetProperty("usize groupId", &Core::FileState::GroupId);
				VFileState.SetProperty("int64 last_access", &Core::FileState::LastAccess);
				VFileState.SetProperty("int64 last_permission_change", &Core::FileState::LastPermissionChange);
				VFileState.SetProperty("int64 last_modified", &Core::FileState::LastModified);
				VFileState.SetProperty("bool exists", &Core::FileState::Exists);
				VFileState.SetConstructor<Core::FileState>("void f()");

				TypeClass VFileEntry = Engine->SetStructTrivial<Core::FileEntry>("file_entry");
				VFileEntry.SetConstructor<Core::FileEntry>("void f()");
				VFileEntry.SetProperty("string path", &Core::FileEntry::Path);
				VFileEntry.SetProperty("usize size", &Core::FileEntry::Size);
				VFileEntry.SetProperty("int64 last_modified", &Core::FileEntry::LastModified);
				VFileEntry.SetProperty("int64 creation_time", &Core::FileEntry::CreationTime);
				VFileEntry.SetProperty("bool is_referenced", &Core::FileEntry::IsReferenced);
				VFileEntry.SetProperty("bool is_directory", &Core::FileEntry::IsDirectory);
				VFileEntry.SetProperty("bool is_exists", &Core::FileEntry::IsExists);

				RefClass VStream = Engine->SetClass<Core::Stream>("base_stream", false);
				VStream.SetMethod("void clear()", &Core::Stream::Clear);
				VStream.SetMethod("bool close()", &Core::Stream::Close);
				VStream.SetMethod("bool seek(file_seek, int64)", &Core::Stream::Seek);
				VStream.SetMethod("bool move(int64)", &Core::Stream::Move);
				VStream.SetMethod("int32 flush()", &Core::Stream::Flush);
				VStream.SetMethod("int32 get_fd()", &Core::Stream::GetFd);
				VStream.SetMethod("usize get_size()", &Core::Stream::GetSize);
				VStream.SetMethod("usize tell()", &Core::Stream::Tell);
				VStream.SetMethodEx("bool open(const string &in, file_mode)", &StreamOpen);
				VStream.SetMethodEx("usize write(const string &in)", &StreamWrite);
				VStream.SetMethodEx("string read(usize)", &StreamRead);

				RefClass VFileStream = Engine->SetClass<Core::FileStream>("file_stream", false);
				VFileStream.SetConstructor<Core::FileStream>("file_stream@ f()");
				VFileStream.SetMethod("void clear()", &Core::FileStream::Clear);
				VFileStream.SetMethod("bool close()", &Core::FileStream::Close);
				VFileStream.SetMethod("bool seek(file_seek, int64)", &Core::FileStream::Seek);
				VFileStream.SetMethod("bool move(int64)", &Core::FileStream::Move);
				VFileStream.SetMethod("int32 flush()", &Core::FileStream::Flush);
				VFileStream.SetMethod("int32 get_fd()", &Core::FileStream::GetFd);
				VFileStream.SetMethod("usize get_size()", &Core::FileStream::GetSize);
				VFileStream.SetMethod("usize tell()", &Core::FileStream::Tell);
				VFileStream.SetMethodEx("bool open(const string &in, file_mode)", &FileStreamOpen);
				VFileStream.SetMethodEx("usize write(const string &in)", &FileStreamWrite);
				VFileStream.SetMethodEx("string read(usize)", &FileStreamRead);

				RefClass VGzStream = Engine->SetClass<Core::GzStream>("gz_stream", false);
				VGzStream.SetConstructor<Core::GzStream>("gz_stream@ f()");
				VGzStream.SetMethod("void clear()", &Core::GzStream::Clear);
				VGzStream.SetMethod("bool close()", &Core::GzStream::Close);
				VGzStream.SetMethod("bool seek(file_seek, int64)", &Core::GzStream::Seek);
				VGzStream.SetMethod("bool move(int64)", &Core::GzStream::Move);
				VGzStream.SetMethod("int32 flush()", &Core::GzStream::Flush);
				VGzStream.SetMethod("int32 get_fd()", &Core::GzStream::GetFd);
				VGzStream.SetMethod("usize get_size()", &Core::GzStream::GetSize);
				VGzStream.SetMethod("usize tell()", &Core::GzStream::Tell);
				VGzStream.SetMethodEx("bool open(const string &in, file_mode)", &GzStreamOpen);
				VGzStream.SetMethodEx("usize write(const string &in)", &GzStreamWrite);
				VGzStream.SetMethodEx("string read(usize)", &GzStreamRead);

				RefClass VWebStream = Engine->SetClass<Core::WebStream>("web_stream", false);
				VWebStream.SetConstructor<Core::WebStream, bool>("web_stream@ f(bool)");
				VWebStream.SetMethod("void clear()", &Core::WebStream::Clear);
				VWebStream.SetMethod("bool close()", &Core::WebStream::Close);
				VWebStream.SetMethod("bool seek(file_seek, int64)", &Core::WebStream::Seek);
				VWebStream.SetMethod("bool move(int64)", &Core::WebStream::Move);
				VWebStream.SetMethod("int32 flush()", &Core::WebStream::Flush);
				VWebStream.SetMethod("int32 get_fd()", &Core::WebStream::GetFd);
				VWebStream.SetMethod("usize get_size()", &Core::WebStream::GetSize);
				VWebStream.SetMethod("usize tell()", &Core::WebStream::Tell);
				VWebStream.SetMethodEx("bool open(const string &in, file_mode)", &WebStreamOpen);
				VWebStream.SetMethodEx("usize write(const string &in)", &WebStreamWrite);
				VWebStream.SetMethodEx("string read(usize)", &WebStreamRead);

				VStream.SetDynamicCast<Core::Stream, Core::FileStream>("file_stream@+");
				VStream.SetDynamicCast<Core::Stream, Core::GzStream>("gz_stream@+");
				VStream.SetDynamicCast<Core::Stream, Core::WebStream>("web_stream@+");
				VFileStream.SetDynamicCast<Core::FileStream, Core::Stream>("base_stream@+", true);
				VGzStream.SetDynamicCast<Core::GzStream, Core::Stream>("base_stream@+", true);
				VWebStream.SetDynamicCast<Core::WebStream, Core::Stream>("base_stream@+", true);

				return true;
#else
				VI_ASSERT(false, false, "<file_system> is not loaded");
				return false;
#endif
			}
			bool Registry::LoadOS(VirtualMachine* Engine)
			{
#ifdef VI_HAS_BINDINGS
				VI_ASSERT(Engine != nullptr, false, "manager should be set");
				Engine->BeginNamespace("os::cpu");
				Enumeration VArch = Engine->SetEnum("arch");
				VArch.SetValue("x64", (int)Core::OS::CPU::Arch::X64);
				VArch.SetValue("arm", (int)Core::OS::CPU::Arch::ARM);
				VArch.SetValue("itanium", (int)Core::OS::CPU::Arch::Itanium);
				VArch.SetValue("x86", (int)Core::OS::CPU::Arch::X86);
				VArch.SetValue("unknown", (int)Core::OS::CPU::Arch::Unknown);

				Enumeration VEndian = Engine->SetEnum("endian");
				VEndian.SetValue("little", (int)Core::OS::CPU::Endian::Little);
				VEndian.SetValue("big", (int)Core::OS::CPU::Endian::Big);

				Enumeration VCache = Engine->SetEnum("cache");
				VCache.SetValue("unified", (int)Core::OS::CPU::Cache::Unified);
				VCache.SetValue("instruction", (int)Core::OS::CPU::Cache::Instruction);
				VCache.SetValue("data", (int)Core::OS::CPU::Cache::Data);
				VCache.SetValue("trace", (int)Core::OS::CPU::Cache::Trace);

				TypeClass VQuantityInfo = Engine->SetPod<Core::OS::CPU::QuantityInfo>("quantity_info");
				VQuantityInfo.SetProperty("uint32 logical", &Core::OS::CPU::QuantityInfo::Logical);
				VQuantityInfo.SetProperty("uint32 physical", &Core::OS::CPU::QuantityInfo::Physical);
				VQuantityInfo.SetProperty("uint32 packages", &Core::OS::CPU::QuantityInfo::Packages);
				VQuantityInfo.SetConstructor<Core::OS::CPU::QuantityInfo>("void f()");

				TypeClass VCacheInfo = Engine->SetPod<Core::OS::CPU::CacheInfo>("cache_info");
				VCacheInfo.SetProperty("usize size", &Core::OS::CPU::CacheInfo::Size);
				VCacheInfo.SetProperty("usize line_size", &Core::OS::CPU::CacheInfo::LineSize);
				VCacheInfo.SetProperty("uint8 associativity", &Core::OS::CPU::CacheInfo::Associativity);
				VCacheInfo.SetProperty("cache type", &Core::OS::CPU::CacheInfo::Type);
				VCacheInfo.SetConstructor<Core::OS::CPU::CacheInfo>("void f()");

				Engine->SetFunction("quantity_info get_quantity_info()", &Core::OS::CPU::GetQuantityInfo);
				Engine->SetFunction("cache_info get_cache_info(uint32)", &Core::OS::CPU::GetCacheInfo);
				Engine->SetFunction("arch get_arch()", &Core::OS::CPU::GetArch);
				Engine->SetFunction("endian get_endianness()", &Core::OS::CPU::GetEndianness);
				Engine->SetFunction("usize get_frequency()", &Core::OS::CPU::GetFrequency);
				Engine->EndNamespace();

				Engine->BeginNamespace("os::directory");
				Engine->SetFunction("array<file_entry>@ scan(const string &in)", &OSDirectoryScan);
				Engine->SetFunction("array<string>@ get_mounts(const string &in)", &OSDirectoryGetMounts);
				Engine->SetFunction("bool create(const string &in)", &OSDirectoryCreate);
				Engine->SetFunction("bool remove(const string &in)", &OSDirectoryRemove);
				Engine->SetFunction("bool is_exists(const string &in)", &OSDirectoryIsExists);
				Engine->SetFunction("string get()", &Core::OS::Directory::Get);
				Engine->SetFunction("void set(const string &in)", &OSDirectorySet);
				Engine->SetFunction("void patch(const string &in)", &Core::OS::Directory::Patch);
				Engine->EndNamespace();

				Engine->BeginNamespace("os::file");
				Engine->SetFunction<bool(const Core::String&, const Core::String&)>("bool write(const string &in, const string &in)", &Core::OS::File::Write);
				Engine->SetFunction("bool state(const string &in, file_entry &out)", &OSFileState);
				Engine->SetFunction("bool move(const string &in, const string &in)", &OSFileMove);
				Engine->SetFunction("bool remove(const string &in)", &OSFileRemove);
				Engine->SetFunction("bool is_exists(const string &in)", &OSFileIsExists);
				Engine->SetFunction("file_state get_properties(const string &in)", &OSFileGetProperties);
				Engine->SetFunction("string read_as_string(const string &in)", &Core::OS::File::ReadAsString);
				Engine->SetFunction("array<string>@ read_as_array(const string &in)", &OSFileReadAsArray);
				Engine->SetFunction("usize join(const string &in, array<string>@+)", &OSFileJoin);
				Engine->SetFunction("int32 compare(const string &in, const string &in)", &Core::OS::File::Compare);
				Engine->SetFunction("int64 get_hash(const string &in)", &Core::OS::File::GetHash);
				Engine->SetFunction("base_stream@ open_join(const string &in, array<string>@+)", &OSFileOpenJoin);
				Engine->SetFunction("base_stream@ open_archive(const string &in, usize)", &Core::OS::File::OpenArchive);
				Engine->SetFunction<Core::Stream*(const Core::String&, Core::FileMode, bool)>("base_stream@ open(const string &in, file_mode, bool = false)", &Core::OS::File::Open);
				Engine->EndNamespace();

				Engine->BeginNamespace("os::path");
				Engine->SetFunction("bool is_remote(const string &in)", &OSPathIsRemote);
				Engine->SetFunction("string resolve(const string &in)", &OSPathResolve);
				Engine->SetFunction("string resolve_directory(const string &in)", &OSPathResolveDirectory);
				Engine->SetFunction("string get_directory(const string &in, usize = 0)", &OSPathGetDirectory);
				Engine->SetFunction("string get_filename(const string &in)", &OSPathGetFilename);
				Engine->SetFunction("string get_extension(const string &in)", &OSPathGetExtension);
				Engine->SetFunction("string get_non_existant(const string &in)", &Core::OS::Path::GetNonExistant);
				Engine->SetFunction<Core::String(const Core::String&, const Core::String&)>("string resolve(const string &in, const string &in)", &Core::OS::Path::Resolve);
				Engine->SetFunction<Core::String(const Core::String&, const Core::String&)>("string resolve_directory(const string &in, const string &in)", &Core::OS::Path::ResolveDirectory);
				Engine->EndNamespace();

				Engine->BeginNamespace("os::process");
				Engine->SetFunction("void interrupt()", &Core::OS::Process::Interrupt);
				Engine->SetFunction("int execute(const string &in)", &OSProcessExecute);
				Engine->SetFunction("int await(uptr@)", &OSProcessAwait);
				Engine->SetFunction("bool free(uptr@)", &OSProcessFree);
				Engine->SetFunction("uptr@ spawn(const string &in, array<string>@+)", &OSProcessSpawn);
				Engine->EndNamespace();

				Engine->BeginNamespace("os::symbol");
				Engine->SetFunction("uptr@ load(const string &in)", &Core::OS::Symbol::Load);
				Engine->SetFunction("uptr@ load_function(uptr@, const string &in)", &Core::OS::Symbol::LoadFunction);
				Engine->SetFunction("bool unload(uptr@)", &Core::OS::Symbol::Unload);
				Engine->EndNamespace();

				Engine->BeginNamespace("os::input");
				Engine->SetFunction("bool text(const string &in, const string &in, const string &in, string &out)", &Core::OS::Input::Text);
				Engine->SetFunction("bool password(const string &in, const string &in, string &out)", &Core::OS::Input::Password);
				Engine->SetFunction("bool save(const string &in, const string &in, const string &in, const string &in, string &out)", &Core::OS::Input::Save);
				Engine->SetFunction("bool open(const string &in, const string &in, const string &in, const string &in, bool, string &out)", &Core::OS::Input::Open);
				Engine->SetFunction("bool folder(const string &in, const string &in, string &out)", &Core::OS::Input::Folder);
				Engine->SetFunction("bool color(const string &in, const string &in, string &out)", &Core::OS::Input::Color);
				Engine->EndNamespace();

				Engine->BeginNamespace("os::error");
				Engine->SetFunction("int32 get()", &Core::OS::Error::Get);
				Engine->SetFunction("string get_name(int32)", &Core::OS::Error::GetName);
				Engine->SetFunction("bool is_error(int32)", &Core::OS::Error::IsError);
				Engine->EndNamespace();

				return true;
#else
				VI_ASSERT(false, false, "<os> is not loaded");
				return false;
#endif
			}
			bool Registry::LoadSchedule(VirtualMachine* Engine)
			{
#ifdef VI_HAS_BINDINGS
				VI_ASSERT(Engine != nullptr, false, "manager should be set");
				Engine->GetEngine()->RegisterTypedef("task_id", "uint64");

				Enumeration VDifficulty = Engine->SetEnum("difficulty");
				VDifficulty.SetValue("coroutine", (int)Core::Difficulty::Coroutine);
				VDifficulty.SetValue("light", (int)Core::Difficulty::Light);
				VDifficulty.SetValue("heavy", (int)Core::Difficulty::Heavy);
				VDifficulty.SetValue("clock", (int)Core::Difficulty::Clock);

				TypeClass VDesc = Engine->SetStructTrivial<Core::Schedule::Desc>("schedule_policy");
				VDesc.SetProperty("usize memory", &Core::Schedule::Desc::Memory);
				VDesc.SetProperty("usize coroutines", &Core::Schedule::Desc::Coroutines);
				VDesc.SetProperty("bool parallel", &Core::Schedule::Desc::Parallel);
				VDesc.SetConstructor<Core::Schedule::Desc>("void f()");
				VDesc.SetMethod("void set_threads(usize)", &Core::Schedule::Desc::SetThreads);

				RefClass VSchedule = Engine->SetClass<Core::Schedule>("schedule", false);
				VSchedule.SetFunctionDef("void task_event()");
				VSchedule.SetMethodEx("task_id set_interval(uint64, task_event@+, difficulty = difficulty::light, bool = false)", &ScheduleSetInterval);
				VSchedule.SetMethodEx("task_id set_timeout(uint64, task_event@+, difficulty = difficulty::light, bool = false)", &ScheduleSetTimeout);
				VSchedule.SetMethodEx("bool set_immediate(task_event@+, difficulty = difficulty::heavy, bool = false)", &ScheduleSetImmediate);
				VSchedule.SetMethod("bool clear_timeout(task_id)", &Core::Schedule::ClearTimeout);
				VSchedule.SetMethod("bool dispatch()", &Core::Schedule::Dispatch);
				VSchedule.SetMethod("bool start(const schedule_policy &in)", &Core::Schedule::Start);
				VSchedule.SetMethod("bool stop()", &Core::Schedule::Stop);
				VSchedule.SetMethod("bool wakeup()", &Core::Schedule::Wakeup);
				VSchedule.SetMethod("bool is_active() const", &Core::Schedule::IsActive);
				VSchedule.SetMethod("bool can_enqueue() const", &Core::Schedule::CanEnqueue);
				VSchedule.SetMethod("bool has_tasks(difficulty) const", &Core::Schedule::HasTasks);
				VSchedule.SetMethod("bool has_any_tasks() const", &Core::Schedule::HasAnyTasks);
				VSchedule.SetMethod("usize get_total_threads() const", &Core::Schedule::GetTotalThreads);
				VSchedule.SetMethod("usize get_thread_global_index()", &Core::Schedule::GetThreadGlobalIndex);
				VSchedule.SetMethod("usize get_thread_local_index()", &Core::Schedule::GetThreadLocalIndex);
				VSchedule.SetMethod("usize get_threads(difficulty) const", &Core::Schedule::GetThreads);
				VSchedule.SetMethod("const schedule_policy& get_policy() const", &Core::Schedule::GetPolicy);
				VSchedule.SetMethodStatic("schedule@+ get()", &Core::Schedule::Get);

				return true;
#else
				VI_ASSERT(false, false, "<schedule> is not loaded");
				return false;
#endif
			}
			bool Registry::LoadVertices(VirtualMachine* Engine)
			{
#ifdef VI_HAS_BINDINGS
				VI_ASSERT(Engine != nullptr, false, "manager should be set");
				TypeClass VVertex = Engine->SetPod<Compute::Vertex>("vertex");
				VVertex.SetProperty<Compute::Vertex>("float position_x", &Compute::Vertex::PositionX);
				VVertex.SetProperty<Compute::Vertex>("float position_y", &Compute::Vertex::PositionY);
				VVertex.SetProperty<Compute::Vertex>("float position_z", &Compute::Vertex::PositionZ);
				VVertex.SetProperty<Compute::Vertex>("float texcoord_x", &Compute::Vertex::TexCoordX);
				VVertex.SetProperty<Compute::Vertex>("float texcoord_y", &Compute::Vertex::TexCoordY);
				VVertex.SetProperty<Compute::Vertex>("float normal_x", &Compute::Vertex::NormalX);
				VVertex.SetProperty<Compute::Vertex>("float normal_y", &Compute::Vertex::NormalY);
				VVertex.SetProperty<Compute::Vertex>("float normal_z", &Compute::Vertex::NormalZ);
				VVertex.SetProperty<Compute::Vertex>("float tangent_x", &Compute::Vertex::TangentX);
				VVertex.SetProperty<Compute::Vertex>("float tangent_y", &Compute::Vertex::TangentY);
				VVertex.SetProperty<Compute::Vertex>("float tangent_z", &Compute::Vertex::TangentZ);
				VVertex.SetProperty<Compute::Vertex>("float bitangent_x", &Compute::Vertex::BitangentX);
				VVertex.SetProperty<Compute::Vertex>("float bitangent_y", &Compute::Vertex::BitangentY);
				VVertex.SetProperty<Compute::Vertex>("float bitangent_z", &Compute::Vertex::BitangentZ);
				VVertex.SetConstructor<Compute::Vertex>("void f()");

				TypeClass VSkinVertex = Engine->SetPod<Compute::SkinVertex>("skin_vertex");
				VSkinVertex.SetProperty<Compute::SkinVertex>("float position_x", &Compute::SkinVertex::PositionX);
				VSkinVertex.SetProperty<Compute::SkinVertex>("float position_y", &Compute::SkinVertex::PositionY);
				VSkinVertex.SetProperty<Compute::SkinVertex>("float position_z", &Compute::SkinVertex::PositionZ);
				VSkinVertex.SetProperty<Compute::SkinVertex>("float texcoord_x", &Compute::SkinVertex::TexCoordX);
				VSkinVertex.SetProperty<Compute::SkinVertex>("float texcoord_y", &Compute::SkinVertex::TexCoordY);
				VSkinVertex.SetProperty<Compute::SkinVertex>("float normal_x", &Compute::SkinVertex::NormalX);
				VSkinVertex.SetProperty<Compute::SkinVertex>("float normal_y", &Compute::SkinVertex::NormalY);
				VSkinVertex.SetProperty<Compute::SkinVertex>("float normal_z", &Compute::SkinVertex::NormalZ);
				VSkinVertex.SetProperty<Compute::SkinVertex>("float tangent_x", &Compute::SkinVertex::TangentX);
				VSkinVertex.SetProperty<Compute::SkinVertex>("float tangent_y", &Compute::SkinVertex::TangentY);
				VSkinVertex.SetProperty<Compute::SkinVertex>("float tangent_z", &Compute::SkinVertex::TangentZ);
				VSkinVertex.SetProperty<Compute::SkinVertex>("float bitangent_x", &Compute::SkinVertex::BitangentX);
				VSkinVertex.SetProperty<Compute::SkinVertex>("float bitangent_y", &Compute::SkinVertex::BitangentY);
				VSkinVertex.SetProperty<Compute::SkinVertex>("float bitangent_z", &Compute::SkinVertex::BitangentZ);
				VSkinVertex.SetProperty<Compute::SkinVertex>("float joint_index0", &Compute::SkinVertex::JointIndex0);
				VSkinVertex.SetProperty<Compute::SkinVertex>("float joint_index1", &Compute::SkinVertex::JointIndex1);
				VSkinVertex.SetProperty<Compute::SkinVertex>("float joint_index2", &Compute::SkinVertex::JointIndex2);
				VSkinVertex.SetProperty<Compute::SkinVertex>("float joint_index3", &Compute::SkinVertex::JointIndex3);
				VSkinVertex.SetProperty<Compute::SkinVertex>("float joint_bias0", &Compute::SkinVertex::JointBias0);
				VSkinVertex.SetProperty<Compute::SkinVertex>("float joint_bias1", &Compute::SkinVertex::JointBias1);
				VSkinVertex.SetProperty<Compute::SkinVertex>("float joint_bias2", &Compute::SkinVertex::JointBias2);
				VSkinVertex.SetProperty<Compute::SkinVertex>("float joint_bias3", &Compute::SkinVertex::JointBias3);
				VSkinVertex.SetConstructor<Compute::SkinVertex>("void f()");

				TypeClass VShapeVertex = Engine->SetPod<Compute::ShapeVertex>("shape_vertex");
				VShapeVertex.SetProperty<Compute::ShapeVertex>("float position_x", &Compute::ShapeVertex::PositionX);
				VShapeVertex.SetProperty<Compute::ShapeVertex>("float position_y", &Compute::ShapeVertex::PositionY);
				VShapeVertex.SetProperty<Compute::ShapeVertex>("float position_z", &Compute::ShapeVertex::PositionZ);
				VShapeVertex.SetProperty<Compute::ShapeVertex>("float texcoord_x", &Compute::ShapeVertex::TexCoordX);
				VShapeVertex.SetProperty<Compute::ShapeVertex>("float texcoord_y", &Compute::ShapeVertex::TexCoordY);
				VShapeVertex.SetConstructor<Compute::ShapeVertex>("void f()");

				TypeClass VElementVertex = Engine->SetPod<Compute::ElementVertex>("element_vertex");
				VElementVertex.SetProperty<Compute::ElementVertex>("float position_x", &Compute::ElementVertex::PositionX);
				VElementVertex.SetProperty<Compute::ElementVertex>("float position_y", &Compute::ElementVertex::PositionY);
				VElementVertex.SetProperty<Compute::ElementVertex>("float position_z", &Compute::ElementVertex::PositionZ);
				VElementVertex.SetProperty<Compute::ElementVertex>("float velocity_x", &Compute::ElementVertex::VelocityX);
				VElementVertex.SetProperty<Compute::ElementVertex>("float velocity_y", &Compute::ElementVertex::VelocityY);
				VElementVertex.SetProperty<Compute::ElementVertex>("float velocity_z", &Compute::ElementVertex::VelocityZ);
				VElementVertex.SetProperty<Compute::ElementVertex>("float color_x", &Compute::ElementVertex::ColorX);
				VElementVertex.SetProperty<Compute::ElementVertex>("float color_y", &Compute::ElementVertex::ColorY);
				VElementVertex.SetProperty<Compute::ElementVertex>("float color_z", &Compute::ElementVertex::ColorZ);
				VElementVertex.SetProperty<Compute::ElementVertex>("float color_w", &Compute::ElementVertex::ColorW);
				VElementVertex.SetProperty<Compute::ElementVertex>("float scale", &Compute::ElementVertex::Scale);
				VElementVertex.SetProperty<Compute::ElementVertex>("float rotation", &Compute::ElementVertex::Rotation);
				VElementVertex.SetProperty<Compute::ElementVertex>("float angular", &Compute::ElementVertex::Angular);
				VElementVertex.SetConstructor<Compute::ElementVertex>("void f()");

				return true;
#else
				VI_ASSERT(false, false, "<vertices> is not loaded");
				return false;
#endif
			}
			bool Registry::LoadVectors(VirtualMachine* Engine)
			{
#ifdef VI_HAS_BINDINGS
				VI_ASSERT(Engine != nullptr, false, "manager should be set");
				Enumeration VCubeFace = Engine->SetEnum("cube_face");
				VCubeFace.SetValue("positive_x", (int)Compute::CubeFace::PositiveX);
				VCubeFace.SetValue("negative_x", (int)Compute::CubeFace::NegativeX);
				VCubeFace.SetValue("positive_y", (int)Compute::CubeFace::PositiveY);
				VCubeFace.SetValue("negative_y", (int)Compute::CubeFace::NegativeY);
				VCubeFace.SetValue("positive_z", (int)Compute::CubeFace::PositiveZ);
				VCubeFace.SetValue("negative_z", (int)Compute::CubeFace::NegativeZ);

				TypeClass VVector2 = Engine->SetPod<Compute::Vector2>("vector2");
				VVector2.SetConstructor<Compute::Vector2>("void f()");
				VVector2.SetConstructor<Compute::Vector2, float, float>("void f(float, float)");
				VVector2.SetConstructor<Compute::Vector2, float>("void f(float)");
				VVector2.SetProperty<Compute::Vector2>("float x", &Compute::Vector2::X);
				VVector2.SetProperty<Compute::Vector2>("float y", &Compute::Vector2::Y);
				VVector2.SetMethod("float length() const", &Compute::Vector2::Length);
				VVector2.SetMethod("float sum() const", &Compute::Vector2::Sum);
				VVector2.SetMethod("float dot(const vector2 &in) const", &Compute::Vector2::Dot);
				VVector2.SetMethod("float distance(const vector2 &in) const", &Compute::Vector2::Distance);
				VVector2.SetMethod("float hypotenuse() const", &Compute::Vector2::Hypotenuse);
				VVector2.SetMethod("float look_at(const vector2 &in) const", &Compute::Vector2::LookAt);
				VVector2.SetMethod("float cross(const vector2 &in) const", &Compute::Vector2::Cross);
				VVector2.SetMethod("vector2 direction(float) const", &Compute::Vector2::Direction);
				VVector2.SetMethod("vector2 inv() const", &Compute::Vector2::Inv);
				VVector2.SetMethod("vector2 inv_x() const", &Compute::Vector2::InvX);
				VVector2.SetMethod("vector2 inv_y() const", &Compute::Vector2::InvY);
				VVector2.SetMethod("vector2 normalize() const", &Compute::Vector2::Normalize);
				VVector2.SetMethod("vector2 snormalize() const", &Compute::Vector2::sNormalize);
				VVector2.SetMethod("vector2 lerp(const vector2 &in, float) const", &Compute::Vector2::Lerp);
				VVector2.SetMethod("vector2 slerp(const vector2 &in, float) const", &Compute::Vector2::sLerp);
				VVector2.SetMethod("vector2 alerp(const vector2 &in, float) const", &Compute::Vector2::aLerp);
				VVector2.SetMethod("vector2 rlerp() const", &Compute::Vector2::rLerp);
				VVector2.SetMethod("vector2 abs() const", &Compute::Vector2::Abs);
				VVector2.SetMethod("vector2 radians() const", &Compute::Vector2::Radians);
				VVector2.SetMethod("vector2 degrees() const", &Compute::Vector2::Degrees);
				VVector2.SetMethod("vector2 xy() const", &Compute::Vector2::XY);
				VVector2.SetMethod<Compute::Vector2, Compute::Vector2, float>("vector2 mul(float) const", &Compute::Vector2::Mul);
				VVector2.SetMethod<Compute::Vector2, Compute::Vector2, float, float>("vector2 mul(float, float) const", &Compute::Vector2::Mul);
				VVector2.SetMethod<Compute::Vector2, Compute::Vector2, const Compute::Vector2&>("vector2 mul(const vector2 &in) const", &Compute::Vector2::Mul);
				VVector2.SetMethod("vector2 div(const vector2 &in) const", &Compute::Vector2::Div);
				VVector2.SetMethod("vector2 set_x(float) const", &Compute::Vector2::SetX);
				VVector2.SetMethod("vector2 set_y(float) const", &Compute::Vector2::SetY);
				VVector2.SetMethod("void set(const vector2 &in) const", &Compute::Vector2::Set);
				VVector2.SetOperatorEx(Operators::MulAssign, (uint32_t)Position::Left, "vector2&", "const vector2 &in", &Vector2MulEq1);
				VVector2.SetOperatorEx(Operators::MulAssign, (uint32_t)Position::Left, "vector2&", "float", &Vector2MulEq2);
				VVector2.SetOperatorEx(Operators::DivAssign, (uint32_t)Position::Left, "vector2&", "const vector2 &in", &Vector2DivEq1);
				VVector2.SetOperatorEx(Operators::DivAssign, (uint32_t)Position::Left, "vector2&", "float", &Vector2DivEq2);
				VVector2.SetOperatorEx(Operators::AddAssign, (uint32_t)Position::Left, "vector2&", "const vector2 &in", &Vector2AddEq1);
				VVector2.SetOperatorEx(Operators::AddAssign, (uint32_t)Position::Left, "vector2&", "float", &Vector2AddEq2);
				VVector2.SetOperatorEx(Operators::SubAssign, (uint32_t)Position::Left, "vector2&", "const vector2 &in", &Vector2SubEq1);
				VVector2.SetOperatorEx(Operators::SubAssign, (uint32_t)Position::Left, "vector2&", "float", &Vector2SubEq2);
				VVector2.SetOperatorEx(Operators::Mul, (uint32_t)Position::Const, "vector2", "const vector2 &in", &Vector2Mul1);
				VVector2.SetOperatorEx(Operators::Mul, (uint32_t)Position::Const, "vector2", "float", &Vector2Mul2);
				VVector2.SetOperatorEx(Operators::Div, (uint32_t)Position::Const, "vector2", "const vector2 &in", &Vector2Div1);
				VVector2.SetOperatorEx(Operators::Div, (uint32_t)Position::Const, "vector2", "float", &Vector2Div2);
				VVector2.SetOperatorEx(Operators::Add, (uint32_t)Position::Const, "vector2", "const vector2 &in", &Vector2Add1);
				VVector2.SetOperatorEx(Operators::Add, (uint32_t)Position::Const, "vector2", "float", &Vector2Add2);
				VVector2.SetOperatorEx(Operators::Sub, (uint32_t)Position::Const, "vector2", "const vector2 &in", &Vector2Sub1);
				VVector2.SetOperatorEx(Operators::Sub, (uint32_t)Position::Const, "vector2", "float", &Vector2Sub2);
				VVector2.SetOperatorEx(Operators::Neg, (uint32_t)Position::Const, "vector2", "", &Vector2Neg);
				VVector2.SetOperatorEx(Operators::Equals, (uint32_t)Position::Const, "bool", "const vector2 &in", &Vector2Eq);
				VVector2.SetOperatorEx(Operators::Cmp, (uint32_t)Position::Const, "int", "const vector2 &in", &Vector2Cmp);

				Engine->BeginNamespace("vector2");
				Engine->SetFunction("vector2 random()", &Compute::Vector2::Random);
				Engine->SetFunction("vector2 random_abs()", &Compute::Vector2::RandomAbs);
				Engine->SetFunction("vector2 one()", &Compute::Vector2::One);
				Engine->SetFunction("vector2 zero()", &Compute::Vector2::Zero);
				Engine->SetFunction("vector2 up()", &Compute::Vector2::Up);
				Engine->SetFunction("vector2 down()", &Compute::Vector2::Down);
				Engine->SetFunction("vector2 left()", &Compute::Vector2::Left);
				Engine->SetFunction("vector2 right()", &Compute::Vector2::Right);
				Engine->EndNamespace();

				TypeClass VVector3 = Engine->SetPod<Compute::Vector3>("vector3");
				VVector3.SetConstructor<Compute::Vector3>("void f()");
				VVector3.SetConstructor<Compute::Vector3, float, float>("void f(float, float)");
				VVector3.SetConstructor<Compute::Vector3, float, float, float>("void f(float, float, float)");
				VVector3.SetConstructor<Compute::Vector3, float>("void f(float)");
				VVector3.SetConstructor<Compute::Vector3, const Compute::Vector2&>("void f(const vector2 &in)");
				VVector3.SetProperty<Compute::Vector3>("float x", &Compute::Vector3::X);
				VVector3.SetProperty<Compute::Vector3>("float y", &Compute::Vector3::Y);
				VVector3.SetProperty<Compute::Vector3>("float z", &Compute::Vector3::Z);
				VVector3.SetMethod("float length() const", &Compute::Vector3::Length);
				VVector3.SetMethod("float sum() const", &Compute::Vector3::Sum);
				VVector3.SetMethod("float dot(const vector3 &in) const", &Compute::Vector3::Dot);
				VVector3.SetMethod("float distance(const vector3 &in) const", &Compute::Vector3::Distance);
				VVector3.SetMethod("float hypotenuse() const", &Compute::Vector3::Hypotenuse);
				VVector3.SetMethod("vector3 look_at(const vector3 &in) const", &Compute::Vector3::LookAt);
				VVector3.SetMethod("vector3 cross(const vector3 &in) const", &Compute::Vector3::Cross);
				VVector3.SetMethod("vector3 direction_horizontal() const", &Compute::Vector3::hDirection);
				VVector3.SetMethod("vector3 direction_depth() const", &Compute::Vector3::dDirection);
				VVector3.SetMethod("vector3 inv() const", &Compute::Vector3::Inv);
				VVector3.SetMethod("vector3 inv_x() const", &Compute::Vector3::InvX);
				VVector3.SetMethod("vector3 inv_y() const", &Compute::Vector3::InvY);
				VVector3.SetMethod("vector3 inv_z() const", &Compute::Vector3::InvZ);
				VVector3.SetMethod("vector3 normalize() const", &Compute::Vector3::Normalize);
				VVector3.SetMethod("vector3 snormalize() const", &Compute::Vector3::sNormalize);
				VVector3.SetMethod("vector3 lerp(const vector3 &in, float) const", &Compute::Vector3::Lerp);
				VVector3.SetMethod("vector3 slerp(const vector3 &in, float) const", &Compute::Vector3::sLerp);
				VVector3.SetMethod("vector3 alerp(const vector3 &in, float) const", &Compute::Vector3::aLerp);
				VVector3.SetMethod("vector3 rlerp() const", &Compute::Vector3::rLerp);
				VVector3.SetMethod("vector3 abs() const", &Compute::Vector3::Abs);
				VVector3.SetMethod("vector3 radians() const", &Compute::Vector3::Radians);
				VVector3.SetMethod("vector3 degrees() const", &Compute::Vector3::Degrees);
				VVector3.SetMethod("vector3 view_space() const", &Compute::Vector3::ViewSpace);
				VVector3.SetMethod("vector2 xy() const", &Compute::Vector3::XY);
				VVector3.SetMethod("vector3 xyz() const", &Compute::Vector3::XYZ);
				VVector3.SetMethod<Compute::Vector3, Compute::Vector3, float>("vector3 mul(float) const", &Compute::Vector3::Mul);
				VVector3.SetMethod<Compute::Vector3, Compute::Vector3, const Compute::Vector2&, float>("vector3 mul(const vector2 &in, float) const", &Compute::Vector3::Mul);
				VVector3.SetMethod<Compute::Vector3, Compute::Vector3, const Compute::Vector3&>("vector3 mul(const vector3 &in) const", &Compute::Vector3::Mul);
				VVector3.SetMethod("vector3 div(const vector3 &in) const", &Compute::Vector3::Div);
				VVector3.SetMethod("vector3 set_x(float) const", &Compute::Vector3::SetX);
				VVector3.SetMethod("vector3 set_y(float) const", &Compute::Vector3::SetY);
				VVector3.SetMethod("vector3 set_z(float) const", &Compute::Vector3::SetZ);
				VVector3.SetMethod("void set(const vector3 &in) const", &Compute::Vector3::Set);
				VVector3.SetOperatorEx(Operators::MulAssign, (uint32_t)Position::Left, "vector3&", "const vector3 &in", &Vector3MulEq1);
				VVector3.SetOperatorEx(Operators::MulAssign, (uint32_t)Position::Left, "vector3&", "float", &Vector3MulEq2);
				VVector3.SetOperatorEx(Operators::DivAssign, (uint32_t)Position::Left, "vector3&", "const vector3 &in", &Vector3DivEq1);
				VVector3.SetOperatorEx(Operators::DivAssign, (uint32_t)Position::Left, "vector3&", "float", &Vector3DivEq2);
				VVector3.SetOperatorEx(Operators::AddAssign, (uint32_t)Position::Left, "vector3&", "const vector3 &in", &Vector3AddEq1);
				VVector3.SetOperatorEx(Operators::AddAssign, (uint32_t)Position::Left, "vector3&", "float", &Vector3AddEq2);
				VVector3.SetOperatorEx(Operators::SubAssign, (uint32_t)Position::Left, "vector3&", "const vector3 &in", &Vector3SubEq1);
				VVector3.SetOperatorEx(Operators::SubAssign, (uint32_t)Position::Left, "vector3&", "float", &Vector3SubEq2);
				VVector3.SetOperatorEx(Operators::Mul, (uint32_t)Position::Const, "vector3", "const vector3 &in", &Vector3Mul1);
				VVector3.SetOperatorEx(Operators::Mul, (uint32_t)Position::Const, "vector3", "float", &Vector3Mul2);
				VVector3.SetOperatorEx(Operators::Div, (uint32_t)Position::Const, "vector3", "const vector3 &in", &Vector3Div1);
				VVector3.SetOperatorEx(Operators::Div, (uint32_t)Position::Const, "vector3", "float", &Vector3Div2);
				VVector3.SetOperatorEx(Operators::Add, (uint32_t)Position::Const, "vector3", "const vector3 &in", &Vector3Add1);
				VVector3.SetOperatorEx(Operators::Add, (uint32_t)Position::Const, "vector3", "float", &Vector3Add2);
				VVector3.SetOperatorEx(Operators::Sub, (uint32_t)Position::Const, "vector3", "const vector3 &in", &Vector3Sub1);
				VVector3.SetOperatorEx(Operators::Sub, (uint32_t)Position::Const, "vector3", "float", &Vector3Sub2);
				VVector3.SetOperatorEx(Operators::Neg, (uint32_t)Position::Const, "vector3", "", &Vector3Neg);
				VVector3.SetOperatorEx(Operators::Equals, (uint32_t)Position::Const, "bool", "const vector3 &in", &Vector3Eq);
				VVector3.SetOperatorEx(Operators::Cmp, (uint32_t)Position::Const, "int", "const vector3 &in", &Vector3Cmp);

				Engine->BeginNamespace("vector3");
				Engine->SetFunction("vector3 random()", &Compute::Vector3::Random);
				Engine->SetFunction("vector3 random_abs()", &Compute::Vector3::RandomAbs);
				Engine->SetFunction("vector3 one()", &Compute::Vector3::One);
				Engine->SetFunction("vector3 zero()", &Compute::Vector3::Zero);
				Engine->SetFunction("vector3 up()", &Compute::Vector3::Up);
				Engine->SetFunction("vector3 down()", &Compute::Vector3::Down);
				Engine->SetFunction("vector3 left()", &Compute::Vector3::Left);
				Engine->SetFunction("vector3 right()", &Compute::Vector3::Right);
				Engine->SetFunction("vector3 forward()", &Compute::Vector3::Forward);
				Engine->SetFunction("vector3 backward()", &Compute::Vector3::Backward);
				Engine->EndNamespace();

				TypeClass VVector4 = Engine->SetPod<Compute::Vector4>("vector4");
				VVector4.SetConstructor<Compute::Vector4>("void f()");
				VVector4.SetConstructor<Compute::Vector4, float, float>("void f(float, float)");
				VVector4.SetConstructor<Compute::Vector4, float, float, float>("void f(float, float, float)");
				VVector4.SetConstructor<Compute::Vector4, float, float, float, float>("void f(float, float, float, float)");
				VVector4.SetConstructor<Compute::Vector4, float>("void f(float)");
				VVector4.SetConstructor<Compute::Vector4, const Compute::Vector2&>("void f(const vector2 &in)");
				VVector4.SetConstructor<Compute::Vector4, const Compute::Vector3&>("void f(const vector3 &in)");
				VVector4.SetProperty<Compute::Vector4>("float x", &Compute::Vector4::X);
				VVector4.SetProperty<Compute::Vector4>("float y", &Compute::Vector4::Y);
				VVector4.SetProperty<Compute::Vector4>("float z", &Compute::Vector4::Z);
				VVector4.SetProperty<Compute::Vector4>("float w", &Compute::Vector4::W);
				VVector4.SetMethod("float length() const", &Compute::Vector4::Length);
				VVector4.SetMethod("float sum() const", &Compute::Vector4::Sum);
				VVector4.SetMethod("float dot(const vector4 &in) const", &Compute::Vector4::Dot);
				VVector4.SetMethod("float distance(const vector4 &in) const", &Compute::Vector4::Distance);
				VVector4.SetMethod("float cross(const vector4 &in) const", &Compute::Vector4::Cross);
				VVector4.SetMethod("vector4 inv() const", &Compute::Vector4::Inv);
				VVector4.SetMethod("vector4 inv_x() const", &Compute::Vector4::InvX);
				VVector4.SetMethod("vector4 inv_y() const", &Compute::Vector4::InvY);
				VVector4.SetMethod("vector4 inv_z() const", &Compute::Vector4::InvZ);
				VVector4.SetMethod("vector4 inv_w() const", &Compute::Vector4::InvW);
				VVector4.SetMethod("vector4 normalize() const", &Compute::Vector4::Normalize);
				VVector4.SetMethod("vector4 snormalize() const", &Compute::Vector4::sNormalize);
				VVector4.SetMethod("vector4 lerp(const vector4 &in, float) const", &Compute::Vector4::Lerp);
				VVector4.SetMethod("vector4 slerp(const vector4 &in, float) const", &Compute::Vector4::sLerp);
				VVector4.SetMethod("vector4 alerp(const vector4 &in, float) const", &Compute::Vector4::aLerp);
				VVector4.SetMethod("vector4 rlerp() const", &Compute::Vector4::rLerp);
				VVector4.SetMethod("vector4 abs() const", &Compute::Vector4::Abs);
				VVector4.SetMethod("vector4 radians() const", &Compute::Vector4::Radians);
				VVector4.SetMethod("vector4 degrees() const", &Compute::Vector4::Degrees);
				VVector4.SetMethod("vector4 view_space() const", &Compute::Vector4::ViewSpace);
				VVector4.SetMethod("vector2 xy() const", &Compute::Vector4::XY);
				VVector4.SetMethod("vector3 xyz() const", &Compute::Vector4::XYZ);
				VVector4.SetMethod("vector4 xyzw() const", &Compute::Vector4::XYZW);
				VVector4.SetMethod<Compute::Vector4, Compute::Vector4, float>("vector4 mul(float) const", &Compute::Vector4::Mul);
				VVector4.SetMethod<Compute::Vector4, Compute::Vector4, const Compute::Vector2&, float, float>("vector4 mul(const vector2 &in, float, float) const", &Compute::Vector4::Mul);
				VVector4.SetMethod<Compute::Vector4, Compute::Vector4, const Compute::Vector3&, float>("vector4 mul(const vector3 &in, float) const", &Compute::Vector4::Mul);
				VVector4.SetMethod<Compute::Vector4, Compute::Vector4, const Compute::Vector4&>("vector4 mul(const vector4 &in) const", &Compute::Vector4::Mul);
				VVector4.SetMethod("vector4 div(const vector4 &in) const", &Compute::Vector4::Div);
				VVector4.SetMethod("vector4 set_x(float) const", &Compute::Vector4::SetX);
				VVector4.SetMethod("vector4 set_y(float) const", &Compute::Vector4::SetY);
				VVector4.SetMethod("vector4 set_z(float) const", &Compute::Vector4::SetZ);
				VVector4.SetMethod("vector4 set_w(float) const", &Compute::Vector4::SetW);
				VVector4.SetMethod("void set(const vector4 &in) const", &Compute::Vector4::Set);
				VVector4.SetOperatorEx(Operators::MulAssign, (uint32_t)Position::Left, "vector4&", "const vector4 &in", &Vector4MulEq1);
				VVector4.SetOperatorEx(Operators::MulAssign, (uint32_t)Position::Left, "vector4&", "float", &Vector4MulEq2);
				VVector4.SetOperatorEx(Operators::DivAssign, (uint32_t)Position::Left, "vector4&", "const vector4 &in", &Vector4DivEq1);
				VVector4.SetOperatorEx(Operators::DivAssign, (uint32_t)Position::Left, "vector4&", "float", &Vector4DivEq2);
				VVector4.SetOperatorEx(Operators::AddAssign, (uint32_t)Position::Left, "vector4&", "const vector4 &in", &Vector4AddEq1);
				VVector4.SetOperatorEx(Operators::AddAssign, (uint32_t)Position::Left, "vector4&", "float", &Vector4AddEq2);
				VVector4.SetOperatorEx(Operators::SubAssign, (uint32_t)Position::Left, "vector4&", "const vector4 &in", &Vector4SubEq1);
				VVector4.SetOperatorEx(Operators::SubAssign, (uint32_t)Position::Left, "vector4&", "float", &Vector4SubEq2);
				VVector4.SetOperatorEx(Operators::Mul, (uint32_t)Position::Const, "vector4", "const vector4 &in", &Vector4Mul1);
				VVector4.SetOperatorEx(Operators::Mul, (uint32_t)Position::Const, "vector4", "float", &Vector4Mul2);
				VVector4.SetOperatorEx(Operators::Div, (uint32_t)Position::Const, "vector4", "const vector4 &in", &Vector4Div1);
				VVector4.SetOperatorEx(Operators::Div, (uint32_t)Position::Const, "vector4", "float", &Vector4Div2);
				VVector4.SetOperatorEx(Operators::Add, (uint32_t)Position::Const, "vector4", "const vector4 &in", &Vector4Add1);
				VVector4.SetOperatorEx(Operators::Add, (uint32_t)Position::Const, "vector4", "float", &Vector4Add2);
				VVector4.SetOperatorEx(Operators::Sub, (uint32_t)Position::Const, "vector4", "const vector4 &in", &Vector4Sub1);
				VVector4.SetOperatorEx(Operators::Sub, (uint32_t)Position::Const, "vector4", "float", &Vector4Sub2);
				VVector4.SetOperatorEx(Operators::Neg, (uint32_t)Position::Const, "vector4", "", &Vector4Neg);
				VVector4.SetOperatorEx(Operators::Equals, (uint32_t)Position::Const, "bool", "const vector4 &in", &Vector4Eq);
				VVector4.SetOperatorEx(Operators::Cmp, (uint32_t)Position::Const, "int", "const vector4 &in", &Vector4Cmp);

				Engine->BeginNamespace("vector4");
				Engine->SetFunction("vector4 random()", &Compute::Vector4::Random);
				Engine->SetFunction("vector4 random_abs()", &Compute::Vector4::RandomAbs);
				Engine->SetFunction("vector4 one()", &Compute::Vector4::One);
				Engine->SetFunction("vector4 zero()", &Compute::Vector4::Zero);
				Engine->SetFunction("vector4 up()", &Compute::Vector4::Up);
				Engine->SetFunction("vector4 down()", &Compute::Vector4::Down);
				Engine->SetFunction("vector4 left()", &Compute::Vector4::Left);
				Engine->SetFunction("vector4 right()", &Compute::Vector4::Right);
				Engine->SetFunction("vector4 forward()", &Compute::Vector4::Forward);
				Engine->SetFunction("vector4 backward()", &Compute::Vector4::Backward);
				Engine->SetFunction("vector4 unitW()", &Compute::Vector4::UnitW);
				Engine->EndNamespace();

				TypeClass VMatrix4x4 = Engine->SetPod<Compute::Matrix4x4>("matrix4x4");
				VMatrix4x4.SetConstructor<Compute::Matrix4x4>("void f()");
				VMatrix4x4.SetConstructor<Compute::Matrix4x4, const Compute::Vector4&, const Compute::Vector4&, const Compute::Vector4&, const Compute::Vector4&>("void f(const vector4 &in, const vector4 &in, const vector4 &in, const vector4 &in)");
				VMatrix4x4.SetConstructor<Compute::Matrix4x4, float, float, float, float, float, float, float, float, float, float, float, float, float, float, float, float>("void f(float, float, float, float, float, float, float, float, float, float, float, float, float, float, float, float)");
				VMatrix4x4.SetMethod("matrix4x4 inv() const", &Compute::Matrix4x4::Inv);
				VMatrix4x4.SetMethod("matrix4x4 transpose() const", &Compute::Matrix4x4::Transpose);
				VMatrix4x4.SetMethod("matrix4x4 set_scale(const vector3 &in) const", &Compute::Matrix4x4::SetScale);
				VMatrix4x4.SetMethod("vector4 row11() const", &Compute::Matrix4x4::Row11);
				VMatrix4x4.SetMethod("vector4 row22() const", &Compute::Matrix4x4::Row22);
				VMatrix4x4.SetMethod("vector4 row33() const", &Compute::Matrix4x4::Row33);
				VMatrix4x4.SetMethod("vector4 row44() const", &Compute::Matrix4x4::Row44);
				VMatrix4x4.SetMethod("vector3 up() const", &Compute::Matrix4x4::Up);
				VMatrix4x4.SetMethod("vector3 right() const", &Compute::Matrix4x4::Right);
				VMatrix4x4.SetMethod("vector3 forward() const", &Compute::Matrix4x4::Forward);
				VMatrix4x4.SetMethod("vector3 rotation_quaternion() const", &Compute::Matrix4x4::RotationQuaternion);
				VMatrix4x4.SetMethod("vector3 rotation_euler() const", &Compute::Matrix4x4::RotationEuler);
				VMatrix4x4.SetMethod("vector3 position() const", &Compute::Matrix4x4::Position);
				VMatrix4x4.SetMethod("vector3 scale() const", &Compute::Matrix4x4::Scale);
				VMatrix4x4.SetMethod("vector3 xy() const", &Compute::Matrix4x4::XY);
				VMatrix4x4.SetMethod("vector3 xyz() const", &Compute::Matrix4x4::XYZ);
				VMatrix4x4.SetMethod("vector3 xyzw() const", &Compute::Matrix4x4::XYZW);
				VMatrix4x4.SetMethod("void identify()", &Compute::Matrix4x4::Identify);
				VMatrix4x4.SetMethod("void set(const matrix4x4 &in)", &Compute::Matrix4x4::Set);
				VMatrix4x4.SetMethod<Compute::Matrix4x4, Compute::Matrix4x4, const Compute::Matrix4x4&>("matrix4x4 mul(const matrix4x4 &in) const", &Compute::Matrix4x4::Mul);
				VMatrix4x4.SetMethod<Compute::Matrix4x4, Compute::Matrix4x4, const Compute::Vector4&>("matrix4x4 mul(const vector4 &in) const", &Compute::Matrix4x4::Mul);
				VMatrix4x4.SetOperatorEx(Operators::Index, (uint32_t)Position::Left, "float&", "usize", &Matrix4x4GetRow);
				VMatrix4x4.SetOperatorEx(Operators::Index, (uint32_t)Position::Const, "const float&", "usize", &Matrix4x4GetRow);
				VMatrix4x4.SetOperatorEx(Operators::Equals, (uint32_t)Position::Const, "bool", "const matrix4x4 &in", &Matrix4x4Eq);
				VMatrix4x4.SetOperatorEx(Operators::Mul, (uint32_t)Position::Const, "matrix4x4", "const matrix4x4 &in", &Matrix4x4Mul1);
				VMatrix4x4.SetOperatorEx(Operators::Mul, (uint32_t)Position::Const, "vector4", "const vector4 &in", &Matrix4x4Mul2);

				Engine->BeginNamespace("matrix4x4");
				Engine->SetFunction("matrix4x4 identity()", &Compute::Matrix4x4::Identity);
				Engine->SetFunction("matrix4x4 create_rotation_x(float)", &Compute::Matrix4x4::CreateRotationX);
				Engine->SetFunction("matrix4x4 create_rotation_y(float)", &Compute::Matrix4x4::CreateRotationY);
				Engine->SetFunction("matrix4x4 create_rotation_z(float)", &Compute::Matrix4x4::CreateRotationZ);
				Engine->SetFunction("matrix4x4 create_view(const vector3 &in, const vector3 &in)", &Compute::Matrix4x4::CreateView);
				Engine->SetFunction("matrix4x4 create_scale(const vector3 &in)", &Compute::Matrix4x4::CreateScale);
				Engine->SetFunction("matrix4x4 create_translated_scale(const vector3& in, const vector3 &in)", &Compute::Matrix4x4::CreateTranslatedScale);
				Engine->SetFunction("matrix4x4 create_translation(const vector3& in)", &Compute::Matrix4x4::CreateTranslation);
				Engine->SetFunction("matrix4x4 create_perspective(float, float, float, float)", &Compute::Matrix4x4::CreatePerspective);
				Engine->SetFunction("matrix4x4 create_perspective_rad(float, float, float, float)", &Compute::Matrix4x4::CreatePerspectiveRad);
				Engine->SetFunction("matrix4x4 create_orthographic(float, float, float, float)", &Compute::Matrix4x4::CreateOrthographic);
				Engine->SetFunction("matrix4x4 create_orthographic_off_center(float, float, float, float, float, float)", &Compute::Matrix4x4::CreateOrthographicOffCenter);
				Engine->SetFunction<Compute::Matrix4x4(const Compute::Vector3&)>("matrix4x4 createRotation(const vector3 &in)", &Compute::Matrix4x4::CreateRotation);
				Engine->SetFunction<Compute::Matrix4x4(const Compute::Vector3&, const Compute::Vector3&, const Compute::Vector3&)>("matrix4x4 create_rotation(const vector3 &in, const vector3 &in, const vector3 &in)", &Compute::Matrix4x4::CreateRotation);
				Engine->SetFunction<Compute::Matrix4x4(const Compute::Vector3&, const Compute::Vector3&, const Compute::Vector3&)>("matrix4x4 create_look_at(const vector3 &in, const vector3 &in, const vector3 &in)", &Compute::Matrix4x4::CreateLookAt);
				Engine->SetFunction<Compute::Matrix4x4(Compute::CubeFace, const Compute::Vector3&)>("matrix4x4 create_look_at(cube_face, const vector3 &in)", &Compute::Matrix4x4::CreateLookAt);
				Engine->SetFunction<Compute::Matrix4x4(const Compute::Vector3&, const Compute::Vector3&, const Compute::Vector3&)>("matrix4x4 create(const vector3 &in, const vector3 &in, const vector3 &in)", &Compute::Matrix4x4::Create);
				Engine->SetFunction<Compute::Matrix4x4(const Compute::Vector3&, const Compute::Vector3&)>("matrix4x4 create(const vector3 &in, const vector3 &in)", &Compute::Matrix4x4::Create);
				Engine->EndNamespace();

				TypeClass VQuaternion = Engine->SetPod<Compute::Vector4>("quaternion");
				VQuaternion.SetConstructor<Compute::Quaternion>("void f()");
				VQuaternion.SetConstructor<Compute::Quaternion, float, float, float, float>("void f(float, float, float, float)");
				VQuaternion.SetConstructor<Compute::Quaternion, const Compute::Vector3&, float>("void f(const vector3 &in, float)");
				VQuaternion.SetConstructor<Compute::Quaternion, const Compute::Vector3&>("void f(const vector3 &in)");
				VQuaternion.SetConstructor<Compute::Quaternion, const Compute::Matrix4x4&>("void f(const matrix4x4 &in)");
				VQuaternion.SetProperty<Compute::Quaternion>("float x", &Compute::Quaternion::X);
				VQuaternion.SetProperty<Compute::Quaternion>("float y", &Compute::Quaternion::Y);
				VQuaternion.SetProperty<Compute::Quaternion>("float z", &Compute::Quaternion::Z);
				VQuaternion.SetProperty<Compute::Quaternion>("float w", &Compute::Quaternion::W);
				VQuaternion.SetMethod("void set_axis(const vector3 &in, float)", &Compute::Quaternion::SetAxis);
				VQuaternion.SetMethod("void set_euler(const vector3 &in)", &Compute::Quaternion::SetEuler);
				VQuaternion.SetMethod("void set_matrix(const matrix4x4 &in)", &Compute::Quaternion::SetMatrix);
				VQuaternion.SetMethod("void set(const quaternion &in)", &Compute::Quaternion::Set);
				VQuaternion.SetMethod("quaternion normalize() const", &Compute::Quaternion::Normalize);
				VQuaternion.SetMethod("quaternion snormalize() const", &Compute::Quaternion::sNormalize);
				VQuaternion.SetMethod("quaternion conjugate() const", &Compute::Quaternion::Conjugate);
				VQuaternion.SetMethod("quaternion sub(const quaternion &in) const", &Compute::Quaternion::Sub);
				VQuaternion.SetMethod("quaternion add(const quaternion &in) const", &Compute::Quaternion::Add);
				VQuaternion.SetMethod("quaternion lerp(const quaternion &in, float) const", &Compute::Quaternion::Lerp);
				VQuaternion.SetMethod("quaternion slerp(const quaternion &in, float) const", &Compute::Quaternion::sLerp);
				VQuaternion.SetMethod("vector3 forward() const", &Compute::Quaternion::Forward);
				VQuaternion.SetMethod("vector3 up() const", &Compute::Quaternion::Up);
				VQuaternion.SetMethod("vector3 right() const", &Compute::Quaternion::Right);
				VQuaternion.SetMethod("matrix4x4 get_matrix() const", &Compute::Quaternion::GetMatrix);
				VQuaternion.SetMethod("vector3 get_euler() const", &Compute::Quaternion::GetEuler);
				VQuaternion.SetMethod("float dot(const quaternion &in) const", &Compute::Quaternion::Dot);
				VQuaternion.SetMethod("float length() const", &Compute::Quaternion::Length);
				VQuaternion.SetMethod<Compute::Quaternion, Compute::Quaternion, float>("quaternion mul(float) const", &Compute::Quaternion::Mul);
				VQuaternion.SetMethod<Compute::Quaternion, Compute::Quaternion, const Compute::Quaternion&>("quaternion mul(const quaternion &in) const", &Compute::Quaternion::Mul);
				VQuaternion.SetMethod<Compute::Quaternion, Compute::Vector3, const Compute::Vector3&>("vector3 mul(const vector3 &in) const", &Compute::Quaternion::Mul);
				VQuaternion.SetOperatorEx(Operators::Mul, (uint32_t)Position::Const, "vector3", "const vector3 &in", &QuaternionMul1);
				VQuaternion.SetOperatorEx(Operators::Mul, (uint32_t)Position::Const, "quaternion", "const quaternion &in", &QuaternionMul2);
				VQuaternion.SetOperatorEx(Operators::Mul, (uint32_t)Position::Const, "quaternion", "float", &QuaternionMul3);
				VQuaternion.SetOperatorEx(Operators::Add, (uint32_t)Position::Const, "quaternion", "const quaternion &in", &QuaternionAdd);
				VQuaternion.SetOperatorEx(Operators::Sub, (uint32_t)Position::Const, "quaternion", "const quaternion &in", &QuaternionSub);

				Engine->BeginNamespace("quaternion");
				Engine->SetFunction("quaternion create_euler_rotation(const vector3 &in)", &Compute::Quaternion::CreateEulerRotation);
				Engine->SetFunction("quaternion create_rotation(const matrix4x4 &in)", &Compute::Quaternion::CreateRotation);
				Engine->EndNamespace();

				return true;
#else
				VI_ASSERT(false, false, "<vectors> is not loaded");
				return false;
#endif
			}
			bool Registry::LoadShapes(VirtualMachine* Engine)
			{
#ifdef VI_HAS_BINDINGS
				VI_ASSERT(Engine != nullptr, false, "manager should be set");
				TypeClass VRectangle = Engine->SetPod<Compute::Rectangle>("rectangle");
				VRectangle.SetProperty<Compute::Rectangle>("int64 left", &Compute::Rectangle::Left);
				VRectangle.SetProperty<Compute::Rectangle>("int64 top", &Compute::Rectangle::Top);
				VRectangle.SetProperty<Compute::Rectangle>("int64 right", &Compute::Rectangle::Right);
				VRectangle.SetProperty<Compute::Rectangle>("int64 bottom", &Compute::Rectangle::Bottom);
				VRectangle.SetConstructor<Compute::Rectangle>("void f()");

				TypeClass VBounding = Engine->SetPod<Compute::Bounding>("bounding");
				VBounding.SetProperty<Compute::Bounding>("vector3 lower", &Compute::Bounding::Lower);
				VBounding.SetProperty<Compute::Bounding>("vector3 upper", &Compute::Bounding::Upper);
				VBounding.SetProperty<Compute::Bounding>("vector3 center", &Compute::Bounding::Center);
				VBounding.SetProperty<Compute::Bounding>("float radius", &Compute::Bounding::Radius);
				VBounding.SetProperty<Compute::Bounding>("float volume", &Compute::Bounding::Volume);
				VBounding.SetConstructor<Compute::Bounding>("void f()");
				VBounding.SetConstructor<Compute::Bounding, const Compute::Vector3&, const Compute::Vector3&>("void f(const vector3 &in, const vector3 &in)");
				VBounding.SetMethod("void merge(const bounding &in, const bounding &in)", &Compute::Bounding::Merge);
				VBounding.SetMethod("bool contains(const bounding &in) const", &Compute::Bounding::Contains);
				VBounding.SetMethod("bool overlaps(const bounding &in) const", &Compute::Bounding::Overlaps);

				TypeClass VRay = Engine->SetPod<Compute::Ray>("ray");
				VRay.SetProperty<Compute::Ray>("vector3 origin", &Compute::Ray::Origin);
				VRay.SetProperty<Compute::Ray>("vector3 direction", &Compute::Ray::Direction);
				VRay.SetConstructor<Compute::Ray>("void f()");
				VRay.SetConstructor<Compute::Ray, const Compute::Vector3&, const Compute::Vector3&>("void f(const vector3 &in, const vector3 &in)");
				VRay.SetMethod("vector3 get_point(float) const", &Compute::Ray::GetPoint);
				VRay.SetMethod("bool intersects_plane(const vector3 &in, float) const", &Compute::Ray::IntersectsPlane);
				VRay.SetMethod("bool intersects_sphere(const vector3 &in, float, bool = true) const", &Compute::Ray::IntersectsSphere);
				VRay.SetMethod("bool intersects_aabb_at(const vector3 &in, const vector3 &in, vector3 &out) const", &Compute::Ray::IntersectsAABBAt);
				VRay.SetMethod("bool intersects_aabb(const vector3 &in, const vector3 &in, vector3 &out) const", &Compute::Ray::IntersectsAABB);
				VRay.SetMethod("bool intersects_obb(const matrix4x4 &in, vector3 &out) const", &Compute::Ray::IntersectsOBB);

				TypeClass VFrustum8C = Engine->SetPod<Compute::Frustum8C>("frustum_8c");
				VFrustum8C.SetConstructor<Compute::Frustum8C>("void f()");
				VFrustum8C.SetConstructor<Compute::Frustum8C, float, float, float, float>("void f(float, float, float, float)");
				VFrustum8C.SetMethod("void transform(const matrix4x4 &in) const", &Compute::Frustum8C::Transform);
				VFrustum8C.SetMethod("void get_bounding_box(vector2 &out, vector2 &out, vector2 &out) const", &Compute::Frustum8C::GetBoundingBox);
				VFrustum8C.SetOperatorEx(Operators::Index, (uint32_t)Position::Left, "vector4&", "usize", &Frustum8CGetCorners);
				VFrustum8C.SetOperatorEx(Operators::Index, (uint32_t)Position::Const, "const vector4&", "usize", &Frustum8CGetCorners);

				TypeClass VFrustum6P = Engine->SetPod<Compute::Frustum6P>("frustum_6p");
				VFrustum6P.SetConstructor<Compute::Frustum6P>("void f()");
				VFrustum6P.SetConstructor<Compute::Frustum6P, const Compute::Matrix4x4&>("void f(const matrix4x4 &in)");
				VFrustum6P.SetMethod("bool overlaps_aabb(const bounding &in) const", &Compute::Frustum6P::OverlapsAABB);
				VFrustum6P.SetMethod("bool overlaps_sphere(const vector3 &in, float) const", &Compute::Frustum6P::OverlapsSphere);
				VFrustum6P.SetOperatorEx(Operators::Index, (uint32_t)Position::Left, "vector4&", "usize", &Frustum6PGetCorners);
				VFrustum6P.SetOperatorEx(Operators::Index, (uint32_t)Position::Const, "const vector4&", "usize", &Frustum6PGetCorners);

				return true;
#else
				VI_ASSERT(false, false, "<shapes> is not loaded");
				return false;
#endif
			}
			bool Registry::LoadKeyFrames(VirtualMachine* Engine)
			{
#ifdef VI_HAS_BINDINGS
				VI_ASSERT(Engine != nullptr, false, "manager should be set");
				TypeClass VJoint = Engine->SetStructTrivial<Compute::Joint>("joint");
				VJoint.SetProperty<Compute::Joint>("string name", &Compute::Joint::Name);
				VJoint.SetProperty<Compute::Joint>("matrix4x4 global", &Compute::Joint::Global);
				VJoint.SetProperty<Compute::Joint>("matrix4x4 local", &Compute::Joint::Local);
				VJoint.SetProperty<Compute::Joint>("usize index", &Compute::Joint::Index);
				VJoint.SetConstructor<Compute::Joint>("void f()");
				VJoint.SetMethodEx("usize size() const", &JointSize);
				VJoint.SetOperatorEx(Operators::Index, (uint32_t)Position::Left, "joint&", "usize", &JointGetChilds);
				VJoint.SetOperatorEx(Operators::Index, (uint32_t)Position::Const, "const joint&", "usize", &JointGetChilds);

				TypeClass VAnimatorKey = Engine->SetPod<Compute::AnimatorKey>("animator_key");
				VAnimatorKey.SetProperty<Compute::AnimatorKey>("vector3 position", &Compute::AnimatorKey::Position);
				VAnimatorKey.SetProperty<Compute::AnimatorKey>("quaternion rotation", &Compute::AnimatorKey::Rotation);
				VAnimatorKey.SetProperty<Compute::AnimatorKey>("vector3 scale", &Compute::AnimatorKey::Scale);
				VAnimatorKey.SetProperty<Compute::AnimatorKey>("float time", &Compute::AnimatorKey::Time);
				VAnimatorKey.SetConstructor<Compute::AnimatorKey>("void f()");

				TypeClass VSkinAnimatorKey = Engine->SetStructTrivial<Compute::SkinAnimatorKey>("skin_animator_key");
				VSkinAnimatorKey.SetProperty<Compute::SkinAnimatorKey>("float time", &Compute::SkinAnimatorKey::Time);
				VSkinAnimatorKey.SetConstructor<Compute::AnimatorKey>("void f()");
				VSkinAnimatorKey.SetMethodEx("usize size() const", &SkinAnimatorKeySize);
				VSkinAnimatorKey.SetOperatorEx(Operators::Index, (uint32_t)Position::Left, "animator_key&", "usize", &SkinAnimatorKeyGetPose);
				VSkinAnimatorKey.SetOperatorEx(Operators::Index, (uint32_t)Position::Const, "const animator_key&", "usize", &SkinAnimatorKeyGetPose);

				TypeClass VSkinAnimatorClip = Engine->SetStructTrivial<Compute::SkinAnimatorClip>("skin_animator_clip");
				VSkinAnimatorClip.SetProperty<Compute::SkinAnimatorClip>("string name", &Compute::SkinAnimatorClip::Name);
				VSkinAnimatorClip.SetProperty<Compute::SkinAnimatorClip>("float duration", &Compute::SkinAnimatorClip::Duration);
				VSkinAnimatorClip.SetProperty<Compute::SkinAnimatorClip>("float rate", &Compute::SkinAnimatorClip::Rate);
				VSkinAnimatorClip.SetConstructor<Compute::SkinAnimatorClip>("void f()");
				VSkinAnimatorClip.SetMethodEx("usize size() const", &SkinAnimatorClipSize);
				VSkinAnimatorClip.SetOperatorEx(Operators::Index, (uint32_t)Position::Left, "skin_animator_key&", "usize", &SkinAnimatorClipGetKeys);
				VSkinAnimatorClip.SetOperatorEx(Operators::Index, (uint32_t)Position::Const, "const skin_animator_key&", "usize", &SkinAnimatorClipGetKeys);

				TypeClass VKeyAnimatorClip = Engine->SetStructTrivial<Compute::KeyAnimatorClip>("key_animator_clip");
				VKeyAnimatorClip.SetProperty<Compute::KeyAnimatorClip>("string name", &Compute::KeyAnimatorClip::Name);
				VKeyAnimatorClip.SetProperty<Compute::KeyAnimatorClip>("float duration", &Compute::KeyAnimatorClip::Duration);
				VKeyAnimatorClip.SetProperty<Compute::KeyAnimatorClip>("float rate", &Compute::KeyAnimatorClip::Rate);
				VKeyAnimatorClip.SetConstructor<Compute::KeyAnimatorClip>("void f()");
				VKeyAnimatorClip.SetMethodEx("usize size() const", &KeyAnimatorClipSize);
				VKeyAnimatorClip.SetOperatorEx(Operators::Index, (uint32_t)Position::Left, "animator_key&", "usize", &KeyAnimatorClipGetKeys);
				VKeyAnimatorClip.SetOperatorEx(Operators::Index, (uint32_t)Position::Const, "const animator_key&", "usize", &KeyAnimatorClipGetKeys);

				TypeClass VRandomVector2 = Engine->SetPod<Compute::RandomVector2>("random_vector2");
				VRandomVector2.SetProperty<Compute::RandomVector2>("vector2 min", &Compute::RandomVector2::Min);
				VRandomVector2.SetProperty<Compute::RandomVector2>("vector2 max", &Compute::RandomVector2::Max);
				VRandomVector2.SetProperty<Compute::RandomVector2>("bool intensity", &Compute::RandomVector2::Intensity);
				VRandomVector2.SetProperty<Compute::RandomVector2>("float accuracy", &Compute::RandomVector2::Accuracy);
				VRandomVector2.SetConstructor<Compute::RandomVector2>("void f()");
				VRandomVector2.SetConstructor<Compute::RandomVector2, const Compute::Vector2&, const Compute::Vector2&, bool, float>("void f(const vector2 &in, const vector2 &in, bool, float)");
				VRandomVector2.SetMethod("vector2 generate()", &Compute::RandomVector2::Generate);

				TypeClass VRandomVector3 = Engine->SetPod<Compute::RandomVector3>("random_vector3");
				VRandomVector3.SetProperty<Compute::RandomVector3>("vector3 min", &Compute::RandomVector3::Min);
				VRandomVector3.SetProperty<Compute::RandomVector3>("vector3 max", &Compute::RandomVector3::Max);
				VRandomVector3.SetProperty<Compute::RandomVector3>("bool intensity", &Compute::RandomVector3::Intensity);
				VRandomVector3.SetProperty<Compute::RandomVector3>("float accuracy", &Compute::RandomVector3::Accuracy);
				VRandomVector3.SetConstructor<Compute::RandomVector3>("void f()");
				VRandomVector3.SetConstructor<Compute::RandomVector3, const Compute::Vector3&, const Compute::Vector3&, bool, float>("void f(const vector3 &in, const vector3 &in, bool, float)");
				VRandomVector3.SetMethod("vector3 generate()", &Compute::RandomVector3::Generate);

				TypeClass VRandomVector4 = Engine->SetPod<Compute::RandomVector4>("random_vector4");
				VRandomVector4.SetProperty<Compute::RandomVector4>("vector4 min", &Compute::RandomVector4::Min);
				VRandomVector4.SetProperty<Compute::RandomVector4>("vector4 max", &Compute::RandomVector4::Max);
				VRandomVector4.SetProperty<Compute::RandomVector4>("bool intensity", &Compute::RandomVector4::Intensity);
				VRandomVector4.SetProperty<Compute::RandomVector4>("float accuracy", &Compute::RandomVector4::Accuracy);
				VRandomVector4.SetConstructor<Compute::RandomVector4>("void f()");
				VRandomVector4.SetConstructor<Compute::RandomVector4, const Compute::Vector4&, const Compute::Vector4&, bool, float>("void f(const vector4 &in, const vector4 &in, bool, float)");
				VRandomVector4.SetMethod("vector4 generate()", &Compute::RandomVector4::Generate);

				TypeClass VRandomFloat = Engine->SetPod<Compute::RandomFloat>("random_float");
				VRandomFloat.SetProperty<Compute::RandomFloat>("float min", &Compute::RandomFloat::Min);
				VRandomFloat.SetProperty<Compute::RandomFloat>("float max", &Compute::RandomFloat::Max);
				VRandomFloat.SetProperty<Compute::RandomFloat>("bool intensity", &Compute::RandomFloat::Intensity);
				VRandomFloat.SetProperty<Compute::RandomFloat>("float accuracy", &Compute::RandomFloat::Accuracy);
				VRandomFloat.SetConstructor<Compute::RandomFloat>("void f()");
				VRandomFloat.SetConstructor<Compute::RandomFloat, float, float, bool, float>("void f(float, float, bool, float)");
				VRandomFloat.SetMethod("float generate()", &Compute::RandomFloat::Generate);

				return true;
#else
				VI_ASSERT(false, false, "<key_frames> is not loaded");
				return false;
#endif
			}
			bool Registry::LoadRegex(VirtualMachine* Engine)
			{
#ifdef VI_HAS_BINDINGS
				VI_ASSERT(Engine != nullptr, false, "manager should be set");
				Enumeration VRegexState = Engine->SetEnum("regex_state");
				VRegexState.SetValue("preprocessed", (int)Compute::RegexState::Preprocessed);
				VRegexState.SetValue("match_found", (int)Compute::RegexState::Match_Found);
				VRegexState.SetValue("no_match", (int)Compute::RegexState::No_Match);
				VRegexState.SetValue("unexpected_quantifier", (int)Compute::RegexState::Unexpected_Quantifier);
				VRegexState.SetValue("unbalanced_brackets", (int)Compute::RegexState::Unbalanced_Brackets);
				VRegexState.SetValue("internal_error", (int)Compute::RegexState::Internal_Error);
				VRegexState.SetValue("invalid_character_set", (int)Compute::RegexState::Invalid_Character_Set);
				VRegexState.SetValue("invalid_metacharacter", (int)Compute::RegexState::Invalid_Metacharacter);
				VRegexState.SetValue("sumatch_array_too_small", (int)Compute::RegexState::Sumatch_Array_Too_Small);
				VRegexState.SetValue("too_many_branches", (int)Compute::RegexState::Too_Many_Branches);
				VRegexState.SetValue("too_many_brackets", (int)Compute::RegexState::Too_Many_Brackets);

				TypeClass VRegexMatch = Engine->SetPod<Compute::RegexMatch>("regex_match");
				VRegexMatch.SetProperty<Compute::RegexMatch>("int64 start", &Compute::RegexMatch::Start);
				VRegexMatch.SetProperty<Compute::RegexMatch>("int64 end", &Compute::RegexMatch::End);
				VRegexMatch.SetProperty<Compute::RegexMatch>("int64 length", &Compute::RegexMatch::Length);
				VRegexMatch.SetProperty<Compute::RegexMatch>("int64 steps", &Compute::RegexMatch::Steps);
				VRegexMatch.SetConstructor<Compute::RegexMatch>("void f()");
				VRegexMatch.SetMethodEx("string target() const", &RegexMatchTarget);

				TypeClass VRegexResult = Engine->SetStructTrivial<Compute::RegexResult>("regex_result");
				VRegexResult.SetConstructor<Compute::RegexResult>("void f()");
				VRegexResult.SetMethod("bool empty() const", &Compute::RegexResult::Empty);
				VRegexResult.SetMethod("int64 get_steps() const", &Compute::RegexResult::GetSteps);
				VRegexResult.SetMethod("regex_state get_state() const", &Compute::RegexResult::GetState);
				VRegexResult.SetMethodEx("array<regex_match>@ get() const", &RegexResultGet);
				VRegexResult.SetMethodEx("array<string>@ to_array() const", &RegexResultToArray);

				TypeClass VRegexSource = Engine->SetStructTrivial<Compute::RegexSource>("regex_source");
				VRegexSource.SetProperty<Compute::RegexSource>("bool ignoreCase", &Compute::RegexSource::IgnoreCase);
				VRegexSource.SetConstructor<Compute::RegexSource>("void f()");
				VRegexSource.SetConstructor<Compute::RegexSource, const Core::String&, bool, int64_t, int64_t, int64_t>("void f(const string &in, bool = false, int64 = -1, int64 = -1, int64 = -1)");
				VRegexSource.SetMethod("const string& get_regex() const", &Compute::RegexSource::GetRegex);
				VRegexSource.SetMethod("int64 get_max_branches() const", &Compute::RegexSource::GetMaxBranches);
				VRegexSource.SetMethod("int64 get_max_brackets() const", &Compute::RegexSource::GetMaxBrackets);
				VRegexSource.SetMethod("int64 get_max_matches() const", &Compute::RegexSource::GetMaxMatches);
				VRegexSource.SetMethod("int64 get_complexity() const", &Compute::RegexSource::GetComplexity);
				VRegexSource.SetMethod("regex_state getState() const", &Compute::RegexSource::GetState);
				VRegexSource.SetMethod("bool is_simple() const", &Compute::RegexSource::IsSimple);
				VRegexSource.SetMethodEx("regex_result match(const string &in) const", &RegexSourceMatch);
				VRegexSource.SetMethodEx("string replace(const string &in, const string &in) const", &RegexSourceReplace);

				return true;
#else
				VI_ASSERT(false, false, "<regex> is not loaded");
				return false;
#endif
			}
			bool Registry::LoadCrypto(VirtualMachine* Engine)
			{
#ifdef VI_HAS_BINDINGS
				VI_ASSERT(Engine != nullptr, false, "manager should be set");
				VI_TYPEREF(WebToken, "web_token");

				Enumeration VCompression = Engine->SetEnum("compression_cdc");
				VCompression.SetValue("none", (int)Compute::Compression::None);
				VCompression.SetValue("best_speed", (int)Compute::Compression::BestSpeed);
				VCompression.SetValue("best_compression", (int)Compute::Compression::BestCompression);
				VCompression.SetValue("default_compression", (int)Compute::Compression::Default);

				TypeClass VPrivateKey = Engine->SetStructTrivial<Compute::PrivateKey>("private_key");
				VPrivateKey.SetConstructor<Compute::PrivateKey>("void f()");
				VPrivateKey.SetConstructor<Compute::PrivateKey, const Core::String&>("void f(const string &in)");
				VPrivateKey.SetMethod("void clear()", &Compute::PrivateKey::Clear);
				VPrivateKey.SetMethod<Compute::PrivateKey, void, const Core::String&>("void secure(const string &in)", &Compute::PrivateKey::Secure);
				VPrivateKey.SetMethod("string expose_to_heap() const", &Compute::PrivateKey::ExposeToHeap);
				VPrivateKey.SetMethod("usize size() const", &Compute::PrivateKey::Clear);
				VPrivateKey.SetMethodStatic<Compute::PrivateKey, const Core::String&>("private_key get_plain(const string &in)", &Compute::PrivateKey::GetPlain);

				RefClass VWebToken = Engine->SetClass<Compute::WebToken>("web_token", true);
				VWebToken.SetProperty<Compute::WebToken>("schema@ header", &Compute::WebToken::Header);
				VWebToken.SetProperty<Compute::WebToken>("schema@ payload", &Compute::WebToken::Payload);
				VWebToken.SetProperty<Compute::WebToken>("schema@ token", &Compute::WebToken::Token);
				VWebToken.SetProperty<Compute::WebToken>("string refresher", &Compute::WebToken::Refresher);
				VWebToken.SetProperty<Compute::WebToken>("string signature", &Compute::WebToken::Signature);
				VWebToken.SetProperty<Compute::WebToken>("string data", &Compute::WebToken::Data);
				VWebToken.SetGcConstructor<Compute::WebToken, WebToken>("web_token@ f()");
				VWebToken.SetGcConstructor<Compute::WebToken, WebToken, const Core::String&, const Core::String&, int64_t>("web_token@ f(const string &in, const string &in, int64)");
				VWebToken.SetMethod("void unsign()", &Compute::WebToken::Unsign);
				VWebToken.SetMethod("void set_algorithm(const string &in)", &Compute::WebToken::SetAlgorithm);
				VWebToken.SetMethod("void set_type(const string &in)", &Compute::WebToken::SetType);
				VWebToken.SetMethod("void set_content_type(const string &in)", &Compute::WebToken::SetContentType);
				VWebToken.SetMethod("void set_issuer(const string &in)", &Compute::WebToken::SetIssuer);
				VWebToken.SetMethod("void set_subject(const string &in)", &Compute::WebToken::SetSubject);
				VWebToken.SetMethod("void set_id(const string &in)", &Compute::WebToken::SetId);
				VWebToken.SetMethod("void set_expiration(int64)", &Compute::WebToken::SetExpiration);
				VWebToken.SetMethod("void set_not_before(int64)", &Compute::WebToken::SetNotBefore);
				VWebToken.SetMethod("void set_created(int64)", &Compute::WebToken::SetCreated);
				VWebToken.SetMethod("void set_refresh_token(const string &in, const private_key &in, const private_key &in)", &Compute::WebToken::SetRefreshToken);
				VWebToken.SetMethod("bool sign(const private_key &in)", &Compute::WebToken::Sign);
				VWebToken.SetMethod("string get_refresh_token(const private_key &in, const private_key &in)", &Compute::WebToken::GetRefreshToken);
				VWebToken.SetMethod("bool is_valid()", &Compute::WebToken::IsValid);
				VWebToken.SetMethodEx("void set_audience(array<string>@+)", &WebTokenSetAudience);
				VWebToken.SetEnumRefsEx<Compute::WebToken>([](Compute::WebToken* Base, asIScriptEngine* Engine)
				{
					Engine->GCEnumCallback(Base->Header);
					Engine->GCEnumCallback(Base->Payload);
					Engine->GCEnumCallback(Base->Token);
				});
				VWebToken.SetReleaseRefsEx<Compute::WebToken>([](Compute::WebToken* Base, asIScriptEngine*)
				{
					Base->~WebToken();
				});

				Engine->BeginNamespace("ciphers");
				Engine->SetFunction("uptr@ des_ecb()", &Compute::Ciphers::DES_ECB);
				Engine->SetFunction("uptr@ des_ede()", &Compute::Ciphers::DES_EDE);
				Engine->SetFunction("uptr@ des_ede3()", &Compute::Ciphers::DES_EDE3);
				Engine->SetFunction("uptr@ des_ede_ecb()", &Compute::Ciphers::DES_EDE_ECB);
				Engine->SetFunction("uptr@ des_ede3_ecb()", &Compute::Ciphers::DES_EDE3_ECB);
				Engine->SetFunction("uptr@ des_cfb64()", &Compute::Ciphers::DES_CFB64);
				Engine->SetFunction("uptr@ des_cfb1()", &Compute::Ciphers::DES_CFB1);
				Engine->SetFunction("uptr@ des_cfb8()", &Compute::Ciphers::DES_CFB8);
				Engine->SetFunction("uptr@ des_ede_cfb64()", &Compute::Ciphers::DES_EDE_CFB64);
				Engine->SetFunction("uptr@ des_ede3_cfb64()", &Compute::Ciphers::DES_EDE3_CFB64);
				Engine->SetFunction("uptr@ des_ede3_cfb1()", &Compute::Ciphers::DES_EDE3_CFB1);
				Engine->SetFunction("uptr@ des_ede3_cfb8()", &Compute::Ciphers::DES_EDE3_CFB8);
				Engine->SetFunction("uptr@ des_ofb()", &Compute::Ciphers::DES_OFB);
				Engine->SetFunction("uptr@ des_ede_ofb()", &Compute::Ciphers::DES_EDE_OFB);
				Engine->SetFunction("uptr@ des_ede3_ofb()", &Compute::Ciphers::DES_EDE3_OFB);
				Engine->SetFunction("uptr@ des_cbc()", &Compute::Ciphers::DES_CBC);
				Engine->SetFunction("uptr@ des_ede_cbc()", &Compute::Ciphers::DES_EDE_CBC);
				Engine->SetFunction("uptr@ des_ede3_cbc()", &Compute::Ciphers::DES_EDE3_CBC);
				Engine->SetFunction("uptr@ des_ede3_wrap()", &Compute::Ciphers::DES_EDE3_Wrap);
				Engine->SetFunction("uptr@ desx_cbc()", &Compute::Ciphers::DESX_CBC);
				Engine->SetFunction("uptr@ rc4()", &Compute::Ciphers::RC4);
				Engine->SetFunction("uptr@ rc4_40()", &Compute::Ciphers::RC4_40);
				Engine->SetFunction("uptr@ rc4_hmac_md5()", &Compute::Ciphers::RC4_HMAC_MD5);
				Engine->SetFunction("uptr@ idea_ecb()", &Compute::Ciphers::IDEA_ECB);
				Engine->SetFunction("uptr@ idea_cfb64()", &Compute::Ciphers::IDEA_CFB64);
				Engine->SetFunction("uptr@ idea_ofb()", &Compute::Ciphers::IDEA_OFB);
				Engine->SetFunction("uptr@ idea_cbc()", &Compute::Ciphers::IDEA_CBC);
				Engine->SetFunction("uptr@ rc2_ecb()", &Compute::Ciphers::RC2_ECB);
				Engine->SetFunction("uptr@ rc2_cbc()", &Compute::Ciphers::RC2_CBC);
				Engine->SetFunction("uptr@ rc2_40_cbc()", &Compute::Ciphers::RC2_40_CBC);
				Engine->SetFunction("uptr@ rc2_64_cbc()", &Compute::Ciphers::RC2_64_CBC);
				Engine->SetFunction("uptr@ rc2_cfb64()", &Compute::Ciphers::RC2_CFB64);
				Engine->SetFunction("uptr@ rc2_ofb()", &Compute::Ciphers::RC2_OFB);
				Engine->SetFunction("uptr@ bf_ecb()", &Compute::Ciphers::BF_ECB);
				Engine->SetFunction("uptr@ bf_cbc()", &Compute::Ciphers::BF_CBC);
				Engine->SetFunction("uptr@ bf_cfb64()", &Compute::Ciphers::BF_CFB64);
				Engine->SetFunction("uptr@ bf_ofb()", &Compute::Ciphers::BF_OFB);
				Engine->SetFunction("uptr@ cast5_ecb()", &Compute::Ciphers::CAST5_ECB);
				Engine->SetFunction("uptr@ cast5_cbc()", &Compute::Ciphers::CAST5_CBC);
				Engine->SetFunction("uptr@ cast5_cfb64()", &Compute::Ciphers::CAST5_CFB64);
				Engine->SetFunction("uptr@ cast5_ofb()", &Compute::Ciphers::CAST5_OFB);
				Engine->SetFunction("uptr@ rc5_32_12_16_cbc()", &Compute::Ciphers::RC5_32_12_16_CBC);
				Engine->SetFunction("uptr@ rc5_32_12_16_ecb()", &Compute::Ciphers::RC5_32_12_16_ECB);
				Engine->SetFunction("uptr@ rc5_32_12_16_cfb64()", &Compute::Ciphers::RC5_32_12_16_CFB64);
				Engine->SetFunction("uptr@ rc5_32_12_16_ofb()", &Compute::Ciphers::RC5_32_12_16_OFB);
				Engine->SetFunction("uptr@ aes_128_ecb()", &Compute::Ciphers::AES_128_ECB);
				Engine->SetFunction("uptr@ aes_128_cbc()", &Compute::Ciphers::AES_128_CBC);
				Engine->SetFunction("uptr@ aes_128_cfb1()", &Compute::Ciphers::AES_128_CFB1);
				Engine->SetFunction("uptr@ aes_128_cfb8()", &Compute::Ciphers::AES_128_CFB8);
				Engine->SetFunction("uptr@ aes_128_cfb128()", &Compute::Ciphers::AES_128_CFB128);
				Engine->SetFunction("uptr@ aes_128_ofb()", &Compute::Ciphers::AES_128_OFB);
				Engine->SetFunction("uptr@ aes_128_ctr()", &Compute::Ciphers::AES_128_CTR);
				Engine->SetFunction("uptr@ aes_128_ccm()", &Compute::Ciphers::AES_128_CCM);
				Engine->SetFunction("uptr@ aes_128_gcm()", &Compute::Ciphers::AES_128_GCM);
				Engine->SetFunction("uptr@ aes_128_xts()", &Compute::Ciphers::AES_128_XTS);
				Engine->SetFunction("uptr@ aes_128_wrap()", &Compute::Ciphers::AES_128_Wrap);
				Engine->SetFunction("uptr@ aes_128_wrap_pad()", &Compute::Ciphers::AES_128_WrapPad);
				Engine->SetFunction("uptr@ aes_128_ocb()", &Compute::Ciphers::AES_128_OCB);
				Engine->SetFunction("uptr@ aes_192_ecb()", &Compute::Ciphers::AES_192_ECB);
				Engine->SetFunction("uptr@ aes_192_cbc()", &Compute::Ciphers::AES_192_CBC);
				Engine->SetFunction("uptr@ aes_192_cfb1()", &Compute::Ciphers::AES_192_CFB1);
				Engine->SetFunction("uptr@ aes_192_cfb8()", &Compute::Ciphers::AES_192_CFB8);
				Engine->SetFunction("uptr@ aes_192_cfb128()", &Compute::Ciphers::AES_192_CFB128);
				Engine->SetFunction("uptr@ aes_192_ofb()", &Compute::Ciphers::AES_192_OFB);
				Engine->SetFunction("uptr@ aes_192_ctr()", &Compute::Ciphers::AES_192_CTR);
				Engine->SetFunction("uptr@ aes_192_ccm()", &Compute::Ciphers::AES_192_CCM);
				Engine->SetFunction("uptr@ aes_192_gcm()", &Compute::Ciphers::AES_192_GCM);
				Engine->SetFunction("uptr@ aes_192_wrap()", &Compute::Ciphers::AES_192_Wrap);
				Engine->SetFunction("uptr@ aes_192_wrap_pad()", &Compute::Ciphers::AES_192_WrapPad);
				Engine->SetFunction("uptr@ aes_192_ocb()", &Compute::Ciphers::AES_192_OCB);
				Engine->SetFunction("uptr@ aes_256_ecb()", &Compute::Ciphers::AES_256_ECB);
				Engine->SetFunction("uptr@ aes_256_cbc()", &Compute::Ciphers::AES_256_CBC);
				Engine->SetFunction("uptr@ aes_256_cfb1()", &Compute::Ciphers::AES_256_CFB1);
				Engine->SetFunction("uptr@ aes_256_cfb8()", &Compute::Ciphers::AES_256_CFB8);
				Engine->SetFunction("uptr@ aes_256_cfb128()", &Compute::Ciphers::AES_256_CFB128);
				Engine->SetFunction("uptr@ aes_256_ofb()", &Compute::Ciphers::AES_256_OFB);
				Engine->SetFunction("uptr@ aes_256_ctr()", &Compute::Ciphers::AES_256_CTR);
				Engine->SetFunction("uptr@ aes_256_ccm()", &Compute::Ciphers::AES_256_CCM);
				Engine->SetFunction("uptr@ aes_256_gcm()", &Compute::Ciphers::AES_256_GCM);
				Engine->SetFunction("uptr@ aes_256_xts()", &Compute::Ciphers::AES_256_XTS);
				Engine->SetFunction("uptr@ aes_256_wrap()", &Compute::Ciphers::AES_256_Wrap);
				Engine->SetFunction("uptr@ aes_256_wrap_pad()", &Compute::Ciphers::AES_256_WrapPad);
				Engine->SetFunction("uptr@ aes_256_ocb()", &Compute::Ciphers::AES_256_OCB);
				Engine->SetFunction("uptr@ aes_128_cbc_hmac_SHA1()", &Compute::Ciphers::AES_128_CBC_HMAC_SHA1);
				Engine->SetFunction("uptr@ aes_256_cbc_hmac_SHA1()", &Compute::Ciphers::AES_256_CBC_HMAC_SHA1);
				Engine->SetFunction("uptr@ aes_128_cbc_hmac_SHA256()", &Compute::Ciphers::AES_128_CBC_HMAC_SHA256);
				Engine->SetFunction("uptr@ aes_256_cbc_hmac_SHA256()", &Compute::Ciphers::AES_256_CBC_HMAC_SHA256);
				Engine->SetFunction("uptr@ aria_128_ecb()", &Compute::Ciphers::ARIA_128_ECB);
				Engine->SetFunction("uptr@ aria_128_cbc()", &Compute::Ciphers::ARIA_128_CBC);
				Engine->SetFunction("uptr@ aria_128_cfb1()", &Compute::Ciphers::ARIA_128_CFB1);
				Engine->SetFunction("uptr@ aria_128_cfb8()", &Compute::Ciphers::ARIA_128_CFB8);
				Engine->SetFunction("uptr@ aria_128_cfb128()", &Compute::Ciphers::ARIA_128_CFB128);
				Engine->SetFunction("uptr@ aria_128_ctr()", &Compute::Ciphers::ARIA_128_CTR);
				Engine->SetFunction("uptr@ aria_128_ofb()", &Compute::Ciphers::ARIA_128_OFB);
				Engine->SetFunction("uptr@ aria_128_gcm()", &Compute::Ciphers::ARIA_128_GCM);
				Engine->SetFunction("uptr@ aria_128_ccm()", &Compute::Ciphers::ARIA_128_CCM);
				Engine->SetFunction("uptr@ aria_192_ecb()", &Compute::Ciphers::ARIA_192_ECB);
				Engine->SetFunction("uptr@ aria_192_cbc()", &Compute::Ciphers::ARIA_192_CBC);
				Engine->SetFunction("uptr@ aria_192_cfb1()", &Compute::Ciphers::ARIA_192_CFB1);
				Engine->SetFunction("uptr@ aria_192_cfb8()", &Compute::Ciphers::ARIA_192_CFB8);
				Engine->SetFunction("uptr@ aria_192_cfb128()", &Compute::Ciphers::ARIA_192_CFB128);
				Engine->SetFunction("uptr@ aria_192_ctr()", &Compute::Ciphers::ARIA_192_CTR);
				Engine->SetFunction("uptr@ aria_192_ofb()", &Compute::Ciphers::ARIA_192_OFB);
				Engine->SetFunction("uptr@ aria_192_gcm()", &Compute::Ciphers::ARIA_192_GCM);
				Engine->SetFunction("uptr@ aria_192_ccm()", &Compute::Ciphers::ARIA_192_CCM);
				Engine->SetFunction("uptr@ aria_256_ecb()", &Compute::Ciphers::ARIA_256_ECB);
				Engine->SetFunction("uptr@ aria_256_cbc()", &Compute::Ciphers::ARIA_256_CBC);
				Engine->SetFunction("uptr@ aria_256_cfb1()", &Compute::Ciphers::ARIA_256_CFB1);
				Engine->SetFunction("uptr@ aria_256_cfb8()", &Compute::Ciphers::ARIA_256_CFB8);
				Engine->SetFunction("uptr@ aria_256_cfb128()", &Compute::Ciphers::ARIA_256_CFB128);
				Engine->SetFunction("uptr@ aria_256_ctr()", &Compute::Ciphers::ARIA_256_CTR);
				Engine->SetFunction("uptr@ aria_256_ofb()", &Compute::Ciphers::ARIA_256_OFB);
				Engine->SetFunction("uptr@ aria_256_gcm()", &Compute::Ciphers::ARIA_256_GCM);
				Engine->SetFunction("uptr@ aria_256_ccm()", &Compute::Ciphers::ARIA_256_CCM);
				Engine->SetFunction("uptr@ camellia_128_ecb()", &Compute::Ciphers::Camellia_128_ECB);
				Engine->SetFunction("uptr@ camellia_128_cbc()", &Compute::Ciphers::Camellia_128_CBC);
				Engine->SetFunction("uptr@ camellia_128_cfb1()", &Compute::Ciphers::Camellia_128_CFB1);
				Engine->SetFunction("uptr@ camellia_128_cfb8()", &Compute::Ciphers::Camellia_128_CFB8);
				Engine->SetFunction("uptr@ camellia_128_cfb128()", &Compute::Ciphers::Camellia_128_CFB128);
				Engine->SetFunction("uptr@ camellia_128_ofb()", &Compute::Ciphers::Camellia_128_OFB);
				Engine->SetFunction("uptr@ camellia_128_ctr()", &Compute::Ciphers::Camellia_128_CTR);
				Engine->SetFunction("uptr@ camellia_192_ecb()", &Compute::Ciphers::Camellia_192_ECB);
				Engine->SetFunction("uptr@ camellia_192_cbc()", &Compute::Ciphers::Camellia_192_CBC);
				Engine->SetFunction("uptr@ camellia_192_cfb1()", &Compute::Ciphers::Camellia_192_CFB1);
				Engine->SetFunction("uptr@ camellia_192_cfb8()", &Compute::Ciphers::Camellia_192_CFB8);
				Engine->SetFunction("uptr@ camellia_192_cfb128()", &Compute::Ciphers::Camellia_192_CFB128);
				Engine->SetFunction("uptr@ camellia_192_ofb()", &Compute::Ciphers::Camellia_192_OFB);
				Engine->SetFunction("uptr@ camellia_192_ctr()", &Compute::Ciphers::Camellia_192_CTR);
				Engine->SetFunction("uptr@ camellia_256_ecb()", &Compute::Ciphers::Camellia_256_ECB);
				Engine->SetFunction("uptr@ camellia_256_cbc()", &Compute::Ciphers::Camellia_256_CBC);
				Engine->SetFunction("uptr@ camellia_256_cfb1()", &Compute::Ciphers::Camellia_256_CFB1);
				Engine->SetFunction("uptr@ camellia_256_cfb8()", &Compute::Ciphers::Camellia_256_CFB8);
				Engine->SetFunction("uptr@ camellia_256_cfb128()", &Compute::Ciphers::Camellia_256_CFB128);
				Engine->SetFunction("uptr@ camellia_256_ofb()", &Compute::Ciphers::Camellia_256_OFB);
				Engine->SetFunction("uptr@ camellia_256_ctr()", &Compute::Ciphers::Camellia_256_CTR);
				Engine->SetFunction("uptr@ chacha20()", &Compute::Ciphers::Chacha20);
				Engine->SetFunction("uptr@ chacha20_poly1305()", &Compute::Ciphers::Chacha20_Poly1305);
				Engine->SetFunction("uptr@ seed_ecb()", &Compute::Ciphers::Seed_ECB);
				Engine->SetFunction("uptr@ seed_cbc()", &Compute::Ciphers::Seed_CBC);
				Engine->SetFunction("uptr@ seed_cfb128()", &Compute::Ciphers::Seed_CFB128);
				Engine->SetFunction("uptr@ seed_ofb()", &Compute::Ciphers::Seed_OFB);
				Engine->SetFunction("uptr@ sm4_ecb()", &Compute::Ciphers::SM4_ECB);
				Engine->SetFunction("uptr@ sm4_cbc()", &Compute::Ciphers::SM4_CBC);
				Engine->SetFunction("uptr@ sm4_cfb128()", &Compute::Ciphers::SM4_CFB128);
				Engine->SetFunction("uptr@ sm4_ofb()", &Compute::Ciphers::SM4_OFB);
				Engine->SetFunction("uptr@ sm4_ctr()", &Compute::Ciphers::SM4_CTR);
				Engine->EndNamespace();

				Engine->BeginNamespace("digests");
				Engine->SetFunction("uptr@ md2()", &Compute::Digests::MD2);
				Engine->SetFunction("uptr@ md4()", &Compute::Digests::MD4);
				Engine->SetFunction("uptr@ md5()", &Compute::Digests::MD5);
				Engine->SetFunction("uptr@ md5_sha1()", &Compute::Digests::MD5_SHA1);
				Engine->SetFunction("uptr@ blake2b512()", &Compute::Digests::Blake2B512);
				Engine->SetFunction("uptr@ blake2s256()", &Compute::Digests::Blake2S256);
				Engine->SetFunction("uptr@ sha1()", &Compute::Digests::SHA1);
				Engine->SetFunction("uptr@ sha224()", &Compute::Digests::SHA224);
				Engine->SetFunction("uptr@ sha256()", &Compute::Digests::SHA256);
				Engine->SetFunction("uptr@ sha384()", &Compute::Digests::SHA384);
				Engine->SetFunction("uptr@ sha512()", &Compute::Digests::SHA512);
				Engine->SetFunction("uptr@ sha512_224()", &Compute::Digests::SHA512_224);
				Engine->SetFunction("uptr@ sha512_256()", &Compute::Digests::SHA512_256);
				Engine->SetFunction("uptr@ sha3_224()", &Compute::Digests::SHA3_224);
				Engine->SetFunction("uptr@ sha3_256()", &Compute::Digests::SHA3_256);
				Engine->SetFunction("uptr@ sha3_384()", &Compute::Digests::SHA3_384);
				Engine->SetFunction("uptr@ sha3_512()", &Compute::Digests::SHA3_512);
				Engine->SetFunction("uptr@ shake128()", &Compute::Digests::Shake128);
				Engine->SetFunction("uptr@ shake256()", &Compute::Digests::Shake256);
				Engine->SetFunction("uptr@ mdc2()", &Compute::Digests::MDC2);
				Engine->SetFunction("uptr@ ripemd160()", &Compute::Digests::RipeMD160);
				Engine->SetFunction("uptr@ whirlpool()", &Compute::Digests::Whirlpool);
				Engine->SetFunction("uptr@ sm3()", &Compute::Digests::SM3);
				Engine->EndNamespace();

				Engine->BeginNamespace("crypto");
				Engine->SetFunction("string random_bytes(usize)", &Compute::Crypto::RandomBytes);
				Engine->SetFunction("string hash(uptr@, const string &in)", &Compute::Crypto::Hash);
				Engine->SetFunction("string hash_binary(uptr@, const string &in)", &Compute::Crypto::HashBinary);
				Engine->SetFunction<Core::String(Compute::Digest, const Core::String&, const Compute::PrivateKey&)>("string sign(uptr@, const string &in, const private_key &in)", &Compute::Crypto::Sign);
				Engine->SetFunction<Core::String(Compute::Digest, const Core::String&, const Compute::PrivateKey&)>("string hmac(uptr@, const string &in, const private_key &in)", &Compute::Crypto::HMAC);
				Engine->SetFunction<Core::String(Compute::Digest, const Core::String&, const Compute::PrivateKey&, const Compute::PrivateKey&, int)>("string encrypt(uptr@, const string &in, const private_key &in, const private_key &in, int = -1)", &Compute::Crypto::Encrypt);
				Engine->SetFunction<Core::String(Compute::Digest, const Core::String&, const Compute::PrivateKey&, const Compute::PrivateKey&, int)>("string decrypt(uptr@, const string &in, const private_key &in, const private_key &in, int = -1)", &Compute::Crypto::Decrypt);
				Engine->SetFunction("string jwt_sign(const string &in, const string &in, const private_key &in)", &Compute::Crypto::JWTSign);
				Engine->SetFunction("string jwt_encode(web_token@+, const private_key &in)", &Compute::Crypto::JWTEncode);
				Engine->SetFunction("web_token@ jwt_decode(const string &in, const private_key &in)", &Compute::Crypto::JWTDecode);
				Engine->SetFunction("string doc_encrypt(schema@+, const private_key &in, const private_key &in)", &Compute::Crypto::DocEncrypt);
				Engine->SetFunction("schema@ doc_decrypt(const string &in, const private_key &in, const private_key &in)", &Compute::Crypto::DocDecrypt);
				Engine->SetFunction("uint8 random_uc()", &Compute::Crypto::RandomUC);
				Engine->SetFunction<uint64_t()>("uint64 random()", &Compute::Crypto::Random);
				Engine->SetFunction<uint64_t(uint64_t, uint64_t)>("uint64 random(uint64, uint64)", &Compute::Crypto::Random);
				Engine->SetFunction("uint64 crc32(const string &in)", &Compute::Crypto::CRC32);
				Engine->SetFunction("void display_crypto_log()", &Compute::Crypto::DisplayCryptoLog);
				Engine->EndNamespace();

				Engine->BeginNamespace("codec");
				Engine->SetFunction("string move(const string &in, int)", &Compute::Codec::Move);
				Engine->SetFunction("string bep45_encode(const string &in)", &Compute::Codec::Bep45Encode);
				Engine->SetFunction("string bep45_decode(const string &in)", &Compute::Codec::Bep45Decode);
				Engine->SetFunction("string base45_encode(const string &in)", &Compute::Codec::Base45Encode);
				Engine->SetFunction("string base45_decode(const string &in)", &Compute::Codec::Base45Decode);
				Engine->SetFunction("string compress(const string &in, compression_cdc)", &Compute::Codec::Compress);
				Engine->SetFunction("string decompress(const string &in)", &Compute::Codec::Decompress);
				Engine->SetFunction<Core::String(const Core::String&)>("string base65_encode(const string &in)", &Compute::Codec::Base64Encode);
				Engine->SetFunction<Core::String(const Core::String&)>("string base65_decode(const string &in)", &Compute::Codec::Base64Decode);
				Engine->SetFunction<Core::String(const Core::String&)>("string base64_url_encode(const string &in)", &Compute::Codec::Base64URLEncode);
				Engine->SetFunction<Core::String(const Core::String&)>("string base64_url_decode(const string &in)", &Compute::Codec::Base64URLDecode);
				Engine->SetFunction<Core::String(const Core::String&)>("string hex_dncode(const string &in)", &Compute::Codec::HexEncode);
				Engine->SetFunction<Core::String(const Core::String&)>("string hex_decode(const string &in)", &Compute::Codec::HexDecode);
				Engine->SetFunction<Core::String(const Core::String&)>("string uri_encode(const string &in)", &Compute::Codec::URIEncode);
				Engine->SetFunction<Core::String(const Core::String&)>("string uri_decode(const string &in)", &Compute::Codec::URIDecode);
				Engine->SetFunction("string decimal_to_hex(uint64)", &Compute::Codec::DecimalToHex);
				Engine->SetFunction("string base10_to_base_n(uint64, uint8)", &Compute::Codec::Base10ToBaseN);
				Engine->EndNamespace();

				return true;
#else
				VI_ASSERT(false, false, "<crypto> is not loaded");
				return false;
#endif
			}
			bool Registry::LoadGeometric(VirtualMachine* Engine)
			{
#ifdef VI_HAS_BINDINGS
				VI_ASSERT(Engine != nullptr, false, "manager should be set");
				Enumeration VPositioning = Engine->SetEnum("positioning");
				VPositioning.SetValue("local", (int)Compute::Positioning::Local);
				VPositioning.SetValue("global", (int)Compute::Positioning::Global);

				TypeClass VSpacing = Engine->SetPod<Compute::Transform::Spacing>("transform_spacing");
				VSpacing.SetProperty<Compute::Transform::Spacing>("matrix4x4 offset", &Compute::Transform::Spacing::Offset);
				VSpacing.SetProperty<Compute::Transform::Spacing>("vector3 position", &Compute::Transform::Spacing::Position);
				VSpacing.SetProperty<Compute::Transform::Spacing>("vector3 rotation", &Compute::Transform::Spacing::Rotation);
				VSpacing.SetProperty<Compute::Transform::Spacing>("vector3 scale", &Compute::Transform::Spacing::Scale);
				VSpacing.SetConstructor<Compute::Transform::Spacing>("void f()");

				RefClass VTransform = Engine->SetClass<Compute::Transform>("transform", false);
				VTransform.SetProperty<Compute::Transform>("uptr@ user_data", &Compute::Transform::UserData);
				VTransform.SetConstructor<Compute::Transform, void*>("transform@ f(uptr@)");
				VTransform.SetMethod("void synchronize()", &Compute::Transform::Synchronize);
				VTransform.SetMethod("void move(const vector3 &in)", &Compute::Transform::Move);
				VTransform.SetMethod("void rotate(const vector3 &in)", &Compute::Transform::Rotate);
				VTransform.SetMethod("void rescale(const vector3 &in)", &Compute::Transform::Rescale);
				VTransform.SetMethod("void localize(transform_spacing &in)", &Compute::Transform::Localize);
				VTransform.SetMethod("void globalize(transform_spacing &in)", &Compute::Transform::Globalize);
				VTransform.SetMethod("void specialize(transform_spacing &in)", &Compute::Transform::Specialize);
				VTransform.SetMethod("void copy(transform@+) const", &Compute::Transform::Copy);
				VTransform.SetMethod("void add_child(transform@+)", &Compute::Transform::AddChild);
				VTransform.SetMethod("void remove_child(transform@+)", &Compute::Transform::RemoveChild);
				VTransform.SetMethod("void remove_childs()", &Compute::Transform::RemoveChilds);
				VTransform.SetMethod("void make_dirty()", &Compute::Transform::MakeDirty);
				VTransform.SetMethod("void set_scaling(bool)", &Compute::Transform::SetScaling);
				VTransform.SetMethod("void set_position(const vector3 &in)", &Compute::Transform::SetPosition);
				VTransform.SetMethod("void set_rotation(const vector3 &in)", &Compute::Transform::SetRotation);
				VTransform.SetMethod("void set_scale(const vector3 &in)", &Compute::Transform::SetScale);
				VTransform.SetMethod("void set_spacing(positioning, transform_spacing &in)", &Compute::Transform::SetSpacing);
				VTransform.SetMethod("void set_pivot(transform@+, transform_spacing &in)", &Compute::Transform::SetPivot);
				VTransform.SetMethod("void set_root(transform@+)", &Compute::Transform::SetRoot);
				VTransform.SetMethod("void get_bounds(matrix4x4 &in, vector3 &in, vector3 &in)", &Compute::Transform::GetBounds);
				VTransform.SetMethod("bool has_root(transform@+) const", &Compute::Transform::HasRoot);
				VTransform.SetMethod("bool has_child(transform@+) const", &Compute::Transform::HasChild);
				VTransform.SetMethod("bool has_scaling() const", &Compute::Transform::HasScaling);
				VTransform.SetMethod("bool is_dirty() const", &Compute::Transform::IsDirty);
				VTransform.SetMethod("const matrix4x4& get_bias() const", &Compute::Transform::GetBias);
				VTransform.SetMethod("const matrix4x4& get_bias_unscaled() const", &Compute::Transform::GetBiasUnscaled);
				VTransform.SetMethod("const vector3& get_position() const", &Compute::Transform::GetPosition);
				VTransform.SetMethod("const vector3& get_rotation() const", &Compute::Transform::GetRotation);
				VTransform.SetMethod("const vector3& get_scale() const", &Compute::Transform::GetScale);
				VTransform.SetMethod("vector3 forward() const", &Compute::Transform::Forward);
				VTransform.SetMethod("vector3 right() const", &Compute::Transform::Right);
				VTransform.SetMethod("vector3 up() const", &Compute::Transform::Up);
				VTransform.SetMethod<Compute::Transform, Compute::Transform::Spacing&>("transform_spacing& get_spacing()", &Compute::Transform::GetSpacing);
				VTransform.SetMethod<Compute::Transform, Compute::Transform::Spacing&, Compute::Positioning>("transform_spacing& get_spacing(positioning)", &Compute::Transform::GetSpacing);
				VTransform.SetMethod("transform@+ get_root() const", &Compute::Transform::GetRoot);
				VTransform.SetMethod("transform@+ get_upper_root() const", &Compute::Transform::GetUpperRoot);
				VTransform.SetMethod("transform@+ get_child(usize) const", &Compute::Transform::GetChild);
				VTransform.SetMethod("usize get_childs_count() const", &Compute::Transform::GetChildsCount);

				TypeClass VNode = Engine->SetPod<Compute::Cosmos::Node>("cosmos_node");
				VNode.SetProperty<Compute::Cosmos::Node>("bounding bounds", &Compute::Cosmos::Node::Bounds);
				VNode.SetProperty<Compute::Cosmos::Node>("usize parent", &Compute::Cosmos::Node::Parent);
				VNode.SetProperty<Compute::Cosmos::Node>("usize next", &Compute::Cosmos::Node::Next);
				VNode.SetProperty<Compute::Cosmos::Node>("usize left", &Compute::Cosmos::Node::Left);
				VNode.SetProperty<Compute::Cosmos::Node>("usize right", &Compute::Cosmos::Node::Right);
				VNode.SetProperty<Compute::Cosmos::Node>("uptr@ item", &Compute::Cosmos::Node::Item);
				VNode.SetProperty<Compute::Cosmos::Node>("int32 height", &Compute::Cosmos::Node::Height);
				VNode.SetConstructor<Compute::Cosmos::Node>("void f()");
				VNode.SetMethod("bool is_leaf() const", &Compute::Cosmos::Node::IsLeaf);

				TypeClass VCosmos = Engine->SetStructTrivial<Compute::Cosmos>("cosmos");
				VCosmos.SetFunctionDef("bool cosmos_query_overlaps(const bounding &in)");
				VCosmos.SetFunctionDef("void cosmos_query_match(uptr@)");
				VCosmos.SetConstructor<Compute::Cosmos>("void f(usize = 16)");
				VCosmos.SetMethod("void reserve(usize)", &Compute::Cosmos::Reserve);
				VCosmos.SetMethod("void clear()", &Compute::Cosmos::Clear);
				VCosmos.SetMethod("void remove_item(uptr@)", &Compute::Cosmos::RemoveItem);
				VCosmos.SetMethod("void insert_item(uptr@, const vector3 &in, const vector3 &in)", &Compute::Cosmos::InsertItem);
				VCosmos.SetMethod("void update_item(uptr@, const vector3 &in, const vector3 &in, bool = false)", &Compute::Cosmos::UpdateItem);
				VCosmos.SetMethod("const bounding& get_area(uptr@)", &Compute::Cosmos::GetArea);
				VCosmos.SetMethod("usize get_nodes_count() const", &Compute::Cosmos::GetNodesCount);
				VCosmos.SetMethod("usize get_height() const", &Compute::Cosmos::GetHeight);
				VCosmos.SetMethod("usize get_max_balance() const", &Compute::Cosmos::GetMaxBalance);
				VCosmos.SetMethod("usize get_root() const", &Compute::Cosmos::GetRoot);
				VCosmos.SetMethod("const cosmos_node& get_root_node() const", &Compute::Cosmos::GetRootNode);
				VCosmos.SetMethod("const cosmos_node& get_node(usize) const", &Compute::Cosmos::GetNode);
				VCosmos.SetMethod("float get_volume_ratio() const", &Compute::Cosmos::GetVolumeRatio);
				VCosmos.SetMethod("bool is_null(usize) const", &Compute::Cosmos::IsNull);
				VCosmos.SetMethod("bool is_empty() const", &Compute::Cosmos::IsEmpty);
				VCosmos.SetMethodEx("void query_index(cosmos_query_overlaps@, cosmos_query_match@)", &CosmosQueryIndex);

				Engine->BeginNamespace("geometric");
				Engine->SetFunction("bool is_cube_in_frustum(const matrix4x4 &in, float)", &Compute::Geometric::IsCubeInFrustum);
				Engine->SetFunction("bool is_left_handed()", &Compute::Geometric::IsLeftHanded);
				Engine->SetFunction("bool has_sphere_intersected(const vector3 &in, float, const vector3 &in, float)", &Compute::Geometric::HasSphereIntersected);
				Engine->SetFunction("bool has_line_intersected(float, float, const vector3 &in, const vector3 &in, vector3 &out)", &Compute::Geometric::HasLineIntersected);
				Engine->SetFunction("bool has_line_intersected_cube(const vector3 &in, const vector3 &in, const vector3 &in, const vector3 &in)", &Compute::Geometric::HasLineIntersectedCube);
				Engine->SetFunction<bool(const Compute::Vector3&, const Compute::Vector3&, const Compute::Vector3&, int)>("bool has_point_intersected_cube(const vector3 &in, const vector3 &in, const vector3 &in, int32)", &Compute::Geometric::HasPointIntersectedCube);
				Engine->SetFunction("bool has_point_intersected_rectangle(const vector3 &in, const vector3 &in, const vector3 &in)", &Compute::Geometric::HasPointIntersectedRectangle);
				Engine->SetFunction<bool(const Compute::Vector3&, const Compute::Vector3&, const Compute::Vector3&)>("bool has_point_intersected_cube(const vector3 &in, const vector3 &in, const vector3 &in)", &Compute::Geometric::HasPointIntersectedCube);
				Engine->SetFunction("bool has_sb_intersected(transform@+, transform@+)", &Compute::Geometric::HasSBIntersected);
				Engine->SetFunction("bool has_obb_intersected(transform@+, transform@+)", &Compute::Geometric::HasOBBIntersected);
				Engine->SetFunction("bool has_aabb_intersected(transform@+, transform@+)", &Compute::Geometric::HasAABBIntersected);
				Engine->SetFunction("void matrix_rh_to_lh(matrix4x4 &out)", &Compute::Geometric::MatrixRhToLh);
				Engine->SetFunction("void set_left_handed(bool)", &Compute::Geometric::SetLeftHanded);
				Engine->SetFunction("ray create_cursor_ray(const vector3 &in, const vector2 &in, const vector2 &in, const matrix4x4 &in, const matrix4x4 &in)", &Compute::Geometric::CreateCursorRay);
				Engine->SetFunction<bool(const Compute::Ray&, const Compute::Vector3&, const Compute::Vector3&, Compute::Vector3*)>("bool cursor_ray_test(const ray &in, const vector3 &in, const vector3 &in, vector3 &out)", &Compute::Geometric::CursorRayTest);
				Engine->SetFunction<bool(const Compute::Ray&, const Compute::Matrix4x4&, Compute::Vector3*)>("bool cursor_ray_test(const ray &in, const matrix4x4 &in, vector3 &out)", &Compute::Geometric::CursorRayTest);
				Engine->SetFunction("float fast_inv_sqrt(float)", &Compute::Geometric::FastInvSqrt);
				Engine->SetFunction("float fast_sqrt(float)", &Compute::Geometric::FastSqrt);
				Engine->SetFunction("float aabb_volume(const vector3 &in, const vector3 &in)", &Compute::Geometric::AabbVolume);
				Engine->EndNamespace();

				return true;
#else
				VI_ASSERT(false, false, "<geometric> is not loaded");
				return false;
#endif
			}
			bool Registry::LoadPreprocessor(VirtualMachine* Engine)
			{
#ifdef VI_HAS_BINDINGS
				VI_ASSERT(Engine != nullptr, false, "manager should be set");
				TypeClass VIncludeDesc = Engine->SetStructTrivial<Compute::IncludeDesc>("include_desc");
				VIncludeDesc.SetProperty<Compute::IncludeDesc>("string from", &Compute::IncludeDesc::From);
				VIncludeDesc.SetProperty<Compute::IncludeDesc>("string path", &Compute::IncludeDesc::Path);
				VIncludeDesc.SetProperty<Compute::IncludeDesc>("string root", &Compute::IncludeDesc::Root);
				VIncludeDesc.SetConstructor<Compute::IncludeDesc>("void f()");
				VIncludeDesc.SetMethodEx("void add_ext(const string &in)", &IncludeDescAddExt);
				VIncludeDesc.SetMethodEx("void remove_ext(const string &in)", &IncludeDescRemoveExt);

				TypeClass VIncludeResult = Engine->SetStructTrivial<Compute::IncludeResult>("include_result");
				VIncludeResult.SetProperty<Compute::IncludeResult>("string module", &Compute::IncludeResult::Module);
				VIncludeResult.SetProperty<Compute::IncludeResult>("bool is_system", &Compute::IncludeResult::IsSystem);
				VIncludeResult.SetProperty<Compute::IncludeResult>("bool is_file", &Compute::IncludeResult::IsFile);
				VIncludeResult.SetConstructor<Compute::IncludeResult>("void f()");

				TypeClass VDesc = Engine->SetPod<Compute::Preprocessor::Desc>("preprocessor_desc");
				VDesc.SetProperty<Compute::Preprocessor::Desc>("string multiline_comment_begin", &Compute::Preprocessor::Desc::MultilineCommentBegin);
				VDesc.SetProperty<Compute::Preprocessor::Desc>("string multiline_comment_end", &Compute::Preprocessor::Desc::MultilineCommentEnd);
				VDesc.SetProperty<Compute::Preprocessor::Desc>("string comment_begin", &Compute::Preprocessor::Desc::CommentBegin);
				VDesc.SetProperty<Compute::Preprocessor::Desc>("bool pragmas", &Compute::Preprocessor::Desc::Pragmas);
				VDesc.SetProperty<Compute::Preprocessor::Desc>("bool includes", &Compute::Preprocessor::Desc::Includes);
				VDesc.SetProperty<Compute::Preprocessor::Desc>("bool defines", &Compute::Preprocessor::Desc::Defines);
				VDesc.SetProperty<Compute::Preprocessor::Desc>("bool conditions", &Compute::Preprocessor::Desc::Conditions);
				VDesc.SetConstructor<Compute::Preprocessor::Desc>("void f()");

				RefClass VPreprocessor = Engine->SetClass<Compute::Preprocessor>("preprocessor", false);
				VPreprocessor.SetFunctionDef("bool proc_include_event(preprocessor@+, const include_result &in, string &out)");
				VPreprocessor.SetFunctionDef("bool proc_pragma_event(preprocessor@+, const string &in, array<string>@+)");
				VPreprocessor.SetConstructor<Compute::Preprocessor>("preprocessor@ f(uptr@)");
				VPreprocessor.SetMethod("void set_include_options(const include_desc &in)", &Compute::Preprocessor::SetIncludeOptions);
				VPreprocessor.SetMethod("void set_features(const preprocessor_desc &in)", &Compute::Preprocessor::SetFeatures);
				VPreprocessor.SetMethod("void define(const string &in)", &Compute::Preprocessor::Define);
				VPreprocessor.SetMethod("void undefine(const string &in)", &Compute::Preprocessor::Undefine);
				VPreprocessor.SetMethod("void clear()", &Compute::Preprocessor::Clear);
				VPreprocessor.SetMethod("bool process(const string &in, string &out)", &Compute::Preprocessor::Process);
				VPreprocessor.SetMethod("const string& get_current_file_path() const", &Compute::Preprocessor::GetCurrentFilePath);
				VPreprocessor.SetMethodEx("void set_include_callback(proc_include_event@+)", &PreprocessorSetIncludeCallback);
				VPreprocessor.SetMethodEx("void set_pragma_callback(proc_pragma_event@+)", &PreprocessorSetPragmaCallback);
				VPreprocessor.SetMethodEx("bool is_defined(const string &in) const", &PreprocessorIsDefined);

				return true;
#else
				VI_ASSERT(false, false, "<preprocessor> is not loaded");
				return false;
#endif
			}
			bool Registry::LoadPhysics(VirtualMachine* Engine)
			{
#ifdef VI_HAS_BINDINGS
				VI_ASSERT(Engine != nullptr, false, "manager should be set");
				RefClass VSimulator = Engine->SetClass<Compute::Simulator>("physics_simulator", false);
				
				Enumeration VShape = Engine->SetEnum("physics_shape");
				VShape.SetValue("box", (int)Compute::Shape::Box);
				VShape.SetValue("triangle", (int)Compute::Shape::Triangle);
				VShape.SetValue("tetrahedral", (int)Compute::Shape::Tetrahedral);
				VShape.SetValue("convex_triangle_mesh", (int)Compute::Shape::Convex_Triangle_Mesh);
				VShape.SetValue("convex_hull", (int)Compute::Shape::Convex_Hull);
				VShape.SetValue("convex_point_cloud", (int)Compute::Shape::Convex_Point_Cloud);
				VShape.SetValue("convex_polyhedral", (int)Compute::Shape::Convex_Polyhedral);
				VShape.SetValue("implicit_convex_start", (int)Compute::Shape::Implicit_Convex_Start);
				VShape.SetValue("sphere", (int)Compute::Shape::Sphere);
				VShape.SetValue("multi_sphere", (int)Compute::Shape::Multi_Sphere);
				VShape.SetValue("capsule", (int)Compute::Shape::Capsule);
				VShape.SetValue("cone", (int)Compute::Shape::Cone);
				VShape.SetValue("convex", (int)Compute::Shape::Convex);
				VShape.SetValue("cylinder", (int)Compute::Shape::Cylinder);
				VShape.SetValue("uniform_scaling", (int)Compute::Shape::Uniform_Scaling);
				VShape.SetValue("minkowski_sum", (int)Compute::Shape::Minkowski_Sum);
				VShape.SetValue("minkowski_difference", (int)Compute::Shape::Minkowski_Difference);
				VShape.SetValue("box_2d", (int)Compute::Shape::Box_2D);
				VShape.SetValue("convex_2d", (int)Compute::Shape::Convex_2D);
				VShape.SetValue("custom_convex", (int)Compute::Shape::Custom_Convex);
				VShape.SetValue("concaves_start", (int)Compute::Shape::Concaves_Start);
				VShape.SetValue("triangle_mesh", (int)Compute::Shape::Triangle_Mesh);
				VShape.SetValue("triangle_mesh_scaled", (int)Compute::Shape::Triangle_Mesh_Scaled);
				VShape.SetValue("fast_concave_mesh", (int)Compute::Shape::Fast_Concave_Mesh);
				VShape.SetValue("terrain", (int)Compute::Shape::Terrain);
				VShape.SetValue("triangle_mesh_multimaterial", (int)Compute::Shape::Triangle_Mesh_Multimaterial);
				VShape.SetValue("empty", (int)Compute::Shape::Empty);
				VShape.SetValue("static_plane", (int)Compute::Shape::Static_Plane);
				VShape.SetValue("custom_concave", (int)Compute::Shape::Custom_Concave);
				VShape.SetValue("concaves_end", (int)Compute::Shape::Concaves_End);
				VShape.SetValue("compound", (int)Compute::Shape::Compound);
				VShape.SetValue("softbody", (int)Compute::Shape::Softbody);
				VShape.SetValue("hf_fluid", (int)Compute::Shape::HF_Fluid);
				VShape.SetValue("hf_fluid_bouyant_convex", (int)Compute::Shape::HF_Fluid_Bouyant_Convex);
				VShape.SetValue("invalid", (int)Compute::Shape::Invalid);

				Enumeration VMotionState = Engine->SetEnum("physics_motion_state");
				VMotionState.SetValue("active", (int)Compute::MotionState::Active);
				VMotionState.SetValue("island_sleeping", (int)Compute::MotionState::Island_Sleeping);
				VMotionState.SetValue("deactivation_needed", (int)Compute::MotionState::Deactivation_Needed);
				VMotionState.SetValue("disable_deactivation", (int)Compute::MotionState::Disable_Deactivation);
				VMotionState.SetValue("disable_simulation", (int)Compute::MotionState::Disable_Simulation);

				Enumeration VSoftFeature = Engine->SetEnum("physics_soft_feature");
				VSoftFeature.SetValue("none", (int)Compute::SoftFeature::None);
				VSoftFeature.SetValue("node", (int)Compute::SoftFeature::Node);
				VSoftFeature.SetValue("link", (int)Compute::SoftFeature::Link);
				VSoftFeature.SetValue("face", (int)Compute::SoftFeature::Face);
				VSoftFeature.SetValue("tetra", (int)Compute::SoftFeature::Tetra);

				Enumeration VSoftAeroModel = Engine->SetEnum("physics_soft_aero_model");
				VSoftAeroModel.SetValue("vpoint", (int)Compute::SoftAeroModel::VPoint);
				VSoftAeroModel.SetValue("vtwo_sided", (int)Compute::SoftAeroModel::VTwoSided);
				VSoftAeroModel.SetValue("vtwo_sided_lift_drag", (int)Compute::SoftAeroModel::VTwoSidedLiftDrag);
				VSoftAeroModel.SetValue("vone_sided", (int)Compute::SoftAeroModel::VOneSided);
				VSoftAeroModel.SetValue("ftwo_sided", (int)Compute::SoftAeroModel::FTwoSided);
				VSoftAeroModel.SetValue("ftwo_sided_lift_drag", (int)Compute::SoftAeroModel::FTwoSidedLiftDrag);
				VSoftAeroModel.SetValue("fone_sided", (int)Compute::SoftAeroModel::FOneSided);

				Enumeration VSoftCollision = Engine->SetEnum("physics_soft_collision");
				VSoftCollision.SetValue("rvs_mask", (int)Compute::SoftCollision::RVS_Mask);
				VSoftCollision.SetValue("sdf_rs", (int)Compute::SoftCollision::SDF_RS);
				VSoftCollision.SetValue("cl_rs", (int)Compute::SoftCollision::CL_RS);
				VSoftCollision.SetValue("sdf_rd", (int)Compute::SoftCollision::SDF_RD);
				VSoftCollision.SetValue("sdf_rdf", (int)Compute::SoftCollision::SDF_RDF);
				VSoftCollision.SetValue("svs_mask", (int)Compute::SoftCollision::SVS_Mask);
				VSoftCollision.SetValue("vf_ss", (int)Compute::SoftCollision::VF_SS);
				VSoftCollision.SetValue("cl_ss", (int)Compute::SoftCollision::CL_SS);
				VSoftCollision.SetValue("cl_self", (int)Compute::SoftCollision::CL_Self);
				VSoftCollision.SetValue("vf_dd", (int)Compute::SoftCollision::VF_DD);
				VSoftCollision.SetValue("default_t", (int)Compute::SoftCollision::Default);

				Enumeration VRotator = Engine->SetEnum("physics_rotator");
				VRotator.SetValue("xyz", (int)Compute::Rotator::XYZ);
				VRotator.SetValue("xzy", (int)Compute::Rotator::XZY);
				VRotator.SetValue("yxz", (int)Compute::Rotator::YXZ);
				VRotator.SetValue("yzx", (int)Compute::Rotator::YZX);
				VRotator.SetValue("zxy", (int)Compute::Rotator::ZXY);
				VRotator.SetValue("zyx", (int)Compute::Rotator::ZYX);

				RefClass VHullShape = Engine->SetClass<Compute::HullShape>("physics_hull_shape", false);
				VHullShape.SetMethod("uptr@ get_shape()", &Compute::HullShape::GetShape);
				VHullShape.SetMethodEx("array<vertex>@ get_vertices()", &HullShapeGetVertices);
				VHullShape.SetMethodEx("array<int>@ get_indices()", &HullShapeGetIndices);

				TypeClass VRigidBodyDesc = Engine->SetPod<Compute::RigidBody::Desc>("physics_rigidbody_desc");
				VRigidBodyDesc.SetProperty<Compute::RigidBody::Desc>("uptr@ shape", &Compute::RigidBody::Desc::Shape);
				VRigidBodyDesc.SetProperty<Compute::RigidBody::Desc>("float anticipation", &Compute::RigidBody::Desc::Anticipation);
				VRigidBodyDesc.SetProperty<Compute::RigidBody::Desc>("float mass", &Compute::RigidBody::Desc::Mass);
				VRigidBodyDesc.SetProperty<Compute::RigidBody::Desc>("vector3 position", &Compute::RigidBody::Desc::Position);
				VRigidBodyDesc.SetProperty<Compute::RigidBody::Desc>("vector3 rotation", &Compute::RigidBody::Desc::Rotation);
				VRigidBodyDesc.SetProperty<Compute::RigidBody::Desc>("vector3 scale", &Compute::RigidBody::Desc::Scale);
				VRigidBodyDesc.SetConstructor<Compute::RigidBody::Desc>("void f()");

				RefClass VRigidBody = Engine->SetClass<Compute::RigidBody>("physics_rigidbody", false);
				VRigidBody.SetMethod("physics_rigidbody@ copy()", &Compute::RigidBody::Copy);
				VRigidBody.SetMethod<Compute::RigidBody, void, const Compute::Vector3&>("void push(const vector3 &in)", &Compute::RigidBody::Push);
				VRigidBody.SetMethod<Compute::RigidBody, void, const Compute::Vector3&, const Compute::Vector3&>("void push(const vector3 &in, const vector3 &in)", &Compute::RigidBody::Push);
				VRigidBody.SetMethod<Compute::RigidBody, void, const Compute::Vector3&, const Compute::Vector3&, const Compute::Vector3&>("void push(const vector3 &in, const vector3 &in, const vector3 &in)", &Compute::RigidBody::Push);
				VRigidBody.SetMethod<Compute::RigidBody, void, const Compute::Vector3&>("void push_kinematic(const vector3 &in)", &Compute::RigidBody::PushKinematic);
				VRigidBody.SetMethod<Compute::RigidBody, void, const Compute::Vector3&, const Compute::Vector3&>("void push_kinematic(const vector3 &in, const vector3 &in)", &Compute::RigidBody::PushKinematic);
				VRigidBody.SetMethod("void synchronize(transform@+, bool)", &Compute::RigidBody::Synchronize);
				VRigidBody.SetMethod("void set_collision_flags(usize)", &Compute::RigidBody::SetCollisionFlags);
				VRigidBody.SetMethod("void set_activity(bool)", &Compute::RigidBody::SetActivity);
				VRigidBody.SetMethod("void set_as_ghost()", &Compute::RigidBody::SetAsGhost);
				VRigidBody.SetMethod("void set_as_normal()", &Compute::RigidBody::SetAsNormal);
				VRigidBody.SetMethod("void set_self_pointer()", &Compute::RigidBody::SetSelfPointer);
				VRigidBody.SetMethod("void set_world_transform(uptr@)", &Compute::RigidBody::SetWorldTransform);
				VRigidBody.SetMethod("void set_collision_shape(uptr@, transform@+)", &Compute::RigidBody::SetCollisionShape);
				VRigidBody.SetMethod("void set_mass(float)", &Compute::RigidBody::SetMass);
				VRigidBody.SetMethod("void set_activation_state(physics_motion_state)", &Compute::RigidBody::SetActivationState);
				VRigidBody.SetMethod("void set_angular_damping(float)", &Compute::RigidBody::SetAngularDamping);
				VRigidBody.SetMethod("void set_angular_sleeping_threshold(float)", &Compute::RigidBody::SetAngularSleepingThreshold);
				VRigidBody.SetMethod("void set_spinning_friction(float)", &Compute::RigidBody::SetSpinningFriction);
				VRigidBody.SetMethod("void set_contact_stiffness(float)", &Compute::RigidBody::SetContactStiffness);
				VRigidBody.SetMethod("void set_contact_damping(float)", &Compute::RigidBody::SetContactDamping);
				VRigidBody.SetMethod("void set_friction(float)", &Compute::RigidBody::SetFriction);
				VRigidBody.SetMethod("void set_restitution(float)", &Compute::RigidBody::SetRestitution);
				VRigidBody.SetMethod("void set_hit_fraction(float)", &Compute::RigidBody::SetHitFraction);
				VRigidBody.SetMethod("void set_linear_damping(float)", &Compute::RigidBody::SetLinearDamping);
				VRigidBody.SetMethod("void set_linear_sleeping_threshold(float)", &Compute::RigidBody::SetLinearSleepingThreshold);
				VRigidBody.SetMethod("void set_ccd_motion_threshold(float)", &Compute::RigidBody::SetCcdMotionThreshold);
				VRigidBody.SetMethod("void set_ccd_swept_sphere_radius(float)", &Compute::RigidBody::SetCcdSweptSphereRadius);
				VRigidBody.SetMethod("void set_contact_processing_threshold(float)", &Compute::RigidBody::SetContactProcessingThreshold);
				VRigidBody.SetMethod("void set_deactivation_time(float)", &Compute::RigidBody::SetDeactivationTime);
				VRigidBody.SetMethod("void set_rolling_friction(float)", &Compute::RigidBody::SetRollingFriction);
				VRigidBody.SetMethod("void set_angular_factor(const vector3 &in)", &Compute::RigidBody::SetAngularFactor);
				VRigidBody.SetMethod("void set_anisotropic_friction(const vector3 &in)", &Compute::RigidBody::SetAnisotropicFriction);
				VRigidBody.SetMethod("void set_gravity(const vector3 &in)", &Compute::RigidBody::SetGravity);
				VRigidBody.SetMethod("void set_linear_factor(const vector3 &in)", &Compute::RigidBody::SetLinearFactor);
				VRigidBody.SetMethod("void set_linear_velocity(const vector3 &in)", &Compute::RigidBody::SetLinearVelocity);
				VRigidBody.SetMethod("void set_angular_velocity(const vector3 &in)", &Compute::RigidBody::SetAngularVelocity);
				VRigidBody.SetMethod("physics_motion_state get_activation_state() const", &Compute::RigidBody::GetActivationState);
				VRigidBody.SetMethod("physics_shape get_collision_shape_type() const", &Compute::RigidBody::GetCollisionShapeType);
				VRigidBody.SetMethod("vector3 get_angular_factor() const", &Compute::RigidBody::GetAngularFactor);
				VRigidBody.SetMethod("vector3 get_anisotropic_friction() const", &Compute::RigidBody::GetAnisotropicFriction);
				VRigidBody.SetMethod("vector3 get_Gravity() const", &Compute::RigidBody::GetGravity);
				VRigidBody.SetMethod("vector3 get_linear_factor() const", &Compute::RigidBody::GetLinearFactor);
				VRigidBody.SetMethod("vector3 get_linear_velocity() const", &Compute::RigidBody::GetLinearVelocity);
				VRigidBody.SetMethod("vector3 get_angular_velocity() const", &Compute::RigidBody::GetAngularVelocity);
				VRigidBody.SetMethod("vector3 get_scale() const", &Compute::RigidBody::GetScale);
				VRigidBody.SetMethod("vector3 get_position() const", &Compute::RigidBody::GetPosition);
				VRigidBody.SetMethod("vector3 get_rotation() const", &Compute::RigidBody::GetRotation);
				VRigidBody.SetMethod("uptr@ get_world_transform() const", &Compute::RigidBody::GetWorldTransform);
				VRigidBody.SetMethod("uptr@ get_collision_shape() const", &Compute::RigidBody::GetCollisionShape);
				VRigidBody.SetMethod("uptr@ get() const", &Compute::RigidBody::Get);
				VRigidBody.SetMethod("bool is_active() const", &Compute::RigidBody::IsActive);
				VRigidBody.SetMethod("bool is_static() const", &Compute::RigidBody::IsStatic);
				VRigidBody.SetMethod("bool is_ghost() const", &Compute::RigidBody::IsGhost);
				VRigidBody.SetMethod("bool Is_colliding() const", &Compute::RigidBody::IsColliding);
				VRigidBody.SetMethod("float get_spinning_friction() const", &Compute::RigidBody::GetSpinningFriction);
				VRigidBody.SetMethod("float get_contact_stiffness() const", &Compute::RigidBody::GetContactStiffness);
				VRigidBody.SetMethod("float get_contact_damping() const", &Compute::RigidBody::GetContactDamping);
				VRigidBody.SetMethod("float get_angular_damping() const", &Compute::RigidBody::GetAngularDamping);
				VRigidBody.SetMethod("float get_angular_sleeping_threshold() const", &Compute::RigidBody::GetAngularSleepingThreshold);
				VRigidBody.SetMethod("float get_friction() const", &Compute::RigidBody::GetFriction);
				VRigidBody.SetMethod("float get_restitution() const", &Compute::RigidBody::GetRestitution);
				VRigidBody.SetMethod("float get_hit_fraction() const", &Compute::RigidBody::GetHitFraction);
				VRigidBody.SetMethod("float get_linear_damping() const", &Compute::RigidBody::GetLinearDamping);
				VRigidBody.SetMethod("float get_linear_sleeping_threshold() const", &Compute::RigidBody::GetLinearSleepingThreshold);
				VRigidBody.SetMethod("float get_ccd_motion_threshold() const", &Compute::RigidBody::GetCcdMotionThreshold);
				VRigidBody.SetMethod("float get_ccd_swept_sphere_radius() const", &Compute::RigidBody::GetCcdSweptSphereRadius);
				VRigidBody.SetMethod("float get_contact_processing_threshold() const", &Compute::RigidBody::GetContactProcessingThreshold);
				VRigidBody.SetMethod("float get_deactivation_time() const", &Compute::RigidBody::GetDeactivationTime);
				VRigidBody.SetMethod("float get_rolling_friction() const", &Compute::RigidBody::GetRollingFriction);
				VRigidBody.SetMethod("float get_mass() const", &Compute::RigidBody::GetMass);
				VRigidBody.SetMethod("usize get_collision_flags() const", &Compute::RigidBody::GetCollisionFlags);
				VRigidBody.SetMethod("physics_rigidbody_desc& get_initial_state()", &Compute::RigidBody::GetInitialState);
				VRigidBody.SetMethod("physics_simulator@+ get_simulator() const", &Compute::RigidBody::GetSimulator);

				TypeClass VSoftBodySConvex = Engine->SetStruct<Compute::SoftBody::Desc::CV::SConvex>("physics_softbody_desc_cv_sconvex");
				VSoftBodySConvex.SetProperty<Compute::SoftBody::Desc::CV::SConvex>("physics_hull_shape@ hull", &Compute::SoftBody::Desc::CV::SConvex::Hull);
				VSoftBodySConvex.SetProperty<Compute::SoftBody::Desc::CV::SConvex>("bool enabled", &Compute::SoftBody::Desc::CV::SConvex::Enabled);
				VSoftBodySConvex.SetConstructor<Compute::SoftBody::Desc::CV::SConvex>("void f()");
				VSoftBodySConvex.SetOperatorCopyStatic(&SoftBodySConvexCopy);
				VSoftBodySConvex.SetDestructorStatic("void f()", &SoftBodySConvexDestructor);

				TypeClass VSoftBodySRope = Engine->SetPod<Compute::SoftBody::Desc::CV::SRope>("physics_softbody_desc_cv_srope");
				VSoftBodySRope.SetProperty<Compute::SoftBody::Desc::CV::SRope>("bool start_fixed", &Compute::SoftBody::Desc::CV::SRope::StartFixed);
				VSoftBodySRope.SetProperty<Compute::SoftBody::Desc::CV::SRope>("bool end_fixed", &Compute::SoftBody::Desc::CV::SRope::EndFixed);
				VSoftBodySRope.SetProperty<Compute::SoftBody::Desc::CV::SRope>("bool enabled", &Compute::SoftBody::Desc::CV::SRope::Enabled);
				VSoftBodySRope.SetProperty<Compute::SoftBody::Desc::CV::SRope>("int count", &Compute::SoftBody::Desc::CV::SRope::Count);
				VSoftBodySRope.SetProperty<Compute::SoftBody::Desc::CV::SRope>("vector3 start", &Compute::SoftBody::Desc::CV::SRope::Start);
				VSoftBodySRope.SetProperty<Compute::SoftBody::Desc::CV::SRope>("vector3 end", &Compute::SoftBody::Desc::CV::SRope::End);
				VSoftBodySRope.SetConstructor<Compute::SoftBody::Desc::CV::SRope>("void f()");

				TypeClass VSoftBodySPatch = Engine->SetPod<Compute::SoftBody::Desc::CV::SPatch>("physics_softbody_desc_cv_spatch");
				VSoftBodySPatch.SetProperty<Compute::SoftBody::Desc::CV::SPatch>("bool generate_diagonals", &Compute::SoftBody::Desc::CV::SPatch::GenerateDiagonals);
				VSoftBodySPatch.SetProperty<Compute::SoftBody::Desc::CV::SPatch>("bool corner00_fixed", &Compute::SoftBody::Desc::CV::SPatch::Corner00Fixed);
				VSoftBodySPatch.SetProperty<Compute::SoftBody::Desc::CV::SPatch>("bool corner10_fixed", &Compute::SoftBody::Desc::CV::SPatch::Corner10Fixed);
				VSoftBodySPatch.SetProperty<Compute::SoftBody::Desc::CV::SPatch>("bool corner01_fixed", &Compute::SoftBody::Desc::CV::SPatch::Corner01Fixed);
				VSoftBodySPatch.SetProperty<Compute::SoftBody::Desc::CV::SPatch>("bool corner11_fixed", &Compute::SoftBody::Desc::CV::SPatch::Corner11Fixed);
				VSoftBodySPatch.SetProperty<Compute::SoftBody::Desc::CV::SPatch>("bool enabled", &Compute::SoftBody::Desc::CV::SPatch::Enabled);
				VSoftBodySPatch.SetProperty<Compute::SoftBody::Desc::CV::SPatch>("int count_x", &Compute::SoftBody::Desc::CV::SPatch::CountX);
				VSoftBodySPatch.SetProperty<Compute::SoftBody::Desc::CV::SPatch>("int count_y", &Compute::SoftBody::Desc::CV::SPatch::CountY);
				VSoftBodySPatch.SetProperty<Compute::SoftBody::Desc::CV::SPatch>("vector3 corner00", &Compute::SoftBody::Desc::CV::SPatch::Corner00);
				VSoftBodySPatch.SetProperty<Compute::SoftBody::Desc::CV::SPatch>("vector3 corner10", &Compute::SoftBody::Desc::CV::SPatch::Corner10);
				VSoftBodySPatch.SetProperty<Compute::SoftBody::Desc::CV::SPatch>("vector3 corner01", &Compute::SoftBody::Desc::CV::SPatch::Corner01);
				VSoftBodySPatch.SetProperty<Compute::SoftBody::Desc::CV::SPatch>("vector3 corner11", &Compute::SoftBody::Desc::CV::SPatch::Corner11);
				VSoftBodySPatch.SetConstructor<Compute::SoftBody::Desc::CV::SPatch>("void f()");

				TypeClass VSoftBodySEllipsoid = Engine->SetPod<Compute::SoftBody::Desc::CV::SEllipsoid>("physics_softbody_desc_cv_sellipsoid");
				VSoftBodySEllipsoid.SetProperty<Compute::SoftBody::Desc::CV::SEllipsoid>("vector3 center", &Compute::SoftBody::Desc::CV::SEllipsoid::Center);
				VSoftBodySEllipsoid.SetProperty<Compute::SoftBody::Desc::CV::SEllipsoid>("vector3 radius", &Compute::SoftBody::Desc::CV::SEllipsoid::Radius);
				VSoftBodySEllipsoid.SetProperty<Compute::SoftBody::Desc::CV::SEllipsoid>("int count", &Compute::SoftBody::Desc::CV::SEllipsoid::Count);
				VSoftBodySEllipsoid.SetProperty<Compute::SoftBody::Desc::CV::SEllipsoid>("bool enabled", &Compute::SoftBody::Desc::CV::SEllipsoid::Enabled);
				VSoftBodySEllipsoid.SetConstructor<Compute::SoftBody::Desc::CV::SEllipsoid>("void f()");

				TypeClass VSoftBodyCV = Engine->SetPod<Compute::SoftBody::Desc::CV>("physics_softbody_desc_cv");
				VSoftBodyCV.SetProperty<Compute::SoftBody::Desc::CV>("physics_softbody_desc_cv_sconvex convex", &Compute::SoftBody::Desc::CV::Convex);
				VSoftBodyCV.SetProperty<Compute::SoftBody::Desc::CV>("physics_softbody_desc_cv_srope rope", &Compute::SoftBody::Desc::CV::Rope);
				VSoftBodyCV.SetProperty<Compute::SoftBody::Desc::CV>("physics_softbody_desc_cv_spatch patch", &Compute::SoftBody::Desc::CV::Patch);
				VSoftBodyCV.SetProperty<Compute::SoftBody::Desc::CV>("physics_softbody_desc_cv_sellipsoid ellipsoid", &Compute::SoftBody::Desc::CV::Ellipsoid);
				VSoftBodyCV.SetConstructor<Compute::SoftBody::Desc::CV>("void f()");

				TypeClass VSoftBodySConfig = Engine->SetPod<Compute::SoftBody::Desc::SConfig>("physics_softbody_desc_config");
				VSoftBodySConfig.SetProperty<Compute::SoftBody::Desc::SConfig>("physics_soft_aero_model aero_model", &Compute::SoftBody::Desc::SConfig::AeroModel);
				VSoftBodySConfig.SetProperty<Compute::SoftBody::Desc::SConfig>("float vcf", &Compute::SoftBody::Desc::SConfig::VCF);
				VSoftBodySConfig.SetProperty<Compute::SoftBody::Desc::SConfig>("float dp", &Compute::SoftBody::Desc::SConfig::DP);
				VSoftBodySConfig.SetProperty<Compute::SoftBody::Desc::SConfig>("float dg", &Compute::SoftBody::Desc::SConfig::DG);
				VSoftBodySConfig.SetProperty<Compute::SoftBody::Desc::SConfig>("float lf", &Compute::SoftBody::Desc::SConfig::LF);
				VSoftBodySConfig.SetProperty<Compute::SoftBody::Desc::SConfig>("float pr", &Compute::SoftBody::Desc::SConfig::PR);
				VSoftBodySConfig.SetProperty<Compute::SoftBody::Desc::SConfig>("float vc", &Compute::SoftBody::Desc::SConfig::VC);
				VSoftBodySConfig.SetProperty<Compute::SoftBody::Desc::SConfig>("float df", &Compute::SoftBody::Desc::SConfig::DF);
				VSoftBodySConfig.SetProperty<Compute::SoftBody::Desc::SConfig>("float mt", &Compute::SoftBody::Desc::SConfig::MT);
				VSoftBodySConfig.SetProperty<Compute::SoftBody::Desc::SConfig>("float chr", &Compute::SoftBody::Desc::SConfig::CHR);
				VSoftBodySConfig.SetProperty<Compute::SoftBody::Desc::SConfig>("float khr", &Compute::SoftBody::Desc::SConfig::KHR);
				VSoftBodySConfig.SetProperty<Compute::SoftBody::Desc::SConfig>("float shr", &Compute::SoftBody::Desc::SConfig::SHR);
				VSoftBodySConfig.SetProperty<Compute::SoftBody::Desc::SConfig>("float ahr", &Compute::SoftBody::Desc::SConfig::AHR);
				VSoftBodySConfig.SetProperty<Compute::SoftBody::Desc::SConfig>("float srhr_cl", &Compute::SoftBody::Desc::SConfig::SRHR_CL);
				VSoftBodySConfig.SetProperty<Compute::SoftBody::Desc::SConfig>("float skhr_cl", &Compute::SoftBody::Desc::SConfig::SKHR_CL);
				VSoftBodySConfig.SetProperty<Compute::SoftBody::Desc::SConfig>("float sshr_cl", &Compute::SoftBody::Desc::SConfig::SSHR_CL);
				VSoftBodySConfig.SetProperty<Compute::SoftBody::Desc::SConfig>("float sr_splt_cl", &Compute::SoftBody::Desc::SConfig::SR_SPLT_CL);
				VSoftBodySConfig.SetProperty<Compute::SoftBody::Desc::SConfig>("float sk_splt_cl", &Compute::SoftBody::Desc::SConfig::SK_SPLT_CL);
				VSoftBodySConfig.SetProperty<Compute::SoftBody::Desc::SConfig>("float ss_splt_cl", &Compute::SoftBody::Desc::SConfig::SS_SPLT_CL);
				VSoftBodySConfig.SetProperty<Compute::SoftBody::Desc::SConfig>("float max_volume", &Compute::SoftBody::Desc::SConfig::MaxVolume);
				VSoftBodySConfig.SetProperty<Compute::SoftBody::Desc::SConfig>("float time_scale", &Compute::SoftBody::Desc::SConfig::TimeScale);
				VSoftBodySConfig.SetProperty<Compute::SoftBody::Desc::SConfig>("float drag", &Compute::SoftBody::Desc::SConfig::Drag);
				VSoftBodySConfig.SetProperty<Compute::SoftBody::Desc::SConfig>("float max_stress", &Compute::SoftBody::Desc::SConfig::MaxStress);
				VSoftBodySConfig.SetProperty<Compute::SoftBody::Desc::SConfig>("int clusters", &Compute::SoftBody::Desc::SConfig::Clusters);
				VSoftBodySConfig.SetProperty<Compute::SoftBody::Desc::SConfig>("int constraints", &Compute::SoftBody::Desc::SConfig::Constraints);
				VSoftBodySConfig.SetProperty<Compute::SoftBody::Desc::SConfig>("int viterations", &Compute::SoftBody::Desc::SConfig::VIterations);
				VSoftBodySConfig.SetProperty<Compute::SoftBody::Desc::SConfig>("int Piterations", &Compute::SoftBody::Desc::SConfig::PIterations);
				VSoftBodySConfig.SetProperty<Compute::SoftBody::Desc::SConfig>("int diterations", &Compute::SoftBody::Desc::SConfig::DIterations);
				VSoftBodySConfig.SetProperty<Compute::SoftBody::Desc::SConfig>("int citerations", &Compute::SoftBody::Desc::SConfig::CIterations);
				VSoftBodySConfig.SetProperty<Compute::SoftBody::Desc::SConfig>("int collisions", &Compute::SoftBody::Desc::SConfig::Collisions);
				VSoftBodySConfig.SetConstructor<Compute::SoftBody::Desc::SConfig>("void f()");

				TypeClass VSoftBodyDesc = Engine->SetPod<Compute::SoftBody::Desc>("physics_softbody_desc");
				VSoftBodyDesc.SetProperty<Compute::SoftBody::Desc>("physics_softbody_desc_cv shape", &Compute::SoftBody::Desc::Shape);
				VSoftBodyDesc.SetProperty<Compute::SoftBody::Desc>("physics_softbody_desc_config feature", &Compute::SoftBody::Desc::Config);
				VSoftBodyDesc.SetProperty<Compute::SoftBody::Desc>("float anticipation", &Compute::SoftBody::SoftBody::Desc::Anticipation);
				VSoftBodyDesc.SetProperty<Compute::SoftBody::Desc>("vector3 position", &Compute::SoftBody::SoftBody::Desc::Position);
				VSoftBodyDesc.SetProperty<Compute::SoftBody::Desc>("vector3 rotation", &Compute::SoftBody::SoftBody::Desc::Rotation);
				VSoftBodyDesc.SetProperty<Compute::SoftBody::Desc>("vector3 scale", &Compute::SoftBody::SoftBody::Desc::Scale);
				VSoftBodyDesc.SetConstructor<Compute::SoftBody::Desc>("void f()");

				RefClass VSoftBody = Engine->SetClass<Compute::SoftBody>("physics_softbody", false);
				TypeClass VSoftBodyRayCast = Engine->SetPod<Compute::SoftBody::RayCast>("physics_softbody_raycast");
				VSoftBodyRayCast.SetProperty<Compute::SoftBody::RayCast>("physics_softbody@ body", &Compute::SoftBody::RayCast::Body);
				VSoftBodyRayCast.SetProperty<Compute::SoftBody::RayCast>("physics_soft_feature feature", &Compute::SoftBody::RayCast::Feature);
				VSoftBodyRayCast.SetProperty<Compute::SoftBody::RayCast>("float fraction", &Compute::SoftBody::RayCast::Fraction);
				VSoftBodyRayCast.SetProperty<Compute::SoftBody::RayCast>("int32 index", &Compute::SoftBody::RayCast::Index);
				VSoftBodyRayCast.SetConstructor<Compute::SoftBody::RayCast>("void f()");

				VSoftBody.SetMethod("physics_softbody@ copy()", &Compute::SoftBody::Copy);
				VSoftBody.SetMethod("void activate(bool)", &Compute::SoftBody::Activate);
				VSoftBody.SetMethod("void synchronize(transform@+, bool)", &Compute::SoftBody::Synchronize);
				VSoftBody.SetMethodEx("array<int32>@ GetIndices() const", &SoftBodyGetIndices);
				VSoftBody.SetMethodEx("array<vertex>@ GetVertices() const", &SoftBodyGetVertices);
				VSoftBody.SetMethod("void get_bounding_box(vector3 &out, vector3 &out) const", &Compute::SoftBody::GetBoundingBox);
				VSoftBody.SetMethod("void set_contact_stiffness_and_damping(float, float)", &Compute::SoftBody::SetContactStiffnessAndDamping);
				VSoftBody.SetMethod<Compute::SoftBody, void, int, Compute::RigidBody*, bool, float>("void add_anchor(int32, physics_rigidbody@+, bool = false, float = 1)", &Compute::SoftBody::AddAnchor);
				VSoftBody.SetMethod<Compute::SoftBody, void, int, Compute::RigidBody*, const Compute::Vector3&, bool, float>("void add_anchor(int32, physics_rigidbody@+, const vector3 &in, bool = false, float = 1)", &Compute::SoftBody::AddAnchor);
				VSoftBody.SetMethod<Compute::SoftBody, void, const Compute::Vector3&>("void add_force(const vector3 &in)", &Compute::SoftBody::AddForce);
				VSoftBody.SetMethod<Compute::SoftBody, void, const Compute::Vector3&, int>("void add_force(const vector3 &in, int)", &Compute::SoftBody::AddForce);
				VSoftBody.SetMethod("void add_aero_force_to_node(const vector3 &in, int)", &Compute::SoftBody::AddAeroForceToNode);
				VSoftBody.SetMethod("void add_aero_force_to_face(const vector3 &in, int)", &Compute::SoftBody::AddAeroForceToFace);
				VSoftBody.SetMethod<Compute::SoftBody, void, const Compute::Vector3&>("void add_velocity(const vector3 &in)", &Compute::SoftBody::AddVelocity);
				VSoftBody.SetMethod<Compute::SoftBody, void, const Compute::Vector3&, int>("void add_velocity(const vector3 &in, int)", &Compute::SoftBody::AddVelocity);
				VSoftBody.SetMethod("void set_selocity(const vector3 &in)", &Compute::SoftBody::SetVelocity);
				VSoftBody.SetMethod("void set_mass(int, float)", &Compute::SoftBody::SetMass);
				VSoftBody.SetMethod("void set_total_mass(float, bool = false)", &Compute::SoftBody::SetTotalMass);
				VSoftBody.SetMethod("void set_total_density(float)", &Compute::SoftBody::SetTotalDensity);
				VSoftBody.SetMethod("void set_volume_mass(float)", &Compute::SoftBody::SetVolumeMass);
				VSoftBody.SetMethod("void set_volume_density(float)", &Compute::SoftBody::SetVolumeDensity);
				VSoftBody.SetMethod("void translate(const vector3 &in)", &Compute::SoftBody::Translate);
				VSoftBody.SetMethod("void rotate(const vector3 &in)", &Compute::SoftBody::Rotate);
				VSoftBody.SetMethod("void scale(const vector3 &in)", &Compute::SoftBody::Scale);
				VSoftBody.SetMethod("void set_rest_length_scale(float)", &Compute::SoftBody::SetRestLengthScale);
				VSoftBody.SetMethod("void set_pose(bool, bool)", &Compute::SoftBody::SetPose);
				VSoftBody.SetMethod("float get_mass(int) const", &Compute::SoftBody::GetMass);
				VSoftBody.SetMethod("float get_total_mass() const", &Compute::SoftBody::GetTotalMass);
				VSoftBody.SetMethod("float get_rest_length_scale() const", &Compute::SoftBody::GetRestLengthScale);
				VSoftBody.SetMethod("float get_volume() const", &Compute::SoftBody::GetVolume);
				VSoftBody.SetMethod("int generate_bending_constraints(int)", &Compute::SoftBody::GenerateBendingConstraints);
				VSoftBody.SetMethod("void randomize_constraints()", &Compute::SoftBody::RandomizeConstraints);
				VSoftBody.SetMethod("bool cut_link(int, int, float)", &Compute::SoftBody::CutLink);
				VSoftBody.SetMethod("bool ray_test(const vector3 &in, const vector3 &in, physics_softbody_raycast &out)", &Compute::SoftBody::RayTest);
				VSoftBody.SetMethod("void set_wind_velocity(const vector3 &in)", &Compute::SoftBody::SetWindVelocity);
				VSoftBody.SetMethod("vector3 get_wind_velocity() const", &Compute::SoftBody::GetWindVelocity);
				VSoftBody.SetMethod("void get_aabb(vector3 &out, vector3 &out) const", &Compute::SoftBody::GetAabb);
				VSoftBody.SetMethod("void set_spinning_friction(float)", &Compute::SoftBody::SetSpinningFriction);
				VSoftBody.SetMethod("vector3 get_linear_velocity() const", &Compute::SoftBody::GetLinearVelocity);
				VSoftBody.SetMethod("vector3 get_angular_velocity() const", &Compute::SoftBody::GetAngularVelocity);
				VSoftBody.SetMethod("vector3 get_center_position() const", &Compute::SoftBody::GetCenterPosition);
				VSoftBody.SetMethod("void set_activity(bool)", &Compute::SoftBody::SetActivity);
				VSoftBody.SetMethod("void set_as_ghost()", &Compute::SoftBody::SetAsGhost);
				VSoftBody.SetMethod("void set_as_normal()", &Compute::SoftBody::SetAsNormal);
				VSoftBody.SetMethod("void set_self_pointer()", &Compute::SoftBody::SetSelfPointer);
				VSoftBody.SetMethod("void set_world_transform(uptr@)", &Compute::SoftBody::SetWorldTransform);
				VSoftBody.SetMethod("void set_activation_state(physics_motion_state)", &Compute::SoftBody::SetActivationState);
				VSoftBody.SetMethod("void set_contact_stiffness(float)", &Compute::SoftBody::SetContactStiffness);
				VSoftBody.SetMethod("void set_contact_damping(float)", &Compute::SoftBody::SetContactDamping);
				VSoftBody.SetMethod("void set_friction(float)", &Compute::SoftBody::SetFriction);
				VSoftBody.SetMethod("void set_restitution(float)", &Compute::SoftBody::SetRestitution);
				VSoftBody.SetMethod("void set_hit_fraction(float)", &Compute::SoftBody::SetHitFraction);
				VSoftBody.SetMethod("void set_ccd_motion_threshold(float)", &Compute::SoftBody::SetCcdMotionThreshold);
				VSoftBody.SetMethod("void set_ccd_swept_sphere_radius(float)", &Compute::SoftBody::SetCcdSweptSphereRadius);
				VSoftBody.SetMethod("void set_contact_processing_threshold(float)", &Compute::SoftBody::SetContactProcessingThreshold);
				VSoftBody.SetMethod("void set_reactivation_time(float)", &Compute::SoftBody::SetDeactivationTime);
				VSoftBody.SetMethod("void set_rolling_friction(float)", &Compute::SoftBody::SetRollingFriction);
				VSoftBody.SetMethod("void set_anisotropic_friction(const vector3 &in)", &Compute::SoftBody::SetAnisotropicFriction);
				VSoftBody.SetMethod("void set_config(const physics_softbody_desc_config &in)", &Compute::SoftBody::SetConfig);
				VSoftBody.SetMethod("physics_shape get_collision_shape_type() const", &Compute::SoftBody::GetCollisionShapeType);
				VSoftBody.SetMethod("physics_motion_state get_activation_state() const", &Compute::SoftBody::GetActivationState);
				VSoftBody.SetMethod("vector3 get_anisotropic_friction() const", &Compute::SoftBody::GetAnisotropicFriction);
				VSoftBody.SetMethod("vector3 get_scale() const", &Compute::SoftBody::GetScale);
				VSoftBody.SetMethod("vector3 get_position() const", &Compute::SoftBody::GetPosition);
				VSoftBody.SetMethod("vector3 get_rotation() const", &Compute::SoftBody::GetRotation);
				VSoftBody.SetMethod("uptr@ get_world_transform() const", &Compute::SoftBody::GetWorldTransform);
				VSoftBody.SetMethod("uptr@ get() const", &Compute::SoftBody::Get);
				VSoftBody.SetMethod("bool is_active() const", &Compute::SoftBody::IsActive);
				VSoftBody.SetMethod("bool is_static() const", &Compute::SoftBody::IsStatic);
				VSoftBody.SetMethod("bool is_ghost() const", &Compute::SoftBody::IsGhost);
				VSoftBody.SetMethod("bool is_colliding() const", &Compute::SoftBody::IsColliding);
				VSoftBody.SetMethod("float get_spinning_friction() const", &Compute::SoftBody::GetSpinningFriction);
				VSoftBody.SetMethod("float get_contact_stiffness() const", &Compute::SoftBody::GetContactStiffness);
				VSoftBody.SetMethod("float get_contact_damping() const", &Compute::SoftBody::GetContactDamping);
				VSoftBody.SetMethod("float get_friction() const", &Compute::SoftBody::GetFriction);
				VSoftBody.SetMethod("float get_restitution() const", &Compute::SoftBody::GetRestitution);
				VSoftBody.SetMethod("float get_hit_fraction() const", &Compute::SoftBody::GetHitFraction);
				VSoftBody.SetMethod("float get_ccd_motion_threshold() const", &Compute::SoftBody::GetCcdMotionThreshold);
				VSoftBody.SetMethod("float get_ccd_swept_sphere_radius() const", &Compute::SoftBody::GetCcdSweptSphereRadius);
				VSoftBody.SetMethod("float get_contact_processing_threshold() const", &Compute::SoftBody::GetContactProcessingThreshold);
				VSoftBody.SetMethod("float get_deactivation_time() const", &Compute::SoftBody::GetDeactivationTime);
				VSoftBody.SetMethod("float get_rolling_friction() const", &Compute::SoftBody::GetRollingFriction);
				VSoftBody.SetMethod("usize get_collision_flags() const", &Compute::SoftBody::GetCollisionFlags);
				VSoftBody.SetMethod("usize get_vertices_count() const", &Compute::SoftBody::GetVerticesCount);
				VSoftBody.SetMethod("physics_softbody_desc& get_initial_state()", &Compute::SoftBody::GetInitialState);
				VSoftBody.SetMethod("physics_simulator@+ get_simulator() const", &Compute::SoftBody::GetSimulator);

				RefClass VConstraint = Engine->SetClass<Compute::Constraint>("physics_constraint", false);
				VConstraint.SetMethod("physics_constraint@ copy() const", &Compute::Constraint::Copy);
				VConstraint.SetMethod("physics_simulator@+ get_simulator() const", &Compute::Constraint::GetSimulator);
				VConstraint.SetMethod("uptr@ get() const", &Compute::Constraint::Get);
				VConstraint.SetMethod("uptr@ get_first() const", &Compute::Constraint::GetFirst);
				VConstraint.SetMethod("uptr@ get_second() const", &Compute::Constraint::GetSecond);
				VConstraint.SetMethod("bool has_collisions() const", &Compute::Constraint::HasCollisions);
				VConstraint.SetMethod("bool is_enabled() const", &Compute::Constraint::IsEnabled);
				VConstraint.SetMethod("bool is_active() const", &Compute::Constraint::IsActive);
				VConstraint.SetMethod("void set_breaking_impulse_threshold(float)", &Compute::Constraint::SetBreakingImpulseThreshold);
				VConstraint.SetMethod("void set_enabled(bool)", &Compute::Constraint::SetEnabled);
				VConstraint.SetMethod("float get_breaking_impulse_threshold() const", &Compute::Constraint::GetBreakingImpulseThreshold);

				TypeClass VPConstraintDesc = Engine->SetPod<Compute::PConstraint::Desc>("physics_pconstraint_desc");
				VPConstraintDesc.SetProperty<Compute::PConstraint::Desc>("physics_rigidbody@ target_a", &Compute::PConstraint::Desc::TargetA);
				VPConstraintDesc.SetProperty<Compute::PConstraint::Desc>("physics_rigidbody@ target_b", &Compute::PConstraint::Desc::TargetB);
				VPConstraintDesc.SetProperty<Compute::PConstraint::Desc>("vector3 pivot_a", &Compute::PConstraint::Desc::PivotA);
				VPConstraintDesc.SetProperty<Compute::PConstraint::Desc>("vector3 pivot_b", &Compute::PConstraint::Desc::PivotB);
				VPConstraintDesc.SetProperty<Compute::PConstraint::Desc>("bool collisions", &Compute::PConstraint::Desc::Collisions);
				VPConstraintDesc.SetConstructor<Compute::PConstraint::Desc>("void f()");

				RefClass VPConstraint = Engine->SetClass<Compute::PConstraint>("physics_pconstraint", false);
				VPConstraint.SetMethod("physics_pconstraint@ copy() const", &Compute::PConstraint::Copy);
				VPConstraint.SetMethod("physics_simulator@+ get_simulator() const", &Compute::PConstraint::GetSimulator);
				VPConstraint.SetMethod("uptr@ get() const", &Compute::PConstraint::Get);
				VPConstraint.SetMethod("uptr@ get_first() const", &Compute::PConstraint::GetFirst);
				VPConstraint.SetMethod("uptr@ get_second() const", &Compute::PConstraint::GetSecond);
				VPConstraint.SetMethod("bool has_collisions() const", &Compute::PConstraint::HasCollisions);
				VPConstraint.SetMethod("bool is_enabled() const", &Compute::PConstraint::IsEnabled);
				VPConstraint.SetMethod("bool is_active() const", &Compute::PConstraint::IsActive);
				VPConstraint.SetMethod("void set_breaking_impulse_threshold(float)", &Compute::PConstraint::SetBreakingImpulseThreshold);
				VPConstraint.SetMethod("void set_enabled(bool)", &Compute::PConstraint::SetEnabled);
				VPConstraint.SetMethod("float get_breaking_impulse_threshold() const", &Compute::PConstraint::GetBreakingImpulseThreshold);
				VPConstraint.SetMethod("void set_pivot_a(const vector3 &in)", &Compute::PConstraint::SetPivotA);
				VPConstraint.SetMethod("void set_pivot_b(const vector3 &in)", &Compute::PConstraint::SetPivotB);
				VPConstraint.SetMethod("vector3 get_pivot_a() const", &Compute::PConstraint::GetPivotA);
				VPConstraint.SetMethod("vector3 get_pivot_b() const", &Compute::PConstraint::GetPivotB);
				VPConstraint.SetMethod("physics_pconstraint_desc get_state() const", &Compute::PConstraint::GetState);

				TypeClass VHConstraintDesc = Engine->SetPod<Compute::HConstraint::Desc>("physics_hconstraint_desc");
				VHConstraintDesc.SetProperty<Compute::HConstraint::Desc>("physics_rigidbody@ target_a", &Compute::HConstraint::Desc::TargetA);
				VHConstraintDesc.SetProperty<Compute::HConstraint::Desc>("physics_rigidbody@ target_b", &Compute::HConstraint::Desc::TargetB);
				VHConstraintDesc.SetProperty<Compute::HConstraint::Desc>("bool references", &Compute::HConstraint::Desc::References);
				VHConstraintDesc.SetProperty<Compute::HConstraint::Desc>("bool collisions", &Compute::HConstraint::Desc::Collisions);
				VPConstraintDesc.SetConstructor<Compute::HConstraint::Desc>("void f()");

				RefClass VHConstraint = Engine->SetClass<Compute::HConstraint>("physics_hconstraint", false);
				VHConstraint.SetMethod("physics_hconstraint@ copy() const", &Compute::HConstraint::Copy);
				VHConstraint.SetMethod("physics_simulator@+ get_simulator() const", &Compute::HConstraint::GetSimulator);
				VHConstraint.SetMethod("uptr@ get() const", &Compute::HConstraint::Get);
				VHConstraint.SetMethod("uptr@ get_first() const", &Compute::HConstraint::GetFirst);
				VHConstraint.SetMethod("uptr@ get_second() const", &Compute::HConstraint::GetSecond);
				VHConstraint.SetMethod("bool has_collisions() const", &Compute::HConstraint::HasCollisions);
				VHConstraint.SetMethod("bool is_enabled() const", &Compute::HConstraint::IsEnabled);
				VHConstraint.SetMethod("bool is_active() const", &Compute::HConstraint::IsActive);
				VHConstraint.SetMethod("void set_breaking_impulse_threshold(float)", &Compute::HConstraint::SetBreakingImpulseThreshold);
				VHConstraint.SetMethod("void set_enabled(bool)", &Compute::HConstraint::SetEnabled);
				VHConstraint.SetMethod("float get_breaking_impulse_threshold() const", &Compute::HConstraint::GetBreakingImpulseThreshold);
				VHConstraint.SetMethod("void enable_angular_motor(bool, float, float)", &Compute::HConstraint::EnableAngularMotor);
				VHConstraint.SetMethod("void enable_motor(bool)", &Compute::HConstraint::EnableMotor);
				VHConstraint.SetMethod("void test_limit(const matrix4x4 &in, const matrix4x4 &in)", &Compute::HConstraint::TestLimit);
				VHConstraint.SetMethod("void set_frames(const matrix4x4 &in, const matrix4x4 &in)", &Compute::HConstraint::SetFrames);
				VHConstraint.SetMethod("void set_angular_only(bool)", &Compute::HConstraint::SetAngularOnly);
				VHConstraint.SetMethod("void set_max_motor_impulse(float)", &Compute::HConstraint::SetMaxMotorImpulse);
				VHConstraint.SetMethod("void set_motor_target_velocity(float)", &Compute::HConstraint::SetMotorTargetVelocity);
				VHConstraint.SetMethod("void set_motor_target(float, float)", &Compute::HConstraint::SetMotorTarget);
				VHConstraint.SetMethod("void set_limit(float Low, float High, float Softness = 0.9f, float BiasFactor = 0.3f, float RelaxationFactor = 1.0f)", &Compute::HConstraint::SetLimit);
				VHConstraint.SetMethod("void set_offset(bool Value)", &Compute::HConstraint::SetOffset);
				VHConstraint.SetMethod("void set_reference_to_a(bool Value)", &Compute::HConstraint::SetReferenceToA);
				VHConstraint.SetMethod("void set_axis(const vector3 &in)", &Compute::HConstraint::SetAxis);
				VHConstraint.SetMethod("int get_solve_limit() const", &Compute::HConstraint::GetSolveLimit);
				VHConstraint.SetMethod("float get_motor_target_velocity() const", &Compute::HConstraint::GetMotorTargetVelocity);
				VHConstraint.SetMethod("float get_max_motor_impulse() const", &Compute::HConstraint::GetMaxMotorImpulse);
				VHConstraint.SetMethod("float get_limit_sign() const", &Compute::HConstraint::GetLimitSign);
				VHConstraint.SetMethod<Compute::HConstraint, float>("float get_hinge_angle() const", &Compute::HConstraint::GetHingeAngle);
				VHConstraint.SetMethod<Compute::HConstraint, float, const Compute::Matrix4x4&, const Compute::Matrix4x4&>("float get_hinge_angle(const matrix4x4 &in, const matrix4x4 &in) const", &Compute::HConstraint::GetHingeAngle);
				VHConstraint.SetMethod("float get_lower_limit() const", &Compute::HConstraint::GetLowerLimit);
				VHConstraint.SetMethod("float get_upper_limit() const", &Compute::HConstraint::GetUpperLimit);
				VHConstraint.SetMethod("float get_limit_softness() const", &Compute::HConstraint::GetLimitSoftness);
				VHConstraint.SetMethod("float get_limit_bias_factor() const", &Compute::HConstraint::GetLimitBiasFactor);
				VHConstraint.SetMethod("float get_limit_relaxation_factor() const", &Compute::HConstraint::GetLimitRelaxationFactor);
				VHConstraint.SetMethod("bool has_limit() const", &Compute::HConstraint::HasLimit);
				VHConstraint.SetMethod("bool is_offset() const", &Compute::HConstraint::IsOffset);
				VHConstraint.SetMethod("bool is_reference_to_a() const", &Compute::HConstraint::IsReferenceToA);
				VHConstraint.SetMethod("bool is_angular_only() const", &Compute::HConstraint::IsAngularOnly);
				VHConstraint.SetMethod("bool is_angular_motor_enabled() const", &Compute::HConstraint::IsAngularMotorEnabled);
				VHConstraint.SetMethod("physics_hconstraint_desc& get_state()", &Compute::HConstraint::GetState);

				TypeClass VSConstraintDesc = Engine->SetPod<Compute::SConstraint::Desc>("physics_sconstraint_desc");
				VSConstraintDesc.SetProperty<Compute::SConstraint::Desc>("physics_rigidbody@ target_a", &Compute::SConstraint::Desc::TargetA);
				VSConstraintDesc.SetProperty<Compute::SConstraint::Desc>("physics_rigidbody@ target_b", &Compute::SConstraint::Desc::TargetB);
				VSConstraintDesc.SetProperty<Compute::SConstraint::Desc>("bool linear", &Compute::SConstraint::Desc::Linear);
				VSConstraintDesc.SetProperty<Compute::SConstraint::Desc>("bool collisions", &Compute::SConstraint::Desc::Collisions);
				VSConstraintDesc.SetConstructor<Compute::SConstraint::Desc>("void f()");

				RefClass VSConstraint = Engine->SetClass<Compute::SConstraint>("physics_sconstraint", false);
				VSConstraint.SetMethod("physics_sconstraint@ copy() const", &Compute::SConstraint::Copy);
				VSConstraint.SetMethod("physics_simulator@+ get_simulator() const", &Compute::SConstraint::GetSimulator);
				VSConstraint.SetMethod("uptr@ get() const", &Compute::SConstraint::Get);
				VSConstraint.SetMethod("uptr@ get_first() const", &Compute::SConstraint::GetFirst);
				VSConstraint.SetMethod("uptr@ get_second() const", &Compute::SConstraint::GetSecond);
				VSConstraint.SetMethod("bool has_collisions() const", &Compute::SConstraint::HasCollisions);
				VSConstraint.SetMethod("bool is_enabled() const", &Compute::SConstraint::IsEnabled);
				VSConstraint.SetMethod("bool is_active() const", &Compute::SConstraint::IsActive);
				VSConstraint.SetMethod("void set_breaking_impulse_threshold(float)", &Compute::SConstraint::SetBreakingImpulseThreshold);
				VSConstraint.SetMethod("void set_enabled(bool)", &Compute::SConstraint::SetEnabled);
				VSConstraint.SetMethod("float get_breaking_impulse_threshold() const", &Compute::SConstraint::GetBreakingImpulseThreshold);
				VSConstraint.SetMethod("void set_angular_motor_velocity(float)", &Compute::SConstraint::SetAngularMotorVelocity);
				VSConstraint.SetMethod("void set_linear_motor_velocity(float)", &Compute::SConstraint::SetLinearMotorVelocity);
				VSConstraint.SetMethod("void set_upper_linear_limit(float)", &Compute::SConstraint::SetUpperLinearLimit);
				VSConstraint.SetMethod("void set_lower_linear_limit(float)", &Compute::SConstraint::SetLowerLinearLimit);
				VSConstraint.SetMethod("void set_angular_damping_direction(float)", &Compute::SConstraint::SetAngularDampingDirection);
				VSConstraint.SetMethod("void set_linear_damping_direction(float)", &Compute::SConstraint::SetLinearDampingDirection);
				VSConstraint.SetMethod("void set_angular_damping_limit(float)", &Compute::SConstraint::SetAngularDampingLimit);
				VSConstraint.SetMethod("void set_linear_damping_limit(float)", &Compute::SConstraint::SetLinearDampingLimit);
				VSConstraint.SetMethod("void set_angular_damping_ortho(float)", &Compute::SConstraint::SetAngularDampingOrtho);
				VSConstraint.SetMethod("void set_linear_damping_ortho(float)", &Compute::SConstraint::SetLinearDampingOrtho);
				VSConstraint.SetMethod("void set_upper_angular_limit(float)", &Compute::SConstraint::SetUpperAngularLimit);
				VSConstraint.SetMethod("void set_lower_angular_limit(float)", &Compute::SConstraint::SetLowerAngularLimit);
				VSConstraint.SetMethod("void set_max_angular_motor_force(float)", &Compute::SConstraint::SetMaxAngularMotorForce);
				VSConstraint.SetMethod("void set_max_linear_motor_force(float)", &Compute::SConstraint::SetMaxLinearMotorForce);
				VSConstraint.SetMethod("void set_angular_restitution_direction(float)", &Compute::SConstraint::SetAngularRestitutionDirection);
				VSConstraint.SetMethod("void set_linear_restitution_direction(float)", &Compute::SConstraint::SetLinearRestitutionDirection);
				VSConstraint.SetMethod("void set_angular_restitution_limit(float)", &Compute::SConstraint::SetAngularRestitutionLimit);
				VSConstraint.SetMethod("void set_linear_restitution_limit(float)", &Compute::SConstraint::SetLinearRestitutionLimit);
				VSConstraint.SetMethod("void set_angular_restitution_ortho(float)", &Compute::SConstraint::SetAngularRestitutionOrtho);
				VSConstraint.SetMethod("void set_linear_restitution_ortho(float)", &Compute::SConstraint::SetLinearRestitutionOrtho);
				VSConstraint.SetMethod("void set_angular_softness_direction(float)", &Compute::SConstraint::SetAngularSoftnessDirection);
				VSConstraint.SetMethod("void SetLinearSoftness_direction(float)", &Compute::SConstraint::SetLinearSoftnessDirection);
				VSConstraint.SetMethod("void Set_angular_softness_limit(float)", &Compute::SConstraint::SetAngularSoftnessLimit);
				VSConstraint.SetMethod("void Set_linear_softness_limit(float)", &Compute::SConstraint::SetLinearSoftnessLimit);
				VSConstraint.SetMethod("void Set_angular_softness_ortho(float)", &Compute::SConstraint::SetAngularSoftnessOrtho);
				VSConstraint.SetMethod("void set_linear_softness_ortho(float)", &Compute::SConstraint::SetLinearSoftnessOrtho);
				VSConstraint.SetMethod("void set_powered_angular_motor(bool)", &Compute::SConstraint::SetPoweredAngularMotor);
				VSConstraint.SetMethod("void set_powered_linear_motor(bool)", &Compute::SConstraint::SetPoweredLinearMotor);
				VSConstraint.SetMethod("float getAngularMotorVelocity() const", &Compute::SConstraint::GetAngularMotorVelocity);
				VSConstraint.SetMethod("float get_linear_motor_velocity() const", &Compute::SConstraint::GetLinearMotorVelocity);
				VSConstraint.SetMethod("float get_upper_linear_limit() const", &Compute::SConstraint::GetUpperLinearLimit);
				VSConstraint.SetMethod("float get_lower_linear_limit() const", &Compute::SConstraint::GetLowerLinearLimit);
				VSConstraint.SetMethod("float get_angular_damping_direction() const", &Compute::SConstraint::GetAngularDampingDirection);
				VSConstraint.SetMethod("float get_linear_damping_direction() const", &Compute::SConstraint::GetLinearDampingDirection);
				VSConstraint.SetMethod("float get_angular_damping_limit() const", &Compute::SConstraint::GetAngularDampingLimit);
				VSConstraint.SetMethod("float get_linear_damping_limit() const", &Compute::SConstraint::GetLinearDampingLimit);
				VSConstraint.SetMethod("float get_angular_damping_ortho() const", &Compute::SConstraint::GetAngularDampingOrtho);
				VSConstraint.SetMethod("float get_linear_damping_ortho() const", &Compute::SConstraint::GetLinearDampingOrtho);
				VSConstraint.SetMethod("float get_upper_angular_limit() const", &Compute::SConstraint::GetUpperAngularLimit);
				VSConstraint.SetMethod("float get_lower_angular_limit() const", &Compute::SConstraint::GetLowerAngularLimit);
				VSConstraint.SetMethod("float get_max_angular_motor_force() const", &Compute::SConstraint::GetMaxAngularMotorForce);
				VSConstraint.SetMethod("float get_max_linear_motor_force() const", &Compute::SConstraint::GetMaxLinearMotorForce);
				VSConstraint.SetMethod("float get_angular_restitution_direction() const", &Compute::SConstraint::GetAngularRestitutionDirection);
				VSConstraint.SetMethod("float get_linear_restitution_direction() const", &Compute::SConstraint::GetLinearRestitutionDirection);
				VSConstraint.SetMethod("float get_angular_restitution_limit() const", &Compute::SConstraint::GetAngularRestitutionLimit);
				VSConstraint.SetMethod("float get_linear_restitution_limit() const", &Compute::SConstraint::GetLinearRestitutionLimit);
				VSConstraint.SetMethod("float get_angular_restitution_ortho() const", &Compute::SConstraint::GetAngularRestitutionOrtho);
				VSConstraint.SetMethod("float get_linearRestitution_ortho() const", &Compute::SConstraint::GetLinearRestitutionOrtho);
				VSConstraint.SetMethod("float get_angular_softness_direction() const", &Compute::SConstraint::GetAngularSoftnessDirection);
				VSConstraint.SetMethod("float get_linear_softness_direction() const", &Compute::SConstraint::GetLinearSoftnessDirection);
				VSConstraint.SetMethod("float get_angular_softness_limit() const", &Compute::SConstraint::GetAngularSoftnessLimit);
				VSConstraint.SetMethod("float get_linear_softness_limit() const", &Compute::SConstraint::GetLinearSoftnessLimit);
				VSConstraint.SetMethod("float get_angular_softness_ortho() const", &Compute::SConstraint::GetAngularSoftnessOrtho);
				VSConstraint.SetMethod("float get_linear_softness_ortho() const", &Compute::SConstraint::GetLinearSoftnessOrtho);
				VSConstraint.SetMethod("bool get_powered_angular_motor() const", &Compute::SConstraint::GetPoweredAngularMotor);
				VSConstraint.SetMethod("bool get_powered_linear_motor() const", &Compute::SConstraint::GetPoweredLinearMotor);
				VSConstraint.SetMethod("physics_sconstraint_desc& get_state()", &Compute::SConstraint::GetState);

				TypeClass VCTConstraintDesc = Engine->SetPod<Compute::CTConstraint::Desc>("physics_ctconstraint_desc");
				VCTConstraintDesc.SetProperty<Compute::CTConstraint::Desc>("physics_rigidbody@ target_a", &Compute::CTConstraint::Desc::TargetA);
				VCTConstraintDesc.SetProperty<Compute::CTConstraint::Desc>("physics_rigidbody@ target_b", &Compute::CTConstraint::Desc::TargetB);
				VCTConstraintDesc.SetProperty<Compute::CTConstraint::Desc>("bool collisions", &Compute::CTConstraint::Desc::Collisions);
				VCTConstraintDesc.SetConstructor<Compute::CTConstraint::Desc>("void f()");

				RefClass VCTConstraint = Engine->SetClass<Compute::CTConstraint>("physics_ctconstraint", false);
				VCTConstraint.SetMethod("physics_ctconstraint@ copy() const", &Compute::CTConstraint::Copy);
				VCTConstraint.SetMethod("physics_simulator@+ get_simulator() const", &Compute::CTConstraint::GetSimulator);
				VCTConstraint.SetMethod("uptr@ get() const", &Compute::CTConstraint::Get);
				VCTConstraint.SetMethod("uptr@ get_first() const", &Compute::CTConstraint::GetFirst);
				VCTConstraint.SetMethod("uptr@ get_second() const", &Compute::CTConstraint::GetSecond);
				VCTConstraint.SetMethod("bool has_collisions() const", &Compute::CTConstraint::HasCollisions);
				VCTConstraint.SetMethod("bool is_enabled() const", &Compute::CTConstraint::IsEnabled);
				VCTConstraint.SetMethod("bool is_active() const", &Compute::CTConstraint::IsActive);
				VCTConstraint.SetMethod("void set_breaking_impulse_threshold(float)", &Compute::CTConstraint::SetBreakingImpulseThreshold);
				VCTConstraint.SetMethod("void set_enabled(bool)", &Compute::CTConstraint::SetEnabled);
				VCTConstraint.SetMethod("float get_breaking_impulse_threshold() const", &Compute::CTConstraint::GetBreakingImpulseThreshold);
				VCTConstraint.SetMethod("void enable_motor(bool)", &Compute::CTConstraint::EnableMotor);
				VCTConstraint.SetMethod("void set_frames(const matrix4x4 &in, const matrix4x4 &in)", &Compute::CTConstraint::SetFrames);
				VCTConstraint.SetMethod("void set_angular_only(bool)", &Compute::CTConstraint::SetAngularOnly);
				VCTConstraint.SetMethod<Compute::CTConstraint, void, int, float>("void set_limit(int, float)", &Compute::CTConstraint::SetLimit);
				VCTConstraint.SetMethod<Compute::CTConstraint, void, float, float, float, float, float, float>("void set_limit(float, float, float, float = 1.f, float = 0.3f, float = 1.0f)", &Compute::CTConstraint::SetLimit);
				VCTConstraint.SetMethod("void set_damping(float)", &Compute::CTConstraint::SetDamping);
				VCTConstraint.SetMethod("void set_max_motor_impulse(float)", &Compute::CTConstraint::SetMaxMotorImpulse);
				VCTConstraint.SetMethod("void set_max_motor_impulse_normalized(float)", &Compute::CTConstraint::SetMaxMotorImpulseNormalized);
				VCTConstraint.SetMethod("void set_fix_thresh(float)", &Compute::CTConstraint::SetFixThresh);
				VCTConstraint.SetMethod("void set_motor_target(const quaternion &in)", &Compute::CTConstraint::SetMotorTarget);
				VCTConstraint.SetMethod("void set_motor_target_in_constraint_space(const quaternion &in)", &Compute::CTConstraint::SetMotorTargetInConstraintSpace);
				VCTConstraint.SetMethod("vector3 get_point_for_angle(float, float) const", &Compute::CTConstraint::GetPointForAngle);
				VCTConstraint.SetMethod("quaternion get_motor_target() const", &Compute::CTConstraint::GetMotorTarget);
				VCTConstraint.SetMethod("int get_solve_twist_limit() const", &Compute::CTConstraint::GetSolveTwistLimit);
				VCTConstraint.SetMethod("int get_solve_swing_limit() const", &Compute::CTConstraint::GetSolveSwingLimit);
				VCTConstraint.SetMethod("float get_twist_limit_sign() const", &Compute::CTConstraint::GetTwistLimitSign);
				VCTConstraint.SetMethod("float get_swing_span1() const", &Compute::CTConstraint::GetSwingSpan1);
				VCTConstraint.SetMethod("float get_swing_span2() const", &Compute::CTConstraint::GetSwingSpan2);
				VCTConstraint.SetMethod("float get_twist_span() const", &Compute::CTConstraint::GetTwistSpan);
				VCTConstraint.SetMethod("float get_limit_softness() const", &Compute::CTConstraint::GetLimitSoftness);
				VCTConstraint.SetMethod("float get_bias_factor() const", &Compute::CTConstraint::GetBiasFactor);
				VCTConstraint.SetMethod("float get_relaxation_factor() const", &Compute::CTConstraint::GetRelaxationFactor);
				VCTConstraint.SetMethod("float get_twist_angle() const", &Compute::CTConstraint::GetTwistAngle);
				VCTConstraint.SetMethod("float get_limit(int) const", &Compute::CTConstraint::GetLimit);
				VCTConstraint.SetMethod("float get_damping() const", &Compute::CTConstraint::GetDamping);
				VCTConstraint.SetMethod("float get_max_motor_impulse() const", &Compute::CTConstraint::GetMaxMotorImpulse);
				VCTConstraint.SetMethod("float get_fix_thresh() const", &Compute::CTConstraint::GetFixThresh);
				VCTConstraint.SetMethod("bool is_motor_enabled() const", &Compute::CTConstraint::IsMotorEnabled);
				VCTConstraint.SetMethod("bool is_max_motor_impulse_normalized() const", &Compute::CTConstraint::IsMaxMotorImpulseNormalized);
				VCTConstraint.SetMethod("bool is_past_swing_limit() const", &Compute::CTConstraint::IsPastSwingLimit);
				VCTConstraint.SetMethod("bool is_angular_only() const", &Compute::CTConstraint::IsAngularOnly);
				VCTConstraint.SetMethod("physics_ctconstraint_desc& get_state()", &Compute::CTConstraint::GetState);

				TypeClass VDF6ConstraintDesc = Engine->SetPod<Compute::DF6Constraint::Desc>("physics_df6constraint_desc");
				VDF6ConstraintDesc.SetProperty<Compute::DF6Constraint::Desc>("physics_rigidbody@ target_a", &Compute::DF6Constraint::Desc::TargetA);
				VDF6ConstraintDesc.SetProperty<Compute::DF6Constraint::Desc>("physics_rigidbody@ target_b", &Compute::DF6Constraint::Desc::TargetB);
				VDF6ConstraintDesc.SetProperty<Compute::DF6Constraint::Desc>("bool collisions", &Compute::DF6Constraint::Desc::Collisions);
				VDF6ConstraintDesc.SetConstructor<Compute::DF6Constraint::Desc>("void f()");

				RefClass VDF6Constraint = Engine->SetClass<Compute::DF6Constraint>("physics_df6constraint", false);
				VDF6Constraint.SetMethod("physics_df6constraint@ copy() const", &Compute::DF6Constraint::Copy);
				VDF6Constraint.SetMethod("physics_simulator@+ get_simulator() const", &Compute::DF6Constraint::GetSimulator);
				VDF6Constraint.SetMethod("uptr@ get() const", &Compute::DF6Constraint::Get);
				VDF6Constraint.SetMethod("uptr@ get_first() const", &Compute::DF6Constraint::GetFirst);
				VDF6Constraint.SetMethod("uptr@ get_second() const", &Compute::DF6Constraint::GetSecond);
				VDF6Constraint.SetMethod("bool has_collisions() const", &Compute::DF6Constraint::HasCollisions);
				VDF6Constraint.SetMethod("bool is_enabled() const", &Compute::DF6Constraint::IsEnabled);
				VDF6Constraint.SetMethod("bool is_active() const", &Compute::DF6Constraint::IsActive);
				VDF6Constraint.SetMethod("void set_breaking_impulse_threshold(float)", &Compute::DF6Constraint::SetBreakingImpulseThreshold);
				VDF6Constraint.SetMethod("void set_enabled(bool)", &Compute::DF6Constraint::SetEnabled);
				VDF6Constraint.SetMethod("float get_breaking_impulse_threshold() const", &Compute::DF6Constraint::GetBreakingImpulseThreshold);		
				VDF6Constraint.SetMethod("void enable_motor(int, bool)", &Compute::DF6Constraint::EnableMotor);
				VDF6Constraint.SetMethod("void enable_spring(int, bool)", &Compute::DF6Constraint::EnableSpring);
				VDF6Constraint.SetMethod("void set_frames(const matrix4x4 &in, const matrix4x4 &in)", &Compute::DF6Constraint::SetFrames);
				VDF6Constraint.SetMethod("void set_linear_lower_limit(const vector3 &in)", &Compute::DF6Constraint::SetLinearLowerLimit);
				VDF6Constraint.SetMethod("void set_linear_upper_limit(const vector3 &in)", &Compute::DF6Constraint::SetLinearUpperLimit);
				VDF6Constraint.SetMethod("void set_angular_lower_limit(const vector3 &in)", &Compute::DF6Constraint::SetAngularLowerLimit);
				VDF6Constraint.SetMethod("void set_angular_lower_limit_reversed(const vector3 &in)", &Compute::DF6Constraint::SetAngularLowerLimitReversed);
				VDF6Constraint.SetMethod("void set_angular_upper_limit(const vector3 &in)", &Compute::DF6Constraint::SetAngularUpperLimit);
				VDF6Constraint.SetMethod("void set_angular_upper_limit_reversed(const vector3 &in)", &Compute::DF6Constraint::SetAngularUpperLimitReversed);
				VDF6Constraint.SetMethod("void set_limit(int, float, float)", &Compute::DF6Constraint::SetLimit);
				VDF6Constraint.SetMethod("void set_limit_reversed(int, float, float)", &Compute::DF6Constraint::SetLimitReversed);
				VDF6Constraint.SetMethod("void set_rotation_order(physics_rotator)", &Compute::DF6Constraint::SetRotationOrder);
				VDF6Constraint.SetMethod("void set_axis(const vector3 &in, const vector3 &in)", &Compute::DF6Constraint::SetAxis);
				VDF6Constraint.SetMethod("void set_bounce(int, float)", &Compute::DF6Constraint::SetBounce);
				VDF6Constraint.SetMethod("void set_servo(int, bool)", &Compute::DF6Constraint::SetServo);
				VDF6Constraint.SetMethod("void set_target_velocity(int, float)", &Compute::DF6Constraint::SetTargetVelocity);
				VDF6Constraint.SetMethod("void set_servo_target(int, float)", &Compute::DF6Constraint::SetServoTarget);
				VDF6Constraint.SetMethod("void set_max_motor_force(int, float)", &Compute::DF6Constraint::SetMaxMotorForce);
				VDF6Constraint.SetMethod("void set_stiffness(int, float, bool = true)", &Compute::DF6Constraint::SetStiffness);
				VDF6Constraint.SetMethod<Compute::DF6Constraint, void>("void set_equilibrium_point()", &Compute::DF6Constraint::SetEquilibriumPoint);
				VDF6Constraint.SetMethod<Compute::DF6Constraint, void, int>("void set_equilibrium_point(int)", &Compute::DF6Constraint::SetEquilibriumPoint);
				VDF6Constraint.SetMethod<Compute::DF6Constraint, void, int, float>("void set_equilibrium_point(int, float)", &Compute::DF6Constraint::SetEquilibriumPoint);
				VDF6Constraint.SetMethod("vector3 get_angular_upper_limit() const", &Compute::DF6Constraint::GetAngularUpperLimit);
				VDF6Constraint.SetMethod("vector3 get_angular_upper_limit_reversed() const", &Compute::DF6Constraint::GetAngularUpperLimitReversed);
				VDF6Constraint.SetMethod("vector3 get_angular_lower_limit() const", &Compute::DF6Constraint::GetAngularLowerLimit);
				VDF6Constraint.SetMethod("vector3 get_angular_lower_limit_reversed() const", &Compute::DF6Constraint::GetAngularLowerLimitReversed);
				VDF6Constraint.SetMethod("vector3 get_linear_upper_limit() const", &Compute::DF6Constraint::GetLinearUpperLimit);
				VDF6Constraint.SetMethod("vector3 get_linear_lower_limit() const", &Compute::DF6Constraint::GetLinearLowerLimit);
				VDF6Constraint.SetMethod("vector3 get_axis(int) const", &Compute::DF6Constraint::GetAxis);
				VDF6Constraint.SetMethod("physics_rotator get_rotation_order() const", &Compute::DF6Constraint::GetRotationOrder);
				VDF6Constraint.SetMethod("float get_angle(int) const", &Compute::DF6Constraint::GetAngle);
				VDF6Constraint.SetMethod("float get_relative_pivot_position(int) const", &Compute::DF6Constraint::GetRelativePivotPosition);
				VDF6Constraint.SetMethod("bool is_limited(int) const", &Compute::DF6Constraint::IsLimited);
				VDF6Constraint.SetMethod("physics_df6constraint_desc& get_state()", &Compute::DF6Constraint::GetState);

				TypeClass VSimulatorDesc = Engine->SetPod<Compute::Simulator::Desc>("physics_simulator_desc");
				VSimulatorDesc.SetProperty<Compute::Simulator::Desc>("vector3 water_normal", &Compute::Simulator::Desc::WaterNormal);
				VSimulatorDesc.SetProperty<Compute::Simulator::Desc>("vector3 gravity", &Compute::Simulator::Desc::Gravity);
				VSimulatorDesc.SetProperty<Compute::Simulator::Desc>("float air_density", &Compute::Simulator::Desc::AirDensity);
				VSimulatorDesc.SetProperty<Compute::Simulator::Desc>("float water_density", &Compute::Simulator::Desc::WaterDensity);
				VSimulatorDesc.SetProperty<Compute::Simulator::Desc>("float water_offset", &Compute::Simulator::Desc::WaterOffset);
				VSimulatorDesc.SetProperty<Compute::Simulator::Desc>("float max_displacement", &Compute::Simulator::Desc::MaxDisplacement);
				VSimulatorDesc.SetProperty<Compute::Simulator::Desc>("bool enable_soft_body", &Compute::Simulator::Desc::EnableSoftBody);
				VSimulatorDesc.SetConstructor<Compute::Simulator::Desc>("void f()");

				VSimulator.SetProperty<Compute::Simulator>("float time_speed", &Compute::Simulator::TimeSpeed);
				VSimulator.SetProperty<Compute::Simulator>("int interpolate", &Compute::Simulator::Interpolate);
				VSimulator.SetProperty<Compute::Simulator>("bool active", &Compute::Simulator::Active);
				VSimulator.SetConstructor<Compute::Simulator, const Compute::Simulator::Desc&>("physics_simulator@ f(const physics_simulator_desc &in)");
				VSimulator.SetMethod("void set_gravity(const vector3 &in)", &Compute::Simulator::SetGravity);
				VSimulator.SetMethod<Compute::Simulator, void, const Compute::Vector3&, bool>("void set_linear_impulse(const vector3 &in, bool = false)", &Compute::Simulator::SetLinearImpulse);
				VSimulator.SetMethod<Compute::Simulator, void, const Compute::Vector3&, int, int, bool>("void set_linear_impulse(const vector3 &in, int, int, bool = false)", &Compute::Simulator::SetLinearImpulse);
				VSimulator.SetMethod<Compute::Simulator, void, const Compute::Vector3&, bool>("void set_angular_impulse(const vector3 &in, bool = false)", &Compute::Simulator::SetAngularImpulse);
				VSimulator.SetMethod<Compute::Simulator, void, const Compute::Vector3&, int, int, bool>("void set_angular_impulse(const vector3 &in, int, int, bool = false)", &Compute::Simulator::SetAngularImpulse);
				VSimulator.SetMethod<Compute::Simulator, void, const Compute::Vector3&, bool>("void create_linear_impulse(const vector3 &in, bool = false)", &Compute::Simulator::CreateLinearImpulse);
				VSimulator.SetMethod<Compute::Simulator, void, const Compute::Vector3&, int, int, bool>("void create_linear_impulse(const vector3 &in, int, int, bool = false)", &Compute::Simulator::CreateLinearImpulse);
				VSimulator.SetMethod<Compute::Simulator, void, const Compute::Vector3&, bool>("void create_angular_impulse(const vector3 &in, bool = false)", &Compute::Simulator::CreateAngularImpulse);
				VSimulator.SetMethod<Compute::Simulator, void, const Compute::Vector3&, int, int, bool>("void create_angular_impulse(const vector3 &in, int, int, bool = false)", &Compute::Simulator::CreateAngularImpulse);
				VSimulator.SetMethod("void add_softbody(physics_softbody@+)", &Compute::Simulator::AddSoftBody);
				VSimulator.SetMethod("void remove_softbody(physics_softbody@+)", &Compute::Simulator::RemoveSoftBody);
				VSimulator.SetMethod("void add_rigidbody(physics_rigidbody@+)", &Compute::Simulator::AddRigidBody);
				VSimulator.SetMethod("void remove_rigidbody(physics_rigidbody@+)", &Compute::Simulator::RemoveRigidBody);
				VSimulator.SetMethod("void add_constraint(physics_constraint@+)", &Compute::Simulator::AddConstraint);
				VSimulator.SetMethod("void remove_constraint(physics_constraint@+)", &Compute::Simulator::RemoveConstraint);
				VSimulator.SetMethod("void remove_all()", &Compute::Simulator::RemoveAll);
				VSimulator.SetMethod("void simulate(int, float, float)", &Compute::Simulator::Simulate);
				VSimulator.SetMethod<Compute::Simulator, Compute::RigidBody*, const Compute::RigidBody::Desc&>("physics_rigidbody@ create_rigidbody(const physics_rigidbody_desc &in)", &Compute::Simulator::CreateRigidBody);
				VSimulator.SetMethod<Compute::Simulator, Compute::RigidBody*, const Compute::RigidBody::Desc&, Compute::Transform*>("physics_rigidbody@ create_rigidbody(const physics_rigidbody_desc &in, transform@+)", &Compute::Simulator::CreateRigidBody);
				VSimulator.SetMethod<Compute::Simulator, Compute::SoftBody*, const Compute::SoftBody::Desc&>("physics_softbody@ create_softbody(const physics_softbody_desc &in)", &Compute::Simulator::CreateSoftBody);
				VSimulator.SetMethod<Compute::Simulator, Compute::SoftBody*, const Compute::SoftBody::Desc&, Compute::Transform*>("physics_softbody@ create_softbody(const physics_softbody_desc &in, transform@+)", &Compute::Simulator::CreateSoftBody);
				VSimulator.SetMethod("physics_pconstraint@ create_pconstraint(const physics_pconstraint_desc &in)", &Compute::Simulator::CreatePoint2PointConstraint);
				VSimulator.SetMethod("physics_hconstraint@ create_hconstraint(const physics_hconstraint_desc &in)", &Compute::Simulator::CreateHingeConstraint);
				VSimulator.SetMethod("physics_sconstraint@ create_sconstraint(const physics_sconstraint_desc &in)", &Compute::Simulator::CreateSliderConstraint);
				VSimulator.SetMethod("physics_ctconstraint@ create_ctconstraint(const physics_ctconstraint_desc &in)", &Compute::Simulator::CreateConeTwistConstraint);
				VSimulator.SetMethod("physics_df6constraint@ create_df6constraint(const physics_df6constraint_desc &in)", &Compute::Simulator::Create6DoFConstraint);
				VSimulator.SetMethod("uptr@ create_shape(physics_shape)", &Compute::Simulator::CreateShape);
				VSimulator.SetMethod("uptr@ create_cube(const vector3 &in = vector3(1, 1, 1))", &Compute::Simulator::CreateCube);
				VSimulator.SetMethod("uptr@ create_sphere(float = 1)", &Compute::Simulator::CreateSphere);
				VSimulator.SetMethod("uptr@ create_capsule(float = 1, float = 1)", &Compute::Simulator::CreateCapsule);
				VSimulator.SetMethod("uptr@ create_cone(float = 1, float = 1)", &Compute::Simulator::CreateCone);
				VSimulator.SetMethod("uptr@ create_cylinder(const vector3 &in = vector3(1, 1, 1))", &Compute::Simulator::CreateCylinder);
				VSimulator.SetMethodEx("uptr@ create_convex_hull(array<skin_vertex>@+)", &SimulatorCreateConvexHullSkinVertex);
				VSimulator.SetMethodEx("uptr@ create_convex_hull(array<vertex>@+)", &SimulatorCreateConvexHullVertex);
				VSimulator.SetMethodEx("uptr@ create_convex_hull(array<vector2>@+)", &SimulatorCreateConvexHullVector2);
				VSimulator.SetMethodEx("uptr@ create_convex_hull(array<vector3>@+)", &SimulatorCreateConvexHullVector3);
				VSimulator.SetMethodEx("uptr@ create_convex_hull(array<vector4>@+)", &SimulatorCreateConvexHullVector4);
				VSimulator.SetMethod<Compute::Simulator, btCollisionShape*, btCollisionShape*>("uptr@ create_convex_hull(uptr@)", &Compute::Simulator::CreateConvexHull);
				VSimulator.SetMethod("uptr@ try_clone_shape(uptr@)", &Compute::Simulator::TryCloneShape);
				VSimulator.SetMethod("uptr@ reuse_shape(uptr@)", &Compute::Simulator::ReuseShape);
				VSimulator.SetMethod("void free_shape(uptr@)", &Compute::Simulator::FreeShape);
				VSimulator.SetMethodEx("array<vector3>@ get_shape_vertices(uptr@) const", &SimulatorGetShapeVertices);
				VSimulator.SetMethod("usize get_shape_vertices_count(uptr@) const", &Compute::Simulator::GetShapeVerticesCount);
				VSimulator.SetMethod("float get_max_displacement() const", &Compute::Simulator::GetMaxDisplacement);
				VSimulator.SetMethod("float get_air_density() const", &Compute::Simulator::GetAirDensity);
				VSimulator.SetMethod("float get_water_offset() const", &Compute::Simulator::GetWaterOffset);
				VSimulator.SetMethod("float get_water_density() const", &Compute::Simulator::GetWaterDensity);
				VSimulator.SetMethod("vector3 get_water_normal() const", &Compute::Simulator::GetWaterNormal);
				VSimulator.SetMethod("vector3 get_gravity() const", &Compute::Simulator::GetGravity);
				VSimulator.SetMethod("bool has_softbody_support() const", &Compute::Simulator::HasSoftBodySupport);
				VSimulator.SetMethod("int get_contact_manifold_count() const", &Compute::Simulator::GetContactManifoldCount);

				VConstraint.SetDynamicCast<Compute::Constraint, Compute::PConstraint>("physics_pconstraint@+");
				VConstraint.SetDynamicCast<Compute::Constraint, Compute::HConstraint>("physics_hconstraint@+");
				VConstraint.SetDynamicCast<Compute::Constraint, Compute::SConstraint>("physics_sconstraint@+");
				VConstraint.SetDynamicCast<Compute::Constraint, Compute::CTConstraint>("physics_ctconstraint@+");
				VConstraint.SetDynamicCast<Compute::Constraint, Compute::DF6Constraint>("physics_df6constraint@+");
				VPConstraint.SetDynamicCast<Compute::PConstraint, Compute::Constraint>("physics_constraint@+", true);
				VHConstraint.SetDynamicCast<Compute::HConstraint, Compute::Constraint>("physics_constraint@+", true);
				VSConstraint.SetDynamicCast<Compute::SConstraint, Compute::Constraint>("physics_constraint@+", true);
				VCTConstraint.SetDynamicCast<Compute::CTConstraint, Compute::Constraint>("physics_constraint@+", true);
				VDF6Constraint.SetDynamicCast<Compute::DF6Constraint, Compute::Constraint>("physics_constraint@+", true);

				return true;
#else
				VI_ASSERT(false, false, "<physics> is not loaded");
				return false;
#endif
			}
			bool Registry::LoadAudio(VirtualMachine* Engine)
			{
#ifdef VI_HAS_BINDINGS
				VI_ASSERT(Engine != nullptr, false, "manager should be set");
				VI_TYPEREF(AudioSource, "audio_source");

				Enumeration VSoundDistanceModel = Engine->SetEnum("sound_distance_model");
				VSoundDistanceModel.SetValue("invalid", (int)Audio::SoundDistanceModel::Invalid);
				VSoundDistanceModel.SetValue("invert", (int)Audio::SoundDistanceModel::Invert);
				VSoundDistanceModel.SetValue("invert_clamp", (int)Audio::SoundDistanceModel::Invert_Clamp);
				VSoundDistanceModel.SetValue("linear", (int)Audio::SoundDistanceModel::Linear);
				VSoundDistanceModel.SetValue("linear_clamp", (int)Audio::SoundDistanceModel::Linear_Clamp);
				VSoundDistanceModel.SetValue("exponent", (int)Audio::SoundDistanceModel::Exponent);
				VSoundDistanceModel.SetValue("exponent_clamp", (int)Audio::SoundDistanceModel::Exponent_Clamp);

				Enumeration VSoundEx = Engine->SetEnum("sound_ex");
				VSoundEx.SetValue("source_relative", (int)Audio::SoundEx::Source_Relative);
				VSoundEx.SetValue("cone_inner_angle", (int)Audio::SoundEx::Cone_Inner_Angle);
				VSoundEx.SetValue("cone_outer_angle", (int)Audio::SoundEx::Cone_Outer_Angle);
				VSoundEx.SetValue("pitch", (int)Audio::SoundEx::Pitch);
				VSoundEx.SetValue("position", (int)Audio::SoundEx::Position);
				VSoundEx.SetValue("direction", (int)Audio::SoundEx::Direction);
				VSoundEx.SetValue("velocity", (int)Audio::SoundEx::Velocity);
				VSoundEx.SetValue("looping", (int)Audio::SoundEx::Looping);
				VSoundEx.SetValue("buffer", (int)Audio::SoundEx::Buffer);
				VSoundEx.SetValue("gain", (int)Audio::SoundEx::Gain);
				VSoundEx.SetValue("min_gain", (int)Audio::SoundEx::Min_Gain);
				VSoundEx.SetValue("max_gain", (int)Audio::SoundEx::Max_Gain);
				VSoundEx.SetValue("orientation", (int)Audio::SoundEx::Orientation);
				VSoundEx.SetValue("channel_mask", (int)Audio::SoundEx::Channel_Mask);
				VSoundEx.SetValue("source_state", (int)Audio::SoundEx::Source_State);
				VSoundEx.SetValue("initial", (int)Audio::SoundEx::Initial);
				VSoundEx.SetValue("playing", (int)Audio::SoundEx::Playing);
				VSoundEx.SetValue("paused", (int)Audio::SoundEx::Paused);
				VSoundEx.SetValue("stopped", (int)Audio::SoundEx::Stopped);
				VSoundEx.SetValue("buffers_queued", (int)Audio::SoundEx::Buffers_Queued);
				VSoundEx.SetValue("buffers_processed", (int)Audio::SoundEx::Buffers_Processed);
				VSoundEx.SetValue("seconds_offset", (int)Audio::SoundEx::Seconds_Offset);
				VSoundEx.SetValue("sample_offset", (int)Audio::SoundEx::Sample_Offset);
				VSoundEx.SetValue("byte_offset", (int)Audio::SoundEx::Byte_Offset);
				VSoundEx.SetValue("source_type", (int)Audio::SoundEx::Source_Type);
				VSoundEx.SetValue("static", (int)Audio::SoundEx::Static);
				VSoundEx.SetValue("streaming", (int)Audio::SoundEx::Streaming);
				VSoundEx.SetValue("undetermined", (int)Audio::SoundEx::Undetermined);
				VSoundEx.SetValue("format_mono8", (int)Audio::SoundEx::Format_Mono8);
				VSoundEx.SetValue("format_mono16", (int)Audio::SoundEx::Format_Mono16);
				VSoundEx.SetValue("format_stereo8", (int)Audio::SoundEx::Format_Stereo8);
				VSoundEx.SetValue("format_stereo16", (int)Audio::SoundEx::Format_Stereo16);
				VSoundEx.SetValue("reference_distance", (int)Audio::SoundEx::Reference_Distance);
				VSoundEx.SetValue("rolloff_gactor", (int)Audio::SoundEx::Rolloff_Factor);
				VSoundEx.SetValue("cone_outer_gain", (int)Audio::SoundEx::Cone_Outer_Gain);
				VSoundEx.SetValue("max_distance", (int)Audio::SoundEx::Max_Distance);
				VSoundEx.SetValue("frequency", (int)Audio::SoundEx::Frequency);
				VSoundEx.SetValue("bits", (int)Audio::SoundEx::Bits);
				VSoundEx.SetValue("channels", (int)Audio::SoundEx::Channels);
				VSoundEx.SetValue("size", (int)Audio::SoundEx::Size);
				VSoundEx.SetValue("unused", (int)Audio::SoundEx::Unused);
				VSoundEx.SetValue("pending", (int)Audio::SoundEx::Pending);
				VSoundEx.SetValue("processed", (int)Audio::SoundEx::Processed);
				VSoundEx.SetValue("invalid_name", (int)Audio::SoundEx::Invalid_Name);
				VSoundEx.SetValue("illegal_enum", (int)Audio::SoundEx::Illegal_Enum);
				VSoundEx.SetValue("invalid_enum", (int)Audio::SoundEx::Invalid_Enum);
				VSoundEx.SetValue("invalid_value", (int)Audio::SoundEx::Invalid_Value);
				VSoundEx.SetValue("illegal_command", (int)Audio::SoundEx::Illegal_Command);
				VSoundEx.SetValue("invalid_operation", (int)Audio::SoundEx::Invalid_Operation);
				VSoundEx.SetValue("out_of_memory", (int)Audio::SoundEx::Out_Of_Memory);
				VSoundEx.SetValue("vendor", (int)Audio::SoundEx::Vendor);
				VSoundEx.SetValue("version", (int)Audio::SoundEx::Version);
				VSoundEx.SetValue("renderer", (int)Audio::SoundEx::Renderer);
				VSoundEx.SetValue("extentions", (int)Audio::SoundEx::Extentions);
				VSoundEx.SetValue("doppler_factor", (int)Audio::SoundEx::Doppler_Factor);
				VSoundEx.SetValue("doppler_velocity", (int)Audio::SoundEx::Doppler_Velocity);
				VSoundEx.SetValue("speed_of_sound", (int)Audio::SoundEx::Speed_Of_Sound);

				TypeClass VAudioSync = Engine->SetPod<Audio::AudioSync>("audio_sync");
				VAudioSync.SetProperty<Audio::AudioSync>("vector3 direction", &Audio::AudioSync::Direction);
				VAudioSync.SetProperty<Audio::AudioSync>("vector3 velocity", &Audio::AudioSync::Velocity);
				VAudioSync.SetProperty<Audio::AudioSync>("float cone_inner_angle", &Audio::AudioSync::ConeInnerAngle);
				VAudioSync.SetProperty<Audio::AudioSync>("float cone_outer_angle", &Audio::AudioSync::ConeOuterAngle);
				VAudioSync.SetProperty<Audio::AudioSync>("float cone_outer_gain", &Audio::AudioSync::ConeOuterGain);
				VAudioSync.SetProperty<Audio::AudioSync>("float pitch", &Audio::AudioSync::Pitch);
				VAudioSync.SetProperty<Audio::AudioSync>("float gain", &Audio::AudioSync::Gain);
				VAudioSync.SetProperty<Audio::AudioSync>("float ref_distance", &Audio::AudioSync::RefDistance);
				VAudioSync.SetProperty<Audio::AudioSync>("float distance", &Audio::AudioSync::Distance);
				VAudioSync.SetProperty<Audio::AudioSync>("float rolloff", &Audio::AudioSync::Rolloff);
				VAudioSync.SetProperty<Audio::AudioSync>("float position", &Audio::AudioSync::Position);
				VAudioSync.SetProperty<Audio::AudioSync>("float air_absorption", &Audio::AudioSync::AirAbsorption);
				VAudioSync.SetProperty<Audio::AudioSync>("float room_roll_off", &Audio::AudioSync::RoomRollOff);
				VAudioSync.SetProperty<Audio::AudioSync>("bool is_relative", &Audio::AudioSync::IsRelative);
				VAudioSync.SetProperty<Audio::AudioSync>("bool is_looped", &Audio::AudioSync::IsLooped);
				VAudioSync.SetConstructor<Audio::AudioSync>("void f()");

				Engine->BeginNamespace("audio_context");
				Engine->SetFunction("void create()", Audio::AudioContext::Create);
				Engine->SetFunction("void release()", Audio::AudioContext::Release);
				Engine->SetFunction("void lock()", Audio::AudioContext::Lock);
				Engine->SetFunction("void unlock()", Audio::AudioContext::Unlock);
				Engine->SetFunction("void generate_buffers(int32, uint32 &out)", Audio::AudioContext::GenerateBuffers);
				Engine->SetFunction("void set_source_data_3f(uint32, sound_ex, float, float, float)", Audio::AudioContext::SetSourceData3F);
				Engine->SetFunction("void get_source_data_3f(uint32, sound_ex, float &out, float &out, float &out)", Audio::AudioContext::GetSourceData3F);
				Engine->SetFunction("void set_source_data_1f(uint32, sound_ex, float)", Audio::AudioContext::SetSourceData1F);
				Engine->SetFunction("void get_source_data_1f(uint32, sound_ex, float &out)", Audio::AudioContext::GetSourceData1F);
				Engine->SetFunction("void set_source_data_3i(uint32, sound_ex, int32, int32, int32)", Audio::AudioContext::SetSourceData3I);
				Engine->SetFunction("void get_source_data_3i(uint32, sound_ex, int32 &out, int32 &out, int32 &out)", Audio::AudioContext::GetSourceData3I);
				Engine->SetFunction("void set_source_data_1i(uint32, sound_ex, int32)", Audio::AudioContext::SetSourceData1I);
				Engine->SetFunction("void get_source_data_1i(uint32, sound_ex, int32 &out)", Audio::AudioContext::GetSourceData1I);
				Engine->SetFunction("void set_listener_data_3f(sound_ex, float, float, float)", Audio::AudioContext::SetListenerData3F);
				Engine->SetFunction("void get_listener_data_3f(sound_ex, float &out, float &out, float &out)", Audio::AudioContext::GetListenerData3F);
				Engine->SetFunction("void set_listener_data_1f(sound_ex, float)", Audio::AudioContext::SetListenerData1F);
				Engine->SetFunction("void get_listener_data_1f(sound_ex, float &out)", Audio::AudioContext::GetListenerData1F);
				Engine->SetFunction("void set_listener_data_3i(sound_ex, int32, int32, int32)", Audio::AudioContext::SetListenerData3I);
				Engine->SetFunction("void get_listener_data_3i(sound_ex, int32 &out, int32 &out, int32 &out)", Audio::AudioContext::GetListenerData3I);
				Engine->SetFunction("void set_listener_data_1i(sound_ex, int32)", Audio::AudioContext::SetListenerData1I);
				Engine->SetFunction("void get_listener_data_1i(sound_ex, int32 &out)", Audio::AudioContext::GetListenerData1I);
				Engine->EndNamespace();

				RefClass VAudioSource = Engine->SetClass<Audio::AudioSource>("audio_source", true);
				RefClass VAudioFilter = Engine->SetClass<Audio::AudioFilter>("base_audio_filter", false);
				PopulateAudioFilterBase<Audio::AudioFilter>(VAudioFilter);

				RefClass VAudioEffect = Engine->SetClass<Audio::AudioEffect>("base_audio_effect", false);
				PopulateAudioEffectBase<Audio::AudioEffect>(VAudioEffect);

				RefClass VAudioClip = Engine->SetClass<Audio::AudioClip>("audio_clip", false);
				VAudioClip.SetConstructor<Audio::AudioClip, int, int>("audio_clip@ f(int, int)");
				VAudioClip.SetMethod("float length() const", &Audio::AudioClip::Length);
				VAudioClip.SetMethod("bool is_mono() const", &Audio::AudioClip::IsMono);
				VAudioClip.SetMethod("uint32 get_buffer() const", &Audio::AudioClip::GetBuffer);
				VAudioClip.SetMethod("int32 get_format() const", &Audio::AudioClip::GetFormat);

				VAudioSource.SetGcConstructor<Audio::AudioSource, AudioSource>("audio_source@ f()");
				VAudioSource.SetMethod("int64 add_effect(base_audio_effect@+)", &Audio::AudioSource::AddEffect);
				VAudioSource.SetMethod("bool remove_effect(usize)", &Audio::AudioSource::RemoveEffect);
				VAudioSource.SetMethod("bool remove_effect_by_id(uint64)", &Audio::AudioSource::RemoveEffectById);
				VAudioSource.SetMethod("void set_clip(audio_clip@+)", &Audio::AudioSource::SetClip);
				VAudioSource.SetMethod("void synchronize(audio_sync &in, const vector3 &in)", &Audio::AudioSource::Synchronize);
				VAudioSource.SetMethod("void reset()", &Audio::AudioSource::Reset);
				VAudioSource.SetMethod("void pause()", &Audio::AudioSource::Pause);
				VAudioSource.SetMethod("void play()", &Audio::AudioSource::Play);
				VAudioSource.SetMethod("void stop()", &Audio::AudioSource::Stop);
				VAudioSource.SetMethod("bool is_playing() const", &Audio::AudioSource::IsPlaying);
				VAudioSource.SetMethod("usize get_effects_count() const", &Audio::AudioSource::GetEffectsCount);
				VAudioSource.SetMethod("uint32 get_instance() const", &Audio::AudioSource::GetInstance);
				VAudioSource.SetMethod("audio_clip@+ get_clip() const", &Audio::AudioSource::GetClip);
				VAudioSource.SetMethod<Audio::AudioSource, Audio::AudioEffect*, uint64_t>("base_audio_effect@+ get_effect(uint64) const", &Audio::AudioSource::GetEffect);
				VAudioSource.SetEnumRefsEx<Audio::AudioSource>([](Audio::AudioSource* Base, asIScriptEngine* Engine)
				{
					for (auto* Item : Base->GetEffects())
						Engine->GCEnumCallback(Item);
				});
				VAudioSource.SetReleaseRefsEx<Audio::AudioSource>([](Audio::AudioSource* Base, asIScriptEngine*)
				{
					Base->RemoveEffects();
				});

				RefClass VAudioDevice = Engine->SetClass<Audio::AudioDevice>("audio_device", false);
				VAudioDevice.SetConstructor<Audio::AudioDevice>("audio_device@ f()");
				VAudioDevice.SetMethod("void offset(audio_source@+, float &out, bool)", &Audio::AudioDevice::Offset);
				VAudioDevice.SetMethod("void velocity(audio_source@+, vector3 &out, bool)", &Audio::AudioDevice::Velocity);
				VAudioDevice.SetMethod("void position(audio_source@+, vector3 &out, bool)", &Audio::AudioDevice::Position);
				VAudioDevice.SetMethod("void direction(audio_source@+, vector3 &out, bool)", &Audio::AudioDevice::Direction);
				VAudioDevice.SetMethod("void relative(audio_source@+, int &out, bool)", &Audio::AudioDevice::Relative);
				VAudioDevice.SetMethod("void pitch(audio_source@+, float &out, bool)", &Audio::AudioDevice::Pitch);
				VAudioDevice.SetMethod("void gain(audio_source@+, float &out, bool)", &Audio::AudioDevice::Gain);
				VAudioDevice.SetMethod("void loop(audio_source@+, int &out, bool)", &Audio::AudioDevice::Loop);
				VAudioDevice.SetMethod("void cone_inner_angle(audio_source@+, float &out, bool)", &Audio::AudioDevice::ConeInnerAngle);
				VAudioDevice.SetMethod("void cone_outer_angle(audio_source@+, float &out, bool)", &Audio::AudioDevice::ConeOuterAngle);
				VAudioDevice.SetMethod("void cone_outer_gain(audio_source@+, float &out, bool)", &Audio::AudioDevice::ConeOuterGain);
				VAudioDevice.SetMethod("void distance(audio_source@+, float &out, bool)", &Audio::AudioDevice::Distance);
				VAudioDevice.SetMethod("void ref_distance(audio_source@+, float &out, bool)", &Audio::AudioDevice::RefDistance);
				VAudioDevice.SetMethod("void set_distance_model(sound_distance_model)", &Audio::AudioDevice::SetDistanceModel);
				VAudioDevice.SetMethod("void set_exception_codes(int32 &out, int32 &out) const", &Audio::AudioDevice::GetExceptionCodes);
				VAudioDevice.SetMethod("bool is_valid() const", &Audio::AudioDevice::IsValid);

				Engine->SetFunction("uint64 component_id(const string &in)", &Core::OS::File::GetHash);

				return true;
#else
				VI_ASSERT(false, false, "<audio> is not loaded");
				return false;
#endif
			}
			bool Registry::LoadActivity(VirtualMachine* Engine)
			{
#ifdef VI_HAS_BINDINGS
				VI_ASSERT(Engine != nullptr, false, "manager should be set");
				Enumeration VAppState = Engine->SetEnum("app_state");
				VAppState.SetValue("close_window", (int)Graphics::AppState::Close_Window);
				VAppState.SetValue("terminating", (int)Graphics::AppState::Terminating);
				VAppState.SetValue("low_memory", (int)Graphics::AppState::Low_Memory);
				VAppState.SetValue("enter_background_start", (int)Graphics::AppState::Enter_Background_Start);
				VAppState.SetValue("enter_background_end", (int)Graphics::AppState::Enter_Background_End);
				VAppState.SetValue("enter_foreground_start", (int)Graphics::AppState::Enter_Foreground_Start);
				VAppState.SetValue("enter_foreground_end", (int)Graphics::AppState::Enter_Foreground_End);

				Enumeration VWindowState = Engine->SetEnum("window_state");
				VWindowState.SetValue("show", (int)Graphics::WindowState::Show);
				VWindowState.SetValue("hide", (int)Graphics::WindowState::Hide);
				VWindowState.SetValue("expose", (int)Graphics::WindowState::Expose);
				VWindowState.SetValue("move", (int)Graphics::WindowState::Move);
				VWindowState.SetValue("resize", (int)Graphics::WindowState::Resize);
				VWindowState.SetValue("size_change", (int)Graphics::WindowState::Size_Change);
				VWindowState.SetValue("minimize", (int)Graphics::WindowState::Minimize);
				VWindowState.SetValue("maximize", (int)Graphics::WindowState::Maximize);
				VWindowState.SetValue("restore", (int)Graphics::WindowState::Restore);
				VWindowState.SetValue("enter", (int)Graphics::WindowState::Enter);
				VWindowState.SetValue("leave", (int)Graphics::WindowState::Leave);
				VWindowState.SetValue("focus", (int)Graphics::WindowState::Focus);
				VWindowState.SetValue("blur", (int)Graphics::WindowState::Blur);
				VWindowState.SetValue("close", (int)Graphics::WindowState::Close);

				Enumeration VKeyCode = Engine->SetEnum("key_code");
				VKeyCode.SetValue("a", (int)Graphics::KeyCode::A);
				VKeyCode.SetValue("b", (int)Graphics::KeyCode::B);
				VKeyCode.SetValue("c", (int)Graphics::KeyCode::C);
				VKeyCode.SetValue("d", (int)Graphics::KeyCode::D);
				VKeyCode.SetValue("e", (int)Graphics::KeyCode::E);
				VKeyCode.SetValue("f", (int)Graphics::KeyCode::F);
				VKeyCode.SetValue("g", (int)Graphics::KeyCode::G);
				VKeyCode.SetValue("h", (int)Graphics::KeyCode::H);
				VKeyCode.SetValue("i", (int)Graphics::KeyCode::I);
				VKeyCode.SetValue("j", (int)Graphics::KeyCode::J);
				VKeyCode.SetValue("k", (int)Graphics::KeyCode::K);
				VKeyCode.SetValue("l", (int)Graphics::KeyCode::L);
				VKeyCode.SetValue("m", (int)Graphics::KeyCode::M);
				VKeyCode.SetValue("n", (int)Graphics::KeyCode::N);
				VKeyCode.SetValue("o", (int)Graphics::KeyCode::O);
				VKeyCode.SetValue("p", (int)Graphics::KeyCode::P);
				VKeyCode.SetValue("q", (int)Graphics::KeyCode::Q);
				VKeyCode.SetValue("r", (int)Graphics::KeyCode::R);
				VKeyCode.SetValue("s", (int)Graphics::KeyCode::S);
				VKeyCode.SetValue("t", (int)Graphics::KeyCode::T);
				VKeyCode.SetValue("u", (int)Graphics::KeyCode::U);
				VKeyCode.SetValue("v", (int)Graphics::KeyCode::V);
				VKeyCode.SetValue("w", (int)Graphics::KeyCode::W);
				VKeyCode.SetValue("x", (int)Graphics::KeyCode::X);
				VKeyCode.SetValue("y", (int)Graphics::KeyCode::Y);
				VKeyCode.SetValue("z", (int)Graphics::KeyCode::Z);
				VKeyCode.SetValue("d1", (int)Graphics::KeyCode::D1);
				VKeyCode.SetValue("d2", (int)Graphics::KeyCode::D2);
				VKeyCode.SetValue("d3", (int)Graphics::KeyCode::D3);
				VKeyCode.SetValue("d4", (int)Graphics::KeyCode::D4);
				VKeyCode.SetValue("d5", (int)Graphics::KeyCode::D5);
				VKeyCode.SetValue("d6", (int)Graphics::KeyCode::D6);
				VKeyCode.SetValue("d7", (int)Graphics::KeyCode::D7);
				VKeyCode.SetValue("d8", (int)Graphics::KeyCode::D8);
				VKeyCode.SetValue("d9", (int)Graphics::KeyCode::D9);
				VKeyCode.SetValue("d0", (int)Graphics::KeyCode::D0);
				VKeyCode.SetValue("returns", (int)Graphics::KeyCode::Return);
				VKeyCode.SetValue("escape", (int)Graphics::KeyCode::Escape);
				VKeyCode.SetValue("backspace", (int)Graphics::KeyCode::Backspace);
				VKeyCode.SetValue("tab", (int)Graphics::KeyCode::Tab);
				VKeyCode.SetValue("space", (int)Graphics::KeyCode::Space);
				VKeyCode.SetValue("minus", (int)Graphics::KeyCode::Minus);
				VKeyCode.SetValue("equals", (int)Graphics::KeyCode::Equals);
				VKeyCode.SetValue("left_bracket", (int)Graphics::KeyCode::LeftBracket);
				VKeyCode.SetValue("right_bracket", (int)Graphics::KeyCode::RightBracket);
				VKeyCode.SetValue("backslash", (int)Graphics::KeyCode::Backslash);
				VKeyCode.SetValue("non_us_hash", (int)Graphics::KeyCode::NonUsHash);
				VKeyCode.SetValue("semicolon", (int)Graphics::KeyCode::Semicolon);
				VKeyCode.SetValue("apostrophe", (int)Graphics::KeyCode::Apostrophe);
				VKeyCode.SetValue("grave", (int)Graphics::KeyCode::Grave);
				VKeyCode.SetValue("comma", (int)Graphics::KeyCode::Comma);
				VKeyCode.SetValue("period", (int)Graphics::KeyCode::Period);
				VKeyCode.SetValue("slash", (int)Graphics::KeyCode::Slash);
				VKeyCode.SetValue("capslock", (int)Graphics::KeyCode::Capslock);
				VKeyCode.SetValue("f1", (int)Graphics::KeyCode::F1);
				VKeyCode.SetValue("f2", (int)Graphics::KeyCode::F2);
				VKeyCode.SetValue("f3", (int)Graphics::KeyCode::F3);
				VKeyCode.SetValue("f4", (int)Graphics::KeyCode::F4);
				VKeyCode.SetValue("f5", (int)Graphics::KeyCode::F5);
				VKeyCode.SetValue("f6", (int)Graphics::KeyCode::F6);
				VKeyCode.SetValue("f7", (int)Graphics::KeyCode::F7);
				VKeyCode.SetValue("f8", (int)Graphics::KeyCode::F8);
				VKeyCode.SetValue("f9", (int)Graphics::KeyCode::F9);
				VKeyCode.SetValue("f10", (int)Graphics::KeyCode::F10);
				VKeyCode.SetValue("f11", (int)Graphics::KeyCode::F11);
				VKeyCode.SetValue("f12", (int)Graphics::KeyCode::F12);
				VKeyCode.SetValue("print_screen", (int)Graphics::KeyCode::PrintScreen);
				VKeyCode.SetValue("scroll_lock", (int)Graphics::KeyCode::ScrollLock);
				VKeyCode.SetValue("pause", (int)Graphics::KeyCode::Pause);
				VKeyCode.SetValue("insert", (int)Graphics::KeyCode::Insert);
				VKeyCode.SetValue("home", (int)Graphics::KeyCode::Home);
				VKeyCode.SetValue("page_up", (int)Graphics::KeyCode::PageUp);
				VKeyCode.SetValue("delete", (int)Graphics::KeyCode::Delete);
				VKeyCode.SetValue("end", (int)Graphics::KeyCode::End);
				VKeyCode.SetValue("page_down", (int)Graphics::KeyCode::PageDown);
				VKeyCode.SetValue("right", (int)Graphics::KeyCode::Right);
				VKeyCode.SetValue("left", (int)Graphics::KeyCode::Left);
				VKeyCode.SetValue("down", (int)Graphics::KeyCode::Down);
				VKeyCode.SetValue("up", (int)Graphics::KeyCode::Up);
				VKeyCode.SetValue("num_lock_clear", (int)Graphics::KeyCode::NumLockClear);
				VKeyCode.SetValue("kp_divide", (int)Graphics::KeyCode::KpDivide);
				VKeyCode.SetValue("kp_multiply", (int)Graphics::KeyCode::KpMultiply);
				VKeyCode.SetValue("kp_minus", (int)Graphics::KeyCode::KpMinus);
				VKeyCode.SetValue("kp_plus", (int)Graphics::KeyCode::KpPlus);
				VKeyCode.SetValue("kp_enter", (int)Graphics::KeyCode::KpEnter);
				VKeyCode.SetValue("kp_1", (int)Graphics::KeyCode::Kp1);
				VKeyCode.SetValue("kp_2", (int)Graphics::KeyCode::Kp2);
				VKeyCode.SetValue("kp_3", (int)Graphics::KeyCode::Kp3);
				VKeyCode.SetValue("kp_4", (int)Graphics::KeyCode::Kp4);
				VKeyCode.SetValue("kp_5", (int)Graphics::KeyCode::Kp5);
				VKeyCode.SetValue("kp_6", (int)Graphics::KeyCode::Kp6);
				VKeyCode.SetValue("kp_7", (int)Graphics::KeyCode::Kp7);
				VKeyCode.SetValue("kp_8", (int)Graphics::KeyCode::Kp8);
				VKeyCode.SetValue("kp_9", (int)Graphics::KeyCode::Kp9);
				VKeyCode.SetValue("kp_0", (int)Graphics::KeyCode::Kp0);
				VKeyCode.SetValue("kp_period", (int)Graphics::KeyCode::KpPeriod);
				VKeyCode.SetValue("non_us_backslash", (int)Graphics::KeyCode::NonUsBackslash);
				VKeyCode.SetValue("app0", (int)Graphics::KeyCode::App0);
				VKeyCode.SetValue("power", (int)Graphics::KeyCode::Power);
				VKeyCode.SetValue("kp_equals", (int)Graphics::KeyCode::KpEquals);
				VKeyCode.SetValue("f13", (int)Graphics::KeyCode::F13);
				VKeyCode.SetValue("f14", (int)Graphics::KeyCode::F14);
				VKeyCode.SetValue("f15", (int)Graphics::KeyCode::F15);
				VKeyCode.SetValue("f16", (int)Graphics::KeyCode::F16);
				VKeyCode.SetValue("f17", (int)Graphics::KeyCode::F17);
				VKeyCode.SetValue("f18", (int)Graphics::KeyCode::F18);
				VKeyCode.SetValue("f19", (int)Graphics::KeyCode::F19);
				VKeyCode.SetValue("f20", (int)Graphics::KeyCode::F20);
				VKeyCode.SetValue("f21", (int)Graphics::KeyCode::F21);
				VKeyCode.SetValue("f22", (int)Graphics::KeyCode::F22);
				VKeyCode.SetValue("f23", (int)Graphics::KeyCode::F23);
				VKeyCode.SetValue("f24", (int)Graphics::KeyCode::F24);
				VKeyCode.SetValue("execute", (int)Graphics::KeyCode::Execute);
				VKeyCode.SetValue("help", (int)Graphics::KeyCode::Help);
				VKeyCode.SetValue("menu", (int)Graphics::KeyCode::Menu);
				VKeyCode.SetValue("select", (int)Graphics::KeyCode::Select);
				VKeyCode.SetValue("stop", (int)Graphics::KeyCode::Stop);
				VKeyCode.SetValue("again", (int)Graphics::KeyCode::Again);
				VKeyCode.SetValue("undo", (int)Graphics::KeyCode::Undo);
				VKeyCode.SetValue("cut", (int)Graphics::KeyCode::Cut);
				VKeyCode.SetValue("copy", (int)Graphics::KeyCode::Copy);
				VKeyCode.SetValue("paste", (int)Graphics::KeyCode::Paste);
				VKeyCode.SetValue("find", (int)Graphics::KeyCode::Find);
				VKeyCode.SetValue("mute", (int)Graphics::KeyCode::Mute);
				VKeyCode.SetValue("volume_up", (int)Graphics::KeyCode::VolumeUp);
				VKeyCode.SetValue("volume_down", (int)Graphics::KeyCode::VolumeDown);
				VKeyCode.SetValue("kp_comma", (int)Graphics::KeyCode::KpComma);
				VKeyCode.SetValue("kp_equals_as_400", (int)Graphics::KeyCode::KpEqualsAs400);
				VKeyCode.SetValue("international1", (int)Graphics::KeyCode::International1);
				VKeyCode.SetValue("international2", (int)Graphics::KeyCode::International2);
				VKeyCode.SetValue("international3", (int)Graphics::KeyCode::International3);
				VKeyCode.SetValue("international4", (int)Graphics::KeyCode::International4);
				VKeyCode.SetValue("international5", (int)Graphics::KeyCode::International5);
				VKeyCode.SetValue("international6", (int)Graphics::KeyCode::International6);
				VKeyCode.SetValue("international7", (int)Graphics::KeyCode::International7);
				VKeyCode.SetValue("international8", (int)Graphics::KeyCode::International8);
				VKeyCode.SetValue("international9", (int)Graphics::KeyCode::International9);
				VKeyCode.SetValue("lang1", (int)Graphics::KeyCode::Lang1);
				VKeyCode.SetValue("lang2", (int)Graphics::KeyCode::Lang2);
				VKeyCode.SetValue("lang3", (int)Graphics::KeyCode::Lang3);
				VKeyCode.SetValue("lang4", (int)Graphics::KeyCode::Lang4);
				VKeyCode.SetValue("lang5", (int)Graphics::KeyCode::Lang5);
				VKeyCode.SetValue("lang6", (int)Graphics::KeyCode::Lang6);
				VKeyCode.SetValue("lang7", (int)Graphics::KeyCode::Lang7);
				VKeyCode.SetValue("lang8", (int)Graphics::KeyCode::Lang8);
				VKeyCode.SetValue("lang9", (int)Graphics::KeyCode::Lang9);
				VKeyCode.SetValue("alterase", (int)Graphics::KeyCode::Alterase);
				VKeyCode.SetValue("sys_req", (int)Graphics::KeyCode::SysReq);
				VKeyCode.SetValue("cancel", (int)Graphics::KeyCode::Cancel);
				VKeyCode.SetValue("clear", (int)Graphics::KeyCode::Clear);
				VKeyCode.SetValue("prior", (int)Graphics::KeyCode::Prior);
				VKeyCode.SetValue("return2", (int)Graphics::KeyCode::Return2);
				VKeyCode.SetValue("separator", (int)Graphics::KeyCode::Separator);
				VKeyCode.SetValue("output", (int)Graphics::KeyCode::Output);
				VKeyCode.SetValue("operation", (int)Graphics::KeyCode::Operation);
				VKeyCode.SetValue("clear_again", (int)Graphics::KeyCode::ClearAgain);
				VKeyCode.SetValue("cr_select", (int)Graphics::KeyCode::CrSelect);
				VKeyCode.SetValue("ex_select", (int)Graphics::KeyCode::ExSelect);
				VKeyCode.SetValue("kp_00", (int)Graphics::KeyCode::Kp00);
				VKeyCode.SetValue("kp_000", (int)Graphics::KeyCode::Kp000);
				VKeyCode.SetValue("thousands_separator", (int)Graphics::KeyCode::ThousandsSeparator);
				VKeyCode.SetValue("decimals_separator", (int)Graphics::KeyCode::DecimalsSeparator);
				VKeyCode.SetValue("currency_unit", (int)Graphics::KeyCode::CurrencyUnit);
				VKeyCode.SetValue("currency_subunit", (int)Graphics::KeyCode::CurrencySubunit);
				VKeyCode.SetValue("kp_left_paren", (int)Graphics::KeyCode::KpLeftParen);
				VKeyCode.SetValue("kp_right_paren", (int)Graphics::KeyCode::KpRightParen);
				VKeyCode.SetValue("kp_left_brace", (int)Graphics::KeyCode::KpLeftBrace);
				VKeyCode.SetValue("kp_right_brace", (int)Graphics::KeyCode::KpRightBrace);
				VKeyCode.SetValue("kp_tab", (int)Graphics::KeyCode::KpTab);
				VKeyCode.SetValue("kp_backspace", (int)Graphics::KeyCode::KpBackspace);
				VKeyCode.SetValue("kp_a", (int)Graphics::KeyCode::KpA);
				VKeyCode.SetValue("kp_b", (int)Graphics::KeyCode::KpB);
				VKeyCode.SetValue("kp_c", (int)Graphics::KeyCode::KpC);
				VKeyCode.SetValue("kp_d", (int)Graphics::KeyCode::KpD);
				VKeyCode.SetValue("kp_e", (int)Graphics::KeyCode::KpE);
				VKeyCode.SetValue("kp_f", (int)Graphics::KeyCode::KpF);
				VKeyCode.SetValue("kp_xor", (int)Graphics::KeyCode::KpXOR);
				VKeyCode.SetValue("kp_power", (int)Graphics::KeyCode::KpPower);
				VKeyCode.SetValue("kp_percent", (int)Graphics::KeyCode::KpPercent);
				VKeyCode.SetValue("kp_less", (int)Graphics::KeyCode::KpLess);
				VKeyCode.SetValue("kp_greater", (int)Graphics::KeyCode::KpGreater);
				VKeyCode.SetValue("kp_ampersand", (int)Graphics::KeyCode::KpAmpersand);
				VKeyCode.SetValue("kp_dbl_ampersand", (int)Graphics::KeyCode::KpDBLAmpersand);
				VKeyCode.SetValue("kp_vertical_bar", (int)Graphics::KeyCode::KpVerticalBar);
				VKeyCode.SetValue("kp_dbl_vertical_bar", (int)Graphics::KeyCode::KpDBLVerticalBar);
				VKeyCode.SetValue("kp_colon", (int)Graphics::KeyCode::KpColon);
				VKeyCode.SetValue("kp_hash", (int)Graphics::KeyCode::KpHash);
				VKeyCode.SetValue("kp_space", (int)Graphics::KeyCode::KpSpace);
				VKeyCode.SetValue("kp_at", (int)Graphics::KeyCode::KpAt);
				VKeyCode.SetValue("kp_exclaim", (int)Graphics::KeyCode::KpExclaim);
				VKeyCode.SetValue("kp_mem_store", (int)Graphics::KeyCode::KpMemStore);
				VKeyCode.SetValue("kp_mem_recall", (int)Graphics::KeyCode::KpMemRecall);
				VKeyCode.SetValue("kp_mem_clear", (int)Graphics::KeyCode::KpMemClear);
				VKeyCode.SetValue("kp_mem_add", (int)Graphics::KeyCode::KpMemAdd);
				VKeyCode.SetValue("kp_mem_subtract", (int)Graphics::KeyCode::KpMemSubtract);
				VKeyCode.SetValue("kp_mem_multiply", (int)Graphics::KeyCode::KpMemMultiply);
				VKeyCode.SetValue("kp_mem_divide", (int)Graphics::KeyCode::KpMemDivide);
				VKeyCode.SetValue("kp_plus_minus", (int)Graphics::KeyCode::KpPlusMinus);
				VKeyCode.SetValue("kp_clear", (int)Graphics::KeyCode::KpClear);
				VKeyCode.SetValue("kp_clear_entry", (int)Graphics::KeyCode::KpClearEntry);
				VKeyCode.SetValue("kp_binary", (int)Graphics::KeyCode::KpBinary);
				VKeyCode.SetValue("kp_octal", (int)Graphics::KeyCode::KpOctal);
				VKeyCode.SetValue("kp_decimal", (int)Graphics::KeyCode::KpDecimal);
				VKeyCode.SetValue("kp_hexadecimal", (int)Graphics::KeyCode::KpHexadecimal);
				VKeyCode.SetValue("left_control", (int)Graphics::KeyCode::LeftControl);
				VKeyCode.SetValue("left_shift", (int)Graphics::KeyCode::LeftShift);
				VKeyCode.SetValue("left_alt", (int)Graphics::KeyCode::LeftAlt);
				VKeyCode.SetValue("left_gui", (int)Graphics::KeyCode::LeftGUI);
				VKeyCode.SetValue("right_control", (int)Graphics::KeyCode::RightControl);
				VKeyCode.SetValue("right_shift", (int)Graphics::KeyCode::RightShift);
				VKeyCode.SetValue("right_alt", (int)Graphics::KeyCode::RightAlt);
				VKeyCode.SetValue("right_gui", (int)Graphics::KeyCode::RightGUI);
				VKeyCode.SetValue("mode", (int)Graphics::KeyCode::Mode);
				VKeyCode.SetValue("audio_next", (int)Graphics::KeyCode::AudioNext);
				VKeyCode.SetValue("audio_prev", (int)Graphics::KeyCode::AudioPrev);
				VKeyCode.SetValue("audio_stop", (int)Graphics::KeyCode::AudioStop);
				VKeyCode.SetValue("audio_play", (int)Graphics::KeyCode::AudioPlay);
				VKeyCode.SetValue("audio_mute", (int)Graphics::KeyCode::AudioMute);
				VKeyCode.SetValue("media_select", (int)Graphics::KeyCode::MediaSelect);
				VKeyCode.SetValue("www", (int)Graphics::KeyCode::WWW);
				VKeyCode.SetValue("mail", (int)Graphics::KeyCode::Mail);
				VKeyCode.SetValue("calculator", (int)Graphics::KeyCode::Calculator);
				VKeyCode.SetValue("computer", (int)Graphics::KeyCode::Computer);
				VKeyCode.SetValue("ac_search", (int)Graphics::KeyCode::AcSearch);
				VKeyCode.SetValue("ac_home", (int)Graphics::KeyCode::AcHome);
				VKeyCode.SetValue("ac_back", (int)Graphics::KeyCode::AcBack);
				VKeyCode.SetValue("ac_forward", (int)Graphics::KeyCode::AcForward);
				VKeyCode.SetValue("ac_stop", (int)Graphics::KeyCode::AcStop);
				VKeyCode.SetValue("ac_refresh", (int)Graphics::KeyCode::AcRefresh);
				VKeyCode.SetValue("ac_bookmarks", (int)Graphics::KeyCode::AcBookmarks);
				VKeyCode.SetValue("brightness_down", (int)Graphics::KeyCode::BrightnessDown);
				VKeyCode.SetValue("brightness_up", (int)Graphics::KeyCode::BrightnessUp);
				VKeyCode.SetValue("display_switch", (int)Graphics::KeyCode::DisplaySwitch);
				VKeyCode.SetValue("kb_illum_toggle", (int)Graphics::KeyCode::KbIllumToggle);
				VKeyCode.SetValue("kb_illum_down", (int)Graphics::KeyCode::KbIllumDown);
				VKeyCode.SetValue("kb_illum_up", (int)Graphics::KeyCode::KbIllumUp);
				VKeyCode.SetValue("eject", (int)Graphics::KeyCode::Eject);
				VKeyCode.SetValue("sleep", (int)Graphics::KeyCode::Sleep);
				VKeyCode.SetValue("app1", (int)Graphics::KeyCode::App1);
				VKeyCode.SetValue("app2", (int)Graphics::KeyCode::App2);
				VKeyCode.SetValue("audio_rewind", (int)Graphics::KeyCode::AudioRewind);
				VKeyCode.SetValue("audio_fast_forward", (int)Graphics::KeyCode::AudioFastForward);
				VKeyCode.SetValue("cursor_left", (int)Graphics::KeyCode::CursorLeft);
				VKeyCode.SetValue("cursor_middle", (int)Graphics::KeyCode::CursorMiddle);
				VKeyCode.SetValue("cursor_right", (int)Graphics::KeyCode::CursorRight);
				VKeyCode.SetValue("cursor_x1", (int)Graphics::KeyCode::CursorX1);
				VKeyCode.SetValue("cursor_x2", (int)Graphics::KeyCode::CursorX2);
				VKeyCode.SetValue("none", (int)Graphics::KeyCode::None);

				Enumeration VKeyMod = Engine->SetEnum("key_mod");
				VKeyMod.SetValue("none", (int)Graphics::KeyMod::None);
				VKeyMod.SetValue("left_shift", (int)Graphics::KeyMod::LeftShift);
				VKeyMod.SetValue("right_shift", (int)Graphics::KeyMod::RightShift);
				VKeyMod.SetValue("left_control", (int)Graphics::KeyMod::LeftControl);
				VKeyMod.SetValue("right_control", (int)Graphics::KeyMod::RightControl);
				VKeyMod.SetValue("left_alt", (int)Graphics::KeyMod::LeftAlt);
				VKeyMod.SetValue("right_alt", (int)Graphics::KeyMod::RightAlt);
				VKeyMod.SetValue("left_gui", (int)Graphics::KeyMod::LeftGUI);
				VKeyMod.SetValue("right_gui", (int)Graphics::KeyMod::RightGUI);
				VKeyMod.SetValue("num", (int)Graphics::KeyMod::Num);
				VKeyMod.SetValue("caps", (int)Graphics::KeyMod::Caps);
				VKeyMod.SetValue("mode", (int)Graphics::KeyMod::Mode);
				VKeyMod.SetValue("reserved", (int)Graphics::KeyMod::Reserved);
				VKeyMod.SetValue("shift", (int)Graphics::KeyMod::Shift);
				VKeyMod.SetValue("control", (int)Graphics::KeyMod::Control);
				VKeyMod.SetValue("alt", (int)Graphics::KeyMod::Alt);
				VKeyMod.SetValue("gui", (int)Graphics::KeyMod::GUI);

				Enumeration VAlertType = Engine->SetEnum("alert_type");
				VAlertType.SetValue("none", (int)Graphics::AlertType::None);
				VAlertType.SetValue("error", (int)Graphics::AlertType::Error);
				VAlertType.SetValue("warning", (int)Graphics::AlertType::Warning);
				VAlertType.SetValue("info", (int)Graphics::AlertType::Info);

				Enumeration VAlertConfirm = Engine->SetEnum("alert_confirm");
				VAlertConfirm.SetValue("none", (int)Graphics::AlertConfirm::None);
				VAlertConfirm.SetValue("returns", (int)Graphics::AlertConfirm::Return);
				VAlertConfirm.SetValue("escape", (int)Graphics::AlertConfirm::Escape);

				Enumeration VJoyStickHat = Engine->SetEnum("joy_stick_hat");
				VJoyStickHat.SetValue("center", (int)Graphics::JoyStickHat::Center);
				VJoyStickHat.SetValue("up", (int)Graphics::JoyStickHat::Up);
				VJoyStickHat.SetValue("right", (int)Graphics::JoyStickHat::Right);
				VJoyStickHat.SetValue("down", (int)Graphics::JoyStickHat::Down);
				VJoyStickHat.SetValue("left", (int)Graphics::JoyStickHat::Left);
				VJoyStickHat.SetValue("right_up", (int)Graphics::JoyStickHat::Right_Up);
				VJoyStickHat.SetValue("right_down", (int)Graphics::JoyStickHat::Right_Down);
				VJoyStickHat.SetValue("left_up", (int)Graphics::JoyStickHat::Left_Up);
				VJoyStickHat.SetValue("left_down", (int)Graphics::JoyStickHat::Left_Down);

				Enumeration VRenderBackend = Engine->SetEnum("render_backend");
				VRenderBackend.SetValue("none", (int)Graphics::RenderBackend::None);
				VRenderBackend.SetValue("automatic", (int)Graphics::RenderBackend::Automatic);
				VRenderBackend.SetValue("d3d11", (int)Graphics::RenderBackend::D3D11);
				VRenderBackend.SetValue("ogl", (int)Graphics::RenderBackend::OGL);

				Enumeration VDisplayCursor = Engine->SetEnum("display_cursor");
				VDisplayCursor.SetValue("none", (int)Graphics::DisplayCursor::None);
				VDisplayCursor.SetValue("arrow", (int)Graphics::DisplayCursor::Arrow);
				VDisplayCursor.SetValue("text_input", (int)Graphics::DisplayCursor::TextInput);
				VDisplayCursor.SetValue("resize_all", (int)Graphics::DisplayCursor::ResizeAll);
				VDisplayCursor.SetValue("resize_ns", (int)Graphics::DisplayCursor::ResizeNS);
				VDisplayCursor.SetValue("resize_ew", (int)Graphics::DisplayCursor::ResizeEW);
				VDisplayCursor.SetValue("resize_nesw", (int)Graphics::DisplayCursor::ResizeNESW);
				VDisplayCursor.SetValue("resize_nwse", (int)Graphics::DisplayCursor::ResizeNWSE);
				VDisplayCursor.SetValue("hand", (int)Graphics::DisplayCursor::Hand);
				VDisplayCursor.SetValue("crosshair", (int)Graphics::DisplayCursor::Crosshair);
				VDisplayCursor.SetValue("wait", (int)Graphics::DisplayCursor::Wait);
				VDisplayCursor.SetValue("progress", (int)Graphics::DisplayCursor::Progress);
				VDisplayCursor.SetValue("no", (int)Graphics::DisplayCursor::No);

				TypeClass VKeyMap = Engine->SetPod<Graphics::KeyMap>("key_map");
				VKeyMap.SetProperty<Graphics::KeyMap>("key_code key", &Graphics::KeyMap::Key);
				VKeyMap.SetProperty<Graphics::KeyMap>("key_mod Mod", &Graphics::KeyMap::Mod);
				VKeyMap.SetProperty<Graphics::KeyMap>("bool Normal", &Graphics::KeyMap::Normal);
				VKeyMap.SetConstructor<Graphics::KeyMap>("void f()");
				VKeyMap.SetConstructor<Graphics::KeyMap, const Graphics::KeyCode&>("void f(const key_code &in)");
				VKeyMap.SetConstructor<Graphics::KeyMap, const Graphics::KeyMod&>("void f(const key_mod &in)");
				VKeyMap.SetConstructor<Graphics::KeyMap, const Graphics::KeyCode&, const Graphics::KeyMod&>("void f(const key_code &in, const key_mod &in)");

				TypeClass VViewport = Engine->SetPod<Graphics::Viewport>("viewport");
				VViewport.SetProperty<Graphics::Viewport>("float top_left_x", &Graphics::Viewport::TopLeftX);
				VViewport.SetProperty<Graphics::Viewport>("float top_left_y", &Graphics::Viewport::TopLeftY);
				VViewport.SetProperty<Graphics::Viewport>("float width", &Graphics::Viewport::Width);
				VViewport.SetProperty<Graphics::Viewport>("float height", &Graphics::Viewport::Height);
				VViewport.SetProperty<Graphics::Viewport>("float min_depth", &Graphics::Viewport::MinDepth);
				VViewport.SetProperty<Graphics::Viewport>("float max_depth", &Graphics::Viewport::MaxDepth);
				VViewport.SetConstructor<Graphics::Viewport>("void f()");

				RefClass VActivity = Engine->SetClass<Graphics::Activity>("activity", false);
				TypeClass VAlert = Engine->SetStructTrivial<Graphics::Alert>("activity_alert");
				VAlert.SetFunctionDef("void alert_event(int)");
				VAlert.SetConstructor<Graphics::Alert, Graphics::Activity*>("void f(activity@+)");
				VAlert.SetMethod("void setup(alert_type, const string &in, const string &in)", &Graphics::Alert::Setup);
				VAlert.SetMethod("void button(alert_confirm, const string &in, int32)", &Graphics::Alert::Button);
				VAlert.SetMethodEx("void result(alert_event@+)", &AlertResult);

				RefClass VSurface = Engine->SetClass<Graphics::Surface>("activity_surface", false);
				VSurface.SetConstructor<Graphics::Surface>("activity_surface@ f()");
				VSurface.SetConstructor<Graphics::Surface, SDL_Surface*>("activity_surface@ f(uptr@)");
				VSurface.SetMethod("void set_handle(uptr@)", &Graphics::Surface::SetHandle);
				VSurface.SetMethod("void lock()", &Graphics::Surface::Lock);
				VSurface.SetMethod("void unlock()", &Graphics::Surface::Unlock);
				VSurface.SetMethod("int get_width()", &Graphics::Surface::GetWidth);
				VSurface.SetMethod("int get_height()", &Graphics::Surface::GetHeight);
				VSurface.SetMethod("int get_pitch()", &Graphics::Surface::GetPitch);
				VSurface.SetMethod("uptr@ get_pixels()", &Graphics::Surface::GetPixels);
				VSurface.SetMethod("uptr@ get_resource()", &Graphics::Surface::GetResource);

				TypeClass VActivityDesc = Engine->SetPod<Graphics::Activity::Desc>("activity_desc");
				VActivityDesc.SetProperty<Graphics::Activity::Desc>("string title", &Graphics::Activity::Desc::Title);
				VActivityDesc.SetProperty<Graphics::Activity::Desc>("uint32 width", &Graphics::Activity::Desc::Width);
				VActivityDesc.SetProperty<Graphics::Activity::Desc>("uint32 height", &Graphics::Activity::Desc::Height);
				VActivityDesc.SetProperty<Graphics::Activity::Desc>("uint32 x", &Graphics::Activity::Desc::X);
				VActivityDesc.SetProperty<Graphics::Activity::Desc>("uint32 y", &Graphics::Activity::Desc::Y);
				VActivityDesc.SetProperty<Graphics::Activity::Desc>("uint32 inactive_sleep_ms", &Graphics::Activity::Desc::InactiveSleepMs);
				VActivityDesc.SetProperty<Graphics::Activity::Desc>("bool fullscreen", &Graphics::Activity::Desc::Fullscreen);
				VActivityDesc.SetProperty<Graphics::Activity::Desc>("bool hidden", &Graphics::Activity::Desc::Hidden);
				VActivityDesc.SetProperty<Graphics::Activity::Desc>("bool borderless", &Graphics::Activity::Desc::Borderless);
				VActivityDesc.SetProperty<Graphics::Activity::Desc>("bool resizable", &Graphics::Activity::Desc::Resizable);
				VActivityDesc.SetProperty<Graphics::Activity::Desc>("bool minimized", &Graphics::Activity::Desc::Minimized);
				VActivityDesc.SetProperty<Graphics::Activity::Desc>("bool maximized", &Graphics::Activity::Desc::Maximized);
				VActivityDesc.SetProperty<Graphics::Activity::Desc>("bool centered", &Graphics::Activity::Desc::Centered);
				VActivityDesc.SetProperty<Graphics::Activity::Desc>("bool free_position", &Graphics::Activity::Desc::FreePosition);
				VActivityDesc.SetProperty<Graphics::Activity::Desc>("bool focused", &Graphics::Activity::Desc::Focused);
				VActivityDesc.SetProperty<Graphics::Activity::Desc>("bool render_even_if_inactive", &Graphics::Activity::Desc::RenderEvenIfInactive);
				VActivityDesc.SetProperty<Graphics::Activity::Desc>("bool gpu_as_renderer", &Graphics::Activity::Desc::GPUAsRenderer);
				VActivityDesc.SetProperty<Graphics::Activity::Desc>("bool high_dpi", &Graphics::Activity::Desc::HighDPI);
				VActivityDesc.SetConstructor<Graphics::Activity::Desc>("void f()");

				VActivity.SetProperty<Graphics::Activity>("activity_alert message", &Graphics::Activity::Message);
				VActivity.SetConstructor<Graphics::Activity, const Graphics::Activity::Desc&>("activity@ f(const activity_desc &in)");
				VActivity.SetFunctionDef("void app_state_change_event(app_state)");
				VActivity.SetFunctionDef("void window_state_change_event(window_state, int, int)");
				VActivity.SetFunctionDef("void key_state_event(key_code, key_mod, int, int, bool)");
				VActivity.SetFunctionDef("void input_edit_event(const string &in, int, int)");
				VActivity.SetFunctionDef("void input_event(const string &in, int)");
				VActivity.SetFunctionDef("void cursor_move_event(int, int, int, int)");
				VActivity.SetFunctionDef("void cursor_wheel_state_event(int, int, bool)");
				VActivity.SetFunctionDef("void joy_stick_axis_move_event(int, int, int)");
				VActivity.SetFunctionDef("void joy_stick_ball_move_event(int, int, int, int)");
				VActivity.SetFunctionDef("void joy_stick_hat_move_event(joy_stick_hat, int, int)");
				VActivity.SetFunctionDef("void joy_stick_key_state_event(int, int, bool)");
				VActivity.SetFunctionDef("void joy_stick_state_event(int, bool)");
				VActivity.SetFunctionDef("void controller_axis_move_event(int, int, int)");
				VActivity.SetFunctionDef("void controller_key_state_event(int, int, bool)");
				VActivity.SetFunctionDef("void controller_state_event(int, int)");
				VActivity.SetFunctionDef("void touch_move_event(int, int, float, float, float, float, float)");
				VActivity.SetFunctionDef("void touch_state_event(int, int, float, float, float, float, float, bool)");
				VActivity.SetFunctionDef("void gesture_state_event(int, int, int, float, float, float, bool)");
				VActivity.SetFunctionDef("void multi_gesture_state_event(int, int, float, float, float, float)");
				VActivity.SetFunctionDef("void drop_file_event(const string &in)");
				VActivity.SetFunctionDef("void drop_text_event(const string &in)");
				VActivity.SetMethodEx("void set_app_state_change(app_state_change_event@+)", &ActivitySetAppStateChange);
				VActivity.SetMethodEx("void set_window_state_change(window_state_change_event@+)", &ActivitySetWindowStateChange);
				VActivity.SetMethodEx("void set_key_state(key_state_event@+)", &ActivitySetKeyState);
				VActivity.SetMethodEx("void set_input_edit(input_edit_event@+)", &ActivitySetInputEdit);
				VActivity.SetMethodEx("void set_input(input_event@+)", &ActivitySetInput);
				VActivity.SetMethodEx("void set_cursor_move(cursor_move_event@+)", &ActivitySetCursorMove);
				VActivity.SetMethodEx("void set_cursor_wheel_state(cursor_wheel_state_event@+)", &ActivitySetCursorWheelState);
				VActivity.SetMethodEx("void set_joy_stick_axis_move(joy_stick_axis_move_event@+)", &ActivitySetJoyStickAxisMove);
				VActivity.SetMethodEx("void set_joy_stick_ball_move(joy_stick_ball_move_event@+)", &ActivitySetJoyStickBallMove);
				VActivity.SetMethodEx("void set_joy_stick_hat_move(joy_stick_hat_move_event@+)", &ActivitySetJoyStickHatMove);
				VActivity.SetMethodEx("void set_joy_stickKeyState(joy_stick_key_state_event@+)", &ActivitySetJoyStickKeyState);
				VActivity.SetMethodEx("void set_joy_stickState(joy_stick_state_event@+)", &ActivitySetJoyStickState);
				VActivity.SetMethodEx("void set_controller_axis_move(controller_axis_move_event@+)", &ActivitySetControllerAxisMove);
				VActivity.SetMethodEx("void set_controller_key_state(controller_key_state_event@+)", &ActivitySetControllerKeyState);
				VActivity.SetMethodEx("void set_controller_state(controller_state_event@+)", &ActivitySetControllerState);
				VActivity.SetMethodEx("void set_touch_move(touch_move_event@+)", &ActivitySetTouchMove);
				VActivity.SetMethodEx("void set_touch_state(touch_state_event@+)", &ActivitySetTouchState);
				VActivity.SetMethodEx("void set_gesture_state(gesture_state_event@+)", &ActivitySetGestureState);
				VActivity.SetMethodEx("void set_multi_gesture_state(multi_gesture_state_event@+)", &ActivitySetMultiGestureState);
				VActivity.SetMethodEx("void set_drop_file(drop_file_event@+)", &ActivitySetDropFile);
				VActivity.SetMethodEx("void set_drop_text(drop_text_event@+)", &ActivitySetDropText);
				VActivity.SetMethod("void set_clipboard_text(const string &in)", &Graphics::Activity::SetClipboardText);
				VActivity.SetMethod<Graphics::Activity, void, const Compute::Vector2&>("void set_cursor_position(const vector2 &in)", &Graphics::Activity::SetCursorPosition);
				VActivity.SetMethod<Graphics::Activity, void, float, float>("void set_cursor_position(float, float)", &Graphics::Activity::SetCursorPosition);
				VActivity.SetMethod<Graphics::Activity, void, const Compute::Vector2&>("void set_global_cursor_position(const vector2 &in)", &Graphics::Activity::SetGlobalCursorPosition);
				VActivity.SetMethod<Graphics::Activity, void, float, float>("void set_global_cursor_position(float, float)", &Graphics::Activity::SetGlobalCursorPosition);
				VActivity.SetMethod("void set_key(key_code, bool)", &Graphics::Activity::SetKey);
				VActivity.SetMethod("void set_cursor(display_cursor)", &Graphics::Activity::SetCursor);
				VActivity.SetMethod("void set_cursor_visibility(bool)", &Graphics::Activity::SetCursorVisibility);
				VActivity.SetMethod("void set_grabbing(bool)", &Graphics::Activity::SetGrabbing);
				VActivity.SetMethod("void set_fullscreen(bool)", &Graphics::Activity::SetFullscreen);
				VActivity.SetMethod("void set_borderless(bool)", &Graphics::Activity::SetBorderless);
				VActivity.SetMethod("void set_icon(activity_surface@+)", &Graphics::Activity::SetIcon);
				VActivity.SetMethodEx("void set_title(const string &in)", &ActivitySetTitle);
				VActivity.SetMethod("void set_screen_keyboard(bool)", &Graphics::Activity::SetScreenKeyboard);
				VActivity.SetMethod("void build_layer(render_backend)", &Graphics::Activity::BuildLayer);
				VActivity.SetMethod("void hide()", &Graphics::Activity::Hide);
				VActivity.SetMethod("void show()", &Graphics::Activity::Show);
				VActivity.SetMethod("void maximize()", &Graphics::Activity::Maximize);
				VActivity.SetMethod("void minimize()", &Graphics::Activity::Minimize);
				VActivity.SetMethod("void focus()", &Graphics::Activity::Focus);
				VActivity.SetMethod("void move(int, int)", &Graphics::Activity::Move);
				VActivity.SetMethod("void resize(int, int)", &Graphics::Activity::Resize);
				VActivity.SetMethod("bool capture_key_map(key_map &out)", &Graphics::Activity::CaptureKeyMap);
				VActivity.SetMethod("bool dispatch()", &Graphics::Activity::Dispatch);
				VActivity.SetMethod("bool is_fullscreen() const", &Graphics::Activity::IsFullscreen);
				VActivity.SetMethod("bool is_any_key_down() const", &Graphics::Activity::IsAnyKeyDown);
				VActivity.SetMethod("bool is_key_down(const key_map &in) const", &Graphics::Activity::IsKeyDown);
				VActivity.SetMethod("bool is_key_up(const key_map &in) const", &Graphics::Activity::IsKeyUp);
				VActivity.SetMethod("bool is_key_down_hit(const key_map &in) const", &Graphics::Activity::IsKeyDownHit);
				VActivity.SetMethod("bool is_key_up_hit(const key_map &in) const", &Graphics::Activity::IsKeyUpHit);
				VActivity.SetMethod("bool is_screen_keyboard_enabled() const", &Graphics::Activity::IsScreenKeyboardEnabled);
				VActivity.SetMethod("uint32 get_x() const", &Graphics::Activity::GetX);
				VActivity.SetMethod("uint32 get_y() const", &Graphics::Activity::GetY);
				VActivity.SetMethod("uint32 get_width() const", &Graphics::Activity::GetWidth);
				VActivity.SetMethod("uint32 get_height() const", &Graphics::Activity::GetHeight);
				VActivity.SetMethod("float get_aspect_ratio() const", &Graphics::Activity::GetAspectRatio);
				VActivity.SetMethod("key_mod get_key_mod_state() const", &Graphics::Activity::GetKeyModState);
				VActivity.SetMethod("viewport get_viewport() const", &Graphics::Activity::GetViewport);
				VActivity.SetMethod("vector2 get_offset() const", &Graphics::Activity::GetOffset);
				VActivity.SetMethod("vector2 get_size() const", &Graphics::Activity::GetSize);
				VActivity.SetMethod("vector2 get_client_size() const", &Graphics::Activity::GetClientSize);
				VActivity.SetMethod("vector2 get_global_cursor_position() const", &Graphics::Activity::GetGlobalCursorPosition);
				VActivity.SetMethod<Graphics::Activity, Compute::Vector2>("vector2 get_cursor_position() const", &Graphics::Activity::GetCursorPosition);
				VActivity.SetMethod<Graphics::Activity, Compute::Vector2, float, float>("vector2 get_cursor_position(float, float) const", &Graphics::Activity::GetCursorPosition);
				VActivity.SetMethod<Graphics::Activity, Compute::Vector2, const Compute::Vector2&>("vector2 get_cursor_position(const vector2 &in) const", &Graphics::Activity::GetCursorPosition);
				VActivity.SetMethod("string get_clipboard_text() const", &Graphics::Activity::GetClipboardText);
				VActivity.SetMethod("string get_error() const", &Graphics::Activity::GetError);
				VActivity.SetMethod("activity_desc& get_options()", &Graphics::Activity::GetOptions);

				return true;
#else
				VI_ASSERT(false, false, "<activity> is not loaded");
				return false;
#endif
			}
			bool Registry::LoadGraphics(VirtualMachine* Engine)
			{
#ifdef VI_HAS_BINDINGS
				VI_ASSERT(Engine != nullptr, false, "manager should be set");
				Enumeration VVSync = Engine->SetEnum("vsync");
				VVSync.SetValue("off", (int)Graphics::VSync::Off);
				VVSync.SetValue("frequency_x1", (int)Graphics::VSync::Frequency_X1);
				VVSync.SetValue("frequency_x2", (int)Graphics::VSync::Frequency_X2);
				VVSync.SetValue("frequency_x3", (int)Graphics::VSync::Frequency_X3);
				VVSync.SetValue("frequency_x4", (int)Graphics::VSync::Frequency_X4);
				VVSync.SetValue("on", (int)Graphics::VSync::On);

				Enumeration VSurfaceTarget = Engine->SetEnum("surface_target");
				VSurfaceTarget.SetValue("t0", (int)Graphics::SurfaceTarget::T0);
				VSurfaceTarget.SetValue("t1", (int)Graphics::SurfaceTarget::T1);
				VSurfaceTarget.SetValue("t2", (int)Graphics::SurfaceTarget::T2);
				VSurfaceTarget.SetValue("t3", (int)Graphics::SurfaceTarget::T3);
				VSurfaceTarget.SetValue("t4", (int)Graphics::SurfaceTarget::T4);
				VSurfaceTarget.SetValue("t5", (int)Graphics::SurfaceTarget::T5);
				VSurfaceTarget.SetValue("t6", (int)Graphics::SurfaceTarget::T6);
				VSurfaceTarget.SetValue("t7", (int)Graphics::SurfaceTarget::T7);

				Enumeration VPrimitiveTopology = Engine->SetEnum("primitive_topology");
				VPrimitiveTopology.SetValue("invalid", (int)Graphics::PrimitiveTopology::Invalid);
				VPrimitiveTopology.SetValue("point_list", (int)Graphics::PrimitiveTopology::Point_List);
				VPrimitiveTopology.SetValue("line_list", (int)Graphics::PrimitiveTopology::Line_List);
				VPrimitiveTopology.SetValue("line_strip", (int)Graphics::PrimitiveTopology::Line_Strip);
				VPrimitiveTopology.SetValue("triangle_list", (int)Graphics::PrimitiveTopology::Triangle_List);
				VPrimitiveTopology.SetValue("triangle_strip", (int)Graphics::PrimitiveTopology::Triangle_Strip);
				VPrimitiveTopology.SetValue("line_list_adj", (int)Graphics::PrimitiveTopology::Line_List_Adj);
				VPrimitiveTopology.SetValue("line_strip_adj", (int)Graphics::PrimitiveTopology::Line_Strip_Adj);
				VPrimitiveTopology.SetValue("triangle_list_adj", (int)Graphics::PrimitiveTopology::Triangle_List_Adj);
				VPrimitiveTopology.SetValue("triangle_strip_adj", (int)Graphics::PrimitiveTopology::Triangle_Strip_Adj);

				Enumeration VFormat = Engine->SetEnum("surface_format");
				VFormat.SetValue("unknown", (int)Graphics::Format::Unknown);
				VFormat.SetValue("A8_unorm", (int)Graphics::Format::A8_Unorm);
				VFormat.SetValue("D16_unorm", (int)Graphics::Format::D16_Unorm);
				VFormat.SetValue("D24_unorm_S8_uint", (int)Graphics::Format::D24_Unorm_S8_Uint);
				VFormat.SetValue("D32_float", (int)Graphics::Format::D32_Float);
				VFormat.SetValue("R10G10B10A2_uint", (int)Graphics::Format::R10G10B10A2_Uint);
				VFormat.SetValue("R10G10B10A2_unorm", (int)Graphics::Format::R10G10B10A2_Unorm);
				VFormat.SetValue("R11G11B10_float", (int)Graphics::Format::R11G11B10_Float);
				VFormat.SetValue("R16G16B16A16_float", (int)Graphics::Format::R16G16B16A16_Float);
				VFormat.SetValue("R16G16B16A16_sint", (int)Graphics::Format::R16G16B16A16_Sint);
				VFormat.SetValue("R16G16B16A16_snorm", (int)Graphics::Format::R16G16B16A16_Snorm);
				VFormat.SetValue("R16G16B16A16_uint", (int)Graphics::Format::R16G16B16A16_Uint);
				VFormat.SetValue("R16G16B16A16_unorm", (int)Graphics::Format::R16G16B16A16_Unorm);
				VFormat.SetValue("R16G16_float", (int)Graphics::Format::R16G16_Float);
				VFormat.SetValue("R16G16_sint", (int)Graphics::Format::R16G16_Sint);
				VFormat.SetValue("R16G16_snorm", (int)Graphics::Format::R16G16_Snorm);
				VFormat.SetValue("R16G16_uint", (int)Graphics::Format::R16G16_Uint);
				VFormat.SetValue("R16G16_unorm", (int)Graphics::Format::R16G16_Unorm);
				VFormat.SetValue("R16_float", (int)Graphics::Format::R16_Float);
				VFormat.SetValue("R16_sint", (int)Graphics::Format::R16_Sint);
				VFormat.SetValue("R16_snorm", (int)Graphics::Format::R16_Snorm);
				VFormat.SetValue("R16_uint", (int)Graphics::Format::R16_Uint);
				VFormat.SetValue("R16_unorm", (int)Graphics::Format::R16_Unorm);
				VFormat.SetValue("R1_unorm", (int)Graphics::Format::R1_Unorm);
				VFormat.SetValue("R32G32B32A32_float", (int)Graphics::Format::R32G32B32A32_Float);
				VFormat.SetValue("R32G32B32A32_sint", (int)Graphics::Format::R32G32B32A32_Sint);
				VFormat.SetValue("R32G32B32A32_uint", (int)Graphics::Format::R32G32B32A32_Uint);
				VFormat.SetValue("R32G32B32_float", (int)Graphics::Format::R32G32B32_Float);
				VFormat.SetValue("R32G32B32_sint", (int)Graphics::Format::R32G32B32_Sint);
				VFormat.SetValue("R32G32B32_uint", (int)Graphics::Format::R32G32B32_Uint);
				VFormat.SetValue("R32G32_float", (int)Graphics::Format::R32G32_Float);
				VFormat.SetValue("R32G32_sint", (int)Graphics::Format::R32G32_Sint);
				VFormat.SetValue("R32G32_uint", (int)Graphics::Format::R32G32_Uint);
				VFormat.SetValue("R32_float", (int)Graphics::Format::R32_Float);
				VFormat.SetValue("R32_sint", (int)Graphics::Format::R32_Sint);
				VFormat.SetValue("R32_uint", (int)Graphics::Format::R32_Uint);
				VFormat.SetValue("R8G8B8A8_sint", (int)Graphics::Format::R8G8B8A8_Sint);
				VFormat.SetValue("R8G8B8A8_snorm", (int)Graphics::Format::R8G8B8A8_Snorm);
				VFormat.SetValue("R8G8B8A8_uint", (int)Graphics::Format::R8G8B8A8_Uint);
				VFormat.SetValue("R8G8B8A8_unorm", (int)Graphics::Format::R8G8B8A8_Unorm);
				VFormat.SetValue("R8G8B8A8_unorm_SRGB", (int)Graphics::Format::R8G8B8A8_Unorm_SRGB);
				VFormat.SetValue("R8G8_B8G8_unorm", (int)Graphics::Format::R8G8_B8G8_Unorm);
				VFormat.SetValue("R8G8_sint", (int)Graphics::Format::R8G8_Sint);
				VFormat.SetValue("R8G8_snorm", (int)Graphics::Format::R8G8_Snorm);
				VFormat.SetValue("R8G8_uint", (int)Graphics::Format::R8G8_Uint);
				VFormat.SetValue("R8G8_unorm", (int)Graphics::Format::R8G8_Unorm);
				VFormat.SetValue("R8_sint", (int)Graphics::Format::R8_Sint);
				VFormat.SetValue("R8_snorm", (int)Graphics::Format::R8_Snorm);
				VFormat.SetValue("R8_uint", (int)Graphics::Format::R8_Uint);
				VFormat.SetValue("R8_unorm", (int)Graphics::Format::R8_Unorm);
				VFormat.SetValue("R9G9B9E5_share_dexp", (int)Graphics::Format::R9G9B9E5_Share_Dexp);

				Enumeration VResourceMap = Engine->SetEnum("resource_map");
				VResourceMap.SetValue("read", (int)Graphics::ResourceMap::Read);
				VResourceMap.SetValue("write", (int)Graphics::ResourceMap::Write);
				VResourceMap.SetValue("read_write", (int)Graphics::ResourceMap::Read_Write);
				VResourceMap.SetValue("write_discard", (int)Graphics::ResourceMap::Write_Discard);
				VResourceMap.SetValue("write_no_overwrite", (int)Graphics::ResourceMap::Write_No_Overwrite);

				Enumeration VResourceUsage = Engine->SetEnum("resource_usage");
				VResourceUsage.SetValue("default_t", (int)Graphics::ResourceUsage::Default);
				VResourceUsage.SetValue("immutable", (int)Graphics::ResourceUsage::Immutable);
				VResourceUsage.SetValue("dynamic", (int)Graphics::ResourceUsage::Dynamic);
				VResourceUsage.SetValue("staging", (int)Graphics::ResourceUsage::Staging);

				Enumeration VShaderModel = Engine->SetEnum("shader_model");
				VShaderModel.SetValue("invalid", (int)Graphics::ShaderModel::Invalid);
				VShaderModel.SetValue("auto_t", (int)Graphics::ShaderModel::Auto);
				VShaderModel.SetValue("HLSL_1_0", (int)Graphics::ShaderModel::HLSL_1_0);
				VShaderModel.SetValue("HLSL_2_0", (int)Graphics::ShaderModel::HLSL_2_0);
				VShaderModel.SetValue("HLSL_3_0", (int)Graphics::ShaderModel::HLSL_3_0);
				VShaderModel.SetValue("HLSL_4_0", (int)Graphics::ShaderModel::HLSL_4_0);
				VShaderModel.SetValue("HLSL_4_1", (int)Graphics::ShaderModel::HLSL_4_1);
				VShaderModel.SetValue("HLSL_5_0", (int)Graphics::ShaderModel::HLSL_5_0);
				VShaderModel.SetValue("GLSL_1_1_0", (int)Graphics::ShaderModel::GLSL_1_1_0);
				VShaderModel.SetValue("GLSL_1_2_0", (int)Graphics::ShaderModel::GLSL_1_2_0);
				VShaderModel.SetValue("GLSL_1_3_0", (int)Graphics::ShaderModel::GLSL_1_3_0);
				VShaderModel.SetValue("GLSL_1_4_0", (int)Graphics::ShaderModel::GLSL_1_4_0);
				VShaderModel.SetValue("GLSL_1_5_0", (int)Graphics::ShaderModel::GLSL_1_5_0);
				VShaderModel.SetValue("GLSL_3_3_0", (int)Graphics::ShaderModel::GLSL_3_3_0);
				VShaderModel.SetValue("GLSL_4_0_0", (int)Graphics::ShaderModel::GLSL_4_0_0);
				VShaderModel.SetValue("GLSL_4_1_0", (int)Graphics::ShaderModel::GLSL_4_1_0);
				VShaderModel.SetValue("GLSL_4_2_0", (int)Graphics::ShaderModel::GLSL_4_2_0);
				VShaderModel.SetValue("GLSL_4_3_0", (int)Graphics::ShaderModel::GLSL_4_3_0);
				VShaderModel.SetValue("GLSL_4_4_0", (int)Graphics::ShaderModel::GLSL_4_4_0);
				VShaderModel.SetValue("GLSL_4_5_0", (int)Graphics::ShaderModel::GLSL_4_5_0);
				VShaderModel.SetValue("GLSL_4_6_0", (int)Graphics::ShaderModel::GLSL_4_6_0);

				Enumeration VResourceBind = Engine->SetEnum("resource_bind");
				VResourceBind.SetValue("vertex_buffer", (int)Graphics::ResourceBind::Vertex_Buffer);
				VResourceBind.SetValue("index_buffer", (int)Graphics::ResourceBind::Index_Buffer);
				VResourceBind.SetValue("constant_buffer", (int)Graphics::ResourceBind::Constant_Buffer);
				VResourceBind.SetValue("shader_input", (int)Graphics::ResourceBind::Shader_Input);
				VResourceBind.SetValue("stream_output", (int)Graphics::ResourceBind::Stream_Output);
				VResourceBind.SetValue("render_target", (int)Graphics::ResourceBind::Render_Target);
				VResourceBind.SetValue("depth_stencil", (int)Graphics::ResourceBind::Depth_Stencil);
				VResourceBind.SetValue("unordered_access", (int)Graphics::ResourceBind::Unordered_Access);

				Enumeration VCPUAccess = Engine->SetEnum("cpu_access");
				VCPUAccess.SetValue("none", (int)Graphics::CPUAccess::None);
				VCPUAccess.SetValue("write", (int)Graphics::CPUAccess::Write);
				VCPUAccess.SetValue("read", (int)Graphics::CPUAccess::Read);

				Enumeration VDepthWrite = Engine->SetEnum("depth_write");
				VDepthWrite.SetValue("zero", (int)Graphics::DepthWrite::Zero);
				VDepthWrite.SetValue("all", (int)Graphics::DepthWrite::All);

				Enumeration VComparison = Engine->SetEnum("comparison");
				VComparison.SetValue("never", (int)Graphics::Comparison::Never);
				VComparison.SetValue("less", (int)Graphics::Comparison::Less);
				VComparison.SetValue("equal", (int)Graphics::Comparison::Equal);
				VComparison.SetValue("less_equal", (int)Graphics::Comparison::Less_Equal);
				VComparison.SetValue("greater", (int)Graphics::Comparison::Greater);
				VComparison.SetValue("not_equal", (int)Graphics::Comparison::Not_Equal);
				VComparison.SetValue("greater_equal", (int)Graphics::Comparison::Greater_Equal);
				VComparison.SetValue("always", (int)Graphics::Comparison::Always);

				Enumeration VStencilOperation = Engine->SetEnum("stencil_operation");
				VStencilOperation.SetValue("keep", (int)Graphics::StencilOperation::Keep);
				VStencilOperation.SetValue("zero", (int)Graphics::StencilOperation::Zero);
				VStencilOperation.SetValue("replace", (int)Graphics::StencilOperation::Replace);
				VStencilOperation.SetValue("sat_add", (int)Graphics::StencilOperation::SAT_Add);
				VStencilOperation.SetValue("sat_subtract", (int)Graphics::StencilOperation::SAT_Subtract);
				VStencilOperation.SetValue("invert", (int)Graphics::StencilOperation::Invert);
				VStencilOperation.SetValue("add", (int)Graphics::StencilOperation::Add);
				VStencilOperation.SetValue("subtract", (int)Graphics::StencilOperation::Subtract);

				Enumeration VBlend = Engine->SetEnum("blend_t");
				VBlend.SetValue("zero", (int)Graphics::Blend::Zero);
				VBlend.SetValue("one", (int)Graphics::Blend::One);
				VBlend.SetValue("source_color", (int)Graphics::Blend::Source_Color);
				VBlend.SetValue("source_color_invert", (int)Graphics::Blend::Source_Color_Invert);
				VBlend.SetValue("source_alpha", (int)Graphics::Blend::Source_Alpha);
				VBlend.SetValue("source_alpha_invert", (int)Graphics::Blend::Source_Alpha_Invert);
				VBlend.SetValue("destination_alpha", (int)Graphics::Blend::Destination_Alpha);
				VBlend.SetValue("destination_alpha_invert", (int)Graphics::Blend::Destination_Alpha_Invert);
				VBlend.SetValue("destination_color", (int)Graphics::Blend::Destination_Color);
				VBlend.SetValue("destination_color_invert", (int)Graphics::Blend::Destination_Color_Invert);
				VBlend.SetValue("source_alpha_sat", (int)Graphics::Blend::Source_Alpha_SAT);
				VBlend.SetValue("blend_factor", (int)Graphics::Blend::Blend_Factor);
				VBlend.SetValue("blend_factor_invert", (int)Graphics::Blend::Blend_Factor_Invert);
				VBlend.SetValue("source1_color", (int)Graphics::Blend::Source1_Color);
				VBlend.SetValue("source1_color_invert", (int)Graphics::Blend::Source1_Color_Invert);
				VBlend.SetValue("source1_alpha", (int)Graphics::Blend::Source1_Alpha);
				VBlend.SetValue("source1_alpha_invert", (int)Graphics::Blend::Source1_Alpha_Invert);

				Enumeration VSurfaceFill = Engine->SetEnum("surface_fill");
				VSurfaceFill.SetValue("wireframe", (int)Graphics::SurfaceFill::Wireframe);
				VSurfaceFill.SetValue("solid", (int)Graphics::SurfaceFill::Solid);

				Enumeration VPixelFilter = Engine->SetEnum("pixel_filter");
				VPixelFilter.SetValue("min_mag_mip_point", (int)Graphics::PixelFilter::Min_Mag_Mip_Point);
				VPixelFilter.SetValue("min_mag_point_mip_linear", (int)Graphics::PixelFilter::Min_Mag_Point_Mip_Linear);
				VPixelFilter.SetValue("min_point_mag_linear_mip_point", (int)Graphics::PixelFilter::Min_Point_Mag_Linear_Mip_Point);
				VPixelFilter.SetValue("min_point_mag_mip_linear", (int)Graphics::PixelFilter::Min_Point_Mag_Mip_Linear);
				VPixelFilter.SetValue("min_linear_mag_mip_point", (int)Graphics::PixelFilter::Min_Linear_Mag_Mip_Point);
				VPixelFilter.SetValue("min_linear_mag_point_mip_linear", (int)Graphics::PixelFilter::Min_Linear_Mag_Point_Mip_Linear);
				VPixelFilter.SetValue("min_mag_linear_mip_point", (int)Graphics::PixelFilter::Min_Mag_Linear_Mip_Point);
				VPixelFilter.SetValue("min_mag_mip_minear", (int)Graphics::PixelFilter::Min_Mag_Mip_Linear);
				VPixelFilter.SetValue("anistropic", (int)Graphics::PixelFilter::Anistropic);
				VPixelFilter.SetValue("compare_min_mag_mip_point", (int)Graphics::PixelFilter::Compare_Min_Mag_Mip_Point);
				VPixelFilter.SetValue("compare_min_mag_point_mip_linear", (int)Graphics::PixelFilter::Compare_Min_Mag_Point_Mip_Linear);
				VPixelFilter.SetValue("compare_min_point_mag_linear_mip_point", (int)Graphics::PixelFilter::Compare_Min_Point_Mag_Linear_Mip_Point);
				VPixelFilter.SetValue("compare_min_point_mag_mip_linear", (int)Graphics::PixelFilter::Compare_Min_Point_Mag_Mip_Linear);
				VPixelFilter.SetValue("compare_min_linear_mag_mip_point", (int)Graphics::PixelFilter::Compare_Min_Linear_Mag_Mip_Point);
				VPixelFilter.SetValue("compare_min_linear_mag_point_mip_linear", (int)Graphics::PixelFilter::Compare_Min_Linear_Mag_Point_Mip_Linear);
				VPixelFilter.SetValue("compare_min_mag_linear_mip_point", (int)Graphics::PixelFilter::Compare_Min_Mag_Linear_Mip_Point);
				VPixelFilter.SetValue("compare_min_mag_mip_linear", (int)Graphics::PixelFilter::Compare_Min_Mag_Mip_Linear);
				VPixelFilter.SetValue("compare_anistropic", (int)Graphics::PixelFilter::Compare_Anistropic);

				Enumeration VTextureAddress = Engine->SetEnum("texture_address");
				VTextureAddress.SetValue("wrap", (int)Graphics::TextureAddress::Wrap);
				VTextureAddress.SetValue("mirror", (int)Graphics::TextureAddress::Mirror);
				VTextureAddress.SetValue("clamp", (int)Graphics::TextureAddress::Clamp);
				VTextureAddress.SetValue("border", (int)Graphics::TextureAddress::Border);
				VTextureAddress.SetValue("mirror_once", (int)Graphics::TextureAddress::Mirror_Once);

				Enumeration VColorWriteEnable = Engine->SetEnum("color_write_enable");
				VColorWriteEnable.SetValue("red", (int)Graphics::ColorWriteEnable::Red);
				VColorWriteEnable.SetValue("green", (int)Graphics::ColorWriteEnable::Green);
				VColorWriteEnable.SetValue("blue", (int)Graphics::ColorWriteEnable::Blue);
				VColorWriteEnable.SetValue("alpha", (int)Graphics::ColorWriteEnable::Alpha);
				VColorWriteEnable.SetValue("all", (int)Graphics::ColorWriteEnable::All);

				Enumeration VBlendOperation = Engine->SetEnum("blend_operation");
				VBlendOperation.SetValue("add", (int)Graphics::BlendOperation::Add);
				VBlendOperation.SetValue("subtract", (int)Graphics::BlendOperation::Subtract);
				VBlendOperation.SetValue("subtract_reverse", (int)Graphics::BlendOperation::Subtract_Reverse);
				VBlendOperation.SetValue("min", (int)Graphics::BlendOperation::Min);
				VBlendOperation.SetValue("max", (int)Graphics::BlendOperation::Max);

				Enumeration VVertexCull = Engine->SetEnum("vertex_cull");
				VVertexCull.SetValue("none", (int)Graphics::VertexCull::None);
				VVertexCull.SetValue("front", (int)Graphics::VertexCull::Front);
				VVertexCull.SetValue("back", (int)Graphics::VertexCull::Back);

				Enumeration VShaderCompile = Engine->SetEnum("shader_compile");
				VShaderCompile.SetValue("debug", (int)Graphics::ShaderCompile::Debug);
				VShaderCompile.SetValue("skip_validation", (int)Graphics::ShaderCompile::Skip_Validation);
				VShaderCompile.SetValue("skip_optimization", (int)Graphics::ShaderCompile::Skip_Optimization);
				VShaderCompile.SetValue("matrix_row_major", (int)Graphics::ShaderCompile::Matrix_Row_Major);
				VShaderCompile.SetValue("matrix_column_major", (int)Graphics::ShaderCompile::Matrix_Column_Major);
				VShaderCompile.SetValue("partial_precision", (int)Graphics::ShaderCompile::Partial_Precision);
				VShaderCompile.SetValue("foe_vs_no_opt", (int)Graphics::ShaderCompile::FOE_VS_No_OPT);
				VShaderCompile.SetValue("foe_ps_no_opt", (int)Graphics::ShaderCompile::FOE_PS_No_OPT);
				VShaderCompile.SetValue("no_preshader", (int)Graphics::ShaderCompile::No_Preshader);
				VShaderCompile.SetValue("avoid_flow_control", (int)Graphics::ShaderCompile::Avoid_Flow_Control);
				VShaderCompile.SetValue("prefer_flow_control", (int)Graphics::ShaderCompile::Prefer_Flow_Control);
				VShaderCompile.SetValue("enable_strictness", (int)Graphics::ShaderCompile::Enable_Strictness);
				VShaderCompile.SetValue("enable_backwards_compatibility", (int)Graphics::ShaderCompile::Enable_Backwards_Compatibility);
				VShaderCompile.SetValue("ieee_strictness", (int)Graphics::ShaderCompile::IEEE_Strictness);
				VShaderCompile.SetValue("optimization_level0", (int)Graphics::ShaderCompile::Optimization_Level0);
				VShaderCompile.SetValue("optimization_level1", (int)Graphics::ShaderCompile::Optimization_Level1);
				VShaderCompile.SetValue("optimization_level2", (int)Graphics::ShaderCompile::Optimization_Level2);
				VShaderCompile.SetValue("optimization_level3", (int)Graphics::ShaderCompile::Optimization_Level3);
				VShaderCompile.SetValue("reseed_x16", (int)Graphics::ShaderCompile::Reseed_X16);
				VShaderCompile.SetValue("reseed_x17", (int)Graphics::ShaderCompile::Reseed_X17);
				VShaderCompile.SetValue("picky", (int)Graphics::ShaderCompile::Picky);

				Enumeration VResourceMisc = Engine->SetEnum("resource_misc");
				VResourceMisc.SetValue("none", (int)Graphics::ResourceMisc::None);
				VResourceMisc.SetValue("generate_mips", (int)Graphics::ResourceMisc::Generate_Mips);
				VResourceMisc.SetValue("shared", (int)Graphics::ResourceMisc::Shared);
				VResourceMisc.SetValue("texture_cube", (int)Graphics::ResourceMisc::Texture_Cube);
				VResourceMisc.SetValue("draw_indirect_args", (int)Graphics::ResourceMisc::Draw_Indirect_Args);
				VResourceMisc.SetValue("buffer_allow_raw_views", (int)Graphics::ResourceMisc::Buffer_Allow_Raw_Views);
				VResourceMisc.SetValue("buffer_structured", (int)Graphics::ResourceMisc::Buffer_Structured);
				VResourceMisc.SetValue("clamp", (int)Graphics::ResourceMisc::Clamp);
				VResourceMisc.SetValue("shared_keyed_mutex", (int)Graphics::ResourceMisc::Shared_Keyed_Mutex);
				VResourceMisc.SetValue("gdi_compatible", (int)Graphics::ResourceMisc::GDI_Compatible);
				VResourceMisc.SetValue("shared_nt_handle", (int)Graphics::ResourceMisc::Shared_NT_Handle);
				VResourceMisc.SetValue("restricted_content", (int)Graphics::ResourceMisc::Restricted_Content);
				VResourceMisc.SetValue("restrict_shared", (int)Graphics::ResourceMisc::Restrict_Shared);
				VResourceMisc.SetValue("restrict_shared_driver", (int)Graphics::ResourceMisc::Restrict_Shared_Driver);
				VResourceMisc.SetValue("guarded", (int)Graphics::ResourceMisc::Guarded);
				VResourceMisc.SetValue("tile_pool", (int)Graphics::ResourceMisc::Tile_Pool);
				VResourceMisc.SetValue("tiled", (int)Graphics::ResourceMisc::Tiled);

				Enumeration VShaderType = Engine->SetEnum("shader_type");
				VShaderType.SetValue("vertex", (int)Graphics::ShaderType::Vertex);
				VShaderType.SetValue("pixel", (int)Graphics::ShaderType::Pixel);
				VShaderType.SetValue("geometry", (int)Graphics::ShaderType::Geometry);
				VShaderType.SetValue("hull", (int)Graphics::ShaderType::Hull);
				VShaderType.SetValue("domain", (int)Graphics::ShaderType::Domain);
				VShaderType.SetValue("compute", (int)Graphics::ShaderType::Compute);
				VShaderType.SetValue("all", (int)Graphics::ShaderType::All);

				Enumeration VShaderLang = Engine->SetEnum("shader_lang");
				VShaderLang.SetValue("none", (int)Graphics::ShaderLang::None);
				VShaderLang.SetValue("spv", (int)Graphics::ShaderLang::SPV);
				VShaderLang.SetValue("msl", (int)Graphics::ShaderLang::MSL);
				VShaderLang.SetValue("hlsl", (int)Graphics::ShaderLang::HLSL);
				VShaderLang.SetValue("glsl", (int)Graphics::ShaderLang::GLSL);
				VShaderLang.SetValue("glsl_es", (int)Graphics::ShaderLang::GLSL_ES);

				Enumeration VAttributeType = Engine->SetEnum("attribute_type");
				VAttributeType.SetValue("byte_t", (int)Graphics::AttributeType::Byte);
				VAttributeType.SetValue("ubyte_t", (int)Graphics::AttributeType::Ubyte);
				VAttributeType.SetValue("half_t", (int)Graphics::AttributeType::Half);
				VAttributeType.SetValue("float_t", (int)Graphics::AttributeType::Float);
				VAttributeType.SetValue("int_t", (int)Graphics::AttributeType::Int);
				VAttributeType.SetValue("uint_t", (int)Graphics::AttributeType::Uint);
				VAttributeType.SetValue("matrix_t", (int)Graphics::AttributeType::Matrix);

				TypeClass VMappedSubresource = Engine->SetPod<Graphics::MappedSubresource>("mapped_subresource");
				VMappedSubresource.SetProperty<Graphics::MappedSubresource>("uptr@ pointer", &Graphics::MappedSubresource::Pointer);
				VMappedSubresource.SetProperty<Graphics::MappedSubresource>("uint32 row_pitch", &Graphics::MappedSubresource::RowPitch);
				VMappedSubresource.SetProperty<Graphics::MappedSubresource>("uint32 depth_pitch", &Graphics::MappedSubresource::DepthPitch);
				VMappedSubresource.SetConstructor<Graphics::MappedSubresource>("void f()");

				TypeClass VRenderTargetBlendState = Engine->SetPod<Graphics::RenderTargetBlendState>("render_target_blend_state");
				VRenderTargetBlendState.SetProperty<Graphics::RenderTargetBlendState>("blend_t src_blend", &Graphics::RenderTargetBlendState::SrcBlend);
				VRenderTargetBlendState.SetProperty<Graphics::RenderTargetBlendState>("blend_t dest_blend", &Graphics::RenderTargetBlendState::DestBlend);
				VRenderTargetBlendState.SetProperty<Graphics::RenderTargetBlendState>("blend_operation blend_operation_mode", &Graphics::RenderTargetBlendState::BlendOperationMode);
				VRenderTargetBlendState.SetProperty<Graphics::RenderTargetBlendState>("blend_t src_blend_alpha", &Graphics::RenderTargetBlendState::SrcBlendAlpha);
				VRenderTargetBlendState.SetProperty<Graphics::RenderTargetBlendState>("blend_t dest_blend_alpha", &Graphics::RenderTargetBlendState::DestBlendAlpha);
				VRenderTargetBlendState.SetProperty<Graphics::RenderTargetBlendState>("blend_operation blend_operation_alpha", &Graphics::RenderTargetBlendState::BlendOperationAlpha);
				VRenderTargetBlendState.SetProperty<Graphics::RenderTargetBlendState>("uint8 render_target_write_mask", &Graphics::RenderTargetBlendState::RenderTargetWriteMask);
				VRenderTargetBlendState.SetProperty<Graphics::RenderTargetBlendState>("bool blend_enable", &Graphics::RenderTargetBlendState::BlendEnable);
				VRenderTargetBlendState.SetConstructor<Graphics::RenderTargetBlendState>("void f()");

				RefClass VSurface = Engine->SetClass<Graphics::Surface>("surface_handle", false);
				VSurface.SetConstructor<Graphics::Surface>("surface_handle@ f()");
				VSurface.SetConstructor<Graphics::Surface, SDL_Surface*>("surface_handle@ f(uptr@)");
				VSurface.SetMethod("void set_handle(uptr@)", &Graphics::Surface::SetHandle);
				VSurface.SetMethod("void lock()", &Graphics::Surface::Lock);
				VSurface.SetMethod("void unlock()", &Graphics::Surface::Unlock);
				VSurface.SetMethod("int get_width() const", &Graphics::Surface::GetWidth);
				VSurface.SetMethod("int get_height() const", &Graphics::Surface::GetHeight);
				VSurface.SetMethod("int get_pitch() const", &Graphics::Surface::GetPitch);
				VSurface.SetMethod("uptr@ get_pixels() const", &Graphics::Surface::GetPixels);
				VSurface.SetMethod("uptr@ get_resource() const", &Graphics::Surface::GetResource);

				TypeClass VDepthStencilStateDesc = Engine->SetPod<Graphics::DepthStencilState::Desc>("depth_stencil_state_desc");
				VDepthStencilStateDesc.SetProperty<Graphics::DepthStencilState::Desc>("stencil_operation front_face_stencil_fail_operation", &Graphics::DepthStencilState::Desc::FrontFaceStencilFailOperation);
				VDepthStencilStateDesc.SetProperty<Graphics::DepthStencilState::Desc>("stencil_operation front_face_stencil_depth_fail_operation", &Graphics::DepthStencilState::Desc::FrontFaceStencilDepthFailOperation);
				VDepthStencilStateDesc.SetProperty<Graphics::DepthStencilState::Desc>("stencil_operation front_face_stencil_pass_operation", &Graphics::DepthStencilState::Desc::FrontFaceStencilPassOperation);
				VDepthStencilStateDesc.SetProperty<Graphics::DepthStencilState::Desc>("comparison front_face_stencil_function", &Graphics::DepthStencilState::Desc::FrontFaceStencilFunction);
				VDepthStencilStateDesc.SetProperty<Graphics::DepthStencilState::Desc>("stencil_operation back_face_stencil_fail_operation", &Graphics::DepthStencilState::Desc::BackFaceStencilFailOperation);
				VDepthStencilStateDesc.SetProperty<Graphics::DepthStencilState::Desc>("stencil_operation back_face_stencil_depth_fail_operation", &Graphics::DepthStencilState::Desc::BackFaceStencilDepthFailOperation);
				VDepthStencilStateDesc.SetProperty<Graphics::DepthStencilState::Desc>("stencil_operation back_face_stencil_pass_operation", &Graphics::DepthStencilState::Desc::BackFaceStencilPassOperation);
				VDepthStencilStateDesc.SetProperty<Graphics::DepthStencilState::Desc>("comparison back_face_stencil_function", &Graphics::DepthStencilState::Desc::BackFaceStencilFunction);
				VDepthStencilStateDesc.SetProperty<Graphics::DepthStencilState::Desc>("depth_write depth_write_mask", &Graphics::DepthStencilState::Desc::DepthWriteMask);
				VDepthStencilStateDesc.SetProperty<Graphics::DepthStencilState::Desc>("comparison depth_function", &Graphics::DepthStencilState::Desc::DepthFunction);
				VDepthStencilStateDesc.SetProperty<Graphics::DepthStencilState::Desc>("uint8 stencil_read_mask", &Graphics::DepthStencilState::Desc::StencilReadMask);
				VDepthStencilStateDesc.SetProperty<Graphics::DepthStencilState::Desc>("uint8 stencil_write_mask", &Graphics::DepthStencilState::Desc::StencilWriteMask);
				VDepthStencilStateDesc.SetProperty<Graphics::DepthStencilState::Desc>("bool depth_enable", &Graphics::DepthStencilState::Desc::DepthEnable);
				VDepthStencilStateDesc.SetProperty<Graphics::DepthStencilState::Desc>("bool stencil_enable", &Graphics::DepthStencilState::Desc::StencilEnable);
				VDepthStencilStateDesc.SetConstructor<Graphics::DepthStencilState::Desc>("void f()");

				RefClass VDepthStencilState = Engine->SetClass<Graphics::DepthStencilState>("depth_stencil_state", false);
				VDepthStencilState.SetMethod("uptr@ get_resource() const", &Graphics::DepthStencilState::GetResource);
				VDepthStencilState.SetMethod("depth_stencil_state_desc get_state() const", &Graphics::DepthStencilState::GetState);

				TypeClass VRasterizerStateDesc = Engine->SetPod<Graphics::RasterizerState::Desc>("rasterizer_state_desc");
				VRasterizerStateDesc.SetProperty<Graphics::RasterizerState::Desc>("surface_fill fill_mode", &Graphics::RasterizerState::Desc::FillMode);
				VRasterizerStateDesc.SetProperty<Graphics::RasterizerState::Desc>("vertex_cull cull_mode", &Graphics::RasterizerState::Desc::CullMode);
				VRasterizerStateDesc.SetProperty<Graphics::RasterizerState::Desc>("float depth_bias_clamp", &Graphics::RasterizerState::Desc::DepthBiasClamp);
				VRasterizerStateDesc.SetProperty<Graphics::RasterizerState::Desc>("float slope_scaled_depth_bias", &Graphics::RasterizerState::Desc::SlopeScaledDepthBias);
				VRasterizerStateDesc.SetProperty<Graphics::RasterizerState::Desc>("int32 depth_bias", &Graphics::RasterizerState::Desc::DepthBias);
				VRasterizerStateDesc.SetProperty<Graphics::RasterizerState::Desc>("bool front_counter_clockwise", &Graphics::RasterizerState::Desc::FrontCounterClockwise);
				VRasterizerStateDesc.SetProperty<Graphics::RasterizerState::Desc>("bool depth_clip_enable", &Graphics::RasterizerState::Desc::DepthClipEnable);
				VRasterizerStateDesc.SetProperty<Graphics::RasterizerState::Desc>("bool scissor_enable", &Graphics::RasterizerState::Desc::ScissorEnable);
				VRasterizerStateDesc.SetProperty<Graphics::RasterizerState::Desc>("bool multisample_enable", &Graphics::RasterizerState::Desc::MultisampleEnable);
				VRasterizerStateDesc.SetProperty<Graphics::RasterizerState::Desc>("bool antialiased_line_enable", &Graphics::RasterizerState::Desc::AntialiasedLineEnable);
				VRasterizerStateDesc.SetConstructor<Graphics::RasterizerState::Desc>("void f()");

				RefClass VRasterizerState = Engine->SetClass<Graphics::RasterizerState>("rasterizer_state", false);
				VRasterizerState.SetMethod("uptr@ get_resource() const", &Graphics::RasterizerState::GetResource);
				VRasterizerState.SetMethod("rasterizer_state_desc get_state() const", &Graphics::RasterizerState::GetState);

				TypeClass VBlendStateDesc = Engine->SetPod<Graphics::BlendState::Desc>("blend_state_desc");
				VBlendStateDesc.SetProperty<Graphics::BlendState::Desc>("bool alpha_to_coverage_enable", &Graphics::BlendState::Desc::AlphaToCoverageEnable);
				VBlendStateDesc.SetProperty<Graphics::BlendState::Desc>("bool independent_blend_enable", &Graphics::BlendState::Desc::IndependentBlendEnable);
				VBlendStateDesc.SetConstructor<Graphics::BlendState::Desc>("void f()");
				VBlendStateDesc.SetOperatorEx(Operators::Index, (uint32_t)Position::Left, "render_target_blend_state&", "usize", &BlendStateDescGetRenderTarget);
				VBlendStateDesc.SetOperatorEx(Operators::Index, (uint32_t)Position::Const, "const render_target_blend_state&", "usize", &BlendStateDescGetRenderTarget);

				RefClass VBlendState = Engine->SetClass<Graphics::BlendState>("blend_state", false);
				VBlendState.SetMethod("uptr@ get_resource() const", &Graphics::BlendState::GetResource);
				VBlendState.SetMethod("blend_state_desc get_state() const", &Graphics::BlendState::GetState);

				TypeClass VSamplerStateDesc = Engine->SetPod<Graphics::SamplerState::Desc>("sampler_state_desc");
				VSamplerStateDesc.SetProperty<Graphics::SamplerState::Desc>("comparison comparison_function", &Graphics::SamplerState::Desc::ComparisonFunction);
				VSamplerStateDesc.SetProperty<Graphics::SamplerState::Desc>("texture_address address_u", &Graphics::SamplerState::Desc::AddressU);
				VSamplerStateDesc.SetProperty<Graphics::SamplerState::Desc>("texture_address address_v", &Graphics::SamplerState::Desc::AddressV);
				VSamplerStateDesc.SetProperty<Graphics::SamplerState::Desc>("texture_address address_w", &Graphics::SamplerState::Desc::AddressW);
				VSamplerStateDesc.SetProperty<Graphics::SamplerState::Desc>("pixel_filter filter", &Graphics::SamplerState::Desc::Filter);
				VSamplerStateDesc.SetProperty<Graphics::SamplerState::Desc>("float mip_lod_bias", &Graphics::SamplerState::Desc::MipLODBias);
				VSamplerStateDesc.SetProperty<Graphics::SamplerState::Desc>("uint32 max_anisotropy", &Graphics::SamplerState::Desc::MaxAnisotropy);
				VSamplerStateDesc.SetProperty<Graphics::SamplerState::Desc>("float min_lod", &Graphics::SamplerState::Desc::MinLOD);
				VSamplerStateDesc.SetProperty<Graphics::SamplerState::Desc>("float max_lod", &Graphics::SamplerState::Desc::MaxLOD);
				VSamplerStateDesc.SetConstructor<Graphics::SamplerState::Desc>("void f()");
				VSamplerStateDesc.SetOperatorEx(Operators::Index, (uint32_t)Position::Left, "float&", "usize", &SamplerStateDescGetBorderColor);
				VSamplerStateDesc.SetOperatorEx(Operators::Index, (uint32_t)Position::Const, "const float&", "usize", &SamplerStateDescGetBorderColor);

				RefClass VSamplerState = Engine->SetClass<Graphics::SamplerState>("sampler_state", false);
				VSamplerState.SetMethod("uptr@ get_resource() const", &Graphics::SamplerState::GetResource);
				VSamplerState.SetMethod("sampler_state_desc get_state() const", &Graphics::SamplerState::GetState);

				TypeClass VInputLayoutAttribute = Engine->SetStructTrivial<Graphics::InputLayout::Attribute>("input_layout_attribute");
				VInputLayoutAttribute.SetProperty<Graphics::InputLayout::Attribute>("string semantic_name", &Graphics::InputLayout::Attribute::SemanticName);
				VInputLayoutAttribute.SetProperty<Graphics::InputLayout::Attribute>("uint32 semantic_index", &Graphics::InputLayout::Attribute::SemanticIndex);
				VInputLayoutAttribute.SetProperty<Graphics::InputLayout::Attribute>("attribute_type format", &Graphics::InputLayout::Attribute::Format);
				VInputLayoutAttribute.SetProperty<Graphics::InputLayout::Attribute>("uint32 components", &Graphics::InputLayout::Attribute::Components);
				VInputLayoutAttribute.SetProperty<Graphics::InputLayout::Attribute>("uint32 aligned_byte_offset", &Graphics::InputLayout::Attribute::AlignedByteOffset);
				VInputLayoutAttribute.SetProperty<Graphics::InputLayout::Attribute>("uint32 slot", &Graphics::InputLayout::Attribute::Slot);
				VInputLayoutAttribute.SetProperty<Graphics::InputLayout::Attribute>("bool per_vertex", &Graphics::InputLayout::Attribute::PerVertex);
				VInputLayoutAttribute.SetConstructor<Graphics::InputLayout::Attribute>("void f()");

				RefClass VShader = Engine->SetClass<Graphics::Shader>("shader", false);
				VShader.SetMethod("bool is_valid() const", &Graphics::Shader::IsValid);

				TypeClass VInputLayoutDesc = Engine->SetStruct<Graphics::InputLayout::Desc>("input_layout_desc");
				VInputLayoutDesc.SetProperty<Graphics::InputLayout::Desc>("shader@ source", &Graphics::InputLayout::Desc::Source);
				VInputLayoutDesc.SetConstructor<Graphics::InputLayout::Desc>("void f()");
				VInputLayoutDesc.SetOperatorCopyStatic(&InputLayoutDescCopy);
				VInputLayoutDesc.SetDestructorStatic("void f()", &InputLayoutDescDestructor);
				VInputLayoutDesc.SetMethodEx("void set_attributes(array<input_layout_attribute>@+)", &InputLayoutDescSetAttributes);

				RefClass VInputLayout = Engine->SetClass<Graphics::InputLayout>("input_layout", false);
				VInputLayout.SetMethod("uptr@ get_resource() const", &Graphics::InputLayout::GetResource);
				VInputLayout.SetMethodEx("array<input_layout_attribute>@ get_attributes() const", &InputLayoutGetAttributes);

				TypeClass VShaderDesc = Engine->SetStructTrivial<Graphics::Shader::Desc>("shader_desc");
				VShaderDesc.SetProperty<Graphics::Shader::Desc>("string filename", &Graphics::Shader::Desc::Filename);
				VShaderDesc.SetProperty<Graphics::Shader::Desc>("string data", &Graphics::Shader::Desc::Data);
				VShaderDesc.SetProperty<Graphics::Shader::Desc>("shader_type stage", &Graphics::Shader::Desc::Stage);
				VShaderDesc.SetConstructor<Graphics::Shader::Desc>("void f()");
				VShaderDesc.SetMethodEx("void set_defines(array<input_layout_attribute>@+)", &ShaderDescSetDefines);

				TypeClass VElementBufferDesc = Engine->SetStructTrivial<Graphics::ElementBuffer::Desc>("element_buffer_desc");
				VElementBufferDesc.SetProperty<Graphics::ElementBuffer::Desc>("uptr@ elements", &Graphics::ElementBuffer::Desc::Elements);
				VElementBufferDesc.SetProperty<Graphics::ElementBuffer::Desc>("uint32 structure_byte_stride", &Graphics::ElementBuffer::Desc::StructureByteStride);
				VElementBufferDesc.SetProperty<Graphics::ElementBuffer::Desc>("uint32 element_width", &Graphics::ElementBuffer::Desc::ElementWidth);
				VElementBufferDesc.SetProperty<Graphics::ElementBuffer::Desc>("uint32 element_count", &Graphics::ElementBuffer::Desc::ElementCount);
				VElementBufferDesc.SetProperty<Graphics::ElementBuffer::Desc>("cpu_access access_flags", &Graphics::ElementBuffer::Desc::AccessFlags);
				VElementBufferDesc.SetProperty<Graphics::ElementBuffer::Desc>("resource_usage usage", &Graphics::ElementBuffer::Desc::Usage);
				VElementBufferDesc.SetProperty<Graphics::ElementBuffer::Desc>("resource_bind bind_flags", &Graphics::ElementBuffer::Desc::BindFlags);
				VElementBufferDesc.SetProperty<Graphics::ElementBuffer::Desc>("resource_misc misc_flags", &Graphics::ElementBuffer::Desc::MiscFlags);
				VElementBufferDesc.SetProperty<Graphics::ElementBuffer::Desc>("bool writable", &Graphics::ElementBuffer::Desc::Writable);
				VElementBufferDesc.SetConstructor<Graphics::ElementBuffer::Desc>("void f()");

				RefClass VElementBuffer = Engine->SetClass<Graphics::ElementBuffer>("element_buffer", false);
				VElementBuffer.SetMethod("uptr@ get_resource() const", &Graphics::ElementBuffer::GetResource);
				VElementBuffer.SetMethod("usize get_elements() const", &Graphics::ElementBuffer::GetElements);
				VElementBuffer.SetMethod("usize get_stride() const", &Graphics::ElementBuffer::GetStride);

				TypeClass VMeshBufferDesc = Engine->SetStructTrivial<Graphics::MeshBuffer::Desc>("mesh_buffer_desc");
				VMeshBufferDesc.SetProperty<Graphics::MeshBuffer::Desc>("cpu_access access_flags", &Graphics::MeshBuffer::Desc::AccessFlags);
				VMeshBufferDesc.SetProperty<Graphics::MeshBuffer::Desc>("resource_usage usage", &Graphics::MeshBuffer::Desc::Usage);
				VMeshBufferDesc.SetConstructor<Graphics::MeshBuffer::Desc>("void f()");
				VMeshBufferDesc.SetMethodEx("void set_elements(array<vertex>@+)", &MeshBufferDescSetElements);
				VMeshBufferDesc.SetMethodEx("void set_indices(array<int>@+)", &MeshBufferDescSetIndices);

				RefClass VMeshBuffer = Engine->SetClass<Graphics::MeshBuffer>("mesh_buffer", true);
				VMeshBuffer.SetProperty<Graphics::MeshBuffer>("matrix4x4 transform", &Graphics::MeshBuffer::Transform);
				VMeshBuffer.SetProperty<Graphics::MeshBuffer>("string name", &Graphics::MeshBuffer::Name);
				VMeshBuffer.SetMethod("element_buffer@+ get_vertex_buffer() const", &Graphics::MeshBuffer::GetVertexBuffer);
				VMeshBuffer.SetMethod("element_buffer@+ get_index_buffer() const", &Graphics::MeshBuffer::GetIndexBuffer);
				VMeshBuffer.SetEnumRefsEx<Graphics::MeshBuffer>([](Graphics::MeshBuffer* Base, asIScriptEngine* Engine)
				{
					Engine->GCEnumCallback(Base->GetVertexBuffer());
					Engine->GCEnumCallback(Base->GetIndexBuffer());
				});
				VMeshBuffer.SetReleaseRefsEx<Graphics::MeshBuffer>([](Graphics::MeshBuffer* Base, asIScriptEngine*)
				{
					Base->~MeshBuffer();
				});

				TypeClass VSkinMeshBufferDesc = Engine->SetStructTrivial<Graphics::SkinMeshBuffer::Desc>("skin_mesh_buffer_desc");
				VSkinMeshBufferDesc.SetProperty<Graphics::SkinMeshBuffer::Desc>("cpu_access access_flags", &Graphics::SkinMeshBuffer::Desc::AccessFlags);
				VSkinMeshBufferDesc.SetProperty<Graphics::SkinMeshBuffer::Desc>("resource_usage usage", &Graphics::SkinMeshBuffer::Desc::Usage);
				VSkinMeshBufferDesc.SetConstructor<Graphics::SkinMeshBuffer::Desc>("void f()");
				VSkinMeshBufferDesc.SetMethodEx("void set_elements(array<vertex>@+)", &SkinMeshBufferDescSetElements);
				VSkinMeshBufferDesc.SetMethodEx("void set_indices(array<int>@+)", &SkinMeshBufferDescSetIndices);

				RefClass VSkinMeshBuffer = Engine->SetClass<Graphics::SkinMeshBuffer>("skin_mesh_buffer", true);
				VSkinMeshBuffer.SetProperty<Graphics::SkinMeshBuffer>("matrix4x4 transform", &Graphics::SkinMeshBuffer::Transform);
				VSkinMeshBuffer.SetProperty<Graphics::SkinMeshBuffer>("string name", &Graphics::SkinMeshBuffer::Name);
				VSkinMeshBuffer.SetMethod("element_buffer@+ get_vertex_buffer() const", &Graphics::SkinMeshBuffer::GetVertexBuffer);
				VSkinMeshBuffer.SetMethod("element_buffer@+ get_index_buffer() const", &Graphics::SkinMeshBuffer::GetIndexBuffer);
				VSkinMeshBuffer.SetEnumRefsEx<Graphics::SkinMeshBuffer>([](Graphics::SkinMeshBuffer* Base, asIScriptEngine* Engine)
				{
					Engine->GCEnumCallback(Base->GetVertexBuffer());
					Engine->GCEnumCallback(Base->GetIndexBuffer());
				});
				VSkinMeshBuffer.SetReleaseRefsEx<Graphics::SkinMeshBuffer>([](Graphics::SkinMeshBuffer* Base, asIScriptEngine*)
				{
					Base->~SkinMeshBuffer();
				});

				RefClass VGraphicsDevice = Engine->SetClass<Graphics::GraphicsDevice>("graphics_device", true);
				TypeClass VInstanceBufferDesc = Engine->SetStruct<Graphics::InstanceBuffer::Desc>("instance_buffer_desc");
				VInstanceBufferDesc.SetProperty<Graphics::InstanceBuffer::Desc>("graphics_device@ device", &Graphics::InstanceBuffer::Desc::Device);
				VInstanceBufferDesc.SetProperty<Graphics::InstanceBuffer::Desc>("uint32 element_width", &Graphics::InstanceBuffer::Desc::ElementWidth);
				VInstanceBufferDesc.SetProperty<Graphics::InstanceBuffer::Desc>("uint32 element_limit", &Graphics::InstanceBuffer::Desc::ElementLimit);
				VInstanceBufferDesc.SetConstructor<Graphics::InstanceBuffer::Desc>("void f()");
				VInstanceBufferDesc.SetOperatorCopyStatic(&InstanceBufferDescCopy);
				VInstanceBufferDesc.SetDestructorStatic("void f()", &InstanceBufferDescDestructor);

				RefClass VInstanceBuffer = Engine->SetClass<Graphics::InstanceBuffer>("instance_buffer", true);
				VInstanceBuffer.SetMethodEx("void set_array(array<element_vertex>@+)", &InstanceBufferSetArray);
				VInstanceBuffer.SetMethodEx("array<element_vertex>@ get_array() const", &InstanceBufferGetArray);
				VInstanceBuffer.SetMethod("element_buffer@+ get_elements() const", &Graphics::InstanceBuffer::GetElements);
				VInstanceBuffer.SetMethod("graphics_device@+ get_device() const", &Graphics::InstanceBuffer::GetDevice);
				VInstanceBuffer.SetMethod("usize get_element_limit() const", &Graphics::InstanceBuffer::GetElementLimit);
				VInstanceBuffer.SetEnumRefsEx<Graphics::InstanceBuffer>([](Graphics::InstanceBuffer* Base, asIScriptEngine* Engine)
				{
					Engine->GCEnumCallback(Base->GetElements());
				});
				VInstanceBuffer.SetReleaseRefsEx<Graphics::InstanceBuffer>([](Graphics::InstanceBuffer* Base, asIScriptEngine*)
				{
					Base->~InstanceBuffer();
				});

				TypeClass VTexture2DDesc = Engine->SetPod<Graphics::Texture2D::Desc>("texture_2d_desc");
				VTexture2DDesc.SetProperty<Graphics::Texture2D::Desc>("cpu_access access_flags", &Graphics::Texture2D::Desc::AccessFlags);
				VTexture2DDesc.SetProperty<Graphics::Texture2D::Desc>("surface_format format_mode", &Graphics::Texture2D::Desc::FormatMode);
				VTexture2DDesc.SetProperty<Graphics::Texture2D::Desc>("resource_usage usage", &Graphics::Texture2D::Desc::Usage);
				VTexture2DDesc.SetProperty<Graphics::Texture2D::Desc>("resource_bind bind_flags", &Graphics::Texture2D::Desc::BindFlags);
				VTexture2DDesc.SetProperty<Graphics::Texture2D::Desc>("resource_misc misc_flags", &Graphics::Texture2D::Desc::MiscFlags);
				VTexture2DDesc.SetProperty<Graphics::Texture2D::Desc>("uptr@ data", &Graphics::Texture2D::Desc::Data);
				VTexture2DDesc.SetProperty<Graphics::Texture2D::Desc>("uint32 row_pitch", &Graphics::Texture2D::Desc::RowPitch);
				VTexture2DDesc.SetProperty<Graphics::Texture2D::Desc>("uint32 depth_pitch", &Graphics::Texture2D::Desc::DepthPitch);
				VTexture2DDesc.SetProperty<Graphics::Texture2D::Desc>("uint32 width", &Graphics::Texture2D::Desc::Width);
				VTexture2DDesc.SetProperty<Graphics::Texture2D::Desc>("uint32 height", &Graphics::Texture2D::Desc::Height);
				VTexture2DDesc.SetProperty<Graphics::Texture2D::Desc>("int32 mip_levels", &Graphics::Texture2D::Desc::MipLevels);
				VTexture2DDesc.SetProperty<Graphics::Texture2D::Desc>("bool writable", &Graphics::Texture2D::Desc::Writable);
				VTexture2DDesc.SetConstructor<Graphics::Texture2D::Desc>("void f()");

				RefClass VTexture2D = Engine->SetClass<Graphics::Texture2D>("texture_2d", false);
				VTexture2D.SetMethod("uptr@ get_resource() const", &Graphics::Texture2D::GetResource);
				VTexture2D.SetMethod("cpu_access get_access_flags() const", &Graphics::Texture2D::GetAccessFlags);
				VTexture2D.SetMethod("surface_format get_format_mode() const", &Graphics::Texture2D::GetFormatMode);
				VTexture2D.SetMethod("resource_usage get_usage() const", &Graphics::Texture2D::GetUsage);
				VTexture2D.SetMethod("resource_bind get_binding() const", &Graphics::Texture2D::GetBinding);
				VTexture2D.SetMethod("uint32 get_width() const", &Graphics::Texture2D::GetWidth);
				VTexture2D.SetMethod("uint32 get_height() const", &Graphics::Texture2D::GetHeight);
				VTexture2D.SetMethod("uint32 get_mip_levels() const", &Graphics::Texture2D::GetMipLevels);

				TypeClass VTexture3DDesc = Engine->SetPod<Graphics::Texture3D::Desc>("texture_3d_desc");
				VTexture3DDesc.SetProperty<Graphics::Texture3D::Desc>("cpu_access access_flags", &Graphics::Texture3D::Desc::AccessFlags);
				VTexture3DDesc.SetProperty<Graphics::Texture3D::Desc>("surface_format format_mode", &Graphics::Texture3D::Desc::FormatMode);
				VTexture3DDesc.SetProperty<Graphics::Texture3D::Desc>("resource_usage usage", &Graphics::Texture3D::Desc::Usage);
				VTexture3DDesc.SetProperty<Graphics::Texture3D::Desc>("resource_bind bind_flags", &Graphics::Texture3D::Desc::BindFlags);
				VTexture3DDesc.SetProperty<Graphics::Texture3D::Desc>("resource_misc misc_flags", &Graphics::Texture3D::Desc::MiscFlags);
				VTexture3DDesc.SetProperty<Graphics::Texture3D::Desc>("uint32 width", &Graphics::Texture3D::Desc::Width);
				VTexture3DDesc.SetProperty<Graphics::Texture3D::Desc>("uint32 height", &Graphics::Texture3D::Desc::Height);
				VTexture3DDesc.SetProperty<Graphics::Texture3D::Desc>("uint32 depth", &Graphics::Texture3D::Desc::Depth);
				VTexture3DDesc.SetProperty<Graphics::Texture3D::Desc>("int32 mip_levels", &Graphics::Texture3D::Desc::MipLevels);
				VTexture3DDesc.SetProperty<Graphics::Texture3D::Desc>("bool writable", &Graphics::Texture3D::Desc::Writable);
				VTexture3DDesc.SetConstructor<Graphics::Texture3D::Desc>("void f()");

				RefClass VTexture3D = Engine->SetClass<Graphics::Texture3D>("texture_3d", false);
				VTexture3D.SetMethod("uptr@ get_resource() const", &Graphics::Texture3D::GetResource);
				VTexture3D.SetMethod("cpu_access get_access_flags() const", &Graphics::Texture3D::GetAccessFlags);
				VTexture3D.SetMethod("surface_format get_format_mode() const", &Graphics::Texture3D::GetFormatMode);
				VTexture3D.SetMethod("resource_usage get_usage() const", &Graphics::Texture3D::GetUsage);
				VTexture3D.SetMethod("resource_bind get_binding() const", &Graphics::Texture3D::GetBinding);
				VTexture3D.SetMethod("uint32 get_width() const", &Graphics::Texture3D::GetWidth);
				VTexture3D.SetMethod("uint32 get_height() const", &Graphics::Texture3D::GetHeight);
				VTexture3D.SetMethod("uint32 get_depth() const", &Graphics::Texture3D::GetDepth);
				VTexture3D.SetMethod("uint32 get_mip_levels() const", &Graphics::Texture3D::GetMipLevels);

				TypeClass VTextureCubeDesc = Engine->SetPod<Graphics::TextureCube::Desc>("texture_cube_desc");
				VTextureCubeDesc.SetProperty<Graphics::TextureCube::Desc>("cpu_access access_flags", &Graphics::TextureCube::Desc::AccessFlags);
				VTextureCubeDesc.SetProperty<Graphics::TextureCube::Desc>("surface_format format_mode", &Graphics::TextureCube::Desc::FormatMode);
				VTextureCubeDesc.SetProperty<Graphics::TextureCube::Desc>("resource_usage usage", &Graphics::TextureCube::Desc::Usage);
				VTextureCubeDesc.SetProperty<Graphics::TextureCube::Desc>("resource_bind bind_flags", &Graphics::TextureCube::Desc::BindFlags);
				VTextureCubeDesc.SetProperty<Graphics::TextureCube::Desc>("resource_misc misc_flags", &Graphics::TextureCube::Desc::MiscFlags);
				VTextureCubeDesc.SetProperty<Graphics::TextureCube::Desc>("uint32 width", &Graphics::TextureCube::Desc::Width);
				VTextureCubeDesc.SetProperty<Graphics::TextureCube::Desc>("uint32 height", &Graphics::TextureCube::Desc::Height);
				VTextureCubeDesc.SetProperty<Graphics::TextureCube::Desc>("int32 mip_levels", &Graphics::TextureCube::Desc::MipLevels);
				VTextureCubeDesc.SetProperty<Graphics::TextureCube::Desc>("bool writable", &Graphics::TextureCube::Desc::Writable);
				VTextureCubeDesc.SetConstructor<Graphics::TextureCube::Desc>("void f()");

				RefClass VTextureCube = Engine->SetClass<Graphics::TextureCube>("texture_cube", false);
				VTextureCube.SetMethod("uptr@ get_resource() const", &Graphics::TextureCube::GetResource);
				VTextureCube.SetMethod("cpu_access get_access_flags() const", &Graphics::TextureCube::GetAccessFlags);
				VTextureCube.SetMethod("surface_format get_format_mode() const", &Graphics::TextureCube::GetFormatMode);
				VTextureCube.SetMethod("resource_usage get_usage() const", &Graphics::TextureCube::GetUsage);
				VTextureCube.SetMethod("resource_bind get_binding() const", &Graphics::TextureCube::GetBinding);
				VTextureCube.SetMethod("uint32 get_width() const", &Graphics::TextureCube::GetWidth);
				VTextureCube.SetMethod("uint32 get_height() const", &Graphics::TextureCube::GetHeight);
				VTextureCube.SetMethod("uint32 get_mip_levels() const", &Graphics::TextureCube::GetMipLevels);

				TypeClass VDepthTarget2DDesc = Engine->SetPod<Graphics::DepthTarget2D::Desc>("depth_target_2d_desc");
				VDepthTarget2DDesc.SetProperty<Graphics::DepthTarget2D::Desc>("cpu_access access_flags", &Graphics::DepthTarget2D::Desc::AccessFlags);
				VDepthTarget2DDesc.SetProperty<Graphics::DepthTarget2D::Desc>("resource_usage usage", &Graphics::DepthTarget2D::Desc::Usage);
				VDepthTarget2DDesc.SetProperty<Graphics::DepthTarget2D::Desc>("surface_format format_mode", &Graphics::DepthTarget2D::Desc::FormatMode);
				VDepthTarget2DDesc.SetProperty<Graphics::DepthTarget2D::Desc>("uint32 width", &Graphics::DepthTarget2D::Desc::Width);
				VDepthTarget2DDesc.SetProperty<Graphics::DepthTarget2D::Desc>("uint32 height", &Graphics::DepthTarget2D::Desc::Height);
				VDepthTarget2DDesc.SetConstructor<Graphics::DepthTarget2D::Desc>("void f()");

				RefClass VDepthTarget2D = Engine->SetClass<Graphics::DepthTarget2D>("depth_target_2d", false);
				VDepthTarget2D.SetMethod("uptr@ get_resource() const", &Graphics::DepthTarget2D::GetResource);
				VDepthTarget2D.SetMethod("uint32 get_width() const", &Graphics::DepthTarget2D::GetWidth);
				VDepthTarget2D.SetMethod("uint32 get_height() const", &Graphics::DepthTarget2D::GetHeight);
				VDepthTarget2D.SetMethod("texture_2d@+ get_target() const", &Graphics::DepthTarget2D::GetTarget);
				VDepthTarget2D.SetMethod("const viewport& get_viewport() const", &Graphics::DepthTarget2D::GetViewport);

				TypeClass VDepthTargetCubeDesc = Engine->SetPod<Graphics::DepthTargetCube::Desc>("depth_target_cube_desc");
				VDepthTargetCubeDesc.SetProperty<Graphics::DepthTargetCube::Desc>("cpu_access access_flags", &Graphics::DepthTargetCube::Desc::AccessFlags);
				VDepthTargetCubeDesc.SetProperty<Graphics::DepthTargetCube::Desc>("resource_usage usage", &Graphics::DepthTargetCube::Desc::Usage);
				VDepthTargetCubeDesc.SetProperty<Graphics::DepthTargetCube::Desc>("surface_format format_mode", &Graphics::DepthTargetCube::Desc::FormatMode);
				VDepthTargetCubeDesc.SetProperty<Graphics::DepthTargetCube::Desc>("uint32 size", &Graphics::DepthTargetCube::Desc::Size);
				VDepthTargetCubeDesc.SetConstructor<Graphics::DepthTargetCube::Desc>("void f()");

				RefClass VDepthTargetCube = Engine->SetClass<Graphics::DepthTargetCube>("depth_target_cube", false);
				VDepthTargetCube.SetMethod("uptr@ get_resource() const", &Graphics::DepthTargetCube::GetResource);
				VDepthTargetCube.SetMethod("uint32 get_width() const", &Graphics::DepthTargetCube::GetWidth);
				VDepthTargetCube.SetMethod("uint32 get_height() const", &Graphics::DepthTargetCube::GetHeight);
				VDepthTargetCube.SetMethod("texture_2d@+ get_target() const", &Graphics::DepthTargetCube::GetTarget);
				VDepthTargetCube.SetMethod("const viewport& get_viewport() const", &Graphics::DepthTargetCube::GetViewport);

				RefClass VRenderTarget = Engine->SetClass<Graphics::RenderTarget>("render_target", false);
				VRenderTarget.SetMethod("uptr@ get_target_buffer() const", &Graphics::RenderTarget::GetTargetBuffer);
				VRenderTarget.SetMethod("uptr@ get_depth_buffer() const", &Graphics::RenderTarget::GetDepthBuffer);
				VRenderTarget.SetMethod("uint32 get_width() const", &Graphics::RenderTarget::GetWidth);
				VRenderTarget.SetMethod("uint32 get_height() const", &Graphics::RenderTarget::GetHeight);
				VRenderTarget.SetMethod("uint32 get_target_count() const", &Graphics::RenderTarget::GetTargetCount);
				VRenderTarget.SetMethod("texture_2d@+ get_target_2d(uint32) const", &Graphics::RenderTarget::GetTarget2D);
				VRenderTarget.SetMethod("texture_cube@+ get_target_cube(uint32) const", &Graphics::RenderTarget::GetTargetCube);
				VRenderTarget.SetMethod("texture_2d@+ get_depth_stencil() const", &Graphics::RenderTarget::GetDepthStencil);
				VRenderTarget.SetMethod("const viewport& get_viewport() const", &Graphics::RenderTarget::GetViewport);

				TypeClass VRenderTarget2DDesc = Engine->SetPod<Graphics::RenderTarget2D::Desc>("render_target_2d_desc");
				VRenderTarget2DDesc.SetProperty<Graphics::RenderTarget2D::Desc>("cpu_access access_flags", &Graphics::RenderTarget2D::Desc::AccessFlags);
				VRenderTarget2DDesc.SetProperty<Graphics::RenderTarget2D::Desc>("surface_format format_mode", &Graphics::RenderTarget2D::Desc::FormatMode);
				VRenderTarget2DDesc.SetProperty<Graphics::RenderTarget2D::Desc>("resource_usage usage", &Graphics::RenderTarget2D::Desc::Usage);
				VRenderTarget2DDesc.SetProperty<Graphics::RenderTarget2D::Desc>("resource_bind bind_flags", &Graphics::RenderTarget2D::Desc::BindFlags);
				VRenderTarget2DDesc.SetProperty<Graphics::RenderTarget2D::Desc>("resource_misc misc_flags", &Graphics::RenderTarget2D::Desc::MiscFlags);
				VRenderTarget2DDesc.SetProperty<Graphics::RenderTarget2D::Desc>("uptr@ render_surface", &Graphics::RenderTarget2D::Desc::RenderSurface);
				VRenderTarget2DDesc.SetProperty<Graphics::RenderTarget2D::Desc>("uint32 width", &Graphics::RenderTarget2D::Desc::Width);
				VRenderTarget2DDesc.SetProperty<Graphics::RenderTarget2D::Desc>("uint32 height", &Graphics::RenderTarget2D::Desc::Height);
				VRenderTarget2DDesc.SetProperty<Graphics::RenderTarget2D::Desc>("uint32 mip_levels", &Graphics::RenderTarget2D::Desc::MipLevels);
				VRenderTarget2DDesc.SetProperty<Graphics::RenderTarget2D::Desc>("bool depth_stencil", &Graphics::RenderTarget2D::Desc::DepthStencil);
				VRenderTarget2DDesc.SetConstructor<Graphics::RenderTarget2D::Desc>("void f()");

				RefClass VRenderTarget2D = Engine->SetClass<Graphics::RenderTarget2D>("render_target_2d", false);
				VRenderTarget2D.SetMethod("uptr@ get_target_buffer() const", &Graphics::RenderTarget2D::GetTargetBuffer);
				VRenderTarget2D.SetMethod("uptr@ get_depth_buffer() const", &Graphics::RenderTarget2D::GetDepthBuffer);
				VRenderTarget2D.SetMethod("uint32 get_width() const", &Graphics::RenderTarget2D::GetWidth);
				VRenderTarget2D.SetMethod("uint32 get_height() const", &Graphics::RenderTarget2D::GetHeight);
				VRenderTarget2D.SetMethod("uint32 get_target_count() const", &Graphics::RenderTarget2D::GetTargetCount);
				VRenderTarget2D.SetMethod("texture_2d@+ get_target_2d(uint32) const", &Graphics::RenderTarget2D::GetTarget2D);
				VRenderTarget2D.SetMethod("texture_cube@+ get_target_cube(uint32) const", &Graphics::RenderTarget2D::GetTargetCube);
				VRenderTarget2D.SetMethod("texture_2d@+ get_depth_stencil() const", &Graphics::RenderTarget2D::GetDepthStencil);
				VRenderTarget2D.SetMethod("const viewport& get_viewport() const", &Graphics::RenderTarget2D::GetViewport);
				VRenderTarget2D.SetMethod("texture_2d@+ get_target() const", &Graphics::RenderTarget2D::GetTarget);

				TypeClass VMultiRenderTarget2DDesc = Engine->SetPod<Graphics::MultiRenderTarget2D::Desc>("multi_render_target_2d_desc");
				VMultiRenderTarget2DDesc.SetProperty<Graphics::MultiRenderTarget2D::Desc>("cpu_access access_flags", &Graphics::MultiRenderTarget2D::Desc::AccessFlags);
				VMultiRenderTarget2DDesc.SetProperty<Graphics::MultiRenderTarget2D::Desc>("surface_target target", &Graphics::MultiRenderTarget2D::Desc::Target);
				VMultiRenderTarget2DDesc.SetProperty<Graphics::MultiRenderTarget2D::Desc>("resource_usage usage", &Graphics::MultiRenderTarget2D::Desc::Usage);
				VMultiRenderTarget2DDesc.SetProperty<Graphics::MultiRenderTarget2D::Desc>("resource_bind bind_flags", &Graphics::MultiRenderTarget2D::Desc::BindFlags);
				VMultiRenderTarget2DDesc.SetProperty<Graphics::MultiRenderTarget2D::Desc>("resource_misc misc_flags", &Graphics::MultiRenderTarget2D::Desc::MiscFlags);
				VMultiRenderTarget2DDesc.SetProperty<Graphics::MultiRenderTarget2D::Desc>("uint32 width", &Graphics::MultiRenderTarget2D::Desc::Width);
				VMultiRenderTarget2DDesc.SetProperty<Graphics::MultiRenderTarget2D::Desc>("uint32 height", &Graphics::MultiRenderTarget2D::Desc::Height);
				VMultiRenderTarget2DDesc.SetProperty<Graphics::MultiRenderTarget2D::Desc>("uint32 mip_levels", &Graphics::MultiRenderTarget2D::Desc::MipLevels);
				VMultiRenderTarget2DDesc.SetProperty<Graphics::MultiRenderTarget2D::Desc>("bool depth_stencil", &Graphics::MultiRenderTarget2D::Desc::DepthStencil);
				VMultiRenderTarget2DDesc.SetConstructor<Graphics::MultiRenderTarget2D::Desc>("void f()");
				VMultiRenderTarget2DDesc.SetMethodEx("void set_format_mode(usize, surface_format)", &MultiRenderTarget2DDescSetFormatMode);

				RefClass VMultiRenderTarget2D = Engine->SetClass<Graphics::MultiRenderTarget2D>("multi_render_target_2d", false);
				VMultiRenderTarget2D.SetMethod("uptr@ get_target_buffer() const", &Graphics::MultiRenderTarget2D::GetTargetBuffer);
				VMultiRenderTarget2D.SetMethod("uptr@ get_depth_buffer() const", &Graphics::MultiRenderTarget2D::GetDepthBuffer);
				VMultiRenderTarget2D.SetMethod("uint32 get_width() const", &Graphics::MultiRenderTarget2D::GetWidth);
				VMultiRenderTarget2D.SetMethod("uint32 get_height() const", &Graphics::MultiRenderTarget2D::GetHeight);
				VMultiRenderTarget2D.SetMethod("uint32 get_target_count() const", &Graphics::MultiRenderTarget2D::GetTargetCount);
				VMultiRenderTarget2D.SetMethod("texture_2d@+ get_target_2d(uint32) const", &Graphics::MultiRenderTarget2D::GetTarget2D);
				VMultiRenderTarget2D.SetMethod("texture_cube@+ get_target_cube(uint32) const", &Graphics::MultiRenderTarget2D::GetTargetCube);
				VMultiRenderTarget2D.SetMethod("texture_2d@+ get_depth_stencil() const", &Graphics::MultiRenderTarget2D::GetDepthStencil);
				VMultiRenderTarget2D.SetMethod("const viewport& get_viewport() const", &Graphics::MultiRenderTarget2D::GetViewport);
				VMultiRenderTarget2D.SetMethod("texture_2d@+ get_target(uint32) const", &Graphics::MultiRenderTarget2D::GetTarget);

				TypeClass VRenderTargetCubeDesc = Engine->SetPod<Graphics::RenderTargetCube::Desc>("render_target_cube_desc");
				VRenderTargetCubeDesc.SetProperty<Graphics::RenderTargetCube::Desc>("cpu_access access_flags", &Graphics::RenderTargetCube::Desc::AccessFlags);
				VRenderTargetCubeDesc.SetProperty<Graphics::RenderTargetCube::Desc>("surface_format format_mode", &Graphics::RenderTargetCube::Desc::FormatMode);
				VRenderTargetCubeDesc.SetProperty<Graphics::RenderTargetCube::Desc>("resource_usage usage", &Graphics::RenderTargetCube::Desc::Usage);
				VRenderTargetCubeDesc.SetProperty<Graphics::RenderTargetCube::Desc>("resource_bind bind_flags", &Graphics::RenderTargetCube::Desc::BindFlags);
				VRenderTargetCubeDesc.SetProperty<Graphics::RenderTargetCube::Desc>("resource_misc misc_flags", &Graphics::RenderTargetCube::Desc::MiscFlags);
				VRenderTargetCubeDesc.SetProperty<Graphics::RenderTargetCube::Desc>("uint32 size", &Graphics::RenderTargetCube::Desc::Size);
				VRenderTargetCubeDesc.SetProperty<Graphics::RenderTargetCube::Desc>("uint32 mip_levels", &Graphics::RenderTargetCube::Desc::MipLevels);
				VRenderTargetCubeDesc.SetProperty<Graphics::RenderTargetCube::Desc>("bool depth_stencil", &Graphics::RenderTargetCube::Desc::DepthStencil);
				VRenderTargetCubeDesc.SetConstructor<Graphics::RenderTargetCube::Desc>("void f()");

				RefClass VRenderTargetCube = Engine->SetClass<Graphics::RenderTargetCube>("render_target_cube", false);
				VRenderTargetCube.SetMethod("uptr@ get_target_buffer() const", &Graphics::RenderTargetCube::GetTargetBuffer);
				VRenderTargetCube.SetMethod("uptr@ get_depth_buffer() const", &Graphics::RenderTargetCube::GetDepthBuffer);
				VRenderTargetCube.SetMethod("uint32 get_width() const", &Graphics::RenderTargetCube::GetWidth);
				VRenderTargetCube.SetMethod("uint32 get_height() const", &Graphics::RenderTargetCube::GetHeight);
				VRenderTargetCube.SetMethod("uint32 get_target_count() const", &Graphics::RenderTargetCube::GetTargetCount);
				VRenderTargetCube.SetMethod("texture_2d@+ get_target_2d(uint32) const", &Graphics::RenderTargetCube::GetTarget2D);
				VRenderTargetCube.SetMethod("texture_cube@+ get_target_cube(uint32) const", &Graphics::RenderTargetCube::GetTargetCube);
				VRenderTargetCube.SetMethod("texture_2d@+ get_depth_stencil() const", &Graphics::RenderTargetCube::GetDepthStencil);
				VRenderTargetCube.SetMethod("const viewport& get_viewport() const", &Graphics::RenderTargetCube::GetViewport);
				VRenderTargetCube.SetMethod("texture_cube@+ get_target() const", &Graphics::RenderTargetCube::GetTarget);

				TypeClass VMultiRenderTargetCubeDesc = Engine->SetPod<Graphics::MultiRenderTargetCube::Desc>("multi_render_target_cube_desc");
				VMultiRenderTargetCubeDesc.SetProperty<Graphics::MultiRenderTargetCube::Desc>("cpu_access access_flags", &Graphics::MultiRenderTargetCube::Desc::AccessFlags);
				VMultiRenderTargetCubeDesc.SetProperty<Graphics::MultiRenderTargetCube::Desc>("surface_target target", &Graphics::MultiRenderTargetCube::Desc::Target);
				VMultiRenderTargetCubeDesc.SetProperty<Graphics::MultiRenderTargetCube::Desc>("resource_usage usage", &Graphics::MultiRenderTargetCube::Desc::Usage);
				VMultiRenderTargetCubeDesc.SetProperty<Graphics::MultiRenderTargetCube::Desc>("resource_bind bind_flags", &Graphics::MultiRenderTargetCube::Desc::BindFlags);
				VMultiRenderTargetCubeDesc.SetProperty<Graphics::MultiRenderTargetCube::Desc>("resource_misc misc_flags", &Graphics::MultiRenderTargetCube::Desc::MiscFlags);
				VMultiRenderTargetCubeDesc.SetProperty<Graphics::MultiRenderTargetCube::Desc>("uint32 size", &Graphics::MultiRenderTargetCube::Desc::Size);
				VMultiRenderTargetCubeDesc.SetProperty<Graphics::MultiRenderTargetCube::Desc>("uint32 mip_levels", &Graphics::MultiRenderTargetCube::Desc::MipLevels);
				VMultiRenderTargetCubeDesc.SetProperty<Graphics::MultiRenderTargetCube::Desc>("bool depth_stencil", &Graphics::MultiRenderTargetCube::Desc::DepthStencil);
				VMultiRenderTargetCubeDesc.SetConstructor<Graphics::MultiRenderTargetCube::Desc>("void f()");
				VMultiRenderTargetCubeDesc.SetMethodEx("void set_format_mode(usize, surface_format)", &MultiRenderTargetCubeDescSetFormatMode);

				RefClass VMultiRenderTargetCube = Engine->SetClass<Graphics::MultiRenderTargetCube>("multi_render_target_cube", false);
				VMultiRenderTargetCube.SetMethod("uptr@ get_target_buffer() const", &Graphics::MultiRenderTargetCube::GetTargetBuffer);
				VMultiRenderTargetCube.SetMethod("uptr@ get_depth_buffer() const", &Graphics::MultiRenderTargetCube::GetDepthBuffer);
				VMultiRenderTargetCube.SetMethod("uint32 get_width() const", &Graphics::MultiRenderTargetCube::GetWidth);
				VMultiRenderTargetCube.SetMethod("uint32 get_height() const", &Graphics::MultiRenderTargetCube::GetHeight);
				VMultiRenderTargetCube.SetMethod("uint32 get_target_count() const", &Graphics::MultiRenderTargetCube::GetTargetCount);
				VMultiRenderTargetCube.SetMethod("texture_2d@+ get_target_2d(uint32) const", &Graphics::MultiRenderTargetCube::GetTarget2D);
				VMultiRenderTargetCube.SetMethod("texture_cube@+ get_target_cube(uint32) const", &Graphics::MultiRenderTargetCube::GetTargetCube);
				VMultiRenderTargetCube.SetMethod("texture_2d@+ get_depth_stencil() const", &Graphics::MultiRenderTargetCube::GetDepthStencil);
				VMultiRenderTargetCube.SetMethod("const viewport& get_viewport() const", &Graphics::MultiRenderTargetCube::GetViewport);
				VMultiRenderTargetCube.SetMethod("texture_cube@+ get_target(uint32) const", &Graphics::MultiRenderTargetCube::GetTarget);

				TypeClass VCubemapDesc = Engine->SetStruct<Graphics::Cubemap::Desc>("cubemap_desc");
				VCubemapDesc.SetProperty<Graphics::Cubemap::Desc>("render_target@ source", &Graphics::Cubemap::Desc::Source);
				VCubemapDesc.SetProperty<Graphics::Cubemap::Desc>("uint32 target", &Graphics::Cubemap::Desc::Target);
				VCubemapDesc.SetProperty<Graphics::Cubemap::Desc>("uint32 size", &Graphics::Cubemap::Desc::Size);
				VCubemapDesc.SetProperty<Graphics::Cubemap::Desc>("uint32 mip_levels", &Graphics::Cubemap::Desc::MipLevels);
				VCubemapDesc.SetConstructor<Graphics::Cubemap::Desc>("void f()");
				VCubemapDesc.SetOperatorCopyStatic(&CubemapDescCopy);
				VCubemapDesc.SetDestructorStatic("void f()", &CubemapDescDestructor);

				RefClass VCubemap = Engine->SetClass<Graphics::Cubemap>("cubemap", false);
				VCubemap.SetMethod("bool is_valid() const", &Graphics::Cubemap::IsValid);

				TypeClass VQueryDesc = Engine->SetPod<Graphics::Query::Desc>("visibility_query_desc");
				VQueryDesc.SetProperty<Graphics::Query::Desc>("bool predicate", &Graphics::Query::Desc::Predicate);
				VQueryDesc.SetProperty<Graphics::Query::Desc>("bool auto_pass", &Graphics::Query::Desc::AutoPass);
				VQueryDesc.SetConstructor<Graphics::Query::Desc>("void f()");

				RefClass VQuery = Engine->SetClass<Graphics::Query>("visibility_query", false);
				VQuery.SetMethod("uptr@ get_resource() const", &Graphics::Query::GetResource);

				TypeClass VGraphicsDeviceDesc = Engine->SetStruct<Graphics::GraphicsDevice::Desc>("graphics_device_desc");
				VGraphicsDeviceDesc.SetProperty<Graphics::GraphicsDevice::Desc>("render_backend backend", &Graphics::GraphicsDevice::Desc::Backend);
				VGraphicsDeviceDesc.SetProperty<Graphics::GraphicsDevice::Desc>("shader_model shader_mode", &Graphics::GraphicsDevice::Desc::ShaderMode);
				VGraphicsDeviceDesc.SetProperty<Graphics::GraphicsDevice::Desc>("surface_format buffer_format", &Graphics::GraphicsDevice::Desc::BufferFormat);
				VGraphicsDeviceDesc.SetProperty<Graphics::GraphicsDevice::Desc>("vsync vsync_mode", &Graphics::GraphicsDevice::Desc::VSyncMode);
				VGraphicsDeviceDesc.SetProperty<Graphics::GraphicsDevice::Desc>("string cache_directory", &Graphics::GraphicsDevice::Desc::CacheDirectory);
				VGraphicsDeviceDesc.SetProperty<Graphics::GraphicsDevice::Desc>("int32 is_windowed", &Graphics::GraphicsDevice::Desc::IsWindowed);
				VGraphicsDeviceDesc.SetProperty<Graphics::GraphicsDevice::Desc>("bool shader_cache", &Graphics::GraphicsDevice::Desc::ShaderCache);
				VGraphicsDeviceDesc.SetProperty<Graphics::GraphicsDevice::Desc>("bool debug", &Graphics::GraphicsDevice::Desc::Debug);
				VGraphicsDeviceDesc.SetProperty<Graphics::GraphicsDevice::Desc>("bool blit_rendering", &Graphics::GraphicsDevice::Desc::BlitRendering);
				VGraphicsDeviceDesc.SetProperty<Graphics::GraphicsDevice::Desc>("uint32 presentation_flags", &Graphics::GraphicsDevice::Desc::PresentationFlags);
				VGraphicsDeviceDesc.SetProperty<Graphics::GraphicsDevice::Desc>("uint32 compilation_flags", &Graphics::GraphicsDevice::Desc::CompilationFlags);
				VGraphicsDeviceDesc.SetProperty<Graphics::GraphicsDevice::Desc>("uint32 creation_flags", &Graphics::GraphicsDevice::Desc::CreationFlags);
				VGraphicsDeviceDesc.SetProperty<Graphics::GraphicsDevice::Desc>("uint32 buffer_width", &Graphics::GraphicsDevice::Desc::BufferWidth);
				VGraphicsDeviceDesc.SetProperty<Graphics::GraphicsDevice::Desc>("uint32 buffer_height", &Graphics::GraphicsDevice::Desc::BufferHeight);
				VGraphicsDeviceDesc.SetProperty<Graphics::GraphicsDevice::Desc>("activity@ window", &Graphics::GraphicsDevice::Desc::Window);
				VGraphicsDeviceDesc.SetConstructor<Graphics::GraphicsDevice::Desc>("void f()");
				VGraphicsDeviceDesc.SetOperatorCopyStatic(&GraphicsDeviceDescCopy);
				VGraphicsDeviceDesc.SetDestructorStatic("void f()", &GraphicsDeviceDescDestructor);

				VGraphicsDevice.SetMethod("void set_as_current_device()", &Graphics::GraphicsDevice::SetAsCurrentDevice);
				VGraphicsDevice.SetMethod("void set_shader_model(shader_model)", &Graphics::GraphicsDevice::SetShaderModel);
				VGraphicsDevice.SetMethod("void set_blend_state(blend_state@+)", &Graphics::GraphicsDevice::SetBlendState);
				VGraphicsDevice.SetMethod("void set_rasterizer_state(rasterizer_state@+)", &Graphics::GraphicsDevice::SetRasterizerState);
				VGraphicsDevice.SetMethod("void set_depth_stencil_state(depth_stencil_state@+)", &Graphics::GraphicsDevice::SetDepthStencilState);
				VGraphicsDevice.SetMethod("void set_input_layout(input_layout@+)", &Graphics::GraphicsDevice::SetInputLayout);
				VGraphicsDevice.SetMethod("void set_shader(shader@+, uint32)", &Graphics::GraphicsDevice::SetShader);
				VGraphicsDevice.SetMethod("void set_sampler_state(sampler_state@+, uint32, uint32, uint32)", &Graphics::GraphicsDevice::SetSamplerState);
				VGraphicsDevice.SetMethod<Graphics::GraphicsDevice, void, Graphics::Shader*, uint32_t, uint32_t>("void set_buffer(shader@+, uint32, uint32)", &Graphics::GraphicsDevice::SetBuffer);
				VGraphicsDevice.SetMethod<Graphics::GraphicsDevice, void, Graphics::InstanceBuffer*, uint32_t, uint32_t>("void set_buffer(instance_buffer@+, uint32, uint32)", &Graphics::GraphicsDevice::SetBuffer);
				VGraphicsDevice.SetMethod("void set_constant_buffer(element_buffer@+, uint32, uint32)", &Graphics::GraphicsDevice::SetConstantBuffer);
				VGraphicsDevice.SetMethod("void set_structure_buffer(element_buffer@+, uint32, uint32)", &Graphics::GraphicsDevice::SetStructureBuffer);
				VGraphicsDevice.SetMethod("void set_texture_2d(texture_2d@+, uint32, uint32)", &Graphics::GraphicsDevice::SetTexture2D);
				VGraphicsDevice.SetMethod("void set_texture_3d(texture_3d@+, uint32, uint32)", &Graphics::GraphicsDevice::SetTexture3D);
				VGraphicsDevice.SetMethod("void set_texture_cube(texture_cube@+, uint32, uint32)", &Graphics::GraphicsDevice::SetTextureCube);
				VGraphicsDevice.SetMethod("void set_index_buffer(element_buffer@+, surface_format)", &Graphics::GraphicsDevice::SetIndexBuffer);
				VGraphicsDevice.SetMethodEx("void set_vertex_buffers(array<element_buffer@>@+, bool = false)", &GraphicsDeviceSetVertexBuffers);
				VGraphicsDevice.SetMethodEx("void set_writeable(array<element_buffer@>@+, uint32, uint32, bool)", &GraphicsDeviceSetWriteable1);
				VGraphicsDevice.SetMethodEx("void set_writeable(array<texture_2d@>@+, uint32, uint32, bool)", &GraphicsDeviceSetWriteable2);
				VGraphicsDevice.SetMethodEx("void set_writeable(array<texture_3d@>@+, uint32, uint32, bool)", &GraphicsDeviceSetWriteable3);
				VGraphicsDevice.SetMethodEx("void set_writeable(array<texture_cube@>@+, uint32, uint32, bool)", &GraphicsDeviceSetWriteable4);
				VGraphicsDevice.SetMethod<Graphics::GraphicsDevice, void, float, float, float>("void set_target(float, float, float)", &Graphics::GraphicsDevice::SetTarget);
				VGraphicsDevice.SetMethod<Graphics::GraphicsDevice, void>("void set_target()", &Graphics::GraphicsDevice::SetTarget);
				VGraphicsDevice.SetMethod<Graphics::GraphicsDevice, void, Graphics::DepthTarget2D*>("void set_target(depth_target_2d@+)", &Graphics::GraphicsDevice::SetTarget);
				VGraphicsDevice.SetMethod<Graphics::GraphicsDevice, void, Graphics::DepthTargetCube*>("void set_target(depth_target_cube@+)", &Graphics::GraphicsDevice::SetTarget);
				VGraphicsDevice.SetMethod<Graphics::GraphicsDevice, void, Graphics::RenderTarget*, uint32_t, float, float, float>("void set_target(render_target@+, uint32, float, float, float)", &Graphics::GraphicsDevice::SetTarget);
				VGraphicsDevice.SetMethod<Graphics::GraphicsDevice, void, Graphics::RenderTarget*, uint32_t>("void set_target(render_target@+, uint32)", &Graphics::GraphicsDevice::SetTarget);
				VGraphicsDevice.SetMethod<Graphics::GraphicsDevice, void, Graphics::RenderTarget*, float, float, float>("void set_target(render_target@+, float, float, float)", &Graphics::GraphicsDevice::SetTarget);
				VGraphicsDevice.SetMethod<Graphics::GraphicsDevice, void, Graphics::RenderTarget*>("void set_target(render_target@+)", &Graphics::GraphicsDevice::SetTarget);
				VGraphicsDevice.SetMethodEx("void set_target_map(render_target@+, array<bool>@+)", &GraphicsDeviceSetTargetMap);
				VGraphicsDevice.SetMethod("void set_target_rect(uint32, uint32)", &Graphics::GraphicsDevice::SetTargetRect);
				VGraphicsDevice.SetMethodEx("void set_viewports(array<viewport>@+)", &GraphicsDeviceSetViewports);
				VGraphicsDevice.SetMethodEx("void set_scissor_rects(array<rectangle>@+)", &GraphicsDeviceSetScissorRects);
				VGraphicsDevice.SetMethod("void set_primitive_topology(primitive_topology)", &Graphics::GraphicsDevice::SetPrimitiveTopology);
				VGraphicsDevice.SetMethod("void flush_texture(uint32, uint32, uint32)", &Graphics::GraphicsDevice::FlushTexture);
				VGraphicsDevice.SetMethod("void flush_state()", &Graphics::GraphicsDevice::FlushState);
				VGraphicsDevice.SetMethod<Graphics::GraphicsDevice, bool, Graphics::ElementBuffer*, Graphics::ResourceMap, Graphics::MappedSubresource*>("bool dictionary(element_buffer@+, resource_map, mapped_subresource &out)", &Graphics::GraphicsDevice::Map);
				VGraphicsDevice.SetMethod<Graphics::GraphicsDevice, bool, Graphics::Texture2D*, Graphics::ResourceMap, Graphics::MappedSubresource*>("bool dictionary(texture_2d@+, resource_map, mapped_subresource &out)", &Graphics::GraphicsDevice::Map);
				VGraphicsDevice.SetMethod<Graphics::GraphicsDevice, bool, Graphics::Texture3D*, Graphics::ResourceMap, Graphics::MappedSubresource*>("bool dictionary(texture_3d@+, resource_map, mapped_subresource &out)", &Graphics::GraphicsDevice::Map);
				VGraphicsDevice.SetMethod<Graphics::GraphicsDevice, bool, Graphics::TextureCube*, Graphics::ResourceMap, Graphics::MappedSubresource*>("bool dictionary(texture_cube@+, resource_map, mapped_subresource &out)", &Graphics::GraphicsDevice::Map);
				VGraphicsDevice.SetMethod<Graphics::GraphicsDevice, bool, Graphics::Texture2D*, Graphics::MappedSubresource*>("bool unmap(texture_2d@+, mapped_subresource &in)", &Graphics::GraphicsDevice::Unmap);
				VGraphicsDevice.SetMethod<Graphics::GraphicsDevice, bool, Graphics::Texture3D*, Graphics::MappedSubresource*>("bool unmap(texture_3d@+, mapped_subresource &in)", &Graphics::GraphicsDevice::Unmap);
				VGraphicsDevice.SetMethod<Graphics::GraphicsDevice, bool, Graphics::TextureCube*, Graphics::MappedSubresource*>("bool unmap(texture_cube@+, mapped_subresource &in)", &Graphics::GraphicsDevice::Unmap);
				VGraphicsDevice.SetMethod<Graphics::GraphicsDevice, bool, Graphics::ElementBuffer*, Graphics::MappedSubresource*>("bool unmap(element_buffer@+, mapped_subresource &in)", &Graphics::GraphicsDevice::Unmap);
				VGraphicsDevice.SetMethod("bool update_constant_buffer(element_buffer@+, uptr@, usize)", &Graphics::GraphicsDevice::UpdateConstantBuffer);
				VGraphicsDevice.SetMethod<Graphics::GraphicsDevice, bool, Graphics::ElementBuffer*, void*, size_t>("bool update_buffer(element_buffer@+, uptr@, usize)", &Graphics::GraphicsDevice::UpdateBuffer);
				VGraphicsDevice.SetMethod<Graphics::GraphicsDevice, bool, Graphics::Shader*, const void*>("bool update_buffer(shader@+, uptr@)", &Graphics::GraphicsDevice::UpdateBuffer);
				VGraphicsDevice.SetMethod<Graphics::GraphicsDevice, bool, Graphics::MeshBuffer*, Compute::Vertex*>("bool update_buffer(mesh_buffer@+, uptr@)", &Graphics::GraphicsDevice::UpdateBuffer);
				VGraphicsDevice.SetMethod<Graphics::GraphicsDevice, bool, Graphics::SkinMeshBuffer*, Compute::SkinVertex*>("bool update_buffer(skin_mesh_buffer@+, uptr@)", &Graphics::GraphicsDevice::UpdateBuffer);
				VGraphicsDevice.SetMethod<Graphics::GraphicsDevice, bool, Graphics::InstanceBuffer*>("bool update_buffer(instance_buffer@+)", &Graphics::GraphicsDevice::UpdateBuffer);
				VGraphicsDevice.SetMethod<Graphics::GraphicsDevice, bool, Graphics::Shader*, size_t>("bool update_buffer_size(shader@+, usize)", &Graphics::GraphicsDevice::UpdateBufferSize);
				VGraphicsDevice.SetMethod<Graphics::GraphicsDevice, bool, Graphics::InstanceBuffer*, size_t>("bool update_buffer_size(instance_buffer@+, usize)", &Graphics::GraphicsDevice::UpdateBufferSize);
				VGraphicsDevice.SetMethod("void clear_buffer(instance_buffer@+)", &Graphics::GraphicsDevice::ClearBuffer);
				VGraphicsDevice.SetMethod<Graphics::GraphicsDevice, void, Graphics::Texture2D*>("void clear_writable(texture_2d@+)", &Graphics::GraphicsDevice::ClearWritable);
				VGraphicsDevice.SetMethod<Graphics::GraphicsDevice, void, Graphics::Texture2D*, float, float, float>("void clear_writable(texture_2d@+, float, float, float)", &Graphics::GraphicsDevice::ClearWritable);
				VGraphicsDevice.SetMethod<Graphics::GraphicsDevice, void, Graphics::Texture3D*>("void clear_writable(texture_3d@+)", &Graphics::GraphicsDevice::ClearWritable);
				VGraphicsDevice.SetMethod<Graphics::GraphicsDevice, void, Graphics::Texture3D*, float, float, float>("void clear_writable(texture_3d@+, float, float, float)", &Graphics::GraphicsDevice::ClearWritable);
				VGraphicsDevice.SetMethod<Graphics::GraphicsDevice, void, Graphics::TextureCube*>("void clear_writable(texture_cube@+)", &Graphics::GraphicsDevice::ClearWritable);
				VGraphicsDevice.SetMethod<Graphics::GraphicsDevice, void, Graphics::TextureCube*, float, float, float>("void clear_writable(texture_cube@+, float, float, float)", &Graphics::GraphicsDevice::ClearWritable);
				VGraphicsDevice.SetMethod<Graphics::GraphicsDevice, void, float, float, float>("void clear(float, float, float)", &Graphics::GraphicsDevice::Clear);
				VGraphicsDevice.SetMethod<Graphics::GraphicsDevice, void, Graphics::RenderTarget*, uint32_t, float, float, float>("void clear(render_target@+, uint32, float, float, float)", &Graphics::GraphicsDevice::Clear);
				VGraphicsDevice.SetMethod<Graphics::GraphicsDevice, void>("void clear_depth()", &Graphics::GraphicsDevice::ClearDepth);
				VGraphicsDevice.SetMethod<Graphics::GraphicsDevice, void, Graphics::DepthTarget2D*>("void clear_depth(depth_target_2d@+)", &Graphics::GraphicsDevice::ClearDepth);
				VGraphicsDevice.SetMethod<Graphics::GraphicsDevice, void, Graphics::DepthTargetCube*>("void clear_depth(depth_target_cube@+)", &Graphics::GraphicsDevice::ClearDepth);
				VGraphicsDevice.SetMethod<Graphics::GraphicsDevice, void, Graphics::RenderTarget*>("void clear_depth(render_target@+)", &Graphics::GraphicsDevice::ClearDepth);
				VGraphicsDevice.SetMethod<Graphics::GraphicsDevice, void, uint32_t, uint32_t, uint32_t>("void draw_indexed(uint32, uint32, uint32)", &Graphics::GraphicsDevice::DrawIndexed);
				VGraphicsDevice.SetMethod<Graphics::GraphicsDevice, void, Graphics::MeshBuffer*>("void draw_indexed(mesh_buffer@+)", &Graphics::GraphicsDevice::DrawIndexed);
				VGraphicsDevice.SetMethod<Graphics::GraphicsDevice, void, Graphics::SkinMeshBuffer*>("void draw_indexed(skin_mesh_buffer@+)", &Graphics::GraphicsDevice::DrawIndexed);
				VGraphicsDevice.SetMethod<Graphics::GraphicsDevice, void, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t>("void draw_indexed_instanced(uint32, uint32, uint32, uint32, uint32)", &Graphics::GraphicsDevice::DrawIndexedInstanced);
				VGraphicsDevice.SetMethod<Graphics::GraphicsDevice, void, Graphics::ElementBuffer*, Graphics::MeshBuffer*, uint32_t>("void draw_indexed_instanced(element_buffer@+, mesh_buffer@+, uint32)", &Graphics::GraphicsDevice::DrawIndexedInstanced);
				VGraphicsDevice.SetMethod<Graphics::GraphicsDevice, void, Graphics::ElementBuffer*, Graphics::SkinMeshBuffer*, uint32_t>("void draw_indexed_instanced(element_buffer@+, skin_mesh_buffer@+, uint32)", &Graphics::GraphicsDevice::DrawIndexedInstanced);
				VGraphicsDevice.SetMethod("void draw(uint32, uint32)", &Graphics::GraphicsDevice::Draw);
				VGraphicsDevice.SetMethod("void draw_instanced(uint32, uint32, uint32, uint32)", &Graphics::GraphicsDevice::DrawInstanced);
				VGraphicsDevice.SetMethod("void dispatch(uint32, uint32, uint32)", &Graphics::GraphicsDevice::Dispatch);
				VGraphicsDevice.SetMethodEx("texture_2d@ copy_texture_2d(texture_2d@+)", &GraphicsDeviceCopyTexture2D1);
				VGraphicsDevice.SetMethodEx("texture_2d@ copy_texture_2d(render_target@+, uint32)", &GraphicsDeviceCopyTexture2D2);
				VGraphicsDevice.SetMethodEx("texture_2d@ copy_texture_2d(render_target_cube@+, cube_face)", &GraphicsDeviceCopyTexture2D3);
				VGraphicsDevice.SetMethodEx("texture_2d@ copy_texture_2d(multi_render_target_cube@+, uint32, cube_face)", &GraphicsDeviceCopyTexture2D4);
				VGraphicsDevice.SetMethodEx("texture_cube@ copy_texture_cube(render_target_cube@+)", &GraphicsDeviceCopyTextureCube1);
				VGraphicsDevice.SetMethodEx("texture_cube@ copy_texture_cube(multi_render_target_cube@+, uint32)", &GraphicsDeviceCopyTextureCube2);
				VGraphicsDevice.SetMethod("bool copy_target(render_target@+, uint32, render_target@+, uint32)", &Graphics::GraphicsDevice::CopyTarget);
				VGraphicsDevice.SetMethodEx("texture_2d@ copy_back_buffer()", &GraphicsDeviceCopyBackBuffer);
				VGraphicsDevice.SetMethodEx("texture_2d@ copy_back_buffer_msaa()", &GraphicsDeviceCopyBackBufferMSAA);
				VGraphicsDevice.SetMethodEx("texture_2d@ copy_back_buffer_noaa()", &GraphicsDeviceCopyBackBufferNoAA);
				VGraphicsDevice.SetMethod("bool cubemap_push(cubemap@+, texture_cube@+)", &Graphics::GraphicsDevice::CubemapPush);
				VGraphicsDevice.SetMethod("bool cubemap_face(cubemap@+, cube_face)", &Graphics::GraphicsDevice::CubemapFace);
				VGraphicsDevice.SetMethod("bool cubemap_pop(cubemap@+)", &Graphics::GraphicsDevice::CubemapPop);
				VGraphicsDevice.SetMethodEx("array<viewport>@ get_viewports()", &GraphicsDeviceGetViewports);
				VGraphicsDevice.SetMethodEx("array<rectangle>@ get_scissor_rects()", &GraphicsDeviceGetScissorRects);
				VGraphicsDevice.SetMethod("bool resize_buffers(uint32, uint32)", &Graphics::GraphicsDevice::ResizeBuffers);
				VGraphicsDevice.SetMethod<Graphics::GraphicsDevice, bool, Graphics::Texture2D*>("bool generate_texture(texture_2d@+)", &Graphics::GraphicsDevice::GenerateTexture);
				VGraphicsDevice.SetMethod<Graphics::GraphicsDevice, bool, Graphics::Texture3D*>("bool generate_texture(texture_3d@+)", &Graphics::GraphicsDevice::GenerateTexture);
				VGraphicsDevice.SetMethod<Graphics::GraphicsDevice, bool, Graphics::TextureCube*>("bool generate_texture(texture_cube@+)", &Graphics::GraphicsDevice::GenerateTexture);
				VGraphicsDevice.SetMethod<Graphics::GraphicsDevice, bool, Graphics::Query*, size_t*, bool>("bool get_query_data(visibility_query@+, usize &out, bool = true)", &Graphics::GraphicsDevice::GetQueryData);
				VGraphicsDevice.SetMethod<Graphics::GraphicsDevice, bool, Graphics::Query*, bool*, bool>("bool get_query_data(visibility_query@+, bool &out, bool = true)", &Graphics::GraphicsDevice::GetQueryData);
				VGraphicsDevice.SetMethod("void query_begin(visibility_query@+)", &Graphics::GraphicsDevice::QueryBegin);
				VGraphicsDevice.SetMethod("void query_end(visibility_query@+)", &Graphics::GraphicsDevice::QueryEnd);
				VGraphicsDevice.SetMethod<Graphics::GraphicsDevice, void, Graphics::Texture2D*>("void generate_mips(texture_2d@+)", &Graphics::GraphicsDevice::GenerateMips);
				VGraphicsDevice.SetMethod<Graphics::GraphicsDevice, void, Graphics::Texture3D*>("void generate_mips(texture_3d@+)", &Graphics::GraphicsDevice::GenerateMips);
				VGraphicsDevice.SetMethod<Graphics::GraphicsDevice, void, Graphics::TextureCube*>("void generate_mips(texture_cube@+)", &Graphics::GraphicsDevice::GenerateMips);
				VGraphicsDevice.SetMethod("bool im_begin()", &Graphics::GraphicsDevice::ImBegin);
				VGraphicsDevice.SetMethod("void im_transform(const matrix4x4 &in)", &Graphics::GraphicsDevice::ImTransform);
				VGraphicsDevice.SetMethod("void im_topology(primitive_topology)", &Graphics::GraphicsDevice::ImTopology);
				VGraphicsDevice.SetMethod("void im_emit()", &Graphics::GraphicsDevice::ImEmit);
				VGraphicsDevice.SetMethod("void im_texture(texture_2d@+)", &Graphics::GraphicsDevice::ImTexture);
				VGraphicsDevice.SetMethod("void im_color(float, float, float, float)", &Graphics::GraphicsDevice::ImColor);
				VGraphicsDevice.SetMethod("void im_intensity(float)", &Graphics::GraphicsDevice::ImIntensity);
				VGraphicsDevice.SetMethod("void im_texcoord(float, float)", &Graphics::GraphicsDevice::ImTexCoord);
				VGraphicsDevice.SetMethod("void im_texcoord_offset(float, float)", &Graphics::GraphicsDevice::ImTexCoordOffset);
				VGraphicsDevice.SetMethod("void im_position(float, float, float)", &Graphics::GraphicsDevice::ImPosition);
				VGraphicsDevice.SetMethod("bool im_end()", &Graphics::GraphicsDevice::ImEnd);
				VGraphicsDevice.SetMethod("bool submit()", &Graphics::GraphicsDevice::Submit);
				VGraphicsDevice.SetMethod("depth_stencil_state@ create_depth_stencil_state(const depth_stencil_state_desc &in)", &Graphics::GraphicsDevice::CreateDepthStencilState);
				VGraphicsDevice.SetMethod("blend_state@ create_blend_state(const blend_state_desc &in)", &Graphics::GraphicsDevice::CreateBlendState);
				VGraphicsDevice.SetMethod("rasterizer_state@ create_rasterizer_state(const rasterizer_state_desc &in)", &Graphics::GraphicsDevice::CreateRasterizerState);
				VGraphicsDevice.SetMethod("sampler_state@ create_sampler_state(const sampler_state_desc &in)", &Graphics::GraphicsDevice::CreateSamplerState);
				VGraphicsDevice.SetMethod("input_layout@ create_input_layout(const input_layout_desc &in)", &Graphics::GraphicsDevice::CreateInputLayout);
				VGraphicsDevice.SetMethod("shader@ create_shader(const shader_desc &in)", &Graphics::GraphicsDevice::CreateShader);
				VGraphicsDevice.SetMethod("element_buffer@ create_element_buffer(const element_buffer_desc &in)", &Graphics::GraphicsDevice::CreateElementBuffer);
				VGraphicsDevice.SetMethodEx("mesh_buffer@ create_mesh_buffer(const mesh_buffer_desc &in)", &GraphicsDeviceCreateMeshBuffer1);
				VGraphicsDevice.SetMethodEx("mesh_buffer@ create_mesh_buffer(element_buffer@+, element_buffer@+)", &GraphicsDeviceCreateMeshBuffer2);
				VGraphicsDevice.SetMethodEx("skin_mesh_buffer@ create_skin_mesh_buffer(const skin_mesh_buffer_desc &in)", &GraphicsDeviceCreateSkinMeshBuffer1);
				VGraphicsDevice.SetMethodEx("skin_mesh_buffer@ create_skin_mesh_buffer(element_buffer@+, element_buffer@+)", &GraphicsDeviceCreateSkinMeshBuffer2);
				VGraphicsDevice.SetMethodEx("instance_buffer@ create_instance_buffer(const instance_buffer_desc &in)", &GraphicsDeviceCreateInstanceBuffer);
				VGraphicsDevice.SetMethod<Graphics::GraphicsDevice, Graphics::Texture2D*>("texture_2d@ create_texture_2d()", &Graphics::GraphicsDevice::CreateTexture2D);
				VGraphicsDevice.SetMethod<Graphics::GraphicsDevice, Graphics::Texture2D*, const Graphics::Texture2D::Desc&>("texture_2d@ create_texture_2d(const texture_2d_desc &in)", &Graphics::GraphicsDevice::CreateTexture2D);
				VGraphicsDevice.SetMethod<Graphics::GraphicsDevice, Graphics::Texture3D*>("texture_3d@ create_texture_3d()", &Graphics::GraphicsDevice::CreateTexture3D);
				VGraphicsDevice.SetMethod<Graphics::GraphicsDevice, Graphics::Texture3D*, const Graphics::Texture3D::Desc&>("texture_3d@ create_texture_3d(const texture_3d_desc &in)", &Graphics::GraphicsDevice::CreateTexture3D);
				VGraphicsDevice.SetMethod<Graphics::GraphicsDevice, Graphics::TextureCube*>("texture_cube@ create_texture_cube()", &Graphics::GraphicsDevice::CreateTextureCube);
				VGraphicsDevice.SetMethod<Graphics::GraphicsDevice, Graphics::TextureCube*, const Graphics::TextureCube::Desc&>("texture_cube@ create_texture_cube(const texture_cube_desc &in)", &Graphics::GraphicsDevice::CreateTextureCube);
				VGraphicsDevice.SetMethodEx("texture_cube@ create_texture_cube(array<texture_2d@>@+)", &GraphicsDeviceCreateTextureCube);
				VGraphicsDevice.SetMethod<Graphics::GraphicsDevice, Graphics::TextureCube*, Graphics::Texture2D*>("texture_cube@ create_texture_cube(texture_2d@+)", &Graphics::GraphicsDevice::CreateTextureCube);
				VGraphicsDevice.SetMethod("depth_target_2d@ create_depth_target_2d(const depth_target_2d_desc &in)", &Graphics::GraphicsDevice::CreateDepthTarget2D);
				VGraphicsDevice.SetMethod("depth_target_cube@ create_depth_target_cube(const depth_target_cube_desc &in)", &Graphics::GraphicsDevice::CreateDepthTargetCube);
				VGraphicsDevice.SetMethod("render_target_2d@ create_render_target_2d(const render_target_2d_desc &in)", &Graphics::GraphicsDevice::CreateRenderTarget2D);
				VGraphicsDevice.SetMethod("multi_render_target_2d@ create_multi_render_target_2d(const multi_render_target_2d_desc &in)", &Graphics::GraphicsDevice::CreateMultiRenderTarget2D);
				VGraphicsDevice.SetMethod("render_target_cube@ create_render_target_cube(const render_target_cube_desc &in)", &Graphics::GraphicsDevice::CreateRenderTargetCube);
				VGraphicsDevice.SetMethod("multi_render_target_cube@ create_multi_render_target_cube(const multi_render_target_cube_desc &in)", &Graphics::GraphicsDevice::CreateMultiRenderTargetCube);
				VGraphicsDevice.SetMethod("cubemap@ create_cubemap(const cubemap_desc &in)", &Graphics::GraphicsDevice::CreateCubemap);
				VGraphicsDevice.SetMethod("visibility_query@ create_query(const visibility_query_desc &in)", &Graphics::GraphicsDevice::CreateQuery);
				VGraphicsDevice.SetMethod("activity_surface@ create_surface(texture_2d@+)", &Graphics::GraphicsDevice::CreateSurface);
				VGraphicsDevice.SetMethod("primitive_topology get_primitive_topology() const", &Graphics::GraphicsDevice::GetPrimitiveTopology);
				VGraphicsDevice.SetMethod("shader_model get_supported_shader_model()  const", &Graphics::GraphicsDevice::GetSupportedShaderModel);
				VGraphicsDevice.SetMethod("uptr@ get_device() const", &Graphics::GraphicsDevice::GetDevice);
				VGraphicsDevice.SetMethod("uptr@ get_context() const", &Graphics::GraphicsDevice::GetContext);
				VGraphicsDevice.SetMethod("bool is_valid() const", &Graphics::GraphicsDevice::IsValid);
				VGraphicsDevice.SetMethod("void set_vertex_buffer(element_buffer@+)", &Graphics::GraphicsDevice::SetVertexBuffer);
				VGraphicsDevice.SetMethod("void set_shader_cache(bool)", &Graphics::GraphicsDevice::SetShaderCache);
				VGraphicsDevice.SetMethod("void set_vsync_mode(vsync)", &Graphics::GraphicsDevice::SetVSyncMode);
				VGraphicsDevice.SetMethod("bool preprocess(shader_desc &in)", &Graphics::GraphicsDevice::Preprocess);
				VGraphicsDevice.SetMethod("bool transpile(string &out, shader_type, shader_lang)", &Graphics::GraphicsDevice::Transpile);
				VGraphicsDevice.SetMethod("bool add_section(const string &in, const string &in)", &Graphics::GraphicsDevice::AddSection);
				VGraphicsDevice.SetMethod("bool remove_section(const string &in)", &Graphics::GraphicsDevice::RemoveSection);
				VGraphicsDevice.SetMethod<Graphics::GraphicsDevice, bool, const Core::String&, Graphics::Shader::Desc*>("bool get_section(const string &in, shader_desc &out)", &Graphics::GraphicsDevice::GetSection);
				VGraphicsDevice.SetMethod("bool is_left_handed() const", &Graphics::GraphicsDevice::IsLeftHanded);
				VGraphicsDevice.SetMethod("string get_shader_main(shader_type) const", &Graphics::GraphicsDevice::GetShaderMain);
				VGraphicsDevice.SetMethod("depth_stencil_state@+ get_depth_stencil_state(const string &in)", &Graphics::GraphicsDevice::GetDepthStencilState);
				VGraphicsDevice.SetMethod("blend_state@+ get_blend_state(const string &in)", &Graphics::GraphicsDevice::GetBlendState);
				VGraphicsDevice.SetMethod("rasterizer_state@+ get_rasterizer_state(const string &in)", &Graphics::GraphicsDevice::GetRasterizerState);
				VGraphicsDevice.SetMethod("sampler_state@+ get_sampler_state(const string &in)", &Graphics::GraphicsDevice::GetSamplerState);
				VGraphicsDevice.SetMethod("input_layout@+ get_input_layout(const string &in)", &Graphics::GraphicsDevice::GetInputLayout);
				VGraphicsDevice.SetMethod("shader_model get_shader_model() const", &Graphics::GraphicsDevice::GetShaderModel);
				VGraphicsDevice.SetMethod("render_target_2d@+ get_render_target()", &Graphics::GraphicsDevice::GetRenderTarget);
				VGraphicsDevice.SetMethod("render_backend get_backend() const", &Graphics::GraphicsDevice::GetBackend);
				VGraphicsDevice.SetMethod("uint32 get_format_size(surface_format) const", &Graphics::GraphicsDevice::GetFormatSize);
				VGraphicsDevice.SetMethod("uint32 get_present_flags() const", &Graphics::GraphicsDevice::GetPresentFlags);
				VGraphicsDevice.SetMethod("uint32 get_compile_flags() const", &Graphics::GraphicsDevice::GetCompileFlags);
				VGraphicsDevice.SetMethod("uint32 get_mip_level(uint32, uint32) const", &Graphics::GraphicsDevice::GetMipLevel);
				VGraphicsDevice.SetMethod("vsync get_vsync_mode() const", &Graphics::GraphicsDevice::GetVSyncMode);
				VGraphicsDevice.SetMethod("bool is_debug() const", &Graphics::GraphicsDevice::IsDebug);
				VGraphicsDevice.SetMethodStatic("graphics_device@ create(graphics_device_desc &in)", &GraphicsDeviceCreate);
				VGraphicsDevice.SetMethodStatic("void compile_buildin_shaders(array<graphics_device@>@+)", &GraphicsDeviceCompileBuiltinShaders);
				VGraphicsDevice.SetEnumRefsEx<Graphics::GraphicsDevice>([](Graphics::GraphicsDevice* Base, asIScriptEngine* Engine)
				{
					Engine->GCEnumCallback(Base->GetRenderTarget());
					for (auto& Item : Base->GetDepthStencilStates())
						Engine->GCEnumCallback(Item.second);

					for (auto& Item : Base->GetRasterizerStates())
						Engine->GCEnumCallback(Item.second);

					for (auto& Item : Base->GetBlendStates())
						Engine->GCEnumCallback(Item.second);

					for (auto& Item : Base->GetSamplerStates())
						Engine->GCEnumCallback(Item.second);

					for (auto& Item : Base->GetInputLayouts())
						Engine->GCEnumCallback(Item.second);
				});
				VGraphicsDevice.SetReleaseRefsEx<Graphics::GraphicsDevice>([](Graphics::GraphicsDevice* Base, asIScriptEngine*) { });

				VRenderTarget.SetDynamicCast<Graphics::RenderTarget, Graphics::RenderTarget2D>("render_target_2d@+");
				VRenderTarget.SetDynamicCast<Graphics::RenderTarget, Graphics::RenderTargetCube>("render_target_cube@+");
				VRenderTarget.SetDynamicCast<Graphics::RenderTarget, Graphics::MultiRenderTarget2D>("multi_render_target_2d@+");
				VRenderTarget.SetDynamicCast<Graphics::RenderTarget, Graphics::MultiRenderTargetCube>("multi_render_target_cube@+");
				VRenderTarget2D.SetDynamicCast<Graphics::RenderTarget2D, Graphics::RenderTarget>("render_target@+", true);
				VMultiRenderTarget2D.SetDynamicCast<Graphics::MultiRenderTarget2D, Graphics::RenderTarget>("render_target@+", true);
				VRenderTargetCube.SetDynamicCast<Graphics::RenderTargetCube, Graphics::RenderTarget>("render_target@+", true);
				VMultiRenderTargetCube.SetDynamicCast<Graphics::MultiRenderTargetCube, Graphics::RenderTarget>("render_target@+", true);

				return true;
#else
				VI_ASSERT(false, false, "<graphics> is not loaded");
				return false;
#endif
			}
			bool Registry::LoadNetwork(VirtualMachine* Engine)
			{
#ifdef VI_HAS_BINDINGS
				VI_ASSERT(Engine != nullptr, false, "manager should be set");
				VI_TYPEREF(SocketListener, "socket_listener");
				VI_TYPEREF(SocketConnection, "socket_connection");
				VI_TYPEREF(SocketServer, "socket_server");

				Enumeration VSecure = Engine->SetEnum("socket_secure");
				VSecure.SetValue("unsecure", (int)Network::Secure::Any);
				VSecure.SetValue("ssl_v2", (int)Network::Secure::SSL_V2);
				VSecure.SetValue("ssl_v3", (int)Network::Secure::SSL_V3);
				VSecure.SetValue("tls_v1", (int)Network::Secure::TLS_V1);
				VSecure.SetValue("tls_v1_1", (int)Network::Secure::TLS_V1_1);

				Enumeration VServerState = Engine->SetEnum("server_state");
				VServerState.SetValue("working", (int)Network::ServerState::Working);
				VServerState.SetValue("stopping", (int)Network::ServerState::Stopping);
				VServerState.SetValue("idle", (int)Network::ServerState::Idle);

				Enumeration VSocketPoll = Engine->SetEnum("socket_poll");
				VSocketPoll.SetValue("next", (int)Network::SocketPoll::Next);
				VSocketPoll.SetValue("reset", (int)Network::SocketPoll::Reset);
				VSocketPoll.SetValue("timeout", (int)Network::SocketPoll::Timeout);
				VSocketPoll.SetValue("cancel", (int)Network::SocketPoll::Cancel);
				VSocketPoll.SetValue("finish", (int)Network::SocketPoll::Finish);
				VSocketPoll.SetValue("finish_sync", (int)Network::SocketPoll::FinishSync);

				Enumeration VSocketProtocol = Engine->SetEnum("socket_protocol");
				VSocketProtocol.SetValue("ip", (int)Network::SocketProtocol::IP);
				VSocketProtocol.SetValue("icmp", (int)Network::SocketProtocol::ICMP);
				VSocketProtocol.SetValue("tcp", (int)Network::SocketProtocol::TCP);
				VSocketProtocol.SetValue("udp", (int)Network::SocketProtocol::UDP);
				VSocketProtocol.SetValue("raw", (int)Network::SocketProtocol::RAW);

				Enumeration VSocketType = Engine->SetEnum("socket_type");
				VSocketType.SetValue("stream", (int)Network::SocketType::Stream);
				VSocketType.SetValue("datagram", (int)Network::SocketType::Datagram);
				VSocketType.SetValue("raw", (int)Network::SocketType::Raw);
				VSocketType.SetValue("reliably_delivered_message", (int)Network::SocketType::Reliably_Delivered_Message);
				VSocketType.SetValue("sequence_packet_stream", (int)Network::SocketType::Sequence_Packet_Stream);

				Enumeration VDNSType = Engine->SetEnum("dns_type");
				VDNSType.SetValue("connect", (int)Network::DNSType::Connect);
				VDNSType.SetValue("listen", (int)Network::DNSType::Listen);

				TypeClass VRemoteHost = Engine->SetStructTrivial<Network::RemoteHost>("remote_host");
				VRemoteHost.SetProperty<Network::RemoteHost>("string hostname", &Network::RemoteHost::Hostname);
				VRemoteHost.SetProperty<Network::RemoteHost>("int32 port", &Network::RemoteHost::Port);
				VRemoteHost.SetProperty<Network::RemoteHost>("bool secure", &Network::RemoteHost::Secure);
				VRemoteHost.SetConstructor<Network::RemoteHost>("void f()");

				TypeClass VLocation = Engine->SetStructTrivial<Network::Location>("url_location");
				VLocation.SetProperty<Network::Location>("string url", &Network::Location::URL);
				VLocation.SetProperty<Network::Location>("string protocol", &Network::Location::Protocol);
				VLocation.SetProperty<Network::Location>("string username", &Network::Location::Username);
				VLocation.SetProperty<Network::Location>("string password", &Network::Location::Password);
				VLocation.SetProperty<Network::Location>("string hostname", &Network::Location::Hostname);
				VLocation.SetProperty<Network::Location>("string fragment", &Network::Location::Fragment);
				VLocation.SetProperty<Network::Location>("string path", &Network::Location::Path);
				VLocation.SetProperty<Network::Location>("int32 port", &Network::Location::Port);
				VLocation.SetConstructor<Network::Location, const Core::String&>("void f(const string &in)");
				VLocation.SetMethodEx("dictionary@ get_query() const", &LocationGetQuery);

				TypeClass VCertificate = Engine->SetStructTrivial<Network::Certificate>("certificate");
				VCertificate.SetProperty<Network::Certificate>("string subject", &Network::Certificate::Subject);
				VCertificate.SetProperty<Network::Certificate>("string issuer", &Network::Certificate::Issuer);
				VCertificate.SetProperty<Network::Certificate>("string serial", &Network::Certificate::Serial);
				VCertificate.SetProperty<Network::Certificate>("string finger", &Network::Certificate::Finger);
				VCertificate.SetConstructor<Network::Certificate>("void f()");

				TypeClass VSocketCertificate = Engine->SetStructTrivial<Network::SocketCertificate>("socket_certificate");
				VSocketCertificate.SetProperty<Network::SocketCertificate>("uptr@ context", &Network::SocketCertificate::Context);
				VSocketCertificate.SetProperty<Network::SocketCertificate>("string key", &Network::SocketCertificate::Key);
				VSocketCertificate.SetProperty<Network::SocketCertificate>("string chain", &Network::SocketCertificate::Chain);
				VSocketCertificate.SetProperty<Network::SocketCertificate>("string ciphers", &Network::SocketCertificate::Ciphers);
				VSocketCertificate.SetProperty<Network::SocketCertificate>("socket_secure protocol", &Network::SocketCertificate::Protocol);
				VSocketCertificate.SetProperty<Network::SocketCertificate>("bool verify_peers", &Network::SocketCertificate::VerifyPeers);
				VSocketCertificate.SetProperty<Network::SocketCertificate>("usize depth", &Network::SocketCertificate::Depth);
				VSocketCertificate.SetConstructor<Network::SocketCertificate>("void f()");

				TypeClass VDataFrame = Engine->SetStructTrivial<Network::DataFrame>("socket_data_frame");
				VDataFrame.SetProperty<Network::DataFrame>("string message", &Network::DataFrame::Message);
				VDataFrame.SetProperty<Network::DataFrame>("bool start", &Network::DataFrame::Start);
				VDataFrame.SetProperty<Network::DataFrame>("bool finish", &Network::DataFrame::Finish);
				VDataFrame.SetProperty<Network::DataFrame>("bool timeout", &Network::DataFrame::Timeout);
				VDataFrame.SetProperty<Network::DataFrame>("bool keep_alive", &Network::DataFrame::KeepAlive);
				VDataFrame.SetProperty<Network::DataFrame>("bool close", &Network::DataFrame::Close);
				VDataFrame.SetConstructor<Network::DataFrame>("void f()");

				RefClass VSocket = Engine->SetClass<Network::Socket>("socket", false);
				TypeClass VEpollFd = Engine->SetStruct<Network::EpollFd>("epoll_fd");
				VEpollFd.SetProperty<Network::EpollFd>("socket@ base", &Network::EpollFd::Base);
				VEpollFd.SetProperty<Network::EpollFd>("bool readable", &Network::EpollFd::Readable);
				VEpollFd.SetProperty<Network::EpollFd>("bool writeable", &Network::EpollFd::Writeable);
				VEpollFd.SetProperty<Network::EpollFd>("bool closed", &Network::EpollFd::Closed);
				VEpollFd.SetConstructor<Network::EpollFd>("void f()");
				VEpollFd.SetOperatorCopyStatic(&EpollFdCopy);
				VEpollFd.SetDestructorStatic("void f()", &EpollFdDestructor);

				TypeClass VEpollHandle = Engine->SetStructTrivial<Network::EpollHandle>("epoll_handle");
				VEpollHandle.SetProperty<Network::EpollHandle>("uptr@ array", &Network::EpollHandle::Array);
				VEpollHandle.SetProperty<Network::EpollHandle>("uptr@ handle", &Network::EpollHandle::Handle);
				VEpollHandle.SetProperty<Network::EpollHandle>("usize array_size", &Network::EpollHandle::ArraySize);
				VEpollHandle.SetConstructor<Network::EpollHandle, size_t>("void f(usize)");
				VEpollHandle.SetMethod("bool add(socket@+, bool, bool)", &Network::EpollHandle::Add);
				VEpollHandle.SetMethod("bool update(socket@+, bool, bool)", &Network::EpollHandle::Update);
				VEpollHandle.SetMethod("bool remove(socket@+, bool, bool)", &Network::EpollHandle::Remove);
				VEpollHandle.SetMethodEx("int wait(array<epoll_fd>@+, uint64)", &EpollHandleWait);

				RefClass VSocketAddress = Engine->SetClass<Network::SocketAddress>("socket_address", false);
				VSocketAddress.SetConstructor<Network::SocketAddress, addrinfo*, addrinfo*>("socket_address@ f(uptr@, uptr@)");
				VSocketAddress.SetMethod("bool is_usable() const", &Network::SocketAddress::IsUsable);
				VSocketAddress.SetMethod("uptr@ get() const", &Network::SocketAddress::Get);
				VSocketAddress.SetMethod("uptr@ get_alternatives() const", &Network::SocketAddress::GetAlternatives);
				VSocketAddress.SetMethod("string get_address() const", &Network::SocketAddress::GetAddress);

				VSocket.SetFunctionDef("void socket_written_event(socket_poll)");
				VSocket.SetFunctionDef("void socket_opened_event(socket_address@+)");
				VSocket.SetFunctionDef("void socket_connected_event(int)");
				VSocket.SetFunctionDef("void socket_closed_event()");
				VSocket.SetFunctionDef("bool socket_read_event(socket_poll, const string &in)");
				VSocket.SetFunctionDef("bool socket_accepted_event(usize, const string &in)");
				VSocket.SetProperty<Network::Socket>("uint64 timeout", &Network::Socket::Timeout);
				VSocket.SetProperty<Network::Socket>("int64 income", &Network::Socket::Income);
				VSocket.SetProperty<Network::Socket>("int64 outcome", &Network::Socket::Outcome);
				VSocket.SetProperty<Network::Socket>("uptr@ user_data", &Network::Socket::UserData);
				VSocket.SetConstructor<Network::Socket>("socket@ f()");
				VSocket.SetConstructor<Network::Socket, socket_t>("socket@ f(usize)");
				VSocket.SetMethodEx("int accept(socket@+, string &out)", &SocketAccept1);
				VSocket.SetMethodEx("int accept(usize &out, string &out)", &SocketAccept2);
				VSocket.SetMethodEx("int accept_async(bool, socket_accepted_event@+)", &SocketAcceptAsync);
				VSocket.SetMethod("int close(bool = true)", &Network::Socket::Close);
				VSocket.SetMethodEx("int close_async(bool, socket_closed_event@+)", &SocketCloseAsync);
				VSocket.SetMethod("int64 send_file(uptr@, int64, int64)", &Network::Socket::SendFile);
				VSocket.SetMethodEx("int64 send_file_async(uptr@, int64, int64, socket_written_event@+)", &SocketSendFileAsync);
				VSocket.SetMethodEx("int write(const string &in)", &SocketWrite);
				VSocket.SetMethodEx("int write_async(const string &in, socket_written_event@+)", &SocketWriteAsync);
				VSocket.SetMethodEx("int read(string &out, int)", &SocketRead1);
				VSocket.SetMethodEx("int read(string &out, int, socket_read_event@+)", &SocketRead2);
				VSocket.SetMethodEx("int read_async(usize, socket_read_event@+)", &SocketReadAsync);
				VSocket.SetMethodEx("int read_until(const string &in, socket_read_event@+)", &SocketReadUntil);
				VSocket.SetMethodEx("int read_until_async(const string &in, socket_read_event@+)", &SocketReadUntilAsync);
				VSocket.SetMethod("int connect(socket_address@+, uint64)", &Network::Socket::Connect);
				VSocket.SetMethodEx("int connect_async(socket_address@+, socket_connected_event@+)", &SocketConnectAsync);
				VSocket.SetMethod("int open(socket_address@+)", &Network::Socket::Open);
				VSocket.SetMethodEx("int secure(uptr@, const string &in)", &SocketSecure);
				VSocket.SetMethod("int bind(socket_address@+)", &Network::Socket::Bind);
				VSocket.SetMethod("int listen(int)", &Network::Socket::Listen);
				VSocket.SetMethod("int clear_events(bool)", &Network::Socket::ClearEvents);
				VSocket.SetMethod("int migrate_to(usize, bool = true)", &Network::Socket::MigrateTo);
				VSocket.SetMethod("int set_close_on_exec()", &Network::Socket::SetCloseOnExec);
				VSocket.SetMethod("int set_time_wait(int)", &Network::Socket::SetTimeWait);
				VSocket.SetMethod("int set_any_flag(int, int, int)", &Network::Socket::SetAnyFlag);
				VSocket.SetMethod("int set_socket_flag(int, int)", &Network::Socket::SetSocketFlag);
				VSocket.SetMethod("int set_blocking(bool)", &Network::Socket::SetBlocking);
				VSocket.SetMethod("int set_no_delay(bool)", &Network::Socket::SetNoDelay);
				VSocket.SetMethod("int set_keep_alive(bool)", &Network::Socket::SetKeepAlive);
				VSocket.SetMethod("int set_timeout(int)", &Network::Socket::SetTimeout);
				VSocket.SetMethod("int get_error(int)", &Network::Socket::GetError);
				VSocket.SetMethod("int get_any_flag(int, int, int &out)", &Network::Socket::GetAnyFlag);
				VSocket.SetMethod("int get_socket_flag(int, int &out)", &Network::Socket::GetSocketFlag);
				VSocket.SetMethod("int get_port()", &Network::Socket::GetPort);
				VSocket.SetMethod("usize get_fd()", &Network::Socket::GetFd);
				VSocket.SetMethod("uptr@ get_device()", &Network::Socket::GetDevice);
				VSocket.SetMethod("bool is_pending_for_read()", &Network::Socket::IsPendingForRead);
				VSocket.SetMethod("bool is_pending_for_write()", &Network::Socket::IsPendingForWrite);
				VSocket.SetMethod("bool is_pending()", &Network::Socket::IsPending);
				VSocket.SetMethod("bool is_valid()", &Network::Socket::IsValid);
				VSocket.SetMethod("string get_remote_address()", &Network::Socket::GetRemoteAddress);
				
				Engine->BeginNamespace("net_packet");
				Engine->SetFunction("bool is_data(socket_poll)", &Network::Packet::IsData);
				Engine->SetFunction("bool is_skip(socket_poll)", &Network::Packet::IsSkip);
				Engine->SetFunction("bool is_done(socket_poll)", &Network::Packet::IsDone);
				Engine->SetFunction("bool is_done_sync(socket_poll)", &Network::Packet::IsDoneSync);
				Engine->SetFunction("bool is_done_async(socket_poll)", &Network::Packet::IsDoneAsync);
				Engine->SetFunction("bool is_timeout(socket_poll)", &Network::Packet::IsTimeout);
				Engine->SetFunction("bool is_error(socket_poll)", &Network::Packet::IsError);
				Engine->SetFunction("bool is_error_or_skip(socket_poll)", &Network::Packet::IsErrorOrSkip);
				Engine->SetFunction("bool will_continue(socket_poll)", &Network::Packet::WillContinue);
				Engine->EndNamespace();

				Engine->BeginNamespace("dns");
				Engine->SetFunction("string find_name_from_address(const string &in, const string &in)", &Network::DNS::FindNameFromAddress);
				Engine->SetFunction("string find_address_from_name(const string &in, const string &in, dns_type, socket_protocol, socket_type)", &Network::DNS::FindAddressFromName);
				Engine->SetFunction("void release()", &Network::DNS::Release);
				Engine->EndNamespace();

				Engine->BeginNamespace("multiplexer");
				Engine->SetFunctionDef("void poll_event(socket@+, socket_poll)");
				Engine->SetFunction("void create(uint64 = 50, usize = 256)", &Network::Multiplexer::Create);
				Engine->SetFunction("void release()", &Network::Multiplexer::Release);
				Engine->SetFunction("void set_active(bool)", &Network::Multiplexer::SetActive);
				Engine->SetFunction("int dispatch(uint64)", &Network::Multiplexer::Dispatch);
				Engine->SetFunction("bool when_readable(socket@+, poll_event@+)", &MultiplexerWhenReadable);
				Engine->SetFunction("bool when_writeable(socket@+, poll_event@+)", &MultiplexerWhenWriteable);
				Engine->SetFunction("bool cancel_events(socket@+, socket_poll = socket_poll::cancel, bool = true)", &Network::Multiplexer::CancelEvents);
				Engine->SetFunction("bool clear_events(socket@+)", &Network::Multiplexer::ClearEvents);
				Engine->SetFunction("bool is_awaiting_events(socket@+)", &Network::Multiplexer::IsAwaitingEvents);
				Engine->SetFunction("bool is_awaiting_readable(socket@+)", &Network::Multiplexer::IsAwaitingReadable);
				Engine->SetFunction("bool is_awaiting_writeable(socket@+)", &Network::Multiplexer::IsAwaitingWriteable);
				Engine->SetFunction("bool is_listening()", &Network::Multiplexer::IsListening);
				Engine->SetFunction("bool is_active()", &Network::Multiplexer::IsActive);
				Engine->SetFunction("usize get_activations()", &Network::Multiplexer::GetActivations);
				Engine->SetFunction("string get_local_address()", &Network::Multiplexer::GetLocalAddress);
				Engine->SetFunction<Core::String(addrinfo*)>("string get_address(uptr@)", &Network::Multiplexer::GetAddress);
				Engine->EndNamespace();

				RefClass VSocketListener = Engine->SetClass<Network::SocketListener>("socket_listener", true);
				VSocketListener.SetProperty<Network::SocketListener>("string name", &Network::SocketListener::Name);
				VSocketListener.SetProperty<Network::SocketListener>("remote_host hostname", &Network::SocketListener::Hostname);
				VSocketListener.SetProperty<Network::SocketListener>("socket_address@ source", &Network::SocketListener::Source);
				VSocketListener.SetProperty<Network::SocketListener>("socket@ base", &Network::SocketListener::Base);
				VSocketListener.SetGcConstructor<Network::SocketListener, SocketListener, const Core::String&, const Network::RemoteHost&, Network::SocketAddress*>("socket_listener@ f(const string &in, const remote_host &in, socket_address@+)");
				VSocketListener.SetEnumRefsEx<Network::SocketListener>([](Network::SocketListener* Base, asIScriptEngine* Engine)
				{
					Engine->GCEnumCallback(Base->Base);
				});
				VSocketListener.SetReleaseRefsEx<Network::SocketListener>([](Network::SocketListener* Base, asIScriptEngine*)
				{
					Base->~SocketListener();
				});

				RefClass VSocketRouter = Engine->SetClass<Network::SocketRouter>("socket_router", false);
				VSocketRouter.SetProperty<Network::SocketRouter>("usize payload_max_length", &Network::SocketRouter::PayloadMaxLength);
				VSocketRouter.SetProperty<Network::SocketRouter>("usize backlog_queue", &Network::SocketRouter::BacklogQueue);
				VSocketRouter.SetProperty<Network::SocketRouter>("usize socket_timeout", &Network::SocketRouter::SocketTimeout);
				VSocketRouter.SetProperty<Network::SocketRouter>("usize max_connections", &Network::SocketRouter::MaxConnections);
				VSocketRouter.SetProperty<Network::SocketRouter>("int64 keep_alive_max_count", &Network::SocketRouter::KeepAliveMaxCount);
				VSocketRouter.SetProperty<Network::SocketRouter>("int64 graceful_time_wait", &Network::SocketRouter::GracefulTimeWait);
				VSocketRouter.SetProperty<Network::SocketRouter>("bool enable_no_delay", &Network::SocketRouter::EnableNoDelay);
				VSocketRouter.SetConstructor<Network::SocketRouter>("socket_router@ f()");
				VSocketRouter.SetMethod<Network::SocketRouter, Network::RemoteHost&, const Core::String&, int, bool>("remote_host& listen(const string &in, int, bool = false)", &Network::SocketRouter::Listen);
				VSocketRouter.SetMethod<Network::SocketRouter, Network::RemoteHost&, const Core::String&, const Core::String&, int, bool>("remote_host& listen(const string &in, const string &in, int, bool = false)", &Network::SocketRouter::Listen);
				VSocketRouter.SetMethodEx("void set_listeners(dictionary@ data)", &SocketRouterSetListeners);
				VSocketRouter.SetMethodEx("dictionary@ get_listeners() const", &SocketRouterGetListeners);
				VSocketRouter.SetMethodEx("void set_certificates(dictionary@ data)", &SocketRouterSetCertificates);
				VSocketRouter.SetMethodEx("dictionary@ get_certificates() const", &SocketRouterGetCertificates);

				RefClass VSocketConnection = Engine->SetClass<Network::SocketConnection>("socket_connection", true);
				VSocketConnection.SetProperty<Network::SocketConnection>("socket@ stream", &Network::SocketConnection::Stream);
				VSocketConnection.SetProperty<Network::SocketConnection>("socket_listener@ host", &Network::SocketConnection::Host);
				VSocketConnection.SetProperty<Network::SocketConnection>("socket_data_frame info", &Network::SocketConnection::Info);
				VSocketConnection.SetGcConstructor<Network::SocketConnection, SocketConnection>("socket_connection@ f()");
				VSocketConnection.SetMethodEx("string get_remote_address() const", &SocketConnectionGetRemoteAddress);
				VSocketConnection.SetMethod("void reset(bool)", &Network::SocketConnection::Reset);
				VSocketConnection.SetMethod<Network::SocketConnection, bool>("bool finish()", &Network::SocketConnection::Finish);
				VSocketConnection.SetMethod<Network::SocketConnection, bool, int>("bool finish(int)", &Network::SocketConnection::Finish);
				VSocketConnection.SetMethodEx("bool error(int, const string &in)", &SocketConnectionError);
				VSocketConnection.SetMethod("bool encryption_info(socket_certificate &out)", &Network::SocketConnection::EncryptionInfo);
				VSocketConnection.SetMethod("bool stop()", &Network::SocketConnection::Break);
				VSocketConnection.SetEnumRefsEx<Network::SocketConnection>([](Network::SocketConnection* Base, asIScriptEngine* Engine)
				{
					Engine->GCEnumCallback(Base->Stream);
					Engine->GCEnumCallback(Base->Host);
				});
				VSocketConnection.SetReleaseRefsEx<Network::SocketConnection>([](Network::SocketConnection* Base, asIScriptEngine*)
				{
					Base->~SocketConnection();
				});

				RefClass VSocketServer = Engine->SetClass<Network::SocketServer>("socket_server", true);
				VSocketServer.SetGcConstructor<Network::SocketServer, SocketServer>("socket_server@ f()");
				VSocketServer.SetMethod("void set_router(socket_router@+)", &Network::SocketServer::SetRouter);
				VSocketServer.SetMethod("void set_backlog(usize)", &Network::SocketServer::SetBacklog);
				VSocketServer.SetMethod("void lock()", &Network::SocketServer::Lock);
				VSocketServer.SetMethod("void unlock()", &Network::SocketServer::Unlock);
				VSocketServer.SetMethod("bool configure(socket_router@+)", &Network::SocketServer::Configure);
				VSocketServer.SetMethod("bool unlisten(uint64 = 5)", &Network::SocketServer::Unlisten);
				VSocketServer.SetMethod("bool listen()", &Network::SocketServer::Listen);
				VSocketServer.SetMethod("usize get_backlog() const", &Network::SocketServer::GetBacklog);
				VSocketServer.SetMethod("server_state get_state() const", &Network::SocketServer::GetState);
				VSocketServer.SetMethod("socket_router@+ get_router() const", &Network::SocketServer::GetRouter);
				VSocketServer.SetEnumRefsEx<Network::SocketServer>([](Network::SocketServer* Base, asIScriptEngine* Engine)
				{
					Engine->GCEnumCallback(Base->GetRouter());
					for (auto* Item : Base->GetActiveClients())
						Engine->GCEnumCallback(Item);

					for (auto* Item : Base->GetPooledClients())
						Engine->GCEnumCallback(Item);
				});
				VSocketServer.SetReleaseRefsEx<Network::SocketServer>([](Network::SocketServer* Base, asIScriptEngine*)
				{
					Base->~SocketServer();
				});

				RefClass VSocketClient = Engine->SetClass<Network::SocketClient>("socket_client", false);
				VSocketClient.SetConstructor<Network::SocketClient, int64_t>("socket_client@ f(int64)");
				VSocketClient.SetMethodEx("promise<int>@ connect(remote_host &in)", &VI_PROMISIFY(Network::SocketClient::Connect, TypeId::INT32));
				VSocketClient.SetMethodEx("promise<int>@ close()", &VI_PROMISIFY(Network::SocketClient::Close, TypeId::INT32));
				VSocketClient.SetMethod("socket@+ get_stream() const", &Network::SocketClient::GetStream);

				return true;
#else
				VI_ASSERT(false, false, "<network> is not loaded");
				return false;
#endif
			}
			bool Registry::LoadVM(VirtualMachine* Engine)
			{
#ifdef VI_HAS_BINDINGS
				VI_ASSERT(Engine != nullptr, false, "manager should be set");

				RefClass VVirtualMachine = Engine->SetClass<VirtualMachine>("virtual_machine", false);

				/* TODO: register bindings for <vm> module */
				return true;
#else
				VI_ASSERT(false, false, "<vm> is not loaded");
				return false;
#endif
			}
			bool Registry::LoadEngine(VirtualMachine* Engine)
			{
#ifdef VI_HAS_BINDINGS
				VI_ASSERT(Engine != nullptr, false, "manager should be set");
				VI_TYPEREF(Material, "material");
				VI_TYPEREF(Model, "model");
				VI_TYPEREF(SkinModel, "skin_model");
				VI_TYPEREF(RenderSystem, "render_system");
				VI_TYPEREF(ShaderCache, "shader_cache");
				VI_TYPEREF(PrimitiveCache, "primitive_cache");
				VI_TYPEREF(ContentManager, "content_manager");
				VI_TYPEREF(AppData, "app_data");
				VI_TYPEREF(SceneGraph, "scene_graph");
				VI_TYPEREF(ApplicationName, "application");

				Enumeration VApplicationSet = Engine->SetEnum("application_set");
				VApplicationSet.SetValue("graphics_set", (int)Engine::ApplicationSet::GraphicsSet);
				VApplicationSet.SetValue("activity_set", (int)Engine::ApplicationSet::ActivitySet);
				VApplicationSet.SetValue("audio_set", (int)Engine::ApplicationSet::AudioSet);
				VApplicationSet.SetValue("script_set", (int)Engine::ApplicationSet::ScriptSet);
				VApplicationSet.SetValue("content_set", (int)Engine::ApplicationSet::ContentSet);
				VApplicationSet.SetValue("network_set", (int)Engine::ApplicationSet::NetworkSet);

				Enumeration VApplicationState = Engine->SetEnum("application_state");
				VApplicationState.SetValue("terminated_t", (int)Engine::ApplicationState::Terminated);
				VApplicationState.SetValue("staging_t", (int)Engine::ApplicationState::Staging);
				VApplicationState.SetValue("active_t", (int)Engine::ApplicationState::Active);
				VApplicationState.SetValue("restart_t", (int)Engine::ApplicationState::Restart);

				Enumeration VRenderOpt = Engine->SetEnum("render_opt");
				VRenderOpt.SetValue("none_t", (int)Engine::RenderOpt::None);
				VRenderOpt.SetValue("transparent_t", (int)Engine::RenderOpt::Transparent);
				VRenderOpt.SetValue("static_t", (int)Engine::RenderOpt::Static);
				VRenderOpt.SetValue("additive_t", (int)Engine::RenderOpt::Additive);
				VRenderOpt.SetValue("backfaces_t", (int)Engine::RenderOpt::Backfaces);

				Enumeration VRenderCulling = Engine->SetEnum("render_culling");
				VRenderCulling.SetValue("linear_t", (int)Engine::RenderCulling::Linear);
				VRenderCulling.SetValue("cubic_t", (int)Engine::RenderCulling::Cubic);
				VRenderCulling.SetValue("disable_t", (int)Engine::RenderCulling::Disable);

				Enumeration VRenderState = Engine->SetEnum("render_state");
				VRenderState.SetValue("geometric_t", (int)Engine::RenderState::Geometric);
				VRenderState.SetValue("voxelization_t", (int)Engine::RenderState::Voxelization);
				VRenderState.SetValue("linearization_t", (int)Engine::RenderState::Linearization);
				VRenderState.SetValue("cubic_t", (int)Engine::RenderState::Cubic);

				Enumeration VGeoCategory = Engine->SetEnum("geo_category");
				VGeoCategory.SetValue("opaque_t", (int)Engine::GeoCategory::Opaque);
				VGeoCategory.SetValue("transparent_t", (int)Engine::GeoCategory::Transparent);
				VGeoCategory.SetValue("additive_t", (int)Engine::GeoCategory::Additive);
				VGeoCategory.SetValue("count_t", (int)Engine::GeoCategory::Count);

				Enumeration VBufferType = Engine->SetEnum("buffer_type");
				VBufferType.SetValue("index_t", (int)Engine::BufferType::Index);
				VBufferType.SetValue("vertex_t", (int)Engine::BufferType::Vertex);

				Enumeration VTargetType = Engine->SetEnum("target_type");
				VTargetType.SetValue("main_t", (int)Engine::TargetType::Main);
				VTargetType.SetValue("secondary_t", (int)Engine::TargetType::Secondary);
				VTargetType.SetValue("count_t", (int)Engine::TargetType::Count);

				Enumeration VVoxelType = Engine->SetEnum("voxel_type");
				VVoxelType.SetValue("diffuse_t", (int)Engine::VoxelType::Diffuse);
				VVoxelType.SetValue("normal_t", (int)Engine::VoxelType::Normal);
				VVoxelType.SetValue("surface_t", (int)Engine::VoxelType::Surface);

				Enumeration VEventTarget = Engine->SetEnum("event_target");
				VEventTarget.SetValue("scene_target", (int)Engine::EventTarget::Scene);
				VEventTarget.SetValue("entity_target", (int)Engine::EventTarget::Entity);
				VEventTarget.SetValue("component_target", (int)Engine::EventTarget::Component);
				VEventTarget.SetValue("listener_target", (int)Engine::EventTarget::Listener);

				Enumeration VActorSet = Engine->SetEnum("actor_set");
				VActorSet.SetValue("none_t", (int)Engine::ActorSet::None);
				VActorSet.SetValue("update_t", (int)Engine::ActorSet::Update);
				VActorSet.SetValue("synchronize_t", (int)Engine::ActorSet::Synchronize);
				VActorSet.SetValue("animate_t", (int)Engine::ActorSet::Animate);
				VActorSet.SetValue("message_t", (int)Engine::ActorSet::Message);
				VActorSet.SetValue("cullable_t", (int)Engine::ActorSet::Cullable);
				VActorSet.SetValue("drawable_t", (int)Engine::ActorSet::Drawable);

				Enumeration VActorType = Engine->SetEnum("actor_type");
				VActorType.SetValue("update_t", (int)Engine::ActorType::Update);
				VActorType.SetValue("synchronize_t", (int)Engine::ActorType::Synchronize);
				VActorType.SetValue("animate_t", (int)Engine::ActorType::Animate);
				VActorType.SetValue("message_t", (int)Engine::ActorType::Message);
				VActorType.SetValue("count_t", (int)Engine::ActorType::Count);

				Enumeration VComposerTag = Engine->SetEnum("composer_tag");
				VComposerTag.SetValue("component_t", (int)Engine::ComposerTag::Component);
				VComposerTag.SetValue("renderer_t", (int)Engine::ComposerTag::Renderer);
				VComposerTag.SetValue("effect_t", (int)Engine::ComposerTag::Effect);
				VComposerTag.SetValue("filter_t", (int)Engine::ComposerTag::Filter);

				Enumeration VRenderBufferType = Engine->SetEnum("render_buffer_type");
				VRenderBufferType.SetValue("Animation", (int)Engine::RenderBufferType::Animation);
				VRenderBufferType.SetValue("Render", (int)Engine::RenderBufferType::Render);
				VRenderBufferType.SetValue("View", (int)Engine::RenderBufferType::View);

				TypeClass VPoseNode = Engine->SetPod<Engine::PoseNode>("pose_node");
				VPoseNode.SetProperty<Engine::PoseNode>("vector3 position", &Engine::PoseNode::Position);
				VPoseNode.SetProperty<Engine::PoseNode>("vector3 scale", &Engine::PoseNode::Scale);
				VPoseNode.SetProperty<Engine::PoseNode>("quaternion rotation", &Engine::PoseNode::Rotation);
				VPoseNode.SetConstructor<Engine::PoseNode>("void f()");

				TypeClass VPoseData = Engine->SetPod<Engine::PoseData>("pose_data");
				VPoseData.SetProperty<Engine::PoseData>("pose_node frame_pose", &Engine::PoseData::Frame);
				VPoseData.SetProperty<Engine::PoseData>("pose_node offset_pose", &Engine::PoseData::Offset);
				VPoseData.SetProperty<Engine::PoseData>("pose_node default_pose", &Engine::PoseData::Default);
				VPoseData.SetConstructor<Engine::PoseData>("void f()");

				TypeClass VAnimationBuffer = Engine->SetPod<Engine::AnimationBuffer>("animation_buffer");
				VAnimationBuffer.SetProperty<Engine::AnimationBuffer>("vector3 padding", &Engine::AnimationBuffer::Padding);
				VAnimationBuffer.SetProperty<Engine::AnimationBuffer>("float animated", &Engine::AnimationBuffer::Animated);
				VAnimationBuffer.SetConstructor<Engine::AnimationBuffer>("void f()");
				VAnimationBuffer.SetOperatorEx(Operators::Index, (uint32_t)Position::Left, "matrix4x4&", "usize", &AnimationBufferGetOffsets);
				VAnimationBuffer.SetOperatorEx(Operators::Index, (uint32_t)Position::Const, "const matrix4x4&", "usize", &AnimationBufferGetOffsets);

				TypeClass VRenderBufferInstance = Engine->SetPod<Engine::RenderBuffer::Instance>("render_buffer_instance");
				VRenderBufferInstance.SetProperty<Engine::RenderBuffer::Instance>("matrix4x4 transform", &Engine::RenderBuffer::Instance::Transform);
				VRenderBufferInstance.SetProperty<Engine::RenderBuffer::Instance>("matrix4x4 world", &Engine::RenderBuffer::Instance::World);
				VRenderBufferInstance.SetProperty<Engine::RenderBuffer::Instance>("vector2 texcoord", &Engine::RenderBuffer::Instance::TexCoord);
				VRenderBufferInstance.SetProperty<Engine::RenderBuffer::Instance>("float diffuse", &Engine::RenderBuffer::Instance::Diffuse);
				VRenderBufferInstance.SetProperty<Engine::RenderBuffer::Instance>("float normal", &Engine::RenderBuffer::Instance::Normal);
				VRenderBufferInstance.SetProperty<Engine::RenderBuffer::Instance>("float height", &Engine::RenderBuffer::Instance::Height);
				VRenderBufferInstance.SetProperty<Engine::RenderBuffer::Instance>("float material_id", &Engine::RenderBuffer::Instance::MaterialId);
				VRenderBufferInstance.SetConstructor<Engine::RenderBuffer::Instance>("void f()");

				TypeClass VRenderBuffer = Engine->SetPod<Engine::RenderBuffer>("render_buffer");
				VRenderBuffer.SetProperty<Engine::RenderBuffer>("matrix4x4 transform", &Engine::RenderBuffer::Transform);
				VRenderBuffer.SetProperty<Engine::RenderBuffer>("matrix4x4 world", &Engine::RenderBuffer::World);
				VRenderBuffer.SetProperty<Engine::RenderBuffer>("vector4 texcoord", &Engine::RenderBuffer::TexCoord);
				VRenderBuffer.SetProperty<Engine::RenderBuffer>("float diffuse", &Engine::RenderBuffer::Diffuse);
				VRenderBuffer.SetProperty<Engine::RenderBuffer>("float normal", &Engine::RenderBuffer::Normal);
				VRenderBuffer.SetProperty<Engine::RenderBuffer>("float height", &Engine::RenderBuffer::Height);
				VRenderBuffer.SetProperty<Engine::RenderBuffer>("float material_id", &Engine::RenderBuffer::MaterialId);
				VRenderBuffer.SetConstructor<Engine::RenderBuffer>("void f()");

				TypeClass VViewBuffer = Engine->SetPod<Engine::ViewBuffer>("view_buffer");
				VViewBuffer.SetProperty<Engine::ViewBuffer>("matrix4x4 inv_view_proj", &Engine::ViewBuffer::InvViewProj);
				VViewBuffer.SetProperty<Engine::ViewBuffer>("matrix4x4 view_proj", &Engine::ViewBuffer::ViewProj);
				VViewBuffer.SetProperty<Engine::ViewBuffer>("matrix4x4 proj", &Engine::ViewBuffer::Proj);
				VViewBuffer.SetProperty<Engine::ViewBuffer>("matrix4x4 view", &Engine::ViewBuffer::View);
				VViewBuffer.SetProperty<Engine::ViewBuffer>("vector3 position", &Engine::ViewBuffer::Position);
				VViewBuffer.SetProperty<Engine::ViewBuffer>("float far", &Engine::ViewBuffer::Far);
				VViewBuffer.SetProperty<Engine::ViewBuffer>("vector3 direction", &Engine::ViewBuffer::Direction);
				VViewBuffer.SetProperty<Engine::ViewBuffer>("float near", &Engine::ViewBuffer::Near);
				VViewBuffer.SetConstructor<Engine::ViewBuffer>("void f()");

				RefClass VSkinModel = Engine->SetClass<Engine::SkinModel>("skin_model", true);
				TypeClass VPoseBuffer = Engine->SetStructTrivial<Engine::PoseBuffer>("pose_buffer");
				VPoseBuffer.SetMethodEx("void set_offset(int64, const pose_data &in)", &PoseBufferSetOffset);
				VPoseBuffer.SetMethodEx("void set_matrix(skin_mesh_buffer@+, usize, const matrix4x4 &in)", &PoseBufferSetMatrix);
				VPoseBuffer.SetMethodEx("pose_data& get_offset(int64)", &PoseBufferGetOffset);
				VPoseBuffer.SetMethodEx("matrix4x4& get_matrix(skin_mesh_buffer@+, usize)", &PoseBufferGetMatrix);
				VPoseBuffer.SetMethodEx("usize get_offsets_size()", &PoseBufferGetOffsetsSize);
				VPoseBuffer.SetMethodEx("usize get_matrices_size(skin_mesh_buffer@+)", &PoseBufferGetMatricesSize);
				VPoseBuffer.SetConstructor<Engine::PoseBuffer>("void f()");

				TypeClass VTicker = Engine->SetStructTrivial<Engine::Ticker>("clock_ticker");
				VTicker.SetProperty<Engine::Ticker>("float delay", &Engine::Ticker::Delay);
				VTicker.SetConstructor<Engine::Ticker>("void f()");
				VTicker.SetMethod("bool tick_event(float)", &Engine::Ticker::TickEvent);
				VTicker.SetMethod("float get_time() const", &Engine::Ticker::GetTime);

				TypeClass VEvent = Engine->SetStructTrivial<Engine::Event>("scene_event");
				VEvent.SetProperty<Engine::Event>("string name", &Engine::Event::Name);
				VEvent.SetConstructor<Engine::Event, const Core::String&>("void f(const string &in)");
				VEvent.SetMethodEx("void set_args(dictionary@+)", &EventSetArgs);
				VEvent.SetMethodEx("dictionary@ get_args() const", &EventGetArgs);

				RefClass VMaterial = Engine->SetClass<Engine::Material>("material", true);
				TypeClass VBatchData = Engine->SetStructTrivial<Engine::BatchData>("batch_data");
				VBatchData.SetProperty<Engine::BatchData>("element_buffer@ instances_buffer", &Engine::BatchData::InstanceBuffer);
				VBatchData.SetProperty<Engine::BatchData>("uptr@ geometry_buffer", &Engine::BatchData::GeometryBuffer);
				VBatchData.SetProperty<Engine::BatchData>("material@ batch_material", &Engine::BatchData::BatchMaterial);
				VBatchData.SetProperty<Engine::BatchData>("usize instances_count", &Engine::BatchData::InstancesCount);
				VBatchData.SetConstructor<Engine::BatchData>("void f()");

				TypeClass VAssetCache = Engine->SetStructTrivial<Engine::AssetCache>("asset_cache");
				VAssetCache.SetProperty<Engine::AssetCache>("string path", &Engine::AssetCache::Path);
				VAssetCache.SetProperty<Engine::AssetCache>("uptr@ resource", &Engine::AssetCache::Resource);
				VAssetCache.SetConstructor<Engine::AssetCache>("void f()");

				TypeClass VAssetArchive = Engine->SetStructTrivial<Engine::AssetArchive>("asset_archive");
				VAssetArchive.SetProperty<Engine::AssetArchive>("base_stream@ stream", &Engine::AssetArchive::Stream);
				VAssetArchive.SetProperty<Engine::AssetArchive>("string path", &Engine::AssetArchive::Path);
				VAssetArchive.SetProperty<Engine::AssetArchive>("usize length", &Engine::AssetArchive::Length);
				VAssetArchive.SetProperty<Engine::AssetArchive>("usize offset", &Engine::AssetArchive::Offset);
				VAssetArchive.SetConstructor<Engine::AssetArchive>("void f()");

				RefClass VAssetFile = Engine->SetClass<Engine::AssetFile>("asset_file", false);
				VAssetFile.SetConstructor<Engine::AssetFile, char*, size_t>("asset_file@ f(uptr@, usize)");
				VAssetFile.SetMethod("uptr@ get_buffer() const", &Engine::AssetFile::GetBuffer);
				VAssetFile.SetMethod("usize get_size() const", &Engine::AssetFile::GetSize);

				TypeClass VVisibilityQuery = Engine->SetPod<Engine::VisibilityQuery>("scene_visibility_query");
				VVisibilityQuery.SetProperty<Engine::VisibilityQuery>("geo_category category", &Engine::VisibilityQuery::Category);
				VVisibilityQuery.SetProperty<Engine::VisibilityQuery>("bool boundary_visible", &Engine::VisibilityQuery::BoundaryVisible);
				VVisibilityQuery.SetProperty<Engine::VisibilityQuery>("bool query_pixels", &Engine::VisibilityQuery::QueryPixels);
				VVisibilityQuery.SetConstructor<Engine::VisibilityQuery>("void f()");

				TypeClass VAnimatorState = Engine->SetPod<Engine::AnimatorState>("animator_state");
				VAnimatorState.SetProperty<Engine::AnimatorState>("bool paused", &Engine::AnimatorState::Paused);
				VAnimatorState.SetProperty<Engine::AnimatorState>("bool looped", &Engine::AnimatorState::Looped);
				VAnimatorState.SetProperty<Engine::AnimatorState>("bool blended", &Engine::AnimatorState::Blended);
				VAnimatorState.SetProperty<Engine::AnimatorState>("float duration", &Engine::AnimatorState::Duration);
				VAnimatorState.SetProperty<Engine::AnimatorState>("float rate", &Engine::AnimatorState::Rate);
				VAnimatorState.SetProperty<Engine::AnimatorState>("float time", &Engine::AnimatorState::Time);
				VAnimatorState.SetProperty<Engine::AnimatorState>("int64 frame", &Engine::AnimatorState::Frame);
				VAnimatorState.SetProperty<Engine::AnimatorState>("int64 clip", &Engine::AnimatorState::Clip);
				VAnimatorState.SetConstructor<Engine::AnimatorState>("void f()");
				VAnimatorState.SetMethod("float get_timeline(clock_timer@+) const", &Engine::AnimatorState::GetTimeline);
				VAnimatorState.SetMethod("float get_seconds_duration() const", &Engine::AnimatorState::GetSecondsDuration);
				VAnimatorState.SetMethod("float get_progress() const", &Engine::AnimatorState::GetProgress);
				VAnimatorState.SetMethod("bool is_playing() const", &Engine::AnimatorState::IsPlaying);

				TypeClass VSpawnerProperties = Engine->SetPod<Engine::SpawnerProperties>("spawner_properties");
				VSpawnerProperties.SetProperty<Engine::SpawnerProperties>("random_vector4 diffusion", &Engine::SpawnerProperties::Diffusion);
				VSpawnerProperties.SetProperty<Engine::SpawnerProperties>("random_vector3 position", &Engine::SpawnerProperties::Position);
				VSpawnerProperties.SetProperty<Engine::SpawnerProperties>("random_vector3 velocity", &Engine::SpawnerProperties::Velocity);
				VSpawnerProperties.SetProperty<Engine::SpawnerProperties>("random_vector3 noise", &Engine::SpawnerProperties::Noise);
				VSpawnerProperties.SetProperty<Engine::SpawnerProperties>("random_float rotation", &Engine::SpawnerProperties::Rotation);
				VSpawnerProperties.SetProperty<Engine::SpawnerProperties>("random_float scale", &Engine::SpawnerProperties::Scale);
				VSpawnerProperties.SetProperty<Engine::SpawnerProperties>("random_float angular", &Engine::SpawnerProperties::Angular);
				VSpawnerProperties.SetProperty<Engine::SpawnerProperties>("int32 iterations", &Engine::SpawnerProperties::Iterations);
				VSpawnerProperties.SetConstructor<Engine::SpawnerProperties>("void f()");

				RefClass VRenderConstants = Engine->SetClass<Engine::RenderConstants>("render_constants", false);
				VRenderConstants.SetProperty<Engine::RenderConstants>("animation_buffer animation", &Engine::RenderConstants::Animation);
				VRenderConstants.SetProperty<Engine::RenderConstants>("render_buffer render", &Engine::RenderConstants::Render);
				VRenderConstants.SetProperty<Engine::RenderConstants>("view_buffer view", &Engine::RenderConstants::View);
				VRenderConstants.SetConstructor<Engine::RenderConstants, Graphics::GraphicsDevice*>("render_constants@ f()");
				VRenderConstants.SetMethod("void set_constant_buffers()", &Engine::RenderConstants::SetConstantBuffers);
				VRenderConstants.SetMethod("void update_constant_buffer(render_buffer_type)", &Engine::RenderConstants::UpdateConstantBuffer);
				VRenderConstants.SetMethod("shader@+ get_basic_effect() const", &Engine::RenderConstants::GetBasicEffect);
				VRenderConstants.SetMethod("graphics_device@+ get_device() const", &Engine::RenderConstants::GetDevice);
				VRenderConstants.SetMethod("element_buffer@+ get_constant_buffer(render_buffer_type) const", &Engine::RenderConstants::GetConstantBuffer);

				RefClass VRenderSystem = Engine->SetClass<Engine::RenderSystem>("render_system", true);
				TypeClass VViewer = Engine->SetStruct<Engine::Viewer>("viewer_t");
				VViewer.SetProperty<Engine::Viewer>("render_system@ renderer", &Engine::Viewer::Renderer);
				VViewer.SetProperty<Engine::Viewer>("render_culling culling", &Engine::Viewer::Culling);
				VViewer.SetProperty<Engine::Viewer>("matrix4x4 inv_view_projection", &Engine::Viewer::InvViewProjection);
				VViewer.SetProperty<Engine::Viewer>("matrix4x4 view_projection", &Engine::Viewer::ViewProjection);
				VViewer.SetProperty<Engine::Viewer>("matrix4x4 projection", &Engine::Viewer::Projection);
				VViewer.SetProperty<Engine::Viewer>("matrix4x4 view", &Engine::Viewer::View);
				VViewer.SetProperty<Engine::Viewer>("vector3 inv_position", &Engine::Viewer::InvPosition);
				VViewer.SetProperty<Engine::Viewer>("vector3 position", &Engine::Viewer::Position);
				VViewer.SetProperty<Engine::Viewer>("vector3 rotation", &Engine::Viewer::Rotation);
				VViewer.SetProperty<Engine::Viewer>("float far_plane", &Engine::Viewer::FarPlane);
				VViewer.SetProperty<Engine::Viewer>("float near_plane", &Engine::Viewer::NearPlane);
				VViewer.SetProperty<Engine::Viewer>("float ratio", &Engine::Viewer::Ratio);
				VViewer.SetProperty<Engine::Viewer>("float fov", &Engine::Viewer::Fov);
				VViewer.SetConstructor<Engine::Viewer>("void f()");
				VViewer.SetOperatorCopyStatic(&ViewerCopy);
				VViewer.SetDestructorStatic("void f()", &ViewerDestructor);
				VViewer.SetMethod<Engine::Viewer, void, const Compute::Matrix4x4&, const Compute::Matrix4x4&, const Compute::Vector3&, float, float, float, float, Engine::RenderCulling>("void set(const matrix4x4 &in, const matrix4x4 &in, const vector3 &in, float, float, float, float, render_culling)", &Engine::Viewer::Set);
				VViewer.SetMethod<Engine::Viewer, void, const Compute::Matrix4x4&, const Compute::Matrix4x4&, const Compute::Vector3&, const Compute::Vector3&, float, float, float, float, Engine::RenderCulling>("void set(const matrix4x4 &in, const matrix4x4 &in, const vector3 &in, const vector3 &in, float, float, float, float, render_culling)", &Engine::Viewer::Set);

				TypeClass VAttenuation = Engine->SetPod<Engine::Attenuation>("attenuation");
				VAttenuation.SetProperty<Engine::Attenuation>("float radius", &Engine::Attenuation::Radius);
				VAttenuation.SetProperty<Engine::Attenuation>("float c1", &Engine::Attenuation::C1);
				VAttenuation.SetProperty<Engine::Attenuation>("float c2", &Engine::Attenuation::C2);
				VAttenuation.SetConstructor<Engine::Attenuation>("void f()");

				TypeClass VSubsurface = Engine->SetPod<Engine::Subsurface>("subsurface");
				VSubsurface.SetProperty<Engine::Subsurface>("vector4 emission", &Engine::Subsurface::Emission);
				VSubsurface.SetProperty<Engine::Subsurface>("vector4 metallic", &Engine::Subsurface::Metallic);
				VSubsurface.SetProperty<Engine::Subsurface>("vector3 diffuse", &Engine::Subsurface::Diffuse);
				VSubsurface.SetProperty<Engine::Subsurface>("float fresnel", &Engine::Subsurface::Fresnel);
				VSubsurface.SetProperty<Engine::Subsurface>("vector3 scatter", &Engine::Subsurface::Scatter);
				VSubsurface.SetProperty<Engine::Subsurface>("float transparency", &Engine::Subsurface::Transparency);
				VSubsurface.SetProperty<Engine::Subsurface>("vector3 padding", &Engine::Subsurface::Padding);
				VSubsurface.SetProperty<Engine::Subsurface>("float bias", &Engine::Subsurface::Bias);
				VSubsurface.SetProperty<Engine::Subsurface>("vector2 roughness", &Engine::Subsurface::Roughness);
				VSubsurface.SetProperty<Engine::Subsurface>("float refraction", &Engine::Subsurface::Refraction);
				VSubsurface.SetProperty<Engine::Subsurface>("float environment", &Engine::Subsurface::Environment);
				VSubsurface.SetProperty<Engine::Subsurface>("vector2 occlusion", &Engine::Subsurface::Occlusion);
				VSubsurface.SetProperty<Engine::Subsurface>("float radius", &Engine::Subsurface::Radius);
				VSubsurface.SetProperty<Engine::Subsurface>("float height", &Engine::Subsurface::Height);
				VSubsurface.SetConstructor<Engine::Subsurface>("void f()");

				RefClass VSkinAnimation = Engine->SetClass<Engine::SkinAnimation>("skin_animation", false);
				VSkinAnimation.SetMethodEx("array<skin_animator_clip>@+ get_clips() const", &SkinAnimationGetClips);
				VSkinAnimation.SetMethod("bool is_valid() const", &Engine::SkinAnimation::IsValid);

				RefClass VSceneGraph = Engine->SetClass<Engine::SceneGraph>("scene_graph", true);
				VMaterial.SetProperty<Engine::Material>("subsurface surface", &Engine::Material::Surface);
				VMaterial.SetProperty<Engine::Material>("usize slot", &Engine::Material::Slot);
				VMaterial.SetGcConstructor<Engine::Material, Material, Engine::SceneGraph*>("material@ f(scene_graph@+)");
				VMaterial.SetMethod("void set_name(const string &in)", &Engine::Material::SetName);
				VMaterial.SetMethod("const string& get_name(const string &in)", &Engine::Material::GetName);
				VMaterial.SetMethod("void set_diffuse_map(texture_2d@+)", &Engine::Material::SetDiffuseMap);
				VMaterial.SetMethod("texture_2d@+ get_diffuse_map() const", &Engine::Material::GetDiffuseMap);
				VMaterial.SetMethod("void set_normal_map(texture_2d@+)", &Engine::Material::SetNormalMap);
				VMaterial.SetMethod("texture_2d@+ get_normal_map() const", &Engine::Material::GetNormalMap);
				VMaterial.SetMethod("void set_metallic_map(texture_2d@+)", &Engine::Material::SetMetallicMap);
				VMaterial.SetMethod("texture_2d@+ get_metallic_map() const", &Engine::Material::GetMetallicMap);
				VMaterial.SetMethod("void set_roughness_map(texture_2d@+)", &Engine::Material::SetRoughnessMap);
				VMaterial.SetMethod("texture_2d@+ get_roughness_map() const", &Engine::Material::GetRoughnessMap);
				VMaterial.SetMethod("void set_height_map(texture_2d@+)", &Engine::Material::SetHeightMap);
				VMaterial.SetMethod("texture_2d@+ get_height_map() const", &Engine::Material::GetHeightMap);
				VMaterial.SetMethod("void set_occlusion_map(texture_2d@+)", &Engine::Material::SetOcclusionMap);
				VMaterial.SetMethod("texture_2d@+ get_occlusion_map() const", &Engine::Material::GetOcclusionMap);
				VMaterial.SetMethod("void set_emission_map(texture_2d@+)", &Engine::Material::SetEmissionMap);
				VMaterial.SetMethod("texture_2d@+ get_emission_map() const", &Engine::Material::GetEmissionMap);
				VMaterial.SetMethod("scene_graph@+ get_scene() const", &Engine::Material::GetScene);
				VMaterial.SetEnumRefsEx<Engine::Material>([](Engine::Material* Base, asIScriptEngine* Engine)
				{
					Engine->GCEnumCallback(Base->GetDiffuseMap());
					Engine->GCEnumCallback(Base->GetNormalMap());
					Engine->GCEnumCallback(Base->GetMetallicMap());
					Engine->GCEnumCallback(Base->GetRoughnessMap());
					Engine->GCEnumCallback(Base->GetHeightMap());
					Engine->GCEnumCallback(Base->GetEmissionMap());
				});
				VMaterial.SetReleaseRefsEx<Engine::Material>([](Engine::Material* Base, asIScriptEngine*)
				{
					Base->~Material();
				});

				RefClass VComponent = Engine->SetClass<Engine::Component>("base_component", false);
				TypeClass VSparseIndex = Engine->SetStructTrivial<Engine::SparseIndex>("sparse_index");
				VSparseIndex.SetProperty<Engine::SparseIndex>("cosmos index", &Engine::SparseIndex::Index);
				VSparseIndex.SetMethodEx("usize size() const", &SparseIndexGetSize);
				VSparseIndex.SetOperatorEx(Operators::Index, (uint32_t)Position::Left, "base_component@+", "usize", &SparseIndexGetData);
				VSparseIndex.SetConstructor<Engine::SparseIndex>("void f()");

				Engine->BeginNamespace("content_series");
				Engine->SetFunction<void(Core::Schema*, bool)>("void pack(schema@+, bool)", &Engine::Series::Pack);
				Engine->SetFunction<void(Core::Schema*, int)>("void pack(schema@+, int32)", &Engine::Series::Pack);
				Engine->SetFunction<void(Core::Schema*, unsigned int)>("void pack(schema@+, uint32)", &Engine::Series::Pack);;
				Engine->SetFunction<void(Core::Schema*, float)>("void pack(schema@+, float)", &Engine::Series::Pack);
				Engine->SetFunction<void(Core::Schema*, double)>("void pack(schema@+, double)", &Engine::Series::Pack);
				Engine->SetFunction<void(Core::Schema*, int64_t)>("void pack(schema@+, int64)", &Engine::Series::Pack);
				Engine->SetFunction<void(Core::Schema*, unsigned long long)>("void pack(schema@+, uint64)", &Engine::Series::Pack);
				Engine->SetFunction<void(Core::Schema*, const Compute::Vector2&)>("void pack(schema@+, const vector2 &in)", &Engine::Series::Pack);
				Engine->SetFunction<void(Core::Schema*, const Compute::Vector3&)>("void pack(schema@+, const vector3 &in)", &Engine::Series::Pack);
				Engine->SetFunction<void(Core::Schema*, const Compute::Vector4&)>("void pack(schema@+, const vector4 &in)", &Engine::Series::Pack);
				Engine->SetFunction<void(Core::Schema*, const Compute::Vector4&)>("void pack(schema@+, const quaternion &in)", &Engine::Series::Pack);
				Engine->SetFunction<void(Core::Schema*, const Compute::Matrix4x4&)>("void pack(schema@+, const matrix4x4 &in)", &Engine::Series::Pack);
				Engine->SetFunction<void(Core::Schema*, const Engine::Attenuation&)>("void pack(schema@+, const attenuation &in)", &Engine::Series::Pack);
				Engine->SetFunction<void(Core::Schema*, const Engine::AnimatorState&)>("void pack(schema@+, const animator_state &in)", &Engine::Series::Pack);
				Engine->SetFunction<void(Core::Schema*, const Engine::SpawnerProperties&)>("void pack(schema@+, const spawner_properties &in)", &Engine::Series::Pack);
				Engine->SetFunction<void(Core::Schema*, const Compute::SkinAnimatorKey&)>("void pack(schema@+, const skin_animator_key &in)", &Engine::Series::Pack);
				Engine->SetFunction<void(Core::Schema*, const Compute::KeyAnimatorClip&)>("void pack(schema@+, const key_animator_clip &in)", &Engine::Series::Pack);
				Engine->SetFunction<void(Core::Schema*, const Compute::AnimatorKey&)>("void pack(schema@+, const animator_key &in)", &Engine::Series::Pack);
				Engine->SetFunction<void(Core::Schema*, const Compute::ElementVertex&)>("void pack(schema@+, const element_vertex &in)", &Engine::Series::Pack);
				Engine->SetFunction<void(Core::Schema*, const Compute::Joint&)>("void pack(schema@+, const joint &in)", &Engine::Series::Pack);
				Engine->SetFunction<void(Core::Schema*, const Compute::Vertex&)>("void pack(schema@+, const vertex &in)", &Engine::Series::Pack);
				Engine->SetFunction<void(Core::Schema*, const Compute::SkinVertex&)>("void pack(schema@+, const skin_vertex &in)", &Engine::Series::Pack);
				Engine->SetFunction<void(Core::Schema*, const Engine::Ticker&)>("void pack(schema@+, const clock_ticker &in)", &Engine::Series::Pack);
				Engine->SetFunction<void(Core::Schema*, const Core::String&)>("void pack(schema@+, const string &in)", &Engine::Series::Pack);
				Engine->SetFunction<bool(Core::Schema*, bool*)>("bool unpack(schema@+, bool &out)", &Engine::Series::Unpack);
				Engine->SetFunction<bool(Core::Schema*, int*)>("bool unpack(schema@+, int32 &out)", &Engine::Series::Unpack);
				Engine->SetFunction<bool(Core::Schema*, unsigned int*)>("bool unpack(schema@+, uint32 &out)", &Engine::Series::Unpack);
				Engine->SetFunction<bool(Core::Schema*, float*)>("bool unpack(schema@+, float &out)", &Engine::Series::Unpack);
				Engine->SetFunction<bool(Core::Schema*, double*)>("bool unpack(schema@+, double &out)", &Engine::Series::Unpack);
				Engine->SetFunction<bool(Core::Schema*, int64_t*)>("bool unpack(schema@+, int64 &out)", &Engine::Series::Unpack);
				Engine->SetFunction<bool(Core::Schema*, unsigned long long*)>("bool unpack(schema@+, uint64 &out)", &Engine::Series::Unpack);
				Engine->SetFunction<bool(Core::Schema*, Compute::Vector2*)>("bool unpack(schema@+, vector2 &out)", &Engine::Series::Unpack);
				Engine->SetFunction<bool(Core::Schema*, Compute::Vector3*)>("bool unpack(schema@+, vector3 &out)", &Engine::Series::Unpack);
				Engine->SetFunction<bool(Core::Schema*, Compute::Vector4*)>("bool unpack(schema@+, vector4 &out)", &Engine::Series::Unpack);
				Engine->SetFunction<bool(Core::Schema*, Compute::Vector4*)>("bool unpack(schema@+, quaternion &out)", &Engine::Series::Unpack);
				Engine->SetFunction<bool(Core::Schema*, Compute::Matrix4x4*)>("bool unpack(schema@+, matrix4x4 &out)", &Engine::Series::Unpack);
				Engine->SetFunction<bool(Core::Schema*, Engine::Attenuation*)>("bool unpack(schema@+, attenuation &out)", &Engine::Series::Unpack);
				Engine->SetFunction<bool(Core::Schema*, Engine::AnimatorState*)>("bool unpack(schema@+, animator_state &out)", &Engine::Series::Unpack);
				Engine->SetFunction<bool(Core::Schema*, Engine::SpawnerProperties*)>("bool unpack(schema@+, spawner_properties &out)", &Engine::Series::Unpack);
				Engine->SetFunction<bool(Core::Schema*, Compute::SkinAnimatorKey*)>("bool unpack(schema@+, skin_animator_key &out)", &Engine::Series::Unpack);
				Engine->SetFunction<bool(Core::Schema*, Compute::KeyAnimatorClip*)>("bool unpack(schema@+, key_animator_clip &out)", &Engine::Series::Unpack);
				Engine->SetFunction<bool(Core::Schema*, Compute::AnimatorKey*)>("bool unpack(schema@+, animator_key &out)", &Engine::Series::Unpack);
				Engine->SetFunction<bool(Core::Schema*, Compute::ElementVertex*)>("bool unpack(schema@+, element_vertex &out)", &Engine::Series::Unpack);
				Engine->SetFunction<bool(Core::Schema*, Compute::Joint*)>("bool unpack(schema@+, joint &out)", &Engine::Series::Unpack);
				Engine->SetFunction<bool(Core::Schema*, Compute::Vertex*)>("bool unpack(schema@+, vertex &out)", &Engine::Series::Unpack);
				Engine->SetFunction<bool(Core::Schema*, Compute::SkinVertex*)>("bool unpack(schema@+, skin_vertex &out)", &Engine::Series::Unpack);
				Engine->SetFunction<bool(Core::Schema*, Engine::Ticker*)>("bool unpack(schema@+, clock_ticker &out)", &Engine::Series::Unpack);
				Engine->SetFunction<bool(Core::Schema*, Core::String*)>("bool unpack(schema@+, string &out)", &Engine::Series::Unpack);
				Engine->EndNamespace();

				RefClass VModel = Engine->SetClass<Engine::Model>("model", true);
				VModel.SetProperty<Engine::Model>("vector4 max", &Engine::Model::Max);
				VModel.SetProperty<Engine::Model>("vector4 min", &Engine::Model::Min);
				VModel.SetGcConstructor<Engine::Model, Model>("model@ f()");
				VModel.SetMethod("mesh_buffer@+ find_mesh(const string &in) const", &Engine::Model::FindMesh);
				VModel.SetMethodEx("array<mesh_buffer@>@ get_meshes() const", &ModelGetMeshes);
				VModel.SetMethodEx("void set_meshes(array<mesh_buffer@>@+)", &ModelSetMeshes);
				VModel.SetEnumRefsEx<Engine::Model>([](Engine::Model* Base, asIScriptEngine* Engine)
				{
					for (auto* Item : Base->Meshes)
						Engine->GCEnumCallback(Item);
				});
				VModel.SetReleaseRefsEx<Engine::Model>([](Engine::Model* Base, asIScriptEngine*)
				{
					Base->Cleanup();
				});

				VSkinModel.SetProperty<Engine::SkinModel>("joint skeleton", &Engine::SkinModel::Skeleton);
				VSkinModel.SetProperty<Engine::SkinModel>("matrix4x4 inv_transform", &Engine::SkinModel::InvTransform);
				VSkinModel.SetProperty<Engine::SkinModel>("matrix4x4 base_transform", &Engine::SkinModel::Transform);
				VSkinModel.SetProperty<Engine::SkinModel>("vector4 max", &Engine::SkinModel::Max);
				VSkinModel.SetProperty<Engine::SkinModel>("vector4 min", &Engine::SkinModel::Min);
				VSkinModel.SetGcConstructor<Engine::SkinModel, SkinModel>("skin_model@ f()");
				VSkinModel.SetMethod<Engine::SkinModel, bool, const Core::String&, Compute::Joint*>("bool find_joint(const string &in, joint &out) const", &Engine::SkinModel::FindJoint);
				VSkinModel.SetMethod<Engine::SkinModel, bool, size_t, Compute::Joint*>("bool find_joint(usize, joint &out) const", &Engine::SkinModel::FindJoint);
				VSkinModel.SetMethod("skin_mesh_buffer@+ find_mesh(const string &in) const", &Engine::SkinModel::FindMesh);
				VSkinModel.SetMethodEx("array<skin_mesh_buffer@>@ get_meshes() const", &SkinModelGetMeshes);
				VSkinModel.SetMethodEx("void set_meshes(array<skin_mesh_buffer@>@+)", &SkinModelSetMeshes);
				VSkinModel.SetEnumRefsEx<Engine::SkinModel>([](Engine::SkinModel* Base, asIScriptEngine* Engine)
				{
					for (auto* Item : Base->Meshes)
						Engine->GCEnumCallback(Item);
				});
				VSkinModel.SetReleaseRefsEx<Engine::SkinModel>([](Engine::SkinModel* Base, asIScriptEngine*)
				{
					Base->Cleanup();
				});

				RefClass VContentManager = Engine->SetClass<Engine::ContentManager>("content_manager", true);
				RefClass VProcessor = Engine->SetClass<Engine::Processor>("base_processor", false);
				PopulateProcessorBase<Engine::Processor>(VProcessor);

				RefClass VEntity = Engine->SetClass<Engine::Entity>("scene_entity", true);
				PopulateComponentBase<Engine::Component>(VComponent);

				VEntity.SetMethod("void set_name(const string &in)", &Engine::Entity::SetName);
				VEntity.SetMethod("void set_root(scene_entity@+)", &Engine::Entity::SetRoot);
				VEntity.SetMethod("void update_bounds()", &Engine::Entity::UpdateBounds);
				VEntity.SetMethod("void remove_childs()", &Engine::Entity::RemoveChilds);
				VEntity.SetMethodEx("void remove_component(base_component@+)", &EntityRemoveComponent);
				VEntity.SetMethodEx("void remove_component(uint64)", &EntityRemoveComponentById);
				VEntity.SetMethodEx("base_component@+ add_component(base_component@+)", &EntityAddComponent);
				VEntity.SetMethodEx("base_component@+ get_component(uint64) const", &EntityGetComponentById);
				VEntity.SetMethodEx("array<base_component@>@ get_components() const", &EntityGetComponents);
				VEntity.SetMethod("scene_graph@+ get_scene() const", &Engine::Entity::GetScene);
				VEntity.SetMethod("scene_entity@+ get_parent() const", &Engine::Entity::GetParent);
				VEntity.SetMethod("scene_entity@+ get_child(usize) const", &Engine::Entity::GetChild);
				VEntity.SetMethod("transform@+ get_transform() const", &Engine::Entity::GetTransform);
				VEntity.SetMethod("const matrix4x4& get_box() const", &Engine::Entity::GetBox);
				VEntity.SetMethod("const vector3& get_min() const", &Engine::Entity::GetMin);
				VEntity.SetMethod("const vector3& get_max() const", &Engine::Entity::GetMax);
				VEntity.SetMethod("const string& get_name() const", &Engine::Entity::GetName);
				VEntity.SetMethod("usize get_components_count() const", &Engine::Entity::GetComponentsCount);
				VEntity.SetMethod("usize get_childs_count() const", &Engine::Entity::GetComponentsCount);
				VEntity.SetMethod("float get_visibility(const viewer_t &in) const", &Engine::Entity::GetVisibility);
				VEntity.SetMethod("float is_active() const", &Engine::Entity::IsActive);
				VEntity.SetMethod("vector3 get_radius3() const", &Engine::Entity::GetRadius3);
				VEntity.SetMethod("float get_radius() const", &Engine::Entity::GetRadius);
				VEntity.SetEnumRefsEx<Engine::Entity>([](Engine::Entity* Base, asIScriptEngine* Engine)
				{
					for (auto& Item : *Base)
						Engine->GCEnumCallback(Item.second);
				});
				VEntity.SetReleaseRefsEx<Engine::Entity>([](Engine::Entity* Base, asIScriptEngine*) { });

				RefClass VDrawable = Engine->SetClass<Engine::Drawable>("drawable_component", false);
				PopulateDrawableBase<Engine::Drawable>(VDrawable);

				RefClass VRenderer = Engine->SetClass<Engine::Renderer>("base_renderer", false);
				PopulateRendererBase<Engine::Renderer>(VRenderer);

				TypeClass VRsState = Engine->SetPod<Engine::RenderSystem::RsState>("rs_state");
				VRsState.SetMethod("bool is_state(render_state) const", &Engine::RenderSystem::RsState::Is);
				VRsState.SetMethod("bool is_set(render_opt) const", &Engine::RenderSystem::RsState::IsSet);
				VRsState.SetMethod("bool is_top() const", &Engine::RenderSystem::RsState::IsTop);
				VRsState.SetMethod("bool is_subpass() const", &Engine::RenderSystem::RsState::IsSubpass);
				VRsState.SetMethod("render_opt get_opts() const", &Engine::RenderSystem::RsState::GetOpts);
				VRsState.SetMethod("render_state get_state() const", &Engine::RenderSystem::RsState::Get);

				RefClass VPrimitiveCache = Engine->SetClass<Engine::Renderer>("primitive_cache", true);
				VRenderSystem.SetFunctionDef("void overlapping_result(base_component@+)");
				VRenderSystem.SetProperty<Engine::RenderSystem>("rs_state state", &Engine::RenderSystem::State);
				VRenderSystem.SetProperty<Engine::RenderSystem>("viewer_t view", &Engine::RenderSystem::View);
				VRenderSystem.SetProperty<Engine::RenderSystem>("usize max_queries", &Engine::RenderSystem::MaxQueries);
				VRenderSystem.SetProperty<Engine::RenderSystem>("usize sorting_frequency", &Engine::RenderSystem::SortingFrequency);
				VRenderSystem.SetProperty<Engine::RenderSystem>("usize occlusion_skips", &Engine::RenderSystem::OcclusionSkips);
				VRenderSystem.SetProperty<Engine::RenderSystem>("usize occluder_skips", &Engine::RenderSystem::OccluderSkips);
				VRenderSystem.SetProperty<Engine::RenderSystem>("usize occludee_skips", &Engine::RenderSystem::OccludeeSkips);
				VRenderSystem.SetProperty<Engine::RenderSystem>("float overflow_visibility", &Engine::RenderSystem::OverflowVisibility);
				VRenderSystem.SetProperty<Engine::RenderSystem>("float threshold", &Engine::RenderSystem::Threshold);
				VRenderSystem.SetProperty<Engine::RenderSystem>("bool occlusion_culling", &Engine::RenderSystem::OcclusionCulling);
				VRenderSystem.SetProperty<Engine::RenderSystem>("bool precise_culling", &Engine::RenderSystem::PreciseCulling);
				VRenderSystem.SetProperty<Engine::RenderSystem>("bool allow_input_lag", &Engine::RenderSystem::AllowInputLag);
				VRenderSystem.SetGcConstructor<Engine::RenderSystem, RenderSystem, Engine::SceneGraph*, Engine::Component*>("render_system@ f(scene_graph@+, base_component@+)");
				VRenderSystem.SetMethod("void set_view(const matrix4x4 &in, const matrix4x4 &in, const vector3 &in, float, float, float, float, render_culling)", &Engine::RenderSystem::SetView);
				VRenderSystem.SetMethod("void clear_culling()", &Engine::RenderSystem::ClearCulling);
				VRenderSystem.SetMethodEx("void restore_view_buffer()", &RenderSystemRestoreViewBuffer);
				VRenderSystem.SetMethod("void restore_view_buffer(viewer_t &out)", &Engine::RenderSystem::RestoreViewBuffer);
				VRenderSystem.SetMethod<Engine::RenderSystem, void, Engine::Renderer*>("void remount(base_renderer@+)", &Engine::RenderSystem::Remount);
				VRenderSystem.SetMethod<Engine::RenderSystem, void>("void remount()", &Engine::RenderSystem::Remount);
				VRenderSystem.SetMethod("void mount()", &Engine::RenderSystem::Mount);
				VRenderSystem.SetMethod("void unmount()", &Engine::RenderSystem::Unmount);
				VRenderSystem.SetMethod("void move_renderer(uint64, usize)", &Engine::RenderSystem::MoveRenderer);
				VRenderSystem.SetMethod<Engine::RenderSystem, void, uint64_t>("void remove_renderer(uint64)", &Engine::RenderSystem::RemoveRenderer);
				VRenderSystem.SetMethodEx("void move_renderer(base_renderer@+, usize)", &RenderSystemMoveRenderer);
				VRenderSystem.SetMethodEx("void remove_renderer(base_renderer@+, usize)", &RenderSystemRemoveRenderer);
				VRenderSystem.SetMethod("void restore_output()", &Engine::RenderSystem::RestoreOutput);
				VRenderSystem.SetMethod<Engine::RenderSystem, void, const Core::String&, Graphics::Shader*>("void free_shader(const string &in, shader@+)", &Engine::RenderSystem::FreeShader);
				VRenderSystem.SetMethod<Engine::RenderSystem, void, Graphics::Shader*>("void free_shader(shader@+)", &Engine::RenderSystem::FreeShader);
				VRenderSystem.SetMethodEx("void free_buffers(const string &in, element_buffer@+, element_buffer@+)", &RenderSystemFreeBuffers1);
				VRenderSystem.SetMethodEx("void free_buffers(element_buffer@+, element_buffer@+)", &RenderSystemFreeBuffers2);
				VRenderSystem.SetMethod("void update_constant_buffer(render_buffer_type)", &Engine::RenderSystem::UpdateConstantBuffer);
				VRenderSystem.SetMethod("void clear_materials()", &Engine::RenderSystem::ClearMaterials);
				VRenderSystem.SetMethod("void fetch_visibility(base_component@+, scene_visibility_query &out)", &Engine::RenderSystem::FetchVisibility);
				VRenderSystem.SetMethod("usize render(clock_timer@+, render_state, render_opt)", &Engine::RenderSystem::Render);
				VRenderSystem.SetMethod("bool try_instance(material@+, render_buffer_instance &out)", &Engine::RenderSystem::TryInstance);
				VRenderSystem.SetMethod("bool try_geometry(material@+, bool)", &Engine::RenderSystem::TryGeometry);
				VRenderSystem.SetMethod("bool has_category(geo_category)", &Engine::RenderSystem::HasCategory);
				VRenderSystem.SetMethod<Engine::RenderSystem, Graphics::Shader*, Graphics::Shader::Desc&, size_t>("shader@+ compile_shader(shader_desc &in, usize = 0)", &Engine::RenderSystem::CompileShader);
				VRenderSystem.SetMethod<Engine::RenderSystem, Graphics::Shader*, const Core::String&, size_t>("shader@+ compile_shader(const string &in, usize = 0)", &Engine::RenderSystem::CompileShader);
				VRenderSystem.SetMethodEx("array<element_buffer@>@ compile_buffers(const string &in, usize, usize)", &RenderSystemCompileBuffers);
				VRenderSystem.SetMethodEx("bool add_renderer(base_renderer@+)", &RenderSystemAddRenderer);
				VRenderSystem.SetMethodEx("base_renderer@+ get_renderer(uint64) const", &RenderSystemGetRenderer);
				VRenderSystem.SetMethodEx("base_renderer@+ get_renderer_by_index(usize) const", &RenderSystemGetRendererByIndex);
				VRenderSystem.SetMethodEx("usize get_renderers_count() const", &RenderSystemGetRenderersCount);
				VRenderSystem.SetMethod<Engine::RenderSystem, bool, uint64_t, size_t&>("bool get_offset(uint64, usize &out) const", &Engine::RenderSystem::GetOffset);
				VRenderSystem.SetMethod("multi_render_target_2d@+ get_mrt(target_type)", &Engine::RenderSystem::GetMRT);
				VRenderSystem.SetMethod("render_target_2d@+ get_rt(target_type)", &Engine::RenderSystem::GetRT);
				VRenderSystem.SetMethod("graphics_device@+ get_device()", &Engine::RenderSystem::GetDevice);
				VRenderSystem.SetMethod("primitive_cache@+ get_primitives()", &Engine::RenderSystem::GetPrimitives);
				VRenderSystem.SetMethod("render_constants@+ get_constants()", &Engine::RenderSystem::GetConstants);
				VRenderSystem.SetMethod("shader@+ get_basic_effect()", &Engine::RenderSystem::GetBasicEffect);
				VRenderSystem.SetMethod("scene_graph@+ get_scene()", &Engine::RenderSystem::GetScene);
				VRenderSystem.SetMethod("base_component@+ get_component()", &Engine::RenderSystem::GetComponent);
				VRenderSystem.SetMethodEx("void query_sync(uint64, overlapping_result@)", &RenderSystemQuerySync);
				VRenderSystem.SetEnumRefsEx<Engine::RenderSystem>([](Engine::RenderSystem* Base, asIScriptEngine* Engine)
				{
					for (auto* Item : Base->GetRenderers())
						Engine->GCEnumCallback(Item);
				});
				VRenderSystem.SetReleaseRefsEx<Engine::RenderSystem>([](Engine::RenderSystem* Base, asIScriptEngine*)
				{
					Base->RemoveRenderers();
				});

				RefClass VShaderCache = Engine->SetClass<Engine::ShaderCache>("shader_cache", true);
				VShaderCache.SetGcConstructor<Engine::ShaderCache, ShaderCache, Graphics::GraphicsDevice*>("shader_cache@ f()");
				VShaderCache.SetMethod("shader@+ compile(const string &in, const shader_desc &in, usize = 0)", &Engine::ShaderCache::Compile);
				VShaderCache.SetMethod("shader@+ get(const string &in)", &Engine::ShaderCache::Get);
				VShaderCache.SetMethod("string find(shader@+)", &Engine::ShaderCache::Find);
				VShaderCache.SetMethod("bool has(const string &in)", &Engine::ShaderCache::Has);
				VShaderCache.SetMethod("bool free(const string &in, shader@+ = null)", &Engine::ShaderCache::Free);
				VShaderCache.SetMethod("void clear_cache()", &Engine::ShaderCache::ClearCache);
				VShaderCache.SetEnumRefsEx<Engine::ShaderCache>([](Engine::ShaderCache* Base, asIScriptEngine* Engine)
				{
					for (auto& Item : Base->GetCaches())
						Engine->GCEnumCallback(Item.second.Shader);
				});
				VShaderCache.SetReleaseRefsEx<Engine::ShaderCache>([](Engine::ShaderCache* Base, asIScriptEngine*)
				{
					Base->ClearCache();
				});

				VPrimitiveCache.SetGcConstructor<Engine::PrimitiveCache, PrimitiveCache, Graphics::GraphicsDevice*>("primitive_cache@ f()");
				VPrimitiveCache.SetMethodEx("array<element_buffer@>@ compile(const string &in, usize, usize)", &PrimitiveCacheCompile);
				VPrimitiveCache.SetMethodEx("array<element_buffer@>@ get(const string &in) const", &PrimitiveCacheGet);
				VPrimitiveCache.SetMethod("bool has(const string &in) const", &Engine::PrimitiveCache::Has);
				VPrimitiveCache.SetMethodEx("bool free(const string &in, element_buffer@+, element_buffer@+)", &PrimitiveCacheFree);
				VPrimitiveCache.SetMethodEx("string find(element_buffer@+, element_buffer@+) const", &PrimitiveCacheFind);
				VPrimitiveCache.SetMethod("model@+ get_box_model() const", &Engine::PrimitiveCache::GetBoxModel);
				VPrimitiveCache.SetMethod("skin_model@+ get_skin_box_model() const", &Engine::PrimitiveCache::GetSkinBoxModel);
				VPrimitiveCache.SetMethod("element_buffer@+ get_quad() const", &Engine::PrimitiveCache::GetQuad);
				VPrimitiveCache.SetMethod("element_buffer@+ get_sphere(buffer_type) const", &Engine::PrimitiveCache::GetSphere);
				VPrimitiveCache.SetMethod("element_buffer@+ get_cube(buffer_type) const", &Engine::PrimitiveCache::GetCube);
				VPrimitiveCache.SetMethod("element_buffer@+ get_box(buffer_type) const", &Engine::PrimitiveCache::GetBox);
				VPrimitiveCache.SetMethod("element_buffer@+ get_skin_box(buffer_type) const", &Engine::PrimitiveCache::GetSkinBox);
				VPrimitiveCache.SetMethodEx("array<element_buffer@>@ get_sphere_buffers() const", &PrimitiveCacheGetSphereBuffers);
				VPrimitiveCache.SetMethodEx("array<element_buffer@>@ get_cube_buffers() const", &PrimitiveCacheGetCubeBuffers);
				VPrimitiveCache.SetMethodEx("array<element_buffer@>@ get_box_buffers() const", &PrimitiveCacheGetBoxBuffers);
				VPrimitiveCache.SetMethodEx("array<element_buffer@>@ get_skin_box_buffers() const", &PrimitiveCacheGetSkinBoxBuffers);
				VPrimitiveCache.SetMethod("void clear_cache()", &Engine::PrimitiveCache::ClearCache);
				VPrimitiveCache.SetEnumRefsEx<Engine::PrimitiveCache>([](Engine::PrimitiveCache* Base, asIScriptEngine* Engine)
				{
					Engine->GCEnumCallback(Base->GetSphere(Engine::BufferType::Vertex));
					Engine->GCEnumCallback(Base->GetSphere(Engine::BufferType::Index));
					Engine->GCEnumCallback(Base->GetCube(Engine::BufferType::Vertex));
					Engine->GCEnumCallback(Base->GetCube(Engine::BufferType::Index));
					Engine->GCEnumCallback(Base->GetBox(Engine::BufferType::Vertex));
					Engine->GCEnumCallback(Base->GetBox(Engine::BufferType::Index));
					Engine->GCEnumCallback(Base->GetSkinBox(Engine::BufferType::Vertex));
					Engine->GCEnumCallback(Base->GetSkinBox(Engine::BufferType::Index));
					Engine->GCEnumCallback(Base->GetQuad());

					for (auto& Item : Base->GetCaches())
					{
						Engine->GCEnumCallback(Item.second.Buffers[0]);
						Engine->GCEnumCallback(Item.second.Buffers[1]);
					}
				});
				VPrimitiveCache.SetReleaseRefsEx<Engine::PrimitiveCache>([](Engine::PrimitiveCache* Base, asIScriptEngine*)
				{
					Base->ClearCache();
				});

				VContentManager.SetFunctionDef("void load_callback(uptr@)");
				VContentManager.SetFunctionDef("void save_callback(bool)");
				VContentManager.SetGcConstructor<Engine::ContentManager, ContentManager, Graphics::GraphicsDevice*>("content_manager@ f()");
				VContentManager.SetMethod("void clear_cache()", &Engine::ContentManager::ClearCache);
				VContentManager.SetMethod("void clear_dockers()", &Engine::ContentManager::ClearDockers);
				VContentManager.SetMethod("void clear_streams()", &Engine::ContentManager::ClearStreams);
				VContentManager.SetMethod("void clear_processors()", &Engine::ContentManager::ClearProcessors);
				VContentManager.SetMethod("void clear_path(const string &in)", &Engine::ContentManager::ClearPath);
				VContentManager.SetMethod("void set_environment(const string &in)", &Engine::ContentManager::SetEnvironment);
				VContentManager.SetMethod("void set_device(graphics_device@+)", &Engine::ContentManager::SetDevice);
				VContentManager.SetMethodEx("uptr@ load(base_processor@+, const string &in, schema@+ = null)", &ContentManagerLoad);
				VContentManager.SetMethodEx("bool save(base_processor@+, const string &in, uptr@, schema@+ = null)", &ContentManagerSave);
				VContentManager.SetMethodEx("void load_async(base_processor@+, const string &in, load_callback@)", &ContentManagerLoadAsync1);
				VContentManager.SetMethodEx("void load_async(base_processor@+, const string &in, schema@+, load_callback@)", &ContentManagerLoadAsync2);
				VContentManager.SetMethodEx("void save_async(base_processor@+, const string &in, uptr@, save_callback@)", &ContentManagerSaveAsync1);
				VContentManager.SetMethodEx("void save_async(base_processor@+, const string &in, uptr@, schema@+, save_callback@)", &ContentManagerSaveAsync2);
				VContentManager.SetMethodEx("bool find_cache_info(base_processor@+, asset_cache &out, const string &in)", &ContentManagerFindCache1);
				VContentManager.SetMethodEx("bool find_cache_info(base_processor@+, asset_cache &out, uptr@)", &ContentManagerFindCache2);
				VContentManager.SetMethod<Engine::ContentManager, Engine::AssetCache*, Engine::Processor*, const Core::String&>("uptr@ find_cache(base_processor@+, const string &in)", &Engine::ContentManager::FindCache);
				VContentManager.SetMethod<Engine::ContentManager, Engine::AssetCache*, Engine::Processor*, void*>("uptr@ find_cache(base_processor@+, uptr@)", &Engine::ContentManager::FindCache);
				VContentManager.SetMethodEx("base_processor@+ add_processor(base_processor@+, uint64)", &ContentManagerAddProcessor);
				VContentManager.SetMethod<Engine::ContentManager, Engine::Processor*, uint64_t>("base_processor@+ get_processor(uint64)", &Engine::ContentManager::GetProcessor);
				VContentManager.SetMethod<Engine::ContentManager, bool, uint64_t>("bool remove_processor(uint64)", &Engine::ContentManager::RemoveProcessor);
				VContentManager.SetMethod("bool import_fs(const string &in)", &Engine::ContentManager::Import);
				VContentManager.SetMethod("bool export_fs(const string &in, const string &in, const string &in = \"\")", &Engine::ContentManager::Export);
				VContentManager.SetMethod("uptr@ try_to_cache(base_processor@+, const string &in, uptr@)", &Engine::ContentManager::TryToCache);
				VContentManager.SetMethod("bool is_busy() const", &Engine::ContentManager::IsBusy);
				VContentManager.SetMethod("graphics_device@+ get_device() const", &Engine::ContentManager::GetDevice);
				VContentManager.SetMethod("const string& get_environment() const", &Engine::ContentManager::GetEnvironment);
				VContentManager.SetEnumRefsEx<Engine::ContentManager>([](Engine::ContentManager* Base, asIScriptEngine* Engine)
				{
					for (auto& Item : Base->GetProcessors())
						Engine->GCEnumCallback(Item.second);
				});
				VContentManager.SetReleaseRefsEx<Engine::ContentManager>([](Engine::ContentManager* Base, asIScriptEngine*)
				{
					Base->ClearProcessors();
				});

				RefClass VAppData = Engine->SetClass<Engine::AppData>("app_data", true);
				VAppData.SetGcConstructor<Engine::AppData, AppData, Engine::ContentManager*, const Core::String&>("app_data@ f(content_manager@+, const string &in)");
				VAppData.SetMethod("void migrate(const string &in)", &Engine::AppData::Migrate);
				VAppData.SetMethod("void set_key(const string &in, schema@+)", &Engine::AppData::SetKey);
				VAppData.SetMethod("void set_text(const string &in, const string &in)", &Engine::AppData::SetText);
				VAppData.SetMethod("schema@+ get_key(const string &in)", &Engine::AppData::GetKey);
				VAppData.SetMethod("string get_text(const string &in)", &Engine::AppData::GetText);
				VAppData.SetMethod("bool has(const string &in)", &Engine::AppData::Has);
				VAppData.SetEnumRefsEx<Engine::AppData>([](Engine::AppData* Base, asIScriptEngine* Engine)
				{
					Engine->GCEnumCallback(Base->GetSnapshot());
				});
				VAppData.SetReleaseRefsEx<Engine::AppData>([](Engine::AppData* Base, asIScriptEngine*)
				{
					Base->~AppData();
				});

				TypeClass VSceneGraphSharedDesc = Engine->SetStruct<Engine::SceneGraph::Desc>("scene_graph_shared_desc");
				VSceneGraphSharedDesc.SetProperty<Engine::SceneGraph::Desc::Dependencies>("graphics_device@ device", &Engine::SceneGraph::Desc::Dependencies::Device);
				VSceneGraphSharedDesc.SetProperty<Engine::SceneGraph::Desc::Dependencies>("activity@ window", &Engine::SceneGraph::Desc::Dependencies::Activity);
				VSceneGraphSharedDesc.SetProperty<Engine::SceneGraph::Desc::Dependencies>("virtual_machine@ vm", &Engine::SceneGraph::Desc::Dependencies::VM);
				VSceneGraphSharedDesc.SetProperty<Engine::SceneGraph::Desc::Dependencies>("content_manager@ content", &Engine::SceneGraph::Desc::Dependencies::Content);
				VSceneGraphSharedDesc.SetProperty<Engine::SceneGraph::Desc::Dependencies>("primitive_cache@ primitives", &Engine::SceneGraph::Desc::Dependencies::Primitives);
				VSceneGraphSharedDesc.SetProperty<Engine::SceneGraph::Desc::Dependencies>("shader_cache@ shaders", &Engine::SceneGraph::Desc::Dependencies::Shaders);
				VSceneGraphSharedDesc.SetConstructor<Engine::SceneGraph::Desc::Dependencies>("void f()");
				VSceneGraphSharedDesc.SetOperatorCopyStatic(&SceneGraphSharedDescCopy);
				VSceneGraphSharedDesc.SetDestructorStatic("void f()", &SceneGraphSharedDescDestructor);

				RefClass VApplication = Engine->SetClass<Application>("application", true);
				TypeClass VSceneGraphDesc = Engine->SetStructTrivial<Engine::SceneGraph::Desc>("scene_graph_desc");
				VSceneGraphDesc.SetProperty<Engine::SceneGraph::Desc>("scene_graph_shared_desc shared", &Engine::SceneGraph::Desc::Shared);
				VSceneGraphDesc.SetProperty<Engine::SceneGraph::Desc>("physics_simulator_desc simulator", &Engine::SceneGraph::Desc::Simulator);
				VSceneGraphDesc.SetProperty<Engine::SceneGraph::Desc>("usize start_materials", &Engine::SceneGraph::Desc::StartMaterials);
				VSceneGraphDesc.SetProperty<Engine::SceneGraph::Desc>("usize start_entities", &Engine::SceneGraph::Desc::StartEntities);
				VSceneGraphDesc.SetProperty<Engine::SceneGraph::Desc>("usize start_components", &Engine::SceneGraph::Desc::StartComponents);
				VSceneGraphDesc.SetProperty<Engine::SceneGraph::Desc>("usize grow_margin", &Engine::SceneGraph::Desc::GrowMargin);
				VSceneGraphDesc.SetProperty<Engine::SceneGraph::Desc>("usize max_updates", &Engine::SceneGraph::Desc::MaxUpdates);
				VSceneGraphDesc.SetProperty<Engine::SceneGraph::Desc>("usize points_size", &Engine::SceneGraph::Desc::PointsSize);
				VSceneGraphDesc.SetProperty<Engine::SceneGraph::Desc>("usize points_max", &Engine::SceneGraph::Desc::PointsMax);
				VSceneGraphDesc.SetProperty<Engine::SceneGraph::Desc>("usize spots_size", &Engine::SceneGraph::Desc::SpotsSize);
				VSceneGraphDesc.SetProperty<Engine::SceneGraph::Desc>("usize spots_max", &Engine::SceneGraph::Desc::SpotsMax);
				VSceneGraphDesc.SetProperty<Engine::SceneGraph::Desc>("usize lines_size", &Engine::SceneGraph::Desc::LinesSize);
				VSceneGraphDesc.SetProperty<Engine::SceneGraph::Desc>("usize lines_max", &Engine::SceneGraph::Desc::LinesMax);
				VSceneGraphDesc.SetProperty<Engine::SceneGraph::Desc>("usize voxels_size", &Engine::SceneGraph::Desc::VoxelsSize);
				VSceneGraphDesc.SetProperty<Engine::SceneGraph::Desc>("usize voxels_max", &Engine::SceneGraph::Desc::VoxelsMax);
				VSceneGraphDesc.SetProperty<Engine::SceneGraph::Desc>("usize voxels_mips", &Engine::SceneGraph::Desc::VoxelsMips);
				VSceneGraphDesc.SetProperty<Engine::SceneGraph::Desc>("double grow_rate", &Engine::SceneGraph::Desc::GrowRate);
				VSceneGraphDesc.SetProperty<Engine::SceneGraph::Desc>("float render_quality", &Engine::SceneGraph::Desc::RenderQuality);
				VSceneGraphDesc.SetProperty<Engine::SceneGraph::Desc>("bool enable_hdr", &Engine::SceneGraph::Desc::EnableHDR);
				VSceneGraphDesc.SetProperty<Engine::SceneGraph::Desc>("bool mutations", &Engine::SceneGraph::Desc::Mutations);
				VSceneGraphDesc.SetConstructor<Engine::SceneGraph::Desc>("void f()");
				VSceneGraphDesc.SetMethodStatic("scene_graph_desc get(application@+)", &Engine::SceneGraph::Desc::Get);

				TypeClass VSceneGraphStatistics = Engine->SetPod<Engine::SceneGraph::SgStatistics>("scene_graph_statistics");
				VSceneGraphStatistics.SetProperty<Engine::SceneGraph::SgStatistics>("usize batching", &Engine::SceneGraph::SgStatistics::Batching);
				VSceneGraphStatistics.SetProperty<Engine::SceneGraph::SgStatistics>("usize sorting", &Engine::SceneGraph::SgStatistics::Sorting);
				VSceneGraphStatistics.SetProperty<Engine::SceneGraph::SgStatistics>("usize instances", &Engine::SceneGraph::SgStatistics::Instances);
				VSceneGraphStatistics.SetProperty<Engine::SceneGraph::SgStatistics>("usize draw_calls", &Engine::SceneGraph::SgStatistics::DrawCalls);

				VSceneGraph.SetGcConstructor<Engine::SceneGraph, SceneGraph, const Engine::SceneGraph::Desc&>("scene_graph@ f(const scene_graph_desc &in)");
				VSceneGraph.SetFunctionDef("bool ray_test_callback(base_component@+, const vector3 &in)");
				VSceneGraph.SetFunctionDef("void transaction_callback()");
				VSceneGraph.SetFunctionDef("void event_callback(const string &in, schema@+)");
				VSceneGraph.SetFunctionDef("void match_callback(const bounding &in)");
				VSceneGraph.SetFunctionDef("void resource_callback(uptr@)");
				VSceneGraph.SetProperty("scene_graph_statistics statistics", &Engine::SceneGraph::Statistics);
				VSceneGraph.SetMethod("void configure(const scene_graph_desc &in)", &Engine::SceneGraph::Configure);
				VSceneGraph.SetMethod("void actualize()", &Engine::SceneGraph::Actualize);
				VSceneGraph.SetMethod("void resize_buffers()", &Engine::SceneGraph::ResizeBuffers);
				VSceneGraph.SetMethod("void submit()", &Engine::SceneGraph::Submit);
				VSceneGraph.SetMethod("void dispatch(clock_timer@+)", &Engine::SceneGraph::Dispatch);
				VSceneGraph.SetMethod("void publish(clock_timer@+)", &Engine::SceneGraph::Publish);
				VSceneGraph.SetMethod("void delete_material(material@+)", &Engine::SceneGraph::DeleteMaterial);
				VSceneGraph.SetMethod("void remove_entity(scene_entity@+)", &Engine::SceneGraph::RemoveEntity);
				VSceneGraph.SetMethod("void delete_entity(scene_entity@+)", &Engine::SceneGraph::DeleteEntity);
				VSceneGraph.SetMethod("void set_camera(scene_entity@+)", &Engine::SceneGraph::SetCamera);
				VSceneGraph.SetMethodEx("void ray_test(uint64, const ray &in, ray_test_callback@)", &SceneGraphRayTest);
				VSceneGraph.SetMethod("void script_hook(const string &in = \"main\")", &Engine::SceneGraph::ScriptHook);
				VSceneGraph.SetMethod("void set_active(bool)", &Engine::SceneGraph::SetActive);
				VSceneGraph.SetMethod("void set_mrt(target_type, bool)", &Engine::SceneGraph::SetMRT);
				VSceneGraph.SetMethod("void set_rt(target_type, bool)", &Engine::SceneGraph::SetRT);
				VSceneGraph.SetMethod("void swap_mrt(target_type, multi_render_target_2d@+)", &Engine::SceneGraph::SwapMRT);
				VSceneGraph.SetMethod("void swap_rt(target_type, render_target_2d@+)", &Engine::SceneGraph::SwapRT);
				VSceneGraph.SetMethod("void clear_mrt(target_type, bool, bool)", &Engine::SceneGraph::ClearMRT);
				VSceneGraph.SetMethod("void clear_rt(target_type, bool, bool)", &Engine::SceneGraph::ClearRT);
				VSceneGraph.SetMethodEx("void mutate(scene_entity@+, scene_entity@+, const string &in)", &SceneGraphMutate1);
				VSceneGraph.SetMethodEx("void mutate(scene_entity@+, const string &in)", &SceneGraphMutate2);
				VSceneGraph.SetMethodEx("void mutate(base_component@+, const string &in)", &SceneGraphMutate3);
				VSceneGraph.SetMethodEx("void mutate(material@+, const string &in)", &SceneGraphMutate4);
				VSceneGraph.SetMethodEx("void transaction(transaction_callback@)", &SceneGraphTransaction);
				VSceneGraph.SetMethod("void clear_culling()", &Engine::SceneGraph::ClearCulling);
				VSceneGraph.SetMethod("void reserve_materials(usize)", &Engine::SceneGraph::ReserveMaterials);
				VSceneGraph.SetMethod("void reserve_entities(usize)", &Engine::SceneGraph::ReserveEntities);
				VSceneGraph.SetMethod("void reserve_components(uint64, usize)", &Engine::SceneGraph::ReserveComponents);
				VSceneGraph.SetMethodEx("bool push_event(const string &in, schema@+, bool)", &SceneGraphPushEvent1);
				VSceneGraph.SetMethodEx("bool push_event(const string &in, schema@+, base_component@+)", &SceneGraphPushEvent2);
				VSceneGraph.SetMethodEx("bool push_event(const string &in, schema@+, scene_entity@+)", &SceneGraphPushEvent3);
				VSceneGraph.SetMethodEx("uptr@ set_listener(const string &in, event_callback@+)", &SceneGraphSetListener);
				VSceneGraph.SetMethod("bool clear_listener(const string &in, uptr@)", &Engine::SceneGraph::ClearListener);
				VSceneGraph.SetMethod("material@+ get_invalid_material()", &Engine::SceneGraph::GetInvalidMaterial);
				VSceneGraph.SetMethod<Engine::SceneGraph, bool, Engine::Material*>("bool add_material(material@+)", &Engine::SceneGraph::AddMaterial);
				VSceneGraph.SetMethod<Engine::SceneGraph, Engine::Material*>("material@+ add_material()", &Engine::SceneGraph::AddMaterial);
				VSceneGraph.SetMethodEx("void load_resource(uint64, base_component@+, const string &in, schema@+, resource_callback@)", &SceneGraphLoadResource);
				VSceneGraph.SetMethod<Engine::SceneGraph, Core::String, uint64_t, void*>("string find_resource_id(uint64, uptr@)", &Engine::SceneGraph::FindResourceId);
				VSceneGraph.SetMethod("material@+ clone_material(material@+)", &Engine::SceneGraph::CloneMaterial);
				VSceneGraph.SetMethod("scene_entity@+ get_entity(usize) const", &Engine::SceneGraph::GetEntity);
				VSceneGraph.SetMethod("scene_entity@+ get_last_entity() const", &Engine::SceneGraph::GetLastEntity);
				VSceneGraph.SetMethod("scene_entity@+ get_camera_entity() const", &Engine::SceneGraph::GetCameraEntity);
				VSceneGraph.SetMethod("base_component@+ get_component(uint64, usize) const", &Engine::SceneGraph::GetComponent);
				VSceneGraph.SetMethod("base_component@+ get_camera() const", &Engine::SceneGraph::GetCamera);
				VSceneGraph.SetMethod("render_system@+ get_renderer() const", &Engine::SceneGraph::GetRenderer);
				VSceneGraph.SetMethod("viewer_t get_camera_viewer() const", &Engine::SceneGraph::GetCameraViewer);
				VSceneGraph.SetMethod<Engine::SceneGraph, Engine::Material*, const Core::String&>("material@+ get_material(const string &in) const", &Engine::SceneGraph::GetMaterial);
				VSceneGraph.SetMethod<Engine::SceneGraph, Engine::Material*, size_t>("material@+ get_material(usize) const", &Engine::SceneGraph::GetMaterial);
				VSceneGraph.SetMethod<Engine::SceneGraph, Engine::SparseIndex&, uint64_t>("sparse_index& get_storage(uint64) const", &Engine::SceneGraph::GetStorage);
				VSceneGraph.SetMethodEx("array<base_component@>@ get_components(uint64) const", &SceneGraphGetComponents);
				VSceneGraph.SetMethodEx("array<base_component@>@ get_actors(actor_type) const", &SceneGraphGetActors);
				VSceneGraph.SetMethod("render_target_2d_desc get_desc_rt() const", &Engine::SceneGraph::GetDescRT);
				VSceneGraph.SetMethod("multi_render_target_2d_desc get_desc_mrt() const", &Engine::SceneGraph::GetDescMRT);
				VSceneGraph.SetMethod("surface_format get_format_mrt(uint32) const", &Engine::SceneGraph::GetFormatMRT);
				VSceneGraph.SetMethodEx("array<scene_entity@>@ clone_entity_as_array(scene_entity@+)", &SceneGraphCloneEntityAsArray);
				VSceneGraph.SetMethodEx("array<scene_entity@>@ query_by_parent(scene_entity@+) const", &SceneGraphQueryByParent);
				VSceneGraph.SetMethodEx("array<scene_entity@>@ query_by_name(const string &in) const", &SceneGraphQueryByName);
				VSceneGraph.SetMethodEx("array<base_component@>@ query_by_position(uint64, const vector3 &in, float) const", &SceneGraphQueryByPosition);
				VSceneGraph.SetMethodEx("array<base_component@>@ query_by_area(uint64, const vector3 &in, const vector3 &in) const", &SceneGraphQueryByArea);
				VSceneGraph.SetMethodEx("array<base_component@>@ query_by_ray(uint64, const ray &in) const", &SceneGraphQueryByRay);
				VSceneGraph.SetMethodEx("array<base_component@>@ query_by_match(uint64, match_callback@) const", &SceneGraphQueryByMatch);
				VSceneGraph.SetMethod("string as_resource_path(const string &in) const", &Engine::SceneGraph::AsResourcePath);
				VSceneGraph.SetMethod<Engine::SceneGraph, Engine::Entity*>("scene_entity@+ add_entity()", &Engine::SceneGraph::AddEntity);
				VSceneGraph.SetMethod("scene_entity@+ clone_entity(scene_entity@+)", &Engine::SceneGraph::CloneEntity);
				VSceneGraph.SetMethod<Engine::SceneGraph, bool, Engine::Entity*>("bool add_entity(scene_entity@+)", &Engine::SceneGraph::AddEntity);
				VSceneGraph.SetMethod("bool is_active() const", &Engine::SceneGraph::IsActive);
				VSceneGraph.SetMethod("bool is_left_handed() const", &Engine::SceneGraph::IsLeftHanded);
				VSceneGraph.SetMethod("bool is_indexed() const", &Engine::SceneGraph::IsIndexed);
				VSceneGraph.SetMethod("bool is_busy() const", &Engine::SceneGraph::IsBusy);
				VSceneGraph.SetMethod("usize get_materials_count() const", &Engine::SceneGraph::GetMaterialsCount);
				VSceneGraph.SetMethod("usize get_entities_count() const", &Engine::SceneGraph::GetEntitiesCount);
				VSceneGraph.SetMethod("usize get_components_count(uint64) const", &Engine::SceneGraph::GetComponentsCount);
				VSceneGraph.SetMethod<Engine::SceneGraph, bool, Engine::Entity*>("bool has_entity(scene_entity@+) const", &Engine::SceneGraph::HasEntity);
				VSceneGraph.SetMethod<Engine::SceneGraph, bool, size_t>("bool has_entity(usize) const", &Engine::SceneGraph::HasEntity);
				VSceneGraph.SetMethod("multi_render_target_2d@+ get_mrt() const", &Engine::SceneGraph::GetMRT);
				VSceneGraph.SetMethod("render_target_2d@+ get_rt() const", &Engine::SceneGraph::GetRT);
				VSceneGraph.SetMethod("element_buffer@+ get_structure() const", &Engine::SceneGraph::GetStructure);
				VSceneGraph.SetMethod("graphics_device@+ get_device() const", &Engine::SceneGraph::GetDevice);
				VSceneGraph.SetMethod("physics_simulator@+ get_simulator() const", &Engine::SceneGraph::GetSimulator);
				VSceneGraph.SetMethod("activity@+ get_activity() const", &Engine::SceneGraph::GetActivity);
				VSceneGraph.SetMethod("render_constants@+ get_constants() const", &Engine::SceneGraph::GetConstants);
				VSceneGraph.SetMethod("shader_cache@+ get_shaders() const", &Engine::SceneGraph::GetShaders);
				VSceneGraph.SetMethod("primitive_cache@+ get_primitives() const", &Engine::SceneGraph::GetPrimitives);
				VSceneGraph.SetMethod("scene_graph_desc& get_conf()", &Engine::SceneGraph::GetConf);
				VSceneGraph.SetEnumRefsEx<Engine::SceneGraph>([](Engine::SceneGraph* Base, asIScriptEngine* Engine)
				{
					auto& Conf = Base->GetConf();
					Engine->GCEnumCallback(Conf.Shared.Shaders);
					Engine->GCEnumCallback(Conf.Shared.Primitives);
					Engine->GCEnumCallback(Conf.Shared.Content);
					Engine->GCEnumCallback(Conf.Shared.Device);
					Engine->GCEnumCallback(Conf.Shared.Activity);
					Engine->GCEnumCallback(Conf.Shared.VM);

					size_t Materials = Base->GetMaterialsCount();
					for (size_t i = 0; i < Materials; i++)
						Engine->GCEnumCallback(Base->GetMaterial(i));

					size_t Entities = Base->GetEntitiesCount();
					for (size_t i = 0; i < Entities; i++)
						Engine->GCEnumCallback(Base->GetEntity(i));

					for (auto& Item : Base->GetRegistry())
					{
						for (auto* Next : Item.second->Data)
							Engine->GCEnumCallback(Next);
					}
				});
				VSceneGraph.SetReleaseRefsEx<Engine::SceneGraph>([](Engine::SceneGraph* Base, asIScriptEngine*) { });

				TypeClass VApplicationFrameInfo = Engine->SetStructTrivial<Application::Desc::FramesInfo>("application_frame_info");
				VApplicationFrameInfo.SetProperty<Application::Desc::FramesInfo>("float stable", &Application::Desc::FramesInfo::Stable);
				VApplicationFrameInfo.SetProperty<Application::Desc::FramesInfo>("float limit", &Application::Desc::FramesInfo::Limit);
				VApplicationFrameInfo.SetConstructor<Application::Desc::FramesInfo>("void f()");

				TypeClass VApplicationCacheInfo = Engine->SetStruct<Application::CacheInfo>("application_cache_info");
				VApplicationCacheInfo.SetProperty<Application::CacheInfo>("shader_cache@ shaders", &Application::CacheInfo::Shaders);
				VApplicationCacheInfo.SetProperty<Application::CacheInfo>("primitive_cache@ primitives", &Application::CacheInfo::Primitives);
				VApplicationCacheInfo.SetConstructor<Application::CacheInfo>("void f()");
				VApplicationCacheInfo.SetOperatorCopyStatic(&ApplicationCacheInfoCopy);
				VApplicationCacheInfo.SetDestructorStatic("void f()", &ApplicationCacheInfoDestructor);

				TypeClass VApplicationDesc = Engine->SetStructTrivial<Application::Desc>("application_desc");
				VApplicationDesc.SetProperty<Application::Desc>("application_frame_info framerate", &Application::Desc::Framerate);
				VApplicationDesc.SetProperty<Application::Desc>("graphics_device_desc graphics", &Application::Desc::GraphicsDevice);
				VApplicationDesc.SetProperty<Application::Desc>("activity_desc window", &Application::Desc::Activity);
				VApplicationDesc.SetProperty<Application::Desc>("string preferences", &Application::Desc::Preferences);
				VApplicationDesc.SetProperty<Application::Desc>("string environment", &Application::Desc::Environment);
				VApplicationDesc.SetProperty<Application::Desc>("string directory", &Application::Desc::Directory);
				VApplicationDesc.SetProperty<Application::Desc>("usize stack", &Application::Desc::Stack);
				VApplicationDesc.SetProperty<Application::Desc>("usize polling_timeout", &Application::Desc::PollingTimeout);
				VApplicationDesc.SetProperty<Application::Desc>("usize polling_events", &Application::Desc::PollingEvents);
				VApplicationDesc.SetProperty<Application::Desc>("usize coroutines", &Application::Desc::Coroutines);
				VApplicationDesc.SetProperty<Application::Desc>("usize threads", &Application::Desc::Threads);
				VApplicationDesc.SetProperty<Application::Desc>("usize usage", &Application::Desc::Usage);
				VApplicationDesc.SetProperty<Application::Desc>("bool daemon", &Application::Desc::Daemon);
				VApplicationDesc.SetProperty<Application::Desc>("bool parallel", &Application::Desc::Parallel);
				VApplicationDesc.SetProperty<Application::Desc>("bool cursor", &Application::Desc::Cursor);
				VApplicationDesc.SetConstructor<Application::Desc>("void f()");

				VApplication.SetFunctionDef("void script_hook_callback()");
				VApplication.SetFunctionDef("void key_event_callback(key_code, key_mod, int32, int32, bool)");
				VApplication.SetFunctionDef("void input_event_callback(const string &in)");
				VApplication.SetFunctionDef("void wheel_event_callback(int, int, bool)");
				VApplication.SetFunctionDef("void window_event_callback(window_state, int, int)");
				VApplication.SetFunctionDef("void close_event_callback()");
				VApplication.SetFunctionDef("void compose_event_callback()");
				VApplication.SetFunctionDef("void dispatch_callback(clock_timer@+)");
				VApplication.SetFunctionDef("void publish_callback(clock_timer@+)");
				VApplication.SetFunctionDef("void initialize_callback()");
				VApplication.SetFunctionDef("gui_context@ fetch_gui_callback()");
				VApplication.SetProperty<Engine::Application>("application_cache_info cache", &Engine::Application::Cache);
				VApplication.SetProperty<Engine::Application>("audio_device@ audio", &Engine::Application::Audio);
				VApplication.SetProperty<Engine::Application>("graphics_device@ renderer", &Engine::Application::Renderer);
				VApplication.SetProperty<Engine::Application>("activity@ window", &Engine::Application::Activity);
				VApplication.SetProperty<Engine::Application>("virtual_machine@ vm", &Engine::Application::VM);
				VApplication.SetProperty<Engine::Application>("render_constants@ constants", &Engine::Application::Constants);
				VApplication.SetProperty<Engine::Application>("content_manager@ content", &Engine::Application::Content);
				VApplication.SetProperty<Engine::Application>("app_data@ database", &Engine::Application::Database);
				VApplication.SetProperty<Engine::Application>("scene_graph@ scene", &Engine::Application::Scene);
				VApplication.SetProperty<Engine::Application>("application_desc control", &Engine::Application::Control);
				VApplication.SetProperty<Application>("script_hook_callback@ script_hook", &Application::OnScriptHook);
				VApplication.SetProperty<Application>("key_event_callback@ key_event", &Application::OnKeyEvent);
				VApplication.SetProperty<Application>("input_event_callback@ input_event", &Application::OnInputEvent);
				VApplication.SetProperty<Application>("wheel_event_callback@ wheel_event", &Application::OnWheelEvent);
				VApplication.SetProperty<Application>("window_event_callback@ window_event", &Application::OnWindowEvent);
				VApplication.SetProperty<Application>("close_event_callback@ close_event", &Application::OnCloseEvent);
				VApplication.SetProperty<Application>("compose_event_callback@ compose_event", &Application::OnComposeEvent);
				VApplication.SetProperty<Application>("dispatch_callback@ dispatch", &Application::OnDispatch);
				VApplication.SetProperty<Application>("publish_callback@ publish", &Application::OnPublish);
				VApplication.SetProperty<Application>("initialize_callback@ initialize", &Application::OnInitialize);
				VApplication.SetProperty<Application>("fetch_gui_callback@ fetch_gui", &Application::OnGetGUI);
				VApplication.SetGcConstructor<Application, ApplicationName, Application::Desc&>("application@ f(application_desc &in)");
				VApplication.SetMethod("int start()", &Engine::Application::Start);
				VApplication.SetMethod("int restart()", &Engine::Application::Restart);
				VApplication.SetMethod("void stop(int = 0)", &Engine::Application::Stop);
				VApplication.SetMethod("application_state get_state() const", &Engine::Application::GetState);
				VApplication.SetMethodStatic("application@+ get()", &Engine::Application::Get);
				VApplication.SetMethodStatic("bool wants_restart(int)", &Application::WantsRestart);
				VApplication.SetEnumRefsEx<Application>([](Application* Base, asIScriptEngine* Engine)
				{
					Engine->GCEnumCallback(Base->Audio);
					Engine->GCEnumCallback(Base->Renderer);
					Engine->GCEnumCallback(Base->Activity);
					Engine->GCEnumCallback(Base->VM);
					Engine->GCEnumCallback(Base->Content);
					Engine->GCEnumCallback(Base->Database);
					Engine->GCEnumCallback(Base->Scene);
					Engine->GCEnumCallback(Base->OnScriptHook);
					Engine->GCEnumCallback(Base->OnKeyEvent);
					Engine->GCEnumCallback(Base->OnInputEvent);
					Engine->GCEnumCallback(Base->OnWheelEvent);
					Engine->GCEnumCallback(Base->OnWindowEvent);
					Engine->GCEnumCallback(Base->OnCloseEvent);
					Engine->GCEnumCallback(Base->OnComposeEvent);
					Engine->GCEnumCallback(Base->OnDispatch);
					Engine->GCEnumCallback(Base->OnPublish);
					Engine->GCEnumCallback(Base->OnInitialize);
					Engine->GCEnumCallback(Base->OnGetGUI);
				});
				VApplication.SetReleaseRefsEx<Application>([](Application* Base, asIScriptEngine*)
				{
					Base->~Application();
				});

				return true;
#else
				VI_ASSERT(false, false, "<engine> is not loaded");
				return false;
#endif
			}
			bool Registry::LoadComponents(VirtualMachine* Engine)
			{
#ifdef VI_HAS_BINDINGS
				VI_ASSERT(Engine != nullptr, false, "manager should be set");
				Engine->SetFunctionDef("void component_resource_event()");

				RefClass VSoftBody = Engine->SetClass<Engine::Components::SoftBody>("soft_body_component", false);
				VSoftBody.SetProperty<Engine::Components::SoftBody>("vector2 texcoord", &Engine::Components::SoftBody::TexCoord);
				VSoftBody.SetProperty<Engine::Components::SoftBody>("bool kinematic", &Engine::Components::SoftBody::Kinematic);
				VSoftBody.SetProperty<Engine::Components::SoftBody>("bool manage", &Engine::Components::SoftBody::Manage);
				VSoftBody.SetMethodEx("void load(const string &in, float = 0.0f, component_resource_event@ = null)", &ComponentsSoftBodyLoad);
				VSoftBody.SetMethod<Engine::Components::SoftBody, void, Compute::HullShape*, float>("void load(physics_hull_shape@+, float = 0.0f)", &Engine::Components::SoftBody::Load);
				VSoftBody.SetMethod("void load_ellipsoid(const physics_softbody_desc_cv_sconvex &in, float)", &Engine::Components::SoftBody::LoadEllipsoid);
				VSoftBody.SetMethod("void load_patch(const physics_softbody_desc_cv_spatch &in, float)", &Engine::Components::SoftBody::LoadPatch);
				VSoftBody.SetMethod("void load_rope(const physics_softbody_desc_cv_srope &in, float)", &Engine::Components::SoftBody::LoadRope);
				VSoftBody.SetMethod("void fill(graphics_device@+, element_buffer@+, element_buffer@+)", &Engine::Components::SoftBody::Fill);
				VSoftBody.SetMethod("void regenerate()", &Engine::Components::SoftBody::Regenerate);
				VSoftBody.SetMethod("void clear()", &Engine::Components::SoftBody::Clear);
				VSoftBody.SetMethod<Engine::Components::SoftBody, void, const Compute::Vector3&, const Compute::Vector3&, const Compute::Vector3&>("void set_transform(const vector3 &in, const vector3 &in, const vector3 &in)", &Engine::Components::SoftBody::SetTransform);
				VSoftBody.SetMethod<Engine::Components::SoftBody, void, bool>("void set_transform(bool)", &Engine::Components::SoftBody::SetTransform);
				VSoftBody.SetMethod("physics_softbody@+ get_body() const", &Engine::Components::SoftBody::GetBody);
				PopulateDrawableInterface<Engine::Components::SoftBody, Engine::Entity*>(VSoftBody, "soft_body_component@+ f(scene_entity@+)");

				RefClass VRigidBody = Engine->SetClass<Engine::Components::RigidBody>("rigid_body_component", false);
				VRigidBody.SetProperty<Engine::Components::RigidBody>("bool kinematic", &Engine::Components::RigidBody::Kinematic);
				VRigidBody.SetProperty<Engine::Components::RigidBody>("bool manage", &Engine::Components::RigidBody::Manage);
				VRigidBody.SetMethodEx("void load(const string &in, float, float = 0.0f, component_resource_event@ = null)", &ComponentsRigidBodyLoad);
				VRigidBody.SetMethod<Engine::Components::RigidBody, void, btCollisionShape*, float, float>("void load(uptr@, float, float = 0.0f)", &Engine::Components::RigidBody::Load);
				VRigidBody.SetMethod("void clear()", &Engine::Components::RigidBody::Clear);
				VRigidBody.SetMethod("void set_mass(float)", &Engine::Components::RigidBody::SetMass);
				VRigidBody.SetMethod<Engine::Components::RigidBody, void, const Compute::Vector3&, const Compute::Vector3&, const Compute::Vector3&>("void set_transform(const vector3 &in, const vector3 &in, const vector3 &in)", &Engine::Components::RigidBody::SetTransform);
				VRigidBody.SetMethod<Engine::Components::RigidBody, void, bool>("void set_transform(bool)", &Engine::Components::RigidBody::SetTransform);
				VRigidBody.SetMethod("physics_rigidbody@+ get_body() const", &Engine::Components::RigidBody::GetBody);
				PopulateComponentInterface<Engine::Components::RigidBody, Engine::Entity*>(VRigidBody, "rigid_body_component@+ f(scene_entity@+)");

				RefClass VSliderConstraint = Engine->SetClass<Engine::Components::SliderConstraint>("slider_constraint_component", false);
				VSliderConstraint.SetMethod("void load(scene_entity@+, bool, bool)", &Engine::Components::SliderConstraint::Load);
				VSliderConstraint.SetMethod("void clear()", &Engine::Components::SliderConstraint::Clear);
				VSliderConstraint.SetMethod("physics_rigidbody@+ get_constraint() const", &Engine::Components::SliderConstraint::GetConstraint);
				VSliderConstraint.SetMethod("scene_entity@+ get_connection() const", &Engine::Components::SliderConstraint::GetConnection);
				PopulateComponentInterface<Engine::Components::SliderConstraint, Engine::Entity*>(VSliderConstraint, "slider_constraint_component@+ f(scene_entity@+)");

				RefClass VAcceleration = Engine->SetClass<Engine::Components::Acceleration>("acceleration_component", false);
				VAcceleration.SetProperty<Engine::Components::Acceleration>("vector3 amplitude_velocity", &Engine::Components::Acceleration::AmplitudeVelocity);
				VAcceleration.SetProperty<Engine::Components::Acceleration>("vector3 amplitude_torque", &Engine::Components::Acceleration::AmplitudeTorque);
				VAcceleration.SetProperty<Engine::Components::Acceleration>("vector3 constant_velocity", &Engine::Components::Acceleration::ConstantVelocity);
				VAcceleration.SetProperty<Engine::Components::Acceleration>("vector3 constant_torque", &Engine::Components::Acceleration::ConstantTorque);
				VAcceleration.SetProperty<Engine::Components::Acceleration>("vector3 constant_center", &Engine::Components::Acceleration::ConstantCenter);
				VAcceleration.SetProperty<Engine::Components::Acceleration>("bool kinematic", &Engine::Components::Acceleration::Kinematic);
				VAcceleration.SetMethod("rigid_body_component@+ get_body() const", &Engine::Components::Acceleration::GetBody);
				PopulateComponentInterface<Engine::Components::Acceleration, Engine::Entity*>(VAcceleration, "acceleration_component@+ f(scene_entity@+)");

				RefClass VModel = Engine->SetClass<Engine::Components::Model>("model_component", false);
				VModel.SetProperty<Engine::Components::Model>("vector2 texcoord", &Engine::Components::Model::TexCoord);
				VModel.SetMethod("void set_drawable(model@+)", &Engine::Components::Model::SetDrawable);
				VModel.SetMethod("void set_material_for(const string &in, material@+)", &Engine::Components::Model::SetMaterialFor);
				VModel.SetMethod("model@+ get_drawable() const", &Engine::Components::Model::GetDrawable);
				VModel.SetMethod("material@+ get_material_for(const string &in)", &Engine::Components::Model::GetMaterialFor);
				PopulateDrawableInterface<Engine::Components::Model, Engine::Entity*>(VModel, "model_component@+ f(scene_entity@+)");

				RefClass VSkin = Engine->SetClass<Engine::Components::Skin>("skin_component", false);
				VSkin.SetProperty<Engine::Components::Skin>("vector2 texcoord", &Engine::Components::Skin::TexCoord);
				VSkin.SetProperty<Engine::Components::Skin>("pose_buffer skeleton", &Engine::Components::Skin::Skeleton);
				VSkin.SetMethod("void set_drawable(skin_model@+)", &Engine::Components::Skin::SetDrawable);
				VSkin.SetMethod("void set_material_for(const string &in, material@+)", &Engine::Components::Skin::SetMaterialFor);
				VSkin.SetMethod("skin_model@+ get_drawable() const", &Engine::Components::Skin::GetDrawable);
				VSkin.SetMethod("material@+ get_material_for(const string &in)", &Engine::Components::Skin::GetMaterialFor);
				PopulateDrawableInterface<Engine::Components::Skin, Engine::Entity*>(VSkin, "skin_component@+ f(scene_entity@+)");

				RefClass VEmitter = Engine->SetClass<Engine::Components::Emitter>("emitter_component", false);
				VEmitter.SetProperty<Engine::Components::Emitter>("vector3 min", &Engine::Components::Emitter::Min);
				VEmitter.SetProperty<Engine::Components::Emitter>("vector3 max", &Engine::Components::Emitter::Max);
				VEmitter.SetProperty<Engine::Components::Emitter>("bool connected", &Engine::Components::Emitter::Connected);
				VEmitter.SetProperty<Engine::Components::Emitter>("bool quad_based", &Engine::Components::Emitter::QuadBased);
				VEmitter.SetMethod("instance_buffer@+ get_buffer() const", &Engine::Components::Emitter::GetBuffer);
				PopulateDrawableInterface<Engine::Components::Emitter, Engine::Entity*>(VEmitter, "emitter_component@+ f(scene_entity@+)");

				RefClass VDecal = Engine->SetClass<Engine::Components::Decal>("decal_component", false);
				VDecal.SetProperty<Engine::Components::Decal>("vector2 texcoord", &Engine::Components::Decal::TexCoord);
				PopulateDrawableInterface<Engine::Components::Decal, Engine::Entity*>(VDecal, "decal_component@+ f(scene_entity@+)");

				RefClass VSkinAnimator = Engine->SetClass<Engine::Components::SkinAnimator>("skin_animator_component", false);
				VSkinAnimator.SetProperty<Engine::Components::SkinAnimator>("animator_state state", &Engine::Components::SkinAnimator::State);
				VSkinAnimator.SetMethod("void set_animation(skin_animation@+)", &Engine::Components::SkinAnimator::SetAnimation);
				VSkinAnimator.SetMethod("void play(int64 = -1, int64 = -1)", &Engine::Components::SkinAnimator::Play);
				VSkinAnimator.SetMethod("void pause()", &Engine::Components::SkinAnimator::Pause);
				VSkinAnimator.SetMethod("void stop()", &Engine::Components::SkinAnimator::Stop);
				VSkinAnimator.SetMethod<Engine::Components::SkinAnimator, bool, int64_t>("bool is_exists(int64) const", &Engine::Components::SkinAnimator::IsExists);
				VSkinAnimator.SetMethod<Engine::Components::SkinAnimator, bool, int64_t, int64_t>("bool is_exists(int64, int64) const", &Engine::Components::SkinAnimator::IsExists);
				VSkinAnimator.SetMethod("skin_animator_key& get_frame(int64, int64) const", &Engine::Components::SkinAnimator::GetFrame);
				VSkinAnimator.SetMethod("skin_component@+ get_skin() const", &Engine::Components::SkinAnimator::GetSkin);
				VSkinAnimator.SetMethod("skin_animation@+ get_animation() const", &Engine::Components::SkinAnimator::GetAnimation);
				VSkinAnimator.SetMethod("string get_path() const", &Engine::Components::SkinAnimator::GetPath);
				VSkinAnimator.SetMethod("int64 get_clip_by_name(const string &in) const", &Engine::Components::SkinAnimator::GetClipByName);
				VSkinAnimator.SetMethod("usize get_clips_count() const", &Engine::Components::SkinAnimator::GetClipsCount);
				PopulateComponentInterface<Engine::Components::SkinAnimator, Engine::Entity*>(VSkinAnimator, "skin_animator_component@+ f(scene_entity@+)");

				RefClass VKeyAnimator = Engine->SetClass<Engine::Components::KeyAnimator>("key_animator_component", false);
				VKeyAnimator.SetProperty<Engine::Components::KeyAnimator>("animator_key offset_pose", &Engine::Components::KeyAnimator::Offset);
				VKeyAnimator.SetProperty<Engine::Components::KeyAnimator>("animator_key default_pose", &Engine::Components::KeyAnimator::Default);
				VKeyAnimator.SetProperty<Engine::Components::KeyAnimator>("animator_state state", &Engine::Components::KeyAnimator::State);
				VKeyAnimator.SetMethodEx("void load_animation(const string &in, component_resource_event@ = null)", &ComponentsKeyAnimatorLoadAnimation);
				VKeyAnimator.SetMethod("void clear_animation()", &Engine::Components::KeyAnimator::ClearAnimation);
				VKeyAnimator.SetMethod("void play(int64 = -1, int64 = -1)", &Engine::Components::KeyAnimator::Play);
				VKeyAnimator.SetMethod("void pause()", &Engine::Components::KeyAnimator::Pause);
				VKeyAnimator.SetMethod("void stop()", &Engine::Components::KeyAnimator::Stop);
				VKeyAnimator.SetMethod<Engine::Components::KeyAnimator, bool, int64_t>("bool is_exists(int64) const", &Engine::Components::KeyAnimator::IsExists);
				VKeyAnimator.SetMethod<Engine::Components::KeyAnimator, bool, int64_t, int64_t>("bool is_exists(int64, int64) const", &Engine::Components::KeyAnimator::IsExists);
				VKeyAnimator.SetMethod("animator_key& get_frame(int64, int64) const", &Engine::Components::KeyAnimator::GetFrame);
				VKeyAnimator.SetMethod("string get_path() const", &Engine::Components::KeyAnimator::GetPath);
				PopulateComponentInterface<Engine::Components::KeyAnimator, Engine::Entity*>(VKeyAnimator, "key_animator_component@+ f(scene_entity@+)");

				RefClass VEmitterAnimator = Engine->SetClass<Engine::Components::EmitterAnimator>("emitter_animator_component", false);
				VEmitterAnimator.SetProperty<Engine::Components::EmitterAnimator>("vector4 diffuse", &Engine::Components::EmitterAnimator::Diffuse);
				VEmitterAnimator.SetProperty<Engine::Components::EmitterAnimator>("vector3 position", &Engine::Components::EmitterAnimator::Position);
				VEmitterAnimator.SetProperty<Engine::Components::EmitterAnimator>("vector3 velocity", &Engine::Components::EmitterAnimator::Velocity);
				VEmitterAnimator.SetProperty<Engine::Components::EmitterAnimator>("spawner_properties spawner", &Engine::Components::EmitterAnimator::Spawner);
				VEmitterAnimator.SetProperty<Engine::Components::EmitterAnimator>("float rotation_speed", &Engine::Components::EmitterAnimator::RotationSpeed);
				VEmitterAnimator.SetProperty<Engine::Components::EmitterAnimator>("float scale_speed", &Engine::Components::EmitterAnimator::ScaleSpeed);
				VEmitterAnimator.SetProperty<Engine::Components::EmitterAnimator>("float noise", &Engine::Components::EmitterAnimator::Noise);
				VEmitterAnimator.SetProperty<Engine::Components::EmitterAnimator>("bool simulate", &Engine::Components::EmitterAnimator::Simulate);
				VEmitterAnimator.SetMethod("emitter_component@+ get_emitter() const", &Engine::Components::EmitterAnimator::GetEmitter);
				PopulateComponentInterface<Engine::Components::EmitterAnimator, Engine::Entity*>(VEmitterAnimator, "emitter_animator_component@+ f(scene_entity@+)");

				RefClass VFreeLook = Engine->SetClass<Engine::Components::FreeLook>("free_look_component", false);
				VFreeLook.SetProperty<Engine::Components::FreeLook>("vector2 direction", &Engine::Components::FreeLook::Direction);
				VFreeLook.SetProperty<Engine::Components::FreeLook>("key_map rotate", &Engine::Components::FreeLook::Rotate);
				VFreeLook.SetProperty<Engine::Components::FreeLook>("float sensivity", &Engine::Components::FreeLook::Sensivity);
				PopulateComponentInterface<Engine::Components::FreeLook, Engine::Entity*>(VFreeLook, "free_look_component@+ f(scene_entity@+)");

				TypeClass VFlyMoveInfo = Engine->SetPod<Engine::Components::Fly::MoveInfo>("fly_move_info");
				VFlyMoveInfo.SetProperty<Engine::Components::Fly::MoveInfo>("vector3 axis", &Engine::Components::Fly::MoveInfo::Axis);
				VFlyMoveInfo.SetProperty<Engine::Components::Fly::MoveInfo>("float faster", &Engine::Components::Fly::MoveInfo::Faster);
				VFlyMoveInfo.SetProperty<Engine::Components::Fly::MoveInfo>("float normal", &Engine::Components::Fly::MoveInfo::Normal);
				VFlyMoveInfo.SetProperty<Engine::Components::Fly::MoveInfo>("float slower", &Engine::Components::Fly::MoveInfo::Slower);
				VFlyMoveInfo.SetProperty<Engine::Components::Fly::MoveInfo>("float fading", &Engine::Components::Fly::MoveInfo::Fading);

				RefClass VFly = Engine->SetClass<Engine::Components::Fly>("fly_component", false);
				VFly.SetProperty<Engine::Components::Fly>("fly_move_info moving", &Engine::Components::Fly::Moving);
				VFly.SetProperty<Engine::Components::Fly>("key_map forward", &Engine::Components::Fly::Forward);
				VFly.SetProperty<Engine::Components::Fly>("key_map backward", &Engine::Components::Fly::Backward);
				VFly.SetProperty<Engine::Components::Fly>("key_map right", &Engine::Components::Fly::Right);
				VFly.SetProperty<Engine::Components::Fly>("key_map left", &Engine::Components::Fly::Left);
				VFly.SetProperty<Engine::Components::Fly>("key_map up", &Engine::Components::Fly::Up);
				VFly.SetProperty<Engine::Components::Fly>("key_map down", &Engine::Components::Fly::Down);
				VFly.SetProperty<Engine::Components::Fly>("key_map fast", &Engine::Components::Fly::Fast);
				VFly.SetProperty<Engine::Components::Fly>("key_map slow", &Engine::Components::Fly::Slow);
				PopulateComponentInterface<Engine::Components::Fly, Engine::Entity*>(VFly, "fly_component@+ f(scene_entity@+)");

				RefClass VAudioSource = Engine->SetClass<Engine::Components::AudioSource>("audio_source_component", false);
				VAudioSource.SetMethod("void apply_playing_position()", &Engine::Components::AudioSource::ApplyPlayingPosition);
				VAudioSource.SetMethod("audio_source@+ get_source() const", &Engine::Components::AudioSource::GetSource);
				VAudioSource.SetMethod("audio_sync& get_sync()", &Engine::Components::AudioSource::GetSync);
				PopulateComponentInterface<Engine::Components::AudioSource, Engine::Entity*>(VAudioSource, "audio_source_component@+ f(scene_entity@+)");

				RefClass VAudioListener = Engine->SetClass<Engine::Components::AudioListener>("audio_listener_component", false);
				VAudioListener.SetProperty<Engine::Components::AudioListener>("float gain", &Engine::Components::AudioListener::Gain);
				PopulateComponentInterface<Engine::Components::AudioListener, Engine::Entity*>(VAudioListener, "audio_listener_component@+ f(scene_entity@+)");

				TypeClass PointLightShadowInfo = Engine->SetPod<Engine::Components::PointLight::ShadowInfo>("point_light_shadow_info");
				PointLightShadowInfo.SetProperty<Engine::Components::PointLight::ShadowInfo>("float softness", &Engine::Components::PointLight::ShadowInfo::Softness);
				PointLightShadowInfo.SetProperty<Engine::Components::PointLight::ShadowInfo>("float distance", &Engine::Components::PointLight::ShadowInfo::Distance);
				PointLightShadowInfo.SetProperty<Engine::Components::PointLight::ShadowInfo>("float bias", &Engine::Components::PointLight::ShadowInfo::Bias);
				PointLightShadowInfo.SetProperty<Engine::Components::PointLight::ShadowInfo>("uint32 iterations", &Engine::Components::PointLight::ShadowInfo::Iterations);
				PointLightShadowInfo.SetProperty<Engine::Components::PointLight::ShadowInfo>("bool enabled", &Engine::Components::PointLight::ShadowInfo::Enabled);

				RefClass VPointLight = Engine->SetClass<Engine::Components::PointLight>("point_light_component", false);
				VPointLight.SetProperty<Engine::Components::PointLight>("point_light_shadow_info shadow", &Engine::Components::PointLight::Shadow);
				VPointLight.SetProperty<Engine::Components::PointLight>("matrix4x4 view", &Engine::Components::PointLight::View);
				VPointLight.SetProperty<Engine::Components::PointLight>("matrix4x4 projection", &Engine::Components::PointLight::Projection);
				VPointLight.SetProperty<Engine::Components::PointLight>("vector3 diffuse", &Engine::Components::PointLight::Diffuse);
				VPointLight.SetProperty<Engine::Components::PointLight>("float emission", &Engine::Components::PointLight::Emission);
				VPointLight.SetProperty<Engine::Components::PointLight>("float disperse", &Engine::Components::PointLight::Disperse);
				VPointLight.SetMethod("void generate_origin()", &Engine::Components::PointLight::GenerateOrigin);
				VPointLight.SetMethod("void set_size(const attenuation &in)", &Engine::Components::PointLight::SetSize);
				VPointLight.SetMethod("const attenuation& get_size() const", &Engine::Components::PointLight::SetSize);
				PopulateComponentInterface<Engine::Components::PointLight, Engine::Entity*>(VPointLight, "point_light_component@+ f(scene_entity@+)");

				TypeClass SpotLightShadowInfo = Engine->SetPod<Engine::Components::SpotLight::ShadowInfo>("spot_light_shadow_info");
				SpotLightShadowInfo.SetProperty<Engine::Components::SpotLight::ShadowInfo>("float softness", &Engine::Components::SpotLight::ShadowInfo::Softness);
				SpotLightShadowInfo.SetProperty<Engine::Components::SpotLight::ShadowInfo>("float distance", &Engine::Components::SpotLight::ShadowInfo::Distance);
				SpotLightShadowInfo.SetProperty<Engine::Components::SpotLight::ShadowInfo>("float bias", &Engine::Components::SpotLight::ShadowInfo::Bias);
				SpotLightShadowInfo.SetProperty<Engine::Components::SpotLight::ShadowInfo>("uint32 iterations", &Engine::Components::SpotLight::ShadowInfo::Iterations);
				SpotLightShadowInfo.SetProperty<Engine::Components::SpotLight::ShadowInfo>("bool enabled", &Engine::Components::SpotLight::ShadowInfo::Enabled);

				RefClass VSpotLight = Engine->SetClass<Engine::Components::SpotLight>("spot_light_component", false);
				VSpotLight.SetProperty<Engine::Components::SpotLight>("spot_light_shadow_info shadow", &Engine::Components::SpotLight::Shadow);
				VSpotLight.SetProperty<Engine::Components::SpotLight>("matrix4x4 view", &Engine::Components::SpotLight::View);
				VSpotLight.SetProperty<Engine::Components::SpotLight>("matrix4x4 projection", &Engine::Components::SpotLight::Projection);
				VSpotLight.SetProperty<Engine::Components::SpotLight>("vector3 diffuse", &Engine::Components::SpotLight::Diffuse);
				VSpotLight.SetProperty<Engine::Components::SpotLight>("float emission", &Engine::Components::SpotLight::Emission);
				VSpotLight.SetProperty<Engine::Components::SpotLight>("float cutoff", &Engine::Components::SpotLight::Cutoff);
				VSpotLight.SetProperty<Engine::Components::SpotLight>("float disperse", &Engine::Components::SpotLight::Disperse);
				VSpotLight.SetMethod("void generate_origin()", &Engine::Components::SpotLight::GenerateOrigin);
				VSpotLight.SetMethod("void set_size(const attenuation &in)", &Engine::Components::SpotLight::SetSize);
				VSpotLight.SetMethod("const attenuation& get_size() const", &Engine::Components::SpotLight::SetSize);
				PopulateComponentInterface<Engine::Components::SpotLight, Engine::Entity*>(VSpotLight, "spot_light_component@+ f(scene_entity@+)");

				TypeClass LineLightSkyInfo = Engine->SetPod<Engine::Components::LineLight::SkyInfo>("line_light_sky_info");
				LineLightSkyInfo.SetProperty<Engine::Components::LineLight::SkyInfo>("vector3 elh_emission", &Engine::Components::LineLight::SkyInfo::RlhEmission);
				LineLightSkyInfo.SetProperty<Engine::Components::LineLight::SkyInfo>("vector3 mie_emission", &Engine::Components::LineLight::SkyInfo::MieEmission);
				LineLightSkyInfo.SetProperty<Engine::Components::LineLight::SkyInfo>("float rlh_height", &Engine::Components::LineLight::SkyInfo::RlhHeight);
				LineLightSkyInfo.SetProperty<Engine::Components::LineLight::SkyInfo>("float mie_height", &Engine::Components::LineLight::SkyInfo::MieHeight);
				LineLightSkyInfo.SetProperty<Engine::Components::LineLight::SkyInfo>("float mie_direction", &Engine::Components::LineLight::SkyInfo::MieDirection);
				LineLightSkyInfo.SetProperty<Engine::Components::LineLight::SkyInfo>("float inner_radius", &Engine::Components::LineLight::SkyInfo::InnerRadius);
				LineLightSkyInfo.SetProperty<Engine::Components::LineLight::SkyInfo>("float outer_radius", &Engine::Components::LineLight::SkyInfo::OuterRadius);
				LineLightSkyInfo.SetProperty<Engine::Components::LineLight::SkyInfo>("float intensity", &Engine::Components::LineLight::SkyInfo::Intensity);

				TypeClass LineLightShadowInfo = Engine->SetPod<Engine::Components::LineLight::ShadowInfo>("line_light_shadow_info");
				LineLightShadowInfo.SetPropertyArray<Engine::Components::LineLight::ShadowInfo>("float distance", &Engine::Components::LineLight::ShadowInfo::Distance, 6);
				LineLightShadowInfo.SetProperty<Engine::Components::LineLight::ShadowInfo>("float softness", &Engine::Components::LineLight::ShadowInfo::Softness);
				LineLightShadowInfo.SetProperty<Engine::Components::LineLight::ShadowInfo>("float bias", &Engine::Components::LineLight::ShadowInfo::Bias);
				LineLightShadowInfo.SetProperty<Engine::Components::LineLight::ShadowInfo>("float near", &Engine::Components::LineLight::ShadowInfo::Near);
				LineLightShadowInfo.SetProperty<Engine::Components::LineLight::ShadowInfo>("float far", &Engine::Components::LineLight::ShadowInfo::Far);
				LineLightShadowInfo.SetProperty<Engine::Components::LineLight::ShadowInfo>("uint32 iterations", &Engine::Components::LineLight::ShadowInfo::Iterations);
				LineLightShadowInfo.SetProperty<Engine::Components::LineLight::ShadowInfo>("uint32 cascades", &Engine::Components::LineLight::ShadowInfo::Cascades);
				LineLightShadowInfo.SetProperty<Engine::Components::LineLight::ShadowInfo>("bool enabled", &Engine::Components::LineLight::ShadowInfo::Enabled);

				RefClass VLineLight = Engine->SetClass<Engine::Components::LineLight>("line_light_component", false);
				VLineLight.SetProperty<Engine::Components::LineLight>("line_light_sky_info sky", &Engine::Components::LineLight::Sky);
				VLineLight.SetProperty<Engine::Components::LineLight>("line_light_shadow_info shadow", &Engine::Components::LineLight::Shadow);
				VLineLight.SetPropertyArray<Engine::Components::LineLight>("matrix4x4 projection", &Engine::Components::LineLight::Projection, 6);
				VLineLight.SetPropertyArray<Engine::Components::LineLight>("matrix4x4 view", &Engine::Components::LineLight::View, 6);
				VLineLight.SetProperty<Engine::Components::LineLight>("vector3 diffuse", &Engine::Components::LineLight::Diffuse);
				VLineLight.SetProperty<Engine::Components::LineLight>("float emission", &Engine::Components::LineLight::Emission);
				VLineLight.SetMethod("void generate_origin()", &Engine::Components::LineLight::GenerateOrigin);
				PopulateComponentInterface<Engine::Components::LineLight, Engine::Entity*>(VLineLight, "line_light_component@+ f(scene_entity@+)");

				RefClass VSurfaceLight = Engine->SetClass<Engine::Components::SurfaceLight>("surface_light_component", false);
				VSurfaceLight.SetPropertyArray<Engine::Components::SurfaceLight>("matrix4x4 view", &Engine::Components::SurfaceLight::View, 6);
				VSurfaceLight.SetProperty<Engine::Components::SurfaceLight>("matrix4x4 projection", &Engine::Components::SurfaceLight::Projection);
				VSurfaceLight.SetProperty<Engine::Components::SurfaceLight>("vector3 offset", &Engine::Components::SurfaceLight::Offset);
				VSurfaceLight.SetProperty<Engine::Components::SurfaceLight>("vector3 diffuse", &Engine::Components::SurfaceLight::Diffuse);
				VSurfaceLight.SetProperty<Engine::Components::SurfaceLight>("clock_ticker tick", &Engine::Components::SurfaceLight::Tick);
				VSurfaceLight.SetProperty<Engine::Components::SurfaceLight>("float emission", &Engine::Components::SurfaceLight::Emission);
				VSurfaceLight.SetProperty<Engine::Components::SurfaceLight>("float infinity", &Engine::Components::SurfaceLight::Infinity);
				VSurfaceLight.SetProperty<Engine::Components::SurfaceLight>("bool parallax", &Engine::Components::SurfaceLight::Parallax);
				VSurfaceLight.SetProperty<Engine::Components::SurfaceLight>("bool locked", &Engine::Components::SurfaceLight::Locked);
				VSurfaceLight.SetProperty<Engine::Components::SurfaceLight>("bool static_mask", &Engine::Components::SurfaceLight::StaticMask);
				VSurfaceLight.SetMethod("void set_probe_cache(texture_cube@+)", &Engine::Components::SurfaceLight::SetProbeCache);
				VSurfaceLight.SetMethod("void set_size(const attenuation &in)", &Engine::Components::SurfaceLight::SetSize);
				VSurfaceLight.SetMethod<Engine::Components::SurfaceLight, bool, Graphics::Texture2D*>("void set_diffuse_map(texture_2d@+)", &Engine::Components::SurfaceLight::SetDiffuseMap);
				VSurfaceLight.SetMethod("bool is_image_based() const", &Engine::Components::SurfaceLight::IsImageBased);
				VSurfaceLight.SetMethod("const attenuation& get_size() const", &Engine::Components::SurfaceLight::GetSize);
				VSurfaceLight.SetMethod("texture_cube@+ get_probe_cache() const", &Engine::Components::SurfaceLight::GetProbeCache);
				VSurfaceLight.SetMethod("texture_2d@+ get_diffuse_map_xp() const", &Engine::Components::SurfaceLight::GetDiffuseMapXP);
				VSurfaceLight.SetMethod("texture_2d@+ get_diffuse_map_xn() const", &Engine::Components::SurfaceLight::GetDiffuseMapXN);
				VSurfaceLight.SetMethod("texture_2d@+ get_diffuse_map_yp() const", &Engine::Components::SurfaceLight::GetDiffuseMapYP);
				VSurfaceLight.SetMethod("texture_2d@+ get_diffuse_map_yn() const", &Engine::Components::SurfaceLight::GetDiffuseMapYN);
				VSurfaceLight.SetMethod("texture_2d@+ get_diffuse_map_zp() const", &Engine::Components::SurfaceLight::GetDiffuseMapZP);
				VSurfaceLight.SetMethod("texture_2d@+ get_diffuse_map_zn() const", &Engine::Components::SurfaceLight::GetDiffuseMapZN);
				VSurfaceLight.SetMethod("texture_2d@+ get_diffuse_map() const", &Engine::Components::SurfaceLight::GetDiffuseMap);
				PopulateComponentInterface<Engine::Components::SurfaceLight, Engine::Entity*>(VSurfaceLight, "surface_light_component@+ f(scene_entity@+)");

				RefClass VIlluminator = Engine->SetClass<Engine::Components::Illuminator>("illuminator_component", false);
				VIlluminator.SetProperty<Engine::Components::Illuminator>("clock_ticker inside", &Engine::Components::Illuminator::Inside);
				VIlluminator.SetProperty<Engine::Components::Illuminator>("clock_ticker outside", &Engine::Components::Illuminator::Outside);
				VIlluminator.SetProperty<Engine::Components::Illuminator>("float ray_step", &Engine::Components::Illuminator::RayStep);
				VIlluminator.SetProperty<Engine::Components::Illuminator>("float max_steps", &Engine::Components::Illuminator::MaxSteps);
				VIlluminator.SetProperty<Engine::Components::Illuminator>("float distance", &Engine::Components::Illuminator::Distance);
				VIlluminator.SetProperty<Engine::Components::Illuminator>("float radiance", &Engine::Components::Illuminator::Radiance);
				VIlluminator.SetProperty<Engine::Components::Illuminator>("float length", &Engine::Components::Illuminator::Length);
				VIlluminator.SetProperty<Engine::Components::Illuminator>("float margin", &Engine::Components::Illuminator::Margin);
				VIlluminator.SetProperty<Engine::Components::Illuminator>("float offset", &Engine::Components::Illuminator::Offset);
				VIlluminator.SetProperty<Engine::Components::Illuminator>("float angle", &Engine::Components::Illuminator::Angle);
				VIlluminator.SetProperty<Engine::Components::Illuminator>("float occlusion", &Engine::Components::Illuminator::Occlusion);
				VIlluminator.SetProperty<Engine::Components::Illuminator>("float specular", &Engine::Components::Illuminator::Specular);
				VIlluminator.SetProperty<Engine::Components::Illuminator>("float bleeding", &Engine::Components::Illuminator::Bleeding);
				VIlluminator.SetProperty<Engine::Components::Illuminator>("bool regenerate", &Engine::Components::Illuminator::Regenerate);
				PopulateComponentInterface<Engine::Components::Illuminator, Engine::Entity*>(VIlluminator, "illuminator_component@+ f(scene_entity@+)");

				Enumeration VCameraProjection = Engine->SetEnum("camera_projection");
				VCameraProjection.SetValue("perspective", (int)Engine::Components::Camera::ProjectionMode::Perspective);
				VCameraProjection.SetValue("orthographic", (int)Engine::Components::Camera::ProjectionMode::Orthographic);

				RefClass VCamera = Engine->SetClass<Engine::Components::Camera>("camera_component", false);
				VCamera.SetProperty<Engine::Components::Camera>("camera_projection mode", &Engine::Components::Camera::Mode);
				VCamera.SetProperty<Engine::Components::Camera>("float near_plane", &Engine::Components::Camera::NearPlane);
				VCamera.SetProperty<Engine::Components::Camera>("float far_plane", &Engine::Components::Camera::FarPlane);
				VCamera.SetProperty<Engine::Components::Camera>("float width", &Engine::Components::Camera::Width);
				VCamera.SetProperty<Engine::Components::Camera>("float height", &Engine::Components::Camera::Height);
				VCamera.SetProperty<Engine::Components::Camera>("float field_of_view", &Engine::Components::Camera::FieldOfView);
				VCamera.SetMethod<Engine::Components::Camera, void, Engine::Viewer*>("void get_viewer(viewer_t &out)", &Engine::Components::Camera::GetViewer);
				VCamera.SetMethod("void resize_buffers()", &Engine::Components::Camera::ResizeBuffers);
				VCamera.SetMethod<Engine::Components::Camera, Engine::Viewer&>("viewer_t& get_viewer()", &Engine::Components::Camera::GetViewer);
				VCamera.SetMethod("render_system@+ get_renderer() const", &Engine::Components::Camera::GetRenderer);
				VCamera.SetMethod("matrix4x4 get_projection() const", &Engine::Components::Camera::GetProjection);
				VCamera.SetMethod("matrix4x4 get_view_projection() const", &Engine::Components::Camera::GetViewProjection);
				VCamera.SetMethod("matrix4x4 get_view() const", &Engine::Components::Camera::GetView);
				VCamera.SetMethod("vector3 get_view_position() const", &Engine::Components::Camera::GetViewPosition);
				VCamera.SetMethod("frustum_8c get_frustum_8c() const", &Engine::Components::Camera::GetFrustum8C);
				VCamera.SetMethod("frustum_6p get_frustum_6p() const", &Engine::Components::Camera::GetFrustum6P);
				VCamera.SetMethod("ray get_screen_ray(const vector2 &in) const", &Engine::Components::Camera::GetScreenRay);
				VCamera.SetMethod("ray get_cursor_ray() const", &Engine::Components::Camera::GetCursorRay);
				VCamera.SetMethod("float get_distance(scene_entity@+) const", &Engine::Components::Camera::GetDistance);
				VCamera.SetMethod("float get_width() const", &Engine::Components::Camera::GetWidth);
				VCamera.SetMethod("float get_height() const", &Engine::Components::Camera::GetHeight);
				VCamera.SetMethod("float get_aspect() const", &Engine::Components::Camera::GetAspect);
				VCamera.SetMethod<Engine::Components::Camera, bool, const Compute::Ray&, Engine::Entity*, Compute::Vector3*>("bool ray_test(const ray &in, scene_entity@+, vector3 &out)", &Engine::Components::Camera::RayTest);
				VCamera.SetMethod<Engine::Components::Camera, bool, const Compute::Ray&, const Compute::Matrix4x4&, Compute::Vector3*>("bool ray_test(const ray &in, const matrix4x4 &in, vector3 &out)", &Engine::Components::Camera::RayTest);
				PopulateComponentInterface<Engine::Components::Camera, Engine::Entity*>(VCamera, "camera_component@+ f(scene_entity@+)");

				RefClass VScriptable = Engine->SetClass<Engine::Components::Scriptable>("scriptable_component", false);
				PopulateComponentInterface<Engine::Components::Scriptable, Engine::Entity*>(VScriptable, "scriptable_component@+ f(scene_entity@+)");

				return true;
#else
				VI_ASSERT(false, false, "<components> is not loaded");
				return false;
#endif
			}
			bool Registry::LoadRenderers(VirtualMachine* Engine)
			{
#ifdef VI_HAS_BINDINGS
				VI_ASSERT(Engine != nullptr, false, "manager should be set");

				RefClass VSoftBody = Engine->SetClass<Engine::Renderers::SoftBody>("soft_body_renderer", false);
				PopulateRendererInterface<Engine::Renderers::SoftBody, Engine::RenderSystem*>(VSoftBody, "soft_body_renderer@+ f(render_system@+)");

				RefClass VModel = Engine->SetClass<Engine::Renderers::Model>("model_renderer", false);
				PopulateRendererInterface<Engine::Renderers::Model, Engine::RenderSystem*>(VModel, "model_renderer@+ f(render_system@+)");

				RefClass VSkin = Engine->SetClass<Engine::Renderers::Skin>("skin_renderer", false);
				PopulateRendererInterface<Engine::Renderers::Skin, Engine::RenderSystem*>(VSkin, "skin_renderer@+ f(render_system@+)");

				RefClass VEmitter = Engine->SetClass<Engine::Renderers::Emitter>("emitter_renderer", false);
				PopulateRendererInterface<Engine::Renderers::Emitter, Engine::RenderSystem*>(VEmitter, "emitter_renderer@+ f(render_system@+)");

				RefClass VDecal = Engine->SetClass<Engine::Renderers::Decal>("decal_renderer", false);
				PopulateRendererInterface<Engine::Renderers::Decal, Engine::RenderSystem*>(VDecal, "decal_renderer@+ f(render_system@+)");

				TypeClass VLightingISurfaceLight = Engine->SetPod<Engine::Renderers::Lighting::ISurfaceLight>("lighting_surface_light");
				VLightingISurfaceLight.SetProperty<Engine::Renderers::Lighting::ISurfaceLight>("matrix4x4 world", &Engine::Renderers::Lighting::ISurfaceLight::Transform);
				VLightingISurfaceLight.SetProperty<Engine::Renderers::Lighting::ISurfaceLight>("vector3 position", &Engine::Renderers::Lighting::ISurfaceLight::Position);
				VLightingISurfaceLight.SetProperty<Engine::Renderers::Lighting::ISurfaceLight>("float range", &Engine::Renderers::Lighting::ISurfaceLight::Range);
				VLightingISurfaceLight.SetProperty<Engine::Renderers::Lighting::ISurfaceLight>("vector3 lighting", &Engine::Renderers::Lighting::ISurfaceLight::Lighting);
				VLightingISurfaceLight.SetProperty<Engine::Renderers::Lighting::ISurfaceLight>("float mips", &Engine::Renderers::Lighting::ISurfaceLight::Mips);
				VLightingISurfaceLight.SetProperty<Engine::Renderers::Lighting::ISurfaceLight>("vector3 scale", &Engine::Renderers::Lighting::ISurfaceLight::Scale);
				VLightingISurfaceLight.SetProperty<Engine::Renderers::Lighting::ISurfaceLight>("float parallax", &Engine::Renderers::Lighting::ISurfaceLight::Parallax);
				VLightingISurfaceLight.SetProperty<Engine::Renderers::Lighting::ISurfaceLight>("vector3 attenuations", &Engine::Renderers::Lighting::ISurfaceLight::Attenuation);
				VLightingISurfaceLight.SetProperty<Engine::Renderers::Lighting::ISurfaceLight>("float infinity", &Engine::Renderers::Lighting::ISurfaceLight::Infinity);

				TypeClass VLightingIPointLight = Engine->SetPod<Engine::Renderers::Lighting::IPointLight>("lighting_point_light");
				VLightingIPointLight.SetProperty<Engine::Renderers::Lighting::IPointLight>("matrix4x4 world", &Engine::Renderers::Lighting::IPointLight::Transform);
				VLightingIPointLight.SetProperty<Engine::Renderers::Lighting::IPointLight>("vector4 attenuations", &Engine::Renderers::Lighting::IPointLight::Attenuation);
				VLightingIPointLight.SetProperty<Engine::Renderers::Lighting::IPointLight>("vector3 position", &Engine::Renderers::Lighting::IPointLight::Position);
				VLightingIPointLight.SetProperty<Engine::Renderers::Lighting::IPointLight>("float range", &Engine::Renderers::Lighting::IPointLight::Range);
				VLightingIPointLight.SetProperty<Engine::Renderers::Lighting::IPointLight>("vector3 lighting", &Engine::Renderers::Lighting::IPointLight::Lighting);
				VLightingIPointLight.SetProperty<Engine::Renderers::Lighting::IPointLight>("float distance", &Engine::Renderers::Lighting::IPointLight::Distance);
				VLightingIPointLight.SetProperty<Engine::Renderers::Lighting::IPointLight>("float umbra", &Engine::Renderers::Lighting::IPointLight::Umbra);
				VLightingIPointLight.SetProperty<Engine::Renderers::Lighting::IPointLight>("float softness", &Engine::Renderers::Lighting::IPointLight::Softness);
				VLightingIPointLight.SetProperty<Engine::Renderers::Lighting::IPointLight>("float bias", &Engine::Renderers::Lighting::IPointLight::Bias);
				VLightingIPointLight.SetProperty<Engine::Renderers::Lighting::IPointLight>("float iterations", &Engine::Renderers::Lighting::IPointLight::Iterations);

				TypeClass VLightingISpotLight = Engine->SetPod<Engine::Renderers::Lighting::ISpotLight>("lighting_spot_light");
				VLightingISpotLight.SetProperty<Engine::Renderers::Lighting::ISpotLight>("matrix4x4 world", &Engine::Renderers::Lighting::ISpotLight::Transform);
				VLightingISpotLight.SetProperty<Engine::Renderers::Lighting::ISpotLight>("matrix4x4 view_projection", &Engine::Renderers::Lighting::ISpotLight::ViewProjection);
				VLightingISpotLight.SetProperty<Engine::Renderers::Lighting::ISpotLight>("vector4 attenuations", &Engine::Renderers::Lighting::ISpotLight::Attenuation);
				VLightingISpotLight.SetProperty<Engine::Renderers::Lighting::ISpotLight>("vector3 direction", &Engine::Renderers::Lighting::ISpotLight::Direction);
				VLightingISpotLight.SetProperty<Engine::Renderers::Lighting::ISpotLight>("float cutoff", &Engine::Renderers::Lighting::ISpotLight::Cutoff);
				VLightingISpotLight.SetProperty<Engine::Renderers::Lighting::ISpotLight>("vector3 position", &Engine::Renderers::Lighting::ISpotLight::Position);
				VLightingISpotLight.SetProperty<Engine::Renderers::Lighting::ISpotLight>("float range", &Engine::Renderers::Lighting::ISpotLight::Range);
				VLightingISpotLight.SetProperty<Engine::Renderers::Lighting::ISpotLight>("vector3 lighting", &Engine::Renderers::Lighting::ISpotLight::Lighting);
				VLightingISpotLight.SetProperty<Engine::Renderers::Lighting::ISpotLight>("float softness", &Engine::Renderers::Lighting::ISpotLight::Softness);
				VLightingISpotLight.SetProperty<Engine::Renderers::Lighting::ISpotLight>("float bias", &Engine::Renderers::Lighting::ISpotLight::Bias);
				VLightingISpotLight.SetProperty<Engine::Renderers::Lighting::ISpotLight>("float iterations", &Engine::Renderers::Lighting::ISpotLight::Iterations);
				VLightingISpotLight.SetProperty<Engine::Renderers::Lighting::ISpotLight>("float umbra", &Engine::Renderers::Lighting::ISpotLight::Umbra);
				VLightingISpotLight.SetProperty<Engine::Renderers::Lighting::ISpotLight>("float padding", &Engine::Renderers::Lighting::ISpotLight::Padding);

				TypeClass VLightingILineLight = Engine->SetPod<Engine::Renderers::Lighting::ILineLight>("lighting_line_light");
				VLightingILineLight.SetPropertyArray<Engine::Renderers::Lighting::ILineLight>("matrix4x4 view_projection", &Engine::Renderers::Lighting::ILineLight::ViewProjection, 6);
				VLightingILineLight.SetProperty<Engine::Renderers::Lighting::ILineLight>("matrix4x4 sky_offset", &Engine::Renderers::Lighting::ILineLight::SkyOffset);
				VLightingILineLight.SetProperty<Engine::Renderers::Lighting::ILineLight>("vector3 rlh_emission", &Engine::Renderers::Lighting::ILineLight::RlhEmission);
				VLightingILineLight.SetProperty<Engine::Renderers::Lighting::ILineLight>("float rlh_height", &Engine::Renderers::Lighting::ILineLight::RlhHeight);
				VLightingILineLight.SetProperty<Engine::Renderers::Lighting::ILineLight>("vector3 mie_emission", &Engine::Renderers::Lighting::ILineLight::MieEmission);
				VLightingILineLight.SetProperty<Engine::Renderers::Lighting::ILineLight>("float mie_height", &Engine::Renderers::Lighting::ILineLight::MieHeight);
				VLightingILineLight.SetProperty<Engine::Renderers::Lighting::ILineLight>("vector3 lighting", &Engine::Renderers::Lighting::ILineLight::Lighting);
				VLightingILineLight.SetProperty<Engine::Renderers::Lighting::ILineLight>("float softness", &Engine::Renderers::Lighting::ILineLight::Softness);
				VLightingILineLight.SetProperty<Engine::Renderers::Lighting::ILineLight>("vector3 position", &Engine::Renderers::Lighting::ILineLight::Position);
				VLightingILineLight.SetProperty<Engine::Renderers::Lighting::ILineLight>("float cascades", &Engine::Renderers::Lighting::ILineLight::Cascades);
				VLightingILineLight.SetProperty<Engine::Renderers::Lighting::ILineLight>("vector2 padding", &Engine::Renderers::Lighting::ILineLight::Padding);
				VLightingILineLight.SetProperty<Engine::Renderers::Lighting::ILineLight>("float bias", &Engine::Renderers::Lighting::ILineLight::Bias);
				VLightingILineLight.SetProperty<Engine::Renderers::Lighting::ILineLight>("float iterations", &Engine::Renderers::Lighting::ILineLight::Iterations);
				VLightingILineLight.SetProperty<Engine::Renderers::Lighting::ILineLight>("float scatter_intensity", &Engine::Renderers::Lighting::ILineLight::ScatterIntensity);
				VLightingILineLight.SetProperty<Engine::Renderers::Lighting::ILineLight>("float planet_radius", &Engine::Renderers::Lighting::ILineLight::PlanetRadius);
				VLightingILineLight.SetProperty<Engine::Renderers::Lighting::ILineLight>("float atmosphere_radius", &Engine::Renderers::Lighting::ILineLight::AtmosphereRadius);
				VLightingILineLight.SetProperty<Engine::Renderers::Lighting::ILineLight>("float mie_direction", &Engine::Renderers::Lighting::ILineLight::MieDirection);

				TypeClass VLightingIAmbientLight = Engine->SetPod<Engine::Renderers::Lighting::IAmbientLight>("lighting_ambient_light");
				VLightingIAmbientLight.SetProperty<Engine::Renderers::Lighting::IAmbientLight>("matrix4x4 sky_offset", &Engine::Renderers::Lighting::IAmbientLight::SkyOffset);
				VLightingIAmbientLight.SetProperty<Engine::Renderers::Lighting::IAmbientLight>("vector3 high_emission", &Engine::Renderers::Lighting::IAmbientLight::HighEmission);
				VLightingIAmbientLight.SetProperty<Engine::Renderers::Lighting::IAmbientLight>("float sky_emission", &Engine::Renderers::Lighting::IAmbientLight::SkyEmission);
				VLightingIAmbientLight.SetProperty<Engine::Renderers::Lighting::IAmbientLight>("vector3 low_emission", &Engine::Renderers::Lighting::IAmbientLight::LowEmission);
				VLightingIAmbientLight.SetProperty<Engine::Renderers::Lighting::IAmbientLight>("float light_emission", &Engine::Renderers::Lighting::IAmbientLight::LightEmission);
				VLightingIAmbientLight.SetProperty<Engine::Renderers::Lighting::IAmbientLight>("vector3 sky_color", &Engine::Renderers::Lighting::IAmbientLight::SkyColor);
				VLightingIAmbientLight.SetProperty<Engine::Renderers::Lighting::IAmbientLight>("float fog_far_off", &Engine::Renderers::Lighting::IAmbientLight::FogFarOff);
				VLightingIAmbientLight.SetProperty<Engine::Renderers::Lighting::IAmbientLight>("vector3 fog_color", &Engine::Renderers::Lighting::IAmbientLight::FogColor);
				VLightingIAmbientLight.SetProperty<Engine::Renderers::Lighting::IAmbientLight>("float fog_near_off", &Engine::Renderers::Lighting::IAmbientLight::FogNearOff);
				VLightingIAmbientLight.SetProperty<Engine::Renderers::Lighting::IAmbientLight>("vector3 fog_far", &Engine::Renderers::Lighting::IAmbientLight::FogFar);
				VLightingIAmbientLight.SetProperty<Engine::Renderers::Lighting::IAmbientLight>("float fog_amount", &Engine::Renderers::Lighting::IAmbientLight::FogAmount);
				VLightingIAmbientLight.SetProperty<Engine::Renderers::Lighting::IAmbientLight>("vector3 fog_near", &Engine::Renderers::Lighting::IAmbientLight::FogNear);
				VLightingIAmbientLight.SetProperty<Engine::Renderers::Lighting::IAmbientLight>("float recursive", &Engine::Renderers::Lighting::IAmbientLight::Recursive);

				TypeClass VLightingIVoxelBuffer = Engine->SetPod<Engine::Renderers::Lighting::IVoxelBuffer>("lighting_voxel_buffer");
				VLightingIVoxelBuffer.SetProperty<Engine::Renderers::Lighting::IVoxelBuffer>("matrix4x4 world", &Engine::Renderers::Lighting::IVoxelBuffer::Transform);
				VLightingIVoxelBuffer.SetProperty<Engine::Renderers::Lighting::IVoxelBuffer>("vector3 center", &Engine::Renderers::Lighting::IVoxelBuffer::Center);
				VLightingIVoxelBuffer.SetProperty<Engine::Renderers::Lighting::IVoxelBuffer>("float ray_step", &Engine::Renderers::Lighting::IVoxelBuffer::RayStep);
				VLightingIVoxelBuffer.SetProperty<Engine::Renderers::Lighting::IVoxelBuffer>("vector3 size", &Engine::Renderers::Lighting::IVoxelBuffer::Size);
				VLightingIVoxelBuffer.SetProperty<Engine::Renderers::Lighting::IVoxelBuffer>("float mips", &Engine::Renderers::Lighting::IVoxelBuffer::Mips);
				VLightingIVoxelBuffer.SetProperty<Engine::Renderers::Lighting::IVoxelBuffer>("vector3 scale", &Engine::Renderers::Lighting::IVoxelBuffer::Scale);
				VLightingIVoxelBuffer.SetProperty<Engine::Renderers::Lighting::IVoxelBuffer>("float max_steps", &Engine::Renderers::Lighting::IVoxelBuffer::MaxSteps);
				VLightingIVoxelBuffer.SetProperty<Engine::Renderers::Lighting::IVoxelBuffer>("vector3 lights", &Engine::Renderers::Lighting::IVoxelBuffer::Lights);
				VLightingIVoxelBuffer.SetProperty<Engine::Renderers::Lighting::IVoxelBuffer>("float radiance", &Engine::Renderers::Lighting::IVoxelBuffer::Radiance);
				VLightingIVoxelBuffer.SetProperty<Engine::Renderers::Lighting::IVoxelBuffer>("float margin", &Engine::Renderers::Lighting::IVoxelBuffer::Margin);
				VLightingIVoxelBuffer.SetProperty<Engine::Renderers::Lighting::IVoxelBuffer>("float offset", &Engine::Renderers::Lighting::IVoxelBuffer::Offset);
				VLightingIVoxelBuffer.SetProperty<Engine::Renderers::Lighting::IVoxelBuffer>("float angle", &Engine::Renderers::Lighting::IVoxelBuffer::Angle);
				VLightingIVoxelBuffer.SetProperty<Engine::Renderers::Lighting::IVoxelBuffer>("float length", &Engine::Renderers::Lighting::IVoxelBuffer::Length);
				VLightingIVoxelBuffer.SetProperty<Engine::Renderers::Lighting::IVoxelBuffer>("float distance", &Engine::Renderers::Lighting::IVoxelBuffer::Distance);
				VLightingIVoxelBuffer.SetProperty<Engine::Renderers::Lighting::IVoxelBuffer>("float occlusion", &Engine::Renderers::Lighting::IVoxelBuffer::Occlusion);
				VLightingIVoxelBuffer.SetProperty<Engine::Renderers::Lighting::IVoxelBuffer>("float specular", &Engine::Renderers::Lighting::IVoxelBuffer::Specular);
				VLightingIVoxelBuffer.SetProperty<Engine::Renderers::Lighting::IVoxelBuffer>("float bleeding", &Engine::Renderers::Lighting::IVoxelBuffer::Bleeding);

				RefClass VLighting = Engine->SetClass<Engine::Renderers::Lighting>("lighting_renderer", false);
				VLighting.SetProperty<Engine::Renderers::Lighting>("lighting_ambient_light ambient_light", &Engine::Renderers::Lighting::AmbientLight);
				VLighting.SetProperty<Engine::Renderers::Lighting>("lighting_voxel_buffer voxel_buffer", &Engine::Renderers::Lighting::VoxelBuffer);
				VLighting.SetProperty<Engine::Renderers::Lighting>("bool enable_gi", &Engine::Renderers::Lighting::EnableGI);
				VLighting.SetMethod("void set_sky_map(texture_2d@+)", &Engine::Renderers::Lighting::SetSkyMap);
				VLighting.SetMethod("void set_surface_buffer_size(usize)", &Engine::Renderers::Lighting::SetSurfaceBufferSize);
				VLighting.SetMethod("texture_cube@+ get_sky_map() const", &Engine::Renderers::Lighting::GetSkyMap);
				VLighting.SetMethod("texture_2d@+ get_sky_base() const", &Engine::Renderers::Lighting::GetSkyBase);
				PopulateRendererInterface<Engine::Renderers::Lighting, Engine::RenderSystem*>(VLighting, "lighting_renderer@+ f(render_system@+)");

				TypeClass VTransparencyRenderConstant = Engine->SetPod<Engine::Renderers::Transparency::RenderConstant>("transparency_render_constant");
				VTransparencyRenderConstant.SetProperty<Engine::Renderers::Transparency::RenderConstant>("vector3 padding", &Engine::Renderers::Transparency::RenderConstant::Padding);
				VTransparencyRenderConstant.SetProperty<Engine::Renderers::Transparency::RenderConstant>("float mips", &Engine::Renderers::Transparency::RenderConstant::Mips);

				RefClass VTransparency = Engine->SetClass<Engine::Renderers::Transparency>("transparency_renderer", false);
				VTransparency.SetProperty<Engine::Renderers::Transparency>("transparency_render_constant render_data", &Engine::Renderers::Transparency::RenderData);
				PopulateRendererInterface<Engine::Renderers::Transparency, Engine::RenderSystem*>(VTransparency, "transparency_renderer@+ f(render_system@+)");

				TypeClass VSSRReflectanceBuffer = Engine->SetPod<Engine::Renderers::SSR::ReflectanceBuffer>("ssr_reflectance_buffer");
				VSSRReflectanceBuffer.SetProperty<Engine::Renderers::SSR::ReflectanceBuffer>("float samples", &Engine::Renderers::SSR::ReflectanceBuffer::Samples);
				VSSRReflectanceBuffer.SetProperty<Engine::Renderers::SSR::ReflectanceBuffer>("float padding", &Engine::Renderers::SSR::ReflectanceBuffer::Padding);
				VSSRReflectanceBuffer.SetProperty<Engine::Renderers::SSR::ReflectanceBuffer>("float intensity", &Engine::Renderers::SSR::ReflectanceBuffer::Intensity);
				VSSRReflectanceBuffer.SetProperty<Engine::Renderers::SSR::ReflectanceBuffer>("float distance", &Engine::Renderers::SSR::ReflectanceBuffer::Distance);

				TypeClass VSSRGlossBuffer = Engine->SetPod<Engine::Renderers::SSR::GlossBuffer>("ssr_gloss_buffer");
				VSSRGlossBuffer.SetProperty<Engine::Renderers::SSR::GlossBuffer>("float padding", &Engine::Renderers::SSR::GlossBuffer::Padding);
				VSSRGlossBuffer.SetProperty<Engine::Renderers::SSR::GlossBuffer>("float deadzone", &Engine::Renderers::SSR::GlossBuffer::Deadzone);
				VSSRGlossBuffer.SetProperty<Engine::Renderers::SSR::GlossBuffer>("float mips", &Engine::Renderers::SSR::GlossBuffer::Mips);
				VSSRGlossBuffer.SetProperty<Engine::Renderers::SSR::GlossBuffer>("float cutoff", &Engine::Renderers::SSR::GlossBuffer::Cutoff);
				VSSRGlossBuffer.SetPropertyArray<Engine::Renderers::SSR::GlossBuffer>("float texel", &Engine::Renderers::SSR::GlossBuffer::Texel, 2);
				VSSRGlossBuffer.SetProperty<Engine::Renderers::SSR::GlossBuffer>("float samples", &Engine::Renderers::SSR::GlossBuffer::Samples);
				VSSRGlossBuffer.SetProperty<Engine::Renderers::SSR::GlossBuffer>("float blur", &Engine::Renderers::SSR::GlossBuffer::Blur);

				RefClass VSSR = Engine->SetClass<Engine::Renderers::SSR>("ssr_renderer", false);
				VSSR.SetProperty<Engine::Renderers::SSR>("ssr_reflectance_buffer reflectance", &Engine::Renderers::SSR::Reflectance);
				VSSR.SetProperty<Engine::Renderers::SSR>("ssr_gloss_buffer gloss", &Engine::Renderers::SSR::Gloss);
				PopulateRendererInterface<Engine::Renderers::SSR, Engine::RenderSystem*>(VSSR, "ssr_renderer@+ f(render_system@+)");

				TypeClass VSSGIStochasticBuffer = Engine->SetPod<Engine::Renderers::SSGI::StochasticBuffer>("ssgi_stochastic_buffer");
				VSSGIStochasticBuffer.SetPropertyArray<Engine::Renderers::SSGI::StochasticBuffer>("float texel", &Engine::Renderers::SSGI::StochasticBuffer::Texel, 2);
				VSSGIStochasticBuffer.SetProperty<Engine::Renderers::SSGI::StochasticBuffer>("float frame_id", &Engine::Renderers::SSGI::StochasticBuffer::FrameId);
				VSSGIStochasticBuffer.SetProperty<Engine::Renderers::SSGI::StochasticBuffer>("float padding", &Engine::Renderers::SSGI::StochasticBuffer::Padding);
				
				TypeClass VSSGIIndirectionBuffer = Engine->SetPod<Engine::Renderers::SSGI::IndirectionBuffer>("ssgi_indirection_buffer");
				VSSGIIndirectionBuffer.SetPropertyArray<Engine::Renderers::SSGI::IndirectionBuffer>("float random", &Engine::Renderers::SSGI::IndirectionBuffer::Random, 2);
				VSSGIIndirectionBuffer.SetProperty<Engine::Renderers::SSGI::IndirectionBuffer>("float samples", &Engine::Renderers::SSGI::IndirectionBuffer::Samples);
				VSSGIIndirectionBuffer.SetProperty<Engine::Renderers::SSGI::IndirectionBuffer>("float distance", &Engine::Renderers::SSGI::IndirectionBuffer::Distance);
				VSSGIIndirectionBuffer.SetProperty<Engine::Renderers::SSGI::IndirectionBuffer>("float initial", &Engine::Renderers::SSGI::IndirectionBuffer::Initial);
				VSSGIIndirectionBuffer.SetProperty<Engine::Renderers::SSGI::IndirectionBuffer>("float cutoff", &Engine::Renderers::SSGI::IndirectionBuffer::Cutoff);
				VSSGIIndirectionBuffer.SetProperty<Engine::Renderers::SSGI::IndirectionBuffer>("float attenuations", &Engine::Renderers::SSGI::IndirectionBuffer::Attenuation);
				VSSGIIndirectionBuffer.SetProperty<Engine::Renderers::SSGI::IndirectionBuffer>("float swing", &Engine::Renderers::SSGI::IndirectionBuffer::Swing);
				VSSGIIndirectionBuffer.SetPropertyArray<Engine::Renderers::SSGI::IndirectionBuffer>("float padding", &Engine::Renderers::SSGI::IndirectionBuffer::Padding, 3);
				VSSGIIndirectionBuffer.SetProperty<Engine::Renderers::SSGI::IndirectionBuffer>("float bias", &Engine::Renderers::SSGI::IndirectionBuffer::Bias);

				TypeClass VSSGIDenoiseBuffer = Engine->SetPod<Engine::Renderers::SSGI::DenoiseBuffer>("ssgi_denoise_buffer");
				VSSGIDenoiseBuffer.SetPropertyArray<Engine::Renderers::SSGI::DenoiseBuffer>("float padding", &Engine::Renderers::SSGI::DenoiseBuffer::Padding, 3);
				VSSGIDenoiseBuffer.SetProperty<Engine::Renderers::SSGI::DenoiseBuffer>("float cutoff", &Engine::Renderers::SSGI::DenoiseBuffer::Cutoff);
				VSSGIDenoiseBuffer.SetPropertyArray<Engine::Renderers::SSGI::DenoiseBuffer>("float texel", &Engine::Renderers::SSGI::DenoiseBuffer::Texel, 2);
				VSSGIDenoiseBuffer.SetProperty<Engine::Renderers::SSGI::DenoiseBuffer>("float samples", &Engine::Renderers::SSGI::DenoiseBuffer::Samples);
				VSSGIDenoiseBuffer.SetProperty<Engine::Renderers::SSGI::DenoiseBuffer>("float blur", &Engine::Renderers::SSGI::DenoiseBuffer::Blur);

				RefClass VSSGI = Engine->SetClass<Engine::Renderers::SSGI>("ssgi_renderer", false);
				VSSGI.SetProperty<Engine::Renderers::SSGI>("ssgi_stochastic_buffer stochastic", &Engine::Renderers::SSGI::Stochastic);
				VSSGI.SetProperty<Engine::Renderers::SSGI>("ssgi_indirection_buffer indirection", &Engine::Renderers::SSGI::Indirection);
				VSSGI.SetProperty<Engine::Renderers::SSGI>("ssgi_denoise_buffer denoise", &Engine::Renderers::SSGI::Indirection);
				VSSGI.SetProperty<Engine::Renderers::SSGI>("uint32 bounces", &Engine::Renderers::SSGI::Bounces);
				PopulateRendererInterface<Engine::Renderers::SSGI, Engine::RenderSystem*>(VSSGI, "ssgi_renderer@+ f(render_system@+)");

				TypeClass VSSAOShadingBuffer = Engine->SetPod<Engine::Renderers::SSAO::ShadingBuffer>("ssao_shading_buffer");
				VSSAOShadingBuffer.SetProperty<Engine::Renderers::SSAO::ShadingBuffer>("float samples", &Engine::Renderers::SSAO::ShadingBuffer::Samples);
				VSSAOShadingBuffer.SetProperty<Engine::Renderers::SSAO::ShadingBuffer>("float intensity", &Engine::Renderers::SSAO::ShadingBuffer::Intensity);
				VSSAOShadingBuffer.SetProperty<Engine::Renderers::SSAO::ShadingBuffer>("float scale", &Engine::Renderers::SSAO::ShadingBuffer::Scale);
				VSSAOShadingBuffer.SetProperty<Engine::Renderers::SSAO::ShadingBuffer>("float bias", &Engine::Renderers::SSAO::ShadingBuffer::Bias);
				VSSAOShadingBuffer.SetProperty<Engine::Renderers::SSAO::ShadingBuffer>("float radius", &Engine::Renderers::SSAO::ShadingBuffer::Radius);
				VSSAOShadingBuffer.SetProperty<Engine::Renderers::SSAO::ShadingBuffer>("float distance", &Engine::Renderers::SSAO::ShadingBuffer::Distance);
				VSSAOShadingBuffer.SetProperty<Engine::Renderers::SSAO::ShadingBuffer>("float fade", &Engine::Renderers::SSAO::ShadingBuffer::Fade);
				VSSAOShadingBuffer.SetProperty<Engine::Renderers::SSAO::ShadingBuffer>("float padding", &Engine::Renderers::SSAO::ShadingBuffer::Padding);

				TypeClass VSSAOFiboBuffer = Engine->SetPod<Engine::Renderers::SSAO::FiboBuffer>("ssao_fibo_buffer");
				VSSAOFiboBuffer.SetPropertyArray<Engine::Renderers::SSAO::FiboBuffer>("float padding", &Engine::Renderers::SSAO::FiboBuffer::Padding, 3);
				VSSAOFiboBuffer.SetProperty<Engine::Renderers::SSAO::FiboBuffer>("float power", &Engine::Renderers::SSAO::FiboBuffer::Power);
				VSSAOFiboBuffer.SetPropertyArray<Engine::Renderers::SSAO::FiboBuffer>("float texel", &Engine::Renderers::SSAO::FiboBuffer::Texel, 2);
				VSSAOFiboBuffer.SetProperty<Engine::Renderers::SSAO::FiboBuffer>("float samples", &Engine::Renderers::SSAO::FiboBuffer::Samples);
				VSSAOFiboBuffer.SetProperty<Engine::Renderers::SSAO::FiboBuffer>("float blur", &Engine::Renderers::SSAO::FiboBuffer::Blur);

				RefClass VSSAO = Engine->SetClass<Engine::Renderers::SSAO>("ssao_renderer", false);
				VSSAO.SetProperty<Engine::Renderers::SSAO>("ssao_shading_buffer shading", &Engine::Renderers::SSAO::Shading);
				VSSAO.SetProperty<Engine::Renderers::SSAO>("ssao_fibo_buffer fibo", &Engine::Renderers::SSAO::Fibo);
				PopulateRendererInterface<Engine::Renderers::SSAO, Engine::RenderSystem*>(VSSAO, "ssao_renderer@+ f(render_system@+)");

				TypeClass VDoFFocusBuffer = Engine->SetPod<Engine::Renderers::DoF::FocusBuffer>("dof_focus_buffer");
				VDoFFocusBuffer.SetPropertyArray<Engine::Renderers::DoF::FocusBuffer>("float texel", &Engine::Renderers::DoF::FocusBuffer::Texel, 2);
				VDoFFocusBuffer.SetProperty<Engine::Renderers::DoF::FocusBuffer>("float radius", &Engine::Renderers::DoF::FocusBuffer::Radius);
				VDoFFocusBuffer.SetProperty<Engine::Renderers::DoF::FocusBuffer>("float bokeh", &Engine::Renderers::DoF::FocusBuffer::Bokeh);
				VDoFFocusBuffer.SetPropertyArray<Engine::Renderers::DoF::FocusBuffer>("float padding", &Engine::Renderers::DoF::FocusBuffer::Padding, 3);
				VDoFFocusBuffer.SetProperty<Engine::Renderers::DoF::FocusBuffer>("float scale", &Engine::Renderers::DoF::FocusBuffer::Scale);
				VDoFFocusBuffer.SetProperty<Engine::Renderers::DoF::FocusBuffer>("float near_distance", &Engine::Renderers::DoF::FocusBuffer::NearDistance);
				VDoFFocusBuffer.SetProperty<Engine::Renderers::DoF::FocusBuffer>("float near_range", &Engine::Renderers::DoF::FocusBuffer::NearRange);
				VDoFFocusBuffer.SetProperty<Engine::Renderers::DoF::FocusBuffer>("float far_distance", &Engine::Renderers::DoF::FocusBuffer::FarDistance);
				VDoFFocusBuffer.SetProperty<Engine::Renderers::DoF::FocusBuffer>("float far_range", &Engine::Renderers::DoF::FocusBuffer::FarRange);

				RefClass VDoF = Engine->SetClass<Engine::Renderers::DoF>("dof_renderer", false);
				VDoF.SetProperty<Engine::Renderers::DoF>("dof_focus_buffer focus", &Engine::Renderers::DoF::Focus);
				VDoF.SetProperty<Engine::Renderers::DoF>("float distance", &Engine::Renderers::DoF::Distance);
				VDoF.SetProperty<Engine::Renderers::DoF>("float radius", &Engine::Renderers::DoF::Radius);
				VDoF.SetProperty<Engine::Renderers::DoF>("float time", &Engine::Renderers::DoF::Time);
				VDoF.SetMethod("void focus_at_nearest_target(float)", &Engine::Renderers::DoF::FocusAtNearestTarget);
				PopulateRendererInterface<Engine::Renderers::DoF, Engine::RenderSystem*>(VDoF, "dof_renderer@+ f(render_system@+)");

				TypeClass VMotionBlurVelocityBuffer = Engine->SetPod<Engine::Renderers::MotionBlur::VelocityBuffer>("motion_blur_velocity_buffer");
				VMotionBlurVelocityBuffer.SetProperty<Engine::Renderers::MotionBlur::VelocityBuffer>("matrix4x4 last_view_projection", &Engine::Renderers::MotionBlur::VelocityBuffer::LastViewProjection);

				TypeClass VMotionBlurMotionBuffer = Engine->SetPod<Engine::Renderers::MotionBlur::MotionBuffer>("motion_blur_motion_buffer");
				VMotionBlurMotionBuffer.SetProperty<Engine::Renderers::MotionBlur::MotionBuffer>("float samples", &Engine::Renderers::MotionBlur::MotionBuffer::Samples);
				VMotionBlurMotionBuffer.SetProperty<Engine::Renderers::MotionBlur::MotionBuffer>("float blur", &Engine::Renderers::MotionBlur::MotionBuffer::Blur);
				VMotionBlurMotionBuffer.SetProperty<Engine::Renderers::MotionBlur::MotionBuffer>("float motion", &Engine::Renderers::MotionBlur::MotionBuffer::Motion);
				VMotionBlurMotionBuffer.SetProperty<Engine::Renderers::MotionBlur::MotionBuffer>("float padding", &Engine::Renderers::MotionBlur::MotionBuffer::Padding);

				RefClass VMotionBlur = Engine->SetClass<Engine::Renderers::MotionBlur>("motion_blur_renderer", false);
				VMotionBlur.SetProperty<Engine::Renderers::MotionBlur>("motion_blur_velocity_buffer velocity", &Engine::Renderers::MotionBlur::Velocity);
				VMotionBlur.SetProperty<Engine::Renderers::MotionBlur>("motion_blur_motion_buffer motion", &Engine::Renderers::MotionBlur::Motion);
				PopulateRendererInterface<Engine::Renderers::MotionBlur, Engine::RenderSystem*>(VMotionBlur, "motion_blur_renderer@+ f(render_system@+)");

				TypeClass VBloomExtractionBuffer = Engine->SetPod<Engine::Renderers::Bloom::ExtractionBuffer>("bloom_extraction_buffer");
				VBloomExtractionBuffer.SetPropertyArray<Engine::Renderers::Bloom::ExtractionBuffer>("float padding", &Engine::Renderers::Bloom::ExtractionBuffer::Padding, 2);
				VBloomExtractionBuffer.SetProperty<Engine::Renderers::Bloom::ExtractionBuffer>("float intensity", &Engine::Renderers::Bloom::ExtractionBuffer::Intensity);
				VBloomExtractionBuffer.SetProperty<Engine::Renderers::Bloom::ExtractionBuffer>("float threshold", &Engine::Renderers::Bloom::ExtractionBuffer::Threshold);

				TypeClass VBloomFiboBuffer = Engine->SetPod<Engine::Renderers::Bloom::FiboBuffer>("bloom_fibo_buffer");
				VBloomFiboBuffer.SetPropertyArray<Engine::Renderers::Bloom::FiboBuffer>("float padding", &Engine::Renderers::Bloom::FiboBuffer::Padding, 3);
				VBloomFiboBuffer.SetProperty<Engine::Renderers::Bloom::FiboBuffer>("float power", &Engine::Renderers::Bloom::FiboBuffer::Power);
				VBloomFiboBuffer.SetPropertyArray<Engine::Renderers::Bloom::FiboBuffer>("float texel", &Engine::Renderers::Bloom::FiboBuffer::Texel, 2);
				VBloomFiboBuffer.SetProperty<Engine::Renderers::Bloom::FiboBuffer>("float samples", &Engine::Renderers::Bloom::FiboBuffer::Samples);
				VBloomFiboBuffer.SetProperty<Engine::Renderers::Bloom::FiboBuffer>("float blur", &Engine::Renderers::Bloom::FiboBuffer::Blur);

				RefClass VBloom = Engine->SetClass<Engine::Renderers::Bloom>("bloom_renderer", false);
				VBloom.SetProperty<Engine::Renderers::Bloom>("bloom_extraction_buffer extraction", &Engine::Renderers::Bloom::Extraction);
				VBloom.SetProperty<Engine::Renderers::Bloom>("bloom_fibo_buffer fibo", &Engine::Renderers::Bloom::Fibo);
				PopulateRendererInterface<Engine::Renderers::Bloom, Engine::RenderSystem*>(VBloom, "bloom_renderer@+ f(render_system@+)");

				TypeClass VToneLuminanceBuffer = Engine->SetPod<Engine::Renderers::Tone::LuminanceBuffer>("tone_luminance_buffer");
				VToneLuminanceBuffer.SetPropertyArray<Engine::Renderers::Tone::LuminanceBuffer>("float texel", &Engine::Renderers::Tone::LuminanceBuffer::Texel, 2);
				VToneLuminanceBuffer.SetProperty<Engine::Renderers::Tone::LuminanceBuffer>("float mips", &Engine::Renderers::Tone::LuminanceBuffer::Mips);
				VToneLuminanceBuffer.SetProperty<Engine::Renderers::Tone::LuminanceBuffer>("float time", &Engine::Renderers::Tone::LuminanceBuffer::Time);

				TypeClass VToneMappingBuffer = Engine->SetPod<Engine::Renderers::Tone::MappingBuffer>("tone_mapping_buffer");
				VToneMappingBuffer.SetPropertyArray<Engine::Renderers::Tone::MappingBuffer>("float padding", &Engine::Renderers::Tone::MappingBuffer::Padding, 2);
				VToneMappingBuffer.SetProperty<Engine::Renderers::Tone::MappingBuffer>("float grayscale", &Engine::Renderers::Tone::MappingBuffer::Grayscale);
				VToneMappingBuffer.SetProperty<Engine::Renderers::Tone::MappingBuffer>("float aces", &Engine::Renderers::Tone::MappingBuffer::ACES);
				VToneMappingBuffer.SetProperty<Engine::Renderers::Tone::MappingBuffer>("float filmic", &Engine::Renderers::Tone::MappingBuffer::Filmic);
				VToneMappingBuffer.SetProperty<Engine::Renderers::Tone::MappingBuffer>("float lottes", &Engine::Renderers::Tone::MappingBuffer::Lottes);
				VToneMappingBuffer.SetProperty<Engine::Renderers::Tone::MappingBuffer>("float reinhard", &Engine::Renderers::Tone::MappingBuffer::Reinhard);
				VToneMappingBuffer.SetProperty<Engine::Renderers::Tone::MappingBuffer>("float reinhard2", &Engine::Renderers::Tone::MappingBuffer::Reinhard2);
				VToneMappingBuffer.SetProperty<Engine::Renderers::Tone::MappingBuffer>("float unreal", &Engine::Renderers::Tone::MappingBuffer::Unreal);
				VToneMappingBuffer.SetProperty<Engine::Renderers::Tone::MappingBuffer>("float uchimura", &Engine::Renderers::Tone::MappingBuffer::Uchimura);
				VToneMappingBuffer.SetProperty<Engine::Renderers::Tone::MappingBuffer>("float ubrightness", &Engine::Renderers::Tone::MappingBuffer::UBrightness);
				VToneMappingBuffer.SetProperty<Engine::Renderers::Tone::MappingBuffer>("float ucontrast", &Engine::Renderers::Tone::MappingBuffer::UContrast);
				VToneMappingBuffer.SetProperty<Engine::Renderers::Tone::MappingBuffer>("float ustart", &Engine::Renderers::Tone::MappingBuffer::UStart);
				VToneMappingBuffer.SetProperty<Engine::Renderers::Tone::MappingBuffer>("float ulength", &Engine::Renderers::Tone::MappingBuffer::ULength);
				VToneMappingBuffer.SetProperty<Engine::Renderers::Tone::MappingBuffer>("float ublack", &Engine::Renderers::Tone::MappingBuffer::UBlack);
				VToneMappingBuffer.SetProperty<Engine::Renderers::Tone::MappingBuffer>("float upedestal", &Engine::Renderers::Tone::MappingBuffer::UPedestal);
				VToneMappingBuffer.SetProperty<Engine::Renderers::Tone::MappingBuffer>("float exposure", &Engine::Renderers::Tone::MappingBuffer::Exposure);
				VToneMappingBuffer.SetProperty<Engine::Renderers::Tone::MappingBuffer>("float eintensity", &Engine::Renderers::Tone::MappingBuffer::EIntensity);
				VToneMappingBuffer.SetProperty<Engine::Renderers::Tone::MappingBuffer>("float egamma", &Engine::Renderers::Tone::MappingBuffer::EGamma);
				VToneMappingBuffer.SetProperty<Engine::Renderers::Tone::MappingBuffer>("float adaptation", &Engine::Renderers::Tone::MappingBuffer::Adaptation);
				VToneMappingBuffer.SetProperty<Engine::Renderers::Tone::MappingBuffer>("float ahray", &Engine::Renderers::Tone::MappingBuffer::AGray);
				VToneMappingBuffer.SetProperty<Engine::Renderers::Tone::MappingBuffer>("float awhite", &Engine::Renderers::Tone::MappingBuffer::AWhite);
				VToneMappingBuffer.SetProperty<Engine::Renderers::Tone::MappingBuffer>("float ablack", &Engine::Renderers::Tone::MappingBuffer::ABlack);
				VToneMappingBuffer.SetProperty<Engine::Renderers::Tone::MappingBuffer>("float aspeed", &Engine::Renderers::Tone::MappingBuffer::ASpeed);

				RefClass VTone = Engine->SetClass<Engine::Renderers::Tone>("tone_renderer", false);
				VTone.SetProperty<Engine::Renderers::Tone>("tone_luminance_buffer luminance", &Engine::Renderers::Tone::Luminance);
				VTone.SetProperty<Engine::Renderers::Tone>("tone_mapping_buffer mapping", &Engine::Renderers::Tone::Mapping);
				PopulateRendererInterface<Engine::Renderers::Tone, Engine::RenderSystem*>(VTone, "tone_renderer@+ f(render_system@+)");

				TypeClass VGlitchDistortionBuffer = Engine->SetPod<Engine::Renderers::Glitch::DistortionBuffer>("glitch_distortion_buffer");
				VGlitchDistortionBuffer.SetProperty<Engine::Renderers::Glitch::DistortionBuffer>("float scan_line_jitter_displacement", &Engine::Renderers::Glitch::DistortionBuffer::ScanLineJitterDisplacement);
				VGlitchDistortionBuffer.SetProperty<Engine::Renderers::Glitch::DistortionBuffer>("float scan_line_jitter_threshold", &Engine::Renderers::Glitch::DistortionBuffer::ScanLineJitterThreshold);
				VGlitchDistortionBuffer.SetProperty<Engine::Renderers::Glitch::DistortionBuffer>("float vertical_jump_amount", &Engine::Renderers::Glitch::DistortionBuffer::VerticalJumpAmount);
				VGlitchDistortionBuffer.SetProperty<Engine::Renderers::Glitch::DistortionBuffer>("float vertical_jump_time", &Engine::Renderers::Glitch::DistortionBuffer::VerticalJumpTime);
				VGlitchDistortionBuffer.SetProperty<Engine::Renderers::Glitch::DistortionBuffer>("float color_drift_amount", &Engine::Renderers::Glitch::DistortionBuffer::ColorDriftAmount);
				VGlitchDistortionBuffer.SetProperty<Engine::Renderers::Glitch::DistortionBuffer>("float color_drift_time", &Engine::Renderers::Glitch::DistortionBuffer::ColorDriftTime);
				VGlitchDistortionBuffer.SetProperty<Engine::Renderers::Glitch::DistortionBuffer>("float horizontal_shake", &Engine::Renderers::Glitch::DistortionBuffer::HorizontalShake);
				VGlitchDistortionBuffer.SetProperty<Engine::Renderers::Glitch::DistortionBuffer>("float elapsed_time", &Engine::Renderers::Glitch::DistortionBuffer::ElapsedTime);

				RefClass VGlitch = Engine->SetClass<Engine::Renderers::Glitch>("glitch_renderer", false);
				VGlitch.SetProperty<Engine::Renderers::Glitch>("glitch_distortion_buffer distortion", &Engine::Renderers::Glitch::Distortion);
				VGlitch.SetProperty<Engine::Renderers::Glitch>("float scan_line_jitter", &Engine::Renderers::Glitch::ScanLineJitter);
				VGlitch.SetProperty<Engine::Renderers::Glitch>("float vertical_jump", &Engine::Renderers::Glitch::VerticalJump);
				VGlitch.SetProperty<Engine::Renderers::Glitch>("float horizontal_shake", &Engine::Renderers::Glitch::HorizontalShake);
				VGlitch.SetProperty<Engine::Renderers::Glitch>("float color_drift", &Engine::Renderers::Glitch::ColorDrift);
				PopulateRendererInterface<Engine::Renderers::Glitch, Engine::RenderSystem*>(VGlitch, "glitch_renderer@+ f(render_system@+)");

				RefClass VUserInterface = Engine->SetClass<Engine::Renderers::UserInterface>("gui_renderer", false);
				VUserInterface.SetMethod("gui_context@+ get_context() const", &Engine::Renderers::UserInterface::GetContext);
				PopulateRendererInterface<Engine::Renderers::UserInterface, Engine::RenderSystem*>(VUserInterface, "gui_renderer@+ f(render_system@+)");

				return true;
#else
				VI_ASSERT(false, false, "<renderers> is not loaded");
				return false;
#endif
			}
			bool Registry::LoadUiControl(VirtualMachine* Engine)
			{
#ifdef VI_HAS_BINDINGS
				VI_ASSERT(Engine != nullptr, false, "manager should be set");
				VI_TYPEREF(ModelListenerName, "ui_listener");

				Enumeration VEventPhase = Engine->SetEnum("ui_event_phase");
				Enumeration VArea = Engine->SetEnum("ui_area");
				Enumeration VDisplay = Engine->SetEnum("ui_display");
				Enumeration VPosition = Engine->SetEnum("ui_position");
				Enumeration VFloat = Engine->SetEnum("ui_float");
				Enumeration VTimingFunc = Engine->SetEnum("ui_timing_func");
				Enumeration VTimingDir = Engine->SetEnum("ui_timing_dir");
				Enumeration VFocusFlag = Engine->SetEnum("ui_focus_flag");
				Enumeration VModalFlag = Engine->SetEnum("ui_modal_flag");
				TypeClass VElement = Engine->SetStructTrivial<Engine::GUI::IElement>("ui_element");
				TypeClass VDocument = Engine->SetStructTrivial<Engine::GUI::IElementDocument>("ui_document");
				TypeClass VEvent = Engine->SetStructTrivial<Engine::GUI::IEvent>("ui_event");
				RefClass VListener = Engine->SetClass<ModelListener>("ui_listener", true);
				Engine->SetFunctionDef("void model_listener_event(ui_event &in)");

				VModalFlag.SetValue("none", (int)Engine::GUI::ModalFlag::None);
				VModalFlag.SetValue("modal", (int)Engine::GUI::ModalFlag::Modal);
				VModalFlag.SetValue("keep", (int)Engine::GUI::ModalFlag::Keep);

				VFocusFlag.SetValue("none", (int)Engine::GUI::FocusFlag::None);
				VFocusFlag.SetValue("document", (int)Engine::GUI::FocusFlag::Document);
				VFocusFlag.SetValue("keep", (int)Engine::GUI::FocusFlag::Keep);
				VFocusFlag.SetValue("automatic", (int)Engine::GUI::FocusFlag::Auto);

				VEventPhase.SetValue("none", (int)Engine::GUI::EventPhase::None);
				VEventPhase.SetValue("capture", (int)Engine::GUI::EventPhase::Capture);
				VEventPhase.SetValue("target", (int)Engine::GUI::EventPhase::Target);
				VEventPhase.SetValue("bubble", (int)Engine::GUI::EventPhase::Bubble);

				VEvent.SetConstructor<Engine::GUI::IEvent>("void f()");
				VEvent.SetConstructor<Engine::GUI::IEvent, Rml::Event*>("void f(uptr@)");
				VEvent.SetMethod("ui_event_phase get_phase() const", &Engine::GUI::IEvent::GetPhase);
				VEvent.SetMethod("void set_phase(ui_event_phase Phase)", &Engine::GUI::IEvent::SetPhase);
				VEvent.SetMethod("void set_current_element(const ui_element &in)", &Engine::GUI::IEvent::SetCurrentElement);
				VEvent.SetMethod("ui_element get_current_element() const", &Engine::GUI::IEvent::GetCurrentElement);
				VEvent.SetMethod("ui_element get_target_element() const", &Engine::GUI::IEvent::GetTargetElement);
				VEvent.SetMethod("string get_type() const", &Engine::GUI::IEvent::GetType);
				VEvent.SetMethod("void stop_propagation()", &Engine::GUI::IEvent::StopPropagation);
				VEvent.SetMethod("void stop_immediate_propagation()", &Engine::GUI::IEvent::StopImmediatePropagation);
				VEvent.SetMethod("bool is_interruptible() const", &Engine::GUI::IEvent::IsInterruptible);
				VEvent.SetMethod("bool is_propagating() const", &Engine::GUI::IEvent::IsPropagating);
				VEvent.SetMethod("bool is_immediate_propagating() const", &Engine::GUI::IEvent::IsImmediatePropagating);
				VEvent.SetMethod("bool get_boolean(const string &in) const", &Engine::GUI::IEvent::GetBoolean);
				VEvent.SetMethod("int64 get_integer(const string &in) const", &Engine::GUI::IEvent::GetInteger);
				VEvent.SetMethod("double get_number(const string &in) const", &Engine::GUI::IEvent::GetNumber);
				VEvent.SetMethod("string get_string(const string &in) const", &Engine::GUI::IEvent::GetString);
				VEvent.SetMethod("vector2 get_vector2(const string &in) const", &Engine::GUI::IEvent::GetVector2);
				VEvent.SetMethod("vector3 get_vector3(const string &in) const", &Engine::GUI::IEvent::GetVector3);
				VEvent.SetMethod("vector4 get_vector4(const string &in) const", &Engine::GUI::IEvent::GetVector4);
				VEvent.SetMethod("uptr@ get_pointer(const string &in) const", &Engine::GUI::IEvent::GetPointer);
				VEvent.SetMethod("uptr@ get_event() const", &Engine::GUI::IEvent::GetEvent);
				VEvent.SetMethod("bool is_valid() const", &Engine::GUI::IEvent::IsValid);

				VListener.SetGcConstructor<ModelListener, ModelListenerName, asIScriptFunction*>("ui_listener@ f(model_listener_event@)");
				VListener.SetGcConstructor<ModelListener, ModelListenerName, const Core::String&>("ui_listener@ f(const string &in)");
				VListener.SetEnumRefsEx<ModelListener>([](ModelListener* Base, asIScriptEngine* Engine)
				{
					Engine->GCEnumCallback(Base->GetCallback());
				});
				VListener.SetReleaseRefsEx<ModelListener>([](ModelListener* Base, asIScriptEngine*)
				{
					Base->~ModelListener();
				});

				VArea.SetValue("margin", (int)Engine::GUI::Area::Margin);
				VArea.SetValue("border", (int)Engine::GUI::Area::Border);
				VArea.SetValue("padding", (int)Engine::GUI::Area::Padding);
				VArea.SetValue("content", (int)Engine::GUI::Area::Content);

				VDisplay.SetValue("none", (int)Engine::GUI::Display::None);
				VDisplay.SetValue("block", (int)Engine::GUI::Display::Block);
				VDisplay.SetValue("inline", (int)Engine::GUI::Display::Inline);
				VDisplay.SetValue("inline_block", (int)Engine::GUI::Display::InlineBlock);
				VDisplay.SetValue("flex", (int)Engine::GUI::Display::Flex);
				VDisplay.SetValue("table", (int)Engine::GUI::Display::Table);
				VDisplay.SetValue("table_row", (int)Engine::GUI::Display::TableRow);
				VDisplay.SetValue("table_row_group", (int)Engine::GUI::Display::TableRowGroup);
				VDisplay.SetValue("table_column", (int)Engine::GUI::Display::TableColumn);
				VDisplay.SetValue("table_column_group", (int)Engine::GUI::Display::TableColumnGroup);
				VDisplay.SetValue("table_cell", (int)Engine::GUI::Display::TableCell);

				VPosition.SetValue("static", (int)Engine::GUI::Position::Static);
				VPosition.SetValue("relative", (int)Engine::GUI::Position::Relative);
				VPosition.SetValue("absolute", (int)Engine::GUI::Position::Absolute);
				VPosition.SetValue("fixed", (int)Engine::GUI::Position::Fixed);

				VFloat.SetValue("none", (int)Engine::GUI::Float::None);
				VFloat.SetValue("left", (int)Engine::GUI::Float::Left);
				VFloat.SetValue("right", (int)Engine::GUI::Float::Right);

				VTimingFunc.SetValue("none", (int)Engine::GUI::TimingFunc::None);
				VTimingFunc.SetValue("back", (int)Engine::GUI::TimingFunc::Back);
				VTimingFunc.SetValue("bounce", (int)Engine::GUI::TimingFunc::Bounce);
				VTimingFunc.SetValue("circular", (int)Engine::GUI::TimingFunc::Circular);
				VTimingFunc.SetValue("cubic", (int)Engine::GUI::TimingFunc::Cubic);
				VTimingFunc.SetValue("elastic", (int)Engine::GUI::TimingFunc::Elastic);
				VTimingFunc.SetValue("exponential", (int)Engine::GUI::TimingFunc::Exponential);
				VTimingFunc.SetValue("linear", (int)Engine::GUI::TimingFunc::Linear);
				VTimingFunc.SetValue("quadratic", (int)Engine::GUI::TimingFunc::Quadratic);
				VTimingFunc.SetValue("quartic", (int)Engine::GUI::TimingFunc::Quartic);
				VTimingFunc.SetValue("sine", (int)Engine::GUI::TimingFunc::Sine);
				VTimingFunc.SetValue("callback", (int)Engine::GUI::TimingFunc::Callback);

				VTimingDir.SetValue("use_in", (int)Engine::GUI::TimingDir::In);
				VTimingDir.SetValue("use_out", (int)Engine::GUI::TimingDir::Out);
				VTimingDir.SetValue("use_in_out", (int)Engine::GUI::TimingDir::InOut);
				
				VElement.SetConstructor<Engine::GUI::IElement>("void f()");
				VElement.SetConstructor<Engine::GUI::IElement, Rml::Element*>("void f(uptr@)");
				VElement.SetMethod("ui_element clone() const", &Engine::GUI::IElement::Clone);
				VElement.SetMethod("void set_class(const string &in, bool)", &Engine::GUI::IElement::SetClass);
				VElement.SetMethod("bool is_class_set(const string &in) const", &Engine::GUI::IElement::IsClassSet);
				VElement.SetMethod("void set_class_names(const string &in)", &Engine::GUI::IElement::SetClassNames);
				VElement.SetMethod("string get_class_names() const", &Engine::GUI::IElement::GetClassNames);
				VElement.SetMethod("string get_address(bool = false, bool = true) const", &Engine::GUI::IElement::GetAddress);
				VElement.SetMethod("void set_offset(const vector2 &in, const ui_element &in, bool = false)", &Engine::GUI::IElement::SetOffset);
				VElement.SetMethod("vector2 get_relative_offset(ui_area = ui_area::Content) const", &Engine::GUI::IElement::GetRelativeOffset);
				VElement.SetMethod("vector2 get_absolute_offset(ui_area = ui_area::Content) const", &Engine::GUI::IElement::GetAbsoluteOffset);
				VElement.SetMethod("void set_client_area(ui_area)", &Engine::GUI::IElement::SetClientArea);
				VElement.SetMethod("ui_area get_client_area() const", &Engine::GUI::IElement::GetClientArea);
				VElement.SetMethod("void set_bontent_box(const vector2 &in, const vector2 &in)", &Engine::GUI::IElement::SetContentBox);
				VElement.SetMethod("float get_baseline() const", &Engine::GUI::IElement::GetBaseline);
				VElement.SetMethod("bool get_intrinsic_dimensions(vector2 &out, float &out)", &Engine::GUI::IElement::GetIntrinsicDimensions);
				VElement.SetMethod("bool is_point_within_element(const vector2 &in)", &Engine::GUI::IElement::IsPointWithinElement);
				VElement.SetMethod("bool is_visible() const", &Engine::GUI::IElement::IsVisible);
				VElement.SetMethod("float get_zindex() const", &Engine::GUI::IElement::GetZIndex);
				VElement.SetMethod("bool set_property(const string &in, const string &in)", &Engine::GUI::IElement::SetProperty);
				VElement.SetMethod("void remove_property(const string &in)", &Engine::GUI::IElement::RemoveProperty);
				VElement.SetMethod("string get_property(const string &in) const", &Engine::GUI::IElement::GetProperty);
				VElement.SetMethod("string get_local_property(const string &in) const", &Engine::GUI::IElement::GetLocalProperty);
				VElement.SetMethod("float resolve_numeric_property(const string &in) const", &Engine::GUI::IElement::ResolveNumericProperty);
				VElement.SetMethod("vector2 get_containing_block() const", &Engine::GUI::IElement::GetContainingBlock);
				VElement.SetMethod("ui_position get_position() const", &Engine::GUI::IElement::GetPosition);
				VElement.SetMethod("ui_float get_float() const", &Engine::GUI::IElement::GetFloat);
				VElement.SetMethod("ui_display get_display() const", &Engine::GUI::IElement::GetDisplay);
				VElement.SetMethod("float get_line_height() const", &Engine::GUI::IElement::GetLineHeight);
				VElement.SetMethod("bool project(vector2 &out) const", &Engine::GUI::IElement::Project);
				VElement.SetMethod("bool animate(const string &in, const string &in, float, ui_timing_func, ui_timing_dir, int = -1, bool = true, float = 0)", &Engine::GUI::IElement::Animate);
				VElement.SetMethod("bool add_animation_key(const string &in, const string &in, float, ui_timing_func, ui_timing_dir)", &Engine::GUI::IElement::AddAnimationKey);
				VElement.SetMethod("void set_pseudo_Class(const string &in, bool)", &Engine::GUI::IElement::SetPseudoClass);
				VElement.SetMethod("bool is_pseudo_class_set(const string &in) const", &Engine::GUI::IElement::IsPseudoClassSet);
				VElement.SetMethod("void set_attribute(const string &in, const string &in)", &Engine::GUI::IElement::SetAttribute);
				VElement.SetMethod("string get_attribute(const string &in) const", &Engine::GUI::IElement::GetAttribute);
				VElement.SetMethod("bool has_attribute(const string &in) const", &Engine::GUI::IElement::HasAttribute);
				VElement.SetMethod("void remove_attribute(const string &in)", &Engine::GUI::IElement::RemoveAttribute);
				VElement.SetMethod("ui_element get_focus_leaf_node()", &Engine::GUI::IElement::GetFocusLeafNode);
				VElement.SetMethod("string get_tag_name() const", &Engine::GUI::IElement::GetTagName);
				VElement.SetMethod("string get_id() const", &Engine::GUI::IElement::GetId);
				VElement.SetMethod("float get_absolute_left()", &Engine::GUI::IElement::GetAbsoluteLeft);
				VElement.SetMethod("float get_absolute_top()", &Engine::GUI::IElement::GetAbsoluteTop);
				VElement.SetMethod("float get_client_left()", &Engine::GUI::IElement::GetClientLeft);
				VElement.SetMethod("float get_client_top()", &Engine::GUI::IElement::GetClientTop);
				VElement.SetMethod("float get_client_width()", &Engine::GUI::IElement::GetClientWidth);
				VElement.SetMethod("float get_client_height()", &Engine::GUI::IElement::GetClientHeight);
				VElement.SetMethod("ui_element get_offset_parent()", &Engine::GUI::IElement::GetOffsetParent);
				VElement.SetMethod("float get_offset_left()", &Engine::GUI::IElement::GetOffsetLeft);
				VElement.SetMethod("float get_offset_top()", &Engine::GUI::IElement::GetOffsetTop);
				VElement.SetMethod("float get_offset_width()", &Engine::GUI::IElement::GetOffsetWidth);
				VElement.SetMethod("float get_offset_height()", &Engine::GUI::IElement::GetOffsetHeight);
				VElement.SetMethod("float get_scroll_left()", &Engine::GUI::IElement::GetScrollLeft);
				VElement.SetMethod("void set_scroll_left(float)", &Engine::GUI::IElement::SetScrollLeft);
				VElement.SetMethod("float get_scroll_top()", &Engine::GUI::IElement::GetScrollTop);
				VElement.SetMethod("void set_scroll_top(float)", &Engine::GUI::IElement::SetScrollTop);
				VElement.SetMethod("float get_scroll_width()", &Engine::GUI::IElement::GetScrollWidth);
				VElement.SetMethod("float get_scroll_height()", &Engine::GUI::IElement::GetScrollHeight);
				VElement.SetMethod("ui_document get_owner_document() const", &Engine::GUI::IElement::GetOwnerDocument);
				VElement.SetMethod("ui_element get_parent_node() const", &Engine::GUI::IElement::GetParentNode);
				VElement.SetMethod("ui_element get_next_sibling() const", &Engine::GUI::IElement::GetNextSibling);
				VElement.SetMethod("ui_element get_previous_sibling() const", &Engine::GUI::IElement::GetPreviousSibling);
				VElement.SetMethod("ui_element get_first_child() const", &Engine::GUI::IElement::GetFirstChild);
				VElement.SetMethod("ui_element get_last_child() const", &Engine::GUI::IElement::GetLastChild);
				VElement.SetMethod("ui_element get_child(int) const", &Engine::GUI::IElement::GetChild);
				VElement.SetMethod("int get_num_children(bool = false) const", &Engine::GUI::IElement::GetNumChildren);
				VElement.SetMethod<Engine::GUI::IElement, void, Core::String&>("void get_inner_html(string &out) const", &Engine::GUI::IElement::GetInnerHTML);
				VElement.SetMethod<Engine::GUI::IElement, Core::String>("string get_inner_html() const", &Engine::GUI::IElement::GetInnerHTML);
				VElement.SetMethod("void set_inner_html(const string &in)", &Engine::GUI::IElement::SetInnerHTML);
				VElement.SetMethod("bool is_focused()", &Engine::GUI::IElement::IsFocused);
				VElement.SetMethod("bool is_hovered()", &Engine::GUI::IElement::IsHovered);
				VElement.SetMethod("bool is_active()", &Engine::GUI::IElement::IsActive);
				VElement.SetMethod("bool is_checked()", &Engine::GUI::IElement::IsChecked);
				VElement.SetMethod("bool focus()", &Engine::GUI::IElement::Focus);
				VElement.SetMethod("void blur()", &Engine::GUI::IElement::Blur);
				VElement.SetMethod("void click()", &Engine::GUI::IElement::Click);
				VElement.SetMethod("void add_event_listener(const string &in, ui_listener@+, bool = false)", &Engine::GUI::IElement::AddEventListener);
				VElement.SetMethod("void remove_event_listener(const string &in, ui_listener@+, bool = false)", &Engine::GUI::IElement::RemoveEventListener);
				VElement.SetMethodEx("bool dispatch_event(const string &in, schema@+)", &IElementDocumentDispatchEvent);
				VElement.SetMethod("void scroll_into_view(bool = true)", &Engine::GUI::IElement::ScrollIntoView);
				VElement.SetMethod("ui_element append_child(const ui_element &in, bool = true)", &Engine::GUI::IElement::AppendChild);
				VElement.SetMethod("ui_element insert_before(const ui_element &in, const ui_element &in)", &Engine::GUI::IElement::InsertBefore);
				VElement.SetMethod("ui_element replace_child(const ui_element &in, const ui_element &in)", &Engine::GUI::IElement::ReplaceChild);
				VElement.SetMethod("ui_element remove_child(const ui_element &in)", &Engine::GUI::IElement::RemoveChild);
				VElement.SetMethod("bool has_child_nodes() const", &Engine::GUI::IElement::HasChildNodes);
				VElement.SetMethod("ui_element get_element_by_id(const string &in)", &Engine::GUI::IElement::GetElementById);
				VElement.SetMethodEx("array<ui_element>@ query_selector_all(const string &in)", &IElementDocumentQuerySelectorAll);
				VElement.SetMethod("bool cast_form_color(vector4 &out, bool)", &Engine::GUI::IElement::CastFormColor);
				VElement.SetMethod("bool cast_form_string(string &out)", &Engine::GUI::IElement::CastFormString);
				VElement.SetMethod("bool cast_form_pointer(uptr@ &out)", &Engine::GUI::IElement::CastFormPointer);
				VElement.SetMethod("bool cast_form_int32(int &out)", &Engine::GUI::IElement::CastFormInt32);
				VElement.SetMethod("bool cast_form_uint32(uint &out)", &Engine::GUI::IElement::CastFormUInt32);
				VElement.SetMethod("bool cast_form_flag32(uint &out, uint)", &Engine::GUI::IElement::CastFormFlag32);
				VElement.SetMethod("bool cast_form_int64(int64 &out)", &Engine::GUI::IElement::CastFormInt64);
				VElement.SetMethod("bool cast_form_uint64(uint64 &out)", &Engine::GUI::IElement::CastFormUInt64);
				VElement.SetMethod("bool cast_form_flag64(uint64 &out, uint64)", &Engine::GUI::IElement::CastFormFlag64);
				VElement.SetMethod<Engine::GUI::IElement, bool, float*>("bool cast_form_float(float &out)", &Engine::GUI::IElement::CastFormFloat);
				VElement.SetMethod<Engine::GUI::IElement, bool, float*, float>("bool cast_form_float(float &out, float)", &Engine::GUI::IElement::CastFormFloat);
				VElement.SetMethod("bool cast_form_double(double &out)", &Engine::GUI::IElement::CastFormDouble);
				VElement.SetMethod("bool cast_form_boolean(bool &out)", &Engine::GUI::IElement::CastFormBoolean);
				VElement.SetMethod("string get_form_name() const", &Engine::GUI::IElement::GetFormName);
				VElement.SetMethod("void set_form_name(const string &in)", &Engine::GUI::IElement::SetFormName);
				VElement.SetMethod("string get_form_value() const", &Engine::GUI::IElement::GetFormValue);
				VElement.SetMethod("void set_form_value(const string &in)", &Engine::GUI::IElement::SetFormValue);
				VElement.SetMethod("bool is_form_disabled() const", &Engine::GUI::IElement::IsFormDisabled);
				VElement.SetMethod("void set_form_disabled(bool)", &Engine::GUI::IElement::SetFormDisabled);
				VElement.SetMethod("uptr@ get_element() const", &Engine::GUI::IElement::GetElement);
				VElement.SetMethod("bool is_valid() const", &Engine::GUI::IElement::IsValid);

				VDocument.SetConstructor<Engine::GUI::IElementDocument>("void f()");
				VDocument.SetConstructor<Engine::GUI::IElementDocument, Rml::ElementDocument*>("void f(uptr@)");
				VDocument.SetMethod("ui_element clone() const", &Engine::GUI::IElementDocument::Clone);
				VDocument.SetMethod("void set_class(const string &in, bool)", &Engine::GUI::IElementDocument::SetClass);
				VDocument.SetMethod("bool is_class_set(const string &in) const", &Engine::GUI::IElementDocument::IsClassSet);
				VDocument.SetMethod("void set_class_names(const string &in)", &Engine::GUI::IElementDocument::SetClassNames);
				VDocument.SetMethod("string get_class_names() const", &Engine::GUI::IElementDocument::GetClassNames);
				VDocument.SetMethod("string get_address(bool = false, bool = true) const", &Engine::GUI::IElementDocument::GetAddress);
				VDocument.SetMethod("void set_offset(const vector2 &in, const ui_element &in, bool = false)", &Engine::GUI::IElementDocument::SetOffset);
				VDocument.SetMethod("vector2 get_relative_offset(ui_area = ui_area::Content) const", &Engine::GUI::IElementDocument::GetRelativeOffset);
				VDocument.SetMethod("vector2 get_absolute_offset(ui_area = ui_area::Content) const", &Engine::GUI::IElementDocument::GetAbsoluteOffset);
				VDocument.SetMethod("void set_client_area(ui_area)", &Engine::GUI::IElementDocument::SetClientArea);
				VDocument.SetMethod("ui_area get_client_area() const", &Engine::GUI::IElementDocument::GetClientArea);
				VDocument.SetMethod("void set_bontent_box(const vector2 &in, const vector2 &in)", &Engine::GUI::IElementDocument::SetContentBox);
				VDocument.SetMethod("float get_baseline() const", &Engine::GUI::IElementDocument::GetBaseline);
				VDocument.SetMethod("bool get_intrinsic_dimensions(vector2 &out, float &out)", &Engine::GUI::IElementDocument::GetIntrinsicDimensions);
				VDocument.SetMethod("bool is_point_within_element(const vector2 &in)", &Engine::GUI::IElementDocument::IsPointWithinElement);
				VDocument.SetMethod("bool is_visible() const", &Engine::GUI::IElementDocument::IsVisible);
				VDocument.SetMethod("float get_zindex() const", &Engine::GUI::IElementDocument::GetZIndex);
				VDocument.SetMethod("bool set_property(const string &in, const string &in)", &Engine::GUI::IElementDocument::SetProperty);
				VDocument.SetMethod("void remove_property(const string &in)", &Engine::GUI::IElementDocument::RemoveProperty);
				VDocument.SetMethod("string get_property(const string &in) const", &Engine::GUI::IElementDocument::GetProperty);
				VDocument.SetMethod("string get_local_property(const string &in) const", &Engine::GUI::IElementDocument::GetLocalProperty);
				VDocument.SetMethod("float resolve_numeric_property(const string &in) const", &Engine::GUI::IElementDocument::ResolveNumericProperty);
				VDocument.SetMethod("vector2 get_containing_block() const", &Engine::GUI::IElementDocument::GetContainingBlock);
				VDocument.SetMethod("ui_position get_position() const", &Engine::GUI::IElementDocument::GetPosition);
				VDocument.SetMethod("ui_float get_float() const", &Engine::GUI::IElementDocument::GetFloat);
				VDocument.SetMethod("ui_display get_display() const", &Engine::GUI::IElementDocument::GetDisplay);
				VDocument.SetMethod("float get_line_height() const", &Engine::GUI::IElementDocument::GetLineHeight);
				VDocument.SetMethod("bool project(vector2 &out) const", &Engine::GUI::IElementDocument::Project);
				VDocument.SetMethod("bool animate(const string &in, const string &in, float, ui_timing_func, ui_timing_dir, int = -1, bool = true, float = 0)", &Engine::GUI::IElementDocument::Animate);
				VDocument.SetMethod("bool add_animation_key(const string &in, const string &in, float, ui_timing_func, ui_timing_dir)", &Engine::GUI::IElementDocument::AddAnimationKey);
				VDocument.SetMethod("void set_pseudo_Class(const string &in, bool)", &Engine::GUI::IElementDocument::SetPseudoClass);
				VDocument.SetMethod("bool is_pseudo_class_set(const string &in) const", &Engine::GUI::IElementDocument::IsPseudoClassSet);
				VDocument.SetMethod("void set_attribute(const string &in, const string &in)", &Engine::GUI::IElementDocument::SetAttribute);
				VDocument.SetMethod("string get_attribute(const string &in) const", &Engine::GUI::IElementDocument::GetAttribute);
				VDocument.SetMethod("bool has_attribute(const string &in) const", &Engine::GUI::IElementDocument::HasAttribute);
				VDocument.SetMethod("void remove_attribute(const string &in)", &Engine::GUI::IElementDocument::RemoveAttribute);
				VDocument.SetMethod("ui_element get_focus_leaf_node()", &Engine::GUI::IElementDocument::GetFocusLeafNode);
				VDocument.SetMethod("string get_tag_name() const", &Engine::GUI::IElementDocument::GetTagName);
				VDocument.SetMethod("string get_id() const", &Engine::GUI::IElementDocument::GetId);
				VDocument.SetMethod("float get_absolute_left()", &Engine::GUI::IElementDocument::GetAbsoluteLeft);
				VDocument.SetMethod("float get_absolute_top()", &Engine::GUI::IElementDocument::GetAbsoluteTop);
				VDocument.SetMethod("float get_client_left()", &Engine::GUI::IElementDocument::GetClientLeft);
				VDocument.SetMethod("float get_client_top()", &Engine::GUI::IElementDocument::GetClientTop);
				VDocument.SetMethod("float get_client_width()", &Engine::GUI::IElementDocument::GetClientWidth);
				VDocument.SetMethod("float get_client_height()", &Engine::GUI::IElementDocument::GetClientHeight);
				VDocument.SetMethod("ui_element get_offset_parent()", &Engine::GUI::IElementDocument::GetOffsetParent);
				VDocument.SetMethod("float get_offset_left()", &Engine::GUI::IElementDocument::GetOffsetLeft);
				VDocument.SetMethod("float get_offset_top()", &Engine::GUI::IElementDocument::GetOffsetTop);
				VDocument.SetMethod("float get_offset_width()", &Engine::GUI::IElementDocument::GetOffsetWidth);
				VDocument.SetMethod("float get_offset_height()", &Engine::GUI::IElementDocument::GetOffsetHeight);
				VDocument.SetMethod("float get_scroll_left()", &Engine::GUI::IElementDocument::GetScrollLeft);
				VDocument.SetMethod("void set_scroll_left(float)", &Engine::GUI::IElementDocument::SetScrollLeft);
				VDocument.SetMethod("float get_scroll_top()", &Engine::GUI::IElementDocument::GetScrollTop);
				VDocument.SetMethod("void set_scroll_top(float)", &Engine::GUI::IElementDocument::SetScrollTop);
				VDocument.SetMethod("float get_scroll_width()", &Engine::GUI::IElementDocument::GetScrollWidth);
				VDocument.SetMethod("float get_scroll_height()", &Engine::GUI::IElementDocument::GetScrollHeight);
				VDocument.SetMethod("ui_document get_owner_document() const", &Engine::GUI::IElementDocument::GetOwnerDocument);
				VDocument.SetMethod("ui_element get_parent_node() const", &Engine::GUI::IElementDocument::GetParentNode);
				VDocument.SetMethod("ui_element get_next_sibling() const", &Engine::GUI::IElementDocument::GetNextSibling);
				VDocument.SetMethod("ui_element get_previous_sibling() const", &Engine::GUI::IElementDocument::GetPreviousSibling);
				VDocument.SetMethod("ui_element get_first_child() const", &Engine::GUI::IElementDocument::GetFirstChild);
				VDocument.SetMethod("ui_element get_last_child() const", &Engine::GUI::IElementDocument::GetLastChild);
				VDocument.SetMethod("ui_element get_child(int) const", &Engine::GUI::IElementDocument::GetChild);
				VDocument.SetMethod("int get_num_children(bool = false) const", &Engine::GUI::IElementDocument::GetNumChildren);
				VDocument.SetMethod<Engine::GUI::IElement, void, Core::String&>("void get_inner_html(string &out) const", &Engine::GUI::IElementDocument::GetInnerHTML);
				VDocument.SetMethod<Engine::GUI::IElement, Core::String>("string get_inner_html() const", &Engine::GUI::IElementDocument::GetInnerHTML);
				VDocument.SetMethod("void set_inner_html(const string &in)", &Engine::GUI::IElementDocument::SetInnerHTML);
				VDocument.SetMethod("bool is_focused()", &Engine::GUI::IElementDocument::IsFocused);
				VDocument.SetMethod("bool is_hovered()", &Engine::GUI::IElementDocument::IsHovered);
				VDocument.SetMethod("bool is_active()", &Engine::GUI::IElementDocument::IsActive);
				VDocument.SetMethod("bool is_checked()", &Engine::GUI::IElementDocument::IsChecked);
				VDocument.SetMethod("bool focus()", &Engine::GUI::IElementDocument::Focus);
				VDocument.SetMethod("void blur()", &Engine::GUI::IElementDocument::Blur);
				VDocument.SetMethod("void click()", &Engine::GUI::IElementDocument::Click);
				VDocument.SetMethod("void add_event_listener(const string &in, ui_listener@+, bool = false)", &Engine::GUI::IElementDocument::AddEventListener);
				VDocument.SetMethod("void remove_event_listener(const string &in, ui_listener@+, bool = false)", &Engine::GUI::IElementDocument::RemoveEventListener);
				VDocument.SetMethodEx("bool dispatch_event(const string &in, schema@+)", &IElementDocumentDispatchEvent);
				VDocument.SetMethod("void scroll_into_view(bool = true)", &Engine::GUI::IElementDocument::ScrollIntoView);
				VDocument.SetMethod("ui_element append_child(const ui_element &in, bool = true)", &Engine::GUI::IElementDocument::AppendChild);
				VDocument.SetMethod("ui_element insert_before(const ui_element &in, const ui_element &in)", &Engine::GUI::IElementDocument::InsertBefore);
				VDocument.SetMethod("ui_element replace_child(const ui_element &in, const ui_element &in)", &Engine::GUI::IElementDocument::ReplaceChild);
				VDocument.SetMethod("ui_element remove_child(const ui_element &in)", &Engine::GUI::IElementDocument::RemoveChild);
				VDocument.SetMethod("bool has_child_nodes() const", &Engine::GUI::IElementDocument::HasChildNodes);
				VDocument.SetMethod("ui_element get_element_by_id(const string &in)", &Engine::GUI::IElementDocument::GetElementById);
				VDocument.SetMethodEx("array<ui_element>@ query_selector_all(const string &in)", &IElementDocumentQuerySelectorAll);
				VDocument.SetMethod("bool cast_form_color(vector4 &out, bool)", &Engine::GUI::IElementDocument::CastFormColor);
				VDocument.SetMethod("bool cast_form_string(string &out)", &Engine::GUI::IElementDocument::CastFormString);
				VDocument.SetMethod("bool cast_form_pointer(uptr@ &out)", &Engine::GUI::IElementDocument::CastFormPointer);
				VDocument.SetMethod("bool cast_form_int32(int &out)", &Engine::GUI::IElementDocument::CastFormInt32);
				VDocument.SetMethod("bool cast_form_uint32(uint &out)", &Engine::GUI::IElementDocument::CastFormUInt32);
				VDocument.SetMethod("bool cast_form_flag32(uint &out, uint)", &Engine::GUI::IElementDocument::CastFormFlag32);
				VDocument.SetMethod("bool cast_form_int64(int64 &out)", &Engine::GUI::IElementDocument::CastFormInt64);
				VDocument.SetMethod("bool cast_form_uint64(uint64 &out)", &Engine::GUI::IElementDocument::CastFormUInt64);
				VDocument.SetMethod("bool cast_form_flag64(uint64 &out, uint64)", &Engine::GUI::IElementDocument::CastFormFlag64);
				VDocument.SetMethod<Engine::GUI::IElement, bool, float*>("bool cast_form_float(float &out)", &Engine::GUI::IElementDocument::CastFormFloat);
				VDocument.SetMethod<Engine::GUI::IElement, bool, float*, float>("bool cast_form_float(float &out, float)", &Engine::GUI::IElementDocument::CastFormFloat);
				VDocument.SetMethod("bool cast_form_double(double &out)", &Engine::GUI::IElementDocument::CastFormDouble);
				VDocument.SetMethod("bool cast_form_boolean(bool &out)", &Engine::GUI::IElementDocument::CastFormBoolean);
				VDocument.SetMethod("string get_form_name() const", &Engine::GUI::IElementDocument::GetFormName);
				VDocument.SetMethod("void set_form_name(const string &in)", &Engine::GUI::IElementDocument::SetFormName);
				VDocument.SetMethod("string get_form_value() const", &Engine::GUI::IElementDocument::GetFormValue);
				VDocument.SetMethod("void set_form_value(const string &in)", &Engine::GUI::IElementDocument::SetFormValue);
				VDocument.SetMethod("bool is_form_disabled() const", &Engine::GUI::IElementDocument::IsFormDisabled);
				VDocument.SetMethod("void set_form_disabled(bool)", &Engine::GUI::IElementDocument::SetFormDisabled);
				VDocument.SetMethod("uptr@ get_element() const", &Engine::GUI::IElementDocument::GetElement);
				VDocument.SetMethod("bool is_valid() const", &Engine::GUI::IElementDocument::IsValid);
				VDocument.SetMethod("void set_title(const string &in)", &Engine::GUI::IElementDocument::SetTitle);
				VDocument.SetMethod("void pull_to_front()", &Engine::GUI::IElementDocument::PullToFront);
				VDocument.SetMethod("void push_to_back()", &Engine::GUI::IElementDocument::PushToBack);
				VDocument.SetMethod("void show(ui_modal_flag = ui_modal_flag::None, ui_focus_flag = ui_focus_flag::Auto)", &Engine::GUI::IElementDocument::Show);
				VDocument.SetMethod("void hide()", &Engine::GUI::IElementDocument::Hide);
				VDocument.SetMethod("void close()", &Engine::GUI::IElementDocument::Close);
				VDocument.SetMethod("string get_title() const", &Engine::GUI::IElementDocument::GetTitle);
				VDocument.SetMethod("string get_source_url() const", &Engine::GUI::IElementDocument::GetSourceURL);
				VDocument.SetMethod("ui_element create_element(const string &in)", &Engine::GUI::IElementDocument::CreateElement);
				VDocument.SetMethod("bool is_modal() const", &Engine::GUI::IElementDocument::IsModal);;
				VDocument.SetMethod("uptr@ get_element_document() const", &Engine::GUI::IElementDocument::GetElementDocument);

				return true;
#else
				VI_ASSERT(false, false, "<gui/control> is not loaded");
				return false;
#endif
			}
			bool Registry::LoadUiModel(VirtualMachine* Engine)
			{
#ifdef VI_HAS_BINDINGS
				VI_ASSERT(Engine != nullptr, false, "manager should be set");
				Engine->SetFunctionDef("void ui_data_event(ui_event &in, array<variant>@+)");

				RefClass VModel = Engine->SetClass<Engine::GUI::DataModel>("ui_model", false);
				VModel.SetMethodEx("bool set(const string &in, schema@+)", &DataModelSet);
				VModel.SetMethodEx("bool set_var(const string &in, const variant &in)", &DataModelSetVar);
				VModel.SetMethodEx("bool set_string(const string &in, const string &in)", &DataModelSetString);
				VModel.SetMethodEx("bool set_integer(const string &in, int64)", &DataModelSetInteger);
				VModel.SetMethodEx("bool set_float(const string &in, float)", &DataModelSetFloat);
				VModel.SetMethodEx("bool set_double(const string &in, double)", &DataModelSetDouble);
				VModel.SetMethodEx("bool set_boolean(const string &in, bool)", &DataModelSetBoolean);
				VModel.SetMethodEx("bool set_pointer(const string &in, uptr@)", &DataModelSetPointer);
				VModel.SetMethodEx("bool set_callback(const string &in, ui_data_event@)", &DataModelSetCallback);
				VModel.SetMethodEx("schema@+ get(const string &in)", &DataModelGet);
				VModel.SetMethod("string get_string(const string &in)", &Engine::GUI::DataModel::GetString);
				VModel.SetMethod("int64 get_integer(const string &in)", &Engine::GUI::DataModel::GetInteger);
				VModel.SetMethod("float get_float(const string &in)", &Engine::GUI::DataModel::GetFloat);
				VModel.SetMethod("double get_double(const string &in)", &Engine::GUI::DataModel::GetDouble);
				VModel.SetMethod("bool get_boolean(const string &in)", &Engine::GUI::DataModel::GetBoolean);
				VModel.SetMethod("uptr@ get_pointer(const string &in)", &Engine::GUI::DataModel::GetPointer);
				VModel.SetMethod("bool has_changed(const string &in)", &Engine::GUI::DataModel::HasChanged);
				VModel.SetMethod("void change(const string &in)", &Engine::GUI::DataModel::Change);
				VModel.SetMethod("bool isValid() const", &Engine::GUI::DataModel::IsValid);
				VModel.SetMethodStatic("vector4 to_color4(const string &in)", &Engine::GUI::IVariant::ToColor4);
				VModel.SetMethodStatic("string from_color4(const vector4 &in, bool)", &Engine::GUI::IVariant::FromColor4);
				VModel.SetMethodStatic("vector4 to_color3(const string &in)", &Engine::GUI::IVariant::ToColor3);
				VModel.SetMethodStatic("string from_color3(const vector4 &in, bool)", &Engine::GUI::IVariant::ToColor3);
				VModel.SetMethodStatic("int get_vector_type(const string &in)", &Engine::GUI::IVariant::GetVectorType);
				VModel.SetMethodStatic("vector4 to_vector4(const string &in)", &Engine::GUI::IVariant::ToVector4);
				VModel.SetMethodStatic("string from_vector4(const vector4 &in)", &Engine::GUI::IVariant::FromVector4);
				VModel.SetMethodStatic("vector3 to_vector3(const string &in)", &Engine::GUI::IVariant::ToVector3);
				VModel.SetMethodStatic("string from_vector3(const vector3 &in)", &Engine::GUI::IVariant::FromVector3);
				VModel.SetMethodStatic("vector2 to_vector2(const string &in)", &Engine::GUI::IVariant::ToVector2);
				VModel.SetMethodStatic("string from_vector2(const vector2 &in)", &Engine::GUI::IVariant::FromVector2);

				return true;
#else
				VI_ASSERT(false, false, "<gui/model> is not loaded");
				return false;
#endif
			}
			bool Registry::LoadUiContext(VirtualMachine* Engine)
			{
#ifdef VI_HAS_BINDINGS
				VI_ASSERT(Engine != nullptr, false, "manager should be set");

				RefClass VContext = Engine->SetClass<Engine::GUI::Context>("gui_context", false);
				VContext.SetMethod("void set_documents_base_tag(const string &in)", &Engine::GUI::Context::SetDocumentsBaseTag);
				VContext.SetMethod("void clear_styles()", &Engine::GUI::Context::ClearStyles);
				VContext.SetMethod("bool clear_documents()", &Engine::GUI::Context::ClearDocuments);
				VContext.SetMethod<Engine::GUI::Context, bool, const Core::String&>("bool initialize(const string &in)", &Engine::GUI::Context::Initialize);
				VContext.SetMethod("bool is_input_focused()", &Engine::GUI::Context::IsInputFocused);
				VContext.SetMethod("bool is_loading()", &Engine::GUI::Context::IsLoading);
				VContext.SetMethod("bool replace_html(const string &in, const string &in, int = 0)", &Engine::GUI::Context::ReplaceHTML);
				VContext.SetMethod("bool remove_data_model(const string &in)", &Engine::GUI::Context::RemoveDataModel);
				VContext.SetMethod("bool remove_data_models()", &Engine::GUI::Context::RemoveDataModels);
				VContext.SetMethod("uptr@ get_context()", &Engine::GUI::Context::GetContext);
				VContext.SetMethod("vector2 get_dimensions() const", &Engine::GUI::Context::GetDimensions);
				VContext.SetMethod("string get_documents_base_tag() const", &Engine::GUI::Context::GetDocumentsBaseTag);
				VContext.SetMethod("void set_density_independent_pixel_ratio(float)", &Engine::GUI::Context::GetDensityIndependentPixelRatio);
				VContext.SetMethod("float get_density_independent_pixel_ratio() const", &Engine::GUI::Context::GetDensityIndependentPixelRatio);
				VContext.SetMethod("void enable_mouse_cursor(bool)", &Engine::GUI::Context::EnableMouseCursor);
				VContext.SetMethod("ui_document eval_html(const string &in, int = 0)", &Engine::GUI::Context::EvalHTML);
				VContext.SetMethod("ui_document add_css(const string &in, int = 0)", &Engine::GUI::Context::AddCSS);
				VContext.SetMethod("ui_document load_css(const string &in, int = 0)", &Engine::GUI::Context::LoadCSS);
				VContext.SetMethod("ui_document load_document(const string &in)", &Engine::GUI::Context::LoadDocument);
				VContext.SetMethod("ui_document add_document(const string &in)", &Engine::GUI::Context::AddDocument);
				VContext.SetMethod("ui_document add_document_empty(const string &in = \"body\")", &Engine::GUI::Context::AddDocumentEmpty);
				VContext.SetMethod<Engine::GUI::Context, Engine::GUI::IElementDocument, const Core::String&>("ui_document get_document(const string &in)", &Engine::GUI::Context::GetDocument);
				VContext.SetMethod<Engine::GUI::Context, Engine::GUI::IElementDocument, int>("ui_document get_document(int)", &Engine::GUI::Context::GetDocument);
				VContext.SetMethod("int get_num_documents() const", &Engine::GUI::Context::GetNumDocuments);
				VContext.SetMethod("ui_element get_element_by_id(const string &in, int = 0)", &Engine::GUI::Context::GetElementById);
				VContext.SetMethod("ui_element get_hover_element()", &Engine::GUI::Context::GetHoverElement);
				VContext.SetMethod("ui_element get_focus_element()", &Engine::GUI::Context::GetFocusElement);
				VContext.SetMethod("ui_element get_root_element()", &Engine::GUI::Context::GetRootElement);
				VContext.SetMethodEx("ui_element get_element_at_point(const vector2 &in)", &ContextGetFocusElement);
				VContext.SetMethod("void pull_document_to_front(const ui_document &in)", &Engine::GUI::Context::PullDocumentToFront);
				VContext.SetMethod("void push_document_to_back(const ui_document &in)", &Engine::GUI::Context::PushDocumentToBack);
				VContext.SetMethod("void unfocus_document(const ui_document &in)", &Engine::GUI::Context::UnfocusDocument);
				VContext.SetMethod("void add_event_listener(const string &in, ui_listener@+, bool = false)", &Engine::GUI::Context::AddEventListener);
				VContext.SetMethod("void remove_event_listener(const string &in, ui_listener@+, bool = false)", &Engine::GUI::Context::RemoveEventListener);
				VContext.SetMethod("bool is_mouse_interacting()", &Engine::GUI::Context::IsMouseInteracting);
				VContext.SetMethod("bool get_active_clip_region(vector2 &out, vector2 &out)", &Engine::GUI::Context::GetActiveClipRegion);
				VContext.SetMethod("void set_active_clip_region(const vector2 &in, const vector2 &in)", &Engine::GUI::Context::SetActiveClipRegion);
				VContext.SetMethod("ui_model@+ set_model(const string &in)", &Engine::GUI::Context::SetDataModel);
				VContext.SetMethod("ui_model@+ get_model(const string &in)", &Engine::GUI::Context::GetDataModel);

				return true;
#else
				VI_ASSERT(false, false, "<gui/context> is not loaded");
				return false;
#endif
			}
			bool Registry::Release()
			{
				StringFactory::Free();
				return false;
			}
		}
	}
}