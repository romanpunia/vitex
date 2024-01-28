#include "audio.h"
#include "../audio/effects.h"
#include "../audio/filters.h"
#ifdef VI_OPENAL
#ifdef VI_AL_AT_OPENAL
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#else
#include <AL/al.h>
#include <AL/alc.h>
#include <AL/efx.h>
#include <AL/efx-presets.h>
#define HAS_EFX
#endif
#endif
#define LOAD_PROC(T, X) ((X) = (T)alGetProcAddress(#X))
#if defined(VI_OPENAL) && defined(HAS_EFX)
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
#define ReturnErrorIf do { auto __error = AudioException(); if (__error.has_error()) return __error; else return Core::Expectation::Met; } while (0)
#define ReturnErrorIfDev(Device) do { auto __error = AudioException(Device); if (__error.has_error()) return __error; else return Core::Expectation::Met; } while (0)

namespace Vitex
{
	namespace Audio
	{
		AudioException::AudioException(void* Device) : AlErrorCode(0), AlcErrorCode(0)
		{
#ifdef VI_OPENAL
			AlErrorCode = alGetError();
			if (AlErrorCode != AL_NO_ERROR)
			{
				auto* Text = alGetString(AlErrorCode);
				Message += "AL:" + Core::ToString(AlErrorCode) + "driver";
				if (Text != nullptr)
				{
					Message += " on ";
					Message += Text;
				}
			}

			if (Device != nullptr)
			{
				AlcErrorCode = alcGetError((ALCdevice*)Device);
				if (AlcErrorCode != ALC_NO_ERROR)
				{
					auto* Text = alcGetString((ALCdevice*)Device, AlcErrorCode);
					Message += Core::Stringify::Text("ALC:%i:0x%" PRIXPTR, AlcErrorCode, Device);
					if (Text != nullptr)
					{
						Message += " on ";
						Message += Text;
					}
				}
			}
#else
			Message = "audio is not supported";
#endif
		}
		const char* AudioException::type() const noexcept
		{
			return "audio_error";
		}
		int AudioException::al_error_code() const noexcept
		{
			return AlErrorCode;
		}
		int AudioException::alc_error_code() const noexcept
		{
			return AlcErrorCode;
		}
		bool AudioException::has_error() const noexcept
		{
#ifdef VI_OPENAL
			return AlErrorCode != AL_NO_ERROR || AlcErrorCode != ALC_NO_ERROR;
#else
			return true;
#endif
		}

