#ifndef THAWK_GRAPHICS_OPENGL45_H
#define THAWK_GRAPHICS_OPENGL45_H

#include "../graphics.h"
#ifdef THAWK_MICROSOFT
#include <Windows.h>
#endif
#ifdef THAWK_HAS_OPENGL
#ifdef THAWK_HAS_GLEW
#include <gl/glew.h>
#define THAWK_HAS_GL

namespace Tomahawk
{
    namespace Graphics
    {
        namespace OGL
        {
            class OGLDevice;

            struct OGLDeviceState : public Graphics::DeviceState
            {
                GLboolean EnableBlend;
                GLboolean EnableCullFace;
                GLboolean EnableDepthTest;
                GLboolean EnableScissorTest;
                GLenum ActiveTexture;
                GLenum BlendSrc;
                GLenum BlendDst;
                GLenum BlendSrcAlpha;
                GLenum BlendDstAlpha;
                GLenum BlendEquation;
                GLenum BlendEquationAlpha;
                GLenum ClipOrigin = 0;
                GLint Program;
                GLint Texture;
                GLint Sampler;
                GLint ArrayBuffer;
                GLint VertexArrayObject;
                GLint PolygonMode[2];
                GLint Viewport[4];
                GLint ScissorBox[4];
            };

            class OGLShader : public Graphics::Shader
            {
            public:
                //OpenGL 4.5 Needs

            public:
                OGLShader(Graphics::GraphicsDevice* Device, const Desc& I);
                ~OGLShader();
                void SendConstantStream(Graphics::GraphicsDevice* Device);
                void Apply(Graphics::GraphicsDevice* Device);
            };

            class OGLElementBuffer : public Graphics::ElementBuffer
            {
            public:
                GLuint Resource = GL_INVALID_VALUE;
                GLenum Bind = GL_INVALID_VALUE;

            public:
                OGLElementBuffer(Graphics::GraphicsDevice* Device, const Desc& I);
                ~OGLElementBuffer();
                void IndexedBuffer(Graphics::GraphicsDevice* Device, Graphics::Format Format, unsigned int Offset);
                void VertexBuffer(Graphics::GraphicsDevice* Device, unsigned int Slot, unsigned int Stride, unsigned int Offset);
                void Map(Graphics::GraphicsDevice* Device, Graphics::ResourceMap Mode, Graphics::MappedSubresource* Map);
                void Unmap(Graphics::GraphicsDevice* Device, Graphics::MappedSubresource* Map);
                void* GetResource();
            };

            class OGLStructureBuffer : public Graphics::StructureBuffer
            {
            public:
                GLuint Resource = GL_INVALID_VALUE;
                GLenum Bind = GL_INVALID_VALUE;

            public:
                OGLStructureBuffer(Graphics::GraphicsDevice* Device, const Desc& I);
                ~OGLStructureBuffer();
                void Map(Graphics::GraphicsDevice* Device, Graphics::ResourceMap Mode, Graphics::MappedSubresource* Map);
                void RemapSubresource(Graphics::GraphicsDevice* Device, void* Pointer, UInt64 Size);
                void Unmap(Graphics::GraphicsDevice* Device, Graphics::MappedSubresource* Map);
                void Apply(Graphics::GraphicsDevice* Device, int Slot);
                void* GetElement();
                void* GetResource();
            };

            class OGLTexture2D : public Graphics::Texture2D
            {
            public:
                GLuint Resource = GL_INVALID_VALUE;

            public:
                OGLTexture2D(Graphics::GraphicsDevice* Device);
                OGLTexture2D(Graphics::GraphicsDevice* Device, const Desc& I);
                ~OGLTexture2D();
                void Apply(Graphics::GraphicsDevice* Device, int Slot);
                void Fill(OGLDevice* Device);
                void* GetResource();
            };

            class OGLTexture3D : public Graphics::Texture3D
            {
            public:
                //OpenGL 4.5 Needs

            public:
                OGLTexture3D(Graphics::GraphicsDevice* Device);
                ~OGLTexture3D();
                void Apply(Graphics::GraphicsDevice* Device, int Slot);
                void* GetResource();
            };

            class OGLTextureCube : public Graphics::TextureCube
            {
            public:
                //OpenGL 4.5 Needs

            public:
                OGLTextureCube(Graphics::GraphicsDevice* Device);
                OGLTextureCube(Graphics::GraphicsDevice* Device, const Desc& I);
                ~OGLTextureCube();
                void Apply(Graphics::GraphicsDevice* Device, int Slot);
                void* GetResource();
            };

            class OGLRenderTarget2D : public Graphics::RenderTarget2D
            {
            public:
                Graphics::Viewport Viewport;

            public:
                GLuint FrameBuffer = GL_INVALID_VALUE;
                GLuint DepthBuffer = GL_INVALID_VALUE;
                GLuint Texture = GL_INVALID_VALUE;

