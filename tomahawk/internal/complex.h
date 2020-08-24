#ifndef SCRIPT_COMPLEX_H
#define SCRIPT_COMPLEX_H
#ifndef ANGELSCRIPT_H 
#include <angelscript.h>
#endif

struct Complex
{
	Complex();
	Complex(const Complex &other);
	Complex(float r, float i = 0);
	Complex &operator=(const Complex &other);
	Complex &operator+=(const Complex &other);
	Complex &operator-=(const Complex &other);
	Complex &operator*=(const Complex &other);
	Complex &operator/=(const Complex &other);
	float length() const;
	float squaredLength() const;
	Complex get_ri() const;
	void set_ri(const Complex &in);
	Complex get_ir() const;
	void set_ir(const Complex &in);
	bool operator==(const Complex &other) const;
	bool operator!=(const Complex &other) const;
	Complex operator+(const Complex &other) const;
	Complex operator-(const Complex &other) const;
	Complex operator*(const Complex &other) const;
	Complex operator/(const Complex &other) const;
	float r;
	float i;
};

void VM_RegisterComplex(asIScriptEngine *engine);
#endif
