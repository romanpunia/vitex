#include "renderers.h"
#include "components.h"

namespace Tomahawk
{
	namespace Engine
	{
		namespace Renderers
		{
			SoftBody::SoftBody(Engine::RenderSystem* Lab) : GeometryRenderer<Components::SoftBody>(Lab), VertexBuffer(nullptr), IndexBuffer(nullptr)
			{
				TH_ASSERT_V(System != nullptr, "render system should be set");
				TH_ASSERT_V(System->GetDevice() != nullptr, "graphics device should be set");

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
					IndexBuffer = Buffers[(size_t)BufferType::Index];
					VertexBuffer = Buffers[(size_t)BufferType::Vertex];
				}
			}
			SoftBody::~SoftBody()
			{
				Graphics::ElementBuffer* Buffers[2];
				Buffers[(size_t)BufferType::Index] = IndexBuffer;
				Buffers[(size_t)BufferType::Vertex] = VertexBuffer;

				System->FreeBuffers(Buffers);
				System->FreeShader(Shaders.Geometry);
				System->FreeShader(Shaders.Voxelize);
				System->FreeShader(Shaders.Occlusion);
				System->FreeShader(Shaders.Depth.Linear);
				System->FreeShader(Shaders.Depth.Cubic);
			}
			size_t SoftBody::CullGeometry(const Viewer& View, const std::vector<Components::SoftBody*>& Geometry, bool Dirty)
			{
				TH_ASSERT(System->GetPrimitives() != nullptr, 0, "primitives cache should be set");

				Graphics::ElementBuffer* Box[2];
				System->GetPrimitives()->GetBoxBuffers(Box);

				Graphics::GraphicsDevice* Device = System->GetDevice();
				Device->SetRasterizerState(Rasterizer);
				Device->SetInputLayout(Layout);
				Device->SetShader(nullptr, TH_PS);
				Device->SetShader(Shaders.Occlusion, TH_VS);

				size_t Count = 0;
				if (System->PreciseCulling)
				{
					for (auto* Base : Geometry)
					{
						if (!Base->WasVisible(Dirty) || Base->GetIndices().empty() || !Base->Query.Begin(Device))
							continue;

						if (Base->Query.GetPassed() > 0)
						{
							Base->Fill(Device, IndexBuffer, VertexBuffer);
							Device->Render.World.Identify();
							Device->Render.WorldViewProj = Device->Render.World * View.ViewProjection;
							Device->UpdateBuffer(Graphics::RenderBufferType::Render);
							Device->SetVertexBuffer(VertexBuffer, 0);
							Device->SetIndexBuffer(IndexBuffer, Graphics::Format::R32_Uint);
							Device->DrawIndexed((unsigned int)Base->GetIndices().size(), 0, 0);
						}
						else
						{
							Device->Render.World = Base->GetEntity()->GetBox();
							Device->Render.WorldViewProj = Device->Render.World * View.ViewProjection;
							Device->UpdateBuffer(Graphics::RenderBufferType::Render);
							Device->SetVertexBuffer(Box[(size_t)BufferType::Vertex], 0);
							Device->SetIndexBuffer(Box[(size_t)BufferType::Index], Graphics::Format::R32_Uint);
							Device->DrawIndexed(Box[(size_t)BufferType::Index]->GetElements(), 0, 0);
						}

						Base->Query.End(Device);
						Count++;
					}
				}
				else
				{
					Device->SetVertexBuffer(Box[(size_t)BufferType::Vertex], 0);
					Device->SetIndexBuffer(Box[(size_t)BufferType::Index], Graphics::Format::R32_Uint);

					for (auto* Base : Geometry)
					{
						if (Base->GetIndices().empty() || !Base->Query.Begin(Device))
							continue;

						Device->Render.World = Base->GetEntity()->GetBox();
						Device->Render.WorldViewProj = Device->Render.World * View.ViewProjection;
						Device->UpdateBuffer(Graphics::RenderBufferType::Render);
						Device->DrawIndexed(Box[(size_t)BufferType::Index]->GetElements(), 0, 0);
						Base->Query.End(Device);

						Count++;
					}
				}

				return Count;
			}
			size_t SoftBody::RenderGeometryResult(Core::Timer* Time, const std::vector<Components::SoftBody*>& Geometry, RenderOpt Options)
			{
				TH_ASSERT(System->GetScene() != nullptr, 0, "scene should be set");

				SceneGraph* Scene = System->GetScene();
				Graphics::GraphicsDevice* Device = System->GetDevice();
				bool Static = ((size_t)Options & (size_t)RenderOpt::Static);

				Device->SetDepthStencilState(DepthStencil);
				Device->SetBlendState(Blend);
				Device->SetRasterizerState(Rasterizer);
				Device->SetInputLayout(Layout);
				Device->SetSamplerState(Sampler, 1, 7, TH_PS);
				Device->SetShader(Shaders.Geometry, TH_VS | TH_PS);

				size_t Count = 0;
				for (auto* Base : Geometry)
				{
					if ((Static && !Base->Static) || Base->GetIndices().empty())
						continue;

					if (!System->PushGeometryBuffer(Base->GetMaterial()))
						continue;

					Base->Fill(Device, IndexBuffer, VertexBuffer);
					Device->Render.World.Identify();
					Device->Render.WorldViewProj = System->View.ViewProjection;
					Device->Render.TexCoord = Base->TexCoord;
					Device->SetVertexBuffer(VertexBuffer, 0);
					Device->SetIndexBuffer(IndexBuffer, Graphics::Format::R32_Uint);
					Device->UpdateBuffer(Graphics::RenderBufferType::Render);
					Device->DrawIndexed((unsigned int)Base->GetIndices().size(), 0, 0);
					Count++;
				}

				return Count;
			}
			size_t SoftBody::RenderGeometryVoxels(Core::Timer* Time, const std::vector<Components::SoftBody*>& Geometry, RenderOpt Options)
			{
				TH_ASSERT(System->GetScene() != nullptr, 0, "scene should be set");

				SceneGraph* Scene = System->GetScene();
				Graphics::GraphicsDevice* Device = System->GetDevice();
				Device->SetInputLayout(Layout);
				Device->SetSamplerState(Sampler, 4, 6, TH_PS);
				Device->SetShader(Shaders.Voxelize, TH_VS | TH_PS | TH_GS);
				Lighting::SetVoxelBuffer(System, Shaders.Voxelize, 3);

				Viewer& View = System->View;
				size_t Count = 0;

				for (auto* Base : Geometry)
				{
					if (!Base->Static || Base->GetIndices().empty())
						continue;

					if (!System->PushVoxelsBuffer(Base->GetMaterial()))
						continue;

					Base->Fill(Device, IndexBuffer, VertexBuffer);
					Device->Render.World.Identify();
					Device->Render.WorldViewProj.Identify();
					Device->Render.TexCoord = Base->TexCoord;
					Device->SetVertexBuffer(VertexBuffer, 0);
					Device->SetIndexBuffer(IndexBuffer, Graphics::Format::R32_Uint);
					Device->UpdateBuffer(Graphics::RenderBufferType::Render);
					Device->DrawIndexed((unsigned int)Base->GetIndices().size(), 0, 0);
					
					Count++;
				}

				Device->SetShader(nullptr, TH_GS);
				return Count;
			}
			size_t SoftBody::RenderDepthLinear(Core::Timer* Time, const std::vector<Components::SoftBody*>& Geometry)
			{
				TH_ASSERT(System->GetScene() != nullptr, 0, "scene should be set");

				SceneGraph* Scene = System->GetScene();
				Graphics::GraphicsDevice* Device = System->GetDevice();
				Device->SetDepthStencilState(DepthStencil);
				Device->SetBlendState(Blend);
				Device->SetRasterizerState(Rasterizer);
				Device->SetInputLayout(Layout);
				Device->SetSamplerState(Sampler, 1, 1, TH_PS);
				Device->SetShader(Shaders.Depth.Linear, TH_VS | TH_PS);

				size_t Count = 0;
				for (auto* Base : Geometry)
				{
					if (Base->GetIndices().empty())
						continue;

					if (!System->PushDepthLinearBuffer(Base->GetMaterial()))
						continue;

					Base->Fill(Device, IndexBuffer, VertexBuffer);
					Device->Render.World.Identify();
					Device->Render.WorldViewProj = System->View.ViewProjection;
					Device->Render.TexCoord = Base->TexCoord;
					Device->UpdateBuffer(Graphics::RenderBufferType::Render);
					Device->SetVertexBuffer(VertexBuffer, 0);
					Device->SetIndexBuffer(IndexBuffer, Graphics::Format::R32_Uint);
					Device->DrawIndexed((unsigned int)Base->GetIndices().size(), 0, 0);

					Count++;
				}

				Device->SetTexture2D(nullptr, 1, TH_PS);
				return Count;
			}
			size_t SoftBody::RenderDepthCubic(Core::Timer* Time, const std::vector<Components::SoftBody*>& Geometry, Compute::Matrix4x4* ViewProjection)
			{
				TH_ASSERT(System->GetScene() != nullptr, 0, "scene should be set");

				SceneGraph* Scene = System->GetScene();
				Graphics::GraphicsDevice* Device = System->GetDevice();
				Device->SetDepthStencilState(DepthStencil);
				Device->SetBlendState(Blend);
				Device->SetRasterizerState(Rasterizer);
				Device->SetInputLayout(Layout);
				Device->SetSamplerState(Sampler, 1, 1, TH_PS);
				Device->SetShader(Shaders.Depth.Linear, TH_VS | TH_PS | TH_GS);
				Device->SetBuffer(Shaders.Depth.Cubic, 3, TH_VS | TH_PS | TH_GS);
				Device->UpdateBuffer(Shaders.Depth.Cubic, ViewProjection);

				size_t Count = 0;
				for (auto* Base : Geometry)
				{
					auto* Transform = Base->GetEntity()->GetTransform();
					if (!Base->GetBody())
						continue;

					if (!System->PushDepthCubicBuffer(Base->GetMaterial()))
						continue;

					Base->Fill(Device, IndexBuffer, VertexBuffer);
					Device->Render.World.Identify();
					Device->Render.TexCoord = Base->TexCoord;
					Device->UpdateBuffer(Graphics::RenderBufferType::Render);
					Device->SetVertexBuffer(VertexBuffer, 0);
					Device->SetIndexBuffer(IndexBuffer, Graphics::Format::R32_Uint);
					Device->DrawIndexed((unsigned int)Base->GetIndices().size(), 0, 0);

					Count++;
				}

				Device->SetShader(nullptr, TH_GS);
				Device->SetTexture2D(nullptr, 1, TH_PS);
				return Count;
			}

