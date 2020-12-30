#include "components.h"
#include "renderers.h"
#include "../audio/effects.h"

namespace Tomahawk
{
	namespace Engine
	{
		namespace Components
		{
			Model::Model(Entity* Ref) : Drawable(Ref, Model::GetTypeId(), true)
			{
			}
			Model::~Model()
			{
				TH_RELEASE(Instance);
			}
			void Model::Deserialize(ContentManager* Content, Rest::Document* Node)
			{
				std::string Path;
				if (NMake::Unpack(Node->Find("model"), &Path))
				{
					TH_RELEASE(Instance);
					Instance = Content->Load<Graphics::Model>(Path);
				}

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

				bool Transparent = false;
				NMake::Unpack(Node->Find("transparency"), &Transparent);
				NMake::Unpack(Node->Find("static"), &Static);
				SetTransparency(Transparent);
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

				NMake::Pack(Node->SetDocument("transparency"), HasTransparency());
				NMake::Pack(Node->SetDocument("static"), Static);
			}
			void Model::Awake(Component* New)
			{
				if (!New)
					Attach();
			}
			void Model::Asleep()
			{
				Detach();
			}
			void Model::SetDrawable(Graphics::Model* Drawable)
			{
				TH_RELEASE(Instance);
				Instance = Drawable;
			}
			float Model::Cull(const Viewer& View)
			{
				float Result = 0.0f;
				if (Instance != nullptr)
				{
					Compute::Matrix4x4 Box = GetBoundingBox();
					Result = IsVisible(View, &Box);
				}

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
				Target->SetTransparency(HasTransparency());
				Target->Visibility = Visibility;
				Target->Instance = Instance;
				Target->Surfaces = Surfaces;

				if (Target->Instance != nullptr)
					Target->Instance->AddRef();

				return Target;
			}
			Graphics::Model* Model::GetDrawable()
			{
				return Instance;
			}

