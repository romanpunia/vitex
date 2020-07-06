#ifndef THAWK_ENGINE_RENDERERS_H
#define THAWK_ENGINE_RENDERERS_H

#include "../engine.h"
#include "gui/context.h"

namespace Tomahawk
{
	namespace Engine
	{
		namespace Renderers
		{
			class THAWK_OUT ModelRenderer : public Renderer
			{
			private:
				struct
				{
					Graphics::Shader* GBuffer = nullptr;
					Graphics::Shader* Depth90 = nullptr;
					Graphics::Shader* Depth360 = nullptr;
				} Shaders;

				struct
				{
					Compute::Matrix4x4 FaceViewProjection[6];
				} Depth360;

			private:
				Rest::Pool<Engine::Component*>* Models = nullptr;
				Graphics::DepthStencilState* DepthStencil = nullptr;
				Graphics::RasterizerState* BackRasterizer = nullptr;
				Graphics::RasterizerState* FrontRasterizer = nullptr;
				Graphics::BlendState* Blend = nullptr;
				Graphics::SamplerState* Sampler = nullptr;

			public:
				ModelRenderer(RenderSystem* Lab);
				virtual ~ModelRenderer();
				void OnInitialize() override;
				void OnRender(Rest::Timer* Time) override;
				void OnPhaseRender(Rest::Timer* Time) override;
				void OnDepthRender(Rest::Timer* Time) override;
				void OnCubicDepthRender(Rest::Timer* Time, Compute::Matrix4x4* ViewProjection) override;

			public:
				THAWK_COMPONENT(ModelRenderer);
			};

			class THAWK_OUT SkinModelRenderer : public Renderer
			{
			private:
				struct
				{
					Graphics::Shader* GBuffer = nullptr;
					Graphics::Shader* Depth90 = nullptr;
					Graphics::Shader* Depth360 = nullptr;
				} Shaders;

				struct
				{
					Compute::Matrix4x4 FaceViewProjection[6];
				} Depth360;

			private:
				Rest::Pool<Engine::Component*>* SkinModels = nullptr;
				Graphics::DepthStencilState* DepthStencil = nullptr;
				Graphics::RasterizerState* BackRasterizer = nullptr;
				Graphics::RasterizerState* FrontRasterizer = nullptr;
				Graphics::BlendState* Blend = nullptr;
				Graphics::SamplerState* Sampler = nullptr;

			public:
				SkinModelRenderer(RenderSystem* Lab);
				virtual ~SkinModelRenderer();
				void OnInitialize() override;
				void OnRender(Rest::Timer* Time) override;
				void OnPhaseRender(Rest::Timer* Time) override;
				void OnDepthRender(Rest::Timer* Time) override;
				void OnCubicDepthRender(Rest::Timer* Time, Compute::Matrix4x4* ViewProjection) override;

			public:
				THAWK_COMPONENT(SkinModelRenderer);
			};

			class THAWK_OUT SoftBodyRenderer : public Renderer
			{
			private:
				struct
				{
					Graphics::Shader* GBuffer = nullptr;
					Graphics::Shader* Depth90 = nullptr;
					Graphics::Shader* Depth360 = nullptr;
				} Shaders;

				struct
				{
					Compute::Matrix4x4 FaceViewProjection[6];
				} Depth360;

			private:
				Rest::Pool<Engine::Component*>* SoftBodies = nullptr;
				Graphics::DepthStencilState* DepthStencil = nullptr;
				Graphics::RasterizerState* BackRasterizer = nullptr;
				Graphics::RasterizerState* FrontRasterizer = nullptr;
				Graphics::BlendState* Blend = nullptr;
				Graphics::SamplerState* Sampler = nullptr;
				Graphics::ElementBuffer* VertexBuffer = nullptr;
				Graphics::ElementBuffer* IndexBuffer = nullptr;

			public:
				SoftBodyRenderer(RenderSystem* Lab);
				virtual ~SoftBodyRenderer();
				void OnInitialize() override;
				void OnRender(Rest::Timer* Time) override;
				void OnPhaseRender(Rest::Timer* Time) override;
				void OnDepthRender(Rest::Timer* Time) override;
				void OnCubicDepthRender(Rest::Timer* Time, Compute::Matrix4x4* ViewProjection) override;

