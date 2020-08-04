#ifndef THAWK_ENGINE_H
#define THAWK_ENGINE_H

#include "graphics.h"
#include "audio.h"
#include "script.h"
#include <atomic>
#include <cstdarg>

namespace Tomahawk
{
	namespace Engine
	{
		typedef std::unordered_map<std::string, struct ContentKey> ContentMap;
		typedef std::function<void(Rest::Timer*, struct Viewer*)> RenderCallback;
		typedef std::function<void(class ContentManager*, bool)> SaveCallback;
		typedef std::function<void(class Entity*, class Component*, bool)> MutationCallback;

		class SceneGraph;

		class Application;

		class ContentManager;

		class Entity;

		class Component;

		class FileProcessor;

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

		enum ContentType
		{
			ContentType_Null,
			ContentType_String,
			ContentType_Integer,
			ContentType_Number,
			ContentType_Boolean,
			ContentType_Pointer,
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
			RenderOpt_Limpid = 1,
			RenderOpt_Static = 2,
			RenderOpt_Inner = 4
		};

		enum RenderState
		{
			RenderState_GBuffer,
			RenderState_Depth_Linear,
			RenderState_Depth_Cubic
		};

		struct THAWK_OUT Material
		{
			Compute::Vector4 Emission;
			Compute::Vector4 Metallic;
			Compute::Vector2 Roughness = { 1, 0 };
			float Fresnel = 0.0f;
			float Limpidity = 0.0f;
			float Refraction = 0.0f;
			float Environment = 0.0f;
			float Occlusion = 1.0f;
			float Radius = 0.0f;
			float Id = 0.0f;
		};

		struct THAWK_OUT AssetResource
		{
			FileProcessor* Processor = nullptr;
			void* Resource = nullptr;
			std::string Path;
		};

		struct THAWK_OUT AssetDocker
		{
			Rest::FileStream* Stream = nullptr;
			std::string Path;
			uint64_t Length = 0;
			uint64_t Offset = 0;
		};

		struct THAWK_OUT ContentKey
		{
			ContentType Type;
			std::string String;
			int64_t Integer;
			double Number;
			bool Boolean;
			void* Pointer;

			ContentKey();
			explicit ContentKey(const std::string& Value);
			explicit ContentKey(int64_t Value);
			explicit ContentKey(double Value);
			explicit ContentKey(bool Value);
			explicit ContentKey(void* Value);
		};

		struct THAWK_OUT ContentArgs
		{
			ContentMap* Args;

			ContentArgs(ContentMap* Map);
			bool Is(const std::string& Name, const ContentKey& Value);
			ContentKey* Get(const std::string& Name);
			ContentKey* Get(const std::string& Name, ContentType Type);
		};

		struct THAWK_OUT ThreadEvent
		{
			std::function<void(Rest::Timer * )> Callback;
			Rest::Timer* Timer = nullptr;
			Application* App = nullptr;
		};

		struct THAWK_OUT AnimatorState
		{
			bool Paused = false;
			bool Looped = false;
			bool Blended = false;
			float Length = 15.0f;
			float Speed = 1.0f;
			float Time = 0.0f;
			int64_t Frame = -1;
			int64_t Clip = -1;
		};

		struct THAWK_OUT SpawnerProperties
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

		struct THAWK_OUT Appearance
		{
			Graphics::Texture2D* DiffuseMap = nullptr;
			Graphics::Texture2D* NormalMap = nullptr;
			Graphics::Texture2D* MetallicMap = nullptr;
			Graphics::Texture2D* RoughnessMap = nullptr;
			Graphics::Texture2D* HeightMap = nullptr;
			Graphics::Texture2D* OcclusionMap = nullptr;
			Graphics::Texture2D* EmissionMap = nullptr;
			Compute::Vector3 Diffuse = 1;
			Compute::Vector2 TexCoord = 1;
			int64_t Material = -1;

			static bool UploadPhase(Graphics::GraphicsDevice* Device, Appearance* Surface);
			static bool UploadDepth(Graphics::GraphicsDevice* Device, Appearance* Surface);
			static bool UploadCubicDepth(Graphics::GraphicsDevice* Device, Appearance* Surface);
		};

