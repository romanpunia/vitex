#include "components.h"
#include "renderers.h"
#include "../audio/effects.h"
#include "../script/std-lib.h"
#include <cstddef>

namespace
{
	enum
	{
		BOX_NONE = 0,
		BOX_GEOMETRY = 1,
		BOX_LIGHT = 2,
		BOX_BODY = 3,
		BOX_DYNAMIC = 4
	};
}

namespace Tomahawk
{
	namespace Engine
	{
		namespace Components
		{
			static float GetVisibilityRadius(Entity* Base, const Viewer& View, float Distance)
			{
				float Visibility = 1.0f - Distance / View.FarPlane;
				if (Visibility <= 0.0f)
					return 0.0f;

				const Compute::Matrix4x4& Box = Base->GetTransform()->GetBiasUnscaled();
				return Compute::Geometric::IsCubeInFrustum(Box * View.ViewProjection, Base->GetRadius()) ? Visibility : 0.0f;
			}

			RigidBody::RigidBody(Entity* Ref) : Component(Ref, ActorSet::Synchronize)
			{
			}
			RigidBody::~RigidBody()
			{
				TH_RELEASE(Instance);
				TH_RELEASE(Hull);
			}
			void RigidBody::Deserialize(ContentManager* Content, Core::Schema* Node)
			{
				TH_ASSERT_V(Content != nullptr, "content manager should be set");
				TH_ASSERT_V(Node != nullptr, "schema should be set");

				bool Extended = false;
				Series::Unpack(Node->Find("extended"), &Extended);
				Series::Unpack(Node->Find("kinematic"), &Kinematic);
				Series::Unpack(Node->Find("manage"), &Manage);

				if (!Extended)
					return;

				float Mass = 0, CcdMotionThreshold = 0;
				Series::Unpack(Node->Find("mass"), &Mass);
				Series::Unpack(Node->Find("ccd-motion-threshold"), &CcdMotionThreshold);

				SceneGraph* Scene = Parent->GetScene();
				Core::Schema* CV = nullptr;
				if ((CV = Node->Find("shape")) != nullptr)
				{
					std::string Path; uint64_t Type;
					if (Series::Unpack(Node->Find("path"), &Path))
					{
						auto* Shape = Content->Load<Compute::HullShape>(Path);
						if (Shape != nullptr)
						{
							Create(Shape->Shape, Mass, CcdMotionThreshold);
							TH_RELEASE(Shape);
						}
					}
					else if (!Series::Unpack(CV->Find("type"), &Type))
					{
						std::vector<Compute::Vector3> Vertices;
						if (Series::Unpack(CV->Find("data"), &Vertices))
						{
							btCollisionShape* Shape = Scene->GetSimulator()->CreateConvexHull(Vertices);
							if (Shape != nullptr)
								Create(Shape, Mass, CcdMotionThreshold);
						}
					}
					else
					{
						btCollisionShape* Shape = Scene->GetSimulator()->CreateShape((Compute::Shape)Type);
						if (Shape != nullptr)
							Create(Shape, Mass, CcdMotionThreshold);
					}
				}

				if (!Instance)
					return;

				uint64_t ActivationState;
				if (Series::Unpack(Node->Find("activation-state"), &ActivationState))
					Instance->SetActivationState((Compute::MotionState)ActivationState);

				float AngularDamping;
				if (Series::Unpack(Node->Find("angular-damping"), &AngularDamping))
					Instance->SetAngularDamping(AngularDamping);

				float AngularSleepingThreshold;
				if (Series::Unpack(Node->Find("angular-sleeping-threshold"), &AngularSleepingThreshold))
					Instance->SetAngularSleepingThreshold(AngularSleepingThreshold);

				float Friction;
				if (Series::Unpack(Node->Find("friction"), &Friction))
					Instance->SetFriction(Friction);

				float Restitution;
				if (Series::Unpack(Node->Find("restitution"), &Restitution))
					Instance->SetRestitution(Restitution);

				float HitFraction;
				if (Series::Unpack(Node->Find("hit-fraction"), &HitFraction))
					Instance->SetHitFraction(HitFraction);

				float LinearDamping;
				if (Series::Unpack(Node->Find("linear-damping"), &LinearDamping))
					Instance->SetLinearDamping(LinearDamping);

				float LinearSleepingThreshold;
				if (Series::Unpack(Node->Find("linear-sleeping-threshold"), &LinearSleepingThreshold))
					Instance->SetLinearSleepingThreshold(LinearSleepingThreshold);

				float CcdSweptSphereRadius;
				if (Series::Unpack(Node->Find("ccd-swept-sphere-radius"), &CcdSweptSphereRadius))
					Instance->SetCcdSweptSphereRadius(CcdSweptSphereRadius);

				float ContactProcessingThreshold;
				if (Series::Unpack(Node->Find("contact-processing-threshold"), &ContactProcessingThreshold))
					Instance->SetContactProcessingThreshold(ContactProcessingThreshold);

				float DeactivationTime;
				if (Series::Unpack(Node->Find("deactivation-time"), &DeactivationTime))
					Instance->SetDeactivationTime(DeactivationTime);

				float RollingFriction;
				if (Series::Unpack(Node->Find("rolling-friction"), &RollingFriction))
					Instance->SetRollingFriction(RollingFriction);

				float SpinningFriction;
				if (Series::Unpack(Node->Find("spinning-friction"), &SpinningFriction))
					Instance->SetSpinningFriction(SpinningFriction);

				float ContactStiffness;
				if (Series::Unpack(Node->Find("contact-stiffness"), &ContactStiffness))
					Instance->SetContactStiffness(ContactStiffness);

				float ContactDamping;
				if (Series::Unpack(Node->Find("contact-damping"), &ContactDamping))
					Instance->SetContactDamping(ContactDamping);

				Compute::Vector3 AngularFactor;
				if (Series::Unpack(Node->Find("angular-factor"), &AngularFactor))
					Instance->SetAngularFactor(AngularFactor);

				Compute::Vector3 AngularVelocity;
				if (Series::Unpack(Node->Find("angular-velocity"), &AngularVelocity))
					Instance->SetAngularVelocity(AngularVelocity);

				Compute::Vector3 AnisotropicFriction;
				if (Series::Unpack(Node->Find("anisotropic-friction"), &AnisotropicFriction))
					Instance->SetAnisotropicFriction(AnisotropicFriction);

				Compute::Vector3 Gravity;
				if (Series::Unpack(Node->Find("gravity"), &Gravity))
					Instance->SetGravity(Gravity);

				Compute::Vector3 LinearFactor;
				if (Series::Unpack(Node->Find("linear-factor"), &LinearFactor))
					Instance->SetLinearFactor(LinearFactor);

				Compute::Vector3 LinearVelocity;
				if (Series::Unpack(Node->Find("linear-velocity"), &LinearVelocity))
					Instance->SetLinearVelocity(LinearVelocity);

				uint64_t CollisionFlags;
				if (Series::Unpack(Node->Find("collision-flags"), &CollisionFlags))
					Instance->SetCollisionFlags(CollisionFlags);
			}
			void RigidBody::Serialize(ContentManager* Content, Core::Schema* Node)
			{
				TH_ASSERT_V(Content != nullptr, "content manager should be set");
				TH_ASSERT_V(Node != nullptr, "schema should be set");

				Series::Pack(Node->Set("kinematic"), Kinematic);
				Series::Pack(Node->Set("manage"), Manage);
				Series::Pack(Node->Set("extended"), Instance != nullptr);

				if (!Instance)
					return;

				SceneGraph* Scene = Parent->GetScene();
				Core::Schema* CV = Node->Set("shape");
				if (Instance->GetCollisionShapeType() == Compute::Shape::Convex_Hull)
				{
					AssetCache* Asset = Content->Find<Compute::HullShape>(Hull);
					if (!Asset || !Hull)
					{
						std::vector<Compute::Vector3> Vertices = Scene->GetSimulator()->GetShapeVertices(Instance->GetCollisionShape());
						Series::Pack(CV->Set("data"), Vertices);
					}
					else
						Series::Pack(CV->Set("path"), Asset->Path);
				}
				else
					Series::Pack(CV->Set("type"), (uint64_t)Instance->GetCollisionShapeType());

				Series::Pack(Node->Set("mass"), Instance->GetMass());
				Series::Pack(Node->Set("ccd-motion-threshold"), Instance->GetCcdMotionThreshold());
				Series::Pack(Node->Set("activation-state"), (uint64_t)Instance->GetActivationState());
				Series::Pack(Node->Set("angular-damping"), Instance->GetAngularDamping());
				Series::Pack(Node->Set("angular-sleeping-threshold"), Instance->GetAngularSleepingThreshold());
				Series::Pack(Node->Set("friction"), Instance->GetFriction());
				Series::Pack(Node->Set("restitution"), Instance->GetRestitution());
				Series::Pack(Node->Set("hit-fraction"), Instance->GetHitFraction());
				Series::Pack(Node->Set("linear-damping"), Instance->GetLinearDamping());
				Series::Pack(Node->Set("linear-sleeping-threshold"), Instance->GetLinearSleepingThreshold());
				Series::Pack(Node->Set("ccd-swept-sphere-radius"), Instance->GetCcdSweptSphereRadius());
				Series::Pack(Node->Set("contact-processing-threshold"), Instance->GetContactProcessingThreshold());
				Series::Pack(Node->Set("deactivation-time"), Instance->GetDeactivationTime());
				Series::Pack(Node->Set("rolling-friction"), Instance->GetRollingFriction());
				Series::Pack(Node->Set("spinning-friction"), Instance->GetSpinningFriction());
				Series::Pack(Node->Set("contact-stiffness"), Instance->GetContactStiffness());
				Series::Pack(Node->Set("contact-damping"), Instance->GetContactDamping());
				Series::Pack(Node->Set("angular-factor"), Instance->GetAngularFactor());
				Series::Pack(Node->Set("angular-velocity"), Instance->GetAngularVelocity());
				Series::Pack(Node->Set("anisotropic-friction"), Instance->GetAnisotropicFriction());
				Series::Pack(Node->Set("gravity"), Instance->GetGravity());
				Series::Pack(Node->Set("linear-factor"), Instance->GetLinearFactor());
				Series::Pack(Node->Set("linear-velocity"), Instance->GetLinearVelocity());
				Series::Pack(Node->Set("collision-flags"), (uint64_t)Instance->GetCollisionFlags());
			}
			void RigidBody::Synchronize(Core::Timer* Time)
			{
				if (Instance && Manage)
					Instance->Synchronize(Parent->GetTransform(), Kinematic);
			}
			void RigidBody::Deactivate()
			{
				if (Instance != nullptr)
					Instance->SetAsGhost();
			}
			void RigidBody::Create(btCollisionShape* Shape, float Mass, float Anticipation)
			{
				TH_ASSERT_V(Shape != nullptr, "collision shape should be set");
				SceneGraph* Scene = Parent->GetScene();
				Scene->Transaction([this, Scene, Shape, Mass, Anticipation]()
				{
					Compute::RigidBody::Desc I;
					I.Anticipation = Anticipation;
					I.Mass = Mass;
					I.Shape = Shape;

					TH_RELEASE(Instance);
					Instance = Scene->GetSimulator()->CreateRigidBody(I, Parent->GetTransform());
					Instance->UserPointer = this;
					Instance->SetActivity(true);
				});
			}
			void RigidBody::Create(ContentManager* Content, const std::string& Path, float Mass, float Anticipation)
			{
				TH_ASSERT_V(Content != nullptr, "content manager should be set");
				TH_RELEASE(Hull);
				Hull = Content->Load<Compute::HullShape>(Path);
				if (Hull != nullptr)
					Create(Hull->Shape, Mass, Anticipation);
			}
			void RigidBody::Clear()
			{
				if (!Instance)
					return;

				SceneGraph* Scene = Parent->GetScene();
				Scene->Transaction([this]()
				{
					TH_CLEAR(Instance);
				});
			}
			void RigidBody::SetTransform(const Compute::Vector3& Position, const Compute::Vector3& Scale, const Compute::Vector3& Rotation)
			{
				if (!Instance)
					return;

				Compute::Transform::Spacing Space;
				Space.Position = Position;
				Space.Rotation = Rotation;
				Space.Scale = Scale;

				auto* Transform = Parent->GetTransform();
				Transform->SetSpacing(Compute::Positioning::Global, Space);
				Instance->Synchronize(Transform, true);

				SceneGraph* Scene = Parent->GetScene();
				Scene->Transaction([this]()
				{
					Instance->SetActivity(true);
				});
			}
			void RigidBody::SetTransform(bool Kinematics)
			{
				if (!Instance)
					return;

				auto* Transform = Parent->GetTransform();
				Instance->Synchronize(Transform, Kinematics);

				SceneGraph* Scene = Parent->GetScene();
				Scene->Transaction([this]()
				{
					Instance->SetActivity(true);
				});
			}
			void RigidBody::SetMass(float Mass)
			{
				if (!Instance)
					return;

				SceneGraph* Scene = Parent->GetScene();
				Scene->Transaction([this, Mass]()
				{
					Instance->SetMass(Mass);
				});
			}
			Component* RigidBody::Copy(Entity* New) const
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
			Compute::RigidBody* RigidBody::GetBody() const
			{
				return Instance;
			}

