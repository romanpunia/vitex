#ifndef VI_GRAPHICS_D3D11_H
#define VI_GRAPHICS_D3D11_H
#include "../core/graphics.h"
#include <array>
#ifdef VI_MICROSOFT
#include <d3d11.h>
#include <d3dcompiler.h>

namespace Vitex
{
	namespace Graphics
	{
		namespace D3D11
		{
			class D3D11Device;

			class D3D11DepthStencilState final : public DepthStencilState
			{
				friend D3D11Device;

			public:
				ID3D11DepthStencilState* Resource;

			public:
				D3D11DepthStencilState(const Desc& I);
				~D3D11DepthStencilState() override;
				void* GetResource() const override;
			};

			class D3D11RasterizerState final : public RasterizerState
			{
				friend D3D11Device;

			public:
				ID3D11RasterizerState* Resource;

			public:
				D3D11RasterizerState(const Desc& I);
				~D3D11RasterizerState() override;
				void* GetResource() const override;
			};

			class D3D11BlendState final : public BlendState
			{
				friend D3D11Device;

			public:
				ID3D11BlendState* Resource;

			public:
				D3D11BlendState(const Desc& I);
				~D3D11BlendState() override;
				void* GetResource() const override;
			};

			class D3D11SamplerState final : public SamplerState
			{
				friend D3D11Device;

			public:
				ID3D11SamplerState* Resource;

			public:
				D3D11SamplerState(const Desc& I);
				~D3D11SamplerState() override;
				void* GetResource() const override;
			};

			class D3D11InputLayout final : public InputLayout
			{
				friend D3D11Device;

			public:
				D3D11InputLayout(const Desc& I);
				~D3D11InputLayout() override;
				void* GetResource() const override;
			};

			class D3D11Shader final : public Shader
			{
				friend D3D11Device;

			private:
				bool Compiled;

			public:
				ID3D11VertexShader* VertexShader;
				ID3D11PixelShader* PixelShader;
				ID3D11GeometryShader* GeometryShader;
				ID3D11DomainShader* DomainShader;
				ID3D11HullShader* HullShader;
				ID3D11ComputeShader* ComputeShader;
				ID3D11Buffer* ConstantBuffer;
				ID3D11InputLayout* VertexLayout;
				ID3DBlob* Signature;

			public:
				D3D11Shader(const Desc& I);
				~D3D11Shader() override;
				bool IsValid() const override;
			};

			class D3D11ElementBuffer final : public ElementBuffer
			{
				friend D3D11Device;

			public:
				ID3D11UnorderedAccessView* Access;
				ID3D11ShaderResourceView* Resource;
				ID3D11Buffer* Element;

			public:
				D3D11ElementBuffer(const Desc& I);
				~D3D11ElementBuffer() override;
				void* GetResource() const override;
			};

			class D3D11MeshBuffer final : public MeshBuffer
			{
				friend D3D11Device;

			public:
				D3D11MeshBuffer(const Desc& I);
				Core::Unique<Compute::Vertex> GetElements(GraphicsDevice* Device) const override;
			};

			class D3D11SkinMeshBuffer final : public SkinMeshBuffer
			{
				friend D3D11Device;

			public:
				D3D11SkinMeshBuffer(const Desc& I);
				Core::Unique<Compute::SkinVertex> GetElements(GraphicsDevice* Device) const override;
			};

			class D3D11InstanceBuffer final : public InstanceBuffer
			{
				friend D3D11Device;

			public:
				ID3D11ShaderResourceView* Resource;

			public:
				D3D11InstanceBuffer(const Desc& I);
				~D3D11InstanceBuffer() override;
			};

			class D3D11Texture2D final : public Texture2D
			{
				friend D3D11Device;

			public:
				ID3D11UnorderedAccessView* Access;
				ID3D11ShaderResourceView* Resource;
				ID3D11Texture2D* View;

			public:
				D3D11Texture2D();
				D3D11Texture2D(const Desc& I);
				~D3D11Texture2D() override;
				void* GetResource() const override;
			};

			class D3D11Texture3D final : public Texture3D
			{
				friend D3D11Device;

			public:
				ID3D11UnorderedAccessView* Access;
				ID3D11ShaderResourceView* Resource;
				ID3D11Texture3D* View;

