#ifndef TH_ENGINE_RENDERERS_H
#define TH_ENGINE_RENDERERS_H

#include "../core/engine.h"
#include "gui/context.h"

namespace Tomahawk
{
	namespace Engine
	{
		namespace Renderers
		{
			class TH_OUT ModelRenderer : public GeoRenderer
			{
			private:
				struct
				{
					Graphics::Shader* GBuffer = nullptr;
					Graphics::Shader* Linear = nullptr;
					Graphics::Shader* Cubic = nullptr;
					Graphics::Shader* Occlusion = nullptr;
				} Shaders;

				struct
				{
					Compute::Matrix4x4 FaceViewProjection[6];
				} Depth;

			private:
				Graphics::DepthStencilState* DepthStencil = nullptr;
				Graphics::RasterizerState* BackRasterizer = nullptr;
				Graphics::RasterizerState* FrontRasterizer = nullptr;
				Graphics::BlendState* Blend = nullptr;
				Graphics::SamplerState* Sampler = nullptr;
				Graphics::InputLayout* Layout = nullptr;

			public:
				ModelRenderer(RenderSystem* Lab);
				virtual ~ModelRenderer() override;
				void Activate() override;
				void Deactivate() override;
				void CullGeometry(const Viewer& View, Rest::Pool<Component*>* Geometry) override;
				void RenderGBuffer(Rest::Timer* Time, Rest::Pool<Component*>* Geometry, RenderOpt Options) override;
				void RenderDepthLinear(Rest::Timer* Time, Rest::Pool<Component*>* Geometry) override;
				void RenderDepthCubic(Rest::Timer* Time, Rest::Pool<Component*>* Geometry, Compute::Matrix4x4* ViewProjection) override;
				Rest::Pool<Component*>* GetOpaque() override;
				Rest::Pool<Component*>* GetLimpid(uint64_t Index) override;
				
			public:
				TH_COMPONENT(ModelRenderer);
			};

			class TH_OUT SkinRenderer : public GeoRenderer
			{
			private:
				struct
				{
					Graphics::Shader* GBuffer = nullptr;
					Graphics::Shader* Linear = nullptr;
					Graphics::Shader* Cubic = nullptr;
					Graphics::Shader* Occlusion = nullptr;
				} Shaders;

				struct
				{
					Compute::Matrix4x4 FaceViewProjection[6];
				} Depth;

			private:
				Graphics::DepthStencilState* DepthStencil = nullptr;
				Graphics::RasterizerState* BackRasterizer = nullptr;
				Graphics::RasterizerState* FrontRasterizer = nullptr;
				Graphics::BlendState* Blend = nullptr;
				Graphics::SamplerState* Sampler = nullptr;
				Graphics::InputLayout* Layout = nullptr;

			public:
				SkinRenderer(RenderSystem* Lab);
				virtual ~SkinRenderer();
				void Activate() override;
				void Deactivate() override;
				void CullGeometry(const Viewer& View, Rest::Pool<Component*>* Geometry) override;
				void RenderGBuffer(Rest::Timer* Time, Rest::Pool<Component*>* Geometry, RenderOpt Options) override;
				void RenderDepthLinear(Rest::Timer* Time, Rest::Pool<Component*>* Geometry) override;
				void RenderDepthCubic(Rest::Timer* Time, Rest::Pool<Component*>* Geometry, Compute::Matrix4x4* ViewProjection) override;
				Rest::Pool<Component*>* GetOpaque() override;
				Rest::Pool<Component*>* GetLimpid(uint64_t Index) override;

			public:
				TH_COMPONENT(SkinRenderer);
			};

			class TH_OUT SoftBodyRenderer : public GeoRenderer
			{
			private:
				struct
				{
					Graphics::Shader* GBuffer = nullptr;
					Graphics::Shader* Linear = nullptr;
					Graphics::Shader* Cubic = nullptr;
					Graphics::Shader* Occlusion = nullptr;
				} Shaders;

				struct
				{
					Compute::Matrix4x4 FaceViewProjection[6];
				} Depth;

			private:
				Graphics::DepthStencilState* DepthStencil = nullptr;
				Graphics::RasterizerState* BackRasterizer = nullptr;
				Graphics::RasterizerState* FrontRasterizer = nullptr;
				Graphics::BlendState* Blend = nullptr;
				Graphics::SamplerState* Sampler = nullptr;
				Graphics::InputLayout* Layout = nullptr;
				Graphics::ElementBuffer* VertexBuffer = nullptr;
				Graphics::ElementBuffer* IndexBuffer = nullptr;

