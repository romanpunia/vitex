#ifndef THAWK_ENGINE_COMPONENTS_H
#define THAWK_ENGINE_COMPONENTS_H

#include "../engine.h"

namespace Tomahawk
{
    namespace Engine
    {
        namespace Components
        {
			class SkinnedModel;

            class THAWK_OUT RigidBody : public Component
            {
            public:
                Compute::RigidBody* Instance;
                bool Kinematic, Synchronize;

            public:
                RigidBody(Entity* Ref);
                virtual	~RigidBody() override;
                virtual void OnLoad(ContentManager* Content, Rest::Document* Node) override;
                virtual void OnSave(ContentManager* Content, Rest::Document* Node) override;
                virtual void OnAwake(Component* New) override;
                virtual void OnSynchronize(Rest::Timer* Time) override;
                virtual void OnAsleep() override;
                virtual Component* OnClone() override;
                void Activate(btCollisionShape* Shape, float Mass, float Anticipation);
                void ResynchronizeFrom(Compute::Matrix4x4 World);
                void Resynchronize(bool Kinematically);
                void Reweight(float Mass);

            public:
                THAWK_COMPONENT(ComponentId_Rigidbody, RigidBody);
            };

            class THAWK_OUT Acceleration : public Component
            {
            public:
                Compute::Vector3 AmplitudeVelocity;
                Compute::Vector3 AmplitudeTorque;
                Compute::Vector3 ConstantVelocity;
                Compute::Vector3 ConstantTorque;
                Compute::Vector3 ConstantCenter;
                Compute::RigidBody* RigidBody;
                bool Velocity;

            public:
                Acceleration(Entity* Ref);
                virtual	~Acceleration() = default;
                virtual void OnLoad(ContentManager* Content, Rest::Document* Node) override;
                virtual void OnSave(ContentManager* Content, Rest::Document* Node) override;
                virtual void OnAwake(Component* New) override;
                virtual void OnSynchronize(Rest::Timer* Time) override;
                virtual Component* OnClone() override;

            public:
                THAWK_COMPONENT(ComponentId_Acceleration, Acceleration);
            };

            class THAWK_OUT SliderConstraint : public Component
            {
            public:
                Compute::SliderConstraint* Constraint;
                Entity* Connection;

            public:
                SliderConstraint(Entity* Ref);
                virtual	~SliderConstraint() override;
                virtual void OnLoad(ContentManager* Content, Rest::Document* Node) override;
                virtual void OnSave(ContentManager* Content, Rest::Document* Node) override;
                virtual void OnAwake(Component* New) override;
                virtual Component* OnClone() override;
                void Activate(bool IsGhosted, bool IsLinearPowered);

            public:
                THAWK_COMPONENT(ComponentId_Slider_Constraint, SliderConstraint);
            };

            class THAWK_OUT AudioSource : public Component
            {
            public:
                Audio::AudioSource* Source;
                Compute::Vector3 Direction;
                Compute::Vector3 Velocity;
                float ConeInnerAngle;
                float ConeOuterAngle;
                float ConeOuterGain;
                float Pitch, Gain;
                float RefDistance;
                float Distance;
                float Rolloff;
                float Position;
                int Relative, Loop;

            public:
                AudioSource(Entity* Ref);
                virtual	~AudioSource() override;
                virtual void OnLoad(ContentManager* Content, Rest::Document* Node) override;
                virtual void OnSave(ContentManager* Content, Rest::Document* Node) override;
                virtual void OnSynchronize(Rest::Timer* Time) override;
                virtual Component* OnClone() override;
                void ApplyPlayingPosition();

            public:
                THAWK_COMPONENT(ComponentId_Audio_Source, AudioSource);
            };

            class THAWK_OUT AudioListener : public Component
            {
            public:
                Compute::Vector3 Velocity;
                float Gain;

            public:
                AudioListener(Entity* Ref);
                virtual	~AudioListener() override;
                virtual void OnLoad(ContentManager* Content, Rest::Document* Node) override;
                virtual void OnSave(ContentManager* Content, Rest::Document* Node) override;
                virtual void OnSynchronize(Rest::Timer* Time) override;
                virtual void OnAsleep() override;
                virtual Component* OnClone() override;