			Skin::Skin(Entity* Ref) : Drawable(Ref, Skin::GetTypeId(), true)
			{
			}
			Skin::~Skin()
			{
				TH_RELEASE(Instance);
			}
			void Skin::Deserialize(ContentManager* Content, Rest::Document* Node)
			{
				std::string Path;
				if (NMake::Unpack(Node->Find("skin-model"), &Path))
				{
					TH_RELEASE(Instance);
					Instance = Content->Load<Graphics::SkinModel>(Path);
				}

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

				bool Transparent = false;
				NMake::Unpack(Node->Find("transparency"), &Transparent);
				NMake::Unpack(Node->Find("static"), &Static);
				SetTransparency(Transparent);
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

				NMake::Pack(Node->SetDocument("transparency"), HasTransparency());
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
			void Skin::Awake(Component* New)
			{
				if (!New)
					Attach();
			}
			void Skin::Asleep()
			{
				Detach();
			}
			void Skin::SetDrawable(Graphics::SkinModel* Drawable)
			{
				TH_RELEASE(Instance);
				Instance = Drawable;
			}
			float Skin::Cull(const Viewer& View)
			{
				float Result = 0.0f;
				if (Instance != nullptr)
				{
					Compute::Matrix4x4 Box = GetBoundingBox();
					Result = IsVisible(View, &Box);
				}

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
				Target->SetTransparency(HasTransparency());
				Target->Visibility = Visibility;
				Target->Instance = Instance;
				Target->Surfaces = Surfaces;

				if (Target->Instance != nullptr)
					Target->Instance->AddRef();

				return Target;
			}
			Graphics::SkinModel* Skin::GetDrawable()
			{
				return Instance;
			}

			Emitter::Emitter(Entity* Ref) : Drawable(Ref, Emitter::GetTypeId(), false)
			{
			}
			Emitter::~Emitter()
			{
				TH_RELEASE(Instance);
			}
			void Emitter::Deserialize(ContentManager* Content, Rest::Document* Node)
			{
				bool Transparent = false;
				NMake::Unpack(Node->Find("surface"), &Surfaces.begin()->second, Content);
				NMake::Unpack(Node->Find("transparency"), &Transparent);
				NMake::Unpack(Node->Find("quad-based"), &QuadBased);
				NMake::Unpack(Node->Find("connected"), &Connected);
				NMake::Unpack(Node->Find("static"), &Static);
				NMake::Unpack(Node->Find("volume"), &Volume);
				SetTransparency(Transparent);

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
				NMake::Pack(Node->SetDocument("transparency"), HasTransparency());
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
			void Emitter::Awake(Component* New)
			{
				if (Instance || !Parent || !Parent->GetScene())
					return;

				Graphics::InstanceBuffer::Desc I = Graphics::InstanceBuffer::Desc();
				I.ElementLimit = 1 << 10;

				Instance = Parent->GetScene()->GetDevice()->CreateInstanceBuffer(I);
				Attach();
			}
			void Emitter::Asleep()
			{
				Detach();
			}
			float Emitter::Cull(const Viewer& View)
			{
				float Result = 1.0f - Parent->Transform->Position.Distance(View.WorldPosition) / (View.FarPlane);
				if (Result > 0.0f)
					Result = Compute::MathCommon::IsCubeInFrustum(Compute::Matrix4x4::CreateScale(Volume) * Parent->Transform->GetWorldUnscaled() * View.ViewProjection, 1.5f) == -1 ? Result : 0.0f;

				return Result;
			}
			Component* Emitter::Copy(Entity* New)
			{
				Emitter* Target = new Emitter(New);
				Target->SetTransparency(HasTransparency());
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

			SoftBody::SoftBody(Entity* Ref) : Drawable(Ref, SoftBody::GetTypeId(), false)
			{
			}
			SoftBody::~SoftBody()
			{
				TH_RELEASE(Instance);
			}
			void SoftBody::Deserialize(ContentManager* Content, Rest::Document* Node)
			{
				bool Extended = false;
				bool Transparent = false;
				std::string Path;

				NMake::Unpack(Node->Find("extended"), &Extended);
				NMake::Unpack(Node->Find("kinematic"), &Kinematic);
				NMake::Unpack(Node->Find("manage"), &Manage);
				NMake::Unpack(Node->Find("static"), &Static);
				NMake::Unpack(Node->Find("transparency"), &Transparent);
				NMake::Unpack(Node->Find("surface"), &Surfaces.begin()->second, Content);
				SetTransparency(Transparent);

				if (!Extended)
					return;

				float CcdMotionThreshold = 0;
				NMake::Unpack(Node->Find("ccd-motion-threshold"), &CcdMotionThreshold);

				Rest::Document* CV = nullptr;
				if ((CV = Node->Find("shape")) != nullptr)
				{
					if (NMake::Unpack(Node->Find("path"), &Path))
					{
						auto* Shape = Content->Load<Compute::UnmanagedShape>(Path);
						if (Shape != nullptr)
							Create(Shape, CcdMotionThreshold);
					}
				}
				else if ((CV = Node->Find("ellipsoid")) != nullptr)
				{
					Compute::SoftBody::Desc::CV::SEllipsoid Shape;
					NMake::Unpack(CV->Get("center"), &Shape.Center);
					NMake::Unpack(CV->Get("radius"), &Shape.Radius);
					NMake::Unpack(CV->Get("count"), &Shape.Count);
					CreateEllipsoid(Shape, CcdMotionThreshold);
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
					CreatePatch(Shape, CcdMotionThreshold);
				}
				else if ((CV = Node->Find("rope")) != nullptr)
				{
					Compute::SoftBody::Desc::CV::SRope Shape;
					NMake::Unpack(CV->Get("start"), &Shape.Start);
					NMake::Unpack(CV->Get("start-fixed"), &Shape.StartFixed);
					NMake::Unpack(CV->Get("end"), &Shape.End);
					NMake::Unpack(CV->Get("end-fixed"), &Shape.EndFixed);
					NMake::Unpack(CV->Get("count"), &Shape.Count);
					CreateRope(Shape, CcdMotionThreshold);
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
				NMake::Pack(Node->SetDocument("transparency"), HasTransparency());
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
			void SoftBody::Awake(Component* New)
			{
				if (!New)
					Attach();
			}
			void SoftBody::Asleep()
			{
				Detach();
				if (Instance != nullptr)
					Instance->SetAsGhost();
			}
			void SoftBody::Create(Compute::UnmanagedShape* Shape, float Anticipation)
			{
				if (!Shape || !Parent || !Parent->GetScene() || !Parent->GetScene()->GetSimulator())
					return;

				Parent->GetScene()->Lock();
				TH_RELEASE(Instance);

				Compute::SoftBody::Desc I;
				I.Anticipation = Anticipation;
				I.Shape.Convex.Hull = Shape;
				I.Shape.Convex.Enabled = true;

				Vertices = Shape->Vertices;
				Indices = Shape->Indices;

				Instance = Parent->GetScene()->GetSimulator()->CreateSoftBody(I, Parent->Transform);
				if (!Instance)
				{
					TH_ERROR("cannot create soft body");
					return;
				}

				Instance->UserPointer = this;
				Instance->SetActivity(true);
				Parent->GetScene()->Unlock();
			}
			void SoftBody::Create(ContentManager* Content, const std::string& Path, float Anticipation)
			{
				if (Content != nullptr)
				{
					Hull = Content->Load<Compute::UnmanagedShape>(Path);
					if (Hull != nullptr)
						Create(Hull, Anticipation);
				}
			}
			void SoftBody::CreateEllipsoid(const Compute::SoftBody::Desc::CV::SEllipsoid& Shape, float Anticipation)
			{
				if (!Parent || !Parent->GetScene() || !Parent->GetScene()->GetSimulator())
					return;

				Parent->GetScene()->Lock();
				TH_RELEASE(Instance);

				Compute::SoftBody::Desc I;
				I.Anticipation = Anticipation;
				I.Shape.Ellipsoid = Shape;
				I.Shape.Ellipsoid.Enabled = true;

				Instance = Parent->GetScene()->GetSimulator()->CreateSoftBody(I, Parent->Transform);
				if (!Instance)
				{
					TH_ERROR("cannot create soft body");
					return;
				}

				Instance->UserPointer = this;
				Instance->SetActivity(true);
				Parent->GetScene()->Unlock();
			}
			void SoftBody::CreatePatch(const Compute::SoftBody::Desc::CV::SPatch& Shape, float Anticipation)
			{
				if (!Parent || !Parent->GetScene() || !Parent->GetScene()->GetSimulator())
					return;

				Parent->GetScene()->Lock();
				TH_RELEASE(Instance);

				Compute::SoftBody::Desc I;
				I.Anticipation = Anticipation;
				I.Shape.Patch = Shape;
				I.Shape.Patch.Enabled = true;

				Instance = Parent->GetScene()->GetSimulator()->CreateSoftBody(I, Parent->Transform);
				if (!Instance)
				{
					TH_ERROR("cannot create soft body");
					return;
				}

				Instance->UserPointer = this;
				Instance->SetActivity(true);
				Parent->GetScene()->Unlock();
			}
			void SoftBody::CreateRope(const Compute::SoftBody::Desc::CV::SRope& Shape, float Anticipation)
			{
				if (!Parent || !Parent->GetScene() || !Parent->GetScene()->GetSimulator())
					return;

				Parent->GetScene()->Lock();
				TH_RELEASE(Instance);

				Compute::SoftBody::Desc I;
				I.Anticipation = Anticipation;
				I.Shape.Rope = Shape;
				I.Shape.Rope.Enabled = true;

				Instance = Parent->GetScene()->GetSimulator()->CreateSoftBody(I, Parent->Transform);
				if (!Instance)
				{
					TH_ERROR("cannot create soft body");
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
				TH_CLEAR(Instance);
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
				{
					Compute::Matrix4x4 Box = Parent->Transform->GetWorldUnscaled();
					Result = IsVisible(View, &Box);
				}

				return Result;
			}
			Component* SoftBody::Copy(Entity* New)
			{
				SoftBody* Target = new SoftBody(New);
				Target->SetTransparency(HasTransparency());
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

			Decal::Decal(Entity* Ref) : Drawable(Ref, Decal::GetTypeId(), false)
			{
			}
			void Decal::Deserialize(ContentManager* Content, Rest::Document* Node)
			{
				bool Transparent = false;
				NMake::Unpack(Node->Find("projection"), &Projection);
				NMake::Unpack(Node->Find("view"), &View);
				NMake::Unpack(Node->Find("field-of-view"), &FieldOfView);
				NMake::Unpack(Node->Find("distance"), &Distance);
				NMake::Unpack(Node->Find("static"), &Static);
				NMake::Unpack(Node->Find("transparency"), &Transparent);
				NMake::Unpack(Node->Find("surface"), &Surfaces.begin()->second, Content);
				SetTransparency(Transparent);
			}
			void Decal::Serialize(ContentManager* Content, Rest::Document* Node)
			{
				NMake::Pack(Node->SetDocument("projection"), Projection);
				NMake::Pack(Node->SetDocument("view"), View);
				NMake::Pack(Node->SetDocument("field-of-view"), FieldOfView);
				NMake::Pack(Node->SetDocument("distance"), Distance);
				NMake::Pack(Node->SetDocument("static"), Static);
				NMake::Pack(Node->SetDocument("transparency"), HasTransparency());
				NMake::Pack(Node->SetDocument("surface"), Surfaces.begin()->second, Content);
			}
			void Decal::Synchronize(Rest::Timer* Time)
			{
				Projection = Compute::Matrix4x4::CreatePerspective(FieldOfView, 1, 0.1f, Distance);
				View = Compute::Matrix4x4::CreateTranslation(-Parent->Transform->Position) * Compute::Matrix4x4::CreateCameraRotation(-Parent->Transform->Rotation);
			}
			void Decal::Awake(Component* New)
			{
				if (!New)
					Attach();
			}
			void Decal::Asleep()
			{
				Detach();
			}
			float Decal::Cull(const Viewer& View)
			{
				float Result = 1.0f - Parent->Transform->Position.Distance(View.WorldPosition) / View.FarPlane;
				if (Result > 0.0f)
					Result = Compute::MathCommon::IsCubeInFrustum(Parent->Transform->GetWorld() * View.ViewProjection, GetRange()) == -1 ? Result : 0.0f;

				return Result;
			}
			Component* Decal::Copy(Entity* New)
			{
				Decal* Target = new Decal(New);
				Target->Visibility = Visibility;
				Target->Surfaces = Surfaces;

				return Target;
			}

			SkinAnimator::SkinAnimator(Entity* Ref) : Component(Ref)
			{
				Current.Pose.resize(96);
				Bind.Pose.resize(96);
				Default.Pose.resize(96);
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
				if (!Reference.empty())
					NMake::Pack(Node->SetDocument("path"), Reference);
				else
					NMake::Pack(Node->SetDocument("animation"), Clips);

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
					Instance->Skeleton.GetPose(Instance->GetDrawable(), &Default.Pose);
				}
				else
					Instance = nullptr;

				SetActive(Instance != nullptr);
			}
			void SkinAnimator::Synchronize(Rest::Timer* Time)
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

						Set->Rotation = Prev->Rotation.AngularLerp(Next->Rotation, T);
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
					Compute::SkinAnimatorKey* Key = GetFrame(State.Clip, State.Frame);
					if (!Key)
						Key = &Bind;

					State.Time = Compute::Mathf::Min(State.Time + State.Rate * (float)Time->GetDeltaTime() / State.Duration, State.Duration);
					float T = Compute::Mathf::Min(State.Time / State.Duration, 1.0f);

					for (auto&& Pose : Instance->Skeleton.Pose)
					{
						Compute::AnimatorKey* Prev = &Current.Pose[Pose.first];
						Compute::AnimatorKey* Next = &Key->Pose[Pose.first];

						Pose.second.Position = Prev->Position.Lerp(Next->Position, T);
						Pose.second.Rotation = Prev->Rotation.AngularLerp(Next->Rotation, T);
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
				if (!Content)
					return false;

				Rest::Document* Result = Content->Load<Rest::Document>(Path);
				if (!Result)
					return false;

				ClearAnimation();
				if (NMake::Unpack(Result, &Clips))
					Reference = Rest::Stroke(Path).Replace(Content->GetEnvironment(), "./").Replace('\\', '/').R();

				TH_RELEASE(Result);
				return true;
			}
			void SkinAnimator::GetPose(Compute::SkinAnimatorKey* Result)
			{
				if (!Result)
					return;

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
				Compute::SkinAnimatorKey* Key = GetFrame(Clip, Frame_);
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
			Compute::SkinAnimatorKey* SkinAnimator::GetFrame(int64_t Clip, int64_t Frame)
			{
				if (Clip < 0 || Clip >= Clips.size() || Frame < 0 || Frame >= Clips[Clip].Keys.size())
					return nullptr;

				return &Clips[Clip].Keys[Frame];
			}
			std::vector<Compute::SkinAnimatorKey>* SkinAnimator::GetClip(int64_t Clip)
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
				if (!Parent->GetScene()->IsActive())
					return;

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

					State.Duration = Clip->Duration;
					State.Rate = Clip->Rate * NextKey.Time;
					State.Time = Compute::Mathf::Min(State.Time + State.Rate * (float)Time->GetDeltaTime() / State.Duration, State.Duration);

					float T = Compute::Mathf::Min(State.Time / State.Duration, 1.0f);
					Position = Current.Position = PrevKey.Position.Lerp(NextKey.Position, T);
					Rotation = Current.Rotation = PrevKey.Rotation.AngularLerp(NextKey.Rotation, T);
					Scale = Current.Scale = PrevKey.Scale.Lerp(NextKey.Scale, T);

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
					Compute::AnimatorKey* Key = GetFrame(State.Clip, State.Frame);
					if (!Key)
						Key = &Bind;

					if (State.Paused)
						return;

					State.Time = Compute::Mathf::Min(State.Time + State.Rate * (float)Time->GetDeltaTime() / State.Duration, State.Duration);
					float T = Compute::Mathf::Min(State.Time / State.Duration, 1.0f);

					Position = Current.Position.Lerp(Key->Position, T);
					Rotation = Current.Rotation.AngularLerp(Key->Rotation, T);
					Scale = Current.Scale.Lerp(Key->Scale, T);
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

				Rest::Document* Result = Content->Load<Rest::Document>(Path);
				if (!Result)
					return false;

				ClearAnimation();
				if (NMake::Unpack(Result, &Clips))
					Reference = Rest::Stroke(Path).Replace(Content->GetEnvironment(), "./").Replace('\\', '/').R();

				TH_RELEASE(Result);
				return true;
			}
			void KeyAnimator::GetPose(Compute::AnimatorKey* Result)
			{
				if (!Result)
					return;

				Result->Position = *Parent->Transform->GetLocalPosition();
				Result->Rotation = *Parent->Transform->GetLocalRotation();
				Result->Scale = *Parent->Transform->GetLocalScale();
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
			void EmitterAnimator::Deserialize(ContentManager* Content, Rest::Document* Node)
			{
				NMake::Unpack(Node->Find("diffuse"), &Diffuse);
				NMake::Unpack(Node->Find("position"), &Position);
				NMake::Unpack(Node->Find("velocity"), &Velocity);
				NMake::Unpack(Node->Find("spawner"), &Spawner);
				NMake::Unpack(Node->Find("noise"), &Noise);
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
				NMake::Pack(Node->SetDocument("noise"), Noise);
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
				if (!Parent->GetScene()->IsActive())
					return;

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
				if (Noise != 0.0f)
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
					Compute::Vector3 Noise = Spawner.Noise.Generate() / Noise;
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
				Target->Noise = Noise;
				Target->Simulate = Simulate;

				return Target;
			}
			Emitter* EmitterAnimator::GetEmitter() const
			{
				return Base;
			}

			RigidBody::RigidBody(Entity* Ref) : Component(Ref)
			{
			}
			RigidBody::~RigidBody()
			{
				TH_RELEASE(Instance);
			}
			void RigidBody::Deserialize(ContentManager* Content, Rest::Document* Node)
			{
				bool Extended = false;
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
						auto* Shape = Content->Load<Compute::UnmanagedShape>(Path);
						if (Shape != nullptr)
							Create(Shape->Shape, Mass, CcdMotionThreshold);
					}
					else if (!NMake::Unpack(CV->Find("type"), &Type))
					{
						std::vector<Compute::Vector3> Vertices;
						if (NMake::Unpack(CV->Find("data"), &Vertices))
						{
							btCollisionShape* Shape = Parent->GetScene()->GetSimulator()->CreateConvexHull(Vertices);
							if (Shape != nullptr)
								Create(Shape, Mass, CcdMotionThreshold);
						}
					}
					else
					{
						btCollisionShape* Shape = Parent->GetScene()->GetSimulator()->CreateShape((Compute::Shape)Type);
						if (Shape != nullptr)
							Create(Shape, Mass, CcdMotionThreshold);
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
			void RigidBody::Create(btCollisionShape* Shape, float Mass, float Anticipation)
			{
				if (!Shape || !Parent || !Parent->GetScene() || !Parent->GetScene()->GetSimulator())
					return;

				Parent->GetScene()->Lock();
				TH_RELEASE(Instance);

				Compute::RigidBody::Desc I;
				I.Anticipation = Anticipation;
				I.Mass = Mass;
				I.Shape = Shape;

				Instance = Parent->GetScene()->GetSimulator()->CreateRigidBody(I, Parent->Transform);
				Instance->UserPointer = this;
				Instance->SetActivity(true);
				Parent->GetScene()->Unlock();
			}
			void RigidBody::Create(ContentManager* Content, const std::string& Path, float Mass, float Anticipation)
			{
				if (Content != nullptr)
				{
					Hull = Content->Load<Compute::UnmanagedShape>(Path);
					if (Hull != nullptr)
						Create(Hull->Shape, Mass, Anticipation);
				}
			}
			void RigidBody::Clear()
			{
				if (!Instance || !Parent || !Parent->GetScene() || !Parent->GetScene()->GetSimulator())
					return;

				Parent->GetScene()->Lock();
				TH_CLEAR(Instance);
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
			}
			void Acceleration::Deserialize(ContentManager* Content, Rest::Document* Node)
			{
				NMake::Unpack(Node->Find("amplitude-velocity"), &AmplitudeVelocity);
				NMake::Unpack(Node->Find("amplitude-torque"), &AmplitudeTorque);
				NMake::Unpack(Node->Find("constant-velocity"), &ConstantVelocity);
				NMake::Unpack(Node->Find("constant-torque"), &ConstantTorque);
				NMake::Unpack(Node->Find("constant-center"), &ConstantCenter);
				NMake::Unpack(Node->Find("kinematic"), &Kinematic);
			}
			void Acceleration::Serialize(ContentManager* Content, Rest::Document* Node)
			{
				NMake::Pack(Node->SetDocument("amplitude-velocity"), AmplitudeVelocity);
				NMake::Pack(Node->SetDocument("amplitude-torque"), AmplitudeTorque);
				NMake::Pack(Node->SetDocument("constant-velocity"), ConstantVelocity);
				NMake::Pack(Node->SetDocument("constant-torque"), ConstantTorque);
				NMake::Pack(Node->SetDocument("constant-center"), ConstantCenter);
				NMake::Pack(Node->SetDocument("kinematic"), Kinematic);
			}
			void Acceleration::Awake(Component* New)
			{
				if (RigidBody)
					return;

				Components::RigidBody* Component = Parent->GetComponent<Components::RigidBody>();
				if (Component != nullptr)
					RigidBody = Component->GetBody();
			}
			void Acceleration::Update(Rest::Timer* Time)
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
			Component* Acceleration::Copy(Entity* New)
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

			SliderConstraint::SliderConstraint(Entity* Ref) : Component(Ref), Instance(nullptr), Connection(nullptr)
			{
			}
			SliderConstraint::~SliderConstraint()
			{
				TH_RELEASE(Instance);
			}
			void SliderConstraint::Deserialize(ContentManager* Content, Rest::Document* Node)
			{
				bool Extended;
				NMake::Unpack(Node->Find("extended"), &Extended);
				if (!Extended)
					return;

				uint64_t Index;
				if (NMake::Unpack(Node->Find("connection"), &Index))
					Wanted.Connection = Index;

				NMake::Unpack(Node->Find("collision-state"), &Wanted.Ghost);
				NMake::Unpack(Node->Find("linear-power-state"), &Wanted.Linear);
				Create(Connection, Wanted.Ghost, Wanted.Linear);

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
			void SliderConstraint::Synchronize(Rest::Timer* Time)
			{
				if (Wanted.Connection < 0)
					return;

				if (!Connection)
					Create(Parent->GetScene()->GetEntity(Wanted.Connection), Wanted.Ghost, Wanted.Linear);

				Wanted.Connection = -1;
			}
			void SliderConstraint::Create(Entity* Other, bool IsGhosted, bool IsLinear)
			{
				if (Parent == Other || !Parent || !Parent->GetScene())
					return;

				Parent->GetScene()->Lock();
				TH_RELEASE(Instance);

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
				TH_CLEAR(Instance);
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

			FreeLook::FreeLook(Entity* Ref) : Component(Ref), Activity(nullptr), Rotate(Graphics::KeyCode_CURSORRIGHT), Sensivity(0.005f)
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
						float X = (Cursor.Y - Position.Y) * Sensivity;
						float Y = (Cursor.X - Position.X) * Sensivity;
						Parent->Transform->Rotation += Compute::Vector3(X, Y);
						Parent->Transform->Rotation.X = Compute::Mathf::Clamp(Parent->Transform->Rotation.X, -1.57079632679f, 1.57079632679f);
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
				Target->Sensivity = Sensivity;

				return Target;
			}
			Graphics::Activity* FreeLook::GetActivity() const
			{
				return Activity;
			}

			Fly::Fly(Entity* Ref) : Component(Ref), Activity(nullptr)
			{
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
				TH_RELEASE(Source);
			}
			void AudioSource::Deserialize(ContentManager* Content, Rest::Document* Node)
			{
				std::string Path;
				if (NMake::Unpack(Node->Find("audio-clip"), &Path))
					Source->SetClip(Content->Load<Audio::AudioClip>(Path));

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
						TH_WARN("audio effect with id %llu cannot be created", Id);
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
					NMake::Pack(Element->SetDocument("id"), Effect->GetId());
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
			}
			void PointLight::Deserialize(ContentManager* Content, Rest::Document* Node)
			{
				NMake::Unpack(Node->Find("projection"), &Projection);
				NMake::Unpack(Node->Find("view"), &View);
				NMake::Unpack(Node->Find("diffuse"), &Diffuse);
				NMake::Unpack(Node->Find("emission"), &Emission);
				NMake::Unpack(Node->Find("disperse"), &Disperse);
				NMake::Unpack(Node->Find("shadow-softness"), &Shadow.Softness);
				NMake::Unpack(Node->Find("shadow-distance"), &Shadow.Distance);
				NMake::Unpack(Node->Find("shadow-bias"), &Shadow.Bias);
				NMake::Unpack(Node->Find("shadow-iterations"), &Shadow.Iterations);
				NMake::Unpack(Node->Find("shadow-enabled"), &Shadow.Enabled);
			}
			void PointLight::Serialize(ContentManager* Content, Rest::Document* Node)
			{
				NMake::Pack(Node->SetDocument("projection"), Projection);
				NMake::Pack(Node->SetDocument("view"), View);
				NMake::Pack(Node->SetDocument("diffuse"), Diffuse);
				NMake::Pack(Node->SetDocument("emission"), Emission);
				NMake::Pack(Node->SetDocument("disperse"), Disperse);
				NMake::Pack(Node->SetDocument("shadow-softness"), Shadow.Softness);
				NMake::Pack(Node->SetDocument("shadow-distance"), Shadow.Distance);
				NMake::Pack(Node->SetDocument("shadow-bias"), Shadow.Bias);
				NMake::Pack(Node->SetDocument("shadow-iterations"), Shadow.Iterations);
				NMake::Pack(Node->SetDocument("shadow-enabled"), Shadow.Enabled);
			}
			float PointLight::Cull(const Viewer& Base)
			{
				float Result = 1.0f - Parent->Transform->Position.Distance(Base.WorldPosition) / Base.FarPlane;
				if (Result > 0.0f)
					Result = Compute::MathCommon::IsCubeInFrustum(Parent->Transform->GetWorldUnscaled() * Base.ViewProjection, GetRange()) == -1 ? Result : 0.0f;

				return Result;
			}
			Component* PointLight::Copy(Entity* New)
			{
				PointLight* Target = new PointLight(New);
				Target->Diffuse = Diffuse;
				Target->Emission = Emission;
				Target->Visibility = Visibility;
				Target->Projection = Projection;
				Target->View = View;
				memcpy(&Target->Shadow, &Shadow, sizeof(Shadow));

				return Target;
			}
			void PointLight::AssembleDepthOrigin()
			{
				Projection = Compute::Matrix4x4::CreatePerspective(90.0f, 1.0f, 0.1f, Shadow.Distance);
				View = Compute::Matrix4x4::CreateCubeMapLookAt(0, Parent->Transform->Position.InvertZ());
			}
			
			SpotLight::SpotLight(Entity* Ref) : Cullable(Ref)
			{
			}
			void SpotLight::Deserialize(ContentManager* Content, Rest::Document* Node)
			{
				NMake::Unpack(Node->Find("projection"), &Projection);
				NMake::Unpack(Node->Find("view"), &View);
				NMake::Unpack(Node->Find("diffuse"), &Diffuse);
				NMake::Unpack(Node->Find("emission"), &Emission);
				NMake::Unpack(Node->Find("disperse"), &Disperse);
				NMake::Unpack(Node->Find("cutoff"), &Cutoff);
				NMake::Unpack(Node->Find("shadow-bias"), &Shadow.Bias);
				NMake::Unpack(Node->Find("shadow-distance"), &Shadow.Distance);
				NMake::Unpack(Node->Find("shadow-softness"), &Shadow.Softness);
				NMake::Unpack(Node->Find("shadow-iterations"), &Shadow.Iterations);
				NMake::Unpack(Node->Find("shadow-enabled"), &Shadow.Enabled);
			}
			void SpotLight::Serialize(ContentManager* Content, Rest::Document* Node)
			{
				NMake::Pack(Node->SetDocument("projection"), Projection);
				NMake::Pack(Node->SetDocument("view"), View);
				NMake::Pack(Node->SetDocument("diffuse"), Diffuse);
				NMake::Pack(Node->SetDocument("emission"), Emission);
				NMake::Pack(Node->SetDocument("disperse"), Disperse);
				NMake::Pack(Node->SetDocument("cutoff"), Cutoff);
				NMake::Pack(Node->SetDocument("shadow-bias"), Shadow.Bias);
				NMake::Pack(Node->SetDocument("shadow-distance"), Shadow.Distance);
				NMake::Pack(Node->SetDocument("shadow-softness"), Shadow.Softness);
				NMake::Pack(Node->SetDocument("shadow-iterations"), Shadow.Iterations);
				NMake::Pack(Node->SetDocument("shadow-enabled"), Shadow.Enabled);
			}
			void SpotLight::Synchronize(Rest::Timer* Time)
			{
				Cutoff = Compute::Mathf::Clamp(Cutoff, 0.0f, 180.0f);
			}
			float SpotLight::Cull(const Viewer& View)
			{
				float Result = 1.0f - Parent->Transform->Position.Distance(View.WorldPosition) / View.FarPlane;
				if (Result > 0.0f)
					Result = Compute::MathCommon::IsCubeInFrustum(Parent->Transform->GetWorldUnscaled() * View.ViewProjection, GetRange()) == -1 ? Result : 0.0f;

				return Result;
			}
			Component* SpotLight::Copy(Entity* New)
			{
				SpotLight* Target = new SpotLight(New);
				Target->Diffuse = Diffuse;
				Target->Projection = Projection;
				Target->View = View;
				Target->Cutoff = Cutoff;
				Target->Emission = Emission;
				memcpy(&Target->Shadow, &Shadow, sizeof(Shadow));

				return Target;
			}
			void SpotLight::AssembleDepthOrigin()
			{
				Projection = Compute::Matrix4x4::CreatePerspective(Cutoff, 1, 0.1f, Shadow.Distance);
				View = Compute::Matrix4x4::CreateTranslation(-Parent->Transform->Position) * Compute::Matrix4x4::CreateCameraRotation(-Parent->Transform->Rotation);
			}

			LineLight::LineLight(Entity* Ref) : Component(Ref)
			{
			}
			void LineLight::Deserialize(ContentManager* Content, Rest::Document* Node)
			{
				NMake::Unpack(Node->Find("diffuse"), &Diffuse);
				NMake::Unpack(Node->Find("emission"), &Emission);
				NMake::Unpack(Node->Find("disperse"), &Disperse);

				for (uint32_t i = 0; i < 6; i++)
				{
					NMake::Unpack(Node->Find("projection-" + std::to_string(i)), &Projection[i]);
					NMake::Unpack(Node->Find("view-" + std::to_string(i)), &View[i]);
				}

				for (uint32_t i = 0; i < 6; i++)
					NMake::Unpack(Node->Find("shadow-distance-" + std::to_string(i)), &Shadow.Distance[i]);

				NMake::Unpack(Node->Find("shadow-cascades"), &Shadow.Cascades);
				NMake::Unpack(Node->Find("shadow-bias"), &Shadow.Bias);
				NMake::Unpack(Node->Find("shadow-offset"), &Shadow.Offset);
				NMake::Unpack(Node->Find("shadow-softness"), &Shadow.Softness);
				NMake::Unpack(Node->Find("shadow-iterations"), &Shadow.Iterations);
				NMake::Unpack(Node->Find("shadow-enabled"), &Shadow.Enabled);
				NMake::Unpack(Node->Find("rlh-emission"), &Sky.RlhEmission);
				NMake::Unpack(Node->Find("mie-emission"), &Sky.MieEmission);
				NMake::Unpack(Node->Find("rlh-height"), &Sky.RlhHeight);
				NMake::Unpack(Node->Find("mie-height"), &Sky.MieEmission);
				NMake::Unpack(Node->Find("mie-direction"), &Sky.MieDirection);
				NMake::Unpack(Node->Find("inner-radius"), &Sky.InnerRadius);
				NMake::Unpack(Node->Find("outer-radius"), &Sky.OuterRadius);
				NMake::Unpack(Node->Find("sky-intensity"), &Sky.Intensity);
			}
			void LineLight::Serialize(ContentManager* Content, Rest::Document* Node)
			{
				NMake::Pack(Node->SetDocument("diffuse"), Diffuse);
				NMake::Pack(Node->SetDocument("emission"), Emission);
				NMake::Pack(Node->SetDocument("disperse"), Disperse);

				for (uint32_t i = 0; i < 6; i++)
				{
					NMake::Pack(Node->SetDocument("projection-" + std::to_string(i)), Projection[i]);
					NMake::Pack(Node->SetDocument("view-" + std::to_string(i)), View[i]);
				}

				for (uint32_t i = 0; i < 6; i++)
					NMake::Pack(Node->SetDocument("shadow-distance-" + std::to_string(i)), Shadow.Distance[i]);

				NMake::Pack(Node->SetDocument("shadow-cascades"), Shadow.Cascades);
				NMake::Pack(Node->SetDocument("shadow-bias"), Shadow.Bias);
				NMake::Pack(Node->SetDocument("shadow-offset"), Shadow.Offset);
				NMake::Pack(Node->SetDocument("shadow-softness"), Shadow.Softness);
				NMake::Pack(Node->SetDocument("shadow-iterations"), Shadow.Iterations);
				NMake::Pack(Node->SetDocument("shadow-enabled"), Shadow.Enabled);
				NMake::Pack(Node->SetDocument("rlh-emission"), Sky.RlhEmission);
				NMake::Pack(Node->SetDocument("mie-emission"), Sky.MieEmission);
				NMake::Pack(Node->SetDocument("rlh-height"), Sky.RlhHeight);
				NMake::Pack(Node->SetDocument("mie-height"), Sky.MieEmission);
				NMake::Pack(Node->SetDocument("mie-direction"), Sky.MieDirection);
				NMake::Pack(Node->SetDocument("inner-radius"), Sky.InnerRadius);
				NMake::Pack(Node->SetDocument("outer-radius"), Sky.OuterRadius);
				NMake::Pack(Node->SetDocument("sky-intensity"), Sky.Intensity);
			}
			Component* LineLight::Copy(Entity* New)
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
			void LineLight::AssembleDepthOrigin()
			{
				auto* Viewer = Parent->GetScene()->GetCamera()->As<Camera>();
				auto* Transform = Viewer->GetEntity()->Transform;
				Compute::Vector3 Direction = -Parent->Transform->Position.NormalizeSafe();
				Compute::Vector3 Eye = Transform->Position * Compute::Vector3(1.0f, 0.1f, 1.0f);
				Compute::Vector3 Up = Transform->GetWorld().Right();
				Compute::Matrix4x4 Look = Compute::Matrix4x4::CreateLockedLookAt(Parent->Transform->Position, Eye, Up);
				float Near = -Viewer->FarPlane - Viewer->NearPlane;
				float Far = Viewer->FarPlane;

				if (Shadow.Cascades > 6)
					return;

				for (uint32_t i = 0; i < Shadow.Cascades; i++)
				{
					float Distance = Shadow.Distance[i];
					Projection[i] = Compute::Matrix4x4::CreateOrthographic(Distance, Distance, Near, Far) *
						Compute::Matrix4x4::CreateTranslation(Compute::Vector3((float)i * Shadow.Offset, 0.0));
					View[i] = Look;
				}
			}

			ReflectionProbe::ReflectionProbe(Entity* Ref) : Cullable(Ref)
			{
				Projection = Compute::Matrix4x4::CreatePerspectiveRad(1.57079632679f, 1, 0.01f, 100.0f);
			}
			ReflectionProbe::~ReflectionProbe()
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
			void ReflectionProbe::Deserialize(ContentManager* Content, Rest::Document* Node)
			{
				std::string Path;
				if (!NMake::Unpack(Node->Find("diffuse-map"), &Path))
				{
					if (NMake::Unpack(Node->Find("diffuse-map-px"), &Path))
					{
						TH_RELEASE(DiffuseMapX[0]);
						DiffuseMapX[0] = Content->Load<Graphics::Texture2D>(Path);
					}

					if (NMake::Unpack(Node->Find("diffuse-map-nx"), &Path))
					{
						TH_RELEASE(DiffuseMapX[1]);
						DiffuseMapX[1] = Content->Load<Graphics::Texture2D>(Path);
					}

					if (NMake::Unpack(Node->Find("diffuse-map-py"), &Path))
					{
						TH_RELEASE(DiffuseMapY[0]);
						DiffuseMapY[0] = Content->Load<Graphics::Texture2D>(Path);
					}

					if (NMake::Unpack(Node->Find("diffuse-map-ny"), &Path))
					{
						TH_RELEASE(DiffuseMapY[1]);
						DiffuseMapY[1] = Content->Load<Graphics::Texture2D>(Path);
					}

					if (NMake::Unpack(Node->Find("diffuse-map-pz"), &Path))
					{
						TH_RELEASE(DiffuseMapZ[0]);
						DiffuseMapZ[0] = Content->Load<Graphics::Texture2D>(Path);
					}

					if (NMake::Unpack(Node->Find("diffuse-map-nz"), &Path))
					{
						TH_RELEASE(DiffuseMapZ[1]);
						DiffuseMapZ[1] = Content->Load<Graphics::Texture2D>(Path);
					}
				}
				else
				{
					TH_RELEASE(DiffuseMap);
					DiffuseMap = Content->Load<Graphics::Texture2D>(Path);
				}

				std::vector<Compute::Matrix4x4> Views;
				NMake::Unpack(Node->Find("projection"), &Projection);
				NMake::Unpack(Node->Find("view"), &Views);
				NMake::Unpack(Node->Find("tick"), &Tick);
				NMake::Unpack(Node->Find("diffuse"), &Diffuse);
				NMake::Unpack(Node->Find("range"), &Range);
				NMake::Unpack(Node->Find("emission"), &Emission);
				NMake::Unpack(Node->Find("infinity"), &Infinity);
				NMake::Unpack(Node->Find("parallax"), &Parallax);
				NMake::Unpack(Node->Find("static-mask"), &StaticMask);

				int64_t Count = Compute::Math<int64_t>::Min((int64_t)Views.size(), 6);
				for (int64_t i = 0; i < Count; i++)
					View[i] = Views[i];

				if (!DiffuseMap)
					SetDiffuseMap(DiffuseMapX, DiffuseMapY, DiffuseMapZ);
				else
					SetDiffuseMap(DiffuseMap);
			}
			void ReflectionProbe::Serialize(ContentManager* Content, Rest::Document* Node)
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

				NMake::Pack(Node->SetDocument("projection"), Projection);
				NMake::Pack(Node->SetDocument("view"), Views);
				NMake::Pack(Node->SetDocument("tick"), Tick);
				NMake::Pack(Node->SetDocument("diffuse"), Diffuse);
				NMake::Pack(Node->SetDocument("range"), Range);
				NMake::Pack(Node->SetDocument("emission"), Emission);
				NMake::Pack(Node->SetDocument("infinity"), Infinity);
				NMake::Pack(Node->SetDocument("parallax"), Parallax);
				NMake::Pack(Node->SetDocument("static-mask"), StaticMask);
			}
			float ReflectionProbe::Cull(const Viewer& View)
			{
				float Result = 1.0f;
				if (Infinity <= 0.0f)
				{
					Result = 1.0f - Parent->Transform->Position.Distance(View.WorldPosition) / View.FarPlane;
					if (Result > 0.0f)
						Result = Compute::MathCommon::IsCubeInFrustum(Parent->Transform->GetWorldUnscaled() * View.ViewProjection, GetRange()) == -1 ? Result : 0.0f;
				}

				return Result;
			}
			Component* ReflectionProbe::Copy(Entity* New)
			{
				ReflectionProbe* Target = new ReflectionProbe(New);
				Target->Projection = Projection;
				Target->Diffuse = Diffuse;
				Target->Visibility = Visibility;
				Target->Emission = Emission;
				Target->Range = Range;
				Target->Tick = Tick;
				memcpy(Target->View, View, 6 * sizeof(Compute::Matrix4x4));

				if (!DiffuseMap)
					Target->SetDiffuseMap(DiffuseMapX, DiffuseMapY, DiffuseMapZ);
				else
					Target->SetDiffuseMap(DiffuseMap);

				return Target;
			}
			void ReflectionProbe::SetProbeCache(Graphics::TextureCube* NewCache)
			{
				Probe = NewCache;
			}
			bool ReflectionProbe::SetDiffuseMap(Graphics::Texture2D* Map)
			{
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

				TH_RELEASE(Probe);
				Probe = Parent->GetScene()->GetDevice()->CreateTextureCube(DiffuseMap);
				return Probe != nullptr;
			}
			bool ReflectionProbe::SetDiffuseMap(Graphics::Texture2D* MapX[2], Graphics::Texture2D* MapY[2], Graphics::Texture2D* MapZ[2])
			{
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
				Resources[0] = DiffuseMapX[0] = MapX[0];
				Resources[1] = DiffuseMapX[1] = MapX[1];
				Resources[2] = DiffuseMapY[0] = MapY[0];
				Resources[3] = DiffuseMapY[1] = MapY[1];
				Resources[4] = DiffuseMapZ[0] = MapZ[0];
				Resources[5] = DiffuseMapZ[1] = MapZ[1];

				TH_RELEASE(Probe);
				Probe = Parent->GetScene()->GetDevice()->CreateTextureCube(Resources);
				return Probe != nullptr;
			}
			bool ReflectionProbe::IsImageBased() const
			{
				return DiffuseMapX[0] != nullptr || DiffuseMap != nullptr;
			}
			Graphics::TextureCube* ReflectionProbe::GetProbeCache() const
			{
				return Probe;
			}
			Graphics::Texture2D* ReflectionProbe::GetDiffuseMapXP()
			{
				return DiffuseMapX[0];
			}
			Graphics::Texture2D* ReflectionProbe::GetDiffuseMapXN()
			{
				return DiffuseMapX[1];
			}
			Graphics::Texture2D* ReflectionProbe::GetDiffuseMapYP()
			{
				return DiffuseMapY[0];
			}
			Graphics::Texture2D* ReflectionProbe::GetDiffuseMapYN()
			{
				return DiffuseMapY[1];
			}
			Graphics::Texture2D* ReflectionProbe::GetDiffuseMapZP()
			{
				return DiffuseMapZ[0];
			}
			Graphics::Texture2D* ReflectionProbe::GetDiffuseMapZN()
			{
				return DiffuseMapZ[1];
			}
			Graphics::Texture2D* ReflectionProbe::GetDiffuseMap()
			{
				return DiffuseMap;
			}

			Camera::Camera(Entity* Ref) : Component(Ref), Mode(ProjectionMode_Perspective)
			{
			}
			Camera::~Camera()
			{
				TH_RELEASE(Renderer);
			}
			void Camera::Awake(Component* New)
			{
				if (!Parent || !Parent->GetScene())
					return;

				Viewport = Parent->GetScene()->GetDevice()->GetRenderTarget()->GetViewport();
				if (New && New != this)
					return;

				if (!Renderer)
					Renderer = new RenderSystem(Parent->GetScene()->GetDevice());

				if (Renderer->GetScene() != Parent->GetScene())
				{
					Renderer->SetScene(Parent->GetScene());
					Renderer->Remount();
				}
				
				if (New == this)
					Renderer->Unmount();
				else if (!New)
					Renderer->Mount();
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

				size_t Size = Renderer->GetDepthSize();
				NMake::Unpack(Node->Find("occlusion-delay"), &Renderer->Occlusion.Delay);
				NMake::Unpack(Node->Find("occlusion-stall"), &Renderer->StallFrames);
				NMake::Unpack(Node->Find("occlusion-size"), &Size);
				NMake::Unpack(Node->Find("sorting-delay"), &Renderer->Sorting.Delay);

				bool FC = true, OC = false;
				NMake::Unpack(Node->Find("frustum-cull"), &FC);
				NMake::Unpack(Node->Find("occlusion-cull"), &OC);

				Renderer->SetFrustumCulling(FC);
				Renderer->SetOcclusionCulling(OC);

				std::vector<Rest::Document*> Renderers = Node->FindCollectionPath("renderers.renderer");
				Renderer->SetScene(Parent->GetScene());
				Renderer->SetDepthSize(Size);

				for (auto& Render : Renderers)
				{
					uint64_t Id;
					if (!NMake::Unpack(Render->Find("id"), &Id))
						continue;

					Engine::Renderer* Target = Rest::Composer::Create<Engine::Renderer>(Id, Renderer);
					if (!Renderer || !Renderer->AddRenderer(Target))
					{
						TH_WARN("cannot create renderer with id %llu", Id);
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
				NMake::Pack(Node->SetDocument("occlusion-delay"), Renderer->Occlusion.Delay);
				NMake::Pack(Node->SetDocument("occlusion-stall"), Renderer->StallFrames);
				NMake::Pack(Node->SetDocument("occlusion-size"), Renderer->GetDepthSize());
				NMake::Pack(Node->SetDocument("sorting-delay"), Renderer->Sorting.Delay);
				NMake::Pack(Node->SetDocument("frustum-cull"), Renderer->HasFrustumCulling());
				NMake::Pack(Node->SetDocument("occlusion-cull"), Renderer->HasOcclusionCulling());

				Rest::Document* Renderers = Node->SetArray("renderers");
				for (auto& Ref : *Renderer->GetRenderers())
				{
					Rest::Document* Render = Renderers->SetDocument("renderer");
					NMake::Pack(Render->SetDocument("id"), Ref->GetId());
					NMake::Pack(Render->SetDocument("active"), Ref->Active);
					Ref->Serialize(Content, Render->SetDocument("metadata"));
				}
			}
			void Camera::Synchronize(Rest::Timer* Time)
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

				if (Parent->GetScene()->GetCamera() == this)
					Renderer->Synchronize(Time, FieldView);
			}
			void Camera::GetViewer(Viewer* View)
			{
				if (!View)
					return;

				Compute::Vector3 Position = Parent->Transform->Position.InvertX().InvertY();
				Compute::Matrix4x4 World = Compute::Matrix4x4::CreateCamera(Position, -Parent->Transform->Rotation);
				View->Set(World, Projection, Parent->Transform->Position, NearPlane, FarPlane);
				View->WorldRotation = Parent->Transform->Rotation;
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
				if ((W <= 0 || H <= 0) && Renderer != nullptr)
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

			Scriptable::Scriptable(Entity* Ref) : Component(Ref), Compiler(nullptr), Source(SourceType_Resource), Invoke(InvokeType_Typeless)
			{
			}
			Scriptable::~Scriptable()
			{
				TH_RELEASE(Compiler);
			}
			void Scriptable::Deserialize(ContentManager* Content, Rest::Document* Node)
			{
				std::string Type;
				if (NMake::Unpack(Node->Find("source"), &Type))
				{
					if (Type == "memory")
						Source = SourceType_Memory;
					else if (Type == "resource")
						Source = SourceType_Resource;
				}

				if (NMake::Unpack(Node->Find("invoke"), &Type))
				{
					if (Type == "typeless")
						Invoke = InvokeType_Typeless;
					else if (Type == "normal")
						Invoke = InvokeType_Normal;
				}

				if (!NMake::Unpack(Node->Find("resource"), &Type))
					return;

				Resource = Rest::OS::Resolve(Type.c_str(), Content->GetEnvironment());
				if (Resource.empty())
					Resource = Type;

				if (SetSource() < 0)
					return;

				Rest::Document* Cache = Node->Find("cache");
				if (Cache != nullptr)
				{
					for (auto& Var : *Cache->GetNodes())
					{
						int TypeId = -1;
						if (!NMake::Unpack(Var->Find("type"), &TypeId))
							continue;

						switch (TypeId)
						{
							case Script::VMTypeId_BOOL:
							{
								bool Result = false;
								if (NMake::Unpack(Var->Find("data"), &Result))
									SetTypePropertyByName(Var->Name.c_str(), Result);
								break;
							}
							case Script::VMTypeId_INT8:
							{
								int64_t Result = 0;
								if (NMake::Unpack(Var->Find("data"), &Result))
									SetTypePropertyByName(Var->Name.c_str(), (char)Result);
								break;
							}
							case Script::VMTypeId_INT16:
							{
								int64_t Result = 0;
								if (NMake::Unpack(Var->Find("data"), &Result))
									SetTypePropertyByName(Var->Name.c_str(), (short)Result);
								break;
							}
							case Script::VMTypeId_INT32:
							{
								int64_t Result = 0;
								if (NMake::Unpack(Var->Find("data"), &Result))
									SetTypePropertyByName(Var->Name.c_str(), (int)Result);
								break;
							}
							case Script::VMTypeId_INT64:
							{
								int64_t Result = 0;
								if (NMake::Unpack(Var->Find("data"), &Result))
									SetTypePropertyByName(Var->Name.c_str(), Result);
								break;
							}
							case Script::VMTypeId_UINT8:
							{
								int64_t Result = 0;
								if (NMake::Unpack(Var->Find("data"), &Result))
									SetTypePropertyByName(Var->Name.c_str(), (unsigned char)Result);
								break;
							}
							case Script::VMTypeId_UINT16:
							{
								int64_t Result = 0;
								if (NMake::Unpack(Var->Find("data"), &Result))
									SetTypePropertyByName(Var->Name.c_str(), (unsigned short)Result);
								break;
							}
							case Script::VMTypeId_UINT32:
							{
								int64_t Result = 0;
								if (NMake::Unpack(Var->Find("data"), &Result))
									SetTypePropertyByName(Var->Name.c_str(), (unsigned int)Result);
								break;
							}
							case Script::VMTypeId_UINT64:
							{
								int64_t Result = 0;
								if (NMake::Unpack(Var->Find("data"), &Result))
									SetTypePropertyByName(Var->Name.c_str(), (uint64_t)Result);
								break;
							}
							case Script::VMTypeId_FLOAT:
							{
								float Result = 0.0f;
								if (NMake::Unpack(Var->Find("data"), &Result))
									SetTypePropertyByName(Var->Name.c_str(), Result);
								break;
							}
							case Script::VMTypeId_DOUBLE:
							{
								double Result = 0.0;
								if (NMake::Unpack(Var->Find("data"), &Result))
									SetTypePropertyByName(Var->Name.c_str(), Result);
								break;
							}
							default:
							{
								std::string Result;
								if (NMake::Unpack(Var->Find("data"), &Result))
									SetTypePropertyByName(Var->Name.c_str(), Result);
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
			void Scriptable::Serialize(ContentManager* Content, Rest::Document* Node)
			{
				if (Source == SourceType_Memory)
					NMake::Pack(Node->SetDocument("source"), "memory");
				else if (Source == SourceType_Resource)
					NMake::Pack(Node->SetDocument("source"), "resource");

				if (Invoke == InvokeType_Typeless)
					NMake::Pack(Node->SetDocument("invoke"), "typeless");
				else if (Invoke == InvokeType_Normal)
					NMake::Pack(Node->SetDocument("invoke"), "normal");

				int Count = GetPropertiesCount();
				if (!NMake::Pack(Node->SetDocument("resource"), Rest::Stroke(Resource).Replace(Content->GetEnvironment(), "./").Replace('\\', '/').R()))
					return;

				Rest::Document* Cache = Node->SetDocument("cache");
				for (int i = 0; i < Count; i++)
				{
					Script::VMProperty Result;
					if (!GetPropertyByIndex(i, &Result) || !Result.Name || !Result.Pointer)
						continue;

					Rest::Document* Var = new Rest::Document();
					NMake::Pack(Var->SetDocument("type"), Result.TypeId);

					switch (Result.TypeId)
					{
						case Script::VMTypeId_BOOL:
							NMake::Pack(Var->SetDocument("data"), *(bool*)Result.Pointer);
							break;
						case Script::VMTypeId_INT8:
							NMake::Pack(Var->SetDocument("data"), (int64_t)*(char*)Result.Pointer);
							break;
						case Script::VMTypeId_INT16:
							NMake::Pack(Var->SetDocument("data"), (int64_t)*(short*)Result.Pointer);
							break;
						case Script::VMTypeId_INT32:
							NMake::Pack(Var->SetDocument("data"), (int64_t)*(int*)Result.Pointer);
							break;
						case Script::VMTypeId_INT64:
							NMake::Pack(Var->SetDocument("data"), *(int64_t*)Result.Pointer);
							break;
						case Script::VMTypeId_UINT8:
							NMake::Pack(Var->SetDocument("data"), (int64_t)*(unsigned char*)Result.Pointer);
							break;
						case Script::VMTypeId_UINT16:
							NMake::Pack(Var->SetDocument("data"), (int64_t)*(unsigned short*)Result.Pointer);
							break;
						case Script::VMTypeId_UINT32:
							NMake::Pack(Var->SetDocument("data"), (int64_t)*(unsigned int*)Result.Pointer);
							break;
						case Script::VMTypeId_UINT64:
							NMake::Pack(Var->SetDocument("data"), (int64_t)*(uint64_t*)Result.Pointer);
							break;
						case Script::VMTypeId_FLOAT:
							NMake::Pack(Var->SetDocument("data"), (double)*(float*)Result.Pointer);
							break;
						case Script::VMTypeId_DOUBLE:
							NMake::Pack(Var->SetDocument("data"), *(double*)Result.Pointer);
							break;
						default:
						{
							Script::VMTypeInfo Type = GetCompiler()->GetManager()->Global().GetTypeInfoById(Result.TypeId);
							if (Type.IsValid() && strcmp(Type.GetName(), "String") == 0)
								NMake::Pack(Var->SetDocument("data"), *(std::string*)Result.Pointer);
							else
								TH_CLEAR(Var);
							break;
						}
					}

					if (Var != nullptr)
						Cache->SetDocument(Result.Name, Var);
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
			void Scriptable::Awake(Component* New)
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
			void Scriptable::Synchronize(Rest::Timer* Time)
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
			void Scriptable::Asleep()
			{
				Call(Entry.Asleep, [this](Script::VMContext* Context)
				{
					if (Invoke == InvokeType_Typeless)
						return;

					Component* Current = this;
					Context->SetArgObject(0, Current);
				});
			}
			void Scriptable::Update(Rest::Timer* Time)
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
			void Scriptable::Message(Event* Value)
			{
				Call(Entry.Message, [this, &Value](Script::VMContext* Context)
				{
					if (Invoke == InvokeType_Typeless)
						return;

					Component* Current = this;
					Context->SetArgObject(0, Current);
					Context->SetArgObject(1, Value);
				});
			}
			Component* Scriptable::Copy(Entity* New)
			{
				Scriptable* Target = new Scriptable(New);
				Target->Invoke = Invoke;
				Target->SetSource(Source, Resource);

				if (!Compiler || !Target->Compiler)
					return Target;

				if (Compiler->GetContext()->GetState() == Tomahawk::Script::VMExecState_ACTIVE)
					return Target;

				if (Target->Compiler->GetContext()->GetState() == Tomahawk::Script::VMExecState_ACTIVE)
					return Target;

				Script::VMModule From = Compiler->GetModule();
				Script::VMModule To = Target->Compiler->GetModule();
				Script::VMManager* Manager = Compiler->GetManager();

				if (!From.IsValid() || !To.IsValid())
					return Target;

				Safe.lock();
				int Count = From.GetPropertiesCount();
				for (int i = 0; i < Count; i++)
				{
					Script::VMProperty Source;
					if (From.GetProperty(i, &Source) < 0)
						continue;

					Script::VMProperty Dest;
					if (To.GetProperty(i, &Dest) < 0)
						continue;

					if (Source.TypeId != Dest.TypeId)
						continue;

					if (Source.TypeId < Script::VMTypeId_BOOL || Source.TypeId > Script::VMTypeId_DOUBLE)
					{
						Script::VMTypeInfo Type = Manager->Global().GetTypeInfoById(Source.TypeId);
						if (Source.Pointer != nullptr && Type.IsValid())
						{
							void* Object = Manager->CreateObjectCopy(Source.Pointer, Type);
							if (Object != nullptr)
								Manager->AssignObject(Dest.Pointer, Object, Type);
						}
					}
					else
					{
						int Size = Manager->Global().GetSizeOfPrimitiveType(Source.TypeId);
						if (Size > 0)
							memcpy(Dest.Pointer, Source.Pointer, (size_t)Size);
					}
				}

				Safe.unlock();
				return Target;
			}
			int Scriptable::Call(const std::string& Name, unsigned int Args, const InvocationCallback& ArgCallback)
			{
				if (!Compiler)
					return Script::VMResult_INVALID_CONFIGURATION;

				return Call(GetFunctionByName(Name, Args).GetFunction(), ArgCallback);
			}
			int Scriptable::Call(Script::VMCFunction* Function, const InvocationCallback& ArgCallback)
			{
				if (!Compiler)
					return Script::VMResult_INVALID_CONFIGURATION;

				if (!Function)
					return Script::VMResult_INVALID_ARG;

				auto* VM = Compiler->GetContext();
				if (VM->GetState() == Script::VMExecState_ACTIVE)
					return Script::VMResult_MODULE_IS_IN_USE;

				Safe.lock();
				int Result = VM->Prepare(Function);
				if (Result < 0)
				{
					Safe.unlock();
					return Result;
				}

				if (ArgCallback)
					ArgCallback(VM);

				Result = VM->Execute();
				Safe.unlock();

				return Result;
			}
			int Scriptable::CallEntry(const std::string& Name)
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
				if (!Scene)
					return Script::VMResult_INVALID_CONFIGURATION;

				if (Compiler != nullptr)
				{
					auto* VM = Compiler->GetContext();
					if (VM->GetState() == Script::VMExecState_ACTIVE)
						return Script::VMResult_MODULE_IS_IN_USE;
				}
				else
				{
					auto* Manager = Scene->GetConf().Manager;
					if (!Manager)
						return Script::VMResult_INVALID_CONFIGURATION;

					Compiler = Manager->CreateCompiler();
					Compiler->SetPragmaCallback([this](Compute::Preprocessor*, const std::string& Pragma)
					{
						Rest::Stroke Comment(&Pragma);
						Comment.Trim();

						auto Start = Comment.Find('(');
						if (!Start.Found)
							return false;

						auto End = Comment.ReverseFind(')');
						if (!End.Found)
							return false;

						if (!Comment.StartsWith("name"))
							return false;

						Rest::Stroke Name(Comment);
						Name.Substring(Start.End, End.Start - Start.End).Trim();
						if (Name.Get()[0] == '\"' && Name.Get()[Name.Size() - 1] == '\"')
							Name.Substring(1, Name.Size() - 2);

						if (!Name.Empty())
							Module = Name.R();

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

					return Script::VMResult_SUCCESS;
				}

				int R = Compiler->PrepareScope("base", Source == SourceType_Resource ? Resource : "anonymous");
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
			Script::VMCompiler* Scriptable::GetCompiler()
			{
				return Compiler;
			}
			Script::VMFunction Scriptable::GetFunctionByName(const std::string& Name, unsigned int Args)
			{
				if (Name.empty() || !Compiler)
					return nullptr;

				auto* VM = Compiler->GetContext();
				if (VM->GetState() == Script::VMExecState_ACTIVE)
					return nullptr;

				Safe.lock();
				auto Result = Compiler->GetModule().GetFunctionByName(Name.c_str());
				if (Result.IsValid() && Result.GetArgsCount() != Args)
					return nullptr;

				Safe.unlock();
				return Result;
			}
			Script::VMFunction Scriptable::GetFunctionByIndex(int Index, unsigned int Args)
			{
				if (Index < 0 || !Compiler)
					return nullptr;

				auto* VM = Compiler->GetContext();
				if (VM->GetState() == Script::VMExecState_ACTIVE)
					return nullptr;

				Safe.lock();
				auto Result = Compiler->GetModule().GetFunctionByIndex(Index);
				if (Result.IsValid() && Result.GetArgsCount() != Args)
					return nullptr;

				Safe.unlock();
				return Result;
			}
			bool Scriptable::GetPropertyByName(const char* Name, Script::VMProperty* Result)
			{
				if (!Name || !Compiler)
					return false;

				auto* VM = Compiler->GetContext();
				if (VM->GetState() == Tomahawk::Script::VMExecState_ACTIVE)
					return false;

				Safe.lock();
				Script::VMModule Module = Compiler->GetModule();
				if (!Module.IsValid())
				{
					Safe.unlock();
					return false;
				}

				int Index = Module.GetPropertyIndexByName(Name);
				if (Index < 0)
				{
					Safe.unlock();
					return false;
				}

				if (Module.GetProperty(Index, Result) < 0)
				{
					Safe.unlock();
					return false;
				}

				Safe.unlock();
				return true;
			}
			bool Scriptable::GetPropertyByIndex(int Index, Script::VMProperty* Result)
			{
				if (Index < 0 || !Compiler)
					return false;

				auto* VM = Compiler->GetContext();
				if (VM->GetState() == Tomahawk::Script::VMExecState_ACTIVE)
					return false;

				Safe.lock();
				Script::VMModule Module = Compiler->GetModule();
				if (!Module.IsValid())
				{
					Safe.unlock();
					return false;
				}

				if (Module.GetProperty(Index, Result) < 0)
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

				int Result = Module.GetPropertiesCount();
				Safe.unlock();

				return Result;
			}
			int Scriptable::GetFunctionsCount()
			{
				if (!Compiler)
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

				int Result = Module.GetFunctionCount();
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