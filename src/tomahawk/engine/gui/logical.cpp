#include "logical.h"
#ifndef NK_IMPLEMENTATION
#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_BUTTON_TRIGGER_ON_RELEASE
#include <nuklear.h>
#endif
#define nk_image_auto(x) x ? nk_image_ptr(x) : nk_image_id(0)

namespace Tomahawk
{
	namespace Engine
	{
		namespace GUI
		{
			extern struct nk_vec2 CastV2(const Compute::Vector2& Value);
			extern nk_color CastV4(const Compute::Vector4& Value);
			extern nk_style_item CastV4Item(ContentManager* Manager, const std::string& Value);
			extern nk_flags CastTextAlign(const std::string& Value);
			extern nk_style_header_align CastHeaderAlign(const std::string& Value);
			extern nk_symbol_type CastSymbol(const std::string& Value);

			Divide::Divide(Context* View) : Element(View)
			{
			}
			bool Divide::BuildBegin(nk_context* C)
			{
				return true;
			}
			void Divide::BuildEnd(nk_context* C, bool Stated)
			{
			}
			void Divide::BuildStyle(nk_context* C, nk_style* S)
			{
			}
			float Divide::GetWidth()
			{
				return 0;
			}
			float Divide::GetHeight()
			{
				return 0;
			}
			std::string Divide::GetType()
			{
				return "divide";
			}

			Restyle::Restyle(Context* View) : Element(View)
			{
				State.Target = nullptr;

				Bind("target", [this]()
				{
					Source.Target = GetString("target", "");
					if (!Source.Target.empty())
					{
						if (Source.Target[0] == '#')
							State.Target = Base->GetLayout()->GetNodeById(Source.Target.substr(1));
						else
							State.Target = Base->GetLayout()->GetNodeByPath(Source.Target);
					}

					if (State.Target != nullptr)
						State.Old = (State.Target ? State.Target->GetString(Source.Rule, "__none__") : "");
				});
				Bind("rule", [this]()
				{
					Source.Rule = GetString("rule", "");
					if (!Source.Target.empty())
					{
						if (Source.Target[0] == '#')
							State.Target = Base->GetLayout()->GetNodeById(Source.Target.substr(1));
						else
							State.Target = Base->GetLayout()->GetNodeByPath(Source.Target);
					}

					if (State.Target != nullptr)
						State.Old = (State.Target ? State.Target->GetString(Source.Rule, "__none__") : "");
				});
				Bind("set", [this]() { Source.Set = GetString("set", ""); });
			}
			bool Restyle::BuildBegin(nk_context* C)
			{
				return true;
			}
			void Restyle::BuildEnd(nk_context* C, bool Stated)
			{
			}
			void Restyle::BuildStyle(nk_context* C, nk_style* S)
			{
				if (!State.Target || Source.Rule.empty())
					return;

				if (S != nullptr)
				{
					if (State.Old == "__none__")
						State.Target->SetLocal(Source.Rule, State.Old, true);
					else
						State.Target->SetLocal(Source.Rule, State.Old);
				}
				else
					State.Target->SetLocal(Source.Rule, Source.Set);

				if (State.Target != nullptr)
					State.Target->BuildStyle(C, nullptr);
			}
			float Restyle::GetWidth()
			{
				return 0;
			}
			float Restyle::GetHeight()
			{
				return 0;
			}
			std::string Restyle::GetType()
			{
				return "restyle";
			}

			For::For(Context* View) : Element(View)
			{
				Bind("range", [this]() { Source.Range = GetFloat("range", 1); });
				Bind("it", [this]() { Source.It = GetString("it", ""); });
			}
			bool For::BuildBegin(nk_context* C)
			{
				return false;
			}
			void For::BuildEnd(nk_context* C, bool Stated)
			{
				for (uint64_t i = 0; i < Source.Range; i++)
				{
					if (!Source.It.empty())
						SetGlobal(Source.It, std::to_string(i));

					for (auto& Node : Nodes)
						Node->Build();
				}
			}
			void For::BuildStyle(nk_context* C, nk_style* S)
			{
			}
			float For::GetWidth()
			{
				return 0;
			}
			float For::GetHeight()
			{
				return 0;
			}
			std::string For::GetType()
			{
				return "for";
			}

