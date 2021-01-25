#include "renderers.h"
#include "components.h"

namespace Tomahawk
{
	namespace Engine
	{
		namespace Renderers
		{
			Model::Model(Engine::RenderSystem* Lab) : GeometryDraw(Lab, Components::Model::GetTypeId())
			{
				DepthStencil = Lab->GetDevice()->GetDepthStencilState("less");
				BackRasterizer = Lab->GetDevice()->GetRasterizerState("cull-back");
				FrontRasterizer = Lab->GetDevice()->GetRasterizerState("cull-front");
				Blend = Lab->GetDevice()->GetBlendState("overwrite");
				Sampler = Lab->GetDevice()->GetSamplerState("trilinear-x16");
				Layout = Lab->GetDevice()->GetInputLayout("vertex");

				Graphics::Shader::Desc I = Graphics::Shader::Desc();
				if (System->GetDevice()->GetSection("shaders/model/geometry", &I.Data))
					Shaders.Geometry = System->CompileShader("mr-geometry", I, 0);

				if (System->GetDevice()->GetSection("shaders/model/voxelize", &I.Data))
					Shaders.Voxelize = System->CompileShader("mr-voxelize", I, sizeof(Lighting::VoxelBuffer));

				if (System->GetDevice()->GetSection("shaders/model/occlusion", &I.Data))
					Shaders.Occlusion = System->CompileShader("mr-occlusion", I, 0);

				if (System->GetDevice()->GetSection("shaders/model/depth/linear", &I.Data))
					Shaders.Depth.Linear = System->CompileShader("mr-depth-linear", I, 0);

				if (System->GetDevice()->GetSection("shaders/model/depth/cubic", &I.Data))
					Shaders.Depth.Cubic = System->CompileShader("mr-depth-cubic", I, sizeof(Compute::Matrix4x4) * 6);
			}
			Model::~Model()
			{
				System->FreeShader("mr-geometry", Shaders.Geometry);
				System->FreeShader("mr-voxelize", Shaders.Voxelize);
				System->FreeShader("mr-occlusion", Shaders.Occlusion);
				System->FreeShader("mr-depth-linear", Shaders.Depth.Linear);
				System->FreeShader("mr-depth-cubic", Shaders.Depth.Cubic);
			}
			void Model::Activate()
			{
				System->AddCull<Engine::Components::Model>();
			}
			void Model::Deactivate()
			{
				System->RemoveCull<Engine::Components::Model>();
			}
			void Model::CullGeometry(const Viewer& View, Rest::Pool<Drawable*>* Geometry)
			{
				Graphics::GraphicsDevice* Device = System->GetDevice();
				Device->SetRasterizerState(BackRasterizer);
				Device->SetInputLayout(Layout);
				Device->SetShader(nullptr, Graphics::ShaderType_Pixel);
				Device->SetShader(Shaders.Occlusion, Graphics::ShaderType_Vertex);

				for (auto It = Geometry->Begin(); It != Geometry->End(); It++)
				{
					Components::Model* Base = (Components::Model*)*It;
					if (!System->PassCullable(Base, CullResult_Last, nullptr))
						continue;

					auto* Drawable = Base->GetDrawable();
					if (!Drawable || Drawable->Meshes.empty())
						continue;

					if (!Base->Begin(Device))
						continue;

					if (Base->GetFragmentsCount() > 0)
					{
						for (auto&& Mesh : Drawable->Meshes)
						{
							Device->Render.World = Mesh->World * Base->GetEntity()->Transform->GetWorld();
							Device->Render.WorldViewProjection = Device->Render.World * View.ViewProjection;
							Device->UpdateBuffer(Graphics::RenderBufferType_Render);
							Device->DrawIndexed(Mesh);
						}
					}
					else
					{
						Device->Render.World = Base->GetBoundingBox();
						Device->Render.WorldViewProjection = Device->Render.World * View.ViewProjection;
						Device->UpdateBuffer(Graphics::RenderBufferType_Render);
						Device->SetIndexBuffer(System->GetBoxIBuffer(), Graphics::Format_R32_Uint);
						Device->SetVertexBuffer(System->GetBoxVBuffer(), 0);
						Device->DrawIndexed(System->GetBoxIBuffer()->GetElements(), 0, 0);
					}
					Base->End(Device);
				}
			}
			void Model::RenderGeometryResult(Rest::Timer* Time, Rest::Pool<Drawable*>* Geometry, RenderOpt Options)
			{
				Graphics::GraphicsDevice* Device = System->GetDevice();
				CullResult Cull = (Options & RenderOpt_Inner ? CullResult_Always : CullResult_Last);
				bool Static = (Options & RenderOpt_Static);

				Device->SetSamplerState(Sampler, 0);
				Device->SetDepthStencilState(DepthStencil);
				Device->SetBlendState(Blend);
				Device->SetRasterizerState(BackRasterizer);
				Device->SetInputLayout(Layout);
				Device->SetShader(Shaders.Geometry, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);
				System->GetScene()->SetSurface();

				for (auto It = Geometry->Begin(); It != Geometry->End(); ++It)
				{
					Engine::Components::Model* Base = (Engine::Components::Model*)*It;
					if (Static && !Base->Static)
						continue;

					auto* Drawable = Base->GetDrawable();
					if (!Drawable || !System->PassDrawable(Base, Cull, nullptr))
						continue;

					for (auto&& Mesh : Drawable->Meshes)
					{
						auto* Surface = Base->GetSurface(Mesh);
						if (!Surface || !Surface->FillGeometry(Device))
							continue;

						Device->Render.World = Mesh->World * Base->GetEntity()->Transform->GetWorld();
						Device->Render.WorldViewProjection = Device->Render.World * System->GetScene()->View.ViewProjection;
						Device->UpdateBuffer(Graphics::RenderBufferType_Render);
						Device->DrawIndexed(Mesh);
					}
				}

				Device->FlushTexture2D(1, 7);
			}
			void Model::RenderGeometryVoxels(Rest::Timer* Time, Rest::Pool<Drawable*>* Geometry, RenderOpt Options)
			{
				Graphics::GraphicsDevice* Device = System->GetDevice();
				Device->SetSamplerState(Sampler, 0);
				Device->SetInputLayout(Layout);
				Device->SetShader(Shaders.Voxelize, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel | Graphics::ShaderType_Geometry);
				Lighting::SetVoxelBuffer(System, Shaders.Voxelize, 3);

				Viewer& View = System->GetScene()->View;
				for (auto It = Geometry->Begin(); It != Geometry->End(); ++It)
				{
					Engine::Components::Model* Base = (Engine::Components::Model*)*It;
					auto* Drawable = Base->GetDrawable();
					if (!Drawable || !Base->IsNear(View))
						continue;

					for (auto&& Mesh : Drawable->Meshes)
					{
						auto* Surface = Base->GetSurface(Mesh);
						if (!Surface || !Surface->FillVoxels(Device))
							continue;

						Device->Render.WorldViewProjection = Device->Render.World = Mesh->World * Base->GetEntity()->Transform->GetWorld();
						Device->UpdateBuffer(Graphics::RenderBufferType_Render);
						Device->DrawIndexed(Mesh);
					}
				}

				Device->FlushTexture2D(4, 6);
				Device->SetShader(nullptr, Graphics::ShaderType_Geometry);
			}
			void Model::RenderDepthLinear(Rest::Timer* Time, Rest::Pool<Drawable*>* Geometry)
			{
				Graphics::GraphicsDevice* Device = System->GetDevice();
				Device->SetSamplerState(Sampler, 0);
				Device->SetDepthStencilState(DepthStencil);
				Device->SetBlendState(Blend);
				Device->SetRasterizerState(FrontRasterizer);
				Device->SetInputLayout(Layout);
				Device->SetShader(Shaders.Depth.Linear, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);

				for (auto It = Geometry->Begin(); It != Geometry->End(); ++It)
				{
					Engine::Components::Model* Base = (Engine::Components::Model*)*It;
					auto* Drawable = Base->GetDrawable();

					if (!Drawable || !System->PassDrawable(Base, CullResult_Always, nullptr))
						continue;

					for (auto&& Mesh : Drawable->Meshes)
					{
						auto* Surface = Base->GetSurface(Mesh);
						if (!Surface || !Surface->FillDepthLinear(Device))
							continue;

						Device->Render.World = Mesh->World * Base->GetEntity()->Transform->GetWorld();
						Device->Render.WorldViewProjection = Device->Render.World * System->GetScene()->View.ViewProjection;
						Device->UpdateBuffer(Graphics::RenderBufferType_Render);
						Device->DrawIndexed(Mesh);
					}
				}

				Device->SetTexture2D(nullptr, 1);
			}
			void Model::RenderDepthCubic(Rest::Timer* Time, Rest::Pool<Drawable*>* Geometry, Compute::Matrix4x4* ViewProjection)
			{
				Graphics::GraphicsDevice* Device = System->GetDevice();
				Device->SetSamplerState(Sampler, 0);
				Device->SetDepthStencilState(DepthStencil);
				Device->SetBlendState(Blend);
				Device->SetRasterizerState(FrontRasterizer);
				Device->SetInputLayout(Layout);
				Device->SetShader(Shaders.Depth.Cubic , Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel | Graphics::ShaderType_Geometry);
				Device->SetBuffer(Shaders.Depth.Cubic, 3, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel | Graphics::ShaderType_Geometry);
				Device->UpdateBuffer(Shaders.Depth.Cubic, ViewProjection);

				for (auto It = Geometry->Begin(); It != Geometry->End(); ++It)
				{
					Engine::Components::Model* Base = (Engine::Components::Model*)*It;
					auto* Drawable = Base->GetDrawable();

					if (!Drawable || !Base->IsNear(System->GetScene()->View))
						continue;

					for (auto&& Mesh : Drawable->Meshes)
					{
						auto* Surface = Base->GetSurface(Mesh);
						if (!Surface || !Surface->FillDepthCubic(Device))
							continue;

						Device->Render.World = Mesh->World * Base->GetEntity()->Transform->GetWorld();
						Device->UpdateBuffer(Graphics::RenderBufferType_Render);
						Device->DrawIndexed(Mesh);
					}
				}

				Device->SetTexture2D(nullptr, 1);
				Device->SetShader(nullptr, Graphics::ShaderType_Geometry);
			}