			SoftBody::SoftBody(Entity* Ref) : Drawable(Ref, ActorSet::Synchronize, SoftBody::GetTypeId(), false)
			{
			}
			SoftBody::~SoftBody()
			{
				TH_RELEASE(Instance);
			}
			void SoftBody::Deserialize(ContentManager* Content, Core::Schema* Node)
			{
				TH_ASSERT_V(Content != nullptr, "content manager should be set");
				TH_ASSERT_V(Node != nullptr, "schema should be set");

				uint64_t Slot = -1;
				if (Series::Unpack(Node->Find("material"), &Slot))
					SetMaterial(nullptr, Parent->GetScene()->GetMaterial((uint64_t)Slot));

				bool Extended = false;
				bool Transparent = false;
				std::string Path;

				uint32_t NewCategory = (uint32_t)GeoCategory::Opaque;
				Series::Unpack(Node->Find("texcoord"), &TexCoord);
				Series::Unpack(Node->Find("extended"), &Extended);
				Series::Unpack(Node->Find("kinematic"), &Kinematic);
				Series::Unpack(Node->Find("manage"), &Manage);
				Series::Unpack(Node->Find("static"), &Static);
				Series::Unpack(Node->Find("category"), &NewCategory);
				SetCategory((GeoCategory)NewCategory);

				if (!Extended)
					return;

				float CcdMotionThreshold = 0;
				Series::Unpack(Node->Find("ccd-motion-threshold"), &CcdMotionThreshold);

				Core::Schema* CV = nullptr;
				if ((CV = Node->Find("shape")) != nullptr)
				{
					if (Series::Unpack(Node->Find("path"), &Path))
					{
						auto* Shape = Content->Load<Compute::HullShape>(Path);
						if (Shape != nullptr)
						{
							Create(Shape, CcdMotionThreshold);
							TH_RELEASE(Shape);
						}
					}
				}
				else if ((CV = Node->Find("ellipsoid")) != nullptr)
				{
					Compute::SoftBody::Desc::CV::SEllipsoid Shape;
					Series::Unpack(CV->Get("center"), &Shape.Center);
					Series::Unpack(CV->Get("radius"), &Shape.Radius);
					Series::Unpack(CV->Get("count"), &Shape.Count);
					CreateEllipsoid(Shape, CcdMotionThreshold);
				}
				else if ((CV = Node->Find("patch")) != nullptr)
				{
					Compute::SoftBody::Desc::CV::SPatch Shape;
					Series::Unpack(CV->Get("corner-00"), &Shape.Corner00);
					Series::Unpack(CV->Get("corner-00-fixed"), &Shape.Corner00Fixed);
					Series::Unpack(CV->Get("corner-01"), &Shape.Corner01);
					Series::Unpack(CV->Get("corner-01-fixed"), &Shape.Corner01Fixed);
					Series::Unpack(CV->Get("corner-10"), &Shape.Corner10);
					Series::Unpack(CV->Get("corner-10-fixed"), &Shape.Corner10Fixed);
					Series::Unpack(CV->Get("corner-11"), &Shape.Corner11);
					Series::Unpack(CV->Get("corner-11-fixed"), &Shape.Corner11Fixed);
					Series::Unpack(CV->Get("count-x"), &Shape.CountX);
					Series::Unpack(CV->Get("count-y"), &Shape.CountY);
					Series::Unpack(CV->Get("diagonals"), &Shape.GenerateDiagonals);
					CreatePatch(Shape, CcdMotionThreshold);
				}
				else if ((CV = Node->Find("rope")) != nullptr)
				{
					Compute::SoftBody::Desc::CV::SRope Shape;
					Series::Unpack(CV->Get("start"), &Shape.Start);
					Series::Unpack(CV->Get("start-fixed"), &Shape.StartFixed);
					Series::Unpack(CV->Get("end"), &Shape.End);
					Series::Unpack(CV->Get("end-fixed"), &Shape.EndFixed);
					Series::Unpack(CV->Get("count"), &Shape.Count);
					CreateRope(Shape, CcdMotionThreshold);
				}

				if (!Instance)
					return;

				Core::Schema* Conf = Node->Get("config");
				if (Conf != nullptr)
				{
					Compute::SoftBody::Desc::SConfig I;
					Series::Unpack(Conf->Get("vcf"), &I.VCF);
					Series::Unpack(Conf->Get("dp"), &I.DP);
					Series::Unpack(Conf->Get("dg"), &I.DG);
					Series::Unpack(Conf->Get("lf"), &I.LF);
					Series::Unpack(Conf->Get("pr"), &I.PR);
					Series::Unpack(Conf->Get("vc"), &I.VC);
					Series::Unpack(Conf->Get("df"), &I.DF);
					Series::Unpack(Conf->Get("mt"), &I.MT);
					Series::Unpack(Conf->Get("chr"), &I.CHR);
					Series::Unpack(Conf->Get("khr"), &I.KHR);
					Series::Unpack(Conf->Get("shr"), &I.SHR);
					Series::Unpack(Conf->Get("ahr"), &I.AHR);
					Series::Unpack(Conf->Get("srhr-cl"), &I.SRHR_CL);
					Series::Unpack(Conf->Get("skhr-cl"), &I.SKHR_CL);
					Series::Unpack(Conf->Get("sshr-cl"), &I.SSHR_CL);
					Series::Unpack(Conf->Get("sr-splt-cl"), &I.SR_SPLT_CL);
					Series::Unpack(Conf->Get("sk-splt-cl"), &I.SK_SPLT_CL);
					Series::Unpack(Conf->Get("ss-splt-cl"), &I.SS_SPLT_CL);
					Series::Unpack(Conf->Get("max-volume"), &I.MaxVolume);
					Series::Unpack(Conf->Get("time-scale"), &I.TimeScale);
					Series::Unpack(Conf->Get("drag"), &I.Drag);
					Series::Unpack(Conf->Get("max-stress"), &I.MaxStress);
					Series::Unpack(Conf->Get("constraints"), &I.Constraints);
					Series::Unpack(Conf->Get("clusters"), &I.Clusters);
					Series::Unpack(Conf->Get("v-it"), &I.VIterations);
					Series::Unpack(Conf->Get("p-it"), &I.PIterations);
					Series::Unpack(Conf->Get("d-it"), &I.DIterations);
					Series::Unpack(Conf->Get("c-it"), &I.CIterations);
					Series::Unpack(Conf->Get("collisions"), &I.Collisions);

					uint64_t AeroModel;
					if (Series::Unpack(Conf->Get("aero-model"), &AeroModel))
						I.AeroModel = (Compute::SoftAeroModel)AeroModel;

					Instance->SetConfig(I);
				}

				uint64_t ActivationState;
				if (Series::Unpack(Node->Find("activation-state"), &ActivationState))
					Instance->SetActivationState((Compute::MotionState)ActivationState);

				float Friction;
				if (Series::Unpack(Node->Find("friction"), &Friction))
					Instance->SetFriction(Friction);

				float Restitution;
				if (Series::Unpack(Node->Find("restitution"), &Restitution))
					Instance->SetRestitution(Restitution);

				float HitFraction;
				if (Series::Unpack(Node->Find("hit-fraction"), &HitFraction))
					Instance->SetHitFraction(HitFraction);

				float CcdSweptSphereRadius;
				if (Series::Unpack(Node->Find("ccd-swept-sphere-radius"), &CcdSweptSphereRadius))
					Instance->SetCcdSweptSphereRadius(CcdSweptSphereRadius);

				float ContactProcessingThreshold;
				if (Series::Unpack(Node->Find("contact-processing-threshold"), &ContactProcessingThreshold))
					Instance->SetContactProcessingThreshold(ContactProcessingThreshold);

				float DeactivationTime;
				if (Series::Unpack(Node->Find("deactivation-time"), &DeactivationTime))
					Instance->SetDeactivationTime(DeactivationTime);

				float RollingFriction;
				if (Series::Unpack(Node->Find("rolling-friction"), &RollingFriction))
					Instance->SetRollingFriction(RollingFriction);

				float SpinningFriction;
				if (Series::Unpack(Node->Find("spinning-friction"), &SpinningFriction))
					Instance->SetSpinningFriction(SpinningFriction);

				float ContactStiffness;
				if (Series::Unpack(Node->Find("contact-stiffness"), &ContactStiffness))
					Instance->SetContactStiffness(ContactStiffness);

				float ContactDamping;
				if (Series::Unpack(Node->Find("contact-damping"), &ContactDamping))
					Instance->SetContactDamping(ContactDamping);

				Compute::Vector3 AnisotropicFriction;
				if (Series::Unpack(Node->Find("anisotropic-friction"), &AnisotropicFriction))
					Instance->SetAnisotropicFriction(AnisotropicFriction);

				Compute::Vector3 WindVelocity;
				if (Series::Unpack(Node->Find("wind-velocity"), &WindVelocity))
					Instance->SetWindVelocity(WindVelocity);

				float TotalMass;
				if (Series::Unpack(Node->Find("total-mass"), &TotalMass))
					Instance->SetTotalMass(TotalMass);

				float RestLengthScale;
				if (Series::Unpack(Node->Find("core-length-scale"), &RestLengthScale))
					Instance->SetRestLengthScale(RestLengthScale);
			}
			void SoftBody::Serialize(ContentManager* Content, Core::Schema* Node)
			{
				TH_ASSERT_V(Content != nullptr, "content manager should be set");
				TH_ASSERT_V(Node != nullptr, "schema should be set");

				Material* Slot = GetMaterial();
				if (Slot != nullptr)
					Series::Pack(Node->Set("material"), Slot->Slot);

				Series::Pack(Node->Set("texcoord"), TexCoord);
				Series::Pack(Node->Set("category"), (uint32_t)GetCategory());
				Series::Pack(Node->Set("kinematic"), Kinematic);
				Series::Pack(Node->Set("manage"), Manage);
				Series::Pack(Node->Set("extended"), Instance != nullptr);
				Series::Pack(Node->Set("static"), Static);

				if (!Instance)
					return;

				Compute::SoftBody::Desc& I = Instance->GetInitialState();
				Core::Schema* Conf = Node->Set("config");
				Series::Pack(Conf->Set("aero-model"), (uint64_t)I.Config.AeroModel);
				Series::Pack(Conf->Set("vcf"), I.Config.VCF);
				Series::Pack(Conf->Set("dp"), I.Config.DP);
				Series::Pack(Conf->Set("dg"), I.Config.DG);
				Series::Pack(Conf->Set("lf"), I.Config.LF);
				Series::Pack(Conf->Set("pr"), I.Config.PR);
				Series::Pack(Conf->Set("vc"), I.Config.VC);
				Series::Pack(Conf->Set("df"), I.Config.DF);
				Series::Pack(Conf->Set("mt"), I.Config.MT);
				Series::Pack(Conf->Set("chr"), I.Config.CHR);
				Series::Pack(Conf->Set("khr"), I.Config.KHR);
				Series::Pack(Conf->Set("shr"), I.Config.SHR);
				Series::Pack(Conf->Set("ahr"), I.Config.AHR);
				Series::Pack(Conf->Set("srhr-cl"), I.Config.SRHR_CL);
				Series::Pack(Conf->Set("skhr-cl"), I.Config.SKHR_CL);
				Series::Pack(Conf->Set("sshr-cl"), I.Config.SSHR_CL);
				Series::Pack(Conf->Set("sr-splt-cl"), I.Config.SR_SPLT_CL);
				Series::Pack(Conf->Set("sk-splt-cl"), I.Config.SK_SPLT_CL);
				Series::Pack(Conf->Set("ss-splt-cl"), I.Config.SS_SPLT_CL);
				Series::Pack(Conf->Set("max-volume"), I.Config.MaxVolume);
				Series::Pack(Conf->Set("time-scale"), I.Config.TimeScale);
				Series::Pack(Conf->Set("drag"), I.Config.Drag);
				Series::Pack(Conf->Set("max-stress"), I.Config.MaxStress);
				Series::Pack(Conf->Set("constraints"), I.Config.Constraints);
				Series::Pack(Conf->Set("clusters"), I.Config.Clusters);
				Series::Pack(Conf->Set("v-it"), I.Config.VIterations);
				Series::Pack(Conf->Set("p-it"), I.Config.PIterations);
				Series::Pack(Conf->Set("d-it"), I.Config.DIterations);
				Series::Pack(Conf->Set("c-it"), I.Config.CIterations);
				Series::Pack(Conf->Set("collisions"), I.Config.Collisions);

				auto& Desc = Instance->GetInitialState();
				if (Desc.Shape.Convex.Enabled)
				{
					if (Instance->GetCollisionShapeType() == Compute::Shape::Convex_Hull)
					{
						AssetCache* Asset = Content->Find<Compute::HullShape>(Desc.Shape.Convex.Hull);
						if (Asset != nullptr)
						{
							Core::Schema* Shape = Node->Set("shape");
							Series::Pack(Shape->Set("path"), Asset->Path);
						}
					}
				}
				else if (Desc.Shape.Ellipsoid.Enabled)
				{
					Core::Schema* Shape = Node->Set("ellipsoid");
					Series::Pack(Shape->Set("center"), Desc.Shape.Ellipsoid.Center);
					Series::Pack(Shape->Set("radius"), Desc.Shape.Ellipsoid.Radius);
					Series::Pack(Shape->Set("count"), Desc.Shape.Ellipsoid.Count);
				}
				else if (Desc.Shape.Patch.Enabled)
				{
					Core::Schema* Shape = Node->Set("patch");
					Series::Pack(Shape->Set("corner-00"), Desc.Shape.Patch.Corner00);
					Series::Pack(Shape->Set("corner-00-fixed"), Desc.Shape.Patch.Corner00Fixed);
					Series::Pack(Shape->Set("corner-01"), Desc.Shape.Patch.Corner01);
					Series::Pack(Shape->Set("corner-01-fixed"), Desc.Shape.Patch.Corner01Fixed);
					Series::Pack(Shape->Set("corner-10"), Desc.Shape.Patch.Corner10);
					Series::Pack(Shape->Set("corner-10-fixed"), Desc.Shape.Patch.Corner10Fixed);
					Series::Pack(Shape->Set("corner-11"), Desc.Shape.Patch.Corner11);
					Series::Pack(Shape->Set("corner-11-fixed"), Desc.Shape.Patch.Corner11Fixed);
					Series::Pack(Shape->Set("count-x"), Desc.Shape.Patch.CountX);
					Series::Pack(Shape->Set("count-y"), Desc.Shape.Patch.CountY);
					Series::Pack(Shape->Set("diagonals"), Desc.Shape.Patch.GenerateDiagonals);
				}
				else if (Desc.Shape.Rope.Enabled)
				{
					Core::Schema* Shape = Node->Set("rope");
					Series::Pack(Shape->Set("start"), Desc.Shape.Rope.Start);
					Series::Pack(Shape->Set("start-fixed"), Desc.Shape.Rope.StartFixed);
					Series::Pack(Shape->Set("end"), Desc.Shape.Rope.End);
					Series::Pack(Shape->Set("end-fixed"), Desc.Shape.Rope.EndFixed);
					Series::Pack(Shape->Set("count"), Desc.Shape.Rope.Count);
				}

				Series::Pack(Node->Set("ccd-motion-threshold"), Instance->GetCcdMotionThreshold());
				Series::Pack(Node->Set("activation-state"), (uint64_t)Instance->GetActivationState());
				Series::Pack(Node->Set("friction"), Instance->GetFriction());
				Series::Pack(Node->Set("restitution"), Instance->GetRestitution());
				Series::Pack(Node->Set("hit-fraction"), Instance->GetHitFraction());
				Series::Pack(Node->Set("ccd-swept-sphere-radius"), Instance->GetCcdSweptSphereRadius());
				Series::Pack(Node->Set("contact-processing-threshold"), Instance->GetContactProcessingThreshold());
				Series::Pack(Node->Set("deactivation-time"), Instance->GetDeactivationTime());
				Series::Pack(Node->Set("rolling-friction"), Instance->GetRollingFriction());
				Series::Pack(Node->Set("spinning-friction"), Instance->GetSpinningFriction());
				Series::Pack(Node->Set("contact-stiffness"), Instance->GetContactStiffness());
				Series::Pack(Node->Set("contact-damping"), Instance->GetContactDamping());
				Series::Pack(Node->Set("angular-velocity"), Instance->GetAngularVelocity());
				Series::Pack(Node->Set("anisotropic-friction"), Instance->GetAnisotropicFriction());
				Series::Pack(Node->Set("linear-velocity"), Instance->GetLinearVelocity());
				Series::Pack(Node->Set("collision-flags"), (uint64_t)Instance->GetCollisionFlags());
				Series::Pack(Node->Set("wind-velocity"), Instance->GetWindVelocity());
				Series::Pack(Node->Set("total-mass"), Instance->GetTotalMass());
				Series::Pack(Node->Set("core-length-scale"), Instance->GetRestLengthScale());
			}
			void SoftBody::Synchronize(Core::Timer* Time)
			{
				if (!Instance)
					return;

				if (Manage)
				{
					auto* Transform = Parent->GetTransform();
					Instance->Synchronize(Transform, Kinematic);
					if (Instance->IsActive())
						Transform->MakeDirty();
				}

				Instance->GetVertices(&Vertices);
				if (Indices.empty())
					Instance->GetIndices(&Indices);
			}
			void SoftBody::Deactivate()
			{
				if (Instance != nullptr)
					Instance->SetAsGhost();
			}
			void SoftBody::Create(Compute::HullShape* Shape, float Anticipation)
			{
				TH_ASSERT_V(Shape != nullptr, "collision shape should be set");
				Shape->AddRef();

				SceneGraph* Scene = Parent->GetScene();
				Scene->Transaction([this, Scene, Shape, Anticipation]()
				{
					Compute::SoftBody::Desc I;
					I.Anticipation = Anticipation;
					I.Shape.Convex.Hull = Shape;
					I.Shape.Convex.Enabled = true;

					TH_RELEASE(Instance);
					Instance = Scene->GetSimulator()->CreateSoftBody(I, Parent->GetTransform());
					if (!Instance)
					{
						TH_ERR("[engine] cannot create soft body");
						TH_RELEASE(Shape);
						return;
					}

					Vertices.clear();
					Indices.clear();

					Instance->UserPointer = this;
					Instance->SetActivity(true);
					TH_RELEASE(Shape);
				});
			}
			void SoftBody::Create(ContentManager* Content, const std::string& Path, float Anticipation)
			{
				TH_ASSERT_V(Content != nullptr, "content manager should be set");
				Compute::HullShape* Hull = Content->Load<Compute::HullShape>(Path);
				if (Hull != nullptr)
				{
					Create(Hull, Anticipation);
					TH_RELEASE(Hull);
				}
			}
			void SoftBody::CreateEllipsoid(const Compute::SoftBody::Desc::CV::SEllipsoid& Shape, float Anticipation)
			{
				SceneGraph* Scene = Parent->GetScene();
				Scene->Transaction([this, Scene, Shape, Anticipation]()
				{
					Compute::SoftBody::Desc I;
					I.Anticipation = Anticipation;
					I.Shape.Ellipsoid = Shape;
					I.Shape.Ellipsoid.Enabled = true;

					TH_RELEASE(Instance);
					Instance = Scene->GetSimulator()->CreateSoftBody(I, Parent->GetTransform());
					if (!Instance)
					{
						TH_ERR("[engine] cannot create soft body");
						return;
					}

					Vertices.clear();
					Indices.clear();

					Instance->UserPointer = this;
					Instance->SetActivity(true);
				});
			}
			void SoftBody::CreatePatch(const Compute::SoftBody::Desc::CV::SPatch& Shape, float Anticipation)
			{
				SceneGraph* Scene = Parent->GetScene();
				Scene->Transaction([this, Scene, Shape, Anticipation]()
				{
					TH_RELEASE(Instance);

					Compute::SoftBody::Desc I;
					I.Anticipation = Anticipation;
					I.Shape.Patch = Shape;
					I.Shape.Patch.Enabled = true;

					Instance = Scene->GetSimulator()->CreateSoftBody(I, Parent->GetTransform());
					if (!Instance)
					{
						TH_ERR("[engine] cannot create soft body");
						return;
					}

					Vertices.clear();
					Indices.clear();

					Instance->UserPointer = this;
					Instance->SetActivity(true);
				});
			}
			void SoftBody::CreateRope(const Compute::SoftBody::Desc::CV::SRope& Shape, float Anticipation)
			{
				SceneGraph* Scene = Parent->GetScene();
				Scene->Transaction([this, Scene, Shape, Anticipation]()
				{
					TH_RELEASE(Instance);

					Compute::SoftBody::Desc I;
					I.Anticipation = Anticipation;
					I.Shape.Rope = Shape;
					I.Shape.Rope.Enabled = true;

					Instance = Scene->GetSimulator()->CreateSoftBody(I, Parent->GetTransform());
					if (!Instance)
					{
						TH_ERR("[engine] cannot create soft body");
						return;
					}

					Vertices.clear();
					Indices.clear();

					Instance->UserPointer = this;
					Instance->SetActivity(true);
				});
			}
			void SoftBody::Fill(Graphics::GraphicsDevice* Device, Graphics::ElementBuffer* IndexBuffer, Graphics::ElementBuffer* VertexBuffer)
			{
				Graphics::MappedSubresource Map;
				if (VertexBuffer != nullptr && !Vertices.empty())
				{
					Device->Map(VertexBuffer, Graphics::ResourceMap::Write_Discard, &Map);
					memcpy(Map.Pointer, (void*)Vertices.data(), Vertices.size() * sizeof(Compute::Vertex));
					Device->Unmap(VertexBuffer, &Map);
				}

				if (IndexBuffer != nullptr && !Indices.empty())
				{
					Device->Map(IndexBuffer, Graphics::ResourceMap::Write_Discard, &Map);
					memcpy(Map.Pointer, (void*)Indices.data(), Indices.size() * sizeof(int));
					Device->Unmap(IndexBuffer, &Map);
				}
			}
			void SoftBody::Regenerate()
			{
				if (!Instance)
					return;

				SceneGraph* Scene = Parent->GetScene();
				Scene->Transaction([this, Scene]()
				{
					Compute::SoftBody::Desc I = Instance->GetInitialState();
					TH_RELEASE(Instance);

					Instance = Scene->GetSimulator()->CreateSoftBody(I, Parent->GetTransform());
					if (!Instance)
						TH_ERR("[engine] cannot regenerate soft body");
				});
			}
			void SoftBody::Clear()
			{
				if (!Instance)
					return;

				SceneGraph* Scene = Parent->GetScene();
				Scene->Transaction([this]()
				{
					TH_CLEAR(Instance);
				});
			}
			void SoftBody::SetTransform(const Compute::Vector3& Position, const Compute::Vector3& Scale, const Compute::Vector3& Rotation)
			{
				if (!Instance)
					return;

				SceneGraph* Scene = Parent->GetScene();
				Scene->Transaction([this, Position, Scale, Rotation]()
				{
					Compute::Transform::Spacing Space;
					Space.Position = Position;
					Space.Rotation = Rotation;
					Space.Scale = Scale;

					auto* Transform = Parent->GetTransform();
					Transform->SetSpacing(Compute::Positioning::Global, Space);
					Instance->Synchronize(Transform, true);
					Instance->SetActivity(true);
				});
			}
			void SoftBody::SetTransform(bool Kinematics)
			{
				if (!Instance)
					return;

				auto* Transform = Parent->GetTransform();
				Instance->Synchronize(Transform, Kinematics);

				SceneGraph* Scene = Parent->GetScene();
				Scene->Transaction([this, Kinematics]()
				{
					Instance->SetActivity(true);
				});
			}
			float SoftBody::GetVisibility(const Viewer& View, float Distance) const
			{
				return Instance ? Component::GetVisibility(View, Distance) : 0.0f;
			}
			size_t SoftBody::GetUnitBounds(Compute::Vector3& Min, Compute::Vector3& Max) const
			{
				if (!Instance)
					return BOX_NONE;

				Compute::Vector3 Center = Instance->GetCenterPosition();;
				Instance->GetBoundingBox(&Min, &Max);
				Min = (Min - Center) * 0.5f;
				Max = (Max - Center) * 0.5f;

				return BOX_BODY;
			}
			Component* SoftBody::Copy(Entity* New) const
			{
				SoftBody* Target = new SoftBody(New);
				Target->SetCategory(GetCategory());
				Target->Kinematic = Kinematic;

				if (Instance != nullptr)
				{
					Target->Instance = Instance->Copy();
					Target->Instance->UserPointer = Target;
				}

				return Target;
			}
			Compute::SoftBody* SoftBody::GetBody()
			{
				return Instance;
			}
			std::vector<Compute::Vertex>& SoftBody::GetVertices()
			{
				return Vertices;
			}
			std::vector<int>& SoftBody::GetIndices()
			{
				return Indices;
			}

