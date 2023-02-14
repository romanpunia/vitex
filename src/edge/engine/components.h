#ifndef ED_ENGINE_COMPONENTS_H
#define ED_ENGINE_COMPONENTS_H
#include "../core/engine.h"

namespace Edge
{
	namespace Engine
	{
		namespace Components
		{
			class ED_OUT SoftBody final : public Drawable
			{
			protected:
				Compute::HullShape* Hull = nullptr;
				Compute::SoftBody * Instance = nullptr;
				std::vector<Compute::Vertex> Vertices;
				std::vector<int> Indices;

			public:
				Compute::Vector2 TexCoord = 1.0f;
				bool Kinematic = false;
				bool Manage = true;

			public:
				SoftBody(Entity* Ref);
				virtual ~SoftBody() override;
				virtual void Deserialize(Core::Schema* Node) override;
				virtual void Serialize(Core::Schema* Node) override;
				virtual void Synchronize(Core::Timer* Time) override;
				virtual void Deactivate() override;
				virtual float GetVisibility(const Viewer& View, float Distance) const override;
				virtual size_t GetUnitBounds(Compute::Vector3& Min, Compute::Vector3& Max) const override;
				virtual Core::Unique<Component> Copy(Entity* New) const override;
				void Load(Compute::HullShape* Shape, float Anticipation = 0.0f);
				void Load(const std::string& Path, float Anticipation = 0.0f, const std::function<void()>& Callback = nullptr);
				void LoadEllipsoid(const Compute::SoftBody::Desc::CV::SEllipsoid& Shape, float Anticipation = 0.0f);
				void LoadPatch(const Compute::SoftBody::Desc::CV::SPatch& Shape, float Anticipation = 0.0f);
				void LoadRope(const Compute::SoftBody::Desc::CV::SRope& Shape, float Anticipation = 0.0f);
				void Fill(Graphics::GraphicsDevice* Device, Graphics::ElementBuffer* IndexBuffer, Graphics::ElementBuffer* VertexBuffer);
				void Regenerate();
				void Clear();
				void SetTransform(const Compute::Vector3& Position, const Compute::Vector3& Scale, const Compute::Vector3& Rotation);
				void SetTransform(bool Kinematic);
				Compute::SoftBody* GetBody();
				std::vector<Compute::Vertex>& GetVertices();
				std::vector<int>& GetIndices();

			private:
				void DeserializeBody(Core::Schema* Node);

			public:
				ED_COMPONENT("soft-body");
			};

			class ED_OUT RigidBody final : public Component
			{
			private:
				Compute::HullShape * Hull = nullptr;
				Compute::RigidBody* Instance = nullptr;

			public:
				bool Kinematic = false;
				bool Manage = true;

			public:
				RigidBody(Entity* Ref);
				virtual ~RigidBody() override;
				virtual void Deserialize(Core::Schema* Node) override;
				virtual void Serialize(Core::Schema* Node) override;
				virtual void Synchronize(Core::Timer* Time) override;
				virtual void Deactivate() override;
				virtual Core::Unique<Component> Copy(Entity* New) const override;
				void Load(btCollisionShape* Shape, float Mass, float Anticipation = 0.0f);
				void Load(const std::string& Path, float Mass, float Anticipation = 0.0f, const std::function<void()>& Callback = nullptr);
				void Clear();
				void SetTransform(const Compute::Vector3& Position, const Compute::Vector3& Scale, const Compute::Vector3& Rotation);
				void SetTransform(bool Kinematic);
				void SetMass(float Mass);
				Compute::RigidBody* GetBody() const;

			private:
				void DeserializeBody(Core::Schema* Node);

			public:
				ED_COMPONENT("rigid-body");
			};

			class ED_OUT SliderConstraint final : public Component
			{
			private:
				Compute::SConstraint * Instance;
				Entity* Connection;

