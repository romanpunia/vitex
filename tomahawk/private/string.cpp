#include "string.h"
#include "array.h"
#include "../rest.h"
#include <assert.h>
#include <sstream>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unordered_map>
#ifndef __psp2__
#include <locale.h>
#endif
#define UNUSED_VAR(x) (void)(x)

using namespace std;

typedef unordered_map<string, int> map_t;

class CStdStringFactory : public asIStringFactory
{
public:
	CStdStringFactory() {}
	~CStdStringFactory()
	{
		// The script engine must release each string 
		// constant that it has requested
		assert(stringCache.size() == 0);
	}
	const void *GetStringConstant(const char *data, asUINT length)
	{
		// The string factory might be modified from multiple 
		// threads, so it is necessary to use a mutex.
		asAcquireExclusiveLock();

		string str(data, length);
		map_t::iterator it = stringCache.find(str);
		if (it != stringCache.end())
			it->second++;
		else
			it = stringCache.insert(map_t::value_type(str, 1)).first;

		asReleaseExclusiveLock();

		return reinterpret_cast<const void*>(&it->first);
	}
	int  ReleaseStringConstant(const void *str)
	{
		if (str == 0)
			return asERROR;

		int ret = asSUCCESS;

		// The string factory might be modified from multiple 
		// threads, so it is necessary to use a mutex.
		asAcquireExclusiveLock();

		map_t::iterator it = stringCache.find(*reinterpret_cast<const string*>(str));
		if (it == stringCache.end())
			ret = asERROR;
		else
		{
			it->second--;
			if (it->second == 0)
				stringCache.erase(it);
		}

		asReleaseExclusiveLock();

		return ret;
	}
	int  GetRawStringData(const void *str, char *data, asUINT *length) const
	{
		if (str == 0)
			return asERROR;

		if (length)
			*length = (asUINT)reinterpret_cast<const string*>(str)->length();

		if (data)
			memcpy(data, reinterpret_cast<const string*>(str)->c_str(), reinterpret_cast<const string*>(str)->length());

		return asSUCCESS;
	}
	map_t stringCache;
};

static CStdStringFactory *stringFactory = 0;

class CStdStringFactoryCleaner
{
public:
	~CStdStringFactoryCleaner()
	{
		if (stringFactory)
		{
			if (stringFactory->stringCache.empty())
			{
				delete stringFactory;
				stringFactory = 0;
			}
		}
	}
};

static CStdStringFactoryCleaner cleaner;