		struct THAWK_OUT Viewer
		{
			Compute::Matrix4x4 CubicViewProjection[6];
			Compute::Matrix4x4 InvViewProjection;
			Compute::Matrix4x4 ViewProjection;
			Compute::Matrix4x4 Projection;
			Compute::Matrix4x4 View;
			Compute::Vector3 InvViewPosition;
			Compute::Vector3 ViewPosition;
			Compute::Vector3 WorldPosition;
			Compute::Vector3 WorldRotation;
			float ViewDistance = 0.0f;
			RenderSystem* Renderer = nullptr;

			void Set(const Compute::Matrix4x4& View, const Compute::Matrix4x4& Projection, const Compute::Vector3& Position, float Distance);
		};

		class THAWK_OUT NMake
		{
		public:
			static bool Pack(Rest::Document* V, bool Value);
			static bool Pack(Rest::Document* V, int Value);
			static bool Pack(Rest::Document* V, unsigned int Value);
			static bool Pack(Rest::Document* V, long O);
			static bool Pack(Rest::Document* V, unsigned long O);
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
			static bool Pack(Rest::Document* V, const std::vector<long>& Value);
			static bool Pack(Rest::Document* V, const std::vector<unsigned long>& Value);
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
			static bool Unpack(Rest::Document* V, long* O);
			static bool Unpack(Rest::Document* V, unsigned long* O);
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
			static bool Unpack(Rest::Document* V, std::vector<long>* O);
			static bool Unpack(Rest::Document* V, std::vector<unsigned long>* O);
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

		class THAWK_OUT Event
		{
			friend SceneGraph;

		private:
			Component* Root = nullptr;
			void* Context = nullptr;
			uint64_t Type = 0;
			bool Foreach = false;

		public:
			template <typename T>
			bool Is()
			{
				return typeid(T).hash_code() == Type;
			}
			template <typename T>
			T* Get()
			{
				return (T*)Context;
			}
		};

		struct THAWK_OUT AssetFile : public Rest::Object
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

		class THAWK_OUT FileProcessor : public Rest::Object
		{
			friend ContentManager;

		protected:
			ContentManager* Content;

		public:
			FileProcessor(ContentManager* NewContent);
			virtual ~FileProcessor() override;
			virtual void Free(AssetResource* Asset);
			virtual void* Duplicate(AssetResource* Asset, ContentArgs* Keys);
			virtual void* Deserialize(Rest::FileStream* Stream, uint64_t Length, uint64_t Offset, ContentArgs* Keys);
			virtual bool Serialize(Rest::FileStream* Stream, void* Instance, ContentArgs* Keys);
			ContentManager* GetContent();
		};

		class THAWK_OUT Component : public Rest::Object
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
			virtual void Pipe(Event* Value);
			virtual Component* Copy(Entity* New) = 0;
			Entity* GetEntity();
			void SetActive(bool Enabled);
			bool IsActive();

		public:
			THAWK_COMPONENT_BASIS(Component);
		};

		class THAWK_OUT Cullable : public Component
		{
			friend RenderSystem;

		protected:
			float Visibility;

		public:
			Cullable(Entity* Ref);
			virtual ~Cullable() = default;
			virtual float Cull(const Viewer& View) = 0;
			virtual Component* Copy(Entity* New) = 0;
			float GetRange();
			bool IsVisible(const Viewer& View, Compute::Matrix4x4* World);
			bool IsNear(const Viewer& View);

		public:
			THAWK_COMPONENT(Cullable);
		};

		class THAWK_OUT Drawable : public Cullable
		{
		private:
			bool Complex;

		protected:
			std::unordered_map<void*, Appearance> Surfaces;

		public:
			bool Static;

		public:
			Drawable(Entity* Ref, bool Complex);
			virtual ~Drawable() = default;
			virtual void Pipe(Event* Value) override;
			virtual Component* Copy(Entity* New) = 0;
			const std::unordered_map<void*, Appearance>& GetSurfaces();
			Material* GetMaterial(Appearance* Surface);
			Material* GetMaterial();
			Appearance* GetSurface(void* Instance);
			Appearance* GetSurface();
			bool IsLimpid();

		public:
			THAWK_COMPONENT(Drawable);
		};

		class THAWK_OUT Entity : public Rest::Object
		{
			friend SceneGraph;

		protected:
			std::unordered_map<uint64_t, Component*> Components;
			SceneGraph* Scene;

		public:
			Compute::Transform* Transform;
			std::string Name;
			int64_t Id, Tag;

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
				RemoveComponent(In::BaseId());
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
				auto It = Components.find(In::BaseId());
				if (It != Components.end())
					return (In*)It->second;

				return nullptr;
			}
		};

