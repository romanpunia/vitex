#include "effects.h"
#include "filters.h"
#include "../engine.h"
#define ReturnErrorIf do { auto __error = AudioException(); if (__error.has_error()) return __error; else return Core::Expectation::Met; } while (0)

namespace Vitex
{
	namespace Audio
	{
		namespace Effects
		{
			AudioFilter* GetFilterDeserialized(Core::Schema* Node)
			{
				Core::Schema* Filter = Node->Find("filter");
				if (!Filter)
					return nullptr;

				uint64_t Id;
				if (!Engine::Series::Unpack(Filter->Find("id"), &Id))
					return nullptr;

				AudioFilter* Target = Core::Composer::Create<AudioFilter>(Id);
				if (!Target)
					return nullptr;

				Core::Schema* Meta = Filter->Find("metadata");
				if (!Meta)
					Meta = Filter->Set("metadata");

				Target->Deserialize(Meta);
				return Target;
			}

			Reverb::Reverb()
			{
				Initialize([this]()
				{
					EAX = (AudioContext::GetEnumValue("AL_EFFECT_EAXREVERB") != 0);
					if (EAX)
						AudioContext::SetEffect1I(Effect, EffectEx::Effect_Type, (int)EffectEx::Effect_EAXReverb);
					else
						AudioContext::SetEffect1I(Effect, EffectEx::Effect_Type, (int)EffectEx::Effect_Reverb);
					return true;
				});
			}
			Reverb::~Reverb()
			{
			}
			ExpectsAudio<void> Reverb::Synchronize()
			{
				if (EAX)
				{
					float ReflectionsPan3[3];
					ReflectionsPan.Get3(ReflectionsPan3);

					float LateReverbPan3[3];
					LateReverbPan.Get3(LateReverbPan3);

					AudioContext::SetEffectVF(Effect, EffectEx::EAXReverb_Reflections_Pan, ReflectionsPan3);
					AudioContext::SetEffectVF(Effect, EffectEx::EAXReverb_Late_Reverb_Pan, LateReverbPan3);
					AudioContext::SetEffect1F(Effect, EffectEx::EAXReverb_Density, Density);
					AudioContext::SetEffect1F(Effect, EffectEx::EAXReverb_Diffusion, Diffusion);
					AudioContext::SetEffect1F(Effect, EffectEx::EAXReverb_Gain, Gain);
					AudioContext::SetEffect1F(Effect, EffectEx::EAXReverb_Gain_HF, GainHF);
					AudioContext::SetEffect1F(Effect, EffectEx::EAXReverb_Gain_LF, GainLF);
					AudioContext::SetEffect1F(Effect, EffectEx::EAXReverb_Decay_Time, DecayTime);
					AudioContext::SetEffect1F(Effect, EffectEx::EAXReverb_Decay_HF_Ratio, DecayHFRatio);
					AudioContext::SetEffect1F(Effect, EffectEx::EAXReverb_Decay_LF_Ratio, DecayLFRatio);
					AudioContext::SetEffect1F(Effect, EffectEx::EAXReverb_Reflections_Gain, ReflectionsGain);
					AudioContext::SetEffect1F(Effect, EffectEx::EAXReverb_Reflections_Delay, ReflectionsDelay);
					AudioContext::SetEffect1F(Effect, EffectEx::EAXReverb_Late_Reverb_Gain, LateReverbGain);
					AudioContext::SetEffect1F(Effect, EffectEx::EAXReverb_Late_Reverb_Delay, LateReverbDelay);
					AudioContext::SetEffect1F(Effect, EffectEx::EAXReverb_Echo_Time, EchoTime);
					AudioContext::SetEffect1F(Effect, EffectEx::EAXReverb_Echo_Depth, EchoDepth);
					AudioContext::SetEffect1F(Effect, EffectEx::EAXReverb_Modulation_Time, ModulationTime);
					AudioContext::SetEffect1F(Effect, EffectEx::EAXReverb_Modulation_Depth, ModulationDepth);
					AudioContext::SetEffect1F(Effect, EffectEx::EAXReverb_Air_Absorption_Gain_HF, AirAbsorptionGainHF);
					AudioContext::SetEffect1F(Effect, EffectEx::EAXReverb_HF_Reference, HFReference);
					AudioContext::SetEffect1F(Effect, EffectEx::EAXReverb_LF_Reference, LFReference);
					AudioContext::SetEffect1F(Effect, EffectEx::EAXReverb_Room_Rolloff_Factor, RoomRolloffFactor);
					AudioContext::SetEffect1I(Effect, EffectEx::EAXReverb_Decay_HF_Limit, IsDecayHFLimited ? 1 : 0);
				}
				else
				{
					AudioContext::SetEffect1F(Effect, EffectEx::Reverb_Density, Density);
					AudioContext::SetEffect1F(Effect, EffectEx::Reverb_Diffusion, Diffusion);
					AudioContext::SetEffect1F(Effect, EffectEx::Reverb_Gain, Gain);
					AudioContext::SetEffect1F(Effect, EffectEx::Reverb_Gain_HF, GainHF);
					AudioContext::SetEffect1F(Effect, EffectEx::Reverb_Decay_Time, DecayTime);
					AudioContext::SetEffect1F(Effect, EffectEx::Reverb_Decay_HF_Ratio, DecayHFRatio);
					AudioContext::SetEffect1F(Effect, EffectEx::Reverb_Reflections_Gain, ReflectionsGain);
					AudioContext::SetEffect1F(Effect, EffectEx::Reverb_Reflections_Delay, ReflectionsDelay);
					AudioContext::SetEffect1F(Effect, EffectEx::Reverb_Late_Reverb_Gain, LateReverbGain);
					AudioContext::SetEffect1F(Effect, EffectEx::Reverb_Late_Reverb_Delay, LateReverbDelay);
					AudioContext::SetEffect1F(Effect, EffectEx::Reverb_Air_Absorption_Gain_HF, AirAbsorptionGainHF);
					AudioContext::SetEffect1F(Effect, EffectEx::Reverb_Room_Rolloff_Factor, RoomRolloffFactor);
					AudioContext::SetEffect1I(Effect, EffectEx::Reverb_Decay_HF_Limit, IsDecayHFLimited ? 1 : 0);
				}
				ReturnErrorIf;
			}
			void Reverb::Deserialize(Core::Schema* Node)
			{
				VI_ASSERT(Node != nullptr, "schema should be set");
				AudioFilter* NewFilter = GetFilterDeserialized(Node);
				if (NewFilter != nullptr)
					SetFilter(&Filter);

				Engine::Series::Unpack(Node->Find("late-reverb-pan"), &LateReverbPan);
				Engine::Series::Unpack(Node->Find("reflections-pan"), &ReflectionsPan);
				Engine::Series::Unpack(Node->Find("density"), &Density);
				Engine::Series::Unpack(Node->Find("diffusion"), &Diffusion);
				Engine::Series::Unpack(Node->Find("gain"), &Gain);
				Engine::Series::Unpack(Node->Find("gain-hf"), &GainHF);
				Engine::Series::Unpack(Node->Find("gain-lf"), &GainLF);
				Engine::Series::Unpack(Node->Find("decay-time"), &DecayTime);
				Engine::Series::Unpack(Node->Find("decay-hg-ratio"), &DecayHFRatio);
				Engine::Series::Unpack(Node->Find("decay-lf-ratio"), &DecayLFRatio);
				Engine::Series::Unpack(Node->Find("reflections-gain"), &ReflectionsGain);
				Engine::Series::Unpack(Node->Find("reflections-delay"), &ReflectionsDelay);
				Engine::Series::Unpack(Node->Find("late-reverb-gain"), &LateReverbGain);
				Engine::Series::Unpack(Node->Find("late-reverb-delay"), &LateReverbDelay);
				Engine::Series::Unpack(Node->Find("echo-time"), &EchoTime);
				Engine::Series::Unpack(Node->Find("echo-depth"), &EchoDepth);
				Engine::Series::Unpack(Node->Find("modulation-time"), &ModulationTime);
				Engine::Series::Unpack(Node->Find("modulation-depth"), &ModulationDepth);
				Engine::Series::Unpack(Node->Find("air-absorption-gain-hf"), &AirAbsorptionGainHF);
				Engine::Series::Unpack(Node->Find("hf-reference"), &HFReference);
				Engine::Series::Unpack(Node->Find("lf-reference"), &LFReference);
				Engine::Series::Unpack(Node->Find("room-rolloff-factor"), &RoomRolloffFactor);
				Engine::Series::Unpack(Node->Find("decay-hf-limited"), &IsDecayHFLimited);
			}
			void Reverb::Serialize(Core::Schema* Node) const
			{
				VI_ASSERT(Node != nullptr, "schema should be set");
				if (Filter != nullptr)
					Filter->Serialize(Node->Set("filter"));

				Engine::Series::Pack(Node->Set("late-reverb-pan"), LateReverbPan);
				Engine::Series::Pack(Node->Set("reflections-pan"), ReflectionsPan);
				Engine::Series::Pack(Node->Set("density"), Density);
				Engine::Series::Pack(Node->Set("diffusion"), Diffusion);
				Engine::Series::Pack(Node->Set("gain"), Gain);
				Engine::Series::Pack(Node->Set("gain-hf"), GainHF);
				Engine::Series::Pack(Node->Set("gain-lf"), GainLF);
				Engine::Series::Pack(Node->Set("decay-time"), DecayTime);
				Engine::Series::Pack(Node->Set("decay-hg-ratio"), DecayHFRatio);
				Engine::Series::Pack(Node->Set("decay-lf-ratio"), DecayLFRatio);
				Engine::Series::Pack(Node->Set("reflections-gain"), ReflectionsGain);
				Engine::Series::Pack(Node->Set("reflections-delay"), ReflectionsDelay);
				Engine::Series::Pack(Node->Set("late-reverb-gain"), LateReverbGain);
				Engine::Series::Pack(Node->Set("late-reverb-delay"), LateReverbDelay);
				Engine::Series::Pack(Node->Set("echo-time"), EchoTime);
				Engine::Series::Pack(Node->Set("echo-depth"), EchoDepth);
				Engine::Series::Pack(Node->Set("modulation-time"), ModulationTime);
				Engine::Series::Pack(Node->Set("modulation-depth"), ModulationDepth);
				Engine::Series::Pack(Node->Set("air-absorption-gain-hf"), AirAbsorptionGainHF);
				Engine::Series::Pack(Node->Set("hf-reference"), HFReference);
				Engine::Series::Pack(Node->Set("lf-reference"), LFReference);
				Engine::Series::Pack(Node->Set("room-rolloff-factor"), RoomRolloffFactor);
				Engine::Series::Pack(Node->Set("decay-hf-limited"), IsDecayHFLimited);
			}
			AudioEffect* Reverb::Copy() const
			{
				Reverb* Target = new Reverb();
				Target->LateReverbPan = LateReverbPan;
				Target->ReflectionsPan = ReflectionsPan;
				Target->Density = Density;
				Target->Diffusion = Diffusion;
				Target->Gain = Gain;
				Target->GainHF = GainHF;
				Target->GainLF = GainLF;
				Target->DecayTime = DecayTime;
				Target->DecayHFRatio = DecayHFRatio;
				Target->DecayLFRatio = DecayLFRatio;
				Target->ReflectionsGain = ReflectionsGain;
				Target->ReflectionsDelay = ReflectionsDelay;
				Target->LateReverbGain = LateReverbGain;
				Target->LateReverbDelay = LateReverbDelay;
				Target->EchoTime = EchoTime;
				Target->EchoDepth = EchoDepth;
				Target->ModulationTime = ModulationTime;
				Target->ModulationDepth = ModulationDepth;
				Target->AirAbsorptionGainHF = AirAbsorptionGainHF;
				Target->HFReference = HFReference;
				Target->LFReference = LFReference;
				Target->RoomRolloffFactor = RoomRolloffFactor;
				Target->IsDecayHFLimited = IsDecayHFLimited;

				return Target;
			}