		ExpectsAudio<void> AudioContext::Initialize()
		{
			VI_TRACE("[audio] load audio context func addresses");
#if defined(VI_OPENAL) && defined(HAS_EFX)
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
			ReturnErrorIf;
		}
		ExpectsAudio<void> AudioContext::GenerateBuffers(int Count, unsigned int* Buffers)
		{
			VI_TRACE("[audio] generate %i buffers", Count);
#ifdef VI_OPENAL
			alGenBuffers(Count, Buffers);
#endif
			ReturnErrorIf;
		}
		ExpectsAudio<void> AudioContext::SetFilter1I(unsigned int Filter, FilterEx Value, int F1)
		{
#if defined(VI_OPENAL) && defined(HAS_EFX)
			alFilteri(Filter, (uint32_t)Value, F1);
#endif
			ReturnErrorIf;
		}
		ExpectsAudio<void> AudioContext::SetFilter1F(unsigned int Filter, FilterEx Value, float F1)
		{
#if defined(VI_OPENAL) && defined(HAS_EFX)
			alFilterf(Filter, (uint32_t)Value, F1);
#endif
			ReturnErrorIf;
		}
		ExpectsAudio<void> AudioContext::SetEffect1I(unsigned int Filter, EffectEx Value, int F1)
		{
#if defined(VI_OPENAL) && defined(HAS_EFX)
			alEffecti(Filter, (uint32_t)Value, F1);
#endif
			ReturnErrorIf;
		}
		ExpectsAudio<void> AudioContext::SetEffect1F(unsigned int Filter, EffectEx Value, float F1)
		{
#if defined(VI_OPENAL) && defined(HAS_EFX)
			alEffectf(Filter, (uint32_t)Value, F1);
#endif
			ReturnErrorIf;
		}
		ExpectsAudio<void> AudioContext::SetEffectVF(unsigned int Filter, EffectEx Value, float* FS)
		{
#if defined(VI_OPENAL) && defined(HAS_EFX)
			alEffectfv(Filter, (uint32_t)Value, FS);
#endif
			ReturnErrorIf;
		}
		ExpectsAudio<void> AudioContext::SetBufferData(unsigned int Buffer, int Format, const void* Data, int Size, int Frequency)
		{
#ifdef VI_OPENAL
			alBufferData(Buffer, Format, Data, Size, Frequency);
#endif
			ReturnErrorIf;
		}
		ExpectsAudio<void> AudioContext::SetSourceData3F(unsigned int Source, SoundEx Value, float F1, float F2, float F3)
		{
#ifdef VI_OPENAL
			alSource3f(Source, (uint32_t)Value, F1, F2, F3);
#endif
			ReturnErrorIf;
		}
		ExpectsAudio<void> AudioContext::GetSourceData3F(unsigned int Source, SoundEx Value, float* F1, float* F2, float* F3)
		{
#ifdef VI_OPENAL
			alGetSource3f(Source, (uint32_t)Value, F1, F2, F3);
#endif
			ReturnErrorIf;
		}
		ExpectsAudio<void> AudioContext::SetSourceDataVF(unsigned int Source, SoundEx Value, float* FS)
		{
#ifdef VI_OPENAL
			alSourcefv(Source, (uint32_t)Value, FS);
#endif
			ReturnErrorIf;
		}
		ExpectsAudio<void> AudioContext::GetSourceDataVF(unsigned int Source, SoundEx Value, float* FS)
		{
#ifdef VI_OPENAL
			alGetSourcefv(Source, (uint32_t)Value, FS);
#endif
			ReturnErrorIf;
		}
		ExpectsAudio<void> AudioContext::SetSourceData1F(unsigned int Source, SoundEx Value, float F1)
		{
#ifdef VI_OPENAL
			alSourcef(Source, (uint32_t)Value, F1);
#endif
			ReturnErrorIf;
		}
		ExpectsAudio<void> AudioContext::GetSourceData1F(unsigned int Source, SoundEx Value, float* F1)
		{
#ifdef VI_OPENAL
			alGetSourcef(Source, (uint32_t)Value, F1);
#endif
			ReturnErrorIf;
		}
		ExpectsAudio<void> AudioContext::SetSourceData3I(unsigned int Source, SoundEx Value, int F1, int F2, int F3)
		{
#ifdef VI_OPENAL
			alSource3i(Source, (uint32_t)Value, F1, F2, F3);
#endif
			ReturnErrorIf;
		}
		ExpectsAudio<void> AudioContext::GetSourceData3I(unsigned int Source, SoundEx Value, int* F1, int* F2, int* F3)
		{
#ifdef VI_OPENAL
			alGetSource3i(Source, (uint32_t)Value, F1, F2, F3);
#endif
			ReturnErrorIf;
		}
		ExpectsAudio<void> AudioContext::SetSourceDataVI(unsigned int Source, SoundEx Value, int* FS)
		{
#ifdef VI_OPENAL
			alSourceiv(Source, (uint32_t)Value, FS);
#endif
			ReturnErrorIf;
		}
		ExpectsAudio<void> AudioContext::GetSourceDataVI(unsigned int Source, SoundEx Value, int* FS)
		{
#ifdef VI_OPENAL
			alGetSourceiv(Source, (uint32_t)Value, FS);
#endif
			ReturnErrorIf;
		}
		ExpectsAudio<void> AudioContext::SetSourceData1I(unsigned int Source, SoundEx Value, int F1)
		{
#ifdef VI_OPENAL
			alSourcei(Source, (uint32_t)Value, F1);
#endif
			ReturnErrorIf;
		}
		ExpectsAudio<void> AudioContext::GetSourceData1I(unsigned int Source, SoundEx Value, int* F1)
		{
#ifdef VI_OPENAL
			alGetSourcei(Source, (uint32_t)Value, F1);
#endif
			ReturnErrorIf;
		}
		ExpectsAudio<void> AudioContext::SetListenerData3F(SoundEx Listener, float F1, float F2, float F3)
		{
#ifdef VI_OPENAL
			alListener3f((uint32_t)Listener, F1, F2, F3);
#endif
			ReturnErrorIf;
		}
		ExpectsAudio<void> AudioContext::GetListenerData3F(SoundEx Listener, float* F1, float* F2, float* F3)
		{
#ifdef VI_OPENAL
			alGetListener3f((uint32_t)Listener, F1, F2, F3);
#endif
			ReturnErrorIf;
		}
		ExpectsAudio<void> AudioContext::SetListenerDataVF(SoundEx Listener, float* FS)
		{
#ifdef VI_OPENAL
			alListenerfv((uint32_t)Listener, FS);
#endif
			ReturnErrorIf;
		}
		ExpectsAudio<void> AudioContext::GetListenerDataVF(SoundEx Listener, float* FS)
		{
#ifdef VI_OPENAL
			alGetListenerfv((uint32_t)Listener, FS);
#endif
			ReturnErrorIf;
		}
		ExpectsAudio<void> AudioContext::SetListenerData1F(SoundEx Listener, float F1)
		{
#ifdef VI_OPENAL
			alListenerf((uint32_t)Listener, F1);
#endif
			ReturnErrorIf;
		}
		ExpectsAudio<void> AudioContext::GetListenerData1F(SoundEx Listener, float* F1)
		{
#ifdef VI_OPENAL
			alGetListenerf((uint32_t)Listener, F1);
#endif
			ReturnErrorIf;
		}
		ExpectsAudio<void> AudioContext::SetListenerData3I(SoundEx Listener, int F1, int F2, int F3)
		{
#ifdef VI_OPENAL
			alListener3i((uint32_t)Listener, F1, F2, F3);
#endif
			ReturnErrorIf;
		}
		ExpectsAudio<void> AudioContext::GetListenerData3I(SoundEx Listener, int* F1, int* F2, int* F3)
		{
#ifdef VI_OPENAL
			alGetListener3i((uint32_t)Listener, F1, F2, F3);
#endif
			ReturnErrorIf;
		}
		ExpectsAudio<void> AudioContext::SetListenerDataVI(SoundEx Listener, int* FS)
		{
#ifdef VI_OPENAL
			alListeneriv((uint32_t)Listener, FS);
#endif
			ReturnErrorIf;
		}
		ExpectsAudio<void> AudioContext::GetListenerDataVI(SoundEx Listener, int* FS)
		{
#ifdef VI_OPENAL
			alGetListeneriv((uint32_t)Listener, FS);
#endif
			ReturnErrorIf;
		}
		ExpectsAudio<void> AudioContext::SetListenerData1I(SoundEx Listener, int F1)
		{
#ifdef VI_OPENAL
			alListeneri((uint32_t)Listener, F1);
#endif
			ReturnErrorIf;
		}
		ExpectsAudio<void> AudioContext::GetListenerData1I(SoundEx Listener, int* F1)
		{
#ifdef VI_OPENAL
			alGetListeneri((uint32_t)Listener, F1);
#endif
			ReturnErrorIf;
		}
		uint32_t AudioContext::GetEnumValue(const char* Name)
		{
#ifdef VI_OPENAL
			VI_ASSERT(Name != nullptr, "name should be set");
			return (uint32_t)alGetEnumValue(Name);
#else
			return 0;
#endif
		}