			Const::Const(Context* View) : Element(View)
			{
				State.Init = false;

				Bind("name", [this]() { Source.Name = GetString("name", "const"); });
				Bind("value", [this]() { Source.Value = GetString("value", ""); });
			}
			bool Const::BuildBegin(nk_context* C)
			{
				if (!State.Init)
				{
					State.Init = true;
					SetGlobal(Source.Name, Source.Value);
				}

				return !State.Init;
			}
			void Const::BuildEnd(nk_context* C, bool Stated)
			{
			}
			void Const::BuildStyle(nk_context* C, nk_style* S)
			{
			}
			float Const::GetWidth()
			{
				return 0;
			}
			float Const::GetHeight()
			{
				return 0;
			}
			std::string Const::GetType()
			{
				return "const";
			}

			Set::Set(Context* View) : Element(View)
			{
				Bind("op", [this]()
				{
					std::string Type = GetString("op", "eq");
					if (Type == "eq")
						Source.Type = SetType_Assign;
					else if (Type == "inv")
						Source.Type = SetType_Invert;
					else if (Type == "add")
						Source.Type = SetType_Append;
					else if (Type == "inc")
						Source.Type = SetType_Increase;
					else if (Type == "dec")
						Source.Type = SetType_Decrease;
					else if (Type == "div")
						Source.Type = SetType_Divide;
					else if (Type == "mul")
						Source.Type = SetType_Multiply;
				});
				Bind("name", [this]() { Source.Name = GetString("name", "set"); });
				Bind("value", [this]() { Source.Value = GetString("value", ""); });
			}
			bool Set::BuildBegin(nk_context* C)
			{
				if (Source.Type == SetType_Assign)
				{
					SetGlobal(Source.Name, Source.Value);
					return true;
				}

				std::string* Global = GetGlobal(Source.Name);
				if (!Global)
					return false;

				if (Source.Type == SetType_Append)
				{
					SetGlobal(Source.Name, *Global + Source.Value);
					return true;
				}
				else if (Source.Type == SetType_Invert)
				{
					if (*Global == "true")
						SetGlobal(Source.Name, "false");
					else if (*Global == "1")
						SetGlobal(Source.Name, "0");
					else if (*Global == "false")
						SetGlobal(Source.Name, "true");
					else if (*Global == "0")
						SetGlobal(Source.Name, "1");
					else
						SetGlobal(Source.Name, Rest::Stroke(*Global).Reverse().R());

					return true;
				}

				Rest::Stroke N1(Global);
				double X1 = (N1.HasNumber() ? N1.ToFloat64() : 0.0);

				Rest::Stroke N2(&Source.Value);
				double X2 = (N2.HasNumber() ? N2.ToFloat64() : 0.0);

				std::string Result;
				switch (Source.Type)
				{
					case SetType_Increase:
						Result = std::to_string(X1 + X2);
						break;
					case SetType_Decrease:
						Result = std::to_string(X1 - X2);
						break;
					case SetType_Divide:
						Result = std::to_string(X1 / (X2 == 0.0 ? 1.0 : X2));
						break;
					case SetType_Multiply:
						Result = std::to_string(X1 * X2);
						break;
					default:
						break;
				}

				Result.erase(Result.find_last_not_of('0') + 1, std::string::npos);
				if (!Result.empty() && Result.back() == '.')
					Result.erase(Result.size() - 1, 1);

				SetGlobal(Source.Name, Result);
				return true;
			}
			void Set::BuildEnd(nk_context* C, bool Stated)
			{
			}
			void Set::BuildStyle(nk_context* C, nk_style* S)
			{
			}
			float Set::GetWidth()
			{
				return 0;
			}
			float Set::GetHeight()
			{
				return 0;
			}
			std::string Set::GetType()
			{
				return "set";
			}