			public:
				SliderConstraint(Entity* Ref);
				virtual ~SliderConstraint() override;
				virtual void Deserialize(Core::Schema* Node) override;
				virtual void Serialize(Core::Schema* Node) override;
				virtual Core::Unique<Component> Copy(Entity* New) const override;
				void Load(Entity* Other, bool IsGhosted, bool IsLinear);
				void Clear();
				Compute::SConstraint* GetConstraint() const;
				Entity* GetConnection() const;

			public:
				ED_COMPONENT("slider-constraint");
			};

			class ED_OUT Acceleration final : public Component
			{
			private:
				Compute::RigidBody * RigidBody = nullptr;

			public:
				Compute::Vector3 AmplitudeVelocity;
				Compute::Vector3 AmplitudeTorque;
				Compute::Vector3 ConstantVelocity;
				Compute::Vector3 ConstantTorque;
				Compute::Vector3 ConstantCenter;
				bool Kinematic = true;

			public:
				Acceleration(Entity* Ref);
				virtual ~Acceleration() = default;
				virtual void Deserialize(Core::Schema * Node) override;
				virtual void Serialize(Core::Schema * Node) override;
				virtual void Activate(Component * New) override;
				virtual void Update(Core::Timer * Time) override;
				virtual Core::Unique<Component> Copy(Entity * New) const override;
				Compute::RigidBody* GetBody() const;

			public:
				ED_COMPONENT("acceleration");
			};

			class ED_OUT Model final : public Drawable
			{
			protected:
				Graphics::Model * Instance = nullptr;

			public:
				Compute::Vector2 TexCoord = 1.0f;

			public:
				Model(Entity* Ref);
				virtual ~Model() override;
				virtual void Deserialize(Core::Schema* Node) override;
				virtual void Serialize(Core::Schema* Node) override;
				virtual float GetVisibility(const Viewer& View, float Distance) const override;
				virtual size_t GetUnitBounds(Compute::Vector3& Min, Compute::Vector3& Max) const override;
				virtual Core::Unique<Component> Copy(Entity* New) const override;
				void SetDrawable(Core::Unique<Graphics::Model> Drawable);
				Graphics::Model* GetDrawable();

			public:
				ED_COMPONENT("model");
			};

			class ED_OUT Skin final : public Drawable
			{
			protected:
				Graphics::SkinModel * Instance = nullptr;

			public:
				Compute::Vector2 TexCoord = 1.0f;
				Graphics::PoseBuffer Skeleton;

			public:
				Skin(Entity* Ref);
				virtual ~Skin() override;
				virtual void Deserialize(Core::Schema* Node) override;
				virtual void Serialize(Core::Schema* Node) override;
				virtual void Synchronize(Core::Timer* Time) override;
				virtual float GetVisibility(const Viewer& View, float Distance) const override;
				virtual size_t GetUnitBounds(Compute::Vector3& Min, Compute::Vector3& Max) const override;
				virtual Core::Unique<Component> Copy(Entity* New) const override;
				void SetDrawable(Core::Unique<Graphics::SkinModel> Drawable);
				Graphics::SkinModel* GetDrawable();

			public:
				ED_COMPONENT("skin");
			};

			class ED_OUT Emitter final : public Drawable
			{
			protected:
				Graphics::InstanceBuffer * Instance = nullptr;

			public:
				Compute::Vector3 Min = 0.0f;
				Compute::Vector3 Max = 1.0f;
				bool Connected = false;
				bool QuadBased = false;

			public:
				Emitter(Entity* Ref);
				virtual ~Emitter() override;
				virtual void Deserialize(Core::Schema* Node) override;
				virtual void Serialize(Core::Schema* Node) override;
				virtual void Activate(Component* New) override;
				virtual size_t GetUnitBounds(Compute::Vector3& Min, Compute::Vector3& Max) const override;
				virtual Core::Unique<Component> Copy(Entity* New) const override;
				Graphics::InstanceBuffer* GetBuffer();