		class THAWK_OUT Renderer : public Rest::Object
		{
			friend SceneGraph;

		protected:
			RenderSystem* System;

		public:
			bool Active;

		public:
			Renderer(RenderSystem* Lab);
			virtual ~Renderer() override;
			virtual void Serialize(ContentManager* Content, Rest::Document* Node);
			virtual void Deserialize(ContentManager* Content, Rest::Document* Node);
			virtual void ResizeBuffers();
			virtual void Activate();
			virtual void Deactivate();
			virtual void Render(Rest::Timer* TimeStep, RenderState State, RenderOpt Options);
			void SetRenderer(RenderSystem* NewSystem);
			RenderSystem* GetRenderer();
			
		public:
			THAWK_COMPONENT_BASIS(Renderer);
		};

		class THAWK_OUT GeoRenderer : public Renderer
		{
		public:
			GeoRenderer(RenderSystem* Lab);
			virtual ~GeoRenderer() override;
			virtual void RenderGBuffer(Rest::Timer* TimeStep, Rest::Pool<Component*>* Geometry, RenderOpt Options) = 0;
			virtual void RenderDepthLinear(Rest::Timer* TimeStep, Rest::Pool<Component*>* Geometry) = 0;
			virtual void RenderDepthCubic(Rest::Timer* TimeStep, Rest::Pool<Component*>* Geometry, Compute::Matrix4x4* ViewProjection) = 0;
			virtual Rest::Pool<Component*>* GetOpaque() = 0;
			virtual Rest::Pool<Component*>* GetLimpid(uint64_t Layer) = 0;
			void Render(Rest::Timer* TimeStep, RenderState State, RenderOpt Options) override;

		public:
			THAWK_COMPONENT(GeoRenderer);
		};

		class THAWK_OUT TickRenderer : public Renderer
		{
		protected:
			Rest::TickTimer Tick;

		public:
			TickRenderer(RenderSystem* Lab);
			virtual ~TickRenderer() override;
			virtual void TickRender(Rest::Timer* TimeStep, RenderState State, RenderOpt Options);
			virtual void FrameRender(Rest::Timer* TimeStep, RenderState State, RenderOpt Options);
			void Render(Rest::Timer* TimeStep, RenderState State, RenderOpt Options) override;

		public:
			THAWK_COMPONENT(TickRenderer);
		};

		class THAWK_OUT EffectRenderer : public Renderer
		{
		protected:
			std::unordered_map<std::string, Graphics::Shader*> Shaders;
			Graphics::DepthStencilState* DepthStencil;
			Graphics::RasterizerState* Rasterizer;
			Graphics::BlendState* Blend;
			Graphics::SamplerState* Sampler;
			Graphics::RenderTarget2D* Output;
			Graphics::Texture2D* Pass;

		public:
			EffectRenderer(RenderSystem* Lab);
			virtual ~EffectRenderer() override;
			virtual void RenderEffect(Rest::Timer* Time);
			void Activate() override;
			void ResizeBuffers() override;
			void Render(Rest::Timer* Time, RenderState State, RenderOpt Options) override;

		protected:
			void RenderMerge(Graphics::Shader* Effect, void* Buffer = nullptr);
			void RenderResult(Graphics::Shader* Effect, void* Buffer = nullptr);
			Graphics::Shader* GetEffect(const std::string& Name);
			Graphics::Shader* CompileEffect(const std::string& Name, const std::string& Code, size_t BufferSize = 0);

		public:
			THAWK_COMPONENT(EffectRenderer);
		};

		class THAWK_OUT ShaderCache : public Rest::Object
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

		class THAWK_OUT RenderSystem : public Rest::Object
		{
		protected:
			std::unordered_map<uint64_t, Rest::Pool<Component*>*> Cull;
			std::vector<Renderer*> Renderers;
			Graphics::ElementBuffer* QuadVertex;
			Graphics::ElementBuffer* SphereVertex;
			Graphics::ElementBuffer* SphereIndex;
			Graphics::ElementBuffer* CubeVertex;
			Graphics::ElementBuffer* CubeIndex;
			Graphics::GraphicsDevice* Device;
			SceneGraph* Scene;

		public:
			bool EnableCull;