			LogIf::LogIf(Context* View) : Element(View)
			{
				Bind("op", [this]()
				{
					std::string Type = GetString("op", "eq");
					if (Type == "eq")
						Source.Type = IfType_Equals;
					else if (Type == "neq")
						Source.Type = IfType_NotEquals;
					else if (Type == "gt")
						Source.Type = IfType_Greater;
					else if (Type == "gte")
						Source.Type = IfType_GreaterEquals;
					else if (Type == "lt")
						Source.Type = IfType_Less;
					else if (Type == "lte")
						Source.Type = IfType_LessEquals;
				});
				Bind("v1", [this]()
				{
					Source.Value1 = GetString("v1", "");
				});
				Bind("v2", [this]() { Source.Value2 = GetString("v2", ""); });
			}
			LogIf::~LogIf()
			{
			}
			bool LogIf::BuildBegin(nk_context* C)
			{
				if (Source.Type == IfType_Equals)
				{
					State.Satisfied = (Source.Value1 == Source.Value2);
					return State.Satisfied;
				}

				if (Source.Type == IfType_NotEquals)
				{
					State.Satisfied = (Source.Value1 != Source.Value2);
					return State.Satisfied;
				}

				Rest::Stroke N1(&Source.Value1);
				double X1 = (N1.HasNumber() ? N1.ToFloat64() : (double)N1.Size());

				Rest::Stroke N2(&Source.Value2);
				double X2 = (N2.HasNumber() ? N2.ToFloat64() : (double)N2.Size());

				bool Result = false;
				switch (Source.Type)
				{
					case IfType_Greater:
						Result = (X1 > X2);
						break;
					case IfType_GreaterEquals:
						Result = (X1 >= X2);
						break;
					case IfType_Less:
						Result = (X1 < X2);
						break;
					case IfType_LessEquals:
						Result = (X1 <= X2);
						break;
					default:
						break;
				}

				State.Satisfied = Result;
				return Result;
			}
			void LogIf::BuildEnd(nk_context* C, bool Stated)
			{
			}
			void LogIf::BuildStyle(nk_context* C, nk_style* S)
			{
			}
			float LogIf::GetWidth()
			{
				return 0;
			}
			float LogIf::GetHeight()
			{
				return 0;
			}
			std::string LogIf::GetType()
			{
				return "if";
			}
			bool LogIf::IsSatisfied()
			{
				return State.Satisfied;
			}

			LogElseIf::LogElseIf(Context* View) : LogIf(View)
			{
				Cond.Layer = nullptr;
			}
			LogElseIf::~LogElseIf()
			{
			}
			bool LogElseIf::BuildBegin(nk_context* C)
			{
				if (!Cond.Layer)
				{
					Cond.Layer = (LogIf*)GetUpperNode("if");
					if (!Cond.Layer)
					{
						Cond.Layer = (LogIf*)GetUpperNode("else-if");
						if (!Cond.Layer)
							return false;
					}
				}

				if (Cond.Layer->IsSatisfied())
				{
					State.Satisfied = false;
					Cond.Satisfied = true;
					return false;
				}

				if (Source.Type == IfType_Equals)
				{
					State.Satisfied = (Source.Value1 == Source.Value2);
					Cond.Satisfied = State.Satisfied;
					return State.Satisfied;
				}

				if (Source.Type == IfType_NotEquals)
				{
					State.Satisfied = (Source.Value1 != Source.Value2);
					Cond.Satisfied = State.Satisfied;
					return State.Satisfied;
				}

				Rest::Stroke N1(&Source.Value1);
				double X1 = (N1.HasNumber() ? N1.ToFloat64() : (double)N1.Size());

				Rest::Stroke N2(&Source.Value2);
				double X2 = (N2.HasNumber() ? N2.ToFloat64() : (double)N2.Size());

				bool Result = false;
				switch (Source.Type)
				{
					case IfType_Greater:
						Result = (X1 > X2);
						break;
					case IfType_GreaterEquals:
						Result = (X1 >= X2);
						break;
					case IfType_Less:
						Result = (X1 < X2);
						break;
					case IfType_LessEquals:
						Result = (X1 <= X2);
						break;
					default:
						break;
				}

				State.Satisfied = Result;
				Cond.Satisfied = Result;

				return Result;
			}
			std::string LogElseIf::GetType()
			{
				return "else-if";
			}
			void LogElseIf::SetLayer(LogIf* New)
			{
				Cond.Layer = New;
			}
			bool LogElseIf::IsSatisfied()
			{
				return State.Satisfied || Cond.Satisfied;
			}

