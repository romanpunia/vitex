#ifndef THAWK_GRAPHICS_OPENGL45_H
#define THAWK_GRAPHICS_OPENGL45_H

#include "../core/graphics.h"
#ifdef THAWK_MICROSOFT
#include <Windows.h>
#endif
#ifdef THAWK_HAS_OPENGL
#ifdef THAWK_HAS_GLEW
#include <GL/glew.h>
#define THAWK_HAS_GL
#ifndef APIENTRY
#define APIENTRY
#endif

namespace Tomahawk
{
	namespace Graphics
	{
		namespace OGL
		{
			class OGLDevice;

			class OGLDepthStencilState : public DepthStencilState
			{
				friend OGLDevice;

			public:
				OGLDepthStencilState(const Desc& I);
				virtual ~OGLDepthStencilState() override;
				void* GetResource() override;
			};

			class OGLRasterizerState : public RasterizerState
			{
				friend OGLDevice;

			public:
				OGLRasterizerState(const Desc& I);
				virtual ~OGLRasterizerState() override;
				void* GetResource() override;
			};

			class OGLBlendState : public BlendState
			{
				friend OGLDevice;

			public:
				OGLBlendState(const Desc& I);
				virtual ~OGLBlendState() override;
				void* GetResource() override;
			};

			class OGLSamplerState : public SamplerState
			{
				friend OGLDevice;

			private:
				GLenum Resource = GL_INVALID_VALUE;

			public:
				OGLSamplerState(const Desc& I);
				virtual ~OGLSamplerState() override;
				void* GetResource() override;
			};

			class OGLShader : public Shader
			{
				friend OGLDevice;

			public:
				OGLShader(const Desc& I);
				virtual ~OGLShader() override;
				bool IsValid() override;
			};

			class OGLElementBuffer : public ElementBuffer
			{
				friend OGLDevice;

			private:
				GLuint Resource = GL_INVALID_VALUE;
				GLenum Flags = GL_INVALID_VALUE;

			public:
				OGLElementBuffer(const Desc& I);
				virtual ~OGLElementBuffer() override;
				void* GetResource() override;
			};

			class OGLStructureBuffer : public StructureBuffer
			{
				friend OGLDevice;

			private:
				GLuint Resource = GL_INVALID_VALUE;
				GLenum Flags = GL_INVALID_VALUE;

			public:
				OGLStructureBuffer(const Desc& I);
				virtual ~OGLStructureBuffer() override;
				void* GetElement() override;
				void* GetResource() override;
			};

			class OGLMeshBuffer : public MeshBuffer
			{
				friend OGLDevice;

			public:
				OGLMeshBuffer(const Desc& I);
				Compute::Vertex* GetElements(GraphicsDevice* Device) override;
			};

			class OGLSkinMeshBuffer : public SkinMeshBuffer
			{
				friend OGLDevice;

			public:
				OGLSkinMeshBuffer(const Desc& I);
				Compute::SkinVertex* GetElements(GraphicsDevice* Device) override;
			};

			class OGLInstanceBuffer : public InstanceBuffer
			{
				friend OGLDevice;

			public:
				OGLInstanceBuffer(const Desc& I);
				virtual ~OGLInstanceBuffer() override;
			};

			class OGLTexture2D : public Texture2D
			{
				friend OGLDevice;

			private:
				GLuint Resource = GL_INVALID_VALUE;

			public:
				OGLTexture2D();
				OGLTexture2D(const Desc& I);
				virtual ~OGLTexture2D() override;
				void* GetResource() override;
				void Fill(OGLDevice* Device);
			};

			class OGLTexture3D : public Texture3D
			{
				friend OGLDevice;

			public:
				OGLTexture3D();
				virtual ~OGLTexture3D() override;
				void* GetResource() override;
			};

			class OGLTextureCube : public TextureCube
			{
				friend OGLDevice;

			public:
				OGLTextureCube();
				OGLTextureCube(const Desc& I);
				virtual ~OGLTextureCube() override;
				void* GetResource() override;
			};

			class OGLDepthBuffer : public DepthBuffer
			{
				friend OGLDevice;

