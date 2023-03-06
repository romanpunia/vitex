#ifndef ED_ENGINE_RENDERERS_H
#define ED_ENGINE_RENDERERS_H
#include "../core/engine.h"
#include "components.h"
#include "gui.h"

namespace Edge
{
	namespace Engine
	{
		namespace Renderers
		{
			class ED_OUT SoftBody final : public GeometryRenderer<Components::SoftBody>
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
				~SoftBody();
				size_t CullGeometry(const Viewer& View, const GeometryRenderer::Objects& Chunk) override;
				size_t RenderGeometryResult(Core::Timer* Time, const GeometryRenderer::Objects& Chunk) override;
				size_t RenderGeometryVoxels(Core::Timer* Time, const GeometryRenderer::Objects& Chunk) override;
				size_t RenderDepthLinear(Core::Timer* Time, const GeometryRenderer::Objects& Chunk) override;
				size_t RenderDepthCubic(Core::Timer* Time, const GeometryRenderer::Objects& Chunk, Compute::Matrix4x4* ViewProjection) override;

			public:
				ED_COMPONENT("soft_body_renderer");
			};

			class ED_OUT Model final : public GeometryRenderer<Components::Model, Graphics::MeshBuffer, Graphics::RenderBuffer::Instance>
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
				Graphics::InputLayout* Layout[2];

			public:
				Model(RenderSystem* Lab);
				~Model() override;
				void BatchGeometry(Components::Model* Base, Batching& Batch, size_t Chunk) override;
				size_t CullGeometry(const Viewer& View, const GeometryRenderer::Objects& Chunk) override;
				size_t RenderGeometryResultBatched(Core::Timer* Time, const GeometryRenderer::Groups& Chunk) override;
				size_t RenderGeometryVoxelsBatched(Core::Timer* Time, const GeometryRenderer::Groups& Chunk) override;
				size_t RenderDepthLinearBatched(Core::Timer* Time, const GeometryRenderer::Groups& Chunk) override;
				size_t RenderDepthCubicBatched(Core::Timer* Time, const GeometryRenderer::Groups& Chunk, Compute::Matrix4x4* ViewProjection) override;

			public:
				ED_COMPONENT("model_renderer");
			};

			class ED_OUT Skin final : public GeometryRenderer<Components::Skin>
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
				~Skin();
				size_t CullGeometry(const Viewer& View, const GeometryRenderer::Objects& Chunk) override;
				size_t RenderGeometryResult(Core::Timer* Time, const GeometryRenderer::Objects& Chunk) override;
				size_t RenderGeometryVoxels(Core::Timer* Time, const GeometryRenderer::Objects& Chunk) override;
				size_t RenderDepthLinear(Core::Timer* Time, const GeometryRenderer::Objects& Chunk) override;
				size_t RenderDepthCubic(Core::Timer* Time, const GeometryRenderer::Objects& Chunk, Compute::Matrix4x4* ViewProjection) override;

