#ifndef TH_GRAPHICS_OPENGL45_H
#define TH_GRAPHICS_OPENGL45_H
#include "../core/graphics.h"
#include <array>
#ifdef TH_MICROSOFT
#include <Windows.h>
#endif
#ifdef TH_HAS_OPENGL
#ifdef TH_HAS_GLEW
#include <GL/glew.h>
#define TH_HAS_GL
#ifndef APIENTRY
#define APIENTRY
#endif

namespace Tomahawk
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
				void Release();
			};

			class OGLDepthStencilState final : public DepthStencilState
			{
				friend OGLDevice;

			public:
				OGLDepthStencilState(const Desc& I);
				virtual ~OGLDepthStencilState() override;
				void* GetResource() override;
			};

			class OGLRasterizerState final : public RasterizerState
			{
				friend OGLDevice;

			public:
				OGLRasterizerState(const Desc& I);
				virtual ~OGLRasterizerState() override;
				void* GetResource() override;
			};

			class OGLBlendState final : public BlendState
			{
				friend OGLDevice;

			public:
				OGLBlendState(const Desc& I);
				virtual ~OGLBlendState() override;
				void* GetResource() override;
			};

			class OGLSamplerState final : public SamplerState
			{
				friend OGLDevice;

			private:
				GLenum Resource = GL_NONE;

			public:
				OGLSamplerState(const Desc& I);
				virtual ~OGLSamplerState() override;
				void* GetResource() override;
			};

			class OGLInputLayout final : public InputLayout
			{
				friend OGLDevice;

			public:
				std::unordered_map<size_t, std::vector<std::function<void(uint64_t)>>> VertexLayout;
				std::unordered_map<std::string, GLuint> Layouts;
				GLuint DynamicResource = GL_NONE;

			public:
				OGLInputLayout(const Desc& I);
				virtual ~OGLInputLayout() override;
				void* GetResource() override;

			public:
				static std::string GetLayoutHash(OGLElementBuffer** Buffers, unsigned int Count);
			};

			class OGLShader final : public Shader
			{
				friend OGLDevice;

			private:
				bool Compiled;

			public:
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
				virtual ~OGLShader() override;
				bool IsValid() override;
			};

			class OGLElementBuffer final : public ElementBuffer
			{
				friend OGLDevice;

			private:
				GLuint Resource = GL_NONE;
				GLenum Flags = GL_NONE;

			public:
				OGLElementBuffer(const Desc& I);
				virtual ~OGLElementBuffer() override;
				void* GetResource() override;
			};

			class OGLMeshBuffer final : public MeshBuffer
			{
				friend OGLDevice;

			public:
				OGLMeshBuffer(const Desc& I);
				Core::Unique<Compute::Vertex> GetElements(GraphicsDevice* Device) override;
			};

			class OGLSkinMeshBuffer final : public SkinMeshBuffer
			{
				friend OGLDevice;

			public:
				OGLSkinMeshBuffer(const Desc& I);
				Core::Unique<Compute::SkinVertex> GetElements(GraphicsDevice* Device) override;
			};

			class OGLInstanceBuffer final : public InstanceBuffer
			{
				friend OGLDevice;

			public:
				OGLInstanceBuffer(const Desc& I);
				virtual ~OGLInstanceBuffer() override;
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
				virtual ~OGLTexture2D() override;
				void* GetResource() override;
			};

			class OGLTexture3D final : public Texture3D
			{
				friend OGLDevice;

			public:
				GLenum Format = GL_NONE;
				GLuint Resource = GL_NONE;

			public:
				OGLTexture3D();
				virtual ~OGLTexture3D() override;
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
				virtual ~OGLTextureCube() override;
				void* GetResource() override;
			};

			class OGLDepthTarget2D final : public DepthTarget2D
			{
				friend OGLDevice;

			public:
				GLuint FrameBuffer = GL_NONE;
				GLuint DepthTexture = GL_NONE;

			public:
				OGLDepthTarget2D(const Desc& I);
				virtual ~OGLDepthTarget2D() override;
				void* GetResource() override;
				uint32_t GetWidth() override;
				uint32_t GetHeight() override;
			};

			class OGLDepthTargetCube final : public DepthTargetCube
			{
				friend OGLDevice;

			public:
				GLuint FrameBuffer = GL_NONE;
				GLuint DepthTexture = GL_NONE;

			public:
				OGLDepthTargetCube(const Desc& I);
				virtual ~OGLDepthTargetCube() override;
				void* GetResource() override;
				uint32_t GetWidth() override;
				uint32_t GetHeight() override;
			};

			class OGLRenderTarget2D final : public RenderTarget2D
			{
				friend OGLDevice;

			public:
				OGLFrameBuffer FrameBuffer;
				GLuint DepthTexture = GL_NONE;

			public:
				OGLRenderTarget2D(const Desc& I);
				virtual ~OGLRenderTarget2D() override;
				void* GetTargetBuffer() override;
				void* GetDepthBuffer() override;
				uint32_t GetWidth() override;
				uint32_t GetHeight() override;
			};

			class OGLMultiRenderTarget2D final : public MultiRenderTarget2D
			{
				friend OGLDevice;

			public:
				OGLFrameBuffer FrameBuffer;
				GLuint DepthTexture = GL_NONE;

			public:
				OGLMultiRenderTarget2D(const Desc& I);
				virtual ~OGLMultiRenderTarget2D() override;
				void* GetTargetBuffer() override;
				void* GetDepthBuffer() override;
				uint32_t GetWidth() override;
				uint32_t GetHeight() override;
			};

			class OGLRenderTargetCube final : public RenderTargetCube
			{
				friend OGLDevice;

			public:
				OGLFrameBuffer FrameBuffer;
				GLuint DepthTexture = GL_NONE;

			public:
				OGLRenderTargetCube(const Desc& I);
				virtual ~OGLRenderTargetCube() override;
				void* GetTargetBuffer() override;
				void* GetDepthBuffer() override;
				uint32_t GetWidth() override;
				uint32_t GetHeight() override;
			};

			class OGLMultiRenderTargetCube final : public MultiRenderTargetCube
			{
				friend OGLDevice;

			public:
				OGLFrameBuffer FrameBuffer;
				GLuint DepthTexture = GL_NONE;

			public:
				OGLMultiRenderTargetCube(const Desc& I);
				virtual ~OGLMultiRenderTargetCube() override;
				void* GetTargetBuffer() override;
				void* GetDepthBuffer() override;
				uint32_t GetWidth() override;
				uint32_t GetHeight() override;
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
				virtual ~OGLCubemap() override;
			};

			class OGLQuery final : public Query
			{
			public:
				GLuint Async = GL_NONE;
				bool Predicate = false;

			public:
				OGLQuery();
				virtual ~OGLQuery() override;
				void* GetResource() override;
			};

			class OGLDevice final : public GraphicsDevice
			{
			private:
				struct
				{
					GLuint VertexShader = GL_NONE;
					GLuint PixelShader = GL_NONE;
					GLuint Program = GL_NONE;
					GLuint VertexBuffer = GL_NONE;
					GLuint VertexArray = GL_NONE;
				} Immediate;

				struct
				{
					std::tuple<OGLElementBuffer*, Format> IndexBuffer = { nullptr, Format::Unknown };
					std::unordered_map<uint64_t, GLuint> Programs;
					std::array<OGLElementBuffer*, TH_MAX_UNITS> VertexBuffers = { };
					std::array<GLuint, TH_MAX_UNITS> Bindings = { };
					std::array<GLuint, TH_MAX_UNITS> Textures = { };
					std::array<GLuint, TH_MAX_UNITS> Samplers = { };
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
				GLuint ConstantBuffer[3];
				size_t ConstantSize[3];
				Activity* Window;
				void* Context;

			public:
				OGLDevice(const Desc& I);
				virtual ~OGLDevice() override;
				void SetConstantBuffers() override;
				void SetShaderModel(ShaderModel Model) override;
				void SetBlendState(BlendState* State) override;
				void SetRasterizerState(RasterizerState* State) override;
				void SetDepthStencilState(DepthStencilState* State) override;
				void SetInputLayout(InputLayout* State) override;
				void SetShader(Shader* Resource, unsigned int Type) override;
				void SetSamplerState(SamplerState* State, unsigned int Slot, unsigned int Count, unsigned int Type) override;
				void SetBuffer(Shader* Resource, unsigned int Slot, unsigned int Type) override;
				void SetBuffer(InstanceBuffer* Resource, unsigned int Slot, unsigned int Type) override;
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
				bool Map(ElementBuffer* Resource, ResourceMap Mode, MappedSubresource* Map) override;
				bool Unmap(ElementBuffer* Resource, MappedSubresource* Map) override;
				bool UpdateBuffer(ElementBuffer* Resource, void* Data, uint64_t Size) override;
				bool UpdateBuffer(Shader* Resource, const void* Data) override;
				bool UpdateBuffer(MeshBuffer* Resource, Compute::Vertex* Data) override;
				bool UpdateBuffer(SkinMeshBuffer* Resource, Compute::SkinVertex* Data) override;
				bool UpdateBuffer(InstanceBuffer* Resource) override;
				bool UpdateBuffer(RenderBufferType Buffer) override;
				bool UpdateBufferSize(Shader* Resource, size_t Size) override;
				bool UpdateBufferSize(InstanceBuffer* Resource, uint64_t Size) override;
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
				void Dispatch(unsigned int GroupX, unsigned int GroupY, unsigned int GroupZ) override;
				bool CopyTexture2D(Texture2D* Resource, Texture2D** Result) override;
				bool CopyTexture2D(Graphics::RenderTarget* Resource, unsigned int Target, Texture2D** Result) override;
				bool CopyTexture2D(RenderTargetCube* Resource, Compute::CubeFace Face, Texture2D** Result) override;
				bool CopyTexture2D(MultiRenderTargetCube* Resource, unsigned int Cube, Compute::CubeFace Face, Texture2D** Result) override;
				bool CopyTextureCube(RenderTargetCube* Resource, TextureCube** Result) override;
				bool CopyTextureCube(MultiRenderTargetCube* Resource, unsigned int Cube, TextureCube** Result) override;
				bool CopyTarget(Graphics::RenderTarget* From, unsigned int FromTarget, Graphics::RenderTarget* To, unsigned ToTarget) override;
				bool CubemapPush(Cubemap* Resource, TextureCube* Result) override;
				bool CubemapFace(Cubemap* Resource, Compute::CubeFace Face) override;
				bool CubemapPop(Cubemap* Resource) override;
				bool CopyBackBuffer(Texture2D** Result) override;
				bool CopyBackBufferMSAA(Texture2D** Result) override;
				bool CopyBackBufferNoAA(Texture2D** Result) override;
				void GetViewports(unsigned int* Count, Viewport* Out) override;
				void GetScissorRects(unsigned int* Count, Compute::Rectangle* Out) override;
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
				bool Submit() override;
				Core::Unique<DepthStencilState> CreateDepthStencilState(const DepthStencilState::Desc& I) override;
				Core::Unique<BlendState> CreateBlendState(const BlendState::Desc& I) override;
				Core::Unique<RasterizerState> CreateRasterizerState(const RasterizerState::Desc& I) override;
				Core::Unique<SamplerState> CreateSamplerState(const SamplerState::Desc& I) override;
				Core::Unique<InputLayout> CreateInputLayout(const InputLayout::Desc& I) override;
				Core::Unique<Shader> CreateShader(const Shader::Desc& I) override;
				Core::Unique<ElementBuffer> CreateElementBuffer(const ElementBuffer::Desc& I) override;
				Core::Unique<MeshBuffer> CreateMeshBuffer(const MeshBuffer::Desc& I) override;
				Core::Unique<SkinMeshBuffer> CreateSkinMeshBuffer(const SkinMeshBuffer::Desc& I) override;
				Core::Unique<InstanceBuffer> CreateInstanceBuffer(const InstanceBuffer::Desc& I) override;
				Core::Unique<Texture2D> CreateTexture2D() override;
				Core::Unique<Texture2D> CreateTexture2D(const Texture2D::Desc& I) override;
				Core::Unique<Texture3D> CreateTexture3D() override;
				Core::Unique<Texture3D> CreateTexture3D(const Texture3D::Desc& I) override;
				Core::Unique<TextureCube> CreateTextureCube() override;
				Core::Unique<TextureCube> CreateTextureCube(const TextureCube::Desc& I) override;
				Core::Unique<TextureCube> CreateTextureCube(Texture2D* Resource[6]) override;
				Core::Unique<TextureCube> CreateTextureCube(Texture2D* Resource) override;
				Core::Unique<DepthTarget2D> CreateDepthTarget2D(const DepthTarget2D::Desc& I) override;
				Core::Unique<DepthTargetCube> CreateDepthTargetCube(const DepthTargetCube::Desc& I) override;
				Core::Unique<RenderTarget2D> CreateRenderTarget2D(const RenderTarget2D::Desc& I) override;
				Core::Unique<MultiRenderTarget2D> CreateMultiRenderTarget2D(const MultiRenderTarget2D::Desc& I) override;
				Core::Unique<RenderTargetCube> CreateRenderTargetCube(const RenderTargetCube::Desc& I) override;
				Core::Unique<MultiRenderTargetCube> CreateMultiRenderTargetCube(const MultiRenderTargetCube::Desc& I) override;
				Core::Unique<Cubemap> CreateCubemap(const Cubemap::Desc& I) override;
				Core::Unique<Query> CreateQuery(const Query::Desc& I) override;
				PrimitiveTopology GetPrimitiveTopology() override;
				ShaderModel GetSupportedShaderModel() override;
				void* GetDevice() override;
				void* GetContext() override;
				bool IsValid() override;
				const char* GetShaderVersion();
				void CopyConstantBuffer(GLuint Buffer, void* Data, size_t Size);
				int CreateConstantBuffer(GLuint* Buffer, size_t Size);
				bool CreateDirectBuffer(uint64_t Size);
				std::string CompileState(GLuint Handle);

			private:
				uint64_t GetProgramHash();

			protected:
				TextureCube* CreateTextureCubeInternal(void* Resources[6]) override;

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