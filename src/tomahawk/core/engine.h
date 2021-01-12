#ifndef TH_ENGINE_H
#define TH_ENGINE_H

#include "graphics.h"
#include "audio.h"
#include "script.h"
#include <atomic>
#include <cstdarg>

namespace Tomahawk
{
	namespace Engine
	{
		typedef Graphics::RenderTargetCube CubicDepthMap;
		typedef Graphics::RenderTarget2D LinearDepthMap;
		typedef std::vector<LinearDepthMap*> CascadedDepthMap;
		typedef void(*JobCallback)(struct Reactor*, class Application*);
		typedef std::function<void(Rest::Timer*, struct Viewer*)> RenderCallback;
		typedef std::function<void(class ContentManager*, bool)> SaveCallback;
		typedef std::function<void(class Event*)> MessageCallback;
		typedef std::function<bool(class Component*)> RayCallback;

		class SceneGraph;

		class Application;

		class ContentManager;

		class Entity;

		class Component;

		class Drawable;

		class Cullable;

		class Processor;

		class RenderSystem;

		enum ApplicationUse
		{
			ApplicationUse_Graphics_Module = 1 << 0,
			ApplicationUse_Activity_Module = 1 << 1,
			ApplicationUse_Audio_Module = 1 << 3,
			ApplicationUse_AngelScript_Module = 1 << 4,
			ApplicationUse_Content_Module = 1 << 5
		};

		enum ApplicationState
		{
			ApplicationState_Terminated,
			ApplicationState_Staging,
			ApplicationState_Singlethreaded,
			ApplicationState_Multithreaded
		};

		enum ThreadId
		{
			ThreadId_Update,
			ThreadId_Render,
			ThreadId_Simulation,
			ThreadId_Synchronize,
			ThreadId_Count
		};

		enum CullResult
		{
			CullResult_Always,
			CullResult_Cache,
			CullResult_Last
		};

		enum RenderOpt
		{
			RenderOpt_None = 0,
			RenderOpt_Transparent = 1,
			RenderOpt_Static = 2,
			RenderOpt_Inner = 4
		};

		enum RenderState
		{
			RenderState_Geometry_Raw,
			RenderState_Geometry_Lumina,
			RenderState_Depth_Linear,
			RenderState_Depth_Cubic
		};

		enum GeoCategory
		{
			GeoCategory_Opaque,
			GeoCategory_Transparent
		};

		struct TH_OUT Material
		{
			Compute::Vector4 Emission = { 0.0f, 0.0f, 0.0f, 0.0f };
			Compute::Vector4 Metallic = { 0.0f, 0.0f, 0.0f, 0.0f };
			Compute::Vector3 Scatter = { 0.1f, 16.0f, 0.0f };
			Compute::Vector2 Roughness = { 1.0f, 0.0f };
			Compute::Vector2 Occlusion = { 1.0f, 0.0f };
			float Fresnel = 0.0f;
			float Transparency = 0.0f;
			float Refraction = 0.0f;
			float Environment = 0.0f;
			float Radius = 0.0f;
			float Id = 0.0f;
		};

		struct TH_OUT AssetCache
		{
			std::string Path;
			void* Resource;
		};

		struct TH_OUT AssetArchive
		{
			Rest::Stream* Stream = nullptr;
			std::string Path;
			uint64_t Length = 0;
			uint64_t Offset = 0;
		};

		struct TH_OUT AnimatorState
		{
			bool Paused = false;
			bool Looped = false;
			bool Blended = false;
			float Duration = -1.0f;
			float Rate = 1.0f;
			float Time = 0.0f;
			int64_t Frame = -1;
			int64_t Clip = -1;
		};

		struct TH_OUT SpawnerProperties
		{
			Compute::RandomVector4 Diffusion;
			Compute::RandomVector3 Position;
			Compute::RandomVector3 Velocity;
			Compute::RandomVector3 Noise;
			Compute::RandomFloat Rotation;
			Compute::RandomFloat Scale;
			Compute::RandomFloat Angular;
			int Iterations = 1;
		};

		struct TH_OUT Appearance
		{
		private:
			Graphics::Texture2D* DiffuseMap = nullptr;
			Graphics::Texture2D* NormalMap = nullptr;
			Graphics::Texture2D* MetallicMap = nullptr;
			Graphics::Texture2D* RoughnessMap = nullptr;
			Graphics::Texture2D* HeightMap = nullptr;
			Graphics::Texture2D* OcclusionMap = nullptr;
			Graphics::Texture2D* EmissionMap = nullptr;

		public:
			Compute::Vector3 Diffuse = 1;
			Compute::Vector2 TexCoord = 1;
			float HeightAmount = 0.0f;
			float HeightBias = 0.0f;
			int64_t Material = -1;

		public:
			Appearance();
			Appearance(const Appearance& Other);
			~Appearance();
			bool FillGeometry(Graphics::GraphicsDevice* Device) const;
			bool FillLumina(Graphics::GraphicsDevice* Device) const;
			bool FillDepthLinear(Graphics::GraphicsDevice* Device) const;
			bool FillDepthCubic(Graphics::GraphicsDevice* Device) const;
			void SetDiffuseMap(Graphics::Texture2D* New);
			Graphics::Texture2D* GetDiffuseMap() const;
			void SetNormalMap(Graphics::Texture2D* New);
			Graphics::Texture2D* GetNormalMap() const;
			void SetMetallicMap(Graphics::Texture2D* New);
			Graphics::Texture2D* GetMetallicMap() const;
			void SetRoughnessMap(Graphics::Texture2D* New);
			Graphics::Texture2D* GetRoughnessMap() const;
			void SetHeightMap(Graphics::Texture2D* New);
			Graphics::Texture2D* GetHeightMap() const;
			void SetOcclusionMap(Graphics::Texture2D* New);
			Graphics::Texture2D* GetOcclusionMap() const;
			void SetEmissionMap(Graphics::Texture2D* New);
			Graphics::Texture2D* GetEmissionMap() const;
			Appearance& operator= (const Appearance& Other);
		};

