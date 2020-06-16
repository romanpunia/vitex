#include "effects.h"
#include "filters.h"
#include "../engine.h"
#ifdef THAWK_HAS_OPENAL
#include <AL/al.h>
#include <AL/efx.h>
#endif
#define LOAD_PROC(T, X) ((X) = (T)alGetProcAddress(#X))

namespace Tomahawk
{
	namespace Audio
	{
		namespace Effects
		{
#ifdef THAWK_HAS_OPENAL
			LPALGENFILTERS alGenFilters = nullptr;
			LPALDELETEFILTERS alDeleteFilters = nullptr;
			LPALISFILTER alIsFilter = nullptr;
			LPALFILTERI alFilteri = nullptr;
			LPALFILTERIV alFilteriv = nullptr;
			LPALFILTERF alFilterf = nullptr;
			LPALFILTERFV alFilterfv = nullptr;
			LPALGETFILTERI alGetFilteri = nullptr;
			LPALGETFILTERIV alGetFilteriv = nullptr;
			LPALGETFILTERF alGetFilterf = nullptr;
			LPALGETFILTERFV alGetFilterfv = nullptr;
			LPALGENEFFECTS alGenEffects = nullptr;
			LPALDELETEEFFECTS alDeleteEffects = nullptr;
			LPALISEFFECT alIsEffect = nullptr;
			LPALEFFECTI alEffecti = nullptr;
			LPALEFFECTIV alEffectiv = nullptr;
			LPALEFFECTF alEffectf = nullptr;
			LPALEFFECTFV alEffectfv = nullptr;
			LPALGETEFFECTI alGetEffecti = nullptr;
			LPALGETEFFECTIV alGetEffectiv = nullptr;
			LPALGETEFFECTF alGetEffectf = nullptr;
			LPALGETEFFECTFV alGetEffectfv = nullptr;
			LPALGENAUXILIARYEFFECTSLOTS alGenAuxiliaryEffectSlots = nullptr;
			LPALDELETEAUXILIARYEFFECTSLOTS alDeleteAuxiliaryEffectSlots = nullptr;
			LPALISAUXILIARYEFFECTSLOT alIsAuxiliaryEffectSlot = nullptr;
			LPALAUXILIARYEFFECTSLOTI alAuxiliaryEffectSloti = nullptr;
			LPALAUXILIARYEFFECTSLOTIV alAuxiliaryEffectSlotiv = nullptr;
			LPALAUXILIARYEFFECTSLOTF alAuxiliaryEffectSlotf = nullptr;
			LPALAUXILIARYEFFECTSLOTFV alAuxiliaryEffectSlotfv = nullptr;
			LPALGETAUXILIARYEFFECTSLOTI alGetAuxiliaryEffectSloti = nullptr;
			LPALGETAUXILIARYEFFECTSLOTIV alGetAuxiliaryEffectSlotiv = nullptr;
			LPALGETAUXILIARYEFFECTSLOTF alGetAuxiliaryEffectSlotf = nullptr;
			LPALGETAUXILIARYEFFECTSLOTFV alGetAuxiliaryEffectSlotfv = nullptr;
#endif
			void EffectContext::Initialize()
			{
#ifdef THAWK_HAS_OPENAL
				LOAD_PROC(LPALGENFILTERS, alGenFilters);
				LOAD_PROC(LPALDELETEFILTERS, alDeleteFilters);
				LOAD_PROC(LPALISFILTER, alIsFilter);
				LOAD_PROC(LPALFILTERI, alFilteri);
				LOAD_PROC(LPALFILTERIV, alFilteriv);
				LOAD_PROC(LPALFILTERF, alFilterf);
				LOAD_PROC(LPALFILTERFV, alFilterfv);
				LOAD_PROC(LPALGETFILTERI, alGetFilteri);
				LOAD_PROC(LPALGETFILTERIV, alGetFilteriv);
				LOAD_PROC(LPALGETFILTERF, alGetFilterf);
				LOAD_PROC(LPALGETFILTERFV, alGetFilterfv);
				LOAD_PROC(LPALGENEFFECTS, alGenEffects);
				LOAD_PROC(LPALDELETEEFFECTS, alDeleteEffects);
				LOAD_PROC(LPALISEFFECT, alIsEffect);
				LOAD_PROC(LPALEFFECTI, alEffecti);
				LOAD_PROC(LPALEFFECTIV, alEffectiv);
				LOAD_PROC(LPALEFFECTF, alEffectf);
				LOAD_PROC(LPALEFFECTFV, alEffectfv);
				LOAD_PROC(LPALGETEFFECTI, alGetEffecti);
				LOAD_PROC(LPALGETEFFECTIV, alGetEffectiv);
				LOAD_PROC(LPALGETEFFECTF, alGetEffectf);
				LOAD_PROC(LPALGETEFFECTFV, alGetEffectfv);
				LOAD_PROC(LPALGENAUXILIARYEFFECTSLOTS, alGenAuxiliaryEffectSlots);
				LOAD_PROC(LPALDELETEAUXILIARYEFFECTSLOTS, alDeleteAuxiliaryEffectSlots);
				LOAD_PROC(LPALISAUXILIARYEFFECTSLOT, alIsAuxiliaryEffectSlot);
				LOAD_PROC(LPALAUXILIARYEFFECTSLOTI, alAuxiliaryEffectSloti);
				LOAD_PROC(LPALAUXILIARYEFFECTSLOTIV, alAuxiliaryEffectSlotiv);
				LOAD_PROC(LPALAUXILIARYEFFECTSLOTF, alAuxiliaryEffectSlotf);
				LOAD_PROC(LPALAUXILIARYEFFECTSLOTFV, alAuxiliaryEffectSlotfv);
				LOAD_PROC(LPALGETAUXILIARYEFFECTSLOTI, alGetAuxiliaryEffectSloti);
				LOAD_PROC(LPALGETAUXILIARYEFFECTSLOTIV, alGetAuxiliaryEffectSlotiv);
				LOAD_PROC(LPALGETAUXILIARYEFFECTSLOTF, alGetAuxiliaryEffectSlotf);
				LOAD_PROC(LPALGETAUXILIARYEFFECTSLOTFV, alGetAuxiliaryEffectSlotfv);
#endif
			}

