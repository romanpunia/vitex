#ifndef SCRIPT_DICTIONARY_H
#define SCRIPT_DICTIONARY_H
#include <unordered_map>
#include <string>
#ifndef ANGELSCRIPT_H 
#include <angelscript.h>
#endif
#ifndef AS_USE_STLNAMES
#define AS_USE_STLNAMES 0
#endif
#ifdef _MSC_VER
#pragma warning (disable:4786)
#endif

class VMCDictValue;
class VMCArray;
class VMCDictionary;

typedef std::string dictKey_t;
typedef std::unordered_map<dictKey_t, VMCDictValue> dictMap_t;

class VMCDictValue
{
public:
	VMCDictValue();
	VMCDictValue(asIScriptEngine *engine, void *value, int typeId);
	~VMCDictValue();
	void Set(asIScriptEngine *engine, void *value, int typeId);
	void Set(asIScriptEngine *engine, const asINT64 &value);
	void Set(asIScriptEngine *engine, const double &value);
	void Set(asIScriptEngine *engine, VMCDictValue &value);
	bool Get(asIScriptEngine *engine, void *value, int typeId) const;
	bool Get(asIScriptEngine *engine, asINT64 &value) const;
	bool Get(asIScriptEngine *engine, double &value) const;
	const void *GetAddressOfValue() const;
	int  GetTypeId() const;
	void FreeValue(asIScriptEngine *engine);
	void EnumReferences(asIScriptEngine *engine);

protected:
	friend class VMCDictionary;

	union
	{
		asINT64 m_valueInt;
		double m_valueFlt;
		void* m_valueObj;
	};
	int m_typeId;
};

class VMCDictionary
{
public:
	static VMCDictionary *Create(asIScriptEngine *engine);
	static VMCDictionary *Create(asBYTE *buffer);
	void AddRef() const;
	void Release() const;
	VMCDictionary &operator =(const VMCDictionary &other);
	void Set(const dictKey_t &key, void *value, int typeId);
	void Set(const dictKey_t &key, const asINT64 &value);
	void Set(const dictKey_t &key, const double &value);
	bool Get(const dictKey_t &key, void *value, int typeId) const;
	bool Get(const dictKey_t &key, asINT64 &value) const;
	bool Get(const dictKey_t &key, double &value) const;
	VMCDictValue *operator[](const dictKey_t &key);
	const VMCDictValue *operator[](const dictKey_t &key) const;
	int GetTypeId(const dictKey_t &key) const;
	bool Exists(const dictKey_t &key) const;
	bool IsEmpty() const;
	asUINT GetSize() const;
	bool Delete(const dictKey_t &key);
	void DeleteAll();
	VMCArray *GetKeys() const;

	class CIterator
	{
	public:
		void operator++();
		void operator++(int);
		CIterator &operator*();
		bool operator==(const CIterator &other) const;
		bool operator!=(const CIterator &other) const;
		const dictKey_t &GetKey() const;
		int GetTypeId() const;
		bool GetValue(asINT64 &value) const;
		bool GetValue(double &value) const;
		bool GetValue(void *value, int typeId) const;
		const void *GetAddressOfValue() const;

	protected:
		friend class VMCDictionary;
		CIterator();
		CIterator(const VMCDictionary &dict, dictMap_t::const_iterator it);
		CIterator &operator=(const CIterator &)
		{
			return *this;
		}

		dictMap_t::const_iterator m_it;
		const VMCDictionary &m_dict;
	};

	CIterator begin() const;
	CIterator end() const;
	CIterator find(const dictKey_t &key) const;
	int GetRefCount();
	void SetGCFlag();
	bool GetGCFlag();
	void EnumReferences(asIScriptEngine *engine);
	void ReleaseAllReferences(asIScriptEngine *engine);

protected:
	VMCDictionary(asIScriptEngine *engine);
	VMCDictionary(asBYTE *buffer);
	virtual ~VMCDictionary();
	void Init(asIScriptEngine *engine);
	asIScriptEngine* engine;
	mutable int refCount;
	mutable bool gcFlag;
	dictMap_t dict;
};

void VM_RegisterDictionary(asIScriptEngine *engine);
#endif