		AudioFilter::AudioFilter() noexcept
		{
#if defined(VI_OPENAL) && defined(HAS_EFX)
			Filter = AL_FILTER_NULL;
#endif
		}
		AudioFilter::~AudioFilter() noexcept
		{
#if defined(VI_OPENAL) && defined(HAS_EFX)
			VI_TRACE("[audio] delete %i filter", (int)Filter);
			if (alDeleteFilters != nullptr && Filter != AL_FILTER_NULL)
				alDeleteFilters(1, &Filter);
#endif
		}
		ExpectsAudio<void> AudioFilter::Initialize(const std::function<bool()>& Callback)
		{
#if defined(VI_OPENAL) && defined(HAS_EFX)
			if (alDeleteFilters != nullptr && Filter != AL_FILTER_NULL)
				alDeleteFilters(1, &Filter);

			if (alGenFilters != nullptr)
				alGenFilters(1, &Filter);

			if (Callback)
				Callback();
			VI_TRACE("[audio] generate %i filter", (int)Filter);
#endif
			ReturnErrorIf;
		}
		AudioSource* AudioFilter::GetSource() const
		{
			return Source;
		}

		AudioEffect::AudioEffect() noexcept
		{
#if defined(VI_OPENAL) && defined(HAS_EFX)
			Effect = AL_EFFECT_NULL;
			Slot = AL_EFFECTSLOT_NULL;
#endif
		}
		AudioEffect::~AudioEffect() noexcept
		{
			Unbind();
#if defined(VI_OPENAL) && defined(HAS_EFX)
			VI_TRACE("[audio] delete %i effect", (int)Effect);
			if (alDeleteEffects != nullptr && Effect != AL_EFFECT_NULL)
				alDeleteEffects(1, &Effect);

			if (alDeleteAuxiliaryEffectSlots != nullptr && Slot != AL_EFFECTSLOT_NULL)
				alDeleteAuxiliaryEffectSlots(1, &Slot);
#endif
			VI_RELEASE(Filter);
		}
		ExpectsAudio<void> AudioEffect::Initialize(const std::function<bool()>& Callback)
		{
#if defined(VI_OPENAL) && defined(HAS_EFX)
			if (alDeleteAuxiliaryEffectSlots != nullptr && Slot != AL_EFFECTSLOT_NULL)
				alDeleteAuxiliaryEffectSlots(1, &Slot);

			if (alGenAuxiliaryEffectSlots != nullptr)
				alGenAuxiliaryEffectSlots(1, &Slot);

			if (alDeleteEffects != nullptr && Effect != AL_EFFECT_NULL)
				alDeleteEffects(1, &Effect);

			if (alGenEffects != nullptr)
				alGenEffects(1, &Effect);

			if (Callback)
				Callback();

			if (alAuxiliaryEffectSloti != nullptr && Effect != AL_EFFECT_NULL)
				alAuxiliaryEffectSloti(Slot, AL_EFFECTSLOT_EFFECT, (ALint)Effect);
			VI_TRACE("[audio] generate %i effect", (int)Effect);
#endif
			ReturnErrorIf;
		}
		ExpectsAudio<void> AudioEffect::SetFilter(AudioFilter** NewFilter)
		{
			VI_CLEAR(Filter);
			if (!NewFilter || !*NewFilter)
				return Core::Expectation::Met;

			Filter = *NewFilter;
			*NewFilter = nullptr;
			return Bind(Source, Zone);
		}
		ExpectsAudio<void> AudioEffect::Bind(AudioSource* NewSource, int NewZone)
		{
			VI_ASSERT(Source != nullptr, "source should not be empty");
			Source = NewSource;
			Zone = NewZone;
#if defined(VI_OPENAL) && defined(HAS_EFX)
			alSource3i(Source->GetInstance(), AL_AUXILIARY_SEND_FILTER, (ALint)Slot, Zone, (ALint)(Filter ? Filter->Filter : AL_FILTER_NULL));
#endif
			ReturnErrorIf;
		}
		ExpectsAudio<void> AudioEffect::Unbind()
		{
			VI_ASSERT(Source != nullptr, "source should not be empty");
#if defined(VI_OPENAL) && defined(HAS_EFX)
			alSource3i(Source->GetInstance(), AL_AUXILIARY_SEND_FILTER, AL_EFFECTSLOT_NULL, Zone, AL_FILTER_NULL);
#endif
			ReturnErrorIf;
		}
		AudioFilter* AudioEffect::GetFilter() const
		{
			return Filter;
		}
		AudioSource* AudioEffect::GetSource() const
		{
			return Source;
		}

