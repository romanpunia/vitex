#include "renderers.h"
#include "components.h"

namespace Tomahawk
{
	namespace Engine
	{
		namespace Renderers
		{
			ModelRenderer::ModelRenderer(Engine::RenderSystem* Lab) : GeoRenderer(Lab)
			{
				DepthStencil = Lab->GetDevice()->GetDepthStencilState("DEF_LESS");
				BackRasterizer = Lab->GetDevice()->GetRasterizerState("DEF_CULL_BACK");
				FrontRasterizer = Lab->GetDevice()->GetRasterizerState("DEF_CULL_FRONT");
				Blend = Lab->GetDevice()->GetBlendState("DEF_OVERWRITE");
				Sampler = Lab->GetDevice()->GetSamplerState("DEF_TRILINEAR_X16");

				Graphics::Shader::Desc I = Graphics::Shader::Desc();
				I.Layout = Graphics::Shader::GetVertexLayout();
				I.LayoutSize = 5;

				if (System->GetDevice()->GetSection("geometry/model/main", &I.Data))
					Shaders.Main = System->CompileShader("mr-main", I, 0);

				if (System->GetDevice()->GetSection("geometry/model/depth-linear", &I.Data))
					Shaders.Linear = System->CompileShader("mr-depth-linear", I, 0);

				if (System->GetDevice()->GetSection("geometry/model/depth-cubic", &I.Data))
					Shaders.Cubic = System->CompileShader("mr-depth-cubic", I, sizeof(Depth));
			}
			ModelRenderer::~ModelRenderer()
			{
				System->FreeShader("mr-main", Shaders.Main);
				System->FreeShader("mr-depth-linear", Shaders.Linear);
				System->FreeShader("mr-depth-cubic", Shaders.Cubic);
			}
			void ModelRenderer::Activate()
			{
				System->AddCull<Engine::Components::Model>();
				System->AddCull<Engine::Components::LimpidModel>();
			}
			void ModelRenderer::Deactivate()
			{
				System->RemoveCull<Engine::Components::Model>();
				System->RemoveCull<Engine::Components::LimpidModel>();
			}
			void ModelRenderer::RenderGBuffer(Rest::Timer* Time, Rest::Pool<Component*>* Geometry, RenderOpt Options)
			{
				Graphics::GraphicsDevice* Device = System->GetDevice();
				CullResult Cull = (Options & RenderOpt_Inner ? CullResult_Always : CullResult_Last);
				bool Static = (Options & RenderOpt_Static);

				Device->SetDepthStencilState(DepthStencil);
				Device->SetSamplerState(Sampler);
				Device->SetBlendState(Blend);
				Device->SetRasterizerState(BackRasterizer);
				Device->SetShader(Shaders.Main, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);
				System->GetScene()->SetSurface();

				for (auto It = Geometry->Begin(); It != Geometry->End(); ++It)
				{
					Engine::Components::Model* Base = (Engine::Components::Model*)*It;
					if (Static && !Base->Static)
						continue;

					auto* Drawable = Base->GetDrawable();
					if (!Drawable || !System->Renderable(Base, Cull, nullptr))
						continue;

					for (auto&& Mesh : Drawable->Meshes)
					{
						if (!Appearance::UploadPhase(Device, Base->GetSurface(Mesh)))
							continue;

						Device->Render.World = Mesh->World * Base->GetEntity()->Transform->GetWorld();
						Device->Render.WorldViewProjection = Device->Render.World * System->GetScene()->View.ViewProjection;
						Device->UpdateBuffer(Graphics::RenderBufferType_Render);
						Device->DrawIndexed(Mesh);
					}
				}

				Device->FlushTexture2D(1, 7);
			}
			void ModelRenderer::RenderDepthLinear(Rest::Timer* Time, Rest::Pool<Component*>* Geometry)
			{
				Graphics::GraphicsDevice* Device = System->GetDevice();
				Device->SetDepthStencilState(DepthStencil);
				Device->SetSamplerState(Sampler);
				Device->SetBlendState(Blend);
				Device->SetRasterizerState(FrontRasterizer);
				Device->SetShader(Shaders.Linear, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);

				for (auto It = Geometry->Begin(); It != Geometry->End(); ++It)
				{
					Engine::Components::Model* Base = (Engine::Components::Model*)*It;
					auto* Drawable = Base->GetDrawable();

					if (!Drawable || !System->Renderable(Base, CullResult_Always, nullptr))
						continue;

					for (auto&& Mesh : Drawable->Meshes)
					{
						if (!Appearance::UploadDepth(Device, Base->GetSurface(Mesh)))
							continue;

						Device->Render.World = Mesh->World * Base->GetEntity()->Transform->GetWorld();
						Device->Render.WorldViewProjection = Device->Render.World * System->GetScene()->View.ViewProjection;
						Device->UpdateBuffer(Graphics::RenderBufferType_Render);
						Device->DrawIndexed(Mesh);
					}
				}

				Device->SetTexture2D(nullptr, 1);
			}
			void ModelRenderer::RenderDepthCubic(Rest::Timer* Time, Rest::Pool<Component*>* Geometry, Compute::Matrix4x4* ViewProjection)
			{
				Graphics::GraphicsDevice* Device = System->GetDevice();
				memcpy(Depth.FaceViewProjection, ViewProjection, sizeof(Compute::Matrix4x4) * 6);
				Device->SetDepthStencilState(DepthStencil);
				Device->SetSamplerState(Sampler);
				Device->SetBlendState(Blend);
				Device->SetRasterizerState(FrontRasterizer);
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
						if (!Appearance::UploadCubicDepth(Device, Base->GetSurface(Mesh)))
							continue;

						Device->Render.World = Mesh->World * Base->GetEntity()->Transform->GetWorld();
						Device->UpdateBuffer(Graphics::RenderBufferType_Render);
						Device->DrawIndexed(Mesh);
					}
				}

				Device->SetTexture2D(nullptr, 1);
				Device->SetShader(nullptr, Graphics::ShaderType_Geometry);
			}
			Rest::Pool<Component*>* ModelRenderer::GetOpaque()
			{
				return System->GetScene()->GetComponents<Components::Model>();
			}
			Rest::Pool<Component*>* ModelRenderer::GetLimpid(uint64_t Layer)
			{
				return System->GetScene()->GetComponents<Components::LimpidModel>();
			}

