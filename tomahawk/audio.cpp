#include "audio.h"
#ifdef THAWK_HAS_OPENAL
#include <AL/al.h>
#include <AL/alc.h>
#endif

namespace Tomahawk
{
	namespace Audio
	{
		void AudioContext::Create()
		{
			if (!Mutex)
				Mutex = new std::mutex();
		}
		void AudioContext::Release()
		{
			delete Mutex;
			Mutex = nullptr;
		}
		void AudioContext::Lock()
		{
			if (Mutex != nullptr)
				Mutex->lock();
		}
		void AudioContext::Unlock()
		{
			if (Mutex != nullptr)
				Mutex->unlock();
		}
		void AudioContext::GenerateBuffers(int Count, unsigned int* Buffers)
		{
			if (Mutex != nullptr)
				Mutex->lock();
#ifdef THAWK_HAS_OPENAL
			alGenBuffers(Count, Buffers);
#endif
			if (Mutex != nullptr)
				Mutex->unlock();
		}
		void AudioContext::SetBufferData(unsigned int Buffer, int Format, const void* Data, int Size, int Frequency)
		{
			if (Mutex != nullptr)
				Mutex->lock();
#ifdef THAWK_HAS_OPENAL
			alBufferData(Buffer, Format, Data, Size, Frequency);
#endif
			if (Mutex != nullptr)
				Mutex->unlock();
		}
		void AudioContext::SetSourceData3F(unsigned int Source, SoundEx Value, float F1, float F2, float F3)
		{
			if (Mutex != nullptr)
				Mutex->lock();
#ifdef THAWK_HAS_OPENAL
			alSource3f(Source, Value, F1, F2, F3);
#endif
			if (Mutex != nullptr)
				Mutex->unlock();
		}
		void AudioContext::GetSourceData3F(unsigned int Source, SoundEx Value, float* F1, float* F2, float* F3)
		{
			if (Mutex != nullptr)
				Mutex->lock();
#ifdef THAWK_HAS_OPENAL
			alGetSource3f(Source, Value, F1, F2, F3);
#endif
			if (Mutex != nullptr)
				Mutex->unlock();
		}
		void AudioContext::SetSourceDataVF(unsigned int Source, SoundEx Value, float* FS)
		{
			if (Mutex != nullptr)
				Mutex->lock();
#ifdef THAWK_HAS_OPENAL
			alSourcefv(Source, Value, FS);
#endif
			if (Mutex != nullptr)
				Mutex->unlock();
		}
		void AudioContext::GetSourceDataVF(unsigned int Source, SoundEx Value, float* FS)
		{
			if (Mutex != nullptr)
				Mutex->lock();
#ifdef THAWK_HAS_OPENAL
			alGetSourcefv(Source, Value, FS);
#endif
			if (Mutex != nullptr)
				Mutex->unlock();
		}
		void AudioContext::SetSourceData1F(unsigned int Source, SoundEx Value, float F1)
		{
			if (Mutex != nullptr)
				Mutex->lock();
#ifdef THAWK_HAS_OPENAL
			alSourcef(Source, Value, F1);
#endif
			if (Mutex != nullptr)
				Mutex->unlock();
		}
		void AudioContext::GetSourceData1F(unsigned int Source, SoundEx Value, float* F1)
		{
			if (Mutex != nullptr)
				Mutex->lock();
#ifdef THAWK_HAS_OPENAL
			alGetSourcef(Source, Value, F1);
#endif
			if (Mutex != nullptr)
				Mutex->unlock();
		}
		void AudioContext::SetSourceData3I(unsigned int Source, SoundEx Value, int F1, int F2, int F3)
		{
			if (Mutex != nullptr)
				Mutex->lock();
#ifdef THAWK_HAS_OPENAL
			alSource3i(Source, Value, F1, F2, F3);
#endif
			if (Mutex != nullptr)
				Mutex->unlock();
		}
		void AudioContext::GetSourceData3I(unsigned int Source, SoundEx Value, int* F1, int* F2, int* F3)
		{
			if (Mutex != nullptr)
				Mutex->lock();
#ifdef THAWK_HAS_OPENAL
			alGetSource3i(Source, Value, F1, F2, F3);
#endif
			if (Mutex != nullptr)
				Mutex->unlock();
		}
		void AudioContext::SetSourceDataVI(unsigned int Source, SoundEx Value, int* FS)
		{
			if (Mutex != nullptr)
				Mutex->lock();
#ifdef THAWK_HAS_OPENAL
			alGetSourceiv(Source, Value, FS);
#endif
			if (Mutex != nullptr)
				Mutex->unlock();
		}
		void AudioContext::GetSourceDataVI(unsigned int Source, SoundEx Value, int* FS)
		{
			if (Mutex != nullptr)
				Mutex->lock();
#ifdef THAWK_HAS_OPENAL
			alGetSourceiv(Source, Value, FS);
#endif
			if (Mutex != nullptr)
				Mutex->unlock();
		}
		void AudioContext::SetSourceData1I(unsigned int Source, SoundEx Value, int F1)
		{
			if (Mutex != nullptr)
				Mutex->lock();
#ifdef THAWK_HAS_OPENAL
			alSourcei(Source, Value, F1);
#endif
			if (Mutex != nullptr)
				Mutex->unlock();
		}
		void AudioContext::GetSourceData1I(unsigned int Source, SoundEx Value, int* F1)
		{
			if (Mutex != nullptr)
				Mutex->lock();
#ifdef THAWK_HAS_OPENAL
			alGetSourcei(Source, Value, F1);
#endif
			if (Mutex != nullptr)
				Mutex->unlock();
		}
		void AudioContext::SetListenerData3F(unsigned int Listener, float F1, float F2, float F3)
		{
			if (Mutex != nullptr)
				Mutex->lock();
#ifdef THAWK_HAS_OPENAL
			alListener3f(Listener, F1, F2, F3);
#endif
			if (Mutex != nullptr)
				Mutex->unlock();
		}
		void AudioContext::GetListenerData3F(unsigned int Listener, float* F1, float* F2, float* F3)
		{
			if (Mutex != nullptr)
				Mutex->lock();
#ifdef THAWK_HAS_OPENAL
			alGetListener3f(Listener, F1, F2, F3);
#endif
			if (Mutex != nullptr)
				Mutex->unlock();
		}
		void AudioContext::SetListenerDataVF(unsigned int Listener, float* FS)
		{
			if (Mutex != nullptr)
				Mutex->lock();
#ifdef THAWK_HAS_OPENAL
			alListenerfv(Listener, FS);
#endif
			if (Mutex != nullptr)
				Mutex->unlock();
		}
		void AudioContext::GetListenerDataVF(unsigned int Listener, float* FS)
		{
			if (Mutex != nullptr)
				Mutex->lock();
#ifdef THAWK_HAS_OPENAL
			alGetListenerfv(Listener, FS);
#endif
			if (Mutex != nullptr)
				Mutex->unlock();
		}
		void AudioContext::SetListenerData1F(unsigned int Listener, float F1)
		{
			if (Mutex != nullptr)
				Mutex->lock();
#ifdef THAWK_HAS_OPENAL
			alListenerf(Listener, F1);
#endif
			if (Mutex != nullptr)
				Mutex->unlock();
		}
		void AudioContext::GetListenerData1F(unsigned int Listener, float* F1)
		{
			if (Mutex != nullptr)
				Mutex->lock();
#ifdef THAWK_HAS_OPENAL
			alGetListenerf(Listener, F1);
#endif
			if (Mutex != nullptr)
				Mutex->unlock();
		}
		void AudioContext::SetListenerData3I(unsigned int Listener, int F1, int F2, int F3)
		{
			if (Mutex != nullptr)
				Mutex->lock();
#ifdef THAWK_HAS_OPENAL
			alListener3i(Listener, F1, F2, F3);
#endif
			if (Mutex != nullptr)
				Mutex->unlock();
		}
		void AudioContext::GetListenerData3I(unsigned int Listener, int* F1, int* F2, int* F3)
		{
			if (Mutex != nullptr)
				Mutex->lock();
#ifdef THAWK_HAS_OPENAL
			alGetListener3i(Listener, F1, F2, F3);
#endif
			if (Mutex != nullptr)
				Mutex->unlock();
		}
		void AudioContext::SetListenerDataVI(unsigned int Listener, int* FS)
		{
			if (Mutex != nullptr)
				Mutex->lock();
#ifdef THAWK_HAS_OPENAL
			alGetListeneriv(Listener, FS);
#endif
			if (Mutex != nullptr)
				Mutex->unlock();
		}
		void AudioContext::GetListenerDataVI(unsigned int Listener, int* FS)
		{
			if (Mutex != nullptr)
				Mutex->lock();
#ifdef THAWK_HAS_OPENAL
			alGetListeneriv(Listener, FS);
#endif
			if (Mutex != nullptr)
				Mutex->unlock();
		}
		void AudioContext::SetListenerData1I(unsigned int Listener, int F1)
		{
			if (Mutex != nullptr)
				Mutex->lock();
#ifdef THAWK_HAS_OPENAL
			alListeneri(Listener, F1);
#endif
			if (Mutex != nullptr)
				Mutex->unlock();
		}
		void AudioContext::GetListenerData1I(unsigned int Listener, int* F1)
		{
			if (Mutex != nullptr)
				Mutex->lock();
#ifdef THAWK_HAS_OPENAL
			alGetListeneri(Listener, F1);
#endif
			if (Mutex != nullptr)
				Mutex->unlock();
		}
		std::mutex* AudioContext::Mutex = nullptr;
		int AudioContext::State = 0;

