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
				int64_t Integer = 0;
				uint64_t High = 0;
				uint64_t Low = 0;
				double Number = 0;
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
				static TDocument* Create(const unsigned char* Buffer, uint64_t Length);
				static void Release(TDocument** Document);
				static void Loop(TDocument* Document, void* Context, const std::function<bool(TDocument*, KeyPair*, void*)>& Callback);
				static bool ParseDecimal(const char* Value, int64_t* High, int64_t* Low);
				static bool GenerateId(unsigned char* Id12);
				static bool AddKeyDocument(TDocument* Document, const char* Key, TDocument** Value, uint64_t ArrayId = 0);
				static bool AddKeyArray(TDocument* Document, const char* Key, TDocument** Array, uint64_t ArrayId = 0);
				static bool AddKeyString(TDocument* Document, const char* Key, const char* Value, uint64_t ArrayId = 0);
				static bool AddKeyStringBuffer(TDocument* Document, const char* Key, const char* Value, uint64_t Length, uint64_t ArrayId = 0);
				static bool AddKeyInteger(TDocument* Document, const char* Key, int64_t Value, uint64_t ArrayId = 0);
				static bool AddKeyNumber(TDocument* Document, const char* Key, double Value, uint64_t ArrayId = 0);
				static bool AddKeyDecimal(TDocument* Document, const char* Key, uint64_t High, uint64_t Low, uint64_t ArrayId = 0);
				static bool AddKeyDecimalString(TDocument* Document, const char* Key, const std::string& Value, uint64_t ArrayId = 0);
				static bool AddKeyDecimalInteger(TDocument* Document, const char* Key, int64_t Value, uint64_t ArrayId = 0);
				static bool AddKeyDecimalNumber(TDocument* Document, const char* Key, double Value, uint64_t ArrayId = 0);
				static bool AddKeyBoolean(TDocument* Document, const char* Key, bool Value, uint64_t ArrayId = 0);
				static bool AddKeyObjectId(TDocument* Document, const char* Key, unsigned char Value[12], uint64_t ArrayId = 0);
				static bool AddKeyNull(TDocument* Document, const char* Key, uint64_t ArrayId = 0);
				static bool AddKey(TDocument* Document, const char* Key, KeyPair* Value, uint64_t ArrayId = 0);
				static bool HasKey(TDocument* Document, const char* Key);
				static bool GetKey(TDocument* Document, const char* Key, KeyPair* Output);
				static bool GetKeyWithNotation(TDocument* Document, const char* Key, KeyPair* Output);
				static unsigned int GetHashId(unsigned char* Id12);
				static int64_t GetTimeId(unsigned char* Id12);
				static uint64_t CountKeys(TDocument* Document);
				static std::string OIdToString(unsigned char* Id12);
				static std::string StringToOId(const std::string& Id24);
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