		struct TH_OUT Viewer
		{
			RenderSystem* Renderer = nullptr;
			Compute::Matrix4x4 CubicViewProjection[6];
			Compute::Matrix4x4 InvViewProjection;
			Compute::Matrix4x4 ViewProjection;
			Compute::Matrix4x4 Projection;
			Compute::Matrix4x4 View;
			Compute::Vector3 InvViewPosition;
			Compute::Vector3 ViewPosition;
			Compute::Vector3 WorldPosition;
			Compute::Vector3 WorldRotation;
			float FarPlane = 0.0f;
			float NearPlane = 0.0f;

			void Set(const Compute::Matrix4x4& View, const Compute::Matrix4x4& Projection, const Compute::Vector3& Position, float Near, float Far);
		};

		struct TH_OUT AssetFile : public Rest::Object
		{
		private:
			char* Buffer;
			size_t Size;

		public:
			AssetFile(char* SrcBuffer, size_t SrcSize);
			virtual ~AssetFile() override;
			char* GetBuffer();
			size_t GetSize();
		};

		struct TH_OUT Reactor
		{
			friend Application;

		private:
			Application* App;
			JobCallback Src;

		public:
			Rest::Timer* Time;

		private:
			Reactor(Application* Ref, double Limit, JobCallback Callback);
			~Reactor();
			void UpdateCore();
			void UpdateTask();
		};

		class TH_OUT NMake
		{
		public:
			static bool Pack(Rest::Document* V, bool Value);
			static bool Pack(Rest::Document* V, int Value);
			static bool Pack(Rest::Document* V, unsigned int Value);
			static bool Pack(Rest::Document* V, float Value);
			static bool Pack(Rest::Document* V, double Value);
			static bool Pack(Rest::Document* V, int64_t Value);
			static bool Pack(Rest::Document* V, long double Value);
			static bool Pack(Rest::Document* V, uint64_t Value);
			static bool Pack(Rest::Document* V, const char* Value);
			static bool Pack(Rest::Document* V, const Compute::Vector2& Value);
			static bool Pack(Rest::Document* V, const Compute::Vector3& Value);
			static bool Pack(Rest::Document* V, const Compute::Vector4& Value);
			static bool Pack(Rest::Document* V, const Compute::Matrix4x4& Value);
			static bool Pack(Rest::Document* V, const Material& Value);
			static bool Pack(Rest::Document* V, const AnimatorState& Value);
			static bool Pack(Rest::Document* V, const SpawnerProperties& Value);
			static bool Pack(Rest::Document* V, const Appearance& Value, ContentManager* Content);
			static bool Pack(Rest::Document* V, const Compute::SkinAnimatorKey& Value);
			static bool Pack(Rest::Document* V, const Compute::SkinAnimatorClip& Value);
			static bool Pack(Rest::Document* V, const Compute::KeyAnimatorClip& Value);
			static bool Pack(Rest::Document* V, const Compute::AnimatorKey& Value);
			static bool Pack(Rest::Document* V, const Compute::ElementVertex& Value);
			static bool Pack(Rest::Document* V, const Compute::Joint& Value);
			static bool Pack(Rest::Document* V, const Compute::Vertex& Value);
			static bool Pack(Rest::Document* V, const Compute::SkinVertex& Value);
			static bool Pack(Rest::Document* V, const Rest::TickTimer& Value);
			static bool Pack(Rest::Document* V, const std::string& Value);
			static bool Pack(Rest::Document* V, const std::vector<bool>& Value);
			static bool Pack(Rest::Document* V, const std::vector<int>& Value);
			static bool Pack(Rest::Document* V, const std::vector<unsigned int>& Value);
			static bool Pack(Rest::Document* V, const std::vector<float>& Value);
			static bool Pack(Rest::Document* V, const std::vector<double>& Value);
			static bool Pack(Rest::Document* V, const std::vector<int64_t>& Value);
			static bool Pack(Rest::Document* V, const std::vector<long double>& Value);
			static bool Pack(Rest::Document* V, const std::vector<uint64_t>& Value);
			static bool Pack(Rest::Document* V, const std::vector<Compute::Vector2>& Value);
			static bool Pack(Rest::Document* V, const std::vector<Compute::Vector3>& Value);
			static bool Pack(Rest::Document* V, const std::vector<Compute::Vector4>& Value);
			static bool Pack(Rest::Document* V, const std::vector<Compute::Matrix4x4>& Value);
			static bool Pack(Rest::Document* V, const std::vector<AnimatorState>& Value);
			static bool Pack(Rest::Document* V, const std::vector<SpawnerProperties>& Value);
			static bool Pack(Rest::Document* V, const std::vector<Compute::SkinAnimatorClip>& Value);
			static bool Pack(Rest::Document* V, const std::vector<Compute::KeyAnimatorClip>& Value);
			static bool Pack(Rest::Document* V, const std::vector<Compute::AnimatorKey>& Value);
			static bool Pack(Rest::Document* V, const std::vector<Compute::ElementVertex>& Value);
			static bool Pack(Rest::Document* V, const std::vector<Compute::Joint>& Value);
			static bool Pack(Rest::Document* V, const std::vector<Compute::Vertex>& Value);
			static bool Pack(Rest::Document* V, const std::vector<Compute::SkinVertex>& Value);
			static bool Pack(Rest::Document* V, const std::vector<Rest::TickTimer>& Value);
			static bool Pack(Rest::Document* V, const std::vector<std::string>& Value);
			static bool Unpack(Rest::Document* V, bool* O);
			static bool Unpack(Rest::Document* V, int* O);
			static bool Unpack(Rest::Document* V, unsigned int* O);
			static bool Unpack(Rest::Document* V, float* O);
			static bool Unpack(Rest::Document* V, double* O);
			static bool Unpack(Rest::Document* V, int64_t* O);
			static bool Unpack(Rest::Document* V, long double* O);
			static bool Unpack(Rest::Document* V, uint64_t* O);
			static bool Unpack(Rest::Document* V, Compute::Vector2* O);
			static bool Unpack(Rest::Document* V, Compute::Vector3* O);
			static bool Unpack(Rest::Document* V, Compute::Vector4* O);
			static bool Unpack(Rest::Document* V, Compute::Matrix4x4* O);
			static bool Unpack(Rest::Document* V, Material* O);
			static bool Unpack(Rest::Document* V, AnimatorState* O);
			static bool Unpack(Rest::Document* V, SpawnerProperties* O);
			static bool Unpack(Rest::Document* V, Appearance* O, ContentManager* Content);
			static bool Unpack(Rest::Document* V, Compute::SkinAnimatorKey* O);
			static bool Unpack(Rest::Document* V, Compute::SkinAnimatorClip* O);
			static bool Unpack(Rest::Document* V, Compute::KeyAnimatorClip* O);
			static bool Unpack(Rest::Document* V, Compute::AnimatorKey* O);
			static bool Unpack(Rest::Document* V, Compute::ElementVertex* O);
			static bool Unpack(Rest::Document* V, Compute::Joint* O);
			static bool Unpack(Rest::Document* V, Compute::Vertex* O);
			static bool Unpack(Rest::Document* V, Compute::SkinVertex* O);
			static bool Unpack(Rest::Document* V, Rest::TickTimer* O);
			static bool Unpack(Rest::Document* V, std::string* O);
			static bool Unpack(Rest::Document* V, std::vector<bool>* O);
			static bool Unpack(Rest::Document* V, std::vector<int>* O);
			static bool Unpack(Rest::Document* V, std::vector<unsigned int>* O);
			static bool Unpack(Rest::Document* V, std::vector<float>* O);
			static bool Unpack(Rest::Document* V, std::vector<double>* O);
			static bool Unpack(Rest::Document* V, std::vector<int64_t>* O);
			static bool Unpack(Rest::Document* V, std::vector<long double>* O);
			static bool Unpack(Rest::Document* V, std::vector<uint64_t>* O);
			static bool Unpack(Rest::Document* V, std::vector<Compute::Vector2>* O);
			static bool Unpack(Rest::Document* V, std::vector<Compute::Vector3>* O);
			static bool Unpack(Rest::Document* V, std::vector<Compute::Vector4>* O);
			static bool Unpack(Rest::Document* V, std::vector<Compute::Matrix4x4>* O);
			static bool Unpack(Rest::Document* V, std::vector<AnimatorState>* O);
			static bool Unpack(Rest::Document* V, std::vector<SpawnerProperties>* O);
			static bool Unpack(Rest::Document* V, std::vector<Compute::SkinAnimatorClip>* O);
			static bool Unpack(Rest::Document* V, std::vector<Compute::KeyAnimatorClip>* O);
			static bool Unpack(Rest::Document* V, std::vector<Compute::AnimatorKey>* O);
			static bool Unpack(Rest::Document* V, std::vector<Compute::ElementVertex>* O);
			static bool Unpack(Rest::Document* V, std::vector<Compute::Joint>* O);
			static bool Unpack(Rest::Document* V, std::vector<Compute::Vertex>* O);
			static bool Unpack(Rest::Document* V, std::vector<Compute::SkinVertex>* O);
			static bool Unpack(Rest::Document* V, std::vector<Rest::TickTimer>* O);
			static bool Unpack(Rest::Document* V, std::vector<std::string>* O);
		};

