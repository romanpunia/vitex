#ifndef THAWK_AUDIO_H
#define THAWK_AUDIO_H

#include "compute.h"

namespace Tomahawk
{
    namespace Audio
    {
        enum SoundDistanceModel
        {
            SoundDistanceModel_Invalid = 0,
            SoundDistanceModel_Invert = 0xD001,
            SoundDistanceModel_Invert_Clamp = 0xD002,
            SoundDistanceModel_Linear = 0xD003,
            SoundDistanceModel_Linear_Clamp = 0xD004,
            SoundDistanceModel_Exponent = 0xD005,
            SoundDistanceModel_Exponent_Clamp = 0xD006,
        };

        enum SoundEx
        {
            SoundEx_Source_Relative = 0x202,
            SoundEx_Cone_Inner_Angle = 0x1001,
            SoundEx_Cone_Outer_Angle = 0x1002,
            SoundEx_Pitch = 0x1003,
            SoundEx_Position = 0x1004,
            SoundEx_Direction = 0x1005,
            SoundEx_Velocity = 0x1006,
            SoundEx_Looping = 0x1007,
            SoundEx_Buffer = 0x1009,
            SoundEx_Gain = 0x100A,
            SoundEx_Min_Gain = 0x100D,
            SoundEx_Max_Gain = 0x100E,
            SoundEx_Orientation = 0x100F,
            SoundEx_Channel_Mask = 0x3000,
            SoundEx_Source_State = 0x1010,
            SoundEx_Initial = 0x1011,
            SoundEx_Playing = 0x1012,
            SoundEx_Paused = 0x1013,
            SoundEx_Stopped = 0x1014,
            SoundEx_Buffers_Queued = 0x1015,
            SoundEx_Buffers_Processed = 0x1016,
            SoundEx_Seconds_Offset = 0x1024,
            SoundEx_Sample_Offset = 0x1025,
            SoundEx_Byte_Offset = 0x1026,
            SoundEx_Source_Type = 0x1027,
            SoundEx_Static = 0x1028,
            SoundEx_Streaming = 0x1029,
            SoundEx_Undetermined = 0x1030,
            SoundEx_Format_Mono8 = 0x1100,
            SoundEx_Format_Mono16 = 0x1101,
            SoundEx_Format_Stereo8 = 0x1102,
            SoundEx_Format_Stereo16 = 0x1103,
            SoundEx_Reference_Distance = 0x1020,
            SoundEx_Rolloff_Factor = 0x1021,
            SoundEx_Cone_Outer_Gain = 0x1022,
            SoundEx_Max_Distance = 0x1023,
            SoundEx_Frequency = 0x2001,
            SoundEx_Bits = 0x2002,
            SoundEx_Channels = 0x2003,
            SoundEx_Size = 0x2004,
            SoundEx_Unused = 0x2010,
            SoundEx_Pending = 0x2011,
            SoundEx_Processed = 0x2012,
            SoundEx_Invalid_Name = 0xA001,
            SoundEx_Illegal_Enum = 0xA002,
            SoundEx_Invalid_Enum = 0xA002,
            SoundEx_Invalid_Value = 0xA003,
            SoundEx_Illegal_Command = 0xA004,
            SoundEx_Invalid_Operation = 0xA004,
            SoundEx_Out_Of_Memory = 0xA005,
            SoundEx_Vendor = 0xB001,
            SoundEx_Version = 0xB002,
            SoundEx_Renderer = 0xB003,
            SoundEx_Extentions = 0xB004,
            SoundEx_Doppler_Factor = 0xC000,
            SoundEx_Doppler_Velocity = 0xC001,
            SoundEx_Speed_Of_Sound = 0xC003
        };

        class THAWK_OUT AudioContext
        {
        private:
			static std::mutex* Mutex;
            static int State;