			Model::Model(Engine::RenderSystem* Lab) : GeometryRenderer<Components::Model>(Lab)
			{
				TH_ASSERT_V(System != nullptr, "render system should be set");
				TH_ASSERT_V(System->GetDevice() != nullptr, "graphics device should be set");

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
			size_t Model::CullGeometry(const Viewer& View, const std::vector<Components::Model*>& Geometry, bool Dirty)
			{
				TH_ASSERT(System->GetPrimitives() != nullptr, 0, "primitive cache should be set");

				Graphics::ElementBuffer* Box[2];
				System->GetPrimitives()->GetBoxBuffers(Box);

				Graphics::GraphicsDevice* Device = System->GetDevice();
				Device->SetRasterizerState(BackRasterizer);
				Device->SetInputLayout(Layout);
				Device->SetShader(nullptr, TH_PS);
				Device->SetShader(Shaders.Occlusion, TH_VS);

				size_t Count = 0;
				if (System->PreciseCulling)
				{
					for (auto* Base : Geometry)
					{
						if (!Base->WasVisible(Dirty))
							continue;

						auto* Drawable = Base->GetDrawable();
						if (!Drawable || Drawable->Meshes.empty() || !Base->Query.Begin(Device))
							continue;

						auto& World = Base->GetEntity()->GetBox();
						if (Base->Query.GetPassed() > 0)
						{
							for (auto&& Mesh : Drawable->Meshes)
							{
								Device->Render.World = Mesh->World * World;
								Device->Render.WorldViewProj = Device->Render.World * View.ViewProjection;
								Device->UpdateBuffer(Graphics::RenderBufferType::Render);
								Device->DrawIndexed(Mesh);
							}
						}
						else
						{
							Device->Render.World = World;
							Device->Render.WorldViewProj = Device->Render.World * View.ViewProjection;
							Device->SetVertexBuffer(Box[(size_t)BufferType::Vertex], 0);
							Device->SetIndexBuffer(Box[(size_t)BufferType::Index], Graphics::Format::R32_Uint);
							Device->UpdateBuffer(Graphics::RenderBufferType::Render);
							Device->DrawIndexed(Box[(size_t)BufferType::Index]->GetElements(), 0, 0);
						}
						Base->Query.End(Device);

						Count++;
					}
				}
				else
				{
					Device->SetVertexBuffer(Box[(size_t)BufferType::Vertex], 0);
					Device->SetIndexBuffer(Box[(size_t)BufferType::Index], Graphics::Format::R32_Uint);
					for (auto* Base : Geometry)
					{
						auto* Drawable = Base->GetDrawable();
						if (!Drawable || Drawable->Meshes.empty() || !Base->Query.Begin(Device))
							continue;

						Device->Render.World = Base->GetEntity()->GetBox();
						Device->Render.WorldViewProj = Device->Render.World * View.ViewProjection;
						Device->UpdateBuffer(Graphics::RenderBufferType::Render);
						Device->DrawIndexed(Box[(size_t)BufferType::Index]->GetElements(), 0, 0);
						Base->Query.End(Device);

						Count++;
					}
				}

				return Count;
			}
			size_t Model::RenderGeometryResult(Core::Timer* Time, const std::vector<Components::Model*>& Geometry, RenderOpt Options)
			{
				TH_ASSERT(System->GetScene() != nullptr, 0, "scene should be set");

				SceneGraph* Scene = System->GetScene();
				Graphics::GraphicsDevice* Device = System->GetDevice();
				bool Static = ((size_t)Options & (size_t)RenderOpt::Static);

				Device->SetDepthStencilState(DepthStencil);
				Device->SetBlendState(Blend);
				Device->SetRasterizerState(BackRasterizer);
				Device->SetInputLayout(Layout);
				Device->SetSamplerState(Sampler, 1, 7, TH_PS);
				Device->SetShader(Shaders.Geometry, TH_VS | TH_PS);

				size_t Count = 0;
				for (auto* Base : Geometry)
				{
					auto* Drawable = Base->GetDrawable();
					if (!Drawable || (Static && !Base->Static))
						continue;

					auto& World = Base->GetEntity()->GetBox();
					Device->Render.TexCoord = Base->TexCoord;

					for (auto&& Mesh : Drawable->Meshes)
					{
						if (!System->PushGeometryBuffer(Base->GetMaterial(Mesh)))
							continue;

						Device->Render.World = Mesh->World * World;
						Device->Render.WorldViewProj = Device->Render.World * System->View.ViewProjection;
						Device->UpdateBuffer(Graphics::RenderBufferType::Render);
						Device->DrawIndexed(Mesh);
					}

					Count++;
				}

				return Count;
			}
			size_t Model::RenderGeometryVoxels(Core::Timer* Time, const std::vector<Components::Model*>& Geometry, RenderOpt Options)
			{
				TH_ASSERT(System->GetScene() != nullptr, 0, "scene should be set");

				SceneGraph* Scene = System->GetScene();
				Graphics::GraphicsDevice* Device = System->GetDevice();
				Device->SetInputLayout(Layout);
				Device->SetSamplerState(Sampler, 4, 6, TH_PS);
				Device->SetShader(Shaders.Voxelize, TH_VS | TH_PS | TH_GS);
				Lighting::SetVoxelBuffer(System, Shaders.Voxelize, 3);

				Viewer& View = System->View;
				size_t Count = 0;

				for (auto* Base : Geometry)
				{
					auto* Drawable = Base->GetDrawable();
					if (!Drawable)
						continue;

					auto& World = Base->GetEntity()->GetBox();
					Device->Render.TexCoord = Base->TexCoord;

					for (auto&& Mesh : Drawable->Meshes)
					{
						if (!System->PushVoxelsBuffer(Base->GetMaterial(Mesh)))
							continue;

						Device->Render.WorldViewProj = Device->Render.World = Mesh->World * World;
						Device->UpdateBuffer(Graphics::RenderBufferType::Render);
						Device->DrawIndexed(Mesh);
					}

					Count++;
				}

				Device->SetShader(nullptr, TH_GS);
				return Count;
			}
			size_t Model::RenderDepthLinear(Core::Timer* Time, const std::vector<Components::Model*>& Geometry)
			{
				TH_ASSERT(System->GetScene() != nullptr, 0, "scene should be set");

				SceneGraph* Scene = System->GetScene();
				Graphics::GraphicsDevice* Device = System->GetDevice();
				Device->SetDepthStencilState(DepthStencil);
				Device->SetBlendState(Blend);
				Device->SetRasterizerState(FrontRasterizer);
				Device->SetInputLayout(Layout);
				Device->SetSamplerState(Sampler, 1, 1, TH_PS);
				Device->SetShader(Shaders.Depth.Linear, TH_VS | TH_PS);

				size_t Count = 0;
				for (auto* Base : Geometry)
				{
					auto* Drawable = Base->GetDrawable();
					if (!Drawable)
						continue;

					auto& World = Base->GetEntity()->GetBox();
					Device->Render.TexCoord = Base->TexCoord;

					for (auto&& Mesh : Drawable->Meshes)
					{
						if (!System->PushDepthLinearBuffer(Base->GetMaterial(Mesh)))
							continue;

						Device->Render.World = Mesh->World * World;
						Device->Render.WorldViewProj = Device->Render.World * System->View.ViewProjection;
						Device->UpdateBuffer(Graphics::RenderBufferType::Render);
						Device->DrawIndexed(Mesh);
					}

					Count++;
				}

				Device->SetTexture2D(nullptr, 1, TH_PS);
				return Count;
			}
			size_t Model::RenderDepthCubic(Core::Timer* Time, const std::vector<Components::Model*>& Geometry, Compute::Matrix4x4* ViewProjection)
			{
				TH_ASSERT(System->GetScene() != nullptr, 0, "scene should be set");

				SceneGraph* Scene = System->GetScene();
				Graphics::GraphicsDevice* Device = System->GetDevice();
				Device->SetDepthStencilState(DepthStencil);
				Device->SetBlendState(Blend);
				Device->SetRasterizerState(FrontRasterizer);
				Device->SetInputLayout(Layout);
				Device->SetSamplerState(Sampler, 1, 1, TH_PS);
				Device->SetShader(Shaders.Depth.Cubic, TH_VS | TH_PS | TH_GS);
				Device->SetBuffer(Shaders.Depth.Cubic, 3, TH_VS | TH_PS | TH_GS);
				Device->UpdateBuffer(Shaders.Depth.Cubic, ViewProjection);

				size_t Count = 0;
				for (auto* Base : Geometry)
				{
					auto* Drawable = Base->GetDrawable();
					if (!Drawable)
						continue;

					auto& World = Base->GetEntity()->GetBox();
					Device->Render.TexCoord = Base->TexCoord;

					for (auto&& Mesh : Drawable->Meshes)
					{
						if (!System->PushDepthCubicBuffer(Base->GetMaterial(Mesh)))
							continue;

						Device->Render.World = Mesh->World * World;
						Device->UpdateBuffer(Graphics::RenderBufferType::Render);
						Device->DrawIndexed(Mesh);
					}

					Count++;
				}

				Device->SetTexture2D(nullptr, 1, TH_PS);
				Device->SetShader(nullptr, TH_GS);
				return Count;
			}

			Skin::Skin(Engine::RenderSystem* Lab) : GeometryRenderer<Components::Skin>(Lab)
			{
				TH_ASSERT_V(System != nullptr, "render system should be set");
				TH_ASSERT_V(System->GetDevice() != nullptr, "graphics device should be set");

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
			size_t Skin::CullGeometry(const Viewer& View, const std::vector<Components::Skin*>& Geometry, bool Dirty)
			{
				TH_ASSERT(System->GetPrimitives() != nullptr, 0, "primitive cache should be set");

				Graphics::ElementBuffer* Box[2];
				System->GetPrimitives()->GetSkinBoxBuffers(Box);

				Graphics::GraphicsDevice* Device = System->GetDevice();
				Device->SetRasterizerState(BackRasterizer);
				Device->SetInputLayout(Layout);
				Device->SetShader(nullptr, TH_PS);
				Device->SetShader(Shaders.Occlusion, TH_VS);

				size_t Count = 0;
				if (System->PreciseCulling)
				{
					for (auto* Base : Geometry)
					{
						if (!Base->WasVisible(Dirty))
							continue;

						auto* Drawable = Base->GetDrawable();
						if (!Drawable || Drawable->Meshes.empty())
							continue;

						if (!Base->Query.Begin(Device))
							continue;

						auto& World = Base->GetEntity()->GetBox();
						if (Base->Query.GetPassed() > 0)
						{
							Device->Animation.Animated = (float)!Base->GetDrawable()->Joints.empty();
							if (Device->Animation.Animated > 0)
								memcpy(Device->Animation.Offsets, Base->Skeleton.Transform, 96 * sizeof(Compute::Matrix4x4));
							Device->UpdateBuffer(Graphics::RenderBufferType::Animation);

							for (auto&& Mesh : Drawable->Meshes)
							{
								Device->Render.World = Mesh->World * World;
								Device->Render.WorldViewProj = Device->Render.World * View.ViewProjection;
								Device->UpdateBuffer(Graphics::RenderBufferType::Render);
								Device->DrawIndexed(Mesh);
							}
						}
						else
						{
							Device->Animation.Animated = (float)false;
							Device->Render.World = World;
							Device->Render.WorldViewProj = Device->Render.World * View.ViewProjection;
							Device->UpdateBuffer(Graphics::RenderBufferType::Animation);
							Device->UpdateBuffer(Graphics::RenderBufferType::Render);
							Device->SetVertexBuffer(Box[(size_t)BufferType::Vertex], 0);
							Device->SetIndexBuffer(Box[(size_t)BufferType::Index], Graphics::Format::R32_Uint);
							Device->DrawIndexed(Box[(size_t)BufferType::Index]->GetElements(), 0, 0);
						}
						Base->Query.End(Device);

						Count++;
					}
				}
				else
				{
					Device->SetVertexBuffer(Box[(size_t)BufferType::Vertex], 0);
					Device->SetIndexBuffer(Box[(size_t)BufferType::Index], Graphics::Format::R32_Uint);
					for (auto* Base : Geometry)
					{
						auto* Drawable = Base->GetDrawable();
						if (!Drawable || Drawable->Meshes.empty())
							continue;

						if (!Base->Query.Begin(Device))
							continue;

						Device->Animation.Animated = (float)false;
						Device->Render.World = Base->GetEntity()->GetBox();
						Device->Render.WorldViewProj = Device->Render.World * View.ViewProjection;
						Device->UpdateBuffer(Graphics::RenderBufferType::Animation);
						Device->UpdateBuffer(Graphics::RenderBufferType::Render);
						Device->DrawIndexed(Box[(size_t)BufferType::Index]->GetElements(), 0, 0);
						Base->Query.End(Device);

						Count++;
					}
				}

				return Count;
			}
			size_t Skin::RenderGeometryResult(Core::Timer* Time, const std::vector<Components::Skin*>& Geometry, RenderOpt Options)
			{
				TH_ASSERT(System->GetScene() != nullptr, 0, "scene should be set");

				SceneGraph* Scene = System->GetScene();
				Graphics::GraphicsDevice* Device = System->GetDevice();
				bool Static = ((size_t)Options & (size_t)RenderOpt::Static);

				Device->SetDepthStencilState(DepthStencil);
				Device->SetBlendState(Blend);
				Device->SetRasterizerState(BackRasterizer);
				Device->SetInputLayout(Layout);
				Device->SetSamplerState(Sampler, 1, 7, TH_PS);
				Device->SetShader(Shaders.Geometry, TH_VS | TH_PS);

				size_t Count = 0;
				for (auto* Base : Geometry)
				{
					auto* Drawable = Base->GetDrawable();
					if (!Drawable || (Static && !Base->Static))
						continue;

					Device->Render.TexCoord = Base->TexCoord;
					Device->Animation.Animated = (float)!Base->GetDrawable()->Joints.empty();

					if (Device->Animation.Animated > 0)
						memcpy(Device->Animation.Offsets, Base->Skeleton.Transform, 96 * sizeof(Compute::Matrix4x4));

					auto& World = Base->GetEntity()->GetBox();
					Device->UpdateBuffer(Graphics::RenderBufferType::Animation);

					for (auto&& Mesh : Drawable->Meshes)
					{
						if (!System->PushGeometryBuffer(Base->GetMaterial(Mesh)))
							continue;

						Device->Render.World = Mesh->World * World;
						Device->Render.WorldViewProj = Device->Render.World * System->View.ViewProjection;
						Device->UpdateBuffer(Graphics::RenderBufferType::Render);
						Device->DrawIndexed(Mesh);
					}

					Count++;
				}

				return Count;
			}
			size_t Skin::RenderGeometryVoxels(Core::Timer* Time, const std::vector<Components::Skin*>& Geometry, RenderOpt Options)
			{
				TH_ASSERT(System->GetScene() != nullptr, 0, "scene should be set");

				SceneGraph* Scene = System->GetScene();
				Graphics::GraphicsDevice* Device = System->GetDevice();
				Device->SetInputLayout(Layout);
				Device->SetSamplerState(Sampler, 4, 6, TH_PS);
				Device->SetShader(Shaders.Voxelize, TH_VS | TH_PS | TH_GS);
				Lighting::SetVoxelBuffer(System, Shaders.Voxelize, 3);

				Viewer& View = System->View;
				size_t Count = 0;

				for (auto* Base : Geometry)
				{
					auto* Drawable = Base->GetDrawable();
					if (!Base->Static || !Drawable)
						continue;

					Device->Render.TexCoord = Base->TexCoord;
					Device->Animation.Animated = (float)!Base->GetDrawable()->Joints.empty();

					if (Device->Animation.Animated > 0)
						memcpy(Device->Animation.Offsets, Base->Skeleton.Transform, 96 * sizeof(Compute::Matrix4x4));

					auto& World = Base->GetEntity()->GetBox();
					Device->UpdateBuffer(Graphics::RenderBufferType::Animation);

					for (auto&& Mesh : Drawable->Meshes)
					{
						if (!System->PushVoxelsBuffer(Base->GetMaterial(Mesh)))
							continue;

						Device->Render.WorldViewProj = Device->Render.World = Mesh->World * World;
						Device->UpdateBuffer(Graphics::RenderBufferType::Render);
						Device->DrawIndexed(Mesh);
					}

					Count++;
				}

				Device->SetShader(nullptr, TH_GS);
				return Count;
			}
			size_t Skin::RenderDepthLinear(Core::Timer* Time, const std::vector<Components::Skin*>& Geometry)
			{
				TH_ASSERT(System->GetScene() != nullptr, 0, "scene should be set");

				SceneGraph* Scene = System->GetScene();
				Graphics::GraphicsDevice* Device = System->GetDevice();
				Device->SetDepthStencilState(DepthStencil);
				Device->SetBlendState(Blend);
				Device->SetRasterizerState(FrontRasterizer);
				Device->SetInputLayout(Layout);
				Device->SetSamplerState(Sampler, 1, 1, TH_PS);
				Device->SetShader(Shaders.Depth.Linear, TH_VS | TH_PS);

				size_t Count = 0;
				for (auto* Base : Geometry)
				{
					auto* Drawable = Base->GetDrawable();
					if (!Drawable)
						continue;

					Device->Render.TexCoord = Base->TexCoord;
					Device->Animation.Animated = (float)!Base->GetDrawable()->Joints.empty();

					if (Device->Animation.Animated > 0)
						memcpy(Device->Animation.Offsets, Base->Skeleton.Transform, 96 * sizeof(Compute::Matrix4x4));

					auto& World = Base->GetEntity()->GetBox();
					Device->UpdateBuffer(Graphics::RenderBufferType::Animation);

					for (auto&& Mesh : Drawable->Meshes)
					{
						if (!System->PushDepthLinearBuffer(Base->GetMaterial(Mesh)))
							continue;

						Device->Render.World = Mesh->World * World;
						Device->Render.WorldViewProj = Device->Render.World * System->View.ViewProjection;
						Device->UpdateBuffer(Graphics::RenderBufferType::Render);
						Device->DrawIndexed(Mesh);
					}

					Count++;
				}

				Device->SetTexture2D(nullptr, 1, TH_PS);
				return Count;
			}
			size_t Skin::RenderDepthCubic(Core::Timer* Time, const std::vector<Components::Skin*>& Geometry, Compute::Matrix4x4* ViewProjection)
			{
				TH_ASSERT(System->GetScene() != nullptr, 0, "scene should be set");

				SceneGraph* Scene = System->GetScene();
				Graphics::GraphicsDevice* Device = System->GetDevice();
				Device->SetDepthStencilState(DepthStencil);
				Device->SetBlendState(Blend);
				Device->SetRasterizerState(FrontRasterizer);
				Device->SetInputLayout(Layout);
				Device->SetSamplerState(Sampler, 1, 1, TH_PS);
				Device->SetShader(Shaders.Depth.Linear, TH_VS | TH_PS | TH_GS);
				Device->SetBuffer(Shaders.Depth.Cubic, 3, TH_VS | TH_PS | TH_GS);
				Device->UpdateBuffer(Shaders.Depth.Cubic, ViewProjection);

				size_t Count = 0;
				for (auto* Base : Geometry)
				{
					auto* Drawable = Base->GetDrawable();
					if (!Drawable)
						continue;

					Device->Render.TexCoord = Base->TexCoord;
					Device->Animation.Animated = (float)!Base->GetDrawable()->Joints.empty();

					if (Device->Animation.Animated > 0)
						memcpy(Device->Animation.Offsets, Base->Skeleton.Transform, 96 * sizeof(Compute::Matrix4x4));

					auto& World = Base->GetEntity()->GetBox();
					Device->UpdateBuffer(Graphics::RenderBufferType::Animation);

					for (auto&& Mesh : Drawable->Meshes)
					{
						if (!System->PushDepthCubicBuffer(Base->GetMaterial(Mesh)))
							continue;

						Device->Render.World = Mesh->World * World;
						Device->UpdateBuffer(Graphics::RenderBufferType::Render);
						Device->DrawIndexed(Mesh);
					}

					Count++;
				}

				Device->SetTexture2D(nullptr, 1, TH_PS);
				Device->SetShader(nullptr, TH_GS);
				return Count;
			}

			Emitter::Emitter(RenderSystem* Lab) : GeometryRenderer<Components::Emitter>(Lab)
			{
				TH_ASSERT_V(System != nullptr, "render system should be set");
				TH_ASSERT_V(System->GetDevice() != nullptr, "graphics device should be set");

				Graphics::GraphicsDevice* Device = System->GetDevice();
				DepthStencilOpaque = Device->GetDepthStencilState("less");
				DepthStencilAdditive = Device->GetDepthStencilState("less-none");
				Rasterizer = Device->GetRasterizerState("cull-back");
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
			size_t Emitter::RenderGeometryResult(Core::Timer* Time, const std::vector<Components::Emitter*>& Geometry, RenderOpt Options)
			{
				TH_ASSERT(System->GetScene() != nullptr, 0, "scene should be set");

				SceneGraph* Scene = System->GetScene();
				Graphics::GraphicsDevice* Device = System->GetDevice();
				Graphics::Shader* BaseShader = nullptr;
				Graphics::PrimitiveTopology T = Device->GetPrimitiveTopology();
				Viewer& View = System->View;
				bool Static = ((size_t)Options & (size_t)RenderOpt::Static);

				if ((size_t)Options & (size_t)RenderOpt::Additive)
				{
					BaseShader = Shaders.Transparency;
					Device->SetDepthStencilState(DepthStencilAdditive);
					Device->SetBlendState(AdditiveBlend);
				}
				else
				{
					BaseShader = Shaders.Opaque;
					Device->SetDepthStencilState(DepthStencilOpaque);
					Device->SetBlendState(OverwriteBlend);
				}

				Device->SetPrimitiveTopology(Graphics::PrimitiveTopology::Point_List);
				Device->SetRasterizerState(Rasterizer);
				Device->SetInputLayout(Layout);
				Device->SetSamplerState(Sampler, 1, 7, TH_PS);
				Device->SetShader(BaseShader, TH_VS | TH_PS);
				Device->SetVertexBuffer(nullptr, 0);

				size_t Count = 0;
				for (auto* Base : Geometry)
				{
					if ((Static && !Base->Static) || !Base->GetBuffer())
						continue;

					if (!System->PushGeometryBuffer(Base->GetMaterial()))
						continue;

					Device->Render.World = View.Projection;
					Device->Render.WorldViewProj = (Base->QuadBased ? View.View : View.ViewProjection);
					Device->Render.TexCoord = Base->GetEntity()->GetTransform()->Forward();
					if (Base->Connected)
						Device->Render.WorldViewProj = Base->GetEntity()->GetBox() * Device->Render.WorldViewProj;

					Device->SetBuffer(Base->GetBuffer(), 8, TH_VS | TH_PS);
					Device->SetShader(Base->QuadBased ? BaseShader : nullptr, TH_GS);
					Device->UpdateBuffer(Base->GetBuffer());
					Device->UpdateBuffer(Graphics::RenderBufferType::Render);
					Device->Draw((unsigned int)Base->GetBuffer()->GetArray()->Size(), 0);

					Count++;
				}

				Device->SetBuffer((Graphics::InstanceBuffer*)nullptr, 8, TH_VS | TH_PS);
				Device->SetShader(nullptr, TH_GS);
				Device->SetPrimitiveTopology(T);
				return Count;
			}
			size_t Emitter::RenderDepthLinear(Core::Timer* Time, const std::vector<Components::Emitter*>& Geometry)
			{
				TH_ASSERT(System->GetScene() != nullptr, 0, "scene should be set");

				SceneGraph* Scene = System->GetScene();
				Graphics::GraphicsDevice* Device = System->GetDevice();
				Graphics::PrimitiveTopology T = Device->GetPrimitiveTopology();
				Viewer& View = System->View;

				Device->SetPrimitiveTopology(Graphics::PrimitiveTopology::Point_List);
				Device->SetDepthStencilState(DepthStencilOpaque);
				Device->SetBlendState(OverwriteBlend);
				Device->SetRasterizerState(Rasterizer);
				Device->SetInputLayout(Layout);
				Device->SetSamplerState(Sampler, 1, 1, TH_PS);
				Device->SetShader(Shaders.Depth.Linear, TH_VS | TH_PS);
				Device->SetVertexBuffer(nullptr, 0);

				size_t Count = 0;
				for (auto* Base : Geometry)
				{
					if (!Base->GetBuffer() || !System->PushDepthLinearBuffer(Base->GetMaterial()))
						continue;

					Device->Render.World = View.Projection;
					Device->Render.WorldViewProj = (Base->QuadBased ? View.View : View.ViewProjection);
					if (Base->Connected)
						Device->Render.WorldViewProj = Base->GetEntity()->GetBox() * Device->Render.WorldViewProj;

					Device->SetBuffer(Base->GetBuffer(), 8, TH_VS | TH_PS);
					Device->SetShader(Base->QuadBased ? Shaders.Depth.Linear : nullptr, TH_GS);
					Device->UpdateBuffer(Graphics::RenderBufferType::Render);
					Device->Draw((unsigned int)Base->GetBuffer()->GetArray()->Size(), 0);

					Count++;
				}

				Device->SetTexture2D(nullptr, 1, TH_PS);
				Device->SetBuffer((Graphics::InstanceBuffer*)nullptr, 8, TH_VS | TH_PS);
				Device->SetShader(nullptr, TH_GS);
				Device->SetPrimitiveTopology(T);
				return Count;
			}
			size_t Emitter::RenderDepthCubic(Core::Timer* Time, const std::vector<Components::Emitter*>& Geometry, Compute::Matrix4x4* ViewProjection)
			{
				TH_ASSERT(System->GetScene() != nullptr, 0, "scene should be set");

				Graphics::GraphicsDevice* Device = System->GetDevice();
				SceneGraph* Scene = System->GetScene();
				auto& Source = System->View;
				Depth.FaceView[0] = Compute::Matrix4x4::CreateLookAt(Compute::CubeFace::PositiveX, Source.Position);
				Depth.FaceView[1] = Compute::Matrix4x4::CreateLookAt(Compute::CubeFace::NegativeX, Source.Position);
				Depth.FaceView[2] = Compute::Matrix4x4::CreateLookAt(Compute::CubeFace::PositiveY, Source.Position);
				Depth.FaceView[3] = Compute::Matrix4x4::CreateLookAt(Compute::CubeFace::NegativeY, Source.Position);
				Depth.FaceView[4] = Compute::Matrix4x4::CreateLookAt(Compute::CubeFace::PositiveZ, Source.Position);
				Depth.FaceView[5] = Compute::Matrix4x4::CreateLookAt(Compute::CubeFace::NegativeZ, Source.Position);

				Graphics::PrimitiveTopology T = Device->GetPrimitiveTopology();
				Device->SetPrimitiveTopology(Graphics::PrimitiveTopology::Point_List);
				Device->SetDepthStencilState(DepthStencilOpaque);
				Device->SetBlendState(OverwriteBlend);
				Device->SetRasterizerState(Rasterizer);
				Device->SetInputLayout(Layout);
				Device->SetVertexBuffer(nullptr, 0);
				Device->SetSamplerState(Sampler, 1, 1, TH_PS);
				Device->SetBuffer(Shaders.Depth.Quad, 3, TH_VS | TH_PS | TH_GS);
				Device->UpdateBuffer(Shaders.Depth.Quad, &Depth);

				size_t Count = 0;
				for (auto* Base : Geometry)
				{
					if (!Base->GetBuffer() || !System->PushDepthCubicBuffer(Base->GetMaterial()))
						continue;

					Device->Render.World = (Base->Connected ? Base->GetEntity()->GetBox() : Compute::Matrix4x4::Identity());
					Device->SetBuffer(Base->GetBuffer(), 8, TH_VS | TH_PS);
					Device->SetShader(Base->QuadBased ? Shaders.Depth.Quad : Shaders.Depth.Point, TH_VS | TH_PS | TH_GS);
					Device->UpdateBuffer(Graphics::RenderBufferType::Render);
					Device->Draw((unsigned int)Base->GetBuffer()->GetArray()->Size(), 0);

					Count++;
				}

				Device->SetTexture2D(nullptr, 1, TH_PS);
				Device->SetBuffer((Graphics::InstanceBuffer*)nullptr, 8, TH_VS | TH_PS);
				Device->SetShader(nullptr, TH_GS);
				Device->SetPrimitiveTopology(T);
				return Count;
			}

			Decal::Decal(RenderSystem* Lab) : GeometryRenderer<Components::Decal>(Lab)
			{
				TH_ASSERT_V(System != nullptr, "render system should be set");
				TH_ASSERT_V(System->GetDevice() != nullptr, "graphics device should be set");

				Graphics::GraphicsDevice* Device = System->GetDevice();
				DepthStencil = Device->GetDepthStencilState("none");
				Rasterizer = Device->GetRasterizerState("cull-back");
				Blend = Device->GetBlendState("additive");
				Sampler = Device->GetSamplerState("trilinear-x16");
				Layout = Device->GetInputLayout("shape-vertex");

				Shader = System->CompileShader("geometry/decal/geometry");
			}
			Decal::~Decal()
			{
				System->FreeShader(Shader);
			}
			size_t Decal::RenderGeometryResult(Core::Timer* Time, const std::vector<Components::Decal*>& Geometry, RenderOpt Options)
			{
				TH_ASSERT(System->GetScene() != nullptr, 0, "scene should be set");

				Graphics::MultiRenderTarget2D* MRT = System->GetMRT(TargetType::Main);
				Graphics::GraphicsDevice* Device = System->GetDevice();
				SceneGraph* Scene = System->GetScene();
				bool Static = ((size_t)Options & (size_t)RenderOpt::Static);

				Graphics::ElementBuffer* Box[2];
				System->GetPrimitives()->GetBoxBuffers(Box);

				Device->SetDepthStencilState(DepthStencil);
				Device->SetBlendState(Blend);
				Device->SetRasterizerState(Rasterizer);
				Device->SetInputLayout(Layout);
				Device->SetTarget(MRT, 0);
				Device->SetSamplerState(Sampler, 1, 8, TH_PS);
				Device->SetShader(Shader, TH_VS | TH_PS);
				Device->SetTexture2D(MRT->GetTarget(2), 8, TH_PS);
				Device->SetVertexBuffer(Box[(size_t)BufferType::Vertex], 0);
				Device->SetIndexBuffer(Box[(size_t)BufferType::Index], Graphics::Format::R32_Uint);

				size_t Count = 0;
				for (auto* Base : Geometry)
				{
					if ((Static && !Base->Static) || !System->PushGeometryBuffer(Base->GetMaterial()))
						continue;

					Device->Render.WorldViewProj = Base->GetEntity()->GetBox() * System->View.ViewProjection;
					Device->Render.World = Device->Render.WorldViewProj.Inv();
					Device->Render.TexCoord = Base->TexCoord;
					Device->UpdateBuffer(Graphics::RenderBufferType::Render);
					Device->DrawIndexed((unsigned int)Box[(size_t)BufferType::Index]->GetElements(), 0, 0);

					Count++;
				}

				Device->SetTexture2D(nullptr, 8, TH_PS);
				System->RestoreOutput();
				return Count;
			}

			Lighting::Lighting(RenderSystem* Lab) : Renderer(Lab), EnableGI(true)
			{
				TH_ASSERT_V(System != nullptr, "render system should be set");
				TH_ASSERT_V(System->GetDevice() != nullptr, "graphics device should be set");

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
				DepthSampler = Device->GetSamplerState("depth");
				DepthLessSampler = Device->GetSamplerState("depth-cmp-less");
				DepthGreaterSampler = Device->GetSamplerState("depth-cmp-greater");
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
			void Lighting::Deserialize(ContentManager* Content, Core::Document* Node)
			{
				TH_ASSERT_V(Content != nullptr, "content manager should be set");
				TH_ASSERT_V(Node != nullptr, "document should be set");

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
			void Lighting::Serialize(ContentManager* Content, Core::Document* Node)
			{
				TH_ASSERT_V(Content != nullptr, "content manager should be set");
				TH_ASSERT_V(Node != nullptr, "document should be set");

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
			void Lighting::ResizeBuffers()
			{
				FlushDepthBuffersAndCache();

				Shadows.PointLight.resize(Shadows.PointLightLimits);
				for (auto It = Shadows.PointLight.begin(); It != Shadows.PointLight.end(); ++It)
				{
					Graphics::DepthTargetCube::Desc F = Graphics::DepthTargetCube::Desc();
					F.Size = (unsigned int)Shadows.PointLightResolution;
					F.FormatMode = Graphics::Format::D32_Float;

					*It = System->GetDevice()->CreateDepthTargetCube(F);
				}

				Shadows.SpotLight.resize(Shadows.SpotLightLimits);
				for (auto It = Shadows.SpotLight.begin(); It != Shadows.SpotLight.end(); ++It)
				{
					Graphics::DepthTarget2D::Desc F = Graphics::DepthTarget2D::Desc();
					F.Width = (unsigned int)Shadows.SpotLightResolution;
					F.Height = (unsigned int)Shadows.SpotLightResolution;
					F.FormatMode = Graphics::Format::D32_Float;

					*It = System->GetDevice()->CreateDepthTarget2D(F);
				}

				Shadows.LineLight.resize(Shadows.LineLightLimits);
				for (auto It = Shadows.LineLight.begin(); It != Shadows.LineLight.end(); ++It)
					*It = nullptr;
			}
			void Lighting::BeginPass()
			{
				auto& Lines = System->GetScene()->GetComponents<Components::LineLight>();
				Lights.Illuminators.Push(System);
				Lights.Surfaces.Push(System);
				Lights.Points.Push(System);
				Lights.Spots.Push(System);
				Lights.Lines = &Lines;

				float Distance = std::numeric_limits<float>::max();
				if (!EnableGI || !System->IsTopLevel())
					return;

				Components::Illuminator* Base = nullptr;
				for (auto* Light : Lights.Illuminators.Top())
				{
					float Subdistance = Light->GetEntity()->GetTransform()->GetPosition().Distance(System->View.Position);
					if (Subdistance < Distance)
					{
						Distance = Subdistance;
						Base = Light;
					}
				}

				Storage.Target = Base;
				Storage.Process = 0;

				if (!Base)
					return;

				if (!Base->GetBuffer())
					Base->SetBufferSize(Base->GetBufferSize());

				auto* Transform = Base->GetEntity()->GetTransform();
				VoxelBuffer.Center = Transform->GetPosition();
				VoxelBuffer.Scale = Transform->GetScale();
				VoxelBuffer.Mips = (float)Base->GetMipLevels();
				VoxelBuffer.Size = (float)Base->GetBufferSize();
				VoxelBuffer.RayStep = Base->RayStep;
				VoxelBuffer.MaxSteps = Base->MaxSteps;
				VoxelBuffer.Distance = Base->Distance;
				VoxelBuffer.Radiance = Base->Radiance;
				VoxelBuffer.Length = Base->Length;
				VoxelBuffer.Margin = Base->Margin;
				VoxelBuffer.Offset = Base->Offset;
				VoxelBuffer.Angle = Base->Angle;
				VoxelBuffer.Occlusion = Base->Occlusion;
				VoxelBuffer.Specular = Base->Specular;
				VoxelBuffer.Bleeding = Base->Bleeding;

				LightBuffer = Base->GetBuffer();
				if (!LightBuffer)
					return;

				bool Inside = Compute::Common::HasPointIntersectedCube(VoxelBuffer.Center, VoxelBuffer.Scale, System->View.Position);
				if (!Inside && Storage.Reference == Base && Storage.Inside == Inside)
					Storage.Process = 1;
				else
					Storage.Process = 2;
			}
			void Lighting::EndPass()
			{
				Lights.Spots.Pop();
				Lights.Points.Pop();
				Lights.Surfaces.Pop();
				Lights.Illuminators.Pop();
			}
			void Lighting::FlushDepthBuffersAndCache()
			{
				if (System != nullptr)
				{
					SceneGraph* Scene = System->GetScene();
					if (Scene != nullptr)
					{
						auto& Lights = Scene->GetComponents<Components::PointLight>();
						for (auto It = Lights.Begin(); It != Lights.End(); ++It)
							((Components::PointLight*)*It)->DepthMap = nullptr;

						Lights = Scene->GetComponents<Components::SpotLight>();
						for (auto It = Lights.Begin(); It != Lights.End(); ++It)
							((Components::SpotLight*)*It)->DepthMap = nullptr;

						Lights = Scene->GetComponents<Components::LineLight>();
						for (auto It = Lights.Begin(); It != Lights.End(); ++It)
							((Components::LineLight*)*It)->DepthMap = nullptr;
					}
				}

				for (auto It = Shadows.PointLight.begin(); It != Shadows.PointLight.end(); ++It)
					TH_RELEASE(*It);

				for (auto It = Shadows.SpotLight.begin(); It != Shadows.SpotLight.end(); ++It)
					TH_RELEASE(*It);

				for (auto It = Shadows.LineLight.begin(); It != Shadows.LineLight.end(); ++It)
				{
					if (*It != nullptr)
					{
						for (auto* Target : *(*It))
							TH_RELEASE(Target);
					}

					TH_DELETE(vector, *It);
				}
			}
			void Lighting::RenderResultBuffers(RenderOpt Options)
			{
				State.Inner = ((size_t)Options & (size_t)RenderOpt::Inner);
				State.Backcull = true;

				SceneGraph* Scene = System->GetScene();
				Graphics::MultiRenderTarget2D* MRT = System->GetMRT(TargetType::Main);
				Graphics::RenderTarget2D* RT = (State.Inner ? Surfaces.Input : System->GetRT(TargetType::Main));

				Graphics::ElementBuffer* Cube[2];
				System->GetPrimitives()->GetCubeBuffers(Cube);

				AmbientLight.SkyOffset = System->View.Projection.Inv() * Compute::Matrix4x4::CreateRotation(System->View.Rotation);
				State.Device->SetDepthStencilState(DepthStencilLess);
				State.Device->SetBlendState(BlendAdditive);
				State.Device->SetRasterizerState(BackRasterizer);
				State.Device->SetInputLayout(Layout);
				State.Device->CopyTarget(MRT, 0, RT, 0);
				State.Device->SetTarget(MRT, 0, 0, 0, 0);
				State.Device->SetSamplerState(WrapSampler, 1, 4, TH_PS);
				State.Device->SetSamplerState(DepthSampler, 5, 1, TH_PS);
				State.Device->SetSamplerState(DepthLessSampler, 6, 1, TH_PS);
				State.Device->SetSamplerState(DepthGreaterSampler, 7, 1, TH_PS);
				State.Device->SetTexture2D(RT->GetTarget(), 1, TH_PS);
				State.Device->SetTexture2D(MRT->GetTarget(1), 2, TH_PS);
				State.Device->SetTexture2D(MRT->GetTarget(2), 3, TH_PS);
				State.Device->SetTexture2D(MRT->GetTarget(3), 4, TH_PS);
				State.Device->SetVertexBuffer(Cube[(size_t)BufferType::Vertex], 0);
				State.Device->SetIndexBuffer(Cube[(size_t)BufferType::Index], Graphics::Format::R32_Uint);

				RenderSurfaceLights();
				RenderPointLights();
				RenderSpotLights();
				RenderLineLights();
				RenderIllumination();
				RenderAmbient();

				State.Device->FlushTexture(1, 11, TH_PS);
			}
			void Lighting::RenderVoxelMap(Core::Timer* Time)
			{
				if (Storage.Process < 2)
					return;

				auto* Target = (Components::Illuminator*)Storage.Target;
				if (!Target->Tick.TickEvent(Time->GetElapsedTime()))
					return;

				Graphics::Texture3D* In[3], * Out[3];
				if (!State.Scene->GetVoxelBuffer(In, Out))
					return;

				size_t Size = State.Scene->GetVoxelBufferSize();
				State.Device->ClearWritable(In[(size_t)VoxelType::Diffuse]);
				State.Device->ClearWritable(In[(size_t)VoxelType::Normal]);
				State.Device->ClearWritable(In[(size_t)VoxelType::Surface]);
				State.Device->SetTargetRect(Size, Size);
				State.Device->SetDepthStencilState(DepthStencilNone);
				State.Device->SetBlendState(BlendOverwrite);
				State.Device->SetRasterizerState(NoneRasterizer);
				State.Device->SetWriteable(In, 1, 3, false);

				if (Storage.Inside)
				{
					System->View.FarPlane = GetDominant(VoxelBuffer.Scale) * 2.0f;
					System->Render(Time, RenderState::Geometry_Voxels, RenderOpt::Inner);
					System->RestoreViewBuffer(nullptr);
				}
				else
					System->Render(Time, RenderState::Geometry_Voxels, RenderOpt::Inner);

				State.Device->SetWriteable(Out, 1, 3, false);
				State.Device->GenerateMips(LightBuffer);
				Storage.Reference = Target;
			}
			void Lighting::RenderSurfaceMaps(Core::Timer* Time)
			{
				auto& Data = Lights.Surfaces.Top();
				if (Data.empty())
					return;

				if (!Surfaces.Merger || !Surfaces.Subresource || !Surfaces.Input || !Surfaces.Output)
					SetSurfaceBufferSize(Surfaces.Size);

				State.Scene->SwapMRT(TargetType::Main, Surfaces.Merger);
				State.Scene->SetMRT(TargetType::Main, false);

				double ElapsedTime = Time->GetElapsedTime();
				for (auto* Light : Data)
				{
					if (Light->IsImageBased())
						continue;

					Graphics::TextureCube* Cache = Light->GetProbeCache();
					if (!Cache)
					{
						Cache = State.Device->CreateTextureCube();
						Light->SetProbeCache(Cache);
					}
					else if (!Light->Tick.TickEvent(ElapsedTime) || Light->Tick.Delay <= 0.0)
						continue;

					State.Device->CubemapPush(Surfaces.Subresource, Cache);
					Light->Locked = true;

					Compute::Vector3 Position = Light->GetEntity()->GetTransform()->GetPosition() * Light->Offset;
					for (unsigned int j = 0; j < 6; j++)
					{
						Compute::CubeFace Face = (Compute::CubeFace)j;
						State.Scene->ClearMRT(TargetType::Main, true, true);
						System->SetView(Light->View[j] = Compute::Matrix4x4::CreateLookAt(Face, Position), Light->Projection, Position, 0.1f, Light->GetSize().Radius, RenderCulling::Spot);
						System->Render(Time, RenderState::Geometry_Result, Light->StaticMask ? RenderOpt::Inner | RenderOpt::Static : RenderOpt::Inner);
						State.Device->CubemapFace(Surfaces.Subresource, Face);
					}

					Light->Locked = false;
					State.Device->CubemapPop(Surfaces.Subresource);
				}

				State.Scene->SwapMRT(TargetType::Main, nullptr);
				System->RestoreViewBuffer(nullptr);
			}
			void Lighting::RenderPointShadowMaps(Core::Timer* Time)
			{
				uint64_t Counter = 0;
				for (auto* Light : Lights.Points.Top())
				{
					if (Counter >= Shadows.PointLight.size())
						break;

					Light->DepthMap = nullptr;
					if (!Light->Shadow.Enabled)
						continue;

					CubicDepthMap* Target = Shadows.PointLight[Counter];
					Light->GenerateOrigin();
					Light->DepthMap = Target;

					State.Device->SetTarget(Target);
					State.Device->ClearDepth(Target);
					System->SetView(Compute::Matrix4x4::Identity(), Light->Projection, Light->GetEntity()->GetTransform()->GetPosition(), 0.1f, Light->Shadow.Distance, RenderCulling::Point);
					System->Render(Time, RenderState::Depth_Cubic, RenderOpt::Inner);
					Counter++;
				}
			}
			void Lighting::RenderSpotShadowMaps(Core::Timer* Time)
			{
				uint64_t Counter = 0;
				for (auto* Light : Lights.Spots.Top())
				{
					if (Counter >= Shadows.SpotLight.size())
						break;

					Light->DepthMap = nullptr;
					if (!Light->Shadow.Enabled)
						continue;

					LinearDepthMap* Target = Shadows.SpotLight[Counter];
					Light->GenerateOrigin();
					Light->DepthMap = Target;

					State.Device->SetTarget(Target);
					State.Device->ClearDepth(Target);
					System->SetView(Light->View, Light->Projection, Light->GetEntity()->GetTransform()->GetPosition(), 0.1f, Light->Shadow.Distance, RenderCulling::Spot);
					System->Render(Time, RenderState::Depth_Linear, RenderOpt::Inner);
					Counter++;
				}
			}
			void Lighting::RenderLineShadowMaps(Core::Timer* Time)
			{
				uint64_t Counter = 0;
				for (auto It = Lights.Lines->Begin(); It != Lights.Lines->End(); ++It)
				{
					auto* Light = (Components::LineLight*)*It;
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

					Light->GenerateOrigin();
					Light->DepthMap = Target;

					for (size_t i = 0; i < Target->size(); i++)
					{
						LinearDepthMap* Cascade = (*Target)[i];
						State.Device->SetTarget(Cascade);
						State.Device->ClearDepth(Cascade);

						float Distance = Light->Shadow.Distance[i];
						System->SetView(Light->View[i], Light->Projection[i], 0.0f, -System->View.FarPlane, System->View.FarPlane, RenderCulling::Line);
						System->Render(Time, RenderState::Depth_Linear, RenderOpt::Inner);
					}

					Counter++;
				}

				System->RestoreViewBuffer(nullptr);
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
					for (auto* Light : Lights.Surfaces.Top())
					{
						if (!Light->GetProbeCache())
							continue;

						Entity* Base = Light->GetEntity();
						GetLightCulling(Light, Base->GetRadius(), &Position, &Scale);
						GetSurfaceLight(&SurfaceLight, Light, Position, Scale);

						SurfaceLight.Lighting *= Base->GetVisibility(System->View);
						State.Device->SetTextureCube(Light->GetProbeCache(), 5, TH_PS);
						State.Device->UpdateBuffer(Shaders.Surface, &SurfaceLight);
						State.Device->DrawIndexed((unsigned int)Cube[(size_t)BufferType::Index]->GetElements(), 0, 0);
					}
				}
				else if (AmbientLight.Recursive > 0.0f)
				{
					State.Device->SetShader(Shaders.Surface, TH_VS | TH_PS);
					State.Device->SetBuffer(Shaders.Surface, 3, TH_VS | TH_PS);

					Compute::Vector3 Position, Scale;
					for (auto* Light : Lights.Surfaces.Top())
					{
						if (Light->Locked || !Light->GetProbeCache())
							continue;

						Entity* Base = Light->GetEntity();
						GetLightCulling(Light, Base->GetRadius(), &Position, &Scale);
						GetSurfaceLight(&SurfaceLight, Light, Position, Scale);

						SurfaceLight.Lighting *= Base->GetVisibility(System->View);
						State.Device->SetTextureCube(Light->GetProbeCache(), 5, TH_PS);
						State.Device->UpdateBuffer(Shaders.Surface, &SurfaceLight);
						State.Device->DrawIndexed((unsigned int)Cube[(size_t)BufferType::Index]->GetElements(), 0, 0);
					}
				}
			}
			void Lighting::RenderPointLights()
			{
				Graphics::ElementBuffer* Cube[2];
				System->GetPrimitives()->GetCubeBuffers(Cube);

				Graphics::Shader* BaseShader = nullptr;
				State.Device->SetBuffer(Shaders.Point[0], 3, TH_VS | TH_PS);

				Compute::Vector3 Position, Scale;
				for (auto* Light : Lights.Points.Top())
				{
					Entity* Base = Light->GetEntity();
					GetLightCulling(Light, Base->GetRadius(), &Position, &Scale);

					if (GetPointLight(&PointLight, Light, Position, Scale))
					{
						Graphics::TextureCube* DepthMap = Light->DepthMap->GetTarget();
						State.Device->SetTextureCube(DepthMap, 5, TH_PS);
						State.Device->SetTextureCube(DepthMap, 6, TH_PS);
						State.Device->SetTextureCube(DepthMap, 7, TH_PS);
						BaseShader = Shaders.Point[1];
					}
					else
						BaseShader = Shaders.Point[0];

					PointLight.Lighting *= Base->GetVisibility(System->View);
					State.Device->SetShader(BaseShader, TH_VS | TH_PS);
					State.Device->UpdateBuffer(Shaders.Point[0], &PointLight);
					State.Device->DrawIndexed((unsigned int)Cube[(size_t)BufferType::Index]->GetElements(), 0, 0);
				}
			}
			void Lighting::RenderSpotLights()
			{
				Graphics::ElementBuffer* Cube[2];
				System->GetPrimitives()->GetCubeBuffers(Cube);

				Graphics::Shader* BaseShader = nullptr;
				State.Device->SetBuffer(Shaders.Spot[0], 3, TH_VS | TH_PS);

				Compute::Vector3 Position, Scale;
				for (auto* Light : Lights.Spots.Top())
				{
					Entity* Base = Light->GetEntity();
					GetLightCulling(Light, Base->GetRadius(), &Position, &Scale);

					if (GetSpotLight(&SpotLight, Light, Position, Scale))
					{
						Graphics::Texture2D* DepthMap = Light->DepthMap->GetTarget();
						State.Device->SetTexture2D(DepthMap, 5, TH_PS);
						State.Device->SetTexture2D(DepthMap, 6, TH_PS);
						State.Device->SetTexture2D(DepthMap, 7, TH_PS);
						BaseShader = Shaders.Spot[1];
					}
					else
						BaseShader = Shaders.Spot[0];

					SpotLight.Lighting *= Base->GetVisibility(System->View);
					State.Device->SetShader(BaseShader, TH_VS | TH_PS);
					State.Device->UpdateBuffer(Shaders.Spot[0], &SpotLight);
					State.Device->DrawIndexed((unsigned int)Cube[(size_t)BufferType::Index]->GetElements(), 0, 0);
				}
			}
			void Lighting::RenderLineLights()
			{
				Graphics::Shader* BaseShader = nullptr;
				if (!State.Backcull)
				{
					State.Device->SetRasterizerState(BackRasterizer);
					State.Backcull = true;
				}

				State.Device->SetDepthStencilState(DepthStencilNone);
				State.Device->SetSamplerState(DepthLessSampler, 5, 6, TH_PS);
				State.Device->SetBuffer(Shaders.Line[0], 3, TH_VS | TH_PS);
				State.Device->SetVertexBuffer(System->GetPrimitives()->GetQuad(), 0);

				for (auto It = Lights.Lines->Begin(); It != Lights.Lines->End(); ++It)
				{
					auto* Light = (Components::LineLight*)*It;
					if (GetLineLight(&LineLight, Light))
					{
						uint32_t Size = (uint32_t)LineLight.Cascades;
						for (uint32_t i = 0; i < Size; i++)
							State.Device->SetTexture2D((*Light->DepthMap)[i]->GetTarget(), 5 + i, TH_PS);
						BaseShader = Shaders.Line[1];

						if (Size < 6)
						{
							auto* Target = (*Light->DepthMap)[Size - 1]->GetTarget();
							for (uint32_t i = Size; i < 6; i++)
								State.Device->SetTexture2D(Target, 5 + i, TH_PS);
						}
					}
					else
						BaseShader = Shaders.Line[0];

					State.Device->SetShader(BaseShader, TH_VS | TH_PS);
					State.Device->UpdateBuffer(Shaders.Line[0], &LineLight);
					State.Device->Draw(6, 0);
				}

				State.Device->SetBlendState(BlendOverload);
			}
			void Lighting::RenderLuminance()
			{
				Graphics::Texture3D* In[3], * Out[3];
				if (Storage.Process < 2 || !State.Scene->GetVoxelBuffer(In, Out))
					return;

				uint32_t X = (uint32_t)(VoxelBuffer.Size.X / 8.0f);
				uint32_t Y = (uint32_t)(VoxelBuffer.Size.Y / 8.0f);
				uint32_t Z = (uint32_t)(VoxelBuffer.Size.Z / 8.0f);

				State.Device->ClearWritable(LightBuffer);
				State.Device->SetSamplerState(nullptr, 1, 6, TH_CS);
				State.Device->SetWriteable(Out, 1, 3, false);
				State.Device->SetWriteable(&LightBuffer, 1, 1, true);
				State.Device->SetTexture3D(In[(size_t)VoxelType::Diffuse], 2, TH_CS);
				State.Device->SetTexture3D(In[(size_t)VoxelType::Normal], 3, TH_CS);
				State.Device->SetTexture3D(In[(size_t)VoxelType::Surface], 4, TH_CS);

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
				State.Device->SetShader(Shaders.Voxelize, TH_VS | TH_PS | TH_CS);
				State.Device->Dispatch(X, Y, Z);
				State.Device->FlushTexture(2, 3, TH_CS);
				State.Device->SetShader(nullptr, TH_VS | TH_PS | TH_CS);
				State.Device->SetStructureBuffer(nullptr, 5, TH_CS);
				State.Device->SetStructureBuffer(nullptr, 6, TH_CS);
				State.Device->SetStructureBuffer(nullptr, 7, TH_CS);
				State.Device->SetWriteable(Out, 1, 1, true);
				State.Device->SetWriteable(In, 1, 3, false);
			}
			void Lighting::RenderIllumination()
			{
				if (!EnableGI || State.Inner || Storage.Process < 1)
					return;

				Graphics::ElementBuffer* Cube[2];
				System->GetPrimitives()->GetCubeBuffers(Cube);

				Compute::Vector3 Position, Scale;
				GetLightCulling(Storage.Target, 0.0f, &Position, &Scale);
				VoxelBuffer.WorldViewProjection = Compute::Matrix4x4::CreateTranslatedScale(Position, Scale) * System->View.ViewProjection;

				Graphics::MultiRenderTarget2D* MRT = System->GetMRT(TargetType::Main);
				Graphics::RenderTarget2D* RT = System->GetRT(TargetType::Secondary);
				State.Device->CopyTarget(MRT, 0, RT, 0);
				State.Device->SetDepthStencilState(DepthStencilNone);
				State.Device->SetSamplerState(WrapSampler, 1, 6, TH_PS);
				State.Device->SetTexture2D(RT->GetTarget(), 1, TH_PS);
				State.Device->SetTexture3D(LightBuffer, 5, TH_PS);
				State.Device->SetShader(Shaders.Ambient[1], TH_VS | TH_PS);
				State.Device->SetBuffer(Shaders.Ambient[1], 3, TH_VS | TH_PS);
				State.Device->SetVertexBuffer(Cube[(size_t)BufferType::Vertex], 0);
				State.Device->SetIndexBuffer(Cube[(size_t)BufferType::Index], Graphics::Format::R32_Uint);
				State.Device->UpdateBuffer(Shaders.Ambient[1], &VoxelBuffer);
				State.Device->DrawIndexed((unsigned int)Cube[(size_t)BufferType::Index]->GetElements(), 0, 0);
				State.Device->SetTexture2D(System->GetRT(TargetType::Main)->GetTarget(), 1, TH_PS);
				State.Device->SetTexture3D(nullptr, 5, TH_PS);
				State.Device->SetVertexBuffer(System->GetPrimitives()->GetQuad(), 0);

				if (!State.Backcull)
					State.Device->SetRasterizerState(BackRasterizer);
			}
			void Lighting::RenderAmbient()
			{
				Graphics::MultiRenderTarget2D* MRT = System->GetMRT(TargetType::Main);
				Graphics::RenderTarget2D* RT = (State.Inner ? Surfaces.Output : System->GetRT(TargetType::Secondary));
				State.Device->CopyTarget(MRT, 0, RT, 0);
				State.Device->SetSamplerState(WrapSampler, 1, 6, TH_PS);
				State.Device->SetTexture2D(RT->GetTarget(), 5, TH_PS);
				State.Device->SetTextureCube(SkyMap, 6, TH_PS);
				State.Device->SetShader(Shaders.Ambient[0], TH_VS | TH_PS);
				State.Device->SetBuffer(Shaders.Ambient[0], 3, TH_VS | TH_PS);
				State.Device->UpdateBuffer(Shaders.Ambient[0], &AmbientLight);
				State.Device->Draw(6, 0);
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
				TH_ASSERT_V(System->GetScene() != nullptr, "scene should be set");

				SceneGraph* Scene = System->GetScene();
				Graphics::GraphicsDevice* Device = System->GetDevice();
				Graphics::MultiRenderTarget2D::Desc F1 = Scene->GetDescMRT();
				F1.MipLevels = Device->GetMipLevel((unsigned int)Surfaces.Size, (unsigned int)Surfaces.Size);
				F1.Width = (unsigned int)Surfaces.Size;
				F1.Height = (unsigned int)Surfaces.Size;
				SurfaceLight.Mips = (float)F1.MipLevels;
				Surfaces.Size = NewSize;

				TH_RELEASE(Surfaces.Merger);
				Surfaces.Merger = Device->CreateMultiRenderTarget2D(F1);

				Graphics::Cubemap::Desc I;
				I.Source = Surfaces.Merger;
				I.MipLevels = F1.MipLevels;
				I.Size = Surfaces.Size;

				TH_RELEASE(Surfaces.Subresource);
				Surfaces.Subresource = Device->CreateCubemap(I);

				Graphics::RenderTarget2D::Desc F2 = Scene->GetDescRT();
				F2.MipLevels = F1.MipLevels;
				F2.Width = F1.Width;
				F2.Height = F1.Height;

				TH_RELEASE(Surfaces.Output);
				Surfaces.Output = Device->CreateRenderTarget2D(F2);

				TH_RELEASE(Surfaces.Input);
				Surfaces.Input = Device->CreateRenderTarget2D(F2);
			}
			void Lighting::SetVoxelBuffer(RenderSystem* System, Graphics::Shader* Src, unsigned int Slot)
			{
				TH_ASSERT_V(System != nullptr, "system should be set");
				TH_ASSERT_V(System->GetScene() != nullptr, "scene should be set");
				TH_ASSERT_V(Src != nullptr, "src should be set");

				Lighting* Renderer = System->GetRenderer<Lighting>();
				if (!Renderer)
					return;

				Graphics::GraphicsDevice* Device = System->GetDevice();
				Device->SetBuffer(Src, Slot, TH_VS | TH_PS | TH_GS);
				Device->UpdateBuffer(Src, &Renderer->VoxelBuffer);
			}
			size_t Lighting::GeneratePointLights()
			{
				if (!Storage.PBuffer)
					GenerateLightBuffers();

				size_t Count = 0;
				for (auto* Light : Lights.Points.Top())
				{
					if (Count >= Storage.MaxLights)
						continue;

					Compute::Vector3 Position(Light->GetEntity()->GetTransform()->GetPosition()), Scale(Light->GetEntity()->GetRadius());
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
				for (auto* Light : Lights.Spots.Top())
				{
					if (Count >= Storage.MaxLights)
						continue;

					Compute::Vector3 Position(Light->GetEntity()->GetTransform()->GetPosition()), Scale(Light->GetEntity()->GetRadius());
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
				for (auto It = Lights.Lines->Begin(); It != Lights.Lines->End(); ++It)
				{
					auto* Light = (Components::LineLight*)*It;
					if (Count >= Storage.MaxLights)
						continue;

					GetLineLight(&Storage.LArray[Count], Light);
					Count++;
				}

				VoxelBuffer.Lights.Z = (float)Count;
				return Count;
			}
			size_t Lighting::RenderPass(Core::Timer* Time, RenderState Status, RenderOpt Options)
			{
				TH_ASSERT(System->GetScene() != nullptr, 0, "scene should be set");
				if ((size_t)Options & (size_t)RenderOpt::Additive)
					return 0;

				State.Device = System->GetDevice();
				State.Scene = System->GetScene();

				if (Status == RenderState::Geometry_Voxels)
				{
					RenderLuminance();
					return 1;
				}
				else if (Status != RenderState::Geometry_Result)
					return 0;

				if (!((size_t)Options & (size_t)RenderOpt::Inner || (size_t)Options & (size_t)RenderOpt::Transparent))
				{
					double ElapsedTime = Time->GetElapsedTime();
					if (Shadows.Tick.TickEvent(ElapsedTime))
					{
						RenderPointShadowMaps(Time);
						RenderSpotShadowMaps(Time);
						RenderLineShadowMaps(Time);
						System->RestoreViewBuffer(nullptr);
					}

					RenderSurfaceMaps(Time);
					if (EnableGI && !Lights.Illuminators.Top().empty())
						RenderVoxelMap(Time);
				}

				RenderResultBuffers(Options);
				System->RestoreOutput();
				return 1;
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
				Components::SurfaceLight* Light = (Components::SurfaceLight*)Src;
				auto* Entity = Light->GetEntity();
				auto* Transform = Entity->GetTransform();
				auto& Size = Light->GetSize();

				Dest->WorldViewProjection = Compute::Matrix4x4::CreateTranslatedScale(Position, Scale) * System->View.ViewProjection;
				Dest->Position = Transform->GetPosition();
				Dest->Lighting = Light->Diffuse.Mul(Light->Emission);
				Dest->Scale = Transform->GetScale();
				Dest->Parallax = (Light->Parallax ? 1.0f : 0.0f);
				Dest->Infinity = Light->Infinity;
				Dest->Attenuation.X = Size.C1;
				Dest->Attenuation.Y = Size.C2;
				Dest->Range = Size.Radius;

				return true;
			}
			bool Lighting::GetPointLight(IPointLight* Dest, Component* Src, Compute::Vector3& Position, Compute::Vector3& Scale)
			{
				Components::PointLight* Light = (Components::PointLight*)Src;
				auto* Entity = Light->GetEntity();
				auto* Transform = Entity->GetTransform();
				auto& Size = Light->GetSize();

				Dest->WorldViewProjection = Compute::Matrix4x4::CreateTranslatedScale(Position, Scale) * System->View.ViewProjection;
				Dest->Position = Transform->GetPosition();
				Dest->Lighting = Light->Diffuse.Mul(Light->Emission);
				Dest->Attenuation.X = Size.C1;
				Dest->Attenuation.Y = Size.C2;
				Dest->Range = Size.Radius;

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
				Components::SpotLight* Light = (Components::SpotLight*)Src;
				auto* Entity = Light->GetEntity();
				auto* Transform = Entity->GetTransform();
				auto& Size = Light->GetSize();

				Dest->WorldViewProjection = Compute::Matrix4x4::CreateTranslatedScale(Position, Scale) * System->View.ViewProjection;
				Dest->ViewProjection = Light->View * Light->Projection;
				Dest->Direction = Transform->GetRotation().dDirection();
				Dest->Position = Transform->GetPosition();
				Dest->Lighting = Light->Diffuse.Mul(Light->Emission);
				Dest->Cutoff = Compute::Mathf::Cos(Compute::Mathf::Deg2Rad() * Light->Cutoff * 0.5f);
				Dest->Attenuation.X = Size.C1;
				Dest->Attenuation.Y = Size.C2;
				Dest->Range = Size.Radius;

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
				Components::LineLight* Light = (Components::LineLight*)Src;
				Dest->Position = Light->GetEntity()->GetTransform()->GetPosition().sNormalize();
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
				Dest->Cascades = (float)std::min(Light->Shadow.Cascades, (uint32_t)Light->DepthMap->size());

				size_t Size = (size_t)Dest->Cascades;
				for (size_t i = 0; i < Size; i++)
					Dest->ViewProjection[i] = Light->View[i] * Light->Projection[i];

				return Size > 0;
			}
			void Lighting::GetLightCulling(Component* Src, float Range, Compute::Vector3* Position, Compute::Vector3* Scale)
			{
				auto* Transform = Src->GetEntity()->GetTransform();
				*Position = Transform->GetPosition();
				*Scale = (Range > 0.0f ? Range : Transform->GetScale());

				bool Front = Compute::Common::HasPointIntersectedCube(*Position, Scale->Mul(1.01f), System->View.Position);
				if (!(Front && State.Backcull) && !(!Front && !State.Backcull))
					return;

				State.Device->SetRasterizerState(Front ? FrontRasterizer : BackRasterizer);
				State.Device->SetDepthStencilState(Front ? DepthStencilGreater : DepthStencilLess);
				State.Backcull = !State.Backcull;
			}
			void Lighting::GenerateLightBuffers()
			{
				Graphics::ElementBuffer::Desc F = Graphics::ElementBuffer::Desc();
				F.AccessFlags = Graphics::CPUAccess::Write;
				F.MiscFlags = Graphics::ResourceMisc::Buffer_Structured;
				F.Usage = Graphics::ResourceUsage::Dynamic;
				F.BindFlags = Graphics::ResourceBind::Shader_Input;
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
				CascadedDepthMap* Target = (*Result ? *Result : TH_NEW(CascadedDepthMap));
				for (auto It = Target->begin(); It != Target->end(); ++It)
					TH_RELEASE(*It);

				Target->resize(Size);
				for (auto It = Target->begin(); It != Target->end(); ++It)
				{
					Graphics::DepthTarget2D::Desc F = Graphics::DepthTarget2D::Desc();
					F.Width = (unsigned int)Shadows.LineLightResolution;
					F.Height = (unsigned int)Shadows.LineLightResolution;
					F.FormatMode = Graphics::Format::D32_Float;

					*It = System->GetDevice()->CreateDepthTarget2D(F);
				}

				*Result = Target;
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
				TH_ASSERT_V(System != nullptr, "render system should be set");
				TH_ASSERT_V(System->GetDevice() != nullptr, "graphics device should be set");

				Graphics::GraphicsDevice* Device = System->GetDevice();
				DepthStencil = Device->GetDepthStencilState("none");
				Rasterizer = Device->GetRasterizerState("cull-back");
				Blend = Device->GetBlendState("overwrite");
				Sampler = Device->GetSamplerState("trilinear-x16");
				Layout = Device->GetInputLayout("shape-vertex");

				Shader = System->CompileShader("merger/transparency", sizeof(RenderData));
			}
			Transparency::~Transparency()
			{
				System->FreeShader(Shader);
				TH_RELEASE(Merger);
				TH_RELEASE(Input);
			}
			void Transparency::ResizeBuffers()
			{
				TH_ASSERT_V(System->GetScene() != nullptr, "scene should be set");

				SceneGraph* Scene = System->GetScene();
				Graphics::MultiRenderTarget2D::Desc F1 = Scene->GetDescMRT();
				Graphics::RenderTarget2D::Desc F2 = Scene->GetDescRT();
				Graphics::GraphicsDevice* Device = System->GetDevice();
				MipLevels[(size_t)TargetType::Main] = (float)F1.MipLevels;

				auto* Renderer = System->GetRenderer<Lighting>();
				if (Renderer != nullptr)
				{
					MipLevels[(size_t)TargetType::Secondary] = (float)Device->GetMipLevel((unsigned int)Renderer->Surfaces.Size, (unsigned int)Renderer->Surfaces.Size);
					F1.MipLevels = (unsigned int)MipLevels[(size_t)TargetType::Secondary];
					F1.Width = (unsigned int)Renderer->Surfaces.Size;
					F1.Height = (unsigned int)Renderer->Surfaces.Size;
					F2.MipLevels = F1.MipLevels;
					F2.Width = F1.Width;
					F2.Height = F1.Height;
				}

				TH_RELEASE(Merger);
				Merger = Device->CreateMultiRenderTarget2D(F1);

				TH_RELEASE(Input);
				Input = Device->CreateRenderTarget2D(F2);
			}
			size_t Transparency::RenderPass(Core::Timer* Time, RenderState State, RenderOpt Options)
			{
				TH_ASSERT(System->GetScene() != nullptr, 0, "scene should be set");

				bool Inner = ((size_t)Options & (size_t)RenderOpt::Inner);
				if (State != RenderState::Geometry_Result || (size_t)Options & (size_t)RenderOpt::Transparent || (size_t)Options & (size_t)RenderOpt::Additive)
					return 0;

				SceneGraph* Scene = System->GetScene();
				if (System->HasCategory(GeoCategory::Additive))
					System->Render(Time, RenderState::Geometry_Result, Options | RenderOpt::Additive);

				if (!System->HasCategory(GeoCategory::Transparent))
					return 0;

				Graphics::MultiRenderTarget2D* MainMRT = System->GetMRT(TargetType::Main);
				Graphics::MultiRenderTarget2D* MRT = (Inner ? Merger : System->GetMRT(TargetType::Secondary));
				Graphics::RenderTarget2D* RT = (Inner ? Input : System->GetRT(TargetType::Main));
				Graphics::GraphicsDevice* Device = System->GetDevice();
				RenderData.Mips = (Inner ? MipLevels[(size_t)TargetType::Secondary] : MipLevels[(size_t)TargetType::Main]);

				Scene->SwapMRT(TargetType::Main, MRT);
				Scene->SetMRT(TargetType::Main, true);
				System->Render(Time, RenderState::Geometry_Result, Options | RenderOpt::Transparent);
				Scene->SwapMRT(TargetType::Main, nullptr);

				Device->CopyTarget(MainMRT, 0, RT, 0);
				Device->GenerateMips(RT->GetTarget());
				Device->SetTarget(MainMRT, 0);
				Device->Clear(MainMRT, 0, 0, 0, 0);
				Device->UpdateBuffer(Shader, &RenderData);
				Device->SetDepthStencilState(DepthStencil);
				Device->SetBlendState(Blend);
				Device->SetRasterizerState(Rasterizer);
				Device->SetInputLayout(Layout);
				Device->SetSamplerState(Sampler, 1, 8, TH_PS);
				Device->SetTexture2D(RT->GetTarget(), 1, TH_PS);
				Device->SetTexture2D(MainMRT->GetTarget(1), 2, TH_PS);
				Device->SetTexture2D(MainMRT->GetTarget(2), 3, TH_PS);
				Device->SetTexture2D(MainMRT->GetTarget(3), 4, TH_PS);
				Device->SetTexture2D(MRT->GetTarget(0), 5, TH_PS);
				Device->SetTexture2D(MRT->GetTarget(1), 6, TH_PS);
				Device->SetTexture2D(MRT->GetTarget(2), 7, TH_PS);
				Device->SetTexture2D(MRT->GetTarget(3), 8, TH_PS);
				Device->SetShader(Shader, TH_VS | TH_PS);
				Device->SetBuffer(Shader, 3, TH_VS | TH_PS);
				Device->SetVertexBuffer(System->GetPrimitives()->GetQuad(), 0);
				Device->UpdateBuffer(Graphics::RenderBufferType::Render);
				Device->Draw(6, 0);
				Device->FlushTexture(1, 8, TH_PS);
				System->RestoreOutput();
				return 1;
			}

			SSR::SSR(RenderSystem* Lab) : EffectRenderer(Lab)
			{
				Shaders.Reflectance = CompileEffect("raytracing/reflectance", sizeof(Reflectance));
				Shaders.Gloss[0] = CompileEffect("blur/gloss-x", sizeof(Gloss));
				Shaders.Gloss[1] = CompileEffect("blur/gloss-y");
				Shaders.Additive = CompileEffect("merger/additive");
			}
			void SSR::Deserialize(ContentManager* Content, Core::Document* Node)
			{
				TH_ASSERT_V(Content != nullptr, "content manager should be set");
				TH_ASSERT_V(Node != nullptr, "document should be set");

				NMake::Unpack(Node->Find("samples-1"), &Reflectance.Samples);
				NMake::Unpack(Node->Find("samples-2"), &Gloss.Samples);
				NMake::Unpack(Node->Find("intensity"), &Reflectance.Intensity);
				NMake::Unpack(Node->Find("blur"), &Gloss.Blur);
			}
			void SSR::Serialize(ContentManager* Content, Core::Document* Node)
			{
				TH_ASSERT_V(Content != nullptr, "content manager should be set");
				TH_ASSERT_V(Node != nullptr, "document should be set");

				NMake::Pack(Node->Set("samples-1"), Reflectance.Samples);
				NMake::Pack(Node->Set("samples-2"), Gloss.Samples);
				NMake::Pack(Node->Set("intensity"), Reflectance.Intensity);
				NMake::Pack(Node->Set("blur"), Gloss.Blur);
			}
			void SSR::RenderEffect(Core::Timer* Time)
			{
				Graphics::MultiRenderTarget2D* MRT = System->GetMRT(TargetType::Main);
				System->GetDevice()->GenerateMips(MRT->GetTarget(0));

				Reflectance.Mips = GetMipLevels();
				Gloss.Texel[0] = 1.0f / GetWidth();
				Gloss.Texel[1] = 1.0f / GetHeight();

				RenderMerge(Shaders.Reflectance, &Reflectance);
				RenderMerge(Shaders.Gloss[0], &Gloss, 2);
				RenderMerge(Shaders.Gloss[1], nullptr, 2);
				RenderResult(Shaders.Additive);
			}

			SSAO::SSAO(RenderSystem* Lab) : EffectRenderer(Lab)
			{
				Shaders.Shading = CompileEffect("raytracing/shading", sizeof(Shading));
				Shaders.Fibo[0] = CompileEffect("blur/fibo-x", sizeof(Fibo));
				Shaders.Fibo[1] = CompileEffect("blur/fibo-y");
				Shaders.Multiply = CompileEffect("merger/multiply");
			}
			void SSAO::Deserialize(ContentManager* Content, Core::Document* Node)
			{
				TH_ASSERT_V(Content != nullptr, "content manager should be set");
				TH_ASSERT_V(Node != nullptr, "document should be set");

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
			void SSAO::Serialize(ContentManager* Content, Core::Document* Node)
			{
				TH_ASSERT_V(Content != nullptr, "content manager should be set");
				TH_ASSERT_V(Node != nullptr, "document should be set");

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
			void SSAO::RenderEffect(Core::Timer* Time)
			{
				Fibo.Texel[0] = 1.0f / GetWidth();
				Fibo.Texel[1] = 1.0f / GetHeight();

				RenderMerge(Shaders.Shading, &Shading);
				RenderMerge(Shaders.Fibo[0], &Fibo, 2);
				RenderMerge(Shaders.Fibo[1], nullptr, 2);
				RenderResult(Shaders.Multiply);
			}

			DoF::DoF(RenderSystem* Lab) : EffectRenderer(Lab)
			{
				CompileEffect("postprocess/focus", sizeof(Focus));
			}
			void DoF::Deserialize(ContentManager* Content, Core::Document* Node)
			{
				TH_ASSERT_V(Content != nullptr, "content manager should be set");
				TH_ASSERT_V(Node != nullptr, "document should be set");

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
			void DoF::Serialize(ContentManager* Content, Core::Document* Node)
			{
				TH_ASSERT_V(Content != nullptr, "content manager should be set");
				TH_ASSERT_V(Node != nullptr, "document should be set");

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
			void DoF::RenderEffect(Core::Timer* fTime)
			{
				TH_ASSERT_V(fTime != nullptr, "time should be set");
				if (Distance > 0.0f)
					FocusAtNearestTarget(fTime->GetDeltaTime());

				Focus.Texel[0] = 1.0f / GetWidth();
				Focus.Texel[1] = 1.0f / GetHeight();
				RenderResult(nullptr, &Focus);
			}
			void DoF::FocusAtNearestTarget(float DeltaTime)
			{
				TH_ASSERT_V(System->GetScene() != nullptr, "scene should be set");

				SceneGraph* Scene = System->GetScene();
				Compute::Ray Origin;
				Origin.Origin = System->View.Position;
				Origin.Direction = System->View.Rotation.dDirection();

				bool Change = false;
				Scene->RayTest<Components::Model>(Origin, Distance, [this, &Origin, &Change](Component* Result, const Compute::Vector3& Hit)
				{
					float NextRange = Result->GetEntity()->GetRadius();
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

			MotionBlur::MotionBlur(RenderSystem* Lab) : EffectRenderer(Lab)
			{
				Shaders.Velocity = CompileEffect("postprocess/velocity", sizeof(Velocity));
				Shaders.Motion = CompileEffect("blur/motion", sizeof(Motion));
			}
			void MotionBlur::Deserialize(ContentManager* Content, Core::Document* Node)
			{
				TH_ASSERT_V(Content != nullptr, "content manager should be set");
				TH_ASSERT_V(Node != nullptr, "document should be set");

				NMake::Unpack(Node->Find("samples"), &Motion.Samples);
				NMake::Unpack(Node->Find("blur"), &Motion.Blur);
				NMake::Unpack(Node->Find("motion"), &Motion.Motion);
			}
			void MotionBlur::Serialize(ContentManager* Content, Core::Document* Node)
			{
				TH_ASSERT_V(Content != nullptr, "content manager should be set");
				TH_ASSERT_V(Node != nullptr, "document should be set");

				NMake::Pack(Node->Set("samples"), Motion.Samples);
				NMake::Pack(Node->Set("blur"), Motion.Blur);
				NMake::Pack(Node->Set("motion"), Motion.Motion);
			}
			void MotionBlur::RenderEffect(Core::Timer* Time)
			{
				TH_ASSERT_V(System->GetScene() != nullptr, "scene should be set");
				RenderMerge(Shaders.Velocity, &Velocity);
				RenderResult(Shaders.Motion, &Motion);

				SceneGraph* Scene = System->GetScene();
				Velocity.LastViewProjection = System->View.ViewProjection;
			}

			Bloom::Bloom(RenderSystem* Lab) : EffectRenderer(Lab)
			{
				Shaders.Bloom = CompileEffect("postprocess/bloom", sizeof(Extraction));
				Shaders.Fibo[0] = CompileEffect("blur/fibo-x", sizeof(Fibo));
				Shaders.Fibo[1] = CompileEffect("blur/fibo-y");
				Shaders.Additive = CompileEffect("merger/additive");
			}
			void Bloom::Deserialize(ContentManager* Content, Core::Document* Node)
			{
				NMake::Unpack(Node->Find("intensity"), &Extraction.Intensity);
				NMake::Unpack(Node->Find("threshold"), &Extraction.Threshold);
				NMake::Unpack(Node->Find("power"), &Fibo.Power);
				NMake::Unpack(Node->Find("samples"), &Fibo.Samples);
				NMake::Unpack(Node->Find("blur"), &Fibo.Blur);
			}
			void Bloom::Serialize(ContentManager* Content, Core::Document* Node)
			{
				NMake::Pack(Node->Set("intensity"), Extraction.Intensity);
				NMake::Pack(Node->Set("threshold"), Extraction.Threshold);
				NMake::Pack(Node->Set("power"), Fibo.Power);
				NMake::Pack(Node->Set("samples"), Fibo.Samples);
				NMake::Pack(Node->Set("blur"), Fibo.Blur);
			}
			void Bloom::RenderEffect(Core::Timer* Time)
			{
				Fibo.Texel[0] = 1.0f / GetWidth();
				Fibo.Texel[1] = 1.0f / GetHeight();

				RenderMerge(Shaders.Bloom, &Extraction);
				RenderMerge(Shaders.Fibo[0], &Fibo, 3);
				RenderMerge(Shaders.Fibo[1], nullptr, 3);
				RenderResult(Shaders.Additive);
			}

			Tone::Tone(RenderSystem* Lab) : EffectRenderer(Lab)
			{
				Shaders.Luminance = CompileEffect("postprocess/luminance", sizeof(Luminance));
				Shaders.Tone = CompileEffect("postprocess/tone", sizeof(Mapping));
			}
			Tone::~Tone()
			{
				TH_RELEASE(LutTarget);
				TH_RELEASE(LutMap);
			}
			void Tone::Deserialize(ContentManager* Content, Core::Document* Node)
			{
				TH_ASSERT_V(Content != nullptr, "content manager should be set");
				TH_ASSERT_V(Node != nullptr, "document should be set");

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
			void Tone::Serialize(ContentManager* Content, Core::Document* Node)
			{
				TH_ASSERT_V(Content != nullptr, "content manager should be set");
				TH_ASSERT_V(Node != nullptr, "document should be set");

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
			void Tone::RenderEffect(Core::Timer* Time)
			{
				if (Mapping.Adaptation > 0.0f)
					RenderLUT(Time);

				RenderResult(Shaders.Tone, &Mapping);
			}
			void Tone::RenderLUT(Core::Timer* Time)
			{
				TH_ASSERT_V(Time != nullptr, "time should be set");
				if (!LutMap || !LutTarget)
					SetLUTSize(1);

				Graphics::MultiRenderTarget2D* MRT = System->GetMRT(TargetType::Main);
				Graphics::GraphicsDevice* Device = System->GetDevice();
				Device->GenerateMips(MRT->GetTarget(0));
				Device->CopyTexture2D(LutTarget, 0, &LutMap);

				Luminance.Texel[0] = 1.0f / GetWidth();
				Luminance.Texel[1] = 1.0f / GetHeight();
				Luminance.Mips = GetMipLevels();
				Luminance.Time = Time->GetTimeStep() * Mapping.ASpeed;

				RenderTexture(0, LutMap);
				RenderOutput(LutTarget);
				RenderMerge(Shaders.Luminance, &Luminance);
			}
			void Tone::SetLUTSize(size_t Size)
			{
				TH_ASSERT_V(System->GetScene() != nullptr, "scene should be set");
				TH_CLEAR(LutTarget);
				TH_CLEAR(LutMap);

				SceneGraph* Scene = System->GetScene();
				Graphics::GraphicsDevice* Device = System->GetDevice();
				Graphics::RenderTarget2D::Desc RT = Scene->GetDescRT();
				RT.MipLevels = Device->GetMipLevel(Size, Size);
				RT.FormatMode = Graphics::Format::R16_Float;
				RT.Width = Size;
				RT.Height = Size;

				LutTarget = Device->CreateRenderTarget2D(RT);
				Device->CopyTexture2D(LutTarget, 0, &LutMap);
			}

			Glitch::Glitch(RenderSystem* Lab) : EffectRenderer(Lab), ScanLineJitter(0), VerticalJump(0), HorizontalShake(0), ColorDrift(0)
			{
				CompileEffect("postprocess/glitch", sizeof(Distortion));
			}
			void Glitch::Deserialize(ContentManager* Content, Core::Document* Node)
			{
				TH_ASSERT_V(Content != nullptr, "content manager should be set");
				TH_ASSERT_V(Node != nullptr, "document should be set");

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
			void Glitch::Serialize(ContentManager* Content, Core::Document* Node)
			{
				TH_ASSERT_V(Content != nullptr, "content manager should be set");
				TH_ASSERT_V(Node != nullptr, "document should be set");

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
			void Glitch::RenderEffect(Core::Timer* Time)
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
#ifdef TH_WITH_RMLUI
				TH_ASSERT_V(System != nullptr, "render system should be set");
				TH_ASSERT_V(System->GetDevice() != nullptr, "graphics device should be set");
				Context = new GUI::Context(System->GetDevice());
#endif
			}
			UserInterface::~UserInterface()
			{
#ifdef TH_WITH_RMLUI
				TH_RELEASE(Context);
#endif
			}
			size_t UserInterface::RenderPass(Core::Timer* Timer, RenderState State, RenderOpt Options)
			{
#ifdef TH_WITH_RMLUI
				TH_ASSERT(Context != nullptr, 0, "context should be set");
				if (State != RenderState::Geometry_Result || (size_t)Options & (size_t)RenderOpt::Inner || (size_t)Options & (size_t)RenderOpt::Transparent || (size_t)Options & (size_t)RenderOpt::Additive)
					return 0;

				Context->UpdateEvents(Activity);
				Context->RenderLists(System->GetMRT(TargetType::Main)->GetTarget(0));
				System->RestoreOutput();

				return 1;
#else
				return 0;
#endif
			}
#ifdef TH_WITH_RMLUI
			GUI::Context* UserInterface::GetContext()
			{
				return Context;
			}
#endif
		}
	}
}