			public:
				THAWK_COMPONENT(SoftBodyRenderer);
			};

			class THAWK_OUT ElementSystemRenderer : public Renderer
			{
			private:
				struct
				{
					Graphics::Shader* GBuffer = nullptr;
					Graphics::Shader* Depth90 = nullptr;
					Graphics::Shader* Depth360Q = nullptr;
					Graphics::Shader* Depth360P = nullptr;
				} Shaders;

				struct
				{
					Compute::Matrix4x4 FaceView[6];
				} Depth360;

			private:
				Rest::Pool<Engine::Component*>* ElementSystems = nullptr;
				Graphics::DepthStencilState* DepthStencil = nullptr;
				Graphics::RasterizerState* BackRasterizer = nullptr;
				Graphics::RasterizerState* FrontRasterizer = nullptr;
				Graphics::BlendState* AdditiveBlend = nullptr;
				Graphics::BlendState* OverwriteBlend = nullptr;
				Graphics::SamplerState* Sampler = nullptr;

			public:
				ElementSystemRenderer(RenderSystem* Lab);
				virtual ~ElementSystemRenderer() override;
				void OnInitialize() override;
				void OnRender(Rest::Timer* Time) override;
				void OnPhaseRender(Rest::Timer* Time) override;
				void OnDepthRender(Rest::Timer* Time) override;
				void OnCubicDepthRender(Rest::Timer* Time, Compute::Matrix4x4* ViewProjection) override;

			public:
				THAWK_COMPONENT(ElementSystemRenderer);
			};

			class THAWK_OUT DepthRenderer : public IntervalRenderer
			{
			public:
				struct
				{
					std::vector<Graphics::RenderTargetCube*> PointLight;
					std::vector<Graphics::RenderTarget2D*> SpotLight;
					std::vector<Graphics::RenderTarget2D*> LineLight;
					uint64_t PointLightResolution = 256;
					uint64_t PointLightLimits = 4;
					uint64_t SpotLightResolution = 1024;
					uint64_t SpotLightLimits = 16;
					uint64_t LineLightResolution = 2048;
					uint64_t LineLightLimits = 2;
				} Renderers;

			private:
				Rest::Pool<Engine::Component*>* PointLights = nullptr;
				Rest::Pool<Engine::Component*>* SpotLights = nullptr;
				Rest::Pool<Engine::Component*>* LineLights = nullptr;

			public:
				float ShadowDistance;

			public:
				DepthRenderer(RenderSystem* Lab);
				virtual ~DepthRenderer();
				void OnLoad(ContentManager* Content, Rest::Document* Node) override;
				void OnSave(ContentManager* Content, Rest::Document* Node) override;
				void OnInitialize() override;
				void OnIntervalRender(Rest::Timer* Time) override;

			public:
				THAWK_COMPONENT(DepthRenderer);
			};

			class THAWK_OUT ProbeRenderer : public Renderer
			{
			private:
				Rest::Pool<Engine::Component*>* ProbeLights = nullptr;
				uint64_t Map;

			public:
				Graphics::MultiRenderTarget2D* Surface = nullptr;
				Graphics::Texture2D* Face = nullptr;
				uint64_t Size, MipLevels;

			public:
				ProbeRenderer(RenderSystem* Lab);
				virtual ~ProbeRenderer();
				void OnLoad(ContentManager* Content, Rest::Document* Node) override;
				void OnSave(ContentManager* Content, Rest::Document* Node) override;
				void OnInitialize() override;
				void OnRender(Rest::Timer* Time) override;
				void CreateRenderTarget();
				void SetCaptureSize(size_t Size);

			public:
				THAWK_COMPONENT(ProbeRenderer);
			};

			class THAWK_OUT LightRenderer : public Renderer
			{
			public:
				struct
				{
					Compute::Matrix4x4 OwnWorldViewProjection;
					Compute::Vector3 Position;
					float Range;
					Compute::Vector3 Lighting;
					float MipLevels;
					Compute::Vector3 Scale;
					float Parallax;
				} ProbeLight;