		class TH_OUT Event
		{
			friend SceneGraph;

		private:
			Component * TComponent;
			Entity* TEntity;
			SceneGraph* TScene;
			std::string Id;

		public:
			Compute::PropertyArgs Args;

		private:
			Event(const std::string& NewName, SceneGraph* Target, const Compute::PropertyArgs& NewArgs);
			Event(const std::string& NewName, Entity* Target, const Compute::PropertyArgs& NewArgs);
			Event(const std::string& NewName, Component* Target, const Compute::PropertyArgs& NewArgs);

		public:
			bool Is(const std::string& Name);
			const std::string& GetName();
			Component* GetComponent();
			Entity* GetEntity();
			SceneGraph* GetScene();
		};

		class TH_OUT Processor : public Rest::Object
		{
			friend ContentManager;

		protected:
			ContentManager* Content;

		public:
			Processor(ContentManager* NewContent);
			virtual ~Processor() override;
			virtual void Free(AssetCache* Asset);
			virtual void* Duplicate(AssetCache* Asset, const Compute::PropertyArgs& Keys);
			virtual void* Deserialize(Rest::Stream* Stream, uint64_t Length, uint64_t Offset, const Compute::PropertyArgs& Keys);
			virtual bool Serialize(Rest::Stream* Stream, void* Instance, const Compute::PropertyArgs& Keys);
			ContentManager* GetContent();
		};

		class TH_OUT Component : public Rest::Object
		{
			friend SceneGraph;
			friend Entity;

		private:
			bool Active;

		protected:
			Entity* Parent;

		public:
			Component(Entity* Ref);
			virtual ~Component() override;
			virtual void Serialize(ContentManager* Content, Rest::Document* Node);
			virtual void Deserialize(ContentManager* Content, Rest::Document* Node);
			virtual void Awake(Component* New);
			virtual void Asleep();
			virtual void Synchronize(Rest::Timer* Time);
			virtual void Update(Rest::Timer* Time);
			virtual void Message(Event* Value);
			virtual Component* Copy(Entity* New) = 0;
			virtual Compute::Matrix4x4 GetBoundingBox();
			Entity* GetEntity();
			void SetActive(bool Enabled);
			bool IsActive();

