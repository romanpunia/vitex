#ifndef THAWK_NETWORK_BSON_H
#define THAWK_NETWORK_BSON_H

#include "../network.h"

struct _bson_t;

namespace Tomahawk
{
    namespace Network
    {
        namespace BSON
        {
            typedef _bson_t TDocument;

            class Document;

            enum Type
            {
                Type_Unknown,
                Type_Uncastable,
                Type_Null,
                Type_Document,
                Type_Array,
                Type_String,
                Type_Integer,
                Type_Number,
                Type_Boolean,
                Type_ObjectId,
				Type_Decimal
            };

            struct THAWK_OUT KeyPair
            {
                std::string Name;
                std::string String;
                TDocument* Document = nullptr;
                TDocument* Array = nullptr;
                Type Mod = Type_Unknown;
                Int64 Integer = 0;
				UInt64 High = 0;
				UInt64 Low = 0;
                Float64 Number = 0;
                unsigned char ObjectId[12] = { 0 };
                bool Boolean = false;
                bool IsValid = false;

                ~KeyPair();
				void Release();
				std::string& ToString();
            };

            class THAWK_OUT Document
            {
            public:
                static TDocument* Create(bool Array = false);
                static TDocument* Create(Rest::Document* Document);
                static TDocument* Create(const std::string& JSON);
                static TDocument* Create(const unsigned char* Buffer, UInt64 Length);
                static void Release(TDocument** Document);
                static void Loop(TDocument* Document, void* Context, const std::function<bool(TDocument*, KeyPair*, void*)>& Callback);
				static bool ParseDecimal(const char* Value, Int64* High, Int64* Low);
                static bool GenerateId(unsigned char* Id12);
                static bool AddKeyDocument(TDocument* Document, const char* Key, TDocument** Value, UInt64 ArrayId = 0);
                static bool AddKeyArray(TDocument* Document, const char* Key, TDocument** Array, UInt64 ArrayId = 0);
                static bool AddKeyString(TDocument* Document, const char* Key, const char* Value, UInt64 ArrayId = 0);
                static bool AddKeyStringBuffer(TDocument* Document, const char* Key, const char* Value, UInt64 Length, UInt64 ArrayId = 0);
                static bool AddKeyInteger(TDocument* Document, const char* Key, Int64 Value, UInt64 ArrayId = 0);
                static bool AddKeyNumber(TDocument* Document, const char* Key, Float64 Value, UInt64 ArrayId = 0);
				static bool AddKeyDecimal(TDocument* Document, const char* Key, UInt64 High, UInt64 Low, UInt64 ArrayId = 0);
				static bool AddKeyDecimalString(TDocument* Document, const char* Key, const std::string& Value, UInt64 ArrayId = 0);
				static bool AddKeyDecimalInteger(TDocument* Document, const char* Key, Int64 Value, UInt64 ArrayId = 0);
				static bool AddKeyDecimalNumber(TDocument* Document, const char* Key, Float64 Value, UInt64 ArrayId = 0);
                static bool AddKeyBoolean(TDocument* Document, const char* Key, bool Value, UInt64 ArrayId = 0);
                static bool AddKeyObjectId(TDocument* Document, const char* Key, unsigned char Value[12], UInt64 ArrayId = 0);
                static bool AddKeyNull(TDocument* Document, const char* Key, UInt64 ArrayId = 0);
                static bool AddKey(TDocument* Document, const char* Key, KeyPair* Value, UInt64 ArrayId = 0);
                static bool HasKey(TDocument* Document, const char* Key);
                static bool GetKey(TDocument* Document, const char* Key, KeyPair* Output);
                static bool GetKeyWithNotation(TDocument* Document, const char* Key, KeyPair* Output);
                static unsigned int GetHashId(unsigned char* Id12);
                static Int64 GetTimeId(unsigned char* Id12);
                static UInt64 CountKeys(TDocument* Document);
                static std::string ToExtendedJSON(TDocument* Document);
                static std::string ToRelaxedAndExtendedJSON(TDocument* Document);
                static std::string ToClassicJSON(TDocument* Document);
                static Rest::Document* ToDocument(TDocument* Document, bool IsArray = false);
                static TDocument* Copy(TDocument* Document);

            private:
                static bool Clone(void* It, KeyPair* Output);
            };
        }
    }
}
#endif