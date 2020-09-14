#ifndef THAWK_ENGINE_GUI_SHAPES_H
#define THAWK_ENGINE_GUI_SHAPES_H

#include "context.h"

namespace Tomahawk
{
	namespace Engine
	{
		namespace GUI
		{
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
		}
	}
}
#endif