#include "components.h"
#include <btBulletDynamicsCommon.h>

namespace Tomahawk
{
    namespace Engine
    {
        namespace Components
        {
            RigidBody::RigidBody(Entity* Ref) : Component(Ref)
            {
                Instance = nullptr;
                Kinematic = false;
                Synchronize = true;
            }
            RigidBody::~RigidBody()
            {
				delete Instance;
            }
            void RigidBody::OnLoad(ContentManager* Content, Rest::Document* Node)
            {
				bool Extended;
				NMake::Unpack(Node->Find("extended"), &Extended);
				NMake::Unpack(Node->Find("kinematic"), &Kinematic);
				if (!Extended)
					return;

				btCollisionShape* Shape = nullptr; UInt64 Type;
				if (NMake::Unpack(Node->Find("shape"), &Type))
				{
					Shape = Compute::CollisionShape::Auto((Compute::Shape)Type);
					if (Shape == nullptr && Type == (UInt64)Compute::Shape_Convex_Hull)
					{
						std::vector<btVector3> Vertices;
						if (NMake::Unpack(Node->Find("shape-data"), &Vertices))
						{
							btConvexHullShape* Hull = new btConvexHullShape();
							for (auto&& Point : Vertices)
								Hull->addPoint(Point, false);

							Hull->recalcLocalAabb();
							Hull->optimizeConvexHull();
						}
					}
				}

				float Mass = 0, CcdMotionThreshold = 0;
				NMake::Unpack(Node->Find("mass"), &Mass);
				NMake::Unpack(Node->Find("ccd-motion-threshold"), &CcdMotionThreshold);
                Activate(Shape ? Shape : Compute::CollisionShape::Cube(1), Mass, CcdMotionThreshold);

				if (!Instance)
					return;

				UInt64 ActivationState;
				if (NMake::Unpack(Node->Find("activation-state"), &ActivationState))
					Instance->SetActivationState((Compute::MotionState)ActivationState);

				float AngularDamping;
				if (NMake::Unpack(Node->Find("angular-damping"), &AngularDamping))
					Instance->SetAngularDamping(AngularDamping);

				float AngularSleepingThreshold;
				if (NMake::Unpack(Node->Find("angular-sleeping-threshold"), &AngularSleepingThreshold))
					Instance->SetAngularSleepingThreshold(AngularSleepingThreshold);

				float Friction;
				if (NMake::Unpack(Node->Find("friction"), &Friction))
					Instance->SetFriction(Friction);

				float Restitution;
				if (NMake::Unpack(Node->Find("restitution"), &Restitution))
					Instance->SetRestitution(Restitution);

				float HitFraction;
				if (NMake::Unpack(Node->Find("hit-fraction"), &HitFraction))
					Instance->SetHitFraction(HitFraction);

				float LinearDamping;
				if (NMake::Unpack(Node->Find("linear-damping"), &LinearDamping))
					Instance->SetLinearDamping(LinearDamping);

				float LinearSleepingThreshold;
				if (NMake::Unpack(Node->Find("linear-sleeping-threshold"), &LinearSleepingThreshold))
					Instance->SetLinearSleepingThreshold(LinearSleepingThreshold);

				float CcdSweptSphereRadius;
				if (NMake::Unpack(Node->Find("ccd-swept-sphere-radius"), &CcdSweptSphereRadius))
					Instance->SetCcdSweptSphereRadius(CcdSweptSphereRadius);

				float ContactProcessingThreshold;
				if (NMake::Unpack(Node->Find("contact-processing-threshold"), &ContactProcessingThreshold))
					Instance->SetContactProcessingThreshold(ContactProcessingThreshold);

				float DeactivationTime;
				if (NMake::Unpack(Node->Find("deactivation-time"), &DeactivationTime))
					Instance->SetDeactivationTime(DeactivationTime);

				float RollingFriction;
				if (NMake::Unpack(Node->Find("rolling-friction"), &RollingFriction))
					Instance->SetRollingFriction(RollingFriction);

				float SpinningFriction;
				if (NMake::Unpack(Node->Find("spinning-friction"), &SpinningFriction))
					Instance->SetSpinningFriction(SpinningFriction);

				float ContactStiffness;
				if (NMake::Unpack(Node->Find("contact-stiffness"), &ContactStiffness))
					Instance->SetContactStiffness(ContactStiffness);

				float ContactDamping;
				if (NMake::Unpack(Node->Find("contact-damping"), &ContactDamping))
					Instance->SetContactDamping(ContactDamping);

				float AngularFactor;
				if (NMake::Unpack(Node->Find("angular-factor"), &AngularFactor))
					Instance->SetAngularFactor(AngularFactor);

				float AngularVelocity;
				if (NMake::Unpack(Node->Find("angular-velocity"), &AngularVelocity))
					Instance->SetAngularVelocity(AngularVelocity);

				float AnisotropicFriction;
				if (NMake::Unpack(Node->Find("anisotropic-friction"), &AnisotropicFriction))
					Instance->SetAnisotropicFriction(AnisotropicFriction);

				float Gravity;
				if (NMake::Unpack(Node->Find("gravity"), &Gravity))
					Instance->SetGravity(Gravity);

				float LinearFactor;
				if (NMake::Unpack(Node->Find("linear-factor"), &LinearFactor))
					Instance->SetLinearFactor(LinearFactor);

				float LinearVelocity;
				if (NMake::Unpack(Node->Find("linear-velocity"), &LinearVelocity))
					Instance->SetLinearVelocity(LinearVelocity);

				UInt64 CollisionFlags;
				if (NMake::Unpack(Node->Find("collision-flags"), &CollisionFlags))
					Instance->SetCollisionFlags(CollisionFlags);
            }
            void RigidBody::OnSave(ContentManager* Content, Rest::Document* Node)
            {
                NMake::Pack(Node->SetDocument("kinematic"), Kinematic);
                NMake::Pack(Node->SetDocument("extended"), !!Instance);
                if (!Instance)
                    return;

				NMake::Pack(Node->SetDocument("shape"), (UInt64)Instance->CollisionShapeType());
                btConvexHullShape* Shape = (btConvexHullShape*)Instance->CollisionShape();

                if (Instance->CollisionShapeType() == Compute::Shape_Convex_Hull && Shape)
                {
                    size_t Count = (size_t)Shape->getNumPoints();
					std::vector<btVector3> Vertices;
					Vertices.reserve(Count);

					btVector3* Points = Shape->getUnscaledPoints();
					for (size_t i = 0; i < Count; i++)
						Vertices.push_back(Points[i]);

					NMake::Pack(Node->SetDocument("shape-data"), Vertices);
                }

                NMake::Pack(Node->SetDocument("mass"), Instance->Mass());
                NMake::Pack(Node->SetDocument("ccd-motion-threshold"), Instance->CcdMotionThreshold());
                NMake::Pack(Node->SetDocument("activation-state"), (UInt64)Instance->ActivationState());
                NMake::Pack(Node->SetDocument("angular-damping"), Instance->AngularDamping());
                NMake::Pack(Node->SetDocument("angular-sleeping-threshold"), Instance->AngularSleepingThreshold());
                NMake::Pack(Node->SetDocument("friction"), Instance->Friction());
                NMake::Pack(Node->SetDocument("restitution"), Instance->Restitution());
                NMake::Pack(Node->SetDocument("hit-fraction"), Instance->HitFraction());
                NMake::Pack(Node->SetDocument("linear-damping"), Instance->LinearDamping());
                NMake::Pack(Node->SetDocument("linear-sleeping-threshold"), Instance->LinearSleepingThreshold());
                NMake::Pack(Node->SetDocument("ccd-swept-sphere-radius"), Instance->CcdSweptSphereRadius());
                NMake::Pack(Node->SetDocument("contact-processing-threshold"), Instance->ContactProcessingThreshold());
                NMake::Pack(Node->SetDocument("deactivation-time"), Instance->DeactivationTime());
                NMake::Pack(Node->SetDocument("rolling-friction"), Instance->RollingFriction());
                NMake::Pack(Node->SetDocument("spinning-friction"), Instance->SpinningFriction());
                NMake::Pack(Node->SetDocument("contact-stiffness"), Instance->ContactStiffness());
                NMake::Pack(Node->SetDocument("contact-damping"), Instance->ContactDamping());
                NMake::Pack(Node->SetDocument("angular-factor"), Instance->AngularFactor());
                NMake::Pack(Node->SetDocument("angular-velocity"), Instance->AngularVelocity());
                NMake::Pack(Node->SetDocument("anisotropic-friction"), Instance->AnisotropicFriction());
                NMake::Pack(Node->SetDocument("gravity"), Instance->Gravity());
                NMake::Pack(Node->SetDocument("linear-factor"), Instance->LinearFactor());
                NMake::Pack(Node->SetDocument("linear-velocity"), Instance->LinearVelocity());
                NMake::Pack(Node->SetDocument("collision-flags"), (UInt64)Instance->CollisionFlags());
            }
            void RigidBody::OnAwake(Component* New)
            {
                if (New != this || Instance)
                    return;

                Activate(Compute::CollisionShape::Cube(), 0, 0.0f);
            }
            void RigidBody::OnSynchronize(Rest::Timer* Time)
            {
                if (!Instance || !Synchronize)
                    return;

                btRigidBody* Bt = Instance->Bullet();
                btTransform& Offset = Bt->getWorldTransform();

                if (Kinematic)
                {
                    Offset.setOrigin(btVector3(Parent->Transform->Position.X, Parent->Transform->Position.Y, -Parent->Transform->Position.Z));
                    Offset.getBasis().setEulerZYX(Parent->Transform->Rotation.X, Parent->Transform->Rotation.Y, Parent->Transform->Rotation.Z);
                    Bt->getCollisionShape()->setLocalScaling(btVector3(Parent->Transform->Scale.X, Parent->Transform->Scale.Y, Parent->Transform->Scale.Z));
                }
                else
                {
                    Parent->Transform->Rotation = Compute::Matrix4x4(&Offset).Rotation();

                    btVector3 Value = Offset.getOrigin();
                    Parent->Transform->Position.X = Value.getX();
                    Parent->Transform->Position.Y = Value.getY();
                    Parent->Transform->Position.Z = -Value.getZ();

                    Value = Bt->getCollisionShape()->getLocalScaling();
                    Parent->Transform->Scale.X = Value.getX();
                    Parent->Transform->Scale.Y = Value.getY();
                    Parent->Transform->Scale.Z = Value.getZ();
                }
            }
            void RigidBody::OnAsleep()
            {
                if (Instance != nullptr)
                    Instance->SetAsGhost();
            }
            void RigidBody::Activate(btCollisionShape* Shape, float Mass, float Anticipation)
            {
                if (!Shape || !Parent || !Parent->GetScene() || !Parent->GetScene()->GetSimulator())
                    return;

                bool FirstTime = false;
                Parent->GetScene()->Lock();

                if (!Instance)
                {
                    Instance = new Compute::RigidBody(Parent->GetScene()->GetSimulator());
                    FirstTime = true;
                }

                Instance->Initialize(Shape, Parent->Transform, Mass, Anticipation);
                Instance->UserPointer = this;

                if (FirstTime)
                {
                    Instance->Add();
                    Instance->Activate(true);
                    Parent->GetScene()->Unlock();
                }
                else
                {
                    Parent->GetScene()->Unlock();
                    Reweight(Mass);
                }
            }
            void RigidBody::ResynchronizeFrom(Compute::Matrix4x4 World)
            {
                if (Parent && Parent->GetScene())
                    Parent->GetScene()->Lock();

                Parent->Transform->SetMatrix(World);
                Instance->Synchronize(Parent->Transform, true);
                if (Parent && Parent->GetScene())
                    Parent->GetScene()->Unlock();
            }
            void RigidBody::Resynchronize(bool Kinematically)
            {
                if (Parent && Parent->GetScene())
                    Parent->GetScene()->Lock();

                Instance->Synchronize(Parent->Transform, Kinematically);
                if (Parent && Parent->GetScene())
                    Parent->GetScene()->Unlock();
            }
            void RigidBody::Reweight(float Mass)
            {
                if (!Parent || !Parent->GetScene())
                    return;

                Parent->GetScene()->Lock();
                btVector3 Inertia = Parent->GetScene()->GetSimulator()->World->getGravity();

                Instance->Remove();
                Instance->SetGravity(Compute::Vector3(Inertia.getX(), Inertia.getY(), Inertia.getZ()));
                Instance->CollisionShape()->calculateLocalInertia(Mass, Inertia);
                Instance->Bullet()->setMassProps(Mass, Inertia);
                Instance->Add();
                Instance->Activate(true);
                Parent->GetScene()->Unlock();
            }
            Component* RigidBody::OnClone()
            {
                RigidBody* Object = new RigidBody(Parent);
                Object->Kinematic = Kinematic;

                if (!Instance)
                    return Object;

                btCollisionShape* Shape = Compute::CollisionShape::Auto(Instance->CollisionShapeType());
                if (Shape == nullptr && Instance->CollisionShapeType() == Compute::Shape_Convex_Hull)
                {
                    btConvexHullShape* ConvexHull = new btConvexHullShape();
                    btConvexHullShape* From = (btConvexHullShape*)Instance->CollisionShape();

                    for (UInt64 i = 0; i < From->getNumPoints(); i++)
                        ConvexHull->addPoint(*(From->getUnscaledPoints() + i), false);

                    ConvexHull->recalcLocalAabb();
                    ConvexHull->optimizeConvexHull();
                    Shape = ConvexHull;
                }

                Object->Activate(Shape, Instance->Mass(), Instance->CcdMotionThreshold());
                Object->Instance->Copy(Instance);
                Object->Instance->UserPointer = Object;

                return Object;
            }