			SliderConstraint::SliderConstraint(Entity* Ref) : Component(Ref, ActorSet::None), Instance(nullptr), Connection(nullptr)
			{
			}
			SliderConstraint::~SliderConstraint()
			{
				TH_RELEASE(Instance);
			}
			void SliderConstraint::Deserialize(ContentManager* Content, Core::Schema* Node)
			{
				TH_ASSERT_V(Content != nullptr, "content manager should be set");
				TH_ASSERT_V(Node != nullptr, "schema should be set");

				bool Extended, Ghost, Linear;
				Series::Unpack(Node->Find("extended"), &Extended);
				Series::Unpack(Node->Find("collision-state"), &Ghost);
				Series::Unpack(Node->Find("linear-state"), &Linear);

				if (!Extended)
					return;

				int64_t ConnectionId = -1;
				if (Series::Unpack(Node->Find("connection"), &ConnectionId))
				{
					IdxSnapshot* Snapshot = Parent->GetScene()->Snapshot;
					if (Snapshot != nullptr)
					{
						auto It = Snapshot->From.find(ConnectionId);
						if (It != Snapshot->From.end())
							Connection = It->second;
					}
				}

				Create(Connection, Ghost, Linear);
				if (!Instance)
					return;

				float AngularMotorVelocity;
				if (Series::Unpack(Node->Find("angular-motor-velocity"), &AngularMotorVelocity))
					Instance->SetAngularMotorVelocity(AngularMotorVelocity);

				float LinearMotorVelocity;
				if (Series::Unpack(Node->Find("linear-motor-velocity"), &LinearMotorVelocity))
					Instance->SetLinearMotorVelocity(LinearMotorVelocity);

				float UpperLinearLimit;
				if (Series::Unpack(Node->Find("upper-linear-limit"), &UpperLinearLimit))
					Instance->SetUpperLinearLimit(UpperLinearLimit);

				float LowerLinearLimit;
				if (Series::Unpack(Node->Find("lower-linear-limit"), &LowerLinearLimit))
					Instance->SetLowerLinearLimit(LowerLinearLimit);

				float AngularDampingDirection;
				if (Series::Unpack(Node->Find("angular-damping-direction"), &AngularDampingDirection))
					Instance->SetAngularDampingDirection(AngularDampingDirection);

				float LinearDampingDirection;
				if (Series::Unpack(Node->Find("linear-damping-direction"), &LinearDampingDirection))
					Instance->SetLinearDampingDirection(LinearDampingDirection);

				float AngularDampingLimit;
				if (Series::Unpack(Node->Find("angular-damping-limit"), &AngularDampingLimit))
					Instance->SetAngularDampingLimit(AngularDampingLimit);

				float LinearDampingLimit;
				if (Series::Unpack(Node->Find("linear-damping-limit"), &LinearDampingLimit))
					Instance->SetLinearDampingLimit(LinearDampingLimit);

				float AngularDampingOrtho;
				if (Series::Unpack(Node->Find("angular-damping-ortho"), &AngularDampingOrtho))
					Instance->SetAngularDampingOrtho(AngularDampingOrtho);

				float LinearDampingOrtho;
				if (Series::Unpack(Node->Find("linear-damping-ortho"), &LinearDampingOrtho))
					Instance->SetLinearDampingOrtho(LinearDampingOrtho);

				float UpperAngularLimit;
				if (Series::Unpack(Node->Find("upper-angular-limit"), &UpperAngularLimit))
					Instance->SetUpperAngularLimit(UpperAngularLimit);

				float LowerAngularLimit;
				if (Series::Unpack(Node->Find("lower-angular-limit"), &LowerAngularLimit))
					Instance->SetLowerAngularLimit(LowerAngularLimit);

				float MaxAngularMotorForce;
				if (Series::Unpack(Node->Find("max-angular-motor-force"), &MaxAngularMotorForce))
					Instance->SetMaxAngularMotorForce(MaxAngularMotorForce);

				float MaxLinearMotorForce;
				if (Series::Unpack(Node->Find("max-linear-motor-force"), &MaxLinearMotorForce))
					Instance->SetMaxLinearMotorForce(MaxLinearMotorForce);

				float AngularRestitutionDirection;
				if (Series::Unpack(Node->Find("angular-restitution-direction"), &AngularRestitutionDirection))
					Instance->SetAngularRestitutionDirection(AngularRestitutionDirection);

				float LinearRestitutionDirection;
				if (Series::Unpack(Node->Find("linear-restitution-direction"), &LinearRestitutionDirection))
					Instance->SetLinearRestitutionDirection(LinearRestitutionDirection);

				float AngularRestitutionLimit;
				if (Series::Unpack(Node->Find("angular-restitution-limit"), &AngularRestitutionLimit))
					Instance->SetAngularRestitutionLimit(AngularRestitutionLimit);

				float LinearRestitutionLimit;
				if (Series::Unpack(Node->Find("linear-restitution-limit"), &LinearRestitutionLimit))
					Instance->SetLinearRestitutionLimit(LinearRestitutionLimit);

				float AngularRestitutionOrtho;
				if (Series::Unpack(Node->Find("angular-restitution-ortho"), &AngularRestitutionOrtho))
					Instance->SetAngularRestitutionOrtho(AngularRestitutionOrtho);

				float LinearRestitutionOrtho;
				if (Series::Unpack(Node->Find("linear-restitution-ortho"), &LinearRestitutionOrtho))
					Instance->SetLinearRestitutionOrtho(LinearRestitutionOrtho);

				float AngularSoftnessDirection;
				if (Series::Unpack(Node->Find("angular-softness-direction"), &AngularSoftnessDirection))
					Instance->SetAngularSoftnessDirection(AngularSoftnessDirection);

				float LinearSoftnessDirection;
				if (Series::Unpack(Node->Find("linear-softness-direction"), &LinearSoftnessDirection))
					Instance->SetLinearSoftnessDirection(LinearSoftnessDirection);

				float AngularSoftnessLimit;
				if (Series::Unpack(Node->Find("angular-softness-limit"), &AngularSoftnessLimit))
					Instance->SetAngularSoftnessLimit(AngularSoftnessLimit);

				float LinearSoftnessLimit;
				if (Series::Unpack(Node->Find("linear-softness-limit"), &LinearSoftnessLimit))
					Instance->SetLinearSoftnessLimit(LinearSoftnessLimit);

				float AngularSoftnessOrtho;
				if (Series::Unpack(Node->Find("angular-softness-ortho"), &AngularSoftnessOrtho))
					Instance->SetAngularSoftnessOrtho(AngularSoftnessOrtho);

				float LinearSoftnessOrtho;
				if (Series::Unpack(Node->Find("linear-softness-ortho"), &LinearSoftnessOrtho))
					Instance->SetLinearSoftnessOrtho(LinearSoftnessOrtho);

				bool PoweredAngularMotor;
				if (Series::Unpack(Node->Find("powered-angular-motor"), &PoweredAngularMotor))
					Instance->SetPoweredAngularMotor(PoweredAngularMotor);

				bool PoweredLinearMotor;
				if (Series::Unpack(Node->Find("powered-linear-motor"), &PoweredLinearMotor))
					Instance->SetPoweredLinearMotor(PoweredLinearMotor);

				bool Enabled;
				if (Series::Unpack(Node->Find("enabled"), &Enabled))
					Instance->SetEnabled(Enabled);
			}
			void SliderConstraint::Serialize(ContentManager* Content, Core::Schema* Node)
			{
				TH_ASSERT_V(Content != nullptr, "content manager should be set");
				TH_ASSERT_V(Node != nullptr, "schema should be set");

				Series::Pack(Node->Set("extended"), Instance != nullptr);
				if (!Instance)
					return;

				int64_t ConnectionId = -1;
				if (Connection != nullptr)
				{
					IdxSnapshot* Snapshot = Parent->GetScene()->Snapshot;
					if (Snapshot != nullptr)
					{
						auto It = Snapshot->To.find(Connection);
						if (It != Snapshot->To.end())
							ConnectionId = (int64_t)It->second;
					}
				}

				Series::Pack(Node->Set("collision-state"), Instance->GetState().Collisions);
				Series::Pack(Node->Set("linear-state"), Instance->GetState().Linear);
				Series::Pack(Node->Set("connection"), ConnectionId);
				Series::Pack(Node->Set("angular-motor-velocity"), Instance->GetAngularMotorVelocity());
				Series::Pack(Node->Set("linear-motor-velocity"), Instance->GetLinearMotorVelocity());
				Series::Pack(Node->Set("upper-linear-limit"), Instance->GetUpperLinearLimit());
				Series::Pack(Node->Set("lower-linear-limit"), Instance->GetLowerLinearLimit());
				Series::Pack(Node->Set("breaking-impulse-threshold"), Instance->GetBreakingImpulseThreshold());
				Series::Pack(Node->Set("angular-damping-direction"), Instance->GetAngularDampingDirection());
				Series::Pack(Node->Set("linear-amping-direction"), Instance->GetLinearDampingDirection());
				Series::Pack(Node->Set("angular-damping-limit"), Instance->GetAngularDampingLimit());
				Series::Pack(Node->Set("linear-damping-limit"), Instance->GetLinearDampingLimit());
				Series::Pack(Node->Set("angular-damping-ortho"), Instance->GetAngularDampingOrtho());
				Series::Pack(Node->Set("linear-damping-ortho"), Instance->GetLinearDampingOrtho());
				Series::Pack(Node->Set("upper-angular-limit"), Instance->GetUpperAngularLimit());
				Series::Pack(Node->Set("lower-angular-limit"), Instance->GetLowerAngularLimit());
				Series::Pack(Node->Set("max-angular-motor-force"), Instance->GetMaxAngularMotorForce());
				Series::Pack(Node->Set("max-linear-motor-force"), Instance->GetMaxLinearMotorForce());
				Series::Pack(Node->Set("angular-restitution-direction"), Instance->GetAngularRestitutionDirection());
				Series::Pack(Node->Set("linear-restitution-direction"), Instance->GetLinearRestitutionDirection());
				Series::Pack(Node->Set("angular-restitution-limit"), Instance->GetAngularRestitutionLimit());
				Series::Pack(Node->Set("linear-restitution-limit"), Instance->GetLinearRestitutionLimit());
				Series::Pack(Node->Set("angular-restitution-ortho"), Instance->GetAngularRestitutionOrtho());
				Series::Pack(Node->Set("linear-restitution-ortho"), Instance->GetLinearRestitutionOrtho());
				Series::Pack(Node->Set("angular-softness-direction"), Instance->GetAngularSoftnessDirection());
				Series::Pack(Node->Set("linear-softness-direction"), Instance->GetLinearSoftnessDirection());
				Series::Pack(Node->Set("angular-softness-limit"), Instance->GetAngularSoftnessLimit());
				Series::Pack(Node->Set("linear-softness-limit"), Instance->GetLinearSoftnessLimit());
				Series::Pack(Node->Set("angular-softness-ortho"), Instance->GetAngularSoftnessOrtho());
				Series::Pack(Node->Set("linear-softness-ortho"), Instance->GetLinearSoftnessOrtho());
				Series::Pack(Node->Set("powered-angular-motor"), Instance->GetPoweredAngularMotor());
				Series::Pack(Node->Set("powered-linear-motor"), Instance->GetPoweredLinearMotor());
				Series::Pack(Node->Set("enabled"), Instance->IsEnabled());
			}
			void SliderConstraint::Create(Entity* Other, bool IsGhosted, bool IsLinear)
			{
				TH_ASSERT_V(Parent != Other, "parent should not be equal to other");
				SceneGraph* Scene = Parent->GetScene();
				Scene->Transaction([this, Scene, Other, IsGhosted, IsLinear]()
				{
					Connection = Other;
					if (!Connection)
						return;

					RigidBody* FirstBody = Parent->GetComponent<RigidBody>();
					RigidBody* SecondBody = Connection->GetComponent<RigidBody>();
					if (!FirstBody || !SecondBody)
						return;

					Compute::SConstraint::Desc I;
					I.TargetA = FirstBody->GetBody();
					I.TargetB = SecondBody->GetBody();
					I.Collisions = !IsGhosted;
					I.Linear = IsLinear;

					if (I.TargetA && I.TargetB)
					{
						TH_RELEASE(Instance);
						Instance = Scene->GetSimulator()->CreateSliderConstraint(I);
					}
				});
			}
			void SliderConstraint::Clear()
			{
				if (!Instance)
					return;

				SceneGraph* Scene = Parent->GetScene();
				Scene->Transaction([this]()
				{
					TH_CLEAR(Instance);
					Connection = nullptr;
				});
			}
			Component* SliderConstraint::Copy(Entity* New) const
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

				Compute::SConstraint::Desc I(Instance->GetState());
				Instance->GetState().TargetA = FirstBody->GetBody();
				Target->Instance = (Compute::SConstraint*)Instance->Copy();
				Instance->GetState() = I;

				return Target;
			}
			Compute::SConstraint* SliderConstraint::GetConstraint() const
			{
				return Instance;
			}
			Entity* SliderConstraint::GetConnection() const
			{
				return Connection;
			}