			public:
				D3D11Texture3D();
				~D3D11Texture3D() override;
				void* GetResource() override;
			};

			class D3D11TextureCube final : public TextureCube
			{
				friend D3D11Device;

			public:
				ID3D11UnorderedAccessView* Access;
				ID3D11ShaderResourceView* Resource;
				ID3D11Texture2D* View;

			public:
				D3D11TextureCube();
				D3D11TextureCube(const Desc& I);
				~D3D11TextureCube() override;
				void* GetResource() const override;
			};

			class D3D11DepthTarget2D final : public DepthTarget2D
			{
				friend D3D11Device;

			public:
				ID3D11DepthStencilView* DepthStencilView;

			public:
				D3D11DepthTarget2D(const Desc& I);
				~D3D11DepthTarget2D() override;
				void* GetResource() const override;
				uint32_t GetWidth() const override;
				uint32_t GetHeight() const override;
			};

			class D3D11DepthTargetCube final : public DepthTargetCube
			{
				friend D3D11Device;

			public:
				ID3D11DepthStencilView* DepthStencilView;

			public:
				D3D11DepthTargetCube(const Desc& I);
				~D3D11DepthTargetCube() override;
				void* GetResource() const override;
				uint32_t GetWidth() const override;
				uint32_t GetHeight() const override;
			};

			class D3D11RenderTarget2D final : public RenderTarget2D
			{
				friend D3D11Device;

			public:
				ID3D11DepthStencilView* DepthStencilView;
				ID3D11RenderTargetView* RenderTargetView;
				ID3D11Texture2D* Texture;

			public:
				D3D11RenderTarget2D(const Desc& I);
				~D3D11RenderTarget2D() override;
				void* GetTargetBuffer() const override;
				void* GetDepthBuffer() const override;
				uint32_t GetWidth() const override;
				uint32_t GetHeight() const override;
			};

			class D3D11MultiRenderTarget2D final : public MultiRenderTarget2D
			{
				friend D3D11Device;

			public:
				ID3D11DepthStencilView* DepthStencilView;
				ID3D11RenderTargetView* RenderTargetView[8];
				ID3D11Texture2D* Texture[8];
				D3D11_TEXTURE2D_DESC Information;

			public:
				D3D11MultiRenderTarget2D(const Desc& I);
				~D3D11MultiRenderTarget2D() override;
				void* GetTargetBuffer() const override;
				void* GetDepthBuffer() const override;
				uint32_t GetWidth() const override;
				uint32_t GetHeight() const override;

			private:
				void FillView(unsigned int Target, unsigned int MipLevels, unsigned int Size);
			};

			class D3D11RenderTargetCube final : public RenderTargetCube
			{
				friend D3D11Device;

			public:
				ID3D11DepthStencilView* DepthStencilView;
				ID3D11RenderTargetView* RenderTargetView;
				ID3D11Texture2D* Texture;

			public:
				D3D11RenderTargetCube(const Desc& I);
				~D3D11RenderTargetCube() override;
				void* GetTargetBuffer() const override;
				void* GetDepthBuffer() const override;
				uint32_t GetWidth() const override;
				uint32_t GetHeight() const override;
			};

			class D3D11MultiRenderTargetCube final : public MultiRenderTargetCube
			{
				friend D3D11Device;

			public:
				ID3D11DepthStencilView* DepthStencilView;
				ID3D11RenderTargetView* RenderTargetView[8];
				ID3D11Texture2D* Texture[8];

			public:
				D3D11MultiRenderTargetCube(const Desc& I);
				~D3D11MultiRenderTargetCube() override;
				void* GetTargetBuffer() const override;
				void* GetDepthBuffer() const override;
				uint32_t GetWidth() const override;
				uint32_t GetHeight() const override;
			};

			class D3D11Cubemap final : public Cubemap
			{
				friend D3D11Device;

			public:
				struct
				{
					D3D11_SHADER_RESOURCE_VIEW_DESC Resource;
					D3D11_TEXTURE2D_DESC Texture;
					D3D11_BOX Region;
				} Options;

			public:
				ID3D11Texture2D* Merger;
				ID3D11Texture2D* Source;

			public:
				D3D11Cubemap(const Desc& I);
				~D3D11Cubemap() override;
			};