            public:
                THAWK_COMPONENT(ComponentId_Audio_Listener, AudioListener);
            };

            class THAWK_OUT SkinAnimator : public Component
            {
			public:
				SkinnedModel* Instance = nullptr;

            public:
                std::vector<Compute::SkinAnimatorClip> Clips;
				std::vector<Compute::AnimatorKey> Current;
				std::vector<Compute::AnimatorKey> Bind;
				std::vector<Compute::AnimatorKey> Default;
                AnimatorState State;

            public:
                SkinAnimator(Entity* Ref);
                virtual	~SkinAnimator() override;
                virtual void OnLoad(ContentManager* Content, Rest::Document* Node) override;
                virtual void OnSave(ContentManager* Content, Rest::Document* Node) override;
                virtual void OnAwake(Component* New) override;
                virtual void OnSynchronize(Rest::Timer* Time) override;
                virtual Component* OnClone() override;
                void Play(Int64 Clip = -1, Int64 Frame = -1);
                void Pause();
                void Stop();
                std::vector<Compute::AnimatorKey>* GetFrame(Int64 Clip, Int64 Frame);
                std::vector<std::vector<Compute::AnimatorKey>>* GetClip(Int64 Clip);

			private:
				void BlendAnimation(Int64 Clip, Int64 Frame);
				void SavePose();
				bool IsPosed(Int64 Clip, Int64 Frame);

            public:
                THAWK_COMPONENT(ComponentId_Skin_Animator, SkinAnimator);
            };

            class THAWK_OUT KeyAnimator : public Component
            {
            public:
				std::vector<Compute::KeyAnimatorClip> Clips;
				Compute::AnimatorKey Current, Bind;
                AnimatorState State;

            public:
                KeyAnimator(Entity* Ref);
                virtual	~KeyAnimator() override;
                virtual void OnLoad(ContentManager* Content, Rest::Document* Node) override;
                virtual void OnSave(ContentManager* Content, Rest::Document* Node) override;
                virtual void OnSynchronize(Rest::Timer* Time) override;
                virtual Component* OnClone() override;
                void Play(Int64 Clip = -1, Int64 Frame = -1);
                void Pause();
                void Stop();
                Compute::AnimatorKey* GetFrame(Int64 Clip, Int64 Frame);
                std::vector<Compute::AnimatorKey>* GetClip(Int64 Clip);

			private:
				void BlendAnimation(Int64 Clip, Int64 Frame);
				void SavePose();
				bool IsPosed(Int64 Clip, Int64 Frame);

            public:
                THAWK_COMPONENT(ComponentId_Key_Animator, KeyAnimator);
            };

            class THAWK_OUT ElementSystem : public Component
            {
            public:
                Compute::Vector3 Diffusion;
                Compute::Vector2 TexCoord;
                UInt64 Material;
                float Visibility;
                float Volume;
                bool StrongConnection;
                bool QuadBased;

            public:
                Graphics::InstanceBuffer* Instance;
                Graphics::Texture2D* Diffuse;

            public:
                ElementSystem(Entity* Ref);
                virtual	~ElementSystem();
                virtual void OnAwake(Component* New) override;
                virtual void OnLoad(ContentManager* Content, Rest::Document* Node) override;
                virtual void OnSave(ContentManager* Content, Rest::Document* Node) override;
                virtual void OnSynchronize(Rest::Timer* Time) override;
                virtual void OnEvent(Event* Value) override;
                virtual Component* OnClone() override;
                Graphics::Material& GetMaterial();

            public:
                THAWK_COMPONENT(ComponentId_Element_System, ElementSystem);
            };

            class THAWK_OUT ElementAnimator : public Component
            {
            protected:
                ElementSystem * System;

            public:
                Compute::Vector4 Diffuse;
                Compute::Vector3 Position;
                Compute::Vector3 Velocity;
                SpawnerProperties Spawner;
                float DiffusionIntensity;
                float RotationSpeed;
                float ScaleSpeed;
                float Noisiness;
                bool Simulate;