			SkinRenderer::SkinRenderer(Engine::RenderSystem* Lab) : GeoRenderer(Lab)
			{
				DepthStencil = Lab->GetDevice()->GetDepthStencilState("DEF_LESS");
				BackRasterizer = Lab->GetDevice()->GetRasterizerState("DEF_CULL_BACK");
				FrontRasterizer = Lab->GetDevice()->GetRasterizerState("DEF_CULL_FRONT");
				Blend = Lab->GetDevice()->GetBlendState("DEF_OVERWRITE");
				Sampler = Lab->GetDevice()->GetSamplerState("DEF_TRILINEAR_X16");

				Graphics::Shader::Desc I = Graphics::Shader::Desc();
				I.Layout = Graphics::Shader::GetSkinVertexLayout();
				I.LayoutSize = 7;

				if (System->GetDevice()->GetSection("geometry/skin/main", &I.Data))
					Shaders.Main = System->CompileShader("sr-main", I, 0);

				if (System->GetDevice()->GetSection("geometry/skin/depth-linear", &I.Data))
					Shaders.Linear = System->CompileShader("sr-depth-linear", I, 0);

				if (System->GetDevice()->GetSection("geometry/skin/depth-cubic", &I.Data))
					Shaders.Cubic = System->CompileShader("sr-depth-cubic", I, sizeof(Depth));
			}
			SkinRenderer::~SkinRenderer()
			{
				System->FreeShader("sr-main", Shaders.Main);
				System->FreeShader("sr-depth-linear", Shaders.Linear);
				System->FreeShader("sr-depth-cubic", Shaders.Cubic);
			}
			void SkinRenderer::Activate()
			{
				System->AddCull<Engine::Components::Skin>();
				System->AddCull<Engine::Components::LimpidSkin>();
			}
			void SkinRenderer::Deactivate()
			{
				System->RemoveCull<Engine::Components::Skin>();
				System->RemoveCull<Engine::Components::LimpidSkin>();
			}
			void SkinRenderer::RenderGBuffer(Rest::Timer* Time, Rest::Pool<Component*>* Geometry, RenderOpt Options)
			{
				Graphics::GraphicsDevice* Device = System->GetDevice();
				CullResult Cull = (Options & RenderOpt_Inner ? CullResult_Always : CullResult_Last);
				bool Static = (Options & RenderOpt_Static);

				Device->SetDepthStencilState(DepthStencil);
				Device->SetSamplerState(Sampler);
				Device->SetBlendState(Blend);
				Device->SetRasterizerState(BackRasterizer);
				Device->SetShader(Shaders.Main, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);
				System->GetScene()->SetSurface();

				for (auto It = Geometry->Begin(); It != Geometry->End(); ++It)
				{
					Engine::Components::Skin* Base = (Engine::Components::Skin*)*It;
					if (Static && !Base->Static)
						continue;

					auto* Drawable = Base->GetDrawable();
					if (!Drawable || !System->Renderable(Base, Cull, nullptr))
						continue;

					Device->Animation.HasAnimation = !Base->GetDrawable()->Joints.empty();
					if (Device->Animation.HasAnimation > 0)
						memcpy(Device->Animation.Offsets, Base->Skeleton.Transform, 96 * sizeof(Compute::Matrix4x4));

					Device->UpdateBuffer(Graphics::RenderBufferType_Animation);
					for (auto&& Mesh : Drawable->Meshes)
					{
						if (!Appearance::UploadPhase(Device, Base->GetSurface(Mesh)))
							continue;

						Device->Render.World = Mesh->World * Base->GetEntity()->Transform->GetWorld();
						Device->Render.WorldViewProjection = Device->Render.World * System->GetScene()->View.ViewProjection;
						Device->UpdateBuffer(Graphics::RenderBufferType_Render);
						Device->DrawIndexed(Mesh);
					}
				}

				Device->FlushTexture2D(1, 7);
			}
			void SkinRenderer::RenderDepthLinear(Rest::Timer* Time, Rest::Pool<Component*>* Geometry)
			{
				Graphics::GraphicsDevice* Device = System->GetDevice();
				Device->SetDepthStencilState(DepthStencil);
				Device->SetSamplerState(Sampler);
				Device->SetBlendState(Blend);
				Device->SetRasterizerState(FrontRasterizer);
				Device->SetShader(Shaders.Linear, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);

				for (auto It = Geometry->Begin(); It != Geometry->End(); ++It)
				{
					Engine::Components::Skin* Base = (Engine::Components::Skin*)*It;
					auto* Drawable = Base->GetDrawable();

					if (!Drawable || !System->Renderable(Base, CullResult_Always, nullptr))
						continue;

					Device->Animation.HasAnimation = !Base->GetDrawable()->Joints.empty();
					if (Device->Animation.HasAnimation > 0)
						memcpy(Device->Animation.Offsets, Base->Skeleton.Transform, 96 * sizeof(Compute::Matrix4x4));

					Device->UpdateBuffer(Graphics::RenderBufferType_Animation);
					for (auto&& Mesh : Drawable->Meshes)
					{
						if (!Appearance::UploadDepth(Device, Base->GetSurface(Mesh)))
							continue;

						Device->Render.World = Mesh->World * Base->GetEntity()->Transform->GetWorld();
						Device->Render.WorldViewProjection = Device->Render.World * System->GetScene()->View.ViewProjection;
						Device->UpdateBuffer(Graphics::RenderBufferType_Render);
						Device->DrawIndexed(Mesh);
					}
				}

				Device->SetTexture2D(nullptr, 1);
			}
			void SkinRenderer::RenderDepthCubic(Rest::Timer* Time, Rest::Pool<Component*>* Geometry, Compute::Matrix4x4* ViewProjection)
			{
				Graphics::GraphicsDevice* Device = System->GetDevice();
				memcpy(Depth.FaceViewProjection, ViewProjection, sizeof(Compute::Matrix4x4) * 6);
				Device->SetDepthStencilState(DepthStencil);
				Device->SetSamplerState(Sampler);
				Device->SetBlendState(Blend);
				Device->SetRasterizerState(FrontRasterizer);
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
						if (!Appearance::UploadCubicDepth(Device, Base->GetSurface(Mesh)))
							continue;

						Device->Render.World = Mesh->World * Base->GetEntity()->Transform->GetWorld();
						Device->UpdateBuffer(Graphics::RenderBufferType_Render);
						Device->DrawIndexed(Mesh);
					}
				}

				Device->SetTexture2D(nullptr, 1);
				Device->SetShader(nullptr, Graphics::ShaderType_Geometry);
			}
			Rest::Pool<Component*>* SkinRenderer::GetOpaque()
			{
				return System->GetScene()->GetComponents<Components::Skin>();
			}
			Rest::Pool<Component*>* SkinRenderer::GetLimpid(uint64_t Layer)
			{
				return System->GetScene()->GetComponents<Components::LimpidSkin>();
			}

			SoftBodyRenderer::SoftBodyRenderer(Engine::RenderSystem* Lab) : GeoRenderer(Lab)
			{
				DepthStencil = Lab->GetDevice()->GetDepthStencilState("DEF_LESS");
				BackRasterizer = Lab->GetDevice()->GetRasterizerState("DEF_CULL_BACK");
				FrontRasterizer = Lab->GetDevice()->GetRasterizerState("DEF_CULL_FRONT");
				Blend = Lab->GetDevice()->GetBlendState("DEF_OVERWRITE");
				Sampler = Lab->GetDevice()->GetSamplerState("DEF_TRILINEAR_X16");

				Graphics::ElementBuffer::Desc F = Graphics::ElementBuffer::Desc();
				F.AccessFlags = Graphics::CPUAccess_Write;
				F.Usage = Graphics::ResourceUsage_Dynamic;
				F.BindFlags = Graphics::ResourceBind_Vertex_Buffer;
				F.ElementWidth = sizeof(Compute::Vertex);
				F.ElementCount = 16384;
				F.UseSubresource = false;

				VertexBuffer = Lab->GetDevice()->CreateElementBuffer(F);

				F = Graphics::ElementBuffer::Desc();
				F.AccessFlags = Graphics::CPUAccess_Write;
				F.Usage = Graphics::ResourceUsage_Dynamic;
				F.BindFlags = Graphics::ResourceBind_Index_Buffer;
				F.ElementWidth = sizeof(int);
				F.ElementCount = 49152;
				F.UseSubresource = false;

				IndexBuffer = Lab->GetDevice()->CreateElementBuffer(F);

				Graphics::Shader::Desc I = Graphics::Shader::Desc();
				I.Layout = Graphics::Shader::GetVertexLayout();
				I.LayoutSize = 5;

				if (System->GetDevice()->GetSection("geometry/model/main", &I.Data))
					Shaders.Main = System->CompileShader("mr-main", I, 0);

				if (System->GetDevice()->GetSection("geometry/model/depth-linear", &I.Data))
					Shaders.Linear = System->CompileShader("mr-depth-linear", I, 0);

				if (System->GetDevice()->GetSection("geometry/model/depth-cubic", &I.Data))
					Shaders.Cubic = System->CompileShader("mr-depth-cubic", I, sizeof(Depth));
			}
			SoftBodyRenderer::~SoftBodyRenderer()
			{
				System->FreeShader("mr-main", Shaders.Main);
				System->FreeShader("mr-depth-linear", Shaders.Linear);
				System->FreeShader("mr-depth-cubic", Shaders.Cubic);
				delete VertexBuffer;
				delete IndexBuffer;
			}
			void SoftBodyRenderer::Activate()
			{
				System->AddCull<Engine::Components::SoftBody>();
				System->AddCull<Engine::Components::LimpidSoftBody>();
			}
			void SoftBodyRenderer::Deactivate()
			{
				System->RemoveCull<Engine::Components::SoftBody>();
				System->RemoveCull<Engine::Components::LimpidSoftBody>();
			}
			void SoftBodyRenderer::RenderGBuffer(Rest::Timer* Time, Rest::Pool<Component*>* Geometry, RenderOpt Options)
			{
				Graphics::GraphicsDevice* Device = System->GetDevice();
				CullResult Cull = (Options & RenderOpt_Inner ? CullResult_Always : CullResult_Last);
				bool Static = (Options & RenderOpt_Static);

				Device->SetDepthStencilState(DepthStencil);
				Device->SetSamplerState(Sampler);
				Device->SetBlendState(Blend);
				Device->SetRasterizerState(BackRasterizer);
				Device->SetShader(Shaders.Main, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);
				System->GetScene()->SetSurface();

				for (auto It = Geometry->Begin(); It != Geometry->End(); ++It)
				{
					Engine::Components::SoftBody* Base = (Engine::Components::SoftBody*)*It;
					if ((Static && !Base->Static) || Base->GetIndices().empty())
						continue;

					if (!System->Renderable(Base, Cull, nullptr))
						continue;

					if (!Appearance::UploadPhase(Device, Base->GetSurface()))
						continue;

					Base->Fill(Device, IndexBuffer, VertexBuffer);

					Device->Render.World.Identify();
					Device->Render.WorldViewProjection = System->GetScene()->View.ViewProjection;
					Device->UpdateBuffer(Graphics::RenderBufferType_Render);
					Device->SetVertexBuffer(VertexBuffer, 0, sizeof(Compute::Vertex), 0);
					Device->SetIndexBuffer(IndexBuffer, Graphics::Format_R32_Uint, 0);
					Device->DrawIndexed((unsigned int)Base->GetIndices().size(), 0, 0);
				}

				Device->FlushTexture2D(1, 7);
			}
			void SoftBodyRenderer::RenderDepthLinear(Rest::Timer* Time, Rest::Pool<Component*>* Geometry)
			{
				Graphics::GraphicsDevice* Device = System->GetDevice();
				Device->SetDepthStencilState(DepthStencil);
				Device->SetSamplerState(Sampler);
				Device->SetBlendState(Blend);
				Device->SetRasterizerState(FrontRasterizer);
				Device->SetShader(Shaders.Linear, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);

				for (auto It = Geometry->Begin(); It != Geometry->End(); ++It)
				{
					Engine::Components::SoftBody* Base = (Engine::Components::SoftBody*)*It;
					if (Base->GetIndices().empty())
						continue;

					if (!System->Renderable(Base, CullResult_Always, nullptr))
						continue;

					if (!Appearance::UploadDepth(Device, Base->GetSurface()))
						continue;

					Base->Fill(Device, IndexBuffer, VertexBuffer);

					Device->Render.World.Identify();
					Device->Render.WorldViewProjection = System->GetScene()->View.ViewProjection;
					Device->UpdateBuffer(Graphics::RenderBufferType_Render);
					Device->SetVertexBuffer(VertexBuffer, 0, sizeof(Compute::Vertex), 0);
					Device->SetIndexBuffer(IndexBuffer, Graphics::Format_R32_Uint, 0);
					Device->DrawIndexed((unsigned int)Base->GetIndices().size(), 0, 0);
				}

				Device->SetTexture2D(nullptr, 1);
			}
			void SoftBodyRenderer::RenderDepthCubic(Rest::Timer* Time, Rest::Pool<Component*>* Geometry, Compute::Matrix4x4* ViewProjection)
			{
				Graphics::GraphicsDevice* Device = System->GetDevice();
				memcpy(Depth.FaceViewProjection, ViewProjection, sizeof(Compute::Matrix4x4) * 6);
				Device->SetDepthStencilState(DepthStencil);
				Device->SetSamplerState(Sampler);
				Device->SetBlendState(Blend);
				Device->SetRasterizerState(FrontRasterizer);
				Device->SetShader(Shaders.Cubic, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel | Graphics::ShaderType_Geometry);
				Device->SetBuffer(Shaders.Cubic, 3, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel | Graphics::ShaderType_Geometry);
				Device->UpdateBuffer(Shaders.Cubic, &Depth);

				for (auto It = Geometry->Begin(); It != Geometry->End(); ++It)
				{
					Engine::Components::SoftBody* Base = (Engine::Components::SoftBody*)*It;
					if (!Base->GetBody() || Base->GetEntity()->Transform->Position.Distance(System->GetScene()->View.WorldPosition) >= System->GetScene()->View.ViewDistance + Base->GetEntity()->Transform->Scale.Length())
						continue;

					if (!Appearance::UploadCubicDepth(Device, Base->GetSurface()))
						continue;

					Base->Fill(Device, IndexBuffer, VertexBuffer);

					Device->Render.World.Identify();
					Device->UpdateBuffer(Graphics::RenderBufferType_Render);
					Device->SetVertexBuffer(VertexBuffer, 0, sizeof(Compute::Vertex), 0);
					Device->SetIndexBuffer(IndexBuffer, Graphics::Format_R32_Uint, 0);
					Device->DrawIndexed((unsigned int)Base->GetIndices().size(), 0, 0);
				}

				Device->SetTexture2D(nullptr, 1);
				Device->SetShader(nullptr, Graphics::ShaderType_Geometry);
			}
			Rest::Pool<Component*>* SoftBodyRenderer::GetOpaque()
			{
				return System->GetScene()->GetComponents<Components::SoftBody>();
			}
			Rest::Pool<Component*>* SoftBodyRenderer::GetLimpid(uint64_t Layer)
			{
				return System->GetScene()->GetComponents<Components::LimpidSoftBody>();
			}

			EmitterRenderer::EmitterRenderer(RenderSystem* Lab) : GeoRenderer(Lab)
			{
				DepthStencil = Lab->GetDevice()->GetDepthStencilState("DEF_NONE_LESS");
				BackRasterizer = Lab->GetDevice()->GetRasterizerState("DEF_CULL_BACK");
				FrontRasterizer = Lab->GetDevice()->GetRasterizerState("DEF_CULL_FRONT");
				AdditiveBlend = Lab->GetDevice()->GetBlendState("DEF_ADDITIVE_ALPHA");
				OverwriteBlend = Lab->GetDevice()->GetBlendState("DEF_OVERWRITE");
				Sampler = Lab->GetDevice()->GetSamplerState("DEF_TRILINEAR_X16");

				Graphics::Shader::Desc I = Graphics::Shader::Desc();
				I.Layout = nullptr;
				I.LayoutSize = 0;

				if (System->GetDevice()->GetSection("geometry/emitter/main", &I.Data))
					Shaders.Main = System->CompileShader("er-main", I, 0);

				if (System->GetDevice()->GetSection("geometry/emitter/depth-linear", &I.Data))
					Shaders.Linear = System->CompileShader("er-depth-linear", I, 0);

				if (System->GetDevice()->GetSection("geometry/emitter/depth-point", &I.Data))
					Shaders.Point = System->CompileShader("er-depth-point", I, 0);

				if (System->GetDevice()->GetSection("geometry/emitter/depth-quad", &I.Data))
					Shaders.Quad = System->CompileShader("er-depth-quad", I, sizeof(Depth));
			}
			EmitterRenderer::~EmitterRenderer()
			{
				System->FreeShader("er-main", Shaders.Main);
				System->FreeShader("er-depth-linear", Shaders.Linear);
				System->FreeShader("er-depth-point", Shaders.Point);
				System->FreeShader("er-depth-quad", Shaders.Quad);
			}
			void EmitterRenderer::Activate()
			{
				System->AddCull<Engine::Components::Emitter>();
				System->AddCull<Engine::Components::LimpidEmitter>();
			}
			void EmitterRenderer::Deactivate()
			{
				System->RemoveCull<Engine::Components::Emitter>();
				System->RemoveCull<Engine::Components::LimpidEmitter>();
			}
			void EmitterRenderer::RenderGBuffer(Rest::Timer* Time, Rest::Pool<Component*>* Geometry, RenderOpt Options)
			{
				Graphics::GraphicsDevice* Device = System->GetDevice();
				Graphics::PrimitiveTopology T = Device->GetPrimitiveTopology();
				CullResult Cull = (Options & RenderOpt_Inner ? CullResult_Always : CullResult_Last);
				bool Static = (Options & RenderOpt_Static);

				Device->SetDepthStencilState(DepthStencil);
				Device->SetSamplerState(Sampler);
				Device->SetBlendState(AdditiveBlend);
				Device->SetRasterizerState(BackRasterizer);
				Device->SetPrimitiveTopology(Graphics::PrimitiveTopology_Point_List);
				Device->SetShader(Shaders.Main, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);
				Device->SetVertexBuffer(nullptr, 0, 0, 0);
				System->GetScene()->SetSurface();

				Compute::Matrix4x4 View = System->GetScene()->View.ViewProjection * System->GetScene()->View.Projection.Invert();
				for (auto It = Geometry->Begin(); It != Geometry->End(); ++It)
				{
					Engine::Components::Emitter* Base = (Engine::Components::Emitter*)*It;
					if ((Static && !Base->Static) && !Base->GetBuffer())
						continue;

					if (!System->Renderable(Base, Cull, nullptr))
						continue;

					if (!Appearance::UploadPhase(Device, Base->GetSurface()))
						continue;

					Device->Render.World = System->GetScene()->View.Projection;
					Device->Render.WorldViewProjection = (Base->QuadBased ? View : System->GetScene()->View.ViewProjection);
					
					if (Base->Connected)
						Device->Render.WorldViewProjection = Base->GetEntity()->Transform->GetWorld() * Device->Render.WorldViewProjection;

					Device->UpdateBuffer(Base->GetBuffer());
					Device->SetBuffer(Base->GetBuffer(), 8);
					Device->SetShader(Base->QuadBased ? Shaders.Main : nullptr, Graphics::ShaderType_Geometry);
					Device->UpdateBuffer(Graphics::RenderBufferType_Render);
					Device->Draw((unsigned int)Base->GetBuffer()->GetArray()->Size(), 0);
				}

				Device->FlushTexture2D(1, 7);
				Device->SetPrimitiveTopology(T);
				Device->SetShader(nullptr, Graphics::ShaderType_Geometry);
			}
			void EmitterRenderer::RenderDepthLinear(Rest::Timer* Time, Rest::Pool<Component*>* Geometry)
			{
				Graphics::GraphicsDevice* Device = System->GetDevice();
				Graphics::PrimitiveTopology T = Device->GetPrimitiveTopology();

				Device->SetDepthStencilState(DepthStencil);
				Device->SetSamplerState(Sampler);
				Device->SetBlendState(OverwriteBlend);
				Device->SetRasterizerState(FrontRasterizer);
				Device->SetPrimitiveTopology(Graphics::PrimitiveTopology_Point_List);
				Device->SetShader(Shaders.Linear, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);
				Device->SetVertexBuffer(nullptr, 0, 0, 0);
				System->GetScene()->SetSurface();

				Compute::Matrix4x4 View = System->GetScene()->View.ViewProjection * System->GetScene()->View.Projection.Invert();
				for (auto It = Geometry->Begin(); It != Geometry->End(); ++It)
				{
					Engine::Components::Emitter* Base = (Engine::Components::Emitter*)*It;
					if (!Base->GetBuffer())
						continue;

					if (!Appearance::UploadDepth(Device, Base->GetSurface()))
						continue;

					Device->Render.World = Base->QuadBased ? System->GetScene()->View.Projection : Compute::Matrix4x4::Identity();
					Device->Render.WorldViewProjection = (Base->QuadBased ? View : System->GetScene()->View.ViewProjection);
					
					if (Base->Connected)
						Device->Render.WorldViewProjection = Base->GetEntity()->Transform->GetWorld() * Device->Render.WorldViewProjection;

					Device->UpdateBuffer(Base->GetBuffer());
					Device->SetBuffer(Base->GetBuffer(), 8);
					Device->SetShader(Base->QuadBased ? Shaders.Linear : nullptr, Graphics::ShaderType_Geometry);
					Device->UpdateBuffer(Graphics::RenderBufferType_Render);
					Device->Draw((unsigned int)Base->GetBuffer()->GetArray()->Size(), 0);
				}

				Device->SetTexture2D(nullptr, 1);
				Device->SetPrimitiveTopology(T);
				Device->SetShader(nullptr, Graphics::ShaderType_Geometry);
			}
			void EmitterRenderer::RenderDepthCubic(Rest::Timer* Time, Rest::Pool<Component*>* Geometry, Compute::Matrix4x4* ViewProjection)
			{
				Graphics::GraphicsDevice* Device = System->GetDevice();
				Depth.FaceView[0] = Compute::Matrix4x4::CreateCubeMapLookAt(0, System->GetScene()->View.InvViewPosition);
				Depth.FaceView[1] = Compute::Matrix4x4::CreateCubeMapLookAt(1, System->GetScene()->View.InvViewPosition);
				Depth.FaceView[2] = Compute::Matrix4x4::CreateCubeMapLookAt(2, System->GetScene()->View.InvViewPosition);
				Depth.FaceView[3] = Compute::Matrix4x4::CreateCubeMapLookAt(3, System->GetScene()->View.InvViewPosition);
				Depth.FaceView[4] = Compute::Matrix4x4::CreateCubeMapLookAt(4, System->GetScene()->View.InvViewPosition);
				Depth.FaceView[5] = Compute::Matrix4x4::CreateCubeMapLookAt(5, System->GetScene()->View.InvViewPosition);

				Graphics::PrimitiveTopology T = Device->GetPrimitiveTopology();
				Device->SetDepthStencilState(DepthStencil);
				Device->SetSamplerState(Sampler);
				Device->SetBlendState(OverwriteBlend);
				Device->SetRasterizerState(FrontRasterizer);
				Device->SetPrimitiveTopology(Graphics::PrimitiveTopology_Point_List);
				Device->SetVertexBuffer(nullptr, 0, 0, 0);
				Device->SetBuffer(Shaders.Quad, 3, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel | Graphics::ShaderType_Geometry);
				Device->UpdateBuffer(Shaders.Quad, &Depth);
				System->GetScene()->SetSurface();

				for (auto It = Geometry->Begin(); It != Geometry->End(); ++It)
				{
					Engine::Components::Emitter* Base = (Engine::Components::Emitter*)*It;
					if (!Base->GetBuffer())
						continue;

					if (!Appearance::UploadCubicDepth(Device, Base->GetSurface()))
						continue;

					Device->Render.World = (Base->Connected ? Base->GetEntity()->Transform->GetWorld() : Compute::Matrix4x4::Identity());			
					Device->UpdateBuffer(Base->GetBuffer());
					Device->SetBuffer(Base->GetBuffer(), 8);
					Device->SetShader(Base->QuadBased ? Shaders.Quad : Shaders.Point, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel | Graphics::ShaderType_Geometry);
					Device->UpdateBuffer(Graphics::RenderBufferType_Render);
					Device->Draw((unsigned int)Base->GetBuffer()->GetArray()->Size(), 0);
				}

				Device->SetTexture2D(nullptr, 1);
				Device->SetPrimitiveTopology(T);
				Device->SetShader(nullptr, Graphics::ShaderType_Geometry);
			}
			Rest::Pool<Component*>* EmitterRenderer::GetOpaque()
			{
				return System->GetScene()->GetComponents<Components::Emitter>();
			}
			Rest::Pool<Component*>* EmitterRenderer::GetLimpid(uint64_t Layer)
			{
				return System->GetScene()->GetComponents<Components::LimpidEmitter>();
			}

			DecalRenderer::DecalRenderer(RenderSystem* Lab) : GeoRenderer(Lab)
			{
				DepthStencil = Lab->GetDevice()->GetDepthStencilState("DEF_NONE");
				Rasterizer = Lab->GetDevice()->GetRasterizerState("DEF_CULL_BACK");
				Blend = Lab->GetDevice()->GetBlendState("DEF_ADDITIVE");
				Sampler = Lab->GetDevice()->GetSamplerState("DEF_TRILINEAR_X16");

				Graphics::Shader::Desc I = Graphics::Shader::Desc();
				I.Layout = Graphics::Shader::GetShapeVertexLayout();
				I.LayoutSize = 2;

				if (System->GetDevice()->GetSection("geometry/decal/main", &I.Data))
					Shader = System->CompileShader("dr-main", I, sizeof(RenderPass));
			}
			DecalRenderer::~DecalRenderer()
			{
				System->FreeShader("dr-main", Shader);
			}
			void DecalRenderer::Activate()
			{
				System->AddCull<Engine::Components::Decal>();
				System->AddCull<Engine::Components::LimpidDecal>();
			}
			void DecalRenderer::Deactivate()
			{
				System->RemoveCull<Engine::Components::Decal>();
				System->RemoveCull<Engine::Components::LimpidDecal>();
			}
			void DecalRenderer::RenderGBuffer(Rest::Timer* Time, Rest::Pool<Component*>* Geometry, RenderOpt Options)
			{
				Graphics::GraphicsDevice* Device = System->GetDevice();
				Rest::Pool<Component*>* Array = (Options & RenderOpt_Limpid ? Limpid : Opaque);
				CullResult Cull = (Options & RenderOpt_Inner ? CullResult_Always : CullResult_Last);
				bool Map[8] = { true, true, false, true, false, false, false, false };
				bool Static = (Options & RenderOpt_Static);
				bool Inner = (Options & RenderOpt_Inner);

				if (!Array || Geometry->Empty())
					return;

				Device->SetDepthStencilState(DepthStencil);
				Device->SetSamplerState(Sampler);
				Device->SetBlendState(Blend);
				Device->SetRasterizerState(Rasterizer);
				Device->SetShader(Shader, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);
				Device->SetBuffer(Shader, 3, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);
				Device->SetIndexBuffer(System->GetSphereIBuffer(), Graphics::Format_R32_Uint, 0);
				Device->SetVertexBuffer(System->GetSphereVBuffer(), 0, System->GetSphereVSize(), 0);
				Device->SetTargetMap(System->GetScene()->GetSurface(), Map);
				Device->SetTexture2D(System->GetScene()->GetSurface()->GetTarget(2), 8);

				for (auto It = Geometry->Begin(); It != Geometry->End(); ++It)
				{
					Engine::Components::Decal* Base = (Engine::Components::Decal*)*It;
					if (Static && !Base->Static || !System->Renderable(Base, Cull, nullptr))
						continue;

					if (!Appearance::UploadPhase(Device, Base->GetSurface()))
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
			void DecalRenderer::RenderDepthLinear(Rest::Timer* Time, Rest::Pool<Component*>* Geometry)
			{
			}
			void DecalRenderer::RenderDepthCubic(Rest::Timer* Time, Rest::Pool<Component*>* Geometry, Compute::Matrix4x4* ViewProjection)
			{
			}
			Rest::Pool<Component*>* DecalRenderer::GetOpaque()
			{
				return System->GetScene()->GetComponents<Components::Decal>();
			}
			Rest::Pool<Component*>* DecalRenderer::GetLimpid(uint64_t Layer)
			{
				return System->GetScene()->GetComponents<Components::LimpidDecal>();
			}

			LimpidRenderer::LimpidRenderer(RenderSystem* Lab) : Renderer(Lab)
			{
				DepthStencil = Lab->GetDevice()->GetDepthStencilState("DEF_NONE");
				Rasterizer = Lab->GetDevice()->GetRasterizerState("DEF_CULL_BACK");
				Blend = Lab->GetDevice()->GetBlendState("DEF_OVERWRITE");
				Sampler = Lab->GetDevice()->GetSamplerState("DEF_TRILINEAR_X16");

				Graphics::Shader::Desc I = Graphics::Shader::Desc();
				I.Layout = Graphics::Shader::GetShapeVertexLayout();
				I.LayoutSize = 2;

				if (System->GetDevice()->GetSection("pass/limpid", &I.Data))
					Shader = System->CompileShader("lr-limpid", I, sizeof(RenderPass));
			}
			LimpidRenderer::~LimpidRenderer()
			{
				System->FreeShader("lr-limpid", Shader);
				delete Surface1;
				delete Input1;
				delete Surface2;
				delete Input2;
			}
			void LimpidRenderer::Activate()
			{
				ResizeBuffers();
			}
			void LimpidRenderer::Render(Rest::Timer* Time, RenderState State, RenderOpt Options)
			{
				bool Inner = (Options & RenderOpt_Inner);
				if (State != RenderState_GBuffer || Options & RenderOpt_Limpid)
					return;

				SceneGraph* Scene = System->GetScene();
				Graphics::MultiRenderTarget2D* S = Scene->GetSurface();
				Graphics::GraphicsDevice* Device = System->GetDevice();
				RenderPass.MipLevels = (Inner ? MipLevels2 : MipLevels1);

				Scene->SwapSurface(Inner ? Surface2 : Surface1);
				Scene->SetSurfaceCleared();
				Scene->RenderGBuffer(Time, Options | RenderOpt_Limpid);
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
				Device->SetShader(Shader, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);
				Device->SetBuffer(Shader, 3, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);
				Device->SetVertexBuffer(System->GetQuadVBuffer(), 0, System->GetQuadVSize(), 0);
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
			void LimpidRenderer::ResizeBuffers()
			{
				Graphics::MultiRenderTarget2D::Desc F1;
				System->GetScene()->GetTargetDesc(&F1);
				MipLevels1 = (float)F1.MipLevels;

				delete Surface1;
				Surface1 = System->GetDevice()->CreateMultiRenderTarget2D(F1);

				Graphics::RenderTarget2D::Desc F2;
				System->GetScene()->GetTargetDesc(&F2);

				delete Input1;
				Input1 = System->GetDevice()->CreateRenderTarget2D(F2);

				auto* Renderer = System->GetRenderer<Renderers::ProbeRenderer>();
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

				delete Surface2;
				Surface2 = System->GetDevice()->CreateMultiRenderTarget2D(F1);

				delete Input2;
				Input2 = System->GetDevice()->CreateRenderTarget2D(F2);
			}
		
			DepthRenderer::DepthRenderer(RenderSystem* Lab) : TickRenderer(Lab), ShadowDistance(0.5f)
			{
				Tick.Delay = 10;
			}
			DepthRenderer::~DepthRenderer()
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
			void DepthRenderer::Deserialize(ContentManager* Content, Rest::Document* Node)
			{
				NMake::Unpack(Node->Find("point-light-resolution"), &Renderers.PointLightResolution);
				NMake::Unpack(Node->Find("point-light-limits"), &Renderers.PointLightLimits);
				NMake::Unpack(Node->Find("spot-light-resolution"), &Renderers.SpotLightResolution);
				NMake::Unpack(Node->Find("spot-light-limits"), &Renderers.SpotLightLimits);
				NMake::Unpack(Node->Find("line-light-resolution"), &Renderers.LineLightResolution);
				NMake::Unpack(Node->Find("line-light-limits"), &Renderers.LineLightLimits);
				NMake::Unpack(Node->Find("shadow-distance"), &ShadowDistance);
			}
			void DepthRenderer::Serialize(ContentManager* Content, Rest::Document* Node)
			{
				NMake::Pack(Node->SetDocument("point-light-resolution"), Renderers.PointLightResolution);
				NMake::Pack(Node->SetDocument("point-light-limits"), Renderers.PointLightLimits);
				NMake::Pack(Node->SetDocument("spot-light-resolution"), Renderers.SpotLightResolution);
				NMake::Pack(Node->SetDocument("spot-light-limits"), Renderers.SpotLightLimits);
				NMake::Pack(Node->SetDocument("line-light-resolution"), Renderers.LineLightResolution);
				NMake::Pack(Node->SetDocument("line-light-limits"), Renderers.LineLightLimits);
				NMake::Pack(Node->SetDocument("shadow-distance"), ShadowDistance);
			}
			void DepthRenderer::Activate()
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
			void DepthRenderer::Deactivate()
			{
				PointLights = System->RemoveCull<Engine::Components::PointLight>();
				SpotLights = System->RemoveCull<Engine::Components::SpotLight>();
			}
			void DepthRenderer::TickRender(Rest::Timer* Time, RenderState State, RenderOpt Options)
			{
				if (State != RenderState_GBuffer || Options & RenderOpt_Inner)
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
					
					if (!System->Renderable(Light, CullResult_Always, &D) || D < ShadowDistance)
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

					if (!System->Renderable(Light, CullResult_Always, &D) || D < ShadowDistance)
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

			ProbeRenderer::ProbeRenderer(RenderSystem* Lab) : Renderer(Lab), Surface(nullptr), Size(128), MipLevels(7)
			{
			}
			ProbeRenderer::~ProbeRenderer()
			{
				delete Surface;
			}
			void ProbeRenderer::Deserialize(ContentManager* Content, Rest::Document* Node)
			{
				NMake::Unpack(Node->Find("size"), &Size);
				NMake::Unpack(Node->Find("mip-levels"), &MipLevels);
			}
			void ProbeRenderer::Serialize(ContentManager* Content, Rest::Document* Node)
			{
				NMake::Pack(Node->SetDocument("size"), Size);
				NMake::Pack(Node->SetDocument("mip-levels"), MipLevels);
			}
			void ProbeRenderer::Activate()
			{
				ProbeLights = System->AddCull<Engine::Components::ProbeLight>();
				CreateRenderTarget();
			}
			void ProbeRenderer::Deactivate()
			{
				ProbeLights = System->RemoveCull<Engine::Components::ProbeLight>();
			}
			void ProbeRenderer::Render(Rest::Timer* Time, RenderState State, RenderOpt Options)
			{
				if (State != RenderState_GBuffer || Options & RenderOpt_Inner)
					return;

				SceneGraph* Scene = System->GetScene();
				Graphics::GraphicsDevice* Device = System->GetDevice();
				Graphics::MultiRenderTarget2D* S = Scene->GetSurface();
				Scene->SwapSurface(Surface);
				Scene->SetSurface();

				double ElapsedTime = Time->GetElapsedTime();
				for (auto It = ProbeLights->Begin(); It != ProbeLights->End(); It++)
				{
					Engine::Components::ProbeLight* Light = (Engine::Components::ProbeLight*)*It;
					if (Light->IsImageBased() || !System->Renderable(Light, CullResult_Always, nullptr))
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
			void ProbeRenderer::CreateRenderTarget()
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

				delete Surface;
				Surface = System->GetDevice()->CreateMultiRenderTarget2D(F);
			}
			void ProbeRenderer::SetCaptureSize(size_t NewSize)
			{
				Size = NewSize;
				CreateRenderTarget();
			}

			LightRenderer::LightRenderer(RenderSystem* Lab) : Renderer(Lab), RecursiveProbes(true), Input1(nullptr), Input2(nullptr), Output1(nullptr), Output2(nullptr)
			{
				DepthStencil = Lab->GetDevice()->GetDepthStencilState("DEF_NONE_LESS");
				Rasterizer = Lab->GetDevice()->GetRasterizerState("DEF_CULL_BACK");
				Blend = Lab->GetDevice()->GetBlendState("DEF_ADDITIVE");
				Sampler = Lab->GetDevice()->GetSamplerState("DEF_TRILINEAR_X16");

				Graphics::Shader::Desc I = Graphics::Shader::Desc();
				I.Layout = Graphics::Shader::GetShapeVertexLayout();
				I.LayoutSize = 2;

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
					Shaders.Probe = System->CompileShader("lr-probe", I, sizeof(ProbeLight));

				if (System->GetDevice()->GetSection("geometry/light/base/ambient", &I.Data))
					Shaders.Ambient = System->CompileShader("lr-ambient", I, sizeof(AmbientLight));
			}
			LightRenderer::~LightRenderer()
			{
				System->FreeShader("lr-point-base", Shaders.PointBase);
				System->FreeShader("lr-point-shade", Shaders.PointShade);
				System->FreeShader("lr-spot-base", Shaders.SpotBase);
				System->FreeShader("lr-spot-shade", Shaders.SpotShade);
				System->FreeShader("lr-line-base", Shaders.LineBase);
				System->FreeShader("lr-line-shade", Shaders.LineShade);
				System->FreeShader("lr-probe", Shaders.Probe);
				System->FreeShader("lr-ambient", Shaders.Ambient);
				delete Input1;
				delete Output1;
				delete Input2;
				delete Output2;
				delete SkyMap;
			}
			void LightRenderer::Deserialize(ContentManager* Content, Rest::Document* Node)
			{
				std::string Path;
				if (NMake::Unpack(Node->Find("sky-map"), &Path))
					SetSkyMap(Content->Load<Graphics::Texture2D>(Path, nullptr));

				NMake::Unpack(Node->Find("high-emission"), &AmbientLight.HighEmission);
				NMake::Unpack(Node->Find("low-emission"), &AmbientLight.LowEmission);
				NMake::Unpack(Node->Find("sky-emission"), &AmbientLight.SkyEmission);
				NMake::Unpack(Node->Find("light-emission"), &AmbientLight.LightEmission);
				NMake::Unpack(Node->Find("sky-color"), &AmbientLight.SkyColor);
				NMake::Unpack(Node->Find("recursive-probes"), &RecursiveProbes);
			}
			void LightRenderer::Serialize(ContentManager* Content, Rest::Document* Node)
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
			void LightRenderer::Activate()
			{
				ProbeLights = System->AddCull<Engine::Components::ProbeLight>();
				PointLights = System->AddCull<Engine::Components::PointLight>();
				SpotLights = System->AddCull<Engine::Components::SpotLight>();
				LineLights = System->GetScene()->GetComponents<Engine::Components::LineLight>();

				DepthRenderer* Depth = System->GetRenderer<DepthRenderer>();
				if (Depth != nullptr)
				{
					Quality.SpotLight = (float)Depth->Renderers.SpotLightResolution;
					Quality.LineLight = (float)Depth->Renderers.LineLightResolution;
				}

				ProbeRenderer* Probe = System->GetRenderer<ProbeRenderer>();
				if (Probe != nullptr)
					ProbeLight.MipLevels = (float)Probe->MipLevels;

				ResizeBuffers();
			}
			void LightRenderer::Deactivate()
			{
				ProbeLights = System->RemoveCull<Engine::Components::ProbeLight>();
				PointLights = System->RemoveCull<Engine::Components::PointLight>();
				SpotLights = System->RemoveCull<Engine::Components::SpotLight>();
			}
			void LightRenderer::Render(Rest::Timer* Time, RenderState State, RenderOpt Options)
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
				Device->SetRasterizerState(Rasterizer);
				Device->CopyTargetTo(Surface, 0, Inner ? Input2 : Input1);
				Device->SetTarget(Inner ? Output2 : Output1, 0, 0, 0);
				Device->SetTexture2D(Inner ? Input2->GetTarget() : Input1->GetTarget(), 1);
				Device->SetTexture2D(Surface->GetTarget(1), 2);
				Device->SetTexture2D(Surface->GetTarget(2), 3);
				Device->SetTexture2D(Surface->GetTarget(3), 4);
				Device->SetIndexBuffer(System->GetSphereIBuffer(), Graphics::Format_R32_Uint, 0);
				Device->SetVertexBuffer(System->GetSphereVBuffer(), 0, System->GetSphereVSize(), 0);

				if (!Inner)
				{
					Device->SetShader(Shaders.Probe, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);
					Device->SetBuffer(Shaders.Probe, 3, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);

					for (auto It = ProbeLights->Begin(); It != ProbeLights->End(); ++It)
					{
						Engine::Components::ProbeLight* Light = (Engine::Components::ProbeLight*)*It;
						if (!Light->GetProbeCache() || !System->Renderable(Light, CullResult_Always, &D))
							continue;

						ProbeLight.Range = Light->GetRange();
						ProbeLight.OwnWorldViewProjection = Compute::Matrix4x4::CreateTranslatedScale(Light->GetEntity()->Transform->Position, ProbeLight.Range * 1.25f) * System->GetScene()->View.ViewProjection;
						ProbeLight.Position = Light->GetEntity()->Transform->Position.InvertZ();
						ProbeLight.Lighting = Light->Diffuse.Mul(Light->Emission * D);
						ProbeLight.Scale = Light->GetEntity()->Transform->Scale;
						ProbeLight.Parallax = (Light->ParallaxCorrected ? 1.0f : 0.0f);
						ProbeLight.Infinity = Light->Infinity;

						Device->SetTextureCube(Light->GetProbeCache(), 5);
						Device->UpdateBuffer(Shaders.Probe, &ProbeLight);
						Device->DrawIndexed((unsigned int)System->GetSphereIBuffer()->GetElements(), 0, 0);
					}
				}
				else if (RecursiveProbes)
				{
					Device->SetShader(Shaders.Probe, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);
					Device->SetBuffer(Shaders.Probe, 3, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);

					for (auto It = ProbeLights->Begin(); It != ProbeLights->End(); ++It)
					{
						Engine::Components::ProbeLight* Light = (Engine::Components::ProbeLight*)*It;
						if (Light->RenderLocked || !Light->GetProbeCache() || !System->Renderable(Light, CullResult_Always, &D))
							continue;

						ProbeLight.Range = Light->GetRange();
						ProbeLight.OwnWorldViewProjection = Compute::Matrix4x4::CreateTranslatedScale(Light->GetEntity()->Transform->Position, ProbeLight.Range * 1.25f) * System->GetScene()->View.ViewProjection;
						ProbeLight.Position = Light->GetEntity()->Transform->Position.InvertZ();
						ProbeLight.Lighting = Light->Diffuse.Mul(Light->Emission * D);
						ProbeLight.Scale = Light->GetEntity()->Transform->Scale;
						ProbeLight.Parallax = (Light->ParallaxCorrected ? 1.0f : 0.0f);
						ProbeLight.Infinity = Light->Infinity;

						Device->SetTextureCube(Light->GetProbeCache(), 5);
						Device->UpdateBuffer(Shaders.Probe, &ProbeLight);
						Device->DrawIndexed((unsigned int)System->GetSphereIBuffer()->GetElements(), 0, 0);
					}
				}

				Device->SetShader(Shaders.PointBase, Graphics::ShaderType_Vertex);
				Device->SetBuffer(Shaders.PointBase, 3, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);

				for (auto It = PointLights->Begin(); It != PointLights->End(); ++It)
				{
					Engine::Components::PointLight* Light = (Engine::Components::PointLight*)*It;
					if (!System->Renderable(Light, CullResult_Always, &D))
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
					PointLight.OwnWorldViewProjection = Compute::Matrix4x4::CreateTranslatedScale(Light->GetEntity()->Transform->Position, PointLight.Range * 1.25f) * System->GetScene()->View.ViewProjection;
					PointLight.Position = Light->GetEntity()->Transform->Position.InvertZ();
					PointLight.Lighting = Light->Diffuse.Mul(Light->Emission * D);
					
					Device->SetShader(Active, Graphics::ShaderType_Pixel);
					Device->UpdateBuffer(Shaders.PointBase, &PointLight);
					Device->DrawIndexed((unsigned int)System->GetSphereIBuffer()->GetElements(), 0, 0);
				}

				Device->SetShader(Shaders.SpotBase, Graphics::ShaderType_Vertex);
				Device->SetBuffer(Shaders.SpotBase, 3, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);

				for (auto It = SpotLights->Begin(); It != SpotLights->End(); ++It)
				{
					Engine::Components::SpotLight* Light = (Engine::Components::SpotLight*)*It;
					if (!System->Renderable(Light, CullResult_Always, &D))
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
					SpotLight.OwnWorldViewProjection = Compute::Matrix4x4::CreateScale(SpotLight.Range * 1.25f) * Compute::Matrix4x4::CreateTranslation(Light->GetEntity()->Transform->Position) * System->GetScene()->View.ViewProjection;
					SpotLight.OwnViewProjection = Light->View * Light->Projection;
					SpotLight.Position = Light->GetEntity()->Transform->Position.InvertZ();
					SpotLight.Lighting = Light->Diffuse.Mul(Light->Emission * D);
					SpotLight.Diffuse = (Light->ProjectMap != nullptr);

					Device->SetTexture2D(Light->ProjectMap, 5);
					Device->SetShader(Active, Graphics::ShaderType_Pixel);
					Device->UpdateBuffer(Shaders.SpotBase, &SpotLight);
					Device->DrawIndexed((unsigned int)System->GetSphereIBuffer()->GetElements(), 0, 0);
				}

				Device->SetShader(Shaders.LineBase, Graphics::ShaderType_Vertex);
				Device->SetBuffer(Shaders.LineBase, 3, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);
				Device->SetVertexBuffer(System->GetQuadVBuffer(), 0, System->GetQuadVSize(), 0);

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
			void LightRenderer::ResizeBuffers()
			{
				Graphics::RenderTarget2D::Desc F;
				System->GetScene()->GetTargetDesc(&F);

				delete Output1;
				Output1 = System->GetDevice()->CreateRenderTarget2D(F);

				delete Input1;
				Input1 = System->GetDevice()->CreateRenderTarget2D(F);

				auto* Renderer = System->GetRenderer<Renderers::ProbeRenderer>();
				if (Renderer != nullptr)
				{
					F.Width = (unsigned int)Renderer->Size;
					F.Height = (unsigned int)Renderer->Size;
					F.MipLevels = (unsigned int)Renderer->MipLevels;
				}

				delete Output2;
				Output2 = System->GetDevice()->CreateRenderTarget2D(F);

				delete Input2;
				Input2 = System->GetDevice()->CreateRenderTarget2D(F);
			}
			void LightRenderer::SetSkyMap(Graphics::Texture2D* Cubemap)
			{
				SkyBase = Cubemap;
				delete SkyMap;
				SkyMap = nullptr;

				if (SkyBase != nullptr)
					SkyMap = System->GetDevice()->CreateTextureCube(SkyBase);
			}
			Graphics::TextureCube* LightRenderer::GetSkyMap()
			{
				if (!SkyBase)
					return nullptr;

				return SkyMap;
			}
			Graphics::Texture2D* LightRenderer::GetSkyBase()
			{
				if (!SkyMap)
					return nullptr;

				return SkyBase;
			}

			GUIRenderer::GUIRenderer(RenderSystem* Lab) : GUIRenderer(Lab, Application::Get() ? Application::Get()->Activity : nullptr)
			{
			}
			GUIRenderer::GUIRenderer(RenderSystem* Lab, Graphics::Activity* NewActivity) : Renderer(Lab), Context(nullptr), AA(true)
			{
				Context = new GUI::Context(System->GetDevice(), NewActivity);
			}
			GUIRenderer::~GUIRenderer()
			{
				delete Context;
			}
			void GUIRenderer::Render(Rest::Timer* Timer, RenderState State, RenderOpt Options)
			{
				if (State != RenderState_GBuffer || Options & RenderOpt_Inner)
					return;

				Graphics::MultiRenderTarget2D* Surface = System->GetScene()->GetSurface();
				if (!Surface)
					return;

				Context->Prepare(Surface->GetWidth(), Surface->GetHeight());
				System->GetScene()->SetSurface();
				Context->Render(Offset, AA);
			}
			GUI::Context* GUIRenderer::GetContext()
			{
				return Context;
			}

			ReflectionsRenderer::ReflectionsRenderer(RenderSystem* Lab) : EffectRenderer(Lab), Pass1(nullptr), Pass2(nullptr)
			{
				std::string Data;
				if (System->GetDevice()->GetSection("pass/reflection", &Data))
					Pass1 = CompileEffect("rr-reflection", Data, sizeof(RenderPass1));

				if (System->GetDevice()->GetSection("pass/gloss", &Data))
					Pass2 = CompileEffect("rr-gloss", Data, sizeof(RenderPass2));
			}
			void ReflectionsRenderer::Deserialize(ContentManager* Content, Rest::Document* Node)
			{
				NMake::Unpack(Node->Find("iteration-count-1"), &RenderPass1.IterationCount);
				NMake::Unpack(Node->Find("iteration-count-2"), &RenderPass2.IterationCount);
				NMake::Unpack(Node->Find("intensity"), &RenderPass1.Intensity);
				NMake::Unpack(Node->Find("blur"), &RenderPass2.Blur);
			}
			void ReflectionsRenderer::Serialize(ContentManager* Content, Rest::Document* Node)
			{
				NMake::Pack(Node->SetDocument("iteration-count-1"), RenderPass1.IterationCount);
				NMake::Pack(Node->SetDocument("iteration-count-2"), RenderPass2.IterationCount);
				NMake::Pack(Node->SetDocument("intensity"), RenderPass1.Intensity);
				NMake::Pack(Node->SetDocument("blur"), RenderPass2.Blur);
			}
			void ReflectionsRenderer::RenderEffect(Rest::Timer* Time)
			{
				Graphics::MultiRenderTarget2D* Surface = System->GetScene()->GetSurface();
				if (Surface != nullptr)
					System->GetDevice()->GenerateMips(Surface->GetTarget(0));

				RenderPass1.MipLevels = System->GetDevice()->GetMipLevel((unsigned int)Output->GetWidth(), (unsigned int)Output->GetHeight());
				RenderPass2.Texel[0] = 1.0f / Output->GetWidth();
				RenderPass2.Texel[1] = 1.0f / Output->GetHeight();
				
				RenderMerge(Pass1, &RenderPass1);
				RenderResult(Pass2, &RenderPass2);
			}

			DepthOfFieldRenderer::DepthOfFieldRenderer(RenderSystem* Lab) : EffectRenderer(Lab)
			{
				std::string Data;
				if (System->GetDevice()->GetSection("pass/focus", &Data))
					CompileEffect("dfr-focus", Data, sizeof(RenderPass));
			}
			void DepthOfFieldRenderer::Deserialize(ContentManager* Content, Rest::Document* Node)
			{
				NMake::Unpack(Node->Find("threshold"), &RenderPass.Threshold);
				NMake::Unpack(Node->Find("gain"), &RenderPass.Gain);
				NMake::Unpack(Node->Find("fringe"), &RenderPass.Fringe);
				NMake::Unpack(Node->Find("bias"), &RenderPass.Bias);
				NMake::Unpack(Node->Find("dither"), &RenderPass.Dither);
				NMake::Unpack(Node->Find("samples"), &RenderPass.Samples);
				NMake::Unpack(Node->Find("rings"), &RenderPass.Rings);
				NMake::Unpack(Node->Find("far-distance"), &RenderPass.FarDistance);
				NMake::Unpack(Node->Find("far-range"), &RenderPass.FarRange);
				NMake::Unpack(Node->Find("near-distance"), &RenderPass.NearDistance);
				NMake::Unpack(Node->Find("near-range"), &RenderPass.NearRange);
				NMake::Unpack(Node->Find("focal-depth"), &RenderPass.FocalDepth);
				NMake::Unpack(Node->Find("intensity"), &RenderPass.Intensity);
				NMake::Unpack(Node->Find("circular"), &RenderPass.Circular);
			}
			void DepthOfFieldRenderer::Serialize(ContentManager* Content, Rest::Document* Node)
			{
				NMake::Pack(Node->SetDocument("threshold"), RenderPass.Threshold);
				NMake::Pack(Node->SetDocument("gain"), RenderPass.Gain);
				NMake::Pack(Node->SetDocument("fringe"), RenderPass.Fringe);
				NMake::Pack(Node->SetDocument("bias"), RenderPass.Bias);
				NMake::Pack(Node->SetDocument("dither"), RenderPass.Dither);
				NMake::Pack(Node->SetDocument("samples"), RenderPass.Samples);
				NMake::Pack(Node->SetDocument("rings"), RenderPass.Rings);
				NMake::Pack(Node->SetDocument("far-distance"), RenderPass.FarDistance);
				NMake::Pack(Node->SetDocument("far-range"), RenderPass.FarRange);
				NMake::Pack(Node->SetDocument("near-distance"), RenderPass.NearDistance);
				NMake::Pack(Node->SetDocument("near-range"), RenderPass.NearRange);
				NMake::Pack(Node->SetDocument("focal-depth"), RenderPass.FocalDepth);
				NMake::Pack(Node->SetDocument("intensity"), RenderPass.Intensity);
				NMake::Pack(Node->SetDocument("circular"), RenderPass.Circular);
			}
			void DepthOfFieldRenderer::RenderEffect(Rest::Timer* Time)
			{
				RenderPass.Texel[0] = 1.0f / Output->GetWidth();
				RenderPass.Texel[1] = 1.0f / Output->GetHeight();
				RenderResult(nullptr, &RenderPass);
			}
			
			EmissionRenderer::EmissionRenderer(RenderSystem* Lab) : EffectRenderer(Lab)
			{
				std::string Data;
				if (System->GetDevice()->GetSection("pass/bloom", &Data))
					CompileEffect("br-bloom", Data, sizeof(RenderPass));
			}
			void EmissionRenderer::Deserialize(ContentManager* Content, Rest::Document* Node)
			{
				NMake::Unpack(Node->Find("iteration-count"), &RenderPass.IterationCount);
				NMake::Unpack(Node->Find("intensity"), &RenderPass.Intensity);
				NMake::Unpack(Node->Find("threshold"), &RenderPass.Threshold);
				NMake::Unpack(Node->Find("scale"), &RenderPass.Scale);
			}
			void EmissionRenderer::Serialize(ContentManager* Content, Rest::Document* Node)
			{
				NMake::Pack(Node->SetDocument("iteration-count"), RenderPass.IterationCount);
				NMake::Pack(Node->SetDocument("intensity"), RenderPass.Intensity);
				NMake::Pack(Node->SetDocument("threshold"), RenderPass.Threshold);
				NMake::Pack(Node->SetDocument("scale"), RenderPass.Scale);
			}
			void EmissionRenderer::RenderEffect(Rest::Timer* Time)
			{
				RenderPass.Texel[0] = 1.0f / Output->GetWidth();
				RenderPass.Texel[1] = 1.0f / Output->GetHeight();
				RenderResult(nullptr, &RenderPass);
			}
			
			GlitchRenderer::GlitchRenderer(RenderSystem* Lab) : EffectRenderer(Lab), ScanLineJitter(0), VerticalJump(0), HorizontalShake(0), ColorDrift(0)
			{
				std::string Data;
				if (System->GetDevice()->GetSection("pass/glitch", &Data))
					CompileEffect("gr-glitch", Data, sizeof(RenderPass));
			}
			void GlitchRenderer::Deserialize(ContentManager* Content, Rest::Document* Node)
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
			void GlitchRenderer::Serialize(ContentManager* Content, Rest::Document* Node)
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
			void GlitchRenderer::RenderEffect(Rest::Timer* Time)
			{
				if (RenderPass.ElapsedTime >= 32000.0f)
					RenderPass.ElapsedTime = 0.0f;

				RenderPass.ElapsedTime += (float)Time->GetDeltaTime() * 10.0f;
				RenderPass.VerticalJumpAmount = VerticalJump;
				RenderPass.VerticalJumpTime += (float)Time->GetDeltaTime() * VerticalJump * 11.3f;
				RenderPass.ScanLineJitterThreshold = Compute::Math<float>::Saturate(1.0f - ScanLineJitter * 1.2f);
				RenderPass.ScanLineJitterDisplacement = 0.002f + Compute::Math<float>::Pow(ScanLineJitter, 3) * 0.05f;
				RenderPass.HorizontalShake = HorizontalShake * 0.2f;
				RenderPass.ColorDriftAmount = ColorDrift * 0.04f;
				RenderPass.ColorDriftTime = RenderPass.ElapsedTime * 606.11f;
				RenderResult(nullptr, &RenderPass);
			}

			AmbientOcclusionRenderer::AmbientOcclusionRenderer(RenderSystem* Lab) : EffectRenderer(Lab), Pass1(nullptr), Pass2(nullptr)
			{
				std::string Data;
				if (System->GetDevice()->GetSection("pass/ambient", &Data))
					Pass1 = CompileEffect("aor-ambient", Data, sizeof(RenderPass1));

				if (System->GetDevice()->GetSection("pass/blur", &Data))
					Pass2 = CompileEffect("aor-blur", Data, sizeof(RenderPass2));
			}
			void AmbientOcclusionRenderer::Deserialize(ContentManager* Content, Rest::Document* Node)
			{
				NMake::Unpack(Node->Find("scale"), &RenderPass1.Scale);
				NMake::Unpack(Node->Find("intensity"), &RenderPass1.Intensity);
				NMake::Unpack(Node->Find("bias"), &RenderPass1.Bias);
				NMake::Unpack(Node->Find("radius"), &RenderPass1.Radius);
				NMake::Unpack(Node->Find("step"), &RenderPass1.Step);
				NMake::Unpack(Node->Find("offset"), &RenderPass1.Offset);
				NMake::Unpack(Node->Find("distance"), &RenderPass1.Distance);
				NMake::Unpack(Node->Find("fading"), &RenderPass1.Fading);
				NMake::Unpack(Node->Find("power-1"), &RenderPass1.Power);
				NMake::Unpack(Node->Find("power-2"), &RenderPass2.Power);
				NMake::Unpack(Node->Find("threshold-1"), &RenderPass1.Threshold);
				NMake::Unpack(Node->Find("threshold-2"), &RenderPass2.Threshold);
				NMake::Unpack(Node->Find("iteration-count-1"), &RenderPass1.IterationCount);
				NMake::Unpack(Node->Find("iteration-count-2"), &RenderPass2.IterationCount);
				NMake::Unpack(Node->Find("blur"), &RenderPass2.Blur);
				NMake::Unpack(Node->Find("additive"), &RenderPass2.Additive);
				NMake::Unpack(Node->Find("discard"), &RenderPass2.Discard);
			}
			void AmbientOcclusionRenderer::Serialize(ContentManager* Content, Rest::Document* Node)
			{
				NMake::Pack(Node->SetDocument("scale"), RenderPass1.Scale);
				NMake::Pack(Node->SetDocument("intensity"), RenderPass1.Intensity);
				NMake::Pack(Node->SetDocument("bias"), RenderPass1.Bias);
				NMake::Pack(Node->SetDocument("radius"), RenderPass1.Radius);
				NMake::Pack(Node->SetDocument("step"), RenderPass1.Step);
				NMake::Pack(Node->SetDocument("offset"), RenderPass1.Offset);
				NMake::Pack(Node->SetDocument("distance"), RenderPass1.Distance);
				NMake::Pack(Node->SetDocument("fading"), RenderPass1.Fading);
				NMake::Pack(Node->SetDocument("power-1"), RenderPass1.Power);
				NMake::Pack(Node->SetDocument("power-2"), RenderPass2.Power);
				NMake::Pack(Node->SetDocument("threshold-1"), RenderPass1.Threshold);
				NMake::Pack(Node->SetDocument("threshold-2"), RenderPass2.Threshold);
				NMake::Pack(Node->SetDocument("iteration-count-1"), RenderPass1.IterationCount);
				NMake::Pack(Node->SetDocument("iteration-count-2"), RenderPass2.IterationCount);
				NMake::Pack(Node->SetDocument("blur"), RenderPass2.Blur);
				NMake::Pack(Node->SetDocument("additive"), RenderPass2.Additive);
				NMake::Pack(Node->SetDocument("discard"), RenderPass2.Discard);
			}
			void AmbientOcclusionRenderer::RenderEffect(Rest::Timer* Time)
			{
				RenderPass2.Texel[0] = 1.0f / Output->GetWidth();
				RenderPass2.Texel[1] = 1.0f / Output->GetHeight();

				RenderMerge(Pass1, &RenderPass1);
				RenderResult(Pass2, &RenderPass2);
			}

			DirectOcclusionRenderer::DirectOcclusionRenderer(RenderSystem* Lab) : EffectRenderer(Lab), Pass1(nullptr), Pass2(nullptr)
			{
				std::string Data;
				if (System->GetDevice()->GetSection("pass/indirect", &Data))
					Pass1 = CompileEffect("dor-indirect", Data, sizeof(RenderPass1));

				if (System->GetDevice()->GetSection("pass/blur", &Data))
					Pass2 = CompileEffect("dor-blur", Data, sizeof(RenderPass2));
			}
			void DirectOcclusionRenderer::Deserialize(ContentManager* Content, Rest::Document* Node)
			{
				NMake::Unpack(Node->Find("scale"), &RenderPass1.Scale);
				NMake::Unpack(Node->Find("intensity"), &RenderPass1.Intensity);
				NMake::Unpack(Node->Find("bias"), &RenderPass1.Bias);
				NMake::Unpack(Node->Find("radius"), &RenderPass1.Radius);
				NMake::Unpack(Node->Find("step"), &RenderPass1.Step);
				NMake::Unpack(Node->Find("offset"), &RenderPass1.Offset);
				NMake::Unpack(Node->Find("distance"), &RenderPass1.Distance);
				NMake::Unpack(Node->Find("fading"), &RenderPass1.Fading);
				NMake::Unpack(Node->Find("power-1"), &RenderPass1.Power);
				NMake::Unpack(Node->Find("power-2"), &RenderPass2.Power);
				NMake::Unpack(Node->Find("threshold-1"), &RenderPass1.Threshold);
				NMake::Unpack(Node->Find("threshold-2"), &RenderPass2.Threshold);
				NMake::Unpack(Node->Find("iteration-count-1"), &RenderPass1.IterationCount);
				NMake::Unpack(Node->Find("iteration-count-2"), &RenderPass2.IterationCount);
				NMake::Unpack(Node->Find("blur"), &RenderPass2.Blur);
				NMake::Unpack(Node->Find("additive"), &RenderPass2.Additive);
				NMake::Unpack(Node->Find("discard"), &RenderPass2.Discard);
			}
			void DirectOcclusionRenderer::Serialize(ContentManager* Content, Rest::Document* Node)
			{
				NMake::Pack(Node->SetDocument("scale"), RenderPass1.Scale);
				NMake::Pack(Node->SetDocument("intensity"), RenderPass1.Intensity);
				NMake::Pack(Node->SetDocument("bias"), RenderPass1.Bias);
				NMake::Pack(Node->SetDocument("radius"), RenderPass1.Radius);
				NMake::Pack(Node->SetDocument("step"), RenderPass1.Step);
				NMake::Pack(Node->SetDocument("offset"), RenderPass1.Offset);
				NMake::Pack(Node->SetDocument("distance"), RenderPass1.Distance);
				NMake::Pack(Node->SetDocument("fading"), RenderPass1.Fading);
				NMake::Pack(Node->SetDocument("power-1"), RenderPass1.Power);
				NMake::Pack(Node->SetDocument("power-2"), RenderPass2.Power);
				NMake::Pack(Node->SetDocument("threshold-1"), RenderPass1.Threshold);
				NMake::Pack(Node->SetDocument("threshold-2"), RenderPass2.Threshold);
				NMake::Pack(Node->SetDocument("iteration-count-1"), RenderPass1.IterationCount);
				NMake::Pack(Node->SetDocument("iteration-count-2"), RenderPass2.IterationCount);
				NMake::Pack(Node->SetDocument("blur"), RenderPass2.Blur);
				NMake::Pack(Node->SetDocument("additive"), RenderPass2.Additive);
				NMake::Pack(Node->SetDocument("discard"), RenderPass2.Discard);
			}
			void DirectOcclusionRenderer::RenderEffect(Rest::Timer* Time)
			{
				RenderPass2.Texel[0] = 1.0f / Output->GetWidth();
				RenderPass2.Texel[1] = 1.0f / Output->GetHeight();

				RenderMerge(Pass1, &RenderPass1);
				RenderResult(Pass2, &RenderPass2);
			}
			
			ToneRenderer::ToneRenderer(RenderSystem* Lab) : EffectRenderer(Lab)
			{
				std::string Data;
				if (System->GetDevice()->GetSection("pass/tone", &Data))
					CompileEffect("tr-tone", Data, sizeof(RenderPass));
			}
			void ToneRenderer::Deserialize(ContentManager* Content, Rest::Document* Node)
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
			void ToneRenderer::Serialize(ContentManager* Content, Rest::Document* Node)
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
			void ToneRenderer::RenderEffect(Rest::Timer* Time)
			{
				RenderResult(nullptr, &RenderPass);
			}
		}
	}
}