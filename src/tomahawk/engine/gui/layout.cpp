#include "layout.h"
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
			void PreLayout::BuildEnd(nk_context* C, bool Stated)
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
			void PrePush::BuildEnd(nk_context* C, bool Stated)
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
			void Layout::BuildEnd(nk_context* C, bool Stated)
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
			void Push::BuildEnd(nk_context* C, bool Stated)
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
		}
	}
}