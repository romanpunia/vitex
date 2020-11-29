#ifndef TH_ENGINE_RENDERERS_H
#define TH_ENGINE_RENDERERS_H

#include "../core/engine.h"
#include "gui.h"

namespace Tomahawk
{
	namespace Engine
	{
		namespace Renderers
		{
			class TH_OUT Model : public GeometryDraw
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
				Model(RenderSystem* Lab);
				virtual ~Model() override;
				void Activate() override;
				void Deactivate() override;
				void CullGeometry(const Viewer& View, Rest::Pool<Drawable*>* Geometry) override;
				void RenderGBuffer(Rest::Timer* Time, Rest::Pool<Drawable*>* Geometry, RenderOpt Options) override;
				void RenderDepthLinear(Rest::Timer* Time, Rest::Pool<Drawable*>* Geometry) override;
				void RenderDepthCubic(Rest::Timer* Time, Rest::Pool<Drawable*>* Geometry, Compute::Matrix4x4* ViewProjection) override;
				
			public:
				TH_COMPONENT("model-renderer");
			};

			class TH_OUT Skin : public GeometryDraw
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
				Skin(RenderSystem* Lab);
				virtual ~Skin();
				void Activate() override;
				void Deactivate() override;
				void CullGeometry(const Viewer& View, Rest::Pool<Drawable*>* Geometry) override;
				void RenderGBuffer(Rest::Timer* Time, Rest::Pool<Drawable*>* Geometry, RenderOpt Options) override;
				void RenderDepthLinear(Rest::Timer* Time, Rest::Pool<Drawable*>* Geometry) override;
				void RenderDepthCubic(Rest::Timer* Time, Rest::Pool<Drawable*>* Geometry, Compute::Matrix4x4* ViewProjection) override;

			public:
				TH_COMPONENT("skin-renderer");
			};

			class TH_OUT SoftBody : public GeometryDraw
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
				SoftBody(RenderSystem* Lab);
				virtual ~SoftBody();
				void Activate() override;
				void Deactivate() override;
				void CullGeometry(const Viewer& View, Rest::Pool<Drawable*>* Geometry) override;
				void RenderGBuffer(Rest::Timer* Time, Rest::Pool<Drawable*>* Geometry, RenderOpt Options) override;
				void RenderDepthLinear(Rest::Timer* Time, Rest::Pool<Drawable*>* Geometry) override;
				void RenderDepthCubic(Rest::Timer* Time, Rest::Pool<Drawable*>* Geometry, Compute::Matrix4x4* ViewProjection) override;

