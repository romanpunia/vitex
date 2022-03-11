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
		typedef Graphics::DepthTarget2D LinearDepthMap;
		typedef Graphics::DepthTargetCube CubicDepthMap;
		typedef std::vector<LinearDepthMap*> CascadedDepthMap;
		typedef std::function<void(Core::Timer*)> PacketCallback;
		typedef std::function<void(Core::Timer*, struct Viewer*)> RenderCallback;
		typedef std::function<void(class ContentManager*, bool)> SaveCallback;
		typedef std::function<void(const std::string&, Core::VariantArgs&)> MessageCallback;
		typedef std::function<bool(class Component*, const Compute::Vector3&)> RayCallback;
		typedef std::function<bool(Graphics::RenderTarget*)> TargetCallback;
		typedef std::function<void(class SceneGraph*, class Entity*)> CloneCallback;

		class NMake;

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

		enum
		{
			MAX_STACK_DEPTH = 4
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
			Singlethreaded,
			Multithreaded
		};

		enum class RenderOpt
		{
			None = 0,
			Transparent = 1,
			Static = 2,
			Inner = 4,
			Additive = 8
		};

		enum class RenderCulling
		{
			Spot,
			Point,
			Line
		};

		enum class RenderState
		{
			Geometry_Result,
			Geometry_Voxels,
			Depth_Linear,
			Depth_Cubic
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
			Message = 1 << 2,
			Cullable = 1 << 3,
			Drawable = 1 << 4
		};

		enum class ActorType
		{
			Update,
			Synchronize,
			Message,
			Count
		};

		enum class ComposerTag
		{
			Component,
			Renderer,
			Effect,
			Filter
		};

		inline ActorSet operator |(ActorSet A, ActorSet B)
		{
			return static_cast<ActorSet>(static_cast<uint64_t>(A) | static_cast<uint64_t>(B));
		}
		inline ApplicationSet operator |(ApplicationSet A, ApplicationSet B)
		{
			return static_cast<ApplicationSet>(static_cast<uint64_t>(A) | static_cast<uint64_t>(B));
		}
		inline RenderOpt operator |(RenderOpt A, RenderOpt B)
		{
			return static_cast<RenderOpt>(static_cast<uint64_t>(A) | static_cast<uint64_t>(B));
		}

		struct TH_OUT Event
		{
			std::string Name;
			Core::VariantArgs Args;

			Event(const std::string& NewName) noexcept;
			Event(const std::string& NewName, const Core::VariantArgs& NewArgs) noexcept;
			Event(const std::string& NewName, Core::VariantArgs&& NewArgs) noexcept;
			Event(const Event& Other) noexcept;
			Event(Event&& Other) noexcept;
			Event& operator= (const Event& Other) noexcept;
			Event& operator= (Event&& Other) noexcept;
		};

		struct TH_OUT AssetCache
		{
			std::string Path;
			void* Resource = nullptr;
		};

		struct TH_OUT AssetArchive
		{
			Core::Stream* Stream = nullptr;
			std::string Path;
			uint64_t Length = 0;
			uint64_t Offset = 0;
		};

		struct TH_OUT AssetFile : public Core::Object
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

		struct TH_OUT IdxSnapshot
		{
			std::unordered_map<Entity*, uint64_t> To;
			std::unordered_map<uint64_t, Entity*> From;
		};

		struct TH_OUT FragmentQuery
		{
		private:
			uint64_t Fragments;
			Graphics::Query* Query;
			int Satisfied;

		public:
			FragmentQuery();
			~FragmentQuery();
			bool Begin(Graphics::GraphicsDevice* Device);
			void End(Graphics::GraphicsDevice* Device);
			void Clear();
			int Fetch(RenderSystem* System);
			uint64_t GetPassed();
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

		struct TH_OUT Viewer
		{
			RenderSystem* Renderer = nullptr;
			RenderCulling Culling = RenderCulling::Spot;
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

			void Set(const Compute::Matrix4x4& View, const Compute::Matrix4x4& Projection, const Compute::Vector3& Position, float Near, float Far, RenderCulling Type);
			void Set(const Compute::Matrix4x4& View, const Compute::Matrix4x4& Projection, const Compute::Vector3& Position, const Compute::Vector3& Rotation, float Near, float Far, RenderCulling Type);
		};

		struct TH_OUT Attenuation
		{
			float Radius = 10.0f;
			float C1 = 0.6f;
			float C2 = 0.6f;
		};

		struct TH_OUT Subsurface
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

		class TH_OUT NMake
		{
		public:
			static void Pack(Core::Schema* V, bool Value);
			static void Pack(Core::Schema* V, int Value);
			static void Pack(Core::Schema* V, unsigned int Value);
			static void Pack(Core::Schema* V, unsigned long Value);
			static void Pack(Core::Schema* V, float Value);
			static void Pack(Core::Schema* V, double Value);
			static void Pack(Core::Schema* V, int64_t Value);
			static void Pack(Core::Schema* V, long double Value);
			static void Pack(Core::Schema* V, unsigned long long Value);
			static void Pack(Core::Schema* V, const char* Value);
			static void Pack(Core::Schema* V, const Compute::Vector2& Value);
			static void Pack(Core::Schema* V, const Compute::Vector3& Value);
			static void Pack(Core::Schema* V, const Compute::Vector4& Value);
			static void Pack(Core::Schema* V, const Compute::Matrix4x4& Value);
			static void Pack(Core::Schema* V, const Attenuation& Value);
			static void Pack(Core::Schema* V, const AnimatorState& Value);
			static void Pack(Core::Schema* V, const SpawnerProperties& Value);
			static void Pack(Core::Schema* V, Material* Value, ContentManager* Content);
			static void Pack(Core::Schema* V, const Compute::SkinAnimatorKey& Value);
			static void Pack(Core::Schema* V, const Compute::SkinAnimatorClip& Value);
			static void Pack(Core::Schema* V, const Compute::KeyAnimatorClip& Value);
			static void Pack(Core::Schema* V, const Compute::AnimatorKey& Value);
			static void Pack(Core::Schema* V, const Compute::ElementVertex& Value);
			static void Pack(Core::Schema* V, const Compute::Joint& Value);
			static void Pack(Core::Schema* V, const Compute::Vertex& Value);
			static void Pack(Core::Schema* V, const Compute::SkinVertex& Value);
			static void Pack(Core::Schema* V, const Core::Ticker& Value);
			static void Pack(Core::Schema* V, const std::string& Value);
			static void Pack(Core::Schema* V, const std::vector<bool>& Value);
			static void Pack(Core::Schema* V, const std::vector<int>& Value);
			static void Pack(Core::Schema* V, const std::vector<unsigned int>& Value);
			static void Pack(Core::Schema* V, const std::vector<float>& Value);
			static void Pack(Core::Schema* V, const std::vector<double>& Value);
			static void Pack(Core::Schema* V, const std::vector<int64_t>& Value);
			static void Pack(Core::Schema* V, const std::vector<long double>& Value);
			static void Pack(Core::Schema* V, const std::vector<uint64_t>& Value);
			static void Pack(Core::Schema* V, const std::vector<Compute::Vector2>& Value);
			static void Pack(Core::Schema* V, const std::vector<Compute::Vector3>& Value);
			static void Pack(Core::Schema* V, const std::vector<Compute::Vector4>& Value);
			static void Pack(Core::Schema* V, const std::vector<Compute::Matrix4x4>& Value);
			static void Pack(Core::Schema* V, const std::vector<AnimatorState>& Value);
			static void Pack(Core::Schema* V, const std::vector<SpawnerProperties>& Value);
			static void Pack(Core::Schema* V, const std::vector<Compute::SkinAnimatorClip>& Value);
			static void Pack(Core::Schema* V, const std::vector<Compute::KeyAnimatorClip>& Value);
			static void Pack(Core::Schema* V, const std::vector<Compute::AnimatorKey>& Value);
			static void Pack(Core::Schema* V, const std::vector<Compute::ElementVertex>& Value);
			static void Pack(Core::Schema* V, const std::vector<Compute::Joint>& Value);
			static void Pack(Core::Schema* V, const std::vector<Compute::Vertex>& Value);
			static void Pack(Core::Schema* V, const std::vector<Compute::SkinVertex>& Value);
			static void Pack(Core::Schema* V, const std::vector<Core::Ticker>& Value);
			static void Pack(Core::Schema* V, const std::vector<std::string>& Value);
			static bool Unpack(Core::Schema* V, bool* O);
			static bool Unpack(Core::Schema* V, int* O);
			static bool Unpack(Core::Schema* V, unsigned int* O);
			static bool Unpack(Core::Schema* V, unsigned long* O);
			static bool Unpack(Core::Schema* V, float* O);
			static bool Unpack(Core::Schema* V, double* O);
			static bool Unpack(Core::Schema* V, int64_t* O);
			static bool Unpack(Core::Schema* V, long double* O);
			static bool Unpack(Core::Schema* V, unsigned long long* O);
			static bool Unpack(Core::Schema* V, Compute::Vector2* O);
			static bool Unpack(Core::Schema* V, Compute::Vector3* O);
			static bool Unpack(Core::Schema* V, Compute::Vector4* O);
			static bool Unpack(Core::Schema* V, Compute::Matrix4x4* O);
			static bool Unpack(Core::Schema* V, Attenuation* O);
			static bool Unpack(Core::Schema* V, AnimatorState* O);
			static bool Unpack(Core::Schema* V, SpawnerProperties* O);
			static bool Unpack(Core::Schema* V, Material* O, ContentManager* Content);
			static bool Unpack(Core::Schema* V, Compute::SkinAnimatorKey* O);
			static bool Unpack(Core::Schema* V, Compute::SkinAnimatorClip* O);
			static bool Unpack(Core::Schema* V, Compute::KeyAnimatorClip* O);
			static bool Unpack(Core::Schema* V, Compute::AnimatorKey* O);
			static bool Unpack(Core::Schema* V, Compute::ElementVertex* O);
			static bool Unpack(Core::Schema* V, Compute::Joint* O);
			static bool Unpack(Core::Schema* V, Compute::Vertex* O);
			static bool Unpack(Core::Schema* V, Compute::SkinVertex* O);
			static bool Unpack(Core::Schema* V, Core::Ticker* O);
			static bool Unpack(Core::Schema* V, std::string* O);
			static bool Unpack(Core::Schema* V, std::vector<bool>* O);
			static bool Unpack(Core::Schema* V, std::vector<int>* O);
			static bool Unpack(Core::Schema* V, std::vector<unsigned int>* O);
			static bool Unpack(Core::Schema* V, std::vector<float>* O);
			static bool Unpack(Core::Schema* V, std::vector<double>* O);
			static bool Unpack(Core::Schema* V, std::vector<int64_t>* O);
			static bool Unpack(Core::Schema* V, std::vector<long double>* O);
			static bool Unpack(Core::Schema* V, std::vector<uint64_t>* O);
			static bool Unpack(Core::Schema* V, std::vector<Compute::Vector2>* O);
			static bool Unpack(Core::Schema* V, std::vector<Compute::Vector3>* O);
			static bool Unpack(Core::Schema* V, std::vector<Compute::Vector4>* O);
			static bool Unpack(Core::Schema* V, std::vector<Compute::Matrix4x4>* O);
			static bool Unpack(Core::Schema* V, std::vector<AnimatorState>* O);
			static bool Unpack(Core::Schema* V, std::vector<SpawnerProperties>* O);
			static bool Unpack(Core::Schema* V, std::vector<Compute::SkinAnimatorClip>* O);
			static bool Unpack(Core::Schema* V, std::vector<Compute::KeyAnimatorClip>* O);
			static bool Unpack(Core::Schema* V, std::vector<Compute::AnimatorKey>* O);
			static bool Unpack(Core::Schema* V, std::vector<Compute::ElementVertex>* O);
			static bool Unpack(Core::Schema* V, std::vector<Compute::Joint>* O);
			static bool Unpack(Core::Schema* V, std::vector<Compute::Vertex>* O);
			static bool Unpack(Core::Schema* V, std::vector<Compute::SkinVertex>* O);
			static bool Unpack(Core::Schema* V, std::vector<Core::Ticker>* O);
			static bool Unpack(Core::Schema* V, std::vector<std::string>* O);
		};

		class TH_OUT Material : public Core::Object
		{
			friend NMake;
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
			std::string Name;
			SceneGraph* Scene;

		public:
			Subsurface Surface;
			uint64_t Slot;

		public:
			Material(SceneGraph* Src);
			Material(const Material& Other);
			virtual ~Material() override;
			void SetName(const std::string& Value, bool Internal = false);
			const std::string& GetName() const;
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

		class TH_OUT Processor : public Core::Object
		{
			friend ContentManager;

		protected:
			ContentManager* Content;

		public:
			Processor(ContentManager* NewContent);
			virtual ~Processor() override;
			virtual void Free(AssetCache* Asset);
			virtual void* Duplicate(AssetCache* Asset, const Core::VariantArgs& Keys);
			virtual void* Deserialize(Core::Stream* Stream, uint64_t Length, uint64_t Offset, const Core::VariantArgs& Keys);
			virtual bool Serialize(Core::Stream* Stream, void* Instance, const Core::VariantArgs& Keys);
			ContentManager* GetContent();
		};

		class TH_OUT Component : public Core::Object
		{
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
			Component(Entity* Ref, ActorSet Rule);
			virtual ~Component() override;
			virtual void Serialize(ContentManager* Content, Core::Schema* Node);
			virtual void Deserialize(ContentManager* Content, Core::Schema* Node);
			virtual void Activate(Component* New);
			virtual void Deactivate();
			virtual void Synchronize(Core::Timer* Time);
			virtual void Update(Core::Timer* Time);
			virtual void Message(const std::string& Name, Core::VariantArgs& Args);
			virtual size_t GetUnitBounds(Compute::Vector3& Min, Compute::Vector3& Max);
			virtual float GetVisibility(const Viewer& View, float Distance);
			virtual Component* Copy(Entity* New) = 0;
			Entity* GetEntity();
			void SetActive(bool Enabled);
			bool IsDrawable();
			bool IsCullable();
			bool IsActive();

		public:
			TH_COMPONENT_ROOT("component");
		};

		class TH_OUT Entity : public Core::Object
		{
			friend SceneGraph;
			friend RenderSystem;

		protected:
			std::unordered_map<uint64_t, Component*> Components;
			SceneGraph* Scene;

		private:
			Compute::Transform* Transform;
			Compute::Matrix4x4 Box;
			Compute::Vector3 Min, Max;
			std::string Name;
			float Distance;
			float Visibility;
			bool Active;
			bool Dirty;

		public:
			uint64_t Tag;

		public:
			Entity(SceneGraph* Ref);
			virtual ~Entity() override;
			void SetName(const std::string& Value, bool Internal = false);
			void SetRoot(Entity* Parent);
			void UpdateBounds();
			void RemoveComponent(uint64_t Id);
			void RemoveChilds();
			Component* AddComponent(Component* In);
			Component* GetComponent(uint64_t Id);
			uint64_t GetComponentsCount() const;
			SceneGraph* GetScene() const;
			Entity* GetParent() const;
			Entity* GetChild(size_t Index) const;
			Compute::Transform* GetTransform() const;
			const Compute::Matrix4x4& GetBox() const;
			const Compute::Vector3& GetMin() const;
			const Compute::Vector3& GetMax() const;
			const std::string& GetName() const;
			std::string GetSystemName() const;
			size_t GetChildsCount() const;
			float GetVisibility(const Viewer& Base) const;
			bool IsDirty(bool Reset = false);
			bool IsActive() const;
			Compute::Vector3 GetRadius3() const;
			float GetRadius() const;

		public:
			std::unordered_map<uint64_t, Component*>::iterator begin()
			{
				return Components.begin();
			}
			std::unordered_map<uint64_t, Component*>::iterator end()
			{
				return Components.end();
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
				Component* Component = new In(this);
				Component->Active = true;
				return (In*)AddComponent(Component);
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
				return A->Parent->Distance - B->Parent->Distance < 0;
			}
		};

		class TH_OUT Drawable : public Component
		{
			friend SceneGraph;

		protected:
			std::unordered_map<void*, Material*> Materials;

		private:
			GeoCategory Category;
			uint64_t Source;
			bool Complex;

		public:
			FragmentQuery Query;
			bool Static;

		public:
			Drawable(Entity* Ref, ActorSet Rule, uint64_t Hash, bool Complex);
			virtual ~Drawable();
			virtual void Message(const std::string& Name, Core::VariantArgs& Args) override;
			virtual Component* Copy(Entity* New) override = 0;
			bool SetCategory(GeoCategory NewCategory);
			bool SetMaterial(void* Instance, Material* Value);
			bool WasVisible(bool Rebake);
			float GetOverlapping(RenderSystem* System);
			GeoCategory GetCategory();
			int64_t GetSlot(void* Surface);
			int64_t GetSlot();
			Material* GetMaterial(void* Surface);
			Material* GetMaterial();
			const std::unordered_map<void*, Material*>& GetMaterials();

		public:
			TH_COMPONENT("drawable");
		};

		class TH_OUT Renderer : public Core::Object
		{
			friend SceneGraph;

		protected:
			RenderSystem* System;

		public:
			bool Active;

		public:
			Renderer(RenderSystem* Lab);
			virtual ~Renderer() override;
			virtual void Serialize(ContentManager* Content, Core::Schema* Node);
			virtual void Deserialize(ContentManager* Content, Core::Schema* Node);
			virtual void ResizeBuffers();
			virtual void Activate();
			virtual void Deactivate();
			virtual void BeginPass();
			virtual void EndPass();
			virtual bool HasCategory(GeoCategory Category);
			virtual size_t CullingPass(const Viewer& View, bool Rebake);
			virtual size_t RenderPass(Core::Timer* Time, RenderState State, RenderOpt Options);
			void SetRenderer(RenderSystem* NewSystem);
			RenderSystem* GetRenderer();

		public:
			TH_COMPONENT_ROOT("renderer");
		};

		class TH_OUT RenderSystem : public Core::Object
		{
		private:
			struct
			{
				Core::Pool<Component*>* Data = nullptr;
				Compute::Cosmos* Index = nullptr;
				Compute::Area Box = Compute::Area(3);
				size_t Offset = 0;
			} Query;

		protected:
			std::vector<Renderer*> Renderers;
			Graphics::DepthStencilState* DepthStencil;
			Graphics::BlendState* Blend;
			Graphics::SamplerState* Sampler;
			Graphics::DepthTarget2D* Target;
			Graphics::GraphicsDevice* Device;
			Material* BaseMaterial;
			SceneGraph* Scene;
			size_t DepthSize;
			size_t StackTop;
			bool Satisfied;

		public:
			Viewer View;
			Core::Ticker Occlusion;
			size_t StallFrames;
			float Threshold;
			bool OcclusionCulling;
			bool FrustumCulling;
			bool PreciseCulling;

		public:
			RenderSystem(SceneGraph* NewScene);
			virtual ~RenderSystem() override;
			void SetDepthSize(size_t Size);
			void SetView(const Compute::Matrix4x4& View, const Compute::Matrix4x4& Projection, const Compute::Vector3& Position, float Near, float Far, RenderCulling Type);
			void RestoreViewBuffer(Viewer* View);
			void Remount(Renderer* Target);
			void Remount();
			void Mount();
			void Unmount();
			void MoveRenderer(uint64_t Id, int64_t Offset);
			void RemoveRenderer(uint64_t Id);
			void RestoreOutput();
			void FreeShader(const std::string& Name, Graphics::Shader* Shader);
			void FreeShader(Graphics::Shader* Shader);
			void FreeBuffers(const std::string& Name, Graphics::ElementBuffer** Buffers);
			void FreeBuffers(Graphics::ElementBuffer** Buffers);
			void ClearMaterials();
			size_t Render(Core::Timer* Time, RenderState Stage, RenderOpt Options);
			void QueryBegin(uint64_t Section);
			void QueryEnd();
			void* QueryNext();
			float EnqueueCullable(Component* Base);
			float EnqueueDrawable(Drawable* Base);
			bool PushCullable(Entity* Target, const Viewer& Source, Component* Base);
			bool PushDrawable(Entity* Target, const Viewer& Source, Drawable* Base);
			bool PushGeometryBuffer(Material* Next);
			bool PushVoxelsBuffer(Material* Next);
			bool PushDepthLinearBuffer(Material* Next);
			bool PushDepthCubicBuffer(Material* Next);
			bool HasCategory(GeoCategory Category);
			bool IsTopLevel();
			Graphics::Shader* CompileShader(Graphics::Shader::Desc& Desc, size_t BufferSize = 0);
			Graphics::Shader* CompileShader(const std::string& SectionName, size_t BufferSize = 0);
			bool CompileBuffers(Graphics::ElementBuffer** Result, const std::string& Name, size_t ElementSize, size_t ElementsCount);
			Renderer* AddRenderer(Renderer* In);
			Renderer* GetRenderer(uint64_t Id);
			size_t GetDepthSize();
			int64_t GetOffset(uint64_t Id);
			std::vector<Renderer*>& GetRenderers();
			Graphics::MultiRenderTarget2D* GetMRT(TargetType Type);
			Graphics::RenderTarget2D* GetRT(TargetType Type);
			Graphics::Texture2D** GetMerger();
			Graphics::GraphicsDevice* GetDevice();
			Graphics::DepthTarget2D* GetDepthTarget();
			PrimitiveCache* GetPrimitives();
			SceneGraph* GetScene();

		private:
			size_t RenderOverlapping(Core::Timer* Time, bool Rebake);

		public:
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
				return GetOffset(In::GetTypeId());
			}
		};

		class TH_OUT ShaderCache : public Core::Object
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
			std::string Find(Graphics::Shader* Shader);
			bool Has(const std::string& Name);
			bool Free(const std::string& Name, Graphics::Shader* Shader = nullptr);
			void ClearCache();
		};

		class TH_OUT PrimitiveCache : public Core::Object
		{
		private:
			struct SCache
			{
				Graphics::ElementBuffer* Buffers[2];
				uint64_t Count;
			};

		private:
			std::unordered_map<std::string, SCache> Cache;
			Graphics::GraphicsDevice* Device;
			Graphics::ElementBuffer* Sphere[2];
			Graphics::ElementBuffer* Cube[2];
			Graphics::ElementBuffer* Box[2];
			Graphics::ElementBuffer* SkinBox[2];
			Graphics::ElementBuffer* Quad;
			std::mutex Safe;

		public:
			PrimitiveCache(Graphics::GraphicsDevice* Device);
			virtual ~PrimitiveCache() override;
			bool Compile(Graphics::ElementBuffer** Result, const std::string& Name, size_t ElementSize, size_t ElementsCount);
			bool Get(Graphics::ElementBuffer** Result, const std::string& Name);
			bool Has(const std::string& Name);
			bool Free(const std::string& Name, Graphics::ElementBuffer** Buffers);
			std::string Find(Graphics::ElementBuffer** Buffer);
			Graphics::ElementBuffer* GetQuad();
			Graphics::ElementBuffer* GetSphere(BufferType Type);
			Graphics::ElementBuffer* GetCube(BufferType Type);
			Graphics::ElementBuffer* GetBox(BufferType Type);
			Graphics::ElementBuffer* GetSkinBox(BufferType Type);
			void GetSphereBuffers(Graphics::ElementBuffer** Result);
			void GetCubeBuffers(Graphics::ElementBuffer** Result);
			void GetBoxBuffers(Graphics::ElementBuffer** Result);
			void GetSkinBoxBuffers(Graphics::ElementBuffer** Result);
			void ClearCache();
		};

		class TH_OUT SceneGraph : public Core::Object
		{
			friend RenderSystem;
			friend Renderer;
			friend Component;
			friend Entity;
			friend Drawable;

		public:
			struct TH_OUT Desc
			{
				Compute::Simulator::Desc Simulator;
				Graphics::GraphicsDevice* Device = nullptr;
				Script::VMManager* Manager = nullptr;
				PrimitiveCache* Primitives = nullptr;
				ShaderCache* Shaders = nullptr;
				uint64_t StartMaterials = 1ll << 8;
				uint64_t StartEntities = 1ll << 12;
				uint64_t StartComponents = 1ll << 13;
				uint64_t GrowMargin = 128;
				uint64_t MaxUpdates = 256;
				double MaxFrames = 60.0;
				double MinFrames = 10.0;
				double FrequencyHZ = 900.0;
				double GrowRate = 0.25f;
				float RenderQuality = 1.0f;
				bool EnableHDR = false;
				bool Async = true;
				bool Mutations = true;

				static Desc Get(Application* Base);
			};

		public:
			struct Table
			{
				Core::Pool<Component*> Data;
				Compute::Cosmos Index;
			};

		private:
			struct Packet
			{
				std::atomic<bool> Active;
				Core::TaskCallback Callback;
				Core::Timer* Time = nullptr;
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
				size_t VoxelSize;
			} Display;

			struct
			{
				Core::Pool<Component*> Components;
				std::unordered_map<uint64_t, std::unordered_set<Component*>> Changes;
				std::vector<Entity*> Heap;
				std::mutex Transaction;
			} Indexer;

		protected:
#ifdef _DEBUG
			std::thread::id ThreadId;
#endif
			std::unordered_map<std::string, std::unordered_set<MessageCallback*>> Listeners;
			std::unordered_map<uint64_t, Table*> Registry;
			std::unordered_map<std::string, Packet*> Tasks;
			std::queue<Core::TaskCallback> Queue;
			std::queue<Event> Events;
			Core::Pool<Component*> Actors[(size_t)ActorType::Count];
			Core::Pool<Material*> Materials;
			Core::Pool<Entity*> Entities;
			Compute::Simulator* Simulator;
			std::condition_variable Stabilize;
			std::atomic<Component*> Camera;
			std::atomic<int> Status;
			std::atomic<bool> Acquire;
			std::atomic<bool> Active;
			std::atomic<bool> Resolve;
			std::mutex Emulation;
			std::mutex Race;
			Desc Conf;

		public:
			IdxSnapshot* Snapshot;

		public:
			SceneGraph(const Desc& I);
			virtual ~SceneGraph() override;
			void Configure(const Desc& Conf);
			void Actualize();
			void Redistribute();
			void Reindex();
			void ResizeBuffers();
			void Sleep();
			void Submit();
			void Conform();
			bool Dispatch(Core::Timer* Time);
			void Publish(Core::Timer* Time);
			void RemoveMaterial(Material* Value);
			void RemoveEntity(Entity* Entity, bool Destroy = true);
			void SetCamera(Entity* Camera);
			void SortBackToFront(Core::Pool<Drawable*>* Array);
			void SortFrontToBack(Core::Pool<Drawable*>* Array);
			void RayTest(uint64_t Section, const Compute::Ray& Origin, float MaxDistance, const RayCallback& Callback);
			void ScriptHook(const std::string& Name = "Main");
			void SetActive(bool Enabled);
			void SetTiming(double Min, double Max);
			void SetVoxelBufferSize(size_t Size);
			void SetMRT(TargetType Type, bool Clear);
			void SetRT(TargetType Type, bool Clear);
			void SwapMRT(TargetType Type, Graphics::MultiRenderTarget2D* New);
			void SwapRT(TargetType Type, Graphics::RenderTarget2D* New);
			void ClearMRT(TargetType Type, bool Color, bool Depth);
			void ClearRT(TargetType Type, bool Color, bool Depth);
			void Transaction(Core::TaskCallback&& Callback);
			void Mutate(Entity* Parent, Entity* Child, const char* Type);
			void Mutate(Entity* Target, const char* Type);
			void Mutate(Component* Target, const char* Type);
			void Mutate(Material* Target, const char* Type);
			void CloneEntity(Entity* Value, CloneCallback&& Callback);
			void MakeSnapshot(IdxSnapshot* Result);
			void ClearCulling();
			bool GetVoxelBuffer(Graphics::Texture3D** In, Graphics::Texture3D** Out);
			bool SetEvent(const std::string& EventName, Core::VariantArgs&& Args, bool Propagate);
			bool SetEvent(const std::string& EventName, Core::VariantArgs&& Args, Component* Target);
			bool SetEvent(const std::string& EventName, Core::VariantArgs&& Args, Entity* Target);
			bool SetParallel(const std::string& Name, PacketCallback&& Callback);
			MessageCallback* SetListener(const std::string& Event, MessageCallback&& Callback);
			bool ClearListener(const std::string& Event, MessageCallback* Id);
			Material* AddMaterial(Material* Base, const std::string& Name = "");
			Material* CloneMaterial(Material* Base, const std::string& Name = "");
			Entity* GetEntity(uint64_t Entity);
			Entity* GetLastEntity();
			Component* GetComponent(uint64_t Component, uint64_t Section);
			Component* GetCamera();
			RenderSystem* GetRenderer();
			Viewer GetCameraViewer();
			Material* GetMaterial(const std::string& Material);
			Material* GetMaterial(uint64_t Material);
			Table& GetStorage(uint64_t Section);
			Core::Pool<Component*>& GetComponents(uint64_t Section);
			Core::Pool<Component*>& GetActors(ActorType Type);
			Graphics::RenderTarget2D::Desc GetDescRT();
			Graphics::MultiRenderTarget2D::Desc GetDescMRT();
			Graphics::Format GetFormatMRT(unsigned int Target);
			std::vector<Entity*> QueryByParent(Entity* Parent);
			std::vector<Entity*> QueryByTag(uint64_t Tag);
			std::vector<Entity*> QueryByName(const std::string& Name);
			bool QueryByArea(uint64_t Section, const Compute::Vector3& Min, const Compute::Vector3& Max, const Compute::CosmosCallback& Callback);
			bool AddEntity(Entity* Entity);
			bool IsActive();
			bool IsLeftHanded();
			bool IsIndexed();
			uint64_t GetMaterialsCount();
			uint64_t GetEntitiesCount();
			uint64_t GetComponentsCount(uint64_t Section);
			size_t GetVoxelBufferSize();
			bool HasEntity(Entity* Entity);
			bool HasEntity(uint64_t Entity);
			bool IsUnstable();
			Graphics::MultiRenderTarget2D* GetMRT(TargetType Type);
			Graphics::RenderTarget2D* GetRT(TargetType Type);
			Graphics::Texture2D** GetMerger();
			Graphics::ElementBuffer* GetStructure();
			Graphics::GraphicsDevice* GetDevice();
			Compute::Simulator* GetSimulator();
			ShaderCache* GetShaders();
			PrimitiveCache* GetPrimitives();
			Desc& GetConf();

		private:
			void Simulate(Core::Timer* Time);
			void Synchronize(Core::Timer* Time);
			void Culling(Core::Timer* Time);

		protected:
			void ReindexComponent(Component* Base, bool Activation, bool Check, bool Notify);
			void CloneEntities(Entity* Instance, std::vector<Entity*>* Array);
			void ProcessCullable(Component* Base);
			void GenerateMaterialBuffer();
			void NotifyCosmos(Component* Base);
			void NotifyCosmosBulk();
			void UpdateCosmos(Component* Base);
			void UpdateCosmos();
			void ExecuteTasks();
			void FillMaterialBuffers();
			void ResizeRenderBuffers();
			void RegisterEntity(Entity* In);
			void ExclusiveLock();
			void ExclusiveUnlock();
			bool UnregisterEntity(Entity* In);
			bool ResolveEvents();
			Entity* CloneEntity(Entity* Entity);

		public:
			template <typename T>
			void RayTest(const Compute::Ray& Origin, float MaxDistance, RayCallback&& Callback)
			{
				RayTest(T::GetTypeId(), Origin, MaxDistance, std::move(Callback));
			}
			template <typename T>
			bool QueryByArea(const Compute::Vector3& Min, const Compute::Vector3& Max, const Compute::CosmosCallback& Callback)
			{
				return QueryByArea(T::GetTypeId(), Min, Max, Callback);
			}
			template <typename T>
			uint64_t GetEntitisCount()
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
			Table& GetStorage()
			{
				return GetStorage(T::GetTypeId());
			}
			template <typename T>
			Core::Pool<Component*>& GetComponents()
			{
				return GetComponents(T::GetTypeId());
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

		class TH_OUT ContentManager : public Core::Object
		{
		private:
			std::unordered_map<std::string, std::unordered_map<Processor*, AssetCache*>> Assets;
			std::unordered_map<std::string, AssetArchive*> Dockers;
			std::unordered_map<int64_t, Processor*> Processors;
			std::unordered_map<Core::Stream*, int64_t> Streams;
			Graphics::GraphicsDevice* Device;
			std::string Environment, Base;
			std::mutex Mutex;

		public:
			ContentManager(Graphics::GraphicsDevice* NewDevice);
			virtual ~ContentManager() override;
			void InvalidateDockers();
			void InvalidateCache();
			void InvalidatePath(const std::string& Path);
			void SetEnvironment(const std::string& Path);
			void SetDevice(Graphics::GraphicsDevice* NewDevice);
			bool Import(const std::string& Path);
			bool Export(const std::string& Path, const std::string& Directory, const std::string& Name = "");
			bool Cache(Processor* Root, const std::string& Path, void* Resource);
			Graphics::GraphicsDevice* GetDevice();
			std::string GetEnvironment();

		public:
			template <typename T>
			T* Load(const std::string& Path, const Core::VariantArgs& Keys = Core::VariantArgs())
			{
				return (T*)LoadForward(Path, GetProcessor<T>(), Keys);
			}
			template <typename T>
			Core::Async<T*> LoadAsync(const std::string& Path, const Core::VariantArgs& Keys = Core::VariantArgs())
			{
				return Core::Async<T*>::Execute([this, Path, Keys](Core::Async<T*>& Future)
				{
					Base = (T*)LoadForward(Path, GetProcessor<T>(), Keys);
				});
			}
			template <typename T>
			bool Save(const std::string& Path, T* Object, const Core::VariantArgs& Keys = Core::VariantArgs())
			{
				return SaveForward(Path, GetProcessor<T>(), Object, Keys);
			}
			template <typename T>
			Core::Async<bool> SaveAsync(const std::string& Path, T* Object, const Core::VariantArgs& Keys = Core::VariantArgs())
			{
				return Core::Async<bool>::Execute([this, Path, Object, Keys](Core::Async<bool>& Future)
				{
					Base = SaveForward(Path, GetProcessor<T>(), Object, Keys);
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

				TH_RELEASE(It->second);
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
					TH_RELEASE(It->second);
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
			void* LoadForward(const std::string& Path, Processor* Processor, const Core::VariantArgs& Keys);
			void* LoadStreaming(const std::string& Path, Processor* Processor, const Core::VariantArgs& Keys);
			bool SaveForward(const std::string& Path, Processor* Processor, void* Object, const Core::VariantArgs& Keys);
			AssetCache* Find(Processor* Target, const std::string& Path);
			AssetCache* Find(Processor* Target, void* Resource);
		};

		class TH_OUT Application : public Core::Object
		{
		public:
			struct Desc
			{
				Graphics::GraphicsDevice::Desc GraphicsDevice;
				Graphics::Activity::Desc Activity;
				std::string Environment;
				std::string Directory;
				uint64_t Stack = TH_STACKSIZE;
				uint64_t Coroutines = 16;
				uint64_t Threads = 0;
				double Framerate = 0;
				double MaxFrames = 60;
				double MinFrames = 10;
				size_t Usage =
					(size_t)ApplicationSet::GraphicsSet |
					(size_t)ApplicationSet::ActivitySet |
					(size_t)ApplicationSet::AudioSet |
					(size_t)ApplicationSet::ScriptSet |
					(size_t)ApplicationSet::ContentSet |
					(size_t)ApplicationSet::NetworkSet;
				bool Daemon = false;
				bool Cursor = true;
				bool Async = false;
			};

		public:
			struct
			{
				ShaderCache* Shaders = nullptr;
				PrimitiveCache* Primitives = nullptr;
			} Cache;

		private:
			static Application* Host;

		private:
			ApplicationState State = ApplicationState::Terminated;
			bool NetworkQueue = false;

		public:
			Audio::AudioDevice* Audio = nullptr;
			Graphics::GraphicsDevice* Renderer = nullptr;
			Graphics::Activity* Activity = nullptr;
			Script::VMManager* VM = nullptr;
			ContentManager* Content = nullptr;
			SceneGraph* Scene = nullptr;
			Desc Control;

		public:
			Application(Desc* I);
			virtual ~Application() override;
			virtual void ScriptHook(Script::VMGlobal* Global);
			virtual void KeyEvent(Graphics::KeyCode Key, Graphics::KeyMod Mod, int Virtual, int Repeat, bool Pressed);
			virtual void InputEvent(char* Buffer, int Length);
			virtual void WheelEvent(int X, int Y, bool Normal);
			virtual void WindowEvent(Graphics::WindowState NewState, int X, int Y);
			virtual void CloseEvent();
			virtual bool ComposeEvent();
			virtual void Dispatch(Core::Timer* Time);
			virtual void Publish(Core::Timer* Time);
			virtual void Initialize();
			virtual void* GetGUI();
			ApplicationState GetState();
			void Start();
			void Stop();

		private:
			static bool Status(Application* App);
			static void Compose();

		public:
			static Core::Schedule* Queue();
			static Application* Get();
		};

		template <typename T, size_t Max = (size_t)MAX_STACK_DEPTH>
		class RendererProxy
		{
			static_assert(std::is_base_of<Component, T>::value, "parameter must be derived from a component");

		public:
			typedef std::vector<T*> Storage;

		public:
			static const size_t Depth = Max;

		private:
			Storage Data[Max][(size_t)GeoCategory::Count];
			size_t Offset;

		public:
			RendererProxy() : Offset(0)
			{
			}
			Storage& Top(GeoCategory Category = GeoCategory::Opaque)
			{
				return Data[Offset > 0 ? Offset - 1 : 0][(size_t)Category];
			}
			Storage& Push(RenderSystem* Base, GeoCategory Category = GeoCategory::Opaque)
			{
				TH_ASSERT(Base != nullptr, Data[0][(size_t)Category], "render system should be present");
				TH_ASSERT(Offset < Max - 1, Data[0][(size_t)Category], "storage heap stack overflow");

				Storage* Frame = Data[Offset++];
				if (Offset > 1)
					Subcull(Base, Frame);
				else
					Cullout(Base, Frame);

				return Frame[(size_t)Category];
			}
			void Pop()
			{
				TH_ASSERT_V(Offset > 0, "storage heap stack underflow");
				Offset--;
			}

		private:
			template<class Q = T>
			typename std::enable_if<std::is_base_of<Drawable, Q>::value>::type Cullout(RenderSystem* System, Storage* Top)
			{
				for (size_t i = 0; i < (size_t)GeoCategory::Count; ++i)
					Top[i].clear();

				T* Base = nullptr;
				System->QueryBegin(T::GetTypeId());
				while (Base = (T*)System->QueryNext())
				{
					if (System->EnqueueDrawable(Base) >= System->Threshold)
						Top[(size_t)Base->GetCategory()].push_back(Base);
				}
				System->QueryEnd();

				for (size_t i = 0; i < (size_t)GeoCategory::Count; ++i)
					std::sort(Top[i].begin(), Top[i].end(), Entity::Sortout<T>);
			}
			template<class Q = T>
			typename std::enable_if<!std::is_base_of<Drawable, Q>::value>::type Cullout(RenderSystem* System, Storage* Top)
			{
				auto& Subframe = Top[(size_t)GeoCategory::Opaque];
				Subframe.clear();

				T* Base = nullptr;
				System->QueryBegin(T::GetTypeId());
				while (Base = (T*)System->QueryNext())
				{
					if (System->EnqueueCullable(Base) >= System->Threshold)
						Subframe.push_back(Base);
				}
				System->QueryEnd();

				std::sort(Subframe.begin(), Subframe.end(), Entity::Sortout<T>);
			}
			template<class Q = T>
			typename std::enable_if<std::is_base_of<Drawable, Q>::value>::type Subcull(RenderSystem* System, Storage* Top)
			{
				for (size_t i = 0; i < (size_t)GeoCategory::Count; ++i)
					Top[i].clear();

				T* Base = nullptr;
				System->QueryBegin(T::GetTypeId());
				while (Base = (T*)System->QueryNext())
				{
					if (System->PushCullable(nullptr, System->View, Base))
						Top[(size_t)Base->GetCategory()].push_back(Base);
				}
				System->QueryEnd();

				for (size_t i = 0; i < (size_t)GeoCategory::Count; ++i)
					std::sort(Top[i].begin(), Top[i].end(), Entity::Sortout<T>);
			}
			template<class Q = T>
			typename std::enable_if<!std::is_base_of<Drawable, Q>::value>::type Subcull(RenderSystem* System, Storage* Top)
			{
				auto& Subframe = Top[(size_t)GeoCategory::Opaque];
				Subframe.clear();

				T* Base = nullptr;
				System->QueryBegin(T::GetTypeId());
				while (Base = (T*)System->QueryNext())
				{
					if (System->PushCullable(nullptr, System->View, Base))
						Subframe.push_back(Base);
				}
				System->QueryEnd();

				std::sort(Subframe.begin(), Subframe.end(), Entity::Sortout<T>);
			}
		};

		template <typename T>
		class TH_OUT GeometryRenderer : public Renderer
		{
			static_assert(std::is_base_of<Drawable, T>::value, "component must be drawable to work within geometry renderer");

		private:
			RendererProxy<T> Proxy;

		public:
			GeometryRenderer(RenderSystem* Lab) : Renderer(Lab)
			{
			}
			~GeometryRenderer()
			{
			}
			size_t CullingPass(const Viewer& View, bool Rebake) override
			{
				TH_PPUSH("geo-renderer-culling", TH_PERF_FRAME);
				auto& Frame = Proxy.Top(GeoCategory::Opaque);
				size_t Count = 0;

				if (!Frame.empty())
					Count += CullGeometry(View, Frame, Rebake);

				TH_PPOP();
				return Count;
			}
			size_t RenderPass(Core::Timer* Time, RenderState State, RenderOpt Options) override
			{
				size_t Count = 0;
				if (State == RenderState::Geometry_Result)
				{
					TH_PPUSH("geo-renderer-result", TH_PERF_CORE);
					GeoCategory Category = GeoCategory::Opaque;
					if ((size_t)Options & (size_t)RenderOpt::Transparent)
						Category = GeoCategory::Transparent;
					else if ((size_t)Options & (size_t)RenderOpt::Additive)
						Category = GeoCategory::Additive;

					auto& Frame = Proxy.Top(Category);
					if (!Frame.empty())
					{
						System->ClearMaterials();
						Count += RenderGeometryResult(Time, Frame, Options);
					}
					TH_PPOP();
				}
				else if (State == RenderState::Geometry_Voxels)
				{
					if ((size_t)Options & (size_t)RenderOpt::Transparent || (size_t)Options & (size_t)RenderOpt::Additive)
						return 0;

					TH_PPUSH("geo-renderer-voxels", TH_PERF_MIX);
					auto& Frame = Proxy.Top(GeoCategory::Opaque);
					if (!Frame.empty())
					{
						System->ClearMaterials();
						Count += RenderGeometryVoxels(Time, Frame, Options);
					}
					TH_PPOP();
				}
				else if (State == RenderState::Depth_Linear)
				{
					if (!((size_t)Options & (size_t)RenderOpt::Inner))
						return 0;

					TH_PPUSH("geo-renderer-depth-linear", TH_PERF_FRAME);
					System->ClearMaterials();
					auto& Frame = Proxy.Top(GeoCategory::Opaque);
					if (!Frame.empty())
						Count += RenderDepthLinear(Time, Frame);

					Frame = Proxy.Top(GeoCategory::Transparent);
					if (!Frame.empty())
						Count += RenderDepthLinear(Time, Frame);
					TH_PPOP();
				}
				else if (State == RenderState::Depth_Cubic)
				{
					if (!((size_t)Options & (size_t)RenderOpt::Inner))
						return 0;

					TH_PPUSH("geo-renderer-depth-cubic", TH_PERF_FRAME);
					System->ClearMaterials();
					auto& Frame = Proxy.Top(GeoCategory::Opaque);
					if (!Frame.empty())
						Count += RenderDepthCubic(Time, Frame, System->View.CubicViewProjection);

					Frame = Proxy.Top(GeoCategory::Transparent);
					if (!Frame.empty())
						Count += RenderDepthCubic(Time, Frame, System->View.CubicViewProjection);
					TH_PPOP();
				}

				return Count;
			}
			virtual void BeginPass() override
			{
				Proxy.Push(System);
			}
			virtual void EndPass() override
			{
				Proxy.Pop();
			}
			virtual bool HasCategory(GeoCategory Category) override
			{
				return !Proxy.Top(Category).empty();
			}
			virtual size_t CullGeometry(const Viewer& View, const std::vector<T*>& Geometry, bool Rebake)
			{
				return 0;
			}
			virtual size_t RenderDepthLinear(Core::Timer* TimeStep, const std::vector<T*>& Geometry)
			{
				return 0;
			}
			virtual size_t RenderDepthCubic(Core::Timer* TimeStep, const std::vector<T*>& Geometry, Compute::Matrix4x4* ViewProjection)
			{
				return 0;
			}
			virtual size_t RenderGeometryVoxels(Core::Timer* TimeStep, const std::vector<T*>& Geometry, RenderOpt Options)
			{
				return 0;
			}
			virtual size_t RenderGeometryResult(Core::Timer* TimeStep, const std::vector<T*>& Geometry, RenderOpt Options) = 0;

		public:
			TH_COMPONENT("geometry-renderer");
		};

		class TH_OUT EffectRenderer : public Renderer
		{
		protected:
			std::unordered_map<std::string, Graphics::Shader*> Effects;
			Graphics::DepthStencilState* DepthStencil;
			Graphics::RasterizerState* Rasterizer;
			Graphics::BlendState* Blend;
			Graphics::SamplerState* Sampler;
			Graphics::InputLayout* Layout;
			Graphics::RenderTarget2D* Output;
			Graphics::RenderTarget2D* Swap;
			unsigned int MaxSlot;

		public:
			EffectRenderer(RenderSystem* Lab);
			virtual ~EffectRenderer() override;
			virtual void ResizeEffect();
			virtual void RenderEffect(Core::Timer* Time) = 0;
			size_t RenderPass(Core::Timer* Time, RenderState State, RenderOpt Options) override;
			void ResizeBuffers() override;
			unsigned int GetMipLevels();
			unsigned int GetWidth();
			unsigned int GetHeight();

		protected:
			void RenderOutput(Graphics::RenderTarget2D* Resource = nullptr);
			void RenderTexture(uint32_t Slot6, Graphics::Texture2D* Resource = nullptr);
			void RenderTexture(uint32_t Slot6, Graphics::Texture3D* Resource = nullptr);
			void RenderTexture(uint32_t Slot6, Graphics::TextureCube* Resource = nullptr);
			void RenderMerge(Graphics::Shader* Effect, void* Buffer = nullptr, size_t Count = 1);
			void RenderResult(Graphics::Shader* Effect, void* Buffer = nullptr);
			Graphics::Shader* GetEffect(const std::string& Name);
			Graphics::Shader* CompileEffect(Graphics::Shader::Desc& Desc, size_t BufferSize = 0);
			Graphics::Shader* CompileEffect(const std::string& SectionName, size_t BufferSize = 0);

		public:
			TH_COMPONENT("effect-renderer");
		};
	}
}
#endif