		public:
			TH_COMPONENT("component");
		};

		class TH_OUT Entity : public Rest::Object
		{
			friend SceneGraph;

		protected:
			std::unordered_map<uint64_t, Component*> Components;
			SceneGraph* Scene;

		public:
			Compute::Transform* Transform;
			std::string Name;
			int64_t Id, Tag;
			float Distance;

		public:
			Entity(SceneGraph* Ref);
			virtual ~Entity() override;
			void RemoveComponent(uint64_t Id);
			void RemoveChilds();
			void SetScene(SceneGraph* NewScene);
			std::unordered_map<uint64_t, Component*>::iterator First();
			std::unordered_map<uint64_t, Component*>::iterator Last();
			Component* AddComponent(Component* In);
			Component* GetComponent(uint64_t Id);
			uint64_t GetComponentCount();
			SceneGraph* GetScene();

		public:
			template <typename In>
			void RemoveComponent()
			{
				RemoveComponent(In::GetTypeId());
			}
			template <typename In>
			In* AddComponent()
			{
				Component* Component = new In(this);
				Component->Active = true;
				return (In*)AddComponent(Component);
			}
			template <typename In>
			In* GetComponent()
			{
				auto It = Components.find(In::GetTypeId());
				if (It != Components.end())
					return (In*)It->second;

				return nullptr;
			}
		};

		class TH_OUT Renderer : public Rest::Object
		{
			friend SceneGraph;

		protected:
			RenderSystem * System;

		public:
			bool Active;

		public:
			Renderer(RenderSystem* Lab);
			virtual ~Renderer() override;
			virtual void Serialize(ContentManager* Content, Rest::Document* Node);
			virtual void Deserialize(ContentManager* Content, Rest::Document* Node);
			virtual void CullGeometry(const Viewer& View);
			virtual void ResizeBuffers();
			virtual void Activate();
			virtual void Deactivate();
			virtual void Render(Rest::Timer* TimeStep, RenderState State, RenderOpt Options);
			void SetRenderer(RenderSystem* NewSystem);
			RenderSystem* GetRenderer();

		public:
			TH_COMPONENT("renderer");
		};

		class TH_OUT ShaderCache : public Rest::Object
		{
		private:
			struct SCache
			{
				Graphics::Shader* Shader;
				uint64_t Count;
			};

		private:
			std::unordered_map<std::string, SCache> Cache;
			Graphics::GraphicsDevice* Device;
			std::mutex Safe;

		public:
			ShaderCache(Graphics::GraphicsDevice* Device);
			virtual ~ShaderCache() override;
			Graphics::Shader* Compile(const std::string& Name, const Graphics::Shader::Desc& Desc, size_t BufferSize = 0);
			Graphics::Shader* Get(const std::string& Name);
			bool Free(const std::string& Name, Graphics::Shader* Shader = nullptr);
			void ClearCache();
		};

		class TH_OUT RenderSystem : public Rest::Object
		{
		protected:
			std::vector<Renderer*> Renderers;
			std::unordered_set<uint64_t> Cull;
			Graphics::DepthStencilState* DepthStencil;
			Graphics::BlendState* Blend;
			Graphics::SamplerState* Sampler;
			Graphics::DepthBuffer* Target;
			Graphics::ElementBuffer* QuadVertex;
			Graphics::ElementBuffer* SphereVertex;
			Graphics::ElementBuffer* SphereIndex;
			Graphics::ElementBuffer* CubeVertex;
			Graphics::ElementBuffer* CubeIndex;
			Graphics::ElementBuffer* BoxVertex;
			Graphics::ElementBuffer* BoxIndex;
			Graphics::ElementBuffer* SkinBoxVertex;
			Graphics::ElementBuffer* SkinBoxIndex;
			Graphics::GraphicsDevice* Device;
			SceneGraph* Scene;
			size_t DepthSize;
			bool OcclusionCulling;
			bool FrustumCulling;
			bool Satisfied;

		public:
			Rest::TickTimer Occlusion;
			Rest::TickTimer Sorting;
			size_t StallFrames;

		public:
			RenderSystem(Graphics::GraphicsDevice* Device);
			virtual ~RenderSystem() override;
			void SetOcclusionCulling(bool Enabled, bool KeepResults = false);
			void SetFrustumCulling(bool Enabled, bool KeepResults = false);
			void SetDepthSize(size_t Size);
			void SetScene(SceneGraph* NewScene);
			void Remount(Renderer* Target);
			void Remount();
			void Mount();
			void Unmount();
			void ClearCull();
			void CullGeometry(Rest::Timer* Time, const Viewer& View);
			void Synchronize(Rest::Timer* Time, const Viewer& View);
			void MoveRenderer(uint64_t Id, int64_t Offset);
			void RemoveRenderer(uint64_t Id);
			void FreeShader(const std::string& Name, Graphics::Shader* Shader);
			bool PassCullable(Cullable* Base, CullResult Mode, float* Result);
			bool PassDrawable(Drawable* Base, CullResult Mode, float* Result);
			bool HasOcclusionCulling();
			bool HasFrustumCulling();
			int64_t GetOffset(uint64_t Id);
			Graphics::Shader* CompileShader(const std::string& Name, Graphics::Shader::Desc& Desc, size_t BufferSize = 0);
			Renderer* AddRenderer(Renderer* In);
			Renderer* GetRenderer(uint64_t Id);
			size_t GetDepthSize();
			Graphics::ElementBuffer* GetQuadVBuffer();
			Graphics::ElementBuffer* GetSphereVBuffer();
			Graphics::ElementBuffer* GetSphereIBuffer();
			Graphics::ElementBuffer* GetCubeVBuffer();
			Graphics::ElementBuffer* GetCubeIBuffer();
			Graphics::ElementBuffer* GetBoxVBuffer();
			Graphics::ElementBuffer* GetBoxIBuffer();
			Graphics::ElementBuffer* GetSkinBoxVBuffer();
			Graphics::ElementBuffer* GetSkinBoxIBuffer();
			std::vector<Renderer*>* GetRenderers();
			Graphics::GraphicsDevice* GetDevice();
			SceneGraph* GetScene();

