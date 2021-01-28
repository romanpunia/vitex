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
					struct
					{
						Graphics::Shader* Linear = nullptr;
						Graphics::Shader* Cubic = nullptr;
					} Depth;

					Graphics::Shader* Geometry = nullptr;
					Graphics::Shader* Voxelize = nullptr;
					Graphics::Shader* Occlusion = nullptr;
				} Shaders;

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
				void RenderGeometryResult(Rest::Timer* Time, Rest::Pool<Drawable*>* Geometry, RenderOpt Options) override;
				void RenderGeometryVoxels(Rest::Timer* Time, Rest::Pool<Drawable*>* Geometry, RenderOpt Options) override;
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
					struct
					{
						Graphics::Shader* Linear = nullptr;
						Graphics::Shader* Cubic = nullptr;
					} Depth;

					Graphics::Shader* Geometry = nullptr;
					Graphics::Shader* Voxelize = nullptr;
					Graphics::Shader* Occlusion = nullptr;
				} Shaders;

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
				void RenderGeometryResult(Rest::Timer* Time, Rest::Pool<Drawable*>* Geometry, RenderOpt Options) override;
				void RenderGeometryVoxels(Rest::Timer* Time, Rest::Pool<Drawable*>* Geometry, RenderOpt Options) override;
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
					struct
					{
						Graphics::Shader* Linear = nullptr;
						Graphics::Shader* Cubic = nullptr;
					} Depth;

					Graphics::Shader* Geometry = nullptr;
					Graphics::Shader* Voxelize = nullptr;
					Graphics::Shader* Occlusion = nullptr;
				} Shaders;

			private:
				Graphics::DepthStencilState* DepthStencil = nullptr;
				Graphics::RasterizerState* Rasterizer = nullptr;
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
				void RenderGeometryResult(Rest::Timer* Time, Rest::Pool<Drawable*>* Geometry, RenderOpt Options) override;
				void RenderGeometryVoxels(Rest::Timer* Time, Rest::Pool<Drawable*>* Geometry, RenderOpt Options) override;
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
					struct
					{
						Graphics::Shader* Linear = nullptr;
						Graphics::Shader* Quad = nullptr;
						Graphics::Shader* Point = nullptr;
					} Depth;

					Graphics::Shader* Opaque = nullptr;
					Graphics::Shader* Transparency = nullptr;
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
				void RenderGeometryResult(Rest::Timer* Time, Rest::Pool<Drawable*>* Geometry, RenderOpt Options) override;
				void RenderGeometryVoxels(Rest::Timer* Time, Rest::Pool<Drawable*>* Geometry, RenderOpt Options) override;
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
				void RenderGeometryResult(Rest::Timer* Time, Rest::Pool<Drawable*>* Geometry, RenderOpt Options) override;
				void RenderGeometryVoxels(Rest::Timer* Time, Rest::Pool<Drawable*>* Geometry, RenderOpt Options) override;
				void RenderDepthLinear(Rest::Timer* Time, Rest::Pool<Drawable*>* Geometry) override;
				void RenderDepthCubic(Rest::Timer* Time, Rest::Pool<Drawable*>* Geometry, Compute::Matrix4x4* ViewProjection) override;

			public:
				TH_COMPONENT("decal-renderer");
			};

			class TH_OUT Lighting : public Renderer
			{
			private:
				struct
				{
					Compute::Matrix4x4 WorldViewProjection;
					Compute::Vector3 Position;
					float Range = 0.0f;
					Compute::Vector3 Lighting;
					float MipLevels = 0.0f;
					Compute::Vector3 Scale;
					float Parallax = 0.0f;
					Compute::Vector3 Attenuation;
					float Infinity = 0.0f;
				} SurfaceLight;

				struct
				{
					Compute::Matrix4x4 WorldViewProjection;
					Compute::Vector4 Attenuation;
					Compute::Vector3 Position;
					float Range = 0.0f;
					Compute::Vector3 Lighting;
					float Distance = 0.0f;
					float Umbra = 0.0f;
					float Softness = 0.0f;
					float Bias = 0.0f;
					float Iterations = 0.0f;
				} PointLight;

				struct
				{
					Compute::Matrix4x4 WorldViewProjection;
					Compute::Matrix4x4 ViewProjection;
					Compute::Vector4 Attenuation;
					Compute::Vector3 Direction;
					float Cutoff = 0.0f;
					Compute::Vector3 Position;
					float Range = 0.0f;
					Compute::Vector3 Lighting;
					float Softness = 0.0f;
					float Bias = 0.0f;
					float Iterations = 0.0f;
					float Umbra = 0.0f;
					float Padding = 0.0f;
				} SpotLight;

				struct
				{
					Compute::Matrix4x4 ViewProjection[6];
					Compute::Matrix4x4 SkyOffset;
					Compute::Vector3 RlhEmission;
					float RlhHeight = 0.0f;
					Compute::Vector3 MieEmission;
					float MieHeight = 0.0f;
					Compute::Vector3 Lighting;
					float Softness = 0.0f;
					Compute::Vector3 Position;
					float Cascades = 0.0f;
					float Bias = 0.0f;
					float Iterations = 0.0f;
					float ScatterIntensity = 0.0f;
					float PlanetRadius = 0.0f;
					float AtmosphereRadius = 0.0f;
					float MieDirection = 0.0f;
					float Umbra = 0.0f;
					float Padding = 0.0f;
				} LineLight;

			public:
				struct VoxelBuffer
				{
					Compute::Vector3 Center;
					float RayStep = 1.0f;
					Compute::Vector3 Size;
					float MipLevels = 8.0f;
					Compute::Vector3 Scale;
					float MaxSteps = 32.0f;
					float Intensity = 3.0f;
					float Occlusion = 0.9f;
					float Shadows = 0.25f;
					float Padding = 0.0f;
				} Voxelizer;

				struct
				{
					Graphics::Texture3D* LightBuffer = nullptr;
					Graphics::Texture3D* DiffuseBuffer = nullptr;
					Graphics::Texture3D* NormalBuffer = nullptr;
					Graphics::Texture3D* SurfaceBuffer = nullptr;
					Compute::Vector3 Distance = 10.0f;
					Rest::TickTimer Tick;
					uint64_t Size = 128;
					bool Enabled = false;
				} GI;

				struct
				{
					Compute::Matrix4x4 SkyOffset;
					Compute::Vector3 HighEmission = 0.028f;
					float SkyEmission = 0.0f;
					Compute::Vector3 LowEmission = 0.016f;
					float LightEmission = 1.0f;
					Compute::Vector3 SkyColor = 1.0f;
					float FogFarOff = 0.1f;
					Compute::Vector3 FogColor = { 0.5f, 0.6f, 0.7f };
					float FogNearOff = 0.1f;
					Compute::Vector3 FogFar = 0.125f;
					float FogAmount = 0.0f;
					Compute::Vector3 FogNear = 0.125f;
					float Recursive = 1.0f;
				} Ambient;

				struct
				{
					std::vector<Graphics::RenderTargetCube*> PointLight;
					std::vector<Graphics::RenderTarget2D*> SpotLight;
					std::vector<CascadedDepthMap*> LineLight;
					uint64_t PointLightResolution = 256;
					uint64_t PointLightLimits = 4;
					uint64_t SpotLightResolution = 512;
					uint64_t SpotLightLimits = 8;
					uint64_t LineLightResolution = 1024;
					uint64_t LineLightLimits = 2;
					Rest::TickTimer Tick;
					float Distance = 0.5f;
				} Shadows;

				struct
				{
					Graphics::MultiRenderTarget2D* Merger = nullptr;
					Graphics::RenderTarget2D* Output = nullptr;
					Graphics::RenderTarget2D* Input = nullptr;
					Graphics::Cubemap* Subresource = nullptr;
					Graphics::Texture2D* Face = nullptr;
					uint64_t Size = 128;
				} Surfaces;

			protected:
				struct
				{
					Graphics::Shader* Ambient[2] = { nullptr };
					Graphics::Shader* Point[3] = { nullptr };
					Graphics::Shader* Spot[3] = { nullptr };
					Graphics::Shader* Line[3] = { nullptr };
					Graphics::Shader* Surface = nullptr;
				} Shaders;

			private:
				Rest::Pool<Engine::Component*>* PointLights = nullptr;
				Rest::Pool<Engine::Component*>* SurfaceLights = nullptr;
				Rest::Pool<Engine::Component*>* SpotLights = nullptr;
				Rest::Pool<Engine::Component*>* LineLights = nullptr;
				Graphics::DepthStencilState* DepthStencilNone = nullptr;
				Graphics::DepthStencilState* DepthStencilGreater = nullptr;
				Graphics::DepthStencilState* DepthStencilLess = nullptr;
				Graphics::RasterizerState* FrontRasterizer = nullptr;
				Graphics::RasterizerState* BackRasterizer = nullptr;
				Graphics::RasterizerState* NoneRasterizer = nullptr;
				Graphics::BlendState* BlendAdditive = nullptr;
				Graphics::BlendState* BlendOverwrite = nullptr;
				Graphics::SamplerState* ShadowSampler = nullptr;
				Graphics::SamplerState* WrapSampler = nullptr;
				Graphics::InputLayout* Layout = nullptr;
				Graphics::Texture2D* LightMap = nullptr;
				Graphics::Texture2D* SkyBase = nullptr;
				Graphics::TextureCube* SkyMap = nullptr;

			public:
				Lighting(RenderSystem* Lab);
				virtual ~Lighting();
				void Deserialize(ContentManager* Content, Rest::Document* Node) override;
				void Serialize(ContentManager* Content, Rest::Document* Node) override;
				void Activate() override;
				void Deactivate() override;
				void ResizeBuffers() override;
				void Render(Rest::Timer* Time, RenderState State, RenderOpt Options) override;
				void SetSkyMap(Graphics::Texture2D* Cubemap);
				void SetSurfaceBufferSize(size_t Size);
				void SetVoxelBufferSize(size_t Size);
				Graphics::TextureCube* GetSkyMap();
				Graphics::Texture2D* GetSkyBase();
				Graphics::Texture2D* GetLightMap();

			private:
				void RenderResultBuffers(Graphics::GraphicsDevice* Device, RenderOpt Options);
				void RenderVoxelBuffers(Graphics::GraphicsDevice* Device, RenderOpt Options);
				void RenderShadowMaps(Graphics::GraphicsDevice* Device, SceneGraph* Scene, Rest::Timer* Time);
				void RenderSurfaceMaps(Graphics::GraphicsDevice* Device, SceneGraph* Scene, Rest::Timer* Time);
				void RenderVoxels(Rest::Timer* Time, Graphics::GraphicsDevice* Device);
				void RenderSurfaceLights(Graphics::GraphicsDevice* Device, Compute::Vector3& Camera, float& Distance, bool& Backcull, const bool& Inner);
				void RenderPointLights(Graphics::GraphicsDevice* Device, Compute::Vector3& Camera, float& Distance, bool& Backcull, const bool& Inner);
				void RenderSpotLights(Graphics::GraphicsDevice* Device, Compute::Vector3& Camera, float& Distance, bool& Backcull, const bool& Inner);
				void RenderLineLights(Graphics::GraphicsDevice* Device, bool& Backcull);
				void RenderAmbientLight(Graphics::GraphicsDevice* Device, const bool& Inner);
				void GenerateCascadeMap(CascadedDepthMap** Result, uint32_t Size);
				void FlushDepthBuffersAndCache();

			public:
				static void SetVoxelBuffer(RenderSystem* System, Graphics::Shader* Src, unsigned int Slot);

			public:
				TH_COMPONENT("lighting-renderer");
			};

			class TH_OUT Transparency : public Renderer
			{
			private:
				Graphics::MultiRenderTarget2D* Merger = nullptr;
				Graphics::RenderTarget2D* Input = nullptr;
				Graphics::DepthStencilState* DepthStencil = nullptr;
				Graphics::RasterizerState* Rasterizer = nullptr;
				Graphics::BlendState* Blend = nullptr;
				Graphics::SamplerState* Sampler = nullptr;
				Graphics::InputLayout* Layout = nullptr;
				Graphics::Shader* Shader = nullptr;
				float MipLevels[2] = { 0.0f, 0.0f };

			public:
				struct RenderConstant
				{
					float MipLevels = 0.0f;
					Compute::Vector3 Padding;
				} RenderPass;

			public:
				Transparency(RenderSystem* Lab);
				virtual ~Transparency();
				void ResizeBuffers() override;
				void Render(Rest::Timer* Time, RenderState State, RenderOpt Options) override;

			public:
				TH_COMPONENT("transparency-renderer");
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
					float Samples = 32.0f;
					float MipLevels = 0.0f;
					float Intensity = 1.4f;
					float Distance = 4.0f;
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
					float Intensity = 5.0f;
					float Scale = 0.0f;
					float Bias = 0.0f;
					float Radius = 0.34f;
					float Distance = 0.5f;
					float Fade = 2.54f;
					float MipLevels = 0.0f;
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
					float Intensity = 3.12f;
					float Scale = 0.5f;
					float Bias = 0.11f;
					float Radius = 0.06f;
					float Distance = 3.83f;
					float Fade = 1.96f;
					float Padding = 0.0f;
				} RenderPass1;

				struct RenderConstant2
				{
					float Texel[2] = { 1.0f, 1.0f };
					float Samples = 8.000f;
					float Blur = 4.000f;
					float Power = 1.000f;
					float Additive = 0.000f;
					float Padding[2] = { 0.0, 0.0 };
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