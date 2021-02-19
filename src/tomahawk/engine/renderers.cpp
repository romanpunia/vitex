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
				Graphics::GraphicsDevice* Device = System->GetDevice();
				DepthStencil = Device->GetDepthStencilState("less");
				BackRasterizer = Device->GetRasterizerState("cull-back");
				FrontRasterizer = Device->GetRasterizerState("cull-front");
				Blend = Device->GetBlendState("overwrite");
				Sampler = Device->GetSamplerState("trilinear-x16");
				Layout = Device->GetInputLayout("vertex");

				Shaders.Geometry = System->CompileShader("geometry/model/geometry");
				Shaders.Voxelize = System->CompileShader("geometry/model/voxelize", sizeof(Lighting::VoxelBuffer));
				Shaders.Occlusion = System->CompileShader("geometry/model/occlusion");
				Shaders.Depth.Linear = System->CompileShader("geometry/model/depth/linear");
				Shaders.Depth.Cubic = System->CompileShader("geometry/model/depth/cubic", sizeof(Compute::Matrix4x4) * 6);
			}
			Model::~Model()
			{
				System->FreeShader(Shaders.Geometry);
				System->FreeShader(Shaders.Voxelize);
				System->FreeShader(Shaders.Occlusion);
				System->FreeShader(Shaders.Depth.Linear);
				System->FreeShader(Shaders.Depth.Cubic);
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
				Graphics::ElementBuffer* Box[2];
				System->GetPrimitives()->GetBoxBuffers(Box);

				Graphics::GraphicsDevice* Device = System->GetDevice();
				Device->SetRasterizerState(BackRasterizer);
				Device->SetInputLayout(Layout);
				Device->SetShader(nullptr, TH_PS);
				Device->SetShader(Shaders.Occlusion, TH_VS);

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
						Device->SetIndexBuffer(Box[BufferType_Index], Graphics::Format_R32_Uint);
						Device->SetVertexBuffer(Box[BufferType_Vertex], 0);
						Device->DrawIndexed(Box[BufferType_Index]->GetElements(), 0, 0);
					}
					Base->End(Device);
				}
			}
			void Model::RenderGeometryResult(Rest::Timer* Time, Rest::Pool<Drawable*>* Geometry, RenderOpt Options)
			{
				Graphics::GraphicsDevice* Device = System->GetDevice();
				CullResult Cull = (Options & RenderOpt_Inner ? CullResult_Always : CullResult_Last);
				bool Static = (Options & RenderOpt_Static);

				Device->SetDepthStencilState(DepthStencil);
				Device->SetBlendState(Blend);
				Device->SetRasterizerState(BackRasterizer);
				Device->SetInputLayout(Layout);
				Device->SetSamplerState(Sampler, 0, TH_PS);
				Device->SetShader(Shaders.Geometry, TH_VS | TH_PS);

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

				Device->FlushTexture2D(1, 7, TH_PS);
			}
			void Model::RenderGeometryVoxels(Rest::Timer* Time, Rest::Pool<Drawable*>* Geometry, RenderOpt Options)
			{
				Graphics::GraphicsDevice* Device = System->GetDevice();
				Device->SetInputLayout(Layout);
				Device->SetSamplerState(Sampler, 0, TH_PS);
				Device->SetShader(Shaders.Voxelize, TH_VS | TH_PS | TH_GS);
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

				Device->FlushTexture2D(5, 6, TH_PS);
				Device->SetShader(nullptr, TH_GS);
			}
			void Model::RenderDepthLinear(Rest::Timer* Time, Rest::Pool<Drawable*>* Geometry)
			{
				Graphics::GraphicsDevice* Device = System->GetDevice();
				Device->SetDepthStencilState(DepthStencil);
				Device->SetBlendState(Blend);
				Device->SetRasterizerState(FrontRasterizer);
				Device->SetInputLayout(Layout);
				Device->SetSamplerState(Sampler, 0, TH_PS);
				Device->SetShader(Shaders.Depth.Linear, TH_VS | TH_PS);

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

				Device->SetTexture2D(nullptr, 1, TH_PS);
			}
			void Model::RenderDepthCubic(Rest::Timer* Time, Rest::Pool<Drawable*>* Geometry, Compute::Matrix4x4* ViewProjection)
			{
				Graphics::GraphicsDevice* Device = System->GetDevice();
				Device->SetDepthStencilState(DepthStencil);
				Device->SetBlendState(Blend);
				Device->SetRasterizerState(FrontRasterizer);
				Device->SetInputLayout(Layout);
				Device->SetSamplerState(Sampler, 0, TH_PS);
				Device->SetShader(Shaders.Depth.Cubic, TH_VS | TH_PS | TH_GS);
				Device->SetBuffer(Shaders.Depth.Cubic, 3, TH_VS | TH_PS | TH_GS);
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

				Device->SetTexture2D(nullptr, 1, TH_PS);
				Device->SetShader(nullptr, TH_GS);
			}

			Skin::Skin(Engine::RenderSystem* Lab) : GeometryDraw(Lab, Components::Skin::GetTypeId())
			{
				Graphics::GraphicsDevice* Device = System->GetDevice();
				DepthStencil = Device->GetDepthStencilState("less");
				BackRasterizer = Device->GetRasterizerState("cull-back");
				FrontRasterizer = Device->GetRasterizerState("cull-front");
				Blend = Device->GetBlendState("overwrite");
				Sampler = Device->GetSamplerState("trilinear-x16");
				Layout = Device->GetInputLayout("skin-vertex");

				Shaders.Geometry = System->CompileShader("geometry/skin/geometry");
				Shaders.Voxelize = System->CompileShader("geometry/skin/voxelize", sizeof(Lighting::VoxelBuffer));
				Shaders.Occlusion = System->CompileShader("geometry/skin/occlusion");
				Shaders.Depth.Linear = System->CompileShader("geometry/skin/depth/linear");
				Shaders.Depth.Cubic = System->CompileShader("geometry/skin/depth/cubic", sizeof(Compute::Matrix4x4) * 6);
			}
			Skin::~Skin()
			{
				System->FreeShader(Shaders.Geometry);
				System->FreeShader(Shaders.Voxelize);
				System->FreeShader(Shaders.Occlusion);
				System->FreeShader(Shaders.Depth.Linear);
				System->FreeShader(Shaders.Depth.Cubic);
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
				Graphics::ElementBuffer* Box[2];
				System->GetPrimitives()->GetSkinBoxBuffers(Box);

				Graphics::GraphicsDevice* Device = System->GetDevice();
				Device->SetRasterizerState(BackRasterizer);
				Device->SetInputLayout(Layout);
				Device->SetShader(nullptr, TH_PS);
				Device->SetShader(Shaders.Occlusion, TH_VS);

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
						Device->SetIndexBuffer(Box[BufferType_Index], Graphics::Format_R32_Uint);
						Device->SetVertexBuffer(Box[BufferType_Vertex], 0);
						Device->DrawIndexed(Box[BufferType_Index]->GetElements(), 0, 0);
					}
					Base->End(Device);
				}
			}
			void Skin::RenderGeometryResult(Rest::Timer* Time, Rest::Pool<Drawable*>* Geometry, RenderOpt Options)
			{
				Graphics::GraphicsDevice* Device = System->GetDevice();
				CullResult Cull = (Options & RenderOpt_Inner ? CullResult_Always : CullResult_Last);
				bool Static = (Options & RenderOpt_Static);

				Device->SetDepthStencilState(DepthStencil);
				Device->SetBlendState(Blend);
				Device->SetRasterizerState(BackRasterizer);
				Device->SetInputLayout(Layout);
				Device->SetSamplerState(Sampler, 0, TH_PS);
				Device->SetShader(Shaders.Geometry, TH_VS | TH_PS);

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

				Device->FlushTexture2D(1, 7, TH_PS);
			}
			void Skin::RenderGeometryVoxels(Rest::Timer* Time, Rest::Pool<Drawable*>* Geometry, RenderOpt Options)
			{
				Graphics::GraphicsDevice* Device = System->GetDevice();
				Device->SetInputLayout(Layout);
				Device->SetSamplerState(Sampler, 0, TH_PS);
				Device->SetShader(Shaders.Voxelize, TH_VS | TH_PS | TH_GS);
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

				Device->FlushTexture2D(5, 6, TH_PS);
				Device->SetShader(nullptr, TH_GS);
			}
			void Skin::RenderDepthLinear(Rest::Timer* Time, Rest::Pool<Drawable*>* Geometry)
			{
				Graphics::GraphicsDevice* Device = System->GetDevice();
				Device->SetDepthStencilState(DepthStencil);
				Device->SetBlendState(Blend);
				Device->SetRasterizerState(FrontRasterizer);
				Device->SetInputLayout(Layout);
				Device->SetSamplerState(Sampler, 0, TH_PS);
				Device->SetShader(Shaders.Depth.Linear, TH_VS | TH_PS);

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

				Device->SetTexture2D(nullptr, 1, TH_PS);
			}
			void Skin::RenderDepthCubic(Rest::Timer* Time, Rest::Pool<Drawable*>* Geometry, Compute::Matrix4x4* ViewProjection)
			{
				Graphics::GraphicsDevice* Device = System->GetDevice();
				Device->SetDepthStencilState(DepthStencil);
				Device->SetBlendState(Blend);
				Device->SetRasterizerState(FrontRasterizer);
				Device->SetInputLayout(Layout);
				Device->SetSamplerState(Sampler, 0, TH_PS);
				Device->SetShader(Shaders.Depth.Linear, TH_VS | TH_PS | TH_GS);
				Device->SetBuffer(Shaders.Depth.Cubic, 3, TH_VS | TH_PS | TH_GS);
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

				Device->SetTexture2D(nullptr, 1, TH_PS);
				Device->SetShader(nullptr, TH_GS);
			}

			SoftBody::SoftBody(Engine::RenderSystem* Lab) : GeometryDraw(Lab, Components::SoftBody::GetTypeId()), VertexBuffer(nullptr), IndexBuffer(nullptr)
			{
				Graphics::GraphicsDevice* Device = System->GetDevice();
				DepthStencil = Device->GetDepthStencilState("less");
				Rasterizer = Device->GetRasterizerState("cull-none");
				Blend = Device->GetBlendState("overwrite");
				Sampler = Device->GetSamplerState("trilinear-x16");
				Layout = Device->GetInputLayout("vertex");

				Shaders.Geometry = System->CompileShader("geometry/model/geometry");
				Shaders.Voxelize = System->CompileShader("geometry/model/voxelize", sizeof(Lighting::VoxelBuffer));
				Shaders.Occlusion = System->CompileShader("geometry/model/occlusion");
				Shaders.Depth.Linear = System->CompileShader("geometry/model/depth/linear");
				Shaders.Depth.Cubic = System->CompileShader("geometry/model/depth/cubic", sizeof(Compute::Matrix4x4) * 6);

				Graphics::ElementBuffer* Buffers[2];
				if (Lab->CompileBuffers(Buffers, "soft-body", sizeof(Compute::Vertex), 16384))
				{
					IndexBuffer = Buffers[BufferType_Index];
					VertexBuffer = Buffers[BufferType_Vertex];
				}
			}
			SoftBody::~SoftBody()
			{
				Graphics::ElementBuffer* Buffers[2];
				Buffers[BufferType_Index] = IndexBuffer;
				Buffers[BufferType_Vertex] = VertexBuffer;

				System->FreeBuffers(Buffers);
				System->FreeShader(Shaders.Geometry);
				System->FreeShader(Shaders.Voxelize);
				System->FreeShader(Shaders.Occlusion);
				System->FreeShader(Shaders.Depth.Linear);
				System->FreeShader(Shaders.Depth.Cubic);
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
				Graphics::ElementBuffer* Box[2];
				System->GetPrimitives()->GetBoxBuffers(Box);

				Graphics::GraphicsDevice* Device = System->GetDevice();
				Device->SetRasterizerState(Rasterizer);
				Device->SetInputLayout(Layout);
				Device->SetShader(nullptr, TH_PS);
				Device->SetShader(Shaders.Occlusion, TH_VS);

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
						Device->DrawIndexed((unsigned int)Base->GetIndices().size(), 0, 0);
					}
					else
					{
						Device->Render.World = Base->GetEntity()->Transform->GetWorld();
						Device->Render.WorldViewProjection = Device->Render.World * View.ViewProjection;
						Device->UpdateBuffer(Graphics::RenderBufferType_Render);
						Device->SetIndexBuffer(Box[BufferType_Index], Graphics::Format_R32_Uint);
						Device->SetVertexBuffer(Box[BufferType_Vertex], 0);
						Device->DrawIndexed(Box[BufferType_Index]->GetElements(), 0, 0);
					}
					Base->End(Device);
				}
			}
			void SoftBody::RenderGeometryResult(Rest::Timer* Time, Rest::Pool<Drawable*>* Geometry, RenderOpt Options)
			{
				Graphics::GraphicsDevice* Device = System->GetDevice();
				CullResult Cull = (Options & RenderOpt_Inner ? CullResult_Always : CullResult_Last);
				bool Static = (Options & RenderOpt_Static);

				Device->SetDepthStencilState(DepthStencil);
				Device->SetBlendState(Blend);
				Device->SetRasterizerState(Rasterizer);
				Device->SetInputLayout(Layout);
				Device->SetSamplerState(Sampler, 0, TH_PS);
				Device->SetShader(Shaders.Geometry, TH_VS | TH_PS);

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

				Device->FlushTexture2D(1, 7, TH_PS);
			}
			void SoftBody::RenderGeometryVoxels(Rest::Timer* Time, Rest::Pool<Drawable*>* Geometry, RenderOpt Options)
			{
				Graphics::GraphicsDevice* Device = System->GetDevice();
				Device->SetSamplerState(Sampler, 0, TH_PS);
				Device->SetInputLayout(Layout);
				Device->SetShader(Shaders.Voxelize, TH_VS | TH_PS | TH_GS);
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

				Device->FlushTexture2D(5, 6, TH_PS);
				Device->SetShader(nullptr, TH_GS);
			}
			void SoftBody::RenderDepthLinear(Rest::Timer* Time, Rest::Pool<Drawable*>* Geometry)
			{
				Graphics::GraphicsDevice* Device = System->GetDevice();
				Device->SetDepthStencilState(DepthStencil);
				Device->SetBlendState(Blend);
				Device->SetRasterizerState(Rasterizer);
				Device->SetInputLayout(Layout);
				Device->SetSamplerState(Sampler, 0, TH_PS);
				Device->SetShader(Shaders.Depth.Linear, TH_VS | TH_PS);

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

				Device->SetTexture2D(nullptr, 1, TH_PS);
			}
			void SoftBody::RenderDepthCubic(Rest::Timer* Time, Rest::Pool<Drawable*>* Geometry, Compute::Matrix4x4* ViewProjection)
			{
				Graphics::GraphicsDevice* Device = System->GetDevice();
				Device->SetDepthStencilState(DepthStencil);
				Device->SetBlendState(Blend);
				Device->SetRasterizerState(Rasterizer);
				Device->SetInputLayout(Layout);
				Device->SetSamplerState(Sampler, 0, TH_PS);
				Device->SetShader(Shaders.Depth.Linear, TH_VS | TH_PS | TH_GS);
				Device->SetBuffer(Shaders.Depth.Cubic, 3, TH_VS | TH_PS | TH_GS);
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

				Device->SetTexture2D(nullptr, 1, TH_PS);
				Device->SetShader(nullptr, TH_GS);
			}

			Emitter::Emitter(RenderSystem* Lab) : GeometryDraw(Lab, Components::Emitter::GetTypeId())
			{
				Graphics::GraphicsDevice* Device = System->GetDevice();
				DepthStencilOpaque = Device->GetDepthStencilState("less");
				DepthStencilLimpid = Device->GetDepthStencilState("less-none");
				BackRasterizer = Device->GetRasterizerState("cull-back");
				FrontRasterizer = Device->GetRasterizerState("cull-front");
				AdditiveBlend = Device->GetBlendState("additive-alpha");
				OverwriteBlend = Device->GetBlendState("overwrite");
				Sampler = Device->GetSamplerState("trilinear-x16");

				Shaders.Opaque = System->CompileShader("geometry/emitter/geometry/opaque");
				Shaders.Transparency = System->CompileShader("geometry/emitter/geometry/transparency");
				Shaders.Depth.Linear = System->CompileShader("geometry/emitter/depth/linear");
				Shaders.Depth.Point = System->CompileShader("geometry/emitter/depth/point");
				Shaders.Depth.Quad = System->CompileShader("geometry/emitter/depth/quad", sizeof(Depth));
			}
			Emitter::~Emitter()
			{
				System->FreeShader(Shaders.Opaque);
				System->FreeShader(Shaders.Transparency);
				System->FreeShader(Shaders.Depth.Linear);
				System->FreeShader(Shaders.Depth.Point);
				System->FreeShader(Shaders.Depth.Quad);
			}
			void Emitter::Activate()
			{
				Opaque = System->AddCull<Engine::Components::Emitter>();
			}
			void Emitter::Deactivate()
			{
				System->RemoveCull<Engine::Components::Emitter>();
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

				Device->SetDepthStencilState(DepthStencilOpaque);
				Device->SetBlendState(OverwriteBlend);
				Device->SetRasterizerState(BackRasterizer);
				Device->SetInputLayout(Layout);
				Device->SetSamplerState(Sampler, 0, TH_PS);
				Device->SetShader(Shaders.Opaque, TH_VS | TH_PS);
				Device->SetVertexBuffer(nullptr, 0);
				Device->SetPrimitiveTopology(Graphics::PrimitiveTopology_Point_List);

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
					Device->SetBuffer(Base->GetBuffer(), 8, TH_VS | TH_PS);
					Device->SetShader(Base->QuadBased ? Shaders.Opaque : nullptr, TH_GS);
					Device->UpdateBuffer(Graphics::RenderBufferType_Render);
					Device->Draw((unsigned int)Base->GetBuffer()->GetArray()->Size(), 0);
				}

				Device->SetDepthStencilState(DepthStencilLimpid);
				Device->SetBlendState(AdditiveBlend);
				Device->SetShader(Shaders.Transparency, TH_VS | TH_PS);

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
					Device->SetBuffer(Base->GetBuffer(), 8, TH_VS | TH_PS);
					Device->SetShader(Base->QuadBased ? Shaders.Transparency : nullptr, TH_GS);
					Device->UpdateBuffer(Graphics::RenderBufferType_Render);
					Device->Draw((unsigned int)Base->GetBuffer()->GetArray()->Size(), 0);
				}

				Device->FlushTexture2D(1, 7, TH_PS);
				Device->SetShader(nullptr, TH_GS);
				Device->SetPrimitiveTopology(T);
			}
			void Emitter::RenderGeometryVoxels(Rest::Timer* Time, Rest::Pool<Drawable*>* Geometry, RenderOpt Options)
			{
			}
			void Emitter::RenderDepthLinear(Rest::Timer* Time, Rest::Pool<Drawable*>* Geometry)
			{
				Graphics::GraphicsDevice* Device = System->GetDevice();
				Graphics::PrimitiveTopology T = Device->GetPrimitiveTopology();
				Compute::Matrix4x4& View = System->GetScene()->View.View;

				Device->SetPrimitiveTopology(Graphics::PrimitiveTopology_Point_List);
				Device->SetDepthStencilState(DepthStencilLimpid);
				Device->SetBlendState(OverwriteBlend);
				Device->SetRasterizerState(FrontRasterizer);
				Device->SetInputLayout(Layout);
				Device->SetSamplerState(Sampler, 0, TH_PS);
				Device->SetShader(Shaders.Depth.Linear, TH_VS | TH_PS);
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

					Device->SetBuffer(Base->GetBuffer(), 8, TH_VS | TH_PS);
					Device->SetShader(Base->QuadBased ? Shaders.Depth.Linear : nullptr, TH_GS);
					Device->UpdateBuffer(Graphics::RenderBufferType_Render);
					Device->Draw((unsigned int)Base->GetBuffer()->GetArray()->Size(), 0);
				}

				Device->SetTexture2D(nullptr, 1, TH_PS);
				Device->SetShader(nullptr, TH_GS);
				Device->SetPrimitiveTopology(T);
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
				Device->SetDepthStencilState(DepthStencilLimpid);
				Device->SetBlendState(OverwriteBlend);
				Device->SetRasterizerState(FrontRasterizer);
				Device->SetInputLayout(Layout);
				Device->SetPrimitiveTopology(Graphics::PrimitiveTopology_Point_List);
				Device->SetVertexBuffer(nullptr, 0);
				Device->SetSamplerState(Sampler, 0, TH_PS);
				Device->SetBuffer(Shaders.Depth.Quad, 3, TH_VS | TH_PS | TH_GS);
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
					Device->SetBuffer(Base->GetBuffer(), 8, TH_VS | TH_PS);
					Device->SetShader(Base->QuadBased ? Shaders.Depth.Quad : Shaders.Depth.Point, TH_VS | TH_PS | TH_GS);
					Device->UpdateBuffer(Graphics::RenderBufferType_Render);
					Device->Draw((unsigned int)Base->GetBuffer()->GetArray()->Size(), 0);
				}

				Device->SetTexture2D(nullptr, 1, TH_PS);
				Device->SetShader(nullptr, TH_GS);
				Device->SetPrimitiveTopology(T);
			}

			Decal::Decal(RenderSystem* Lab) : GeometryDraw(Lab, Components::Decal::GetTypeId())
			{
				Graphics::GraphicsDevice* Device = System->GetDevice();
				DepthStencil = Device->GetDepthStencilState("none");
				Rasterizer = Device->GetRasterizerState("cull-back");
				Blend = Device->GetBlendState("additive");
				Sampler = Device->GetSamplerState("trilinear-x16");
				Layout = Device->GetInputLayout("shape-vertex");

				Shader = System->CompileShader("geometry/decal/geometry", sizeof(RenderPass));
			}
			Decal::~Decal()
			{
				System->FreeShader(Shader);
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
				Graphics::MultiRenderTarget2D* MRT = System->GetMRT(TargetType_Main);
				Graphics::GraphicsDevice* Device = System->GetDevice();
				Rest::Pool<Component*>* Array = (Options & RenderOpt_Transparent ? Limpid : Opaque);
				CullResult Cull = (Options & RenderOpt_Inner ? CullResult_Always : CullResult_Last);
				bool Map[8] = { true, true, false, true, false, false, false, false };
				bool Static = (Options & RenderOpt_Static);

				if (!Array || Geometry->Empty())
					return;

				Graphics::ElementBuffer* Sphere[2];
				System->GetPrimitives()->GetSphereBuffers(Sphere);

				Device->SetDepthStencilState(DepthStencil);
				Device->SetBlendState(Blend);
				Device->SetRasterizerState(Rasterizer);
				Device->SetInputLayout(Layout);
				Device->SetTargetMap(MRT, Map);
				Device->SetSamplerState(Sampler, 0, TH_PS);
				Device->SetShader(Shader, TH_VS | TH_PS);
				Device->SetBuffer(Shader, 3, TH_VS | TH_PS);
				Device->SetTexture2D(MRT->GetTarget(2), 8, TH_PS);
				Device->SetIndexBuffer(Sphere[BufferType_Index], Graphics::Format_R32_Uint);
				Device->SetVertexBuffer(Sphere[BufferType_Vertex], 0);

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
					Device->DrawIndexed((unsigned int)Sphere[BufferType_Index]->GetElements(), 0, 0);
				}

				Device->FlushTexture2D(1, 8, TH_PS);
				System->RestoreOutput();
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

			Lighting::Lighting(RenderSystem* Lab) : Renderer(Lab), EnableGI(true)
			{
				Graphics::GraphicsDevice* Device = System->GetDevice();
				DepthStencilNone = Device->GetDepthStencilState("none");
				DepthStencilGreater = Device->GetDepthStencilState("greater-read-only");
				DepthStencilLess = Device->GetDepthStencilState("less-read-only");
				FrontRasterizer = Device->GetRasterizerState("cull-front");
				BackRasterizer = Device->GetRasterizerState("cull-back");
				NoneRasterizer = Device->GetRasterizerState("cull-none");
				BlendAdditive = Device->GetBlendState("additive-opaque");
				BlendOverwrite = Device->GetBlendState("overwrite-colorless");
				BlendOverload = Device->GetBlendState("overwrite");
				ShadowSampler = Device->GetSamplerState("shadow");
				WrapSampler = Device->GetSamplerState("trilinear-x16");
				Layout = Device->GetInputLayout("shape-vertex");

				Shaders.Ambient[0] = System->CompileShader("lighting/ambient/direct", sizeof(IAmbientLight));
				Shaders.Ambient[1] = System->CompileShader("lighting/ambient/indirect", sizeof(IVoxelBuffer));
				Shaders.Point[0] = System->CompileShader("lighting/point/low", sizeof(IPointLight));
				Shaders.Point[1] = System->CompileShader("lighting/point/high");
				Shaders.Spot[0] = System->CompileShader("lighting/spot/low", sizeof(ISpotLight));
				Shaders.Spot[1] = System->CompileShader("lighting/spot/high");
				Shaders.Line[0] = System->CompileShader("lighting/line/low", sizeof(ILineLight));
				Shaders.Line[1] = System->CompileShader("lighting/line/high");
				Shaders.Voxelize = System->CompileShader("lighting/voxelize", sizeof(IVoxelBuffer));
				Shaders.Surface = System->CompileShader("lighting/surface", sizeof(ISurfaceLight));

				Shadows.Tick.Delay = 5;
			}
			Lighting::~Lighting()
			{
				FlushDepthBuffersAndCache();
				System->FreeShader(Shaders.Voxelize);
				System->FreeShader(Shaders.Surface);

				for (unsigned int i = 0; i < 2; i++)
				{
					System->FreeShader(Shaders.Line[i]);
					System->FreeShader(Shaders.Spot[i]);
					System->FreeShader(Shaders.Point[i]);
					System->FreeShader(Shaders.Ambient[i]);
				}

				TH_RELEASE(Storage.PBuffer);
				TH_RELEASE(Storage.SBuffer);
				TH_RELEASE(Storage.LBuffer);
				TH_RELEASE(Surfaces.Subresource);
				TH_RELEASE(Surfaces.Input);
				TH_RELEASE(Surfaces.Output);
				TH_RELEASE(Surfaces.Merger);
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
				NMake::Unpack(Node->Find("gi"), &EnableGI);
			}
			void Lighting::Serialize(ContentManager* Content, Rest::Document* Node)
			{
				AssetCache* Asset = Content->Find<Graphics::Texture2D>(SkyBase);
				if (Asset != nullptr)
					NMake::Pack(Node->Set("sky-map"), Asset->Path);

				NMake::Pack(Node->Set("high-emission"), AmbientLight.HighEmission);
				NMake::Pack(Node->Set("low-emission"), AmbientLight.LowEmission);
				NMake::Pack(Node->Set("sky-emission"), AmbientLight.SkyEmission);
				NMake::Pack(Node->Set("light-emission"), AmbientLight.LightEmission);
				NMake::Pack(Node->Set("sky-color"), AmbientLight.SkyColor);
				NMake::Pack(Node->Set("fog-color"), AmbientLight.FogColor);
				NMake::Pack(Node->Set("fog-amount"), AmbientLight.FogAmount);
				NMake::Pack(Node->Set("fog-far-off"), AmbientLight.FogFarOff);
				NMake::Pack(Node->Set("fog-far"), AmbientLight.FogFar);
				NMake::Pack(Node->Set("fog-near-off"), AmbientLight.FogNearOff);
				NMake::Pack(Node->Set("fog-near"), AmbientLight.FogNear);
				NMake::Pack(Node->Set("recursive"), AmbientLight.Recursive);
				NMake::Pack(Node->Set("point-light-resolution"), Shadows.PointLightResolution);
				NMake::Pack(Node->Set("point-light-limits"), Shadows.PointLightLimits);
				NMake::Pack(Node->Set("spot-light-resolution"), Shadows.SpotLightResolution);
				NMake::Pack(Node->Set("spot-light-limits"), Shadows.SpotLightLimits);
				NMake::Pack(Node->Set("line-light-resolution"), Shadows.LineLightResolution);
				NMake::Pack(Node->Set("line-light-limits"), Shadows.LineLightLimits);
				NMake::Pack(Node->Set("shadow-distance"), Shadows.Distance);
				NMake::Pack(Node->Set("sf-size"), Surfaces.Size);
				NMake::Pack(Node->Set("gi"), EnableGI);
			}
			void Lighting::Activate()
			{
				Illuminators = System->AddCull<Engine::Components::Illuminator>();
				SurfaceLights = System->AddCull<Engine::Components::SurfaceLight>();
				PointLights = System->AddCull<Engine::Components::PointLight>();
				SpotLights = System->AddCull<Engine::Components::SpotLight>();
				LineLights = System->GetScene()->GetComponents<Engine::Components::LineLight>();
			}
			void Lighting::Deactivate()
			{
				System->RemoveCull<Engine::Components::Illuminator>();
				System->RemoveCull<Engine::Components::SurfaceLight>();
				System->RemoveCull<Engine::Components::PointLight>();
				System->RemoveCull<Engine::Components::SpotLight>();
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
			}
			Component* Lighting::GetIlluminator(Rest::Timer* Time)
			{
				Engine::Components::Illuminator* Src = nullptr;
				float Distance = std::numeric_limits<float>::max();

				for (auto It = Illuminators->Begin(); It != Illuminators->End(); It++)
				{
					Engine::Components::Illuminator* Area = (Engine::Components::Illuminator*)*It;
					float Subdistance = Area->GetEntity()->Distance;
					if (Subdistance < Distance)
					{
						Distance = Subdistance;
						if (System->PassCullable(Area, CullResult_Always, nullptr))
							Src = Area;
					}
				}

				if (!Src)
				{
					LightBuffer = nullptr;
					return nullptr;
				}

				if (!Src->GetBuffer())
					Src->SetBufferSize(Src->GetBufferSize());

				LightBuffer = Src->GetBuffer();
				if (!Src->Tick.TickEvent(Time->GetElapsedTime()))
					return nullptr;

				Entity* Base = Src->GetEntity();
				VoxelBuffer.Center = Base->Transform->Position.InvertZ();
				VoxelBuffer.Scale = Base->Transform->Scale;
				VoxelBuffer.MipLevels = (float)Src->GetMipLevels();
				VoxelBuffer.Size = (float)Src->GetBufferSize();
				VoxelBuffer.RayStep = Src->RayStep;
				VoxelBuffer.MaxSteps = Src->MaxSteps;
				VoxelBuffer.Distance = Src->Distance;
				VoxelBuffer.Intensity = Src->Intensity;
				VoxelBuffer.Occlusion = Src->Occlusion;
				VoxelBuffer.Shadows = Src->Shadows;
				VoxelBuffer.Bleeding = Src->Bleeding;

				return Src;
			}
			float Lighting::GetDominant(const Compute::Vector3& Axis)
			{
				float Max = Axis.X;
				if (Axis.Y > Max)
					Max = Axis.Y;

				if (Axis.Z > Max)
					Max = Axis.Z;

				return Max;
			}
			bool Lighting::GetSurfaceLight(ISurfaceLight* Dest, Component* Src, Compute::Vector3& Position, Compute::Vector3& Scale)
			{
				Engine::Components::SurfaceLight* Light = (Engine::Components::SurfaceLight*)Src;
				Dest->WorldViewProjection = Compute::Matrix4x4::CreateTranslatedScale(Position, Scale) * State.Scene->View.ViewProjection;
				Dest->Position = Light->GetEntity()->Transform->Position.InvertZ();
				Dest->Lighting = Light->Diffuse.Mul(Light->Emission * State.Distance);
				Dest->Scale = Light->GetEntity()->Transform->Scale;
				Dest->Parallax = (Light->Parallax ? 1.0f : 0.0f);
				Dest->Infinity = Light->Infinity;
				Dest->Attenuation.X = Light->Size.C1;
				Dest->Attenuation.Y = Light->Size.C2;
				Dest->Range = Light->Size.Range;

				return true;
			}
			bool Lighting::GetPointLight(IPointLight* Dest, Component* Src, Compute::Vector3& Position, Compute::Vector3& Scale)
			{
				Engine::Components::PointLight* Light = (Engine::Components::PointLight*)Src;
				Dest->WorldViewProjection = Compute::Matrix4x4::CreateTranslatedScale(Position, Scale) * System->GetScene()->View.ViewProjection;
				Dest->Position = Light->GetEntity()->Transform->Position.InvertZ();
				Dest->Lighting = Light->Diffuse.Mul(Light->Emission * State.Distance);
				Dest->Attenuation.X = Light->Size.C1;
				Dest->Attenuation.Y = Light->Size.C2;
				Dest->Range = Light->Size.Range;

				if (!Light->Shadow.Enabled || !Light->DepthMap)
				{
					Dest->Softness = 0.0f;
					return false;
				}

				Dest->Softness = Light->Shadow.Softness <= 0 ? 0 : (float)Shadows.PointLightResolution / Light->Shadow.Softness;
				Dest->Bias = Light->Shadow.Bias;
				Dest->Distance = Light->Shadow.Distance;
				Dest->Iterations = (float)Light->Shadow.Iterations;
				Dest->Umbra = Light->Disperse;
				return true;
			}
			bool Lighting::GetSpotLight(ISpotLight* Dest, Component* Src, Compute::Vector3& Position, Compute::Vector3& Scale)
			{
				Engine::Components::SpotLight* Light = (Engine::Components::SpotLight*)Src;
				Dest->WorldViewProjection = Compute::Matrix4x4::CreateTranslatedScale(Position, Scale) * State.Scene->View.ViewProjection;
				Dest->ViewProjection = Light->View * Light->Projection;
				Dest->Direction = Light->GetEntity()->Transform->Rotation.DepthDirection();
				Dest->Position = Light->GetEntity()->Transform->Position.InvertZ();
				Dest->Lighting = Light->Diffuse.Mul(Light->Emission * State.Distance);
				Dest->Cutoff = Compute::Mathf::Cos(Compute::Mathf::Deg2Rad() * Light->Cutoff * 0.5f);
				Dest->Attenuation.X = Light->Size.C1;
				Dest->Attenuation.Y = Light->Size.C2;
				Dest->Range = Light->Size.Range;

				if (!Light->Shadow.Enabled || !Light->DepthMap)
				{
					Dest->Softness = 0.0f;
					return false;
				}

				Dest->Softness = Light->Shadow.Softness <= 0 ? 0 : (float)Shadows.SpotLightResolution / Light->Shadow.Softness;
				Dest->Bias = Light->Shadow.Bias;
				Dest->Iterations = (float)Light->Shadow.Iterations;
				Dest->Umbra = Light->Disperse;
				return true;
			}
			bool Lighting::GetLineLight(ILineLight* Dest, Component* Src)
			{
				Engine::Components::LineLight* Light = (Engine::Components::LineLight*)Src;
				Dest->Position = Light->GetEntity()->Transform->Position.InvertZ().NormalizeSafe();
				Dest->Lighting = Light->Diffuse.Mul(Light->Emission);
				Dest->RlhEmission = Light->Sky.RlhEmission;
				Dest->RlhHeight = Light->Sky.RlhHeight;
				Dest->MieEmission = Light->Sky.MieEmission;
				Dest->MieHeight = Light->Sky.MieHeight;
				Dest->ScatterIntensity = Light->Sky.Intensity;
				Dest->PlanetRadius = Light->Sky.InnerRadius;
				Dest->AtmosphereRadius = Light->Sky.OuterRadius;
				Dest->MieDirection = Light->Sky.MieDirection;
				Dest->SkyOffset = AmbientLight.SkyOffset;

				if (!Light->Shadow.Enabled || !Light->DepthMap)
				{
					Dest->Softness = 0.0f;
					return false;
				}

				Dest->Softness = Light->Shadow.Softness <= 0 ? 0 : (float)Shadows.LineLightResolution / Light->Shadow.Softness;
				Dest->Iterations = (float)Light->Shadow.Iterations;
				Dest->Bias = Light->Shadow.Bias;
				Dest->Cascades = Light->DepthMap->size();
				Dest->Umbra = Light->Disperse;

				for (size_t i = 0; i < Light->DepthMap->size(); i++)
					Dest->ViewProjection[i] = Light->View[i] * Light->Projection[i];

				return true;
			}
			void Lighting::GetLightCulling(Component* Src, float Range, Compute::Vector3* Position, Compute::Vector3* Scale)
			{
				*Position = Src->GetEntity()->Transform->Position;
				*Scale = (Range > 0.0f ? Range : Src->GetEntity()->Transform->Scale);
				
				bool Front = Compute::Common::HasPointIntersectedCube(*Position, Scale->Mul(1.01f), State.View);
				if (!(Front && State.Backcull) && !(!Front && !State.Backcull))
					return;

				State.Device->SetRasterizerState(Front ? FrontRasterizer : BackRasterizer);
				State.Device->SetDepthStencilState(Front ? DepthStencilGreater : DepthStencilLess);
				State.Backcull = !State.Backcull;
			}
			void Lighting::GenerateLightBuffers()
			{
				Graphics::ElementBuffer::Desc F = Graphics::ElementBuffer::Desc();
				F.AccessFlags = Graphics::CPUAccess_Write;
				F.MiscFlags = Graphics::ResourceMisc_Buffer_Structured;
				F.Usage = Graphics::ResourceUsage_Dynamic;
				F.BindFlags = Graphics::ResourceBind_Shader_Input;
				F.ElementCount = (unsigned int)Storage.MaxLights;
				F.ElementWidth = sizeof(IPointLight);
				F.StructureByteStride = F.ElementWidth;

				TH_RELEASE(Storage.PBuffer);
				Storage.PBuffer = State.Device->CreateElementBuffer(F);
				Storage.PArray.resize(Storage.MaxLights);

				F.ElementWidth = sizeof(ISpotLight);
				F.StructureByteStride = F.ElementWidth;
				TH_RELEASE(Storage.SBuffer);
				Storage.SBuffer = State.Device->CreateElementBuffer(F);
				Storage.SArray.resize(Storage.MaxLights);

				F.ElementWidth = sizeof(ILineLight);
				F.StructureByteStride = F.ElementWidth;
				TH_RELEASE(Storage.LBuffer);
				Storage.LBuffer = State.Device->CreateElementBuffer(F);
				Storage.LArray.resize(Storage.MaxLights);
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
			size_t Lighting::GeneratePointLights()
			{
				if (!Storage.PBuffer)
					GenerateLightBuffers();

				size_t Count = 0;
				for (auto It = PointLights->Begin(); It != PointLights->End(); ++It)
				{
					Engine::Components::PointLight* Light = (Engine::Components::PointLight*)*It;
					if (Count >= Storage.MaxLights || !Light->IsNear(State.Scene->View))
						continue;

					Compute::Vector3 Position(Light->GetEntity()->Transform->Position), Scale(Light->GetBoxRange());
					GetPointLight(&Storage.PArray[Count], Light, Position, Scale); Count++;
				}

				VoxelBuffer.Lights.X = (float)Count;
				return Count;
			}
			size_t Lighting::GenerateSpotLights()
			{
				if (!Storage.SBuffer)
					GenerateLightBuffers();

				size_t Count = 0;
				for (auto It = SpotLights->Begin(); It != SpotLights->End(); ++It)
				{
					Engine::Components::SpotLight* Light = (Engine::Components::SpotLight*)*It;
					if (Count >= Storage.MaxLights || !Light->IsNear(State.Scene->View))
						continue;

					Compute::Vector3 Position(Light->GetEntity()->Transform->Position), Scale(Light->GetBoxRange());
					GetSpotLight(&Storage.SArray[Count], Light, Position, Scale); Count++;
				}

				VoxelBuffer.Lights.Y = (float)Count;
				return Count;
			}
			size_t Lighting::GenerateLineLights()
			{
				if (!Storage.LBuffer)
					GenerateLightBuffers();

				size_t Count = 0;
				for (auto It = LineLights->Begin(); It != LineLights->End(); ++It)
				{
					Engine::Components::LineLight* Light = (Engine::Components::LineLight*)*It;
					if (Count >= Storage.MaxLights)
						continue;

					GetLineLight(&Storage.LArray[Count], Light);
					Count++;
				}

				VoxelBuffer.Lights.Z = (float)Count;
				return Count;
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
					TH_RELEASE(*It);

				for (auto It = Shadows.SpotLight.begin(); It != Shadows.SpotLight.end(); It++)
					TH_RELEASE(*It);

				for (auto It = Shadows.LineLight.begin(); It != Shadows.LineLight.end(); It++)
				{
					if (*It != nullptr)
					{
						for (auto* Target : *(*It))
							TH_RELEASE(Target);
					}

					delete *It;
				}
			}
			void Lighting::Render(Rest::Timer* Time, RenderState Status, RenderOpt Options)
			{
				State.Device = System->GetDevice();
				State.Scene = System->GetScene();

				if (Status == RenderState_Geometry_Voxels)
					return RenderLuminance();
				else if (Status != RenderState_Geometry_Result)
					return;

				if (!(Options & RenderOpt_Inner || Options & RenderOpt_Transparent))
				{
					double ElapsedTime = Time->GetElapsedTime();
					if (Shadows.Tick.TickEvent(ElapsedTime))
						RenderShadowMaps(Time);
					else
						RenderSurfaceMaps(Time);

					if (EnableGI && !Illuminators->Empty())
						RenderVoxelMap(Time);
				}

				RenderResultBuffers(Options);
				System->RestoreOutput();
			}
			void Lighting::RenderResultBuffers(RenderOpt Options)
			{
				State.View = State.Scene->View.WorldPosition;
				State.Distance = 0.0f;
				State.Inner = (Options & RenderOpt_Inner);
				State.Backcull = true;

				Graphics::MultiRenderTarget2D* MRT = System->GetMRT(TargetType_Main);
				Graphics::RenderTarget2D* RT = (State.Inner ? Surfaces.Input : System->GetRT(TargetType_Main));

				Graphics::ElementBuffer* Cube[2];
				System->GetPrimitives()->GetCubeBuffers(Cube);

				AmbientLight.SkyOffset = System->GetScene()->View.Projection.Invert() * Compute::Matrix4x4::CreateRotation(System->GetScene()->View.WorldRotation);
				State.Device->SetDepthStencilState(DepthStencilLess);
				State.Device->SetBlendState(BlendAdditive);
				State.Device->SetRasterizerState(BackRasterizer);
				State.Device->SetInputLayout(Layout);
				State.Device->CopyTarget(MRT, 0, RT, 0);
				State.Device->SetTarget(MRT, 0, 0, 0, 0);
				State.Device->SetSamplerState(WrapSampler, 0, TH_PS);
				State.Device->SetSamplerState(ShadowSampler, 1, TH_PS);
				State.Device->SetTexture2D(RT->GetTarget(0), 1, TH_PS);
				State.Device->SetTexture2D(MRT->GetTarget(1), 2, TH_PS);
				State.Device->SetTexture2D(MRT->GetTarget(2), 3, TH_PS);
				State.Device->SetTexture2D(MRT->GetTarget(3), 4, TH_PS);
				State.Device->SetIndexBuffer(Cube[BufferType_Index], Graphics::Format_R32_Uint);
				State.Device->SetVertexBuffer(Cube[BufferType_Vertex], 0);

				RenderSurfaceLights();
				RenderPointLights();
				RenderSpotLights();
				RenderLineLights();
				RenderIllumination();
				RenderAmbient();

				State.Device->FlushTexture2D(1, 10, TH_PS);
			}
			void Lighting::RenderShadowMaps(Rest::Timer* Time)
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
					State.Device->SetTarget(Target, 0, 0, 0, 0);
					State.Device->ClearDepth(Target);

					Light->AssembleDepthOrigin();
					State.Scene->SetView(Compute::Matrix4x4::Identity(), Light->Projection, Light->GetEntity()->Transform->Position, 0.1f, Light->Shadow.Distance, true);
					State.Scene->Render(Time, RenderState_Depth_Cubic, RenderOpt_Inner);

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
					State.Device->SetTarget(Target, 0, 0, 0, 0);
					State.Device->ClearDepth(Target);

					Light->AssembleDepthOrigin();
					State.Scene->SetView(Light->View, Light->Projection, Light->GetEntity()->Transform->Position, 0.1f, Light->Shadow.Distance, true);
					State.Scene->Render(Time, RenderState_Depth_Linear, RenderOpt_Inner);

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
						State.Device->SetTarget(Cascade, 0, 0, 0, 0);
						State.Device->ClearDepth(Cascade);

						State.Scene->SetView(Light->View[i], Light->Projection[i], 0.0f, 0.1f, Light->Shadow.Distance[i], true);
						State.Scene->Render(Time, RenderState_Depth_Linear, RenderOpt_Inner);
					}

					Light->DepthMap = Target;
					Counter++;
				}

				State.Device->FlushTexture2D(1, 8, TH_PS);
				State.Scene->RestoreViewBuffer(nullptr);
			}
			void Lighting::RenderSurfaceMaps(Rest::Timer* Time)
			{
				if (SurfaceLights->Empty())
					return;

				if (!Surfaces.Merger || !Surfaces.Subresource || !Surfaces.Input || !Surfaces.Output)
					SetSurfaceBufferSize(Surfaces.Size);

				State.Scene->SwapMRT(TargetType_Main, Surfaces.Merger);
				State.Scene->SetMRT(TargetType_Main, false);

				double ElapsedTime = Time->GetElapsedTime();
				for (auto It = SurfaceLights->Begin(); It != SurfaceLights->End(); It++)
				{
					Engine::Components::SurfaceLight* Light = (Engine::Components::SurfaceLight*)*It;
					if (Light->IsImageBased() || !System->PassCullable(Light, CullResult_Always, nullptr))
						continue;

					Graphics::TextureCube* Cache = Light->GetProbeCache();
					if (!Cache)
					{
						Cache = State.Device->CreateTextureCube();
						Light->SetProbeCache(Cache);
					}
					else if (!Light->Tick.TickEvent(ElapsedTime) || Light->Tick.Delay <= 0.0)
						continue;

					State.Device->CubemapBegin(Surfaces.Subresource);
					Light->Locked = true;

					Compute::Vector3 Position = Light->GetEntity()->Transform->Position * Light->Offset;
					for (unsigned int j = 0; j < 6; j++)
					{
						Light->View[j] = Compute::Matrix4x4::CreateCubeMapLookAt(j, Position);
						State.Scene->SetView(Light->View[j], Light->Projection, Position, 0.1f, Light->Size.Range, true);
						State.Scene->ClearMRT(TargetType_Main, true, true);
						State.Scene->Render(Time, RenderState_Geometry_Result, Light->StaticMask ? RenderOpt_Inner | RenderOpt_Static : RenderOpt_Inner);
						State.Device->CubemapFace(Surfaces.Subresource, 0, j);
					}

					Light->Locked = false;
					State.Device->CubemapEnd(Surfaces.Subresource, Cache);
				}

				State.Scene->SwapMRT(TargetType_Main, nullptr);
				State.Scene->RestoreViewBuffer(nullptr);
			}
			void Lighting::RenderVoxelMap(Rest::Timer* Time)
			{
				Components::Illuminator* Area = (Components::Illuminator*)GetIlluminator(Time);
				if (!Area || !LightBuffer)
					return;

				bool Inside = Compute::Common::HasPointIntersectedCube(VoxelBuffer.Center, VoxelBuffer.Scale, State.Scene->View.InvViewPosition);
				if (!Inside && Storage.Area == Area && Storage.Inside == Inside)
					return;

				//if (!Application::Get()->Activity->IsKeyDown(Graphics::KeyCode_0))
					//return;

				Graphics::Texture3D* In[3], *Out[3];
				if (!State.Scene->GetVoxelBuffer(In, Out))
					return;

				size_t Size = State.Scene->GetVoxelBufferSize();
				State.Device->ClearWritable(In[VoxelType_Diffuse]);
				State.Device->ClearWritable(In[VoxelType_Normal]);
				State.Device->ClearWritable(In[VoxelType_Surface]);
				State.Device->SetTargetRect(Size, Size);
				State.Device->SetDepthStencilState(DepthStencilNone);
				State.Device->SetBlendState(BlendOverwrite);
				State.Device->SetRasterizerState(NoneRasterizer);
				State.Device->SetWriteable(In, 3, 1, false);

				if (Inside)
				{
					State.Scene->View.FarPlane = GetDominant(VoxelBuffer.Scale) * 2.0f;
					State.Scene->Render(Time, RenderState_Geometry_Voxels, RenderOpt_Inner);
					State.Scene->RestoreViewBuffer(nullptr);
				}
				else
					State.Scene->Render(Time, RenderState_Geometry_Voxels, RenderOpt_Inner);

				State.Device->SetWriteable(Out, 3, 1, false);
				State.Device->GenerateMips(LightBuffer);
				Storage.Inside = Inside;
				Storage.Area = Area;
			}
			void Lighting::RenderLuminance()
			{
				Graphics::Texture3D* In[3], *Out[3];
				if (!LightBuffer || !State.Scene->GetVoxelBuffer(In, Out))
					return;

				uint32_t X = (uint32_t)(VoxelBuffer.Size.X / 8.0f);
				uint32_t Y = (uint32_t)(VoxelBuffer.Size.Y / 8.0f);
				uint32_t Z = (uint32_t)(VoxelBuffer.Size.Z / 8.0f);

				State.Distance = 1.0;
				State.Device->ClearWritable(LightBuffer);
				State.Device->SetWriteable(Out, 3, 1, false);
				State.Device->SetWriteable(&LightBuffer, 1, 1, true);
				State.Device->SetTexture3D(In[VoxelType_Diffuse], 2, TH_CS);
				State.Device->SetTexture3D(In[VoxelType_Normal], 3, TH_CS);
				State.Device->SetTexture3D(In[VoxelType_Surface], 4, TH_CS);

				size_t PointLightsCount = GeneratePointLights();
				if (PointLightsCount > 0)
				{
					State.Device->UpdateBuffer(Storage.PBuffer, Storage.PArray.data(), PointLightsCount * sizeof(IPointLight));
					State.Device->SetStructureBuffer(Storage.PBuffer, 5, TH_CS);
				}

				size_t SpotLightsCount = GenerateSpotLights();
				if (SpotLightsCount > 0)
				{
					State.Device->UpdateBuffer(Storage.SBuffer, Storage.SArray.data(), SpotLightsCount * sizeof(ISpotLight));
					State.Device->SetStructureBuffer(Storage.SBuffer, 6, TH_CS);
				}

				size_t LineLightsCount = GenerateLineLights();
				if (LineLightsCount > 0)
				{
					State.Device->UpdateBuffer(Storage.LBuffer, Storage.LArray.data(), LineLightsCount * sizeof(ILineLight));
					State.Device->SetStructureBuffer(Storage.LBuffer, 7, TH_CS);
				}

				State.Device->UpdateBuffer(Shaders.Voxelize, &VoxelBuffer);
				State.Device->SetBuffer(Shaders.Voxelize, 3, TH_CS);
				State.Device->SetShader(Shaders.Voxelize, TH_CS);
				State.Device->Dispatch(X, Y, Z);
				State.Device->FlushTexture3D(2, 3, TH_CS);
				State.Device->SetShader(nullptr, TH_CS);
				State.Device->SetStructureBuffer(nullptr, 5, TH_CS);
				State.Device->SetStructureBuffer(nullptr, 6, TH_CS);
				State.Device->SetStructureBuffer(nullptr, 7, TH_CS);
				State.Device->SetWriteable(Out, 1, 1, true);
				State.Device->SetWriteable(In, 3, 1, false);
			}
			void Lighting::RenderSurfaceLights()
			{
				Graphics::ElementBuffer* Cube[2];
				System->GetPrimitives()->GetCubeBuffers(Cube);

				if (!State.Inner)
				{
					State.Device->SetShader(Shaders.Surface, TH_VS | TH_PS);
					State.Device->SetBuffer(Shaders.Surface, 3, TH_VS | TH_PS);

					Compute::Vector3 Position, Scale;
					for (auto It = SurfaceLights->Begin(); It != SurfaceLights->End(); ++It)
					{
						Engine::Components::SurfaceLight* Light = (Engine::Components::SurfaceLight*)*It;
						if (!Light->GetProbeCache() || !System->PassCullable(Light, CullResult_Always, &State.Distance))
							continue;

						GetLightCulling(Light, Light->GetBoxRange(), &Position, &Scale);
						GetSurfaceLight(&SurfaceLight, Light, Position, Scale);

						State.Device->SetTextureCube(Light->GetProbeCache(), 5, TH_PS);
						State.Device->UpdateBuffer(Shaders.Surface, &SurfaceLight);
						State.Device->DrawIndexed((unsigned int)Cube[BufferType_Index]->GetElements(), 0, 0);
					}
				}
				else if (AmbientLight.Recursive > 0.0f)
				{
					State.Device->SetShader(Shaders.Surface, TH_VS | TH_PS);
					State.Device->SetBuffer(Shaders.Surface, 3, TH_VS | TH_PS);

					Compute::Vector3 Position, Scale;
					for (auto It = SurfaceLights->Begin(); It != SurfaceLights->End(); ++It)
					{
						Engine::Components::SurfaceLight* Light = (Engine::Components::SurfaceLight*)*It;
						if (Light->Locked || !Light->GetProbeCache() || !System->PassCullable(Light, CullResult_Always, &State.Distance))
							continue;

						GetLightCulling(Light, Light->GetBoxRange(), &Position, &Scale);
						GetSurfaceLight(&SurfaceLight, Light, Position, Scale);

						State.Device->SetTextureCube(Light->GetProbeCache(), 5, TH_PS);
						State.Device->UpdateBuffer(Shaders.Surface, &SurfaceLight);
						State.Device->DrawIndexed((unsigned int)Cube[BufferType_Index]->GetElements(), 0, 0);
					}
				}
			}
			void Lighting::RenderPointLights()
			{
				Graphics::ElementBuffer* Cube[2];
				System->GetPrimitives()->GetCubeBuffers(Cube);

				Graphics::Shader* Active = nullptr;
				State.Device->SetShader(Shaders.Point[0], TH_VS);
				State.Device->SetBuffer(Shaders.Point[0], 3, TH_VS | TH_PS);

				Compute::Vector3 Position, Scale;
				for (auto It = PointLights->Begin(); It != PointLights->End(); ++It)
				{
					Engine::Components::PointLight* Light = (Engine::Components::PointLight*)*It;
					if (!System->PassCullable(Light, CullResult_Always, &State.Distance))
						continue;

					GetLightCulling(Light, Light->GetBoxRange(), &Position, &Scale);
					if (GetPointLight(&PointLight, Light, Position, Scale))
					{
						Active = Shaders.Point[1];
						State.Device->SetTexture2D(Light->DepthMap->GetTarget(0), 5, TH_PS);
					}
					else
						Active = Shaders.Point[0];

					State.Device->SetShader(Active, TH_PS);
					State.Device->UpdateBuffer(Shaders.Point[0], &PointLight);
					State.Device->DrawIndexed((unsigned int)Cube[BufferType_Index]->GetElements(), 0, 0);
				}
			}
			void Lighting::RenderSpotLights()
			{
				Graphics::ElementBuffer* Cube[2];
				System->GetPrimitives()->GetCubeBuffers(Cube);

				Graphics::Shader* Active = nullptr;
				State.Device->SetShader(Shaders.Spot[0], TH_VS);
				State.Device->SetBuffer(Shaders.Spot[0], 3, TH_VS | TH_PS);

				Compute::Vector3 Position, Scale;
				for (auto It = SpotLights->Begin(); It != SpotLights->End(); ++It)
				{
					Engine::Components::SpotLight* Light = (Engine::Components::SpotLight*)*It;
					if (!System->PassCullable(Light, CullResult_Always, &State.Distance))
						continue;

					GetLightCulling(Light, Light->GetBoxRange(), &Position, &Scale);
					if (GetSpotLight(&SpotLight, Light, Position, Scale))
					{
						Active = Shaders.Spot[1];
						State.Device->SetTexture2D(Light->DepthMap->GetTarget(0), 5, TH_PS);
					}
					else
						Active = Shaders.Spot[0];

					State.Device->SetShader(Active, TH_PS);
					State.Device->UpdateBuffer(Shaders.Spot[0], &SpotLight);
					State.Device->DrawIndexed((unsigned int)Cube[BufferType_Index]->GetElements(), 0, 0);
				}
			}
			void Lighting::RenderLineLights()
			{
				Graphics::Shader* Active = nullptr;
				if (!State.Backcull)
				{
					State.Device->SetRasterizerState(BackRasterizer);
					State.Backcull = true;
				}

				State.Device->SetDepthStencilState(DepthStencilNone);
				State.Device->SetShader(Shaders.Line[0], TH_VS);
				State.Device->SetBuffer(Shaders.Line[0], 3, TH_VS | TH_PS);
				State.Device->SetVertexBuffer(System->GetPrimitives()->GetQuad(), 0);

				for (auto It = LineLights->Begin(); It != LineLights->End(); ++It)
				{
					Engine::Components::LineLight* Light = (Engine::Components::LineLight*)*It;
					if (GetLineLight(&LineLight, Light))
					{
						Active = Shaders.Line[1];
						for (size_t i = 0; i < Light->DepthMap->size(); i++)
							State.Device->SetTexture2D((*Light->DepthMap)[i]->GetTarget(0), 5 + i, TH_PS);
					}
					else
						Active = Shaders.Line[0];

					State.Device->SetShader(Active, TH_PS);
					State.Device->UpdateBuffer(Shaders.Line[0], &LineLight);
					State.Device->Draw(6, 0);
				}

				State.Device->SetBlendState(BlendOverload);
			}
			void Lighting::RenderIllumination()
			{
				if (!EnableGI || !LightBuffer || State.Inner || !Storage.Area)
					return;

				Graphics::ElementBuffer* Cube[2];
				System->GetPrimitives()->GetCubeBuffers(Cube);

				Compute::Vector3 Position, Scale;
				GetLightCulling(Storage.Area, 0.0f, &Position, &Scale);
				VoxelBuffer.WorldViewProjection = Compute::Matrix4x4::CreateTranslatedScale(Position, Scale) * State.Scene->View.ViewProjection;

				if (State.Backcull)
					State.Device->SetDepthStencilState(DepthStencilLess);

				Graphics::MultiRenderTarget2D* MRT = System->GetMRT(TargetType_Main);
				Graphics::RenderTarget2D* RT = System->GetRT(TargetType_Secondary);
				State.Device->CopyTarget(MRT, 0, RT, 0);
				State.Device->SetVertexBuffer(Cube[BufferType_Vertex], 0);
				State.Device->SetTexture2D(RT->GetTarget(0), 1, TH_PS);
				State.Device->SetTexture3D(LightBuffer, 5, TH_PS);
				State.Device->SetShader(Shaders.Ambient[1], TH_VS | TH_PS);
				State.Device->SetBuffer(Shaders.Ambient[1], 3, TH_VS | TH_PS);
				State.Device->UpdateBuffer(Shaders.Ambient[1], &VoxelBuffer);
				State.Device->DrawIndexed((unsigned int)Cube[BufferType_Index]->GetElements(), 0, 0);
				State.Device->SetTexture2D(System->GetRT(TargetType_Main)->GetTarget(0), 1, TH_PS);
				State.Device->SetTexture3D(nullptr, 5, TH_PS);
				State.Device->SetVertexBuffer(System->GetPrimitives()->GetQuad(), 0);
				State.Device->SetDepthStencilState(DepthStencilNone);

				if (!State.Backcull)
					State.Device->SetRasterizerState(BackRasterizer);
			}
			void Lighting::RenderAmbient()
			{
				Graphics::MultiRenderTarget2D* MRT = System->GetMRT(TargetType_Main);
				Graphics::RenderTarget2D* RT = (State.Inner ? Surfaces.Output : System->GetRT(TargetType_Secondary));
				State.Device->CopyTarget(MRT, 0, RT, 0);
				State.Device->SetTexture2D(RT->GetTarget(0), 5, TH_PS);
				State.Device->SetTextureCube(SkyMap, 6, TH_PS);
				State.Device->SetShader(Shaders.Ambient[0], TH_VS | TH_PS);
				State.Device->SetBuffer(Shaders.Ambient[0], 3, TH_VS | TH_PS);
				State.Device->UpdateBuffer(Shaders.Ambient[0], &AmbientLight);
				State.Device->Draw(6, 0);
			}
			void Lighting::ClearStorage()
			{
				Storage.Area = nullptr;
				LightBuffer = nullptr;
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
				Graphics::MultiRenderTarget2D::Desc F1 = System->GetScene()->GetDescMRT();
				F1.MipLevels = System->GetDevice()->GetMipLevel((unsigned int)Surfaces.Size, (unsigned int)Surfaces.Size);
				F1.Width = (unsigned int)Surfaces.Size;
				F1.Height = (unsigned int)Surfaces.Size;
				SurfaceLight.MipLevels = (float)F1.MipLevels;
				Surfaces.Size = NewSize;

				TH_RELEASE(Surfaces.Merger);
				Surfaces.Merger = System->GetDevice()->CreateMultiRenderTarget2D(F1);

				Graphics::Cubemap::Desc I;
				I.Source = Surfaces.Merger;
				I.MipLevels = F1.MipLevels;
				I.Size = Surfaces.Size;

				TH_RELEASE(Surfaces.Subresource);
				Surfaces.Subresource = System->GetDevice()->CreateCubemap(I);

				Graphics::RenderTarget2D::Desc F2 = System->GetScene()->GetDescRT();
				F2.MipLevels = F1.MipLevels;
				F2.Width = F1.Width;
				F2.Height = F1.Height;

				TH_RELEASE(Surfaces.Output);
				Surfaces.Output = System->GetDevice()->CreateRenderTarget2D(F2);

				TH_RELEASE(Surfaces.Input);
				Surfaces.Input = System->GetDevice()->CreateRenderTarget2D(F2);
			}
			void Lighting::SetVoxelBuffer(RenderSystem* System, Graphics::Shader* Src, unsigned int Slot)
			{
				if (!System || !Src)
					return;

				Lighting* Renderer = System->GetRenderer<Lighting>();
				if (!Renderer)
					return;

				Graphics::GraphicsDevice* Device = System->GetDevice();
				Device->SetBuffer(Src, Slot, TH_VS | TH_PS | TH_GS);
				Device->UpdateBuffer(Src, &Renderer->VoxelBuffer);
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
				Graphics::GraphicsDevice* Device = System->GetDevice();
				DepthStencil = Device->GetDepthStencilState("none");
				Rasterizer = Device->GetRasterizerState("cull-back");
				Blend = Device->GetBlendState("overwrite");
				Sampler = Device->GetSamplerState("trilinear-x16");
				Layout = Device->GetInputLayout("shape-vertex");

				Shader = System->CompileShader("merger/transparency", sizeof(RenderPass));
			}
			Transparency::~Transparency()
			{
				System->FreeShader(Shader);
				TH_RELEASE(Merger);
				TH_RELEASE(Input);
			}
			void Transparency::ResizeBuffers()
			{
				Graphics::MultiRenderTarget2D::Desc F1 = System->GetScene()->GetDescMRT();
				Graphics::RenderTarget2D::Desc F2 = System->GetScene()->GetDescRT();
				MipLevels[TargetType_Main] = (float)F1.MipLevels;

				auto* Renderer = System->GetRenderer<Lighting>();
				if (Renderer != nullptr)
				{
					MipLevels[TargetType_Secondary] = (float)System->GetDevice()->GetMipLevel((unsigned int)Renderer->Surfaces.Size, (unsigned int)Renderer->Surfaces.Size);
					F1.MipLevels = (unsigned int)MipLevels[TargetType_Secondary];
					F1.Width = (unsigned int)Renderer->Surfaces.Size;
					F1.Height = (unsigned int)Renderer->Surfaces.Size;
					F2.MipLevels = F1.MipLevels;
					F2.Width = F1.Width;
					F2.Height = F1.Width;
				}

				TH_RELEASE(Merger);
				Merger = System->GetDevice()->CreateMultiRenderTarget2D(F1);

				TH_RELEASE(Input);
				Input = System->GetDevice()->CreateRenderTarget2D(F2);
			}
			void Transparency::Render(Rest::Timer* Time, RenderState State, RenderOpt Options)
			{
				bool Inner = (Options & RenderOpt_Inner);
				if (State != RenderState_Geometry_Result || Options & RenderOpt_Transparent)
					return;

				SceneGraph* Scene = System->GetScene();
				if (!Scene->GetTransparentCount())
					return;

				Graphics::MultiRenderTarget2D* MainMRT = System->GetMRT(TargetType_Main);
				Graphics::MultiRenderTarget2D* MRT = (Inner ? Merger : System->GetMRT(TargetType_Secondary));
				Graphics::RenderTarget2D* RT = (Inner ? Input : System->GetRT(TargetType_Main));
				Graphics::GraphicsDevice* Device = System->GetDevice();
				RenderPass.MipLevels = (Inner ? MipLevels[TargetType_Secondary] : MipLevels[TargetType_Main]);

				Scene->SwapMRT(TargetType_Main, MRT);
				Scene->SetMRT(TargetType_Main, true);
				Scene->Render(Time, RenderState_Geometry_Result, Options | RenderOpt_Transparent);
				Scene->SwapMRT(TargetType_Main, nullptr);

				Device->CopyTarget(MainMRT, 0, RT, 0);
				Device->GenerateMips(RT->GetTarget(0));
				Device->SetTarget(MainMRT, 0);
				Device->Clear(MainMRT, 0, 0, 0, 0);
				Device->UpdateBuffer(Shader, &RenderPass);
				Device->SetDepthStencilState(DepthStencil);
				Device->SetBlendState(Blend);
				Device->SetRasterizerState(Rasterizer);
				Device->SetInputLayout(Layout);
				Device->SetSamplerState(Sampler, 0, TH_PS);
				Device->SetShader(Shader, TH_VS | TH_PS);
				Device->SetBuffer(Shader, 3, TH_VS | TH_PS);
				Device->SetTexture2D(RT->GetTarget(0), 1, TH_PS);
				Device->SetTexture2D(MainMRT->GetTarget(1), 2, TH_PS);
				Device->SetTexture2D(MainMRT->GetTarget(2), 3, TH_PS);
				Device->SetTexture2D(MainMRT->GetTarget(3), 4, TH_PS);
				Device->SetTexture2D(MRT->GetTarget(0), 5, TH_PS);
				Device->SetTexture2D(MRT->GetTarget(1), 6, TH_PS);
				Device->SetTexture2D(MRT->GetTarget(2), 7, TH_PS);
				Device->SetTexture2D(MRT->GetTarget(3), 8, TH_PS);
				Device->SetVertexBuffer(System->GetPrimitives()->GetQuad(), 0);
				Device->UpdateBuffer(Graphics::RenderBufferType_Render);
				Device->Draw(6, 0);
				Device->FlushTexture2D(1, 8, TH_PS);
				System->RestoreOutput();
			}

			SSR::SSR(RenderSystem* Lab) : EffectDraw(Lab)
			{
				Shaders.Reflectance = CompileEffect("raytracing/reflectance", sizeof(Reflectance));
				Shaders.Gloss[0] = CompileEffect("blur/gloss-x", sizeof(Gloss));
				Shaders.Gloss[1] = CompileEffect("blur/gloss-y");
				Shaders.Additive = CompileEffect("merger/additive");
			}
			void SSR::Deserialize(ContentManager* Content, Rest::Document* Node)
			{
				NMake::Unpack(Node->Find("samples-1"), &Reflectance.Samples);
				NMake::Unpack(Node->Find("samples-2"), &Gloss.Samples);
				NMake::Unpack(Node->Find("intensity"), &Reflectance.Intensity);
				NMake::Unpack(Node->Find("blur"), &Gloss.Blur);
			}
			void SSR::Serialize(ContentManager* Content, Rest::Document* Node)
			{
				NMake::Pack(Node->Set("samples-1"), Reflectance.Samples);
				NMake::Pack(Node->Set("samples-2"), Gloss.Samples);
				NMake::Pack(Node->Set("intensity"), Reflectance.Intensity);
				NMake::Pack(Node->Set("blur"), Gloss.Blur);
			}
			void SSR::RenderEffect(Rest::Timer* Time)
			{
				Graphics::MultiRenderTarget2D* MRT = System->GetMRT(TargetType_Main);
				System->GetDevice()->GenerateMips(MRT->GetTarget(0));

				Reflectance.MipLevels = GetMipLevels();
				Gloss.Texel[0] = 1.0f / GetWidth();
				Gloss.Texel[1] = 1.0f / GetHeight();

				RenderMerge(Shaders.Reflectance, &Reflectance);
				RenderMerge(Shaders.Gloss[0], &Gloss, 2);
				RenderMerge(Shaders.Gloss[1], nullptr, 2);
				RenderResult(Shaders.Additive);
			}

			SSAO::SSAO(RenderSystem* Lab) : EffectDraw(Lab)
			{
				Shaders.Shading = CompileEffect("raytracing/shading", sizeof(Shading));
				Shaders.Fibo[0] = CompileEffect("blur/fibo-x", sizeof(Fibo));
				Shaders.Fibo[1] = CompileEffect("blur/fibo-y");
				Shaders.Multiply = CompileEffect("merger/multiply");
			}
			void SSAO::Deserialize(ContentManager* Content, Rest::Document* Node)
			{
				NMake::Unpack(Node->Find("samples-1"), &Shading.Samples);
				NMake::Unpack(Node->Find("scale"), &Shading.Scale);
				NMake::Unpack(Node->Find("intensity"), &Shading.Intensity);
				NMake::Unpack(Node->Find("bias"), &Shading.Bias);
				NMake::Unpack(Node->Find("radius"), &Shading.Radius);
				NMake::Unpack(Node->Find("distance"), &Shading.Distance);
				NMake::Unpack(Node->Find("fade"), &Shading.Fade);
				NMake::Unpack(Node->Find("power"), &Fibo.Power);
				NMake::Unpack(Node->Find("samples-2"), &Fibo.Samples);
				NMake::Unpack(Node->Find("blur"), &Fibo.Blur);
			}
			void SSAO::Serialize(ContentManager* Content, Rest::Document* Node)
			{
				NMake::Pack(Node->Set("samples-1"), Shading.Samples);
				NMake::Pack(Node->Set("scale"), Shading.Scale);
				NMake::Pack(Node->Set("intensity"), Shading.Intensity);
				NMake::Pack(Node->Set("bias"), Shading.Bias);
				NMake::Pack(Node->Set("radius"), Shading.Radius);
				NMake::Pack(Node->Set("distance"), Shading.Distance);
				NMake::Pack(Node->Set("fade"), Shading.Fade);
				NMake::Pack(Node->Set("power"), Fibo.Power);
				NMake::Pack(Node->Set("samples-2"), Fibo.Samples);
				NMake::Pack(Node->Set("blur"), Fibo.Blur);
			}
			void SSAO::RenderEffect(Rest::Timer* Time)
			{
				Fibo.Texel[0] = 1.0f / GetWidth();
				Fibo.Texel[1] = 1.0f / GetHeight();

				RenderMerge(Shaders.Shading, &Shading);
				RenderMerge(Shaders.Fibo[0], &Fibo, 2);
				RenderMerge(Shaders.Fibo[1], nullptr, 2);
				RenderResult(Shaders.Multiply);
			}

			DoF::DoF(RenderSystem* Lab) : EffectDraw(Lab)
			{
				CompileEffect("postprocess/focus", sizeof(Focus));
			}
			void DoF::Deserialize(ContentManager* Content, Rest::Document* Node)
			{
				NMake::Unpack(Node->Find("distance"), &Distance);
				NMake::Unpack(Node->Find("time"), &Time);
				NMake::Unpack(Node->Find("radius"), &Radius);
				NMake::Unpack(Node->Find("radius"), &Focus.Radius);
				NMake::Unpack(Node->Find("bokeh"), &Focus.Bokeh);
				NMake::Unpack(Node->Find("scale"), &Focus.Scale);
				NMake::Unpack(Node->Find("near-distance"), &Focus.NearDistance);
				NMake::Unpack(Node->Find("near-range"), &Focus.NearRange);
				NMake::Unpack(Node->Find("far-distance"), &Focus.FarDistance);
				NMake::Unpack(Node->Find("far-range"), &Focus.FarRange);
			}
			void DoF::Serialize(ContentManager* Content, Rest::Document* Node)
			{
				NMake::Pack(Node->Set("distance"), Distance);
				NMake::Pack(Node->Set("time"), Time);
				NMake::Pack(Node->Set("radius"), Radius);
				NMake::Pack(Node->Set("radius"), Focus.Radius);
				NMake::Pack(Node->Set("bokeh"), Focus.Bokeh);
				NMake::Pack(Node->Set("scale"), Focus.Scale);
				NMake::Pack(Node->Set("near-distance"), Focus.NearDistance);
				NMake::Pack(Node->Set("near-range"), Focus.NearRange);
				NMake::Pack(Node->Set("far-distance"), Focus.FarDistance);
				NMake::Pack(Node->Set("far-range"), Focus.FarRange);
			}
			void DoF::RenderEffect(Rest::Timer* Time)
			{
				if (Distance > 0.0f)
					FocusAtNearestTarget(Time->GetDeltaTime());
				
				Focus.Texel[0] = 1.0f / GetWidth();
				Focus.Texel[1] = 1.0f / GetHeight();
				RenderResult(nullptr, &Focus);
			}
			void DoF::FocusAtNearestTarget(float DeltaTime)
			{
				Compute::Ray Origin;
				Origin.Origin = System->GetScene()->View.WorldPosition.InvertZ();
				Origin.Direction = System->GetScene()->View.WorldRotation.DepthDirection();

				bool Change = false;
				System->GetScene()->RayTest<Components::Model>(Origin, Distance, [this, &Origin, &Change](Component* Result, const Compute::Vector3& Hit)
				{
					float NextRange = Result->As<Components::Model>()->GetRange();
					float NextDistance = Origin.Origin.Distance(Hit) + NextRange / 2.0f;

					if (NextDistance <= Focus.NearRange || NextDistance + NextRange / 2.0f >= Distance)
						return true;

					if (NextDistance >= State.Distance && State.Distance > 0.0f)
						return true;

					State.Distance = NextDistance;
					State.Range = NextRange;
					Change = true;

					return true;
				});

				if (Change)
				{
					State.Radius = Focus.Radius;
					State.Factor = 0.0f;
				}

				State.Factor += Time * DeltaTime;
				if (State.Factor > 1.0f)
					State.Factor = 1.0f;

				if (State.Distance > 0.0f)
				{
					State.Distance += State.Range / 2.0f + Focus.FarRange;
					Focus.FarDistance = State.Distance;
					Focus.Radius = Compute::Math<float>::Lerp(State.Radius, Radius, State.Factor);
				}
				else
				{
					State.Distance = 0.0f;
					if (State.Factor >= 1.0f)
						Focus.FarDistance = State.Distance;

					Focus.Radius = Compute::Math<float>::Lerp(State.Radius, 0.0f, State.Factor);
				}

				if (Focus.Radius < 0.0f)
					Focus.Radius = 0.0f;
			}

			MotionBlur::MotionBlur(RenderSystem* Lab) : EffectDraw(Lab)
			{
				Shaders.Velocity = CompileEffect("postprocess/velocity", sizeof(Velocity));
				Shaders.Motion = CompileEffect("blur/motion", sizeof(Motion));
			}
			void MotionBlur::Deserialize(ContentManager* Content, Rest::Document* Node)
			{
				NMake::Unpack(Node->Find("samples"), &Motion.Samples);
				NMake::Unpack(Node->Find("blur"), &Motion.Blur);
				NMake::Unpack(Node->Find("motion"), &Motion.Motion);
			}
			void MotionBlur::Serialize(ContentManager* Content, Rest::Document* Node)
			{
				NMake::Pack(Node->Set("samples"), Motion.Samples);
				NMake::Pack(Node->Set("blur"), Motion.Blur);
				NMake::Pack(Node->Set("motion"), Motion.Motion);
			}
			void MotionBlur::RenderEffect(Rest::Timer* Time)
			{
				RenderMerge(Shaders.Velocity, &Velocity);
				RenderResult(Shaders.Motion, &Motion);

				Velocity.LastViewProjection = System->GetScene()->View.ViewProjection;
			}

			Bloom::Bloom(RenderSystem* Lab) : EffectDraw(Lab)
			{
				Shaders.Bloom = CompileEffect("postprocess/bloom", sizeof(Extraction));
				Shaders.Fibo[0] = CompileEffect("blur/fibo-x", sizeof(Fibo));
				Shaders.Fibo[1] = CompileEffect("blur/fibo-y");
				Shaders.Additive = CompileEffect("merger/additive");
			}
			void Bloom::Deserialize(ContentManager* Content, Rest::Document* Node)
			{
				NMake::Unpack(Node->Find("intensity"), &Extraction.Intensity);
				NMake::Unpack(Node->Find("threshold"), &Extraction.Threshold);
				NMake::Unpack(Node->Find("power"), &Fibo.Power);
				NMake::Unpack(Node->Find("samples"), &Fibo.Samples);
				NMake::Unpack(Node->Find("blur"), &Fibo.Blur);
			}
			void Bloom::Serialize(ContentManager* Content, Rest::Document* Node)
			{
				NMake::Pack(Node->Set("intensity"), Extraction.Intensity);
				NMake::Pack(Node->Set("threshold"), Extraction.Threshold);
				NMake::Pack(Node->Set("power"), Fibo.Power);
				NMake::Pack(Node->Set("samples"), Fibo.Samples);
				NMake::Pack(Node->Set("blur"), Fibo.Blur);
			}
			void Bloom::RenderEffect(Rest::Timer* Time)
			{
				Fibo.Texel[0] = 1.0f / GetWidth();
				Fibo.Texel[1] = 1.0f / GetHeight();

				RenderMerge(Shaders.Bloom, &Extraction);
				RenderMerge(Shaders.Fibo[0], &Fibo, 3);
				RenderMerge(Shaders.Fibo[1], nullptr, 3);
				RenderResult(Shaders.Additive);
			}

			Tone::Tone(RenderSystem* Lab) : EffectDraw(Lab)
			{
				Shaders.Luminance = CompileEffect("postprocess/luminance", sizeof(Luminance));
				Shaders.Tone = CompileEffect("postprocess/tone", sizeof(Mapping));
			}
			Tone::~Tone()
			{
				TH_RELEASE(LutTarget);
				TH_RELEASE(LutMap);
			}
			void Tone::Deserialize(ContentManager* Content, Rest::Document* Node)
			{
				NMake::Unpack(Node->Find("grayscale"), &Mapping.Grayscale);
				NMake::Unpack(Node->Find("aces"), &Mapping.ACES);
				NMake::Unpack(Node->Find("filmic"), &Mapping.Filmic);
				NMake::Unpack(Node->Find("lottes"), &Mapping.Lottes);
				NMake::Unpack(Node->Find("reinhard"), &Mapping.Reinhard);
				NMake::Unpack(Node->Find("reinhard2"), &Mapping.Reinhard2);
				NMake::Unpack(Node->Find("unreal"), &Mapping.Unreal);
				NMake::Unpack(Node->Find("uchimura"), &Mapping.Uchimura);
				NMake::Unpack(Node->Find("ubrightness"), &Mapping.UBrightness);
				NMake::Unpack(Node->Find("usontrast"), &Mapping.UContrast);
				NMake::Unpack(Node->Find("ustart"), &Mapping.UStart);
				NMake::Unpack(Node->Find("ulength"), &Mapping.ULength);
				NMake::Unpack(Node->Find("ublack"), &Mapping.UBlack);
				NMake::Unpack(Node->Find("upedestal"), &Mapping.UPedestal);
				NMake::Unpack(Node->Find("exposure"), &Mapping.Exposure);
				NMake::Unpack(Node->Find("eintensity"), &Mapping.EIntensity);
				NMake::Unpack(Node->Find("egamma"), &Mapping.EGamma);
				NMake::Unpack(Node->Find("adaptation"), &Mapping.Adaptation);
				NMake::Unpack(Node->Find("agray"), &Mapping.AGray);
				NMake::Unpack(Node->Find("awhite"), &Mapping.AWhite);
				NMake::Unpack(Node->Find("ablack"), &Mapping.ABlack);
				NMake::Unpack(Node->Find("aspeed"), &Mapping.ASpeed);
			}
			void Tone::Serialize(ContentManager* Content, Rest::Document* Node)
			{
				NMake::Pack(Node->Set("grayscale"), Mapping.Grayscale);
				NMake::Pack(Node->Set("aces"), Mapping.ACES);
				NMake::Pack(Node->Set("filmic"), Mapping.Filmic);
				NMake::Pack(Node->Set("lottes"), Mapping.Lottes);
				NMake::Pack(Node->Set("reinhard"), Mapping.Reinhard);
				NMake::Pack(Node->Set("reinhard2"), Mapping.Reinhard2);
				NMake::Pack(Node->Set("unreal"), Mapping.Unreal);
				NMake::Pack(Node->Set("uchimura"), Mapping.Uchimura);
				NMake::Pack(Node->Set("ubrightness"), Mapping.UBrightness);
				NMake::Pack(Node->Set("usontrast"), Mapping.UContrast);
				NMake::Pack(Node->Set("ustart"), Mapping.UStart);
				NMake::Pack(Node->Set("ulength"), Mapping.ULength);
				NMake::Pack(Node->Set("ublack"), Mapping.UBlack);
				NMake::Pack(Node->Set("upedestal"), Mapping.UPedestal);
				NMake::Pack(Node->Set("exposure"), Mapping.Exposure);
				NMake::Pack(Node->Set("eintensity"), Mapping.EIntensity);
				NMake::Pack(Node->Set("egamma"), Mapping.EGamma);
				NMake::Pack(Node->Set("adaptation"), Mapping.Adaptation);
				NMake::Pack(Node->Set("agray"), Mapping.AGray);
				NMake::Pack(Node->Set("awhite"), Mapping.AWhite);
				NMake::Pack(Node->Set("ablack"), Mapping.ABlack);
				NMake::Pack(Node->Set("aspeed"), Mapping.ASpeed);
			}
			void Tone::RenderEffect(Rest::Timer* Time)
			{
				if (Mapping.Adaptation > 0.0f)
					RenderLUT(Time);

				RenderResult(Shaders.Tone, &Mapping);
			}
			void Tone::RenderLUT(Rest::Timer* Time)
			{
				if (!LutMap || !LutTarget)
					SetLUTSize(1);

				Graphics::MultiRenderTarget2D* MRT = System->GetMRT(TargetType_Main);
				Graphics::GraphicsDevice* Device = System->GetDevice();
				Device->GenerateMips(MRT->GetTarget(0));
				Device->CopyTexture2D(LutTarget, 0, &LutMap);

				Luminance.Texel[0] = 1.0f / GetWidth();
				Luminance.Texel[1] = 1.0f / GetHeight();
				Luminance.MipLevels = GetMipLevels();
				Luminance.Time = Time->GetTimeStep() * Mapping.ASpeed;

				RenderTexture(0, LutMap);
				RenderOutput(LutTarget);
				RenderMerge(Shaders.Luminance, &Luminance);
			}
			void Tone::SetLUTSize(size_t Size)
			{
				TH_CLEAR(LutTarget);
				TH_CLEAR(LutMap);

				Graphics::GraphicsDevice* Device = System->GetDevice();
				Graphics::RenderTarget2D::Desc RT = System->GetScene()->GetDescRT();
				RT.MipLevels = Device->GetMipLevel(Size, Size);
				RT.FormatMode = Graphics::Format_R16_Float;
				RT.Width = Size;
				RT.Height = Size;

				LutTarget = Device->CreateRenderTarget2D(RT);
				Device->CopyTexture2D(LutTarget, 0, &LutMap);
			}

			Glitch::Glitch(RenderSystem* Lab) : EffectDraw(Lab), ScanLineJitter(0), VerticalJump(0), HorizontalShake(0), ColorDrift(0)
			{
				CompileEffect("postprocess/glitch", sizeof(Distortion));
			}
			void Glitch::Deserialize(ContentManager* Content, Rest::Document* Node)
			{
				NMake::Unpack(Node->Find("scanline-jitter"), &ScanLineJitter);
				NMake::Unpack(Node->Find("vertical-jump"), &VerticalJump);
				NMake::Unpack(Node->Find("horizontal-shake"), &HorizontalShake);
				NMake::Unpack(Node->Find("color-drift"), &ColorDrift);
				NMake::Unpack(Node->Find("horizontal-shake"), &HorizontalShake);
				NMake::Unpack(Node->Find("elapsed-time"), &Distortion.ElapsedTime);
				NMake::Unpack(Node->Find("scanline-jitter-displacement"), &Distortion.ScanLineJitterDisplacement);
				NMake::Unpack(Node->Find("scanline-jitter-threshold"), &Distortion.ScanLineJitterThreshold);
				NMake::Unpack(Node->Find("vertical-jump-amount"), &Distortion.VerticalJumpAmount);
				NMake::Unpack(Node->Find("vertical-jump-time"), &Distortion.VerticalJumpTime);
				NMake::Unpack(Node->Find("color-drift-amount"), &Distortion.ColorDriftAmount);
				NMake::Unpack(Node->Find("color-drift-time"), &Distortion.ColorDriftTime);
			}
			void Glitch::Serialize(ContentManager* Content, Rest::Document* Node)
			{
				NMake::Pack(Node->Set("scanline-jitter"), ScanLineJitter);
				NMake::Pack(Node->Set("vertical-jump"), VerticalJump);
				NMake::Pack(Node->Set("horizontal-shake"), HorizontalShake);
				NMake::Pack(Node->Set("color-drift"), ColorDrift);
				NMake::Pack(Node->Set("horizontal-shake"), HorizontalShake);
				NMake::Pack(Node->Set("elapsed-time"), Distortion.ElapsedTime);
				NMake::Pack(Node->Set("scanline-jitter-displacement"), Distortion.ScanLineJitterDisplacement);
				NMake::Pack(Node->Set("scanline-jitter-threshold"), Distortion.ScanLineJitterThreshold);
				NMake::Pack(Node->Set("vertical-jump-amount"), Distortion.VerticalJumpAmount);
				NMake::Pack(Node->Set("vertical-jump-time"), Distortion.VerticalJumpTime);
				NMake::Pack(Node->Set("color-drift-amount"), Distortion.ColorDriftAmount);
				NMake::Pack(Node->Set("color-drift-time"), Distortion.ColorDriftTime);
			}
			void Glitch::RenderEffect(Rest::Timer* Time)
			{
				if (Distortion.ElapsedTime >= 32000.0f)
					Distortion.ElapsedTime = 0.0f;

				Distortion.ElapsedTime += (float)Time->GetDeltaTime() * 10.0f;
				Distortion.VerticalJumpAmount = VerticalJump;
				Distortion.VerticalJumpTime += (float)Time->GetDeltaTime() * VerticalJump * 11.3f;
				Distortion.ScanLineJitterThreshold = Compute::Mathf::Saturate(1.0f - ScanLineJitter * 1.2f);
				Distortion.ScanLineJitterDisplacement = 0.002f + Compute::Mathf::Pow(ScanLineJitter, 3) * 0.05f;
				Distortion.HorizontalShake = HorizontalShake * 0.2f;
				Distortion.ColorDriftAmount = ColorDrift * 0.04f;
				Distortion.ColorDriftTime = Distortion.ElapsedTime * 606.11f;
				RenderResult(nullptr, &Distortion);
			}

			UserInterface::UserInterface(RenderSystem* Lab) : UserInterface(Lab, Application::Get() ? Application::Get()->Activity : nullptr)
			{
			}
			UserInterface::UserInterface(RenderSystem* Lab, Graphics::Activity* NewActivity) : Renderer(Lab), Activity(NewActivity)
			{
				Context = new GUI::Context(System->GetDevice());
			}
			UserInterface::~UserInterface()
			{
				TH_RELEASE(Context);
			}
			void UserInterface::Render(Rest::Timer* Timer, RenderState State, RenderOpt Options)
			{
				if (!Context || State != RenderState_Geometry_Result || Options & RenderOpt_Inner || Options & RenderOpt_Transparent)
					return;

				Context->UpdateEvents(Activity);
				Context->RenderLists(System->GetMRT(TargetType_Main)->GetTarget(0));
				System->RestoreOutput();
			}
			GUI::Context* UserInterface::GetContext()
			{
				return Context;
			}
		}
	}
}
