#ifndef TH_ENGINE_COMPONENTS_H
#define TH_ENGINE_COMPONENTS_H

#include "../core/engine.h"

namespace Tomahawk
{
	namespace Engine
	{
		namespace Components
		{
			typedef std::function<void(Script::VMContext*)> InvocationCallback;

			class TH_OUT Model : public Drawable
			{
			protected:
				Graphics::Model* Instance = nullptr;

			public:
				Model(Entity* Ref);
				virtual ~Model() override;
				virtual void Deserialize(ContentManager* Content, Rest::Document* Node) override;
				virtual void Serialize(ContentManager* Content, Rest::Document* Node) override;
				virtual void Awake(Component* New) override;
				virtual void Asleep() override;
				virtual float Cull(const Viewer& View) override;
				virtual Component* Copy(Entity* New) override;
				virtual Compute::Matrix4x4 GetBoundingBox() override;
				void SetDrawable(Graphics::Model* Drawable);
				Graphics::Model* GetDrawable();

			public:
				TH_COMPONENT("model");
			};

			class TH_OUT Skin : public Drawable
			{
			protected:
				Graphics::SkinModel* Instance = nullptr;

			public:
				Graphics::PoseBuffer Skeleton;

			public:
				Skin(Entity* Ref);
				virtual ~Skin() override;
				virtual void Deserialize(ContentManager* Content, Rest::Document* Node) override;
				virtual void Serialize(ContentManager* Content, Rest::Document* Node) override;
				virtual void Synchronize(Rest::Timer* Time) override;
				virtual void Awake(Component* New) override;
				virtual void Asleep() override;
				virtual float Cull(const Viewer& View) override;
				virtual Component* Copy(Entity* New) override;
				virtual Compute::Matrix4x4 GetBoundingBox() override;
				void SetDrawable(Graphics::SkinModel* Drawable);
				Graphics::SkinModel* GetDrawable();

			public:
				TH_COMPONENT("skin");
			};

			class TH_OUT Emitter : public Drawable
			{
			protected:
				Graphics::InstanceBuffer* Instance;

			public:
				Compute::Vector3 Volume;
				bool Connected;
				bool QuadBased;

			public:
				Emitter(Entity* Ref);
				virtual ~Emitter() override;
				virtual void Deserialize(ContentManager* Content, Rest::Document* Node) override;
				virtual void Serialize(ContentManager* Content, Rest::Document* Node) override;
				virtual void Awake(Component* New) override;
				virtual void Asleep() override;
				virtual float Cull(const Viewer& View) override;
				virtual Component* Copy(Entity* New) override;
				Graphics::InstanceBuffer* GetBuffer();

			public:
				TH_COMPONENT("emitter");
			};

			class TH_OUT SoftBody : public Drawable
			{
			protected:
				Compute::UnmanagedShape* Hull;
				Compute::SoftBody* Instance;
				std::vector<Compute::Vertex> Vertices;
				std::vector<int> Indices;

			public:
				bool Kinematic;
				bool Manage;

			public:
				SoftBody(Entity* Ref);
				virtual ~SoftBody() override;
				virtual void Deserialize(ContentManager* Content, Rest::Document* Node) override;
				virtual void Serialize(ContentManager* Content, Rest::Document* Node) override;
				virtual void Synchronize(Rest::Timer* Time) override;
				virtual void Awake(Component* New) override;
				virtual void Asleep() override;
				virtual float Cull(const Viewer& View) override;
				virtual Component* Copy(Entity* New) override;
				void Create(Compute::UnmanagedShape* Shape, float Anticipation);
				void Create(ContentManager* Content, const std::string& Path, float Anticipation);
				void CreateEllipsoid(const Compute::SoftBody::Desc::CV::SEllipsoid& Shape, float Anticipation);
				void CreatePatch(const Compute::SoftBody::Desc::CV::SPatch& Shape, float Anticipation);
				void CreateRope(const Compute::SoftBody::Desc::CV::SRope& Shape, float Anticipation);
				void Fill(Graphics::GraphicsDevice* Device, Graphics::ElementBuffer* IndexBuffer, Graphics::ElementBuffer* VertexBuffer);
				void Clear();
				void SetTransform(const Compute::Vector3& Position, const Compute::Vector3& Scale, const Compute::Vector3& Rotation);
				void SetTransform(bool Kinematic);
				Compute::SoftBody* GetBody();
				std::vector<Compute::Vertex>& GetVertices();
				std::vector<int>& GetIndices();

