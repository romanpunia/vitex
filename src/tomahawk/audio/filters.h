#ifndef TH_AUDIO_FILTERS_H
#define TH_AUDIO_FILTERS_H
#include "../core/audio.h"

namespace Tomahawk
{
	namespace Audio
	{
		namespace Filters
		{
			class TH_OUT FilterContext
			{
			public:
				static void Create();
			};

			class TH_OUT Lowpass final : public AudioFilter
			{
			public:
				float GainHF = 1.0f;
				float Gain = 1.0f;

			public:
				Lowpass();
				virtual ~Lowpass() override;
				void Synchronize() override;
				void Deserialize(Core::Schema* Node) override;
				void Serialize(Core::Schema* Node) override;
				Core::Unique<AudioFilter> Copy() override;

			public:
				TH_COMPONENT("lowpass-filter");
			};

			class TH_OUT Highpass final : public AudioFilter
			{
			public:
				float GainLF = 1.0f;
				float Gain = 1.0f;

			public:
				Highpass();
				virtual ~Highpass() override;
				void Synchronize() override;
				void Deserialize(Core::Schema* Node) override;
				void Serialize(Core::Schema* Node) override;
				Core::Unique<AudioFilter> Copy() override;

			public:
				TH_COMPONENT("highpass-filter");
			};

			class TH_OUT Bandpass final : public AudioFilter
			{
			public:
				float GainHF = 1.0f;
				float GainLF = 1.0f;
				float Gain = 1.0f;

			public:
				Bandpass();
				virtual ~Bandpass() override;
				void Synchronize() override;
				void Deserialize(Core::Schema* Node) override;
				void Serialize(Core::Schema* Node) override;
				Core::Unique<AudioFilter> Copy() override;

			public:
				TH_COMPONENT("bandpass-filter");
			};
		}
	}
}
#endif