			Skin::Skin(Engine::RenderSystem* Lab) : GeometryDraw(Lab, Components::Skin::GetTypeId())
			{
				DepthStencil = Lab->GetDevice()->GetDepthStencilState("less");
				BackRasterizer = Lab->GetDevice()->GetRasterizerState("cull-back");
				FrontRasterizer = Lab->GetDevice()->GetRasterizerState("cull-front");
				Blend = Lab->GetDevice()->GetBlendState("overwrite");
				Sampler = Lab->GetDevice()->GetSamplerState("trilinear-x16");
				Layout = Lab->GetDevice()->GetInputLayout("skin-vertex");

				Graphics::Shader::Desc I = Graphics::Shader::Desc();
				if (System->GetDevice()->GetSection("shaders/skin/geometry", &I.Data))
					Shaders.Geometry = System->CompileShader("sr-geometry", I, 0);

				if (System->GetDevice()->GetSection("shaders/skin/voxelize", &I.Data))
					Shaders.Voxelize = System->CompileShader("sr-voxelize", I, sizeof(Lighting::VoxelBuffer));

				if (System->GetDevice()->GetSection("shaders/skin/occlusion", &I.Data))
					Shaders.Occlusion = System->CompileShader("sr-occlusion", I, 0);

				if (System->GetDevice()->GetSection("shaders/skin/depth/linear", &I.Data))
					Shaders.Depth.Linear = System->CompileShader("sr-depth-linear", I, 0);

				if (System->GetDevice()->GetSection("shaders/skin/depth/cubic", &I.Data))
					Shaders.Depth.Cubic = System->CompileShader("sr-depth-cubic", I, sizeof(Compute::Matrix4x4) * 6);
			}
			Skin::~Skin()
			{
				System->FreeShader("sr-geometry", Shaders.Geometry);
				System->FreeShader("sr-voxelize", Shaders.Voxelize);
				System->FreeShader("sr-occlusion", Shaders.Occlusion);
				System->FreeShader("sr-depth-linear", Shaders.Depth.Linear);
				System->FreeShader("sr-depth-cubic", Shaders.Depth.Cubic);
			}
			void Skin::Activate()
			{
				System->AddCull<Engine::Components::Skin>();
			}
			void Skin::Deactivate()
			{
				System->RemoveCull<Engine::Components::Skin>();
			}
			void Skin::CullGeometry(const Viewer& View, Rest::Pool<Drawable*>* Geometry)
			{
				Graphics::GraphicsDevice* Device = System->GetDevice();
				Device->SetRasterizerState(BackRasterizer);
				Device->SetInputLayout(Layout);
				Device->SetShader(nullptr, Graphics::ShaderType_Pixel);
				Device->SetShader(Shaders.Occlusion, Graphics::ShaderType_Vertex);

				for (auto It = Geometry->Begin(); It != Geometry->End(); It++)
				{
					Components::Skin* Base = (Components::Skin*)*It;
					if (!System->PassCullable(Base, CullResult_Last, nullptr))
						continue;

					auto* Drawable = Base->GetDrawable();
					if (!Drawable || Drawable->Meshes.empty())
						continue;

					if (!Base->Begin(Device))
						continue;

					if (Base->GetFragmentsCount() > 0)
					{
						Device->Animation.HasAnimation = !Base->GetDrawable()->Joints.empty();
						if (Device->Animation.HasAnimation > 0)
							memcpy(Device->Animation.Offsets, Base->Skeleton.Transform, 96 * sizeof(Compute::Matrix4x4));

						Device->UpdateBuffer(Graphics::RenderBufferType_Animation);
						for (auto&& Mesh : Drawable->Meshes)
						{
							Device->Render.World = Mesh->World * Base->GetEntity()->Transform->GetWorld();
							Device->Render.WorldViewProjection = Device->Render.World * View.ViewProjection;
							Device->UpdateBuffer(Graphics::RenderBufferType_Render);
							Device->DrawIndexed(Mesh);
						}
					}
					else
					{
						Device->Animation.HasAnimation = false;
						Device->Render.World = Base->GetBoundingBox();
						Device->Render.WorldViewProjection = Device->Render.World * View.ViewProjection;
						Device->UpdateBuffer(Graphics::RenderBufferType_Animation);
						Device->UpdateBuffer(Graphics::RenderBufferType_Render);
						Device->SetIndexBuffer(System->GetSkinBoxIBuffer(), Graphics::Format_R32_Uint);
						Device->SetVertexBuffer(System->GetSkinBoxVBuffer(), 0);
						Device->DrawIndexed(System->GetSkinBoxIBuffer()->GetElements(), 0, 0);
					}
					Base->End(Device);
				}
			}
			void Skin::RenderGeometryResult(Rest::Timer* Time, Rest::Pool<Drawable*>* Geometry, RenderOpt Options)
			{
				Graphics::GraphicsDevice* Device = System->GetDevice();
				CullResult Cull = (Options & RenderOpt_Inner ? CullResult_Always : CullResult_Last);
				bool Static = (Options & RenderOpt_Static);

				Device->SetSamplerState(Sampler, 0);
				Device->SetDepthStencilState(DepthStencil);
				Device->SetBlendState(Blend);
				Device->SetRasterizerState(BackRasterizer);
				Device->SetInputLayout(Layout);
				Device->SetShader(Shaders.Geometry, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);
				System->GetScene()->SetSurface();

				for (auto It = Geometry->Begin(); It != Geometry->End(); ++It)
				{
					Engine::Components::Skin* Base = (Engine::Components::Skin*)*It;
					if (Static && !Base->Static)
						continue;

					auto* Drawable = Base->GetDrawable();
					if (!Drawable || !System->PassDrawable(Base, Cull, nullptr))
						continue;

					Device->Animation.HasAnimation = !Base->GetDrawable()->Joints.empty();
					if (Device->Animation.HasAnimation > 0)
						memcpy(Device->Animation.Offsets, Base->Skeleton.Transform, 96 * sizeof(Compute::Matrix4x4));

					Device->UpdateBuffer(Graphics::RenderBufferType_Animation);
					for (auto&& Mesh : Drawable->Meshes)
					{
						auto* Surface = Base->GetSurface(Mesh);
						if (!Surface || !Surface->FillGeometry(Device))
							continue;

						Device->Render.World = Mesh->World * Base->GetEntity()->Transform->GetWorld();
						Device->Render.WorldViewProjection = Device->Render.World * System->GetScene()->View.ViewProjection;
						Device->UpdateBuffer(Graphics::RenderBufferType_Render);
						Device->DrawIndexed(Mesh);
					}
				}

				Device->FlushTexture2D(1, 7);
			}
			void Skin::RenderGeometryVoxels(Rest::Timer* Time, Rest::Pool<Drawable*>* Geometry, RenderOpt Options)
			{
				Graphics::GraphicsDevice* Device = System->GetDevice();
				Device->SetSamplerState(Sampler, 0);
				Device->SetInputLayout(Layout);
				Device->SetShader(Shaders.Voxelize, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel | Graphics::ShaderType_Geometry);
				Lighting::SetVoxelBuffer(System, Shaders.Voxelize, 3);

				Viewer& View = System->GetScene()->View;
				for (auto It = Geometry->Begin(); It != Geometry->End(); ++It)
				{
					Engine::Components::Skin* Base = (Engine::Components::Skin*)*It;
					if (!Base->Static)
						continue;

					auto* Drawable = Base->GetDrawable();
					if (!Drawable || !Base->IsNear(View))
						continue;

					Device->Animation.HasAnimation = !Base->GetDrawable()->Joints.empty();
					if (Device->Animation.HasAnimation > 0)
						memcpy(Device->Animation.Offsets, Base->Skeleton.Transform, 96 * sizeof(Compute::Matrix4x4));

					Device->UpdateBuffer(Graphics::RenderBufferType_Animation);
					for (auto&& Mesh : Drawable->Meshes)
					{
						auto* Surface = Base->GetSurface(Mesh);
						if (!Surface || !Surface->FillVoxels(Device))
							continue;

						Device->Render.WorldViewProjection = Device->Render.World = Mesh->World * Base->GetEntity()->Transform->GetWorld();
						Device->UpdateBuffer(Graphics::RenderBufferType_Render);
						Device->DrawIndexed(Mesh);
					}
				}

				Device->FlushTexture2D(4, 6);
				Device->SetShader(nullptr, Graphics::ShaderType_Geometry);
			}
			void Skin::RenderDepthLinear(Rest::Timer* Time, Rest::Pool<Drawable*>* Geometry)
			{
				Graphics::GraphicsDevice* Device = System->GetDevice();
				Device->SetSamplerState(Sampler, 0);
				Device->SetDepthStencilState(DepthStencil);
				Device->SetBlendState(Blend);
				Device->SetRasterizerState(FrontRasterizer);
				Device->SetInputLayout(Layout);
				Device->SetShader(Shaders.Depth.Linear, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);

				for (auto It = Geometry->Begin(); It != Geometry->End(); ++It)
				{
					Engine::Components::Skin* Base = (Engine::Components::Skin*)*It;
					auto* Drawable = Base->GetDrawable();

					if (!Drawable || !System->PassDrawable(Base, CullResult_Always, nullptr))
						continue;

					Device->Animation.HasAnimation = !Base->GetDrawable()->Joints.empty();
					if (Device->Animation.HasAnimation > 0)
						memcpy(Device->Animation.Offsets, Base->Skeleton.Transform, 96 * sizeof(Compute::Matrix4x4));

					Device->UpdateBuffer(Graphics::RenderBufferType_Animation);
					for (auto&& Mesh : Drawable->Meshes)
					{
						auto* Surface = Base->GetSurface(Mesh);
						if (!Surface || !Surface->FillDepthLinear(Device))
							continue;

						Device->Render.World = Mesh->World * Base->GetEntity()->Transform->GetWorld();
						Device->Render.WorldViewProjection = Device->Render.World * System->GetScene()->View.ViewProjection;
						Device->UpdateBuffer(Graphics::RenderBufferType_Render);
						Device->DrawIndexed(Mesh);
					}
				}

				Device->SetTexture2D(nullptr, 1);
			}
			void Skin::RenderDepthCubic(Rest::Timer* Time, Rest::Pool<Drawable*>* Geometry, Compute::Matrix4x4* ViewProjection)
			{
				Graphics::GraphicsDevice* Device = System->GetDevice();
				Device->SetSamplerState(Sampler, 0);
				Device->SetDepthStencilState(DepthStencil);
				Device->SetBlendState(Blend);
				Device->SetRasterizerState(FrontRasterizer);
				Device->SetInputLayout(Layout);
				Device->SetShader(Shaders.Depth.Linear, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel | Graphics::ShaderType_Geometry);
				Device->SetBuffer(Shaders.Depth.Cubic, 3, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel | Graphics::ShaderType_Geometry);
				Device->UpdateBuffer(Shaders.Depth.Cubic, ViewProjection);

				for (auto It = Geometry->Begin(); It != Geometry->End(); ++It)
				{
					Engine::Components::Skin* Base = (Engine::Components::Skin*)*It;
					auto* Drawable = Base->GetDrawable();

					if (!Drawable || !Base->IsNear(System->GetScene()->View))
						continue;

					Device->Animation.HasAnimation = !Base->GetDrawable()->Joints.empty();
					if (Device->Animation.HasAnimation > 0)
						memcpy(Device->Animation.Offsets, Base->Skeleton.Transform, 96 * sizeof(Compute::Matrix4x4));

					Device->UpdateBuffer(Graphics::RenderBufferType_Animation);
					for (auto&& Mesh : Drawable->Meshes)
					{
						auto* Surface = Base->GetSurface(Mesh);
						if (!Surface || !Surface->FillDepthCubic(Device))
							continue;

						Device->Render.World = Mesh->World * Base->GetEntity()->Transform->GetWorld();
						Device->UpdateBuffer(Graphics::RenderBufferType_Render);
						Device->DrawIndexed(Mesh);
					}
				}

				Device->SetTexture2D(nullptr, 1);
				Device->SetShader(nullptr, Graphics::ShaderType_Geometry);
			}

			SoftBody::SoftBody(Engine::RenderSystem* Lab) : GeometryDraw(Lab, Components::SoftBody::GetTypeId())
			{
				DepthStencil = Lab->GetDevice()->GetDepthStencilState("less");
				Rasterizer = Lab->GetDevice()->GetRasterizerState("cull-none");
				Blend = Lab->GetDevice()->GetBlendState("overwrite");
				Sampler = Lab->GetDevice()->GetSamplerState("trilinear-x16");
				Layout = Lab->GetDevice()->GetInputLayout("vertex");

				Graphics::ElementBuffer::Desc F = Graphics::ElementBuffer::Desc();
				F.AccessFlags = Graphics::CPUAccess_Write;
				F.Usage = Graphics::ResourceUsage_Dynamic;
				F.BindFlags = Graphics::ResourceBind_Vertex_Buffer;
				F.ElementWidth = sizeof(Compute::Vertex);
				F.ElementCount = 16384;

				VertexBuffer = Lab->GetDevice()->CreateElementBuffer(F);

				F = Graphics::ElementBuffer::Desc();
				F.AccessFlags = Graphics::CPUAccess_Write;
				F.Usage = Graphics::ResourceUsage_Dynamic;
				F.BindFlags = Graphics::ResourceBind_Index_Buffer;
				F.ElementWidth = sizeof(int);
				F.ElementCount = 49152;

				IndexBuffer = Lab->GetDevice()->CreateElementBuffer(F);

				Graphics::Shader::Desc I = Graphics::Shader::Desc();
				if (System->GetDevice()->GetSection("shaders/model/geometry", &I.Data))
					Shaders.Geometry = System->CompileShader("mr-geometry", I, 0);

				if (System->GetDevice()->GetSection("shaders/model/voxelize", &I.Data))
					Shaders.Voxelize = System->CompileShader("mr-voxelize", I, sizeof(Lighting::VoxelBuffer));

				if (System->GetDevice()->GetSection("shaders/model/occlusion", &I.Data))
					Shaders.Occlusion = System->CompileShader("mr-occlusion", I, 0);

				if (System->GetDevice()->GetSection("shaders/model/depth/linear", &I.Data))
					Shaders.Depth.Linear = System->CompileShader("mr-depth-linear", I, 0);

				if (System->GetDevice()->GetSection("shaders/model/depth/cubic", &I.Data))
					Shaders.Depth.Cubic = System->CompileShader("mr-depth-cubic", I, sizeof(Compute::Matrix4x4) * 6);
			}
			SoftBody::~SoftBody()
			{
				System->FreeShader("mr-geometry", Shaders.Geometry);
				System->FreeShader("mr-voxelize", Shaders.Voxelize);
				System->FreeShader("mr-occlusion", Shaders.Occlusion);
				System->FreeShader("mr-depth-linear", Shaders.Depth.Linear);
				System->FreeShader("mr-depth-cubic", Shaders.Depth.Cubic);
				TH_RELEASE(VertexBuffer);
				TH_RELEASE(IndexBuffer);
			}
			void SoftBody::Activate()
			{
				System->AddCull<Engine::Components::SoftBody>();
			}
			void SoftBody::Deactivate()
			{
				System->RemoveCull<Engine::Components::SoftBody>();
			}
			void SoftBody::CullGeometry(const Viewer& View, Rest::Pool<Drawable*>* Geometry)
			{
				Graphics::GraphicsDevice* Device = System->GetDevice();
				Device->SetRasterizerState(Rasterizer);
				Device->SetInputLayout(Layout);
				Device->SetShader(nullptr, Graphics::ShaderType_Pixel);
				Device->SetShader(Shaders.Occlusion, Graphics::ShaderType_Vertex);

				for (auto It = Geometry->Begin(); It != Geometry->End(); It++)
				{
					Components::SoftBody* Base = (Components::SoftBody*)*It;
					if (!System->PassCullable(Base, CullResult_Last, nullptr))
						continue;

					if (Base->GetIndices().empty())
						continue;

					if (!Base->Begin(Device))
						continue;

					if (Base->GetFragmentsCount() > 0)
					{
						Base->Fill(Device, IndexBuffer, VertexBuffer);
						Device->Render.World.Identify();
						Device->Render.WorldViewProjection = Device->Render.World * View.ViewProjection;
						Device->UpdateBuffer(Graphics::RenderBufferType_Render);
						Device->SetVertexBuffer(VertexBuffer, 0);
						Device->SetIndexBuffer(IndexBuffer, Graphics::Format_R32_Uint);
						Device->DrawIndexed(System->GetBoxIBuffer()->GetElements(), 0, 0);
					}
					else
					{
						Device->Render.World = Base->GetEntity()->Transform->GetWorld();
						Device->Render.WorldViewProjection = Device->Render.World * View.ViewProjection;
						Device->UpdateBuffer(Graphics::RenderBufferType_Render);
						Device->SetIndexBuffer(System->GetBoxIBuffer(), Graphics::Format_R32_Uint);
						Device->SetVertexBuffer(System->GetBoxVBuffer(), 0);
						Device->DrawIndexed((unsigned int)Base->GetIndices().size(), 0, 0);
					}
					Base->End(Device);
				}
			}
			void SoftBody::RenderGeometryResult(Rest::Timer* Time, Rest::Pool<Drawable*>* Geometry, RenderOpt Options)
			{
				Graphics::GraphicsDevice* Device = System->GetDevice();
				CullResult Cull = (Options & RenderOpt_Inner ? CullResult_Always : CullResult_Last);
				bool Static = (Options & RenderOpt_Static);

				Device->SetSamplerState(Sampler, 0);
				Device->SetDepthStencilState(DepthStencil);
				Device->SetBlendState(Blend);
				Device->SetRasterizerState(Rasterizer);
				Device->SetInputLayout(Layout);
				Device->SetShader(Shaders.Geometry, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);
				System->GetScene()->SetSurface();

				for (auto It = Geometry->Begin(); It != Geometry->End(); ++It)
				{
					Engine::Components::SoftBody* Base = (Engine::Components::SoftBody*)*It;
					if ((Static && !Base->Static) || Base->GetIndices().empty())
						continue;

					if (!System->PassDrawable(Base, Cull, nullptr))
						continue;

					auto* Surface = Base->GetSurface();
					if (!Surface || !Surface->FillGeometry(Device))
						continue;

					Base->Fill(Device, IndexBuffer, VertexBuffer);

					Device->Render.World.Identify();
					Device->Render.WorldViewProjection = System->GetScene()->View.ViewProjection;
					Device->UpdateBuffer(Graphics::RenderBufferType_Render);
					Device->SetVertexBuffer(VertexBuffer, 0);
					Device->SetIndexBuffer(IndexBuffer, Graphics::Format_R32_Uint);
					Device->DrawIndexed((unsigned int)Base->GetIndices().size(), 0, 0);
				}

				Device->FlushTexture2D(1, 7);
			}
			void SoftBody::RenderGeometryVoxels(Rest::Timer* Time, Rest::Pool<Drawable*>* Geometry, RenderOpt Options)
			{
				Graphics::GraphicsDevice* Device = System->GetDevice();
				Device->SetSamplerState(Sampler, 0);
				Device->SetInputLayout(Layout);
				Device->SetShader(Shaders.Voxelize, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel | Graphics::ShaderType_Geometry);
				Lighting::SetVoxelBuffer(System, Shaders.Voxelize, 3);

				Viewer& View = System->GetScene()->View;
				for (auto It = Geometry->Begin(); It != Geometry->End(); ++It)
				{
					Engine::Components::SoftBody* Base = (Engine::Components::SoftBody*)*It;
					if (!Base->Static || Base->GetIndices().empty())
						continue;

					if (!Base->IsNear(View))
						continue;

					auto* Surface = Base->GetSurface();
					if (!Surface || !Surface->FillVoxels(Device))
						continue;

					Base->Fill(Device, IndexBuffer, VertexBuffer);

					Device->Render.World.Identify();
					Device->Render.WorldViewProjection.Identify();
					Device->UpdateBuffer(Graphics::RenderBufferType_Render);
					Device->SetVertexBuffer(VertexBuffer, 0);
					Device->SetIndexBuffer(IndexBuffer, Graphics::Format_R32_Uint);
					Device->DrawIndexed((unsigned int)Base->GetIndices().size(), 0, 0);
				}

				Device->FlushTexture2D(4, 6);
				Device->SetShader(nullptr, Graphics::ShaderType_Geometry);
			}
			void SoftBody::RenderDepthLinear(Rest::Timer* Time, Rest::Pool<Drawable*>* Geometry)
			{
				Graphics::GraphicsDevice* Device = System->GetDevice();
				Device->SetSamplerState(Sampler, 0);
				Device->SetDepthStencilState(DepthStencil);
				Device->SetBlendState(Blend);
				Device->SetRasterizerState(Rasterizer);
				Device->SetInputLayout(Layout);
				Device->SetShader(Shaders.Depth.Linear, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);

				for (auto It = Geometry->Begin(); It != Geometry->End(); ++It)
				{
					Engine::Components::SoftBody* Base = (Engine::Components::SoftBody*)*It;
					if (Base->GetIndices().empty())
						continue;

					if (!System->PassDrawable(Base, CullResult_Always, nullptr))
						continue;

					auto* Surface = Base->GetSurface();
					if (!Surface || !Surface->FillDepthLinear(Device))
						continue;

					Base->Fill(Device, IndexBuffer, VertexBuffer);

					Device->Render.World.Identify();
					Device->Render.WorldViewProjection = System->GetScene()->View.ViewProjection;
					Device->UpdateBuffer(Graphics::RenderBufferType_Render);
					Device->SetVertexBuffer(VertexBuffer, 0);
					Device->SetIndexBuffer(IndexBuffer, Graphics::Format_R32_Uint);
					Device->DrawIndexed((unsigned int)Base->GetIndices().size(), 0, 0);
				}

				Device->SetTexture2D(nullptr, 1);
			}
			void SoftBody::RenderDepthCubic(Rest::Timer* Time, Rest::Pool<Drawable*>* Geometry, Compute::Matrix4x4* ViewProjection)
			{
				Graphics::GraphicsDevice* Device = System->GetDevice();
				Device->SetSamplerState(Sampler, 0);
				Device->SetDepthStencilState(DepthStencil);
				Device->SetBlendState(Blend);
				Device->SetRasterizerState(Rasterizer);
				Device->SetInputLayout(Layout);
				Device->SetShader(Shaders.Depth.Linear, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel | Graphics::ShaderType_Geometry);
				Device->SetBuffer(Shaders.Depth.Cubic, 3, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel | Graphics::ShaderType_Geometry);
				Device->UpdateBuffer(Shaders.Depth.Cubic, ViewProjection);

				for (auto It = Geometry->Begin(); It != Geometry->End(); ++It)
				{
					Engine::Components::SoftBody* Base = (Engine::Components::SoftBody*)*It;
					if (!Base->GetBody() || Base->GetEntity()->Transform->Position.Distance(System->GetScene()->View.WorldPosition) >= System->GetScene()->View.FarPlane + Base->GetEntity()->Transform->Scale.Length())
						continue;

					auto* Surface = Base->GetSurface();
					if (!Surface || !Surface->FillDepthCubic(Device))
						continue;

					Base->Fill(Device, IndexBuffer, VertexBuffer);

					Device->Render.World.Identify();
					Device->UpdateBuffer(Graphics::RenderBufferType_Render);
					Device->SetVertexBuffer(VertexBuffer, 0);
					Device->SetIndexBuffer(IndexBuffer, Graphics::Format_R32_Uint);
					Device->DrawIndexed((unsigned int)Base->GetIndices().size(), 0, 0);
				}

				Device->SetTexture2D(nullptr, 1);
				Device->SetShader(nullptr, Graphics::ShaderType_Geometry);
			}