            Acceleration::Acceleration(Entity* Ref) : Component(Ref)
            {
                AmplitudeVelocity = 0;
                AmplitudeTorque = 0;
                ConstantVelocity = 0;
                ConstantTorque = 0;
                ConstantCenter = 0;
                RigidBody = nullptr;
                Velocitize = false;
            }
            void Acceleration::OnLoad(ContentManager* Content, Rest::Document* Node)
            {
				NMake::Unpack(Node->Find("amplitude-velocity"), &AmplitudeVelocity);
				NMake::Unpack(Node->Find("amplitude-torque"), &AmplitudeTorque);
				NMake::Unpack(Node->Find("constant-velocity"), &ConstantVelocity);
				NMake::Unpack(Node->Find("constant-torque"), &ConstantTorque);
				NMake::Unpack(Node->Find("constant-center"), &ConstantCenter);
				NMake::Unpack(Node->Find("velocitize"), &Velocitize);
            }
            void Acceleration::OnSave(ContentManager* Content, Rest::Document* Node)
            {
                NMake::Pack(Node->SetDocument("amplitude-velocity"), AmplitudeVelocity);
                NMake::Pack(Node->SetDocument("amplitude-torque"), AmplitudeTorque);
                NMake::Pack(Node->SetDocument("constant-velocity"), ConstantVelocity);
                NMake::Pack(Node->SetDocument("constant-torque"), ConstantTorque);
                NMake::Pack(Node->SetDocument("constant-center"), ConstantCenter);
                NMake::Pack(Node->SetDocument("velocitize"), Velocitize);
            }
            void Acceleration::OnAwake(Component* New)
            {
                if (RigidBody)
                    return;

                Components::RigidBody* Component = Parent->GetComponent<Components::RigidBody>();
                if (Component != nullptr)
                    RigidBody = Component->Instance;
            }
            void Acceleration::OnSynchronize(Rest::Timer* Time)
            {
                if (!RigidBody)
                    return;

                float DeltaTime = (float)Time->GetDeltaTime();
                if (!Velocitize)
                {
                    RigidBody->SetLinearVelocity(ConstantVelocity);
                    RigidBody->SetAngularVelocity(ConstantTorque);
                }
                else
                    RigidBody->Push(ConstantVelocity * DeltaTime, ConstantTorque * DeltaTime, ConstantCenter);

                Compute::Vector3 Velocity = RigidBody->LinearVelocity();
                Compute::Vector3 Torque = RigidBody->AngularVelocity();
                Compute::Vector3 ACT = ConstantTorque.Abs();
                Compute::Vector3 ACV = ConstantVelocity.Abs();

                if (AmplitudeVelocity.X > 0 && Velocity.X > AmplitudeVelocity.X)
                    ConstantVelocity.X = -ACV.X;
                else if (AmplitudeVelocity.X > 0 && Velocity.X < -AmplitudeVelocity.X)
                    ConstantVelocity.X = ACV.X;

                if (AmplitudeVelocity.Y > 0 && Velocity.Y > AmplitudeVelocity.Y)
                    ConstantVelocity.Y = -ACV.Y;
                else if (AmplitudeVelocity.Y > 0 && Velocity.Y < -AmplitudeVelocity.Y)
                    ConstantVelocity.Y = ACV.Y;

                if (AmplitudeVelocity.Z > 0 && Velocity.Z > AmplitudeVelocity.Z)
                    ConstantVelocity.Z = -ACV.Z;
                else if (AmplitudeVelocity.Z > 0 && Velocity.Z < -AmplitudeVelocity.Z)
                    ConstantVelocity.Z = ACV.Z;

                if (AmplitudeTorque.X > 0 && Torque.X > AmplitudeTorque.X)
                    ConstantTorque.X = -ACT.X;
                else if (AmplitudeTorque.X > 0 && Torque.X < -AmplitudeTorque.X)
                    ConstantTorque.X = ACT.X;

                if (AmplitudeTorque.Y > 0 && Torque.Y > AmplitudeTorque.Y)
                    ConstantTorque.Y = -ACT.Y;
                else if (AmplitudeTorque.Y > 0 && Torque.Y < -AmplitudeTorque.Y)
                    ConstantTorque.Y = ACT.Y;

                if (AmplitudeTorque.Z > 0 && Torque.Z > AmplitudeTorque.Z)
                    ConstantTorque.Z = -ACT.Z;
                else if (AmplitudeTorque.Z > 0 && Torque.Z < -AmplitudeTorque.Z)
                    ConstantTorque.Z = ACT.Z;
            }
            Component* Acceleration::OnClone()
            {
                Acceleration* Instance = new Acceleration(Parent);
                Instance->Velocitize = Velocitize;
                Instance->AmplitudeTorque = AmplitudeTorque;
                Instance->AmplitudeVelocity = AmplitudeVelocity;
                Instance->ConstantCenter = ConstantCenter;
                Instance->ConstantTorque = ConstantTorque;
                Instance->ConstantVelocity = ConstantVelocity;
                Instance->RigidBody = RigidBody;

                return Instance;
            }

            SliderConstraint::SliderConstraint(Entity* Ref) : Component(Ref)
            {
                Constraint = nullptr;
                Connection = nullptr;
            }
            SliderConstraint::~SliderConstraint()
            {
				delete Constraint;
            }
            void SliderConstraint::OnLoad(ContentManager* Content, Rest::Document* Node)
            {
				bool Extended;
				NMake::Unpack(Node->Find("extended"), &Extended);
				if (!Extended)
					return;

				UInt64 Index;
				if (NMake::Unpack(Node->Find("connection"), &Index))
				{
					if (Parent->GetScene()->HasEntity(Index) != -1)
						Connection = Parent->GetScene()->GetEntity(Index);
				}

				bool CollisionState = false, LinearPowerState = false;
				NMake::Unpack(Node->Find("collision-state"), &CollisionState);
				NMake::Unpack(Node->Find("linear-power-state"), &CollisionState);
                Activate(CollisionState, LinearPowerState);

                if (!Constraint)
                    return;

				float AngularMotorVelocity;
				if (NMake::Unpack(Node->Find("angular-motor-velocity"), &AngularMotorVelocity))
					Constraint->SetAngularMotorVelocity(AngularMotorVelocity);

				float LinearMotorVelocity;
				if (NMake::Unpack(Node->Find("linear-motor-velocity"), &LinearMotorVelocity))
					Constraint->SetLinearMotorVelocity(LinearMotorVelocity);

				float UpperLinearLimit;
				if (NMake::Unpack(Node->Find("upper-linear-limit"), &UpperLinearLimit))
					Constraint->SetUpperLinearLimit(UpperLinearLimit);

				float LowerLinearLimit;
				if (NMake::Unpack(Node->Find("lower-linear-limit"), &LowerLinearLimit))
					Constraint->SetLowerLinearLimit(LowerLinearLimit);

				float AngularDampingDirection;
				if (NMake::Unpack(Node->Find("angular-damping-direction"), &AngularDampingDirection))
					Constraint->SetAngularDampingDirection(AngularDampingDirection);

				float LinearDampingDirection;
				if (NMake::Unpack(Node->Find("linear-damping-direction"), &LinearDampingDirection))
					Constraint->SetLinearDampingDirection(LinearDampingDirection);

				float AngularDampingLimit;
				if (NMake::Unpack(Node->Find("angular-damping-limit"), &AngularDampingLimit))
					Constraint->SetAngularDampingLimit(AngularDampingLimit);

				float LinearDampingLimit;
				if (NMake::Unpack(Node->Find("linear-damping-limit"), &LinearDampingLimit))
					Constraint->SetLinearDampingLimit(LinearDampingLimit);

				float AngularDampingOrtho;
				if (NMake::Unpack(Node->Find("angular-damping-ortho"), &AngularDampingOrtho))
					Constraint->SetAngularDampingOrtho(AngularDampingOrtho);

				float LinearDampingOrtho;
				if (NMake::Unpack(Node->Find("linear-damping-ortho"), &LinearDampingOrtho))
					Constraint->SetLinearDampingOrtho(LinearDampingOrtho);

				float UpperAngularLimit;
				if (NMake::Unpack(Node->Find("upper-angular-limit"), &UpperAngularLimit))
					Constraint->SetUpperAngularLimit(UpperAngularLimit);

				float LowerAngularLimit;
				if (NMake::Unpack(Node->Find("lower-angular-limit"), &LowerAngularLimit))
					Constraint->SetLowerAngularLimit(LowerAngularLimit);

				float MaxAngularMotorForce;
				if (NMake::Unpack(Node->Find("max-angular-motor-force"), &MaxAngularMotorForce))
					Constraint->SetMaxAngularMotorForce(MaxAngularMotorForce);

				float MaxLinearMotorForce;
				if (NMake::Unpack(Node->Find("max-linear-motor-force"), &MaxLinearMotorForce))
					Constraint->SetMaxLinearMotorForce(MaxLinearMotorForce);

				float AngularRestitutionDirection;
				if (NMake::Unpack(Node->Find("angular-restitution-direction"), &AngularRestitutionDirection))
					Constraint->SetAngularRestitutionDirection(AngularRestitutionDirection);

				float LinearRestitutionDirection;
				if (NMake::Unpack(Node->Find("linear-restitution-direction"), &LinearRestitutionDirection))
					Constraint->SetLinearRestitutionDirection(LinearRestitutionDirection);

				float AngularRestitutionLimit;
				if (NMake::Unpack(Node->Find("angular-restitution-limit"), &AngularRestitutionLimit))
					Constraint->SetAngularRestitutionLimit(AngularRestitutionLimit);

				float LinearRestitutionLimit;
				if (NMake::Unpack(Node->Find("linear-restitution-limit"), &LinearRestitutionLimit))
					Constraint->SetLinearRestitutionLimit(LinearRestitutionLimit);

				float AngularRestitutionOrtho;
				if (NMake::Unpack(Node->Find("angular-restitution-ortho"), &AngularRestitutionOrtho))
					Constraint->SetAngularRestitutionOrtho(AngularRestitutionOrtho);

				float LinearRestitutionOrtho;
				if (NMake::Unpack(Node->Find("linear-restitution-ortho"), &LinearRestitutionOrtho))
					Constraint->SetLinearRestitutionOrtho(LinearRestitutionOrtho);

				float AngularSoftnessDirection;
				if (NMake::Unpack(Node->Find("angular-softness-direction"), &AngularSoftnessDirection))
					Constraint->SetAngularSoftnessDirection(AngularSoftnessDirection);

				float LinearSoftnessDirection;
				if (NMake::Unpack(Node->Find("linear-softness-direction"), &LinearSoftnessDirection))
					Constraint->SetLinearSoftnessDirection(LinearSoftnessDirection);

				float AngularSoftnessLimit;
				if (NMake::Unpack(Node->Find("angular-softness-limit"), &AngularSoftnessLimit))
					Constraint->SetAngularSoftnessLimit(AngularSoftnessLimit);

				float LinearSoftnessLimit;
				if (NMake::Unpack(Node->Find("linear-softness-limit"), &LinearSoftnessLimit))
					Constraint->SetLinearSoftnessLimit(LinearSoftnessLimit);

				float AngularSoftnessOrtho;
				if (NMake::Unpack(Node->Find("angular-softness-ortho"), &AngularSoftnessOrtho))
					Constraint->SetAngularSoftnessOrtho(AngularSoftnessOrtho);

				float LinearSoftnessOrtho;
				if (NMake::Unpack(Node->Find("linear-softness-ortho"), &LinearSoftnessOrtho))
					Constraint->SetLinearSoftnessOrtho(LinearSoftnessOrtho);

				bool PoweredAngularMotor;
				if (NMake::Unpack(Node->Find("powered-angular-motor"), &PoweredAngularMotor))
					Constraint->SetPoweredAngularMotor(PoweredAngularMotor);

				bool PoweredLinearMotor;
				if (NMake::Unpack(Node->Find("powered-linear-motor"), &PoweredLinearMotor))
					Constraint->SetPoweredLinearMotor(PoweredLinearMotor);

				bool Enabled;
				if (NMake::Unpack(Node->Find("enabled"), &Enabled))
					Constraint->SetEnabled(Enabled);
            }
            void SliderConstraint::OnSave(ContentManager* Content, Rest::Document* Node)
            {
                NMake::Pack(Node->SetDocument("extended"), !!Constraint);
                if (!Constraint)
                    return;

                NMake::Pack(Node->SetDocument("connection"), (UInt64)(Connection ? Connection->Self : -1));
                NMake::Pack(Node->SetDocument("collision-state"), Constraint->FindCollisionState());
                NMake::Pack(Node->SetDocument("linear-power-state"), Constraint->FindLinearPowerState());
                NMake::Pack(Node->SetDocument("angular-motor-velocity"), Constraint->AngularMotorVelocity());
                NMake::Pack(Node->SetDocument("linear-motor-velocity"), Constraint->LinearMotorVelocity());
                NMake::Pack(Node->SetDocument("upper-linear-limit"), Constraint->UpperLinearLimit());
                NMake::Pack(Node->SetDocument("lower-linear-limit"), Constraint->LowerLinearLimit());
                NMake::Pack(Node->SetDocument("breaking-impulse-threshold"), Constraint->BreakingImpulseThreshold());
                NMake::Pack(Node->SetDocument("angular-damping-direction"), Constraint->AngularDampingDirection());
                NMake::Pack(Node->SetDocument("linear-amping-direction"), Constraint->LinearDampingDirection());
                NMake::Pack(Node->SetDocument("angular-damping-limit"), Constraint->AngularDampingLimit());
                NMake::Pack(Node->SetDocument("linear-damping-limit"), Constraint->LinearDampingLimit());
                NMake::Pack(Node->SetDocument("angular-damping-ortho"), Constraint->AngularDampingOrtho());
                NMake::Pack(Node->SetDocument("linear-damping-ortho"), Constraint->LinearDampingOrtho());
                NMake::Pack(Node->SetDocument("upper-angular-limit"), Constraint->UpperAngularLimit());
                NMake::Pack(Node->SetDocument("lower-angular-limit"), Constraint->LowerAngularLimit());
                NMake::Pack(Node->SetDocument("max-angular-motor-force"), Constraint->MaxAngularMotorForce());
                NMake::Pack(Node->SetDocument("max-linear-motor-force"), Constraint->MaxLinearMotorForce());
NMake::Pack(Node->SetDocument("angular-restitution-direction"), Constraint->AngularRestitutionDirection());
NMake::Pack(Node->SetDocument("linear-restitution-direction"), Constraint->LinearRestitutionDirection());
NMake::Pack(Node->SetDocument("angular-restitution-limit"), Constraint->AngularRestitutionLimit());
NMake::Pack(Node->SetDocument("linear-restitution-limit"), Constraint->LinearRestitutionLimit());
NMake::Pack(Node->SetDocument("angular-restitution-ortho"), Constraint->AngularRestitutionOrtho());
NMake::Pack(Node->SetDocument("linear-restitution-ortho"), Constraint->LinearRestitutionOrtho());
NMake::Pack(Node->SetDocument("angular-softness-direction"), Constraint->AngularSoftnessDirection());
NMake::Pack(Node->SetDocument("linear-softness-direction"), Constraint->LinearSoftnessDirection());
NMake::Pack(Node->SetDocument("angular-softness-limit"), Constraint->AngularSoftnessLimit());
NMake::Pack(Node->SetDocument("linear-softness-limit"), Constraint->LinearSoftnessLimit());
NMake::Pack(Node->SetDocument("angular-softness-ortho"), Constraint->AngularSoftnessOrtho());
NMake::Pack(Node->SetDocument("linear-softness-ortho"), Constraint->LinearSoftnessOrtho());
NMake::Pack(Node->SetDocument("powered-angular-motor"), Constraint->PoweredAngularMotor());
NMake::Pack(Node->SetDocument("powered-linear-motor"), Constraint->PoweredLinearMotor());
NMake::Pack(Node->SetDocument("enabled"), Constraint->Enabled());
			}
			void SliderConstraint::OnAwake(Component* New)
			{
				if (Parent->GetComponent<RigidBody>() && Connection)
					Activate(true, true);
			}
			void SliderConstraint::Activate(bool UseCollisions, bool UseLinearPower)
			{
				if (!Parent || !Parent->GetScene())
					return;

				Parent->GetScene()->Lock();
				delete Constraint;

				if (!Connection)
					return Parent->GetScene()->Unlock();

				RigidBody* FirstBody = Parent->GetComponent<RigidBody>();
				RigidBody* SecondBody = Connection->GetComponent<RigidBody>();
				if (!FirstBody || !SecondBody)
					return Parent->GetScene()->Unlock();

				Constraint = new Compute::SliderConstraint(Parent->GetScene()->GetSimulator());
				Constraint->Initialize(FirstBody->Instance, SecondBody->Instance, UseCollisions, UseLinearPower);
				Parent->GetScene()->Unlock();
			}
			Component* SliderConstraint::OnClone()
			{
				SliderConstraint* Instance = new SliderConstraint(Parent);
				Instance->Connection = Connection;

				if (!Constraint)
					return Instance;

				Instance->Constraint = new Compute::SliderConstraint(Parent->GetScene()->GetSimulator());
				Instance->Constraint->Copy(Constraint);

				return Instance;
			}

