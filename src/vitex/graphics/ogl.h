#ifndef VI_GRAPHICS_OPENGL45_H
#define VI_GRAPHICS_OPENGL45_H
#include "../core/graphics.h"
#include <array>
#ifdef VI_MICROSOFT
#include <Windows.h>
#endif
#ifdef VI_OPENGL
#ifdef VI_GLEW
#include <GL/glew.h>
#define VI_GL
#ifndef APIENTRY
#define APIENTRY
#endif

namespace Vitex
{
	namespace Graphics
	{
		namespace OGL
		{
			class OGLElementBuffer;

			class OGLDevice;

			struct OGLFrameBuffer
			{
				GLenum Format[8] = { GL_COLOR_ATTACHMENT0, GL_NONE, GL_NONE, GL_NONE, GL_NONE, GL_NONE, GL_NONE, GL_NONE };
				GLuint Texture[8] = { GL_NONE };
				GLuint Buffer = GL_NONE;
				GLuint Count = 1;
				bool Backbuffer = false;

				OGLFrameBuffer(GLuint Targets);
				void Cleanup();
			};

			class OGLDepthStencilState final : public DepthStencilState
			{
				friend OGLDevice;

			public:
				OGLDepthStencilState(const Desc& I);
				~OGLDepthStencilState() override;
				void* GetResource() const override;
			};

			class OGLRasterizerState final : public RasterizerState
			{
				friend OGLDevice;

			public:
				OGLRasterizerState(const Desc& I);
				~OGLRasterizerState() override;
				void* GetResource() const override;
			};

			class OGLBlendState final : public BlendState
			{
				friend OGLDevice;

			public:
				OGLBlendState(const Desc& I);
				~OGLBlendState() override;
				void* GetResource() const override;
			};

			class OGLSamplerState final : public SamplerState
			{
				friend OGLDevice;

			private:
				GLenum Resource = GL_NONE;

			public:
				OGLSamplerState(const Desc& I);
				~OGLSamplerState() override;
				void* GetResource() const override;
			};

			class OGLInputLayout final : public InputLayout
			{
				friend OGLDevice;
				friend OGLElementBuffer;

			public:
				Core::UnorderedMap<size_t, Core::Vector<std::function<void(size_t)>>> VertexLayout;
				Core::UnorderedMap<Core::String, GLuint> Layouts;
				GLuint DynamicResource = GL_NONE;

			public:
				OGLInputLayout(const Desc& I);
				~OGLInputLayout() override;
				void* GetResource() const override;

			public:
				static Core::String GetLayoutHash(OGLElementBuffer** Buffers, unsigned int Count);
			};

			class OGLShader final : public Shader
			{
				friend OGLDevice;

			private:
				bool Compiled;

			public:
				Core::UnorderedMap<GLuint, OGLDevice*> Programs;
				GLuint VertexShader = GL_NONE;
				GLuint PixelShader = GL_NONE;
				GLuint GeometryShader = GL_NONE;
				GLuint DomainShader = GL_NONE;
				GLuint HullShader = GL_NONE;
				GLuint ComputeShader = GL_NONE;
				GLuint ConstantBuffer = GL_NONE;
				size_t ConstantSize = 0;

			public:
				OGLShader(const Desc& I);
				~OGLShader() override;
				bool IsValid() const override;
			};

			class OGLElementBuffer final : public ElementBuffer
			{
				friend OGLDevice;

			private:
				Core::UnorderedMap<GLuint, OGLInputLayout*> Bindings;
				GLuint Resource = GL_NONE;
				GLenum Flags = GL_NONE;

			public:
				OGLElementBuffer(const Desc& I);
				~OGLElementBuffer() override;
				void* GetResource() const override;
			};

			class OGLMeshBuffer final : public MeshBuffer
			{
				friend OGLDevice;

			public:
				OGLMeshBuffer(const Desc& I);
				Core::Unique<Compute::Vertex> GetElements(GraphicsDevice* Device) const override;
			};

			class OGLSkinMeshBuffer final : public SkinMeshBuffer
			{
				friend OGLDevice;

			public:
				OGLSkinMeshBuffer(const Desc& I);
				Core::Unique<Compute::SkinVertex> GetElements(GraphicsDevice* Device) const override;
			};

