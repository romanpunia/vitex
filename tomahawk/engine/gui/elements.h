#ifndef THAWK_ENGINE_GUI_ELEMENTS_H
#define THAWK_ENGINE_GUI_ELEMENTS_H

#include "context.h"

namespace Tomahawk
{
	namespace Engine
	{
		namespace GUI
		{
			enum LayoutType
			{
				LayoutType_Dynamic,
				LayoutType_Static,
				LayoutType_Flex,
				LayoutType_Space
			};

			enum PushType
			{
				PushType_Dynamic,
				PushType_Static,
				PushType_Variable
			};

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

			enum ButtonType
			{
				ButtonType_Text,
				ButtonType_Color,
				ButtonType_Symbol,
				ButtonType_Symbol_Text,
				ButtonType_Image,
				ButtonType_Image_Text
			};

			enum TextType
			{
				TextType_Label,
				TextType_Label_Colored,
				TextType_Wrap,
				TextType_Wrap_Colored
			};

			enum EditType
			{
				EditType_Default,
				EditType_ASCII,
				EditType_Float,
				EditType_Decimal,
				EditType_Hex,
				EditType_Oct,
				EditType_Binary,
				EditType_Password
			};

			enum TreeType
			{
				TreeType_Node,
				TreeType_Tab
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

			class THAWK_OUT Panel : public Element
			{
			private:
				nk_user_font* Cache;
				nk_style_window* Style;

			public:
				struct
				{
					nk_font* Font;
					float Width, Height;
					float X, Y;
					std::string Text;
					uint32_t Flags;
					bool Relative;
				} Source;

			public:
				Panel(Context* View);
				virtual ~Panel() override;
				virtual bool BuildBegin(nk_context* C) override;
				virtual void BuildEnd(nk_context* C, bool Stated) override;
				virtual void BuildStyle(nk_context* C, nk_style* S) override;
				virtual float GetWidth() override;
				virtual float GetHeight() override;
				virtual std::string GetType() override;
				void SetPosition(const Compute::Vector2& Position);
				void SetSize(const Compute::Vector2& Size);
				void SetFocus();
				void SetScroll(const Compute::Vector2& Offset);
				void Close();
				void Collapse(bool Maximized);
				void Show(bool Shown);
				float GetClientWidth();
				float GetClientHeight();
				bool HasFocus();
				bool IsHovered();
				bool IsCollapsed();
				bool IsHidden();
				bool IsFocusActive();
				Compute::Vector2 GetPosition();
				Compute::Vector2 GetSize();
				Compute::Vector2 GetContentRegionMin();
				Compute::Vector2 GetContentRegionMax();
				Compute::Vector2 GetContentRegionSize();
				Compute::Vector2 GetScroll();
			};

			class THAWK_OUT PreLayout : public Element
			{
			public:
				struct
				{
					float Height;
					bool Relative;
				} Source;

			public:
				PreLayout(Context* View);
				virtual ~PreLayout() = default;
				virtual bool BuildBegin(nk_context* C) override;
				virtual void BuildEnd(nk_context* C, bool Stated) override;
				virtual void BuildStyle(nk_context* C, nk_style* S) override;
				virtual float GetWidth() override;
				virtual float GetHeight() override;
				virtual std::string GetType() override;
			};

			class THAWK_OUT PrePush : public Element
			{
			private:
				struct
				{
					PreLayout* Layer;
				} State;

			public:
				struct
				{
					PushType Type;
					float MinWidth;
					float Width;
					bool Relative;
				} Source;

			public:
				PrePush(Context* View);
				virtual ~PrePush() = default;
				virtual bool BuildBegin(nk_context* C) override;
				virtual void BuildEnd(nk_context* C, bool Stated) override;
				virtual void BuildStyle(nk_context* C, nk_style* S) override;
				virtual float GetWidth() override;
				virtual float GetHeight() override;
				virtual std::string GetType() override;
			};

			class THAWK_OUT Layout : public Element
			{
			public:
				struct
				{
					LayoutType Type;
					unsigned int Format;
					float Width;
					float Height;
					int Size;
					bool Relative;
				} Source;

