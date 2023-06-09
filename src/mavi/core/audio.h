#ifndef VI_AUDIO_H
#define VI_AUDIO_H
#include "compute.h"

namespace Mavi
{
	namespace Audio
	{
		class AudioSource;

		class AudioEffect;

		enum class SoundDistanceModel
		{
			Invalid = 0,
			Invert = 0xD001,
			Invert_Clamp = 0xD002,
			Linear = 0xD003,
			Linear_Clamp = 0xD004,
			Exponent = 0xD005,
			Exponent_Clamp = 0xD006,
		};

		enum class SoundEx
		{
			Source_Relative = 0x202,
			Cone_Inner_Angle = 0x1001,
			Cone_Outer_Angle = 0x1002,
			Pitch = 0x1003,
			Position = 0x1004,
			Direction = 0x1005,
			Velocity = 0x1006,
			Looping = 0x1007,
			Buffer = 0x1009,
			Gain = 0x100A,
			Min_Gain = 0x100D,
			Max_Gain = 0x100E,
			Orientation = 0x100F,
			Channel_Mask = 0x3000,
			Source_State = 0x1010,
			Initial = 0x1011,
			Playing = 0x1012,
			Paused = 0x1013,
			Stopped = 0x1014,
			Buffers_Queued = 0x1015,
			Buffers_Processed = 0x1016,
			Seconds_Offset = 0x1024,
			Sample_Offset = 0x1025,
			Byte_Offset = 0x1026,
			Source_Type = 0x1027,
			Static = 0x1028,
			Streaming = 0x1029,
			Undetermined = 0x1030,
			Format_Mono8 = 0x1100,
			Format_Mono16 = 0x1101,
			Format_Stereo8 = 0x1102,
			Format_Stereo16 = 0x1103,
			Reference_Distance = 0x1020,
			Rolloff_Factor = 0x1021,
			Cone_Outer_Gain = 0x1022,
			Max_Distance = 0x1023,
			Frequency = 0x2001,
			Bits = 0x2002,
			Channels = 0x2003,
			Size = 0x2004,
			Unused = 0x2010,
			Pending = 0x2011,
			Processed = 0x2012,
			Invalid_Name = 0xA001,
			Illegal_Enum = 0xA002,
			Invalid_Enum = 0xA002,
			Invalid_Value = 0xA003,
			Illegal_Command = 0xA004,
			Invalid_Operation = 0xA004,
			Out_Of_Memory = 0xA005,
			Vendor = 0xB001,
			Version = 0xB002,
			Renderer = 0xB003,
			Extentions = 0xB004,
			Doppler_Factor = 0xC000,
			Doppler_Velocity = 0xC001,
			Speed_Of_Sound = 0xC003
		};

		struct VI_OUT AudioSync
		{
			Compute::Vector3 Direction;
			Compute::Vector3 Velocity;
			float ConeInnerAngle = 360.0f;
			float ConeOuterAngle = 360.0f;
			float ConeOuterGain = 0.0f;
			float Pitch = 1.0f;
			float Gain = 1.0f;
			float RefDistance = 0.0f;
			float Distance = 10.0f;
			float Rolloff = 1.0f;
			float Position = 0.0f;
			float AirAbsorption = 0.0f;
			float RoomRollOff = 0.0f;
			bool IsRelative = false;
			bool IsLooped = false;
		};

		class VI_OUT AudioContext final : public Core::Singletonish
		{
		public:
			static void Initialize();
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
			static void SetListenerData3F(SoundEx Listener, float F1, float F2, float F3);
			static void GetListenerData3F(SoundEx Listener, float* F1, float* F2, float* F3);
			static void SetListenerDataVF(SoundEx Listener, float* FS);
			static void GetListenerDataVF(SoundEx Listener, float* FS);
			static void SetListenerData1F(SoundEx Listener, float F1);
			static void GetListenerData1F(SoundEx Listener, float* F1);
			static void SetListenerData3I(SoundEx Listener, int F1, int F2, int F3);
			static void GetListenerData3I(SoundEx Listener, int* F1, int* F2, int* F3);
			static void SetListenerDataVI(SoundEx Listener, int* FS);
			static void GetListenerDataVI(SoundEx Listener, int* FS);
			static void SetListenerData1I(SoundEx Listener, int F1);
			static void GetListenerData1I(SoundEx Listener, int* F1);
		};

