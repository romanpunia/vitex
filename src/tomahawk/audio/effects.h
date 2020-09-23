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
				static void Initialize();
			};

			class TH_OUT ReverbEffect : public AudioEffect
			{
			private:
				bool EAX;

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
				ReverbEffect();
				virtual ~ReverbEffect() override;
				void Synchronize() override;
				void Deserialize(Rest::Document* Node) override;
				void Serialize(Rest::Document* Node) override;
				AudioEffect* Copy() override;

			public:
				TH_COMPONENT(ReverbEffect);
			};

			class TH_OUT ChorusEffect : public AudioEffect
			{
			public:
				float Rate = 1.1f;
				float Depth = 0.1f;
				float Feedback = 0.25f;
				float Delay = 0.016f;
				int Waveform = 1;
				int Phase = 90;

			public:
				ChorusEffect();
				virtual ~ChorusEffect() override;
				void Synchronize() override;
				void Deserialize(Rest::Document* Node) override;
				void Serialize(Rest::Document* Node) override;
				AudioEffect* Copy() override;

			public:
				TH_COMPONENT(ChorusEffect);
			};

			class TH_OUT DistortionEffect : public AudioEffect
			{
			public:
				float Edge = 0.2f;
				float Gain = 0.05f;
				float LowpassCutOff = 8000.0f;
				float EQCenter = 3600.0f;
				float EQBandwidth = 3600.0f;

			public:
				DistortionEffect();
				virtual ~DistortionEffect() override;
				void Synchronize() override;
				void Deserialize(Rest::Document* Node) override;
				void Serialize(Rest::Document* Node) override;
				AudioEffect* Copy() override;

			public:
				TH_COMPONENT(DistortionEffect);
			};

			class TH_OUT EchoEffect : public AudioEffect
			{
			public:
				float Delay = 0.1f;
				float LRDelay = 0.1f;
				float Damping = 0.5f;
				float Feedback = 0.5f;
				float Spread = -1.0f;

			public:
				EchoEffect();
				virtual ~EchoEffect() override;
				void Synchronize() override;
				void Deserialize(Rest::Document* Node) override;
				void Serialize(Rest::Document* Node) override;
				AudioEffect* Copy() override;

			public:
				TH_COMPONENT(EchoEffect);
			};

			class TH_OUT FlangerEffect : public AudioEffect
			{
			public:
				float Rate = 0.27f;
				float Depth = 1.0f;
				float Feedback = -0.5f;
				float Delay = 0.002f;
				int Waveform = 1;
				int Phase = 0;

			public:
				FlangerEffect();
				virtual ~FlangerEffect() override;
				void Synchronize() override;
				void Deserialize(Rest::Document* Node) override;
				void Serialize(Rest::Document* Node) override;
				AudioEffect* Copy() override;

			public:
				TH_COMPONENT(FlangerEffect);
			};

			class TH_OUT FrequencyShifterEffect : public AudioEffect
			{
			public:
				float Frequency = 0.0f;
				int LeftDirection = 0;
				int RightDirection = 0;

			public:
				FrequencyShifterEffect();
				virtual ~FrequencyShifterEffect() override;
				void Synchronize() override;
				void Deserialize(Rest::Document* Node) override;
				void Serialize(Rest::Document* Node) override;
				AudioEffect* Copy() override;

			public:
				TH_COMPONENT(FrequencyShifterEffect);
			};

			class TH_OUT VocalMorpherEffect : public AudioEffect
			{
			public:
				float Rate = 1.41f;
				int Phonemea = 0;
				int PhonemeaCoarseTuning = 0;
				int Phonemeb = 10;
				int PhonemebCoarseTuning = 0;
				int Waveform = 0;

			public:
				VocalMorpherEffect();
				virtual ~VocalMorpherEffect() override;
				void Synchronize() override;
				void Deserialize(Rest::Document* Node) override;
				void Serialize(Rest::Document* Node) override;
				AudioEffect* Copy() override;

			public:
				TH_COMPONENT(VocalMorpherEffect);
			};

			class TH_OUT PitchShifterEffect : public AudioEffect
			{
			public:
				int CoarseTune = 12;
				int FineTune = 0;

			public:
				PitchShifterEffect();
				virtual ~PitchShifterEffect() override;
				void Synchronize() override;
				void Deserialize(Rest::Document* Node) override;
				void Serialize(Rest::Document* Node) override;
				AudioEffect* Copy() override;

			public:
				TH_COMPONENT(PitchShifterEffect);
			};

			class TH_OUT RingModulatorEffect : public AudioEffect
			{
			public:
				float Frequency = 440.0f;
				float HighpassCutOff = 800.0f;
				int Waveform = 0;

			public:
				RingModulatorEffect();
				virtual ~RingModulatorEffect() override;
				void Synchronize() override;
				void Deserialize(Rest::Document* Node) override;
				void Serialize(Rest::Document* Node) override;
				AudioEffect* Copy() override;

			public:
				TH_COMPONENT(RingModulatorEffect);
			};

			class TH_OUT AutowahEffect : public AudioEffect
			{
			public:
				float AttackTime = 0.06f;
				float ReleaseTime = 0.06f;
				float Resonance = 1000.0f;
				float PeakGain = 11.22f;

			public:
				AutowahEffect();
				virtual ~AutowahEffect() override;
				void Synchronize() override;
				void Deserialize(Rest::Document* Node) override;
				void Serialize(Rest::Document* Node) override;
				AudioEffect* Copy() override;

			public:
				TH_COMPONENT(AutowahEffect);
			};

			class TH_OUT CompressorEffect : public AudioEffect
			{
			public:
				CompressorEffect();
				virtual ~CompressorEffect() override;
				void Synchronize() override;
				void Deserialize(Rest::Document* Node) override;
				void Serialize(Rest::Document* Node) override;
				AudioEffect* Copy() override;

			public:
				TH_COMPONENT(CompressorEffect);
			};

			class TH_OUT EqualizerEffect : public AudioEffect
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
				EqualizerEffect();
				virtual ~EqualizerEffect() override;
				void Synchronize() override;
				void Deserialize(Rest::Document* Node) override;
				void Serialize(Rest::Document* Node) override;
				AudioEffect* Copy() override;

			public:
				TH_COMPONENT(EqualizerEffect);
			};
		}
	}
}
#endif