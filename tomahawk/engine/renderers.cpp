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

				if (System->GetDevice()->GetSection("geometry/model/gbuffer", &I.Data))
					Shaders.GBuffer = System->CompileShader("MR_GBUFFER", I, 0);

				if (System->GetDevice()->GetSection("geometry/model/depth", &I.Data))
					Shaders.Depth90 = System->CompileShader("MR_DEPTH90", I, 0);

				if (System->GetDevice()->GetSection("geometry/model/depth-360", &I.Data))
					Shaders.Depth360 = System->CompileShader("MR_DEPTH360", I, sizeof(Depth360));
			}
			ModelRenderer::~ModelRenderer()
			{
				System->FreeShader("MR_GBUFFER", Shaders.GBuffer);
				System->FreeShader("MR_DEPTH90", Shaders.Depth90);
				System->FreeShader("MR_DEPTH360", Shaders.Depth360);
			}
			void ModelRenderer::OnCulling(const Viewer& View)
			{
				for (auto It = Models->Begin(); It != Models->End(); ++It)
				{
					Engine::Components::Model* Model = (Engine::Components::Model*)*It;
					if (Model->GetDrawable() != nullptr)
						Model->Visibility = Model->IsVisibleTo(View, &Model->GetBoundingBox());
					else
						Model->Visibility = false;
				}
			}
			void ModelRenderer::OnInitialize()
			{
				Models = System->GetScene()->GetComponents<Engine::Components::Model>();
			}
			void ModelRenderer::OnRender(Rest::Timer* Time)
			{
				Graphics::GraphicsDevice* Device = System->GetDevice();
				if (!Models || Models->Empty())
					return;

				Device->SetDepthStencilState(DepthStencil);
				Device->SetSamplerState(Sampler);
				Device->SetBlendState(Blend);
				Device->SetRasterizerState(BackRasterizer);
				Device->SetShader(Shaders.GBuffer, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);
				System->GetScene()->SetSurface();

				for (auto It = Models->Begin(); It != Models->End(); ++It)
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
			}
			void ModelRenderer::OnPhaseRender(Rest::Timer* Time)
			{
				Graphics::GraphicsDevice* Device = System->GetDevice();
				if (!Models || Models->Empty())
					return;

				Device->SetDepthStencilState(DepthStencil);
				Device->SetSamplerState(Sampler);
				Device->SetBlendState(Blend);
				Device->SetRasterizerState(BackRasterizer);
				Device->SetShader(Shaders.GBuffer, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);
				System->GetScene()->SetSurface();

				for (auto It = Models->Begin(); It != Models->End(); ++It)
				{
					Engine::Components::Model* Model = (Engine::Components::Model*)*It;
					auto* Drawable = Model->GetDrawable();

					if (!Drawable || !Model->IsVisibleTo(System->GetScene()->View, nullptr))
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
			}
			void ModelRenderer::OnDepthRender(Rest::Timer* Time)
			{
				Graphics::GraphicsDevice* Device = System->GetDevice();
				if (!Models || Models->Empty())
					return;

				Device->SetDepthStencilState(DepthStencil);
				Device->SetSamplerState(Sampler);
				Device->SetBlendState(Blend);
				Device->SetRasterizerState(FrontRasterizer);
				Device->SetShader(Shaders.Depth90, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);

				for (auto It = Models->Begin(); It != Models->End(); ++It)
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
			}
			void ModelRenderer::OnCubicDepthRender(Rest::Timer* Time, Compute::Matrix4x4* ViewProjection)
			{
				Graphics::GraphicsDevice* Device = System->GetDevice();
				if (!Models || Models->Empty())
					return;

				memcpy(Depth360.FaceViewProjection, ViewProjection, sizeof(Compute::Matrix4x4) * 6);
				Device->SetDepthStencilState(DepthStencil);
				Device->SetSamplerState(Sampler);
				Device->SetBlendState(Blend);
				Device->SetRasterizerState(FrontRasterizer);
				Device->SetShader(Shaders.Depth360, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel | Graphics::ShaderType_Geometry);
				Device->SetBuffer(Shaders.Depth360, 3, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel | Graphics::ShaderType_Geometry);
				Device->UpdateBuffer(Shaders.Depth360, &Depth360);

				for (auto It = Models->Begin(); It != Models->End(); ++It)
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

				Device->SetShader(nullptr, Graphics::ShaderType_Geometry);
			}

			SkinModelRenderer::SkinModelRenderer(Engine::RenderSystem* Lab) : Renderer(Lab)
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

				if (System->GetDevice()->GetSection("geometry/skin-model/gbuffer", &I.Data))
					Shaders.GBuffer = System->CompileShader("SMR_GBUFFER", I, 0);

				if (System->GetDevice()->GetSection("geometry/skin-model/depth", &I.Data))
					Shaders.Depth90 = System->CompileShader("SMR_DEPTH90", I, 0);

				if (System->GetDevice()->GetSection("geometry/skin-model/depth-360", &I.Data))
					Shaders.Depth360 = System->CompileShader("SMR_DEPTH360", I, sizeof(Depth360));
			}
			SkinModelRenderer::~SkinModelRenderer()
			{
				System->FreeShader("SMR_GBUFFER", Shaders.GBuffer);
				System->FreeShader("SMR_DEPTH90", Shaders.Depth90);
				System->FreeShader("SMR_DEPTH360", Shaders.Depth360);
			}
			void SkinModelRenderer::OnInitialize()
			{
				SkinModels = System->GetScene()->GetComponents<Engine::Components::SkinModel>();
			}
			void SkinModelRenderer::OnCulling(const Viewer& View)
			{
				for (auto It = SkinModels->Begin(); It != SkinModels->End(); ++It)
				{
					Engine::Components::SkinModel* Model = (Engine::Components::SkinModel*)*It;
					if (Model->GetDrawable() != nullptr)
						Model->Visibility = Model->IsVisibleTo(View, &Model->GetBoundingBox());
					else
						Model->Visibility = false;
				}
			}
			void SkinModelRenderer::OnRender(Rest::Timer* Time)
			{
				Graphics::GraphicsDevice* Device = System->GetDevice();
				if (!SkinModels || SkinModels->Empty())
					return;

				Device->SetDepthStencilState(DepthStencil);
				Device->SetSamplerState(Sampler);
				Device->SetBlendState(Blend);
				Device->SetRasterizerState(BackRasterizer);
				Device->SetShader(Shaders.GBuffer, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);
				System->GetScene()->SetSurface();

				for (auto It = SkinModels->Begin(); It != SkinModels->End(); ++It)
				{
					Engine::Components::SkinModel* SkinModel = (Engine::Components::SkinModel*)*It;
					if (!SkinModel->Visibility)
						continue;

					auto* Drawable = SkinModel->GetDrawable();
					if (!Drawable)
						continue;

					Device->Animation.HasAnimation = !SkinModel->GetDrawable()->Joints.empty();
					if (Device->Animation.HasAnimation > 0)
						memcpy(Device->Animation.Offsets, SkinModel->Skeleton.Transform, 96 * sizeof(Compute::Matrix4x4));

					Device->UpdateBuffer(Graphics::RenderBufferType_Animation);
					for (auto&& Mesh : Drawable->Meshes)
					{
						if (!Appearance::UploadPhase(Device, SkinModel->GetSurface(Mesh)))
							continue;

						Device->Render.World = Mesh->World * SkinModel->GetEntity()->Transform->GetWorld();
						Device->Render.WorldViewProjection = Device->Render.World * System->GetScene()->View.ViewProjection;
						Device->UpdateBuffer(Graphics::RenderBufferType_Render);
						Device->DrawIndexed(Mesh);
					}
				}
			}
			void SkinModelRenderer::OnPhaseRender(Rest::Timer* Time)
			{
				Graphics::GraphicsDevice* Device = System->GetDevice();
				if (!SkinModels || SkinModels->Empty())
					return;

				Device->SetDepthStencilState(DepthStencil);
				Device->SetSamplerState(Sampler);
				Device->SetBlendState(Blend);
				Device->SetRasterizerState(BackRasterizer);
				Device->SetShader(Shaders.GBuffer, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);
				System->GetScene()->SetSurface();

				for (auto It = SkinModels->Begin(); It != SkinModels->End(); ++It)
				{
					Engine::Components::SkinModel* SkinModel = (Engine::Components::SkinModel*)*It;
					auto* Drawable = SkinModel->GetDrawable();

					if (!Drawable || !SkinModel->IsVisibleTo(System->GetScene()->View, nullptr))
						continue;

					Device->Animation.HasAnimation = !SkinModel->GetDrawable()->Joints.empty();
					if (Device->Animation.HasAnimation > 0)
						memcpy(Device->Animation.Offsets, SkinModel->Skeleton.Transform, 96 * sizeof(Compute::Matrix4x4));

					Device->UpdateBuffer(Graphics::RenderBufferType_Animation);
					for (auto&& Mesh : Drawable->Meshes)
					{
						if (!Appearance::UploadPhase(Device, SkinModel->GetSurface(Mesh)))
							continue;

						Device->Render.World = Mesh->World * SkinModel->GetEntity()->Transform->GetWorld();
						Device->Render.WorldViewProjection = Device->Render.World * System->GetScene()->View.ViewProjection;
						Device->UpdateBuffer(Graphics::RenderBufferType_Render);
						Device->DrawIndexed(Mesh);
					}
				}
			}
			void SkinModelRenderer::OnDepthRender(Rest::Timer* Time)
			{
				Graphics::GraphicsDevice* Device = System->GetDevice();
				if (!SkinModels || SkinModels->Empty())
					return;

				Device->SetDepthStencilState(DepthStencil);
				Device->SetSamplerState(Sampler);
				Device->SetBlendState(Blend);
				Device->SetRasterizerState(FrontRasterizer);
				Device->SetShader(Shaders.Depth90, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);

				for (auto It = SkinModels->Begin(); It != SkinModels->End(); ++It)
				{
					Engine::Components::SkinModel* SkinModel = (Engine::Components::SkinModel*)*It;
					auto* Drawable = SkinModel->GetDrawable();

					if (!Drawable || !SkinModel->IsVisibleTo(System->GetScene()->View, nullptr))
						continue;

					Device->Animation.HasAnimation = !SkinModel->GetDrawable()->Joints.empty();
					if (Device->Animation.HasAnimation > 0)
						memcpy(Device->Animation.Offsets, SkinModel->Skeleton.Transform, 96 * sizeof(Compute::Matrix4x4));

					Device->UpdateBuffer(Graphics::RenderBufferType_Animation);
					for (auto&& Mesh : Drawable->Meshes)
					{
						if (!Appearance::UploadDepth(Device, SkinModel->GetSurface(Mesh)))
							continue;

						Device->Render.World = Mesh->World * SkinModel->GetEntity()->Transform->GetWorld();
						Device->Render.WorldViewProjection = Device->Render.World * System->GetScene()->View.ViewProjection;
						Device->UpdateBuffer(Graphics::RenderBufferType_Render);
						Device->DrawIndexed(Mesh);
					}
				}
			}
			void SkinModelRenderer::OnCubicDepthRender(Rest::Timer* Time, Compute::Matrix4x4* ViewProjection)
			{
				Graphics::GraphicsDevice* Device = System->GetDevice();
				if (!SkinModels || SkinModels->Empty())
					return;

				memcpy(Depth360.FaceViewProjection, ViewProjection, sizeof(Compute::Matrix4x4) * 6);
				Device->SetDepthStencilState(DepthStencil);
				Device->SetSamplerState(Sampler);
				Device->SetBlendState(Blend);
				Device->SetRasterizerState(FrontRasterizer);
				Device->SetShader(Shaders.Depth360, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel | Graphics::ShaderType_Geometry);
				Device->SetBuffer(Shaders.Depth360, 3, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel | Graphics::ShaderType_Geometry);
				Device->UpdateBuffer(Shaders.Depth360, &Depth360);

				for (auto It = SkinModels->Begin(); It != SkinModels->End(); ++It)
				{
					Engine::Components::SkinModel* SkinModel = (Engine::Components::SkinModel*)*It;
					auto* Drawable = SkinModel->GetDrawable();

					if (!Drawable || !SkinModel->IsNearTo(System->GetScene()->View))
						continue;

					Device->Animation.HasAnimation = !SkinModel->GetDrawable()->Joints.empty();
					if (Device->Animation.HasAnimation > 0)
						memcpy(Device->Animation.Offsets, SkinModel->Skeleton.Transform, 96 * sizeof(Compute::Matrix4x4));

					Device->UpdateBuffer(Graphics::RenderBufferType_Animation);
					for (auto&& Mesh : Drawable->Meshes)
					{
						if (!Appearance::UploadCubicDepth(Device, SkinModel->GetSurface(Mesh)))
							continue;

						Device->Render.World = Mesh->World * SkinModel->GetEntity()->Transform->GetWorld();
						Device->UpdateBuffer(Graphics::RenderBufferType_Render);
						Device->DrawIndexed(Mesh);
					}
				}

				Device->SetShader(nullptr, Graphics::ShaderType_Geometry);
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

				if (System->GetDevice()->GetSection("geometry/model/gbuffer", &I.Data))
					Shaders.GBuffer = System->CompileShader("MR_GBUFFER", I, 0);

				if (System->GetDevice()->GetSection("geometry/model/depth", &I.Data))
					Shaders.Depth90 = System->CompileShader("MR_DEPTH90", I, 0);

				if (System->GetDevice()->GetSection("geometry/model/depth-360", &I.Data))
					Shaders.Depth360 = System->CompileShader("MR_DEPTH360", I, sizeof(Depth360));
			}
			SoftBodyRenderer::~SoftBodyRenderer()
			{
				System->FreeShader("MR_GBUFFER", Shaders.GBuffer);
				System->FreeShader("MR_DEPTH90", Shaders.Depth90);
				System->FreeShader("MR_DEPTH360", Shaders.Depth360);
				delete VertexBuffer;
				delete IndexBuffer;
			}
			void SoftBodyRenderer::OnInitialize()
			{
				SoftBodies = System->GetScene()->GetComponents<Engine::Components::SoftBody>();
			}
			void SoftBodyRenderer::OnCulling(const Viewer& View)
			{
				for (auto It = SoftBodies->Begin(); It != SoftBodies->End(); ++It)
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
			void SoftBodyRenderer::OnRender(Rest::Timer* Time)
			{
				Graphics::GraphicsDevice* Device = System->GetDevice();
				if (!SoftBodies || SoftBodies->Empty())
					return;

				Device->SetDepthStencilState(DepthStencil);
				Device->SetSamplerState(Sampler);
				Device->SetBlendState(Blend);
				Device->SetRasterizerState(BackRasterizer);
				Device->SetShader(Shaders.GBuffer, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);
				System->GetScene()->SetSurface();

				for (auto It = SoftBodies->Begin(); It != SoftBodies->End(); ++It)
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
			}
			void SoftBodyRenderer::OnPhaseRender(Rest::Timer* Time)
			{
				Graphics::GraphicsDevice* Device = System->GetDevice();
				if (!SoftBodies || SoftBodies->Empty())
					return;

				Device->SetDepthStencilState(DepthStencil);
				Device->SetSamplerState(Sampler);
				Device->SetBlendState(Blend);
				Device->SetRasterizerState(BackRasterizer);
				Device->SetShader(Shaders.GBuffer, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);
				System->GetScene()->SetSurface();

				for (auto It = SoftBodies->Begin(); It != SoftBodies->End(); ++It)
				{
					Engine::Components::SoftBody* SoftBody = (Engine::Components::SoftBody*)*It;
					if (!SoftBody->IsVisibleTo(System->GetScene()->View, nullptr) || SoftBody->GetIndices().empty())
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
			}
			void SoftBodyRenderer::OnDepthRender(Rest::Timer* Time)
			{
				Graphics::GraphicsDevice* Device = System->GetDevice();
				if (!SoftBodies || SoftBodies->Empty())
					return;

				Device->SetDepthStencilState(DepthStencil);
				Device->SetSamplerState(Sampler);
				Device->SetBlendState(Blend);
				Device->SetRasterizerState(FrontRasterizer);
				Device->SetShader(Shaders.Depth90, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);

				for (auto It = SoftBodies->Begin(); It != SoftBodies->End(); ++It)
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
			}
			void SoftBodyRenderer::OnCubicDepthRender(Rest::Timer* Time, Compute::Matrix4x4* ViewProjection)
			{
				Graphics::GraphicsDevice* Device = System->GetDevice();
				if (!SoftBodies || SoftBodies->Empty())
					return;

				memcpy(Depth360.FaceViewProjection, ViewProjection, sizeof(Compute::Matrix4x4) * 6);
				Device->SetDepthStencilState(DepthStencil);
				Device->SetSamplerState(Sampler);
				Device->SetBlendState(Blend);
				Device->SetRasterizerState(FrontRasterizer);
				Device->SetShader(Shaders.Depth360, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel | Graphics::ShaderType_Geometry);
				Device->SetBuffer(Shaders.Depth360, 3, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel | Graphics::ShaderType_Geometry);
				Device->UpdateBuffer(Shaders.Depth360, &Depth360);

				for (auto It = SoftBodies->Begin(); It != SoftBodies->End(); ++It)
				{
					Engine::Components::SoftBody* SoftBody = (Engine::Components::SoftBody*)*It;
					if (!SoftBody->GetBody() || SoftBody->GetEntity()->Transform->Position.Distance(System->GetScene()->View.RawPosition) >= System->GetScene()->View.ViewDistance + SoftBody->GetEntity()->Transform->Scale.Length())
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

				Device->SetShader(nullptr, Graphics::ShaderType_Geometry);
			}

			ElementSystemRenderer::ElementSystemRenderer(RenderSystem* Lab) : Renderer(Lab)
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

				if (System->GetDevice()->GetSection("geometry/element-system/gbuffer", &I.Data))
					Shaders.GBuffer = System->CompileShader("ESR_GBUFFER", I, 0);

				if (System->GetDevice()->GetSection("geometry/element-system/depth", &I.Data))
					Shaders.Depth90 = System->CompileShader("ESR_DEPTH90", I, 0);

				if (System->GetDevice()->GetSection("geometry/element-system/depth-360-point", &I.Data))
					Shaders.Depth360P = System->CompileShader("ESR_DEPTH360P", I, 0);

				if (System->GetDevice()->GetSection("geometry/element-system/depth-360-quad", &I.Data))
					Shaders.Depth360Q = System->CompileShader("ESR_DEPTH360Q", I, sizeof(Depth360));
			}
			ElementSystemRenderer::~ElementSystemRenderer()
			{
				System->FreeShader("ESR_GBUFFER", Shaders.GBuffer);
				System->FreeShader("ESR_DEPTH90", Shaders.Depth90);
				System->FreeShader("ESR_DEPTH360P", Shaders.Depth360P);
				System->FreeShader("ESR_DEPTH360Q", Shaders.Depth360Q);
			}
			void ElementSystemRenderer::OnInitialize()
			{
				ElementSystems = System->GetScene()->GetComponents<Engine::Components::ElementSystem>();
			}
			void ElementSystemRenderer::OnCulling(const Viewer& View)
			{
				float Hardness = 0.0f;
				for (auto It = ElementSystems->Begin(); It != ElementSystems->End(); ++It)
				{
					Engine::Components::ElementSystem* ElementSystem = (Engine::Components::ElementSystem*)*It;
					Entity* Base = ElementSystem->GetEntity();

					Hardness = 1.0f - Base->Transform->Position.Distance(View.RawPosition) / (View.ViewDistance + ElementSystem->Volume);
					if (Hardness > 0.0f)
						ElementSystem->Visibility = Compute::MathCommon::IsClipping(View.ViewProjection, Base->Transform->GetWorld(), 1.5f) == -1 ? Hardness : 0.0f;
					else
						ElementSystem->Visibility = 0.0f;
				}
			}
			void ElementSystemRenderer::OnRender(Rest::Timer* Time)
			{
				Graphics::GraphicsDevice* Device = System->GetDevice();
				if (!ElementSystems || ElementSystems->Empty())
					return;

				Graphics::PrimitiveTopology T = Device->GetPrimitiveTopology();
				Device->SetDepthStencilState(DepthStencil);
				Device->SetSamplerState(Sampler);
				Device->SetBlendState(AdditiveBlend);
				Device->SetRasterizerState(BackRasterizer);
				Device->SetPrimitiveTopology(Graphics::PrimitiveTopology_Point_List);
				Device->SetShader(Shaders.GBuffer, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);
				Device->SetVertexBuffer(nullptr, 0, 0, 0);
				System->GetScene()->SetSurface();

				Compute::Matrix4x4 View = System->GetScene()->View.ViewProjection * System->GetScene()->View.Projection.Invert();
				for (auto It = ElementSystems->Begin(); It != ElementSystems->End(); ++It)
				{
					Engine::Components::ElementSystem* ElementSystem = (Engine::Components::ElementSystem*)*It;
					if (!ElementSystem->GetBuffer())
						continue;

					if (!Appearance::UploadPhase(Device, ElementSystem->GetSurface()))
						continue;

					Device->Render.World = System->GetScene()->View.Projection;
					Device->Render.WorldViewProjection = (ElementSystem->QuadBased ? View : System->GetScene()->View.ViewProjection);
					
					if (ElementSystem->Connected)
						Device->Render.WorldViewProjection = ElementSystem->GetEntity()->Transform->GetWorld() * Device->Render.WorldViewProjection;

					Device->UpdateBuffer(ElementSystem->GetBuffer());
					Device->SetBuffer(ElementSystem->GetBuffer(), 7);
					Device->SetShader(ElementSystem->QuadBased ? Shaders.GBuffer : nullptr, Graphics::ShaderType_Geometry);
					Device->UpdateBuffer(Graphics::RenderBufferType_Render);
					Device->Draw((unsigned int)ElementSystem->GetBuffer()->GetArray()->Size(), 0);
				}

				Device->SetPrimitiveTopology(T);
				Device->SetShader(nullptr, Graphics::ShaderType_Geometry);
			}
			void ElementSystemRenderer::OnPhaseRender(Rest::Timer* Time)
			{
				Graphics::GraphicsDevice* Device = System->GetDevice();
				if (!ElementSystems || ElementSystems->Empty())
					return;

				Graphics::PrimitiveTopology T = Device->GetPrimitiveTopology();
				Device->SetDepthStencilState(DepthStencil);
				Device->SetSamplerState(Sampler);
				Device->SetBlendState(AdditiveBlend);
				Device->SetRasterizerState(BackRasterizer);
				Device->SetPrimitiveTopology(Graphics::PrimitiveTopology_Point_List);
				Device->SetShader(Shaders.GBuffer, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);
				Device->SetVertexBuffer(nullptr, 0, 0, 0);
				System->GetScene()->SetSurface();

				Compute::Matrix4x4 View = System->GetScene()->View.ViewProjection * System->GetScene()->View.Projection.Invert();
				for (auto It = ElementSystems->Begin(); It != ElementSystems->End(); ++It)
				{
					Engine::Components::ElementSystem* ElementSystem = (Engine::Components::ElementSystem*)*It;
					if (!ElementSystem->GetBuffer())
						continue;

					if (!Appearance::UploadPhase(Device, ElementSystem->GetSurface()))
						continue;

					Device->Render.World = System->GetScene()->View.Projection;
					Device->Render.WorldViewProjection = (ElementSystem->QuadBased ? View : System->GetScene()->View.ViewProjection);
					
					if (ElementSystem->Connected)
						Device->Render.WorldViewProjection = ElementSystem->GetEntity()->Transform->GetWorld() * Device->Render.WorldViewProjection;

					Device->UpdateBuffer(ElementSystem->GetBuffer());
					Device->SetBuffer(ElementSystem->GetBuffer(), 7);
					Device->SetShader(ElementSystem->QuadBased ? Shaders.GBuffer : nullptr, Graphics::ShaderType_Geometry);
					Device->UpdateBuffer(Graphics::RenderBufferType_Render);
					Device->Draw((unsigned int)ElementSystem->GetBuffer()->GetArray()->Size(), 0);
				}

				Device->SetPrimitiveTopology(T);
				Device->SetShader(nullptr, Graphics::ShaderType_Geometry);
			}
			void ElementSystemRenderer::OnDepthRender(Rest::Timer* Time)
			{
				Graphics::GraphicsDevice* Device = System->GetDevice();
				if (!ElementSystems || ElementSystems->Empty())
					return;

				Graphics::PrimitiveTopology T = Device->GetPrimitiveTopology();
				Device->SetDepthStencilState(DepthStencil);
				Device->SetSamplerState(Sampler);
				Device->SetBlendState(OverwriteBlend);
				Device->SetRasterizerState(FrontRasterizer);
				Device->SetPrimitiveTopology(Graphics::PrimitiveTopology_Point_List);
				Device->SetShader(Shaders.Depth90, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);
				Device->SetVertexBuffer(nullptr, 0, 0, 0);
				System->GetScene()->SetSurface();

				Compute::Matrix4x4 View = System->GetScene()->View.ViewProjection * System->GetScene()->View.Projection.Invert();
				for (auto It = ElementSystems->Begin(); It != ElementSystems->End(); ++It)
				{
					Engine::Components::ElementSystem* ElementSystem = (Engine::Components::ElementSystem*)*It;
					if (!ElementSystem->GetBuffer())
						continue;

					if (!Appearance::UploadDepth(Device, ElementSystem->GetSurface()))
						continue;

					Device->Render.World = ElementSystem->QuadBased ? System->GetScene()->View.Projection : Compute::Matrix4x4::Identity();
					Device->Render.WorldViewProjection = (ElementSystem->QuadBased ? View : System->GetScene()->View.ViewProjection);
					
					if (ElementSystem->Connected)
						Device->Render.WorldViewProjection = ElementSystem->GetEntity()->Transform->GetWorld() * Device->Render.WorldViewProjection;

					Device->UpdateBuffer(ElementSystem->GetBuffer());
					Device->SetBuffer(ElementSystem->GetBuffer(), 7);
					Device->SetShader(ElementSystem->QuadBased ? Shaders.Depth90 : nullptr, Graphics::ShaderType_Geometry);
					Device->UpdateBuffer(Graphics::RenderBufferType_Render);
					Device->Draw((unsigned int)ElementSystem->GetBuffer()->GetArray()->Size(), 0);
				}

				Device->SetPrimitiveTopology(T);
				Device->SetShader(nullptr, Graphics::ShaderType_Geometry);
			}
			void ElementSystemRenderer::OnCubicDepthRender(Rest::Timer* Time, Compute::Matrix4x4* ViewProjection)
			{
				Graphics::GraphicsDevice* Device = System->GetDevice();
				if (!ElementSystems || ElementSystems->Empty())
					return;

				Depth360.FaceView[0] = Compute::Matrix4x4::CreateCubeMapLookAt(0, System->GetScene()->View.Position);
				Depth360.FaceView[1] = Compute::Matrix4x4::CreateCubeMapLookAt(1, System->GetScene()->View.Position);
				Depth360.FaceView[2] = Compute::Matrix4x4::CreateCubeMapLookAt(2, System->GetScene()->View.Position);
				Depth360.FaceView[3] = Compute::Matrix4x4::CreateCubeMapLookAt(3, System->GetScene()->View.Position);
				Depth360.FaceView[4] = Compute::Matrix4x4::CreateCubeMapLookAt(4, System->GetScene()->View.Position);
				Depth360.FaceView[5] = Compute::Matrix4x4::CreateCubeMapLookAt(5, System->GetScene()->View.Position);

				Graphics::PrimitiveTopology T = Device->GetPrimitiveTopology();
				Device->SetDepthStencilState(DepthStencil);
				Device->SetSamplerState(Sampler);
				Device->SetBlendState(OverwriteBlend);
				Device->SetRasterizerState(FrontRasterizer);
				Device->SetPrimitiveTopology(Graphics::PrimitiveTopology_Point_List);
				Device->SetVertexBuffer(nullptr, 0, 0, 0);
				Device->SetBuffer(Shaders.Depth360Q, 3, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel | Graphics::ShaderType_Geometry);
				Device->UpdateBuffer(Shaders.Depth360Q, &Depth360);
				System->GetScene()->SetSurface();

				for (auto It = ElementSystems->Begin(); It != ElementSystems->End(); ++It)
				{
					Engine::Components::ElementSystem* ElementSystem = (Engine::Components::ElementSystem*)*It;
					if (!ElementSystem->GetBuffer())
						continue;

					if (!Appearance::UploadCubicDepth(Device, ElementSystem->GetSurface()))
						continue;

					Device->Render.World = (ElementSystem->Connected ? ElementSystem->GetEntity()->Transform->GetWorld() : Compute::Matrix4x4::Identity());			
					Device->UpdateBuffer(ElementSystem->GetBuffer());
					Device->SetBuffer(ElementSystem->GetBuffer(), 7);
					Device->SetShader(ElementSystem->QuadBased ? Shaders.Depth360Q : Shaders.Depth360P, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel | Graphics::ShaderType_Geometry);
					Device->UpdateBuffer(Graphics::RenderBufferType_Render);
					Device->Draw((unsigned int)ElementSystem->GetBuffer()->GetArray()->Size(), 0);
				}

				Device->SetPrimitiveTopology(T);
				Device->SetShader(nullptr, Graphics::ShaderType_Geometry);
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
			void DepthRenderer::OnLoad(ContentManager* Content, Rest::Document* Node)
			{
				NMake::Unpack(Node->Find("point-light-resolution"), &Renderers.PointLightResolution);
				NMake::Unpack(Node->Find("point-light-limits"), &Renderers.PointLightLimits);
				NMake::Unpack(Node->Find("spot-light-resolution"), &Renderers.SpotLightResolution);
				NMake::Unpack(Node->Find("spot-light-limits"), &Renderers.SpotLightLimits);
				NMake::Unpack(Node->Find("line-light-resolution"), &Renderers.LineLightResolution);
				NMake::Unpack(Node->Find("line-light-limits"), &Renderers.LineLightLimits);
				NMake::Unpack(Node->Find("shadow-distance"), &ShadowDistance);
			}
			void DepthRenderer::OnSave(ContentManager* Content, Rest::Document* Node)
			{
				NMake::Pack(Node->SetDocument("point-light-resolution"), Renderers.PointLightResolution);
				NMake::Pack(Node->SetDocument("point-light-limits"), Renderers.PointLightLimits);
				NMake::Pack(Node->SetDocument("spot-light-resolution"), Renderers.SpotLightResolution);
				NMake::Pack(Node->SetDocument("spot-light-limits"), Renderers.SpotLightLimits);
				NMake::Pack(Node->SetDocument("line-light-resolution"), Renderers.LineLightResolution);
				NMake::Pack(Node->SetDocument("line-light-limits"), Renderers.LineLightLimits);
				NMake::Pack(Node->SetDocument("shadow-distance"), ShadowDistance);
			}
			void DepthRenderer::OnInitialize()
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
					Graphics::RenderTargetCube::Desc I = Graphics::RenderTargetCube::Desc();
					I.Size = (unsigned int)Renderers.PointLightResolution;
					I.FormatMode = Graphics::Format_R32G32_Float;

					*It = System->GetDevice()->CreateRenderTargetCube(I);
				}

				Lights = System->GetScene()->GetComponents<Engine::Components::SpotLight>();
				for (auto It = Lights->Begin(); It != Lights->End(); It++)
					(*It)->As<Engine::Components::SpotLight>()->SetShadowCache(nullptr);

				for (auto It = Renderers.SpotLight.begin(); It != Renderers.SpotLight.end(); It++)
					delete *It;

				Renderers.SpotLight.resize(Renderers.SpotLightLimits);
				for (auto It = Renderers.SpotLight.begin(); It != Renderers.SpotLight.end(); It++)
				{
					Graphics::RenderTarget2D::Desc I = Graphics::RenderTarget2D::Desc();
					I.Width = (unsigned int)Renderers.SpotLightResolution;
					I.Height = (unsigned int)Renderers.SpotLightResolution;
					I.FormatMode = Graphics::Format_R32G32_Float;

					*It = System->GetDevice()->CreateRenderTarget2D(I);
				}

				Lights = System->GetScene()->GetComponents<Engine::Components::LineLight>();
				for (auto It = Lights->Begin(); It != Lights->End(); It++)
					(*It)->As<Engine::Components::LineLight>()->SetShadowCache(nullptr);

				for (auto It = Renderers.LineLight.begin(); It != Renderers.LineLight.end(); It++)
					delete *It;

				Renderers.LineLight.resize(Renderers.LineLightLimits);
				for (auto It = Renderers.LineLight.begin(); It != Renderers.LineLight.end(); It++)
				{
					Graphics::RenderTarget2D::Desc I = Graphics::RenderTarget2D::Desc();
					I.Width = (unsigned int)Renderers.LineLightResolution;
					I.Height = (unsigned int)Renderers.LineLightResolution;
					I.FormatMode = Graphics::Format_R32G32_Float;

					*It = System->GetDevice()->CreateRenderTarget2D(I);
				}
			}
			void DepthRenderer::OnIntervalRender(Rest::Timer* Time)
			{
				Graphics::GraphicsDevice* Device = System->GetDevice();

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

					RenderCubicDepth(Time, Light->Projection, Light->GetEntity()->Transform->Position.XYZW().SetW(Light->ShadowDistance));
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

					RenderDepth(Time, Light->View, Light->Projection, Light->GetEntity()->Transform->Position.XYZW().SetW(Light->ShadowDistance));
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

					RenderDepth(Time, Light->View, Light->Projection, Compute::Vector4(0, 0, 0, -1));
					Light->SetShadowCache(Target->GetTarget()); Shadows++;
				}

				System->GetScene()->RestoreViewBuffer(nullptr);
			}

			ProbeRenderer::ProbeRenderer(RenderSystem* Lab) : Renderer(Lab), Surface(nullptr), Size(128), MipLevels(7)
			{
				Geometric = false;
			}
			ProbeRenderer::~ProbeRenderer()
			{
				delete Surface;
			}
			void ProbeRenderer::OnLoad(ContentManager* Content, Rest::Document* Node)
			{
				NMake::Unpack(Node->Find("size"), &Size);
				NMake::Unpack(Node->Find("mip-levels"), &MipLevels);
			}
			void ProbeRenderer::OnSave(ContentManager* Content, Rest::Document* Node)
			{
				NMake::Pack(Node->SetDocument("size"), Size);
				NMake::Pack(Node->SetDocument("mip-levels"), MipLevels);
			}
			void ProbeRenderer::OnInitialize()
			{
				CreateRenderTarget();
				ProbeLights = System->GetScene()->GetComponents<Engine::Components::ProbeLight>();
			}
			void ProbeRenderer::OnRender(Rest::Timer* Time)
			{
				Graphics::GraphicsDevice* Device = System->GetDevice();
				Graphics::MultiRenderTarget2D* S = System->GetScene()->GetSurface();
				System->GetScene()->SetSurface(Surface);

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
					else if (!Light->Rebuild.OnTickEvent(ElapsedTime) || Light->Rebuild.Delay <= 0.0)
						continue;

					Device->CopyBegin(Surface, 0, MipLevels, Size);
					Light->RenderLocked = true;

					Compute::Vector3 Position = Light->GetEntity()->Transform->Position * Light->ViewOffset;
					for (int j = 0; j < 6; j++)
					{
						Light->View[j] = Compute::Matrix4x4::CreateCubeMapLookAt(j, Position);
						RenderPhase(Time, Light->View[j], Light->Projection, Position.XYZW().SetW(Light->CaptureRange));
						Device->CopyFace(Surface, 0, j);
					}

					Light->RenderLocked = false;
					Device->CopyEnd(Surface, Cache);
				}

				System->GetScene()->SetSurface(S);
				System->GetScene()->RestoreViewBuffer(nullptr);
			}
			void ProbeRenderer::CreateRenderTarget()
			{
				if (Map == Size)
					return;

				Map = Size;

				Graphics::MultiRenderTarget2D::Desc F1 = Graphics::MultiRenderTarget2D::Desc();
				F1.FormatMode[0] = Graphics::Format_R8G8B8A8_Unorm;
				F1.FormatMode[1] = Graphics::Format_R16G16B16A16_Float;
				F1.FormatMode[2] = Graphics::Format_R32G32_Float;
				F1.FormatMode[3] = Graphics::Format_Invalid;
				F1.FormatMode[4] = Graphics::Format_Invalid;
				F1.FormatMode[5] = Graphics::Format_Invalid;
				F1.FormatMode[6] = Graphics::Format_Invalid;
				F1.FormatMode[7] = Graphics::Format_Invalid;
				F1.SVTarget = Graphics::SurfaceTarget2;
				F1.MiscFlags = Graphics::ResourceMisc_Generate_Mips;
				F1.MipLevels = System->GetDevice()->GetMipLevel((unsigned int)Size, (unsigned int)Size);
				F1.Width = (unsigned int)Size;
				F1.Height = (unsigned int)Size;

				if (MipLevels > F1.MipLevels)
					MipLevels = F1.MipLevels;
				else
					F1.MipLevels = (unsigned int)MipLevels;

				delete Surface;
				Surface = System->GetDevice()->CreateMultiRenderTarget2D(F1);
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
					Shaders.PointShade = System->CompileShader("LR_POINT_SHADE", I, 0);

				if (System->GetDevice()->GetSection("pass/light/shade/spot", &I.Data))
					Shaders.SpotShade = System->CompileShader("LR_SPOT_SHADE", I, 0);

				if (System->GetDevice()->GetSection("pass/light/shade/line", &I.Data))
					Shaders.LineShade = System->CompileShader("LR_LINE_SHADE", I, 0);

				if (System->GetDevice()->GetSection("pass/light/base/point", &I.Data))
					Shaders.PointBase = System->CompileShader("LR_POINT_BASE", I, sizeof(PointLight));

				if (System->GetDevice()->GetSection("pass/light/base/spot", &I.Data))
					Shaders.SpotBase = System->CompileShader("LR_SPOT_BASE", I, sizeof(SpotLight));

				if (System->GetDevice()->GetSection("pass/light/base/line", &I.Data))
					Shaders.LineBase = System->CompileShader("LR_LINE_BASE", I, sizeof(LineLight));

				if (System->GetDevice()->GetSection("pass/light/base/probe", &I.Data))
					Shaders.Probe = System->CompileShader("LR_PROBE", I, sizeof(ProbeLight));

				if (System->GetDevice()->GetSection("pass/light/base/ambient", &I.Data))
					Shaders.Ambient = System->CompileShader("LR_AMBIENT", I, sizeof(AmbientLight));
			}
			LightRenderer::~LightRenderer()
			{
				System->FreeShader("LR_POINT_BASE", Shaders.PointBase);
				System->FreeShader("LR_POINT_SHADE", Shaders.PointShade);
				System->FreeShader("LR_SPOT_BASE", Shaders.SpotBase);
				System->FreeShader("LR_SPOT_SHADE", Shaders.SpotShade);
				System->FreeShader("LR_LINE_BASE", Shaders.LineBase);
				System->FreeShader("LR_LINE_SHADE", Shaders.LineShade);
				System->FreeShader("LR_PROBE", Shaders.Probe);
				System->FreeShader("LR_AMBIENT", Shaders.Ambient);
				delete Input1;
				delete Output1;
				delete Input2;
				delete Output2;
				delete SkyMap;
			}
			void LightRenderer::OnLoad(ContentManager* Content, Rest::Document* Node)
			{
				std::string Path;
				if (NMake::Unpack(Node->Find("sky-map"), &Path))
					SetSkyMap(Content->Load<Graphics::Texture2D>(Path, nullptr));

				NMake::Unpack(Node->Find("high-emission"), &AmbientLight.HighEmission);
				NMake::Unpack(Node->Find("low-emission"), &AmbientLight.LowEmission);
				NMake::Unpack(Node->Find("sky-emission"), &AmbientLight.SkyEmission);
				NMake::Unpack(Node->Find("recursive-probes"), &RecursiveProbes);
			}
			void LightRenderer::OnSave(ContentManager* Content, Rest::Document* Node)
			{
				AssetResource* Asset = Content->FindAsset(SkyBase);
				if (Asset != nullptr)
					NMake::Pack(Node->SetDocument("sky-map"), Asset->Path);

				NMake::Pack(Node->SetDocument("high-emission"), AmbientLight.HighEmission);
				NMake::Pack(Node->SetDocument("low-emission"), AmbientLight.LowEmission);
				NMake::Pack(Node->SetDocument("sky-emission"), AmbientLight.SkyEmission);
				NMake::Pack(Node->SetDocument("recursive-probes"), RecursiveProbes);
			}
			void LightRenderer::OnInitialize()
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

				CreateRenderTargets();
			}
			void LightRenderer::OnCulling(const Viewer& View)
			{
				float Hardness = 0.0f;
				for (auto It = PointLights->Begin(); It != PointLights->End(); ++It)
				{
					Engine::Components::PointLight* PointLight = (Engine::Components::PointLight*)*It;
					Entity* Base = PointLight->GetEntity();

					Hardness = 1.0f - Base->Transform->Position.Distance(View.RawPosition) / View.ViewDistance;
					if (Hardness > 0.0f)
						PointLight->Visibility = Compute::MathCommon::IsClipping(View.ViewProjection, Base->Transform->GetWorld(), PointLight->Range) == -1 ? Hardness : 0.0f;
					else
						PointLight->Visibility = 0.0f;
				}

				for (auto It = SpotLights->Begin(); It != SpotLights->End(); ++It)
				{
					Engine::Components::SpotLight* SpotLight = (Engine::Components::SpotLight*)*It;
					Entity* Base = SpotLight->GetEntity();

					Hardness = 1.0f - Base->Transform->Position.Distance(View.RawPosition) / View.ViewDistance;
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
						Hardness = 1.0f - Base->Transform->Position.Distance(View.RawPosition) / View.ViewDistance;
						if (Hardness > 0.0f)
							ProbeLight->Visibility = Compute::MathCommon::IsClipping(View.ViewProjection, Base->Transform->GetWorld(), ProbeLight->Range) == -1 ? Hardness : 0.0f;
						else
							ProbeLight->Visibility = 0.0f;
					}
					else
						ProbeLight->Visibility = 1.0f;
				}
			}
			void LightRenderer::OnRender(Rest::Timer* Time)
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
				Device->SetTexture2D(Surface->GetTarget(1), 1);
				Device->SetTexture2D(Surface->GetTarget(2), 2);
				Device->SetTexture2D(Input1->GetTarget(), 3);
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

					Device->SetTextureCube(Light->GetProbeCache(), 4);
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
						Device->SetTexture2D(Light->GetShadowCache(), 4);
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
						Device->SetTexture2D(Light->GetShadowCache(), 5);
					}
					else
						Active = Shaders.SpotBase;

					SpotLight.OwnWorldViewProjection = Compute::Matrix4x4::CreateScale(Light->Range * 1.25f) * Compute::Matrix4x4::CreateTranslation(Light->GetEntity()->Transform->Position) * System->GetScene()->View.ViewProjection;
					SpotLight.OwnViewProjection = Light->View * Light->Projection;
					SpotLight.Position = Light->GetEntity()->Transform->Position.InvertZ();
					SpotLight.Lighting = Light->Diffuse.Mul(Light->Emission * Light->Visibility);
					SpotLight.Range = Light->Range;
					SpotLight.Diffuse = (Light->ProjectMap != nullptr);

					Device->SetTexture2D(Light->ProjectMap, 4);
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
						Device->SetTexture2D(Light->GetShadowCache(), 4);
					}
					else
						Active = Shaders.LineBase;

					LineLight.Position = Light->GetEntity()->Transform->Position.InvertZ().NormalizeSafe();
					LineLight.Lighting = Light->Diffuse.Mul(Light->Emission);

					Device->SetShader(Active, Graphics::ShaderType_Pixel);
					Device->UpdateBuffer(Shaders.LineBase, &LineLight);
					Device->Draw(6, 0);
				}

				AmbientLight.SkyOffset = System->GetScene()->View.Projection.Invert() * Compute::Matrix4x4::CreateRotation(System->GetScene()->View.Rotation);
				Device->SetTarget(Surface, 0, 0, 0, 0);
				Device->ClearDepth(Surface);
				Device->SetTexture2D(Output1->GetTarget(), 4);
				Device->SetTextureCube(SkyMap, 5);
				Device->SetShader(Shaders.Ambient, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);
				Device->SetBuffer(Shaders.Ambient, 3, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);
				Device->UpdateBuffer(Shaders.Ambient, &AmbientLight);
				Device->Draw(6, 0);
				Device->FlushTexture2D(1, 5);
			}
			void LightRenderer::OnPhaseRender(Rest::Timer* Time)
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
				Device->SetTexture2D(Surface->GetTarget(1), 1);
				Device->SetTexture2D(Surface->GetTarget(2), 2);
				Device->SetTexture2D(Input2->GetTarget(), 3);
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
						if (Light->Visibility <= 0 && (Visibility = 1.0f - Light->GetEntity()->Transform->Position.Distance(View.Position) / View.ViewDistance) <= 0.0f)
							continue;

						ProbeLight.OwnWorldViewProjection = Compute::Matrix4x4::CreateTranslatedScale(Light->GetEntity()->Transform->Position, Light->Range * 1.25f) * System->GetScene()->View.ViewProjection;
						ProbeLight.Position = Light->GetEntity()->Transform->Position.InvertZ();
						ProbeLight.Lighting = Light->Diffuse.Mul(Light->Emission * Visibility);
						ProbeLight.Scale = Light->GetEntity()->Transform->Scale;
						ProbeLight.Parallax = (Light->ParallaxCorrected ? 1.0f : 0.0f);
						ProbeLight.Range = Light->Range;
						ProbeLight.Infinity = Light->Infinity;

						Device->SetTextureCube(Light->GetProbeCache(), 4);
						Device->UpdateBuffer(Shaders.Probe, &ProbeLight);
						Device->DrawIndexed((unsigned int)System->GetSphereIBuffer()->GetElements(), 0, 0);
					}
				}

				Device->SetShader(Shaders.PointBase, Graphics::ShaderType_Vertex);
				Device->SetBuffer(Shaders.PointBase, 3, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);

				for (auto It = PointLights->Begin(); It != PointLights->End(); ++It)
				{
					Engine::Components::PointLight* Light = (Engine::Components::PointLight*)*It;

					float Visibility = 1.0f - Light->GetEntity()->Transform->Position.Distance(System->GetScene()->View.RawPosition) / System->GetScene()->View.ViewDistance;
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
						Device->SetTexture2D(Light->GetShadowCache(), 4);
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

					float Visibility = 1.0f - Light->GetEntity()->Transform->Position.Distance(System->GetScene()->View.RawPosition) / System->GetScene()->View.ViewDistance;
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
						Device->SetTexture2D(Light->GetShadowCache(), 5);
					}
					else
						Active = Shaders.SpotBase;

					SpotLight.OwnWorldViewProjection = Compute::Matrix4x4::CreateScale(Light->Range * 1.25f) * Compute::Matrix4x4::CreateTranslation(Light->GetEntity()->Transform->Position) * System->GetScene()->View.ViewProjection;
					SpotLight.OwnViewProjection = Light->View * Light->Projection;
					SpotLight.Position = Light->GetEntity()->Transform->Position.InvertZ();
					SpotLight.Lighting = Light->Diffuse.Mul(Light->Emission * Light->Visibility);
					SpotLight.Range = Light->Range;
					SpotLight.Diffuse = (Light->ProjectMap != nullptr);

					Device->SetTexture2D(Light->ProjectMap, 4);
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
						Device->SetTexture2D(Light->GetShadowCache(), 4);
					}
					else
						Active = Shaders.LineBase;

					LineLight.Position = Light->GetEntity()->Transform->Position.InvertZ().NormalizeSafe();
					LineLight.Lighting = Light->Diffuse.Mul(Light->Emission);

					Device->SetShader(Active, Graphics::ShaderType_Pixel);
					Device->UpdateBuffer(Shaders.LineBase, &LineLight);
					Device->Draw(6, 0);
				}

				AmbientLight.SkyOffset = System->GetScene()->View.Projection.Invert() * Compute::Matrix4x4::CreateRotation(System->GetScene()->View.Rotation);
				Device->SetTarget(Surface, 0, 0, 0, 0);
				Device->ClearDepth(Surface);
				Device->SetTexture2D(Output2->GetTarget(), 4);
				Device->SetTextureCube(SkyMap, 5);
				Device->SetShader(Shaders.Ambient, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);
				Device->SetBuffer(Shaders.Ambient, 3, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);
				Device->UpdateBuffer(Shaders.Ambient, &AmbientLight);
				Device->Draw(6, 0);
				Device->FlushTexture2D(1, 5);
			}
			void LightRenderer::OnResizeBuffers()
			{
				CreateRenderTargets();
			}
			void LightRenderer::CreateRenderTargets()
			{
				Graphics::RenderTarget2D::Desc I = Graphics::RenderTarget2D::Desc();
				I.FormatMode = Graphics::Format_R8G8B8A8_Unorm;
				I.MiscFlags = Graphics::ResourceMisc_Generate_Mips;
				I.Width = (unsigned int)System->GetScene()->GetSurface()->GetWidth();
				I.Height = (unsigned int)System->GetScene()->GetSurface()->GetHeight();
				I.MipLevels = System->GetDevice()->GetMipLevel(I.Width, I.Height);

				delete Output1;
				Output1 = System->GetDevice()->CreateRenderTarget2D(I);

				delete Input1;
				Input1 = System->GetDevice()->CreateRenderTarget2D(I);

				auto* RenderStages = System->GetRenderers();
				for (auto It = RenderStages->begin(); It != RenderStages->end(); It++)
				{
					if ((*It)->Id() != THAWK_COMPONENT_ID(ProbeRenderer))
						continue;

					Engine::Renderers::ProbeRenderer* ProbeRenderer = (*It)->As<Engine::Renderers::ProbeRenderer>();
					I.Width = (unsigned int)ProbeRenderer->Size;
					I.Height = (unsigned int)ProbeRenderer->Size;
					I.MipLevels = (unsigned int)ProbeRenderer->MipLevels;
					break;
				}

				delete Output2;
				Output2 = System->GetDevice()->CreateRenderTarget2D(I);

				delete Input2;
				Input2 = System->GetDevice()->CreateRenderTarget2D(I);
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
			void ImageRenderer::OnLoad(ContentManager* Content, Rest::Document* Node)
			{
			}
			void ImageRenderer::OnSave(ContentManager* Content, Rest::Document* Node)
			{
			}
			void ImageRenderer::OnRender(Rest::Timer* Time)
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
					Pass1 = CompileEffect("RR_REFLECTION", Data, sizeof(RenderPass1));

				if (System->GetDevice()->GetSection("pass/gloss", &Data))
					Pass2 = CompileEffect("RR_GLOSS", Data, sizeof(RenderPass2));
			}
			void ReflectionsRenderer::OnLoad(ContentManager* Content, Rest::Document* Node)
			{
				NMake::Unpack(Node->Find("iteration-count-1"), &RenderPass1.IterationCount);
				NMake::Unpack(Node->Find("iteration-count-2"), &RenderPass2.IterationCount);
				NMake::Unpack(Node->Find("intensity"), &RenderPass1.Intensity);
				NMake::Unpack(Node->Find("blur"), &RenderPass2.Blur);
			}
			void ReflectionsRenderer::OnSave(ContentManager* Content, Rest::Document* Node)
			{
				NMake::Pack(Node->SetDocument("iteration-count-1"), RenderPass1.IterationCount);
				NMake::Pack(Node->SetDocument("iteration-count-2"), RenderPass2.IterationCount);
				NMake::Pack(Node->SetDocument("intensity"), RenderPass1.Intensity);
				NMake::Pack(Node->SetDocument("blur"), RenderPass2.Blur);
			}
			void ReflectionsRenderer::OnRenderEffect(Rest::Timer* Time)
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
					CompileEffect("DOFR_FOCUS", Data, sizeof(RenderPass));
			}
			void DepthOfFieldRenderer::OnLoad(ContentManager* Content, Rest::Document* Node)
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
			void DepthOfFieldRenderer::OnSave(ContentManager* Content, Rest::Document* Node)
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
			void DepthOfFieldRenderer::OnRenderEffect(Rest::Timer* Time)
			{
				RenderPass.Texel[0] = 1.0f / Output->GetWidth();
				RenderPass.Texel[1] = 1.0f / Output->GetHeight();
				RenderResult(nullptr, &RenderPass);
			}
			
			EmissionRenderer::EmissionRenderer(RenderSystem* Lab) : PostProcessRenderer(Lab)
			{
				std::string Data;
				if (System->GetDevice()->GetSection("pass/bloom", &Data))
					CompileEffect("ER_BLOOM", Data, sizeof(RenderPass));
			}
			void EmissionRenderer::OnLoad(ContentManager* Content, Rest::Document* Node)
			{
				NMake::Unpack(Node->Find("iteration-count"), &RenderPass.IterationCount);
				NMake::Unpack(Node->Find("intensity"), &RenderPass.Intensity);
				NMake::Unpack(Node->Find("threshold"), &RenderPass.Threshold);
				NMake::Unpack(Node->Find("scale"), &RenderPass.Scale);
			}
			void EmissionRenderer::OnSave(ContentManager* Content, Rest::Document* Node)
			{
				NMake::Pack(Node->SetDocument("iteration-count"), RenderPass.IterationCount);
				NMake::Pack(Node->SetDocument("intensity"), RenderPass.Intensity);
				NMake::Pack(Node->SetDocument("threshold"), RenderPass.Threshold);
				NMake::Pack(Node->SetDocument("scale"), RenderPass.Scale);
			}
			void EmissionRenderer::OnRenderEffect(Rest::Timer* Time)
			{
				RenderPass.Texel[0] = 1.0f / Output->GetWidth();
				RenderPass.Texel[1] = 1.0f / Output->GetHeight();
				RenderResult(nullptr, &RenderPass);
			}
			
			GlitchRenderer::GlitchRenderer(RenderSystem* Lab) : PostProcessRenderer(Lab), ScanLineJitter(0), VerticalJump(0), HorizontalShake(0), ColorDrift(0)
			{
				std::string Data;
				if (System->GetDevice()->GetSection("pass/glitch", &Data))
					CompileEffect("GR_GLITCH", Data, sizeof(RenderPass));
			}
			void GlitchRenderer::OnLoad(ContentManager* Content, Rest::Document* Node)
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
			void GlitchRenderer::OnSave(ContentManager* Content, Rest::Document* Node)
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
			void GlitchRenderer::OnRenderEffect(Rest::Timer* Time)
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
					Pass1 = CompileEffect("AOR_AMBIENT", Data, sizeof(RenderPass1));

				if (System->GetDevice()->GetSection("pass/blur", &Data))
					Pass2 = CompileEffect("AOR_BLUR", Data, sizeof(RenderPass2));
			}
			void AmbientOcclusionRenderer::OnLoad(ContentManager* Content, Rest::Document* Node)
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
			void AmbientOcclusionRenderer::OnSave(ContentManager* Content, Rest::Document* Node)
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
			void AmbientOcclusionRenderer::OnRenderEffect(Rest::Timer* Time)
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
					Pass1 = CompileEffect("IOR_INDIRECT", Data, sizeof(RenderPass1));

				if (System->GetDevice()->GetSection("pass/blur", &Data))
					Pass2 = CompileEffect("AOR_BLUR", Data, sizeof(RenderPass2));
			}
			void DirectOcclusionRenderer::OnLoad(ContentManager* Content, Rest::Document* Node)
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
			void DirectOcclusionRenderer::OnSave(ContentManager* Content, Rest::Document* Node)
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
			void DirectOcclusionRenderer::OnRenderEffect(Rest::Timer* Time)
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
					CompileEffect("TR_TONE", Data, sizeof(RenderPass));
			}
			void ToneRenderer::OnLoad(ContentManager* Content, Rest::Document* Node)
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
			}
			void ToneRenderer::OnSave(ContentManager* Content, Rest::Document* Node)
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
			}
			void ToneRenderer::OnRenderEffect(Rest::Timer* Time)
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
			void GUIRenderer::OnRender(Rest::Timer* Timer)
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