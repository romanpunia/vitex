namespace CE
{
	enum FileMode
	{
		ReadOnly = 0,
		WriteOnly = 1,
		AppendOnly = 2,
		ReadWrite = 3,
		WriteRead = 4,
		ReadAppendWrite = 5,
		BinaryReadOnly = 6,
		BinaryWriteOnly = 7,
		BinaryAppendOnly = 8,
		BinaryRead_Write = 9,
		BinaryWriteRead = 10,
		BinaryReadAppendWrite = 11
	}

	enum FileSeek
	{
		Begin = 0,
		Current = 1,
		End = 2
	}

	enum VarType
	{
		Null = 0,
		Undefined = 1,
		Object = 2,
		Array = 3,
		String = 5,
		Base64 = 6,
		Integer = 7,
		Number = 8,
		Decimal = 9,
		Boolean = 10
	}

	enum StdColor
	{
		Black = 0,
		DarkBlue = 1,
		DarkGreen = 2,
		LightBlue = 3,
		DarkRed = 4,
		Magenta = 5,
		Orange = 6,
		LightGray = 7,
		Gray = 8,
		Blue = 9,
		Green = 10,
		Cyan = 11,
		Red = 12,
		Pink = 13,
		Yellow = 14,
		White = 15
	}

	class FileStream
	{
		CE::FileStream@ FileStream();
		void Clear();
		bool Open(Address@, CE::FileMode);
		bool Close();
		bool Seek(CE::FileSeek, int64);
		bool Move(int64);
		uint64 Read(uint64, String&out);
		uint64 Write(Address@, uint64);
		uint64 Tell();
		int Flush();
		int GetFd();
		uint64 GetSize();
		String GetSource();
	}

	class Schedule
	{
		funcdef void TaskCallback();

		uint64 SetTimeout(uint64, Schedule::TaskCallback@);
		bool SetTask(Schedule::TaskCallback@);
		bool ClearTimeout(uint64);
		bool Start(bool, uint64, uint64 = 16, uint64 = 524288);
		bool Stop();
		bool Dispatch();
		bool IsBlockable();
		bool IsActive();
		bool IsProcessing();
	}

	class GzStream
	{
		CE::GzStream@ GzStream();
		void Clear();
		bool Open(Address@, CE::FileMode);
		bool Close();
		bool Seek(CE::FileSeek, int64);
		bool Move(int64);
		uint64 Read(uint64, String&out);
		uint64 Write(Address@, uint64);
		uint64 Tell();
		int Flush();
		int GetFd();
		uint64 GetSize();
		String GetSource();
	}

	class Decimal
	{
		CE::Decimal& opAssign(const CE::Decimal&in);
		CE::Decimal& Precise(int);
		CE::Decimal& Trim();
		CE::Decimal& Unlead();
		CE::Decimal& Untrail();
		bool IsNaN();
		double ToDouble();
		int64 ToInt64();
		String ToString();
		String Exp();
		String Decimals();
		String Ints();
		String Size();
		CE::Decimal opNeg() const;
		CE::Decimal& opMulAssign(const CE::Decimal&in);
		CE::Decimal& opDivAssign(const CE::Decimal&in);
		CE::Decimal& opAddAssign(const CE::Decimal&in);
		CE::Decimal& opSubAssign(const CE::Decimal&in);
		CE::Decimal& opPostInc();
		CE::Decimal& opPostDec();
		bool opEquals(const CE::Decimal&in) const;
		int opCmp(const CE::Decimal&in) const;
		CE::Decimal opAdd(const CE::Decimal&in) const;
		CE::Decimal opSub(const CE::Decimal&in) const;
		CE::Decimal opMul(const CE::Decimal&in) const;
		CE::Decimal opDiv(const CE::Decimal&in) const;
		CE::Decimal opMod(const CE::Decimal&in) const;
	}

	namespace Decimal
	{
		CE::Decimal Size(const CE::Decimal&in, const CE::Decimal&in, int);
		CE::Decimal Size();
	}

	class FileState
	{
		uint64 Size;
		uint64 Links;
		uint64 Permissions;
		uint64 IDocument;
		uint64 Device;
		uint64 UserId;
		uint64 GroupId;
		int64 LastAccess;
		int64 LastPermissionChange;
		int64 LastModified;
		bool Exists;
	}

	class Resource
	{
		uint64 Key;
		int64 LastModified;
		int64 CreationTime;
		bool IsReferenced;
		bool IsDirectory;
	}

	class Timer
	{
		CE::Timer@ Timer();
		void SetStepLimitation(double, double);
		void Synchronize();
		void CaptureTime();
		void Sleep(uint64);
		double GetTimeIncrement() const;
		double GetTickCounter() const;
		double GetFrameCount() const;
		double GetElapsedTime() const;
		double GetCapturedTime() const;
		double GetDeltaTime() const;
		double GetTimeStep() const;
	}

	class Format
	{
		CE::Format@ Format();
	}

	namespace Format
	{
		String JSON(const ?&in);
	}

	class Variant
	{
		CE::Variant& opAssign(const CE::Variant&in);
		bool Deserialize(const String&in, bool = false);
		String Serialize() const;
		String Dec() const;
		String Str() const;
		Address@ Ptr() const;
		int64 Int() const;
		double Num() const;
		bool Bool() const;
		CE::VarType GetType() const;
		bool IsObject() const;
		bool IsEmpty() const;
		uint64 GetSize() const;
		bool opEquals(const CE::Variant&in) const;
		bool opImplCast() const;
	}

	class DateTime
	{
		CE::DateTime& opAssign(const CE::DateTime&in);
		String Format(const String&in);
		String Date(const String&in);
		String Iso8601();
		CE::DateTime Now();
		CE::DateTime FromNanoseconds(uint64);
		CE::DateTime FromMicroseconds(uint64);
		CE::DateTime FromMilliseconds(uint64);
		CE::DateTime FromSeconds(uint64);
		CE::DateTime FromMinutes(uint64);
		CE::DateTime FromHours(uint64);
		CE::DateTime FromDays(uint64);
		CE::DateTime FromWeeks(uint64);
		CE::DateTime FromMonths(uint64);
		CE::DateTime FromYears(uint64);
		CE::DateTime& SetDateSeconds(uint64, bool = true);
		CE::DateTime& SetDateMinutes(uint64, bool = true);
		CE::DateTime& SetDateHours(uint64, bool = true);
		CE::DateTime& SetDateDay(uint64, bool = true);
		CE::DateTime& SetDateWeek(uint64, bool = true);
		CE::DateTime& SetDateMonth(uint64, bool = true);
		CE::DateTime& SetDateYear(uint64, bool = true);
		uint64 DateSecond();
		uint64 DateMinute();
		uint64 DateHour();
		uint64 DateDay();
		uint64 DateWeek();
		uint64 DateMonth();
		uint64 DateYear();
		uint64 Nanoseconds();
		uint64 Microseconds();
		uint64 Milliseconds();
		uint64 Seconds();
		uint64 Minutes();
		uint64 Hours();
		uint64 Days();
		uint64 Weeks();
		uint64 Months();
		uint64 Years();
		CE::DateTime& opAddAssign(const CE::DateTime&in);
		CE::DateTime& opSubAssign(const CE::DateTime&in);
		bool opEquals(const CE::DateTime&in) const;
		int opCmp(const CE::DateTime&in) const;
		CE::DateTime opAdd(const CE::DateTime&in) const;
		CE::DateTime opSub(const CE::DateTime&in) const;
	}

	class Ticker
	{
		CE::Ticker& opAssign(const CE::Ticker&in);
		bool TickEvent(double);
		double GetTime();
	}

	class Console
	{
		void Hide();
		void Show();
		void Clear();
		void Flush();
		void FlushWrite();
		void SetColoring(bool);
		void ColorBegin(CE::StdColor, CE::StdColor);
		void ColorEnd();
		void CaptureTime();
		void WriteLine(const String&in);
		void Write(const String&in);
		double GetCapturedTime();
		String Read(uint64);
		void Trace(uint = 32);
		void WriteLine(const String&in, CE::Format@);
		void Write(const String&in, CE::Format@);
	}

	namespace Console
	{
		CE::Console@ Get();
	}

	class WebStream
	{
		CE::WebStream@ WebStream();
		void Clear();
		bool Open(Address@, CE::FileMode);
		bool Close();
		bool Seek(CE::FileSeek, int64);
		bool Move(int64);
		uint64 Read(uint64, String&out);
		uint64 Write(Address@, uint64);
		uint64 Tell();
		int Flush();
		int GetFd();
		uint64 GetSize();
		String GetSource();
	}

	class Document
	{
		String Key;
		CE::Variant Value;

		CE::Document@ Document(const CE::Variant&in);
		CE::Variant GetVar(uint) const;
		CE::Variant GetVar(const String&in) const;
		CE::Document@ GetParent() const;
		CE::Document@ GetAttribute(const String&in) const;
		CE::Document@ Get(uint) const;
		CE::Document@ Get(const String&in, bool = false) const;
		CE::Document@ Set(const String&in);
		CE::Document@ Set(const String&in, const CE::Variant&in);
		CE::Document@ SetAttribute(const String&in, const CE::Variant&in);
		CE::Document@ Push(const CE::Variant&in);
		CE::Document@ Pop(uint);
		CE::Document@ Pop(const String&in);
		CE::Document@ Copy() const;
		bool Has(const String&in) const;
		bool Has64(const String&in, uint = 12) const;
		bool IsEmpty() const;
		bool IsAttribute() const;
		bool IsSaved() const;
		String GetName() const;
		void Join(CE::Document@);
		void Clear();
		void Save();
		CE::Document@ Set(const String&in, CE::Document@);
		CE::Document@ Push(CE::Document@);
		CE::Document@[]@ GetCollection(const String&in, bool = false) const;
		CE::Document@[]@ GetAttributes() const;
		CE::Document@[]@ GetChilds() const;
		Map@ GetNames() const;
		uint64 Size() const;
		String JSON() const;
		String XML() const;
		String Str() const;
		String B64() const;
		int64 Int() const;
		double Num() const;
		String Dec() const;
		bool Bool() const;
		CE::Document@ opAssign(const CE::Variant&in);
		bool opEquals(CE::Document@) const;
		CE::Document@ opIndex(const String&in);
		CE::Document@ opIndex(uint64);
	}

	namespace Document
	{
		CE::Document@ FromJSON(const String&in);
		CE::Document@ FromXML(const String&in);
		CE::Document@ Import(const String&in);
	}
}

