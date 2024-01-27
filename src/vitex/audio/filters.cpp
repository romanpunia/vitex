#include "filters.h"
#include "../core/engine.h"
#define ReturnErrorIf do { auto __error = AudioException(); if (__error.has_error()) return __error; else return Core::Expectation::Met; } while (0)

namespace Vitex
{
	namespace Audio
	{
		namespace Filters
		{
			Lowpass::Lowpass()
			{
				Initialize([this]()
				{
					AudioContext::SetFilter1I(Filter, FilterEx::Filter_Type, (int)FilterEx::Filter_Lowpass);
					return true;
				});
			}
			Lowpass::~Lowpass()
			{
			}
			ExpectsAudio<void> Lowpass::Synchronize()
			{
				AudioContext::SetFilter1F(Filter, FilterEx::Lowpass_Gain, Gain);
				AudioContext::SetFilter1F(Filter, FilterEx::Lowpass_Gain_HF, GainHF);
				ReturnErrorIf;
			}
			void Lowpass::Deserialize(Core::Schema* Node)
			{
				VI_ASSERT(Node != nullptr, "schema should be set");
				Engine::Series::Unpack(Node->Find("gain"), &Gain);
				Engine::Series::Unpack(Node->Find("gain-hf"), &GainHF);
			}
			void Lowpass::Serialize(Core::Schema* Node) const
			{
				VI_ASSERT(Node != nullptr, "schema should be set");
				Engine::Series::Pack(Node->Set("gain"), Gain);
				Engine::Series::Pack(Node->Set("gain-hf"), GainHF);
			}
			AudioFilter* Lowpass::Copy() const
			{
				Lowpass* Target = new Lowpass();
				Target->Gain = Gain;
				Target->GainHF = GainHF;

				return Target;
			}

			Highpass::Highpass()
			{
				Initialize([this]()
				{
					AudioContext::SetFilter1I(Filter, FilterEx::Filter_Type, (int)FilterEx::Filter_Highpass);
					return true;
				});
			}
			Highpass::~Highpass()
			{
			}
			ExpectsAudio<void> Highpass::Synchronize()
			{
				AudioContext::SetFilter1F(Filter, FilterEx::Highpass_Gain, Gain);
				AudioContext::SetFilter1F(Filter, FilterEx::Highpass_Gain_LF, GainLF);
				ReturnErrorIf;
			}
			void Highpass::Deserialize(Core::Schema* Node)
			{
				VI_ASSERT(Node != nullptr, "schema should be set");
				Engine::Series::Unpack(Node->Find("gain"), &Gain);
				Engine::Series::Unpack(Node->Find("gain-lf"), &GainLF);
			}
			void Highpass::Serialize(Core::Schema* Node) const
			{
				VI_ASSERT(Node != nullptr, "schema should be set");
				Engine::Series::Pack(Node->Set("gain"), Gain);
				Engine::Series::Pack(Node->Set("gain-lf"), GainLF);
			}
			AudioFilter* Highpass::Copy() const
			{
				Highpass* Target = new Highpass();
				Target->Gain = Gain;
				Target->GainLF = GainLF;

				return Target;
			}

			Bandpass::Bandpass()
			{
				Initialize([this]()
				{
					AudioContext::SetFilter1I(Filter, FilterEx::Filter_Type, (int)FilterEx::Filter_Bandpass);
					return true;
				});
			}
			Bandpass::~Bandpass()
			{
			}
			ExpectsAudio<void> Bandpass::Synchronize()
			{
				AudioContext::SetFilter1F(Filter, FilterEx::Bandpass_Gain, Gain);
				AudioContext::SetFilter1F(Filter, FilterEx::Bandpass_Gain_LF, GainLF);
				AudioContext::SetFilter1F(Filter, FilterEx::Bandpass_Gain_HF, GainHF);
				ReturnErrorIf;
			}
			void Bandpass::Deserialize(Core::Schema* Node)
			{
				VI_ASSERT(Node != nullptr, "schema should be set");
				Engine::Series::Unpack(Node->Find("gain"), &Gain);
				Engine::Series::Unpack(Node->Find("gain-lf"), &GainLF);
				Engine::Series::Unpack(Node->Find("gain-hf"), &GainHF);
			}
			void Bandpass::Serialize(Core::Schema* Node) const
			{
				VI_ASSERT(Node != nullptr, "schema should be set");
				Engine::Series::Pack(Node->Set("gain"), Gain);
				Engine::Series::Pack(Node->Set("gain-lf"), GainLF);
				Engine::Series::Pack(Node->Set("gain-hf"), GainHF);
			}
			AudioFilter* Bandpass::Copy() const
			{
				Bandpass* Target = new Bandpass();
				Target->Gain = Gain;
				Target->GainLF = GainLF;
				Target->GainHF = GainHF;

				return Target;
			}
		}
	}
}
