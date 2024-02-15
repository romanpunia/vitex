#ifndef VI_ENGINE_H
#define VI_ENGINE_H
#include "graphics.h"
#include "audio.h"
#include "scripting.h"
#include <atomic>
#include <cstdarg>

namespace Vitex
{
	namespace Engine
	{
		namespace GUI
		{
			class Context;
		}

		typedef std::pair<Graphics::Texture3D*, class Component*> VoxelMapping;
		typedef Graphics::DepthTarget2D LinearDepthMap;
		typedef Graphics::DepthTargetCube CubicDepthMap;
		typedef Core::Vector<LinearDepthMap*> CascadedDepthMap;
		typedef std::function<void(Core::Timer*, struct Viewer*)> RenderCallback;
		typedef std::function<void(class ContentManager*, bool)> SaveCallback;
		typedef std::function<void(const std::string_view&, Core::VariantArgs&)> MessageCallback;
		typedef std::function<bool(class Component*, const Compute::Vector3&)> RayCallback;
		typedef std::function<bool(Graphics::RenderTarget*)> TargetCallback;

		class Series;

		class SceneGraph;

		class Application;

		class ContentManager;

		class Entity;

		class Component;

		class Drawable;

		class Processor;

		class PrimitiveCache;

		class RenderSystem;

		class Material;

		class SkinModel;

		enum
		{
			MAX_STACK_DEPTH = 4,
			EXIT_JUMP = 9600,
			EXIT_RESTART = EXIT_JUMP * 2
		};

		enum class ApplicationSet
		{
			GraphicsSet = 1 << 0,
			ActivitySet = 1 << 1,
			AudioSet = 1 << 3,
			ScriptSet = 1 << 4,
			ContentSet = 1 << 5,
			NetworkSet = 1 << 6
		};

		enum class ApplicationState
		{
			Terminated,
			Staging,
			Active,
			Restart
		};

		enum class RenderOpt
		{
			None = 0,
			Transparent = 1,
			Static = 2,
			Additive = 4,
			Backfaces = 8
		};

		enum class RenderCulling
		{
			Linear,
			Cubic,
			Disable
		};

		enum class RenderState
		{
			Geometric,
			Voxelization,
			Linearization,
			Cubic
		};

		enum class GeoCategory
		{
			Opaque,
			Transparent,
			Additive,
			Count
		};

		enum class BufferType
		{
			Index = 0,
			Vertex = 1
		};

		enum class TargetType
		{
			Main = 0,
			Secondary = 1,
			Count
		};

		enum class VoxelType
		{
			Diffuse = 0,
			Normal = 1,
			Surface = 2
		};

		enum class EventTarget
		{
			Scene = 0,
			Entity = 1,
			Component = 2,
			Listener = 3
		};

		enum class ActorSet
		{
			None = 0,
			Update = 1 << 0,
			Synchronize = 1 << 1,
			Animate = 1 << 2,
			Message = 1 << 3,
			Cullable = 1 << 4,
			Drawable = 1 << 5
		};

		enum class ActorType
		{
			Update,
			Synchronize,
			Animate,
			Message,
			Count
		};

		enum class TaskType
		{
			Processing,
			Rendering,
			Count
		};

		enum class ComposerTag
		{
			Component,
			Renderer,
			Effect,
			Filter
		};

		enum class RenderBufferType
		{
			Animation,
			Render,
			View
		};

		inline ActorSet operator |(ActorSet A, ActorSet B)
		{
			return static_cast<ActorSet>(static_cast<size_t>(A) | static_cast<size_t>(B));
		}
		inline ApplicationSet operator |(ApplicationSet A, ApplicationSet B)
		{
			return static_cast<ApplicationSet>(static_cast<size_t>(A) | static_cast<size_t>(B));
		}
		inline RenderOpt operator |(RenderOpt A, RenderOpt B)
		{
			return static_cast<RenderOpt>(static_cast<size_t>(A) | static_cast<size_t>(B));
		}

		struct VI_OUT Ticker
		{
		private:
			float Time;

		public:
			float Delay;

		public:
			Ticker() noexcept;
			bool TickEvent(float ElapsedTime);
			float GetTime();
		};

		struct VI_OUT Event
		{
			Core::String Name;
			Core::VariantArgs Args;

			Event(const std::string_view& NewName) noexcept;
			Event(const std::string_view& NewName, const Core::VariantArgs& NewArgs) noexcept;
			Event(const std::string_view& NewName, Core::VariantArgs&& NewArgs) noexcept;
			Event(const Event& Other) noexcept;
			Event(Event&& Other) noexcept;
			Event& operator= (const Event& Other) noexcept;
			Event& operator= (Event&& Other) noexcept;
		};

		struct VI_OUT BatchData
		{
			Graphics::ElementBuffer* InstanceBuffer;
			void* GeometryBuffer;
			Material* BatchMaterial;
			size_t InstancesCount;
		};

		struct VI_OUT AssetCache
		{
			Core::String Path;
			void* Resource = nullptr;
		};

		struct VI_OUT AssetArchive
		{
			Core::Stream* Stream = nullptr;
			Core::String Path;
			size_t Length = 0;
			size_t Offset = 0;
		};

		struct VI_OUT IdxSnapshot
		{
			Core::UnorderedMap<Entity*, size_t> To;
			Core::UnorderedMap<size_t, Entity*> From;
		};

		struct VI_OUT VisibilityQuery
		{
			GeoCategory Category = GeoCategory::Opaque;
			bool BoundaryVisible = false;
			bool QueryPixels = false;
		};

		struct VI_OUT AnimatorState
		{
			bool Paused = false;
			bool Looped = false;
			bool Blended = false;
			float Duration = -1.0f;
			float Rate = 1.0f;
			float Time = 0.0f;
			int64_t Frame = -1;
			int64_t Clip = -1;

			float GetTimeline(Core::Timer* Time) const;
			float GetSecondsDuration() const;
			float GetProgressTotal() const;
			float GetProgress() const;
			bool IsPlaying() const;
		};

		struct VI_OUT SpawnerProperties
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

		struct VI_OUT Viewer
		{
			RenderSystem* Renderer = nullptr;
			RenderCulling Culling = RenderCulling::Linear;
			Compute::Matrix4x4 CubicViewProjection[6];
			Compute::Matrix4x4 InvViewProjection;
			Compute::Matrix4x4 ViewProjection;
			Compute::Matrix4x4 Projection;
			Compute::Matrix4x4 View;
			Compute::Vector3 InvPosition;
			Compute::Vector3 Position;
			Compute::Vector3 Rotation;
			float FarPlane = 0.0f;
			float NearPlane = 0.0f;
			float Ratio = 0.0f;
			float Fov = 0.0f;

			void Set(const Compute::Matrix4x4& View, const Compute::Matrix4x4& Projection, const Compute::Vector3& Position, float Fov, float Ratio, float Near, float Far, RenderCulling Type);
			void Set(const Compute::Matrix4x4& View, const Compute::Matrix4x4& Projection, const Compute::Vector3& Position, const Compute::Vector3& Rotation, float Fov, float Ratio, float Near, float Far, RenderCulling Type);
		};

		struct VI_OUT Attenuation
		{
			float Radius = 10.0f;
			float C1 = 0.6f;
			float C2 = 0.6f;
		};

		struct VI_OUT Subsurface
		{
			Compute::Vector4 Emission = { 0.0f, 0.0f, 0.0f, 0.0f };
			Compute::Vector4 Metallic = { 0.0f, 0.0f, 0.0f, 0.0f };
			Compute::Vector3 Diffuse = { 1.0f, 1.0f, 1.0f };
			float Fresnel = 0.0f;
			Compute::Vector3 Scatter = { 0.1f, 16.0f, 0.0f };
			float Transparency = 0.0f;
			Compute::Vector3 Padding;
			float Bias = 0.0f;
			Compute::Vector2 Roughness = { 1.0f, 0.0f };
			float Refraction = 0.0f;
			float Environment = 0.0f;
			Compute::Vector2 Occlusion = { 1.0f, 0.0f };
			float Radius = 0.0f;
			float Height = 0.0f;
		};

		struct VI_OUT SparseIndex
		{
			Core::Pool<Component*> Data;
			Compute::Cosmos Index;
		};

		struct VI_OUT AnimationBuffer
		{
			Compute::Matrix4x4 Offsets[Graphics::JOINTS_SIZE];
			Compute::Vector3 Padding;
			float Animated = 0.0f;
		};

		struct VI_OUT RenderBuffer
		{
			struct Instance
			{
				Compute::Matrix4x4 Transform;
				Compute::Matrix4x4 World;
				Compute::Vector2 TexCoord;
				float Diffuse = 0.0f;
				float Normal = 0.0f;
				float Height = 0.0f;
				float MaterialId = 0.0f;
			};

			Compute::Matrix4x4 Transform;
			Compute::Matrix4x4 World;
			Compute::Vector4 TexCoord;
			float Diffuse = 0.0f;
			float Normal = 0.0f;
			float Height = 0.0f;
			float MaterialId = 0.0f;
		};

		struct VI_OUT ViewBuffer
		{
			Compute::Matrix4x4 InvViewProj;
			Compute::Matrix4x4 ViewProj;
			Compute::Matrix4x4 Proj;
			Compute::Matrix4x4 View;
			Compute::Vector3 Position;
			float Far = 1000.0f;
			Compute::Vector3 Direction;
			float Near = 0.1f;
		};

		struct VI_OUT PoseNode
		{
			Compute::Vector3 Position;
			Compute::Vector3 Scale = Compute::Vector3::One();
			Compute::Quaternion Rotation;
		};

		struct VI_OUT PoseData
		{
			PoseNode Frame;
			PoseNode Offset;
			PoseNode Default;
		};

		struct VI_OUT PoseMatrices
		{
			Compute::Matrix4x4 Data[Graphics::JOINTS_SIZE];
		};

		struct VI_OUT PoseBuffer
		{
			Core::UnorderedMap<Graphics::SkinMeshBuffer*, PoseMatrices> Matrices;
			Core::UnorderedMap<size_t, PoseData> Offsets;

			void Fill(SkinModel* Mesh);
			void Fill(Compute::Joint& Next);
		};

		class ContentException : public Core::SystemException
		{
		public:
			VI_OUT ContentException();
			VI_OUT ContentException(const std::string_view& Message);
			VI_OUT const char* type() const noexcept override;
		};

		template <typename V>
		using ExpectsContent = Core::Expects<V, ContentException>;

		template <typename T, typename Executor = Core::ParallelExecutor>
		using ExpectsPromiseContent = Core::BasicPromise<ExpectsContent<T>, Executor>;