			class D3D11Query final : public Query
			{
			public:
				ID3D11Query* Async;

			public:
				D3D11Query();
				~D3D11Query() override;
				void* GetResource() const override;
			};

			class D3D11Device final : public GraphicsDevice
			{
			private:
				struct
				{
					ID3D11VertexShader* VertexShader = nullptr;
					ID3D11PixelShader* PixelShader = nullptr;
					ID3D11InputLayout* VertexLayout = nullptr;
					ID3D11Buffer* ConstantBuffer = nullptr;
					ID3D11Buffer* VertexBuffer = nullptr;
					ID3D11SamplerState* Sampler = nullptr;
				} Immediate;

				struct
				{
					std::array<D3D11Shader*, 6> Shaders = { };
					std::array<std::pair<ID3D11ShaderResourceView*, unsigned int>, UNITS_SIZE> Resources = { };
					std::array<std::pair<D3D11ElementBuffer*, unsigned int>, UNITS_SIZE> VertexBuffers = { };
					std::tuple<ID3D11SamplerState*, unsigned int, unsigned int> Sampler = { nullptr, 0, 0 };
					std::tuple<D3D11ElementBuffer*, Format> IndexBuffer = { nullptr, Format::Unknown };
					ID3D11BlendState* Blend = nullptr;
					ID3D11RasterizerState* Rasterizer = nullptr;
					ID3D11DepthStencilState* DepthStencil = nullptr;
					D3D11InputLayout* Layout = nullptr;
					PrimitiveTopology Primitive = PrimitiveTopology::Triangle_List;
				} Register;

				struct
				{
					const char* Vertex = "";
					const char* Pixel = "";
					const char* Geometry = "";
					const char* Hull = "";
					const char* Domain = "";
					const char* Compute = "";
				} Models;

			public:
				ID3D11DeviceContext* ImmediateContext;
				ID3D11Device* Context;
				IDXGISwapChain* SwapChain;
				DXGI_SWAP_CHAIN_DESC SwapChainResource;
				D3D_FEATURE_LEVEL FeatureLevel;
				D3D_DRIVER_TYPE DriverType;

