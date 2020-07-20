#ifndef THAWK_AUDIO_FILTERS_H
#define THAWK_AUDIO_FILTERS_H

#include "../audio.h"

namespace Tomahawk
{
	namespace Audio
	{
		namespace Filters
		{
			class THAWK_OUT FilterContext
			{
			public:
				static void Initialize();
			};

			class THAWK_OUT LowpassFilter : public AudioFilter
			{
			public:
				float GainHF = 1.0f;
				float Gain = 1.0f;

			public:
				LowpassFilter();
				virtual ~LowpassFilter() override;
				void Synchronize() override;
				void Deserialize(Rest::Document* Node) override;
				void Serialize(Rest::Document* Node) override;
				AudioFilter* Copy() override;

			public:
				THAWK_COMPONENT(LowpassFilter);
			};

			class THAWK_OUT HighpassFilter : public AudioFilter
			{
			public:
				float GainLF = 1.0f;
				float Gain = 1.0f;

			public:
				HighpassFilter();
				virtual ~HighpassFilter() override;
				void Synchronize() override;
				void Deserialize(Rest::Document* Node) override;
				void Serialize(Rest::Document* Node) override;
				AudioFilter* Copy() override;

			public:
				THAWK_COMPONENT(HighpassFilter);
			};

			class THAWK_OUT BandpassFilter : public AudioFilter
			{
			public:
				float GainHF = 1.0f;
				float GainLF = 1.0f;
				float Gain = 1.0f;

			public:
				BandpassFilter();
				virtual ~BandpassFilter() override;
				void Synchronize() override;
				void Deserialize(Rest::Document* Node) override;
				void Serialize(Rest::Document* Node) override;
				AudioFilter* Copy() override;

			public:
				THAWK_COMPONENT(BandpassFilter);
			};
		}
	}
}
#endif