			public:
				Layout(Context* View);
				virtual ~Layout() = default;
				virtual bool BuildBegin(nk_context* C) override;
				virtual void BuildEnd(nk_context* C, bool Stated) override;
				virtual void BuildStyle(nk_context* C, nk_style* S) override;
				virtual float GetWidth() override;
				virtual float GetHeight() override;
				virtual std::string GetType() override;
			};

			class THAWK_OUT Push : public Element
			{
			private:
				struct
				{
					Layout* Layer;
				} State;

			public:
				struct
				{
					float Width;
					float Height;
					float X, Y;
					bool Relative;
				} Source;

			public:
				Push(Context* View);
				virtual ~Push() = default;
				virtual bool BuildBegin(nk_context* C) override;
				virtual void BuildEnd(nk_context* C, bool Stated) override;
				virtual void BuildStyle(nk_context* C, nk_style* S) override;
				virtual float GetWidth() override;
				virtual float GetHeight() override;
				virtual std::string GetType() override;
			};

			class THAWK_OUT Button : public Widget
			{
			private:
				nk_style_button* Style;

			private:
				struct
				{
					bool Clicked;
				} State;

			public:
				struct
				{
					Graphics::Texture2D* Image;
					Compute::Vector4 Color;
					std::string Text;
					uint8_t Symbol;
					uint8_t Align;
					ButtonType Type;
					bool Repeat;
				} Source;

			public:
				Button(Context* View);
				virtual ~Button() override;
				virtual bool BuildBegin(nk_context* C) override;
				virtual void BuildEnd(nk_context* C, bool Stated) override;
				virtual void BuildStyle(nk_context* C, nk_style* S) override;
				virtual float GetWidth() override;
				virtual float GetHeight() override;
				virtual std::string GetType() override;
				bool IsClicked();
			};

			class THAWK_OUT Text : public Widget
			{
			private:
				nk_style_text* Style;

			public:
				struct
				{
					Compute::Vector4 Color;
					std::string Text;
					uint8_t Align;
					TextType Type;
				} Source;

			public:
				Text(Context* View);
				virtual ~Text() override;
				virtual bool BuildBegin(nk_context* C) override;
				virtual void BuildEnd(nk_context* C, bool Stated) override;
				virtual void BuildStyle(nk_context* C, nk_style* S) override;
				virtual float GetWidth() override;
				virtual float GetHeight() override;
				virtual std::string GetType() override;
			};

			class THAWK_OUT Image : public Widget
			{
			public:
				struct
				{
					Graphics::Texture2D* Image;
					Compute::Vector4 Color;
					TextType Type;
				} Source;

			public:
				Image(Context* View);
				virtual ~Image() override;
				virtual bool BuildBegin(nk_context* C) override;
				virtual void BuildEnd(nk_context* C, bool Stated) override;
				virtual void BuildStyle(nk_context* C, nk_style* S) override;
				virtual float GetWidth() override;
				virtual float GetHeight() override;
				virtual std::string GetType() override;
			};

			class THAWK_OUT Checkbox : public Widget
			{
			private:
				nk_style_toggle* Style;

			private:
				struct
				{
					int Value;
					bool Toggled;
				} State;

			public:
				struct
				{
					std::string Text;
					std::string Bind;
				} Source;

			public:
				Checkbox(Context* View);
				virtual ~Checkbox() override;
				virtual bool BuildBegin(nk_context* C) override;
				virtual void BuildEnd(nk_context* C, bool Stated) override;
				virtual void BuildStyle(nk_context* C, nk_style* S) override;
				virtual float GetWidth() override;
				virtual float GetHeight() override;
				virtual std::string GetType() override;
				void SetValue(bool Value);
				bool GetValue();
				bool IsToggled();
			};

			class THAWK_OUT Radio : public Widget
			{
			private:
				nk_style_toggle* Style;

			private:
				struct
				{
					int Value;
					bool Toggled;
				} State;

			public:
				struct
				{
					std::string Text;
					std::string Bind;
				} Source;