        public:
			static void Create();
			static void Release();
			static void Lock();
			static void Unlock();
            static void GenerateBuffers(int Count, unsigned int* Buffers);
            static void SetBufferData(unsigned int Buffer, int Format, const void* Data, int Size, int Frequency);
            static void SetSourceData3F(unsigned int Source, SoundEx Value, float F1, float F2, float F3);
            static void GetSourceData3F(unsigned int Source, SoundEx Value, float* F1, float* F2, float* F3);
            static void SetSourceDataVF(unsigned int Source, SoundEx Value, float* FS);
            static void GetSourceDataVF(unsigned int Source, SoundEx Value, float* FS);
            static void SetSourceData1F(unsigned int Source, SoundEx Value, float F1);
            static void GetSourceData1F(unsigned int Source, SoundEx Value, float* F1);
            static void SetSourceData3I(unsigned int Source, SoundEx Value, int F1, int F2, int F3);
            static void GetSourceData3I(unsigned int Source, SoundEx Value, int* F1, int* F2, int* F3);
            static void SetSourceDataVI(unsigned int Source, SoundEx Value, int* FS);
            static void GetSourceDataVI(unsigned int Source, SoundEx Value, int* FS);
            static void SetSourceData1I(unsigned int Source, SoundEx Value, int F1);
            static void GetSourceData1I(unsigned int Source, SoundEx Value, int* F1);
            static void SetListenerData3F(unsigned int Listener, float F1, float F2, float F3);
            static void GetListenerData3F(unsigned int Listener, float* F1, float* F2, float* F3);
            static void SetListenerDataVF(unsigned int Listener, float* FS);
            static void GetListenerDataVF(unsigned int Listener, float* FS);
            static void SetListenerData1F(unsigned int Listener, float F1);
            static void GetListenerData1F(unsigned int Listener, float* F1);
            static void SetListenerData3I(unsigned int Listener, int F1, int F2, int F3);
            static void GetListenerData3I(unsigned int Listener, int* F1, int* F2, int* F3);
            static void SetListenerDataVI(unsigned int Listener, int* FS);
            static void GetListenerDataVI(unsigned int Listener, int* FS);
            static void SetListenerData1I(unsigned int Listener, int F1);
            static void GetListenerData1I(unsigned int Listener, int* F1);
        };

        class THAWK_OUT AudioClip : public Rest::Object
        {
        private:
            unsigned int Buffer = 0;
            int Format = 0;

        public:
            AudioClip(int BufferCount, int NewFormat);
            virtual ~AudioClip() override;
            float Length();
            bool IsMono();
			unsigned int GetBuffer();
			int GetFormat();
        };

        class THAWK_OUT AudioSource : public Rest::Object
        {
			friend class AudioDevice;

        private:
            AudioClip* Clip = nullptr;
            unsigned int Instance = 0;

        public:
            AudioSource();
            virtual ~AudioSource() override;
            void Apply(AudioClip* Clip);
            void Reset();
            void Pause();
            void Play();
            void Stop();
            bool IsPlaying() const;
			AudioClip* GetClip() const;
			unsigned int GetInstance() const;
        };

        class THAWK_OUT AudioDevice : public Rest::Object
        {
        public:
            void* Context = nullptr;
            void* Device = nullptr;

        public:
            AudioDevice();
            virtual ~AudioDevice() override;
            void Offset(AudioSource* Source, float& Seconds, bool Get);
            void Velocity(AudioSource* Source, Compute::Vector3& Velocity, bool Get);
            void Position(AudioSource* Source, Compute::Vector3& Position, bool Get);
            void Direction(AudioSource* Source, Compute::Vector3& Direction, bool Get);
            void Relative(AudioSource* Source, int& Value, bool Get);
            void Pitch(AudioSource* Source, float& Value, bool Get);
            void Gain(AudioSource* Source, float& Value, bool Get);
            void Loop(AudioSource* Source, int& IsLoop, bool Get);
            void ConeInnerAngle(AudioSource* Source, float& Value, bool Get);
            void ConeOuterAngle(AudioSource* Source, float& Value, bool Get);
            void ConeOuterGain(AudioSource* Source, float& Value, bool Get);
            void Distance(AudioSource* Source, float& Value, bool Get);
            void RefDistance(AudioSource* Source, float& Value, bool Get);
            void SetDistanceModel(SoundDistanceModel Model);
            void GetExceptionCodes(int& ALCCode, int& ALCode);
            bool IsValid();
        };
    }
}
#endif