		class VI_OUT AudioFilter : public Core::Reference<AudioFilter>
		{
			friend AudioEffect;
			friend AudioSource;

		protected:
			AudioSource* Source = nullptr;
			unsigned int Filter = 0;

		public:
			AudioFilter() noexcept;
			virtual ~AudioFilter() noexcept;
			virtual void Synchronize() = 0;
			virtual void Deserialize(Core::Schema* Node) = 0;
			virtual void Serialize(Core::Schema* Node) const = 0;
			virtual Core::Unique<AudioFilter> Copy() const = 0;
			AudioSource* GetSource() const;

		protected:
			bool CreateLocked(const std::function<bool()>& Callback);

		public:
			VI_COMPONENT_ROOT("base_audio_filter");
		};

		class VI_OUT AudioEffect : public Core::Reference<AudioEffect>
		{
			friend AudioSource;

		private:
			int Zone = -1;

		protected:
			AudioSource* Source = nullptr;
			AudioFilter* Filter = nullptr;
			unsigned int Effect = 0;
			unsigned int Slot = 0;

		public:
			AudioEffect() noexcept;
			virtual ~AudioEffect() noexcept;
			virtual void Synchronize() = 0;
			virtual void Deserialize(Core::Schema* Node) = 0;
			virtual void Serialize(Core::Schema* Node) const = 0;
			virtual Core::Unique<AudioEffect> Copy() const = 0;
			bool SetFilter(AudioFilter** Filter);
			AudioFilter* GetFilter() const;
			AudioSource* GetSource() const;

		protected:
			bool CreateLocked(const std::function<bool()>& Callback);

		private:
			bool Bind(AudioSource* NewSource, int NewZone);
			bool Unbind();

		public:
			VI_COMPONENT_ROOT("base_audio_effect");
		};

		class VI_OUT AudioClip final : public Core::Reference<AudioClip>
		{
		private:
			unsigned int Buffer = 0;
			int Format = 0;

		public:
			AudioClip(int BufferCount, int NewFormat) noexcept;
			~AudioClip() noexcept;
			float Length() const;
			bool IsMono() const;
			unsigned int GetBuffer() const;
			int GetFormat() const;
		};

		class VI_OUT AudioSource final : public Core::Reference<AudioSource>
		{
			friend class AudioDevice;

		private:
			Core::Vector<AudioEffect*> Effects;
			AudioClip* Clip = nullptr;
			unsigned int Instance = 0;

		public:
			AudioSource() noexcept;
			~AudioSource() noexcept;
			int64_t AddEffect(AudioEffect* Effect);
			bool RemoveEffect(size_t EffectId);
			bool RemoveEffectById(uint64_t EffectId);
			void RemoveEffects();
			void SetClip(AudioClip* Clip);
			void Synchronize(AudioSync* Sync, const Compute::Vector3& Position);
			void Reset();
			void Pause();
			void Play();
			void Stop();
			bool IsPlaying() const;
			size_t GetEffectsCount() const;
			AudioClip* GetClip() const;
			AudioEffect* GetEffect(uint64_t Section) const;
			unsigned int GetInstance() const;
			const Core::Vector<AudioEffect*>& GetEffects() const;

		public:
			template <typename T>
			T* GetEffect()
			{
				return (T*)GetEffect(T::GetTypeId());
			}
		};

		class VI_OUT AudioDevice final : public Core::Reference<AudioDevice>
		{
		public:
			void* Context = nullptr;
			void* Device = nullptr;

		public:
			AudioDevice() noexcept;
			~AudioDevice() noexcept;
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
			void GetExceptionCodes(int& ALCCode, int& ALCode) const;
			bool IsValid() const;
		};
	}
}
#endif
