#include "renderers.h"
#include "components.h"

namespace Tomahawk
{
	namespace Engine
	{
		namespace Renderers
		{
			SoftBody::SoftBody(Engine::RenderSystem* Lab) : GeometryRenderer(Lab), VertexBuffer(nullptr), IndexBuffer(nullptr)
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
				Shaders.Voxelize = System->CompileShader("geometry/model/voxelize", sizeof(Lighting::IVoxelBuffer));
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
			size_t SoftBody::CullGeometry(const Viewer& View, const GeometryRenderer::Objects& Chunk)
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
					for (auto* Base : Chunk)
					{
						if (Base->GetIndices().empty() || !CullingBegin(Base))
							continue;

						Base->Fill(Device, IndexBuffer, VertexBuffer);
						Device->Render.World.Identify();
						Device->Render.Transform = Device->Render.World * View.ViewProjection;
						Device->UpdateBuffer(Graphics::RenderBufferType::Render);
						Device->SetVertexBuffer(VertexBuffer);
						Device->SetIndexBuffer(IndexBuffer, Graphics::Format::R32_Uint);
						Device->DrawIndexed((unsigned int)Base->GetIndices().size(), 0, 0);
						CullingEnd();
						Count++;
					}
				}
				else
				{
					Device->SetVertexBuffer(Box[(size_t)BufferType::Vertex]);
					Device->SetIndexBuffer(Box[(size_t)BufferType::Index], Graphics::Format::R32_Uint);

					for (auto* Base : Chunk)
					{
						if (Base->GetIndices().empty() || !CullingBegin(Base))
							continue;

						Device->Render.World = Base->GetEntity()->GetBox();
						Device->Render.Transform = Device->Render.World * View.ViewProjection;
						Device->UpdateBuffer(Graphics::RenderBufferType::Render);
						Device->DrawIndexed((unsigned int)Box[(size_t)BufferType::Index]->GetElements(), 0, 0);
						CullingEnd();

						Count++;
					}
				}

				return Count;
			}
			size_t SoftBody::RenderGeometryResult(Core::Timer* Time, const GeometryRenderer::Objects& Chunk)
			{
				TH_ASSERT(System->GetScene() != nullptr, 0, "scene should be set");

				SceneGraph* Scene = System->GetScene();
				Graphics::GraphicsDevice* Device = System->GetDevice();
				bool Static = System->State.IsSet(RenderOpt::Static);

				Device->SetDepthStencilState(DepthStencil);
				Device->SetBlendState(Blend);
				Device->SetRasterizerState(Rasterizer);
				Device->SetInputLayout(Layout);
				Device->SetSamplerState(Sampler, 1, 7, TH_PS);
				Device->SetShader(Shaders.Geometry, TH_VS | TH_PS);

				size_t Count = 0;
				for (auto* Base : Chunk)
				{
					if ((Static && !Base->Static) || Base->GetIndices().empty())
						continue;

					if (!System->PostGeometry(Base->GetMaterial(), true))
						continue;

					Base->Fill(Device, IndexBuffer, VertexBuffer);
					Device->Render.World.Identify();
					Device->Render.Transform = System->View.ViewProjection;
					Device->Render.TexCoord = Base->TexCoord;
					Device->SetVertexBuffer(VertexBuffer);
					Device->SetIndexBuffer(IndexBuffer, Graphics::Format::R32_Uint);
					Device->UpdateBuffer(Graphics::RenderBufferType::Render);
					Device->DrawIndexed((unsigned int)Base->GetIndices().size(), 0, 0);
					Count++;
				}

				return Count;
			}
			size_t SoftBody::RenderGeometryVoxels(Core::Timer* Time, const GeometryRenderer::Objects& Chunk)
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

				for (auto* Base : Chunk)
				{
					if (!Base->Static || Base->GetIndices().empty())
						continue;

					if (!System->PostGeometry(Base->GetMaterial(), true))
						continue;

					Base->Fill(Device, IndexBuffer, VertexBuffer);
					Device->Render.World.Identify();
					Device->Render.Transform.Identify();
					Device->Render.TexCoord = Base->TexCoord;
					Device->SetVertexBuffer(VertexBuffer);
					Device->SetIndexBuffer(IndexBuffer, Graphics::Format::R32_Uint);
					Device->UpdateBuffer(Graphics::RenderBufferType::Render);
					Device->DrawIndexed((unsigned int)Base->GetIndices().size(), 0, 0);

					Count++;
				}

