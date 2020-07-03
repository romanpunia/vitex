#include "gui.h"
#ifndef NK_IMPLEMENTATION
#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_BUTTON_TRIGGER_ON_RELEASE
#define NK_IMPLEMENTATION
#include <nuklear.h>
#endif
#ifdef THAWK_HAS_SDL2
#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#define MAP_BUTTON(NAV_NO, BUTTON_NO) { Settings->NavInputs[NAV_NO] = (SDL_GameControllerGetButton(Controller, BUTTON_NO) != 0) ? 1.0f : 0.0f; }
#define MAP_ANALOG(NAV_NO, AXIS_NO, V0, V1) { float V = (float)(SDL_GameControllerGetAxis(Controller, AXIS_NO) - V0) / (float)(V1 - V0); if (V > 1.0f) V = 1.0f; if (V > 0.0f && Settings->NavInputs[NAV_NO] < V) Settings->NavInputs[NAV_NO] = V; }
#endif
#define nk_image_auto(x) x ? nk_image_ptr(x) : nk_image_id(0)

namespace Tomahawk
{
	namespace Engine
	{
		namespace GUI
		{
			struct Vertex
			{
				float Position[2];
				float TexCoord[2];
				nk_byte Color[4];
			};

			static void ClipboardPaste(nk_handle UserData, struct nk_text_edit* Buffer)
			{
				Context* GUI = (Context*)UserData.ptr;
				if (!GUI || !GUI->GetActivity() || !Buffer)
					return;

				std::string Text = GUI->GetActivity()->GetClipboardText();
				nk_textedit_paste(Buffer, Text.c_str(), (int)Text.size());
			}
			static void ClipboardCopy(nk_handle UserData, const char* Buffer, int Size)
			{
				Context* GUI = (Context*)UserData.ptr;
				if (!GUI || !GUI->GetActivity() || !Buffer || Size < 0)
					return;

				GUI->GetActivity()->SetClipboardText(std::string(Buffer, (size_t)Size));
			}
			static struct nk_vec2 CastV2(const Compute::Vector2& Value)
			{
				return nk_vec2(Value.X, Value.Y);
			}
			static nk_color CastV4(const Compute::Vector4& Value)
			{
				return nk_rgba((int)Value.X, (int)Value.Y, (int)Value.Z, (int)Value.W);
			}
			static nk_style_item CastV4Item(ContentManager* Manager, const std::string& Value)
			{
				if (Value.empty())
					return nk_style_item_hide();

				if (Value.size() > 2 && Value[0] == '.' && Value[1] == '/')
				{
					auto* Image = Manager->Load<Graphics::Texture2D>(Value, nullptr);
					if (!Image)
						return nk_style_item_hide();

					return nk_style_item_image(nk_image_auto(Image));
				}

				std::vector<std::string> Args = Rest::Stroke(&Value).SplitMax(' ', 4);
				struct nk_color Color = { 0, 0, 0, 255 };

				if (Args.size() > 0)
					Color.r = Rest::Stroke(&Args[0]).ToFloat();

				if (Args.size() > 1)
					Color.g = Rest::Stroke(&Args[1]).ToFloat();

				if (Args.size() > 2)
					Color.b = Rest::Stroke(&Args[2]).ToFloat();

				if (Args.size() > 3)
					Color.a = Rest::Stroke(&Args[3]).ToFloat();

				return nk_style_item_color(Color);
			}
			static nk_flags CastTextAlign(const std::string& Value)
			{
				nk_flags Flags = 0;
				if (Value.find("right") != std::string::npos)
					Flags |= NK_TEXT_ALIGN_RIGHT;

				if (Value.find("left") != std::string::npos)
					Flags |= NK_TEXT_ALIGN_LEFT;

				if (Value.find("center") != std::string::npos)
					Flags |= NK_TEXT_ALIGN_CENTERED;

				if (Value.find("top") != std::string::npos)
					Flags |= NK_TEXT_ALIGN_TOP;

				if (Value.find("middle") != std::string::npos)
					Flags |= NK_TEXT_ALIGN_MIDDLE;

				if (Value.find("bottom") != std::string::npos)
					Flags |= NK_TEXT_ALIGN_BOTTOM;

				return Flags;
			}
			static nk_style_header_align CastHeaderAlign(const std::string& Value)
			{
				if (Value == "left")
					return NK_HEADER_LEFT;

				return NK_HEADER_RIGHT;
			}
			static nk_symbol_type CastSymbol(const std::string& Value)
			{
				if (Value == "cross")
					return NK_SYMBOL_X;

				if (Value == "underscore")
					return NK_SYMBOL_UNDERSCORE;

				if (Value == "circle-solid")
					return NK_SYMBOL_CIRCLE_SOLID;

				if (Value == "circle-outline")
					return NK_SYMBOL_CIRCLE_OUTLINE;

				if (Value == "rect-solid")
					return NK_SYMBOL_RECT_SOLID;

				if (Value == "rect-outline")
					return NK_SYMBOL_RECT_OUTLINE;

				if (Value == "triangle-up")
					return NK_SYMBOL_TRIANGLE_UP;

				if (Value == "triangle-down")
					return NK_SYMBOL_TRIANGLE_DOWN;

				if (Value == "triangle-left")
					return NK_SYMBOL_TRIANGLE_LEFT;

				if (Value == "triangle-right")
					return NK_SYMBOL_TRIANGLE_RIGHT;

				if (Value == "plus")
					return NK_SYMBOL_PLUS;

				if (Value == "minus")
					return NK_SYMBOL_MINUS;

				return NK_SYMBOL_NONE;
			}
	
			Element::Element(Context* View) : Base(View), Parent(nullptr), Content(nullptr), Class(nullptr), Active(true)
			{
			}
			Element::~Element()
			{
				auto Id = Proxy.find("id");
				if (Id != Proxy.end())
				{
					auto It = Base->Uniques.find(Id->second);
					if (It != Base->Uniques.end() && It->second == this)
						Base->Uniques.erase(It);
				}

				if (Parent != nullptr)
				{
					for (auto It = Parent->Nodes.begin(); It != Parent->Nodes.end(); It++)
					{
						if ((*It) != this)
							continue;

						Parent->Nodes.erase(It);
						break;
					}
				}

				if (Class != nullptr)
					Class->Watchers.erase(this);

				for (auto& It : Nodes)
				{
					It->Parent = nullptr;
					delete It;
				}
			}
			bool Element::Assign(const std::string& Path, const QueryCallback& Function)
			{
				Element* Node = ResolveName(Path);
				if (!Node && !Path.empty())
					return false;

				Node->Callback = Function;
				return true;
			}
			bool Element::Build()
			{
				if (!Active)
					return false;

				BuildStyle(Base->Engine, nullptr);
				bool Continue = BuildBegin(Base->Engine);

				if (Callback != nullptr)
				{
					auto Function = Callback;
					Continue = Function(this, Continue);
				}

				if (Continue)
				{
					for (auto& It : Nodes)
						It->Build();
				}
				
				BuildEnd(Base->Engine);
				BuildStyle(Base->Engine, Base->Style);
				return Continue;
			}
			void Element::Add(Element* Node)
			{
				Nodes.push_back(Node);
			}
			void Element::Remove(Element* Node)
			{
				for (auto It = Nodes.begin(); It != Nodes.end(); It++)
				{
					if ((*It) == Node)
					{
						delete Node;
						Nodes.erase(It);
						break;
					}
				}
			}
			void Element::Recompute(const std::string& Name)
			{
				int Query = 0;
				if (Name.empty())
				{
					std::string* Value = GetStatic("class", &Query);
					if (Value != nullptr)
						SetClass(GetDynamic(Query, *Value));

					for (auto& Item : Reactors)
					{
						if (Item.second)
							Item.second();
					}
				}
				else if (Name == "class")
				{
					std::string* Value = GetStatic("class", &Query);
					if (Value != nullptr)
						SetClass(GetDynamic(Query, *Value));
				}
				else if (Name[0] == ':')
				{
					for (auto& Dynamic : Dynamics)
					{
						auto It = Reactors.find(Dynamic);
						if (It != Reactors.end() && It->second)
							It->second();
					}
				}
				else
				{
					auto It = Reactors.find(Name);
					if (It != Reactors.end() && It->second)
						It->second();
				}
			}
			void Element::Copy(ContentManager* NewContent, Rest::Document* DOM)
			{
				Content = NewContent;
				if (!DOM || !Content)
					return;

				for (auto It : *DOM->GetNodes())
				{
					if (It->IsAttribute())
						Proxy[It->GetName()] = It->Serialize();
				}

				auto Id = Proxy.find("id");
				if (Id != Proxy.end())
					Base->Uniques[Id->second] = this;

				Recompute("");
				for (auto It : *DOM->GetNodes())
				{
					if (It->IsAttribute())
						continue;
					
					auto Factory = Base->Factories.find(It->Name);
					if (Factory == Base->Factories.end() || !Factory->second)
					{
						THAWK_WARN("element \"%s\" has no factory", It->Name.c_str());
						continue;
					}

					Element* Node = Factory->second();
					if (!Node)
					{
						THAWK_WARN("cannot find \"%s\" element type", It->Name.c_str());
						continue;
					}

					Node->Parent = this;
					Node->Copy(Content, It);
					Nodes.push_back(Node);
				}
			}
			void Element::ResolveTable(Rest::Stroke* Name)
			{
				Rest::Stroke::Settle F;
				uint64_t Offset = 0;

				while ((F = Name->Find('$', Offset)).Found)
				{
					uint64_t Size = 0;
					while (F.End + Size < Name->Size() && Name->R()[F.End + Size] != '$')
						Size++;

					Offset = F.End;
					if (Size < 1)
					{
						if (F.End < Name->Size() && Name->R()[F.End] == '$')
						{
							Name->EraseOffsets(F.End, F.End + 1);
							Offset = F.End;
						}

						continue;
					}

					std::string* Sub = GetGlobal(Name->R().substr(F.End, Size));
					Name->ReplacePart(F.Start, F.Start + Size + 2, Sub ? *Sub : "");
				}
			}
			void Element::SetActive(bool Enabled)
			{
				Active = Enabled;
			}
			void Element::SetGlobal(const std::string& Name, const std::string& Value, bool Erase)
			{
				Rest::Stroke Param(Value);
				ResolveTable(&Param);

				auto It = Base->Args.find(Name);
				if (It != Base->Args.end())
				{
					if (Erase)
					{
						delete It->second;
						Base->Args.erase(It);
					}
					else if (It->second->Value != Param.R())
					{
						It->second->Value = Param.R();
						for (auto& Node : It->second->Watchers)
							Node->Recompute(":");
					}
				}
				else if (!Erase)
				{
					Actor* New = new Actor();
					New->Value = Param.R();
					Base->Args[Name] = New;
				}
			}
			void Element::SetLocal(const std::string& Name, const std::string& Value, bool Erase)
			{
				if (Name.empty())
					return;

				if (Erase)
				{
					auto It = Proxy.find(Name);
					if (It != Proxy.end())
						Proxy.erase(It);
				}
				else
					Proxy[Name] = Value;
	
				Recompute(Name);
			}
			void Element::SetClass(const std::string& Name)
			{
				if (Class && Class->ClassName == Name)
					return;

				if (Class != nullptr)
					Class->Watchers.erase(this);

				auto It = Base->Classes.find(Name);
				if (It != Base->Classes.end())
				{
					Class = It->second;
					Class->Watchers.insert(this);
				}
				else
					Class = nullptr;
			}
			void Element::Bind(const std::string& Name, const std::function<void()>& Callback)
			{
				Reactors[Name] = Callback;
			}
			bool Element::IsActive()
			{
				return Active;
			}
			float Element::GetAreaWidth()
			{
				Element* Result = Parent;
				if (!Result)
					return GetWidth();

				while (Result != nullptr)
				{
					float Value = Result->GetWidth();
					if (Value > 0)
						return Value;

					Result = Result->Parent;
				}

				return GetWidth();
			}
			float Element::GetAreaHeight()
			{
				Element* Result = Parent;
				if (!Result)
					return GetHeight();

				while (Result != nullptr)
				{
					float Value = Result->GetHeight();
					if (Value > 0)
						return Value;

					Result = Result->Parent;
				}

				return GetHeight();
			}
			bool Element::GetBoolean(const std::string& Name, bool Default)
			{
				int Query = 0;
				std::string* Value = GetStatic(Name, &Query);

				if (!Value)
					return Default;

				std::string V = GetDynamic(Query, *Value);
				return V == "1" || V == "true";
			}
			float Element::GetFloat(const std::string& Name, float Default)
			{
				int Query = 0;
				std::string* Value = GetStatic(Name, &Query);

				if (!Value)
					return Default;

				return GetFloatRelative(GetDynamic(Query, *Value));
			}
			int Element::GetInteger(const std::string& Name, int Default)
			{
				int Query = 0;
				std::string* Value = GetStatic(Name, &Query);

				if (!Value)
					return Default;

				return (int)GetFloatRelative(GetDynamic(Query, *Value));
			}
			std::string Element::GetModel(const std::string& Name)
			{
				return GetString(Name, "");
			}
			std::string Element::GetString(const std::string& Name, const std::string& Default)
			{
				int Query = 0;
				std::string* Value = GetStatic(Name, &Query);

				if (!Value)
					return Default;

				return GetDynamic(Query, *Value);
			}
			std::string* Element::GetGlobal(const std::string& Name)
			{
				Rest::Stroke Param(Name);
				ResolveTable(&Param);

				auto It = Base->Args.find(Param.R());
				if (It != Base->Args.end())
				{
					It->second->Watchers.insert(this);
					return &It->second->Value;
				}

				Actor* New = new Actor();
				New->Watchers.insert(this);
				Base->Args[Param.R()] = New;

				return &New->Value;
			}
			nk_font* Element::GetFont(const std::string& Name, const std::string& Default)
			{
				return Base->GetFont(GetString(Name, Default));
			}
			std::string Element::GetClass()
			{
				if (!Class)
					return "";

				return Class->ClassName;
			}
			std::string Element::GetId()
			{
				auto It = Proxy.find("id");
				if (It != Proxy.end())
					return It->second;

				return "";
			}
			Compute::Vector2 Element::GetV2(const std::string& Name, const Compute::Vector2& Default)
			{
				int Query = 0;
				std::string* Value = GetStatic(Name, &Query);

				if (!Value)
					return Default;

				std::vector<std::string> Args = Rest::Stroke(GetDynamic(Query, *Value)).SplitMax(' ', 2);
				Compute::Vector2 Result = Default;

				if (Args.size() > 0)
					Result.X = GetFloatRelative(Args[0]);

				if (Args.size() > 1)
					Result.Y = GetFloatRelative(Args[1]);

				return Result;
			}
			Compute::Vector3 Element::GetV3(const std::string& Name, const Compute::Vector3& Default)
			{
				int Query = 0;
				std::string* Value = GetStatic(Name, &Query);

				if (!Value)
					return Default;

				std::vector<std::string> Args = Rest::Stroke(GetDynamic(Query, *Value)).SplitMax(' ', 3);
				Compute::Vector3 Result = Default;

				if (Args.size() > 0)
					Result.X = GetFloatRelative(Args[0]);

				if (Args.size() > 1)
					Result.Y = GetFloatRelative(Args[1]);

				if (Args.size() > 2)
					Result.Z = GetFloatRelative(Args[2]);

				return Result;
			}
			Compute::Vector4 Element::GetV4(const std::string& Name, const Compute::Vector4& Default)
			{
				int Query = 0;
				std::string* Value = GetStatic(Name, &Query);

				if (!Value)
					return Default;

				std::vector<std::string> Args = Rest::Stroke(GetDynamic(Query, *Value)).SplitMax(' ', 4);
				Compute::Vector4 Result = Default;

				if (Args.size() > 0)
					Result.X = GetFloatRelative(Args[0]);

				if (Args.size() > 1)
					Result.Y = GetFloatRelative(Args[1]);

				if (Args.size() > 2)
					Result.Z = GetFloatRelative(Args[2]);

				if (Args.size() > 3)
					Result.W = GetFloatRelative(Args[3]);

				return Result;
			}
			std::vector<Element*>& Element::GetNodes()
			{
				return Nodes;
			}
			Element* Element::GetUpperNode(const std::string& Name)
			{
				if (!Parent)
					return nullptr;

				for (auto It = Parent->Nodes.begin(); It != Parent->Nodes.end(); It++)
				{
					if (*It != this)
						continue;

					if (It == Parent->Nodes.begin())
						return nullptr;

					Element* Result = *(It - 1);
					if (!Result || Name.empty())
						return Result;

					if (Result->GetType() != Name)
						break;

					return Result;	
				}

				return nullptr;
			}
			Element* Element::GetLowerNode(const std::string& Name)
			{
				if (!Parent)
					return nullptr;

				for (auto It = Parent->Nodes.begin(); It != Parent->Nodes.end(); It++)
				{
					if (*It != this)
						continue;

					if (It + 1 == Parent->Nodes.end())
						return nullptr;

					Element* Result = *(It + 1);
					if (!Result || Name.empty())
						return Result;

					if (Result->GetType() != Name)
						break;

					return Result;
				}

				return nullptr;
			}
			Element* Element::GetNodeById(const std::string& Id)
			{
				if (Id.empty())
					return nullptr;

				auto It = Base->Uniques.find(Id);
				if (It != Base->Uniques.end())
					return It->second;

				return nullptr;
			}
			Element* Element::GetNodeByPath(const std::string& Path)
			{
				return ResolveName(Path);
			}
			Element* Element::GetNode(const std::string& Name)
			{
				Rest::Stroke Number(&Name);
				if (Number.HasInteger())
				{
					int64_t Index = Number.ToInt64();
					if (Index >= 0 && Index < Nodes.size())
						return Nodes[Index];
				}

				for (auto K : Nodes)
				{
					if (K->GetType() == Name)
						return K;
				}

				return nullptr;
			}
			Element* Element::GetParent(const std::string& Type)
			{
				Element* Next = Parent;
				while (Next != nullptr)
				{
					if (Next->GetType() == Type)
						return Next;

					Next = Next->Parent;
				}

				return Next;
			}
			Context* Element::GetContext()
			{
				return Base;
			}
			Element* Element::ResolveName(const std::string& Name)
			{
				if (Name.empty())
					return this;

				std::vector<std::string> Names = Rest::Stroke(Name).Split('.');
				if (Names.empty())
					return this;

				Element* Current = this;
				for (auto It = Names.begin(); It != Names.end(); It++)
				{
					Current = Current->GetNode(*It);
					if (!Current)
						return nullptr;
				}

				return Current;
			}
			std::string Element::ResolveValue(const std::string& Value)
			{
				Rest::Stroke Name(Value);
				Rest::Stroke::Settle F;
				uint64_t Offset = 0;

				while ((F = Name.Find('%', Offset)).Found)
				{
					uint64_t Size = 0;
					while (F.End + Size < Name.Size() && Name.R()[F.End + Size] != '%')
						Size++;

					Offset = F.End;
					if (Size < 1)
					{
						if (F.End < Name.Size() && Name.R()[F.End] == '%')
						{
							Name.EraseOffsets(F.End, F.End + 1);
							Offset = F.End;
						}

						continue;
					}

					std::string* Sub = GetGlobal(Name.R().substr(F.End, Size));
					Name.ReplacePart(F.Start, F.Start + Size + 2, Sub ? *Sub : "");
				}

				return Name.R();
			}
			std::string* Element::GetStatic(const std::string& Name, int* Query)
			{
				auto It = Proxy.find(Name);
				if (It != Proxy.end())
				{
					*Query = 0;
					Dynamics.erase(Name);
					return &It->second;
				}

				It = Proxy.find(':' + Name);
				if (It != Proxy.end())
				{
					*Query = 1;
					Dynamics.insert(Name);
					return &It->second;
				}

				It = Proxy.find('@' + Name);
				if (It != Proxy.end())
				{
					*Query = 2;
					Dynamics.insert(Name);
					return &It->second;
				}

				if (!Class)
					return nullptr;

				It = Class->Proxy.find(Name);
				if (It != Class->Proxy.end())
				{
					*Query = 0;
					Dynamics.erase(Name);
					return &It->second;
				}

				It = Class->Proxy.find(':' + Name);
				if (It != Class->Proxy.end())
				{
					*Query = 1;
					Dynamics.insert(Name);
					return &It->second;
				}

				It = Class->Proxy.find('@' + Name);
				if (It != Class->Proxy.end())
				{
					*Query = 2;
					Dynamics.insert(Name);
					return &It->second;
				}

				return nullptr;
			}
			std::string Element::GetDynamic(int Query, const std::string& Value)
			{
				if (Query == 0)
					return Value;

				if (Query == 2)
					return ResolveValue(Value);

				std::string* Result = GetGlobal(Value);
				return Result ? *Result : Value;
			}
			float Element::GetFloatRelative(const std::string& Value)
			{
				Rest::Stroke Number(Value);
				auto Offset = Number.ReverseFind('w');
				bool Width = true;

				if (!Offset.Found)
				{
					Offset = Number.ReverseFind('h');
					if (!Offset.Found)
						return Number.HasNumber() ? Number.ToFloat() : 0;

					Width = false;
				}

				Number.Substring(0, Offset.Start);
				if (!Number.HasNumber())
					return 0;

				float Result = Number.ToFloat();
				if (Base && Width)
					Result *= GetAreaWidth();
				else if (Base && !Width)
					Result *= GetAreaHeight();

				return Result;
			}

			Widget::Widget(Context* View) : Element(View), Cache(nullptr), Font(nullptr)
			{
				Bind("font", [this]() { Font = GetFont("font", ""); });
			}
			Widget::~Widget()
			{
			}
			bool Widget::Build()
			{
				if (Input)
					Input(this);

				return Element::Build();
			}
			void Widget::BuildFont(nk_context* C, nk_style* S)
			{
				if (!Font)
					return;

				if (!S)
				{
					Cache = (nk_user_font*)C->style.font;
					nk_style_set_font(C, &Font->handle);
				}
				else if (Cache != nullptr)
					nk_style_set_font(C, Cache);
			}
			void Widget::SetInputEvents(const std::function<void(Widget*)>& Callback)
			{
				Input = Callback;
			}
			Compute::Vector2 Widget::GetWidgetPosition()
			{
				struct nk_vec2 Result = nk_widget_position(Base->GetContext());
				return Compute::Vector2(Result.x, Result.y);
			}
			Compute::Vector2 Widget::GetWidgetSize()
			{
				struct nk_vec2 Result = nk_widget_size(Base->GetContext());
				return Compute::Vector2(Result.x, Result.y);
			}
			float Widget::GetWidgetWidth()
			{
				return nk_widget_width(Base->GetContext());
			}
			float Widget::GetWidgetHeight()
			{
				return nk_widget_height(Base->GetContext());
			}
			bool Widget::IsWidgetHovered()
			{
				return nk_widget_is_hovered(Base->GetContext());
			}
			bool Widget::IsWidgetClicked(CursorButton Key)
			{
				return nk_widget_is_mouse_clicked(Base->GetContext(), (nk_buttons)Key);
			}
			bool Widget::IsWidgetClickedDown(CursorButton Key, bool Down)
			{
				return nk_widget_has_mouse_click_down(Base->GetContext(), (nk_buttons)Key, (int)Down);
			}