			Chorus::Chorus()
			{
				Initialize([this]()
				{
					AudioContext::SetEffect1I(Effect, EffectEx::Effect_Type, (int)EffectEx::Effect_Chorus);
					return true;
				});
			}
			Chorus::~Chorus()
			{

			}
			ExpectsAudio<void> Chorus::Synchronize()
			{
				AudioContext::SetEffect1F(Effect, EffectEx::Chorus_Rate, Rate);
				AudioContext::SetEffect1F(Effect, EffectEx::Chorus_Depth, Depth);
				AudioContext::SetEffect1F(Effect, EffectEx::Chorus_Feedback, Feedback);
				AudioContext::SetEffect1F(Effect, EffectEx::Chorus_Delay, Delay);
				AudioContext::SetEffect1I(Effect, EffectEx::Chorus_Waveform, Waveform);
				AudioContext::SetEffect1I(Effect, EffectEx::Chorus_Phase, Phase);
				ReturnErrorIf;
			}
			void Chorus::Deserialize(Core::Schema* Node)
			{
				VI_ASSERT(Node != nullptr, "schema should be set");
				AudioFilter* NewFilter = GetFilterDeserialized(Node);
				if (NewFilter != nullptr)
					SetFilter(&Filter);

				Engine::Series::Unpack(Node->Find("rate"), &Rate);
				Engine::Series::Unpack(Node->Find("depth"), &Depth);
				Engine::Series::Unpack(Node->Find("feedback"), &Feedback);
				Engine::Series::Unpack(Node->Find("delay"), &Delay);
				Engine::Series::Unpack(Node->Find("waveform"), &Waveform);
				Engine::Series::Unpack(Node->Find("phase"), &Phase);
			}
			void Chorus::Serialize(Core::Schema* Node) const
			{
				VI_ASSERT(Node != nullptr, "schema should be set");
				if (Filter != nullptr)
					Filter->Serialize(Node->Set("filter"));

				Engine::Series::Pack(Node->Set("rate"), Rate);
				Engine::Series::Pack(Node->Set("depth"), Depth);
				Engine::Series::Pack(Node->Set("feedback"), Feedback);
				Engine::Series::Pack(Node->Set("delay"), Delay);
				Engine::Series::Pack(Node->Set("waveform"), Waveform);
				Engine::Series::Pack(Node->Set("phase"), Phase);
			}
			AudioEffect* Chorus::Copy() const
			{
				Chorus* Target = new Chorus();
				Target->Rate = 1.1f;
				Target->Depth = 0.1f;
				Target->Feedback = 0.25f;
				Target->Delay = 0.016f;
				Target->Waveform = 1;
				Target->Phase = 90;

				return Target;
			}

