#include "../gui.h"
#ifdef VI_RMLUI
#include <RmlUi/Core.h>
#include <Source/Core/ElementStyle.h>

namespace Vitex
{
	namespace Engine
	{
		namespace GUI
		{
			class ExpandElement final : public Rml::Element
			{
			private:
				bool Hidden;

			public:
				ExpandElement(const Rml::String& Tag) : Element(Tag), Hidden(false)
				{
					SetPseudoClass("hidden", false);
				}
				virtual ~ExpandElement() = default;
				void ProcessDefaultAction(Rml::Event& Event) override
				{
					Rml::Element::ProcessDefaultAction(Event);
					if (Event == Rml::EventId::Click && Event.GetCurrentElement() == this)
					{
						Hidden = !Hidden;
						SetPseudoClass("hidden", Hidden);
					}
				}
			};

			class FontFaceElement final : public Rml::Element
			{
			private:
				bool Initialized;

			public:
				FontFaceElement(const Rml::String& Tag) : Element(Tag), Initialized(false)
				{
				}
				virtual ~FontFaceElement() = default;
				void OnUpdate() override
				{
					if (Initialized)
						return;

					auto* Href = GetAttribute("href");
					if (!Href || Href->GetType() != Rml::Variant::Type::STRING)
						return;

					auto* Fallback = GetAttribute("fallback"); auto* Weight = GetAttribute("weight");
					Context::LoadFontFace(Href->Get<Rml::String>(), Fallback ? Fallback->Get<bool>(false) : false, GetWeight(Weight));
					Initialized = true;
				}

			private:
				static FontWeight GetWeight(Rml::Variant* Value)
				{
					if (!Value)
						return FontWeight::Auto;

					if (Value->GetType() != Rml::Variant::STRING)
					{
						uint16_t Weight = Value->Get<uint16_t>();
						return (FontWeight)std::min<uint16_t>(Weight, 1000);
					}

					Rml::String Type = Value->Get<Rml::String>();
					if (Type == "normal")
						return FontWeight::Normal;

					if (Type == "bold")
						return FontWeight::Bold;

					return FontWeight::Auto;
				}
			};

			static Rml::ElementInstancerGeneric<ExpandElement>* IExpandInstancer = nullptr;
			static Rml::ElementInstancerGeneric<FontFaceElement>* IFontFaceInstancer = nullptr;
			void Subsystem::CreateElements() noexcept
			{
				if (!IExpandInstancer)
				{
					IExpandInstancer = VI_NEW(Rml::ElementInstancerGeneric<ExpandElement>);
					Rml::Factory::RegisterElementInstancer("expand", IExpandInstancer);
				}

				if (!IFontFaceInstancer)
				{
					IFontFaceInstancer = VI_NEW(Rml::ElementInstancerGeneric<FontFaceElement>);
					Rml::Factory::RegisterElementInstancer("font-face", IFontFaceInstancer);
				}
			}
			void Subsystem::ReleaseElements() noexcept
			{
				VI_DELETE(ElementInstancerGeneric, IExpandInstancer);
				IExpandInstancer = nullptr;
				VI_DELETE(ElementInstancerGeneric, IFontFaceInstancer);
				IFontFaceInstancer = nullptr;
			}
		}
	}
}
#endif