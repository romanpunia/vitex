#ifndef TH_GRAPHICS_DIRECTX11_H
#define TH_GRAPHICS_DIRECTX11_H

#include "../core/graphics.h"

#ifdef TH_MICROSOFT
#include <d3d11.h>
#include <d3dcompiler.h>

namespace Tomahawk
{
	namespace Graphics
	{
		namespace D3D11
		{
			class D3D11Device;

			class D3D11DepthStencilState : public DepthStencilState
			{
				friend D3D11Device;

			public:
				ID3D11DepthStencilState* Resource;

			public:
				D3D11DepthStencilState(const Desc& I);
				virtual ~D3D11DepthStencilState() override;
				void* GetResource() override;
			};

			class D3D11RasterizerState : public RasterizerState
			{
				friend D3D11Device;

			public:
				ID3D11RasterizerState* Resource;

			public:
				D3D11RasterizerState(const Desc& I);
				virtual ~D3D11RasterizerState() override;
				void* GetResource() override;
			};

			class D3D11BlendState : public BlendState
			{
				friend D3D11Device;

			public:
				ID3D11BlendState* Resource;

			public:
				D3D11BlendState(const Desc& I);
				virtual ~D3D11BlendState() override;
				void* GetResource() override;
			};

			class D3D11SamplerState : public SamplerState
			{
				friend D3D11Device;

			public:
				ID3D11SamplerState* Resource;

			public:
				D3D11SamplerState(const Desc& I);
				virtual ~D3D11SamplerState() override;
				void* GetResource() override;
			};

			class D3D11InputLayout : public InputLayout
			{
				friend D3D11Device;

			public:
				D3D11InputLayout(const Desc& I);
				virtual ~D3D11InputLayout() override;
				void* GetResource() override;
			};

			class D3D11Shader : public Shader
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
				virtual ~D3D11Shader() override;
				bool IsValid() override;
			};

			class D3D11ElementBuffer : public ElementBuffer
			{
				friend D3D11Device;

			public:
				ID3D11UnorderedAccessView* Access;
				ID3D11ShaderResourceView* Resource;
				ID3D11Buffer* Element;

			public:
				D3D11ElementBuffer(const Desc& I);
				virtual ~D3D11ElementBuffer() override;
				void* GetResource() override;
			};

			class D3D11MeshBuffer : public MeshBuffer
			{
				friend D3D11Device;

			public:
				D3D11MeshBuffer(const Desc& I);
				Compute::Vertex* GetElements(GraphicsDevice* Device) override;
			};

			class D3D11SkinMeshBuffer : public SkinMeshBuffer
			{
				friend D3D11Device;

			public:
				D3D11SkinMeshBuffer(const Desc& I);
				Compute::SkinVertex* GetElements(GraphicsDevice* Device) override;
			};

			class D3D11InstanceBuffer : public InstanceBuffer
			{
				friend D3D11Device;

			private:
				bool Sync;

			public:
				ID3D11ShaderResourceView* Resource;

			public:
				D3D11InstanceBuffer(const Desc& I);
				virtual ~D3D11InstanceBuffer() override;
			};

			class D3D11Texture2D : public Texture2D
			{
				friend D3D11Device;

			public:
				ID3D11UnorderedAccessView* Access;
				ID3D11ShaderResourceView* Resource;
				ID3D11Texture2D* View;

			public:
				D3D11Texture2D();
				D3D11Texture2D(const Desc& I);
				virtual ~D3D11Texture2D() override;
				void* GetResource() override;
			};

			class D3D11Texture3D : public Texture3D
			{
				friend D3D11Device;

			public:
				ID3D11UnorderedAccessView* Access;
				ID3D11ShaderResourceView* Resource;
				ID3D11Texture3D* View;

			public:
				D3D11Texture3D();
				virtual ~D3D11Texture3D() override;
				void* GetResource() override;
			};

