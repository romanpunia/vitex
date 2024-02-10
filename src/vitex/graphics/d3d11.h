#ifndef VI_GRAPHICS_D3D11_H
#define VI_GRAPHICS_D3D11_H
#include "../graphics.h"
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
				void FillView(uint32_t Target, uint32_t MipLevels, uint32_t Size);
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
					std::array<std::pair<ID3D11ShaderResourceView*, uint32_t>, UNITS_SIZE> Resources = { };
					std::array<std::pair<D3D11ElementBuffer*, uint32_t>, UNITS_SIZE> VertexBuffers = { };
					std::tuple<ID3D11SamplerState*, uint32_t, uint32_t> Sampler = { nullptr, 0, 0 };
					std::tuple<D3D11ElementBuffer*, Format> IndexBuffer = { nullptr, Format::Unknown };
					ID3D11BlendState* Blend = nullptr;
					ID3D11RasterizerState* Rasterizer = nullptr;
					ID3D11DepthStencilState* DepthStencil = nullptr;
					D3D11InputLayout* Layout = nullptr;
					PrimitiveTopology Primitive = PrimitiveTopology::Triangle_List;
				} Register;

				struct
				{
					std::string_view Vertex = "";
					std::string_view Pixel = "";
					std::string_view Geometry = "";
					std::string_view Hull = "";
					std::string_view Domain = "";
					std::string_view Compute = "";
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
				ExpectsGraphics<void> SetShader(Shader* Resource, uint32_t Type) override;
				void SetSamplerState(SamplerState* State, uint32_t Slot, uint32_t Count, uint32_t Type) override;
				void SetBuffer(Shader* Resource, uint32_t Slot, uint32_t Type) override;
				void SetBuffer(InstanceBuffer* Resource, uint32_t Slot, uint32_t Type) override;
				void SetConstantBuffer(ElementBuffer* Resource, uint32_t Slot, uint32_t Type) override;
				void SetStructureBuffer(ElementBuffer* Resource, uint32_t Slot, uint32_t Type) override;
				void SetTexture2D(Texture2D* Resource, uint32_t Slot, uint32_t Type) override;
				void SetTexture3D(Texture3D* Resource, uint32_t Slot, uint32_t Type) override;
				void SetTextureCube(TextureCube* Resource, uint32_t Slot, uint32_t Type) override;
				void SetIndexBuffer(ElementBuffer* Resource, Format FormatMode) override;
				void SetVertexBuffers(ElementBuffer** Resources, uint32_t Count, bool DynamicLinkage = false) override;
				void SetWriteable(ElementBuffer** Resource, uint32_t Slot, uint32_t Count, bool Computable) override;
				void SetWriteable(Texture2D** Resource, uint32_t Slot, uint32_t Count, bool Computable) override;
				void SetWriteable(Texture3D** Resource, uint32_t Slot, uint32_t Count, bool Computable) override;
				void SetWriteable(TextureCube** Resource, uint32_t Slot, uint32_t Count, bool Computable) override;
				void SetTarget(float R, float G, float B) override;
				void SetTarget() override;
				void SetTarget(DepthTarget2D* Resource) override;
				void SetTarget(DepthTargetCube* Resource) override;
				void SetTarget(Graphics::RenderTarget* Resource, uint32_t Target, float R, float G, float B) override;
				void SetTarget(Graphics::RenderTarget* Resource, uint32_t Target) override;
				void SetTarget(Graphics::RenderTarget* Resource, float R, float G, float B) override;
				void SetTarget(Graphics::RenderTarget* Resource) override;
				void SetTargetMap(Graphics::RenderTarget* Resource, bool Enabled[8]) override;
				void SetTargetRect(uint32_t Width, uint32_t Height) override;
				void SetViewports(uint32_t Count, Viewport* Viewports) override;
				void SetScissorRects(uint32_t Count, Compute::Rectangle* Value) override;
				void SetPrimitiveTopology(PrimitiveTopology Topology) override;
				void FlushTexture(uint32_t Slot, uint32_t Count, uint32_t Type) override;
				void FlushState() override;
				void ClearBuffer(InstanceBuffer* Resource) override;
				void ClearWritable(Texture2D* Resource) override;
				void ClearWritable(Texture2D* Resource, float R, float G, float B) override;
				void ClearWritable(Texture3D* Resource) override;
				void ClearWritable(Texture3D* Resource, float R, float G, float B) override;
				void ClearWritable(TextureCube* Resource) override;
				void ClearWritable(TextureCube* Resource, float R, float G, float B) override;
				void Clear(float R, float G, float B) override;
				void Clear(Graphics::RenderTarget* Resource, uint32_t Target, float R, float G, float B) override;
				void ClearDepth() override;
				void ClearDepth(DepthTarget2D* Resource) override;
				void ClearDepth(DepthTargetCube* Resource) override;
				void ClearDepth(Graphics::RenderTarget* Resource) override;
				void DrawIndexed(uint32_t Count, uint32_t IndexLocation, uint32_t BaseLocation) override;
				void DrawIndexed(MeshBuffer* Resource) override;
				void DrawIndexed(SkinMeshBuffer* Resource) override;
				void DrawIndexedInstanced(uint32_t IndexCountPerInstance, uint32_t InstanceCount, uint32_t IndexLocation, uint32_t VertexLocation, uint32_t InstanceLocation) override;
				void DrawIndexedInstanced(ElementBuffer* Instances, MeshBuffer* Resource, uint32_t InstanceCount) override;
				void DrawIndexedInstanced(ElementBuffer* Instances, SkinMeshBuffer* Resource, uint32_t InstanceCount) override;
				void Draw(uint32_t Count, uint32_t Location) override;
				void DrawInstanced(uint32_t VertexCountPerInstance, uint32_t InstanceCount,uint32_t VertexLocation, uint32_t InstanceLocation) override;
				void Dispatch(uint32_t GroupX, uint32_t GroupY, uint32_t GroupZ) override;
				void GetViewports(uint32_t* Count, Viewport* Out) override;
				void GetScissorRects(uint32_t* Count, Compute::Rectangle* Out) override;
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
				ExpectsGraphics<void> CopyTexture2D(Graphics::RenderTarget* Resource, uint32_t Target, Texture2D** Result) override;
				ExpectsGraphics<void> CopyTexture2D(RenderTargetCube* Resource, Compute::CubeFace Face, Texture2D** Result) override;
				ExpectsGraphics<void> CopyTexture2D(MultiRenderTargetCube* Resource, uint32_t Cube, Compute::CubeFace Face, Texture2D** Result) override;
				ExpectsGraphics<void> CopyTextureCube(RenderTargetCube* Resource, TextureCube** Result) override;
				ExpectsGraphics<void> CopyTextureCube(MultiRenderTargetCube* Resource, uint32_t Cube, TextureCube** Result) override;
				ExpectsGraphics<void> CopyTarget(Graphics::RenderTarget* From, uint32_t FromTarget, Graphics::RenderTarget* To, uint32_t ToTarget) override;
				ExpectsGraphics<void> CubemapPush(Cubemap* Resource, TextureCube* Result) override;
				ExpectsGraphics<void> CubemapFace(Cubemap* Resource, Compute::CubeFace Face) override;
				ExpectsGraphics<void> CubemapPop(Cubemap* Resource) override;
				ExpectsGraphics<void> CopyBackBuffer(Texture2D** Result) override;
				ExpectsGraphics<void> ResizeBuffers(uint32_t Width, uint32_t Height) override;
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
				ExpectsGraphics<void> CreateDirectBuffer(size_t Size);
				ExpectsGraphics<void> CreateTexture2D(Texture2D* Resource, DXGI_FORMAT Format);
				ExpectsGraphics<void> CreateTextureCube(TextureCube* Resource, DXGI_FORMAT Format);
				ExpectsGraphics<ID3D11InputLayout*> GenerateInputLayout(D3D11Shader* Shader);
				HRESULT CreateConstantBuffer(ID3D11Buffer** Buffer, size_t Size);
				std::string_view GetCompileState(ID3DBlob* Error);
				const std::string_view& GetVSProfile();
				const std::string_view& GetPSProfile();
				const std::string_view& GetGSProfile();
				const std::string_view& GetHSProfile();
				const std::string_view& GetCSProfile();
				const std::string_view& GetDSProfile();

			protected:
				ExpectsGraphics<TextureCube*> CreateTextureCubeInternal(void* Resource[6]) override;
			};
		}
	}
}
#endif
#endif