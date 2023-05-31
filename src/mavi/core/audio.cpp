#include "audio.h"
#include "../audio/effects.h"
#include "../audio/filters.h"
#ifdef VI_HAS_OPENAL
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
#if defined(VI_HAS_OPENAL) && defined(HAS_EFX)
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
namespace Mavi
{
	namespace Audio
	{
		void AudioContext::Initialize()
		{
			VI_TRACE("[audio] load audio context func addresses");
#if defined(VI_HAS_OPENAL) && defined(HAS_EFX)
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
			Effects::EffectContext::Initialize();
			Filters::FilterContext::Initialize();
		}
		void AudioContext::GenerateBuffers(int Count, unsigned int* Buffers)
		{
			VI_TRACE("[audio] generate %i buffers", Count);
#ifdef VI_HAS_OPENAL
			alGenBuffers(Count, Buffers);
#endif
		}
		void AudioContext::SetBufferData(unsigned int Buffer, int Format, const void* Data, int Size, int Frequency)
		{
#ifdef VI_HAS_OPENAL
			alBufferData(Buffer, Format, Data, Size, Frequency);
#endif
		}
		void AudioContext::SetSourceData3F(unsigned int Source, SoundEx Value, float F1, float F2, float F3)
		{
#ifdef VI_HAS_OPENAL
			alSource3f(Source, (uint32_t)Value, F1, F2, F3);
#endif
		}
		void AudioContext::GetSourceData3F(unsigned int Source, SoundEx Value, float* F1, float* F2, float* F3)
		{
#ifdef VI_HAS_OPENAL
			alGetSource3f(Source, (uint32_t)Value, F1, F2, F3);
#endif
		}
		void AudioContext::SetSourceDataVF(unsigned int Source, SoundEx Value, float* FS)
		{
#ifdef VI_HAS_OPENAL
			alSourcefv(Source, (uint32_t)Value, FS);
#endif
		}
		void AudioContext::GetSourceDataVF(unsigned int Source, SoundEx Value, float* FS)
		{
#ifdef VI_HAS_OPENAL
			alGetSourcefv(Source, (uint32_t)Value, FS);
#endif
		}
		void AudioContext::SetSourceData1F(unsigned int Source, SoundEx Value, float F1)
		{
#ifdef VI_HAS_OPENAL
			alSourcef(Source, (uint32_t)Value, F1);
#endif
		}
		void AudioContext::GetSourceData1F(unsigned int Source, SoundEx Value, float* F1)
		{
#ifdef VI_HAS_OPENAL
			alGetSourcef(Source, (uint32_t)Value, F1);
#endif
		}
		void AudioContext::SetSourceData3I(unsigned int Source, SoundEx Value, int F1, int F2, int F3)
		{
#ifdef VI_HAS_OPENAL
			alSource3i(Source, (uint32_t)Value, F1, F2, F3);
#endif
		}
		void AudioContext::GetSourceData3I(unsigned int Source, SoundEx Value, int* F1, int* F2, int* F3)
		{
#ifdef VI_HAS_OPENAL
			alGetSource3i(Source, (uint32_t)Value, F1, F2, F3);
#endif
		}
		void AudioContext::SetSourceDataVI(unsigned int Source, SoundEx Value, int* FS)
		{
#ifdef VI_HAS_OPENAL
			alSourceiv(Source, (uint32_t)Value, FS);
#endif
		}
		void AudioContext::GetSourceDataVI(unsigned int Source, SoundEx Value, int* FS)
		{
#ifdef VI_HAS_OPENAL
			alGetSourceiv(Source, (uint32_t)Value, FS);
#endif
		}
		void AudioContext::SetSourceData1I(unsigned int Source, SoundEx Value, int F1)
		{
#ifdef VI_HAS_OPENAL
			alSourcei(Source, (uint32_t)Value, F1);
#endif
		}
		void AudioContext::GetSourceData1I(unsigned int Source, SoundEx Value, int* F1)
		{
#ifdef VI_HAS_OPENAL
			alGetSourcei(Source, (uint32_t)Value, F1);
#endif
		}
		void AudioContext::SetListenerData3F(SoundEx Listener, float F1, float F2, float F3)
		{
#ifdef VI_HAS_OPENAL
			alListener3f((uint32_t)Listener, F1, F2, F3);
#endif
		}
		void AudioContext::GetListenerData3F(SoundEx Listener, float* F1, float* F2, float* F3)
		{
#ifdef VI_HAS_OPENAL
			alGetListener3f((uint32_t)Listener, F1, F2, F3);
#endif
		}
		void AudioContext::SetListenerDataVF(SoundEx Listener, float* FS)
		{
#ifdef VI_HAS_OPENAL
			alListenerfv((uint32_t)Listener, FS);
#endif
		}
		void AudioContext::GetListenerDataVF(SoundEx Listener, float* FS)
		{
#ifdef VI_HAS_OPENAL
			alGetListenerfv((uint32_t)Listener, FS);
#endif
		}
		void AudioContext::SetListenerData1F(SoundEx Listener, float F1)
		{
#ifdef VI_HAS_OPENAL
			alListenerf((uint32_t)Listener, F1);
#endif
		}
		void AudioContext::GetListenerData1F(SoundEx Listener, float* F1)
		{
#ifdef VI_HAS_OPENAL
			alGetListenerf((uint32_t)Listener, F1);
#endif
		}
		void AudioContext::SetListenerData3I(SoundEx Listener, int F1, int F2, int F3)
		{
#ifdef VI_HAS_OPENAL
			alListener3i((uint32_t)Listener, F1, F2, F3);
#endif
		}
		void AudioContext::GetListenerData3I(SoundEx Listener, int* F1, int* F2, int* F3)
		{
#ifdef VI_HAS_OPENAL
			alGetListener3i((uint32_t)Listener, F1, F2, F3);
#endif
		}
		void AudioContext::SetListenerDataVI(SoundEx Listener, int* FS)
		{
#ifdef VI_HAS_OPENAL
			alListeneriv((uint32_t)Listener, FS);
#endif
		}
		void AudioContext::GetListenerDataVI(SoundEx Listener, int* FS)
		{
#ifdef VI_HAS_OPENAL
			alGetListeneriv((uint32_t)Listener, FS);
#endif
		}
		void AudioContext::SetListenerData1I(SoundEx Listener, int F1)
		{
#ifdef VI_HAS_OPENAL
			alListeneri((uint32_t)Listener, F1);
#endif
		}
		void AudioContext::GetListenerData1I(SoundEx Listener, int* F1)
		{
#ifdef VI_HAS_OPENAL
			alGetListeneri((uint32_t)Listener, F1);
#endif
		}