			Emitter::Emitter(RenderSystem* Lab) : GeometryDraw(Lab, Components::Emitter::GetTypeId())
			{
				DepthStencilOpaque = Lab->GetDevice()->GetDepthStencilState("less");
				DepthStencilLimpid = Lab->GetDevice()->GetDepthStencilState("less-none");
				BackRasterizer = Lab->GetDevice()->GetRasterizerState("cull-back");
				FrontRasterizer = Lab->GetDevice()->GetRasterizerState("cull-front");
				AdditiveBlend = Lab->GetDevice()->GetBlendState("additive-alpha");
				OverwriteBlend = Lab->GetDevice()->GetBlendState("overwrite");
				Sampler = Lab->GetDevice()->GetSamplerState("trilinear-x16");

				Graphics::Shader::Desc I = Graphics::Shader::Desc();
				if (System->GetDevice()->GetSection("shaders/emitter/geometry/opaque", &I.Data))
					Shaders.Opaque = System->CompileShader("er-opaque", I, 0);

				if (System->GetDevice()->GetSection("shaders/emitter/geometry/transparency", &I.Data))
					Shaders.Transparency = System->CompileShader("er-transparency", I, 0);

				if (System->GetDevice()->GetSection("shaders/emitter/depth/linear", &I.Data))
					Shaders.Depth.Linear = System->CompileShader("er-depth-linear", I, 0);

				if (System->GetDevice()->GetSection("shaders/emitter/depth/point", &I.Data))
					Shaders.Depth.Point = System->CompileShader("er-depth-point", I, 0);

				if (System->GetDevice()->GetSection("shaders/emitter/depth/quad", &I.Data))
					Shaders.Depth.Quad = System->CompileShader("er-depth-quad", I, sizeof(Depth));
			}
			Emitter::~Emitter()
			{
				System->FreeShader("er-opaque", Shaders.Opaque);
				System->FreeShader("er-transparency", Shaders.Transparency);
				System->FreeShader("er-depth-linear", Shaders.Depth.Linear);
				System->FreeShader("er-depth-point", Shaders.Depth.Point);
				System->FreeShader("er-depth-quad", Shaders.Depth.Quad);
			}
			void Emitter::Activate()
			{
				Opaque = System->AddCull<Engine::Components::Emitter>();
			}
			void Emitter::Deactivate()
			{
				Opaque = System->RemoveCull<Engine::Components::Emitter>();
			}
			void Emitter::RenderGeometryResult(Rest::Timer* Time, Rest::Pool<Drawable*>* Geometry, RenderOpt Options)
			{
				if (Options & RenderOpt_Transparent || !Opaque || !Limpid)
					return;

				Graphics::GraphicsDevice* Device = System->GetDevice();
				Graphics::PrimitiveTopology T = Device->GetPrimitiveTopology();
				Compute::Matrix4x4& View = System->GetScene()->View.View;
				CullResult Cull = (Options & RenderOpt_Inner ? CullResult_Always : CullResult_Last);
				bool Static = (Options & RenderOpt_Static);

				Device->SetSamplerState(Sampler, 0);
				Device->SetDepthStencilState(DepthStencilOpaque);
				Device->SetBlendState(OverwriteBlend);
				Device->SetRasterizerState(BackRasterizer);
				Device->SetInputLayout(Layout);
				Device->SetPrimitiveTopology(Graphics::PrimitiveTopology_Point_List);
				Device->SetShader(Shaders.Opaque, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);
				Device->SetVertexBuffer(nullptr, 0);
				System->GetScene()->SetSurface();

				for (auto It = Opaque->Begin(); It != Opaque->End(); ++It)
				{
					Engine::Components::Emitter* Base = (Engine::Components::Emitter*)*It;
					if ((Static && !Base->Static) && !Base->GetBuffer())
						continue;

					if (!System->PassDrawable(Base, Cull, nullptr))
						continue;

					auto* Surface = Base->GetSurface();
					if (!Surface || !Surface->FillGeometry(Device))
						continue;

					Device->Render.World = System->GetScene()->View.Projection;
					Device->Render.WorldViewProjection = (Base->QuadBased ? View : System->GetScene()->View.ViewProjection);
					
					if (Base->Connected)
						Device->Render.WorldViewProjection = Base->GetEntity()->Transform->GetWorld() * Device->Render.WorldViewProjection;

					Device->UpdateBuffer(Base->GetBuffer());
					Device->SetBuffer(Base->GetBuffer(), 8);
					Device->SetShader(Base->QuadBased ? Shaders.Opaque : nullptr, Graphics::ShaderType_Geometry);
					Device->UpdateBuffer(Graphics::RenderBufferType_Render);
					Device->Draw((unsigned int)Base->GetBuffer()->GetArray()->Size(), 0);
				}

				Device->SetDepthStencilState(DepthStencilLimpid);
				Device->SetBlendState(AdditiveBlend);
				Device->SetShader(Shaders.Transparency, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);
				
				for (auto It = Limpid->Begin(); It != Limpid->End(); ++It)
				{
					Engine::Components::Emitter* Base = (Engine::Components::Emitter*)*It;
					if ((Static && !Base->Static) && !Base->GetBuffer())
						continue;

					if (!System->PassDrawable(Base, Cull, nullptr))
						continue;

					auto* Surface = Base->GetSurface();
					if (!Surface || !Surface->FillGeometry(Device))
						continue;

					Device->Render.World = System->GetScene()->View.Projection;
					Device->Render.WorldViewProjection = (Base->QuadBased ? View : System->GetScene()->View.ViewProjection);

					if (Base->Connected)
						Device->Render.WorldViewProjection = Base->GetEntity()->Transform->GetWorld() * Device->Render.WorldViewProjection;

					Device->UpdateBuffer(Base->GetBuffer());
					Device->SetBuffer(Base->GetBuffer(), 8);
					Device->SetShader(Base->QuadBased ? Shaders.Transparency : nullptr, Graphics::ShaderType_Geometry);
					Device->UpdateBuffer(Graphics::RenderBufferType_Render);
					Device->Draw((unsigned int)Base->GetBuffer()->GetArray()->Size(), 0);
				}

				Device->FlushTexture2D(1, 7);
				Device->SetPrimitiveTopology(T);
				Device->SetShader(nullptr, Graphics::ShaderType_Geometry);
			}
			void Emitter::RenderGeometryVoxels(Rest::Timer* Time, Rest::Pool<Drawable*>* Geometry, RenderOpt Options)
			{
			}
			void Emitter::RenderDepthLinear(Rest::Timer* Time, Rest::Pool<Drawable*>* Geometry)
			{
				Graphics::GraphicsDevice* Device = System->GetDevice();
				Graphics::PrimitiveTopology T = Device->GetPrimitiveTopology();
				Compute::Matrix4x4& View = System->GetScene()->View.View;

				Device->SetSamplerState(Sampler, 0);
				Device->SetDepthStencilState(DepthStencilLimpid);
				Device->SetBlendState(OverwriteBlend);
				Device->SetRasterizerState(FrontRasterizer);
				Device->SetInputLayout(Layout);
				Device->SetPrimitiveTopology(Graphics::PrimitiveTopology_Point_List);
				Device->SetShader(Shaders.Depth.Linear, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);
				Device->SetVertexBuffer(nullptr, 0);

				for (auto It = Geometry->Begin(); It != Geometry->End(); ++It)
				{
					Engine::Components::Emitter* Base = (Engine::Components::Emitter*)*It;
					if (!Base->GetBuffer())
						continue;

					auto* Surface = Base->GetSurface();
					if (!Surface || !Surface->FillDepthLinear(Device))
						continue;

					Device->Render.World = Base->QuadBased ? System->GetScene()->View.Projection : Compute::Matrix4x4::Identity();
					Device->Render.WorldViewProjection = (Base->QuadBased ? View : System->GetScene()->View.ViewProjection);
					
					if (Base->Connected)
						Device->Render.WorldViewProjection = Base->GetEntity()->Transform->GetWorld() * Device->Render.WorldViewProjection;

					Device->SetBuffer(Base->GetBuffer(), 8);
					Device->SetShader(Base->QuadBased ? Shaders.Depth.Linear : nullptr, Graphics::ShaderType_Geometry);
					Device->UpdateBuffer(Graphics::RenderBufferType_Render);
					Device->Draw((unsigned int)Base->GetBuffer()->GetArray()->Size(), 0);
				}

				Device->SetTexture2D(nullptr, 1);
				Device->SetPrimitiveTopology(T);
				Device->SetShader(nullptr, Graphics::ShaderType_Geometry);
			}
			void Emitter::RenderDepthCubic(Rest::Timer* Time, Rest::Pool<Drawable*>* Geometry, Compute::Matrix4x4* ViewProjection)
			{
				Graphics::GraphicsDevice* Device = System->GetDevice();
				Depth.FaceView[0] = Compute::Matrix4x4::CreateCubeMapLookAt(0, System->GetScene()->View.InvViewPosition);
				Depth.FaceView[1] = Compute::Matrix4x4::CreateCubeMapLookAt(1, System->GetScene()->View.InvViewPosition);
				Depth.FaceView[2] = Compute::Matrix4x4::CreateCubeMapLookAt(2, System->GetScene()->View.InvViewPosition);
				Depth.FaceView[3] = Compute::Matrix4x4::CreateCubeMapLookAt(3, System->GetScene()->View.InvViewPosition);
				Depth.FaceView[4] = Compute::Matrix4x4::CreateCubeMapLookAt(4, System->GetScene()->View.InvViewPosition);
				Depth.FaceView[5] = Compute::Matrix4x4::CreateCubeMapLookAt(5, System->GetScene()->View.InvViewPosition);

				Graphics::PrimitiveTopology T = Device->GetPrimitiveTopology();
				Device->SetSamplerState(Sampler, 0);
				Device->SetDepthStencilState(DepthStencilLimpid);
				Device->SetBlendState(OverwriteBlend);
				Device->SetRasterizerState(FrontRasterizer);
				Device->SetInputLayout(Layout);
				Device->SetPrimitiveTopology(Graphics::PrimitiveTopology_Point_List);
				Device->SetVertexBuffer(nullptr, 0);
				Device->SetBuffer(Shaders.Depth.Quad, 3, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel | Graphics::ShaderType_Geometry);
				Device->UpdateBuffer(Shaders.Depth.Quad, &Depth);

				for (auto It = Geometry->Begin(); It != Geometry->End(); ++It)
				{
					Engine::Components::Emitter* Base = (Engine::Components::Emitter*)*It;
					if (!Base->GetBuffer())
						continue;

					auto* Surface = Base->GetSurface();
					if (!Surface || !Surface->FillDepthCubic(Device))
						continue;

					Device->Render.World = (Base->Connected ? Base->GetEntity()->Transform->GetWorld() : Compute::Matrix4x4::Identity());			
					Device->SetBuffer(Base->GetBuffer(), 8);
					Device->SetShader(Base->QuadBased ? Shaders.Depth.Quad : Shaders.Depth.Point, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel | Graphics::ShaderType_Geometry);
					Device->UpdateBuffer(Graphics::RenderBufferType_Render);
					Device->Draw((unsigned int)Base->GetBuffer()->GetArray()->Size(), 0);
				}

				Device->SetTexture2D(nullptr, 1);
				Device->SetPrimitiveTopology(T);
				Device->SetShader(nullptr, Graphics::ShaderType_Geometry);
			}