		class VI_OUT_TS Series
		{
		public:
			static void Pack(Core::Schema* V, bool Value);
			static void Pack(Core::Schema* V, int32_t Value);
			static void Pack(Core::Schema* V, int64_t Value);
			static void Pack(Core::Schema* V, uint32_t Value);
			static void Pack(Core::Schema* V, uint64_t Value);
			static void Pack(Core::Schema* V, float Value);
			static void Pack(Core::Schema* V, double Value);
			static void Pack(Core::Schema* V, const Compute::Vector2& Value);
			static void Pack(Core::Schema* V, const Compute::Vector3& Value);
			static void Pack(Core::Schema* V, const Compute::Vector4& Value);
			static void Pack(Core::Schema* V, const Compute::Quaternion& Value);
			static void Pack(Core::Schema* V, const Compute::Matrix4x4& Value);
			static void Pack(Core::Schema* V, const Attenuation& Value);
			static void Pack(Core::Schema* V, const AnimatorState& Value);
			static void Pack(Core::Schema* V, const SpawnerProperties& Value);
			static void Pack(Core::Schema* V, const Compute::KeyAnimatorClip& Value);
			static void Pack(Core::Schema* V, const Compute::AnimatorKey& Value);
			static void Pack(Core::Schema* V, const Compute::SkinAnimatorKey& Value);
			static void Pack(Core::Schema* V, const Compute::ElementVertex& Value);
			static void Pack(Core::Schema* V, const Compute::Joint& Value);
			static void Pack(Core::Schema* V, const Compute::Vertex& Value);
			static void Pack(Core::Schema* V, const Compute::SkinVertex& Value);
			static void Pack(Core::Schema* V, const Ticker& Value);
			static void Pack(Core::Schema* V, const std::string_view& Value);
			static void Pack(Core::Schema* V, const Core::Vector<bool>& Value);
			static void Pack(Core::Schema* V, const Core::Vector<int32_t>& Value);
			static void Pack(Core::Schema* V, const Core::Vector<int64_t>& Value);
			static void Pack(Core::Schema* V, const Core::Vector<uint32_t>& Value);
			static void Pack(Core::Schema* V, const Core::Vector<uint64_t>& Value);
			static void Pack(Core::Schema* V, const Core::Vector<float>& Value);
			static void Pack(Core::Schema* V, const Core::Vector<double>& Value);
			static void Pack(Core::Schema* V, const Core::Vector<Compute::Vector2>& Value);
			static void Pack(Core::Schema* V, const Core::Vector<Compute::Vector3>& Value);
			static void Pack(Core::Schema* V, const Core::Vector<Compute::Vector4>& Value);
			static void Pack(Core::Schema* V, const Core::Vector<Compute::Matrix4x4>& Value);
			static void Pack(Core::Schema* V, const Core::Vector<AnimatorState>& Value);
			static void Pack(Core::Schema* V, const Core::Vector<SpawnerProperties>& Value);
			static void Pack(Core::Schema* V, const Core::Vector<Compute::KeyAnimatorClip>& Value);
			static void Pack(Core::Schema* V, const Core::Vector<Compute::AnimatorKey>& Value);
			static void Pack(Core::Schema* V, const Core::Vector<Compute::ElementVertex>& Value);
			static void Pack(Core::Schema* V, const Core::Vector<Compute::Joint>& Value);
			static void Pack(Core::Schema* V, const Core::Vector<Compute::Vertex>& Value);
			static void Pack(Core::Schema* V, const Core::Vector<Compute::SkinVertex>& Value);
			static void Pack(Core::Schema* V, const Core::Vector<Ticker>& Value);
			static void Pack(Core::Schema* V, const Core::Vector<Core::String>& Value);
			static void Pack(Core::Schema* V, const Core::UnorderedMap<size_t, size_t>& Value);
			static bool Unpack(Core::Schema* V, bool* O);
			static bool Unpack(Core::Schema* V, int32_t* O);
			static bool Unpack(Core::Schema* V, int64_t* O);
			static bool Unpack(Core::Schema* V, uint32_t* O);
			static bool Unpack(Core::Schema* V, uint64_t* O);
			static bool Unpack(Core::Schema* V, float* O);
			static bool Unpack(Core::Schema* V, double* O);
			static bool Unpack(Core::Schema* V, Compute::Vector2* O);
			static bool Unpack(Core::Schema* V, Compute::Vector3* O);
			static bool Unpack(Core::Schema* V, Compute::Vector4* O);
			static bool Unpack(Core::Schema* V, Compute::Quaternion* O);
			static bool Unpack(Core::Schema* V, Compute::Matrix4x4* O);
			static bool Unpack(Core::Schema* V, Attenuation* O);
			static bool Unpack(Core::Schema* V, AnimatorState* O);
			static bool Unpack(Core::Schema* V, SpawnerProperties* O);
			static bool Unpack(Core::Schema* V, Compute::KeyAnimatorClip* O);
			static bool Unpack(Core::Schema* V, Compute::AnimatorKey* O);
			static bool Unpack(Core::Schema* V, Compute::SkinAnimatorKey* O);
			static bool Unpack(Core::Schema* V, Compute::ElementVertex* O);
			static bool Unpack(Core::Schema* V, Compute::Joint* O);
			static bool Unpack(Core::Schema* V, Compute::Vertex* O);
			static bool Unpack(Core::Schema* V, Compute::SkinVertex* O);
			static bool Unpack(Core::Schema* V, Ticker* O);
			static bool Unpack(Core::Schema* V, Core::String* O);
			static bool Unpack(Core::Schema* V, Core::Vector<bool>* O);
			static bool Unpack(Core::Schema* V, Core::Vector<int32_t>* O);
			static bool Unpack(Core::Schema* V, Core::Vector<int64_t>* O);
			static bool Unpack(Core::Schema* V, Core::Vector<uint32_t>* O);
			static bool Unpack(Core::Schema* V, Core::Vector<uint64_t>* O);
			static bool Unpack(Core::Schema* V, Core::Vector<float>* O);
			static bool Unpack(Core::Schema* V, Core::Vector<double>* O);
			static bool Unpack(Core::Schema* V, Core::Vector<Compute::Vector2>* O);
			static bool Unpack(Core::Schema* V, Core::Vector<Compute::Vector3>* O);
			static bool Unpack(Core::Schema* V, Core::Vector<Compute::Vector4>* O);
			static bool Unpack(Core::Schema* V, Core::Vector<Compute::Matrix4x4>* O);
			static bool Unpack(Core::Schema* V, Core::Vector<AnimatorState>* O);
			static bool Unpack(Core::Schema* V, Core::Vector<SpawnerProperties>* O);
			static bool Unpack(Core::Schema* V, Core::Vector<Compute::KeyAnimatorClip>* O);
			static bool Unpack(Core::Schema* V, Core::Vector<Compute::AnimatorKey>* O);
			static bool Unpack(Core::Schema* V, Core::Vector<Compute::ElementVertex>* O);
			static bool Unpack(Core::Schema* V, Core::Vector<Compute::Joint>* O);
			static bool Unpack(Core::Schema* V, Core::Vector<Compute::Vertex>* O);
			static bool Unpack(Core::Schema* V, Core::Vector<Compute::SkinVertex>* O);
			static bool Unpack(Core::Schema* V, Core::Vector<Ticker>* O);
			static bool Unpack(Core::Schema* V, Core::Vector<Core::String>* O);
			static bool Unpack(Core::Schema* V, Core::UnorderedMap<size_t, size_t>* O);

		public:

		};

		class VI_OUT_TS Parallel
		{
		public:
			static Core::Promise<void> Enqueue(Core::TaskCallback&& Callback);
			static Core::Vector<Core::Promise<void>> EnqueueAll(Core::Vector<Core::TaskCallback>&& Callbacks);
			static void Wait(Core::Promise<void>&& Value);
			static void WailAll(Core::Vector<Core::Promise<void>>&& Values);
			static size_t GetThreadIndex();
			static size_t GetThreads();

		public:
			template <typename Iterator, typename Function>
			static Core::Vector<Core::Promise<void>> ForEach(Iterator Begin, Iterator End, Function&& Callback)
			{
				Core::Vector<Core::Promise<void>> Tasks;
				size_t Size = End - Begin;

				if (!Size)
					return Tasks;

				size_t Threads = std::max<size_t>(1, GetThreads());
				size_t Step = Size / Threads;
				size_t Remains = Size % Threads;
				Tasks.reserve(Threads);

				while (Begin != End)
				{
					auto Offset = Begin;
					Begin += Remains > 0 ? --Remains, Step + 1 : Step;
					Tasks.emplace_back(Enqueue(std::bind(std::for_each<Iterator, Function>, Offset, Begin, Callback)));
				}

				return Tasks;
			}
			template <typename Iterator, typename InitFunction, typename ElementFunction>
			static Core::Vector<Core::Promise<void>> Distribute(Iterator Begin, Iterator End, InitFunction&& InitCallback, ElementFunction&& ElementCallback)
			{
				Core::Vector<Core::Promise<void>> Tasks;
				size_t Size = End - Begin;
				if (!Size)
				{
					InitCallback((size_t)0);
					return Tasks;
				}

				size_t Threads = std::max<size_t>(1, GetThreads());
				size_t Step = Size / Threads;
				size_t Remains = Size % Threads;
				size_t Remainder = Remains;
				size_t Index = 0, Counting = 0;
				auto Start = Begin;

				while (Start != End)
				{
					Start += Remainder > 0 ? --Remainder, Step + 1 : Step;
					++Counting;
				}

				Tasks.reserve(Counting);
				InitCallback(Counting);

				while (Begin != End)
				{
					auto Offset = Begin;
					auto Bound = std::bind(ElementCallback, Index++, std::placeholders::_1);
					Begin += Remains > 0 ? --Remains, Step + 1 : Step;
					Tasks.emplace_back(Enqueue([Offset, Begin, Bound]()
					{
						std::for_each<Iterator, decltype(Bound)>(Offset, Begin, Bound);
					}));
				}

				return Tasks;
			}
		};

		class VI_OUT AssetFile final : public Core::Reference<AssetFile>
		{
		private:
			char* Buffer;
			size_t Length;

		public:
			AssetFile(char* SrcBuffer, size_t SrcSize) noexcept;
			~AssetFile() noexcept;
			char* GetBuffer();
			size_t Size();
		};

		class VI_OUT Model final : public Core::Reference<Model>
		{
		public:
			Core::Vector<Graphics::MeshBuffer*> Meshes;
			Compute::Vector4 Max;
			Compute::Vector4 Min;

		public:
			Model() noexcept;
			~Model() noexcept;
			void Cleanup();
			Graphics::MeshBuffer* FindMesh(const std::string_view& Name);
		};

		class VI_OUT SkinModel final : public Core::Reference<SkinModel>
		{
		public:
			Core::Vector<Graphics::SkinMeshBuffer*> Meshes;
			Compute::Joint Skeleton;
			Compute::Matrix4x4 InvTransform;
			Compute::Matrix4x4 Transform;
			Compute::Vector4 Max;
			Compute::Vector4 Min;

		public:
			SkinModel() noexcept;
			~SkinModel() noexcept;
			bool FindJoint(const std::string_view& Name, Compute::Joint* Output);
			bool FindJoint(size_t Index, Compute::Joint* Output);
			void Synchronize(PoseBuffer* Map);
			void Cleanup();
			Graphics::SkinMeshBuffer* FindMesh(const std::string_view& Name);

		private:
			void Synchronize(PoseBuffer* Map, Compute::Joint& Next, const Compute::Matrix4x4& ParentOffset);
		};

		class VI_OUT SkinAnimation final : public Core::Reference<SkinAnimation>
		{
		private:
			Core::Vector<Compute::SkinAnimatorClip> Clips;

		public:
			SkinAnimation(Core::Vector<Compute::SkinAnimatorClip>&& Data) noexcept;
			~SkinAnimation() = default;
			const Core::Vector<Compute::SkinAnimatorClip>& GetClips();
			bool IsValid();
		};

		class VI_OUT Material final : public Core::Reference<Material>
		{
			friend Series;
			friend RenderSystem;
			friend SceneGraph;

		private:
			Graphics::Texture2D* DiffuseMap;
			Graphics::Texture2D* NormalMap;
			Graphics::Texture2D* MetallicMap;
			Graphics::Texture2D* RoughnessMap;
			Graphics::Texture2D* HeightMap;
			Graphics::Texture2D* OcclusionMap;
			Graphics::Texture2D* EmissionMap;
			Core::String Name;
			SceneGraph* Scene;

		public:
			Subsurface Surface;
			size_t Slot;

		public:
			Material(SceneGraph* NewScene = nullptr) noexcept;
			Material(const Material& Other) noexcept;
			~Material() noexcept;
			void SetName(const std::string_view& Value);
			const Core::String& GetName() const;
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
			SceneGraph* GetScene() const;
		};

		class VI_OUT Processor : public Core::Reference<Processor>
		{
			friend ContentManager;

		protected:
			ContentManager* Content;

		public:
			Processor(ContentManager* NewContent) noexcept;
			virtual ~Processor() noexcept;
			virtual void Free(AssetCache* Asset);
			virtual ExpectsContent<Core::Unique<void>> Duplicate(AssetCache* Asset, const Core::VariantArgs& Keys);
			virtual ExpectsContent<Core::Unique<void>> Deserialize(Core::Stream* Stream, size_t Offset, const Core::VariantArgs& Keys);
			virtual ExpectsContent<void> Serialize(Core::Stream* Stream, void* Instance, const Core::VariantArgs& Keys);
			ContentManager* GetContent() const;
		};

