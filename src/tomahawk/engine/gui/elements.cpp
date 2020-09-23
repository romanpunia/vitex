#include "elements.h"
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

			Panel::Panel(Context* View) : Element(View), Cache(nullptr)
			{
				Style = (nk_style_window*)TH_MALLOC(sizeof(nk_style_window));
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
				TH_FREE(Style);
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

				return nk_begin(C, Source.Text.c_str(), Bounds, Source.Flags);
			}
			void Panel::BuildEnd(nk_context* C, bool Stated)
			{
				if (!Base->HasOverflow() && C->current && C->current->layout)
				{
					if (C->current->flags & NK_WINDOW_ROM || C->current->layout->flags & NK_WINDOW_ROM)
						C->current->layout->flags |= NK_WINDOW_REMOVE_ROM;
				}
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

			Button::Button(Context* View) : Widget(View)
			{
				Style = (nk_style_button*)TH_MALLOC(sizeof(nk_style_button));
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
				Bind("image-align", [this]() { Source.Align = CastTextAlign(GetString("image-align", "middle left")); });
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
				TH_FREE(Style);
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
			void Button::BuildEnd(nk_context* C, bool Stated)
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
				Style = (nk_style_text*)TH_MALLOC(sizeof(nk_style_text));
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
				Bind("background", [this]() { Source.Color = GetV4("background", { 175, 175, 175, 255 }); });
				Bind("align", [this]() { Source.Align = CastTextAlign(GetString("align", "middle left")); });
				Bind("color", [this]() { Style->color = CastV4(GetV4("color", { 175, 175, 175, 255 })); });
				Bind("padding", [this]() { Style->padding = CastV2(GetV2("padding", { 0.0f, 0.0f })); });
			}
			Text::~Text()
			{
				TH_FREE(Style);
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
			void Text::BuildEnd(nk_context* C, bool Stated)
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
			void Image::BuildEnd(nk_context* C, bool Stated)
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
				Style = (nk_style_toggle*)TH_MALLOC(sizeof(nk_style_toggle));
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
				TH_FREE(Style);
			}
			bool Checkbox::BuildBegin(nk_context* C)
			{
				State.Toggled = nk_checkbox_text(C, Source.Text.c_str(), Source.Text.size(), &State.Value);
				if (State.Toggled && !Source.Bind.empty())
					SetGlobal(Source.Bind, State.Value ? "1" : "0");

				return State.Toggled;
			}
			void Checkbox::BuildEnd(nk_context* C, bool Stated)
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
				Style = (nk_style_toggle*)TH_MALLOC(sizeof(nk_style_toggle));
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
				TH_FREE(Style);
			}
			bool Radio::BuildBegin(nk_context* C)
			{
				State.Toggled = nk_radio_text(C, Source.Text.c_str(), Source.Text.size(), &State.Value);
				if (State.Toggled && !Source.Bind.empty())
					SetGlobal(Source.Bind, State.Value ? "1" : "0");

				return State.Toggled;
			}
			void Radio::BuildEnd(nk_context* C, bool Stated)
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
				Style = (nk_style_selectable*)TH_MALLOC(sizeof(nk_style_selectable));
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
				Bind("image-align", [this]() { Source.Align = CastTextAlign(GetString("image-align", "middle left")); });
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
				TH_FREE(Style);
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
			void Selectable::BuildEnd(nk_context* C, bool Stated)
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
				Style = (nk_style_slider*)TH_MALLOC(sizeof(nk_style_slider));
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
				TH_FREE(Style);
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
			void Slider::BuildEnd(nk_context* C, bool Stated)
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
				Style = (nk_style_progress*)TH_MALLOC(sizeof(nk_style_progress));
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
				TH_FREE(Style);
			}
			bool Progress::BuildBegin(nk_context* C)
			{
				State.Clicked = nk_progress(C, &State.Value, Source.Max, Source.Changeable);
				if (State.Clicked && !Source.Bind.empty())
					SetGlobal(Source.Bind, std::to_string(State.Value));

				return State.Clicked;
			}
			void Progress::BuildEnd(nk_context* C, bool Stated)
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
				nk_colorf Color = nk_color_cf(CastV4(State.Value * 255.0f));
				State.Clicked = nk_color_pick(C, &Color, Source.Alpha ? NK_RGBA : NK_RGB);

				if (!State.Clicked)
					return false;

				nk_color Cast = nk_rgba_cf(Color);
				State.Value.X = (float)Cast.r / 255.0f;
				State.Value.Y = (float)Cast.g / 255.0f;
				State.Value.Z = (float)Cast.b / 255.0f;
				State.Value.W = (float)Cast.a / 255.0f;

				if (Source.Bind.empty())
					return true;
				
				SetGlobal(Source.Bind, Rest::Form("%f %f %f %f", State.Value.X, State.Value.Y, State.Value.Z, State.Value.W).R());
				return true;
			}
			void ColorPicker::BuildEnd(nk_context* C, bool Stated)
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
				Style = (nk_style_property*)TH_MALLOC(sizeof(nk_style_property));
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
				Bind("min", [this]() { Source.Min = (double)GetFloat("min", -std::numeric_limits<float>::max()); });
				Bind("max", [this]() { Source.Max = (double)GetFloat("max", std::numeric_limits<float>::max()); });
				Bind("range", [this]() { Source.Range = (double)GetFloat("range", 100.0f); });
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
				TH_FREE(Style);
			}
			bool Property::BuildBegin(nk_context* C)
			{
				double PPI = abs(Source.Min - Source.Max) / Source.Range;
				double Last = State.Value;

				if (Source.Decimal)
				{
					if ((int)PPI < 1)
						PPI = 1.0;

					int Value = (int)State.Value;
					nk_property_int(C, Source.Text.c_str(), (int)Source.Min, &Value, (int)Source.Max, (int)PPI, (float)PPI);
					State.Value = (double)Value;
				}
				else
					nk_property_double(C, Source.Text.c_str(), Source.Min, &State.Value, Source.Max, PPI, (float)PPI);

				if (Last != State.Value && !Source.Bind.empty())
					SetGlobal(Source.Bind, Source.Decimal ? std::to_string((int)State.Value) : std::to_string(State.Value));

				return Last != State.Value;
			}
			void Property::BuildEnd(nk_context* C, bool Stated)
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
				Style = (nk_style_edit*)TH_MALLOC(sizeof(nk_style_edit));
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
				TH_FREE(Style);
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
			void Edit::BuildEnd(nk_context* C, bool Stated)
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
				Style = (nk_style_combo*)TH_MALLOC(sizeof(nk_style_combo));
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
				Bind("text", [this]() { Source.Text = GetString("text", ""); });
				Bind("text-value", [this]() { Source.TextValue = GetBoolean("text-value", false); });
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
				Bind("content-padding", [this]() { Style->content_padding = CastV2(GetV2("content-padding", { 4, 0 })); });
				Bind("button-padding", [this]() { Style->button_padding = CastV2(GetV2("button-padding", { 0, 0 })); });
				Bind("spacing", [this]() { Style->spacing = CastV2(GetV2("spacing", { 4, 0 })); });
				Bind("border-size", [this]() { Style->border = GetFloat("border-size", 1); });
				Bind("border-radius", [this]() { Style->rounding = GetFloat("border-radius", 0); });
				Bind("exp-normal", [this]() { Style->button.normal = CastV4Item(Content, GetString("exp-normal", "0 0 0 0")); });
				Bind("exp-hover", [this]() { Style->button.hover = CastV4Item(Content, GetString("exp-hover", "0 0 0 0")); });
				Bind("exp-active", [this]() { Style->button.active = CastV4Item(Content, GetString("exp-active", "0 0 0 0")); });
				Bind("exp-border", [this]() { Style->button.border_color = CastV4(GetV4("exp-border", { 65, 65, 65, 255 })); });
				Bind("exp-text-background", [this]() { Style->button.text_background = CastV4(GetV4("exp-text-background", { 0, 0, 0, 0 })); });
				Bind("exp-text-normal", [this]() { Style->button.text_normal = CastV4(GetV4("exp-text-normal", { 175, 175, 175, 255 })); });
				Bind("exp-text-hover", [this]() { Style->button.text_hover = CastV4(GetV4("exp-text-hover", { 175, 175, 175, 255 })); });
				Bind("exp-text-active", [this]() { Style->button.text_active = CastV4(GetV4("exp-text-active", { 175, 175, 175, 255 })); });
				Bind("exp-padding", [this]() { Style->button.padding = CastV2(GetV2("exp-padding", { 2.0f, 2.0f })); });
				Bind("exp-image-padding", [this]() { Style->button.image_padding = CastV2(GetV2("exp-image-padding", { 0, 0 })); });
				Bind("exp-touch-padding", [this]() { Style->button.touch_padding = CastV2(GetV2("exp-touch-padding", { 0, 0 })); });
				Bind("exp-text-align", [this]() { Style->button.text_alignment = CastTextAlign(GetString("exp-text-align", "middle center")); });
				Bind("exp-border-size", [this]() { Style->button.border = GetFloat("exp-border-size", 0); });
				Bind("exp-border-radius", [this]() { Style->button.rounding = GetFloat("exp-border-radius", 0); });
			}
			Combo::~Combo()
			{
				TH_FREE(Style);
			}
			bool Combo::BuildBegin(nk_context* C)
			{
				bool Stated = false;
				switch (Source.Type)
				{
					case ButtonType_Color:
						Stated = nk_combo_begin_color(C, CastV4(Source.Color), nk_vec2(Source.Width, Source.Height));
						break;
					case ButtonType_Symbol:
						Stated = nk_combo_begin_symbol(C, (nk_symbol_type)Source.Symbol, nk_vec2(Source.Width, Source.Height));
						break;
					case ButtonType_Symbol_Text:
						Stated = nk_combo_begin_symbol_text(C, Source.Text.c_str(), Source.Text.size(), (nk_symbol_type)Source.Symbol, nk_vec2(Source.Width, Source.Height));
						break;
					case ButtonType_Image:
						Stated = nk_combo_begin_image(C, nk_image_auto(Source.Image), nk_vec2(Source.Width, Source.Height));
						break;
					case ButtonType_Image_Text:
						Stated = nk_combo_begin_image_text(C, Source.Text.c_str(), Source.Text.size(), nk_image_auto(Source.Image), nk_vec2(Source.Width, Source.Height));
						break;
					case ButtonType_Text:
					default:
						Stated = nk_combo_begin_text(C, Source.Text.c_str(), Source.Text.size(), nk_vec2(Source.Width, Source.Height));
						break;
				}

				return Stated;
			}
			void Combo::BuildEnd(nk_context* C, bool Stated)
			{
				if (Stated)
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
				if (Source.TextValue)
					Source.Text = Value;

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
			void ComboItem::BuildEnd(nk_context* C, bool Stated)
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
			void ComboItem::SetLayer(Combo* New)
			{
				State.Layer = New;
			}
			bool ComboItem::IsClicked()
			{
				return State.Clicked;
			}

			Vector::Vector(Context* View) : Widget(View)
			{
				Style.Combo = (nk_style_combo*)TH_MALLOC(sizeof(nk_style_combo));
				memset(Style.Combo, 0, sizeof(nk_style_combo));

				Style.Property = (nk_style_property*)TH_MALLOC(sizeof(nk_style_property));
				memset(Style.Property, 0, sizeof(nk_style_property));

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
				Bind("count", [this]() { Source.Count = GetInteger("count", 3); });
				Bind("step", [this]() { Source.Step = GetFloat("step", 0.01f); });
				Bind("ppi", [this]() { Source.PPI = GetFloat("ppi", 0.05f); });
				Bind("text", [this]() { Source.Text = GetString("text", ""); });
				Bind("symbol", [this]() { Source.Symbol = CastSymbol(GetString("symbol", "none")); });
				Bind("width", [this]() { Source.Width = GetFloat("width", 120); });
				Bind("height", [this]() { Source.Height = GetFloat("height", 80); });
				Bind("combo-normal", [this]() { Style.Combo->normal = CastV4Item(Content, GetString("combo-normal", "45 45 45 255")); });
				Bind("combo-hover", [this]() { Style.Combo->hover = CastV4Item(Content, GetString("combo-hover", "45 45 45 255")); });
				Bind("combo-active", [this]() { Style.Combo->active = CastV4Item(Content, GetString("combo-active", "45 45 45 255")); });
				Bind("combo-border", [this]() { Style.Combo->border_color = CastV4(GetV4("combo-border", { 65, 65, 65, 255 })); });
				Bind("combo-label-normal", [this]() { Style.Combo->label_normal = CastV4(GetV4("combo-label-normal", { 175, 175, 175, 255 })); });
				Bind("combo-label-hover", [this]() { Style.Combo->label_hover = CastV4(GetV4("combo-label-hover", { 175, 175, 175, 255 })); });
				Bind("combo-label-active", [this]() { Style.Combo->label_active = CastV4(GetV4("combo-label-active", { 175, 175, 175, 255 })); });
				Bind("combo-normal-symbol", [this]() { Style.Combo->sym_normal = CastSymbol(GetString("combo-normal-symbol", "circle-outline")); });
				Bind("combo-hover-symbol", [this]() { Style.Combo->sym_hover = CastSymbol(GetString("combo-hover-symbol", "circle-outline")); });
				Bind("combo-active-symbol", [this]() { Style.Combo->sym_active = CastSymbol(GetString("combo-active-symbol", "circle-outline")); });
				Bind("combo-content-padding", [this]() { Style.Combo->content_padding = CastV2(GetV2("combo-content-padding", { 4, 0 })); });
				Bind("combo-button-padding", [this]() { Style.Combo->button_padding = CastV2(GetV2("combo-button-padding", { 0, 0 })); });
				Bind("combo-spacing", [this]() { Style.Combo->spacing = CastV2(GetV2("combo-spacing", { 4, 0 })); });
				Bind("combo-border-size", [this]() { Style.Combo->border = GetFloat("combo-border-size", 1); });
				Bind("combo-border-radius", [this]() { Style.Combo->rounding = GetFloat("combo-border-radius", 0); });
				Bind("combo-exp-normal", [this]() { Style.Combo->button.normal = CastV4Item(Content, GetString("combo-exp-normal", "45 45 45 255")); });
				Bind("combo-exp-hover", [this]() { Style.Combo->button.hover = CastV4Item(Content, GetString("combo-exp-hover", "45 45 45 255")); });
				Bind("combo-exp-active", [this]() { Style.Combo->button.active = CastV4Item(Content, GetString("combo-exp-active", "45 45 45 255")); });
				Bind("combo-exp-border", [this]() { Style.Combo->button.border_color = CastV4(GetV4("combo-exp-border", { 65, 65, 65, 255 })); });
				Bind("combo-exp-text-background", [this]() { Style.Combo->button.text_background = CastV4(GetV4("combo-exp-text-background", { 45, 45, 45, 255 })); });
				Bind("combo-exp-text-normal", [this]() { Style.Combo->button.text_normal = CastV4(GetV4("combo-exp-text-normal", { 175, 175, 175, 255 })); });
				Bind("combo-exp-text-hover", [this]() { Style.Combo->button.text_hover = CastV4(GetV4("combo-exp-text-hover", { 175, 175, 175, 255 })); });
				Bind("combo-exp-text-active", [this]() { Style.Combo->button.text_active = CastV4(GetV4("combo-exp-text-active", { 175, 175, 175, 255 })); });
				Bind("combo-exp-padding", [this]() { Style.Combo->button.padding = CastV2(GetV2("combo-exp-padding", { 2.0f, 2.0f })); });
				Bind("combo-exp-image-padding", [this]() { Style.Combo->button.image_padding = CastV2(GetV2("combo-exp-image-padding", { 0, 0 })); });
				Bind("combo-exp-touch-padding", [this]() { Style.Combo->button.touch_padding = CastV2(GetV2("combo-exp-touch-padding", { 0, 0 })); });
				Bind("combo-exp-text-align", [this]() { Style.Combo->button.text_alignment = CastTextAlign(GetString("combo-exp-text-align", "middle center")); });
				Bind("combo-exp-border-size", [this]() { Style.Combo->button.border = GetFloat("combo-exp-border-size", 0.0f); });
				Bind("combo-exp-border-radius", [this]() { Style.Combo->button.rounding = GetFloat("combo-exp-border-radius", 0.0f); });
				Bind("prop-normal", [this]() { Style.Property->normal = CastV4Item(Content, GetString("prop-normal", "38 38 38 255")); });
				Bind("prop-hover", [this]() { Style.Property->hover = CastV4Item(Content, GetString("prop-hover", "38 38 38 255")); });
				Bind("prop-active", [this]() { Style.Property->active = CastV4Item(Content, GetString("prop-active", "38 38 38 255")); });
				Bind("prop-border", [this]() { Style.Property->border_color = CastV4(GetV4("prop-border", { 65, 65, 65, 255 })); });
				Bind("prop-text-normal", [this]() { Style.Property->label_normal = CastV4(GetV4("prop-text-normal", { 175, 175, 175, 255 })); });
				Bind("prop-text-hover", [this]() { Style.Property->label_hover = CastV4(GetV4("prop-text-hover", { 175, 175, 175, 255 })); });
				Bind("prop-text-active", [this]() { Style.Property->label_active = CastV4(GetV4("prop-text-active", { 175, 175, 175, 255 })); });
				Bind("prop-inc-symbol", [this]() { Style.Property->sym_right = CastSymbol(GetString("prop-inc-symbol", "none")); });
				Bind("prop-dec-symbol", [this]() { Style.Property->sym_left = CastSymbol(GetString("prop-dec-symbol", "none")); });
				Bind("prop-padding", [this]() { Style.Property->padding = CastV2(GetV2("prop-padding", { 0.0f, 4.0f })); });
				Bind("prop-border-size", [this]() { Style.Property->border = GetFloat("prop-border-size", 0.0f); });
				Bind("prop-border-radius", [this]() { Style.Property->rounding = GetFloat("prop-border-radius", 0.0f); });
				Bind("prop-inc-normal", [this]() { Style.Property->inc_button.normal = CastV4Item(Content, GetString("prop-inc-normal", "38 38 38 255")); });
				Bind("prop-inc-hover", [this]() { Style.Property->inc_button.hover = CastV4Item(Content, GetString("prop-inc-hover", "38 38 38 255")); });
				Bind("prop-inc-active", [this]() { Style.Property->inc_button.active = CastV4Item(Content, GetString("prop-inc-active", "38 38 38 255")); });
				Bind("prop-inc-border", [this]() { Style.Property->inc_button.border_color = CastV4(GetV4("prop-inc-border", { 0, 0, 0, 0 })); });
				Bind("prop-inc-text-background", [this]() { Style.Property->inc_button.text_background = CastV4(GetV4("prop-inc-text-background", { 38, 38, 38, 255 })); });
				Bind("prop-inc-text-normal", [this]() { Style.Property->inc_button.text_normal = CastV4(GetV4("prop-inc-text-normal", { 175, 175, 175, 255 })); });
				Bind("prop-inc-text-hover", [this]() { Style.Property->inc_button.text_hover = CastV4(GetV4("prop-inc-text-hover", { 175, 175, 175, 255 })); });
				Bind("prop-inc-text-active", [this]() { Style.Property->inc_button.text_active = CastV4(GetV4("prop-inc-text-active", { 175, 175, 175, 255 })); });
				Bind("prop-inc-padding", [this]() { Style.Property->inc_button.padding = CastV2(GetV2("prop-inc-padding", { 0.0f, 0.0f })); });
				Bind("prop-inc-image-padding", [this]() { Style.Property->inc_button.image_padding = CastV2(GetV2("prop-inc-image-padding", { 0, 0 })); });
				Bind("prop-inc-touch-padding", [this]() { Style.Property->inc_button.touch_padding = CastV2(GetV2("prop-inc-touch-padding", { 0, 0 })); });
				Bind("prop-inc-text-align", [this]() { Style.Property->inc_button.text_alignment = CastTextAlign(GetString("prop-inc-text-align", "middle center")); });
				Bind("prop-inc-border-size", [this]() { Style.Property->inc_button.border = GetFloat("prop-inc-border-size", 0.0f); });
				Bind("prop-inc-border-radius", [this]() { Style.Property->inc_button.rounding = GetFloat("prop-inc-border-radius", 0.0f); });
				Bind("prop-dec-normal", [this]() { Style.Property->dec_button.normal = CastV4Item(Content, GetString("prop-dec-normal", "38 38 38 255")); });
				Bind("prop-dec-hover", [this]() { Style.Property->dec_button.hover = CastV4Item(Content, GetString("prop-dec-hover", "38 38 38 255")); });
				Bind("prop-dec-active", [this]() { Style.Property->dec_button.active = CastV4Item(Content, GetString("prop-dec-active", "38 38 38 255")); });
				Bind("prop-dec-border", [this]() { Style.Property->dec_button.border_color = CastV4(GetV4("prop-dec-border", { 0, 0, 0, 0 })); });
				Bind("prop-dec-text-background", [this]() { Style.Property->dec_button.text_background = CastV4(GetV4("prop-dec-text-background", { 38, 38, 38, 255 })); });
				Bind("prop-dec-text-normal", [this]() { Style.Property->dec_button.text_normal = CastV4(GetV4("prop-dec-text-normal", { 175, 175, 175, 255 })); });
				Bind("prop-dec-text-hover", [this]() { Style.Property->dec_button.text_hover = CastV4(GetV4("prop-dec-text-hover", { 175, 175, 175, 255 })); });
				Bind("prop-dec-text-active", [this]() { Style.Property->dec_button.text_active = CastV4(GetV4("prop-dec-text-active", { 175, 175, 175, 255 })); });
				Bind("prop-dec-padding", [this]() { Style.Property->dec_button.padding = CastV2(GetV2("prop-dec-padding", { 0.0f, 0.0f })); });
				Bind("prop-dec-image-padding", [this]() { Style.Property->dec_button.image_padding = CastV2(GetV2("prop-dec-image-padding", { 0, 0 })); });
				Bind("prop-dec-touch-padding", [this]() { Style.Property->dec_button.touch_padding = CastV2(GetV2("prop-dec-touch-padding", { 0, 0 })); });
				Bind("prop-dec-text-align", [this]() { Style.Property->dec_button.text_alignment = CastTextAlign(GetString("prop-dec-text-align", "middle center")); });
				Bind("prop-dec-border-size", [this]() { Style.Property->dec_button.border = GetFloat("prop-dec-border-size", 0.0f); });
				Bind("prop-dec-border-radius", [this]() { Style.Property->dec_button.rounding = GetFloat("prop-dec-border-radius", 0.0f); });
				Bind("prop-edit-normal", [this]() { Style.Property->edit.normal = CastV4Item(Content, GetString("prop-edit-normal", "38 38 38 255")); });
				Bind("prop-edit-hover", [this]() { Style.Property->edit.hover = CastV4Item(Content, GetString("prop-edit-hover", "38 38 38 255")); });
				Bind("prop-edit-active", [this]() { Style.Property->edit.active = CastV4Item(Content, GetString("prop-edit-active", "38 38 38 255")); });
				Bind("prop-edit-border", [this]() { Style.Property->edit.border_color = CastV4(GetV4("prop-edit-active", { 0, 0, 0, 0 })); });
				Bind("prop-edit-cursor-normal", [this]() { Style.Property->edit.cursor_normal = CastV4(GetV4("prop-edit-cursor-normal", { 175, 175, 175, 255 })); });
				Bind("prop-edit-cursor-hover", [this]() { Style.Property->edit.cursor_hover = CastV4(GetV4("prop-edit-cursor-hover", { 175, 175, 175, 255 })); });
				Bind("prop-edit-cursor-text-normal", [this]() { Style.Property->edit.cursor_text_normal = CastV4(GetV4("prop-edit-cursor-text-normal", { 38, 38, 38, 255 })); });
				Bind("prop-edit-cursor-text-hover", [this]() { Style.Property->edit.cursor_text_hover = CastV4(GetV4("prop-edit-cursor-text-hover", { 38, 38, 38, 255 })); });
				Bind("prop-edit-text-normal", [this]() { Style.Property->edit.text_normal = CastV4(GetV4("prop-edit-text-normal", { 175, 175, 175, 255 })); });
				Bind("prop-edit-text-hover", [this]() { Style.Property->edit.text_hover = CastV4(GetV4("prop-edit-text-hover", { 175, 175, 175, 255 })); });
				Bind("prop-edit-text-active", [this]() { Style.Property->edit.text_active = CastV4(GetV4("prop-edit-text-active", { 175, 175, 175, 255 })); });
				Bind("prop-edit-select-normal", [this]() { Style.Property->edit.selected_normal = CastV4(GetV4("prop-edit-select-normal", { 175, 175, 175, 255 })); });
				Bind("prop-edit-select-hover", [this]() { Style.Property->edit.selected_hover = CastV4(GetV4("prop-edit-select-hover", { 175, 175, 175, 255 })); });
				Bind("prop-edit-select-text-normal", [this]() { Style.Property->edit.selected_text_normal = CastV4(GetV4("prop-edit-select-text-normal", { 38, 38, 38, 255 })); });
				Bind("prop-edit-select-text-hover", [this]() { Style.Property->edit.selected_text_hover = CastV4(GetV4("prop-edit-select-text-hover", { 38, 38, 38, 255 })); });
				Bind("prop-edit-cursor-size", [this]() { Style.Property->edit.cursor_size = GetFloat("prop-edit-cursor-size", 8); });
				Bind("prop-edit-border-size", [this]() { Style.Property->edit.border = GetFloat("prop-edit-border-size", 0); });
				Bind("prop-edit-border-radius", [this]() { Style.Property->edit.rounding = GetFloat("prop-edit-border-radius", 0); });
			}
			Vector::~Vector()
			{
				TH_FREE(Style.Combo);
				TH_FREE(Style.Property);
			}
			bool Vector::BuildBegin(nk_context* C)
			{
				bool Stated = false;
				switch (Source.Type)
				{
					case ButtonType_Color:
						Stated = nk_combo_begin_color(C, CastV4(Source.Color), nk_vec2(Source.Width, Source.Height));
						break;
					case ButtonType_Symbol:
						Stated = nk_combo_begin_symbol(C, (nk_symbol_type)Source.Symbol, nk_vec2(Source.Width, Source.Height));
						break;
					case ButtonType_Symbol_Text:
						Stated = nk_combo_begin_symbol_text(C, Source.Text.c_str(), Source.Text.size(), (nk_symbol_type)Source.Symbol, nk_vec2(Source.Width, Source.Height));
						break;
					case ButtonType_Image:
						Stated = nk_combo_begin_image(C, nk_image_auto(Source.Image), nk_vec2(Source.Width, Source.Height));
						break;
					case ButtonType_Image_Text:
						Stated = nk_combo_begin_image_text(C, Source.Text.c_str(), Source.Text.size(), nk_image_auto(Source.Image), nk_vec2(Source.Width, Source.Height));
						break;
					case ButtonType_Text:
					default:
						Stated = nk_combo_begin_text(C, Source.Text.c_str(), Source.Text.size(), nk_vec2(Source.Width, Source.Height));
						break;
				}

				float Max = std::numeric_limits<float>::max();
				Compute::Vector4 Last = State.Value;

				if (Stated && Source.Count >= 1)
				{
					nk_layout_row_dynamic(C, 24, 1);
					if (Source.Count >= 1)
						nk_property_float(C, "#X", -Max, &State.Value.X, Max, Source.Step, Source.PPI);

					if (Source.Count >= 2)
						nk_property_float(C, "#Y", -Max, &State.Value.Y, Max, Source.Step, Source.PPI);

					if (Source.Count >= 3)
						nk_property_float(C, "#Z", -Max, &State.Value.Z, Max, Source.Step, Source.PPI);

					if (Source.Count >= 4)
						nk_property_float(C, "#W", -Max, &State.Value.W, Max, Source.Step, Source.PPI);

					if (!Source.Bind.empty() && Last != State.Value)
					{
						if (Source.Count <= 1)
							SetGlobal(Source.Bind, Rest::Form("%f", State.Value.X).R());
						else if (Source.Count <= 2)
							SetGlobal(Source.Bind, Rest::Form("%f %f", State.Value.X, State.Value.Y).R());
						else if (Source.Count <= 3)
							SetGlobal(Source.Bind, Rest::Form("%f %f %f", State.Value.X, State.Value.Y, State.Value.Z).R());
						else if (Source.Count <= 4)
							SetGlobal(Source.Bind, Rest::Form("%f %f %f %f", State.Value.X, State.Value.Y, State.Value.Z, State.Value.W).R());
					}
				}

				return Stated;
			}
			void Vector::BuildEnd(nk_context* C, bool Stated)
			{
				if (Stated)
					nk_combo_end(C);
			}
			void Vector::BuildStyle(nk_context* C, nk_style* S)
			{
				memcpy(&C->style.combo, S ? &S->combo : Style.Combo, sizeof(nk_style_combo));
				memcpy(&C->style.property, S ? &S->property : Style.Property, sizeof(nk_style_property));
				BuildFont(C, S);
			}
			float Vector::GetWidth()
			{
				return Source.Width;
			}
			float Vector::GetHeight()
			{
				return Source.Height;
			}
			std::string Vector::GetType()
			{
				return "vector";
			}
			void Vector::SetValue(const Compute::Vector4& Value)
			{
				State.Value = Value;
			}
			Compute::Vector4 Vector::GetValue()
			{
				return State.Value;
			}

			Group::Group(Context* View) : Stateful(View)
			{
				Style = (nk_style_window*)TH_MALLOC(sizeof(nk_style_window));
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
				TH_FREE(Style);
			}
			bool Group::BuildBegin(nk_context* C)
			{
				nk_flags Flags = 0;
				if (Source.Scrolled)
					Flags = nk_group_scrolled_offset_begin(C, &Source.OffsetX, &Source.OffsetY, Source.Text.c_str(), Source.Flags);
				else
					Flags = nk_group_begin_titled(C, Hash.Text.c_str(), Source.Text.c_str(), Source.Flags);

				return (Flags != NK_WINDOW_CLOSED && Flags != NK_WINDOW_MINIMIZED);
			}
			void Group::BuildEnd(nk_context* C, bool Stated)
			{
				if (!Stated)
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

			Tree::Tree(Context* View) : Stateful(View)
			{
				Style.Tab = (nk_style_tab*)TH_MALLOC(sizeof(nk_style_tab));
				memset(Style.Tab, 0, sizeof(nk_style_tab));

				Style.Selectable = (nk_style_selectable*)TH_MALLOC(sizeof(nk_style_selectable));
				memset(Style.Selectable, 0, sizeof(nk_style_selectable));

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
						State.Default.Value = (int)(*Result == "1" || *Result == "true");
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
				Bind("auto-toggle", [this]() { Source.AutoToggle = GetBoolean("auto-toggle", true); });
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
				TH_FREE(Style.Tab);
				TH_FREE(Style.Selectable);
			}
			bool Tree::BuildBegin(nk_context* C)
			{
				Internal* It = GetState();
				bool Stated = false;

				if (Source.Selectable)
				{
					nk_collapse_states* Value = (Source.AutoToggle ? (nk_collapse_states*)nk_find_value(C->current, nk_murmur_hash(Hash.Text.c_str(), Hash.Text.size(), (nk_hash)Hash.Id)) : nullptr);
					nk_collapse_states Old = (!Value ? NK_MINIMIZED : *Value);

					It->Selected = It->Value;
					if (Source.Image != nullptr)
						Stated = nk_tree_element_image_push_hashed(C, (nk_tree_type)Source.Type, nk_image_auto(Source.Image), Source.Text.c_str(), Source.Minimized ? NK_MINIMIZED : NK_MAXIMIZED, &It->Value, Hash.Text.c_str(), Hash.Text.size(), Hash.Id);
					else
						Stated = nk_tree_element_push_hashed(C, (nk_tree_type)Source.Type, Source.Text.c_str(), Source.Minimized ? NK_MINIMIZED : NK_MAXIMIZED, &It->Value, Hash.Text.c_str(), Hash.Text.size(), Hash.Id);

					if (It->Selected != It->Value)
					{
						if (!Source.Bind.empty())
							SetGlobal(Source.Bind, It->Value ? "1" : "0");

						if (Source.AutoToggle && Value != nullptr && *Value == Old)
							*Value = (*Value == NK_MINIMIZED ? NK_MAXIMIZED : NK_MINIMIZED);
					}
				}
				else
				{
					if (Source.Image != nullptr)
						Stated = nk_tree_image_push_hashed(C, (nk_tree_type)Source.Type, nk_image_auto(Source.Image), Source.Text.c_str(), Source.Minimized ? NK_MINIMIZED : NK_MAXIMIZED, Hash.Text.c_str(), Hash.Text.size(), Hash.Id);
					else
						Stated = nk_tree_push_hashed(C, (nk_tree_type)Source.Type, Source.Text.c_str(), Source.Minimized ? NK_MINIMIZED : NK_MAXIMIZED, Hash.Text.c_str(), Hash.Text.size(), Hash.Id);
				}

				It->Hovered = GetContext()->IsCurrentWidgetHovered();
				return Stated;
			}
			void Tree::BuildEnd(nk_context* C, bool Stated)
			{
				if (!Stated)
					return;

				if (Source.Selectable)
					nk_tree_element_pop(C);
				else
					nk_tree_pop(C);
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
				GetState()->Value = (int)Value;
			}
			bool Tree::GetValue()
			{
				return GetState()->Value > 0;
			}
			bool Tree::IsClicked()
			{
				Internal* It = GetState();
				return It->Selected != It->Value;
			}
			bool Tree::IsHovered()
			{
				return GetState()->Hovered;
			}
			Tree::Internal* Tree::GetState()
			{
				Internal* It = &State.Default;
				if (Hash.Count > 0)
					It = &State.Data[Hash.Text];

				return It;
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
				bool Stated = nk_popup_begin(C, Source.Dynamic ? NK_POPUP_DYNAMIC : NK_POPUP_STATIC, Source.Text.c_str(), Source.Flags, nk_rect(Source.X, Source.Y, Source.Width, Source.Height));
				if (Stated)
					Base->SetOverflow(true);
				
				return Stated;
			}
			void Popup::BuildEnd(nk_context* C, bool Stated)
			{
				if (Stated)
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
				Bind("relative", [this]() { Source.Relative = GetBoolean("relative", false); });
			}
			Contextual::~Contextual()
			{
			}
			bool Contextual::BuildBegin(nk_context* C)
			{
				struct nk_vec2 Size = nk_vec2(Source.Width, Source.Height);
				struct nk_rect Trigger = nk_rect(Source.TriggerX, Source.TriggerY, Source.TriggerWidth, Source.TriggerHeight);

				if (Source.Relative)
				{
					float Width = GetAreaWidth();
					Trigger.x *= Width;
					Trigger.w *= Width;
					Size.x *= Width;

					float Height = GetAreaHeight();
					Trigger.y *= Height;
					Trigger.h *= Height;
					Size.y *= Height;
				}

				bool Stated = nk_contextual_begin(C, Source.Flags, Size, Trigger);
				if (Stated)
					Base->SetOverflow(true);
				
				return Stated;
			}
			void Contextual::BuildEnd(nk_context* C, bool Stated)
			{
				if (Stated)
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

			ContextualItem::ContextualItem(Context* View) : Widget(View)
			{
				Style = (nk_style_button*)TH_MALLOC(sizeof(nk_style_button));
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
			ContextualItem::~ContextualItem()
			{
				TH_FREE(Style);
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
			void ContextualItem::BuildEnd(nk_context* C, bool Stated)
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
			void ContextualItem::SetLayer(Contextual* New)
			{
				State.Layer = New;
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
			void Tooltip::BuildEnd(nk_context* C, bool Stated)
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
			void Header::BuildEnd(nk_context* C, bool Stated)
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
				Style = (nk_style_button*)TH_MALLOC(sizeof(nk_style_button));
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
				Bind("align", [this]() { Source.Align = CastTextAlign(GetString("align", "middle left")); });
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
				TH_FREE(Style);
			}
			bool HeaderTab::BuildBegin(nk_context* C)
			{
				if (!State.Layer)
				{
					State.Layer = (Header*)GetParent("header");
					if (!State.Layer)
						return false;
				}

				bool Stated = false;
				switch (Source.Type)
				{
					case ButtonType_Symbol:
					case ButtonType_Symbol_Text:
						Stated = nk_menu_begin_symbol_text(C, Source.Text.c_str(), Source.Text.size(), Source.Align, (nk_symbol_type)Source.Symbol, nk_vec2(Source.Width, Source.Height));
						break;
					case ButtonType_Image:
					case ButtonType_Image_Text:
						Stated = nk_menu_begin_image_text(C, Source.Text.c_str(), Source.Text.size(), Source.Align, nk_image_auto(Source.Image), nk_vec2(Source.Width, Source.Height));
						break;
					case ButtonType_Text:
					case ButtonType_Color:
					default:
						Stated = nk_menu_begin_text(C, Source.Text.c_str(), Source.Text.size(), Source.Align, nk_vec2(Source.Width, Source.Height));
						break;
				}

				if (Stated)
					Base->SetOverflow(true);

				return Stated;
			}
			void HeaderTab::BuildEnd(nk_context* C, bool Stated)
			{
				if (Stated)
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
			void HeaderTab::SetLayer(Header* New)
			{
				State.Layer = New;
			}
			std::string HeaderTab::GetType()
			{
				return "header-tab";
			}

			HeaderItem::HeaderItem(Context* View) : Widget(View)
			{
				Style = (nk_style_button*)TH_MALLOC(sizeof(nk_style_button));
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
				TH_FREE(Style);
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
			void HeaderItem::BuildEnd(nk_context* C, bool Stated)
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
			void HeaderItem::SetLayer(HeaderTab* New)
			{
				State.Layer = New;
			}
			bool HeaderItem::IsClicked()
			{
				return State.Clicked;
			}
		}
	}
}