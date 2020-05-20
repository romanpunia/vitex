#include "ogl.h"
#include "../resource.h"
#include <imgui.h>
#ifdef THAWK_HAS_SDL2
#include <SDL2/SDL_syswm.h>
#endif
#ifdef THAWK_HAS_GL

namespace Tomahawk
{
    namespace Graphics
    {
        namespace OGL
        {
            OGLShader::OGLShader(Graphics::GraphicsDevice* Device, const Desc& I) : Graphics::Shader(Device, I)
            {
            }
            OGLShader::~OGLShader()
            {
            }
            void OGLShader::SendConstantStream(Graphics::GraphicsDevice* Device)
            {
            }
            void OGLShader::Apply(Graphics::GraphicsDevice* Device)
            {
            }

            OGLElementBuffer::OGLElementBuffer(Graphics::GraphicsDevice* Device, const Desc& I) : Graphics::ElementBuffer(Device, I)
            {
                Bind = OGLDevice::GetResourceBind(I.BindFlags);

                glGenBuffers(1, &Resource);
                glBindBuffer(Bind, Resource);
                glBufferData(Bind, I.ElementCount * I.ElementWidth, I.Elements, GL_STATIC_DRAW);
            }
            OGLElementBuffer::~OGLElementBuffer()
            {
                glDeleteBuffers(1, &Resource);
            }
            void OGLElementBuffer::IndexedBuffer(Graphics::GraphicsDevice* Device, Graphics::Format Format, unsigned int Offset)
            {
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, Resource);
            }
            void OGLElementBuffer::VertexBuffer(Graphics::GraphicsDevice* Device, unsigned int Slot, unsigned int Stride, unsigned int Offset)
            {
                glBindBuffer(GL_ARRAY_BUFFER, Resource);
            }
            void OGLElementBuffer::Map(Graphics::GraphicsDevice* Device, Graphics::ResourceMap Mode, Graphics::MappedSubresource* Map)
            {
                GLint Size;
                glBindBuffer(Bind, Resource);
                glGetBufferParameteriv(Bind, GL_BUFFER_SIZE, &Size);
                Map->Pointer = glMapBuffer(Bind, OGLDevice::GetResourceMap(Mode));
                Map->RowPitch = Size;
                Map->DepthPitch = 1;
            }
            void OGLElementBuffer::Unmap(Graphics::GraphicsDevice* Device, Graphics::MappedSubresource* Map)
            {
                glBindBuffer(Bind, Resource);
                glUnmapBuffer(Bind);
            }
            void* OGLElementBuffer::GetResource()
            {
                return (void*)(intptr_t)Resource;
            }