		private:
			Rest::Pool<Component*>* GetSceneComponents(uint64_t Section);

		public:
			template <typename In>
			void RemoveRenderer()
			{
				RemoveRenderer(In::GetTypeId());
			}
			template <typename In>
			int64_t GetOffset()
			{
				return GetOffset(In::GetTypeId());
			}
			template <typename In, typename... Args>
			In* AddRenderer(Args&& ... Data)
			{
				return (In*)AddRenderer(new In(this, Data...));
			}
			template <typename In>
			In* GetRenderer()
			{
				return (In*)GetRenderer(In::GetTypeId());
			}
			template <typename T>
			Rest::Pool<Component*>* AddCull()
			{
				static_assert(std::is_base_of<Cullable, T>::value,
					"component is not cullable");

				Cull.insert(T::GetTypeId());
				return GetSceneComponents(T::GetTypeId());
			}
			template <typename T>
			Rest::Pool<Component*>* RemoveCull()
			{
				static_assert(std::is_base_of<Cullable, T>::value,
					"component is not cullable");

				auto It = Cull.find(T::GetTypeId());
				if (It != Cull.end())
					Cull.erase(*It);

				return nullptr;
			}
		};

		class TH_OUT Cullable : public Component
		{
			friend RenderSystem;

		protected:
			float Visibility;

		public:
			Cullable(Entity* Ref);
			virtual ~Cullable() = default;
			virtual float Cull(const Viewer& View) = 0;
			virtual Component* Copy(Entity* New) override = 0;
			virtual void ClearCull();
			float GetRange();
			bool IsVisible(const Viewer& View, Compute::Matrix4x4* World);
			bool IsNear(const Viewer& View);

		public:
			TH_COMPONENT("cullable");
		};

		class TH_OUT Drawable : public Cullable
		{
			friend SceneGraph;

		private:
			Graphics::Query* Query;
			GeoCategory Category;
			uint64_t Fragments;
			uint64_t Source;
			int Satisfied;
			bool Complex;

		protected:
			std::unordered_map<void*, Appearance> Surfaces;

		public:
			bool Static;

		public:
			Drawable(Entity* Ref, uint64_t Hash, bool Complex);
			virtual ~Drawable();
			virtual void Message(Event* Value) override;
			virtual Component* Copy(Entity* New) override = 0;
			virtual void ClearCull() override;
			bool SetTransparency(bool Enabled);
			bool Begin(Graphics::GraphicsDevice* Device);
			void End(Graphics::GraphicsDevice* Device);
			int Fetch(RenderSystem* System);
			uint64_t GetFragmentsCount();
			const std::unordered_map<void*, Appearance>& GetSurfaces();
			Material* GetMaterial(Appearance* Surface);
			Material* GetMaterial();
			Appearance* GetSurface(void* Instance);
			Appearance* GetSurface();
			bool HasTransparency();

		protected:
			void Attach();
			void Detach();

		public:
			TH_COMPONENT("drawable");
		};

		class TH_OUT GeometryDraw : public Renderer
		{
		private:
			uint64_t Source;

		public:
			GeometryDraw(RenderSystem* Lab, uint64_t Hash);
			virtual ~GeometryDraw() override;
			virtual void CullGeometry(const Viewer& View, Rest::Pool<Drawable*>* Geometry);
			virtual void RenderGeometryRaw(Rest::Timer* TimeStep, Rest::Pool<Drawable*>* Geometry, RenderOpt Options) = 0;
			virtual void RenderGeometryLumina(Rest::Timer* TimeStep, Rest::Pool<Drawable*>* Geometry, RenderOpt Options) = 0;
			virtual void RenderDepthLinear(Rest::Timer* TimeStep, Rest::Pool<Drawable*>* Geometry) = 0;
			virtual void RenderDepthCubic(Rest::Timer* TimeStep, Rest::Pool<Drawable*>* Geometry, Compute::Matrix4x4* ViewProjection) = 0;
			void CullGeometry(const Viewer& View) override;
			void Render(Rest::Timer* TimeStep, RenderState State, RenderOpt Options) override;
			Rest::Pool<Drawable*>* GetOpaque();
			Rest::Pool<Drawable*>* GetTransparent();

		public:
			TH_COMPONENT("geometry-draw");
		};

		class TH_OUT TimingDraw : public Renderer
		{
		public:
			Rest::TickTimer Tick;

		public:
			TimingDraw(RenderSystem* Lab);
			virtual ~TimingDraw() override;
			virtual void TickRender(Rest::Timer* TimeStep, RenderState State, RenderOpt Options);
			virtual void FrameRender(Rest::Timer* TimeStep, RenderState State, RenderOpt Options);
			void Render(Rest::Timer* TimeStep, RenderState State, RenderOpt Options) override;

		public:
			TH_COMPONENT("timing-draw");
		};

		class TH_OUT EffectDraw : public Renderer
		{
		protected:
			std::unordered_map<std::string, Graphics::Shader*> Shaders;
			Graphics::DepthStencilState* DepthStencil;
			Graphics::RasterizerState* Rasterizer;
			Graphics::BlendState* Blend;
			Graphics::SamplerState* Sampler;
			Graphics::InputLayout* Layout;
			Graphics::RenderTarget2D* Output;
			Graphics::Texture2D* Pass;