			public:
				TH_COMPONENT("soft-body");
			};

			class TH_OUT Decal : public Drawable
			{
			public:
				Compute::Matrix4x4 Projection;
				Compute::Matrix4x4 View;
				float FieldOfView;
				float Distance;

			public:
				Decal(Entity* Ref);
				virtual ~Decal() = default;
				virtual void Deserialize(ContentManager* Content, Rest::Document* Node) override;
				virtual void Serialize(ContentManager* Content, Rest::Document* Node) override;
				virtual void Synchronize(Rest::Timer* Time) override;
				virtual void Awake(Component* New) override;
				virtual void Asleep() override;
				virtual float Cull(const Viewer& View) override;
				virtual Component* Copy(Entity* New) override;

			public:
				TH_COMPONENT("decal");
			};

			class TH_OUT SkinAnimator : public Component
			{
			private:
				Skin* Instance = nullptr;
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
				virtual void Deserialize(ContentManager* Content, Rest::Document* Node) override;
				virtual void Serialize(ContentManager* Content, Rest::Document* Node) override;
				virtual void Awake(Component* New) override;
				virtual void Synchronize(Rest::Timer* Time) override;
				virtual Component* Copy(Entity* New) override;
				bool GetAnimation(ContentManager* Content, const std::string& Path);
				void GetPose(Compute::SkinAnimatorKey* Result);
				void ClearAnimation();
				void Play(int64_t Clip = -1, int64_t Frame = -1);
				void Pause();
				void Stop();
				Compute::SkinAnimatorKey* GetFrame(int64_t Clip, int64_t Frame);
				std::vector<Compute::SkinAnimatorKey>* GetClip(int64_t Clip);
				Skin* GetSkin() const;
				std::string GetPath();

			private:
				void BlendAnimation(int64_t Clip, int64_t Frame);
				bool IsPosed(int64_t Clip, int64_t Frame);

			public:
				TH_COMPONENT("skin-animator");
			};

			class TH_OUT KeyAnimator : public Component
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
				virtual void Deserialize(ContentManager* Content, Rest::Document* Node) override;
				virtual void Serialize(ContentManager* Content, Rest::Document* Node) override;
				virtual void Synchronize(Rest::Timer* Time) override;
				virtual Component* Copy(Entity* New) override;
				bool GetAnimation(ContentManager* Content, const std::string& Path);
				void GetPose(Compute::AnimatorKey* Result);
				void ClearAnimation();
				void Play(int64_t Clip = -1, int64_t Frame = -1);
				void Pause();
				void Stop();
				Compute::AnimatorKey* GetFrame(int64_t Clip, int64_t Frame);
				std::vector<Compute::AnimatorKey>* GetClip(int64_t Clip);
				std::string GetPath();

			private:
				void BlendAnimation(int64_t Clip, int64_t Frame);
				bool IsPosed(int64_t Clip, int64_t Frame);

			public:
				TH_COMPONENT("key-animator");
			};

			class TH_OUT EmitterAnimator : public Component
			{
			private:
				Emitter* Base;

			public:
				Compute::Vector4 Diffuse;
				Compute::Vector3 Position;
				Compute::Vector3 Velocity;
				SpawnerProperties Spawner;
				float RotationSpeed;
				float ScaleSpeed;
				float Noisiness;
				bool Simulate;

			public:
				EmitterAnimator(Entity* Ref);
				virtual ~EmitterAnimator() = default;
				virtual void Deserialize(ContentManager* Content, Rest::Document* Node) override;
				virtual void Serialize(ContentManager* Content, Rest::Document* Node) override;
				virtual void Awake(Component* New) override;
				virtual void Synchronize(Rest::Timer* Time) override;
				virtual Component* Copy(Entity* New) override;
				Emitter* GetEmitter() const;

			protected:
				void AccurateSynchronization(float DeltaTime);
				void FastSynchronization(float DeltaTime);

			public:
				TH_COMPONENT("emitter-animator");
			};

			class TH_OUT RigidBody : public Component
			{
			private:
				Compute::UnmanagedShape* Hull;
				Compute::RigidBody* Instance;

			public:
				bool Kinematic;
				bool Manage;

