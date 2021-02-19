#include "filters.h"
#include "../core/engine.h"
#ifdef TH_HAS_OPENAL
#include <AL/al.h>
#include <AL/efx.h>
#endif
#define LOAD_PROC(T, X) ((X) = (T)alGetProcAddress(#X))
#ifdef TH_HAS_OPENAL
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

namespace Tomahawk
{
	namespace Audio
	{
		namespace Filters
		{
			void FilterContext::Create()
			{
#ifdef TH_HAS_OPENAL
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

			Lowpass::Lowpass()
			{
#ifdef TH_HAS_OPENAL
				CreateLocked([this]()
				{
					alFilteri(Filter, AL_FILTER_TYPE, AL_FILTER_LOWPASS);
					return true;
				});
#endif
			}
			Lowpass::~Lowpass()
			{
			}
			void Lowpass::Synchronize()
			{
#ifdef TH_HAS_OPENAL
				AudioContext::Lock();
				alFilterf(Filter, AL_LOWPASS_GAIN, Gain);
				alFilterf(Filter, AL_LOWPASS_GAINHF, GainHF);
				AudioContext::Unlock();
#endif
			}
			void Lowpass::Deserialize(Rest::Document* Node)
			{
				Engine::NMake::Unpack(Node->Find("gain"), &Gain);
				Engine::NMake::Unpack(Node->Find("gain-hf"), &GainHF);
			}
			void Lowpass::Serialize(Rest::Document* Node)
			{
				Engine::NMake::Pack(Node->Set("gain"), Gain);
				Engine::NMake::Pack(Node->Set("gain-hf"), GainHF);
			}
			AudioFilter* Lowpass::Copy()
			{
				Lowpass* Target = new Lowpass();
				Target->Gain = Gain;
				Target->GainHF = GainHF;

				return Target;
			}

			Highpass::Highpass()
			{
#ifdef TH_HAS_OPENAL
				CreateLocked([this]()
				{
					alFilteri(Filter, AL_FILTER_TYPE, AL_FILTER_HIGHPASS);
					return true;
				});
#endif
			}
			Highpass::~Highpass()
			{
			}
			void Highpass::Synchronize()
			{
#ifdef TH_HAS_OPENAL
				AudioContext::Lock();
				alFilterf(Filter, AL_HIGHPASS_GAIN, Gain);
				alFilterf(Filter, AL_HIGHPASS_GAINLF, GainLF);
				AudioContext::Unlock();
#endif
			}
			void Highpass::Deserialize(Rest::Document* Node)
			{
				Engine::NMake::Unpack(Node->Find("gain"), &Gain);
				Engine::NMake::Unpack(Node->Find("gain-lf"), &GainLF);
			}
			void Highpass::Serialize(Rest::Document* Node)
			{
				Engine::NMake::Pack(Node->Set("gain"), Gain);
				Engine::NMake::Pack(Node->Set("gain-lf"), GainLF);
			}
			AudioFilter* Highpass::Copy()
			{
				Highpass* Target = new Highpass();
				Target->Gain = Gain;
				Target->GainLF = GainLF;

				return Target;
			}

			Bandpass::Bandpass()
			{
#ifdef TH_HAS_OPENAL
				CreateLocked([this]()
				{
					alFilteri(Filter, AL_FILTER_TYPE, AL_FILTER_BANDPASS);
					return true;
				});
#endif
			}
			Bandpass::~Bandpass()
			{
			}
			void Bandpass::Synchronize()
			{
#ifdef TH_HAS_OPENAL
				AudioContext::Lock();
				alFilterf(Filter, AL_BANDPASS_GAIN, Gain);
				alFilterf(Filter, AL_BANDPASS_GAINLF, GainLF);
				alFilterf(Filter, AL_BANDPASS_GAINHF, GainHF);
				AudioContext::Unlock();
#endif
			}
			void Bandpass::Deserialize(Rest::Document* Node)
			{
				Engine::NMake::Unpack(Node->Find("gain"), &Gain);
				Engine::NMake::Unpack(Node->Find("gain-lf"), &GainLF);
				Engine::NMake::Unpack(Node->Find("gain-hf"), &GainHF);
			}
			void Bandpass::Serialize(Rest::Document* Node)
			{
				Engine::NMake::Pack(Node->Set("gain"), Gain);
				Engine::NMake::Pack(Node->Set("gain-lf"), GainLF);
				Engine::NMake::Pack(Node->Set("gain-hf"), GainHF);
			}
			AudioFilter* Bandpass::Copy()
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