				Device->SetShader(nullptr, TH_GS);
				return Count;
			}
			size_t SoftBody::RenderDepthLinear(Core::Timer* Time, const GeometryRenderer::Objects& Chunk)
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
				for (auto* Base : Chunk)
				{
					if (Base->GetIndices().empty())
						continue;

					if (!System->PostGeometry(Base->GetMaterial(), true))
						continue;

					Base->Fill(Device, IndexBuffer, VertexBuffer);
					Device->Render.World.Identify();
					Device->Render.Transform = System->View.ViewProjection;
					Device->Render.TexCoord = Base->TexCoord;
					Device->UpdateBuffer(Graphics::RenderBufferType::Render);
					Device->SetVertexBuffer(VertexBuffer);
					Device->SetIndexBuffer(IndexBuffer, Graphics::Format::R32_Uint);
					Device->DrawIndexed((unsigned int)Base->GetIndices().size(), 0, 0);

					Count++;
				}

				Device->SetTexture2D(nullptr, 1, TH_PS);
				return Count;
			}
			size_t SoftBody::RenderDepthCubic(Core::Timer* Time, const GeometryRenderer::Objects& Chunk, Compute::Matrix4x4* ViewProjection)
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
				for (auto* Base : Chunk)
				{
					auto* Transform = Base->GetEntity()->GetTransform();
					if (!Base->GetBody())
						continue;

					if (!System->PostGeometry(Base->GetMaterial(), true))
						continue;

					Base->Fill(Device, IndexBuffer, VertexBuffer);
					Device->Render.World.Identify();
					Device->Render.TexCoord = Base->TexCoord;
					Device->UpdateBuffer(Graphics::RenderBufferType::Render);
					Device->SetVertexBuffer(VertexBuffer);
					Device->SetIndexBuffer(IndexBuffer, Graphics::Format::R32_Uint);
					Device->DrawIndexed((unsigned int)Base->GetIndices().size(), 0, 0);

					Count++;
				}

				Device->SetShader(nullptr, TH_GS);
				Device->SetTexture2D(nullptr, 1, TH_PS);
				return Count;
			}

			Model::Model(Engine::RenderSystem* Lab) : GeometryRenderer(Lab)
			{
				TH_ASSERT_V(System != nullptr, "render system should be set");
				TH_ASSERT_V(System->GetDevice() != nullptr, "graphics device should be set");

				Graphics::GraphicsDevice* Device = System->GetDevice();
				DepthStencil = Device->GetDepthStencilState("less");
				BackRasterizer = Device->GetRasterizerState("cull-back");
				FrontRasterizer = Device->GetRasterizerState("cull-front");
				Blend = Device->GetBlendState("overwrite");
				Sampler = Device->GetSamplerState("trilinear-x16");
				Layout[0] = Device->GetInputLayout("vertex");
				Layout[1] = Device->GetInputLayout("vertex-instance");

				Shaders.Geometry = System->CompileShader("geometry/model/geometry");
				Shaders.Voxelize = System->CompileShader("geometry/model/voxelize", sizeof(Lighting::IVoxelBuffer));
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
			void Model::BatchGeometry(Components::Model* Base, Graphics::RenderBuffer::Instance& Data, GeometryRenderer::Batching& Batch)
			{
				auto* Drawable = Base->GetDrawable();
				if (!Drawable || (!Base->Static && !System->State.IsSet(RenderOpt::Static)))
					return;

				auto& World = Base->GetEntity()->GetBox();
				Data.TexCoord = Base->TexCoord;

				for (auto* Mesh : Drawable->Meshes)
				{
					Material* Source = Base->GetMaterial(Mesh);
					if (System->PostInstance(Source, Data))
					{
						Data.World = Mesh->World * World;
						Data.Transform = Data.World * System->View.ViewProjection;
						Batch.Emplace(Mesh, Source, Data);
					}
				}
			}
			size_t Model::CullGeometry(const Viewer& View, const GeometryRenderer::Objects& Chunk)
			{
				TH_ASSERT(System->GetPrimitives() != nullptr, 0, "primitive cache should be set");

				Graphics::ElementBuffer* Box[2];
				System->GetPrimitives()->GetBoxBuffers(Box);

				Graphics::GraphicsDevice* Device = System->GetDevice();
				Device->SetRasterizerState(BackRasterizer);
				Device->SetInputLayout(Layout[0]);
				Device->SetShader(nullptr, TH_PS);
				Device->SetShader(Shaders.Occlusion, TH_VS);

				size_t Count = 0;
				if (System->PreciseCulling)
				{
					for (auto* Base : Chunk)
					{
						auto* Drawable = Base->GetDrawable();
						if (!Drawable || Drawable->Meshes.empty() || !CullingBegin(Base))
							continue;

						auto& World = Base->GetEntity()->GetBox();
						for (auto* Mesh : Drawable->Meshes)
						{
							Device->Render.World = Mesh->World * World;
							Device->Render.Transform = Device->Render.World * View.ViewProjection;
							Device->UpdateBuffer(Graphics::RenderBufferType::Render);
							Device->DrawIndexed(Mesh);
						}
						CullingEnd();
						Count++;
					}
				}
				else
				{
					Device->SetVertexBuffer(Box[(size_t)BufferType::Vertex]);
					Device->SetIndexBuffer(Box[(size_t)BufferType::Index], Graphics::Format::R32_Uint);
					for (auto* Base : Chunk)
					{
						auto* Drawable = Base->GetDrawable();
						if (!Drawable || Drawable->Meshes.empty() || !CullingBegin(Base))
							continue;

						Device->Render.World = Base->GetEntity()->GetBox();
						Device->Render.Transform = Device->Render.World * View.ViewProjection;
						Device->UpdateBuffer(Graphics::RenderBufferType::Render);
						Device->DrawIndexed((unsigned int)Box[(size_t)BufferType::Index]->GetElements(), 0, 0);
						CullingEnd();
						Count++;
					}
				}

				return Count;
			}
			size_t Model::RenderGeometryResultBatched(Core::Timer* Time, const GeometryRenderer::Groups& Chunk)
			{
				TH_ASSERT(System->GetScene() != nullptr, 0, "scene should be set");

				SceneGraph* Scene = System->GetScene();
				Graphics::GraphicsDevice* Device = System->GetDevice();
				bool Static = System->State.IsSet(RenderOpt::Static);

				Device->SetDepthStencilState(DepthStencil);
				Device->SetBlendState(Blend);
				Device->SetRasterizerState(BackRasterizer);
				Device->SetInputLayout(Layout[1]);
				Device->SetSamplerState(Sampler, 1, 7, TH_PS);
				Device->SetShader(Shaders.Geometry, TH_VS | TH_PS);

				for (auto& Group : Chunk)
				{
					auto* Data = Group.second;
					System->PostGeometry(Data->MaterialData, true);
					Device->DrawIndexedInstanced(Data->DataBuffer, Data->GeometryBuffer, (unsigned int)Data->Instances.size());
				}

				static Graphics::ElementBuffer* VertexBuffers[2] = { nullptr, nullptr };
				Device->SetVertexBuffers(VertexBuffers, 2);
				return Chunk.size();
			}
			size_t Model::RenderGeometryVoxelsBatched(Core::Timer* Time, const GeometryRenderer::Groups& Chunk)
			{
				TH_ASSERT(System->GetScene() != nullptr, 0, "scene should be set");

				SceneGraph* Scene = System->GetScene();
				Graphics::GraphicsDevice* Device = System->GetDevice();
				Device->SetInputLayout(Layout[1]);
				Device->SetSamplerState(Sampler, 4, 6, TH_PS);
				Device->SetShader(Shaders.Voxelize, TH_VS | TH_PS | TH_GS);
				Lighting::SetVoxelBuffer(System, Shaders.Voxelize, 3);

				Viewer& View = System->View;
				for (auto& Group : Chunk)
				{
					auto* Data = Group.second;
					System->PostGeometry(Data->MaterialData, true);
					Device->DrawIndexedInstanced(Data->DataBuffer, Data->GeometryBuffer, (unsigned int)Data->Instances.size());
				}

				static Graphics::ElementBuffer* VertexBuffers[2] = { nullptr, nullptr };
				Device->SetVertexBuffers(VertexBuffers, 2);
				Device->SetShader(nullptr, TH_GS);
				return Chunk.size();
			}
			size_t Model::RenderDepthLinearBatched(Core::Timer* Time, const GeometryRenderer::Groups& Chunk)
			{
				TH_ASSERT(System->GetScene() != nullptr, 0, "scene should be set");

				SceneGraph* Scene = System->GetScene();
				Graphics::GraphicsDevice* Device = System->GetDevice();
				Device->SetDepthStencilState(DepthStencil);
				Device->SetBlendState(Blend);
				Device->SetRasterizerState(FrontRasterizer);
				Device->SetInputLayout(Layout[1]);
				Device->SetSamplerState(Sampler, 1, 1, TH_PS);
				Device->SetShader(Shaders.Depth.Linear, TH_VS | TH_PS);

				for (auto& Group : Chunk)
				{
					auto* Data = Group.second;
					System->PostGeometry(Data->MaterialData, true);
					Device->DrawIndexedInstanced(Data->DataBuffer, Data->GeometryBuffer, (unsigned int)Data->Instances.size());
				}

				static Graphics::ElementBuffer* VertexBuffers[2] = { nullptr, nullptr };
				Device->SetVertexBuffers(VertexBuffers, 2);
				Device->SetTexture2D(nullptr, 1, TH_PS);
				return Chunk.size();
			}
			size_t Model::RenderDepthCubicBatched(Core::Timer* Time, const GeometryRenderer::Groups& Chunk, Compute::Matrix4x4* ViewProjection)
			{
				TH_ASSERT(System->GetScene() != nullptr, 0, "scene should be set");

				SceneGraph* Scene = System->GetScene();
				Graphics::GraphicsDevice* Device = System->GetDevice();
				Device->SetDepthStencilState(DepthStencil);
				Device->SetBlendState(Blend);
				Device->SetRasterizerState(FrontRasterizer);
				Device->SetInputLayout(Layout[1]);
				Device->SetSamplerState(Sampler, 1, 1, TH_PS);
				Device->SetShader(Shaders.Depth.Cubic, TH_VS | TH_PS | TH_GS);
				Device->SetBuffer(Shaders.Depth.Cubic, 3, TH_VS | TH_PS | TH_GS);
				Device->UpdateBuffer(Shaders.Depth.Cubic, ViewProjection);

				for (auto& Group : Chunk)
				{
					auto* Data = Group.second;
					System->PostGeometry(Data->MaterialData, true);
					Device->DrawIndexedInstanced(Data->DataBuffer, Data->GeometryBuffer, (unsigned int)Data->Instances.size());
				}

				static Graphics::ElementBuffer* VertexBuffers[2] = { nullptr, nullptr };
				Device->SetVertexBuffers(VertexBuffers, 2);
				Device->SetTexture2D(nullptr, 1, TH_PS);
				Device->SetShader(nullptr, TH_GS);
				return Chunk.size();
			}

			Skin::Skin(Engine::RenderSystem* Lab) : GeometryRenderer(Lab)
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
				Shaders.Voxelize = System->CompileShader("geometry/skin/voxelize", sizeof(Lighting::IVoxelBuffer));
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
			size_t Skin::CullGeometry(const Viewer& View, const GeometryRenderer::Objects& Chunk)
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
					for (auto* Base : Chunk)
					{
						auto* Drawable = Base->GetDrawable();
						if (!Drawable || Drawable->Meshes.empty())
							continue;

						if (!CullingBegin(Base))
							continue;

						Device->Animation.Animated = (float)!Base->GetDrawable()->Joints.empty();
						if (Device->Animation.Animated > 0)
							memcpy(Device->Animation.Offsets, Base->Skeleton.Transform, 96 * sizeof(Compute::Matrix4x4));
						Device->UpdateBuffer(Graphics::RenderBufferType::Animation);

						auto& World = Base->GetEntity()->GetBox();
						for (auto* Mesh : Drawable->Meshes)
						{
							Device->Render.World = Mesh->World * World;
							Device->Render.Transform = Device->Render.World * View.ViewProjection;
							Device->UpdateBuffer(Graphics::RenderBufferType::Render);
							Device->DrawIndexed(Mesh);
						}

						CullingEnd();
						Count++;
					}
				}
				else
				{
					Device->SetVertexBuffer(Box[(size_t)BufferType::Vertex]);
					Device->SetIndexBuffer(Box[(size_t)BufferType::Index], Graphics::Format::R32_Uint);
					for (auto* Base : Chunk)
					{
						auto* Drawable = Base->GetDrawable();
						if (!Drawable || Drawable->Meshes.empty())
							continue;

						if (!CullingBegin(Base))
							continue;

						Device->Animation.Animated = (float)false;
						Device->Render.World = Base->GetEntity()->GetBox();
						Device->Render.Transform = Device->Render.World * View.ViewProjection;
						Device->UpdateBuffer(Graphics::RenderBufferType::Animation);
						Device->UpdateBuffer(Graphics::RenderBufferType::Render);
						Device->DrawIndexed((unsigned int)Box[(size_t)BufferType::Index]->GetElements(), 0, 0);
						CullingEnd();

						Count++;
					}
				}

				return Count;
			}
			size_t Skin::RenderGeometryResult(Core::Timer* Time, const GeometryRenderer::Objects& Chunk)
			{
				TH_ASSERT(System->GetScene() != nullptr, 0, "scene should be set");

				SceneGraph* Scene = System->GetScene();
				Graphics::GraphicsDevice* Device = System->GetDevice();
				bool Static = System->State.IsSet(RenderOpt::Static);

				Device->SetDepthStencilState(DepthStencil);
				Device->SetBlendState(Blend);
				Device->SetRasterizerState(BackRasterizer);
				Device->SetInputLayout(Layout);
				Device->SetSamplerState(Sampler, 1, 7, TH_PS);
				Device->SetShader(Shaders.Geometry, TH_VS | TH_PS);

				size_t Count = 0;
				for (auto* Base : Chunk)
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

					for (auto* Mesh : Drawable->Meshes)
					{
						if (!System->PostGeometry(Base->GetMaterial(Mesh), true))
							continue;

						Device->Render.World = Mesh->World * World;
						Device->Render.Transform = Device->Render.World * System->View.ViewProjection;
						Device->UpdateBuffer(Graphics::RenderBufferType::Render);
						Device->DrawIndexed(Mesh);
					}

					Count++;
				}

				return Count;
			}
			size_t Skin::RenderGeometryVoxels(Core::Timer* Time, const GeometryRenderer::Objects& Chunk)
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

				for (auto* Base : Chunk)
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

					for (auto* Mesh : Drawable->Meshes)
					{
						if (!System->PostGeometry(Base->GetMaterial(Mesh), true))
							continue;

						Device->Render.Transform = Device->Render.World = Mesh->World * World;
						Device->UpdateBuffer(Graphics::RenderBufferType::Render);
						Device->DrawIndexed(Mesh);
					}

					Count++;
				}

				Device->SetShader(nullptr, TH_GS);
				return Count;
			}
			size_t Skin::RenderDepthLinear(Core::Timer* Time, const GeometryRenderer::Objects& Chunk)
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
				for (auto* Base : Chunk)
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

					for (auto* Mesh : Drawable->Meshes)
					{
						if (!System->PostGeometry(Base->GetMaterial(Mesh), true))
							continue;

						Device->Render.World = Mesh->World * World;
						Device->Render.Transform = Device->Render.World * System->View.ViewProjection;
						Device->UpdateBuffer(Graphics::RenderBufferType::Render);
						Device->DrawIndexed(Mesh);
					}

					Count++;
				}

				Device->SetTexture2D(nullptr, 1, TH_PS);
				return Count;
			}
			size_t Skin::RenderDepthCubic(Core::Timer* Time, const GeometryRenderer::Objects& Chunk, Compute::Matrix4x4* ViewProjection)
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
				for (auto* Base : Chunk)
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

					for (auto* Mesh : Drawable->Meshes)
					{
						if (!System->PostGeometry(Base->GetMaterial(Mesh), true))
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

			Emitter::Emitter(RenderSystem* Lab) : GeometryRenderer(Lab)
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
			size_t Emitter::RenderGeometryResult(Core::Timer* Time, const GeometryRenderer::Objects& Chunk)
			{
				TH_ASSERT(System->GetScene() != nullptr, 0, "scene should be set");

				SceneGraph* Scene = System->GetScene();
				Graphics::GraphicsDevice* Device = System->GetDevice();
				Graphics::Shader* BaseShader = nullptr;
				Graphics::PrimitiveTopology T = Device->GetPrimitiveTopology();
				Viewer& View = System->View;
				bool Static = System->State.IsSet(RenderOpt::Static);

				if (System->State.IsSet(RenderOpt::Additive))
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
				Device->SetVertexBuffer(nullptr);

				size_t Count = 0;
				for (auto* Base : Chunk)
				{
					if ((Static && !Base->Static) || !Base->GetBuffer())
						continue;

					if (!System->PostGeometry(Base->GetMaterial(), true))
						continue;

					Device->Render.World = View.Projection;
					Device->Render.Transform = (Base->QuadBased ? View.View : View.ViewProjection);
					Device->Render.TexCoord = Base->GetEntity()->GetTransform()->Forward();
					if (Base->Connected)
						Device->Render.Transform = Base->GetEntity()->GetBox() * Device->Render.Transform;

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
			size_t Emitter::RenderDepthLinear(Core::Timer* Time, const GeometryRenderer::Objects& Chunk)
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
				Device->SetVertexBuffer(nullptr);

				size_t Count = 0;
				for (auto* Base : Chunk)
				{
					if (!Base->GetBuffer() || !System->PostGeometry(Base->GetMaterial(), true))
						continue;

					Device->Render.World = View.Projection;
					Device->Render.Transform = (Base->QuadBased ? View.View : View.ViewProjection);
					if (Base->Connected)
						Device->Render.Transform = Base->GetEntity()->GetBox() * Device->Render.Transform;

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
			size_t Emitter::RenderDepthCubic(Core::Timer* Time, const GeometryRenderer::Objects& Chunk, Compute::Matrix4x4* ViewProjection)
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
				Device->SetVertexBuffer(nullptr);
				Device->SetSamplerState(Sampler, 1, 1, TH_PS);
				Device->SetBuffer(Shaders.Depth.Quad, 3, TH_VS | TH_PS | TH_GS);
				Device->UpdateBuffer(Shaders.Depth.Quad, &Depth);

				size_t Count = 0;
				for (auto* Base : Chunk)
				{
					if (!Base->GetBuffer() || !System->PostGeometry(Base->GetMaterial(), true))
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

			Decal::Decal(RenderSystem* Lab) : GeometryRenderer(Lab)
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
			size_t Decal::RenderGeometryResult(Core::Timer* Time, const GeometryRenderer::Objects& Chunk)
			{
				TH_ASSERT(System->GetScene() != nullptr, 0, "scene should be set");

				Graphics::MultiRenderTarget2D* MRT = System->GetMRT(TargetType::Main);
				Graphics::GraphicsDevice* Device = System->GetDevice();
				SceneGraph* Scene = System->GetScene();
				bool Static = System->State.IsSet(RenderOpt::Static);

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
				Device->SetVertexBuffer(Box[(size_t)BufferType::Vertex]);
				Device->SetIndexBuffer(Box[(size_t)BufferType::Index], Graphics::Format::R32_Uint);

				size_t Count = 0;
				for (auto* Base : Chunk)
				{
					if ((Static && !Base->Static) || !System->PostGeometry(Base->GetMaterial(), true))
						continue;

					Device->Render.Transform = Base->GetEntity()->GetBox() * System->View.ViewProjection;
					Device->Render.World = Device->Render.Transform.Inv();
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
				System->FreeShader(Shaders.Voxelize);
				System->FreeShader(Shaders.Surface);

				for (unsigned int i = 0; i < 2; i++)
				{
					System->FreeShader(Shaders.Line[i]);
					System->FreeShader(Shaders.Spot[i]);
					System->FreeShader(Shaders.Point[i]);
					System->FreeShader(Shaders.Ambient[i]);
				}

				TH_RELEASE(Voxels.PBuffer);
				TH_RELEASE(Voxels.SBuffer);
				TH_RELEASE(Voxels.LBuffer);
				TH_RELEASE(Surfaces.Subresource);
				TH_RELEASE(Surfaces.Input);
				TH_RELEASE(Surfaces.Output);
				TH_RELEASE(Surfaces.Merger);
				TH_RELEASE(SkyBase);
				TH_RELEASE(SkyMap);
				TH_RELEASE(LightingMap);
			}
			void Lighting::Deserialize(ContentManager* Content, Core::Schema* Node)
			{
				TH_ASSERT_V(Content != nullptr, "content manager should be set");
				TH_ASSERT_V(Node != nullptr, "schema should be set");

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
				NMake::Unpack(Node->Find("shadow-distance"), &Shadows.Distance);
				NMake::Unpack(Node->Find("sf-size"), &Surfaces.Size);
				NMake::Unpack(Node->Find("gi"), &EnableGI);
			}
			void Lighting::Serialize(ContentManager* Content, Core::Schema* Node)
			{
				TH_ASSERT_V(Content != nullptr, "content manager should be set");
				TH_ASSERT_V(Node != nullptr, "schema should be set");

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
				NMake::Pack(Node->Set("shadow-distance"), Shadows.Distance);
				NMake::Pack(Node->Set("sf-size"), Surfaces.Size);
				NMake::Pack(Node->Set("gi"), EnableGI);
			}
			void Lighting::ResizeBuffers()
			{
				TH_CLEAR(LightingMap);
			}
			void Lighting::BeginPass()
			{
				if (System->State.Is(RenderState::Depth_Linear) || System->State.Is(RenderState::Depth_Cubic))
					return;

				auto& Lines = System->GetScene()->GetComponents<Components::LineLight>();
				Lights.Illuminators.Push(System);
				Lights.Surfaces.Push(System);
				Lights.Points.Push(System);
				Lights.Spots.Push(System);
				Lights.Lines = &Lines;
			}
			void Lighting::EndPass()
			{
				if (System->State.Is(RenderState::Depth_Linear) || System->State.Is(RenderState::Depth_Cubic))
					return;

				Lights.Spots.Pop();
				Lights.Points.Pop();
				Lights.Surfaces.Pop();
				Lights.Illuminators.Pop();
			}
			void Lighting::RenderResultBuffers()
			{
				SceneGraph* Scene = System->GetScene();
				Graphics::MultiRenderTarget2D* MRT = System->GetMRT(TargetType::Main);
				Graphics::RenderTarget2D* RT = (System->State.IsSubpass() ? Surfaces.Input : System->GetRT(TargetType::Main));
				State.Backcull = true;

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
				State.Device->SetVertexBuffer(Cube[(size_t)BufferType::Vertex]);
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
				if (!EnableGI || Lights.Illuminators.Top().empty())
					return;

				Graphics::Texture3D* In[3], *Out[3];
				if (!State.Scene->GetVoxelBuffer(In, Out))
					return;

				auto& Buffers = State.Scene->GetVoxelsMapping();
				auto& Top = Lights.Illuminators.Top();
				bool MustRedeploy = true;
				uint64_t Counter = 0;

				for (auto It = Top.begin(); It != Top.end(); ++It)
				{
					if (Counter >= Buffers.size())
						break;

					auto& Buffer = Buffers[Counter++];
					auto* Last = (Components::Illuminator*)Buffer.second;
					auto* Light = *It;

					if (!Light->Regenerate)
						Light->Regenerate = (Last != Light);

					if (Last != nullptr && Light->Regenerate)
						Last->Regenerate = true;

					Light->VoxelMap = Buffer.first;
					Buffer.second = Light;

					if (!GetIlluminator(&VoxelBuffer, Light))
						continue;

					bool Inside = Compute::Geometric::HasPointIntersectedCube(VoxelBuffer.Center, VoxelBuffer.Scale, System->View.Position);
					auto& Delay = (Inside ? Light->Inside : Light->Outside);
					if (!Light->Regenerate && !Delay.TickEvent(Time->GetElapsedTime()))
						continue;

					Voxels.LightBuffer = Light->VoxelMap;
					Light->Regenerate = false;

                    unsigned int Size = (unsigned int)State.Scene->GetConf().VoxelsSize;
					State.Device->ClearWritable(In[(size_t)VoxelType::Diffuse]);
					State.Device->ClearWritable(In[(size_t)VoxelType::Normal]);
					State.Device->ClearWritable(In[(size_t)VoxelType::Surface]);
					State.Device->SetTargetRect(Size, Size);
					State.Device->SetDepthStencilState(DepthStencilNone);
					State.Device->SetBlendState(BlendOverwrite);
					State.Device->SetRasterizerState(NoneRasterizer);
					State.Device->SetWriteable(In, 1, 3, false);

					Compute::Matrix4x4 Offset = Compute::Matrix4x4::CreateTranslatedScale(VoxelBuffer.Center, VoxelBuffer.Scale);
					System->SetView(Offset, System->View.Projection, VoxelBuffer.Center, 90.0f, 1.0f, 0.1f, GetDominant(VoxelBuffer.Scale) * 2.0f, RenderCulling::Point);
					System->Render(Time, RenderState::Geometry_Voxels, RenderOpt::None);
					System->RestoreViewBuffer(nullptr);

					State.Device->SetWriteable(Out, 1, 3, false);
					State.Device->GenerateMips(Voxels.LightBuffer);
				}
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
						System->SetView(Light->View[j] = Compute::Matrix4x4::CreateLookAt(Face, Position), Light->Projection, Position, 90.0f, 1.0f, 0.1f, Light->GetSize().Radius, RenderCulling::Spot);
						System->Render(Time, RenderState::Geometry_Result, Light->StaticMask ? RenderOpt::Static : RenderOpt::None);
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
				auto& Buffers = State.Scene->GetPointsMapping(); uint64_t Counter = 0;
				for (auto* Light : Lights.Points.Top())
				{
					if (Counter >= Buffers.size())
						break;

					Light->DepthMap = nullptr;
					if (!Light->Shadow.Enabled)
						continue;

					CubicDepthMap* Target = Buffers[Counter++];
					Light->GenerateOrigin();
					Light->DepthMap = Target;

					State.Device->SetTarget(Target);
					State.Device->ClearDepth(Target);
					System->SetView(Compute::Matrix4x4::Identity(), Light->Projection, Light->GetEntity()->GetTransform()->GetPosition(), 90.0f, 1.0f, 0.1f, Light->Shadow.Distance, RenderCulling::Point);
					System->Render(Time, RenderState::Depth_Cubic, RenderOpt::None);
				}
			}
			void Lighting::RenderSpotShadowMaps(Core::Timer* Time)
			{
				auto& Buffers = State.Scene->GetSpotsMapping(); uint64_t Counter = 0;
				for (auto* Light : Lights.Spots.Top())
				{
					if (Counter >= Buffers.size())
						break;

					Light->DepthMap = nullptr;
					if (!Light->Shadow.Enabled)
						continue;

					LinearDepthMap* Target = Buffers[Counter++];
					Light->GenerateOrigin();
					Light->DepthMap = Target;

					State.Device->SetTarget(Target);
					State.Device->ClearDepth(Target);
					System->SetView(Light->View, Light->Projection, Light->GetEntity()->GetTransform()->GetPosition(), Light->Cutoff, 1.0f, 0.1f, Light->Shadow.Distance, RenderCulling::Spot);
					System->Render(Time, RenderState::Depth_Linear, RenderOpt::None);
				}
			}
			void Lighting::RenderLineShadowMaps(Core::Timer* Time)
			{
				auto& Buffers = State.Scene->GetLinesMapping(); uint64_t Counter = 0;
				for (auto It = Lights.Lines->Begin(); It != Lights.Lines->End(); ++It)
				{
					auto* Light = (Components::LineLight*)*It;
					if (Counter >= Buffers.size())
						break;

					Light->DepthMap = nullptr;
					if (!Light->Shadow.Enabled || Light->Shadow.Cascades < 1 || Light->Shadow.Cascades > 6)
						continue;

					CascadedDepthMap*& Target = Buffers[Counter++];
					if (!Target || Target->size() < Light->Shadow.Cascades)
						State.Scene->GenerateDepthCascades(&Target, Light->Shadow.Cascades);

					Light->GenerateOrigin();
					Light->DepthMap = Target;

					for (size_t i = 0; i < Target->size(); i++)
					{
						LinearDepthMap* Cascade = (*Target)[i];
						State.Device->SetTarget(Cascade);
						State.Device->ClearDepth(Cascade);

						float Distance = Light->Shadow.Distance[i];
						System->SetView(Light->View[i], Light->Projection[i], 0.0f, 90.0f, 1.0f, -System->View.FarPlane, System->View.FarPlane, RenderCulling::Line);
						System->Render(Time, RenderState::Depth_Linear, RenderOpt::None);
					}
				}

				System->RestoreViewBuffer(nullptr);
			}
			void Lighting::RenderSurfaceLights()
			{
				Graphics::ElementBuffer* Cube[2];
				System->GetPrimitives()->GetCubeBuffers(Cube);

				if (!System->State.IsSubpass())
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

					if (GetPointLight(&PointLight, Light, Position, Scale, false))
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

					if (GetSpotLight(&SpotLight, Light, Position, Scale, false))
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
				State.Device->SetVertexBuffer(System->GetPrimitives()->GetQuad());

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
				Graphics::Texture3D* In[3], *Out[3];
				if (!State.Scene->GetVoxelBuffer(In, Out))
					return;

				uint32_t X = (uint32_t)(VoxelBuffer.Size.X / 8.0f);
				uint32_t Y = (uint32_t)(VoxelBuffer.Size.Y / 8.0f);
				uint32_t Z = (uint32_t)(VoxelBuffer.Size.Z / 8.0f);

				State.Device->ClearWritable(Voxels.LightBuffer);
				State.Device->SetSamplerState(nullptr, 1, 6, TH_CS);
				State.Device->SetWriteable(Out, 1, 3, false);
				State.Device->SetWriteable(&Voxels.LightBuffer, 1, 1, true);
				State.Device->SetTexture3D(In[(size_t)VoxelType::Diffuse], 2, TH_CS);
				State.Device->SetTexture3D(In[(size_t)VoxelType::Normal], 3, TH_CS);
				State.Device->SetTexture3D(In[(size_t)VoxelType::Surface], 4, TH_CS);

				size_t PointLightsCount = GeneratePointLights();
				if (PointLightsCount > 0)
				{
					State.Device->UpdateBuffer(Voxels.PBuffer, Voxels.PArray.data(), PointLightsCount * sizeof(IPointLight));
					State.Device->SetStructureBuffer(Voxels.PBuffer, 5, TH_CS);
				}

				size_t SpotLightsCount = GenerateSpotLights();
				if (SpotLightsCount > 0)
				{
					State.Device->UpdateBuffer(Voxels.SBuffer, Voxels.SArray.data(), SpotLightsCount * sizeof(ISpotLight));
					State.Device->SetStructureBuffer(Voxels.SBuffer, 6, TH_CS);
				}

				size_t LineLightsCount = GenerateLineLights();
				if (LineLightsCount > 0)
				{
					State.Device->UpdateBuffer(Voxels.LBuffer, Voxels.LArray.data(), LineLightsCount * sizeof(ILineLight));
					State.Device->SetStructureBuffer(Voxels.LBuffer, 7, TH_CS);
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
				if (!EnableGI || System->State.IsSubpass() || Lights.Illuminators.Top().empty())
					return;

				Graphics::ElementBuffer* Cube[2];
				System->GetPrimitives()->GetCubeBuffers(Cube);
				State.Backcull = true;

				Graphics::MultiRenderTarget2D* MRT = System->GetMRT(TargetType::Main);
				Graphics::RenderTarget2D* RT = System->GetRT(TargetType::Secondary);
				State.Device->CopyTarget(MRT, 0, RT, 0);
				State.Device->SetDepthStencilState(DepthStencilLess);
				State.Device->SetRasterizerState(BackRasterizer);
				State.Device->SetSamplerState(WrapSampler, 1, 6, TH_PS);
				State.Device->SetTexture2D(RT->GetTarget(), 1, TH_PS);
				State.Device->SetShader(Shaders.Ambient[1], TH_VS | TH_PS);
				State.Device->SetBuffer(Shaders.Ambient[1], 3, TH_VS | TH_PS);
				State.Device->SetVertexBuffer(Cube[(size_t)BufferType::Vertex]);
				State.Device->SetIndexBuffer(Cube[(size_t)BufferType::Index], Graphics::Format::R32_Uint);

				Compute::Vector3 Position, Scale;
				for (auto* Light : Lights.Illuminators.Top())
				{
					if (!GetIlluminator(&VoxelBuffer, Light))
						continue;

					GetLightCulling(Light, 0.0f, &Position, &Scale);
					VoxelBuffer.Transform = Compute::Matrix4x4::CreateTranslatedScale(Position, Scale) * System->View.ViewProjection;

					State.Device->SetTexture3D(Light->VoxelMap, 5, TH_PS);
					State.Device->UpdateBuffer(Shaders.Ambient[1], &VoxelBuffer);
					State.Device->DrawIndexed((unsigned int)Cube[(size_t)BufferType::Index]->GetElements(), 0, 0);
				}

				State.Device->SetTexture2D(System->GetRT(TargetType::Main)->GetTarget(), 1, TH_PS);
				State.Device->SetTexture3D(nullptr, 5, TH_PS);
				State.Device->SetVertexBuffer(System->GetPrimitives()->GetQuad());

				if (!State.Backcull)
					State.Device->SetRasterizerState(BackRasterizer);
			}
			void Lighting::RenderAmbient()
			{
				Graphics::MultiRenderTarget2D* MRT = System->GetMRT(TargetType::Main);
				Graphics::RenderTarget2D* RT = (System->State.IsSubpass() ? Surfaces.Output : System->GetRT(TargetType::Secondary));
				State.Device->CopyTarget(MRT, 0, RT, 0);
				State.Device->CopyTexture2D(RT, 0, &LightingMap);
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
				I.Size = (unsigned int)Surfaces.Size;

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
				if (!Voxels.PBuffer)
					GenerateLightBuffers();

				Compute::Vector3 Offset = 1.0f;
				Offset.X = (VoxelBuffer.Center.X > 0.0f ? 1.0f : -1.0f);
				Offset.Y = (VoxelBuffer.Center.Y > 0.0f ? 1.0f : -1.0f);
				Offset.Z = (VoxelBuffer.Center.Z > 0.0f ? 1.0f : -1.0f);

				size_t Count = 0;
				for (auto* Light : Lights.Points.Top())
				{
					if (Count >= Voxels.MaxLights)
						break;

					auto* Base = Light->GetEntity();
					Compute::Vector3 Position(Base->GetTransform()->GetPosition());
					Compute::Vector3 Scale(Base->GetRadius());
					Position *= Offset;

					GetPointLight(&Voxels.PArray[Count++], Light, Position, Scale, true);
				}

				VoxelBuffer.Lights.X = (float)Count;
				return Count;
			}
			size_t Lighting::GenerateSpotLights()
			{
				if (!Voxels.SBuffer)
					GenerateLightBuffers();

				Compute::Vector3 Offset = 1.0f;
				Offset.X = (VoxelBuffer.Center.X > 0.0f ? 1.0f : -1.0f);
				Offset.Y = (VoxelBuffer.Center.Y > 0.0f ? 1.0f : -1.0f);
				Offset.Z = (VoxelBuffer.Center.Z > 0.0f ? 1.0f : -1.0f);

				size_t Count = 0;
				for (auto* Light : Lights.Spots.Top())
				{
					if (Count >= Voxels.MaxLights)
						break;

					auto* Base = Light->GetEntity();
					Compute::Vector3 Position(Base->GetTransform()->GetPosition());
					Compute::Vector3 Scale(Base->GetRadius());
					Position *= Offset;

					GetSpotLight(&Voxels.SArray[Count++], Light, Position, Scale, true);
				}

				VoxelBuffer.Lights.Y = (float)Count;
				return Count;
			}
			size_t Lighting::GenerateLineLights()
			{
				if (!Voxels.LBuffer)
					GenerateLightBuffers();

				size_t Count = 0;
				for (auto It = Lights.Lines->Begin(); It != Lights.Lines->End(); ++It)
				{
					auto* Light = (Components::LineLight*)*It;
					if (Count >= Voxels.MaxLights)
						break;

					GetLineLight(&Voxels.LArray[Count++], Light);
				}

				VoxelBuffer.Lights.Z = (float)Count;
				return Count;
			}
			size_t Lighting::RenderPass(Core::Timer* Time)
			{
				TH_ASSERT(System->GetScene() != nullptr, 0, "scene should be set");
				if (System->State.IsSet(RenderOpt::Additive))
					return 0;

				State.Device = System->GetDevice();
				State.Scene = System->GetScene();

				if (System->State.Is(RenderState::Geometry_Result))
				{
					if (!System->State.IsSubpass() && !System->State.IsSet(RenderOpt::Transparent))
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
						RenderVoxelMap(Time);
					}

					RenderResultBuffers();
					System->RestoreOutput();
				}
				else if (System->State.Is(RenderState::Geometry_Voxels))
					RenderLuminance();

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

				Dest->Transform = Compute::Matrix4x4::CreateTranslatedScale(Position, Scale) * System->View.ViewProjection;
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
			bool Lighting::GetPointLight(IPointLight* Dest, Component* Src, Compute::Vector3& Position, Compute::Vector3& Scale, bool Reposition)
			{
				Components::PointLight* Light = (Components::PointLight*)Src;
				auto* Entity = Light->GetEntity();
				auto* Transform = Entity->GetTransform();
				auto& Size = Light->GetSize();

				Dest->Transform = Compute::Matrix4x4::CreateTranslatedScale(Position, Scale) * System->View.ViewProjection;
				Dest->Position = (Reposition ? Position : Transform->GetPosition());
				Dest->Lighting = Light->Diffuse.Mul(Light->Emission);
				Dest->Attenuation.X = Size.C1;
				Dest->Attenuation.Y = Size.C2;
				Dest->Range = Size.Radius;

				if (!Light->Shadow.Enabled || !Light->DepthMap)
				{
					Dest->Softness = 0.0f;
					return false;
				}

				Dest->Softness = Light->Shadow.Softness <= 0 ? 0 : (float)State.Scene->GetConf().PointsSize / Light->Shadow.Softness;
				Dest->Bias = Light->Shadow.Bias;
				Dest->Distance = Light->Shadow.Distance;
				Dest->Iterations = (float)Light->Shadow.Iterations;
				Dest->Umbra = Light->Disperse;
				return true;
			}
			bool Lighting::GetSpotLight(ISpotLight* Dest, Component* Src, Compute::Vector3& Position, Compute::Vector3& Scale, bool Reposition)
			{
				Components::SpotLight* Light = (Components::SpotLight*)Src;
				auto* Entity = Light->GetEntity();
				auto* Transform = Entity->GetTransform();
				auto& Size = Light->GetSize();

				Dest->Transform = Compute::Matrix4x4::CreateTranslatedScale(Position, Scale) * System->View.ViewProjection;
				Dest->ViewProjection = Light->View * Light->Projection;
				Dest->Direction = Transform->GetRotation().dDirection();
				Dest->Position = (Reposition ? Position : Transform->GetPosition());
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

				Dest->Softness = Light->Shadow.Softness <= 0 ? 0 : (float)State.Scene->GetConf().SpotsSize / Light->Shadow.Softness;
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

				Dest->Softness = Light->Shadow.Softness <= 0 ? 0 : (float)State.Scene->GetConf().LinesSize / Light->Shadow.Softness;
				Dest->Iterations = (float)Light->Shadow.Iterations;
				Dest->Bias = Light->Shadow.Bias;
				Dest->Cascades = (float)std::min(Light->Shadow.Cascades, (uint32_t)Light->DepthMap->size());

				size_t Size = (size_t)Dest->Cascades;
				for (size_t i = 0; i < Size; i++)
					Dest->ViewProjection[i] = Light->View[i] * Light->Projection[i];

				return Size > 0;
			}
			bool Lighting::GetIlluminator(IVoxelBuffer* Dest, Component* Src)
			{
				auto* Light = (Components::Illuminator*)Src;
				if (!Light->VoxelMap)
					return false;

				auto& Conf = State.Scene->GetConf();
				auto* Transform = Light->GetEntity()->GetTransform();
				VoxelBuffer.Center = Transform->GetPosition();
				VoxelBuffer.Scale = Transform->GetScale();
				VoxelBuffer.Mips = (float)Conf.VoxelsMips;
				VoxelBuffer.Size = (float)Conf.VoxelsSize;
				VoxelBuffer.RayStep = Light->RayStep;
				VoxelBuffer.MaxSteps = Light->MaxSteps;
				VoxelBuffer.Distance = Light->Distance;
				VoxelBuffer.Radiance = Light->Radiance;
				VoxelBuffer.Length = Light->Length;
				VoxelBuffer.Margin = Light->Margin;
				VoxelBuffer.Offset = Light->Offset;
				VoxelBuffer.Angle = Light->Angle;
				VoxelBuffer.Occlusion = Light->Occlusion;
				VoxelBuffer.Specular = Light->Specular;
				VoxelBuffer.Bleeding = Light->Bleeding;

				return true;
			}
			void Lighting::GetLightCulling(Component* Src, float Range, Compute::Vector3* Position, Compute::Vector3* Scale)
			{
				auto* Transform = Src->GetEntity()->GetTransform();
				*Position = Transform->GetPosition();
				*Scale = (Range > 0.0f ? Range : Transform->GetScale());

				bool Front = Compute::Geometric::HasPointIntersectedCube(*Position, Scale->Mul(1.01f), System->View.Position);
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
				F.ElementCount = (unsigned int)Voxels.MaxLights;
				F.ElementWidth = sizeof(IPointLight);
				F.StructureByteStride = F.ElementWidth;

				TH_RELEASE(Voxels.PBuffer);
				Voxels.PBuffer = State.Device->CreateElementBuffer(F);
				Voxels.PArray.resize(Voxels.MaxLights);

				F.ElementWidth = sizeof(ISpotLight);
				F.StructureByteStride = F.ElementWidth;
				TH_RELEASE(Voxels.SBuffer);
				Voxels.SBuffer = State.Device->CreateElementBuffer(F);
				Voxels.SArray.resize(Voxels.MaxLights);

				F.ElementWidth = sizeof(ILineLight);
				F.StructureByteStride = F.ElementWidth;
				TH_RELEASE(Voxels.LBuffer);
				Voxels.LBuffer = State.Device->CreateElementBuffer(F);
				Voxels.LArray.resize(Voxels.MaxLights);
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

				Shader = System->CompileShader("postprocess/transparency", sizeof(RenderData));
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
			size_t Transparency::RenderPass(Core::Timer* Time)
			{
				TH_ASSERT(System->GetScene() != nullptr, 0, "scene should be set");
				if (!System->State.Is(RenderState::Geometry_Result) || System->State.IsSet(RenderOpt::Transparent) || System->State.IsSet(RenderOpt::Additive))
					return 0;

				SceneGraph* Scene = System->GetScene();
				if (System->HasCategory(GeoCategory::Additive))
					System->Render(Time, RenderState::Geometry_Result, System->State.GetOpts() | RenderOpt::Additive);

				if (!System->HasCategory(GeoCategory::Transparent))
					return 0;

				Graphics::MultiRenderTarget2D* MainMRT = System->GetMRT(TargetType::Main);
				Graphics::MultiRenderTarget2D* MRT = (System->State.IsSubpass() ? Merger : System->GetMRT(TargetType::Secondary));
				Graphics::RenderTarget2D* RT = (System->State.IsSubpass() ? Input : System->GetRT(TargetType::Main));
				Graphics::GraphicsDevice* Device = System->GetDevice();
				RenderData.Mips = (System->State.IsSubpass() ? MipLevels[(size_t)TargetType::Secondary] : MipLevels[(size_t)TargetType::Main]);

				Scene->SwapMRT(TargetType::Main, MRT);
				Scene->SetMRT(TargetType::Main, true);
				System->Render(Time, RenderState::Geometry_Result, System->State.GetOpts() | RenderOpt::Transparent);
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
				Device->SetVertexBuffer(System->GetPrimitives()->GetQuad());
				Device->UpdateBuffer(Graphics::RenderBufferType::Render);
				Device->Draw(6, 0);
				Device->FlushTexture(1, 8, TH_PS);
				System->RestoreOutput();
				return 1;
			}

			SSR::SSR(RenderSystem* Lab) : EffectRenderer(Lab)
			{
				Shaders.Reflectance = CompileEffect("postprocess/reflectance", sizeof(Reflectance));
				Shaders.Gloss[0] = CompileEffect("postprocess/gloss-x", sizeof(Gloss));
				Shaders.Gloss[1] = CompileEffect("postprocess/gloss-y");
				Shaders.Additive = CompileEffect("postprocess/additive");
			}
			void SSR::Deserialize(ContentManager* Content, Core::Schema* Node)
			{
				TH_ASSERT_V(Content != nullptr, "content manager should be set");
				TH_ASSERT_V(Node != nullptr, "schema should be set");

				NMake::Unpack(Node->Find("samples-1"), &Reflectance.Samples);
				NMake::Unpack(Node->Find("samples-2"), &Gloss.Samples);
				NMake::Unpack(Node->Find("intensity"), &Reflectance.Intensity);
				NMake::Unpack(Node->Find("distance"), &Reflectance.Distance);
				NMake::Unpack(Node->Find("cutoff"), &Gloss.Cutoff);
				NMake::Unpack(Node->Find("blur"), &Gloss.Blur);
				NMake::Unpack(Node->Find("deadzone"), &Gloss.Deadzone);
				NMake::Unpack(Node->Find("mips"), &Gloss.Mips);
			}
			void SSR::Serialize(ContentManager* Content, Core::Schema* Node)
			{
				TH_ASSERT_V(Content != nullptr, "content manager should be set");
				TH_ASSERT_V(Node != nullptr, "schema should be set");

				NMake::Pack(Node->Set("samples-1"), Reflectance.Samples);
				NMake::Pack(Node->Set("samples-2"), Gloss.Samples);
				NMake::Pack(Node->Set("intensity"), Reflectance.Intensity);
				NMake::Pack(Node->Set("distance"), Reflectance.Distance);
				NMake::Pack(Node->Set("cutoff"), Gloss.Cutoff);
				NMake::Pack(Node->Set("blur"), Gloss.Blur);
				NMake::Pack(Node->Set("deadzone"), Gloss.Deadzone);
				NMake::Pack(Node->Set("mips"), Gloss.Mips);
			}
			void SSR::RenderEffect(Core::Timer* Time)
			{
				Graphics::MultiRenderTarget2D* MRT = System->GetMRT(TargetType::Main);
				System->GetDevice()->GenerateMips(MRT->GetTarget(0));

				Gloss.Mips = GetMipLevels();
				Gloss.Texel[0] = 1.0f / GetWidth();
				Gloss.Texel[1] = 1.0f / GetHeight();

				RenderMerge(Shaders.Reflectance, &Reflectance);
				RenderMerge(Shaders.Gloss[0], &Gloss, 2);
				RenderMerge(Shaders.Gloss[1], nullptr, 2);
				RenderResult(Shaders.Additive);
			}

			SSGI::SSGI(RenderSystem* Lab) : EffectRenderer(Lab)
			{
				Shaders.Stochastic = CompileEffect("postprocess/stochastic", sizeof(Stochastic));
				Shaders.Indirection = CompileEffect("postprocess/indirection", sizeof(Indirection));
				Shaders.Denoise[0] = CompileEffect("postprocess/denoise-x", sizeof(Denoise));
				Shaders.Denoise[1] = CompileEffect("postprocess/denoise-y");
				Shaders.Additive = CompileEffect("postprocess/additive");
			}
			void SSGI::Deserialize(ContentManager* Content, Core::Schema* Node)
			{
				TH_ASSERT_V(Content != nullptr, "content manager should be set");
				TH_ASSERT_V(Node != nullptr, "schema should be set");

				NMake::Unpack(Node->Find("samples-1"), &Indirection.Samples);
				NMake::Unpack(Node->Find("samples-2"), &Denoise.Samples);
				NMake::Unpack(Node->Find("cutoff-1"), &Indirection.Cutoff);
				NMake::Unpack(Node->Find("cutoff-2"), &Denoise.Cutoff);
				NMake::Unpack(Node->Find("attenuation"), &Indirection.Attenuation);
				NMake::Unpack(Node->Find("swing"), &Indirection.Swing);
				NMake::Unpack(Node->Find("blur"), &Denoise.Blur);
			}
			void SSGI::Serialize(ContentManager* Content, Core::Schema* Node)
			{
				TH_ASSERT_V(Content != nullptr, "content manager should be set");
				TH_ASSERT_V(Node != nullptr, "schema should be set");

				NMake::Pack(Node->Set("samples-1"), Indirection.Samples);
				NMake::Pack(Node->Set("samples-2"), Denoise.Samples);
				NMake::Pack(Node->Set("cutoff-1"), Indirection.Cutoff);
				NMake::Pack(Node->Set("cutoff-2"), Denoise.Cutoff);
				NMake::Pack(Node->Set("attenuation"), Indirection.Attenuation);
				NMake::Pack(Node->Set("swing"), Indirection.Swing);
				NMake::Pack(Node->Set("blur"), Denoise.Blur);
			}
			void SSGI::RenderEffect(Core::Timer* Time)
			{
				Indirection.Random[0] = Compute::Math<float>::Random();
				Indirection.Random[1] = Compute::Math<float>::Random();
				Denoise.Texel[0] = 1.0f / GetWidth();
				Denoise.Texel[1] = 1.0f / GetHeight();
				Stochastic.Texel[0] = GetWidth();
				Stochastic.Texel[1] = GetHeight();
				Stochastic.FrameId++;

				RenderMerge(Shaders.Stochastic, &Stochastic);
				RenderMerge(Shaders.Indirection, &Indirection);
				RenderMerge(Shaders.Denoise[0], &Denoise, 3);
				RenderMerge(Shaders.Denoise[1], nullptr, 3);
				RenderResult(Shaders.Additive);
			}

			SSAO::SSAO(RenderSystem* Lab) : EffectRenderer(Lab)
			{
				Shaders.Shading = CompileEffect("postprocess/shading", sizeof(Shading));
				Shaders.Fibo[0] = CompileEffect("postprocess/fibo-x", sizeof(Fibo));
				Shaders.Fibo[1] = CompileEffect("postprocess/fibo-y");
				Shaders.Multiply = CompileEffect("postprocess/multiply");
			}
			void SSAO::Deserialize(ContentManager* Content, Core::Schema* Node)
			{
				TH_ASSERT_V(Content != nullptr, "content manager should be set");
				TH_ASSERT_V(Node != nullptr, "schema should be set");

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
			void SSAO::Serialize(ContentManager* Content, Core::Schema* Node)
			{
				TH_ASSERT_V(Content != nullptr, "content manager should be set");
				TH_ASSERT_V(Node != nullptr, "schema should be set");

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
			void DoF::Deserialize(ContentManager* Content, Core::Schema* Node)
			{
				TH_ASSERT_V(Content != nullptr, "content manager should be set");
				TH_ASSERT_V(Node != nullptr, "schema should be set");

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
			void DoF::Serialize(ContentManager* Content, Core::Schema* Node)
			{
				TH_ASSERT_V(Content != nullptr, "content manager should be set");
				TH_ASSERT_V(Node != nullptr, "schema should be set");

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
				Shaders.Motion = CompileEffect("postprocess/motion", sizeof(Motion));
			}
			void MotionBlur::Deserialize(ContentManager* Content, Core::Schema* Node)
			{
				TH_ASSERT_V(Content != nullptr, "content manager should be set");
				TH_ASSERT_V(Node != nullptr, "schema should be set");

				NMake::Unpack(Node->Find("samples"), &Motion.Samples);
				NMake::Unpack(Node->Find("blur"), &Motion.Blur);
				NMake::Unpack(Node->Find("motion"), &Motion.Motion);
			}
			void MotionBlur::Serialize(ContentManager* Content, Core::Schema* Node)
			{
				TH_ASSERT_V(Content != nullptr, "content manager should be set");
				TH_ASSERT_V(Node != nullptr, "schema should be set");

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
				Shaders.Fibo[0] = CompileEffect("postprocess/fibo-x", sizeof(Fibo));
				Shaders.Fibo[1] = CompileEffect("postprocess/fibo-y");
				Shaders.Additive = CompileEffect("postprocess/additive");
			}
			void Bloom::Deserialize(ContentManager* Content, Core::Schema* Node)
			{
				NMake::Unpack(Node->Find("intensity"), &Extraction.Intensity);
				NMake::Unpack(Node->Find("threshold"), &Extraction.Threshold);
				NMake::Unpack(Node->Find("power"), &Fibo.Power);
				NMake::Unpack(Node->Find("samples"), &Fibo.Samples);
				NMake::Unpack(Node->Find("blur"), &Fibo.Blur);
			}
			void Bloom::Serialize(ContentManager* Content, Core::Schema* Node)
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
			void Tone::Deserialize(ContentManager* Content, Core::Schema* Node)
			{
				TH_ASSERT_V(Content != nullptr, "content manager should be set");
				TH_ASSERT_V(Node != nullptr, "schema should be set");

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
			void Tone::Serialize(ContentManager* Content, Core::Schema* Node)
			{
				TH_ASSERT_V(Content != nullptr, "content manager should be set");
				TH_ASSERT_V(Node != nullptr, "schema should be set");

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
				RT.MipLevels = Device->GetMipLevel((unsigned int)Size, (unsigned int)Size);
				RT.FormatMode = Graphics::Format::R16_Float;
				RT.Width = (unsigned int)Size;
				RT.Height = (unsigned int)Size;

				LutTarget = Device->CreateRenderTarget2D(RT);
				Device->CopyTexture2D(LutTarget, 0, &LutMap);
			}

			Glitch::Glitch(RenderSystem* Lab) : EffectRenderer(Lab), ScanLineJitter(0), VerticalJump(0), HorizontalShake(0), ColorDrift(0)
			{
				CompileEffect("postprocess/glitch", sizeof(Distortion));
			}
			void Glitch::Deserialize(ContentManager* Content, Core::Schema* Node)
			{
				TH_ASSERT_V(Content != nullptr, "content manager should be set");
				TH_ASSERT_V(Node != nullptr, "schema should be set");

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
			void Glitch::Serialize(ContentManager* Content, Core::Schema* Node)
			{
				TH_ASSERT_V(Content != nullptr, "content manager should be set");
				TH_ASSERT_V(Node != nullptr, "schema should be set");

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
			size_t UserInterface::RenderPass(Core::Timer* Timer)
			{
#ifdef TH_WITH_RMLUI
				TH_ASSERT(Context != nullptr, 0, "context should be set");
				if (!System->State.Is(RenderState::Geometry_Result) || System->State.IsSubpass() || System->State.IsSet(RenderOpt::Transparent) || System->State.IsSet(RenderOpt::Additive))
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