			AudioFilter* GetFilterDeserialized(Rest::Document* Node)
			{
				Rest::Document* Filter = Node->Find("filter");
				if (!Filter)
					return nullptr;

				uint64_t FilterId;
				if (!Engine::NMake::Unpack(Filter->Find("id"), &FilterId))
					return nullptr;

				AudioFilter* Target = nullptr;
				if (FilterId == THAWK_COMPONENT_ID(LowpassFilter))
					Target = new Audio::Filters::LowpassFilter();
				else if (FilterId == THAWK_COMPONENT_ID(BandpassFilter))
					Target = new Audio::Filters::BandpassFilter();
				else if (FilterId == THAWK_COMPONENT_ID(HighpassFilter))
					Target = new Audio::Filters::HighpassFilter();

				if (!Target)
				{
					THAWK_WARN("audio filter with id %llu cannot be created", FilterId);
					return nullptr;
				}

				Rest::Document* Meta = Filter->Find("metadata", "");
				if (!Meta)
					Meta = Filter->SetDocument("metadata");

				Target->OnDeserialize(Meta);
				return Target;
			}

			ReverbEffect::ReverbEffect()
			{
#ifdef THAWK_HAS_OPENAL
				CreateLocked([this]()
				{
					EAX = (alGetEnumValue("AL_EFFECT_EAXREVERB") != 0);
					if (EAX)
						alEffecti(Effect, AL_EFFECT_TYPE, AL_EFFECT_EAXREVERB);
					else
						alEffecti(Effect, AL_EFFECT_TYPE, AL_EFFECT_REVERB);
					return true;
				});
#endif
			}
			ReverbEffect::~ReverbEffect()
			{
			}
			void ReverbEffect::Synchronize()
			{
#ifdef THAWK_HAS_OPENAL
				AudioContext::Lock();
				if (EAX)
				{
					float ReflectionsPan3[3];
					ReflectionsPan.Get3(ReflectionsPan3);

					float LateReverbPan3[3];
					LateReverbPan.Get3(LateReverbPan3);

					alEffectfv(Effect, AL_EAXREVERB_REFLECTIONS_PAN, ReflectionsPan3);
					alEffectfv(Effect, AL_EAXREVERB_LATE_REVERB_PAN, LateReverbPan3);
					alEffectf(Effect, AL_EAXREVERB_DENSITY, Density);
					alEffectf(Effect, AL_EAXREVERB_DIFFUSION, Diffusion);
					alEffectf(Effect, AL_EAXREVERB_GAIN, Gain);
					alEffectf(Effect, AL_EAXREVERB_GAINHF, GainHF);
					alEffectf(Effect, AL_EAXREVERB_GAINLF, GainLF);
					alEffectf(Effect, AL_EAXREVERB_DECAY_TIME, DecayTime);
					alEffectf(Effect, AL_EAXREVERB_DECAY_HFRATIO, DecayHFRatio);
					alEffectf(Effect, AL_EAXREVERB_DECAY_LFRATIO, DecayLFRatio);
					alEffectf(Effect, AL_EAXREVERB_REFLECTIONS_GAIN, ReflectionsGain);
					alEffectf(Effect, AL_EAXREVERB_REFLECTIONS_DELAY, ReflectionsDelay);
					alEffectf(Effect, AL_EAXREVERB_LATE_REVERB_GAIN, LateReverbGain);
					alEffectf(Effect, AL_EAXREVERB_LATE_REVERB_DELAY, LateReverbDelay);
					alEffectf(Effect, AL_EAXREVERB_ECHO_TIME, EchoTime);
					alEffectf(Effect, AL_EAXREVERB_ECHO_DEPTH, EchoDepth);
					alEffectf(Effect, AL_EAXREVERB_MODULATION_TIME, ModulationTime);
					alEffectf(Effect, AL_EAXREVERB_MODULATION_DEPTH, ModulationDepth);
					alEffectf(Effect, AL_EAXREVERB_AIR_ABSORPTION_GAINHF, AirAbsorptionGainHF);
					alEffectf(Effect, AL_EAXREVERB_HFREFERENCE, HFReference);
					alEffectf(Effect, AL_EAXREVERB_LFREFERENCE, LFReference);
					alEffectf(Effect, AL_EAXREVERB_ROOM_ROLLOFF_FACTOR, RoomRolloffFactor);
					alEffecti(Effect, AL_EAXREVERB_DECAY_HFLIMIT, IsDecayHFLimited ? 1 : 0);
				}
				else
				{
					alEffectf(Effect, AL_REVERB_DENSITY, Density);
					alEffectf(Effect, AL_REVERB_DIFFUSION, Diffusion);
					alEffectf(Effect, AL_REVERB_GAIN, Gain);
					alEffectf(Effect, AL_REVERB_GAINHF, GainHF);
					alEffectf(Effect, AL_REVERB_DECAY_TIME, DecayTime);
					alEffectf(Effect, AL_REVERB_DECAY_HFRATIO, DecayHFRatio);
					alEffectf(Effect, AL_REVERB_REFLECTIONS_GAIN, ReflectionsGain);
					alEffectf(Effect, AL_REVERB_REFLECTIONS_DELAY, ReflectionsDelay);
					alEffectf(Effect, AL_REVERB_LATE_REVERB_GAIN, LateReverbGain);
					alEffectf(Effect, AL_REVERB_LATE_REVERB_DELAY, LateReverbDelay);
					alEffectf(Effect, AL_REVERB_AIR_ABSORPTION_GAINHF, AirAbsorptionGainHF);
					alEffectf(Effect, AL_REVERB_ROOM_ROLLOFF_FACTOR, RoomRolloffFactor);
					alEffecti(Effect, AL_REVERB_DECAY_HFLIMIT, IsDecayHFLimited ? 1 : 0);
				}
				AudioContext::Unlock();
#endif
			}
			void ReverbEffect::OnDeserialize(Rest::Document* Node)
			{
				AudioFilter* NewFilter = GetFilterDeserialized(Node);
				if (NewFilter != nullptr)
					SetFilter(&Filter);

				Engine::NMake::Unpack(Node->Find("late-reverb-pan"), &LateReverbPan);
				Engine::NMake::Unpack(Node->Find("reflections-pan"), &ReflectionsPan);
				Engine::NMake::Unpack(Node->Find("density"), &Density);
				Engine::NMake::Unpack(Node->Find("diffusion"), &Diffusion);
				Engine::NMake::Unpack(Node->Find("gain"), &Gain);
				Engine::NMake::Unpack(Node->Find("gain-hf"), &GainHF);
				Engine::NMake::Unpack(Node->Find("gain-lf"), &GainLF);
				Engine::NMake::Unpack(Node->Find("decay-time"), &DecayTime);
				Engine::NMake::Unpack(Node->Find("decay-hg-ratio"), &DecayHFRatio);
				Engine::NMake::Unpack(Node->Find("decay-lf-ratio"), &DecayLFRatio);
				Engine::NMake::Unpack(Node->Find("reflections-gain"), &ReflectionsGain);
				Engine::NMake::Unpack(Node->Find("reflections-delay"), &ReflectionsDelay);
				Engine::NMake::Unpack(Node->Find("late-reverb-gain"), &LateReverbGain);
				Engine::NMake::Unpack(Node->Find("late-reverb-delay"), &LateReverbDelay);
				Engine::NMake::Unpack(Node->Find("echo-time"), &EchoTime);
				Engine::NMake::Unpack(Node->Find("echo-depth"), &EchoDepth);
				Engine::NMake::Unpack(Node->Find("modulation-time"), &ModulationTime);
				Engine::NMake::Unpack(Node->Find("modulation-depth"), &ModulationDepth);
				Engine::NMake::Unpack(Node->Find("air-absorption-gain-hf"), &AirAbsorptionGainHF);
				Engine::NMake::Unpack(Node->Find("hf-reference"), &HFReference);
				Engine::NMake::Unpack(Node->Find("lf-reference"), &LFReference);
				Engine::NMake::Unpack(Node->Find("room-rolloff-factor"), &RoomRolloffFactor);
				Engine::NMake::Unpack(Node->Find("decay-hf-limited"), &IsDecayHFLimited);
			}
			void ReverbEffect::OnSerialize(Rest::Document* Node)
			{
				if (Filter != nullptr)
					Filter->OnSerialize(Node->SetDocument("filter"));

				Engine::NMake::Pack(Node->SetDocument("late-reverb-pan"), LateReverbPan);
				Engine::NMake::Pack(Node->SetDocument("reflections-pan"), ReflectionsPan);
				Engine::NMake::Pack(Node->SetDocument("density"), Density);
				Engine::NMake::Pack(Node->SetDocument("diffusion"), Diffusion);
				Engine::NMake::Pack(Node->SetDocument("gain"), Gain);
				Engine::NMake::Pack(Node->SetDocument("gain-hf"), GainHF);
				Engine::NMake::Pack(Node->SetDocument("gain-lf"), GainLF);
				Engine::NMake::Pack(Node->SetDocument("decay-time"), DecayTime);
				Engine::NMake::Pack(Node->SetDocument("decay-hg-ratio"), DecayHFRatio);
				Engine::NMake::Pack(Node->SetDocument("decay-lf-ratio"), DecayLFRatio);
				Engine::NMake::Pack(Node->SetDocument("reflections-gain"), ReflectionsGain);
				Engine::NMake::Pack(Node->SetDocument("reflections-delay"), ReflectionsDelay);
				Engine::NMake::Pack(Node->SetDocument("late-reverb-gain"), LateReverbGain);
				Engine::NMake::Pack(Node->SetDocument("late-reverb-delay"), LateReverbDelay);
				Engine::NMake::Pack(Node->SetDocument("echo-time"), EchoTime);
				Engine::NMake::Pack(Node->SetDocument("echo-depth"), EchoDepth);
				Engine::NMake::Pack(Node->SetDocument("modulation-time"), ModulationTime);
				Engine::NMake::Pack(Node->SetDocument("modulation-depth"), ModulationDepth);
				Engine::NMake::Pack(Node->SetDocument("air-absorption-gain-hf"), AirAbsorptionGainHF);
				Engine::NMake::Pack(Node->SetDocument("hf-reference"), HFReference);
				Engine::NMake::Pack(Node->SetDocument("lf-reference"), LFReference);
				Engine::NMake::Pack(Node->SetDocument("room-rolloff-factor"), RoomRolloffFactor);
				Engine::NMake::Pack(Node->SetDocument("decay-hf-limited"), IsDecayHFLimited);
			}
			AudioEffect* ReverbEffect::OnCopy()
			{
				ReverbEffect* Target = new ReverbEffect();
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

			ChorusEffect::ChorusEffect()
			{
#ifdef THAWK_HAS_OPENAL
				CreateLocked([this]()
				{
					alEffecti(Effect, AL_EFFECT_TYPE, AL_EFFECT_CHORUS);
					return true;
				});
#endif
			}
			ChorusEffect::~ChorusEffect()
			{

			}
			void ChorusEffect::Synchronize()
			{
#ifdef THAWK_HAS_OPENAL
				AudioContext::Lock();
				alEffectf(Effect, AL_CHORUS_DELAY, Rate);
				alEffectf(Effect, AL_CHORUS_DEPTH, Depth);
				alEffectf(Effect, AL_CHORUS_FEEDBACK, Feedback);
				alEffectf(Effect, AL_CHORUS_DELAY, Delay);
				alEffecti(Effect, AL_CHORUS_WAVEFORM, Waveform);
				alEffecti(Effect, AL_CHORUS_PHASE, Phase);
				AudioContext::Unlock();
#endif
			}
			void ChorusEffect::OnDeserialize(Rest::Document* Node)
			{
				AudioFilter* NewFilter = GetFilterDeserialized(Node);
				if (NewFilter != nullptr)
					SetFilter(&Filter);

				Engine::NMake::Unpack(Node->Find("rate"), &Rate);
				Engine::NMake::Unpack(Node->Find("depth"), &Depth);
				Engine::NMake::Unpack(Node->Find("feedback"), &Feedback);
				Engine::NMake::Unpack(Node->Find("delay"), &Delay);
				Engine::NMake::Unpack(Node->Find("waveform"), &Waveform);
				Engine::NMake::Unpack(Node->Find("phase"), &Phase);
			}
			void ChorusEffect::OnSerialize(Rest::Document* Node)
			{
				if (Filter != nullptr)
					Filter->OnSerialize(Node->SetDocument("filter"));

				Engine::NMake::Pack(Node->SetDocument("rate"), Rate);
				Engine::NMake::Pack(Node->SetDocument("depth"), Depth);
				Engine::NMake::Pack(Node->SetDocument("feedback"), Feedback);
				Engine::NMake::Pack(Node->SetDocument("delay"), Delay);
				Engine::NMake::Pack(Node->SetDocument("waveform"), Waveform);
				Engine::NMake::Pack(Node->SetDocument("phase"), Phase);
			}
			AudioEffect* ChorusEffect::OnCopy()
			{
				ChorusEffect* Target = new ChorusEffect();
				Target->Rate = 1.1f;
				Target->Depth = 0.1f;
				Target->Feedback = 0.25f;
				Target->Delay = 0.016f;
				Target->Waveform = 1;
				Target->Phase = 90;

				return Target;
			}

			DistortionEffect::DistortionEffect()
			{
#ifdef THAWK_HAS_OPENAL
				CreateLocked([this]()
				{
					alEffecti(Effect, AL_EFFECT_TYPE, AL_EFFECT_DISTORTION);
					return true;
				});
#endif
			}
			DistortionEffect::~DistortionEffect()
			{

			}
			void DistortionEffect::Synchronize()
			{
#ifdef THAWK_HAS_OPENAL
				AudioContext::Lock();
				alEffectf(Effect, AL_DISTORTION_EDGE, Edge);
				alEffectf(Effect, AL_DISTORTION_GAIN, Gain);
				alEffectf(Effect, AL_DISTORTION_LOWPASS_CUTOFF, LowpassCutOff);
				alEffectf(Effect, AL_DISTORTION_EQCENTER, EQCenter);
				alEffectf(Effect, AL_DISTORTION_EQBANDWIDTH, EQBandwidth);
				AudioContext::Unlock();
#endif
			}
			void DistortionEffect::OnDeserialize(Rest::Document* Node)
			{
				AudioFilter* NewFilter = GetFilterDeserialized(Node);
				if (NewFilter != nullptr)
					SetFilter(&Filter);

				Engine::NMake::Unpack(Node->Find("edge"), &Edge);
				Engine::NMake::Unpack(Node->Find("gain"), &Gain);
				Engine::NMake::Unpack(Node->Find("lowpass-cut-off"), &LowpassCutOff);
				Engine::NMake::Unpack(Node->Find("eq-center"), &EQCenter);
				Engine::NMake::Unpack(Node->Find("eq-bandwidth"), &EQBandwidth);
			}
			void DistortionEffect::OnSerialize(Rest::Document* Node)
			{
				if (Filter != nullptr)
					Filter->OnSerialize(Node->SetDocument("filter"));

				Engine::NMake::Pack(Node->SetDocument("edge"), Edge);
				Engine::NMake::Pack(Node->SetDocument("gain"), Gain);
				Engine::NMake::Pack(Node->SetDocument("lowpass-cut-off"), LowpassCutOff);
				Engine::NMake::Pack(Node->SetDocument("eq-center"), EQCenter);
				Engine::NMake::Pack(Node->SetDocument("eq-bandwidth"), EQBandwidth);
			}
			AudioEffect* DistortionEffect::OnCopy()
			{
				DistortionEffect* Target = new DistortionEffect();
				Target->Edge = 0.2f;
				Target->Gain = 0.05f;
				Target->LowpassCutOff = 8000.0f;
				Target->EQCenter = 3600.0f;
				Target->EQBandwidth = 3600.0f;

				return Target;
			}

			EchoEffect::EchoEffect()
			{
#ifdef THAWK_HAS_OPENAL
				CreateLocked([this]()
				{
					alEffecti(Effect, AL_EFFECT_TYPE, AL_EFFECT_ECHO);
					return true;
				});
#endif
			}
			EchoEffect::~EchoEffect()
			{

			}
			void EchoEffect::Synchronize()
			{
#ifdef THAWK_HAS_OPENAL
				AudioContext::Lock();
				alEffectf(Effect, AL_ECHO_DELAY, Delay);
				alEffectf(Effect, AL_ECHO_LRDELAY, LRDelay);
				alEffectf(Effect, AL_ECHO_DAMPING, Damping);
				alEffectf(Effect, AL_ECHO_FEEDBACK, Feedback);
				alEffectf(Effect, AL_ECHO_SPREAD, Spread);
				AudioContext::Unlock();
#endif
			}
			void EchoEffect::OnDeserialize(Rest::Document* Node)
			{
				AudioFilter* NewFilter = GetFilterDeserialized(Node);
				if (NewFilter != nullptr)
					SetFilter(&Filter);

				Engine::NMake::Unpack(Node->Find("delay"), &Delay);
				Engine::NMake::Unpack(Node->Find("lr-delay"), &LRDelay);
				Engine::NMake::Unpack(Node->Find("damping"), &Damping);
				Engine::NMake::Unpack(Node->Find("feedback"), &Feedback);
				Engine::NMake::Unpack(Node->Find("spread"), &Spread);
			}
			void EchoEffect::OnSerialize(Rest::Document* Node)
			{
				if (Filter != nullptr)
					Filter->OnSerialize(Node->SetDocument("filter"));

				Engine::NMake::Pack(Node->SetDocument("delay"), Delay);
				Engine::NMake::Pack(Node->SetDocument("lr-delay"), LRDelay);
				Engine::NMake::Pack(Node->SetDocument("damping"), Damping);
				Engine::NMake::Pack(Node->SetDocument("feedback"), Feedback);
				Engine::NMake::Pack(Node->SetDocument("spread"), Spread);
			}
			AudioEffect* EchoEffect::OnCopy()
			{
				EchoEffect* Target = new EchoEffect();
				Target->Delay = 0.1f;
				Target->LRDelay = 0.1f;
				Target->Damping = 0.5f;
				Target->Feedback = 0.5f;
				Target->Spread = -1.0f;

				return Target;
			}

			FlangerEffect::FlangerEffect()
			{
#ifdef THAWK_HAS_OPENAL
				CreateLocked([this]()
				{
					alEffecti(Effect, AL_EFFECT_TYPE, AL_EFFECT_FLANGER);
					return true;
				});
#endif
			}
			FlangerEffect::~FlangerEffect()
			{

			}
			void FlangerEffect::Synchronize()
			{
#ifdef THAWK_HAS_OPENAL
				AudioContext::Lock();
				alEffectf(Effect, AL_FLANGER_RATE, Rate);
				alEffectf(Effect, AL_FLANGER_DEPTH, Depth);
				alEffectf(Effect, AL_FLANGER_FEEDBACK, Feedback);
				alEffectf(Effect, AL_FLANGER_DELAY, Delay);
				alEffecti(Effect, AL_FLANGER_WAVEFORM, Waveform);
				alEffecti(Effect, AL_FLANGER_PHASE, Phase);
				AudioContext::Unlock();
#endif
			}
			void FlangerEffect::OnDeserialize(Rest::Document* Node)
			{
				AudioFilter* NewFilter = GetFilterDeserialized(Node);
				if (NewFilter != nullptr)
					SetFilter(&Filter);

				Engine::NMake::Unpack(Node->Find("rate"), &Rate);
				Engine::NMake::Unpack(Node->Find("depth"), &Depth);
				Engine::NMake::Unpack(Node->Find("feedback"), &Feedback);
				Engine::NMake::Unpack(Node->Find("delay"), &Delay);
				Engine::NMake::Unpack(Node->Find("waveform"), &Waveform);
				Engine::NMake::Unpack(Node->Find("phase"), &Phase);
			}
			void FlangerEffect::OnSerialize(Rest::Document* Node)
			{
				if (Filter != nullptr)
					Filter->OnSerialize(Node->SetDocument("filter"));

				Engine::NMake::Pack(Node->SetDocument("rate"), Rate);
				Engine::NMake::Pack(Node->SetDocument("depth"), Depth);
				Engine::NMake::Pack(Node->SetDocument("feedback"), Feedback);
				Engine::NMake::Pack(Node->SetDocument("delay"), Delay);
				Engine::NMake::Pack(Node->SetDocument("waveform"), Waveform);
				Engine::NMake::Pack(Node->SetDocument("phase"), Phase);
			}
			AudioEffect* FlangerEffect::OnCopy()
			{
				FlangerEffect* Target = new FlangerEffect();
				Target->Rate = 0.27f;
				Target->Depth = 1.0f;
				Target->Feedback = -0.5f;
				Target->Delay = 0.002f;
				Target->Waveform = 1;
				Target->Phase = 0;

				return Target;
			}

			FrequencyShifterEffect::FrequencyShifterEffect()
			{
#ifdef THAWK_HAS_OPENAL
				CreateLocked([this]()
				{
					alEffecti(Effect, AL_EFFECT_TYPE, AL_EFFECT_FREQUENCY_SHIFTER);
					return true;
				});
#endif
			}
			FrequencyShifterEffect::~FrequencyShifterEffect()
			{

			}
			void FrequencyShifterEffect::Synchronize()
			{
#ifdef THAWK_HAS_OPENAL
				AudioContext::Lock();
				alEffectf(Effect, AL_FREQUENCY_SHIFTER_FREQUENCY, Frequency);
				alEffecti(Effect, AL_FREQUENCY_SHIFTER_LEFT_DIRECTION, LeftDirection);
				alEffecti(Effect, AL_FREQUENCY_SHIFTER_RIGHT_DIRECTION, RightDirection);
				AudioContext::Unlock();
#endif
			}
			void FrequencyShifterEffect::OnDeserialize(Rest::Document* Node)
			{
				AudioFilter* NewFilter = GetFilterDeserialized(Node);
				if (NewFilter != nullptr)
					SetFilter(&Filter);

				Engine::NMake::Unpack(Node->Find("frequency"), &Frequency);
				Engine::NMake::Unpack(Node->Find("left-direction"), &LeftDirection);
				Engine::NMake::Unpack(Node->Find("right-direction"), &RightDirection);
			}
			void FrequencyShifterEffect::OnSerialize(Rest::Document* Node)
			{
				if (Filter != nullptr)
					Filter->OnSerialize(Node->SetDocument("filter"));

				Engine::NMake::Pack(Node->SetDocument("frequency"), &Frequency);
				Engine::NMake::Pack(Node->SetDocument("left-direction"), &LeftDirection);
				Engine::NMake::Pack(Node->SetDocument("right-direction"), &RightDirection);
			}
			AudioEffect* FrequencyShifterEffect::OnCopy()
			{
				FrequencyShifterEffect* Target = new FrequencyShifterEffect();
				Target->Frequency = 0.0f;
				Target->LeftDirection = 0;
				Target->RightDirection = 0;

				return Target;
			}

			VocalMorpherEffect::VocalMorpherEffect()
			{
#ifdef THAWK_HAS_OPENAL
				CreateLocked([this]()
				{
					alEffecti(Effect, AL_EFFECT_TYPE, AL_EFFECT_VOCAL_MORPHER);
					return true;
				});
#endif
			}
			VocalMorpherEffect::~VocalMorpherEffect()
			{

			}
			void VocalMorpherEffect::Synchronize()
			{
#ifdef THAWK_HAS_OPENAL
				AudioContext::Lock();
				alEffectf(Effect, AL_VOCAL_MORPHER_RATE, Rate);
				alEffecti(Effect, AL_VOCAL_MORPHER_PHONEMEA, Phonemea);
				alEffecti(Effect, AL_VOCAL_MORPHER_PHONEMEA_COARSE_TUNING, PhonemeaCoarseTuning);
				alEffecti(Effect, AL_VOCAL_MORPHER_PHONEMEB, Phonemeb);
				alEffecti(Effect, AL_VOCAL_MORPHER_PHONEMEB_COARSE_TUNING, PhonemebCoarseTuning);
				alEffecti(Effect, AL_VOCAL_MORPHER_WAVEFORM, Waveform);
				AudioContext::Unlock();
#endif
			}
			void VocalMorpherEffect::OnDeserialize(Rest::Document* Node)
			{
				AudioFilter* NewFilter = GetFilterDeserialized(Node);
				if (NewFilter != nullptr)
					SetFilter(&Filter);

				Engine::NMake::Unpack(Node->Find("rate"), &Rate);
				Engine::NMake::Unpack(Node->Find("phonemea"), &Phonemea);
				Engine::NMake::Unpack(Node->Find("phonemea-coarse-tuning"), &PhonemeaCoarseTuning);
				Engine::NMake::Unpack(Node->Find("phonemeb"), &Phonemeb);
				Engine::NMake::Unpack(Node->Find("phonemeb-coarse-tuning"), &PhonemebCoarseTuning);
				Engine::NMake::Unpack(Node->Find("waveform"), &Waveform);
			}
			void VocalMorpherEffect::OnSerialize(Rest::Document* Node)
			{
				if (Filter != nullptr)
					Filter->OnSerialize(Node->SetDocument("filter"));

				Engine::NMake::Pack(Node->SetDocument("rate"), Rate);
				Engine::NMake::Pack(Node->SetDocument("phonemea"), Phonemea);
				Engine::NMake::Pack(Node->SetDocument("phonemea-coarse-tuning"), PhonemeaCoarseTuning);
				Engine::NMake::Pack(Node->SetDocument("phonemeb"), Phonemeb);
				Engine::NMake::Pack(Node->SetDocument("phonemeb-coarse-tuning"), PhonemebCoarseTuning);
				Engine::NMake::Pack(Node->SetDocument("waveform"), Waveform);
			}
			AudioEffect* VocalMorpherEffect::OnCopy()
			{
				VocalMorpherEffect* Target = new VocalMorpherEffect();
				Target->Rate = 1.41f;
				Target->Phonemea = 0;
				Target->PhonemeaCoarseTuning = 0;
				Target->Phonemeb = 10;
				Target->PhonemebCoarseTuning = 0;
				Target->Waveform = 0;

				return Target;
			}

			PitchShifterEffect::PitchShifterEffect()
			{
#ifdef THAWK_HAS_OPENAL
				CreateLocked([this]()
				{
					alEffecti(Effect, AL_EFFECT_TYPE, AL_EFFECT_PITCH_SHIFTER);
					return true;
				});
#endif
			}
			PitchShifterEffect::~PitchShifterEffect()
			{

			}
			void PitchShifterEffect::Synchronize()
			{
#ifdef THAWK_HAS_OPENAL
				AudioContext::Lock();
				alEffecti(Effect, AL_PITCH_SHIFTER_COARSE_TUNE, CoarseTune);
				alEffecti(Effect, AL_PITCH_SHIFTER_FINE_TUNE, FineTune);
				AudioContext::Unlock();
#endif
			}
			void PitchShifterEffect::OnDeserialize(Rest::Document* Node)
			{
				AudioFilter* NewFilter = GetFilterDeserialized(Node);
				if (NewFilter != nullptr)
					SetFilter(&Filter);

				Engine::NMake::Unpack(Node->Find("coarse-tune"), &CoarseTune);
				Engine::NMake::Unpack(Node->Find("fine-tune"), &FineTune);
			}
			void PitchShifterEffect::OnSerialize(Rest::Document* Node)
			{
				if (Filter != nullptr)
					Filter->OnSerialize(Node->SetDocument("filter"));

				Engine::NMake::Pack(Node->SetDocument("coarse-tune"), CoarseTune);
				Engine::NMake::Pack(Node->SetDocument("fine-tune"), FineTune);
			}
			AudioEffect* PitchShifterEffect::OnCopy()
			{
				PitchShifterEffect* Target = new PitchShifterEffect();
				Target->CoarseTune = 12;
				Target->FineTune = 0;

				return Target;
			}

			RingModulatorEffect::RingModulatorEffect()
			{
#ifdef THAWK_HAS_OPENAL
				CreateLocked([this]()
				{
					alEffecti(Effect, AL_EFFECT_TYPE, AL_EFFECT_RING_MODULATOR);
					return true;
				});
#endif
			}
			RingModulatorEffect::~RingModulatorEffect()
			{

			}
			void RingModulatorEffect::Synchronize()
			{
#ifdef THAWK_HAS_OPENAL
				AudioContext::Lock();
				alEffectf(Effect, AL_RING_MODULATOR_FREQUENCY, Frequency);
				alEffectf(Effect, AL_RING_MODULATOR_HIGHPASS_CUTOFF, HighpassCutOff);
				alEffecti(Effect, AL_RING_MODULATOR_WAVEFORM, Waveform);
				AudioContext::Unlock();
#endif
			}
			void RingModulatorEffect::OnDeserialize(Rest::Document* Node)
			{
				AudioFilter* NewFilter = GetFilterDeserialized(Node);
				if (NewFilter != nullptr)
					SetFilter(&Filter);

				Engine::NMake::Unpack(Node->Find("frequency"), &Frequency);
				Engine::NMake::Unpack(Node->Find("highpass-cut-off"), &HighpassCutOff);
				Engine::NMake::Unpack(Node->Find("waveform"), &Waveform);
			}
			void RingModulatorEffect::OnSerialize(Rest::Document* Node)
			{
				if (Filter != nullptr)
					Filter->OnSerialize(Node->SetDocument("filter"));

				Engine::NMake::Pack(Node->SetDocument("frequency"), Frequency);
				Engine::NMake::Pack(Node->SetDocument("highpass-cut-off"), HighpassCutOff);
				Engine::NMake::Pack(Node->SetDocument("waveform"), Waveform);
			}
			AudioEffect* RingModulatorEffect::OnCopy()
			{
				RingModulatorEffect* Target = new RingModulatorEffect();
				Target->Frequency = 440.0f;
				Target->HighpassCutOff = 800.0f;
				Target->Waveform = 0;

				return Target;
			}

			AutowahEffect::AutowahEffect()
			{
#ifdef THAWK_HAS_OPENAL
				CreateLocked([this]()
				{
					alEffecti(Effect, AL_EFFECT_TYPE, AL_EFFECT_AUTOWAH);
					return true;
				});
#endif
			}
			AutowahEffect::~AutowahEffect()
			{

			}
			void AutowahEffect::Synchronize()
			{
#ifdef THAWK_HAS_OPENAL
				AudioContext::Lock();
				alEffectf(Effect, AL_AUTOWAH_ATTACK_TIME, AttackTime);
				alEffectf(Effect, AL_AUTOWAH_RELEASE_TIME, ReleaseTime);
				alEffectf(Effect, AL_AUTOWAH_RESONANCE, Resonance);
				alEffectf(Effect, AL_AUTOWAH_PEAK_GAIN, PeakGain);
				AudioContext::Unlock();
#endif
			}
			void AutowahEffect::OnDeserialize(Rest::Document* Node)
			{
				AudioFilter* NewFilter = GetFilterDeserialized(Node);
				if (NewFilter != nullptr)
					SetFilter(&Filter);

				Engine::NMake::Unpack(Node->Find("attack-time"), &AttackTime);
				Engine::NMake::Unpack(Node->Find("release-time"), &ReleaseTime);
				Engine::NMake::Unpack(Node->Find("resonance"), &Resonance);
				Engine::NMake::Unpack(Node->Find("peak-gain"), &PeakGain);
			}
			void AutowahEffect::OnSerialize(Rest::Document* Node)
			{
				if (Filter != nullptr)
					Filter->OnSerialize(Node->SetDocument("filter"));

				Engine::NMake::Pack(Node->SetDocument("attack-time"), AttackTime);
				Engine::NMake::Pack(Node->SetDocument("release-time"), ReleaseTime);
				Engine::NMake::Pack(Node->SetDocument("resonance"), Resonance);
				Engine::NMake::Pack(Node->SetDocument("peak-gain"), PeakGain);
			}
			AudioEffect* AutowahEffect::OnCopy()
			{
				AutowahEffect* Target = new AutowahEffect();
				Target->AttackTime = 0.06f;
				Target->ReleaseTime = 0.06f;
				Target->Resonance = 1000.0f;
				Target->PeakGain = 11.22f;

				return Target;
			}

			CompressorEffect::CompressorEffect()
			{
#ifdef THAWK_HAS_OPENAL
				CreateLocked([this]()
				{
					alEffecti(Effect, AL_EFFECT_TYPE, AL_EFFECT_COMPRESSOR);
					alEffecti(Effect, AL_COMPRESSOR_ONOFF, 1);
					return true;
				});
#endif
			}
			CompressorEffect::~CompressorEffect()
			{
			}
			void CompressorEffect::Synchronize()
			{
			}
			void CompressorEffect::OnDeserialize(Rest::Document* Node)
			{
				AudioFilter* NewFilter = GetFilterDeserialized(Node);
				if (NewFilter != nullptr)
					SetFilter(&Filter);
			}
			void CompressorEffect::OnSerialize(Rest::Document* Node)
			{
				if (Filter != nullptr)
					Filter->OnSerialize(Node->SetDocument("filter"));
			}
			AudioEffect* CompressorEffect::OnCopy()
			{
				return new CompressorEffect();
			}

			EqualizerEffect::EqualizerEffect()
			{
#ifdef THAWK_HAS_OPENAL
				CreateLocked([this]()
				{
					alEffecti(Effect, AL_EFFECT_TYPE, AL_EFFECT_EQUALIZER);
					return true;
				});
#endif
			}
			EqualizerEffect::~EqualizerEffect()
			{

			}
			void EqualizerEffect::Synchronize()
			{
#ifdef THAWK_HAS_OPENAL
				AudioContext::Lock();
				alEffectf(Effect, AL_EQUALIZER_LOW_GAIN, LowGain);
				alEffectf(Effect, AL_EQUALIZER_LOW_CUTOFF, LowCutOff);
				alEffectf(Effect, AL_EQUALIZER_MID1_CENTER, Mid1Center);
				alEffectf(Effect, AL_EQUALIZER_MID1_WIDTH, Mid1Width);
				alEffectf(Effect, AL_EQUALIZER_MID2_GAIN, Mid2Gain);
				alEffectf(Effect, AL_EQUALIZER_MID2_CENTER, Mid2Center);
				alEffectf(Effect, AL_EQUALIZER_MID2_WIDTH, Mid2Width);
				alEffectf(Effect, AL_EQUALIZER_HIGH_GAIN, HighGain);
				alEffectf(Effect, AL_EQUALIZER_HIGH_CUTOFF, HighCutOff);
				AudioContext::Unlock();
#endif
			}
			void EqualizerEffect::OnDeserialize(Rest::Document* Node)
			{
				AudioFilter* NewFilter = GetFilterDeserialized(Node);
				if (NewFilter != nullptr)
					SetFilter(&Filter);

				Engine::NMake::Unpack(Node->Find("low-gain"), &LowGain);
				Engine::NMake::Unpack(Node->Find("low-cut-off"), &LowCutOff);
				Engine::NMake::Unpack(Node->Find("mid1-gain"), &Mid1Gain);
				Engine::NMake::Unpack(Node->Find("mid1-center"), &Mid1Center);
				Engine::NMake::Unpack(Node->Find("mid1-width"), &Mid1Width);
				Engine::NMake::Unpack(Node->Find("mid2-gain"), &Mid2Gain);
				Engine::NMake::Unpack(Node->Find("mid2-center"), &Mid2Center);
				Engine::NMake::Unpack(Node->Find("mid2-width"), &Mid2Width);
				Engine::NMake::Unpack(Node->Find("high-gain"), &HighGain);
				Engine::NMake::Unpack(Node->Find("high-cut-off"), &HighCutOff);
			}
			void EqualizerEffect::OnSerialize(Rest::Document* Node)
			{
				if (Filter != nullptr)
					Filter->OnSerialize(Node->SetDocument("filter"));

				Engine::NMake::Pack(Node->Find("low-gain"), LowGain);
				Engine::NMake::Pack(Node->Find("low-cut-off"), LowCutOff);
				Engine::NMake::Pack(Node->Find("mid1-gain"), Mid1Gain);
				Engine::NMake::Pack(Node->Find("mid1-center"), Mid1Center);
				Engine::NMake::Pack(Node->Find("mid1-width"), Mid1Width);
				Engine::NMake::Pack(Node->Find("mid2-gain"), Mid2Gain);
				Engine::NMake::Pack(Node->Find("mid2-center"), Mid2Center);
				Engine::NMake::Pack(Node->Find("mid2-width"), Mid2Width);
				Engine::NMake::Pack(Node->Find("high-gain"),&HighGain);
				Engine::NMake::Pack(Node->Find("high-cut-off"), HighCutOff);
			}
			AudioEffect* EqualizerEffect::OnCopy()
			{
				EqualizerEffect* Target = new EqualizerEffect();
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