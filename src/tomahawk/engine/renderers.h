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
			class TH_OUT SoftBody final : public GeometryDraw
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
				void CullGeometry(const Viewer& View, Core::Pool<Drawable*>* Geometry) override;
				void RenderGeometryResult(Core::Timer* Time, Core::Pool<Drawable*>* Geometry, RenderOpt Options) override;
				void RenderGeometryVoxels(Core::Timer* Time, Core::Pool<Drawable*>* Geometry, RenderOpt Options) override;
				void RenderDepthLinear(Core::Timer* Time, Core::Pool<Drawable*>* Geometry) override;
				void RenderDepthCubic(Core::Timer* Time, Core::Pool<Drawable*>* Geometry, Compute::Matrix4x4* ViewProjection) override;

			public:
				TH_COMPONENT("soft-body-renderer");
			};

			class TH_OUT Model final : public GeometryDraw
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
				void CullGeometry(const Viewer& View, Core::Pool<Drawable*>* Geometry) override;
				void RenderGeometryResult(Core::Timer* Time, Core::Pool<Drawable*>* Geometry, RenderOpt Options) override;
				void RenderGeometryVoxels(Core::Timer* Time, Core::Pool<Drawable*>* Geometry, RenderOpt Options) override;
				void RenderDepthLinear(Core::Timer* Time, Core::Pool<Drawable*>* Geometry) override;
				void RenderDepthCubic(Core::Timer* Time, Core::Pool<Drawable*>* Geometry, Compute::Matrix4x4* ViewProjection) override;

			public:
				TH_COMPONENT("model-renderer");
			};

			class TH_OUT Skin final : public GeometryDraw
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
				void CullGeometry(const Viewer& View, Core::Pool<Drawable*>* Geometry) override;
				void RenderGeometryResult(Core::Timer* Time, Core::Pool<Drawable*>* Geometry, RenderOpt Options) override;
				void RenderGeometryVoxels(Core::Timer* Time, Core::Pool<Drawable*>* Geometry, RenderOpt Options) override;
				void RenderDepthLinear(Core::Timer* Time, Core::Pool<Drawable*>* Geometry) override;
				void RenderDepthCubic(Core::Timer* Time, Core::Pool<Drawable*>* Geometry, Compute::Matrix4x4* ViewProjection) override;

			public:
				TH_COMPONENT("skin-renderer");
			};

			class TH_OUT Emitter final : public GeometryDraw
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
				void RenderGeometryResult(Core::Timer* Time, Core::Pool<Drawable*>* Geometry, RenderOpt Options) override;
				void RenderGeometryVoxels(Core::Timer* Time, Core::Pool<Drawable*>* Geometry, RenderOpt Options) override;
				void RenderDepthLinear(Core::Timer* Time, Core::Pool<Drawable*>* Geometry) override;
				void RenderDepthCubic(Core::Timer* Time, Core::Pool<Drawable*>* Geometry, Compute::Matrix4x4* ViewProjection) override;

			public:
				TH_COMPONENT("emitter-renderer");
			};

			class TH_OUT Decal final : public GeometryDraw
			{
			public:
				struct RenderConstant
				{
					Compute::Matrix4x4 ViewProjection;
				} RenderPass;

			private:
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
				void RenderGeometryResult(Core::Timer* Time, Core::Pool<Drawable*>* Geometry, RenderOpt Options) override;
				void RenderGeometryVoxels(Core::Timer* Time, Core::Pool<Drawable*>* Geometry, RenderOpt Options) override;
				void RenderDepthLinear(Core::Timer* Time, Core::Pool<Drawable*>* Geometry) override;
				void RenderDepthCubic(Core::Timer* Time, Core::Pool<Drawable*>* Geometry, Compute::Matrix4x4* ViewProjection) override;

			public:
				TH_COMPONENT("decal-renderer");
			};

			class TH_OUT Lighting final : public Renderer
			{
			private:
				struct ISurfaceLight
				{
					Compute::Matrix4x4 WorldViewProjection;
					Compute::Vector3 Position;
					float Range = 0.0f;
					Compute::Vector3 Lighting;
					float Mips = 0.0f;
					Compute::Vector3 Scale;
					float Parallax = 0.0f;
					Compute::Vector3 Attenuation;
					float Infinity = 0.0f;
				};

				struct IPointLight
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
				};

				struct ISpotLight
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
				};

				struct ILineLight
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
					Compute::Vector2 Padding;
					float Bias = 0.0f;
					float Iterations = 0.0f;
					float ScatterIntensity = 0.0f;
					float PlanetRadius = 0.0f;
					float AtmosphereRadius = 0.0f;
					float MieDirection = 0.0f;
				};

				struct IAmbientLight
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
				};

				struct IVoxelBuffer
				{
					Compute::Matrix4x4 WorldViewProjection;
					Compute::Vector3 Center;
					float RayStep = 1.0f;
					Compute::Vector3 Size;
					float Mips = 8.0f;
					Compute::Vector3 Scale;
					float MaxSteps = 32.0f;
					Compute::Vector3 Lights;
					float Radiance = 1.0f;
					Compute::Vector3 Padding;
					float Length = 1.0f;
					float Distance = 1.0f;
					float Occlusion = 0.9f;
					float Specular = 2.0f;
					float Bleeding = 1.0f;
				};

			protected:
				struct
				{
					Graphics::Shader* Ambient[2] = { nullptr };
					Graphics::Shader* Point[2] = { nullptr };
					Graphics::Shader* Spot[2] = { nullptr };
					Graphics::Shader* Line[2] = { nullptr };
					Graphics::Shader* Voxelize = nullptr;
					Graphics::Shader* Surface = nullptr;
				} Shaders;

				struct
				{
					Graphics::ElementBuffer* PBuffer = nullptr;
					Graphics::ElementBuffer* SBuffer = nullptr;
					Graphics::ElementBuffer* LBuffer = nullptr;
					std::vector<IPointLight> PArray;
					std::vector<ISpotLight> SArray;
					std::vector<ILineLight> LArray;
					const size_t MaxLights = 64;
					Component* Area = nullptr;
					bool Inside = false;
				} Storage;

				struct
				{
					Graphics::GraphicsDevice* Device = nullptr;
					SceneGraph* Scene = nullptr;
					Compute::Vector3 View;
					float Distance = 0.0f;
					bool Backcull = true;
					bool Inner = false;
				} State;

			public:
				struct
				{
					std::vector<CubicDepthMap*> PointLight;
					std::vector<LinearDepthMap*> SpotLight;
					std::vector<CascadedDepthMap*> LineLight;
					uint64_t PointLightResolution = 256;
					uint64_t PointLightLimits = 4;
					uint64_t SpotLightResolution = 512;
					uint64_t SpotLightLimits = 8;
					uint64_t LineLightResolution = 1024;
					uint64_t LineLightLimits = 2;
					Core::Ticker Tick;
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

			private:
				Core::Pool<Engine::Component*>* Illuminators = nullptr;
				Core::Pool<Engine::Component*>* SurfaceLights = nullptr;
				Core::Pool<Engine::Component*>* PointLights = nullptr;
				Core::Pool<Engine::Component*>* SpotLights = nullptr;
				Core::Pool<Engine::Component*>* LineLights = nullptr;
				Graphics::DepthStencilState* DepthStencilNone = nullptr;
				Graphics::DepthStencilState* DepthStencilGreater = nullptr;
				Graphics::DepthStencilState* DepthStencilLess = nullptr;
				Graphics::RasterizerState* FrontRasterizer = nullptr;
				Graphics::RasterizerState* BackRasterizer = nullptr;
				Graphics::RasterizerState* NoneRasterizer = nullptr;
				Graphics::BlendState* BlendAdditive = nullptr;
				Graphics::BlendState* BlendOverwrite = nullptr;
				Graphics::BlendState* BlendOverload = nullptr;
				Graphics::SamplerState* DepthSampler = nullptr;
				Graphics::SamplerState* DepthLessSampler = nullptr;
				Graphics::SamplerState* DepthGreaterSampler = nullptr;
				Graphics::SamplerState* WrapSampler = nullptr;
				Graphics::InputLayout* Layout = nullptr;
				Graphics::Texture2D* SkyBase = nullptr;
				Graphics::Texture3D* LightBuffer = nullptr;
				Graphics::TextureCube* SkyMap = nullptr;
				ISurfaceLight SurfaceLight;
				IPointLight PointLight;
				ISpotLight SpotLight;
				ILineLight LineLight;

			public:
				IAmbientLight AmbientLight;
				IVoxelBuffer VoxelBuffer;
				bool EnableGI;

			public:
				Lighting(RenderSystem* Lab);
				virtual ~Lighting();
				void Deserialize(ContentManager* Content, Core::Document* Node) override;
				void Serialize(ContentManager* Content, Core::Document* Node) override;
				void Activate() override;
				void Deactivate() override;
				void ResizeBuffers() override;
				void Render(Core::Timer* Time, RenderState State, RenderOpt Options) override;
				void ClearStorage();
				void SetSkyMap(Graphics::Texture2D* Cubemap);
				void SetSurfaceBufferSize(size_t Size);
				Graphics::TextureCube* GetSkyMap();
				Graphics::Texture2D* GetSkyBase();

			private:
				Component* GetIlluminator(Core::Timer* Time);
				float GetDominant(const Compute::Vector3& Axis);
				bool GetSurfaceLight(ISurfaceLight* Dest, Component* Src, Compute::Vector3& Position, Compute::Vector3& Scale);
				bool GetPointLight(IPointLight* Dest, Component* Src, Compute::Vector3& Position, Compute::Vector3& Scale);
				bool GetSpotLight(ISpotLight* Dest, Component* Src, Compute::Vector3& Position, Compute::Vector3& Scale);
				bool GetLineLight(ILineLight* Dest, Component* Src);
				void GetLightCulling(Component* Src, float Range, Compute::Vector3* Position, Compute::Vector3* Scale);
				void GenerateLightBuffers();
				void GenerateCascadeMap(CascadedDepthMap** Result, uint32_t Size);
				size_t GeneratePointLights();
				size_t GenerateSpotLights();
				size_t GenerateLineLights();
				void FlushDepthBuffersAndCache();
				void RenderResultBuffers(RenderOpt Options);
				void RenderShadowMaps(Core::Timer* Time);
				void RenderSurfaceMaps(Core::Timer* Time);
				void RenderVoxelMap(Core::Timer* Time);
				void RenderLuminance();
				void RenderSurfaceLights();
				void RenderPointLights();
				void RenderSpotLights();
				void RenderLineLights();
				void RenderIllumination();
				void RenderAmbient();

			public:
				static void SetVoxelBuffer(RenderSystem* System, Graphics::Shader* Src, unsigned int Slot);

			public:
				TH_COMPONENT("lighting-renderer");
			};

			class TH_OUT Transparency final : public Renderer
			{
			private:
				Graphics::MultiRenderTarget2D * Merger = nullptr;
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
					Compute::Vector3 Padding;
					float Mips = 0.0f;
				} RenderPass;

			public:
				Transparency(RenderSystem* Lab);
				virtual ~Transparency();
				void ResizeBuffers() override;
				void Render(Core::Timer* Time, RenderState State, RenderOpt Options) override;

			public:
				TH_COMPONENT("transparency-renderer");
			};

			class TH_OUT SSR final : public EffectDraw
			{
			private:
				struct
				{
					Graphics::Shader* Gloss[2] = { nullptr };
					Graphics::Shader* Reflectance = nullptr;
					Graphics::Shader* Additive = nullptr;
				} Shaders;

			public:
				struct ReflectanceBuffer
				{
					float Samples = 32.0f;
					float Mips = 0.0f;
					float Intensity = 1.4f;
					float Distance = 4.0f;
				} Reflectance;

				struct GlossBuffer
				{
					float Texel[2] = { 1.0f, 1.0f };
					float Samples = 4.000f;
					float Blur = 4.000f;
				} Gloss;

			public:
				SSR(RenderSystem* Lab);
				virtual ~SSR() = default;
				void Deserialize(ContentManager * Content, Core::Document * Node) override;
				void Serialize(ContentManager * Content, Core::Document * Node) override;
				void RenderEffect(Core::Timer * Time) override;

			public:
				TH_COMPONENT("ssr-renderer");
			};

			class TH_OUT SSAO final : public EffectDraw
			{
			private:
				struct
				{
					Graphics::Shader* Shading = nullptr;
					Graphics::Shader* Fibo[2] = { nullptr };
					Graphics::Shader* Multiply = nullptr;
				} Shaders;

			public:
				struct ShadingBuffer
				{
					float Samples = 4.0f;
					float Intensity = 3.12f;
					float Scale = 0.5f;
					float Bias = 0.11f;
					float Radius = 0.06f;
					float Distance = 3.83f;
					float Fade = 1.96f;
					float Padding = 0.0f;
				} Shading;

				struct FiboBuffer
				{
					float Texel[2] = { 1.0f, 1.0f };
					float Samples = 8.000f;
					float Blur = 4.000f;
					float Padding[3] = { 0.0f };
					float Power = 1.000f;
				} Fibo;

			public:
				SSAO(RenderSystem* Lab);
				virtual ~SSAO() = default;
				void Deserialize(ContentManager * Content, Core::Document * Node) override;
				void Serialize(ContentManager * Content, Core::Document * Node) override;
				void RenderEffect(Core::Timer * Time) override;

			public:
				TH_COMPONENT("ssao-renderer");
			};

			class TH_OUT DoF final : public EffectDraw
			{
			private:
				struct
				{
					float Radius = 0.0f;
					float Factor = 0.0f;
					float Distance = 0.0f;
					float Range = 0.0f;
				} State;

			public:
				struct FocusBuffer
				{
					float Texel[2] = { 1.0f / 512.0f };
					float Radius = 1.0f;
					float Bokeh = 8.0f;
					float Padding[3] = { 0.0f };
					float Scale = 1.0f;
					float NearDistance = 0.0f;
					float NearRange = 0.0f;
					float FarDistance = 32.0f;
					float FarRange = 2.0f;
				} Focus;

			public:
				float Distance = -1.0f;
				float Radius = 1.5f;
				float Time = 0.1f;

			public:
				DoF(RenderSystem* Lab);
				virtual ~DoF() = default;
				void Deserialize(ContentManager * Content, Core::Document * Node) override;
				void Serialize(ContentManager * Content, Core::Document * Node) override;
				void RenderEffect(Core::Timer * Time) override;
				void FocusAtNearestTarget(float DeltaTime);

			public:
				TH_COMPONENT("dof-renderer");
			};

			class TH_OUT MotionBlur final : public EffectDraw
			{
			private:
				struct
				{
					Graphics::Shader* Velocity = nullptr;
					Graphics::Shader* Motion = nullptr;
				} Shaders;

			public:
				struct VelocityBuffer
				{
					Compute::Matrix4x4 LastViewProjection;
				} Velocity;

				struct MotionBuffer
				{
					float Samples = 32.000f;
					float Blur = 1.8f;
					float Motion = 0.3f;
					float Padding = 0.0f;
				} Motion;

			public:
				MotionBlur(RenderSystem* Lab);
				virtual ~MotionBlur() = default;
				void Deserialize(ContentManager * Content, Core::Document * Node) override;
				void Serialize(ContentManager * Content, Core::Document * Node) override;
				void RenderEffect(Core::Timer * Time) override;

			public:
				TH_COMPONENT("motionblur-renderer");
			};

			class TH_OUT Bloom final : public EffectDraw
			{
			private:
				struct
				{
					Graphics::Shader* Bloom = nullptr;
					Graphics::Shader* Fibo[2] = { nullptr };
					Graphics::Shader* Additive = nullptr;
				} Shaders;

			public:
				struct ExtractionBuffer
				{
					float Padding[2] = { 0.0f };
					float Intensity = 8.0f;
					float Threshold = 0.73f;
				} Extraction;

				struct FiboBuffer
				{
					float Texel[2] = { 1.0f, 1.0f };
					float Samples = 14.000f;
					float Blur = 64.000f;
					float Padding[3] = { 0.0f };
					float Power = 1.000f;
				} Fibo;

			public:
				Bloom(RenderSystem* Lab);
				virtual ~Bloom() = default;
				void Deserialize(ContentManager * Content, Core::Document * Node) override;
				void Serialize(ContentManager * Content, Core::Document * Node) override;
				void RenderEffect(Core::Timer * Time) override;

			public:
				TH_COMPONENT("bloom-renderer");
			};

			class TH_OUT Tone final : public EffectDraw
			{
			private:
				struct
				{
					Graphics::Shader* Luminance = nullptr;
					Graphics::Shader* Tone = nullptr;
				} Shaders;

			public:
				struct LuminanceBuffer
				{
					float Texel[2] = { 1.0f, 1.0f };
					float Mips = 0.0f;
					float Time = 0.0f;
				} Luminance;

				struct MappingBuffer
				{
					float Padding[2] = { 0.0f };
					float Grayscale = -0.12f;
					float ACES = 0.6f;
					float Filmic = -0.12f;
					float Lottes = 0.109f;
					float Reinhard = -0.09f;
					float Reinhard2 = -0.03f;
					float Unreal = -0.13f;
					float Uchimura = 1.0f;
					float UBrightness = 2.0f;
					float UContrast = 1.0f;
					float UStart = 0.82f;
					float ULength = 0.4f;
					float UBlack = 1.13f;
					float UPedestal = 0.05f;
					float Exposure = 0.0f;
					float EIntensity = 0.9f;
					float EGamma = 2.2f;
					float Adaptation = 0.0f;
					float AGray = 1.0f;
					float AWhite = 1.0f;
					float ABlack = 0.05f;
					float ASpeed = 2.0f;
				} Mapping;

			private:
				Graphics::RenderTarget2D* LutTarget = nullptr;
				Graphics::Texture2D* LutMap = nullptr;

			public:
				Tone(RenderSystem* Lab);
				virtual ~Tone() override;
				void Deserialize(ContentManager* Content, Core::Document* Node) override;
				void Serialize(ContentManager* Content, Core::Document* Node) override;
				void RenderEffect(Core::Timer* Time) override;

			private:
				void RenderLUT(Core::Timer* Time);
				void SetLUTSize(size_t Size);

			public:
				TH_COMPONENT("tone-renderer");
			};

			class TH_OUT Glitch final : public EffectDraw
			{
			public:
				struct DistortionBuffer
				{
					float ScanLineJitterDisplacement = 0;
					float ScanLineJitterThreshold = 0;
					float VerticalJumpAmount = 0;
					float VerticalJumpTime = 0;
					float ColorDriftAmount = 0;
					float ColorDriftTime = 0;
					float HorizontalShake = 0;
					float ElapsedTime = 0;
				} Distortion;

			public:
				float ScanLineJitter;
				float VerticalJump;
				float HorizontalShake;
				float ColorDrift;

			public:
				Glitch(RenderSystem* Lab);
				virtual ~Glitch() = default;
				void Deserialize(ContentManager * Content, Core::Document * Node) override;
				void Serialize(ContentManager * Content, Core::Document * Node) override;
				void RenderEffect(Core::Timer * Time) override;

			public:
				TH_COMPONENT("glitch-renderer");
			};

			class TH_OUT UserInterface final : public Renderer
			{
			private:
#ifdef TH_WITH_RMLUI
				GUI::Context * Context;
#endif
				Graphics::Activity* Activity;

			public:
				UserInterface(RenderSystem* Lab);
				UserInterface(RenderSystem* Lab, Graphics::Activity* NewActivity);
				virtual ~UserInterface() override;
				void Render(Core::Timer* Time, RenderState State, RenderOpt Options) override;
#ifdef TH_WITH_RMLUI
				GUI::Context* GetContext();
#endif
			public:
				TH_COMPONENT("user-interface-renderer");
			};
		}
	}
}
#endif