			AudioSource::AudioSource(Entity* Ref) : Component(Ref)
			{
				Source = new Audio::AudioSource();
				Position = 0.0f;
				Pitch = Gain = 1;
				Loop = 0;
				RefDistance = 0.25f;
				Distance = 100;
				Relative = 1;
				Direction = 0;
				Rolloff = 1;
				ConeInnerAngle = 360;
				ConeOuterAngle = 360;
				ConeOuterGain = 0;
			}
			AudioSource::~AudioSource()
			{
				delete Source;
			}
			void AudioSource::OnLoad(ContentManager* Content, Rest::Document* Node)
			{
				std::string Path;
				if (NMake::Unpack(Node->Find("audio-clip"), &Path))
					Source->Apply(Content->Load<Audio::AudioClip>(Path));

				NMake::Unpack(Node->Find("velocity"), &Velocity);
				NMake::Unpack(Node->Find("direction"), &Direction);
				NMake::Unpack(Node->Find("rolloff"), &Rolloff);
				NMake::Unpack(Node->Find("cone-inner-angle"), &ConeInnerAngle);
				NMake::Unpack(Node->Find("cone-outer-angle"), &ConeOuterAngle);
				NMake::Unpack(Node->Find("cone-outer-gain"), &ConeOuterGain);
				NMake::Unpack(Node->Find("distance"), &Distance);
				NMake::Unpack(Node->Find("gain"), &Gain);
				NMake::Unpack(Node->Find("pitch"), &Pitch);
				NMake::Unpack(Node->Find("ref-distance"), &RefDistance);
				NMake::Unpack(Node->Find("position"), &Position);
				NMake::Unpack(Node->Find("relative"), &Relative);
				NMake::Unpack(Node->Find("loop"), &Loop);
				NMake::Unpack(Node->Find("distance"), &Distance);

				bool Autoplay;
				if (NMake::Unpack(Node->Find("autoplay"), &Autoplay) && Autoplay && Source->Clip)
					Source->Play();

                ApplyPlayingPosition();
                OnSynchronize(nullptr);
            }
            void AudioSource::OnSave(ContentManager* Content, Rest::Document* Node)
            {
                AssetResource* Asset = Content->FindAsset(Source->Clip);
				if (Asset != nullptr)
					NMake::Pack(Node->SetDocument("audio-clip"), Asset->Path);

                NMake::Pack(Node->SetDocument("velocity"), Velocity);
                NMake::Pack(Node->SetDocument("direction"), Direction);
                NMake::Pack(Node->SetDocument("rolloff"), Rolloff);
                NMake::Pack(Node->SetDocument("cone-inner-angle"), ConeInnerAngle);
                NMake::Pack(Node->SetDocument("cone-outer-angle"), ConeOuterAngle);
                NMake::Pack(Node->SetDocument("cone-outer-gain"), ConeOuterGain);
                NMake::Pack(Node->SetDocument("distance"), Distance);
                NMake::Pack(Node->SetDocument("gain"), Gain);
                NMake::Pack(Node->SetDocument("pitch"), Pitch);
                NMake::Pack(Node->SetDocument("ref-distance"), RefDistance);
                NMake::Pack(Node->SetDocument("position"), Position);
                NMake::Pack(Node->SetDocument("relative"), Relative);
                NMake::Pack(Node->SetDocument("loop"), Loop);
                NMake::Pack(Node->SetDocument("distance"), Distance);
                NMake::Pack(Node->SetDocument("autoplay"), Source->IsPlaying());
            }
            void AudioSource::OnSynchronize(Rest::Timer* Time)
            {
                if (!Source->Clip)
                    return;

                if (Relative)
                    Audio::AudioContext::SetSourceData3F(Source->Instance, Audio::SoundEx_Position, 0, 0, 0);
                else
                    Audio::AudioContext::SetSourceData3F(Source->Instance, Audio::SoundEx_Position, -Parent->Transform->Position.X, -Parent->Transform->Position.Y, Parent->Transform->Position.Z);

                Audio::AudioContext::SetSourceData3F(Source->Instance, Audio::SoundEx_Velocity, Velocity.X, Velocity.Y, Velocity.Z);
                Audio::AudioContext::SetSourceData3F(Source->Instance, Audio::SoundEx_Direction, Direction.X, Direction.Y, Direction.Z);
                Audio::AudioContext::SetSourceData1I(Source->Instance, Audio::SoundEx_Source_Relative, Relative);
                Audio::AudioContext::SetSourceData1I(Source->Instance, Audio::SoundEx_Looping, Loop);
                Audio::AudioContext::SetSourceData1F(Source->Instance, Audio::SoundEx_Pitch, Pitch);
                Audio::AudioContext::SetSourceData1F(Source->Instance, Audio::SoundEx_Gain, Gain);
                Audio::AudioContext::SetSourceData1F(Source->Instance, Audio::SoundEx_Max_Distance, Distance);
                Audio::AudioContext::SetSourceData1F(Source->Instance, Audio::SoundEx_Reference_Distance, RefDistance);
                Audio::AudioContext::SetSourceData1F(Source->Instance, Audio::SoundEx_Rolloff_Factor, Rolloff);
                Audio::AudioContext::SetSourceData1F(Source->Instance, Audio::SoundEx_Cone_Inner_Angle, ConeInnerAngle);
                Audio::AudioContext::SetSourceData1F(Source->Instance, Audio::SoundEx_Cone_Outer_Angle, ConeOuterAngle);
                Audio::AudioContext::SetSourceData1F(Source->Instance, Audio::SoundEx_Cone_Outer_Gain, ConeOuterGain);
                Audio::AudioContext::GetSourceData1F(Source->Instance, Audio::SoundEx_Seconds_Offset, &Position);
            }
            void AudioSource::ApplyPlayingPosition()
            {
                Audio::AudioContext::SetSourceData1F(Source->Instance, Audio::SoundEx_Seconds_Offset, Position);
            }
            Component* AudioSource::OnClone()
            {
                AudioSource* Instance = new AudioSource(Parent);
                Instance->Distance = Distance;
                Instance->Gain = Gain;
                Instance->Loop = Loop;
                Instance->Pitch = Pitch;
                Instance->Position = Position;
                Instance->RefDistance = RefDistance;
                Instance->Relative = Relative;
                Instance->Source->Apply(Source->Clip);

                return Instance;
            }

            AudioListener::AudioListener(Entity* Ref) : Component(Ref)
            {
                Velocity = { 0, 0, 0 };
                Gain = 1.0f;
            }
            AudioListener::~AudioListener()
            {
                OnAsleep();
            }
            void AudioListener::OnLoad(ContentManager* Content, Rest::Document* Node)
            {
				NMake::Unpack(Node->Find("velocity"), &Velocity);
				NMake::Unpack(Node->Find("gain"), &Gain);
            }
            void AudioListener::OnSave(ContentManager* Content, Rest::Document* Node)
            {
                NMake::Pack(Node->SetDocument("velocity"), Velocity);
                NMake::Pack(Node->SetDocument("gain"), Gain);
            }
            void AudioListener::OnSynchronize(Rest::Timer* Time)
            {
                Compute::Vector3 Rotation = Parent->Transform->Rotation.DepthDirection();
                float LookAt[6] = { Rotation.X, Rotation.Y, Rotation.Z, 0.0f, 1.0f, 0.0f };

                Audio::AudioContext::SetListenerData3F(Audio::SoundEx_Velocity, Velocity.X, Velocity.Y, Velocity.Z);
                Audio::AudioContext::SetListenerData3F(Audio::SoundEx_Position, -Parent->Transform->Position.X, -Parent->Transform->Position.Y, Parent->Transform->Position.Z);
                Audio::AudioContext::SetListenerDataVF(Audio::SoundEx_Orientation, LookAt);
                Audio::AudioContext::SetListenerData1F(Audio::SoundEx_Gain, Gain);
            }
            void AudioListener::OnAsleep()
            {
                float LookAt[6] = { 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f };

                Audio::AudioContext::SetListenerData3F(Audio::SoundEx_Velocity, 0.0f, 0.0f, 0.0f);
                Audio::AudioContext::SetListenerData3F(Audio::SoundEx_Position, 0.0f, 0.0f, 0.0f);
                Audio::AudioContext::SetListenerDataVF(Audio::SoundEx_Orientation, LookAt);
                Audio::AudioContext::SetListenerData1F(Audio::SoundEx_Gain, 0.0f);
            }
            Component* AudioListener::OnClone()
            {
                AudioListener* Instance = new AudioListener(Parent);
                Instance->Velocity = Velocity;
                Instance->Gain = Gain;

                return Instance;
            }