			public:
				ED_COMPONENT("skin_renderer");
			};

			class ED_OUT Emitter final : public GeometryRenderer<Components::Emitter>
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
				Graphics::DepthStencilState* DepthStencilAdditive = nullptr;
				Graphics::RasterizerState* Rasterizer = nullptr;
				Graphics::BlendState* AdditiveBlend = nullptr;
				Graphics::BlendState* OverwriteBlend = nullptr;
				Graphics::SamplerState* Sampler = nullptr;
				Graphics::InputLayout* Layout = nullptr;

			public:
				Emitter(RenderSystem* Lab);
				~Emitter() override;
				size_t RenderGeometryResult(Core::Timer* Time, const GeometryRenderer::Objects& Chunk) override;
				size_t RenderDepthLinear(Core::Timer* Time, const GeometryRenderer::Objects& Chunk) override;
				size_t RenderDepthCubic(Core::Timer* Time, const GeometryRenderer::Objects& Chunk, Compute::Matrix4x4* ViewProjection) override;

			public:
				ED_COMPONENT("emitter_renderer");
			};

			class ED_OUT Decal final : public GeometryRenderer<Components::Decal>
			{
			private:
				Graphics::DepthStencilState* DepthStencil = nullptr;
				Graphics::RasterizerState* Rasterizer = nullptr;
				Graphics::BlendState* Blend = nullptr;
				Graphics::SamplerState* Sampler = nullptr;
				Graphics::InputLayout* Layout = nullptr;
				Graphics::Shader* Shader = nullptr;

			public:
				Decal(RenderSystem* Lab);
				~Decal() override;
				size_t RenderGeometryResult(Core::Timer* Time, const GeometryRenderer::Objects& Chunk) override;

			public:
				ED_COMPONENT("decal_renderer");
			};

			class ED_OUT Lighting final : public Renderer
			{
			public:
				struct ISurfaceLight
				{
					Compute::Matrix4x4 Transform;
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
					Compute::Matrix4x4 Transform;
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
					Compute::Matrix4x4 Transform;
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
					Compute::Matrix4x4 Transform;
					Compute::Vector3 Center;
					float RayStep = 1.0f;
					Compute::Vector3 Size;
					float Mips = 8.0f;
					Compute::Vector3 Scale;
					float MaxSteps = 32.0f;
					Compute::Vector3 Lights;
					float Radiance = 1.0f;
					float Margin = 3.828424f;
					float Offset = -0.01f;
					float Angle = 0.5f;
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
					Graphics::GraphicsDevice* Device = nullptr;
					SceneGraph* Scene = nullptr;
					bool Backcull = true;
				} State;

				struct
				{
					RendererProxy<Components::Illuminator> Illuminators;
					RendererProxy<Components::SurfaceLight> Surfaces;
					RendererProxy<Components::PointLight> Points;
					RendererProxy<Components::SpotLight> Spots;
					Core::Pool<Component*>* Lines;
				} Lights;

			public:
				struct
				{
					Ticker Tick;
					float Distance = 0.5f;
				} Shadows;

				struct
				{
					Graphics::ElementBuffer* PBuffer = nullptr;
					Graphics::ElementBuffer* SBuffer = nullptr;
					Graphics::ElementBuffer* LBuffer = nullptr;
					Graphics::Texture3D* LightBuffer = nullptr;
					std::vector<IPointLight> PArray;
					std::vector<ISpotLight> SArray;
					std::vector<ILineLight> LArray;
					const size_t MaxLights = 64;
				} Voxels;

				struct
				{
					Graphics::MultiRenderTarget2D* Merger = nullptr;
					Graphics::RenderTarget2D* Output = nullptr;
					Graphics::RenderTarget2D* Input = nullptr;
					Graphics::Cubemap* Subresource = nullptr;
					Graphics::Texture2D* Face = nullptr;
					size_t Size = 128;
				} Surfaces;

			private:
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
				Graphics::TextureCube* SkyMap = nullptr;
				ISurfaceLight SurfaceLight;
				IPointLight PointLight;
				ISpotLight SpotLight;
				ILineLight LineLight;

			public:
				Graphics::Texture2D* LightingMap = nullptr;
				IAmbientLight AmbientLight;
				IVoxelBuffer VoxelBuffer;
				bool EnableGI;

			public:
				Lighting(RenderSystem* Lab);
				~Lighting();
				size_t RenderPass(Core::Timer* Time) override;
				void Deserialize(Core::Schema* Node) override;
				void Serialize(Core::Schema* Node) override;
				void ResizeBuffers() override;
				void BeginPass() override;
				void EndPass() override;
				void SetSkyMap(Graphics::Texture2D* Cubemap);
				void SetSurfaceBufferSize(size_t Size);
				Graphics::TextureCube* GetSkyMap();
				Graphics::Texture2D* GetSkyBase();

			private:
				float GetDominant(const Compute::Vector3& Axis);
				bool GetSurfaceLight(ISurfaceLight* Dest, Component* Src, Compute::Vector3& Position, Compute::Vector3& Scale);
				bool GetPointLight(IPointLight* Dest, Component* Src, Compute::Vector3& Position, Compute::Vector3& Scale, bool Reposition);
				bool GetSpotLight(ISpotLight* Dest, Component* Src, Compute::Vector3& Position, Compute::Vector3& Scale, bool Reposition);
				bool GetLineLight(ILineLight* Dest, Component* Src);
				bool GetIlluminator(IVoxelBuffer* Dest, Component* Src);
				void GetLightCulling(Component* Src, float Range, Compute::Vector3* Position, Compute::Vector3* Scale);
				void GenerateLightBuffers();
				size_t GeneratePointLights();
				size_t GenerateSpotLights();
				size_t GenerateLineLights();
				void RenderResultBuffers();
				void RenderVoxelMap(Core::Timer* Time);
				void RenderSurfaceMaps(Core::Timer* Time);
				void RenderPointShadowMaps(Core::Timer* Time);
				void RenderSpotShadowMaps(Core::Timer* Time);
				void RenderLineShadowMaps(Core::Timer* Time);
				void RenderSurfaceLights();
				void RenderPointLights();
				void RenderSpotLights();
				void RenderLineLights();
				void RenderLuminance();
				void RenderIllumination();
				void RenderAmbient();

			public:
				static void SetVoxelBuffer(RenderSystem* System, Graphics::Shader* Src, unsigned int Slot);

			public:
				ED_COMPONENT("lighting_renderer");
			};

			class ED_OUT Transparency final : public Renderer
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
				} RenderData;

			public:
				Transparency(RenderSystem* Lab);
				~Transparency();
				void ResizeBuffers() override;
				size_t RenderPass(Core::Timer* Time) override;

			public:
				ED_COMPONENT("transparency_renderer");
			};

			class ED_OUT SSR final : public EffectRenderer
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
					float Padding = 0.0f;
					float Intensity = 1.0f;
					float Distance = 16.0f;
				} Reflectance;

				struct GlossBuffer
				{
					float Padding = 0.0f;
					float Deadzone = 0.05f;
					float Mips = 0.0f;
					float Cutoff = 0.95f;
					float Texel[2] = { 1.0f, 1.0f };
					float Samples = 48.000f;
					float Blur = 64.000f;
				} Gloss;

			public:
				SSR(RenderSystem* Lab);
				void Deserialize(Core::Schema * Node) override;
				void Serialize(Core::Schema * Node) override;
				void RenderEffect(Core::Timer * Time) override;

			public:
				ED_COMPONENT("ssr_renderer");
			};

			class ED_OUT SSGI final : public EffectRenderer
			{
			private:
				struct
				{
					Graphics::Shader* Stochastic = nullptr;
					Graphics::Shader* Indirection = nullptr;
					Graphics::Shader* Denoise[2] = { nullptr };
					Graphics::Shader* Additive = nullptr;
				} Shaders;

			public:
				struct StochasticBuffer
				{
					float Texel[2] = { 1.0f, 1.0f };
					float FrameId = 0.0f;
					float Padding = 0.0f;
				} Stochastic;

				struct IndirectionBuffer
				{
					float Random[2] = { 0.0f, 0.0f };
					float Samples = 10.0f;
					float Distance = 3.0f;
					float Initial = 0.0f;
					float Cutoff = -0.05f;
					float Attenuation = 1.0f;
					float Swing = 0.333333f;
					float Padding[3] = { 0.0f, 0.0f, 0.0f };
					float Bias = 2.0f;
				} Indirection;

				struct DenoiseBuffer
				{
					float Padding[3] = { 0.0f, 0.0f, 0.0f };
					float Cutoff = 0.95f;
					float Texel[2] = { 1.0f, 1.0f };
					float Samples = 32.000f;
					float Blur = 16.000f;
				} Denoise;

			private:
				Graphics::Texture2D* EmissionMap = nullptr;

			public:
				uint32_t Bounces = 1;

			public:
				SSGI(RenderSystem* Lab);
				~SSGI() override;
				void Deserialize(Core::Schema* Node) override;
				void Serialize(Core::Schema* Node) override;
				void RenderEffect(Core::Timer* Time) override;
				void ResizeEffect() override;

			public:
				ED_COMPONENT("ssgi_renderer");
			};

			class ED_OUT SSAO final : public EffectRenderer
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
					float Padding[3] = { 0.0f };
					float Power = 1.000f;
					float Texel[2] = { 1.0f, 1.0f };
					float Samples = 14.000f;
					float Blur = 64.000f;
				} Fibo;

			public:
				SSAO(RenderSystem* Lab);
				void Deserialize(Core::Schema * Node) override;
				void Serialize(Core::Schema * Node) override;
				void RenderEffect(Core::Timer * Time) override;

			public:
				ED_COMPONENT("ssao_renderer");
			};

			class ED_OUT DoF final : public EffectRenderer
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
				void Deserialize(Core::Schema * Node) override;
				void Serialize(Core::Schema * Node) override;
				void RenderEffect(Core::Timer * Time) override;
				void FocusAtNearestTarget(float Step);

			public:
				ED_COMPONENT("dof_renderer");
			};

			class ED_OUT MotionBlur final : public EffectRenderer
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
				void Deserialize(Core::Schema * Node) override;
				void Serialize(Core::Schema * Node) override;
				void RenderEffect(Core::Timer * Time) override;

			public:
				ED_COMPONENT("motion_blur_renderer");
			};

			class ED_OUT Bloom final : public EffectRenderer
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
					float Padding[3] = { 0.0f };
					float Power = 1.000f;
					float Texel[2] = { 1.0f, 1.0f };
					float Samples = 14.000f;
					float Blur = 64.000f;
				} Fibo;

			public:
				Bloom(RenderSystem* Lab);
				void Deserialize(Core::Schema * Node) override;
				void Serialize(Core::Schema * Node) override;
				void RenderEffect(Core::Timer * Time) override;

			public:
				ED_COMPONENT("bloom_renderer");
			};

			class ED_OUT Tone final : public EffectRenderer
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
				~Tone() override;
				void Deserialize(Core::Schema* Node) override;
				void Serialize(Core::Schema* Node) override;
				void RenderEffect(Core::Timer* Time) override;

			private:
				void RenderLUT(Core::Timer* Time);
				void SetLUTSize(size_t Size);

			public:
				ED_COMPONENT("tone_renderer");
			};

			class ED_OUT Glitch final : public EffectRenderer
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
				void Deserialize(Core::Schema * Node) override;
				void Serialize(Core::Schema * Node) override;
				void RenderEffect(Core::Timer * Time) override;

			public:
				ED_COMPONENT("glitch_renderer");
			};

			class ED_OUT UserInterface final : public Renderer
			{
			private:
				GUI::Context * Context;
				Graphics::Activity* Activity;

			public:
				UserInterface(RenderSystem* Lab);
				UserInterface(RenderSystem* Lab, Graphics::Activity* NewActivity);
				~UserInterface() override;
				size_t RenderPass(Core::Timer* Time) override;
				GUI::Context* GetContext();

			public:
				ED_COMPONENT("user_interface_renderer");
			};
		}
	}
}
#endif