#include "context.h"
#include "elements.h"
#include "layout.h"
#include "logical.h"
#include "shapes.h"
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
				float Color[4];
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
			extern struct nk_vec2 CastV2(const Compute::Vector2& Value)
			{
				return nk_vec2(Value.X, Value.Y);
			}
			extern nk_color CastV4(const Compute::Vector4& Value)
			{
				return nk_rgba((int)Value.X, (int)Value.Y, (int)Value.Z, (int)Value.W);
			}
			extern nk_style_item CastV4Item(ContentManager* Manager, const std::string& Value)
			{
				if (Value.empty())
					return nk_style_item_hide();

				if (Value.size() > 2 && Value[0] == '.' && Value[1] == '/')
				{
					auto* Image = Manager->Load<Graphics::Texture2D>(Value, nullptr);
					if (!Image)
						return nk_style_item_hide();

					return nk_style_item_image(nk_image_ptr(Image));
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
			extern nk_flags CastTextAlign(const std::string& Value)
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
			extern nk_style_header_align CastHeaderAlign(const std::string& Value)
			{
				if (Value == "left")
					return NK_HEADER_LEFT;

				return NK_HEADER_RIGHT;
			}
			extern nk_symbol_type CastSymbol(const std::string& Value)
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
				Bind("active", [this]() { Active = GetBoolean("active", true); });
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
				bool Stated = Continue;

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
				
				BuildEnd(Base->Engine, Stated);
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
            std::unordered_map<std::string, Element*>* Element::GetContextUniques()
            {
			    return &Base->Uniques;
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
			void Widget::GetWidgetBounds(float* X, float* Y, float* W, float* H)
			{
				struct nk_rect Bounds = nk_widget_bounds(Base->GetContext());
				if (X != nullptr)
					*X = Bounds.x;

				if (Y != nullptr)
					*Y = Bounds.y;

				if (W != nullptr)
					*W = Bounds.w;

				if (H != nullptr)
					*H = Bounds.h;
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
			nk_font* Widget::GetFontHandle()
			{
				return Font;
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
			void Stateful::BuildFont(nk_context* C, nk_style* S)
			{
				if (!Font)
					return;

				if (!S)
				{
					if (!Hash.Count || !Cache)
						Cache = (nk_user_font*)C->style.font;

					nk_style_set_font(C, &Font->handle);
				}
				else if (Cache != nullptr)
					nk_style_set_font(C, Cache);
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
			void Stateful::Pop(uint64_t Count)
			{
				if (!Count)
					Count = Hash.Count;

				if (!Hash.Text.empty() && Count > 0 && Count < Hash.Text.size())
				{
					Hash.Text.erase(Hash.Text.size() - Count, Count);
					Hash.Count -= Count;
				}
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
			void Body::BuildEnd(nk_context* C, bool Stated)
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

				DepthStencil = Device->GetDepthStencilState("none");
				Rasterizer = Device->GetRasterizerState("cull-none-scissor");
				Blend = Device->GetBlendState("additive-source");
				Sampler = Device->GetSamplerState("linear");

				Graphics::InputLayout::Desc Desc;
				Desc.Attributes =
				{
					{ "POSITION", 0, Graphics::AttributeType_Float, 2, (unsigned int)(size_t)(&((Vertex*)0)->Position) },
					{ "TEXCOORD", 0, Graphics::AttributeType_Float, 2, (unsigned int)(size_t)(&((Vertex*)0)->TexCoord) },
					{ "COLOR", 0, Graphics::AttributeType_Float, 4, (unsigned int)(size_t)(&((Vertex*)0)->Color) }
				};
				Layout = Device->CreateInputLayout(Desc);

				Graphics::Shader::Desc I = Graphics::Shader::Desc();
				if (Device->GetSection("pass/gui", &I.Data))
				{
					Shader = Device->CreateShader(I);
					Device->UpdateBufferSize(Shader, sizeof(Compute::Matrix4x4));
				}
				
				FactoryPush<GUI::Divide>("divide");
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
				FactoryPush<GUI::Vector>("vector");
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
				FactoryPush<GUI::DrawLabel>("label");
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
				delete Layout;
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
				static const struct nk_draw_vertex_layout_element VertexLayout[] =
				{
					{ NK_VERTEX_POSITION, NK_FORMAT_FLOAT, NK_OFFSETOF(Vertex, Position) },
					{ NK_VERTEX_TEXCOORD, NK_FORMAT_FLOAT, NK_OFFSETOF(Vertex, TexCoord) },
					{ NK_VERTEX_COLOR, NK_FORMAT_R32G32B32A32_FLOAT, NK_OFFSETOF(Vertex, Color) },
					{ NK_VERTEX_LAYOUT_END }
				};
				Projection = Offset * Projection;

				struct nk_convert_config Converter;
				memset(&Converter, 0, sizeof(Converter));
				Converter.vertex_layout = VertexLayout;
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
				Device->SetInputLayout(Layout);
				Device->SetShader(Shader, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);
				Device->SetBuffer(Shader, 3, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);
				Device->SetVertexBuffer(VertexBuffer, 0);
				Device->SetIndexBuffer(IndexBuffer, Graphics::Format_R16_Uint);
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
			void Context::GetCurrentWidgetBounds(float* X, float* Y, float* W, float* H)
			{
				if (X != nullptr)
					*X = Engine->current->layout->at_x;

				if (Y != nullptr)
					*Y = Engine->current->layout->at_y;

				if (W != nullptr)
					*W = Engine->current->layout->bounds.w;

				if (H != nullptr)
					*H = Engine->current->layout->row.height;
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

				bool HasPath = false;
				for (auto& Group : Array)
				{
					if (Group->IsAttribute())
						continue;

					Rest::Document* Basis = Group->GetAttribute("name");
					if (!Basis || Basis->String.empty())
						continue;

					Rest::Document* Path = Group->FindPath("path.[v]", true);
					HasPath = (Path != nullptr);

					AssetFile* TTF = (HasPath ? Content->Load<AssetFile>(Path->String, nullptr) : nullptr);
					if (!TTF && HasPath)
					{
						THAWK_ERROR("couldn't bake font \"%s\" from path:\n\t%s", Basis->String.c_str(), Path->String.c_str());
						continue;
					}

					FontConfig Desc;
					if (TTF != nullptr)
					{
						Desc.Buffer = TTF->GetBuffer();
						Desc.BufferSize = TTF->GetSize();
					}

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

					if (HasPath)
					{
						if (!FontBake(Basis->String, &Desc))
							THAWK_ERROR("couldn't bake font \"%s\" from path:\n\t%s", Basis->String.c_str(), Path->String.c_str());

						delete TTF;
					}
					else if (!FontBakeDefault(Basis->String, &Desc))
						THAWK_ERROR("couldn't bake default font \"%s\"", Basis->String.c_str());
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
			bool Context::FontBakeDefault(const std::string& Name, FontConfig* Config)
			{
				if (!FontBaking || !Config)
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

				nk_font* Font = nk_font_atlas_add_default(Atlas, Config->Height, &Desc);
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
			bool Context::IsCurrentWidgetHovered()
			{
				struct nk_rect Bounds = Engine->current->layout->bounds;
				Bounds.h = Engine->current->layout->row.height;
				Bounds.x = Engine->current->layout->at_x;
				Bounds.y = Engine->current->layout->at_y;

				return (nk_input_is_mouse_hovering_rect(&Engine->input, Bounds) > 0);
			}
			bool Context::IsCurrentWidgetClicked()
			{
				struct nk_rect Bounds = Engine->current->layout->bounds;
				Bounds.h = Engine->current->layout->row.height;
				Bounds.x = Engine->current->layout->at_x;
				Bounds.y = Engine->current->layout->at_y;
	
				return (nk_input_has_mouse_click_in_rect(&Engine->input, NK_BUTTON_LEFT, Bounds) > 0);
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