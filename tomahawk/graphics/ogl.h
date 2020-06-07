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
				virtual ~OGLShader() override;
				void UpdateBuffer(Graphics::GraphicsDevice* Device, const void* Data) override;
				void CreateBuffer(Graphics::GraphicsDevice* Device, size_t Size) override;
				void SetShader(Graphics::GraphicsDevice* Device, unsigned int Type) override;
				void SetBuffer(Graphics::GraphicsDevice* Device, unsigned int Slot, unsigned int Type) override;
			};

			class OGLElementBuffer : public Graphics::ElementBuffer
			{
			public:
				GLuint Resource = GL_INVALID_VALUE;
				GLenum Bind = GL_INVALID_VALUE;

			public:
				OGLElementBuffer(Graphics::GraphicsDevice* Device, const Desc& I);
				virtual ~OGLElementBuffer() override;
				void SetIndexBuffer(Graphics::GraphicsDevice* Device, Graphics::Format Format, unsigned int Offset) override;
				void SetVertexBuffer(Graphics::GraphicsDevice* Device, unsigned int Slot, unsigned int Stride, unsigned int Offset) override;
				void Map(Graphics::GraphicsDevice* Device, Graphics::ResourceMap Mode, Graphics::MappedSubresource* Map) override;
				void Unmap(Graphics::GraphicsDevice* Device, Graphics::MappedSubresource* Map) override;
				void* GetResource() override;
			};

			class OGLStructureBuffer : public Graphics::StructureBuffer
			{
			public:
				GLuint Resource = GL_INVALID_VALUE;
				GLenum Bind = GL_INVALID_VALUE;

			public:
				OGLStructureBuffer(Graphics::GraphicsDevice* Device, const Desc& I);
				virtual ~OGLStructureBuffer() override;
				void Map(Graphics::GraphicsDevice* Device, Graphics::ResourceMap Mode, Graphics::MappedSubresource* Map) override;
				void RemapSubresource(Graphics::GraphicsDevice* Device, void* Pointer, uint64_t Size) override;
				void Unmap(Graphics::GraphicsDevice* Device, Graphics::MappedSubresource* Map) override;
				void SetBuffer(Graphics::GraphicsDevice* Device, int Slot) override;
				void* GetElement() override;
				void* GetResource() override;
			};

			class OGLTexture2D : public Graphics::Texture2D
			{
			public:
				GLuint Resource = GL_INVALID_VALUE;

			public:
				OGLTexture2D(Graphics::GraphicsDevice* Device);
				OGLTexture2D(Graphics::GraphicsDevice* Device, const Desc& I);
				virtual ~OGLTexture2D() override;
				void SetTexture(Graphics::GraphicsDevice* Device, int Slot) override;
				void* GetResource() override;
				void Fill(OGLDevice* Device);
			};

			class OGLTexture3D : public Graphics::Texture3D
			{
			public:
				//OpenGL 4.5 Needs

			public:
				OGLTexture3D(Graphics::GraphicsDevice* Device);
				virtual ~OGLTexture3D() override;
				void SetTexture(Graphics::GraphicsDevice* Device, int Slot) override;
				void* GetResource() override;
			};

			class OGLTextureCube : public Graphics::TextureCube
			{
			public:
				//OpenGL 4.5 Needs

			public:
				OGLTextureCube(Graphics::GraphicsDevice* Device);
				OGLTextureCube(Graphics::GraphicsDevice* Device, const Desc& I);
				virtual ~OGLTextureCube() override;
				void SetTexture(Graphics::GraphicsDevice* Device, int Slot) override;
				void* GetResource() override;
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
				virtual ~OGLRenderTarget2D() override;
				void SetTarget(Graphics::GraphicsDevice* Device, float R, float G, float B) override;
				void SetTarget(Graphics::GraphicsDevice* Device) override;
				void Clear(Graphics::GraphicsDevice* Device, float R, float G, float B) override;
				void CopyTexture2D(Graphics::GraphicsDevice* Device, Graphics::Texture2D** Value) override;
				void SetViewport(const Graphics::Viewport& In) override;
				Graphics::Viewport GetViewport() override;
				float GetWidth() override;
				float GetHeight() override;
				void* GetResource() override;
			};

			class OGLMultiRenderTarget2D : public Graphics::MultiRenderTarget2D
			{
			public:
				//OpenGL 4.5 Needs

			public:
				OGLMultiRenderTarget2D(Graphics::GraphicsDevice* Device, const Desc& I);
				virtual ~OGLMultiRenderTarget2D() override;
				void SetTarget(Graphics::GraphicsDevice* Device, int Target, float R, float G, float B) override;
				void SetTarget(Graphics::GraphicsDevice* Device, int Target) override;
				void SetTarget(Graphics::GraphicsDevice* Device, float R, float G, float B) override;
				void SetTarget(Graphics::GraphicsDevice* Device) override;
				void Clear(Graphics::GraphicsDevice* Device, int Target, float R, float G, float B) override;
				void CopyTargetTo(int Target, GraphicsDevice* Device, RenderTarget2D* Output) override;
				void CopyTargetFrom(int Target, GraphicsDevice* Device, RenderTarget2D* Output) override;
				void CopyTexture2D(int Target, GraphicsDevice* Device, Texture2D** Value) override;
				void CopyBegin(GraphicsDevice* Device, int Target, unsigned int MipLevels, unsigned int Size) override;
				void CopyFace(GraphicsDevice* Device, int Target, int Face) override;
				void CopyEnd(GraphicsDevice* Device, TextureCube* Value) override;
				void SetViewport(const Graphics::Viewport& In) override;
				Graphics::Viewport GetViewport() override;
				float GetWidth() override;
				float GetHeight() override;
				void* GetResource(int Id) override;
			};

			class OGLRenderTarget2DArray : public Graphics::RenderTarget2DArray
			{
			public:
				//OpenGL 4.5 Needs

			public:
				OGLRenderTarget2DArray(Graphics::GraphicsDevice* Device, const Desc& I);
				virtual ~OGLRenderTarget2DArray() override;
				void SetTarget(Graphics::GraphicsDevice* Device, int Target, float R, float G, float B) override;
				void SetTarget(Graphics::GraphicsDevice* Device, int Target) override;
				void Clear(Graphics::GraphicsDevice* Device, int Target, float R, float G, float B) override;
				void SetViewport(const Graphics::Viewport& In) override;
				Graphics::Viewport GetViewport() override;
				float GetWidth() override;
				float GetHeight() override;
				void* GetResource() override;
			};

			class OGLRenderTargetCube : public Graphics::RenderTargetCube
			{
			public:
				//OpenGL 4.5 Needs

			public:
				OGLRenderTargetCube(Graphics::GraphicsDevice* Device, const Desc& I);
				virtual ~OGLRenderTargetCube() override;
				void SetTarget(Graphics::GraphicsDevice* Device, float R, float G, float B) override;
				void SetTarget(Graphics::GraphicsDevice* Device) override;
				void Clear(Graphics::GraphicsDevice* Device, float R, float G, float B) override;
				void CopyTextureCube(Graphics::GraphicsDevice* Device, Graphics::TextureCube** Value) override;
				void CopyTexture2D(int FaceId, Graphics::GraphicsDevice* Device, Graphics::Texture2D** Value) override;
				void SetViewport(const Graphics::Viewport& In) override;
				Graphics::Viewport GetViewport() override;
				float GetWidth() override;
				float GetHeight() override;
				void* GetResource() override;
			};

			class OGLMultiRenderTargetCube : public Graphics::MultiRenderTargetCube
			{
			public:
				Graphics::SurfaceTarget SVTarget;
				//OpenGL 4.5 Needs

			public:
				OGLMultiRenderTargetCube(Graphics::GraphicsDevice* Device, const Desc& I);
				virtual ~OGLMultiRenderTargetCube() override;
				void SetTarget(Graphics::GraphicsDevice* Device, int Target, float R, float G, float B) override;
				void SetTarget(Graphics::GraphicsDevice* Device, int Target) override;
				void SetTarget(Graphics::GraphicsDevice* Device, float R, float G, float B) override;
				void SetTarget(Graphics::GraphicsDevice* Device) override;
				void Clear(Graphics::GraphicsDevice* Device, int Target, float R, float G, float B) override;
				void CopyTextureCube(int CubeId, Graphics::GraphicsDevice* Device, Graphics::TextureCube** Value) override;
				void CopyTexture2D(int CubeId, int FaceId, Graphics::GraphicsDevice* Device, Graphics::Texture2D** Value) override;
				void SetViewport(const Graphics::Viewport& In) override;
				Graphics::Viewport GetViewport() override;
				float GetWidth() override;
				float GetHeight() override;
				void* GetResource(int Id) override;
			};

			class OGLMesh : public Graphics::Mesh
			{
			public:
				OGLMesh(Graphics::GraphicsDevice* Device, const Desc& I);
				void Update(Graphics::GraphicsDevice* Device, Compute::Vertex* Elements) override;
				void Draw(Graphics::GraphicsDevice* Device) override;
				Compute::Vertex* Elements(Graphics::GraphicsDevice* Device) override;
			};

			class OGLSkinnedMesh : public Graphics::SkinnedMesh
			{
			public:
				OGLSkinnedMesh(Graphics::GraphicsDevice* Device, const Desc& I);
				void Update(Graphics::GraphicsDevice* Device, Compute::InfluenceVertex* Elements) override;
				void Draw(Graphics::GraphicsDevice* Device) override;
				Compute::InfluenceVertex* Elements(Graphics::GraphicsDevice* Device) override;
			};

			class OGLInstanceBuffer : public Graphics::InstanceBuffer
			{
			private:
				bool SynchronizationState;

			public:
				//OpenGL 4.5 Needs

			public:
				OGLInstanceBuffer(Graphics::GraphicsDevice* Device, const Desc& I);
				virtual ~OGLInstanceBuffer() override;
				void SetResource(GraphicsDevice* Device, int Slot) override;
				void Update() override;
				void Restore() override;
				void Resize(uint64_t Size) override;
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
				virtual ~OGLDirectBuffer() override;
				void Begin() override;
				void End() override;
				void EmitVertex() override;
				void Position(float X, float Y, float Z) override;
				void TexCoord(float X, float Y) override;
				void Color(float X, float Y, float Z, float W) override;
				void Texture(Graphics::Texture2D* InView) override;
				void Intensity(float Intensity) override;
				void TexCoordOffset(float X, float Y) override;
				void Transform(Compute::Matrix4x4 Input) override;
				void Topology(Graphics::PrimitiveTopology DrawTopology) override;
				void AllocVertexBuffer(uint64_t Size);
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
				virtual ~OGLDevice() override;
				void RestoreSamplerStates() override;
				void RestoreBlendStates() override;
				void RestoreRasterizerStates() override;
				void RestoreDepthStencilStates() override;
				void RestoreState(Graphics::DeviceState* RefState) override;
				void ReleaseState(Graphics::DeviceState** RefState) override;
				void RestoreState() override;
				void SetConstantBuffers() override;
				void SetShaderModel(Graphics::ShaderModel RShaderModel) override;
				uint64_t AddDepthStencilState(Graphics::DepthStencilState* In) override;
				uint64_t AddBlendState(Graphics::BlendState* In) override;
				uint64_t AddRasterizerState(Graphics::RasterizerState* In) override;
				uint64_t AddSamplerState(Graphics::SamplerState* In) override;
				void SetSamplerState(uint64_t State) override;
				void SetBlendState(uint64_t State) override;
				void SetRasterizerState(uint64_t State) override;
				void SetDepthStencilState(uint64_t State) override;
				void UpdateBuffer(Graphics::RenderBufferType Buffer) override;
				void Present() override;
				void SetPrimitiveTopology(Graphics::PrimitiveTopology Topology) override;
				void RestoreTexture2D(int Slot, int Size) override;
				void RestoreTexture2D(int Size) override;
				void RestoreTexture3D(int Slot, int Size) override;
				void RestoreTexture3D(int Size) override;
				void RestoreTextureCube(int Slot, int Size) override;
				void RestoreTextureCube(int Size) override;
				void RestoreShader(unsigned int Type) override;
				void RestoreVertexBuffer(int Slot) override;
				void RestoreIndexBuffer() override;
				void SetViewport(unsigned int Count, Viewport* Viewports) override;
				void SetScissorRect(unsigned int Count, Rectangle* Value) override;
				void GetViewport(unsigned int* Count, Viewport* Out) override;
				void GetScissorRect(unsigned int* Count, Rectangle* Out) override;
				void ResizeBuffers(unsigned int Width, unsigned int Height) override;
				void DrawIndexed(unsigned int Count, unsigned int IndexLocation, unsigned int BaseLocation) override;
				void Draw(unsigned int Count, unsigned int Start) override;
				PrimitiveTopology GetPrimitiveTopology() override;
				Graphics::ShaderModel GetSupportedShaderModel() override;
				void* GetDevice() override;
				void* GetContext() override;
				void* GetBackBuffer() override;
				void* GetBackBufferMSAA() override;
				void* GetBackBufferNoAA() override;
				Graphics::DeviceState* CreateState() override;
				void LoadShaderSections() override;
				bool IsValid() override;
				const char* GetShaderVersion();
				void CopyConstantBuffer(GLuint Id, void* Buffer, size_t Size);
				int CreateConstantBuffer(int Size, GLuint& Buffer);
				std::string CompileState(GLuint Handle);

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
				static void APIENTRY DebugMessage(GLenum Source, GLenum Type, GLuint Id, GLenum Severity, GLsizei Length, const GLchar* Message, const void* Data);
			};
		}
	}
}
#endif
#endif
#endif