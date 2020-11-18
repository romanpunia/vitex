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
				if (System->GetDevice()->GetSection("geometry/model/gbuffer", &I.Data))
					Shaders.GBuffer = System->CompileShader("mr-gbuffer", I, 0);

				if (System->GetDevice()->GetSection("geometry/model/depth-linear", &I.Data))
					Shaders.Linear = System->CompileShader("mr-depth-linear", I, 0);

				if (System->GetDevice()->GetSection("geometry/model/depth-cubic", &I.Data))
					Shaders.Cubic = System->CompileShader("mr-depth-cubic", I, sizeof(Depth));

				if (System->GetDevice()->GetSection("geometry/model/occlusion", &I.Data))
					Shaders.Occlusion = System->CompileShader("mr-occlusion", I, 0);
			}
			Model::~Model()
			{
				System->FreeShader("mr-gbuffer", Shaders.GBuffer);
				System->FreeShader("mr-depth-linear", Shaders.Linear);
				System->FreeShader("mr-depth-cubic", Shaders.Cubic);
				System->FreeShader("mr-occlusion", Shaders.Occlusion);
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
			void Model::RenderGBuffer(Rest::Timer* Time, Rest::Pool<Drawable*>* Geometry, RenderOpt Options)
			{
				Graphics::GraphicsDevice* Device = System->GetDevice();
				CullResult Cull = (Options & RenderOpt_Inner ? CullResult_Always : CullResult_Last);
				bool Static = (Options & RenderOpt_Static);

				Device->SetDepthStencilState(DepthStencil);
				Device->SetSamplerState(Sampler);
				Device->SetBlendState(Blend);
				Device->SetRasterizerState(BackRasterizer);
				Device->SetInputLayout(Layout);
				Device->SetShader(Shaders.GBuffer, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);
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
						if (!Surface || !Surface->FillGBuffer(Device))
							continue;

						Device->Render.World = Mesh->World * Base->GetEntity()->Transform->GetWorld();
						Device->Render.WorldViewProjection = Device->Render.World * System->GetScene()->View.ViewProjection;
						Device->UpdateBuffer(Graphics::RenderBufferType_Render);
						Device->DrawIndexed(Mesh);
					}
				}

				Device->FlushTexture2D(1, 7);
			}
			void Model::RenderDepthLinear(Rest::Timer* Time, Rest::Pool<Drawable*>* Geometry)
			{
				Graphics::GraphicsDevice* Device = System->GetDevice();
				Device->SetDepthStencilState(DepthStencil);
				Device->SetSamplerState(Sampler);
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
						if (!Surface || !Surface->FillLinearDepth(Device))
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
				memcpy(Depth.FaceViewProjection, ViewProjection, sizeof(Compute::Matrix4x4) * 6);
				Device->SetDepthStencilState(DepthStencil);
				Device->SetSamplerState(Sampler);
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
						if (!Surface || !Surface->FillCubicDepth(Device))
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
				if (System->GetDevice()->GetSection("geometry/skin/gbuffer", &I.Data))
					Shaders.GBuffer = System->CompileShader("sr-gbuffer", I, 0);

				if (System->GetDevice()->GetSection("geometry/skin/depth-linear", &I.Data))
					Shaders.Linear = System->CompileShader("sr-depth-linear", I, 0);

				if (System->GetDevice()->GetSection("geometry/skin/depth-cubic", &I.Data))
					Shaders.Cubic = System->CompileShader("sr-depth-cubic", I, sizeof(Depth));

				if (System->GetDevice()->GetSection("geometry/skin/occlusion", &I.Data))
					Shaders.Occlusion = System->CompileShader("sr-occlusion", I, 0);
			}
			Skin::~Skin()
			{
				System->FreeShader("sr-gbuffer", Shaders.GBuffer);
				System->FreeShader("sr-depth-linear", Shaders.Linear);
				System->FreeShader("sr-depth-cubic", Shaders.Cubic);
				System->FreeShader("sr-occlusion", Shaders.Occlusion);
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
			void Skin::RenderGBuffer(Rest::Timer* Time, Rest::Pool<Drawable*>* Geometry, RenderOpt Options)
			{
				Graphics::GraphicsDevice* Device = System->GetDevice();
				CullResult Cull = (Options & RenderOpt_Inner ? CullResult_Always : CullResult_Last);
				bool Static = (Options & RenderOpt_Static);

				Device->SetDepthStencilState(DepthStencil);
				Device->SetSamplerState(Sampler);
				Device->SetBlendState(Blend);
				Device->SetRasterizerState(BackRasterizer);
				Device->SetInputLayout(Layout);
				Device->SetShader(Shaders.GBuffer, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);
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
						if (!Surface || !Surface->FillGBuffer(Device))
							continue;

						Device->Render.World = Mesh->World * Base->GetEntity()->Transform->GetWorld();
						Device->Render.WorldViewProjection = Device->Render.World * System->GetScene()->View.ViewProjection;
						Device->UpdateBuffer(Graphics::RenderBufferType_Render);
						Device->DrawIndexed(Mesh);
					}
				}

				Device->FlushTexture2D(1, 7);
			}
			void Skin::RenderDepthLinear(Rest::Timer* Time, Rest::Pool<Drawable*>* Geometry)
			{
				Graphics::GraphicsDevice* Device = System->GetDevice();
				Device->SetDepthStencilState(DepthStencil);
				Device->SetSamplerState(Sampler);
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
						if (!Surface || !Surface->FillLinearDepth(Device))
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
				memcpy(Depth.FaceViewProjection, ViewProjection, sizeof(Compute::Matrix4x4) * 6);
				Device->SetDepthStencilState(DepthStencil);
				Device->SetSamplerState(Sampler);
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
						if (!Surface || !Surface->FillCubicDepth(Device))
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
				if (System->GetDevice()->GetSection("geometry/model/gbuffer", &I.Data))
					Shaders.GBuffer = System->CompileShader("mr-gbuffer", I, 0);

				if (System->GetDevice()->GetSection("geometry/model/depth-linear", &I.Data))
					Shaders.Linear = System->CompileShader("mr-depth-linear", I, 0);

				if (System->GetDevice()->GetSection("geometry/model/depth-cubic", &I.Data))
					Shaders.Cubic = System->CompileShader("mr-depth-cubic", I, sizeof(Depth));

				if (System->GetDevice()->GetSection("geometry/model/occlusion", &I.Data))
					Shaders.Occlusion = System->CompileShader("mr-occlusion", I, 0);
			}
			SoftBody::~SoftBody()
			{
				System->FreeShader("mr-gbuffer", Shaders.GBuffer);
				System->FreeShader("mr-depth-linear", Shaders.Linear);
				System->FreeShader("mr-depth-cubic", Shaders.Cubic);
				System->FreeShader("mr-occlusion", Shaders.Occlusion);
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
			void SoftBody::RenderGBuffer(Rest::Timer* Time, Rest::Pool<Drawable*>* Geometry, RenderOpt Options)
			{
				Graphics::GraphicsDevice* Device = System->GetDevice();
				CullResult Cull = (Options & RenderOpt_Inner ? CullResult_Always : CullResult_Last);
				bool Static = (Options & RenderOpt_Static);

				Device->SetDepthStencilState(DepthStencil);
				Device->SetSamplerState(Sampler);
				Device->SetBlendState(Blend);
				Device->SetRasterizerState(BackRasterizer);
				Device->SetInputLayout(Layout);
				Device->SetShader(Shaders.GBuffer, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);
				System->GetScene()->SetSurface();

				for (auto It = Geometry->Begin(); It != Geometry->End(); ++It)
				{
					Engine::Components::SoftBody* Base = (Engine::Components::SoftBody*)*It;
					if ((Static && !Base->Static) || Base->GetIndices().empty())
						continue;

					if (!System->PassDrawable(Base, Cull, nullptr))
						continue;

					auto* Surface = Base->GetSurface();
					if (!Surface || !Surface->FillGBuffer(Device))
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
			void SoftBody::RenderDepthLinear(Rest::Timer* Time, Rest::Pool<Drawable*>* Geometry)
			{
				Graphics::GraphicsDevice* Device = System->GetDevice();
				Device->SetDepthStencilState(DepthStencil);
				Device->SetSamplerState(Sampler);
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
					if (!Surface || !Surface->FillLinearDepth(Device))
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
				memcpy(Depth.FaceViewProjection, ViewProjection, sizeof(Compute::Matrix4x4) * 6);
				Device->SetDepthStencilState(DepthStencil);
				Device->SetSamplerState(Sampler);
				Device->SetBlendState(Blend);
				Device->SetRasterizerState(FrontRasterizer);
				Device->SetInputLayout(Layout);
				Device->SetShader(Shaders.Cubic, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel | Graphics::ShaderType_Geometry);
				Device->SetBuffer(Shaders.Cubic, 3, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel | Graphics::ShaderType_Geometry);
				Device->UpdateBuffer(Shaders.Cubic, &Depth);

				for (auto It = Geometry->Begin(); It != Geometry->End(); ++It)
				{
					Engine::Components::SoftBody* Base = (Engine::Components::SoftBody*)*It;
					if (!Base->GetBody() || Base->GetEntity()->Transform->Position.Distance(System->GetScene()->View.WorldPosition) >= System->GetScene()->View.ViewDistance + Base->GetEntity()->Transform->Scale.Length())
						continue;

					auto* Surface = Base->GetSurface();
					if (!Surface || !Surface->FillCubicDepth(Device))
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
				if (System->GetDevice()->GetSection("geometry/emitter/gbuffer-opaque", &I.Data))
					Shaders.Opaque = System->CompileShader("er-gbuffer-opaque", I, 0);

				if (System->GetDevice()->GetSection("geometry/emitter/gbuffer-limpid", &I.Data))
					Shaders.Limpid = System->CompileShader("er-gbuffer-limpid", I, 0);

				if (System->GetDevice()->GetSection("geometry/emitter/depth-linear", &I.Data))
					Shaders.Linear = System->CompileShader("er-depth-linear", I, 0);

				if (System->GetDevice()->GetSection("geometry/emitter/depth-point", &I.Data))
					Shaders.Point = System->CompileShader("er-depth-point", I, 0);

				if (System->GetDevice()->GetSection("geometry/emitter/depth-quad", &I.Data))
					Shaders.Quad = System->CompileShader("er-depth-quad", I, sizeof(Depth));
			}
			Emitter::~Emitter()
			{
				System->FreeShader("er-gbuffer-opaque", Shaders.Opaque);
				System->FreeShader("er-gbuffer-limpid", Shaders.Limpid);
				System->FreeShader("er-depth-linear", Shaders.Linear);
				System->FreeShader("er-depth-point", Shaders.Point);
				System->FreeShader("er-depth-quad", Shaders.Quad);
			}
			void Emitter::Activate()
			{
				Opaque = System->AddCull<Engine::Components::Emitter>();
			}
			void Emitter::Deactivate()
			{
				Opaque = System->RemoveCull<Engine::Components::Emitter>();
			}
			void Emitter::RenderGBuffer(Rest::Timer* Time, Rest::Pool<Drawable*>* Geometry, RenderOpt Options)
			{
				if (Options & RenderOpt_Transparent || !Opaque || !Limpid)
					return;

				Graphics::GraphicsDevice* Device = System->GetDevice();
				Graphics::PrimitiveTopology T = Device->GetPrimitiveTopology();
				Compute::Matrix4x4& View = System->GetScene()->View.View;
				CullResult Cull = (Options & RenderOpt_Inner ? CullResult_Always : CullResult_Last);
				bool Static = (Options & RenderOpt_Static);

				Device->SetDepthStencilState(DepthStencilOpaque);
				Device->SetSamplerState(Sampler);
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
					if (!Surface || !Surface->FillGBuffer(Device))
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
				Device->SetShader(Shaders.Limpid, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);
				
				for (auto It = Limpid->Begin(); It != Limpid->End(); ++It)
				{
					Engine::Components::Emitter* Base = (Engine::Components::Emitter*)*It;
					if ((Static && !Base->Static) && !Base->GetBuffer())
						continue;

					if (!System->PassDrawable(Base, Cull, nullptr))
						continue;

					auto* Surface = Base->GetSurface();
					if (!Surface || !Surface->FillGBuffer(Device))
						continue;

					Device->Render.World = System->GetScene()->View.Projection;
					Device->Render.WorldViewProjection = (Base->QuadBased ? View : System->GetScene()->View.ViewProjection);

					if (Base->Connected)
						Device->Render.WorldViewProjection = Base->GetEntity()->Transform->GetWorld() * Device->Render.WorldViewProjection;

					Device->UpdateBuffer(Base->GetBuffer());
					Device->SetBuffer(Base->GetBuffer(), 8);
					Device->SetShader(Base->QuadBased ? Shaders.Limpid : nullptr, Graphics::ShaderType_Geometry);
					Device->UpdateBuffer(Graphics::RenderBufferType_Render);
					Device->Draw((unsigned int)Base->GetBuffer()->GetArray()->Size(), 0);
				}

				Device->FlushTexture2D(1, 7);
				Device->SetPrimitiveTopology(T);
				Device->SetShader(nullptr, Graphics::ShaderType_Geometry);
			}
			void Emitter::RenderDepthLinear(Rest::Timer* Time, Rest::Pool<Drawable*>* Geometry)
			{
				Graphics::GraphicsDevice* Device = System->GetDevice();
				Graphics::PrimitiveTopology T = Device->GetPrimitiveTopology();
				Compute::Matrix4x4& View = System->GetScene()->View.View;

				Device->SetDepthStencilState(DepthStencilLimpid);
				Device->SetSamplerState(Sampler);
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
					if (!Surface || !Surface->FillLinearDepth(Device))
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
				Device->SetSamplerState(Sampler);
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
					if (!Surface || !Surface->FillCubicDepth(Device))
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
				if (System->GetDevice()->GetSection("geometry/decal/gbuffer", &I.Data))
					Shader = System->CompileShader("dr-gbuffer", I, sizeof(RenderPass));
			}
			Decal::~Decal()
			{
				System->FreeShader("dr-gbuffer", Shader);
			}
			void Decal::Activate()
			{
				System->AddCull<Engine::Components::Decal>();
			}
			void Decal::Deactivate()
			{
				System->RemoveCull<Engine::Components::Decal>();
			}
			void Decal::RenderGBuffer(Rest::Timer* Time, Rest::Pool<Drawable*>* Geometry, RenderOpt Options)
			{
				Graphics::GraphicsDevice* Device = System->GetDevice();
				Rest::Pool<Component*>* Array = (Options & RenderOpt_Transparent ? Limpid : Opaque);
				CullResult Cull = (Options & RenderOpt_Inner ? CullResult_Always : CullResult_Last);
				bool Map[8] = { true, true, false, true, false, false, false, false };
				bool Static = (Options & RenderOpt_Static);

				if (!Array || Geometry->Empty())
					return;

				Device->SetDepthStencilState(DepthStencil);
				Device->SetSamplerState(Sampler);
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
					if (!Surface || !Surface->FillGBuffer(Device))
						continue;

					RenderPass.OwnViewProjection = Base->View * Base->Projection;
					Device->Render.World = Compute::Matrix4x4::CreateScale(Base->GetRange()) * Compute::Matrix4x4::CreateTranslation(Base->GetEntity()->Transform->Position);
					Device->Render.WorldViewProjection = Device->Render.World * System->GetScene()->View.ViewProjection;
					Device->UpdateBuffer(Graphics::RenderBufferType_Render);
					Device->UpdateBuffer(Shader, &RenderPass);
					Device->DrawIndexed((unsigned int)System->GetSphereIBuffer()->GetElements(), 0, 0);
				}

				Device->FlushTexture2D(1, 8);
			}
			void Decal::RenderDepthLinear(Rest::Timer* Time, Rest::Pool<Drawable*>* Geometry)
			{
			}
			void Decal::RenderDepthCubic(Rest::Timer* Time, Rest::Pool<Drawable*>* Geometry, Compute::Matrix4x4* ViewProjection)
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
				if (System->GetDevice()->GetSection("pass/limpid", &I.Data))
					Shader = System->CompileShader("lr-limpid", I, sizeof(RenderPass));
			}
			Transparency::~Transparency()
			{
				System->FreeShader("lr-limpid", Shader);
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
				if (State != RenderState_GBuffer || Options & RenderOpt_Transparent)
					return;

				SceneGraph* Scene = System->GetScene();
				if (!Scene->GetTransparentCount())
					return;

				Graphics::MultiRenderTarget2D* S = Scene->GetSurface();
				Graphics::GraphicsDevice* Device = System->GetDevice();
				RenderPass.MipLevels = (Inner ? MipLevels2 : MipLevels1);

				Scene->SwapSurface(Inner ? Surface2 : Surface1);
				Scene->SetSurfaceCleared();
				Scene->RenderGBuffer(Time, Options | RenderOpt_Transparent);
				Scene->SwapSurface(S);

				Device->CopyTargetTo(S, 0, Inner ? Input2 : Input1);
				Device->GenerateMips(Inner ? Input2->GetTarget() : Input1->GetTarget());
				Device->SetTarget(S, 0);
				Device->Clear(S, 0, 0, 0, 0);
				Device->UpdateBuffer(Shader, &RenderPass);
				Device->SetDepthStencilState(DepthStencil);
				Device->SetSamplerState(Sampler);
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
					delete *It;

				if (!System || !System->GetScene())
					return;

				Rest::Pool<Component*>* Lights = System->GetScene()->GetComponents<Components::PointLight>();
				for (auto It = Lights->Begin(); It != Lights->End(); It++)
					(*It)->As<Components::PointLight>()->SetShadowCache(nullptr);

				Lights = System->GetScene()->GetComponents<Components::SpotLight>();
				for (auto It = Lights->Begin(); It != Lights->End(); It++)
					(*It)->As<Components::SpotLight>()->SetShadowCache(nullptr);

				Lights = System->GetScene()->GetComponents<Components::LineLight>();
				for (auto It = Lights->Begin(); It != Lights->End(); It++)
					(*It)->As<Components::LineLight>()->SetShadowCache(nullptr);
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
					(*It)->As<Engine::Components::PointLight>()->SetShadowCache(nullptr);

				for (auto It = SpotLights->Begin(); It != SpotLights->End(); It++)
					(*It)->As<Engine::Components::SpotLight>()->SetShadowCache(nullptr);

				for (auto It = LineLights->Begin(); It != LineLights->End(); It++)
					(*It)->As<Engine::Components::LineLight>()->SetShadowCache(nullptr);

				for (auto It = Renderers.PointLight.begin(); It != Renderers.PointLight.end(); It++)
					delete *It;

				for (auto It = Renderers.SpotLight.begin(); It != Renderers.SpotLight.end(); It++)
					delete *It;

				for (auto It = Renderers.LineLight.begin(); It != Renderers.LineLight.end(); It++)
					delete *It;

				Renderers.PointLight.resize(Renderers.PointLightLimits);
				for (auto It = Renderers.PointLight.begin(); It != Renderers.PointLight.end(); It++)
				{
					Graphics::RenderTargetCube::Desc F = Graphics::RenderTargetCube::Desc();
					F.Size = (unsigned int)Renderers.PointLightResolution;
					F.FormatMode = Graphics::Format_R32G32_Float;

					*It = System->GetDevice()->CreateRenderTargetCube(F);
				}

				Renderers.SpotLight.resize(Renderers.SpotLightLimits);
				for (auto It = Renderers.SpotLight.begin(); It != Renderers.SpotLight.end(); It++)
				{
					Graphics::RenderTarget2D::Desc F = Graphics::RenderTarget2D::Desc();
					F.Width = (unsigned int)Renderers.SpotLightResolution;
					F.Height = (unsigned int)Renderers.SpotLightResolution;
					F.FormatMode = Graphics::Format_R32G32_Float;

					*It = System->GetDevice()->CreateRenderTarget2D(F);
				}

				Renderers.LineLight.resize(Renderers.LineLightLimits);
				for (auto It = Renderers.LineLight.begin(); It != Renderers.LineLight.end(); It++)
				{
					Graphics::RenderTarget2D::Desc F = Graphics::RenderTarget2D::Desc();
					F.Width = (unsigned int)Renderers.LineLightResolution;
					F.Height = (unsigned int)Renderers.LineLightResolution;
					F.FormatMode = Graphics::Format_R32G32_Float;

					*It = System->GetDevice()->CreateRenderTarget2D(F);
				}
			}
			void Depth::Deactivate()
			{
				PointLights = System->RemoveCull<Engine::Components::PointLight>();
				SpotLights = System->RemoveCull<Engine::Components::SpotLight>();
			}
			void Depth::TickRender(Rest::Timer* Time, RenderState State, RenderOpt Options)
			{
				if (State != RenderState_GBuffer || Options & RenderOpt_Inner || Options & RenderOpt_Transparent)
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

					Light->SetShadowCache(nullptr);
					if (!Light->Shadowed)
						continue;
					
					if (!System->PassCullable(Light, CullResult_Always, &D) || D < ShadowDistance)
						continue;

					Graphics::RenderTargetCube* Target = Renderers.PointLight[Shadows];
					Device->SetTarget(Target, 0, 0, 0);
					Device->ClearDepth(Target);

					Scene->SetView(Compute::Matrix4x4::Identity(), Light->Projection, Light->GetEntity()->Transform->Position, Light->ShadowDistance, true);
					Scene->RenderDepthCubic(Time);
					Light->SetShadowCache(Target->GetTarget()); Shadows++;
				}

				Shadows = 0;
				for (auto It = SpotLights->Begin(); It != SpotLights->End(); It++)
				{
					Engine::Components::SpotLight* Light = (Engine::Components::SpotLight*)*It;
					if (Shadows >= Renderers.SpotLight.size())
						break;

					Light->SetShadowCache(nullptr);
					if (!Light->Shadowed)
						continue;

					if (!System->PassCullable(Light, CullResult_Always, &D) || D < ShadowDistance)
						continue;

					Graphics::RenderTarget2D* Target = Renderers.SpotLight[Shadows];
					Device->SetTarget(Target, 0, 0, 0);
					Device->ClearDepth(Target);

					Scene->SetView(Light->View, Light->Projection, Light->GetEntity()->Transform->Position, Light->ShadowDistance, true);
					Scene->RenderDepthLinear(Time);
					Light->SetShadowCache(Target->GetTarget()); Shadows++;
				}

				Shadows = 0;
				for (auto It = LineLights->Begin(); It != LineLights->End(); It++)
				{
					Engine::Components::LineLight* Light = (Engine::Components::LineLight*)*It;
					if (Shadows >= Renderers.LineLight.size())
						break;

					Light->SetShadowCache(nullptr);
					if (!Light->Shadowed)
						continue;

					Graphics::RenderTarget2D* Target = Renderers.LineLight[Shadows];
					Device->SetTarget(Target, 0, 0, 0);
					Device->ClearDepth(Target);

					Scene->SetView(Light->View, Light->Projection, 0.0f, -1.0f, true);
					Scene->RenderDepthLinear(Time);
					Light->SetShadowCache(Target->GetTarget()); Shadows++;
				}

				Scene->RestoreViewBuffer(nullptr);
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
				if (State != RenderState_GBuffer || Options & RenderOpt_Inner || Options & RenderOpt_Transparent)
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
					else if (!Light->Rebuild.TickEvent(ElapsedTime) || Light->Rebuild.Delay <= 0.0)
						continue;

					Device->CopyBegin(Surface, 0, MipLevels, Size);
					Light->RenderLocked = true;

					Compute::Vector3 Position = Light->GetEntity()->Transform->Position * Light->ViewOffset;
					for (unsigned int j = 0; j < 6; j++)
					{
						Light->View[j] = Compute::Matrix4x4::CreateCubeMapLookAt(j, Position);
						Scene->SetView(Light->View[j], Light->Projection, Position, Light->CaptureRange, true);
						Scene->ClearSurface();
						Scene->RenderGBuffer(Time, Light->StaticMask ? RenderOpt_Inner | RenderOpt_Static : RenderOpt_Inner);
						Device->CopyFace(Surface, 0, j);
					}

					Light->RenderLocked = false;
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
				Sampler = Lab->GetDevice()->GetSamplerState("trilinear-x16");
				Layout = Lab->GetDevice()->GetInputLayout("shape-vertex");

				Graphics::Shader::Desc I = Graphics::Shader::Desc();
				if (System->GetDevice()->GetSection("geometry/light/shade/point", &I.Data))
					Shaders.PointShade = System->CompileShader("lr-point-shade", I, 0);

				if (System->GetDevice()->GetSection("geometry/light/shade/spot", &I.Data))
					Shaders.SpotShade = System->CompileShader("lr-spot-shade", I, 0);

				if (System->GetDevice()->GetSection("geometry/light/shade/line", &I.Data))
					Shaders.LineShade = System->CompileShader("lr-line-shade", I, 0);

				if (System->GetDevice()->GetSection("geometry/light/base/point", &I.Data))
					Shaders.PointBase = System->CompileShader("lr-point-base", I, sizeof(PointLight));

				if (System->GetDevice()->GetSection("geometry/light/base/spot", &I.Data))
					Shaders.SpotBase = System->CompileShader("lr-spot-base", I, sizeof(SpotLight));

				if (System->GetDevice()->GetSection("geometry/light/base/line", &I.Data))
					Shaders.LineBase = System->CompileShader("lr-line-base", I, sizeof(LineLight));

				if (System->GetDevice()->GetSection("geometry/light/base/probe", &I.Data))
					Shaders.Probe = System->CompileShader("lr-probe", I, sizeof(ReflectionProbe));

				if (System->GetDevice()->GetSection("geometry/light/base/ambient", &I.Data))
					Shaders.Ambient = System->CompileShader("lr-ambient", I, sizeof(AmbientLight));
			}
			Lighting::~Lighting()
			{
				System->FreeShader("lr-point-base", Shaders.PointBase);
				System->FreeShader("lr-point-shade", Shaders.PointShade);
				System->FreeShader("lr-spot-base", Shaders.SpotBase);
				System->FreeShader("lr-spot-shade", Shaders.SpotShade);
				System->FreeShader("lr-line-base", Shaders.LineBase);
				System->FreeShader("lr-line-shade", Shaders.LineShade);
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
				if (State != RenderState_GBuffer)
					return;

				Graphics::GraphicsDevice* Device = System->GetDevice();
				Graphics::MultiRenderTarget2D* Surface = System->GetScene()->GetSurface();
				Graphics::Shader* Active = nullptr;
				bool Inner = (Options & RenderOpt_Inner);
				float D = 0.0;

				Device->SetDepthStencilState(DepthStencil);
				Device->SetSamplerState(Sampler);
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
						ReflectionProbe.OwnWorldViewProjection = Compute::Matrix4x4::CreateTranslatedScale(Light->GetEntity()->Transform->Position, ReflectionProbe.Range) * System->GetScene()->View.ViewProjection;
						ReflectionProbe.Position = Light->GetEntity()->Transform->Position.InvertZ();
						ReflectionProbe.Lighting = Light->Diffuse.Mul(Light->Emission * D);
						ReflectionProbe.Scale = Light->GetEntity()->Transform->Scale;
						ReflectionProbe.Parallax = (Light->ParallaxCorrected ? 1.0f : 0.0f);
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
						if (Light->RenderLocked || !Light->GetProbeCache() || !System->PassCullable(Light, CullResult_Always, &D))
							continue;

						ReflectionProbe.Range = Light->GetRange();
						ReflectionProbe.OwnWorldViewProjection = Compute::Matrix4x4::CreateTranslatedScale(Light->GetEntity()->Transform->Position, ReflectionProbe.Range) * System->GetScene()->View.ViewProjection;
						ReflectionProbe.Position = Light->GetEntity()->Transform->Position.InvertZ();
						ReflectionProbe.Lighting = Light->Diffuse.Mul(Light->Emission * D);
						ReflectionProbe.Scale = Light->GetEntity()->Transform->Scale;
						ReflectionProbe.Parallax = (Light->ParallaxCorrected ? 1.0f : 0.0f);
						ReflectionProbe.Infinity = Light->Infinity;

						Device->SetTextureCube(Light->GetProbeCache(), 5);
						Device->UpdateBuffer(Shaders.Probe, &ReflectionProbe);
						Device->DrawIndexed((unsigned int)System->GetCubeIBuffer()->GetElements(), 0, 0);
					}
				}

				Device->SetShader(Shaders.PointBase, Graphics::ShaderType_Vertex);
				Device->SetBuffer(Shaders.PointBase, 3, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);

				for (auto It = PointLights->Begin(); It != PointLights->End(); ++It)
				{
					Engine::Components::PointLight* Light = (Engine::Components::PointLight*)*It;
					if (!System->PassCullable(Light, CullResult_Always, &D))
						continue;

					if (Light->Shadowed && Light->GetShadowCache())
					{
						PointLight.Softness = Light->ShadowSoftness <= 0 ? 0 : Quality.SpotLight / Light->ShadowSoftness;
						PointLight.Recount = 8.0f * Light->ShadowIterations * Light->ShadowIterations * Light->ShadowIterations;
						PointLight.Bias = Light->ShadowBias;
						PointLight.Distance = Light->ShadowDistance;
						PointLight.Iterations = (float)Light->ShadowIterations;

						Active = Shaders.PointShade;
						Device->SetTexture2D(Light->GetShadowCache(), 5);
					}
					else
						Active = Shaders.PointBase;

					PointLight.Range = Light->GetRange();
					PointLight.OwnWorldViewProjection = Compute::Matrix4x4::CreateTranslatedScale(Light->GetEntity()->Transform->Position, PointLight.Range) * System->GetScene()->View.ViewProjection;
					PointLight.Position = Light->GetEntity()->Transform->Position.InvertZ();
					PointLight.Lighting = Light->Diffuse.Mul(Light->Emission * D);
					
					Device->SetShader(Active, Graphics::ShaderType_Pixel);
					Device->UpdateBuffer(Shaders.PointBase, &PointLight);
					Device->DrawIndexed((unsigned int)System->GetCubeIBuffer()->GetElements(), 0, 0);
				}

				Device->SetShader(Shaders.SpotBase, Graphics::ShaderType_Vertex);
				Device->SetBuffer(Shaders.SpotBase, 3, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);

				for (auto It = SpotLights->Begin(); It != SpotLights->End(); ++It)
				{
					Engine::Components::SpotLight* Light = (Engine::Components::SpotLight*)*It;
					if (!System->PassCullable(Light, CullResult_Always, &D))
						continue;

					if (Light->Shadowed && Light->GetShadowCache())
					{
						SpotLight.Softness = Light->ShadowSoftness <= 0 ? 0 : Quality.SpotLight / Light->ShadowSoftness;
						SpotLight.Recount = 4.0f * Light->ShadowIterations * Light->ShadowIterations;
						SpotLight.Bias = Light->ShadowBias;
						SpotLight.Iterations = (float)Light->ShadowIterations;

						Active = Shaders.SpotShade;
						Device->SetTexture2D(Light->GetShadowCache(), 6);
					}
					else
						Active = Shaders.SpotBase;

					SpotLight.Range = Light->GetRange();
					SpotLight.OwnWorldViewProjection = Compute::Matrix4x4::CreateScale(SpotLight.Range) * Compute::Matrix4x4::CreateTranslation(Light->GetEntity()->Transform->Position) * System->GetScene()->View.ViewProjection;
					SpotLight.OwnViewProjection = Light->View * Light->Projection;
					SpotLight.Position = Light->GetEntity()->Transform->Position.InvertZ();
					SpotLight.Lighting = Light->Diffuse.Mul(Light->Emission * D);
					SpotLight.Diffuse = (Light->GetProjectMap() != nullptr);

					Device->SetTexture2D(Light->GetProjectMap(), 5);
					Device->SetShader(Active, Graphics::ShaderType_Pixel);
					Device->UpdateBuffer(Shaders.SpotBase, &SpotLight);
					Device->DrawIndexed((unsigned int)System->GetCubeIBuffer()->GetElements(), 0, 0);
				}

				Device->SetRasterizerState(BackRasterizer);
				Device->SetShader(Shaders.LineBase, Graphics::ShaderType_Vertex);
				Device->SetBuffer(Shaders.LineBase, 3, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);
				Device->SetVertexBuffer(System->GetQuadVBuffer(), 0);

				for (auto It = LineLights->Begin(); It != LineLights->End(); ++It)
				{
					Engine::Components::LineLight* Light = (Engine::Components::LineLight*)*It;
					if (Light->Shadowed && Light->GetShadowCache())
					{
						LineLight.OwnViewProjection = Light->View * Light->Projection;
						LineLight.Softness = Light->ShadowSoftness <= 0 ? 0 : Quality.LineLight / Light->ShadowSoftness;
						LineLight.Recount = 4.0f * Light->ShadowIterations * Light->ShadowIterations;
						LineLight.Bias = Light->ShadowBias;
						LineLight.ShadowDistance = Light->ShadowDistance / 2.0f;
						LineLight.ShadowLength = Light->ShadowLength;
						LineLight.Iterations = (float)Light->ShadowIterations;

						Active = Shaders.LineShade;
						Device->SetTexture2D(Light->GetShadowCache(), 5);
					}
					else
						Active = Shaders.LineBase;

					LineLight.Position = Light->GetEntity()->Transform->Position.InvertZ().NormalizeSafe();
					LineLight.Lighting = Light->Diffuse.Mul(Light->Emission);
					LineLight.RlhEmission = Light->RlhEmission;
					LineLight.RlhHeight = Light->RlhHeight;
					LineLight.MieEmission = Light->MieEmission;
					LineLight.MieHeight = Light->MieHeight;
					LineLight.ScatterIntensity = Light->ScatterIntensity;
					LineLight.PlanetRadius = Light->PlanetRadius;
					LineLight.AtmosphereRadius = Light->AtmosphereRadius;
					LineLight.MieDirection = Light->MieDirection;

					Device->SetShader(Active, Graphics::ShaderType_Pixel);
					Device->UpdateBuffer(Shaders.LineBase, &LineLight);
					Device->Draw(6, 0);
				}

				AmbientLight.SkyOffset = System->GetScene()->View.Projection.Invert() * Compute::Matrix4x4::CreateRotation(System->GetScene()->View.WorldRotation);
				Device->SetTarget(Surface, 0, 0, 0, 0);
				Device->ClearDepth(Surface);
				Device->SetTexture2D(Inner ? Output2->GetTarget() : Output1->GetTarget(), 5);
				Device->SetTextureCube(SkyMap, 6);
				Device->SetShader(Shaders.Ambient, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);
				Device->SetBuffer(Shaders.Ambient, 3, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);
				Device->UpdateBuffer(Shaders.Ambient, &AmbientLight);
				Device->Draw(6, 0);
				Device->FlushTexture2D(1, 6);
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
				if (System->GetDevice()->GetSection("pass/glitch", &Data))
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
				if (System->GetDevice()->GetSection("pass/tone", &Data))
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
				if (System->GetDevice()->GetSection("pass/focus", &Data))
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
				if (System->GetDevice()->GetSection("pass/bloom-x", &Data))
					Pass1 = CompileEffect("br-bloom-x", Data, sizeof(RenderPass));

				if (System->GetDevice()->GetSection("pass/bloom-y", &Data))
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
				if (System->GetDevice()->GetSection("pass/reflection", &Data))
					Pass1 = CompileEffect("rr-reflection", Data, sizeof(RenderPass1));

				if (System->GetDevice()->GetSection("pass/gloss-x", &Data))
					Pass2 = CompileEffect("rr-gloss-x", Data, sizeof(RenderPass2));

				if (System->GetDevice()->GetSection("pass/gloss-y", &Data))
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
				if (System->GetDevice()->GetSection("pass/ambient", &Data))
					Pass1 = CompileEffect("aor-ambient", Data, sizeof(RenderPass1));

				if (System->GetDevice()->GetSection("pass/blur-x", &Data))
					Pass2 = CompileEffect("aor-blur-x", Data, sizeof(RenderPass2));

				if (System->GetDevice()->GetSection("pass/blur-y", &Data))
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
				if (System->GetDevice()->GetSection("pass/indirect", &Data))
					Pass1 = CompileEffect("dor-indirect", Data, sizeof(RenderPass1));

				if (System->GetDevice()->GetSection("pass/blur-x", &Data))
					Pass2 = CompileEffect("dor-blur-x", Data, sizeof(RenderPass2));

				if (System->GetDevice()->GetSection("pass/blur-y", &Data))
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
				if (State != RenderState_GBuffer || Options & RenderOpt_Inner || Options & RenderOpt_Transparent)
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