			Decal::Decal(RenderSystem* Lab) : GeometryDraw(Lab, Components::Decal::GetTypeId())
			{
				DepthStencil = Lab->GetDevice()->GetDepthStencilState("none");
				Rasterizer = Lab->GetDevice()->GetRasterizerState("cull-back");
				Blend = Lab->GetDevice()->GetBlendState("additive");
				Sampler = Lab->GetDevice()->GetSamplerState("trilinear-x16");
				Layout = Lab->GetDevice()->GetInputLayout("shape-vertex");

				Graphics::Shader::Desc I = Graphics::Shader::Desc();
				if (System->GetDevice()->GetSection("shaders/decal/geometry", &I.Data))
					Shader = System->CompileShader("dr-geometry", I, sizeof(RenderPass));
			}
			Decal::~Decal()
			{
				System->FreeShader("dr-geometry", Shader);
			}
			void Decal::Activate()
			{
				System->AddCull<Engine::Components::Decal>();
			}
			void Decal::Deactivate()
			{
				System->RemoveCull<Engine::Components::Decal>();
			}
			void Decal::RenderGeometryResult(Rest::Timer* Time, Rest::Pool<Drawable*>* Geometry, RenderOpt Options)
			{
				Graphics::GraphicsDevice* Device = System->GetDevice();
				Rest::Pool<Component*>* Array = (Options & RenderOpt_Transparent ? Limpid : Opaque);
				CullResult Cull = (Options & RenderOpt_Inner ? CullResult_Always : CullResult_Last);
				bool Map[8] = { true, true, false, true, false, false, false, false };
				bool Static = (Options & RenderOpt_Static);

				if (!Array || Geometry->Empty())
					return;

				Device->SetSamplerState(Sampler, 0);
				Device->SetDepthStencilState(DepthStencil);
				Device->SetBlendState(Blend);
				Device->SetRasterizerState(Rasterizer);
				Device->SetInputLayout(Layout);
				Device->SetShader(Shader, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);
				Device->SetBuffer(Shader, 3, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);
				Device->SetIndexBuffer(System->GetSphereIBuffer(), Graphics::Format_R32_Uint);
				Device->SetVertexBuffer(System->GetSphereVBuffer(), 0);
				Device->SetTargetMap(System->GetScene()->GetSurface(), Map);
				Device->SetTexture2D(System->GetScene()->GetSurface()->GetTarget(2), 8);

				for (auto It = Geometry->Begin(); It != Geometry->End(); ++It)
				{
					Engine::Components::Decal* Base = (Engine::Components::Decal*)*It;
					if ((Static && !Base->Static) || !System->PassDrawable(Base, Cull, nullptr))
						continue;

					auto* Surface = Base->GetSurface();
					if (!Surface || !Surface->FillGeometry(Device))
						continue;

					RenderPass.ViewProjection = Base->View * Base->Projection;
					Device->Render.World = Compute::Matrix4x4::CreateScale(Base->GetRange()) * Compute::Matrix4x4::CreateTranslation(Base->GetEntity()->Transform->Position);
					Device->Render.WorldViewProjection = Device->Render.World * System->GetScene()->View.ViewProjection;
					Device->UpdateBuffer(Graphics::RenderBufferType_Render);
					Device->UpdateBuffer(Shader, &RenderPass);
					Device->DrawIndexed((unsigned int)System->GetSphereIBuffer()->GetElements(), 0, 0);
				}

				Device->FlushTexture2D(1, 8);
			}
			void Decal::RenderGeometryVoxels(Rest::Timer* Time, Rest::Pool<Drawable*>* Geometry, RenderOpt Options)
			{
			}
			void Decal::RenderDepthLinear(Rest::Timer* Time, Rest::Pool<Drawable*>* Geometry)
			{
			}
			void Decal::RenderDepthCubic(Rest::Timer* Time, Rest::Pool<Drawable*>* Geometry, Compute::Matrix4x4* ViewProjection)
			{
			}