			public:
				SoftBodyRenderer(RenderSystem* Lab);
				virtual ~SoftBodyRenderer();
				void Activate() override;
				void Deactivate() override;
				void CullGeometry(const Viewer& View, Rest::Pool<Component*>* Geometry) override;
				void RenderGBuffer(Rest::Timer* Time, Rest::Pool<Component*>* Geometry, RenderOpt Options) override;
				void RenderDepthLinear(Rest::Timer* Time, Rest::Pool<Component*>* Geometry) override;
				void RenderDepthCubic(Rest::Timer* Time, Rest::Pool<Component*>* Geometry, Compute::Matrix4x4* ViewProjection) override;
				Rest::Pool<Component*>* GetOpaque() override;
				Rest::Pool<Component*>* GetLimpid(uint64_t Index) override;

			public:
				TH_COMPONENT(SoftBodyRenderer);
			};

			class TH_OUT EmitterRenderer : public GeoRenderer
			{
			private:
				struct
				{
					Graphics::Shader* Opaque = nullptr;
					Graphics::Shader* Limpid = nullptr;
					Graphics::Shader* Linear = nullptr;
					Graphics::Shader* Quad = nullptr;
					Graphics::Shader* Point = nullptr;
				} Shaders;

				struct
				{
					Compute::Matrix4x4 FaceView[6];
				} Depth;

			private:
				Rest::Pool<Engine::Component*>* Opaque = nullptr;
				Rest::Pool<Engine::Component*>* Limpid = nullptr;
				Graphics::DepthStencilState* DepthStencilOpaque = nullptr;
				Graphics::DepthStencilState* DepthStencilLimpid = nullptr;
				Graphics::RasterizerState* BackRasterizer = nullptr;
				Graphics::RasterizerState* FrontRasterizer = nullptr;
				Graphics::BlendState* AdditiveBlend = nullptr;
				Graphics::BlendState* OverwriteBlend = nullptr;
				Graphics::SamplerState* Sampler = nullptr;
				Graphics::InputLayout* Layout = nullptr;

			public:
				EmitterRenderer(RenderSystem* Lab);
				virtual ~EmitterRenderer() override;
				void Activate() override;
				void Deactivate() override;
				void RenderGBuffer(Rest::Timer* Time, Rest::Pool<Component*>* Geometry, RenderOpt Options) override;
				void RenderDepthLinear(Rest::Timer* Time, Rest::Pool<Component*>* Geometry) override;
				void RenderDepthCubic(Rest::Timer* Time, Rest::Pool<Component*>* Geometry, Compute::Matrix4x4* ViewProjection) override;
				Rest::Pool<Component*>* GetOpaque() override;
				Rest::Pool<Component*>* GetLimpid(uint64_t Index) override;

			public:
				TH_COMPONENT(EmitterRenderer);
			};

			class TH_OUT DecalRenderer : public GeoRenderer
			{
			public:
				struct RenderConstant
				{
					Compute::Matrix4x4 OwnViewProjection;
				} RenderPass;

			private:
				Rest::Pool<Engine::Component*>* Opaque = nullptr;
				Rest::Pool<Engine::Component*>* Limpid = nullptr;
				Graphics::DepthStencilState* DepthStencil = nullptr;
				Graphics::RasterizerState* Rasterizer = nullptr;
				Graphics::BlendState* Blend = nullptr;
				Graphics::SamplerState* Sampler = nullptr;
				Graphics::InputLayout* Layout = nullptr;
				Graphics::Shader* Shader = nullptr;

			public:
				DecalRenderer(RenderSystem* Lab);
				virtual ~DecalRenderer() override;
				void Activate() override;
				void Deactivate() override;
				void RenderGBuffer(Rest::Timer* Time, Rest::Pool<Component*>* Geometry, RenderOpt Options) override;
				void RenderDepthLinear(Rest::Timer* Time, Rest::Pool<Component*>* Geometry) override;
				void RenderDepthCubic(Rest::Timer* Time, Rest::Pool<Component*>* Geometry, Compute::Matrix4x4* ViewProjection) override;
				Rest::Pool<Component*>* GetOpaque() override;
				Rest::Pool<Component*>* GetLimpid(uint64_t Index) override;