			public:
				RigidBody(Entity* Ref);
				virtual ~RigidBody() override;
				virtual void Deserialize(ContentManager* Content, Rest::Document* Node) override;
				virtual void Serialize(ContentManager* Content, Rest::Document* Node) override;
				virtual void Synchronize(Rest::Timer* Time) override;
				virtual void Asleep() override;
				virtual Component* Copy(Entity* New) override;
				void Create(btCollisionShape* Shape, float Mass, float Anticipation);
				void Create(ContentManager* Content, const std::string& Path, float Mass, float Anticipation);
				void Clear();
				void SetTransform(const Compute::Vector3& Position, const Compute::Vector3& Scale, const Compute::Vector3& Rotation);
				void SetTransform(bool Kinematic);
				void SetMass(float Mass);
				Compute::RigidBody* GetBody() const;

			public:
				TH_COMPONENT("rigid-body");
			};

			class TH_OUT Acceleration : public Component
			{
			private:
				Compute::RigidBody* RigidBody;

			public:
				Compute::Vector3 AmplitudeVelocity;
				Compute::Vector3 AmplitudeTorque;
				Compute::Vector3 ConstantVelocity;
				Compute::Vector3 ConstantTorque;
				Compute::Vector3 ConstantCenter;
				bool Kinematic;

			public:
				Acceleration(Entity* Ref);
				virtual ~Acceleration() = default;
				virtual void Deserialize(ContentManager* Content, Rest::Document* Node) override;
				virtual void Serialize(ContentManager* Content, Rest::Document* Node) override;
				virtual void Awake(Component* New) override;
				virtual void Update(Rest::Timer* Time) override;
				virtual Component* Copy(Entity* New) override;
				Compute::RigidBody* GetBody() const;

			public:
				TH_COMPONENT("acceleration");
			};

			class TH_OUT SliderConstraint : public Component
			{
			private:
				struct
				{
					int64_t Connection;
					bool Ghost;
					bool Linear;
				} Wanted;

			private:
				Compute::SliderConstraint* Instance;
				Entity* Connection;

			public:
				SliderConstraint(Entity* Ref);
				virtual ~SliderConstraint() override;
				virtual void Deserialize(ContentManager* Content, Rest::Document* Node) override;
				virtual void Serialize(ContentManager* Content, Rest::Document* Node) override;
				virtual void Synchronize(Rest::Timer* Time) override;
				virtual Component* Copy(Entity* New) override;
				void Create(Entity* Other, bool IsGhosted, bool IsLinear);
				void Clear();
				Compute::SliderConstraint* GetConstraint() const;
				Entity* GetConnection() const;

			public:
				TH_COMPONENT("slider-constraint");
			};

			class TH_OUT FreeLook : public Component
			{
			private:
				Graphics::Activity* Activity;
				Compute::Vector2 Position;

			public:
				Graphics::KeyMap Rotate;
				float Sensivity;

			public:
				FreeLook(Entity* Ref);
				virtual ~FreeLook() = default;
				virtual void Awake(Component* New) override;
				virtual void Update(Rest::Timer* Time) override;
				virtual Component* Copy(Entity* New) override;
				Graphics::Activity* GetActivity() const;

			public:
				TH_COMPONENT("free-look");
			};

			class TH_OUT Fly : public Component
			{
			private:
				Graphics::Activity* Activity;

			public:
				Graphics::KeyMap Forward;
				Graphics::KeyMap Backward;
				Graphics::KeyMap Right;
				Graphics::KeyMap Left;
				Graphics::KeyMap Up;
				Graphics::KeyMap Down;
				Graphics::KeyMap Fast;
				Graphics::KeyMap Slow;
				Compute::Vector3 Axis;
				float SpeedNormal;
				float SpeedUp;
				float SpeedDown;

			public:
				Fly(Entity* Ref);
				virtual ~Fly() = default;
				virtual void Awake(Component* New) override;
				virtual void Update(Rest::Timer* Time) override;
				virtual Component* Copy(Entity* New) override;
				Graphics::Activity* GetActivity() const;

			public:
				TH_COMPONENT("fly");
			};

			class TH_OUT AudioSource : public Component
			{
			private:
				Compute::Vector3 LastPosition;
				Audio::AudioSource* Source;
				Audio::AudioSync Sync;

			public:
				AudioSource(Entity* Ref);
				virtual ~AudioSource() override;
				virtual void Deserialize(ContentManager* Content, Rest::Document* Node) override;
				virtual void Serialize(ContentManager* Content, Rest::Document* Node) override;
				virtual void Synchronize(Rest::Timer* Time) override;
				virtual Component* Copy(Entity* New) override;
				void ApplyPlayingPosition();
				Audio::AudioSource* GetSource() const;
				Audio::AudioSync& GetSync();

