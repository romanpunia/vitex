#include "renderers.h"
#include "components.h"

namespace Tomahawk
{
	namespace Engine
	{
		namespace Renderers
		{
			ModelRenderer::ModelRenderer(Engine::RenderSystem* Lab) : Renderer(Lab)
			{
				Geometric = true;
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
			void ModelRenderer::Initialize()
			{
				Opaque = System->GetScene()->GetComponents<Engine::Components::Model>();
				Limpid = System->GetScene()->GetComponents<Engine::Components::LimpidModel>();
			}
			void ModelRenderer::Cull(const Viewer& View)
			{
				System->GetScene()->SortEntitiesBackToFront(Limpid);
				for (auto It = Opaque->Begin(); It != Opaque->End(); ++It)
				{
					Engine::Components::Model* Model = (Engine::Components::Model*)*It;
					if (Model->GetDrawable() != nullptr)
						Model->Visibility = Model->IsVisibleTo(View, &Model->GetBoundingBox());
					else
						Model->Visibility = false;
				}

				for (auto It = Limpid->Begin(); It != Limpid->End(); ++It)
				{
					Engine::Components::Model* Model = (Engine::Components::Model*)*It;
					if (Model->GetDrawable() != nullptr)
						Model->Visibility = Model->IsVisibleTo(View, &Model->GetBoundingBox());
					else
						Model->Visibility = false;
				}
			}
			void ModelRenderer::RenderStep(Rest::Timer* Time, bool Limpidity)
			{
				Graphics::GraphicsDevice* Device = System->GetDevice();
				Rest::Pool<Component*>* Array = (Limpidity ? Limpid : Opaque);
				if (!Array || Array->Empty())
					return;

				Device->SetDepthStencilState(DepthStencil);
				Device->SetSamplerState(Sampler);
				Device->SetBlendState(Blend);
				Device->SetRasterizerState(BackRasterizer);
				Device->SetShader(Shaders.Main, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);
				System->GetScene()->SetSurface();

				for (auto It = Array->Begin(); It != Array->End(); ++It)
				{
					Engine::Components::Model* Model = (Engine::Components::Model*)*It;
					if (!Model->Visibility)
						continue;

					auto* Drawable = Model->GetDrawable();
					if (!Drawable)
						continue;

					for (auto&& Mesh : Drawable->Meshes)
					{
						if (!Appearance::UploadPhase(Device, Model->GetSurface(Mesh)))
							continue;

						Device->Render.World = Mesh->World * Model->GetEntity()->Transform->GetWorld();
						Device->Render.WorldViewProjection = Device->Render.World * System->GetScene()->View.ViewProjection;
						Device->UpdateBuffer(Graphics::RenderBufferType_Render);
						Device->DrawIndexed(Mesh);
					}
				}

				Device->FlushTexture2D(1, 7);
			}
			void ModelRenderer::RenderSubstep(Rest::Timer* Time, bool Limpidity, bool Static)
			{
				Graphics::GraphicsDevice* Device = System->GetDevice();
				Rest::Pool<Component*>* Array = (Limpidity ? Limpid : Opaque);
				if (!Array || Array->Empty())
					return;

				Device->SetDepthStencilState(DepthStencil);
				Device->SetSamplerState(Sampler);
				Device->SetBlendState(Blend);
				Device->SetRasterizerState(BackRasterizer);
				Device->SetShader(Shaders.Main, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);
				System->GetScene()->SetSurface();

				for (auto It = Array->Begin(); It != Array->End(); ++It)
				{
					Engine::Components::Model* Model = (Engine::Components::Model*)*It;
					auto* Drawable = Model->GetDrawable();

					if (!Drawable || (Static && !Model->Static) || !Model->IsVisibleTo(System->GetScene()->View, nullptr))
						continue;

					for (auto&& Mesh : Drawable->Meshes)
					{
						if (!Appearance::UploadPhase(Device, Model->GetSurface(Mesh)))
							continue;

						Device->Render.World = Mesh->World * Model->GetEntity()->Transform->GetWorld();
						Device->Render.WorldViewProjection = Device->Render.World * System->GetScene()->View.ViewProjection;
						Device->UpdateBuffer(Graphics::RenderBufferType_Render);
						Device->DrawIndexed(Mesh);
					}
				}

				Device->FlushTexture2D(1, 7);
			}
			void ModelRenderer::RenderDepthLinear(Rest::Timer* Time)
			{
				Graphics::GraphicsDevice* Device = System->GetDevice();
				if ((!Opaque || Opaque->Empty()) && (!Limpid || Limpid->Empty()))
					return;

				Device->SetDepthStencilState(DepthStencil);
				Device->SetSamplerState(Sampler);
				Device->SetBlendState(Blend);
				Device->SetRasterizerState(FrontRasterizer);
				Device->SetShader(Shaders.Linear, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);

				for (auto It = Opaque->Begin(); It != Opaque->End(); ++It)
				{
					Engine::Components::Model* Model = (Engine::Components::Model*)*It;
					auto* Drawable = Model->GetDrawable();

					if (!Drawable || !Model->IsVisibleTo(System->GetScene()->View, nullptr))
						continue;

					for (auto&& Mesh : Drawable->Meshes)
					{
						if (!Appearance::UploadDepth(Device, Model->GetSurface(Mesh)))
							continue;

						Device->Render.World = Mesh->World * Model->GetEntity()->Transform->GetWorld();
						Device->Render.WorldViewProjection = Device->Render.World * System->GetScene()->View.ViewProjection;
						Device->UpdateBuffer(Graphics::RenderBufferType_Render);
						Device->DrawIndexed(Mesh);
					}
				}

				for (auto It = Limpid->Begin(); It != Limpid->End(); ++It)
				{
					Engine::Components::Model* Model = (Engine::Components::Model*)*It;
					auto* Drawable = Model->GetDrawable();

					if (!Drawable || !Model->IsVisibleTo(System->GetScene()->View, nullptr))
						continue;

					for (auto&& Mesh : Drawable->Meshes)
					{
						if (!Appearance::UploadDepth(Device, Model->GetSurface(Mesh)))
							continue;

						Device->Render.World = Mesh->World * Model->GetEntity()->Transform->GetWorld();
						Device->Render.WorldViewProjection = Device->Render.World * System->GetScene()->View.ViewProjection;
						Device->UpdateBuffer(Graphics::RenderBufferType_Render);
						Device->DrawIndexed(Mesh);
					}
				}

				Device->SetTexture2D(nullptr, 1);
			}
			void ModelRenderer::RenderDepthCubic(Rest::Timer* Time, Compute::Matrix4x4* ViewProjection)
			{
				Graphics::GraphicsDevice* Device = System->GetDevice();
				if ((!Opaque || Opaque->Empty()) && (!Limpid || Limpid->Empty()))
					return;

				memcpy(Depth.FaceViewProjection, ViewProjection, sizeof(Compute::Matrix4x4) * 6);
				Device->SetDepthStencilState(DepthStencil);
				Device->SetSamplerState(Sampler);
				Device->SetBlendState(Blend);
				Device->SetRasterizerState(FrontRasterizer);
				Device->SetShader(Shaders.Cubic, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel | Graphics::ShaderType_Geometry);
				Device->SetBuffer(Shaders.Cubic, 3, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel | Graphics::ShaderType_Geometry);
				Device->UpdateBuffer(Shaders.Cubic, &Depth);

				for (auto It = Opaque->Begin(); It != Opaque->End(); ++It)
				{
					Engine::Components::Model* Model = (Engine::Components::Model*)*It;
					auto* Drawable = Model->GetDrawable();

					if (!Drawable || !Model->IsNearTo(System->GetScene()->View))
						continue;

					for (auto&& Mesh : Drawable->Meshes)
					{
						if (!Appearance::UploadCubicDepth(Device, Model->GetSurface(Mesh)))
							continue;

						Device->Render.World = Mesh->World * Model->GetEntity()->Transform->GetWorld();
						Device->UpdateBuffer(Graphics::RenderBufferType_Render);
						Device->DrawIndexed(Mesh);
					}
				}

				for (auto It = Limpid->Begin(); It != Limpid->End(); ++It)
				{
					Engine::Components::Model* Model = (Engine::Components::Model*)*It;
					auto* Drawable = Model->GetDrawable();

					if (!Drawable || !Model->IsNearTo(System->GetScene()->View))
						continue;

					for (auto&& Mesh : Drawable->Meshes)
					{
						if (!Appearance::UploadCubicDepth(Device, Model->GetSurface(Mesh)))
							continue;

						Device->Render.World = Mesh->World * Model->GetEntity()->Transform->GetWorld();
						Device->UpdateBuffer(Graphics::RenderBufferType_Render);
						Device->DrawIndexed(Mesh);
					}
				}

				Device->SetTexture2D(nullptr, 1);
				Device->SetShader(nullptr, Graphics::ShaderType_Geometry);
			}
			Rest::Pool<Component*>* ModelRenderer::GetGeometry(uint64_t Index)
			{
				return Opaque;
			}
			uint64_t ModelRenderer::GetGeometryCount()
			{
				return 1;
			}

			SkinRenderer::SkinRenderer(Engine::RenderSystem* Lab) : Renderer(Lab)
			{
				Geometric = true;
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
			void SkinRenderer::Initialize()
			{
				Opaque = System->GetScene()->GetComponents<Engine::Components::Skin>();
				Limpid = System->GetScene()->GetComponents<Engine::Components::LimpidSkin>();
			}
			void SkinRenderer::Cull(const Viewer& View)
			{
				System->GetScene()->SortEntitiesBackToFront(Limpid);
				for (auto It = Opaque->Begin(); It != Opaque->End(); ++It)
				{
					Engine::Components::Skin* Model = (Engine::Components::Skin*)*It;
					if (Model->GetDrawable() != nullptr)
						Model->Visibility = Model->IsVisibleTo(View, &Model->GetBoundingBox());
					else
						Model->Visibility = false;
				}

				for (auto It = Limpid->Begin(); It != Limpid->End(); ++It)
				{
					Engine::Components::Skin* Model = (Engine::Components::Skin*)*It;
					if (Model->GetDrawable() != nullptr)
						Model->Visibility = Model->IsVisibleTo(View, &Model->GetBoundingBox());
					else
						Model->Visibility = false;
				}
			}
			void SkinRenderer::RenderStep(Rest::Timer* Time, bool Limpidity)
			{
				Graphics::GraphicsDevice* Device = System->GetDevice();
				Rest::Pool<Component*>* Array = (Limpidity ? Limpid : Opaque);
				if (!Array || Array->Empty())
					return;

				Device->SetDepthStencilState(DepthStencil);
				Device->SetSamplerState(Sampler);
				Device->SetBlendState(Blend);
				Device->SetRasterizerState(BackRasterizer);
				Device->SetShader(Shaders.Main, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);
				System->GetScene()->SetSurface();

				for (auto It = Array->Begin(); It != Array->End(); ++It)
				{
					Engine::Components::Skin* Skin = (Engine::Components::Skin*)*It;
					if (!Skin->Visibility)
						continue;

					auto* Drawable = Skin->GetDrawable();
					if (!Drawable)
						continue;

					Device->Animation.HasAnimation = !Skin->GetDrawable()->Joints.empty();
					if (Device->Animation.HasAnimation > 0)
						memcpy(Device->Animation.Offsets, Skin->Skeleton.Transform, 96 * sizeof(Compute::Matrix4x4));

					Device->UpdateBuffer(Graphics::RenderBufferType_Animation);
					for (auto&& Mesh : Drawable->Meshes)
					{
						if (!Appearance::UploadPhase(Device, Skin->GetSurface(Mesh)))
							continue;

						Device->Render.World = Mesh->World * Skin->GetEntity()->Transform->GetWorld();
						Device->Render.WorldViewProjection = Device->Render.World * System->GetScene()->View.ViewProjection;
						Device->UpdateBuffer(Graphics::RenderBufferType_Render);
						Device->DrawIndexed(Mesh);
					}
				}

				Device->FlushTexture2D(1, 7);
			}
			void SkinRenderer::RenderSubstep(Rest::Timer* Time, bool Limpidity, bool Static)
			{
				Graphics::GraphicsDevice* Device = System->GetDevice();
				Rest::Pool<Component*>* Array = (Limpidity ? Limpid : Opaque);
				if (!Array || Array->Empty())
					return;

				Device->SetDepthStencilState(DepthStencil);
				Device->SetSamplerState(Sampler);
				Device->SetBlendState(Blend);
				Device->SetRasterizerState(BackRasterizer);
				Device->SetShader(Shaders.Main, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);
				System->GetScene()->SetSurface();

				for (auto It = Array->Begin(); It != Array->End(); ++It)
				{
					Engine::Components::Skin* Skin = (Engine::Components::Skin*)*It;
					auto* Drawable = Skin->GetDrawable();

					if (!Drawable || (Static && !Skin->Static) || !Skin->IsVisibleTo(System->GetScene()->View, nullptr))
						continue;

					Device->Animation.HasAnimation = !Skin->GetDrawable()->Joints.empty();
					if (Device->Animation.HasAnimation > 0)
						memcpy(Device->Animation.Offsets, Skin->Skeleton.Transform, 96 * sizeof(Compute::Matrix4x4));

					Device->UpdateBuffer(Graphics::RenderBufferType_Animation);
					for (auto&& Mesh : Drawable->Meshes)
					{
						if (!Appearance::UploadPhase(Device, Skin->GetSurface(Mesh)))
							continue;

						Device->Render.World = Mesh->World * Skin->GetEntity()->Transform->GetWorld();
						Device->Render.WorldViewProjection = Device->Render.World * System->GetScene()->View.ViewProjection;
						Device->UpdateBuffer(Graphics::RenderBufferType_Render);
						Device->DrawIndexed(Mesh);
					}
				}

				Device->FlushTexture2D(1, 7);
			}
			void SkinRenderer::RenderDepthLinear(Rest::Timer* Time)
			{
				Graphics::GraphicsDevice* Device = System->GetDevice();
				if ((!Opaque || Opaque->Empty()) && (!Limpid || Limpid->Empty()))
					return;

				Device->SetDepthStencilState(DepthStencil);
				Device->SetSamplerState(Sampler);
				Device->SetBlendState(Blend);
				Device->SetRasterizerState(FrontRasterizer);
				Device->SetShader(Shaders.Linear, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);

				for (auto It = Opaque->Begin(); It != Opaque->End(); ++It)
				{
					Engine::Components::Skin* Skin = (Engine::Components::Skin*)*It;
					auto* Drawable = Skin->GetDrawable();

					if (!Drawable || !Skin->IsVisibleTo(System->GetScene()->View, nullptr))
						continue;

					Device->Animation.HasAnimation = !Skin->GetDrawable()->Joints.empty();
					if (Device->Animation.HasAnimation > 0)
						memcpy(Device->Animation.Offsets, Skin->Skeleton.Transform, 96 * sizeof(Compute::Matrix4x4));

					Device->UpdateBuffer(Graphics::RenderBufferType_Animation);
					for (auto&& Mesh : Drawable->Meshes)
					{
						if (!Appearance::UploadDepth(Device, Skin->GetSurface(Mesh)))
							continue;

						Device->Render.World = Mesh->World * Skin->GetEntity()->Transform->GetWorld();
						Device->Render.WorldViewProjection = Device->Render.World * System->GetScene()->View.ViewProjection;
						Device->UpdateBuffer(Graphics::RenderBufferType_Render);
						Device->DrawIndexed(Mesh);
					}
				}

				for (auto It = Limpid->Begin(); It != Limpid->End(); ++It)
				{
					Engine::Components::Skin* Skin = (Engine::Components::Skin*)*It;
					auto* Drawable = Skin->GetDrawable();

					if (!Drawable || !Skin->IsVisibleTo(System->GetScene()->View, nullptr))
						continue;

					Device->Animation.HasAnimation = !Skin->GetDrawable()->Joints.empty();
					if (Device->Animation.HasAnimation > 0)
						memcpy(Device->Animation.Offsets, Skin->Skeleton.Transform, 96 * sizeof(Compute::Matrix4x4));

					Device->UpdateBuffer(Graphics::RenderBufferType_Animation);
					for (auto&& Mesh : Drawable->Meshes)
					{
						if (!Appearance::UploadDepth(Device, Skin->GetSurface(Mesh)))
							continue;

						Device->Render.World = Mesh->World * Skin->GetEntity()->Transform->GetWorld();
						Device->Render.WorldViewProjection = Device->Render.World * System->GetScene()->View.ViewProjection;
						Device->UpdateBuffer(Graphics::RenderBufferType_Render);
						Device->DrawIndexed(Mesh);
					}
				}

				Device->SetTexture2D(nullptr, 1);
			}
			void SkinRenderer::RenderDepthCubic(Rest::Timer* Time, Compute::Matrix4x4* ViewProjection)
			{
				Graphics::GraphicsDevice* Device = System->GetDevice();
				if ((!Opaque || Opaque->Empty()) && (!Limpid || Limpid->Empty()))
					return;

				memcpy(Depth.FaceViewProjection, ViewProjection, sizeof(Compute::Matrix4x4) * 6);
				Device->SetDepthStencilState(DepthStencil);
				Device->SetSamplerState(Sampler);
				Device->SetBlendState(Blend);
				Device->SetRasterizerState(FrontRasterizer);
				Device->SetShader(Shaders.Cubic, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel | Graphics::ShaderType_Geometry);
				Device->SetBuffer(Shaders.Cubic, 3, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel | Graphics::ShaderType_Geometry);
				Device->UpdateBuffer(Shaders.Cubic, &Depth);

				for (auto It = Opaque->Begin(); It != Opaque->End(); ++It)
				{
					Engine::Components::Skin* Skin = (Engine::Components::Skin*)*It;
					auto* Drawable = Skin->GetDrawable();

					if (!Drawable || !Skin->IsNearTo(System->GetScene()->View))
						continue;

					Device->Animation.HasAnimation = !Skin->GetDrawable()->Joints.empty();
					if (Device->Animation.HasAnimation > 0)
						memcpy(Device->Animation.Offsets, Skin->Skeleton.Transform, 96 * sizeof(Compute::Matrix4x4));

					Device->UpdateBuffer(Graphics::RenderBufferType_Animation);
					for (auto&& Mesh : Drawable->Meshes)
					{
						if (!Appearance::UploadCubicDepth(Device, Skin->GetSurface(Mesh)))
							continue;

						Device->Render.World = Mesh->World * Skin->GetEntity()->Transform->GetWorld();
						Device->UpdateBuffer(Graphics::RenderBufferType_Render);
						Device->DrawIndexed(Mesh);
					}
				}

				for (auto It = Limpid->Begin(); It != Limpid->End(); ++It)
				{
					Engine::Components::Skin* Skin = (Engine::Components::Skin*)*It;
					auto* Drawable = Skin->GetDrawable();

					if (!Drawable || !Skin->IsNearTo(System->GetScene()->View))
						continue;

					Device->Animation.HasAnimation = !Skin->GetDrawable()->Joints.empty();
					if (Device->Animation.HasAnimation > 0)
						memcpy(Device->Animation.Offsets, Skin->Skeleton.Transform, 96 * sizeof(Compute::Matrix4x4));

					Device->UpdateBuffer(Graphics::RenderBufferType_Animation);
					for (auto&& Mesh : Drawable->Meshes)
					{
						if (!Appearance::UploadCubicDepth(Device, Skin->GetSurface(Mesh)))
							continue;

						Device->Render.World = Mesh->World * Skin->GetEntity()->Transform->GetWorld();
						Device->UpdateBuffer(Graphics::RenderBufferType_Render);
						Device->DrawIndexed(Mesh);
					}
				}

				Device->SetTexture2D(nullptr, 1);
				Device->SetShader(nullptr, Graphics::ShaderType_Geometry);
			}
			Rest::Pool<Component*>* SkinRenderer::GetGeometry(uint64_t Index)
			{
				return Opaque;
			}
			uint64_t SkinRenderer::GetGeometryCount()
			{
				return 1;
			}

			SoftBodyRenderer::SoftBodyRenderer(Engine::RenderSystem* Lab) : Renderer(Lab)
			{
				Geometric = true;
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
			void SoftBodyRenderer::Initialize()
			{
				Opaque = System->GetScene()->GetComponents<Engine::Components::SoftBody>();
				Limpid = System->GetScene()->GetComponents<Engine::Components::LimpidSoftBody>();
			}
			void SoftBodyRenderer::Cull(const Viewer& View)
			{
				System->GetScene()->SortEntitiesBackToFront(Limpid);
				for (auto It = Opaque->Begin(); It != Opaque->End(); ++It)
				{
					Engine::Components::SoftBody* SoftBody = (Engine::Components::SoftBody*)*It;
					if (SoftBody->GetBody() != nullptr)
					{
						Compute::Matrix4x4 World = SoftBody->GetEntity()->Transform->GetWorldUnscaled();
						SoftBody->Visibility = SoftBody->IsVisibleTo(View, &World);
					}
					else
						SoftBody->Visibility = false;
				}

				for (auto It = Limpid->Begin(); It != Limpid->End(); ++It)
				{
					Engine::Components::SoftBody* SoftBody = (Engine::Components::SoftBody*)*It;
					if (SoftBody->GetBody() != nullptr)
					{
						Compute::Matrix4x4 World = SoftBody->GetEntity()->Transform->GetWorldUnscaled();
						SoftBody->Visibility = SoftBody->IsVisibleTo(View, &World);
					}
					else
						SoftBody->Visibility = false;
				}
			}
			void SoftBodyRenderer::RenderStep(Rest::Timer* Time, bool Limpidity)
			{
				Graphics::GraphicsDevice* Device = System->GetDevice();
				Rest::Pool<Component*>* Array = (Limpidity ? Limpid : Opaque);
				if (!Array || Array->Empty())
					return;

				Device->SetDepthStencilState(DepthStencil);
				Device->SetSamplerState(Sampler);
				Device->SetBlendState(Blend);
				Device->SetRasterizerState(BackRasterizer);
				Device->SetShader(Shaders.Main, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);
				System->GetScene()->SetSurface();

				for (auto It = Array->Begin(); It != Array->End(); ++It)
				{
					Engine::Components::SoftBody* SoftBody = (Engine::Components::SoftBody*)*It;
					if (!SoftBody->Visibility || SoftBody->GetIndices().empty())
						continue;

					if (!Appearance::UploadPhase(Device, SoftBody->GetSurface()))
						continue;

					SoftBody->Fill(Device, IndexBuffer, VertexBuffer);

					Device->Render.World.Identify();
					Device->Render.WorldViewProjection = System->GetScene()->View.ViewProjection;
					Device->UpdateBuffer(Graphics::RenderBufferType_Render);
					Device->SetVertexBuffer(VertexBuffer, 0, sizeof(Compute::Vertex), 0);
					Device->SetIndexBuffer(IndexBuffer, Graphics::Format_R32_Uint, 0);
					Device->DrawIndexed((unsigned int)SoftBody->GetIndices().size(), 0, 0);
				}

				Device->FlushTexture2D(1, 7);
			}
			void SoftBodyRenderer::RenderSubstep(Rest::Timer* Time, bool Limpidity, bool Static)
			{
				Graphics::GraphicsDevice* Device = System->GetDevice();
				Rest::Pool<Component*>* Array = (Limpidity ? Limpid : Opaque);
				if (!Array || Array->Empty())
					return;

				Device->SetDepthStencilState(DepthStencil);
				Device->SetSamplerState(Sampler);
				Device->SetBlendState(Blend);
				Device->SetRasterizerState(BackRasterizer);
				Device->SetShader(Shaders.Main, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);
				System->GetScene()->SetSurface();

				for (auto It = Array->Begin(); It != Array->End(); ++It)
				{
					Engine::Components::SoftBody* SoftBody = (Engine::Components::SoftBody*)*It;
					if ((Static && !SoftBody->Static) || !SoftBody->IsVisibleTo(System->GetScene()->View, nullptr) || SoftBody->GetIndices().empty())
						continue;

					if (!Appearance::UploadPhase(Device, SoftBody->GetSurface()))
						continue;

					SoftBody->Fill(Device, IndexBuffer, VertexBuffer);

					Device->Render.World.Identify();
					Device->Render.WorldViewProjection = System->GetScene()->View.ViewProjection;
					Device->UpdateBuffer(Graphics::RenderBufferType_Render);
					Device->SetVertexBuffer(VertexBuffer, 0, sizeof(Compute::Vertex), 0);
					Device->SetIndexBuffer(IndexBuffer, Graphics::Format_R32_Uint, 0);
					Device->DrawIndexed((unsigned int)SoftBody->GetIndices().size(), 0, 0);
				}

				Device->FlushTexture2D(1, 7);
			}
			void SoftBodyRenderer::RenderDepthLinear(Rest::Timer* Time)
			{
				Graphics::GraphicsDevice* Device = System->GetDevice();
				if ((!Opaque || Opaque->Empty()) && (!Limpid || Limpid->Empty()))
					return;

				Device->SetDepthStencilState(DepthStencil);
				Device->SetSamplerState(Sampler);
				Device->SetBlendState(Blend);
				Device->SetRasterizerState(FrontRasterizer);
				Device->SetShader(Shaders.Linear, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);

				for (auto It = Opaque->Begin(); It != Opaque->End(); ++It)
				{
					Engine::Components::SoftBody* SoftBody = (Engine::Components::SoftBody*)*It;
					if (!SoftBody->IsVisibleTo(System->GetScene()->View, nullptr) || SoftBody->GetIndices().empty())
						continue;

					if (!Appearance::UploadDepth(Device, SoftBody->GetSurface()))
						continue;

					SoftBody->Fill(Device, IndexBuffer, VertexBuffer);

					Device->Render.World.Identify();
					Device->Render.WorldViewProjection = System->GetScene()->View.ViewProjection;
					Device->UpdateBuffer(Graphics::RenderBufferType_Render);
					Device->SetVertexBuffer(VertexBuffer, 0, sizeof(Compute::Vertex), 0);
					Device->SetIndexBuffer(IndexBuffer, Graphics::Format_R32_Uint, 0);
					Device->DrawIndexed((unsigned int)SoftBody->GetIndices().size(), 0, 0);
				}

				for (auto It = Limpid->Begin(); It != Limpid->End(); ++It)
				{
					Engine::Components::SoftBody* SoftBody = (Engine::Components::SoftBody*)*It;
					if (!SoftBody->IsVisibleTo(System->GetScene()->View, nullptr) || SoftBody->GetIndices().empty())
						continue;

					if (!Appearance::UploadDepth(Device, SoftBody->GetSurface()))
						continue;

					SoftBody->Fill(Device, IndexBuffer, VertexBuffer);

					Device->Render.World.Identify();
					Device->Render.WorldViewProjection = System->GetScene()->View.ViewProjection;
					Device->UpdateBuffer(Graphics::RenderBufferType_Render);
					Device->SetVertexBuffer(VertexBuffer, 0, sizeof(Compute::Vertex), 0);
					Device->SetIndexBuffer(IndexBuffer, Graphics::Format_R32_Uint, 0);
					Device->DrawIndexed((unsigned int)SoftBody->GetIndices().size(), 0, 0);
				}

				Device->SetTexture2D(nullptr, 1);
			}
			void SoftBodyRenderer::RenderDepthCubic(Rest::Timer* Time, Compute::Matrix4x4* ViewProjection)
			{
				Graphics::GraphicsDevice* Device = System->GetDevice();
				if ((!Opaque || Opaque->Empty()) && (!Limpid || Limpid->Empty()))
					return;

				memcpy(Depth.FaceViewProjection, ViewProjection, sizeof(Compute::Matrix4x4) * 6);
				Device->SetDepthStencilState(DepthStencil);
				Device->SetSamplerState(Sampler);
				Device->SetBlendState(Blend);
				Device->SetRasterizerState(FrontRasterizer);
				Device->SetShader(Shaders.Cubic, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel | Graphics::ShaderType_Geometry);
				Device->SetBuffer(Shaders.Cubic, 3, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel | Graphics::ShaderType_Geometry);
				Device->UpdateBuffer(Shaders.Cubic, &Depth);

				for (auto It = Opaque->Begin(); It != Opaque->End(); ++It)
				{
					Engine::Components::SoftBody* SoftBody = (Engine::Components::SoftBody*)*It;
					if (!SoftBody->GetBody() || SoftBody->GetEntity()->Transform->Position.Distance(System->GetScene()->View.WorldPosition) >= System->GetScene()->View.ViewDistance + SoftBody->GetEntity()->Transform->Scale.Length())
						continue;

					if (!Appearance::UploadCubicDepth(Device, SoftBody->GetSurface()))
						continue;

					SoftBody->Fill(Device, IndexBuffer, VertexBuffer);

					Device->Render.World.Identify();
					Device->UpdateBuffer(Graphics::RenderBufferType_Render);
					Device->SetVertexBuffer(VertexBuffer, 0, sizeof(Compute::Vertex), 0);
					Device->SetIndexBuffer(IndexBuffer, Graphics::Format_R32_Uint, 0);
					Device->DrawIndexed((unsigned int)SoftBody->GetIndices().size(), 0, 0);
				}

				for (auto It = Limpid->Begin(); It != Limpid->End(); ++It)
				{
					Engine::Components::SoftBody* SoftBody = (Engine::Components::SoftBody*)*It;
					if (!SoftBody->GetBody() || SoftBody->GetEntity()->Transform->Position.Distance(System->GetScene()->View.WorldPosition) >= System->GetScene()->View.ViewDistance + SoftBody->GetEntity()->Transform->Scale.Length())
						continue;

					if (!Appearance::UploadCubicDepth(Device, SoftBody->GetSurface()))
						continue;

					SoftBody->Fill(Device, IndexBuffer, VertexBuffer);

					Device->Render.World.Identify();
					Device->UpdateBuffer(Graphics::RenderBufferType_Render);
					Device->SetVertexBuffer(VertexBuffer, 0, sizeof(Compute::Vertex), 0);
					Device->SetIndexBuffer(IndexBuffer, Graphics::Format_R32_Uint, 0);
					Device->DrawIndexed((unsigned int)SoftBody->GetIndices().size(), 0, 0);
				}

				Device->SetTexture2D(nullptr, 1);
				Device->SetShader(nullptr, Graphics::ShaderType_Geometry);
			}
			Rest::Pool<Component*>* SoftBodyRenderer::GetGeometry(uint64_t Index)
			{
				return Opaque;
			}
			uint64_t SoftBodyRenderer::GetGeometryCount()
			{
				return 1;
			}

			EmitterRenderer::EmitterRenderer(RenderSystem* Lab) : Renderer(Lab)
			{
				Geometric = true;
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
			void EmitterRenderer::Initialize()
			{
				Opaque = System->GetScene()->GetComponents<Engine::Components::Emitter>();
				Limpid = System->GetScene()->GetComponents<Engine::Components::LimpidEmitter>();
			}
			void EmitterRenderer::Cull(const Viewer& View)
			{
				System->GetScene()->SortEntitiesBackToFront(Limpid);
				for (auto It = Opaque->Begin(); It != Opaque->End(); ++It)
				{
					Engine::Components::Emitter* Emitter = (Engine::Components::Emitter*)*It;
					Entity* Base = Emitter->GetEntity();

					float Hardness = 1.0f - Base->Transform->Position.Distance(View.WorldPosition) / (View.ViewDistance + Emitter->Volume);
					if (Hardness > 0.0f)
						Emitter->Visibility = Compute::MathCommon::IsClipping(View.ViewProjection, Base->Transform->GetWorld(), 1.5f) == -1 ? Hardness : 0.0f;
					else
						Emitter->Visibility = 0.0f;
				}

				for (auto It = Limpid->Begin(); It != Limpid->End(); ++It)
				{
					Engine::Components::Emitter* Emitter = (Engine::Components::Emitter*)*It;
					Entity* Base = Emitter->GetEntity();

					float Hardness = 1.0f - Base->Transform->Position.Distance(View.WorldPosition) / (View.ViewDistance + Emitter->Volume);
					if (Hardness > 0.0f)
						Emitter->Visibility = Compute::MathCommon::IsClipping(View.ViewProjection, Base->Transform->GetWorld(), 1.5f) == -1 ? Hardness : 0.0f;
					else
						Emitter->Visibility = 0.0f;
				}
			}
			void EmitterRenderer::RenderStep(Rest::Timer* Time, bool Limpidity)
			{
				Graphics::GraphicsDevice* Device = System->GetDevice();
				Rest::Pool<Component*>* Array = (Limpidity ? Limpid : Opaque);
				if (!Array || Array->Empty())
					return;

				Graphics::PrimitiveTopology T = Device->GetPrimitiveTopology();
				Device->SetDepthStencilState(DepthStencil);
				Device->SetSamplerState(Sampler);
				Device->SetBlendState(AdditiveBlend);
				Device->SetRasterizerState(BackRasterizer);
				Device->SetPrimitiveTopology(Graphics::PrimitiveTopology_Point_List);
				Device->SetShader(Shaders.Main, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);
				Device->SetVertexBuffer(nullptr, 0, 0, 0);
				System->GetScene()->SetSurface();

				Compute::Matrix4x4 View = System->GetScene()->View.ViewProjection * System->GetScene()->View.Projection.Invert();
				for (auto It = Array->Begin(); It != Array->End(); ++It)
				{
					Engine::Components::Emitter* Emitter = (Engine::Components::Emitter*)*It;
					if (!Emitter->GetBuffer())
						continue;

					if (!Appearance::UploadPhase(Device, Emitter->GetSurface()))
						continue;

					Device->Render.World = System->GetScene()->View.Projection;
					Device->Render.WorldViewProjection = (Emitter->QuadBased ? View : System->GetScene()->View.ViewProjection);
					
					if (Emitter->Connected)
						Device->Render.WorldViewProjection = Emitter->GetEntity()->Transform->GetWorld() * Device->Render.WorldViewProjection;

					Device->UpdateBuffer(Emitter->GetBuffer());
					Device->SetBuffer(Emitter->GetBuffer(), 8);
					Device->SetShader(Emitter->QuadBased ? Shaders.Main : nullptr, Graphics::ShaderType_Geometry);
					Device->UpdateBuffer(Graphics::RenderBufferType_Render);
					Device->Draw((unsigned int)Emitter->GetBuffer()->GetArray()->Size(), 0);
				}

				Device->FlushTexture2D(1, 7);
				Device->SetPrimitiveTopology(T);
				Device->SetShader(nullptr, Graphics::ShaderType_Geometry);
			}
			void EmitterRenderer::RenderSubstep(Rest::Timer* Time, bool Limpidity, bool Static)
			{
				Graphics::GraphicsDevice* Device = System->GetDevice();
				Rest::Pool<Component*>* Array = (Limpidity ? Limpid : Opaque);
				if (!Array || Array->Empty())
					return;

				Graphics::PrimitiveTopology T = Device->GetPrimitiveTopology();
				Device->SetDepthStencilState(DepthStencil);
				Device->SetSamplerState(Sampler);
				Device->SetBlendState(AdditiveBlend);
				Device->SetRasterizerState(BackRasterizer);
				Device->SetPrimitiveTopology(Graphics::PrimitiveTopology_Point_List);
				Device->SetShader(Shaders.Main, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);
				Device->SetVertexBuffer(nullptr, 0, 0, 0);
				System->GetScene()->SetSurface();

				Compute::Matrix4x4 View = System->GetScene()->View.ViewProjection * System->GetScene()->View.Projection.Invert();
				for (auto It = Array->Begin(); It != Array->End(); ++It)
				{
					Engine::Components::Emitter* Emitter = (Engine::Components::Emitter*)*It;
					if ((Static && !Emitter->Static) || !Emitter->GetBuffer())
						continue;

					if (!Appearance::UploadPhase(Device, Emitter->GetSurface()))
						continue;

					Device->Render.World = System->GetScene()->View.Projection;
					Device->Render.WorldViewProjection = (Emitter->QuadBased ? View : System->GetScene()->View.ViewProjection);
					
					if (Emitter->Connected)
						Device->Render.WorldViewProjection = Emitter->GetEntity()->Transform->GetWorld() * Device->Render.WorldViewProjection;

					Device->UpdateBuffer(Emitter->GetBuffer());
					Device->SetBuffer(Emitter->GetBuffer(), 8);
					Device->SetShader(Emitter->QuadBased ? Shaders.Main : nullptr, Graphics::ShaderType_Geometry);
					Device->UpdateBuffer(Graphics::RenderBufferType_Render);
					Device->Draw((unsigned int)Emitter->GetBuffer()->GetArray()->Size(), 0);
				}

				Device->FlushTexture2D(1, 7);
				Device->SetPrimitiveTopology(T);
				Device->SetShader(nullptr, Graphics::ShaderType_Geometry);
			}
			void EmitterRenderer::RenderDepthLinear(Rest::Timer* Time)
			{
				Graphics::GraphicsDevice* Device = System->GetDevice();
				if ((!Opaque || Opaque->Empty()) && (!Limpid || Limpid->Empty()))
					return;

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
				for (auto It = Opaque->Begin(); It != Opaque->End(); ++It)
				{
					Engine::Components::Emitter* Emitter = (Engine::Components::Emitter*)*It;
					if (!Emitter->GetBuffer())
						continue;

					if (!Appearance::UploadDepth(Device, Emitter->GetSurface()))
						continue;

					Device->Render.World = Emitter->QuadBased ? System->GetScene()->View.Projection : Compute::Matrix4x4::Identity();
					Device->Render.WorldViewProjection = (Emitter->QuadBased ? View : System->GetScene()->View.ViewProjection);
					
					if (Emitter->Connected)
						Device->Render.WorldViewProjection = Emitter->GetEntity()->Transform->GetWorld() * Device->Render.WorldViewProjection;

					Device->UpdateBuffer(Emitter->GetBuffer());
					Device->SetBuffer(Emitter->GetBuffer(), 8);
					Device->SetShader(Emitter->QuadBased ? Shaders.Linear : nullptr, Graphics::ShaderType_Geometry);
					Device->UpdateBuffer(Graphics::RenderBufferType_Render);
					Device->Draw((unsigned int)Emitter->GetBuffer()->GetArray()->Size(), 0);
				}

				for (auto It = Limpid->Begin(); It != Limpid->End(); ++It)
				{
					Engine::Components::Emitter* Emitter = (Engine::Components::Emitter*)*It;
					if (!Emitter->GetBuffer())
						continue;

					if (!Appearance::UploadDepth(Device, Emitter->GetSurface()))
						continue;

					Device->Render.World = Emitter->QuadBased ? System->GetScene()->View.Projection : Compute::Matrix4x4::Identity();
					Device->Render.WorldViewProjection = (Emitter->QuadBased ? View : System->GetScene()->View.ViewProjection);

					if (Emitter->Connected)
						Device->Render.WorldViewProjection = Emitter->GetEntity()->Transform->GetWorld() * Device->Render.WorldViewProjection;

					Device->UpdateBuffer(Emitter->GetBuffer());
					Device->SetBuffer(Emitter->GetBuffer(), 8);
					Device->SetShader(Emitter->QuadBased ? Shaders.Linear : nullptr, Graphics::ShaderType_Geometry);
					Device->UpdateBuffer(Graphics::RenderBufferType_Render);
					Device->Draw((unsigned int)Emitter->GetBuffer()->GetArray()->Size(), 0);
				}

				Device->SetTexture2D(nullptr, 1);
				Device->SetPrimitiveTopology(T);
				Device->SetShader(nullptr, Graphics::ShaderType_Geometry);
			}
			void EmitterRenderer::RenderDepthCubic(Rest::Timer* Time, Compute::Matrix4x4* ViewProjection)
			{
				Graphics::GraphicsDevice* Device = System->GetDevice();
				if ((!Opaque || Opaque->Empty()) && (!Limpid || Limpid->Empty()))
					return;

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

				for (auto It = Opaque->Begin(); It != Opaque->End(); ++It)
				{
					Engine::Components::Emitter* Emitter = (Engine::Components::Emitter*)*It;
					if (!Emitter->GetBuffer())
						continue;

					if (!Appearance::UploadCubicDepth(Device, Emitter->GetSurface()))
						continue;

					Device->Render.World = (Emitter->Connected ? Emitter->GetEntity()->Transform->GetWorld() : Compute::Matrix4x4::Identity());			
					Device->UpdateBuffer(Emitter->GetBuffer());
					Device->SetBuffer(Emitter->GetBuffer(), 8);
					Device->SetShader(Emitter->QuadBased ? Shaders.Quad : Shaders.Point, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel | Graphics::ShaderType_Geometry);
					Device->UpdateBuffer(Graphics::RenderBufferType_Render);
					Device->Draw((unsigned int)Emitter->GetBuffer()->GetArray()->Size(), 0);
				}

				for (auto It = Limpid->Begin(); It != Limpid->End(); ++It)
				{
					Engine::Components::Emitter* Emitter = (Engine::Components::Emitter*)*It;
					if (!Emitter->GetBuffer())
						continue;

					if (!Appearance::UploadCubicDepth(Device, Emitter->GetSurface()))
						continue;

					Device->Render.World = (Emitter->Connected ? Emitter->GetEntity()->Transform->GetWorld() : Compute::Matrix4x4::Identity());
					Device->UpdateBuffer(Emitter->GetBuffer());
					Device->SetBuffer(Emitter->GetBuffer(), 8);
					Device->SetShader(Emitter->QuadBased ? Shaders.Quad : Shaders.Point, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel | Graphics::ShaderType_Geometry);
					Device->UpdateBuffer(Graphics::RenderBufferType_Render);
					Device->Draw((unsigned int)Emitter->GetBuffer()->GetArray()->Size(), 0);
				}

				Device->SetTexture2D(nullptr, 1);
				Device->SetPrimitiveTopology(T);
				Device->SetShader(nullptr, Graphics::ShaderType_Geometry);
			}
			Rest::Pool<Component*>* EmitterRenderer::GetGeometry(uint64_t Index)
			{
				return Opaque;
			}
			uint64_t EmitterRenderer::GetGeometryCount()
			{
				return 1;
			}

			LimpidRenderer::LimpidRenderer(RenderSystem* Lab) : Renderer(Lab)
			{
				Geometric = true;
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
			void LimpidRenderer::Initialize()
			{
				ResizeBuffers();
			}
			void LimpidRenderer::RenderStep(Rest::Timer* Time, bool Limpid)
			{
				if (Limpid)
					return;

				SceneGraph* Scene = System->GetScene();
				Graphics::MultiRenderTarget2D* S = Scene->GetSurface();
				Graphics::GraphicsDevice* Device = System->GetDevice();
				RenderPass.MipLevels = MipLevels1;

				Scene->SwapSurface(Surface1);
				Scene->SetSurfaceCleared();
				Scene->RenderStep(Time, true);
				Scene->SwapSurface(S);

				Device->CopyTargetTo(S, 0, Input1);
				Device->GenerateMips(Input1->GetTarget());
				Device->SetTarget(S, 0);
				Device->Clear(S, 0, 0, 0, 0);
				Device->UpdateBuffer(Shader, &RenderPass);
				Device->SetDepthStencilState(DepthStencil);
				Device->SetSamplerState(Sampler);
				Device->SetBlendState(Blend);
				Device->SetRasterizerState(Rasterizer);
				Device->SetShader(Shader, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);
				Device->SetBuffer(Shader, 3, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);
				Device->SetVertexBuffer(System->GetQuadVBuffer(), 0, sizeof(Compute::ShapeVertex), 0);
				Device->SetTexture2D(Input1->GetTarget(), 1);
				Device->SetTexture2D(S->GetTarget(1), 2);
				Device->SetTexture2D(S->GetTarget(2), 3);
				Device->SetTexture2D(S->GetTarget(3), 4);
				Device->SetTexture2D(Surface1->GetTarget(0), 5);
				Device->SetTexture2D(Surface1->GetTarget(1), 6);
				Device->SetTexture2D(Surface1->GetTarget(2), 7);
				Device->SetTexture2D(Surface1->GetTarget(3), 8);
				Device->UpdateBuffer(Graphics::RenderBufferType_Render);
				Device->Draw(6, 0);
				Device->FlushTexture2D(1, 8);
			}
			void LimpidRenderer::RenderSubstep(Rest::Timer* Time, bool Limpid, bool Static)
			{
				if (Limpid)
					return;

				SceneGraph* Scene = System->GetScene();
				Graphics::MultiRenderTarget2D* S = Scene->GetSurface();
				Graphics::GraphicsDevice* Device = System->GetDevice();
				RenderPass.MipLevels = MipLevels2;

				Scene->SwapSurface(Surface2);
				Scene->SetSurfaceCleared();
				Scene->RenderSubstep(Time, true, Static);
				Scene->SwapSurface(S);

				Device->CopyTargetTo(S, 0, Input2);
				Device->GenerateMips(Input2->GetTarget());
				Device->SetTarget(S, 0);
				Device->Clear(S, 0, 0, 0, 0);
				Device->UpdateBuffer(Shader, &RenderPass);
				Device->SetDepthStencilState(DepthStencil);
				Device->SetSamplerState(Sampler);
				Device->SetBlendState(Blend);
				Device->SetRasterizerState(Rasterizer);
				Device->SetShader(Shader, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);
				Device->SetBuffer(Shader, 3, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);
				Device->SetVertexBuffer(System->GetQuadVBuffer(), 0, sizeof(Compute::ShapeVertex), 0);
				Device->SetTexture2D(Input2->GetTarget(), 1);
				Device->SetTexture2D(S->GetTarget(1), 2);
				Device->SetTexture2D(S->GetTarget(2), 3);
				Device->SetTexture2D(S->GetTarget(3), 4);
				Device->SetTexture2D(Surface2->GetTarget(0), 5);
				Device->SetTexture2D(Surface2->GetTarget(1), 6);
				Device->SetTexture2D(Surface2->GetTarget(2), 7);
				Device->SetTexture2D(Surface2->GetTarget(3), 8);
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
			Rest::Pool<Component*>* LimpidRenderer::GetGeometry(uint64_t Index)
			{
				return nullptr;
			}
			uint64_t LimpidRenderer::GetGeometryCount()
			{
				return 0;
			}
			
			DepthRenderer::DepthRenderer(RenderSystem* Lab) : IntervalRenderer(Lab), ShadowDistance(0.5f)
			{
				Geometric = false;
				Timer.Delay = 10;
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
			void DepthRenderer::Initialize()
			{
				PointLights = System->GetScene()->GetComponents<Engine::Components::PointLight>();
				SpotLights = System->GetScene()->GetComponents<Engine::Components::SpotLight>();
				LineLights = System->GetScene()->GetComponents<Engine::Components::LineLight>();

				Rest::Pool<Engine::Component*>* Lights = System->GetScene()->GetComponents<Engine::Components::PointLight>();
				for (auto It = Lights->Begin(); It != Lights->End(); It++)
					(*It)->As<Engine::Components::PointLight>()->SetShadowCache(nullptr);

				for (auto It = Renderers.PointLight.begin(); It != Renderers.PointLight.end(); It++)
					delete *It;

				Renderers.PointLight.resize(Renderers.PointLightLimits);
				for (auto It = Renderers.PointLight.begin(); It != Renderers.PointLight.end(); It++)
				{
					Graphics::RenderTargetCube::Desc F = Graphics::RenderTargetCube::Desc();
					F.Size = (unsigned int)Renderers.PointLightResolution;
					F.FormatMode = Graphics::Format_R32G32_Float;

					*It = System->GetDevice()->CreateRenderTargetCube(F);
				}

				Lights = System->GetScene()->GetComponents<Engine::Components::SpotLight>();
				for (auto It = Lights->Begin(); It != Lights->End(); It++)
					(*It)->As<Engine::Components::SpotLight>()->SetShadowCache(nullptr);

				for (auto It = Renderers.SpotLight.begin(); It != Renderers.SpotLight.end(); It++)
					delete *It;

				Renderers.SpotLight.resize(Renderers.SpotLightLimits);
				for (auto It = Renderers.SpotLight.begin(); It != Renderers.SpotLight.end(); It++)
				{
					Graphics::RenderTarget2D::Desc F = Graphics::RenderTarget2D::Desc();
					F.Width = (unsigned int)Renderers.SpotLightResolution;
					F.Height = (unsigned int)Renderers.SpotLightResolution;
					F.FormatMode = Graphics::Format_R32G32_Float;

					*It = System->GetDevice()->CreateRenderTarget2D(F);
				}

				Lights = System->GetScene()->GetComponents<Engine::Components::LineLight>();
				for (auto It = Lights->Begin(); It != Lights->End(); It++)
					(*It)->As<Engine::Components::LineLight>()->SetShadowCache(nullptr);

				for (auto It = Renderers.LineLight.begin(); It != Renderers.LineLight.end(); It++)
					delete *It;

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
			void DepthRenderer::IntervalRenderStep(Rest::Timer* Time, bool Limpid)
			{
				Graphics::GraphicsDevice* Device = System->GetDevice();
				SceneGraph* Scene = System->GetScene();

				uint64_t Shadows = 0;
				for (auto It = PointLights->Begin(); It != PointLights->End(); It++)
				{
					Engine::Components::PointLight* Light = (Engine::Components::PointLight*)*It;
					if (Shadows >= Renderers.PointLight.size())
						break;

					Light->SetShadowCache(nullptr);
					if (!Light->Shadowed || Light->Visibility < ShadowDistance)
						continue;

					Graphics::RenderTargetCube* Target = Renderers.PointLight[Shadows];
					Device->SetTarget(Target, 0, 0, 0);
					Device->ClearDepth(Target);

					Scene->SetView(Compute::Matrix4x4::Identity(), Light->Projection, Light->GetEntity()->Transform->Position, Light->ShadowDistance);
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
					if (!Light->Shadowed || Light->Visibility < ShadowDistance)
						continue;

					Graphics::RenderTarget2D* Target = Renderers.SpotLight[Shadows];
					Device->SetTarget(Target, 0, 0, 0);
					Device->ClearDepth(Target);

					Scene->SetView(Light->View, Light->Projection, Light->GetEntity()->Transform->Position, Light->ShadowDistance);
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

					Scene->SetView(Light->View, Light->Projection, 0.0f, -1.0f);
					Scene->RenderDepthLinear(Time);
					Light->SetShadowCache(Target->GetTarget()); Shadows++;
				}

				Scene->RestoreViewBuffer(nullptr);
			}

			ProbeRenderer::ProbeRenderer(RenderSystem* Lab) : Renderer(Lab), Surface(nullptr), Size(128), MipLevels(7)
			{
				Geometric = false;
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
			void ProbeRenderer::Initialize()
			{
				CreateRenderTarget();
				ProbeLights = System->GetScene()->GetComponents<Engine::Components::ProbeLight>();
			}
			void ProbeRenderer::RenderStep(Rest::Timer* Time, bool Limpid)
			{
				SceneGraph* Scene = System->GetScene();
				Graphics::GraphicsDevice* Device = System->GetDevice();
				Graphics::MultiRenderTarget2D* S = Scene->GetSurface();
				Scene->SwapSurface(Surface);
				Scene->SetSurface();

				double ElapsedTime = Time->GetElapsedTime();
				for (auto It = ProbeLights->Begin(); It != ProbeLights->End(); It++)
				{
					Engine::Components::ProbeLight* Light = (Engine::Components::ProbeLight*)*It;
					if (Light->Visibility <= 0 || Light->IsImageBased())
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
					for (int j = 0; j < 6; j++)
					{
						Light->View[j] = Compute::Matrix4x4::CreateCubeMapLookAt(j, Position);
						Scene->SetView(Light->View[j], Light->Projection, Position, Light->CaptureRange);
						Scene->ClearSurface();
						Scene->RenderSubstep(Time, false, Light->StaticMask);
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
				if (Map == Size)
					return;

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
				Geometric = true;
				DepthStencil = Lab->GetDevice()->GetDepthStencilState("DEF_NONE_LESS");
				Rasterizer = Lab->GetDevice()->GetRasterizerState("DEF_CULL_BACK");
				Blend = Lab->GetDevice()->GetBlendState("DEF_ADDITIVE");
				Sampler = Lab->GetDevice()->GetSamplerState("DEF_TRILINEAR_X16");

				Graphics::Shader::Desc I = Graphics::Shader::Desc();
				I.Layout = Graphics::Shader::GetShapeVertexLayout();
				I.LayoutSize = 2;

				if (System->GetDevice()->GetSection("pass/light/shade/point", &I.Data))
					Shaders.PointShade = System->CompileShader("lr-point-shade", I, 0);

				if (System->GetDevice()->GetSection("pass/light/shade/spot", &I.Data))
					Shaders.SpotShade = System->CompileShader("lr-spot-shade", I, 0);

				if (System->GetDevice()->GetSection("pass/light/shade/line", &I.Data))
					Shaders.LineShade = System->CompileShader("lr-line-shade", I, 0);

				if (System->GetDevice()->GetSection("pass/light/base/point", &I.Data))
					Shaders.PointBase = System->CompileShader("lr-point-base", I, sizeof(PointLight));

				if (System->GetDevice()->GetSection("pass/light/base/spot", &I.Data))
					Shaders.SpotBase = System->CompileShader("lr-spot-base", I, sizeof(SpotLight));

				if (System->GetDevice()->GetSection("pass/light/base/line", &I.Data))
					Shaders.LineBase = System->CompileShader("lr-line-base", I, sizeof(LineLight));

				if (System->GetDevice()->GetSection("pass/light/base/probe", &I.Data))
					Shaders.Probe = System->CompileShader("lr-probe", I, sizeof(ProbeLight));

				if (System->GetDevice()->GetSection("pass/light/base/ambient", &I.Data))
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
			void LightRenderer::Initialize()
			{
				PointLights = System->GetScene()->GetComponents<Engine::Components::PointLight>();
				ProbeLights = System->GetScene()->GetComponents<Engine::Components::ProbeLight>();
				SpotLights = System->GetScene()->GetComponents<Engine::Components::SpotLight>();
				LineLights = System->GetScene()->GetComponents<Engine::Components::LineLight>();

				auto* RenderStages = System->GetRenderers();
				for (auto It = RenderStages->begin(); It != RenderStages->end(); It++)
				{
					if ((*It)->Id() == THAWK_COMPONENT_ID(DepthRenderer))
					{
						Engine::Renderers::DepthRenderer* DepthRenderer = (*It)->As<Engine::Renderers::DepthRenderer>();
						Quality.SpotLight = (float)DepthRenderer->Renderers.SpotLightResolution;
						Quality.LineLight = (float)DepthRenderer->Renderers.LineLightResolution;
					}

					if ((*It)->Id() == THAWK_COMPONENT_ID(ProbeRenderer))
						ProbeLight.MipLevels = (float)(*It)->As<Engine::Renderers::ProbeRenderer>()->MipLevels;
				}

				ResizeBuffers();
			}
			void LightRenderer::Cull(const Viewer& View)
			{
				float Hardness = 0.0f;
				for (auto It = PointLights->Begin(); It != PointLights->End(); ++It)
				{
					Engine::Components::PointLight* PointLight = (Engine::Components::PointLight*)*It;
					Entity* Base = PointLight->GetEntity();

					Hardness = 1.0f - Base->Transform->Position.Distance(View.WorldPosition) / View.ViewDistance;
					if (Hardness > 0.0f)
						PointLight->Visibility = Compute::MathCommon::IsClipping(View.ViewProjection, Base->Transform->GetWorld(), PointLight->Range) == -1 ? Hardness : 0.0f;
					else
						PointLight->Visibility = 0.0f;
				}

				for (auto It = SpotLights->Begin(); It != SpotLights->End(); ++It)
				{
					Engine::Components::SpotLight* SpotLight = (Engine::Components::SpotLight*)*It;
					Entity* Base = SpotLight->GetEntity();

					Hardness = 1.0f - Base->Transform->Position.Distance(View.WorldPosition) / View.ViewDistance;
					if (Hardness > 0.0f)
						SpotLight->Visibility = Compute::MathCommon::IsClipping(View.ViewProjection, Base->Transform->GetWorld(), SpotLight->Range) == -1 ? Hardness : 0.0f;
					else
						SpotLight->Visibility = 0.0f;
				}

				for (auto It = ProbeLights->Begin(); It != ProbeLights->End(); ++It)
				{
					Engine::Components::ProbeLight* ProbeLight = (Engine::Components::ProbeLight*)*It;
					Entity* Base = ProbeLight->GetEntity();

					if (ProbeLight->Infinity <= 0.0f)
					{
						Hardness = 1.0f - Base->Transform->Position.Distance(View.WorldPosition) / View.ViewDistance;
						if (Hardness > 0.0f)
							ProbeLight->Visibility = Compute::MathCommon::IsClipping(View.ViewProjection, Base->Transform->GetWorld(), ProbeLight->Range) == -1 ? Hardness : 0.0f;
						else
							ProbeLight->Visibility = 0.0f;
					}
					else
						ProbeLight->Visibility = 1.0f;
				}
			}
			void LightRenderer::RenderStep(Rest::Timer* Time, bool Limpid)
			{
				Graphics::GraphicsDevice* Device = System->GetDevice();
				Graphics::MultiRenderTarget2D* Surface = System->GetScene()->GetSurface();
				Graphics::Shader* Active = nullptr;
				
				Device->SetDepthStencilState(DepthStencil);
				Device->SetSamplerState(Sampler);
				Device->SetBlendState(Blend);
				Device->SetRasterizerState(Rasterizer);
				Device->CopyTargetTo(Surface, 0, Input1);
				Device->SetTarget(Output1, 0, 0, 0);
				Device->SetTexture2D(Input1->GetTarget(), 1);
				Device->SetTexture2D(Surface->GetTarget(1), 2);
				Device->SetTexture2D(Surface->GetTarget(2), 3);
				Device->SetTexture2D(Surface->GetTarget(3), 4);
				Device->SetShader(Shaders.Probe, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);
				Device->SetBuffer(Shaders.Probe, 3, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);
				Device->SetIndexBuffer(System->GetSphereIBuffer(), Graphics::Format_R32_Uint, 0);
				Device->SetVertexBuffer(System->GetSphereVBuffer(), 0, sizeof(Compute::ShapeVertex), 0);

				for (auto It = ProbeLights->Begin(); It != ProbeLights->End(); ++It)
				{
					Engine::Components::ProbeLight* Light = (Engine::Components::ProbeLight*)*It;
					if (Light->Visibility <= 0 || !Light->GetProbeCache())
						continue;

					ProbeLight.OwnWorldViewProjection = Compute::Matrix4x4::CreateTranslatedScale(Light->GetEntity()->Transform->Position, Light->Range * 1.25f) * System->GetScene()->View.ViewProjection;
					ProbeLight.Position = Light->GetEntity()->Transform->Position.InvertZ();
					ProbeLight.Lighting = Light->Diffuse.Mul(Light->Emission * Light->Visibility);
					ProbeLight.Scale = Light->GetEntity()->Transform->Scale;
					ProbeLight.Parallax = (Light->ParallaxCorrected ? 1.0f : 0.0f);
					ProbeLight.Range = Light->Range;
					ProbeLight.Infinity = Light->Infinity;

					Device->SetTextureCube(Light->GetProbeCache(), 5);
					Device->UpdateBuffer(Shaders.Probe, &ProbeLight);
					Device->DrawIndexed((unsigned int)System->GetSphereIBuffer()->GetElements(), 0, 0);
				}

				Device->SetShader(Shaders.PointBase, Graphics::ShaderType_Vertex);
				Device->SetBuffer(Shaders.PointBase, 3, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);

				for (auto It = PointLights->Begin(); It != PointLights->End(); ++It)
				{
					Engine::Components::PointLight* Light = (Engine::Components::PointLight*)*It;
					if (Light->Visibility <= 0.0f)
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

					PointLight.OwnWorldViewProjection = Compute::Matrix4x4::CreateTranslatedScale(Light->GetEntity()->Transform->Position, Light->Range * 1.25f) * System->GetScene()->View.ViewProjection;
					PointLight.Position = Light->GetEntity()->Transform->Position.InvertZ();
					PointLight.Lighting = Light->Diffuse.Mul(Light->Emission * Light->Visibility);
					PointLight.Range = Light->Range;

					Device->SetShader(Active, Graphics::ShaderType_Pixel);
					Device->UpdateBuffer(Shaders.PointBase, &PointLight);
					Device->DrawIndexed((unsigned int)System->GetSphereIBuffer()->GetElements(), 0, 0);
				}

				Device->SetShader(Shaders.SpotBase, Graphics::ShaderType_Vertex);
				Device->SetBuffer(Shaders.SpotBase, 3, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);
				Device->SetTexture2D(nullptr, 4);

				for (auto It = SpotLights->Begin(); It != SpotLights->End(); ++It)
				{
					Engine::Components::SpotLight* Light = (Engine::Components::SpotLight*)*It;
					if (Light->Visibility <= 0.0f)
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

					SpotLight.OwnWorldViewProjection = Compute::Matrix4x4::CreateScale(Light->Range * 1.25f) * Compute::Matrix4x4::CreateTranslation(Light->GetEntity()->Transform->Position) * System->GetScene()->View.ViewProjection;
					SpotLight.OwnViewProjection = Light->View * Light->Projection;
					SpotLight.Position = Light->GetEntity()->Transform->Position.InvertZ();
					SpotLight.Lighting = Light->Diffuse.Mul(Light->Emission * Light->Visibility);
					SpotLight.Range = Light->Range;
					SpotLight.Diffuse = (Light->ProjectMap != nullptr);

					Device->SetTexture2D(Light->ProjectMap, 5);
					Device->SetShader(Active, Graphics::ShaderType_Pixel);
					Device->UpdateBuffer(Shaders.SpotBase, &SpotLight);
					Device->DrawIndexed((unsigned int)System->GetSphereIBuffer()->GetElements(), 0, 0);
				}

				Device->SetTarget(Output1);
				Device->SetShader(Shaders.LineBase, Graphics::ShaderType_Vertex);
				Device->SetBuffer(Shaders.LineBase, 3, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);
				Device->SetVertexBuffer(System->GetQuadVBuffer(), 0, sizeof(Compute::ShapeVertex), 0);

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
				Device->SetTexture2D(Output1->GetTarget(), 5);
				Device->SetTextureCube(SkyMap, 6);
				Device->SetShader(Shaders.Ambient, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);
				Device->SetBuffer(Shaders.Ambient, 3, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);
				Device->UpdateBuffer(Shaders.Ambient, &AmbientLight);
				Device->Draw(6, 0);
				Device->FlushTexture2D(1, 6);
			}
			void LightRenderer::RenderSubstep(Rest::Timer* Time, bool Limpid, bool Static)
			{
				Graphics::GraphicsDevice* Device = System->GetDevice();
				Graphics::MultiRenderTarget2D* Surface = System->GetScene()->GetSurface();
				Graphics::Shader* Active = nullptr;
				Engine::Viewer View = System->GetScene()->GetCameraViewer();

				Device->SetDepthStencilState(DepthStencil);
				Device->SetSamplerState(Sampler);
				Device->SetBlendState(Blend);
				Device->SetRasterizerState(Rasterizer);
				Device->CopyTargetTo(Surface, 0, Input2);
				Device->SetTarget(Output2, 0, 0, 0);
				Device->SetTexture2D(Input2->GetTarget(), 1);
				Device->SetTexture2D(Surface->GetTarget(1), 2);
				Device->SetTexture2D(Surface->GetTarget(2), 3);
				Device->SetTexture2D(Surface->GetTarget(3), 4);
				Device->SetIndexBuffer(System->GetSphereIBuffer(), Graphics::Format_R32_Uint, 0);
				Device->SetVertexBuffer(System->GetSphereVBuffer(), 0, sizeof(Compute::ShapeVertex), 0);

				if (RecursiveProbes)
				{
					Device->SetShader(Shaders.Probe, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);
					Device->SetBuffer(Shaders.Probe, 3, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);

					for (auto It = ProbeLights->Begin(); It != ProbeLights->End(); ++It)
					{
						Engine::Components::ProbeLight* Light = (Engine::Components::ProbeLight*)*It;
						if (!Light->GetProbeCache() || Light->RenderLocked)
							continue;

						float Visibility = Light->Visibility;
						if (Light->Visibility <= 0 && (Visibility = 1.0f - Light->GetEntity()->Transform->Position.Distance(View.WorldPosition) / View.ViewDistance) <= 0.0f)
							continue;

						ProbeLight.OwnWorldViewProjection = Compute::Matrix4x4::CreateTranslatedScale(Light->GetEntity()->Transform->Position, Light->Range * 1.25f) * System->GetScene()->View.ViewProjection;
						ProbeLight.Position = Light->GetEntity()->Transform->Position.InvertZ();
						ProbeLight.Lighting = Light->Diffuse.Mul(Light->Emission * Visibility);
						ProbeLight.Scale = Light->GetEntity()->Transform->Scale;
						ProbeLight.Parallax = (Light->ParallaxCorrected ? 1.0f : 0.0f);
						ProbeLight.Range = Light->Range;
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

					float Visibility = 1.0f - Light->GetEntity()->Transform->Position.Distance(System->GetScene()->View.WorldPosition) / System->GetScene()->View.ViewDistance;
					if (Visibility <= 0.0f)
						continue;

					Visibility = Compute::MathCommon::IsClipping(System->GetScene()->View.ViewProjection, Light->GetEntity()->Transform->GetWorld(), Light->Range) == -1 ? Visibility : 0.0f;
					if (Visibility <= 0.0f)
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

					PointLight.OwnWorldViewProjection = Compute::Matrix4x4::CreateTranslatedScale(Light->GetEntity()->Transform->Position, Light->Range * 1.25f) * System->GetScene()->View.ViewProjection;
					PointLight.Position = Light->GetEntity()->Transform->Position.InvertZ();
					PointLight.Lighting = Light->Diffuse.Mul(Light->Emission * Light->Visibility);
					PointLight.Range = Light->Range;

					Device->SetShader(Active, Graphics::ShaderType_Pixel);
					Device->UpdateBuffer(Shaders.PointBase, &PointLight);
					Device->DrawIndexed((unsigned int)System->GetSphereIBuffer()->GetElements(), 0, 0);
				}

				Device->SetShader(Shaders.SpotBase, Graphics::ShaderType_Vertex);
				Device->SetBuffer(Shaders.SpotBase, 3, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);
				Device->SetTexture2D(nullptr, 4);

				for (auto It = SpotLights->Begin(); It != SpotLights->End(); ++It)
				{
					Engine::Components::SpotLight* Light = (Engine::Components::SpotLight*)*It;

					float Visibility = 1.0f - Light->GetEntity()->Transform->Position.Distance(System->GetScene()->View.WorldPosition) / System->GetScene()->View.ViewDistance;
					if (Visibility <= 0.0f)
						continue;

					Visibility = Compute::MathCommon::IsClipping(System->GetScene()->View.ViewProjection, Light->GetEntity()->Transform->GetWorld(), Light->Range) == -1 ? Visibility : 0.0f;
					if (Visibility <= 0.0f)
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

					SpotLight.OwnWorldViewProjection = Compute::Matrix4x4::CreateScale(Light->Range * 1.25f) * Compute::Matrix4x4::CreateTranslation(Light->GetEntity()->Transform->Position) * System->GetScene()->View.ViewProjection;
					SpotLight.OwnViewProjection = Light->View * Light->Projection;
					SpotLight.Position = Light->GetEntity()->Transform->Position.InvertZ();
					SpotLight.Lighting = Light->Diffuse.Mul(Light->Emission * Light->Visibility);
					SpotLight.Range = Light->Range;
					SpotLight.Diffuse = (Light->ProjectMap != nullptr);

					Device->SetTexture2D(Light->ProjectMap, 5);
					Device->SetShader(Active, Graphics::ShaderType_Pixel);
					Device->UpdateBuffer(Shaders.SpotBase, &SpotLight);
					Device->DrawIndexed((unsigned int)System->GetSphereIBuffer()->GetElements(), 0, 0);
				}

				Device->SetTarget(Output2);
				Device->SetShader(Shaders.LineBase, Graphics::ShaderType_Vertex);
				Device->SetBuffer(Shaders.LineBase, 3, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);
				Device->SetVertexBuffer(System->GetQuadVBuffer(), 0, sizeof(Compute::ShapeVertex), 0);

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

					Device->SetShader(Active, Graphics::ShaderType_Pixel);
					Device->UpdateBuffer(Shaders.LineBase, &LineLight);
					Device->Draw(6, 0);
				}

				AmbientLight.SkyOffset = System->GetScene()->View.Projection.Invert() * Compute::Matrix4x4::CreateRotation(System->GetScene()->View.WorldRotation);
				Device->SetTarget(Surface, 0, 0, 0, 0);
				Device->ClearDepth(Surface);
				Device->SetTexture2D(Output2->GetTarget(), 5);
				Device->SetTextureCube(SkyMap, 6);
				Device->SetShader(Shaders.Ambient, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);
				Device->SetBuffer(Shaders.Ambient, 3, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);
				Device->UpdateBuffer(Shaders.Ambient, &AmbientLight);
				Device->Draw(6, 0);
				Device->FlushTexture2D(1, 5);
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

			ImageRenderer::ImageRenderer(RenderSystem* Lab) : ImageRenderer(Lab, nullptr)
			{
			}
			ImageRenderer::ImageRenderer(RenderSystem* Lab, Graphics::RenderTarget2D* Value) : Renderer(Lab), RenderTarget(Value)
			{
				Geometric = false;
				DepthStencil = Lab->GetDevice()->GetDepthStencilState("DEF_NONE");
				Rasterizer = Lab->GetDevice()->GetRasterizerState("DEF_CULL_BACK");
				Blend = Lab->GetDevice()->GetBlendState("DEF_OVERWRITE");
				Sampler = Lab->GetDevice()->GetSamplerState("DEF_TRILINEAR_X16");
			}
			ImageRenderer::~ImageRenderer()
			{
			}
			void ImageRenderer::Deserialize(ContentManager* Content, Rest::Document* Node)
			{
			}
			void ImageRenderer::Serialize(ContentManager* Content, Rest::Document* Node)
			{
			}
			void ImageRenderer::RenderStep(Rest::Timer* Time, bool Limpid)
			{
				Graphics::GraphicsDevice* Device = System->GetDevice();
				if (!System->GetScene()->GetSurface()->GetTarget(0)->GetResource())
					return;

				if (RenderTarget != nullptr)
					Device->SetTarget(RenderTarget);
				else
					Device->SetTarget();

				Device->Render.Diffuse = 1.0f;
				Device->Render.WorldViewProjection.Identify();
				Device->SetDepthStencilState(DepthStencil);
				Device->SetSamplerState(Sampler);
				Device->SetBlendState(Blend);
				Device->SetRasterizerState(Rasterizer);
				Device->SetShader(Device->GetBasicEffect(), Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);
				Device->SetVertexBuffer(System->GetQuadVBuffer(), 0, sizeof(Compute::ShapeVertex), 0);
				Device->SetTexture2D(System->GetScene()->GetSurface()->GetTarget(0), 0);
				Device->UpdateBuffer(Graphics::RenderBufferType_Render);
				Device->Draw(6, 0);
				Device->SetTexture2D(nullptr, 0);
			}

			ReflectionsRenderer::ReflectionsRenderer(RenderSystem* Lab) : PostProcessRenderer(Lab), Pass1(nullptr), Pass2(nullptr)
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

			DepthOfFieldRenderer::DepthOfFieldRenderer(RenderSystem* Lab) : PostProcessRenderer(Lab)
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
			
			EmissionRenderer::EmissionRenderer(RenderSystem* Lab) : PostProcessRenderer(Lab)
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
			
			GlitchRenderer::GlitchRenderer(RenderSystem* Lab) : PostProcessRenderer(Lab), ScanLineJitter(0), VerticalJump(0), HorizontalShake(0), ColorDrift(0)
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

			AmbientOcclusionRenderer::AmbientOcclusionRenderer(RenderSystem* Lab) : PostProcessRenderer(Lab), Pass1(nullptr), Pass2(nullptr)
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

			DirectOcclusionRenderer::DirectOcclusionRenderer(RenderSystem* Lab) : PostProcessRenderer(Lab), Pass1(nullptr), Pass2(nullptr)
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
			
			ToneRenderer::ToneRenderer(RenderSystem* Lab) : PostProcessRenderer(Lab)
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

			GUIRenderer::GUIRenderer(RenderSystem* Lab) : GUIRenderer(Lab, Application::Get() ? Application::Get()->Activity : nullptr)
			{
			}
			GUIRenderer::GUIRenderer(RenderSystem* Lab, Graphics::Activity* NewActivity) : Renderer(Lab), Context(nullptr), AA(true)
			{
				Geometric = false;
				Context = new GUI::Context(System->GetDevice(), NewActivity);
			}
			GUIRenderer::~GUIRenderer()
			{
				delete Context;
			}
			void GUIRenderer::RenderStep(Rest::Timer* Timer, bool Limpid)
			{
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
		}
	}
}