			class OGLInstanceBuffer final : public InstanceBuffer
			{
				friend OGLDevice;

			public:
				OGLInstanceBuffer(const Desc& I);
				~OGLInstanceBuffer() override;
			};

			class OGLTexture2D final : public Texture2D
			{
				friend OGLDevice;

			public:
				GLenum Format = GL_NONE;
				GLuint Resource = GL_NONE;
				bool Backbuffer = false;

			public:
				OGLTexture2D();
				OGLTexture2D(const Desc& I);
				~OGLTexture2D() override;
				void* GetResource() const override;
			};

			class OGLTexture3D final : public Texture3D
			{
				friend OGLDevice;

			public:
				GLenum Format = GL_NONE;
				GLuint Resource = GL_NONE;

			public:
				OGLTexture3D();
				~OGLTexture3D() override;
				void* GetResource() override;
			};

			class OGLTextureCube final : public TextureCube
			{
				friend OGLDevice;

			public:
				GLenum Format = GL_NONE;
				GLuint Resource = GL_NONE;

			public:
				OGLTextureCube();
				OGLTextureCube(const Desc& I);
				~OGLTextureCube() override;
				void* GetResource() const override;
			};

			class OGLDepthTarget2D final : public DepthTarget2D
			{
				friend OGLDevice;

			public:
				GLuint FrameBuffer = GL_NONE;
				GLuint DepthTexture = GL_NONE;
				bool HasStencilBuffer = false;

			public:
				OGLDepthTarget2D(const Desc& I);
				~OGLDepthTarget2D() override;
				void* GetResource() const override;
				uint32_t GetWidth() const override;
				uint32_t GetHeight() const override;
			};

			class OGLDepthTargetCube final : public DepthTargetCube
			{
				friend OGLDevice;

			public:
				GLuint FrameBuffer = GL_NONE;
				GLuint DepthTexture = GL_NONE;
				bool HasStencilBuffer = false;

			public:
				OGLDepthTargetCube(const Desc& I);
				~OGLDepthTargetCube() override;
				void* GetResource() const override;
				uint32_t GetWidth() const override;
				uint32_t GetHeight() const override;
			};

			class OGLRenderTarget2D final : public RenderTarget2D
			{
				friend OGLDevice;

			public:
				OGLFrameBuffer FrameBuffer;
				GLuint DepthTexture = GL_NONE;
				bool HasStencilBuffer = false;

			public:
				OGLRenderTarget2D(const Desc& I);
				~OGLRenderTarget2D() override;
				void* GetTargetBuffer() const override;
				void* GetDepthBuffer() const override;
				uint32_t GetWidth() const override;
				uint32_t GetHeight() const override;
			};

			class OGLMultiRenderTarget2D final : public MultiRenderTarget2D
			{
				friend OGLDevice;

			public:
				OGLFrameBuffer FrameBuffer;
				GLuint DepthTexture = GL_NONE;

			public:
				OGLMultiRenderTarget2D(const Desc& I);
				~OGLMultiRenderTarget2D() override;
				void* GetTargetBuffer() const override;
				void* GetDepthBuffer() const override;
				uint32_t GetWidth() const override;
				uint32_t GetHeight() const override;
			};

			class OGLRenderTargetCube final : public RenderTargetCube
			{
				friend OGLDevice;

			public:
				OGLFrameBuffer FrameBuffer;
				GLuint DepthTexture = GL_NONE;
				bool HasStencilBuffer = false;

			public:
				OGLRenderTargetCube(const Desc& I);
				~OGLRenderTargetCube() override;
				void* GetTargetBuffer() const override;
				void* GetDepthBuffer() const override;
				uint32_t GetWidth() const override;
				uint32_t GetHeight() const override;
			};

			class OGLMultiRenderTargetCube final : public MultiRenderTargetCube
			{
				friend OGLDevice;

			public:
				OGLFrameBuffer FrameBuffer;
				GLuint DepthTexture = GL_NONE;

			public:
				OGLMultiRenderTargetCube(const Desc& I);
				~OGLMultiRenderTargetCube() override;
				void* GetTargetBuffer() const override;
				void* GetDepthBuffer() const override;
				uint32_t GetWidth() const override;
				uint32_t GetHeight() const override;
			};

			class OGLCubemap final : public Cubemap
			{
				friend OGLDevice;

