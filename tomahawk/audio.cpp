#include "audio.h"
#ifdef THAWK_HAS_OPENAL
#include <AL/al.h>
#include <AL/alc.h>
#endif

namespace Tomahawk
{
    namespace Audio
    {
        void AudioContext::GenerateBuffers(int Count, unsigned int* Buffers)
        {
#ifdef THAWK_HAS_OPENAL
            alGenBuffers(Count, Buffers);
#endif
        }
        void AudioContext::SetBufferData(unsigned int Buffer, int Format, const void* Data, int Size, int Frequency)
        {
#ifdef THAWK_HAS_OPENAL
            alBufferData(Buffer, Format, Data, Size, Frequency);
#endif
        }
        void AudioContext::SetSourceData3F(unsigned int Source, SoundEx Value, float F1, float F2, float F3)
        {
#ifdef THAWK_HAS_OPENAL
            alSource3f(Source, Value, F1, F2, F3);
#endif
        }
        void AudioContext::GetSourceData3F(unsigned int Source, SoundEx Value, float* F1, float* F2, float* F3)
        {
#ifdef THAWK_HAS_OPENAL
            alGetSource3f(Source, Value, F1, F2, F3);
#endif
        }
        void AudioContext::SetSourceDataVF(unsigned int Source, SoundEx Value, float* FS)
        {
#ifdef THAWK_HAS_OPENAL
            alSourcefv(Source, Value, FS);
#endif
        }
        void AudioContext::GetSourceDataVF(unsigned int Source, SoundEx Value, float* FS)
        {
#ifdef THAWK_HAS_OPENAL
            alGetSourcefv(Source, Value, FS);
#endif
        }
        void AudioContext::SetSourceData1F(unsigned int Source, SoundEx Value, float F1)
        {
#ifdef THAWK_HAS_OPENAL
            alSourcef(Source, Value, F1);
#endif
        }
        void AudioContext::GetSourceData1F(unsigned int Source, SoundEx Value, float* F1)
        {
#ifdef THAWK_HAS_OPENAL
            alGetSourcef(Source, Value, F1);
#endif
        }
        void AudioContext::SetSourceData3I(unsigned int Source, SoundEx Value, int F1, int F2, int F3)
        {
#ifdef THAWK_HAS_OPENAL
            alSource3i(Source, Value, F1, F2, F3);
#endif
        }
        void AudioContext::GetSourceData3I(unsigned int Source, SoundEx Value, int* F1, int* F2, int* F3)
        {
#ifdef THAWK_HAS_OPENAL
            alGetSource3i(Source, Value, F1, F2, F3);
#endif
        }
        void AudioContext::SetSourceDataVI(unsigned int Source, SoundEx Value, int* FS)
        {
#ifdef THAWK_HAS_OPENAL
            alGetSourceiv(Source, Value, FS);
#endif
        }
        void AudioContext::GetSourceDataVI(unsigned int Source, SoundEx Value, int* FS)
        {
#ifdef THAWK_HAS_OPENAL
            alGetSourceiv(Source, Value, FS);
#endif
        }
        void AudioContext::SetSourceData1I(unsigned int Source, SoundEx Value, int F1)
        {
#ifdef THAWK_HAS_OPENAL
            alSourcei(Source, Value, F1);
#endif
        }
        void AudioContext::GetSourceData1I(unsigned int Source, SoundEx Value, int* F1)
        {
#ifdef THAWK_HAS_OPENAL
            alGetSourcei(Source, Value, F1);
#endif
        }
        void AudioContext::SetListenerData3F(unsigned int Listener, float F1, float F2, float F3)
        {
#ifdef THAWK_HAS_OPENAL
            alListener3f(Listener, F1, F2, F3);
#endif
        }
        void AudioContext::GetListenerData3F(unsigned int Listener, float* F1, float* F2, float* F3)
        {
#ifdef THAWK_HAS_OPENAL
            alGetListener3f(Listener, F1, F2, F3);
#endif
        }
        void AudioContext::SetListenerDataVF(unsigned int Listener, float* FS)
        {
#ifdef THAWK_HAS_OPENAL
            alListenerfv(Listener, FS);
#endif
        }
        void AudioContext::GetListenerDataVF(unsigned int Listener, float* FS)
        {
#ifdef THAWK_HAS_OPENAL
            alGetListenerfv(Listener, FS);
#endif
        }
        void AudioContext::SetListenerData1F(unsigned int Listener, float F1)
        {
#ifdef THAWK_HAS_OPENAL
            alListenerf(Listener, F1);
#endif
        }
        void AudioContext::GetListenerData1F(unsigned int Listener, float* F1)
        {
#ifdef THAWK_HAS_OPENAL
            alGetListenerf(Listener, F1);
#endif
        }
        void AudioContext::SetListenerData3I(unsigned int Listener, int F1, int F2, int F3)
        {
#ifdef THAWK_HAS_OPENAL
            alListener3i(Listener, F1, F2, F3);
#endif
        }
        void AudioContext::GetListenerData3I(unsigned int Listener, int* F1, int* F2, int* F3)
        {
#ifdef THAWK_HAS_OPENAL
            alGetListener3i(Listener, F1, F2, F3);
#endif
        }
        void AudioContext::SetListenerDataVI(unsigned int Listener, int* FS)
        {
#ifdef THAWK_HAS_OPENAL
            alGetListeneriv(Listener, FS);
#endif
        }
        void AudioContext::GetListenerDataVI(unsigned int Listener, int* FS)
        {
#ifdef THAWK_HAS_OPENAL
            alGetListeneriv(Listener, FS);
#endif
        }
        void AudioContext::SetListenerData1I(unsigned int Listener, int F1)
        {
#ifdef THAWK_HAS_OPENAL
            alListeneri(Listener, F1);
#endif
        }
        void AudioContext::GetListenerData1I(unsigned int Listener, int* F1)
        {
#ifdef THAWK_HAS_OPENAL
            alGetListeneri(Listener, F1);
#endif
        }
        int AudioContext::State = 0;

