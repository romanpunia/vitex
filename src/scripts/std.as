class Nullable
{
	
}

class RefPromise<class T>
{
	T@ Get();
}

class Address
{
	
}

class Any
{
	Any@ Any();
	Any@ Any(?&in);
	Any@ Any(const int64&in);
	Any@ Any(const double&in);
	Any& opAssign(Any&in);
	void Store(?&in);
	bool Retrieve(?&out);
}

class Array<class T>
{
	funcdef bool Less(const T&in a, const T&in b);

	T[]@ Array(int&in);
	T[]@ Array(int&in, uint length);
	T[]@ Array(int&in, uint length, const T&in Value);
	T& opIndex(uint Index);
	const T& opIndex(uint Index) const;
	T[]& opAssign(const T[]&in);
	void InsertAt(uint Index, const T&in Value);
	void InsertAt(uint Index, const T[]&inout Array);
	void InsertLast(const T&in Value);
	void RemoveAt(uint Index);
	void RemoveLast();
	void RemoveRange(uint Start, uint Count);
	uint Size() const;
	void Reserve(uint length);
	void Resize(uint length);
	void SortAsc();
	void SortAsc(uint StartAt, uint Count);
	void SortDesc();
	void SortDesc(uint StartAt, uint Count);
	void Reverse();
	int Find(const T&in Value) const;
	int Find(uint StartAt, const T&in Value) const;
	int FindByRef(const T&in Value) const;
	int FindByRef(uint StartAt, const T&in Value) const;
	bool opEquals(const T[]&in) const;
	bool IsEmpty() const;
	void Sort(Array::Less&in, uint StartAt = 0, uint Count = uint ( - 1 ));
}

class WeakRef<class T>
{
	T@ opImplCast();
	T@ Get() const;
	WeakRef<T>& opHndlAssign(const WeakRef<T>&in);
	WeakRef<T>& opAssign(const WeakRef<T>&in);
	bool opEquals(const WeakRef<T>&in) const;
	WeakRef<T>& opHndlAssign(T@);
	bool opEquals(const T@) const;
}

class Thread
{
	Thread@ Thread(ThreadEvent@);
	bool IsActive();
	bool Invoke();
	void Suspend();
	void Resume();
	void Push(const ?&in);
	bool Pop(?&out);
	bool Pop(?&out, uint64);
	int Join(uint64);
	int Join();
}

class Map
{
	Map@ Map();
	Map& opAssign(const Map&in);
	void Set(const String&in, const ?&in);
	bool Get(const String&in, ?&out) const;
	bool Exists(const String&in) const;
	bool IsEmpty() const;
	uint GetSize() const;
	bool Delete(const String&in);
	void DeleteAll();
	String[]@ GetKeys() const;
	MapKey& opIndex(const String&in);
	const MapKey& opIndex(const String&in) const;
}

class ConstWeakRef<class T>
{
	const T@ opImplCast() const;
	const T@ Get() const;
	ConstWeakRef<T>& opHndlAssign(const ConstWeakRef<T>&in);
	ConstWeakRef<T>& opAssign(const ConstWeakRef<T>&in);
	bool opEquals(const ConstWeakRef<T>&in) const;
	ConstWeakRef<T>& opHndlAssign(const T@);
	bool opEquals(const T@) const;
	ConstWeakRef<T>& opHndlAssign(const WeakRef<T>&in);
	bool opEquals(const WeakRef<T>&in) const;
}

class Ref
{
	void opCast(?&out);
	Ref& opHndlAssign(const Ref&in);
	Ref& opHndlAssign(const ?&in);
	bool opEquals(const Ref&in) const;
	bool opEquals(const ?&in) const;
}

class Promise<class T>
{
	bool To(?&out);
	T& Get();
}

class Complex
{
	float R;
	float I;

	Complex& opAddAssign(const Complex&in);
	Complex& opSubAssign(const Complex&in);
	Complex& opMulAssign(const Complex&in);
	Complex& opDivAssign(const Complex&in);
	bool opEquals(const Complex&in) const;
	Complex opAdd(const Complex&in) const;
	Complex opSub(const Complex&in) const;
	Complex opMul(const Complex&in) const;
	Complex opDiv(const Complex&in) const;
	float Abs() const;
	Complex get_RI() const;
	Complex get_IR() const;
	void set_RI(const Complex&in);
	void set_IR(const Complex&in);
}