            public:
                OGLRenderTarget2D(Graphics::GraphicsDevice* Device, const Desc& I);
                ~OGLRenderTarget2D();
                void Apply(Graphics::GraphicsDevice* Device, float R, float G, float B);
                void Apply(Graphics::GraphicsDevice* Device);
                void Clear(Graphics::GraphicsDevice* Device, float R, float G, float B);
                void CopyTexture2D(Graphics::GraphicsDevice* Device, Graphics::Texture2D** Value);
                void SetViewport(Graphics::Viewport In);
                Graphics::Viewport GetViewport();
                float GetWidth();
                float GetHeight();
                void* GetResource();
            };

            class OGLMultiRenderTarget2D : public Graphics::MultiRenderTarget2D
            {
            public:
                //OpenGL 4.5 Needs

            public:
                OGLMultiRenderTarget2D(Graphics::GraphicsDevice* Device, const Desc& I);
                ~OGLMultiRenderTarget2D();
                void Apply(Graphics::GraphicsDevice* Device, int Target, float R, float G, float B);
                void Apply(Graphics::GraphicsDevice* Device, int Target);
                void Apply(Graphics::GraphicsDevice* Device, float R, float G, float B);
                void Apply(Graphics::GraphicsDevice* Device);
                void Clear(Graphics::GraphicsDevice* Device, int Target, float R, float G, float B);
                void CopyTexture2D(int Target, Graphics::GraphicsDevice* Device, Graphics::Texture2D** Value);
                void SetViewport(Graphics::Viewport In);
                Graphics::Viewport GetViewport();
                float GetWidth();
                float GetHeight();
                void* GetResource(int Id);
            };

            class OGLRenderTarget2DArray : public Graphics::RenderTarget2DArray
            {
            public:
                //OpenGL 4.5 Needs

            public:
                OGLRenderTarget2DArray(Graphics::GraphicsDevice* Device, const Desc& I);
                ~OGLRenderTarget2DArray();
                void Apply(Graphics::GraphicsDevice* Device, int Target, float R, float G, float B);
                void Apply(Graphics::GraphicsDevice* Device, int Target);
                void Clear(Graphics::GraphicsDevice* Device, int Target, float R, float G, float B);
                void SetViewport(Graphics::Viewport In);
                Graphics::Viewport GetViewport();
                float GetWidth();
                float GetHeight();
                void* GetResource();
            };

            class OGLRenderTargetCube : public Graphics::RenderTargetCube
            {
            public:
                //OpenGL 4.5 Needs

            public:
                OGLRenderTargetCube(Graphics::GraphicsDevice* Device, const Desc& I);
                ~OGLRenderTargetCube();
                void Apply(Graphics::GraphicsDevice* Device, float R, float G, float B);
                void Apply(Graphics::GraphicsDevice* Device);
                void Clear(Graphics::GraphicsDevice* Device, float R, float G, float B);
                void CopyTextureCube(Graphics::GraphicsDevice* Device, Graphics::TextureCube** Value);
                void CopyTexture2D(int FaceId, Graphics::GraphicsDevice* Device, Graphics::Texture2D** Value);
                void SetViewport(Graphics::Viewport In);
                Graphics::Viewport GetViewport();
                float GetWidth();
                float GetHeight();
                void* GetResource();
            };

            class OGLMultiRenderTargetCube : public Graphics::MultiRenderTargetCube
            {
            public:
                Graphics::SurfaceTarget SVTarget;
                //OpenGL 4.5 Needs

            public:
                OGLMultiRenderTargetCube(Graphics::GraphicsDevice* Device, const Desc& I);
                ~OGLMultiRenderTargetCube();
                void Apply(Graphics::GraphicsDevice* Device, int Target, float R, float G, float B);
                void Apply(Graphics::GraphicsDevice* Device, int Target);
                void Apply(Graphics::GraphicsDevice* Device, float R, float G, float B);
                void Apply(Graphics::GraphicsDevice* Device);
                void Clear(Graphics::GraphicsDevice* Device, int Target, float R, float G, float B);
                void CopyTextureCube(int CubeId, Graphics::GraphicsDevice* Device, Graphics::TextureCube** Value);
                void CopyTexture2D(int CubeId, int FaceId, Graphics::GraphicsDevice* Device, Graphics::Texture2D** Value);
                void SetViewport(Graphics::Viewport In);
                Graphics::Viewport GetViewport();
                float GetWidth();
                float GetHeight();
                void* GetResource(int Id);
            };

            class OGLMesh : public Graphics::Mesh
            {
            public:
                OGLMesh(Graphics::GraphicsDevice* Device, const Desc& I);
                void UpdateSubresource(Graphics::GraphicsDevice* Device, Compute::Vertex* Elements);
                void Draw(Graphics::GraphicsDevice* Device);
                Compute::Vertex* Elements(Graphics::GraphicsDevice* Device);
            };