			public:
				ED_COMPONENT("emitter");
			};

			class ED_OUT Decal final : public Drawable
			{
			public:
				Compute::Vector2 TexCoord = 1.0f;

			public:
				Decal(Entity* Ref);
				virtual ~Decal() = default;
				virtual void Deserialize(Core::Schema * Node) override;
				virtual void Serialize(Core::Schema * Node) override;
				virtual float GetVisibility(const Viewer& View, float Distance) const override;
				virtual Core::Unique<Component> Copy(Entity * New) const override;

			public:
				ED_COMPONENT("decal");
			};

			class ED_OUT SkinAnimator final : public Component
			{
			private:
				Skin * Instance = nullptr;
				std::string Reference;

			public:
				std::vector<Compute::SkinAnimatorClip> Clips;
				Compute::SkinAnimatorKey Current;
				Compute::SkinAnimatorKey Bind;
				Compute::SkinAnimatorKey Default;
				AnimatorState State;

			public:
				SkinAnimator(Entity* Ref);
				virtual ~SkinAnimator() override;
				virtual void Deserialize(Core::Schema* Node) override;
				virtual void Serialize(Core::Schema* Node) override;
				virtual void Activate(Component* New) override;
				virtual void Animate(Core::Timer* Time) override;
				virtual Core::Unique<Component> Copy(Entity* New) const override;
				void LoadAnimation(const std::string& Path, const std::function<void(bool)>& Callback = nullptr);
				void GetPose(Compute::SkinAnimatorKey* Result);
				void ClearAnimation();
				void Play(int64_t Clip = -1, int64_t Frame = -1);
				void Pause();
				void Stop();
				bool IsExists(int64_t Clip);
				bool IsExists(int64_t Clip, int64_t Frame);
				Compute::SkinAnimatorKey* GetFrame(int64_t Clip, int64_t Frame);
				std::vector<Compute::SkinAnimatorKey>* GetClip(int64_t Clip);
				Skin* GetSkin() const;
				std::string GetPath();

			private:
				void BlendAnimation(int64_t Clip, int64_t Frame);
				bool IsPosed(int64_t Clip, int64_t Frame);

			public:
				ED_COMPONENT("skin-animator");
			};

			class ED_OUT KeyAnimator final : public Component
			{
			private:
				std::string Reference;

			public:
				std::vector<Compute::KeyAnimatorClip> Clips;
				Compute::AnimatorKey Current, Bind;
				AnimatorState State;

			public:
				KeyAnimator(Entity* Ref);
				virtual ~KeyAnimator() override;
				virtual void Deserialize(Core::Schema* Node) override;
				virtual void Serialize(Core::Schema* Node) override;
				virtual void Animate(Core::Timer* Time) override;
				virtual Core::Unique<Component> Copy(Entity* New) const override;
				void LoadAnimation(const std::string& Path, const std::function<void(bool)>& Callback = nullptr);
				void GetPose(Compute::AnimatorKey* Result);
				void ClearAnimation();
				void Play(int64_t Clip = -1, int64_t Frame = -1);
				void Pause();
				void Stop();
				bool IsExists(int64_t Clip);
				bool IsExists(int64_t Clip, int64_t Frame);
				Compute::AnimatorKey* GetFrame(int64_t Clip, int64_t Frame);
				std::vector<Compute::AnimatorKey>* GetClip(int64_t Clip);
				std::string GetPath();

			private:
				void BlendAnimation(int64_t Clip, int64_t Frame);
				bool IsPosed(int64_t Clip, int64_t Frame);

			public:
				ED_COMPONENT("key-animator");
			};

			class ED_OUT EmitterAnimator final : public Component
			{
			private:
				Emitter * Base = nullptr;