			public:
				GLuint FrameBuffer = GL_INVALID_VALUE;
				GLuint DepthBuffer = GL_INVALID_VALUE;
				GLuint Texture = GL_INVALID_VALUE;
				Viewport View;

			public:
				OGLDepthBuffer(const Desc& I);
				virtual ~OGLDepthBuffer() override;
				Graphics::Viewport GetViewport() override;
				float GetWidth() override;
				float GetHeight() override;
				void* GetResource() override;
			};
			
			class OGLRenderTarget2D : public RenderTarget2D
			{
				friend OGLDevice;

			public:
				GLuint FrameBuffer = GL_INVALID_VALUE;
				GLuint DepthBuffer = GL_INVALID_VALUE;
				GLuint Texture = GL_INVALID_VALUE;
				Viewport View;

			public:
				OGLRenderTarget2D(const Desc& I);
				virtual ~OGLRenderTarget2D() override;
				Graphics::Viewport GetViewport() override;
				float GetWidth() override;
				float GetHeight() override;
				void* GetResource() override;
			};

			class OGLMultiRenderTarget2D : public MultiRenderTarget2D
			{
				friend OGLDevice;

			public:
				Viewport View;

			public:
				OGLMultiRenderTarget2D(const Desc& I);
				virtual ~OGLMultiRenderTarget2D() override;
				Graphics::Viewport GetViewport() override;
				float GetWidth() override;
				float GetHeight() override;
				void* GetResource(int Id) override;
			};

			class OGLRenderTarget2DArray : public RenderTarget2DArray
			{
				friend OGLDevice;

			public:
				Viewport View;

			public:
				OGLRenderTarget2DArray(const Desc& I);
				virtual ~OGLRenderTarget2DArray() override;
				Graphics::Viewport GetViewport() override;
				float GetWidth() override;
				float GetHeight() override;
				void* GetResource() override;
			};

			class OGLRenderTargetCube : public RenderTargetCube
			{
				friend OGLDevice;

			public:
				Viewport View;

			public:
				OGLRenderTargetCube(const Desc& I);
				virtual ~OGLRenderTargetCube() override;
				Graphics::Viewport GetViewport() override;
				float GetWidth() override;
				float GetHeight() override;
				void* GetResource() override;
			};

			class OGLMultiRenderTargetCube : public MultiRenderTargetCube
			{
				friend OGLDevice;

			public:
				Viewport View;

			public:
				OGLMultiRenderTargetCube(const Desc& I);
				virtual ~OGLMultiRenderTargetCube() override;
				Graphics::Viewport GetViewport() override;
				float GetWidth() override;
				float GetHeight() override;
				void* GetResource(int Id) override;
			};

			class OGLQuery : public Query
			{
			public:
				OGLQuery();
				virtual ~OGLQuery() override;
				void* GetResource() override;
			};

			class OGLDevice : public GraphicsDevice
			{
			private:
				struct ConstantBuffer
				{
					Compute::Matrix4x4 WorldViewProjection;
					Compute::Vector4 Padding;
				};

			private:
				const char* ShaderVersion;
				ConstantBuffer Direct;

			public:
				GLuint ConstantBuffer[3];
				Activity* Window;
				PrimitiveTopology Topology;
				void* Context;

