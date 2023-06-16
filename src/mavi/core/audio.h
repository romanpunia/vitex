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

		enum class FilterEx
		{
			Lowpass_Gain = 0x0001,
			Lowpass_Gain_HF = 0x0002,
			Highpass_Gain = 0x0001,
			Highpass_Gain_LF = 0x0002,
			Bandpass_Gain = 0x0001,
			Bandpass_Gain_LF = 0x0002,
			Bandpass_Gain_HF = 0x0003,
			Filter_Type = 0x8001,
			Filter_Null = 0x0000,
			Filter_Lowpass = 0x0001,
			Filter_Highpass = 0x0002,
			Filter_Bandpass = 0x0003
		};

		enum class EffectEx
		{
			Reverb_Density = 0x0001,
			Reverb_Diffusion = 0x0002,
			Reverb_Gain = 0x0003,
			Reverb_Gain_HF = 0x0004,
			Reverb_Decay_Time = 0x0005,
			Reverb_Decay_HF_Ratio = 0x0006,
			Reverb_Reflections_Gain = 0x0007,
			Reverb_Reflections_Delay = 0x0008,
			Reverb_Late_Reverb_Gain = 0x0009,
			Reverb_Late_Reverb_Delay = 0x000A,
			Reverb_Air_Absorption_Gain_HF = 0x000B,
			Reverb_Room_Rolloff_Factor = 0x000C,
			Reverb_Decay_HF_Limit = 0x000D,
			EAXReverb_Density = 0x0001,
			EAXReverb_Diffusion = 0x0002,
			EAXReverb_Gain = 0x0003,
			EAXReverb_Gain_HF = 0x0004,
			EAXReverb_Gain_LF = 0x0005,
			EAXReverb_Decay_Time = 0x0006,
			EAXReverb_Decay_HF_Ratio = 0x0007,
			EAXReverb_Decay_LF_Ratio = 0x0008,
			EAXReverb_Reflections_Gain = 0x0009,
			EAXReverb_Reflections_Delay = 0x000A,
			EAXReverb_Reflections_Pan = 0x000B,
			EAXReverb_Late_Reverb_Gain = 0x000C,
			EAXReverb_Late_Reverb_Delay = 0x000D,
			EAXReverb_Late_Reverb_Pan = 0x000E,
			EAXReverb_Echo_Time = 0x000F,
			EAXReverb_Echo_Depth = 0x0010,
			EAXReverb_Modulation_Time = 0x0011,
			EAXReverb_Modulation_Depth = 0x0012,
			EAXReverb_Air_Absorption_Gain_HF = 0x0013,
			EAXReverb_HF_Reference = 0x0014,
			EAXReverb_LF_Reference = 0x0015,
			EAXReverb_Room_Rolloff_Factor = 0x0016,
			EAXReverb_Decay_HF_Limit = 0x0017,
			Chorus_Waveform = 0x0001,
			Chorus_Phase = 0x0002,
			Chorus_Rate = 0x0003,
			Chorus_Depth = 0x0004,
			Chorus_Feedback = 0x0005,
			Chorus_Delay = 0x0006,
			Distortion_Edge = 0x0001,
			Distortion_Gain = 0x0002,
			Distortion_Lowpass_Cutoff = 0x0003,
			Distortion_EQ_Center = 0x0004,
			Distortion_EQ_Bandwidth = 0x0005,
			Echo_Delay = 0x0001,
			Echo_LR_Delay = 0x0002,
			Echo_Damping = 0x0003,
			Echo_Feedback = 0x0004,
			Echo_Spread = 0x0005,
			Flanger_Waveform = 0x0001,
			Flanger_Phase = 0x0002,
			Flanger_Rate = 0x0003,
			Flanger_Depth = 0x0004,
			Flanger_Feedback = 0x0005,
			Flanger_Delay = 0x0006,
			Frequency_Shifter_Frequency = 0x0001,
			Frequency_Shifter_Left_Direction = 0x0002,
			Frequency_Shifter_Right_Direction = 0x0003,
			Vocmorpher_Phoneme_A = 0x0001,
			Vocmorpher_Phoneme_A_Coarse_Tuning = 0x0002,
			Vocmorpher_Phoneme_B = 0x0003,
			Vocmorpher_Phoneme_B_Coarse_Tuning = 0x0004,
			Vocmorpher_Waveform = 0x0005,
			Vocmorpher_Rate = 0x0006,
			Pitch_Shifter_Coarse_Tune = 0x0001,
			Pitch_Shifter_Fine_Tune = 0x0002,
			Ring_Modulator_Frequency = 0x0001,
			Ring_Modulator_Highpass_Cutoff = 0x0002,
			Ring_Modulator_Waveform = 0x0003,
			Autowah_Attack_Time = 0x0001,
			Autowah_Release_Time = 0x0002,
			Autowah_Resonance = 0x0003,
			Autowah_Peak_Gain = 0x0004,
			Compressor_ON_OFF = 0x0001,
			Equalizer_LOW_Gain = 0x0001,
			Equalizer_LOW_Cutoff = 0x0002,
			Equalizer_MID1_Gain = 0x0003,
			Equalizer_MID1_Center = 0x0004,
			Equalizer_MID1_Width = 0x0005,
			Equalizer_MID2_Gain = 0x0006,
			Equalizer_MID2_Center = 0x0007,
			Equalizer_MID2_Width = 0x0008,
			Equalizer_HIGH_Gain = 0x0009,
			Equalizer_HIGH_Cutoff = 0x000A,
			Effect_First_Parameter = 0x0000,
			Effect_Last_Parameter = 0x8000,
			Effect_Type = 0x8001,
			Effect_Null = 0x0000,
			Effect_Reverb = 0x0001,
			Effect_Chorus = 0x0002,
			Effect_Distortion = 0x0003,
			Effect_Echo = 0x0004,
			Effect_Flanger = 0x0005,
			Effect_Frequency_Shifter = 0x0006,
			Effect_Vocmorpher = 0x0007,
			Effect_Pitch_Shifter = 0x0008,
			Effect_Ring_Modulator = 0x0009,
			Effect_Autowah = 0x000A,
			Effect_Compressor = 0x000B,
			Effect_Equalizer = 0x000C,
			Effect_EAXReverb = 0x8000,
			Effect_Slot_Effect = 0x0001,
			Effect_Slot_Gain = 0x0002,
			Effect_Slot_Auxiliary_Send_Auto = 0x0003,
			Effect_Slot_Null = 0x0000
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
			static void SetFilter1I(unsigned int Filter, FilterEx Value, int F1);
			static void SetFilter1F(unsigned int Filter, FilterEx Value, float F1);
			static void SetEffect1I(unsigned int Effect, EffectEx Value, int F1);
			static void SetEffect1F(unsigned int Effect, EffectEx Value, float F1);
			static void SetEffectVF(unsigned int Effect, EffectEx Value, float* FS);
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
			static uint32_t GetEnumValue(const char* Name);
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