			public:
				TH_COMPONENT("soft-body-renderer");
			};

			class TH_OUT Emitter : public GeometryDraw
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
				Emitter(RenderSystem* Lab);
				virtual ~Emitter() override;
				void Activate() override;
				void Deactivate() override;
				void RenderGBuffer(Rest::Timer* Time, Rest::Pool<Drawable*>* Geometry, RenderOpt Options) override;
				void RenderDepthLinear(Rest::Timer* Time, Rest::Pool<Drawable*>* Geometry) override;
				void RenderDepthCubic(Rest::Timer* Time, Rest::Pool<Drawable*>* Geometry, Compute::Matrix4x4* ViewProjection) override;

			public:
				TH_COMPONENT("emitter-renderer");
			};

			class TH_OUT Decal : public GeometryDraw
			{
			public:
				struct RenderConstant
				{
					Compute::Matrix4x4 ViewProjection;
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
				Decal(RenderSystem* Lab);
				virtual ~Decal() override;
				void Activate() override;
				void Deactivate() override;
				void RenderGBuffer(Rest::Timer* Time, Rest::Pool<Drawable*>* Geometry, RenderOpt Options) override;
				void RenderDepthLinear(Rest::Timer* Time, Rest::Pool<Drawable*>* Geometry) override;
				void RenderDepthCubic(Rest::Timer* Time, Rest::Pool<Drawable*>* Geometry, Compute::Matrix4x4* ViewProjection) override;

			public:
				TH_COMPONENT("decal-renderer");
			};

			class TH_OUT Transparency : public Renderer
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
				Transparency(RenderSystem* Lab);
				virtual ~Transparency();
				void Activate() override;
				void Render(Rest::Timer* Time, RenderState State, RenderOpt Options) override;
				void ResizeBuffers() override;

			public:
				TH_COMPONENT("transparency-renderer");
			};

			class TH_OUT Depth : public TimingDraw
			{
			public:
				struct
				{
					std::vector<Graphics::MultiRenderTargetCube*> PointLight;
					std::vector<Graphics::MultiRenderTarget2D*> SpotLight;
					std::vector<Graphics::MultiRenderTarget2D*> LineLight;
					uint64_t PointLightResolution = 256;
					uint64_t PointLightLimits = 4;
					uint64_t SpotLightResolution = 512;
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
				Depth(RenderSystem* Lab);
				virtual ~Depth();
				void Deserialize(ContentManager* Content, Rest::Document* Node) override;
				void Serialize(ContentManager* Content, Rest::Document* Node) override;
				void Activate() override;
				void Deactivate() override;
				void TickRender(Rest::Timer* Time, RenderState State, RenderOpt Options) override;

			public:
				TH_COMPONENT("depth-renderer");
			};

			class TH_OUT Environment : public Renderer
			{
			private:
				Rest::Pool<Engine::Component*>* ReflectionProbes = nullptr;
				uint64_t Map;

			public:
				Graphics::MultiRenderTarget2D* Surface = nullptr;
				Graphics::Texture2D* Face = nullptr;
				uint64_t Size, MipLevels;

			public:
				Environment(RenderSystem* Lab);
				virtual ~Environment();
				void Deserialize(ContentManager* Content, Rest::Document* Node) override;
				void Serialize(ContentManager* Content, Rest::Document* Node) override;
				void Activate() override;
				void Deactivate() override;
				void Render(Rest::Timer* Time, RenderState State, RenderOpt Options) override;
				void CreateRenderTarget();
				void SetCaptureSize(size_t Size);

			public:
				TH_COMPONENT("environment-renderer");
			};

			class TH_OUT Lighting : public Renderer
			{
			public:
				struct
				{
					Compute::Matrix4x4 WorldViewProjection;
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
					Compute::Matrix4x4 WorldViewProjection;
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
					Compute::Matrix4x4 WorldViewProjection;
					Compute::Matrix4x4 ViewProjection;
					Compute::Vector3 Direction;
					float Cutoff;
					Compute::Vector3 Position;
					float Range;
					Compute::Vector3 Lighting;
					float Softness;
					float Recount;
					float Bias;
					float Iterations;
					float Padding;
				} SpotLight;

				struct
				{
					Compute::Matrix4x4 ViewProjection;
					Compute::Matrix4x4 SkyOffset;
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
					Graphics::Shader* Point[2] = { nullptr };
					Graphics::Shader* Spot[2] = { nullptr };
					Graphics::Shader* Line[2] = { nullptr };
					Graphics::Shader* Probe = nullptr;
					Graphics::Shader* Ambient = nullptr;
				} Shaders;

				struct
				{
					float PointLight = 256;
					float SpotLight = 512;
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
				Lighting(RenderSystem* Lab);
				virtual ~Lighting();
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
				TH_COMPONENT("lighting-renderer");
			};

			class TH_OUT Glitch : public EffectDraw
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
				Glitch(RenderSystem* Lab);
				virtual ~Glitch() = default;
				void Deserialize(ContentManager* Content, Rest::Document* Node) override;
				void Serialize(ContentManager* Content, Rest::Document* Node) override;
				void RenderEffect(Rest::Timer* Time) override;

			public:
				TH_COMPONENT("glitch-renderer");
			};

			class TH_OUT Tone : public EffectDraw
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
				Tone(RenderSystem* Lab);
				virtual ~Tone() = default;
				void Deserialize(ContentManager* Content, Rest::Document* Node) override;
				void Serialize(ContentManager* Content, Rest::Document* Node) override;
				void RenderEffect(Rest::Timer* Time) override;

			public:
				TH_COMPONENT("tone-renderer");
			};

			class TH_OUT DoF : public EffectDraw
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
				DoF(RenderSystem* Lab);
				virtual ~DoF() = default;
				void Deserialize(ContentManager* Content, Rest::Document* Node) override;
				void Serialize(ContentManager* Content, Rest::Document* Node) override;
				void RenderEffect(Rest::Timer* Time) override;
				void FocusAtNearestTarget(float DeltaTime);

			public:
				TH_COMPONENT("dof-renderer");
			};

			class TH_OUT Bloom : public EffectDraw
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
				Bloom(RenderSystem* Lab);
				virtual ~Bloom() = default;
				void Deserialize(ContentManager* Content, Rest::Document* Node) override;
				void Serialize(ContentManager* Content, Rest::Document* Node) override;
				void RenderEffect(Rest::Timer* Time) override;

			public:
				TH_COMPONENT("bloom-renderer");
			};

			class TH_OUT SSR : public EffectDraw
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
				SSR(RenderSystem* Lab);
				virtual ~SSR() = default;
				void Deserialize(ContentManager* Content, Rest::Document* Node) override;
				void Serialize(ContentManager* Content, Rest::Document* Node) override;
				void RenderEffect(Rest::Timer* Time) override;

			public:
				TH_COMPONENT("ssr-renderer");
			};

			class TH_OUT SSAO : public EffectDraw
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
				SSAO(RenderSystem* Lab);
				virtual ~SSAO() = default;
				void Deserialize(ContentManager* Content, Rest::Document* Node) override;
				void Serialize(ContentManager* Content, Rest::Document* Node) override;
				void RenderEffect(Rest::Timer* Time) override;

			public:
				TH_COMPONENT("ssao-renderer");
			};

			class TH_OUT SSDO : public EffectDraw
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
				SSDO(RenderSystem* Lab);
				virtual ~SSDO() = default;
				void Deserialize(ContentManager* Content, Rest::Document* Node) override;
				void Serialize(ContentManager* Content, Rest::Document* Node) override;
				void RenderEffect(Rest::Timer* Time) override;

			public:
				TH_COMPONENT("ssdo-renderer");
			};

			class TH_OUT UserInterface : public Renderer
			{
			private:
				Graphics::Activity* Activity;
				GUI::Context* Context;

			public:
				UserInterface(RenderSystem* Lab);
				UserInterface(RenderSystem* Lab, Graphics::Activity* NewActivity);
				virtual ~UserInterface() override;
				void Render(Rest::Timer* Time, RenderState State, RenderOpt Options) override;
				GUI::Context* GetContext();

			public:
				TH_COMPONENT("user-interface-renderer");
			};
		}
	}
}
#endif