			public:
				TH_COMPONENT("audio-source");
			};

			class TH_OUT AudioListener : public Component
			{
			private:
				Compute::Vector3 LastPosition;

			public:
				float Gain;

			public:
				AudioListener(Entity* Ref);
				virtual ~AudioListener() override;
				virtual void Deserialize(ContentManager* Content, Rest::Document* Node) override;
				virtual void Serialize(ContentManager* Content, Rest::Document* Node) override;
				virtual void Synchronize(Rest::Timer* Time) override;
				virtual void Asleep() override;
				virtual Component* Copy(Entity* New) override;

			public:
				TH_COMPONENT("audio-listener");
			};

			class TH_OUT PointLight : public Cullable
			{
			private:
				Graphics::Texture2D* ShadowCache;

			public:
				Compute::Matrix4x4 View;
				Compute::Matrix4x4 Projection;
				Compute::Vector3 Diffuse;
				float Emission = 1.0f;
				float ShadowSoftness = 0.0f;
				float ShadowDistance = 10.0f;
				float ShadowBias = 0.0f;
				int ShadowIterations = 2;
				bool Shadowed = false;

			public:
				PointLight(Entity* Ref);
				virtual ~PointLight() = default;
				virtual void Deserialize(ContentManager* Content, Rest::Document* Node) override;
				virtual void Serialize(ContentManager* Content, Rest::Document* Node) override;
				virtual void Synchronize(Rest::Timer* Time) override;
				virtual float Cull(const Viewer& View) override;
				virtual Component* Copy(Entity* New) override;
				void SetShadowCache(Graphics::Texture2D* NewCache);
				Graphics::Texture2D* GetShadowCache() const;

			public:
				TH_COMPONENT("point-light");
			};

			class TH_OUT SpotLight : public Cullable
			{
			private:
				Graphics::Texture2D* ShadowCache;
				Graphics::Texture2D* ProjectMap;

			public:
				Compute::Matrix4x4 Projection;
				Compute::Matrix4x4 View;
				Compute::Vector3 Diffuse;
				float ShadowSoftness;
				float ShadowDistance;
				float ShadowBias;
				float FieldOfView;
				float Emission;
				int ShadowIterations;
				bool Shadowed;

			public:
				SpotLight(Entity* Ref);
				virtual ~SpotLight() override;
				virtual void Deserialize(ContentManager* Content, Rest::Document* Node) override;
				virtual void Serialize(ContentManager* Content, Rest::Document* Node) override;
				virtual void Synchronize(Rest::Timer* Time) override;
				virtual float Cull(const Viewer& View) override;
				virtual Component* Copy(Entity* New) override;
				void SetShadowCache(Graphics::Texture2D* NewCache);
				Graphics::Texture2D* GetShadowCache() const;
				void SetProjectMap(Graphics::Texture2D* NewCache);
				Graphics::Texture2D* GetProjectMap() const;

			public:
				TH_COMPONENT("spot-light");
			};

			class TH_OUT LineLight : public Component
			{
			private:
				Graphics::Texture2D* ShadowCache;

			public:
				Compute::Matrix4x4 Projection;
				Compute::Matrix4x4 View;
				Compute::Vector3 Diffuse;
				Compute::Vector3 RlhEmission;
				Compute::Vector3 MieEmission;
				float RlhHeight;
				float MieHeight;
				float ScatterIntensity;
				float PlanetRadius;
				float AtmosphereRadius;
				float MieDirection;
				float ShadowDistance;
				float ShadowSoftness;
				float ShadowBias;
				float ShadowFarBias;
				float ShadowLength;
				float ShadowHeight;
				float Emission;
				int ShadowIterations;
				bool Shadowed;

			public:
				LineLight(Entity* Ref);
				virtual ~LineLight() = default;
				virtual void Deserialize(ContentManager* Content, Rest::Document* Node) override;
				virtual void Serialize(ContentManager* Content, Rest::Document* Node) override;
				virtual void Synchronize(Rest::Timer* Time) override;
				virtual Component* Copy(Entity* New) override;
				void SetShadowCache(Graphics::Texture2D* NewCache);
				Graphics::Texture2D* GetShadowCache() const;

			public:
				TH_COMPONENT("line-light");
			};

