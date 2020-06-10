#include "components.h"
#include "renderers.h"

namespace Tomahawk
{
	namespace Engine
	{
		namespace Components
		{
			RigidBody::RigidBody(Entity* Ref) : Component(Ref)
			{
				Hull = nullptr;
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

				float Mass = 0, CcdMotionThreshold = 0;
				NMake::Unpack(Node->Find("mass"), &Mass);
				NMake::Unpack(Node->Find("ccd-motion-threshold"), &CcdMotionThreshold);

				Rest::Document* CV = nullptr;
				if ((CV = Node->Find("shape")) != nullptr)
				{
					std::string Path;
					uint64_t Type;
					if (NMake::Unpack(Node->Find("path"), &Path))
					{
						auto* Shape = Content->Load<Compute::UnmanagedShape>(Path, nullptr);
						if (Shape != nullptr)
							Initialize(Shape->Shape, Mass, CcdMotionThreshold);
					}
					else if (!NMake::Unpack(CV->Find("type"), &Type))
					{
						std::vector<Compute::Vector3> Vertices;
						if (NMake::Unpack(CV->Find("data"), &Vertices))
						{
							btCollisionShape* Shape = Parent->GetScene()->GetSimulator()->CreateConvexHull(Vertices);
							if (Shape != nullptr)
								Initialize(Shape, Mass, CcdMotionThreshold);
						}
					}
					else
					{
						btCollisionShape* Shape = Parent->GetScene()->GetSimulator()->CreateShape((Compute::Shape)Type);
						if (Shape != nullptr)
							Initialize(Shape, Mass, CcdMotionThreshold);
					}
				}

				if (!Instance)
					return;

				uint64_t ActivationState;
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

				Compute::Vector3 AngularFactor;
				if (NMake::Unpack(Node->Find("angular-factor"), &AngularFactor))
					Instance->SetAngularFactor(AngularFactor);

				Compute::Vector3 AngularVelocity;
				if (NMake::Unpack(Node->Find("angular-velocity"), &AngularVelocity))
					Instance->SetAngularVelocity(AngularVelocity);

				Compute::Vector3 AnisotropicFriction;
				if (NMake::Unpack(Node->Find("anisotropic-friction"), &AnisotropicFriction))
					Instance->SetAnisotropicFriction(AnisotropicFriction);

				Compute::Vector3 Gravity;
				if (NMake::Unpack(Node->Find("gravity"), &Gravity))
					Instance->SetGravity(Gravity);

				Compute::Vector3 LinearFactor;
				if (NMake::Unpack(Node->Find("linear-factor"), &LinearFactor))
					Instance->SetLinearFactor(LinearFactor);

				Compute::Vector3 LinearVelocity;
				if (NMake::Unpack(Node->Find("linear-velocity"), &LinearVelocity))
					Instance->SetLinearVelocity(LinearVelocity);

				uint64_t CollisionFlags;
				if (NMake::Unpack(Node->Find("collision-flags"), &CollisionFlags))
					Instance->SetCollisionFlags(CollisionFlags);
			}
			void RigidBody::OnSave(ContentManager* Content, Rest::Document* Node)
			{
				NMake::Pack(Node->SetDocument("kinematic"), Kinematic);
				NMake::Pack(Node->SetDocument("extended"), Instance != nullptr);
				if (!Instance)
					return;

				Rest::Document* CV = Node->SetDocument("shape");
				if (Instance->GetCollisionShapeType() == Compute::Shape_Convex_Hull)
				{
					AssetResource* Asset = Content->FindAsset(Hull);
					if (!Asset || !Hull)
					{
						std::vector<Compute::Vector3> Vertices = Parent->GetScene()->GetSimulator()->GetShapeVertices(Instance->GetCollisionShape());
						NMake::Pack(CV->SetDocument("data"), Vertices);
					}
					else
						NMake::Pack(CV->SetDocument("path"), Asset->Path);
				}
				else
					NMake::Pack(CV->SetDocument("type"), (uint64_t)Instance->GetCollisionShapeType());

				NMake::Pack(Node->SetDocument("mass"), Instance->GetMass());
				NMake::Pack(Node->SetDocument("ccd-motion-threshold"), Instance->GetCcdMotionThreshold());
				NMake::Pack(Node->SetDocument("activation-state"), (uint64_t)Instance->GetActivationState());
				NMake::Pack(Node->SetDocument("angular-damping"), Instance->GetAngularDamping());
				NMake::Pack(Node->SetDocument("angular-sleeping-threshold"), Instance->GetAngularSleepingThreshold());
				NMake::Pack(Node->SetDocument("friction"), Instance->GetFriction());
				NMake::Pack(Node->SetDocument("restitution"), Instance->GetRestitution());
				NMake::Pack(Node->SetDocument("hit-fraction"), Instance->GetHitFraction());
				NMake::Pack(Node->SetDocument("linear-damping"), Instance->GetLinearDamping());
				NMake::Pack(Node->SetDocument("linear-sleeping-threshold"), Instance->GetLinearSleepingThreshold());
				NMake::Pack(Node->SetDocument("ccd-swept-sphere-radius"), Instance->GetCcdSweptSphereRadius());
				NMake::Pack(Node->SetDocument("contact-processing-threshold"), Instance->GetContactProcessingThreshold());
				NMake::Pack(Node->SetDocument("deactivation-time"), Instance->GetDeactivationTime());
				NMake::Pack(Node->SetDocument("rolling-friction"), Instance->GetRollingFriction());
				NMake::Pack(Node->SetDocument("spinning-friction"), Instance->GetSpinningFriction());
				NMake::Pack(Node->SetDocument("contact-stiffness"), Instance->GetContactStiffness());
				NMake::Pack(Node->SetDocument("contact-damping"), Instance->GetContactDamping());
				NMake::Pack(Node->SetDocument("angular-factor"), Instance->GetAngularFactor());
				NMake::Pack(Node->SetDocument("angular-velocity"), Instance->GetAngularVelocity());
				NMake::Pack(Node->SetDocument("anisotropic-friction"), Instance->GetAnisotropicFriction());
				NMake::Pack(Node->SetDocument("gravity"), Instance->GetGravity());
				NMake::Pack(Node->SetDocument("linear-factor"), Instance->GetLinearFactor());
				NMake::Pack(Node->SetDocument("linear-velocity"), Instance->GetLinearVelocity());
				NMake::Pack(Node->SetDocument("collision-flags"), (uint64_t)Instance->GetCollisionFlags());
			}
			void RigidBody::OnSynchronize(Rest::Timer* Time)
			{
				if (Instance && Synchronize)
					Instance->Synchronize(Parent->Transform, Kinematic);
			}
			void RigidBody::OnAsleep()
			{
				if (Instance != nullptr)
					Instance->SetAsGhost();
			}
			void RigidBody::Initialize(btCollisionShape* Shape, float Mass, float Anticipation)
			{
				if (!Shape || !Parent || !Parent->GetScene() || !Parent->GetScene()->GetSimulator())
					return;

				Parent->GetScene()->Lock();
				delete Instance;

				Compute::RigidBody::Desc I;
				I.Anticipation = Anticipation;
				I.Mass = Mass;
				I.Shape = Shape;

				Instance = Parent->GetScene()->GetSimulator()->CreateRigidBody(I, Parent->Transform);
				Instance->UserPointer = this;
				Instance->SetActivity(true);
				Parent->GetScene()->Unlock();
			}
			void RigidBody::Initialize(ContentManager* Content, const std::string& Path, float Mass, float Anticipation)
			{
				if (Content != nullptr)
				{
					Hull = Content->Load<Compute::UnmanagedShape>(Path, nullptr);
					if (Hull != nullptr)
						Initialize(Hull->Shape, Mass, Anticipation);
				}
			}
			void RigidBody::Clear()
			{
				if (!Instance || !Parent || !Parent->GetScene() || !Parent->GetScene()->GetSimulator())
					return;

				Parent->GetScene()->Lock();
				delete Instance;
				Instance = nullptr;
				Parent->GetScene()->Unlock();
			}
			void RigidBody::SetTransform(const Compute::Matrix4x4& World)
			{
				if (!Instance)
					return;

				if (Parent && Parent->GetScene())
					Parent->GetScene()->Lock();

				Parent->Transform->SetMatrix(World);
				Instance->Synchronize(Parent->Transform, true);
				Instance->SetActivity(true);

				if (Parent && Parent->GetScene())
					Parent->GetScene()->Unlock();
			}
			void RigidBody::SetTransform(bool Kinematics)
			{
				if (!Instance)
					return;

				if (Parent && Parent->GetScene())
					Parent->GetScene()->Lock();

				Instance->Synchronize(Parent->Transform, Kinematics);
				Instance->SetActivity(true);

				if (Parent && Parent->GetScene())
					Parent->GetScene()->Unlock();
			}
			void RigidBody::SetMass(float Mass)
			{
				if (!Parent || !Parent->GetScene() || !Instance)
					return;

				Parent->GetScene()->Lock();
				Instance->SetMass(Mass);
				Parent->GetScene()->Unlock();
			}
			Component* RigidBody::OnClone(Entity* New)
			{
				RigidBody* Target = new RigidBody(New);
				Target->Kinematic = Kinematic;

				if (Instance != nullptr)
				{
					Target->Instance = Instance->Copy();
					Target->Instance->UserPointer = Target;
				}

				return Target;
			}