			public:
				TH_COMPONENT(DecalRenderer);
			};

			class TH_OUT LimpidRenderer : public Renderer
			{
			private:
				Graphics::MultiRenderTarget2D* Surface1 = nullptr;
				Graphics::RenderTarget2D* Input1 = nullptr;
				Graphics::MultiRenderTarget2D* Surface2 = nullptr;
				Graphics::RenderTarget2D* Input2 = nullptr;
				Graphics::DepthStencilState* DepthStencil = nullptr;
				Graphics::RasterizerState* Rasterizer = nullptr;
				Graphics::BlendState* Blend = nullptr;
				Graphics::SamplerState* Sampler = nullptr;
				Graphics::InputLayout* Layout = nullptr;
				Graphics::Shader* Shader = nullptr;
				float MipLevels1 = 0.0f;
				float MipLevels2 = 0.0f;

			public:
				struct RenderConstant
				{
					float MipLevels = 0.0f;
					Compute::Vector3 Padding;
				} RenderPass;

			public:
				LimpidRenderer(RenderSystem* Lab);
				virtual ~LimpidRenderer();
				void Activate() override;
				void Render(Rest::Timer* Time, RenderState State, RenderOpt Options) override;
				void ResizeBuffers() override;

			public:
				TH_COMPONENT(LimpidRenderer);
			};

			class TH_OUT DepthRenderer : public TickRenderer
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
				void Deserialize(ContentManager* Content, Rest::Document* Node) override;
				void Serialize(ContentManager* Content, Rest::Document* Node) override;
				void Activate() override;
				void Deactivate() override;
				void TickRender(Rest::Timer* Time, RenderState State, RenderOpt Options) override;

			public:
				TH_COMPONENT(DepthRenderer);
			};

			class TH_OUT ProbeRenderer : public Renderer
			{
			private:
				Rest::Pool<Engine::Component*>* ReflectionProbes = nullptr;
				uint64_t Map;

			public:
				Graphics::MultiRenderTarget2D* Surface = nullptr;
				Graphics::Texture2D* Face = nullptr;
				uint64_t Size, MipLevels;

			public:
				ProbeRenderer(RenderSystem* Lab);
				virtual ~ProbeRenderer();
				void Deserialize(ContentManager* Content, Rest::Document* Node) override;
				void Serialize(ContentManager* Content, Rest::Document* Node) override;
				void Activate() override;
				void Deactivate() override;
				void Render(Rest::Timer* Time, RenderState State, RenderOpt Options) override;
				void CreateRenderTarget();
				void SetCaptureSize(size_t Size);

			public:
				TH_COMPONENT(ProbeRenderer);
			};

