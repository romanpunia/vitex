#include "datetime.h"
#include <string.h>
#include <assert.h>
#include <new>

using namespace std;
using namespace std::chrono;

static tm time_point_to_tm(const std::chrono::time_point<std::chrono::system_clock> &tp)
{
	time_t t = system_clock::to_time_t(tp);
	tm local;
#ifdef _MSC_VER
	localtime_s(&local, &t);
#else
	local = *localtime(&t);
#endif
	return local;
}
static bool tm_to_time_point(const tm &_tm, std::chrono::time_point<std::chrono::system_clock> &tp)
{
	tm localTm = _tm;
	localTm.tm_isdst = -1; // Always use current settings, so mktime doesn't modify the time for daylight savings

	time_t t = mktime(&localTm);
	if (t == -1)
		return false;

	// Verify if the members were modified, indicating an out-of-range value in input
	if (localTm.tm_year != _tm.tm_year ||
		localTm.tm_mon != _tm.tm_mon ||
		localTm.tm_mday != _tm.tm_mday ||
		localTm.tm_hour != _tm.tm_hour ||
		localTm.tm_min != _tm.tm_min ||
		localTm.tm_sec != _tm.tm_sec)
		return false;

	tp = system_clock::from_time_t(t);
	return true;
}

VMCDateTime::VMCDateTime() : tp(std::chrono::system_clock::now())
{
}
VMCDateTime::VMCDateTime(const VMCDateTime &o) : tp(o.tp)
{
}
VMCDateTime &VMCDateTime::operator=(const VMCDateTime &o)
{
	tp = o.tp;
	return *this;
}
asUINT VMCDateTime::getYear() const
{
	tm local = time_point_to_tm(tp);
	return local.tm_year + 1900;
}
asUINT VMCDateTime::getMonth() const
{
	tm local = time_point_to_tm(tp);
	return local.tm_mon + 1;
}
asUINT VMCDateTime::getDay() const
{
	tm local = time_point_to_tm(tp);
	return local.tm_mday;
}
asUINT VMCDateTime::getHour() const
{
	tm local = time_point_to_tm(tp);
	return local.tm_hour;
}
asUINT VMCDateTime::getMinute() const
{
	tm local = time_point_to_tm(tp);
	return local.tm_min;
}
asUINT VMCDateTime::getSecond() const
{
	tm local = time_point_to_tm(tp);
	return local.tm_sec;
}
bool VMCDateTime::setDate(asUINT year, asUINT month, asUINT day)
{
	tm local = time_point_to_tm(tp);
	local.tm_year = int(year) - 1900;
	local.tm_mon = month - 1;
	local.tm_mday = day;

	return tm_to_time_point(local, tp);
}
bool VMCDateTime::setTime(asUINT hour, asUINT minute, asUINT second)
{
	tm local = time_point_to_tm(tp);
	local.tm_hour = hour;
	local.tm_min = minute;
	local.tm_sec = second;

	return tm_to_time_point(local, tp);
}
VMCDateTime::VMCDateTime(asUINT year, asUINT month, asUINT day, asUINT hour, asUINT minute, asUINT second)
{
	tm local;
	local.tm_year = int(year) - 1900;
	local.tm_mon = month - 1;
	local.tm_mday = day;
	local.tm_hour = hour;
	local.tm_min = minute;
	local.tm_sec = second;
	if (!tm_to_time_point(local, tp))
	{
		// TODO: How should the user know the input was wrong? Throw exception or have a flag? Set to epoch?
		tp = std::chrono::system_clock::now();
	}
}
asINT64 VMCDateTime::operator-(const VMCDateTime &dt) const
{
	return (tp - dt.tp).count() / std::chrono::system_clock::period::den * std::chrono::system_clock::period::num;
}
VMCDateTime VMCDateTime::operator+(asINT64 seconds) const
{
	VMCDateTime dt(*this);
	dt.tp += std::chrono::system_clock::duration(seconds * std::chrono::system_clock::period::den / std::chrono::system_clock::period::num);
	return dt;
}
VMCDateTime &VMCDateTime::operator+=(asINT64 seconds)
{
	tp += std::chrono::system_clock::duration(seconds * std::chrono::system_clock::period::den / std::chrono::system_clock::period::num);
	return *this;
}
VMCDateTime operator+(asINT64 seconds, const VMCDateTime &other)
{
	return other + seconds;
}
VMCDateTime VMCDateTime::operator-(asINT64 seconds) const
{
	return *this + -seconds;
}
VMCDateTime &VMCDateTime::operator-=(asINT64 seconds)
{
	return *this += -seconds;
}
VMCDateTime operator-(asINT64 seconds, const VMCDateTime &other)
{
	return other + -seconds;
}
bool VMCDateTime::operator==(const VMCDateTime &other) const
{
	return tp == other.tp;
}
bool VMCDateTime::operator<(const VMCDateTime &other) const
{
	return tp < other.tp;
}