			Lighting::Lighting(RenderSystem* Lab) : Renderer(Lab)
			{
				Surfaces.Size = 128;
				SurfaceLight.MipLevels = 0;
				IndirectLight.Enabled = false;
				IndirectLight.Distance = 10.0f;
				IndirectLight.Size = 128;
				IndirectLight.Tick.Delay = 15;
				Shadows.Tick.Delay = 5;
				Shadows.Distance = 0.5f;

				DepthStencilNone = Lab->GetDevice()->GetDepthStencilState("none");
				DepthStencilGreater = Lab->GetDevice()->GetDepthStencilState("greater-read-only");
				DepthStencilLess = Lab->GetDevice()->GetDepthStencilState("less-read-only");
				FrontRasterizer = Lab->GetDevice()->GetRasterizerState("cull-front");
				BackRasterizer = Lab->GetDevice()->GetRasterizerState("cull-back");
				NoneRasterizer = Lab->GetDevice()->GetRasterizerState("cull-none");
				BlendAdditive = Lab->GetDevice()->GetBlendState("additive-opaque");
				BlendOverwrite = Lab->GetDevice()->GetBlendState("overwrite-colorless");
				ShadowSampler = Lab->GetDevice()->GetSamplerState("shadow");
				WrapSampler = Lab->GetDevice()->GetSamplerState("trilinear-x16");
				Layout = Lab->GetDevice()->GetInputLayout("shape-vertex");

				Graphics::Shader::Desc I = Graphics::Shader::Desc();
				if (System->GetDevice()->GetSection("shaders/lighting/ambient/direct", &I.Data))
					Shaders.Ambient[0] = System->CompileShader("lr-ambient-direct", I, sizeof(AmbientLight));

				if (System->GetDevice()->GetSection("shaders/lighting/ambient/indirect", &I.Data))
					Shaders.Ambient[1] = System->CompileShader("lr-ambient-indirect", I, sizeof(Voxelizer));

				if (System->GetDevice()->GetSection("shaders/lighting/point/low", &I.Data))
					Shaders.Point[0] = System->CompileShader("lr-point-low", I, sizeof(PointLight));

				if (System->GetDevice()->GetSection("shaders/lighting/point/high", &I.Data))
					Shaders.Point[1] = System->CompileShader("lr-point-high", I, 0);

				if (System->GetDevice()->GetSection("shaders/lighting/point/indirect", &I.Data))
					Shaders.Point[2] = System->CompileShader("lr-point-indirect", I, 0);

				if (System->GetDevice()->GetSection("shaders/lighting/spot/low", &I.Data))
					Shaders.Spot[0] = System->CompileShader("lr-spot-low", I, sizeof(SpotLight));

				if (System->GetDevice()->GetSection("shaders/lighting/spot/high", &I.Data))
					Shaders.Spot[1] = System->CompileShader("lr-spot-high", I, 0);

				if (System->GetDevice()->GetSection("shaders/lighting/spot/indirect", &I.Data))
					Shaders.Spot[2] = System->CompileShader("lr-spot-indirect", I, 0);

				if (System->GetDevice()->GetSection("shaders/lighting/line/low", &I.Data))
					Shaders.Line[0] = System->CompileShader("lr-line-low", I, sizeof(LineLight));

				if (System->GetDevice()->GetSection("shaders/lighting/line/high", &I.Data))
					Shaders.Line[1] = System->CompileShader("lr-line-high", I, 0);

				if (System->GetDevice()->GetSection("shaders/lighting/line/indirect", &I.Data))
					Shaders.Line[2] = System->CompileShader("lr-line-indirect", I, 0);

				if (System->GetDevice()->GetSection("shaders/lighting/surface", &I.Data))
					Shaders.Surface = System->CompileShader("lr-surface", I, sizeof(SurfaceLight));
			}
			Lighting::~Lighting()
			{
				FlushDepthBuffersAndCache();
				System->FreeShader("lr-ambient-direct", Shaders.Ambient[0]);
				System->FreeShader("lr-ambient-indirect", Shaders.Ambient[1]);
				System->FreeShader("lr-point-low", Shaders.Point[0]);
				System->FreeShader("lr-point-high", Shaders.Point[1]);
				System->FreeShader("lr-point-indirect", Shaders.Point[2]);
				System->FreeShader("lr-spot-low", Shaders.Spot[0]);
				System->FreeShader("lr-spot-high", Shaders.Spot[1]);
				System->FreeShader("lr-spot-indirect", Shaders.Spot[2]);
				System->FreeShader("lr-line-low", Shaders.Line[0]);
				System->FreeShader("lr-line-high", Shaders.Line[1]);
				System->FreeShader("lr-line-indirect", Shaders.Line[2]);
				System->FreeShader("lr-surface", Shaders.Surface);
				TH_RELEASE(DiffuseBuffer);
				TH_RELEASE(NormalBuffer);
				TH_RELEASE(SurfaceBuffer);
				TH_RELEASE(Surface);
				TH_RELEASE(Subresource);
				TH_RELEASE(Input1);
				TH_RELEASE(Output1);
				TH_RELEASE(Input2);
				TH_RELEASE(Output2);
				TH_RELEASE(SkyBase);
				TH_RELEASE(SkyMap);
			}
			void Lighting::Deserialize(ContentManager* Content, Rest::Document* Node)
			{
				std::string Path;
				if (NMake::Unpack(Node->Find("sky-map"), &Path))
					SetSkyMap(Content->Load<Graphics::Texture2D>(Path));

				NMake::Unpack(Node->Find("high-emission"), &AmbientLight.HighEmission);
				NMake::Unpack(Node->Find("low-emission"), &AmbientLight.LowEmission);
				NMake::Unpack(Node->Find("sky-emission"), &AmbientLight.SkyEmission);
				NMake::Unpack(Node->Find("light-emission"), &AmbientLight.LightEmission);
				NMake::Unpack(Node->Find("sky-color"), &AmbientLight.SkyColor);
				NMake::Unpack(Node->Find("fog-color"), &AmbientLight.FogColor);
				NMake::Unpack(Node->Find("fog-amount"), &AmbientLight.FogAmount);
				NMake::Unpack(Node->Find("fog-far-off"), &AmbientLight.FogFarOff);
				NMake::Unpack(Node->Find("fog-far"), &AmbientLight.FogFar);
				NMake::Unpack(Node->Find("fog-near-off"), &AmbientLight.FogNearOff);
				NMake::Unpack(Node->Find("fog-near"), &AmbientLight.FogNear);
				NMake::Unpack(Node->Find("recursive"), &AmbientLight.Recursive);
				NMake::Unpack(Node->Find("point-light-resolution"), &Shadows.PointLightResolution);
				NMake::Unpack(Node->Find("point-light-limits"), &Shadows.PointLightLimits);
				NMake::Unpack(Node->Find("spot-light-resolution"), &Shadows.SpotLightResolution);
				NMake::Unpack(Node->Find("spot-light-limits"), &Shadows.SpotLightLimits);
				NMake::Unpack(Node->Find("line-light-resolution"), &Shadows.LineLightResolution);
				NMake::Unpack(Node->Find("line-light-limits"), &Shadows.LineLightLimits);
				NMake::Unpack(Node->Find("shadow-distance"), &Shadows.Distance);
				NMake::Unpack(Node->Find("sf-size"), &Surfaces.Size);
			}
			void Lighting::Serialize(ContentManager* Content, Rest::Document* Node)
			{
				AssetCache* Asset = Content->Find<Graphics::Texture2D>(SkyBase);
				if (Asset != nullptr)
					NMake::Pack(Node->SetDocument("sky-map"), Asset->Path);

				NMake::Pack(Node->SetDocument("high-emission"), AmbientLight.HighEmission);
				NMake::Pack(Node->SetDocument("low-emission"), AmbientLight.LowEmission);
				NMake::Pack(Node->SetDocument("sky-emission"), AmbientLight.SkyEmission);
				NMake::Pack(Node->SetDocument("light-emission"), AmbientLight.LightEmission);
				NMake::Pack(Node->SetDocument("sky-color"), AmbientLight.SkyColor);
				NMake::Pack(Node->SetDocument("fog-color"), AmbientLight.FogColor);
				NMake::Pack(Node->SetDocument("fog-amount"), AmbientLight.FogAmount);
				NMake::Pack(Node->SetDocument("fog-far-off"), AmbientLight.FogFarOff);
				NMake::Pack(Node->SetDocument("fog-far"), AmbientLight.FogFar);
				NMake::Pack(Node->SetDocument("fog-near-off"), AmbientLight.FogNearOff);
				NMake::Pack(Node->SetDocument("fog-near"), AmbientLight.FogNear);
				NMake::Pack(Node->SetDocument("recursive"), AmbientLight.Recursive);
				NMake::Pack(Node->SetDocument("point-light-resolution"), Shadows.PointLightResolution);
				NMake::Pack(Node->SetDocument("point-light-limits"), Shadows.PointLightLimits);
				NMake::Pack(Node->SetDocument("spot-light-resolution"), Shadows.SpotLightResolution);
				NMake::Pack(Node->SetDocument("spot-light-limits"), Shadows.SpotLightLimits);
				NMake::Pack(Node->SetDocument("line-light-resolution"), Shadows.LineLightResolution);
				NMake::Pack(Node->SetDocument("line-light-limits"), Shadows.LineLightLimits);
				NMake::Pack(Node->SetDocument("shadow-distance"), Shadows.Distance);
				NMake::Pack(Node->SetDocument("sf-size"), Surfaces.Size);
			}
			void Lighting::Activate()
			{
				SurfaceLights = System->AddCull<Engine::Components::SurfaceLight>();
				PointLights = System->AddCull<Engine::Components::PointLight>();
				SpotLights = System->AddCull<Engine::Components::SpotLight>();
				LineLights = System->GetScene()->GetComponents<Engine::Components::LineLight>();
			}
			void Lighting::Deactivate()
			{
				SurfaceLights = System->RemoveCull<Engine::Components::SurfaceLight>();
				PointLights = System->RemoveCull<Engine::Components::PointLight>();
				SpotLights = System->RemoveCull<Engine::Components::SpotLight>();
			}
			void Lighting::ResizeBuffers()
			{
				FlushDepthBuffersAndCache();

				Shadows.PointLight.resize(Shadows.PointLightLimits);
				for (auto It = Shadows.PointLight.begin(); It != Shadows.PointLight.end(); It++)
				{
					Graphics::RenderTargetCube::Desc F = Graphics::RenderTargetCube::Desc();
					F.Size = (unsigned int)Shadows.PointLightResolution;
					F.FormatMode = Graphics::Format_R32_Float;

					*It = System->GetDevice()->CreateRenderTargetCube(F);
				}

				Shadows.SpotLight.resize(Shadows.SpotLightLimits);
				for (auto It = Shadows.SpotLight.begin(); It != Shadows.SpotLight.end(); It++)
				{
					Graphics::RenderTarget2D::Desc F = Graphics::RenderTarget2D::Desc();
					F.Width = (unsigned int)Shadows.SpotLightResolution;
					F.Height = (unsigned int)Shadows.SpotLightResolution;
					F.FormatMode = Graphics::Format_R32_Float;

					*It = System->GetDevice()->CreateRenderTarget2D(F);
				}

				Shadows.LineLight.resize(Shadows.LineLightLimits);
				for (auto It = Shadows.LineLight.begin(); It != Shadows.LineLight.end(); It++)
					*It = nullptr;

				Graphics::RenderTarget2D::Desc F;
				System->GetScene()->GetTargetDesc(&F);

				TH_RELEASE(Output1);
				Output1 = System->GetDevice()->CreateRenderTarget2D(F);

				TH_RELEASE(Input1);
				Input1 = System->GetDevice()->CreateRenderTarget2D(F);
			}
			void Lighting::Render(Rest::Timer* Time, RenderState State, RenderOpt Options)
			{
				Graphics::GraphicsDevice* Device = System->GetDevice();
				if (State != RenderState_Geometry_Result)
					return;

				SceneGraph* Scene = System->GetScene();
				if (!(Options & RenderOpt_Inner || Options & RenderOpt_Transparent))
				{
					double ElapsedTime = Time->GetElapsedTime();
					if (Shadows.Tick.TickEvent(ElapsedTime))
						RenderShadowMaps(Device, Scene, Time);
					else
						RenderSurfaceMaps(Device, Scene, Time);

					if (IndirectLight.Enabled && IndirectLight.Tick.TickEvent(ElapsedTime))
						RenderVoxels(Time, Device, Scene->GetSurface());
				}

				if (IndirectLight.Enabled)
					RenderVoxelsBuffers(Device, Options);

				RenderResultBuffers(Device, Options);
			}
			void Lighting::RenderResultBuffers(Graphics::GraphicsDevice* Device, RenderOpt Options)
			{
				Compute::Vector3& Camera = System->GetScene()->View.WorldPosition;
				Graphics::MultiRenderTarget2D* Target = System->GetScene()->GetSurface();
				bool Inner = (Options & RenderOpt_Inner);
				bool Backcull = true;
				float Distance = 0.0f;

				AmbientLight.SkyOffset = System->GetScene()->View.Projection.Invert() * Compute::Matrix4x4::CreateRotation(System->GetScene()->View.WorldRotation);
				Device->SetSamplerState(WrapSampler, 0);
				Device->SetSamplerState(ShadowSampler, 1);
				Device->SetDepthStencilState(DepthStencilLess);
				Device->SetBlendState(BlendAdditive);
				Device->SetRasterizerState(BackRasterizer);
				Device->SetInputLayout(Layout);
				Device->CopyTarget(Target, 0, Inner ? Input2 : Input1, 0);
				Device->SetTarget(Target, 0, 0, 0, 0);
				Device->SetTexture2D(Inner ? Input2->GetTarget(0) : Input1->GetTarget(0), 1);
				Device->SetTexture2D(Target->GetTarget(1), 2);
				Device->SetTexture2D(Target->GetTarget(2), 3);
				Device->SetTexture2D(Target->GetTarget(3), 4);
				Device->SetIndexBuffer(System->GetCubeIBuffer(), Graphics::Format_R32_Uint);
				Device->SetVertexBuffer(System->GetCubeVBuffer(), 0);

				RenderSurfaceLights(Device, Camera, Distance, Backcull, Inner);
				RenderPointLights(Device, Camera, Distance, Backcull, Inner);
				RenderSpotLights(Device, Camera, Distance, Backcull, Inner);
				RenderLineLights(Device, Inner);
				RenderAmbientLight(Device, Target, Inner);

				Device->FlushTexture2D(1, 10);
			}
			void Lighting::RenderVoxelsBuffers(Graphics::GraphicsDevice* Device, RenderOpt Options)
			{
				Graphics::MultiRenderTarget2D* Target = System->GetScene()->GetSurface();
				if (!DiffuseBuffer || !NormalBuffer || !SurfaceBuffer)
					return;

				Device->SetSamplerState(ShadowSampler, 0);
				Device->SetDepthStencilState(DepthStencilNone);
				Device->SetBlendState(BlendAdditive);
				Device->SetRasterizerState(BackRasterizer);
				Device->SetInputLayout(Layout);
				Device->SetTarget(Target, 0);
				Device->SetTexture3D(DiffuseBuffer, 1);
				Device->SetTexture2D(Target->GetTarget(1), 2);
				Device->SetTexture2D(Target->GetTarget(2), 3);
				Device->SetTexture2D(Target->GetTarget(3), 4);
				Device->SetVertexBuffer(System->GetQuadVBuffer(), 0);

				Device->SetShader(Shaders.Ambient[1], Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);
				Device->SetBuffer(Shaders.Ambient[1], 3, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);
				Device->UpdateBuffer(Shaders.Ambient[1], &Voxelizer);
				Device->Draw(6, 0);

				Device->FlushTexture3D(1, 1);
				Device->FlushTexture2D(2, 3);
			}
			void Lighting::RenderShadowMaps(Graphics::GraphicsDevice* Device, SceneGraph* Scene, Rest::Timer* Time)
			{
				uint64_t Counter = 0; float D = 0.0f;
				for (auto It = PointLights->Begin(); It != PointLights->End(); It++)
				{
					Engine::Components::PointLight* Light = (Engine::Components::PointLight*)*It;
					if (Counter >= Shadows.PointLight.size())
						break;

					Light->DepthMap = nullptr;
					if (!Light->Shadow.Enabled)
						continue;

					if (!System->PassCullable(Light, CullResult_Always, &D) || D < Shadows.Distance)
						continue;

					CubicDepthMap* Target = Shadows.PointLight[Counter];
					Device->SetTarget(Target, 0, 0, 0, 0);
					Device->ClearDepth(Target);

					Light->AssembleDepthOrigin();
					Scene->SetView(Compute::Matrix4x4::Identity(), Light->Projection, Light->GetEntity()->Transform->Position, 0.1f, Light->Shadow.Distance, true);
					Scene->Render(Time, RenderState_Depth_Cubic, RenderOpt_Inner);

					Light->DepthMap = Target;
					Counter++;
				}

				Counter = 0;
				for (auto It = SpotLights->Begin(); It != SpotLights->End(); It++)
				{
					Engine::Components::SpotLight* Light = (Engine::Components::SpotLight*)*It;
					if (Counter >= Shadows.SpotLight.size())
						break;

					Light->DepthMap = nullptr;
					if (!Light->Shadow.Enabled)
						continue;

					if (!System->PassCullable(Light, CullResult_Always, &D) || D < Shadows.Distance)
						continue;

					LinearDepthMap* Target = Shadows.SpotLight[Counter];
					Device->SetTarget(Target, 0, 0, 0, 0);
					Device->ClearDepth(Target);

					Light->AssembleDepthOrigin();
					Scene->SetView(Light->View, Light->Projection, Light->GetEntity()->Transform->Position, 0.1f, Light->Shadow.Distance, true);
					Scene->Render(Time, RenderState_Depth_Linear, RenderOpt_Inner);

					Light->DepthMap = Target;
					Counter++;
				}

				Counter = 0;
				for (auto It = LineLights->Begin(); It != LineLights->End(); It++)
				{
					Engine::Components::LineLight* Light = (Engine::Components::LineLight*)*It;
					if (Counter >= Shadows.LineLight.size())
						break;

					Light->DepthMap = nullptr;
					if (!Light->Shadow.Enabled)
						continue;

					CascadedDepthMap*& Target = Shadows.LineLight[Counter];
					if (Light->Shadow.Cascades < 1 || Light->Shadow.Cascades > 6)
						continue;

					if (!Target || Target->size() < Light->Shadow.Cascades)
						GenerateCascadeMap(&Target, Light->Shadow.Cascades);

					Light->AssembleDepthOrigin();
					for (size_t i = 0; i < Target->size(); i++)
					{
						LinearDepthMap* Cascade = (*Target)[i];
						Device->SetTarget(Cascade, 0, 0, 0, 0);
						Device->ClearDepth(Cascade);

						Scene->SetView(Light->View[i], Light->Projection[i], 0.0f, 0.1f, Light->Shadow.Distance[i], true);
						Scene->Render(Time, RenderState_Depth_Linear, RenderOpt_Inner);
					}

					Light->DepthMap = Target;
					Counter++;
				}

				Device->FlushTexture2D(1, 8);
				Scene->RestoreViewBuffer(nullptr);
			}
			void Lighting::RenderSurfaceMaps(Graphics::GraphicsDevice* Device, SceneGraph* Scene, Rest::Timer* Time)
			{
				if (SurfaceLights->Empty())
					return;

				if (!Surface || !Subresource || !Output1 || !Output2)
					SetSurfaceBufferSize(Surfaces.Size);

				Graphics::MultiRenderTarget2D* S = Scene->GetSurface();
				Scene->SwapSurface(Surface);
				Scene->SetSurface();

				double ElapsedTime = Time->GetElapsedTime();
				for (auto It = SurfaceLights->Begin(); It != SurfaceLights->End(); It++)
				{
					Engine::Components::SurfaceLight* Light = (Engine::Components::SurfaceLight*)*It;
					if (Light->IsImageBased() || !System->PassCullable(Light, CullResult_Always, nullptr))
						continue;

					Graphics::TextureCube* Cache = Light->GetProbeCache();
					if (!Cache)
					{
						Cache = Device->CreateTextureCube();
						Light->SetProbeCache(Cache);
					}
					else if (!Light->Tick.TickEvent(ElapsedTime) || Light->Tick.Delay <= 0.0)
						continue;

					Device->CubemapBegin(Subresource);
					Light->Locked = true;

					Compute::Vector3 Position = Light->GetEntity()->Transform->Position * Light->Offset;
					for (unsigned int j = 0; j < 6; j++)
					{
						Light->View[j] = Compute::Matrix4x4::CreateCubeMapLookAt(j, Position);
						Scene->SetView(Light->View[j], Light->Projection, Position, 0.1f, Light->Size.Range, true);
						Scene->ClearSurface();
						Scene->Render(Time, RenderState_Geometry_Result, Light->StaticMask ? RenderOpt_Inner | RenderOpt_Static : RenderOpt_Inner);
						Device->CubemapFace(Subresource, 0, j);
					}

					Light->Locked = false;
					Device->CubemapEnd(Subresource, Cache);
				}

				Scene->SwapSurface(S);
				Scene->RestoreViewBuffer(nullptr);
			}
			void Lighting::RenderVoxels(Rest::Timer* Time, Graphics::GraphicsDevice* Device, Graphics::MultiRenderTarget2D* Surface)
			{
				SceneGraph* Scene = System->GetScene();
				if (!DiffuseBuffer || !NormalBuffer || !SurfaceBuffer)
					SetVoxelBufferSize(IndirectLight.Size);

				Graphics::Texture3D* Buffer[3];
				Buffer[0] = DiffuseBuffer;
				Buffer[1] = NormalBuffer;
				Buffer[2] = SurfaceBuffer;

				Compute::Vector3 Center = Scene->View.WorldPosition.InvertZ();
				if (Voxelizer.GridCenter.Distance(Center) > 0.75 * IndirectLight.Distance.Length() / 3)
					Voxelizer.GridCenter = Center;

				Voxelizer.GridSize = (float)IndirectLight.Size;
				Voxelizer.GridScale = IndirectLight.Distance;
				Scene->View.FarPlane = (IndirectLight.Distance.X + IndirectLight.Distance.Y + IndirectLight.Distance.Z) / 3.0f;

				Device->ClearWritable(DiffuseBuffer);
				Device->ClearWritable(NormalBuffer);
				Device->ClearWritable(SurfaceBuffer);
				Device->SetTargetRect(IndirectLight.Size, IndirectLight.Size);
				Device->SetDepthStencilState(DepthStencilNone);
				Device->SetBlendState(BlendOverwrite);
				Device->SetRasterizerState(NoneRasterizer);
				Device->SetWriteable(Buffer, 3, 1);

				Scene->Render(Time, RenderState_Geometry_Voxels, RenderOpt_Inner);
				Scene->RestoreViewBuffer(nullptr);

				Graphics::Texture3D* Flush[3] = { nullptr };
				Device->SetWriteable(Flush, 3, 1);
				Device->SetTarget(Surface);
				Device->GenerateMips(DiffuseBuffer);
			}
			void Lighting::RenderSurfaceLights(Graphics::GraphicsDevice* Device, Compute::Vector3& Camera, float& Distance, bool& Backcull, const bool& Inner)
			{
				if (!Inner)
				{
					Device->SetShader(Shaders.Surface, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);
					Device->SetBuffer(Shaders.Surface, 3, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);

					for (auto It = SurfaceLights->Begin(); It != SurfaceLights->End(); ++It)
					{
						Engine::Components::SurfaceLight* Light = (Engine::Components::SurfaceLight*)*It;
						if (!Light->GetProbeCache() || !System->PassCullable(Light, CullResult_Always, &Distance))
							continue;

						Compute::Vector3 Position(Light->GetEntity()->Transform->Position), Scale(Light->Size.Range);
						bool Front = Compute::Common::HasPointIntersectedCube(Position, Scale.Mul(1.025), Camera);
						if ((Front && Backcull) || (!Front && !Backcull))
						{
							Device->SetRasterizerState(Front ? FrontRasterizer : BackRasterizer);
							Device->SetDepthStencilState(Front ? DepthStencilGreater : DepthStencilLess);
							Backcull = !Backcull;
						}

						SurfaceLight.WorldViewProjection = Compute::Matrix4x4::CreateTranslatedScale(Position, Scale) * System->GetScene()->View.ViewProjection;
						SurfaceLight.Position = Light->GetEntity()->Transform->Position.InvertZ();
						SurfaceLight.Lighting = Light->Diffuse.Mul(Light->Emission * Distance);
						SurfaceLight.Scale = Light->GetEntity()->Transform->Scale;
						SurfaceLight.Parallax = (Light->Parallax ? 1.0f : 0.0f);
						SurfaceLight.Infinity = Light->Infinity;
						SurfaceLight.Attenuation.X = Light->Size.C1;
						SurfaceLight.Attenuation.Y = Light->Size.C2;
						SurfaceLight.Range = Scale.X;

						Device->SetTextureCube(Light->GetProbeCache(), 5);
						Device->UpdateBuffer(Shaders.Surface, &SurfaceLight);
						Device->DrawIndexed((unsigned int)System->GetCubeIBuffer()->GetElements(), 0, 0);
					}
				}
				else if (AmbientLight.Recursive > 0.0f)
				{
					Device->SetShader(Shaders.Surface, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);
					Device->SetBuffer(Shaders.Surface, 3, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);

					for (auto It = SurfaceLights->Begin(); It != SurfaceLights->End(); ++It)
					{
						Engine::Components::SurfaceLight* Light = (Engine::Components::SurfaceLight*)*It;
						if (Light->Locked || !Light->GetProbeCache() || !System->PassCullable(Light, CullResult_Always, &Distance))
							continue;

						Compute::Vector3 Position(Light->GetEntity()->Transform->Position), Scale(Light->Size.Range);
						bool Front = Compute::Common::HasPointIntersectedCube(Position, Scale.Mul(1.025), Camera);
						if ((Front && Backcull) || (!Front && !Backcull))
						{
							Device->SetRasterizerState(Front ? FrontRasterizer : BackRasterizer);
							Device->SetDepthStencilState(Front ? DepthStencilGreater : DepthStencilLess);
							Backcull = !Backcull;
						}

						SurfaceLight.WorldViewProjection = Compute::Matrix4x4::CreateTranslatedScale(Position, Scale) * System->GetScene()->View.ViewProjection;
						SurfaceLight.Position = Light->GetEntity()->Transform->Position.InvertZ();
						SurfaceLight.Lighting = Light->Diffuse.Mul(Light->Emission * Distance);
						SurfaceLight.Scale = Light->GetEntity()->Transform->Scale;
						SurfaceLight.Parallax = (Light->Parallax ? 1.0f : 0.0f);
						SurfaceLight.Infinity = Light->Infinity;
						SurfaceLight.Attenuation.X = Light->Size.C1;
						SurfaceLight.Attenuation.Y = Light->Size.C2;
						SurfaceLight.Range = Scale.X;

						Device->SetTextureCube(Light->GetProbeCache(), 5);
						Device->UpdateBuffer(Shaders.Surface, &SurfaceLight);
						Device->DrawIndexed((unsigned int)System->GetCubeIBuffer()->GetElements(), 0, 0);
					}
				}
			}
			void Lighting::RenderPointLights(Graphics::GraphicsDevice* Device, Compute::Vector3& Camera, float& Distance, bool& Backcull, const bool& Inner)
			{
				Graphics::Shader* Active = nullptr;
				Device->SetShader(Shaders.Point[0], Graphics::ShaderType_Vertex);
				Device->SetBuffer(Shaders.Point[0], 3, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);

				for (auto It = PointLights->Begin(); It != PointLights->End(); ++It)
				{
					Engine::Components::PointLight* Light = (Engine::Components::PointLight*)*It;
					if (!System->PassCullable(Light, CullResult_Always, &Distance))
						continue;

					Compute::Vector3 Position(Light->GetEntity()->Transform->Position), Scale(Light->Size.Range);
					bool Front = Compute::Common::HasPointIntersectedCube(Position, Scale.Mul(1.025), Camera);
					if ((Front && Backcull) || (!Front && !Backcull))
					{
						Device->SetRasterizerState(Front ? FrontRasterizer : BackRasterizer);
						Device->SetDepthStencilState(Front ? DepthStencilGreater : DepthStencilLess);
						Backcull = !Backcull;
					}

					if (Light->Shadow.Enabled && Light->DepthMap != nullptr)
					{
						PointLight.Softness = Light->Shadow.Softness <= 0 ? 0 : (float)Shadows.PointLightResolution / Light->Shadow.Softness;
						PointLight.Bias = Light->Shadow.Bias;
						PointLight.Distance = Light->Shadow.Distance;
						PointLight.Iterations = (float)Light->Shadow.Iterations;
						PointLight.Umbra = Light->Disperse;
						Active = Shaders.Point[1];

						Device->SetTexture2D(Light->DepthMap->GetTarget(0), 5);
					}
					else
						Active = Shaders.Point[0];

					PointLight.WorldViewProjection = Compute::Matrix4x4::CreateTranslatedScale(Position, Scale) * System->GetScene()->View.ViewProjection;
					PointLight.Position = Light->GetEntity()->Transform->Position.InvertZ();
					PointLight.Lighting = Light->Diffuse.Mul(Light->Emission * Distance);
					PointLight.Attenuation.X = Light->Size.C1;
					PointLight.Attenuation.Y = Light->Size.C2;
					PointLight.Range = Scale.X;

					Device->SetShader(Active, Graphics::ShaderType_Pixel);
					Device->UpdateBuffer(Shaders.Point[0], &PointLight);
					Device->DrawIndexed((unsigned int)System->GetCubeIBuffer()->GetElements(), 0, 0);
				}
			}
			void Lighting::RenderSpotLights(Graphics::GraphicsDevice* Device, Compute::Vector3& Camera, float& Distance, bool& Backcull, const bool& Inner)
			{
				Graphics::Shader* Active = nullptr;
				Device->SetShader(Shaders.Spot[0], Graphics::ShaderType_Vertex);
				Device->SetBuffer(Shaders.Spot[0], 3, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);

				for (auto It = SpotLights->Begin(); It != SpotLights->End(); ++It)
				{
					Engine::Components::SpotLight* Light = (Engine::Components::SpotLight*)*It;
					if (!System->PassCullable(Light, CullResult_Always, &Distance))
						continue;

					Compute::Vector3 Position(Light->GetEntity()->Transform->Position), Scale(Light->Size.Range);
					bool Front = Compute::Common::HasPointIntersectedCube(Position, Scale.Mul(1.025), Camera);
					if ((Front && Backcull) || (!Front && !Backcull))
					{
						Device->SetRasterizerState(Front ? FrontRasterizer : BackRasterizer);
						Device->SetDepthStencilState(Front ? DepthStencilGreater : DepthStencilLess);
						Backcull = !Backcull;
					}

					if (Light->Shadow.Enabled && Light->DepthMap != nullptr)
					{
						SpotLight.Softness = Light->Shadow.Softness <= 0 ? 0 : (float)Shadows.SpotLightResolution / Light->Shadow.Softness;
						SpotLight.Bias = Light->Shadow.Bias;
						SpotLight.Iterations = (float)Light->Shadow.Iterations;
						SpotLight.Umbra = Light->Disperse;
						Active = Shaders.Spot[1];

						Device->SetTexture2D(Light->DepthMap->GetTarget(0), 5);
					}
					else
						Active = Shaders.Spot[0];

					SpotLight.WorldViewProjection = Compute::Matrix4x4::CreateTranslatedScale(Position, Scale) * System->GetScene()->View.ViewProjection;
					SpotLight.ViewProjection = Light->View * Light->Projection;
					SpotLight.Direction = Light->GetEntity()->Transform->Rotation.DepthDirection();
					SpotLight.Position = Light->GetEntity()->Transform->Position.InvertZ();
					SpotLight.Lighting = Light->Diffuse.Mul(Light->Emission * Distance);
					SpotLight.Cutoff = Compute::Mathf::Cos(Compute::Mathf::Deg2Rad() * Light->Cutoff * 0.5f);
					SpotLight.Attenuation.X = Light->Size.C1;
					SpotLight.Attenuation.Y = Light->Size.C2;
					SpotLight.Range = Scale.X;

					Device->SetShader(Active, Graphics::ShaderType_Pixel);
					Device->UpdateBuffer(Shaders.Spot[0], &SpotLight);
					Device->DrawIndexed((unsigned int)System->GetCubeIBuffer()->GetElements(), 0, 0);
				}
			}
			void Lighting::RenderLineLights(Graphics::GraphicsDevice* Device, bool& Backcull)
			{
				Graphics::Shader* Active = nullptr;
				if (!Backcull)
					Device->SetRasterizerState(BackRasterizer);

				Device->SetDepthStencilState(DepthStencilNone);
				Device->SetShader(Shaders.Line[0], Graphics::ShaderType_Vertex);
				Device->SetBuffer(Shaders.Line[0], 3, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);
				Device->SetVertexBuffer(System->GetQuadVBuffer(), 0);

				for (auto It = LineLights->Begin(); It != LineLights->End(); ++It)
				{
					Engine::Components::LineLight* Light = (Engine::Components::LineLight*)*It;
					if (Light->Shadow.Enabled && Light->DepthMap != nullptr)
					{
						LineLight.Softness = Light->Shadow.Softness <= 0 ? 0 : (float)Shadows.LineLightResolution / Light->Shadow.Softness;
						LineLight.Iterations = (float)Light->Shadow.Iterations;
						LineLight.Bias = Light->Shadow.Bias;
						LineLight.Cascades = Light->DepthMap->size();
						LineLight.Umbra = Light->Disperse;
						Active = Shaders.Line[1];

						for (size_t i = 0; i < Light->DepthMap->size(); i++)
						{
							LineLight.ViewProjection[i] = Light->View[i] * Light->Projection[i];
							Device->SetTexture2D((*Light->DepthMap)[i]->GetTarget(0), 5 + i);
						}
					}
					else
						Active = Shaders.Line[0];

					LineLight.Position = Light->GetEntity()->Transform->Position.InvertZ().NormalizeSafe();
					LineLight.Lighting = Light->Diffuse.Mul(Light->Emission);
					LineLight.RlhEmission = Light->Sky.RlhEmission;
					LineLight.RlhHeight = Light->Sky.RlhHeight;
					LineLight.MieEmission = Light->Sky.MieEmission;
					LineLight.MieHeight = Light->Sky.MieHeight;
					LineLight.ScatterIntensity = Light->Sky.Intensity;
					LineLight.PlanetRadius = Light->Sky.InnerRadius;
					LineLight.AtmosphereRadius = Light->Sky.OuterRadius;
					LineLight.MieDirection = Light->Sky.MieDirection;
					LineLight.SkyOffset = AmbientLight.SkyOffset;

					Device->SetShader(Active, Graphics::ShaderType_Pixel);
					Device->UpdateBuffer(Shaders.Line[0], &LineLight);
					Device->Draw(6, 0);
				}
			}
			void Lighting::RenderAmbientLight(Graphics::GraphicsDevice* Device, Graphics::MultiRenderTarget2D* Target, const bool& Inner)
			{
				Device->CopyTarget(Target, 0, Inner ? Output2 : Output1, 0);
				Device->Clear(Target, 0, 0, 0, 0);
				Device->SetTexture2D(Inner ? Output2->GetTarget(0) : Output1->GetTarget(0), 5);
				Device->SetTextureCube(SkyMap, 6);
				Device->SetShader(Shaders.Ambient[0], Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);
				Device->SetBuffer(Shaders.Ambient[0], 3, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);
				Device->UpdateBuffer(Shaders.Ambient[0], &AmbientLight);
				Device->Draw(6, 0);
			}
			void Lighting::GenerateCascadeMap(CascadedDepthMap** Result, uint32_t Size)
			{
				CascadedDepthMap* Target = (*Result ? *Result : new CascadedDepthMap());
				for (auto It = Target->begin(); It != Target->end(); It++)
					delete *It;

				Target->resize(Size);
				for (auto It = Target->begin(); It != Target->end(); It++)
				{
					Graphics::RenderTarget2D::Desc F = Graphics::RenderTarget2D::Desc();
					F.Width = (unsigned int)Shadows.LineLightResolution;
					F.Height = (unsigned int)Shadows.LineLightResolution;
					F.FormatMode = Graphics::Format_R32_Float;

					*It = System->GetDevice()->CreateRenderTarget2D(F);
				}

				*Result = Target;
			}
			void Lighting::FlushDepthBuffersAndCache()
			{
				if (System != nullptr && System->GetScene())
				{
					Rest::Pool<Component*>* Lights = System->GetScene()->GetComponents<Components::PointLight>();
					for (auto It = Lights->Begin(); It != Lights->End(); It++)
						(*It)->As<Components::PointLight>()->DepthMap = nullptr;

					Lights = System->GetScene()->GetComponents<Components::SpotLight>();
					for (auto It = Lights->Begin(); It != Lights->End(); It++)
						(*It)->As<Components::SpotLight>()->DepthMap = nullptr;

					Lights = System->GetScene()->GetComponents<Components::LineLight>();
					for (auto It = Lights->Begin(); It != Lights->End(); It++)
						(*It)->As<Components::LineLight>()->DepthMap = nullptr;
				}

				for (auto It = Shadows.PointLight.begin(); It != Shadows.PointLight.end(); It++)
					delete *It;

				for (auto It = Shadows.SpotLight.begin(); It != Shadows.SpotLight.end(); It++)
					delete *It;

				for (auto It = Shadows.LineLight.begin(); It != Shadows.LineLight.end(); It++)
				{
					if (*It != nullptr)
					{
						for (auto* Target : *(*It))
							delete Target;
					}

					delete *It;
				}
			}
			void Lighting::SetSkyMap(Graphics::Texture2D* Cubemap)
			{
				TH_RELEASE(SkyBase);
				SkyBase = Cubemap;

				TH_CLEAR(SkyMap);
				if (SkyBase != nullptr)
					SkyMap = System->GetDevice()->CreateTextureCube(SkyBase);
			}
			void Lighting::SetSurfaceBufferSize(size_t NewSize)
			{
				Graphics::MultiRenderTarget2D::Desc F;
				System->GetScene()->GetTargetDesc(&F);

				Surfaces.Size = NewSize;
				F.MipLevels = System->GetDevice()->GetMipLevel((unsigned int)Surfaces.Size, (unsigned int)Surfaces.Size);
				F.Width = (unsigned int)Surfaces.Size;
				F.Height = (unsigned int)Surfaces.Size;
				SurfaceLight.MipLevels = (float)F.MipLevels;

				TH_RELEASE(Surface);
				Surface = System->GetDevice()->CreateMultiRenderTarget2D(F);

				Graphics::Cubemap::Desc I;
				I.Source = Surface;
				I.MipLevels = F.MipLevels;
				I.Size = Surfaces.Size;

				TH_RELEASE(Subresource);
				Subresource = System->GetDevice()->CreateCubemap(I);

				Graphics::RenderTarget2D::Desc F1;
				System->GetScene()->GetTargetDesc(&F1);

				F1.Width = (unsigned int)Surfaces.Size;
				F1.Height = (unsigned int)Surfaces.Size;
				F1.MipLevels = F.MipLevels;

				TH_RELEASE(Output2);
				Output2 = System->GetDevice()->CreateRenderTarget2D(F1);

				TH_RELEASE(Input2);
				Input2 = System->GetDevice()->CreateRenderTarget2D(F1);
			}
			void Lighting::SetVoxelBufferSize(size_t NewSize)
			{
				Graphics::Format Formats[3];
				System->GetScene()->GetTargetFormat(Formats, 3);

				Graphics::Texture3D::Desc I;
				I.Width = I.Height = I.Depth = IndirectLight.Size = NewSize;
				I.MipLevels = System->GetDevice()->GetMipLevel(IndirectLight.Size, IndirectLight.Size);
				I.FormatMode = Formats[0];
				I.Writable = true;

				TH_RELEASE(DiffuseBuffer);
				DiffuseBuffer = System->GetDevice()->CreateTexture3D(I);

				I.MipLevels = 0;
				I.FormatMode = Formats[1];
				TH_RELEASE(NormalBuffer);
				NormalBuffer = System->GetDevice()->CreateTexture3D(I);

				I.MipLevels = 0;
				I.FormatMode = Formats[2];
				TH_RELEASE(SurfaceBuffer);
				SurfaceBuffer = System->GetDevice()->CreateTexture3D(I);
			}
			void Lighting::SetVoxelBuffer(RenderSystem* System, Graphics::Shader* Src, unsigned int Slot)
			{
				if (!System || !Src)
					return;

				Lighting* Renderer = System->GetRenderer<Lighting>();
				if (!Renderer)
					return;

				Graphics::GraphicsDevice* Device = System->GetDevice();
				Device->SetBuffer(Src, Slot, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel | Graphics::ShaderType_Geometry);
				Device->UpdateBuffer(Src, &Renderer->Voxelizer);
			}
			Graphics::TextureCube* Lighting::GetSkyMap()
			{
				if (!SkyBase)
					return nullptr;

				return SkyMap;
			}
			Graphics::Texture2D* Lighting::GetSkyBase()
			{
				if (!SkyMap)
					return nullptr;

				return SkyBase;
			}

