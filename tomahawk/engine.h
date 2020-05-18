#ifndef THAWK_ENGINE_H
#define THAWK_ENGINE_H

#include "graphics.h"
#include "audio.h"
#include "script.h"
#include <atomic>
#include <cstdarg>
#define THAWK_COMPONENT(Identification, ClassName) \
virtual const char* Name() override { return #ClassName; } \
virtual UInt64 Id() override { return Identification; } \
static const char* BaseName() { return #ClassName; } \
static UInt64 BaseId() { return Identification; }
#define THAWK_COMPONENT_TABLE(Identification, ClassName) \
virtual const char* Name() { return #ClassName; } \
virtual UInt64 Id() { return Identification; } \
static const char* BaseName() { return #ClassName; } \
static UInt64 BaseId() { return Identification; }

namespace Tomahawk
{
	namespace Engine
	{
		typedef std::unordered_map<std::string, struct ContentKey> ContentMap;

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

		enum ComponentId
		{
			ComponentId_Empty,
			ComponentId_Slider_Constraint,
			ComponentId_Rigidbody,
			ComponentId_Audio_Source,
			ComponentId_Audio_Listener,
			ComponentId_Acceleration,
			ComponentId_Key_Animator,
			ComponentId_Skin_Animator,
			ComponentId_Element_Animator,
			ComponentId_Free_Look,
			ComponentId_Fly,
			ComponentId_Model,
			ComponentId_Skinned_Model,
			ComponentId_Point_Light,
			ComponentId_Spot_Light,
			ComponentId_Line_Light,
			ComponentId_Probe_Light,
			ComponentId_Element_System,
			ComponentId_Camera,
			ComponentId_Count
		};

		enum RendererId
		{
			RendererId_Empty,
			RendererId_Model,
			RendererId_Skinned_Model,
			RendererId_Depth,
			RendererId_Light,
			RendererId_Probe,
			RendererId_Element_System,
			RendererId_Image,
			RendererId_Reflections,
			RendererId_Depth_Of_Field,
			RendererId_Emission,
			RendererId_Glitch,
			RendererId_Ambient_Occlusion,
			RendererId_Indirect_Occlusion,
			RendererId_Tone,
            RendererId_GUI,
			RendererId_Count
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
			UInt64 Length = 0;
			UInt64 Offset = 0;
		};

		struct THAWK_OUT ContentKey
		{
			ContentType Type;
			std::string String;
			Int64 Integer;
			Float64 Number;
			bool Boolean;
			void* Pointer;

			ContentKey();
			explicit ContentKey(const std::string& Value);
			explicit ContentKey(Int64 Value);
			explicit ContentKey(Float64 Value);
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
			std::function<void(Rest::Timer*)> Callback;
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
			Int64 Frame = -1;
			Int64 Clip = -1;
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

		struct THAWK_OUT TSurface
		{
			Graphics::Texture2D* Diffuse = nullptr;
			Graphics::Texture2D* Normal = nullptr;
			Graphics::Texture2D* Surface = nullptr;
			Compute::Vector3 Diffusion = 1;
			Compute::Vector2 TexCoord = 1;
			UInt64 Material = 0;
		};

		struct THAWK_OUT Viewer
		{
			Compute::Matrix4x4 InvViewProjection;
			Compute::Matrix4x4 ViewProjection;
			Compute::Matrix4x4 Projection;
			Compute::Vector3 ViewPosition;
			Compute::Vector3 Position;
			Compute::Vector3 RealPosition;
			float ViewDistance = 0.0f;
			RenderSystem* Renderer = nullptr;
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
			static bool Pack(Rest::Document* V, Float64 Value);
			static bool Pack(Rest::Document* V, Int64 Value);
			static bool Pack(Rest::Document* V, LFloat64 Value);
			static bool Pack(Rest::Document* V, UInt64 Value);
			static bool Pack(Rest::Document* V, const char* Value);
			static bool Pack(Rest::Document* V, const Compute::Vector2& Value);
			static bool Pack(Rest::Document* V, const Compute::Vector3& Value);
			static bool Pack(Rest::Document* V, const Compute::Vector4& Value);
			static bool Pack(Rest::Document* V, const Compute::Matrix4x4& Value);
			static bool Pack(Rest::Document* V, const AnimatorState& Value);
			static bool Pack(Rest::Document* V, const SpawnerProperties& Value);
			static bool Pack(Rest::Document* V, const Compute::SkinAnimatorClip& Value);
			static bool Pack(Rest::Document* V, const Compute::KeyAnimatorClip& Value);
			static bool Pack(Rest::Document* V, const Compute::AnimatorKey& Value);
			static bool Pack(Rest::Document* V, const Compute::ElementVertex& Value);
			static bool Pack(Rest::Document* V, const Compute::Joint& Value);
			static bool Pack(Rest::Document* V, const Compute::Vertex& Value);
			static bool Pack(Rest::Document* V, const Compute::InfluenceVertex& Value);
			static bool Pack(Rest::Document* V, const Rest::TickTimer& Value);
			static bool Pack(Rest::Document* V, const std::string& Value);
			static bool Pack(Rest::Document* V, const std::vector<bool>& Value);
			static bool Pack(Rest::Document* V, const std::vector<int>& Value);
			static bool Pack(Rest::Document* V, const std::vector<unsigned int>& Value);
			static bool Pack(Rest::Document* V, const std::vector<long>& Value);
			static bool Pack(Rest::Document* V, const std::vector<unsigned long>& Value);
			static bool Pack(Rest::Document* V, const std::vector<float>& Value);
			static bool Pack(Rest::Document* V, const std::vector<Float64>& Value);
			static bool Pack(Rest::Document* V, const std::vector<Int64>& Value);
			static bool Pack(Rest::Document* V, const std::vector<LFloat64>& Value);
			static bool Pack(Rest::Document* V, const std::vector<UInt64>& Value);
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
			static bool Pack(Rest::Document* V, const std::vector<Compute::InfluenceVertex>& Value);
			static bool Pack(Rest::Document* V, const std::vector<Rest::TickTimer>& Value);
			static bool Pack(Rest::Document* V, const std::vector<std::string>& Value);
			static bool Unpack(Rest::Document* V, bool* O);
			static bool Unpack(Rest::Document* V, int* O);
			static bool Unpack(Rest::Document* V, unsigned int* O);
			static bool Unpack(Rest::Document* V, long* O);
			static bool Unpack(Rest::Document* V, unsigned long* O);
			static bool Unpack(Rest::Document* V, float* O);
			static bool Unpack(Rest::Document* V, Float64* O);
			static bool Unpack(Rest::Document* V, Int64* O);
			static bool Unpack(Rest::Document* V, LFloat64* O);
			static bool Unpack(Rest::Document* V, UInt64* O);
			static bool Unpack(Rest::Document* V, Compute::Vector2* O);
			static bool Unpack(Rest::Document* V, Compute::Vector3* O);
			static bool Unpack(Rest::Document* V, Compute::Vector4* O);
			static bool Unpack(Rest::Document* V, Compute::Matrix4x4* O);
			static bool Unpack(Rest::Document* V, AnimatorState* O);
			static bool Unpack(Rest::Document* V, SpawnerProperties* O);
			static bool Unpack(Rest::Document* V, Compute::SkinAnimatorClip* O);
			static bool Unpack(Rest::Document* V, Compute::KeyAnimatorClip* O);
			static bool Unpack(Rest::Document* V, Compute::AnimatorKey* O);
			static bool Unpack(Rest::Document* V, Compute::ElementVertex* O);
			static bool Unpack(Rest::Document* V, Compute::Joint* O);
			static bool Unpack(Rest::Document* V, Compute::Vertex* O);
			static bool Unpack(Rest::Document* V, Compute::InfluenceVertex* O);
			static bool Unpack(Rest::Document* V, Rest::TickTimer* O);
			static bool Unpack(Rest::Document* V, std::string* O);
			static bool Unpack(Rest::Document* V, std::vector<bool>* O);
			static bool Unpack(Rest::Document* V, std::vector<int>* O);
			static bool Unpack(Rest::Document* V, std::vector<unsigned int>* O);
			static bool Unpack(Rest::Document* V, std::vector<long>* O);
			static bool Unpack(Rest::Document* V, std::vector<unsigned long>* O);
			static bool Unpack(Rest::Document* V, std::vector<float>* O);
			static bool Unpack(Rest::Document* V, std::vector<Float64>* O);
			static bool Unpack(Rest::Document* V, std::vector<Int64>* O);
			static bool Unpack(Rest::Document* V, std::vector<LFloat64>* O);
			static bool Unpack(Rest::Document* V, std::vector<UInt64>* O);
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
			static bool Unpack(Rest::Document* V, std::vector<Compute::InfluenceVertex>* O);
			static bool Unpack(Rest::Document* V, std::vector<Rest::TickTimer>* O);
			static bool Unpack(Rest::Document* V, std::vector<std::string>* O);
		};

		class THAWK_OUT Event
		{
			friend SceneGraph;

		private:
			Component* Root = nullptr;
			void* Context = nullptr;
			UInt64 Type = 0;
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
			virtual void* Load(Rest::FileStream* Stream, UInt64 Length, UInt64 Offset, ContentArgs* Keys);
			virtual bool Save(Rest::FileStream* Stream, void* Instance, ContentArgs* Keys);
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
			virtual void OnLoad(ContentManager* Content, Rest::Document* Node);
			virtual void OnSave(ContentManager* Content, Rest::Document* Node);
			virtual void OnAwake(Component* New);
			virtual void OnAsleep();
			virtual void OnSynchronize(Rest::Timer* Time);
			virtual void OnUpdate(Rest::Timer* Time);
			virtual void OnEvent(Event* Value);
			virtual Component* OnClone();
            Entity* GetEntity();
			void SetActive(bool Enabled);
			bool IsActive();

        public:
            THAWK_COMPONENT_TABLE(ComponentId_Empty, Component);
		};

		class THAWK_OUT Entity : public Rest::Object
		{
			friend SceneGraph;

		protected:
			std::unordered_map<UInt64, Component*> Components;
            SceneGraph* Scene;

		public:
			Compute::Transform* Transform;
			std::string Name;
			Int64 Self, Tag;

		public:
			Entity(SceneGraph* Ref);
			virtual ~Entity() override;
			void RemoveComponent(UInt64 Id);
			void RemoveChilds();
			void SetScene(SceneGraph* NewScene);
			std::unordered_map<UInt64, Component*>::iterator First();
			std::unordered_map<UInt64, Component*>::iterator Last();
			Component* AddComponent(Component* In);
			Component* GetComponent(UInt64 Id);
			UInt64 GetComponentCount();
			SceneGraph* GetScene();

		public:
			template <typename In> void RemoveComponent()
			{
				RemoveComponent(In::BaseId());
			}
			template <typename In> In* AddComponent()
			{
				Component* Component = new In(this);
				Component->Active = true;
				return (In*)AddComponent(Component);
			}
			template <typename In> In* GetComponent()
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
            RenderSystem* System = nullptr;
			unsigned int Stride = 0;
			unsigned int Offset = 0;
			UInt64 Flag = 0;

		public:
            bool Active = true;
            bool Priority = true;

		public:
			Renderer(RenderSystem* Lab, UInt64 iFlag);
			virtual ~Renderer() override;
			virtual void OnLoad(ContentManager* Content, Rest::Document* Node) { }
			virtual void OnSave(ContentManager* Content, Rest::Document* Node) { }
			virtual void OnResizeBuffers() { }
			virtual void OnInitialize() { }
			virtual void OnRender(Rest::Timer* TimeStep) { }
			virtual void OnDepthRender(Rest::Timer* TimeStep) { }
			virtual void OnCubicDepthRender(Rest::Timer* TimeStep) { }
			virtual void OnCubicDepthRender(Rest::Timer* TimeStep, Compute::Matrix4x4* ViewProjection) { }
			virtual void OnPhaseRender(Rest::Timer* TimeStep) { }
			virtual void OnRelease() { }
			void SetRenderer(RenderSystem* NewSystem);
			void RenderCubicDepth(Rest::Timer* Time, Compute::Matrix4x4 Projection, Compute::Vector4 Position);
			void RenderDepth(Rest::Timer* Time, Compute::Matrix4x4 View, Compute::Matrix4x4 Projection, Compute::Vector4 Position);
			void RenderPhase(Rest::Timer* Time, Compute::Matrix4x4 View, Compute::Matrix4x4 Projection, Compute::Vector4 Position);
			bool Is(UInt64 Flag);
            RenderSystem* GetRenderer();
			UInt64 Type();

		public:
			static Rest::Object* Abstract(RenderSystem* Lab, UInt64 iFlag);
		};

		class THAWK_OUT IntervalRenderer : public Renderer
		{
		protected:
			Rest::TickTimer Timer;

		public:
			IntervalRenderer(RenderSystem* Lab, UInt64 Flag);
			virtual ~IntervalRenderer() override;
			virtual void OnIntervalRender(Rest::Timer* Time) { }
			virtual void OnImmediateRender(Rest::Timer* Time) { }
			virtual void OnIntervalDepthRender(Rest::Timer* Time) { }
			virtual void OnImmediateDepthRender(Rest::Timer* Time) { }
			virtual void OnIntervalCubicDepthRender(Rest::Timer* Time, Compute::Matrix4x4* ViewProjection) { }
			virtual void OnImmediateCubicDepthRender(Rest::Timer* Time, Compute::Matrix4x4* ViewProjection) { }
			virtual void OnIntervalPhaseRender(Rest::Timer* Time) { }
			virtual void OnImmediatePhaseRender(Rest::Timer* Time) { }
			void OnRender(Rest::Timer* Time) override;
			void OnDepthRender(Rest::Timer* Time) override;
			void OnCubicDepthRender(Rest::Timer* Time, Compute::Matrix4x4* ViewProjection) override;
			void OnPhaseRender(Rest::Timer* Time) override;
		};

		class THAWK_OUT RenderSystem : public Rest::Object
		{
		protected:
			Graphics::ElementBuffer* QuadVertex;
			Graphics::ElementBuffer* SphereVertex;
			Graphics::ElementBuffer* SphereIndex;
			std::vector<Renderer*> RenderStages;
			Graphics::GraphicsDevice* Device;
            SceneGraph* Scene;

		public:
			RenderSystem(Graphics::GraphicsDevice* Device);
			virtual ~RenderSystem() override;
			void SetScene(SceneGraph* NewScene);
			void RemoveRenderStage(Renderer* In);
			void RemoveRenderStageByType(UInt64 Type);
			Renderer* AddRenderStage(Renderer* In);
			Renderer* AddRenderStageByType(UInt64 Type);
			Renderer* GetRenderStage(UInt64 Type);
			Graphics::ElementBuffer* VertexQuad();
			Graphics::ElementBuffer* VertexSphere();
			Graphics::ElementBuffer* IndexSphere();
            std::vector<Renderer*>* GetRenderStages();
            Graphics::GraphicsDevice* GetDevice();
            SceneGraph* GetScene();
		};

		class THAWK_OUT SceneGraph : public Rest::Object
		{
			friend Renderer;
			friend Component;
			friend Entity;

		public:
		    struct Desc
            {
                float RenderQuality = 1.0f;
                UInt64 ComponentTypes = ComponentId_Count;
                UInt64 EntityCount = 1ll << 15;
                UInt64 ComponentCount = 1ll << 16;
                bool EnableSoftBodies = false;
                Graphics::GraphicsDevice* Device = nullptr;
                Rest::EventQueue* Queue = nullptr;
            };

        private:
			struct Thread
			{
				std::atomic<std::thread::id> Id;
				std::atomic<int> State;
			};

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

		protected:
            Graphics::MultiRenderTarget2D* Surface = nullptr;
            Graphics::StructureBuffer* Structure = nullptr;
            Graphics::GraphicsDevice* Device = nullptr;
            Compute::Simulator* Simulator = nullptr;
            Rest::EventQueue* Queue = nullptr;
			std::vector<Rest::Pool<Component*>> Components;
			std::vector<Graphics::Material> Materials;
			std::vector<Event*> Events;
            Rest::Pool<Component*> Pending;
            Rest::Pool<Entity*> Entities;
			Component* Camera = nullptr;
			float RenderQuality = 1.0f;

		public:
			Viewer View;

		public:
            SceneGraph(const Desc& I);
			virtual ~SceneGraph() override;
			void Configure(const Desc& Conf);
			void Render(Rest::Timer* Time);
			void Update(Rest::Timer* Time);
			void Simulation(Rest::Timer* Time);
			void Synchronize(Rest::Timer* Time);
			void Rescale(const Compute::Vector3& Scale);
			void RandomizeMaterial(Graphics::Material& Material);
			void RemoveMaterial(UInt64 MaterialId);
			void RemoveEntity(Entity* Entity, bool Release);
			void SetCamera(Entity* Camera);
			void CloneEntities(Entity* Instance, std::vector<Entity*>* Array);
			void AddMaterial(const Graphics::Material& Material);
			void RestoreViewBuffer(Viewer* View);
			void SortEntitiesBackToFront();
			void Redistribute();
			void Reindex();
			void ExpandMaterialStructure();
			void Lock();
			void Unlock();
			void ResizeBuffers();
			void SetSurface(Graphics::MultiRenderTarget2D* NewSurface);
			Entity* CloneEntities(Entity* Value);
			Entity* FindNamedEntity(const std::string& Name);
			Entity* FindEntityAt(Compute::Vector3 Position, float Radius);
			Entity* FindTaggedEntity(UInt64 Tag);
			Entity* GetEntity(UInt64 Entity);
			Entity* GetLastEntity();
			Component* GetComponent(UInt64 Component, UInt64 Section);
			Component* GetCamera();
			RenderSystem* GetRenderer();
			Viewer GetCameraViewer();
			Graphics::Material& CloneMaterial(UInt64 Material);
			Graphics::Material& GetMaterial(UInt64 Material);
			Graphics::Material& GetMaterialStandartLit();
            Rest::Pool<Component*>* GetComponents(UInt64 Section);
			std::vector<Entity*> FindParentFreeEntities(Entity* Entity);
			std::vector<Entity*> FindNamedEntities(const std::string& Name);
			std::vector<Entity*> FindEntitiesAt(Compute::Vector3 Position, float Radius);
			std::vector<Entity*> FindTaggedEntities(UInt64 Tag);
			bool IsEntityVisible(Entity* Entity, Compute::Matrix4x4 ViewProjection);
			bool IsEntityVisible(Entity* Entity, Compute::Matrix4x4 ViewProjection, Compute::Vector3 ViewPosition, float DrawDistance);
			bool AddEntity(Entity* Entity);
			bool Denotify();
			float GetRenderQuality();
			UInt64 GetEntityStorageCount();
			UInt64 GetComponentStorageCount();
			UInt64 GetComponentTypesCount();
			UInt64 GetMaterialCount();
			UInt64 GetEntityCount();
			UInt64 GetComponentCount(UInt64 Section);
			UInt64 HasMaterial(UInt64 Material);
			UInt64 HasEntity(Entity* Entity);
			UInt64 HasEntity(UInt64 Entity);
            Graphics::MultiRenderTarget2D* GetSurface();
            Graphics::StructureBuffer* GetStructure();
            Graphics::GraphicsDevice* GetDevice();
            Compute::Simulator* GetSimulator();
            Rest::EventQueue* GetQueue();

		protected:
			void RenderCubicDepth(Rest::Timer* Time, Compute::Matrix4x4 Projection, Compute::Vector4 Position);
			void RenderDepth(Rest::Timer* Time, Compute::Matrix4x4 View, Compute::Matrix4x4 Projection, Compute::Vector4 Position);
			void RenderPhase(Rest::Timer* Time, Compute::Matrix4x4 View, Compute::Matrix4x4 Projection, Compute::Vector4 Position);
			void BeginThread(ThreadId Thread);
			void EndThread(ThreadId Thread);
			void DispatchEvents();
			void RegisterEntity(Entity* In);
			bool UnregisterEntity(Entity* In);
			Entity* CloneEntity(Entity* Entity);

		public:
			template <typename T> bool Notify(Component* To, const T& Value)
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
			template <typename T> bool NotifyEach(Component* To, const T& Value)
			{
				if (!Queue)
					return false;

				Event* Message = new Event();
				Message->Type = typeid(T).hash_code();
				Message->Foreach = true;
				Message->Root = To;
				Message->Context = malloc(sizeof(T));
				memcpy(Message->Context, &Value, sizeof(T));

				return Queue->Event<Event>(Message);
			}
			template <typename T> UInt64 GetEntityCount()
			{
				return Components[T::BaseId()].Count();
			}
			template <typename T> Entity* GetLastEntity()
			{
				if (Components[T::BaseId()].Empty())
					return nullptr;

				Component* Value = GetComponent(Components[T::BaseId()].Count() - 1, T::BaseId());
				if (Value != nullptr)
					return Value->Root;

				return nullptr;
			}
			template <typename T> Entity* GetEntity()
			{
				Component* Value = GetComponent(0, T::BaseId());
				if (Value != nullptr)
					return Value->Root;

				return nullptr;
			}
			template <typename T> T* GetComponent()
			{
				return (T*)GetComponent(0, T::BaseId());
			}
			template <typename T> T* GetLastComponent()
			{
				if (Components[T::BaseId()].Empty())
					return nullptr;

				return (T*)GetComponent(Components[T::BaseId()].Count() - 1, T::BaseId());
			}
		};

		class THAWK_OUT ContentManager : public Rest::Object
		{
		private:
			std::unordered_map<std::string, AssetDocker*> Dockers;
			std::unordered_map<std::string, AssetResource*> Assets;
			std::unordered_map<Int64, FileProcessor*> Processors;
			std::unordered_map<Rest::FileStream*, Int64> Streams;
            Graphics::GraphicsDevice* Device;
			std::string Environment, Base;

		public:
			ContentManager(Graphics::GraphicsDevice* NewDevice);
			virtual ~ContentManager() override;
			void InvalidateDockers();
			void InvalidateCache();
			void SetEnvironment(const std::string& Path);
			bool Import(const std::string& Path);
			bool Export(const std::string& Path, const std::string& Directory, const std::string& Name = "");
			bool Cache(FileProcessor* Root, const std::string& Path, void* Resource);
			AssetResource* FindAsset(const std::string& Path);
			AssetResource* FindAsset(void* Resource);
            Graphics::GraphicsDevice* GetDevice();
			std::string GetEnvironment();

		public:
			template <typename T> T* Load(const std::string& Path, ContentMap* Keys = nullptr)
			{
				return (T*)LoadForward(Path, GetProcessor<T>(), Keys);
			}
			template <typename T> bool Save(const std::string& Path, T* Object, ContentMap* Keys = nullptr)
			{
				return SaveForward(Path, GetProcessor<T>(), Object, Keys);
			}
			template <typename T> bool RemoveProcessor()
			{
				auto It = Processors.find(typeid(T).hash_code());
				if (It == Processors.end())
					return false;

				if (It->second != nullptr)
					delete It->second;

				Processors.erase(It);
				return true;
			}
			template <typename V, typename T> V* AddProcessor()
			{
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

				return Instance;
			}
			template <typename T> FileProcessor* GetProcessor()
			{
				auto It = Processors.find(typeid(T).hash_code());
				if (It != Processors.end())
					return It->second;

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
                UInt64 TaskWorkersCount = 0;
                UInt64 EventWorkersCount = 0;
                Float64 FrameLimit = 0;
                Float64 MaxFrames = 60;
                Float64 MinFrames = 10;
                unsigned int Usage =
                        ApplicationUse_Graphics_Module |
                        ApplicationUse_Activity_Module |
                        ApplicationUse_Audio_Module |
                        ApplicationUse_AngelScript_Module |
                        ApplicationUse_Content_Module;
                bool DisableCursor = false;
            };

        private:
			static Application* Host;
			UInt64 Workers = 0;

		public:
			Audio::AudioDevice* Audio = nullptr;
			Graphics::GraphicsDevice* Renderer = nullptr;
			Graphics::Activity* Activity = nullptr;
			Script::VMManager* VM = nullptr;
			Rest::EventQueue* Queue = nullptr;
			ContentManager* Content = nullptr;
            SceneGraph* Scene = nullptr;
			ApplicationState State = ApplicationState_Terminated;

		public:
			Application(Desc* I);
			virtual ~Application() override;
			virtual void OnKeyState(Graphics::KeyCode Key, Graphics::KeyMod Mod, int Virtual, int Repeat, bool Pressed);
            virtual void OnInput(char* Buffer, int Length);
            virtual void OnCursorWheelState(int X, int Y, bool Normal);
            virtual void OnWindowState(Graphics::WindowState NewState, int X, int Y);
            virtual void OnInteract(Engine::Renderer* GUI);
			virtual void OnRender(Rest::Timer* Time);
			virtual void OnUpdate(Rest::Timer* Time);
			virtual void OnInitialize(Desc* I);
            void Run(Desc* I);
            void Restate(ApplicationState Value);
            void Enqueue(const std::function<void(Rest::Timer*)>& Callback, Float64 Limit = 0);

		private:
			static void Callee(Rest::EventQueue* Queue, Rest::EventArgs* Args);
			
		public:
			static Application* Get();
		};
	}
}
#endif