			public:
				Compute::Vector4 Diffuse;
				Compute::Vector3 Position;
				Compute::Vector3 Velocity;
				SpawnerProperties Spawner;
				float RotationSpeed = 0.0f;
				float ScaleSpeed = 0.0f;
				float Noise = 0.0f;
				bool Simulate = false;

			public:
				EmitterAnimator(Entity* Ref);
				virtual ~EmitterAnimator() = default;
				virtual void Deserialize(Core::Schema * Node) override;
				virtual void Serialize(Core::Schema * Node) override;
				virtual void Activate(Component * New) override;
				virtual void Animate(Core::Timer * Time) override;
				virtual Core::Unique<Component> Copy(Entity * New) const override;
				Emitter* GetEmitter() const;

			protected:
				void AccurateSynchronization(float Step);
				void FastSynchronization(float Step);

			public:
				ED_COMPONENT("emitter-animator");
			};

			class ED_OUT FreeLook final : public Component
			{
			private:
				Compute::Vector2 Position;

			public:
				Graphics::KeyMap Rotate;
				float Sensivity;

			public:
				FreeLook(Entity* Ref);
				virtual ~FreeLook() = default;
				virtual void Update(Core::Timer * Time) override;
				virtual Core::Unique<Component> Copy(Entity * New) const override;

			public:
				ED_COMPONENT("free-look");
			};

			class ED_OUT Fly final : public Component
			{
			private:
				Compute::Vector3 Velocity;

			public:
				struct
				{
					Compute::Vector3 Axis = Compute::Vector3(1.0f, 1.0f, -1.0f);
					float Faster = 320.3f;
					float Normal = 185.4f;
					float Slower = 32.6f;
					float Fading = 5.4f;
				} Moving;

			public:
				Graphics::KeyMap Forward = Graphics::KeyCode::W;
				Graphics::KeyMap Backward = Graphics::KeyCode::S;
				Graphics::KeyMap Right = Graphics::KeyCode::D;
				Graphics::KeyMap Left = Graphics::KeyCode::A;
				Graphics::KeyMap Up = Graphics::KeyCode::SPACE;
				Graphics::KeyMap Down = Graphics::KeyCode::Z;
				Graphics::KeyMap Fast = Graphics::KeyCode::LSHIFT;
				Graphics::KeyMap Slow = Graphics::KeyCode::LCTRL;

			public:
				Fly(Entity* Ref);
				virtual ~Fly() = default;
				virtual void Update(Core::Timer * Time) override;
				virtual Core::Unique<Component> Copy(Entity * New) const override;

			private:
				Compute::Vector3 GetSpeed(Graphics::Activity* Activity);

			public:
				ED_COMPONENT("fly");
			};

			class ED_OUT AudioSource final : public Component
			{
			private:
				Compute::Vector3 LastPosition;
				Audio::AudioSource* Source;
				Audio::AudioSync Sync;

			public:
				AudioSource(Entity* Ref);
				virtual ~AudioSource() override;
				virtual void Deserialize(Core::Schema* Node) override;
				virtual void Serialize(Core::Schema* Node) override;
				virtual void Synchronize(Core::Timer* Time) override;
				virtual Core::Unique<Component> Copy(Entity* New) const override;
				void ApplyPlayingPosition();
				Audio::AudioSource* GetSource() const;
				Audio::AudioSync& GetSync();

			public:
				ED_COMPONENT("audio-source");
			};

			class ED_OUT AudioListener final : public Component
			{
			private:
				Compute::Vector3 LastPosition;

			public:
				float Gain = 1.0f;

			public:
				AudioListener(Entity* Ref);
				virtual ~AudioListener() override;
				virtual void Deserialize(Core::Schema* Node) override;
				virtual void Serialize(Core::Schema* Node) override;
				virtual void Synchronize(Core::Timer* Time) override;
				virtual void Deactivate() override;
				virtual Core::Unique<Component> Copy(Entity* New) const override;

			public:
				ED_COMPONENT("audio-listener");
			};

