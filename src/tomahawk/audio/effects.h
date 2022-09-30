#ifndef TH_AUDIO_EFFECTS_H
#define TH_AUDIO_EFFECTS_H
#include "../core/audio.h"

namespace Tomahawk
{
	namespace Audio
	{
		namespace Effects
		{
			class TH_OUT EffectContext
			{
			public:
				static void Create();
			};

			class TH_OUT Reverb final : public AudioEffect
			{
			private:
				bool EAX = false;

			public:
				Compute::Vector3 LateReverbPan;
				Compute::Vector3 ReflectionsPan;
				float Density = 1.0f;
				float Diffusion = 1.0f;
				float Gain = 0.32f;
				float GainHF = 0.89f;
				float GainLF = 1.0f;
				float DecayTime = 1.49f;
				float DecayHFRatio = 0.83f;
				float DecayLFRatio = 1.0f;
				float ReflectionsGain = 0.05f;
				float ReflectionsDelay = 0.007f;
				float LateReverbGain = 1.26f;
				float LateReverbDelay = 0.011f;
				float EchoTime = 0.25f;
				float EchoDepth = 0.0f;
				float ModulationTime = 0.25f;
				float ModulationDepth = 0.0f;
				float AirAbsorptionGainHF = 0.994f;
				float HFReference = 5000.0f;
				float LFReference = 250.0f;
				float RoomRolloffFactor = 0.0f;
				bool IsDecayHFLimited = true;

			public:
				Reverb();
				virtual ~Reverb() override;
				void Synchronize() override;
				void Deserialize(Core::Schema* Node) override;
				void Serialize(Core::Schema* Node) override;
				AudioEffect* Copy() override;

			public:
				TH_COMPONENT("reverb-effect");
			};

			class TH_OUT Chorus final : public AudioEffect
			{
			public:
				float Rate = 1.1f;
				float Depth = 0.1f;
				float Feedback = 0.25f;
				float Delay = 0.016f;
				int Waveform = 1;
				int Phase = 90;

			public:
				Chorus();
				virtual ~Chorus() override;
				void Synchronize() override;
				void Deserialize(Core::Schema* Node) override;
				void Serialize(Core::Schema* Node) override;
				AudioEffect* Copy() override;

			public:
				TH_COMPONENT("chorus-effect");
			};

			class TH_OUT Distortion final : public AudioEffect
			{
			public:
				float Edge = 0.2f;
				float Gain = 0.05f;
				float LowpassCutOff = 8000.0f;
				float EQCenter = 3600.0f;
				float EQBandwidth = 3600.0f;

			public:
				Distortion();
				virtual ~Distortion() override;
				void Synchronize() override;
				void Deserialize(Core::Schema* Node) override;
				void Serialize(Core::Schema* Node) override;
				AudioEffect* Copy() override;

			public:
				TH_COMPONENT("distortion-effect");
			};

			class TH_OUT Echo final : public AudioEffect
			{
			public:
				float Delay = 0.1f;
				float LRDelay = 0.1f;
				float Damping = 0.5f;
				float Feedback = 0.5f;
				float Spread = -1.0f;

			public:
				Echo();
				virtual ~Echo() override;
				void Synchronize() override;
				void Deserialize(Core::Schema* Node) override;
				void Serialize(Core::Schema* Node) override;
				AudioEffect* Copy() override;

			public:
				TH_COMPONENT("echo-effect");
			};

			class TH_OUT Flanger final : public AudioEffect
			{
			public:
				float Rate = 0.27f;
				float Depth = 1.0f;
				float Feedback = -0.5f;
				float Delay = 0.002f;
				int Waveform = 1;
				int Phase = 0;

			public:
				Flanger();
				virtual ~Flanger() override;
				void Synchronize() override;
				void Deserialize(Core::Schema* Node) override;
				void Serialize(Core::Schema* Node) override;
				AudioEffect* Copy() override;

			public:
				TH_COMPONENT("flanger-effect");
			};

			class TH_OUT FrequencyShifter final : public AudioEffect
			{
			public:
				float Frequency = 0.0f;
				int LeftDirection = 0;
				int RightDirection = 0;

			public:
				FrequencyShifter();
				virtual ~FrequencyShifter() override;
				void Synchronize() override;
				void Deserialize(Core::Schema* Node) override;
				void Serialize(Core::Schema* Node) override;
				AudioEffect* Copy() override;

			public:
				TH_COMPONENT("frequency-shifter-effect");
			};

			class TH_OUT VocalMorpher final : public AudioEffect
			{
			public:
				float Rate = 1.41f;
				int Phonemea = 0;
				int PhonemeaCoarseTuning = 0;
				int Phonemeb = 10;
				int PhonemebCoarseTuning = 0;
				int Waveform = 0;

			public:
				VocalMorpher();
				virtual ~VocalMorpher() override;
				void Synchronize() override;
				void Deserialize(Core::Schema* Node) override;
				void Serialize(Core::Schema* Node) override;
				AudioEffect* Copy() override;

			public:
				TH_COMPONENT("vocal-morpher-effect");
			};

			class TH_OUT PitchShifter final : public AudioEffect
			{
			public:
				int CoarseTune = 12;
				int FineTune = 0;

			public:
				PitchShifter();
				virtual ~PitchShifter() override;
				void Synchronize() override;
				void Deserialize(Core::Schema* Node) override;
				void Serialize(Core::Schema* Node) override;
				AudioEffect* Copy() override;

			public:
				TH_COMPONENT("pitch-shifter-effect");
			};

			class TH_OUT RingModulator final : public AudioEffect
			{
			public:
				float Frequency = 440.0f;
				float HighpassCutOff = 800.0f;
				int Waveform = 0;

			public:
				RingModulator();
				virtual ~RingModulator() override;
				void Synchronize() override;
				void Deserialize(Core::Schema* Node) override;
				void Serialize(Core::Schema* Node) override;
				AudioEffect* Copy() override;

			public:
				TH_COMPONENT("ring-modulator-effect");
			};

			class TH_OUT Autowah final : public AudioEffect
			{
			public:
				float AttackTime = 0.06f;
				float ReleaseTime = 0.06f;
				float Resonance = 1000.0f;
				float PeakGain = 11.22f;

			public:
				Autowah();
				virtual ~Autowah() override;
				void Synchronize() override;
				void Deserialize(Core::Schema* Node) override;
				void Serialize(Core::Schema* Node) override;
				AudioEffect* Copy() override;

			public:
				TH_COMPONENT("autowah-effect");
			};

			class TH_OUT Compressor final : public AudioEffect
			{
			public:
				Compressor();
				virtual ~Compressor() override;
				void Synchronize() override;
				void Deserialize(Core::Schema* Node) override;
				void Serialize(Core::Schema* Node) override;
				AudioEffect* Copy() override;

			public:
				TH_COMPONENT("compressor-effect");
			};

			class TH_OUT Equalizer final : public AudioEffect
			{
			public:
				float LowGain = 1.0f;
				float LowCutOff = 200.0f;
				float Mid1Gain = 1.0f;
				float Mid1Center = 500.0f;
				float Mid1Width = 1.0f;
				float Mid2Gain = 1.0f;
				float Mid2Center = 3000.0f;
				float Mid2Width = 1.0f;
				float HighGain = 1.0f;
				float HighCutOff = 6000.0f;

			public:
				Equalizer();
				virtual ~Equalizer() override;
				void Synchronize() override;
				void Deserialize(Core::Schema* Node) override;
				void Serialize(Core::Schema* Node) override;
				AudioEffect* Copy() override;

			public:
				TH_COMPONENT("equalizer-effect");
			};
		}
	}
}
#endif