#include "components.h"
#include "renderers.h"
#include "../audio/effects.h"

namespace Tomahawk
{
	namespace Engine
	{
		namespace Components
		{
			Model::Model(Entity* Ref) : Drawable(Ref, true)
			{
				Instance = nullptr;
			}
			void Model::Deserialize(ContentManager* Content, Rest::Document* Node)
			{
				std::string Path;
				if (NMake::Unpack(Node->Find("model"), &Path))
					Instance = Content->Load<Graphics::Model>(Path, nullptr);

				std::vector<Rest::Document*> Faces = Node->FindCollectionPath("surfaces.surface");
				for (auto&& Surface : Faces)
				{
					if (!Instance || !NMake::Unpack(Surface->Find("name"), &Path))
						continue;

					Graphics::MeshBuffer* Ref = Instance->Find(Path);
					if (!Ref)
						continue;

					Appearance Face;
					if (NMake::Unpack(Surface, &Face, Content))
						Surfaces[Ref] = Face;
				}

				NMake::Unpack(Node->Find("static"), &Static);
			}
			void Model::Serialize(ContentManager* Content, Rest::Document* Node)
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
						NMake::Pack(Surface->SetDocument("name"), ((Graphics::MeshBuffer*)It.first)->Name);
						NMake::Pack(Surface, It.second, Content);
					}
				}