namespace CE::Var
{
	CE::Variant Auto(const String&in, bool = false);
	CE::Variant Null();
	CE::Variant Undefined();
	CE::Variant Object();
	CE::Variant Array();
	CE::Variant Pointer(Address@);
	CE::Variant Integer(int64);
	CE::Variant Number(double);
	CE::Variant Boolean(bool);
	CE::Variant String(const String&in);
	CE::Variant Base64(const String&in);
	CE::Variant Decimal(const String&in);
	CE::Document@ Init(CE::Document@);
}

namespace CE::OS::File
{
	bool Write(Address@, const String&in);
	bool State(const String&in, CE::Resource&out);
	bool Move(Address@, Address@);
	bool Remove(Address@);
	bool IsExists(Address@);
	int Compare(const String&in, const String&in);
	uint64 GetCheckSum(const String&in);
	CE::FileState GetState(Address@);
	String ReadAsString(Address@);
}

namespace CE::OS::Path
{
	String Resolve(Address@);
	String Resolve(const String&in, const String&in);
	String ResolveDirectory(Address@);
	String ResolveDirectory(const String&in, const String&in);
	String ResolveResource(Address@);
	String ResolveResource(const String&in, const String&in);
	String GetDirectory(Address@, uint = 0);
}

namespace CE::Var::Set
{
	CE::Document@ Auto(const String&in, bool = false);
	CE::Document@ Null();
	CE::Document@ Undefined();
	CE::Document@ Object();
	CE::Document@ Array();
	CE::Document@ Pointer(Address@);
	CE::Document@ Integer(int64);
	CE::Document@ Number(double);
	CE::Document@ Boolean(bool);
	CE::Document@ String(const String&in);
	CE::Document@ Base64(const String&in);
	CE::Document@ Decimal(const String&in);
}

namespace CE::OS::Input
{
	bool Text(const String&in, const String&in, const String&in, String&out);
	bool Password(const String&in, const String&in, String&out);
	bool Save(const String&in, const String&in, const String&in, const String&in, String&out);
	bool Open(const String&in, const String&in, const String&in, const String&in, bool, String&out);
	bool Folder(const String&in, const String&in, String&out);
	bool Color(const String&in, const String&in, String&out);
}

namespace CE::OS::Error
{
	int Get();
	String GetName(int);
}

namespace CE::OS::Symbol
{
	Address@ Load(const String&in = "");
	Address@ LoadFunction(Address@, const String&in);
	bool Unload(Address@);
}

namespace CE::OS::Process
{
	int Execute(const String&in);
}

namespace CE::OS::Directory
{
	void Set(Address@);
	void Patch(const String&in);
	bool Create(Address@);
	bool Remove(Address@);
	bool IsExists(Address@);
	String Get();
	String[]@ GetMounts();
	String[]@ Scan(const String&in);
}