class Grid<class T>
{
	Grid<T>@ Grid(int&in);
	Grid<T>@ Grid(int&in, uint, uint);
	Grid<T>@ Grid(int&in, uint, uint, const T&in);
	T& opIndex(uint, uint);
	const T& opIndex(uint, uint) const;
	void Resize(uint, uint);
	uint Width() const;
	uint Height() const;
}

class Random
{
	Random@ Random();
	void opAssign(const Random&inout);
	void Seed(uint);
	void Seed(uint[]&inout);
	int GetI();
	uint GetU();
	double GetD();
	void SeedFromTime();
}

class String
{
	String& opAssign(const String&in);
	String& opAddAssign(const String&in);
	bool opEquals(const String&in) const;
	int opCmp(const String&in) const;
	String opAdd(const String&in) const;
	uint Size() const;
	void Resize(uint);
	bool IsEmpty() const;
	uint8& opIndex(uint);
	const uint8& opIndex(uint) const;
	String& opAssign(double);
	String& opAddAssign(double);
	String opAdd(double) const;
	String opAdd_r(double) const;
	String& opAssign(float);
	String& opAddAssign(float);
	String opAdd(float) const;
	String opAdd_r(float) const;
	String& opAssign(int64);
	String& opAddAssign(int64);
	String opAdd(int64) const;
	String opAdd_r(int64) const;
	String& opAssign(uint64);
	String& opAddAssign(uint64);
	String opAdd(uint64) const;
	String opAdd_r(uint64) const;
	String& opAssign(bool);
	String& opAddAssign(bool);
	String opAdd(bool) const;
	String opAdd_r(bool) const;
	String Needle(uint Start = 0, int Count = - 1) const;
	int FindFirst(const String&in, uint Start = 0) const;
	int FindFirstOf(const String&in, uint Start = 0) const;
	int FindFirstNotOf(const String&in, uint Start = 0) const;
	int FindLast(const String&in, int Start = - 1) const;
	int FindLastOf(const String&in, int Start = - 1) const;
	int FindLastNotOf(const String&in, int Start = - 1) const;
	void Insert(uint Offset, const String&in Other);
	void Erase(uint Offset, int Count = - 1);
	String Replace(const String&in, const String&in, uint64 Other = 0);
	String[]@ Split(const String&in) const;
	String ToLower() const;
	String ToUpper() const;
	String Reverse() const;
	Address@ GetAddress() const;
	Address@ opImplCast();
	Address@ opImplConv() const;
	Address@ opImplCast() const;
}

class MapKey
{
	MapKey& opAssign(const MapKey&in);
	MapKey& opHndlAssign(const ?&in);
	MapKey& opHndlAssign(const MapKey&in);
	MapKey& opAssign(const ?&in);
	MapKey& opAssign(double);
	MapKey& opAssign(int64);
	void opCast(?&out);
	void opConv(?&out);
	int64 opConv();
	double opConv();
}

class Mutex
{
	Mutex@ Mutex();
	bool TryLock();
	void Lock();
	void Unlock();
}

funcdef void ThreadEvent(Thread@)

Nullable@ get_nullptr();
float FpFromIEEE(uint);
uint FpToIEEE(float);
double FpFromIEEE(uint64);
uint64 FpToIEEE(double);
bool CloseTo(float, float, float = 0.00001f);
bool CloseTo(double, double, double = 0.0000000001);
float Cos(float);
float Sin(float);
float Tan(float);
float Acos(float);
float Asin(float);
float Atan(float);
float Atan2(float, float);
float Cosh(float);
float Sinh(float);
float Tanh(float);
float Log(float);
float Log10(float);
float Pow(float, float);
float Sqrt(float);
float Ceil(float);
float Abs(float);
float Floor(float);
float Fraction(float);
int64 ToInt(const String&in, uint Base = 10, uint&out ByteCount = 0);
uint64 ToUInt(const String&in, uint Base = 10, uint&out ByteCount = 0);
double ToFloat(const String&in, uint&out ByteCount = 0);
uint8 ToChar(const String&in);
String ToString(const String[]&in, const String&in);
String ToString(int8);
String ToString(int16);
String ToString(int);
String ToString(int64);
String ToString(uint8);
String ToString(uint16);
String ToString(uint);
String ToString(uint64);
String ToString(float);
String ToString(double);
void Throw(const String&in = "");
String GetException();
Thread@ GetThread();
uint64 GetThreadId();
void Sleep(uint64);