            OGLStructureBuffer::OGLStructureBuffer(Graphics::GraphicsDevice* Device, const Desc& I) : Graphics::StructureBuffer(Device, I)
            {
                Bind = OGLDevice::GetResourceBind(I.BindFlags);

                glGenBuffers(1, &Resource);
                glBindBuffer(GL_SHADER_STORAGE_BUFFER, Resource);
                glBufferData(GL_SHADER_STORAGE_BUFFER, I.ElementWidth * I.ElementCount, I.Elements, GL_DYNAMIC_DRAW);
                glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, Resource);
                glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
            }
            OGLStructureBuffer::~OGLStructureBuffer()
            {
                glDeleteBuffers(1, &Resource);
            }
            void OGLStructureBuffer::Map(Graphics::GraphicsDevice* Device, Graphics::ResourceMap Mode, Graphics::MappedSubresource* Map)
            {
                GLint Size;
                glBindBuffer(Bind, Resource);
                glGetBufferParameteriv(Bind, GL_BUFFER_SIZE, &Size);
                Map->Pointer = glMapBuffer(Bind, OGLDevice::GetResourceMap(Mode));
                Map->RowPitch = Size;
                Map->DepthPitch = 1;
            }
            void OGLStructureBuffer::RemapSubresource(Graphics::GraphicsDevice* Device, void* Pointer, UInt64 Size)
            {
                glBindBuffer(Bind, Resource);
                glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)Size, Pointer, GL_DYNAMIC_DRAW);
            }
            void OGLStructureBuffer::Unmap(Graphics::GraphicsDevice* Device, Graphics::MappedSubresource* Map)
            {
                glBindBuffer(Bind, Resource);
                glUnmapBuffer(Bind);
            }
            void OGLStructureBuffer::Apply(Graphics::GraphicsDevice* Device, int Slot)
            {
                glBindBuffer(GL_SHADER_STORAGE_BUFFER, Resource);
            }
            void* OGLStructureBuffer::GetElement()
            {
                return (void*)(intptr_t)Resource;
            }
            void* OGLStructureBuffer::GetResource()
            {
                return (void*)(intptr_t)Resource;
            }

            OGLTexture2D::OGLTexture2D(Graphics::GraphicsDevice* Device) : Graphics::Texture2D(Device)
            {
                Width = 512;
                Height = 512;
                MipLevels = 1;
                FormatMode = Graphics::Format_Invalid;
                Usage = Graphics::ResourceUsage_Default;
                AccessFlags = Graphics::CPUAccess_Invalid;
            }
            OGLTexture2D::OGLTexture2D(Graphics::GraphicsDevice* Device, const Desc& I) : Graphics::Texture2D(Device, I)
            {
                Width = I.Width;
                Height = I.Height;
                MipLevels = I.MipLevels;
                FormatMode = I.FormatMode;
                Usage = I.Usage;
                AccessFlags = I.AccessFlags;

                glGenTextures(1, &Resource);
                glBindTexture(GL_TEXTURE_2D, Resource);
                glTexImage2D(GL_TEXTURE_2D, 0, OGLDevice::GetFormat(FormatMode), Width, Height, 0, OGLDevice::GetFormat(FormatMode), GL_UNSIGNED_BYTE, I.Data);

                if (MipLevels != 0)
                {
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                    glGenerateMipmap(GL_TEXTURE_2D);
                }
                else
                {
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                }
            }
            OGLTexture2D::~OGLTexture2D()
            {
                glDeleteTextures(1, &Resource);
            }
            void OGLTexture2D::Apply(Graphics::GraphicsDevice* Device, int Slot)
            {
                glActiveTexture(GL_TEXTURE0 + Slot);
                glBindTexture(GL_TEXTURE_2D, Resource);
            }
            void OGLTexture2D::Fill(OGLDevice* Device)
            {
            }
            void* OGLTexture2D::GetResource()
            {
                return (void*)(intptr_t)Resource;
            }

            OGLTexture3D::OGLTexture3D(Graphics::GraphicsDevice* Device) : Graphics::Texture3D(Device)
            {
            }
            OGLTexture3D::~OGLTexture3D()
            {
            }
            void OGLTexture3D::Apply(Graphics::GraphicsDevice* Device, int Slot)
            {
            }
            void* OGLTexture3D::GetResource()
            {
                return (void*)nullptr;
            }

            OGLTextureCube::OGLTextureCube(Graphics::GraphicsDevice* Device) : Graphics::TextureCube(Device)
            {
            }
            OGLTextureCube::OGLTextureCube(Graphics::GraphicsDevice* Device, const Desc& I) : Graphics::TextureCube(Device, I)
            {
            }
            OGLTextureCube::~OGLTextureCube()
            {
            }
            void OGLTextureCube::Apply(Graphics::GraphicsDevice* Device, int Slot)
            {
            }
            void* OGLTextureCube::GetResource()
            {
                return (void*)nullptr;
            }

            OGLRenderTarget2D::OGLRenderTarget2D(Graphics::GraphicsDevice* Device, const Desc& I) : Graphics::RenderTarget2D(Device, I)
            {
                if (!I.RenderSurface)
                {
                    GLenum Format = OGLDevice::GetFormat(I.FormatMode);
                    glGenFramebuffers(1, &FrameBuffer);
                    glGenTextures(1, &Texture);
                    glBindTexture(GL_TEXTURE_2D, Texture);
                    glTexImage2D(GL_TEXTURE_2D, 0, Format, I.Width, I.Height, 0, Format, GL_UNSIGNED_BYTE, NULL);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                    glBindFramebuffer(GL_FRAMEBUFFER, FrameBuffer);
                    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, Texture, 0);
                    glGenRenderbuffers(1, &DepthBuffer);
                    glBindRenderbuffer(GL_RENDERBUFFER, DepthBuffer);
                    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, I.Width, I.Height);
                    glBindRenderbuffer(GL_RENDERBUFFER, 0);
                    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, DepthBuffer);
                    glBindFramebuffer(GL_FRAMEBUFFER, 0);
                }
                else if (I.RenderSurface != (void*)Device)
                {
                    Texture = *(GLuint*)I.RenderSurface;
                    glGenFramebuffers(1, &FrameBuffer);
                    glBindFramebuffer(GL_FRAMEBUFFER, FrameBuffer);
                    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, Texture, 0);
                    glGenRenderbuffers(1, &DepthBuffer);
                    glBindRenderbuffer(GL_RENDERBUFFER, DepthBuffer);
                    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, I.Width, I.Height);
                    glBindRenderbuffer(GL_RENDERBUFFER, 0);
                    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, DepthBuffer);
                    glBindFramebuffer(GL_FRAMEBUFFER, 0);
                }

                Viewport.Width = (float)I.Width;
                Viewport.Height = (float)I.Height;
                Viewport.TopLeftX = 0.0f;
                Viewport.TopLeftY = 0.0f;
                Viewport.MinDepth = 0.0f;
                Viewport.MaxDepth = 1.0f;
            }
            OGLRenderTarget2D::~OGLRenderTarget2D()
            {
                glDeleteFramebuffers(1, &FrameBuffer);
            }
            void OGLRenderTarget2D::Apply(Graphics::GraphicsDevice* Device, float R, float G, float B)
            {
                glBindFramebuffer(GL_FRAMEBUFFER, FrameBuffer != GL_INVALID_VALUE ? FrameBuffer : 0);
                glViewport((GLuint)Viewport.TopLeftX, (GLuint)Viewport.TopLeftY, (GLuint)Viewport.Width, (GLuint)Viewport.Height);
                glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
                glClearColor(R, G, B, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
            }
            void OGLRenderTarget2D::Apply(Graphics::GraphicsDevice* Device)
            {
                glBindFramebuffer(GL_FRAMEBUFFER, FrameBuffer != GL_INVALID_VALUE ? FrameBuffer : 0);
                glViewport((GLuint)Viewport.TopLeftX, (GLuint)Viewport.TopLeftY, (GLuint)Viewport.Width, (GLuint)Viewport.Height);
            }
            void OGLRenderTarget2D::Clear(Graphics::GraphicsDevice* Device, float R, float G, float B)
            {
                glBindFramebuffer(GL_FRAMEBUFFER, FrameBuffer != GL_INVALID_VALUE ? FrameBuffer : 0);
                glViewport((GLuint)Viewport.TopLeftX, (GLuint)Viewport.TopLeftY, (GLuint)Viewport.Width, (GLuint)Viewport.Height);
                glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
                glClearColor(R, G, B, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
            }
            void OGLRenderTarget2D::CopyTexture2D(Graphics::GraphicsDevice* Device, Graphics::Texture2D** Value)
            {
            }
            void OGLRenderTarget2D::SetViewport(const Graphics::Viewport& In)
            {
                Viewport = In;
                glViewport((GLuint)Viewport.TopLeftX, (GLuint)Viewport.TopLeftY, (GLuint)Viewport.Width, (GLuint)Viewport.Height);
                glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
            }
            Graphics::Viewport OGLRenderTarget2D::GetViewport()
            {
                return Viewport;
            }
            float OGLRenderTarget2D::GetWidth()
            {
                return Viewport.Width;
            }
            float OGLRenderTarget2D::GetHeight()
            {
                return Viewport.Height;
            }
            void* OGLRenderTarget2D::GetResource()
            {
                return (void*)(intptr_t)FrameBuffer;
            }

            OGLMultiRenderTarget2D::OGLMultiRenderTarget2D(Graphics::GraphicsDevice* Device, const Desc& I) : Graphics::MultiRenderTarget2D(Device, I)
            {
            }
            OGLMultiRenderTarget2D::~OGLMultiRenderTarget2D()
            {
            }
            void OGLMultiRenderTarget2D::Apply(Graphics::GraphicsDevice* Device, int Target, float R, float G, float B)
            {
            }
            void OGLMultiRenderTarget2D::Apply(Graphics::GraphicsDevice* Device, int Target)
            {
            }
            void OGLMultiRenderTarget2D::Apply(Graphics::GraphicsDevice* Device, float R, float G, float B)
            {
            }
            void OGLMultiRenderTarget2D::Apply(Graphics::GraphicsDevice* Device)
            {
            }
            void OGLMultiRenderTarget2D::Clear(Graphics::GraphicsDevice* Device, int Target, float R, float G, float B)
            {
            }
            void OGLMultiRenderTarget2D::CopyTexture2D(int Target, Graphics::GraphicsDevice* Device, Graphics::Texture2D** Value)
            {
            }
            void OGLMultiRenderTarget2D::SetViewport(const Graphics::Viewport& In)
            {
            }
            Graphics::Viewport OGLMultiRenderTarget2D::GetViewport()
            {
                Graphics::Viewport Output = Graphics::Viewport();
                return Output;
            }
            float OGLMultiRenderTarget2D::GetWidth()
            {
                return 0.0f;
            }
            float OGLMultiRenderTarget2D::GetHeight()
            {
                return 0.0f;
            }
            void* OGLMultiRenderTarget2D::GetResource(int Id)
            {
                return nullptr;
            }

            OGLRenderTarget2DArray::OGLRenderTarget2DArray(Graphics::GraphicsDevice* Device, const Desc& I) : Graphics::RenderTarget2DArray(Device, I)
            {
            }
            OGLRenderTarget2DArray::~OGLRenderTarget2DArray()
            {
                delete Resource;
            }
            void OGLRenderTarget2DArray::Apply(Graphics::GraphicsDevice* Device, int Target, float R, float G, float B)
            {
            }
            void OGLRenderTarget2DArray::Apply(Graphics::GraphicsDevice* Device, int Target)
            {
            }
            void OGLRenderTarget2DArray::Clear(Graphics::GraphicsDevice* Device, int Target, float R, float G, float B)
            {
            }
            void OGLRenderTarget2DArray::SetViewport(const Graphics::Viewport& In)
            {
            }
            Graphics::Viewport OGLRenderTarget2DArray::GetViewport()
            {
                Graphics::Viewport Output = Graphics::Viewport();
                return Output;
            }
            float OGLRenderTarget2DArray::GetWidth()
            {
                return 0.0f;
            }
            float OGLRenderTarget2DArray::GetHeight()
            {
                return 0.0f;
            }
            void* OGLRenderTarget2DArray::GetResource()
            {
                return nullptr;
            }

            OGLRenderTargetCube::OGLRenderTargetCube(Graphics::GraphicsDevice* Device, const Desc& I) : Graphics::RenderTargetCube(Device, I)
            {
            }
            OGLRenderTargetCube::~OGLRenderTargetCube()
            {
            }
            void OGLRenderTargetCube::Apply(Graphics::GraphicsDevice* Device, float R, float G, float B)
            {
            }
            void OGLRenderTargetCube::Apply(Graphics::GraphicsDevice* Device)
            {
            }
            void OGLRenderTargetCube::Clear(Graphics::GraphicsDevice* Device, float R, float G, float B)
            {
            }
            void OGLRenderTargetCube::CopyTextureCube(Graphics::GraphicsDevice* Device, Graphics::TextureCube** Value)
            {
            }
            void OGLRenderTargetCube::CopyTexture2D(int FaceId, Graphics::GraphicsDevice* Device, Graphics::Texture2D** Value)
            {
            }
            void OGLRenderTargetCube::SetViewport(const Graphics::Viewport& In)
            {
            }
            Graphics::Viewport OGLRenderTargetCube::GetViewport()
            {
                Graphics::Viewport Output = Graphics::Viewport();
                return Output;
            }
            float OGLRenderTargetCube::GetWidth()
            {
                return 0.0f;
            }
            float OGLRenderTargetCube::GetHeight()
            {
                return 0.0f;
            }
            void* OGLRenderTargetCube::GetResource()
            {
                return (void*)nullptr;
            }

            OGLMultiRenderTargetCube::OGLMultiRenderTargetCube(Graphics::GraphicsDevice* Device, const Desc& I) : Graphics::MultiRenderTargetCube(Device, I)
            {
            }
            OGLMultiRenderTargetCube::~OGLMultiRenderTargetCube()
            {
            }
            void OGLMultiRenderTargetCube::Apply(Graphics::GraphicsDevice* Device, int Target, float R, float G, float B)
            {
            }
            void OGLMultiRenderTargetCube::Apply(Graphics::GraphicsDevice* Device, int Target)
            {
            }
            void OGLMultiRenderTargetCube::Apply(Graphics::GraphicsDevice* Device, float R, float G, float B)
            {
            }
            void OGLMultiRenderTargetCube::Apply(Graphics::GraphicsDevice* Device)
            {
            }
            void OGLMultiRenderTargetCube::Clear(Graphics::GraphicsDevice* Device, int Target, float R, float G, float B)
            {
            }
            void OGLMultiRenderTargetCube::CopyTextureCube(int CubeId, Graphics::GraphicsDevice* Device, Graphics::TextureCube** Value)
            {
            }
            void OGLMultiRenderTargetCube::CopyTexture2D(int CubeId, int FaceId, Graphics::GraphicsDevice* Device, Graphics::Texture2D** Value)
            {
            }
            void OGLMultiRenderTargetCube::SetViewport(const Graphics::Viewport& In)
            {
            }
            Graphics::Viewport OGLMultiRenderTargetCube::GetViewport()
            {
                Graphics::Viewport Output = Graphics::Viewport();
                return Output;
            }
            float OGLMultiRenderTargetCube::GetWidth()
            {
                return 0.0f;
            }
            float OGLMultiRenderTargetCube::GetHeight()
            {
                return 0.0f;
            }
            void* OGLMultiRenderTargetCube::GetResource(int Id)
            {
                return (void*)nullptr;
            }

            OGLMesh::OGLMesh(Graphics::GraphicsDevice* Device, const Desc& I) : Graphics::Mesh(Device, I)
            {
                Graphics::ElementBuffer::Desc F = Graphics::ElementBuffer::Desc();
                F.AccessFlags = I.AccessFlags;
                F.Usage = I.Usage;
                F.BindFlags = Graphics::ResourceBind_Vertex_Buffer;
                F.ElementCount = (unsigned int)I.Elements.size();
                F.UseSubresource = true;
                F.Elements = (void*)I.Elements.data();
                F.ElementWidth = sizeof(Compute::Vertex);

                VertexBuffer = Graphics::ElementBuffer::Create(Device, F);

                F = Graphics::ElementBuffer::Desc();
                F.AccessFlags = I.AccessFlags;
                F.Usage = I.Usage;
                F.BindFlags = Graphics::ResourceBind_Index_Buffer;
                F.ElementCount = (unsigned int)I.Indices.size();
                F.ElementWidth = sizeof(UInt64);
                F.Elements = (void*)I.Indices.data();
                F.UseSubresource = true;

                IndexBuffer = Graphics::ElementBuffer::Create(Device, F);
            }
            void OGLMesh::Update(Graphics::GraphicsDevice* Device, Compute::Vertex* Elements)
            {
                Graphics::MappedSubresource Resource;
                VertexBuffer->Map(Device, Graphics::ResourceMap_Write, &Resource);
                memcpy(Resource.Pointer, Elements, (size_t)VertexBuffer->GetElements() * sizeof(Compute::Vertex));
                VertexBuffer->Unmap(Device, &Resource);
            }
            void OGLMesh::Draw(Graphics::GraphicsDevice* Device)
            {
            }
            Compute::Vertex* OGLMesh::Elements(Graphics::GraphicsDevice* Device)
            {
                Graphics::MappedSubresource Resource;
                VertexBuffer->Map(Device, Graphics::ResourceMap_Write, &Resource);

                Compute::Vertex* Vertices = new Compute::Vertex[(unsigned int)VertexBuffer->GetElements()];
                memcpy(Vertices, Resource.Pointer, (size_t)VertexBuffer->GetElements() * sizeof(Compute::Vertex));

                VertexBuffer->Unmap(Device, &Resource);
                return Vertices;
            }

            OGLSkinnedMesh::OGLSkinnedMesh(Graphics::GraphicsDevice* Device, const Desc& I) : Graphics::SkinnedMesh(Device, I)
            {
                Graphics::ElementBuffer::Desc F = Graphics::ElementBuffer::Desc();
                F.AccessFlags = I.AccessFlags;
                F.Usage = I.Usage;
                F.BindFlags = Graphics::ResourceBind_Vertex_Buffer;
                F.ElementCount = (unsigned int)I.Elements.size();
                F.UseSubresource = true;
                F.Elements = (void*)I.Elements.data();
                F.ElementWidth = sizeof(Compute::InfluenceVertex);

                VertexBuffer = Graphics::ElementBuffer::Create(Device, F);

                F = Graphics::ElementBuffer::Desc();
                F.AccessFlags = I.AccessFlags;
                F.Usage = I.Usage;
                F.BindFlags = Graphics::ResourceBind_Index_Buffer;
                F.ElementCount = (unsigned int)I.Indices.size();
                F.ElementWidth = sizeof(UInt64);
                F.Elements = (void*)I.Indices.data();
                F.UseSubresource = true;

                IndexBuffer = Graphics::ElementBuffer::Create(Device, F);
            }
            void OGLSkinnedMesh::Update(Graphics::GraphicsDevice* Device, Compute::InfluenceVertex* Elements)
            {
                Graphics::MappedSubresource Resource;
                VertexBuffer->Map(Device, Graphics::ResourceMap_Write, &Resource);
                memcpy(Resource.Pointer, Elements, (size_t)VertexBuffer->GetElements() * sizeof(Compute::InfluenceVertex));
                VertexBuffer->Unmap(Device, &Resource);
            }
            void OGLSkinnedMesh::Draw(Graphics::GraphicsDevice* Device)
            {
            }
            Compute::InfluenceVertex* OGLSkinnedMesh::Elements(Graphics::GraphicsDevice* Device)
            {
                Graphics::MappedSubresource Resource;
                VertexBuffer->Map(Device, Graphics::ResourceMap_Write, &Resource);

                Compute::InfluenceVertex* Vertices = new Compute::InfluenceVertex[(unsigned int)VertexBuffer->GetElements()];
                memcpy(Vertices, Resource.Pointer, (size_t)VertexBuffer->GetElements() * sizeof(Compute::InfluenceVertex));

                VertexBuffer->Unmap(Device, &Resource);
                return Vertices;
            }

            OGLInstanceBuffer::OGLInstanceBuffer(Graphics::GraphicsDevice* Device, const Desc& I) : Graphics::InstanceBuffer(Device, I)
            {
            }
            OGLInstanceBuffer::~OGLInstanceBuffer()
            {
                if (SynchronizationState)
                    Restore();
            }
            void OGLInstanceBuffer::SendPool()
            {
                if (Array.Size() <= 0 || Array.Size() > ElementLimit)
                    return;
                else
                    SynchronizationState = true;
            }
            void OGLInstanceBuffer::Restore()
            {
                if (!SynchronizationState)
                    return;
                else
                    SynchronizationState = false;
            }
            void OGLInstanceBuffer::Resize(UInt64 Size)
            {
                Restore();
                delete Elements;

                ElementLimit = Size + 1;
                if (ElementLimit < 1)
                    ElementLimit = 2;

                Array.Clear();
                Array.Reserve(ElementLimit);

                Graphics::ElementBuffer::Desc F = Graphics::ElementBuffer::Desc();
                F.AccessFlags = Graphics::CPUAccess_Write;
                F.MiscFlags = Graphics::ResourceMisc_Buffer_Structured;
                F.Usage = Graphics::ResourceUsage_Dynamic;
                F.BindFlags = Graphics::ResourceBind_Shader_Input;
                F.ElementCount = (unsigned int)ElementLimit;
                F.ElementWidth = sizeof(Compute::ElementVertex);
                F.StructureByteStride = F.ElementWidth;
                F.UseSubresource = false;

                Elements = Graphics::ElementBuffer::Create(Device, F);
            }

            OGLDirectBuffer::OGLDirectBuffer(Graphics::GraphicsDevice* NewDevice) : Graphics::DirectBuffer(NewDevice)
            {
                Elements.push_back(Vertex());
            }
            OGLDirectBuffer::~OGLDirectBuffer()
            {
            }
            void OGLDirectBuffer::Begin()
            {
                Buffer.WorldViewProjection = Compute::Matrix4x4::Identity();
                Buffer.Padding = { 0, 0, 0, 1 };
                View = nullptr;
                Primitives = Graphics::PrimitiveTopology_Triangle_List;
                Elements.clear();
            }
            void OGLDirectBuffer::End()
            {
                OGLDevice* Dev = Device->As<OGLDevice>();
                if (Elements.empty())
                    return;

                if (Elements.size() > MaxElements)
                    AllocVertexBuffer(Elements.size());

                if (View)
                    View->Apply(Dev, 0);
                else
                    Dev->RestoreTexture2D(0, 1);
            }
            void OGLDirectBuffer::EmitVertex()
            {
                Elements.push_back({ 0, 0, 0, 0, 0, 1, 1, 1, 1 });
            }
            void OGLDirectBuffer::Position(float X, float Y, float Z)
            {
                if (Elements.size() > 0)
                {
                    Elements[Elements.size() - 1].PX = X;
                    Elements[Elements.size() - 1].PY = Y;
                    Elements[Elements.size() - 1].PZ = Z;
                }
            }
            void OGLDirectBuffer::TexCoord(float X, float Y)
            {
                if (Elements.size() > 0)
                {
                    Elements[Elements.size() - 1].TX = X;
                    Elements[Elements.size() - 1].TY = Y;
                }
            }
            void OGLDirectBuffer::Color(float X, float Y, float Z, float W)
            {
                if (Elements.size() > 0)
                {
                    Elements[Elements.size() - 1].CX = X;
                    Elements[Elements.size() - 1].CY = Y;
                    Elements[Elements.size() - 1].CZ = Z;
                    Elements[Elements.size() - 1].CW = W;
                }
            }
            void OGLDirectBuffer::Texture(Graphics::Texture2D* InView)
            {
                View = InView;
                Buffer.Padding.Z = 1;
            }
            void OGLDirectBuffer::Intensity(float Intensity)
            {
                Buffer.Padding.W = Intensity;
            }
            void OGLDirectBuffer::TexCoordOffset(float X, float Y)
            {
                Buffer.Padding.X = X;
                Buffer.Padding.Y = Y;
            }
            void OGLDirectBuffer::Transform(Compute::Matrix4x4 Input)
            {
                Buffer.WorldViewProjection = Buffer.WorldViewProjection * Input;
            }
            void OGLDirectBuffer::AllocVertexBuffer(UInt64 Size)
            {
                MaxElements = Size;
            }
            void OGLDirectBuffer::Topology(Graphics::PrimitiveTopology DrawTopology)
            {
                Primitives = DrawTopology;
            }

            OGLDevice::OGLDevice(const Desc& I) : Graphics::GraphicsDevice(I), Window(I.Window), Context(nullptr)
            {
                if (!Window)
                {
                    THAWK_ERROR("OpenGL cannot be created without a window");
                    return;
                }

#ifdef THAWK_HAS_SDL2
                SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
                SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);
                SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 6);
                SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 5);

                Context = SDL_GL_CreateContext(Window->GetHandle());
                if (!Context)
                {
                    THAWK_ERROR("OpenGL creation conflict -> %s", Window->GetError().c_str());
                    return;
                }

                SDL_GL_MakeCurrent(Window->GetHandle(), Context);
                switch (VSyncMode)
                {
                    case Graphics::VSync_Disabled:
                        SDL_GL_SetSwapInterval(0);
                        break;
                    case Graphics::VSync_Frequency_X1:
                        SDL_GL_SetSwapInterval(1);
                        break;
                    case Graphics::VSync_Frequency_X2:
                    case Graphics::VSync_Frequency_X3:
                    case Graphics::VSync_Frequency_X4:
                        if (SDL_GL_SetSwapInterval(-1) == -1)
                            SDL_GL_SetSwapInterval(1);
                        break;
                }