			Distortion::Distortion()
			{
				Initialize([this]()
				{
					AudioContext::SetEffect1I(Effect, EffectEx::Effect_Type, (int)EffectEx::Effect_Distortion);
					return true;
				});
			}
			Distortion::~Distortion()
			{

			}
			ExpectsAudio<void> Distortion::Synchronize()
			{
				AudioContext::SetEffect1F(Effect, EffectEx::Distortion_Edge, Edge);
				AudioContext::SetEffect1F(Effect, EffectEx::Distortion_Gain, Gain);
				AudioContext::SetEffect1F(Effect, EffectEx::Distortion_Lowpass_Cutoff, LowpassCutOff);
				AudioContext::SetEffect1F(Effect, EffectEx::Distortion_EQ_Center, EQCenter);
				AudioContext::SetEffect1F(Effect, EffectEx::Distortion_EQ_Bandwidth, EQBandwidth);
				ReturnErrorIf;
			}
			void Distortion::Deserialize(Core::Schema* Node)
			{
				VI_ASSERT(Node != nullptr, "schema should be set");
				AudioFilter* NewFilter = GetFilterDeserialized(Node);
				if (NewFilter != nullptr)
					SetFilter(&Filter);

				Engine::Series::Unpack(Node->Find("edge"), &Edge);
				Engine::Series::Unpack(Node->Find("gain"), &Gain);
				Engine::Series::Unpack(Node->Find("lowpass-cut-off"), &LowpassCutOff);
				Engine::Series::Unpack(Node->Find("eq-center"), &EQCenter);
				Engine::Series::Unpack(Node->Find("eq-bandwidth"), &EQBandwidth);
			}
			void Distortion::Serialize(Core::Schema* Node) const
			{
				VI_ASSERT(Node != nullptr, "schema should be set");
				if (Filter != nullptr)
					Filter->Serialize(Node->Set("filter"));

				Engine::Series::Pack(Node->Set("edge"), Edge);
				Engine::Series::Pack(Node->Set("gain"), Gain);
				Engine::Series::Pack(Node->Set("lowpass-cut-off"), LowpassCutOff);
				Engine::Series::Pack(Node->Set("eq-center"), EQCenter);
				Engine::Series::Pack(Node->Set("eq-bandwidth"), EQBandwidth);
			}
			AudioEffect* Distortion::Copy() const
			{
				Distortion* Target = new Distortion();
				Target->Edge = 0.2f;
				Target->Gain = 0.05f;
				Target->LowpassCutOff = 8000.0f;
				Target->EQCenter = 3600.0f;
				Target->EQBandwidth = 3600.0f;

				return Target;
			}