			class ED_OUT PointLight final : public Component
			{
			public:
				struct
				{
					float Softness = 1.0f;
					float Distance = 100.0f;
					float Bias = 0.0f;
					uint32_t Iterations = 2;
					bool Enabled = false;
				} Shadow;

			private:
				Attenuation Size;

			public:
				CubicDepthMap* DepthMap = nullptr;
				Compute::Matrix4x4 View;
				Compute::Matrix4x4 Projection;
				Compute::Vector3 Diffuse = 1.0f;
				float Emission = 1.0f;
				float Disperse = 0.0f;

			public:
				PointLight(Entity* Ref);
				virtual ~PointLight() = default;
				virtual void Deserialize(Core::Schema * Node) override;
				virtual void Serialize(Core::Schema * Node) override;
				virtual void Message(const std::string& Name, Core::VariantArgs& Args) override;
				virtual size_t GetUnitBounds(Compute::Vector3& Min, Compute::Vector3& Max) const override;
				virtual float GetVisibility(const Viewer& View, float Distance) const override;
				virtual Core::Unique<Component> Copy(Entity * New) const override;
				void GenerateOrigin();
				void SetSize(const Attenuation& Value);
				const Attenuation& GetSize();

			public:
				ED_COMPONENT("point-light");
			};

			class ED_OUT SpotLight final : public Component
			{
			public:
				struct
				{
					float Softness = 1.0f;
					float Distance = 100.0f;
					float Bias = 0.0f;
					uint32_t Iterations = 2;
					bool Enabled = false;
				} Shadow;

			private:
				Attenuation Size;

			public:
				LinearDepthMap* DepthMap = nullptr;
				Compute::Matrix4x4 Projection;
				Compute::Matrix4x4 View;
				Compute::Vector3 Diffuse = 1.0f;
				float Emission = 1.0f;
				float Cutoff = 60.0f;
				float Disperse = 0.0f;

			public:
				SpotLight(Entity* Ref);
				virtual ~SpotLight() = default;
				virtual void Deserialize(Core::Schema * Node) override;
				virtual void Serialize(Core::Schema * Node) override;
				virtual void Message(const std::string& Name, Core::VariantArgs& Args) override;
				virtual void Synchronize(Core::Timer * Time) override;
				virtual size_t GetUnitBounds(Compute::Vector3& Min, Compute::Vector3& Max) const override;
				virtual float GetVisibility(const Viewer& View, float Distance) const override;
				virtual Core::Unique<Component> Copy(Entity * New) const override;
				void GenerateOrigin();
				void SetSize(const Attenuation& Value);
				const Attenuation& GetSize();

			public:
				ED_COMPONENT("spot-light");
			};

			class ED_OUT LineLight final : public Component
			{
			public:
				struct
				{
					Compute::Vector3 RlhEmission = Compute::Vector3(0.0000055f, 0.000013f, 0.0000224f);
					Compute::Vector3 MieEmission = 0.000021f;
					float RlhHeight = 8000.0f;
					float MieHeight = 1200.0f;
					float MieDirection = 0.94f;
					float InnerRadius = 6371000.0f;
					float OuterRadius = 6471000.0f;
					float Intensity = 7.0f;
				} Sky;

				struct
				{
					float Distance[6] = { 25.0f, 50.0f, 100.0f, 175.0f, 250.0f, 325.0f };
					float Softness = 1.0f;
					float Bias = 0.0f;
					float Near = 6.0f;
					float Far = 9.0f;
					uint32_t Iterations = 2;
					uint32_t Cascades = 3;
					bool Enabled = false;
				} Shadow;

			public:
				CascadedDepthMap* DepthMap = nullptr;
				Compute::Matrix4x4 Projection[6];
				Compute::Matrix4x4 View[6];
				Compute::Vector3 Diffuse = 1.0f;
				float Emission = 1.0f;