#endif

                const GLenum ErrorCode = glewInit();
                if (ErrorCode != GLEW_OK)
                {
                    THAWK_ERROR("OpenGL extentions cannot be loaded -> %s", (const char*)glewGetErrorString(ErrorCode));
                    return;
                }

                LoadShaderSections();
                CreateConstantBuffer(sizeof(Graphics::AnimationBuffer), ConstantBuffer[0]);
                ConstantData[0] = &Animation;

                CreateConstantBuffer(sizeof(Graphics::RenderBuffer), ConstantBuffer[1]);
                ConstantData[1] = &Render;

                CreateConstantBuffer(sizeof(Graphics::ViewBuffer), ConstantBuffer[2]);
                ConstantData[2] = &View;

                glBindBuffer(GL_UNIFORM_BUFFER, ConstantBuffer[0]);
                glBindBuffer(GL_UNIFORM_BUFFER, ConstantBuffer[1]);
                glBindBuffer(GL_UNIFORM_BUFFER, ConstantBuffer[2]);
                glEnable(GL_TEXTURE_2D);
                SetShaderModel(I.ShaderMode == Graphics::ShaderModel_Auto ? GetSupportedShaderModel() : I.ShaderMode);
                ResizeBuffers(I.BufferWidth, I.BufferHeight);
                CreateRendererStates();

                Graphics::Shader::Desc F = Graphics::Shader::Desc();
                F.Layout = Graphics::Shader::GetShapeVertexLayout();
                F.LayoutSize = 2;