		AudioClip::AudioClip(int BufferCount, int NewFormat) : Format(NewFormat)
		{
			if (BufferCount > 0)
				AudioContext::GenerateBuffers(BufferCount, &Buffer);
		}
		AudioClip::~AudioClip()
		{
#ifdef THAWK_HAS_OPENAL
			AudioContext::Lock();
			alDeleteBuffers(1, &Buffer);
			AudioContext::Unlock();
			Buffer = 0;
#endif
		}
		float AudioClip::Length()
		{
#ifdef THAWK_HAS_OPENAL
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
		bool AudioClip::IsMono()
		{
#ifdef THAWK_HAS_OPENAL
			if (Format == AL_FORMAT_MONO8 || Format == AL_FORMAT_MONO16)
				return true;
#endif
			return false;
		}
		unsigned int AudioClip::GetBuffer()
		{
			return Buffer;
		}
		int AudioClip::GetFormat()
		{
			return Format;
		}

		AudioSource::AudioSource()
		{
#ifdef THAWK_HAS_OPENAL
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
#endif
		}
		AudioSource::~AudioSource()
		{
#ifdef THAWK_HAS_OPENAL
			AudioContext::Lock();
			alSourceStop(Instance);
			alSourcei(Instance, AL_BUFFER, 0);
			alDeleteSources(1, &Instance);
			AudioContext::Unlock();

			Clip = nullptr;
			Instance = 0;
#endif
		}
		void AudioSource::Apply(AudioClip* In)
		{
#ifdef THAWK_HAS_OPENAL
			AudioContext::Lock();
			alSourceStop(Instance);

			Clip = In;
			if (Clip != nullptr)
				alSourcei(Instance, AL_BUFFER, Clip->GetBuffer());

			AudioContext::Unlock();
#endif
		}
		void AudioSource::Reset()
		{
#ifdef THAWK_HAS_OPENAL
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
#ifdef THAWK_HAS_OPENAL
			AudioContext::Lock();
			alSourcePause(Instance);
			AudioContext::Unlock();
#endif
		}
		void AudioSource::Play()
		{
#ifdef THAWK_HAS_OPENAL
			AudioContext::Lock();
			alSourcePlay(Instance);
			AudioContext::Unlock();
#endif
		}
		void AudioSource::Stop()
		{
#ifdef THAWK_HAS_OPENAL
			AudioContext::Lock();
			alSourceStop(Instance);
			AudioContext::Unlock();
#endif
		}
		bool AudioSource::IsPlaying() const
		{
#ifdef THAWK_HAS_OPENAL
			int State = 0;

			AudioContext::Lock();
			alGetSourcei(Instance, AL_SOURCE_STATE, &State);
			AudioContext::Unlock();

			return State == AL_PLAYING;
#else
			return false;
#endif
		}
		AudioClip* AudioSource::GetClip() const
		{
			return Clip;
		}
		unsigned int AudioSource::GetInstance() const
		{
			return Instance;
		}

		AudioDevice::AudioDevice()
		{
#ifdef THAWK_HAS_OPENAL
			AudioContext::Lock();
			Device = (void*)alcOpenDevice(nullptr);
			if (!Device)
			{
				THAWK_ERROR("couldn't create alc device");
				return;
			}

			Context = (void*)alcCreateContext((ALCdevice*)Device, nullptr);
			if (!Context)
			{
				THAWK_ERROR("couldn't create alc device context");
				return;
			}

			alcMakeContextCurrent((ALCcontext*)Context);
			alDistanceModel(AL_LINEAR_DISTANCE);
			alListenerf(AL_GAIN, 0.0f);
			AudioContext::Unlock();
#endif
		}
		AudioDevice::~AudioDevice()
		{
#ifdef THAWK_HAS_OPENAL
			AudioContext::Lock();
			if (Context != nullptr)
			{
				alcMakeContextCurrent(nullptr);
				alcDestroyContext((ALCcontext*)Context);
				Context = nullptr;
			}

			if (Device != nullptr)
			{
				alcCloseDevice((ALCdevice*)Device);
				Device = nullptr;
			}
			AudioContext::Unlock();
#endif
		}
		void AudioDevice::Offset(AudioSource* Source, float& Seconds, bool Get)
		{
#ifdef THAWK_HAS_OPENAL
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
#ifdef THAWK_HAS_OPENAL
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
#ifdef THAWK_HAS_OPENAL
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
#ifdef THAWK_HAS_OPENAL
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
#ifdef THAWK_HAS_OPENAL
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
#ifdef THAWK_HAS_OPENAL
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
#ifdef THAWK_HAS_OPENAL
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
#ifdef THAWK_HAS_OPENAL
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
#ifdef THAWK_HAS_OPENAL
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
#ifdef THAWK_HAS_OPENAL
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
#ifdef THAWK_HAS_OPENAL
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
#ifdef THAWK_HAS_OPENAL
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
#ifdef THAWK_HAS_OPENAL
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
#ifdef THAWK_HAS_OPENAL
			AudioContext::Lock();
			alDistanceModel((int)Model);
			AudioContext::Unlock();
#endif
		}
		void AudioDevice::GetExceptionCodes(int& ALCCode, int& ALCode)
		{
#ifdef THAWK_HAS_OPENAL
			AudioContext::Lock();
			if ((ALCCode = alcGetError((ALCdevice*)Device)) != ALC_NO_ERROR)
			{
				AudioContext::Unlock();
				THAWK_ERROR(alcGetString((ALCdevice*)Device, ALCCode));
				AudioContext::Lock();
			}

			if ((ALCode = alGetError()) != AL_NO_ERROR)
			{
				AudioContext::Unlock();
				THAWK_ERROR(alGetString(ALCode));
				AudioContext::Lock();
			}
			AudioContext::Unlock();
#endif
		}
		bool AudioDevice::IsValid()
		{
			return Context && Device;
		}
	}
}