			public:
				Radio(Context* View);
				virtual ~Radio() override;
				virtual bool BuildBegin(nk_context* C) override;
				virtual void BuildEnd(nk_context* C, bool Stated) override;
				virtual void BuildStyle(nk_context* C, nk_style* S) override;
				virtual float GetWidth() override;
				virtual float GetHeight() override;
				virtual std::string GetType() override;
				void SetValue(bool Value);
				bool GetValue();
				bool IsToggled();
			};

			class THAWK_OUT Selectable : public Widget
			{
			private:
				nk_style_selectable* Style;

			private:
				struct
				{
					int Value;
					bool Clicked;
				} State;

			public:
				struct
				{
					Graphics::Texture2D* Image;
					std::string Text;
					std::string Bind;
					uint8_t Symbol;
					uint8_t Align;
					ButtonType Type;
				} Source;

			public:
				Selectable(Context* View);
				virtual ~Selectable() override;
				virtual bool BuildBegin(nk_context* C) override;
				virtual void BuildEnd(nk_context* C, bool Stated) override;
				virtual void BuildStyle(nk_context* C, nk_style* S) override;
				virtual float GetWidth() override;
				virtual float GetHeight() override;
				virtual std::string GetType() override;
				void SetValue(bool Value);
				bool GetValue();
				bool IsClicked();
			};

			class THAWK_OUT Slider : public Widget
			{
			private:
				nk_style_slider* Style;

			private:
				struct
				{
					float Value;
					bool Clicked;
				} State;

			public:
				struct
				{
					std::string Bind;
					float Min, Max;
					float Step;
					bool Decimal;
				} Source;

			public:
				Slider(Context* View);
				virtual ~Slider() override;
				virtual bool BuildBegin(nk_context* C) override;
				virtual void BuildEnd(nk_context* C, bool Stated) override;
				virtual void BuildStyle(nk_context* C, nk_style* S) override;
				virtual float GetWidth() override;
				virtual float GetHeight() override;
				virtual std::string GetType() override;
				void SetValue(float Value);
				float GetValue();
				bool IsClicked();
			};

			class THAWK_OUT Progress : public Widget
			{
			private:
				nk_style_progress* Style;

			private:
				struct
				{
					uintptr_t Value;
					bool Clicked;
				} State;

			public:
				struct
				{
					std::string Bind;
					bool Changeable;
					uintptr_t Max;
				} Source;

			public:
				Progress(Context* View);
				virtual ~Progress() override;
				virtual bool BuildBegin(nk_context* C) override;
				virtual void BuildEnd(nk_context* C, bool Stated) override;
				virtual void BuildStyle(nk_context* C, nk_style* S) override;
				virtual float GetWidth() override;
				virtual float GetHeight() override;
				virtual std::string GetType() override;
				void SetValue(uintptr_t Value);
				uintptr_t GetValue();
				bool IsClicked();
			};

			class THAWK_OUT ColorPicker : public Widget
			{
			private:
				struct
				{
					Compute::Vector4 Value;
					bool Clicked;
				} State;

			public:
				struct
				{
					std::string Bind;
					bool Alpha;
				} Source;

			public:
				ColorPicker(Context* View);
				virtual ~ColorPicker() override;
				virtual bool BuildBegin(nk_context* C) override;
				virtual void BuildEnd(nk_context* C, bool Stated) override;
				virtual void BuildStyle(nk_context* C, nk_style* S) override;
				virtual float GetWidth() override;
				virtual float GetHeight() override;
				virtual std::string GetType() override;
				void SetValue(const Compute::Vector4& Value);
				Compute::Vector4 GetValue();
				bool IsClicked();
			};

			class THAWK_OUT Property : public Widget
			{
			private:
				nk_style_property* Style;

			private:
				struct
				{
					double Value;
				} State;

			public:
				struct
				{
					std::string Text;
					std::string Bind;
					double Min, Max;
					double Step;
					float PPI;
					bool Decimal;
				} Source;