		class VI_OUT Component : public Core::Reference<Component>
		{
			friend Core::Reference<Component>;
			friend SceneGraph;
			friend RenderSystem;
			friend Entity;

		protected:
			Entity* Parent;

		private:
			size_t Set;
			bool Indexed;
			bool Active;

		public:
			virtual void Serialize(Core::Schema* Node);
			virtual void Deserialize(Core::Schema* Node);
			virtual void Activate(Component* New);
			virtual void Deactivate();
			virtual void Synchronize(Core::Timer* Time);
			virtual void Animate(Core::Timer* Time);
			virtual void Update(Core::Timer* Time);
			virtual void Message(const std::string_view& Name, Core::VariantArgs& Args);
			virtual void Movement();
			virtual size_t GetUnitBounds(Compute::Vector3& Min, Compute::Vector3& Max) const;
			virtual float GetVisibility(const Viewer& View, float Distance) const;
			virtual Core::Unique<Component> Copy(Entity* New) const = 0;
			Entity* GetEntity() const;
			void SetActive(bool Enabled);
			bool IsDrawable() const;
			bool IsCullable() const;
			bool IsActive() const;

		protected:
			Component(Entity* Ref, ActorSet Rule) noexcept;
			virtual ~Component() noexcept;

		public:
			VI_COMPONENT_ROOT("base_component");
		};

		class VI_OUT Entity final : public Core::Reference<Entity>
		{
			friend Core::Reference<Entity>;
			friend SceneGraph;
			friend RenderSystem;

		private:
			struct
			{
				Compute::Matrix4x4 Box;
				Compute::Vector3 Min;
				Compute::Vector3 Max;
				float Distance = 0.0f;
				float Visibility = 0.0f;
			} Snapshot;

			struct
			{
				Core::UnorderedMap<uint64_t, Component*> Components;
				Core::String Name;
			} Type;

		private:
			Compute::Transform* Transform;
			SceneGraph* Scene;
			bool Active;

		public:
			void SetName(const std::string_view& Value);
			void SetRoot(Entity* Parent);
			void UpdateBounds();
			void RemoveComponent(uint64_t Id);
			void RemoveChilds();
			Component* AddComponent(Core::Unique<Component> In);
			Component* GetComponent(uint64_t Id);
			size_t GetComponentsCount() const;
			SceneGraph* GetScene() const;
			Entity* GetParent() const;
			Entity* GetChild(size_t Index) const;
			Compute::Transform* GetTransform() const;
			const Compute::Matrix4x4& GetBox() const;
			const Compute::Vector3& GetMin() const;
			const Compute::Vector3& GetMax() const;
			const Core::String& GetName() const;
			size_t GetChildsCount() const;
			float GetVisibility(const Viewer& Base) const;
			bool IsActive() const;
			Compute::Vector3 GetRadius3() const;
			float GetRadius() const;

		private:
			Entity(SceneGraph* NewScene) noexcept;
			~Entity() noexcept;

		public:
			Core::UnorderedMap<uint64_t, Component*>::iterator begin()
			{
				return Type.Components.begin();
			}
			Core::UnorderedMap<uint64_t, Component*>::iterator end()
			{
				return Type.Components.end();
			}

		public:
			template <typename In>
			void RemoveComponent()
			{
				RemoveComponent(In::GetTypeId());
			}
			template <typename In>
			In* AddComponent()
			{
				return (In*)AddComponent(new In(this));
			}
			template <typename In>
			In* GetComponent()
			{
				return (In*)GetComponent(In::GetTypeId());
			}

		public:
			template <typename T>
			static bool Sortout(T* A, T* B)
			{
				return A->Parent->Snapshot.Distance - B->Parent->Snapshot.Distance < 0;
			}
		};

		class VI_OUT Drawable : public Component
		{
			friend SceneGraph;

		protected:
			Core::UnorderedMap<void*, Material*> Materials;

		private:
			GeoCategory Category;
			uint64_t Source;

		public:
			float Overlapping;
			bool Static;

		public:
			Drawable(Entity* Ref, ActorSet Rule, uint64_t Hash) noexcept;
			virtual ~Drawable() noexcept;
			virtual void Message(const std::string_view& Name, Core::VariantArgs& Args) override;
			virtual void Movement() override;
			virtual Core::Unique<Component> Copy(Entity* New) const override = 0;
			void ClearMaterials();
			bool SetCategory(GeoCategory NewCategory);
			bool SetMaterial(void* Instance, Material* Value);
			bool SetMaterial(Material* Value);
			GeoCategory GetCategory() const;
			int64_t GetSlot(void* Surface);
			int64_t GetSlot();
			Material* GetMaterial(void* Surface);
			Material* GetMaterial();
			const Core::UnorderedMap<void*, Material*>& GetMaterials();

		public:
			VI_COMPONENT("drawable_component");
		};

		class VI_OUT Renderer : public Core::Reference<Renderer>
		{
			friend SceneGraph;

		protected:
			RenderSystem* System;

		public:
			bool Active;

		public:
			Renderer(RenderSystem* Lab) noexcept;
			virtual ~Renderer() noexcept;
			virtual void Serialize(Core::Schema* Node);
			virtual void Deserialize(Core::Schema* Node);
			virtual void ClearCulling();
			virtual void ResizeBuffers();
			virtual void Activate();
			virtual void Deactivate();
			virtual void BeginPass(Core::Timer* Time);
			virtual void EndPass();
			virtual bool HasCategory(GeoCategory Category);
			virtual size_t RenderPrepass(Core::Timer* Time);
			virtual size_t RenderPass(Core::Timer* Time) = 0;
			void SetRenderer(RenderSystem* NewSystem);
			RenderSystem* GetRenderer() const;

		public:
			VI_COMPONENT_ROOT("base_renderer");
		};

		class VI_OUT RenderConstants final : public Core::Reference<RenderConstants>
		{
		private:
			struct
			{
				Graphics::ElementBuffer* Buffers[3] = { nullptr };
				Graphics::Shader* BasicEffect = nullptr;
				void* Pointers[3] = { nullptr };
				size_t Sizes[3] = { 0 };
			} Binding;

		private:
			Graphics::GraphicsDevice* Device;

		public:
			AnimationBuffer Animation;
			RenderBuffer Render;
			ViewBuffer View;

		public:
			RenderConstants(Graphics::GraphicsDevice* NewDevice) noexcept;
			~RenderConstants() noexcept;
			void SetConstantBuffers();
			void UpdateConstantBuffer(RenderBufferType Buffer);
			Graphics::Shader* GetBasicEffect() const;
			Graphics::GraphicsDevice* GetDevice() const;
			Graphics::ElementBuffer* GetConstantBuffer(RenderBufferType Buffer) const;
		};

		class VI_OUT RenderSystem final : public Core::Reference<RenderSystem>
		{
		public:
			struct RsIndex
			{
				Compute::Cosmos::Iterator Stack;
				Compute::Frustum6P Frustum;
				Compute::Bounding Bounds;
				Core::Vector<void*> Queue;
			} Indexing;

			struct RsState
			{
				friend RenderSystem;

			private:
				RenderState Target = RenderState::Geometric;
				RenderOpt Options = RenderOpt::None;
				size_t Top = 0;

			public:
				bool Is(RenderState State) const
				{
					return Target == State;
				}
				bool IsSet(RenderOpt Option) const
				{
					return (size_t)Options & (size_t)Option;
				}
				bool IsTop() const
				{
					return Top <= 1;
				}
				bool IsSubpass() const
				{
					return !IsTop();
				}
				RenderOpt GetOpts() const
				{
					return Options;
				}
				RenderState Get() const
				{
					return Target;
				}
			} State;

		protected:
			Core::Vector<Renderer*> Renderers;
			Graphics::GraphicsDevice* Device;
			Material* BaseMaterial;
			SceneGraph* Scene;
			Component* Owner;

		public:
			RenderConstants* Constants;
			Viewer View;
			size_t MaxQueries;
			size_t SortingFrequency;
			size_t OcclusionSkips;
			size_t OccluderSkips;
			size_t OccludeeSkips;
			float OccludeeScaling;
			float OverflowVisibility;
			float Threshold;
			bool OcclusionCulling;
			bool PreciseCulling;
			bool AllowInputLag;

		public:
			RenderSystem(SceneGraph* NewScene, Component* NewComponent) noexcept;
			~RenderSystem() noexcept;
			void SetView(const Compute::Matrix4x4& View, const Compute::Matrix4x4& Projection, const Compute::Vector3& Position, float Fov, float Ratio, float Near, float Far, RenderCulling Type);
			void ClearCulling();
			void RemoveRenderers();
			void RestoreViewBuffer(Viewer* View);
			void Remount(Renderer* Target);
			void Remount();
			void Mount();
			void Unmount();
			void MoveRenderer(uint64_t Id, size_t Offset);
			void RemoveRenderer(uint64_t Id);
			void RestoreOutput();
			void FreeShader(const std::string_view& Name, Graphics::Shader* Shader);
			void FreeShader(Graphics::Shader* Shader);
			void FreeBuffers(const std::string_view& Name, Graphics::ElementBuffer** Buffers);
			void FreeBuffers(Graphics::ElementBuffer** Buffers);
			void UpdateConstantBuffer(RenderBufferType Buffer);
			void ClearMaterials();
			void FetchVisibility(Component* Base, VisibilityQuery& Data);
			size_t Render(Core::Timer* Time, RenderState Stage, RenderOpt Options);
			bool TryInstance(Material* Next, RenderBuffer::Instance& Target);
			bool TryGeometry(Material* Next, bool WithTextures);
			bool HasCategory(GeoCategory Category);
			Graphics::ExpectsGraphics<Graphics::Shader*> CompileShader(Graphics::Shader::Desc& Desc, size_t BufferSize = 0);
			Graphics::ExpectsGraphics<Graphics::Shader*> CompileShader(const std::string_view& SectionName, size_t BufferSize = 0);
			Graphics::ExpectsGraphics<void> CompileBuffers(Graphics::ElementBuffer** Result, const std::string_view& Name, size_t ElementSize, size_t ElementsCount);
			Renderer* AddRenderer(Core::Unique<Renderer> In);
			Renderer* GetRenderer(uint64_t Id);
			bool GetOffset(uint64_t Id, size_t& Offset) const;
			Core::Vector<Renderer*>& GetRenderers();
			Graphics::MultiRenderTarget2D* GetMRT(TargetType Type) const;
			Graphics::RenderTarget2D* GetRT(TargetType Type) const;
			Graphics::Texture2D** GetMerger();
			Graphics::GraphicsDevice* GetDevice() const;
			Graphics::Shader* GetBasicEffect() const;
			RenderConstants* GetConstants() const;
			PrimitiveCache* GetPrimitives() const;
			SceneGraph* GetScene() const;
			Component* GetComponent() const;

		private:
			SparseIndex& GetStorageWrapper(uint64_t Section);
			void Watch(Core::Vector<Core::Promise<void>>&& Tasks);

		private:
			template <typename T, typename OverlapsFunction, typename MatchFunction>
			void DispatchQuerySync(Compute::Cosmos& Index, const OverlapsFunction& Overlaps, const MatchFunction& Match)
			{
				Indexing.Stack.clear();
				if (!Index.Empty())
					Indexing.Stack.push_back(Index.GetRoot());

				while (!Indexing.Stack.empty())
				{
					auto& Next = Index.GetNode(Indexing.Stack.back());
					Indexing.Stack.pop_back();

					if (Overlaps(Next.Bounds))
					{
						if (!Next.IsLeaf())
						{
							Indexing.Stack.push_back(Next.Left);
							Indexing.Stack.push_back(Next.Right);
						}
						else if (Next.Item != nullptr)
							Match((T*)Next.Item);
					}
				}
			}
			template <typename T, typename OverlapsFunction, typename MatchFunction>
			void DispatchQueryAsync(Compute::Cosmos& Index, const OverlapsFunction& Overlaps, const MatchFunction& Match)
			{
				Indexing.Stack.clear();
				Indexing.Queue.clear();

				if (!Index.Empty())
					Indexing.Stack.push_back(Index.GetRoot());

				while (!Indexing.Stack.empty())
				{
					auto& Next = Index.GetNode(Indexing.Stack.back());
					Indexing.Stack.pop_back();

					if (Overlaps(Next.Bounds))
					{
						if (!Next.IsLeaf())
						{
							Indexing.Stack.push_back(Next.Left);
							Indexing.Stack.push_back(Next.Right);
						}
						else if (Next.Item != nullptr)
							Indexing.Queue.push_back(Next.Item);
					}
				}

				if (Indexing.Queue.empty())
					return;

				Watch(Parallel::ForEach(Indexing.Queue.begin(), Indexing.Queue.end(), [Match](void* Item)
				{
					Match(Parallel::GetThreadIndex(), (T*)Item);
				}));
			}

