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
			void DrawLine::BuildEnd(nk_context* C, bool Stated)
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
			void DrawCurve::BuildEnd(nk_context* C, bool Stated)
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
			void DrawRect::BuildEnd(nk_context* C, bool Stated)
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
			void DrawCircle::BuildEnd(nk_context* C, bool Stated)
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
			void DrawArc::BuildEnd(nk_context* C, bool Stated)
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
			void DrawTriangle::BuildEnd(nk_context* C, bool Stated)
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
			void DrawPolyline::BuildEnd(nk_context* C, bool Stated)
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
			void DrawPolygon::BuildEnd(nk_context* C, bool Stated)
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

			DrawLabel::DrawLabel(Context* View) : Widget(View)
			{
				Bind("background", [this]() { Source.Background = GetV4("background", { 0, 0, 0, 0 }); });
				Bind("color", [this]() { Source.Color = GetV4("color", { 175, 175, 175, 255 }); });
				Bind("width", [this]() { Source.Width = GetFloat("width", 40.0f); });
				Bind("height", [this]() { Source.Height = GetFloat("height", 15.0f); });
				Bind("x", [this]() { Source.X = GetFloat("x", 0.0f); });
				Bind("y", [this]() { Source.Y = GetFloat("y", 0.0f); });
				Bind("text", [this]() { Source.Text = GetString("text", ""); });
				Bind("relative", [this]() { Source.Relative = GetBoolean("relative", false); });
			}
			DrawLabel::~DrawLabel()
			{
			}
			bool DrawLabel::BuildBegin(nk_context* C)
			{
				if (Source.Text.empty())
					return false;

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

				nk_draw_text(&C->current->buffer, Bounds, Source.Text.c_str(), Source.Text.size(), GetFontHandle() ? &GetFontHandle()->handle : C->style.font, CastV4(Source.Background), CastV4(Source.Color));
				return true;
			}
			void DrawLabel::BuildEnd(nk_context* C, bool Stated)
			{
			}
			void DrawLabel::BuildStyle(nk_context* C, nk_style* S)
			{
			}
			float DrawLabel::GetWidth()
			{
				return 0;
			}
			float DrawLabel::GetHeight()
			{
				return 0;
			}
			std::string DrawLabel::GetType()
			{
				return "label";
			}
		}
	}
}