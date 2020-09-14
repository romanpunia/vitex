#ifndef THAWK_ENGINE_GUI_LOGICAL_H
#define THAWK_ENGINE_GUI_LOGICAL_H

#include "context.h"

namespace Tomahawk
{
	namespace Engine
	{
		namespace GUI
		{
			enum SetType
			{
				SetType_Assign,
				SetType_Invert,
				SetType_Append,
				SetType_Increase,
				SetType_Decrease,
				SetType_Divide,
				SetType_Multiply
			};

			enum IfType
			{
				IfType_Equals,
				IfType_NotEquals,
				IfType_Greater,
				IfType_GreaterEquals,
				IfType_Less,
				IfType_LessEquals
			};

			class THAWK_OUT Divide : public Element
			{
			public:
				Divide(Context* View);
				virtual ~Divide() = default;
				virtual bool BuildBegin(nk_context* C) override;
				virtual void BuildEnd(nk_context* C, bool Stated) override;
				virtual void BuildStyle(nk_context* C, nk_style* S) override;
				virtual float GetWidth() override;
				virtual float GetHeight() override;
				virtual std::string GetType() override;
			};

			class THAWK_OUT Restyle : public Element
			{
			private:
				struct
				{
					Element* Target;
					std::string Old;
				} State;

			public:
				struct
				{
					std::string Target;
					std::string Rule;
					std::string Set;
				} Source;

			public:
				Restyle(Context* View);
				virtual ~Restyle() = default;
				virtual bool BuildBegin(nk_context* C) override;
				virtual void BuildEnd(nk_context* C, bool Stated) override;
				virtual void BuildStyle(nk_context* C, nk_style* S) override;
				virtual float GetWidth() override;
				virtual float GetHeight() override;
				virtual std::string GetType() override;
			};

			class THAWK_OUT For : public Element
			{
			public:
				struct
				{
					std::string It;
					uint64_t Range;
				} Source;

			public:
				For(Context* View);
				virtual ~For() = default;
				virtual bool BuildBegin(nk_context* C) override;
				virtual void BuildEnd(nk_context* C, bool Stated) override;
				virtual void BuildStyle(nk_context* C, nk_style* S) override;
				virtual float GetWidth() override;
				virtual float GetHeight() override;
				virtual std::string GetType() override;
			};

			class THAWK_OUT Const : public Element
			{
			private:
				struct
				{
					bool Init;
				} State;

			public:
				struct
				{
					std::string Name;
					std::string Value;
				} Source;

			public:
				Const(Context* View);
				virtual ~Const() = default;
				virtual bool BuildBegin(nk_context* C) override;
				virtual void BuildEnd(nk_context* C, bool Stated) override;
				virtual void BuildStyle(nk_context* C, nk_style* S) override;
				virtual float GetWidth() override;
				virtual float GetHeight() override;
				virtual std::string GetType() override;
			};

			class THAWK_OUT Set : public Element
			{
			public:
				struct
				{
					std::string Name;
					std::string Value;
					SetType Type;
				} Source;

			public:
				Set(Context* View);
				virtual ~Set() = default;
				virtual bool BuildBegin(nk_context* C) override;
				virtual void BuildEnd(nk_context* C, bool Stated) override;
				virtual void BuildStyle(nk_context* C, nk_style* S) override;
				virtual float GetWidth() override;
				virtual float GetHeight() override;
				virtual std::string GetType() override;
			};

			class THAWK_OUT LogIf : public Element
			{
			protected:
				struct
				{
					bool Satisfied;
				} State;

			public:
				struct
				{
					std::string Value1;
					std::string Value2;
					IfType Type;
				} Source;

			public:
				LogIf(Context* View);
				virtual ~LogIf() override;
				virtual bool BuildBegin(nk_context* C) override;
				virtual void BuildEnd(nk_context* C, bool Stated) override;
				virtual void BuildStyle(nk_context* C, nk_style* S) override;
				virtual float GetWidth() override;
				virtual float GetHeight() override;
				virtual std::string GetType() override;
				virtual bool IsSatisfied();
			};

			class THAWK_OUT LogElseIf : public LogIf
			{
			private:
				struct
				{
					bool Satisfied;
					LogIf* Layer;
				} Cond;

			public:
				LogElseIf(Context* View);
				virtual ~LogElseIf() override;
				virtual bool BuildBegin(nk_context* C) override;
				virtual std::string GetType() override;
				virtual bool IsSatisfied() override;
				void SetLayer(LogIf* New);
			};

			class THAWK_OUT LogElse : public Element
			{
			private:
				struct
				{
					bool Satisfied;
					LogIf* Layer;
				} State;

			public:
				LogElse(Context* View);
				virtual ~LogElse() = default;
				virtual bool BuildBegin(nk_context* C) override;
				virtual void BuildEnd(nk_context* C, bool Stated) override;
				virtual void BuildStyle(nk_context* C, nk_style* S) override;
				virtual float GetWidth() override;
				virtual float GetHeight() override;
				virtual std::string GetType() override;
				void SetLayer(LogIf* New);
				bool IsSatisfied();
			};

			class THAWK_OUT Escape : public Element
			{
			public:
				Escape(Context* View);
				virtual ~Escape() = default;
				virtual bool BuildBegin(nk_context* C) override;
				virtual void BuildEnd(nk_context* C, bool Stated) override;
				virtual void BuildStyle(nk_context* C, nk_style* S) override;
				virtual float GetWidth() override;
				virtual float GetHeight() override;
				virtual std::string GetType() override;
			};

			class THAWK_OUT Template : public Element
			{
			private:
				struct
				{
					std::unordered_map<std::string, Element*> Cache;
					std::unordered_set<Stateful*> States;
				} State;

			public:
				Template(Context* View);
				virtual ~Template() override;
				virtual bool BuildBegin(nk_context* C) override;
				virtual void BuildEnd(nk_context* C, bool Stated) override;
				virtual void BuildStyle(nk_context* C, nk_style* S) override;
				virtual float GetWidth() override;
				virtual float GetHeight() override;
				virtual std::string GetType() override;
				bool Compose(Element* Base, const std::function<bool(bool)>& Callback);
				bool Compose(const std::string& Name, const std::function<bool(bool)>& Callback);
				bool ComposeStateful(Stateful* Base, const std::function<void()>& Setup, const std::function<bool(bool)>& Callback);

			public:
				template <typename T>
				T* Get(const std::string& Name)
				{
					if (Name.empty())
						return nullptr;

					auto It = State.Cache.find(Name);
					if (It != State.Cache.end())
						return (T*)It->second;

					Element* Result = (Name[0] == '#' ? GetNodeById(Name.substr(1)) : GetNodeByPath(Name));
					State.Cache[Name] = Result;

					if (!Result)
						THAWK_WARN("template has no needed composer elements");

					return (T*)Result;
				}
			};
		}
	}
}
#endif