		AudioFilter::AudioFilter() noexcept
		{
#if defined(VI_HAS_OPENAL) && defined(HAS_EFX)
			Filter = AL_FILTER_NULL;
#endif
		}
		AudioFilter::~AudioFilter() noexcept
		{
#if defined(VI_HAS_OPENAL) && defined(HAS_EFX)
			VI_TRACE("[audio] delete %i filter", (int)Filter);
			if (alDeleteFilters != nullptr && Filter != AL_FILTER_NULL)
				alDeleteFilters(1, &Filter);
#endif
		}
		bool AudioFilter::CreateLocked(const std::function<bool()>& Callback)
		{
#if defined(VI_HAS_OPENAL) && defined(HAS_EFX)
			if (alDeleteFilters != nullptr && Filter != AL_FILTER_NULL)
				alDeleteFilters(1, &Filter);

			if (alGenFilters != nullptr)
				alGenFilters(1, &Filter);

			if (Callback)
				Callback();
			VI_TRACE("[audio] generate %i filter", (int)Filter);
#endif
			return true;
		}
		AudioSource* AudioFilter::GetSource() const
		{
			return Source;
		}

		AudioEffect::AudioEffect() noexcept
		{
#if defined(VI_HAS_OPENAL) && defined(HAS_EFX)
			Effect = AL_EFFECT_NULL;
			Slot = AL_EFFECTSLOT_NULL;
#endif
		}
		AudioEffect::~AudioEffect() noexcept
		{
			Unbind();
#if defined(VI_HAS_OPENAL) && defined(HAS_EFX)
			VI_TRACE("[audio] delete %i effect", (int)Effect);
			if (alDeleteEffects != nullptr && Effect != AL_EFFECT_NULL)
				alDeleteEffects(1, &Effect);

			if (alDeleteAuxiliaryEffectSlots != nullptr && Slot != AL_EFFECTSLOT_NULL)
				alDeleteAuxiliaryEffectSlots(1, &Slot);
#endif
			VI_RELEASE(Filter);
		}
		bool AudioEffect::CreateLocked(const std::function<bool()>& Callback)
		{
#if defined(VI_HAS_OPENAL) && defined(HAS_EFX)
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
			return true;
		}
		bool AudioEffect::SetFilter(AudioFilter** NewFilter)
		{
			VI_CLEAR(Filter);
			if (!NewFilter || !*NewFilter)
				return true;

			Filter = *NewFilter;
			*NewFilter = nullptr;
			Bind(Source, Zone);

			return true;
		}
		bool AudioEffect::Bind(AudioSource* NewSource, int NewZone)
		{
			VI_ASSERT(Source != nullptr, "source should not be empty");
			Source = NewSource;
			Zone = NewZone;
#if defined(VI_HAS_OPENAL) && defined(HAS_EFX)
			alSource3i(Source->GetInstance(), AL_AUXILIARY_SEND_FILTER, (ALint)Slot, Zone, (ALint)(Filter ? Filter->Filter : AL_FILTER_NULL));
#endif
			return true;
		}
		bool AudioEffect::Unbind()
		{
			VI_ASSERT(Source != nullptr, "source should not be empty");
#if defined(VI_HAS_OPENAL) && defined(HAS_EFX)
			alSource3i(Source->GetInstance(), AL_AUXILIARY_SEND_FILTER, AL_EFFECTSLOT_NULL, Zone, AL_FILTER_NULL);
#endif
			return true;
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
#ifdef VI_HAS_OPENAL
			VI_TRACE("[audio] delete %i buffer", (int)Buffer);
			alDeleteBuffers(1, &Buffer);
			Buffer = 0;
#endif
		}
		float AudioClip::Length() const
		{
#ifdef VI_HAS_OPENAL
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
#ifdef VI_HAS_OPENAL
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
#ifdef VI_HAS_OPENAL
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
#ifdef VI_HAS_OPENAL
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
		bool AudioSource::RemoveEffect(size_t EffectId)
		{
			VI_ASSERT(EffectId < Effects.size(), "index outside of range");
			auto It = Effects.begin() + EffectId;
			VI_RELEASE(*It);
			Effects.erase(It);

			for (size_t i = EffectId; i < Effects.size(); i++)
			{
				AudioEffect* Effect = Effects[i];
				if (!Effect)
					continue;

				Effect->Unbind();
				Effect->Bind(this, (int)i);
			}

			return true;
		}
		bool AudioSource::RemoveEffectById(uint64_t EffectId)
		{
			for (size_t i = 0; i < Effects.size(); i++)
			{
				if (Effects[i]->GetId() == EffectId)
					return RemoveEffect(i);
			}

			return false;
		}
		void AudioSource::RemoveEffects()
		{
			for (auto* Effect : Effects)
				VI_RELEASE(Effect);
			Effects.clear();
		}
		void AudioSource::SetClip(AudioClip* NewClip)
		{
#ifdef VI_HAS_OPENAL
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
		}
		void AudioSource::Synchronize(AudioSync* Sync, const Compute::Vector3& Position)
		{
			VI_ASSERT(Sync != nullptr, "sync should be set");
			for (auto* Effect : Effects)
			{
				if (!Effect)
					continue;

				Effect->Synchronize();
				if (Effect->Filter != nullptr)
					Effect->Filter->Synchronize();
#if defined(VI_HAS_OPENAL) && defined(HAS_EFX)
				if (alAuxiliaryEffectSloti != nullptr && Effect->Effect != AL_EFFECT_NULL && Effect->Slot != AL_EFFECTSLOT_NULL)
					alAuxiliaryEffectSloti(Effect->Slot, AL_EFFECTSLOT_EFFECT, (ALint)Effect->Effect);
#endif
			}
#ifdef VI_HAS_OPENAL
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
		}
		void AudioSource::Reset()
		{
#ifdef VI_HAS_OPENAL
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
		}
		void AudioSource::Pause()
		{
#ifdef VI_HAS_OPENAL
			VI_TRACE("[audio] pause on %i source", (int)Instance);
			alSourcePause(Instance);
#endif
		}
		void AudioSource::Play()
		{
#ifdef VI_HAS_OPENAL
			VI_TRACE("[audio] play on %i source", (int)Instance);
			alSourcePlay(Instance);
#endif
		}
		void AudioSource::Stop()
		{
#ifdef VI_HAS_OPENAL
			VI_TRACE("[audio] stop on %i source", (int)Instance);
			alSourceStop(Instance);
#endif
		}
		bool AudioSource::IsPlaying() const
		{
#ifdef VI_HAS_OPENAL
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
#ifdef VI_HAS_OPENAL
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
#ifdef VI_HAS_OPENAL
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
		void AudioDevice::Offset(AudioSource* Source, float& Seconds, bool Get)
		{
			VI_ASSERT(Source != nullptr, "souce should be set");
#ifdef VI_HAS_OPENAL
			if (!Get)
				alSourcef(Source->Instance, AL_SEC_OFFSET, Seconds);
			else
				alGetSourcef(Source->Instance, AL_SEC_OFFSET, &Seconds);
#endif
		}
		void AudioDevice::Relative(AudioSource* Source, int& Value, bool Get)
		{
			VI_ASSERT(Source != nullptr, "souce should be set");
#ifdef VI_HAS_OPENAL
			if (!Get)
				alSourcei(Source->Instance, AL_SOURCE_RELATIVE, Value);
			else
				alGetSourcei(Source->Instance, AL_SOURCE_RELATIVE, &Value);
#endif
		}
		void AudioDevice::Position(AudioSource* Source, Compute::Vector3& Position, bool Get)
		{
			VI_ASSERT(Source != nullptr, "souce should be set");
#ifdef VI_HAS_OPENAL
			if (!Get)
			{
				alGetSource3f(Source->Instance, AL_POSITION, &Position.X, &Position.Y, &Position.Z);
				Position.X = -Position.X;
			}
			else
				alSource3f(Source->Instance, AL_POSITION, -Position.X, Position.Y, Position.Z);
#endif
		}
		void AudioDevice::Direction(AudioSource* Source, Compute::Vector3& Direction, bool Get)
		{
			VI_ASSERT(Source != nullptr, "souce should be set");
#ifdef VI_HAS_OPENAL
			if (!Get)
				alSource3f(Source->Instance, AL_DIRECTION, Direction.X, Direction.Y, Direction.Z);
			else
				alGetSource3f(Source->Instance, AL_DIRECTION, &Direction.X, &Direction.Y, &Direction.Z);
#endif
		}
		void AudioDevice::Velocity(AudioSource* Source, Compute::Vector3& Velocity, bool Get)
		{
			VI_ASSERT(Source != nullptr, "souce should be set");
#ifdef VI_HAS_OPENAL
			if (!Get)
				alSource3f(Source->Instance, AL_VELOCITY, Velocity.X, Velocity.Y, Velocity.Z);
			else
				alGetSource3f(Source->Instance, AL_VELOCITY, &Velocity.X, &Velocity.Y, &Velocity.Z);
#endif
		}
		void AudioDevice::Pitch(AudioSource* Source, float& Value, bool Get)
		{
			VI_ASSERT(Source != nullptr, "souce should be set");
#ifdef VI_HAS_OPENAL
			if (!Get)
				alSourcef(Source->Instance, AL_PITCH, Value);
			else
				alGetSourcef(Source->Instance, AL_PITCH, &Value);
#endif
		}
		void AudioDevice::Gain(AudioSource* Source, float& Value, bool Get)
		{
			VI_ASSERT(Source != nullptr, "souce should be set");
#ifdef VI_HAS_OPENAL
			if (!Get)
				alSourcef(Source->Instance, AL_GAIN, Value);
			else
				alGetSourcef(Source->Instance, AL_GAIN, &Value);
#endif
		}
		void AudioDevice::ConeInnerAngle(AudioSource* Source, float& Value, bool Get)
		{
			VI_ASSERT(Source != nullptr, "souce should be set");
#ifdef VI_HAS_OPENAL
			if (!Get)
				alSourcef(Source->Instance, AL_CONE_INNER_ANGLE, Value);
			else
				alGetSourcef(Source->Instance, AL_CONE_INNER_ANGLE, &Value);
#endif
		}
		void AudioDevice::ConeOuterAngle(AudioSource* Source, float& Value, bool Get)
		{
			VI_ASSERT(Source != nullptr, "souce should be set");
#ifdef VI_HAS_OPENAL
			if (!Get)
				alSourcef(Source->Instance, AL_CONE_OUTER_ANGLE, Value);
			else
				alGetSourcef(Source->Instance, AL_CONE_OUTER_ANGLE, &Value);
#endif
		}
		void AudioDevice::ConeOuterGain(AudioSource* Source, float& Value, bool Get)
		{
			VI_ASSERT(Source != nullptr, "souce should be set");
#ifdef VI_HAS_OPENAL
			if (!Get)
				alSourcef(Source->Instance, AL_CONE_OUTER_GAIN, Value);
			else
				alGetSourcef(Source->Instance, AL_CONE_OUTER_GAIN, &Value);
#endif
		}
		void AudioDevice::Distance(AudioSource* Source, float& Value, bool Get)
		{
			VI_ASSERT(Source != nullptr, "souce should be set");
#ifdef VI_HAS_OPENAL
			if (!Get)
				alSourcef(Source->Instance, AL_MAX_DISTANCE, Value);
			else
				alGetSourcef(Source->Instance, AL_MAX_DISTANCE, &Value);
#endif
		}
		void AudioDevice::RefDistance(AudioSource* Source, float& Value, bool Get)
		{
			VI_ASSERT(Source != nullptr, "souce should be set");
#ifdef VI_HAS_OPENAL
			if (!Get)
				alSourcef(Source->Instance, AL_REFERENCE_DISTANCE, Value);
			else
				alGetSourcef(Source->Instance, AL_REFERENCE_DISTANCE, &Value);
#endif
		}
		void AudioDevice::Loop(AudioSource* Source, int& IsLoop, bool Get)
		{
			VI_ASSERT(Source != nullptr, "souce should be set");
#ifdef VI_HAS_OPENAL
			if (!Get)
				alSourcei(Source->Instance, AL_LOOPING, IsLoop);
			else
				alGetSourcei(Source->Instance, AL_LOOPING, &IsLoop);
#endif
		}
		void AudioDevice::SetDistanceModel(SoundDistanceModel Model)
		{
#ifdef VI_HAS_OPENAL
			alDistanceModel((int)Model);
#endif
		}
		void AudioDevice::GetExceptionCodes(int& ALCCode, int& ALCode) const
		{
#ifdef VI_HAS_OPENAL
			if ((ALCCode = alcGetError((ALCdevice*)Device)) != ALC_NO_ERROR)
				VI_ERR("[audio] %s", alcGetString((ALCdevice*)Device, ALCCode));

			if ((ALCode = alGetError()) != AL_NO_ERROR)
				VI_ERR("[audio] %s", alGetString(ALCode));
#endif
		}
		bool AudioDevice::IsValid() const
		{
			return Context && Device;
		}
	}
}
