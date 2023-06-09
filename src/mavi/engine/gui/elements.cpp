#include "../gui.h"
#ifdef VI_RMLUI
#include <RmlUi/Core.h>
#include <Source/Core/ElementStyle.h>

namespace Mavi
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
				~ExpandElement() override
				{
				}
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

			static Rml::ElementInstancerGeneric<ExpandElement>* IExpandInstancer = nullptr;
			void Subsystem::CreateElements() noexcept
			{
				if (!IExpandInstancer)
				{
					IExpandInstancer = VI_NEW(Rml::ElementInstancerGeneric<ExpandElement>);
					Rml::Factory::RegisterElementInstancer("expand", IExpandInstancer);
				}
			}
			void Subsystem::ReleaseElements() noexcept
			{
				VI_DELETE(ElementInstancerGeneric, IExpandInstancer);
				IExpandInstancer = nullptr;
			}
		}
	}
}
#endif