            SkinAnimator::SkinAnimator(Entity* Ref) : Component(Ref)
            {
                Current.resize(96);
				Bind.resize(96);
				Default.resize(96);
            }
            SkinAnimator::~SkinAnimator()
            {
            }
			void SkinAnimator::OnLoad(ContentManager* Content, Rest::Document* Node)
			{
				NMake::Unpack(Node->Find("state"), &State);
				NMake::Unpack(Node->Find("bind"), &Bind);
				NMake::Unpack(Node->Find("current"), &Current);
				NMake::Unpack(Node->Find("animation"), &Clips);
			}
			void SkinAnimator::OnSave(ContentManager* Content, Rest::Document* Node)
			{
				NMake::Pack(Node->SetDocument("state"), State);
				NMake::Pack(Node->SetDocument("bind"), Bind);
				NMake::Pack(Node->SetDocument("current"), Current);
				NMake::Pack(Node->SetDocument("animation"), Clips);
			}
            void SkinAnimator::OnAwake(Component* New)
            {
                Components::SkinnedModel* Model = Parent->GetComponent<Components::SkinnedModel>();
				if (Model != nullptr && Model->Instance != nullptr)
				{
					Instance = Model;
					Instance->Skeleton.ResetKeys(Instance->Instance, &Default);
				}
				else
					Instance = nullptr;

				SetActive(!!Instance);
            }
			void SkinAnimator::OnSynchronize(Rest::Timer* Time)
			{
				Compute::Vector3& Position = *Parent->Transform->GetLocalPosition();
				Compute::Vector3& Rotation = *Parent->Transform->GetLocalRotation();
				Compute::Vector3& Scale = *Parent->Transform->GetLocalScale();

				if (!State.Blended)
				{
					if (State.Paused || State.Clip < 0 || State.Clip >= Clips.size() || State.Frame < 0 || State.Frame >= Clips[State.Clip].Keys.size())
						return;

					Compute::SkinAnimatorClip* Clip = &Clips[State.Clip];
					std::vector<Compute::AnimatorKey>& NextKey = Clip->Keys[State.Frame + 1 >= Clip->Keys.size() ? 0 : State.Frame + 1];
					std::vector<Compute::AnimatorKey>& PrevKey = Clip->Keys[State.Frame];
					State.Time = Compute::Math<float>::Min(State.Time + State.Speed * (float)Time->GetDeltaTime() / State.Length, State.Length);
					float Timing = Compute::Math<float>::Min(State.Time / State.Length, 1.0f);

					for (auto&& Pose : Instance->Skeleton.Pose)
					{
						Compute::Vector3 PositionSet = Pose.second.Position();
						Compute::Vector3 RotationSet = Pose.second.Rotation();
						Compute::Vector3 ScaleSet = Pose.second.Scale();
						Compute::AnimatorKey* Prev = &PrevKey[Pose.first];
						Compute::AnimatorKey* Next = &NextKey[Pose.first];
						Compute::AnimatorKey* Set = &Current[Pose.first];

						PositionSet = Set->Position = Prev->Position.Lerp(Next->Position, Timing);
						RotationSet = Set->Rotation = Prev->Rotation.AngularLerp(Next->Rotation, Timing);
						Pose.second = Compute::Matrix4x4::Create(PositionSet, RotationSet);
					}

					if (State.Time >= State.Length)
					{
						if (State.Frame + 1 >= Clip->Keys.size())
						{
							if (!State.Looped)
								BlendAnimation(-1, -1);

							State.Frame = -1;
						}

						State.Time = 0.0f;
						if (State.Looped || State.Frame != -1)
							State.Frame++;
					}
				}
				else if (State.Time < State.Length)
				{
					std::vector<Compute::AnimatorKey>* Key = GetFrame(State.Clip, State.Frame);
					if (!Key)
						Key = &Bind;

					State.Time = Compute::Math<float>::Min(State.Time + State.Speed * (float)Time->GetDeltaTime() / State.Length, State.Length);
					float Timing = Compute::Math<float>::Min(State.Time / State.Length, 1.0f);

					for (auto&& Pose : Instance->Skeleton.Pose)
					{
						Compute::Vector3 PositionSet = Pose.second.Position();
						Compute::Vector3 RotationSet = Pose.second.Rotation();
						Compute::Vector3 ScaleSet = Pose.second.Scale();
						Compute::AnimatorKey* Prev = &Current[Pose.first];
						Compute::AnimatorKey* Next = &Key->at(Pose.first);

						PositionSet = Prev->Position.Lerp(Next->Position, Timing);
						RotationSet = Prev->Rotation.AngularLerp(Next->Rotation, Timing);
						Pose.second = Compute::Matrix4x4::Create(PositionSet, RotationSet);
					}
				}
				else
				{
					State.Blended = false;
					State.Time = 0.0f;
				}
			}
			void SkinAnimator::BlendAnimation(Int64 Clip, Int64 Frame_)
			{
				State.Blended = true;
				State.Time = 0.0f;
				State.Frame = Frame_;
				State.Clip = Clip;

				if (State.Clip >= 0 && State.Clip < Clips.size())
				{
					Compute::SkinAnimatorClip* Clip = &Clips[State.Clip];
					if (State.Frame < 0 || State.Frame >= Clip->Keys.size())
						State.Frame = -1;
				}
				else
					State.Clip = -1;
			}
			void SkinAnimator::SavePose()
			{
				for (auto&& Pose : Instance->Skeleton.Pose)
				{
					Compute::Vector3 Position = Pose.second.Position();
					Compute::Vector3 Rotation = Pose.second.Rotation();
					Compute::Vector3 Scale = Pose.second.Scale();
					Compute::AnimatorKey* Frame = &Bind[Pose.first];
					Frame->Position = Position;
					Frame->Rotation = Rotation;
					Frame->Scale = Scale;
				}
			}
			void SkinAnimator::Stop()
			{
				Bind = Default;
				State.Paused = false;
				BlendAnimation(-1, -1);
			}
			void SkinAnimator::Pause()
			{
				State.Paused = true;
			}
			void SkinAnimator::Play(Int64 Clip, Int64 Frame_)
			{
				if (State.Paused)
				{
					State.Paused = false;
					return;
				}

				State.Time = 0.0f;
				State.Frame = (Frame_ == -1 ? 0 : Frame_);
				State.Clip = (Clip == -1 ? 0 : Clip);

				if (State.Clip >= 0 && State.Clip < Clips.size())
				{
					Compute::SkinAnimatorClip* Clip = &Clips[State.Clip];
					if (State.Frame < 0 || State.Frame >= Clip->Keys.size())
						State.Frame = -1;
				}
				else
					State.Clip = -1;

				SavePose(); Current = Bind;
				if (!IsPosed(State.Clip, State.Frame))
					BlendAnimation(State.Clip, State.Frame);
			}
			bool SkinAnimator::IsPosed(Int64 Clip, Int64 Frame_)
			{
				std::vector<Compute::AnimatorKey>* Key = GetFrame(Clip, Frame_);
				if (!Key)
					Key = &Bind;

				for (auto&& Pose : Instance->Skeleton.Pose)
				{
					Compute::Vector3 Position = Pose.second.Position();
					Compute::Vector3 Rotation = Pose.second.Rotation();
					Compute::Vector3 Scale = Pose.second.Scale();
					Compute::AnimatorKey* Frame = &Key->at(Pose.first);

					if (Position != Frame->Position || Rotation != Frame->Rotation || Scale != Frame->Scale)
						return false;
				}

				return true;
			}
			std::vector<Compute::AnimatorKey>* SkinAnimator::GetFrame(Int64 Clip, Int64 Frame)
			{
				if (Clip < 0 || Clip >= Clips.size() || Frame < 0 || Frame >= Clips[Clip].Keys.size())
					return nullptr;

				return &Clips[Clip].Keys[Frame];
			}
			std::vector<std::vector<Compute::AnimatorKey>>* SkinAnimator::GetClip(Int64 Clip)
			{
				if (Clip < 0 || Clip >= Clips.size())
					return nullptr;

				return &Clips[Clip].Keys;
			}
			Component* SkinAnimator::OnClone()
			{
				SkinAnimator* Instance = new SkinAnimator(Parent);
				Instance->Clips = Clips;
				Instance->State = State;
				Instance->Bind = Bind;
				Instance->Current = Current;

				return Instance;
			}

            KeyAnimator::KeyAnimator(Entity* Ref) : Component(Ref)
            {
            }
            KeyAnimator::~KeyAnimator()
            {
            }
            void KeyAnimator::OnLoad(ContentManager* Content, Rest::Document* Node)
            {
				NMake::Unpack(Node->Find("state"), &State);
				NMake::Unpack(Node->Find("bind"), &Bind);
				NMake::Unpack(Node->Find("current"), &Current);
				NMake::Unpack(Node->Find("animation"), &Clips);
            }
            void KeyAnimator::OnSave(ContentManager* Content, Rest::Document* Node)
            {
                NMake::Pack(Node->SetDocument("state"), State);
				NMake::Pack(Node->SetDocument("bind"), Bind);
				NMake::Pack(Node->SetDocument("current"), Current);
				NMake::Pack(Node->SetDocument("animation"), Clips);
            }
			void KeyAnimator::OnSynchronize(Rest::Timer* Time)
			{
				Compute::Vector3& Position = *Parent->Transform->GetLocalPosition();
				Compute::Vector3& Rotation = *Parent->Transform->GetLocalRotation();
				Compute::Vector3& Scale = *Parent->Transform->GetLocalScale();

				if (!State.Blended)
				{
					if (State.Paused || State.Clip < 0 || State.Clip >= Clips.size() || State.Frame < 0 || State.Frame >= Clips[State.Clip].Keys.size())
						return;

					Compute::KeyAnimatorClip* Clip = &Clips[State.Clip];
					Compute::AnimatorKey& NextKey = Clip->Keys[State.Frame + 1 >= Clip->Keys.size() ? 0 : State.Frame + 1];
					Compute::AnimatorKey& PrevKey = Clip->Keys[State.Frame];
					State.Time = Compute::Math<float>::Min(State.Time + PrevKey.PlayingSpeed * (float)Time->GetDeltaTime() / State.Length, State.Length);
					float Timing = Compute::Math<float>::Min(State.Time / State.Length, 1.0f);

					Position = Current.Position = PrevKey.Position.Lerp(NextKey.Position, Timing);
					Rotation = Current.Rotation = PrevKey.Rotation.AngularLerp(NextKey.Rotation, Timing);
					Scale = Current.Scale = PrevKey.Scale.Lerp(NextKey.Scale, Timing);

					if (State.Time >= State.Length)
					{
						if (State.Frame + 1 >= Clip->Keys.size())
						{
							if (!State.Looped)
								BlendAnimation(-1, -1);
							
							State.Frame = -1;
						}

						State.Time = 0.0f;
						if (State.Looped || State.Frame != -1)
							State.Frame++;
					}
				}
				else if (State.Time < State.Length)
				{
					Compute::AnimatorKey* Key = GetFrame(State.Clip, State.Frame);
					if (!Key)
						Key = &Bind;

					State.Time = Compute::Math<float>::Min(State.Time + State.Speed * (float)Time->GetDeltaTime() / State.Length, State.Length);
					float Timing = Compute::Math<float>::Min(State.Time / State.Length, 1.0f);

					Position = Current.Position.Lerp(Key->Position, Timing);
					Rotation = Current.Rotation.AngularLerp(Key->Rotation, Timing);
					Scale = Current.Scale.Lerp(Key->Scale, Timing);
				}
				else
				{
					State.Blended = false;
					State.Time = 0.0f;
				}
			}
			void KeyAnimator::BlendAnimation(Int64 Clip, Int64 Frame_)
			{
				State.Blended = true;
				State.Time = 0.0f;
				State.Frame = Frame_;
				State.Clip = Clip;

				if (State.Clip >= 0 && State.Clip < Clips.size())
				{
					Compute::KeyAnimatorClip* Clip = &Clips[State.Clip];
					if (State.Frame < 0 || State.Frame >= Clip->Keys.size())
						State.Frame = -1;
				}
				else
					State.Clip = -1;
			}
			void KeyAnimator::SavePose()
			{
				Bind.Position = *Parent->Transform->GetLocalPosition();
				Bind.Rotation = *Parent->Transform->GetLocalRotation();
				Bind.Scale = *Parent->Transform->GetLocalScale();
			}
            void KeyAnimator::Stop()
            {
                State.Paused = false;
                BlendAnimation(-1, -1);
            }
            void KeyAnimator::Pause()
            {
                State.Paused = true;
            }
			void KeyAnimator::Play(Int64 Clip, Int64 Frame_)
			{
				if (State.Paused)
				{
					State.Paused = false;
					return;
				}

				State.Time = 0.0f;
				State.Frame = (Frame_ == -1 ? 0 : Frame_);
				State.Clip = (Clip == -1 ? 0 : Clip);

				if (State.Clip >= 0 && State.Clip < Clips.size())
				{
					Compute::KeyAnimatorClip* Clip = &Clips[State.Clip];
					if (State.Frame < 0 || State.Frame >= Clip->Keys.size())
						State.Frame = -1;
				}
				else
					State.Clip = -1;

				SavePose(); Current = Bind;
				if (!IsPosed(State.Clip, State.Frame))
					BlendAnimation(State.Clip, State.Frame);
			}
            bool KeyAnimator::IsPosed(Int64 Clip, Int64 Frame_)
            {
				Compute::AnimatorKey* Key = GetFrame(Clip, Frame_);
				if (!Key)
					Key = &Bind;

                return
					*Parent->Transform->GetLocalPosition() == Key->Position &&
					*Parent->Transform->GetLocalRotation() == Key->Rotation &&
					*Parent->Transform->GetLocalScale() == Key->Scale;
            }
			Compute::AnimatorKey* KeyAnimator::GetFrame(Int64 Clip, Int64 Frame)
			{
				if (Clip < 0 || Clip >= Clips.size() || Frame < 0 || Frame >= Clips[Clip].Keys.size())
					return nullptr;

				return &Clips[Clip].Keys[Frame];
			}
			std::vector<Compute::AnimatorKey>* KeyAnimator::GetClip(Int64 Clip)
			{
				if (Clip < 0 || Clip >= Clips.size())
					return nullptr;

				return &Clips[Clip].Keys;
			}
            Component* KeyAnimator::OnClone()
            {
                KeyAnimator* Instance = new KeyAnimator(Parent);
				Instance->Clips = Clips;
				Instance->State = State;
				Instance->Bind = Bind;
				Instance->Current = Current;

                return Instance;
            }