		public:
			RenderSystem(Graphics::GraphicsDevice* Device);
			virtual ~RenderSystem() override;
			void SetScene(SceneGraph* NewScene);
			void Synchronize(const Viewer& View);
			void MoveRenderer(uint64_t Id, int64_t Offset);
			void RemoveRenderer(uint64_t Id);
			void FreeShader(const std::string& Name, Graphics::Shader* Shader);
			bool Renderable(Cullable* Base, CullResult Mode, float* Result);
			Graphics::Shader* CompileShader(const std::string& Name, Graphics::Shader::Desc& Desc, size_t BufferSize = 0);
			Renderer* AddRenderer(Renderer* In);
			Renderer* GetRenderer(uint64_t Id);
			size_t GetQuadVSize();
			size_t GetSphereVSize();
			size_t GetSphereISize();
			size_t GetCubeVSize();
			size_t GetCubeISize();
			Graphics::ElementBuffer* GetQuadVBuffer();
			Graphics::ElementBuffer* GetSphereVBuffer();
			Graphics::ElementBuffer* GetSphereIBuffer();
			Graphics::ElementBuffer* GetCubeVBuffer();
			Graphics::ElementBuffer* GetCubeIBuffer();
			std::vector<Renderer*>* GetRenderers();
			Graphics::GraphicsDevice* GetDevice();
			SceneGraph* GetScene();

		public:
			template <typename In>
			void RemoveRenderer()
			{
				RemoveRenderer(In::BaseId());
			}
			template <typename In, typename... Args>
			In* AddRenderer(Args&& ... Data)
			{
				return (In*)AddRenderer(new In(this, Data...));
			}
			template <typename In>
			In* GetRenderer()
			{
				return (In*)GetRenderer(In::BaseId());
			}
			template <typename T>
			Rest::Pool<Component*>* AddCull()
			{
				static_assert(std::is_base_of<Cullable, T>::value,
					"component is not cullable");

				auto* Result = Scene->GetComponents<T>();
				Cull[T::BaseId()] = Result;
				return Result;
			}
			template <typename T>
			Rest::Pool<Component*>* RemoveCull()
			{
				static_assert(std::is_base_of<Cullable, T>::value,
					"component is not cullable");

				auto It = Cull.find(T::BaseId());
				if (It != Cull.end())
					Cull.erase(It);

				return nullptr;
			}
		};

		class THAWK_OUT SceneGraph : public Rest::Object
		{
			friend Renderer;
			friend Component;
			friend Entity;

		public:
			struct Desc
			{
				bool EnableHDR = false;
				float RenderQuality = 1.0f;
				uint64_t EntityCount = 1ll << 15;
				uint64_t ComponentCount = 1ll << 16;
				Compute::Simulator::Desc Simulator;
				Graphics::GraphicsDevice* Device = nullptr;
				Rest::EventQueue* Queue = nullptr;
				ShaderCache* Cache = nullptr;
			};

			struct Thread
			{
				std::atomic<std::thread::id> Id;
				std::atomic<int> State;
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
				std::mutex Global;
			} Sync;

			struct
			{
				std::unordered_map<std::string, MutationCallback> Callbacks;
				std::mutex Safe;
				bool Invoked;
			} Mutation;

			struct
			{
				Graphics::DepthStencilState* DepthStencil;
				Graphics::RasterizerState* Rasterizer;
				Graphics::BlendState* Blend;
				Graphics::SamplerState* Sampler;
			} Image;

		protected:
			Graphics::MultiRenderTarget2D* Surface = nullptr;
			Graphics::StructureBuffer* Structure = nullptr;
			Compute::Simulator* Simulator = nullptr;
			std::unordered_map<uint64_t, Rest::Pool<Component*>> Components;
			std::vector<Material> Materials;
			std::vector<std::string> Names;
			std::vector<Event*> Events;
			Rest::Pool<Component*> Pending;
			Rest::Pool<Entity*> Entities;
			Component* Camera = nullptr;
			Desc Conf;

		public:
			Viewer View;

