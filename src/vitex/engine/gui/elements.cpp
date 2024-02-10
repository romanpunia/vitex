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

			static Rml::ElementInstancerGeneric<ExpandElement>* IExpandInstancer = nullptr;
			void Subsystem::CreateElements() noexcept
			{
				if (!IExpandInstancer)
				{
					IExpandInstancer = Core::Memory::New<Rml::ElementInstancerGeneric<ExpandElement>>();
					Rml::Factory::RegisterElementInstancer("expand", IExpandInstancer);
				}
			}
			void Subsystem::ReleaseElements() noexcept
			{
				Core::Memory::Delete(IExpandInstancer);
				IExpandInstancer = nullptr;
			}
		}
	}
}
#endif