static int opCmp(const VMCDateTime &a, const VMCDateTime &b)
{
	if (a < b) return -1;
	if (a == b) return 0;
	return 1;
}
static void Construct(VMCDateTime *mem)
{
	new(mem) VMCDateTime();
}
static void ConstructCopy(VMCDateTime *mem, const VMCDateTime &o)
{
	new(mem) VMCDateTime(o);
}
static void ConstructSet(VMCDateTime *mem, asUINT year, asUINT month, asUINT day, asUINT hour, asUINT minute, asUINT second)
{
	new(mem) VMCDateTime(year, month, day, hour, minute, second);
}
void VM_RegisterDateTime(asIScriptEngine *engine)
{
	assert(strstr(asGetLibraryOptions(), "AS_MAX_PORTABILITY") == 0);

	int r = engine->RegisterObjectType("datetime", sizeof(VMCDateTime), asOBJ_VALUE | asOBJ_POD | asGetTypeTraits<VMCDateTime>()); assert(r >= 0);

	r = engine->RegisterObjectBehaviour("datetime", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(Construct), asCALL_CDECL_OBJLAST); assert(r >= 0);
	r = engine->RegisterObjectBehaviour("datetime", asBEHAVE_CONSTRUCT, "void f(const datetime &in)", asFUNCTION(ConstructCopy), asCALL_CDECL_OBJFIRST); assert(r >= 0);
	r = engine->RegisterObjectBehaviour("datetime", asBEHAVE_CONSTRUCT, "void f(uint, uint, uint, uint = 0, uint = 0, uint = 0)", asFUNCTION(ConstructSet), asCALL_CDECL_OBJFIRST); assert(r >= 0);
	r = engine->RegisterObjectMethod("datetime", "datetime &opAssign(const datetime &in)", asMETHOD(VMCDateTime, operator=), asCALL_THISCALL); assert(r >= 0);
	r = engine->RegisterObjectMethod("datetime", "uint get_year() const property", asMETHOD(VMCDateTime, getYear), asCALL_THISCALL); assert(r >= 0);
	r = engine->RegisterObjectMethod("datetime", "uint get_month() const property", asMETHOD(VMCDateTime, getMonth), asCALL_THISCALL); assert(r >= 0);
	r = engine->RegisterObjectMethod("datetime", "uint get_day() const property", asMETHOD(VMCDateTime, getDay), asCALL_THISCALL); assert(r >= 0);
	r = engine->RegisterObjectMethod("datetime", "uint get_hour() const property", asMETHOD(VMCDateTime, getHour), asCALL_THISCALL); assert(r >= 0);
	r = engine->RegisterObjectMethod("datetime", "uint get_minute() const property", asMETHOD(VMCDateTime, getMinute), asCALL_THISCALL); assert(r >= 0);
	r = engine->RegisterObjectMethod("datetime", "uint get_second() const property", asMETHOD(VMCDateTime, getSecond), asCALL_THISCALL); assert(r >= 0);
	r = engine->RegisterObjectMethod("datetime", "bool setDate(uint year, uint month, uint day)", asMETHOD(VMCDateTime, setDate), asCALL_THISCALL); assert(r >= 0);
	r = engine->RegisterObjectMethod("datetime", "bool setTime(uint hour, uint minute, uint second)", asMETHOD(VMCDateTime, setTime), asCALL_THISCALL); assert(r >= 0);
	r = engine->RegisterObjectMethod("datetime", "int64 opSub(const datetime &in) const", asMETHODPR(VMCDateTime, operator-, (const VMCDateTime &other) const, asINT64), asCALL_THISCALL); assert(r >= 0);
	r = engine->RegisterObjectMethod("datetime", "datetime opAdd(int64 seconds) const", asMETHOD(VMCDateTime, operator+), asCALL_THISCALL); assert(r >= 0);
	r = engine->RegisterObjectMethod("datetime", "datetime opAdd_r(int64 seconds) const", asFUNCTIONPR(operator+, (asINT64 seconds, const VMCDateTime &other), VMCDateTime), asCALL_CDECL_OBJLAST); assert(r >= 0);
	r = engine->RegisterObjectMethod("datetime", "datetime &opAddAssign(int64 seconds)", asMETHOD(VMCDateTime, operator+=), asCALL_THISCALL); assert(r >= 0);
	r = engine->RegisterObjectMethod("datetime", "datetime opSub(int64 seconds) const", asMETHODPR(VMCDateTime, operator-, (asINT64) const, VMCDateTime), asCALL_THISCALL); assert(r >= 0);
	r = engine->RegisterObjectMethod("datetime", "datetime opSub_r(int64 seconds) const", asFUNCTIONPR(operator-, (asINT64 seconds, const VMCDateTime &other), VMCDateTime), asCALL_CDECL_OBJLAST); assert(r >= 0);
	r = engine->RegisterObjectMethod("datetime", "datetime &opSubAssign(int64 seconds)", asMETHOD(VMCDateTime, operator-=), asCALL_THISCALL); assert(r >= 0);
	r = engine->RegisterObjectMethod("datetime", "bool opEquals(const datetime &in) const", asMETHODPR(VMCDateTime, operator==, (const VMCDateTime &other) const, bool), asCALL_THISCALL); assert(r >= 0);
	r = engine->RegisterObjectMethod("datetime", "int opCmp(const datetime &in) const", asFUNCTION(opCmp), asCALL_CDECL_OBJFIRST); assert(r >= 0);
}