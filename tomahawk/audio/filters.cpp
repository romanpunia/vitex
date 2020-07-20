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
		namespace Filters
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
			void FilterContext::Initialize()
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

			LowpassFilter::LowpassFilter()
			{
#ifdef THAWK_HAS_OPENAL
				CreateLocked([this]()
				{
					alFilteri(Filter, AL_FILTER_TYPE, AL_FILTER_LOWPASS);
					return true;
				});
#endif
			}
			LowpassFilter::~LowpassFilter()
			{
			}
			void LowpassFilter::Synchronize()
			{
#ifdef THAWK_HAS_OPENAL
				AudioContext::Lock();
				alFilterf(Filter, AL_LOWPASS_GAIN, Gain);
				alFilterf(Filter, AL_LOWPASS_GAINHF, GainHF);
				AudioContext::Unlock();
#endif
			}
			void LowpassFilter::Deserialize(Rest::Document* Node)
			{
				Engine::NMake::Unpack(Node->Find("gain"), &Gain);
				Engine::NMake::Unpack(Node->Find("gain-hf"), &GainHF);
			}
			void LowpassFilter::Serialize(Rest::Document* Node)
			{
				Engine::NMake::Pack(Node->SetDocument("gain"), Gain);
				Engine::NMake::Pack(Node->SetDocument("gain-hf"), GainHF);
			}
			AudioFilter* LowpassFilter::Copy()
			{
				LowpassFilter* Target = new LowpassFilter();
				Target->Gain = Gain;
				Target->GainHF = GainHF;

				return Target;
			}

			HighpassFilter::HighpassFilter()
			{
#ifdef THAWK_HAS_OPENAL
				CreateLocked([this]()
				{
					alFilteri(Filter, AL_FILTER_TYPE, AL_FILTER_HIGHPASS);
					return true;
				});
#endif
			}
			HighpassFilter::~HighpassFilter()
			{
			}
			void HighpassFilter::Synchronize()
			{
#ifdef THAWK_HAS_OPENAL
				AudioContext::Lock();
				alFilterf(Filter, AL_HIGHPASS_GAIN, Gain);
				alFilterf(Filter, AL_HIGHPASS_GAINLF, GainLF);
				AudioContext::Unlock();
#endif
			}
			void HighpassFilter::Deserialize(Rest::Document* Node)
			{
				Engine::NMake::Unpack(Node->Find("gain"), &Gain);
				Engine::NMake::Unpack(Node->Find("gain-lf"), &GainLF);
			}
			void HighpassFilter::Serialize(Rest::Document* Node)
			{
				Engine::NMake::Pack(Node->SetDocument("gain"), Gain);
				Engine::NMake::Pack(Node->SetDocument("gain-lf"), GainLF);
			}
			AudioFilter* HighpassFilter::Copy()
			{
				HighpassFilter* Target = new HighpassFilter();
				Target->Gain = Gain;
				Target->GainLF = GainLF;

				return Target;
			}

			BandpassFilter::BandpassFilter()
			{
#ifdef THAWK_HAS_OPENAL
				CreateLocked([this]()
				{
					alFilteri(Filter, AL_FILTER_TYPE, AL_FILTER_BANDPASS);
					return true;
				});
#endif
			}
			BandpassFilter::~BandpassFilter()
			{
			}
			void BandpassFilter::Synchronize()
			{
#ifdef THAWK_HAS_OPENAL
				AudioContext::Lock();
				alFilterf(Filter, AL_BANDPASS_GAIN, Gain);
				alFilterf(Filter, AL_BANDPASS_GAINLF, GainLF);
				alFilterf(Filter, AL_BANDPASS_GAINHF, GainHF);
				AudioContext::Unlock();
#endif
			}
			void BandpassFilter::Deserialize(Rest::Document* Node)
			{
				Engine::NMake::Unpack(Node->Find("gain"), &Gain);
				Engine::NMake::Unpack(Node->Find("gain-lf"), &GainLF);
				Engine::NMake::Unpack(Node->Find("gain-hf"), &GainHF);
			}
			void BandpassFilter::Serialize(Rest::Document* Node)
			{
				Engine::NMake::Pack(Node->SetDocument("gain"), Gain);
				Engine::NMake::Pack(Node->SetDocument("gain-lf"), GainLF);
				Engine::NMake::Pack(Node->SetDocument("gain-hf"), GainHF);
			}
			AudioFilter* BandpassFilter::Copy()
			{
				BandpassFilter* Target = new BandpassFilter();
				Target->Gain = Gain;
				Target->GainLF = GainLF;
				Target->GainHF = GainHF;

				return Target;
			}
		}
	}
}