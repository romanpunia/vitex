#include "d3d11.h"
#ifdef TH_HAS_SDL2
#include <SDL2/SDL_syswm.h>
#endif
#ifdef TH_MICROSOFT
#define ReleaseCom(Value) { if (Value != nullptr) { Value->Release(); Value = nullptr; } }
#define HLSL_INLINE(Code) #Code

namespace Tomahawk
{
	namespace Graphics
	{
		namespace D3D11
		{
			D3D11DepthStencilState::D3D11DepthStencilState(const Desc& I) : DepthStencilState(I), Resource(nullptr)
			{
			}
			D3D11DepthStencilState::~D3D11DepthStencilState()
			{
				ReleaseCom(Resource);
			}
			void* D3D11DepthStencilState::GetResource()
			{
				return Resource;
			}

			D3D11RasterizerState::D3D11RasterizerState(const Desc& I) : RasterizerState(I), Resource(nullptr)
			{
			}
			D3D11RasterizerState::~D3D11RasterizerState()
			{
				ReleaseCom(Resource);
			}
			void* D3D11RasterizerState::GetResource()
			{
				return Resource;
			}

			D3D11BlendState::D3D11BlendState(const Desc& I) : BlendState(I), Resource(nullptr)
			{
			}
			D3D11BlendState::~D3D11BlendState()
			{
				ReleaseCom(Resource);
			}
			void* D3D11BlendState::GetResource()
			{
				return Resource;
			}

			D3D11SamplerState::D3D11SamplerState(const Desc& I) : SamplerState(I), Resource(nullptr)
			{
			}
			D3D11SamplerState::~D3D11SamplerState()
			{
				ReleaseCom(Resource);
			}
			void* D3D11SamplerState::GetResource()
			{
				return Resource;
			}

			D3D11InputLayout::D3D11InputLayout(const Desc& I) : InputLayout(I)
			{
			}
			D3D11InputLayout::~D3D11InputLayout()
			{
			}
			void* D3D11InputLayout::GetResource()
			{
				return (void*)this;
			}

			D3D11Shader::D3D11Shader(const Desc& I) : Shader(I), Compiled(false)
			{
				VertexShader = nullptr;
				PixelShader = nullptr;
				GeometryShader = nullptr;
				HullShader = nullptr;
				DomainShader = nullptr;
				ComputeShader = nullptr;
				ConstantBuffer = nullptr;
				VertexLayout = nullptr;
				Signature = nullptr;
			}
			D3D11Shader::~D3D11Shader()
			{
				ReleaseCom(ConstantBuffer);
				ReleaseCom(VertexShader);
				ReleaseCom(PixelShader);
				ReleaseCom(GeometryShader);
				ReleaseCom(DomainShader);
				ReleaseCom(HullShader);
				ReleaseCom(ComputeShader);
				ReleaseCom(VertexLayout);
				ReleaseCom(Signature);
			}
			bool D3D11Shader::IsValid()
			{
				return Compiled;
			}

			D3D11ElementBuffer::D3D11ElementBuffer(const Desc& I) : ElementBuffer(I)
			{
				Resource = nullptr;
				Element = nullptr;
				Access = nullptr;
			}
			D3D11ElementBuffer::~D3D11ElementBuffer()
			{
				ReleaseCom(Resource);
				ReleaseCom(Element);
				ReleaseCom(Access);
			}
			void* D3D11ElementBuffer::GetResource()
			{
				return (void*)Element;
			}

			D3D11MeshBuffer::D3D11MeshBuffer(const Desc& I) : MeshBuffer(I)
			{
			}
			Compute::Vertex* D3D11MeshBuffer::GetElements(GraphicsDevice* Device)
			{
				MappedSubresource Resource;
				Device->Map(VertexBuffer, ResourceMap_Write, &Resource);

				Compute::Vertex* Vertices = (Compute::Vertex*)TH_MALLOC(sizeof(Compute::Vertex) * (unsigned int)VertexBuffer->GetElements());
				memcpy(Vertices, Resource.Pointer, (size_t)VertexBuffer->GetElements() * sizeof(Compute::Vertex));

				Device->Unmap(VertexBuffer, &Resource);
				return Vertices;
			}

			D3D11SkinMeshBuffer::D3D11SkinMeshBuffer(const Desc& I) : SkinMeshBuffer(I)
			{
			}
			Compute::SkinVertex* D3D11SkinMeshBuffer::GetElements(GraphicsDevice* Device)
			{
				MappedSubresource Resource;
				Device->Map(VertexBuffer, ResourceMap_Write, &Resource);

				Compute::SkinVertex* Vertices = (Compute::SkinVertex*)TH_MALLOC(sizeof(Compute::SkinVertex) * (unsigned int)VertexBuffer->GetElements());
				memcpy(Vertices, Resource.Pointer, (size_t)VertexBuffer->GetElements() * sizeof(Compute::SkinVertex));

				Device->Unmap(VertexBuffer, &Resource);
				return Vertices;
			}

			D3D11InstanceBuffer::D3D11InstanceBuffer(const Desc& I) : InstanceBuffer(I), Resource(nullptr)
			{
			}
			D3D11InstanceBuffer::~D3D11InstanceBuffer()
			{
				if (Device != nullptr && Sync)
					Device->ClearBuffer(this);

				ReleaseCom(Resource);
			}

			D3D11Texture2D::D3D11Texture2D() : Texture2D(), Resource(nullptr), View(nullptr), Access(nullptr)
			{
			}
			D3D11Texture2D::D3D11Texture2D(const Desc& I) : Texture2D(I), Resource(nullptr), View(nullptr), Access(nullptr)
			{
			}
			D3D11Texture2D::~D3D11Texture2D()
			{
				ReleaseCom(View);
				ReleaseCom(Resource);
				ReleaseCom(Access);
			}
			void* D3D11Texture2D::GetResource()
			{
				return (void*)Resource;
			}

			D3D11Texture3D::D3D11Texture3D() : Texture3D(), Resource(nullptr), View(nullptr), Access(nullptr)
			{
			}
			D3D11Texture3D::~D3D11Texture3D()
			{
				ReleaseCom(View);
				ReleaseCom(Resource);
				ReleaseCom(Access);
			}
			void* D3D11Texture3D::GetResource()
			{
				return (void*)Resource;
			}

			D3D11TextureCube::D3D11TextureCube() : TextureCube(), Resource(nullptr), View(nullptr), Access(nullptr)
			{
			}
			D3D11TextureCube::D3D11TextureCube(const Desc& I) : TextureCube(I), Resource(nullptr), View(nullptr), Access(nullptr)
			{
			}
			D3D11TextureCube::~D3D11TextureCube()
			{
				ReleaseCom(View);
				ReleaseCom(Resource);
				ReleaseCom(Access);
			}
			void* D3D11TextureCube::GetResource()
			{
				return (void*)Resource;
			}

			D3D11DepthBuffer::D3D11DepthBuffer(const Desc& I) : DepthBuffer(I)
			{
				DepthStencilView = nullptr;
			}
			D3D11DepthBuffer::~D3D11DepthBuffer()
			{
				ReleaseCom(DepthStencilView);
			}
			void* D3D11DepthBuffer::GetResource()
			{
				return DepthStencilView;
			}
			uint32_t D3D11DepthBuffer::GetWidth()
			{
				return Viewarea.Width;
			}
			uint32_t D3D11DepthBuffer::GetHeight()
			{
				return Viewarea.Height;
			}

			D3D11RenderTarget2D::D3D11RenderTarget2D(const Desc& I) : RenderTarget2D(I)
			{
				RenderTargetView = nullptr;
				DepthStencilView = nullptr;
				Texture = nullptr;
			}
			D3D11RenderTarget2D::~D3D11RenderTarget2D()
			{
				ReleaseCom(Texture);
				ReleaseCom(DepthStencilView);
				ReleaseCom(RenderTargetView);
			}
			void* D3D11RenderTarget2D::GetTargetBuffer()
			{
				return (void*)&RenderTargetView;
			}
			void* D3D11RenderTarget2D::GetDepthBuffer()
			{
				return (void*)DepthStencilView;
			}
			uint32_t D3D11RenderTarget2D::GetWidth()
			{
				return Viewarea.Width;
			}
			uint32_t D3D11RenderTarget2D::GetHeight()
			{
				return Viewarea.Height;
			}

			D3D11MultiRenderTarget2D::D3D11MultiRenderTarget2D(const Desc& I) : MultiRenderTarget2D(I), DepthStencilView(nullptr)
			{
				ZeroMemory(&Information, sizeof(Information));
				for (unsigned int i = 0; i < 8; i++)
				{
					RenderTargetView[i] = nullptr;
					Texture[i] = nullptr;
				}
			}
			D3D11MultiRenderTarget2D::~D3D11MultiRenderTarget2D()
			{
				ReleaseCom(DepthStencilView);
				for (unsigned int i = 0; i < 8; i++)
				{
					ReleaseCom(Texture[i]);
					ReleaseCom(RenderTargetView[i]);
				}
			}
			void* D3D11MultiRenderTarget2D::GetTargetBuffer()
			{
				return (void*)RenderTargetView;
			}
			void* D3D11MultiRenderTarget2D::GetDepthBuffer()
			{
				return (void*)DepthStencilView;
			}
			uint32_t D3D11MultiRenderTarget2D::GetWidth()
			{
				return Viewarea.Width;
			}
			uint32_t D3D11MultiRenderTarget2D::GetHeight()
			{
				return Viewarea.Height;
			}

			D3D11RenderTargetCube::D3D11RenderTargetCube(const Desc& I) : RenderTargetCube(I)
			{
				DepthStencilView = nullptr;
				RenderTargetView = nullptr;
				Texture = nullptr;
			}
			D3D11RenderTargetCube::~D3D11RenderTargetCube()
			{
				ReleaseCom(DepthStencilView);
				ReleaseCom(RenderTargetView);
				ReleaseCom(Texture);
			}
			void* D3D11RenderTargetCube::GetTargetBuffer()
			{
				return (void*)&RenderTargetView;
			}
			void* D3D11RenderTargetCube::GetDepthBuffer()
			{
				return (void*)DepthStencilView;
			}
			uint32_t D3D11RenderTargetCube::GetWidth()
			{
				return Viewarea.Width;
			}
			uint32_t D3D11RenderTargetCube::GetHeight()
			{
				return Viewarea.Height;
			}

			D3D11MultiRenderTargetCube::D3D11MultiRenderTargetCube(const Desc& I) : MultiRenderTargetCube(I), DepthStencilView(nullptr)
			{
				for (unsigned int i = 0; i < 8; i++)
				{
					RenderTargetView[i] = nullptr;
					Texture[i] = nullptr;
					Resource[i] = nullptr;
				}
			}
			D3D11MultiRenderTargetCube::~D3D11MultiRenderTargetCube()
			{
				for (unsigned int i = 0; i < Target; i++)
				{
					ReleaseCom(RenderTargetView[i]);
					ReleaseCom(Texture[i]);
				}
				ReleaseCom(DepthStencilView);
			}
			void* D3D11MultiRenderTargetCube::GetTargetBuffer()
			{
				return (void*)RenderTargetView;
			}
			void* D3D11MultiRenderTargetCube::GetDepthBuffer()
			{
				return (void*)DepthStencilView;
			}
			uint32_t D3D11MultiRenderTargetCube::GetWidth()
			{
				return Viewarea.Width;
			}
			uint32_t D3D11MultiRenderTargetCube::GetHeight()
			{
				return Viewarea.Height;
			}

			D3D11Cubemap::D3D11Cubemap(const Desc& I) : Cubemap(I), Subresource(nullptr), Face(nullptr)
			{
				if (!I.Source || I.Target >= I.Source->GetTargetCount())
				{
					TH_ERROR("no correct render target found for cubemap");
					Meta.Source = nullptr;
					return;
				}

				D3D11Texture2D* Source = (D3D11Texture2D*)I.Source->GetTarget(I.Target);
				if (!Source || !Source->View)
				{
					TH_ERROR("render target configuration error");
					Meta.Source = nullptr;
					return;
				}
				
				Source->View->GetDesc(&Texture);
				Texture.ArraySize = 1;
				Texture.CPUAccessFlags = 0;
				Texture.MiscFlags = 0;
				Texture.MipLevels = I.MipLevels;
				Cube = Texture;
				Cube.MipLevels = I.MipLevels;
				Cube.ArraySize = 6;
				Cube.Usage = D3D11_USAGE_DEFAULT;
				Cube.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
				Cube.CPUAccessFlags = 0;
				Cube.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE | D3D11_RESOURCE_MISC_GENERATE_MIPS;
				Resource.Format = Texture.Format;
				Resource.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
				Resource.TextureCube.MostDetailedMip = 0;
				Resource.TextureCube.MipLevels = I.MipLevels;
				Region = { 0, 0, 0, (unsigned int)I.Size, (unsigned int)I.Size, 1 };
			}
			D3D11Cubemap::~D3D11Cubemap()
			{
				ReleaseCom(Face);
				ReleaseCom(Subresource);
			}

			D3D11Query::D3D11Query() : Query(), Async(nullptr)
			{
			}
			D3D11Query::~D3D11Query()
			{
				if (Async != nullptr)
					Async->Release();
			}
			void* D3D11Query::GetResource()
			{
				return (void*)Async;
			}

