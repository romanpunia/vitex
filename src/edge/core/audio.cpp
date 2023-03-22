#include "audio.h"
#include "../audio/effects.h"
#include "../audio/filters.h"
#ifdef ED_HAS_OPENAL
#ifdef ED_AL_AT_OPENAL
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
		void AudioContext::Create()
		{
			ED_TRACE("[audio] initialize audio context");
			if (!Mutex)
				Mutex = ED_NEW(std::mutex);

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
			Effects::EffectContext::Create();
			Filters::FilterContext::Create();
		}
		void AudioContext::Release()
		{
			ED_TRACE("[audio] free audio context");
			ED_DELETE(mutex, Mutex);
			Mutex = nullptr;
		}
		void AudioContext::Lock()
		{
			ED_ASSERT_V(Mutex != nullptr, "context should be initialized");
			Mutex->lock();
		}
		void AudioContext::Unlock()
		{
			ED_ASSERT_V(Mutex != nullptr, "context should be initialized");
			Mutex->unlock();
		}
		void AudioContext::GenerateBuffers(int Count, unsigned int* Buffers)
		{
			ED_ASSERT_V(Mutex != nullptr, "context should be initialized");
			ED_TRACE("[audio] generate %i buffers", Count);
			Mutex->lock();
#ifdef ED_HAS_OPENAL
			alGenBuffers(Count, Buffers);
#endif
			Mutex->unlock();
		}
		void AudioContext::SetBufferData(unsigned int Buffer, int Format, const void* Data, int Size, int Frequency)
		{
			ED_ASSERT_V(Mutex != nullptr, "context should be initialized");
			Mutex->lock();
#ifdef ED_HAS_OPENAL
			alBufferData(Buffer, Format, Data, Size, Frequency);
#endif
			Mutex->unlock();
		}
		void AudioContext::SetSourceData3F(unsigned int Source, SoundEx Value, float F1, float F2, float F3)
		{
			ED_ASSERT_V(Mutex != nullptr, "context should be initialized");
			Mutex->lock();
#ifdef ED_HAS_OPENAL
			alSource3f(Source, (uint32_t)Value, F1, F2, F3);
#endif
			Mutex->unlock();
		}
		void AudioContext::GetSourceData3F(unsigned int Source, SoundEx Value, float* F1, float* F2, float* F3)
		{
			ED_ASSERT_V(Mutex != nullptr, "context should be initialized");
			Mutex->lock();
#ifdef ED_HAS_OPENAL
			alGetSource3f(Source, (uint32_t)Value, F1, F2, F3);
#endif
			Mutex->unlock();
		}
		void AudioContext::SetSourceDataVF(unsigned int Source, SoundEx Value, float* FS)
		{
			ED_ASSERT_V(Mutex != nullptr, "context should be initialized");
			Mutex->lock();
#ifdef ED_HAS_OPENAL
			alSourcefv(Source, (uint32_t)Value, FS);
#endif
			Mutex->unlock();
		}
		void AudioContext::GetSourceDataVF(unsigned int Source, SoundEx Value, float* FS)
		{
			ED_ASSERT_V(Mutex != nullptr, "context should be initialized");
			Mutex->lock();
#ifdef ED_HAS_OPENAL
			alGetSourcefv(Source, (uint32_t)Value, FS);
#endif
			Mutex->unlock();
		}
		void AudioContext::SetSourceData1F(unsigned int Source, SoundEx Value, float F1)
		{
			ED_ASSERT_V(Mutex != nullptr, "context should be initialized");
			Mutex->lock();
#ifdef ED_HAS_OPENAL
			alSourcef(Source, (uint32_t)Value, F1);
#endif
			Mutex->unlock();
		}
		void AudioContext::GetSourceData1F(unsigned int Source, SoundEx Value, float* F1)
		{
			ED_ASSERT_V(Mutex != nullptr, "context should be initialized");
			Mutex->lock();
#ifdef ED_HAS_OPENAL
			alGetSourcef(Source, (uint32_t)Value, F1);
#endif
			Mutex->unlock();
		}
		void AudioContext::SetSourceData3I(unsigned int Source, SoundEx Value, int F1, int F2, int F3)
		{
			ED_ASSERT_V(Mutex != nullptr, "context should be initialized");
			Mutex->lock();
#ifdef ED_HAS_OPENAL
			alSource3i(Source, (uint32_t)Value, F1, F2, F3);
#endif
			Mutex->unlock();
		}
		void AudioContext::GetSourceData3I(unsigned int Source, SoundEx Value, int* F1, int* F2, int* F3)
		{
			ED_ASSERT_V(Mutex != nullptr, "context should be initialized");
			Mutex->lock();
#ifdef ED_HAS_OPENAL
			alGetSource3i(Source, (uint32_t)Value, F1, F2, F3);
#endif
			Mutex->unlock();
		}
		void AudioContext::SetSourceDataVI(unsigned int Source, SoundEx Value, int* FS)
		{
			ED_ASSERT_V(Mutex != nullptr, "context should be initialized");
			Mutex->lock();
#ifdef ED_HAS_OPENAL
			alSourceiv(Source, (uint32_t)Value, FS);
#endif
			Mutex->unlock();
		}
		void AudioContext::GetSourceDataVI(unsigned int Source, SoundEx Value, int* FS)
		{
			ED_ASSERT_V(Mutex != nullptr, "context should be initialized");
			Mutex->lock();
#ifdef ED_HAS_OPENAL
			alGetSourceiv(Source, (uint32_t)Value, FS);
#endif
			Mutex->unlock();
		}
		void AudioContext::SetSourceData1I(unsigned int Source, SoundEx Value, int F1)
		{
			ED_ASSERT_V(Mutex != nullptr, "context should be initialized");
			Mutex->lock();
#ifdef ED_HAS_OPENAL
			alSourcei(Source, (uint32_t)Value, F1);
#endif
			Mutex->unlock();
		}
		void AudioContext::GetSourceData1I(unsigned int Source, SoundEx Value, int* F1)
		{
			ED_ASSERT_V(Mutex != nullptr, "context should be initialized");
			Mutex->lock();
#ifdef ED_HAS_OPENAL
			alGetSourcei(Source, (uint32_t)Value, F1);
#endif
			Mutex->unlock();
		}
		void AudioContext::SetListenerData3F(SoundEx Listener, float F1, float F2, float F3)
		{
			ED_ASSERT_V(Mutex != nullptr, "context should be initialized");
			Mutex->lock();
#ifdef ED_HAS_OPENAL
			alListener3f((uint32_t)Listener, F1, F2, F3);
#endif
			Mutex->unlock();
		}
		void AudioContext::GetListenerData3F(SoundEx Listener, float* F1, float* F2, float* F3)
		{
			ED_ASSERT_V(Mutex != nullptr, "context should be initialized");
			Mutex->lock();
#ifdef ED_HAS_OPENAL
			alGetListener3f((uint32_t)Listener, F1, F2, F3);
#endif
			Mutex->unlock();
		}
		void AudioContext::SetListenerDataVF(SoundEx Listener, float* FS)
		{
			ED_ASSERT_V(Mutex != nullptr, "context should be initialized");
			Mutex->lock();
#ifdef ED_HAS_OPENAL
			alListenerfv((uint32_t)Listener, FS);
#endif
			Mutex->unlock();
		}
		void AudioContext::GetListenerDataVF(SoundEx Listener, float* FS)
		{
			ED_ASSERT_V(Mutex != nullptr, "context should be initialized");
			Mutex->lock();
#ifdef ED_HAS_OPENAL
			alGetListenerfv((uint32_t)Listener, FS);
#endif
			Mutex->unlock();
		}
		void AudioContext::SetListenerData1F(SoundEx Listener, float F1)
		{
			ED_ASSERT_V(Mutex != nullptr, "context should be initialized");
			Mutex->lock();
#ifdef ED_HAS_OPENAL
			alListenerf((uint32_t)Listener, F1);
#endif
			Mutex->unlock();
		}
		void AudioContext::GetListenerData1F(SoundEx Listener, float* F1)
		{
			ED_ASSERT_V(Mutex != nullptr, "context should be initialized");
			Mutex->lock();
#ifdef ED_HAS_OPENAL
			alGetListenerf((uint32_t)Listener, F1);
#endif
			Mutex->unlock();
		}
		void AudioContext::SetListenerData3I(SoundEx Listener, int F1, int F2, int F3)
		{
			ED_ASSERT_V(Mutex != nullptr, "context should be initialized");
			Mutex->lock();
#ifdef ED_HAS_OPENAL
			alListener3i((uint32_t)Listener, F1, F2, F3);
#endif
			Mutex->unlock();
		}
		void AudioContext::GetListenerData3I(SoundEx Listener, int* F1, int* F2, int* F3)
		{
			ED_ASSERT_V(Mutex != nullptr, "context should be initialized");
			Mutex->lock();
#ifdef ED_HAS_OPENAL
			alGetListener3i((uint32_t)Listener, F1, F2, F3);
#endif
			Mutex->unlock();
		}
		void AudioContext::SetListenerDataVI(SoundEx Listener, int* FS)
		{
			ED_ASSERT_V(Mutex != nullptr, "context should be initialized");
			Mutex->lock();
#ifdef ED_HAS_OPENAL
			alListeneriv((uint32_t)Listener, FS);
#endif
			Mutex->unlock();
		}
		void AudioContext::GetListenerDataVI(SoundEx Listener, int* FS)
		{
			ED_ASSERT_V(Mutex != nullptr, "context should be initialized");
			Mutex->lock();
#ifdef ED_HAS_OPENAL
			alGetListeneriv((uint32_t)Listener, FS);
#endif
			Mutex->unlock();
		}
		void AudioContext::SetListenerData1I(SoundEx Listener, int F1)
		{
			ED_ASSERT_V(Mutex != nullptr, "context should be initialized");
			Mutex->lock();
#ifdef ED_HAS_OPENAL
			alListeneri((uint32_t)Listener, F1);
#endif
			Mutex->unlock();
		}
		void AudioContext::GetListenerData1I(SoundEx Listener, int* F1)
		{
			ED_ASSERT_V(Mutex != nullptr, "context should be initialized");
			Mutex->lock();
#ifdef ED_HAS_OPENAL
			alGetListeneri((uint32_t)Listener, F1);
#endif
			Mutex->unlock();
		}
		std::mutex* AudioContext::Mutex = nullptr;
		int AudioContext::State = 0;

		AudioFilter::AudioFilter() noexcept
		{
#if defined(ED_HAS_OPENAL) && defined(HAS_EFX)
			Filter = AL_FILTER_NULL;
#endif
		}
		AudioFilter::~AudioFilter() noexcept
		{
#if defined(ED_HAS_OPENAL) && defined(HAS_EFX)
			ED_TRACE("[audio] delete %i filter", (int)Filter);
			AudioContext::Lock();
			if (alDeleteFilters != nullptr && Filter != AL_FILTER_NULL)
				alDeleteFilters(1, &Filter);
			AudioContext::Unlock();
#endif
		}
		bool AudioFilter::CreateLocked(const std::function<bool()>& Callback)
		{
#if defined(ED_HAS_OPENAL) && defined(HAS_EFX)
			AudioContext::Lock();
			if (alDeleteFilters != nullptr && Filter != AL_FILTER_NULL)
				alDeleteFilters(1, &Filter);

			if (alGenFilters != nullptr)
				alGenFilters(1, &Filter);

			if (Callback)
				Callback();
			AudioContext::Unlock();
			ED_TRACE("[audio] generate %i filter", (int)Filter);
#endif
			return true;
		}
		AudioSource* AudioFilter::GetSource() const
		{
			return Source;
		}

		AudioEffect::AudioEffect() noexcept
		{
#if defined(ED_HAS_OPENAL) && defined(HAS_EFX)
			Effect = AL_EFFECT_NULL;
			Slot = AL_EFFECTSLOT_NULL;
#endif
		}
		AudioEffect::~AudioEffect() noexcept
		{
			Unbind();
#if defined(ED_HAS_OPENAL) && defined(HAS_EFX)
			ED_TRACE("[audio] delete %i effect", (int)Effect);
			AudioContext::Lock();
			if (alDeleteEffects != nullptr && Effect != AL_EFFECT_NULL)
				alDeleteEffects(1, &Effect);

			if (alDeleteAuxiliaryEffectSlots != nullptr && Slot != AL_EFFECTSLOT_NULL)
				alDeleteAuxiliaryEffectSlots(1, &Slot);
			AudioContext::Unlock();
#endif
			ED_RELEASE(Filter);
		}
		bool AudioEffect::CreateLocked(const std::function<bool()>& Callback)
		{
#if defined(ED_HAS_OPENAL) && defined(HAS_EFX)
			AudioContext::Lock();
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
			AudioContext::Unlock();
			ED_TRACE("[audio] generate %i effect", (int)Effect);
#endif
			return true;
		}
		bool AudioEffect::SetFilter(AudioFilter** NewFilter)
		{
			ED_CLEAR(Filter);
			if (!NewFilter || !*NewFilter)
				return true;

			Filter = *NewFilter;
			*NewFilter = nullptr;
			Bind(Source, Zone);

			return true;
		}
		bool AudioEffect::Bind(AudioSource* NewSource, int NewZone)
		{
			ED_ASSERT(Source != nullptr, false, "source should not be empty");
			Source = NewSource;
			Zone = NewZone;
#if defined(ED_HAS_OPENAL) && defined(HAS_EFX)
			AudioContext::Lock();
			alSource3i(Source->GetInstance(), AL_AUXILIARY_SEND_FILTER, (ALint)Slot, Zone, (ALint)(Filter ? Filter->Filter : AL_FILTER_NULL));
			AudioContext::Unlock();
#endif
			return true;
		}
		bool AudioEffect::Unbind()
		{
			ED_ASSERT(Source != nullptr, false, "source should not be empty");
#if defined(ED_HAS_OPENAL) && defined(HAS_EFX)
			AudioContext::Lock();
			alSource3i(Source->GetInstance(), AL_AUXILIARY_SEND_FILTER, AL_EFFECTSLOT_NULL, Zone, AL_FILTER_NULL);
			AudioContext::Unlock();
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
#ifdef ED_HAS_OPENAL
			ED_TRACE("[audio] delete %i buffer", (int)Buffer);
			AudioContext::Lock();
			alDeleteBuffers(1, &Buffer);
			AudioContext::Unlock();
			Buffer = 0;
#endif
		}
		float AudioClip::Length() const
		{
#ifdef ED_HAS_OPENAL
			AudioContext::Lock();
			int ByteSize = 0, ChannelCount = 0, Bits = 0, Frequency = 0;
			alGetBufferi(Buffer, AL_SIZE, &ByteSize);
			alGetBufferi(Buffer, AL_CHANNELS, &ChannelCount);
			alGetBufferi(Buffer, AL_BITS, &Bits);
			alGetBufferi(Buffer, AL_FREQUENCY, &Frequency);

			AudioContext::Unlock();
			if (ByteSize == 0 || ChannelCount == 0 || Bits == 0 || Frequency == 0)
				return 0;

			return (float)(ByteSize * 8 / (ChannelCount * Bits)) / (float)Frequency;
#else
			return 0.0f;
#endif
		}
		bool AudioClip::IsMono() const
		{
#ifdef ED_HAS_OPENAL
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
#ifdef ED_HAS_OPENAL
			AudioContext::Lock();
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
			AudioContext::Unlock();
			ED_TRACE("[audio] generate %i source", (int)Instance);
#endif
		}
		AudioSource::~AudioSource() noexcept
		{
			RemoveEffects();
			ED_RELEASE(Clip);
#ifdef ED_HAS_OPENAL
			ED_TRACE("[audio] delete %i source", (int)Instance);
			AudioContext::Lock();
			alSourceStop(Instance);
			alSourcei(Instance, AL_BUFFER, 0);
			alDeleteSources(1, &Instance);
			AudioContext::Unlock();
#endif
		}
		int64_t AudioSource::AddEffect(AudioEffect* Effect)
		{
			ED_ASSERT(Effect != nullptr, -1, "effect should be set");
			Effect->Bind(this, (int)Effects.size());
			Effects.push_back(Effect);
			return Effects.size() - 1;
		}
		bool AudioSource::RemoveEffect(size_t EffectId)
		{
			ED_ASSERT(EffectId < Effects.size(), false, "index outside of range");
			auto It = Effects.begin() + EffectId;
			ED_RELEASE(*It);
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
				ED_RELEASE(Effect);
			Effects.clear();
		}
		void AudioSource::SetClip(AudioClip* NewClip)
		{
#ifdef ED_HAS_OPENAL
			ED_TRACE("[audio] set clip %i on %i source", NewClip ? (int)NewClip->GetBuffer() : 0, (int)Instance);
			AudioContext::Lock();
			alSourceStop(Instance);

			ED_RELEASE(Clip);
			Clip = NewClip;

			if (Clip != nullptr)
				alSourcei(Instance, AL_BUFFER, Clip->GetBuffer());

			AudioContext::Unlock();
#endif
		}
		void AudioSource::Synchronize(AudioSync* Sync, const Compute::Vector3& Position)
		{
			ED_ASSERT_V(Sync != nullptr, "sync should be set");
			for (auto* Effect : Effects)
			{
				if (!Effect)
					continue;

				Effect->Synchronize();
				if (Effect->Filter != nullptr)
					Effect->Filter->Synchronize();
#if defined(ED_HAS_OPENAL) && defined(HAS_EFX)
				if (alAuxiliaryEffectSloti != nullptr && Effect->Effect != AL_EFFECT_NULL && Effect->Slot != AL_EFFECTSLOT_NULL)
					alAuxiliaryEffectSloti(Effect->Slot, AL_EFFECTSLOT_EFFECT, (ALint)Effect->Effect);
#endif
			}
#ifdef ED_HAS_OPENAL
			AudioContext::Lock();
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
			AudioContext::Unlock();
#endif
		}
		void AudioSource::Reset()
		{
#ifdef ED_HAS_OPENAL
			ED_TRACE("[audio] reset on %i source", (int)Instance);
			AudioContext::Lock();
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
			AudioContext::Unlock();
#endif
		}
		void AudioSource::Pause()
		{
#ifdef ED_HAS_OPENAL
			ED_TRACE("[audio] pause on %i source", (int)Instance);
			AudioContext::Lock();
			alSourcePause(Instance);
			AudioContext::Unlock();
#endif
		}
		void AudioSource::Play()
		{
#ifdef ED_HAS_OPENAL
			ED_TRACE("[audio] play on %i source", (int)Instance);
			AudioContext::Lock();
			alSourcePlay(Instance);
			AudioContext::Unlock();
#endif
		}
		void AudioSource::Stop()
		{
#ifdef ED_HAS_OPENAL
			ED_TRACE("[audio] stop on %i source", (int)Instance);
			AudioContext::Lock();
			alSourceStop(Instance);
			AudioContext::Unlock();
#endif
		}
		bool AudioSource::IsPlaying() const
		{
#ifdef ED_HAS_OPENAL
			int State = 0;
			AudioContext::Lock();
			alGetSourcei(Instance, AL_SOURCE_STATE, &State);
			AudioContext::Unlock();

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
		const std::vector<AudioEffect*>& AudioSource::GetEffects() const
		{
			return Effects;
		}

		AudioDevice::AudioDevice() noexcept
		{
#ifdef ED_HAS_OPENAL
			AudioContext::Lock();
			Device = (void*)alcOpenDevice(nullptr);
			ED_TRACE("[audio] open alc device: 0x%" PRIXPTR, (void*)Device);
			if (!Device)
			{
				AudioContext::Unlock();
				ED_ERR("[audio] couldn't create alc device");

				int Code = alGetError();
				if (Code != AL_NO_ERROR)
					ED_ERR("[audio] %s", alGetString(Code));

				return;
			}

			Context = (void*)alcCreateContext((ALCdevice*)Device, nullptr);
			ED_TRACE("[audio] create alc context: 0x%" PRIXPTR, (void*)Context);
			if (!Context)
			{
				AudioContext::Unlock();
				ED_ERR("[audio] couldn't create alc device context");

				int Code = alcGetError((ALCdevice*)Device);
				if (Code != AL_NO_ERROR)
					ED_ERR("[audio] %s", alcGetString((ALCdevice*)Device, Code));

				return;
			}

			alcMakeContextCurrent((ALCcontext*)Context);
			alDistanceModel(AL_LINEAR_DISTANCE);
			alListenerf(AL_GAIN, 0.0f);
			AudioContext::Unlock();
#endif
		}
		AudioDevice::~AudioDevice() noexcept
		{
#ifdef ED_HAS_OPENAL
			AudioContext::Lock();
			if (Context != nullptr)
			{
				ED_TRACE("[audio] delete alc context: 0x%" PRIXPTR, (void*)Context);
				alcMakeContextCurrent(nullptr);
				alcDestroyContext((ALCcontext*)Context);
				Context = nullptr;
			}

			if (Device != nullptr)
			{
				ED_TRACE("[audio] close alc context: 0x%" PRIXPTR, (void*)Device);
				alcCloseDevice((ALCdevice*)Device);
				Device = nullptr;
			}
			AudioContext::Unlock();
#endif
		}
		void AudioDevice::Offset(AudioSource* Source, float& Seconds, bool Get)
		{
			ED_ASSERT_V(Source != nullptr, "souce should be set");
#ifdef ED_HAS_OPENAL
			AudioContext::Lock();
			if (!Get)
				alSourcef(Source->Instance, AL_SEC_OFFSET, Seconds);
			else
				alGetSourcef(Source->Instance, AL_SEC_OFFSET, &Seconds);
			AudioContext::Unlock();
#endif
		}
		void AudioDevice::Relative(AudioSource* Source, int& Value, bool Get)
		{
			ED_ASSERT_V(Source != nullptr, "souce should be set");
#ifdef ED_HAS_OPENAL
			AudioContext::Lock();
			if (!Get)
				alSourcei(Source->Instance, AL_SOURCE_RELATIVE, Value);
			else
				alGetSourcei(Source->Instance, AL_SOURCE_RELATIVE, &Value);
			AudioContext::Unlock();
#endif
		}
		void AudioDevice::Position(AudioSource* Source, Compute::Vector3& Position, bool Get)
		{
			ED_ASSERT_V(Source != nullptr, "souce should be set");
#ifdef ED_HAS_OPENAL
			AudioContext::Lock();
			if (!Get)
			{
				alGetSource3f(Source->Instance, AL_POSITION, &Position.X, &Position.Y, &Position.Z);
				Position.X = -Position.X;
			}
			else
				alSource3f(Source->Instance, AL_POSITION, -Position.X, Position.Y, Position.Z);
			AudioContext::Unlock();
#endif
		}
		void AudioDevice::Direction(AudioSource* Source, Compute::Vector3& Direction, bool Get)
		{
			ED_ASSERT_V(Source != nullptr, "souce should be set");
#ifdef ED_HAS_OPENAL
			AudioContext::Lock();
			if (!Get)
				alSource3f(Source->Instance, AL_DIRECTION, Direction.X, Direction.Y, Direction.Z);
			else
				alGetSource3f(Source->Instance, AL_DIRECTION, &Direction.X, &Direction.Y, &Direction.Z);
			AudioContext::Unlock();
#endif
		}
		void AudioDevice::Velocity(AudioSource* Source, Compute::Vector3& Velocity, bool Get)
		{
			ED_ASSERT_V(Source != nullptr, "souce should be set");
#ifdef ED_HAS_OPENAL
			AudioContext::Lock();
			if (!Get)
				alSource3f(Source->Instance, AL_VELOCITY, Velocity.X, Velocity.Y, Velocity.Z);
			else
				alGetSource3f(Source->Instance, AL_VELOCITY, &Velocity.X, &Velocity.Y, &Velocity.Z);
			AudioContext::Unlock();
#endif
		}
		void AudioDevice::Pitch(AudioSource* Source, float& Value, bool Get)
		{
			ED_ASSERT_V(Source != nullptr, "souce should be set");
#ifdef ED_HAS_OPENAL
			AudioContext::Lock();
			if (!Get)
				alSourcef(Source->Instance, AL_PITCH, Value);
			else
				alGetSourcef(Source->Instance, AL_PITCH, &Value);
			AudioContext::Unlock();
#endif
		}
		void AudioDevice::Gain(AudioSource* Source, float& Value, bool Get)
		{
			ED_ASSERT_V(Source != nullptr, "souce should be set");
#ifdef ED_HAS_OPENAL
			AudioContext::Lock();
			if (!Get)
				alSourcef(Source->Instance, AL_GAIN, Value);
			else
				alGetSourcef(Source->Instance, AL_GAIN, &Value);
			AudioContext::Unlock();
#endif
		}
		void AudioDevice::ConeInnerAngle(AudioSource* Source, float& Value, bool Get)
		{
			ED_ASSERT_V(Source != nullptr, "souce should be set");
#ifdef ED_HAS_OPENAL
			AudioContext::Lock();
			if (!Get)
				alSourcef(Source->Instance, AL_CONE_INNER_ANGLE, Value);
			else
				alGetSourcef(Source->Instance, AL_CONE_INNER_ANGLE, &Value);
			AudioContext::Unlock();
#endif
		}
		void AudioDevice::ConeOuterAngle(AudioSource* Source, float& Value, bool Get)
		{
			ED_ASSERT_V(Source != nullptr, "souce should be set");
#ifdef ED_HAS_OPENAL
			AudioContext::Lock();
			if (!Get)
				alSourcef(Source->Instance, AL_CONE_OUTER_ANGLE, Value);
			else
				alGetSourcef(Source->Instance, AL_CONE_OUTER_ANGLE, &Value);
			AudioContext::Unlock();
#endif
		}
		void AudioDevice::ConeOuterGain(AudioSource* Source, float& Value, bool Get)
		{
			ED_ASSERT_V(Source != nullptr, "souce should be set");
#ifdef ED_HAS_OPENAL
			AudioContext::Lock();
			if (!Get)
				alSourcef(Source->Instance, AL_CONE_OUTER_GAIN, Value);
			else
				alGetSourcef(Source->Instance, AL_CONE_OUTER_GAIN, &Value);
			AudioContext::Unlock();
#endif
		}
		void AudioDevice::Distance(AudioSource* Source, float& Value, bool Get)
		{
			ED_ASSERT_V(Source != nullptr, "souce should be set");
#ifdef ED_HAS_OPENAL
			AudioContext::Lock();
			if (!Get)
				alSourcef(Source->Instance, AL_MAX_DISTANCE, Value);
			else
				alGetSourcef(Source->Instance, AL_MAX_DISTANCE, &Value);
			AudioContext::Unlock();
#endif
		}
		void AudioDevice::RefDistance(AudioSource* Source, float& Value, bool Get)
		{
			ED_ASSERT_V(Source != nullptr, "souce should be set");
#ifdef ED_HAS_OPENAL
			AudioContext::Lock();
			if (!Get)
				alSourcef(Source->Instance, AL_REFERENCE_DISTANCE, Value);
			else
				alGetSourcef(Source->Instance, AL_REFERENCE_DISTANCE, &Value);
			AudioContext::Unlock();
#endif
		}
		void AudioDevice::Loop(AudioSource* Source, int& IsLoop, bool Get)
		{
			ED_ASSERT_V(Source != nullptr, "souce should be set");
#ifdef ED_HAS_OPENAL
			AudioContext::Lock();
			if (!Get)
				alSourcei(Source->Instance, AL_LOOPING, IsLoop);
			else
				alGetSourcei(Source->Instance, AL_LOOPING, &IsLoop);
			AudioContext::Unlock();
#endif
		}
		void AudioDevice::SetDistanceModel(SoundDistanceModel Model)
		{
#ifdef ED_HAS_OPENAL
			AudioContext::Lock();
			alDistanceModel((int)Model);
			AudioContext::Unlock();
#endif
		}
		void AudioDevice::GetExceptionCodes(int& ALCCode, int& ALCode) const
		{
#ifdef ED_HAS_OPENAL
			AudioContext::Lock();
			if ((ALCCode = alcGetError((ALCdevice*)Device)) != ALC_NO_ERROR)
			{
				AudioContext::Unlock();
				ED_ERR("[audio] %s", alcGetString((ALCdevice*)Device, ALCCode));
				AudioContext::Lock();
			}

			if ((ALCode = alGetError()) != AL_NO_ERROR)
			{
				AudioContext::Unlock();
				ED_ERR("[audio] %s", alGetString(ALCode));
				AudioContext::Lock();
			}
			AudioContext::Unlock();
#endif
		}
		bool AudioDevice::IsValid() const
		{
			return Context && Device;
		}
	}
}