            ElementSystem::ElementSystem(Entity* Ref) : Component(Ref)
            {
                StrongConnection = false;
                QuadBased = false;
                Visibility = 0.0f;
                Volume = 3.0f;
                Diffusion = 1.0f;
                TexCoord = 1.0f;
                Diffuse = nullptr;
				Instance = nullptr;
            }
            ElementSystem::~ElementSystem()
            {
                if (Instance != nullptr)
                {
                    Instance->Restore();
					delete Instance;
                }
            }
            void ElementSystem::OnAwake(Component* New)
            {
                if (Instance || !Parent || !Parent->GetScene())
                    return;

                Graphics::InstanceBuffer::Desc I = Graphics::InstanceBuffer::Desc();
                I.ElementLimit = 1 << 10;

                Instance = Graphics::InstanceBuffer::Create(Parent->GetScene()->GetDevice(), I);
            }
            void ElementSystem::OnLoad(ContentManager* Content, Rest::Document* Node)
            {
				std::string Path;
				if (NMake::Unpack(Node->Find("diffuse"), &Path))
					Diffuse = Content->Load<Graphics::Texture2D>(Path);

				NMake::Unpack(Node->Find("quad-based"), &QuadBased);
				NMake::Unpack(Node->Find("diffusion"), &Diffusion);
				NMake::Unpack(Node->Find("texcoord"), &TexCoord);
				NMake::Unpack(Node->Find("strong-connection"), &StrongConnection);
				NMake::Unpack(Node->Find("visibility"), &Visibility);
				NMake::Unpack(Node->Find("volume"), &Volume);
				NMake::Unpack(Node->Find("material"), &Material);

				UInt64 Limit;
				if (!NMake::Unpack(Node->Find("limit"), &Limit))
				{
					std::vector<Compute::ElementVertex> Vertices;
					if (Instance != nullptr)
					{
						Instance->Resize(Limit);
						if (NMake::Unpack(Node->Find("elements"), &Vertices))
						{
							Instance->GetArray()->Reserve(Vertices.size());
							for (auto&& Vertex : Vertices)
								Instance->GetArray()->Add(Vertex);
						}
					}
				}
            }
            void ElementSystem::OnSave(ContentManager* Content, Rest::Document* Node)
            {
				AssetResource* Asset = Content->FindAsset(Diffuse);
				if (Asset != nullptr)
					NMake::Pack(Node->SetDocument("diffuse"), Asset->Path);

                NMake::Pack(Node->SetDocument("quad-based"), QuadBased);
                NMake::Pack(Node->SetDocument("diffusion"), Diffusion);
                NMake::Pack(Node->SetDocument("texcoord"), TexCoord);
                NMake::Pack(Node->SetDocument("strong-connection"), StrongConnection);
                NMake::Pack(Node->SetDocument("visibility"), Visibility);
                NMake::Pack(Node->SetDocument("volume"), Volume);
                NMake::Pack(Node->SetDocument("material"), Material);

				if (Instance != nullptr)
				{
					std::vector<Compute::ElementVertex> Vertices;
					Vertices.reserve(Instance->GetArray()->Size());

					for (auto It = Instance->GetArray()->Begin(); It != Instance->GetArray()->End(); It++)
						Vertices.emplace_back(*It);

					NMake::Pack(Node->SetDocument("limit"), Instance->GetElementLimit());
					NMake::Pack(Node->SetDocument("elements"), Vertices);
				}
            }
            void ElementSystem::OnSynchronize(Rest::Timer* Time)
            {
                Viewer View = Parent->GetScene()->GetCameraViewer();
                float Hardness = 1.0f - Parent->Transform->Position.Distance(Parent->GetScene()->GetCamera()->GetEntity()->Transform->Position) / (View.ViewDistance + Volume);

                if (Hardness > 0.0f)
                    Visibility = Compute::MathCommon::IsClipping(View.ViewProjection, Parent->Transform->GetWorld(), 1.5f) == -1 ? Hardness : 0.0f;
                else
                    Visibility = 0.0f;
            }
            void ElementSystem::OnEvent(Event* Value)
            {
                if (Value->Is<Graphics::Material>())
                {
                    if (Value->Get<Graphics::Material>()->Self == (float)Material)
                        Material = 0;
                }
            }
            Graphics::Material& ElementSystem::GetMaterial()
            {
                if (Material < 0 || Material >= (unsigned int)Parent->GetScene()->GetMaterialCount())
                    return Parent->GetScene()->GetMaterialStandartLit();

                return Parent->GetScene()->GetMaterial((int)Material);
            }
            Component* ElementSystem::OnClone()
            {
                ElementSystem* Clone = new ElementSystem(Parent);
                Clone->Visibility = Visibility;
                Clone->Volume = Volume;
                Clone->StrongConnection = StrongConnection;
                Clone->Instance->GetArray()->Copy(*Instance->GetArray());

                return Clone;
            }

            ElementAnimator::ElementAnimator(Entity* Ref) : Component(Ref)
            {
                Positioning = 0.0f;
                Diffusing = 0.0f;
                ScaleSpeed = 0.0f;
                RotationSpeed = 0.0f;
                Velocitizing = 1000000000.0f;
                Spawner.Scale.Max = 1;
                Spawner.Scale.Min = 1;
                Spawner.Rotation.Max = 0;
                Spawner.Rotation.Min = 0;
                Spawner.Angular.Max = 0;
                Spawner.Angular.Min = 0;
                Spawner.Diffusion.Min = 1;
                Spawner.Diffusion.Max = 1;
                Spawner.Velocity.Min = 0;
                Spawner.Velocity.Max = 0;
                Spawner.Position.Min = -1;
                Spawner.Position.Max = 1;
                Spawner.Noise.Min = -1;
                Spawner.Noise.Max = 1;
                Spawner.Iterations = 1;
                Simulate = true;
                Noisiness = 0.0f;
                System = nullptr;
            }
            void ElementAnimator::OnLoad(ContentManager* Content, Rest::Document* Node)
            {
				NMake::Unpack(Node->Find("diffusing"), &Diffusing);
				NMake::Unpack(Node->Find("positioning"), &Positioning);
				NMake::Unpack(Node->Find("velocitizing"), &Velocitizing);
				NMake::Unpack(Node->Find("spawner"), &Spawner);
				NMake::Unpack(Node->Find("noisiness"), &Noisiness);
				NMake::Unpack(Node->Find("rotation-speed"), &RotationSpeed);
				NMake::Unpack(Node->Find("scale-speed"), &ScaleSpeed);
				NMake::Unpack(Node->Find("simulate"), &Simulate);
            }
            void ElementAnimator::OnSave(ContentManager* Content, Rest::Document* Node)
            {
                NMake::Pack(Node->SetDocument("diffusing"), Diffusing);
                NMake::Pack(Node->SetDocument("positioning"), Positioning);
                NMake::Pack(Node->SetDocument("velocitizing"), Velocitizing);
                NMake::Pack(Node->SetDocument("spawner"), Spawner);
                NMake::Pack(Node->SetDocument("noisiness"), Noisiness);
                NMake::Pack(Node->SetDocument("rotation-speed"), RotationSpeed);
                NMake::Pack(Node->SetDocument("scale-speed"), ScaleSpeed);
                NMake::Pack(Node->SetDocument("simulate"), Simulate);
            }
            void ElementAnimator::OnAwake(Component* New)
            {
                System = Parent->GetComponent<ElementSystem>();
                SetActive(System != nullptr);
            }
            void ElementAnimator::OnSynchronize(Rest::Timer* Time)
            {
                if (!System || !System->Instance)
                    return;

                Rest::Pool<Compute::ElementVertex>* Array = System->Instance->GetArray();
                for (int i = 0; i < Spawner.Iterations; i++)
                {
                    if (Array->Size() + 1 >= System->Instance->GetElementLimit())
                        continue;

                    Compute::Vector3 Position = (System->StrongConnection ? Spawner.Position.Generate() : Spawner.Position.Generate() + Parent->Transform->Position.InvertZ());
                    Compute::Vector3 Velocity = Spawner.Velocity.Generate();
                    Compute::Vector4 Diffusion = Spawner.Diffusion.Generate();

                    Compute::ElementVertex Element;
                    Element.PositionX = Position.X;
                    Element.PositionY = Position.Y;
                    Element.PositionZ = Position.Z;
                    Element.VelocityX = Velocity.X;
                    Element.VelocityY = Velocity.Y;
                    Element.VelocityZ = Velocity.Z;
                    Element.ColorX = Diffusion.X;
                    Element.ColorY = Diffusion.Y;
                    Element.ColorZ = Diffusion.Z;
                    Element.ColorW = Diffusion.W;
                    Element.Angular = Spawner.Angular.Generate();
                    Element.Rotation = Spawner.Rotation.Generate();
                    Element.Scale = Spawner.Scale.Generate();
                    Array->Add(Element);
                }

                if (!Simulate)
                    return;

                float DeltaTime = (float)Time->GetDeltaTime();
                if (Noisiness != 0.0f)
                    AccurateSynchronization(DeltaTime);
                else
                    FastSynchronization(DeltaTime);
            }
            void ElementAnimator::AccurateSynchronization(float DeltaTime)
            {
                Rest::Pool<Compute::ElementVertex>* Array = System->Instance->GetArray();
                for (auto It = Array->Begin(); It != Array->End(); It++)
                {
                    Compute::Vector3 Noise = Spawner.Noise.Generate() / Noisiness;
                    It->PositionX += (It->VelocityX + Positioning.X + Noise.X) * DeltaTime;
                    It->PositionY += (It->VelocityY + Positioning.Y + Noise.Y) * DeltaTime;
                    It->PositionZ += (It->VelocityZ + Positioning.Z + Noise.Z) * DeltaTime;
                    It->VelocityX -= (It->VelocityX / Velocitizing.X) * DeltaTime;
                    It->VelocityY -= (It->VelocityY / Velocitizing.Y) * DeltaTime;
                    It->VelocityZ -= (It->VelocityZ / Velocitizing.Z) * DeltaTime;
                    It->ColorX += Diffusing.X * DeltaTime;
                    It->ColorY += Diffusing.Y * DeltaTime;
                    It->ColorZ += Diffusing.Z * DeltaTime;
                    It->ColorW += Diffusing.W * DeltaTime;
                    It->Scale += ScaleSpeed * DeltaTime;
                    It->Rotation += (It->Angular + RotationSpeed) * DeltaTime;

                    if (It->ColorW <= 0 || It->Scale <= 0)
                        Array->RemoveAt(It);
                }
            }
            void ElementAnimator::FastSynchronization(float DeltaTime)
            {
                Rest::Pool<Compute::ElementVertex>* Array = System->Instance->GetArray();
                for (auto It = Array->Begin(); It != Array->End(); It++)
                {
                    It->PositionX += (It->VelocityX + Positioning.X) * DeltaTime;
                    It->PositionY += (It->VelocityY + Positioning.Y) * DeltaTime;
                    It->PositionZ += (It->VelocityZ + Positioning.Z) * DeltaTime;
                    It->VelocityX -= (It->VelocityX / Velocitizing.X) * DeltaTime;
                    It->VelocityY -= (It->VelocityY / Velocitizing.Y) * DeltaTime;
                    It->VelocityZ -= (It->VelocityZ / Velocitizing.Z) * DeltaTime;
                    It->ColorX += Diffusing.X * DeltaTime;
                    It->ColorY += Diffusing.Y * DeltaTime;
                    It->ColorZ += Diffusing.Z * DeltaTime;
                    It->ColorW += Diffusing.W * DeltaTime;
                    It->Scale += ScaleSpeed * DeltaTime;
                    It->Rotation += (It->Angular + RotationSpeed) * DeltaTime;

                    if (It->ColorW <= 0 || It->Scale <= 0)
                        Array->RemoveAt(It);
                }
            }
            Component* ElementAnimator::OnClone()
            {
                ElementAnimator* Instance = new ElementAnimator(Parent);
                Instance->Diffusing = Diffusing;
                Instance->Positioning = Positioning;
                Instance->Velocitizing = Velocitizing;
                Instance->ScaleSpeed = ScaleSpeed;
                Instance->RotationSpeed = RotationSpeed;
                Instance->Spawner = Spawner;
                Instance->Noisiness = Noisiness;
                Instance->Simulate = Simulate;

                return Instance;
            }