            class OGLSkinnedMesh : public Graphics::SkinnedMesh
            {
            public:
                OGLSkinnedMesh(Graphics::GraphicsDevice* Device, const Desc& I);
                void UpdateSubresource(Graphics::GraphicsDevice* Device, Compute::InfluenceVertex* Elements);
                void Draw(Graphics::GraphicsDevice* Device);
                Compute::InfluenceVertex* Elements(Graphics::GraphicsDevice* Device);
            };

            class OGLInstanceBuffer : public Graphics::InstanceBuffer
            {
            private:
                bool SynchronizationState;

            public:
                //OpenGL 4.5 Needs

            public:
                OGLInstanceBuffer(Graphics::GraphicsDevice* Device, const Desc& I);
                ~OGLInstanceBuffer();
                void SendPool();
                void Restore();
                void Resize(UInt64 Size);
            };

            class OGLDirectBuffer : public Graphics::DirectBuffer
            {
            private:
                struct ConstantBuffer
                {
                    Compute::Matrix4x4 WorldViewProjection;
                    Compute::Vector4 Padding;
                };

            private:
                ConstantBuffer Buffer;

            public:
                OGLDirectBuffer(Graphics::GraphicsDevice* NewDevice);
                ~OGLDirectBuffer();
                void Begin();
                void End();
                void EmitVertex();
                void Position(float X, float Y, float Z);
                void TexCoord(float X, float Y);
                void Color(float X, float Y, float Z, float W);
                void Texture(Graphics::Texture2D* InView);
                void Intensity(float Intensity);
                void TexCoordOffset(float X, float Y);
                void Transform(Compute::Matrix4x4 Input);
                void AllocVertexBuffer(UInt64 Size);
                void Topology(Graphics::PrimitiveTopology DrawTopology);
            };

            class OGLDevice : public Graphics::GraphicsDevice
            {
            private:
                const char* ShaderVersion;

            public:
                GLuint ConstantBuffer[3];
                Graphics::Activity* Window;
                Graphics::PrimitiveTopology Topology;
                void* Context;

            public:
                OGLDevice(const Desc& I);
                ~OGLDevice();
                void RestoreSamplerStates();
                void RestoreBlendStates();
                void RestoreRasterizerStates();
                void RestoreDepthStencilStates();
                void RestoreState(Graphics::DeviceState* RefState);
                void ReleaseState(Graphics::DeviceState** RefState);
                void RestoreState();
                void SetConstantBuffers();
                void SetShaderModel(Graphics::ShaderModel RShaderModel);
                UInt64 AddDepthStencilState(Graphics::DepthStencilState* In);
                UInt64 AddBlendState(Graphics::BlendState* In);
                UInt64 AddRasterizerState(Graphics::RasterizerState* In);
                UInt64 AddSamplerState(Graphics::SamplerState* In);
                void SetSamplerState(UInt64 State);
                void SetBlendState(UInt64 State);
                void SetRasterizerState(UInt64 State);
                void SetDepthStencilState(UInt64 State);
                void SendBufferStream(Graphics::RenderBufferType Buffer);
                void Present();
                void SetPrimitiveTopology(Graphics::PrimitiveTopology Topology);
                void RestoreTexture2D(int Slot, int Size);
                void RestoreTexture2D(int Size);
                void RestoreTexture3D(int Slot, int Size);
                void RestoreTexture3D(int Size);
                void RestoreTextureCube(int Slot, int Size);
                void RestoreTextureCube(int Size);
                void RestoreShader();
                void ResizeBuffers(unsigned int Width, unsigned int Height);
                void DrawIndexed(unsigned int Count, unsigned int IndexLocation, unsigned int BaseLocation);
                void Draw(unsigned int Count, unsigned int Start);
                Graphics::ShaderModel GetSupportedShaderModel();
                void* GetDevice();
                void* GetContext();
                void* GetBackBuffer();
                void* GetBackBufferMSAA();
                void* GetBackBufferNoAA();
                Graphics::DeviceState* CreateState();
                const char* GetShaderVersion();
                void CopyConstantBuffer(GLuint Id, void* Buffer, size_t Size);
                int CreateConstantBuffer(int Size, GLuint& Buffer);
                std::string CompileState(GLuint Handle);
                void LoadShaderSections();

            public:
                static GLenum GetFormat(Graphics::Format Value);
                static GLenum GetTextureAddress(Graphics::TextureAddress Value);
                static GLenum GetComparison(Graphics::Comparison Value);
                static GLenum GetPixelFilter(Graphics::PixelFilter Value, bool Mag);
                static GLenum GetBlendOperation(Graphics::BlendOperation Value);
                static GLenum GetBlend(Graphics::Blend Value);
                static GLenum GetStencilOperation(Graphics::StencilOperation Value);
                static GLenum GetPrimitiveTopology(Graphics::PrimitiveTopology Value);
                static GLenum GetPrimitiveTopologyDraw(Graphics::PrimitiveTopology Value);
                static GLenum GetResourceBind(Graphics::ResourceBind Value);
                static GLenum GetResourceMap(Graphics::ResourceMap Value);
            };
        }
    }
}
#endif
#endif
#endif