			public:
				LineLight(Entity* Ref);
				virtual ~LineLight() = default;
				virtual void Deserialize(Core::Schema * Node) override;
				virtual void Serialize(Core::Schema * Node) override;
				virtual void Message(const std::string& Name, Core::VariantArgs& Args) override;
				virtual Core::Unique<Component> Copy(Entity * New) const override;
				void GenerateOrigin();

			public:
				ED_COMPONENT("line-light");
			};

			class ED_OUT SurfaceLight final : public Component
			{
			private:
				Graphics::Texture2D * DiffuseMapX[2] = { nullptr };
				Graphics::Texture2D* DiffuseMapY[2] = { nullptr };
				Graphics::Texture2D* DiffuseMapZ[2] = { nullptr };
				Graphics::Texture2D* DiffuseMap = nullptr;
				Graphics::TextureCube* Probe = nullptr;

			private:
				Attenuation Size;

			public:
				Compute::Matrix4x4 View[6];
				Compute::Matrix4x4 Projection;
				Compute::Vector3 Offset = Compute::Vector3(1.0f, 1.0f, 1.0f);
				Compute::Vector3 Diffuse = 1.0f;
				Ticker Tick;
				float Emission = 1.0f;
				float Infinity = 0.0f;
				bool Parallax = false;
				bool Locked = false;
				bool StaticMask = false;

			public:
				SurfaceLight(Entity* Ref);
				virtual ~SurfaceLight() override;
				virtual void Deserialize(Core::Schema* Node) override;
				virtual void Serialize(Core::Schema* Node) override;
				virtual size_t GetUnitBounds(Compute::Vector3& Min, Compute::Vector3& Max) const override;
				virtual float GetVisibility(const Viewer& View, float Distance) const override;
				virtual Core::Unique<Component> Copy(Entity* New) const override;
				void SetProbeCache(Core::Unique<Graphics::TextureCube> NewCache);
				void SetSize(const Attenuation& Value);
				bool SetDiffuseMap(Graphics::Texture2D* Map);
				bool SetDiffuseMap(Graphics::Texture2D* const MapX[2], Graphics::Texture2D* const MapY[2], Graphics::Texture2D* const MapZ[2]);
				bool IsImageBased() const;
				const Attenuation& GetSize();
				Graphics::TextureCube* GetProbeCache() const;
				Graphics::Texture2D* GetDiffuseMapXP();
				Graphics::Texture2D* GetDiffuseMapXN();
				Graphics::Texture2D* GetDiffuseMapYP();
				Graphics::Texture2D* GetDiffuseMapYN();
				Graphics::Texture2D* GetDiffuseMapZP();
				Graphics::Texture2D* GetDiffuseMapZN();
				Graphics::Texture2D* GetDiffuseMap();

			public:
				ED_COMPONENT("surface-light");
			};

			class ED_OUT Illuminator final : public Component
			{
			public:
				Graphics::Texture3D* VoxelMap;
				Ticker Inside;
				Ticker Outside;
				float RayStep;
				float MaxSteps;
				float Distance;
				float Radiance;
				float Length;
				float Margin;
				float Offset;
				float Angle;
				float Occlusion;
				float Specular;
				float Bleeding;
				bool Regenerate;

			public:
				Illuminator(Entity* Ref);
				virtual ~Illuminator() = default;
				virtual void Deserialize(Core::Schema* Node) override;
				virtual void Serialize(Core::Schema* Node) override;
				virtual void Message(const std::string& Name, Core::VariantArgs& Args) override;
				virtual Core::Unique<Component> Copy(Entity* New) const override;

			public:
				ED_COMPONENT("illuminator");
			};

			class ED_OUT Camera final : public Component
			{
			public:
				enum class ProjectionMode
				{
					Perspective,
					Orthographic
				} Mode;

			protected:
				RenderSystem* Renderer = nullptr;
				Compute::Matrix4x4 Projection;
				Graphics::Viewport Viewport;
				Viewer View;