			Echo::Echo()
			{
				Initialize([this]()
				{
					AudioContext::SetEffect1I(Effect, EffectEx::Effect_Type, (int)EffectEx::Effect_Echo);
					return true;
				});
			}
			Echo::~Echo()
			{

			}
			ExpectsAudio<void> Echo::Synchronize()
			{
				AudioContext::SetEffect1F(Effect, EffectEx::Echo_Delay, Delay);
				AudioContext::SetEffect1F(Effect, EffectEx::Echo_LR_Delay, LRDelay);
				AudioContext::SetEffect1F(Effect, EffectEx::Echo_Damping, Damping);
				AudioContext::SetEffect1F(Effect, EffectEx::Echo_Feedback, Feedback);
				AudioContext::SetEffect1F(Effect, EffectEx::Echo_Spread, Spread);
				ReturnErrorIf;
			}
			void Echo::Deserialize(Core::Schema* Node)
			{
				VI_ASSERT(Node != nullptr, "schema should be set");
				AudioFilter* NewFilter = GetFilterDeserialized(Node);
				if (NewFilter != nullptr)
					SetFilter(&Filter);

				Engine::Series::Unpack(Node->Find("delay"), &Delay);
				Engine::Series::Unpack(Node->Find("lr-delay"), &LRDelay);
				Engine::Series::Unpack(Node->Find("damping"), &Damping);
				Engine::Series::Unpack(Node->Find("feedback"), &Feedback);
				Engine::Series::Unpack(Node->Find("spread"), &Spread);
			}
			void Echo::Serialize(Core::Schema* Node) const
			{
				VI_ASSERT(Node != nullptr, "schema should be set");
				if (Filter != nullptr)
					Filter->Serialize(Node->Set("filter"));

				Engine::Series::Pack(Node->Set("delay"), Delay);
				Engine::Series::Pack(Node->Set("lr-delay"), LRDelay);
				Engine::Series::Pack(Node->Set("damping"), Damping);
				Engine::Series::Pack(Node->Set("feedback"), Feedback);
				Engine::Series::Pack(Node->Set("spread"), Spread);
			}
			AudioEffect* Echo::Copy() const
			{
				Echo* Target = new Echo();
				Target->Delay = 0.1f;
				Target->LRDelay = 0.1f;
				Target->Damping = 0.5f;
				Target->Feedback = 0.5f;
				Target->Spread = -1.0f;

				return Target;
			}