			LogElse::LogElse(Context* View) : Element(View)
			{
				State.Layer = nullptr;
			}
			bool LogElse::BuildBegin(nk_context* C)
			{
				if (!State.Layer)
				{
					State.Layer = (LogIf*)GetUpperNode("if");
					if (!State.Layer)
					{
						State.Layer = (LogIf*)GetUpperNode("else-if");
						if (!State.Layer)
							return false;
					}
				}

				return !State.Layer->IsSatisfied();
			}
			void LogElse::BuildEnd(nk_context* C, bool Stated)
			{
			}
			void LogElse::BuildStyle(nk_context* C, nk_style* S)
			{
			}
			float LogElse::GetWidth()
			{
				return 0;
			}
			float LogElse::GetHeight()
			{
				return 0;
			}
			std::string LogElse::GetType()
			{
				return "else";
			}
			void LogElse::SetLayer(LogIf* New)
			{
				State.Layer = New;
			}

			Escape::Escape(Context* View) : Element(View)
			{
			}
			bool Escape::BuildBegin(nk_context* C)
			{
				Base->EmitEscape();
				return false;
			}
			void Escape::BuildEnd(nk_context* C, bool Stated)
			{
			}
			void Escape::BuildStyle(nk_context* C, nk_style* S)
			{
			}
			float Escape::GetWidth()
			{
				return 0;
			}
			float Escape::GetHeight()
			{
				return 0;
			}
			std::string Escape::GetType()
			{
				return "escape";
			}

			Template::Template(Context* View) : Element(View)
			{
			}
			Template::~Template()
			{
			}
			bool Template::BuildBegin(nk_context* C)
			{
				return false;
			}
			void Template::BuildEnd(nk_context* C, bool Stated)
			{
				for (auto& It : State.States)
					It->Pop();
				State.States.clear();
			}
			void Template::BuildStyle(nk_context* C, nk_style* S)
			{
			}
			float Template::GetWidth()
			{
				return 0;
			}
			float Template::GetHeight()
			{
				return 0;
			}
			std::string Template::GetType()
			{
				return "template";
			}
			bool Template::Compose(Element* Target, const std::function<bool(bool)>& Function)
			{
				if (!Target)
					return false;

				Target->BuildStyle(Base->GetContext(), nullptr);
				bool Continue = Target->BuildBegin(Base->GetContext());
				bool Stated = Continue;

				if (Function != nullptr)
					Continue = Function(Continue);

				if (Continue)
				{
					for (auto& It : Target->GetNodes())
						It->Build();
				}

				Target->BuildEnd(Base->GetContext(), Stated);
				Target->BuildStyle(Base->GetContext(), (nk_style*)Base->GetDefaultStyle());

				return Continue;
			}
			bool Template::Compose(const std::string& Name, const std::function<bool(bool)>& Function)
			{
				return Compose(Get<Element>(Name), Function);
			}
			bool Template::ComposeStateful(Stateful* Target, const std::function<void()>& Setup, const std::function<bool(bool)>& Function)
			{
				if (!Target)
					return false;

				Target->Push();
				if (Setup)
					Setup();

				State.States.insert(Target);
				return Compose(Target, Function);
			}
		}
	}
}