				struct
				{
					Compute::Matrix4x4 OwnWorldViewProjection;
					Compute::Vector3 Position;
					float Range;
					Compute::Vector3 Lighting;
					float Distance;
					float Softness;
					float Recount;
					float Bias;
					float Iterations;
				} PointLight;

				struct
				{
					Compute::Matrix4x4 OwnWorldViewProjection;
					Compute::Matrix4x4 OwnViewProjection;
					Compute::Vector3 Position;
					float Range;
					Compute::Vector3 Lighting;
					float Diffuse;
					float Softness;
					float Recount;
					float Bias;
					float Iterations;
				} SpotLight;

				struct
				{
					Compute::Matrix4x4 OwnViewProjection;
					Compute::Vector3 Position;
					float ShadowDistance;
					Compute::Vector3 Lighting;
					float ShadowLength;
					float Softness;
					float Recount;
					float Bias;
					float Iterations;
				} LineLight;

				struct
				{
					Compute::Vector3 HighEmission = 0.05;
					float Padding1;
					Compute::Vector3 LowEmission = 0.025;
					float Padding2;
				} AmbientLight;

			protected:
				struct
				{
					Graphics::Shader* PointBase = nullptr;
					Graphics::Shader* PointShade = nullptr;
					Graphics::Shader* SpotBase = nullptr;
					Graphics::Shader* SpotShade = nullptr;
					Graphics::Shader* LineBase = nullptr;
					Graphics::Shader* LineShade = nullptr;
					Graphics::Shader* Probe = nullptr;
					Graphics::Shader* Ambient = nullptr;
				} Shaders;

				struct
				{
					float SpotLight = 1024;
					float LineLight = 2048;
				} Quality;

			private:
				Rest::Pool<Engine::Component*>* PointLights = nullptr;
				Rest::Pool<Engine::Component*>* ProbeLights = nullptr;
				Rest::Pool<Engine::Component*>* SpotLights = nullptr;
				Rest::Pool<Engine::Component*>* LineLights = nullptr;
				Graphics::DepthStencilState* DepthStencil = nullptr;
				Graphics::RasterizerState* Rasterizer = nullptr;
				Graphics::BlendState* Blend = nullptr;
				Graphics::SamplerState* Sampler = nullptr;
				Graphics::RenderTarget2D* Output1 = nullptr;
				Graphics::RenderTarget2D* Output2 = nullptr;
				Graphics::RenderTarget2D* Input1 = nullptr;
				Graphics::RenderTarget2D* Input2 = nullptr;

			public:
				bool RecursiveProbes;

			public:
				LightRenderer(RenderSystem* Lab);
				virtual ~LightRenderer();
				void OnLoad(ContentManager* Content, Rest::Document* Node) override;
				void OnSave(ContentManager* Content, Rest::Document* Node) override;
				void OnInitialize() override;
				void OnRender(Rest::Timer* Time) override;
				void OnPhaseRender(Rest::Timer* Time) override;
				void OnResizeBuffers() override;
				void CreateRenderTargets();

			public:
				THAWK_COMPONENT(LightRenderer);
			};

			class THAWK_OUT ImageRenderer : public Renderer
			{
			public:
				Graphics::RenderTarget2D* RenderTarget = nullptr;
				Graphics::DepthStencilState* DepthStencil = nullptr;
				Graphics::RasterizerState* Rasterizer = nullptr;
				Graphics::BlendState* Blend = nullptr;
				Graphics::SamplerState* Sampler = nullptr;

			public:
				ImageRenderer(RenderSystem* Lab);
				ImageRenderer(RenderSystem* Lab, Graphics::RenderTarget2D* Target);
				virtual ~ImageRenderer() override;
				void OnLoad(ContentManager* Content, Rest::Document* Node) override;
				void OnSave(ContentManager* Content, Rest::Document* Node) override;
				void OnRender(Rest::Timer* Time) override;

			public:
				THAWK_COMPONENT(ImageRenderer);
			};

			class THAWK_OUT ReflectionsRenderer : public PostProcessRenderer
			{
			private:
				Graphics::Shader* Pass1;
				Graphics::Shader* Pass2;

			public:
				struct RenderConstant1
				{
					float IterationCount = 24.0f;
					float MipLevels = 0.0f;
					float Intensity = 1.0f;
					float Padding = 0.0f;
				} RenderPass1;