			class TH_OUT ReflectionProbe : public Cullable
			{
			private:
				Graphics::Texture2D* DiffuseMapX[2];
				Graphics::Texture2D* DiffuseMapY[2];
				Graphics::Texture2D* DiffuseMapZ[2];
				Graphics::Texture2D* DiffuseMap;
				Graphics::TextureCube* ProbeCache;

			public:
				Compute::Matrix4x4 View[6];
				Compute::Matrix4x4 Projection;
				Compute::Vector3 Diffuse;
				Compute::Vector3 ViewOffset;
				Rest::TickTimer Rebuild;
				float CaptureRange;
				float Emission;
				float Infinity;
				bool ParallaxCorrected;
				bool RenderLocked;
				bool StaticMask;

			public:
				ReflectionProbe(Entity* Ref);
				virtual ~ReflectionProbe() override;
				virtual void Deserialize(ContentManager* Content, Rest::Document* Node) override;
				virtual void Serialize(ContentManager* Content, Rest::Document* Node) override;
				virtual float Cull(const Viewer& View) override;
				virtual Component* Copy(Entity* New) override;
				void SetProbeCache(Graphics::TextureCube* NewCache);
				bool SetDiffuseMap(Graphics::Texture2D* Map);
				bool SetDiffuseMap(Graphics::Texture2D* MapX[2], Graphics::Texture2D* MapY[2], Graphics::Texture2D* MapZ[2]);
				bool IsImageBased() const;
				Graphics::TextureCube* GetProbeCache() const;
				Graphics::Texture2D* GetDiffuseMapXP();
				Graphics::Texture2D* GetDiffuseMapXN();
				Graphics::Texture2D* GetDiffuseMapYP();
				Graphics::Texture2D* GetDiffuseMapYN();
				Graphics::Texture2D* GetDiffuseMapZP();
				Graphics::Texture2D* GetDiffuseMapZN();
				Graphics::Texture2D* GetDiffuseMap();

			public:
				TH_COMPONENT("reflection-probe");
			};

			class TH_OUT Camera : public Component
			{
			public:
				enum ProjectionMode
				{
					ProjectionMode_Perspective,
					ProjectionMode_Orthographic
				} Mode;

			protected:
				RenderSystem* Renderer = nullptr;
				Compute::Matrix4x4 Projection;
				Graphics::Viewport Viewport;
				Viewer FieldView;

			public:
				float NearPlane, FarPlane;
				float Width, Height;
				float FieldOfView;

			public:
				Camera(Entity* Ref);
				virtual ~Camera() override;
				virtual void Deserialize(ContentManager* Content, Rest::Document* Node) override;
				virtual void Serialize(ContentManager* Content, Rest::Document* Node) override;
				virtual void Awake(Component* New) override;
				virtual void Synchronize(Rest::Timer* Time) override;
				virtual void Asleep() override;
				virtual Component* Copy(Entity* New) override;
				void GetViewer(Viewer* View);
				void ResizeBuffers();
				Viewer GetViewer();
				RenderSystem* GetRenderer();
				Compute::Matrix4x4 GetProjection();
				Compute::Matrix4x4 GetViewProjection();
				Compute::Matrix4x4 GetView();
				Compute::Vector3 GetViewPosition();
				Compute::Ray GetScreenRay(const Compute::Vector2& Position);
				float GetDistance(Entity* Other);
				bool RayTest(Compute::Ray& Ray, Entity* Other);
				bool RayTest(Compute::Ray& Ray, const Compute::Matrix4x4& World);

			public:
				TH_COMPONENT("camera");
			};

			class TH_OUT Scriptable : public Component
			{
			public:
				enum SourceType
				{
					SourceType_Resource,
					SourceType_Memory
				};

				enum InvokeType
				{
					InvokeType_Typeless,
					InvokeType_Normal
				};

			protected:
				struct
				{
					Script::VMCFunction* Serialize = nullptr;
					Script::VMCFunction* Deserialize = nullptr;
					Script::VMCFunction* Awake = nullptr;
					Script::VMCFunction* Asleep = nullptr;
					Script::VMCFunction* Synchronize = nullptr;
					Script::VMCFunction* Update = nullptr;
					Script::VMCFunction* Message = nullptr;
				} Entry;

			protected:
				Script::VMCompiler* Compiler;
				std::string Resource;
				std::string Module;
				std::mutex Safe;
				SourceType Source;
				InvokeType Invoke;