			Transparency::Transparency(RenderSystem* Lab) : Renderer(Lab)
			{
				DepthStencil = Lab->GetDevice()->GetDepthStencilState("none");
				Rasterizer = Lab->GetDevice()->GetRasterizerState("cull-back");
				Blend = Lab->GetDevice()->GetBlendState("overwrite");
				Sampler = Lab->GetDevice()->GetSamplerState("trilinear-x16");
				Layout = Lab->GetDevice()->GetInputLayout("shape-vertex");

				Graphics::Shader::Desc I = Graphics::Shader::Desc();
				if (System->GetDevice()->GetSection("shaders/effects/transparency", &I.Data))
					Shader = System->CompileShader("lr-transparency", I, sizeof(RenderPass));
			}
			Transparency::~Transparency()
			{
				System->FreeShader("lr-transparency", Shader);
				TH_RELEASE(Surface1);
				TH_RELEASE(Input1);
				TH_RELEASE(Surface2);
				TH_RELEASE(Input2);
			}
			void Transparency::ResizeBuffers()
			{
				Graphics::MultiRenderTarget2D::Desc F1;
				System->GetScene()->GetTargetDesc(&F1);
				MipLevels1 = (float)F1.MipLevels;

				TH_RELEASE(Surface1);
				Surface1 = System->GetDevice()->CreateMultiRenderTarget2D(F1);

				Graphics::RenderTarget2D::Desc F2;
				System->GetScene()->GetTargetDesc(&F2);

				TH_RELEASE(Input1);
				Input1 = System->GetDevice()->CreateRenderTarget2D(F2);

				auto* Renderer = System->GetRenderer<Lighting>();
				if (Renderer != nullptr)
				{
					F1.MipLevels = System->GetDevice()->GetMipLevel((unsigned int)Renderer->Surfaces.Size, (unsigned int)Renderer->Surfaces.Size);
					F1.Width = (unsigned int)Renderer->Surfaces.Size;
					F1.Height = (unsigned int)Renderer->Surfaces.Size;
					F2.MipLevels = F1.MipLevels;
					F2.Width = (unsigned int)Renderer->Surfaces.Size;
					F2.Height = (unsigned int)Renderer->Surfaces.Size;
					MipLevels2 = (float)F1.MipLevels;
				}

				TH_RELEASE(Surface2);
				Surface2 = System->GetDevice()->CreateMultiRenderTarget2D(F1);

				TH_RELEASE(Input2);
				Input2 = System->GetDevice()->CreateRenderTarget2D(F2);
			}
			void Transparency::Render(Rest::Timer* Time, RenderState State, RenderOpt Options)
			{
				bool Inner = (Options & RenderOpt_Inner);
				if (State != RenderState_Geometry_Result || Options & RenderOpt_Transparent)
					return;

				SceneGraph* Scene = System->GetScene();
				if (!Scene->GetTransparentCount())
					return;

				Graphics::MultiRenderTarget2D* S = Scene->GetSurface();
				Graphics::GraphicsDevice* Device = System->GetDevice();
				RenderPass.MipLevels = (Inner ? MipLevels2 : MipLevels1);

				Scene->SwapSurface(Inner ? Surface2 : Surface1);
				Scene->SetSurfaceCleared();
				Scene->Render(Time, RenderState_Geometry_Result, Options | RenderOpt_Transparent);
				Scene->SwapSurface(S);

				Device->CopyTarget(S, 0, Inner ? Input2 : Input1, 0);
				Device->GenerateMips(Inner ? Input2->GetTarget(0) : Input1->GetTarget(0));
				Device->SetTarget(S, 0);
				Device->Clear(S, 0, 0, 0, 0);
				Device->UpdateBuffer(Shader, &RenderPass);
				Device->SetSamplerState(Sampler, 0);
				Device->SetDepthStencilState(DepthStencil);
				Device->SetBlendState(Blend);
				Device->SetRasterizerState(Rasterizer);
				Device->SetInputLayout(Layout);
				Device->SetShader(Shader, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);
				Device->SetBuffer(Shader, 3, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);
				Device->SetVertexBuffer(System->GetQuadVBuffer(), 0);
				Device->SetTexture2D(Inner ? Input2->GetTarget(0) : Input1->GetTarget(0), 1);
				Device->SetTexture2D(S->GetTarget(1), 2);
				Device->SetTexture2D(S->GetTarget(2), 3);
				Device->SetTexture2D(S->GetTarget(3), 4);
				Device->SetTexture2D(Inner ? Surface2->GetTarget(0) : Surface1->GetTarget(0), 5);
				Device->SetTexture2D(Inner ? Surface2->GetTarget(1) : Surface1->GetTarget(1), 6);
				Device->SetTexture2D(Inner ? Surface2->GetTarget(2) : Surface1->GetTarget(2), 7);
				Device->SetTexture2D(Inner ? Surface2->GetTarget(3) : Surface1->GetTarget(3), 8);
				Device->UpdateBuffer(Graphics::RenderBufferType_Render);
				Device->Draw(6, 0);
				Device->FlushTexture2D(1, 8);
			}