				struct RenderConstant2
				{
					float Texel[2] = { 1.0f, 1.0f };
					float IterationCount = 4.000f;
					float Blur = 4.000f;
				} RenderPass2;

			public:
				ReflectionsRenderer(RenderSystem* Lab);
				virtual ~ReflectionsRenderer() = default;
				void OnLoad(ContentManager* Content, Rest::Document* Node) override;
				void OnSave(ContentManager* Content, Rest::Document* Node) override;
				void OnRenderEffect(Rest::Timer* Time) override;

			public:
				THAWK_COMPONENT(ReflectionsRenderer);
			};

			class THAWK_OUT DepthOfFieldRenderer : public PostProcessRenderer
			{
			public:
				struct RenderConstant
				{
					float Texel[2] = { 1.0f / 512.0f };
					float Threshold = 0.5f;
					float Gain = 2.0f;
					float Fringe = 0.7f;
					float Bias = 0.5f;
					float Dither = 0.0001f;
					float Samples = 3.0f;
					float Rings = 3.0f;
					float FarDistance = 100.0f;
					float FarRange = 10.0f;
					float NearDistance = 1.0f;
					float NearRange = 1.0f;
					float FocalDepth = 1.0f;
					float Intensity = 1.0f;
					float Circular = 1.0f;
				} RenderPass;

			public:
				DepthOfFieldRenderer(RenderSystem* Lab);
				virtual ~DepthOfFieldRenderer() = default;
				void OnLoad(ContentManager* Content, Rest::Document* Node) override;
				void OnSave(ContentManager* Content, Rest::Document* Node) override;
				void OnRenderEffect(Rest::Timer* Time) override;

			public:
				THAWK_COMPONENT(DepthOfFieldRenderer);
			};

			class THAWK_OUT EmissionRenderer : public PostProcessRenderer
			{
			public:
				struct RenderConstant
				{
					float Texel[2] = { 1.0f, 1.0f };
					float IterationCount = 24.0f;
					float Intensity = 1.736f;
					float Threshold = 0.38f;
					float Scale = 0.1f;
					float Padding1 = 0.0f;
					float Padding2 = 0.0f;
				} RenderPass;

			public:
				EmissionRenderer(RenderSystem* Lab);
				virtual ~EmissionRenderer() = default;
				void OnLoad(ContentManager* Content, Rest::Document* Node) override;
				void OnSave(ContentManager* Content, Rest::Document* Node) override;
				void OnRenderEffect(Rest::Timer* Time) override;

			public:
				THAWK_COMPONENT(EmissionRenderer);
			};

			class THAWK_OUT GlitchRenderer : public PostProcessRenderer
			{
			public:
				struct RenderConstant
				{
					float ScanLineJitterDisplacement = 0;
					float ScanLineJitterThreshold = 0;
					float VerticalJumpAmount = 0;
					float VerticalJumpTime = 0;
					float ColorDriftAmount = 0;
					float ColorDriftTime = 0;
					float HorizontalShake = 0;
					float ElapsedTime = 0;
				} RenderPass;

			public:
				float ScanLineJitter;
				float VerticalJump;
				float HorizontalShake;
				float ColorDrift;

			public:
				GlitchRenderer(RenderSystem* Lab);
				virtual ~GlitchRenderer() = default;
				void OnLoad(ContentManager* Content, Rest::Document* Node) override;
				void OnSave(ContentManager* Content, Rest::Document* Node) override;
				void OnRenderEffect(Rest::Timer* Time) override;

			public:
				THAWK_COMPONENT(GlitchRenderer);
			};

			class THAWK_OUT AmbientOcclusionRenderer : public PostProcessRenderer
			{
			private:
				Graphics::Shader* Pass1;
				Graphics::Shader* Pass2;

			public:
				struct RenderConstant1
				{
					float Scale = 0.000f;
					float Intensity = 4.700f;
					float Bias = 0.500f;
					float Radius = 0.010f;
					float Step = 0.012f;
					float Offset = 0.000f;
					float Distance = 3.000f;
					float Fading = 1.965f;
					float Power = 1.200f;
					float IterationCount = 4.000f;
					float Threshold = 0.071f;
					float Padding = 0.0f;
				} RenderPass1;