			D3D11Device::D3D11Device(const Desc& I) : GraphicsDevice(I), ImmediateContext(nullptr), SwapChain(nullptr), D3DDevice(nullptr), DriverType(D3D_DRIVER_TYPE_HARDWARE), FeatureLevel(D3D_FEATURE_LEVEL_11_0)
			{
				DirectRenderer.VertexShader = nullptr;
				DirectRenderer.VertexLayout = nullptr;
				DirectRenderer.ConstantBuffer = nullptr;
				DirectRenderer.PixelShader = nullptr;
				DirectRenderer.VertexBuffer = nullptr;
				ConstantBuffer[0] = nullptr;
				ConstantBuffer[1] = nullptr;
				ConstantBuffer[2] = nullptr;

				unsigned int CreationFlags = I.CreationFlags | D3D11_CREATE_DEVICE_DISABLE_GPU_TIMEOUT;
				if (I.Debug)
					CreationFlags |= D3D11_CREATE_DEVICE_DEBUG;

				D3D_FEATURE_LEVEL FeatureLevels[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0, D3D_FEATURE_LEVEL_9_3, D3D_FEATURE_LEVEL_9_2, D3D_FEATURE_LEVEL_9_1, };
				ZeroMemory(&SwapChainResource, sizeof(SwapChainResource));
				SwapChainResource.BufferCount = 2;
				SwapChainResource.BufferDesc.Width = I.BufferWidth;
				SwapChainResource.BufferDesc.Height = I.BufferHeight;
				SwapChainResource.BufferDesc.Format = (DXGI_FORMAT)I.BufferFormat;
				SwapChainResource.BufferDesc.RefreshRate.Numerator = 60;
				SwapChainResource.BufferDesc.RefreshRate.Denominator = 1;
				SwapChainResource.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
				SwapChainResource.SampleDesc.Count = 1;
				SwapChainResource.SampleDesc.Quality = 0;
				SwapChainResource.Windowed = I.IsWindowed;
				SwapChainResource.Flags = 0;
				SwapChainResource.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
				SwapChainResource.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
				SwapChainResource.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
#if defined(TH_MICROSOFT) && defined(TH_HAS_SDL2)
				if (I.Window != nullptr)
				{
					SDL_SysWMinfo Info;
					I.Window->Load(&Info);
					SwapChainResource.OutputWindow = (HWND)Info.info.win.window;
				}
#endif
				if (D3D11CreateDeviceAndSwapChain(nullptr, DriverType, nullptr, CreationFlags, FeatureLevels, ARRAYSIZE(FeatureLevels), D3D11_SDK_VERSION, &SwapChainResource, &SwapChain, &D3DDevice, &FeatureLevel, &ImmediateContext) != S_OK)
				{
					TH_ERROR("couldn't create swap chain, device or immediate context");
					return;
				}

				CreateConstantBuffer(&ConstantBuffer[0], sizeof(AnimationBuffer));
				Constants[0] = &Animation;

				CreateConstantBuffer(&ConstantBuffer[1], sizeof(RenderBuffer));
				Constants[1] = &Render;

				CreateConstantBuffer(&ConstantBuffer[2], sizeof(ViewBuffer));
				Constants[2] = &View;

				ImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
				ImmediateContext->VSSetConstantBuffers(0, 3, ConstantBuffer);
				ImmediateContext->PSSetConstantBuffers(0, 3, ConstantBuffer);
				ImmediateContext->GSSetConstantBuffers(0, 3, ConstantBuffer);
				ImmediateContext->DSSetConstantBuffers(0, 3, ConstantBuffer);
				ImmediateContext->HSSetConstantBuffers(0, 3, ConstantBuffer);
				ImmediateContext->CSSetConstantBuffers(0, 3, ConstantBuffer);

				SetShaderModel(I.ShaderMode == ShaderModel_Auto ? GetSupportedShaderModel() : I.ShaderMode);
				ResizeBuffers(I.BufferWidth, I.BufferHeight);
				CreateStates();

				Shader::Desc F = Shader::Desc();
				if (GetSection("geometry/basic/geometry", &F))
					BasicEffect = CreateShader(F);
			}
			D3D11Device::~D3D11Device()
			{
				ReleaseProxy();
				ReleaseCom(DirectRenderer.VertexShader);
				ReleaseCom(DirectRenderer.VertexLayout);
				ReleaseCom(DirectRenderer.ConstantBuffer);
				ReleaseCom(DirectRenderer.PixelShader);
				ReleaseCom(DirectRenderer.VertexBuffer);
				ReleaseCom(ConstantBuffer[0]);
				ReleaseCom(ConstantBuffer[1]);
				ReleaseCom(ConstantBuffer[2]);
				ReleaseCom(ImmediateContext);
				ReleaseCom(SwapChain);

				if (Debug)
				{
					ID3D11Debug* Debugger = nullptr;
					D3DDevice->QueryInterface(__uuidof(ID3D11Debug), reinterpret_cast<void**>(&Debugger));

					if (Debugger != nullptr)
					{
						D3D11_RLDO_FLAGS Flags = (D3D11_RLDO_FLAGS)(D3D11_RLDO_DETAIL | 0x4); // D3D11_RLDO_IGNORE_INTERNAL
						Debugger->ReportLiveDeviceObjects(Flags);
						Debugger->Release();
					}
				}

				ReleaseCom(D3DDevice);
			}
			void D3D11Device::SetConstantBuffers()
			{
				ImmediateContext->VSSetConstantBuffers(0, 3, ConstantBuffer);
				ImmediateContext->PSSetConstantBuffers(0, 3, ConstantBuffer);
				ImmediateContext->GSSetConstantBuffers(0, 3, ConstantBuffer);
				ImmediateContext->DSSetConstantBuffers(0, 3, ConstantBuffer);
				ImmediateContext->HSSetConstantBuffers(0, 3, ConstantBuffer);
				ImmediateContext->CSSetConstantBuffers(0, 3, ConstantBuffer);
			}
			void D3D11Device::SetShaderModel(ShaderModel Model)
			{
				ShaderModelType = Model;
				if (ShaderModelType == ShaderModel_HLSL_1_0)
				{
					VSP = "vs_1_0";
					PSP = "ps_1_0";
					GSP = "gs_1_0";
					CSP = "cs_1_0";
					DSP = "ds_1_0";
					HSP = "hs_1_0";
				}
				else if (ShaderModelType == ShaderModel_HLSL_2_0)
				{
					VSP = "vs_2_0";
					PSP = "ps_2_0";
					GSP = "gs_2_0";
					CSP = "cs_2_0";
					DSP = "ds_2_0";
					HSP = "hs_2_0";
				}
				else if (ShaderModelType == ShaderModel_HLSL_3_0)
				{
					VSP = "vs_3_0";
					PSP = "ps_3_0";
					GSP = "gs_3_0";
					CSP = "cs_3_0";
					DSP = "ds_3_0";
					HSP = "hs_3_0";
				}
				else if (ShaderModelType == ShaderModel_HLSL_4_0)
				{
					VSP = "vs_4_0";
					PSP = "ps_4_0";
					GSP = "gs_4_0";
					CSP = "cs_4_0";
					DSP = "ds_4_0";
					HSP = "hs_4_0";
				}
				else if (ShaderModelType == ShaderModel_HLSL_4_1)
				{
					VSP = "vs_4_1";
					PSP = "ps_4_1";
					GSP = "gs_4_1";
					CSP = "cs_4_1";
					DSP = "ds_4_1";
					HSP = "hs_4_1";
				}
				else if (ShaderModelType == ShaderModel_HLSL_5_0)
				{
					VSP = "vs_5_0";
					PSP = "ps_5_0";
					GSP = "gs_5_0";
					CSP = "cs_5_0";
					DSP = "ds_5_0";
					HSP = "hs_5_0";
				}
				else
				{
					VSP = "";
					PSP = VSP;
					GSP = VSP;
					CSP = VSP;
					DSP = VSP;
					HSP = VSP;
				}
			}
			void D3D11Device::SetBlendState(BlendState* State)
			{
				ID3D11BlendState* NewState = (ID3D11BlendState*)(State ? State->GetResource() : nullptr);
				ImmediateContext->OMSetBlendState(NewState, 0, 0xffffffff);
			}
			void D3D11Device::SetRasterizerState(RasterizerState* State)
			{
				ID3D11RasterizerState* NewState = (ID3D11RasterizerState*)(State ? State->GetResource() : nullptr);
				ImmediateContext->RSSetState(NewState);
			}
			void D3D11Device::SetDepthStencilState(DepthStencilState* State)
			{
				ID3D11DepthStencilState* NewState = (ID3D11DepthStencilState*)(State ? State->GetResource() : nullptr);
				ImmediateContext->OMSetDepthStencilState(NewState, 1);
			}
			void D3D11Device::SetInputLayout(InputLayout* Resource)
			{
				Layout = (D3D11InputLayout*)Resource;
			}
			void D3D11Device::SetShader(Shader* Resource, unsigned int Type)
			{
				D3D11Shader* IResource = Resource->As<D3D11Shader>();
				if (Type & ShaderType_Vertex)
					ImmediateContext->VSSetShader(IResource ? IResource->VertexShader : nullptr, nullptr, 0);

				if (Type & ShaderType_Pixel)
					ImmediateContext->PSSetShader(IResource ? IResource->PixelShader : nullptr, nullptr, 0);

				if (Type & ShaderType_Geometry)
					ImmediateContext->GSSetShader(IResource ? IResource->GeometryShader : nullptr, nullptr, 0);

				if (Type & ShaderType_Hull)
					ImmediateContext->HSSetShader(IResource ? IResource->HullShader : nullptr, nullptr, 0);

				if (Type & ShaderType_Domain)
					ImmediateContext->DSSetShader(IResource ? IResource->DomainShader : nullptr, nullptr, 0);

				if (Type & ShaderType_Compute)
					ImmediateContext->CSSetShader(IResource ? IResource->ComputeShader : nullptr, nullptr, 0);

				ImmediateContext->IASetInputLayout(GenerateInputLayout(IResource));
			}
			void D3D11Device::SetSamplerState(SamplerState* State, unsigned int Slot, unsigned int Type)
			{
				ID3D11SamplerState* NewState = (ID3D11SamplerState*)(State ? State->GetResource() : nullptr);
				if (Type & ShaderType_Vertex)
					ImmediateContext->VSSetSamplers(Slot, 1, &NewState);

				if (Type & ShaderType_Pixel)
					ImmediateContext->PSSetSamplers(Slot, 1, &NewState);

				if (Type & ShaderType_Geometry)
					ImmediateContext->GSSetSamplers(Slot, 1, &NewState);

				if (Type & ShaderType_Hull)
					ImmediateContext->HSSetSamplers(Slot, 1, &NewState);

				if (Type & ShaderType_Domain)
					ImmediateContext->DSSetSamplers(Slot, 1, &NewState);

				if (Type & ShaderType_Compute)
					ImmediateContext->CSSetSamplers(Slot, 1, &NewState);
			}
			void D3D11Device::SetBuffer(Shader* Resource, unsigned int Slot, unsigned int Type)
			{
				ID3D11Buffer* IBuffer = (Resource ? Resource->As<D3D11Shader>()->ConstantBuffer : nullptr);
				if (Type & ShaderType_Vertex)
					ImmediateContext->VSSetConstantBuffers(Slot, 1, &IBuffer);

				if (Type & ShaderType_Pixel)
					ImmediateContext->PSSetConstantBuffers(Slot, 1, &IBuffer);

				if (Type & ShaderType_Geometry)
					ImmediateContext->GSSetConstantBuffers(Slot, 1, &IBuffer);

				if (Type & ShaderType_Hull)
					ImmediateContext->HSSetConstantBuffers(Slot, 1, &IBuffer);

				if (Type & ShaderType_Domain)
					ImmediateContext->DSSetConstantBuffers(Slot, 1, &IBuffer);

				if (Type & ShaderType_Compute)
					ImmediateContext->CSSetConstantBuffers(Slot, 1, &IBuffer);
			}
			void D3D11Device::SetBuffer(InstanceBuffer* Resource, unsigned int Slot, unsigned int Type)
			{
				ID3D11ShaderResourceView* NewState = (Resource ? Resource->As<D3D11InstanceBuffer>()->Resource : nullptr);
				if (Type & ShaderType_Vertex)
					ImmediateContext->VSSetShaderResources(Slot, 1, &NewState);

				if (Type & ShaderType_Pixel)
					ImmediateContext->PSSetShaderResources(Slot, 1, &NewState);

				if (Type & ShaderType_Geometry)
					ImmediateContext->GSSetShaderResources(Slot, 1, &NewState);

				if (Type & ShaderType_Hull)
					ImmediateContext->HSSetShaderResources(Slot, 1, &NewState);

				if (Type & ShaderType_Domain)
					ImmediateContext->DSSetShaderResources(Slot, 1, &NewState);

				if (Type & ShaderType_Compute)
					ImmediateContext->CSSetShaderResources(Slot, 1, &NewState);
			}
			void D3D11Device::SetStructureBuffer(ElementBuffer* Resource, unsigned int Slot, unsigned int Type)
			{
				ID3D11ShaderResourceView* NewState = (Resource ? Resource->As<D3D11ElementBuffer>()->Resource : nullptr);
				if (Type & ShaderType_Vertex)
					ImmediateContext->VSSetShaderResources(Slot, 1, &NewState);

				if (Type & ShaderType_Pixel)
					ImmediateContext->PSSetShaderResources(Slot, 1, &NewState);

				if (Type & ShaderType_Geometry)
					ImmediateContext->GSSetShaderResources(Slot, 1, &NewState);

				if (Type & ShaderType_Hull)
					ImmediateContext->HSSetShaderResources(Slot, 1, &NewState);

				if (Type & ShaderType_Domain)
					ImmediateContext->DSSetShaderResources(Slot, 1, &NewState);

				if (Type & ShaderType_Compute)
					ImmediateContext->CSSetShaderResources(Slot, 1, &NewState);
			}
			void D3D11Device::SetTexture2D(Texture2D* Resource, unsigned int Slot, unsigned int Type)
			{
				ID3D11ShaderResourceView* NewState = (Resource ? Resource->As<D3D11Texture2D>()->Resource : nullptr);
				if (Type & ShaderType_Vertex)
					ImmediateContext->VSSetShaderResources(Slot, 1, &NewState);

				if (Type & ShaderType_Pixel)
					ImmediateContext->PSSetShaderResources(Slot, 1, &NewState);

				if (Type & ShaderType_Geometry)
					ImmediateContext->GSSetShaderResources(Slot, 1, &NewState);

				if (Type & ShaderType_Hull)
					ImmediateContext->HSSetShaderResources(Slot, 1, &NewState);

				if (Type & ShaderType_Domain)
					ImmediateContext->DSSetShaderResources(Slot, 1, &NewState);

				if (Type & ShaderType_Compute)
					ImmediateContext->CSSetShaderResources(Slot, 1, &NewState);
			}
			void D3D11Device::SetTexture3D(Texture3D* Resource, unsigned int Slot, unsigned int Type)
			{
				ID3D11ShaderResourceView* NewState = (Resource ? Resource->As<D3D11Texture3D>()->Resource : nullptr);
				if (Type & ShaderType_Vertex)
					ImmediateContext->VSSetShaderResources(Slot, 1, &NewState);

				if (Type & ShaderType_Pixel)
					ImmediateContext->PSSetShaderResources(Slot, 1, &NewState);

				if (Type & ShaderType_Geometry)
					ImmediateContext->GSSetShaderResources(Slot, 1, &NewState);

				if (Type & ShaderType_Hull)
					ImmediateContext->HSSetShaderResources(Slot, 1, &NewState);

				if (Type & ShaderType_Domain)
					ImmediateContext->DSSetShaderResources(Slot, 1, &NewState);

				if (Type & ShaderType_Compute)
					ImmediateContext->CSSetShaderResources(Slot, 1, &NewState);
			}
			void D3D11Device::SetTextureCube(TextureCube* Resource, unsigned int Slot, unsigned int Type)
			{
				ID3D11ShaderResourceView* NewState = (Resource ? Resource->As<D3D11TextureCube>()->Resource : nullptr);
				if (Type & ShaderType_Vertex)
					ImmediateContext->VSSetShaderResources(Slot, 1, &NewState);

				if (Type & ShaderType_Pixel)
					ImmediateContext->PSSetShaderResources(Slot, 1, &NewState);

				if (Type & ShaderType_Geometry)
					ImmediateContext->GSSetShaderResources(Slot, 1, &NewState);

				if (Type & ShaderType_Hull)
					ImmediateContext->HSSetShaderResources(Slot, 1, &NewState);

				if (Type & ShaderType_Domain)
					ImmediateContext->DSSetShaderResources(Slot, 1, &NewState);

				if (Type & ShaderType_Compute)
					ImmediateContext->CSSetShaderResources(Slot, 1, &NewState);
			}
			void D3D11Device::SetIndexBuffer(ElementBuffer* Resource, Format FormatMode)
			{
				ImmediateContext->IASetIndexBuffer(Resource ? Resource->As<D3D11ElementBuffer>()->Element : nullptr, (DXGI_FORMAT)FormatMode, 0);
			}
			void D3D11Device::SetVertexBuffer(ElementBuffer* Resource, unsigned int Slot)
			{
				D3D11ElementBuffer* IResource = (D3D11ElementBuffer*)Resource;
				ID3D11Buffer* IBuffer = (IResource ? IResource->Element : nullptr);
				unsigned int Stride = (IResource ? IResource->Stride : 0), Offset = 0;
				ImmediateContext->IASetVertexBuffers(Slot, 1, &IBuffer, &Stride, &Offset);
			}
			void D3D11Device::SetWriteable(ElementBuffer** Resource, unsigned int Count, unsigned int Slot, bool Computable)
			{
				if (!Resource || Count > 8)
					return;

				ID3D11UnorderedAccessView* Array[8] = { nullptr };
				for (unsigned int i = 0; i < Count; i++)
					Array[i] = (Resource[i] ? ((D3D11ElementBuffer*)(Resource[i]))->Access : nullptr);

				UINT Offset = 0;
				if (!Computable)
					ImmediateContext->OMSetRenderTargetsAndUnorderedAccessViews(D3D11_KEEP_RENDER_TARGETS_AND_DEPTH_STENCIL, nullptr, nullptr, Slot, Count, Array, &Offset);
				else
					ImmediateContext->CSSetUnorderedAccessViews(Slot, Count, Array, &Offset);
			}
			void D3D11Device::SetWriteable(Texture2D** Resource, unsigned int Count, unsigned int Slot, bool Computable)
			{
				if (!Resource || Count > 8)
					return;

				ID3D11UnorderedAccessView* Array[8] = { nullptr };
				for (unsigned int i = 0; i < Count; i++)
					Array[i] = (Resource[i] ? ((D3D11Texture2D*)(Resource[i]))->Access : nullptr);

				UINT Offset = 0;
				if (!Computable)
					ImmediateContext->OMSetRenderTargetsAndUnorderedAccessViews(D3D11_KEEP_RENDER_TARGETS_AND_DEPTH_STENCIL, nullptr, nullptr, Slot, Count, Array, &Offset);
				else
					ImmediateContext->CSSetUnorderedAccessViews(Slot, Count, Array, &Offset);
			}
			void D3D11Device::SetWriteable(Texture3D** Resource, unsigned int Count, unsigned int Slot, bool Computable)
			{
				if (!Resource || Count > 8)
					return;

				ID3D11UnorderedAccessView* Array[8] = { nullptr };
				for (unsigned int i = 0; i < Count; i++)
					Array[i] = (Resource[i] ? ((D3D11Texture3D*)(Resource[i]))->Access : nullptr);

				UINT Offset = 0;
				if (!Computable)
					ImmediateContext->OMSetRenderTargetsAndUnorderedAccessViews(D3D11_KEEP_RENDER_TARGETS_AND_DEPTH_STENCIL, nullptr, nullptr, Slot, Count, Array, &Offset);
				else
					ImmediateContext->CSSetUnorderedAccessViews(Slot, Count, Array, &Offset);
			}
			void D3D11Device::SetWriteable(TextureCube** Resource, unsigned int Count, unsigned int Slot, bool Computable)
			{
				if (!Resource || Count > 8)
					return;

				ID3D11UnorderedAccessView* Array[8] = { nullptr };
				for (unsigned int i = 0; i < Count; i++)
					Array[i] = (Resource[i] ? ((D3D11TextureCube*)(Resource[i]))->Access : nullptr);

				UINT Offset = 0;
				if (!Computable)
					ImmediateContext->OMSetRenderTargetsAndUnorderedAccessViews(D3D11_KEEP_RENDER_TARGETS_AND_DEPTH_STENCIL, nullptr, nullptr, Slot, Count, Array, &Offset);
				else
					ImmediateContext->CSSetUnorderedAccessViews(Slot, Count, Array, &Offset);
			}
			void D3D11Device::SetTarget(float R, float G, float B)
			{
				SetTarget(RenderTarget, 0, R, G, B);
			}
			void D3D11Device::SetTarget()
			{
				SetTarget(RenderTarget, 0);
			}
			void D3D11Device::SetTarget(DepthBuffer* Resource)
			{
				D3D11DepthBuffer* IResource = (D3D11DepthBuffer*)Resource;
				if (!IResource)
					return;

				const Viewport& Viewarea = Resource->GetViewport();
				D3D11_VIEWPORT Viewport = { Viewarea.TopLeftX, Viewarea.TopLeftY, Viewarea.Width, Viewarea.Height, Viewarea.MinDepth, Viewarea.MaxDepth };
				
				ImmediateContext->OMSetRenderTargets(0, nullptr, IResource->DepthStencilView);
				ImmediateContext->RSSetViewports(1, &Viewport);
			}
			void D3D11Device::SetTarget(Graphics::RenderTarget* Resource, unsigned int Target, float R, float G, float B)
			{
				if (!Resource || Target >= Resource->GetTargetCount())
					return;

				const Viewport& Viewarea = Resource->GetViewport();
				ID3D11RenderTargetView** TargetBuffer = (ID3D11RenderTargetView**)Resource->GetTargetBuffer();
				ID3D11DepthStencilView* DepthBuffer = (ID3D11DepthStencilView*)Resource->GetDepthBuffer();
				D3D11_VIEWPORT Viewport = { Viewarea.TopLeftX, Viewarea.TopLeftY, Viewarea.Width, Viewarea.Height, Viewarea.MinDepth, Viewarea.MaxDepth };
				float ClearColor[4] = { R, G, B, 0.0f };

				ImmediateContext->RSSetViewports(1, &Viewport);
				ImmediateContext->OMSetRenderTargets(1, &TargetBuffer[Target], DepthBuffer);
				ImmediateContext->ClearRenderTargetView(TargetBuffer[Target], ClearColor);
			}
			void D3D11Device::SetTarget(Graphics::RenderTarget* Resource, unsigned int Target)
			{
				if (!Resource || Target >= Resource->GetTargetCount())
					return;

				const Viewport& Viewarea = Resource->GetViewport();
				ID3D11RenderTargetView** TargetBuffer = (ID3D11RenderTargetView**)Resource->GetTargetBuffer();
				ID3D11DepthStencilView* DepthBuffer = (ID3D11DepthStencilView*)Resource->GetDepthBuffer();
				D3D11_VIEWPORT Viewport = { Viewarea.TopLeftX, Viewarea.TopLeftY, Viewarea.Width, Viewarea.Height, Viewarea.MinDepth, Viewarea.MaxDepth };

				ImmediateContext->RSSetViewports(1, &Viewport);
				ImmediateContext->OMSetRenderTargets(1, &TargetBuffer[Target], DepthBuffer);
			}
			void D3D11Device::SetTarget(Graphics::RenderTarget* Resource, float R, float G, float B)
			{
				if (!Resource)
					return;

				const Viewport& Viewarea = Resource->GetViewport();
				ID3D11RenderTargetView** TargetBuffer = (ID3D11RenderTargetView**)Resource->GetTargetBuffer();
				ID3D11DepthStencilView* DepthBuffer = (ID3D11DepthStencilView*)Resource->GetDepthBuffer();
				D3D11_VIEWPORT Viewport = { Viewarea.TopLeftX, Viewarea.TopLeftY, Viewarea.Width, Viewarea.Height, Viewarea.MinDepth, Viewarea.MaxDepth };
				uint32_t Count = Resource->GetTargetCount();
				float ClearColor[4] = { R, G, B, 0.0f };

				ImmediateContext->RSSetViewports(1, &Viewport);
				ImmediateContext->OMSetRenderTargets(Count, TargetBuffer, DepthBuffer);

				for (uint32_t i = 0; i < Count; i++)
					ImmediateContext->ClearRenderTargetView(TargetBuffer[i], ClearColor);
			}
			void D3D11Device::SetTarget(Graphics::RenderTarget* Resource)
			{
				if (!Resource)
					return;

				const Viewport& Viewarea = Resource->GetViewport();
				ID3D11RenderTargetView** TargetBuffer = (ID3D11RenderTargetView**)Resource->GetTargetBuffer();
				ID3D11DepthStencilView* DepthBuffer = (ID3D11DepthStencilView*)Resource->GetDepthBuffer();
				D3D11_VIEWPORT Viewport = { Viewarea.TopLeftX, Viewarea.TopLeftY, Viewarea.Width, Viewarea.Height, Viewarea.MinDepth, Viewarea.MaxDepth };

				ImmediateContext->RSSetViewports(1, &Viewport);
				ImmediateContext->OMSetRenderTargets(Resource->GetTargetCount(), TargetBuffer, DepthBuffer);
			}
			void D3D11Device::SetTargetMap(Graphics::RenderTarget* Resource, bool Enabled[8])
			{
				if (!Resource)
					return;

				const Viewport& Viewarea = Resource->GetViewport();
				ID3D11RenderTargetView** TargetBuffer = (ID3D11RenderTargetView**)Resource->GetTargetBuffer();
				ID3D11DepthStencilView* DepthBuffer = (ID3D11DepthStencilView*)Resource->GetDepthBuffer();
				D3D11_VIEWPORT Viewport = { Viewarea.TopLeftX, Viewarea.TopLeftY, Viewarea.Width, Viewarea.Height, Viewarea.MinDepth, Viewarea.MaxDepth };
				uint32_t Count = Resource->GetTargetCount();

				for (uint32_t i = 0; i < Count; i++)
					TargetBuffer[i] = (Enabled[i] ? TargetBuffer[i] : nullptr);

				ImmediateContext->RSSetViewports(1, &Viewport);
				ImmediateContext->OMSetRenderTargets(Count, TargetBuffer, DepthBuffer);
			}
			void D3D11Device::SetTargetRect(unsigned int Width, unsigned int Height)
			{
				if (!Width || !Height)
					return;

				D3D11_VIEWPORT Viewport = { 0, 0, (FLOAT)Width, (FLOAT)Height, 0, 1 };
				ImmediateContext->RSSetViewports(1, &Viewport);
				ImmediateContext->OMSetRenderTargets(0, nullptr, nullptr);
			}
			void D3D11Device::SetViewports(unsigned int Count, Viewport* Value)
			{
				if (!Count || !Value)
					return;

				D3D11_VIEWPORT Viewports[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
				for (unsigned int i = 0; i < Count; i++)
					memcpy(&Viewports[i], &Value[i], sizeof(Viewport));

				ImmediateContext->RSSetViewports(Count, Viewports);
			}
			void D3D11Device::SetScissorRects(unsigned int Count, Compute::Rectangle* Value)
			{
				if (!Count || !Value)
					return;

				D3D11_RECT Rects[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
				for (unsigned int i = 0; i < Count; i++)
					memcpy(&Rects[i], &Value[i], sizeof(Compute::Rectangle));

				ImmediateContext->RSSetScissorRects(Count, Rects);
			}
			void D3D11Device::SetPrimitiveTopology(PrimitiveTopology Topology)
			{
				ImmediateContext->IASetPrimitiveTopology((D3D11_PRIMITIVE_TOPOLOGY)Topology);
			}
			void D3D11Device::FlushTexture2D(unsigned int Slot, unsigned int Count, unsigned int Type)
			{
				static ID3D11ShaderResourceView* Array[32] = { nullptr };
				unsigned int Size = (Count > 32 ? 32 : Count);

				if (Type & ShaderType_Vertex)
					ImmediateContext->VSSetShaderResources(Slot, Size, Array);

				if (Type & ShaderType_Pixel)
					ImmediateContext->PSSetShaderResources(Slot, Size, Array);

				if (Type & ShaderType_Geometry)
					ImmediateContext->GSSetShaderResources(Slot, Size, Array);

				if (Type & ShaderType_Hull)
					ImmediateContext->HSSetShaderResources(Slot, Size, Array);

				if (Type & ShaderType_Domain)
					ImmediateContext->DSSetShaderResources(Slot, Size, Array);

				if (Type & ShaderType_Compute)
					ImmediateContext->CSSetShaderResources(Slot, Size, Array);
			}
			void D3D11Device::FlushTexture3D(unsigned int Slot, unsigned int Count, unsigned int Type)
			{
				FlushTexture2D(Slot, Count, Type);
			}
			void D3D11Device::FlushTextureCube(unsigned int Slot, unsigned int Count, unsigned int Type)
			{
				FlushTexture2D(Slot, Count, Type);
			}
			void D3D11Device::FlushState()
			{
				ImmediateContext->ClearState();
			}
			bool D3D11Device::Map(ElementBuffer* Resource, ResourceMap Mode, MappedSubresource* Map)
			{
				D3D11ElementBuffer* IResource = (D3D11ElementBuffer*)Resource;
				if (!IResource)
					return false;

				D3D11_MAPPED_SUBRESOURCE MappedResource;
				if (ImmediateContext->Map(IResource->Element, 0, (D3D11_MAP)Mode, 0, &MappedResource) != S_OK)
					return false;

				Map->Pointer = MappedResource.pData;
				Map->RowPitch = MappedResource.RowPitch;
				Map->DepthPitch = MappedResource.DepthPitch;
				return true;
			}
			bool D3D11Device::Unmap(ElementBuffer* Resource, MappedSubresource* Map)
			{
				D3D11ElementBuffer* IResource = (D3D11ElementBuffer*)Resource;
				if (!IResource)
					return false;

				ImmediateContext->Unmap(IResource->Element, 0);
				return true;
			}
			bool D3D11Device::UpdateBuffer(ElementBuffer* Resource, void* Data, uint64_t Size)
			{
				D3D11ElementBuffer* IResource = (D3D11ElementBuffer*)Resource;
				if (!IResource)
					return false;

				D3D11_MAPPED_SUBRESOURCE MappedResource;
				if (ImmediateContext->Map(IResource->Element, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource) != S_OK)
					return false;

				memcpy(MappedResource.pData, Data, (size_t)Size);
				ImmediateContext->Unmap(IResource->Element, 0);
				return true;
			}
			bool D3D11Device::UpdateBuffer(Shader* Resource, const void* Data)
			{
				D3D11Shader* IResource = (D3D11Shader*)Resource;
				if (!IResource)
					return false;

				ImmediateContext->UpdateSubresource(IResource->ConstantBuffer, 0, nullptr, Data, 0, 0);
				return true;
			}
			bool D3D11Device::UpdateBuffer(MeshBuffer* Resource, Compute::Vertex* Data)
			{
				D3D11MeshBuffer* IResource = (D3D11MeshBuffer*)Resource;
				if (!IResource)
					return false;

				MappedSubresource MappedResource;
				if (!Map(IResource->VertexBuffer, ResourceMap_Write, &MappedResource))
					return false;

				memcpy(MappedResource.Pointer, Data, (size_t)IResource->VertexBuffer->GetElements() * sizeof(Compute::Vertex));
				return Unmap(IResource->VertexBuffer, &MappedResource);
			}
			bool D3D11Device::UpdateBuffer(SkinMeshBuffer* Resource, Compute::SkinVertex* Data)
			{
				D3D11SkinMeshBuffer* IResource = (D3D11SkinMeshBuffer*)Resource;
				if (!IResource)
					return false;

				MappedSubresource MappedResource;
				if (!Map(IResource->VertexBuffer, ResourceMap_Write, &MappedResource))
					return false;

				memcpy(MappedResource.Pointer, Data, (size_t)IResource->VertexBuffer->GetElements() * sizeof(Compute::SkinVertex));
				return Unmap(IResource->VertexBuffer, &MappedResource);
			}
			bool D3D11Device::UpdateBuffer(InstanceBuffer* Resource)
			{
				D3D11InstanceBuffer* IResource = (D3D11InstanceBuffer*)Resource;
				if (!IResource || IResource->Array.Size() <= 0 || IResource->Array.Size() > IResource->ElementLimit)
					return false;

				D3D11ElementBuffer* Element = IResource->Elements->As<D3D11ElementBuffer>();
				IResource->Sync = true;

				D3D11_MAPPED_SUBRESOURCE MappedResource;
				if (ImmediateContext->Map(Element->Element, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource) != S_OK)
					return false;

				memcpy(MappedResource.pData, IResource->Array.Get(), (size_t)IResource->Array.Size() * IResource->ElementWidth);
				ImmediateContext->Unmap(Element->Element, 0);
				return true;
			}
			bool D3D11Device::UpdateBuffer(RenderBufferType fBuffer)
			{
				ImmediateContext->UpdateSubresource(ConstantBuffer[fBuffer], 0, nullptr, Constants[fBuffer], 0, 0);
				return true;
			}
			bool D3D11Device::UpdateBufferSize(Shader* Resource, size_t Size)
			{
				D3D11Shader* IResource = (D3D11Shader*)Resource;
				if (!IResource || !Size)
					return false;

				ReleaseCom(IResource->ConstantBuffer);
				return CreateConstantBuffer(&IResource->ConstantBuffer, Size) == S_OK;
			}
			bool D3D11Device::UpdateBufferSize(InstanceBuffer* Resource, uint64_t Size)
			{
				D3D11InstanceBuffer* IResource = (D3D11InstanceBuffer*)Resource;
				if (!IResource)
					return false;

				ClearBuffer(IResource);
				TH_RELEASE(IResource->Elements);

				ReleaseCom(IResource->Resource);
				IResource->ElementLimit = Size;
				if (IResource->ElementLimit < 1)
					IResource->ElementLimit = 1;

				IResource->Array.Clear();
				IResource->Array.Reserve(IResource->ElementLimit);

				ElementBuffer::Desc F = ElementBuffer::Desc();
				F.AccessFlags = CPUAccess_Write;
				F.MiscFlags = ResourceMisc_Buffer_Structured;
				F.Usage = ResourceUsage_Dynamic;
				F.BindFlags = ResourceBind_Shader_Input;
				F.ElementCount = (unsigned int)IResource->ElementLimit;
				F.ElementWidth = (unsigned int)IResource->ElementWidth;
				F.StructureByteStride = F.ElementWidth;

				IResource->Elements = CreateElementBuffer(F);

				D3D11_SHADER_RESOURCE_VIEW_DESC SRV;
				ZeroMemory(&SRV, sizeof(SRV));
				SRV.Format = DXGI_FORMAT_UNKNOWN;
				SRV.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
				SRV.Buffer.ElementWidth = (unsigned int)IResource->ElementLimit;

				return D3DDevice->CreateShaderResourceView(IResource->Elements->As<D3D11ElementBuffer>()->Element, &SRV, &IResource->Resource) == S_OK;
			}
			void D3D11Device::ClearBuffer(InstanceBuffer* Resource)
			{
				D3D11InstanceBuffer* IResource = (D3D11InstanceBuffer*)Resource;
				if (!IResource || !IResource->Sync)
					return;

				D3D11ElementBuffer* Element = IResource->Elements->As<D3D11ElementBuffer>();
				IResource->Sync = false;

				D3D11_MAPPED_SUBRESOURCE MappedResource;
				ImmediateContext->Map(Element->Element, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
				MappedResource.pData = nullptr;
				ImmediateContext->Unmap(Element->Element, 0);
			}
			void D3D11Device::ClearWritable(Texture2D* Resource)
			{
				D3D11Texture2D* IResource = (D3D11Texture2D*)Resource;
				if (!IResource || !IResource->Access)
					return;

				UINT ClearColor[4] = { 0, 0, 0, 0 };
				ImmediateContext->ClearUnorderedAccessViewUint(IResource->Access, ClearColor);
			}
			void D3D11Device::ClearWritable(Texture2D* Resource, float R, float G, float B)
			{
				D3D11Texture2D* IResource = (D3D11Texture2D*)Resource;
				if (!IResource || !IResource->Access)
					return;

				float ClearColor[4] = { R, G, B, 0.0f };
				ImmediateContext->ClearUnorderedAccessViewFloat(IResource->Access, ClearColor);
			}
			void D3D11Device::ClearWritable(Texture3D* Resource)
			{
				D3D11Texture3D* IResource = (D3D11Texture3D*)Resource;
				if (!IResource || !IResource->Access)
					return;

				UINT ClearColor[4] = { 0, 0, 0, 0 };
				ImmediateContext->ClearUnorderedAccessViewUint(IResource->Access, ClearColor);
			}
			void D3D11Device::ClearWritable(Texture3D* Resource, float R, float G, float B)
			{
				D3D11Texture3D* IResource = (D3D11Texture3D*)Resource;
				if (!IResource || !IResource->Access)
					return;

				float ClearColor[4] = { R, G, B, 0.0f };
				ImmediateContext->ClearUnorderedAccessViewFloat(IResource->Access, ClearColor);
			}
			void D3D11Device::ClearWritable(TextureCube* Resource)
			{
				D3D11TextureCube* IResource = (D3D11TextureCube*)Resource;
				if (!IResource || !IResource->Access)
					return;

				UINT ClearColor[4] = { 0, 0, 0, 0 };
				ImmediateContext->ClearUnorderedAccessViewUint(IResource->Access, ClearColor);
			}
			void D3D11Device::ClearWritable(TextureCube* Resource, float R, float G, float B)
			{
				D3D11TextureCube* IResource = (D3D11TextureCube*)Resource;
				if (!IResource || !IResource->Access)
					return;

				float ClearColor[4] = { R, G, B, 0.0f };
				ImmediateContext->ClearUnorderedAccessViewFloat(IResource->Access, ClearColor);
			}
			void D3D11Device::Clear(float R, float G, float B)
			{
				Clear(RenderTarget, 0, R, G, B);
			}
			void D3D11Device::Clear(Graphics::RenderTarget* Resource, unsigned int Target, float R, float G, float B)
			{
				if (!Resource || Target >= Resource->GetTargetCount())
					return;

				ID3D11RenderTargetView** TargetBuffer = (ID3D11RenderTargetView**)Resource->GetTargetBuffer();
				float ClearColor[4] = { R, G, B, 0.0f };

				ImmediateContext->ClearRenderTargetView(TargetBuffer[Target], ClearColor);
			}
			void D3D11Device::ClearDepth()
			{
				ClearDepth(RenderTarget);
			}
			void D3D11Device::ClearDepth(DepthBuffer* Resource)
			{
				D3D11DepthBuffer* IResource = (D3D11DepthBuffer*)Resource;
				if (!IResource)
					return;

				ImmediateContext->ClearDepthStencilView(IResource->DepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
			}
			void D3D11Device::ClearDepth(Graphics::RenderTarget* Resource)
			{
				if (!Resource)
					return;

				ID3D11DepthStencilView* DepthBuffer = (ID3D11DepthStencilView*)Resource->GetDepthBuffer();
				ImmediateContext->ClearDepthStencilView(DepthBuffer, D3D11_CLEAR_DEPTH, 1.0f, 0);
			}
			void D3D11Device::DrawIndexed(unsigned int Count, unsigned int IndexLocation, unsigned int BaseLocation)
			{
				ImmediateContext->DrawIndexed(Count, IndexLocation, BaseLocation);
			}
			void D3D11Device::DrawIndexed(MeshBuffer* Resource)
			{
				if (!Resource)
					return;

				D3D11ElementBuffer* VertexBuffer = (D3D11ElementBuffer*)Resource->GetVertexBuffer();
				D3D11ElementBuffer* IndexBuffer = (D3D11ElementBuffer*)Resource->GetIndexBuffer();
				unsigned int Stride = VertexBuffer->Stride, Offset = 0;

				ImmediateContext->IASetVertexBuffers(0, 1, &VertexBuffer->Element, &Stride, &Offset);
				ImmediateContext->IASetIndexBuffer(IndexBuffer->Element, DXGI_FORMAT_R32_UINT, 0);
				ImmediateContext->DrawIndexed((unsigned int)IndexBuffer->GetElements(), 0, 0);
			}
			void D3D11Device::DrawIndexed(SkinMeshBuffer* Resource)
			{
				if (!Resource)
					return;

				D3D11ElementBuffer* VertexBuffer = (D3D11ElementBuffer*)Resource->GetVertexBuffer();
				D3D11ElementBuffer* IndexBuffer = (D3D11ElementBuffer*)Resource->GetIndexBuffer();
				unsigned int Stride = VertexBuffer->Stride, Offset = 0;

				ImmediateContext->IASetVertexBuffers(0, 1, &VertexBuffer->Element, &Stride, &Offset);
				ImmediateContext->IASetIndexBuffer(IndexBuffer->Element, DXGI_FORMAT_R32_UINT, 0);
				ImmediateContext->DrawIndexed((unsigned int)IndexBuffer->GetElements(), 0, 0);
			}
			void D3D11Device::Draw(unsigned int Count, unsigned int Location)
			{
				ImmediateContext->Draw(Count, Location);
			}
			void D3D11Device::Dispatch(unsigned int GroupX, unsigned int GroupY, unsigned int GroupZ)
			{
				ImmediateContext->Dispatch(GroupX, GroupY, GroupZ);
			}
			bool D3D11Device::CopyTexture2D(Texture2D* Resource, Texture2D** Result)
			{
				D3D11Texture2D* IResource = (D3D11Texture2D*)Resource;
				if (!IResource || !IResource->View || !Result)
					return false;

				D3D11_TEXTURE2D_DESC Information;
				IResource->View->GetDesc(&Information);
				Information.BindFlags |= D3D11_BIND_SHADER_RESOURCE;

				D3D11Texture2D* Texture = (D3D11Texture2D*)(*Result ? *Result : CreateTexture2D());
				if (!*Result)
				{
					if (D3DDevice->CreateTexture2D(&Information, nullptr, &Texture->View) != S_OK)
						return false;
				}

				ImmediateContext->CopyResource(Texture->View, IResource->View);
				*Result = Texture;

				return GenerateTexture(Texture);
			}
			bool D3D11Device::CopyTexture2D(Graphics::RenderTarget* Resource, unsigned int Target, Texture2D** Result)
			{
				if (!Resource || !Result)
					return false;

				D3D11Texture2D* Source = (D3D11Texture2D*)Resource->GetTarget(Target);
				if (!Source || !Source->View)
					return false;

				D3D11_TEXTURE2D_DESC Information;
				Source->View->GetDesc(&Information);
				Information.BindFlags |= D3D11_BIND_SHADER_RESOURCE;

				D3D11Texture2D* Texture = (D3D11Texture2D*)(*Result ? *Result : CreateTexture2D());
				if (!*Result)
				{
					if (D3DDevice->CreateTexture2D(&Information, nullptr, &Texture->View) != S_OK)
						return false;
				}

				ImmediateContext->CopyResource(Texture->View, Source->View);
				*Result = Texture;

				return GenerateTexture(Texture);
			}
			bool D3D11Device::CopyTexture2D(RenderTargetCube* Resource, unsigned int Face, Texture2D** Result)
			{
				D3D11RenderTargetCube* IResource = (D3D11RenderTargetCube*)Resource;
				if (!IResource || Face >= 6 || !IResource->Texture || !Result)
					return false;

				D3D11_TEXTURE2D_DESC Information;
				IResource->Texture->GetDesc(&Information);
				Information.BindFlags |= D3D11_BIND_SHADER_RESOURCE;

				D3D11Texture2D* Texture = (D3D11Texture2D*)(*Result ? *Result : CreateTexture2D());
				if (!*Result)
				{
					if (D3DDevice->CreateTexture2D(&Information, nullptr, &Texture->View) != S_OK)
						return false;
				}

				ImmediateContext->CopySubresourceRegion(Texture->View, Face * Information.MipLevels, 0, 0, 0, IResource->Texture, 0, 0);
				*Result = Texture;

				return GenerateTexture(Texture);
			}
			bool D3D11Device::CopyTexture2D(MultiRenderTargetCube* Resource, unsigned int Cube, unsigned int Face, Texture2D** Result)
			{
				D3D11MultiRenderTargetCube* IResource = (D3D11MultiRenderTargetCube*)Resource;
				if (!IResource || Cube >= IResource->Target || Face >= 6 || !IResource->Texture[Cube] || !Result)
					return false;

				D3D11_TEXTURE2D_DESC Information;
				IResource->Texture[Cube]->GetDesc(&Information);
				Information.BindFlags |= D3D11_BIND_SHADER_RESOURCE;

				D3D11Texture2D* Texture = (D3D11Texture2D*)(*Result ? *Result : CreateTexture2D());
				if (!*Result)
				{
					if (D3DDevice->CreateTexture2D(&Information, nullptr, &Texture->View) != S_OK)
						return false;
				}

				ImmediateContext->CopySubresourceRegion(Texture->View, Face * Information.MipLevels, 0, 0, 0, IResource->Texture[Cube], 0, 0);
				*Result = Texture;

				return GenerateTexture(Texture);
			}
			bool D3D11Device::CopyTextureCube(RenderTargetCube* Resource, TextureCube** Result)
			{
				D3D11RenderTargetCube* IResource = (D3D11RenderTargetCube*)Resource;
				if (!IResource || !IResource->Texture || !Result)
					return false;

				D3D11_TEXTURE2D_DESC Information;
				IResource->Texture->GetDesc(&Information);
				Information.BindFlags |= D3D11_BIND_SHADER_RESOURCE;

				void* Resources[6] = { nullptr };
				for (unsigned int i = 0; i < 6; i++)
				{
					ID3D11Texture2D* Subresource;
					if (D3DDevice->CreateTexture2D(&Information, nullptr, &Subresource) != S_OK)
					{
						for (unsigned int j = 0; j < 6; j++)
						{
							ID3D11Texture2D* Src = (ID3D11Texture2D*)Resources[j];
							ReleaseCom(Src);
						}

						return false;
					}

					ImmediateContext->CopySubresourceRegion(Subresource, i, 0, 0, 0, IResource->Texture, 0, 0);
					Resources[i] = (bool*)Subresource;
				}

				*Result = CreateTextureCubeInternal(Resources);
				return false;
			}
			bool D3D11Device::CopyTextureCube(MultiRenderTargetCube* Resource, unsigned int Cube, TextureCube** Result)
			{
				D3D11MultiRenderTargetCube* IResource = (D3D11MultiRenderTargetCube*)Resource;
				if (!IResource || Cube >= IResource->Target || !IResource->Texture[Cube] || !Result)
					return false;

				D3D11_TEXTURE2D_DESC Information;
				IResource->Texture[Cube]->GetDesc(&Information);
				Information.BindFlags |= D3D11_BIND_SHADER_RESOURCE;

				void* Resources[6] = { nullptr };
				for (unsigned int i = 0; i < 6; i++)
				{
					ID3D11Texture2D* Subresource;
					if (D3DDevice->CreateTexture2D(&Information, nullptr, &Subresource) != S_OK)
					{
						for (unsigned int j = 0; j < 6; j++)
						{
							ID3D11Texture2D* Src = (ID3D11Texture2D*)Resources[j];
							ReleaseCom(Src);
						}

						return false;
					}

					ImmediateContext->CopySubresourceRegion(Subresource, i, 0, 0, 0, IResource->Texture[Cube], 0, 0);
					Resources[i] = (bool*)Subresource;
				}

				*Result = CreateTextureCubeInternal(Resources);
				return true;
			}
			bool D3D11Device::CopyTarget(Graphics::RenderTarget* From, unsigned int FromTarget, Graphics::RenderTarget* To, unsigned ToTarget)
			{
				if (!From || !To)
					return false;

				D3D11Texture2D* Source = (D3D11Texture2D*)From->GetTarget(FromTarget);
				D3D11Texture2D* Dest = (D3D11Texture2D*)To->GetTarget(ToTarget);
				if (!Source || !Source->View || !Dest || !Dest->View)
					return false;

				ImmediateContext->CopyResource(Dest->View, Source->View);
				return true;
			}
			bool D3D11Device::CopyBackBuffer(Texture2D** Result)
			{
				ID3D11Texture2D* BackBuffer = nullptr;
				SwapChain->GetBuffer(0, __uuidof(BackBuffer), reinterpret_cast<void**>(&BackBuffer));

				D3D11_TEXTURE2D_DESC Information;
				BackBuffer->GetDesc(&Information);
				Information.BindFlags = D3D11_BIND_SHADER_RESOURCE;

				D3D11Texture2D* Texture = (D3D11Texture2D*)(*Result ? *Result : CreateTexture2D());
				if (!*Result)
				{
					if (SwapChainResource.SampleDesc.Count > 1)
					{
						Information.SampleDesc.Count = 1;
						Information.SampleDesc.Quality = 0;
					}

					if (D3DDevice->CreateTexture2D(&Information, nullptr, &Texture->View) != S_OK)
						return false;
				}

				if (SwapChainResource.SampleDesc.Count > 1)
					ImmediateContext->ResolveSubresource(Texture->View, 0, BackBuffer, 0, DXGI_FORMAT_R8G8B8A8_UNORM);
				else
					ImmediateContext->CopyResource(Texture->View, BackBuffer);

				ReleaseCom(BackBuffer);
				return GenerateTexture(Texture);
			}
			bool D3D11Device::CopyBackBufferMSAA(Texture2D** Result)
			{
				ID3D11Texture2D* BackBuffer = nullptr;
				SwapChain->GetBuffer(0, __uuidof(BackBuffer), reinterpret_cast<void**>(&BackBuffer));

				D3D11_TEXTURE2D_DESC Texture;
				BackBuffer->GetDesc(&Texture);
				Texture.BindFlags = D3D11_BIND_SHADER_RESOURCE;
				Texture.SampleDesc.Count = 1;
				Texture.SampleDesc.Quality = 0;

				ID3D11Texture2D* Resource = nullptr;
				D3DDevice->CreateTexture2D(&Texture, nullptr, &Resource);
				ImmediateContext->ResolveSubresource(Resource, 0, BackBuffer, 0, DXGI_FORMAT_R8G8B8A8_UNORM);

				D3D11_SHADER_RESOURCE_VIEW_DESC SRV;
				SRV.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
				SRV.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
				SRV.Texture2D.MostDetailedMip = 0;
				SRV.Texture2D.MipLevels = 1;

				ID3D11ShaderResourceView* ResourceView = nullptr;
				D3DDevice->CreateShaderResourceView(Resource, &SRV, &ResourceView);
				ReleaseCom(BackBuffer);
				ReleaseCom(Resource);

				return (void*)Resource;
			}
			bool D3D11Device::CopyBackBufferNoAA(Texture2D** Result)
			{
				ID3D11Texture2D* BackBuffer = nullptr;
				SwapChain->GetBuffer(0, __uuidof(BackBuffer), reinterpret_cast<void**>(&BackBuffer));

				D3D11_TEXTURE2D_DESC Texture;
				BackBuffer->GetDesc(&Texture);
				Texture.BindFlags = D3D11_BIND_SHADER_RESOURCE;

				ID3D11Texture2D* Resource = nullptr;
				D3DDevice->CreateTexture2D(&Texture, nullptr, &Resource);
				ImmediateContext->CopyResource(Resource, BackBuffer);

				D3D11_SHADER_RESOURCE_VIEW_DESC SRV;
				SRV.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
				SRV.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
				SRV.Texture2D.MostDetailedMip = 0;
				SRV.Texture2D.MipLevels = 1;

				ID3D11ShaderResourceView* ResourceView = nullptr;
				D3DDevice->CreateShaderResourceView(Resource, &SRV, &ResourceView);
				ReleaseCom(BackBuffer);
				ReleaseCom(Resource);

				return (void*)ResourceView;
			}
			bool D3D11Device::CubemapBegin(Cubemap* Resource)
			{
				D3D11Cubemap* IResource = (D3D11Cubemap*)Resource;
				if (!IResource || !IResource->IsValid())
					return false;

				ReleaseCom(IResource->Subresource);
				ReleaseCom(IResource->Face);

				if (D3DDevice->CreateTexture2D(&IResource->Texture, nullptr, &IResource->Subresource) != S_OK)
					return false;

				return D3DDevice->CreateTexture2D(&IResource->Cube, nullptr, &IResource->Face) == S_OK;
			}
			bool D3D11Device::CubemapFace(Cubemap* Resource, unsigned int Target, unsigned int Face)
			{
				D3D11Cubemap* IResource = (D3D11Cubemap*)Resource;
				if (!IResource || !IResource->IsValid() || Face >= 6)
					return false;

				D3D11Texture2D* Source = (D3D11Texture2D*)IResource->Meta.Source->GetTarget(Target);
				ImmediateContext->CopyResource(IResource->Subresource, Source->View);
				ImmediateContext->CopySubresourceRegion(IResource->Face, Face * IResource->Cube.MipLevels, 0, 0, 0, IResource->Subresource, 0, &IResource->Region);
				return true;
			}
			bool D3D11Device::CubemapEnd(Cubemap* Resource, TextureCube* Result)
			{
				D3D11Cubemap* IResource = (D3D11Cubemap*)Resource;
				if (!IResource || !IResource->IsValid() || !Result)
					return false;

				ID3D11ShaderResourceView** Subresource = &Result->As<D3D11TextureCube>()->Resource;
				ReleaseCom((*Subresource));

				if (D3DDevice->CreateShaderResourceView(IResource->Face, &IResource->Resource, Subresource) != S_OK)
					return false;

				ImmediateContext->GenerateMips(*Subresource);
				ReleaseCom(IResource->Subresource);
				ReleaseCom(IResource->Face);
				return true;
			}
			void D3D11Device::GetViewports(unsigned int* Count, Viewport* Out)
			{
				D3D11_VIEWPORT Viewports[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
				UINT ViewCount = (Count ? *Count : 1);

				ImmediateContext->RSGetViewports(&ViewCount, Viewports);
				if (!ViewCount || !Out)
					return;

				for (UINT i = 0; i < ViewCount; i++)
					memcpy(&Out[i], &Viewports[i], sizeof(D3D11_VIEWPORT));
			}
			void D3D11Device::GetScissorRects(unsigned int* Count, Compute::Rectangle* Out)
			{
				D3D11_RECT Rects[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
				UINT RectCount = (Count ? *Count : 1);

				ImmediateContext->RSGetScissorRects(&RectCount, Rects);
				if (!Out)
					return;

				if (Count != nullptr)
					*Count = RectCount;

				for (UINT i = 0; i < RectCount; i++)
					memcpy(&Out[i], &Rects[i], sizeof(D3D11_RECT));
			}
			bool D3D11Device::ResizeBuffers(unsigned int Width, unsigned int Height)
			{
				if (RenderTarget != nullptr)
				{
					ImmediateContext->OMSetRenderTargets(0, 0, 0);
					TH_CLEAR(RenderTarget);

					DXGI_SWAP_CHAIN_DESC Info;
					if (SwapChain->GetDesc(&Info) != S_OK)
						return false;

					if (SwapChain->ResizeBuffers(2, Width, Height, Info.BufferDesc.Format, Info.Flags) != S_OK)
						return false;
				}

				ID3D11Texture2D* BackBuffer = nullptr;
				if (SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&BackBuffer) != S_OK)
					return false;

				RenderTarget2D::Desc F = RenderTarget2D::Desc();
				F.Width = Width;
				F.Height = Height;
				F.MipLevels = 1;
				F.MiscFlags = ResourceMisc_None;
				F.FormatMode = Format_R8G8B8A8_Unorm;
				F.Usage = ResourceUsage_Default;
				F.AccessFlags = CPUAccess_Invalid;
				F.BindFlags = ResourceBind_Render_Target | ResourceBind_Shader_Input;
				F.RenderSurface = (void*)BackBuffer;

				RenderTarget = CreateRenderTarget2D(F);
				SetTarget(RenderTarget);
				ReleaseCom(BackBuffer);

				return true;
			}
			bool D3D11Device::GenerateTexture(Texture2D* Resource)
			{
				return CreateTexture2D(Resource, DXGI_FORMAT_UNKNOWN);
			}
			bool D3D11Device::GenerateTexture(Texture3D* Resource)
			{
				D3D11Texture3D* IResource = (D3D11Texture3D*)Resource;
				if (!IResource || !IResource->View)
					return false;

				D3D11_TEXTURE3D_DESC Description;
				IResource->View->GetDesc(&Description);
				IResource->FormatMode = (Format)Description.Format;
				IResource->Usage = (ResourceUsage)Description.Usage;
				IResource->Width = Description.Width;
				IResource->Height = Description.Height;
				IResource->MipLevels = Description.MipLevels;
				IResource->AccessFlags = (CPUAccess)Description.CPUAccessFlags;

				D3D11_SHADER_RESOURCE_VIEW_DESC SRV;
				ZeroMemory(&SRV, sizeof(SRV));
				SRV.Format = Description.Format;
				SRV.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
				SRV.Texture3D.MostDetailedMip = 0;
				SRV.Texture3D.MipLevels = Description.MipLevels;

				ReleaseCom(IResource->Resource);
				if (D3DDevice->CreateShaderResourceView(IResource->View, &SRV, &IResource->Resource) == S_OK)
					return true;

				TH_ERROR("could not generate texture 3d resource");
				return false;
			}
			bool D3D11Device::GenerateTexture(TextureCube* Resource)
			{
				D3D11TextureCube* IResource = (D3D11TextureCube*)Resource;
				if (!IResource || !IResource->View)
					return false;

				D3D11_TEXTURE2D_DESC Description;
				IResource->View->GetDesc(&Description);
				IResource->FormatMode = (Format)Description.Format;
				IResource->Usage = (ResourceUsage)Description.Usage;
				IResource->Width = Description.Width;
				IResource->Height = Description.Height;
				IResource->MipLevels = Description.MipLevels;
				IResource->AccessFlags = (CPUAccess)Description.CPUAccessFlags;

				D3D11_SHADER_RESOURCE_VIEW_DESC SRV;
				ZeroMemory(&SRV, sizeof(SRV));
				SRV.Format = Description.Format;
				SRV.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
				SRV.TextureCube.MostDetailedMip = 0;
				SRV.TextureCube.MipLevels = Description.MipLevels;

				ReleaseCom(IResource->Resource);
				if (D3DDevice->CreateShaderResourceView(IResource->View, &SRV, &IResource->Resource) == S_OK)
					return true;

				TH_ERROR("could not generate texture cube resource");
				return false;
			}
			bool D3D11Device::GetQueryData(Query* Resource, uint64_t* Result, bool Flush)
			{
				D3D11Query* IResource = (D3D11Query*)Resource;
				if (!Result || !IResource || !IResource->Async)
					return false;

				return ImmediateContext->GetData(IResource->Async, Result, sizeof(uint64_t), !Flush ? D3D11_ASYNC_GETDATA_DONOTFLUSH : 0) == S_OK;
			}
			bool D3D11Device::GetQueryData(Query* Resource, bool* Result, bool Flush)
			{
				D3D11Query* IResource = (D3D11Query*)Resource;
				if (!Result || !IResource || !IResource->Async)
					return false;

				return ImmediateContext->GetData(IResource->Async, Result, sizeof(bool), !Flush ? D3D11_ASYNC_GETDATA_DONOTFLUSH : 0) == S_OK;
			}
			void D3D11Device::QueryBegin(Query* Resource)
			{
				D3D11Query* IResource = (D3D11Query*)Resource;
				if (!IResource || !IResource->Async)
					return;

				ImmediateContext->Begin(IResource->Async);
			}
			void D3D11Device::QueryEnd(Query* Resource)
			{
				D3D11Query* IResource = (D3D11Query*)Resource;
				if (!IResource || !IResource->Async)
					return;

				ImmediateContext->End(IResource->Async);
			}
			void D3D11Device::GenerateMips(Texture2D* Resource)
			{
				D3D11Texture2D* IResource = (D3D11Texture2D*)Resource;
				if (!IResource || !IResource->Resource)
					return;

				ImmediateContext->GenerateMips(IResource->Resource);
			}
			void D3D11Device::GenerateMips(Texture3D* Resource)
			{
				D3D11Texture3D* IResource = (D3D11Texture3D*)Resource;
				if (!IResource || !IResource->Resource)
					return;

				ImmediateContext->GenerateMips(IResource->Resource);
			}
			void D3D11Device::GenerateMips(TextureCube* Resource)
			{
				D3D11TextureCube* IResource = (D3D11TextureCube*)Resource;
				if (!IResource || !IResource->Resource)
					return;

				ImmediateContext->GenerateMips(IResource->Resource);
			}
			bool D3D11Device::Begin()
			{
				if (!DirectRenderer.VertexBuffer && !CreateDirectBuffer(0))
					return false;

				Primitives = PrimitiveTopology_Triangle_List;
				Direct.WorldViewProjection = Compute::Matrix4x4::Identity();
				Direct.Padding = { 0, 0, 0, 1 };
				ViewResource = nullptr;

				Elements.clear();
				return true;
			}
			void D3D11Device::Transform(const Compute::Matrix4x4& Transform)
			{
				Direct.WorldViewProjection = Direct.WorldViewProjection * Transform;
			}
			void D3D11Device::Topology(PrimitiveTopology Topology)
			{
				Primitives = Topology;
			}
			void D3D11Device::Emit()
			{
				Elements.push_back({ 0, 0, 0, 0, 0, 1, 1, 1, 1 });
			}
			void D3D11Device::Texture(Texture2D* In)
			{
				ViewResource = In;
				Direct.Padding.Z = 1;
			}
			void D3D11Device::Color(float X, float Y, float Z, float W)
			{
				if (Elements.empty())
					return;

				auto& Element = Elements.back();
				Element.CX = X;
				Element.CY = Y;
				Element.CZ = Z;
				Element.CW = W;
			}
			void D3D11Device::Intensity(float Intensity)
			{
				Direct.Padding.W = Intensity;
			}
			void D3D11Device::TexCoord(float X, float Y)
			{
				if (Elements.empty())
					return;

				auto& Element = Elements.back();
				Element.TX = X;
				Element.TY = Y;
			}
			void D3D11Device::TexCoordOffset(float X, float Y)
			{
				Direct.Padding.X = X;
				Direct.Padding.Y = Y;
			}
			void D3D11Device::Position(float X, float Y, float Z)
			{
				if (Elements.empty())
					return;

				auto& Element = Elements.back();
				Element.PX = X;
				Element.PY = Y;
				Element.PZ = Z;
			}
			bool D3D11Device::End()
			{
				if (!DirectRenderer.VertexBuffer || Elements.empty())
					return false;

				if (Elements.size() > MaxElements && !CreateDirectBuffer(Elements.size()))
					return false;

				D3D11_PRIMITIVE_TOPOLOGY LastTopology;
				ImmediateContext->IAGetPrimitiveTopology(&LastTopology);
				ImmediateContext->IASetPrimitiveTopology((D3D11_PRIMITIVE_TOPOLOGY)Primitives);

				ID3D11InputLayout* LastLayout;
				ImmediateContext->IAGetInputLayout(&LastLayout);
				ImmediateContext->IASetInputLayout(DirectRenderer.VertexLayout);

				ID3D11VertexShader* LastVertexShader;
				ImmediateContext->VSGetShader(&LastVertexShader, nullptr, nullptr);
				ImmediateContext->VSSetShader(DirectRenderer.VertexShader, nullptr, 0);

				ID3D11PixelShader* LastPixelShader;
				ImmediateContext->PSGetShader(&LastPixelShader, nullptr, nullptr);
				ImmediateContext->PSSetShader(DirectRenderer.PixelShader, nullptr, 0);

				ID3D11Buffer* LastBuffer1, *LastBuffer2;
				ImmediateContext->VSGetConstantBuffers(0, 1, &LastBuffer1);
				ImmediateContext->VSSetConstantBuffers(0, 1, &DirectRenderer.ConstantBuffer);
				ImmediateContext->PSGetConstantBuffers(0, 1, &LastBuffer2);
				ImmediateContext->PSSetConstantBuffers(0, 1, &DirectRenderer.ConstantBuffer);

				ID3D11ShaderResourceView* LastTexture, *NullTexture = nullptr;
				ImmediateContext->PSGetShaderResources(0, 1, &LastTexture);
				ImmediateContext->PSSetShaderResources(0, 1, ViewResource ? &ViewResource->As<D3D11Texture2D>()->Resource : &NullTexture);

				D3D11_MAPPED_SUBRESOURCE MappedResource;
				ImmediateContext->Map(DirectRenderer.VertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
				memcpy(MappedResource.pData, Elements.data(), (size_t)Elements.size() * sizeof(Vertex));
				ImmediateContext->Unmap(DirectRenderer.VertexBuffer, 0);

				unsigned int Stride = sizeof(Vertex), Offset = 0;
				ImmediateContext->IASetVertexBuffers(0, 1, &DirectRenderer.VertexBuffer, &Stride, &Offset);
				ImmediateContext->UpdateSubresource(DirectRenderer.ConstantBuffer, 0, nullptr, &Direct, 0, 0);
				ImmediateContext->Draw((unsigned int)Elements.size(), 0);
				ImmediateContext->IASetPrimitiveTopology(LastTopology);
				ImmediateContext->IASetInputLayout(LastLayout);
				ImmediateContext->VSSetShader(LastVertexShader, nullptr, 0);
				ImmediateContext->VSSetConstantBuffers(0, 1, &LastBuffer1);
				ImmediateContext->PSSetShader(LastPixelShader, nullptr, 0);
				ImmediateContext->PSSetConstantBuffers(0, 1, &LastBuffer2);
				ImmediateContext->PSSetShaderResources(0, 1, &LastTexture);
				ReleaseCom(LastLayout);
				ReleaseCom(LastVertexShader);
				ReleaseCom(LastPixelShader);
				ReleaseCom(LastTexture);
				ReleaseCom(LastBuffer1);
				ReleaseCom(LastBuffer2);

				return true;
			}
			bool D3D11Device::Submit()
			{
				return SwapChain->Present(VSyncMode, PresentFlags) == S_OK;
			}
			DepthStencilState* D3D11Device::CreateDepthStencilState(const DepthStencilState::Desc& I)
			{
				D3D11_DEPTH_STENCIL_DESC State;
				State.BackFace.StencilDepthFailOp = (D3D11_STENCIL_OP)I.BackFaceStencilDepthFailOperation;
				State.BackFace.StencilFailOp = (D3D11_STENCIL_OP)I.BackFaceStencilFailOperation;
				State.BackFace.StencilFunc = (D3D11_COMPARISON_FUNC)I.BackFaceStencilFunction;
				State.BackFace.StencilPassOp = (D3D11_STENCIL_OP)I.BackFaceStencilPassOperation;
				State.FrontFace.StencilDepthFailOp = (D3D11_STENCIL_OP)I.FrontFaceStencilDepthFailOperation;
				State.FrontFace.StencilFailOp = (D3D11_STENCIL_OP)I.FrontFaceStencilFailOperation;
				State.FrontFace.StencilFunc = (D3D11_COMPARISON_FUNC)I.FrontFaceStencilFunction;
				State.FrontFace.StencilPassOp = (D3D11_STENCIL_OP)I.FrontFaceStencilPassOperation;
				State.DepthEnable = I.DepthEnable;
				State.DepthFunc = (D3D11_COMPARISON_FUNC)I.DepthFunction;
				State.DepthWriteMask = (D3D11_DEPTH_WRITE_MASK)I.DepthWriteMask;
				State.StencilEnable = I.StencilEnable;
				State.StencilReadMask = I.StencilReadMask;
				State.StencilWriteMask = I.StencilWriteMask;

				ID3D11DepthStencilState* DeviceState = nullptr;
				if (D3DDevice->CreateDepthStencilState(&State, &DeviceState) != S_OK)
				{
					ReleaseCom(DeviceState);
					TH_ERROR("couldn't create depth stencil state");
					return nullptr;
				}

				D3D11DepthStencilState* Result = new D3D11DepthStencilState(I);
				Result->Resource = DeviceState;

				return Result;
			}
			BlendState* D3D11Device::CreateBlendState(const BlendState::Desc& I)
			{
				D3D11_BLEND_DESC State;
				State.AlphaToCoverageEnable = I.AlphaToCoverageEnable;
				State.IndependentBlendEnable = I.IndependentBlendEnable;

				for (unsigned int i = 0; i < 8; i++)
				{
					State.RenderTarget[i].BlendEnable = I.RenderTarget[i].BlendEnable;
					State.RenderTarget[i].BlendOp = (D3D11_BLEND_OP)I.RenderTarget[i].BlendOperationMode;
					State.RenderTarget[i].BlendOpAlpha = (D3D11_BLEND_OP)I.RenderTarget[i].BlendOperationAlpha;
					State.RenderTarget[i].DestBlend = (D3D11_BLEND)I.RenderTarget[i].DestBlend;
					State.RenderTarget[i].DestBlendAlpha = (D3D11_BLEND)I.RenderTarget[i].DestBlendAlpha;
					State.RenderTarget[i].RenderTargetWriteMask = I.RenderTarget[i].RenderTargetWriteMask;
					State.RenderTarget[i].SrcBlend = (D3D11_BLEND)I.RenderTarget[i].SrcBlend;
					State.RenderTarget[i].SrcBlendAlpha = (D3D11_BLEND)I.RenderTarget[i].SrcBlendAlpha;
				}

				ID3D11BlendState* DeviceState = nullptr;
				if (D3DDevice->CreateBlendState(&State, &DeviceState) != S_OK)
				{
					ReleaseCom(DeviceState);
					TH_ERROR("couldn't create blend state");
					return nullptr;
				}

				D3D11BlendState* Result = new D3D11BlendState(I);
				Result->Resource = DeviceState;

				return Result;
			}
			RasterizerState* D3D11Device::CreateRasterizerState(const RasterizerState::Desc& I)
			{
				D3D11_RASTERIZER_DESC State;
				State.AntialiasedLineEnable = I.AntialiasedLineEnable;
				State.CullMode = (D3D11_CULL_MODE)I.CullMode;
				State.DepthBias = I.DepthBias;
				State.DepthBiasClamp = I.DepthBiasClamp;
				State.DepthClipEnable = I.DepthClipEnable;
				State.FillMode = (D3D11_FILL_MODE)I.FillMode;
				State.FrontCounterClockwise = I.FrontCounterClockwise;
				State.MultisampleEnable = I.MultisampleEnable;
				State.ScissorEnable = I.ScissorEnable;
				State.SlopeScaledDepthBias = I.SlopeScaledDepthBias;

				ID3D11RasterizerState* DeviceState = nullptr;
				if (D3DDevice->CreateRasterizerState(&State, &DeviceState) != S_OK)
				{
					ReleaseCom(DeviceState);
					TH_ERROR("couldn't create Rasterizer state");
					return nullptr;
				}

				D3D11RasterizerState* Result = new D3D11RasterizerState(I);
				Result->Resource = DeviceState;

				return Result;
			}
			SamplerState* D3D11Device::CreateSamplerState(const SamplerState::Desc& I)
			{
				D3D11_SAMPLER_DESC State;
				State.AddressU = (D3D11_TEXTURE_ADDRESS_MODE)I.AddressU;
				State.AddressV = (D3D11_TEXTURE_ADDRESS_MODE)I.AddressV;
				State.AddressW = (D3D11_TEXTURE_ADDRESS_MODE)I.AddressW;
				State.ComparisonFunc = (D3D11_COMPARISON_FUNC)I.ComparisonFunction;
				State.Filter = (D3D11_FILTER)I.Filter;
				State.MaxAnisotropy = I.MaxAnisotropy;
				State.MaxLOD = I.MaxLOD;
				State.MinLOD = I.MinLOD;
				State.MipLODBias = I.MipLODBias;
				State.BorderColor[0] = I.BorderColor[0];
				State.BorderColor[1] = I.BorderColor[1];
				State.BorderColor[2] = I.BorderColor[2];
				State.BorderColor[3] = I.BorderColor[3];

				ID3D11SamplerState* DeviceState = nullptr;
				if (D3DDevice->CreateSamplerState(&State, &DeviceState) != S_OK)
				{
					ReleaseCom(DeviceState);
					TH_ERROR("couldn't create sampler state");
					return nullptr;
				}

				D3D11SamplerState* Result = new D3D11SamplerState(I);
				Result->Resource = DeviceState;

				return Result;
			}
			InputLayout* D3D11Device::CreateInputLayout(const InputLayout::Desc& I)
			{
				return new D3D11InputLayout(I);
			}
			Shader* D3D11Device::CreateShader(const Shader::Desc& I)
			{
				D3D11Shader* Result = new D3D11Shader(I);
				ID3DBlob* ShaderBlob = nullptr;
				ID3DBlob* ErrorBlob = nullptr;
				Shader::Desc F(I);

				if (!Preprocess(F))
				{
					TH_ERROR("shader preprocessing failed");
					return Result;
				}

				Core::Parser Code(&F.Data);
				uint64_t Length = Code.Size();

				std::string VertexEntry = GetShaderMain(ShaderType_Vertex);
				if (Code.Find(VertexEntry).Found)
				{
					D3DCompile(Code.Get(), (SIZE_T)Length * sizeof(char), F.Filename.empty() ? nullptr : F.Filename.c_str(), nullptr, nullptr, VertexEntry.c_str(), GetVSProfile(), CompileFlags, 0, &Result->Signature, &ErrorBlob);
					if (GetCompileState(ErrorBlob))
					{
						std::string Message = GetCompileState(ErrorBlob);
						ReleaseCom(ErrorBlob);

						TH_ERROR("couldn't compile vertex shader\n\t%s", Message.c_str());
						return Result;
					}

					D3DDevice->CreateVertexShader(Result->Signature->GetBufferPointer(), Result->Signature->GetBufferSize(), nullptr, &Result->VertexShader);
				}

				std::string PixelEntry = GetShaderMain(ShaderType_Pixel);
				if (Code.Find(PixelEntry).Found)
				{
					ShaderBlob = nullptr;
					D3DCompile(Code.Get(), (SIZE_T)Length * sizeof(char), F.Filename.empty() ? nullptr : F.Filename.c_str(), nullptr, nullptr, PixelEntry.c_str(), GetPSProfile(), CompileFlags, 0, &ShaderBlob, &ErrorBlob);
					if (GetCompileState(ErrorBlob))
					{
						std::string Message = GetCompileState(ErrorBlob);
						ReleaseCom(ErrorBlob);

						TH_ERROR("couldn't compile pixel shader\n\t%s", Message.c_str());
						return Result;
					}

					D3DDevice->CreatePixelShader(ShaderBlob->GetBufferPointer(), ShaderBlob->GetBufferSize(), nullptr, &Result->PixelShader);
					ReleaseCom(ShaderBlob);
				}

				std::string GeometryEntry = GetShaderMain(ShaderType_Geometry);
				if (Code.Find(GeometryEntry).Found)
				{
					ShaderBlob = nullptr;
					D3DCompile(Code.Get(), (SIZE_T)Length * sizeof(char), F.Filename.empty() ? nullptr : F.Filename.c_str(), nullptr, nullptr, GeometryEntry.c_str(), GetGSProfile(), GetCompileFlags(), 0, &ShaderBlob, &ErrorBlob);
					if (GetCompileState(ErrorBlob))
					{
						std::string Message = GetCompileState(ErrorBlob);
						ReleaseCom(ErrorBlob);

						TH_ERROR("couldn't compile geometry shader\n\t%s", Message.c_str());
						return Result;
					}

					D3DDevice->CreateGeometryShader(ShaderBlob->GetBufferPointer(), ShaderBlob->GetBufferSize(), nullptr, &Result->GeometryShader);
					ReleaseCom(ShaderBlob);
				}

				std::string ComputeEntry = GetShaderMain(ShaderType_Compute);
				if (Code.Find(ComputeEntry).Found)
				{
					ShaderBlob = nullptr;
					D3DCompile(Code.Get(), (SIZE_T)Length * sizeof(char), F.Filename.empty() ? nullptr : F.Filename.c_str(), nullptr, nullptr, ComputeEntry.c_str(), GetCSProfile(), GetCompileFlags(), 0, &ShaderBlob, &ErrorBlob);
					if (GetCompileState(ErrorBlob))
					{
						std::string Message = GetCompileState(ErrorBlob);
						ReleaseCom(ErrorBlob);

						TH_ERROR("couldn't compile compute shader\n\t%s", Message.c_str());
						return Result;
					}

					D3DDevice->CreateComputeShader(ShaderBlob->GetBufferPointer(), ShaderBlob->GetBufferSize(), nullptr, &Result->ComputeShader);
					ReleaseCom(ShaderBlob);
				}

				std::string HullEntry = GetShaderMain(ShaderType_Hull);
				if (Code.Find(HullEntry).Found)
				{
					ShaderBlob = nullptr;
					D3DCompile(Code.Get(), (SIZE_T)Length * sizeof(char), F.Filename.empty() ? nullptr : F.Filename.c_str(), nullptr, nullptr, HullEntry.c_str(), GetHSProfile(), GetCompileFlags(), 0, &ShaderBlob, &ErrorBlob);
					if (GetCompileState(ErrorBlob))
					{
						std::string Message = GetCompileState(ErrorBlob);
						ReleaseCom(ErrorBlob);

						TH_ERROR("couldn't compile hull shader\n\t%s", Message.c_str());
						return Result;
					}

					D3DDevice->CreateHullShader(ShaderBlob->GetBufferPointer(), ShaderBlob->GetBufferSize(), nullptr, &Result->HullShader);
					ReleaseCom(ShaderBlob);
				}

				std::string DomainEntry = GetShaderMain(ShaderType_Domain);
				if (Code.Find(DomainEntry).Found)
				{
					ShaderBlob = nullptr;
					D3DCompile(Code.Get(), (SIZE_T)Length * sizeof(char), F.Filename.empty() ? nullptr : F.Filename.c_str(), nullptr, nullptr, DomainEntry.c_str(), GetDSProfile(), GetCompileFlags(), 0, &ShaderBlob, &ErrorBlob);
					if (GetCompileState(ErrorBlob))
					{
						std::string Message = GetCompileState(ErrorBlob);
						ReleaseCom(ErrorBlob);

						TH_ERROR("couldn't compile domain shader\n\t%s", Message.c_str());
						return Result;
					}

					D3DDevice->CreateDomainShader(ShaderBlob->GetBufferPointer(), ShaderBlob->GetBufferSize(), nullptr, &Result->DomainShader);
					ReleaseCom(ShaderBlob);
				}

				Result->Compiled = true;
				return Result;
			}
			ElementBuffer* D3D11Device::CreateElementBuffer(const ElementBuffer::Desc& I)
			{
				D3D11_BUFFER_DESC BufferDesc;
				ZeroMemory(&BufferDesc, sizeof(BufferDesc));
				BufferDesc.Usage = (D3D11_USAGE)I.Usage;
				BufferDesc.ByteWidth = (unsigned int)I.ElementCount * I.ElementWidth;
				BufferDesc.BindFlags = I.BindFlags;
				BufferDesc.CPUAccessFlags = I.AccessFlags;
				BufferDesc.MiscFlags = I.MiscFlags;
				BufferDesc.StructureByteStride = I.StructureByteStride;

				D3D11ElementBuffer* Result = new D3D11ElementBuffer(I);
				if (I.Elements != nullptr)
				{
					D3D11_SUBRESOURCE_DATA Subresource;
					ZeroMemory(&Subresource, sizeof(Subresource));
					Subresource.pSysMem = I.Elements;

					D3DDevice->CreateBuffer(&BufferDesc, &Subresource, &Result->Element);
				}
				else
					D3DDevice->CreateBuffer(&BufferDesc, nullptr, &Result->Element);

				if (I.Writable)
				{
					D3D11_UNORDERED_ACCESS_VIEW_DESC AccessDesc;
					ZeroMemory(&AccessDesc, sizeof(AccessDesc));
					AccessDesc.Format = DXGI_FORMAT_R32_FLOAT;
					AccessDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
					AccessDesc.Buffer.Flags = 0;
					AccessDesc.Buffer.FirstElement = 0;
					AccessDesc.Buffer.NumElements = I.ElementCount;

					if (D3DDevice->CreateUnorderedAccessView(Result->Element, &AccessDesc, &Result->Access) != S_OK)
					{
						TH_ERROR("couldn't create buffer access view");
						return Result;
					}
				}

				if (!(I.MiscFlags & ResourceMisc_Buffer_Structured))
					return Result;

				D3D11_SHADER_RESOURCE_VIEW_DESC SRV;
				ZeroMemory(&SRV, sizeof(SRV));
				SRV.Format = DXGI_FORMAT_UNKNOWN;
				SRV.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
				SRV.Buffer.ElementWidth = I.ElementCount;

				if (D3DDevice->CreateShaderResourceView(Result->Element, &SRV, &Result->Resource) != S_OK)
				{
					TH_ERROR("couldn't create shader resource view");
					return Result;
				}

				return Result;
			}
			MeshBuffer* D3D11Device::CreateMeshBuffer(const MeshBuffer::Desc& I)
			{
				ElementBuffer::Desc F = ElementBuffer::Desc();
				F.AccessFlags = I.AccessFlags;
				F.Usage = I.Usage;
				F.BindFlags = ResourceBind_Vertex_Buffer;
				F.ElementCount = (unsigned int)I.Elements.size();
				F.Elements = (void*)I.Elements.data();
				F.ElementWidth = sizeof(Compute::Vertex);

				D3D11MeshBuffer* Result = new D3D11MeshBuffer(I);
				Result->VertexBuffer = CreateElementBuffer(F);

				F = ElementBuffer::Desc();
				F.AccessFlags = I.AccessFlags;
				F.Usage = I.Usage;
				F.BindFlags = ResourceBind_Index_Buffer;
				F.ElementCount = (unsigned int)I.Indices.size();
				F.ElementWidth = sizeof(int);
				F.Elements = (void*)I.Indices.data();

				Result->IndexBuffer = CreateElementBuffer(F);
				return Result;
			}
			SkinMeshBuffer* D3D11Device::CreateSkinMeshBuffer(const SkinMeshBuffer::Desc& I)
			{
				ElementBuffer::Desc F = ElementBuffer::Desc();
				F.AccessFlags = I.AccessFlags;
				F.Usage = I.Usage;
				F.BindFlags = ResourceBind_Vertex_Buffer;
				F.ElementCount = (unsigned int)I.Elements.size();
				F.Elements = (void*)I.Elements.data();
				F.ElementWidth = sizeof(Compute::SkinVertex);

				D3D11SkinMeshBuffer* Result = new D3D11SkinMeshBuffer(I);
				Result->VertexBuffer = CreateElementBuffer(F);

				F = ElementBuffer::Desc();
				F.AccessFlags = I.AccessFlags;
				F.Usage = I.Usage;
				F.BindFlags = ResourceBind_Index_Buffer;
				F.ElementCount = (unsigned int)I.Indices.size();
				F.ElementWidth = sizeof(int);
				F.Elements = (void*)I.Indices.data();

				Result->IndexBuffer = CreateElementBuffer(F);
				return Result;
			}
			InstanceBuffer* D3D11Device::CreateInstanceBuffer(const InstanceBuffer::Desc& I)
			{
				ElementBuffer::Desc F = ElementBuffer::Desc();
				F.AccessFlags = CPUAccess_Write;
				F.MiscFlags = ResourceMisc_Buffer_Structured;
				F.Usage = ResourceUsage_Dynamic;
				F.BindFlags = ResourceBind_Shader_Input;
				F.ElementCount = I.ElementLimit;
				F.ElementWidth = I.ElementWidth;
				F.StructureByteStride = F.ElementWidth;

				D3D11InstanceBuffer* Result = new D3D11InstanceBuffer(I);
				Result->Elements = CreateElementBuffer(F);

				D3D11_SHADER_RESOURCE_VIEW_DESC SRV;
				ZeroMemory(&SRV, sizeof(SRV));
				SRV.Format = DXGI_FORMAT_UNKNOWN;
				SRV.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
				SRV.Buffer.ElementWidth = I.ElementLimit;

				if (D3DDevice->CreateShaderResourceView(Result->Elements->As<D3D11ElementBuffer>()->Element, &SRV, &Result->Resource) != S_OK)
				{
					TH_ERROR("couldn't create shader resource view");
					return Result;
				}

				return Result;
			}
			Texture2D* D3D11Device::CreateTexture2D()
			{
				return new D3D11Texture2D();
			}
			Texture2D* D3D11Device::CreateTexture2D(const Texture2D::Desc& I)
			{
				D3D11_TEXTURE2D_DESC Description;
				ZeroMemory(&Description, sizeof(Description));
				Description.Width = I.Width;
				Description.Height = I.Height;
				Description.MipLevels = I.MipLevels;
				Description.ArraySize = 1;
				Description.Format = (DXGI_FORMAT)I.FormatMode;
				Description.SampleDesc.Count = 1;
				Description.SampleDesc.Quality = 0;
				Description.Usage = (D3D11_USAGE)I.Usage;
				Description.BindFlags = I.BindFlags;
				Description.CPUAccessFlags = I.AccessFlags;
				Description.MiscFlags = (unsigned int)I.MiscFlags;
		
				if (I.Data != nullptr && I.MipLevels > 0)
				{
					Description.BindFlags |= D3D11_BIND_RENDER_TARGET;
					Description.MiscFlags |= D3D11_RESOURCE_MISC_GENERATE_MIPS;
				}

				if (I.Writable)
					Description.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;

				D3D11_SUBRESOURCE_DATA Data;
				Data.pSysMem = I.Data;
				Data.SysMemPitch = I.RowPitch;
				Data.SysMemSlicePitch = I.DepthPitch;

				D3D11Texture2D* Result = new D3D11Texture2D();
				if (D3DDevice->CreateTexture2D(&Description, I.Data && I.MipLevels <= 0 ? &Data : nullptr, &Result->View) != S_OK)
				{
					TH_ERROR("couldn't create 2d texture");
					return Result;
				}

				if (!GenerateTexture(Result))
				{
					TH_ERROR("couldn't generate 2d resource");
					return Result;
				}

				if (I.Data != nullptr && I.MipLevels > 0)
				{
					ImmediateContext->UpdateSubresource(Result->View, 0, nullptr, I.Data, I.RowPitch, I.DepthPitch);
					ImmediateContext->GenerateMips(Result->Resource);
				}

				if (!I.Writable)
					return Result;

				D3D11_UNORDERED_ACCESS_VIEW_DESC AccessDesc;
				ZeroMemory(&AccessDesc, sizeof(AccessDesc));
				AccessDesc.Format = (DXGI_FORMAT)I.FormatMode;
				AccessDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
				AccessDesc.Texture2D.MipSlice = 0;

				if (D3DDevice->CreateUnorderedAccessView(Result->View, &AccessDesc, &Result->Access) != S_OK)
				{
					TH_ERROR("couldn't create 2d texture access view");
					return Result;
				}

				return Result;
			}
			Texture3D* D3D11Device::CreateTexture3D()
			{
				return new D3D11Texture3D();
			}
			Texture3D* D3D11Device::CreateTexture3D(const Texture3D::Desc& I)
			{
				D3D11_TEXTURE3D_DESC Description;
				ZeroMemory(&Description, sizeof(Description));
				Description.Width = I.Width;
				Description.Height = I.Height;
				Description.Depth = I.Depth;
				Description.MipLevels = I.MipLevels;
				Description.Format = (DXGI_FORMAT)I.FormatMode;
				Description.Usage = (D3D11_USAGE)I.Usage;
				Description.BindFlags = I.BindFlags;
				Description.CPUAccessFlags = I.AccessFlags;
				Description.MiscFlags = (unsigned int)I.MiscFlags;

				if (I.MipLevels > 0)
				{
					Description.BindFlags |= D3D11_BIND_RENDER_TARGET;
					Description.MiscFlags |= D3D11_RESOURCE_MISC_GENERATE_MIPS;
				}

				if (I.Writable)
					Description.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;

				D3D11Texture3D* Result = new D3D11Texture3D();
				if (D3DDevice->CreateTexture3D(&Description, nullptr, &Result->View) != S_OK)
				{
					TH_ERROR("couldn't create 3d resource");
					return Result;
				}

				if (!GenerateTexture(Result))
				{
					TH_ERROR("couldn't generate 3d resource");
					return Result;
				}

				if (!I.Writable)
					return Result;

				D3D11_UNORDERED_ACCESS_VIEW_DESC AccessDesc;
				ZeroMemory(&AccessDesc, sizeof(AccessDesc));
				AccessDesc.Format = (DXGI_FORMAT)I.FormatMode;
				AccessDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE3D;
				AccessDesc.Texture3D.MipSlice = 0;
				AccessDesc.Texture3D.FirstWSlice = 0;
				AccessDesc.Texture3D.WSize = I.Depth;

				if (D3DDevice->CreateUnorderedAccessView(Result->View, &AccessDesc, &Result->Access) != S_OK)
				{
					TH_ERROR("couldn't create 3d texture access view");
					return Result;
				}

				return Result;
			}
			TextureCube* D3D11Device::CreateTextureCube()
			{
				return new D3D11TextureCube();
			}
			TextureCube* D3D11Device::CreateTextureCube(const TextureCube::Desc& I)
			{
				D3D11_TEXTURE2D_DESC Description;
				ZeroMemory(&Description, sizeof(Description));
				Description.Width = I.Width;
				Description.Height = I.Height;
				Description.MipLevels = I.MipLevels;
				Description.ArraySize = 6;
				Description.Format = (DXGI_FORMAT)I.FormatMode;
				Description.SampleDesc.Count = 1;
				Description.SampleDesc.Quality = 0;
				Description.Usage = (D3D11_USAGE)I.Usage;
				Description.BindFlags = I.BindFlags;
				Description.CPUAccessFlags = I.AccessFlags;
				Description.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE | (unsigned int)I.MiscFlags;

				if (I.MipLevels > 0)
				{
					Description.BindFlags |= D3D11_BIND_RENDER_TARGET;
					Description.MiscFlags |= D3D11_RESOURCE_MISC_GENERATE_MIPS;
				}

				if (I.Writable)
					Description.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;

				D3D11TextureCube* Result = new D3D11TextureCube();
				if (D3DDevice->CreateTexture2D(&Description, 0, &Result->View) != S_OK)
				{
					TH_ERROR("couldn't create texture cube");
					return Result;
				}

				D3D11_BOX Region;
				Region.left = 0;
				Region.right = Description.Width;
				Region.top = 0;
				Region.bottom = Description.Height;
				Region.front = 0;
				Region.back = 1;

				if (!GenerateTexture(Result))
				{
					TH_ERROR("couldn't generate cube resource");
					return Result;
				}

				if (!I.Writable)
					return Result;

				D3D11_UNORDERED_ACCESS_VIEW_DESC AccessDesc;
				ZeroMemory(&AccessDesc, sizeof(AccessDesc));
				AccessDesc.Format = (DXGI_FORMAT)I.FormatMode;
				AccessDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
				AccessDesc.Texture2DArray.MipSlice = 0;
				AccessDesc.Texture2DArray.FirstArraySlice = 0;
				AccessDesc.Texture2DArray.ArraySize = 6;

				if (D3DDevice->CreateUnorderedAccessView(Result->View, &AccessDesc, &Result->Access) != S_OK)
				{
					TH_ERROR("couldn't create cube texture access view");
					return Result;
				}

				return Result;
			}
			TextureCube* D3D11Device::CreateTextureCube(Graphics::Texture2D* Resource[6])
			{
				if (!Resource[0] || !Resource[1] || !Resource[2] || !Resource[3] || !Resource[4] || !Resource[5])
				{
					TH_ERROR("couldn't create texture cube without proper mapping");
					return nullptr;
				}

				void* Resources[6];
				for (unsigned int i = 0; i < 6; i++)
					Resources[i] = (void*)Resource[i]->As<D3D11Texture2D>()->View;

				return CreateTextureCubeInternal(Resources);
			}
			TextureCube* D3D11Device::CreateTextureCube(Graphics::Texture2D* Resource)
			{
				ID3D11Texture2D* Src = (Resource ? Resource->As<D3D11Texture2D>()->View : nullptr);
				if (!Src)
				{
					TH_ERROR("couldn't create texture cube without proper mapping");
					return nullptr;
				}

				D3D11_TEXTURE2D_DESC Description;
				Src->GetDesc(&Description);
				Description.ArraySize = 6;
				Description.Usage = D3D11_USAGE_DEFAULT;
				Description.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
				Description.CPUAccessFlags = 0;
				Description.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE | D3D11_RESOURCE_MISC_GENERATE_MIPS;

				unsigned int Width = Description.Width;
				Description.Width = Description.Width / 4;

				unsigned int Height = Description.Height;
				Description.Height = Description.Width;

				D3D11_BOX Region;
				Region.front = 0;
				Region.back = 1;

				Description.MipLevels = GetMipLevel(Description.Width, Description.Height);
				if (Width % 4 != 0 || Height % 3 != 0)
				{
					TH_ERROR("couldn't create texture cube because width or height cannot be not divided");
					return nullptr;
				}

				D3D11TextureCube* Result = new D3D11TextureCube();
				if (D3DDevice->CreateTexture2D(&Description, 0, &Result->View) != S_OK)
				{
					TH_ERROR("couldn't create texture 2d");
					return Result;
				}

				Region.left = Description.Width * 2 - 1;
				Region.top = Description.Height - 1;
				Region.right = Region.left + Description.Width;
				Region.bottom = Region.top + Description.Height;
				ImmediateContext->CopySubresourceRegion(Result->View, D3D11CalcSubresource(0, 0, Description.MipLevels), 0, 0, 0, Src, 0, &Region);

				Region.left = 0;
				Region.top = Description.Height;
				Region.right = Region.left + Description.Width;
				Region.bottom = Region.top + Description.Height;
				ImmediateContext->CopySubresourceRegion(Result->View, D3D11CalcSubresource(0, 1, Description.MipLevels), 0, 0, 0, Src, 0, &Region);

				Region.left = Description.Width;
				Region.top = 0;
				Region.right = Region.left + Description.Width;
				Region.bottom = Region.top + Description.Height;
				ImmediateContext->CopySubresourceRegion(Result->View, D3D11CalcSubresource(0, 2, Description.MipLevels), 0, 0, 0, Src, 0, &Region);

				Region.top = Description.Height * 2;
				Region.right = Region.left + Description.Width;
				Region.bottom = Region.top + Description.Height;
				ImmediateContext->CopySubresourceRegion(Result->View, D3D11CalcSubresource(0, 3, Description.MipLevels), 0, 0, 0, Src, 0, &Region);

				Region.top = Description.Height;
				Region.right = Region.left + Description.Width;
				Region.bottom = Region.top + Description.Height;
				ImmediateContext->CopySubresourceRegion(Result->View, D3D11CalcSubresource(0, 4, Description.MipLevels), 0, 0, 0, Src, 0, &Region);

				Region.left = Description.Width * 3;
				Region.right = Region.left + Description.Width;
				Region.bottom = Region.top + Description.Height;
				ImmediateContext->CopySubresourceRegion(Result->View, D3D11CalcSubresource(0, 5, Description.MipLevels), 0, 0, 0, Src, 0, &Region);

				GenerateTexture(Result);
				if (Description.MipLevels > 0)
					ImmediateContext->GenerateMips(Result->Resource);

				return Result;
			}
			TextureCube* D3D11Device::CreateTextureCubeInternal(void* Resource[6])
			{
				if (!Resource[0] || !Resource[1] || !Resource[2] || !Resource[3] || !Resource[4] || !Resource[5])
				{
					TH_ERROR("couldn't create texture cube without proper mapping");
					return nullptr;
				}

				D3D11_TEXTURE2D_DESC Description;
				((ID3D11Texture2D*)Resource[0])->GetDesc(&Description);
				Description.MipLevels = 1;
				Description.ArraySize = 6;
				Description.Usage = D3D11_USAGE_DEFAULT;
				Description.BindFlags = D3D11_BIND_SHADER_RESOURCE;
				Description.CPUAccessFlags = 0;
				Description.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;

				D3D11TextureCube* Result = new D3D11TextureCube();
				if (D3DDevice->CreateTexture2D(&Description, 0, &Result->View) != S_OK)
				{
					TH_ERROR("couldn't create texture 2d");
					return Result;
				}

				D3D11_BOX Region;
				Region.left = 0;
				Region.right = Description.Width;
				Region.top = 0;
				Region.bottom = Description.Height;
				Region.front = 0;
				Region.back = 1;

				for (unsigned int j = 0; j < 6; j++)
					ImmediateContext->CopySubresourceRegion(Result->View, j, 0, 0, 0, (ID3D11Texture2D*)Resource[j], 0, &Region);

				GenerateTexture(Result);
				return Result;
			}
			DepthBuffer* D3D11Device::CreateDepthBuffer(const DepthBuffer::Desc& I)
			{
				D3D11DepthBuffer* Result = new D3D11DepthBuffer(I);
				
				D3D11_TEXTURE2D_DESC DepthBuffer;
				ZeroMemory(&DepthBuffer, sizeof(DepthBuffer));
				DepthBuffer.Width = I.Width;
				DepthBuffer.Height = I.Height;
				DepthBuffer.MipLevels = 1;
				DepthBuffer.ArraySize = 1;
				DepthBuffer.Format = DXGI_FORMAT_R24G8_TYPELESS;
				DepthBuffer.SampleDesc.Count = 1;
				DepthBuffer.SampleDesc.Quality = 0;
				DepthBuffer.Usage = (D3D11_USAGE)I.Usage;
				DepthBuffer.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
				DepthBuffer.CPUAccessFlags = I.AccessFlags;
				DepthBuffer.MiscFlags = 0;

				ID3D11Texture2D* DepthTexture = nullptr;
				if (D3DDevice->CreateTexture2D(&DepthBuffer, nullptr, &DepthTexture) != S_OK)
				{
					TH_ERROR("couldn't create depth buffer texture 2d");
					return Result;
				}

				D3D11_DEPTH_STENCIL_VIEW_DESC DSV;
				ZeroMemory(&DSV, sizeof(DSV));
				DSV.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
				DSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
				DSV.Texture2D.MipSlice = 0;

				if (D3DDevice->CreateDepthStencilView(DepthTexture, &DSV, &Result->DepthStencilView) != S_OK)
				{
					TH_ERROR("couldn't create depth stencil view");
					return Result;
				}

				Result->Resource = CreateTexture2D();
				Result->Resource->As<D3D11Texture2D>()->View = DepthTexture;

				if (!CreateTexture2D(Result->Resource, DXGI_FORMAT_R24_UNORM_X8_TYPELESS))
				{
					TH_ERROR("couldn't create shader resource view");
					return Result;
				}

				Result->Viewarea.Width = (FLOAT)I.Width;
				Result->Viewarea.Height = (FLOAT)I.Height;
				Result->Viewarea.MinDepth = 0.0f;
				Result->Viewarea.MaxDepth = 1.0f;
				Result->Viewarea.TopLeftX = 0.0f;
				Result->Viewarea.TopLeftY = 0.0f;

				return Result;
			}
			RenderTarget2D* D3D11Device::CreateRenderTarget2D(const RenderTarget2D::Desc& I)
			{
				D3D11RenderTarget2D* Result = new D3D11RenderTarget2D(I);

				D3D11_TEXTURE2D_DESC DepthBuffer;
				ZeroMemory(&DepthBuffer, sizeof(DepthBuffer));
				DepthBuffer.Width = I.Width;
				DepthBuffer.Height = I.Height;
				DepthBuffer.MipLevels = 1;
				DepthBuffer.ArraySize = 1;
				DepthBuffer.Format = DXGI_FORMAT_R24G8_TYPELESS;
				DepthBuffer.SampleDesc.Count = 1;
				DepthBuffer.SampleDesc.Quality = 0;
				DepthBuffer.Usage = (D3D11_USAGE)I.Usage;
				DepthBuffer.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
				DepthBuffer.CPUAccessFlags = I.AccessFlags;
				DepthBuffer.MiscFlags = 0;

				ID3D11Texture2D* DepthTexture = nullptr;
				if (D3DDevice->CreateTexture2D(&DepthBuffer, nullptr, &DepthTexture) != S_OK)
				{
					TH_ERROR("couldn't create depth buffer texture 2d");
					return Result;
				}

				D3D11_DEPTH_STENCIL_VIEW_DESC DSV;
				ZeroMemory(&DSV, sizeof(DSV));
				DSV.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
				DSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
				DSV.Texture2D.MipSlice = 0;

				if (D3DDevice->CreateDepthStencilView(DepthTexture, &DSV, &Result->DepthStencilView) != S_OK)
				{
					TH_ERROR("couldn't create depth stencil view");
					return Result;
				}

				Result->DepthStencil = CreateTexture2D();
				Result->DepthStencil->As<D3D11Texture2D>()->View = DepthTexture;

				if (!CreateTexture2D(Result->DepthStencil, DXGI_FORMAT_R24_UNORM_X8_TYPELESS))
				{
					TH_ERROR("couldn't create shader resource view");
					return Result;
				}

				if (I.RenderSurface == nullptr)
				{
					D3D11_TEXTURE2D_DESC Description;
					ZeroMemory(&Description, sizeof(Description));
					Description.Width = I.Width;
					Description.Height = I.Height;
					Description.MipLevels = (I.MipLevels < 1 ? 1 : I.MipLevels);
					Description.ArraySize = 1;
					Description.Format = (DXGI_FORMAT)I.FormatMode;
					Description.SampleDesc.Count = 1;
					Description.SampleDesc.Quality = 0;
					Description.Usage = (D3D11_USAGE)I.Usage;
					Description.BindFlags = I.BindFlags;
					Description.CPUAccessFlags = I.AccessFlags;
					Description.MiscFlags = (unsigned int)I.MiscFlags;

					if (D3DDevice->CreateTexture2D(&Description, nullptr, &Result->Texture) != S_OK)
					{
						TH_ERROR("couldn't create surface texture view");
						return Result;
					}

					Result->Resource = CreateTexture2D();
					Result->Resource->As<D3D11Texture2D>()->View = Result->Texture;
					Result->Texture->AddRef();

					if (!GenerateTexture(Result->Resource))
					{
						TH_ERROR("couldn't create shader resource view");
						return Result;
					}
				}

				D3D11_RENDER_TARGET_VIEW_DESC RTV;
				RTV.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
				RTV.Texture2DArray.MipSlice = 0;
				RTV.Texture2DArray.ArraySize = 1;
				RTV.Format = (DXGI_FORMAT)I.FormatMode;

				if (D3DDevice->CreateRenderTargetView(I.RenderSurface ? (ID3D11Texture2D*)I.RenderSurface : Result->Texture, &RTV, &Result->RenderTargetView) != S_OK)
				{
					TH_ERROR("couldn't create render target view");
					return Result;
				}

				Result->Viewarea.Width = I.Width;
				Result->Viewarea.Height = I.Height;
				Result->Viewarea.MinDepth = 0.0f;
				Result->Viewarea.MaxDepth = 1.0f;
				Result->Viewarea.TopLeftX = 0.0f;
				Result->Viewarea.TopLeftY = 0.0f;

				if (!I.RenderSurface)
					return Result;

				SwapChain->GetBuffer(0, __uuidof(Result->Texture), reinterpret_cast<void**>(&Result->Texture));

				D3D11Texture2D* Target = (D3D11Texture2D*)CreateTexture2D();
				Target->View = Result->Texture;
				Result->Resource = Target;
				Result->Texture->AddRef();

				D3D11_TEXTURE2D_DESC Description;
				Target->View->GetDesc(&Description);
				Target->FormatMode = (Format)Description.Format;
				Target->Usage = (ResourceUsage)Description.Usage;
				Target->Width = Description.Width;
				Target->Height = Description.Height;
				Target->MipLevels = Description.MipLevels;
				Target->AccessFlags = (CPUAccess)Description.CPUAccessFlags;

				return Result;
			}
			MultiRenderTarget2D* D3D11Device::CreateMultiRenderTarget2D(const MultiRenderTarget2D::Desc& I)
			{
				D3D11MultiRenderTarget2D* Result = new D3D11MultiRenderTarget2D(I);
				unsigned int MipLevels = (I.MipLevels < 1 ? 1 : I.MipLevels);

				D3D11_TEXTURE2D_DESC DepthBuffer;
				ZeroMemory(&DepthBuffer, sizeof(DepthBuffer));
				DepthBuffer.Width = I.Width;
				DepthBuffer.Height = I.Height;
				DepthBuffer.MipLevels = 1;
				DepthBuffer.ArraySize = 1;
				DepthBuffer.Format = DXGI_FORMAT_R24G8_TYPELESS;
				DepthBuffer.SampleDesc.Count = 1;
				DepthBuffer.SampleDesc.Quality = 0;
				DepthBuffer.Usage = (D3D11_USAGE)I.Usage;
				DepthBuffer.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
				DepthBuffer.CPUAccessFlags = I.AccessFlags;
				DepthBuffer.MiscFlags = 0;

				ID3D11Texture2D* DepthTexture = nullptr;
				if (D3DDevice->CreateTexture2D(&DepthBuffer, nullptr, &DepthTexture) != S_OK)
				{
					TH_ERROR("couldn't create depth buffer texture 2d");
					return Result;
				}

				D3D11_DEPTH_STENCIL_VIEW_DESC DSV;
				ZeroMemory(&DSV, sizeof(DSV));
				DSV.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
				DSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
				DSV.Texture2D.MipSlice = 0;

				if (D3DDevice->CreateDepthStencilView(DepthTexture, &DSV, &Result->DepthStencilView) != S_OK)
				{
					TH_ERROR("couldn't create depth stencil view");
					return Result;
				}

				Result->DepthStencil = CreateTexture2D();
				Result->DepthStencil->As<D3D11Texture2D>()->View = DepthTexture;

				if (!CreateTexture2D(Result->DepthStencil, DXGI_FORMAT_R24_UNORM_X8_TYPELESS))
				{
					TH_ERROR("couldn't create shader resource view");
					return Result;
				}

				ZeroMemory(&Result->Information, sizeof(Result->Information));
				Result->Information.Width = I.Width;
				Result->Information.Height = I.Height;
				Result->Information.MipLevels = MipLevels;
				Result->Information.ArraySize = 1;
				Result->Information.SampleDesc.Count = 1;
				Result->Information.SampleDesc.Quality = 0;
				Result->Information.Usage = (D3D11_USAGE)I.Usage;
				Result->Information.BindFlags = I.BindFlags;
				Result->Information.CPUAccessFlags = I.AccessFlags;
				Result->Information.MiscFlags = (unsigned int)I.MiscFlags;

				D3D11_RENDER_TARGET_VIEW_DESC RTV;
				RTV.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
				RTV.Texture2DArray.MipSlice = 0;
				RTV.Texture2DArray.ArraySize = 1;

				for (unsigned int i = 0; i < Result->Target; i++)
				{
					Result->Information.Format = (DXGI_FORMAT)I.FormatMode[i];
					if (D3DDevice->CreateTexture2D(&Result->Information, nullptr, &Result->Texture[i]) != S_OK)
					{
						TH_ERROR("couldn't create surface texture 2d #%i", i);
						return Result;
					}

					RTV.Format = (DXGI_FORMAT)I.FormatMode[i];
					if (D3DDevice->CreateRenderTargetView(Result->Texture[i], &RTV, &Result->RenderTargetView[i]) != S_OK)
					{
						TH_ERROR("couldn't create render target view #%i", i);
						return Result;
					}

					Result->Resource[i] = CreateTexture2D();
					Result->Resource[i]->As<D3D11Texture2D>()->View = Result->Texture[i];
					Result->Texture[i]->AddRef();

					if (!GenerateTexture(Result->Resource[i]))
					{
						TH_ERROR("couldn't create shader resource view");
						return Result;
					}
				}

				Result->Viewarea.Width = I.Width;
				Result->Viewarea.Height = I.Height;
				Result->Viewarea.MinDepth = 0.0f;
				Result->Viewarea.MaxDepth = 1.0f;
				Result->Viewarea.TopLeftX = 0.0f;
				Result->Viewarea.TopLeftY = 0.0f;

				return Result;
			}
			RenderTargetCube* D3D11Device::CreateRenderTargetCube(const RenderTargetCube::Desc& I)
			{
				D3D11RenderTargetCube* Result = new D3D11RenderTargetCube(I);
				unsigned int MipLevels = (I.MipLevels < 1 ? 1 : I.MipLevels);

				D3D11_TEXTURE2D_DESC DepthBuffer;
				ZeroMemory(&DepthBuffer, sizeof(DepthBuffer));
				DepthBuffer.Width = I.Size;
				DepthBuffer.Height = I.Size;
				DepthBuffer.MipLevels = 1;
				DepthBuffer.ArraySize = 6;
				DepthBuffer.Format = DXGI_FORMAT_R24G8_TYPELESS;
				DepthBuffer.SampleDesc.Count = 1;
				DepthBuffer.SampleDesc.Quality = 0;
				DepthBuffer.Usage = (D3D11_USAGE)I.Usage;
				DepthBuffer.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
				DepthBuffer.CPUAccessFlags = I.AccessFlags;
				DepthBuffer.MiscFlags = 0;

				ID3D11Texture2D* DepthTexture = nullptr;
				if (D3DDevice->CreateTexture2D(&DepthBuffer, nullptr, &DepthTexture) != S_OK)
				{
					TH_ERROR("couldn't create depth buffer texture 2d");
					return Result;
				}

				D3D11_DEPTH_STENCIL_VIEW_DESC DSV;
				ZeroMemory(&DSV, sizeof(DSV));
				DSV.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
				DSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
				DSV.Texture2DArray.FirstArraySlice = 0;
				DSV.Texture2DArray.ArraySize = 6;
				DSV.Texture2DArray.MipSlice = 0;

				if (D3DDevice->CreateDepthStencilView(DepthTexture, &DSV, &Result->DepthStencilView) != S_OK)
				{
					TH_ERROR("couldn't create depth stencil view");
					return Result;
				}

				Result->DepthStencil = CreateTexture2D();
				Result->DepthStencil->As<D3D11Texture2D>()->View = DepthTexture;

				if (!CreateTexture2D(Result->DepthStencil, DXGI_FORMAT_R24_UNORM_X8_TYPELESS))
				{
					TH_ERROR("couldn't create shader resource view");
					return Result;
				}

				D3D11_TEXTURE2D_DESC Description;
				ZeroMemory(&Description, sizeof(Description));
				Description.Width = I.Size;
				Description.Height = I.Size;
				Description.MipLevels = MipLevels;
				Description.ArraySize = 6;
				Description.SampleDesc.Count = 1;
				Description.SampleDesc.Quality = 0;
				Description.Format = (DXGI_FORMAT)I.FormatMode;
				Description.Usage = (D3D11_USAGE)I.Usage;
				Description.BindFlags = I.BindFlags;
				Description.CPUAccessFlags = I.AccessFlags;
				Description.MiscFlags = (unsigned int)I.MiscFlags;

				if (D3DDevice->CreateTexture2D(&Description, nullptr, &Result->Texture) != S_OK)
				{
					TH_ERROR("couldn't create cube map texture 2d");
					return Result;
				}

				D3D11_RENDER_TARGET_VIEW_DESC RTV;
				ZeroMemory(&RTV, sizeof(RTV));
				RTV.Format = Description.Format;
				RTV.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
				RTV.Texture2DArray.FirstArraySlice = 0;
				RTV.Texture2DArray.ArraySize = 6;
				RTV.Texture2DArray.MipSlice = 0;

				if (D3DDevice->CreateRenderTargetView(Result->Texture, &RTV, &Result->RenderTargetView) != S_OK)
				{
					TH_ERROR("couldn't create render target view");
					return Result;
				}

				Result->Resource = CreateTexture2D();
				Result->Resource->As<D3D11Texture2D>()->View = Result->Texture;
				Result->Texture->AddRef();

				if (!GenerateTexture(Result->Resource))
				{
					TH_ERROR("couldn't create shader resource view");
					return Result;
				}

				Result->Viewarea.Width = I.Size;
				Result->Viewarea.Height = I.Size;
				Result->Viewarea.MinDepth = 0.0f;
				Result->Viewarea.MaxDepth = 1.0f;
				Result->Viewarea.TopLeftX = 0.0f;
				Result->Viewarea.TopLeftY = 0.0f;

				return Result;
			}
			MultiRenderTargetCube* D3D11Device::CreateMultiRenderTargetCube(const MultiRenderTargetCube::Desc& I)
			{
				D3D11MultiRenderTargetCube* Result = new D3D11MultiRenderTargetCube(I);
				unsigned int MipLevels = (I.MipLevels < 1 ? 1 : I.MipLevels);

				D3D11_TEXTURE2D_DESC DepthBuffer;
				ZeroMemory(&DepthBuffer, sizeof(DepthBuffer));
				DepthBuffer.Width = I.Size;
				DepthBuffer.Height = I.Size;
				DepthBuffer.MipLevels = 1;
				DepthBuffer.ArraySize = 6;
				DepthBuffer.Format = DXGI_FORMAT_R24G8_TYPELESS;
				DepthBuffer.SampleDesc.Count = 1;
				DepthBuffer.SampleDesc.Quality = 0;
				DepthBuffer.Usage = (D3D11_USAGE)I.Usage;
				DepthBuffer.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
				DepthBuffer.CPUAccessFlags = I.AccessFlags;
				DepthBuffer.MiscFlags = 0;

				ID3D11Texture2D* DepthTexture = nullptr;
				if (D3DDevice->CreateTexture2D(&DepthBuffer, nullptr, &DepthTexture) != S_OK)
				{
					TH_ERROR("couldn't create depth buffer texture 2d");
					return Result;
				}

				D3D11_DEPTH_STENCIL_VIEW_DESC DSV;
				ZeroMemory(&DSV, sizeof(DSV));
				DSV.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
				DSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
				DSV.Texture2DArray.FirstArraySlice = 0;
				DSV.Texture2DArray.ArraySize = 6;
				DSV.Texture2DArray.MipSlice = 0;

				if (D3DDevice->CreateDepthStencilView(DepthTexture, &DSV, &Result->DepthStencilView) != S_OK)
				{
					TH_ERROR("couldn't create depth stencil view");
					return Result;
				}

				Result->DepthStencil = CreateTexture2D();
				Result->DepthStencil->As<D3D11Texture2D>()->View = DepthTexture;

				if (!CreateTexture2D(Result->DepthStencil, DXGI_FORMAT_R24_UNORM_X8_TYPELESS))
				{
					TH_ERROR("couldn't create shader resource view");
					return Result;
				}

				D3D11_TEXTURE2D_DESC Description;
				ZeroMemory(&Description, sizeof(Description));
				Description.Width = I.Size;
				Description.Height = I.Size;
				Description.ArraySize = 6;
				Description.SampleDesc.Count = 1;
				Description.SampleDesc.Quality = 0;
				Description.Usage = (D3D11_USAGE)I.Usage;
				Description.CPUAccessFlags = I.AccessFlags;
				Description.MiscFlags = (unsigned int)I.MiscFlags;
				Description.BindFlags = I.BindFlags;
				Description.MipLevels = MipLevels;

				for (unsigned int i = 0; i < Result->Target; i++)
				{
					Description.Format = (DXGI_FORMAT)I.FormatMode[i];
					if (D3DDevice->CreateTexture2D(&Description, nullptr, &Result->Texture[i]) != S_OK)
					{
						TH_ERROR("couldn't create cube map rexture 2d");
						return Result;
					}
				}

				D3D11_RENDER_TARGET_VIEW_DESC RTV;
				ZeroMemory(&RTV, sizeof(RTV));
				RTV.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
				RTV.Texture2DArray.FirstArraySlice = 0;
				RTV.Texture2DArray.ArraySize = 6;
				RTV.Texture2DArray.MipSlice = 0;

				for (unsigned int i = 0; i < Result->Target; i++)
				{
					RTV.Format = (DXGI_FORMAT)I.FormatMode[i];
					if (D3DDevice->CreateRenderTargetView(Result->Texture[i], &RTV, &Result->RenderTargetView[i]) != S_OK)
					{
						TH_ERROR("couldn't create render target view");
						return Result;
					}
				}

				D3D11_SHADER_RESOURCE_VIEW_DESC SRV;
				ZeroMemory(&SRV, sizeof(SRV));
				SRV.Format = Description.Format;
				SRV.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
				SRV.TextureCube.MostDetailedMip = 0;
				SRV.TextureCube.MipLevels = MipLevels;

				for (unsigned int i = 0; i < Result->Target; i++)
				{
					Result->Resource[i] = CreateTexture2D();
					Result->Resource[i]->As<D3D11Texture2D>()->View = Result->Texture[i];
					Result->Texture[i]->AddRef();

					if (!GenerateTexture(Result->Resource[i]))
					{
						TH_ERROR("couldn't create shader resource view");
						return Result;
					}
				}

				Result->Viewarea.Width = I.Size;
				Result->Viewarea.Height = I.Size;
				Result->Viewarea.MinDepth = 0.0f;
				Result->Viewarea.MaxDepth = 1.0f;
				Result->Viewarea.TopLeftX = 0.0f;
				Result->Viewarea.TopLeftY = 0.0f;

				return Result;
			}
			Cubemap* D3D11Device::CreateCubemap(const Cubemap::Desc& I)
			{
				return new D3D11Cubemap(I);
			}
			Query* D3D11Device::CreateQuery(const Query::Desc& I)
			{
				D3D11_QUERY_DESC Desc;
				Desc.Query = (I.Predicate ? D3D11_QUERY_OCCLUSION_PREDICATE : D3D11_QUERY_OCCLUSION);
				Desc.MiscFlags = (I.AutoPass ? D3D11_QUERY_MISC_PREDICATEHINT : 0);

				D3D11Query* Result = new D3D11Query();
				D3DDevice->CreateQuery(&Desc, &Result->Async);

				return Result;
			}
			PrimitiveTopology D3D11Device::GetPrimitiveTopology()
			{
				D3D11_PRIMITIVE_TOPOLOGY Topology;
				ImmediateContext->IAGetPrimitiveTopology(&Topology);

				return (PrimitiveTopology)Topology;
			}
			ShaderModel D3D11Device::GetSupportedShaderModel()
			{
				if (FeatureLevel == D3D_FEATURE_LEVEL::D3D_FEATURE_LEVEL_11_0)
					return ShaderModel_HLSL_5_0;

				if (FeatureLevel == D3D_FEATURE_LEVEL::D3D_FEATURE_LEVEL_10_1)
					return ShaderModel_HLSL_4_1;

				if (FeatureLevel == D3D_FEATURE_LEVEL::D3D_FEATURE_LEVEL_10_0)
					return ShaderModel_HLSL_4_0;

				if (FeatureLevel == D3D_FEATURE_LEVEL::D3D_FEATURE_LEVEL_9_3)
					return ShaderModel_HLSL_3_0;

				if (FeatureLevel == D3D_FEATURE_LEVEL::D3D_FEATURE_LEVEL_9_2)
					return ShaderModel_HLSL_2_0;

				if (FeatureLevel == D3D_FEATURE_LEVEL::D3D_FEATURE_LEVEL_9_1)
					return ShaderModel_HLSL_1_0;

				return ShaderModel_Invalid;
			}
			void* D3D11Device::GetDevice()
			{
				return (void*)D3DDevice;
			}
			void* D3D11Device::GetContext()
			{
				return (void*)ImmediateContext;
			}
			bool D3D11Device::IsValid()
			{
				return BasicEffect != nullptr;
			}
			bool D3D11Device::CreateDirectBuffer(uint64_t Size)
			{
				MaxElements = Size + 1;
				ReleaseCom(DirectRenderer.VertexBuffer);

				D3D11_BUFFER_DESC BufferDesc;
				ZeroMemory(&BufferDesc, sizeof(BufferDesc));
				BufferDesc.Usage = D3D11_USAGE_DYNAMIC;
				BufferDesc.ByteWidth = (unsigned int)MaxElements * sizeof(Vertex);
				BufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
				BufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

				if (D3DDevice->CreateBuffer(&BufferDesc, nullptr, &DirectRenderer.VertexBuffer) != S_OK)
				{
					TH_ERROR("couldn't create vertex buffer");
					return false;
				}

				if (!DirectRenderer.VertexShader)
				{
					static const char* VertexShaderCode = HLSL_INLINE(
						cbuffer VertexBuffer : register(b0)
						{
							matrix WorldViewProjection;
							float4 Padding;
						};

						struct VS_INPUT
						{
							float4 Position : POSITION0;
							float2 TexCoord : TEXCOORD0;
							float4 Color : COLOR0;
						};

						struct PS_INPUT
						{
							float4 Position : SV_POSITION;
							float2 TexCoord : TEXCOORD0;
							float4 Color : COLOR0;
						};

						PS_INPUT vs_main(VS_INPUT Input)
						{
							PS_INPUT Output;
							Output.Position = mul(WorldViewProjection, float4(Input.Position.xyz, 1));
							Output.Color = Input.Color;
							Output.TexCoord = Input.TexCoord;

							return Output;
						};
					);

					ID3DBlob* Blob = nullptr, *Error = nullptr;
					D3DCompile(VertexShaderCode, strlen(VertexShaderCode), nullptr, nullptr, nullptr, "vs_main", GetVSProfile(), 0, 0, &Blob, nullptr);
					if (GetCompileState(Error))
					{
						std::string Message = GetCompileState(Error);
						ReleaseCom(Error);

						TH_ERROR("couldn't compile vertex shader\n\t%s", Message.c_str());
						return false;
					}

					if (D3DDevice->CreateVertexShader((DWORD*)Blob->GetBufferPointer(), Blob->GetBufferSize(), nullptr, &DirectRenderer.VertexShader) != S_OK)
					{
						ReleaseCom(Blob);
						TH_ERROR("couldn't create vertex shader");
						return false;
					}

					D3D11_INPUT_ELEMENT_DESC LayoutDesc[] =
					{
						{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
						{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 3 * sizeof(float), D3D11_INPUT_PER_VERTEX_DATA, 0 },
						{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 5 * sizeof(float), D3D11_INPUT_PER_VERTEX_DATA, 0 }
					};
					if (D3DDevice->CreateInputLayout(LayoutDesc, 3, Blob->GetBufferPointer(), Blob->GetBufferSize(), &DirectRenderer.VertexLayout) != S_OK)
					{
						ReleaseCom(Blob);
						TH_ERROR("couldn't create input layout");
						return false;
					}

					ReleaseCom(Blob);
				}

				if (!DirectRenderer.PixelShader)
				{
					static const char* PixelShaderCode = HLSL_INLINE(
						cbuffer VertexBuffer : register(b0)
						{
							matrix WorldViewProjection;
							float4 Padding;
						};

						struct PS_INPUT
						{
							float4 Position : SV_POSITION;
							float2 TexCoord : TEXCOORD0;
							float4 Color : COLOR0;
						};

						Texture2D Diffuse : register(t0);
						SamplerState State : register(s0);

						float4 ps_main(PS_INPUT Input) : SV_TARGET0
						{
						   if (Padding.z > 0)
							   return Input.Color * Diffuse.Sample(State, Input.TexCoord + Padding.xy) * Padding.w;

						   return Input.Color * Padding.w;
						};
					);

					ID3DBlob* Blob = nullptr, *Error = nullptr;
					D3DCompile(PixelShaderCode, strlen(PixelShaderCode), nullptr, nullptr, nullptr, "ps_main", GetPSProfile(), 0, 0, &Blob, &Error);
					if (GetCompileState(Error))
					{
						std::string Message = GetCompileState(Error);
						ReleaseCom(Error);

						TH_ERROR("couldn't compile pixel shader\n\t%s", Message.c_str());
						return false;
					}

					if (D3DDevice->CreatePixelShader((DWORD*)Blob->GetBufferPointer(), Blob->GetBufferSize(), nullptr, &DirectRenderer.PixelShader) != S_OK)
					{
						ReleaseCom(Blob);
						TH_ERROR("couldn't create pixel shader");
						return false;
					}

					ReleaseCom(Blob);
				}

				if (!DirectRenderer.ConstantBuffer)
				{
					CreateConstantBuffer(&DirectRenderer.ConstantBuffer, sizeof(Direct));
					if (!DirectRenderer.ConstantBuffer)
					{
						TH_ERROR("couldn't create vertex constant buffer");
						return false;
					}
				}

				return true;
			}
			bool D3D11Device::CreateTexture2D(Texture2D* Resource, DXGI_FORMAT InternalFormat)
			{
				D3D11Texture2D* IResource = (D3D11Texture2D*)Resource;
				if (!IResource || !IResource->View)
					return false;

				if (IResource->Resource != nullptr)
					return true;

				D3D11_TEXTURE2D_DESC Description;
				IResource->View->GetDesc(&Description);
				IResource->FormatMode = (Format)Description.Format;
				IResource->Usage = (ResourceUsage)Description.Usage;
				IResource->Width = Description.Width;
				IResource->Height = Description.Height;
				IResource->MipLevels = Description.MipLevels;
				IResource->AccessFlags = (CPUAccess)Description.CPUAccessFlags;

				D3D11_SHADER_RESOURCE_VIEW_DESC SRV;
				ZeroMemory(&SRV, sizeof(SRV));
	
				if (InternalFormat == DXGI_FORMAT_UNKNOWN)
					SRV.Format = Description.Format;
				else
					SRV.Format = InternalFormat;

				if (Description.ArraySize > 1)
				{
					if (Description.MiscFlags & D3D11_RESOURCE_MISC_TEXTURECUBE)
					{
						SRV.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
						SRV.TextureCube.MostDetailedMip = 0;
						SRV.TextureCube.MipLevels = Description.MipLevels;
					}
					else if (Description.SampleDesc.Count <= 1)
					{
						SRV.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
						SRV.Texture2DArray.MostDetailedMip = 0;
						SRV.Texture2DArray.MipLevels = Description.MipLevels;
						SRV.Texture2DArray.FirstArraySlice = 0;
						SRV.Texture2DArray.ArraySize = Description.ArraySize;
					}
					else
					{
						SRV.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMSARRAY;
						SRV.Texture2DMSArray.FirstArraySlice = 0;
						SRV.Texture2DMSArray.ArraySize = Description.ArraySize;
					}
				}
				else if (Description.SampleDesc.Count <= 1)
				{
					SRV.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
					SRV.Texture2D.MostDetailedMip = 0;
					SRV.Texture2D.MipLevels = Description.MipLevels;
				}
				else
					SRV.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMS;

				ReleaseCom(IResource->Resource);
				if (D3DDevice->CreateShaderResourceView(IResource->View, &SRV, &IResource->Resource) == S_OK)
					return true;

				TH_ERROR("could not generate texture 2d resource");
				return false;
			}
			ID3D11InputLayout* D3D11Device::GenerateInputLayout(D3D11Shader* Shader)
			{
				if (!Shader)
					return nullptr;

				if (Shader->VertexLayout != nullptr)
					return Shader->VertexLayout;

				if (!Shader->Signature || !Layout || Layout->Layout.empty())
					return nullptr;

				D3D11_INPUT_ELEMENT_DESC* Result = (D3D11_INPUT_ELEMENT_DESC*)TH_MALLOC(sizeof(D3D11_INPUT_ELEMENT_DESC) * Layout->Layout.size());
				for (size_t i = 0; i < Layout->Layout.size(); i++)
				{
					const InputLayout::Attribute& It = Layout->Layout[i];
					Result[i].SemanticName = It.SemanticName;
					Result[i].AlignedByteOffset = It.AlignedByteOffset;
					Result[i].Format = DXGI_FORMAT_R32_FLOAT;
					Result[i].SemanticIndex = It.SemanticIndex;
					Result[i].InputSlot = Result[i].InstanceDataStepRate = 0;
					Result[i].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;

					switch (It.Format)
					{
						case Tomahawk::Graphics::AttributeType_Byte:
							if (It.Components == 3)
							{
								TH_ERROR("D3D11 does not support 24bit format for this type");
								TH_FREE(Result);
								return nullptr;
							}
							else if (It.Components == 1)
								Result[i].Format = DXGI_FORMAT_R8_SNORM;
							else if (It.Components == 2)
								Result[i].Format = DXGI_FORMAT_R8G8_SNORM;
							else if (It.Components == 4)
								Result[i].Format = DXGI_FORMAT_R8G8B8A8_SNORM;
							break;
						case Tomahawk::Graphics::AttributeType_Ubyte:
							if (It.Components == 3)
							{
								TH_ERROR("D3D11 does not support 24bit format for this type");
								TH_FREE(Result);
								return nullptr;
							}
							else if (It.Components == 1)
								Result[i].Format = DXGI_FORMAT_R8_UNORM;
							else if (It.Components == 2)
								Result[i].Format = DXGI_FORMAT_R8G8_UNORM;
							else if (It.Components == 4)
								Result[i].Format = DXGI_FORMAT_R8G8B8A8_UNORM;
							break;
						case Tomahawk::Graphics::AttributeType_Half:
							if (It.Components == 1)
								Result[i].Format = DXGI_FORMAT_R16_FLOAT;
							else if (It.Components == 2)
								Result[i].Format = DXGI_FORMAT_R16G16_FLOAT;
							else if (It.Components == 3)
								Result[i].Format = DXGI_FORMAT_R11G11B10_FLOAT;
							else if (It.Components == 4)
								Result[i].Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
							break;
						case Tomahawk::Graphics::AttributeType_Float:
							if (It.Components == 1)
								Result[i].Format = DXGI_FORMAT_R32_FLOAT;
							else if (It.Components == 2)
								Result[i].Format = DXGI_FORMAT_R32G32_FLOAT;
							else if (It.Components == 3)
								Result[i].Format = DXGI_FORMAT_R32G32B32_FLOAT;
							else if (It.Components == 4)
								Result[i].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
							break;
						case Tomahawk::Graphics::AttributeType_Int:
							if (It.Components == 1)
								Result[i].Format = DXGI_FORMAT_R32_SINT;
							else if (It.Components == 2)
								Result[i].Format = DXGI_FORMAT_R32G32_SINT;
							else if (It.Components == 3)
								Result[i].Format = DXGI_FORMAT_R32G32B32_SINT;
							else if (It.Components == 4)
								Result[i].Format = DXGI_FORMAT_R32G32B32A32_SINT;
							break;
						case Tomahawk::Graphics::AttributeType_Uint:
							if (It.Components == 1)
								Result[i].Format = DXGI_FORMAT_R32_UINT;
							else if (It.Components == 2)
								Result[i].Format = DXGI_FORMAT_R32G32_UINT;
							else if (It.Components == 3)
								Result[i].Format = DXGI_FORMAT_R32G32B32_UINT;
							else if (It.Components == 4)
								Result[i].Format = DXGI_FORMAT_R32G32B32A32_UINT;
							break;
						default:
							break;
					}
				}

				if (D3DDevice->CreateInputLayout(Result, Layout->Layout.size(), Shader->Signature->GetBufferPointer(), Shader->Signature->GetBufferSize(), &Shader->VertexLayout) != S_OK)
					TH_ERROR("couldn't generate input layout for specified shader");

				TH_FREE(Result);
				return Shader->VertexLayout;
			}
			int D3D11Device::CreateConstantBuffer(ID3D11Buffer** fBuffer, size_t Size)
			{
				D3D11_BUFFER_DESC Description;
				ZeroMemory(&Description, sizeof(Description));
				Description.Usage = D3D11_USAGE_DEFAULT;
				Description.ByteWidth = Size;
				Description.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
				Description.CPUAccessFlags = 0;

				return D3DDevice->CreateBuffer(&Description, nullptr, fBuffer);
			}
			char* D3D11Device::GetCompileState(ID3DBlob* Error)
			{
				if (!Error)
					return nullptr;

				return (char*)Error->GetBufferPointer();
			}
			char* D3D11Device::GetVSProfile()
			{
				return (char*)VSP;
			}
			char* D3D11Device::GetPSProfile()
			{
				return (char*)PSP;
			}
			char* D3D11Device::GetGSProfile()
			{
				return (char*)GSP;
			}
			char* D3D11Device::GetHSProfile()
			{
				return (char*)HSP;
			}
			char* D3D11Device::GetCSProfile()
			{
				return (char*)CSP;
			}
			char* D3D11Device::GetDSProfile()
			{
				return (char*)DSP;
			}
		}
	}
}
#endif