		public:
			SceneGraph(const Desc& I);
			virtual ~SceneGraph() override;
			void Configure(const Desc& Conf);
			void Submit();
			void RenderGBuffer(Rest::Timer* Time, RenderOpt Options);
			void RenderDepthLinear(Rest::Timer* Time);
			void RenderDepthCubic(Rest::Timer* Time);
			void Render(Rest::Timer* Time);
			void Update(Rest::Timer* Time);
			void Simulation(Rest::Timer* Time);
			void Synchronize(Rest::Timer* Time);
			void RemoveMaterial(uint64_t MaterialId);
			void RemoveEntity(Entity* Entity, bool Release);
			void SetMutation(const std::string& Name, const MutationCallback& Callback);
			void SetCamera(Entity* Camera);
			void CloneEntities(Entity* Instance, std::vector<Entity*>* Array);
			void RestoreViewBuffer(Viewer* View);
			void SortEntitiesBackToFront();
			void SortEntitiesBackToFront(uint64_t Section);
			void SortEntitiesBackToFront(Rest::Pool<Component*>* Array);
			void Redistribute();
			void Reindex();
			void ExpandMaterialStructure();
			void Lock();
			void Unlock();
			void ResizeBuffers();
			void SwapSurface(Graphics::MultiRenderTarget2D* NewSurface);
			void SetView(const Compute::Matrix4x4& View, const Compute::Matrix4x4& Projection, const Compute::Vector3& Position, float Distance, bool Upload);
			void SetSurface();
			void SetSurfaceCleared();
			void SetMaterialName(uint64_t Material, const std::string& Name);
			void ClearSurface();
			void ClearSurfaceColor();
			void ClearSurfaceDepth();
			void GetTargetDesc(Graphics::RenderTarget2D::Desc* Result);
			void GetTargetDesc(Graphics::MultiRenderTarget2D::Desc* Result);
			void GetTargetFormat(Graphics::Format* Result, uint64_t Size);
			Material* AddMaterial(const std::string& Name, const Material& Material);
			Entity* CloneEntities(Entity* Value);
			Entity* FindNamedEntity(const std::string& Name);
			Entity* FindEntityAt(Compute::Vector3 Position, float Radius);
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
			std::vector<Entity*> FindEntitiesAt(Compute::Vector3 Position, float Radius);
			std::vector<Entity*> FindTaggedEntities(uint64_t Tag);
			bool IsEntityVisible(Entity* Entity, Compute::Matrix4x4 ViewProjection);
			bool IsEntityVisible(Entity* Entity, Compute::Matrix4x4 ViewProjection, Compute::Vector3 ViewPosition, float DrawDistance);
			bool AddEntity(Entity* Entity);
			bool Denotify();
			uint64_t GetMaterialCount();
			uint64_t GetEntityCount();
			uint64_t GetComponentCount(uint64_t Section);
			bool HasEntity(Entity* Entity);
			bool HasEntity(uint64_t Entity);
			Graphics::MultiRenderTarget2D* GetSurface();
			Graphics::StructureBuffer* GetStructure();
			Graphics::GraphicsDevice* GetDevice();
			Rest::EventQueue* GetQueue();
			Compute::Simulator* GetSimulator();
			ShaderCache* GetCache();
			Desc& GetConf();

		protected:
			void InvokeMutation(Entity* Target, Component* Base, bool Push);
			void BeginThread(ThreadId Thread);
			void EndThread(ThreadId Thread);
			void DispatchEvents();
			void RegisterEntity(Entity* In);
			bool UnregisterEntity(Entity* In);
			Entity* CloneEntity(Entity* Entity);

		public:
			template <typename T>
			void SortEntitiesBackToFront()
			{
				SortEntitiesBackToFront(T::BaseId());
			}
			template <typename T>
			bool Notify(Component* To, const T& Value)
			{
				if (!Queue)
					return false;

				Event* Message = new Event();
				Message->Type = typeid(T).hash_code();
				Message->Foreach = false;
				Message->Root = To;
				Message->Context = malloc(sizeof(T));
				memcpy(Message->Context, &Value, sizeof(T));

				return Queue->Event<Event>(Message);
			}
			template <typename T>
			bool NotifyEach(Component* To, const T& Value)
			{
				if (!Conf.Queue)
					return false;

				Event* Message = new Event();
				Message->Type = typeid(T).hash_code();
				Message->Foreach = true;
				Message->Root = To;
				Message->Context = malloc(sizeof(T));
				memcpy(Message->Context, &Value, sizeof(T));

				return Conf.Queue->Event<Event>(Message);
			}
			template <typename T>
			uint64_t GetEntityCount()
			{
				return GetComponents(T::BaseId())->Count();
			}
			template <typename T>
			Entity* GetLastEntity()
			{
				auto* Array = GetComponents(T::BaseId());
				if (Array->Empty())
					return nullptr;

				Component* Value = GetComponent(Array->Count() - 1, T::BaseId());
				if (Value != nullptr)
					return Value->Root;

				return nullptr;
			}
			template <typename T>
			Entity* GetEntity()
			{
				Component* Value = GetComponent(0, T::BaseId());
				if (Value != nullptr)
					return Value->Root;

				return nullptr;
			}
			template <typename T>
			Rest::Pool<Component*>* GetComponents()
			{
				return GetComponents(T::BaseId());
			}
			template <typename T>
			T* GetComponent()
			{
				return (T*)GetComponent(0, T::BaseId());
			}
			template <typename T>
			T* GetLastComponent()
			{
				auto* Array = GetComponents(T::BaseId());
				if (Array->Empty())
					return nullptr;

				return (T*)GetComponent(Array->Count() - 1, T::BaseId());
			}
		};

