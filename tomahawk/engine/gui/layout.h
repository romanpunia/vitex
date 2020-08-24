#ifndef THAWK_ENGINE_GUI_LAYOUT_H
#define THAWK_ENGINE_GUI_LAYOUT_H

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
				void SetLayer(PreLayout* New);
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
				void SetLayer(Layout* New);
			};
		}
	}
}
#endif