#ifdef HAS_OGL_BASIC_EFFECT_GLSL
                F.Data = reinterpret_cast<const char*>(resource_batch::ogl_basic_effect_glsl::data);
#else
                THAWK_ERROR("basic-effect.glsl was not compiled");
#endif

                BasicEffect = Shader::Create(this, F);
            }
            OGLDevice::~OGLDevice()
            {
                RestoreSamplerStates();
                RestoreBlendStates();
                RestoreRasterizerStates();
                RestoreDepthStencilStates();

                if (Context != nullptr)
                {
                    SDL_GL_DeleteContext(Context);
                    Context = nullptr;
                }

                glDeleteBuffers(3, ConstantBuffer);
            }
            void OGLDevice::RestoreSamplerStates()
            {
                for (int i = 0; i < SamplerStates.size(); i++)
                {
                    GLuint* State = (GLuint*)SamplerStates[i]->Pointer;
                    delete State;
                    delete SamplerStates[i];
                }
                SamplerStates.clear();
            }
            void OGLDevice::RestoreBlendStates()
            {
                for (int i = 0; i < BlendStates.size(); i++)
                    delete BlendStates[i];
                BlendStates.clear();
            }
            void OGLDevice::RestoreRasterizerStates()
            {
                for (int i = 0; i < RasterizerStates.size(); i++)
                    delete RasterizerStates[i];
                RasterizerStates.clear();
            }
            void OGLDevice::RestoreDepthStencilStates()
            {
                for (int i = 0; i < DepthStencilStates.size(); i++)
                    delete DepthStencilStates[i];
                DepthStencilStates.clear();
            }
            void OGLDevice::RestoreState(Graphics::DeviceState* RefState)
            {
                if (!RefState)
                    return;

                OGLDeviceState* State = (OGLDeviceState*)RefState;
#ifdef GL_SAMPLER_BINDING
                glBindSampler(0, State->Sampler);
#endif
#ifdef GL_POLYGON_MODE
                glPolygonMode(GL_FRONT_AND_BACK, (GLenum)State->PolygonMode[0]);
#endif
                glUseProgram(State->Program);
                glBindTexture(GL_TEXTURE_2D, State->Texture);
                glActiveTexture(State->ActiveTexture);
                glBindVertexArray(State->VertexArrayObject);
                glBindBuffer(GL_ARRAY_BUFFER, State->ArrayBuffer);
                glBlendEquationSeparate(State->BlendEquation, State->BlendEquationAlpha);
                glBlendFuncSeparate(State->BlendSrc, State->BlendDst, State->BlendSrcAlpha, State->BlendDstAlpha);
                if (State->EnableBlend)
                    glEnable(GL_BLEND);
                else
                    glDisable(GL_BLEND);
                if (State->EnableCullFace)
                    glEnable(GL_CULL_FACE);
                else
                    glDisable(GL_CULL_FACE);
                if (State->EnableDepthTest)
                    glEnable(GL_DEPTH_TEST);
                else
                    glDisable(GL_DEPTH_TEST);
                if (State->EnableScissorTest)
                    glEnable(GL_SCISSOR_TEST);
                else
                    glDisable(GL_SCISSOR_TEST);
                glViewport(State->Viewport[0], State->Viewport[1], (GLsizei)State->Viewport[2], (GLsizei)State->Viewport[3]);
                glScissor(State->ScissorBox[0], State->ScissorBox[1], (GLsizei)State->ScissorBox[2], (GLsizei)State->ScissorBox[3]);
            }
            void OGLDevice::ReleaseState(Graphics::DeviceState** RefState)
            {
                if (!RefState || !*RefState)
                    return;

                delete (OGLDeviceState*)*RefState;
                *RefState = nullptr;
            }
            void OGLDevice::RestoreState()
            {
            }
            void OGLDevice::SetConstantBuffers()
            {
                glBindBufferBase(GL_UNIFORM_BUFFER, 0, ConstantBuffer[0]);
                glBindBufferBase(GL_UNIFORM_BUFFER, 1, ConstantBuffer[1]);
                glBindBufferBase(GL_UNIFORM_BUFFER, 2, ConstantBuffer[2]);
            }
            void OGLDevice::SetShaderModel(Graphics::ShaderModel RShaderModel)
            {
                ShaderModelType = RShaderModel;
                if (RShaderModel == Graphics::ShaderModel_GLSL_1_1_0)
                    ShaderVersion = "#version 110\n";
                else if (RShaderModel == Graphics::ShaderModel_GLSL_1_2_0)
                    ShaderVersion = "#version 120\n";
                else if (RShaderModel == Graphics::ShaderModel_GLSL_1_3_0)
                    ShaderVersion = "#version 130\n";
                else if (RShaderModel == Graphics::ShaderModel_GLSL_1_4_0)
                    ShaderVersion = "#version 140\n";
                else if (RShaderModel == Graphics::ShaderModel_GLSL_1_5_0)
                    ShaderVersion = "#version 150\n";
                else if (RShaderModel == Graphics::ShaderModel_GLSL_3_3_0)
                    ShaderVersion = "#version 330\n";
                else if (RShaderModel == Graphics::ShaderModel_GLSL_4_0_0)
                    ShaderVersion = "#version 400\n";
                else if (RShaderModel == Graphics::ShaderModel_GLSL_4_1_0)
                    ShaderVersion = "#version 410\n";
                else if (RShaderModel == Graphics::ShaderModel_GLSL_4_2_0)
                    ShaderVersion = "#version 420\n";
                else if (RShaderModel == Graphics::ShaderModel_GLSL_4_3_0)
                    ShaderVersion = "#version 430\n";
                else if (RShaderModel == Graphics::ShaderModel_GLSL_4_4_0)
                    ShaderVersion = "#version 440\n";
                else if (RShaderModel == Graphics::ShaderModel_GLSL_4_5_0)
                    ShaderVersion = "#version 450\n";
                else if (RShaderModel == Graphics::ShaderModel_GLSL_4_6_0)
                    ShaderVersion = "#version 460\n";
                else
                    SetShaderModel(Graphics::ShaderModel_GLSL_1_1_0);
            }
            UInt64 OGLDevice::AddDepthStencilState(Graphics::DepthStencilState* In)
            {
                if (!In)
                    return 0;

                In->Index = DepthStencilStates.size();
                DepthStencilStates.push_back(In);
                return DepthStencilStates.size() - 1;
            }
            UInt64 OGLDevice::AddBlendState(Graphics::BlendState* In)
            {
                if (!In)
                    return 0;

                In->Index = BlendStates.size();
                BlendStates.push_back(In);
                return BlendStates.size() - 1;
            }
            UInt64 OGLDevice::AddRasterizerState(Graphics::RasterizerState* In)
            {
                if (!In)
                    return 0;

                In->Index = RasterizerStates.size();
                RasterizerStates.push_back(In);
                return RasterizerStates.size() - 1;
            }
            UInt64 OGLDevice::AddSamplerState(Graphics::SamplerState* In)
            {
                if (!In)
                    return 0;

                GLuint SamplerState = 0;
                glGenSamplers(1, &SamplerState);
                glSamplerParameteri(SamplerState, GL_TEXTURE_WRAP_S, OGLDevice::GetTextureAddress(In->AddressU));
                glSamplerParameteri(SamplerState, GL_TEXTURE_WRAP_T, OGLDevice::GetTextureAddress(In->AddressV));
                glSamplerParameteri(SamplerState, GL_TEXTURE_WRAP_R, OGLDevice::GetTextureAddress(In->AddressW));
                glSamplerParameteri(SamplerState, GL_TEXTURE_MAG_FILTER, OGLDevice::GetPixelFilter(In->Filter, true));
                glSamplerParameteri(SamplerState, GL_TEXTURE_MIN_FILTER, OGLDevice::GetPixelFilter(In->Filter, false));
                glSamplerParameteri(SamplerState, GL_TEXTURE_COMPARE_FUNC, OGLDevice::GetComparison(In->ComparisonFunction));
                glSamplerParameterf(SamplerState, GL_TEXTURE_LOD_BIAS, In->MipLODBias);
                glSamplerParameterf(SamplerState, GL_TEXTURE_MAX_LOD, In->MaxLOD);
                glSamplerParameterf(SamplerState, GL_TEXTURE_MIN_LOD, In->MinLOD);
                glSamplerParameterfv(SamplerState, GL_TEXTURE_BORDER_COLOR, (GLfloat*)In->BorderColor);

#ifdef GL_TEXTURE_MAX_ANISOTROPY
                if (In->Filter & Graphics::PixelFilter_Anistropic || In->Filter & Graphics::PixelFilter_Compare_Anistropic)
                    glSamplerParameterf(SamplerState, GL_TEXTURE_MAX_ANISOTROPY, (float)In->MaxAnisotropy);
#elif defined(GL_TEXTURE_MAX_ANISOTROPY_EXT)
                if (In->Filter & Graphics::PixelFilter_Anistropic || In->Filter & Graphics::PixelFilter_Compare_Anistropic)
                    glSamplerParameterf(SamplerState, GL_TEXTURE_MAX_ANISOTROPY_EXT, (float)In->MaxAnisotropy);
#endif
                In->Pointer = (void*)new GLuint(SamplerState);
                In->Index = SamplerStates.size();

                SamplerStates.push_back(In);
                return SamplerStates.size() - 1;
            }
            void OGLDevice::SetSamplerState(UInt64 State)
            {
                glBindSampler(0, (GLuint)State);
            }
            void OGLDevice::SetBlendState(UInt64 State)
            {
                if (State >= BlendStates.size())
                    return;

                Graphics::BlendState* In = BlendStates[State];
                if (In->AlphaToCoverageEnable)
                {
                    glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);
                    glSampleCoverage(1.0f, GL_FALSE);
                }
                else
                {
                    glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);
                    glSampleCoverage(0.0f, GL_FALSE);
                }

                if (!In->IndependentBlendEnable)
                {
                    for (int i = 0; i < 8; i++)
                    {
                        if (In->RenderTarget[i].BlendEnable)
                            glEnablei(GL_BLEND, i);
                        else
                            glDisablei(GL_BLEND, i);

                        glBlendEquationSeparatei(i, GetBlendOperation(In->RenderTarget[i].BlendOperationMode), GetBlendOperation(In->RenderTarget[i].BlendOperationAlpha));
                        glBlendFuncSeparatei(i, GetBlend(In->RenderTarget[i].SrcBlend), GetBlend(In->RenderTarget[i].DestBlend), GetBlend(In->RenderTarget[i].SrcBlendAlpha), GetBlend(In->RenderTarget[i].DestBlendAlpha));
                    }
                }
                else
                {
                    if (In->RenderTarget[0].BlendEnable)
                        glEnable(GL_BLEND);
                    else
                        glDisable(GL_BLEND);

                    glBlendEquationSeparate(GetBlendOperation(In->RenderTarget[0].BlendOperationMode), GetBlendOperation(In->RenderTarget[0].BlendOperationAlpha));
                    glBlendFuncSeparate(GetBlend(In->RenderTarget[0].SrcBlend), GetBlend(In->RenderTarget[0].DestBlend), GetBlend(In->RenderTarget[0].SrcBlendAlpha), GetBlend(In->RenderTarget[0].DestBlendAlpha));
                }
            }
            void OGLDevice::SetRasterizerState(UInt64 State)
            {
                if (State >= RasterizerStates.size())
                    return;

                Graphics::RasterizerState* In = RasterizerStates[State];
                if (In->AntialiasedLineEnable || In->MultisampleEnable)
                    glEnable(GL_MULTISAMPLE);
                else
                    glDisable(GL_MULTISAMPLE);

                if (In->CullMode == Graphics::VertexCull_Back)
                {
                    glEnable(GL_CULL_FACE);
                    glCullFace(GL_BACK);
                }
                else if (In->CullMode == Graphics::VertexCull_Front)
                {
                    glEnable(GL_CULL_FACE);
                    glCullFace(GL_FRONT);
                }
                else
                    glDisable(GL_CULL_FACE);

                if (In->FillMode == Graphics::SurfaceFill_Solid)
                    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                else
                    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

                if (In->FrontCounterClockwise)
                    glFrontFace(GL_CW);
                else
                    glFrontFace(GL_CCW);
            }
            void OGLDevice::SetDepthStencilState(UInt64 State)
            {
                if (State >= DepthStencilStates.size())
                    return;

                Graphics::DepthStencilState* In = DepthStencilStates[State];
                if (In->DepthEnable)
                    glEnable(GL_DEPTH);
                else
                    glDisable(GL_DEPTH);

                if (In->StencilEnable)
                    glEnable(GL_STENCIL);
                else
                    glDisable(GL_STENCIL);

                glDepthFunc(GetComparison(In->DepthFunction));
                glDepthMask(In->DepthWriteMask == Graphics::DepthWrite_All ? GL_TRUE : GL_FALSE);
                glStencilMask((GLuint)In->StencilWriteMask);
                glStencilFuncSeparate(GL_FRONT, GetComparison(In->FrontFaceStencilFunction), 0, 1);
                glStencilOpSeparate(GL_FRONT, GetStencilOperation(In->FrontFaceStencilFailOperation), GetStencilOperation(In->FrontFaceStencilDepthFailOperation), GetStencilOperation(In->FrontFaceStencilPassOperation));
                glStencilFuncSeparate(GL_BACK, GetComparison(In->BackFaceStencilFunction), 0, 1);
                glStencilOpSeparate(GL_BACK, GetStencilOperation(In->BackFaceStencilFailOperation), GetStencilOperation(In->BackFaceStencilDepthFailOperation), GetStencilOperation(In->BackFaceStencilPassOperation));
            }
            void OGLDevice::SendBufferStream(Graphics::RenderBufferType Buffer)
            {
                glBindBufferBase(GL_UNIFORM_BUFFER, (int)Buffer, ConstantBuffer[Buffer]);
            }
            void OGLDevice::Present()
            {
#ifdef THAWK_HAS_SDL2
                SDL_GL_SwapWindow(Window->GetHandle());
#endif
            }
            void OGLDevice::SetPrimitiveTopology(Graphics::PrimitiveTopology _Topology)
            {
                glPolygonMode(GL_FRONT_AND_BACK, GetPrimitiveTopology(_Topology));
                Topology = _Topology;
            }
            void OGLDevice::RestoreTexture2D(int Slot, int Size)
            {
                if (Size <= 0 || Size > 31)
                    return;

                for (int i = 0; i < Size; i++)
                {
                    glActiveTexture(GL_TEXTURE0 + Slot + i);
                    glEnable(GL_TEXTURE_2D);
                    glBindTexture(GL_TEXTURE_2D, 0);
                }
            }
            void OGLDevice::RestoreTexture2D(int Size)
            {
                RestoreTexture2D(0, Size);
            }
            void OGLDevice::RestoreTexture3D(int Slot, int Size)
            {
                if (Size <= 0 || Size > 31)
                    return;

                for (int i = 0; i < Size; i++)
                {
                    glActiveTexture(GL_TEXTURE0 + Slot + i);
                    glEnable(GL_TEXTURE_3D);
                    glBindTexture(GL_TEXTURE_3D, 0);
                }
            }
            void OGLDevice::RestoreTexture3D(int Size)
            {
                RestoreTexture3D(0, Size);
            }
            void OGLDevice::RestoreTextureCube(int Slot, int Size)
            {
                if (Size <= 0 || Size > 31)
                    return;

                for (int i = 0; i < Size; i++)
                {
                    glActiveTexture(GL_TEXTURE0 + Slot + i);
                    glEnable(GL_TEXTURE_CUBE_MAP);
                    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
                }
            }
            void OGLDevice::RestoreTextureCube(int Size)
            {
                RestoreTextureCube(0, Size);
            }
            void OGLDevice::RestoreShader()
            {
                glUseProgram(0);
            }
            void OGLDevice::ResizeBuffers(unsigned int Width, unsigned int Height)
            {
                Graphics::RenderTarget2D::Desc F = Graphics::RenderTarget2D::Desc();
                F.Width = Width;
                F.Height = Height;
                F.MipLevels = 1;
                F.MiscFlags = Graphics::ResourceMisc_None;
                F.FormatMode = Graphics::Format_R8G8B8A8_Unorm;
                F.Usage = Graphics::ResourceUsage_Default;
                F.AccessFlags = Graphics::CPUAccess_Invalid;
                F.BindFlags = Graphics::ResourceBind_Render_Target | Graphics::ResourceBind_Shader_Input;
                F.RenderSurface = (void*)this;

                delete RenderTarget;
                RenderTarget = Graphics::RenderTarget2D::Create(this, F);
                RenderTarget->Apply(this);
            }
            void OGLDevice::DrawIndexed(unsigned int Count, unsigned int IndexLocation, unsigned int)
            {
                glDrawElements(GetPrimitiveTopologyDraw(Topology), Count, GL_UNSIGNED_INT, &IndexLocation);
            }
            void OGLDevice::Draw(unsigned int Count, unsigned int Start)
            {
                glDrawArrays(GetPrimitiveTopologyDraw(Topology), (GLint)Start, (GLint)Count);
            }
            Graphics::ShaderModel OGLDevice::GetSupportedShaderModel()
            {
                const char* Version = (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION);
                if (!Version)
                    return Graphics::ShaderModel_Invalid;

                int Major, Minor;
                if (sscanf(Version, "%i.%i", &Major, &Minor) != 2)
                    return Graphics::ShaderModel_Invalid;

                if (Major >= 1 && Major <= 2)
                {
                    if (Minor <= 10)
                        return Graphics::ShaderModel_GLSL_1_1_0;

                    if (Minor <= 20)
                        return Graphics::ShaderModel_GLSL_1_2_0;

                    if (Minor <= 30)
                        return Graphics::ShaderModel_GLSL_1_3_0;

                    if (Minor <= 40)
                        return Graphics::ShaderModel_GLSL_1_4_0;

                    if (Minor <= 50)
                        return Graphics::ShaderModel_GLSL_1_5_0;

                    return Graphics::ShaderModel_GLSL_1_5_0;
                }
                else if (Major >= 2 && Major <= 3)
                {
                    if (Minor <= 30)
                        return Graphics::ShaderModel_GLSL_3_3_0;

                    return Graphics::ShaderModel_GLSL_1_5_0;
                }
                else if (Major >= 3 && Major <= 4)
                {
                    if (Minor <= 10)
                        return Graphics::ShaderModel_GLSL_4_1_0;

                    if (Minor <= 20)
                        return Graphics::ShaderModel_GLSL_4_2_0;

                    if (Minor <= 30)
                        return Graphics::ShaderModel_GLSL_4_3_0;

                    if (Minor <= 40)
                        return Graphics::ShaderModel_GLSL_4_4_0;

                    if (Minor <= 50)
                        return Graphics::ShaderModel_GLSL_4_5_0;

                    if (Minor <= 60)
                        return Graphics::ShaderModel_GLSL_4_6_0;

                    return Graphics::ShaderModel_GLSL_4_6_0;
                }

                return Graphics::ShaderModel_Invalid;
            }
            void* OGLDevice::GetDevice()
            {
                return Context;
            }
            void* OGLDevice::GetContext()
            {
                return Context;
            }
            void* OGLDevice::GetBackBuffer()
            {
                GLuint Resource;
                glGenTextures(1, &Resource);
                glBindTexture(GL_TEXTURE_2D, Resource);
                glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, (GLsizei)RenderTarget->GetWidth(), (GLsizei)RenderTarget->GetHeight());

                return (void*)(intptr_t)Resource;
            }
            void* OGLDevice::GetBackBufferMSAA()
            {
                return GetBackBuffer();
            }
            void* OGLDevice::GetBackBufferNoAA()
            {
                return GetBackBuffer();
            }
            Graphics::DeviceState* OGLDevice::CreateState()
            {
                OGLDeviceState* State = new OGLDeviceState();
                memset(State, 0, sizeof(OGLDeviceState));
                State->EnableBlend = glIsEnabled(GL_BLEND);
                State->EnableCullFace = glIsEnabled(GL_CULL_FACE);
                State->EnableDepthTest = glIsEnabled(GL_DEPTH_TEST);
                State->EnableScissorTest = glIsEnabled(GL_SCISSOR_TEST);
                glGetIntegerv(GL_ACTIVE_TEXTURE, (GLint*)&State->ActiveTexture);
                glActiveTexture(GL_TEXTURE0);
                glGetIntegerv(GL_CURRENT_PROGRAM, &State->Program);
                glGetIntegerv(GL_TEXTURE_BINDING_2D, &State->Texture);
                glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &State->ArrayBuffer);
                glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &State->VertexArrayObject);
                glGetIntegerv(GL_VIEWPORT, State->Viewport);
                glGetIntegerv(GL_SCISSOR_BOX, State->ScissorBox);
                glGetIntegerv(GL_BLEND_SRC_RGB, (GLint*)&State->BlendSrc);
                glGetIntegerv(GL_BLEND_DST_RGB, (GLint*)&State->BlendDst);
                glGetIntegerv(GL_BLEND_SRC_ALPHA, (GLint*)&State->BlendSrcAlpha);
                glGetIntegerv(GL_BLEND_DST_ALPHA, (GLint*)&State->BlendDstAlpha);
                glGetIntegerv(GL_BLEND_EQUATION_RGB, (GLint*)&State->BlendEquation);
                glGetIntegerv(GL_BLEND_EQUATION_ALPHA, (GLint*)&State->BlendEquationAlpha);