		public:
			template <typename MatchFunction>
			void QueryBasicSync(uint64_t Id, MatchFunction&& Callback)
			{
				auto& Storage = GetStorageWrapper(Id);
				switch (View.Culling)
				{
					case RenderCulling::Linear:
					{
						auto Overlaps = [this](const Compute::Bounding& Bounds) { return Indexing.Frustum.OverlapsAABB(Bounds); };
						DispatchQuerySync<Component, decltype(Overlaps), decltype(Callback)>(Storage.Index, Overlaps, Callback);
						break;
					}
					case RenderCulling::Cubic:
					{
						auto Overlaps = [this](const Compute::Bounding& Bounds) { return Indexing.Bounds.Overlaps(Bounds); };
						DispatchQuerySync<Component, decltype(Overlaps), decltype(Callback)>(Storage.Index, Overlaps, Callback);
						break;
					}
					default:
						std::for_each(Storage.Data.Begin(), Storage.Data.End(), std::move(Callback));
						break;
				}
			}
			template <typename InitFunction, typename MatchFunction>
			void QueryBasicAsync(uint64_t Id, InitFunction&& InitCallback, MatchFunction&& ElementCallback)
			{
				auto& Storage = GetStorageWrapper(Id);
				switch (View.Culling)
				{
					case RenderCulling::Linear:
					{
						auto Overlaps = [this](const Compute::Bounding& Bounds) { return Indexing.Frustum.OverlapsAABB(Bounds); };
						InitCallback(Parallel::GetThreads());
						DispatchQueryAsync<Component, decltype(Overlaps), decltype(ElementCallback)>(Storage.Index, Overlaps, ElementCallback);
						break;
					}
					case RenderCulling::Cubic:
					{
						auto Overlaps = [this](const Compute::Bounding& Bounds) { return Indexing.Bounds.Overlaps(Bounds); };
						InitCallback(Parallel::GetThreads());
						DispatchQueryAsync<Component, decltype(Overlaps), decltype(ElementCallback)>(Storage.Index, Overlaps, ElementCallback);
						break;
					}
					default:
						if (!Storage.Data.Empty())
							Watch(Parallel::Distribute(Storage.Data.Begin(), Storage.Data.End(), std::move(InitCallback), std::move(ElementCallback)));
						break;
				}
			}
			template <typename T, typename MatchFunction>
			void QuerySync(MatchFunction&& Callback)
			{
				auto& Storage = GetStorageWrapper(T::GetTypeId());
				switch (View.Culling)
				{
					case RenderCulling::Linear:
					{
						auto Overlaps = [this](const Compute::Bounding& Bounds) { return Indexing.Frustum.OverlapsAABB(Bounds); };
						DispatchQuerySync<T, decltype(Overlaps), decltype(Callback)>(Storage.Index, Overlaps, Callback);
						break;
					}
					case RenderCulling::Cubic:
					{
						auto Overlaps = [this](const Compute::Bounding& Bounds) { return Indexing.Bounds.Overlaps(Bounds); };
						DispatchQuerySync<T, decltype(Overlaps), decltype(Callback)>(Storage.Index, Overlaps, Callback);
						break;
					}
					default:
						std::for_each(Storage.Data.Begin(), Storage.Data.End(), std::move(Callback));
						break;
				}
			}
			template <typename T, typename InitFunction, typename MatchFunction>
			void QueryAsync(InitFunction&& InitCallback, MatchFunction&& ElementCallback)
			{
				auto& Storage = GetStorageWrapper(T::GetTypeId());
				switch (View.Culling)
				{
					case RenderCulling::Linear:
					{
						auto Overlaps = [this](const Compute::Bounding& Bounds) { return Indexing.Frustum.OverlapsAABB(Bounds); };
						InitCallback(Parallel::GetThreads());
						DispatchQueryAsync<T, decltype(Overlaps), decltype(ElementCallback)>(Storage.Index, Overlaps, ElementCallback);
						break;
					}
					case RenderCulling::Cubic:
					{
						auto Overlaps = [this](const Compute::Bounding& Bounds) { return Indexing.Bounds.Overlaps(Bounds); };
						InitCallback(Parallel::GetThreads());
						DispatchQueryAsync<T, decltype(Overlaps), decltype(ElementCallback)>(Storage.Index, Overlaps, ElementCallback);
						break;
					}
					default:
						if (!Storage.Data.Empty())
							Watch(Parallel::Distribute(Storage.Data.Begin(), Storage.Data.End(), std::move(InitCallback), std::move(ElementCallback)));
						break;
				}
			}
			template <typename T>
			void RemoveRenderer()
			{
				RemoveRenderer(T::GetTypeId());
			}
			template <typename T, typename... Args>
			T* AddRenderer(Args&& ... Data)
			{
				return (T*)AddRenderer(new T(this, Data...));
			}
			template <typename T>
			T* GetRenderer()
			{
				return (T*)GetRenderer(T::GetTypeId());
			}
			template <typename In>
			int64_t GetOffset()
			{
				size_t Offset = 0;
				if (!GetOffset(In::GetTypeId(), Offset))
					return -1;

				return (int64_t)Offset;
			}
		};

		class VI_OUT_TS ShaderCache final : public Core::Reference<ShaderCache>
		{
		public:
			struct SCache
			{
				Graphics::Shader* Shader;
				size_t Count;
			};

		private:
			Core::UnorderedMap<Core::String, SCache> Cache;
			Graphics::GraphicsDevice* Device;
			std::mutex Exclusive;

		public:
			ShaderCache(Graphics::GraphicsDevice* Device) noexcept;
			~ShaderCache() noexcept;
			Graphics::ExpectsGraphics<Graphics::Shader*> Compile(const std::string_view& Name, const Graphics::Shader::Desc& Desc, size_t BufferSize = 0);
			Graphics::Shader* Get(const std::string_view& Name);
			Core::String Find(Graphics::Shader* Shader);
			const Core::UnorderedMap<Core::String, SCache>& GetCaches() const;
			bool Has(const std::string_view& Name);
			bool Free(const std::string_view& Name, Graphics::Shader* Shader = nullptr);
			void ClearCache();
		};

		class VI_OUT_TS PrimitiveCache final : public Core::Reference<PrimitiveCache>
		{
		public:
			struct SCache
			{
				Graphics::ElementBuffer* Buffers[2];
				size_t Count;
			};

		private:
			Core::UnorderedMap<Core::String, SCache> Cache;
			Graphics::GraphicsDevice* Device;
			Graphics::ElementBuffer* Sphere[2];
			Graphics::ElementBuffer* Cube[2];
			Graphics::ElementBuffer* Box[2];
			Graphics::ElementBuffer* SkinBox[2];
			Graphics::ElementBuffer* Quad;
			Model* BoxModel;
			SkinModel* SkinBoxModel;
			std::recursive_mutex Exclusive;

		public:
			PrimitiveCache(Graphics::GraphicsDevice* Device) noexcept;
			~PrimitiveCache() noexcept;
			Graphics::ExpectsGraphics<void> Compile(Graphics::ElementBuffer** Result, const std::string_view& Name, size_t ElementSize, size_t ElementsCount);
			bool Get(Graphics::ElementBuffer** Result, const std::string_view& Name);
			bool Has(const std::string_view& Name);
			bool Free(const std::string_view& Name, Graphics::ElementBuffer** Buffers);
			Core::String Find(Graphics::ElementBuffer** Buffer);
			Model* GetBoxModel();
			SkinModel* GetSkinBoxModel();
			Graphics::ElementBuffer* GetQuad();
			Graphics::ElementBuffer* GetSphere(BufferType Type);
			Graphics::ElementBuffer* GetCube(BufferType Type);
			Graphics::ElementBuffer* GetBox(BufferType Type);
			Graphics::ElementBuffer* GetSkinBox(BufferType Type);
			const Core::UnorderedMap<Core::String, SCache>& GetCaches() const;
			void GetSphereBuffers(Graphics::ElementBuffer** Result);
			void GetCubeBuffers(Graphics::ElementBuffer** Result);
			void GetBoxBuffers(Graphics::ElementBuffer** Result);
			void GetSkinBoxBuffers(Graphics::ElementBuffer** Result);
			void ClearCache();
		};

		class VI_OUT_TS ContentManager final : public Core::Reference<ContentManager>
		{
		private:
			Core::UnorderedMap<Core::String, Core::UnorderedMap<Processor*, AssetCache*>> Assets;
			Core::UnorderedMap<Core::String, AssetArchive*> Archives;
			Core::UnorderedMap<uint64_t, Processor*> Processors;
			Core::UnorderedMap<Core::Stream*, size_t> Streams;
			Graphics::GraphicsDevice* Device;
			Core::String Environment, Base;
			std::mutex Exclusive;
			size_t Queue;

		public:
			ContentManager(Graphics::GraphicsDevice* NewDevice) noexcept;
			~ContentManager() noexcept;
			void ClearCache();
			void ClearArchives();
			void ClearStreams();
			void ClearProcessors();
			void ClearPath(const std::string_view& Path);
			void SetEnvironment(const std::string_view& Path);
			void SetDevice(Graphics::GraphicsDevice* NewDevice);
			ExpectsContent<void> ImportArchive(const std::string_view& Path, bool ValidateChecksum);
			ExpectsContent<void> ExportArchive(const std::string_view& Path, const std::string_view& PhysicalDirectory, const std::string_view& VirtualDirectory = std::string_view());
			ExpectsContent<Core::Unique<void>> Load(Processor* Processor, const std::string_view& Path, const Core::VariantArgs& Keys);
			ExpectsContent<void> Save(Processor* Processor, const std::string_view& Path, void* Object, const Core::VariantArgs& Keys);
			ExpectsPromiseContent<Core::Unique<void>> LoadAsync(Processor* Processor, const std::string_view& Path, const Core::VariantArgs& Keys);
			ExpectsPromiseContent<void> SaveAsync(Processor* Processor, const std::string_view& Path, void* Object, const Core::VariantArgs& Keys);
			Processor* AddProcessor(Processor* Value, uint64_t Id);
			Processor* GetProcessor(uint64_t Id);
			AssetCache* FindCache(Processor* Target, const std::string_view& Path);
			AssetCache* FindCache(Processor* Target, void* Resource);
			const Core::UnorderedMap<uint64_t, Processor*>& GetProcessors() const;
			bool RemoveProcessor(uint64_t Id);
			void* TryToCache(Processor* Root, const std::string_view& Path, void* Resource);
			bool IsBusy();
			Graphics::GraphicsDevice* GetDevice() const;
			const Core::String& GetEnvironment() const;

		private:
			ExpectsContent<void*> LoadFromArchive(Processor* Processor, const std::string_view& Path, const Core::VariantArgs& Keys);
			void Enqueue();
			void Dequeue();