        AudioClip::AudioClip()
        {
#ifdef THAWK_HAS_OPENAL
            Format = AL_FORMAT_MONO8;
#endif
        }
        AudioClip::~AudioClip()
        {
#ifdef THAWK_HAS_OPENAL
            alDeleteBuffers(1, &Buffer);
            Buffer = 0;
#endif
        }
        float AudioClip::Length()
        {
#ifdef THAWK_HAS_OPENAL
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
        bool AudioClip::IsMono()
        {
#ifdef THAWK_HAS_OPENAL
            if (Format == AL_FORMAT_MONO8 || Format == AL_FORMAT_MONO16)
                return true;
#endif
            return false;
        }

        AudioSource::AudioSource()
        {
#ifdef THAWK_HAS_OPENAL
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
#endif
        }
        AudioSource::~AudioSource()
        {
#ifdef THAWK_HAS_OPENAL
            alSourceStop(Instance);
            alSourcei(Instance, AL_BUFFER, 0);
            alDeleteSources(1, &Instance);
            Clip = nullptr;
            Instance = 0;
#endif
        }
        void AudioSource::Apply(AudioClip* In)
        {
            if (!In)
                return;

            Clip = In;
#ifdef THAWK_HAS_OPENAL
            alSourcei(Instance, AL_BUFFER, Clip->Buffer);
#endif
        }
        void AudioSource::Reset()
        {
#ifdef THAWK_HAS_OPENAL
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
#ifdef THAWK_HAS_OPENAL
            alSourcePause(Instance);
#endif
        }
        void AudioSource::Play()
        {
#ifdef THAWK_HAS_OPENAL
            alSourcePlay(Instance);
#endif
        }
        void AudioSource::Stop()
        {
#ifdef THAWK_HAS_OPENAL
            alSourceStop(Instance);
#endif
        }
        bool AudioSource::IsPlaying()
        {
#ifdef THAWK_HAS_OPENAL
            int State = 0;
            alGetSourcei(Instance, AL_SOURCE_STATE, &State);
            return State == AL_PLAYING;
#else
            return false;
#endif
        }

        AudioDevice::AudioDevice()
        {
#ifdef THAWK_HAS_OPENAL
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
#endif
        }
        AudioDevice::~AudioDevice()
        {
#ifdef THAWK_HAS_OPENAL
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
#endif
        }
        void AudioDevice::Offset(AudioSource* Source, float& Seconds, bool Get)
        {
#ifdef THAWK_HAS_OPENAL
            if (!Get)
                alSourcef(Source->Instance, AL_SEC_OFFSET, Seconds);
            else
                alGetSourcef(Source->Instance, AL_SEC_OFFSET, &Seconds);
#endif
        }
        void AudioDevice::Relative(AudioSource* Source, int& Value, bool Get)
        {
#ifdef THAWK_HAS_OPENAL
            if (!Get)
                alSourcei(Source->Instance, AL_SOURCE_RELATIVE, Value);
            else
                alGetSourcei(Source->Instance, AL_SOURCE_RELATIVE, &Value);
#endif
        }
        void AudioDevice::Position(AudioSource* Source, Compute::Vector3& Position, bool Get)
        {
#ifdef THAWK_HAS_OPENAL
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
#ifdef THAWK_HAS_OPENAL
            if (!Get)
                alSource3f(Source->Instance, AL_DIRECTION, Direction.X, Direction.Y, Direction.Z);
            else
                alGetSource3f(Source->Instance, AL_DIRECTION, &Direction.X, &Direction.Y, &Direction.Z);
#endif
        }
        void AudioDevice::Velocity(AudioSource* Source, Compute::Vector3& Velocity, bool Get)
        {
#ifdef THAWK_HAS_OPENAL
            if (!Get)
                alSource3f(Source->Instance, AL_VELOCITY, Velocity.X, Velocity.Y, Velocity.Z);
            else
                alGetSource3f(Source->Instance, AL_VELOCITY, &Velocity.X, &Velocity.Y, &Velocity.Z);
#endif
        }
        void AudioDevice::Pitch(AudioSource* Source, float& Value, bool Get)
        {
#ifdef THAWK_HAS_OPENAL
            if (!Get)
                alSourcef(Source->Instance, AL_PITCH, Value);
            else
                alGetSourcef(Source->Instance, AL_PITCH, &Value);
#endif
        }
        void AudioDevice::Gain(AudioSource* Source, float& Value, bool Get)
        {
#ifdef THAWK_HAS_OPENAL
            if (!Get)
                alSourcef(Source->Instance, AL_GAIN, Value);
            else
                alGetSourcef(Source->Instance, AL_GAIN, &Value);
#endif
        }
        void AudioDevice::ConeInnerAngle(AudioSource* Source, float& Value, bool Get)
        {
#ifdef THAWK_HAS_OPENAL
            if (!Get)
                alSourcef(Source->Instance, AL_CONE_INNER_ANGLE, Value);
            else
                alGetSourcef(Source->Instance, AL_CONE_INNER_ANGLE, &Value);
#endif
        }
        void AudioDevice::ConeOuterAngle(AudioSource* Source, float& Value, bool Get)
        {
#ifdef THAWK_HAS_OPENAL
            if (!Get)
                alSourcef(Source->Instance, AL_CONE_OUTER_ANGLE, Value);
            else
                alGetSourcef(Source->Instance, AL_CONE_OUTER_ANGLE, &Value);
#endif
        }
        void AudioDevice::ConeOuterGain(AudioSource* Source, float& Value, bool Get)
        {
#ifdef THAWK_HAS_OPENAL
            if (!Get)
                alSourcef(Source->Instance, AL_CONE_OUTER_GAIN, Value);
            else
                alGetSourcef(Source->Instance, AL_CONE_OUTER_GAIN, &Value);
#endif
        }
        void AudioDevice::Distance(AudioSource* Source, float& Value, bool Get)
        {
#ifdef THAWK_HAS_OPENAL
            if (!Get)
                alSourcef(Source->Instance, AL_MAX_DISTANCE, Value);
            else
                alGetSourcef(Source->Instance, AL_MAX_DISTANCE, &Value);
#endif
        }
        void AudioDevice::RefDistance(AudioSource* Source, float& Value, bool Get)
        {
#ifdef THAWK_HAS_OPENAL
            if (!Get)
                alSourcef(Source->Instance, AL_REFERENCE_DISTANCE, Value);
            else
                alGetSourcef(Source->Instance, AL_REFERENCE_DISTANCE, &Value);
#endif
        }
        void AudioDevice::Loop(AudioSource* Source, int& IsLoop, bool Get)
        {
#ifdef THAWK_HAS_OPENAL
            if (!Get)
                alSourcei(Source->Instance, AL_LOOPING, IsLoop);
            else
                alGetSourcei(Source->Instance, AL_LOOPING, &IsLoop);
#endif
        }
        void AudioDevice::SetDistanceModel(SoundDistanceModel Model)
        {
#ifdef THAWK_HAS_OPENAL
            alDistanceModel((int)Model);
#endif
        }
        void AudioDevice::GetExceptionCodes(int& ALCCode, int& ALCode)
        {
#ifdef THAWK_HAS_OPENAL
            if ((ALCCode = alcGetError((ALCdevice*)Device)) != ALC_NO_ERROR)
                THAWK_ERROR(alcGetString((ALCdevice*)Device, ALCCode));

            if ((ALCode = alGetError()) != AL_NO_ERROR)
                THAWK_ERROR(alGetString(ALCode));
#endif
        }
        bool AudioDevice::IsValid()
        {
            return Context && Device;
        }
    }
}