			Flanger::Flanger()
			{
				Initialize([this]()
				{
					AudioContext::SetEffect1I(Effect, EffectEx::Effect_Type, (int)EffectEx::Effect_Flanger);
					return true;
				});
			}
			Flanger::~Flanger()
			{

			}
			ExpectsAudio<void> Flanger::Synchronize()
			{
				AudioContext::SetEffect1F(Effect, EffectEx::Flanger_Rate, Rate);
				AudioContext::SetEffect1F(Effect, EffectEx::Flanger_Depth, Depth);
				AudioContext::SetEffect1F(Effect, EffectEx::Flanger_Feedback, Feedback);
				AudioContext::SetEffect1F(Effect, EffectEx::Flanger_Delay, Delay);
				AudioContext::SetEffect1I(Effect, EffectEx::Flanger_Waveform, Waveform);
				AudioContext::SetEffect1I(Effect, EffectEx::Flanger_Phase, Phase);
				ReturnErrorIf;
			}
			void Flanger::Deserialize(Core::Schema* Node)
			{
				VI_ASSERT(Node != nullptr, "schema should be set");
				AudioFilter* NewFilter = GetFilterDeserialized(Node);
				if (NewFilter != nullptr)
					SetFilter(&Filter);

				Engine::Series::Unpack(Node->Find("rate"), &Rate);
				Engine::Series::Unpack(Node->Find("depth"), &Depth);
				Engine::Series::Unpack(Node->Find("feedback"), &Feedback);
				Engine::Series::Unpack(Node->Find("delay"), &Delay);
				Engine::Series::Unpack(Node->Find("waveform"), &Waveform);
				Engine::Series::Unpack(Node->Find("phase"), &Phase);
			}
			void Flanger::Serialize(Core::Schema* Node) const
			{
				VI_ASSERT(Node != nullptr, "schema should be set");
				if (Filter != nullptr)
					Filter->Serialize(Node->Set("filter"));

				Engine::Series::Pack(Node->Set("rate"), Rate);
				Engine::Series::Pack(Node->Set("depth"), Depth);
				Engine::Series::Pack(Node->Set("feedback"), Feedback);
				Engine::Series::Pack(Node->Set("delay"), Delay);
				Engine::Series::Pack(Node->Set("waveform"), Waveform);
				Engine::Series::Pack(Node->Set("phase"), Phase);
			}
			AudioEffect* Flanger::Copy() const
			{
				Flanger* Target = new Flanger();
				Target->Rate = 0.27f;
				Target->Depth = 1.0f;
				Target->Feedback = -0.5f;
				Target->Delay = 0.002f;
				Target->Waveform = 1;
				Target->Phase = 0;

				return Target;
			}

			FrequencyShifter::FrequencyShifter()
			{
				Initialize([this]()
				{
					AudioContext::SetEffect1I(Effect, EffectEx::Effect_Type, (int)EffectEx::Effect_Frequency_Shifter);
					return true;
				});
			}
			FrequencyShifter::~FrequencyShifter()
			{

			}
			ExpectsAudio<void> FrequencyShifter::Synchronize()
			{
				AudioContext::SetEffect1F(Effect, EffectEx::Frequency_Shifter_Frequency, Frequency);
				AudioContext::SetEffect1I(Effect, EffectEx::Frequency_Shifter_Left_Direction, LeftDirection);
				AudioContext::SetEffect1I(Effect, EffectEx::Frequency_Shifter_Right_Direction, RightDirection);
				ReturnErrorIf;
			}
			void FrequencyShifter::Deserialize(Core::Schema* Node)
			{
				VI_ASSERT(Node != nullptr, "schema should be set");
				AudioFilter* NewFilter = GetFilterDeserialized(Node);
				if (NewFilter != nullptr)
					SetFilter(&Filter);

				Engine::Series::Unpack(Node->Find("frequency"), &Frequency);
				Engine::Series::Unpack(Node->Find("left-direction"), &LeftDirection);
				Engine::Series::Unpack(Node->Find("right-direction"), &RightDirection);
			}
			void FrequencyShifter::Serialize(Core::Schema* Node) const
			{
				VI_ASSERT(Node != nullptr, "schema should be set");
				if (Filter != nullptr)
					Filter->Serialize(Node->Set("filter"));

				Engine::Series::Pack(Node->Set("frequency"), Frequency);
				Engine::Series::Pack(Node->Set("left-direction"), LeftDirection);
				Engine::Series::Pack(Node->Set("right-direction"), RightDirection);
			}
			AudioEffect* FrequencyShifter::Copy() const
			{
				FrequencyShifter* Target = new FrequencyShifter();
				Target->Frequency = 0.0f;
				Target->LeftDirection = 0;
				Target->RightDirection = 0;

				return Target;
			}