		public:
			EffectDraw(RenderSystem* Lab);
			virtual ~EffectDraw() override;
			virtual void RenderEffect(Rest::Timer* Time);
			void ResizeBuffers() override;
			void Render(Rest::Timer* Time, RenderState State, RenderOpt Options) override;

		protected:
			void RenderMerge(Graphics::Shader* Effect, void* Buffer = nullptr);
			void RenderResult(Graphics::Shader* Effect, void* Buffer = nullptr);
			Graphics::Shader* GetEffect(const std::string& Name);
			Graphics::Shader* CompileEffect(const std::string& Name, const std::string& Code, size_t BufferSize = 0);

		public:
			TH_COMPONENT("effect-draw");
		};

		class TH_OUT SceneGraph : public Rest::Object
		{
			friend Renderer;
			friend Component;
			friend Entity;
			friend Drawable;

		public:
			struct Desc
			{
				bool EnableHDR = false;
				float RenderQuality = 1.0f;
				uint64_t EntityCount = 1ll << 15;
				uint64_t ComponentCount = 1ll << 16;
				Compute::Simulator::Desc Simulator;
				Graphics::GraphicsDevice* Device = nullptr;
				Script::VMManager* Manager = nullptr;
				Rest::EventQueue* Queue = nullptr;
				ShaderCache* Cache = nullptr;
			};

			struct Thread
			{
				std::atomic<std::thread::id> Id;
				std::atomic<int> State;
			};

		private:
			struct Geometry
			{
				Rest::Pool<Drawable*> Opaque;
				Rest::Pool<Drawable*> Transparent;
			};

		private:
			struct
			{
				Thread Threads[ThreadId_Count];
				std::condition_variable Callback;
				std::condition_variable Condition;
				std::atomic<std::thread::id> Id;
				std::atomic<int> Count;
				std::atomic<bool> Locked;
				std::mutex Await, Safe, Events;
				std::mutex Global, Listener;
			} Sync;

			struct
			{
				Graphics::DepthStencilState* DepthStencil;
				Graphics::RasterizerState* Rasterizer;
				Graphics::BlendState* Blend;
				Graphics::SamplerState* Sampler;
				Graphics::InputLayout* Layout;
			} Image;

		protected:
			Graphics::MultiRenderTarget2D* Surface = nullptr;
			Graphics::ElementBuffer* Structure = nullptr;
			Compute::Simulator* Simulator = nullptr;
			std::unordered_map<std::string, std::pair<std::string, MessageCallback>> Listeners;
			std::unordered_map<uint64_t, Rest::Pool<Component*>> Components;
			std::unordered_map<uint64_t, Geometry> Drawables;
			std::vector<Material> Materials;
			std::vector<std::string> Names;
			std::vector<Event*> Events;
			Rest::Pool<Component*> Pending;
			Rest::Pool<Entity*> Entities;
			Component* Camera = nullptr;
			Desc Conf;
			bool Invoked;
			bool Active;

		public:
			Viewer View;

		public:
			SceneGraph(const Desc& I);
			virtual ~SceneGraph() override;
			void Configure(const Desc& Conf);
			void Submit();
			void Render(Rest::Timer* Time);
			void Render(Rest::Timer* Time, RenderState Stage, RenderOpt Options);
			void Update(Rest::Timer* Time);
			void Simulation(Rest::Timer* Time);
			void Synchronize(Rest::Timer* Time);
			void RemoveMaterial(uint64_t MaterialId);
			void RemoveEntity(Entity* Entity, bool Release);
			void SetCamera(Entity* Camera);
			void CloneEntities(Entity* Instance, std::vector<Entity*>* Array);
			void RestoreViewBuffer(Viewer* View);
			void SortOpaqueBackToFront(uint64_t Section);
			void SortOpaqueBackToFront(Rest::Pool<Drawable*>* Array);
			void SortOpaqueFrontToBack(uint64_t Section);
			void SortOpaqueFrontToBack(Rest::Pool<Drawable*>* Array);
			void Actualize();
			void Redistribute();
			void Reindex();
			void ExpandMaterialStructure();
			void Lock();
			void Unlock();
			void ResizeBuffers();
			void RayTest(uint64_t Section, const Compute::Ray& Origin, float MaxDistance, const RayCallback& Callback);
			void ScriptHook(const std::string& Name = "Main");
			void SwapSurface(Graphics::MultiRenderTarget2D* NewSurface);
			void SetActive(bool Enabled);
			void SetView(const Compute::Matrix4x4& View, const Compute::Matrix4x4& Projection, const Compute::Vector3& Position, float Near, float Far, bool Upload);
			void SetSurface();
			void SetSurfaceCleared();
			void SetMaterialName(uint64_t Material, const std::string& Name);
			void ClearSurface();
			void ClearSurfaceColor();
			void ClearSurfaceDepth();
			void GetTargetDesc(Graphics::RenderTarget2D::Desc* Result);
			void GetTargetDesc(Graphics::MultiRenderTarget2D::Desc* Result);
			void GetTargetFormat(Graphics::Format* Result, uint64_t Size);
			bool AddEventListener(const std::string& Name, const std::string& Event, const MessageCallback& Callback);
			bool RemoveEventListener(const std::string& Name);
			bool DispatchEvent(const std::string& EventName, const Compute::PropertyArgs& Args);
			bool DispatchEvent(Component* Target, const std::string& EventName, const Compute::PropertyArgs& Args);
			bool DispatchEvent(Entity* Target, const std::string& EventName, const Compute::PropertyArgs& Args);
			void Dispatch();
			Material* AddMaterial(const std::string& Name, const Material& Material);
			Entity* CloneEntities(Entity* Value);
			Entity* FindNamedEntity(const std::string& Name);
			Entity* FindEntityAt(const Compute::Vector3& Position, float Radius);
			Entity* FindTaggedEntity(uint64_t Tag);
			Entity* GetEntity(uint64_t Entity);
			Entity* GetLastEntity();
			Component* GetComponent(uint64_t Component, uint64_t Section);
			Component* GetCamera();
			RenderSystem* GetRenderer();
			Viewer GetCameraViewer();
			std::string GetMaterialName(uint64_t Material);
			Material* GetMaterialByName(const std::string& Material);
			Material* GetMaterialById(uint64_t Material);
			Rest::Pool<Component*>* GetComponents(uint64_t Section);
			std::vector<Entity*> FindParentFreeEntities(Entity* Entity);
			std::vector<Entity*> FindNamedEntities(const std::string& Name);
			std::vector<Entity*> FindEntitiesAt(const Compute::Vector3& Position, float Radius);
			std::vector<Entity*> FindTaggedEntities(uint64_t Tag);
			bool IsEntityVisible(Entity* Entity, const Compute::Matrix4x4& ViewProjection);
			bool IsEntityVisible(Entity* Entity, const Compute::Matrix4x4& ViewProjection, const Compute::Vector3& ViewPosition, float DrawDistance);
			bool AddEntity(Entity* Entity);
			bool IsActive();
			uint64_t GetMaterialCount();
			uint64_t GetEntityCount();
			uint64_t GetComponentCount(uint64_t Section);
			uint64_t GetOpaqueCount();
			uint64_t GetTransparentCount();
			bool HasEntity(Entity* Entity);
			bool HasEntity(uint64_t Entity);
			Rest::Pool<Drawable*>* GetOpaque(uint64_t Section);
			Rest::Pool<Drawable*>* GetTransparent(uint64_t Section);
			Graphics::MultiRenderTarget2D* GetSurface();
			Graphics::ElementBuffer* GetStructure();
			Graphics::GraphicsDevice* GetDevice();
			Rest::EventQueue* GetQueue();
			Compute::Simulator* GetSimulator();
			ShaderCache* GetCache();
			Desc& GetConf();