		public:
			template <typename T>
			ExpectsContent<Core::Unique<T>> Load(const std::string_view& Path, const Core::VariantArgs& Keys = Core::VariantArgs())
			{
				auto Result = Load(GetProcessor<T>(), Path, Keys);
				if (!Result)
					return ExpectsContent<T*>(Result.Error());

				return ExpectsContent<T*>((T*)*Result);
			}
			template <typename T>
			ExpectsPromiseContent<Core::Unique<T>> LoadAsync(const std::string_view& Path, const Core::VariantArgs& Keys = Core::VariantArgs())
			{
				Enqueue();
				Core::String TargetPath = Core::String(Path);
				return Core::Cotask<ExpectsContent<T*>>([this, TargetPath, Keys]()
				{
					auto Result = Load(GetProcessor<T>(), TargetPath, Keys);
					Dequeue();
					if (!Result)
						return ExpectsContent<T*>(Result.Error());

					return ExpectsContent<T*>((T*)*Result);
				});
			}
			template <typename T>
			ExpectsContent<void> Save(const std::string_view& Path, T* Object, const Core::VariantArgs& Keys = Core::VariantArgs())
			{
				return Save(GetProcessor<T>(), Path, Object, Keys);
			}
			template <typename T>
			ExpectsPromiseContent<void> SaveAsync(const std::string_view& Path, T* Object, const Core::VariantArgs& Keys = Core::VariantArgs())
			{
				return SaveAsync(GetProcessor<T>(), Path, (void*)Object, Keys);
			}
			template <typename T>
			bool RemoveProcessor()
			{
				return RemoveProcessor(typeid(T).hash_code());
			}
			template <typename V, typename T>
			V* AddProcessor()
			{
				return (V*)AddProcessor(new V(this), typeid(T).hash_code());
			}
			template <typename T>
			Processor* GetProcessor()
			{
				return GetProcessor(typeid(T).hash_code());
			}
			template <typename T>
			AssetCache* FindCache(const std::string_view& Path)
			{
				return FindCache(GetProcessor<T>(), Path);
			}
			template <typename T>
			AssetCache* FindCache(void* Resource)
			{
				return FindCache(GetProcessor<T>(), Resource);
			}
		};

		class VI_OUT_TS AppData final : public Core::Reference<AppData>
		{
		private:
			ContentManager* Content;
			Core::Schema* Data;
			Core::String Path;
			std::mutex Exclusive;

		public:
			AppData(ContentManager* Manager, const std::string_view& Path) noexcept;
			~AppData() noexcept;
			void Migrate(const std::string_view& Path);
			void SetKey(const std::string_view& Name, Core::Unique<Core::Schema> Value);
			void SetText(const std::string_view& Name, const std::string_view& Value);
			Core::Unique<Core::Schema> GetKey(const std::string_view& Name);
			Core::String GetText(const std::string_view& Name);
			bool Has(const std::string_view& Name);
			Core::Schema* GetSnapshot() const;

		private:
			bool ReadAppData(const std::string_view& Path);
			bool WriteAppData(const std::string_view& Path);
		};

		class VI_OUT SceneGraph final : public Core::Reference<SceneGraph>
		{
			friend RenderSystem;
			friend Renderer;
			friend Component;
			friend Entity;
			friend Drawable;

		public:
			struct VI_OUT Desc
			{
				struct Dependencies
				{
					Graphics::GraphicsDevice* Device = nullptr;
					Graphics::Activity* Activity = nullptr;
					Scripting::VirtualMachine* VM = nullptr;
					ContentManager* Content = nullptr;
					PrimitiveCache* Primitives = nullptr;
					ShaderCache* Shaders = nullptr;
					RenderConstants* Constants = nullptr;
				} Shared;

				Compute::Simulator::Desc Simulator;
				size_t StartMaterials = 1ll << 8;
				size_t StartEntities = 1ll << 8;
				size_t StartComponents = 1ll << 8;
				size_t GrowMargin = 128;
				size_t MaxUpdates = 256;
				size_t PointsSize = 256;
				size_t PointsMax = 4;
				size_t SpotsSize = 512;
				size_t SpotsMax = 8;
				size_t LinesSize = 1024;
				size_t LinesMax = 2;
				size_t VoxelsSize = 128;
				size_t VoxelsMax = 4;
				size_t VoxelsMips = 0;
				double GrowRate = 0.25f;
				float RenderQuality = 1.0f;
				bool EnableHDR = false;
				bool Mutations = false;

				void AddRef();
				void Release();
				static Desc Get(Application* Base);
			};

		private:
			struct
			{
				Graphics::MultiRenderTarget2D* MRT[(size_t)TargetType::Count * 2];
				Graphics::RenderTarget2D* RT[(size_t)TargetType::Count * 2];
				Graphics::Texture3D* VoxelBuffers[3];
				Graphics::ElementBuffer* MaterialBuffer;
				Graphics::DepthStencilState* DepthStencil;
				Graphics::RasterizerState* Rasterizer;
				Graphics::BlendState* Blend;
				Graphics::SamplerState* Sampler;
				Graphics::InputLayout* Layout;
				Graphics::Texture2D* Merger;
				Core::Vector<CubicDepthMap*> Points;
				Core::Vector<LinearDepthMap*> Spots;
				Core::Vector<CascadedDepthMap*> Lines;
				Core::Vector<VoxelMapping> Voxels;
			} Display;

			struct
			{
				std::atomic<Material*> Default;
				float Progress = 1.0f;
			} Loading;

			struct
			{
				Core::SingleQueue<Core::Promise<void>> Queue[(size_t)TaskType::Count];
				std::mutex Update[(size_t)TaskType::Count];
				bool IsRendering = false;
			} Tasking;

		protected:
			Core::UnorderedMap<Core::String, Core::UnorderedSet<MessageCallback*>> Listeners;
			Core::UnorderedMap<uint64_t, Core::UnorderedSet<Component*>> Changes;
			Core::UnorderedMap<uint64_t, SparseIndex*> Registry;
			Core::UnorderedMap<Component*, size_t> Incomplete;
			Core::SingleQueue<Core::TaskCallback> Transactions;
			Core::SingleQueue<Event> Events;
			Core::Pool<Component*> Actors[(size_t)ActorType::Count];
			Core::Pool<Material*> Materials;
			Core::Pool<Entity*> Entities;
			Core::Pool<Entity*> Dirty;
			Compute::Simulator* Simulator;
			std::atomic<Component*> Camera;
			std::atomic<bool> Active;
			std::mutex Exclusive;
			Desc Conf;

		public:
			struct SgStatistics
			{
				size_t Batching = 0;
				size_t Sorting = 0;
				size_t Instances = 0;
				size_t DrawCalls = 0;
			} Statistics;

		public:
			IdxSnapshot* Snapshot;

		public:
			SceneGraph(const Desc& I) noexcept;
			~SceneGraph() noexcept;
			void Configure(const Desc& Conf);
			void Actualize();
			void ResizeBuffers();
			void Submit();
			void Dispatch(Core::Timer* Time);
			void Publish(Core::Timer* Time);
			void PublishAndSubmit(Core::Timer* Time, float R, float G, float B, bool IsParallel);
			void DeleteMaterial(Core::Unique<Material> Value);
			void RemoveEntity(Core::Unique<Entity> Entity);
			void DeleteEntity(Core::Unique<Entity> Entity);
			void SetCamera(Entity* Camera);
			void RayTest(uint64_t Section, const Compute::Ray& Origin, const RayCallback& Callback);
			void ScriptHook(const std::string_view& Name = "main");
			void SetActive(bool Enabled);
			void SetMRT(TargetType Type, bool Clear);
			void SetRT(TargetType Type, bool Clear);
			void SwapMRT(TargetType Type, Graphics::MultiRenderTarget2D* New);
			void SwapRT(TargetType Type, Graphics::RenderTarget2D* New);
			void ClearMRT(TargetType Type, bool Color, bool Depth);
			void ClearRT(TargetType Type, bool Color, bool Depth);
			void Mutate(Entity* Parent, Entity* Child, const std::string_view& Type);
			void Mutate(Entity* Target, const std::string_view& Type);
			void Mutate(Component* Target, const std::string_view& Type);
			void Mutate(Material* Target, const std::string_view& Type);
			void MakeSnapshot(IdxSnapshot* Result);
			void Transaction(Core::TaskCallback&& Callback);
			void Watch(TaskType Type, Core::Promise<void>&& Awaitable);
			void Watch(TaskType Type, Core::Vector<Core::Promise<void>>&& Awaitables);
			void Await(TaskType Type);
			void ClearCulling();
			void ReserveMaterials(size_t Size);
			void ReserveEntities(size_t Size);
			void ReserveComponents(uint64_t Section, size_t Size);
			void GenerateDepthCascades(Core::Unique<CascadedDepthMap>* Result, uint32_t Size) const;
			bool GetVoxelBuffer(Graphics::Texture3D** In, Graphics::Texture3D** Out);
			bool PushEvent(const std::string_view& EventName, Core::VariantArgs&& Args, bool Propagate);
			bool PushEvent(const std::string_view& EventName, Core::VariantArgs&& Args, Component* Target);
			bool PushEvent(const std::string_view& EventName, Core::VariantArgs&& Args, Entity* Target);
			MessageCallback* SetListener(const std::string_view& Event, MessageCallback&& Callback);
			bool ClearListener(const std::string_view& Event, MessageCallback* Id);
			bool AddMaterial(Core::Unique<Material> Base);
			void LoadResource(uint64_t Id, Component* Context, const std::string_view& Path, const Core::VariantArgs& Keys, std::function<void(ExpectsContent<void*>&&)>&& Callback);
			Core::String FindResourceId(uint64_t Id, void* Resource);
			Material* GetInvalidMaterial();
			Material* AddMaterial();
			Material* CloneMaterial(Material* Base);
			Entity* GetEntity(size_t Entity);
			Entity* GetLastEntity();
			Entity* GetCameraEntity();
			Component* GetComponent(uint64_t Section, size_t Component);
			Component* GetCamera();
			RenderSystem* GetRenderer();
			Viewer GetCameraViewer() const;
			Material* GetMaterial(const std::string_view& Material);
			Material* GetMaterial(size_t Material);
			SparseIndex& GetStorage(uint64_t Section);
			Core::Pool<Component*>& GetComponents(uint64_t Section);
			Core::Pool<Component*>& GetActors(ActorType Type);
			Graphics::RenderTarget2D::Desc GetDescRT() const;
			Graphics::MultiRenderTarget2D::Desc GetDescMRT() const;
			Graphics::Format GetFormatMRT(unsigned int Target) const;
			Core::Vector<Entity*> CloneEntityAsArray(Entity* Value);
			Core::Vector<Entity*> QueryByParent(Entity* Parent) const;
			Core::Vector<Entity*> QueryByName(const std::string_view& Name) const;
			Core::Vector<Component*> QueryByPosition(uint64_t Section, const Compute::Vector3& Position, float Radius);
			Core::Vector<Component*> QueryByArea(uint64_t Section, const Compute::Vector3& Min, const Compute::Vector3& Max);
			Core::Vector<Component*> QueryByMatch(uint64_t Section, std::function<bool(const Compute::Bounding&)>&& MatchCallback);
			Core::Vector<std::pair<Component*, Compute::Vector3>> QueryByRay(uint64_t Section, const Compute::Ray& Origin);
			Core::Vector<CubicDepthMap*>& GetPointsMapping();
			Core::Vector<LinearDepthMap*>& GetSpotsMapping();
			Core::Vector<CascadedDepthMap*>& GetLinesMapping();
			Core::Vector<VoxelMapping>& GetVoxelsMapping();
			const Core::UnorderedMap<uint64_t, SparseIndex*>& GetRegistry() const;
			Core::String AsResourcePath(const std::string_view& Path);
			Entity* AddEntity();
			Entity* CloneEntity(Entity* Value);
			bool AddEntity(Core::Unique<Entity> Entity);
			bool IsActive() const;
			bool IsLeftHanded() const;
			bool IsIndexed() const;
			bool IsBusy(TaskType Type);
			size_t GetMaterialsCount() const;
			size_t GetEntitiesCount() const;
			size_t GetComponentsCount(uint64_t Section);
			bool HasEntity(Entity* Entity) const;
			bool HasEntity(size_t Entity) const;
			Graphics::MultiRenderTarget2D* GetMRT(TargetType Type) const;
			Graphics::RenderTarget2D* GetRT(TargetType Type) const;
			Graphics::Texture2D** GetMerger();
			Graphics::ElementBuffer* GetStructure() const;
			Graphics::GraphicsDevice* GetDevice() const;
			Compute::Simulator* GetSimulator() const;
			Graphics::Activity* GetActivity() const;
			RenderConstants* GetConstants() const;
			ShaderCache* GetShaders() const;
			PrimitiveCache* GetPrimitives() const;
			Desc& GetConf();