            public:
                ElementAnimator(Entity* Ref);
                virtual	~ElementAnimator() = default;
                virtual void OnLoad(ContentManager* Content, Rest::Document* Node) override;
                virtual void OnSave(ContentManager* Content, Rest::Document* Node) override;
                virtual void OnAwake(Component* New) override;
                virtual void OnSynchronize(Rest::Timer* Time) override;
                virtual Component* OnClone() override;

            protected:
                void AccurateSynchronization(float DeltaTime);
                void FastSynchronization(float DeltaTime);

            public:
                THAWK_COMPONENT(ComponentId_Element_Animator, ElementAnimator);
            };

            class THAWK_OUT FreeLook : public Component
            {
            private:
                Graphics::Activity* Activity;
                Compute::Vector2 Position;

            public:
                Graphics::KeyMap Rotate;
                float Sensitivity;

            public:
                FreeLook(Entity* Ref);
                virtual	~FreeLook() = default;
                virtual void OnAwake(Component* New) override;
                virtual void OnUpdate(Rest::Timer* Time) override;

            public:
                THAWK_COMPONENT(ComponentId_Free_Look, FreeLook);
            };

            class THAWK_OUT Fly : public Component
            {
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

            private:
                Graphics::Activity* Activity;

            public:
                Fly(Entity* Ref);
                virtual	~Fly() = default;
                virtual void OnAwake(Component* New) override;
                virtual void OnUpdate(Rest::Timer* Time) override;

            public:
                THAWK_COMPONENT(ComponentId_Fly, Fly);
            };

            class THAWK_OUT Model : public Component
            {
            public:
				std::unordered_map<Graphics::Mesh*, TSurface> Surfaces;
				Graphics::Model* Instance = nullptr;
				bool Visibility;

            public:
				Model(Entity* Ref);
                virtual	~Model() = default;
                virtual void OnLoad(ContentManager* Content, Rest::Document* Node) override;
                virtual void OnSave(ContentManager* Content, Rest::Document* Node) override;
                virtual void OnSynchronize(Rest::Timer* Time) override;
                virtual void OnEvent(Event* Value) override;
                virtual Component* OnClone() override;
                Graphics::Material& GetMaterial(Graphics::Mesh* Mesh);
				Graphics::Material& GetMaterial(TSurface* Surface);
				TSurface* GetSurface(Graphics::Mesh* Mesh);
				Compute::Matrix4x4 GetBoundingBox();

            public:
                THAWK_COMPONENT(ComponentId_Model, Model);
            };

            class THAWK_OUT SkinnedModel : public Component
            {
			public:
				std::unordered_map<Graphics::SkinnedMesh*, TSurface> Surfaces;
				Graphics::SkinnedModel* Instance = nullptr;
				Graphics::PoseBuffer Skeleton;
				bool Visibility;

            public:
				SkinnedModel(Entity* Ref);
                virtual	~SkinnedModel() = default;
                virtual void OnLoad(ContentManager* Content, Rest::Document* Node) override;
                virtual void OnSave(ContentManager* Content, Rest::Document* Node) override;
                virtual void OnSynchronize(Rest::Timer* Time) override;
                virtual void OnEvent(Event* Value) override;
                virtual Component* OnClone() override;
				Graphics::Material& GetMaterial(Graphics::SkinnedMesh* Mesh);
				Graphics::Material& GetMaterial(TSurface* Surface);
				TSurface* GetSurface(Graphics::SkinnedMesh* Mesh);
				Compute::Matrix4x4 GetBoundingBox();

            public:
                THAWK_COMPONENT(ComponentId_Skinned_Model, SkinnedModel);
            };

            class THAWK_OUT PointLight : public Component
            {
            public:
                Compute::Matrix4x4 View;
                Compute::Matrix4x4 Projection;
                Compute::Vector3 Diffusion;
                float Visibility = 0.0f;
                float Emission = 1.0f;
                float Range = 5.0f;
                float ShadowSoftness = 0.0f;
                float ShadowDistance = 10.0f;
                float ShadowBias = 0.0f;
                int ShadowIterations = 2;
                bool Shadowed = false;

            public:
                void* Occlusion = nullptr;

            public:
                PointLight(Entity* Ref);
                virtual	~PointLight() = default;
                virtual void OnLoad(ContentManager* Content, Rest::Document* Node) override;
                virtual void OnSave(ContentManager* Content, Rest::Document* Node) override;
                virtual void OnSynchronize(Rest::Timer* Time) override;
                virtual Component* OnClone() override;