			class D3D11TextureCube : public TextureCube
			{
				friend D3D11Device;

			public:
				ID3D11UnorderedAccessView* Access;
				ID3D11ShaderResourceView* Resource;
				ID3D11Texture2D* View;

			public:
				D3D11TextureCube();
				D3D11TextureCube(const Desc& I);
				virtual ~D3D11TextureCube() override;
				void* GetResource() override;
			};

			class D3D11DepthBuffer : public DepthBuffer
			{
				friend D3D11Device;

			public:
				ID3D11DepthStencilView* DepthStencilView;

			public:
				D3D11DepthBuffer(const Desc& I);
				virtual ~D3D11DepthBuffer() override;
				void* GetResource() override;
				uint32_t GetWidth() override;
				uint32_t GetHeight() override;
			};

			class D3D11RenderTarget2D : public RenderTarget2D
			{
				friend D3D11Device;

			public:
				ID3D11DepthStencilView* DepthStencilView;
				ID3D11RenderTargetView* RenderTargetView;
				ID3D11Texture2D* Texture;

			public:
				D3D11RenderTarget2D(const Desc& I);
				virtual ~D3D11RenderTarget2D() override;
				void* GetTargetBuffer() override;
				void* GetDepthBuffer() override;
				uint32_t GetWidth() override;
				uint32_t GetHeight() override;
			};

			class D3D11MultiRenderTarget2D : public MultiRenderTarget2D
			{
				friend D3D11Device;

			public:
				ID3D11DepthStencilView* DepthStencilView;
				ID3D11RenderTargetView* RenderTargetView[8];
				ID3D11Texture2D* Texture[8];
				D3D11_TEXTURE2D_DESC Information;

			public:
				D3D11MultiRenderTarget2D(const Desc& I);
				virtual ~D3D11MultiRenderTarget2D() override;
				void* GetTargetBuffer() override;
				void* GetDepthBuffer() override;
				uint32_t GetWidth() override;
				uint32_t GetHeight() override;

			private:
				void FillView(unsigned int Target, unsigned int MipLevels, unsigned int Size);
			};

			class D3D11RenderTargetCube : public RenderTargetCube
			{
				friend D3D11Device;

			public:
				ID3D11DepthStencilView* DepthStencilView;
				ID3D11RenderTargetView* RenderTargetView;
				ID3D11Texture2D* Texture;

			public:
				D3D11RenderTargetCube(const Desc& I);
				virtual ~D3D11RenderTargetCube() override;
				void* GetTargetBuffer() override;
				void* GetDepthBuffer() override;
				uint32_t GetWidth() override;
				uint32_t GetHeight() override;
			};

			class D3D11MultiRenderTargetCube : public MultiRenderTargetCube
			{
				friend D3D11Device;

			public:
				ID3D11DepthStencilView* DepthStencilView;
				ID3D11RenderTargetView* RenderTargetView[8];
				ID3D11Texture2D* Texture[8];

			public:
				D3D11MultiRenderTargetCube(const Desc& I);
				virtual ~D3D11MultiRenderTargetCube() override;
				void* GetTargetBuffer() override;
				void* GetDepthBuffer() override;
				uint32_t GetWidth() override;
				uint32_t GetHeight() override;
			};

			class D3D11Cubemap : public Cubemap
			{
				friend D3D11Device;

			public:
				ID3D11Texture2D* Subresource;
				ID3D11Texture2D* Face;
				D3D11_SHADER_RESOURCE_VIEW_DESC Resource;
				D3D11_TEXTURE2D_DESC Texture;
				D3D11_TEXTURE2D_DESC Cube;
				D3D11_BOX Region;

			public:
				D3D11Cubemap(const Desc& I);
				virtual ~D3D11Cubemap() override;
			};

			class D3D11Query : public Query
			{
			public:
				ID3D11Query* Async;

			public:
				D3D11Query();
				virtual ~D3D11Query() override;
				void* GetResource() override;
			};