			public:
				float NearPlane = 0.1f;
				float FarPlane = 250.0f;
				float Width = -1;
				float Height = -1;
				float FieldOfView = 75.0f;

			public:
				Camera(Entity* Ref);
				virtual ~Camera() override;
				virtual void Deserialize(Core::Schema* Node) override;
				virtual void Serialize(Core::Schema* Node) override;
				virtual void Activate(Component* New) override;
				virtual void Synchronize(Core::Timer* Time) override;
				virtual Core::Unique<Component> Copy(Entity* New) const override;
				void GetViewer(Viewer* View);
				void ResizeBuffers();
				Viewer& GetViewer();
				RenderSystem* GetRenderer();
				Compute::Matrix4x4 GetProjection();
				Compute::Matrix4x4 GetViewProjection();
				Compute::Matrix4x4 GetView();
				Compute::Vector3 GetViewPosition();
				Compute::Frustum8C GetFrustum8C();
				Compute::Frustum6P GetFrustum6P();
				Compute::Ray GetScreenRay(const Compute::Vector2& Position);
				Compute::Ray GetCursorRay();
				float GetDistance(Entity* Other);
				float GetWidth();
				float GetHeight();
				float GetAspect();
				bool RayTest(const Compute::Ray& Ray, Entity* Other, Compute::Vector3* Hit = nullptr);
				bool RayTest(const Compute::Ray& Ray, const Compute::Matrix4x4& World, Compute::Vector3* Hit = nullptr);

			public:
				ED_COMPONENT("camera");
			};

			class ED_OUT Scriptable final : public Component
			{
			public:
				enum class SourceType
				{
					Resource,
					Memory
				};

				enum class InvokeType
				{
					Typeless,
					Normal
				};

			protected:
				struct
				{
					Script::VMCFunction* Serialize = nullptr;
					Script::VMCFunction* Deserialize = nullptr;
					Script::VMCFunction* Awake = nullptr;
					Script::VMCFunction* Asleep = nullptr;
					Script::VMCFunction* Synchronize = nullptr;
					Script::VMCFunction* Animate = nullptr;
					Script::VMCFunction* Update = nullptr;
					Script::VMCFunction* Message = nullptr;
				} Entry;

			protected:
				Script::VMCompiler* Compiler;
				std::string Resource;
				std::string Module;
				SourceType Source;
				InvokeType Invoke;

			public:
				Scriptable(Entity* Ref);
				virtual ~Scriptable() override;
				virtual void Deserialize(Core::Schema* Node) override;
				virtual void Serialize(Core::Schema* Node) override;
				virtual void Activate(Component* New) override;
				virtual void Deactivate() override;
				virtual void Update(Core::Timer* Time) override;
				virtual void Message(const std::string& Name, Core::VariantArgs& Args) override;
				virtual Core::Unique<Component> Copy(Entity* New) const override;
				Core::Promise<int> Call(const std::string& Name, unsigned int Args, Script::ArgsCallback&& OnArgs);
				Core::Promise<int> Call(Edge::Script::VMCFunction* Entry, Script::ArgsCallback&& OnArgs);
				Core::Promise<int> CallEntry(const std::string& Name);
				int LoadSource();
				int LoadSource(SourceType Type, const std::string& Source);
				void SetInvocation(InvokeType Type);
				void UnloadSource();
				Script::VMCompiler* GetCompiler();
				bool GetPropertyByName(const char* Name, Script::VMProperty* Result);
				bool GetPropertyByIndex(int Index, Script::VMProperty* Result);
				Script::VMFunction GetFunctionByName(const std::string& Name, unsigned int Args);
				Script::VMFunction GetFunctionByIndex(int Index, unsigned int Args);
				SourceType GetSourceType();
				InvokeType GetInvokeType();
				int GetPropertiesCount();
				int GetFunctionsCount();
				const std::string& GetSource();
				const std::string& GetModuleName();