            FreeLook::FreeLook(Entity* Ref) : Component(Ref), Activity(nullptr), Rotate(Graphics::KeyCode_CURSORRIGHT), Sensivity(0.005f)
            {
            }
            void FreeLook::OnAwake(Component* New)
            {
                Application* App = Application::Get();
                if (App == nullptr)
                    return SetActive(false);

                Activity = App->Activity;
                SetActive(Activity != nullptr);
            }
            void FreeLook::OnRenovate(Rest::Timer* Time)
            {
                if (!Activity)
                    return;

                if (Activity->IsKeyDown(Rotate))
                {
                    Compute::Vector2 Cursor = Activity->GetCursorPosition();
                    if (!Activity->IsKeyDownHit(Rotate))
                    {
                        float X = (Cursor.Y - Position.Y) * Sensivity;
                        float Y = (Cursor.X - Position.X) * Sensivity;
                        Parent->Transform->Rotation += Compute::Vector3(X, Y);
                        Parent->Transform->Rotation.X = Compute::Math<float>::Clamp(Parent->Transform->Rotation.X, -1.57079632679f, 1.57079632679f);
                    }
                    else
                        Position = Cursor;

                    if ((int)Cursor.X != (int)Position.X || (int)Cursor.Y != (int)Position.Y)
                        Activity->SetCursorPosition(Position);
                }
            }

            Fly::Fly(Entity* Ref) : Component(Ref), Activity(nullptr)
            {
                SpeedNormal = 1.0f;
                SpeedUp = 4.0f;
                SpeedDown = 0.25f;
                Forward = Graphics::KeyCode_W;
                Backward = Graphics::KeyCode_S;
                Right = Graphics::KeyCode_D;
                Left = Graphics::KeyCode_A;
                Up = Graphics::KeyCode_SPACE;
                Down = Graphics::KeyCode_Z;
                Fast = Graphics::KeyCode_LSHIFT;
                Slow = Graphics::KeyCode_LCTRL;
                Axis = Compute::Vector3(1, 1, -1);
            }
            void Fly::OnAwake(Component* New)
            {
                Application* App = Application::Get();
                if (App == nullptr)
                    return SetActive(false);

                Activity = App->Activity;
                SetActive(Activity != nullptr);
            }
            void Fly::OnRenovate(Rest::Timer* Time)
            {
                if (!Activity)
                    return;

                float DeltaTime = (float)Time->GetDeltaTime();
                Compute::Vector3 Speed = Axis * DeltaTime * SpeedNormal;

                if (Activity->IsKeyDown(Fast))
                    Speed = Axis * DeltaTime * SpeedUp;

                if (Activity->IsKeyDown(Slow))
                    Speed = Axis * DeltaTime * SpeedDown;

                if (Activity->IsKeyDown(Forward))
                    Parent->Transform->Position += Parent->Transform->Forward() * Speed;

                if (Activity->IsKeyDown(Backward))
                    Parent->Transform->Position -= Parent->Transform->Forward() * Speed;

                if (Activity->IsKeyDown(Right))
                    Parent->Transform->Position += Parent->Transform->Right() * Speed;

                if (Activity->IsKeyDown(Left))
                    Parent->Transform->Position -= Parent->Transform->Right() * Speed;

                if (Activity->IsKeyDown(Up))
                    Parent->Transform->Position += Parent->Transform->Up() * Speed;

                if (Activity->IsKeyDown(Down))
                    Parent->Transform->Position -= Parent->Transform->Up() * Speed;
            }

            Model::Model(Entity* Ref) : Component(Ref)
            {
                Visibility = false;
                Instance = nullptr;
            }
            void Model::OnLoad(ContentManager* Content, Rest::Document* Node)
            {
				std::string Path;
				if (NMake::Unpack(Node->Find("model"), &Path))
					Instance = Content->Load<Graphics::Model>(Path);

				std::vector<Rest::Document*> Faces = Node->FindCollectionPath("surfaces.surface");
				for (auto&& Surface : Faces)
				{
					TSurface Face;
					if (NMake::Unpack(Surface->Find("diffuse"), &Path))
						Face.Diffuse = Content->Load<Graphics::Texture2D>(Path);

					if (NMake::Unpack(Surface->Find("normal"), &Path))
						Face.Normal = Content->Load<Graphics::Texture2D>(Path);

					if (NMake::Unpack(Surface->Find("surface"), &Path))
						Face.Surface = Content->Load<Graphics::Texture2D>(Path);

					NMake::Unpack(Surface->Find("diffusion"), &Face.Diffusion);
					NMake::Unpack(Surface->Find("texcoord"), &Face.TexCoord);
					NMake::Unpack(Surface->Find("material"), &Face.Material);

					if (NMake::Unpack(Surface->Find("name"), &Path))
					{
						if (Instance != nullptr)
						{
							Graphics::Mesh* Ref = Instance->Find(Path);
							if (Ref != nullptr)
								Surfaces[Ref] = Face;
						}
					}
				}

				NMake::Unpack(Node->Find("visibility"), &Visibility);
            }
            void Model::OnSave(ContentManager* Content, Rest::Document* Node)
            {
				AssetResource* Asset = Content->FindAsset(Instance);
				if (Asset != nullptr)
					NMake::Pack(Node->SetDocument("model"), Asset->Path);

				Rest::Document* Faces = Node->SetArray("surfaces");
				for (auto&& It : Surfaces)
				{
					Rest::Document* Surface = Faces->SetDocument("surface");

					Asset = Content->FindAsset(It.second.Diffuse);
					if (Asset != nullptr)
						NMake::Pack(Surface->SetDocument("diffuse"), Asset->Path);

					Asset = Content->FindAsset(It.second.Normal);
					if (Asset != nullptr)
						NMake::Pack(Surface->SetDocument("normal"), Asset->Path);

					Asset = Content->FindAsset(It.second.Surface);
					if (Asset != nullptr)
						NMake::Pack(Surface->SetDocument("surface"), Asset->Path);

					NMake::Pack(Surface->SetDocument("diffusion"), It.second.Diffusion);
					NMake::Pack(Surface->SetDocument("texcoord"), It.second.TexCoord);
					NMake::Pack(Surface->SetDocument("material"), It.second.Material);

					if (It.first != nullptr)
						NMake::Pack(Surface->SetDocument("name"), It.first->Name);
				}

                NMake::Pack(Node->SetDocument("visibility"), Visibility);
            }
            void Model::OnSynchronize(Rest::Timer* Time)
            {
                if (!Instance)
                {
                    Visibility = false;
                    return;
                }

                Viewer View = Parent->GetScene()->GetCameraViewer();
                if (Parent->Transform->Position.Distance(View.RealPosition) < View.ViewDistance + Parent->Transform->Scale.Length())
					Visibility = Compute::MathCommon::IsClipping(View.ViewProjection, GetBoundingBox(), 1.5f) == -1;
                else
                    Visibility = false;
            }
            void Model::OnEvent(Event* Value)
            {
				if (!Value->Is<Graphics::Material>())
					return;

				UInt64 Material = (UInt64)Value->Get<Graphics::Material>()->Self;
				for (auto&& Surface : Surfaces)
				{
					if (Surface.second.Material == Material)
						Surface.second.Material = 0;
				}
            }
			Graphics::Material& Model::GetMaterial(Graphics::Mesh* Mesh)
			{
				return GetMaterial(GetSurface(Mesh));
			}
			Graphics::Material& Model::GetMaterial(TSurface* Surface)
			{
				if (!Surface || Surface->Material >= Parent->GetScene()->GetMaterialCount())
					return Parent->GetScene()->GetMaterialStandartLit();

				return Parent->GetScene()->GetMaterial(Surface->Material);
			}
			TSurface* Model::GetSurface(Graphics::Mesh* Mesh)
			{
				auto It = Surfaces.find(Mesh);
				if (It == Surfaces.end())
				{
					Surfaces[Mesh] = TSurface();
					It = Surfaces.find(Mesh);
				}

				return &It->second;
			}
			Compute::Matrix4x4 Model::GetBoundingBox()
			{
				if (!Instance)
					return Parent->Transform->GetWorld();

				return Compute::Matrix4x4::Create(
					Parent->Transform->Position,
					Parent->Transform->Scale * Compute::Vector3(Instance->Min.W - Instance->Max.W).Div(2.0f).Abs(),
					Parent->Transform->Rotation);
			}
            Component* Model::OnClone()
            {
                Model* New = new Model(Parent);
                New->Visibility = Visibility;
                New->Instance = Instance;
				New->Surfaces = Surfaces;

                return New;
            }

            SkinnedModel::SkinnedModel(Entity* Ref) : Component(Ref)
            {
                Visibility = false;
                Instance = nullptr;
            }
            void SkinnedModel::OnLoad(ContentManager* Content, Rest::Document* Node)
            {
				std::string Path;
				if (NMake::Unpack(Node->Find("skinned-model"), &Path))
					Instance = Content->Load<Graphics::SkinnedModel>(Path);

				std::vector<Rest::Document*> Faces = Node->FindCollectionPath("surfaces.surface");
				for (auto&& Surface : Faces)
				{
					TSurface Face;
					if (NMake::Unpack(Surface->Find("diffuse"), &Path))
						Face.Diffuse = Content->Load<Graphics::Texture2D>(Path);

					if (NMake::Unpack(Surface->Find("normal"), &Path))
						Face.Normal = Content->Load<Graphics::Texture2D>(Path);

					if (NMake::Unpack(Surface->Find("surface"), &Path))
						Face.Surface = Content->Load<Graphics::Texture2D>(Path);

					NMake::Unpack(Surface->Find("diffusion"), &Face.Diffusion);
					NMake::Unpack(Surface->Find("texcoord"), &Face.TexCoord);
					NMake::Unpack(Surface->Find("material"), &Face.Material);

					if (NMake::Unpack(Surface->Find("name"), &Path))
					{
						if (Instance != nullptr)
						{
							Graphics::SkinnedMesh* Ref = Instance->FindMesh(Path);
							if (Ref != nullptr)
								Surfaces[Ref] = Face;
						}
					}
				}

				std::vector<Rest::Document*> Poses = Node->FindCollectionPath("poses.pose");
				for (auto&& Pose : Poses)
				{
					Int64 Index;
					NMake::Unpack(Pose->Find("[index]"), &Index);

					Compute::Matrix4x4 Matrix;
					NMake::Unpack(Pose->Find("[matrix]"), &Matrix);

					Skeleton.Pose[Index] = Matrix;
				}

				NMake::Unpack(Node->Find("visibility"), &Visibility);
            }
            void SkinnedModel::OnSave(ContentManager* Content, Rest::Document* Node)
            {
				AssetResource* Asset = Content->FindAsset(Instance);
				if (Asset != nullptr)
					NMake::Pack(Node->SetDocument("skinned-model"), Asset->Path);

				Rest::Document* Faces = Node->SetArray("surfaces");
				for (auto&& It : Surfaces)
				{
					Rest::Document* Surface = Faces->SetDocument("surface");

					Asset = Content->FindAsset(It.second.Diffuse);
					if (Asset != nullptr)
						NMake::Pack(Surface->SetDocument("diffuse"), Asset->Path);

					Asset = Content->FindAsset(It.second.Normal);
					if (Asset != nullptr)
						NMake::Pack(Surface->SetDocument("normal"), Asset->Path);

					Asset = Content->FindAsset(It.second.Surface);
					if (Asset != nullptr)
						NMake::Pack(Surface->SetDocument("surface"), Asset->Path);

					NMake::Pack(Surface->SetDocument("diffusion"), It.second.Diffusion);
					NMake::Pack(Surface->SetDocument("texcoord"), It.second.TexCoord);
					NMake::Pack(Surface->SetDocument("material"), It.second.Material);

					if (It.first != nullptr)
						NMake::Pack(Surface->SetDocument("name"), It.first->Name);
				}

				NMake::Pack(Node->SetDocument("visibility"), Visibility);

				Rest::Document* Poses = Node->SetArray("poses");
				for (auto&& Pose : Skeleton.Pose)
				{
					Rest::Document* Value = Poses->SetDocument("pose");
					NMake::Pack(Value->SetDocument("[index]"), Pose.first);
					NMake::Pack(Value->SetDocument("[matrix]"), Pose.second);
				}
            }
            void SkinnedModel::OnSynchronize(Rest::Timer* Time)
            {
				if (!Instance)
				{
					Visibility = false;
					return;
				}
				else
					Instance->BuildSkeleton(&Skeleton);

                Viewer View = Parent->GetScene()->GetCameraViewer();
				if (Parent->Transform->Position.Distance(View.RealPosition) < View.ViewDistance + Parent->Transform->Scale.Length())
					Visibility = Compute::MathCommon::IsClipping(View.ViewProjection, GetBoundingBox(), 1.5f) == -1;
				else
                    Visibility = false;
            }
            void SkinnedModel::OnEvent(Event* Value)
            {
				if (!Value->Is<Graphics::Material>())
					return;

				UInt64 Material = (UInt64)Value->Get<Graphics::Material>()->Self;
				for (auto&& Surface : Surfaces)
				{
					if (Surface.second.Material == Material)
						Surface.second.Material = 0;
				}
            }
			Graphics::Material& SkinnedModel::GetMaterial(Graphics::SkinnedMesh* Mesh)
			{
				return GetMaterial(GetSurface(Mesh));
			}
			Graphics::Material& SkinnedModel::GetMaterial(TSurface* Surface)
			{
				if (!Surface || Surface->Material >= Parent->GetScene()->GetMaterialCount())
					return Parent->GetScene()->GetMaterialStandartLit();

				return Parent->GetScene()->GetMaterial(Surface->Material);
			}
			TSurface* SkinnedModel::GetSurface(Graphics::SkinnedMesh* Mesh)
			{
				auto It = Surfaces.find(Mesh);
				if (It == Surfaces.end())
				{
					Surfaces[Mesh] = TSurface();
					It = Surfaces.find(Mesh);
				}

				return &It->second;
			}
			Compute::Matrix4x4 SkinnedModel::GetBoundingBox()
			{
				if (!Instance)
					return Parent->Transform->GetWorld();

				return Compute::Matrix4x4::Create(
					Parent->Transform->Position,
					Parent->Transform->Scale * Compute::Vector3(Instance->Min.W - Instance->Max.W).Div(2.0f).Abs(),
					Parent->Transform->Rotation);
			}
            Component* SkinnedModel::OnClone()
            {
                SkinnedModel* New = new SkinnedModel(Parent);
                New->Visibility = Visibility;
                New->Instance = Instance;
				New->Surfaces = Surfaces;

                return New;
            }