			class D3D11Device : public GraphicsDevice
			{
			private:
				struct
				{
					ID3D11VertexShader* VertexShader;
					ID3D11PixelShader* PixelShader;
					ID3D11InputLayout* VertexLayout;
					ID3D11Buffer* ConstantBuffer;
					ID3D11Buffer* VertexBuffer;
				} DirectRenderer;

			private:
				const char* VSP, *PSP, *GSP, *HSP, *DSP, *CSP;
				D3D11InputLayout* Layout;
				ID3D11Buffer* Buffer;

			public:
				ID3D11DeviceContext* ImmediateContext;
				ID3D11Device* D3DDevice;
				IDXGISwapChain* SwapChain;
				ID3D11Buffer* ConstantBuffer[3];
				DXGI_SWAP_CHAIN_DESC SwapChainResource;
				D3D_FEATURE_LEVEL FeatureLevel;
				D3D_DRIVER_TYPE DriverType;

			public:
				D3D11Device(const Desc& I);
				virtual ~D3D11Device() override;
				void SetConstantBuffers() override;
				void SetShaderModel(ShaderModel Model) override;
				void SetSamplerState(SamplerState* State, unsigned int Slot) override;
				void SetBlendState(BlendState* State) override;
				void SetRasterizerState(RasterizerState* State) override;
				void SetDepthStencilState(DepthStencilState* State) override;
				void SetInputLayout(InputLayout* State) override;
				void SetShader(Shader* Resource, unsigned int Type) override;
				void SetBuffer(Shader* Resource, unsigned int Slot, unsigned int Type) override;
				void SetBuffer(InstanceBuffer* Resource, unsigned int Slot) override;
				void SetStructureBuffer(ElementBuffer* Resource, unsigned int Slot) override;
				void SetIndexBuffer(ElementBuffer* Resource, Format FormatMode) override;
				void SetVertexBuffer(ElementBuffer* Resource, unsigned int Slot) override;
				void SetTexture2D(Texture2D* Resource, unsigned int Slot) override;
				void SetTexture3D(Texture3D* Resource, unsigned int Slot) override;
				void SetTextureCube(TextureCube* Resource, unsigned int Slot) override;
				void SetWriteable(ElementBuffer** Resource, unsigned int Count, unsigned int Slot) override;
				void SetWriteable(Texture2D** Resource, unsigned int Count, unsigned int Slot) override;
				void SetWriteable(Texture3D** Resource, unsigned int Count, unsigned int Slot) override;
				void SetWriteable(TextureCube** Resource, unsigned int Count, unsigned int Slot) override;
				void SetComputable(ElementBuffer** Resource, unsigned int Count, unsigned int Slot) override;
				void SetComputable(Texture2D** Resource, unsigned int Count, unsigned int Slot) override;
				void SetComputable(Texture3D** Resource, unsigned int Count, unsigned int Slot) override;
				void SetComputable(TextureCube** Resource, unsigned int Count, unsigned int Slot) override;
				void SetTarget(float R, float G, float B) override;
				void SetTarget() override;
				void SetTarget(DepthBuffer* Resource) override;
				void SetTarget(Graphics::RenderTarget* Resource, unsigned int Target, float R, float G, float B) override;
				void SetTarget(Graphics::RenderTarget* Resource, unsigned int Target) override;
				void SetTarget(Graphics::RenderTarget* Resource, float R, float G, float B) override;
				void SetTarget(Graphics::RenderTarget* Resource) override;
				void SetTargetMap(Graphics::RenderTarget* Resource, bool Enabled[8]) override;
				void SetTargetRect(unsigned int Width, unsigned int Height) override;
				void SetViewports(unsigned int Count, Viewport* Viewports) override;
				void SetScissorRects(unsigned int Count, Compute::Rectangle* Value) override;
				void SetPrimitiveTopology(PrimitiveTopology Topology) override;
				void FlushTexture2D(unsigned int Slot, unsigned int Count) override;
				void FlushTexture3D(unsigned int Slot, unsigned int Count) override;
				void FlushTextureCube(unsigned int Slot, unsigned int Count) override;
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
				void ClearDepth(DepthBuffer* Resource) override;
				void ClearDepth(Graphics::RenderTarget* Resource) override;
				void DrawIndexed(unsigned int Count, unsigned int IndexLocation, unsigned int BaseLocation) override;
				void DrawIndexed(MeshBuffer* Resource) override;
				void DrawIndexed(SkinMeshBuffer* Resource) override;
				void Draw(unsigned int Count, unsigned int Location) override;
				void Dispatch(unsigned int GroupX, unsigned int GroupY, unsigned int GroupZ);
				bool CopyTexture2D(Texture2D* Resource, Texture2D** Result) override;
				bool CopyTexture2D(Graphics::RenderTarget* Resource, unsigned int Target, Texture2D** Result) override;
				bool CopyTexture2D(RenderTargetCube* Resource, unsigned int Face, Texture2D** Result) override;
				bool CopyTexture2D(MultiRenderTargetCube* Resource, unsigned int Cube, unsigned int Face, Texture2D** Result) override;
				bool CopyTextureCube(RenderTargetCube* Resource, TextureCube** Result) override;
				bool CopyTextureCube(MultiRenderTargetCube* Resource, unsigned int Cube, TextureCube** Result) override;
				bool CopyTarget(Graphics::RenderTarget* From, unsigned int FromTarget, Graphics::RenderTarget* To, unsigned int ToTarget) override;
				bool CopyBackBuffer(Texture2D** Result) override;
				bool CopyBackBufferMSAA(Texture2D** Result) override;
				bool CopyBackBufferNoAA(Texture2D** Result) override;
				bool CubemapBegin(Cubemap* Resource) override;
				bool CubemapFace(Cubemap* Resource, unsigned int Target, unsigned int Face) override;
				bool CubemapEnd(Cubemap* Resource, TextureCube* Result) override;
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
				bool Begin() override;
				void Transform(const Compute::Matrix4x4& Transform) override;
				void Topology(PrimitiveTopology Topology) override;
				void Emit() override;
				void Texture(Texture2D* In) override;
				void Color(float X, float Y, float Z, float W) override;
				void Intensity(float Intensity) override;
				void TexCoord(float X, float Y) override;
				void TexCoordOffset(float X, float Y) override;
				void Position(float X, float Y, float Z) override;
				bool End() override;
				bool Submit() override;
				DepthStencilState* CreateDepthStencilState(const DepthStencilState::Desc& I) override;
				BlendState* CreateBlendState(const BlendState::Desc& I) override;
				RasterizerState* CreateRasterizerState(const RasterizerState::Desc& I) override;
				SamplerState* CreateSamplerState(const SamplerState::Desc& I) override;
				InputLayout* CreateInputLayout(const InputLayout::Desc& I) override;
				Shader* CreateShader(const Shader::Desc& I) override;
				ElementBuffer* CreateElementBuffer(const ElementBuffer::Desc& I) override;
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
				RenderTargetCube* CreateRenderTargetCube(const RenderTargetCube::Desc& I) override;
				MultiRenderTargetCube* CreateMultiRenderTargetCube(const MultiRenderTargetCube::Desc& I) override;
				Cubemap* CreateCubemap(const Cubemap::Desc& I) override;
				Query* CreateQuery(const Query::Desc& I) override;
				PrimitiveTopology GetPrimitiveTopology() override;
				ShaderModel GetSupportedShaderModel() override;
				void* GetDevice() override;
				void* GetContext() override;
				bool IsValid() override;
				bool CreateDirectBuffer(uint64_t Size);
				bool CreateTexture2D(Texture2D* Resource, DXGI_FORMAT Format);
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