			VocalMorpher::VocalMorpher()
			{
				Initialize([this]()
				{
					AudioContext::SetEffect1I(Effect, EffectEx::Effect_Type, (int)EffectEx::Effect_Vocmorpher);
					return true;
				});
			}
			VocalMorpher::~VocalMorpher()
			{

			}
			ExpectsAudio<void> VocalMorpher::Synchronize()
			{
				AudioContext::SetEffect1F(Effect, EffectEx::Vocmorpher_Rate, Rate);
				AudioContext::SetEffect1I(Effect, EffectEx::Vocmorpher_Phoneme_A, Phonemea);
				AudioContext::SetEffect1I(Effect, EffectEx::Vocmorpher_Phoneme_A_Coarse_Tuning, PhonemeaCoarseTuning);
				AudioContext::SetEffect1I(Effect, EffectEx::Vocmorpher_Phoneme_B, Phonemeb);
				AudioContext::SetEffect1I(Effect, EffectEx::Vocmorpher_Phoneme_B_Coarse_Tuning, PhonemebCoarseTuning);
				AudioContext::SetEffect1I(Effect, EffectEx::Vocmorpher_Waveform, Waveform);
				ReturnErrorIf;
			}
			void VocalMorpher::Deserialize(Core::Schema* Node)
			{
				VI_ASSERT(Node != nullptr, "schema should be set");
				AudioFilter* NewFilter = GetFilterDeserialized(Node);
				if (NewFilter != nullptr)
					SetFilter(&Filter);

				Engine::Series::Unpack(Node->Find("rate"), &Rate);
				Engine::Series::Unpack(Node->Find("phonemea"), &Phonemea);
				Engine::Series::Unpack(Node->Find("phonemea-coarse-tuning"), &PhonemeaCoarseTuning);
				Engine::Series::Unpack(Node->Find("phonemeb"), &Phonemeb);
				Engine::Series::Unpack(Node->Find("phonemeb-coarse-tuning"), &PhonemebCoarseTuning);
				Engine::Series::Unpack(Node->Find("waveform"), &Waveform);
			}
			void VocalMorpher::Serialize(Core::Schema* Node) const
			{
				VI_ASSERT(Node != nullptr, "schema should be set");
				if (Filter != nullptr)
					Filter->Serialize(Node->Set("filter"));

				Engine::Series::Pack(Node->Set("rate"), Rate);
				Engine::Series::Pack(Node->Set("phonemea"), Phonemea);
				Engine::Series::Pack(Node->Set("phonemea-coarse-tuning"), PhonemeaCoarseTuning);
				Engine::Series::Pack(Node->Set("phonemeb"), Phonemeb);
				Engine::Series::Pack(Node->Set("phonemeb-coarse-tuning"), PhonemebCoarseTuning);
				Engine::Series::Pack(Node->Set("waveform"), Waveform);
			}
			AudioEffect* VocalMorpher::Copy() const
			{
				VocalMorpher* Target = new VocalMorpher();
				Target->Rate = 1.41f;
				Target->Phonemea = 0;
				Target->PhonemeaCoarseTuning = 0;
				Target->Phonemeb = 10;
				Target->PhonemebCoarseTuning = 0;
				Target->Waveform = 0;

				return Target;
			}

			PitchShifter::PitchShifter()
			{
				Initialize([this]()
				{
					AudioContext::SetEffect1I(Effect, EffectEx::Effect_Type, (int)EffectEx::Effect_Pitch_Shifter);
					return true;
				});
			}
			PitchShifter::~PitchShifter()
			{

			}
			ExpectsAudio<void> PitchShifter::Synchronize()
			{
				AudioContext::SetEffect1I(Effect, EffectEx::Pitch_Shifter_Coarse_Tune, CoarseTune);
				AudioContext::SetEffect1I(Effect, EffectEx::Pitch_Shifter_Fine_Tune, FineTune);
				ReturnErrorIf;
			}
			void PitchShifter::Deserialize(Core::Schema* Node)
			{
				VI_ASSERT(Node != nullptr, "schema should be set");
				AudioFilter* NewFilter = GetFilterDeserialized(Node);
				if (NewFilter != nullptr)
					SetFilter(&Filter);

				Engine::Series::Unpack(Node->Find("coarse-tune"), &CoarseTune);
				Engine::Series::Unpack(Node->Find("fine-tune"), &FineTune);
			}
			void PitchShifter::Serialize(Core::Schema* Node) const
			{
				VI_ASSERT(Node != nullptr, "schema should be set");
				if (Filter != nullptr)
					Filter->Serialize(Node->Set("filter"));

				Engine::Series::Pack(Node->Set("coarse-tune"), CoarseTune);
				Engine::Series::Pack(Node->Set("fine-tune"), FineTune);
			}
			AudioEffect* PitchShifter::Copy() const
			{
				PitchShifter* Target = new PitchShifter();
				Target->CoarseTune = 12;
				Target->FineTune = 0;

				return Target;
			}