			public:
				D3D11Device(const Desc& I);
				~D3D11Device() override;
				void SetAsCurrentDevice() override;
				void SetShaderModel(ShaderModel Model) override;
				void SetBlendState(BlendState* State) override;
				void SetRasterizerState(RasterizerState* State) override;
				void SetDepthStencilState(DepthStencilState* State) override;
				void SetInputLayout(InputLayout* State) override;
				void SetShader(Shader* Resource, unsigned int Type) override;
				void SetSamplerState(SamplerState* State, unsigned int Slot, unsigned int Count, unsigned int Type) override;
				void SetBuffer(Shader* Resource, unsigned int Slot, unsigned int Type) override;
				void SetBuffer(InstanceBuffer* Resource, unsigned int Slot, unsigned int Type) override;
				void SetConstantBuffer(ElementBuffer* Resource, unsigned int Slot, unsigned int Type) override;
				void SetStructureBuffer(ElementBuffer* Resource, unsigned int Slot, unsigned int Type) override;
				void SetTexture2D(Texture2D* Resource, unsigned int Slot, unsigned int Type) override;
				void SetTexture3D(Texture3D* Resource, unsigned int Slot, unsigned int Type) override;
				void SetTextureCube(TextureCube* Resource, unsigned int Slot, unsigned int Type) override;
				void SetIndexBuffer(ElementBuffer* Resource, Format FormatMode) override;
				void SetVertexBuffers(ElementBuffer** Resources, unsigned int Count, bool DynamicLinkage = false) override;
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
				bool Map(Texture2D* Resource, ResourceMap Mode, MappedSubresource* Map) override;
				bool Map(Texture3D* Resource, ResourceMap Mode, MappedSubresource* Map) override;
				bool Map(TextureCube* Resource, ResourceMap Mode, MappedSubresource* Map) override;
				bool Unmap(Texture2D* Resource, MappedSubresource* Map) override;
				bool Unmap(Texture3D* Resource, MappedSubresource* Map) override;
				bool Unmap(TextureCube* Resource, MappedSubresource* Map) override;
				bool Unmap(ElementBuffer* Resource, MappedSubresource* Map) override;
				bool UpdateConstantBuffer(ElementBuffer* Resource, void* Data, size_t Size) override;
				bool UpdateBuffer(ElementBuffer* Resource, void* Data, size_t Size) override;
				bool UpdateBuffer(Shader* Resource, const void* Data) override;
				bool UpdateBuffer(MeshBuffer* Resource, Compute::Vertex* Data) override;
				bool UpdateBuffer(SkinMeshBuffer* Resource, Compute::SkinVertex* Data) override;
				bool UpdateBuffer(InstanceBuffer* Resource) override;
				bool UpdateBufferSize(Shader* Resource, size_t Size) override;
				bool UpdateBufferSize(InstanceBuffer* Resource, size_t Size) override;
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
				void DrawInstanced(unsigned int VertexCountPerInstance, unsigned int InstanceCount,unsigned int VertexLocation, unsigned int InstanceLocation) override;
				void Dispatch(unsigned int GroupX, unsigned int GroupY, unsigned int GroupZ) override;
				bool CopyTexture2D(Texture2D* Resource, Texture2D** Result) override;
				bool CopyTexture2D(Graphics::RenderTarget* Resource, unsigned int Target, Texture2D** Result) override;
				bool CopyTexture2D(RenderTargetCube* Resource, Compute::CubeFace Face, Texture2D** Result) override;
				bool CopyTexture2D(MultiRenderTargetCube* Resource, unsigned int Cube, Compute::CubeFace Face, Texture2D** Result) override;
				bool CopyTextureCube(RenderTargetCube* Resource, TextureCube** Result) override;
				bool CopyTextureCube(MultiRenderTargetCube* Resource, unsigned int Cube, TextureCube** Result) override;
				bool CopyTarget(Graphics::RenderTarget* From, unsigned int FromTarget, Graphics::RenderTarget* To, unsigned int ToTarget) override;
				bool CopyBackBuffer(Texture2D** Result) override;
				bool CopyBackBufferMSAA(Texture2D** Result) override;
				bool CopyBackBufferNoAA(Texture2D** Result) override;
				bool CubemapPush(Cubemap* Resource, TextureCube* Result) override;
				bool CubemapFace(Cubemap* Resource, Compute::CubeFace Face) override;
				bool CubemapPop(Cubemap* Resource) override;
				void GetViewports(unsigned int* Count, Viewport* Out) override;
				void GetScissorRects(unsigned int* Count, Compute::Rectangle* Out) override;
				bool ResizeBuffers(unsigned int Width, unsigned int Height) override;
				bool GenerateTexture(Texture2D* Resource) override;
				bool GenerateTexture(Texture3D* Resource) override;
				bool GenerateTexture(TextureCube* Resource) override;
				bool GetQueryData(Query* Resource, size_t* Result, bool Flush) override;
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
				Core::Unique<MeshBuffer> CreateMeshBuffer(ElementBuffer* VertexBuffer, ElementBuffer* IndexBuffer) override;
				Core::Unique<SkinMeshBuffer> CreateSkinMeshBuffer(const SkinMeshBuffer::Desc& I) override;
				Core::Unique<SkinMeshBuffer> CreateSkinMeshBuffer(ElementBuffer* VertexBuffer, ElementBuffer* IndexBuffer) override;
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
				PrimitiveTopology GetPrimitiveTopology() const override;
				ShaderModel GetSupportedShaderModel() const override;
				void* GetDevice() const override;
				void* GetContext() const override;
				bool IsValid() const override;
				bool CreateDirectBuffer(size_t Size);
				bool CreateTexture2D(Texture2D* Resource, DXGI_FORMAT Format);
				bool CreateTextureCube(TextureCube* Resource, DXGI_FORMAT Format);
				ID3D11InputLayout* GenerateInputLayout(D3D11Shader* Shader);
				int CreateConstantBuffer(ID3D11Buffer** Buffer, size_t Size);
				char* GetCompileState(ID3DBlob* Error);
				char* GetVSProfile();
				char* GetPSProfile();
				char* GetGSProfile();
				char* GetHSProfile();
				char* GetCSProfile();
				char* GetDSProfile();

			protected:
				TextureCube* CreateTextureCubeInternal(void* Resource[6]) override;
			};
		}
	}
}
#endif
#endif