			public:
				OGLDevice(const Desc& I);
				virtual ~OGLDevice() override;
				void SetConstantBuffers() override;
				void SetShaderModel(ShaderModel Model) override;
				void SetSamplerState(SamplerState* State) override;
				void SetBlendState(BlendState* State) override;
				void SetRasterizerState(RasterizerState* State) override;
				void SetDepthStencilState(DepthStencilState* State) override;
				void SetShader(Shader* Resource, unsigned int Type) override;
				void SetBuffer(Shader* Resource, unsigned int Slot, unsigned int Type) override;
				void SetBuffer(StructureBuffer* Resource, unsigned int Slot) override;
				void SetBuffer(InstanceBuffer* Resource, unsigned int Slot) override;
				void SetIndexBuffer(ElementBuffer* Resource, Format FormatMode, unsigned int Offset) override;
				void SetVertexBuffer(ElementBuffer* Resource, unsigned int Slot, unsigned int Stride, unsigned int Offset) override;
				void SetTexture2D(Texture2D* Resource, unsigned int Slot) override;
				void SetTexture3D(Texture3D* Resource, unsigned int Slot) override;
				void SetTextureCube(TextureCube* Resource, unsigned int Slot) override;
				void SetTarget(float R, float G, float B) override;
				void SetTarget() override;
				void SetTarget(DepthBuffer* Resource) override;
				void SetTarget(RenderTarget2D* Resource, float R, float G, float B) override;
				void SetTarget(RenderTarget2D* Resource) override;
				void SetTarget(MultiRenderTarget2D* Resource, unsigned int Target, float R, float G, float B) override;
				void SetTarget(MultiRenderTarget2D* Resource, unsigned int Target) override;
				void SetTarget(MultiRenderTarget2D* Resource, float R, float G, float B) override;
				void SetTarget(MultiRenderTarget2D* Resource) override;
				void SetTarget(RenderTarget2DArray* Resource, unsigned int Target, float R, float G, float B) override;
				void SetTarget(RenderTarget2DArray* Resource, unsigned int Target) override;
				void SetTarget(RenderTargetCube* Resource, float R, float G, float B) override;
				void SetTarget(RenderTargetCube* Resource) override;
				void SetTarget(MultiRenderTargetCube* Resource, unsigned int Target, float R, float G, float B) override;
				void SetTarget(MultiRenderTargetCube* Resource, unsigned int Target) override;
				void SetTarget(MultiRenderTargetCube* Resource, float R, float G, float B) override;
				void SetTarget(MultiRenderTargetCube* Resource) override;
				void SetTargetMap(MultiRenderTarget2D* Resource, bool Enabled[8]) override;
				void SetViewport(const Viewport& In) override;
				void SetViewport(RenderTarget2D* Resource, const Viewport& In) override;
				void SetViewport(MultiRenderTarget2D* Resource, const Viewport& In) override;
				void SetViewport(RenderTarget2DArray* Resource, const Viewport& In) override;
				void SetViewport(RenderTargetCube* Resource, const Viewport& In) override;
				void SetViewport(MultiRenderTargetCube* Resource, const Viewport& In) override;
				void SetViewports(unsigned int Count, Viewport* Viewports) override;
				void SetScissorRects(unsigned int Count, Rectangle* Value) override;
				void SetPrimitiveTopology(PrimitiveTopology Topology) override;
				void FlushTexture2D(unsigned int Slot, unsigned int Count) override;
				void FlushTexture3D(unsigned int Slot, unsigned int Count) override;
				void FlushTextureCube(unsigned int Slot, unsigned int Count) override;
				void FlushState() override;
				bool Map(ElementBuffer* Resource, ResourceMap Mode, MappedSubresource* Map) override;
				bool Map(StructureBuffer* Resource, ResourceMap Mode, MappedSubresource* Map) override;
				bool Unmap(ElementBuffer* Resource, MappedSubresource* Map) override;
				bool Unmap(StructureBuffer* Resource, MappedSubresource* Map) override;
				bool UpdateBuffer(StructureBuffer* Resource, void* Data, uint64_t Size) override;
				bool UpdateBuffer(Shader* Resource, const void* Data) override;
				bool UpdateBuffer(MeshBuffer* Resource, Compute::Vertex* Data) override;
				bool UpdateBuffer(SkinMeshBuffer* Resource, Compute::SkinVertex* Data) override;
				bool UpdateBuffer(InstanceBuffer* Resource) override;
				bool UpdateBuffer(RenderBufferType Buffer) override;
				bool UpdateBufferSize(Shader* Resource, size_t Size) override;
				bool UpdateBufferSize(InstanceBuffer* Resource, uint64_t Size) override;
				void ClearBuffer(InstanceBuffer* Resource) override;
				void Clear(float R, float G, float B) override;
				void Clear(RenderTarget2D* Resource, float R, float G, float B) override;
				void Clear(MultiRenderTarget2D* Resource, unsigned int Target, float R, float G, float B) override;
				void Clear(RenderTarget2DArray* Resource, unsigned int Target, float R, float G, float B) override;
				void Clear(RenderTargetCube* Resource, float R, float G, float B) override;
				void Clear(MultiRenderTargetCube* Resource, unsigned int Target, float R, float G, float B) override;
				void ClearDepth() override;
				void ClearDepth(DepthBuffer* Resource) override;
				void ClearDepth(RenderTarget2D* Resource) override;
				void ClearDepth(MultiRenderTarget2D* Resource) override;
				void ClearDepth(RenderTarget2DArray* Resource) override;
				void ClearDepth(RenderTargetCube* Resource) override;
				void ClearDepth(MultiRenderTargetCube* Resource) override;
				void DrawIndexed(unsigned int Count, unsigned int IndexLocation, unsigned int BaseLocation) override;
				void DrawIndexed(MeshBuffer* Resource) override;
				void DrawIndexed(SkinMeshBuffer* Resource) override;
				void Draw(unsigned int Count, unsigned int Location) override;
				bool CopyTexture2D(Texture2D** Result) override;
				bool CopyTexture2D(RenderTarget2D* Resource, Texture2D** Result) override;
				bool CopyTexture2D(MultiRenderTarget2D* Resource, unsigned int Target, Texture2D** Result) override;
				bool CopyTexture2D(RenderTargetCube* Resource, unsigned int Face, Texture2D** Result) override;
				bool CopyTexture2D(MultiRenderTargetCube* Resource, unsigned int Cube, unsigned int Face, Texture2D** Result) override;
				bool CopyTextureCube(RenderTargetCube* Resource, TextureCube** Result) override;
				bool CopyTextureCube(MultiRenderTargetCube* Resource, unsigned int Cube, TextureCube** Result) override;
				bool CopyTargetTo(MultiRenderTarget2D* Resource, unsigned int Target, RenderTarget2D* To) override;
				bool CopyTargetFrom(MultiRenderTarget2D* Resource, unsigned int Target, RenderTarget2D* From) override;
				bool CopyTargetDepth(RenderTarget2D* From, RenderTarget2D* To) override;
				bool CopyTargetDepth(MultiRenderTarget2D* From, MultiRenderTarget2D* To) override;
				bool CopyTargetDepth(RenderTarget2DArray* From, RenderTarget2DArray* To) override;
				bool CopyTargetDepth(RenderTargetCube* From, RenderTargetCube* To) override;
				bool CopyTargetDepth(MultiRenderTargetCube* From, MultiRenderTargetCube* To) override;
				bool CopyBegin(MultiRenderTarget2D* Resource, unsigned int Target, unsigned int MipLevels, unsigned int Size) override;
				bool CopyFace(MultiRenderTarget2D* Resource, unsigned int Target, unsigned int Face) override;
				bool CopyEnd(MultiRenderTarget2D* Resource, TextureCube* Result) override;
				void SwapTargetDepth(RenderTarget2D* From, RenderTarget2D* To) override;
				void SwapTargetDepth(MultiRenderTarget2D* From, MultiRenderTarget2D* To) override;
				void SwapTargetDepth(RenderTarget2DArray* From, RenderTarget2DArray* To) override;
				void SwapTargetDepth(RenderTargetCube* From, RenderTargetCube* To) override;
				void SwapTargetDepth(MultiRenderTargetCube* From, MultiRenderTargetCube* To) override;
				void FetchViewports(unsigned int* Count, Viewport* Out) override;
				void FetchScissorRects(unsigned int* Count, Rectangle* Out) override;
				bool ResizeBuffers(unsigned int Width, unsigned int Height) override;
				bool GenerateTexture(Texture2D* Resource) override;
				bool GenerateTexture(Texture3D* Resource) override;
				bool GenerateTexture(TextureCube* Resource) override;
				bool GetQueryData(Query* Resource, uint64_t* Result, bool Flush) override;
				bool GetQueryData(Query* Resource, bool* Result, bool Flush) override;
				void QueryBegin(Query* Resource) override;
				void QueryEnd(Query* Resource) override;
				void GenerateMips(Texture2D* Resource) override;
				void GenerateMips(Texture3D* Resource) override;
				void GenerateMips(TextureCube* Resource) override;
				bool DirectBegin() override;
				void DirectTransform(const Compute::Matrix4x4& Transform) override;
				void DirectTopology(PrimitiveTopology Topology) override;
				void DirectEmit() override;
				void DirectTexture(Texture2D* In) override;
				void DirectColor(float X, float Y, float Z, float W) override;
				void DirectIntensity(float Intensity) override;
				void DirectTexCoord(float X, float Y) override;
				void DirectTexCoordOffset(float X, float Y) override;
				void DirectPosition(float X, float Y, float Z) override;
				bool DirectEnd() override;
				bool Submit() override;
				DepthStencilState* CreateDepthStencilState(const DepthStencilState::Desc& I) override;
				BlendState* CreateBlendState(const BlendState::Desc& I) override;
				RasterizerState* CreateRasterizerState(const RasterizerState::Desc& I) override;
				SamplerState* CreateSamplerState(const SamplerState::Desc& I) override;
				Shader* CreateShader(const Shader::Desc& I) override;
				ElementBuffer* CreateElementBuffer(const ElementBuffer::Desc& I) override;
				StructureBuffer* CreateStructureBuffer(const StructureBuffer::Desc& I) override;
				MeshBuffer* CreateMeshBuffer(const MeshBuffer::Desc& I) override;
				SkinMeshBuffer* CreateSkinMeshBuffer(const SkinMeshBuffer::Desc& I) override;
				InstanceBuffer* CreateInstanceBuffer(const InstanceBuffer::Desc& I) override;
				Texture2D* CreateTexture2D() override;
				Texture2D* CreateTexture2D(const Texture2D::Desc& I) override;
				Texture3D* CreateTexture3D() override;
				Texture3D* CreateTexture3D(const Texture3D::Desc& I) override;
				TextureCube* CreateTextureCube() override;
				TextureCube* CreateTextureCube(const TextureCube::Desc& I) override;
				TextureCube* CreateTextureCube(Texture2D* Resource[6]) override;
				TextureCube* CreateTextureCube(Texture2D* Resource) override;
				DepthBuffer* CreateDepthBuffer(const DepthBuffer::Desc& I) override;
				RenderTarget2D* CreateRenderTarget2D(const RenderTarget2D::Desc& I) override;
				MultiRenderTarget2D* CreateMultiRenderTarget2D(const MultiRenderTarget2D::Desc& I) override;
				RenderTarget2DArray* CreateRenderTarget2DArray(const RenderTarget2DArray::Desc& I) override;
				RenderTargetCube* CreateRenderTargetCube(const RenderTargetCube::Desc& I) override;
				MultiRenderTargetCube* CreateMultiRenderTargetCube(const MultiRenderTargetCube::Desc& I) override;
				Query* CreateQuery(const Query::Desc& I) override;
				PrimitiveTopology GetPrimitiveTopology() override;
				ShaderModel GetSupportedShaderModel() override;
				void* GetBackBuffer() override;
				void* GetBackBufferMSAA() override;
				void* GetBackBufferNoAA() override;
				void* GetDevice() override;
				void* GetContext() override;
				bool IsValid() override;
				const char* GetShaderVersion();
				void CopyConstantBuffer(GLuint Buffer, void* Data, size_t Size);
				int CreateConstantBuffer(GLuint* Buffer, size_t Size);
				std::string CompileState(GLuint Handle);

			protected:
				TextureCube* CreateTextureCubeInternal(void* Resources[6]) override;

			public:
				static GLenum GetFormat(Format Value);
				static GLenum GetTextureAddress(TextureAddress Value);
				static GLenum GetComparison(Comparison Value);
				static GLenum GetPixelFilter(PixelFilter Value, bool Mag);
				static GLenum GetBlendOperation(BlendOperation Value);
				static GLenum GetBlend(Blend Value);
				static GLenum GetStencilOperation(StencilOperation Value);
				static GLenum GetPrimitiveTopology(PrimitiveTopology Value);
				static GLenum GetPrimitiveTopologyDraw(PrimitiveTopology Value);
				static GLenum GetResourceBind(ResourceBind Value);
				static GLenum GetResourceMap(ResourceMap Value);
				static void APIENTRY DebugMessage(GLenum Source, GLenum Type, GLuint Id, GLenum Severity, GLsizei Length, const GLchar* Message, const void* Data);
			};
		}
	}
}
#endif
#endif
#endif