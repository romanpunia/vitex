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

		class Cullable;

		class Processor;

		class RenderSystem;

		class Material;

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

		enum class CullResult
		{
			Always,
			Cache,
			Last
		};

		enum class RenderOpt
		{
			None = 0,
			Transparent = 1,
			Static = 2,
			Inner = 4,
			Additive = 8
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
			Additive
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
			Message = 1 << 2
		};

		enum class ActorType
		{
			Update,
			Synchronize,
			Message,
			Count
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

			void Set(const Compute::Matrix4x4& View, const Compute::Matrix4x4& Projection, const Compute::Vector3& Position, float Near, float Far);
			void Set(const Compute::Matrix4x4& View, const Compute::Matrix4x4& Projection, const Compute::Vector3& Position, const Compute::Vector3& Rotation, float Near, float Far);
		};

		struct TH_OUT Attenuation
		{
			float Range = 10.0f;
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
			static void Pack(Core::Document* V, bool Value);
			static void Pack(Core::Document* V, int Value);
			static void Pack(Core::Document* V, unsigned int Value);
			static void Pack(Core::Document* V, unsigned long Value);
			static void Pack(Core::Document* V, float Value);
			static void Pack(Core::Document* V, double Value);
			static void Pack(Core::Document* V, int64_t Value);
			static void Pack(Core::Document* V, long double Value);
			static void Pack(Core::Document* V, unsigned long long Value);
			static void Pack(Core::Document* V, const char* Value);
			static void Pack(Core::Document* V, const Compute::Vector2& Value);
			static void Pack(Core::Document* V, const Compute::Vector3& Value);
			static void Pack(Core::Document* V, const Compute::Vector4& Value);
			static void Pack(Core::Document* V, const Compute::Matrix4x4& Value);
			static void Pack(Core::Document* V, const Attenuation& Value);
			static void Pack(Core::Document* V, const AnimatorState& Value);
			static void Pack(Core::Document* V, const SpawnerProperties& Value);
			static void Pack(Core::Document* V, Material* Value, ContentManager* Content);
			static void Pack(Core::Document* V, const Compute::SkinAnimatorKey& Value);
			static void Pack(Core::Document* V, const Compute::SkinAnimatorClip& Value);
			static void Pack(Core::Document* V, const Compute::KeyAnimatorClip& Value);
			static void Pack(Core::Document* V, const Compute::AnimatorKey& Value);
			static void Pack(Core::Document* V, const Compute::ElementVertex& Value);
			static void Pack(Core::Document* V, const Compute::Joint& Value);
			static void Pack(Core::Document* V, const Compute::Vertex& Value);
			static void Pack(Core::Document* V, const Compute::SkinVertex& Value);
			static void Pack(Core::Document* V, const Core::Ticker& Value);
			static void Pack(Core::Document* V, const std::string& Value);
			static void Pack(Core::Document* V, const std::vector<bool>& Value);
			static void Pack(Core::Document* V, const std::vector<int>& Value);
			static void Pack(Core::Document* V, const std::vector<unsigned int>& Value);
			static void Pack(Core::Document* V, const std::vector<float>& Value);
			static void Pack(Core::Document* V, const std::vector<double>& Value);
			static void Pack(Core::Document* V, const std::vector<int64_t>& Value);
			static void Pack(Core::Document* V, const std::vector<long double>& Value);
			static void Pack(Core::Document* V, const std::vector<uint64_t>& Value);
			static void Pack(Core::Document* V, const std::vector<Compute::Vector2>& Value);
			static void Pack(Core::Document* V, const std::vector<Compute::Vector3>& Value);
			static void Pack(Core::Document* V, const std::vector<Compute::Vector4>& Value);
			static void Pack(Core::Document* V, const std::vector<Compute::Matrix4x4>& Value);
			static void Pack(Core::Document* V, const std::vector<AnimatorState>& Value);
			static void Pack(Core::Document* V, const std::vector<SpawnerProperties>& Value);
			static void Pack(Core::Document* V, const std::vector<Compute::SkinAnimatorClip>& Value);
			static void Pack(Core::Document* V, const std::vector<Compute::KeyAnimatorClip>& Value);
			static void Pack(Core::Document* V, const std::vector<Compute::AnimatorKey>& Value);
			static void Pack(Core::Document* V, const std::vector<Compute::ElementVertex>& Value);
			static void Pack(Core::Document* V, const std::vector<Compute::Joint>& Value);
			static void Pack(Core::Document* V, const std::vector<Compute::Vertex>& Value);
			static void Pack(Core::Document* V, const std::vector<Compute::SkinVertex>& Value);
			static void Pack(Core::Document* V, const std::vector<Core::Ticker>& Value);
			static void Pack(Core::Document* V, const std::vector<std::string>& Value);
			static bool Unpack(Core::Document* V, bool* O);
			static bool Unpack(Core::Document* V, int* O);
			static bool Unpack(Core::Document* V, unsigned int* O);
			static bool Unpack(Core::Document* V, unsigned long* O);
			static bool Unpack(Core::Document* V, float* O);
			static bool Unpack(Core::Document* V, double* O);
			static bool Unpack(Core::Document* V, int64_t* O);
			static bool Unpack(Core::Document* V, long double* O);
			static bool Unpack(Core::Document* V, unsigned long long* O);
			static bool Unpack(Core::Document* V, Compute::Vector2* O);
			static bool Unpack(Core::Document* V, Compute::Vector3* O);
			static bool Unpack(Core::Document* V, Compute::Vector4* O);
			static bool Unpack(Core::Document* V, Compute::Matrix4x4* O);
			static bool Unpack(Core::Document* V, Attenuation* O);
			static bool Unpack(Core::Document* V, AnimatorState* O);
			static bool Unpack(Core::Document* V, SpawnerProperties* O);
			static bool Unpack(Core::Document* V, Material* O, ContentManager* Content);
			static bool Unpack(Core::Document* V, Compute::SkinAnimatorKey* O);
			static bool Unpack(Core::Document* V, Compute::SkinAnimatorClip* O);
			static bool Unpack(Core::Document* V, Compute::KeyAnimatorClip* O);
			static bool Unpack(Core::Document* V, Compute::AnimatorKey* O);
			static bool Unpack(Core::Document* V, Compute::ElementVertex* O);
			static bool Unpack(Core::Document* V, Compute::Joint* O);
			static bool Unpack(Core::Document* V, Compute::Vertex* O);
			static bool Unpack(Core::Document* V, Compute::SkinVertex* O);
			static bool Unpack(Core::Document* V, Core::Ticker* O);
			static bool Unpack(Core::Document* V, std::string* O);
			static bool Unpack(Core::Document* V, std::vector<bool>* O);
			static bool Unpack(Core::Document* V, std::vector<int>* O);
			static bool Unpack(Core::Document* V, std::vector<unsigned int>* O);
			static bool Unpack(Core::Document* V, std::vector<float>* O);
			static bool Unpack(Core::Document* V, std::vector<double>* O);
			static bool Unpack(Core::Document* V, std::vector<int64_t>* O);
			static bool Unpack(Core::Document* V, std::vector<long double>* O);
			static bool Unpack(Core::Document* V, std::vector<uint64_t>* O);
			static bool Unpack(Core::Document* V, std::vector<Compute::Vector2>* O);
			static bool Unpack(Core::Document* V, std::vector<Compute::Vector3>* O);
			static bool Unpack(Core::Document* V, std::vector<Compute::Vector4>* O);
			static bool Unpack(Core::Document* V, std::vector<Compute::Matrix4x4>* O);
			static bool Unpack(Core::Document* V, std::vector<AnimatorState>* O);
			static bool Unpack(Core::Document* V, std::vector<SpawnerProperties>* O);
			static bool Unpack(Core::Document* V, std::vector<Compute::SkinAnimatorClip>* O);
			static bool Unpack(Core::Document* V, std::vector<Compute::KeyAnimatorClip>* O);
			static bool Unpack(Core::Document* V, std::vector<Compute::AnimatorKey>* O);
			static bool Unpack(Core::Document* V, std::vector<Compute::ElementVertex>* O);
			static bool Unpack(Core::Document* V, std::vector<Compute::Joint>* O);
			static bool Unpack(Core::Document* V, std::vector<Compute::Vertex>* O);
			static bool Unpack(Core::Document* V, std::vector<Compute::SkinVertex>* O);
			static bool Unpack(Core::Document* V, std::vector<Core::Ticker>* O);
			static bool Unpack(Core::Document* V, std::vector<std::string>* O);
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
			friend Entity;

		protected:
			Entity* Parent;

		private:
			size_t Set;
			bool Active;

		public:
			Component(Entity* Ref, ActorSet Rule);
			virtual ~Component() override;
			virtual void Serialize(ContentManager* Content, Core::Document* Node);
			virtual void Deserialize(ContentManager* Content, Core::Document* Node);
			virtual void Activate(Component* New);
			virtual void Deactivate();
			virtual void Synchronize(Core::Timer* Time);
			virtual void Update(Core::Timer* Time);
			virtual void Message(const std::string& Name, Core::VariantArgs& Args);
			virtual Component* Copy(Entity* New) = 0;
			virtual Compute::Matrix4x4 GetBoundingBox();
			Entity* GetEntity();
			void SetActive(bool Enabled);
			bool IsActive();

		public:
			TH_COMPONENT_ROOT("component");
		};

		class TH_OUT Entity : public Core::Object
		{
			friend SceneGraph;

		protected:
			std::unordered_map<uint64_t, Component*> Components;
			SceneGraph* Scene;

		private:
			Compute::Transform* Transform;
			std::string Name;

		public:
			int64_t Tag;
			float Distance;

		public:
			Entity(SceneGraph* Ref);
			virtual ~Entity() override;
			void SetName(const std::string& Value, bool Internal = false);
			void SetRoot(Entity* Parent);
			void RemoveComponent(uint64_t Id);
			void RemoveChilds();
			Component* AddComponent(Component* In);
			Component* GetComponent(uint64_t Id);
			uint64_t GetComponentCount() const;
			SceneGraph* GetScene() const;
			Entity* GetParent() const;
			Entity* GetChild(size_t Index) const;
			Compute::Transform* GetTransform() const;
			const std::string& GetName() const;
			size_t GetChildsCount() const;

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
			virtual void Serialize(ContentManager* Content, Core::Document* Node);
			virtual void Deserialize(ContentManager* Content, Core::Document* Node);
			virtual void CullGeometry(const Viewer& View);
			virtual void ResizeBuffers();
			virtual void Activate();
			virtual void Deactivate();
			virtual void Render(Core::Timer* TimeStep, RenderState State, RenderOpt Options);
			void SetRenderer(RenderSystem* NewSystem);
			RenderSystem* GetRenderer();

		public:
			TH_COMPONENT_ROOT("renderer");
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

		class TH_OUT RenderSystem : public Core::Object
		{
		protected:
			std::vector<Renderer*> Renderers;
			std::unordered_set<uint64_t> Cull;
			Graphics::DepthStencilState* DepthStencil;
			Graphics::BlendState* Blend;
			Graphics::SamplerState* Sampler;
			Graphics::DepthTarget2D* Target;
			Graphics::GraphicsDevice* Device;
			Material* BaseMaterial;
			SceneGraph* Scene;
			size_t DepthSize;
			bool OcclusionCulling;
			bool FrustumCulling;
			bool Satisfied;

		public:
			Core::Ticker Occlusion;
			Core::Ticker Sorting;
			size_t StallFrames;
			bool PreciseCulling;

		public:
			RenderSystem(SceneGraph* NewScene);
			virtual ~RenderSystem() override;
			void SetOcclusionCulling(bool Enabled, bool KeepResults = false);
			void SetFrustumCulling(bool Enabled, bool KeepResults = false);
			void SetDepthSize(size_t Size);
			void Remount(Renderer* Target);
			void Remount();
			void Mount();
			void Unmount();
			void ClearCull();
			void CullGeometry(Core::Timer* Time, const Viewer& View);
			void Synchronize(Core::Timer* Time, const Viewer& View);
			void MoveRenderer(uint64_t Id, int64_t Offset);
			void RemoveRenderer(uint64_t Id);
			void RestoreOutput();
			void FreeShader(const std::string& Name, Graphics::Shader* Shader);
			void FreeShader(Graphics::Shader* Shader);
			void FreeBuffers(const std::string& Name, Graphics::ElementBuffer** Buffers);
			void FreeBuffers(Graphics::ElementBuffer** Buffers);
			void ClearMaterials();
			bool PushGeometryBuffer(Material* Next);
			bool PushVoxelsBuffer(Material* Next);
			bool PushDepthLinearBuffer(Material* Next);
			bool PushDepthCubicBuffer(Material* Next);
			bool PassCullable(Cullable* Base, CullResult Mode, float* Result);
			bool PassDrawable(Drawable* Base, CullResult Mode, float* Result);
			bool HasOcclusionCulling();
			bool HasFrustumCulling();
			int64_t GetOffset(uint64_t Id);
			Graphics::Shader* CompileShader(Graphics::Shader::Desc& Desc, size_t BufferSize = 0);
			Graphics::Shader* CompileShader(const std::string& SectionName, size_t BufferSize = 0);
			bool CompileBuffers(Graphics::ElementBuffer** Result, const std::string& Name, size_t ElementSize, size_t ElementsCount);
			Renderer* AddRenderer(Renderer* In);
			Renderer* GetRenderer(uint64_t Id);
			size_t GetDepthSize();
			std::vector<Renderer*>* GetRenderers();
			Graphics::MultiRenderTarget2D* GetMRT(TargetType Type);
			Graphics::RenderTarget2D* GetRT(TargetType Type);
			Graphics::Texture2D** GetMerger();
			Graphics::GraphicsDevice* GetDevice();
			PrimitiveCache* GetPrimitives();
			SceneGraph* GetScene();

		private:
			Core::Pool<Component*>* GetSceneComponents(uint64_t Section);

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
			Core::Pool<Component*>* AddCull()
			{
				static_assert(std::is_base_of<Cullable, T>::value,
					"component is not cullable");

				Cull.insert(T::GetTypeId());
				return GetSceneComponents(T::GetTypeId());
			}
			template <typename T>
			Core::Pool<Component*>* RemoveCull()
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
			Cullable(Entity* Ref, ActorSet Rule);
			virtual ~Cullable() = default;
			virtual bool IsVisible(const Viewer& View, Compute::Matrix4x4* World);
			virtual bool IsNear(const Viewer& View);
			virtual float Cull(const Viewer& View) = 0;
			virtual Component* Copy(Entity* New) override = 0;
			virtual void ClearCull();
			float GetRange();

		public:
			TH_COMPONENT("cullable");
		};

		class TH_OUT Drawable : public Cullable
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
			virtual void ClearCull() override;
			bool SetCategory(GeoCategory Category);
			bool SetMaterial(void* Instance, Material* Value);
			GeoCategory GetCategory();
			int64_t GetSlot(void* Surface);
			int64_t GetSlot();
			Material* GetMaterial(void* Surface);
			Material* GetMaterial();
			const std::unordered_map<void*, Material*>& GetMaterials();

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
			virtual void CullGeometry(const Viewer& View, Core::Pool<Drawable*>* Geometry);
			virtual void RenderGeometryResult(Core::Timer* TimeStep, Core::Pool<Drawable*>* Geometry, RenderOpt Options) = 0;
			virtual void RenderGeometryVoxels(Core::Timer* TimeStep, Core::Pool<Drawable*>* Geometry, RenderOpt Options) = 0;
			virtual void RenderDepthLinear(Core::Timer* TimeStep, Core::Pool<Drawable*>* Geometry) = 0;
			virtual void RenderDepthCubic(Core::Timer* TimeStep, Core::Pool<Drawable*>* Geometry, Compute::Matrix4x4* ViewProjection) = 0;
			void CullGeometry(const Viewer& View) override;
			void Render(Core::Timer* TimeStep, RenderState State, RenderOpt Options) override;
			Core::Pool<Drawable*>* GetOpaque();
			Core::Pool<Drawable*>* GetTransparent();
			Core::Pool<Drawable*>* GetAdditive();

		public:
			TH_COMPONENT("geometry-draw");
		};

		class TH_OUT EffectDraw : public Renderer
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
			EffectDraw(RenderSystem* Lab);
			virtual ~EffectDraw() override;
			virtual void ResizeEffect();
			virtual void RenderEffect(Core::Timer* Time);
			void Render(Core::Timer* Time, RenderState State, RenderOpt Options) override;
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
			TH_COMPONENT("effect-draw");
		};

		class TH_OUT SceneGraph : public Core::Object
		{
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
				uint64_t MaterialCount = 1ll << 14;
				uint64_t EntityCount = 1ll << 15;
				uint64_t ComponentCount = 1ll << 16;
				double MaxFrames = 60.0;
				double MinFrames = 10.0;
				double FrequencyHZ = 480.0;
				float RenderQuality = 1.0f;
				bool EnableHDR = false;
				bool Async = true;

				static Desc Get(Application* Base);
			};

		private:
			struct Packet
			{
				std::atomic<bool> Active;
				Core::TaskCallback Callback;
				Core::Timer* Time = nullptr;
			};

			struct Geometry
			{
				Core::Pool<Drawable*> Opaque;
				Core::Pool<Drawable*> Transparent;
				Core::Pool<Drawable*> Additive;
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

		protected:
#ifdef _DEBUG
			std::thread::id ThreadId;
#endif
			std::unordered_map<std::string, std::unordered_set<MessageCallback*>> Listeners;
			std::unordered_map<uint64_t, Core::Pool<Component*>> Components;
			std::unordered_map<uint64_t, Geometry> Drawables;
			std::unordered_map<std::string, Packet*> Tasks;
			std::queue<Core::TaskCallback> Queue;
			std::queue<Event> Events;
			Core::Pool<Component*> Actors[(size_t)ActorType::Count];
			Core::Pool<Material*> Materials;
			Core::Pool<Entity*> Entities;
			Compute::Simulator* Simulator;
			std::condition_variable Stabilize;
			std::atomic<Component*> Camera;
			std::atomic<uint64_t> Surfaces;
			std::atomic<int> Status;
			std::atomic<bool> Acquire;
			std::atomic<bool> Active;
			std::atomic<bool> Resolve;
			std::mutex Race;
			Desc Conf;

		public:
			IdxSnapshot* Snapshot;
			Viewer View;

		public:
			SceneGraph(const Desc& I);
			virtual ~SceneGraph() override;
			void Configure(const Desc& Conf);
			void ExclusiveLock();
			void ExclusiveUnlock();
			void Actualize();
			void Redistribute();
			void Reindex();
			void ExpandMaterials();
			void ResizeBuffers();
			void Sleep();
			void Submit();
			void Conform();
			bool Dispatch(Core::Timer* Time);
			void Publish(Core::Timer* Time);
			void Render(Core::Timer* Time, RenderState Stage, RenderOpt Options);
			void RemoveMaterial(Material* Value);
			void RemoveEntity(Entity* Entity, bool Release);
			void SetCamera(Entity* Camera);
			void RestoreViewBuffer(Viewer* View);
			void SortBackToFront(Core::Pool<Drawable*>* Array);
			void SortFrontToBack(Core::Pool<Drawable*>* Array);
			void RayTest(uint64_t Section, const Compute::Ray& Origin, float MaxDistance, const RayCallback& Callback);
			void ScriptHook(const std::string& Name = "Main");
			void SetActive(bool Enabled);
			void SetTiming(double Min, double Max);
			void SetView(const Compute::Matrix4x4& View, const Compute::Matrix4x4& Projection, const Compute::Vector3& Position, float Near, float Far, bool Upload);
			void SetVoxelBufferSize(size_t Size);
			void SetMRT(TargetType Type, bool Clear);
			void SetRT(TargetType Type, bool Clear);
			void SwapMRT(TargetType Type, Graphics::MultiRenderTarget2D* New);
			void SwapRT(TargetType Type, Graphics::RenderTarget2D* New);
			void ClearMRT(TargetType Type, bool Color, bool Depth);
			void ClearRT(TargetType Type, bool Color, bool Depth);
			void Exclusive(Core::TaskCallback&& Callback);
			void Mutate(Entity* Parent, Entity* Child, const char* Type);
			void Mutate(Entity* Target, const char* Type);
			void Mutate(Component* Target, const char* Type);
			void Mutate(Material* Target, const char* Type);
			void CloneEntity(Entity* Value, CloneCallback&& Callback);
			void MakeSnapshot(IdxSnapshot* Result);
			bool GetVoxelBuffer(Graphics::Texture3D** In, Graphics::Texture3D** Out);
			bool SetEvent(const std::string& EventName, Core::VariantArgs&& Args, bool Propagate);
			bool SetEvent(const std::string& EventName, Core::VariantArgs&& Args, Component* Target);
			bool SetEvent(const std::string& EventName, Core::VariantArgs&& Args, Entity* Target);
			bool SetParallel(const std::string& Name, PacketCallback&& Callback);
			MessageCallback* SetListener(const std::string& Event, MessageCallback&& Callback);
			bool ClearListener(const std::string& Event, MessageCallback* Id);
			Material* AddMaterial(Material* Base, const std::string& Name = "");
			Material* CloneMaterial(Material* Base, const std::string& Name = "");
			Entity* FindNamedEntity(const std::string& Name);
			Entity* FindEntityAt(const Compute::Vector3& Position, float Radius);
			Entity* FindTaggedEntity(uint64_t Tag);
			Entity* GetEntity(uint64_t Entity);
			Entity* GetLastEntity();
			Component* GetComponent(uint64_t Component, uint64_t Section);
			Component* GetCamera();
			RenderSystem* GetRenderer();
			Viewer GetCameraViewer();
			Material* GetMaterial(const std::string& Material);
			Material* GetMaterial(uint64_t Material);
			Core::Pool<Component*>* GetComponents(uint64_t Section);
			Graphics::RenderTarget2D::Desc GetDescRT();
			Graphics::MultiRenderTarget2D::Desc GetDescMRT();
			Graphics::Format GetFormatMRT(unsigned int Target);
			std::vector<Entity*> FindParentFreeEntities(Entity* Entity);
			std::vector<Entity*> FindNamedEntities(const std::string& Name);
			std::vector<Entity*> FindEntitiesAt(const Compute::Vector3& Position, float Radius);
			std::vector<Entity*> FindTaggedEntities(uint64_t Tag);
			bool IsEntityVisible(Entity* Entity, const Compute::Matrix4x4& ViewProjection);
			bool IsEntityVisible(Entity* Entity, const Compute::Matrix4x4& ViewProjection, const Compute::Vector3& ViewPos, float DrawDistance);
			bool AddEntity(Entity* Entity);
			bool IsActive();
			bool IsLeftHanded();
			uint64_t GetMaterialsCount();
			uint64_t GetEntitiesCount();
			uint64_t GetComponentsCount(uint64_t Section);
			uint64_t GetOpaquesCount();
			uint64_t GetTransparentsCount();
			uint64_t GetAdditivesCount();
			size_t GetVoxelBufferSize();
			bool HasEntity(Entity* Entity);
			bool HasEntity(uint64_t Entity);
			bool IsUnstable();
			Core::Pool<Drawable*>* GetOpaque(uint64_t Section);
			Core::Pool<Drawable*>* GetTransparent(uint64_t Section);
			Core::Pool<Drawable*>* GetAdditive(uint64_t Section);
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

		protected:
			void CloneEntities(Entity* Instance, std::vector<Entity*>* Array);
			void ExecuteTasks();
			void FillMaterialBuffers();
			void ResizeRenderBuffers();
			void AddDrawable(Drawable* Source, GeoCategory Category);
			void RemoveDrawable(Drawable* Source, GeoCategory Category);
			void RegisterEntity(Entity* In);
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
			Core::Pool<Component*>* GetComponents()
			{
				return GetComponents(T::GetTypeId());
			}
			template <typename T>
			Core::Pool<Drawable*>* GetOpaque()
			{
				return GetOpaque(T::GetTypeId());
			}
			template <typename T>
			Core::Pool<Drawable*>* GetTransparent()
			{
				return GetTransparent(T::GetTypeId());
			}
			template <typename T>
			Core::Pool<Drawable*>* GetAdditive()
			{
				return GetAdditive(T::GetTypeId());
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
			static void Compose();

		public:
			static Core::Schedule* Queue();
			static Application* Get();
		};
	}
}
#endif