			Acceleration::Acceleration(Entity* Ref) : Component(Ref, ActorSet::Update)
			{
			}
			void Acceleration::Deserialize(ContentManager* Content, Core::Schema* Node)
			{
				TH_ASSERT_V(Content != nullptr, "content manager should be set");
				TH_ASSERT_V(Node != nullptr, "schema should be set");

				Series::Unpack(Node->Find("amplitude-velocity"), &AmplitudeVelocity);
				Series::Unpack(Node->Find("amplitude-torque"), &AmplitudeTorque);
				Series::Unpack(Node->Find("constant-velocity"), &ConstantVelocity);
				Series::Unpack(Node->Find("constant-torque"), &ConstantTorque);
				Series::Unpack(Node->Find("constant-center"), &ConstantCenter);
				Series::Unpack(Node->Find("kinematic"), &Kinematic);
			}
			void Acceleration::Serialize(ContentManager* Content, Core::Schema* Node)
			{
				TH_ASSERT_V(Content != nullptr, "content manager should be set");
				TH_ASSERT_V(Node != nullptr, "schema should be set");

				Series::Pack(Node->Set("amplitude-velocity"), AmplitudeVelocity);
				Series::Pack(Node->Set("amplitude-torque"), AmplitudeTorque);
				Series::Pack(Node->Set("constant-velocity"), ConstantVelocity);
				Series::Pack(Node->Set("constant-torque"), ConstantTorque);
				Series::Pack(Node->Set("constant-center"), ConstantCenter);
				Series::Pack(Node->Set("kinematic"), Kinematic);
			}
			void Acceleration::Activate(Component* New)
			{
				if (RigidBody != nullptr)
					return;

				Components::RigidBody* Component = Parent->GetComponent<Components::RigidBody>();
				if (Component != nullptr)
					RigidBody = Component->GetBody();
			}
			void Acceleration::Update(Core::Timer* Time)
			{
				if (!RigidBody)
					return;

				float DeltaTime = (float)Time->GetDeltaTime();
				if (Kinematic)
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
			Component* Acceleration::Copy(Entity* New) const
			{
				Acceleration* Target = new Acceleration(New);
				Target->Kinematic = Kinematic;
				Target->AmplitudeTorque = AmplitudeTorque;
				Target->AmplitudeVelocity = AmplitudeVelocity;
				Target->ConstantCenter = ConstantCenter;
				Target->ConstantTorque = ConstantTorque;
				Target->ConstantVelocity = ConstantVelocity;
				Target->RigidBody = RigidBody;

				return Target;
			}
			Compute::RigidBody* Acceleration::GetBody() const
			{
				return RigidBody;
			}

			Model::Model(Entity* Ref) : Drawable(Ref, ActorSet::None, Model::GetTypeId(), true)
			{
			}
			Model::~Model()
			{
				TH_RELEASE(Instance);
			}
			void Model::Deserialize(ContentManager* Content, Core::Schema* Node)
			{
				TH_ASSERT_V(Content != nullptr, "content manager should be set");
				TH_ASSERT_V(Node != nullptr, "schema should be set");

				std::string Path;
				if (Series::Unpack(Node->Find("model"), &Path))
				{
					TH_RELEASE(Instance);
					Instance = Content->Load<Graphics::Model>(Path);
				}

				SceneGraph* Scene = Parent->GetScene();
				std::vector<Core::Schema*> Slots = Node->FetchCollection("materials.material");
				for (auto&& Material : Slots)
				{
					uint64_t Slot = 0;
					Series::Unpack(Material->Find("name"), &Path);
					Series::Unpack(Material->Find("slot"), &Slot);

					Graphics::MeshBuffer* Surface = (Instance ? Instance->FindMesh(Path) : nullptr);
					if (Surface != nullptr)
						Materials[Surface] = Scene->GetMaterial(Slot);
				}

				uint32_t NewCategory = (uint32_t)GeoCategory::Opaque;
				Series::Unpack(Node->Find("texcoord"), &TexCoord);
				Series::Unpack(Node->Find("static"), &Static);
				Series::Unpack(Node->Find("category"), &NewCategory);
				SetCategory((GeoCategory)NewCategory);
			}
			void Model::Serialize(ContentManager* Content, Core::Schema* Node)
			{
				TH_ASSERT_V(Content != nullptr, "content manager should be set");
				TH_ASSERT_V(Node != nullptr, "schema should be set");

				AssetCache* Asset = Content->Find<Graphics::Model>(Instance);
				if (Asset != nullptr)
					Series::Pack(Node->Set("model"), Asset->Path);

				Core::Schema* Slots = Node->Set("materials", Core::Var::Array());
				for (auto&& Slot : Materials)
				{
					if (Slot.first != nullptr)
					{
						Core::Schema* Material = Slots->Set("material");
						Series::Pack(Material->Set("name"), ((Graphics::MeshBuffer*)Slot.first)->Name);

						if (Slot.second != nullptr)
							Series::Pack(Material->Set("slot"), Slot.second->Slot);
					}
				}

				Series::Pack(Node->Set("texcoord"), TexCoord);
				Series::Pack(Node->Set("category"), (uint32_t)GetCategory());
				Series::Pack(Node->Set("static"), Static);
			}
			void Model::SetDrawable(Graphics::Model* Drawable)
			{
				TH_RELEASE(Instance);
				Instance = Drawable;
			}
			float Model::GetVisibility(const Viewer& View, float Distance) const
			{
				return Instance ? Component::GetVisibility(View, Distance) : 0.0f;
			}
			size_t Model::GetUnitBounds(Compute::Vector3& Min, Compute::Vector3& Max) const
			{
				if (!Instance)
					return BOX_NONE;

				Min = Instance->Min * 0.5f;
				Max = Instance->Max * 0.5f;
				return BOX_GEOMETRY;
			}
			Component* Model::Copy(Entity* New) const
			{
				Model* Target = new Model(New);
				Target->SetCategory(GetCategory());
				Target->Instance = Instance;
				Target->Materials = Materials;

				if (Target->Instance != nullptr)
					Target->Instance->AddRef();

				return Target;
			}
			Graphics::Model* Model::GetDrawable()
			{
				return Instance;
			}

			Skin::Skin(Entity* Ref) : Drawable(Ref, ActorSet::Synchronize, Skin::GetTypeId(), true)
			{
			}
			Skin::~Skin()
			{
				TH_RELEASE(Instance);
			}
			void Skin::Deserialize(ContentManager* Content, Core::Schema* Node)
			{
				TH_ASSERT_V(Content != nullptr, "content manager should be set");
				TH_ASSERT_V(Node != nullptr, "schema should be set");

				std::string Path;
				if (Series::Unpack(Node->Find("skin-model"), &Path))
				{
					TH_RELEASE(Instance);
					Instance = Content->Load<Graphics::SkinModel>(Path);
				}

				SceneGraph* Scene = Parent->GetScene();
				std::vector<Core::Schema*> Slots = Node->FetchCollection("materials.material");
				for (auto&& Material : Slots)
				{
					uint64_t Slot = 0;
					Series::Unpack(Material->Find("name"), &Path);
					Series::Unpack(Material->Find("slot"), &Slot);

					Graphics::SkinMeshBuffer* Surface = (Instance ? Instance->FindMesh(Path) : nullptr);
					if (Surface != nullptr)
						Materials[Surface] = Scene->GetMaterial(Slot);
				}


				std::vector<Core::Schema*> Poses = Node->FetchCollection("poses.pose");
				for (auto&& Pose : Poses)
				{
					int64_t Index;
					Series::Unpack(Pose->Find("index"), &Index);

					auto& Offset = Skeleton.Pose[Index];
					Series::Unpack(Pose->Find("position"), &Offset.Position);
					Series::Unpack(Pose->Find("rotation"), &Offset.Rotation);
				}

				uint32_t NewCategory = (uint32_t)GeoCategory::Opaque;
				Series::Unpack(Node->Find("texcoord"), &TexCoord);
				Series::Unpack(Node->Find("static"), &Static);
				Series::Unpack(Node->Find("category"), &NewCategory);
				SetCategory((GeoCategory)NewCategory);
			}
			void Skin::Serialize(ContentManager* Content, Core::Schema* Node)
			{
				TH_ASSERT_V(Content != nullptr, "content manager should be set");
				TH_ASSERT_V(Node != nullptr, "schema should be set");

				AssetCache* Asset = Content->Find<Graphics::SkinModel>(Instance);
				if (Asset != nullptr)
					Series::Pack(Node->Set("skin-model"), Asset->Path);

				Core::Schema* Slots = Node->Set("materials", Core::Var::Array());
				for (auto&& Slot : Materials)
				{
					if (Slot.first != nullptr)
					{
						Core::Schema* Material = Slots->Set("material");
						Series::Pack(Material->Set("name"), ((Graphics::MeshBuffer*)Slot.first)->Name);

						if (Slot.second != nullptr)
							Series::Pack(Material->Set("slot"), Slot.second->Slot);
					}
				}

				Series::Pack(Node->Set("texcoord"), TexCoord);
				Series::Pack(Node->Set("category"), (uint32_t)GetCategory());
				Series::Pack(Node->Set("static"), Static);

				Core::Schema* Poses = Node->Set("poses", Core::Var::Array());
				for (auto&& Pose : Skeleton.Pose)
				{
					Core::Schema* Value = Poses->Set("pose");
					Series::Pack(Value->Set("index"), Pose.first);
					Series::Pack(Value->Set("position"), Pose.second.Position);
					Series::Pack(Value->Set("rotation"), Pose.second.Rotation);
				}
			}
			void Skin::Synchronize(Core::Timer* Time)
			{
				if (Instance != nullptr)
					Instance->ComputePose(&Skeleton);
			}
			void Skin::SetDrawable(Graphics::SkinModel* Drawable)
			{
				TH_RELEASE(Instance);
				Instance = Drawable;
			}
			float Skin::GetVisibility(const Viewer& View, float Distance) const
			{
				return Instance ? Component::GetVisibility(View, Distance) : 0.0f;
			}
			size_t Skin::GetUnitBounds(Compute::Vector3& Min, Compute::Vector3& Max) const
			{
				if (!Instance)
					return BOX_NONE;

				Min = Instance->Min * 0.5f;
				Max = Instance->Max * 0.5f;
				return BOX_GEOMETRY;
			}
			Component* Skin::Copy(Entity* New) const
			{
				Skin* Target = new Skin(New);
				Target->SetCategory(GetCategory());
				Target->Instance = Instance;
				Target->Materials = Materials;

				if (Target->Instance != nullptr)
					Target->Instance->AddRef();

				return Target;
			}
			Graphics::SkinModel* Skin::GetDrawable()
			{
				return Instance;
			}

			Emitter::Emitter(Entity* Ref) : Drawable(Ref, ActorSet::None, Emitter::GetTypeId(), false)
			{
			}
			Emitter::~Emitter()
			{
				TH_RELEASE(Instance);
			}
			void Emitter::Deserialize(ContentManager* Content, Core::Schema* Node)
			{
				TH_ASSERT_V(Content != nullptr, "content manager should be set");
				TH_ASSERT_V(Node != nullptr, "schema should be set");

				SceneGraph* Scene = Parent->GetScene(); uint64_t Slot = -1;
				if (Series::Unpack(Node->Find("material"), &Slot))
					SetMaterial(nullptr, Scene->GetMaterial((uint64_t)Slot));

				uint32_t NewCategory = (uint32_t)GeoCategory::Opaque;
				Series::Unpack(Node->Find("category"), &NewCategory);
				Series::Unpack(Node->Find("quad-based"), &QuadBased);
				Series::Unpack(Node->Find("connected"), &Connected);
				Series::Unpack(Node->Find("static"), &Static);
				Series::Unpack(Node->Find("min"), &Min);
				Series::Unpack(Node->Find("max"), &Max);
				SetCategory((GeoCategory)NewCategory);

				uint64_t Limit;
				if (Series::Unpack(Node->Find("limit"), &Limit))
				{
					std::vector<Compute::ElementVertex> Vertices;
					if (Instance != nullptr)
					{
						Scene->GetDevice()->UpdateBufferSize(Instance, Limit);
						if (Series::Unpack(Node->Find("elements"), &Vertices))
						{
							Instance->GetArray()->Reserve(Vertices.size());
							for (auto&& Vertex : Vertices)
								Instance->GetArray()->Add(Vertex);
						}
					}
				}
			}
			void Emitter::Serialize(ContentManager* Content, Core::Schema* Node)
			{
				TH_ASSERT_V(Content != nullptr, "content manager should be set");
				TH_ASSERT_V(Node != nullptr, "schema should be set");

				Material* Slot = GetMaterial();
				if (Slot != nullptr)
					Series::Pack(Node->Set("material"), Slot->Slot);

				Series::Pack(Node->Set("category"), (uint32_t)GetCategory());
				Series::Pack(Node->Set("quad-based"), QuadBased);
				Series::Pack(Node->Set("connected"), Connected);
				Series::Pack(Node->Set("static"), Static);
				Series::Pack(Node->Set("min"), Min);
				Series::Pack(Node->Set("max"), Max);

				if (Instance != nullptr)
				{
					auto* fArray = Instance->GetArray();
					std::vector<Compute::ElementVertex> Vertices;
					Vertices.reserve(fArray->Size());

					for (auto It = fArray->Begin(); It != fArray->End(); It++)
						Vertices.emplace_back(*It);

					Series::Pack(Node->Set("limit"), Instance->GetElementLimit());
					Series::Pack(Node->Set("elements"), Vertices);
				}
			}
			void Emitter::Activate(Component* New)
			{
				if (Instance != nullptr)
					return;

				SceneGraph* Scene = Parent->GetScene();
				Graphics::InstanceBuffer::Desc I = Graphics::InstanceBuffer::Desc();
				I.ElementLimit = 1 << 10;

				Instance = Scene->GetDevice()->CreateInstanceBuffer(I);
			}
			size_t Emitter::GetUnitBounds(Compute::Vector3& _Min, Compute::Vector3& _Max) const
			{
				if (!Instance)
					return BOX_NONE;

				_Min = Min * 0.5f;
				_Max = Max * 0.5f;
				return BOX_DYNAMIC;
			}
			Component* Emitter::Copy(Entity* New) const
			{
				Emitter* Target = new Emitter(New);
				Target->SetCategory(GetCategory());
				Target->Connected = Connected;
				Target->Materials = Materials;
				Target->Min = Min;
				Target->Max = Max;
				Target->Instance->GetArray()->Copy(*Instance->GetArray());

				return Target;
			}
			Graphics::InstanceBuffer* Emitter::GetBuffer()
			{
				return Instance;
			}

			Decal::Decal(Entity* Ref) : Drawable(Ref, ActorSet::Synchronize, Decal::GetTypeId(), false)
			{
			}
			void Decal::Deserialize(ContentManager* Content, Core::Schema* Node)
			{
				TH_ASSERT_V(Content != nullptr, "content manager should be set");
				TH_ASSERT_V(Node != nullptr, "schema should be set");

				uint64_t Slot = -1;
				if (Series::Unpack(Node->Find("material"), &Slot))
					SetMaterial(nullptr, Parent->GetScene()->GetMaterial((uint64_t)Slot));

				uint32_t NewCategory = (uint32_t)GeoCategory::Opaque;
				Series::Unpack(Node->Find("texcoord"), &TexCoord);
				Series::Unpack(Node->Find("static"), &Static);
				Series::Unpack(Node->Find("category"), &NewCategory);
				SetCategory((GeoCategory)NewCategory);
			}
			void Decal::Serialize(ContentManager* Content, Core::Schema* Node)
			{
				TH_ASSERT_V(Content != nullptr, "content manager should be set");
				TH_ASSERT_V(Node != nullptr, "schema should be set");

				Material* Slot = GetMaterial();
				if (Slot != nullptr)
					Series::Pack(Node->Set("material"), Slot->Slot);

				Series::Pack(Node->Set("texcoord"), TexCoord);
				Series::Pack(Node->Set("static"), Static);
				Series::Pack(Node->Set("category"), (uint32_t)GetCategory());
			}
			float Decal::GetVisibility(const Viewer& View, float Distance) const
			{
				return GetVisibilityRadius(Parent, View, Distance);
			}
			Component* Decal::Copy(Entity* New) const
			{
				Decal* Target = new Decal(New);
				Target->Materials = Materials;

				return Target;
			}

			SkinAnimator::SkinAnimator(Entity* Ref) : Component(Ref, ActorSet::Synchronize)
			{
				Current.Pose.resize(96);
				Bind.Pose.resize(96);
				Default.Pose.resize(96);
			}
			SkinAnimator::~SkinAnimator()
			{
			}
			void SkinAnimator::Deserialize(ContentManager* Content, Core::Schema* Node)
			{
				TH_ASSERT_V(Content != nullptr, "content manager should be set");
				TH_ASSERT_V(Node != nullptr, "schema should be set");

				std::string Path;
				if (!Series::Unpack(Node->Find("path"), &Path))
					Series::Unpack(Node->Find("animation"), &Clips);
				else
					GetAnimation(Content, Path);

				Series::Unpack(Node->Find("state"), &State);
				Series::Unpack(Node->Find("bind"), &Bind);
				Series::Unpack(Node->Find("current"), &Current);
			}
			void SkinAnimator::Serialize(ContentManager* Content, Core::Schema* Node)
			{
				TH_ASSERT_V(Content != nullptr, "content manager should be set");
				TH_ASSERT_V(Node != nullptr, "schema should be set");

				if (!Reference.empty())
					Series::Pack(Node->Set("path"), Reference);
				else
					Series::Pack(Node->Set("animation"), Clips);

				Series::Pack(Node->Set("state"), State);
				Series::Pack(Node->Set("bind"), Bind);
				Series::Pack(Node->Set("current"), Current);
			}
			void SkinAnimator::Activate(Component* New)
			{
				Components::Skin* Base = Parent->GetComponent<Components::Skin>();
				if (Base != nullptr && Base->GetDrawable() != nullptr)
				{
					Instance = Base;
					Instance->Skeleton.GetPose(Instance->GetDrawable(), &Default.Pose);
				}
				else
					Instance = nullptr;

				SetActive(Instance != nullptr);
			}
			void SkinAnimator::Synchronize(Core::Timer* Time)
			{
				if (!Parent->GetScene()->IsActive())
					return;

				if (!State.Blended)
				{
					if (State.Paused || State.Clip < 0 || State.Clip >= Clips.size() || State.Frame < 0 || State.Frame >= Clips[State.Clip].Keys.size())
						return;

					Compute::SkinAnimatorClip* Clip = &Clips[State.Clip];
					auto& NextKey = Clip->Keys[State.Frame + 1 >= Clip->Keys.size() ? 0 : State.Frame + 1];
					auto& PrevKey = Clip->Keys[State.Frame];

					State.Duration = Clip->Duration;
					State.Rate = Clip->Rate * NextKey.Time;
					State.Time = Compute::Mathf::Min(State.Time + State.Rate * (float)Time->GetDeltaTime() / State.Duration, State.Duration);

					for (auto&& Pose : Instance->Skeleton.Pose)
					{
						Compute::AnimatorKey* Prev = &PrevKey.Pose[Pose.first];
						Compute::AnimatorKey* Next = &NextKey.Pose[Pose.first];
						Compute::AnimatorKey* Set = &Current.Pose[Pose.first];
						float T = Compute::Mathf::Min(State.Time / State.Duration, 1.0f);

						Set->Position = Prev->Position.Lerp(Next->Position, T);
						Pose.second.Position = Set->Position;

						Set->Rotation = Prev->Rotation.aLerp(Next->Rotation, T);
						Pose.second.Rotation = Set->Rotation;
					}

					if (State.Time >= State.Duration)
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
				else if (State.Time < State.Duration)
				{
					Compute::SkinAnimatorKey* Key = (IsExists(State.Clip, State.Frame) ? GetFrame(State.Clip, State.Frame) : nullptr);
					if (!Key)
						Key = &Bind;

					State.Time = Compute::Mathf::Min(State.Time + State.Rate * (float)Time->GetDeltaTime() / State.Duration, State.Duration);
					float T = Compute::Mathf::Min(State.Time / State.Duration, 1.0f);

					for (auto&& Pose : Instance->Skeleton.Pose)
					{
						Compute::AnimatorKey* Prev = &Current.Pose[Pose.first];
						Compute::AnimatorKey* Next = &Key->Pose[Pose.first];

						Pose.second.Position = Prev->Position.Lerp(Next->Position, T);
						Pose.second.Rotation = Prev->Rotation.aLerp(Next->Rotation, T);
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

					if (State.Duration <= 0.0f)
					{
						State.Duration = CurrentClip->Duration;
						State.Rate = CurrentClip->Rate;
					}
				}
				else
					State.Clip = -1;
			}
			bool SkinAnimator::GetAnimation(ContentManager* Content, const std::string& Path)
			{
				TH_ASSERT(Content != nullptr, false, "content manager should be set");
				Core::Schema* Result = Content->Load<Core::Schema>(Path);
				if (!Result)
					return false;

				ClearAnimation();
				if (Series::Unpack(Result, &Clips))
					Reference = Core::Parser(Path).Replace(Content->GetEnvironment(), "./").Replace('\\', '/').R();

				TH_RELEASE(Result);
				return true;
			}
			void SkinAnimator::GetPose(Compute::SkinAnimatorKey* Result)
			{
				TH_ASSERT_V(Result != nullptr, "result should be set");
				Result->Pose.resize(Default.Pose.size());
				Result->Time = Default.Time;

				for (auto&& Pose : Instance->Skeleton.Pose)
				{
					Compute::AnimatorKey* Frame = &Result->Pose[Pose.first];
					Frame->Position = Pose.second.Position;
					Frame->Rotation = Pose.second.Rotation;
				}
			}
			void SkinAnimator::ClearAnimation()
			{
				Reference.clear();
				Clips.clear();
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

				if (!IsExists(State.Clip, State.Frame))
					return;

				if (State.Clip >= 0 && State.Clip < Clips.size())
				{
					Compute::SkinAnimatorClip* CurrentClip = &Clips[State.Clip];
					if (State.Frame < 0 || State.Frame >= CurrentClip->Keys.size())
						State.Frame = -1;
				}
				else
					State.Clip = -1;

				GetPose(&Bind);
				Current = Bind;

				if (!IsPosed(State.Clip, State.Frame))
					BlendAnimation(State.Clip, State.Frame);
			}
			bool SkinAnimator::IsPosed(int64_t Clip, int64_t Frame_)
			{
				Compute::SkinAnimatorKey* Key = (IsExists(Clip, Frame_) ? GetFrame(Clip, Frame_) : nullptr);
				if (!Key)
					Key = &Bind;

				for (auto&& Pose : Instance->Skeleton.Pose)
				{
					Compute::AnimatorKey* Frame = &Key->Pose[Pose.first];
					if (Pose.second.Position != Frame->Position || Pose.second.Rotation != Frame->Rotation)
						return false;
				}

				return true;
			}
			bool SkinAnimator::IsExists(int64_t Clip)
			{
				return Clip >= 0 && Clip < Clips.size();
			}
			bool SkinAnimator::IsExists(int64_t Clip, int64_t Frame)
			{
				if (!IsExists(Clip))
					return false;

				return Frame >= 0 && Frame < Clips[Clip].Keys.size();
			}
			Compute::SkinAnimatorKey* SkinAnimator::GetFrame(int64_t Clip, int64_t Frame)
			{
				TH_ASSERT(Clip >= 0 && Clip < Clips.size(), nullptr, "clip index outside of range");
				TH_ASSERT(Frame >= 0 && Frame < Clips[Clip].Keys.size(), nullptr, "frame index outside of range");

				return &Clips[Clip].Keys[Frame];
			}
			std::vector<Compute::SkinAnimatorKey>* SkinAnimator::GetClip(int64_t Clip)
			{
				TH_ASSERT(Clip >= 0 && Clip < Clips.size(), nullptr, "clip index outside of range");
				return &Clips[Clip].Keys;
			}
			std::string SkinAnimator::GetPath()
			{
				return Reference;
			}
			Component* SkinAnimator::Copy(Entity* New) const
			{
				SkinAnimator* Target = new SkinAnimator(New);
				Target->Clips = Clips;
				Target->State = State;
				Target->Bind = Bind;
				Target->Current = Current;

				return Target;
			}
			Skin* SkinAnimator::GetSkin() const
			{
				return Instance;
			}

			KeyAnimator::KeyAnimator(Entity* Ref) : Component(Ref, ActorSet::Synchronize)
			{
			}
			KeyAnimator::~KeyAnimator()
			{
			}
			void KeyAnimator::Deserialize(ContentManager* Content, Core::Schema* Node)
			{
				TH_ASSERT_V(Content != nullptr, "content manager should be set");
				TH_ASSERT_V(Node != nullptr, "schema should be set");

				std::string Path;
				if (!Series::Unpack(Node->Find("path"), &Path))
					Series::Unpack(Node->Find("animation"), &Clips);
				else
					GetAnimation(Content, Path);

				Series::Unpack(Node->Find("state"), &State);
				Series::Unpack(Node->Find("bind"), &Bind);
				Series::Unpack(Node->Find("current"), &Current);
			}
			void KeyAnimator::Serialize(ContentManager* Content, Core::Schema* Node)
			{
				TH_ASSERT_V(Content != nullptr, "content manager should be set");
				TH_ASSERT_V(Node != nullptr, "schema should be set");

				if (Reference.empty())
					Series::Pack(Node->Set("animation"), Clips);
				else
					Series::Pack(Node->Set("path"), Reference);

				Series::Pack(Node->Set("state"), State);
				Series::Pack(Node->Set("bind"), Bind);
				Series::Pack(Node->Set("current"), Current);
			}
			void KeyAnimator::Synchronize(Core::Timer* Time)
			{
				if (!Parent->GetScene()->IsActive())
					return;

				auto* Transform = Parent->GetTransform();
				if (!State.Blended)
				{
					if (State.Paused || State.Clip < 0 || State.Clip >= Clips.size() || State.Frame < 0 || State.Frame >= Clips[State.Clip].Keys.size())
						return;

					Compute::KeyAnimatorClip* Clip = &Clips[State.Clip];
					Compute::AnimatorKey& NextKey = Clip->Keys[State.Frame + 1 >= Clip->Keys.size() ? 0 : State.Frame + 1];
					Compute::AnimatorKey& PrevKey = Clip->Keys[State.Frame];

					State.Duration = Clip->Duration;
					State.Rate = Clip->Rate * NextKey.Time;
					State.Time = Compute::Mathf::Min(State.Time + State.Rate * (float)Time->GetDeltaTime() / State.Duration, State.Duration);

					float T = Compute::Mathf::Min(State.Time / State.Duration, 1.0f);
					Transform->SetPosition(Current.Position = PrevKey.Position.Lerp(NextKey.Position, T));
					Transform->SetRotation(Current.Rotation = PrevKey.Rotation.aLerp(NextKey.Rotation, T));
					Transform->SetScale(Current.Scale = PrevKey.Scale.Lerp(NextKey.Scale, T));

					if (State.Time >= State.Duration)
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
				else if (State.Time < State.Duration)
				{
					Compute::AnimatorKey* Key = (IsExists(State.Clip, State.Frame) ? GetFrame(State.Clip, State.Frame) : nullptr);
					if (!Key)
						Key = &Bind;

					if (State.Paused)
						return;

					State.Time = Compute::Mathf::Min(State.Time + State.Rate * (float)Time->GetDeltaTime() / State.Duration, State.Duration);
					float T = Compute::Mathf::Min(State.Time / State.Duration, 1.0f);

					Transform->SetPosition(Current.Position.Lerp(Key->Position, T));
					Transform->SetRotation(Current.Rotation.aLerp(Key->Rotation, T));
					Transform->SetScale(Current.Scale.Lerp(Key->Scale, T));
				}
				else
				{
					State.Blended = false;
					State.Time = 0.0f;
				}
			}
			bool KeyAnimator::GetAnimation(ContentManager* Content, const std::string& Path)
			{
				TH_ASSERT(Content != nullptr, false, "content manager should be set");
				Core::Schema* Result = Content->Load<Core::Schema>(Path);
				if (!Result)
					return false;

				ClearAnimation();
				if (Series::Unpack(Result, &Clips))
					Reference = Core::Parser(Path).Replace(Content->GetEnvironment(), "./").Replace('\\', '/').R();

				TH_RELEASE(Result);
				return true;
			}
			void KeyAnimator::GetPose(Compute::AnimatorKey* Result)
			{
				TH_ASSERT_V(Result != nullptr, "result should be set");
				auto& Space = Parent->GetTransform()->GetSpacing();
				Result->Position = Space.Position;
				Result->Rotation = Space.Rotation;
				Result->Scale = Space.Scale;
			}
			void KeyAnimator::ClearAnimation()
			{
				Reference.clear();
				Clips.clear();
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

				if (!IsExists(State.Clip, State.Frame))
					return;

				if (State.Clip >= 0 && State.Clip < Clips.size())
				{
					Compute::KeyAnimatorClip* CurrentClip = &Clips[State.Clip];
					if (State.Frame < 0 || State.Frame >= CurrentClip->Keys.size())
						State.Frame = -1;

					if (State.Duration <= 0.0f)
					{
						State.Duration = CurrentClip->Duration;
						State.Rate = CurrentClip->Rate;
					}
				}
				else
					State.Clip = -1;

				GetPose(&Bind);
				Current = Bind;

				if (!IsPosed(State.Clip, State.Frame))
					BlendAnimation(State.Clip, State.Frame);
			}
			bool KeyAnimator::IsPosed(int64_t Clip, int64_t Frame_)
			{
				Compute::AnimatorKey* Key = (IsExists(Clip, Frame_) ? GetFrame(Clip, Frame_) : nullptr);
				if (!Key)
					Key = &Bind;

				auto& Space = Parent->GetTransform()->GetSpacing();
				return Space.Position == Key->Position && Space.Rotation == Key->Rotation && Space.Scale == Key->Scale;
			}
			bool KeyAnimator::IsExists(int64_t Clip)
			{
				return Clip >= 0 && Clip < Clips.size();
			}
			bool KeyAnimator::IsExists(int64_t Clip, int64_t Frame)
			{
				if (!IsExists(Clip))
					return false;

				return Frame >= 0 && Frame < Clips[Clip].Keys.size();
			}
			Compute::AnimatorKey* KeyAnimator::GetFrame(int64_t Clip, int64_t Frame)
			{
				TH_ASSERT(Clip >= 0 && Clip < Clips.size(), nullptr, "clip index outside of range");
				TH_ASSERT(Frame >= 0 && Frame < Clips[Clip].Keys.size(), nullptr, "frame index outside of range");

				return &Clips[Clip].Keys[Frame];
			}
			std::vector<Compute::AnimatorKey>* KeyAnimator::GetClip(int64_t Clip)
			{
				TH_ASSERT(Clip >= 0 && Clip < Clips.size(), nullptr, "clip index outside of range");
				return &Clips[Clip].Keys;
			}
			std::string KeyAnimator::GetPath()
			{
				return Reference;
			}
			Component* KeyAnimator::Copy(Entity* New) const
			{
				KeyAnimator* Target = new KeyAnimator(New);
				Target->Clips = Clips;
				Target->State = State;
				Target->Bind = Bind;
				Target->Current = Current;

				return Target;
			}

			EmitterAnimator::EmitterAnimator(Entity* Ref) : Component(Ref, ActorSet::Synchronize)
			{
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
			}
			void EmitterAnimator::Deserialize(ContentManager* Content, Core::Schema* Node)
			{
				TH_ASSERT_V(Content != nullptr, "content manager should be set");
				TH_ASSERT_V(Node != nullptr, "schema should be set");

				Series::Unpack(Node->Find("diffuse"), &Diffuse);
				Series::Unpack(Node->Find("position"), &Position);
				Series::Unpack(Node->Find("velocity"), &Velocity);
				Series::Unpack(Node->Find("spawner"), &Spawner);
				Series::Unpack(Node->Find("noise"), &Noise);
				Series::Unpack(Node->Find("rotation-speed"), &RotationSpeed);
				Series::Unpack(Node->Find("scale-speed"), &ScaleSpeed);
				Series::Unpack(Node->Find("simulate"), &Simulate);
			}
			void EmitterAnimator::Serialize(ContentManager* Content, Core::Schema* Node)
			{
				TH_ASSERT_V(Content != nullptr, "content manager should be set");
				TH_ASSERT_V(Node != nullptr, "schema should be set");

				Series::Pack(Node->Set("diffuse"), Diffuse);
				Series::Pack(Node->Set("position"), Position);
				Series::Pack(Node->Set("velocity"), Velocity);
				Series::Pack(Node->Set("spawner"), Spawner);
				Series::Pack(Node->Set("noise"), Noise);
				Series::Pack(Node->Set("rotation-speed"), RotationSpeed);
				Series::Pack(Node->Set("scale-speed"), ScaleSpeed);
				Series::Pack(Node->Set("simulate"), Simulate);
			}
			void EmitterAnimator::Activate(Component* New)
			{
				Base = Parent->GetComponent<Emitter>();
				SetActive(Base != nullptr);
			}
			void EmitterAnimator::Synchronize(Core::Timer* Time)
			{
				if (!Parent->GetScene()->IsActive() || !Simulate || !Base || !Base->GetBuffer())
					return;

				auto* Transform = Parent->GetTransform();
				Core::Pool<Compute::ElementVertex>* Array = Base->GetBuffer()->GetArray();
				Compute::Vector3 Offset = Transform->GetPosition();

				for (int i = 0; i < Spawner.Iterations; i++)
				{
					if (Array->Size() >= Array->Capacity())
						break;

					Compute::Vector3 FPosition = (Base->Connected ? Spawner.Position.Generate() : Spawner.Position.Generate() + Offset);
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

				float DeltaTime = (float)Time->GetDeltaTime();
				if (Noise != 0.0f)
					AccurateSynchronization(DeltaTime);
				else
					FastSynchronization(DeltaTime);
				Transform->MakeDirty();
			}
			void EmitterAnimator::AccurateSynchronization(float DeltaTime)
			{
				Core::Pool<Compute::ElementVertex>* Array = Base->GetBuffer()->GetArray();
				float MinX = 0.0f, MaxX = 0.0f;
				float MinY = 0.0f, MaxY = 0.0f;
				float MinZ = 0.0f, MaxZ = 0.0f;
				float L = Velocity.Length();

				for (auto It = Array->Begin(); It != Array->End(); It++)
				{
					Compute::Vector3 NextVelocity(It->VelocityX, It->VelocityY, It->VelocityZ);
					Compute::Vector3 NextNoise = Spawner.Noise.Generate() / Noise;
					Compute::Vector3 NextPosition(It->PositionX, It->PositionY, It->PositionZ);
					Compute::Vector4 NextDiffuse(It->ColorX, It->ColorY, It->ColorZ, It->ColorW);
					NextPosition += (NextVelocity + Position + NextNoise) * DeltaTime;
					NextDiffuse += Diffuse * DeltaTime;
					memcpy(&It->PositionX, &NextPosition, sizeof(float) * 3);
					memcpy(&It->ColorX, &NextDiffuse, sizeof(float) * 4);
					It->Rotation += (It->Angular + RotationSpeed) * DeltaTime;
					It->Scale += ScaleSpeed * DeltaTime;

					if (L > 0)
					{
						NextVelocity -= (NextVelocity / Velocity) * DeltaTime;
						memcpy(&It->VelocityX, &NextVelocity, sizeof(float) * 3);
					}

					if (It->ColorW <= 0 || It->Scale <= 0)
					{
						Array->RemoveAt(It);
						It--;
						continue;
					}

					if (It->PositionX < MinX)
						MinX = It->PositionX;
					else if (It->PositionX > MaxX)
						MaxX = It->PositionX;

					if (It->PositionY < MinY)
						MinY = It->PositionY;
					else if (It->PositionY > MaxY)
						MaxY = It->PositionY;

					if (It->PositionZ < MinZ)
						MinZ = It->PositionZ;
					else if (It->PositionZ > MaxZ)
						MaxZ = It->PositionZ;
				}

				Base->Min = Compute::Vector3(MinX, MinY, MinZ);
				Base->Max = Compute::Vector3(MaxX, MaxY, MaxZ);
			}
			void EmitterAnimator::FastSynchronization(float DeltaTime)
			{
				Core::Pool<Compute::ElementVertex>* Array = Base->GetBuffer()->GetArray();
				float MinX = 0.0f, MaxX = 0.0f;
				float MinY = 0.0f, MaxY = 0.0f;
				float MinZ = 0.0f, MaxZ = 0.0f;
				float L = Velocity.Length();

				for (auto It = Array->Begin(); It != Array->End(); It++)
				{
					Compute::Vector3 NextVelocity(It->VelocityX, It->VelocityY, It->VelocityZ);
					Compute::Vector3 NextPosition(It->PositionX, It->PositionY, It->PositionZ);
					Compute::Vector4 NextDiffuse(It->ColorX, It->ColorY, It->ColorZ, It->ColorW);
					NextPosition += (NextVelocity + Position) * DeltaTime;
					NextDiffuse += Diffuse * DeltaTime;
					memcpy(&It->PositionX, &NextPosition, sizeof(float) * 3);
					memcpy(&It->ColorX, &NextDiffuse, sizeof(float) * 4);
					It->Rotation += (It->Angular + RotationSpeed) * DeltaTime;
					It->Scale += ScaleSpeed * DeltaTime;

					if (L > 0)
					{
						NextVelocity -= (NextVelocity / Velocity) * DeltaTime;
						memcpy(&It->VelocityX, &NextVelocity, sizeof(float) * 3);
					}

					if (It->ColorW <= 0 || It->Scale <= 0)
					{
						Array->RemoveAt(It);
						It--;
						continue;
					}

					if (It->PositionX < MinX)
						MinX = It->PositionX;
					else if (It->PositionX > MaxX)
						MaxX = It->PositionX;

					if (It->PositionY < MinY)
						MinY = It->PositionY;
					else if (It->PositionY > MaxY)
						MaxY = It->PositionY;

					if (It->PositionZ < MinZ)
						MinZ = It->PositionZ;
					else if (It->PositionZ > MaxZ)
						MaxZ = It->PositionZ;
				}

				Base->Min = Compute::Vector3(MinX, MinY, MinZ);
				Base->Max = Compute::Vector3(MaxX, MaxY, MaxZ);
			}
			Component* EmitterAnimator::Copy(Entity* New) const
			{
				EmitterAnimator* Target = new EmitterAnimator(New);
				Target->Diffuse = Diffuse;
				Target->Position = Position;
				Target->Velocity = Velocity;
				Target->ScaleSpeed = ScaleSpeed;
				Target->RotationSpeed = RotationSpeed;
				Target->Spawner = Spawner;
				Target->Noise = Noise;
				Target->Simulate = Simulate;

				return Target;
			}
			Emitter* EmitterAnimator::GetEmitter() const
			{
				return Base;
			}

			FreeLook::FreeLook(Entity* Ref) : Component(Ref, ActorSet::Update), Activity(nullptr), Rotate(Graphics::KeyCode::CURSORRIGHT), Sensivity(0.005f)
			{
			}
			void FreeLook::Activate(Component* New)
			{
				Application* App = Application::Get();
				if (App != nullptr)
					Activity = App->Activity;

				SetActive(Activity != nullptr);
			}
			void FreeLook::Update(Core::Timer* Time)
			{
				if (!Activity)
					return;

				Compute::Vector2 Cursor = Activity->GetGlobalCursorPosition();
				if (!Activity->IsKeyDown(Rotate))
				{
					Position = Cursor;
					return;
				}

				if (!Activity->IsKeyDownHit(Rotate))
				{
					auto* Transform = Parent->GetTransform();
					Compute::Vector2 Next = (Cursor - Position) * Sensivity;
					Transform->Rotate(Compute::Vector3(Next.Y, Next.X));

					const Compute::Vector3& Rotation = Transform->GetRotation();
					Transform->SetRotation(Rotation.SetX(Compute::Mathf::Clamp(Rotation.X, -1.57079632679f, 1.57079632679f)));
				}
				else
					Position = Cursor;

				if ((int)Cursor.X != (int)Position.X || (int)Cursor.Y != (int)Position.Y)
					Activity->SetGlobalCursorPosition(Position);
			}
			Component* FreeLook::Copy(Entity* New) const
			{
				FreeLook* Target = new FreeLook(New);
				Target->Activity = Activity;
				Target->Position = Position;
				Target->Rotate = Rotate;
				Target->Sensivity = Sensivity;

				return Target;
			}
			Graphics::Activity* FreeLook::GetActivity() const
			{
				return Activity;
			}

			Fly::Fly(Entity* Ref) : Component(Ref, ActorSet::Update), Activity(nullptr)
			{
			}
			void Fly::Activate(Component* New)
			{
				Application* App = Application::Get();
				if (App != nullptr)
					Activity = App->Activity;

				SetActive(Activity != nullptr);
			}
			void Fly::Update(Core::Timer* Time)
			{
				if (!Activity)
					return;

				Compute::Vector3 Speed;
				if (Activity->IsKeyDown(Fast))
					Speed = Axis * SpeedUp * (float)Time->GetDeltaTime();
				else if (Activity->IsKeyDown(Slow))
					Speed = Axis * SpeedDown * (float)Time->GetDeltaTime();
				else
					Speed = Axis * SpeedNormal * (float)Time->GetDeltaTime();

				auto* Transform = Parent->GetTransform();
				bool ViewSpace = (Parent->GetComponent<Camera>() != nullptr);
				if (Activity->IsKeyDown(Forward))
					Transform->Move(Transform->Forward(ViewSpace) * Speed);

				if (Activity->IsKeyDown(Backward))
					Transform->Move(-Transform->Forward(ViewSpace) * Speed);

				if (Activity->IsKeyDown(Right))
					Transform->Move(Transform->Right(ViewSpace) * Speed);

				if (Activity->IsKeyDown(Left))
					Transform->Move(-Transform->Right(ViewSpace) * Speed);

				if (Activity->IsKeyDown(Up))
					Transform->Move(Transform->Up(ViewSpace) * Speed);

				if (Activity->IsKeyDown(Down))
					Transform->Move(-Transform->Up(ViewSpace) * Speed);
			}
			Component* Fly::Copy(Entity* New) const
			{
				Fly* Target = new Fly(New);
				Target->Activity = Activity;
				Target->Forward = Forward;
				Target->Backward = Backward;
				Target->Right = Right;
				Target->Left = Left;
				Target->Up = Up;
				Target->Down = Down;
				Target->Fast = Fast;
				Target->Slow = Slow;
				Target->Axis = Axis;
				Target->SpeedNormal = SpeedNormal;
				Target->SpeedUp = SpeedUp;
				Target->SpeedDown = SpeedDown;

				return Target;
			}
			Graphics::Activity* Fly::GetActivity() const
			{
				return Activity;
			}

			AudioSource::AudioSource(Entity* Ref) : Component(Ref, ActorSet::Synchronize)
			{
				Source = new Audio::AudioSource();
			}
			AudioSource::~AudioSource()
			{
				TH_RELEASE(Source);
			}
			void AudioSource::Deserialize(ContentManager* Content, Core::Schema* Node)
			{
				TH_ASSERT_V(Content != nullptr, "content manager should be set");
				TH_ASSERT_V(Node != nullptr, "schema should be set");

				std::string Path;
				if (Series::Unpack(Node->Find("audio-clip"), &Path))
					Source->SetClip(Content->Load<Audio::AudioClip>(Path));

				Series::Unpack(Node->Find("velocity"), &Sync.Velocity);
				Series::Unpack(Node->Find("direction"), &Sync.Direction);
				Series::Unpack(Node->Find("rolloff"), &Sync.Rolloff);
				Series::Unpack(Node->Find("cone-inner-angle"), &Sync.ConeInnerAngle);
				Series::Unpack(Node->Find("cone-outer-angle"), &Sync.ConeOuterAngle);
				Series::Unpack(Node->Find("cone-outer-gain"), &Sync.ConeOuterGain);
				Series::Unpack(Node->Find("distance"), &Sync.Distance);
				Series::Unpack(Node->Find("gain"), &Sync.Gain);
				Series::Unpack(Node->Find("pitch"), &Sync.Pitch);
				Series::Unpack(Node->Find("ref-distance"), &Sync.RefDistance);
				Series::Unpack(Node->Find("position"), &Sync.Position);
				Series::Unpack(Node->Find("relative"), &Sync.IsRelative);
				Series::Unpack(Node->Find("looped"), &Sync.IsLooped);
				Series::Unpack(Node->Find("distance"), &Sync.Distance);
				Series::Unpack(Node->Find("air-absorption"), &Sync.AirAbsorption);
				Series::Unpack(Node->Find("room-roll-off"), &Sync.RoomRollOff);

				std::vector<Core::Schema*> Effects = Node->FetchCollection("effects.effect");
				for (auto& Effect : Effects)
				{
					uint64_t Id;
					if (!Series::Unpack(Effect->Find("id"), &Id))
						continue;

					Audio::AudioEffect* Target = Core::Composer::Create<Audio::AudioEffect>(Id);
					if (!Target)
					{
						TH_WARN("[engine] audio effect with id %llu cannot be created", Id);
						continue;
					}

					Core::Schema* Meta = Effect->Find("metadata");
					if (!Meta)
						Meta = Effect->Set("metadata");

					Target->Deserialize(Meta);
					Source->AddEffect(Target);
				}

				bool Autoplay;
				if (Series::Unpack(Node->Find("autoplay"), &Autoplay) && Autoplay && Source->GetClip())
					Source->Play();

				ApplyPlayingPosition();
				Synchronize(nullptr);
			}
			void AudioSource::Serialize(ContentManager* Content, Core::Schema* Node)
			{
				TH_ASSERT_V(Content != nullptr, "content manager should be set");
				TH_ASSERT_V(Node != nullptr, "schema should be set");

				AssetCache* Asset = Content->Find<Audio::AudioClip>(Source->GetClip());
				if (Asset != nullptr)
					Series::Pack(Node->Set("audio-clip"), Asset->Path);

				Core::Schema* Effects = Node->Set("effects", Core::Var::Array());
				for (auto* Effect : *Source->GetEffects())
				{
					if (!Effect)
						continue;

					Core::Schema* Element = Effects->Set("effect");
					Series::Pack(Element->Set("id"), Effect->GetId());
					Effect->Serialize(Element->Set("metadata"));
				}

				Series::Pack(Node->Set("velocity"), Sync.Velocity);
				Series::Pack(Node->Set("direction"), Sync.Direction);
				Series::Pack(Node->Set("rolloff"), Sync.Rolloff);
				Series::Pack(Node->Set("cone-inner-angle"), Sync.ConeInnerAngle);
				Series::Pack(Node->Set("cone-outer-angle"), Sync.ConeOuterAngle);
				Series::Pack(Node->Set("cone-outer-gain"), Sync.ConeOuterGain);
				Series::Pack(Node->Set("distance"), Sync.Distance);
				Series::Pack(Node->Set("gain"), Sync.Gain);
				Series::Pack(Node->Set("pitch"), Sync.Pitch);
				Series::Pack(Node->Set("ref-distance"), Sync.RefDistance);
				Series::Pack(Node->Set("position"), Sync.Position);
				Series::Pack(Node->Set("relative"), Sync.IsRelative);
				Series::Pack(Node->Set("looped"), Sync.IsLooped);
				Series::Pack(Node->Set("distance"), Sync.Distance);
				Series::Pack(Node->Set("autoplay"), Source->IsPlaying());
				Series::Pack(Node->Set("air-absorption"), Sync.AirAbsorption);
				Series::Pack(Node->Set("room-roll-off"), Sync.RoomRollOff);
			}
			void AudioSource::Synchronize(Core::Timer* Time)
			{
				auto* Transform = Parent->GetTransform();
				if (Transform->IsDirty())
				{
					const Compute::Vector3& Position = Transform->GetPosition();
					Sync.Velocity = (Position - LastPosition) * Time->GetDeltaTime();
					LastPosition = Position;
				}
				else
					Sync.Velocity = 0.0f;

				if (Source->GetClip() != nullptr)
					Source->Synchronize(&Sync, Transform->GetPosition());
			}
			void AudioSource::ApplyPlayingPosition()
			{
				Audio::AudioContext::SetSourceData1F(Source->GetInstance(), Audio::SoundEx::Seconds_Offset, Sync.Position);
			}
			Component* AudioSource::Copy(Entity* New) const
			{
				AudioSource* Target = new AudioSource(New);
				Target->LastPosition = LastPosition;
				Target->Source->SetClip(Source->GetClip());
				Target->Sync = Sync;

				for (auto* Effect : *Source->GetEffects())
				{
					if (Effect != nullptr)
						Target->Source->AddEffect(Effect->Copy());
				}

				return Target;
			}
			Audio::AudioSource* AudioSource::GetSource() const
			{
				return Source;
			}
			Audio::AudioSync& AudioSource::GetSync()
			{
				return Sync;
			}

			AudioListener::AudioListener(Entity* Ref) : Component(Ref, ActorSet::Synchronize)
			{
			}
			AudioListener::~AudioListener()
			{
				Deactivate();
			}
			void AudioListener::Deserialize(ContentManager* Content, Core::Schema* Node)
			{
				TH_ASSERT_V(Node != nullptr, "schema should be set");
				Series::Unpack(Node->Find("gain"), &Gain);
			}
			void AudioListener::Serialize(ContentManager* Content, Core::Schema* Node)
			{
				TH_ASSERT_V(Node != nullptr, "schema should be set");
				Series::Pack(Node->Set("gain"), Gain);
			}
			void AudioListener::Synchronize(Core::Timer* Time)
			{
				auto* Transform = Parent->GetTransform();
				if (Transform->IsDirty())
				{
					const Compute::Vector3& Position = Transform->GetPosition();
					Compute::Vector3 Velocity = (Position - LastPosition) * Time->GetDeltaTime();
					Compute::Vector3 Rotation = Transform->GetRotation().dDirection();
					float LookAt[6] = { Rotation.X, Rotation.Y, Rotation.Z, 0.0f, 1.0f, 0.0f };
					LastPosition = Position;

					Audio::AudioContext::SetListenerData3F(Audio::SoundEx::Velocity, Velocity.X, Velocity.Y, Velocity.Z);
					Audio::AudioContext::SetListenerData3F(Audio::SoundEx::Position, -Position.X, -Position.Y, Position.Z);
					Audio::AudioContext::SetListenerDataVF(Audio::SoundEx::Orientation, LookAt);
				}

				Audio::AudioContext::SetListenerData1F(Audio::SoundEx::Gain, Gain);
			}
			void AudioListener::Deactivate()
			{
				float LookAt[6] = { 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f };
				Audio::AudioContext::SetListenerData3F(Audio::SoundEx::Velocity, 0.0f, 0.0f, 0.0f);
				Audio::AudioContext::SetListenerData3F(Audio::SoundEx::Position, 0.0f, 0.0f, 0.0f);
				Audio::AudioContext::SetListenerDataVF(Audio::SoundEx::Orientation, LookAt);
				Audio::AudioContext::SetListenerData1F(Audio::SoundEx::Gain, 0.0f);
			}
			Component* AudioListener::Copy(Entity* New) const
			{
				AudioListener* Target = new AudioListener(New);
				Target->LastPosition = LastPosition;
				Target->Gain = Gain;

				return Target;
			}

			PointLight::PointLight(Entity* Ref) : Component(Ref, ActorSet::Cullable)
			{
			}
			void PointLight::Deserialize(ContentManager* Content, Core::Schema* Node)
			{
				TH_ASSERT_V(Node != nullptr, "schema should be set");
				Series::Unpack(Node->Find("projection"), &Projection);
				Series::Unpack(Node->Find("view"), &View);
				Series::Unpack(Node->Find("size"), &Size);
				Series::Unpack(Node->Find("diffuse"), &Diffuse);
				Series::Unpack(Node->Find("emission"), &Emission);
				Series::Unpack(Node->Find("disperse"), &Disperse);
				Series::Unpack(Node->Find("shadow-softness"), &Shadow.Softness);
				Series::Unpack(Node->Find("shadow-distance"), &Shadow.Distance);
				Series::Unpack(Node->Find("shadow-bias"), &Shadow.Bias);
				Series::Unpack(Node->Find("shadow-iterations"), &Shadow.Iterations);
				Series::Unpack(Node->Find("shadow-enabled"), &Shadow.Enabled);
			}
			void PointLight::Serialize(ContentManager* Content, Core::Schema* Node)
			{
				TH_ASSERT_V(Node != nullptr, "schema should be set");
				Series::Pack(Node->Set("projection"), Projection);
				Series::Pack(Node->Set("view"), View);
				Series::Pack(Node->Set("size"), Size);
				Series::Pack(Node->Set("diffuse"), Diffuse);
				Series::Pack(Node->Set("emission"), Emission);
				Series::Pack(Node->Set("disperse"), Disperse);
				Series::Pack(Node->Set("shadow-softness"), Shadow.Softness);
				Series::Pack(Node->Set("shadow-distance"), Shadow.Distance);
				Series::Pack(Node->Set("shadow-bias"), Shadow.Bias);
				Series::Pack(Node->Set("shadow-iterations"), Shadow.Iterations);
				Series::Pack(Node->Set("shadow-enabled"), Shadow.Enabled);
			}
			void PointLight::Message(const std::string& Name, Core::VariantArgs& Args)
			{
				if (Name == "depth-flush")
					DepthMap = nullptr;
			}
			size_t PointLight::GetUnitBounds(Compute::Vector3& Min, Compute::Vector3& Max) const
			{
				Min = Size.Radius * -1.25f;
				Max = Size.Radius * 1.25f;
				return BOX_LIGHT;
			}
			float PointLight::GetVisibility(const Viewer& View, float Distance) const
			{
				return GetVisibilityRadius(Parent, View, Distance);
			}
			Component* PointLight::Copy(Entity* New) const
			{
				PointLight* Target = new PointLight(New);
				Target->Diffuse = Diffuse;
				Target->Emission = Emission;
				Target->Projection = Projection;
				Target->View = View;
				Target->Size = Size;
				memcpy(&Target->Shadow, &Shadow, sizeof(Shadow));

				return Target;
			}
			void PointLight::GenerateOrigin()
			{
				auto* Transform = Parent->GetTransform();
				if (Transform->IsDirty())
					View = Compute::Matrix4x4::CreateTranslation(Transform->GetPosition());
				Projection = Compute::Matrix4x4::CreatePerspective(90.0f, 1.0f, 0.1f, Shadow.Distance);
			}
			void PointLight::SetSize(const Attenuation& Value)
			{
				Size = Value;
				GetEntity()->GetTransform()->MakeDirty();
			}
			const Attenuation& PointLight::GetSize()
			{
				return Size;
			}

			SpotLight::SpotLight(Entity* Ref) : Component(Ref, ActorSet::Synchronize)
			{
			}
			void SpotLight::Deserialize(ContentManager* Content, Core::Schema* Node)
			{
				TH_ASSERT_V(Node != nullptr, "schema should be set");
				Series::Unpack(Node->Find("projection"), &Projection);
				Series::Unpack(Node->Find("view"), &View);
				Series::Unpack(Node->Find("size"), &Size);
				Series::Unpack(Node->Find("diffuse"), &Diffuse);
				Series::Unpack(Node->Find("emission"), &Emission);
				Series::Unpack(Node->Find("disperse"), &Disperse);
				Series::Unpack(Node->Find("cutoff"), &Cutoff);
				Series::Unpack(Node->Find("shadow-bias"), &Shadow.Bias);
				Series::Unpack(Node->Find("shadow-distance"), &Shadow.Distance);
				Series::Unpack(Node->Find("shadow-softness"), &Shadow.Softness);
				Series::Unpack(Node->Find("shadow-iterations"), &Shadow.Iterations);
				Series::Unpack(Node->Find("shadow-enabled"), &Shadow.Enabled);
			}
			void SpotLight::Serialize(ContentManager* Content, Core::Schema* Node)
			{
				TH_ASSERT_V(Node != nullptr, "schema should be set");
				Series::Pack(Node->Set("projection"), Projection);
				Series::Pack(Node->Set("view"), View);
				Series::Pack(Node->Set("size"), Size);
				Series::Pack(Node->Set("diffuse"), Diffuse);
				Series::Pack(Node->Set("emission"), Emission);
				Series::Pack(Node->Set("disperse"), Disperse);
				Series::Pack(Node->Set("cutoff"), Cutoff);
				Series::Pack(Node->Set("shadow-bias"), Shadow.Bias);
				Series::Pack(Node->Set("shadow-distance"), Shadow.Distance);
				Series::Pack(Node->Set("shadow-softness"), Shadow.Softness);
				Series::Pack(Node->Set("shadow-iterations"), Shadow.Iterations);
				Series::Pack(Node->Set("shadow-enabled"), Shadow.Enabled);
			}
			void SpotLight::Message(const std::string& Name, Core::VariantArgs& Args)
			{
				if (Name == "depth-flush")
					DepthMap = nullptr;
			}
			void SpotLight::Synchronize(Core::Timer* Time)
			{
				Cutoff = Compute::Mathf::Clamp(Cutoff, 0.0f, 180.0f);
			}
			size_t SpotLight::GetUnitBounds(Compute::Vector3& Min, Compute::Vector3& Max) const
			{
				Min = Size.Radius * -1.25f;
				Max = Size.Radius * 1.25f;
				return BOX_LIGHT;
			}
			float SpotLight::GetVisibility(const Viewer& View, float Distance) const
			{
				return GetVisibilityRadius(Parent, View, Distance);
			}
			Component* SpotLight::Copy(Entity* New) const
			{
				SpotLight* Target = new SpotLight(New);
				Target->Diffuse = Diffuse;
				Target->Projection = Projection;
				Target->View = View;
				Target->Size = Size;
				Target->Cutoff = Cutoff;
				Target->Emission = Emission;
				memcpy(&Target->Shadow, &Shadow, sizeof(Shadow));

				return Target;
			}
			void SpotLight::GenerateOrigin()
			{
				auto* Transform = Parent->GetTransform();
				if (Transform->IsDirty())
				{
					auto& Space = Transform->GetSpacing(Compute::Positioning::Global);
					View = Compute::Matrix4x4::CreateOrigin(Space.Position, Space.Rotation);
				}
				Projection = Compute::Matrix4x4::CreatePerspective(Cutoff, 1, 0.1f, Shadow.Distance);
			}
			void SpotLight::SetSize(const Attenuation& Value)
			{
				Size = Value;
				GetEntity()->GetTransform()->MakeDirty();
			}
			const Attenuation& SpotLight::GetSize()
			{
				return Size;
			}

			LineLight::LineLight(Entity* Ref) : Component(Ref, ActorSet::Cullable)
			{
			}
			void LineLight::Deserialize(ContentManager* Content, Core::Schema* Node)
			{
				TH_ASSERT_V(Node != nullptr, "schema should be set");
				Series::Unpack(Node->Find("diffuse"), &Diffuse);
				Series::Unpack(Node->Find("emission"), &Emission);

				for (uint32_t i = 0; i < 6; i++)
				{
					Series::Unpack(Node->Find("projection-" + std::to_string(i)), &Projection[i]);
					Series::Unpack(Node->Find("view-" + std::to_string(i)), &View[i]);
				}

				for (uint32_t i = 0; i < 6; i++)
					Series::Unpack(Node->Find("shadow-distance-" + std::to_string(i)), &Shadow.Distance[i]);

				Series::Unpack(Node->Find("shadow-cascades"), &Shadow.Cascades);
				Series::Unpack(Node->Find("shadow-far"), &Shadow.Far);
				Series::Unpack(Node->Find("shadow-near"), &Shadow.Near);
				Series::Unpack(Node->Find("shadow-bias"), &Shadow.Bias);
				Series::Unpack(Node->Find("shadow-softness"), &Shadow.Softness);
				Series::Unpack(Node->Find("shadow-iterations"), &Shadow.Iterations);
				Series::Unpack(Node->Find("shadow-enabled"), &Shadow.Enabled);
				Series::Unpack(Node->Find("rlh-emission"), &Sky.RlhEmission);
				Series::Unpack(Node->Find("mie-emission"), &Sky.MieEmission);
				Series::Unpack(Node->Find("rlh-height"), &Sky.RlhHeight);
				Series::Unpack(Node->Find("mie-height"), &Sky.MieEmission);
				Series::Unpack(Node->Find("mie-direction"), &Sky.MieDirection);
				Series::Unpack(Node->Find("inner-radius"), &Sky.InnerRadius);
				Series::Unpack(Node->Find("outer-radius"), &Sky.OuterRadius);
				Series::Unpack(Node->Find("sky-intensity"), &Sky.Intensity);
			}
			void LineLight::Serialize(ContentManager* Content, Core::Schema* Node)
			{
				TH_ASSERT_V(Node != nullptr, "schema should be set");
				Series::Pack(Node->Set("diffuse"), Diffuse);
				Series::Pack(Node->Set("emission"), Emission);

				for (uint32_t i = 0; i < 6; i++)
				{
					Series::Pack(Node->Set("projection-" + std::to_string(i)), Projection[i]);
					Series::Pack(Node->Set("view-" + std::to_string(i)), View[i]);
				}

				for (uint32_t i = 0; i < 6; i++)
					Series::Pack(Node->Set("shadow-distance-" + std::to_string(i)), Shadow.Distance[i]);

				Series::Pack(Node->Set("shadow-cascades"), Shadow.Cascades);
				Series::Pack(Node->Set("shadow-far"), Shadow.Far);
				Series::Pack(Node->Set("shadow-near"), Shadow.Near);
				Series::Pack(Node->Set("shadow-bias"), Shadow.Bias);
				Series::Pack(Node->Set("shadow-softness"), Shadow.Softness);
				Series::Pack(Node->Set("shadow-iterations"), Shadow.Iterations);
				Series::Pack(Node->Set("shadow-enabled"), Shadow.Enabled);
				Series::Pack(Node->Set("rlh-emission"), Sky.RlhEmission);
				Series::Pack(Node->Set("mie-emission"), Sky.MieEmission);
				Series::Pack(Node->Set("rlh-height"), Sky.RlhHeight);
				Series::Pack(Node->Set("mie-height"), Sky.MieEmission);
				Series::Pack(Node->Set("mie-direction"), Sky.MieDirection);
				Series::Pack(Node->Set("inner-radius"), Sky.InnerRadius);
				Series::Pack(Node->Set("outer-radius"), Sky.OuterRadius);
				Series::Pack(Node->Set("sky-intensity"), Sky.Intensity);
			}
			void LineLight::Message(const std::string& Name, Core::VariantArgs& Args)
			{
				if (Name == "depth-flush")
					DepthMap = nullptr;
			}
			Component* LineLight::Copy(Entity* New) const
			{
				LineLight* Target = new LineLight(New);
				Target->Diffuse = Diffuse;
				Target->Emission = Emission;
				memcpy(Target->Projection, Projection, sizeof(Compute::Matrix4x4) * 6);
				memcpy(Target->View, View, sizeof(Compute::Matrix4x4) * 6);
				memcpy(&Target->Shadow, &Shadow, sizeof(Shadow));
				memcpy(&Target->Sky, &Sky, sizeof(Sky));

				return Target;
			}
			void LineLight::GenerateOrigin()
			{
				auto* Viewer = (Components::Camera*)Parent->GetScene()->GetCamera();
				Compute::Vector3 Direction = Parent->GetTransform()->GetPosition().sNormalize();
				Compute::Vector3 Position = Viewer->GetEntity()->GetTransform()->GetPosition();
				Compute::Matrix4x4 LightView = Compute::Matrix4x4::CreateLookAt(Position, Position - Direction, Compute::Vector3::Up());
				Compute::Matrix4x4 ViewToLight = Viewer->GetView().Inv() * LightView;
				float FieldOfView = Compute::Mathf::Deg2Rad() * Viewer->FieldOfView;
				float Aspect = Viewer->GetAspect();

				if (Shadow.Cascades > 6)
					return;

				for (uint32_t i = 0; i < Shadow.Cascades; i++)
				{
					float Near = (i < 1 ? 0.1f : Shadow.Distance[i - 1]), Far = Shadow.Distance[i];
					Compute::Frustum Frustum(FieldOfView, Aspect, Near, Far);
					Frustum.Transform(ViewToLight);

					Compute::Vector2 X, Y, Z;
					Frustum.GetBoundingBox(&X, &Y, &Z);

					if (Z.X > -1.0f)
						Z.X = -1.0f;
					if (Z.Y < 1.0f)
						Z.Y = 1.0f;

					Projection[i] = Compute::Matrix4x4::CreateOrthographicOffCenter(X.X, X.Y, Y.X, Y.Y, Z.X * Shadow.Near, Z.Y * Shadow.Far);
					View[i] = LightView;
				}
			}

			SurfaceLight::SurfaceLight(Entity* Ref) : Component(Ref, ActorSet::Cullable), Projection(Compute::Matrix4x4::CreatePerspective(90.0f, 1, 0.01f, 100.0f))
			{
			}
			SurfaceLight::~SurfaceLight()
			{
				TH_RELEASE(DiffuseMapX[0]);
				TH_RELEASE(DiffuseMapX[1]);
				TH_RELEASE(DiffuseMapY[0]);
				TH_RELEASE(DiffuseMapY[1]);
				TH_RELEASE(DiffuseMapZ[0]);
				TH_RELEASE(DiffuseMapZ[1]);
				TH_RELEASE(DiffuseMap);
				TH_RELEASE(Probe);
			}
			void SurfaceLight::Deserialize(ContentManager* Content, Core::Schema* Node)
			{
				TH_ASSERT_V(Content != nullptr, "content manager should be set");
				TH_ASSERT_V(Node != nullptr, "schema should be set");

				std::string Path;
				if (!Series::Unpack(Node->Find("diffuse-map"), &Path))
				{
					if (Series::Unpack(Node->Find("diffuse-map-px"), &Path))
					{
						TH_RELEASE(DiffuseMapX[0]);
						DiffuseMapX[0] = Content->Load<Graphics::Texture2D>(Path);
						DiffuseMapX[0]->AddRef();
					}

					if (Series::Unpack(Node->Find("diffuse-map-nx"), &Path))
					{
						TH_RELEASE(DiffuseMapX[1]);
						DiffuseMapX[1] = Content->Load<Graphics::Texture2D>(Path);
						DiffuseMapX[1]->AddRef();
					}

					if (Series::Unpack(Node->Find("diffuse-map-py"), &Path))
					{
						TH_RELEASE(DiffuseMapY[0]);
						DiffuseMapY[0] = Content->Load<Graphics::Texture2D>(Path);
						DiffuseMapY[0]->AddRef();
					}

					if (Series::Unpack(Node->Find("diffuse-map-ny"), &Path))
					{
						TH_RELEASE(DiffuseMapY[1]);
						DiffuseMapY[1] = Content->Load<Graphics::Texture2D>(Path);
						DiffuseMapY[1]->AddRef();
					}

					if (Series::Unpack(Node->Find("diffuse-map-pz"), &Path))
					{
						TH_RELEASE(DiffuseMapZ[0]);
						DiffuseMapZ[0] = Content->Load<Graphics::Texture2D>(Path);
						DiffuseMapZ[0]->AddRef();
					}

					if (Series::Unpack(Node->Find("diffuse-map-nz"), &Path))
					{
						TH_RELEASE(DiffuseMapZ[1]);
						DiffuseMapZ[1] = Content->Load<Graphics::Texture2D>(Path);
						DiffuseMapZ[1]->AddRef();
					}
				}
				else
				{
					TH_RELEASE(DiffuseMap);
					DiffuseMap = Content->Load<Graphics::Texture2D>(Path);
					DiffuseMap->AddRef();
				}

				std::vector<Compute::Matrix4x4> Views;
				Series::Unpack(Node->Find("projection"), &Projection);
				Series::Unpack(Node->Find("view"), &Views);
				Series::Unpack(Node->Find("tick"), &Tick);
				Series::Unpack(Node->Find("diffuse"), &Diffuse);
				Series::Unpack(Node->Find("size"), &Size);
				Series::Unpack(Node->Find("emission"), &Emission);
				Series::Unpack(Node->Find("infinity"), &Infinity);
				Series::Unpack(Node->Find("parallax"), &Parallax);
				Series::Unpack(Node->Find("static-mask"), &StaticMask);

				int64_t Count = Compute::Math<int64_t>::Min((int64_t)Views.size(), 6);
				for (int64_t i = 0; i < Count; i++)
					View[i] = Views[i];

				if (!DiffuseMap)
					SetDiffuseMap(DiffuseMapX, DiffuseMapY, DiffuseMapZ);
				else
					SetDiffuseMap(DiffuseMap);
			}
			void SurfaceLight::Serialize(ContentManager* Content, Core::Schema* Node)
			{
				TH_ASSERT_V(Content != nullptr, "content manager should be set");
				TH_ASSERT_V(Node != nullptr, "schema should be set");

				AssetCache* Asset = nullptr;
				if (!DiffuseMap)
				{
					Asset = Content->Find<Graphics::Texture2D>(DiffuseMapX[0]);
					if (Asset != nullptr)
						Series::Pack(Node->Set("diffuse-map-px"), Asset->Path);

					Asset = Content->Find<Graphics::Texture2D>(DiffuseMapX[1]);
					if (Asset != nullptr)
						Series::Pack(Node->Set("diffuse-map-nx"), Asset->Path);

					Asset = Content->Find<Graphics::Texture2D>(DiffuseMapY[0]);
					if (Asset != nullptr)
						Series::Pack(Node->Set("diffuse-map-py"), Asset->Path);

					Asset = Content->Find<Graphics::Texture2D>(DiffuseMapY[1]);
					if (Asset != nullptr)
						Series::Pack(Node->Set("diffuse-map-ny"), Asset->Path);

					Asset = Content->Find<Graphics::Texture2D>(DiffuseMapZ[0]);
					if (Asset != nullptr)
						Series::Pack(Node->Set("diffuse-map-pz"), Asset->Path);

					Asset = Content->Find<Graphics::Texture2D>(DiffuseMapZ[1]);
					if (Asset != nullptr)
						Series::Pack(Node->Set("diffuse-map-nz"), Asset->Path);
				}
				else
				{
					Asset = Content->Find<Graphics::Texture2D>(DiffuseMap);
					if (Asset != nullptr)
						Series::Pack(Node->Set("diffuse-map"), Asset->Path);
				}

				std::vector<Compute::Matrix4x4> Views;
				for (int64_t i = 0; i < 6; i++)
					Views.push_back(View[i]);

				Series::Pack(Node->Set("projection"), Projection);
				Series::Pack(Node->Set("view"), Views);
				Series::Pack(Node->Set("tick"), Tick);
				Series::Pack(Node->Set("size"), Size);
				Series::Pack(Node->Set("diffuse"), Diffuse);
				Series::Pack(Node->Set("emission"), Emission);
				Series::Pack(Node->Set("infinity"), Infinity);
				Series::Pack(Node->Set("parallax"), Parallax);
				Series::Pack(Node->Set("static-mask"), StaticMask);
			}
			size_t SurfaceLight::GetUnitBounds(Compute::Vector3& Min, Compute::Vector3& Max) const
			{
				Min = Size.Radius * -1.25f;
				Max = Size.Radius * 1.25f;
				return BOX_LIGHT;
			}
			float SurfaceLight::GetVisibility(const Viewer& View, float Distance) const
			{
				if (Infinity > 0.0f)
					return 1.0f;

				return GetVisibilityRadius(Parent, View, Distance);
			}
			Component* SurfaceLight::Copy(Entity* New) const
			{
				SurfaceLight* Target = new SurfaceLight(New);
				Target->Projection = Projection;
				Target->Diffuse = Diffuse;
				Target->Emission = Emission;
				Target->Size = Size;
				Target->Tick = Tick;
				memcpy(Target->View, View, 6 * sizeof(Compute::Matrix4x4));

				if (!DiffuseMap)
					Target->SetDiffuseMap(DiffuseMapX, DiffuseMapY, DiffuseMapZ);
				else
					Target->SetDiffuseMap(DiffuseMap);

				return Target;
			}
			void SurfaceLight::SetProbeCache(Graphics::TextureCube* NewCache)
			{
				Probe = NewCache;
			}
			void SurfaceLight::SetSize(const Attenuation& Value)
			{
				Size = Value;
				GetEntity()->GetTransform()->MakeDirty();
			}
			bool SurfaceLight::SetDiffuseMap(Graphics::Texture2D* Map)
			{
				TH_ASSERT(Parent->GetScene()->GetDevice() != nullptr, false, "graphics device should be set");
				if (!Map)
				{
					TH_CLEAR(DiffuseMapX[0]);
					TH_CLEAR(DiffuseMapX[1]);
					TH_CLEAR(DiffuseMapY[0]);
					TH_CLEAR(DiffuseMapY[1]);
					TH_CLEAR(DiffuseMapZ[0]);
					TH_CLEAR(DiffuseMapZ[1]);
					TH_CLEAR(DiffuseMap);
					return false;
				}

				TH_CLEAR(DiffuseMapX[0]);
				TH_CLEAR(DiffuseMapX[1]);
				TH_CLEAR(DiffuseMapY[0]);
				TH_CLEAR(DiffuseMapY[1]);
				TH_CLEAR(DiffuseMapZ[0]);
				TH_CLEAR(DiffuseMapZ[1]);
				TH_RELEASE(DiffuseMap);
				DiffuseMap = Map;
				Map->AddRef();

				TH_RELEASE(Probe);
				Probe = Parent->GetScene()->GetDevice()->CreateTextureCube(DiffuseMap);
				return Probe != nullptr;
			}
			bool SurfaceLight::SetDiffuseMap(Graphics::Texture2D* const MapX[2], Graphics::Texture2D* const MapY[2], Graphics::Texture2D* const MapZ[2])
			{
				TH_ASSERT(Parent->GetScene()->GetDevice() != nullptr, false, "graphics device should be set");
				if (!MapX[0] || !MapX[1] || !MapY[0] || !MapY[1] || !MapZ[0] || !MapZ[1])
				{
					TH_CLEAR(DiffuseMapX[0]);
					TH_CLEAR(DiffuseMapX[1]);
					TH_CLEAR(DiffuseMapY[0]);
					TH_CLEAR(DiffuseMapY[1]);
					TH_CLEAR(DiffuseMapZ[0]);
					TH_CLEAR(DiffuseMapZ[1]);
					TH_CLEAR(DiffuseMap);
					return false;
				}

				TH_RELEASE(DiffuseMapX[0]);
				TH_RELEASE(DiffuseMapX[1]);
				TH_RELEASE(DiffuseMapY[0]);
				TH_RELEASE(DiffuseMapY[1]);
				TH_RELEASE(DiffuseMapZ[0]);
				TH_RELEASE(DiffuseMapZ[1]);
				TH_CLEAR(DiffuseMap);

				Graphics::Texture2D* Resources[6];
				Resources[0] = DiffuseMapX[0] = MapX[0]; MapX[0]->AddRef();
				Resources[1] = DiffuseMapX[1] = MapX[1]; MapX[1]->AddRef();
				Resources[2] = DiffuseMapY[0] = MapY[0]; MapY[0]->AddRef();
				Resources[3] = DiffuseMapY[1] = MapY[1]; MapY[1]->AddRef();
				Resources[4] = DiffuseMapZ[0] = MapZ[0]; MapZ[0]->AddRef();
				Resources[5] = DiffuseMapZ[1] = MapZ[1]; MapZ[1]->AddRef();

				TH_RELEASE(Probe);
				Probe = Parent->GetScene()->GetDevice()->CreateTextureCube(Resources);
				return Probe != nullptr;
			}
			bool SurfaceLight::IsImageBased() const
			{
				return DiffuseMapX[0] != nullptr || DiffuseMap != nullptr;
			}
			const Attenuation& SurfaceLight::GetSize()
			{
				return Size;
			}
			Graphics::TextureCube* SurfaceLight::GetProbeCache() const
			{
				return Probe;
			}
			Graphics::Texture2D* SurfaceLight::GetDiffuseMapXP()
			{
				return DiffuseMapX[0];
			}
			Graphics::Texture2D* SurfaceLight::GetDiffuseMapXN()
			{
				return DiffuseMapX[1];
			}
			Graphics::Texture2D* SurfaceLight::GetDiffuseMapYP()
			{
				return DiffuseMapY[0];
			}
			Graphics::Texture2D* SurfaceLight::GetDiffuseMapYN()
			{
				return DiffuseMapY[1];
			}
			Graphics::Texture2D* SurfaceLight::GetDiffuseMapZP()
			{
				return DiffuseMapZ[0];
			}
			Graphics::Texture2D* SurfaceLight::GetDiffuseMapZN()
			{
				return DiffuseMapZ[1];
			}
			Graphics::Texture2D* SurfaceLight::GetDiffuseMap()
			{
				return DiffuseMap;
			}

			Illuminator::Illuminator(Entity* Ref) : Component(Ref, ActorSet::Cullable), VoxelMap(nullptr), Regenerate(true)
			{
				Inside.Delay = 30.0;
				Outside.Delay = 10000.0;
				RayStep = 0.5f;
				MaxSteps = 256.0f;
				Distance = 12.0f;
				Radiance = 1.0f;
				Occlusion = 0.33f;
				Specular = 2.0f;
				Length = 1.0f;
				Margin = 3.828424;
				Offset = -0.01;
				Angle = 0.5;
				Bleeding = 0.33f;
			}
			void Illuminator::Deserialize(ContentManager* Content, Core::Schema* Node)
			{
				TH_ASSERT_V(Content != nullptr, "content manager should be set");
				TH_ASSERT_V(Node != nullptr, "schema should be set");

				Series::Unpack(Node->Find("inside-delay"), &Inside.Delay);
				Series::Unpack(Node->Find("outside-delay"), &Outside.Delay);
				Series::Unpack(Node->Find("ray-step"), &RayStep);
				Series::Unpack(Node->Find("max-steps"), &MaxSteps);
				Series::Unpack(Node->Find("distance"), &Distance);
				Series::Unpack(Node->Find("radiance"), &Radiance);
				Series::Unpack(Node->Find("length"), &Length);
				Series::Unpack(Node->Find("margin"), &Margin);
				Series::Unpack(Node->Find("offset"), &Offset);
				Series::Unpack(Node->Find("angle"), &Angle);
				Series::Unpack(Node->Find("occlusion"), &Occlusion);
				Series::Unpack(Node->Find("specular"), &Specular);
				Series::Unpack(Node->Find("bleeding"), &Bleeding);
			}
			void Illuminator::Serialize(ContentManager* Content, Core::Schema* Node)
			{
				TH_ASSERT_V(Content != nullptr, "content manager should be set");
				TH_ASSERT_V(Node != nullptr, "schema should be set");

				Series::Pack(Node->Set("inside-delay"), Inside.Delay);
				Series::Pack(Node->Set("outside-delay"), Outside.Delay);
				Series::Pack(Node->Set("ray-step"), RayStep);
				Series::Pack(Node->Set("max-steps"), MaxSteps);
				Series::Pack(Node->Set("distance"), Distance);
				Series::Pack(Node->Set("radiance"), Radiance);
				Series::Pack(Node->Set("length"), Length);
				Series::Pack(Node->Set("margin"), Margin);
				Series::Pack(Node->Set("offset"), Offset);
				Series::Pack(Node->Set("angle"), Angle);
				Series::Pack(Node->Set("occlusion"), Occlusion);
				Series::Pack(Node->Set("specular"), Specular);
				Series::Pack(Node->Set("bleeding"), Bleeding);
			}
			void Illuminator::Message(const std::string& Name, Core::VariantArgs& Args)
			{
				if (Name == "depth-flush")
				{
					VoxelMap = nullptr;
					Regenerate = true;
				}
			}
			Component* Illuminator::Copy(Entity* New) const
			{
				Illuminator* Target = new Illuminator(New);
				Target->Inside = Inside;
				Target->Outside = Outside;
				Target->RayStep = RayStep;
				Target->MaxSteps = MaxSteps;
				Target->Radiance = Radiance;
				Target->Length = Length;
				Target->Occlusion = Occlusion;
				Target->Specular = Specular;

				return Target;
			}

			Camera::Camera(Entity* Ref) : Component(Ref, ActorSet::Synchronize), Mode(ProjectionMode_Perspective), Renderer(new RenderSystem(Ref->GetScene())), Viewport({ 0, 0, 512, 512, 0, 1 })
			{
			}
			Camera::~Camera()
			{
				TH_RELEASE(Renderer);
			}
			void Camera::Activate(Component* New)
			{
				TH_ASSERT_V(Parent->GetScene()->GetDevice() != nullptr, "graphics device should be set");
				TH_ASSERT_V(Parent->GetScene()->GetDevice()->GetRenderTarget() != nullptr, "render target should be set");

				SceneGraph* Scene = Parent->GetScene();
				Viewport = Scene->GetDevice()->GetRenderTarget()->GetViewport();
				if (New == this)
					Renderer->Remount();
			}
			void Camera::Deactivate()
			{
				SceneGraph* Scene = Parent->GetScene();
				if (Scene->GetCamera() == this)
					Scene->SetCamera(nullptr);
			}
			void Camera::Deserialize(ContentManager* Content, Core::Schema* Node)
			{
				TH_ASSERT_V(Content != nullptr, "content manager should be set");
				TH_ASSERT_V(Node != nullptr, "schema should be set");
				TH_ASSERT_V(Parent->GetScene()->GetDevice() != nullptr, "graphics device should be set");

				int _Mode = 0;
				if (Series::Unpack(Node->Find("mode"), &_Mode))
					Mode = (ProjectionMode)_Mode;

				Series::Unpack(Node->Find("projection"), &Projection);
				Series::Unpack(Node->Find("field-of-view"), &FieldOfView);
				Series::Unpack(Node->Find("far-plane"), &FarPlane);
				Series::Unpack(Node->Find("near-plane"), &NearPlane);
				Series::Unpack(Node->Find("width"), &Width);
				Series::Unpack(Node->Find("height"), &Height);

				SceneGraph* Scene = Parent->GetScene();
				Series::Unpack(Node->Find("occluder-skips"), &Renderer->OccluderSkips);
				Series::Unpack(Node->Find("occludee-skips"), &Renderer->OccludeeSkips);
				Series::Unpack(Node->Find("occlusion-skips"), &Renderer->OcclusionSkips);
				Series::Unpack(Node->Find("frustum-cull"), &Renderer->FrustumCulling);
				Series::Unpack(Node->Find("occlusion-cull"), &Renderer->OcclusionCulling);
				Series::Unpack(Node->Find("max-queries"), &Renderer->MaxQueries);

				std::vector<Core::Schema*> Renderers = Node->FetchCollection("renderers.renderer");
				for (auto& Render : Renderers)
				{
					uint64_t Id;
					if (!Series::Unpack(Render->Find("id"), &Id))
						continue;

					Engine::Renderer* Target = Core::Composer::Create<Engine::Renderer>(Id, Renderer);
					if (!Renderer->AddRenderer(Target))
					{
						TH_WARN("[engine] cannot create renderer with id %llu", Id);
						continue;
					}

					Core::Schema* Meta = Render->Find("metadata");
					if (!Meta)
						Meta = Render->Set("metadata");

					Target->Deactivate();
					Target->Deserialize(Content, Meta);
					Target->Activate();

					Series::Unpack(Render->Find("active"), &Target->Active);
				}
			}
			void Camera::Serialize(ContentManager* Content, Core::Schema* Node)
			{
				TH_ASSERT_V(Content != nullptr, "content manager should be set");
				TH_ASSERT_V(Node != nullptr, "schema should be set");

				Series::Pack(Node->Set("mode"), (int)Mode);
				Series::Pack(Node->Set("projection"), Projection);
				Series::Pack(Node->Set("field-of-view"), FieldOfView);
				Series::Pack(Node->Set("far-plane"), FarPlane);
				Series::Pack(Node->Set("near-plane"), NearPlane);
				Series::Pack(Node->Set("width"), Width);
				Series::Pack(Node->Set("height"), Height);
				Series::Pack(Node->Set("occluder-skips"), Renderer->OccluderSkips);
				Series::Pack(Node->Set("occludee-skips"), Renderer->OccludeeSkips);
				Series::Pack(Node->Set("occlusion-skips"), Renderer->OcclusionSkips);
				Series::Pack(Node->Set("frustum-cull"), Renderer->FrustumCulling);
				Series::Pack(Node->Set("occlusion-cull"), Renderer->OcclusionCulling);
				Series::Pack(Node->Set("max-queries"), Renderer->MaxQueries);

				Core::Schema* Renderers = Node->Set("renderers", Core::Var::Array());
				for (auto* Next : Renderer->GetRenderers())
				{
					Core::Schema* Render = Renderers->Set("renderer");
					Series::Pack(Render->Set("id"), Next->GetId());
					Series::Pack(Render->Set("active"), Next->Active);
					Next->Serialize(Content, Render->Set("metadata"));
				}
			}
			void Camera::Synchronize(Core::Timer* Time)
			{
				float W = Width, H = Height;
				if (W <= 0 || H <= 0)
				{
					W = Viewport.Width;
					H = Viewport.Height;
				}

				if (Mode == ProjectionMode_Perspective)
					Projection = Compute::Matrix4x4::CreatePerspective(FieldOfView, W / H, NearPlane, FarPlane);
				else if (Mode == ProjectionMode_Orthographic)
					Projection = Compute::Matrix4x4::CreateOrthographic(W, H, NearPlane, FarPlane);
			}
			void Camera::GetViewer(Viewer* Output)
			{
				TH_ASSERT_V(Output != nullptr, "viewer should be set");

				auto& Space = Parent->GetTransform()->GetSpacing(Compute::Positioning::Global);
				RenderCulling Culling = (Mode == ProjectionMode_Perspective ? RenderCulling::Spot : RenderCulling::Line);

				Output->Set(GetView(), Projection, Space.Position, Space.Rotation, FieldOfView, GetAspect(), NearPlane, FarPlane, Culling);
				Output->Renderer = Renderer;
				View = *Output;
			}
			void Camera::ResizeBuffers()
			{
				for (auto* Next : Renderer->GetRenderers())
					Next->ResizeBuffers();
			}
			Viewer& Camera::GetViewer()
			{
				return View;
			}
			RenderSystem* Camera::GetRenderer()
			{
				return Renderer;
			}
			Compute::Matrix4x4 Camera::GetProjection()
			{
				return Projection;
			}
			Compute::Vector3 Camera::GetViewPosition()
			{
				auto* Transform = Parent->GetTransform();
				return Transform->GetPosition().InvX().InvY();
			}
			Compute::Matrix4x4 Camera::GetViewProjection()
			{
				return GetView() * Projection;
			}
			Compute::Matrix4x4 Camera::GetView()
			{
				auto& Space = Parent->GetTransform()->GetSpacing(Compute::Positioning::Global);
				return Compute::Matrix4x4::CreateOrigin(Space.Position, Space.Rotation);
			}
			Compute::Frustum Camera::GetFrustum()
			{
				Compute::Frustum Result(Compute::Mathf::Deg2Rad() * FieldOfView, GetAspect(), NearPlane, FarPlane * 0.25f);
				Result.Transform(GetView().Inv());
				return Result;
			}
			Compute::Ray Camera::GetScreenRay(const Compute::Vector2& Position)
			{
				float W = Width, H = Height;
				if (W <= 0 || H <= 0)
				{
					Graphics::Viewport V = Renderer->GetDevice()->GetRenderTarget()->GetViewport();
					W = V.Width; H = V.Height;
				}

				return Compute::Geometric::CreateCursorRay(Parent->GetTransform()->GetPosition(), Position, Compute::Vector2(W, H), Projection.Inv(), GetView().Inv());
			}
			float Camera::GetDistance(Entity* Other)
			{
				TH_ASSERT(Other != nullptr, -1.0f, "other should be set");
				return Other->GetTransform()->GetPosition().Distance(View.Position);
			}
			float Camera::GetWidth()
			{
				float W = Width;
				if (W <= 0)
					W = Viewport.Width;

				return W;
			}
			float Camera::GetHeight()
			{
				float H = Height;
				if (H <= 0)
					H = Viewport.Height;

				return H;
			}
			float Camera::GetAspect()
			{
				float W = Width, H = Height;
				if (W <= 0 || H <= 0)
				{
					W = Viewport.Width;
					H = Viewport.Height;
				}

				return W / H;
			}
			bool Camera::RayTest(const Compute::Ray& Ray, Entity* Other)
			{
				TH_ASSERT(Other != nullptr, false, "other should be set");
				return Compute::Geometric::CursorRayTest(Ray, Other->GetTransform()->GetBias());
			}
			bool Camera::RayTest(const Compute::Ray& Ray, const Compute::Matrix4x4& World)
			{
				return Compute::Geometric::CursorRayTest(Ray, World);
			}
			Component* Camera::Copy(Entity* New) const
			{
				Camera* Target = new Camera(New);
				Target->FarPlane = FarPlane;
				Target->NearPlane = NearPlane;
				Target->Width = Width;
				Target->Height = Height;
				Target->Mode = Mode;
				Target->FieldOfView = FieldOfView;
				Target->Projection = Projection;

				return Target;
			}

			Scriptable::Scriptable(Entity* Ref) : Component(Ref, ActorSet::Synchronize | ActorSet::Update | ActorSet::Message), Compiler(nullptr), Source(SourceType_Resource), Invoke(InvokeType_Typeless)
			{
			}
			Scriptable::~Scriptable()
			{
				TH_RELEASE(Compiler);
			}
			void Scriptable::Deserialize(ContentManager* Content, Core::Schema* Node)
			{
				TH_ASSERT_V(Content != nullptr, "content manager should be set");
				TH_ASSERT_V(Node != nullptr, "schema should be set");

				std::string Type;
				if (Series::Unpack(Node->Find("source"), &Type))
				{
					if (Type == "memory")
						Source = SourceType_Memory;
					else if (Type == "resource")
						Source = SourceType_Resource;
				}

				if (Series::Unpack(Node->Find("invoke"), &Type))
				{
					if (Type == "typeless")
						Invoke = InvokeType_Typeless;
					else if (Type == "normal")
						Invoke = InvokeType_Normal;
				}

				if (!Series::Unpack(Node->Find("resource"), &Type))
					return;

				Resource = Core::OS::Path::Resolve(Type.c_str(), Content->GetEnvironment());
				if (Resource.empty())
					Resource = std::move(Type);

				if (SetSource() < 0)
					return;

				Core::Schema* Cache = Node->Find("cache");
				if (Cache != nullptr)
				{
					for (auto& Var : Cache->GetChilds())
					{
						int TypeId = -1;
						if (!Series::Unpack(Var->Find("type"), &TypeId))
							continue;

						switch ((Script::VMTypeId)TypeId)
						{
							case Script::VMTypeId::BOOL:
							{
								bool Result = false;
								if (Series::Unpack(Var->Find("data"), &Result))
									SetTypePropertyByName(Var->Key.c_str(), Result);
								break;
							}
							case Script::VMTypeId::INT8:
							{
								int64_t Result = 0;
								if (Series::Unpack(Var->Find("data"), &Result))
									SetTypePropertyByName(Var->Key.c_str(), (char)Result);
								break;
							}
							case Script::VMTypeId::INT16:
							{
								int64_t Result = 0;
								if (Series::Unpack(Var->Find("data"), &Result))
									SetTypePropertyByName(Var->Key.c_str(), (short)Result);
								break;
							}
							case Script::VMTypeId::INT32:
							{
								int64_t Result = 0;
								if (Series::Unpack(Var->Find("data"), &Result))
									SetTypePropertyByName(Var->Key.c_str(), (int)Result);
								break;
							}
							case Script::VMTypeId::INT64:
							{
								int64_t Result = 0;
								if (Series::Unpack(Var->Find("data"), &Result))
									SetTypePropertyByName(Var->Key.c_str(), Result);
								break;
							}
							case Script::VMTypeId::UINT8:
							{
								int64_t Result = 0;
								if (Series::Unpack(Var->Find("data"), &Result))
									SetTypePropertyByName(Var->Key.c_str(), (unsigned char)Result);
								break;
							}
							case Script::VMTypeId::UINT16:
							{
								int64_t Result = 0;
								if (Series::Unpack(Var->Find("data"), &Result))
									SetTypePropertyByName(Var->Key.c_str(), (unsigned short)Result);
								break;
							}
							case Script::VMTypeId::UINT32:
							{
								int64_t Result = 0;
								if (Series::Unpack(Var->Find("data"), &Result))
									SetTypePropertyByName(Var->Key.c_str(), (unsigned int)Result);
								break;
							}
							case Script::VMTypeId::UINT64:
							{
								int64_t Result = 0;
								if (Series::Unpack(Var->Find("data"), &Result))
									SetTypePropertyByName(Var->Key.c_str(), (uint64_t)Result);
								break;
							}
							case Script::VMTypeId::FLOAT:
							{
								float Result = 0.0f;
								if (Series::Unpack(Var->Find("data"), &Result))
									SetTypePropertyByName(Var->Key.c_str(), Result);
								break;
							}
							case Script::VMTypeId::DOUBLE:
							{
								double Result = 0.0;
								if (Series::Unpack(Var->Find("data"), &Result))
									SetTypePropertyByName(Var->Key.c_str(), Result);
								break;
							}
							default:
							{
								std::string Result;
								if (Series::Unpack(Var->Find("data"), &Result))
									SetTypePropertyByName(Var->Key.c_str(), Result);
								break;
							}
						}
					}
				}

				Call(Entry.Deserialize, [this, &Content, &Node](Script::VMContext* Context)
				{
					if (Invoke == InvokeType_Typeless)
						return;

					Component* Current = this;
					Context->SetArgObject(0, Current);
					Context->SetArgObject(1, Content);
					Context->SetArgObject(2, Node);
				});
			}
			void Scriptable::Serialize(ContentManager* Content, Core::Schema* Node)
			{
				TH_ASSERT_V(Content != nullptr, "content manager should be set");
				TH_ASSERT_V(Node != nullptr, "schema should be set");

				if (Source == SourceType_Memory)
					Series::Pack(Node->Set("source"), "memory");
				else if (Source == SourceType_Resource)
					Series::Pack(Node->Set("source"), "resource");

				if (Invoke == InvokeType_Typeless)
					Series::Pack(Node->Set("invoke"), "typeless");
				else if (Invoke == InvokeType_Normal)
					Series::Pack(Node->Set("invoke"), "normal");

				int Count = GetPropertiesCount();
				Series::Pack(Node->Set("resource"), Core::Parser(Resource).Replace(Content->GetEnvironment(), "./").Replace('\\', '/').R());

				Core::Schema* Cache = Node->Set("cache");
				for (int i = 0; i < Count; i++)
				{
					Script::VMProperty Result;
					if (!GetPropertyByIndex(i, &Result) || !Result.Name || !Result.Pointer)
						continue;

					Core::Schema* Var = Core::Var::Set::Object();
					Series::Pack(Var->Set("type"), Result.TypeId);

					switch ((Script::VMTypeId)Result.TypeId)
					{
						case Script::VMTypeId::BOOL:
							Series::Pack(Var->Set("data"), *(bool*)Result.Pointer);
							break;
						case Script::VMTypeId::INT8:
							Series::Pack(Var->Set("data"), (int64_t) * (char*)Result.Pointer);
							break;
						case Script::VMTypeId::INT16:
							Series::Pack(Var->Set("data"), (int64_t) * (short*)Result.Pointer);
							break;
						case Script::VMTypeId::INT32:
							Series::Pack(Var->Set("data"), (int64_t) * (int*)Result.Pointer);
							break;
						case Script::VMTypeId::INT64:
							Series::Pack(Var->Set("data"), *(int64_t*)Result.Pointer);
							break;
						case Script::VMTypeId::UINT8:
							Series::Pack(Var->Set("data"), (int64_t) * (unsigned char*)Result.Pointer);
							break;
						case Script::VMTypeId::UINT16:
							Series::Pack(Var->Set("data"), (int64_t) * (unsigned short*)Result.Pointer);
							break;
						case Script::VMTypeId::UINT32:
							Series::Pack(Var->Set("data"), (int64_t) * (unsigned int*)Result.Pointer);
							break;
						case Script::VMTypeId::UINT64:
							Series::Pack(Var->Set("data"), (int64_t) * (uint64_t*)Result.Pointer);
							break;
						case Script::VMTypeId::FLOAT:
							Series::Pack(Var->Set("data"), (double)*(float*)Result.Pointer);
							break;
						case Script::VMTypeId::DOUBLE:
							Series::Pack(Var->Set("data"), *(double*)Result.Pointer);
							break;
						default:
						{
							Script::VMTypeInfo Type = GetCompiler()->GetManager()->Global().GetTypeInfoById(Result.TypeId);
							if (Type.IsValid() && strcmp(Type.GetName(), "String") == 0)
								Series::Pack(Var->Set("data"), *(std::string*)Result.Pointer);
							else
								TH_CLEAR(Var);
							break;
						}
					}

					if (Var != nullptr)
						Cache->Set(Result.Name, Var);
				}

				Call(Entry.Serialize, [this, &Content, &Node](Script::VMContext* Context)
				{
					if (Invoke == InvokeType_Typeless)
						return;

					Component* Current = this;
					Context->SetArgObject(0, Current);
					Context->SetArgObject(1, Content);
					Context->SetArgObject(2, Node);
				});
			}
			void Scriptable::Activate(Component* New)
			{
				if (!Parent->GetScene()->IsActive())
					return;

				Call(Entry.Awake, [this, &New](Script::VMContext* Context)
				{
					if (Invoke == InvokeType_Typeless)
						return;

					Component* Current = this;
					Context->SetArgObject(0, Current);
					Context->SetArgObject(1, New);
				});
			}
			void Scriptable::Synchronize(Core::Timer* Time)
			{
				Call(Entry.Synchronize, [this, &Time](Script::VMContext* Context)
				{
					if (Invoke == InvokeType_Typeless)
						return;

					Component* Current = this;
					Context->SetArgObject(0, Current);
					Context->SetArgObject(1, Time);
				});
			}
			void Scriptable::Deactivate()
			{
				Call(Entry.Asleep, [this](Script::VMContext* Context)
				{
					if (Invoke == InvokeType_Typeless)
						return;

					Component* Current = this;
					Context->SetArgObject(0, Current);
				});
			}
			void Scriptable::Update(Core::Timer* Time)
			{
				Call(Entry.Update, [this, &Time](Script::VMContext* Context)
				{
					if (Invoke == InvokeType_Typeless)
						return;

					Component* Current = this;
					Context->SetArgObject(0, Current);
					Context->SetArgObject(1, Time);
				});
			}
			void Scriptable::Message(const std::string& Name, Core::VariantArgs& Args)
			{
				Call(Entry.Message, [this, Name, Args](Script::VMContext* Context)
				{
					if (Invoke == InvokeType_Typeless)
						return;

					Script::STDMap* Map = Script::STDMap::Create(Compiler->GetManager()->GetEngine());
					if (Map != nullptr)
					{
						int TypeId = Compiler->GetManager()->Global().GetTypeIdByDecl("Variant");
						for (auto& Item : Args)
						{
							Core::Variant Next = std::move(Item.second);
							Map->Set(Item.first, &Next, TypeId);
						}
					}

					Component* Current = this;
					Context->SetArgObject(0, Current);
					Context->SetArgObject(1, Map);
				});
			}
			Component* Scriptable::Copy(Entity* New) const
			{
				Scriptable* Target = new Scriptable(New);
				Target->Invoke = Invoke;
				Target->SetSource(Source, Resource);

				if (!Compiler || !Target->Compiler)
					return Target;

				if (Compiler->GetContext()->GetState() == Tomahawk::Script::VMRuntime::ACTIVE)
					return Target;

				if (Target->Compiler->GetContext()->GetState() == Tomahawk::Script::VMRuntime::ACTIVE)
					return Target;

				Script::VMModule From = Compiler->GetModule();
				Script::VMModule To = Target->Compiler->GetModule();
				Script::VMManager* Manager = Compiler->GetManager();

				if (!From.IsValid() || !To.IsValid())
					return Target;

				int Count = (int)From.GetPropertiesCount();
				for (int i = 0; i < Count; i++)
				{
					Script::VMProperty fSource;
					if (From.GetProperty(i, &fSource) < 0)
						continue;

					Script::VMProperty Dest;
					if (To.GetProperty(i, &Dest) < 0)
						continue;

					if (fSource.TypeId != Dest.TypeId)
						continue;

					if (fSource.TypeId < (int)Script::VMTypeId::BOOL || fSource.TypeId >(int)Script::VMTypeId::DOUBLE)
					{
						Script::VMTypeInfo Type = Manager->Global().GetTypeInfoById(fSource.TypeId);
						if (fSource.Pointer != nullptr && Type.IsValid())
						{
							void* Object = Manager->CreateObjectCopy(fSource.Pointer, Type);
							if (Object != nullptr)
								Manager->AssignObject(Dest.Pointer, Object, Type);
						}
					}
					else
					{
						int Size = Manager->Global().GetSizeOfPrimitiveType(fSource.TypeId);
						if (Size > 0)
							memcpy(Dest.Pointer, fSource.Pointer, (size_t)Size);
					}
				}

				return Target;
			}
			Core::Async<int> Scriptable::Call(const std::string& Name, unsigned int Args, Script::ArgsCallback&& OnArgs)
			{
				if (!Compiler)
					return (int)Script::VMResult::INVALID_CONFIGURATION;

				return Call(GetFunctionByName(Name, Args).GetFunction(), std::move(OnArgs));
			}
			Core::Async<int> Scriptable::Call(Script::VMCFunction* Function, Script::ArgsCallback&& OnArgs)
			{
				if (!Compiler)
					return (int)Script::VMResult::INVALID_CONFIGURATION;

				if (!Function)
					return (int)Script::VMResult::INVALID_ARG;

				Safe.lock();
				Core::Async<int> Result = Compiler->GetContext()->TryExecute(Function, [this, OnArgs = std::move(OnArgs)](Script::VMContext* Context)
				{
					this->Protect();
					if (OnArgs)
						OnArgs(Context);
				});
				Safe.unlock();

				return Result.Then<int>([this](int&& Result)
				{
					this->Unprotect();
					return Result;
				});;
			}
			Core::Async<int> Scriptable::CallEntry(const std::string& Name)
			{
				return Call(GetFunctionByName(Name, Invoke == InvokeType_Typeless ? 0 : 1).GetFunction(), [this](Script::VMContext* Context)
				{
					if (Invoke == InvokeType_Typeless)
						return;

					Component* Current = this;
					Context->SetArgObject(0, Current);
				});
			}
			int Scriptable::SetSource()
			{
				return SetSource(Source, Resource);
			}
			int Scriptable::SetSource(SourceType Type, const std::string& Data)
			{
				SceneGraph* Scene = Parent->GetScene();
				if (Compiler != nullptr)
				{
					auto* VM = Compiler->GetContext();
					if (VM->GetState() == Script::VMRuntime::ACTIVE)
						return (int)Script::VMResult::MODULE_IS_IN_USE;
				}
				else
				{
					auto* Manager = Scene->GetConf().Manager;
					if (!Manager)
						return (int)Script::VMResult::INVALID_CONFIGURATION;

					Compiler = Manager->CreateCompiler();
					Compiler->SetPragmaCallback([this](Compute::Preprocessor*, const std::string& Name, const std::vector<std::string>& Args)
					{
						if (Name == "name" && Args.size() == 1)
							Module = Args[0];

						return true;
					});
				}

				Safe.lock();
				Source = Type;
				Resource = Data;

				if (Resource.empty())
				{
					Entry.Serialize = nullptr;
					Entry.Deserialize = nullptr;
					Entry.Awake = nullptr;
					Entry.Asleep = nullptr;
					Entry.Synchronize = nullptr;
					Entry.Update = nullptr;
					Entry.Message = nullptr;
					Compiler->Clear();
					Safe.unlock();

					return (int)Script::VMResult::SUCCESS;
				}

				int R = Compiler->Prepare("base", Source == SourceType_Resource ? Resource : "anonymous", true, true);
				if (R < 0)
				{
					Safe.unlock();
					return R;
				}

				R = (Source == SourceType_Resource ? Compiler->LoadFile(Resource) : Compiler->LoadCode("anonymous", Resource));
				if (R < 0)
				{
					Safe.unlock();
					return R;
				}

				R = Compiler->Compile(true);
				if (R < 0)
				{
					Safe.unlock();
					return R;
				}

				Safe.unlock();
				Entry.Serialize = GetFunctionByName("Serialize", Invoke == InvokeType_Typeless ? 0 : 3).GetFunction();
				Entry.Deserialize = GetFunctionByName("Deserialize", Invoke == InvokeType_Typeless ? 0 : 3).GetFunction();
				Entry.Awake = GetFunctionByName("Awake", Invoke == InvokeType_Typeless ? 0 : 2).GetFunction();
				Entry.Asleep = GetFunctionByName("Asleep", Invoke == InvokeType_Typeless ? 0 : 1).GetFunction();
				Entry.Synchronize = GetFunctionByName("Synchronize", Invoke == InvokeType_Typeless ? 0 : 2).GetFunction();
				Entry.Update = GetFunctionByName("Update", Invoke == InvokeType_Typeless ? 0 : 2).GetFunction();
				Entry.Message = GetFunctionByName("Message", Invoke == InvokeType_Typeless ? 0 : 2).GetFunction();

				return R;
			}
			void Scriptable::SetInvocation(InvokeType Type)
			{
				Invoke = Type;
			}
			void Scriptable::UnsetSource()
			{
				SetSource(Source, "");
			}
			void Scriptable::Protect()
			{
				AddRef();
				GetEntity()->AddRef();
			}
			void Scriptable::Unprotect()
			{
				GetEntity()->Release();
				Release();
			}
			Script::VMCompiler* Scriptable::GetCompiler()
			{
				return Compiler;
			}
			Script::VMFunction Scriptable::GetFunctionByName(const std::string& Name, unsigned int Args)
			{
				TH_ASSERT(!Name.empty(), nullptr, "name should not be empty");
				if (!Compiler)
					return nullptr;

				auto* VM = Compiler->GetContext();
				if (VM->GetState() == Script::VMRuntime::ACTIVE)
					return nullptr;

				Safe.lock();
				auto Result = Compiler->GetModule().GetFunctionByName(Name.c_str());
				if (Result.IsValid() && Result.GetArgsCount() != Args)
				{
					Safe.unlock();
					return nullptr;
				}

				Safe.unlock();
				return Result;
			}
			Script::VMFunction Scriptable::GetFunctionByIndex(int Index, unsigned int Args)
			{
				TH_ASSERT(Index >= 0, nullptr, "index should be greater or equal to zero");
				if (!Compiler)
					return nullptr;

				auto* VM = Compiler->GetContext();
				if (VM->GetState() == Script::VMRuntime::ACTIVE)
					return nullptr;

				Safe.lock();
				auto Result = Compiler->GetModule().GetFunctionByIndex(Index);
				if (Result.IsValid() && Result.GetArgsCount() != Args)
				{
					Safe.unlock();
					return nullptr;
				}

				Safe.unlock();
				return Result;
			}
			bool Scriptable::GetPropertyByName(const char* Name, Script::VMProperty* Result)
			{
				TH_ASSERT(Name != nullptr, false, "name should be set");
				if (!Compiler)
					return false;

				auto* VM = Compiler->GetContext();
				if (VM->GetState() == Tomahawk::Script::VMRuntime::ACTIVE)
					return false;

				Safe.lock();
				Script::VMModule fModule = Compiler->GetModule();
				if (!fModule.IsValid())
				{
					Safe.unlock();
					return false;
				}

				int Index = fModule.GetPropertyIndexByName(Name);
				if (Index < 0)
				{
					Safe.unlock();
					return false;
				}

				if (fModule.GetProperty(Index, Result) < 0)
				{
					Safe.unlock();
					return false;
				}

				Safe.unlock();
				return true;
			}
			bool Scriptable::GetPropertyByIndex(int Index, Script::VMProperty* Result)
			{
				TH_ASSERT(Index >= 0, false, "index should be greater or equal to zero");
				if (!Compiler)
					return false;

				auto* VM = Compiler->GetContext();
				if (VM->GetState() == Tomahawk::Script::VMRuntime::ACTIVE)
					return false;

				Safe.lock();
				Script::VMModule fModule = Compiler->GetModule();
				if (!fModule.IsValid())
				{
					Safe.unlock();
					return false;
				}

				if (fModule.GetProperty(Index, Result) < 0)
				{
					Safe.unlock();
					return false;
				}

				Safe.unlock();
				return true;
			}
			Scriptable::SourceType Scriptable::GetSourceType()
			{
				return Source;
			}
			Scriptable::InvokeType Scriptable::GetInvokeType()
			{
				return Invoke;
			}
			int Scriptable::GetPropertiesCount()
			{
				if (!Compiler)
					return (int)Script::VMResult::INVALID_ARG;

				auto* VM = Compiler->GetContext();
				if (VM->GetState() == Tomahawk::Script::VMRuntime::ACTIVE)
					return (int)Script::VMResult::MODULE_IS_IN_USE;

				Safe.lock();
				Script::VMModule fModule = Compiler->GetModule();
				if (!fModule.IsValid())
				{
					Safe.unlock();
					return 0;
				}

				int Result = (int)fModule.GetPropertiesCount();
				Safe.unlock();

				return Result;
			}
			int Scriptable::GetFunctionsCount()
			{
				if (!Compiler)
					return (int)Script::VMResult::INVALID_ARG;

				auto* VM = Compiler->GetContext();
				if (VM->GetState() == Tomahawk::Script::VMRuntime::ACTIVE)
					return (int)Script::VMResult::MODULE_IS_IN_USE;

				Safe.lock();
				Script::VMModule fModule = Compiler->GetModule();
				if (!fModule.IsValid())
				{
					Safe.unlock();
					return 0;
				}

				int Result = (int)fModule.GetFunctionCount();
				Safe.unlock();

				return Result;
			}
			const std::string& Scriptable::GetSource()
			{
				return Resource;
			}
			const std::string& Scriptable::GetModuleName()
			{
				return Module;
			}
		}
	}
}