			public:
				Property(Context* View);
				virtual ~Property() override;
				virtual bool BuildBegin(nk_context* C) override;
				virtual void BuildEnd(nk_context* C, bool Stated) override;
				virtual void BuildStyle(nk_context* C, nk_style* S) override;
				virtual float GetWidth() override;
				virtual float GetHeight() override;
				virtual std::string GetType() override;
				void SetValue(double Value);
				double GetValue();
			};

			class THAWK_OUT Edit : public Widget
			{
			private:
				nk_style_edit* Style;

			private:
				struct
				{
					std::string Value;
					std::string Buffer;
					uint32_t Flags;
				} State;

			public:
				struct
				{
					std::string Text;
					std::string Bind;
					EditType Type;
					size_t MaxSize;
					uint32_t Flags;
				} Source;

			public:
				Edit(Context* View);
				virtual ~Edit() override;
				virtual bool BuildBegin(nk_context* C) override;
				virtual void BuildEnd(nk_context* C, bool Stated) override;
				virtual void BuildStyle(nk_context* C, nk_style* S) override;
				virtual float GetWidth() override;
				virtual float GetHeight() override;
				virtual std::string GetType() override;
				void SetValue(const std::string& Value);
				std::string GetValue();
				bool IsTextChanged();
				bool IsTextActive();
				bool IsTextActivated();
				bool IsTextDeactivated();
				void Focus();
				void Unfocus();
			};

			class THAWK_OUT Combo : public Widget
			{
			private:
				nk_style_combo* Style;

			private:
				struct
				{
					std::string Value;
				} State;

			public:
				struct
				{
					Graphics::Texture2D* Image;
					Compute::Vector4 Color;
					std::string Bind;
					std::string Text;
					ButtonType Type;
					uint8_t Symbol;
					float Width;
					float Height;
				} Source;

			public:
				Combo(Context* View);
				virtual ~Combo() override;
				virtual bool BuildBegin(nk_context* C) override;
				virtual void BuildEnd(nk_context* C, bool Stated) override;
				virtual void BuildStyle(nk_context* C, nk_style* S) override;
				virtual float GetWidth() override;
				virtual float GetHeight() override;
				virtual std::string GetType() override;
				void SetValue(const std::string& Value);
				std::string GetValue();
			};

			class THAWK_OUT ComboItem : public Element
			{
			private:
				struct
				{
					Combo* Layer;
					bool Clicked;
				} State;

			public:
				struct
				{
					Graphics::Texture2D* Image;
					std::string Text;
					ButtonType Type;
					uint8_t Symbol;
					uint8_t Align;
				} Source;

			public:
				ComboItem(Context* View);
				virtual ~ComboItem() = default;
				virtual bool BuildBegin(nk_context* C) override;
				virtual void BuildEnd(nk_context* C, bool Stated) override;
				virtual void BuildStyle(nk_context* C, nk_style* S) override;
				virtual float GetWidth() override;
				virtual float GetHeight() override;
				virtual std::string GetType() override;
				bool IsClicked();
			};

			class THAWK_OUT Vector : public Widget
			{
			private:
				struct
				{
					nk_style_combo* Combo;
					nk_style_property* Property;
				} Style;

			private:
				struct
				{
					Compute::Vector4 Value;
				} State;

			public:
				struct
				{
					Graphics::Texture2D* Image;
					Compute::Vector4 Color;
					std::string Bind;
					std::string Text;
					ButtonType Type;
					uint8_t Symbol;
					uint32_t Count;
					float Step;
					float PPI;
					float Width;
					float Height;
				} Source;

			public:
				Vector(Context* View);
				virtual ~Vector() override;
				virtual bool BuildBegin(nk_context* C) override;
				virtual void BuildEnd(nk_context* C, bool Stated) override;
				virtual void BuildStyle(nk_context* C, nk_style* S) override;
				virtual float GetWidth() override;
				virtual float GetHeight() override;
				virtual std::string GetType() override;
				void SetValue(const Compute::Vector4& Value);
				Compute::Vector4 GetValue();
			};

			class THAWK_OUT Group : public Stateful
			{
			private:
				nk_style_window* Style;