            PointLight::PointLight(Entity* Ref) : Component(Ref)
            {
                Diffusion = Compute::Vector3::One();
                Visibility = 0.0f;
                Emission = 1.0f;
                Range = 5.0f;
            }
            void PointLight::OnLoad(ContentManager* Content, Rest::Document* Node)
            {
				NMake::Unpack(Node->Find("diffusion"), &Diffusion);
				NMake::Unpack(Node->Find("visibility"), &Visibility);
				NMake::Unpack(Node->Find("emission"), &Emission);
				NMake::Unpack(Node->Find("range"), &Range);
				NMake::Unpack(Node->Find("shadow-softness"), &ShadowSoftness);
				NMake::Unpack(Node->Find("shadow-distance"), &ShadowDistance);
				NMake::Unpack(Node->Find("shadow-bias"), &ShadowBias);
				NMake::Unpack(Node->Find("shadow-iterations"), &ShadowIterations);
				NMake::Unpack(Node->Find("shadowed"), &Shadowed);
				NMake::Unpack(Node->Find("projection"), &Projection);
				NMake::Unpack(Node->Find("view"), &View);
            }
            void PointLight::OnSave(ContentManager* Content, Rest::Document* Node)
            {
                NMake::Pack(Node->SetDocument("diffusion"), Diffusion);
                NMake::Pack(Node->SetDocument("visibility"), Visibility);
                NMake::Pack(Node->SetDocument("emission"), Emission);
                NMake::Pack(Node->SetDocument("range"), Range);
                NMake::Pack(Node->SetDocument("shadow-softness"), ShadowSoftness);
                NMake::Pack(Node->SetDocument("shadow-distance"), ShadowDistance);
                NMake::Pack(Node->SetDocument("shadow-bias"), ShadowBias);
                NMake::Pack(Node->SetDocument("shadow-iterations"), ShadowIterations);
                NMake::Pack(Node->SetDocument("shadowed"), Shadowed);
                NMake::Pack(Node->SetDocument("projection"), Projection);
                NMake::Pack(Node->SetDocument("view"), View);
            }
            void PointLight::OnSynchronize(Rest::Timer* Time)
            {
                Viewer IView = Parent->GetScene()->GetCameraViewer();
                float Hardness = 1.0f - Parent->Transform->Position.Distance(Parent->GetScene()->GetCamera()->GetEntity()->Transform->Position) / IView.ViewDistance;

                Projection = Compute::Matrix4x4::CreatePerspective(90.0f, 1.0f, 0.1f, ShadowDistance);
                View = Compute::Matrix4x4::CreateCubeMapLookAt(0, Parent->Transform->Position.InvertZ());

                if (Hardness > 0.0f)
                    Visibility = Compute::MathCommon::IsClipping(IView.ViewProjection, Parent->Transform->GetWorld(), Range) == -1 ? Hardness : 0.0f;
                else
                    Visibility = 0.0f;
            }
            Component* PointLight::OnClone()
            {
                PointLight* Instance = new PointLight(Parent);
                Instance->Diffusion = Diffusion;
                Instance->Emission = Emission;
                Instance->Visibility = Visibility;
                Instance->Range = Range;
                Instance->Projection = Projection;
                Instance->ShadowBias = ShadowBias;
                Instance->ShadowDistance = ShadowDistance;
                Instance->ShadowIterations = ShadowIterations;
                Instance->ShadowSoftness = ShadowSoftness;
                Instance->Shadowed = Shadowed;
                Instance->View = View;

                return Instance;
            }

            SpotLight::SpotLight(Entity* Ref) : Component(Ref)
            {
                Diffusion = Compute::Vector3::One();
                Occlusion = nullptr;
                Diffuse = nullptr;
                ShadowSoftness = 0.0f;
                ShadowIterations = 2;
                ShadowDistance = 100;
                ShadowBias = 0.0f;
                FieldOfView = 90.0f;
                Range = 10.0f;
                Emission = 1.0f;
                Shadowed = false;
            }
            void SpotLight::OnLoad(ContentManager* Content, Rest::Document* Node)
            {
				std::string Path;
				if (NMake::Unpack(Node->Find("diffuse"), &Path))
					Diffuse = Content->Load<Graphics::Texture2D>(Path);

				NMake::Unpack(Node->Find("diffusion"), &Diffusion);
				NMake::Unpack(Node->Find("visibility"), &Visibility);
				NMake::Unpack(Node->Find("projection"), &Projection);
				NMake::Unpack(Node->Find("view"), &View);
				NMake::Unpack(Node->Find("shadow-bias"), &ShadowBias);
				NMake::Unpack(Node->Find("shadow-distance"), &ShadowDistance);
				NMake::Unpack(Node->Find("shadow-softness"), &ShadowSoftness);
				NMake::Unpack(Node->Find("shadow-iterations"), &ShadowIterations);
				NMake::Unpack(Node->Find("field-of-view"), &FieldOfView);
				NMake::Unpack(Node->Find("range"), &Range);
				NMake::Unpack(Node->Find("emission"), &Emission);
				NMake::Unpack(Node->Find("shadowed"), &Shadowed);
            }
            void SpotLight::OnSave(ContentManager* Content, Rest::Document* Node)
            {
				AssetResource* Asset = Content->FindAsset(Diffuse);
				if (Asset != nullptr)
					NMake::Pack(Node->SetDocument("diffuse"), Asset->Path);

                NMake::Pack(Node->SetDocument("diffusion"), Diffusion);
                NMake::Pack(Node->SetDocument("visibility"), Visibility);
                NMake::Pack(Node->SetDocument("projection"), Projection);
                NMake::Pack(Node->SetDocument("view"), View);
                NMake::Pack(Node->SetDocument("shadow-bias"), ShadowBias);
                NMake::Pack(Node->SetDocument("shadow-distance"), ShadowDistance);
                NMake::Pack(Node->SetDocument("shadow-softness"), ShadowSoftness);
                NMake::Pack(Node->SetDocument("shadow-iterations"), ShadowIterations);
                NMake::Pack(Node->SetDocument("field-of-view"), FieldOfView);
                NMake::Pack(Node->SetDocument("range"), Range);
                NMake::Pack(Node->SetDocument("emission"), Emission);
                NMake::Pack(Node->SetDocument("shadowed"), Shadowed);
            }
            void SpotLight::OnSynchronize(Rest::Timer* Time)
            {
                Viewer IView = Parent->GetScene()->GetCameraViewer();
                float Hardness = 1.0f - Parent->Transform->Position.Distance(Parent->GetScene()->GetCamera()->GetEntity()->Transform->Position) / IView.ViewDistance;

                Projection = Compute::Matrix4x4::CreatePerspective(FieldOfView, 1, 0.1f, ShadowDistance);
                View = Compute::Matrix4x4::CreateTranslation(-Parent->Transform->Position) * Compute::Matrix4x4::CreateCameraRotation(-Parent->Transform->Rotation);

                if (Hardness > 0.0f)
                    Visibility = Compute::MathCommon::IsClipping(IView.ViewProjection, Parent->Transform->GetWorld(), Range) == -1 ? Hardness : 0.0f;
                else
                    Visibility = 0.0f;
            }
            Component* SpotLight::OnClone()
            {
                SpotLight* Instance = new SpotLight(Parent);
                Instance->Diffuse = Diffuse;
                Instance->Projection = Projection;
                Instance->View = View;
                Instance->Diffusion = Diffusion;
                Instance->FieldOfView = FieldOfView;
                Instance->Range = Range;
                Instance->Emission = Emission;
                Instance->Shadowed = Shadowed;
                Instance->ShadowBias = ShadowBias;
                Instance->ShadowDistance = ShadowDistance;
                Instance->ShadowIterations = ShadowIterations;
                Instance->ShadowSoftness = ShadowSoftness;

                return Instance;
            }

            LineLight::LineLight(Entity* Ref) : Component(Ref)
            {
                Diffusion = Compute::Vector3(1.0, 0.8, 0.501961);
                Occlusion = nullptr;
                Shadowed = false;
                ShadowSoftness = 0.0f;
                ShadowDistance = 100;
                ShadowBias = 0.0f;
                ShadowFarBias = 20.0f;
                ShadowLength = 16;
                ShadowHeight = 0.0f;
                ShadowIterations = 2;
                Emission = 1.0f;
            }
            void LineLight::OnLoad(ContentManager* Content, Rest::Document* Node)
            {
				NMake::Unpack(Node->Find("diffusion"), &Diffusion);
				NMake::Unpack(Node->Find("projection"), &Projection);
				NMake::Unpack(Node->Find("view"), &View);
				NMake::Unpack(Node->Find("shadow-bias"), &ShadowBias);
				NMake::Unpack(Node->Find("shadow-distance"), &ShadowDistance);
				NMake::Unpack(Node->Find("shadow-far-bias"), &ShadowFarBias);
				NMake::Unpack(Node->Find("shadow-softness"), &ShadowSoftness);
				NMake::Unpack(Node->Find("shadow-length"), &ShadowLength);
				NMake::Unpack(Node->Find("shadow-height"), &ShadowHeight);
				NMake::Unpack(Node->Find("shadow-iterations"), &ShadowIterations);
				NMake::Unpack(Node->Find("emission"), &Emission);
				NMake::Unpack(Node->Find("shadowed"), &Shadowed);
            }
            void LineLight::OnSave(ContentManager* Content, Rest::Document* Node)
            {
                NMake::Pack(Node->SetDocument("diffusion"), Diffusion);
                NMake::Pack(Node->SetDocument("projection"), Projection);
                NMake::Pack(Node->SetDocument("view"), View);
                NMake::Pack(Node->SetDocument("shadow-bias"), ShadowBias);
                NMake::Pack(Node->SetDocument("shadow-distance"), ShadowDistance);
                NMake::Pack(Node->SetDocument("shadow-far-bias"), ShadowFarBias);
                NMake::Pack(Node->SetDocument("shadow-softness"), ShadowSoftness);
                NMake::Pack(Node->SetDocument("shadow-length"), ShadowLength);
                NMake::Pack(Node->SetDocument("shadow-height"), ShadowHeight);
                NMake::Pack(Node->SetDocument("shadow-iterations"), ShadowIterations);
                NMake::Pack(Node->SetDocument("emission"), Emission);
                NMake::Pack(Node->SetDocument("shadowed"), Shadowed);
            }
            void LineLight::OnSynchronize(Rest::Timer* Time)
            {
                Projection = Compute::Matrix4x4::CreateOrthographic(ShadowDistance, ShadowDistance, -ShadowDistance / 2.0f - ShadowFarBias, ShadowDistance / 2.0f + ShadowFarBias);
                View = Compute::Matrix4x4::CreateLineLightLookAt(Parent->Transform->Position, Parent->GetScene()->GetCamera()->GetEntity()->Transform->Position.SetY(ShadowHeight));
            }
            Component* LineLight::OnClone()
            {
                LineLight* Instance = new LineLight(Parent);
                Instance->Projection = Projection;
                Instance->View = View;
                Instance->Diffusion = Diffusion;
                Instance->ShadowBias = ShadowBias;
                Instance->ShadowDistance = ShadowDistance;
                Instance->ShadowFarBias = ShadowFarBias;
                Instance->ShadowSoftness = ShadowSoftness;
                Instance->ShadowLength = ShadowLength;
                Instance->ShadowHeight = ShadowHeight;
                Instance->ShadowIterations = ShadowIterations;
                Instance->Emission = Emission;
                Instance->Shadowed = Shadowed;

                return Instance;
            }