		AudioClip::AudioClip(int BufferCount, int NewFormat) noexcept : Format(NewFormat)
		{
			if (BufferCount > 0)
				AudioContext::GenerateBuffers(BufferCount, &Buffer);
		}
		AudioClip::~AudioClip() noexcept
		{
#ifdef VI_OPENAL
			VI_TRACE("[audio] delete %i buffer", (int)Buffer);
			alDeleteBuffers(1, &Buffer);
			Buffer = 0;
#endif
		}
		float AudioClip::Length() const
		{
#ifdef VI_OPENAL
			int ByteSize = 0, ChannelCount = 0, Bits = 0, Frequency = 0;
			alGetBufferi(Buffer, AL_SIZE, &ByteSize);
			alGetBufferi(Buffer, AL_CHANNELS, &ChannelCount);
			alGetBufferi(Buffer, AL_BITS, &Bits);
			alGetBufferi(Buffer, AL_FREQUENCY, &Frequency);

			if (ByteSize == 0 || ChannelCount == 0 || Bits == 0 || Frequency == 0)
				return 0;

			return (float)(ByteSize * 8 / (ChannelCount * Bits)) / (float)Frequency;
#else
			return 0.0f;
#endif
		}
		bool AudioClip::IsMono() const
		{
#ifdef VI_OPENAL
			if (Format == AL_FORMAT_MONO8 || Format == AL_FORMAT_MONO16)
				return true;
#endif
			return false;
		}
		unsigned int AudioClip::GetBuffer() const
		{
			return Buffer;
		}
		int AudioClip::GetFormat() const
		{
			return Format;
		}