		protected:
			void AddDrawable(Drawable* Source, GeoCategory Category);
			void RemoveDrawable(Drawable* Source, GeoCategory Category);
			void BeginThread(ThreadId Thread);
			void EndThread(ThreadId Thread);
			void Mutate(Entity* Target, bool Added);
			void RegisterEntity(Entity* In);
			bool UnregisterEntity(Entity* In);
			bool DispatchLastEvent();
			Entity* CloneEntity(Entity* Entity);

		public:
			template <typename T>
			void RayTest(const Compute::Ray& Origin, float MaxDistance, const RayCallback& Callback)
			{
				RayTest(T::GetTypeId(), Origin, MaxDistance, Callback);
			}
			template <typename T>
			void SortEntitiesBackToFront()
			{
				SortEntitiesBackToFront(T::GetTypeId());
			}
			template <typename T>
			uint64_t GetEntityCount()
			{
				return GetComponents(T::GetTypeId())->Count();
			}
			template <typename T>
			Entity* GetLastEntity()
			{
				auto* Array = GetComponents(T::GetTypeId());
				if (Array->Empty())
					return nullptr;

				Component* Value = GetComponent(Array->Count() - 1, T::GetTypeId());
				if (Value != nullptr)
					return Value->Parent;

				return nullptr;
			}
			template <typename T>
			Entity* GetEntity()
			{
				Component* Value = GetComponent(0, T::GetTypeId());
				if (Value != nullptr)
					return Value->Parent;

				return nullptr;
			}
			template <typename T>
			Rest::Pool<Component*>* GetComponents()
			{
				return GetComponents(T::GetTypeId());
			}
			template <typename T>
			Rest::Pool<Drawable*>* GetOpaque()
			{
				return GetOpaque(T::GetTypeId());
			}
			template <typename T>
			Rest::Pool<Drawable*>* GetTransparent()
			{
				return GetTransparent(T::GetTypeId());
			}
			template <typename T>
			T* GetComponent()
			{
				return (T*)GetComponent(0, T::GetTypeId());
			}
			template <typename T>
			T* GetLastComponent()
			{
				auto* Array = GetComponents(T::GetTypeId());
				if (Array->Empty())
					return nullptr;

				return (T*)GetComponent(Array->Count() - 1, T::GetTypeId());
			}
		};

		class TH_OUT ContentManager : public Rest::Object
		{
		private:
			std::unordered_map<std::string, std::unordered_map<Processor*, AssetCache*>> Assets;
			std::unordered_map<std::string, AssetArchive*> Dockers;
			std::unordered_map<int64_t, Processor*> Processors;
			std::unordered_map<Rest::Stream*, int64_t> Streams;
			Graphics::GraphicsDevice* Device;
			Rest::EventQueue* Queue;
			std::string Environment, Base;
			std::mutex Mutex;

		public:
			ContentManager(Graphics::GraphicsDevice* NewDevice, Rest::EventQueue* NewQueue);
			virtual ~ContentManager() override;
			void InvalidateDockers();
			void InvalidateCache();
			void InvalidatePath(const std::string& Path);
			void SetEnvironment(const std::string& Path);
			bool Import(const std::string& Path);
			bool Export(const std::string& Path, const std::string& Directory, const std::string& Name = "");
			bool Cache(Processor* Root, const std::string& Path, void* Resource);
			Graphics::GraphicsDevice* GetDevice();
			Rest::EventQueue* GetQueue();
			std::string GetEnvironment();