			RingModulator::RingModulator()
			{
				Initialize([this]()
				{
					AudioContext::SetEffect1I(Effect, EffectEx::Effect_Type, (int)EffectEx::Effect_Ring_Modulator);
					return true;
				});
			}
			RingModulator::~RingModulator()
			{

			}
			ExpectsAudio<void> RingModulator::Synchronize()
			{
				AudioContext::SetEffect1F(Effect, EffectEx::Ring_Modulator_Frequency, Frequency);
				AudioContext::SetEffect1F(Effect, EffectEx::Ring_Modulator_Highpass_Cutoff, HighpassCutOff);
				AudioContext::SetEffect1I(Effect, EffectEx::Ring_Modulator_Waveform, Waveform);
				ReturnErrorIf;
			}
			void RingModulator::Deserialize(Core::Schema* Node)
			{
				VI_ASSERT(Node != nullptr, "schema should be set");
				AudioFilter* NewFilter = GetFilterDeserialized(Node);
				if (NewFilter != nullptr)
					SetFilter(&Filter);

				Engine::Series::Unpack(Node->Find("frequency"), &Frequency);
				Engine::Series::Unpack(Node->Find("highpass-cut-off"), &HighpassCutOff);
				Engine::Series::Unpack(Node->Find("waveform"), &Waveform);
			}
			void RingModulator::Serialize(Core::Schema* Node) const
			{
				VI_ASSERT(Node != nullptr, "schema should be set");
				if (Filter != nullptr)
					Filter->Serialize(Node->Set("filter"));

				Engine::Series::Pack(Node->Set("frequency"), Frequency);
				Engine::Series::Pack(Node->Set("highpass-cut-off"), HighpassCutOff);
				Engine::Series::Pack(Node->Set("waveform"), Waveform);
			}
			AudioEffect* RingModulator::Copy() const
			{
				RingModulator* Target = new RingModulator();
				Target->Frequency = 440.0f;
				Target->HighpassCutOff = 800.0f;
				Target->Waveform = 0;

				return Target;
			}

			Autowah::Autowah()
			{
				Initialize([this]()
				{
					AudioContext::SetEffect1I(Effect, EffectEx::Effect_Type, (int)EffectEx::Effect_Autowah);
					return true;
				});
			}
			Autowah::~Autowah()
			{

			}
			ExpectsAudio<void> Autowah::Synchronize()
			{
				AudioContext::SetEffect1F(Effect, EffectEx::Autowah_Attack_Time, AttackTime);
				AudioContext::SetEffect1F(Effect, EffectEx::Autowah_Release_Time, ReleaseTime);
				AudioContext::SetEffect1F(Effect, EffectEx::Autowah_Resonance, Resonance);
				AudioContext::SetEffect1F(Effect, EffectEx::Autowah_Peak_Gain, PeakGain);
				ReturnErrorIf;
			}
			void Autowah::Deserialize(Core::Schema* Node)
			{
				VI_ASSERT(Node != nullptr, "schema should be set");
				AudioFilter* NewFilter = GetFilterDeserialized(Node);
				if (NewFilter != nullptr)
					SetFilter(&Filter);

				Engine::Series::Unpack(Node->Find("attack-time"), &AttackTime);
				Engine::Series::Unpack(Node->Find("release-time"), &ReleaseTime);
				Engine::Series::Unpack(Node->Find("resonance"), &Resonance);
				Engine::Series::Unpack(Node->Find("peak-gain"), &PeakGain);
			}
			void Autowah::Serialize(Core::Schema* Node) const
			{
				VI_ASSERT(Node != nullptr, "schema should be set");
				if (Filter != nullptr)
					Filter->Serialize(Node->Set("filter"));

				Engine::Series::Pack(Node->Set("attack-time"), AttackTime);
				Engine::Series::Pack(Node->Set("release-time"), ReleaseTime);
				Engine::Series::Pack(Node->Set("resonance"), Resonance);
				Engine::Series::Pack(Node->Set("peak-gain"), PeakGain);
			}
			AudioEffect* Autowah::Copy() const
			{
				Autowah* Target = new Autowah();
				Target->AttackTime = 0.06f;
				Target->ReleaseTime = 0.06f;
				Target->Resonance = 1000.0f;
				Target->PeakGain = 11.22f;

				return Target;
			}