            ProbeLight::ProbeLight(Entity* Ref) : Component(Ref)
            {
                Projection = Compute::Matrix4x4::CreatePerspectiveRad(1.57079632679f, 1, 0.01f, 100.0f);
                Diffusion = Compute::Vector3::One();
                ViewOffset = Compute::Vector3(1, 1, -1);
                DiffusePX = nullptr;
				DiffuseNX = nullptr;
				DiffusePY = nullptr;
				DiffuseNY = nullptr;
				DiffusePZ = nullptr;
				DiffuseNZ = nullptr;
				Diffuse = nullptr;
                Emission = 1.0f;
                Range = 5.0f;
                Visibility = 0.0f;
                CaptureRange = Range;
                ImageBased = false;
                ParallaxCorrected = false;
            }
            ProbeLight::~ProbeLight()
            {
                if (!ImageBased && Diffuse != nullptr)
					delete Diffuse;
            }
            void ProbeLight::OnLoad(ContentManager* Content, Rest::Document* Node)
            {
				std::string Path;
				if (NMake::Unpack(Node->Find("diffuse-px"), &Path))
					DiffusePX = Content->Load<Graphics::Texture2D>(Path);

				if (NMake::Unpack(Node->Find("diffuse-nx"), &Path))
					DiffuseNX = Content->Load<Graphics::Texture2D>(Path);

				if (NMake::Unpack(Node->Find("diffuse-py"), &Path))
					DiffusePY = Content->Load<Graphics::Texture2D>(Path);

				if (NMake::Unpack(Node->Find("diffuse-ny"), &Path))
					DiffuseNY = Content->Load<Graphics::Texture2D>(Path);

				if (NMake::Unpack(Node->Find("diffuse-pz"), &Path))
					DiffusePZ = Content->Load<Graphics::Texture2D>(Path);

				if (NMake::Unpack(Node->Find("diffuse-nz"), &Path))
					DiffuseNZ = Content->Load<Graphics::Texture2D>(Path);

				std::vector<Compute::Matrix4x4> Views;
				NMake::Unpack(Node->Find("view"), &Views);
				NMake::Unpack(Node->Find("rebuild"), &Rebuild);
				NMake::Unpack(Node->Find("projection"), &Projection);
				NMake::Unpack(Node->Find("diffusion"), &Diffusion);
				NMake::Unpack(Node->Find("visibility"), &Visibility);
				NMake::Unpack(Node->Find("range"), &Range);
				NMake::Unpack(Node->Find("capture-range"), &CaptureRange);
				NMake::Unpack(Node->Find("emission"), &Emission);
				NMake::Unpack(Node->Find("image-based"), &ImageBased);
				NMake::Unpack(Node->Find("parallax-corrected"), &ParallaxCorrected);

				Int64 Count = Compute::Math<Int64>::Min((Int64)Views.size(), 6);
				for (Int64 i = 0; i < Count; i++)
					View[i] = Views[i];

				if (ImageBased)
					RebuildDiffuseMap();
            }
            void ProbeLight::OnSave(ContentManager* Content, Rest::Document* Node)
            {
				AssetResource* Asset = Content->FindAsset(DiffusePX);
				if (Asset != nullptr)
					NMake::Pack(Node->SetDocument("diffuse-px"), Asset->Path);

				Asset = Content->FindAsset(DiffuseNX);
				if (Asset != nullptr)
					NMake::Pack(Node->SetDocument("diffuse-nx"), Asset->Path);

				Asset = Content->FindAsset(DiffusePY);
				if (Asset != nullptr)
					NMake::Pack(Node->SetDocument("diffuse-py"), Asset->Path);

				Asset = Content->FindAsset(DiffuseNY);
				if (Asset != nullptr)
					NMake::Pack(Node->SetDocument("diffuse-ny"), Asset->Path);

				Asset = Content->FindAsset(DiffusePZ);
				if (Asset != nullptr)
					NMake::Pack(Node->SetDocument("diffuse-pz"), Asset->Path);

				Asset = Content->FindAsset(DiffuseNZ);
				if (Asset != nullptr)
					NMake::Pack(Node->SetDocument("diffuse-nz"), Asset->Path);

				std::vector<Compute::Matrix4x4> Views;
				for (Int64 i = 0; i < 6; i++)
					Views.push_back(View[i]);

				NMake::Pack(Node->SetDocument("view"), Views);
                NMake::Pack(Node->SetDocument("rebuild"), Rebuild);
                NMake::Pack(Node->SetDocument("projection"), Projection);
                NMake::Pack(Node->SetDocument("diffusion"), Diffusion);
                NMake::Pack(Node->SetDocument("visibility"), Visibility);
                NMake::Pack(Node->SetDocument("range"), Range);
                NMake::Pack(Node->SetDocument("capture-range"), CaptureRange);
                NMake::Pack(Node->SetDocument("emission"), Emission);
                NMake::Pack(Node->SetDocument("image-based"), ImageBased);
                NMake::Pack(Node->SetDocument("parallax-corrected"), ParallaxCorrected);
            }
            void ProbeLight::OnSynchronize(Rest::Timer* Time)
            {
                Viewer View = Parent->GetScene()->GetCameraViewer();
                float Hardness = 1.0f - Parent->Transform->Position.Distance(Parent->GetScene()->GetCamera()->GetEntity()->Transform->Position) / (View.ViewDistance);

                if (Hardness > 0.0f)
                    Visibility = Compute::MathCommon::IsClipping(View.ViewProjection, Parent->Transform->GetWorld(), Range) == -1 ? Hardness : 0.0f;
                else
                    Visibility = 0.0f;
            }
			bool ProbeLight::RebuildDiffuseMap()
			{
				if (!DiffusePX || !DiffuseNX || !DiffusePY || !DiffuseNY || !DiffusePZ || !DiffuseNZ)
					return false;

				Graphics::TextureCube::Desc F;
				F.Texture2D[0] = DiffusePX;
				F.Texture2D[1] = DiffuseNX;
				F.Texture2D[2] = DiffusePY;
				F.Texture2D[3] = DiffuseNY;
				F.Texture2D[4] = DiffusePZ;
				F.Texture2D[5] = DiffuseNZ;

				delete Diffuse;
				Diffuse = Graphics::TextureCube::Create(Parent->GetScene()->GetDevice(), F);
				return Diffuse != nullptr;
			}
            Component* ProbeLight::OnClone()
            {
                ProbeLight* Instance = new ProbeLight(Parent);
                Instance->Projection = Projection;
                Instance->Range = Range;
                Instance->Diffusion = Diffusion;
                Instance->Visibility = Visibility;
                Instance->Emission = Emission;
                Instance->CaptureRange = CaptureRange;
                Instance->ImageBased = ImageBased;
                Instance->Rebuild = Rebuild;
                memcpy(Instance->View, View, 6 * sizeof(Compute::Matrix4x4));

                return Instance;
            }

            Camera::Camera(Entity* Ref) : Component(Ref)
            {
                Projection = Compute::Matrix4x4::Identity();
                ViewDistance = 500;
            }
            Camera::~Camera()
            {
				delete Renderer;
            }
            void Camera::OnAwake(Component* New)
            {
                if (!Parent || !Parent->GetScene() || (New && New != this))
                    return;

                if (!Renderer)
                    Renderer = new RenderSystem(Parent->GetScene()->GetDevice());

                if (Renderer->GetScene() == Parent->GetScene() && New != this)
                    return;

                Renderer->SetScene(Parent->GetScene());
                auto* RenderStages = Renderer->GetRenderStages();
                for (auto It = RenderStages->begin(); It != RenderStages->end(); It++)
                {
                    (*It)->OnRelease();
                    (*It)->SetRenderer(Renderer);
                    (*It)->OnInitialize();
                }
            }
            void Camera::OnAsleep()
            {
                if (!Renderer)
                    return;

                if (Parent && Parent->GetScene() && Parent->GetScene()->GetCamera() == this)
                    Parent->GetScene()->SetCamera(nullptr);
            }
            void Camera::OnLoad(ContentManager* Content, Rest::Document* Node)
            {
				NMake::Unpack(Node->Find("projection"), &Projection);
				NMake::Unpack(Node->Find("view-distance"), &ViewDistance);

				if (!Renderer)
				{
					if (Parent->GetScene() && Parent->GetScene()->GetDevice())
						Renderer = new RenderSystem(Parent->GetScene()->GetDevice());
					else
						Renderer = new RenderSystem(Content->GetDevice());
				}

				std::vector<Rest::Document*> Renderers = Node->FindCollectionPath("renderers.renderer");
                Renderer->SetScene(Parent->GetScene());

				for (auto& Render : Renderers)
				{
					UInt64 Id = RendererId_Empty;
					NMake::Unpack(Render->Find("type"), &Id);

					Engine::Renderer* Ref = Renderer->AddRenderStageByType(Id);
					if (!Ref)
						continue;

					Rest::Document* Meta = Render->Find("metadata");
					if (!Meta)
						Meta = Render->SetDocument("metadata");

					Ref->OnRelease();
					Ref->OnLoad(Content, Meta);
					Ref->OnInitialize();
					NMake::Unpack(Render->Find("active"), &Ref->Active);
				}
            }
            void Camera::OnSave(ContentManager* Content, Rest::Document* Node)
            {
                NMake::Pack(Node->SetDocument("projection"), Projection);
                NMake::Pack(Node->SetDocument("view-distance"), ViewDistance);

                Rest::Document* Renderers = Node->SetArray("renderers");
                for (auto& Ref : *Renderer->GetRenderStages())
                {
                    Rest::Document* Render = Renderers->SetDocument("renderer");
					NMake::Pack(Render->SetDocument("type"), Ref->Type());
					NMake::Pack(Render->SetDocument("active"), Ref->Active);
                    Ref->OnSave(Content, Render->SetDocument("metadata"));
                }
            }
            void Camera::FillViewer(Viewer* View)
            {
                View->ViewPosition = Compute::Vector3(-Parent->Transform->Position.X, -Parent->Transform->Position.Y, Parent->Transform->Position.Z);
                View->ViewProjection = Compute::Matrix4x4::CreateCamera(View->ViewPosition, -Parent->Transform->Rotation) * Projection;
                View->InvViewProjection = View->ViewProjection.Invert();
                View->Projection = Projection;
                View->Position = Parent->Transform->Position.InvertZ();
                View->RealPosition = Parent->Transform->Position;
                View->ViewDistance = ViewDistance;
                View->Renderer = Renderer;
                FieldView = *View;
            }
            void Camera::ResizeBuffers()
            {
                if (!Renderer)
                    return;

                Renderer->SetScene(Parent->GetScene());
                auto* RenderStages = Renderer->GetRenderStages();
                for (auto It = RenderStages->begin(); It != RenderStages->end(); It++)
                    (*It)->OnResizeBuffers();
            }
            Viewer Camera::GetViewer()
            {
                return FieldView;
            }
            Compute::Vector3 Camera::GetViewPosition()
            {
                return Compute::Vector3(-Parent->Transform->Position.X, -Parent->Transform->Position.Y, Parent->Transform->Position.Z);
            }
            Compute::Matrix4x4 Camera::GetViewProjection()
            {
                return GetView() * Projection;
            }
            Compute::Matrix4x4 Camera::GetView()
            {
                return Compute::Matrix4x4::CreateCamera(GetViewPosition(), -Parent->Transform->Rotation);
            }
            float Camera::GetFieldOfView()
            {
                return Compute::Math<float>::ACotan(Projection.Row[5]) * 2.0f / Compute::Math<float>::Deg2Rad();
            }
            Component* Camera::OnClone()
            {
                Camera* Instance = new Camera(Parent);
                Instance->ViewDistance = ViewDistance;
                Instance->Projection = Projection;

                return Instance;
            }
        }
    }
}