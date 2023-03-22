#include "effects.h"
#include "filters.h"
#include "../core/engine.h"
#if defined(ED_HAS_OPENAL) && defined(HAS_EFX)
#ifdef ED_AL_AT_OPENAL
#include <OpenAL/al.h>
#else
#include <AL/al.h>
#include <AL/efx.h>
#define HAS_EFX
#endif
#endif
#define LOAD_PROC(T, X) ((X) = (T)alGetProcAddress(#X))
#if defined(ED_HAS_OPENAL) && defined(HAS_EFX)
namespace
{
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
}
#endif

namespace Edge
{
	namespace Audio
	{
		namespace Effects
		{
			void EffectContext::Create()
			{
				ED_TRACE("[audio] load effect functions");
#if defined(ED_HAS_OPENAL) && defined(HAS_EFX)
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
				{
					ED_WARN("[audio] filter with id %" PRIu64 " cannot be created", Id);
					return nullptr;
				}

				Core::Schema* Meta = Filter->Find("metadata");
				if (!Meta)
					Meta = Filter->Set("metadata");

				Target->Deserialize(Meta);
				return Target;
			}

			Reverb::Reverb()
			{
#if defined(ED_HAS_OPENAL) && defined(HAS_EFX)
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
			Reverb::~Reverb()
			{
			}
			void Reverb::Synchronize()
			{
#if defined(ED_HAS_OPENAL) && defined(HAS_EFX)
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
			void Reverb::Deserialize(Core::Schema* Node)
			{
				ED_ASSERT_V(Node != nullptr, "schema should be set");
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
				ED_ASSERT_V(Node != nullptr, "schema should be set");
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
#if defined(ED_HAS_OPENAL) && defined(HAS_EFX)
				CreateLocked([this]()
				{
					alEffecti(Effect, AL_EFFECT_TYPE, AL_EFFECT_CHORUS);
					return true;
				});
#endif
			}
			Chorus::~Chorus()
			{

			}
			void Chorus::Synchronize()
			{
#if defined(ED_HAS_OPENAL) && defined(HAS_EFX)
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
			void Chorus::Deserialize(Core::Schema* Node)
			{
				ED_ASSERT_V(Node != nullptr, "schema should be set");
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
				ED_ASSERT_V(Node != nullptr, "schema should be set");
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
#if defined(ED_HAS_OPENAL) && defined(HAS_EFX)
				CreateLocked([this]()
				{
					alEffecti(Effect, AL_EFFECT_TYPE, AL_EFFECT_DISTORTION);
					return true;
				});
#endif
			}
			Distortion::~Distortion()
			{

			}
			void Distortion::Synchronize()
			{
#if defined(ED_HAS_OPENAL) && defined(HAS_EFX)
				AudioContext::Lock();
				alEffectf(Effect, AL_DISTORTION_EDGE, Edge);
				alEffectf(Effect, AL_DISTORTION_GAIN, Gain);
				alEffectf(Effect, AL_DISTORTION_LOWPASS_CUTOFF, LowpassCutOff);
				alEffectf(Effect, AL_DISTORTION_EQCENTER, EQCenter);
				alEffectf(Effect, AL_DISTORTION_EQBANDWIDTH, EQBandwidth);
				AudioContext::Unlock();
#endif
			}
			void Distortion::Deserialize(Core::Schema* Node)
			{
				ED_ASSERT_V(Node != nullptr, "schema should be set");
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
				ED_ASSERT_V(Node != nullptr, "schema should be set");
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
#if defined(ED_HAS_OPENAL) && defined(HAS_EFX)
				CreateLocked([this]()
				{
					alEffecti(Effect, AL_EFFECT_TYPE, AL_EFFECT_ECHO);
					return true;
				});
#endif
			}
			Echo::~Echo()
			{

			}
			void Echo::Synchronize()
			{
#if defined(ED_HAS_OPENAL) && defined(HAS_EFX)
				AudioContext::Lock();
				alEffectf(Effect, AL_ECHO_DELAY, Delay);
				alEffectf(Effect, AL_ECHO_LRDELAY, LRDelay);
				alEffectf(Effect, AL_ECHO_DAMPING, Damping);
				alEffectf(Effect, AL_ECHO_FEEDBACK, Feedback);
				alEffectf(Effect, AL_ECHO_SPREAD, Spread);
				AudioContext::Unlock();
#endif
			}
			void Echo::Deserialize(Core::Schema* Node)
			{
				ED_ASSERT_V(Node != nullptr, "schema should be set");
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
				ED_ASSERT_V(Node != nullptr, "schema should be set");
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
#if defined(ED_HAS_OPENAL) && defined(HAS_EFX)
				CreateLocked([this]()
				{
					alEffecti(Effect, AL_EFFECT_TYPE, AL_EFFECT_FLANGER);
					return true;
				});
#endif
			}
			Flanger::~Flanger()
			{

			}
			void Flanger::Synchronize()
			{
#if defined(ED_HAS_OPENAL) && defined(HAS_EFX)
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
			void Flanger::Deserialize(Core::Schema* Node)
			{
				ED_ASSERT_V(Node != nullptr, "schema should be set");
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
				ED_ASSERT_V(Node != nullptr, "schema should be set");
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
#if defined(ED_HAS_OPENAL) && defined(HAS_EFX)
				CreateLocked([this]()
				{
					alEffecti(Effect, AL_EFFECT_TYPE, AL_EFFECT_FREQUENCY_SHIFTER);
					return true;
				});
#endif
			}
			FrequencyShifter::~FrequencyShifter()
			{

			}
			void FrequencyShifter::Synchronize()
			{
#if defined(ED_HAS_OPENAL) && defined(HAS_EFX)
				AudioContext::Lock();
				alEffectf(Effect, AL_FREQUENCY_SHIFTER_FREQUENCY, Frequency);
				alEffecti(Effect, AL_FREQUENCY_SHIFTER_LEFT_DIRECTION, LeftDirection);
				alEffecti(Effect, AL_FREQUENCY_SHIFTER_RIGHT_DIRECTION, RightDirection);
				AudioContext::Unlock();
#endif
			}
			void FrequencyShifter::Deserialize(Core::Schema* Node)
			{
				ED_ASSERT_V(Node != nullptr, "schema should be set");
				AudioFilter* NewFilter = GetFilterDeserialized(Node);
				if (NewFilter != nullptr)
					SetFilter(&Filter);

				Engine::Series::Unpack(Node->Find("frequency"), &Frequency);
				Engine::Series::Unpack(Node->Find("left-direction"), &LeftDirection);
				Engine::Series::Unpack(Node->Find("right-direction"), &RightDirection);
			}
			void FrequencyShifter::Serialize(Core::Schema* Node) const
			{
				ED_ASSERT_V(Node != nullptr, "schema should be set");
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
#if defined(ED_HAS_OPENAL) && defined(HAS_EFX)
				CreateLocked([this]()
				{
					alEffecti(Effect, AL_EFFECT_TYPE, AL_EFFECT_VOCAL_MORPHER);
					return true;
				});
#endif
			}
			VocalMorpher::~VocalMorpher()
			{

			}
			void VocalMorpher::Synchronize()
			{
#if defined(ED_HAS_OPENAL) && defined(HAS_EFX)
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
			void VocalMorpher::Deserialize(Core::Schema* Node)
			{
				ED_ASSERT_V(Node != nullptr, "schema should be set");
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
				ED_ASSERT_V(Node != nullptr, "schema should be set");
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
#if defined(ED_HAS_OPENAL) && defined(HAS_EFX)
				CreateLocked([this]()
				{
					alEffecti(Effect, AL_EFFECT_TYPE, AL_EFFECT_PITCH_SHIFTER);
					return true;
				});
#endif
			}
			PitchShifter::~PitchShifter()
			{

			}
			void PitchShifter::Synchronize()
			{
#if defined(ED_HAS_OPENAL) && defined(HAS_EFX)
				AudioContext::Lock();
				alEffecti(Effect, AL_PITCH_SHIFTER_COARSE_TUNE, CoarseTune);
				alEffecti(Effect, AL_PITCH_SHIFTER_FINE_TUNE, FineTune);
				AudioContext::Unlock();
#endif
			}
			void PitchShifter::Deserialize(Core::Schema* Node)
			{
				ED_ASSERT_V(Node != nullptr, "schema should be set");
				AudioFilter* NewFilter = GetFilterDeserialized(Node);
				if (NewFilter != nullptr)
					SetFilter(&Filter);

				Engine::Series::Unpack(Node->Find("coarse-tune"), &CoarseTune);
				Engine::Series::Unpack(Node->Find("fine-tune"), &FineTune);
			}
			void PitchShifter::Serialize(Core::Schema* Node) const
			{
				ED_ASSERT_V(Node != nullptr, "schema should be set");
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
#if defined(ED_HAS_OPENAL) && defined(HAS_EFX)
				CreateLocked([this]()
				{
					alEffecti(Effect, AL_EFFECT_TYPE, AL_EFFECT_RING_MODULATOR);
					return true;
				});
#endif
			}
			RingModulator::~RingModulator()
			{

			}
			void RingModulator::Synchronize()
			{
#if defined(ED_HAS_OPENAL) && defined(HAS_EFX)
				AudioContext::Lock();
				alEffectf(Effect, AL_RING_MODULATOR_FREQUENCY, Frequency);
				alEffectf(Effect, AL_RING_MODULATOR_HIGHPASS_CUTOFF, HighpassCutOff);
				alEffecti(Effect, AL_RING_MODULATOR_WAVEFORM, Waveform);
				AudioContext::Unlock();
#endif
			}
			void RingModulator::Deserialize(Core::Schema* Node)
			{
				ED_ASSERT_V(Node != nullptr, "schema should be set");
				AudioFilter* NewFilter = GetFilterDeserialized(Node);
				if (NewFilter != nullptr)
					SetFilter(&Filter);

				Engine::Series::Unpack(Node->Find("frequency"), &Frequency);
				Engine::Series::Unpack(Node->Find("highpass-cut-off"), &HighpassCutOff);
				Engine::Series::Unpack(Node->Find("waveform"), &Waveform);
			}
			void RingModulator::Serialize(Core::Schema* Node) const
			{
				ED_ASSERT_V(Node != nullptr, "schema should be set");
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
#if defined(ED_HAS_OPENAL) && defined(HAS_EFX)
				CreateLocked([this]()
				{
					alEffecti(Effect, AL_EFFECT_TYPE, AL_EFFECT_AUTOWAH);
					return true;
				});
#endif
			}
			Autowah::~Autowah()
			{

			}
			void Autowah::Synchronize()
			{
#if defined(ED_HAS_OPENAL) && defined(HAS_EFX)
				AudioContext::Lock();
				alEffectf(Effect, AL_AUTOWAH_ATTACK_TIME, AttackTime);
				alEffectf(Effect, AL_AUTOWAH_RELEASE_TIME, ReleaseTime);
				alEffectf(Effect, AL_AUTOWAH_RESONANCE, Resonance);
				alEffectf(Effect, AL_AUTOWAH_PEAK_GAIN, PeakGain);
				AudioContext::Unlock();
#endif
			}
			void Autowah::Deserialize(Core::Schema* Node)
			{
				ED_ASSERT_V(Node != nullptr, "schema should be set");
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
				ED_ASSERT_V(Node != nullptr, "schema should be set");
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
#if defined(ED_HAS_OPENAL) && defined(HAS_EFX)
				CreateLocked([this]()
				{
					alEffecti(Effect, AL_EFFECT_TYPE, AL_EFFECT_COMPRESSOR);
					alEffecti(Effect, AL_COMPRESSOR_ONOFF, 1);
					return true;
				});
#endif
			}
			Compressor::~Compressor()
			{
			}
			void Compressor::Synchronize()
			{
			}
			void Compressor::Deserialize(Core::Schema* Node)
			{
				ED_ASSERT_V(Node != nullptr, "schema should be set");
				AudioFilter* NewFilter = GetFilterDeserialized(Node);
				if (NewFilter != nullptr)
					SetFilter(&Filter);
			}
			void Compressor::Serialize(Core::Schema* Node) const
			{
				ED_ASSERT_V(Node != nullptr, "schema should be set");
				if (Filter != nullptr)
					Filter->Serialize(Node->Set("filter"));
			}
			AudioEffect* Compressor::Copy() const
			{
				return new Compressor();
			}

			Equalizer::Equalizer()
			{
#if defined(ED_HAS_OPENAL) && defined(HAS_EFX)
				CreateLocked([this]()
				{
					alEffecti(Effect, AL_EFFECT_TYPE, AL_EFFECT_EQUALIZER);
					return true;
				});
#endif
			}
			Equalizer::~Equalizer()
			{

			}
			void Equalizer::Synchronize()
			{
#if defined(ED_HAS_OPENAL) && defined(HAS_EFX)
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
			void Equalizer::Deserialize(Core::Schema* Node)
			{
				ED_ASSERT_V(Node != nullptr, "schema should be set");
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
				ED_ASSERT_V(Node != nullptr, "schema should be set");
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