				NMake::Pack(Node->SetDocument("static"), Static);
			}
			void Model::SetDrawable(Graphics::Model* Drawable)
			{
				Instance = Drawable;
			}
			float Model::Cull(const Viewer& View)
			{
				float Result = 0.0f;
				if (Instance != nullptr)
					Result = IsVisible(View, &GetBoundingBox());

				return Result;
			}
			Compute::Matrix4x4 Model::GetBoundingBox()
			{
				if (!Instance)
					return Parent->Transform->GetWorld();

				return Compute::Matrix4x4::Create(Parent->Transform->Position, Parent->Transform->Scale * Compute::Vector3(Instance->Min.W - Instance->Max.W).Div(2.0f).Abs(), Parent->Transform->Rotation);
			}
			Component* Model::Copy(Entity* New)
			{
				Model* Target = new Model(New);
				Target->Visibility = Visibility;
				Target->Instance = Instance;
				Target->Surfaces = Surfaces;

				return Target;
			}
			Graphics::Model* Model::GetDrawable()
			{
				return Instance;
			}

			LimpidModel::LimpidModel(Entity* Ref) : Model(Ref)
			{
			}
			Component* LimpidModel::Copy(Entity* New)
			{
				LimpidModel* Target = new LimpidModel(New);
				Target->Visibility = Visibility;
				Target->Instance = Instance;
				Target->Surfaces = Surfaces;

				return Target;
			}

			Skin::Skin(Entity* Ref) : Drawable(Ref, true)
			{
				Instance = nullptr;
			}
			void Skin::Deserialize(ContentManager* Content, Rest::Document* Node)
			{
				std::string Path;
				if (NMake::Unpack(Node->Find("skin-model"), &Path))
					Instance = Content->Load<Graphics::SkinModel>(Path, nullptr);

				std::vector<Rest::Document*> Faces = Node->FindCollectionPath("surfaces.surface");
				for (auto&& Surface : Faces)
				{
					if (!Instance || !NMake::Unpack(Surface->Find("name"), &Path))
						continue;

					Graphics::SkinMeshBuffer* Ref = Instance->FindMesh(Path);
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
					NMake::Unpack(Pose->Find("index"), &Index);

					auto& Node = Skeleton.Pose[Index];
					NMake::Unpack(Pose->Find("position"), &Node.Position);
					NMake::Unpack(Pose->Find("rotation"), &Node.Rotation);
				}

				NMake::Unpack(Node->Find("static"), &Static);
			}
			void Skin::Serialize(ContentManager* Content, Rest::Document* Node)
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
						NMake::Pack(Surface->SetDocument("name"), ((Graphics::SkinMeshBuffer*)It.first)->Name);
						NMake::Pack(Surface, It.second, Content);
					}
				}

				NMake::Pack(Node->SetDocument("static"), Static);

				Rest::Document* Poses = Node->SetArray("poses");
				for (auto&& Pose : Skeleton.Pose)
				{
					Rest::Document* Value = Poses->SetDocument("pose");
					NMake::Pack(Value->SetDocument("index"), Pose.first);
					NMake::Pack(Value->SetDocument("position"), Pose.second.Position);
					NMake::Pack(Value->SetDocument("rotation"), Pose.second.Rotation);
				}
			}
			void Skin::Synchronize(Rest::Timer* Time)
			{
				if (Instance != nullptr)
					Instance->ComputePose(&Skeleton);
			}
			void Skin::SetDrawable(Graphics::SkinModel* Drawable)
			{
				Instance = Drawable;
			}
			float Skin::Cull(const Viewer& View)
			{
				float Result = 0.0f;
				if (Instance != nullptr)
					Result = IsVisible(View, &GetBoundingBox());

				return Result;
			}
			Compute::Matrix4x4 Skin::GetBoundingBox()
			{
				if (!Instance)
					return Parent->Transform->GetWorld();

				return Compute::Matrix4x4::Create(Parent->Transform->Position, Parent->Transform->Scale * Compute::Vector3(Instance->Min.W - Instance->Max.W).Div(2.0f).Abs(), Parent->Transform->Rotation);
			}
			Component* Skin::Copy(Entity* New)
			{
				Skin* Target = new Skin(New);
				Target->Visibility = Visibility;
				Target->Instance = Instance;
				Target->Surfaces = Surfaces;

				return Target;
			}
			Graphics::SkinModel* Skin::GetDrawable()
			{
				return Instance;
			}

			LimpidSkin::LimpidSkin(Entity* Ref) : Skin(Ref)
			{
			}
			Component* LimpidSkin::Copy(Entity* New)
			{
				LimpidSkin* Target = new LimpidSkin(New);
				Target->Visibility = Visibility;
				Target->Instance = Instance;
				Target->Surfaces = Surfaces;

				return Target;
			}

			Emitter::Emitter(Entity* Ref) : Drawable(Ref, false)
			{
				Instance = nullptr;
				Connected = false;
				QuadBased = false;
				Volume = 3.0f;
			}
			Emitter::~Emitter()
			{
				if (Instance != nullptr)
					delete Instance;
			}
			void Emitter::Awake(Component* New)
			{
				if (Instance || !Parent || !Parent->GetScene())
					return;

				Graphics::InstanceBuffer::Desc I = Graphics::InstanceBuffer::Desc();
				I.ElementLimit = 1 << 10;

				Instance = Parent->GetScene()->GetDevice()->CreateInstanceBuffer(I);
			}
			void Emitter::Deserialize(ContentManager* Content, Rest::Document* Node)
			{
				NMake::Unpack(Node->Find("surface"), &Surfaces.begin()->second, Content);
				NMake::Unpack(Node->Find("quad-based"), &QuadBased);
				NMake::Unpack(Node->Find("connected"), &Connected);
				NMake::Unpack(Node->Find("static"), &Static);
				NMake::Unpack(Node->Find("volume"), &Volume);

				uint64_t Limit;
				if (!NMake::Unpack(Node->Find("limit"), &Limit))
				{
					std::vector<Compute::ElementVertex> Vertices;
					if (Instance != nullptr)
					{
						Parent->GetScene()->GetDevice()->UpdateBufferSize(Instance, Limit);
						if (NMake::Unpack(Node->Find("elements"), &Vertices))
						{
							Instance->GetArray()->Reserve(Vertices.size());
							for (auto&& Vertex : Vertices)
								Instance->GetArray()->Add(Vertex);
						}
					}
				}
			}
			void Emitter::Serialize(ContentManager* Content, Rest::Document* Node)
			{
				NMake::Pack(Node->SetDocument("surface"), Surfaces.begin()->second, Content);
				NMake::Pack(Node->SetDocument("quad-based"), QuadBased);
				NMake::Pack(Node->SetDocument("connected"), Connected);
				NMake::Pack(Node->SetDocument("static"), Static);
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
			float Emitter::Cull(const Viewer& View)
			{
				float Result = 1.0f - Parent->Transform->Position.Distance(View.WorldPosition) / (View.ViewDistance + Volume);
				if (Result > 0.0f)
					Result = Compute::MathCommon::IsClipping(View.ViewProjection, Parent->Transform->GetWorld(), 1.5f) == -1 ? Result : 0.0f;

				return Result;
			}
			Component* Emitter::Copy(Entity* New)
			{
				Emitter* Target = new Emitter(New);
				Target->Visibility = Visibility;
				Target->Volume = Volume;
				Target->Connected = Connected;
				Target->Surfaces = Surfaces;
				Target->Instance->GetArray()->Copy(*Instance->GetArray());

				return Target;
			}
			Graphics::InstanceBuffer* Emitter::GetBuffer()
			{
				return Instance;
			}

			LimpidEmitter::LimpidEmitter(Entity* Ref) : Emitter(Ref)
			{
			}
			Component* LimpidEmitter::Copy(Entity* New)
			{
				LimpidEmitter* Target = new LimpidEmitter(New);
				Target->Visibility = Visibility;
				Target->Volume = Volume;
				Target->Connected = Connected;
				Target->Surfaces = Surfaces;
				Target->Instance->GetArray()->Copy(*Instance->GetArray());

				return Target;
			}

			SoftBody::SoftBody(Entity* Ref) : Drawable(Ref, false)
			{
				Hull = nullptr;
				Instance = nullptr;
				Kinematic = false;
				Manage = true;
			}
			SoftBody::~SoftBody()
			{
				delete Instance;
			}
			void SoftBody::Deserialize(ContentManager* Content, Rest::Document* Node)
			{
				std::string Path; bool Extended;
				NMake::Unpack(Node->Find("extended"), &Extended);
				NMake::Unpack(Node->Find("kinematic"), &Kinematic);
				NMake::Unpack(Node->Find("manage"), &Manage);
				NMake::Unpack(Node->Find("static"), &Static);
				NMake::Unpack(Node->Find("surface"), &Surfaces.begin()->second, Content);

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
			void SoftBody::Serialize(ContentManager* Content, Rest::Document* Node)
			{
				NMake::Pack(Node->SetDocument("kinematic"), Kinematic);
				NMake::Pack(Node->SetDocument("manage"), Manage);
				NMake::Pack(Node->SetDocument("extended"), Instance != nullptr);
				NMake::Pack(Node->SetDocument("static"), Static);
				NMake::Pack(Node->SetDocument("surface"), Surfaces.begin()->second, Content);

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
			void SoftBody::Synchronize(Rest::Timer* Time)
			{
				if (!Instance)
					return;

				if (Manage)
					Instance->Synchronize(Parent->Transform, Kinematic);

				if (!Visibility)
					return;

				Instance->Update(&Vertices);
				if (Indices.empty())
					Instance->Reindex(&Indices);
			}
			void SoftBody::Asleep()
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
					Device->Map(VertexBuffer, Graphics::ResourceMap_Write_Discard, &Map);
					memcpy(Map.Pointer, (void*)Vertices.data(), Vertices.size() * sizeof(Compute::Vertex));
					Device->Unmap(VertexBuffer, &Map);
				}

				if (IndexBuffer != nullptr && !Indices.empty())
				{
					Device->Map(IndexBuffer, Graphics::ResourceMap_Write_Discard, &Map);
					memcpy(Map.Pointer, (void*)Indices.data(), Indices.size() * sizeof(int));
					Device->Unmap(IndexBuffer, &Map);
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
			void SoftBody::SetTransform(const Compute::Vector3& Position, const Compute::Vector3& Scale, const Compute::Vector3& Rotation)
			{
				if (!Instance)
					return;

				if (Parent && Parent->GetScene())
					Parent->GetScene()->Lock();

				Parent->Transform->SetTransform(Compute::TransformSpace_Global, Position, Scale, Rotation);
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
			float SoftBody::Cull(const Viewer& View)
			{
				float Result = 0.0f;
				if (Instance != nullptr)
					Result = IsVisible(View, &Parent->Transform->GetWorldUnscaled());

				return Result;
			}
			Component* SoftBody::Copy(Entity* New)
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

			LimpidSoftBody::LimpidSoftBody(Entity* Ref) : SoftBody(Ref)
			{
			}
			Component* LimpidSoftBody::Copy(Entity* New)
			{
				LimpidSoftBody* Target = new LimpidSoftBody(New);
				Target->Kinematic = Kinematic;

				if (Instance != nullptr)
				{
					Target->Instance = Instance->Copy();
					Target->Instance->UserPointer = Target;
				}

				return Target;
			}

			Decal::Decal(Entity* Ref) : Drawable(Ref, false)
			{
				FieldOfView = 90.0f;
				Distance = 15.0f;
			}
			void Decal::Deserialize(ContentManager* Content, Rest::Document* Node)
			{
				NMake::Unpack(Node->Find("projection"), &Projection);
				NMake::Unpack(Node->Find("view"), &View);
				NMake::Unpack(Node->Find("field-of-view"), &FieldOfView);
				NMake::Unpack(Node->Find("distance"), &Distance);
				NMake::Unpack(Node->Find("static"), &Static);
				NMake::Unpack(Node->Find("surface"), &Surfaces.begin()->second, Content);
			}
			void Decal::Serialize(ContentManager* Content, Rest::Document* Node)
			{
				NMake::Pack(Node->SetDocument("projection"), Projection);
				NMake::Pack(Node->SetDocument("view"), View);
				NMake::Pack(Node->SetDocument("field-of-view"), FieldOfView);
				NMake::Pack(Node->SetDocument("distance"), Distance);
				NMake::Pack(Node->SetDocument("static"), Static);
				NMake::Pack(Node->SetDocument("surface"), Surfaces.begin()->second, Content);
			}
			void Decal::Synchronize(Rest::Timer* Time)
			{
				Projection = Compute::Matrix4x4::CreatePerspective(FieldOfView, 1, 0.1f, Distance);
				View = Compute::Matrix4x4::CreateTranslation(-Parent->Transform->Position) * Compute::Matrix4x4::CreateCameraRotation(-Parent->Transform->Rotation);
			}
			float Decal::Cull(const Viewer& View)
			{
				float Result = 1.0f - Parent->Transform->Position.Distance(View.WorldPosition) / View.ViewDistance;
				if (Result > 0.0f)
					Result = Compute::MathCommon::IsClipping(View.ViewProjection, Parent->Transform->GetWorld(), GetRange()) == -1 ? Result : 0.0f;

				return Result;
			}
			Component* Decal::Copy(Entity* New)
			{
				Decal* Target = new Decal(New);
				Target->Visibility = Visibility;
				Target->Surfaces = Surfaces;

				return Target;
			}

			LimpidDecal::LimpidDecal(Entity* Ref) : Decal(Ref)
			{
			}
			Component* LimpidDecal::Copy(Entity* New)
			{
				LimpidDecal* Target = new LimpidDecal(New);
				Target->Visibility = Visibility;
				Target->Surfaces = Surfaces;

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
			void SkinAnimator::Deserialize(ContentManager* Content, Rest::Document* Node)
			{
				std::string Path;
				if (!NMake::Unpack(Node->Find("path"), &Path))
					NMake::Unpack(Node->Find("animation"), &Clips);
				else
					GetAnimation(Content, Path);

				NMake::Unpack(Node->Find("state"), &State);
				NMake::Unpack(Node->Find("bind"), &Bind);
				NMake::Unpack(Node->Find("current"), &Current);
			}
			void SkinAnimator::Serialize(ContentManager* Content, Rest::Document* Node)
			{
				if (Reference.empty())
					NMake::Pack(Node->SetDocument("animation"), Clips);
				else
					NMake::Pack(Node->SetDocument("path"), Reference);

				NMake::Pack(Node->SetDocument("state"), State);
				NMake::Pack(Node->SetDocument("bind"), Bind);
				NMake::Pack(Node->SetDocument("current"), Current);
			}
			void SkinAnimator::Awake(Component* New)
			{
				Components::Skin* Base = Parent->GetComponent<Components::Skin>();
				if (Base != nullptr && Base->GetDrawable() != nullptr)
				{
					Instance = Base;
					Instance->Skeleton.GetPose(Instance->GetDrawable(), &Default);
				}
				else
					Instance = nullptr;

				SetActive(Instance != nullptr);
			}
			void SkinAnimator::Synchronize(Rest::Timer* Time)
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
						Compute::AnimatorKey* Prev = &PrevKey[Pose.first];
						Compute::AnimatorKey* Next = &NextKey[Pose.first];
						Compute::AnimatorKey* Set = &Current[Pose.first];

						Set->Position = Prev->Position.Lerp(Next->Position, Timing);
						Pose.second.Position = Set->Position;

						Set->Rotation = Prev->Rotation.AngularLerp(Next->Rotation, Timing);
						Pose.second.Rotation = Set->Rotation;
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
						Compute::AnimatorKey* Prev = &Current[Pose.first];
						Compute::AnimatorKey* Next = &Key->at(Pose.first);

						Pose.second.Position = Prev->Position.Lerp(Next->Position, Timing);
						Pose.second.Rotation = Prev->Rotation.AngularLerp(Next->Rotation, Timing);
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
			bool SkinAnimator::GetAnimation(ContentManager* Content, const std::string& Path)
			{
				if (!Content)
					return false;

				Rest::Document* Result = Content->Load<Rest::Document>(Path, nullptr);
				if (!Result)
					return false;

				ClearAnimation();
				if (NMake::Unpack(Result, &Clips))
					Reference = Path;

				delete Result;
				return true;
			}
			void SkinAnimator::ClearAnimation()
			{
				Reference.clear();
				Clips.clear();
			}
			void SkinAnimator::RecordPose()
			{
				for (auto&& Pose : Instance->Skeleton.Pose)
				{
					Compute::AnimatorKey* Frame = &Bind[Pose.first];
					Frame->Position = Pose.second.Position;
					Frame->Rotation = Pose.second.Rotation;
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

				RecordPose();
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
					Compute::AnimatorKey* Frame = &Key->at(Pose.first);
					if (Pose.second.Position != Frame->Position || Pose.second.Rotation != Frame->Rotation)
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
			std::string SkinAnimator::GetPath()
			{
				return Reference;
			}
			Component* SkinAnimator::Copy(Entity* New)
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

			KeyAnimator::KeyAnimator(Entity* Ref) : Component(Ref)
			{
			}
			KeyAnimator::~KeyAnimator()
			{
			}
			void KeyAnimator::Deserialize(ContentManager* Content, Rest::Document* Node)
			{
				std::string Path;
				if (!NMake::Unpack(Node->Find("path"), &Path))
					NMake::Unpack(Node->Find("animation"), &Clips);
				else
					GetAnimation(Content, Path);

				NMake::Unpack(Node->Find("state"), &State);
				NMake::Unpack(Node->Find("bind"), &Bind);
				NMake::Unpack(Node->Find("current"), &Current);
			}
			void KeyAnimator::Serialize(ContentManager* Content, Rest::Document* Node)
			{
				if (Reference.empty())
					NMake::Pack(Node->SetDocument("animation"), Clips);
				else
					NMake::Pack(Node->SetDocument("path"), Reference);

				NMake::Pack(Node->SetDocument("state"), State);
				NMake::Pack(Node->SetDocument("bind"), Bind);
				NMake::Pack(Node->SetDocument("current"), Current);
			}
			void KeyAnimator::Synchronize(Rest::Timer* Time)
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
					State.Time = Compute::Math<float>::Min(State.Time + State.Speed * PrevKey.PlayingSpeed * (float)Time->GetDeltaTime() / State.Length, State.Length);
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

					if (State.Paused)
						return;

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
			bool KeyAnimator::GetAnimation(ContentManager* Content, const std::string& Path)
			{
				if (!Content)
					return false;

				Rest::Document* Result = Content->Load<Rest::Document>(Path, nullptr);
				if (!Result)
					return false;

				ClearAnimation();
				if (NMake::Unpack(Result, &Clips))
					Reference = Path;

				delete Result;
				return true;
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
			void KeyAnimator::RecordPose()
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

				RecordPose();
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
			std::string KeyAnimator::GetPath()
			{
				return Reference;
			}
			Component* KeyAnimator::Copy(Entity* New)
			{
				KeyAnimator* Target = new KeyAnimator(New);
				Target->Clips = Clips;
				Target->State = State;
				Target->Bind = Bind;
				Target->Current = Current;

				return Target;
			}

			EmitterAnimator::EmitterAnimator(Entity* Ref) : Component(Ref)
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
				Base = nullptr;
			}
			void EmitterAnimator::Deserialize(ContentManager* Content, Rest::Document* Node)
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
			void EmitterAnimator::Serialize(ContentManager* Content, Rest::Document* Node)
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
			void EmitterAnimator::Awake(Component* New)
			{
				Base = Parent->GetComponent<Emitter>();
				SetActive(Base != nullptr);
			}
			void EmitterAnimator::Synchronize(Rest::Timer* Time)
			{
				if (!Base || !Base->GetBuffer())
					return;

				Rest::Pool<Compute::ElementVertex>* Array = Base->GetBuffer()->GetArray();
				for (int i = 0; i < Spawner.Iterations; i++)
				{
					if (Array->Size() >= Array->Capacity())
						continue;

					Compute::Vector3 FPosition = (Base->Connected ? Spawner.Position.Generate() : Spawner.Position.Generate() + Parent->Transform->Position.InvertZ());
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
			void EmitterAnimator::AccurateSynchronization(float DeltaTime)
			{
				Rest::Pool<Compute::ElementVertex>* Array = Base->GetBuffer()->GetArray();
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
			void EmitterAnimator::FastSynchronization(float DeltaTime)
			{
				Rest::Pool<Compute::ElementVertex>* Array = Base->GetBuffer()->GetArray();
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
			Component* EmitterAnimator::Copy(Entity* New)
			{
				EmitterAnimator* Target = new EmitterAnimator(New);
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
			Emitter* EmitterAnimator::GetEmitter() const
			{
				return Base;
			}

			RigidBody::RigidBody(Entity* Ref) : Component(Ref)
			{
				Hull = nullptr;
				Instance = nullptr;
				Kinematic = false;
				Manage = true;
			}
			RigidBody::~RigidBody()
			{
				delete Instance;
			}
			void RigidBody::Deserialize(ContentManager* Content, Rest::Document* Node)
			{
				bool Extended;
				NMake::Unpack(Node->Find("extended"), &Extended);
				NMake::Unpack(Node->Find("kinematic"), &Kinematic);
				NMake::Unpack(Node->Find("manage"), &Manage);
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
			void RigidBody::Serialize(ContentManager* Content, Rest::Document* Node)
			{
				NMake::Pack(Node->SetDocument("kinematic"), Kinematic);
				NMake::Pack(Node->SetDocument("manage"), Manage);
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
			void RigidBody::Synchronize(Rest::Timer* Time)
			{
				if (Instance && Manage)
					Instance->Synchronize(Parent->Transform, Kinematic);
			}
			void RigidBody::Asleep()
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
			void RigidBody::SetTransform(const Compute::Vector3& Position, const Compute::Vector3& Scale, const Compute::Vector3& Rotation)
			{
				if (!Instance)
					return;

				if (Parent && Parent->GetScene())
					Parent->GetScene()->Lock();

				Parent->Transform->SetTransform(Compute::TransformSpace_Global, Position, Scale, Rotation);
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
			Component* RigidBody::Copy(Entity* New)
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
			void Acceleration::Deserialize(ContentManager* Content, Rest::Document* Node)
			{
				NMake::Unpack(Node->Find("amplitude-velocity"), &AmplitudeVelocity);
				NMake::Unpack(Node->Find("amplitude-torque"), &AmplitudeTorque);
				NMake::Unpack(Node->Find("constant-velocity"), &ConstantVelocity);
				NMake::Unpack(Node->Find("constant-torque"), &ConstantTorque);
				NMake::Unpack(Node->Find("constant-center"), &ConstantCenter);
				NMake::Unpack(Node->Find("velocity"), &Velocity);
			}
			void Acceleration::Serialize(ContentManager* Content, Rest::Document* Node)
			{
				NMake::Pack(Node->SetDocument("amplitude-velocity"), AmplitudeVelocity);
				NMake::Pack(Node->SetDocument("amplitude-torque"), AmplitudeTorque);
				NMake::Pack(Node->SetDocument("constant-velocity"), ConstantVelocity);
				NMake::Pack(Node->SetDocument("constant-torque"), ConstantTorque);
				NMake::Pack(Node->SetDocument("constant-center"), ConstantCenter);
				NMake::Pack(Node->SetDocument("velocity"), Velocity);
			}
			void Acceleration::Awake(Component* New)
			{
				if (RigidBody)
					return;

				Components::RigidBody* Component = Parent->GetComponent<Components::RigidBody>();
				if (Component != nullptr)
					RigidBody = Component->GetBody();
			}
			void Acceleration::Synchronize(Rest::Timer* Time)
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
			Component* Acceleration::Copy(Entity* New)
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
			Compute::RigidBody* Acceleration::GetBody() const
			{
				return RigidBody;
			}

			SliderConstraint::SliderConstraint(Entity* Ref) : Component(Ref), Instance(nullptr), Connection(nullptr)
			{
			}
			SliderConstraint::~SliderConstraint()
			{
				delete Instance;
			}
			void SliderConstraint::Deserialize(ContentManager* Content, Rest::Document* Node)
			{
				bool Extended;
				NMake::Unpack(Node->Find("extended"), &Extended);
				if (!Extended)
					return;

				uint64_t Index;
				if (NMake::Unpack(Node->Find("connection"), &Index))
				{
					if (Parent->GetScene()->HasEntity(Index))
						Connection = Parent->GetScene()->GetEntity(Index);
				}

				bool CollisionState = false, LinearPowerState = false;
				NMake::Unpack(Node->Find("collision-state"), &CollisionState);
				NMake::Unpack(Node->Find("linear-power-state"), &CollisionState);
				Initialize(Connection, CollisionState, LinearPowerState);

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
			void SliderConstraint::Serialize(ContentManager* Content, Rest::Document* Node)
			{
				NMake::Pack(Node->SetDocument("extended"), Instance != nullptr);
				if (!Instance)
					return;

				NMake::Pack(Node->SetDocument("collision-state"), Instance->GetInitialState().UseCollisions);
				NMake::Pack(Node->SetDocument("linear-power-state"), Instance->GetInitialState().UseLinearPower);
				NMake::Pack(Node->SetDocument("connection"), (uint64_t)(Connection ? Connection->Id : -1));
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
			void SliderConstraint::Initialize(Entity* Other, bool IsGhosted, bool IsLinear)
			{
				if (Parent == Other || !Parent || !Parent->GetScene())
					return;

				Parent->GetScene()->Lock();
				delete Instance;

				Connection = Other;
				if (!Connection)
					return Parent->GetScene()->Unlock();

				RigidBody* FirstBody = Parent->GetComponent<RigidBody>();
				RigidBody* SecondBody = Connection->GetComponent<RigidBody>();
				if (!FirstBody || !SecondBody)
					return Parent->GetScene()->Unlock();

				Compute::SliderConstraint::Desc I;
				I.Target1 = FirstBody->GetBody();
				I.Target2 = SecondBody->GetBody();
				I.UseCollisions = !IsGhosted;
				I.UseLinearPower = IsLinear;

				if (!I.Target1 || !I.Target2)
					return Parent->GetScene()->Unlock();

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
				Connection = nullptr;
				Parent->GetScene()->Unlock();
			}
			Component* SliderConstraint::Copy(Entity* New)
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
				Instance->GetInitialState().Target1 = FirstBody->GetBody();
				Target->Instance = Instance->Copy();
				Instance->GetInitialState() = I;

				return Target;
			}
			Compute::SliderConstraint* SliderConstraint::GetConstraint() const
			{
				return Instance;
			}
			Entity* SliderConstraint::GetConnection() const
			{
				return Connection;
			}

			FreeLook::FreeLook(Entity* Ref) : Component(Ref), Activity(nullptr), Rotate(Graphics::KeyCode_CURSORRIGHT), Sensitivity(0.005f)
			{
			}
			void FreeLook::Awake(Component* New)
			{
				Application* App = Application::Get();
				if (App == nullptr)
					return SetActive(false);

				Activity = App->Activity;
				SetActive(Activity != nullptr);
			}
			void FreeLook::Update(Rest::Timer* Time)
			{
				if (!Activity)
					return;

				if (Activity->IsKeyDown(Rotate))
				{
					Compute::Vector2 Cursor = Activity->GetGlobalCursorPosition();
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
						Activity->SetGlobalCursorPosition(Position);
				}
			}
			Component* FreeLook::Copy(Entity* New)
			{
				FreeLook* Target = new FreeLook(New);
				Target->Activity = Activity;
				Target->Position = Position;
				Target->Rotate = Rotate;
				Target->Sensitivity = Sensitivity;

				return Target;
			}
			Graphics::Activity* FreeLook::GetActivity() const
			{
				return Activity;
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
			void Fly::Awake(Component* New)
			{
				Application* App = Application::Get();
				if (App == nullptr)
					return SetActive(false);

				Activity = App->Activity;
				SetActive(Activity != nullptr);
			}
			void Fly::Update(Rest::Timer* Time)
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
			Component* Fly::Copy(Entity* New)
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

			AudioSource::AudioSource(Entity* Ref) : Component(Ref)
			{
				Source = new Audio::AudioSource();
			}
			AudioSource::~AudioSource()
			{
				delete Source;
			}
			void AudioSource::Deserialize(ContentManager* Content, Rest::Document* Node)
			{
				std::string Path;
				if (NMake::Unpack(Node->Find("audio-clip"), &Path))
					Source->SetClip(Content->Load<Audio::AudioClip>(Path, nullptr));

				NMake::Unpack(Node->Find("velocity"), &Sync.Velocity);
				NMake::Unpack(Node->Find("direction"), &Sync.Direction);
				NMake::Unpack(Node->Find("rolloff"), &Sync.Rolloff);
				NMake::Unpack(Node->Find("cone-inner-angle"), &Sync.ConeInnerAngle);
				NMake::Unpack(Node->Find("cone-outer-angle"), &Sync.ConeOuterAngle);
				NMake::Unpack(Node->Find("cone-outer-gain"), &Sync.ConeOuterGain);
				NMake::Unpack(Node->Find("distance"), &Sync.Distance);
				NMake::Unpack(Node->Find("gain"), &Sync.Gain);
				NMake::Unpack(Node->Find("pitch"), &Sync.Pitch);
				NMake::Unpack(Node->Find("ref-distance"), &Sync.RefDistance);
				NMake::Unpack(Node->Find("position"), &Sync.Position);
				NMake::Unpack(Node->Find("relative"), &Sync.IsRelative);
				NMake::Unpack(Node->Find("looped"), &Sync.IsLooped);
				NMake::Unpack(Node->Find("distance"), &Sync.Distance);
				NMake::Unpack(Node->Find("air-absorption"), &Sync.AirAbsorption);
				NMake::Unpack(Node->Find("room-roll-off"), &Sync.RoomRollOff);

				std::vector<Rest::Document*> Effects = Node->FindCollectionPath("effects.effect");
				for (auto& Effect : Effects)
				{
					uint64_t Id;
					if (!NMake::Unpack(Effect->Find("id"), &Id))
						continue;

					Audio::AudioEffect* Target = Rest::Composer::Create<Audio::AudioEffect>(Id);
					if (!Target)
					{
						THAWK_WARN("audio effect with id %llu cannot be created", Id);
						continue;
					}

					Rest::Document* Meta = Effect->Find("metadata", "");
					if (!Meta)
						Meta = Effect->SetDocument("metadata");

					Target->Deserialize(Meta);
					Source->AddEffect(Target);
				}

				bool Autoplay;
				if (NMake::Unpack(Node->Find("autoplay"), &Autoplay) && Autoplay && Source->GetClip())
					Source->Play();

				ApplyPlayingPosition();
				Synchronize(nullptr);
			}
			void AudioSource::Serialize(ContentManager* Content, Rest::Document* Node)
			{
				AssetResource* Asset = Content->FindAsset(Source->GetClip());
				if (Asset != nullptr)
					NMake::Pack(Node->SetDocument("audio-clip"), Asset->Path);

				Rest::Document* Effects = Node->SetArray("effects");
				for (auto* Effect : *Source->GetEffects())
				{
					if (!Effect)
						continue;

					Rest::Document* Element = Effects->SetDocument("effect");
					NMake::Pack(Element->SetDocument("id"), Effect->Id());
					Effect->Serialize(Element->SetDocument("metadata"));
				}

				NMake::Pack(Node->SetDocument("velocity"), Sync.Velocity);
				NMake::Pack(Node->SetDocument("direction"), Sync.Direction);
				NMake::Pack(Node->SetDocument("rolloff"), Sync.Rolloff);
				NMake::Pack(Node->SetDocument("cone-inner-angle"), Sync.ConeInnerAngle);
				NMake::Pack(Node->SetDocument("cone-outer-angle"), Sync.ConeOuterAngle);
				NMake::Pack(Node->SetDocument("cone-outer-gain"), Sync.ConeOuterGain);
				NMake::Pack(Node->SetDocument("distance"), Sync.Distance);
				NMake::Pack(Node->SetDocument("gain"), Sync.Gain);
				NMake::Pack(Node->SetDocument("pitch"), Sync.Pitch);
				NMake::Pack(Node->SetDocument("ref-distance"), Sync.RefDistance);
				NMake::Pack(Node->SetDocument("position"), Sync.Position);
				NMake::Pack(Node->SetDocument("relative"), Sync.IsRelative);
				NMake::Pack(Node->SetDocument("looped"), Sync.IsLooped);
				NMake::Pack(Node->SetDocument("distance"), Sync.Distance);
				NMake::Pack(Node->SetDocument("autoplay"), Source->IsPlaying());
				NMake::Pack(Node->SetDocument("air-absorption"), Sync.AirAbsorption);
				NMake::Pack(Node->SetDocument("room-roll-off"), Sync.RoomRollOff);
			}
			void AudioSource::Synchronize(Rest::Timer* Time)
			{
				if (Time != nullptr && Time->GetDeltaTime() > 0.0)
				{
					Sync.Velocity = (Parent->Transform->Position - LastPosition) * Time->GetDeltaTime();
					LastPosition = Parent->Transform->Position;
				}

				if (Source->GetClip())
					Source->Synchronize(&Sync, Parent->Transform->Position);
			}
			void AudioSource::ApplyPlayingPosition()
			{
				Audio::AudioContext::SetSourceData1F(Source->GetInstance(), Audio::SoundEx_Seconds_Offset, Sync.Position);
			}
			Component* AudioSource::Copy(Entity* New)
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

			AudioListener::AudioListener(Entity* Ref) : Component(Ref)
			{
				Gain = 1.0f;
			}
			AudioListener::~AudioListener()
			{
				Asleep();
			}
			void AudioListener::Deserialize(ContentManager* Content, Rest::Document* Node)
			{
				NMake::Unpack(Node->Find("gain"), &Gain);
			}
			void AudioListener::Serialize(ContentManager* Content, Rest::Document* Node)
			{
				NMake::Pack(Node->SetDocument("gain"), Gain);
			}
			void AudioListener::Synchronize(Rest::Timer* Time)
			{
				Compute::Vector3 Velocity;
				if (Time != nullptr && Time->GetDeltaTime() > 0.0)
				{
					Velocity = (Parent->Transform->Position - LastPosition) * Time->GetDeltaTime();
					LastPosition = Parent->Transform->Position;
				}

				Compute::Vector3 Rotation = Parent->Transform->Rotation.DepthDirection();
				float LookAt[6] = { Rotation.X, Rotation.Y, Rotation.Z, 0.0f, 1.0f, 0.0f };

				Audio::AudioContext::SetListenerData3F(Audio::SoundEx_Velocity, Velocity.X, Velocity.Y, Velocity.Z);
				Audio::AudioContext::SetListenerData3F(Audio::SoundEx_Position, -Parent->Transform->Position.X, -Parent->Transform->Position.Y, Parent->Transform->Position.Z);
				Audio::AudioContext::SetListenerDataVF(Audio::SoundEx_Orientation, LookAt);
				Audio::AudioContext::SetListenerData1F(Audio::SoundEx_Gain, Gain);
			}
			void AudioListener::Asleep()
			{
				float LookAt[6] = { 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f };

				Audio::AudioContext::SetListenerData3F(Audio::SoundEx_Velocity, 0.0f, 0.0f, 0.0f);
				Audio::AudioContext::SetListenerData3F(Audio::SoundEx_Position, 0.0f, 0.0f, 0.0f);
				Audio::AudioContext::SetListenerDataVF(Audio::SoundEx_Orientation, LookAt);
				Audio::AudioContext::SetListenerData1F(Audio::SoundEx_Gain, 0.0f);
			}
			Component* AudioListener::Copy(Entity* New)
			{
				AudioListener* Target = new AudioListener(New);
				Target->LastPosition = LastPosition;
				Target->Gain = Gain;

				return Target;
			}

			PointLight::PointLight(Entity* Ref) : Cullable(Ref)
			{
				ShadowCache = nullptr;
				Diffuse = Compute::Vector3::One();
				Emission = 1.0f;
			}
			void PointLight::Deserialize(ContentManager* Content, Rest::Document* Node)
			{
				NMake::Unpack(Node->Find("diffuse"), &Diffuse);
				NMake::Unpack(Node->Find("emission"), &Emission);
				NMake::Unpack(Node->Find("shadow-softness"), &ShadowSoftness);
				NMake::Unpack(Node->Find("shadow-distance"), &ShadowDistance);
				NMake::Unpack(Node->Find("shadow-bias"), &ShadowBias);
				NMake::Unpack(Node->Find("shadow-iterations"), &ShadowIterations);
				NMake::Unpack(Node->Find("shadowed"), &Shadowed);
				NMake::Unpack(Node->Find("projection"), &Projection);
				NMake::Unpack(Node->Find("view"), &View);
			}
			void PointLight::Serialize(ContentManager* Content, Rest::Document* Node)
			{
				NMake::Pack(Node->SetDocument("diffuse"), Diffuse);
				NMake::Pack(Node->SetDocument("emission"), Emission);
				NMake::Pack(Node->SetDocument("shadow-softness"), ShadowSoftness);
				NMake::Pack(Node->SetDocument("shadow-distance"), ShadowDistance);
				NMake::Pack(Node->SetDocument("shadow-bias"), ShadowBias);
				NMake::Pack(Node->SetDocument("shadow-iterations"), ShadowIterations);
				NMake::Pack(Node->SetDocument("shadowed"), Shadowed);
				NMake::Pack(Node->SetDocument("projection"), Projection);
				NMake::Pack(Node->SetDocument("view"), View);
			}
			void PointLight::Synchronize(Rest::Timer* Time)
			{
				Projection = Compute::Matrix4x4::CreatePerspective(90.0f, 1.0f, 0.1f, ShadowDistance);
				View = Compute::Matrix4x4::CreateCubeMapLookAt(0, Parent->Transform->Position.InvertZ());
			}
			float PointLight::Cull(const Viewer& View)
			{
				float Result = 1.0f - Parent->Transform->Position.Distance(View.WorldPosition) / View.ViewDistance;
				if (Result > 0.0f)
					Result = Compute::MathCommon::IsClipping(View.ViewProjection, Parent->Transform->GetWorld(), GetRange()) == -1 ? Result : 0.0f;

				return Result;
			}
			Component* PointLight::Copy(Entity* New)
			{
				PointLight* Target = new PointLight(New);
				Target->Diffuse = Diffuse;
				Target->Emission = Emission;
				Target->Visibility = Visibility;
				Target->Projection = Projection;
				Target->ShadowBias = ShadowBias;
				Target->ShadowDistance = ShadowDistance;
				Target->ShadowIterations = ShadowIterations;
				Target->ShadowSoftness = ShadowSoftness;
				Target->Shadowed = Shadowed;
				Target->View = View;

				return Target;
			}
			void PointLight::SetShadowCache(Graphics::Texture2D* NewCache)
			{
				ShadowCache = NewCache;
			}
			Graphics::Texture2D* PointLight::GetShadowCache() const
			{
				return ShadowCache;
			}

			SpotLight::SpotLight(Entity* Ref) : Cullable(Ref)
			{
				Diffuse = Compute::Vector3::One();
				ShadowCache = nullptr;
				ProjectMap = nullptr;
				ShadowSoftness = 0.0f;
				ShadowIterations = 2;
				ShadowDistance = 100;
				ShadowBias = 0.0f;
				FieldOfView = 90.0f;
				Emission = 1.0f;
				Shadowed = false;
			}
			void SpotLight::Deserialize(ContentManager* Content, Rest::Document* Node)
			{
				std::string Path;
				if (NMake::Unpack(Node->Find("project-map"), &Path))
					ProjectMap = Content->Load<Graphics::Texture2D>(Path, nullptr);

				NMake::Unpack(Node->Find("diffuse"), &Diffuse);
				NMake::Unpack(Node->Find("projection"), &Projection);
				NMake::Unpack(Node->Find("view"), &View);
				NMake::Unpack(Node->Find("shadow-bias"), &ShadowBias);
				NMake::Unpack(Node->Find("shadow-distance"), &ShadowDistance);
				NMake::Unpack(Node->Find("shadow-softness"), &ShadowSoftness);
				NMake::Unpack(Node->Find("shadow-iterations"), &ShadowIterations);
				NMake::Unpack(Node->Find("field-of-view"), &FieldOfView);
				NMake::Unpack(Node->Find("emission"), &Emission);
				NMake::Unpack(Node->Find("shadowed"), &Shadowed);
			}
			void SpotLight::Serialize(ContentManager* Content, Rest::Document* Node)
			{
				AssetResource* Asset = Content->FindAsset(ProjectMap);
				if (Asset != nullptr)
					NMake::Pack(Node->SetDocument("project-map"), Asset->Path);

				NMake::Pack(Node->SetDocument("diffuse"), Diffuse);
				NMake::Pack(Node->SetDocument("projection"), Projection);
				NMake::Pack(Node->SetDocument("view"), View);
				NMake::Pack(Node->SetDocument("shadow-bias"), ShadowBias);
				NMake::Pack(Node->SetDocument("shadow-distance"), ShadowDistance);
				NMake::Pack(Node->SetDocument("shadow-softness"), ShadowSoftness);
				NMake::Pack(Node->SetDocument("shadow-iterations"), ShadowIterations);
				NMake::Pack(Node->SetDocument("field-of-view"), FieldOfView);
				NMake::Pack(Node->SetDocument("emission"), Emission);
				NMake::Pack(Node->SetDocument("shadowed"), Shadowed);
			}
			void SpotLight::Synchronize(Rest::Timer* Time)
			{
				Projection = Compute::Matrix4x4::CreatePerspective(FieldOfView, 1, 0.1f, ShadowDistance);
				View = Compute::Matrix4x4::CreateTranslation(-Parent->Transform->Position) * Compute::Matrix4x4::CreateCameraRotation(-Parent->Transform->Rotation);
			}
			float SpotLight::Cull(const Viewer& View)
			{
				float Result = 1.0f - Parent->Transform->Position.Distance(View.WorldPosition) / View.ViewDistance;
				if (Result > 0.0f)
					Result = Compute::MathCommon::IsClipping(View.ViewProjection, Parent->Transform->GetWorld(), GetRange()) == -1 ? Result : 0.0f;

				return Result;
			}
			Component* SpotLight::Copy(Entity* New)
			{
				SpotLight* Target = new SpotLight(New);
				Target->Diffuse = Diffuse;
				Target->Projection = Projection;
				Target->View = View;
				Target->ProjectMap = ProjectMap;
				Target->FieldOfView = FieldOfView;
				Target->Emission = Emission;
				Target->Shadowed = Shadowed;
				Target->ShadowBias = ShadowBias;
				Target->ShadowDistance = ShadowDistance;
				Target->ShadowIterations = ShadowIterations;
				Target->ShadowSoftness = ShadowSoftness;

				return Target;
			}
			void SpotLight::SetShadowCache(Graphics::Texture2D* NewCache)
			{
				ShadowCache = NewCache;
			}
			Graphics::Texture2D* SpotLight::GetShadowCache() const
			{
				return ShadowCache;
			}

			LineLight::LineLight(Entity* Ref) : Component(Ref)
			{
				Diffuse = Compute::Vector3(1.0f, 1.0f, 1.0f);
				RlhEmission = Compute::Vector3(0.0000055f, 0.000013f, 0.0000224f);
				MieEmission = 0.000021f;
				RlhHeight = 8000.0f;
				MieHeight = 1200.0f;
				ScatterIntensity = 7.0f;
				PlanetRadius = 6371000.0f;
				AtmosphereRadius = 6471000.0f;
				MieDirection = 0.87f;
				ShadowCache = nullptr;
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
			void LineLight::Deserialize(ContentManager* Content, Rest::Document* Node)
			{
				NMake::Unpack(Node->Find("rlh-emission"), &RlhEmission);
				NMake::Unpack(Node->Find("mie-emission"), &MieEmission);
				NMake::Unpack(Node->Find("rlh-height"), &RlhHeight);
				NMake::Unpack(Node->Find("mie-height"), &MieEmission);
				NMake::Unpack(Node->Find("scatter-intensity"), &ScatterIntensity);
				NMake::Unpack(Node->Find("planet-radius"), &PlanetRadius);
				NMake::Unpack(Node->Find("atmosphere-radius"), &AtmosphereRadius);
				NMake::Unpack(Node->Find("mie-direction"), &MieDirection);
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
			void LineLight::Serialize(ContentManager* Content, Rest::Document* Node)
			{
				NMake::Pack(Node->SetDocument("rlh-emission"), RlhEmission);
				NMake::Pack(Node->SetDocument("mie-emission"), MieEmission);
				NMake::Pack(Node->SetDocument("rlh-height"), RlhHeight);
				NMake::Pack(Node->SetDocument("mie-height"), MieEmission);
				NMake::Pack(Node->SetDocument("scatter-intensity"), ScatterIntensity);
				NMake::Pack(Node->SetDocument("planet-radius"), PlanetRadius);
				NMake::Pack(Node->SetDocument("atmosphere-radius"), AtmosphereRadius);
				NMake::Pack(Node->SetDocument("mie-direction"), MieDirection);
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
			void LineLight::Synchronize(Rest::Timer* Time)
			{
				Projection = Compute::Matrix4x4::CreateOrthographic(ShadowDistance, ShadowDistance, -ShadowDistance / 2.0f - ShadowFarBias, ShadowDistance / 2.0f + ShadowFarBias);
				View = Compute::Matrix4x4::CreateLineLightLookAt(Parent->Transform->Position, Parent->GetScene()->GetCamera()->GetEntity()->Transform->Position.SetY(ShadowHeight));
			}
			Component* LineLight::Copy(Entity* New)
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
			void LineLight::SetShadowCache(Graphics::Texture2D* NewCache)
			{
				ShadowCache = NewCache;
			}
			Graphics::Texture2D* LineLight::GetShadowCache() const
			{
				return ShadowCache;
			}

			ProbeLight::ProbeLight(Entity* Ref) : Cullable(Ref)
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
				ProbeCache = nullptr;
				Infinity = 0.0f;
				Emission = 1.0f;
				CaptureRange = 10.0f;
				RenderLocked = false;
				ParallaxCorrected = false;
				StaticMask = false;
			}
			ProbeLight::~ProbeLight()
			{
				delete ProbeCache;
			}
			void ProbeLight::Deserialize(ContentManager* Content, Rest::Document* Node)
			{
				std::string Path;
				if (!NMake::Unpack(Node->Find("diffuse-map"), &Path))
				{
					if (NMake::Unpack(Node->Find("diffuse-map-px"), &Path))
						DiffuseMapX[0] = Content->Load<Graphics::Texture2D>(Path, nullptr);

					if (NMake::Unpack(Node->Find("diffuse-map-nx"), &Path))
						DiffuseMapX[1] = Content->Load<Graphics::Texture2D>(Path, nullptr);

					if (NMake::Unpack(Node->Find("diffuse-map-py"), &Path))
						DiffuseMapY[0] = Content->Load<Graphics::Texture2D>(Path, nullptr);

					if (NMake::Unpack(Node->Find("diffuse-map-ny"), &Path))
						DiffuseMapY[1] = Content->Load<Graphics::Texture2D>(Path, nullptr);

					if (NMake::Unpack(Node->Find("diffuse-map-pz"), &Path))
						DiffuseMapZ[0] = Content->Load<Graphics::Texture2D>(Path, nullptr);

					if (NMake::Unpack(Node->Find("diffuse-map-nz"), &Path))
						DiffuseMapZ[1] = Content->Load<Graphics::Texture2D>(Path, nullptr);
				}
				else
					DiffuseMap = Content->Load<Graphics::Texture2D>(Path, nullptr);

				std::vector<Compute::Matrix4x4> Views;
				NMake::Unpack(Node->Find("view"), &Views);
				NMake::Unpack(Node->Find("rebuild"), &Rebuild);
				NMake::Unpack(Node->Find("projection"), &Projection);
				NMake::Unpack(Node->Find("diffuse"), &Diffuse);
				NMake::Unpack(Node->Find("capture-range"), &CaptureRange);
				NMake::Unpack(Node->Find("emission"), &Emission);
				NMake::Unpack(Node->Find("infinity"), &Infinity);
				NMake::Unpack(Node->Find("parallax-corrected"), &ParallaxCorrected);
				NMake::Unpack(Node->Find("static-mask"), &StaticMask);

				int64_t Count = Compute::Math<int64_t>::Min((int64_t)Views.size(), 6);
				for (int64_t i = 0; i < Count; i++)
					View[i] = Views[i];

				if (!DiffuseMap)
					SetDiffuseMap(DiffuseMapX, DiffuseMapY, DiffuseMapZ);
				else
					SetDiffuseMap(DiffuseMap);
			}
			void ProbeLight::Serialize(ContentManager* Content, Rest::Document* Node)
			{
				AssetResource* Asset = nullptr;
				if (!DiffuseMap)
				{
					Asset = Content->FindAsset(DiffuseMapX[0]);
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
				}
				else
				{
					Asset = Content->FindAsset(DiffuseMap);
					if (Asset != nullptr)
						NMake::Pack(Node->SetDocument("diffuse-map"), Asset->Path);
				}

				std::vector<Compute::Matrix4x4> Views;
				for (int64_t i = 0; i < 6; i++)
					Views.push_back(View[i]);

				NMake::Pack(Node->SetDocument("view"), Views);
				NMake::Pack(Node->SetDocument("rebuild"), Rebuild);
				NMake::Pack(Node->SetDocument("projection"), Projection);
				NMake::Pack(Node->SetDocument("diffuse"), Diffuse);
				NMake::Pack(Node->SetDocument("capture-range"), CaptureRange);
				NMake::Pack(Node->SetDocument("emission"), Emission);
				NMake::Pack(Node->SetDocument("infinity"), Infinity);
				NMake::Pack(Node->SetDocument("parallax-corrected"), ParallaxCorrected);
				NMake::Pack(Node->SetDocument("static-mask"), StaticMask);
			}
			float ProbeLight::Cull(const Viewer& View)
			{
				float Result = 1.0f;
				if (Infinity <= 0.0f)
				{
					Result = 1.0f - Parent->Transform->Position.Distance(View.WorldPosition) / View.ViewDistance;
					if (Result > 0.0f)
						Result = Compute::MathCommon::IsClipping(View.ViewProjection, Parent->Transform->GetWorld(), GetRange()) == -1 ? Result : 0.0f;
				}

				return Result;
			}
			Component* ProbeLight::Copy(Entity* New)
			{
				ProbeLight* Target = new ProbeLight(New);
				Target->Projection = Projection;
				Target->Diffuse = Diffuse;
				Target->Visibility = Visibility;
				Target->Emission = Emission;
				Target->CaptureRange = CaptureRange;
				Target->Rebuild = Rebuild;
				memcpy(Target->View, View, 6 * sizeof(Compute::Matrix4x4));

				if (!DiffuseMap)
					Target->SetDiffuseMap(DiffuseMapX, DiffuseMapY, DiffuseMapZ);
				else
					Target->SetDiffuseMap(DiffuseMap);

				return Target;
			}
			void ProbeLight::SetProbeCache(Graphics::TextureCube* NewCache)
			{
				ProbeCache = NewCache;
			}
			bool ProbeLight::SetDiffuseMap(Graphics::Texture2D* Map)
			{
				if (!Map)
				{
					DiffuseMapX[0] = DiffuseMapX[1] = nullptr;
					DiffuseMapY[0] = DiffuseMapY[1] = nullptr;
					DiffuseMapZ[0] = DiffuseMapZ[1] = nullptr;
					DiffuseMap = nullptr;
					return false;
				}

				DiffuseMapX[0] = DiffuseMapX[1] = nullptr;
				DiffuseMapY[0] = DiffuseMapY[1] = nullptr;
				DiffuseMapZ[0] = DiffuseMapZ[1] = nullptr;
				DiffuseMap = Map;

				delete ProbeCache;
				ProbeCache = Parent->GetScene()->GetDevice()->CreateTextureCube(DiffuseMap);
				return ProbeCache != nullptr;
			}
			bool ProbeLight::SetDiffuseMap(Graphics::Texture2D* MapX[2], Graphics::Texture2D* MapY[2], Graphics::Texture2D* MapZ[2])
			{
				if (!MapX[0] || !MapX[1] || !MapY[0] || !MapY[1] || !MapZ[0] || !MapZ[1])
				{
					DiffuseMapX[0] = DiffuseMapX[1] = nullptr;
					DiffuseMapY[0] = DiffuseMapY[1] = nullptr;
					DiffuseMapZ[0] = DiffuseMapZ[1] = nullptr;
					DiffuseMap = nullptr;
					return false;
				}

				Graphics::Texture2D* Resources[6];
				Resources[0] = DiffuseMapX[0] = MapX[0];
				Resources[1] = DiffuseMapX[1] = MapX[1];
				Resources[2] = DiffuseMapY[0] = MapY[0];
				Resources[3] = DiffuseMapY[1] = MapY[1];
				Resources[4] = DiffuseMapZ[0] = MapZ[0];
				Resources[5] = DiffuseMapZ[1] = MapZ[1];
				DiffuseMap = nullptr;

				delete ProbeCache;
				ProbeCache = Parent->GetScene()->GetDevice()->CreateTextureCube(Resources);
				return ProbeCache != nullptr;
			}
			bool ProbeLight::IsImageBased() const
			{
				return DiffuseMapX[0] != nullptr || DiffuseMap != nullptr;
			}
			Graphics::TextureCube* ProbeLight::GetProbeCache() const
			{
				return ProbeCache;
			}
			Graphics::Texture2D* ProbeLight::GetDiffuseMapXP()
			{
				return DiffuseMapX[0];
			}
			Graphics::Texture2D* ProbeLight::GetDiffuseMapXN()
			{
				return DiffuseMapX[1];
			}
			Graphics::Texture2D* ProbeLight::GetDiffuseMapYP()
			{
				return DiffuseMapY[0];
			}
			Graphics::Texture2D* ProbeLight::GetDiffuseMapYN()
			{
				return DiffuseMapY[1];
			}
			Graphics::Texture2D* ProbeLight::GetDiffuseMapZP()
			{
				return DiffuseMapZ[0];
			}
			Graphics::Texture2D* ProbeLight::GetDiffuseMapZN()
			{
				return DiffuseMapZ[1];
			}
			Graphics::Texture2D* ProbeLight::GetDiffuseMap()
			{
				return DiffuseMap;
			}
			
			Camera::Camera(Entity* Ref) : Component(Ref), Mode(ProjectionMode_Perspective)
			{
				Projection = Compute::Matrix4x4::Identity();
				NearPlane = 0.1f;
				FarPlane = 1000.0f;
				Width = Height = -1;
				FieldOfView = 75.0f;
			}
			Camera::~Camera()
			{
				delete Renderer;
			}
			void Camera::Awake(Component* New)
			{
				if (!Parent || !Parent->GetScene() || (New && New != this))
					return;

				if (!Renderer)
					Renderer = new RenderSystem(Parent->GetScene()->GetDevice());

				if (Renderer->GetScene() == Parent->GetScene() && New != this)
					return;

				Renderer->SetScene(Parent->GetScene());
				for (auto& Render : *Renderer->GetRenderers())
				{
					Render->Deactivate();
					Render->SetRenderer(Renderer);
					Render->Activate();
				}
			}
			void Camera::Asleep()
			{
				if (!Renderer)
					return;

				if (Parent && Parent->GetScene() && Parent->GetScene()->GetCamera() == this)
					Parent->GetScene()->SetCamera(nullptr);
			}
			void Camera::Deserialize(ContentManager* Content, Rest::Document* Node)
			{
				int _Mode = 0;
				if (NMake::Unpack(Node->Find("mode"), &_Mode))
					Mode = (ProjectionMode)_Mode;

				NMake::Unpack(Node->Find("projection"), &Projection);
				NMake::Unpack(Node->Find("field-of-view"), &FieldOfView);
				NMake::Unpack(Node->Find("far-plane"), &FarPlane);
				NMake::Unpack(Node->Find("near-plane"), &NearPlane);
				NMake::Unpack(Node->Find("width"), &Width);
				NMake::Unpack(Node->Find("height"), &Height);

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
					uint64_t Id;
					if (!NMake::Unpack(Render->Find("id"), &Id))
						continue;

					Engine::Renderer* Target = Rest::Composer::Create<Engine::Renderer>(Id, Renderer);
					if (!Renderer || !Renderer->AddRenderer(Target))
					{
						THAWK_WARN("cannot create renderer with id %llu", Id);
						continue;
					}

					Rest::Document* Meta = Render->Find("metadata");
					if (!Meta)
						Meta = Render->SetDocument("metadata");

					Target->Deactivate();
					Target->Deserialize(Content, Meta);
					Target->Activate();

					NMake::Unpack(Render->Find("active"), &Target->Active);
				}
			}
			void Camera::Serialize(ContentManager* Content, Rest::Document* Node)
			{
				NMake::Pack(Node->SetDocument("mode"), (int)Mode);
				NMake::Pack(Node->SetDocument("projection"), Projection);
				NMake::Pack(Node->SetDocument("field-of-view"), FieldOfView);
				NMake::Pack(Node->SetDocument("far-plane"), FarPlane);
				NMake::Pack(Node->SetDocument("near-plane"), NearPlane);
				NMake::Pack(Node->SetDocument("width"), Width);
				NMake::Pack(Node->SetDocument("height"), Height);

				Rest::Document* Renderers = Node->SetArray("renderers");
				for (auto& Ref : *Renderer->GetRenderers())
				{
					Rest::Document* Render = Renderers->SetDocument("renderer");
					NMake::Pack(Render->SetDocument("id"), Ref->Id());
					NMake::Pack(Render->SetDocument("active"), Ref->Active);
					Ref->Serialize(Content, Render->SetDocument("metadata"));
				}
			}
			void Camera::Synchronize(Rest::Timer* Time)
			{
				float W = Width, H = Height;
				if (W <= 0 || H <= 0 && Renderer != nullptr)
				{
					Graphics::Viewport V = Renderer->GetDevice()->GetRenderTarget()->GetViewport();
					W = V.Width; H = V.Height;
				}
				
				if (Mode == ProjectionMode_Perspective)
					Projection = Compute::Matrix4x4::CreatePerspective(FieldOfView, W / H, NearPlane, FarPlane);
				else if (Mode == ProjectionMode_Orthographic)
					Projection = Compute::Matrix4x4::CreateOrthographic(W, H, NearPlane, FarPlane);
			}
			void Camera::FillViewer(Viewer* View)
			{
				if (!View)
					return;

				View->WorldPosition = Parent->Transform->Position;
				View->WorldRotation = Parent->Transform->Rotation;
				View->InvViewPosition = Parent->Transform->Position.InvertZ();
				View->ViewPosition = View->InvViewPosition.Invert();
				View->View = Compute::Matrix4x4::CreateCamera(View->ViewPosition, -Parent->Transform->Rotation);
				View->Projection = Projection;
				View->ViewProjection = View->View * Projection;
				View->InvViewProjection = View->ViewProjection.Invert();
				View->ViewDistance = FarPlane;
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
					(*It)->ResizeBuffers();
			}
			Viewer Camera::GetViewer()
			{
				return FieldView;
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
			Compute::Ray Camera::GetScreenRay(const Compute::Vector2& Position)
			{
				float W = Width, H = Height;
				if (W <= 0 || H <= 0 && Renderer != nullptr)
				{
					Graphics::Viewport V = Renderer->GetDevice()->GetRenderTarget()->GetViewport();
					W = V.Width; H = V.Height;
				}

				return Compute::MathCommon::CreateCursorRay(Parent->Transform->Position, Position, Compute::Vector2(W, H), Projection.Invert(), GetView().Invert());
			}
			float Camera::GetDistance(Entity* Other)
			{
				if (!Other)
					return -1.0f;

				return Other->Transform->Position.Distance(FieldView.WorldPosition);
			}
			bool Camera::RayTest(Compute::Ray& Ray, Entity* Other)
			{
				if (!Other)
					return false;

				return Compute::MathCommon::CursorRayTest(Ray, Other->Transform->GetWorld());
			}
			bool Camera::RayTest(Compute::Ray& Ray, const Compute::Matrix4x4& World)
			{
				return Compute::MathCommon::CursorRayTest(Ray, World);
			}
			Component* Camera::Copy(Entity* New)
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
		}
	}
}