		public:
			template <typename T>
			T* Load(const std::string& Path, const Compute::PropertyArgs& Keys = Compute::PropertyArgs())
			{
				return (T*)LoadForward(Path, GetProcessor<T>(), Keys);
			}
			template <typename T>
			bool LoadAsync(const std::string& Path, const Compute::PropertyArgs& Keys, const std::function<void(class ContentManager*, T*)>& Callback)
			{
				if (!Queue)
					return false;

				return Queue->Task<ContentManager>(this, [this, Path, Callback, Keys](Rest::EventQueue*, Rest::EventArgs*)
				{
					T* Result = (T*)LoadForward(Path, GetProcessor<T>(), Keys);
					if (Callback)
						Callback(this, Result);
				});
			}
			template <typename T>
			bool Save(const std::string& Path, T* Object, const Compute::PropertyArgs& Keys = Compute::PropertyArgs())
			{
				return SaveForward(Path, GetProcessor<T>(), Object, Keys);
			}
			template <typename T>
			bool SaveAsync(const std::string& Path, T* Object, const Compute::PropertyArgs& Keys, const SaveCallback& Callback)
			{
				if (!Queue)
					return false;

				return Queue->Task<ContentManager>(this, [this, Path, Callback, Object, Keys](Rest::EventQueue*, Rest::EventArgs*)
				{
					bool Result = SaveForward(Path, GetProcessor<T>(), Object, Keys);
					if (Callback)
						Callback(this, Result);
				});
			}
			template <typename T>
			bool RemoveProcessor()
			{
				Mutex.lock();
				auto It = Processors.find(typeid(T).hash_code());
				if (It == Processors.end())
				{
					Mutex.unlock();
					return false;
				}

				if (It->second != nullptr)
					delete It->second;

				Processors.erase(It);
				Mutex.unlock();
				return true;
			}
			template <typename V, typename T>
			V* AddProcessor()
			{
				Mutex.lock();
				V* Instance = new V(this);
				auto It = Processors.find(typeid(T).hash_code());
				if (It != Processors.end())
				{
					if (It->second != nullptr)
						delete It->second;
					It->second = Instance;
				}
				else
					Processors[typeid(T).hash_code()] = Instance;

				Mutex.unlock();
				return Instance;
			}
			template <typename T>
			Processor* GetProcessor()
			{
				Mutex.lock();
				auto It = Processors.find(typeid(T).hash_code());
				if (It != Processors.end())
				{
					Mutex.unlock();
					return It->second;
				}

				Mutex.unlock();
				return nullptr;
			}
			template <typename T>
			AssetCache* Find(const std::string& Path)
			{
				return Find(GetProcessor<T>(), Path);
			}
			template <typename T>
			AssetCache* Find(void* Resource)
			{
				return Find(GetProcessor<T>(), Resource);
			}

		private:
			void* LoadForward(const std::string& Path, Processor* Processor, const Compute::PropertyArgs& Keys);
			void* LoadStreaming(const std::string& Path, Processor* Processor, const Compute::PropertyArgs& Keys);
			bool SaveForward(const std::string& Path, Processor* Processor, void* Object, const Compute::PropertyArgs& Keys);
			AssetCache* Find(Processor* Target, const std::string& Path);
			AssetCache* Find(Processor* Target, void* Resource);
		};

		class TH_OUT Application : public Rest::Object
		{
		public:
			struct Desc
			{
				Graphics::GraphicsDevice::Desc GraphicsDevice;
				Graphics::Activity::Desc Activity;
				Rest::EventWorkflow Threading = Rest::EventWorkflow_Singlethreaded;
				std::string Environment;
				std::string Directory;
				uint64_t TaskWorkersCount = 0;
				uint64_t EventWorkersCount = 0;
				double FrameLimit = 0;
				double MaxFrames = 60;
				double MinFrames = 10;
				unsigned int Usage = ApplicationUse_Graphics_Module | ApplicationUse_Activity_Module | ApplicationUse_Audio_Module | ApplicationUse_AngelScript_Module | ApplicationUse_Content_Module;
				bool DisableCursor = false;
				bool EnableShaderCache = true;
			};

		private:
			static Application* Host;

		private:
			std::vector<Reactor*> Workers;
			ApplicationState State = ApplicationState_Terminated;

		public:
			Audio::AudioDevice* Audio = nullptr;
			Graphics::GraphicsDevice* Renderer = nullptr;
			Graphics::Activity* Activity = nullptr;
			Script::VMManager* VM = nullptr;
			Rest::EventQueue* Queue = nullptr;
			ContentManager* Content = nullptr;
			SceneGraph* Scene = nullptr;
			ShaderCache* Shaders = nullptr;

		public:
			Application(Desc* I);
			virtual ~Application() override;
			virtual void KeyEvent(Graphics::KeyCode Key, Graphics::KeyMod Mod, int Virtual, int Repeat, bool Pressed);
			virtual void InputEvent(char* Buffer, int Length);
			virtual void WheelEvent(int X, int Y, bool Normal);
			virtual void WindowEvent(Graphics::WindowState NewState, int X, int Y);
			virtual void ScriptHook(Script::VMGlobal* Global);
			virtual bool ComposeEvent();
			virtual void Render(Rest::Timer* Time);
			virtual void Initialize(Desc* I);
			virtual void* GetGUI();
			void Start(Desc* I);
			void Stop();

		public:
			template <typename T, void(T::*Event)(Rest::Timer*)>
			Reactor* Enqueue(double UpdateLimit = 0.0)
			{
				static_assert(std::is_base_of<Application, T>::value,
					"method is not from Application class");

				if (!Event)
					return nullptr;

				Reactor* Result = new Reactor(this, UpdateLimit, [](Reactor* Job, Application* App)
				{
					(((T*)App)->*Event)(Job->Time);
				});

				Workers.push_back(Result);
				return Result;
			}

		private:
			static void Callee(Rest::EventQueue* Queue, Rest::EventArgs* Args);
			static void Compose();

		public:
			static Application* Get();
		};

		inline RenderOpt operator |(RenderOpt A, RenderOpt B)
		{
			return static_cast<RenderOpt>(static_cast<uint64_t>(A) | static_cast<uint64_t>(B));
		}
	}
}
#endif