CStdStringFactory *GetStdStringFactorySingleton()
{
	if (stringFactory == 0)
	{
		// The following instance will be destroyed by the global 
		// CStdStringFactoryCleaner instance upon application shutdown
		stringFactory = new CStdStringFactory();
	}
	return stringFactory;
}
static void ConstructString(string *thisPointer)
{
	new(thisPointer) string();
}
static void CopyConstructString(const string &other, string *thisPointer)
{
	new(thisPointer) string(other);
}
static void DestructString(string *thisPointer)
{
	thisPointer->~string();
}
static string &AddAssignStringToString(const string &str, string &dest)
{
	// We don't register the method directly because some compilers
	// and standard libraries inline the definition, resulting in the
	// linker being unable to find the declaration.
	// Example: CLang/LLVM with XCode 4.3 on OSX 10.7
	dest += str;
	return dest;
}
static bool StringIsEmpty(const string &str)
{
	// We don't register the method directly because some compilers
	// and standard libraries inline the definition, resulting in the
	// linker being unable to find the declaration
	// Example: CLang/LLVM with XCode 4.3 on OSX 10.7
	return str.empty();
}
static string &AssignUInt64ToString(asQWORD i, string &dest)
{
	ostringstream stream;
	stream << i;
	dest = stream.str();
	return dest;
}
static string &AddAssignUInt64ToString(asQWORD i, string &dest)
{
	ostringstream stream;
	stream << i;
	dest += stream.str();
	return dest;
}
static string AddStringUInt64(const string &str, asQWORD i)
{
	ostringstream stream;
	stream << i;
	return str + stream.str();
}
static string AddInt64String(asINT64 i, const string &str)
{
	ostringstream stream;
	stream << i;
	return stream.str() + str;
}
static string &AssignInt64ToString(asINT64 i, string &dest)
{
	ostringstream stream;
	stream << i;
	dest = stream.str();
	return dest;
}
static string &AddAssignInt64ToString(asINT64 i, string &dest)
{
	ostringstream stream;
	stream << i;
	dest += stream.str();
	return dest;
}
static string AddStringInt64(const string &str, asINT64 i)
{
	ostringstream stream;
	stream << i;
	return str + stream.str();
}
static string AddUInt64String(asQWORD i, const string &str)
{
	ostringstream stream;
	stream << i;
	return stream.str() + str;
}
static string &AssignDoubleToString(double f, string &dest)
{
	ostringstream stream;
	stream << f;
	dest = stream.str();
	return dest;
}
static string &AddAssignDoubleToString(double f, string &dest)
{
	ostringstream stream;
	stream << f;
	dest += stream.str();
	return dest;
}
static string &AssignFloatToString(float f, string &dest)
{
	ostringstream stream;
	stream << f;
	dest = stream.str();
	return dest;
}
static string &AddAssignFloatToString(float f, string &dest)
{
	ostringstream stream;
	stream << f;
	dest += stream.str();
	return dest;
}
static string &AssignBoolToString(bool b, string &dest)
{
	ostringstream stream;
	stream << (b ? "true" : "false");
	dest = stream.str();
	return dest;
}
static string &AddAssignBoolToString(bool b, string &dest)
{
	ostringstream stream;
	stream << (b ? "true" : "false");
	dest += stream.str();
	return dest;
}
static string AddStringDouble(const string &str, double f)
{
	ostringstream stream;
	stream << f;
	return str + stream.str();
}
static string AddDoubleString(double f, const string &str)
{
	ostringstream stream;
	stream << f;
	return stream.str() + str;
}
static string AddStringFloat(const string &str, float f)
{
	ostringstream stream;
	stream << f;
	return str + stream.str();
}
static string AddFloatString(float f, const string &str)
{
	ostringstream stream;
	stream << f;
	return stream.str() + str;
}
static string AddStringBool(const string &str, bool b)
{
	ostringstream stream;
	stream << (b ? "true" : "false");
	return str + stream.str();
}
static string AddBoolString(bool b, const string &str)
{
	ostringstream stream;
	stream << (b ? "true" : "false");
	return stream.str() + str;
}
static char *StringCharAt(unsigned int i, string &str)
{
	if (i >= str.size())
	{
		// Set a script exception
		asIScriptContext *ctx = asGetActiveContext();
		ctx->SetException("Out of range");

		// Return a null pointer
		return 0;
	}

	return &str[i];
}
static int StringCmp(const string &a, const string &b)
{
	int cmp = 0;
	if (a < b) cmp = -1;
	else if (a > b) cmp = 1;
	return cmp;
}
static int StringFindFirst(const string &sub, asUINT start, const string &str)
{
	// We don't register the method directly because the argument types change between 32bit and 64bit platforms
	return (int)str.find(sub, (size_t)(start < 0 ? string::npos : start));
}
static int StringFindFirstOf(const string &sub, asUINT start, const string &str)
{
	// We don't register the method directly because the argument types change between 32bit and 64bit platforms
	return (int)str.find_first_of(sub, (size_t)(start < 0 ? string::npos : start));
}
static int StringFindLastOf(const string &sub, asUINT start, const string &str)
{
	// We don't register the method directly because the argument types change between 32bit and 64bit platforms
	return (int)str.find_last_of(sub, (size_t)(start < 0 ? string::npos : start));
}
static int StringFindFirstNotOf(const string &sub, asUINT start, const string &str)
{
	// We don't register the method directly because the argument types change between 32bit and 64bit platforms
	return (int)str.find_first_not_of(sub, (size_t)(start < 0 ? string::npos : start));
}
static int StringFindLastNotOf(const string &sub, asUINT start, const string &str)
{
	// We don't register the method directly because the argument types change between 32bit and 64bit platforms
	return (int)str.find_last_not_of(sub, (size_t)(start < 0 ? string::npos : start));
}
static int StringFindLast(const string &sub, int start, const string &str)
{
	// We don't register the method directly because the argument types change between 32bit and 64bit platforms
	return (int)str.rfind(sub, (size_t)(start < 0 ? string::npos : start));
}
static void StringInsert(unsigned int pos, const string &other, string &str)
{
	// We don't register the method directly because the argument types change between 32bit and 64bit platforms
	str.insert(pos, other);
}
static void StringErase(unsigned int pos, int count, string &str)
{
	// We don't register the method directly because the argument types change between 32bit and 64bit platforms
	str.erase(pos, (size_t)(count < 0 ? string::npos : count));
}
static asUINT StringLength(const string &str)
{
	// We don't register the method directly because the return type changes between 32bit and 64bit platforms
	return (asUINT)str.length();
}
static void StringResize(asUINT l, string &str)
{
	// We don't register the method directly because the argument types change between 32bit and 64bit platforms
	str.resize(l);
}
static string StringReplace(const string& a, const string& b, uint64_t o, const string& base)
{
	return Tomahawk::Rest::Stroke(base).Replace(a, b, o).R();
}
static string formatInt(asINT64 value, const string &options, asUINT width)
{
	bool leftJustify = options.find("l") != string::npos;
	bool padWithZero = options.find("0") != string::npos;
	bool alwaysSign = options.find("+") != string::npos;
	bool spaceOnSign = options.find(" ") != string::npos;
	bool hexSmall = options.find("h") != string::npos;
	bool hexLarge = options.find("H") != string::npos;

	string fmt = "%";
	if (leftJustify) fmt += "-";
	if (alwaysSign) fmt += "+";
	if (spaceOnSign) fmt += " ";
	if (padWithZero) fmt += "0";

#ifdef _WIN32
	fmt += "*I64";
#else
#ifdef _LP64
	fmt += "*l";
#else
	fmt += "*ll";
#endif
#endif

	if (hexSmall) fmt += "x";
	else if (hexLarge) fmt += "X";
	else fmt += "d";

	string buf;
	buf.resize(width + 30);
#if _MSC_VER >= 1400 && !defined(__S3E__)
	// MSVC 8.0 / 2005 or newer
	sprintf_s(&buf[0], buf.size(), fmt.c_str(), width, value);
#else
	sprintf(&buf[0], fmt.c_str(), width, value);
#endif
	buf.resize(strlen(&buf[0]));

	return buf;
}
static string formatUInt(asQWORD value, const string &options, asUINT width)
{
	bool leftJustify = options.find("l") != string::npos;
	bool padWithZero = options.find("0") != string::npos;
	bool alwaysSign = options.find("+") != string::npos;
	bool spaceOnSign = options.find(" ") != string::npos;
	bool hexSmall = options.find("h") != string::npos;
	bool hexLarge = options.find("H") != string::npos;

	string fmt = "%";
	if (leftJustify) fmt += "-";
	if (alwaysSign) fmt += "+";
	if (spaceOnSign) fmt += " ";
	if (padWithZero) fmt += "0";

#ifdef _WIN32
	fmt += "*I64";
#else
#ifdef _LP64
	fmt += "*l";
#else
	fmt += "*ll";
#endif
#endif

	if (hexSmall) fmt += "x";
	else if (hexLarge) fmt += "X";
	else fmt += "u";

	string buf;
	buf.resize(width + 30);
#if _MSC_VER >= 1400 && !defined(__S3E__)
	// MSVC 8.0 / 2005 or newer
	sprintf_s(&buf[0], buf.size(), fmt.c_str(), width, value);
#else
	sprintf(&buf[0], fmt.c_str(), width, value);
#endif
	buf.resize(strlen(&buf[0]));

	return buf;
}
static string formatFloat(double value, const string &options, asUINT width, asUINT precision)
{
	bool leftJustify = options.find("l") != string::npos;
	bool padWithZero = options.find("0") != string::npos;
	bool alwaysSign = options.find("+") != string::npos;
	bool spaceOnSign = options.find(" ") != string::npos;
	bool expSmall = options.find("e") != string::npos;
	bool expLarge = options.find("E") != string::npos;

	string fmt = "%";
	if (leftJustify) fmt += "-";
	if (alwaysSign) fmt += "+";
	if (spaceOnSign) fmt += " ";
	if (padWithZero) fmt += "0";

	fmt += "*.*";

	if (expSmall) fmt += "e";
	else if (expLarge) fmt += "E";
	else fmt += "f";

	string buf;
	buf.resize(width + precision + 50);
#if _MSC_VER >= 1400 && !defined(__S3E__)
	// MSVC 8.0 / 2005 or newer
	sprintf_s(&buf[0], buf.size(), fmt.c_str(), width, precision, value);
#else
	sprintf(&buf[0], fmt.c_str(), width, precision, value);
#endif
	buf.resize(strlen(&buf[0]));

	return buf;
}
static asINT64 parseInt(const string &val, asUINT base, asUINT *byteCount)
{
	// Only accept base 10 and 16
	if (base != 10 && base != 16)
	{
		if (byteCount) *byteCount = 0;
		return 0;
	}

	const char *end = &val[0];

	// Determine the sign
	bool sign = false;
	if (*end == '-')
	{
		sign = true;
		end++;
	}
	else if (*end == '+')
		end++;

	asINT64 res = 0;
	if (base == 10)
	{
		while (*end >= '0' && *end <= '9')
		{
			res *= 10;
			res += *end++ - '0';
		}
	}
	else if (base == 16)
	{
		while ((*end >= '0' && *end <= '9') ||
			(*end >= 'a' && *end <= 'f') ||
			(*end >= 'A' && *end <= 'F'))
		{
			res *= 16;
			if (*end >= '0' && *end <= '9')
				res += *end++ - '0';
			else if (*end >= 'a' && *end <= 'f')
				res += *end++ - 'a' + 10;
			else if (*end >= 'A' && *end <= 'F')
				res += *end++ - 'A' + 10;
		}
	}

	if (byteCount)
		*byteCount = asUINT(size_t(end - val.c_str()));

	if (sign)
		res = -res;

	return res;
}
static asQWORD parseUInt(const string &val, asUINT base, asUINT *byteCount)
{
	// Only accept base 10 and 16
	if (base != 10 && base != 16)
	{
		if (byteCount) *byteCount = 0;
		return 0;
	}

	const char *end = &val[0];

	asQWORD res = 0;
	if (base == 10)
	{
		while (*end >= '0' && *end <= '9')
		{
			res *= 10;
			res += *end++ - '0';
		}
	}
	else if (base == 16)
	{
		while ((*end >= '0' && *end <= '9') ||
			(*end >= 'a' && *end <= 'f') ||
			(*end >= 'A' && *end <= 'F'))
		{
			res *= 16;
			if (*end >= '0' && *end <= '9')
				res += *end++ - '0';
			else if (*end >= 'a' && *end <= 'f')
				res += *end++ - 'a' + 10;
			else if (*end >= 'A' && *end <= 'F')
				res += *end++ - 'A' + 10;
		}
	}

	if (byteCount)
		*byteCount = asUINT(size_t(end - val.c_str()));

	return res;
}
double parseFloat(const string &val, asUINT *byteCount)
{
	char *end;

	// WinCE doesn't have setlocale. Some quick testing on my current platform
	// still manages to parse the numbers such as "3.14" even if the decimal for the
	// locale is ",".
#if !defined(_WIN32_WCE) && !defined(ANDROID) && !defined(__psp2__)
	// Set the locale to C so that we are guaranteed to parse the float value correctly
	char *tmp = setlocale(LC_NUMERIC, 0);
	string orig = tmp ? tmp : "C";
	setlocale(LC_NUMERIC, "C");
#endif

	double res = strtod(val.c_str(), &end);

#if !defined(_WIN32_WCE) && !defined(ANDROID) && !defined(__psp2__)
	// Restore the locale
	setlocale(LC_NUMERIC, orig.c_str());
#endif

	if (byteCount)
		*byteCount = asUINT(size_t(end - val.c_str()));

	return res;
}
static string StringSubString(asUINT start, int count, const string &str)
{
	// Check for out-of-bounds
	string ret;
	if (start < str.length() && count != 0)
		ret = str.substr(start, (size_t)(count < 0 ? string::npos : count));

	return ret;
}
static bool StringEquals(const std::string& lhs, const std::string& rhs)
{
	return lhs == rhs;
}
void RegisterStdString_Native(asIScriptEngine *engine)
{
	int r = 0;
	UNUSED_VAR(r);

	// Register the string type
#if AS_CAN_USE_CPP11
	// With C++11 it is possible to use asGetTypeTraits to automatically determine the correct flags to use
	r = engine->RegisterObjectType("string", sizeof(string), asOBJ_VALUE | asGetTypeTraits<string>()); assert(r >= 0);
#else
	r = engine->RegisterObjectType("string", sizeof(string), asOBJ_VALUE | asOBJ_APP_CLASS_CDAK); assert(r >= 0);
#endif

	r = engine->RegisterStringFactory("string", GetStdStringFactorySingleton());

	// Register the object operator overloads
	r = engine->RegisterObjectBehaviour("string", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(ConstructString), asCALL_CDECL_OBJLAST); assert(r >= 0);
	r = engine->RegisterObjectBehaviour("string", asBEHAVE_CONSTRUCT, "void f(const string &in)", asFUNCTION(CopyConstructString), asCALL_CDECL_OBJLAST); assert(r >= 0);
	r = engine->RegisterObjectBehaviour("string", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(DestructString), asCALL_CDECL_OBJLAST); assert(r >= 0);
	r = engine->RegisterObjectMethod("string", "string &opAssign(const string &in)", asMETHODPR(string, operator =, (const string&), string&), asCALL_THISCALL); assert(r >= 0);
	// Need to use a wrapper on Mac OS X 10.7/XCode 4.3 and CLang/LLVM, otherwise the linker fails
	r = engine->RegisterObjectMethod("string", "string &opAddAssign(const string &in)", asFUNCTION(AddAssignStringToString), asCALL_CDECL_OBJLAST); assert(r >= 0);
	//	r = engine->RegisterObjectMethod("string", "string &opAddAssign(const string &in)", asMETHODPR(string, operator+=, (const string&), string&), asCALL_THISCALL); assert( r >= 0 );

		// Need to use a wrapper for operator== otherwise gcc 4.7 fails to compile
	r = engine->RegisterObjectMethod("string", "bool opEquals(const string &in) const", asFUNCTIONPR(StringEquals, (const string &, const string &), bool), asCALL_CDECL_OBJFIRST); assert(r >= 0);
	r = engine->RegisterObjectMethod("string", "int opCmp(const string &in) const", asFUNCTION(StringCmp), asCALL_CDECL_OBJFIRST); assert(r >= 0);
	r = engine->RegisterObjectMethod("string", "string opAdd(const string &in) const", asFUNCTIONPR(operator +, (const string &, const string &), string), asCALL_CDECL_OBJFIRST); assert(r >= 0);

	// The string length can be accessed through methods or through virtual property
	// TODO: Register as size() for consistency with other types
#if AS_USE_ACCESSORS != 1
	r = engine->RegisterObjectMethod("string", "uint length() const", asFUNCTION(StringLength), asCALL_CDECL_OBJLAST); assert(r >= 0);
#endif
	r = engine->RegisterObjectMethod("string", "void resize(uint)", asFUNCTION(StringResize), asCALL_CDECL_OBJLAST); assert(r >= 0);
#if AS_USE_STLNAMES != 1 && AS_USE_ACCESSORS == 1
	// Don't register these if STL names is used, as they conflict with the method size()
	r = engine->RegisterObjectMethod("string", "uint get_length() const property", asFUNCTION(StringLength), asCALL_CDECL_OBJLAST); assert(r >= 0);
	r = engine->RegisterObjectMethod("string", "void set_length(uint) property", asFUNCTION(StringResize), asCALL_CDECL_OBJLAST); assert(r >= 0);
#endif
	// Need to use a wrapper on Mac OS X 10.7/XCode 4.3 and CLang/LLVM, otherwise the linker fails
//	r = engine->RegisterObjectMethod("string", "bool isEmpty() const", asMETHOD(string, empty), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectMethod("string", "bool isEmpty() const", asFUNCTION(StringIsEmpty), asCALL_CDECL_OBJLAST); assert(r >= 0);

	// Register the index operator, both as a mutator and as an inspector
	// Note that we don't register the operator[] directly, as it doesn't do bounds checking
	r = engine->RegisterObjectMethod("string", "uint8 &opIndex(uint)", asFUNCTION(StringCharAt), asCALL_CDECL_OBJLAST); assert(r >= 0);
	r = engine->RegisterObjectMethod("string", "const uint8 &opIndex(uint) const", asFUNCTION(StringCharAt), asCALL_CDECL_OBJLAST); assert(r >= 0);

	// Automatic conversion from values
	r = engine->RegisterObjectMethod("string", "string &opAssign(double)", asFUNCTION(AssignDoubleToString), asCALL_CDECL_OBJLAST); assert(r >= 0);
	r = engine->RegisterObjectMethod("string", "string &opAddAssign(double)", asFUNCTION(AddAssignDoubleToString), asCALL_CDECL_OBJLAST); assert(r >= 0);
	r = engine->RegisterObjectMethod("string", "string opAdd(double) const", asFUNCTION(AddStringDouble), asCALL_CDECL_OBJFIRST); assert(r >= 0);
	r = engine->RegisterObjectMethod("string", "string opAdd_r(double) const", asFUNCTION(AddDoubleString), asCALL_CDECL_OBJLAST); assert(r >= 0);

	r = engine->RegisterObjectMethod("string", "string &opAssign(float)", asFUNCTION(AssignFloatToString), asCALL_CDECL_OBJLAST); assert(r >= 0);
	r = engine->RegisterObjectMethod("string", "string &opAddAssign(float)", asFUNCTION(AddAssignFloatToString), asCALL_CDECL_OBJLAST); assert(r >= 0);
	r = engine->RegisterObjectMethod("string", "string opAdd(float) const", asFUNCTION(AddStringFloat), asCALL_CDECL_OBJFIRST); assert(r >= 0);
	r = engine->RegisterObjectMethod("string", "string opAdd_r(float) const", asFUNCTION(AddFloatString), asCALL_CDECL_OBJLAST); assert(r >= 0);

	r = engine->RegisterObjectMethod("string", "string &opAssign(int64)", asFUNCTION(AssignInt64ToString), asCALL_CDECL_OBJLAST); assert(r >= 0);
	r = engine->RegisterObjectMethod("string", "string &opAddAssign(int64)", asFUNCTION(AddAssignInt64ToString), asCALL_CDECL_OBJLAST); assert(r >= 0);
	r = engine->RegisterObjectMethod("string", "string opAdd(int64) const", asFUNCTION(AddStringInt64), asCALL_CDECL_OBJFIRST); assert(r >= 0);
	r = engine->RegisterObjectMethod("string", "string opAdd_r(int64) const", asFUNCTION(AddInt64String), asCALL_CDECL_OBJLAST); assert(r >= 0);

	r = engine->RegisterObjectMethod("string", "string &opAssign(uint64)", asFUNCTION(AssignUInt64ToString), asCALL_CDECL_OBJLAST); assert(r >= 0);
	r = engine->RegisterObjectMethod("string", "string &opAddAssign(uint64)", asFUNCTION(AddAssignUInt64ToString), asCALL_CDECL_OBJLAST); assert(r >= 0);
	r = engine->RegisterObjectMethod("string", "string opAdd(uint64) const", asFUNCTION(AddStringUInt64), asCALL_CDECL_OBJFIRST); assert(r >= 0);
	r = engine->RegisterObjectMethod("string", "string opAdd_r(uint64) const", asFUNCTION(AddUInt64String), asCALL_CDECL_OBJLAST); assert(r >= 0);

	r = engine->RegisterObjectMethod("string", "string &opAssign(bool)", asFUNCTION(AssignBoolToString), asCALL_CDECL_OBJLAST); assert(r >= 0);
	r = engine->RegisterObjectMethod("string", "string &opAddAssign(bool)", asFUNCTION(AddAssignBoolToString), asCALL_CDECL_OBJLAST); assert(r >= 0);
	r = engine->RegisterObjectMethod("string", "string opAdd(bool) const", asFUNCTION(AddStringBool), asCALL_CDECL_OBJFIRST); assert(r >= 0);
	r = engine->RegisterObjectMethod("string", "string opAdd_r(bool) const", asFUNCTION(AddBoolString), asCALL_CDECL_OBJLAST); assert(r >= 0);

	// Utilities
	r = engine->RegisterObjectMethod("string", "string substr(uint start = 0, int count = -1) const", asFUNCTION(StringSubString), asCALL_CDECL_OBJLAST); assert(r >= 0);
	r = engine->RegisterObjectMethod("string", "int findFirst(const string &in, uint start = 0) const", asFUNCTION(StringFindFirst), asCALL_CDECL_OBJLAST); assert(r >= 0);
	r = engine->RegisterObjectMethod("string", "int findFirstOf(const string &in, uint start = 0) const", asFUNCTION(StringFindFirstOf), asCALL_CDECL_OBJLAST); assert(r >= 0);
	r = engine->RegisterObjectMethod("string", "int findFirstNotOf(const string &in, uint start = 0) const", asFUNCTION(StringFindFirstNotOf), asCALL_CDECL_OBJLAST); assert(r >= 0);
	r = engine->RegisterObjectMethod("string", "int findLast(const string &in, int start = -1) const", asFUNCTION(StringFindLast), asCALL_CDECL_OBJLAST); assert(r >= 0);
	r = engine->RegisterObjectMethod("string", "int findLastOf(const string &in, int start = -1) const", asFUNCTION(StringFindLastOf), asCALL_CDECL_OBJLAST); assert(r >= 0);
	r = engine->RegisterObjectMethod("string", "int findLastNotOf(const string &in, int start = -1) const", asFUNCTION(StringFindLastNotOf), asCALL_CDECL_OBJLAST); assert(r >= 0);
	r = engine->RegisterObjectMethod("string", "void insert(uint pos, const string &in other)", asFUNCTION(StringInsert), asCALL_CDECL_OBJLAST); assert(r >= 0);
	r = engine->RegisterObjectMethod("string", "void erase(uint pos, int count = -1)", asFUNCTION(StringErase), asCALL_CDECL_OBJLAST); assert(r >= 0);
	r = engine->RegisterObjectMethod("string", "string replace(const string &in, const string &in, uint64 o = 0)", asFUNCTION(StringReplace), asCALL_CDECL_OBJLAST); assert(r >= 0);


	r = engine->RegisterGlobalFunction("string formatInt(int64 val, const string &in options = \"\", uint width = 0)", asFUNCTION(formatInt), asCALL_CDECL); assert(r >= 0);
	r = engine->RegisterGlobalFunction("string formatUInt(uint64 val, const string &in options = \"\", uint width = 0)", asFUNCTION(formatUInt), asCALL_CDECL); assert(r >= 0);
	r = engine->RegisterGlobalFunction("string formatFloat(double val, const string &in options = \"\", uint width = 0, uint precision = 0)", asFUNCTION(formatFloat), asCALL_CDECL); assert(r >= 0);
	r = engine->RegisterGlobalFunction("int64 parseInt(const string &in, uint base = 10, uint &out byteCount = 0)", asFUNCTION(parseInt), asCALL_CDECL); assert(r >= 0);
	r = engine->RegisterGlobalFunction("uint64 parseUInt(const string &in, uint base = 10, uint &out byteCount = 0)", asFUNCTION(parseUInt), asCALL_CDECL); assert(r >= 0);
	r = engine->RegisterGlobalFunction("double parseFloat(const string &in, uint &out byteCount = 0)", asFUNCTION(parseFloat), asCALL_CDECL); assert(r >= 0);

#if AS_USE_STLNAMES == 1
	// Same as length
	r = engine->RegisterObjectMethod("string", "uint size() const", asFUNCTION(StringLength), asCALL_CDECL_OBJLAST); assert(r >= 0);
	// Same as isEmpty
	r = engine->RegisterObjectMethod("string", "bool empty() const", asFUNCTION(StringIsEmpty), asCALL_CDECL_OBJLAST); assert(r >= 0);
	// Same as findFirst
	r = engine->RegisterObjectMethod("string", "int find(const string &in, uint start = 0) const", asFUNCTION(StringFindFirst), asCALL_CDECL_OBJLAST); assert(r >= 0);
	// Same as findLast
	r = engine->RegisterObjectMethod("string", "int rfind(const string &in, int start = -1) const", asFUNCTION(StringFindLast), asCALL_CDECL_OBJLAST); assert(r >= 0);
#endif

	// TODO: Implement the following
	// findAndReplace - replaces a text found in the string
	// replaceRange - replaces a range of bytes in the string
	// multiply/times/opMul/opMul_r - takes the string and multiplies it n times, e.g. "-".multiply(5) returns "-----"
}
char ToChar(const std::string& Symbol)
{
	return Symbol.empty() ? '\0' : Symbol[0];
}
string ToLower(const std::string& Symbol)
{
	return Tomahawk::Rest::Stroke(Symbol).ToLower().R();
}
string ToUpper(const std::string& Symbol)
{
	return Tomahawk::Rest::Stroke(Symbol).ToUpper().R();
}
string ToStrInt8(char Value)
{
	return std::to_string(Value);
}
string ToStrInt16(short Value)
{
	return std::to_string(Value);
}
string ToStrInt(int Value)
{
	return std::to_string(Value);
}
string ToStrInt64(int64_t Value)
{
	return std::to_string(Value);
}
string ToStrUInt8(unsigned char Value)
{
	return std::to_string(Value);
}
string ToStrUInt16(unsigned short Value)
{
	return std::to_string(Value);
}
string ToStrUInt(unsigned int Value)
{
	return std::to_string(Value);
}
string ToStrUInt64(uint64_t Value)
{
	return std::to_string(Value);
}
string ToStrFloat(float Value)
{
	return std::to_string(Value);
}
string ToStrDouble(double Value)
{
	return std::to_string(Value);
}
static void ConstructStringGeneric(asIScriptGeneric * gen)
{
	new (gen->GetObject()) string();
}
static void CopyConstructStringGeneric(asIScriptGeneric * gen)
{
	string * a = static_cast<string *>(gen->GetArgObject(0));
	new (gen->GetObject()) string(*a);
}
static void DestructStringGeneric(asIScriptGeneric * gen)
{
	string * ptr = static_cast<string *>(gen->GetObject());
	ptr->~string();
}
static void AssignStringGeneric(asIScriptGeneric *gen)
{
	string * a = static_cast<string *>(gen->GetArgObject(0));
	string * self = static_cast<string *>(gen->GetObject());
	*self = *a;
	gen->SetReturnAddress(self);
}
static void AddAssignStringGeneric(asIScriptGeneric *gen)
{
	string * a = static_cast<string *>(gen->GetArgObject(0));
	string * self = static_cast<string *>(gen->GetObject());
	*self += *a;
	gen->SetReturnAddress(self);
}
static void StringEqualsGeneric(asIScriptGeneric * gen)
{
	string * a = static_cast<string *>(gen->GetObject());
	string * b = static_cast<string *>(gen->GetArgAddress(0));
	*(bool*)gen->GetAddressOfReturnLocation() = (*a == *b);
}
static void StringCmpGeneric(asIScriptGeneric * gen)
{
	string * a = static_cast<string *>(gen->GetObject());
	string * b = static_cast<string *>(gen->GetArgAddress(0));

	int cmp = 0;
	if (*a < *b) cmp = -1;
	else if (*a > *b) cmp = 1;

	*(int*)gen->GetAddressOfReturnLocation() = cmp;
}
static void StringAddGeneric(asIScriptGeneric * gen)
{
	string * a = static_cast<string *>(gen->GetObject());
	string * b = static_cast<string *>(gen->GetArgAddress(0));
	string ret_val = *a + *b;
	gen->SetReturnObject(&ret_val);
}
static void StringLengthGeneric(asIScriptGeneric * gen)
{
	string * self = static_cast<string *>(gen->GetObject());
	*static_cast<asUINT *>(gen->GetAddressOfReturnLocation()) = (asUINT)self->length();
}
static void StringIsEmptyGeneric(asIScriptGeneric * gen)
{
	string * self = reinterpret_cast<string *>(gen->GetObject());
	*reinterpret_cast<bool *>(gen->GetAddressOfReturnLocation()) = StringIsEmpty(*self);
}
static void StringResizeGeneric(asIScriptGeneric * gen)
{
	string * self = static_cast<string *>(gen->GetObject());
	self->resize(*static_cast<asUINT *>(gen->GetAddressOfArg(0)));
}
static void StringInsert_Generic(asIScriptGeneric *gen)
{
	string * self = static_cast<string *>(gen->GetObject());
	asUINT pos = gen->GetArgDWord(0);
	string *other = reinterpret_cast<string*>(gen->GetArgAddress(1));
	StringInsert(pos, *other, *self);
}
static void StringErase_Generic(asIScriptGeneric *gen)
{
	string * self = static_cast<string *>(gen->GetObject());
	asUINT pos = gen->GetArgDWord(0);
	int count = int(gen->GetArgDWord(1));
	StringErase(pos, count, *self);
}
static void StringFindFirst_Generic(asIScriptGeneric * gen)
{
	string *find = reinterpret_cast<string*>(gen->GetArgAddress(0));
	asUINT start = gen->GetArgDWord(1);
	string *self = reinterpret_cast<string *>(gen->GetObject());
	*reinterpret_cast<int *>(gen->GetAddressOfReturnLocation()) = StringFindFirst(*find, start, *self);
}
static void StringFindLast_Generic(asIScriptGeneric * gen)
{
	string *find = reinterpret_cast<string*>(gen->GetArgAddress(0));
	asUINT start = gen->GetArgDWord(1);
	string *self = reinterpret_cast<string *>(gen->GetObject());
	*reinterpret_cast<int *>(gen->GetAddressOfReturnLocation()) = StringFindLast(*find, start, *self);
}
static void StringFindFirstOf_Generic(asIScriptGeneric * gen)
{
	string *find = reinterpret_cast<string*>(gen->GetArgAddress(0));
	asUINT start = gen->GetArgDWord(1);
	string *self = reinterpret_cast<string *>(gen->GetObject());
	*reinterpret_cast<int *>(gen->GetAddressOfReturnLocation()) = StringFindFirstOf(*find, start, *self);
}
static void StringFindLastOf_Generic(asIScriptGeneric * gen)
{
	string *find = reinterpret_cast<string*>(gen->GetArgAddress(0));
	asUINT start = gen->GetArgDWord(1);
	string *self = reinterpret_cast<string *>(gen->GetObject());
	*reinterpret_cast<int *>(gen->GetAddressOfReturnLocation()) = StringFindLastOf(*find, start, *self);
}
static void StringFindFirstNotOf_Generic(asIScriptGeneric * gen)
{
	string *find = reinterpret_cast<string*>(gen->GetArgAddress(0));
	asUINT start = gen->GetArgDWord(1);
	string *self = reinterpret_cast<string *>(gen->GetObject());
	*reinterpret_cast<int *>(gen->GetAddressOfReturnLocation()) = StringFindFirstNotOf(*find, start, *self);
}
static void StringFindLastNotOf_Generic(asIScriptGeneric * gen)
{
	string *find = reinterpret_cast<string*>(gen->GetArgAddress(0));
	asUINT start = gen->GetArgDWord(1);
	string *self = reinterpret_cast<string *>(gen->GetObject());
	*reinterpret_cast<int *>(gen->GetAddressOfReturnLocation()) = StringFindLastNotOf(*find, start, *self);
}
static void formatInt_Generic(asIScriptGeneric * gen)
{
	asINT64 val = gen->GetArgQWord(0);
	string *options = reinterpret_cast<string*>(gen->GetArgAddress(1));
	asUINT width = gen->GetArgDWord(2);
	new(gen->GetAddressOfReturnLocation()) string(formatInt(val, *options, width));
}
static void formatUInt_Generic(asIScriptGeneric * gen)
{
	asQWORD val = gen->GetArgQWord(0);
	string *options = reinterpret_cast<string*>(gen->GetArgAddress(1));
	asUINT width = gen->GetArgDWord(2);
	new(gen->GetAddressOfReturnLocation()) string(formatUInt(val, *options, width));
}
static void formatFloat_Generic(asIScriptGeneric *gen)
{
	double val = gen->GetArgDouble(0);
	string *options = reinterpret_cast<string*>(gen->GetArgAddress(1));
	asUINT width = gen->GetArgDWord(2);
	asUINT precision = gen->GetArgDWord(3);
	new(gen->GetAddressOfReturnLocation()) string(formatFloat(val, *options, width, precision));
}
static void parseInt_Generic(asIScriptGeneric *gen)
{
	string *str = reinterpret_cast<string*>(gen->GetArgAddress(0));
	asUINT base = gen->GetArgDWord(1);
	asUINT *byteCount = reinterpret_cast<asUINT*>(gen->GetArgAddress(2));
	gen->SetReturnQWord(parseInt(*str, base, byteCount));
}
static void parseUInt_Generic(asIScriptGeneric *gen)
{
	string *str = reinterpret_cast<string*>(gen->GetArgAddress(0));
	asUINT base = gen->GetArgDWord(1);
	asUINT *byteCount = reinterpret_cast<asUINT*>(gen->GetArgAddress(2));
	gen->SetReturnQWord(parseUInt(*str, base, byteCount));
}
static void parseFloat_Generic(asIScriptGeneric *gen)
{
	string *str = reinterpret_cast<string*>(gen->GetArgAddress(0));
	asUINT *byteCount = reinterpret_cast<asUINT*>(gen->GetArgAddress(1));
	gen->SetReturnDouble(parseFloat(*str, byteCount));
}
static void StringCharAtGeneric(asIScriptGeneric * gen)
{
	unsigned int index = gen->GetArgDWord(0);
	string * self = static_cast<string *>(gen->GetObject());

	if (index >= self->size())
	{
		// Set a script exception
		asIScriptContext *ctx = asGetActiveContext();
		ctx->SetException("Out of range");

		gen->SetReturnAddress(0);
	}
	else
	{
		gen->SetReturnAddress(&(self->operator [](index)));
	}
}
static void AssignInt2StringGeneric(asIScriptGeneric *gen)
{
	asINT64 *a = static_cast<asINT64*>(gen->GetAddressOfArg(0));
	string *self = static_cast<string*>(gen->GetObject());
	std::stringstream sstr;
	sstr << *a;
	*self = sstr.str();
	gen->SetReturnAddress(self);
}
static void AssignUInt2StringGeneric(asIScriptGeneric *gen)
{
	asQWORD *a = static_cast<asQWORD*>(gen->GetAddressOfArg(0));
	string *self = static_cast<string*>(gen->GetObject());
	std::stringstream sstr;
	sstr << *a;
	*self = sstr.str();
	gen->SetReturnAddress(self);
}
static void AssignDouble2StringGeneric(asIScriptGeneric *gen)
{
	double *a = static_cast<double*>(gen->GetAddressOfArg(0));
	string *self = static_cast<string*>(gen->GetObject());
	std::stringstream sstr;
	sstr << *a;
	*self = sstr.str();
	gen->SetReturnAddress(self);
}
static void AssignFloat2StringGeneric(asIScriptGeneric *gen)
{
	float *a = static_cast<float*>(gen->GetAddressOfArg(0));
	string *self = static_cast<string*>(gen->GetObject());
	std::stringstream sstr;
	sstr << *a;
	*self = sstr.str();
	gen->SetReturnAddress(self);
}
static void AssignBool2StringGeneric(asIScriptGeneric *gen)
{
	bool *a = static_cast<bool*>(gen->GetAddressOfArg(0));
	string *self = static_cast<string*>(gen->GetObject());
	std::stringstream sstr;
	sstr << (*a ? "true" : "false");
	*self = sstr.str();
	gen->SetReturnAddress(self);
}
static void AddAssignDouble2StringGeneric(asIScriptGeneric * gen)
{
	double * a = static_cast<double *>(gen->GetAddressOfArg(0));
	string * self = static_cast<string *>(gen->GetObject());
	std::stringstream sstr;
	sstr << *a;
	*self += sstr.str();
	gen->SetReturnAddress(self);
}
static void AddAssignFloat2StringGeneric(asIScriptGeneric * gen)
{
	float * a = static_cast<float *>(gen->GetAddressOfArg(0));
	string * self = static_cast<string *>(gen->GetObject());
	std::stringstream sstr;
	sstr << *a;
	*self += sstr.str();
	gen->SetReturnAddress(self);
}
static void AddAssignInt2StringGeneric(asIScriptGeneric * gen)
{
	asINT64 * a = static_cast<asINT64 *>(gen->GetAddressOfArg(0));
	string * self = static_cast<string *>(gen->GetObject());
	std::stringstream sstr;
	sstr << *a;
	*self += sstr.str();
	gen->SetReturnAddress(self);
}
static void AddAssignUInt2StringGeneric(asIScriptGeneric * gen)
{
	asQWORD * a = static_cast<asQWORD *>(gen->GetAddressOfArg(0));
	string * self = static_cast<string *>(gen->GetObject());
	std::stringstream sstr;
	sstr << *a;
	*self += sstr.str();
	gen->SetReturnAddress(self);
}
static void AddAssignBool2StringGeneric(asIScriptGeneric * gen)
{
	bool * a = static_cast<bool *>(gen->GetAddressOfArg(0));
	string * self = static_cast<string *>(gen->GetObject());
	std::stringstream sstr;
	sstr << (*a ? "true" : "false");
	*self += sstr.str();
	gen->SetReturnAddress(self);
}
static void AddString2DoubleGeneric(asIScriptGeneric * gen)
{
	string * a = static_cast<string *>(gen->GetObject());
	double * b = static_cast<double *>(gen->GetAddressOfArg(0));
	std::stringstream sstr;
	sstr << *a << *b;
	std::string ret_val = sstr.str();
	gen->SetReturnObject(&ret_val);
}
static void AddString2FloatGeneric(asIScriptGeneric * gen)
{
	string * a = static_cast<string *>(gen->GetObject());
	float * b = static_cast<float *>(gen->GetAddressOfArg(0));
	std::stringstream sstr;
	sstr << *a << *b;
	std::string ret_val = sstr.str();
	gen->SetReturnObject(&ret_val);
}
static void AddString2IntGeneric(asIScriptGeneric * gen)
{
	string * a = static_cast<string *>(gen->GetObject());
	asINT64 * b = static_cast<asINT64 *>(gen->GetAddressOfArg(0));
	std::stringstream sstr;
	sstr << *a << *b;
	std::string ret_val = sstr.str();
	gen->SetReturnObject(&ret_val);
}
static void AddString2UIntGeneric(asIScriptGeneric * gen)
{
	string * a = static_cast<string *>(gen->GetObject());
	asQWORD * b = static_cast<asQWORD *>(gen->GetAddressOfArg(0));
	std::stringstream sstr;
	sstr << *a << *b;
	std::string ret_val = sstr.str();
	gen->SetReturnObject(&ret_val);
}
static void AddString2BoolGeneric(asIScriptGeneric * gen)
{
	string * a = static_cast<string *>(gen->GetObject());
	bool * b = static_cast<bool *>(gen->GetAddressOfArg(0));
	std::stringstream sstr;
	sstr << *a << (*b ? "true" : "false");
	std::string ret_val = sstr.str();
	gen->SetReturnObject(&ret_val);
}
static void AddDouble2StringGeneric(asIScriptGeneric * gen)
{
	double* a = static_cast<double *>(gen->GetAddressOfArg(0));
	string * b = static_cast<string *>(gen->GetObject());
	std::stringstream sstr;
	sstr << *a << *b;
	std::string ret_val = sstr.str();
	gen->SetReturnObject(&ret_val);
}
static void AddFloat2StringGeneric(asIScriptGeneric * gen)
{
	float* a = static_cast<float *>(gen->GetAddressOfArg(0));
	string * b = static_cast<string *>(gen->GetObject());
	std::stringstream sstr;
	sstr << *a << *b;
	std::string ret_val = sstr.str();
	gen->SetReturnObject(&ret_val);
}
static void AddInt2StringGeneric(asIScriptGeneric * gen)
{
	asINT64* a = static_cast<asINT64 *>(gen->GetAddressOfArg(0));
	string * b = static_cast<string *>(gen->GetObject());
	std::stringstream sstr;
	sstr << *a << *b;
	std::string ret_val = sstr.str();
	gen->SetReturnObject(&ret_val);
}
static void AddUInt2StringGeneric(asIScriptGeneric * gen)
{
	asQWORD* a = static_cast<asQWORD *>(gen->GetAddressOfArg(0));
	string * b = static_cast<string *>(gen->GetObject());
	std::stringstream sstr;
	sstr << *a << *b;
	std::string ret_val = sstr.str();
	gen->SetReturnObject(&ret_val);
}
static void AddBool2StringGeneric(asIScriptGeneric * gen)
{
	bool* a = static_cast<bool *>(gen->GetAddressOfArg(0));
	string * b = static_cast<string *>(gen->GetObject());
	std::stringstream sstr;
	sstr << (*a ? "true" : "false") << *b;
	std::string ret_val = sstr.str();
	gen->SetReturnObject(&ret_val);
}
static void StringSubString_Generic(asIScriptGeneric *gen)
{
	// Get the arguments
	string *str = (string*)gen->GetObject();
	asUINT  start = *(int*)gen->GetAddressOfArg(0);
	int     count = *(int*)gen->GetAddressOfArg(1);

	// Return the substring
	new(gen->GetAddressOfReturnLocation()) string(StringSubString(start, count, *str));
}
static VMCArray *StringSplit(const string &delim, const string &str)
{
	// Obtain a pointer to the engine
	asIScriptContext *ctx = asGetActiveContext();
	asIScriptEngine *engine = ctx->GetEngine();

	// TODO: This should only be done once
	// TODO: This assumes that VMCArray was already registered
	asITypeInfo *arrayType = engine->GetTypeInfoByDecl("array<string>");

	// Create the array object
	VMCArray *array = VMCArray::Create(arrayType);

	// Find the existence of the delimiter in the input string
	int pos = 0, prev = 0, count = 0;
	while ((pos = (int)str.find(delim, prev)) != (int)string::npos)
	{
		// Add the part to the array
		array->Resize(array->GetSize() + 1);
		((string*)array->At(count))->assign(&str[prev], pos - prev);

		// Find the next part
		count++;
		prev = pos + (int)delim.length();
	}

	// Add the remaining part
	array->Resize(array->GetSize() + 1);
	((string*)array->At(count))->assign(&str[prev]);

	return array;
}
static void StringSplit_Generic(asIScriptGeneric *gen)
{
	// Get the arguments
	string *str = (string*)gen->GetObject();
	string *delim = *(string**)gen->GetAddressOfArg(0);

	// Return the array by handle
	*(VMCArray**)gen->GetAddressOfReturnLocation() = StringSplit(*delim, *str);
}
static string StringJoin(const VMCArray &array, const string &delim)
{
	// Create the new string
	string str = "";
	if (array.GetSize())
	{
		int n;
		for (n = 0; n < (int)array.GetSize() - 1; n++)
		{
			str += *(string*)array.At(n);
			str += delim;
		}

		// Add the last part
		str += *(string*)array.At(n);
	}

	return str;
}
static void StringJoin_Generic(asIScriptGeneric *gen)
{
	// Get the arguments
	VMCArray  *array = *(VMCArray**)gen->GetAddressOfArg(0);
	string *delim = *(string**)gen->GetAddressOfArg(1);

	// Return the string
	new(gen->GetAddressOfReturnLocation()) string(StringJoin(*array, *delim));
}
void RegisterStdString_Generic(asIScriptEngine *engine)
{
	int r = 0;
	UNUSED_VAR(r);

	// Register the string type
	r = engine->RegisterObjectType("string", sizeof(string), asOBJ_VALUE | asOBJ_APP_CLASS_CDAK); assert(r >= 0);

	r = engine->RegisterStringFactory("string", GetStdStringFactorySingleton());

	// Register the object operator overloads
	r = engine->RegisterObjectBehaviour("string", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(ConstructStringGeneric), asCALL_GENERIC); assert(r >= 0);
	r = engine->RegisterObjectBehaviour("string", asBEHAVE_CONSTRUCT, "void f(const string &in)", asFUNCTION(CopyConstructStringGeneric), asCALL_GENERIC); assert(r >= 0);
	r = engine->RegisterObjectBehaviour("string", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(DestructStringGeneric), asCALL_GENERIC); assert(r >= 0);
	r = engine->RegisterObjectMethod("string", "string &opAssign(const string &in)", asFUNCTION(AssignStringGeneric), asCALL_GENERIC); assert(r >= 0);
	r = engine->RegisterObjectMethod("string", "string &opAddAssign(const string &in)", asFUNCTION(AddAssignStringGeneric), asCALL_GENERIC); assert(r >= 0);

	r = engine->RegisterObjectMethod("string", "bool opEquals(const string &in) const", asFUNCTION(StringEqualsGeneric), asCALL_GENERIC); assert(r >= 0);
	r = engine->RegisterObjectMethod("string", "int opCmp(const string &in) const", asFUNCTION(StringCmpGeneric), asCALL_GENERIC); assert(r >= 0);
	r = engine->RegisterObjectMethod("string", "string opAdd(const string &in) const", asFUNCTION(StringAddGeneric), asCALL_GENERIC); assert(r >= 0);

	// Register the object methods
#if AS_USE_ACCESSORS != 1
	r = engine->RegisterObjectMethod("string", "uint length() const", asFUNCTION(StringLengthGeneric), asCALL_GENERIC); assert(r >= 0);
#endif
	r = engine->RegisterObjectMethod("string", "void resize(uint)", asFUNCTION(StringResizeGeneric), asCALL_GENERIC); assert(r >= 0);
#if AS_USE_STLNAMES != 1 && AS_USE_ACCESSORS == 1
	r = engine->RegisterObjectMethod("string", "uint get_length() const property", asFUNCTION(StringLengthGeneric), asCALL_GENERIC); assert(r >= 0);
	r = engine->RegisterObjectMethod("string", "void set_length(uint) property", asFUNCTION(StringResizeGeneric), asCALL_GENERIC); assert(r >= 0);
#endif
	r = engine->RegisterObjectMethod("string", "bool isEmpty() const", asFUNCTION(StringIsEmptyGeneric), asCALL_GENERIC); assert(r >= 0);

	// Register the index operator, both as a mutator and as an inspector
	r = engine->RegisterObjectMethod("string", "uint8 &opIndex(uint)", asFUNCTION(StringCharAtGeneric), asCALL_GENERIC); assert(r >= 0);
	r = engine->RegisterObjectMethod("string", "const uint8 &opIndex(uint) const", asFUNCTION(StringCharAtGeneric), asCALL_GENERIC); assert(r >= 0);

	// Automatic conversion from values
	r = engine->RegisterObjectMethod("string", "string &opAssign(double)", asFUNCTION(AssignDouble2StringGeneric), asCALL_GENERIC); assert(r >= 0);
	r = engine->RegisterObjectMethod("string", "string &opAddAssign(double)", asFUNCTION(AddAssignDouble2StringGeneric), asCALL_GENERIC); assert(r >= 0);
	r = engine->RegisterObjectMethod("string", "string opAdd(double) const", asFUNCTION(AddString2DoubleGeneric), asCALL_GENERIC); assert(r >= 0);
	r = engine->RegisterObjectMethod("string", "string opAdd_r(double) const", asFUNCTION(AddDouble2StringGeneric), asCALL_GENERIC); assert(r >= 0);

	r = engine->RegisterObjectMethod("string", "string &opAssign(float)", asFUNCTION(AssignFloat2StringGeneric), asCALL_GENERIC); assert(r >= 0);
	r = engine->RegisterObjectMethod("string", "string &opAddAssign(float)", asFUNCTION(AddAssignFloat2StringGeneric), asCALL_GENERIC); assert(r >= 0);
	r = engine->RegisterObjectMethod("string", "string opAdd(float) const", asFUNCTION(AddString2FloatGeneric), asCALL_GENERIC); assert(r >= 0);
	r = engine->RegisterObjectMethod("string", "string opAdd_r(float) const", asFUNCTION(AddFloat2StringGeneric), asCALL_GENERIC); assert(r >= 0);

	r = engine->RegisterObjectMethod("string", "string &opAssign(int64)", asFUNCTION(AssignInt2StringGeneric), asCALL_GENERIC); assert(r >= 0);
	r = engine->RegisterObjectMethod("string", "string &opAddAssign(int64)", asFUNCTION(AddAssignInt2StringGeneric), asCALL_GENERIC); assert(r >= 0);
	r = engine->RegisterObjectMethod("string", "string opAdd(int64) const", asFUNCTION(AddString2IntGeneric), asCALL_GENERIC); assert(r >= 0);
	r = engine->RegisterObjectMethod("string", "string opAdd_r(int64) const", asFUNCTION(AddInt2StringGeneric), asCALL_GENERIC); assert(r >= 0);

	r = engine->RegisterObjectMethod("string", "string &opAssign(uint64)", asFUNCTION(AssignUInt2StringGeneric), asCALL_GENERIC); assert(r >= 0);
	r = engine->RegisterObjectMethod("string", "string &opAddAssign(uint64)", asFUNCTION(AddAssignUInt2StringGeneric), asCALL_GENERIC); assert(r >= 0);
	r = engine->RegisterObjectMethod("string", "string opAdd(uint64) const", asFUNCTION(AddString2UIntGeneric), asCALL_GENERIC); assert(r >= 0);
	r = engine->RegisterObjectMethod("string", "string opAdd_r(uint64) const", asFUNCTION(AddUInt2StringGeneric), asCALL_GENERIC); assert(r >= 0);

	r = engine->RegisterObjectMethod("string", "string &opAssign(bool)", asFUNCTION(AssignBool2StringGeneric), asCALL_GENERIC); assert(r >= 0);
	r = engine->RegisterObjectMethod("string", "string &opAddAssign(bool)", asFUNCTION(AddAssignBool2StringGeneric), asCALL_GENERIC); assert(r >= 0);
	r = engine->RegisterObjectMethod("string", "string opAdd(bool) const", asFUNCTION(AddString2BoolGeneric), asCALL_GENERIC); assert(r >= 0);
	r = engine->RegisterObjectMethod("string", "string opAdd_r(bool) const", asFUNCTION(AddBool2StringGeneric), asCALL_GENERIC); assert(r >= 0);

	r = engine->RegisterObjectMethod("string", "string substr(uint start = 0, int count = -1) const", asFUNCTION(StringSubString_Generic), asCALL_GENERIC); assert(r >= 0);
	r = engine->RegisterObjectMethod("string", "int findFirst(const string &in, uint start = 0) const", asFUNCTION(StringFindFirst_Generic), asCALL_GENERIC); assert(r >= 0);
	r = engine->RegisterObjectMethod("string", "int findFirstOf(const string &in, uint start = 0) const", asFUNCTION(StringFindFirstOf_Generic), asCALL_GENERIC); assert(r >= 0);
	r = engine->RegisterObjectMethod("string", "int findFirstNotOf(const string &in, uint start = 0) const", asFUNCTION(StringFindFirstNotOf_Generic), asCALL_GENERIC); assert(r >= 0);
	r = engine->RegisterObjectMethod("string", "int findLast(const string &in, int start = -1) const", asFUNCTION(StringFindLast_Generic), asCALL_GENERIC); assert(r >= 0);
	r = engine->RegisterObjectMethod("string", "int findLastOf(const string &in, int start = -1) const", asFUNCTION(StringFindLastOf_Generic), asCALL_GENERIC); assert(r >= 0);
	r = engine->RegisterObjectMethod("string", "int findLastNotOf(const string &in, int start = -1) const", asFUNCTION(StringFindLastNotOf_Generic), asCALL_GENERIC); assert(r >= 0);
	r = engine->RegisterObjectMethod("string", "void insert(uint pos, const string &in other)", asFUNCTION(StringInsert_Generic), asCALL_GENERIC); assert(r >= 0);
	r = engine->RegisterObjectMethod("string", "void erase(uint pos, int count = -1)", asFUNCTION(StringErase_Generic), asCALL_GENERIC); assert(r >= 0);


	r = engine->RegisterGlobalFunction("string formatInt(int64 val, const string &in options = \"\", uint width = 0)", asFUNCTION(formatInt_Generic), asCALL_GENERIC); assert(r >= 0);
	r = engine->RegisterGlobalFunction("string formatUInt(uint64 val, const string &in options = \"\", uint width = 0)", asFUNCTION(formatUInt_Generic), asCALL_GENERIC); assert(r >= 0);
	r = engine->RegisterGlobalFunction("string formatFloat(double val, const string &in options = \"\", uint width = 0, uint precision = 0)", asFUNCTION(formatFloat_Generic), asCALL_GENERIC); assert(r >= 0);
	r = engine->RegisterGlobalFunction("int64 parseInt(const string &in, uint base = 10, uint &out byteCount = 0)", asFUNCTION(parseInt_Generic), asCALL_GENERIC); assert(r >= 0);
	r = engine->RegisterGlobalFunction("uint64 parseUInt(const string &in, uint base = 10, uint &out byteCount = 0)", asFUNCTION(parseUInt_Generic), asCALL_GENERIC); assert(r >= 0);
	r = engine->RegisterGlobalFunction("double parseFloat(const string &in, uint &out byteCount = 0)", asFUNCTION(parseFloat_Generic), asCALL_GENERIC); assert(r >= 0);
}
void VM_RegisterString(asIScriptEngine * engine)
{
	if (strstr(asGetLibraryOptions(), "AS_MAX_PORTABILITY"))
	{
		RegisterStdString_Generic(engine);
		engine->RegisterObjectMethod("string", "array<string>@ split(const string &in) const", asFUNCTION(StringSplit_Generic), asCALL_GENERIC);
		engine->RegisterGlobalFunction("string join(const array<string> &in, const string &in)", asFUNCTION(StringJoin_Generic), asCALL_GENERIC);
	}
	else
	{
		RegisterStdString_Native(engine);
		engine->RegisterObjectMethod("string", "array<string>@ split(const string &in) const", asFUNCTION(StringSplit), asCALL_CDECL_OBJLAST);
		engine->RegisterGlobalFunction("string join(const array<string> &in, const string &in)", asFUNCTION(StringJoin), asCALL_CDECL);
	}

	engine->RegisterGlobalFunction("uint8 toChar(const string &in)", asFUNCTION(ToChar), asCALL_CDECL);
	engine->RegisterGlobalFunction("string toLower(const string &in)", asFUNCTION(ToLower), asCALL_CDECL);
	engine->RegisterGlobalFunction("string toUpper(const string &in)", asFUNCTION(ToUpper), asCALL_CDECL);
	engine->RegisterGlobalFunction("string toStr(int8)", asFUNCTION(ToStrInt8), asCALL_CDECL);
	engine->RegisterGlobalFunction("string toStr(int16)", asFUNCTION(ToStrInt16), asCALL_CDECL);
	engine->RegisterGlobalFunction("string toStr(int)", asFUNCTION(ToStrInt), asCALL_CDECL);
	engine->RegisterGlobalFunction("string toStr(int64)", asFUNCTION(ToStrInt64), asCALL_CDECL);
	engine->RegisterGlobalFunction("string toStr(uint8)", asFUNCTION(ToStrUInt8), asCALL_CDECL);
	engine->RegisterGlobalFunction("string toStr(uint16)", asFUNCTION(ToStrUInt16), asCALL_CDECL);
	engine->RegisterGlobalFunction("string toStr(uint)", asFUNCTION(ToStrUInt), asCALL_CDECL);
	engine->RegisterGlobalFunction("string toStr(uint64)", asFUNCTION(ToStrUInt64), asCALL_CDECL);
	engine->RegisterGlobalFunction("string toStr(float)", asFUNCTION(ToStrFloat), asCALL_CDECL);
	engine->RegisterGlobalFunction("string toStr(double)", asFUNCTION(ToStrDouble), asCALL_CDECL);
}
void VM_FreeStringProxy()
{
	if (stringFactory)
	{
		if (stringFactory->stringCache.empty())
		{
			delete stringFactory;
			stringFactory = 0;
		}
	}
}