		private:
			void StepSimulate(Core::Timer* Time);
			void StepSynchronize(Core::Timer* Time);
			void StepAnimate(Core::Timer* Time);
			void StepGameplay(Core::Timer* Time);
			void StepTransactions();
			void StepEvents();
			void StepIndexing();
			void StepFinalize();

		protected:
			void LoadComponent(Component* Base);
			void UnloadComponentAll(Component* Base);
			bool UnloadComponent(Component* Base);
			void RegisterComponent(Component* Base, bool Verify);
			void UnregisterComponent(Component* Base);
			void CloneEntities(Entity* Instance, Core::Vector<Entity*>* Array);
			void GenerateMaterialBuffer();
			void GenerateVoxelBuffers();
			void GenerateDepthBuffers();
			void NotifyCosmos(Component* Base);
			void ClearCosmos(Component* Base);
			void UpdateCosmos(SparseIndex& Storage, Component* Base);
			void FillMaterialBuffers();
			void ResizeRenderBuffers();
			void RegisterEntity(Entity* In);
			bool UnregisterEntity(Entity* In);
			bool ResolveEvent(Event& Data);
			void WatchMovement(Entity* Base);
			void UnwatchMovement(Entity* Base);
			Entity* CloneEntityInstance(Entity* Entity);

		public:
			template <typename T, typename MatchFunction>
			Core::Vector<Component*> QueryByMatch(MatchFunction&& MatchCallback)
			{
				Core::Vector<Component*> Result;
				Compute::Cosmos::Iterator Context;
				auto& Storage = GetStorage(T::GetTypeId());
				auto Enqueue = [&Result](Component* Item) { Result.push_back(Item); };
				Storage.Index.template QueryIndex<Component, MatchFunction, decltype(Enqueue)>(Context, std::move(MatchCallback), std::move(Enqueue));

				return Result;
			}
			template <typename T>
			Core::Vector<Component*> QueryByPosition(const Compute::Vector3& Position, float Radius)
			{
				return QueryByPosition(T::GetTypeId(), Position, Radius);
			}
			template <typename T>
			Core::Vector<Component*> QueryByArea(const Compute::Vector3& Min, const Compute::Vector3& Max)
			{
				return QueryByArea(T::GetTypeId(), Min, Max);
			}
			template <typename T>
			Core::Vector<std::pair<Component*, Compute::Vector3>> QueryByRay(const Compute::Ray& Origin)
			{
				return QueryByRay(T::GetTypeId(), Origin);
			}
			template <typename T>
			void RayTest(const Compute::Ray& Origin, RayCallback&& Callback)
			{
				RayTest(T::GetTypeId(), Origin, std::move(Callback));
			}
			template <typename T>
			void LoadResource(Component* Context, const std::string_view& Path, std::function<void(ExpectsContent<T*>&&)>&& Callback)
			{
				LoadResource<T>(Context, Path, Core::VariantArgs(), std::move(Callback));
			}
			template <typename T>
			void LoadResource(Component* Context, const std::string_view& Path, const Core::VariantArgs& Keys, std::function<void(ExpectsContent<T*>&&)>&& Callback)
			{
				VI_ASSERT(Callback != nullptr, "callback should be set");
				LoadResource((uint64_t)typeid(T).hash_code(), Context, Path, Keys, [Callback = std::move(Callback)](ExpectsContent<void*> Object)
				{
					if (Object)
						Callback((T*)*Object);
					else
						Callback(Object.Error());
				});
			}
			template <typename T>
			Core::String FindResourceId(T* Resource)
			{
				return FindResourceId(typeid(T).hash_code(), (void*)Resource);
			}
			template <typename T>
			SparseIndex& GetStorage()
			{
				return GetStorage(T::GetTypeId());
			}
			template <typename T>
			Core::Pool<Component*>& GetComponents()
			{
				return GetComponents(T::GetTypeId());
			}
		};

		class VI_OUT Application : public Core::Singleton<Application>
		{
		public:
			struct Desc
			{
				struct FramesInfo
				{
					float Stable = 120.0f;
					float Limit = 0.0f;
				} Framerate;

				Graphics::GraphicsDevice::Desc GraphicsDevice;
				Graphics::Activity::Desc Activity;
				Core::Schedule::Desc Scheduler;
				Core::String Preferences;
				Core::String Environment;
				Core::String Directory;
				size_t PollingTimeout = 100;
				size_t PollingEvents = 256;
				size_t Threads = 0;
				size_t Usage =
					(size_t)ApplicationSet::GraphicsSet |
					(size_t)ApplicationSet::ActivitySet |
					(size_t)ApplicationSet::AudioSet |
					(size_t)ApplicationSet::ScriptSet |
					(size_t)ApplicationSet::ContentSet |
					(size_t)ApplicationSet::NetworkSet;
				bool Daemon = false;
				bool Cursor = true;
			};

		public:
			struct CacheInfo
			{
				ShaderCache* Shaders = nullptr;
				PrimitiveCache* Primitives = nullptr;
			} Cache;

		private:
			Core::Timer* InternalClock = nullptr;
			GUI::Context* InternalUI = nullptr;
			ApplicationState State = ApplicationState::Terminated;
			int ExitCode = 0;

		public:
			Audio::AudioDevice* Audio = nullptr;
			Graphics::GraphicsDevice* Renderer = nullptr;
			Graphics::Activity* Activity = nullptr;
			Scripting::VirtualMachine* VM = nullptr;
			RenderConstants* Constants = nullptr;
			ContentManager* Content = nullptr;
			AppData* Database = nullptr;
			SceneGraph* Scene = nullptr;
			Desc Control;

		public:
			Application(Desc* I) noexcept;
			virtual ~Application() noexcept;
			virtual void KeyEvent(Graphics::KeyCode Key, Graphics::KeyMod Mod, int Virtual, int Repeat, bool Pressed);
			virtual void InputEvent(char* Buffer, size_t Length);
			virtual void WheelEvent(int X, int Y, bool Normal);
			virtual void WindowEvent(Graphics::WindowState NewState, int X, int Y);
			virtual void Dispatch(Core::Timer* Time);
			virtual void Publish(Core::Timer* Time);
			virtual void Composition();
			virtual void ScriptHook();
			virtual void Initialize();
			virtual Core::Promise<void> Startup();
			virtual Core::Promise<void> Shutdown();
			GUI::Context* TryGetUI() const;
			GUI::Context* FetchUI();
			ApplicationState GetState() const;
			int Start();
			void Restart();
			void Stop(int ExitCode = 0);

		private:
			void LoopTrigger();

		private:
			static bool Status(Application* App);
			static void Compose();

		public:
			template <typename T, typename ...A>
			static int StartApp(A... Args)
			{
				Core::UPtr<T> App = new T(Args...);
				int ExitCode = App->Start();
				VI_ASSERT(ExitCode != EXIT_RESTART, "application cannot be restarted");
				return ExitCode;
			}
			template <typename T, typename ...A>
			static int StartAppWithRestart(A... Args)
			{
			RestartApp:
				Core::UPtr<T> App = new T(Args...);
				int ExitCode = App->Start();
				if (ExitCode == EXIT_RESTART)
					goto RestartApp;

				return ExitCode;
			}
		};

		template <typename Geometry, typename Instance>
		struct BatchingGroup
		{
			Core::Vector<Instance> Instances;
			Graphics::ElementBuffer* DataBuffer = nullptr;
			Geometry* GeometryBuffer = nullptr;
			Material* MaterialData = nullptr;
		};

		template <typename Geometry, typename Instance>
		struct BatchDispatchable
		{
			size_t Name;
			Geometry* Data;
			Material* Surface;
			Instance Params;

			BatchDispatchable(size_t NewName, Geometry* NewData, Material* NewSurface, const Instance& NewParams) noexcept : Name(NewName), Data(NewData), Surface(NewSurface), Params(NewParams)
			{
			}
		};

		template <typename Geometry, typename Instance>
		class BatchingProxy
		{
		public:
			typedef BatchingGroup<Geometry, Instance> BatchGroup;
			typedef BatchDispatchable<Geometry, Instance> Dispatchable;

		public:
			Core::SingleQueue<BatchGroup*>* Cache = nullptr;
			Core::Vector<Core::Vector<Dispatchable>>* Queue = nullptr;
			Core::Vector<Dispatchable>* Instances = nullptr;
			Core::Vector<BatchGroup*> Groups;

		public:
			void Clear()
			{
				for (auto* Group : Groups)
				{
					Group->Instances.clear();
					Cache->push(Group);
				}

				Queue->clear();
				Groups.clear();
			}
			void Prepare(size_t MaxSize)
			{
				if (MaxSize > 0)
					Queue->resize(MaxSize);
				
				for (auto* Group : Groups)
				{
					Group->Instances.clear();
					Cache->push(Group);
				}
				Groups.clear();
			}
			void Emplace(Geometry* Data, Material* Surface, const Instance& Params, size_t Chunk)
			{
				VI_ASSERT(Chunk < Queue->size(), "chunk index is out of range");
				(*Queue)[Chunk].emplace_back(GetKeyId(Data, Surface), Data, Surface, Params);
			}
			size_t Compile(Graphics::GraphicsDevice* Device)
			{
				VI_ASSERT(Device != nullptr, "device should be set");
				PopulateInstances();
				PopulateGroups();
				CompileGroups(Device);
				return Groups.size();
			}

		private:
			void PopulateInstances()
			{
				size_t Total = 0;
				for (auto& Context : *Queue)
					Total += Context.size();
				
				Instances->reserve(Total);
				for (auto& Context : *Queue)
				{
					std::move(Context.begin(), Context.end(), std::back_inserter(*Instances));
					Context.clear();
				}

				VI_SORT(Instances->begin(), Instances->end(), [](Dispatchable& A, Dispatchable& B)
				{
					return A.Name < B.Name;
				});
			}
			void PopulateGroups()
			{
				size_t Name = 0;
				BatchGroup* Next = nullptr;
				for (auto& Item : *Instances)
				{
					if (Next != nullptr && Name == Item.Name)
					{
						Next->Instances.emplace_back(std::move(Item.Params));
						continue;
					}

					Name = Item.Name;
					Next = FetchGroup();
					Next->GeometryBuffer = Item.Data;
					Next->MaterialData = Item.Surface;
					Next->Instances.emplace_back(std::move(Item.Params));
					Groups.push_back(Next);
				}
				Instances->clear();
			}
			void CompileGroups(Graphics::GraphicsDevice* Device)
			{
				for (auto* Group : Groups)
				{
					if (Group->DataBuffer && Group->Instances.size() < (size_t)Group->DataBuffer->GetElements())
					{
						Device->UpdateBuffer(Group->DataBuffer, (void*)Group->Instances.data(), sizeof(Instance) * Group->Instances.size());
						continue;
					}

					Graphics::ElementBuffer::Desc Desc = Graphics::ElementBuffer::Desc();
					Desc.AccessFlags = Graphics::CPUAccess::Write;
					Desc.Usage = Graphics::ResourceUsage::Dynamic;
					Desc.BindFlags = Graphics::ResourceBind::Vertex_Buffer;
					Desc.ElementCount = (unsigned int)Group->Instances.size();
					Desc.Elements = (void*)Group->Instances.data();
					Desc.ElementWidth = sizeof(Instance);

					Core::Memory::Release(Group->DataBuffer);
					Group->DataBuffer = Device->CreateElementBuffer(Desc).Or(nullptr);
					if (!Group->DataBuffer)
						Group->Instances.clear();
				}
			}
			BatchGroup* FetchGroup()
			{
				if (Cache->empty())
					return Core::Memory::New<BatchGroup>();

				BatchGroup* Result = Cache->front();
				Cache->pop();
				return Result;
			}
			size_t GetKeyId(Geometry* Data, Material* Surface)
			{
				std::hash<void*> Hash;
				size_t Seed = Hash((void*)Data);
				Seed ^= Hash((void*)Surface) + 0x9e3779b9 + (Seed << 6) + (Seed >> 2);
				return Seed;
			}
		};

		template <typename T, typename Geometry = char, typename Instance = char, size_t Max = (size_t)MAX_STACK_DEPTH>
		class RendererProxy
		{
			static_assert(std::is_base_of<Component, T>::value, "parameter must be derived from a component");