			public:
				struct
				{
					std::string Text;
					uint32_t OffsetX;
					uint32_t OffsetY;
					uint32_t Flags;
					bool Scrolled;
				} Source;

			public:
				Group(Context* View);
				virtual ~Group() override;
				virtual bool BuildBegin(nk_context* C) override;
				virtual void BuildEnd(nk_context* C, bool Stated) override;
				virtual void BuildStyle(nk_context* C, nk_style* S) override;
				virtual float GetWidth() override;
				virtual float GetHeight() override;
				virtual std::string GetType() override;
			};

			class THAWK_OUT Tree : public Stateful
			{
			private:
				struct Internal
				{
					int Value = 0;
					int Selected = 0;
					bool Hovered = false;
				};

			private:
				struct
				{
					nk_style_selectable* Selectable;
					nk_style_tab* Tab;
				} Style;

			protected:
				struct
				{
					std::unordered_map<std::string, Internal> Data;
					Internal Default;
				} State;

			public:
				struct
				{
					Graphics::Texture2D* Image;
					std::string Text;
					std::string Bind;
					TreeType Type;
					bool Minimized;
					bool Selectable;
				} Source;

			public:
				Tree(Context* View);
				virtual ~Tree() override;
				virtual bool BuildBegin(nk_context* C) override;
				virtual void BuildEnd(nk_context* C, bool Stated) override;
				virtual void BuildStyle(nk_context* C, nk_style* S) override;
				virtual float GetWidth() override;
				virtual float GetHeight() override;
				virtual std::string GetType() override;
				void SetValue(bool Value);
				bool GetValue();
				bool IsClicked();
				bool IsHovered();

			private:
				Internal* GetState();
			};

			class THAWK_OUT Popup : public Widget
			{
			public:
				struct
				{
					std::string Text;
					uint64_t Flags;
					float Width, Height;
					float X, Y;
					bool Dynamic;
				} Source;

			public:
				Popup(Context* View);
				virtual ~Popup() override;
				virtual bool BuildBegin(nk_context* C) override;
				virtual void BuildEnd(nk_context* C, bool Stated) override;
				virtual void BuildStyle(nk_context* C, nk_style* S) override;
				virtual float GetWidth() override;
				virtual float GetHeight() override;
				virtual std::string GetType() override;
				void Close();
				void SetScrollOffset(const Compute::Vector2& Offset);
				Compute::Vector2 GetScrollOffset();
			};

			class THAWK_OUT Contextual : public Widget
			{
			public:
				struct
				{
					float Width, Height;
					float TriggerWidth;
					float TriggerHeight;
					float TriggerX;
					float TriggerY;
					uint64_t Flags;
					bool Relative;
				} Source;

			public:
				Contextual(Context* View);
				virtual ~Contextual() override;
				virtual bool BuildBegin(nk_context* C) override;
				virtual void BuildEnd(nk_context* C, bool Stated) override;
				virtual void BuildStyle(nk_context* C, nk_style* S) override;
				virtual float GetWidth() override;
				virtual float GetHeight() override;
				virtual std::string GetType() override;
				void Close();
			};

			class THAWK_OUT ContextualItem : public Widget
			{
			private:
				nk_style_button* Style;

			private:
				struct
				{
					Contextual* Layer;
					bool Clicked;
				} State;

			public:
				struct
				{
					Graphics::Texture2D* Image;
					std::string Text;
					uint8_t Symbol;
					uint8_t Align;
					ButtonType Type;
				} Source;

			public:
				ContextualItem(Context* View);
				virtual ~ContextualItem() override;
				virtual bool BuildBegin(nk_context* C) override;
				virtual void BuildEnd(nk_context* C, bool Stated) override;
				virtual void BuildStyle(nk_context* C, nk_style* S) override;
				virtual float GetWidth() override;
				virtual float GetHeight() override;
				virtual std::string GetType() override;
				bool IsClicked();
			};

			class THAWK_OUT Tooltip : public Widget
			{
			public:
				struct
				{
					std::string Text;
					float Width;
					bool Sub;
				} Source;