            public:
                THAWK_COMPONENT(ComponentId_Point_Light, PointLight);
            };

            class THAWK_OUT SpotLight : public Component
            {
            public:
                Compute::Matrix4x4 Projection;
                Compute::Matrix4x4 View;
                Compute::Vector3 Diffusion;
                float ShadowSoftness;
                float ShadowDistance;
                float ShadowBias;
                float FieldOfView;
                float Emission;
                float Range;
                float Visibility;
                int ShadowIterations;
                bool Shadowed;

            public:
                Graphics::Texture2D* Diffuse;
                void* Occlusion;

            public:
                SpotLight(Entity* Ref);
                virtual	~SpotLight() = default;
                virtual void OnLoad(ContentManager* Content, Rest::Document* Node) override;
                virtual void OnSave(ContentManager* Content, Rest::Document* Node) override;
                virtual void OnSynchronize(Rest::Timer* Time) override;
                virtual Component* OnClone() override;

            public:
                THAWK_COMPONENT(ComponentId_Spot_Light, SpotLight);
            };

            class THAWK_OUT LineLight : public Component
            {
            public:
                Compute::Matrix4x4 Projection;
                Compute::Matrix4x4 View;
                Compute::Vector3 Diffusion;
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
                void* Occlusion;

            public:
                LineLight(Entity* Ref);
                virtual	~LineLight() = default;
                virtual void OnLoad(ContentManager* Content, Rest::Document* Node) override;
                virtual void OnSave(ContentManager* Content, Rest::Document* Node) override;
                virtual void OnSynchronize(Rest::Timer* Time) override;
                virtual Component* OnClone() override;

            public:
                THAWK_COMPONENT(ComponentId_Line_Light, LineLight);
            };

            class THAWK_OUT ProbeLight : public Component
            {
            public:
                Compute::Matrix4x4 View[6];
                Compute::Matrix4x4 Projection;
                Compute::Vector3 Diffusion;
                Compute::Vector3 ViewOffset;
                float CaptureRange;
                float Emission;
                float Range;
                float Visibility;
                bool ParallaxCorrected;
                bool ImageBased;
                bool RenderLocked;

            public:
                Graphics::Texture2D* DiffusePX;
				Graphics::Texture2D* DiffuseNX;
				Graphics::Texture2D* DiffusePY;
				Graphics::Texture2D* DiffuseNY;
				Graphics::Texture2D* DiffusePZ;
				Graphics::Texture2D* DiffuseNZ;
				Graphics::TextureCube* Diffuse;
                Rest::TickTimer Rebuild;

            public:
                ProbeLight(Entity* Ref);
                virtual	~ProbeLight() override;
                virtual void OnLoad(ContentManager* Content, Rest::Document* Node) override;
                virtual void OnSave(ContentManager* Content, Rest::Document* Node) override;
                virtual void OnSynchronize(Rest::Timer* Time) override;
                virtual Component* OnClone() override;
				bool RebuildDiffuseMap();

            public:
                THAWK_COMPONENT(ComponentId_Probe_Light, ProbeLight);
            };

            class THAWK_OUT Camera : public Component
            {
            protected:
                Viewer FieldView;

            public:
                RenderSystem* Renderer = nullptr;
                Compute::Matrix4x4 Projection;
                float ViewDistance;

            public:
                Camera(Entity* Ref);
                virtual	~Camera() override;
                virtual void OnLoad(ContentManager* Content, Rest::Document* Node) override;
                virtual void OnSave(ContentManager* Content, Rest::Document* Node) override;
                virtual void OnAwake(Component* New) override;
                virtual void OnAsleep() override;
                virtual Component* OnClone() override;
                void FillViewer(Viewer* View);
                void ResizeBuffers();
                Viewer GetViewer();
                Compute::Matrix4x4 GetViewProjection();
                Compute::Matrix4x4 GetView();
                Compute::Vector3 GetViewPosition();
                float GetFieldOfView();

            public:
                THAWK_COMPONENT(ComponentId_Camera, Camera);
            };
        }
    }
}
#endif