			SoftBody::SoftBody(Entity* Ref) : Component(Ref)
			{
				Hull = nullptr;
				Instance = nullptr;
				Kinematic = false;
				Synchronize = true;
				Visibility = false;
			}
			SoftBody::~SoftBody()
			{
				delete Instance;
			}
			void SoftBody::OnLoad(ContentManager* Content, Rest::Document* Node)
			{
				std::string Path; bool Extended;
				NMake::Unpack(Node->Find("extended"), &Extended);
				NMake::Unpack(Node->Find("kinematic"), &Kinematic);
				NMake::Unpack(Node->Find("visibility"), &Visibility);
				NMake::Unpack(Node->Get("surface"), &Surface, Content);

				if (!Extended)
					return;

				float CcdMotionThreshold = 0;
				NMake::Unpack(Node->Find("ccd-motion-threshold"), &CcdMotionThreshold);

				Rest::Document* CV = nullptr;
				if ((CV = Node->Find("shape")) != nullptr)
				{
					if (NMake::Unpack(Node->Find("path"), &Path))
					{
						auto* Shape = Content->Load<Compute::UnmanagedShape>(Path, nullptr);
						if (Shape != nullptr)
							InitializeShape(Shape, CcdMotionThreshold);
					}
				}
				else if ((CV = Node->Find("ellipsoid")) != nullptr)
				{
					Compute::SoftBody::Desc::CV::SEllipsoid Shape;
					NMake::Unpack(CV->Get("center"), &Shape.Center);
					NMake::Unpack(CV->Get("radius"), &Shape.Radius);
					NMake::Unpack(CV->Get("count"), &Shape.Count);
					InitializeEllipsoid(Shape, CcdMotionThreshold);
				}
				else if ((CV = Node->Find("patch")) != nullptr)
				{
					Compute::SoftBody::Desc::CV::SPatch Shape;
					NMake::Unpack(CV->Get("corner-00"), &Shape.Corner00);
					NMake::Unpack(CV->Get("corner-00-fixed"), &Shape.Corner00Fixed);
					NMake::Unpack(CV->Get("corner-01"), &Shape.Corner01);
					NMake::Unpack(CV->Get("corner-01-fixed"), &Shape.Corner01Fixed);
					NMake::Unpack(CV->Get("corner-10"), &Shape.Corner10);
					NMake::Unpack(CV->Get("corner-10-fixed"), &Shape.Corner10Fixed);
					NMake::Unpack(CV->Get("corner-11"), &Shape.Corner11);
					NMake::Unpack(CV->Get("corner-11-fixed"), &Shape.Corner11Fixed);
					NMake::Unpack(CV->Get("count-x"), &Shape.CountX);
					NMake::Unpack(CV->Get("count-y"), &Shape.CountY);
					NMake::Unpack(CV->Get("diagonals"), &Shape.GenerateDiagonals);
					InitializePatch(Shape, CcdMotionThreshold);
				}
				else if ((CV = Node->Find("rope")) != nullptr)
				{
					Compute::SoftBody::Desc::CV::SRope Shape;
					NMake::Unpack(CV->Get("start"), &Shape.Start);
					NMake::Unpack(CV->Get("start-fixed"), &Shape.StartFixed);
					NMake::Unpack(CV->Get("end"), &Shape.End);
					NMake::Unpack(CV->Get("end-fixed"), &Shape.EndFixed);
					NMake::Unpack(CV->Get("count"), &Shape.Count);
					InitializeRope(Shape, CcdMotionThreshold);
				}

				if (!Instance)
					return;

				Rest::Document* Conf = Node->Get("config");
				if (Conf != nullptr)
				{
					Compute::SoftBody::Desc::SConfig I;
					NMake::Unpack(Conf->Get("vcf"), &I.VCF);
					NMake::Unpack(Conf->Get("dp"), &I.DP);
					NMake::Unpack(Conf->Get("dg"), &I.DG);
					NMake::Unpack(Conf->Get("lf"), &I.LF);
					NMake::Unpack(Conf->Get("pr"), &I.PR);
					NMake::Unpack(Conf->Get("vc"), &I.VC);
					NMake::Unpack(Conf->Get("df"), &I.DF);
					NMake::Unpack(Conf->Get("mt"), &I.MT);
					NMake::Unpack(Conf->Get("chr"), &I.CHR);
					NMake::Unpack(Conf->Get("khr"), &I.KHR);
					NMake::Unpack(Conf->Get("shr"), &I.SHR);
					NMake::Unpack(Conf->Get("ahr"), &I.AHR);
					NMake::Unpack(Conf->Get("srhr-cl"), &I.SRHR_CL);
					NMake::Unpack(Conf->Get("skhr-cl"), &I.SKHR_CL);
					NMake::Unpack(Conf->Get("sshr-cl"), &I.SSHR_CL);
					NMake::Unpack(Conf->Get("sr-splt-cl"), &I.SR_SPLT_CL);
					NMake::Unpack(Conf->Get("sk-splt-cl"), &I.SK_SPLT_CL);
					NMake::Unpack(Conf->Get("ss-splt-cl"), &I.SS_SPLT_CL);
					NMake::Unpack(Conf->Get("max-volume"), &I.MaxVolume);
					NMake::Unpack(Conf->Get("time-scale"), &I.TimeScale);
					NMake::Unpack(Conf->Get("drag"), &I.Drag);
					NMake::Unpack(Conf->Get("max-stress"), &I.MaxStress);
					NMake::Unpack(Conf->Get("constraints"), &I.Constraints);
					NMake::Unpack(Conf->Get("clusters"), &I.Clusters);
					NMake::Unpack(Conf->Get("v-it"), &I.VIterations);
					NMake::Unpack(Conf->Get("p-it"), &I.PIterations);
					NMake::Unpack(Conf->Get("d-it"), &I.DIterations);
					NMake::Unpack(Conf->Get("c-it"), &I.CIterations);
					NMake::Unpack(Conf->Get("collisions"), &I.Collisions);

					uint64_t AeroModel;
					if (NMake::Unpack(Conf->Get("aero-model"), &AeroModel))
						I.AeroModel = (Compute::SoftAeroModel)AeroModel;

					Instance->SetConfig(I);
				}

				uint64_t ActivationState;
				if (NMake::Unpack(Node->Find("activation-state"), &ActivationState))
					Instance->SetActivationState((Compute::MotionState)ActivationState);

				float Friction;
				if (NMake::Unpack(Node->Find("friction"), &Friction))
					Instance->SetFriction(Friction);

				float Restitution;
				if (NMake::Unpack(Node->Find("restitution"), &Restitution))
					Instance->SetRestitution(Restitution);

				float HitFraction;
				if (NMake::Unpack(Node->Find("hit-fraction"), &HitFraction))
					Instance->SetHitFraction(HitFraction);

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

				Compute::Vector3 AnisotropicFriction;
				if (NMake::Unpack(Node->Find("anisotropic-friction"), &AnisotropicFriction))
					Instance->SetAnisotropicFriction(AnisotropicFriction);

				Compute::Vector3 WindVelocity;
				if (NMake::Unpack(Node->Find("wind-velocity"), &WindVelocity))
					Instance->SetWindVelocity(WindVelocity);

				float TotalMass;
				if (NMake::Unpack(Node->Find("total-mass"), &TotalMass))
					Instance->SetTotalMass(TotalMass);

				float RestLengthScale;
				if (NMake::Unpack(Node->Find("rest-length-scale"), &RestLengthScale))
					Instance->SetRestLengthScale(RestLengthScale);
			}
			void SoftBody::OnSave(ContentManager* Content, Rest::Document* Node)
			{
				NMake::Pack(Node->SetDocument("kinematic"), Kinematic);
				NMake::Pack(Node->SetDocument("extended"), Instance != nullptr);
				NMake::Pack(Node->SetDocument("visibility"), Visibility);
				NMake::Pack(Node->SetDocument("surface"), Surface, Content);

				if (!Instance)
					return;

				Compute::SoftBody::Desc& I = Instance->GetInitialState();
				Rest::Document* Conf = Node->SetDocument("config");
				NMake::Pack(Conf->SetDocument("aero-model"), (uint64_t)I.Config.AeroModel);
				NMake::Pack(Conf->SetDocument("vcf"), I.Config.VCF);
				NMake::Pack(Conf->SetDocument("dp"), I.Config.DP);
				NMake::Pack(Conf->SetDocument("dg"), I.Config.DG);
				NMake::Pack(Conf->SetDocument("lf"), I.Config.LF);
				NMake::Pack(Conf->SetDocument("pr"), I.Config.PR);
				NMake::Pack(Conf->SetDocument("vc"), I.Config.VC);
				NMake::Pack(Conf->SetDocument("df"), I.Config.DF);
				NMake::Pack(Conf->SetDocument("mt"), I.Config.MT);
				NMake::Pack(Conf->SetDocument("chr"), I.Config.CHR);
				NMake::Pack(Conf->SetDocument("khr"), I.Config.KHR);
				NMake::Pack(Conf->SetDocument("shr"), I.Config.SHR);
				NMake::Pack(Conf->SetDocument("ahr"), I.Config.AHR);
				NMake::Pack(Conf->SetDocument("srhr-cl"), I.Config.SRHR_CL);
				NMake::Pack(Conf->SetDocument("skhr-cl"), I.Config.SKHR_CL);
				NMake::Pack(Conf->SetDocument("sshr-cl"), I.Config.SSHR_CL);
				NMake::Pack(Conf->SetDocument("sr-splt-cl"), I.Config.SR_SPLT_CL);
				NMake::Pack(Conf->SetDocument("sk-splt-cl"), I.Config.SK_SPLT_CL);
				NMake::Pack(Conf->SetDocument("ss-splt-cl"), I.Config.SS_SPLT_CL);
				NMake::Pack(Conf->SetDocument("max-volume"), I.Config.MaxVolume);
				NMake::Pack(Conf->SetDocument("time-scale"), I.Config.TimeScale);
				NMake::Pack(Conf->SetDocument("drag"), I.Config.Drag);
				NMake::Pack(Conf->SetDocument("max-stress"), I.Config.MaxStress);
				NMake::Pack(Conf->SetDocument("constraints"), I.Config.Constraints);
				NMake::Pack(Conf->SetDocument("clusters"), I.Config.Clusters);
				NMake::Pack(Conf->SetDocument("v-it"), I.Config.VIterations);
				NMake::Pack(Conf->SetDocument("p-it"), I.Config.PIterations);
				NMake::Pack(Conf->SetDocument("d-it"), I.Config.DIterations);
				NMake::Pack(Conf->SetDocument("c-it"), I.Config.CIterations);
				NMake::Pack(Conf->SetDocument("collisions"), I.Config.Collisions);

				auto& Desc = Instance->GetInitialState();
				if (Desc.Shape.Convex.Enabled)
				{
					Rest::Document* CV = Node->SetDocument("shape");
					if (Instance->GetCollisionShapeType() == Compute::Shape_Convex_Hull)
					{
						AssetResource* Asset = Content->FindAsset(Hull);
						if (Asset != nullptr && Hull != nullptr)
							NMake::Pack(CV->SetDocument("path"), Asset->Path);
					}
				}
				else if (Desc.Shape.Ellipsoid.Enabled)
				{
					Rest::Document* Shape = Node->SetDocument("ellipsoid");
					NMake::Pack(Shape->SetDocument("center"), Desc.Shape.Ellipsoid.Center);
					NMake::Pack(Shape->SetDocument("radius"), Desc.Shape.Ellipsoid.Radius);
					NMake::Pack(Shape->SetDocument("count"), Desc.Shape.Ellipsoid.Count);
				}
				else if (Desc.Shape.Patch.Enabled)
				{
					Rest::Document* Shape = Node->SetDocument("patch");
					NMake::Pack(Shape->SetDocument("corner-00"), Desc.Shape.Patch.Corner00);
					NMake::Pack(Shape->SetDocument("corner-00-fixed"), Desc.Shape.Patch.Corner00Fixed);
					NMake::Pack(Shape->SetDocument("corner-01"), Desc.Shape.Patch.Corner01);
					NMake::Pack(Shape->SetDocument("corner-01-fixed"), Desc.Shape.Patch.Corner01Fixed);
					NMake::Pack(Shape->SetDocument("corner-10"), Desc.Shape.Patch.Corner10);
					NMake::Pack(Shape->SetDocument("corner-10-fixed"), Desc.Shape.Patch.Corner10Fixed);
					NMake::Pack(Shape->SetDocument("corner-11"), Desc.Shape.Patch.Corner11);
					NMake::Pack(Shape->SetDocument("corner-11-fixed"), Desc.Shape.Patch.Corner11Fixed);
					NMake::Pack(Shape->SetDocument("count-x"), Desc.Shape.Patch.CountX);
					NMake::Pack(Shape->SetDocument("count-y"), Desc.Shape.Patch.CountY);
					NMake::Pack(Shape->SetDocument("diagonals"), Desc.Shape.Patch.GenerateDiagonals);
				}
				else if (Desc.Shape.Rope.Enabled)
				{
					Rest::Document* Shape = Node->SetDocument("rope");
					NMake::Pack(Shape->SetDocument("start"), Desc.Shape.Rope.Start);
					NMake::Pack(Shape->SetDocument("start-fixed"), Desc.Shape.Rope.StartFixed);
					NMake::Pack(Shape->SetDocument("end"), Desc.Shape.Rope.End);
					NMake::Pack(Shape->SetDocument("end-fixed"), Desc.Shape.Rope.EndFixed);
					NMake::Pack(Shape->SetDocument("count"), Desc.Shape.Rope.Count);
				}

				NMake::Pack(Node->SetDocument("ccd-motion-threshold"), Instance->GetCcdMotionThreshold());
				NMake::Pack(Node->SetDocument("activation-state"), (uint64_t)Instance->GetActivationState());
				NMake::Pack(Node->SetDocument("friction"), Instance->GetFriction());
				NMake::Pack(Node->SetDocument("restitution"), Instance->GetRestitution());
				NMake::Pack(Node->SetDocument("hit-fraction"), Instance->GetHitFraction());
				NMake::Pack(Node->SetDocument("ccd-swept-sphere-radius"), Instance->GetCcdSweptSphereRadius());
				NMake::Pack(Node->SetDocument("contact-processing-threshold"), Instance->GetContactProcessingThreshold());
				NMake::Pack(Node->SetDocument("deactivation-time"), Instance->GetDeactivationTime());
				NMake::Pack(Node->SetDocument("rolling-friction"), Instance->GetRollingFriction());
				NMake::Pack(Node->SetDocument("spinning-friction"), Instance->GetSpinningFriction());
				NMake::Pack(Node->SetDocument("contact-stiffness"), Instance->GetContactStiffness());
				NMake::Pack(Node->SetDocument("contact-damping"), Instance->GetContactDamping());
				NMake::Pack(Node->SetDocument("angular-velocity"), Instance->GetAngularVelocity());
				NMake::Pack(Node->SetDocument("anisotropic-friction"), Instance->GetAnisotropicFriction());
				NMake::Pack(Node->SetDocument("linear-velocity"), Instance->GetLinearVelocity());
				NMake::Pack(Node->SetDocument("collision-flags"), (uint64_t)Instance->GetCollisionFlags());
				NMake::Pack(Node->SetDocument("wind-velocity"), Instance->GetWindVelocity());
				NMake::Pack(Node->SetDocument("total-mass"), Instance->GetTotalMass());
				NMake::Pack(Node->SetDocument("rest-length-scale"), Instance->GetRestLengthScale());
			}
			void SoftBody::OnSynchronize(Rest::Timer* Time)
			{
				if (!Instance)
				{
					Visibility = false;
					return;
				}

				if (Synchronize)
					Instance->Synchronize(Parent->Transform, Kinematic);

				Viewer View = Parent->GetScene()->GetCameraViewer();
				if (Parent->Transform->Position.Distance(View.RawPosition) < View.ViewDistance + Parent->Transform->Scale.Length())
					Visibility = Compute::MathCommon::IsClipping(View.ViewProjection, Parent->Transform->GetWorldUnscaled(), 1.5f) == -1;
				else
					Visibility = false;

				if (!Visibility)
					return;

				Instance->Update(&Vertices);
				if (Indices.empty())
					Instance->Reindex(&Indices);
			}
			void SoftBody::OnAsleep()
			{
				if (Instance != nullptr)
					Instance->SetAsGhost();
			}
			void SoftBody::InitializeShape(Compute::UnmanagedShape* Shape, float Anticipation)
			{
				if (!Shape || !Parent || !Parent->GetScene() || !Parent->GetScene()->GetSimulator())
					return;

				Parent->GetScene()->Lock();
				delete Instance;

				Compute::SoftBody::Desc I;
				I.Anticipation = Anticipation;
				I.Shape.Convex.Hull = Shape;
				I.Shape.Convex.Enabled = true;

				Vertices = Shape->Vertices;
				Indices = Shape->Indices;

				Instance = Parent->GetScene()->GetSimulator()->CreateSoftBody(I, Parent->Transform);
				if (!Instance)
				{
					THAWK_ERROR("cannot create soft body");
					return;
				}

				Instance->UserPointer = this;
				Instance->SetActivity(true);
				Parent->GetScene()->Unlock();
			}
			void SoftBody::InitializeShape(ContentManager* Content, const std::string& Path, float Anticipation)
			{
				if (Content != nullptr)
				{
					Hull = Content->Load<Compute::UnmanagedShape>(Path, nullptr);
					if (Hull != nullptr)
						InitializeShape(Hull, Anticipation);
				}
			}
			void SoftBody::InitializeEllipsoid(const Compute::SoftBody::Desc::CV::SEllipsoid& Shape, float Anticipation)
			{
				if (!Parent || !Parent->GetScene() || !Parent->GetScene()->GetSimulator())
					return;

				Parent->GetScene()->Lock();
				delete Instance;

				Compute::SoftBody::Desc I;
				I.Anticipation = Anticipation;
				I.Shape.Ellipsoid = Shape;
				I.Shape.Ellipsoid.Enabled = true;

				Instance = Parent->GetScene()->GetSimulator()->CreateSoftBody(I, Parent->Transform);
				if (!Instance)
				{
					THAWK_ERROR("cannot create soft body");
					return;
				}

				Instance->UserPointer = this;
				Instance->SetActivity(true);
				Parent->GetScene()->Unlock();
			}
			void SoftBody::InitializePatch(const Compute::SoftBody::Desc::CV::SPatch& Shape, float Anticipation)
			{
				if (!Parent || !Parent->GetScene() || !Parent->GetScene()->GetSimulator())
					return;

				Parent->GetScene()->Lock();
				delete Instance;

				Compute::SoftBody::Desc I;
				I.Anticipation = Anticipation;
				I.Shape.Patch = Shape;
				I.Shape.Patch.Enabled = true;

				Instance = Parent->GetScene()->GetSimulator()->CreateSoftBody(I, Parent->Transform);
				if (!Instance)
				{
					THAWK_ERROR("cannot create soft body");
					return;
				}

				Instance->UserPointer = this;
				Instance->SetActivity(true);
				Parent->GetScene()->Unlock();
			}
			void SoftBody::InitializeRope(const Compute::SoftBody::Desc::CV::SRope& Shape, float Anticipation)
			{
				if (!Parent || !Parent->GetScene() || !Parent->GetScene()->GetSimulator())
					return;

				Parent->GetScene()->Lock();
				delete Instance;

				Compute::SoftBody::Desc I;
				I.Anticipation = Anticipation;
				I.Shape.Rope = Shape;
				I.Shape.Rope.Enabled = true;

				Instance = Parent->GetScene()->GetSimulator()->CreateSoftBody(I, Parent->Transform);
				if (!Instance)
				{
					THAWK_ERROR("cannot create soft body");
					return;
				}

				Instance->UserPointer = this;
				Instance->SetActivity(true);
				Parent->GetScene()->Unlock();
			}
			void SoftBody::Fill(Graphics::GraphicsDevice* Device, Graphics::ElementBuffer* IndexBuffer, Graphics::ElementBuffer* VertexBuffer)
			{
				Graphics::MappedSubresource Map;
				if (VertexBuffer != nullptr && !Vertices.empty())
				{
					VertexBuffer->Map(Device, Graphics::ResourceMap_Write_Discard, &Map);
					memcpy(Map.Pointer, (void*)Vertices.data(), Vertices.size() * sizeof(Compute::Vertex));
					VertexBuffer->Unmap(Device, &Map);
				}

				if (IndexBuffer != nullptr && !Indices.empty())
				{
					IndexBuffer->Map(Device, Graphics::ResourceMap_Write_Discard, &Map);
					memcpy(Map.Pointer, (void*)Indices.data(), Indices.size() * sizeof(int));
					IndexBuffer->Unmap(Device, &Map);
				}
			}
			void SoftBody::Clear()
			{
				if (!Instance || !Parent || !Parent->GetScene() || !Parent->GetScene()->GetSimulator())
					return;

				Parent->GetScene()->Lock();
				delete Instance;
				Instance = nullptr;
				Parent->GetScene()->Unlock();
			}
			void SoftBody::SetTransform(const Compute::Matrix4x4& World)
			{
				if (!Instance)
					return;

				if (Parent && Parent->GetScene())
					Parent->GetScene()->Lock();

				Parent->Transform->SetMatrix(World);
				Instance->Synchronize(Parent->Transform, true);
				Instance->SetActivity(true);

				if (Parent && Parent->GetScene())
					Parent->GetScene()->Unlock();
			}
			void SoftBody::SetTransform(bool Kinematics)
			{
				if (!Instance)
					return;

				if (Parent && Parent->GetScene())
					Parent->GetScene()->Lock();

				Instance->Synchronize(Parent->Transform, Kinematics);
				Instance->SetActivity(true);

				if (Parent && Parent->GetScene())
					Parent->GetScene()->Unlock();
			}
			Graphics::Material& SoftBody::GetMaterial()
			{
				if (Surface.Material >= Parent->GetScene()->GetMaterialCount())
					return Parent->GetScene()->GetMaterialStandartLit();

				return Parent->GetScene()->GetMaterial(Surface.Material);
			}
			Component* SoftBody::OnClone(Entity* New)
			{
				SoftBody* Target = new SoftBody(New);
				Target->Kinematic = Kinematic;

				if (Instance != nullptr)
				{
					Target->Instance = Instance->Copy();
					Target->Instance->UserPointer = Target;
				}

				return Target;
			}