		class THAWK_OUT ContentManager : public Rest::Object
		{
		private:
			std::unordered_map<std::string, AssetDocker*> Dockers;
			std::unordered_map<std::string, AssetResource*> Assets;
			std::unordered_map<int64_t, FileProcessor*> Processors;
			std::unordered_map<Rest::FileStream*, int64_t> Streams;
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
			bool Cache(FileProcessor* Root, const std::string& Path, void* Resource);
			AssetResource* FindAsset(const std::string& Path);
			AssetResource* FindAsset(void* Resource);
			Graphics::GraphicsDevice* GetDevice();
			Rest::EventQueue* GetQueue();
			std::string GetEnvironment();

		public:
			template <typename T>
			T* Load(const std::string& Path, ContentMap* Keys)
			{
				return (T*)LoadForward(Path, GetProcessor<T>(), Keys);
			}
			template <typename T>
			bool LoadAsync(const std::string& Path, ContentMap* Keys, const std::function<void(class ContentManager*, T*)>& Callback)
			{
				if (!Queue)
					return false;

				ContentMap* Map = Keys ? new ContentMap(*Keys) : nullptr;
				return Queue->Task<ContentManager>(this, [this, Path, Callback, Map](Rest::EventQueue*, Rest::EventArgs*)
				{
					T* Result = (T*)LoadForward(Path, GetProcessor<T>(), Map);
					if (Callback)
						Callback(this, Result);
					delete Map;
				});
			}
			template <typename T>
			bool Save(const std::string& Path, T* Object, ContentMap* Keys)
			{
				return SaveForward(Path, GetProcessor<T>(), Object, Keys);
			}
			template <typename T>
			bool SaveAsync(const std::string& Path, T* Object, ContentMap* Keys, const SaveCallback& Callback)
			{
				if (!Queue)
					return false;

				ContentMap* Map = Keys ? new ContentMap(*Keys) : nullptr;
				return Queue->Task<ContentManager>(this, [this, Path, Callback, Object, Map](Rest::EventQueue*, Rest::EventArgs*)
				{
					bool Result = SaveForward(Path, GetProcessor<T>(), Object, Map);
					if (Callback)
						Callback(this, Result);
					delete Map;
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
			FileProcessor* GetProcessor()
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

		private:
			void* LoadForward(const std::string& Path, FileProcessor* Processor, ContentMap* Keys);
			void* LoadStreaming(const std::string& Path, FileProcessor* Processor, ContentMap* Keys);
			bool SaveForward(const std::string& Path, FileProcessor* Processor, void* Object, ContentMap* Keys);
		};

		class THAWK_OUT Application : public Rest::Object
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
			uint64_t Workers = 0;

		public:
			Audio::AudioDevice* Audio = nullptr;
			Graphics::GraphicsDevice* Renderer = nullptr;
			Graphics::Activity* Activity = nullptr;
			Script::VMManager* VM = nullptr;
			Rest::EventQueue* Queue = nullptr;
			ContentManager* Content = nullptr;
			SceneGraph* Scene = nullptr;
			ShaderCache* Shaders = nullptr;
			ApplicationState State = ApplicationState_Terminated;

		public:
			Application(Desc* I);
			virtual ~Application() override;
			virtual void KeyEvent(Graphics::KeyCode Key, Graphics::KeyMod Mod, int Virtual, int Repeat, bool Pressed);
			virtual void InputEvent(char* Buffer, int Length);
			virtual void WheelEvent(int X, int Y, bool Normal);
			virtual void WindowEvent(Graphics::WindowState NewState, int X, int Y);
			virtual void Render(Rest::Timer* Time);
			virtual void Update(Rest::Timer* Time);
			virtual void Initialize(Desc* I);
			virtual void* GetGUI();
			void Restate(ApplicationState Value);
			void Enqueue(const std::function<void(Rest::Timer*)>& Callback, double Limit = 0);
			void Run(Desc* I);

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