			SSR::SSR(RenderSystem* Lab) : EffectDraw(Lab), Pass1(nullptr), Pass2(nullptr)
			{
				std::string Data;
				if (System->GetDevice()->GetSection("shaders/effects/reflection", &Data))
					Pass1 = CompileEffect("rr-reflection", Data, sizeof(RenderPass1));

				if (System->GetDevice()->GetSection("shaders/effects/gloss-x", &Data))
					Pass2 = CompileEffect("rr-gloss-x", Data, sizeof(RenderPass2));

				if (System->GetDevice()->GetSection("shaders/effects/gloss-y", &Data))
					Pass3 = CompileEffect("rr-gloss-y", Data, sizeof(RenderPass2));
			}
			void SSR::Deserialize(ContentManager* Content, Rest::Document* Node)
			{
				NMake::Unpack(Node->Find("samples-1"), &RenderPass1.Samples);
				NMake::Unpack(Node->Find("samples-2"), &RenderPass2.Samples);
				NMake::Unpack(Node->Find("intensity"), &RenderPass1.Intensity);
				NMake::Unpack(Node->Find("blur"), &RenderPass2.Blur);
			}
			void SSR::Serialize(ContentManager* Content, Rest::Document* Node)
			{
				NMake::Pack(Node->SetDocument("samples-1"), RenderPass1.Samples);
				NMake::Pack(Node->SetDocument("samples-2"), RenderPass2.Samples);
				NMake::Pack(Node->SetDocument("intensity"), RenderPass1.Intensity);
				NMake::Pack(Node->SetDocument("blur"), RenderPass2.Blur);
			}
			void SSR::RenderEffect(Rest::Timer* Time)
			{
				Graphics::MultiRenderTarget2D* Surface = System->GetScene()->GetSurface();
				if (Surface != nullptr)
					System->GetDevice()->GenerateMips(Surface->GetTarget(0));

				RenderPass1.MipLevels = System->GetDevice()->GetMipLevel((unsigned int)Output->GetWidth(), (unsigned int)Output->GetHeight());
				RenderPass2.Texel[0] = 1.0f / Output->GetWidth();
				RenderPass2.Texel[1] = 1.0f / Output->GetHeight();

				RenderMerge(Pass1, &RenderPass1);
				RenderMerge(Pass2, &RenderPass2);
				RenderResult(Pass3, &RenderPass2);
			}

			SSDO::SSDO(RenderSystem* Lab) : EffectDraw(Lab), Pass1(nullptr), Pass2(nullptr)
			{
				std::string Data;
				if (System->GetDevice()->GetSection("shaders/effects/indirect", &Data))
					Pass1 = CompileEffect("dor-indirect", Data, sizeof(RenderPass1));

				if (System->GetDevice()->GetSection("shaders/effects/blur-x", &Data))
					Pass2 = CompileEffect("dor-blur-x", Data, sizeof(RenderPass2));

				if (System->GetDevice()->GetSection("shaders/effects/blur-y", &Data))
					Pass3 = CompileEffect("dor-blur-y", Data, sizeof(RenderPass2));
			}
			void SSDO::Deserialize(ContentManager* Content, Rest::Document* Node)
			{
				NMake::Unpack(Node->Find("scale"), &RenderPass1.Scale);
				NMake::Unpack(Node->Find("intensity"), &RenderPass1.Intensity);
				NMake::Unpack(Node->Find("bias"), &RenderPass1.Bias);
				NMake::Unpack(Node->Find("radius"), &RenderPass1.Radius);
				NMake::Unpack(Node->Find("distance"), &RenderPass1.Distance);
				NMake::Unpack(Node->Find("fade"), &RenderPass1.Fade);
				NMake::Unpack(Node->Find("power"), &RenderPass2.Power);
				NMake::Unpack(Node->Find("samples-1"), &RenderPass1.Samples);
				NMake::Unpack(Node->Find("samples-2"), &RenderPass2.Samples);
				NMake::Unpack(Node->Find("blur"), &RenderPass2.Blur);
				NMake::Unpack(Node->Find("additive"), &RenderPass2.Additive);
			}
			void SSDO::Serialize(ContentManager* Content, Rest::Document* Node)
			{
				NMake::Pack(Node->SetDocument("scale"), RenderPass1.Scale);
				NMake::Pack(Node->SetDocument("intensity"), RenderPass1.Intensity);
				NMake::Pack(Node->SetDocument("bias"), RenderPass1.Bias);
				NMake::Pack(Node->SetDocument("radius"), RenderPass1.Radius);
				NMake::Pack(Node->SetDocument("distance"), RenderPass1.Distance);
				NMake::Pack(Node->SetDocument("fade"), RenderPass1.Fade);
				NMake::Pack(Node->SetDocument("power"), RenderPass2.Power);
				NMake::Pack(Node->SetDocument("samples-1"), RenderPass1.Samples);
				NMake::Pack(Node->SetDocument("samples-2"), RenderPass2.Samples);
				NMake::Pack(Node->SetDocument("blur"), RenderPass2.Blur);
				NMake::Pack(Node->SetDocument("additive"), RenderPass2.Additive);
			}
			void SSDO::RenderEffect(Rest::Timer* Time)
			{
				RenderPass2.Texel[0] = 1.0f / Output->GetWidth();
				RenderPass2.Texel[1] = 1.0f / Output->GetHeight();

				RenderMerge(Pass1, &RenderPass1);
				RenderMerge(Pass2, &RenderPass2);
				RenderResult(Pass3, &RenderPass2);
			}

			SSAO::SSAO(RenderSystem* Lab) : EffectDraw(Lab), Pass1(nullptr), Pass2(nullptr)
			{
				std::string Data;
				if (System->GetDevice()->GetSection("shaders/effects/ambient", &Data))
					Pass1 = CompileEffect("aor-ambient", Data, sizeof(RenderPass1));

				if (System->GetDevice()->GetSection("shaders/effects/blur-x", &Data))
					Pass2 = CompileEffect("aor-blur-x", Data, sizeof(RenderPass2));

				if (System->GetDevice()->GetSection("shaders/effects/blur-y", &Data))
					Pass3 = CompileEffect("aor-blur-y", Data, sizeof(RenderPass2));
			}
			void SSAO::Deserialize(ContentManager* Content, Rest::Document* Node)
			{
				NMake::Unpack(Node->Find("scale"), &RenderPass1.Scale);
				NMake::Unpack(Node->Find("intensity"), &RenderPass1.Intensity);
				NMake::Unpack(Node->Find("bias"), &RenderPass1.Bias);
				NMake::Unpack(Node->Find("radius"), &RenderPass1.Radius);
				NMake::Unpack(Node->Find("distance"), &RenderPass1.Distance);
				NMake::Unpack(Node->Find("fade"), &RenderPass1.Fade);
				NMake::Unpack(Node->Find("power"), &RenderPass2.Power);
				NMake::Unpack(Node->Find("samples-1"), &RenderPass1.Samples);
				NMake::Unpack(Node->Find("samples-2"), &RenderPass2.Samples);
				NMake::Unpack(Node->Find("blur"), &RenderPass2.Blur);
				NMake::Unpack(Node->Find("additive"), &RenderPass2.Additive);
			}
			void SSAO::Serialize(ContentManager* Content, Rest::Document* Node)
			{
				NMake::Pack(Node->SetDocument("scale"), RenderPass1.Scale);
				NMake::Pack(Node->SetDocument("intensity"), RenderPass1.Intensity);
				NMake::Pack(Node->SetDocument("bias"), RenderPass1.Bias);
				NMake::Pack(Node->SetDocument("radius"), RenderPass1.Radius);
				NMake::Pack(Node->SetDocument("distance"), RenderPass1.Distance);
				NMake::Pack(Node->SetDocument("fade"), RenderPass1.Fade);
				NMake::Pack(Node->SetDocument("power"), RenderPass2.Power);
				NMake::Pack(Node->SetDocument("samples-1"), RenderPass1.Samples);
				NMake::Pack(Node->SetDocument("samples-2"), RenderPass2.Samples);
				NMake::Pack(Node->SetDocument("blur"), RenderPass2.Blur);
				NMake::Pack(Node->SetDocument("additive"), RenderPass2.Additive);
			}
			void SSAO::RenderEffect(Rest::Timer* Time)
			{
				RenderPass2.Texel[0] = 1.0f / Output->GetWidth();
				RenderPass2.Texel[1] = 1.0f / Output->GetHeight();

				RenderMerge(Pass1, &RenderPass1);
				RenderMerge(Pass2, &RenderPass2);
				RenderResult(Pass3, &RenderPass2);
			}