			Stateful::Stateful(Context* View) : Widget(View)
			{
				Hash.Count = 0;
				if (View != nullptr)
				{
					Hash.Id = View->GetNextWidgetId();
					Hash.Text = std::to_string(Hash.Id);
				}
			}
			Stateful::~Stateful()
			{
			}
			std::string& Stateful::GetHash()
			{
				return Hash.Text;
			}
			void Stateful::Push()
			{
				if (!Hash.Text.empty())
				{
					Hash.Text.append(1, '?');
					Hash.Count++;
				}
			}
			void Stateful::Pop()
			{
				if (!Hash.Text.empty() && Hash.Count > 0 && Hash.Count < Hash.Text.size())
				{
					Hash.Text.erase(Hash.Text.size() - Hash.Count, Hash.Count);
					Hash.Count = 0;
				}
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
			void Restyle::BuildEnd(nk_context* C)
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
			void For::BuildEnd(nk_context* C)
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
			void Const::BuildEnd(nk_context* C)
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
			void Set::BuildEnd(nk_context* C)
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
			void LogIf::BuildEnd(nk_context* C)
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
			void LogElse::BuildEnd(nk_context* C)
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

			Escape::Escape(Context* View) : Element(View)
			{
			}
			bool Escape::BuildBegin(nk_context* C)
			{
				Base->EmitEscape();
				return false;
			}
			void Escape::BuildEnd(nk_context* C)
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

			Body::Body(Context* View) : Element(View)
			{
				Style.ScrollH = (nk_style_scrollbar*)malloc(sizeof(nk_style_scrollbar));
				memset(Style.ScrollH, 0, sizeof(nk_style_scrollbar));

				Style.ScrollV = (nk_style_scrollbar*)malloc(sizeof(nk_style_scrollbar));
				memset(Style.ScrollV, 0, sizeof(nk_style_scrollbar));

				Bind("scroll-h-normal", [this]() { Style.ScrollH->normal = CastV4Item(Content, GetString("scroll-h-normal", "40 40 40 255")); });
				Bind("scroll-h-hover", [this]() { Style.ScrollH->hover = CastV4Item(Content, GetString("scroll-h-hover", "40 40 40 255")); });
				Bind("scroll-h-active", [this]() { Style.ScrollH->active = CastV4Item(Content, GetString("scroll-h-active", "40 40 40 255")); });
				Bind("scroll-h-cursor-normal", [this]() { Style.ScrollH->cursor_normal = CastV4Item(Content, GetString("scroll-h-cursor-normal", "100 100 100 255")); });
				Bind("scroll-h-cursor-hover", [this]() { Style.ScrollH->cursor_hover = CastV4Item(Content, GetString("scroll-h-cursor-hover", "120 120 120 255")); });
				Bind("scroll-h-cursor-active", [this]() { Style.ScrollH->cursor_active = CastV4Item(Content, GetString("scroll-h-cursor-active", "150 150 150 255")); });
				Bind("scroll-h-inc-symbol", [this]() { Style.ScrollH->inc_symbol = CastSymbol(GetString("scroll-h-inc-symbol", "circle-solid")); });
				Bind("scroll-h-dec-symbol", [this]() { Style.ScrollH->dec_symbol = CastSymbol(GetString("scroll-h-dec-symbol", "circle-solid")); });
				Bind("scroll-h-border", [this]() { Style.ScrollH->border_color = CastV4(GetV4("scroll-h-border", { 40, 40, 40, 255 })); });
				Bind("scroll-h-cursor-border", [this]() { Style.ScrollH->cursor_border_color = CastV4(GetV4("scroll-h-cursor-border", { 40, 40, 40, 255 })); });
				Bind("scroll-h-padding", [this]() { Style.ScrollH->padding = CastV2(GetV2("scroll-h-padding", { 0, 0 })); });
				Bind("scroll-h-buttons", [this]() { Style.ScrollH->show_buttons = GetBoolean("scroll-h-buttons", false); });
				Bind("scroll-h-border-size", [this]() { Style.ScrollH->border = GetFloat("scroll-h-border-size", 0); });
				Bind("scroll-h-border-radius", [this]() { Style.ScrollH->rounding = GetFloat("scroll-h-border-radius", 0); });
				Bind("scroll-h-cursor-border-size", [this]() { Style.ScrollH->border_cursor = GetFloat("scroll-h-cursor-border-size", 0); });
				Bind("scroll-h-cursor-border-radius", [this]() { Style.ScrollH->rounding_cursor = GetFloat("scroll-h-cursor-border-radius", 0); });
				Bind("scroll-v-normal", [this]() { Style.ScrollV->normal = CastV4Item(Content, GetString("scroll-v-normal", "40 40 40 255")); });
				Bind("scroll-v-hover", [this]() { Style.ScrollV->hover = CastV4Item(Content, GetString("scroll-v-hover", "40 40 40 255")); });
				Bind("scroll-v-active", [this]() { Style.ScrollV->active = CastV4Item(Content, GetString("scroll-v-active", "40 40 40 255")); });
				Bind("scroll-v-cursor-normal", [this]() { Style.ScrollV->cursor_normal = CastV4Item(Content, GetString("scroll-v-cursor-normal", "100 100 100 255")); });
				Bind("scroll-v-cursor-hover", [this]() { Style.ScrollV->cursor_hover = CastV4Item(Content, GetString("scroll-v-cursor-hover", "120 120 120 255")); });
				Bind("scroll-v-cursor-active", [this]() { Style.ScrollV->cursor_active = CastV4Item(Content, GetString("scroll-v-cursor-active", "150 150 150 255")); });
				Bind("scroll-v-inc-symbol", [this]() { Style.ScrollV->inc_symbol = CastSymbol(GetString("scroll-v-inc-symbol", "circle-solid")); });
				Bind("scroll-v-dec-symbol", [this]() { Style.ScrollV->dec_symbol = CastSymbol(GetString("scroll-v-dec-symbol", "circle-solid")); });
				Bind("scroll-v-border", [this]() { Style.ScrollV->border_color = CastV4(GetV4("scroll-v-border", { 40, 40, 40, 255 })); });
				Bind("scroll-v-cursor-border", [this]() { Style.ScrollV->cursor_border_color = CastV4(GetV4("scroll-v-cursor-border", { 40, 40, 40, 255 })); });
				Bind("scroll-v-padding", [this]() { Style.ScrollV->padding = CastV2(GetV2("scroll-v-padding", { 0, 0 })); });
				Bind("scroll-v-buttons", [this]() { Style.ScrollV->show_buttons = GetBoolean("scroll-v-buttons", false); });
				Bind("scroll-v-border-size", [this]() { Style.ScrollV->border = GetFloat("scroll-v-border-size", 0); });
				Bind("scroll-v-border-radius", [this]() { Style.ScrollV->rounding = GetFloat("scroll-v-border-radius", 0); });
				Bind("scroll-v-cursor-border-size", [this]() { Style.ScrollV->border_cursor = GetFloat("scroll-v-cursor-border-size", 0); });
				Bind("scroll-v-cursor-border-radius", [this]() { Style.ScrollV->rounding_cursor = GetFloat("scroll-v-cursor-border-radius", 0); });			
			}
			Body::~Body()
			{
				free(Style.ScrollH);
				free(Style.ScrollV);
			}
			bool Body::BuildBegin(nk_context* C)
			{
				return true;
			}
			void Body::BuildEnd(nk_context* C)
			{
			}
			void Body::BuildStyle(nk_context* C, nk_style* S)
			{
				memcpy(&C->style.scrollh, S ? &S->scrollh : Style.ScrollH, sizeof(nk_style_scrollbar));
				memcpy(&C->style.scrollv, S ? &S->scrollv : Style.ScrollV, sizeof(nk_style_scrollbar));
			}
			float Body::GetWidth()
			{
				return Base->GetWidth();
			}
			float Body::GetHeight()
			{
				return Base->GetHeight();
			}
			std::string Body::GetType()
			{
				return "body";
			}
			bool Body::IsHovered()
			{
				return nk_item_is_any_active(Base->GetContext());
			}

			Panel::Panel(Context* View) : Element(View), Cache(nullptr)
			{
				Style = (nk_style_window*)malloc(sizeof(nk_style_window));
				memset(Style, 0, sizeof(nk_style_window));

				Source.Font = nullptr;
				Source.Flags = 0;
				
				Bind("bordered", [this]()
				{
					if (GetBoolean("bordered", true))
						Source.Flags |= NK_WINDOW_BORDER;
					else
						Source.Flags &= ~NK_WINDOW_BORDER;
				});
				Bind("movable", [this]()
				{
					if (GetBoolean("movable", true))
						Source.Flags |= NK_WINDOW_MOVABLE;
					else
						Source.Flags &= ~NK_WINDOW_MOVABLE;
				});
				Bind("scalable", [this]()
				{
					if (GetBoolean("scalable", true))
						Source.Flags |= NK_WINDOW_SCALABLE;
					else
						Source.Flags &= ~NK_WINDOW_SCALABLE;
				});
				Bind("closable", [this]()
				{
					if (GetBoolean("closable", false))
						Source.Flags |= NK_WINDOW_CLOSABLE;
					else
						Source.Flags &= ~NK_WINDOW_CLOSABLE;
				});
				Bind("minimizable", [this]()
				{
					if (GetBoolean("minimizable", true))
						Source.Flags |= NK_WINDOW_MINIMIZABLE;
					else
						Source.Flags &= ~NK_WINDOW_MINIMIZABLE;
				});
				Bind("no-scrollbar", [this]()
				{
					if (GetBoolean("no-scrollbar", false))
						Source.Flags |= NK_WINDOW_NO_SCROLLBAR;
					else
						Source.Flags &= ~NK_WINDOW_NO_SCROLLBAR;
				});
				Bind("titled", [this]()
				{
					if (GetBoolean("titled", true))
						Source.Flags |= NK_WINDOW_TITLE;
					else
						Source.Flags &= ~NK_WINDOW_TITLE;
				});
				Bind("scroll-auto-hide", [this]()
				{
					if (GetBoolean("scroll-auto-hide", false))
						Source.Flags |= NK_WINDOW_SCROLL_AUTO_HIDE;
					else
						Source.Flags &= ~NK_WINDOW_SCROLL_AUTO_HIDE;
				});
				Bind("focusless", [this]()
				{
					if (GetBoolean("focusless", false))
						Source.Flags |= NK_WINDOW_BACKGROUND;
					else
						Source.Flags &= ~NK_WINDOW_BACKGROUND;
				});
				Bind("scale-left", [this]()
				{
					if (GetBoolean("scale-left", false))
						Source.Flags |= NK_WINDOW_SCALE_LEFT;
					else
						Source.Flags &= ~NK_WINDOW_SCALE_LEFT;
				});
				Bind("no-input", [this]()
				{
					if (GetBoolean("no-input", false))
						Source.Flags |= NK_WINDOW_NO_INPUT;
					else
						Source.Flags &= ~NK_WINDOW_NO_INPUT;
				});
				Bind("text", [this]()
				{
					Source.Text = GetString("text", "");
					Rest::Stroke(&Source.Text).ReplaceOf("\r\n", "");
				});
				Bind("font", [this]() { Source.Font = GetFont("font", ""); });
				Bind("width", [this]() { Source.Width = GetFloat("width", 100); });
				Bind("height", [this]() { Source.Height = GetFloat("height", 100); });
				Bind("x", [this]() { Source.X = GetFloat("x", 0); });
				Bind("y", [this]() { Source.Y = GetFloat("y", 0); });
				Bind("relative", [this]() { Source.Relative = GetBoolean("relative", 0); });
				Bind("header-align", [this]() { Style->header.align = CastHeaderAlign(GetString("header-align", "right")); });
				Bind("header-close-symbol", [this]() { Style->header.close_symbol = CastSymbol(GetString("header-close-symbol", "cross")); });
				Bind("header-hide-symbol", [this]() { Style->header.minimize_symbol = CastSymbol(GetString("header-hide-symbol", "minus")); });
				Bind("header-show-symbol", [this]() { Style->header.maximize_symbol = CastSymbol(GetString("header-show-symbol", "plus")); });
				Bind("header-normal", [this]() { Style->header.normal = CastV4Item(Content, GetString("header-normal", "40 40 40 255")); });
				Bind("header-hover", [this]() { Style->header.hover = CastV4Item(Content, GetString("header-hover", "40 40 40 255")); });
				Bind("header-active", [this]() { Style->header.active = CastV4Item(Content, GetString("header-active", "40 40 40 255")); });
				Bind("header-label-normal", [this]() { Style->header.label_normal = CastV4(GetV4("header-label-normal", { 175, 175, 175, 255 })); });
				Bind("header-label-hover", [this]() { Style->header.label_hover = CastV4(GetV4("header-label-hover", { 175, 175, 175, 255 })); });
				Bind("header-label-active", [this]() { Style->header.label_active = CastV4(GetV4("header-label-active", { 175, 175, 175, 255 })); });
				Bind("header-label-padding", [this]() { Style->header.label_padding = CastV2(GetV2("header-label-padding", { 4, 4 })); });
				Bind("header-padding", [this]() { Style->header.padding = CastV2(GetV2("header-padding", { 4, 4 })); });
				Bind("header-spacing", [this]() { Style->header.spacing = CastV2(GetV2("header-spacing", { 0, 0 })); });
				Bind("header-close-normal", [this]() { Style->header.close_button.normal = CastV4Item(Content, GetString("header-close-normal", "40 40 40 255")); });
				Bind("header-close-hover", [this]() { Style->header.close_button.hover = CastV4Item(Content, GetString("header-close-hover", "40 40 40 255")); });
				Bind("header-close-active", [this]() { Style->header.close_button.active = CastV4Item(Content, GetString("header-close-active", "40 40 40 255")); });
				Bind("header-close-border", [this]() { Style->header.close_button.border_color = CastV4(GetV4("header-close-border", { 0, 0, 0, 0 })); });
				Bind("header-close-text-background", [this]() { Style->header.close_button.text_background = CastV4(GetV4("header-close-text-background", { 40, 40, 40, 255 })); });
				Bind("header-close-text-normal", [this]() { Style->header.close_button.text_normal = CastV4(GetV4("header-close-text-normal", { 175, 175, 175, 255 })); });
				Bind("header-close-text-hover", [this]() { Style->header.close_button.text_hover = CastV4(GetV4("header-close-text-hover", { 175, 175, 175, 255 })); });
				Bind("header-close-text-active", [this]() { Style->header.close_button.text_active = CastV4(GetV4("header-close-text-active", { 175, 175, 175, 255 })); });
				Bind("header-close-padding", [this]() { Style->header.close_button.padding = CastV2(GetV2("header-close-padding", { 4, 4 })); });
				Bind("header-close-touch-padding", [this]() { Style->header.close_button.touch_padding = CastV2(GetV2("header-close-touch-padding", { 4, 4 })); });
				Bind("header-close-text-align", [this]() { Style->header.close_button.text_alignment = CastTextAlign(GetString("header-close-text-align", "middle center")); });
				Bind("header-close-border-size", [this]() { Style->header.close_button.border = GetFloat("header-close-border-size", 0.0f); });
				Bind("header-close-border-radius", [this]() { Style->header.close_button.rounding = GetFloat("header-close-border-radius", 0.0f); });
				Bind("header-hide-normal", [this]() { Style->header.minimize_button.normal = CastV4Item(Content, GetString("header-hide-normal", "40 40 40 255")); });
				Bind("header-hide-hover", [this]() { Style->header.minimize_button.hover = CastV4Item(Content, GetString("header-hide-hover", "40 40 40 255")); });
				Bind("header-hide-active", [this]() { Style->header.minimize_button.active = CastV4Item(Content, GetString("header-hide-active", "40 40 40 255")); });
				Bind("header-hide-border", [this]() { Style->header.minimize_button.border_color = CastV4(GetV4("header-hide-border", { 0, 0, 0, 0 })); });
				Bind("header-hide-text-background", [this]() { Style->header.minimize_button.text_background = CastV4(GetV4("header-hide-text-background", { 40, 40, 40, 255 })); });
				Bind("header-hide-text-normal", [this]() { Style->header.minimize_button.text_normal = CastV4(GetV4("header-hide-text-normal", { 175, 175, 175, 255 })); });
				Bind("header-hide-text-hover", [this]() { Style->header.minimize_button.text_hover = CastV4(GetV4("header-hide-text-hover", { 175, 175, 175, 255 })); });
				Bind("header-hide-text-active", [this]() { Style->header.minimize_button.text_active = CastV4(GetV4("header-hide-text-active", { 175, 175, 175, 255 })); });
				Bind("header-hide-padding", [this]() { Style->header.minimize_button.padding = CastV2(GetV2("header-hide-padding", { 0, 0 })); });
				Bind("header-hide-touch-padding", [this]() { Style->header.minimize_button.touch_padding = CastV2(GetV2("header-hide-touch-padding", { 0, 0 })); });
				Bind("header-hide-text-align", [this]() { Style->header.minimize_button.text_alignment = CastTextAlign(GetString("header-hide-text-align", "middle center")); });
				Bind("header-hide-border-size", [this]() { Style->header.minimize_button.border = GetFloat("header-hide-border-size", 0.0f); });
				Bind("header-hide-border-radius", [this]() { Style->header.minimize_button.rounding = GetFloat("header-hide-border-radius", 0.0f); });
				Bind("background", [this]() { Style->background = CastV4(GetV4("background", { 45, 45, 45, 255 })); });
				Bind("fixed", [this]() { Style->fixed_background = CastV4Item(Content, GetString("fixed", "45 45 45 255")); });
				Bind("border", [this]() { Style->border_color = CastV4(GetV4("border", { 65, 65, 65, 255 })); });
				Bind("popup-border", [this]() { Style->popup_border_color = CastV4(GetV4("popup-border", { 65, 65, 65, 255 })); });
				Bind("combo-border", [this]() { Style->combo_border_color = CastV4(GetV4("combo-border", { 65, 65, 65, 255 })); });
				Bind("context-border", [this]() { Style->contextual_border_color = CastV4(GetV4("context-border", { 65, 65, 65, 255 })); });
				Bind("menu-border", [this]() { Style->menu_border_color = CastV4(GetV4("menu-border", { 65, 65, 65, 255 })); });
				Bind("group-border", [this]() { Style->group_border_color = CastV4(GetV4("group-border", { 65, 65, 65, 255 })); });
				Bind("tool-border", [this]() { Style->tooltip_border_color = CastV4(GetV4("tool-border", { 65, 65, 65, 255 })); });
				Bind("scaler", [this]() { Style->scaler = CastV4Item(Content, GetString("scaler", "175 175 175 255")); });
				Bind("border-radius", [this]() { Style->rounding = GetFloat("border-radius", 0.0f); });
				Bind("spacing", [this]() { Style->spacing = CastV2(GetV2("spacing", { 4, 4 })); });
				Bind("scroll-size", [this]() { Style->scrollbar_size = CastV2(GetV2("scroll-size", { 10, 10 })); });
				Bind("min-size", [this]() { Style->min_size = CastV2(GetV2("min-size", { 64, 64 })); });
				Bind("combo-border-size", [this]() { Style->combo_border = GetFloat("combo-border-size", 1.0f); });
				Bind("context-border-size", [this]() { Style->contextual_border = GetFloat("context-border-size", 1.0f); });
				Bind("menu-border-size", [this]() { Style->menu_border = GetFloat("menu-border-size", 1.0f); });
				Bind("group-border-size", [this]() { Style->group_border = GetFloat("group-border-size", 1.0f); });
				Bind("tool-border-size", [this]() { Style->tooltip_border = GetFloat("tool-border-size", 1.0f); });
				Bind("popup-border-size", [this]() { Style->popup_border = GetFloat("popup-border-size", 1.0f); });
				Bind("border-size", [this]() { Style->border = GetFloat("border-size", 2.0f); });
				Bind("min-row-height-padding", [this]() { Style->min_row_height_padding = GetFloat("min-row-height-padding", 8.0f); });
				Bind("padding", [this]() { Style->padding = CastV2(GetV2("padding", { 4, 4 })); });
				Bind("group-padding", [this]() { Style->group_padding = CastV2(GetV2("group-padding", { 4, 4 })); });
				Bind("popup-padding", [this]() { Style->popup_padding = CastV2(GetV2("popup-padding", { 4, 4 })); });
				Bind("combo-padding", [this]() { Style->combo_padding = CastV2(GetV2("combo-padding", { 4, 4 })); });
				Bind("context-padding", [this]() { Style->contextual_padding = CastV2(GetV2("context-padding", { 4, 4 })); });
				Bind("menu-padding", [this]() { Style->menu_padding = CastV2(GetV2("menu-padding", { 4, 4 })); });
				Bind("tool-padding", [this]() { Style->tooltip_padding = CastV2(GetV2("tool-padding", { 4, 4 })); });
			}
			Panel::~Panel()
			{
				free(Style);
			}
			bool Panel::BuildBegin(nk_context* C)
			{
				struct nk_rect Bounds = nk_rect(Source.X, Source.Y, Source.Width, Source.Height);
				if (Source.Relative)
				{
					float Width = GetAreaWidth();
					Bounds.x *= Width;
					Bounds.w *= Width;

					float Height = GetAreaHeight();
					Bounds.y *= Height;
					Bounds.h *= Height;

					nk_window_set_bounds(C, Source.Text.c_str(), Bounds);
				}

				State.Opened = nk_begin(C, Source.Text.c_str(), Bounds, Source.Flags);
				return State.Opened;
			}
			void Panel::BuildEnd(nk_context* C)
			{
				nk_end(C);
			}
			void Panel::BuildStyle(nk_context* C, nk_style* S)
			{
				memcpy(&C->style.window, S ? &S->window : Style, sizeof(nk_style_window));
				if (!Source.Font)
					return;

				if (S != nullptr)
				{
					Cache = (nk_user_font*)S->font;
					nk_style_set_font(C, &Source.Font->handle);
				}
				else if (Cache != nullptr)
					nk_style_set_font(C, Cache);
			}
			float Panel::GetWidth()
			{
				return Source.Relative ? Source.Width * GetAreaWidth() : Source.Width;
			}
			float Panel::GetHeight()
			{
				return Source.Relative ? Source.Height * GetAreaHeight() : Source.Height;
			}
			std::string Panel::GetType()
			{
				return "canvas";
			}
			void Panel::SetPosition(const Compute::Vector2& Position)
			{
				nk_window_set_position(Base->GetContext(), Source.Text.c_str(), nk_vec2(Position.X, Position.Y));
			}
			void Panel::SetSize(const Compute::Vector2& Size)
			{
				nk_window_set_size(Base->GetContext(), Source.Text.c_str(), nk_vec2(Size.X, Size.Y));
			}
			void Panel::SetFocus()
			{
				nk_window_set_focus(Base->GetContext(), Source.Text.c_str());
			}
			void Panel::SetScroll(const Compute::Vector2& Offset)
			{
				nk_window_set_scroll(Base->GetContext(), Offset.X, Offset.Y);
			}
			void Panel::Close()
			{
				nk_window_close(Base->GetContext(), Source.Text.c_str());
			}
			void Panel::Collapse(bool Maximized)
			{
				nk_window_collapse(Base->GetContext(), Source.Text.c_str(), Maximized ? NK_MAXIMIZED : NK_MINIMIZED);
			}
			void Panel::Show(bool Shown)
			{
				nk_window_show(Base->GetContext(), Source.Text.c_str(), Shown ? NK_SHOWN : NK_HIDDEN);
			}
			float Panel::GetClientWidth()
			{
				return nk_window_get_width(Base->GetContext());
			}
			float Panel::GetClientHeight()
			{
				return nk_window_get_height(Base->GetContext());
			}
			bool Panel::HasFocus()
			{
				return nk_window_has_focus(Base->GetContext());
			}
			bool Panel::IsHovered()
			{
				return nk_window_is_hovered(Base->GetContext());
			}
			bool Panel::IsCollapsed()
			{
				return nk_window_is_collapsed(Base->GetContext(), Source.Text.c_str());
			}
			bool Panel::IsHidden()
			{
				return nk_window_is_hidden(Base->GetContext(), Source.Text.c_str());
			}
			bool Panel::IsFocusActive()
			{
				return nk_window_is_active(Base->GetContext(), Source.Text.c_str());
			}
			bool Panel::IsOpened()
			{
				return State.Opened;
			}
			Compute::Vector2 Panel::GetPosition()
			{
				struct nk_vec2 Result = nk_window_get_position(Base->GetContext());
				return Compute::Vector2(Result.x, Result.y);
			}
			Compute::Vector2 Panel::GetSize()
			{
				struct nk_vec2 Result = nk_window_get_size(Base->GetContext());
				return Compute::Vector2(Result.x, Result.y);
			}
			Compute::Vector2 Panel::GetContentRegionMin()
			{
				struct nk_vec2 Result = nk_window_get_content_region_min(Base->GetContext());
				return Compute::Vector2(Result.x, Result.y);
			}
			Compute::Vector2 Panel::GetContentRegionMax()
			{
				struct nk_vec2 Result = nk_window_get_content_region_max(Base->GetContext());
				return Compute::Vector2(Result.x, Result.y);
			}
			Compute::Vector2 Panel::GetContentRegionSize()
			{
				struct nk_vec2 Result = nk_window_get_content_region_size(Base->GetContext());
				return Compute::Vector2(Result.x, Result.y);
			}
			Compute::Vector2 Panel::GetScroll()
			{
				nk_uint X, Y;
				nk_window_get_scroll(Base->GetContext(), &X, &Y);
				return Compute::Vector2((float)X, (float)Y);
			}

			PreLayout::PreLayout(Context* View) : Element(View)
			{
				Bind("height", [this]() { Source.Height = GetFloat("height", 30); });
				Bind("relative", [this]() { Source.Relative = GetBoolean("relative", 0); });
			}
			bool PreLayout::BuildBegin(nk_context* C)
			{
				nk_layout_row_template_begin(C, Source.Relative ? Source.Height * GetAreaHeight() : Source.Height);
				return true;
			}
			void PreLayout::BuildEnd(nk_context* C)
			{
				nk_layout_row_template_end(C);
			}
			void PreLayout::BuildStyle(nk_context* C, nk_style* S)
			{
			}
			float PreLayout::GetWidth()
			{
				return 0;
			}
			float PreLayout::GetHeight()
			{
				return Source.Relative ? Source.Height * GetAreaHeight() : Source.Height;
			}
			std::string PreLayout::GetType()
			{
				return "pre-layout";
			}

			PrePush::PrePush(Context* View) : Element(View)
			{
				State.Layer = nullptr;

				Bind("display", [this]()
				{
					std::string Display = GetString("display", "static");
					if (Display == "static")
						Source.Type = PushType_Static;
					else if (Display == "variable")
						Source.Type = PushType_Variable;
					else
						Source.Type = PushType_Dynamic;
				});
				Bind("min-width", [this]() { Source.MinWidth = GetFloat("min-width", 40); });
				Bind("width", [this]() { Source.Width = GetFloat("width", 40); });
				Bind("relative", [this]() { Source.Relative = GetBoolean("relative", 0); });
			}
			bool PrePush::BuildBegin(nk_context* C)
			{
				if (!State.Layer)
				{
					State.Layer = (PreLayout*)GetParent("pre-layout");
					if (!State.Layer)
						return false;
				}

				if (Source.Type == PushType_Dynamic)
					nk_layout_row_template_push_dynamic(C);
				else if (Source.Type == PushType_Static)
					nk_layout_row_template_push_static(C, Source.Relative ? Source.Width * GetAreaWidth() : Source.Width);
				else if (Source.Type == PushType_Variable)
					nk_layout_row_template_push_variable(C, Source.Relative ? Source.MinWidth * GetAreaWidth() : Source.MinWidth);

				return true;
			}
			void PrePush::BuildEnd(nk_context* C)
			{
			}
			void PrePush::BuildStyle(nk_context* C, nk_style* S)
			{
			}
			float PrePush::GetWidth()
			{
				return Source.Relative ? Source.Width * GetAreaWidth() : Source.Width;
			}
			float PrePush::GetHeight()
			{
				return 0;
			}
			std::string PrePush::GetType()
			{
				return "pre-push";
			}

			Layout::Layout(Context* View) : Element(View)
			{
				Bind("display", [this]()
				{
					std::string Display = GetString("display", "static");
					if (Display == "dynamic")
						Source.Type = LayoutType_Dynamic;
					else if (Display == "static")
						Source.Type = LayoutType_Static;
					else if (Display == "flex")
						Source.Type = LayoutType_Flex;
					else if (Display == "space")
						Source.Type = LayoutType_Space;
				});
				Bind("format", [this]()
				{
					std::string Type = GetString("format", "static");
					if (Type == "static")
						Source.Format = NK_STATIC;
					else
						Source.Format = NK_DYNAMIC;
				});
				Bind("width", [this]() { Source.Width = GetFloat("width", 80); });
				Bind("height", [this]() { Source.Height = GetFloat("height", 30); });
				Bind("size", [this]() { Source.Size = GetInteger("size", 1); });
				Bind("relative", [this]() { Source.Relative = GetBoolean("relative", 0); });
			}
			bool Layout::BuildBegin(nk_context* C)
			{
				switch (Source.Type)
				{
					case LayoutType_Dynamic:
						nk_layout_row_dynamic(C, Source.Relative ? Source.Height * GetAreaHeight() : Source.Height, Source.Size);
						break;
					case LayoutType_Static:
						nk_layout_row_static(C, Source.Relative ? Source.Height * GetAreaHeight() : Source.Height, Source.Relative ? Source.Width * GetAreaWidth() : Source.Width, Source.Size);
						break;
					case LayoutType_Flex:
						nk_layout_row_begin(C, (nk_layout_format)Source.Format, Source.Relative ? Source.Height * GetAreaHeight() : Source.Height, Source.Size);
						break;
					case LayoutType_Space:
						nk_layout_space_begin(C, (nk_layout_format)Source.Format, Source.Relative ? Source.Height * GetAreaHeight() : Source.Height, INT_MAX);
						break;
				}

				return true;
			}
			void Layout::BuildEnd(nk_context* C)
			{
				switch (Source.Type)
				{
					case LayoutType_Flex:
						nk_layout_row_end(C);
						break;
					case LayoutType_Space:
						nk_layout_space_end(C);
						break;
					default:
						break;
				}
			}
			void Layout::BuildStyle(nk_context* C, nk_style* S)
			{
			}
			float Layout::GetWidth()
			{
				return 0;
			}
			float Layout::GetHeight()
			{
				return Source.Relative ? Source.Height * GetAreaHeight() : Source.Height;
			}
			std::string Layout::GetType()
			{
				return "layout";
			}

			Push::Push(Context* View) : Element(View)
			{
				State.Layer = nullptr;

				Bind("width", [this]() { Source.Width = GetFloat("width", 40); });
				Bind("height", [this]() { Source.Height = GetFloat("height", 80); });
				Bind("x", [this]() { Source.X = GetFloat("x", 0); });
				Bind("y", [this]() { Source.Y = GetFloat("y", 0); });
				Bind("relative", [this]() { Source.Relative = GetBoolean("relative", 0); });
			}
			bool Push::BuildBegin(nk_context* C)
			{
				if (!State.Layer)
				{
					State.Layer = (Layout*)GetParent("layout");
					if (!State.Layer)
						return false;
				}

				if (State.Layer->Source.Type == LayoutType_Space)
				{
					struct nk_rect Bounds = nk_rect(Source.X, Source.Y, Source.Width, Source.Height);
					if (Source.Relative)
					{
						float Width = GetAreaWidth();
						Bounds.x *= Width;
						Bounds.w *= Width;

						float Height = GetAreaHeight();
						Bounds.y *= Height;
						Bounds.h *= Height;
					}

					nk_layout_space_push(C, Bounds);
				}
				else if (State.Layer->Source.Type == LayoutType_Flex)
					nk_layout_row_push(C, Source.Relative ? Source.Width * GetAreaWidth() : Source.Width);

				return true;
			}
			void Push::BuildEnd(nk_context* C)
			{
			}
			void Push::BuildStyle(nk_context* C, nk_style* S)
			{
			}
			float Push::GetWidth()
			{
				return Source.Relative ? Source.Width * GetAreaWidth() : Source.Width;
			}
			float Push::GetHeight()
			{
				return Source.Relative ? Source.Height * GetAreaHeight() : Source.Height;
			}
			std::string Push::GetType()
			{
				return "push";
			}

			Button::Button(Context* View) : Widget(View)
			{
				Style = (nk_style_button*)malloc(sizeof(nk_style_button));
				memset(Style, 0, sizeof(nk_style_button));

				Source.Image = nullptr;

				Bind("type", [this]()
				{
					std::string Type = GetString("type", "text");
					if (Type == "text")
						Source.Type = ButtonType_Text;
					else if (Type == "color")
						Source.Type = ButtonType_Color;
					else if (Type == "symbol")
						Source.Type = ButtonType_Symbol;
					else if (Type == "symbol-text")
						Source.Type = ButtonType_Symbol_Text;
					else if (Type == "image")
						Source.Type = ButtonType_Image;
					else if (Type == "image-text")
						Source.Type = ButtonType_Image_Text;
				});
				Bind("text", [this]()
				{
					Source.Text = GetString("text", "");
					Rest::Stroke(&Source.Text).ReplaceOf("\r\n", "");
				});
				Bind("image", [this]() { Source.Image = Content->Load<Graphics::Texture2D>(GetString("image", ""), nullptr); });
				Bind("color", [this]() { Source.Color = GetV4("color", { 50, 50, 50, 255 }); });
				Bind("symbol", [this]() { Source.Symbol = CastSymbol(GetString("symbol", "none")); });
				Bind("image-align", [this]() { Source.Align = CastTextAlign(GetString("image-align", "left")); });
				Bind("repeat", [this]() { Source.Repeat = GetBoolean("repeat", false); });
				Bind("normal", [this]() { Style->normal = CastV4Item(Content, GetString("normal", "50 50 50 255")); });
				Bind("hover", [this]() { Style->hover = CastV4Item(Content, GetString("hover", "40 40 40 255")); });
				Bind("active", [this]() { Style->active = CastV4Item(Content, GetString("active", "35 35 35 255")); });
				Bind("border", [this]() { Style->border_color = CastV4(GetV4("border", { 65, 65, 65, 255 })); });
				Bind("text-background", [this]() { Style->text_background = CastV4(GetV4("text-background", { 50, 50, 50, 255 })); });
				Bind("text-normal", [this]() { Style->text_normal = CastV4(GetV4("text-normal", { 175, 175, 175, 255 })); });
				Bind("text-hover", [this]() { Style->text_hover = CastV4(GetV4("text-hover", { 175, 175, 175, 255 })); });
				Bind("text-active", [this]() { Style->text_active = CastV4(GetV4("text-active", { 175, 175, 175, 255 })); });
				Bind("padding", [this]() { Style->padding = CastV2(GetV2("padding", { 2.0f, 2.0f })); });
				Bind("image-padding", [this]() { Style->image_padding = CastV2(GetV2("image-padding", { 0, 0 })); });
				Bind("touch-padding", [this]() { Style->touch_padding = CastV2(GetV2("touch-padding", { 0, 0 })); });
				Bind("text-align", [this]() { Style->text_alignment = CastTextAlign(GetString("text-align", "middle center")); });
				Bind("border-size", [this]() { Style->border = GetFloat("border-size", 1.0f); });
				Bind("border-radius", [this]() { Style->rounding = GetFloat("border-radius", 4.0f); });
			}
			Button::~Button()
			{
				free(Style);
			}
			bool Button::BuildBegin(nk_context* C)
			{
				if (Source.Repeat)
					nk_button_push_behavior(C, NK_BUTTON_REPEATER);

				switch (Source.Type)
				{
					case ButtonType_Color:
						State.Clicked = nk_button_color(C, CastV4(Source.Color));
						break;
					case ButtonType_Symbol:
						State.Clicked = nk_button_symbol(C, (nk_symbol_type)Source.Symbol);
						break;
					case ButtonType_Symbol_Text:
						State.Clicked = nk_button_symbol_text(C, (nk_symbol_type)Source.Symbol, Source.Text.c_str(), Source.Text.size(), Source.Align);
						break;
					case ButtonType_Image:
						State.Clicked = nk_button_image(C, nk_image_auto(Source.Image));
						break;
					case ButtonType_Image_Text:
						State.Clicked = nk_button_image_text(C, nk_image_auto(Source.Image), Source.Text.c_str(), Source.Text.size(), Source.Align);
						break;
					case ButtonType_Text:
					default:
						State.Clicked = nk_button_text(C, Source.Text.c_str(), Source.Text.size());
						break;
				}

				if (Source.Repeat)
					nk_button_pop_behavior(C);

				return State.Clicked;
			}
			void Button::BuildEnd(nk_context* C)
			{
			}
			void Button::BuildStyle(nk_context* C, nk_style* S)
			{
				memcpy(&C->style.button, S ? &S->button : Style, sizeof(nk_style_button));
				BuildFont(C, S);
			}
			float Button::GetWidth()
			{
				return 0;
			}
			float Button::GetHeight()
			{
				return 0;
			}
			std::string Button::GetType()
			{
				return "button";
			}
			bool Button::IsClicked()
			{
				return State.Clicked;
			}

			Text::Text(Context* View) : Widget(View)
			{
				Style = (nk_style_text*)malloc(sizeof(nk_style_text));
				memset(Style, 0, sizeof(nk_style_text));

				Bind("type", [this]()
				{
					std::string Type = GetString("type", "label");
					if (Type == "label")
						Source.Type = TextType_Label;
					else if (Type == "label-colored")
						Source.Type = TextType_Label_Colored;
					else if (Type == "wrap")
						Source.Type = TextType_Wrap;
					else if (Type == "wrap-colored")
						Source.Type = TextType_Wrap_Colored;
				});
				Bind("text", [this]()
				{
					Source.Text = GetString("text", "");
					Rest::Stroke(&Source.Text).ReplaceOf("\r\n", "");
				});
				Bind("color", [this]() { Source.Color = GetV4("color", { 175, 175, 175, 255 }); });
				Bind("align", [this]() { Source.Align = CastTextAlign(GetString("align", "left")); });
				Bind("color", [this]() { Style->color = CastV4(GetV4("color", { 175, 175, 175, 255 })); });
				Bind("padding", [this]() { Style->padding = CastV2(GetV2("padding", { 0.0f, 0.0f })); });
			}
			Text::~Text()
			{
				free(Style);
			}
			bool Text::BuildBegin(nk_context* C)
			{
				switch (Source.Type)
				{
					case TextType_Label_Colored:
						nk_text_colored(C, Source.Text.c_str(), Source.Text.size(), Source.Align, CastV4(Source.Color));
						break;
					case TextType_Wrap:
						nk_text_wrap(C, Source.Text.c_str(), Source.Text.size());
						break;
					case TextType_Wrap_Colored:
						nk_text_wrap_colored(C, Source.Text.c_str(), Source.Text.size(), CastV4(Source.Color));
						break;
					case TextType_Label:
					default:
						nk_text(C, Source.Text.c_str(), Source.Text.size(), Source.Align);
						break;
				}

				return true;
			}
			void Text::BuildEnd(nk_context* C)
			{
			}
			void Text::BuildStyle(nk_context* C, nk_style* S)
			{
				memcpy(&C->style.text, S ? &S->text : Style, sizeof(nk_style_text));
				BuildFont(C, S);
			}
			float Text::GetWidth()
			{
				return 0;
			}
			float Text::GetHeight()
			{
				return 0;
			}
			std::string Text::GetType()
			{
				return "text";
			}

			Image::Image(Context* View) : Widget(View)
			{
				Source.Image = nullptr;

				Bind("type", [this]()
				{
					std::string Type = GetString("type", "image");
					if (Type == "image")
						Source.Type = TextType_Label;
					else if (Type == "image-colored")
						Source.Type = TextType_Label_Colored;
				});
				Bind("image", [this]() { Source.Image = Content->Load<Graphics::Texture2D>(GetString("image", ""), nullptr); });
				Bind("color", [this]() { Source.Color = GetV4("color", { 175, 175, 175, 255 }); });
			}
			Image::~Image()
			{
			}
			bool Image::BuildBegin(nk_context* C)
			{
				switch (Source.Type)
				{
					case TextType_Label_Colored:
						nk_image_color(C, nk_image_auto(Source.Image), CastV4(Source.Color));
						break;
					case TextType_Label:
					default:
						nk_image(C, Source.Image ? nk_image_auto(Source.Image) : nk_image_id(0));
						break;
				}

				return true;
			}
			void Image::BuildEnd(nk_context* C)
			{
			}
			void Image::BuildStyle(nk_context* C, nk_style* S)
			{
			}
			float Image::GetWidth()
			{
				return 0;
			}
			float Image::GetHeight()
			{
				return 0;
			}
			std::string Image::GetType()
			{
				return "image";
			}

			Checkbox::Checkbox(Context* View) : Widget(View)
			{
				Style = (nk_style_toggle*)malloc(sizeof(nk_style_toggle));
				memset(Style, 0, sizeof(nk_style_toggle));

				State.Value = 0;

				Bind("bind", [this]()
				{
					Source.Bind = GetModel("bind");
					if (Source.Bind.empty())
						return;

					std::string* Result = GetGlobal(Source.Bind);
					if (Result != nullptr)
						State.Value = (int)(*Result == "1" || *Result == "true");
				});
				Bind("text", [this]()
				{
					Source.Text = GetString("text", "");
					Rest::Stroke(&Source.Text).ReplaceOf("\r\n", "");
				});
				Bind("normal", [this]() { Style->normal = CastV4Item(Content, GetString("normal", "100 100 100 255")); });
				Bind("hover", [this]() { Style->hover = CastV4Item(Content, GetString("hover", "120 120 120 255")); });
				Bind("active", [this]() { Style->active = CastV4Item(Content, GetString("active", "120 120 120 255")); });
				Bind("cursor-normal", [this]() { Style->cursor_normal = CastV4Item(Content, GetString("cursor-normal", "45 45 45 255")); });
				Bind("cursor-hover", [this]() { Style->cursor_hover = CastV4Item(Content, GetString("cursor-hover", "45 45 45 255")); });
				Bind("border", [this]() { Style->border_color = CastV4(GetV4("border", { 0, 0, 0, 0 })); });
				Bind("text-background", [this]() { Style->text_background = CastV4(GetV4("text-background", { 45, 45, 45, 255 })); });
				Bind("text-normal", [this]() { Style->text_normal = CastV4(GetV4("text-normal", { 175, 175, 175, 255 })); });
				Bind("text-hover", [this]() { Style->text_hover = CastV4(GetV4("text-hover", { 175, 175, 175, 255 })); });
				Bind("text-active", [this]() { Style->text_active = CastV4(GetV4("text-active", { 175, 175, 175, 255 })); });
				Bind("padding", [this]() { Style->padding = CastV2(GetV2("padding", { 2.0f, 2.0f })); });
				Bind("touch-padding", [this]() { Style->touch_padding = CastV2(GetV2("touch-padding", { 0, 0 })); });
				Bind("text-align", [this]() { Style->text_alignment = CastTextAlign(GetString("text-align", "middle center")); });
				Bind("border-size", [this]() { Style->border = GetFloat("border-size", 0.0f); });
				Bind("spacing", [this]() { Style->spacing = GetFloat("spacing", 4.0f); });
			}
			Checkbox::~Checkbox()
			{
				free(Style);
			}
			bool Checkbox::BuildBegin(nk_context* C)
			{
				State.Toggled = nk_checkbox_text(C, Source.Text.c_str(), Source.Text.size(), &State.Value);
				if (State.Toggled && !Source.Bind.empty())
					SetGlobal(Source.Bind, State.Value ? "1" : "0");

				return State.Toggled;
			}
			void Checkbox::BuildEnd(nk_context* C)
			{
			}
			void Checkbox::BuildStyle(nk_context* C, nk_style* S)
			{
				memcpy(&C->style.checkbox, S ? &S->checkbox : Style, sizeof(nk_style_toggle));
				BuildFont(C, S);
			}
			float Checkbox::GetWidth()
			{
				return 0;
			}
			float Checkbox::GetHeight()
			{
				return 0;
			}
			std::string Checkbox::GetType()
			{
				return "checkbox";
			}
			void Checkbox::SetValue(bool Value)
			{
				State.Value = (int)Value;
			}
			bool Checkbox::GetValue()
			{
				return State.Value > 0;
			}
			bool Checkbox::IsToggled()
			{
				return State.Toggled;
			}

			Radio::Radio(Context* View) : Widget(View)
			{
				Style = (nk_style_toggle*)malloc(sizeof(nk_style_toggle));
				memset(Style, 0, sizeof(nk_style_toggle));

				State.Value = 0;

				Bind("bind", [this]()
				{
					Source.Bind = GetModel("bind");
					if (Source.Bind.empty())
						return;

					std::string* Result = GetGlobal(Source.Bind);
					if (Result != nullptr)
						State.Value = (int)(*Result == "1" || *Result == "true");
				});
				Bind("text", [this]()
				{
					Source.Text = GetString("text", "");
					Rest::Stroke(&Source.Text).ReplaceOf("\r\n", "");
				});
				Bind("normal", [this]() { Style->normal = CastV4Item(Content, GetString("normal", "100 100 100 255")); });
				Bind("hover", [this]() { Style->hover = CastV4Item(Content, GetString("hover", "120 120 120 255")); });
				Bind("active", [this]() { Style->active = CastV4Item(Content, GetString("active", "120 120 120 255")); });
				Bind("cursor-normal", [this]() { Style->cursor_normal = CastV4Item(Content, GetString("cursor-normal", "45 45 45 255")); });
				Bind("cursor-hover", [this]() { Style->cursor_hover = CastV4Item(Content, GetString("cursor-hover", "45 45 45 255")); });
				Bind("border", [this]() { Style->border_color = CastV4(GetV4("border", { 0, 0, 0, 0 })); });
				Bind("text-background", [this]() { Style->text_background = CastV4(GetV4("text-background", { 45, 45, 45, 255 })); });
				Bind("text-normal", [this]() { Style->text_normal = CastV4(GetV4("text-normal", { 175, 175, 175, 255 })); });
				Bind("text-hover", [this]() { Style->text_hover = CastV4(GetV4("text-hover", { 175, 175, 175, 255 })); });
				Bind("text-active", [this]() { Style->text_active = CastV4(GetV4("text-active", { 175, 175, 175, 255 })); });
				Bind("padding", [this]() { Style->padding = CastV2(GetV2("padding", { 3.0f, 3.0f })); });
				Bind("touch-padding", [this]() { Style->touch_padding = CastV2(GetV2("touch-padding", { 0, 0 })); });
				Bind("text-align", [this]() { Style->text_alignment = CastTextAlign(GetString("text-align", "middle center")); });
				Bind("border-size", [this]() { Style->border = GetFloat("border-size", 0.0f); });
				Bind("spacing", [this]() { Style->spacing = GetFloat("spacing", 4.0f); });
			}
			Radio::~Radio()
			{
				free(Style);
			}
			bool Radio::BuildBegin(nk_context* C)
			{
				State.Toggled = nk_radio_text(C, Source.Text.c_str(), Source.Text.size(), &State.Value);
				if (State.Toggled && !Source.Bind.empty())
					SetGlobal(Source.Bind, State.Value ? "1" : "0");

				return State.Toggled;
			}
			void Radio::BuildEnd(nk_context* C)
			{
			}
			void Radio::BuildStyle(nk_context* C, nk_style* S)
			{
				memcpy(&C->style.checkbox, S ? &S->checkbox : Style, sizeof(nk_style_toggle));
				BuildFont(C, S);
			}
			float Radio::GetWidth()
			{
				return 0;
			}
			float Radio::GetHeight()
			{
				return 0;
			}
			std::string Radio::GetType()
			{
				return "radio";
			}
			void Radio::SetValue(bool Value)
			{
				State.Value = (int)Value;
			}
			bool Radio::GetValue()
			{
				return State.Value > 0;
			}
			bool Radio::IsToggled()
			{
				return State.Toggled;
			}

			Selectable::Selectable(Context* View) : Widget(View)
			{
				Style = (nk_style_selectable*)malloc(sizeof(nk_style_selectable));
				memset(Style, 0, sizeof(nk_style_selectable));

				State.Value = 0;
				Source.Image = nullptr;

				Bind("type", [this]()
				{
					std::string Type = GetString("type", "text");
					if (Type == "text")
						Source.Type = ButtonType_Text;
					else if (Type == "symbol")
						Source.Type = ButtonType_Symbol;
					else if (Type == "image")
						Source.Type = ButtonType_Image;
				});
				Bind("bind", [this]()
				{
					Source.Bind = GetModel("bind");
					if (Source.Bind.empty())
						return;

					std::string* Result = GetGlobal(Source.Bind);
					if (Result != nullptr)
						State.Value = (int)(*Result == "1" || *Result == "true");
				});
				Bind("text", [this]()
				{
					Source.Text = GetString("text", "");
					Rest::Stroke(&Source.Text).ReplaceOf("\r\n", "");
				});
				Bind("image", [this]() { Source.Image = Content->Load<Graphics::Texture2D>(GetString("image", ""), nullptr); });
				Bind("symbol", [this]() { Source.Symbol = CastSymbol(GetString("symbol", "none")); });
				Bind("image-align", [this]() { Source.Align = CastTextAlign(GetString("image-align", "left")); });
				Bind("normal", [this]() { Style->normal = CastV4Item(Content, GetString("normal", "45 45 45 255")); });
				Bind("hover", [this]() { Style->hover = CastV4Item(Content, GetString("hover", "45 45 45 255")); });
				Bind("pressed", [this]() { Style->pressed = CastV4Item(Content, GetString("pressed", "45 45 45 255")); });
				Bind("normal-active", [this]() { Style->normal_active = CastV4Item(Content, GetString("normal-active", "35 35 35 255")); });
				Bind("hover-active", [this]() { Style->hover_active = CastV4Item(Content, GetString("hover-active", "35 35 35 255")); });
				Bind("pressed-active", [this]() { Style->pressed_active = CastV4Item(Content, GetString("pressed-active", "35 35 35 255")); });
				Bind("text-background", [this]() { Style->text_background = CastV4(GetV4("text-background", { 0, 0, 0, 0 })); });
				Bind("text-normal", [this]() { Style->text_normal = CastV4(GetV4("text-normal", { 175, 175, 175, 255 })); });
				Bind("text-hover", [this]() { Style->text_hover = CastV4(GetV4("text-hover", { 175, 175, 175, 255 })); });
				Bind("text-pressed", [this]() { Style->text_pressed = CastV4(GetV4("text-pressed", { 175, 175, 175, 255 })); });
				Bind("text-normal-active", [this]() { Style->text_normal_active = CastV4(GetV4("text-normal-active", { 175, 175, 175, 255 })); });
				Bind("text-hover-active", [this]() { Style->text_hover_active = CastV4(GetV4("text-hover-active", { 175, 175, 175, 255 })); });
				Bind("text-pressed-active", [this]() { Style->text_pressed_active = CastV4(GetV4("text-pressed-active", { 175, 175, 175, 255 })); });
				Bind("padding", [this]() { Style->padding = CastV2(GetV2("padding", { 2.0f, 2.0f })); });
				Bind("image-padding", [this]() { Style->image_padding = CastV2(GetV2("image-padding", { 2.0f, 2.0f })); });
				Bind("touch-padding", [this]() { Style->touch_padding = CastV2(GetV2("touch-padding", { 0, 0 })); });
				Bind("text-align", [this]() { Style->text_alignment = CastTextAlign(GetString("text-align", "")); });
				Bind("border-radius", [this]() { Style->rounding = GetFloat("border-radius", 0.0f); });
			}
			Selectable::~Selectable()
			{
				free(Style);
			}
			bool Selectable::BuildBegin(nk_context* C)
			{
				switch (Source.Type)
				{
					case ButtonType_Symbol:
					case ButtonType_Symbol_Text:
						State.Clicked = nk_selectable_symbol_text(C, (nk_symbol_type)Source.Symbol, Source.Text.c_str(), Source.Text.size(), Source.Align, &State.Value);
						break;
					case ButtonType_Image:
					case ButtonType_Image_Text:
						State.Clicked = nk_selectable_image_text(C, nk_image_auto(Source.Image), Source.Text.c_str(), Source.Text.size(), Source.Align, &State.Value);
						break;
					case ButtonType_Text:
					case ButtonType_Color:
					default:
						State.Clicked = nk_selectable_text(C, Source.Text.c_str(), Source.Text.size(), Source.Align, &State.Value);
						break;
				}

				if (State.Clicked && !Source.Bind.empty())
					SetGlobal(Source.Bind, State.Value ? "1" : "0");

				return State.Clicked;
			}
			void Selectable::BuildEnd(nk_context* C)
			{
			}
			void Selectable::BuildStyle(nk_context* C, nk_style* S)
			{
				memcpy(&C->style.selectable, S ? &S->selectable : Style, sizeof(nk_style_selectable));
				BuildFont(C, S);
			}
			float Selectable::GetWidth()
			{
				return 0;
			}
			float Selectable::GetHeight()
			{
				return 0;
			}
			std::string Selectable::GetType()
			{
				return "selectable";
			}
			void Selectable::SetValue(bool Value)
			{
				State.Value = (int)Value;
			}
			bool Selectable::GetValue()
			{
				return State.Value > 0;
			}
			bool Selectable::IsClicked()
			{
				return State.Clicked;
			}

			Slider::Slider(Context* View) : Widget(View)
			{
				Style = (nk_style_slider*)malloc(sizeof(nk_style_slider));
				memset(Style, 0, sizeof(nk_style_slider));

				State.Value = 0.0f;

				Bind("bind", [this]()
				{
					Source.Bind = GetModel("bind");
					if (Source.Bind.empty())
						return;

					std::string* Result = GetGlobal(Source.Bind);
					if (!Result)
						return;

					Rest::Stroke Number(Result);
					if (Number.HasNumber())
						State.Value = Number.ToFloat();
				});
				Bind("min", [this]() { Source.Min = GetFloat("min", 0.0f); });
				Bind("max", [this]() { Source.Max = GetFloat("max", 1.0f); });
				Bind("step", [this]() { Source.Step = GetFloat("step", 1.0f); });
				Bind("decimal", [this]() { Source.Decimal = GetBoolean("decimal", false); });
				Bind("normal", [this]() { Style->normal = CastV4Item(Content, GetString("normal", "")); });
				Bind("hover", [this]() { Style->hover = CastV4Item(Content, GetString("hover", "")); });
				Bind("active", [this]() { Style->active = CastV4Item(Content, GetString("active", "")); });
				Bind("bar-normal", [this]() { Style->bar_normal = CastV4(GetV4("bar-normal", { 38, 38, 38, 0 })); });
				Bind("bar-hover", [this]() { Style->bar_hover = CastV4(GetV4("bar-hover", { 38, 38, 38, 0 })); });
				Bind("bar-active", [this]() { Style->bar_active = CastV4(GetV4("bar-active", { 38, 38, 38, 0 })); });
				Bind("bar-filled", [this]() { Style->bar_filled = CastV4(GetV4("bar-filled", { 100, 100, 100, 0 })); });
				Bind("cursor-normal", [this]() { Style->cursor_normal = CastV4Item(Content, GetString("cursor-normal", "100 100 100 255")); });
				Bind("cursor-hover", [this]() { Style->cursor_hover = CastV4Item(Content, GetString("cursor-hover", "120 120 120 255")); });
				Bind("cursor-active", [this]() { Style->cursor_active = CastV4Item(Content, GetString("cursor-active", "150 150 150 255")); });
				Bind("inc-symbol", [this]() { Style->inc_symbol = CastSymbol(GetString("inc-symbol", "triangle-right")); });
				Bind("dec-symbol", [this]() { Style->dec_symbol = CastSymbol(GetString("dec-symbol", "triangle-left")); });
				Bind("cursor-size", [this]() { Style->cursor_size = CastV2(GetV2("cursor-size", { 16.0f, 16.0f })); });
				Bind("padding", [this]() { Style->padding = CastV2(GetV2("padding", { 2.0f, 2.0f })); });
				Bind("spacing", [this]() { Style->spacing = CastV2(GetV2("spacing", { 2.0f, 2.0f })); });
				Bind("buttons", [this]() { Style->show_buttons = GetBoolean("buttons", false); });
				Bind("bar-height", [this]() { Style->bar_height = GetFloat("bar-height", 0.0f); });
				Bind("border-size", [this]() { Style->border = GetFloat("border-size", 0.0f); });
				Bind("border-radius", [this]() { Style->rounding = GetFloat("border-radius", 0.0f); });
				Bind("inc-normal", [this]() { Style->inc_button.normal = CastV4Item(Content, GetString("inc-normal", "40 40 40 255")); });
				Bind("inc-hover", [this]() { Style->inc_button.hover = CastV4Item(Content, GetString("inc-hover", "42 42 42 255")); });
				Bind("inc-active", [this]() { Style->inc_button.active = CastV4Item(Content, GetString("inc-active", "44 44 44 255")); });
				Bind("inc-border", [this]() { Style->inc_button.border_color = CastV4(GetV4("inc-border", { 65, 65, 65, 255 })); });
				Bind("inc-text-background", [this]() { Style->inc_button.text_background = CastV4(GetV4("inc-text-background", { 40, 40, 40, 255 })); });
				Bind("inc-text-normal", [this]() { Style->inc_button.text_normal = CastV4(GetV4("inc-text-normal", { 175, 175, 175, 255 })); });
				Bind("inc-text-hover", [this]() { Style->inc_button.text_hover = CastV4(GetV4("inc-text-hover", { 175, 175, 175, 255 })); });
				Bind("inc-text-active", [this]() { Style->inc_button.text_active = CastV4(GetV4("inc-text-active", { 175, 175, 175, 255 })); });
				Bind("inc-padding", [this]() { Style->inc_button.padding = CastV2(GetV2("inc-padding", { 8.0f, 8.0f })); });
				Bind("inc-image-padding", [this]() { Style->inc_button.image_padding = CastV2(GetV2("inc-image-padding", { 0, 0 })); });
				Bind("inc-touch-padding", [this]() { Style->inc_button.touch_padding = CastV2(GetV2("inc-touch-padding", { 0, 0 })); });
				Bind("inc-text-align", [this]() { Style->inc_button.text_alignment = CastTextAlign(GetString("inc-text-align", "middle center")); });
				Bind("inc-border-size", [this]() { Style->inc_button.border = GetFloat("inc-border-size", 1.0f); });
				Bind("inc-border-radius", [this]() { Style->inc_button.rounding = GetFloat("inc-border-radius", 0.0f); });
				Bind("dec-normal", [this]() { Style->dec_button.normal = CastV4Item(Content, GetString("dec-normal", "40 40 40 255")); });
				Bind("dec-hover", [this]() { Style->dec_button.hover = CastV4Item(Content, GetString("dec-hover", "42 42 42 255")); });
				Bind("dec-active", [this]() { Style->dec_button.active = CastV4Item(Content, GetString("dec-active", "44 44 44 255")); });
				Bind("dec-border", [this]() { Style->dec_button.border_color = CastV4(GetV4("dec-border", { 65, 65, 65, 255 })); });
				Bind("dec-text-background", [this]() { Style->dec_button.text_background = CastV4(GetV4("dec-text-background", { 40, 40, 40, 255 })); });
				Bind("dec-text-normal", [this]() { Style->dec_button.text_normal = CastV4(GetV4("dec-text-normal", { 175, 175, 175, 255 })); });
				Bind("dec-text-hover", [this]() { Style->dec_button.text_hover = CastV4(GetV4("dec-text-hover", { 175, 175, 175, 255 })); });
				Bind("dec-text-active", [this]() { Style->dec_button.text_active = CastV4(GetV4("dec-text-active", { 175, 175, 175, 255 })); });
				Bind("dec-padding", [this]() { Style->dec_button.padding = CastV2(GetV2("dec-padding", { 8.0f, 8.0f })); });
				Bind("dec-image-padding", [this]() { Style->dec_button.image_padding = CastV2(GetV2("dec-image-padding", { 0, 0 })); });
				Bind("dec-touch-padding", [this]() { Style->dec_button.touch_padding = CastV2(GetV2("dec-touch-padding", { 0, 0 })); });
				Bind("dec-text-align", [this]() { Style->dec_button.text_alignment = CastTextAlign(GetString("dec-text-align", "middle center")); });
				Bind("dec-border-size", [this]() { Style->dec_button.border = GetFloat("dec-border-size", 1.0f); });
				Bind("dec-border-radius", [this]() { Style->dec_button.rounding = GetFloat("dec-border-radius", 0.0f); });
			}
			Slider::~Slider()
			{
				free(Style);
			}
			bool Slider::BuildBegin(nk_context* C)
			{
				if (Source.Decimal)
				{
					int Value = (int)State.Value;
					State.Clicked = nk_slider_int(C, (int)Source.Min, &Value, (int)Source.Max, (int)Source.Step);
					State.Value = (float)Value;
				}
				else
					State.Clicked = nk_slider_float(C, Source.Min, &State.Value, Source.Max, Source.Step);

				if (State.Clicked && !Source.Bind.empty())
					SetGlobal(Source.Bind, Source.Decimal ? std::to_string((int)State.Value) : std::to_string(State.Value));

				return State.Clicked;
			}
			void Slider::BuildEnd(nk_context* C)
			{
			}
			void Slider::BuildStyle(nk_context* C, nk_style* S)
			{
				memcpy(&C->style.slider, S ? &S->slider : Style, sizeof(nk_style_slider));
				BuildFont(C, S);
			}
			float Slider::GetWidth()
			{
				return 0;
			}
			float Slider::GetHeight()
			{
				return 0;
			}
			std::string Slider::GetType()
			{
				return "slider";
			}
			void Slider::SetValue(float Value)
			{
				State.Value = Value;
			}
			float Slider::GetValue()
			{
				return State.Value;
			}
			bool Slider::IsClicked()
			{
				return State.Clicked;
			}

			Progress::Progress(Context* View) : Widget(View)
			{
				Style = (nk_style_progress*)malloc(sizeof(nk_style_progress));
				memset(Style, 0, sizeof(nk_style_progress));

				State.Value = 0.0f;

				Bind("bind", [this]()
				{
					Source.Bind = GetModel("bind");
					if (Source.Bind.empty())
						return;

					std::string* Result = GetGlobal(Source.Bind);
					if (!Result)
						return;

					Rest::Stroke Number(Result);
					if (Number.HasInteger())
						State.Value = Number.ToUInt();
				});
				Bind("max", [this]() { Source.Max = (uintptr_t)GetInteger("max", 100); });
				Bind("changeable", [this]() { Source.Changeable = GetBoolean("changeable", false); });
				Bind("normal", [this]() { Style->normal = CastV4Item(Content, GetString("normal", "38 38 38 255")); });
				Bind("hover", [this]() { Style->hover = CastV4Item(Content, GetString("hover", "38 38 38 255")); });
				Bind("active", [this]() { Style->active = CastV4Item(Content, GetString("active", "38 38 38 255")); });
				Bind("cursor-normal", [this]() { Style->cursor_normal = CastV4Item(Content, GetString("cursor-normal", "100 100 100 255")); });
				Bind("cursor-hover", [this]() { Style->cursor_hover = CastV4Item(Content, GetString("cursor-hover", "120 120 120 255")); });
				Bind("cursor-active", [this]() { Style->cursor_active = CastV4Item(Content, GetString("cursor-active", "150 150 150 255")); });
				Bind("cursor-border", [this]() { Style->cursor_border_color = CastV4(GetV4("cursor-border", { 0, 0, 0, 0 })); });
				Bind("border", [this]() { Style->border_color = CastV4(GetV4("border", { 0, 0, 0, 0 })); });
				Bind("padding", [this]() { Style->padding = CastV2(GetV2("padding", { 4.0f, 4.0f })); });
				Bind("border-size", [this]() { Style->border = GetFloat("border-size", 0.0f); });
				Bind("border-radius", [this]() { Style->rounding = GetFloat("border-radius", 0.0f); });
				Bind("cursor-border-size", [this]() { Style->cursor_border = GetFloat("cursor-border-size", 0.0f); });
				Bind("cursor-border-radius", [this]() { Style->cursor_rounding = GetFloat("cursor-border-radius", 0.0f); });
			}
			Progress::~Progress()
			{
				free(Style);
			}
			bool Progress::BuildBegin(nk_context* C)
			{
				State.Clicked = nk_progress(C, &State.Value, Source.Max, Source.Changeable);
				if (State.Clicked && !Source.Bind.empty())
					SetGlobal(Source.Bind, std::to_string(State.Value));

				return State.Clicked;
			}
			void Progress::BuildEnd(nk_context* C)
			{
			}
			void Progress::BuildStyle(nk_context* C, nk_style* S)
			{
				memcpy(&C->style.progress, S ? &S->progress : Style, sizeof(nk_style_progress));
				BuildFont(C, S);
			}
			float Progress::GetWidth()
			{
				return 0;
			}
			float Progress::GetHeight()
			{
				return 0;
			}
			std::string Progress::GetType()
			{
				return "progress";
			}
			void Progress::SetValue(uintptr_t Value)
			{
				State.Value = Value;
			}
			uintptr_t Progress::GetValue()
			{
				return State.Value;
			}
			bool Progress::IsClicked()
			{
				return State.Clicked;
			}

			ColorPicker::ColorPicker(Context* View) : Widget(View)
			{
				State.Value.SetW(255);

				Bind("bind", [this]()
				{
					Source.Bind = GetModel("bind");
					if (Source.Bind.empty())
						return;

					std::string* Result = GetGlobal(Source.Bind);
					if (!Result)
						return;

					std::vector<std::string> Args = Rest::Stroke(Result).SplitMax(' ', 4);
					if (Args.size() > 0)
						State.Value.X = GetFloatRelative(Args[0]);

					if (Args.size() > 1)
						State.Value.Y = GetFloatRelative(Args[1]);

					if (Args.size() > 2)
						State.Value.Z = GetFloatRelative(Args[2]);

					if (Args.size() > 3)
						State.Value.W = GetFloatRelative(Args[3]);
				});
				Bind("alpha", [this]() { Source.Alpha = GetBoolean("alpha", false); });
			}
			ColorPicker::~ColorPicker()
			{
			}
			bool ColorPicker::BuildBegin(nk_context* C)
			{
				nk_colorf Color = nk_color_cf(CastV4(State.Value));
				State.Clicked = nk_color_pick(C, &Color, Source.Alpha ? NK_RGBA : NK_RGB);

				if (!State.Clicked)
					return false;

				nk_color Cast = nk_rgba_cf(Color);
				State.Value.X = (float)Cast.r;
				State.Value.Y = (float)Cast.g;
				State.Value.Z = (float)Cast.b;
				State.Value.W = (float)Cast.a;

				if (Source.Bind.empty())
					return true;
				
				SetGlobal(Source.Bind, Rest::Form("%f %f %f %f", State.Value.X, State.Value.Y, State.Value.Z, State.Value.W).R());
				return true;
			}
			void ColorPicker::BuildEnd(nk_context* C)
			{
			}
			void ColorPicker::BuildStyle(nk_context* C, nk_style* S)
			{
				BuildFont(C, S);
			}
			float ColorPicker::GetWidth()
			{
				return 0;
			}
			float ColorPicker::GetHeight()
			{
				return 0;
			}
			std::string ColorPicker::GetType()
			{
				return "color-picker";
			}
			void ColorPicker::SetValue(const Compute::Vector4& Value)
			{
				State.Value = Value;
			}
			Compute::Vector4 ColorPicker::GetValue()
			{
				return State.Value;
			}
			bool ColorPicker::IsClicked()
			{
				return State.Clicked;
			}

			Property::Property(Context* View) : Widget(View)
			{
				Style = (nk_style_property*)malloc(sizeof(nk_style_property));
				memset(Style, 0, sizeof(nk_style_property));

				State.Value = 0.0f;

				Bind("bind", [this]()
				{
					Source.Bind = GetModel("bind");
					if (Source.Bind.empty())
						return;

					std::string* Result = GetGlobal(Source.Bind);
					if (!Result)
						return;

					Rest::Stroke Number(Result);
					if (Number.HasNumber())
						State.Value = Number.ToFloat64();
				});
				Bind("text", [this]()
				{
					Source.Text = GetString("text", "");
					Rest::Stroke(&Source.Text).ReplaceOf("\r\n", "");
				});
				Bind("min", [this]() { Source.Min = GetFloat("min", 0.0f); });
				Bind("max", [this]() { Source.Max = GetFloat("max", 1.0f); });
				Bind("step", [this]() { Source.Step = GetFloat("step", 1.0f); });
				Bind("ppi", [this]() { Source.PPI = GetFloat("ppi", 0.05f); });
				Bind("decimal", [this]() { Source.Decimal = GetBoolean("decimal", false); });
				Bind("normal", [this]() { Style->normal = CastV4Item(Content, GetString("normal", "38 38 38 255")); });
				Bind("hover", [this]() { Style->hover = CastV4Item(Content, GetString("hover", "38 38 38 255")); });
				Bind("active", [this]() { Style->active = CastV4Item(Content, GetString("active", "38 38 38 255")); });
				Bind("border", [this]() { Style->border_color = CastV4(GetV4("border", { 65, 65, 65, 255 })); });
				Bind("text-normal", [this]() { Style->label_normal = CastV4(GetV4("text-normal", { 175, 175, 175, 255 })); });
				Bind("text-hover", [this]() { Style->label_hover = CastV4(GetV4("text-hover", { 175, 175, 175, 255 })); });
				Bind("text-active", [this]() { Style->label_active = CastV4(GetV4("text-active", { 175, 175, 175, 255 })); });
				Bind("inc-symbol", [this]() { Style->sym_right = CastSymbol(GetString("inc-symbol", "triangle-right")); });
				Bind("dec-symbol", [this]() { Style->sym_left = CastSymbol(GetString("dec-symbol", "triangle-left")); });		
				Bind("padding", [this]() { Style->padding = CastV2(GetV2("padding", { 4.0f, 4.0f })); });
				Bind("border-size", [this]() { Style->border = GetFloat("border-size", 1.0f); });
				Bind("border-radius", [this]() { Style->rounding = GetFloat("border-radius", 10.0f); });
				Bind("inc-normal", [this]() { Style->inc_button.normal = CastV4Item(Content, GetString("inc-normal", "38 38 38 255")); });
				Bind("inc-hover", [this]() { Style->inc_button.hover = CastV4Item(Content, GetString("inc-hover", "38 38 38 255")); });
				Bind("inc-active", [this]() { Style->inc_button.active = CastV4Item(Content, GetString("inc-active", "38 38 38 255")); });
				Bind("inc-border", [this]() { Style->inc_button.border_color = CastV4(GetV4("inc-border", { 0, 0, 0, 0 })); });
				Bind("inc-text-background", [this]() { Style->inc_button.text_background = CastV4(GetV4("inc-text-background", { 38, 38, 38, 255 })); });
				Bind("inc-text-normal", [this]() { Style->inc_button.text_normal = CastV4(GetV4("inc-text-normal", { 175, 175, 175, 255 })); });
				Bind("inc-text-hover", [this]() { Style->inc_button.text_hover = CastV4(GetV4("inc-text-hover", { 175, 175, 175, 255 })); });
				Bind("inc-text-active", [this]() { Style->inc_button.text_active = CastV4(GetV4("inc-text-active", { 175, 175, 175, 255 })); });
				Bind("inc-padding", [this]() { Style->inc_button.padding = CastV2(GetV2("inc-padding", { 0.0f, 0.0f })); });
				Bind("inc-image-padding", [this]() { Style->inc_button.image_padding = CastV2(GetV2("inc-image-padding", { 0, 0 })); });
				Bind("inc-touch-padding", [this]() { Style->inc_button.touch_padding = CastV2(GetV2("inc-touch-padding", { 0, 0 })); });
				Bind("inc-text-align", [this]() { Style->inc_button.text_alignment = CastTextAlign(GetString("inc-text-align", "middle center")); });
				Bind("inc-border-size", [this]() { Style->inc_button.border = GetFloat("inc-border-size", 0.0f); });
				Bind("inc-border-radius", [this]() { Style->inc_button.rounding = GetFloat("inc-border-radius", 0.0f); });
				Bind("dec-normal", [this]() { Style->dec_button.normal = CastV4Item(Content, GetString("dec-normal", "38 38 38 255")); });
				Bind("dec-hover", [this]() { Style->dec_button.hover = CastV4Item(Content, GetString("dec-hover", "38 38 38 255")); });
				Bind("dec-active", [this]() { Style->dec_button.active = CastV4Item(Content, GetString("dec-active", "38 38 38 255")); });
				Bind("dec-border", [this]() { Style->dec_button.border_color = CastV4(GetV4("dec-border", { 0, 0, 0, 0 })); });
				Bind("dec-text-background", [this]() { Style->dec_button.text_background = CastV4(GetV4("dec-text-background", { 38, 38, 38, 255 })); });
				Bind("dec-text-normal", [this]() { Style->dec_button.text_normal = CastV4(GetV4("dec-text-normal", { 175, 175, 175, 255 })); });
				Bind("dec-text-hover", [this]() { Style->dec_button.text_hover = CastV4(GetV4("dec-text-hover", { 175, 175, 175, 255 })); });
				Bind("dec-text-active", [this]() { Style->dec_button.text_active = CastV4(GetV4("dec-text-active", { 175, 175, 175, 255 })); });
				Bind("dec-padding", [this]() { Style->dec_button.padding = CastV2(GetV2("dec-padding", { 0.0f, 0.0f })); });
				Bind("dec-image-padding", [this]() { Style->dec_button.image_padding = CastV2(GetV2("dec-image-padding", { 0, 0 })); });
				Bind("dec-touch-padding", [this]() { Style->dec_button.touch_padding = CastV2(GetV2("dec-touch-padding", { 0, 0 })); });
				Bind("dec-text-align", [this]() { Style->dec_button.text_alignment = CastTextAlign(GetString("dec-text-align", "middle center")); });
				Bind("dec-border-size", [this]() { Style->dec_button.border = GetFloat("dec-border-size", 0.0f); });
				Bind("dec-border-radius", [this]() { Style->dec_button.rounding = GetFloat("dec-border-radius", 0.0f); });		
				Bind("edit-normal", [this]() { Style->edit.normal = CastV4Item(Content, GetString("edit-normal", "38 38 38 255")); });
				Bind("edit-hover", [this]() { Style->edit.hover = CastV4Item(Content, GetString("edit-hover", "38 38 38 255")); });
				Bind("edit-active", [this]() { Style->edit.active = CastV4Item(Content, GetString("edit-active", "38 38 38 255")); });
				Bind("edit-border", [this]() { Style->edit.border_color = CastV4(GetV4("edit-active", { 0, 0, 0, 0 })); });
				Bind("edit-cursor-normal", [this]() { Style->edit.cursor_normal = CastV4(GetV4("edit-cursor-normal", { 175, 175, 175, 255 })); });
				Bind("edit-cursor-hover", [this]() { Style->edit.cursor_hover = CastV4(GetV4("edit-cursor-hover", { 175, 175, 175, 255 })); });
				Bind("edit-cursor-text-normal", [this]() { Style->edit.cursor_text_normal = CastV4(GetV4("edit-cursor-text-normal", { 38, 38, 38, 255 })); });
				Bind("edit-cursor-text-hover", [this]() { Style->edit.cursor_text_hover = CastV4(GetV4("edit-cursor-text-hover", { 38, 38, 38, 255 })); });
				Bind("edit-text-normal", [this]() { Style->edit.text_normal = CastV4(GetV4("edit-text-normal", { 175, 175, 175, 255 })); });
				Bind("edit-text-hover", [this]() { Style->edit.text_hover = CastV4(GetV4("edit-text-hover", { 175, 175, 175, 255 })); });
				Bind("edit-text-active", [this]() { Style->edit.text_active = CastV4(GetV4("edit-text-active", { 175, 175, 175, 255 })); });
				Bind("edit-select-normal", [this]() { Style->edit.selected_normal = CastV4(GetV4("edit-select-normal", { 175, 175, 175, 255 })); });
				Bind("edit-select-hover", [this]() { Style->edit.selected_hover = CastV4(GetV4("edit-select-hover", { 175, 175, 175, 255 })); });
				Bind("edit-select-text-normal", [this]() { Style->edit.selected_text_normal = CastV4(GetV4("edit-select-text-normal", { 38, 38, 38, 255 })); });
				Bind("edit-select-text-hover", [this]() { Style->edit.selected_text_hover = CastV4(GetV4("edit-select-text-hover", { 38, 38, 38, 255 })); });
				Bind("edit-cursor-size", [this]() { Style->edit.cursor_size = GetFloat("edit-cursor-size", 8); });
				Bind("edit-border-size", [this]() { Style->edit.border = GetFloat("edit-border-size", 0); });
				Bind("edit-border-radius", [this]() { Style->edit.rounding = GetFloat("edit-border-radius", 0); });
			}
			Property::~Property()
			{
				free(Style);
			}
			bool Property::BuildBegin(nk_context* C)
			{
				double Last = State.Value;
				if (Source.Decimal)
				{
					int Value = (int)State.Value;
					nk_property_int(C, Source.Text.c_str(), (int)Source.Min, &Value, (int)Source.Max, (int)Source.Step, Source.PPI);
					State.Value = (double)Value;
				}
				else
					nk_property_double(C, Source.Text.c_str(), Source.Min, &State.Value, Source.Max, Source.Step, Source.PPI);

				if (Last != State.Value && !Source.Bind.empty())
					SetGlobal(Source.Bind, Source.Decimal ? std::to_string((int)State.Value) : std::to_string(State.Value));

				return Last != State.Value;
			}
			void Property::BuildEnd(nk_context* C)
			{
			}
			void Property::BuildStyle(nk_context* C, nk_style* S)
			{
				memcpy(&C->style.property, S ? &S->property : Style, sizeof(nk_style_property));
				BuildFont(C, S);
			}
			float Property::GetWidth()
			{
				return 0;
			}
			float Property::GetHeight()
			{
				return 0;
			}
			std::string Property::GetType()
			{
				return "property";
			}
			void Property::SetValue(double Value)
			{
				State.Value = Value;
			}
			double Property::GetValue()
			{
				return State.Value;
			}

			Edit::Edit(Context* View) : Widget(View)
			{
				Style = (nk_style_edit*)malloc(sizeof(nk_style_edit));
				memset(Style, 0, sizeof(nk_style_edit));

				Bind("bind", [this]()
				{
					Source.Bind = GetModel("bind");
					if (Source.Bind.empty())
						return;

					std::string* Result = GetGlobal(Source.Bind);
					if (Result != nullptr)
						State.Value = *Result;
				});
				Bind("max-size", [this]() { Source.MaxSize = GetInteger("max-size", 1024); });
				Bind("type", [this]()
				{
					std::string Type = GetString("type", "text");
					if (Type == "text")
						Source.Type = EditType_Default;
					else if (Type == "ascii")
						Source.Type = EditType_ASCII;
					else if (Type == "float")
						Source.Type = EditType_Float;
					else if (Type == "decimal")
						Source.Type = EditType_Decimal;
					else if (Type == "hex")
						Source.Type = EditType_Hex;
					else if (Type == "oct")
						Source.Type = EditType_Oct;
					else if (Type == "binary")
						Source.Type = EditType_Binary;
					else if (Type == "password")
						Source.Type = EditType_Password;
				});
				Bind("mode", [this]()
				{
					std::string Type = GetString("mode", "field");
					if (Type == "field")
						Source.Flags = NK_EDIT_FIELD | NK_EDIT_SIG_ENTER;
					else if (Type == "simple")
						Source.Flags = NK_EDIT_SIMPLE | NK_EDIT_SIG_ENTER;
					else if (Type == "box")
						Source.Flags = NK_EDIT_BOX;
					else if (Type == "editor")
						Source.Flags = NK_EDIT_EDITOR;
				});
				Bind("read-only", [this]()
				{
					if (GetBoolean("read-only", false))
						Source.Flags |= NK_EDIT_READ_ONLY;
					else
						Source.Flags &= ~NK_EDIT_READ_ONLY;
				});
				Bind("text", [this]()
				{
					Source.Text = GetString("text", "");
					Rest::Stroke(&Source.Text).ReplaceOf("\r\n", "");
				});
				Bind("normal", [this]() { Style->normal = CastV4Item(Content, GetString("normal", "38 38 38 255")); });
				Bind("hover", [this]() { Style->hover = CastV4Item(Content, GetString("hover", "38 38 38 255")); });
				Bind("active", [this]() { Style->active = CastV4Item(Content, GetString("active", "38 38 38 255")); });
				Bind("cursor-normal", [this]() { Style->cursor_normal = CastV4(GetV4("cursor-normal", { 175, 175, 175, 255 })); });
				Bind("cursor-hover", [this]() { Style->cursor_hover = CastV4(GetV4("cursor-hover", { 175, 175, 175, 255 })); });
				Bind("cursor-text-normal", [this]() { Style->cursor_text_normal = CastV4(GetV4("cursor-text-normal", { 38, 38, 38, 255 })); });
				Bind("cursor-text-hover", [this]() { Style->cursor_text_hover = CastV4(GetV4("cursor-text-hover", { 38, 38, 38, 255 })); });
				Bind("border", [this]() { Style->border_color = CastV4(GetV4("border", { 65, 65, 65, 255 })); });
				Bind("text-normal", [this]() { Style->text_normal = CastV4(GetV4("text-normal", { 175, 175, 175, 255 })); });
				Bind("text-hover", [this]() { Style->text_hover = CastV4(GetV4("text-hover", { 175, 175, 175, 255 })); });
				Bind("text-active", [this]() { Style->text_active = CastV4(GetV4("text-active", { 175, 175, 175, 255 })); });
				Bind("select-normal", [this]() { Style->selected_normal = CastV4(GetV4("select-normal", { 175, 175, 175, 255 })); });
				Bind("select-hover", [this]() { Style->selected_hover = CastV4(GetV4("select-hover", { 175, 175, 175, 255 })); });
				Bind("scroll-size", [this]() { Style->scrollbar_size = CastV2(GetV2("scroll-size", { 10, 10 })); });
				Bind("padding", [this]() { Style->padding = CastV2(GetV2("padding", { 4, 4 })); });
				Bind("row-padding", [this]() { Style->row_padding = GetFloat("row-padding", 2); });
				Bind("cursor-size", [this]() { Style->cursor_size = GetFloat("cursor-size", 2); });
				Bind("border-size", [this]() { Style->border = GetFloat("border-size", 1); });
				Bind("border-radius", [this]() { Style->rounding = GetFloat("border-radius", 0); });
				Bind("scroll-normal", [this]() { Style->scrollbar.normal = CastV4Item(Content, GetString("scroll-normal", "40 40 40 255")); });
				Bind("scroll-hover", [this]() { Style->scrollbar.hover = CastV4Item(Content, GetString("scroll-hover", "40 40 40 255")); });
				Bind("scroll-active", [this]() { Style->scrollbar.active = CastV4Item(Content, GetString("scroll-active", "40 40 40 255")); });
				Bind("scroll-cursor-normal", [this]() { Style->scrollbar.cursor_normal = CastV4Item(Content, GetString("scroll-cursor-normal", "100 100 100 255")); });
				Bind("scroll-cursor-hover", [this]() { Style->scrollbar.cursor_hover = CastV4Item(Content, GetString("scroll-cursor-hover", "120 120 120 255")); });
				Bind("scroll-cursor-active", [this]() { Style->scrollbar.cursor_active = CastV4Item(Content, GetString("scroll-cursor-active", "150 150 150 255")); });
				Bind("scroll-inc-symbol", [this]() { Style->scrollbar.inc_symbol = CastSymbol(GetString("scroll-inc-symbol", "circle-solid")); });
				Bind("scroll-dec-symbol", [this]() { Style->scrollbar.dec_symbol = CastSymbol(GetString("scroll-dec-symbol", "circle-solid")); });
				Bind("scroll-border", [this]() { Style->scrollbar.border_color = CastV4(GetV4("scroll-border", { 40, 40, 40, 255 })); });
				Bind("scroll-cursor-border", [this]() { Style->scrollbar.cursor_border_color = CastV4(GetV4("scroll-cursor-border", { 40, 40, 40, 255 })); });
				Bind("scroll-padding", [this]() { Style->scrollbar.padding = CastV2(GetV2("scroll-padding", { 0, 0 })); });
				Bind("scroll-buttons", [this]() { Style->scrollbar.show_buttons = GetBoolean("scroll-buttons", false); });
				Bind("scroll-border-size", [this]() { Style->scrollbar.border = GetFloat("scroll-border-size", 0); });
				Bind("scroll-border-radius", [this]() { Style->scrollbar.rounding = GetFloat("scroll-border-radius", 0); });
				Bind("scroll-cursor-border-size", [this]() { Style->scrollbar.border_cursor = GetFloat("scroll-cursor-border-size", 0); });
				Bind("scroll-cursor-border-radius", [this]() { Style->scrollbar.rounding_cursor = GetFloat("scroll-cursor-border-radius", 0); });
			}
			Edit::~Edit()
			{
				free(Style);
			}
			bool Edit::BuildBegin(nk_context* C)
			{
				State.Buffer = State.Value;
				if (State.Buffer.capacity() != Source.MaxSize)
					State.Buffer.reserve(Source.MaxSize);

				int Length = State.Value.size();	
				switch (Source.Type)
				{
					case EditType_Password:
					{
						std::string Buffer;
						Buffer.reserve(Source.MaxSize);
						Buffer.resize(Length, '*');

						State.Flags = nk_edit_string(C, Source.Flags, (char*)Buffer.c_str(), &Length, (int)Buffer.capacity(), nk_filter_default);
						if (Length > (int)State.Value.size())
							State.Value.append(Buffer.substr(State.Value.size() - 1));
						else if (Length < (int)State.Value.size())
							State.Value.resize((size_t)Length);
						break;
					}
					case EditType_Default:
						State.Flags = nk_edit_string(C, Source.Flags, (char*)State.Buffer.c_str(), &Length, (int)State.Buffer.capacity(), nk_filter_default);
						break;
					case EditType_ASCII:
						State.Flags = nk_edit_string(C, Source.Flags, (char*)State.Value.c_str(), &Length, (int)State.Value.capacity(), nk_filter_ascii);
						break;
					case EditType_Float:
						State.Flags = nk_edit_string(C, Source.Flags, (char*)State.Value.c_str(), &Length, (int)State.Value.capacity(), nk_filter_float);
						break;
					case EditType_Decimal:
						State.Flags = nk_edit_string(C, Source.Flags, (char*)State.Value.c_str(), &Length, (int)State.Value.capacity(), nk_filter_decimal);
						break;
					case EditType_Hex:
						State.Flags = nk_edit_string(C, Source.Flags, (char*)State.Value.c_str(), &Length, (int)State.Value.capacity(), nk_filter_hex);
						break;
					case EditType_Oct:
						State.Flags = nk_edit_string(C, Source.Flags, (char*)State.Value.c_str(), &Length, (int)State.Value.capacity(), nk_filter_oct);
						break;
					case EditType_Binary:
						State.Flags = nk_edit_string(C, Source.Flags, (char*)State.Value.c_str(), &Length, (int)State.Value.capacity(), nk_filter_binary);
						break;
				}

				if (State.Flags & NK_EDIT_ACTIVE && Length >= 0)
					State.Value.assign(State.Buffer.c_str(), (size_t)Length);

				if ((State.Flags & NK_EDIT_SIMPLE || State.Flags & NK_EDIT_FIELD))
				{
					if (State.Flags & NK_EDIT_COMMITED && !Source.Bind.empty())
						SetGlobal(Source.Bind, State.Value);
				}
				else if (State.Flags & NK_EDIT_ACTIVE)
					SetGlobal(Source.Bind, State.Value);

				return State.Flags & NK_EDIT_ACTIVE;
			}
			void Edit::BuildEnd(nk_context* C)
			{
			}
			void Edit::BuildStyle(nk_context* C, nk_style* S)
			{
				memcpy(&C->style.edit, S ? &S->edit : Style, sizeof(nk_style_edit));
				BuildFont(C, S);
			}
			float Edit::GetWidth()
			{
				return 0;
			}
			float Edit::GetHeight()
			{
				return 0;
			}
			std::string Edit::GetType()
			{
				return "edit";
			}
			void Edit::SetValue(const std::string& Value)
			{
				State.Value = Value;
			}
			std::string Edit::GetValue()
			{
				return State.Value;
			}
			bool Edit::IsTextChanged()
			{
				return State.Flags & NK_EDIT_COMMITED;
			}
			bool Edit::IsTextActive()
			{
				return State.Flags & NK_EDIT_ACTIVE;
			}
			bool Edit::IsTextActivated()
			{
				return State.Flags & NK_EDIT_ACTIVATED;
			}
			bool Edit::IsTextDeactivated()
			{
				return State.Flags & NK_EDIT_ACTIVATED;
			}
			void Edit::Focus()
			{
				nk_edit_focus(Base->GetContext(), 0);
			}
			void Edit::Unfocus()
			{
				nk_edit_unfocus(Base->GetContext());
			}

			Combo::Combo(Context* View) : Widget(View)
			{
				Style = (nk_style_combo*)malloc(sizeof(nk_style_combo));
				memset(Style, 0, sizeof(nk_style_combo));

				Bind("bind", [this]()
				{
					Source.Bind = GetModel("bind");
					if (Source.Bind.empty())
						return;

					std::string* Result = GetGlobal(Source.Bind);
					if (!Result)
						return;

					Rest::Stroke Number(Result);
					if (Number.HasInteger())
						State.Value = Number.ToInt();
				});
				Bind("type", [this]()
				{
					std::string Type = GetString("type", "text");
					if (Type == "text")
						Source.Type = ButtonType_Text;
					else if (Type == "color")
						Source.Type = ButtonType_Color;
					else if (Type == "symbol")
						Source.Type = ButtonType_Symbol;
					else if (Type == "symbol-text")
						Source.Type = ButtonType_Symbol_Text;
					else if (Type == "image")
						Source.Type = ButtonType_Image;
					else if (Type == "image-text")
						Source.Type = ButtonType_Image_Text;
				});
				Bind("image", [this]() { Source.Image = Content->Load<Graphics::Texture2D>(GetString("image", ""), nullptr); });
				Bind("color", [this]() { Source.Color = GetV4("color", { 50, 50, 50, 255 }); });
				Bind("symbol", [this]() { Source.Symbol = CastSymbol(GetString("symbol", "none")); });
				Bind("width", [this]() { Source.Width = GetFloat("width", 120); });
				Bind("height", [this]() { Source.Height = GetFloat("height", 80); });
				Bind("normal", [this]() { Style->normal = CastV4Item(Content, GetString("normal", "45 45 45 255")); });
				Bind("hover", [this]() { Style->hover = CastV4Item(Content, GetString("hover", "45 45 45 255")); });
				Bind("active", [this]() { Style->active = CastV4Item(Content, GetString("active", "45 45 45 255")); });
				Bind("border", [this]() { Style->border_color = CastV4(GetV4("border", { 65, 65, 65, 255 })); });
				Bind("label-normal", [this]() { Style->label_normal = CastV4(GetV4("label-normal", { 175, 175, 175, 255 })); });
				Bind("label-hover", [this]() { Style->label_hover = CastV4(GetV4("label-hover", { 175, 175, 175, 255 })); });
				Bind("label-active", [this]() { Style->label_active = CastV4(GetV4("label-active", { 175, 175, 175, 255 })); });
				Bind("normal-symbol", [this]() { Style->sym_normal = CastSymbol(GetString("normal-symbol", "triangle-down")); });
				Bind("hover-symbol", [this]() { Style->sym_hover = CastSymbol(GetString("hover-symbol", "triangle-down")); });
				Bind("active-symbol", [this]() { Style->sym_active = CastSymbol(GetString("active-symbol", "triangle-down")); });
				Bind("content-padding", [this]() { Style->content_padding = CastV2(GetV2("content-padding", { 4, 4 })); });
				Bind("button-padding", [this]() { Style->button_padding = CastV2(GetV2("button-padding", { 0, 4 })); });
				Bind("spacing", [this]() { Style->spacing = CastV2(GetV2("spacing", { 4, 0 })); });
				Bind("border-size", [this]() { Style->border = GetFloat("border-size", 1); });
				Bind("border-radius", [this]() { Style->rounding = GetFloat("border-radius", 0); });
			}
			Combo::~Combo()
			{
				free(Style);
			}
			bool Combo::BuildBegin(nk_context* C)
			{
				switch (Source.Type)
				{
					case ButtonType_Color:
						State.Open = nk_combo_begin_color(C, CastV4(Source.Color), nk_vec2(Source.Width, Source.Height));
						break;
					case ButtonType_Symbol:
						State.Open = nk_combo_begin_symbol(C, (nk_symbol_type)Source.Symbol, nk_vec2(Source.Width, Source.Height));
						break;
					case ButtonType_Symbol_Text:
						State.Open = nk_combo_begin_symbol_text(C, State.Value.c_str(), State.Value.size(), (nk_symbol_type)Source.Symbol, nk_vec2(Source.Width, Source.Height));
						break;
					case ButtonType_Image:
						State.Open = nk_combo_begin_image(C, nk_image_auto(Source.Image), nk_vec2(Source.Width, Source.Height));
						break;
					case ButtonType_Image_Text:
						State.Open = nk_combo_begin_image_text(C, State.Value.c_str(), State.Value.size(), nk_image_auto(Source.Image), nk_vec2(Source.Width, Source.Height));
						break;
					case ButtonType_Text:
					default:
						State.Open = nk_combo_begin_text(C, State.Value.c_str(), State.Value.size(), nk_vec2(Source.Width, Source.Height));
						break;
				}

				return State.Open;
			}
			void Combo::BuildEnd(nk_context* C)
			{
				if (!State.Open)
					return;

				nk_combo_end(C);
			}
			void Combo::BuildStyle(nk_context* C, nk_style* S)
			{
				memcpy(&C->style.combo, S ? &S->combo : Style, sizeof(nk_style_combo));
				BuildFont(C, S);
			}
			float Combo::GetWidth()
			{
				return Source.Width;
			}
			float Combo::GetHeight()
			{
				return Source.Height;
			}
			std::string Combo::GetType()
			{
				return "combo";
			}
			void Combo::SetValue(const std::string& Value)
			{
				State.Value = Value;
				if (!Source.Bind.empty())
					SetGlobal(Source.Bind, State.Value);
			}
			std::string Combo::GetValue()
			{
				return State.Value;
			}

			ComboItem::ComboItem(Context* View) : Element(View)
			{
				State.Layer = nullptr;

				Bind("type", [this]()
				{
					std::string Type = GetString("type", "text");
					if (Type == "text")
						Source.Type = ButtonType_Text;
					else if (Type == "symbol")
						Source.Type = ButtonType_Symbol_Text;
					else if (Type == "image")
						Source.Type = ButtonType_Image_Text;
				});
				Bind("text", [this]()
				{
					Source.Text = GetString("text", "");
					Rest::Stroke(&Source.Text).ReplaceOf("\r\n", "");
				});
				Bind("image", [this]() { Source.Image = Content->Load<Graphics::Texture2D>(GetString("image", ""), nullptr); });
				Bind("symbol", [this]() { Source.Symbol = CastSymbol(GetString("symbol", "none")); });
				Bind("align", [this]() { Source.Align = CastTextAlign(GetString("align", "middle left")); });
			}
			bool ComboItem::BuildBegin(nk_context* C)
			{
				if (!State.Layer)
				{
					State.Layer = (Combo*)GetParent("combo");
					if (!State.Layer)
						return false;
				}

				switch (Source.Type)
				{
					case ButtonType_Symbol:
					case ButtonType_Symbol_Text:
						State.Clicked = nk_combo_item_symbol_text(C, (nk_symbol_type)Source.Symbol, Source.Text.c_str(), Source.Text.size(), Source.Align);
						break;
					case ButtonType_Image:
					case ButtonType_Image_Text:
						State.Clicked = nk_combo_item_image_text(C, nk_image_auto(Source.Image), Source.Text.c_str(), Source.Text.size(), Source.Align);
						break;
					case ButtonType_Text:
					case ButtonType_Color:
					default:
						State.Clicked = nk_combo_item_text(C, Source.Text.c_str(), Source.Text.size(), Source.Align);
						break;
				}

				if (State.Clicked)
					State.Layer->SetValue(Source.Text);

				return State.Clicked;
			}
			void ComboItem::BuildEnd(nk_context* C)
			{
			}
			void ComboItem::BuildStyle(nk_context* C, nk_style* S)
			{
			}
			float ComboItem::GetWidth()
			{
				return 0;
			}
			float ComboItem::GetHeight()
			{
				return 0;
			}
			std::string ComboItem::GetType()
			{
				return "combo-item";
			}
			bool ComboItem::IsClicked()
			{
				return State.Clicked;
			}

			Group::Group(Context* View) : Stateful(View)
			{
				Style = (nk_style_window*)malloc(sizeof(nk_style_window));
				memset(Style, 0, sizeof(nk_style_window));

				Source.Flags = 0;

				Bind("bordered", [this]()
				{
					if (GetBoolean("bordered", true))
						Source.Flags |= NK_WINDOW_BORDER;
					else
						Source.Flags &= ~NK_WINDOW_BORDER;
				});
				Bind("movable", [this]()
				{
					if (GetBoolean("movable", false))
						Source.Flags |= NK_WINDOW_MOVABLE;
					else
						Source.Flags &= ~NK_WINDOW_MOVABLE;
				});
				Bind("scalable", [this]()
				{
					if (GetBoolean("scalable", false))
						Source.Flags |= NK_WINDOW_SCALABLE;
					else
						Source.Flags &= ~NK_WINDOW_SCALABLE;
				});
				Bind("closable", [this]()
				{
					if (GetBoolean("closable", false))
						Source.Flags |= NK_WINDOW_CLOSABLE;
					else
						Source.Flags &= ~NK_WINDOW_CLOSABLE;
				});
				Bind("minimizable", [this]()
				{
					if (GetBoolean("minimizable", false))
						Source.Flags |= NK_WINDOW_MINIMIZABLE;
					else
						Source.Flags &= ~NK_WINDOW_MINIMIZABLE;
				});
				Bind("no-scrollbar", [this]()
				{
					if (GetBoolean("no-scrollbar", false))
						Source.Flags |= NK_WINDOW_NO_SCROLLBAR;
					else
						Source.Flags &= ~NK_WINDOW_NO_SCROLLBAR;
				});
				Bind("titled", [this]()
				{
					if (GetBoolean("titled", true))
						Source.Flags |= NK_WINDOW_TITLE;
					else
						Source.Flags &= ~NK_WINDOW_TITLE;
				});
				Bind("scroll-auto-hide", [this]()
				{
					if (GetBoolean("scroll-auto-hide", false))
						Source.Flags |= NK_WINDOW_SCROLL_AUTO_HIDE;
					else
						Source.Flags &= ~NK_WINDOW_SCROLL_AUTO_HIDE;
				});
				Bind("focusless", [this]()
				{
					if (GetBoolean("focusless", false))
						Source.Flags |= NK_WINDOW_BACKGROUND;
					else
						Source.Flags &= ~NK_WINDOW_BACKGROUND;
				});
				Bind("scale-left", [this]()
				{
					if (GetBoolean("scale-left", false))
						Source.Flags |= NK_WINDOW_SCALE_LEFT;
					else
						Source.Flags &= ~NK_WINDOW_SCALE_LEFT;
				});
				Bind("no-input", [this]()
				{
					if (GetBoolean("no-input", false))
						Source.Flags |= NK_WINDOW_NO_INPUT;
					else
						Source.Flags &= ~NK_WINDOW_NO_INPUT;
				});
				Bind("text", [this]()
				{
					Source.Text = GetString("text", "");
					Rest::Stroke(&Source.Text).ReplaceOf("\r\n", "");
				});
				Bind("scrolled", [this]() { Source.Scrolled = GetBoolean("scrolled", false); });
				Bind("offset-x", [this]() { Source.OffsetX = GetInteger("offset-x", 0); });
				Bind("offset-y", [this]() { Source.OffsetY = GetInteger("offset-y", 0); });
				Bind("header-align", [this]() { Style->header.align = CastHeaderAlign(GetString("header-align", "right")); });
				Bind("header-close-symbol", [this]() { Style->header.close_symbol = CastSymbol(GetString("header-close-symbol", "cross")); });
				Bind("header-hide-symbol", [this]() { Style->header.minimize_symbol = CastSymbol(GetString("header-hide-symbol", "minus")); });
				Bind("header-show-symbol", [this]() { Style->header.maximize_symbol = CastSymbol(GetString("header-show-symbol", "plus")); });
				Bind("header-normal", [this]() { Style->header.normal = CastV4Item(Content, GetString("header-normal", "40 40 40 255")); });
				Bind("header-hover", [this]() { Style->header.hover = CastV4Item(Content, GetString("header-hover", "40 40 40 255")); });
				Bind("header-active", [this]() { Style->header.active = CastV4Item(Content, GetString("header-active", "40 40 40 255")); });
				Bind("header-label-normal", [this]() { Style->header.label_normal = CastV4(GetV4("header-label-normal", { 175, 175, 175, 255 })); });
				Bind("header-label-hover", [this]() { Style->header.label_hover = CastV4(GetV4("header-label-hover", { 175, 175, 175, 255 })); });
				Bind("header-label-active", [this]() { Style->header.label_active = CastV4(GetV4("header-label-active", { 175, 175, 175, 255 })); });
				Bind("header-label-padding", [this]() { Style->header.label_padding = CastV2(GetV2("header-label-padding", { 4, 4 })); });
				Bind("header-padding", [this]() { Style->header.padding = CastV2(GetV2("header-padding", { 4, 4 })); });
				Bind("header-spacing", [this]() { Style->header.spacing = CastV2(GetV2("header-spacing", { 0, 0 })); });
				Bind("header-close-normal", [this]() { Style->header.close_button.normal = CastV4Item(Content, GetString("header-close-normal", "40 40 40 255")); });
				Bind("header-close-hover", [this]() { Style->header.close_button.hover = CastV4Item(Content, GetString("header-close-hover", "40 40 40 255")); });
				Bind("header-close-active", [this]() { Style->header.close_button.active = CastV4Item(Content, GetString("header-close-active", "40 40 40 255")); });
				Bind("header-close-border", [this]() { Style->header.close_button.border_color = CastV4(GetV4("header-close-border", { 0, 0, 0, 0 })); });
				Bind("header-close-text-background", [this]() { Style->header.close_button.text_background = CastV4(GetV4("header-close-text-background", { 40, 40, 40, 255 })); });
				Bind("header-close-text-normal", [this]() { Style->header.close_button.text_normal = CastV4(GetV4("header-close-text-normal", { 175, 175, 175, 255 })); });
				Bind("header-close-text-hover", [this]() { Style->header.close_button.text_hover = CastV4(GetV4("header-close-text-hover", { 175, 175, 175, 255 })); });
				Bind("header-close-text-active", [this]() { Style->header.close_button.text_active = CastV4(GetV4("header-close-text-active", { 175, 175, 175, 255 })); });
				Bind("header-close-padding", [this]() { Style->header.close_button.padding = CastV2(GetV2("header-close-padding", { 4, 4 })); });
				Bind("header-close-touch-padding", [this]() { Style->header.close_button.touch_padding = CastV2(GetV2("header-close-touch-padding", { 4, 4 })); });
				Bind("header-close-text-align", [this]() { Style->header.close_button.text_alignment = CastTextAlign(GetString("header-close-text-align", "middle center")); });
				Bind("header-close-border-size", [this]() { Style->header.close_button.border = GetFloat("header-close-border-size", 0.0f); });
				Bind("header-close-border-radius", [this]() { Style->header.close_button.rounding = GetFloat("header-close-border-radius", 0.0f); });
				Bind("header-hide-normal", [this]() { Style->header.minimize_button.normal = CastV4Item(Content, GetString("header-hide-normal", "40 40 40 255")); });
				Bind("header-hide-hover", [this]() { Style->header.minimize_button.hover = CastV4Item(Content, GetString("header-hide-hover", "40 40 40 255")); });
				Bind("header-hide-active", [this]() { Style->header.minimize_button.active = CastV4Item(Content, GetString("header-hide-active", "40 40 40 255")); });
				Bind("header-hide-border", [this]() { Style->header.minimize_button.border_color = CastV4(GetV4("header-hide-border", { 0, 0, 0, 0 })); });
				Bind("header-hide-text-background", [this]() { Style->header.minimize_button.text_background = CastV4(GetV4("header-hide-text-background", { 40, 40, 40, 255 })); });
				Bind("header-hide-text-normal", [this]() { Style->header.minimize_button.text_normal = CastV4(GetV4("header-hide-text-normal", { 175, 175, 175, 255 })); });
				Bind("header-hide-text-hover", [this]() { Style->header.minimize_button.text_hover = CastV4(GetV4("header-hide-text-hover", { 175, 175, 175, 255 })); });
				Bind("header-hide-text-active", [this]() { Style->header.minimize_button.text_active = CastV4(GetV4("header-hide-text-active", { 175, 175, 175, 255 })); });
				Bind("header-hide-padding", [this]() { Style->header.minimize_button.padding = CastV2(GetV2("header-hide-padding", { 0, 0 })); });
				Bind("header-hide-touch-padding", [this]() { Style->header.minimize_button.touch_padding = CastV2(GetV2("header-hide-touch-padding", { 0, 0 })); });
				Bind("header-hide-text-align", [this]() { Style->header.minimize_button.text_alignment = CastTextAlign(GetString("header-hide-text-align", "middle center")); });
				Bind("header-hide-border-size", [this]() { Style->header.minimize_button.border = GetFloat("header-hide-border-size", 0.0f); });
				Bind("header-hide-border-radius", [this]() { Style->header.minimize_button.rounding = GetFloat("header-hide-border-radius", 0.0f); });
				Bind("background", [this]() { Style->background = CastV4(GetV4("background", { 45, 45, 45, 255 })); });
				Bind("fixed", [this]() { Style->fixed_background = CastV4Item(Content, GetString("fixed", "45 45 45 255")); });
				Bind("border", [this]() { Style->border_color = CastV4(GetV4("border", { 65, 65, 65, 255 })); });
				Bind("popup-border", [this]() { Style->popup_border_color = CastV4(GetV4("popup-border", { 65, 65, 65, 255 })); });
				Bind("combo-border", [this]() { Style->combo_border_color = CastV4(GetV4("combo-border", { 65, 65, 65, 255 })); });
				Bind("context-border", [this]() { Style->contextual_border_color = CastV4(GetV4("context-border", { 65, 65, 65, 255 })); });
				Bind("menu-border", [this]() { Style->menu_border_color = CastV4(GetV4("menu-border", { 65, 65, 65, 255 })); });
				Bind("group-border", [this]() { Style->group_border_color = CastV4(GetV4("group-border", { 65, 65, 65, 255 })); });
				Bind("tool-border", [this]() { Style->tooltip_border_color = CastV4(GetV4("tool-border", { 65, 65, 65, 255 })); });
				Bind("scaler", [this]() { Style->scaler = CastV4Item(Content, GetString("scaler", "175 175 175 255")); });
				Bind("border-radius", [this]() { Style->rounding = GetFloat("border-radius", 0.0f); });
				Bind("spacing", [this]() { Style->spacing = CastV2(GetV2("spacing", { 4, 4 })); });
				Bind("scroll-size", [this]() { Style->scrollbar_size = CastV2(GetV2("scroll-size", { 10, 10 })); });
				Bind("min-size", [this]() { Style->min_size = CastV2(GetV2("min-size", { 64, 64 })); });
				Bind("combo-border-size", [this]() { Style->combo_border = GetFloat("combo-border-size", 1.0f); });
				Bind("context-border-size", [this]() { Style->contextual_border = GetFloat("context-border-size", 1.0f); });
				Bind("menu-border-size", [this]() { Style->menu_border = GetFloat("menu-border-size", 1.0f); });
				Bind("group-border-size", [this]() { Style->group_border = GetFloat("group-border-size", 1.0f); });
				Bind("tool-border-size", [this]() { Style->tooltip_border = GetFloat("tool-border-size", 1.0f); });
				Bind("popup-border-size", [this]() { Style->popup_border = GetFloat("popup-border-size", 1.0f); });
				Bind("border-size", [this]() { Style->border = GetFloat("border-size", 2.0f); });
				Bind("min-row-height-padding", [this]() { Style->min_row_height_padding = GetFloat("min-row-height-padding", 8.0f); });
				Bind("padding", [this]() { Style->padding = CastV2(GetV2("padding", { 4, 4 })); });
				Bind("group-padding", [this]() { Style->group_padding = CastV2(GetV2("group-padding", { 4, 4 })); });
				Bind("popup-padding", [this]() { Style->popup_padding = CastV2(GetV2("popup-padding", { 4, 4 })); });
				Bind("combo-padding", [this]() { Style->combo_padding = CastV2(GetV2("combo-padding", { 4, 4 })); });
				Bind("context-padding", [this]() { Style->contextual_padding = CastV2(GetV2("context-padding", { 4, 4 })); });
				Bind("menu-padding", [this]() { Style->menu_padding = CastV2(GetV2("menu-padding", { 4, 4 })); });
				Bind("tool-padding", [this]() { Style->tooltip_padding = CastV2(GetV2("tool-padding", { 4, 4 })); });
			}
			Group::~Group()
			{
			}
			bool Group::BuildBegin(nk_context* C)
			{
				nk_flags Flags = 0;
				if (Source.Scrolled)
					Flags = nk_group_scrolled_offset_begin(C, &Source.OffsetX, &Source.OffsetY, Source.Text.c_str(), Source.Flags);
				else
					Flags = nk_group_begin_titled(C, Hash.Text.c_str(), Source.Text.c_str(), Source.Flags);

				State.Opened = (Flags != NK_WINDOW_CLOSED && Flags != NK_WINDOW_MINIMIZED);
				return State.Opened;
			}
			void Group::BuildEnd(nk_context* C)
			{
				if (!State.Opened)
					return;

				if (Source.Scrolled)
					nk_group_scrolled_end(C);
				else
					nk_group_end(C);
			}
			void Group::BuildStyle(nk_context* C, nk_style* S)
			{
				memcpy(&C->style.window, S ? &S->window : Style, sizeof(nk_style_window));
				BuildFont(C, S);
			}
			float Group::GetWidth()
			{
				return 0;
			}
			float Group::GetHeight()
			{
				return 0;
			}
			std::string Group::GetType()
			{
				return "group";
			}
			bool Group::IsOpened()
			{
				return State.Opened;
			}

			Tree::Tree(Context* View) : Stateful(View)
			{
				Style.Tab = (nk_style_tab*)malloc(sizeof(nk_style_tab));
				memset(Style.Tab, 0, sizeof(nk_style_tab));

				Style.Selectable = (nk_style_selectable*)malloc(sizeof(nk_style_selectable));
				memset(Style.Selectable, 0, sizeof(nk_style_selectable));

				State.Value = 0;

				Bind("minimize-normal", [this]()
				{
					Style.Tab->tab_minimize_button.normal = CastV4Item(Content, GetString("minimize-normal", "40 40 40 255"));
					Style.Tab->node_minimize_button.normal = Style.Tab->tab_minimize_button.normal;
				});
				Bind("minimize-hover", [this]()
				{
					Style.Tab->tab_minimize_button.hover = CastV4Item(Content, GetString("minimize-hover", "40 40 40 255"));
					Style.Tab->node_minimize_button.hover = Style.Tab->tab_minimize_button.hover;
				});
				Bind("minimize-active", [this]()
				{
					Style.Tab->tab_minimize_button.active = CastV4Item(Content, GetString("minimize-active", "40 40 40 255"));
					Style.Tab->node_minimize_button.active = Style.Tab->tab_minimize_button.active;
				});
				Bind("minimize-border", [this]()
				{
					Style.Tab->tab_minimize_button.border_color = CastV4(GetV4("minimize-border", { 0, 0, 0, 0 }));
					Style.Tab->node_minimize_button.border_color = Style.Tab->tab_minimize_button.border_color;
				});
				Bind("minimize-text-background", [this]()
				{
					Style.Tab->tab_minimize_button.text_background = CastV4(GetV4("minimize-text-background", { 40, 40, 40, 255 }));
					Style.Tab->node_minimize_button.text_background = Style.Tab->tab_minimize_button.text_background;
				});
				Bind("minimize-text-normal", [this]()
				{
					Style.Tab->tab_minimize_button.text_normal = CastV4(GetV4("minimize-text-normal", { 175, 175, 175, 255 }));
					Style.Tab->node_minimize_button.text_normal = Style.Tab->tab_minimize_button.text_normal;
				});
				Bind("minimize-text-hover", [this]()
				{
					Style.Tab->tab_minimize_button.text_hover = CastV4(GetV4("minimize-text-hover", { 175, 175, 175, 255 }));
					Style.Tab->node_minimize_button.text_hover = Style.Tab->tab_minimize_button.text_hover;
				});
				Bind("minimize-text-active", [this]()
				{
					Style.Tab->tab_minimize_button.text_active = CastV4(GetV4("minimize-text-active", { 175, 175, 175, 255 }));
					Style.Tab->node_minimize_button.text_active = Style.Tab->tab_minimize_button.text_active;
				});
				Bind("minimize-padding", [this]()
				{
					Style.Tab->tab_minimize_button.padding = CastV2(GetV2("minimize-padding", { 2.0f, 2.0f }));
					Style.Tab->node_minimize_button.padding = Style.Tab->tab_minimize_button.padding;
				});
				Bind("minimize-image-padding", [this]()
				{
					Style.Tab->tab_minimize_button.image_padding = CastV2(GetV2("minimize-image-padding", { 0, 0 }));
					Style.Tab->node_minimize_button.image_padding = Style.Tab->tab_minimize_button.image_padding;
				});
				Bind("minimize-touch-padding", [this]()
				{
					Style.Tab->tab_minimize_button.touch_padding = CastV2(GetV2("minimize-touch-padding", { 0, 0 }));
					Style.Tab->node_minimize_button.touch_padding = Style.Tab->tab_minimize_button.touch_padding;
				});
				Bind("minimize-text-align", [this]()
				{
					Style.Tab->tab_minimize_button.text_alignment = CastTextAlign(GetString("minimize-text-align", "middle center"));
					Style.Tab->node_minimize_button.text_alignment = Style.Tab->tab_minimize_button.text_alignment;
				});
				Bind("minimize-border-size", [this]()
				{
					Style.Tab->tab_minimize_button.border = GetFloat("minimize-border-size", 0.0f);
					Style.Tab->node_minimize_button.border = Style.Tab->tab_minimize_button.border;
				});
				Bind("minimize-border-radius", [this]()
				{
					Style.Tab->tab_minimize_button.rounding = GetFloat("minimize-border-radius", 0.0f);
					Style.Tab->node_minimize_button.rounding = Style.Tab->tab_minimize_button.rounding;
				});
				Bind("maximize-normal", [this]()
				{
					Style.Tab->tab_maximize_button.normal = CastV4Item(Content, GetString("maximize-normal", "40 40 40 255"));
					Style.Tab->node_maximize_button.normal = Style.Tab->tab_maximize_button.normal;
				});
				Bind("maximize-hover", [this]()
				{
					Style.Tab->tab_maximize_button.hover = CastV4Item(Content, GetString("maximize-hover", "40 40 40 255"));
					Style.Tab->node_maximize_button.hover = Style.Tab->tab_maximize_button.hover;
				});
				Bind("maximize-active", [this]()
				{
					Style.Tab->tab_maximize_button.active = CastV4Item(Content, GetString("maximize-active", "40 40 40 255"));
					Style.Tab->node_maximize_button.active = Style.Tab->tab_maximize_button.active;
				});
				Bind("maximize-border", [this]()
				{
					Style.Tab->tab_maximize_button.border_color = CastV4(GetV4("maximize-border", { 0, 0, 0, 0 }));
					Style.Tab->node_maximize_button.border_color = Style.Tab->tab_maximize_button.border_color;
				});
				Bind("maximize-text-background", [this]()
				{
					Style.Tab->tab_maximize_button.text_background = CastV4(GetV4("maximize-text-background", { 40, 40, 40, 255 }));
					Style.Tab->node_maximize_button.text_background = Style.Tab->tab_maximize_button.text_background;
				});
				Bind("maximize-text-normal", [this]()
				{
					Style.Tab->tab_maximize_button.text_normal = CastV4(GetV4("maximize-text-normal", { 175, 175, 175, 255 }));
					Style.Tab->node_maximize_button.text_normal = Style.Tab->tab_maximize_button.text_normal;
				});
				Bind("maximize-text-hover", [this]()
				{
					Style.Tab->tab_maximize_button.text_hover = CastV4(GetV4("maximize-text-hover", { 175, 175, 175, 255 }));
					Style.Tab->node_maximize_button.text_hover = Style.Tab->tab_maximize_button.text_hover;
				});
				Bind("maximize-text-active", [this]()
				{
					Style.Tab->tab_maximize_button.text_active = CastV4(GetV4("maximize-text-active", { 175, 175, 175, 255 }));
					Style.Tab->node_maximize_button.text_active = Style.Tab->tab_maximize_button.text_active;
				});
				Bind("maximize-padding", [this]()
				{
					Style.Tab->tab_maximize_button.padding = CastV2(GetV2("maximize-padding", { 2.0f, 2.0f }));
					Style.Tab->node_maximize_button.padding = Style.Tab->tab_maximize_button.padding;
				});
				Bind("maximize-image-padding", [this]()
				{
					Style.Tab->tab_maximize_button.image_padding = CastV2(GetV2("maximize-image-padding", { 0, 0 }));
					Style.Tab->node_maximize_button.image_padding = Style.Tab->tab_maximize_button.image_padding;
				});
				Bind("maximize-touch-padding", [this]()
				{
					Style.Tab->tab_maximize_button.touch_padding = CastV2(GetV2("maximize-touch-padding", { 0, 0 }));
					Style.Tab->node_maximize_button.touch_padding = Style.Tab->tab_maximize_button.touch_padding;
				});
				Bind("maximize-text-align", [this]()
				{
					Style.Tab->tab_maximize_button.text_alignment = CastTextAlign(GetString("maximize-text-align", "middle center"));
					Style.Tab->node_maximize_button.text_alignment = Style.Tab->tab_maximize_button.text_alignment;
				});
				Bind("maximize-border-size", [this]()
				{
					Style.Tab->tab_maximize_button.border = GetFloat("maximize-border-size", 0.0f);
					Style.Tab->node_maximize_button.border = Style.Tab->tab_maximize_button.border;
				});
				Bind("maximize-border-radius", [this]()
				{
					Style.Tab->tab_maximize_button.rounding = GetFloat("maximize-border-radius", 0.0f);
					Style.Tab->node_maximize_button.rounding = Style.Tab->tab_maximize_button.rounding;
				});
				Bind("bind", [this]()
				{
					Source.Bind = GetModel("bind");
					if (Source.Bind.empty())
						return;

					std::string* Result = GetGlobal(Source.Bind);
					if (Result != nullptr)
						State.Value = (int)(*Result == "1" || *Result == "true");
				});
				Bind("type", [this]()
				{
					std::string Type = GetString("type", "node");
					if (Type == "node")
						Source.Type = TreeType_Node;
					else if (Type == "tab")
						Source.Type = TreeType_Tab;
				});
				Bind("text", [this]()
				{
					Source.Text = GetString("text", "");
					Rest::Stroke(&Source.Text).ReplaceOf("\r\n", "");
				});
				Bind("image", [this]() { Source.Image = Content->Load<Graphics::Texture2D>(GetString("image", ""), nullptr); });
				Bind("selectable", [this]() { Source.Selectable = GetBoolean("selectable", false); });
				Bind("minimized", [this]() { Source.Minimized = GetBoolean("minimized", true); });
				Bind("background", [this]() { Style.Tab->background = CastV4Item(Content, GetString("background", "40 40 40 255")); });
				Bind("border", [this]() { Style.Tab->border_color = CastV4(GetV4("border", { 65, 65, 65, 255 })); });
				Bind("text-background", [this]() { Style.Tab->text = CastV4(GetV4("text-background", { 175, 175, 175, 255 })); });
				Bind("minimize-symbol", [this]() { Style.Tab->sym_minimize = CastSymbol(GetString("minimize-symbol", "triangle-right")); });
				Bind("maximize-symbol", [this]() { Style.Tab->sym_maximize = CastSymbol(GetString("maximize-symbol", "triangle-down")); });
				Bind("padding", [this]() { Style.Tab->padding = CastV2(GetV2("padding", { 4, 4 })); });
				Bind("spacing", [this]() { Style.Tab->spacing = CastV2(GetV2("spacing", { 4, 4 })); });
				Bind("indent", [this]() { Style.Tab->indent = GetFloat("indent", 10); });
				Bind("border-size", [this]() { Style.Tab->border = GetFloat("border-size", 1); });
				Bind("border-radius", [this]() { Style.Tab->rounding = GetFloat("border-radius", 0); });
				Bind("select-normal", [this]() { Style.Selectable->normal = CastV4Item(Content, GetString("select-normal", "45 45 45 255")); });
				Bind("select-hover", [this]() { Style.Selectable->hover = CastV4Item(Content, GetString("select-hover", "45 45 45 255")); });
				Bind("select-pressed", [this]() { Style.Selectable->pressed = CastV4Item(Content, GetString("select-pressed", "45 45 45 255")); });
				Bind("select-normal-active", [this]() { Style.Selectable->normal_active = CastV4Item(Content, GetString("select-normal-active", "35 35 35 255")); });
				Bind("select-hover-active", [this]() { Style.Selectable->hover_active = CastV4Item(Content, GetString("select-hover-active", "35 35 35 255")); });
				Bind("select-pressed-active", [this]() { Style.Selectable->pressed_active = CastV4Item(Content, GetString("select-pressed-active", "35 35 35 255")); });
				Bind("select-text-background", [this]() { Style.Selectable->text_background = CastV4(GetV4("select-text-background", { 0, 0, 0, 0 })); });
				Bind("select-text-normal", [this]() { Style.Selectable->text_normal = CastV4(GetV4("select-text-normal", { 175, 175, 175, 255 })); });
				Bind("select-text-hover", [this]() { Style.Selectable->text_hover = CastV4(GetV4("select-text-hover", { 175, 175, 175, 255 })); });
				Bind("select-text-pressed", [this]() { Style.Selectable->text_pressed = CastV4(GetV4("select-text-pressed", { 175, 175, 175, 255 })); });
				Bind("select-text-normal-active", [this]() { Style.Selectable->text_normal_active = CastV4(GetV4("select-text-normal-active", { 175, 175, 175, 255 })); });
				Bind("select-text-hover-active", [this]() { Style.Selectable->text_hover_active = CastV4(GetV4("select-text-hover-active", { 175, 175, 175, 255 })); });
				Bind("select-text-pressed-active", [this]() { Style.Selectable->text_pressed_active = CastV4(GetV4("select-text-pressed-active", { 175, 175, 175, 255 })); });
				Bind("select-padding", [this]() { Style.Selectable->padding = CastV2(GetV2("select-padding", { 2.0f, 2.0f })); });
				Bind("select-image-padding", [this]() { Style.Selectable->image_padding = CastV2(GetV2("select-image-padding", { 2.0f, 2.0f })); });
				Bind("select-touch-padding", [this]() { Style.Selectable->touch_padding = CastV2(GetV2("select-touch-padding", { 0, 0 })); });
				Bind("select-text-align", [this]() { Style.Selectable->text_alignment = CastTextAlign(GetString("select-text-align", "")); });
				Bind("select-border-radius", [this]() { Style.Selectable->rounding = GetFloat("select-border-radius", 0.0f); });
			}
			Tree::~Tree()
			{
				free(Style.Tab);
				free(Style.Selectable);
			}
			bool Tree::BuildBegin(nk_context* C)
			{
				if (Source.Selectable)
				{
					State.Selected = State.Value;
					if (Source.Image != nullptr)
						State.Opened = nk_tree_element_image_push_hashed(C, (nk_tree_type)Source.Type, nk_image_auto(Source.Image), Source.Text.c_str(), Source.Minimized ? NK_MINIMIZED : NK_MAXIMIZED, &State.Value, Hash.Text.c_str(), Hash.Text.size(), Hash.Id);
					else
						State.Opened = nk_tree_element_push_hashed(C, (nk_tree_type)Source.Type, Source.Text.c_str(), Source.Minimized ? NK_MINIMIZED : NK_MAXIMIZED, &State.Value, Hash.Text.c_str(), Hash.Text.size(), Hash.Id);

					if (State.Selected != State.Value && !Source.Bind.empty())
						SetGlobal(Source.Bind, State.Value ? "1" : "0");
				}
				else
				{
					if (Source.Image != nullptr)
						State.Opened = nk_tree_image_push_hashed(C, (nk_tree_type)Source.Type, nk_image_auto(Source.Image), Source.Text.c_str(), Source.Minimized ? NK_MINIMIZED : NK_MAXIMIZED, Hash.Text.c_str(), Hash.Text.size(), Hash.Id);
					else
						State.Opened = nk_tree_push_hashed(C, (nk_tree_type)Source.Type, Source.Text.c_str(), Source.Minimized ? NK_MINIMIZED : NK_MAXIMIZED, Hash.Text.c_str(), Hash.Text.size(), Hash.Id);
				}

				return State.Opened;
			}
			void Tree::BuildEnd(nk_context* C)
			{
				if (State.Opened)
				{
					if (Source.Selectable)
						nk_tree_element_pop(C);
					else
						nk_tree_pop(C);
				}
			}
			void Tree::BuildStyle(nk_context* C, nk_style* S)
			{
				if (Source.Selectable)
					memcpy(&C->style.selectable, S ? &S->selectable : Style.Selectable, sizeof(nk_style_tab));

				memcpy(&C->style.tab, S ? &S->tab : Style.Tab, sizeof(nk_style_tab));
				BuildFont(C, S);
			}
			float Tree::GetWidth()
			{
				return 0;
			}
			float Tree::GetHeight()
			{
				return 0;
			}
			std::string Tree::GetType()
			{
				return "tree";
			}
			void Tree::SetValue(bool Value)
			{
				State.Value = (int)Value;
			}
			bool Tree::GetValue()
			{
				return State.Value > 0;
			}
			bool Tree::IsClicked()
			{
				return State.Selected != State.Value;
			}
			bool Tree::IsOpened()
			{
				return State.Opened;
			}

			Popup::Popup(Context* View) : Widget(View)
			{
				Source.Flags = 0;

				Bind("bordered", [this]()
				{
					if (GetBoolean("bordered", true))
						Source.Flags |= NK_WINDOW_BORDER;
					else
						Source.Flags &= ~NK_WINDOW_BORDER;
				});
				Bind("movable", [this]()
				{
					if (GetBoolean("movable", false))
						Source.Flags |= NK_WINDOW_MOVABLE;
					else
						Source.Flags &= ~NK_WINDOW_MOVABLE;
				});
				Bind("scalable", [this]()
				{
					if (GetBoolean("scalable", false))
						Source.Flags |= NK_WINDOW_SCALABLE;
					else
						Source.Flags &= ~NK_WINDOW_SCALABLE;
				});
				Bind("closable", [this]()
				{
					if (GetBoolean("closable", false))
						Source.Flags |= NK_WINDOW_CLOSABLE;
					else
						Source.Flags &= ~NK_WINDOW_CLOSABLE;
				});
				Bind("minimizable", [this]()
				{
					if (GetBoolean("minimizable", false))
						Source.Flags |= NK_WINDOW_MINIMIZABLE;
					else
						Source.Flags &= ~NK_WINDOW_MINIMIZABLE;
				});
				Bind("no-scrollbar", [this]()
				{
					if (GetBoolean("no-scrollbar", false))
						Source.Flags |= NK_WINDOW_NO_SCROLLBAR;
					else
						Source.Flags &= ~NK_WINDOW_NO_SCROLLBAR;
				});
				Bind("titled", [this]()
				{
					if (GetBoolean("titled", true))
						Source.Flags |= NK_WINDOW_TITLE;
					else
						Source.Flags &= ~NK_WINDOW_TITLE;
				});
				Bind("scroll-auto-hide", [this]()
				{
					if (GetBoolean("scroll-auto-hide", false))
						Source.Flags |= NK_WINDOW_SCROLL_AUTO_HIDE;
					else
						Source.Flags &= ~NK_WINDOW_SCROLL_AUTO_HIDE;
				});
				Bind("focusless", [this]()
				{
					if (GetBoolean("focusless", false))
						Source.Flags |= NK_WINDOW_BACKGROUND;
					else
						Source.Flags &= ~NK_WINDOW_BACKGROUND;
				});
				Bind("scale-left", [this]()
				{
					if (GetBoolean("scale-left", false))
						Source.Flags |= NK_WINDOW_SCALE_LEFT;
					else
						Source.Flags &= ~NK_WINDOW_SCALE_LEFT;
				});
				Bind("no-input", [this]()
				{
					if (GetBoolean("no-input", false))
						Source.Flags |= NK_WINDOW_NO_INPUT;
					else
						Source.Flags &= ~NK_WINDOW_NO_INPUT;
				});
				Bind("text", [this]()
				{
					Source.Text = GetString("text", "");
					Rest::Stroke(&Source.Text).ReplaceOf("\r\n", "");
				});
				Bind("width", [this]() { Source.Width = GetFloat("width", 100); });
				Bind("height", [this]() { Source.Height = GetFloat("height", 100); });
				Bind("x", [this]() { Source.X = GetFloat("x", 0); });
				Bind("y", [this]() { Source.Y = GetFloat("y", 0); });
				Bind("dynamic", [this]() { Source.Dynamic = GetBoolean("dynamic", false); });
			}
			Popup::~Popup()
			{
			}
			bool Popup::BuildBegin(nk_context* C)
			{
				State.Opened = nk_popup_begin(C, Source.Dynamic ? NK_POPUP_DYNAMIC : NK_POPUP_STATIC, Source.Text.c_str(), Source.Flags, nk_rect(Source.X, Source.Y, Source.Width, Source.Height));
				if (State.Opened)
					Base->SetOverflow(true);
				
				return State.Opened;
			}
			void Popup::BuildEnd(nk_context* C)
			{
				if (State.Opened)
					nk_popup_end(C);
			}
			void Popup::BuildStyle(nk_context* C, nk_style* S)
			{
				BuildFont(C, S);
			}
			float Popup::GetWidth()
			{
				return Source.Width;
			}
			float Popup::GetHeight()
			{
				return Source.Height;
			}
			std::string Popup::GetType()
			{
				return "popup";
			}
			void Popup::Close()
			{
				nk_popup_close(Base->GetContext());
			}
			void Popup::SetScrollOffset(const Compute::Vector2& Offset)
			{
				nk_popup_set_scroll(Base->GetContext(), Offset.X, Offset.Y);
			}
			Compute::Vector2 Popup::GetScrollOffset()
			{
				nk_uint OffsetX = 0, OffsetY = 0;
				nk_popup_get_scroll(Base->GetContext(), &OffsetX, &OffsetY);
				return Compute::Vector2(OffsetX, OffsetY);
			}
			bool Popup::IsOpened()
			{
				return State.Opened;
			}

			Contextual::Contextual(Context* View) : Widget(View)
			{
				Source.Flags = 0;

				Bind("bordered", [this]()
				{
					if (GetBoolean("bordered", true))
						Source.Flags |= NK_WINDOW_BORDER;
					else
						Source.Flags &= ~NK_WINDOW_BORDER;
				});
				Bind("movable", [this]()
				{
					if (GetBoolean("movable", false))
						Source.Flags |= NK_WINDOW_MOVABLE;
					else
						Source.Flags &= ~NK_WINDOW_MOVABLE;
				});
				Bind("scalable", [this]()
				{
					if (GetBoolean("scalable", false))
						Source.Flags |= NK_WINDOW_SCALABLE;
					else
						Source.Flags &= ~NK_WINDOW_SCALABLE;
				});
				Bind("closable", [this]()
				{
					if (GetBoolean("closable", false))
						Source.Flags |= NK_WINDOW_CLOSABLE;
					else
						Source.Flags &= ~NK_WINDOW_CLOSABLE;
				});
				Bind("minimizable", [this]()
				{
					if (GetBoolean("minimizable", false))
						Source.Flags |= NK_WINDOW_MINIMIZABLE;
					else
						Source.Flags &= ~NK_WINDOW_MINIMIZABLE;
				});
				Bind("no-scrollbar", [this]()
				{
					if (GetBoolean("no-scrollbar", false))
						Source.Flags |= NK_WINDOW_NO_SCROLLBAR;
					else
						Source.Flags &= ~NK_WINDOW_NO_SCROLLBAR;
				});
				Bind("titled", [this]()
				{
					if (GetBoolean("titled", true))
						Source.Flags |= NK_WINDOW_TITLE;
					else
						Source.Flags &= ~NK_WINDOW_TITLE;
				});
				Bind("scroll-auto-hide", [this]()
				{
					if (GetBoolean("scroll-auto-hide", false))
						Source.Flags |= NK_WINDOW_SCROLL_AUTO_HIDE;
					else
						Source.Flags &= ~NK_WINDOW_SCROLL_AUTO_HIDE;
				});
				Bind("focusless", [this]()
				{
					if (GetBoolean("focusless", false))
						Source.Flags |= NK_WINDOW_BACKGROUND;
					else
						Source.Flags &= ~NK_WINDOW_BACKGROUND;
				});
				Bind("scale-left", [this]()
				{
					if (GetBoolean("scale-left", false))
						Source.Flags |= NK_WINDOW_SCALE_LEFT;
					else
						Source.Flags &= ~NK_WINDOW_SCALE_LEFT;
				});
				Bind("no-input", [this]()
				{
					if (GetBoolean("no-input", false))
						Source.Flags |= NK_WINDOW_NO_INPUT;
					else
						Source.Flags &= ~NK_WINDOW_NO_INPUT;
				});
				Bind("width", [this]() { Source.Width = GetFloat("width", 100); });
				Bind("height", [this]() { Source.Height = GetFloat("height", 100); });
				Bind("trigger-width", [this]() { Source.TriggerWidth = GetFloat("trigger-width", 100); });
				Bind("trigger-height", [this]() { Source.TriggerHeight = GetFloat("trigger-height", 100); });
				Bind("trigger-x", [this]() { Source.TriggerX = GetFloat("trigger-x", 0); });
				Bind("trigger-y", [this]() { Source.TriggerY = GetFloat("trigger-y", 0); });
			}
			Contextual::~Contextual()
			{
			}
			bool Contextual::BuildBegin(nk_context* C)
			{
				State.Opened = nk_contextual_begin(C, Source.Flags, nk_vec2(Source.Width, Source.Height), nk_rect(Source.TriggerX, Source.TriggerY, Source.TriggerWidth, Source.TriggerHeight));
				if (State.Opened)
					Base->SetOverflow(true);
				
				return State.Opened;
			}
			void Contextual::BuildEnd(nk_context* C)
			{
				if (State.Opened)
					nk_contextual_end(C);
			}
			void Contextual::BuildStyle(nk_context* C, nk_style* S)
			{
				BuildFont(C, S);
			}
			float Contextual::GetWidth()
			{
				return Source.Width;
			}
			float Contextual::GetHeight()
			{
				return Source.Height;
			}
			std::string Contextual::GetType()
			{
				return "contextual";
			}
			void Contextual::Close()
			{
				nk_contextual_close(Base->GetContext());
			}
			bool Contextual::IsOpened()
			{
				return State.Opened;
			}

			ContextualItem::ContextualItem(Context* View) : Widget(View)
			{
				Style = (nk_style_button*)malloc(sizeof(nk_style_button));
				memset(Style, 0, sizeof(nk_style_button));

				Source.Image = nullptr;

				Bind("type", [this]()
				{
					std::string Type = GetString("type", "text");
					if (Type == "text")
						Source.Type = ButtonType_Text;
					else if (Type == "symbol")
						Source.Type = ButtonType_Symbol;
					else if (Type == "image")
						Source.Type = ButtonType_Image;
				});
				Bind("text", [this]()
				{
					Source.Text = GetString("text", "");
					Rest::Stroke(&Source.Text).ReplaceOf("\r\n", "");
				});
				Bind("image", [this]() { Source.Image = Content->Load<Graphics::Texture2D>(GetString("image", ""), nullptr); });
				Bind("symbol", [this]() { Source.Symbol = CastSymbol(GetString("symbol", "none")); });
				Bind("align", [this]() { Source.Align = CastTextAlign(GetString("align", "left")); });
				Bind("normal", [this]() { Style->normal = CastV4Item(Content, GetString("normal", "50 50 50 255")); });
				Bind("hover", [this]() { Style->hover = CastV4Item(Content, GetString("hover", "40 40 40 255")); });
				Bind("active", [this]() { Style->active = CastV4Item(Content, GetString("active", "35 35 35 255")); });
				Bind("border", [this]() { Style->border_color = CastV4(GetV4("border", { 65, 65, 65, 255 })); });
				Bind("text-background", [this]() { Style->text_background = CastV4(GetV4("text-background", { 50, 50, 50, 255 })); });
				Bind("text-normal", [this]() { Style->text_normal = CastV4(GetV4("text-normal", { 175, 175, 175, 255 })); });
				Bind("text-hover", [this]() { Style->text_hover = CastV4(GetV4("text-hover", { 175, 175, 175, 255 })); });
				Bind("text-active", [this]() { Style->text_active = CastV4(GetV4("text-active", { 175, 175, 175, 255 })); });
				Bind("padding", [this]() { Style->padding = CastV2(GetV2("padding", { 2.0f, 2.0f })); });
				Bind("image-padding", [this]() { Style->image_padding = CastV2(GetV2("image-padding", { 0, 0 })); });
				Bind("touch-padding", [this]() { Style->touch_padding = CastV2(GetV2("touch-padding", { 0, 0 })); });
				Bind("text-align", [this]() { Style->text_alignment = CastTextAlign(GetString("text-align", "middle center")); });
				Bind("border-size", [this]() { Style->border = GetFloat("border-size", 1.0f); });
				Bind("border-radius", [this]() { Style->rounding = GetFloat("border-radius", 4.0f); });
			}
			ContextualItem::~ContextualItem()
			{
				free(Style);
			}
			bool ContextualItem::BuildBegin(nk_context* C)
			{
				if (!State.Layer)
				{
					State.Layer = (Contextual*)GetParent("contextual");
					if (!State.Layer)
						return false;
				}

				switch (Source.Type)
				{
					case ButtonType_Symbol:
					case ButtonType_Symbol_Text:
						State.Clicked = nk_contextual_item_symbol_text(C, (nk_symbol_type)Source.Symbol, Source.Text.c_str(), Source.Text.size(), Source.Align);
						break;
					case ButtonType_Image:
					case ButtonType_Image_Text:
						State.Clicked = nk_contextual_item_image_text(C, nk_image_auto(Source.Image), Source.Text.c_str(), Source.Text.size(), Source.Align);
						break;
					case ButtonType_Text:
					case ButtonType_Color:
					default:
						State.Clicked = nk_contextual_item_text(C, Source.Text.c_str(), Source.Text.size(), Source.Align);
						break;
				}

				return State.Clicked;
			}
			void ContextualItem::BuildEnd(nk_context* C)
			{
			}
			void ContextualItem::BuildStyle(nk_context* C, nk_style* S)
			{
				memcpy(&C->style.contextual_button, S ? &S->contextual_button : Style, sizeof(nk_style_button));
				BuildFont(C, S);
			}
			float ContextualItem::GetWidth()
			{
				return 0;
			}
			float ContextualItem::GetHeight()
			{
				return 0;
			}
			std::string ContextualItem::GetType()
			{
				return "contextual-item";
			}
			bool ContextualItem::IsClicked()
			{
				return State.Clicked;
			}

			Tooltip::Tooltip(Context* View) : Widget(View)
			{
				Bind("text", [this]()
				{
					Source.Text = GetString("text", "");
					Rest::Stroke(&Source.Text).ReplaceOf("\r\n", "");
				});
				Bind("width", [this]() { Source.Width = GetFloat("width", 40.0f); });
				Bind("sub", [this]() { Source.Sub = GetBoolean("sub", false); });
			}
			Tooltip::~Tooltip()
			{
			}
			bool Tooltip::BuildBegin(nk_context* C)
			{
				if (Source.Sub)
					nk_tooltip_begin(C, Source.Width);
				else
					nk_tooltip(C, Source.Text.c_str());

				return true;
			}
			void Tooltip::BuildEnd(nk_context* C)
			{
				if (Source.Sub)
					nk_tooltip_end(C);
			}
			void Tooltip::BuildStyle(nk_context* C, nk_style* S)
			{
				BuildFont(C, S);
			}
			float Tooltip::GetWidth()
			{
				return Source.Sub ? Source.Width : 0;
			}
			float Tooltip::GetHeight()
			{
				return 0;
			}
			std::string Tooltip::GetType()
			{
				return "tooltip";
			}

			Header::Header(Context* View) : Widget(View)
			{
			}
			Header::~Header()
			{
			}
			bool Header::BuildBegin(nk_context* C)
			{
				nk_menubar_begin(C);
				return true;
			}
			void Header::BuildEnd(nk_context* C)
			{
				nk_menubar_end(C);
			}
			void Header::BuildStyle(nk_context* C, nk_style* S)
			{
				BuildFont(C, S);
			}
			float Header::GetWidth()
			{
				return 0;
			}
			float Header::GetHeight()
			{
				return 0;
			}
			std::string Header::GetType()
			{
				return "header";
			}

			HeaderTab::HeaderTab(Context* View) : Widget(View)
			{
				Style = (nk_style_button*)malloc(sizeof(nk_style_button));
				memset(Style, 0, sizeof(nk_style_button));

				Source.Image = nullptr;

				Bind("type", [this]()
				{
					std::string Type = GetString("type", "text");
					if (Type == "text")
						Source.Type = ButtonType_Text;
					else if (Type == "symbol")
						Source.Type = ButtonType_Symbol;
					else if (Type == "symbol-text")
						Source.Type = ButtonType_Symbol_Text;
					else if (Type == "image")
						Source.Type = ButtonType_Image;
					else if (Type == "image-text")
						Source.Type = ButtonType_Image_Text;
				});
				Bind("text", [this]()
				{
					Source.Text = GetString("text", "");
					Rest::Stroke(&Source.Text).ReplaceOf("\r\n", "");
				});
				Bind("image", [this]() { Source.Image = Content->Load<Graphics::Texture2D>(GetString("image", ""), nullptr); });
				Bind("symbol", [this]() { Source.Symbol = CastSymbol(GetString("symbol", "none")); });
				Bind("align", [this]() { Source.Align = CastTextAlign(GetString("align", "left")); });
				Bind("width", [this]() { Source.Width = GetFloat("width", 40); });
				Bind("height", [this]() { Source.Height = GetFloat("height", 15); });
				Bind("normal", [this]() { Style->normal = CastV4Item(Content, GetString("normal", "50 50 50 255")); });
				Bind("hover", [this]() { Style->hover = CastV4Item(Content, GetString("hover", "40 40 40 255")); });
				Bind("active", [this]() { Style->active = CastV4Item(Content, GetString("active", "35 35 35 255")); });
				Bind("border", [this]() { Style->border_color = CastV4(GetV4("border", { 65, 65, 65, 255 })); });
				Bind("text-background", [this]() { Style->text_background = CastV4(GetV4("text-background", { 50, 50, 50, 255 })); });
				Bind("text-normal", [this]() { Style->text_normal = CastV4(GetV4("text-normal", { 175, 175, 175, 255 })); });
				Bind("text-hover", [this]() { Style->text_hover = CastV4(GetV4("text-hover", { 175, 175, 175, 255 })); });
				Bind("text-active", [this]() { Style->text_active = CastV4(GetV4("text-active", { 175, 175, 175, 255 })); });
				Bind("padding", [this]() { Style->padding = CastV2(GetV2("padding", { 2.0f, 2.0f })); });
				Bind("image-padding", [this]() { Style->image_padding = CastV2(GetV2("image-padding", { 0, 0 })); });
				Bind("touch-padding", [this]() { Style->touch_padding = CastV2(GetV2("touch-padding", { 0, 0 })); });
				Bind("text-align", [this]() { Style->text_alignment = CastTextAlign(GetString("text-align", "middle center")); });
				Bind("border-size", [this]() { Style->border = GetFloat("border-size", 1.0f); });
				Bind("border-radius", [this]() { Style->rounding = GetFloat("border-radius", 4.0f); });
			}
			HeaderTab::~HeaderTab()
			{
				free(Style);
			}
			bool HeaderTab::BuildBegin(nk_context* C)
			{
				if (!State.Layer)
				{
					State.Layer = (Header*)GetParent("header");
					if (!State.Layer)
						return false;
				}

				switch (Source.Type)
				{
					case ButtonType_Symbol:
					case ButtonType_Symbol_Text:
						State.Opened = nk_menu_begin_symbol_text(C, Source.Text.c_str(), Source.Text.size(), Source.Align, (nk_symbol_type)Source.Symbol, nk_vec2(Source.Width, Source.Height));
						break;
					case ButtonType_Image:
					case ButtonType_Image_Text:
						State.Opened = nk_menu_begin_image_text(C, Source.Text.c_str(), Source.Text.size(), Source.Align, nk_image_auto(Source.Image), nk_vec2(Source.Width, Source.Height));
						break;
					case ButtonType_Text:
					case ButtonType_Color:
					default:
						State.Opened = nk_menu_begin_text(C, Source.Text.c_str(), Source.Text.size(), Source.Align, nk_vec2(Source.Width, Source.Height));
						break;
				}

				if (State.Opened)
					Base->SetOverflow(true);

				return State.Opened;
			}
			void HeaderTab::BuildEnd(nk_context* C)
			{
				if (State.Opened)
					nk_menu_end(C);
			}
			void HeaderTab::BuildStyle(nk_context* C, nk_style* S)
			{
				memcpy(&C->style.menu_button, S ? &S->menu_button : Style, sizeof(nk_style_button));
				BuildFont(C, S);
			}
			float HeaderTab::GetWidth()
			{
				return 0;
			}
			float HeaderTab::GetHeight()
			{
				return 0;
			}
			std::string HeaderTab::GetType()
			{
				return "header-tab";
			}
			bool HeaderTab::IsOpened()
			{
				return State.Opened;
			}

			HeaderItem::HeaderItem(Context* View) : Widget(View)
			{
				Style = (nk_style_button*)malloc(sizeof(nk_style_button));
				memset(Style, 0, sizeof(nk_style_button));

				Source.Image = nullptr;

				Bind("type", [this]()
				{
					std::string Type = GetString("type", "text");
					if (Type == "text")
						Source.Type = ButtonType_Text;
					else if (Type == "symbol")
						Source.Type = ButtonType_Symbol;
					else if (Type == "image")
						Source.Type = ButtonType_Image;
				});
				Bind("text", [this]()
				{
					Source.Text = GetString("text", "");
					Rest::Stroke(&Source.Text).ReplaceOf("\r\n", "");
				});
				Bind("image", [this]() { Source.Image = Content->Load<Graphics::Texture2D>(GetString("image", ""), nullptr); });
				Bind("symbol", [this]() { Source.Symbol = CastSymbol(GetString("symbol", "none")); });
				Bind("align", [this]() { Source.Align = CastTextAlign(GetString("align", "middle left")); });
				Bind("normal", [this]() { Style->normal = CastV4Item(Content, GetString("normal", "50 50 50 255")); });
				Bind("hover", [this]() { Style->hover = CastV4Item(Content, GetString("hover", "40 40 40 255")); });
				Bind("active", [this]() { Style->active = CastV4Item(Content, GetString("active", "35 35 35 255")); });
				Bind("border", [this]() { Style->border_color = CastV4(GetV4("border", { 65, 65, 65, 255 })); });
				Bind("text-background", [this]() { Style->text_background = CastV4(GetV4("text-background", { 50, 50, 50, 255 })); });
				Bind("text-normal", [this]() { Style->text_normal = CastV4(GetV4("text-normal", { 175, 175, 175, 255 })); });
				Bind("text-hover", [this]() { Style->text_hover = CastV4(GetV4("text-hover", { 175, 175, 175, 255 })); });
				Bind("text-active", [this]() { Style->text_active = CastV4(GetV4("text-active", { 175, 175, 175, 255 })); });
				Bind("padding", [this]() { Style->padding = CastV2(GetV2("padding", { 2.0f, 2.0f })); });
				Bind("image-padding", [this]() { Style->image_padding = CastV2(GetV2("image-padding", { 0, 0 })); });
				Bind("touch-padding", [this]() { Style->touch_padding = CastV2(GetV2("touch-padding", { 0, 0 })); });
				Bind("text-align", [this]() { Style->text_alignment = CastTextAlign(GetString("text-align", "middle center")); });
				Bind("border-size", [this]() { Style->border = GetFloat("border-size", 1.0f); });
				Bind("border-radius", [this]() { Style->rounding = GetFloat("border-radius", 4.0f); });
			}
			HeaderItem::~HeaderItem()
			{
				free(Style);
			}
			bool HeaderItem::BuildBegin(nk_context* C)
			{
				if (!State.Layer)
				{
					State.Layer = (HeaderTab*)GetParent("header-tab");
					if (!State.Layer)
						return false;
				}

				switch (Source.Type)
				{
					case ButtonType_Symbol:
					case ButtonType_Symbol_Text:
						State.Clicked = nk_menu_item_symbol_text(C, (nk_symbol_type)Source.Symbol, Source.Text.c_str(), Source.Text.size(), Source.Align);
						break;
					case ButtonType_Image:
					case ButtonType_Image_Text:
						State.Clicked = nk_menu_item_image_text(C, nk_image_auto(Source.Image), Source.Text.c_str(), Source.Text.size(), Source.Align);
						break;
					case ButtonType_Text:
					case ButtonType_Color:
					default:
						State.Clicked = nk_menu_item_text(C, Source.Text.c_str(), Source.Text.size(), Source.Align);
						break;
				}

				return State.Clicked;
			}
			void HeaderItem::BuildEnd(nk_context* C)
			{
			}
			void HeaderItem::BuildStyle(nk_context* C, nk_style* S)
			{
				memcpy(&C->style.contextual_button, S ? &S->contextual_button : Style, sizeof(nk_style_button));
				BuildFont(C, S);
			}
			float HeaderItem::GetWidth()
			{
				return 0;
			}
			float HeaderItem::GetHeight()
			{
				return 0;
			}
			std::string HeaderItem::GetType()
			{
				return "header-item";
			}
			bool HeaderItem::IsClicked()
			{
				return State.Clicked;
			}

			DrawLine::DrawLine(Context* View) : Element(View)
			{
				Bind("color", [this]() { Source.Color = GetV4("color", { 65, 65, 65, 255 }); });
				Bind("x1", [this]() { Source.X1 = GetFloat("x1", 0.0f); });
				Bind("y1", [this]() { Source.Y1 = GetFloat("y1", 0.0f); });
				Bind("x2", [this]() { Source.X2 = GetFloat("x2", 0.0f); });
				Bind("y2", [this]() { Source.Y2 = GetFloat("y2", 0.0f); });
				Bind("thickness", [this]() { Source.Thickness = GetFloat("thickness", 1.0f); });
				Bind("relative", [this]() { Source.Relative = GetBoolean("relative", false); });
			}
			DrawLine::~DrawLine()
			{
			}
			bool DrawLine::BuildBegin(nk_context* C)
			{
				float X1 = Source.X1, Y1 = Source.Y1;
				float X2 = Source.X2, Y2 = Source.Y2;

				if (Source.Relative)
				{
					float Width = GetAreaWidth();
					X1 *= Width;
					X2 *= Width;

					float Height = GetAreaHeight();
					Y1 *= Height;
					Y2 *= Height;
				}

				nk_stroke_line(&C->current->buffer, X1, Y1, X2, Y2, Source.Thickness, CastV4(Source.Color));
				return true;
			}
			void DrawLine::BuildEnd(nk_context* C)
			{
			}
			void DrawLine::BuildStyle(nk_context* C, nk_style* S)
			{
			}
			float DrawLine::GetWidth()
			{
				return 0;
			}
			float DrawLine::GetHeight()
			{
				return 0;
			}
			std::string DrawLine::GetType()
			{
				return "line";
			}

			DrawCurve::DrawCurve(Context* View) : Element(View)
			{
				Bind("color", [this]() { Source.Color = GetV4("color", { 65, 65, 65, 255 }); });
				Bind("x1", [this]() { Source.X1 = GetFloat("x1", 0.0f); });
				Bind("y1", [this]() { Source.Y1 = GetFloat("y1", 0.0f); });
				Bind("x2", [this]() { Source.X2 = GetFloat("x2", 0.0f); });
				Bind("y2", [this]() { Source.Y2 = GetFloat("y2", 0.0f); });
				Bind("x3", [this]() { Source.X3 = GetFloat("x3", 0.0f); });
				Bind("y3", [this]() { Source.Y3 = GetFloat("y3", 0.0f); });
				Bind("x4", [this]() { Source.X4 = GetFloat("x4", 0.0f); });
				Bind("y4", [this]() { Source.Y4 = GetFloat("y4", 0.0f); });
				Bind("thickness", [this]() { Source.Thickness = GetFloat("thickness", 1.0f); });
				Bind("relative", [this]() { Source.Relative = GetBoolean("relative", false); });
			}
			DrawCurve::~DrawCurve()
			{
			}
			bool DrawCurve::BuildBegin(nk_context* C)
			{
				float X1 = Source.X1, Y1 = Source.Y1;
				float X2 = Source.X2, Y2 = Source.Y2;
				float X3 = Source.X3, Y3 = Source.Y3;
				float X4 = Source.X4, Y4 = Source.Y4;

				if (Source.Relative)
				{
					float Width = GetAreaWidth();
					X1 *= Width;
					X2 *= Width;
					X3 *= Width;
					X4 *= Width;

					float Height = GetAreaHeight();
					Y1 *= Height;
					Y2 *= Height;
					Y3 *= Height;
					Y4 *= Height;
				}

				nk_stroke_curve(&C->current->buffer, X1, Y1, X2, Y2, X3, Y3, X4, Y4, Source.Thickness, CastV4(Source.Color));
				return true;
			}
			void DrawCurve::BuildEnd(nk_context* C)
			{
			}
			void DrawCurve::BuildStyle(nk_context* C, nk_style* S)
			{
			}
			float DrawCurve::GetWidth()
			{
				return 0;
			}
			float DrawCurve::GetHeight()
			{
				return 0;
			}
			std::string DrawCurve::GetType()
			{
				return "curve";
			}

			DrawRect::DrawRect(Context* View) : Element(View)
			{
				Bind("left", [this]() { Source.Left = GetV4("left", { 45, 45, 45, 255 }); });
				Bind("top", [this]() { Source.Top = GetV4("top", { 45, 45, 45, 255 }); });
				Bind("right", [this]() { Source.Right = GetV4("right", { 45, 45, 45, 255 }); });
				Bind("bottom", [this]() { Source.Bottom = GetV4("bottom", { 45, 45, 45, 255 }); });
				Bind("thickness", [this]() { Source.Thickness = GetFloat("thickness", 1.0f); });
				Bind("radius", [this]() { Source.Radius = GetFloat("radius", 0.0f); });
				Bind("width", [this]() { Source.Width = GetFloat("width", 10.0f); });
				Bind("height", [this]() { Source.Height = GetFloat("height", 10.0f); });
				Bind("x", [this]() { Source.X = GetFloat("x", 0.0f); });
				Bind("y", [this]() { Source.Y = GetFloat("y", 0.0f); });
				Bind("gradient", [this]() { Source.Gradient = GetBoolean("gradient", false); });
				Bind("fill", [this]() { Source.Fill = GetBoolean("fill", true); });
				Bind("relative", [this]() { Source.Relative = GetBoolean("relative", false); });
			}
			DrawRect::~DrawRect()
			{
			}
			bool DrawRect::BuildBegin(nk_context* C)
			{
				struct nk_rect Bounds = nk_rect(Source.X, Source.Y, Source.Width, Source.Height);
				if (Source.Relative)
				{
					float Width = GetAreaWidth();
					Bounds.x *= Width;
					Bounds.w *= Width;

					float Height = GetAreaHeight();
					Bounds.y *= Height;
					Bounds.h *= Height;
				}

				if (Source.Fill)
				{
					if (Source.Gradient)
						nk_fill_rect_multi_color(&C->current->buffer, Bounds, CastV4(Source.Left), CastV4(Source.Top), CastV4(Source.Right), CastV4(Source.Bottom));
					else
						nk_fill_rect(&C->current->buffer, Bounds, Source.Radius, CastV4(Source.Left));
				}
				else
					nk_stroke_rect(&C->current->buffer, Bounds, Source.Radius, Source.Thickness, CastV4(Source.Left));

				return true;
			}
			void DrawRect::BuildEnd(nk_context* C)
			{
			}
			void DrawRect::BuildStyle(nk_context* C, nk_style* S)
			{
			}
			float DrawRect::GetWidth()
			{
				return Source.Width;
			}
			float DrawRect::GetHeight()
			{
				return Source.Height;
			}
			std::string DrawRect::GetType()
			{
				return "rect";
			}

			DrawCircle::DrawCircle(Context* View) : Element(View)
			{
				Bind("color", [this]() { Source.Color = GetV4("color", { 45, 45, 45, 255 }); });
				Bind("thickness", [this]() { Source.Thickness = GetFloat("thickness", 1.0f); });
				Bind("width", [this]() { Source.Width = GetFloat("width", 10.0f); });
				Bind("height", [this]() { Source.Height = GetFloat("height", 10.0f); });
				Bind("x", [this]() { Source.X = GetFloat("x", 0.0f); });
				Bind("y", [this]() { Source.Y = GetFloat("y", 0.0f); });
				Bind("fill", [this]() { Source.Fill = GetBoolean("fill", true); });
				Bind("relative", [this]() { Source.Relative = GetBoolean("relative", false); });
			}
			DrawCircle::~DrawCircle()
			{
			}
			bool DrawCircle::BuildBegin(nk_context* C)
			{
				struct nk_rect Bounds = nk_rect(Source.X, Source.Y, Source.Width, Source.Height);
				if (Source.Relative)
				{
					float Width = GetAreaWidth();
					Bounds.x *= Width;
					Bounds.w *= Width;

					float Height = GetAreaHeight();
					Bounds.y *= Height;
					Bounds.h *= Height;
				}

				if (Source.Fill)
					nk_fill_circle(&C->current->buffer, Bounds, CastV4(Source.Color));
				else
					nk_stroke_circle(&C->current->buffer, Bounds, Source.Thickness, CastV4(Source.Color));

				return true;
			}
			void DrawCircle::BuildEnd(nk_context* C)
			{
			}
			void DrawCircle::BuildStyle(nk_context* C, nk_style* S)
			{
			}
			float DrawCircle::GetWidth()
			{
				return Source.Width;
			}
			float DrawCircle::GetHeight()
			{
				return Source.Height;
			}
			std::string DrawCircle::GetType()
			{
				return "circle";
			}

			DrawArc::DrawArc(Context* View) : Element(View)
			{
				Bind("color", [this]() { Source.Color = GetV4("color", { 45, 45, 45, 255 }); });
				Bind("thickness", [this]() { Source.Thickness = GetFloat("thickness", 1.0f); });
				Bind("radius", [this]() { Source.Radius = GetFloat("radius", 5.0f); });
				Bind("min", [this]() { Source.Min = GetFloat("min", 0.0f); });
				Bind("max", [this]() { Source.Max = GetFloat("max", 90.0f); });
				Bind("x", [this]() { Source.X = GetFloat("x", 0.0f); });
				Bind("y", [this]() { Source.Y = GetFloat("y", 0.0f); });
				Bind("fill", [this]() { Source.Fill = GetBoolean("fill", true); });
				Bind("relative", [this]() { Source.Relative = GetBoolean("relative", false); });
			}
			DrawArc::~DrawArc()
			{
			}
			bool DrawArc::BuildBegin(nk_context* C)
			{
				float X = Source.X, Y = Source.Y;
				if (Source.Relative)
				{
					X *= GetAreaWidth();
					Y *= GetAreaHeight();
				}

				if (Source.Fill)
					nk_fill_arc(&C->current->buffer, X, Y, Source.Radius, Source.Min, Source.Max, CastV4(Source.Color));
				else
					nk_stroke_arc(&C->current->buffer, X, Y, Source.Radius, Source.Min, Source.Max, Source.Thickness, CastV4(Source.Color));

				return true;
			}
			void DrawArc::BuildEnd(nk_context* C)
			{
			}
			void DrawArc::BuildStyle(nk_context* C, nk_style* S)
			{
			}
			float DrawArc::GetWidth()
			{
				return 0;
			}
			float DrawArc::GetHeight()
			{
				return 0;
			}
			std::string DrawArc::GetType()
			{
				return "arc";
			}

			DrawTriangle::DrawTriangle(Context* View) : Element(View)
			{
				Bind("color", [this]() { Source.Color = GetV4("color", { 65, 65, 65, 255 }); });
				Bind("x1", [this]() { Source.X1 = GetFloat("x1", 0.0f); });
				Bind("y1", [this]() { Source.Y1 = GetFloat("y1", 0.0f); });
				Bind("x2", [this]() { Source.X2 = GetFloat("x2", 0.0f); });
				Bind("y2", [this]() { Source.Y2 = GetFloat("y2", 0.0f); });
				Bind("x3", [this]() { Source.X3 = GetFloat("x3", 0.0f); });
				Bind("y3", [this]() { Source.Y3 = GetFloat("y3", 0.0f); });
				Bind("thickness", [this]() { Source.Thickness = GetFloat("thickness", 1.0f); });
				Bind("fill", [this]() { Source.Fill = GetBoolean("fill", true); });
				Bind("relative", [this]() { Source.Relative = GetBoolean("relative", false); });
			}
			DrawTriangle::~DrawTriangle()
			{
			}
			bool DrawTriangle::BuildBegin(nk_context* C)
			{
				float X1 = Source.X1, Y1 = Source.Y1;
				float X2 = Source.X2, Y2 = Source.Y2;
				float X3 = Source.X3, Y3 = Source.Y3;

				if (Source.Relative)
				{
					float Width = GetAreaWidth();
					X1 *= Width;
					X2 *= Width;
					X3 *= Width;

					float Height = GetAreaHeight();
					Y1 *= Height;
					Y2 *= Height;
					Y3 *= Height;
				}

				if (Source.Fill)
					nk_fill_triangle(&C->current->buffer, X1, Y1, X2, Y2, X3, Y3, CastV4(Source.Color));
				else
					nk_stroke_triangle(&C->current->buffer, X1, Y1, X2, Y2, X3, Y3, Source.Thickness, CastV4(Source.Color));

				return true;
			}
			void DrawTriangle::BuildEnd(nk_context* C)
			{
			}
			void DrawTriangle::BuildStyle(nk_context* C, nk_style* S)
			{
			}
			float DrawTriangle::GetWidth()
			{
				return 0;
			}
			float DrawTriangle::GetHeight()
			{
				return 0;
			}
			std::string DrawTriangle::GetType()
			{
				return "triangle";
			}

			DrawPolyline::DrawPolyline(Context* View) : Element(View)
			{
				Bind("points", [this]()
				{
					Source.Points.clear();

					const std::string& Buffer = GetString("points", "");
					if (Buffer.empty())
						return;

					std::vector<std::string> Array = Rest::Stroke(&Buffer).Split(' ');
					for (size_t i = 0; i < Array.size() / 2; i++)
					{
						float X = 0.0f, Y = 0.0f;

						Rest::Stroke Number1(&Array[i + 0]);
						if (Number1.HasNumber())
							X = Number1.ToFloat();

						Rest::Stroke Number2(&Array[i + 1]);
						if (Number2.HasNumber())
							Y = Number2.ToFloat();

						Source.Points.push_back(X);
						Source.Points.push_back(Y);
					}
				});
				Bind("color", [this]() { Source.Color = GetV4("color", { 65, 65, 65, 255 }); });
				Bind("thickness", [this]() { Source.Thickness = GetFloat("thickness", 1.0f); });
				Bind("relative", [this]() { Source.Relative = GetBoolean("relative", false); });
			}
			DrawPolyline::~DrawPolyline()
			{
			}
			bool DrawPolyline::BuildBegin(nk_context* C)
			{
				if (Source.Relative)
				{
					if (Source.Points.size() != Source.Points.size())
						State.Points = Source.Points;

					float Width = GetAreaWidth();
					float Height = GetAreaHeight();

					for (size_t i = 0; i < Source.Points.size() / 2; i++)
					{
						State.Points[i + 0] = Source.Points[i + 0] * Width;
						State.Points[i + 1] = Source.Points[i + 1] * Height;
					}
				}

				nk_stroke_polyline(&C->current->buffer, State.Points.data(), State.Points.size(), Source.Thickness, CastV4(Source.Color));
				return true;
			}
			void DrawPolyline::BuildEnd(nk_context* C)
			{
			}
			void DrawPolyline::BuildStyle(nk_context* C, nk_style* S)
			{
			}
			float DrawPolyline::GetWidth()
			{
				return 0;
			}
			float DrawPolyline::GetHeight()
			{
				return 0;
			}
			std::string DrawPolyline::GetType()
			{
				return "polyline";
			}

			DrawPolygon::DrawPolygon(Context* View) : Element(View)
			{
				Bind("points", [this]()
				{
					Source.Points.clear();

					const std::string& Buffer = GetString("points", "");
					if (Buffer.empty())
						return;

					std::vector<std::string> Array = Rest::Stroke(&Buffer).Split(' ');
					for (size_t i = 0; i < Array.size() / 2; i++)
					{
						float X = 0.0f, Y = 0.0f;

						Rest::Stroke Number1(&Array[i + 0]);
						if (Number1.HasNumber())
							X = Number1.ToFloat();

						Rest::Stroke Number2(&Array[i + 1]);
						if (Number2.HasNumber())
							Y = Number2.ToFloat();

						Source.Points.push_back(X);
						Source.Points.push_back(Y);
					}
				});
				Bind("color", [this]() { Source.Color = GetV4("color", { 65, 65, 65, 255 }); });
				Bind("thickness", [this]() { Source.Thickness = GetFloat("thickness", 1.0f); });
				Bind("fill", [this]() { Source.Fill = GetBoolean("fill", true); });
				Bind("relative", [this]() { Source.Relative = GetBoolean("relative", false); });
			}
			DrawPolygon::~DrawPolygon()
			{
			}
			bool DrawPolygon::BuildBegin(nk_context* C)
			{
				if (Source.Relative)
				{
					if (Source.Points.size() != Source.Points.size())
						State.Points = Source.Points;

					float Width = GetAreaWidth();
					float Height = GetAreaHeight();

					for (size_t i = 0; i < Source.Points.size() / 2; i++)
					{
						State.Points[i + 0] = Source.Points[i + 0] * Width;
						State.Points[i + 1] = Source.Points[i + 1] * Height;
					}
				}

				if (Source.Fill)
					nk_fill_polygon(&C->current->buffer, State.Points.data(), State.Points.size(), CastV4(Source.Color));
				else
					nk_stroke_polygon(&C->current->buffer, State.Points.data(), State.Points.size(), Source.Thickness, CastV4(Source.Color));
				
				return true;
			}
			void DrawPolygon::BuildEnd(nk_context* C)
			{
			}
			void DrawPolygon::BuildStyle(nk_context* C, nk_style* S)
			{
			}
			float DrawPolygon::GetWidth()
			{
				return 0;
			}
			float DrawPolygon::GetHeight()
			{
				return 0;
			}
			std::string DrawPolygon::GetType()
			{
				return "polygon";
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
			void Template::BuildEnd(nk_context* C)
			{
				for (auto& It : State.Statefuls)
					It->Pop();
				State.Statefuls.clear();
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

				if (Function != nullptr)
					Continue = Function(Continue);

				if (Continue)
				{
					for (auto& It : Target->GetNodes())
						It->Build();
				}

				Target->BuildEnd(Base->GetContext());
				Target->BuildStyle(Base->GetContext(), (nk_style*)Base->GetDefaultStyle());

				return Continue;
			}
			bool Template::Compose(const std::string& Name, const std::function<bool(bool)>& Function)
			{
				return Compose(Get<Element>(Name), Function);
			}
			bool Template::ComposeStateful(Stateful* Target, const std::function<bool(bool)>& Function)
			{
				if (!Target)
					return false;

				Target->Push();
				State.Statefuls.push_back(Target);
				return Compose(Target, Function);
			}

			Context::Context(Graphics::GraphicsDevice* NewDevice, Graphics::Activity* NewActivity) : Device(NewDevice), Activity(NewActivity), IndexBuffer(nullptr), VertexBuffer(nullptr), Shader(nullptr), Font(nullptr), FontBaking(false), DOM(nullptr), Width(512), Height(512), WidgetId(0), Overflowing(false)
			{
				if (!Device)
					return;

				Engine = (nk_context*)malloc(sizeof(nk_context));
				if (nk_init_default(Engine, nullptr) > 0)
				{
					Engine->clip.copy = ClipboardCopy;
					Engine->clip.paste = ClipboardPaste;
					Engine->clip.userdata = nk_handle_ptr(this);
				}

				Commands = (nk_buffer*)malloc(sizeof(nk_buffer));
				nk_buffer_init_default(Commands);

				Atlas = (nk_font_atlas*)malloc(sizeof(nk_font_atlas));
				nk_font_atlas_init_default(Atlas);

				Style = (nk_style*)malloc(sizeof(nk_style));
				memcpy(Style, &Engine->style, sizeof(nk_style));

				Null = (nk_draw_null_texture*)malloc(sizeof(nk_draw_null_texture));
				if (!FontBakingBegin() || !FontBakingEnd())
					return;

				ResizeBuffers(512 * 1024, 128 * 1024);
				nk_input_begin(Engine);
				nk_input_end(Engine);

				DepthStencil = Device->GetDepthStencilState("DEF_NONE");
				Rasterizer = Device->GetRasterizerState("DEF_CULL_NONE_SCISSOR");
				Blend = Device->GetBlendState("DEF_ADDITIVE_SOURCE");
				Sampler = Device->GetSamplerState("DEF_LINEAR");

				static Graphics::InputLayout Layout[3] =
				{
					{ "POSITION", Graphics::Format_R32G32_Float, (size_t)(&((Vertex*)0)->Position) },
					{ "TEXCOORD", Graphics::Format_R32G32_Float, (size_t)(&((Vertex*)0)->TexCoord) },
					{ "COLOR", Graphics::Format_R8G8B8A8_Unorm, (size_t)(&((Vertex*)0)->Color) }
				};

				Graphics::Shader::Desc I = Graphics::Shader::Desc();
				I.Layout = Layout;
				I.LayoutSize = 3;

				if (Device->GetSection("pass/gui", &I.Data))
				{
					Shader = Device->CreateShader(I);
					Device->UpdateBufferSize(Shader, sizeof(Compute::Matrix4x4));
				}

				FactoryPush<GUI::Panel>("panel");
				FactoryPush<GUI::Restyle>("restyle");
				FactoryPush<GUI::For>("for");
				FactoryPush<GUI::Const>("const");
				FactoryPush<GUI::Set>("set");
				FactoryPush<GUI::LogIf>("if");
				FactoryPush<GUI::LogElseIf>("else-if");
				FactoryPush<GUI::LogElse>("else");
				FactoryPush<GUI::Escape>("escape");
				FactoryPush<GUI::PreLayout>("pre-layout");
				FactoryPush<GUI::PrePush>("pre-push");
				FactoryPush<GUI::Layout>("layout");
				FactoryPush<GUI::Push>("push");
				FactoryPush<GUI::Button>("button");
				FactoryPush<GUI::Text>("text");
				FactoryPush<GUI::Image>("image");
				FactoryPush<GUI::Checkbox>("checkbox");
				FactoryPush<GUI::Radio>("radio");
				FactoryPush<GUI::Selectable>("selectable");
				FactoryPush<GUI::Slider>("slider");
				FactoryPush<GUI::Progress>("progress");
				FactoryPush<GUI::ColorPicker>("color-picker");
				FactoryPush<GUI::Property>("property");
				FactoryPush<GUI::Edit>("edit");
				FactoryPush<GUI::Combo>("combo");
				FactoryPush<GUI::ComboItem>("combo-item");
				FactoryPush<GUI::Group>("group");
				FactoryPush<GUI::Tree>("tree");
				FactoryPush<GUI::Popup>("popup");
				FactoryPush<GUI::Contextual>("contextual");
				FactoryPush<GUI::ContextualItem>("contextual-item");
				FactoryPush<GUI::Tooltip>("tooltip");
				FactoryPush<GUI::Header>("header");
				FactoryPush<GUI::HeaderTab>("header-tab");
				FactoryPush<GUI::HeaderItem>("header-item");
				FactoryPush<GUI::DrawLine>("line");
				FactoryPush<GUI::DrawCurve>("curve");
				FactoryPush<GUI::DrawRect>("rect");
				FactoryPush<GUI::DrawCircle>("circle");
				FactoryPush<GUI::DrawArc>("arc");
				FactoryPush<GUI::DrawTriangle>("triangle");
				FactoryPush<GUI::DrawPolyline>("polyline");
				FactoryPush<GUI::DrawPolygon>("polygon");
				FactoryPush<GUI::Template>("template");
			}
			Context::~Context()
			{
				Restore();
				nk_font_atlas_clear(Atlas);
				nk_buffer_free(Commands);
				nk_free(Engine);

				free(Style);
				free(Null);
				free(Commands);
				free(Atlas);
				free(Engine);
				delete Shader;
				delete Font;
				delete VertexBuffer;
				delete IndexBuffer;
			}
			void Context::ResizeBuffers(size_t MaxVertices, size_t MaxIndices)
			{
				Graphics::ElementBuffer::Desc F;
				F.AccessFlags = Graphics::CPUAccess_Write;
				F.Usage = Graphics::ResourceUsage_Dynamic;
				F.BindFlags = Graphics::ResourceBind_Vertex_Buffer;
				F.ElementCount = (unsigned int)MaxVertices;
				F.ElementWidth = sizeof(Vertex);
				F.UseSubresource = false;

				delete VertexBuffer;
				VertexBuffer = Device->CreateElementBuffer(F);

				F.BindFlags = Graphics::ResourceBind_Index_Buffer;
				F.ElementCount = (unsigned int)MaxIndices;
				F.ElementWidth = sizeof(nk_draw_index);

				delete IndexBuffer;
				IndexBuffer = Device->CreateElementBuffer(F);
			}
			void Context::Prepare(unsigned int _Width, unsigned int _Height)
			{
				Width = (float)_Width; Height = (float)_Height;
				Projection.Row[0] = 2.0f / Width;
				Projection.Row[1] = 0.0f;
				Projection.Row[2] = 0.0f;
				Projection.Row[3] = 0.0f;
				Projection.Row[4] = 0.0f;
				Projection.Row[5] = -2.0f / Height;
				Projection.Row[6] = 0.0f;
				Projection.Row[7] = 0.0f;
				Projection.Row[8] = 0.0f;
				Projection.Row[9] = 0.0f;
				Projection.Row[10] = 0.5f;
				Projection.Row[11] = 0.0f;
				Projection.Row[12] = -1;
				Projection.Row[13] = 1;
				Projection.Row[14] = 0.5f;
				Projection.Row[15] = 1.0f;
				Overflowing = false;

				if (Activity != nullptr)
				{
					Compute::Vector2 Cursor = Activity->GetCursorPosition();
					EmitCursor(Cursor.X, Cursor.Y);
				}

				nk_input_end(Engine);
				nk_clear(Engine);
				nk_buffer_clear(Commands);

				if (DOM != nullptr)
					DOM->Build();

				nk_input_begin(Engine);
			}
			void Context::Render(const Compute::Matrix4x4& Offset, bool AA)
			{
				static const struct nk_draw_vertex_layout_element Layout[] =
				{
					{ NK_VERTEX_POSITION, NK_FORMAT_FLOAT, NK_OFFSETOF(Vertex, Position) },
					{ NK_VERTEX_TEXCOORD, NK_FORMAT_FLOAT, NK_OFFSETOF(Vertex, TexCoord) },
					{ NK_VERTEX_COLOR, NK_FORMAT_R8G8B8A8, NK_OFFSETOF(Vertex, Color) },
					{ NK_VERTEX_LAYOUT_END }
				};
				Projection = Offset * Projection;

				struct nk_convert_config Converter;
				memset(&Converter, 0, sizeof(Converter));
				Converter.vertex_layout = Layout;
				Converter.vertex_size = sizeof(Vertex);
				Converter.vertex_alignment = NK_ALIGNOF(Vertex);
				Converter.circle_segment_count = 24;
				Converter.curve_segment_count = 24;
				Converter.arc_segment_count = 24;
				Converter.global_alpha = 1.0f;
				Converter.shape_AA = AA ? NK_ANTI_ALIASING_ON : NK_ANTI_ALIASING_OFF;
				Converter.line_AA = AA ? NK_ANTI_ALIASING_ON : NK_ANTI_ALIASING_OFF;
				Converter.null = *Null;

				Graphics::MappedSubresource VertexData, IndexData;
				Device->Map(VertexBuffer, Graphics::ResourceMap_Write_Discard, &VertexData);
				Device->Map(IndexBuffer, Graphics::ResourceMap_Write_Discard, &IndexData);

				nk_buffer Vertices, Indices;
				nk_buffer_init_fixed(&Vertices, VertexData.Pointer, (size_t)VertexBuffer->GetElements());
				nk_buffer_init_fixed(&Indices, IndexData.Pointer, (size_t)IndexBuffer->GetElements());
				nk_convert(Engine, Commands, &Vertices, &Indices, &Converter);

				unsigned int Index = 0;
				Device->Unmap(VertexBuffer, &VertexData);
				Device->Unmap(IndexBuffer, &IndexData);
				Device->SetDepthStencilState(DepthStencil);
				Device->SetSamplerState(Sampler);
				Device->SetBlendState(Blend);
				Device->SetRasterizerState(Rasterizer);
				Device->SetShader(Shader, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);
				Device->SetBuffer(Shader, 3, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);
				Device->SetVertexBuffer(VertexBuffer, 0, sizeof(Vertex), 0);
				Device->SetIndexBuffer(IndexBuffer, Graphics::Format_R16_Uint, 0);
				Device->UpdateBuffer(Shader, &Projection);

				const nk_draw_command* Command = nullptr;
				nk_draw_foreach(Command, Engine, Commands)
				{
					if (!Command->elem_count)
						continue;

					Graphics::Rectangle Scissor;
					Scissor.Left = (long)Command->clip_rect.x;
					Scissor.Right = (long)(Command->clip_rect.x + Command->clip_rect.w);
					Scissor.Top = (long)Command->clip_rect.y;
					Scissor.Bottom = (long)(Command->clip_rect.y + Command->clip_rect.h);

					Device->SetTexture2D((Graphics::Texture2D*)Command->texture.ptr, 0);
					Device->SetScissorRects(1, &Scissor);
					Device->DrawIndexed(Command->elem_count, Index, 0);
					Index += Command->elem_count;
				}
			}
			void Context::EmitKey(Graphics::KeyCode Key, Graphics::KeyMod Mod, int Virtual, int Repeat, bool Pressed)
			{
				nk_input_key(Engine, NK_KEY_SHIFT, Mod & Graphics::KeyMod_SHIFT);
				nk_input_key(Engine, NK_KEY_CTRL, Mod & Graphics::KeyMod_CTRL);

				switch (Key)
				{
					case Tomahawk::Graphics::KeyCode_DELETE:
						nk_input_key(Engine, NK_KEY_DEL, Pressed);
						break;
					case Tomahawk::Graphics::KeyCode_RETURN:
						nk_input_key(Engine, NK_KEY_ENTER, Pressed);
						break;
					case Tomahawk::Graphics::KeyCode_TAB:
						nk_input_key(Engine, NK_KEY_TAB, Pressed);
						break;
					case Tomahawk::Graphics::KeyCode_BACKSPACE:
						nk_input_key(Engine, NK_KEY_BACKSPACE, Pressed);
						break;
					case Tomahawk::Graphics::KeyCode_HOME:
						nk_input_key(Engine, NK_KEY_TEXT_START, Pressed);
						nk_input_key(Engine, NK_KEY_SCROLL_START, Pressed);
						break;
					case Tomahawk::Graphics::KeyCode_END:
						nk_input_key(Engine, NK_KEY_TEXT_END, Pressed);
						nk_input_key(Engine, NK_KEY_SCROLL_END, Pressed);
						break;
					case Tomahawk::Graphics::KeyCode_PAGEDOWN:
						nk_input_key(Engine, NK_KEY_SCROLL_DOWN, Pressed);
						break;
					case Tomahawk::Graphics::KeyCode_PAGEUP:
						nk_input_key(Engine, NK_KEY_SCROLL_UP, Pressed);
						break;
					case Tomahawk::Graphics::KeyCode_LEFT:
						if (Mod & Graphics::KeyMod_CTRL)
							nk_input_key(Engine, NK_KEY_TEXT_WORD_LEFT, Pressed);
						else
							nk_input_key(Engine, NK_KEY_LEFT, Pressed);
						break;
					case Tomahawk::Graphics::KeyCode_RIGHT:
						if (Mod & Graphics::KeyMod_CTRL)
							nk_input_key(Engine, NK_KEY_TEXT_WORD_RIGHT, Pressed);
						else
							nk_input_key(Engine, NK_KEY_RIGHT, Pressed);
						break;
					case Tomahawk::Graphics::KeyCode_C:
						if (Mod & Graphics::KeyMod_CTRL)
							nk_input_key(Engine, NK_KEY_COPY, Pressed);
						break;
					case Tomahawk::Graphics::KeyCode_V:
						if (Mod & Graphics::KeyMod_CTRL)
							nk_input_key(Engine, NK_KEY_PASTE, Pressed);
						break;
					case Tomahawk::Graphics::KeyCode_X:
						if (Mod & Graphics::KeyMod_CTRL)
							nk_input_key(Engine, NK_KEY_CUT, Pressed);
						break;
					case Tomahawk::Graphics::KeyCode_Z:
						if (Mod & Graphics::KeyMod_CTRL)
							nk_input_key(Engine, NK_KEY_TEXT_UNDO, Pressed);
						break;
					case Tomahawk::Graphics::KeyCode_R:
						if (Mod & Graphics::KeyMod_CTRL)
							nk_input_key(Engine, NK_KEY_TEXT_REDO, Pressed);
						break;
					case Tomahawk::Graphics::KeyCode_CURSORLEFT:
					{
						Compute::Vector2 Cursor = (Activity ? Activity->GetCursorPosition() : 0);
						nk_input_button(Engine, NK_BUTTON_LEFT, Cursor.X, Cursor.Y, Pressed);
						if (!Pressed)
							nk_input_button(Engine, NK_BUTTON_DOUBLE, Cursor.X, Cursor.Y, 0);
						else if (Repeat >= 2)
							nk_input_button(Engine, NK_BUTTON_DOUBLE, Cursor.X, Cursor.Y, 1);
						break;
					}
					case Tomahawk::Graphics::KeyCode_CURSORRIGHT:
					{
						Compute::Vector2 Cursor = (Activity ? Activity->GetCursorPosition() : 0);
						nk_input_button(Engine, NK_BUTTON_RIGHT, Cursor.X, Cursor.Y, Pressed);
						break;
					}
					case Tomahawk::Graphics::KeyCode_CURSORMIDDLE:
					{
						Compute::Vector2 Cursor = (Activity ? Activity->GetCursorPosition() : 0);
						nk_input_button(Engine, NK_BUTTON_MIDDLE, Cursor.X, Cursor.Y, Pressed);
						break;
					}
					default:
						break;
				}
			}
			void Context::EmitInput(char* Buffer, int Length)
			{
				if (!Buffer || Length <= 0)
					return;

				for (int i = 0; i < Length; i++)
					nk_input_unicode(Engine, Buffer[i]);
			}
			void Context::EmitWheel(int X, int Y, bool Normal)
			{
				nk_input_scroll(Engine, nk_vec2(X, Y));
			}
			void Context::EmitCursor(int X, int Y)
			{
				nk_input_motion(Engine, X, Y);
			}
			void Context::EmitEscape()
			{
				nk_window* Popup = Engine->current;
				if (!Popup)
					return;

				if (Popup->layout->type & NK_PANEL_SET_POPUP)
					nk_popup_close(Engine);
			}
			void Context::Restore()
			{
				delete DOM;
				DOM = nullptr;

				ClearClasses();
				ClearGlobals();
				ClearFonts();

				nk_input_end(Engine);
				nk_clear(Engine);
				nk_buffer_clear(Commands);
				nk_input_begin(Engine);
			}
			void Context::SetOverflow(bool Enabled)
			{
				Overflowing = Enabled;
			}
			void Context::SetClass(const std::string& ClassName, const std::string& Name, const std::string& Value)
			{
				auto It = Classes.find(ClassName);
				if (It == Classes.end())
					return;

				It->second->Proxy[Name] = Value;
				for (auto& Node : It->second->Watchers)
					Node->Recompute(Name);
			}
			void Context::ClearGlobals()
			{
				for (auto& Item : Args)
					delete Item.second;
				Args.clear();
			}
			void Context::ClearClasses()
			{
				for (auto& Space : Classes)
					delete Space.second;
				Classes.clear();
			}
			void Context::ClearFonts()
			{
				for (auto& Font : Fonts)
				{
					if (Font.second.second != nullptr)
						free(Font.second.second);
				}
				Fonts.clear();

				nk_style_set_font(Engine, nullptr);
				nk_font_atlas_clear(Atlas);
				nk_font_atlas_init_default(Atlas);
			}
			void Context::LoadGlobals(Rest::Document* Document)
			{
				if (!Document)
					return;

				std::vector<Rest::Document*> Array = Document->FindCollection("table", true);
				for (auto& Group : Array)
				{
					if (Group->IsAttribute())
						continue;

					Rest::Document* Basis = Group->GetAttribute("name");
					for (auto& Space : *Group->GetNodes())
					{
						if (Space->IsAttribute())
							continue;

						std::string GlobalName = Basis ? Basis->String + "." + Space->Name : Space->Name;
						if (GlobalName.empty())
							continue;

						auto It = Args.find(GlobalName);
						Actor* Global = (It == Args.end() ? new Actor() : It->second);

						Rest::Document* Sub = Space->Get("[v]");
						if (Sub != nullptr)
							Global->Value = Sub->Serialize();

						if (It == Args.end())
							Args[GlobalName] = Global;
					}
				}
			}
			void Context::LoadClasses(ContentManager* Content, Rest::Document* Document)
			{
				LoadFonts(Content, Document);
				if (!Document)
					return;

				std::vector<Rest::Document*> Array = Document->FindCollection("group", true);
				for (auto& Group : Array)
				{
					if (Group->IsAttribute())
						continue;

					Rest::Document* Basis = Group->GetAttribute("class");
					for (auto& Space : *Group->GetNodes())
					{
						if (Space->IsAttribute())
							continue;

						std::string ClassName = Basis ? Basis->String + "." + Space->Name : Space->Name;
						if (ClassName.empty())
							continue;

						auto It = Classes.find(ClassName);
						ClassProxy* Class = (It == Classes.end() ? new ClassProxy() : It->second);

						for (auto& Node : *Space->GetNodes())
						{
							Rest::Document* Sub = Node->Get("[v]");
							if (!Sub)
							{
								Sub = Node->Get("[:v]");
								if (Sub != nullptr)
									Class->Proxy[':' + Node->GetName()] = Sub->Serialize();
							}
							else
								Class->Proxy[Node->GetName()] = Sub->Serialize();
						}

						if (It == Classes.end())
						{
							Class->ClassName = ClassName;
							Classes[ClassName] = Class;
						}
					}
				}
			}
			void Context::LoadFonts(ContentManager* Content, Rest::Document* Document)
			{
				if (!Content || !Document)
					return;

				std::vector<Rest::Document*> Array = Document->FindCollection("font", true);
				if (Array.empty())
				{
					if (Atlas->font_num < 1)
					{
						if (FontBakingBegin())
							FontBakingEnd();
					}

					return;
				}

				if (!FontBakingBegin())
					return;

				for (auto& Group : Array)
				{
					if (Group->IsAttribute())
						continue;

					Rest::Document* Basis = Group->GetAttribute("name");
					if (!Basis || Basis->String.empty())
						continue;

					Rest::Document* Path = Group->FindPath("path.[v]", true);
					if (!Path)
						continue;

					AssetFile* TTF = Content->Load<AssetFile>(Path->String, nullptr);
					if (!TTF)
					{
						THAWK_ERROR("couldn't bake font \"%s\" from path:\n\t%s", Basis->String.c_str(), Path->String.c_str());
						continue;
					}

					FontConfig Desc;
					Desc.Buffer = TTF->GetBuffer();
					Desc.BufferSize = TTF->GetSize();

					Rest::Document* Node = Group->FindPath("fallback-glyph.[v]", true);
					if (Node != nullptr)
					{
						Rest::Stroke Number(Node->Serialize());
						if (Number.HasInteger())
							Desc.FallbackGlyph = Number.ToInt();
					}

					Node = Group->FindPath("oversample-v.[v]", true);
					if (Node != nullptr)
					{
						Rest::Stroke Number(Node->Serialize());
						if (Number.HasInteger())
							Desc.OversampleV = Number.ToInt();
					}

					Node = Group->FindPath("oversample-h.[v]", true);
					if (Node != nullptr)
					{
						Rest::Stroke Number(Node->Serialize());
						if (Number.HasInteger())
							Desc.OversampleH = Number.ToInt();
					}

					Node = Group->FindPath("height.[v]", true);
					if (Node != nullptr)
					{
						Rest::Stroke Number(Node->Serialize());
						if (Number.HasNumber())
							Desc.Height = Number.ToFloat();
					}

					Node = Group->FindPath("snapping.[v]", true);
					if (Node != nullptr)
						Desc.Snapping = (Node->String == "1" || Node->String == "true");

					Node = Group->FindPath("predef-ranges.[v]", true);
					if (Node != nullptr)
					{
						if (Node->String == "chinese")
							Desc.PredefRanges = GlyphRanges_Chinese;
						else if (Node->String == "cyrillic")
							Desc.PredefRanges = GlyphRanges_Cyrillic;
						else if (Node->String == "korean")
							Desc.PredefRanges = GlyphRanges_Chinese;
						else
							Desc.PredefRanges = GlyphRanges_Manual;
					}

					Node = Group->Get("spacing");
					if (Node != nullptr)
					{
						Rest::Document* X = Node->Get("[x]");
						if (X != nullptr)
						{
							Rest::Stroke Number(X->Serialize());
							if (Number.HasNumber())
								Desc.Spacing.X = Number.ToFloat();
						}

						Rest::Document* Y = Node->Get("[y]");
						if (Y != nullptr)
						{
							Rest::Stroke Number(Y->Serialize());
							if (Number.HasNumber())
								Desc.Spacing.Y = Number.ToFloat();
						}
					}

					std::vector<Rest::Document*> Ranges = Group->FindCollection("range", true);
					if (!Ranges.empty())
						Desc.Ranges.clear();

					for (auto& Range : Ranges)
					{
						uint32_t X1 = 32, X2 = 255;
						if (Range->IsAttribute())
							continue;

						Node = Range->Get("[x1]");
						if (Node != nullptr)
						{
							Rest::Stroke Number(Node->Serialize());
							if (Number.HasInteger())
								X1 = Number.ToInt();
						}

						Node = Range->Get("[x2]");
						if (Node != nullptr)
						{
							Rest::Stroke Number(Node->Serialize());
							if (Number.HasInteger())
								X2 = Number.ToInt();
						}

						Desc.Ranges.push_back(std::make_pair(X1, X2));
					}

					if (!FontBake(Basis->String, &Desc))
						THAWK_ERROR("couldn't bake font \"%s\" from path:\n\t%s", Basis->String.c_str(), Path->String.c_str());
			
					delete TTF;
				}

				FontBakingEnd();
			}
			Body* Context::Load(ContentManager* Content, Rest::Document* Document)
			{
				WidgetId = 0;
				LoadGlobals(Document);
				LoadClasses(Content, Document);

				delete DOM;
				DOM = new Body(this);

				Rest::Document* Main = (Document ? Document->Find("body", true) : nullptr);
				if (!Main)
					return DOM;

				Rest::Document* X = Main->GetAttribute("width");
				if (X != nullptr)
					Width = (float)X->Number;
				else if (Activity != nullptr)
					Width = (float)Activity->GetWidth();

				Rest::Document* Y = Main->GetAttribute("height");
				if (Y != nullptr)
					Height = (float)Y->Number;
				else if (Activity != nullptr)
					Height = (float)Activity->GetHeight();

				DOM->Copy(Content, Main);
				return DOM;
			}
			bool Context::FontBakingBegin()
			{
				if (FontBaking)
					return true;

				nk_font_atlas_begin(Atlas);
				if (!Atlas->default_font)
					Atlas->default_font = nk_font_atlas_add_default(Atlas, 13.0f, nullptr);
				
				FontBaking = true;
				return true;
			}
			bool Context::FontBake(const std::string& Name, FontConfig* Config)
			{
				if (!FontBaking || !Config || !Config->Buffer)
					return false;

				if (Fonts.find(Name) != Fonts.end())
				{
					THAWK_ERROR("font \"%s\" is already presented", Name.c_str());
					return false;
				}

				nk_rune* Ranges = (nk_rune*)malloc(sizeof(nk_rune) * (Config->Ranges.size() * 2 + 1));
				size_t Offset = 0;

				for (auto& Range : Config->Ranges)
				{
					Ranges[Offset + 0] = Range.first;
					Ranges[Offset + 1] = Range.second;
					Offset += 2;
				}

				struct nk_font_config Desc = nk_font_config(Config->Height);
				Desc.fallback_glyph = Config->FallbackGlyph;
				Desc.oversample_h = Config->OversampleH;
				Desc.oversample_v = Config->OversampleV;
				Desc.pixel_snap = Config->Snapping;
				Desc.size = Config->Height;
				Desc.spacing = CastV2(Config->Spacing);
				Desc.ttf_data_owned_by_atlas = 1;

				if (Config->PredefRanges == GlyphRanges_Manual)
				{
					if (!Config->Ranges.empty())
					{
						Ranges[Offset] = 0;
						Desc.range = Ranges;
					}
				}
				else if (Config->PredefRanges == GlyphRanges_Chinese)
				{
					static const nk_rune Ranges[] =
					{
						0x0020, 0x00FF,
						0x3000, 0x30FF,
						0x31F0, 0x31FF,
						0xFF00, 0xFFEF,
						0x4e00, 0x9FAF,
						0
					};
					Desc.range = Ranges;
				}
				else if (Config->PredefRanges == GlyphRanges_Cyrillic)
				{
					static const nk_rune Ranges[] =
					{
						0x0400, 0x052F,
						0x0020, 0x00FF,
						0xA640, 0xA69F,
						0x2DE0, 0x2DFF,
						0
					};
					Desc.range = Ranges;
				}
				else if (Config->PredefRanges == GlyphRanges_Korean)
				{
					static const nk_rune Ranges[] =
					{
						0x0020, 0x00FF,
						0x3131, 0x3163,
						0xAC00, 0xD79D,
						0
					};
					Desc.range = Ranges;
				}

				nk_font* Font = nk_font_atlas_add_from_memory(Atlas, (void*)Config->Buffer, Config->BufferSize, Config->Height, &Desc);
				if (!Font)
				{
					THAWK_ERROR("font \"%s\" cannot be loaded", Name.c_str());
					return false;
				}

				Fonts.insert(std::make_pair(Name, std::make_pair(Font, (void*)Ranges)));
				return true;
			}
			bool Context::FontBakingEnd()
			{
				if (!FontBaking)
					return false;

				int Width = 0, Height = 0;
				const void* Pixels = nk_font_atlas_bake(Atlas, &Width, &Height, NK_FONT_ATLAS_RGBA32);

				if (!Pixels || Width <= 0 || Height <= 0)
				{
					THAWK_ERROR("cannot bake font atlas");
					FontBaking = false;
					return false;
				}

				Graphics::Texture2D::Desc F;
				F.FormatMode = Graphics::Format_R8G8B8A8_Unorm;
				F.Usage = Graphics::ResourceUsage_Default;
				F.BindFlags = Graphics::ResourceBind_Shader_Input;
				F.Width = Width;
				F.Height = Height;
				F.MipLevels = 1;
				F.Data = (void*)Pixels;
				F.RowPitch = Width * 4;
				F.DepthPitch = 0;

				delete Font;
				Font = Device->CreateTexture2D(F);

				nk_font_atlas_end(Atlas, nk_handle_ptr(Font), Null);
				if (Atlas->default_font != nullptr)
					nk_style_set_font(Engine, &Atlas->default_font->handle);

				FontBaking = false;
				return true;
			}
			bool Context::HasOverflow()
			{
				return Overflowing;
			}
			std::string Context::GetClass(const std::string& ClassName, const std::string& Name)
			{
				auto It = Classes.find(ClassName);
				if (It != Classes.end())
					return It->second->Proxy[Name];

				return "";
			}
			nk_font* Context::GetFont(const std::string& Name)
			{
				auto It = Fonts.find(Name);
				if (It != Fonts.end())
					return It->second.first;
				
				return nullptr;
			}
			nk_context* Context::GetContext()
			{
				return Engine;
			}
			const nk_style* Context::GetDefaultStyle()
			{
				return Style;
			}
			Body* Context::GetLayout()
			{
				return DOM;
			}
			Graphics::Activity* Context::GetActivity()
			{
				return Activity;
			}
			uint64_t Context::GetNextWidgetId()
			{
				return WidgetId++;
			}
			float Context::GetWidth()
			{
				return Width;
			}
			float Context::GetHeight()
			{
				return Height;
			}
		}
	}
}