			public:
				Tooltip(Context* View);
				virtual ~Tooltip() override;
				virtual bool BuildBegin(nk_context* C) override;
				virtual void BuildEnd(nk_context* C, bool Stated) override;
				virtual void BuildStyle(nk_context* C, nk_style* S) override;
				virtual float GetWidth() override;
				virtual float GetHeight() override;
				virtual std::string GetType() override;
			};

			class THAWK_OUT Header : public Widget
			{
			public:
				Header(Context* View);
				virtual ~Header() override;
				virtual bool BuildBegin(nk_context* C) override;
				virtual void BuildEnd(nk_context* C, bool Stated) override;
				virtual void BuildStyle(nk_context* C, nk_style* S) override;
				virtual float GetWidth() override;
				virtual float GetHeight() override;
				virtual std::string GetType() override;
			};

			class THAWK_OUT HeaderTab : public Widget
			{
			private:
				nk_style_button* Style;

			private:
				struct
				{
					Header* Layer;
				} State;

			public:
				struct
				{
					Graphics::Texture2D* Image;
					std::string Text;
					uint8_t Symbol;
					uint8_t Align;
					ButtonType Type;
					float Width;
					float Height;
				} Source;

			public:
				HeaderTab(Context* View);
				virtual ~HeaderTab() override;
				virtual bool BuildBegin(nk_context* C) override;
				virtual void BuildEnd(nk_context* C, bool Stated) override;
				virtual void BuildStyle(nk_context* C, nk_style* S) override;
				virtual float GetWidth() override;
				virtual float GetHeight() override;
				virtual std::string GetType() override;
			};

			class THAWK_OUT HeaderItem : public Widget
			{
			private:
				nk_style_button* Style;

			private:
				struct
				{
					HeaderTab* Layer;
					bool Clicked;
				} State;

			public:
				struct
				{
					Graphics::Texture2D* Image;
					std::string Text;
					uint8_t Symbol;
					uint8_t Align;
					ButtonType Type;
				} Source;

			public:
				HeaderItem(Context* View);
				virtual ~HeaderItem() override;
				virtual bool BuildBegin(nk_context* C) override;
				virtual void BuildEnd(nk_context* C, bool Stated) override;
				virtual void BuildStyle(nk_context* C, nk_style* S) override;
				virtual float GetWidth() override;
				virtual float GetHeight() override;
				virtual std::string GetType() override;
				bool IsClicked();
			};

			class THAWK_OUT DrawLine : public Element
			{
			public:
				struct
				{
					Compute::Vector4 Color;
					float Thickness;
					float X1, Y1;
					float X2, Y2;
					bool Relative;
				} Source;

			public:
				DrawLine(Context* View);
				virtual ~DrawLine() override;
				virtual bool BuildBegin(nk_context* C) override;
				virtual void BuildEnd(nk_context* C, bool Stated) override;
				virtual void BuildStyle(nk_context* C, nk_style* S) override;
				virtual float GetWidth() override;
				virtual float GetHeight() override;
				virtual std::string GetType() override;
			};

			class THAWK_OUT DrawCurve : public Element
			{
			public:
				struct
				{
					Compute::Vector4 Color;
					float Thickness;
					float X1, Y1;
					float X2, Y2;
					float X3, Y3;
					float X4, Y4;
					bool Relative;
				} Source;

			public:
				DrawCurve(Context* View);
				virtual ~DrawCurve() override;
				virtual bool BuildBegin(nk_context* C) override;
				virtual void BuildEnd(nk_context* C, bool Stated) override;
				virtual void BuildStyle(nk_context* C, nk_style* S) override;
				virtual float GetWidth() override;
				virtual float GetHeight() override;
				virtual std::string GetType() override;
			};

			class THAWK_OUT DrawRect : public Element
			{
			public:
				struct
				{
					Compute::Vector4 Left;
					Compute::Vector4 Top;
					Compute::Vector4 Right;
					Compute::Vector4 Bottom;
					float Thickness;
					float Radius;
					float Width;
					float Height;
					float X, Y;
					bool Gradient;
					bool Fill;
					bool Relative;
				} Source;