		public:
			typedef BatchingProxy<Geometry, Instance> Batching;
			typedef BatchingGroup<Geometry, Instance> BatchGroup;
			typedef BatchDispatchable<Geometry, Instance> Dispatchable;
			typedef std::pair<T*, VisibilityQuery> QueryGroup;
			typedef Core::Vector<BatchGroup*> Groups;
			typedef Core::Vector<T*> Storage;

		public:
			static const size_t Depth = Max;

		private:
			struct
			{
				Core::Vector<Core::Vector<Dispatchable>> Queue;
				Core::Vector<Dispatchable> Instances;
				Core::SingleQueue<BatchGroup*> Groups;
			} Caching;

		private:
			Batching Batchers[Max][(size_t)GeoCategory::Count];
			Storage Data[Max][(size_t)GeoCategory::Count];
			Core::Vector<Core::Vector<QueryGroup>> Queries;
			size_t Offset;

		public:
			Storage Culling;

		public:
			RendererProxy() noexcept : Offset(0)
			{
				for (size_t i = 0; i < (size_t)GeoCategory::Count; ++i)
				{
					for (size_t j = 0; j < Depth; ++j)
					{
						auto& Next = Batchers[j][i];
						Next.Queue = &Caching.Queue;
						Next.Instances = &Caching.Instances;
						Next.Cache = &Caching.Groups;
					}
				}
			}
			~RendererProxy() noexcept
			{
				for (size_t i = 0; i < (size_t)GeoCategory::Count; ++i)
				{
					for (size_t j = 0; j < Depth; ++j)
						Batchers[j][i].Clear();
				}

				while (!Caching.Groups.empty())
				{
					auto* Next = Caching.Groups.front();
					Core::Memory::Release(Next->DataBuffer);
					Core::Memory::Delete(Next);
					Caching.Groups.pop();
				}
			}
			Batching& Batcher(GeoCategory Category = GeoCategory::Opaque)
			{
				return Batchers[Offset > 0 ? Offset - 1 : 0][(size_t)Category];
			}
			Groups& Batches(GeoCategory Category = GeoCategory::Opaque)
			{
				return Batchers[Offset > 0 ? Offset - 1 : 0][(size_t)Category].Groups;
			}
			Storage& Top(GeoCategory Category = GeoCategory::Opaque)
			{
				return Data[Offset > 0 ? Offset - 1 : 0][(size_t)Category];
			}
			bool Push(Core::Timer* Time, RenderSystem* Base, GeoCategory Category = GeoCategory::Opaque)
			{
				VI_ASSERT(Base != nullptr, "render system should be present");
				VI_ASSERT(Offset < Max - 1, "storage heap stack overflow");

				Storage* Frame = Data[Offset++];
				if (Base->State.IsSubpass())
				{
					Subcull(Base, Frame);
					return true;
				}
				
				bool AssumeSorted = (Time->GetFrameIndex() % Base->SortingFrequency != 0);
				Cullout(Base, Frame, AssumeSorted);
				return !AssumeSorted;
			}
			void Pop()
			{
				VI_ASSERT(Offset > 0, "storage heap stack underflow");
				Offset--;
			}
			bool HasBatching()
			{
				return !std::is_same<Geometry, char>::value && !std::is_same<Instance, char>::value;
			}

		private:
			void Prepare(size_t Threads)
			{
				Queries.resize(Threads);
				for (auto& Queue : Queries)
					Queue.clear();
			}

		private:
			template <typename PushFunction>
			void Dispatch(PushFunction&& Callback)
			{
				for (auto& Queue : Queries)
					std::for_each(Queue.begin(), Queue.end(), std::move(Callback));
			}
			template <class Q = T>
			typename std::enable_if<std::is_base_of<Drawable, Q>::value>::type Cullout(RenderSystem* System, Storage* Top, bool AssumeSorted)
			{
				VI_MEASURE(Core::Timings::Frame);
				if (AssumeSorted)
				{
					auto* Scene = System->GetScene();
					Scene->Statistics.Instances += Culling.size();
					for (size_t i = 0; i < (size_t)GeoCategory::Count; ++i)
						Scene->Statistics.Instances += Top[i].size();
					return;
				}

				for (size_t i = 0; i < (size_t)GeoCategory::Count; ++i)
					Top[i].clear();
				Culling.clear();

				System->QueryAsync<T>([this](size_t Threads)
				{
					Prepare(Threads);
				}, [this, &System](size_t Chunk, Component* Item)
				{
					VisibilityQuery Info;
					System->FetchVisibility(Item, Info);
					if (Info.BoundaryVisible || Info.QueryPixels)
						Queries[Chunk].emplace_back(std::make_pair((T*)Item, std::move(Info)));
				});

				auto* Scene = System->GetScene();
				Scene->Await(TaskType::Rendering);

				Dispatch([this, Top](QueryGroup& Group)
				{
					if (Group.second.BoundaryVisible)
						Top[(size_t)Group.second.Category].push_back(Group.first);

					if (Group.second.QueryPixels)
						Culling.push_back(Group.first);
				});

				++Scene->Statistics.Sorting;
				if (!HasBatching())
				{
					for (size_t i = 0; i < (size_t)GeoCategory::Count; ++i)
					{
						auto& Array = Top[i];
						Scene->Statistics.Instances += Array.size();
						Scene->Watch(TaskType::Rendering, Parallel::Enqueue([&Array]()
						{
							VI_SORT(Array.begin(), Array.end(), Entity::Sortout<T>);
						}));
					}

					Scene->Statistics.Instances += Culling.size();
					Scene->Watch(TaskType::Rendering, Parallel::Enqueue([this]()
					{
						VI_SORT(Culling.begin(), Culling.end(), Entity::Sortout<T>);
					}));
					Scene->Await(TaskType::Rendering);
				}
				else
				{
					Scene->Statistics.Instances += Culling.size();
					for (size_t i = 0; i < (size_t)GeoCategory::Count; ++i)
						Scene->Statistics.Instances += Top[i].size();
				}
			}
			template <class Q = T>
			typename std::enable_if<!std::is_base_of<Drawable, Q>::value>::type Cullout(RenderSystem* System, Storage* Top, bool AssumeSorted)
			{
				VI_MEASURE(Core::Timings::Frame);
				auto& Subframe = Top[(size_t)GeoCategory::Opaque];
				if (AssumeSorted)
				{
					auto* Scene = System->GetScene();
					Scene->Statistics.Instances += Subframe.size();
					return;
				}

				Subframe.clear();
				System->QueryAsync<T>([this](size_t Threads)
				{
					Prepare(Threads);
				}, [this, &System](size_t Chunk, Component* Item)
				{
					VisibilityQuery Info;
					System->FetchVisibility(Item, Info);
					if (Info.BoundaryVisible)
						Queries[Chunk].emplace_back(std::make_pair((T*)Item, std::move(Info)));
				});

				auto* Scene = System->GetScene();
				Scene->Await(TaskType::Rendering);

				Dispatch([&Subframe](QueryGroup& Group)
				{
					Subframe.push_back(Group.first);
				});

				++Scene->Statistics.Sorting;
				Scene->Statistics.Instances += Subframe.size();
				if (!HasBatching())
					VI_SORT(Subframe.begin(), Subframe.end(), Entity::Sortout<T>);
			}
			template <class Q = T>
			typename std::enable_if<std::is_base_of<Drawable, Q>::value>::type Subcull(RenderSystem* System, Storage* Top)
			{
				VI_MEASURE(Core::Timings::Frame);
				for (size_t i = 0; i < (size_t)GeoCategory::Count; ++i)
					Top[i].clear();

				System->QueryAsync<T>([this](size_t Threads)
				{
					Prepare(Threads);
				}, [this, &System](size_t Chunk, Component* Item)
				{
					VisibilityQuery Info;
					System->FetchVisibility(Item, Info);
					if (Info.BoundaryVisible)
						Queries[Chunk].emplace_back(std::make_pair((T*)Item, std::move(Info)));
				});

				auto* Scene = System->GetScene();
				Scene->Await(TaskType::Rendering);

				Dispatch([Top](QueryGroup& Group)
				{
					Top[(size_t)Group.second.Category].push_back(Group.first);
				});

				++Scene->Statistics.Sorting;
				if (!HasBatching())
				{
					for (size_t i = 0; i < (size_t)GeoCategory::Count; ++i)
					{
						auto& Array = Top[i];
						Scene->Statistics.Instances += Array.size();
						Scene->Watch(TaskType::Rendering, Parallel::Enqueue([&Array]()
						{
							VI_SORT(Array.begin(), Array.end(), Entity::Sortout<T>);
						}));
					}
					Scene->Await(TaskType::Rendering);
				}
				else
				{
					for (size_t i = 0; i < (size_t)GeoCategory::Count; ++i)
						Scene->Statistics.Instances += Top[i].size();
				}
			}
			template <class Q = T>
			typename std::enable_if<!std::is_base_of<Drawable, Q>::value>::type Subcull(RenderSystem* System, Storage* Top)
			{
				auto& Subframe = Top[(size_t)GeoCategory::Opaque];
				Subframe.clear();

				System->QueryAsync<T>([this](size_t Threads)
				{
					Prepare(Threads);
				}, [this, &System](size_t Chunk, Component* Item)
				{
					VisibilityQuery Info;
					System->FetchVisibility(Item, Info);
					if (Info.BoundaryVisible)
						Queries[Chunk].emplace_back(std::make_pair((T*)Item, std::move(Info)));
				});

				auto* Scene = System->GetScene();
				Scene->Await(TaskType::Rendering);

				Dispatch([&Subframe](QueryGroup& Group)
				{
					Subframe.push_back(Group.first);
				});

				++Scene->Statistics.Sorting;
				Scene->Statistics.Instances += Subframe.size();
				if (!HasBatching())
					VI_SORT(Subframe.begin(), Subframe.end(), Entity::Sortout<T>);
			}
		};

		template <typename T, typename Geometry = char, typename Instance = char>
		class VI_OUT GeometryRenderer : public Renderer
		{
			static_assert(std::is_base_of<Drawable, T>::value, "component must be drawable to work within geometry renderer");

		public:
			typedef BatchingGroup<Geometry, Instance> BatchGroup;
			typedef BatchingProxy<Geometry, Instance> Batching;
			typedef Core::Vector<BatchGroup*> Groups;
			typedef Core::Vector<T*> Objects;

		private:
			RendererProxy<T, Geometry, Instance> Proxy;
			std::function<void(T*, Instance&, Batching&)> Upsert;
			Core::UnorderedMap<T*, Graphics::Query*> Active;
			Core::SingleQueue<Graphics::Query*> Inactive;
			Graphics::DepthStencilState* DepthStencil;
			Graphics::BlendState* Blend;
			Graphics::Query* Current;
			size_t FrameTop[3];
			bool Skippable[2];