		AudioSource::AudioSource() noexcept
		{
#ifdef VI_OPENAL
			if (alIsSource(Instance))
			{
				alSourceStop(Instance);
				alDeleteSources(1, &Instance);
			}

			alGenSources(1, &Instance);
			alSource3f(Instance, AL_DIRECTION, 0, 0, 0);
			alSourcei(Instance, AL_SOURCE_RELATIVE, 0);
			alSourcei(Instance, AL_LOOPING, 0);
			alSourcef(Instance, AL_PITCH, 1);
			alSourcef(Instance, AL_GAIN, 1);
			alSourcef(Instance, AL_MAX_DISTANCE, 100);
			alSourcef(Instance, AL_REFERENCE_DISTANCE, 0.25f);
			alSourcef(Instance, AL_ROLLOFF_FACTOR, 1);
			alSourcef(Instance, AL_CONE_INNER_ANGLE, 360);
			alSourcef(Instance, AL_CONE_OUTER_ANGLE, 360);
			alSourcef(Instance, AL_CONE_OUTER_GAIN, 0);
			alSource3f(Instance, AL_POSITION, 0, 0, 0);
			alSourcei(Instance, AL_SEC_OFFSET, 0);
			VI_TRACE("[audio] generate %i source", (int)Instance);
#endif
		}
		AudioSource::~AudioSource() noexcept
		{
			RemoveEffects();
			VI_RELEASE(Clip);
#ifdef VI_OPENAL
			VI_TRACE("[audio] delete %i source", (int)Instance);
			alSourceStop(Instance);
			alSourcei(Instance, AL_BUFFER, 0);
			alDeleteSources(1, &Instance);
#endif
		}
		int64_t AudioSource::AddEffect(AudioEffect* Effect)
		{
			VI_ASSERT(Effect != nullptr, "effect should be set");
			Effect->Bind(this, (int)Effects.size());
			Effects.push_back(Effect);
			return Effects.size() - 1;
		}
		ExpectsAudio<void> AudioSource::RemoveEffect(size_t EffectId)
		{
			VI_ASSERT(EffectId < Effects.size(), "index outside of range");
			auto It = Effects.begin() + EffectId;
			VI_RELEASE(*It);
			Effects.erase(It);

			for (size_t i = EffectId; i < Effects.size(); i++)
			{
				AudioEffect* Effect = Effects[i];
				Effect->Unbind();
				Effect->Bind(this, (int)i);
			}

			ReturnErrorIf;
		}
		ExpectsAudio<void> AudioSource::RemoveEffectById(uint64_t EffectId)
		{
			for (size_t i = 0; i < Effects.size(); i++)
			{
				if (Effects[i]->GetId() == EffectId)
					return RemoveEffect(i);
			}

			ReturnErrorIf;
		}
		ExpectsAudio<void> AudioSource::RemoveEffects()
		{
			for (auto* Effect : Effects)
				VI_RELEASE(Effect);

			Effects.clear();
			ReturnErrorIf;
		}
		ExpectsAudio<void> AudioSource::SetClip(AudioClip* NewClip)
		{
#ifdef VI_OPENAL
			VI_TRACE("[audio] set clip %i on %i source", NewClip ? (int)NewClip->GetBuffer() : 0, (int)Instance);
			alSourceStop(Instance);

			VI_RELEASE(Clip);
			Clip = NewClip;
			if (Clip != nullptr)
			{
				alSourcei(Instance, AL_BUFFER, Clip->GetBuffer());
				Clip->AddRef();
			}
#endif
			ReturnErrorIf;
		}
		ExpectsAudio<void> AudioSource::Synchronize(AudioSync* Sync, const Compute::Vector3& Position)
		{
			VI_ASSERT(Sync != nullptr, "sync should be set");
			for (auto* Effect : Effects)
			{
				if (!Effect)
					continue;

				Effect->Synchronize();
				if (Effect->Filter != nullptr)
					Effect->Filter->Synchronize();
#if defined(VI_OPENAL) && defined(HAS_EFX)
				if (alAuxiliaryEffectSloti != nullptr && Effect->Effect != AL_EFFECT_NULL && Effect->Slot != AL_EFFECTSLOT_NULL)
					alAuxiliaryEffectSloti(Effect->Slot, AL_EFFECTSLOT_EFFECT, (ALint)Effect->Effect);
#endif
			}
#ifdef VI_OPENAL
			if (!Sync->IsRelative)
				alSource3f(Instance, AL_POSITION, 0, 0, 0);
			else
				alSource3f(Instance, AL_POSITION, -Position.X, -Position.Y, Position.Z);
#ifdef AL_ROOM_ROLLOFF_FACTOR
			alSourcef(Instance, AL_ROOM_ROLLOFF_FACTOR, Sync->RoomRollOff);
#endif
#ifdef AL_AIR_ABSORPTION_FACTOR
			alSourcef(Instance, AL_AIR_ABSORPTION_FACTOR, Sync->AirAbsorption);
#endif
			alSource3f(Instance, AL_VELOCITY, Sync->Velocity.X, Sync->Velocity.Y, Sync->Velocity.Z);
			alSource3f(Instance, AL_DIRECTION, Sync->Direction.X, Sync->Direction.Y, Sync->Direction.Z);
			alSourcei(Instance, AL_SOURCE_RELATIVE, Sync->IsRelative ? 0 : 1);
			alSourcei(Instance, AL_LOOPING, Sync->IsLooped ? 1 : 0);
			alSourcef(Instance, AL_PITCH, Sync->Pitch);
			alSourcef(Instance, AL_GAIN, Sync->Gain);
			alSourcef(Instance, AL_MAX_DISTANCE, Sync->Distance);
			alSourcef(Instance, AL_REFERENCE_DISTANCE, Sync->RefDistance);
			alSourcef(Instance, AL_ROLLOFF_FACTOR, Sync->Rolloff);
			alSourcef(Instance, AL_CONE_INNER_ANGLE, Sync->ConeInnerAngle);
			alSourcef(Instance, AL_CONE_OUTER_ANGLE, Sync->ConeOuterAngle);
			alSourcef(Instance, AL_CONE_OUTER_GAIN, Sync->ConeOuterGain);
			alGetSourcef(Instance, AL_SEC_OFFSET, &Sync->Position);
#endif
			ReturnErrorIf;
		}
		ExpectsAudio<void> AudioSource::Reset()
		{
#ifdef VI_OPENAL
			VI_TRACE("[audio] reset on %i source", (int)Instance);
			alSource3f(Instance, AL_DIRECTION, 0, 0, 0);
			alSourcei(Instance, AL_SOURCE_RELATIVE, 0);
			alSourcei(Instance, AL_LOOPING, 0);
			alSourcef(Instance, AL_PITCH, 1);
			alSourcef(Instance, AL_GAIN, 1);
			alSourcef(Instance, AL_MAX_DISTANCE, 100);
			alSourcef(Instance, AL_REFERENCE_DISTANCE, 0.25f);
			alSourcef(Instance, AL_ROLLOFF_FACTOR, 1);
			alSourcef(Instance, AL_CONE_INNER_ANGLE, 360);
			alSourcef(Instance, AL_CONE_OUTER_ANGLE, 360);
			alSourcef(Instance, AL_CONE_OUTER_GAIN, 0);
			alSource3f(Instance, AL_POSITION, 0, 0, 0);
			alSourcei(Instance, AL_SEC_OFFSET, 0);
#endif
			ReturnErrorIf;
		}
		ExpectsAudio<void> AudioSource::Pause()
		{
#ifdef VI_OPENAL
			VI_TRACE("[audio] pause on %i source", (int)Instance);
			alSourcePause(Instance);
#endif
			ReturnErrorIf;
		}
		ExpectsAudio<void> AudioSource::Play()
		{
#ifdef VI_OPENAL
			VI_TRACE("[audio] play on %i source", (int)Instance);
			alSourcePlay(Instance);
#endif
			ReturnErrorIf;
		}
		ExpectsAudio<void> AudioSource::Stop()
		{
#ifdef VI_OPENAL
			VI_TRACE("[audio] stop on %i source", (int)Instance);
			alSourceStop(Instance);
#endif
			ReturnErrorIf;
		}
		bool AudioSource::IsPlaying() const
		{
#ifdef VI_OPENAL
			int State = 0;
			alGetSourcei(Instance, AL_SOURCE_STATE, &State);
			return State == AL_PLAYING;
#else
			return false;
#endif
		}
		size_t AudioSource::GetEffectsCount() const
		{
			return Effects.size();
		}
		AudioClip* AudioSource::GetClip() const
		{
			return Clip;
		}
		AudioEffect* AudioSource::GetEffect(uint64_t Section) const
		{
			for (auto* Effect : Effects)
			{
				if (Effect && Effect->GetId() == Section)
					return Effect;
			}

			return nullptr;
		}
		unsigned int AudioSource::GetInstance() const
		{
			return Instance;
		}
		const Core::Vector<AudioEffect*>& AudioSource::GetEffects() const
		{
			return Effects;
		}