			class TH_OUT LightRenderer : public Renderer
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
					Compute::Vector3 Padding;
					float Infinity;
				} ReflectionProbe;

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
					Compute::Vector3 RlhEmission;
					float RlhHeight;
					Compute::Vector3 MieEmission;
					float MieHeight;
					float ScatterIntensity;
					float PlanetRadius;
					float AtmosphereRadius;
					float MieDirection;
				} LineLight;

				struct
				{
					Compute::Matrix4x4 SkyOffset;
					Compute::Vector3 HighEmission = 0.05f;
					float SkyEmission = 0.0f;
					Compute::Vector3 LowEmission = 0.025f;
					float LightEmission = 1.0f;
					Compute::Vector3 SkyColor = 1.0;
					float Padding = 0.0f;
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
				Rest::Pool<Engine::Component*>* ReflectionProbes = nullptr;
				Rest::Pool<Engine::Component*>* SpotLights = nullptr;
				Rest::Pool<Engine::Component*>* LineLights = nullptr;
				Graphics::DepthStencilState* DepthStencil = nullptr;
				Graphics::RasterizerState* FrontRasterizer = nullptr;
				Graphics::RasterizerState* BackRasterizer = nullptr;
				Graphics::BlendState* Blend = nullptr;
				Graphics::SamplerState* Sampler = nullptr;
				Graphics::InputLayout* Layout = nullptr;
				Graphics::RenderTarget2D* Output1 = nullptr;
				Graphics::RenderTarget2D* Output2 = nullptr;
				Graphics::RenderTarget2D* Input1 = nullptr;
				Graphics::RenderTarget2D* Input2 = nullptr;
				Graphics::Texture2D* SkyBase = nullptr;
				Graphics::TextureCube* SkyMap = nullptr;

			public:
				bool RecursiveProbes;

			public:
				LightRenderer(RenderSystem* Lab);
				virtual ~LightRenderer();
				void Deserialize(ContentManager* Content, Rest::Document* Node) override;
				void Serialize(ContentManager* Content, Rest::Document* Node) override;
				void Activate() override;
				void Deactivate() override;
				void Render(Rest::Timer* Time, RenderState State, RenderOpt Options) override;
				void ResizeBuffers() override;
				void SetSkyMap(Graphics::Texture2D* Cubemap);
				Graphics::TextureCube* GetSkyMap();
				Graphics::Texture2D* GetSkyBase();

			public:
				TH_COMPONENT(LightRenderer);
			};

			class TH_OUT GUIRenderer : public Renderer
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
				void Render(Rest::Timer* Time, RenderState State, RenderOpt Options) override;
				GUI::Context* GetContext();

			public:
				TH_COMPONENT(GUIRenderer);
			};

			class TH_OUT ReflectionsRenderer : public EffectRenderer
			{
			private:
				Graphics::Shader* Pass1;
				Graphics::Shader* Pass2;
				Graphics::Shader* Pass3;

			public:
				struct RenderConstant1
				{
					float Samples = 24.0f;
					float MipLevels = 0.0f;
					float Intensity = 1.0f;
					float Padding = 0.0f;
				} RenderPass1;

				struct RenderConstant2
				{
					float Texel[2] = { 1.0f, 1.0f };
					float Samples = 4.000f;
					float Blur = 4.000f;
				} RenderPass2;

			public:
				ReflectionsRenderer(RenderSystem* Lab);
				virtual ~ReflectionsRenderer() = default;
				void Deserialize(ContentManager* Content, Rest::Document* Node) override;
				void Serialize(ContentManager* Content, Rest::Document* Node) override;
				void RenderEffect(Rest::Timer* Time) override;

			public:
				TH_COMPONENT(ReflectionsRenderer);
			};

			class TH_OUT DepthOfFieldRenderer : public EffectRenderer
			{
			private:
				struct
				{
					Component* Target = nullptr;
					float Radius = 0.0f;
					float Factor = 0.0f;
					float Distance = -1.0f;
					float Range = 0.0f;
				} State;

			public:
				struct RenderConstant
				{
					float Texel[2] = { 1.0f / 512.0f };
					float Radius = 1.0f;
					float Bokeh = 8.0f;
					float Scale = 1.0f;
					float NearDistance = 0.0f;
					float NearRange = 0.0f;
					float FarDistance = 32.0f;
					float FarRange = 2.0f;
					float Padding[3] = { 0.0f };
				} RenderPass;

			public:
				float FocusDistance = -1.0f;
				float FocusRadius = 1.5f;
				float FocusTime = 0.1f;

			public:
				DepthOfFieldRenderer(RenderSystem* Lab);
				virtual ~DepthOfFieldRenderer() = default;
				void Deserialize(ContentManager* Content, Rest::Document* Node) override;
				void Serialize(ContentManager* Content, Rest::Document* Node) override;
				void RenderEffect(Rest::Timer* Time) override;
				void FocusAtNearestTarget(float DeltaTime);

			public:
				TH_COMPONENT(DepthOfFieldRenderer);
			};

			class TH_OUT EmissionRenderer : public EffectRenderer
			{
			private:
				Graphics::Shader* Pass1;
				Graphics::Shader* Pass2;

			public:
				struct RenderConstant
				{
					float Texel[2] = { 1.0f, 1.0f };
					float Samples = 24.0f;
					float Intensity = 1.736f;
					float Threshold = 0.38f;
					float Scale = 0.1f;
					float Padding1 = 0.0f;
					float Padding2 = 0.0f;
				} RenderPass;

			public:
				EmissionRenderer(RenderSystem* Lab);
				virtual ~EmissionRenderer() = default;
				void Deserialize(ContentManager* Content, Rest::Document* Node) override;
				void Serialize(ContentManager* Content, Rest::Document* Node) override;
				void RenderEffect(Rest::Timer* Time) override;

			public:
				TH_COMPONENT(EmissionRenderer);
			};

			class TH_OUT GlitchRenderer : public EffectRenderer
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
				void Deserialize(ContentManager* Content, Rest::Document* Node) override;
				void Serialize(ContentManager* Content, Rest::Document* Node) override;
				void RenderEffect(Rest::Timer* Time) override;

			public:
				TH_COMPONENT(GlitchRenderer);
			};

			class TH_OUT AmbientOcclusionRenderer : public EffectRenderer
			{
			private:
				Graphics::Shader* Pass1;
				Graphics::Shader* Pass2;
				Graphics::Shader* Pass3;

			public:
				struct RenderConstant1
				{
					float Samples = 4.0f;
					float Intensity = 2.25f;
					float Scale = 1.0f;
					float Bias = 0.11f;
					float Radius = 0.0275f;
					float Distance = 3.83f;
					float Fade = 1.96f;
					float Padding = 0.0f;
				} RenderPass1;

				struct RenderConstant2
				{
					float Texel[2] = { 1.0f, 1.0f };
					float Samples = 4.000f;
					float Blur = 2.000f;
					float Power = 1.000f;
					float Additive = 0.000f;
					float Padding[2];
				} RenderPass2;

			public:
				AmbientOcclusionRenderer(RenderSystem* Lab);
				virtual ~AmbientOcclusionRenderer() = default;
				void Deserialize(ContentManager* Content, Rest::Document* Node) override;
				void Serialize(ContentManager* Content, Rest::Document* Node) override;
				void RenderEffect(Rest::Timer* Time) override;

			public:
				TH_COMPONENT(AmbientOcclusionRenderer);
			};

			class TH_OUT DirectOcclusionRenderer : public EffectRenderer
			{
			private:
				Graphics::Shader* Pass1;
				Graphics::Shader* Pass2;
				Graphics::Shader* Pass3;

			public:
				struct RenderConstant1
				{
					float Samples = 3.1f;
					float Intensity = 1.35f;
					float Scale = 0.0f;
					float Bias = -0.55f;
					float Radius = 0.34f;
					float Distance = 4.5f;
					float Fade = 2.54f;
					float Padding = 0.0f;
				} RenderPass1;

				struct RenderConstant2
				{
					float Texel[2] = { 1.0f, 1.0f };
					float Samples = 4.000f;
					float Blur = 2.000f;
					float Power = 1.000f;
					float Additive = 1.000f;
					float Padding[2];
				} RenderPass2;

			public:
				DirectOcclusionRenderer(RenderSystem* Lab);
				virtual ~DirectOcclusionRenderer() = default;
				void Deserialize(ContentManager* Content, Rest::Document* Node) override;
				void Serialize(ContentManager* Content, Rest::Document* Node) override;
				void RenderEffect(Rest::Timer* Time) override;

			public:
				TH_COMPONENT(DirectOcclusionRenderer);
			};

			class TH_OUT ToneRenderer : public EffectRenderer
			{
			public:
				struct RenderConstant
				{
					Compute::Vector3 BlindVisionR = Compute::Vector3(1, 0, 0);
					float VignetteAmount = 0.0f;
					Compute::Vector3 BlindVisionG = Compute::Vector3(0, 1, 0);
					float VignetteCurve = 1.5f;
					Compute::Vector3 BlindVisionB = Compute::Vector3(0, 0, 1);
					float VignetteRadius = 1.0f;
					Compute::Vector3 VignetteColor;
					float LinearIntensity = 0.0f;
					Compute::Vector3 ColorGamma = Compute::Vector3(1, 1, 1);
					float GammaIntensity = 2.2f;
					Compute::Vector3 DesaturationGamma = Compute::Vector3(0.3f, 0.59f, 0.11f);
					float DesaturationIntensity = 0.0f;
					float ToneIntensity = 1.0f;
					float AcesIntensity = 1.0f;
					float AcesA = 3.01f;
					float AcesB = 0.03f;
					float AcesC = 2.43f;
					float AcesD = 0.59f;
					float AcesE = 0.14f;
					float Padding = 0.0f;
				} RenderPass;

			public:
				ToneRenderer(RenderSystem* Lab);
				virtual ~ToneRenderer() = default;
				void Deserialize(ContentManager* Content, Rest::Document* Node) override;
				void Serialize(ContentManager* Content, Rest::Document* Node) override;
				void RenderEffect(Rest::Timer* Time) override;

			public:
				TH_COMPONENT(ToneRenderer);
			};
		}
	}
}
#endif