		public:
			GeometryRenderer(RenderSystem* Lab) noexcept : Renderer(Lab), Current(nullptr)
			{
				Graphics::GraphicsDevice* Device = System->GetDevice();
				DepthStencil = Device->GetDepthStencilState("dro_soo_lte");
				Blend = Device->GetBlendState("bo_woooo_one");
				FrameTop[0] = 0;
				Skippable[0] = false;
				FrameTop[1] = 0;
				Skippable[1] = false;
				FrameTop[2] = 0;
			}
			virtual ~GeometryRenderer() noexcept override
			{
				for (auto& Item : Active)
					Core::Memory::Release(Item.second);
				
				while (!Inactive.empty())
				{
					Core::Memory::Release(Inactive.front());
					Inactive.pop();
				}
			}
			virtual void BatchGeometry(T* Base, Batching& Batch, size_t Chunk)
			{
			}
			virtual size_t CullGeometry(const Viewer& View, const Objects& Chunk)
			{
				return 0;
			}
			virtual size_t RenderLinearization(Core::Timer* TimeStep, const Objects& Chunk)
			{
				return 0;
			}
			virtual size_t RenderLinearizationBatched(Core::Timer* TimeStep, const Groups& Chunk)
			{
				return 0;
			}
			virtual size_t RenderCubic(Core::Timer* TimeStep, const Objects& Chunk, Compute::Matrix4x4* ViewProjection)
			{
				return 0;
			}
			virtual size_t RenderCubicBatched(Core::Timer* TimeStep, const Groups& Chunk, Compute::Matrix4x4* ViewProjection)
			{
				return 0;
			}
			virtual size_t RenderVoxelization(Core::Timer* TimeStep, const Objects& Chunk)
			{
				return 0;
			}
			virtual size_t RenderVoxelizationBatched(Core::Timer* TimeStep, const Groups& Chunk)
			{
				return 0;
			}
			virtual size_t RenderGeometric(Core::Timer* TimeStep, const Objects& Chunk)
			{
				return 0;
			}
			virtual size_t RenderGeometricBatched(Core::Timer* TimeStep, const Groups& Chunk)
			{
				return 0;
			}
			virtual size_t RenderGeometricPrepass(Core::Timer* TimeStep, const Objects& Chunk)
			{
				return 0;
			}
			virtual size_t RenderGeometricPrepassBatched(Core::Timer* TimeStep, const Groups& Chunk)
			{
				return 0;
			}
			void ClearCulling() override
			{
				for (auto& Item : Active)
					Inactive.push(Item.second);
				Active.clear();
			}
			void BeginPass(Core::Timer* Time) override
			{
				bool Proceed = Proxy.Push(Time, System);
				if (!System->AllowInputLag)
					Proceed = true;

				if (!Proceed || !Proxy.HasBatching())
					return;

				Graphics::GraphicsDevice* Device = System->GetDevice();
				for (size_t i = 0; i < (size_t)GeoCategory::Count; ++i)
				{
					auto& Batcher = Proxy.Batcher((GeoCategory)i);
					auto& Frame = Proxy.Top((GeoCategory)i);
					Parallel::WailAll(Parallel::Distribute(Frame.begin(), Frame.end(), [&Batcher](size_t Threads)
					{
						Batcher.Prepare(Threads);
					}, [this, &Batcher](size_t Thread, T* Next)
					{
						BatchGeometry(Next, Batcher, Thread);
					}));
						
					auto* Scene = System->GetScene();
					Scene->Statistics.Batching += Batcher.Compile(Device);
				}
			}
			void EndPass() override
			{
				Proxy.Pop();
			}
			bool HasCategory(GeoCategory Category) override
			{
				return !Proxy.Top(Category).empty();
			}
			size_t RenderPrepass(Core::Timer* Time) override
			{
				size_t Count = 0;
				if (!System->State.Is(RenderState::Geometric))
					return Count;

				GeoCategory Category = GeoCategory::Opaque;
				if (System->State.IsSet(RenderOpt::Transparent))
					Category = GeoCategory::Transparent;
				else if (System->State.IsSet(RenderOpt::Additive))
					Category = GeoCategory::Additive;

				VI_MEASURE(Core::Timings::Frame);
				if (Proxy.HasBatching())
				{
					auto& Frame = Proxy.Batches(Category);
					if (!Frame.empty())
						Count += RenderGeometricPrepassBatched(Time, Frame);
				}
				else
				{
					auto& Frame = Proxy.Top(Category);
					if (!Frame.empty())
						Count += RenderGeometricPrepass(Time, Frame);
				}

				if (System->State.IsTop())
					Count += CullingPass();

				return Count;
			}
			size_t RenderPass(Core::Timer* Time) override
			{
				size_t Count = 0;
				if (System->State.Is(RenderState::Geometric))
				{
					GeoCategory Category = GeoCategory::Opaque;
					if (System->State.IsSet(RenderOpt::Transparent))
						Category = GeoCategory::Transparent;
					else if (System->State.IsSet(RenderOpt::Additive))
						Category = GeoCategory::Additive;

					VI_MEASURE(Core::Timings::Frame);
					if (Proxy.HasBatching())
					{
						auto& Frame = Proxy.Batches(Category);
						if (!Frame.empty())
						{
							System->ClearMaterials();
							Count += RenderGeometricBatched(Time, Frame);
						}
					}
					else
					{
						auto& Frame = Proxy.Top(Category);
						if (!Frame.empty())
						{
							System->ClearMaterials();
							Count += RenderGeometric(Time, Frame);
						}
					}
				}
				else if (System->State.Is(RenderState::Voxelization))
				{
					if (System->State.IsSet(RenderOpt::Transparent) || System->State.IsSet(RenderOpt::Additive))
						return 0;

					VI_MEASURE(Core::Timings::Mixed);
					if (Proxy.HasBatching())
					{
						auto& Frame = Proxy.Batches(GeoCategory::Opaque);
						if (!Frame.empty())
						{
							System->ClearMaterials();
							Count += RenderVoxelizationBatched(Time, Frame);
						}
					}
					else
					{
						auto& Frame = Proxy.Top(GeoCategory::Opaque);
						if (!Frame.empty())
						{
							System->ClearMaterials();
							Count += RenderVoxelization(Time, Frame);
						}
					}
				}
				else if (System->State.Is(RenderState::Linearization))
				{
					if (!System->State.IsSubpass())
						return 0;

					VI_MEASURE(Core::Timings::Pass);
					if (Proxy.HasBatching())
					{
						auto& Frame1 = Proxy.Batches(GeoCategory::Opaque);
						auto& Frame2 = Proxy.Batches(GeoCategory::Transparent);
						if (!Frame1.empty() || !Frame2.empty())
							System->ClearMaterials();

						if (!Frame1.empty())
							Count += RenderLinearizationBatched(Time, Frame1);

						if (!Frame2.empty())
							Count += RenderLinearizationBatched(Time, Frame2);
					}
					else
					{
						auto& Frame1 = Proxy.Top(GeoCategory::Opaque);
						auto& Frame2 = Proxy.Top(GeoCategory::Transparent);
						if (!Frame1.empty() || !Frame2.empty())
							System->ClearMaterials();

						if (!Frame1.empty())
							Count += RenderLinearization(Time, Frame1);

						if (!Frame2.empty())
							Count += RenderLinearization(Time, Frame2);
					}
				}
				else if (System->State.Is(RenderState::Cubic))
				{
					if (!System->State.IsSubpass())
						return 0;

					VI_MEASURE(Core::Timings::Pass);
					if (Proxy.HasBatching())
					{
						auto& Frame1 = Proxy.Batches(GeoCategory::Opaque);
						auto& Frame2 = Proxy.Batches(GeoCategory::Transparent);
						if (!Frame1.empty() || !Frame2.empty())
							System->ClearMaterials();

						if (!Frame1.empty())
							Count += RenderCubicBatched(Time, Frame1, System->View.CubicViewProjection);

						if (!Frame2.empty())
							Count += RenderCubicBatched(Time, Frame2, System->View.CubicViewProjection);
					}
					else
					{
						auto& Frame1 = Proxy.Top(GeoCategory::Opaque);
						auto& Frame2 = Proxy.Top(GeoCategory::Transparent);
						if (!Frame1.empty() || !Frame2.empty())
							System->ClearMaterials();

						if (!Frame1.empty())
							Count += RenderCubic(Time, Frame1, System->View.CubicViewProjection);

						if (!Frame2.empty())
							Count += RenderCubic(Time, Frame2, System->View.CubicViewProjection);
					}
				}

				return Count;
			}
			size_t CullingPass()
			{
				if (!System->OcclusionCulling)
					return 0;

				VI_MEASURE(Core::Timings::Pass);
				Graphics::GraphicsDevice* Device = System->GetDevice();
				size_t Count = 0; size_t Fragments = 0;

				for (auto It = Active.begin(); It != Active.end();)
				{
					auto* Query = It->second;
					if (Device->GetQueryData(Query, &Fragments))
					{
						It->first->Overlapping = (Fragments > 0 ? 1.0f : 0.0f);
						It = Active.erase(It);
						Inactive.push(Query);
					}
					else
						++It;
				}

				Skippable[0] = (FrameTop[0]++ < System->OccluderSkips);
				if (!Skippable[0])
					FrameTop[0] = 0;

				Skippable[1] = (FrameTop[1]++ < System->OccludeeSkips);
				if (!Skippable[1])
					FrameTop[1] = 0;

				if (FrameTop[2]++ >= System->OcclusionSkips && !Proxy.Culling.empty())
				{
					auto ViewProjection = System->View.ViewProjection;
					System->View.ViewProjection = Compute::Matrix4x4::CreateScale(System->OccludeeScaling) * ViewProjection;
					Device->SetDepthStencilState(DepthStencil);
					Device->SetBlendState(Blend);
					Count += CullGeometry(System->View, Proxy.Culling);
					System->View.ViewProjection = ViewProjection;
				}

				return Count;
			}
			bool CullingBegin(T* Base)
			{
				VI_ASSERT(Base != nullptr, "base should be set");
				if (Skippable[1] && Base->Overlapping < System->Threshold)
					return false;
				else if (Skippable[0] && Base->Overlapping >= System->Threshold)
					return false;

				if (Inactive.empty() && Active.size() >= System->MaxQueries)
				{
					Base->Overlapping = System->OverflowVisibility;
					return false;
				}

				if (Active.find(Base) != Active.end())
					return false;

				Graphics::GraphicsDevice* Device = System->GetDevice();
				if (Inactive.empty())
				{
					Graphics::Query::Desc I;
					I.Predicate = false;

					Current = Device->CreateQuery(I).Or(nullptr);	
					if (!Current)
						return false;
				}
				else
				{
					Current = Inactive.front();
					Inactive.pop();
				}

				Active[Base] = Current;
				Device->QueryBegin(Current);
				return true;
			}
			bool CullingEnd()
			{
				VI_ASSERT(Current != nullptr, "culling query must be started");
				if (!Current)
					return false;

				System->GetDevice()->QueryEnd(Current);
				Current = nullptr;

				return true;
			}

		public:
			VI_COMPONENT("geometry_renderer");
		};

		class VI_OUT EffectRenderer : public Renderer
		{
		protected:
			Core::UnorderedMap<Core::String, Graphics::Shader*> Effects;
			Graphics::DepthStencilState* DepthStencil;
			Graphics::RasterizerState* Rasterizer;
			Graphics::BlendState* Blend;
			Graphics::SamplerState* SamplerWrap;
			Graphics::SamplerState* SamplerClamp;
			Graphics::SamplerState* SamplerMirror;
			Graphics::InputLayout* Layout;
			Graphics::RenderTarget2D* Output;
			Graphics::RenderTarget2D* Swap;
			unsigned int MaxSlot;

		public:
			EffectRenderer(RenderSystem* Lab) noexcept;
			virtual ~EffectRenderer() noexcept override;
			virtual void ResizeEffect();
			virtual void RenderEffect(Core::Timer* Time) = 0;
			size_t RenderPass(Core::Timer* Time) override;
			void ResizeBuffers() override;
			unsigned int GetMipLevels() const;
			unsigned int GetWidth() const;
			unsigned int GetHeight() const;

		protected:
			void RenderCopyMain(uint32_t Slot, Graphics::Texture2D* Target);
			void RenderCopyLast(Graphics::Texture2D* Target);
			void RenderOutput(Graphics::RenderTarget2D* Resource = nullptr);
			void RenderTexture(uint32_t Slot6, Graphics::Texture2D* Resource = nullptr);
			void RenderTexture(uint32_t Slot6, Graphics::Texture3D* Resource = nullptr);
			void RenderTexture(uint32_t Slot6, Graphics::TextureCube* Resource = nullptr);
			void RenderMerge(Graphics::Shader* Effect, void* Buffer = nullptr, size_t Count = 1);
			void RenderResult(Graphics::Shader* Effect, void* Buffer = nullptr);
			void RenderResult();
			void SampleWrap();
			void SampleClamp();
			void SampleMirror();
			void GenerateMips();
			Graphics::Shader* GetEffect(const std::string_view& Name);
			Graphics::ExpectsGraphics<Graphics::Shader*> CompileEffect(Graphics::Shader::Desc& Desc, size_t BufferSize = 0);
			Graphics::ExpectsGraphics<Graphics::Shader*> CompileEffect(const std::string_view& SectionName, size_t BufferSize = 0);

		public:
			VI_COMPONENT("effect_renderer");
		};
	}
}
#endif