			public:
				Scriptable(Entity* Ref);
				virtual ~Scriptable() override;
				virtual void Deserialize(ContentManager* Content, Rest::Document* Node) override;
				virtual void Serialize(ContentManager* Content, Rest::Document* Node) override;
				virtual void Awake(Component* New) override;
				virtual void Synchronize(Rest::Timer* Time) override;
				virtual void Asleep() override;
				virtual void Update(Rest::Timer* Time) override;
				virtual void Message(Event* Value) override;
				virtual Component* Copy(Entity* New) override;
				int Call(const std::string& Name, unsigned int Args, const InvocationCallback& ArgCallback);
				int Call(Tomahawk::Script::VMCFunction* Entry, const InvocationCallback& ArgCallback);
				int CallEntry(const std::string& Name);
				int SetSource();
				int SetSource(SourceType Type, const std::string& Source);
				void SetInvocation(InvokeType Type);
				void UnsetSource();
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
					if (!Name || !Compiler)
						return Script::VMResult_INVALID_ARG;

					auto* VM = Compiler->GetContext();
					if (VM->GetState() == Tomahawk::Script::VMExecState_ACTIVE)
						return Script::VMResult_MODULE_IS_IN_USE;

					Safe.lock();
					Script::VMModule Module = Compiler->GetModule();
					if (!Module.IsValid())
					{
						Safe.unlock();
						return 0;
					}

					int Index = Module.GetPropertyIndexByName(Name);
					if (Index < 0)
					{
						Safe.unlock();
						return Index;
					}

					T* Address = (T*)Module.GetAddressOfProperty(Index);
					if (!Address)
					{
						Safe.unlock();
						return -1;
					}

					*Address = Value;
					Safe.unlock();

					return 0;
				}
				template <typename T>
				int SetRefPropertyByName(const char* Name, T* Value)
				{
					if (!Name || !Compiler)
						return Script::VMResult_INVALID_ARG;

					auto* VM = Compiler->GetContext();
					if (VM->GetState() == Tomahawk::Script::VMExecState_ACTIVE)
						return Script::VMResult_MODULE_IS_IN_USE;

					Safe.lock();
					Script::VMModule Module = Compiler->GetModule();
					if (!Module.IsValid())
					{
						Safe.unlock();
						return Script::VMResult_INVALID_CONFIGURATION;
					}

					int Index = Module.GetPropertyIndexByName(Name);
					if (Index < 0)
					{
						Safe.unlock();
						return Index;
					}

					T** Address = (T**)Module.GetAddressOfProperty(Index);
					if (!Address)
					{
						Safe.unlock();
						return -1;
					}

					if (*Address != nullptr)
						(*Address)->Release();

					*Address = Value;
					if (*Address != nullptr)
						(*Address)->AddRef();

					Safe.unlock();
					return 0;
				}
				template <typename T>
				int SetTypePropertyByIndex(int Index, const T& Value)
				{
					if (Index < 0 || !Compiler)
						return Script::VMResult_INVALID_ARG;

					auto* VM = Compiler->GetContext();
					if (VM->GetState() == Tomahawk::Script::VMExecState_ACTIVE)
						return Script::VMResult_MODULE_IS_IN_USE;

					Safe.lock();
					Script::VMModule Module = Compiler->GetModule();
					if (!Module.IsValid())
					{
						Safe.unlock();
						return 0;
					}

					T* Address = (T*)Module.GetAddressOfProperty(Index);
					if (!Address)
					{
						Safe.unlock();
						return -1;
					}

					*Address = Value;
					Safe.unlock();

					return 0;
				}
				template <typename T>
				int SetRefPropertyByIndex(int Index, T* Value)
				{
					if (Index < 0 || !Compiler)
						return Script::VMResult_INVALID_ARG;

					auto* VM = Compiler->GetContext();
					if (VM->GetState() == Tomahawk::Script::VMExecState_ACTIVE)
						return Script::VMResult_MODULE_IS_IN_USE;

					Safe.lock();
					Script::VMModule Module = Compiler->GetModule();
					if (!Module.IsValid())
					{
						Safe.unlock();
						return Script::VMResult_INVALID_CONFIGURATION;
					}

					T** Address = (T**)Module.GetAddressOfProperty(Index);
					if (!Address)
					{
						Safe.unlock();
						return -1;
					}

					if (*Address != nullptr)
						(*Address)->Release();

					*Address = Value;
					if (*Address != nullptr)
						(*Address)->AddRef();

					Safe.unlock();
					return 0;
				}

			public:
				TH_COMPONENT("scriptable");
			};
		}
	}
}
#endif