			public:
				struct
				{
					Format FormatMode;
					GLenum SizeFormat;
				} Options;

			public:
				GLuint FrameBuffer = GL_NONE;
				GLuint Source = GL_NONE;

			public:
				OGLCubemap(const Desc& I);
				~OGLCubemap() override;
			};

			class OGLQuery final : public Query
			{
			public:
				GLuint Async = GL_NONE;
				bool Predicate = false;

			public:
				OGLQuery();
				~OGLQuery() override;
				void* GetResource() const override;
			};

			class OGLDevice final : public GraphicsDevice
			{
				friend OGLShader;

			private:
				struct
				{
					GLuint VertexShader = GL_NONE;
					GLuint PixelShader = GL_NONE;
					GLuint Program = GL_NONE;
					GLuint VertexBuffer = GL_NONE;
					GLuint VertexArray = GL_NONE;
					GLuint Sampler = GL_NONE;
				} Immediate;

				struct
				{
					std::tuple<OGLElementBuffer*, Format> IndexBuffer = { nullptr, Format::Unknown };
					Core::UnorderedMap<uint64_t, GLuint> Programs;
					std::array<OGLElementBuffer*, UNITS_SIZE> VertexBuffers = { };
					std::array<GLuint, UNITS_SIZE> Bindings = { };
					std::array<GLuint, UNITS_SIZE> Textures = { };
					std::array<GLuint, UNITS_SIZE> Samplers = { };
					std::array<OGLShader*, 6> Shaders = { };
					OGLBlendState* Blend = nullptr;
					OGLRasterizerState* Rasterizer = nullptr;
					OGLDepthStencilState* DepthStencil = nullptr;
					OGLInputLayout* Layout = nullptr;
					PrimitiveTopology Primitive = PrimitiveTopology::Triangle_List;
					GLenum IndexFormat = GL_UNSIGNED_INT;
					GLenum DrawTopology = GL_TRIANGLES;
				} Register;

			private:
				const char* ShaderVersion;

			public:
				Activity* Window;
				void* Context;