			public:
				template <typename T>
				int SetTypePropertyByName(const char* Name, const T& Value)
				{
					ED_ASSERT(Name != nullptr, (int)Script::VMResult::INVALID_ARG, "name should be set");
					ED_ASSERT(Compiler != nullptr, (int)Script::VMResult::INVALID_ARG, "compiler should be set");

					auto* VM = Compiler->GetContext();
					if (VM->GetState() == Edge::Script::VMRuntime::ACTIVE)
						return (int)Script::VMResult::MODULE_IS_IN_USE;

					Script::VMModule Src = Compiler->GetModule();
					if (!Src.IsValid())
						return 0;

					int Index = Src.GetPropertyIndexByName(Name);
					if (Index < 0)
						return Index;

					T* Address = (T*)Src.GetAddressOfProperty(Index);
					if (!Address)
						return -1;

					*Address = Value;
					return 0;
				}
				template <typename T>
				int SetRefPropertyByName(const char* Name, T* Value)
				{
					ED_ASSERT(Name != nullptr, (int)Script::VMResult::INVALID_ARG, "name should be set");
					ED_ASSERT(Compiler != nullptr, (int)Script::VMResult::INVALID_ARG, "compiler should be set");

					auto* VM = Compiler->GetContext();
					if (VM->GetState() == Edge::Script::VMRuntime::ACTIVE)
						return (int)Script::VMResult::MODULE_IS_IN_USE;

					Script::VMModule Src = Compiler->GetModule();
					if (!Src.IsValid())
						return (int)Script::VMResult::INVALID_CONFIGURATION;

					int Index = Src.GetPropertyIndexByName(Name);
					if (Index < 0)
						return Index;

					T** Address = (T**)Src.GetAddressOfProperty(Index);
					if (!Address)
						return -1;

					if (*Address != nullptr)
						(*Address)->Release();

					*Address = Value;
					if (*Address != nullptr)
						(*Address)->AddRef();

					return 0;
				}
				template <typename T>
				int SetTypePropertyByIndex(int Index, const T& Value)
				{
					ED_ASSERT(Index >= 0, (int)Script::VMResult::INVALID_ARG, "index should be greater or equal to zero");
					ED_ASSERT(Compiler != nullptr, (int)Script::VMResult::INVALID_ARG, "compiler should be set");

					auto* VM = Compiler->GetContext();
					if (VM->GetState() == Edge::Script::VMRuntime::ACTIVE)
						return (int)Script::VMResult::MODULE_IS_IN_USE;

					Script::VMModule Src = Compiler->GetModule();
					if (!Src.IsValid())
						return 0;

					T* Address = (T*)Src.GetAddressOfProperty(Index);
					if (!Address)
						return -1;

					*Address = Value;
					return 0;
				}
				template <typename T>
				int SetRefPropertyByIndex(int Index, T* Value)
				{
					ED_ASSERT(Index >= 0, (int)Script::VMResult::INVALID_ARG, "index should be greater or equal to zero");
					ED_ASSERT(Compiler != nullptr, (int)Script::VMResult::INVALID_ARG, "compiler should be set");

					auto* VM = Compiler->GetContext();
					if (VM->GetState() == Edge::Script::VMRuntime::ACTIVE)
						return (int)Script::VMResult::MODULE_IS_IN_USE;

					Script::VMModule Src = Compiler->GetModule();
					if (!Src.IsValid())
						return (int)Script::VMResult::INVALID_CONFIGURATION;

					T** Address = (T**)Src.GetAddressOfProperty(Index);
					if (!Address)
						return -1;

					if (*Address != nullptr)
						(*Address)->Release();

					*Address = Value;
					if (*Address != nullptr)
						(*Address)->AddRef();

					return 0;
				}

			private:
				void Protect();
				void Unprotect();

			public:
				ED_COMPONENT("scriptable");
			};
		}
	}
}
#endif