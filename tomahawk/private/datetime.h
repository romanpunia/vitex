#ifndef SCRIPT_DATETIME_H
#define SCRIPT_DATETIME_H
#ifndef ANGELSCRIPT_H
#include <angelscript.h>
#endif
#include <chrono>

class VMCDateTime
{
public:
    VMCDateTime();
    VMCDateTime(const VMCDateTime &other);
    VMCDateTime(asUINT year, asUINT month, asUINT day, asUINT hour, asUINT minute, asUINT second);
    VMCDateTime &operator=(const VMCDateTime &other);
    asUINT getYear() const;
    asUINT getMonth() const;
    asUINT getDay() const;
    asUINT getHour() const;
    asUINT getMinute() const;
    asUINT getSecond() const;
    bool setDate(asUINT year, asUINT month, asUINT day);
    bool setTime(asUINT hour, asUINT minute, asUINT second);
    asINT64 operator-(const VMCDateTime &other) const;
    VMCDateTime operator+(asINT64 seconds) const;
    friend VMCDateTime operator+(asINT64 seconds, const VMCDateTime &other);
    VMCDateTime& operator+=(asINT64 seconds);
    VMCDateTime operator-(asINT64 seconds) const;
    friend VMCDateTime operator-(asINT64 seconds, const VMCDateTime &other);
    VMCDateTime& operator-=(asINT64 seconds);
    bool operator==(const VMCDateTime &other) const;
    bool operator<(const VMCDateTime &other) const;

protected:
    std::chrono::system_clock::time_point tp;
};

void VM_RegisterDateTime(asIScriptEngine *engine);
#endif