			public:
				DrawRect(Context* View);
				virtual ~DrawRect() override;
				virtual bool BuildBegin(nk_context* C) override;
				virtual void BuildEnd(nk_context* C, bool Stated) override;
				virtual void BuildStyle(nk_context* C, nk_style* S) override;
				virtual float GetWidth() override;
				virtual float GetHeight() override;
				virtual std::string GetType() override;
			};

			class THAWK_OUT DrawCircle : public Element
			{
			public:
				struct
				{
					Compute::Vector4 Color;
					float Thickness;
					float Width;
					float Height;
					float X, Y;
					bool Fill;
					bool Relative;
				} Source;

			public:
				DrawCircle(Context* View);
				virtual ~DrawCircle() override;
				virtual bool BuildBegin(nk_context* C) override;
				virtual void BuildEnd(nk_context* C, bool Stated) override;
				virtual void BuildStyle(nk_context* C, nk_style* S) override;
				virtual float GetWidth() override;
				virtual float GetHeight() override;
				virtual std::string GetType() override;
			};

			class THAWK_OUT DrawArc : public Element
			{
			public:
				struct
				{
					Compute::Vector4 Color;
					float Thickness;
					float Radius;
					float Min;
					float Max;
					float X, Y;
					bool Fill;
					bool Relative;
				} Source;

			public:
				DrawArc(Context* View);
				virtual ~DrawArc() override;
				virtual bool BuildBegin(nk_context* C) override;
				virtual void BuildEnd(nk_context* C, bool Stated) override;
				virtual void BuildStyle(nk_context* C, nk_style* S) override;
				virtual float GetWidth() override;
				virtual float GetHeight() override;
				virtual std::string GetType() override;
			};

			class THAWK_OUT DrawTriangle : public Element
			{
			public:
				struct
				{
					Compute::Vector4 Color;
					float Thickness;
					float X1, Y1;
					float X2, Y2;
					float X3, Y3;
					bool Fill;
					bool Relative;
				} Source;

			public:
				DrawTriangle(Context* View);
				virtual ~DrawTriangle() override;
				virtual bool BuildBegin(nk_context* C) override;
				virtual void BuildEnd(nk_context* C, bool Stated) override;
				virtual void BuildStyle(nk_context* C, nk_style* S) override;
				virtual float GetWidth() override;
				virtual float GetHeight() override;
				virtual std::string GetType() override;
			};

			class THAWK_OUT DrawPolyline : public Element
			{
			private:
				struct
				{
					std::vector<float> Points;
				} State;

			public:
				struct
				{
					Compute::Vector4 Color;
					std::vector<float> Points;
					float Thickness;
					bool Relative;
				} Source;

			public:
				DrawPolyline(Context* View);
				virtual ~DrawPolyline() override;
				virtual bool BuildBegin(nk_context* C) override;
				virtual void BuildEnd(nk_context* C, bool Stated) override;
				virtual void BuildStyle(nk_context* C, nk_style* S) override;
				virtual float GetWidth() override;
				virtual float GetHeight() override;
				virtual std::string GetType() override;
			};

			class THAWK_OUT DrawPolygon : public Element
			{
			private:
				struct
				{
					std::vector<float> Points;
				} State;

			public:
				struct
				{
					Compute::Vector4 Color;
					std::vector<float> Points;
					float Thickness;
					bool Fill;
					bool Relative;
				} Source;

			public:
				DrawPolygon(Context* View);
				virtual ~DrawPolygon() override;
				virtual bool BuildBegin(nk_context* C) override;
				virtual void BuildEnd(nk_context* C, bool Stated) override;
				virtual void BuildStyle(nk_context* C, nk_style* S) override;
				virtual float GetWidth() override;
				virtual float GetHeight() override;
				virtual std::string GetType() override;
			};

			class THAWK_OUT DrawLabel : public Widget
			{
			public:
				struct
				{
					Compute::Vector4 Background;
					Compute::Vector4 Color;
					std::string Text;
					float Width;
					float Height;
					float X, Y;
					bool Relative;
				} Source;

			public:
				DrawLabel(Context* View);
				virtual ~DrawLabel() override;
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