				struct RenderConstant2
				{
					float Texel[2] = { 1.0f, 1.0f };
					float IterationCount = 4.000f;
					float Blur = 2.000f;
					float Threshold = 0.000f;
					float Power = 1.000f;
					float Discard = 0.000f;
					float Additive = 0.000f;
				} RenderPass2;

			public:
				AmbientOcclusionRenderer(RenderSystem* Lab);
				virtual ~AmbientOcclusionRenderer() = default;
				void OnLoad(ContentManager* Content, Rest::Document* Node) override;
				void OnSave(ContentManager* Content, Rest::Document* Node) override;
				void OnRenderEffect(Rest::Timer* Time) override;

			public:
				THAWK_COMPONENT(AmbientOcclusionRenderer);
			};

			class THAWK_OUT IndirectOcclusionRenderer : public PostProcessRenderer
			{
			private:
				Graphics::Shader* Pass1;
				Graphics::Shader* Pass2;

			public:
				struct RenderConstant1
				{
					float Scale = 0.000f;
					float Intensity = 2.000f;
					float Bias = 0.550f;
					float Radius = 0.055556f;
					float Step = 0.092f;
					float Offset = 0.000f;
					float Distance = 3.839f;
					float Fading = 1.965f;
					float Power = 2.000f;
					float IterationCount = 4.000f;
					float Threshold = 1.000f;
					float Padding = 0.0f;
				} RenderPass1;

				struct RenderConstant2
				{
					float Texel[2] = { 1.0f, 1.0f };
					float IterationCount = 6.000f;
					float Blur = 3.000f;
					float Threshold = 0.000f;
					float Power = 0.600f;
					float Discard = 0.800f;
					float Additive = 1.000f;
				} RenderPass2;

			public:
				IndirectOcclusionRenderer(RenderSystem* Lab);
				virtual ~IndirectOcclusionRenderer() = default;
				void OnLoad(ContentManager* Content, Rest::Document* Node) override;
				void OnSave(ContentManager* Content, Rest::Document* Node) override;
				void OnRenderEffect(Rest::Timer* Time) override;

			public:
				THAWK_COMPONENT(IndirectOcclusionRenderer);
			};

			class THAWK_OUT ToneRenderer : public PostProcessRenderer
			{
			public:
				struct RenderConstant
				{
					Compute::Vector3 BlindVisionR = Compute::Vector3(1, 0, 0);
					float VignetteAmount = 1.0f;
					Compute::Vector3 BlindVisionG = Compute::Vector3(0, 1, 0);
					float VignetteCurve = 1.5f;
					Compute::Vector3 BlindVisionB = Compute::Vector3(0, 0, 1);
					float VignetteRadius = 1.0f;
					Compute::Vector3 VignetteColor;
					float LinearIntensity = 0.25f;
					Compute::Vector3 ColorGamma = Compute::Vector3(1, 1, 1);
					float GammaIntensity = 1.25f;
					Compute::Vector3 DesaturationGamma = Compute::Vector3(0.3f, 0.59f, 0.11f);
					float DesaturationIntensity = 0.5f;
				} RenderPass;

			public:
				ToneRenderer(RenderSystem* Lab);
				virtual ~ToneRenderer() = default;
				void OnLoad(ContentManager* Content, Rest::Document* Node) override;
				void OnSave(ContentManager* Content, Rest::Document* Node) override;
				void OnRenderEffect(Rest::Timer* Time) override;

			public:
				THAWK_COMPONENT(ToneRenderer);
			};

			class THAWK_OUT GUIRenderer : public Renderer
			{
			private:
				GUI::Context* Context;
				
			public:
				Compute::Matrix4x4 Offset;
				bool AA;

			public:
				GUIRenderer(RenderSystem* Lab);
				GUIRenderer(RenderSystem* Lab, Graphics::Activity* NewWindow);
				virtual ~GUIRenderer() override;
				void OnRender(Rest::Timer* Time) override;
				GUI::Context* GetContext();

			public:
				THAWK_COMPONENT(GUIRenderer);
			};
		}
	}
}
#endif