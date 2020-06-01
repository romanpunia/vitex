#ifndef THAWK_GRAPHICS_DIRECTX11_H
#define THAWK_GRAPHICS_DIRECTX11_H

#include "../graphics.h"

#ifdef THAWK_MICROSOFT
#include <d3d11.h>
#include <d3dcompiler.h>

namespace Tomahawk
{
	namespace Graphics
	{
		namespace D3D11
		{
			class D3D11Device;

			struct D3D11DeviceState : public Graphics::DeviceState
			{
				ID3D11Buffer* CSConstantBuffer;
				ID3D11SamplerState* CSSampler;
				ID3D11ComputeShader* CSShader;
				ID3D11ShaderResourceView* CSShaderResource;
				ID3D11UnorderedAccessView* CSUnorderedAccessView;
				ID3D11Buffer* DSConstantBuffer;
				ID3D11SamplerState* DSSampler;
				ID3D11ShaderResourceView* DSShaderResource;
				ID3D11DomainShader* DSShader;
				ID3D11Buffer* GSConstantBuffer;
				ID3D11SamplerState* GSSampler;
				ID3D11ShaderResourceView* GSShaderResource;
				ID3D11GeometryShader* GSShader;
				ID3D11Buffer* HSConstantBuffer;
				ID3D11SamplerState* HSSampler;
				ID3D11ShaderResourceView* HSShaderResource;
				ID3D11HullShader* HSShader;
				ID3D11Buffer* PSConstantBuffer;
				ID3D11SamplerState* PSSampler;
				ID3D11ShaderResourceView* PSShaderResource;
				ID3D11PixelShader* PSShader;
				ID3D11Buffer* VSConstantBuffer;
				ID3D11SamplerState* VSSampler;
				ID3D11ShaderResourceView* VSShaderResource;
				ID3D11VertexShader* VSShader;
				ID3D11Buffer* IndexBuffer;
				DXGI_FORMAT IDXGIFormat;
				unsigned int IOffset;
				ID3D11InputLayout* InputLayout;
				D3D11_PRIMITIVE_TOPOLOGY PrimitiveTopology;
				ID3D11Buffer* VertexBuffer;
				unsigned int VStride, VOffset;
				ID3D11BlendState* BlendState;
				float BlendFactor[4];
				unsigned int SampleMask;
				ID3D11DepthStencilState* DepthStencilState;
				unsigned int StencilReference;
				ID3D11RenderTargetView* RenderTargetView;
				ID3D11DepthStencilView* DepthStencilView;
				unsigned int RectCount;
				D3D11_RECT Rects[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
				ID3D11RasterizerState* RasterizerState;
				unsigned int ViewportCount;
				D3D11_VIEWPORT Viewports[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
				unsigned int UAVInitialCounts;
			};

			class D3D11Shader : public Graphics::Shader
			{
			public:
				ID3D11VertexShader* VertexShader;
				ID3D11PixelShader* PixelShader;
				ID3D11GeometryShader* GeometryShader;
				ID3D11DomainShader* DomainShader;
				ID3D11HullShader* HullShader;
				ID3D11ComputeShader* ComputeShader;
				ID3D11InputLayout* VertexLayout;
				ID3D11Buffer* ConstantBuffer;

			public:
				D3D11Shader(Graphics::GraphicsDevice* Device, const Desc& I);
				virtual ~D3D11Shader() override;
				void UpdateBuffer(Graphics::GraphicsDevice* Device, const void* Data) override;
				void CreateBuffer(GraphicsDevice* Device, size_t Size) override;
				void SetShader(Graphics::GraphicsDevice* Device, unsigned int Type) override;
				void SetBuffer(Graphics::GraphicsDevice* Device, unsigned int Slot, unsigned int Type) override;
			};

			class D3D11ElementBuffer : public Graphics::ElementBuffer
			{
			public:
				ID3D11Buffer* Element;

			public:
				D3D11ElementBuffer(Graphics::GraphicsDevice* Device, const Desc& I);
				virtual ~D3D11ElementBuffer() override;
				void SetIndexBuffer(Graphics::GraphicsDevice* Device, Graphics::Format Format, unsigned int Offset) override;
				void SetVertexBuffer(Graphics::GraphicsDevice* Device, unsigned int Slot, unsigned int Stride, unsigned int Offset) override;
				void Map(Graphics::GraphicsDevice* Device, Graphics::ResourceMap Mode, Graphics::MappedSubresource* Map) override;
				void Unmap(Graphics::GraphicsDevice* Device, Graphics::MappedSubresource* Map) override;
				void* GetResource() override;
			};

			class D3D11StructureBuffer : public Graphics::StructureBuffer
			{
			public:
				ID3D11Buffer* Element;
				ID3D11ShaderResourceView* Resource;

			public:
				D3D11StructureBuffer(Graphics::GraphicsDevice* Device, const Desc& I);
				virtual ~D3D11StructureBuffer() override;
				void Map(Graphics::GraphicsDevice* Device, Graphics::ResourceMap Mode, Graphics::MappedSubresource* Map) override;
				void RemapSubresource(Graphics::GraphicsDevice* Device, void* Pointer, UInt64 Size) override;
				void Unmap(Graphics::GraphicsDevice* Device, Graphics::MappedSubresource* Map) override;
				void SetBuffer(Graphics::GraphicsDevice* Device, int Slot) override;
				void* GetElement() override;
				void* GetResource() override;
			};

			class D3D11Texture2D : public Graphics::Texture2D
			{
			public:
				ID3D11ShaderResourceView* Resource;
				ID3D11Texture2D* Rest;

			public:
				D3D11Texture2D(Graphics::GraphicsDevice* Device);
				D3D11Texture2D(Graphics::GraphicsDevice* Device, const Desc& I);
				virtual ~D3D11Texture2D() override;
				void SetTexture(Graphics::GraphicsDevice* Device, int Slot) override;
				void* GetResource() override;
				void Fill(D3D11Device* Device);
				void Generate(D3D11Device* Device);
			};

			class D3D11Texture3D : public Graphics::Texture3D
			{
			public:
				ID3D11ShaderResourceView* Resource;
				ID3D11Texture3D* Rest;

			public:
				D3D11Texture3D(Graphics::GraphicsDevice* Device);
				virtual ~D3D11Texture3D() override;
				void SetTexture(Graphics::GraphicsDevice* Device, int Slot) override;
				void* GetResource() override;
			};

			class D3D11TextureCube : public Graphics::TextureCube
			{
			public:
				ID3D11ShaderResourceView* Resource;
				ID3D11Texture2D* Rest;

			public:
				D3D11TextureCube(Graphics::GraphicsDevice* Device);
				D3D11TextureCube(Graphics::GraphicsDevice* Device, const Desc& I);
				virtual ~D3D11TextureCube() override;
				void SetTexture(Graphics::GraphicsDevice* Device, int Slot) override;
				void* GetResource() override;
			};

			class D3D11RenderTarget2D : public Graphics::RenderTarget2D
			{
			public:
				ID3D11RenderTargetView* RenderTargetView;
				ID3D11Texture2D* Texture;
				ID3D11DepthStencilView* DepthStencilView;
				D3D11_VIEWPORT Viewport;

			public:
				D3D11RenderTarget2D(Graphics::GraphicsDevice* Device, const Desc& I);
				virtual ~D3D11RenderTarget2D() override;
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

			class D3D11MultiRenderTarget2D : public Graphics::MultiRenderTarget2D
			{
			public:
				struct
				{
					ID3D11Texture2D* Face = nullptr;
					ID3D11Texture2D* Subresource = nullptr;
					D3D11_SHADER_RESOURCE_VIEW_DESC Resource;
					D3D11_TEXTURE2D_DESC CubeMap;
					D3D11_TEXTURE2D_DESC Texture;
					D3D11_BOX Region;
					int Target = -1;
				} View;

			public:
				D3D11_TEXTURE2D_DESC Information;
				D3D11_VIEWPORT Viewport;
				ID3D11RenderTargetView* RenderTargetView[8];
				ID3D11Texture2D* Texture[8];
				ID3D11DepthStencilView* DepthStencilView;

			public:
				D3D11MultiRenderTarget2D(Graphics::GraphicsDevice* Device, const Desc& I);
				virtual ~D3D11MultiRenderTarget2D() override;
				void SetTarget(Graphics::GraphicsDevice* Device, int Target, float R, float G, float B) override;
				void SetTarget(Graphics::GraphicsDevice* Device, int Target) override;
				void SetTarget(Graphics::GraphicsDevice* Device, float R, float G, float B) override;
				void SetTarget(Graphics::GraphicsDevice* Device) override;
				void Clear(Graphics::GraphicsDevice* Device, int Target, float R, float G, float B) override;
				void CopyTargetTo(int Target, Graphics::GraphicsDevice* Device, Graphics::RenderTarget2D* Output) override;
				void CopyTargetFrom(int Target, Graphics::GraphicsDevice* Device, Graphics::RenderTarget2D* Output) override;
				void CopyTexture2D(int Target, Graphics::GraphicsDevice* Device, Graphics::Texture2D** Value) override;
				void CopyBegin(GraphicsDevice* Device, int Target, unsigned int MipLevels, unsigned int Size) override;
				void CopyFace(GraphicsDevice* Device, int Target, int Face) override;
				void CopyEnd(GraphicsDevice* Device, TextureCube* Value) override;
				void SetViewport(const Graphics::Viewport& In) override;
				Graphics::Viewport GetViewport() override;
				float GetWidth() override;
				float GetHeight() override;
				void* GetResource(int Id) override;

			private:
				void FillView(int Target, unsigned int MipLevels, unsigned int Size);
			};

			class D3D11RenderTarget2DArray : public Graphics::RenderTarget2DArray
			{
			public:
				std::vector<ID3D11RenderTargetView*> RenderTargetView;
				ID3D11DepthStencilView* DepthStencilView;
				ID3D11Texture2D* Texture;
				D3D11_TEXTURE2D_DESC Information;
				D3D11_VIEWPORT Viewport;

			public:
				D3D11RenderTarget2DArray(Graphics::GraphicsDevice* Device, const Desc& I);
				virtual ~D3D11RenderTarget2DArray() override;
				void SetTarget(Graphics::GraphicsDevice* Device, int Target, float R, float G, float B) override;
				void SetTarget(Graphics::GraphicsDevice* Device, int Target) override;
				void Clear(Graphics::GraphicsDevice* Device, int Target, float R, float G, float B) override;
				void SetViewport(const Graphics::Viewport& In) override;
				Graphics::Viewport GetViewport() override;
				float GetWidth() override;
				float GetHeight() override;
				void* GetResource() override;
			};

			class D3D11RenderTargetCube : public Graphics::RenderTargetCube
			{
			public:
				ID3D11DepthStencilView* DepthStencilView;
				ID3D11RenderTargetView* RenderTargetView;
				ID3D11Texture2D* Texture;
				ID3D11Texture2D* Cube;
				D3D11_VIEWPORT Viewport;

			public:
				D3D11RenderTargetCube(Graphics::GraphicsDevice* Device, const Desc& I);
				virtual ~D3D11RenderTargetCube() override;
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

			class D3D11MultiRenderTargetCube : public Graphics::MultiRenderTargetCube
			{
			public:
				ID3D11DepthStencilView* DepthStencilView;
				ID3D11RenderTargetView* RenderTargetView[8];
				ID3D11Texture2D* Texture[8];
				ID3D11Texture2D* Cube[8];
				D3D11_VIEWPORT Viewport;
				Graphics::SurfaceTarget SVTarget;

			public:
				D3D11MultiRenderTargetCube(Graphics::GraphicsDevice* Device, const Desc& I);
				virtual ~D3D11MultiRenderTargetCube() override;
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

			class D3D11Mesh : public Graphics::Mesh
			{
			public:
				D3D11Mesh(Graphics::GraphicsDevice* Device, const Desc& I);
				void Update(Graphics::GraphicsDevice* Device, Compute::Vertex* Elements) override;
				void Draw(Graphics::GraphicsDevice* Device) override;
				Compute::Vertex* Elements(Graphics::GraphicsDevice* Device) override;
			};

			class D3D11SkinnedMesh : public Graphics::SkinnedMesh
			{
			public:
				D3D11SkinnedMesh(Graphics::GraphicsDevice* Device, const Desc& I);
				void Update(Graphics::GraphicsDevice* Device, Compute::InfluenceVertex* Elements) override;
				void Draw(Graphics::GraphicsDevice* Device) override;
				Compute::InfluenceVertex* Elements(Graphics::GraphicsDevice* Device) override;
			};

			class D3D11InstanceBuffer : public Graphics::InstanceBuffer
			{
			private:
				bool SynchronizationState;

			public:
				ID3D11ShaderResourceView* Resource;

			public:
				D3D11InstanceBuffer(Graphics::GraphicsDevice* Device, const Desc& I);
				virtual ~D3D11InstanceBuffer() override;
				void Update() override;
				void Restore() override;
				void Resize(UInt64 Size) override;
				void SetResource(Graphics::GraphicsDevice* Device, int Slot) override;
			};

			class D3D11DirectBuffer : public Graphics::DirectBuffer
			{
			private:
				struct ConstantBuffer
				{
					Compute::Matrix4x4 WorldViewProjection;
					Compute::Vector4 Padding;
				};

			private:
				ID3DBlob* VertexShaderBlob;
				ID3D11VertexShader* VertexShader;
				ID3D11InputLayout* InputLayout;
				ID3D11Buffer* VertexConstantBuffer;
				ID3DBlob* PixelShaderBlob;
				ID3D11PixelShader* PixelShader;
				ID3D11Buffer* ElementBuffer;
				ConstantBuffer Buffer;

			public:
				D3D11DirectBuffer(Graphics::GraphicsDevice* NewDevice);
				virtual ~D3D11DirectBuffer() override;
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
				void AllocVertexBuffer(UInt64 Size);
			};

			class D3D11Device : public Graphics::GraphicsDevice
			{
			private:
				const char* VSP, * PSP, * GSP, * HSP, * DSP, * CSP;

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
				void RestoreSamplerStates() override;
				void RestoreBlendStates() override;
				void RestoreRasterizerStates() override;
				void RestoreDepthStencilStates() override;
				void RestoreState(Graphics::DeviceState* RefState) override;
				void ReleaseState(Graphics::DeviceState** RefState) override;
				void SetConstantBuffers() override;
				void SetShaderModel(Graphics::ShaderModel RShaderModel) override;
				UInt64 AddDepthStencilState(Graphics::DepthStencilState* In) override;
				UInt64 AddBlendState(Graphics::BlendState* In) override;
				UInt64 AddRasterizerState(Graphics::RasterizerState* In) override;
				UInt64 AddSamplerState(Graphics::SamplerState* In) override;
				void SetSamplerState(UInt64 State) override;
				void SetBlendState(UInt64 State) override;
				void SetRasterizerState(UInt64 State) override;
				void SetDepthStencilState(UInt64 State) override;
				void UpdateBuffer(Graphics::RenderBufferType Buffer) override;
				void Present() override;
				void SetPrimitiveTopology(Graphics::PrimitiveTopology Topology) override;
				void RestoreTexture2D(int Slot, int Size) override;
				void RestoreTexture2D(int Size) override;
				void RestoreTexture3D(int Slot, int Size) override;
				void RestoreTexture3D(int Size) override;
				void RestoreTextureCube(int Slot, int Size) override;
				void RestoreTextureCube(int Size) override;
				void RestoreVertexBuffer(int Slot) override;
				void RestoreIndexBuffer() override;
				void RestoreState() override;
				void RestoreShader(unsigned int Type) override;
				void ResizeBuffers(unsigned int Width, unsigned int Height) override;
				void DrawIndexed(unsigned int Count, unsigned int IndexLocation, unsigned int BaseLocation) override;
				void Draw(unsigned int Count, unsigned int Start) override;
				void SetViewport(unsigned int Count, Graphics::Viewport* Viewports) override;
				void SetScissorRect(unsigned int Count, Graphics::Rectangle* Value) override;
				void GetViewport(unsigned int* Count, Graphics::Viewport* Out) override;
				void GetScissorRect(unsigned int* Count, Graphics::Rectangle* Out) override;
				void LoadShaderSections() override;
				Graphics::ShaderModel GetSupportedShaderModel() override;
				Graphics::PrimitiveTopology GetPrimitiveTopology() override;
				void* GetDevice() override;
				void* GetContext() override;
				void* GetBackBuffer() override;
				void* GetBackBufferMSAA() override;
				void* GetBackBufferNoAA() override;
				Graphics::DeviceState* CreateState() override;
				bool IsValid() override;
				int CreateConstantBuffer(int Size, ID3D11Buffer*& Buffer);
				char* GetVSProfile();
				char* GetPSProfile();
				char* GetGSProfile();
				char* GetHSProfile();
				char* GetCSProfile();
				char* GetDSProfile();
				char* CompileState(ID3DBlob* Error);
			};
		}
	}
}
#endif
#endif