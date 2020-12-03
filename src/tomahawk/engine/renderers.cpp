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
				if (System->GetDevice()->GetSection("topology/model/geometry", &I.Data))
					Shaders.Geometry = System->CompileShader("mr-geometry", I, 0);

				if (System->GetDevice()->GetSection("topology/model/occlusion", &I.Data))
					Shaders.Occlusion = System->CompileShader("mr-occlusion", I, 0);

				if (System->GetDevice()->GetSection("topology/model/flux/linear", &I.Data))
					Shaders.Linear = System->CompileShader("mr-flux-linear", I, 0);

				if (System->GetDevice()->GetSection("topology/model/flux/cubic", &I.Data))
					Shaders.Cubic = System->CompileShader("mr-flux-cubic", I, sizeof(Depth));
			}
			Model::~Model()
			{
				System->FreeShader("mr-geometry", Shaders.Geometry);
				System->FreeShader("mr-occlusion", Shaders.Occlusion);
				System->FreeShader("mr-flux-linear", Shaders.Linear);
				System->FreeShader("mr-flux-cubic", Shaders.Cubic);
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
			void Model::RenderGeometry(Rest::Timer* Time, Rest::Pool<Drawable*>* Geometry, RenderOpt Options)
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
			void Model::RenderFluxLinear(Rest::Timer* Time, Rest::Pool<Drawable*>* Geometry)
			{
				Graphics::GraphicsDevice* Device = System->GetDevice();
				Device->SetSamplerState(Sampler, 0);
				Device->SetDepthStencilState(DepthStencil);
				Device->SetBlendState(Blend);
				Device->SetRasterizerState(FrontRasterizer);
				Device->SetInputLayout(Layout);
				Device->SetShader(Shaders.Linear, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);

				for (auto It = Geometry->Begin(); It != Geometry->End(); ++It)
				{
					Engine::Components::Model* Base = (Engine::Components::Model*)*It;
					auto* Drawable = Base->GetDrawable();

					if (!Drawable || !System->PassDrawable(Base, CullResult_Always, nullptr))
						continue;

					for (auto&& Mesh : Drawable->Meshes)
					{
						auto* Surface = Base->GetSurface(Mesh);
						if (!Surface || !Surface->FillFluxLinear(Device))
							continue;

						Device->Render.World = Mesh->World * Base->GetEntity()->Transform->GetWorld();
						Device->Render.WorldViewProjection = Device->Render.World * System->GetScene()->View.ViewProjection;
						Device->UpdateBuffer(Graphics::RenderBufferType_Render);
						Device->DrawIndexed(Mesh);
					}
				}

				Device->SetTexture2D(nullptr, 1);
			}
			void Model::RenderFluxCubic(Rest::Timer* Time, Rest::Pool<Drawable*>* Geometry, Compute::Matrix4x4* ViewProjection)
			{
				Graphics::GraphicsDevice* Device = System->GetDevice();
				memcpy(Depth.FaceViewProjection, ViewProjection, sizeof(Compute::Matrix4x4) * 6);
				Device->SetSamplerState(Sampler, 0);
				Device->SetDepthStencilState(DepthStencil);
				Device->SetBlendState(Blend);
				Device->SetRasterizerState(FrontRasterizer);
				Device->SetInputLayout(Layout);
				Device->SetShader(Shaders.Cubic, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel | Graphics::ShaderType_Geometry);
				Device->SetBuffer(Shaders.Cubic, 3, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel | Graphics::ShaderType_Geometry);
				Device->UpdateBuffer(Shaders.Cubic, &Depth);

				for (auto It = Geometry->Begin(); It != Geometry->End(); ++It)
				{
					Engine::Components::Model* Base = (Engine::Components::Model*)*It;
					auto* Drawable = Base->GetDrawable();

					if (!Drawable || !Base->IsNear(System->GetScene()->View))
						continue;

					for (auto&& Mesh : Drawable->Meshes)
					{
						auto* Surface = Base->GetSurface(Mesh);
						if (!Surface || !Surface->FillFluxCubic(Device))
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
				if (System->GetDevice()->GetSection("topology/skin/geometry", &I.Data))
					Shaders.Geometry = System->CompileShader("sr-geometry", I, 0);

				if (System->GetDevice()->GetSection("topology/skin/occlusion", &I.Data))
					Shaders.Occlusion = System->CompileShader("sr-occlusion", I, 0);

				if (System->GetDevice()->GetSection("topology/skin/flux/linear", &I.Data))
					Shaders.Linear = System->CompileShader("sr-flux-linear", I, 0);

				if (System->GetDevice()->GetSection("topology/skin/flux/cubic", &I.Data))
					Shaders.Cubic = System->CompileShader("sr-flux-cubic", I, sizeof(Depth));
			}
			Skin::~Skin()
			{
				System->FreeShader("sr-geometry", Shaders.Geometry);
				System->FreeShader("sr-occlusion", Shaders.Occlusion);
				System->FreeShader("sr-flux-linear", Shaders.Linear);
				System->FreeShader("sr-flux-cubic", Shaders.Cubic);
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
			void Skin::RenderGeometry(Rest::Timer* Time, Rest::Pool<Drawable*>* Geometry, RenderOpt Options)
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
			void Skin::RenderFluxLinear(Rest::Timer* Time, Rest::Pool<Drawable*>* Geometry)
			{
				Graphics::GraphicsDevice* Device = System->GetDevice();
				Device->SetSamplerState(Sampler, 0);
				Device->SetDepthStencilState(DepthStencil);
				Device->SetBlendState(Blend);
				Device->SetRasterizerState(FrontRasterizer);
				Device->SetInputLayout(Layout);
				Device->SetShader(Shaders.Linear, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);

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
						if (!Surface || !Surface->FillFluxLinear(Device))
							continue;

						Device->Render.World = Mesh->World * Base->GetEntity()->Transform->GetWorld();
						Device->Render.WorldViewProjection = Device->Render.World * System->GetScene()->View.ViewProjection;
						Device->UpdateBuffer(Graphics::RenderBufferType_Render);
						Device->DrawIndexed(Mesh);
					}
				}

				Device->SetTexture2D(nullptr, 1);
			}
			void Skin::RenderFluxCubic(Rest::Timer* Time, Rest::Pool<Drawable*>* Geometry, Compute::Matrix4x4* ViewProjection)
			{
				Graphics::GraphicsDevice* Device = System->GetDevice();
				memcpy(Depth.FaceViewProjection, ViewProjection, sizeof(Compute::Matrix4x4) * 6);
				Device->SetSamplerState(Sampler, 0);
				Device->SetDepthStencilState(DepthStencil);
				Device->SetBlendState(Blend);
				Device->SetRasterizerState(FrontRasterizer);
				Device->SetInputLayout(Layout);
				Device->SetShader(Shaders.Cubic, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel | Graphics::ShaderType_Geometry);
				Device->SetBuffer(Shaders.Cubic, 3, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel | Graphics::ShaderType_Geometry);
				Device->UpdateBuffer(Shaders.Cubic, &Depth);

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
						if (!Surface || !Surface->FillFluxCubic(Device))
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
				BackRasterizer = Lab->GetDevice()->GetRasterizerState("cull-back");
				FrontRasterizer = Lab->GetDevice()->GetRasterizerState("cull-front");
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
				if (System->GetDevice()->GetSection("topology/model/geometry", &I.Data))
					Shaders.Geometry = System->CompileShader("mr-geometry", I, 0);

				if (System->GetDevice()->GetSection("topology/model/occlusion", &I.Data))
					Shaders.Occlusion = System->CompileShader("mr-occlusion", I, 0);

				if (System->GetDevice()->GetSection("topology/model/flux/linear", &I.Data))
					Shaders.Linear = System->CompileShader("mr-flux-linear", I, 0);

				if (System->GetDevice()->GetSection("topology/model/flux/cubic", &I.Data))
					Shaders.Cubic = System->CompileShader("mr-flux-cubic", I, sizeof(Depth));
			}
			SoftBody::~SoftBody()
			{
				System->FreeShader("mr-geometry", Shaders.Geometry);
				System->FreeShader("mr-occlusion", Shaders.Occlusion);
				System->FreeShader("mr-flux-linear", Shaders.Linear);
				System->FreeShader("mr-flux-cubic", Shaders.Cubic);
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
				Device->SetRasterizerState(BackRasterizer);
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
			void SoftBody::RenderGeometry(Rest::Timer* Time, Rest::Pool<Drawable*>* Geometry, RenderOpt Options)
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
			void SoftBody::RenderFluxLinear(Rest::Timer* Time, Rest::Pool<Drawable*>* Geometry)
			{
				Graphics::GraphicsDevice* Device = System->GetDevice();
				Device->SetSamplerState(Sampler, 0);
				Device->SetDepthStencilState(DepthStencil);
				Device->SetBlendState(Blend);
				Device->SetRasterizerState(FrontRasterizer);
				Device->SetInputLayout(Layout);
				Device->SetShader(Shaders.Linear, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);

				for (auto It = Geometry->Begin(); It != Geometry->End(); ++It)
				{
					Engine::Components::SoftBody* Base = (Engine::Components::SoftBody*)*It;
					if (Base->GetIndices().empty())
						continue;

					if (!System->PassDrawable(Base, CullResult_Always, nullptr))
						continue;

					auto* Surface = Base->GetSurface();
					if (!Surface || !Surface->FillFluxLinear(Device))
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
			void SoftBody::RenderFluxCubic(Rest::Timer* Time, Rest::Pool<Drawable*>* Geometry, Compute::Matrix4x4* ViewProjection)
			{
				Graphics::GraphicsDevice* Device = System->GetDevice();
				memcpy(Depth.FaceViewProjection, ViewProjection, sizeof(Compute::Matrix4x4) * 6);
				Device->SetSamplerState(Sampler, 0);
				Device->SetDepthStencilState(DepthStencil);
				Device->SetBlendState(Blend);
				Device->SetRasterizerState(FrontRasterizer);
				Device->SetInputLayout(Layout);
				Device->SetShader(Shaders.Cubic, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel | Graphics::ShaderType_Geometry);
				Device->SetBuffer(Shaders.Cubic, 3, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel | Graphics::ShaderType_Geometry);
				Device->UpdateBuffer(Shaders.Cubic, &Depth);

				for (auto It = Geometry->Begin(); It != Geometry->End(); ++It)
				{
					Engine::Components::SoftBody* Base = (Engine::Components::SoftBody*)*It;
					if (!Base->GetBody() || Base->GetEntity()->Transform->Position.Distance(System->GetScene()->View.WorldPosition) >= System->GetScene()->View.FarPlane + Base->GetEntity()->Transform->Scale.Length())
						continue;

					auto* Surface = Base->GetSurface();
					if (!Surface || !Surface->FillFluxCubic(Device))
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
				if (System->GetDevice()->GetSection("topology/emitter/geometry/opaque", &I.Data))
					Shaders.Opaque = System->CompileShader("er-opaque", I, 0);

				if (System->GetDevice()->GetSection("topology/emitter/geometry/transparency", &I.Data))
					Shaders.Transparency = System->CompileShader("er-transparency", I, 0);

				if (System->GetDevice()->GetSection("topology/emitter/flux/linear", &I.Data))
					Shaders.Linear = System->CompileShader("er-flux-linear", I, 0);

				if (System->GetDevice()->GetSection("topology/emitter/flux/point", &I.Data))
					Shaders.Point = System->CompileShader("er-flux-point", I, 0);

				if (System->GetDevice()->GetSection("topology/emitter/flux/quad", &I.Data))
					Shaders.Quad = System->CompileShader("er-flux-quad", I, sizeof(Depth));
			}
			Emitter::~Emitter()
			{
				System->FreeShader("er-opaque", Shaders.Opaque);
				System->FreeShader("er-transparency", Shaders.Transparency);
				System->FreeShader("er-flux-linear", Shaders.Linear);
				System->FreeShader("er-flux-point", Shaders.Point);
				System->FreeShader("er-flux-quad", Shaders.Quad);
			}
			void Emitter::Activate()
			{
				Opaque = System->AddCull<Engine::Components::Emitter>();
			}
			void Emitter::Deactivate()
			{
				Opaque = System->RemoveCull<Engine::Components::Emitter>();
			}
			void Emitter::RenderGeometry(Rest::Timer* Time, Rest::Pool<Drawable*>* Geometry, RenderOpt Options)
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
			void Emitter::RenderFluxLinear(Rest::Timer* Time, Rest::Pool<Drawable*>* Geometry)
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
				Device->SetShader(Shaders.Linear, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);
				Device->SetVertexBuffer(nullptr, 0);

				for (auto It = Geometry->Begin(); It != Geometry->End(); ++It)
				{
					Engine::Components::Emitter* Base = (Engine::Components::Emitter*)*It;
					if (!Base->GetBuffer())
						continue;

					auto* Surface = Base->GetSurface();
					if (!Surface || !Surface->FillFluxLinear(Device))
						continue;

					Device->Render.World = Base->QuadBased ? System->GetScene()->View.Projection : Compute::Matrix4x4::Identity();
					Device->Render.WorldViewProjection = (Base->QuadBased ? View : System->GetScene()->View.ViewProjection);
					
					if (Base->Connected)
						Device->Render.WorldViewProjection = Base->GetEntity()->Transform->GetWorld() * Device->Render.WorldViewProjection;

					Device->SetBuffer(Base->GetBuffer(), 8);
					Device->SetShader(Base->QuadBased ? Shaders.Linear : nullptr, Graphics::ShaderType_Geometry);
					Device->UpdateBuffer(Graphics::RenderBufferType_Render);
					Device->Draw((unsigned int)Base->GetBuffer()->GetArray()->Size(), 0);
				}

				Device->SetTexture2D(nullptr, 1);
				Device->SetPrimitiveTopology(T);
				Device->SetShader(nullptr, Graphics::ShaderType_Geometry);
			}
			void Emitter::RenderFluxCubic(Rest::Timer* Time, Rest::Pool<Drawable*>* Geometry, Compute::Matrix4x4* ViewProjection)
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
				Device->SetBuffer(Shaders.Quad, 3, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel | Graphics::ShaderType_Geometry);
				Device->UpdateBuffer(Shaders.Quad, &Depth);

				for (auto It = Geometry->Begin(); It != Geometry->End(); ++It)
				{
					Engine::Components::Emitter* Base = (Engine::Components::Emitter*)*It;
					if (!Base->GetBuffer())
						continue;

					auto* Surface = Base->GetSurface();
					if (!Surface || !Surface->FillFluxCubic(Device))
						continue;

					Device->Render.World = (Base->Connected ? Base->GetEntity()->Transform->GetWorld() : Compute::Matrix4x4::Identity());			
					Device->SetBuffer(Base->GetBuffer(), 8);
					Device->SetShader(Base->QuadBased ? Shaders.Quad : Shaders.Point, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel | Graphics::ShaderType_Geometry);
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
				if (System->GetDevice()->GetSection("topology/decal/geometry", &I.Data))
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
			void Decal::RenderGeometry(Rest::Timer* Time, Rest::Pool<Drawable*>* Geometry, RenderOpt Options)
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
			void Decal::RenderFluxLinear(Rest::Timer* Time, Rest::Pool<Drawable*>* Geometry)
			{
			}
			void Decal::RenderFluxCubic(Rest::Timer* Time, Rest::Pool<Drawable*>* Geometry, Compute::Matrix4x4* ViewProjection)
			{
			}

			Transparency::Transparency(RenderSystem* Lab) : Renderer(Lab)
			{
				DepthStencil = Lab->GetDevice()->GetDepthStencilState("none");
				Rasterizer = Lab->GetDevice()->GetRasterizerState("cull-back");
				Blend = Lab->GetDevice()->GetBlendState("overwrite");
				Sampler = Lab->GetDevice()->GetSamplerState("trilinear-x16");
				Layout = Lab->GetDevice()->GetInputLayout("shape-vertex");

				Graphics::Shader::Desc I = Graphics::Shader::Desc();
				if (System->GetDevice()->GetSection("effects/transparency", &I.Data))
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
			void Transparency::Activate()
			{
				ResizeBuffers();
			}
			void Transparency::Render(Rest::Timer* Time, RenderState State, RenderOpt Options)
			{
				bool Inner = (Options & RenderOpt_Inner);
				if (State != RenderState_Geometry || Options & RenderOpt_Transparent)
					return;

				SceneGraph* Scene = System->GetScene();
				if (!Scene->GetTransparentCount())
					return;

				Graphics::MultiRenderTarget2D* S = Scene->GetSurface();
				Graphics::GraphicsDevice* Device = System->GetDevice();
				RenderPass.MipLevels = (Inner ? MipLevels2 : MipLevels1);

				Scene->SwapSurface(Inner ? Surface2 : Surface1);
				Scene->SetSurfaceCleared();
				Scene->RenderGeometry(Time, Options | RenderOpt_Transparent);
				Scene->SwapSurface(S);

				Device->CopyTargetTo(S, 0, Inner ? Input2 : Input1);
				Device->GenerateMips(Inner ? Input2->GetTarget() : Input1->GetTarget());
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
				Device->SetTexture2D(Inner ? Input2->GetTarget() : Input1->GetTarget(), 1);
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

				auto* Renderer = System->GetRenderer<Renderers::Environment>();
				if (Renderer != nullptr)
				{
					F1.Width = (unsigned int)Renderer->Size;
					F1.Height = (unsigned int)Renderer->Size;
					F1.MipLevels = (unsigned int)Renderer->MipLevels;
					F2.Width = (unsigned int)Renderer->Size;
					F2.Height = (unsigned int)Renderer->Size;
					F2.MipLevels = (unsigned int)Renderer->MipLevels;
					MipLevels2 = (float)Renderer->MipLevels;
				}

				TH_RELEASE(Surface2);
				Surface2 = System->GetDevice()->CreateMultiRenderTarget2D(F1);

				TH_RELEASE(Input2);
				Input2 = System->GetDevice()->CreateRenderTarget2D(F2);
			}
		
			Depth::Depth(RenderSystem* Lab) : TimingDraw(Lab), ShadowDistance(0.5f)
			{
				Tick.Delay = 10;
			}
			Depth::~Depth()
			{
				for (auto It = Renderers.PointLight.begin(); It != Renderers.PointLight.end(); It++)
					delete *It;

				for (auto It = Renderers.SpotLight.begin(); It != Renderers.SpotLight.end(); It++)
					delete *It;

				for (auto It = Renderers.LineLight.begin(); It != Renderers.LineLight.end(); It++)
				{
					if (*It != nullptr)
					{
						for (auto* Target : *(*It))
							delete Target;
					}

					delete *It;
				}
				
				if (!System || !System->GetScene())
					return;

				Rest::Pool<Component*>* Lights = System->GetScene()->GetComponents<Components::PointLight>();
				for (auto It = Lights->Begin(); It != Lights->End(); It++)
					(*It)->As<Components::PointLight>()->SetDepthCache(nullptr);

				Lights = System->GetScene()->GetComponents<Components::SpotLight>();
				for (auto It = Lights->Begin(); It != Lights->End(); It++)
					(*It)->As<Components::SpotLight>()->SetDepthCache(nullptr);

				Lights = System->GetScene()->GetComponents<Components::LineLight>();
				for (auto It = Lights->Begin(); It != Lights->End(); It++)
					(*It)->As<Components::LineLight>()->SetDepthCache(nullptr);
			}
			void Depth::Deserialize(ContentManager* Content, Rest::Document* Node)
			{
				NMake::Unpack(Node->Find("point-light-resolution"), &Renderers.PointLightResolution);
				NMake::Unpack(Node->Find("point-light-limits"), &Renderers.PointLightLimits);
				NMake::Unpack(Node->Find("spot-light-resolution"), &Renderers.SpotLightResolution);
				NMake::Unpack(Node->Find("spot-light-limits"), &Renderers.SpotLightLimits);
				NMake::Unpack(Node->Find("line-light-resolution"), &Renderers.LineLightResolution);
				NMake::Unpack(Node->Find("line-light-limits"), &Renderers.LineLightLimits);
				NMake::Unpack(Node->Find("shadow-distance"), &ShadowDistance);
			}
			void Depth::Serialize(ContentManager* Content, Rest::Document* Node)
			{
				NMake::Pack(Node->SetDocument("point-light-resolution"), Renderers.PointLightResolution);
				NMake::Pack(Node->SetDocument("point-light-limits"), Renderers.PointLightLimits);
				NMake::Pack(Node->SetDocument("spot-light-resolution"), Renderers.SpotLightResolution);
				NMake::Pack(Node->SetDocument("spot-light-limits"), Renderers.SpotLightLimits);
				NMake::Pack(Node->SetDocument("line-light-resolution"), Renderers.LineLightResolution);
				NMake::Pack(Node->SetDocument("line-light-limits"), Renderers.LineLightLimits);
				NMake::Pack(Node->SetDocument("shadow-distance"), ShadowDistance);
			}
			void Depth::Activate()
			{
				PointLights = System->AddCull<Engine::Components::PointLight>();
				SpotLights = System->AddCull<Engine::Components::SpotLight>();
				LineLights = System->GetScene()->GetComponents<Engine::Components::LineLight>();

				for (auto It = PointLights->Begin(); It != PointLights->End(); It++)
					(*It)->As<Engine::Components::PointLight>()->SetDepthCache(nullptr);

				for (auto It = SpotLights->Begin(); It != SpotLights->End(); It++)
					(*It)->As<Engine::Components::SpotLight>()->SetDepthCache(nullptr);

				for (auto It = LineLights->Begin(); It != LineLights->End(); It++)
					(*It)->As<Engine::Components::LineLight>()->SetDepthCache(nullptr);

				for (auto It = Renderers.PointLight.begin(); It != Renderers.PointLight.end(); It++)
					delete *It;

				for (auto It = Renderers.SpotLight.begin(); It != Renderers.SpotLight.end(); It++)
					delete *It;

				for (auto It = Renderers.LineLight.begin(); It != Renderers.LineLight.end(); It++)
					delete *It;

				Renderers.PointLight.resize(Renderers.PointLightLimits);
				for (auto It = Renderers.PointLight.begin(); It != Renderers.PointLight.end(); It++)
				{
					Graphics::MultiRenderTargetCube::Desc F = Graphics::MultiRenderTargetCube::Desc();
					F.Size = (unsigned int)Renderers.PointLightResolution;
					GetDepthFormat(F.FormatMode);
					GetDepthTarget(&F.Target);

					*It = System->GetDevice()->CreateMultiRenderTargetCube(F);
				}

				Renderers.SpotLight.resize(Renderers.SpotLightLimits);
				for (auto It = Renderers.SpotLight.begin(); It != Renderers.SpotLight.end(); It++)
				{
					Graphics::MultiRenderTarget2D::Desc F = Graphics::MultiRenderTarget2D::Desc();
					F.Width = (unsigned int)Renderers.SpotLightResolution;
					F.Height = (unsigned int)Renderers.SpotLightResolution;
					GetDepthFormat(F.FormatMode);
					GetDepthTarget(&F.Target);

					*It = System->GetDevice()->CreateMultiRenderTarget2D(F);
				}

				Renderers.LineLight.resize(Renderers.LineLightLimits);
				for (auto It = Renderers.LineLight.begin(); It != Renderers.LineLight.end(); It++)
					*It = nullptr;
			}
			void Depth::Deactivate()
			{
				PointLights = System->RemoveCull<Engine::Components::PointLight>();
				SpotLights = System->RemoveCull<Engine::Components::SpotLight>();
			}
			void Depth::TickRender(Rest::Timer* Time, RenderState State, RenderOpt Options)
			{
				if (State != RenderState_Geometry || Options & RenderOpt_Inner || Options & RenderOpt_Transparent)
					return;

				Graphics::GraphicsDevice* Device = System->GetDevice();
				SceneGraph* Scene = System->GetScene();
				float D = 0.0f;

				uint64_t Shadows = 0;
				for (auto It = PointLights->Begin(); It != PointLights->End(); It++)
				{
					Engine::Components::PointLight* Light = (Engine::Components::PointLight*)*It;
					if (Shadows >= Renderers.PointLight.size())
						break;

					Light->SetDepthCache(nullptr);
					if (!Light->Shadow.Enabled)
						continue;
					
					if (!System->PassCullable(Light, CullResult_Always, &D) || D < ShadowDistance)
						continue;

					Graphics::MultiRenderTargetCube* Target = Renderers.PointLight[Shadows];
					Device->SetTarget(Target, 0, 0, 0);
					Device->ClearDepth(Target);

					Light->AssembleDepthOrigin();
					Scene->SetView(Compute::Matrix4x4::Identity(), Light->Projection, Light->GetEntity()->Transform->Position, 0.1f, Light->Shadow.Distance, true);
					Scene->RenderFluxCubic(Time);

					Light->SetDepthCache(Target);
					Shadows++;
				}

				Shadows = 0;
				for (auto It = SpotLights->Begin(); It != SpotLights->End(); It++)
				{
					Engine::Components::SpotLight* Light = (Engine::Components::SpotLight*)*It;
					if (Shadows >= Renderers.SpotLight.size())
						break;

					Light->SetDepthCache(nullptr);
					if (!Light->Shadow.Enabled)
						continue;

					if (!System->PassCullable(Light, CullResult_Always, &D) || D < ShadowDistance)
						continue;

					Graphics::MultiRenderTarget2D* Target = Renderers.SpotLight[Shadows];
					Device->SetTarget(Target, 0, 0, 0);
					Device->ClearDepth(Target);

					Light->AssembleDepthOrigin();
					Scene->SetView(Light->View, Light->Projection, Light->GetEntity()->Transform->Position, 0.1f, Light->Shadow.Distance, true);
					Scene->RenderFluxLinear(Time);

					Light->SetDepthCache(Target);
					Shadows++;
				}

				Shadows = 0;
				for (auto It = LineLights->Begin(); It != LineLights->End(); It++)
				{
					Engine::Components::LineLight* Light = (Engine::Components::LineLight*)*It;
					if (Shadows >= Renderers.LineLight.size())
						break;

					Light->SetDepthCache(nullptr);
					if (!Light->Shadow.Enabled)
						continue;

					CascadeMap*& Target = Renderers.LineLight[Shadows];
					if (Light->Shadow.Cascades < 1 || Light->Shadow.Cascades > 6)
						continue;

					if (!Target || Target->size() < Light->Shadow.Cascades)
						GenerateCascadeMap(&Target, Light->Shadow.Cascades);

					Light->AssembleDepthOrigin();
					for (size_t i = 0; i < Target->size(); i++)
					{
						Graphics::MultiRenderTarget2D* Cascade = (*Target)[i];
						Device->SetTarget(Cascade, 0, 0, 0);
						Device->ClearDepth(Cascade);

						Scene->SetView(Light->View[i], Light->Projection[i], 0.0f, 0.1f, Light->Shadow.Distance[i], true);
						Scene->RenderFluxLinear(Time);
					}

					Light->SetDepthCache(Target);
					Shadows++;
				}

				Scene->RestoreViewBuffer(nullptr);
			}
			void Depth::GenerateCascadeMap(CascadeMap** Result, uint32_t Size)
			{
				CascadeMap* Target = (*Result ? *Result : new CascadeMap());
				for (auto It = Target->begin(); It != Target->end(); It++)
					delete *It;

				Target->resize(Size);
				for (auto It = Target->begin(); It != Target->end(); It++)
				{
					Graphics::MultiRenderTarget2D::Desc F = Graphics::MultiRenderTarget2D::Desc();
					F.Width = (unsigned int)Renderers.LineLightResolution;
					F.Height = (unsigned int)Renderers.LineLightResolution;
					GetDepthFormat(F.FormatMode);
					GetDepthTarget(&F.Target);
		
					*It = System->GetDevice()->CreateMultiRenderTarget2D(F);
				}

				*Result = Target;
			}
			void Depth::GetDepthFormat(Graphics::Format* Result)
			{
				Result[0] = Graphics::Format_R32G32_Float;
				Result[1] = Graphics::Format_R16G16B16A16_Float;
				Result[2] = Graphics::Format_R16G16B16A16_Float;
			}
			void Depth::GetDepthTarget(Graphics::SurfaceTarget* Result)
			{
				*Result = Graphics::SurfaceTarget2;
			}

			Environment::Environment(RenderSystem* Lab) : Renderer(Lab), Surface(nullptr), Size(128), MipLevels(7)
			{
			}
			Environment::~Environment()
			{
				TH_RELEASE(Surface);
			}
			void Environment::Deserialize(ContentManager* Content, Rest::Document* Node)
			{
				NMake::Unpack(Node->Find("size"), &Size);
				NMake::Unpack(Node->Find("mip-levels"), &MipLevels);
			}
			void Environment::Serialize(ContentManager* Content, Rest::Document* Node)
			{
				NMake::Pack(Node->SetDocument("size"), Size);
				NMake::Pack(Node->SetDocument("mip-levels"), MipLevels);
			}
			void Environment::Activate()
			{
				ReflectionProbes = System->AddCull<Engine::Components::ReflectionProbe>();
				CreateRenderTarget();
			}
			void Environment::Deactivate()
			{
				ReflectionProbes = System->RemoveCull<Engine::Components::ReflectionProbe>();
			}
			void Environment::Render(Rest::Timer* Time, RenderState State, RenderOpt Options)
			{
				if (State != RenderState_Geometry || Options & RenderOpt_Inner || Options & RenderOpt_Transparent)
					return;

				SceneGraph* Scene = System->GetScene();
				Graphics::GraphicsDevice* Device = System->GetDevice();
				Graphics::MultiRenderTarget2D* S = Scene->GetSurface();
				Scene->SwapSurface(Surface);
				Scene->SetSurface();

				double ElapsedTime = Time->GetElapsedTime();
				for (auto It = ReflectionProbes->Begin(); It != ReflectionProbes->End(); It++)
				{
					Engine::Components::ReflectionProbe* Light = (Engine::Components::ReflectionProbe*)*It;
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

					Device->CopyBegin(Surface, 0, MipLevels, Size);
					Light->Locked = true;

					Compute::Vector3 Position = Light->GetEntity()->Transform->Position * Light->Offset;
					for (unsigned int j = 0; j < 6; j++)
					{
						Light->View[j] = Compute::Matrix4x4::CreateCubeMapLookAt(j, Position);
						Scene->SetView(Light->View[j], Light->Projection, Position, 0.1f, Light->Range, true);
						Scene->ClearSurface();
						Scene->RenderGeometry(Time, Light->StaticMask ? RenderOpt_Inner | RenderOpt_Static : RenderOpt_Inner);
						Device->CopyFace(Surface, 0, j);
					}

					Light->Locked = false;
					Device->CopyEnd(Surface, Cache);
				}

				Scene->SwapSurface(S);
				Scene->RestoreViewBuffer(nullptr);
			}
			void Environment::CreateRenderTarget()
			{
				Map = Size;

				Graphics::MultiRenderTarget2D::Desc F;
				System->GetScene()->GetTargetDesc(&F);

				F.MipLevels = System->GetDevice()->GetMipLevel((unsigned int)Size, (unsigned int)Size);
				F.Width = (unsigned int)Size;
				F.Height = (unsigned int)Size;

				if (MipLevels > F.MipLevels)
					MipLevels = F.MipLevels;
				else
					F.MipLevels = (unsigned int)MipLevels;

				TH_RELEASE(Surface);
				Surface = System->GetDevice()->CreateMultiRenderTarget2D(F);
			}
			void Environment::SetCaptureSize(size_t NewSize)
			{
				Size = NewSize;
				CreateRenderTarget();
			}

			Lighting::Lighting(RenderSystem* Lab) : Renderer(Lab), RecursiveProbes(true), Input1(nullptr), Input2(nullptr), Output1(nullptr), Output2(nullptr)
			{
				DepthStencil = Lab->GetDevice()->GetDepthStencilState("none");
				FrontRasterizer = Lab->GetDevice()->GetRasterizerState("cull-front");
				BackRasterizer = Lab->GetDevice()->GetRasterizerState("cull-back");
				Blend = Lab->GetDevice()->GetBlendState("additive");
				ShadowSampler = Lab->GetDevice()->GetSamplerState("shadow");
				WrapSampler = Lab->GetDevice()->GetSamplerState("trilinear-x16");
				Layout = Lab->GetDevice()->GetInputLayout("shape-vertex");

				Graphics::Shader::Desc I = Graphics::Shader::Desc();
				if (System->GetDevice()->GetSection("light/direct/point", &I.Data))
					Shaders.Point[0] = System->CompileShader("lr-point-direct", I, sizeof(PointLight));

				if (System->GetDevice()->GetSection("light/indirect/point", &I.Data))
					Shaders.Point[1] = System->CompileShader("lr-point-indirect", I, 0);

				if (System->GetDevice()->GetSection("light/direct/spot", &I.Data))
					Shaders.Spot[0] = System->CompileShader("lr-spot-direct", I, sizeof(SpotLight));

				if (System->GetDevice()->GetSection("light/indirect/spot", &I.Data))
					Shaders.Spot[1] = System->CompileShader("lr-spot-indirect", I, 0);

				if (System->GetDevice()->GetSection("light/direct/line", &I.Data))
					Shaders.Line[0] = System->CompileShader("lr-line-direct", I, sizeof(LineLight));

				if (System->GetDevice()->GetSection("light/indirect/line", &I.Data))
					Shaders.Line[1] = System->CompileShader("lr-line-indirect", I, 0);

				if (System->GetDevice()->GetSection("light/probe", &I.Data))
					Shaders.Probe = System->CompileShader("lr-probe", I, sizeof(ReflectionProbe));

				if (System->GetDevice()->GetSection("light/ambient", &I.Data))
					Shaders.Ambient = System->CompileShader("lr-ambient", I, sizeof(AmbientLight));
			}
			Lighting::~Lighting()
			{
				System->FreeShader("lr-point-direct", Shaders.Point[0]);
				System->FreeShader("lr-point-indirect", Shaders.Point[1]);
				System->FreeShader("lr-spot-direct", Shaders.Spot[0]);
				System->FreeShader("lr-spot-indirect", Shaders.Spot[1]);
				System->FreeShader("lr-line-direct", Shaders.Line[0]);
				System->FreeShader("lr-line-indirect", Shaders.Line[1]);
				System->FreeShader("lr-probe", Shaders.Probe);
				System->FreeShader("lr-ambient", Shaders.Ambient);
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
				NMake::Unpack(Node->Find("recursive-probes"), &RecursiveProbes);
			}
			void Lighting::Serialize(ContentManager* Content, Rest::Document* Node)
			{
				AssetResource* Asset = Content->FindAsset(SkyBase);
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
				NMake::Pack(Node->SetDocument("recursive-probes"), RecursiveProbes);
			}
			void Lighting::Activate()
			{
				ReflectionProbes = System->AddCull<Engine::Components::ReflectionProbe>();
				PointLights = System->AddCull<Engine::Components::PointLight>();
				SpotLights = System->AddCull<Engine::Components::SpotLight>();
				LineLights = System->GetScene()->GetComponents<Engine::Components::LineLight>();

				Depth* Depth = System->GetRenderer<Renderers::Depth>();
				if (Depth != nullptr)
				{
					Quality.PointLight = (float)Depth->Renderers.PointLightResolution;
					Quality.SpotLight = (float)Depth->Renderers.SpotLightResolution;
					Quality.LineLight = (float)Depth->Renderers.LineLightResolution;
				}

				Environment* Probe = System->GetRenderer<Renderers::Environment>();
				if (Probe != nullptr)
					ReflectionProbe.MipLevels = (float)Probe->MipLevels;

				ResizeBuffers();
			}
			void Lighting::Deactivate()
			{
				ReflectionProbes = System->RemoveCull<Engine::Components::ReflectionProbe>();
				PointLights = System->RemoveCull<Engine::Components::PointLight>();
				SpotLights = System->RemoveCull<Engine::Components::SpotLight>();
			}
			void Lighting::Render(Rest::Timer* Time, RenderState State, RenderOpt Options)
			{
				if (State != RenderState_Geometry)
					return;

				Graphics::GraphicsDevice* Device = System->GetDevice();
				Graphics::MultiRenderTarget2D* Surface = System->GetScene()->GetSurface();
				Graphics::Shader* Active = nullptr;
				bool Inner = (Options & RenderOpt_Inner);
				float D = 0.0f;

				AmbientLight.SkyOffset = System->GetScene()->View.Projection.Invert() * Compute::Matrix4x4::CreateRotation(System->GetScene()->View.WorldRotation);
				Device->SetSamplerState(WrapSampler, 0);
				Device->SetSamplerState(ShadowSampler, 1);
				Device->SetDepthStencilState(DepthStencil);
				Device->SetBlendState(Blend);
				Device->SetRasterizerState(FrontRasterizer);
				Device->SetInputLayout(Layout);
				Device->CopyTargetTo(Surface, 0, Inner ? Input2 : Input1);
				Device->SetTarget(Inner ? Output2 : Output1, 0, 0, 0);
				Device->SetTexture2D(Inner ? Input2->GetTarget() : Input1->GetTarget(), 1);
				Device->SetTexture2D(Surface->GetTarget(1), 2);
				Device->SetTexture2D(Surface->GetTarget(2), 3);
				Device->SetTexture2D(Surface->GetTarget(3), 4);
				Device->SetIndexBuffer(System->GetCubeIBuffer(), Graphics::Format_R32_Uint);
				Device->SetVertexBuffer(System->GetCubeVBuffer(), 0);

				if (!Inner)
				{
					Device->SetShader(Shaders.Probe, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);
					Device->SetBuffer(Shaders.Probe, 3, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);

					for (auto It = ReflectionProbes->Begin(); It != ReflectionProbes->End(); ++It)
					{
						Engine::Components::ReflectionProbe* Light = (Engine::Components::ReflectionProbe*)*It;
						if (!Light->GetProbeCache() || !System->PassCullable(Light, CullResult_Always, &D))
							continue;

						ReflectionProbe.Range = Light->GetRange();
						ReflectionProbe.WorldViewProjection = Compute::Matrix4x4::CreateTranslatedScale(Light->GetEntity()->Transform->Position, ReflectionProbe.Range) * System->GetScene()->View.ViewProjection;
						ReflectionProbe.Position = Light->GetEntity()->Transform->Position.InvertZ();
						ReflectionProbe.Lighting = Light->Diffuse.Mul(Light->Emission * D);
						ReflectionProbe.Scale = Light->GetEntity()->Transform->Scale;
						ReflectionProbe.Parallax = (Light->Parallax ? 1.0f : 0.0f);
						ReflectionProbe.Infinity = Light->Infinity;

						Device->SetTextureCube(Light->GetProbeCache(), 5);
						Device->UpdateBuffer(Shaders.Probe, &ReflectionProbe);
						Device->DrawIndexed((unsigned int)System->GetCubeIBuffer()->GetElements(), 0, 0);
					}
				}
				else if (RecursiveProbes)
				{
					Device->SetShader(Shaders.Probe, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);
					Device->SetBuffer(Shaders.Probe, 3, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);

					for (auto It = ReflectionProbes->Begin(); It != ReflectionProbes->End(); ++It)
					{
						Engine::Components::ReflectionProbe* Light = (Engine::Components::ReflectionProbe*)*It;
						if (Light->Locked || !Light->GetProbeCache() || !System->PassCullable(Light, CullResult_Always, &D))
							continue;

						ReflectionProbe.Range = Light->GetRange();
						ReflectionProbe.WorldViewProjection = Compute::Matrix4x4::CreateTranslatedScale(Light->GetEntity()->Transform->Position, ReflectionProbe.Range) * System->GetScene()->View.ViewProjection;
						ReflectionProbe.Position = Light->GetEntity()->Transform->Position.InvertZ();
						ReflectionProbe.Lighting = Light->Diffuse.Mul(Light->Emission * D);
						ReflectionProbe.Scale = Light->GetEntity()->Transform->Scale;
						ReflectionProbe.Parallax = (Light->Parallax ? 1.0f : 0.0f);
						ReflectionProbe.Infinity = Light->Infinity;

						Device->SetTextureCube(Light->GetProbeCache(), 5);
						Device->UpdateBuffer(Shaders.Probe, &ReflectionProbe);
						Device->DrawIndexed((unsigned int)System->GetCubeIBuffer()->GetElements(), 0, 0);
					}
				}

				Device->SetShader(Shaders.Point[0], Graphics::ShaderType_Vertex);
				Device->SetBuffer(Shaders.Point[0], 3, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);

				for (auto It = PointLights->Begin(); It != PointLights->End(); ++It)
				{
					Engine::Components::PointLight* Light = (Engine::Components::PointLight*)*It;
					if (!System->PassCullable(Light, CullResult_Always, &D))
						continue;

					Graphics::MultiRenderTargetCube* Depth = Light->GetDepthCache();
					if (Light->Shadow.Enabled && Depth != nullptr)
					{
						PointLight.Softness = Light->Shadow.Softness <= 0 ? 0 : Quality.PointLight / Light->Shadow.Softness;
						PointLight.Bias = Light->Shadow.Bias;
						PointLight.Distance = Light->Shadow.Distance;
						PointLight.Iterations = (float)Light->Shadow.Iterations;
						PointLight.Umbra = Light->Disperse;
						Active = Shaders.Point[1];

						Device->SetTexture2D(Depth->GetTarget(0), 5);
					}
					else
						Active = Shaders.Point[0];

					PointLight.Range = Light->GetRange();
					PointLight.WorldViewProjection = Compute::Matrix4x4::CreateTranslatedScale(Light->GetEntity()->Transform->Position, PointLight.Range) * System->GetScene()->View.ViewProjection;
					PointLight.Position = Light->GetEntity()->Transform->Position.InvertZ();
					PointLight.Lighting = Light->Diffuse.Mul(Light->Emission * D);
					
					Device->SetShader(Active, Graphics::ShaderType_Pixel);
					Device->UpdateBuffer(Shaders.Point[0], &PointLight);
					Device->DrawIndexed((unsigned int)System->GetCubeIBuffer()->GetElements(), 0, 0);
				}

				Device->SetShader(Shaders.Spot[0], Graphics::ShaderType_Vertex);
				Device->SetBuffer(Shaders.Spot[0], 3, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);

				for (auto It = SpotLights->Begin(); It != SpotLights->End(); ++It)
				{
					Engine::Components::SpotLight* Light = (Engine::Components::SpotLight*)*It;
					if (!System->PassCullable(Light, CullResult_Always, &D))
						continue;

					Graphics::MultiRenderTarget2D* Depth = Light->GetDepthCache();
					if (Light->Shadow.Enabled && Depth != nullptr)
					{
						SpotLight.Softness = Light->Shadow.Softness <= 0 ? 0 : Quality.SpotLight / Light->Shadow.Softness;
						SpotLight.Bias = Light->Shadow.Bias;
						SpotLight.Iterations = (float)Light->Shadow.Iterations;
						SpotLight.Umbra = Light->Disperse;
						Active = Shaders.Spot[1];

						Device->SetTexture2D(Depth->GetTarget(0), 5);
					}
					else
						Active = Shaders.Spot[0];

					SpotLight.Range = Light->GetRange();
					SpotLight.WorldViewProjection = Compute::Matrix4x4::CreateScale(SpotLight.Range) * Compute::Matrix4x4::CreateTranslation(Light->GetEntity()->Transform->Position) * System->GetScene()->View.ViewProjection;
					SpotLight.ViewProjection = Light->View * Light->Projection;
					SpotLight.Direction = Light->GetEntity()->Transform->Rotation.DepthDirection();
					SpotLight.Position = Light->GetEntity()->Transform->Position.InvertZ();
					SpotLight.Lighting = Light->Diffuse.Mul(Light->Emission * D);
					SpotLight.Cutoff = Compute::Mathf::Cos(Compute::Mathf::Deg2Rad() * Light->Cutoff * 0.5f);

					Device->SetShader(Active, Graphics::ShaderType_Pixel);
					Device->UpdateBuffer(Shaders.Spot[0], &SpotLight);
					Device->DrawIndexed((unsigned int)System->GetCubeIBuffer()->GetElements(), 0, 0);
				}

				Device->SetRasterizerState(BackRasterizer);
				Device->SetShader(Shaders.Line[0], Graphics::ShaderType_Vertex);
				Device->SetBuffer(Shaders.Line[0], 3, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);
				Device->SetVertexBuffer(System->GetQuadVBuffer(), 0);

				for (auto It = LineLights->Begin(); It != LineLights->End(); ++It)
				{
					Engine::Components::LineLight* Light = (Engine::Components::LineLight*)*It;
					Depth::CascadeMap* Depth = Light->GetDepthCache();

					if (Light->Shadow.Enabled && Depth != nullptr)
					{
						LineLight.Softness = Light->Shadow.Softness <= 0 ? 0 : Quality.LineLight / Light->Shadow.Softness;
						LineLight.Iterations = (float)Light->Shadow.Iterations;
						LineLight.Bias = Light->Shadow.Bias;
						LineLight.Cascades = Depth->size();
						LineLight.Umbra = Light->Disperse;
						Active = Shaders.Line[1];

						for (size_t i = 0; i < Depth->size(); i++)
						{
							LineLight.ViewProjection[i] = Light->View[i] * Light->Projection[i];
							Device->SetTexture2D((*Depth)[i]->GetTarget(0), 5 + i);
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

				Device->SetTarget(Surface, 0, 0, 0, 0);
				Device->ClearDepth(Surface);
				Device->SetTexture2D(Inner ? Output2->GetTarget() : Output1->GetTarget(), 5);
				Device->SetTextureCube(SkyMap, 6);
				Device->SetShader(Shaders.Ambient, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);
				Device->SetBuffer(Shaders.Ambient, 3, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);
				Device->UpdateBuffer(Shaders.Ambient, &AmbientLight);
				Device->Draw(6, 0);
				Device->FlushTexture2D(1, 10);
			}
			void Lighting::ResizeBuffers()
			{
				Graphics::RenderTarget2D::Desc F;
				System->GetScene()->GetTargetDesc(&F);

				TH_RELEASE(Output1);
				Output1 = System->GetDevice()->CreateRenderTarget2D(F);

				TH_RELEASE(Input1);
				Input1 = System->GetDevice()->CreateRenderTarget2D(F);

				auto* Renderer = System->GetRenderer<Renderers::Environment>();
				if (Renderer != nullptr)
				{
					F.Width = (unsigned int)Renderer->Size;
					F.Height = (unsigned int)Renderer->Size;
					F.MipLevels = (unsigned int)Renderer->MipLevels;
				}

				TH_RELEASE(Output2);
				Output2 = System->GetDevice()->CreateRenderTarget2D(F);

				TH_RELEASE(Input2);
				Input2 = System->GetDevice()->CreateRenderTarget2D(F);
			}
			void Lighting::SetSkyMap(Graphics::Texture2D* Cubemap)
			{
				TH_RELEASE(SkyBase);
				SkyBase = Cubemap;

				TH_CLEAR(SkyMap);
				if (SkyBase != nullptr)
					SkyMap = System->GetDevice()->CreateTextureCube(SkyBase);
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

			Glitch::Glitch(RenderSystem* Lab) : EffectDraw(Lab), ScanLineJitter(0), VerticalJump(0), HorizontalShake(0), ColorDrift(0)
			{
				std::string Data;
				if (System->GetDevice()->GetSection("effects/glitch", &Data))
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

			Tone::Tone(RenderSystem* Lab) : EffectDraw(Lab)
			{
				std::string Data;
				if (System->GetDevice()->GetSection("effects/tone", &Data))
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

			DoF::DoF(RenderSystem* Lab) : EffectDraw(Lab)
			{
				std::string Data;
				if (System->GetDevice()->GetSection("effects/focus", &Data))
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
				if (System->GetDevice()->GetSection("effects/bloom-x", &Data))
					Pass1 = CompileEffect("br-bloom-x", Data, sizeof(RenderPass));

				if (System->GetDevice()->GetSection("effects/bloom-y", &Data))
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

			SSR::SSR(RenderSystem* Lab) : EffectDraw(Lab), Pass1(nullptr), Pass2(nullptr)
			{
				std::string Data;
				if (System->GetDevice()->GetSection("effects/reflection", &Data))
					Pass1 = CompileEffect("rr-reflection", Data, sizeof(RenderPass1));

				if (System->GetDevice()->GetSection("effects/gloss-x", &Data))
					Pass2 = CompileEffect("rr-gloss-x", Data, sizeof(RenderPass2));

				if (System->GetDevice()->GetSection("effects/gloss-y", &Data))
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

			SSAO::SSAO(RenderSystem* Lab) : EffectDraw(Lab), Pass1(nullptr), Pass2(nullptr)
			{
				std::string Data;
				if (System->GetDevice()->GetSection("effects/ambient", &Data))
					Pass1 = CompileEffect("aor-ambient", Data, sizeof(RenderPass1));

				if (System->GetDevice()->GetSection("effects/blur-x", &Data))
					Pass2 = CompileEffect("aor-blur-x", Data, sizeof(RenderPass2));

				if (System->GetDevice()->GetSection("effects/blur-y", &Data))
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

			SSDO::SSDO(RenderSystem* Lab) : EffectDraw(Lab), Pass1(nullptr), Pass2(nullptr)
			{
				std::string Data;
				if (System->GetDevice()->GetSection("effects/indirect", &Data))
					Pass1 = CompileEffect("dor-indirect", Data, sizeof(RenderPass1));

				if (System->GetDevice()->GetSection("effects/blur-x", &Data))
					Pass2 = CompileEffect("dor-blur-x", Data, sizeof(RenderPass2));

				if (System->GetDevice()->GetSection("effects/blur-y", &Data))
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
				if (State != RenderState_Geometry || Options & RenderOpt_Inner || Options & RenderOpt_Transparent)
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