		AudioDevice::AudioDevice() noexcept
		{
#ifdef VI_OPENAL
			Device = (void*)alcOpenDevice(nullptr);
			VI_TRACE("[audio] open alc device: 0x%" PRIXPTR, (void*)Device);
			VI_PANIC(Device != nullptr, "audio device cannot be created [ %s ]", alGetString(alGetError()));

			Context = (void*)alcCreateContext((ALCdevice*)Device, nullptr);
			VI_TRACE("[audio] create alc context: 0x%" PRIXPTR, (void*)Context);
			VI_PANIC(Context != nullptr, "audio context cannot be created [ %s ]", alcGetString((ALCdevice*)Device, alcGetError((ALCdevice*)Device)));

			alcMakeContextCurrent((ALCcontext*)Context);
			alDistanceModel(AL_LINEAR_DISTANCE);
			alListenerf(AL_GAIN, 0.0f);
#endif
		}
		AudioDevice::~AudioDevice() noexcept
		{
#ifdef VI_OPENAL
			if (Context != nullptr)
			{
				VI_TRACE("[audio] delete alc context: 0x%" PRIXPTR, (void*)Context);
				alcMakeContextCurrent(nullptr);
				alcDestroyContext((ALCcontext*)Context);
				Context = nullptr;
			}

			if (Device != nullptr)
			{
				VI_TRACE("[audio] close alc context: 0x%" PRIXPTR, (void*)Device);
				alcCloseDevice((ALCdevice*)Device);
				Device = nullptr;
			}
#endif
		}
		ExpectsAudio<void> AudioDevice::Offset(AudioSource* Source, float& Seconds, bool Get)
		{
			VI_ASSERT(Source != nullptr, "souce should be set");
#ifdef VI_OPENAL
			if (!Get)
				alSourcef(Source->Instance, AL_SEC_OFFSET, Seconds);
			else
				alGetSourcef(Source->Instance, AL_SEC_OFFSET, &Seconds);
#endif
			ReturnErrorIfDev(Device);
		}
		ExpectsAudio<void> AudioDevice::Relative(AudioSource* Source, int& Value, bool Get)
		{
			VI_ASSERT(Source != nullptr, "souce should be set");
#ifdef VI_OPENAL
			if (!Get)
				alSourcei(Source->Instance, AL_SOURCE_RELATIVE, Value);
			else
				alGetSourcei(Source->Instance, AL_SOURCE_RELATIVE, &Value);
#endif
			ReturnErrorIfDev(Device);
		}
		ExpectsAudio<void> AudioDevice::Position(AudioSource* Source, Compute::Vector3& Position, bool Get)
		{
			VI_ASSERT(Source != nullptr, "souce should be set");
#ifdef VI_OPENAL
			if (!Get)
			{
				alGetSource3f(Source->Instance, AL_POSITION, &Position.X, &Position.Y, &Position.Z);
				Position.X = -Position.X;
			}
			else
				alSource3f(Source->Instance, AL_POSITION, -Position.X, Position.Y, Position.Z);
#endif
			ReturnErrorIfDev(Device);
		}
		ExpectsAudio<void> AudioDevice::Direction(AudioSource* Source, Compute::Vector3& Direction, bool Get)
		{
			VI_ASSERT(Source != nullptr, "souce should be set");
#ifdef VI_OPENAL
			if (!Get)
				alSource3f(Source->Instance, AL_DIRECTION, Direction.X, Direction.Y, Direction.Z);
			else
				alGetSource3f(Source->Instance, AL_DIRECTION, &Direction.X, &Direction.Y, &Direction.Z);
#endif
			ReturnErrorIfDev(Device);
		}
		ExpectsAudio<void> AudioDevice::Velocity(AudioSource* Source, Compute::Vector3& Velocity, bool Get)
		{
			VI_ASSERT(Source != nullptr, "souce should be set");
#ifdef VI_OPENAL
			if (!Get)
				alSource3f(Source->Instance, AL_VELOCITY, Velocity.X, Velocity.Y, Velocity.Z);
			else
				alGetSource3f(Source->Instance, AL_VELOCITY, &Velocity.X, &Velocity.Y, &Velocity.Z);
#endif
			ReturnErrorIfDev(Device);
		}
		ExpectsAudio<void> AudioDevice::Pitch(AudioSource* Source, float& Value, bool Get)
		{
			VI_ASSERT(Source != nullptr, "souce should be set");
#ifdef VI_OPENAL
			if (!Get)
				alSourcef(Source->Instance, AL_PITCH, Value);
			else
				alGetSourcef(Source->Instance, AL_PITCH, &Value);
#endif
			ReturnErrorIfDev(Device);
		}
		ExpectsAudio<void> AudioDevice::Gain(AudioSource* Source, float& Value, bool Get)
		{
			VI_ASSERT(Source != nullptr, "souce should be set");
#ifdef VI_OPENAL
			if (!Get)
				alSourcef(Source->Instance, AL_GAIN, Value);
			else
				alGetSourcef(Source->Instance, AL_GAIN, &Value);
#endif
			ReturnErrorIfDev(Device);
		}
		ExpectsAudio<void> AudioDevice::ConeInnerAngle(AudioSource* Source, float& Value, bool Get)
		{
			VI_ASSERT(Source != nullptr, "souce should be set");
#ifdef VI_OPENAL
			if (!Get)
				alSourcef(Source->Instance, AL_CONE_INNER_ANGLE, Value);
			else
				alGetSourcef(Source->Instance, AL_CONE_INNER_ANGLE, &Value);
#endif
			ReturnErrorIfDev(Device);
		}
		ExpectsAudio<void> AudioDevice::ConeOuterAngle(AudioSource* Source, float& Value, bool Get)
		{
			VI_ASSERT(Source != nullptr, "souce should be set");
#ifdef VI_OPENAL
			if (!Get)
				alSourcef(Source->Instance, AL_CONE_OUTER_ANGLE, Value);
			else
				alGetSourcef(Source->Instance, AL_CONE_OUTER_ANGLE, &Value);
#endif
			ReturnErrorIfDev(Device);
		}
		ExpectsAudio<void> AudioDevice::ConeOuterGain(AudioSource* Source, float& Value, bool Get)
		{
			VI_ASSERT(Source != nullptr, "souce should be set");
#ifdef VI_OPENAL
			if (!Get)
				alSourcef(Source->Instance, AL_CONE_OUTER_GAIN, Value);
			else
				alGetSourcef(Source->Instance, AL_CONE_OUTER_GAIN, &Value);
#endif
			ReturnErrorIfDev(Device);
		}
		ExpectsAudio<void> AudioDevice::Distance(AudioSource* Source, float& Value, bool Get)
		{
			VI_ASSERT(Source != nullptr, "souce should be set");
#ifdef VI_OPENAL
			if (!Get)
				alSourcef(Source->Instance, AL_MAX_DISTANCE, Value);
			else
				alGetSourcef(Source->Instance, AL_MAX_DISTANCE, &Value);
#endif
			ReturnErrorIfDev(Device);
		}
		ExpectsAudio<void> AudioDevice::RefDistance(AudioSource* Source, float& Value, bool Get)
		{
			VI_ASSERT(Source != nullptr, "souce should be set");
#ifdef VI_OPENAL
			if (!Get)
				alSourcef(Source->Instance, AL_REFERENCE_DISTANCE, Value);
			else
				alGetSourcef(Source->Instance, AL_REFERENCE_DISTANCE, &Value);
#endif
			ReturnErrorIfDev(Device);
		}
		ExpectsAudio<void> AudioDevice::Loop(AudioSource* Source, int& IsLoop, bool Get)
		{
			VI_ASSERT(Source != nullptr, "souce should be set");
#ifdef VI_OPENAL
			if (!Get)
				alSourcei(Source->Instance, AL_LOOPING, IsLoop);
			else
				alGetSourcei(Source->Instance, AL_LOOPING, &IsLoop);
#endif
			ReturnErrorIfDev(Device);
		}
		ExpectsAudio<void> AudioDevice::SetDistanceModel(SoundDistanceModel Model)
		{
#ifdef VI_OPENAL
			alDistanceModel((int)Model);
#endif
			ReturnErrorIfDev(Device);
		}
		void AudioDevice::DisplayAudioLog() const
		{
#ifdef VI_OPENAL
			int ALCCode = ALC_NO_ERROR;
			if ((ALCCode = alcGetError((ALCdevice*)Device)) != ALC_NO_ERROR)
				VI_ERR("[openalc] %s", alcGetString((ALCdevice*)Device, ALCCode));

			int ALCode = AL_NO_ERROR;
			if ((ALCode = alGetError()) != AL_NO_ERROR)
				VI_ERR("[openal] %s", alGetString(ALCode));
#endif
		}
		bool AudioDevice::IsValid() const
		{
			return Context && Device;
		}
	}
}
