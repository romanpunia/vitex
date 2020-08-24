#ifndef THAWK_ENGINE_GUI_ELEMENTS_H
#define THAWK_ENGINE_GUI_ELEMENTS_H

#include "context.h"

namespace Tomahawk
{
	namespace Engine
	{
		namespace GUI
		{
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
					double Range;
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
					bool TextValue;
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
				void SetLayer(Combo* New);
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
					bool AutoToggle;
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
				void SetLayer(Contextual* New);
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
				void SetLayer(Header* New);
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
				void SetLayer(HeaderTab* New);
				bool IsClicked();
			};
		}
	}
}
#endif