			public:
				OGLDevice(const Desc& I);
				~OGLDevice() override;
				void SetAsCurrentDevice() override;
				void SetShaderModel(ShaderModel Model) override;
				void SetBlendState(BlendState* State) override;
				void SetRasterizerState(RasterizerState* State) override;
				void SetDepthStencilState(DepthStencilState* State) override;
				void SetInputLayout(InputLayout* State) override;
				ExpectsGraphics<void> SetShader(Shader* Resource, unsigned int Type) override;
				void SetSamplerState(SamplerState* State, unsigned int Slot, unsigned int Count, unsigned int Type) override;
				void SetBuffer(Shader* Resource, unsigned int Slot, unsigned int Type) override;
				void SetBuffer(InstanceBuffer* Resource, unsigned int Slot, unsigned int Type) override;
				void SetConstantBuffer(ElementBuffer* Resource, unsigned int Slot, unsigned int Type) override;
				void SetStructureBuffer(ElementBuffer* Resource, unsigned int Slot, unsigned int Type) override;
				void SetIndexBuffer(ElementBuffer* Resource, Format FormatMode) override;
				void SetVertexBuffers(ElementBuffer** Resources, unsigned int Count, bool DynamicLinkage = false) override;
				void SetTexture2D(Texture2D* Resource, unsigned int Slot, unsigned int Type) override;
				void SetTexture3D(Texture3D* Resource, unsigned int Slot, unsigned int Type) override;
				void SetTextureCube(TextureCube* Resource, unsigned int Slot, unsigned int Type) override;
				void SetWriteable(ElementBuffer** Resource, unsigned int Slot, unsigned int Count, bool Computable) override;
				void SetWriteable(Texture2D** Resource, unsigned int Slot, unsigned int Count, bool Computable) override;
				void SetWriteable(Texture3D** Resource, unsigned int Slot, unsigned int Count, bool Computable) override;
				void SetWriteable(TextureCube** Resource, unsigned int Slot, unsigned int Count, bool Computable) override;
				void SetTarget(float R, float G, float B) override;
				void SetTarget() override;
				void SetTarget(DepthTarget2D* Resource) override;
				void SetTarget(DepthTargetCube* Resource) override;
				void SetTarget(Graphics::RenderTarget* Resource, unsigned int Target, float R, float G, float B) override;
				void SetTarget(Graphics::RenderTarget* Resource, unsigned int Target) override;
				void SetTarget(Graphics::RenderTarget* Resource, float R, float G, float B) override;
				void SetTarget(Graphics::RenderTarget* Resource) override;
				void SetTargetMap(Graphics::RenderTarget* Resource, bool Enabled[8]) override;
				void SetTargetRect(unsigned int Width, unsigned int Height) override;
				void SetViewports(unsigned int Count, Viewport* Viewports) override;
				void SetScissorRects(unsigned int Count, Compute::Rectangle* Value) override;
				void SetPrimitiveTopology(PrimitiveTopology Topology) override;
				void FlushTexture(unsigned int Slot, unsigned int Count, unsigned int Type) override;
				void FlushState() override;
				void ClearBuffer(InstanceBuffer* Resource) override;
				void ClearWritable(Texture2D* Resource) override;
				void ClearWritable(Texture2D* Resource, float R, float G, float B) override;
				void ClearWritable(Texture3D* Resource) override;
				void ClearWritable(Texture3D* Resource, float R, float G, float B) override;
				void ClearWritable(TextureCube* Resource) override;
				void ClearWritable(TextureCube* Resource, float R, float G, float B) override;
				void Clear(float R, float G, float B) override;
				void Clear(Graphics::RenderTarget* Resource, unsigned int Target, float R, float G, float B) override;
				void ClearDepth() override;
				void ClearDepth(DepthTarget2D* Resource) override;
				void ClearDepth(DepthTargetCube* Resource) override;
				void ClearDepth(Graphics::RenderTarget* Resource) override;
				void DrawIndexed(unsigned int Count, unsigned int IndexLocation, unsigned int BaseLocation) override;
				void DrawIndexed(MeshBuffer* Resource) override;
				void DrawIndexed(SkinMeshBuffer* Resource) override;
				void DrawIndexedInstanced(unsigned int IndexCountPerInstance, unsigned int InstanceCount, unsigned int IndexLocation, unsigned int VertexLocation, unsigned int InstanceLocation) override;
				void DrawIndexedInstanced(ElementBuffer* Instances, MeshBuffer* Resource, unsigned int InstanceCount) override;
				void DrawIndexedInstanced(ElementBuffer* Instances, SkinMeshBuffer* Resource, unsigned int InstanceCount) override;
				void Draw(unsigned int Count, unsigned int Location) override;
				void DrawInstanced(unsigned int VertexCountPerInstance, unsigned int InstanceCount, unsigned int VertexLocation, unsigned int InstanceLocation) override;
				void Dispatch(unsigned int GroupX, unsigned int GroupY, unsigned int GroupZ) override;
				void GetViewports(unsigned int* Count, Viewport* Out) override;
				void GetScissorRects(unsigned int* Count, Compute::Rectangle* Out) override;
				void QueryBegin(Query* Resource) override;
				void QueryEnd(Query* Resource) override;
				void GenerateMips(Texture2D* Resource) override;
				void GenerateMips(Texture3D* Resource) override;
				void GenerateMips(TextureCube* Resource) override;
				bool ImBegin() override;
				void ImTransform(const Compute::Matrix4x4& Transform) override;
				void ImTopology(PrimitiveTopology Topology) override;
				void ImEmit() override;
				void ImTexture(Texture2D* In) override;
				void ImColor(float X, float Y, float Z, float W) override;
				void ImIntensity(float Intensity) override;
				void ImTexCoord(float X, float Y) override;
				void ImTexCoordOffset(float X, float Y) override;
				void ImPosition(float X, float Y, float Z) override;
				bool ImEnd() override;
				ExpectsGraphics<void> Submit() override;
				ExpectsGraphics<void> Map(ElementBuffer* Resource, ResourceMap Mode, MappedSubresource* Map) override;
				ExpectsGraphics<void> Map(Texture2D* Resource, ResourceMap Mode, MappedSubresource* Map) override;
				ExpectsGraphics<void> Map(Texture3D* Resource, ResourceMap Mode, MappedSubresource* Map) override;
				ExpectsGraphics<void> Map(TextureCube* Resource, ResourceMap Mode, MappedSubresource* Map) override;
				ExpectsGraphics<void> Unmap(Texture2D* Resource, MappedSubresource* Map) override;
				ExpectsGraphics<void> Unmap(Texture3D* Resource, MappedSubresource* Map) override;
				ExpectsGraphics<void> Unmap(TextureCube* Resource, MappedSubresource* Map) override;
				ExpectsGraphics<void> Unmap(ElementBuffer* Resource, MappedSubresource* Map) override;
				ExpectsGraphics<void> UpdateConstantBuffer(ElementBuffer* Resource, void* Data, size_t Size) override;
				ExpectsGraphics<void> UpdateBuffer(ElementBuffer* Resource, void* Data, size_t Size) override;
				ExpectsGraphics<void> UpdateBuffer(Shader* Resource, const void* Data) override;
				ExpectsGraphics<void> UpdateBuffer(MeshBuffer* Resource, Compute::Vertex* Data) override;
				ExpectsGraphics<void> UpdateBuffer(SkinMeshBuffer* Resource, Compute::SkinVertex* Data) override;
				ExpectsGraphics<void> UpdateBuffer(InstanceBuffer* Resource) override;
				ExpectsGraphics<void> UpdateBufferSize(Shader* Resource, size_t Size) override;
				ExpectsGraphics<void> UpdateBufferSize(InstanceBuffer* Resource, size_t Size) override;
				ExpectsGraphics<void> CopyTexture2D(Texture2D* Resource, Texture2D** Result) override;
				ExpectsGraphics<void> CopyTexture2D(Graphics::RenderTarget* Resource, unsigned int Target, Texture2D** Result) override;
				ExpectsGraphics<void> CopyTexture2D(RenderTargetCube* Resource, Compute::CubeFace Face, Texture2D** Result) override;
				ExpectsGraphics<void> CopyTexture2D(MultiRenderTargetCube* Resource, unsigned int Cube, Compute::CubeFace Face, Texture2D** Result) override;
				ExpectsGraphics<void> CopyTextureCube(RenderTargetCube* Resource, TextureCube** Result) override;
				ExpectsGraphics<void> CopyTextureCube(MultiRenderTargetCube* Resource, unsigned int Cube, TextureCube** Result) override;
				ExpectsGraphics<void> CopyTarget(Graphics::RenderTarget* From, unsigned int FromTarget, Graphics::RenderTarget* To, unsigned ToTarget) override;
				ExpectsGraphics<void> CubemapPush(Cubemap* Resource, TextureCube* Result) override;
				ExpectsGraphics<void> CubemapFace(Cubemap* Resource, Compute::CubeFace Face) override;
				ExpectsGraphics<void> CubemapPop(Cubemap* Resource) override;
				ExpectsGraphics<void> CopyBackBuffer(Texture2D** Result) override;
				ExpectsGraphics<void> ResizeBuffers(unsigned int Width, unsigned int Height) override;
				ExpectsGraphics<void> GenerateTexture(Texture2D* Resource) override;
				ExpectsGraphics<void> GenerateTexture(Texture3D* Resource) override;
				ExpectsGraphics<void> GenerateTexture(TextureCube* Resource) override;
				ExpectsGraphics<void> GetQueryData(Query* Resource, size_t* Result, bool Flush) override;
				ExpectsGraphics<void> GetQueryData(Query* Resource, bool* Result, bool Flush) override;
				ExpectsGraphics<Core::Unique<DepthStencilState>> CreateDepthStencilState(const DepthStencilState::Desc& I) override;
				ExpectsGraphics<Core::Unique<BlendState>> CreateBlendState(const BlendState::Desc& I) override;
				ExpectsGraphics<Core::Unique<RasterizerState>> CreateRasterizerState(const RasterizerState::Desc& I) override;
				ExpectsGraphics<Core::Unique<SamplerState>> CreateSamplerState(const SamplerState::Desc& I) override;
				ExpectsGraphics<Core::Unique<InputLayout>> CreateInputLayout(const InputLayout::Desc& I) override;
				ExpectsGraphics<Core::Unique<Shader>> CreateShader(const Shader::Desc& I) override;
				ExpectsGraphics<Core::Unique<ElementBuffer>> CreateElementBuffer(const ElementBuffer::Desc& I) override;
				ExpectsGraphics<Core::Unique<MeshBuffer>> CreateMeshBuffer(const MeshBuffer::Desc& I) override;
				ExpectsGraphics<Core::Unique<MeshBuffer>> CreateMeshBuffer(ElementBuffer* VertexBuffer, ElementBuffer* IndexBuffer) override;
				ExpectsGraphics<Core::Unique<SkinMeshBuffer>> CreateSkinMeshBuffer(const SkinMeshBuffer::Desc& I) override;
				ExpectsGraphics<Core::Unique<SkinMeshBuffer>> CreateSkinMeshBuffer(ElementBuffer* VertexBuffer, ElementBuffer* IndexBuffer) override;
				ExpectsGraphics<Core::Unique<InstanceBuffer>> CreateInstanceBuffer(const InstanceBuffer::Desc& I) override;
				ExpectsGraphics<Core::Unique<Texture2D>> CreateTexture2D() override;
				ExpectsGraphics<Core::Unique<Texture2D>> CreateTexture2D(const Texture2D::Desc& I) override;
				ExpectsGraphics<Core::Unique<Texture3D>> CreateTexture3D() override;
				ExpectsGraphics<Core::Unique<Texture3D>> CreateTexture3D(const Texture3D::Desc& I) override;
				ExpectsGraphics<Core::Unique<TextureCube>> CreateTextureCube() override;
				ExpectsGraphics<Core::Unique<TextureCube>> CreateTextureCube(const TextureCube::Desc& I) override;
				ExpectsGraphics<Core::Unique<TextureCube>> CreateTextureCube(Texture2D* Resource[6]) override;
				ExpectsGraphics<Core::Unique<TextureCube>> CreateTextureCube(Texture2D* Resource) override;
				ExpectsGraphics<Core::Unique<DepthTarget2D>> CreateDepthTarget2D(const DepthTarget2D::Desc& I) override;
				ExpectsGraphics<Core::Unique<DepthTargetCube>> CreateDepthTargetCube(const DepthTargetCube::Desc& I) override;
				ExpectsGraphics<Core::Unique<RenderTarget2D>> CreateRenderTarget2D(const RenderTarget2D::Desc& I) override;
				ExpectsGraphics<Core::Unique<MultiRenderTarget2D>> CreateMultiRenderTarget2D(const MultiRenderTarget2D::Desc& I) override;
				ExpectsGraphics<Core::Unique<RenderTargetCube>> CreateRenderTargetCube(const RenderTargetCube::Desc& I) override;
				ExpectsGraphics<Core::Unique<MultiRenderTargetCube>> CreateMultiRenderTargetCube(const MultiRenderTargetCube::Desc& I) override;
				ExpectsGraphics<Core::Unique<Cubemap>> CreateCubemap(const Cubemap::Desc& I) override;
				ExpectsGraphics<Core::Unique<Query>> CreateQuery(const Query::Desc& I) override;
				PrimitiveTopology GetPrimitiveTopology() const override;
				ShaderModel GetSupportedShaderModel() const override;
				void* GetDevice() const override;
				void* GetContext() const override;
				bool IsValid() const override;
				void SetVSyncMode(VSync Mode) override;
				const char* GetShaderVersion();
				ExpectsGraphics<void> CopyConstantBuffer(GLuint Buffer, void* Data, size_t Size);
				ExpectsGraphics<void> CreateConstantBuffer(GLuint* Buffer, size_t Size);
				ExpectsGraphics<void> CreateDirectBuffer(size_t Size);
				Core::String CompileState(GLuint Handle);

			private:
				uint64_t GetProgramHash();

			protected:
				ExpectsGraphics<TextureCube*> CreateTextureCubeInternal(void* Resources[6]) override;

			public:
				static bool IsComparator(PixelFilter Value);
				static GLenum GetAccessControl(CPUAccess Access, ResourceUsage Usage);
				static GLenum GetBaseFormat(Format Value);
				static GLenum GetSizedFormat(Format Value);
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
				static void GetBackBufferSize(Format Value, int* X, int* Y, int* Z, int* W);
				static void APIENTRY DebugMessage(GLenum Source, GLenum Type, GLuint Id, GLenum Severity, GLsizei Length, const GLchar* Message, const void* Data);
			};
		}
	}
}
#endif
#endif
#endif