			Acceleration::Acceleration(Entity* Ref) : Component(Ref)
			{
				AmplitudeVelocity = 0;
				AmplitudeTorque = 0;
				ConstantVelocity = 0;
				ConstantTorque = 0;
				ConstantCenter = 0;
				RigidBody = nullptr;
				Velocity = false;
			}
			void Acceleration::OnLoad(ContentManager* Content, Rest::Document* Node)
			{
				NMake::Unpack(Node->Find("amplitude-velocity"), &AmplitudeVelocity);
				NMake::Unpack(Node->Find("amplitude-torque"), &AmplitudeTorque);
				NMake::Unpack(Node->Find("constant-velocity"), &ConstantVelocity);
				NMake::Unpack(Node->Find("constant-torque"), &ConstantTorque);
				NMake::Unpack(Node->Find("constant-center"), &ConstantCenter);
				NMake::Unpack(Node->Find("velocity"), &Velocity);
			}
			void Acceleration::OnSave(ContentManager* Content, Rest::Document* Node)
			{
				NMake::Pack(Node->SetDocument("amplitude-velocity"), AmplitudeVelocity);
				NMake::Pack(Node->SetDocument("amplitude-torque"), AmplitudeTorque);
				NMake::Pack(Node->SetDocument("constant-velocity"), ConstantVelocity);
				NMake::Pack(Node->SetDocument("constant-torque"), ConstantTorque);
				NMake::Pack(Node->SetDocument("constant-center"), ConstantCenter);
				NMake::Pack(Node->SetDocument("velocity"), Velocity);
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
				if (!Velocity)
				{
					RigidBody->SetLinearVelocity(ConstantVelocity);
					RigidBody->SetAngularVelocity(ConstantTorque);
				}
				else
					RigidBody->Push(ConstantVelocity * DeltaTime, ConstantTorque * DeltaTime, ConstantCenter);

				Compute::Vector3 Force = RigidBody->GetLinearVelocity();
				Compute::Vector3 Torque = RigidBody->GetAngularVelocity();
				Compute::Vector3 ACT = ConstantTorque.Abs();
				Compute::Vector3 ACV = ConstantVelocity.Abs();

				if (AmplitudeVelocity.X > 0 && Force.X > AmplitudeVelocity.X)
					ConstantVelocity.X = -ACV.X;
				else if (AmplitudeVelocity.X > 0 && Force.X < -AmplitudeVelocity.X)
					ConstantVelocity.X = ACV.X;

				if (AmplitudeVelocity.Y > 0 && Force.Y > AmplitudeVelocity.Y)
					ConstantVelocity.Y = -ACV.Y;
				else if (AmplitudeVelocity.Y > 0 && Force.Y < -AmplitudeVelocity.Y)
					ConstantVelocity.Y = ACV.Y;

				if (AmplitudeVelocity.Z > 0 && Force.Z > AmplitudeVelocity.Z)
					ConstantVelocity.Z = -ACV.Z;
				else if (AmplitudeVelocity.Z > 0 && Force.Z < -AmplitudeVelocity.Z)
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
			Component* Acceleration::OnClone(Entity* New)
			{
				Acceleration* Target = new Acceleration(New);
				Target->Velocity = Velocity;
				Target->AmplitudeTorque = AmplitudeTorque;
				Target->AmplitudeVelocity = AmplitudeVelocity;
				Target->ConstantCenter = ConstantCenter;
				Target->ConstantTorque = ConstantTorque;
				Target->ConstantVelocity = ConstantVelocity;
				Target->RigidBody = RigidBody;

				return Target;
			}

			SliderConstraint::SliderConstraint(Entity* Ref) : Component(Ref), Instance(nullptr), Connection(nullptr)
			{
			}
			SliderConstraint::~SliderConstraint()
			{
				delete Instance;
			}
			void SliderConstraint::OnLoad(ContentManager* Content, Rest::Document* Node)
			{
				bool Extended;
				NMake::Unpack(Node->Find("extended"), &Extended);
				if (!Extended)
					return;

				uint64_t Index;
				if (NMake::Unpack(Node->Find("connection"), &Index))
				{
					if (Parent->GetScene()->HasEntity(Index) != -1)
						Connection = Parent->GetScene()->GetEntity(Index);
				}

				bool CollisionState = false, LinearPowerState = false;
				NMake::Unpack(Node->Find("collision-state"), &CollisionState);
				NMake::Unpack(Node->Find("linear-power-state"), &CollisionState);
				Initialize(CollisionState, LinearPowerState);

				if (!Instance)
					return;

				float AngularMotorVelocity;
				if (NMake::Unpack(Node->Find("angular-motor-velocity"), &AngularMotorVelocity))
					Instance->SetAngularMotorVelocity(AngularMotorVelocity);

				float LinearMotorVelocity;
				if (NMake::Unpack(Node->Find("linear-motor-velocity"), &LinearMotorVelocity))
					Instance->SetLinearMotorVelocity(LinearMotorVelocity);

				float UpperLinearLimit;
				if (NMake::Unpack(Node->Find("upper-linear-limit"), &UpperLinearLimit))
					Instance->SetUpperLinearLimit(UpperLinearLimit);

				float LowerLinearLimit;
				if (NMake::Unpack(Node->Find("lower-linear-limit"), &LowerLinearLimit))
					Instance->SetLowerLinearLimit(LowerLinearLimit);

				float AngularDampingDirection;
				if (NMake::Unpack(Node->Find("angular-damping-direction"), &AngularDampingDirection))
					Instance->SetAngularDampingDirection(AngularDampingDirection);

				float LinearDampingDirection;
				if (NMake::Unpack(Node->Find("linear-damping-direction"), &LinearDampingDirection))
					Instance->SetLinearDampingDirection(LinearDampingDirection);

				float AngularDampingLimit;
				if (NMake::Unpack(Node->Find("angular-damping-limit"), &AngularDampingLimit))
					Instance->SetAngularDampingLimit(AngularDampingLimit);

				float LinearDampingLimit;
				if (NMake::Unpack(Node->Find("linear-damping-limit"), &LinearDampingLimit))
					Instance->SetLinearDampingLimit(LinearDampingLimit);

				float AngularDampingOrtho;
				if (NMake::Unpack(Node->Find("angular-damping-ortho"), &AngularDampingOrtho))
					Instance->SetAngularDampingOrtho(AngularDampingOrtho);

				float LinearDampingOrtho;
				if (NMake::Unpack(Node->Find("linear-damping-ortho"), &LinearDampingOrtho))
					Instance->SetLinearDampingOrtho(LinearDampingOrtho);

				float UpperAngularLimit;
				if (NMake::Unpack(Node->Find("upper-angular-limit"), &UpperAngularLimit))
					Instance->SetUpperAngularLimit(UpperAngularLimit);

				float LowerAngularLimit;
				if (NMake::Unpack(Node->Find("lower-angular-limit"), &LowerAngularLimit))
					Instance->SetLowerAngularLimit(LowerAngularLimit);

				float MaxAngularMotorForce;
				if (NMake::Unpack(Node->Find("max-angular-motor-force"), &MaxAngularMotorForce))
					Instance->SetMaxAngularMotorForce(MaxAngularMotorForce);

				float MaxLinearMotorForce;
				if (NMake::Unpack(Node->Find("max-linear-motor-force"), &MaxLinearMotorForce))
					Instance->SetMaxLinearMotorForce(MaxLinearMotorForce);

				float AngularRestitutionDirection;
				if (NMake::Unpack(Node->Find("angular-restitution-direction"), &AngularRestitutionDirection))
					Instance->SetAngularRestitutionDirection(AngularRestitutionDirection);

				float LinearRestitutionDirection;
				if (NMake::Unpack(Node->Find("linear-restitution-direction"), &LinearRestitutionDirection))
					Instance->SetLinearRestitutionDirection(LinearRestitutionDirection);

				float AngularRestitutionLimit;
				if (NMake::Unpack(Node->Find("angular-restitution-limit"), &AngularRestitutionLimit))
					Instance->SetAngularRestitutionLimit(AngularRestitutionLimit);

				float LinearRestitutionLimit;
				if (NMake::Unpack(Node->Find("linear-restitution-limit"), &LinearRestitutionLimit))
					Instance->SetLinearRestitutionLimit(LinearRestitutionLimit);

				float AngularRestitutionOrtho;
				if (NMake::Unpack(Node->Find("angular-restitution-ortho"), &AngularRestitutionOrtho))
					Instance->SetAngularRestitutionOrtho(AngularRestitutionOrtho);

				float LinearRestitutionOrtho;
				if (NMake::Unpack(Node->Find("linear-restitution-ortho"), &LinearRestitutionOrtho))
					Instance->SetLinearRestitutionOrtho(LinearRestitutionOrtho);

				float AngularSoftnessDirection;
				if (NMake::Unpack(Node->Find("angular-softness-direction"), &AngularSoftnessDirection))
					Instance->SetAngularSoftnessDirection(AngularSoftnessDirection);

				float LinearSoftnessDirection;
				if (NMake::Unpack(Node->Find("linear-softness-direction"), &LinearSoftnessDirection))
					Instance->SetLinearSoftnessDirection(LinearSoftnessDirection);

				float AngularSoftnessLimit;
				if (NMake::Unpack(Node->Find("angular-softness-limit"), &AngularSoftnessLimit))
					Instance->SetAngularSoftnessLimit(AngularSoftnessLimit);

				float LinearSoftnessLimit;
				if (NMake::Unpack(Node->Find("linear-softness-limit"), &LinearSoftnessLimit))
					Instance->SetLinearSoftnessLimit(LinearSoftnessLimit);

				float AngularSoftnessOrtho;
				if (NMake::Unpack(Node->Find("angular-softness-ortho"), &AngularSoftnessOrtho))
					Instance->SetAngularSoftnessOrtho(AngularSoftnessOrtho);

				float LinearSoftnessOrtho;
				if (NMake::Unpack(Node->Find("linear-softness-ortho"), &LinearSoftnessOrtho))
					Instance->SetLinearSoftnessOrtho(LinearSoftnessOrtho);

				bool PoweredAngularMotor;
				if (NMake::Unpack(Node->Find("powered-angular-motor"), &PoweredAngularMotor))
					Instance->SetPoweredAngularMotor(PoweredAngularMotor);

				bool PoweredLinearMotor;
				if (NMake::Unpack(Node->Find("powered-linear-motor"), &PoweredLinearMotor))
					Instance->SetPoweredLinearMotor(PoweredLinearMotor);

				bool Enabled;
				if (NMake::Unpack(Node->Find("enabled"), &Enabled))
					Instance->SetEnabled(Enabled);
			}
			void SliderConstraint::OnSave(ContentManager* Content, Rest::Document* Node)
			{
				NMake::Pack(Node->SetDocument("extended"), Instance != nullptr);
				if (!Instance)
					return;

				NMake::Pack(Node->SetDocument("collision-state"), Instance->GetInitialState().UseCollisions);
				NMake::Pack(Node->SetDocument("linear-power-state"), Instance->GetInitialState().UseLinearPower);
				NMake::Pack(Node->SetDocument("connection"), (uint64_t)(Connection ? Connection->Self : -1));
				NMake::Pack(Node->SetDocument("angular-motor-velocity"), Instance->GetAngularMotorVelocity());
				NMake::Pack(Node->SetDocument("linear-motor-velocity"), Instance->GetLinearMotorVelocity());
				NMake::Pack(Node->SetDocument("upper-linear-limit"), Instance->GetUpperLinearLimit());
				NMake::Pack(Node->SetDocument("lower-linear-limit"), Instance->GetLowerLinearLimit());
				NMake::Pack(Node->SetDocument("breaking-impulse-threshold"), Instance->GetBreakingImpulseThreshold());
				NMake::Pack(Node->SetDocument("angular-damping-direction"), Instance->GetAngularDampingDirection());
				NMake::Pack(Node->SetDocument("linear-amping-direction"), Instance->GetLinearDampingDirection());
				NMake::Pack(Node->SetDocument("angular-damping-limit"), Instance->GetAngularDampingLimit());
				NMake::Pack(Node->SetDocument("linear-damping-limit"), Instance->GetLinearDampingLimit());
				NMake::Pack(Node->SetDocument("angular-damping-ortho"), Instance->GetAngularDampingOrtho());
				NMake::Pack(Node->SetDocument("linear-damping-ortho"), Instance->GetLinearDampingOrtho());
				NMake::Pack(Node->SetDocument("upper-angular-limit"), Instance->GetUpperAngularLimit());
				NMake::Pack(Node->SetDocument("lower-angular-limit"), Instance->GetLowerAngularLimit());
				NMake::Pack(Node->SetDocument("max-angular-motor-force"), Instance->GetMaxAngularMotorForce());
				NMake::Pack(Node->SetDocument("max-linear-motor-force"), Instance->GetMaxLinearMotorForce());
				NMake::Pack(Node->SetDocument("angular-restitution-direction"), Instance->GetAngularRestitutionDirection());
				NMake::Pack(Node->SetDocument("linear-restitution-direction"), Instance->GetLinearRestitutionDirection());
				NMake::Pack(Node->SetDocument("angular-restitution-limit"), Instance->GetAngularRestitutionLimit());
				NMake::Pack(Node->SetDocument("linear-restitution-limit"), Instance->GetLinearRestitutionLimit());
				NMake::Pack(Node->SetDocument("angular-restitution-ortho"), Instance->GetAngularRestitutionOrtho());
				NMake::Pack(Node->SetDocument("linear-restitution-ortho"), Instance->GetLinearRestitutionOrtho());
				NMake::Pack(Node->SetDocument("angular-softness-direction"), Instance->GetAngularSoftnessDirection());
				NMake::Pack(Node->SetDocument("linear-softness-direction"), Instance->GetLinearSoftnessDirection());
				NMake::Pack(Node->SetDocument("angular-softness-limit"), Instance->GetAngularSoftnessLimit());
				NMake::Pack(Node->SetDocument("linear-softness-limit"), Instance->GetLinearSoftnessLimit());
				NMake::Pack(Node->SetDocument("angular-softness-ortho"), Instance->GetAngularSoftnessOrtho());
				NMake::Pack(Node->SetDocument("linear-softness-ortho"), Instance->GetLinearSoftnessOrtho());
				NMake::Pack(Node->SetDocument("powered-angular-motor"), Instance->GetPoweredAngularMotor());
				NMake::Pack(Node->SetDocument("powered-linear-motor"), Instance->GetPoweredLinearMotor());
				NMake::Pack(Node->SetDocument("enabled"), Instance->IsEnabled());
			}
			void SliderConstraint::Initialize(bool IsGhosted, bool IsLinear)
			{
				if (!Parent || !Parent->GetScene())
					return;

				Parent->GetScene()->Lock();
				delete Instance;

				if (!Connection)
					return Parent->GetScene()->Unlock();

				RigidBody* FirstBody = Parent->GetComponent<RigidBody>();
				RigidBody* SecondBody = Connection->GetComponent<RigidBody>();
				if (!FirstBody || !SecondBody)
					return Parent->GetScene()->Unlock();

				Compute::SliderConstraint::Desc I;
				I.Target1 = FirstBody->Instance;
				I.Target2 = SecondBody->Instance;
				I.UseCollisions = !IsGhosted;
				I.UseLinearPower = IsLinear;

				Instance = Parent->GetScene()->GetSimulator()->CreateSliderConstraint(I);
				Parent->GetScene()->Unlock();
			}
			void SliderConstraint::Clear()
			{
				if (!Instance || !Parent || !Parent->GetScene() || !Parent->GetScene()->GetSimulator())
					return;

				Parent->GetScene()->Lock();
				delete Instance;
				Instance = nullptr;
				Parent->GetScene()->Unlock();
			}
			Component* SliderConstraint::OnClone(Entity* New)
			{
				SliderConstraint* Target = new SliderConstraint(New);
				Target->Connection = Connection;

				if (!Instance)
					return Target;

				RigidBody* FirstBody = New->GetComponent<RigidBody>();
				if (!FirstBody)
					FirstBody = Parent->GetComponent<RigidBody>();

				if (!FirstBody)
					return Target;

				Compute::SliderConstraint::Desc I(Instance->GetInitialState());
				Instance->GetInitialState().Target1 = FirstBody->Instance;
				Target->Instance = Instance->Copy();
				Instance->GetInitialState() = I;

				return Target;
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
					Source->Apply(Content->Load<Audio::AudioClip>(Path, nullptr));

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
				if (NMake::Unpack(Node->Find("autoplay"), &Autoplay) && Autoplay && Source->GetClip())
					Source->Play();

				ApplyPlayingPosition();
				OnSynchronize(nullptr);
			}
			void AudioSource::OnSave(ContentManager* Content, Rest::Document* Node)
			{
				AssetResource* Asset = Content->FindAsset(Source->GetClip());
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
				if (!Source->GetClip())
					return;

				if (Relative)
					Audio::AudioContext::SetSourceData3F(Source->GetInstance(), Audio::SoundEx_Position, 0, 0, 0);
				else
					Audio::AudioContext::SetSourceData3F(Source->GetInstance(), Audio::SoundEx_Position, -Parent->Transform->Position.X, -Parent->Transform->Position.Y, Parent->Transform->Position.Z);

				Audio::AudioContext::SetSourceData3F(Source->GetInstance(), Audio::SoundEx_Velocity, Velocity.X, Velocity.Y, Velocity.Z);
				Audio::AudioContext::SetSourceData3F(Source->GetInstance(), Audio::SoundEx_Direction, Direction.X, Direction.Y, Direction.Z);
				Audio::AudioContext::SetSourceData1I(Source->GetInstance(), Audio::SoundEx_Source_Relative, Relative);
				Audio::AudioContext::SetSourceData1I(Source->GetInstance(), Audio::SoundEx_Looping, Loop);
				Audio::AudioContext::SetSourceData1F(Source->GetInstance(), Audio::SoundEx_Pitch, Pitch);
				Audio::AudioContext::SetSourceData1F(Source->GetInstance(), Audio::SoundEx_Gain, Gain);
				Audio::AudioContext::SetSourceData1F(Source->GetInstance(), Audio::SoundEx_Max_Distance, Distance);
				Audio::AudioContext::SetSourceData1F(Source->GetInstance(), Audio::SoundEx_Reference_Distance, RefDistance);
				Audio::AudioContext::SetSourceData1F(Source->GetInstance(), Audio::SoundEx_Rolloff_Factor, Rolloff);
				Audio::AudioContext::SetSourceData1F(Source->GetInstance(), Audio::SoundEx_Cone_Inner_Angle, ConeInnerAngle);
				Audio::AudioContext::SetSourceData1F(Source->GetInstance(), Audio::SoundEx_Cone_Outer_Angle, ConeOuterAngle);
				Audio::AudioContext::SetSourceData1F(Source->GetInstance(), Audio::SoundEx_Cone_Outer_Gain, ConeOuterGain);
				Audio::AudioContext::GetSourceData1F(Source->GetInstance(), Audio::SoundEx_Seconds_Offset, &Position);
			}
			void AudioSource::ApplyPlayingPosition()
			{
				Audio::AudioContext::SetSourceData1F(Source->GetInstance(), Audio::SoundEx_Seconds_Offset, Position);
			}
			Component* AudioSource::OnClone(Entity* New)
			{
				AudioSource* Target = new AudioSource(New);
				Target->Distance = Distance;
				Target->Gain = Gain;
				Target->Loop = Loop;
				Target->Pitch = Pitch;
				Target->Position = Position;
				Target->RefDistance = RefDistance;
				Target->Relative = Relative;
				Target->Source->Apply(Source->GetClip());

				return Target;
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
			Component* AudioListener::OnClone(Entity* New)
			{
				AudioListener* Target = new AudioListener(New);
				Target->Velocity = Velocity;
				Target->Gain = Gain;

				return Target;
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
				Components::SkinModel* Model = Parent->GetComponent<Components::SkinModel>();
				if (Model != nullptr && Model->Instance != nullptr)
				{
					Instance = Model;
					Instance->Skeleton.ResetKeys(Instance->Instance, &Default);
				}
				else
					Instance = nullptr;

				SetActive(Instance != nullptr);
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
			void SkinAnimator::BlendAnimation(int64_t Clip, int64_t Frame_)
			{
				State.Blended = true;
				State.Time = 0.0f;
				State.Frame = Frame_;
				State.Clip = Clip;

				if (State.Clip >= 0 && State.Clip < Clips.size())
				{
					Compute::SkinAnimatorClip* CurrentClip = &Clips[State.Clip];
					if (State.Frame < 0 || State.Frame >= CurrentClip->Keys.size())
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
			void SkinAnimator::Play(int64_t Clip, int64_t Frame_)
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
					Compute::SkinAnimatorClip* CurrentClip = &Clips[State.Clip];
					if (State.Frame < 0 || State.Frame >= CurrentClip->Keys.size())
						State.Frame = -1;
				}
				else
					State.Clip = -1;

				SavePose();
				Current = Bind;
				if (!IsPosed(State.Clip, State.Frame))
					BlendAnimation(State.Clip, State.Frame);
			}
			bool SkinAnimator::IsPosed(int64_t Clip, int64_t Frame_)
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
			std::vector<Compute::AnimatorKey>* SkinAnimator::GetFrame(int64_t Clip, int64_t Frame)
			{
				if (Clip < 0 || Clip >= Clips.size() || Frame < 0 || Frame >= Clips[Clip].Keys.size())
					return nullptr;

				return &Clips[Clip].Keys[Frame];
			}
			std::vector<std::vector<Compute::AnimatorKey>>* SkinAnimator::GetClip(int64_t Clip)
			{
				if (Clip < 0 || Clip >= Clips.size())
					return nullptr;

				return &Clips[Clip].Keys;
			}
			Component* SkinAnimator::OnClone(Entity* New)
			{
				SkinAnimator* Target = new SkinAnimator(New);
				Target->Clips = Clips;
				Target->State = State;
				Target->Bind = Bind;
				Target->Current = Current;

				return Target;
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
			void KeyAnimator::BlendAnimation(int64_t Clip, int64_t Frame_)
			{
				State.Blended = true;
				State.Time = 0.0f;
				State.Frame = Frame_;
				State.Clip = Clip;

				if (State.Clip >= 0 && State.Clip < Clips.size())
				{
					Compute::KeyAnimatorClip* CurrentClip = &Clips[State.Clip];
					if (State.Frame < 0 || State.Frame >= CurrentClip->Keys.size())
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
			void KeyAnimator::Play(int64_t Clip, int64_t Frame_)
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
					Compute::KeyAnimatorClip* CurrentClip = &Clips[State.Clip];
					if (State.Frame < 0 || State.Frame >= CurrentClip->Keys.size())
						State.Frame = -1;
				}
				else
					State.Clip = -1;

				SavePose();
				Current = Bind;
				if (!IsPosed(State.Clip, State.Frame))
					BlendAnimation(State.Clip, State.Frame);
			}
			bool KeyAnimator::IsPosed(int64_t Clip, int64_t Frame_)
			{
				Compute::AnimatorKey* Key = GetFrame(Clip, Frame_);
				if (!Key)
					Key = &Bind;

				return *Parent->Transform->GetLocalPosition() == Key->Position && *Parent->Transform->GetLocalRotation() == Key->Rotation && *Parent->Transform->GetLocalScale() == Key->Scale;
			}
			Compute::AnimatorKey* KeyAnimator::GetFrame(int64_t Clip, int64_t Frame)
			{
				if (Clip < 0 || Clip >= Clips.size() || Frame < 0 || Frame >= Clips[Clip].Keys.size())
					return nullptr;

				return &Clips[Clip].Keys[Frame];
			}
			std::vector<Compute::AnimatorKey>* KeyAnimator::GetClip(int64_t Clip)
			{
				if (Clip < 0 || Clip >= Clips.size())
					return nullptr;

				return &Clips[Clip].Keys;
			}
			Component* KeyAnimator::OnClone(Entity* New)
			{
				KeyAnimator* Target = new KeyAnimator(New);
				Target->Clips = Clips;
				Target->State = State;
				Target->Bind = Bind;
				Target->Current = Current;

				return Target;
			}

			ElementSystem::ElementSystem(Entity* Ref) : Component(Ref)
			{
				Instance = nullptr;
				Connected = false;
				QuadBased = false;
				Visibility = 0.0f;
				Volume = 3.0f;
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
				NMake::Unpack(Node->Find("surface"), &Surface, Content);
				NMake::Unpack(Node->Find("quad-based"), &QuadBased);
				NMake::Unpack(Node->Find("connected"), &Connected);
				NMake::Unpack(Node->Find("visibility"), &Visibility);
				NMake::Unpack(Node->Find("volume"), &Volume);

				uint64_t Limit;
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
				NMake::Pack(Node->SetDocument("surface"), Surface, Content);
				NMake::Pack(Node->SetDocument("quad-based"), QuadBased);
				NMake::Pack(Node->SetDocument("connected"), Connected);
				NMake::Pack(Node->SetDocument("visibility"), Visibility);
				NMake::Pack(Node->SetDocument("volume"), Volume);

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
					if (Value->Get<Graphics::Material>()->Self == (float)Surface.Material)
						Surface.Material = 0;
				}
			}
			Graphics::Material& ElementSystem::GetMaterial()
			{
				if (Surface.Material < 0 || Surface.Material >= (unsigned int)Parent->GetScene()->GetMaterialCount())
					return Parent->GetScene()->GetMaterialStandartLit();

				return Parent->GetScene()->GetMaterial((uint64_t)Surface.Material);
			}
			Component* ElementSystem::OnClone(Entity* New)
			{
				ElementSystem* Target = new ElementSystem(New);
				Target->Visibility = Visibility;
				Target->Volume = Volume;
				Target->Connected = Connected;
				Target->Surface = Surface;
				Target->Instance->GetArray()->Copy(*Instance->GetArray());

				return Target;
			}

			ElementAnimator::ElementAnimator(Entity* Ref) : Component(Ref)
			{
				Position = 0.0f;
				Diffuse = 0.0f;
				ScaleSpeed = 0.0f;
				RotationSpeed = 0.0f;
				Velocity = 0.0f;
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
				Simulate = false;
				Noisiness = 0.0f;
				System = nullptr;
			}
			void ElementAnimator::OnLoad(ContentManager* Content, Rest::Document* Node)
			{
				NMake::Unpack(Node->Find("diffuse"), &Diffuse);
				NMake::Unpack(Node->Find("position"), &Position);
				NMake::Unpack(Node->Find("velocity"), &Velocity);
				NMake::Unpack(Node->Find("spawner"), &Spawner);
				NMake::Unpack(Node->Find("noisiness"), &Noisiness);
				NMake::Unpack(Node->Find("rotation-speed"), &RotationSpeed);
				NMake::Unpack(Node->Find("scale-speed"), &ScaleSpeed);
				NMake::Unpack(Node->Find("simulate"), &Simulate);
			}
			void ElementAnimator::OnSave(ContentManager* Content, Rest::Document* Node)
			{
				NMake::Pack(Node->SetDocument("diffuse"), Diffuse);
				NMake::Pack(Node->SetDocument("position"), Position);
				NMake::Pack(Node->SetDocument("velocity"), Velocity);
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
					if (Array->Size() >= Array->Capacity())
						continue;

					Compute::Vector3 FPosition = (System->Connected ? Spawner.Position.Generate() : Spawner.Position.Generate() + Parent->Transform->Position.InvertZ());
					Compute::Vector3 FVelocity = Spawner.Velocity.Generate();
					Compute::Vector4 FDiffusion = Spawner.Diffusion.Generate();

					Compute::ElementVertex Element;
					Element.PositionX = FPosition.X;
					Element.PositionY = FPosition.Y;
					Element.PositionZ = FPosition.Z;
					Element.VelocityX = FVelocity.X;
					Element.VelocityY = FVelocity.Y;
					Element.VelocityZ = FVelocity.Z;
					Element.ColorX = FDiffusion.X;
					Element.ColorY = FDiffusion.Y;
					Element.ColorZ = FDiffusion.Z;
					Element.ColorW = FDiffusion.W;
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
				float L = Velocity.Length();

				for (auto It = Array->Begin(); It != Array->End(); It++)
				{
					Compute::Vector3 Noise = Spawner.Noise.Generate() / Noisiness;
					It->PositionX += (It->VelocityX + Position.X + Noise.X) * DeltaTime;
					It->PositionY += (It->VelocityY + Position.Y + Noise.Y) * DeltaTime;
					It->PositionZ += (It->VelocityZ + Position.Z + Noise.Z) * DeltaTime;
					It->ColorX += Diffuse.X * DeltaTime;
					It->ColorY += Diffuse.Y * DeltaTime;
					It->ColorZ += Diffuse.Z * DeltaTime;
					It->ColorW += Diffuse.W * DeltaTime;
					It->Scale += ScaleSpeed * DeltaTime;
					It->Rotation += (It->Angular + RotationSpeed) * DeltaTime;

					if (L > 0)
					{
						It->VelocityX -= (It->VelocityX / Velocity.X) * DeltaTime;
						It->VelocityY -= (It->VelocityY / Velocity.Y) * DeltaTime;
						It->VelocityZ -= (It->VelocityZ / Velocity.Z) * DeltaTime;
					}

					if (It->ColorW <= 0 || It->Scale <= 0)
					{
						Array->RemoveAt(It);
						It--;
					}
				}
			}
			void ElementAnimator::FastSynchronization(float DeltaTime)
			{
				Rest::Pool<Compute::ElementVertex>* Array = System->Instance->GetArray();
				float L = Velocity.Length();

				for (auto It = Array->Begin(); It != Array->End(); It++)
				{
					It->PositionX += (It->VelocityX + Position.X) * DeltaTime;
					It->PositionY += (It->VelocityY + Position.Y) * DeltaTime;
					It->PositionZ += (It->VelocityZ + Position.Z) * DeltaTime;
					It->ColorX += Diffuse.X * DeltaTime;
					It->ColorY += Diffuse.Y * DeltaTime;
					It->ColorZ += Diffuse.Z * DeltaTime;
					It->ColorW += Diffuse.W * DeltaTime;
					It->Scale += ScaleSpeed * DeltaTime;
					It->Rotation += (It->Angular + RotationSpeed) * DeltaTime;

					if (L > 0)
					{
						It->VelocityX -= (It->VelocityX / Velocity.X) * DeltaTime;
						It->VelocityY -= (It->VelocityY / Velocity.Y) * DeltaTime;
						It->VelocityZ -= (It->VelocityZ / Velocity.Z) * DeltaTime;
					}

					if (It->ColorW <= 0 || It->Scale <= 0)
					{
						Array->RemoveAt(It);
						It--;
					}
				}
			}
			Component* ElementAnimator::OnClone(Entity* New)
			{
				ElementAnimator* Target = new ElementAnimator(New);
				Target->Diffuse = Diffuse;
				Target->Position = Position;
				Target->Velocity = Velocity;
				Target->ScaleSpeed = ScaleSpeed;
				Target->RotationSpeed = RotationSpeed;
				Target->Spawner = Spawner;
				Target->Noisiness = Noisiness;
				Target->Simulate = Simulate;

				return Target;
			}

			FreeLook::FreeLook(Entity* Ref) : Component(Ref), Activity(nullptr), Rotate(Graphics::KeyCode_CURSORRIGHT), Sensitivity(0.005f)
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
			void FreeLook::OnUpdate(Rest::Timer* Time)
			{
				if (!Activity)
					return;

				if (Activity->IsKeyDown(Rotate))
				{
					Compute::Vector2 Cursor = Activity->GetCursorPosition();
					if (!Activity->IsKeyDownHit(Rotate))
					{
						float X = (Cursor.Y - Position.Y) * Sensitivity;
						float Y = (Cursor.X - Position.X) * Sensitivity;
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
				SpeedNormal = 1.2f;
				SpeedUp = 2.6f;
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
			void Fly::OnUpdate(Rest::Timer* Time)
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
					Instance = Content->Load<Graphics::Model>(Path, nullptr);

				std::vector<Rest::Document*> Faces = Node->FindCollectionPath("surfaces.surface");
				for (auto&& Surface : Faces)
				{
					if (!Instance || !NMake::Unpack(Surface->Find("name"), &Path))
						continue;

					Graphics::Mesh* Ref = Instance->Find(Path);
					if (!Ref)
						continue;

					Appearance Face;
					if (NMake::Unpack(Surface, &Face, Content))
						Surfaces[Ref] = Face;
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
					if (It.first != nullptr)
					{
						NMake::Pack(Surface->SetDocument("name"), It.first->Name);
						NMake::Pack(Surface, It.second, Content);
					}
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
				if (Parent->Transform->Position.Distance(View.RawPosition) < View.ViewDistance + Parent->Transform->Scale.Length())
					Visibility = Compute::MathCommon::IsClipping(View.ViewProjection, GetBoundingBox(), 1.5f) == -1;
				else
					Visibility = false;
			}
			void Model::OnEvent(Event* Value)
			{
				if (!Value->Is<Graphics::Material>())
					return;

				uint64_t Material = (uint64_t)Value->Get<Graphics::Material>()->Self;
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
			Graphics::Material& Model::GetMaterial(Appearance* Surface)
			{
				if (!Surface || Surface->Material >= Parent->GetScene()->GetMaterialCount())
					return Parent->GetScene()->GetMaterialStandartLit();

				return Parent->GetScene()->GetMaterial(Surface->Material);
			}
			Appearance* Model::GetSurface(Graphics::Mesh* Mesh)
			{
				auto It = Surfaces.find(Mesh);
				if (It == Surfaces.end())
				{
					Surfaces[Mesh] = Appearance();
					It = Surfaces.find(Mesh);
				}

				return &It->second;
			}
			Compute::Matrix4x4 Model::GetBoundingBox()
			{
				if (!Instance)
					return Parent->Transform->GetWorld();

				return Compute::Matrix4x4::Create(Parent->Transform->Position, Parent->Transform->Scale * Compute::Vector3(Instance->Min.W - Instance->Max.W).Div(2.0f).Abs(), Parent->Transform->Rotation);
			}
			Component* Model::OnClone(Entity* New)
			{
				Model* Target = new Model(New);
				Target->Visibility = Visibility;
				Target->Instance = Instance;
				Target->Surfaces = Surfaces;

				return Target;
			}

			SkinModel::SkinModel(Entity* Ref) : Component(Ref)
			{
				Visibility = false;
				Instance = nullptr;
			}
			void SkinModel::OnLoad(ContentManager* Content, Rest::Document* Node)
			{
				std::string Path;
				if (NMake::Unpack(Node->Find("skin-model"), &Path))
					Instance = Content->Load<Graphics::SkinModel>(Path, nullptr);

				std::vector<Rest::Document*> Faces = Node->FindCollectionPath("surfaces.surface");
				for (auto&& Surface : Faces)
				{
					if (!Instance || !NMake::Unpack(Surface->Find("name"), &Path))
						continue;

					Graphics::SkinMesh* Ref = Instance->FindMesh(Path);
					if (!Ref)
						continue;

					Appearance Face;
					if (NMake::Unpack(Surface, &Face, Content))
						Surfaces[Ref] = Face;
				}

				std::vector<Rest::Document*> Poses = Node->FindCollectionPath("poses.pose");
				for (auto&& Pose : Poses)
				{
					int64_t Index;
					NMake::Unpack(Pose->Find("[index]"), &Index);

					Compute::Matrix4x4 Matrix;
					NMake::Unpack(Pose->Find("[matrix]"), &Matrix);

					Skeleton.Pose[Index] = Matrix;
				}

				NMake::Unpack(Node->Find("visibility"), &Visibility);
			}
			void SkinModel::OnSave(ContentManager* Content, Rest::Document* Node)
			{
				AssetResource* Asset = Content->FindAsset(Instance);
				if (Asset != nullptr)
					NMake::Pack(Node->SetDocument("skin-model"), Asset->Path);

				Rest::Document* Faces = Node->SetArray("surfaces");
				for (auto&& It : Surfaces)
				{
					Rest::Document* Surface = Faces->SetDocument("surface");
					if (It.first != nullptr)
					{
						NMake::Pack(Surface->SetDocument("name"), It.first->Name);
						NMake::Pack(Surface, It.second, Content);
					}
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
			void SkinModel::OnSynchronize(Rest::Timer* Time)
			{
				if (!Instance)
				{
					Visibility = false;
					return;
				}
				else
					Instance->BuildSkeleton(&Skeleton);

				Viewer View = Parent->GetScene()->GetCameraViewer();
				if (Parent->Transform->Position.Distance(View.RawPosition) < View.ViewDistance + Parent->Transform->Scale.Length())
					Visibility = Compute::MathCommon::IsClipping(View.ViewProjection, GetBoundingBox(), 1.5f) == -1;
				else
					Visibility = false;
			}
			void SkinModel::OnEvent(Event* Value)
			{
				if (!Value->Is<Graphics::Material>())
					return;

				uint64_t Material = (uint64_t)Value->Get<Graphics::Material>()->Self;
				for (auto&& Surface : Surfaces)
				{
					if (Surface.second.Material == Material)
						Surface.second.Material = 0;
				}
			}
			Graphics::Material& SkinModel::GetMaterial(Graphics::SkinMesh* Mesh)
			{
				return GetMaterial(GetSurface(Mesh));
			}
			Graphics::Material& SkinModel::GetMaterial(Appearance* Surface)
			{
				if (!Surface || Surface->Material >= Parent->GetScene()->GetMaterialCount())
					return Parent->GetScene()->GetMaterialStandartLit();

				return Parent->GetScene()->GetMaterial(Surface->Material);
			}
			Appearance* SkinModel::GetSurface(Graphics::SkinMesh* Mesh)
			{
				auto It = Surfaces.find(Mesh);
				if (It == Surfaces.end())
				{
					Surfaces[Mesh] = Appearance();
					It = Surfaces.find(Mesh);
				}

				return &It->second;
			}
			Compute::Matrix4x4 SkinModel::GetBoundingBox()
			{
				if (!Instance)
					return Parent->Transform->GetWorld();

				return Compute::Matrix4x4::Create(Parent->Transform->Position, Parent->Transform->Scale * Compute::Vector3(Instance->Min.W - Instance->Max.W).Div(2.0f).Abs(), Parent->Transform->Rotation);
			}
			Component* SkinModel::OnClone(Entity* New)
			{
				SkinModel* Target = new SkinModel(New);
				Target->Visibility = Visibility;
				Target->Instance = Instance;
				Target->Surfaces = Surfaces;

				return Target;
			}

			PointLight::PointLight(Entity* Ref) : Component(Ref)
			{
				Occlusion = nullptr;
				Diffuse = Compute::Vector3::One();
				Visibility = 0.0f;
				Emission = 1.0f;
				Range = 5.0f;
			}
			void PointLight::OnLoad(ContentManager* Content, Rest::Document* Node)
			{
				NMake::Unpack(Node->Find("diffuse"), &Diffuse);
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
				NMake::Pack(Node->SetDocument("diffuse"), Diffuse);
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
			Component* PointLight::OnClone(Entity* New)
			{
				PointLight* Target = new PointLight(New);
				Target->Diffuse = Diffuse;
				Target->Emission = Emission;
				Target->Visibility = Visibility;
				Target->Range = Range;
				Target->Projection = Projection;
				Target->ShadowBias = ShadowBias;
				Target->ShadowDistance = ShadowDistance;
				Target->ShadowIterations = ShadowIterations;
				Target->ShadowSoftness = ShadowSoftness;
				Target->Shadowed = Shadowed;
				Target->View = View;

				return Target;
			}

			SpotLight::SpotLight(Entity* Ref) : Component(Ref)
			{
				Diffuse = Compute::Vector3::One();
				Occlusion = nullptr;
				ProjectMap = nullptr;
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
				if (NMake::Unpack(Node->Find("project-map"), &Path))
					ProjectMap = Content->Load<Graphics::Texture2D>(Path, nullptr);

				NMake::Unpack(Node->Find("diffuse"), &Diffuse);
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
				AssetResource* Asset = Content->FindAsset(ProjectMap);
				if (Asset != nullptr)
					NMake::Pack(Node->SetDocument("project-map"), Asset->Path);

				NMake::Pack(Node->SetDocument("diffuse"), Diffuse);
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
			Component* SpotLight::OnClone(Entity* New)
			{
				SpotLight* Target = new SpotLight(New);
				Target->Diffuse = Diffuse;
				Target->Projection = Projection;
				Target->View = View;
				Target->ProjectMap = ProjectMap;
				Target->FieldOfView = FieldOfView;
				Target->Range = Range;
				Target->Emission = Emission;
				Target->Shadowed = Shadowed;
				Target->ShadowBias = ShadowBias;
				Target->ShadowDistance = ShadowDistance;
				Target->ShadowIterations = ShadowIterations;
				Target->ShadowSoftness = ShadowSoftness;

				return Target;
			}

			LineLight::LineLight(Entity* Ref) : Component(Ref)
			{
				Diffuse = Compute::Vector3(1.0, 0.8, 0.501961);
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
				NMake::Unpack(Node->Find("diffuse"), &Diffuse);
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
				NMake::Pack(Node->SetDocument("diffuse"), Diffuse);
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
			Component* LineLight::OnClone(Entity* New)
			{
				LineLight* Target = new LineLight(New);
				Target->Projection = Projection;
				Target->View = View;
				Target->Diffuse = Diffuse;
				Target->ShadowBias = ShadowBias;
				Target->ShadowDistance = ShadowDistance;
				Target->ShadowFarBias = ShadowFarBias;
				Target->ShadowSoftness = ShadowSoftness;
				Target->ShadowLength = ShadowLength;
				Target->ShadowHeight = ShadowHeight;
				Target->ShadowIterations = ShadowIterations;
				Target->Emission = Emission;
				Target->Shadowed = Shadowed;

				return Target;
			}

			ProbeLight::ProbeLight(Entity* Ref) : Component(Ref)
			{
				Projection = Compute::Matrix4x4::CreatePerspectiveRad(1.57079632679f, 1, 0.01f, 100.0f);
				Diffuse = Compute::Vector3::One();
				ViewOffset = Compute::Vector3(1, 1, -1);
				DiffuseMapX[0] = nullptr;
				DiffuseMapX[1] = nullptr;
				DiffuseMapY[0] = nullptr;
				DiffuseMapY[1] = nullptr;
				DiffuseMapZ[0] = nullptr;
				DiffuseMapZ[1] = nullptr;
				DiffuseMap = nullptr;
				Emission = 1.0f;
				Range = 5.0f;
				Visibility = 0.0f;
				CaptureRange = Range;
				ImageBased = false;
				ParallaxCorrected = false;
			}
			ProbeLight::~ProbeLight()
			{
				if (!ImageBased && DiffuseMap != nullptr)
					delete DiffuseMap;
			}
			void ProbeLight::OnLoad(ContentManager* Content, Rest::Document* Node)
			{
				std::string Path;
				if (NMake::Unpack(Node->Find("diffuse-map-px"), &Path))
					DiffuseMapX[0] = Content->Load<Graphics::Texture2D>(Path, nullptr);

				if (NMake::Unpack(Node->Find("diffuse-map-nx"), &Path))
					DiffuseMapX[1] = Content->Load<Graphics::Texture2D>(Path, nullptr);

				if (NMake::Unpack(Node->Find("diffuse-map-py"), &Path))
					DiffuseMapY[0] = Content->Load<Graphics::Texture2D>(Path, nullptr);

				if (NMake::Unpack(Node->Find("diffuse-map-ny"), &Path))
					DiffuseMapY[1]= Content->Load<Graphics::Texture2D>(Path, nullptr);

				if (NMake::Unpack(Node->Find("diffuse-map-pz"), &Path))
					DiffuseMapZ[0] = Content->Load<Graphics::Texture2D>(Path, nullptr);

				if (NMake::Unpack(Node->Find("diffuse-map-nz"), &Path))
					DiffuseMapZ[1] = Content->Load<Graphics::Texture2D>(Path, nullptr);

				std::vector<Compute::Matrix4x4> Views;
				NMake::Unpack(Node->Find("view"), &Views);
				NMake::Unpack(Node->Find("rebuild"), &Rebuild);
				NMake::Unpack(Node->Find("projection"), &Projection);
				NMake::Unpack(Node->Find("diffuse"), &Diffuse);
				NMake::Unpack(Node->Find("visibility"), &Visibility);
				NMake::Unpack(Node->Find("range"), &Range);
				NMake::Unpack(Node->Find("capture-range"), &CaptureRange);
				NMake::Unpack(Node->Find("emission"), &Emission);
				NMake::Unpack(Node->Find("image-based"), &ImageBased);
				NMake::Unpack(Node->Find("parallax-corrected"), &ParallaxCorrected);

				int64_t Count = Compute::Math<int64_t>::Min((int64_t)Views.size(), 6);
				for (int64_t i = 0; i < Count; i++)
					View[i] = Views[i];

				if (ImageBased)
					RebuildDiffuseMap();
			}
			void ProbeLight::OnSave(ContentManager* Content, Rest::Document* Node)
			{
				AssetResource* Asset = Content->FindAsset(DiffuseMapX[0]);
				if (Asset != nullptr)
					NMake::Pack(Node->SetDocument("diffuse-map-px"), Asset->Path);

				Asset = Content->FindAsset(DiffuseMapX[1]);
				if (Asset != nullptr)
					NMake::Pack(Node->SetDocument("diffuse-map-nx"), Asset->Path);

				Asset = Content->FindAsset(DiffuseMapY[0]);
				if (Asset != nullptr)
					NMake::Pack(Node->SetDocument("diffuse-map-py"), Asset->Path);

				Asset = Content->FindAsset(DiffuseMapY[1]);
				if (Asset != nullptr)
					NMake::Pack(Node->SetDocument("diffuse-map-ny"), Asset->Path);

				Asset = Content->FindAsset(DiffuseMapZ[0]);
				if (Asset != nullptr)
					NMake::Pack(Node->SetDocument("diffuse-map-pz"), Asset->Path);

				Asset = Content->FindAsset(DiffuseMapZ[1]);
				if (Asset != nullptr)
					NMake::Pack(Node->SetDocument("diffuse-map-nz"), Asset->Path);

				std::vector<Compute::Matrix4x4> Views;
				for (int64_t i = 0; i < 6; i++)
					Views.push_back(View[i]);

				NMake::Pack(Node->SetDocument("view"), Views);
				NMake::Pack(Node->SetDocument("rebuild"), Rebuild);
				NMake::Pack(Node->SetDocument("projection"), Projection);
				NMake::Pack(Node->SetDocument("diffuse"), Diffuse);
				NMake::Pack(Node->SetDocument("visibility"), Visibility);
				NMake::Pack(Node->SetDocument("range"), Range);
				NMake::Pack(Node->SetDocument("capture-range"), CaptureRange);
				NMake::Pack(Node->SetDocument("emission"), Emission);
				NMake::Pack(Node->SetDocument("image-based"), ImageBased);
				NMake::Pack(Node->SetDocument("parallax-corrected"), ParallaxCorrected);
			}
			void ProbeLight::OnSynchronize(Rest::Timer* Time)
			{
				Viewer ViewPoint = Parent->GetScene()->GetCameraViewer();
				float Hardness = 1.0f - Parent->Transform->Position.Distance(Parent->GetScene()->GetCamera()->GetEntity()->Transform->Position) / (ViewPoint.ViewDistance);

				if (Hardness > 0.0f)
					Visibility = Compute::MathCommon::IsClipping(ViewPoint.ViewProjection, Parent->Transform->GetWorld(), Range) == -1 ? Hardness : 0.0f;
				else
					Visibility = 0.0f;
			}
			bool ProbeLight::RebuildDiffuseMap()
			{
				if (!DiffuseMapX[0] || !DiffuseMapX[1] || !DiffuseMapY[0] || !DiffuseMapY[1] || !DiffuseMapZ[0] || !DiffuseMapZ[1])
					return false;

				Graphics::TextureCube::Desc F;
				F.Texture2D[0] = DiffuseMapX[0];
				F.Texture2D[1] = DiffuseMapX[1];
				F.Texture2D[2] = DiffuseMapY[0];
				F.Texture2D[3] = DiffuseMapY[1];
				F.Texture2D[4] = DiffuseMapZ[0];
				F.Texture2D[5] = DiffuseMapZ[1];

				delete DiffuseMap;
				DiffuseMap = Graphics::TextureCube::Create(Parent->GetScene()->GetDevice(), F);
				return DiffuseMap != nullptr;
			}
			Component* ProbeLight::OnClone(Entity* New)
			{
				ProbeLight* Target = new ProbeLight(New);
				Target->Projection = Projection;
				Target->Range = Range;
				Target->Diffuse = Diffuse;
				Target->Visibility = Visibility;
				Target->Emission = Emission;
				Target->CaptureRange = CaptureRange;
				Target->ImageBased = ImageBased;
				Target->Rebuild = Rebuild;
				Target->DiffuseMapX[0] = DiffuseMapX[0];
				Target->DiffuseMapX[1] = DiffuseMapX[1];
				Target->DiffuseMapY[0] = DiffuseMapY[0];
				Target->DiffuseMapY[1] = DiffuseMapY[1];
				Target->DiffuseMapZ[0] = DiffuseMapZ[0];
				Target->DiffuseMapZ[1] = DiffuseMapZ[1];
				Target->RebuildDiffuseMap();
				memcpy(Target->View, View, 6 * sizeof(Compute::Matrix4x4));

				return Target;
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
				auto* RenderStages = Renderer->GetRenderers();
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
					uint64_t RendererId;
					NMake::Unpack(Render->Find("id"), &RendererId);
					Engine::Renderer* Target = nullptr;

					if (RendererId == THAWK_COMPONENT_ID(ModelRenderer))
						Target = new Engine::Renderers::ModelRenderer(Renderer);
					else if (RendererId == THAWK_COMPONENT_ID(SkinModelRenderer))
						Target = new Engine::Renderers::SkinModelRenderer(Renderer);
					else if (RendererId == THAWK_COMPONENT_ID(SoftBodyRenderer))
						Target = new Engine::Renderers::SoftBodyRenderer(Renderer);
					else if (RendererId == THAWK_COMPONENT_ID(DepthRenderer))
						Target = new Engine::Renderers::DepthRenderer(Renderer);
					else if (RendererId == THAWK_COMPONENT_ID(LightRenderer))
						Target = new Engine::Renderers::LightRenderer(Renderer);
					else if (RendererId == THAWK_COMPONENT_ID(ProbeRenderer))
						Target = new Engine::Renderers::ProbeRenderer(Renderer);
					else if (RendererId == THAWK_COMPONENT_ID(ImageRenderer))
						Target = new Engine::Renderers::ImageRenderer(Renderer);
					else if (RendererId == THAWK_COMPONENT_ID(ElementSystemRenderer))
						Target = new Engine::Renderers::ElementSystemRenderer(Renderer);
					else if (RendererId == THAWK_COMPONENT_ID(ReflectionsRenderer))
						Target = new Engine::Renderers::ReflectionsRenderer(Renderer);
					else if (RendererId == THAWK_COMPONENT_ID(DepthOfFieldRenderer))
						Target = new Engine::Renderers::DepthOfFieldRenderer(Renderer);
					else if (RendererId == THAWK_COMPONENT_ID(EmissionRenderer))
						Target = new Engine::Renderers::EmissionRenderer(Renderer);
					else if (RendererId == THAWK_COMPONENT_ID(GlitchRenderer))
						Target = new Engine::Renderers::GlitchRenderer(Renderer);
					else if (RendererId == THAWK_COMPONENT_ID(AmbientOcclusionRenderer))
						Target = new Engine::Renderers::AmbientOcclusionRenderer(Renderer);
					else if (RendererId == THAWK_COMPONENT_ID(IndirectOcclusionRenderer))
						Target = new Engine::Renderers::IndirectOcclusionRenderer(Renderer);
					else if (RendererId == THAWK_COMPONENT_ID(ToneRenderer))
						Target = new Engine::Renderers::ToneRenderer(Renderer);
					else if (RendererId == THAWK_COMPONENT_ID(GUIRenderer))
						Target = new Engine::Renderers::GUIRenderer(Renderer);

					if (!Renderer || !Target)
					{
						THAWK_WARN("cannot create renderer with id %llu", RendererId);
						continue;
					}

					Rest::Document* Meta = Render->Find("metadata");
					if (!Meta)
						Meta = Render->SetDocument("metadata");

					Target->OnRelease();
					Target->OnLoad(Content, Meta);
					Target->OnInitialize();

					Renderer->AddRenderer(Target);
					NMake::Unpack(Render->Find("active"), &Target->Active);
				}
			}
			void Camera::OnSave(ContentManager* Content, Rest::Document* Node)
			{
				NMake::Pack(Node->SetDocument("projection"), Projection);
				NMake::Pack(Node->SetDocument("view-distance"), ViewDistance);

				Rest::Document* Renderers = Node->SetArray("renderers");
				for (auto& Ref : *Renderer->GetRenderers())
				{
					Rest::Document* Render = Renderers->SetDocument("renderer");
					NMake::Pack(Render->SetDocument("id"), Ref->Id());
					NMake::Pack(Render->SetDocument("active"), Ref->Active);
					Ref->OnSave(Content, Render->SetDocument("metadata"));
				}
			}
			void Camera::FillViewer(Viewer* View)
			{
				if (!View)
					return;

				View->ViewPosition = Compute::Vector3(-Parent->Transform->Position.X, -Parent->Transform->Position.Y, Parent->Transform->Position.Z);
				View->View = Compute::Matrix4x4::CreateCamera(View->ViewPosition, -Parent->Transform->Rotation);
				View->Projection = Projection;
				View->ViewProjection = View->View * Projection;
				View->InvViewProjection = View->ViewProjection.Invert();
				View->Position = Parent->Transform->Position.InvertZ();
				View->RawPosition = Parent->Transform->Position;
				View->ViewDistance = ViewDistance;
				View->Renderer = Renderer;
				FieldView = *View;
			}
			void Camera::ResizeBuffers()
			{
				if (!Renderer)
					return;

				Renderer->SetScene(Parent->GetScene());
				auto* RenderStages = Renderer->GetRenderers();
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
			Component* Camera::OnClone(Entity* New)
			{
				Camera* Target = new Camera(New);
				Target->ViewDistance = ViewDistance;
				Target->Projection = Projection;

				return Target;
			}
		}
	}
}