			Compressor::Compressor()
			{
				Initialize([this]()
				{
					AudioContext::SetEffect1I(Effect, EffectEx::Effect_Type, (int)EffectEx::Effect_Compressor);
					AudioContext::SetEffect1I(Effect, EffectEx::Compressor_ON_OFF, 1);
					return true;
				});
			}
			Compressor::~Compressor()
			{
			}
			ExpectsAudio<void> Compressor::Synchronize()
			{
				return Core::Expectation::Met;
			}
			void Compressor::Deserialize(Core::Schema* Node)
			{
				VI_ASSERT(Node != nullptr, "schema should be set");
				AudioFilter* NewFilter = GetFilterDeserialized(Node);
				if (NewFilter != nullptr)
					SetFilter(&Filter);
			}
			void Compressor::Serialize(Core::Schema* Node) const
			{
				VI_ASSERT(Node != nullptr, "schema should be set");
				if (Filter != nullptr)
					Filter->Serialize(Node->Set("filter"));
			}
			AudioEffect* Compressor::Copy() const
			{
				return new Compressor();
			}

			Equalizer::Equalizer()
			{
				Initialize([this]()
				{
					AudioContext::SetEffect1I(Effect, EffectEx::Effect_Type, (int)EffectEx::Effect_Equalizer);
					return true;
				});
			}
			Equalizer::~Equalizer()
			{

			}
			ExpectsAudio<void> Equalizer::Synchronize()
			{
				AudioContext::SetEffect1F(Effect, EffectEx::Equalizer_LOW_Gain, LowGain);
				AudioContext::SetEffect1F(Effect, EffectEx::Equalizer_LOW_Cutoff, LowCutOff);
				AudioContext::SetEffect1F(Effect, EffectEx::Equalizer_MID1_Center, Mid1Center);
				AudioContext::SetEffect1F(Effect, EffectEx::Equalizer_MID1_Width, Mid1Width);
				AudioContext::SetEffect1F(Effect, EffectEx::Equalizer_MID2_Gain, Mid2Gain);
				AudioContext::SetEffect1F(Effect, EffectEx::Equalizer_MID2_Center, Mid2Center);
				AudioContext::SetEffect1F(Effect, EffectEx::Equalizer_MID2_Width, Mid2Width);
				AudioContext::SetEffect1F(Effect, EffectEx::Equalizer_HIGH_Gain, HighGain);
				AudioContext::SetEffect1F(Effect, EffectEx::Equalizer_HIGH_Cutoff, HighCutOff);
				ReturnErrorIf;
			}
			void Equalizer::Deserialize(Core::Schema* Node)
			{
				VI_ASSERT(Node != nullptr, "schema should be set");
				AudioFilter* NewFilter = GetFilterDeserialized(Node);
				if (NewFilter != nullptr)
					SetFilter(&Filter);

				Engine::Series::Unpack(Node->Find("low-gain"), &LowGain);
				Engine::Series::Unpack(Node->Find("low-cut-off"), &LowCutOff);
				Engine::Series::Unpack(Node->Find("mid1-gain"), &Mid1Gain);
				Engine::Series::Unpack(Node->Find("mid1-center"), &Mid1Center);
				Engine::Series::Unpack(Node->Find("mid1-width"), &Mid1Width);
				Engine::Series::Unpack(Node->Find("mid2-gain"), &Mid2Gain);
				Engine::Series::Unpack(Node->Find("mid2-center"), &Mid2Center);
				Engine::Series::Unpack(Node->Find("mid2-width"), &Mid2Width);
				Engine::Series::Unpack(Node->Find("high-gain"), &HighGain);
				Engine::Series::Unpack(Node->Find("high-cut-off"), &HighCutOff);
			}
			void Equalizer::Serialize(Core::Schema* Node) const
			{
				VI_ASSERT(Node != nullptr, "schema should be set");
				if (Filter != nullptr)
					Filter->Serialize(Node->Set("filter"));

				Engine::Series::Pack(Node->Find("low-gain"), LowGain);
				Engine::Series::Pack(Node->Find("low-cut-off"), LowCutOff);
				Engine::Series::Pack(Node->Find("mid1-gain"), Mid1Gain);
				Engine::Series::Pack(Node->Find("mid1-center"), Mid1Center);
				Engine::Series::Pack(Node->Find("mid1-width"), Mid1Width);
				Engine::Series::Pack(Node->Find("mid2-gain"), Mid2Gain);
				Engine::Series::Pack(Node->Find("mid2-center"), Mid2Center);
				Engine::Series::Pack(Node->Find("mid2-width"), Mid2Width);
				Engine::Series::Pack(Node->Find("high-gain"), HighGain);
				Engine::Series::Pack(Node->Find("high-cut-off"), HighCutOff);
			}
			AudioEffect* Equalizer::Copy() const
			{
				Equalizer* Target = new Equalizer();
				Target->LowGain = 1.0f;
				Target->LowCutOff = 200.0f;
				Target->Mid1Gain = 1.0f;
				Target->Mid1Center = 500.0f;
				Target->Mid1Width = 1.0f;
				Target->Mid2Gain = 1.0f;
				Target->Mid2Center = 3000.0f;
				Target->Mid2Width = 1.0f;
				Target->HighGain = 1.0f;
				Target->HighCutOff = 6000.0f;

				return Target;
			}
		}
	}
}