			DoF::DoF(RenderSystem* Lab) : EffectDraw(Lab)
			{
				std::string Data;
				if (System->GetDevice()->GetSection("shaders/effects/focus", &Data))
					CompileEffect("dfr-focus", Data, sizeof(RenderPass));
			}
			void DoF::Deserialize(ContentManager* Content, Rest::Document* Node)
			{
				NMake::Unpack(Node->Find("focus-distance"), &FocusDistance);
				NMake::Unpack(Node->Find("focus-time"), &FocusTime);
				NMake::Unpack(Node->Find("focus-radius"), &FocusRadius);
				NMake::Unpack(Node->Find("radius"), &RenderPass.Radius);
				NMake::Unpack(Node->Find("bokeh"), &RenderPass.Bokeh);
				NMake::Unpack(Node->Find("scale"), &RenderPass.Scale);
				NMake::Unpack(Node->Find("near-distance"), &RenderPass.NearDistance);
				NMake::Unpack(Node->Find("near-range"), &RenderPass.NearRange);
				NMake::Unpack(Node->Find("far-distance"), &RenderPass.FarDistance);
				NMake::Unpack(Node->Find("far-range"), &RenderPass.FarRange);
			}
			void DoF::Serialize(ContentManager* Content, Rest::Document* Node)
			{
				NMake::Pack(Node->SetDocument("focus-distance"), FocusDistance);
				NMake::Pack(Node->SetDocument("focus-time"), FocusTime);
				NMake::Pack(Node->SetDocument("focus-radius"), FocusRadius);
				NMake::Pack(Node->SetDocument("radius"), RenderPass.Radius);
				NMake::Pack(Node->SetDocument("bokeh"), RenderPass.Bokeh);
				NMake::Pack(Node->SetDocument("scale"), RenderPass.Scale);
				NMake::Pack(Node->SetDocument("near-distance"), RenderPass.NearDistance);
				NMake::Pack(Node->SetDocument("near-range"), RenderPass.NearRange);
				NMake::Pack(Node->SetDocument("far-distance"), RenderPass.FarDistance);
				NMake::Pack(Node->SetDocument("far-range"), RenderPass.FarRange);
			}
			void DoF::RenderEffect(Rest::Timer* Time)
			{
				if (FocusDistance > 0.0f)
					FocusAtNearestTarget(Time->GetDeltaTime());

				RenderPass.Texel[0] = 1.0f / Output->GetWidth();
				RenderPass.Texel[1] = 1.0f / Output->GetHeight();
				RenderResult(nullptr, &RenderPass);
			}
			void DoF::FocusAtNearestTarget(float DeltaTime)
			{
				Compute::Ray Origin;
				Origin.Origin = System->GetScene()->View.WorldPosition.InvertZ();
				Origin.Direction = System->GetScene()->View.WorldRotation.DepthDirection();

				Component* Target = nullptr;
				System->GetScene()->RayTest<Components::Model>(Origin, FocusDistance, [this, &Target](Component* Result)
				{
					float NextRange = Result->As<Components::Model>()->GetRange();
					float NextDistance = Result->GetEntity()->Distance + NextRange / 2.0f;

					if (NextDistance <= RenderPass.NearRange || NextDistance + NextRange / 2.0f >= FocusDistance)
						return true;

					if (NextDistance >= State.Distance && State.Distance > 0.0f)
						return true;

					State.Distance = NextDistance;
					State.Range = NextRange;
					Target = Result;

					return true;
				});

				if (State.Target != Target)
				{
					State.Target = Target;
					State.Radius = RenderPass.Radius;
					State.Factor = 0.0f;
				}
				else if (!State.Target)
					State.Distance = 0.0f;

				State.Factor += FocusTime * DeltaTime;
				if (State.Factor > 1.0f)
					State.Factor = 1.0f;

				if (State.Distance > 0.0f)
				{
					State.Distance += State.Range / 2.0f + RenderPass.FarRange;
					RenderPass.FarDistance = State.Distance;
					RenderPass.Radius = Compute::Math<float>::Lerp(State.Radius, FocusRadius, State.Factor);
				}
				else
				{
					State.Distance = 0.0f;
					if (State.Factor >= 1.0f)
						RenderPass.FarDistance = State.Distance;

					RenderPass.Radius = Compute::Math<float>::Lerp(State.Radius, 0.0f, State.Factor);
				}

				if (RenderPass.Radius < 0.0f)
					RenderPass.Radius = 0.0f;
			}

			Bloom::Bloom(RenderSystem* Lab) : EffectDraw(Lab)
			{
				std::string Data;
				if (System->GetDevice()->GetSection("shaders/effects/bloom-x", &Data))
					Pass1 = CompileEffect("br-bloom-x", Data, sizeof(RenderPass));

				if (System->GetDevice()->GetSection("shaders/effects/bloom-y", &Data))
					Pass2 = CompileEffect("br-bloom-y", Data, sizeof(RenderPass));
			}
			void Bloom::Deserialize(ContentManager* Content, Rest::Document* Node)
			{
				NMake::Unpack(Node->Find("samples"), &RenderPass.Samples);
				NMake::Unpack(Node->Find("intensity"), &RenderPass.Intensity);
				NMake::Unpack(Node->Find("threshold"), &RenderPass.Threshold);
				NMake::Unpack(Node->Find("scale"), &RenderPass.Scale);
			}
			void Bloom::Serialize(ContentManager* Content, Rest::Document* Node)
			{
				NMake::Pack(Node->SetDocument("samples"), RenderPass.Samples);
				NMake::Pack(Node->SetDocument("intensity"), RenderPass.Intensity);
				NMake::Pack(Node->SetDocument("threshold"), RenderPass.Threshold);
				NMake::Pack(Node->SetDocument("scale"), RenderPass.Scale);
			}
			void Bloom::RenderEffect(Rest::Timer* Time)
			{
				RenderPass.Texel[0] = 1.0f / Output->GetWidth();
				RenderPass.Texel[1] = 1.0f / Output->GetHeight();
				RenderMerge(Pass1, &RenderPass);
				RenderResult(Pass2, &RenderPass);
			}

			Tone::Tone(RenderSystem* Lab) : EffectDraw(Lab)
			{
				std::string Data;
				if (System->GetDevice()->GetSection("shaders/effects/tone", &Data))
					CompileEffect("tr-tone", Data, sizeof(RenderPass));
			}
			void Tone::Deserialize(ContentManager* Content, Rest::Document* Node)
			{
				NMake::Unpack(Node->Find("blind-vision-r"), &RenderPass.BlindVisionR);
				NMake::Unpack(Node->Find("blind-vision-g"), &RenderPass.BlindVisionG);
				NMake::Unpack(Node->Find("blind-vision-b"), &RenderPass.BlindVisionB);
				NMake::Unpack(Node->Find("vignette-color"), &RenderPass.VignetteColor);
				NMake::Unpack(Node->Find("color-gamma"), &RenderPass.ColorGamma);
				NMake::Unpack(Node->Find("desaturation-gamma"), &RenderPass.DesaturationGamma);
				NMake::Unpack(Node->Find("vignette-amount"), &RenderPass.VignetteAmount);
				NMake::Unpack(Node->Find("vignette-curve"), &RenderPass.VignetteCurve);
				NMake::Unpack(Node->Find("vignette-radius"), &RenderPass.VignetteRadius);
				NMake::Unpack(Node->Find("linear-intensity"), &RenderPass.LinearIntensity);
				NMake::Unpack(Node->Find("gamma-intensity"), &RenderPass.GammaIntensity);
				NMake::Unpack(Node->Find("desaturation-intensity"), &RenderPass.DesaturationIntensity);
				NMake::Unpack(Node->Find("tone-intensity"), &RenderPass.ToneIntensity);
				NMake::Unpack(Node->Find("aces-intensity"), &RenderPass.AcesIntensity);
				NMake::Unpack(Node->Find("aces-a"), &RenderPass.AcesA);
				NMake::Unpack(Node->Find("aces-b"), &RenderPass.AcesB);
				NMake::Unpack(Node->Find("aces-c"), &RenderPass.AcesC);
				NMake::Unpack(Node->Find("aces-d"), &RenderPass.AcesD);
				NMake::Unpack(Node->Find("aces-e"), &RenderPass.AcesE);
			}
			void Tone::Serialize(ContentManager* Content, Rest::Document* Node)
			{
				NMake::Pack(Node->SetDocument("blind-vision-r"), RenderPass.BlindVisionR);
				NMake::Pack(Node->SetDocument("blind-vision-g"), RenderPass.BlindVisionG);
				NMake::Pack(Node->SetDocument("blind-vision-b"), RenderPass.BlindVisionB);
				NMake::Pack(Node->SetDocument("vignette-color"), RenderPass.VignetteColor);
				NMake::Pack(Node->SetDocument("color-gamma"), RenderPass.ColorGamma);
				NMake::Pack(Node->SetDocument("desaturation-gamma"), RenderPass.DesaturationGamma);
				NMake::Pack(Node->SetDocument("vignette-amount"), RenderPass.VignetteAmount);
				NMake::Pack(Node->SetDocument("vignette-curve"), RenderPass.VignetteCurve);
				NMake::Pack(Node->SetDocument("vignette-radius"), RenderPass.VignetteRadius);
				NMake::Pack(Node->SetDocument("linear-intensity"), RenderPass.LinearIntensity);
				NMake::Pack(Node->SetDocument("gamma-intensity"), RenderPass.GammaIntensity);
				NMake::Pack(Node->SetDocument("desaturation-intensity"), RenderPass.DesaturationIntensity);
				NMake::Pack(Node->SetDocument("tone-intensity"), RenderPass.ToneIntensity);
				NMake::Pack(Node->SetDocument("aces-intensity"), RenderPass.AcesIntensity);
				NMake::Pack(Node->SetDocument("aces-a"), RenderPass.AcesA);
				NMake::Pack(Node->SetDocument("aces-b"), RenderPass.AcesB);
				NMake::Pack(Node->SetDocument("aces-c"), RenderPass.AcesC);
				NMake::Pack(Node->SetDocument("aces-d"), RenderPass.AcesD);
				NMake::Pack(Node->SetDocument("aces-e"), RenderPass.AcesE);
			}
			void Tone::RenderEffect(Rest::Timer* Time)
			{
				RenderResult(nullptr, &RenderPass);
			}

			Glitch::Glitch(RenderSystem* Lab) : EffectDraw(Lab), ScanLineJitter(0), VerticalJump(0), HorizontalShake(0), ColorDrift(0)
			{
				std::string Data;
				if (System->GetDevice()->GetSection("shaders/effects/glitch", &Data))
					CompileEffect("gr-glitch", Data, sizeof(RenderPass));
			}
			void Glitch::Deserialize(ContentManager* Content, Rest::Document* Node)
			{
				NMake::Unpack(Node->Find("scanline-jitter"), &ScanLineJitter);
				NMake::Unpack(Node->Find("vertical-jump"), &VerticalJump);
				NMake::Unpack(Node->Find("horizontal-shake"), &HorizontalShake);
				NMake::Unpack(Node->Find("color-drift"), &ColorDrift);
				NMake::Unpack(Node->Find("horizontal-shake"), &HorizontalShake);
				NMake::Unpack(Node->Find("elapsed-time"), &RenderPass.ElapsedTime);
				NMake::Unpack(Node->Find("scanline-jitter-displacement"), &RenderPass.ScanLineJitterDisplacement);
				NMake::Unpack(Node->Find("scanline-jitter-threshold"), &RenderPass.ScanLineJitterThreshold);
				NMake::Unpack(Node->Find("vertical-jump-amount"), &RenderPass.VerticalJumpAmount);
				NMake::Unpack(Node->Find("vertical-jump-time"), &RenderPass.VerticalJumpTime);
				NMake::Unpack(Node->Find("color-drift-amount"), &RenderPass.ColorDriftAmount);
				NMake::Unpack(Node->Find("color-drift-time"), &RenderPass.ColorDriftTime);
			}
			void Glitch::Serialize(ContentManager* Content, Rest::Document* Node)
			{
				NMake::Pack(Node->SetDocument("scanline-jitter"), ScanLineJitter);
				NMake::Pack(Node->SetDocument("vertical-jump"), VerticalJump);
				NMake::Pack(Node->SetDocument("horizontal-shake"), HorizontalShake);
				NMake::Pack(Node->SetDocument("color-drift"), ColorDrift);
				NMake::Pack(Node->SetDocument("horizontal-shake"), HorizontalShake);
				NMake::Pack(Node->SetDocument("elapsed-time"), RenderPass.ElapsedTime);
				NMake::Pack(Node->SetDocument("scanline-jitter-displacement"), RenderPass.ScanLineJitterDisplacement);
				NMake::Pack(Node->SetDocument("scanline-jitter-threshold"), RenderPass.ScanLineJitterThreshold);
				NMake::Pack(Node->SetDocument("vertical-jump-amount"), RenderPass.VerticalJumpAmount);
				NMake::Pack(Node->SetDocument("vertical-jump-time"), RenderPass.VerticalJumpTime);
				NMake::Pack(Node->SetDocument("color-drift-amount"), RenderPass.ColorDriftAmount);
				NMake::Pack(Node->SetDocument("color-drift-time"), RenderPass.ColorDriftTime);
			}
			void Glitch::RenderEffect(Rest::Timer* Time)
			{
				if (RenderPass.ElapsedTime >= 32000.0f)
					RenderPass.ElapsedTime = 0.0f;

				RenderPass.ElapsedTime += (float)Time->GetDeltaTime() * 10.0f;
				RenderPass.VerticalJumpAmount = VerticalJump;
				RenderPass.VerticalJumpTime += (float)Time->GetDeltaTime() * VerticalJump * 11.3f;
				RenderPass.ScanLineJitterThreshold = Compute::Mathf::Saturate(1.0f - ScanLineJitter * 1.2f);
				RenderPass.ScanLineJitterDisplacement = 0.002f + Compute::Mathf::Pow(ScanLineJitter, 3) * 0.05f;
				RenderPass.HorizontalShake = HorizontalShake * 0.2f;
				RenderPass.ColorDriftAmount = ColorDrift * 0.04f;
				RenderPass.ColorDriftTime = RenderPass.ElapsedTime * 606.11f;
				RenderResult(nullptr, &RenderPass);
			}

			UserInterface::UserInterface(RenderSystem* Lab) : UserInterface(Lab, Application::Get() ? Application::Get()->Activity : nullptr)
			{
			}
			UserInterface::UserInterface(RenderSystem* Lab, Graphics::Activity* NewActivity) : Renderer(Lab), Activity(NewActivity)
			{
				Context = new GUI::Context(Lab->GetDevice());
			}
			UserInterface::~UserInterface()
			{
				TH_RELEASE(Context);
			}
			void UserInterface::Render(Rest::Timer* Timer, RenderState State, RenderOpt Options)
			{
				if (State != RenderState_Geometry_Result || Options & RenderOpt_Inner || Options & RenderOpt_Transparent)
					return;

				Graphics::MultiRenderTarget2D* Surface = System->GetScene()->GetSurface();
				if (!Context)
					return;

				Context->UpdateEvents(Activity);
				if (!Surface)
					return;

				System->GetDevice()->SetTarget(Surface);
				Context->RenderLists(Surface->GetTarget(0));
			}
			GUI::Context* UserInterface::GetContext()
			{
				return Context;
			}
		}
	}
}