#ifdef GL_SAMPLER_BINDING
                glGetIntegerv(GL_SAMPLER_BINDING, &State->Sampler);
#endif
#ifdef GL_POLYGON_MODE
                glGetIntegerv(GL_POLYGON_MODE, State->PolygonMode);
#endif
#if defined(GL_CLIP_ORIGIN) && !defined(__APPLE__)
                glGetIntegerv(GL_CLIP_ORIGIN, (GLint*)&State->ClipOrigin);
#endif
                return State;
            }
            const char* OGLDevice::GetShaderVersion()
            {
                return ShaderVersion;
            }
            void OGLDevice::CopyConstantBuffer(GLuint Id, void* Buffer, size_t Size)
            {
                if (!Buffer || !Size)
                    return;

                glBindBuffer(GL_UNIFORM_BUFFER, Id);
                GLvoid* Data = glMapBuffer(GL_UNIFORM_BUFFER, GL_WRITE_ONLY);
                memcpy(Data, Buffer, Size);
                glUnmapBuffer(GL_UNIFORM_BUFFER);
            }
            int OGLDevice::CreateConstantBuffer(int Size, GLuint& Buffer)
            {
                glGenBuffers(1, &Buffer);
                glBindBuffer(GL_UNIFORM_BUFFER, Buffer);
                glBufferData(GL_UNIFORM_BUFFER, Size, nullptr, GL_DYNAMIC_DRAW);
                glBindBuffer(GL_UNIFORM_BUFFER, 0);

                return (int)Buffer;
            }
            std::string OGLDevice::CompileState(GLuint Handle)
            {
                GLint Stat = 0, Size = 0;
                glGetShaderiv(Handle, GL_COMPILE_STATUS, &Stat);
                glGetShaderiv(Handle, GL_INFO_LOG_LENGTH, &Size);

                if ((GLboolean)Stat == GL_TRUE || !Size)
                    return "";

                GLchar* Buffer = (GLchar*)malloc(sizeof(GLchar) * Size);
                glGetShaderInfoLog(Handle, Size, NULL, Buffer);

                std::string Result((char*)Buffer, Size);
                free(Buffer);

                return Result;
            }
            GLenum OGLDevice::GetFormat(Graphics::Format Value)
            {
                switch (Value)
                {
                    case Graphics::Format_R32G32B32A32_Typeless:
                        return GL_RGBA32UI;
                    case Graphics::Format_R32G32B32A32_Float:
                        return GL_RGBA32F;
                    case Graphics::Format_R32G32B32A32_Uint:
                        return GL_RGBA32UI;
                    case Graphics::Format_R32G32B32A32_Sint:
                        return GL_RGBA32I;
                    case Graphics::Format_R32G32B32_Typeless:
                        return GL_RGB32UI;
                    case Graphics::Format_R32G32B32_Float:
                        return GL_RGB32F;
                    case Graphics::Format_R32G32B32_Uint:
                        return GL_RGB32UI;
                    case Graphics::Format_R32G32B32_Sint:
                        return GL_RGB32I;
                    case Graphics::Format_R16G16B16A16_Typeless:
                        return GL_RGBA16UI;
                    case Graphics::Format_R16G16B16A16_Float:
                        return GL_RGBA16F;
                    case Graphics::Format_R16G16B16A16_Unorm:
                        return GL_RGBA16;
                    case Graphics::Format_R16G16B16A16_Uint:
                        return GL_RGBA16UI;
                    case Graphics::Format_R16G16B16A16_Snorm:
                        return GL_RGBA16I;
                    case Graphics::Format_R16G16B16A16_Sint:
                        return GL_RGBA16I;
                    case Graphics::Format_R32G32_Typeless:
                        return GL_RG16UI;
                    case Graphics::Format_R32G32_Float:
                        return GL_RG16F;
                    case Graphics::Format_R32G32_Uint:
                        return GL_RG16UI;
                    case Graphics::Format_R32G32_Sint:
                        return GL_RG16I;
                    case Graphics::Format_R32G8X24_Typeless:
                        return GL_R32UI;
                    case Graphics::Format_D32_Float_S8X24_Uint:
                        return GL_R32UI;
                    case Graphics::Format_R32_Float_X8X24_Typeless:
                        return GL_R32UI;
                    case Graphics::Format_X32_Typeless_G8X24_Uint:
                        return GL_R32UI;
                    case Graphics::Format_R10G10B10A2_Typeless:
                        return GL_RGB10_A2;
                    case Graphics::Format_R10G10B10A2_Unorm:
                        return GL_RGB10_A2;
                    case Graphics::Format_R10G10B10A2_Uint:
                        return GL_RGB10_A2UI;
                    case Graphics::Format_R11G11B10_Float:
                        return GL_RGB12;
                    case Graphics::Format_R8G8B8A8_Typeless:
                        return GL_RGBA8UI;
                    case Graphics::Format_R8G8B8A8_Unorm:
                        return GL_RGBA;
                    case Graphics::Format_R8G8B8A8_Unorm_SRGB:
                        return GL_RGBA;
                    case Graphics::Format_R8G8B8A8_Uint:
                        return GL_RGBA8UI;
                    case Graphics::Format_R8G8B8A8_Snorm:
                        return GL_RGBA8I;
                    case Graphics::Format_R8G8B8A8_Sint:
                        return GL_RGBA8I;
                    case Graphics::Format_R16G16_Typeless:
                        return GL_RG16UI;
                    case Graphics::Format_R16G16_Float:
                        return GL_RG16F;
                    case Graphics::Format_R16G16_Unorm:
                        return GL_RG16;
                    case Graphics::Format_R16G16_Uint:
                        return GL_RG16UI;
                    case Graphics::Format_R16G16_Snorm:
                        return GL_RG16_SNORM;
                    case Graphics::Format_R16G16_Sint:
                        return GL_RG16I;
                    case Graphics::Format_R32_Typeless:
                        return GL_R32UI;
                    case Graphics::Format_D32_Float:
                        return GL_R32F;
                    case Graphics::Format_R32_Float:
                        return GL_R32F;
                    case Graphics::Format_R32_Uint:
                        return GL_R32UI;
                    case Graphics::Format_R32_Sint:
                        return GL_R32I;
                    case Graphics::Format_R24G8_Typeless:
                        return GL_R32UI;
                    case Graphics::Format_D24_Unorm_S8_Uint:
                        return GL_R32UI;
                    case Graphics::Format_R24_Unorm_X8_Typeless:
                        return GL_R32UI;
                    case Graphics::Format_X24_Typeless_G8_Uint:
                        return GL_R32UI;
                    case Graphics::Format_R8G8_Typeless:
                        return GL_RG8UI;
                    case Graphics::Format_R8G8_Unorm:
                        return GL_RG8;
                    case Graphics::Format_R8G8_Uint:
                        return GL_RG8UI;
                    case Graphics::Format_R8G8_Snorm:
                        return GL_RG8I;
                    case Graphics::Format_R8G8_Sint:
                        return GL_RG8I;
                    case Graphics::Format_R16_Typeless:
                        return GL_R16UI;
                    case Graphics::Format_R16_Float:
                        return GL_R16F;
                    case Graphics::Format_D16_Unorm:
                        return GL_R16;
                    case Graphics::Format_R16_Unorm:
                        return GL_R16;
                    case Graphics::Format_R16_Uint:
                        return GL_R16UI;
                    case Graphics::Format_R16_Snorm:
                        return GL_R16I;
                    case Graphics::Format_R16_Sint:
                        return GL_R16I;
                    case Graphics::Format_R8_Typeless:
                        return GL_R8UI;
                    case Graphics::Format_R8_Unorm:
                        return GL_R8;
                    case Graphics::Format_R8_Uint:
                        return GL_R8UI;
                    case Graphics::Format_R8_Snorm:
                        return GL_R8I;
                    case Graphics::Format_R8_Sint:
                        return GL_R8I;
                    case Graphics::Format_A8_Unorm:
                        return GL_ALPHA8_EXT;
                    case Graphics::Format_R1_Unorm:
                        return GL_R8;
                    case Graphics::Format_R9G9B9E5_Share_Dexp:
                        return GL_RGB9_E5;
                    case Graphics::Format_R8G8_B8G8_Unorm:
                        return GL_RGB;
                    default:
                        break;
                }

                return GL_RGB;
            }
            GLenum OGLDevice::GetTextureAddress(Graphics::TextureAddress Value)
            {
                switch (Value)
                {
                    case Graphics::TextureAddress_Wrap:
                        return GL_REPEAT;
                    case Graphics::TextureAddress_Mirror:
                        return GL_MIRRORED_REPEAT;
                    case Graphics::TextureAddress_Clamp:
                        return GL_CLAMP_TO_EDGE;
                    case Graphics::TextureAddress_Border:
                        return GL_CLAMP_TO_BORDER;
                    case Graphics::TextureAddress_Mirror_Once:
                        return GL_MIRROR_CLAMP_TO_EDGE;
                }

                return GL_REPEAT;
            }
            GLenum OGLDevice::GetComparison(Graphics::Comparison Value)
            {
                switch (Value)
                {
                    case Graphics::Comparison_Never:
                        return GL_NEVER;
                    case Graphics::Comparison_Less:
                        return GL_LESS;
                    case Graphics::Comparison_Equal:
                        return GL_EQUAL;
                    case Graphics::Comparison_Less_Equal:
                        return GL_LEQUAL;
                    case Graphics::Comparison_Greater:
                        return GL_GREATER;
                    case Graphics::Comparison_Not_Equal:
                        return GL_NOTEQUAL;
                    case Graphics::Comparison_Greater_Equal:
                        return GL_GEQUAL;
                    case Graphics::Comparison_Always:
                        return GL_ALWAYS;
                }

                return GL_ALWAYS;
            }
            GLenum OGLDevice::GetPixelFilter(Graphics::PixelFilter Value, bool Mag)
            {
                switch (Value)
                {
                    case Graphics::PixelFilter_Min_Mag_Mip_Point:
                    case Graphics::PixelFilter_Compare_Min_Mag_Mip_Point:
                        return GL_NEAREST;
                    case Graphics::PixelFilter_Min_Mag_Point_Mip_Linear:
                    case Graphics::PixelFilter_Compare_Min_Mag_Point_Mip_Linear:
                        return (Mag ? GL_NEAREST : GL_NEAREST_MIPMAP_LINEAR);
                    case Graphics::PixelFilter_Min_Point_Mag_Linear_Mip_Point:
                    case Graphics::PixelFilter_Compare_Min_Point_Mag_Linear_Mip_Point:
                        return (Mag ? GL_LINEAR : GL_NEAREST_MIPMAP_NEAREST);
                    case Graphics::PixelFilter_Min_Point_Mag_Mip_Linear:
                    case Graphics::PixelFilter_Compare_Min_Point_Mag_Mip_Linear:
                        return (Mag ? GL_LINEAR : GL_NEAREST_MIPMAP_LINEAR);
                    case Graphics::PixelFilter_Min_Linear_Mag_Mip_Point:
                    case Graphics::PixelFilter_Compare_Min_Linear_Mag_Mip_Point:
                        return (Mag ? GL_NEAREST : GL_LINEAR_MIPMAP_NEAREST);
                    case Graphics::PixelFilter_Min_Linear_Mag_Point_Mip_Linear:
                    case Graphics::PixelFilter_Compare_Min_Linear_Mag_Point_Mip_Linear:
                        return (Mag ? GL_NEAREST : GL_LINEAR_MIPMAP_LINEAR);
                    case Graphics::PixelFilter_Min_Mag_Linear_Mip_Point:
                    case Graphics::PixelFilter_Compare_Min_Mag_Linear_Mip_Point:
                        return (Mag ? GL_LINEAR : GL_LINEAR_MIPMAP_NEAREST);
                    case Graphics::PixelFilter_Min_Mag_Mip_Linear:
                    case Graphics::PixelFilter_Compare_Min_Mag_Mip_Linear:
                        return (Mag ? GL_LINEAR : GL_LINEAR_MIPMAP_LINEAR);
                    case Graphics::PixelFilter_Anistropic:
                    case Graphics::PixelFilter_Compare_Anistropic:
                        return GL_LINEAR;
                }

                return GL_LINEAR;
            }
            GLenum OGLDevice::GetBlendOperation(Graphics::BlendOperation Value)
            {
                switch (Value)
                {
                    case Graphics::BlendOperation_Add:
                        return GL_FUNC_ADD;
                    case Graphics::BlendOperation_Subtract:
                        return GL_FUNC_SUBTRACT;
                    case Graphics::BlendOperation_Subtract_Reverse:
                        return GL_FUNC_REVERSE_SUBTRACT;
                    case Graphics::BlendOperation_Min:
                        return GL_MIN;
                    case Graphics::BlendOperation_Max:
                        return GL_MAX;
                }

                return GL_FUNC_ADD;
            }
            GLenum OGLDevice::GetBlend(Graphics::Blend Value)
            {
                switch (Value)
                {
                    case Graphics::Blend_Zero:
                        return GL_ZERO;
                    case Graphics::Blend_One:
                        return GL_ONE;
                    case Graphics::Blend_Source_Color:
                        return GL_SRC_COLOR;
                    case Graphics::Blend_Source_Color_Invert:
                        return GL_ONE_MINUS_SRC_COLOR;
                    case Graphics::Blend_Source_Alpha:
                        return GL_SRC_ALPHA;
                    case Graphics::Blend_Source_Alpha_Invert:
                        return GL_ONE_MINUS_SRC_ALPHA;
                    case Graphics::Blend_Destination_Alpha:
                        return GL_DST_ALPHA;
                    case Graphics::Blend_Destination_Alpha_Invert:
                        return GL_ONE_MINUS_DST_ALPHA;
                    case Graphics::Blend_Destination_Color:
                        return GL_DST_COLOR;
                    case Graphics::Blend_Destination_Color_Invert:
                        return GL_ONE_MINUS_DST_COLOR;
                    case Graphics::Blend_Source_Alpha_SAT:
                        return GL_SRC_ALPHA_SATURATE;
                    case Graphics::Blend_Blend_Factor:
                        return GL_CONSTANT_COLOR;
                    case Graphics::Blend_Blend_Factor_Invert:
                        return GL_ONE_MINUS_CONSTANT_ALPHA;
                    case Graphics::Blend_Source1_Color:
                        return GL_SRC1_COLOR;
                    case Graphics::Blend_Source1_Color_Invert:
                        return GL_ONE_MINUS_SRC1_COLOR;
                    case Graphics::Blend_Source1_Alpha:
                        return GL_SRC1_ALPHA;
                    case Graphics::Blend_Source1_Alpha_Invert:
                        return GL_ONE_MINUS_SRC1_ALPHA;
                }

                return GL_ONE;
            }
            GLenum OGLDevice::GetStencilOperation(Graphics::StencilOperation Value)
            {
                switch (Value)
                {
                    case Graphics::StencilOperation_Keep:
                        return GL_KEEP;
                    case Graphics::StencilOperation_Zero:
                        return GL_ZERO;
                    case Graphics::StencilOperation_Replace:
                        return GL_REPLACE;
                    case Graphics::StencilOperation_SAT_Add:
                        return GL_INCR_WRAP;
                    case Graphics::StencilOperation_SAT_Subtract:
                        return GL_DECR_WRAP;
                    case Graphics::StencilOperation_Invert:
                        return GL_INVERT;
                    case Graphics::StencilOperation_Add:
                        return GL_INCR;
                    case Graphics::StencilOperation_Subtract:
                        return GL_DECR;
                }

                return GL_KEEP;
            }
            GLenum OGLDevice::GetPrimitiveTopology(Graphics::PrimitiveTopology Value)
            {
                switch (Value)
                {
                    case Graphics::PrimitiveTopology_Point_List:
                        return GL_POINT;
                    case Graphics::PrimitiveTopology_Line_List:
                    case Graphics::PrimitiveTopology_Line_Strip:
                    case Graphics::PrimitiveTopology_Line_List_Adj:
                    case Graphics::PrimitiveTopology_Line_Strip_Adj:
                        return GL_LINE;
                    case Graphics::PrimitiveTopology_Triangle_List:
                    case Graphics::PrimitiveTopology_Triangle_Strip:
                    case Graphics::PrimitiveTopology_Triangle_List_Adj:
                    case Graphics::PrimitiveTopology_Triangle_Strip_Adj:
                        return GL_FILL;
                    default:
                        break;
                }

                return GL_FILL;
            }
            GLenum OGLDevice::GetPrimitiveTopologyDraw(Graphics::PrimitiveTopology Value)
            {
                switch (Value)
                {
                    case Graphics::PrimitiveTopology_Point_List:
                        return GL_POINTS;
                    case Graphics::PrimitiveTopology_Line_List:
                        return GL_LINES;
                    case Graphics::PrimitiveTopology_Line_Strip:
                        return GL_LINE_STRIP;
                    case Graphics::PrimitiveTopology_Line_List_Adj:
                        return GL_LINES_ADJACENCY;
                    case Graphics::PrimitiveTopology_Line_Strip_Adj:
                        return GL_LINE_STRIP_ADJACENCY;
                    case Graphics::PrimitiveTopology_Triangle_List:
                        return GL_TRIANGLES;
                    case Graphics::PrimitiveTopology_Triangle_Strip:
                        return GL_TRIANGLE_STRIP;
                    case Graphics::PrimitiveTopology_Triangle_List_Adj:
                        return GL_TRIANGLES_ADJACENCY;
                    case Graphics::PrimitiveTopology_Triangle_Strip_Adj:
                        return GL_TRIANGLE_STRIP_ADJACENCY;
                    default:
                        break;
                }

                return GL_TRIANGLES;
            }
            GLenum OGLDevice::GetResourceBind(Graphics::ResourceBind Value)
            {
                switch (Value)
                {
                    case Graphics::ResourceBind_Vertex_Buffer:
                        return GL_ARRAY_BUFFER;
                    case Graphics::ResourceBind_Index_Buffer:
                        return GL_ELEMENT_ARRAY_BUFFER;
                    case Graphics::ResourceBind_Constant_Buffer:
                        return GL_UNIFORM_BUFFER;
                    case Graphics::ResourceBind_Shader_Input:
                        return GL_SHADER_STORAGE_BUFFER;
                    case Graphics::ResourceBind_Stream_Output:
                        return GL_TEXTURE_BUFFER;
                    case Graphics::ResourceBind_Render_Target:
                        return GL_DRAW_INDIRECT_BUFFER;
                    case Graphics::ResourceBind_Depth_Stencil:
                        return GL_DRAW_INDIRECT_BUFFER;
                    case Graphics::ResourceBind_Unordered_Access:
                        return GL_DISPATCH_INDIRECT_BUFFER;
                }

                return GL_ARRAY_BUFFER;
            }
            GLenum OGLDevice::GetResourceMap(Graphics::ResourceMap Value)
            {
                switch (Value)
                {
                    case Graphics::ResourceMap_Read:
                        return GL_READ_ONLY;
                    case Graphics::ResourceMap_Write:
                        return GL_WRITE_ONLY;
                    case Graphics::ResourceMap_Read_Write:
                        return GL_READ_WRITE;
                    case Graphics::ResourceMap_Write_Discard:
                        return GL_WRITE_DISCARD_NV;
                    case Graphics::ResourceMap_Write_No_Overwrite:
                        return GL_WRITE_ONLY;
                }

                return GL_READ_ONLY;
            }
            void OGLDevice::LoadShaderSections()
            {
#ifdef HAS_OGL_ANIMATION_BUFFER_GLSL
                AddSection("animation-buffer.glsl", reinterpret_cast<const char*>(resource_batch::ogl_animation_buffer_glsl::data));
#else
                THAWK_ERROR("animation-buffer.glsl was not compiled");
#endif
#ifdef HAS_OGL_RENDER_BUFFER_GLSL
                AddSection("render-buffer.glsl", reinterpret_cast<const char*>(resource_batch::ogl_render_buffer_glsl::data));
#else
                THAWK_ERROR("render-buffer.glsl was not compiled");
#endif
#ifdef HAS_OGL_VIEW_BUFFER_GLSL
                AddSection("view-buffer.glsl", reinterpret_cast<const char*>(resource_batch::ogl_view_buffer_glsl::data));
#else
                THAWK_ERROR("view-buffer.glsl was not compiled");
#endif
#ifdef HAS_OGL_VERTEX_IN_GLSL
                AddSection("vertex-in.glsl", reinterpret_cast<const char*>(resource_batch::ogl_vertex_in_glsl::data));
#else
                THAWK_ERROR("vertex-in.glsl was not compiled");
#endif
#ifdef HAS_OGL_VERTEX_OUT_GLSL
                AddSection("vertex-out.glsl", reinterpret_cast<const char*>(resource_batch::ogl_vertex_out_glsl::data));
#else
                THAWK_ERROR("vertex-out.glsl was not compiled");
#endif
#ifdef HAS_OGL_INFLUENCE_VERTEX_IN_GLSL
                AddSection("influence-vertex-in.glsl", reinterpret_cast<const char*>(resource_batch::ogl_influence_vertex_in_glsl::data));
#else
                THAWK_ERROR("influence-vertex-in.glsl was not compiled");
#endif
#ifdef HAS_OGL_INFLUENCE_VERTEX_OUT_GLSL
                AddSection("influence-vertex-out.glsl", reinterpret_cast<const char*>(resource_batch::ogl_influence_vertex_out_glsl::data));
#else
                THAWK_ERROR("influence-vertex-out.glsl was not compiled");
#endif
#ifdef HAS_OGL_SHAPE_VERTEX_IN_GLSL
                AddSection("shape-vertex-in.glsl", reinterpret_cast<const char*>(resource_batch::ogl_shape_vertex_in_glsl::data));
#else
                THAWK_ERROR("shape-vertex-in.glsl was not compiled");
#endif
#ifdef HAS_OGL_SHAPE_VERTEX_OUT_GLSL
                AddSection("shape-vertex-out.glsl", reinterpret_cast<const char*>(resource_batch::ogl_shape_vertex_out_glsl::data));
#else
                THAWK_ERROR("shape-vertex-out.glsl was not compiled");
#endif
#ifdef HAS_OGL_DEFERRED_OUT_GLSL
                AddSection("deferred-out.glsl", reinterpret_cast<const char*>(resource_batch::ogl_deferred_out_glsl::data));
#else
                THAWK_ERROR("deferred-out.glsl was not compiled");
#endif
#ifdef HAS_OGL_INSTANCE_ELEMENT_GLSL
                AddSection("instance-element.glsl", reinterpret_cast<const char*>(resource_batch::ogl_instance_element_glsl::data));
#else
                THAWK_ERROR("instance-element.glsl was not compiled");
#endif
#ifdef HAS_OGL_GEOMETRY_GLSL
                AddSection("geometry.glsl", reinterpret_cast<const char*>(resource_batch::ogl_geometry_glsl::data));
#else
                THAWK_ERROR("geometry.glsl was not compiled");
#endif
#ifdef HAS_OGL_LOAD_GEOMETRY_GLSL
                AddSection("load-geometry.glsl", reinterpret_cast<const char*>(resource_batch::ogl_load_geometry_glsl::data));
#else
                THAWK_ERROR("load-geometry.glsl was not compiled");
#endif
#ifdef HAS_OGL_LOAD_TEXCOORD_GLSL
                AddSection("load-texcoord.glsl", reinterpret_cast<const char*>(resource_batch::ogl_load_texcoord_glsl::data));
#else
                THAWK_ERROR("load-texcoord.glsl was not compiled");
#endif
#ifdef HAS_OGL_LOAD_POSITION_GLSL
                AddSection("load-position.glsl", reinterpret_cast<const char*>(resource_batch::ogl_load_position_glsl::data));
#else
                THAWK_ERROR("load-position.glsl was not compiled");
#endif
#ifdef HAS_OGL_COOK_TORRANCE_GLSL
                AddSection("cook-torrance.glsl", reinterpret_cast<const char*>(resource_batch::ogl_cook_torrance_glsl::data));
#else
                THAWK_ERROR("cook-torrance.glsl was not compiled");
#endif
#ifdef HAS_OGL_HEMI_AMBIENT_GLSL
                AddSection("hemi-ambient.glsl", reinterpret_cast<const char*>(resource_batch::ogl_hemi_ambient_glsl::data));
#else
                THAWK_ERROR("hemi-ambient.glsl was not compiled");
#endif
#ifdef HAS_OGL_HEMI_AMBIENT_GLSL
                AddSection("random-float-2.glsl", reinterpret_cast<const char*>(resource_batch::ogl_random_float_2_glsl::data));
#else
                THAWK_ERROR("random-float-2.glsl was not compiled");
#endif
            }
        }
    }
}
#endif