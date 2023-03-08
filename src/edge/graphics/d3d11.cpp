#include "d3d11.h"
#ifdef ED_HAS_SDL2
#include <SDL2/SDL_syswm.h>
#endif
#ifdef ED_MICROSOFT
#define REG_EXCHANGE(Name, Value) { if (Register.Name == Value) return; Register.Name = Value; }
#define REG_EXCHANGE_T2(Name, Value1, Value2) { if (std::get<0>(Register.Name) == Value1 && std::get<1>(Register.Name) == Value2) return; Register.Name = std::make_tuple(Value1, Value2); }
#define REG_EXCHANGE_T3(Name, Value1, Value2, Value3) { if (std::get<0>(Register.Name) == Value1 && std::get<1>(Register.Name) == Value2 && std::get<2>(Register.Name) == Value3) return; Register.Name = std::make_tuple(Value1, Value2, Value3); }
#define REG_EXCHANGE_RS(Name, Value1, Value2, Value3) { auto& __vregrs = Register.Name[Value2]; if (__vregrs.first == Value1 && __vregrs.second == Value3) return; __vregrs = std::make_pair(Value1, Value3); }
#define D3D_RELEASE(Value) { if (Value != nullptr) { Value->Release(); Value = nullptr; } }
#define D3D_INLINE(Code) #Code

namespace
{
	static DXGI_FORMAT GetNonDepthFormat(Edge::Graphics::Format Format)
	{
		switch (Format)
		{
			case Edge::Graphics::Format::D32_Float:
				return DXGI_FORMAT_R32_FLOAT;
			case Edge::Graphics::Format::D16_Unorm:
				return DXGI_FORMAT_R16_UNORM;
			case Edge::Graphics::Format::D24_Unorm_S8_Uint:
				return DXGI_FORMAT_R24G8_TYPELESS;
			default:
				return (DXGI_FORMAT)Format;
		}
	}
	static DXGI_FORMAT GetBaseDepthFormat(Edge::Graphics::Format Format)
	{
		switch (Format)
		{
			case Edge::Graphics::Format::R32_Float:
			case Edge::Graphics::Format::D32_Float:
				return DXGI_FORMAT_R32_TYPELESS;
			case Edge::Graphics::Format::R16_Float:
			case Edge::Graphics::Format::D16_Unorm:
				return DXGI_FORMAT_R16_TYPELESS;
			case Edge::Graphics::Format::D24_Unorm_S8_Uint:
				return DXGI_FORMAT_R24G8_TYPELESS;
			default:
				return (DXGI_FORMAT)Format;
		}
	}
	static DXGI_FORMAT GetInternalDepthFormat(Edge::Graphics::Format Format)
	{
		switch (Format)
		{
			case Edge::Graphics::Format::R32_Float:
			case Edge::Graphics::Format::D32_Float:
				return DXGI_FORMAT_D32_FLOAT;
			case Edge::Graphics::Format::R16_Float:
			case Edge::Graphics::Format::D16_Unorm:
				return DXGI_FORMAT_D16_UNORM;
			case Edge::Graphics::Format::D24_Unorm_S8_Uint:
				return DXGI_FORMAT_D24_UNORM_S8_UINT;
			default:
				return (DXGI_FORMAT)Format;
		}
	}
	static DXGI_FORMAT GetDepthFormat(Edge::Graphics::Format Format)
	{
		switch (Format)
		{
			case Edge::Graphics::Format::R32_Float:
			case Edge::Graphics::Format::D32_Float:
				return DXGI_FORMAT_R32_FLOAT;
			case Edge::Graphics::Format::R16_Float:
				return DXGI_FORMAT_R16_FLOAT;
			case Edge::Graphics::Format::D16_Unorm:
				return DXGI_FORMAT_R16_UNORM;
			case Edge::Graphics::Format::D24_Unorm_S8_Uint:
				return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
			default:
				return (DXGI_FORMAT)Format;
		}
	}
}

namespace Edge
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
				D3D_RELEASE(Resource);
			}
			void* D3D11DepthStencilState::GetResource() const
			{
				return Resource;
			}

			D3D11RasterizerState::D3D11RasterizerState(const Desc& I) : RasterizerState(I), Resource(nullptr)
			{
			}
			D3D11RasterizerState::~D3D11RasterizerState()
			{
				D3D_RELEASE(Resource);
			}
			void* D3D11RasterizerState::GetResource() const
			{
				return Resource;
			}

			D3D11BlendState::D3D11BlendState(const Desc& I) : BlendState(I), Resource(nullptr)
			{
			}
			D3D11BlendState::~D3D11BlendState()
			{
				D3D_RELEASE(Resource);
			}
			void* D3D11BlendState::GetResource() const
			{
				return Resource;
			}

			D3D11SamplerState::D3D11SamplerState(const Desc& I) : SamplerState(I), Resource(nullptr)
			{
			}
			D3D11SamplerState::~D3D11SamplerState()
			{
				D3D_RELEASE(Resource);
			}
			void* D3D11SamplerState::GetResource() const
			{
				return Resource;
			}

			D3D11InputLayout::D3D11InputLayout(const Desc& I) : InputLayout(I)
			{
			}
			D3D11InputLayout::~D3D11InputLayout()
			{
			}
			void* D3D11InputLayout::GetResource() const
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
				D3D_RELEASE(ConstantBuffer);
				D3D_RELEASE(VertexShader);
				D3D_RELEASE(PixelShader);
				D3D_RELEASE(GeometryShader);
				D3D_RELEASE(DomainShader);
				D3D_RELEASE(HullShader);
				D3D_RELEASE(ComputeShader);
				D3D_RELEASE(VertexLayout);
				D3D_RELEASE(Signature);
			}
			bool D3D11Shader::IsValid() const
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
				D3D_RELEASE(Resource);
				D3D_RELEASE(Element);
				D3D_RELEASE(Access);
			}
			void* D3D11ElementBuffer::GetResource() const
			{
				return (void*)Element;
			}

			D3D11MeshBuffer::D3D11MeshBuffer(const Desc& I) : MeshBuffer(I)
			{
			}
			Compute::Vertex* D3D11MeshBuffer::GetElements(GraphicsDevice* Device) const
			{
				ED_ASSERT(Device != nullptr, nullptr, "graphics device should be set");

				MappedSubresource Resource;
				Device->Map(VertexBuffer, ResourceMap::Write, &Resource);

				Compute::Vertex* Vertices = ED_MALLOC(Compute::Vertex, sizeof(Compute::Vertex) * (unsigned int)VertexBuffer->GetElements());
				memcpy(Vertices, Resource.Pointer, (size_t)VertexBuffer->GetElements() * sizeof(Compute::Vertex));

				Device->Unmap(VertexBuffer, &Resource);
				return Vertices;
			}

			D3D11SkinMeshBuffer::D3D11SkinMeshBuffer(const Desc& I) : SkinMeshBuffer(I)
			{
			}
			Compute::SkinVertex* D3D11SkinMeshBuffer::GetElements(GraphicsDevice* Device) const
			{
				ED_ASSERT(Device != nullptr, nullptr, "graphics device should be set");

				MappedSubresource Resource;
				Device->Map(VertexBuffer, ResourceMap::Write, &Resource);

				Compute::SkinVertex* Vertices = ED_MALLOC(Compute::SkinVertex, sizeof(Compute::SkinVertex) * (unsigned int)VertexBuffer->GetElements());
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

				D3D_RELEASE(Resource);
			}

			D3D11Texture2D::D3D11Texture2D() : Texture2D(), Access(nullptr), Resource(nullptr), View(nullptr)
			{
			}
			D3D11Texture2D::D3D11Texture2D(const Desc& I) : Texture2D(I), Access(nullptr), Resource(nullptr), View(nullptr)
			{
			}
			D3D11Texture2D::~D3D11Texture2D()
			{
				D3D_RELEASE(View);
				D3D_RELEASE(Resource);
				D3D_RELEASE(Access);
			}
			void* D3D11Texture2D::GetResource() const
			{
				return (void*)Resource;
			}

			D3D11Texture3D::D3D11Texture3D() : Texture3D(), Access(nullptr), Resource(nullptr), View(nullptr)
			{
			}
			D3D11Texture3D::~D3D11Texture3D()
			{
				D3D_RELEASE(View);
				D3D_RELEASE(Resource);
				D3D_RELEASE(Access);
			}
			void* D3D11Texture3D::GetResource()
			{
				return (void*)Resource;
			}

			D3D11TextureCube::D3D11TextureCube() : TextureCube(), Access(nullptr), Resource(nullptr), View(nullptr)
			{
			}
			D3D11TextureCube::D3D11TextureCube(const Desc& I) : TextureCube(I), Access(nullptr), Resource(nullptr), View(nullptr)
			{
			}
			D3D11TextureCube::~D3D11TextureCube()
			{
				D3D_RELEASE(View);
				D3D_RELEASE(Resource);
				D3D_RELEASE(Access);
			}
			void* D3D11TextureCube::GetResource() const
			{
				return (void*)Resource;
			}

			D3D11DepthTarget2D::D3D11DepthTarget2D(const Desc& I) : DepthTarget2D(I)
			{
				DepthStencilView = nullptr;
			}
			D3D11DepthTarget2D::~D3D11DepthTarget2D()
			{
				D3D_RELEASE(DepthStencilView);
			}
			void* D3D11DepthTarget2D::GetResource() const
			{
				return DepthStencilView;
			}
			uint32_t D3D11DepthTarget2D::GetWidth() const
			{
				return (uint32_t)Viewarea.Width;
			}
			uint32_t D3D11DepthTarget2D::GetHeight() const
			{
				return (uint32_t)Viewarea.Height;
			}

			D3D11DepthTargetCube::D3D11DepthTargetCube(const Desc& I) : DepthTargetCube(I)
			{
				DepthStencilView = nullptr;
			}
			D3D11DepthTargetCube::~D3D11DepthTargetCube()
			{
				D3D_RELEASE(DepthStencilView);
			}
			void* D3D11DepthTargetCube::GetResource() const
			{
				return DepthStencilView;
			}
			uint32_t D3D11DepthTargetCube::GetWidth() const
			{
				return (uint32_t)Viewarea.Width;
			}
			uint32_t D3D11DepthTargetCube::GetHeight() const
			{
				return (uint32_t)Viewarea.Height;
			}

			D3D11RenderTarget2D::D3D11RenderTarget2D(const Desc& I) : RenderTarget2D(I)
			{
				RenderTargetView = nullptr;
				DepthStencilView = nullptr;
				Texture = nullptr;
			}
			D3D11RenderTarget2D::~D3D11RenderTarget2D()
			{
				D3D_RELEASE(Texture);
				D3D_RELEASE(DepthStencilView);
				D3D_RELEASE(RenderTargetView);
			}
			void* D3D11RenderTarget2D::GetTargetBuffer() const
			{
				return (void*)&RenderTargetView;
			}
			void* D3D11RenderTarget2D::GetDepthBuffer() const
			{
				return (void*)DepthStencilView;
			}
			uint32_t D3D11RenderTarget2D::GetWidth() const
			{
				return (uint32_t)Viewarea.Width;
			}
			uint32_t D3D11RenderTarget2D::GetHeight() const
			{
				return (uint32_t)Viewarea.Height;
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
				D3D_RELEASE(DepthStencilView);
				for (unsigned int i = 0; i < 8; i++)
				{
					D3D_RELEASE(Texture[i]);
					D3D_RELEASE(RenderTargetView[i]);
				}
			}
			void* D3D11MultiRenderTarget2D::GetTargetBuffer() const
			{
				return (void*)RenderTargetView;
			}
			void* D3D11MultiRenderTarget2D::GetDepthBuffer() const
			{
				return (void*)DepthStencilView;
			}
			uint32_t D3D11MultiRenderTarget2D::GetWidth() const
			{
				return (uint32_t)Viewarea.Width;
			}
			uint32_t D3D11MultiRenderTarget2D::GetHeight() const
			{
				return (uint32_t)Viewarea.Height;
			}

			D3D11RenderTargetCube::D3D11RenderTargetCube(const Desc& I) : RenderTargetCube(I)
			{
				DepthStencilView = nullptr;
				RenderTargetView = nullptr;
				Texture = nullptr;
			}
			D3D11RenderTargetCube::~D3D11RenderTargetCube()
			{
				D3D_RELEASE(DepthStencilView);
				D3D_RELEASE(RenderTargetView);
				D3D_RELEASE(Texture);
			}
			void* D3D11RenderTargetCube::GetTargetBuffer() const
			{
				return (void*)&RenderTargetView;
			}
			void* D3D11RenderTargetCube::GetDepthBuffer() const
			{
				return (void*)DepthStencilView;
			}
			uint32_t D3D11RenderTargetCube::GetWidth() const
			{
				return (uint32_t)Viewarea.Width;
			}
			uint32_t D3D11RenderTargetCube::GetHeight() const
			{
				return (uint32_t)Viewarea.Height;
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
				ED_ASSERT_V((unsigned int)Target <= 8, "targets count should be less than 9");
				for (unsigned int i = 0; i < (unsigned int)Target; i++)
				{
					D3D_RELEASE(RenderTargetView[i]);
					D3D_RELEASE(Texture[i]);
				}
				D3D_RELEASE(DepthStencilView);
			}
			void* D3D11MultiRenderTargetCube::GetTargetBuffer() const
			{
				return (void*)RenderTargetView;
			}
			void* D3D11MultiRenderTargetCube::GetDepthBuffer() const
			{
				return (void*)DepthStencilView;
			}
			uint32_t D3D11MultiRenderTargetCube::GetWidth() const
			{
				return (uint32_t)Viewarea.Width;
			}
			uint32_t D3D11MultiRenderTargetCube::GetHeight() const
			{
				return (uint32_t)Viewarea.Height;
			}

			D3D11Cubemap::D3D11Cubemap(const Desc& I) : Cubemap(I), Merger(nullptr), Source(nullptr)
			{
				ED_ASSERT_V(I.Source != nullptr, "source should be set");
				ED_ASSERT_V(I.Target < I.Source->GetTargetCount(), "targets count should be less than %i", (int)I.Source->GetTargetCount());

				D3D11Texture2D* Target = (D3D11Texture2D*)I.Source->GetTarget2D(I.Target);
				ED_ASSERT_V(Target != nullptr && Target->View != nullptr, "render target should be valid");

				Source = Target->View;
				Source->GetDesc(&Options.Texture);
				Source->AddRef();

				D3D11_TEXTURE2D_DESC& Texture = Options.Texture;
				Texture.MipLevels = I.MipLevels;
				Texture.ArraySize = 6;
				Texture.Usage = D3D11_USAGE_DEFAULT;
				Texture.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
				Texture.CPUAccessFlags = 0;
				Texture.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE | D3D11_RESOURCE_MISC_GENERATE_MIPS;

				D3D11_SHADER_RESOURCE_VIEW_DESC& Resource = Options.Resource;
				Resource.Format = Texture.Format;
				Resource.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
				Resource.TextureCube.MostDetailedMip = 0;
				Resource.TextureCube.MipLevels = I.MipLevels;

				D3D11_BOX& Region = Options.Region;
				Region = { 0, 0, 0, (unsigned int)I.Size, (unsigned int)I.Size, 1 };
			}
			D3D11Cubemap::~D3D11Cubemap()
			{
				D3D_RELEASE(Source);
				D3D_RELEASE(Merger);
			}

			D3D11Query::D3D11Query() : Query(), Async(nullptr)
			{
			}
			D3D11Query::~D3D11Query()
			{
				if (Async != nullptr)
					Async->Release();
			}
			void* D3D11Query::GetResource() const
			{
				return (void*)Async;
			}

			D3D11Device::D3D11Device(const Desc& I) : GraphicsDevice(I), ImmediateContext(nullptr), Context(nullptr), SwapChain(nullptr), FeatureLevel(D3D_FEATURE_LEVEL_11_0), DriverType(D3D_DRIVER_TYPE_HARDWARE)
			{
				if (I.Window != nullptr && !I.Window->GetHandle())
				{
					I.Window->BuildLayer(Backend);
					if (!I.Window->GetHandle())
						return;
				}

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
				SwapChainResource.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
				SwapChainResource.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
				SwapChainResource.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
				SwapChainResource.SampleDesc.Count = 1;
				SwapChainResource.SampleDesc.Quality = 0;
				SwapChainResource.Windowed = I.IsWindowed;
				SwapChainResource.Flags = 0;
				SwapChainResource.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
#if defined(ED_MICROSOFT) && defined(ED_HAS_SDL2)
				if (I.Window != nullptr)
				{
					SDL_SysWMinfo Info;
					I.Window->Load(&Info);
					SwapChainResource.OutputWindow = (HWND)Info.info.win.window;
				}
#endif
				if (D3D11CreateDeviceAndSwapChain(nullptr, DriverType, nullptr, CreationFlags, FeatureLevels, ARRAYSIZE(FeatureLevels), D3D11_SDK_VERSION, &SwapChainResource, &SwapChain, &Context, &FeatureLevel, &ImmediateContext) != S_OK)
				{
					ED_ERR("[d3d11] couldn't create swap chain, device or immediate context");
					return;
				}

				CreateConstantBuffer(&ConstantBuffer[0], sizeof(AnimationBuffer));
				Constants[0] = &Animation;

				CreateConstantBuffer(&ConstantBuffer[1], sizeof(RenderBuffer));
				Constants[1] = &Render;

				CreateConstantBuffer(&ConstantBuffer[2], sizeof(ViewBuffer));
				Constants[2] = &View;

				SetConstantBuffers();
				SetShaderModel(I.ShaderMode == ShaderModel::Auto ? GetSupportedShaderModel() : I.ShaderMode);
				SetPrimitiveTopology(PrimitiveTopology::Triangle_List);
				ResizeBuffers(I.BufferWidth, I.BufferHeight);
				CreateStates();

				Shader::Desc F = Shader::Desc();
				if (GetSection("geometry/basic/geometry", &F))
					BasicEffect = CreateShader(F);
			}
			D3D11Device::~D3D11Device()
			{
				ReleaseProxy();
				D3D_RELEASE(Immediate.VertexShader);
				D3D_RELEASE(Immediate.VertexLayout);
				D3D_RELEASE(Immediate.ConstantBuffer);
				D3D_RELEASE(Immediate.PixelShader);
				D3D_RELEASE(Immediate.VertexBuffer);
				D3D_RELEASE(ConstantBuffer[0]);
				D3D_RELEASE(ConstantBuffer[1]);
				D3D_RELEASE(ConstantBuffer[2]);
				D3D_RELEASE(ImmediateContext);
				D3D_RELEASE(SwapChain);

				if (Debug)
				{
					ID3D11Debug* Debugger = nullptr;
					Context->QueryInterface(__uuidof(ID3D11Debug), reinterpret_cast<void**>(&Debugger));

					if (Debugger != nullptr)
					{
						D3D11_RLDO_FLAGS Flags = (D3D11_RLDO_FLAGS)(D3D11_RLDO_DETAIL | 0x4); // D3D11_RLDO_IGNORE_INTERNAL
						Debugger->ReportLiveDeviceObjects(Flags);
						Debugger->Release();
					}
				}

				D3D_RELEASE(Context);
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
				ShaderGen = Model;
				if (ShaderGen == ShaderModel::HLSL_1_0)
				{
					VSP = "vs_1_0";
					PSP = "ps_1_0";
					GSP = "gs_1_0";
					CSP = "cs_1_0";
					DSP = "ds_1_0";
					HSP = "hs_1_0";
				}
				else if (ShaderGen == ShaderModel::HLSL_2_0)
				{
					VSP = "vs_2_0";
					PSP = "ps_2_0";
					GSP = "gs_2_0";
					CSP = "cs_2_0";
					DSP = "ds_2_0";
					HSP = "hs_2_0";
				}
				else if (ShaderGen == ShaderModel::HLSL_3_0)
				{
					VSP = "vs_3_0";
					PSP = "ps_3_0";
					GSP = "gs_3_0";
					CSP = "cs_3_0";
					DSP = "ds_3_0";
					HSP = "hs_3_0";
				}
				else if (ShaderGen == ShaderModel::HLSL_4_0)
				{
					VSP = "vs_4_0";
					PSP = "ps_4_0";
					GSP = "gs_4_0";
					CSP = "cs_4_0";
					DSP = "ds_4_0";
					HSP = "hs_4_0";
				}
				else if (ShaderGen == ShaderModel::HLSL_4_1)
				{
					VSP = "vs_4_1";
					PSP = "ps_4_1";
					GSP = "gs_4_1";
					CSP = "cs_4_1";
					DSP = "ds_4_1";
					HSP = "hs_4_1";
				}
				else if (ShaderGen == ShaderModel::HLSL_5_0)
				{
					VSP = "vs_5_0";
					PSP = "ps_5_0";
					GSP = "gs_5_0";
					CSP = "cs_5_0";
					DSP = "ds_5_0";
					HSP = "hs_5_0";
				}
				else
					SetShaderModel(ShaderModel::HLSL_4_0);
			}
			void D3D11Device::SetBlendState(BlendState* State)
			{
				ID3D11BlendState* NewState = (ID3D11BlendState*)(State ? State->GetResource() : nullptr);
				REG_EXCHANGE(Blend, NewState);
				ImmediateContext->OMSetBlendState(NewState, 0, 0xffffffff);
			}
			void D3D11Device::SetRasterizerState(RasterizerState* State)
			{
				ID3D11RasterizerState* NewState = (ID3D11RasterizerState*)(State ? State->GetResource() : nullptr);
				REG_EXCHANGE(Rasterizer, NewState);
				ImmediateContext->RSSetState(NewState);
			}
			void D3D11Device::SetDepthStencilState(DepthStencilState* State)
			{
				ID3D11DepthStencilState* NewState = (ID3D11DepthStencilState*)(State ? State->GetResource() : nullptr);
				REG_EXCHANGE(DepthStencil, NewState);
				ImmediateContext->OMSetDepthStencilState(NewState, 1);
			}
			void D3D11Device::SetInputLayout(InputLayout* Resource)
			{
				Register.Layout = (D3D11InputLayout*)Resource;
			}
			void D3D11Device::SetShader(Shader* Resource, unsigned int Type)
			{
				D3D11Shader* IResource = (D3D11Shader*)Resource;
				bool Flush = (!IResource), Update = false;

				if (Type & (uint32_t)ShaderType::Vertex)
				{
					auto& Item = Register.Shaders[0];
					if (Item != IResource)
					{
						ImmediateContext->VSSetShader(Flush ? nullptr : IResource->VertexShader, nullptr, 0);
						Item = IResource;
						Update = true;
					}
				}

				if (Type & (uint32_t)ShaderType::Pixel)
				{
					auto& Item = Register.Shaders[1];
					if (Item != IResource)
					{
						ImmediateContext->PSSetShader(Flush ? nullptr : IResource->PixelShader, nullptr, 0);
						Item = IResource;
						Update = true;
					}
				}

				if (Type & (uint32_t)ShaderType::Geometry)
				{
					auto& Item = Register.Shaders[2];
					if (Item != IResource)
					{
						ImmediateContext->GSSetShader(Flush ? nullptr : IResource->GeometryShader, nullptr, 0);
						Item = IResource;
						Update = true;
					}
				}

				if (Type & (uint32_t)ShaderType::Hull)
				{
					auto& Item = Register.Shaders[3];
					if (Item != IResource)
					{
						ImmediateContext->HSSetShader(Flush ? nullptr : IResource->HullShader, nullptr, 0);
						Item = IResource;
						Update = true;
					}
				}

				if (Type & (uint32_t)ShaderType::Domain)
				{
					auto& Item = Register.Shaders[4];
					if (Item != IResource)
					{
						ImmediateContext->DSSetShader(Flush ? nullptr : IResource->DomainShader, nullptr, 0);
						Item = IResource;
						Update = true;
					}
				}

				if (Type & (uint32_t)ShaderType::Compute)
				{
					auto& Item = Register.Shaders[5];
					if (Item != IResource)
					{
						ImmediateContext->CSSetShader(Flush ? nullptr : IResource->ComputeShader, nullptr, 0);
						Item = IResource;
						Update = true;
					}
				}
				
				if (Update)
					ImmediateContext->IASetInputLayout(Flush ? nullptr : GenerateInputLayout(IResource));
			}
			void D3D11Device::SetSamplerState(SamplerState* State, unsigned int Slot, unsigned int Count, unsigned int Type)
			{
				ED_ASSERT_V(Slot < ED_MAX_UNITS, "slot should be less than %i", (int)ED_MAX_UNITS);
				ED_ASSERT_V(Count <= ED_MAX_UNITS && Slot + Count <= ED_MAX_UNITS, "count should be less than or equal %i", (int)ED_MAX_UNITS);

				ID3D11SamplerState* NewState = (ID3D11SamplerState*)(State ? State->GetResource() : nullptr);
				REG_EXCHANGE_T3(Sampler, NewState, Slot, Type);

				if (Type & (uint32_t)ShaderType::Vertex)
					ImmediateContext->VSSetSamplers(Slot, 1, &NewState);

				if (Type & (uint32_t)ShaderType::Pixel)
					ImmediateContext->PSSetSamplers(Slot, 1, &NewState);

				if (Type & (uint32_t)ShaderType::Geometry)
					ImmediateContext->GSSetSamplers(Slot, 1, &NewState);

				if (Type & (uint32_t)ShaderType::Hull)
					ImmediateContext->HSSetSamplers(Slot, 1, &NewState);

				if (Type & (uint32_t)ShaderType::Domain)
					ImmediateContext->DSSetSamplers(Slot, 1, &NewState);

				if (Type & (uint32_t)ShaderType::Compute)
					ImmediateContext->CSSetSamplers(Slot, 1, &NewState);
			}
			void D3D11Device::SetBuffer(Shader* Resource, unsigned int Slot, unsigned int Type)
			{
				ED_ASSERT_V(Slot < ED_MAX_UNITS, "slot should be less than %i", (int)ED_MAX_UNITS);

				ID3D11Buffer* IBuffer = (Resource ? ((D3D11Shader*)Resource)->ConstantBuffer : nullptr);
				if (Type & (uint32_t)ShaderType::Vertex)
					ImmediateContext->VSSetConstantBuffers(Slot, 1, &IBuffer);

				if (Type & (uint32_t)ShaderType::Pixel)
					ImmediateContext->PSSetConstantBuffers(Slot, 1, &IBuffer);

				if (Type & (uint32_t)ShaderType::Geometry)
					ImmediateContext->GSSetConstantBuffers(Slot, 1, &IBuffer);

				if (Type & (uint32_t)ShaderType::Hull)
					ImmediateContext->HSSetConstantBuffers(Slot, 1, &IBuffer);

				if (Type & (uint32_t)ShaderType::Domain)
					ImmediateContext->DSSetConstantBuffers(Slot, 1, &IBuffer);

				if (Type & (uint32_t)ShaderType::Compute)
					ImmediateContext->CSSetConstantBuffers(Slot, 1, &IBuffer);
			}
			void D3D11Device::SetBuffer(InstanceBuffer* Resource, unsigned int Slot, unsigned int Type)
			{
				ED_ASSERT_V(Slot < ED_MAX_UNITS, "slot should be less than %i", (int)ED_MAX_UNITS);

				ID3D11ShaderResourceView* NewState = (Resource ? ((D3D11InstanceBuffer*)Resource)->Resource : nullptr);
				REG_EXCHANGE_RS(Resources, NewState, Slot, Type);

				if (Type & (uint32_t)ShaderType::Vertex)
					ImmediateContext->VSSetShaderResources(Slot, 1, &NewState);

				if (Type & (uint32_t)ShaderType::Pixel)
					ImmediateContext->PSSetShaderResources(Slot, 1, &NewState);

				if (Type & (uint32_t)ShaderType::Geometry)
					ImmediateContext->GSSetShaderResources(Slot, 1, &NewState);

				if (Type & (uint32_t)ShaderType::Hull)
					ImmediateContext->HSSetShaderResources(Slot, 1, &NewState);

				if (Type & (uint32_t)ShaderType::Domain)
					ImmediateContext->DSSetShaderResources(Slot, 1, &NewState);

				if (Type & (uint32_t)ShaderType::Compute)
					ImmediateContext->CSSetShaderResources(Slot, 1, &NewState);
			}
			void D3D11Device::SetStructureBuffer(ElementBuffer* Resource, unsigned int Slot, unsigned int Type)
			{
				ED_ASSERT_V(Slot < ED_MAX_UNITS, "slot should be less than %i", (int)ED_MAX_UNITS);

				ID3D11ShaderResourceView* NewState = (Resource ? ((D3D11ElementBuffer*)Resource)->Resource : nullptr);
				REG_EXCHANGE_RS(Resources, NewState, Slot, Type);

				if (Type & (uint32_t)ShaderType::Vertex)
					ImmediateContext->VSSetShaderResources(Slot, 1, &NewState);

				if (Type & (uint32_t)ShaderType::Pixel)
					ImmediateContext->PSSetShaderResources(Slot, 1, &NewState);

				if (Type & (uint32_t)ShaderType::Geometry)
					ImmediateContext->GSSetShaderResources(Slot, 1, &NewState);

				if (Type & (uint32_t)ShaderType::Hull)
					ImmediateContext->HSSetShaderResources(Slot, 1, &NewState);

				if (Type & (uint32_t)ShaderType::Domain)
					ImmediateContext->DSSetShaderResources(Slot, 1, &NewState);

				if (Type & (uint32_t)ShaderType::Compute)
					ImmediateContext->CSSetShaderResources(Slot, 1, &NewState);
			}
			void D3D11Device::SetTexture2D(Texture2D* Resource, unsigned int Slot, unsigned int Type)
			{
				ED_ASSERT_V(Slot < ED_MAX_UNITS, "slot should be less than %i", (int)ED_MAX_UNITS);

				ID3D11ShaderResourceView* NewState = (Resource ? ((D3D11Texture2D*)Resource)->Resource : nullptr);
				REG_EXCHANGE_RS(Resources, NewState, Slot, Type);

				if (Type & (uint32_t)ShaderType::Vertex)
					ImmediateContext->VSSetShaderResources(Slot, 1, &NewState);

				if (Type & (uint32_t)ShaderType::Pixel)
					ImmediateContext->PSSetShaderResources(Slot, 1, &NewState);

				if (Type & (uint32_t)ShaderType::Geometry)
					ImmediateContext->GSSetShaderResources(Slot, 1, &NewState);

				if (Type & (uint32_t)ShaderType::Hull)
					ImmediateContext->HSSetShaderResources(Slot, 1, &NewState);

				if (Type & (uint32_t)ShaderType::Domain)
					ImmediateContext->DSSetShaderResources(Slot, 1, &NewState);

				if (Type & (uint32_t)ShaderType::Compute)
					ImmediateContext->CSSetShaderResources(Slot, 1, &NewState);
			}
			void D3D11Device::SetTexture3D(Texture3D* Resource, unsigned int Slot, unsigned int Type)
			{
				ED_ASSERT_V(Slot < ED_MAX_UNITS, "slot should be less than %i", (int)ED_MAX_UNITS);

				ID3D11ShaderResourceView* NewState = (Resource ? ((D3D11Texture3D*)Resource)->Resource : nullptr);
				REG_EXCHANGE_RS(Resources, NewState, Slot, Type);

				if (Type & (uint32_t)ShaderType::Vertex)
					ImmediateContext->VSSetShaderResources(Slot, 1, &NewState);

				if (Type & (uint32_t)ShaderType::Pixel)
					ImmediateContext->PSSetShaderResources(Slot, 1, &NewState);

				if (Type & (uint32_t)ShaderType::Geometry)
					ImmediateContext->GSSetShaderResources(Slot, 1, &NewState);

				if (Type & (uint32_t)ShaderType::Hull)
					ImmediateContext->HSSetShaderResources(Slot, 1, &NewState);

				if (Type & (uint32_t)ShaderType::Domain)
					ImmediateContext->DSSetShaderResources(Slot, 1, &NewState);

				if (Type & (uint32_t)ShaderType::Compute)
					ImmediateContext->CSSetShaderResources(Slot, 1, &NewState);
			}
			void D3D11Device::SetTextureCube(TextureCube* Resource, unsigned int Slot, unsigned int Type)
			{
				ED_ASSERT_V(Slot < ED_MAX_UNITS, "slot should be less than %i", (int)ED_MAX_UNITS);

				ID3D11ShaderResourceView* NewState = (Resource ? ((D3D11TextureCube*)Resource)->Resource : nullptr);
				REG_EXCHANGE_RS(Resources, NewState, Slot, Type);

				if (Type & (uint32_t)ShaderType::Vertex)
					ImmediateContext->VSSetShaderResources(Slot, 1, &NewState);

				if (Type & (uint32_t)ShaderType::Pixel)
					ImmediateContext->PSSetShaderResources(Slot, 1, &NewState);

				if (Type & (uint32_t)ShaderType::Geometry)
					ImmediateContext->GSSetShaderResources(Slot, 1, &NewState);

				if (Type & (uint32_t)ShaderType::Hull)
					ImmediateContext->HSSetShaderResources(Slot, 1, &NewState);

				if (Type & (uint32_t)ShaderType::Domain)
					ImmediateContext->DSSetShaderResources(Slot, 1, &NewState);

				if (Type & (uint32_t)ShaderType::Compute)
					ImmediateContext->CSSetShaderResources(Slot, 1, &NewState);
			}
			void D3D11Device::SetIndexBuffer(ElementBuffer* Resource, Format FormatMode)
			{
				D3D11ElementBuffer* IResource = (D3D11ElementBuffer*)Resource;
				REG_EXCHANGE_T2(IndexBuffer, IResource, FormatMode);
				ImmediateContext->IASetIndexBuffer(IResource ? IResource->Element : nullptr, (DXGI_FORMAT)FormatMode, 0);
			}
			void D3D11Device::SetVertexBuffers(ElementBuffer** Resources, unsigned int Count, bool)
			{
				ED_ASSERT_V(Resources != nullptr || !Count, "invalid vertex buffer array pointer");
				ED_ASSERT_V(Count <= ED_MAX_UNITS, "slot should be less than or equal to %i", (int)ED_MAX_UNITS);

				static ID3D11Buffer* IBuffers[ED_MAX_UNITS] = { nullptr };
				static unsigned int Strides[ED_MAX_UNITS] = { 0 };
				static unsigned int Offsets[ED_MAX_UNITS] = { 0 };

				for (unsigned int i = 0; i < Count; i++)
				{
					D3D11ElementBuffer* IResource = (D3D11ElementBuffer*)Resources[i];
					IBuffers[i] = (IResource ? IResource->Element : nullptr);
					Strides[i] = (unsigned int)(IResource ? IResource->Stride : 0);
					REG_EXCHANGE_RS(VertexBuffers, IResource, i, i);
				}

				ImmediateContext->IASetVertexBuffers(0, Count, IBuffers, Strides, Offsets);
			}
			void D3D11Device::SetWriteable(ElementBuffer** Resource, unsigned int Slot, unsigned int Count, bool Computable)
			{
				ED_ASSERT_V(Slot < 8, "slot should be less than 8");
				ED_ASSERT_V(Count <= 8 && Slot + Count <= 8, "count should be less than or equal 8");
				ED_ASSERT_V(Resource != nullptr, "buffers ptr should be set");

				ID3D11UnorderedAccessView* Array[8] = { nullptr };
				for (unsigned int i = 0; i < Count; i++)
					Array[i] = (Resource[i] ? ((D3D11ElementBuffer*)(Resource[i]))->Access : nullptr);

				UINT Offset = 0;
				if (!Computable)
					ImmediateContext->OMSetRenderTargetsAndUnorderedAccessViews(D3D11_KEEP_RENDER_TARGETS_AND_DEPTH_STENCIL, nullptr, nullptr, Slot, Count, Array, &Offset);
				else
					ImmediateContext->CSSetUnorderedAccessViews(Slot, Count, Array, &Offset);
			}
			void D3D11Device::SetWriteable(Texture2D** Resource, unsigned int Slot, unsigned int Count, bool Computable)
			{
				ED_ASSERT_V(Slot < 8, "slot should be less than 8");
				ED_ASSERT_V(Count <= 8 && Slot + Count <= 8, "count should be less than or equal 8");
				ED_ASSERT_V(Resource != nullptr, "buffers ptr should be set");

				ID3D11UnorderedAccessView* Array[8] = { nullptr };
				for (unsigned int i = 0; i < Count; i++)
					Array[i] = (Resource[i] ? ((D3D11Texture2D*)(Resource[i]))->Access : nullptr);

				UINT Offset = 0;
				if (!Computable)
					ImmediateContext->OMSetRenderTargetsAndUnorderedAccessViews(D3D11_KEEP_RENDER_TARGETS_AND_DEPTH_STENCIL, nullptr, nullptr, Slot, Count, Array, &Offset);
				else
					ImmediateContext->CSSetUnorderedAccessViews(Slot, Count, Array, &Offset);
			}
			void D3D11Device::SetWriteable(Texture3D** Resource, unsigned int Slot, unsigned int Count, bool Computable)
			{
				ED_ASSERT_V(Slot < 8, "slot should be less than 8");
				ED_ASSERT_V(Count <= 8 && Slot + Count <= 8, "count should be less than or equal 8");
				ED_ASSERT_V(Resource != nullptr, "buffers ptr should be set");

				ID3D11UnorderedAccessView* Array[8] = { nullptr };
				for (unsigned int i = 0; i < Count; i++)
					Array[i] = (Resource[i] ? ((D3D11Texture3D*)(Resource[i]))->Access : nullptr);

				UINT Offset = 0;
				if (!Computable)
					ImmediateContext->OMSetRenderTargetsAndUnorderedAccessViews(D3D11_KEEP_RENDER_TARGETS_AND_DEPTH_STENCIL, nullptr, nullptr, Slot, Count, Array, &Offset);
				else
					ImmediateContext->CSSetUnorderedAccessViews(Slot, Count, Array, &Offset);
			}
			void D3D11Device::SetWriteable(TextureCube** Resource, unsigned int Slot, unsigned int Count, bool Computable)
			{
				ED_ASSERT_V(Slot < 8, "slot should be less than 8");
				ED_ASSERT_V(Count <= 8 && Slot + Count <= 8, "count should be less than or equal 8");
				ED_ASSERT_V(Resource != nullptr, "buffers ptr should be set");

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
			void D3D11Device::SetTarget(DepthTarget2D* Resource)
			{
				ED_ASSERT_V(Resource != nullptr, "depth target should be set");
				D3D11DepthTarget2D* IResource = (D3D11DepthTarget2D*)Resource;
				const Viewport& Viewarea = Resource->GetViewport();
				D3D11_VIEWPORT Viewport = { Viewarea.TopLeftX, Viewarea.TopLeftY, Viewarea.Width, Viewarea.Height, Viewarea.MinDepth, Viewarea.MaxDepth };

				ImmediateContext->OMSetRenderTargets(0, nullptr, IResource->DepthStencilView);
				ImmediateContext->RSSetViewports(1, &Viewport);
			}
			void D3D11Device::SetTarget(DepthTargetCube* Resource)
			{
				ED_ASSERT_V(Resource != nullptr, "depth target should be set");
				D3D11DepthTargetCube* IResource = (D3D11DepthTargetCube*)Resource;
				const Viewport& Viewarea = Resource->GetViewport();
				D3D11_VIEWPORT Viewport = { Viewarea.TopLeftX, Viewarea.TopLeftY, Viewarea.Width, Viewarea.Height, Viewarea.MinDepth, Viewarea.MaxDepth };

				ImmediateContext->OMSetRenderTargets(0, nullptr, IResource->DepthStencilView);
				ImmediateContext->RSSetViewports(1, &Viewport);
			}
			void D3D11Device::SetTarget(Graphics::RenderTarget* Resource, unsigned int Target, float R, float G, float B)
			{
				ED_ASSERT_V(Resource != nullptr, "render target should be set");
				ED_ASSERT_V(Target < Resource->GetTargetCount(), "targets count should be less than %i", (int)Resource->GetTargetCount());

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
				ED_ASSERT_V(Resource != nullptr, "render target should be set");
				ED_ASSERT_V(Target < Resource->GetTargetCount(), "targets count should be less than %i", (int)Resource->GetTargetCount());

				const Viewport& Viewarea = Resource->GetViewport();
				ID3D11RenderTargetView** TargetBuffer = (ID3D11RenderTargetView**)Resource->GetTargetBuffer();
				ID3D11DepthStencilView* DepthBuffer = (ID3D11DepthStencilView*)Resource->GetDepthBuffer();
				D3D11_VIEWPORT Viewport = { Viewarea.TopLeftX, Viewarea.TopLeftY, Viewarea.Width, Viewarea.Height, Viewarea.MinDepth, Viewarea.MaxDepth };

				ImmediateContext->RSSetViewports(1, &Viewport);
				ImmediateContext->OMSetRenderTargets(1, &TargetBuffer[Target], DepthBuffer);
			}
			void D3D11Device::SetTarget(Graphics::RenderTarget* Resource, float R, float G, float B)
			{
				ED_ASSERT_V(Resource != nullptr, "render target should be set");

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
				ED_ASSERT_V(Resource != nullptr, "render target should be set");

				const Viewport& Viewarea = Resource->GetViewport();
				ID3D11RenderTargetView** TargetBuffer = (ID3D11RenderTargetView**)Resource->GetTargetBuffer();
				ID3D11DepthStencilView* DepthBuffer = (ID3D11DepthStencilView*)Resource->GetDepthBuffer();
				D3D11_VIEWPORT Viewport = { Viewarea.TopLeftX, Viewarea.TopLeftY, Viewarea.Width, Viewarea.Height, Viewarea.MinDepth, Viewarea.MaxDepth };

				ImmediateContext->RSSetViewports(1, &Viewport);
				ImmediateContext->OMSetRenderTargets(Resource->GetTargetCount(), TargetBuffer, DepthBuffer);
			}
			void D3D11Device::SetTargetMap(Graphics::RenderTarget* Resource, bool Enabled[8])
			{
				ED_ASSERT_V(Resource != nullptr, "render target should be set");
				ED_ASSERT_V(Resource->GetTargetCount() > 1, "render target should have more than one targets");

				const Viewport& Viewarea = Resource->GetViewport();
				ID3D11RenderTargetView** TargetBuffers = (ID3D11RenderTargetView**)Resource->GetTargetBuffer();
				ID3D11DepthStencilView* DepthBuffer = (ID3D11DepthStencilView*)Resource->GetDepthBuffer();
				D3D11_VIEWPORT Viewport = { Viewarea.TopLeftX, Viewarea.TopLeftY, Viewarea.Width, Viewarea.Height, Viewarea.MinDepth, Viewarea.MaxDepth };
				uint32_t Count = Resource->GetTargetCount();

				ID3D11RenderTargetView* Targets[8] = { };
				for (uint32_t i = 0; i < Count; i++)
					Targets[i] = (Enabled[i] ? TargetBuffers[i] : nullptr);

				ImmediateContext->RSSetViewports(1, &Viewport);
				ImmediateContext->OMSetRenderTargets(Count, Targets, DepthBuffer);
			}
			void D3D11Device::SetTargetRect(unsigned int Width, unsigned int Height)
			{
				ED_ASSERT_V(Width > 0 && Height > 0, "width and height should be greater than zero");

				D3D11_VIEWPORT Viewport = { 0, 0, (FLOAT)Width, (FLOAT)Height, 0, 1 };
				ImmediateContext->RSSetViewports(1, &Viewport);
				ImmediateContext->OMSetRenderTargets(0, nullptr, nullptr);
			}
			void D3D11Device::SetViewports(unsigned int Count, Viewport* Value)
			{
				ED_ASSERT_V(Value != nullptr, "value should be set");
				ED_ASSERT_V(Count > 0, "count should be greater than zero");

				D3D11_VIEWPORT Viewports[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
				for (unsigned int i = 0; i < Count; i++)
					memcpy(&Viewports[i], &Value[i], sizeof(Viewport));

				ImmediateContext->RSSetViewports(Count, Viewports);
			}
			void D3D11Device::SetScissorRects(unsigned int Count, Compute::Rectangle* Value)
			{
				ED_ASSERT_V(Value != nullptr, "value should be set");
				ED_ASSERT_V(Count > 0, "count should be greater than zero");

				D3D11_RECT Rects[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
				for (unsigned int i = 0; i < Count; i++)
				{
					Compute::Rectangle& From = Value[i];
					D3D11_RECT& To = Rects[i];
					To.left = (LONG)From.Left;
					To.right = (LONG)From.Right;
					To.bottom = (LONG)From.Bottom;
					To.top = (LONG)From.Top;
				}

				ImmediateContext->RSSetScissorRects(Count, Rects);
			}
			void D3D11Device::SetPrimitiveTopology(PrimitiveTopology Topology)
			{
				ImmediateContext->IASetPrimitiveTopology((D3D11_PRIMITIVE_TOPOLOGY)Topology);
			}
			void D3D11Device::FlushTexture(unsigned int Slot, unsigned int Count, unsigned int Type)
			{
				ED_ASSERT_V(Slot < ED_MAX_UNITS, "slot should be less than %i", (int)ED_MAX_UNITS);
				ED_ASSERT_V(Count <= ED_MAX_UNITS && Slot + Count <= ED_MAX_UNITS, "count should be less than or equal %i", (int)ED_MAX_UNITS);

				static ID3D11ShaderResourceView* Array[ED_MAX_UNITS] = { nullptr };
				if (Type & (uint32_t)ShaderType::Vertex)
					ImmediateContext->VSSetShaderResources(Slot, Count, Array);

				if (Type & (uint32_t)ShaderType::Pixel)
					ImmediateContext->PSSetShaderResources(Slot, Count, Array);

				if (Type & (uint32_t)ShaderType::Geometry)
					ImmediateContext->GSSetShaderResources(Slot, Count, Array);

				if (Type & (uint32_t)ShaderType::Hull)
					ImmediateContext->HSSetShaderResources(Slot, Count, Array);

				if (Type & (uint32_t)ShaderType::Domain)
					ImmediateContext->DSSetShaderResources(Slot, Count, Array);

				if (Type & (uint32_t)ShaderType::Compute)
					ImmediateContext->CSSetShaderResources(Slot, Count, Array);

				size_t Offset = Slot + Count;
				for (size_t i = Slot; i < Offset; i++)
					Register.Resources[i] = std::make_pair<ID3D11ShaderResourceView*, unsigned int>(nullptr, 0);
			}
			void D3D11Device::FlushState()
			{
				ImmediateContext->ClearState();
			}
			bool D3D11Device::Map(ElementBuffer* Resource, ResourceMap Mode, MappedSubresource* Map)
			{
				ED_ASSERT(Resource != nullptr, false, "resource should be set");
				ED_ASSERT(Map != nullptr, false, "map should be set");

				D3D11ElementBuffer* IResource = (D3D11ElementBuffer*)Resource;
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
				ED_ASSERT(Resource != nullptr, false, "resource should be set");
				ED_ASSERT(Map != nullptr, false, "map should be set");

				D3D11ElementBuffer* IResource = (D3D11ElementBuffer*)Resource;
				ImmediateContext->Unmap(IResource->Element, 0);
				return true;
			}
			bool D3D11Device::UpdateBuffer(ElementBuffer* Resource, void* Data, size_t Size)
			{
				ED_ASSERT(Resource != nullptr, false, "resource should be set");
				ED_ASSERT(Data != nullptr, false, "data should be set");

				D3D11ElementBuffer* IResource = (D3D11ElementBuffer*)Resource;
				D3D11_MAPPED_SUBRESOURCE MappedResource;
				if (ImmediateContext->Map(IResource->Element, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource) != S_OK)
					return false;

				memcpy(MappedResource.pData, Data, Size);
				ImmediateContext->Unmap(IResource->Element, 0);
				return true;
			}
			bool D3D11Device::UpdateBuffer(Shader* Resource, const void* Data)
			{
				ED_ASSERT(Resource != nullptr, false, "resource should be set");
				ED_ASSERT(Data != nullptr, false, "data should be set");

				D3D11Shader* IResource = (D3D11Shader*)Resource;
				ImmediateContext->UpdateSubresource(IResource->ConstantBuffer, 0, nullptr, Data, 0, 0);
				return true;
			}
			bool D3D11Device::UpdateBuffer(MeshBuffer* Resource, Compute::Vertex* Data)
			{
				ED_ASSERT(Resource != nullptr, false, "resource should be set");
				ED_ASSERT(Data != nullptr, false, "data should be set");

				D3D11MeshBuffer* IResource = (D3D11MeshBuffer*)Resource;
				MappedSubresource MappedResource;
				if (!Map(IResource->VertexBuffer, ResourceMap::Write, &MappedResource))
					return false;

				memcpy(MappedResource.Pointer, Data, (size_t)IResource->VertexBuffer->GetElements() * sizeof(Compute::Vertex));
				return Unmap(IResource->VertexBuffer, &MappedResource);
			}
			bool D3D11Device::UpdateBuffer(SkinMeshBuffer* Resource, Compute::SkinVertex* Data)
			{
				ED_ASSERT(Resource != nullptr, false, "resource should be set");
				ED_ASSERT(Data != nullptr, false, "data should be set");

				D3D11SkinMeshBuffer* IResource = (D3D11SkinMeshBuffer*)Resource;
				MappedSubresource MappedResource;
				if (!Map(IResource->VertexBuffer, ResourceMap::Write, &MappedResource))
					return false;

				memcpy(MappedResource.Pointer, Data, (size_t)IResource->VertexBuffer->GetElements() * sizeof(Compute::SkinVertex));
				return Unmap(IResource->VertexBuffer, &MappedResource);
			}
			bool D3D11Device::UpdateBuffer(InstanceBuffer* Resource)
			{
				ED_ASSERT(Resource != nullptr, false, "resource should be set");
				D3D11InstanceBuffer* IResource = (D3D11InstanceBuffer*)Resource;
				if (IResource->Array.empty() || IResource->Array.size() > IResource->ElementLimit)
					return false;

				D3D11ElementBuffer* Element = (D3D11ElementBuffer*)IResource->Elements;
				IResource->Sync = true;

				D3D11_MAPPED_SUBRESOURCE MappedResource;
				if (ImmediateContext->Map(Element->Element, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource) != S_OK)
					return false;

				memcpy(MappedResource.pData, IResource->Array.data(), (size_t)IResource->Array.size() * IResource->ElementWidth);
				ImmediateContext->Unmap(Element->Element, 0);
				return true;
			}
			bool D3D11Device::UpdateBuffer(RenderBufferType fBuffer)
			{
				ImmediateContext->UpdateSubresource(ConstantBuffer[(size_t)fBuffer], 0, nullptr, Constants[(size_t)fBuffer], 0, 0);
				return true;
			}
			bool D3D11Device::UpdateBufferSize(Shader* Resource, size_t Size)
			{
				ED_ASSERT(Resource != nullptr, false, "resource should be set");
				ED_ASSERT(Size > 0, false, "size should be greater than zero");

				D3D11Shader* IResource = (D3D11Shader*)Resource;
				D3D_RELEASE(IResource->ConstantBuffer);
				return CreateConstantBuffer(&IResource->ConstantBuffer, Size) == S_OK;
			}
			bool D3D11Device::UpdateBufferSize(InstanceBuffer* Resource, size_t Size)
			{
				ED_ASSERT(Resource != nullptr, false, "resource should be set");
				ED_ASSERT(Size > 0, false, "size should be greater than zero");

				D3D11InstanceBuffer* IResource = (D3D11InstanceBuffer*)Resource;
				ClearBuffer(IResource);
				ED_RELEASE(IResource->Elements);

				D3D_RELEASE(IResource->Resource);
				IResource->ElementLimit = Size;
				IResource->Array.clear();
				IResource->Array.reserve(IResource->ElementLimit);

				ElementBuffer::Desc F = ElementBuffer::Desc();
				F.AccessFlags = CPUAccess::Write;
				F.MiscFlags = ResourceMisc::Buffer_Structured;
				F.Usage = ResourceUsage::Dynamic;
				F.BindFlags = ResourceBind::Shader_Input;
				F.ElementCount = (unsigned int)IResource->ElementLimit;
				F.ElementWidth = (unsigned int)IResource->ElementWidth;
				F.StructureByteStride = F.ElementWidth;

				IResource->Elements = CreateElementBuffer(F);

				D3D11_SHADER_RESOURCE_VIEW_DESC SRV;
				ZeroMemory(&SRV, sizeof(SRV));
				SRV.Format = DXGI_FORMAT_UNKNOWN;
				SRV.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
				SRV.Buffer.ElementWidth = (unsigned int)IResource->ElementLimit;

				return Context->CreateShaderResourceView(((D3D11ElementBuffer*)IResource->Elements)->Element, &SRV, &IResource->Resource) == S_OK;
			}
			void D3D11Device::ClearBuffer(InstanceBuffer* Resource)
			{
				ED_ASSERT_V(Resource != nullptr, "resource should be set");
				D3D11InstanceBuffer* IResource = (D3D11InstanceBuffer*)Resource;
				if (!IResource->Sync)
					return;

				D3D11ElementBuffer* Element = (D3D11ElementBuffer*)IResource->Elements;
				IResource->Sync = false;

				D3D11_MAPPED_SUBRESOURCE MappedResource;
				ImmediateContext->Map(Element->Element, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
				MappedResource.pData = nullptr;
				ImmediateContext->Unmap(Element->Element, 0);
			}
			void D3D11Device::ClearWritable(Texture2D* Resource)
			{
				ED_ASSERT_V(Resource != nullptr, "resource should be set");
				D3D11Texture2D* IResource = (D3D11Texture2D*)Resource;

				ED_ASSERT_V(IResource->Access != nullptr, "resource should be valid");
				UINT ClearColor[4] = { 0, 0, 0, 0 };
				ImmediateContext->ClearUnorderedAccessViewUint(IResource->Access, ClearColor);
			}
			void D3D11Device::ClearWritable(Texture2D* Resource, float R, float G, float B)
			{
				ED_ASSERT_V(Resource != nullptr, "resource should be set");
				D3D11Texture2D* IResource = (D3D11Texture2D*)Resource;

				ED_ASSERT_V(IResource->Access != nullptr, "resource should be valid");
				float ClearColor[4] = { R, G, B, 0.0f };
				ImmediateContext->ClearUnorderedAccessViewFloat(IResource->Access, ClearColor);
			}
			void D3D11Device::ClearWritable(Texture3D* Resource)
			{
				ED_ASSERT_V(Resource != nullptr, "resource should be set");
				D3D11Texture3D* IResource = (D3D11Texture3D*)Resource;

				ED_ASSERT_V(IResource->Access != nullptr, "resource should be valid");
				UINT ClearColor[4] = { 0, 0, 0, 0 };
				ImmediateContext->ClearUnorderedAccessViewUint(IResource->Access, ClearColor);
			}
			void D3D11Device::ClearWritable(Texture3D* Resource, float R, float G, float B)
			{
				ED_ASSERT_V(Resource != nullptr, "resource should be set");
				D3D11Texture3D* IResource = (D3D11Texture3D*)Resource;

				ED_ASSERT_V(IResource->Access != nullptr, "resource should be valid");
				float ClearColor[4] = { R, G, B, 0.0f };
				ImmediateContext->ClearUnorderedAccessViewFloat(IResource->Access, ClearColor);
			}
			void D3D11Device::ClearWritable(TextureCube* Resource)
			{
				ED_ASSERT_V(Resource != nullptr, "resource should be set");
				D3D11TextureCube* IResource = (D3D11TextureCube*)Resource;

				ED_ASSERT_V(IResource->Access != nullptr, "resource should be valid");
				UINT ClearColor[4] = { 0, 0, 0, 0 };
				ImmediateContext->ClearUnorderedAccessViewUint(IResource->Access, ClearColor);
			}
			void D3D11Device::ClearWritable(TextureCube* Resource, float R, float G, float B)
			{
				ED_ASSERT_V(Resource != nullptr, "resource should be set");
				D3D11TextureCube* IResource = (D3D11TextureCube*)Resource;

				ED_ASSERT_V(IResource->Access != nullptr, "resource should be valid");
				float ClearColor[4] = { R, G, B, 0.0f };
				ImmediateContext->ClearUnorderedAccessViewFloat(IResource->Access, ClearColor);
			}
			void D3D11Device::Clear(float R, float G, float B)
			{
				Clear(RenderTarget, 0, R, G, B);
			}
			void D3D11Device::Clear(Graphics::RenderTarget* Resource, unsigned int Target, float R, float G, float B)
			{
				ED_ASSERT_V(Resource != nullptr, "resource should be set");
				ED_ASSERT_V(Target < Resource->GetTargetCount(), "targets count should be less than %i", (int)Resource->GetTargetCount());

				ID3D11RenderTargetView** TargetBuffer = (ID3D11RenderTargetView**)Resource->GetTargetBuffer();
				float ClearColor[4] = { R, G, B, 0.0f };

				ImmediateContext->ClearRenderTargetView(TargetBuffer[Target], ClearColor);
			}
			void D3D11Device::ClearDepth()
			{
				ClearDepth(RenderTarget);
			}
			void D3D11Device::ClearDepth(DepthTarget2D* Resource)
			{
				ED_ASSERT_V(Resource != nullptr, "resource should be set");
				D3D11DepthTarget2D* IResource = (D3D11DepthTarget2D*)Resource;
				ImmediateContext->ClearDepthStencilView(IResource->DepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
			}
			void D3D11Device::ClearDepth(DepthTargetCube* Resource)
			{
				ED_ASSERT_V(Resource != nullptr, "resource should be set");
				D3D11DepthTargetCube* IResource = (D3D11DepthTargetCube*)Resource;
				ImmediateContext->ClearDepthStencilView(IResource->DepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
			}
			void D3D11Device::ClearDepth(Graphics::RenderTarget* Resource)
			{
				ED_ASSERT_V(Resource != nullptr, "resource should be set");
				ID3D11DepthStencilView* DepthBuffer = (ID3D11DepthStencilView*)Resource->GetDepthBuffer();
				ImmediateContext->ClearDepthStencilView(DepthBuffer, D3D11_CLEAR_DEPTH, 1.0f, 0);
			}
			void D3D11Device::DrawIndexed(unsigned int Count, unsigned int IndexLocation, unsigned int BaseLocation)
			{
				ImmediateContext->DrawIndexed(Count, IndexLocation, BaseLocation);
			}
			void D3D11Device::DrawIndexed(MeshBuffer* Resource)
			{
				ED_ASSERT_V(Resource != nullptr, "resource should be set");
				D3D11ElementBuffer* VertexBuffer = (D3D11ElementBuffer*)Resource->GetVertexBuffer();
				D3D11ElementBuffer* IndexBuffer = (D3D11ElementBuffer*)Resource->GetIndexBuffer();
				unsigned int Stride = (unsigned int)VertexBuffer->Stride, Offset = 0;

				if (Register.VertexBuffers[0].first != VertexBuffer)
				{
					Register.VertexBuffers[0] = std::make_pair(VertexBuffer, 0);
					ImmediateContext->IASetVertexBuffers(0, 1, &VertexBuffer->Element, &Stride, &Offset);
				}

				if (std::get<0>(Register.IndexBuffer) != IndexBuffer || std::get<1>(Register.IndexBuffer) != Format::R32_Uint)
				{
					Register.IndexBuffer = std::make_tuple(IndexBuffer, Format::R32_Uint);
					ImmediateContext->IASetIndexBuffer(IndexBuffer->Element, DXGI_FORMAT_R32_UINT, 0);
				}

				ImmediateContext->DrawIndexed((unsigned int)IndexBuffer->GetElements(), 0, 0);
			}
			void D3D11Device::DrawIndexed(SkinMeshBuffer* Resource)
			{
				ED_ASSERT_V(Resource != nullptr, "resource should be set");
				D3D11ElementBuffer* VertexBuffer = (D3D11ElementBuffer*)Resource->GetVertexBuffer();
				D3D11ElementBuffer* IndexBuffer = (D3D11ElementBuffer*)Resource->GetIndexBuffer();
				unsigned int Stride = (unsigned int)VertexBuffer->Stride, Offset = 0;

				if (Register.VertexBuffers[0].first != VertexBuffer)
				{
					Register.VertexBuffers[0] = std::make_pair(VertexBuffer, 0);
					ImmediateContext->IASetVertexBuffers(0, 1, &VertexBuffer->Element, &Stride, &Offset);
				}

				if (std::get<0>(Register.IndexBuffer) != IndexBuffer || std::get<1>(Register.IndexBuffer) != Format::R32_Uint)
				{
					Register.IndexBuffer = std::make_tuple(IndexBuffer, Format::R32_Uint);
					ImmediateContext->IASetIndexBuffer(IndexBuffer->Element, DXGI_FORMAT_R32_UINT, 0);
				}

				ImmediateContext->DrawIndexed((unsigned int)IndexBuffer->GetElements(), 0, 0);
			}
			void D3D11Device::DrawIndexedInstanced(unsigned int IndexCountPerInstance, unsigned int InstanceCount, unsigned int IndexLocation, unsigned int VertexLocation, unsigned int InstanceLocation)
			{
				ImmediateContext->DrawIndexedInstanced(IndexCountPerInstance, InstanceCount, IndexLocation, VertexLocation, InstanceLocation);
			}
			void D3D11Device::DrawIndexedInstanced(ElementBuffer* Instances, MeshBuffer* Resource, unsigned int InstanceCount)
			{
				ED_ASSERT_V(Instances != nullptr, "instances should be set");
				ED_ASSERT_V(Resource != nullptr, "resource should be set");

				D3D11ElementBuffer* InstanceBuffer = (D3D11ElementBuffer*)Instances;
				D3D11ElementBuffer* VertexBuffer = (D3D11ElementBuffer*)Resource->GetVertexBuffer();
				D3D11ElementBuffer* IndexBuffer = (D3D11ElementBuffer*)Resource->GetIndexBuffer();
				unsigned int Stride = (unsigned int)VertexBuffer->Stride, Offset = 0;

				if (Register.VertexBuffers[0].first != VertexBuffer)
				{
					Register.VertexBuffers[0] = std::make_pair(VertexBuffer, 0);
					ImmediateContext->IASetVertexBuffers(0, 1, &VertexBuffer->Element, &Stride, &Offset);
				}

				if (std::get<0>(Register.IndexBuffer) != IndexBuffer || std::get<1>(Register.IndexBuffer) != Format::R32_Uint)
				{
					Register.IndexBuffer = std::make_tuple(IndexBuffer, Format::R32_Uint);
					ImmediateContext->IASetIndexBuffer(IndexBuffer->Element, DXGI_FORMAT_R32_UINT, 0);
				}

				Stride = (unsigned int)InstanceBuffer->Stride;
				ImmediateContext->IASetVertexBuffers(1, 1, &InstanceBuffer->Element, &Stride, &Offset);
				ImmediateContext->DrawIndexedInstanced((unsigned int)IndexBuffer->GetElements(), InstanceCount, 0, 0, 0);
			}
			void D3D11Device::DrawIndexedInstanced(ElementBuffer* Instances, SkinMeshBuffer* Resource, unsigned int InstanceCount)
			{
				ED_ASSERT_V(Instances != nullptr, "instances should be set");
				ED_ASSERT_V(Resource != nullptr, "resource should be set");

				D3D11ElementBuffer* InstanceBuffer = (D3D11ElementBuffer*)Instances;
				D3D11ElementBuffer* VertexBuffer = (D3D11ElementBuffer*)Resource->GetVertexBuffer();
				D3D11ElementBuffer* IndexBuffer = (D3D11ElementBuffer*)Resource->GetIndexBuffer();
				unsigned int Stride = (unsigned int)VertexBuffer->Stride, Offset = 0;

				if (Register.VertexBuffers[0].first != VertexBuffer)
				{
					Register.VertexBuffers[0] = std::make_pair(VertexBuffer, 0);
					ImmediateContext->IASetVertexBuffers(0, 1, &VertexBuffer->Element, &Stride, &Offset);
				}

				if (std::get<0>(Register.IndexBuffer) != IndexBuffer || std::get<1>(Register.IndexBuffer) != Format::R32_Uint)
				{
					Register.IndexBuffer = std::make_tuple(IndexBuffer, Format::R32_Uint);
					ImmediateContext->IASetIndexBuffer(IndexBuffer->Element, DXGI_FORMAT_R32_UINT, 0);
				}

				Stride = (unsigned int)InstanceBuffer->Stride;
				ImmediateContext->IASetVertexBuffers(1, 1, &InstanceBuffer->Element, &Stride, &Offset);
				ImmediateContext->DrawIndexedInstanced((unsigned int)IndexBuffer->GetElements(), InstanceCount, 0, 0, 0);
			}
			void D3D11Device::Draw(unsigned int Count, unsigned int Location)
			{
				ImmediateContext->Draw(Count, Location);
			}
			void D3D11Device::DrawInstanced(unsigned int VertexCountPerInstance, unsigned int InstanceCount, unsigned int VertexLocation, unsigned int InstanceLocation)
			{
				ImmediateContext->DrawInstanced(VertexCountPerInstance, InstanceCount, VertexLocation, InstanceLocation);
			}
			void D3D11Device::Dispatch(unsigned int GroupX, unsigned int GroupY, unsigned int GroupZ)
			{
				ImmediateContext->Dispatch(GroupX, GroupY, GroupZ);
			}
			bool D3D11Device::CopyTexture2D(Texture2D* Resource, Texture2D** Result)
			{
				ED_ASSERT(Resource != nullptr, false, "resource should be set");
				ED_ASSERT(Result != nullptr, false, "result should be set");
				D3D11Texture2D* IResource = (D3D11Texture2D*)Resource;

				ED_ASSERT(IResource->View != nullptr, false, "resource should be valid");
				D3D11_TEXTURE2D_DESC Information;
				IResource->View->GetDesc(&Information);
				Information.BindFlags |= D3D11_BIND_SHADER_RESOURCE;

				D3D11Texture2D* Texture = (D3D11Texture2D*)(*Result ? *Result : CreateTexture2D());
				if (!*Result)
				{
					if (Context->CreateTexture2D(&Information, nullptr, &Texture->View) != S_OK)
						return false;
				}

				ImmediateContext->CopyResource(Texture->View, IResource->View);
				*Result = Texture;

				return GenerateTexture(Texture);
			}
			bool D3D11Device::CopyTexture2D(Graphics::RenderTarget* Resource, unsigned int Target, Texture2D** Result)
			{
				ED_ASSERT(Resource != nullptr, false, "resource should be set");
				ED_ASSERT(Result != nullptr, false, "result should be set");

				D3D11Texture2D* Source = (D3D11Texture2D*)Resource->GetTarget2D(Target);
				ED_ASSERT(Source != nullptr, false, "source should be set");
				ED_ASSERT(Source->View != nullptr, false, "source should be valid");

				D3D11_TEXTURE2D_DESC Information;
				Source->View->GetDesc(&Information);
				Information.BindFlags |= D3D11_BIND_SHADER_RESOURCE;

				D3D11Texture2D* Texture = (D3D11Texture2D*)(*Result ? *Result : CreateTexture2D());
				if (!*Result)
				{
					if (Context->CreateTexture2D(&Information, nullptr, &Texture->View) != S_OK)
						return false;
				}

				ImmediateContext->CopyResource(Texture->View, Source->View);
				*Result = Texture;

				return GenerateTexture(Texture);
			}
			bool D3D11Device::CopyTexture2D(RenderTargetCube* Resource, Compute::CubeFace Face, Texture2D** Result)
			{
				ED_ASSERT(Resource != nullptr, false, "resource should be set");
				ED_ASSERT(Result != nullptr, false, "result should be set");

				D3D11RenderTargetCube* IResource = (D3D11RenderTargetCube*)Resource;

				ED_ASSERT(IResource->Texture != nullptr, false, "resource should be valid");
				D3D11_TEXTURE2D_DESC Information;
				IResource->Texture->GetDesc(&Information);
				Information.BindFlags |= D3D11_BIND_SHADER_RESOURCE;

				D3D11Texture2D* Texture = (D3D11Texture2D*)(*Result ? *Result : CreateTexture2D());
				if (!*Result)
				{
					if (Context->CreateTexture2D(&Information, nullptr, &Texture->View) != S_OK)
						return false;
				}

				ImmediateContext->CopySubresourceRegion(Texture->View, (unsigned int)Face * Information.MipLevels, 0, 0, 0, IResource->Texture, 0, 0);
				*Result = Texture;

				return GenerateTexture(Texture);
			}
			bool D3D11Device::CopyTexture2D(MultiRenderTargetCube* Resource, unsigned int Cube, Compute::CubeFace Face, Texture2D** Result)
			{
				ED_ASSERT(Resource != nullptr, false, "resource should be set");
				ED_ASSERT(Result != nullptr, false, "result should be set");

				D3D11MultiRenderTargetCube* IResource = (D3D11MultiRenderTargetCube*)Resource;

				ED_ASSERT(IResource->Texture[Cube] != nullptr, false, "source should be set");
				ED_ASSERT(Cube < (unsigned int)IResource->Target, false, "cube index should be less than %i", (int)IResource->Target);

				D3D11_TEXTURE2D_DESC Information;
				IResource->Texture[Cube]->GetDesc(&Information);
				Information.BindFlags |= D3D11_BIND_SHADER_RESOURCE;

				D3D11Texture2D* Texture = (D3D11Texture2D*)(*Result ? *Result : CreateTexture2D());
				if (!*Result)
				{
					if (Context->CreateTexture2D(&Information, nullptr, &Texture->View) != S_OK)
						return false;
				}

				ImmediateContext->CopySubresourceRegion(Texture->View, (unsigned int)Face * Information.MipLevels, 0, 0, 0, IResource->Texture[Cube], 0, 0);
				*Result = Texture;

				return GenerateTexture(Texture);
			}
			bool D3D11Device::CopyTextureCube(RenderTargetCube* Resource, TextureCube** Result)
			{
				ED_ASSERT(Resource != nullptr, false, "resource should be set");
				ED_ASSERT(Result != nullptr, false, "result should be set");

				D3D11RenderTargetCube* IResource = (D3D11RenderTargetCube*)Resource;

				ED_ASSERT(IResource->Texture != nullptr, false, "resource should be valid");
				D3D11_TEXTURE2D_DESC Information;
				IResource->Texture->GetDesc(&Information);
				Information.BindFlags |= D3D11_BIND_SHADER_RESOURCE;

				void* Resources[6] = { nullptr };
				for (unsigned int i = 0; i < 6; i++)
				{
					ID3D11Texture2D* Subresource;
					if (Context->CreateTexture2D(&Information, nullptr, &Subresource) != S_OK)
					{
						for (unsigned int j = 0; j < 6; j++)
						{
							ID3D11Texture2D* Src = (ID3D11Texture2D*)Resources[j];
							D3D_RELEASE(Src);
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
				ED_ASSERT(Resource != nullptr, false, "resource should be set");
				ED_ASSERT(Result != nullptr, false, "result should be set");

				D3D11MultiRenderTargetCube* IResource = (D3D11MultiRenderTargetCube*)Resource;

				ED_ASSERT(IResource->Texture[Cube] != nullptr, false, "source should be set");
				ED_ASSERT(Cube < (unsigned int)IResource->Target, false, "cube index should be less than %i", (int)IResource->Target);

				D3D11_TEXTURE2D_DESC Information;
				IResource->Texture[Cube]->GetDesc(&Information);
				Information.BindFlags |= D3D11_BIND_SHADER_RESOURCE;

				void* Resources[6] = { nullptr };
				for (unsigned int i = 0; i < 6; i++)
				{
					ID3D11Texture2D* Subresource;
					if (Context->CreateTexture2D(&Information, nullptr, &Subresource) != S_OK)
					{
						for (unsigned int j = 0; j < 6; j++)
						{
							ID3D11Texture2D* Src = (ID3D11Texture2D*)Resources[j];
							D3D_RELEASE(Src);
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
				ED_ASSERT(From != nullptr, false, "from should be set");
				ED_ASSERT(To != nullptr, false, "to should be set");

				D3D11Texture2D* Source2D = (D3D11Texture2D*)From->GetTarget2D(FromTarget);
				D3D11TextureCube* SourceCube = (D3D11TextureCube*)From->GetTargetCube(FromTarget);
				D3D11Texture2D* Dest2D = (D3D11Texture2D*)To->GetTarget2D(ToTarget);
				D3D11TextureCube* DestCube = (D3D11TextureCube*)To->GetTargetCube(ToTarget);
				ID3D11Texture2D* Source = (Source2D ? Source2D->View : (SourceCube ? SourceCube->View : nullptr));
				ID3D11Texture2D* Dest = (Dest2D ? Dest2D->View : (DestCube ? DestCube->View : nullptr));

				ED_ASSERT(Source != nullptr, false, "from should be set");
				ED_ASSERT(Dest != nullptr, false, "to should be set");

				ImmediateContext->CopyResource(Dest, Source);
				return true;
			}
			bool D3D11Device::CopyBackBuffer(Texture2D** Result)
			{
				ED_ASSERT(Result != nullptr, false, "result should be set");

				ID3D11Texture2D* BackBuffer = nullptr;
				if (SwapChain->GetBuffer(0, __uuidof(BackBuffer), reinterpret_cast<void**>(&BackBuffer)) != S_OK)
					return false;

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

					if (Context->CreateTexture2D(&Information, nullptr, &Texture->View) != S_OK)
						return false;
				}

				if (SwapChainResource.SampleDesc.Count > 1)
					ImmediateContext->ResolveSubresource(Texture->View, 0, BackBuffer, 0, DXGI_FORMAT_R8G8B8A8_UNORM);
				else
					ImmediateContext->CopyResource(Texture->View, BackBuffer);

				D3D_RELEASE(BackBuffer);
				return GenerateTexture(Texture);
			}
			bool D3D11Device::CopyBackBufferMSAA(Texture2D** Result)
			{
				ED_ASSERT(Result != nullptr, false, "result should be set");

				ID3D11Texture2D* BackBuffer = nullptr;
				if (SwapChain->GetBuffer(0, __uuidof(BackBuffer), reinterpret_cast<void**>(&BackBuffer)) != S_OK)
					return false;

				D3D11_TEXTURE2D_DESC Texture;
				BackBuffer->GetDesc(&Texture);
				Texture.BindFlags = D3D11_BIND_SHADER_RESOURCE;
				Texture.SampleDesc.Count = 1;
				Texture.SampleDesc.Quality = 0;

				ID3D11Texture2D* Resource = nullptr;
				Context->CreateTexture2D(&Texture, nullptr, &Resource);
				ImmediateContext->ResolveSubresource(Resource, 0, BackBuffer, 0, DXGI_FORMAT_R8G8B8A8_UNORM);

				D3D11_SHADER_RESOURCE_VIEW_DESC SRV;
				SRV.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
				SRV.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
				SRV.Texture2D.MostDetailedMip = 0;
				SRV.Texture2D.MipLevels = 1;

				ID3D11ShaderResourceView* ResourceView = nullptr;
				Context->CreateShaderResourceView(Resource, &SRV, &ResourceView);
				D3D_RELEASE(BackBuffer);
				D3D_RELEASE(Resource);

				return (void*)Resource;
			}
			bool D3D11Device::CopyBackBufferNoAA(Texture2D** Result)
			{
				ED_ASSERT(Result != nullptr, false, "result should be set");

				ID3D11Texture2D* BackBuffer = nullptr;
				if (SwapChain->GetBuffer(0, __uuidof(BackBuffer), reinterpret_cast<void**>(&BackBuffer)) != S_OK)
					return false;

				D3D11_TEXTURE2D_DESC Texture;
				BackBuffer->GetDesc(&Texture);
				Texture.BindFlags = D3D11_BIND_SHADER_RESOURCE;

				ID3D11Texture2D* Resource = nullptr;
				Context->CreateTexture2D(&Texture, nullptr, &Resource);
				ImmediateContext->CopyResource(Resource, BackBuffer);

				D3D11_SHADER_RESOURCE_VIEW_DESC SRV;
				SRV.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
				SRV.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
				SRV.Texture2D.MostDetailedMip = 0;
				SRV.Texture2D.MipLevels = 1;

				ID3D11ShaderResourceView* ResourceView = nullptr;
				Context->CreateShaderResourceView(Resource, &SRV, &ResourceView);
				D3D_RELEASE(BackBuffer);
				D3D_RELEASE(Resource);

				return (void*)ResourceView;
			}
			bool D3D11Device::CubemapPush(Cubemap* Resource, TextureCube* Result)
			{
				ED_ASSERT(Resource != nullptr, false, "resource should be set");
				ED_ASSERT(Resource->IsValid(), false, "resource should be valid");
				ED_ASSERT(Result != nullptr, false, "result should be set");

				D3D11Cubemap* IResource = (D3D11Cubemap*)Resource;
				D3D11TextureCube* Dest = (D3D11TextureCube*)Result;
				IResource->Dest = Dest;

				if (Dest->View != nullptr && Dest->Resource != nullptr)
					return true;
				
				D3D_RELEASE(Dest->View);
				if (Context->CreateTexture2D(&IResource->Options.Texture, nullptr, &Dest->View) != S_OK)
				{
					ED_ERR("[d3d11] cannot create texture cube for cubemap");
					return Result;
				}

				D3D_RELEASE(Dest->Resource);
				if (Context->CreateShaderResourceView(Dest->View, &IResource->Options.Resource, &Dest->Resource) != S_OK)
				{
					ED_ERR("[d3d11] cannot create texture resource for cubemap");
					return false;
				}

				return GenerateTexture(Dest);
			}
			bool D3D11Device::CubemapFace(Cubemap* Resource, Compute::CubeFace Face)
			{
				ED_ASSERT(Resource != nullptr, false, "resource should be set");
				ED_ASSERT(Resource->IsValid(), false, "resource should be valid");

				D3D11Cubemap* IResource = (D3D11Cubemap*)Resource;
				D3D11TextureCube* Dest = (D3D11TextureCube*)IResource->Dest;

				ED_ASSERT(IResource->Dest != nullptr, false, "result should be set");
				ImmediateContext->CopyResource(IResource->Merger, IResource->Source);
				ImmediateContext->CopySubresourceRegion(Dest->View, (unsigned int)Face * IResource->Meta.MipLevels, 0, 0, 0, IResource->Merger, 0, &IResource->Options.Region);
				return true;
			}
			bool D3D11Device::CubemapPop(Cubemap* Resource)
			{
				ED_ASSERT(Resource != nullptr, false, "resource should be set");
				ED_ASSERT(Resource->IsValid(), false, "resource should be valid");

				D3D11Cubemap* IResource = (D3D11Cubemap*)Resource;
				D3D11TextureCube* Dest = (D3D11TextureCube*)IResource->Dest;

				ED_ASSERT(IResource->Dest != nullptr, false, "result should be set");
				if (IResource->Meta.MipLevels > 0)
					ImmediateContext->GenerateMips(Dest->Resource);

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
					ED_CLEAR(RenderTarget);

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
				F.MiscFlags = ResourceMisc::None;
				F.FormatMode = Format::R8G8B8A8_Unorm;
				F.Usage = ResourceUsage::Default;
				F.AccessFlags = CPUAccess::None;
				F.BindFlags = ResourceBind::Render_Target | ResourceBind::Shader_Input;
				F.RenderSurface = (void*)BackBuffer;

				RenderTarget = CreateRenderTarget2D(F);
				SetTarget(RenderTarget, 0);
				D3D_RELEASE(BackBuffer);

				return true;
			}
			bool D3D11Device::GenerateTexture(Texture2D* Resource)
			{
				return CreateTexture2D(Resource, DXGI_FORMAT_UNKNOWN);
			}
			bool D3D11Device::GenerateTexture(Texture3D* Resource)
			{
				ED_ASSERT(Resource != nullptr, false, "resource should be set");
				D3D11Texture3D* IResource = (D3D11Texture3D*)Resource;

				ED_ASSERT(IResource->View != nullptr, false, "resource should be valid");
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

				D3D_RELEASE(IResource->Resource);
				if (Context->CreateShaderResourceView(IResource->View, &SRV, &IResource->Resource) == S_OK)
					return true;

				ED_ERR("[d3d11] could not generate texture 3d resource");
				return false;
			}
			bool D3D11Device::GenerateTexture(TextureCube* Resource)
			{
				ED_ASSERT(Resource != nullptr, false, "resource should be set");
				D3D11TextureCube* IResource = (D3D11TextureCube*)Resource;

				ED_ASSERT(IResource->View != nullptr, false, "resource should be valid");
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

				D3D_RELEASE(IResource->Resource);
				if (Context->CreateShaderResourceView(IResource->View, &SRV, &IResource->Resource) == S_OK)
					return true;

				ED_ERR("[d3d11] could not generate texture cube resource");
				return false;
			}
			bool D3D11Device::GetQueryData(Query* Resource, size_t* Result, bool Flush)
			{
				ED_ASSERT(Resource != nullptr, false, "resource should be set");
				ED_ASSERT(Result != nullptr, false, "result should be set");

				D3D11Query* IResource = (D3D11Query*)Resource;
				uint64_t Passing = 0;

				ED_ASSERT(IResource->Async != nullptr, false, "resource should be valid");
				bool Success = ImmediateContext->GetData(IResource->Async, &Passing, sizeof(uint64_t), !Flush ? D3D11_ASYNC_GETDATA_DONOTFLUSH : 0) == S_OK;
				*Result = (size_t)Passing;

				return Success;
			}
			bool D3D11Device::GetQueryData(Query* Resource, bool* Result, bool Flush)
			{
				ED_ASSERT(Resource != nullptr, false, "resource should be set");
				ED_ASSERT(Result != nullptr, false, "result should be set");

				D3D11Query* IResource = (D3D11Query*)Resource;

				ED_ASSERT(IResource->Async != nullptr, false, "resource should be valid");
				return ImmediateContext->GetData(IResource->Async, Result, sizeof(bool), !Flush ? D3D11_ASYNC_GETDATA_DONOTFLUSH : 0) == S_OK;
			}
			void D3D11Device::QueryBegin(Query* Resource)
			{
				ED_ASSERT_V(Resource != nullptr, "resource should be set");
				D3D11Query* IResource = (D3D11Query*)Resource;

				ED_ASSERT_V(IResource->Async != nullptr, "resource should be valid");
				ImmediateContext->Begin(IResource->Async);
			}
			void D3D11Device::QueryEnd(Query* Resource)
			{
				ED_ASSERT_V(Resource != nullptr, "resource should be set");
				D3D11Query* IResource = (D3D11Query*)Resource;

				ED_ASSERT_V(IResource->Async != nullptr, "resource should be valid");
				ImmediateContext->End(IResource->Async);
			}
			void D3D11Device::GenerateMips(Texture2D* Resource)
			{
				ED_ASSERT_V(Resource != nullptr, "resource should be set");
				D3D11Texture2D* IResource = (D3D11Texture2D*)Resource;

				ED_ASSERT_V(IResource->Resource != nullptr, "resource should be valid");
				ImmediateContext->GenerateMips(IResource->Resource);
			}
			void D3D11Device::GenerateMips(Texture3D* Resource)
			{
				ED_ASSERT_V(Resource != nullptr, "resource should be set");
				D3D11Texture3D* IResource = (D3D11Texture3D*)Resource;

				ED_ASSERT_V(IResource->Resource != nullptr, "resource should be valid");
				ImmediateContext->GenerateMips(IResource->Resource);
			}
			void D3D11Device::GenerateMips(TextureCube* Resource)
			{
				ED_ASSERT_V(Resource != nullptr, "resource should be set");
				D3D11TextureCube* IResource = (D3D11TextureCube*)Resource;

				ED_ASSERT_V(IResource->Resource != nullptr, "resource should be valid");
				ImmediateContext->GenerateMips(IResource->Resource);
			}
			bool D3D11Device::ImBegin()
			{
				if (!Immediate.VertexBuffer && !CreateDirectBuffer(0))
					return false;

				Primitives = PrimitiveTopology::Triangle_List;
				Direct.Transform = Compute::Matrix4x4::Identity();
				Direct.Padding = { 0, 0, 0, 1 };
				ViewResource = nullptr;

				Elements.clear();
				return true;
			}
			void D3D11Device::ImTransform(const Compute::Matrix4x4& Transform)
			{
				Direct.Transform = Direct.Transform * Transform;
			}
			void D3D11Device::ImTopology(PrimitiveTopology Topology)
			{
				Primitives = Topology;
			}
			void D3D11Device::ImEmit()
			{
				Elements.push_back({ 0, 0, 0, 0, 0, 1, 1, 1, 1 });
			}
			void D3D11Device::ImTexture(Texture2D* In)
			{
				ViewResource = In;
				Direct.Padding.Z = (In != nullptr);
			}
			void D3D11Device::ImColor(float X, float Y, float Z, float W)
			{
				ED_ASSERT_V(!Elements.empty(), "vertex should already be emitted");
				auto& Element = Elements.back();
				Element.CX = X;
				Element.CY = Y;
				Element.CZ = Z;
				Element.CW = W;
			}
			void D3D11Device::ImIntensity(float Intensity)
			{
				Direct.Padding.W = Intensity;
			}
			void D3D11Device::ImTexCoord(float X, float Y)
			{
				ED_ASSERT_V(!Elements.empty(), "vertex should already be emitted");
				auto& Element = Elements.back();
				Element.TX = X;
				Element.TY = Y;
			}
			void D3D11Device::ImTexCoordOffset(float X, float Y)
			{
				Direct.Padding.X = X;
				Direct.Padding.Y = Y;
			}
			void D3D11Device::ImPosition(float X, float Y, float Z)
			{
				ED_ASSERT_V(!Elements.empty(), "vertex should already be emitted");
				auto& Element = Elements.back();
				Element.PX = X;
				Element.PY = Y;
				Element.PZ = Z;
			}
			bool D3D11Device::ImEnd()
			{
				if (Elements.size() > MaxElements && !CreateDirectBuffer(Elements.size()))
					return false;

				unsigned int Stride = sizeof(Vertex), Offset = 0;
				unsigned int LastStride = 0, LastOffset = 0;
				D3D11_MAPPED_SUBRESOURCE MappedResource;
				ImmediateContext->Map(Immediate.VertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
				memcpy(MappedResource.pData, Elements.data(), (size_t)Elements.size() * sizeof(Vertex));
				ImmediateContext->Unmap(Immediate.VertexBuffer, 0);

				D3D11_PRIMITIVE_TOPOLOGY LastTopology;
				ImmediateContext->IAGetPrimitiveTopology(&LastTopology);
				ImmediateContext->IASetPrimitiveTopology((D3D11_PRIMITIVE_TOPOLOGY)Primitives);

				ID3D11InputLayout* LastLayout;
				ImmediateContext->IAGetInputLayout(&LastLayout);
				ImmediateContext->IASetInputLayout(Immediate.VertexLayout);

				ID3D11VertexShader* LastVertexShader;
				ImmediateContext->VSGetShader(&LastVertexShader, nullptr, nullptr);
				ImmediateContext->VSSetShader(Immediate.VertexShader, nullptr, 0);

				ID3D11PixelShader* LastPixelShader;
				ImmediateContext->PSGetShader(&LastPixelShader, nullptr, nullptr);
				ImmediateContext->PSSetShader(Immediate.PixelShader, nullptr, 0);

				ID3D11Buffer* LastVertexBuffer;
				ImmediateContext->IAGetVertexBuffers(0, 1, &LastVertexBuffer, &LastStride, &LastOffset);
				ImmediateContext->IASetVertexBuffers(0, 1, &Immediate.VertexBuffer, &Stride, &Offset);

				ID3D11Buffer* LastBuffer1, * LastBuffer2;
				ImmediateContext->VSGetConstantBuffers(0, 1, &LastBuffer1);
				ImmediateContext->VSSetConstantBuffers(0, 1, &Immediate.ConstantBuffer);
				ImmediateContext->PSGetConstantBuffers(0, 1, &LastBuffer2);
				ImmediateContext->PSSetConstantBuffers(0, 1, &Immediate.ConstantBuffer);

				ID3D11SamplerState* LastSampler;
				ImmediateContext->PSGetSamplers(1, 1, &LastSampler);
				ImmediateContext->PSSetSamplers(1, 1, &Immediate.Sampler);

				ID3D11ShaderResourceView* LastTexture, * NullTexture = nullptr;
				ImmediateContext->PSGetShaderResources(1, 1, &LastTexture);
				ImmediateContext->PSSetShaderResources(1, 1, ViewResource ? &((D3D11Texture2D*)ViewResource)->Resource : &NullTexture);

				ImmediateContext->UpdateSubresource(Immediate.ConstantBuffer, 0, nullptr, &Direct, 0, 0);
				ImmediateContext->Draw((unsigned int)Elements.size(), 0);
				ImmediateContext->IASetPrimitiveTopology(LastTopology);
				ImmediateContext->IASetInputLayout(LastLayout);
				ImmediateContext->VSSetShader(LastVertexShader, nullptr, 0);
				ImmediateContext->VSSetConstantBuffers(0, 1, &LastBuffer1);
				ImmediateContext->PSSetShader(LastPixelShader, nullptr, 0);
				ImmediateContext->PSSetConstantBuffers(0, 1, &LastBuffer2);
				ImmediateContext->PSSetSamplers(1, 1, &LastSampler);
				ImmediateContext->PSSetShaderResources(1, 1, &LastTexture);
				ImmediateContext->IASetVertexBuffers(0, 1, &LastVertexBuffer, &LastStride, &LastOffset);
				D3D_RELEASE(LastLayout);
				D3D_RELEASE(LastVertexShader);
				D3D_RELEASE(LastPixelShader);
				D3D_RELEASE(LastTexture);
				D3D_RELEASE(LastSampler);
				D3D_RELEASE(LastVertexBuffer);
				D3D_RELEASE(LastBuffer1);
				D3D_RELEASE(LastBuffer2);

				return true;
			}
			bool D3D11Device::Submit()
			{
				bool Success = SwapChain->Present((unsigned int)VSyncMode, PresentFlags) == S_OK;
				DispatchQueue();
				return Success;
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
				if (Context->CreateDepthStencilState(&State, &DeviceState) != S_OK)
				{
					D3D_RELEASE(DeviceState);
					ED_ERR("[d3d11] couldn't create depth stencil state");
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
				if (Context->CreateBlendState(&State, &DeviceState) != S_OK)
				{
					D3D_RELEASE(DeviceState);
					ED_ERR("[d3d11] couldn't create blend state");
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
				if (Context->CreateRasterizerState(&State, &DeviceState) != S_OK)
				{
					D3D_RELEASE(DeviceState);
					ED_ERR("[d3d11] couldn't create rasterizer state");
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
				if (Context->CreateSamplerState(&State, &DeviceState) != S_OK)
				{
					D3D_RELEASE(DeviceState);
					ED_ERR("[d3d11] couldn't create sampler state");
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
				void* Data = nullptr;
				HRESULT State = S_OK;
				size_t Size = 0;
				Shader::Desc F(I);

				if (!Preprocess(F))
				{
					ED_ERR("[d3d11] shader preprocessing failed");
					return Result;
				}

				std::string Name = GetProgramName(F);
				std::string VertexEntry = GetShaderMain(ShaderType::Vertex);
				if (F.Data.find(VertexEntry) != std::string::npos)
				{
					std::string Stage = Name + ".vtx", Bytecode;
					if (!GetProgramCache(Stage, &Bytecode))
					{
						ED_DEBUG("[d3d11] compile %s vertex shader source", Stage.c_str());

						State = D3DCompile(F.Data.c_str(), F.Data.size() * sizeof(char), F.Filename.empty() ? nullptr : F.Filename.c_str(), nullptr, nullptr, VertexEntry.c_str(), GetVSProfile(), CompileFlags, 0, &Result->Signature, &ErrorBlob);
						if (State != S_OK || GetCompileState(ErrorBlob))
						{
							std::string Message = GetCompileState(ErrorBlob);
							D3D_RELEASE(ErrorBlob);

							ED_ERR("[d3d11] couldn't compile vertex shader\n\t%s", Message.c_str());
							return Result;
						}

						Data = (void*)Result->Signature->GetBufferPointer();
						Size = (size_t)Result->Signature->GetBufferSize();
						if (!SetProgramCache(Stage, std::string((char*)Data, Size)))
							ED_WARN("[d3d11] couldn't cache vertex shader");
					}
					else
					{
						Data = (void*)Bytecode.c_str();
						Size = Bytecode.size();
						if (D3DCreateBlob(Size, &Result->Signature) != S_OK)
						{
							ED_ERR("[d3d11] couldn't load shader signature");
							return Result;
						}

						void* Buffer = Result->Signature->GetBufferPointer();
						memcpy(Buffer, Data, Size);
					}

					ED_DEBUG("[d3d11] load %s vertex shader bytecode", Stage.c_str());
					State = Context->CreateVertexShader(Data, Size, nullptr, &Result->VertexShader);
					if (State != S_OK)
					{
						ED_ERR("[d3d11] couldn't load vertex shader bytecode");
						return Result;
					}
				}

				std::string PixelEntry = GetShaderMain(ShaderType::Pixel);
				if (F.Data.find(PixelEntry) != std::string::npos)
				{
					std::string Stage = Name + ".pxl", Bytecode;
					if (!GetProgramCache(Stage, &Bytecode))
					{
						ED_DEBUG("[d3d11] compile %s pixel shader source", Stage.c_str());

						State = D3DCompile(F.Data.c_str(), F.Data.size() * sizeof(char), F.Filename.empty() ? nullptr : F.Filename.c_str(), nullptr, nullptr, PixelEntry.c_str(), GetPSProfile(), CompileFlags, 0, &ShaderBlob, &ErrorBlob);
						if (State != S_OK || GetCompileState(ErrorBlob))
						{
							std::string Message = GetCompileState(ErrorBlob);
							D3D_RELEASE(ErrorBlob);

							ED_ERR("[d3d11] couldn't compile pixel shader\n\t%s", Message.c_str());
							return Result;
						}

						Data = (void*)ShaderBlob->GetBufferPointer();
						Size = (size_t)ShaderBlob->GetBufferSize();
						if (!SetProgramCache(Stage, std::string((char*)Data, Size)))
							ED_WARN("[d3d11] couldn't cache pixel shader");
					}
					else
					{
						Data = (void*)Bytecode.c_str();
						Size = Bytecode.size();
					}

					ED_DEBUG("[d3d11] load %s pixel shader bytecode", Stage.c_str());
					State = Context->CreatePixelShader(Data, Size, nullptr, &Result->PixelShader);
					D3D_RELEASE(ShaderBlob);

					if (State != S_OK)
					{
						ED_ERR("[d3d11] couldn't load pixel shader bytecode");
						return Result;
					}
				}

				std::string GeometryEntry = GetShaderMain(ShaderType::Geometry);
				if (F.Data.find(GeometryEntry) != std::string::npos)
				{
					std::string Stage = Name + ".geo", Bytecode;
					if (!GetProgramCache(Stage, &Bytecode))
					{
						ED_DEBUG("[d3d11] compile %s geometry shader source", Stage.c_str());

						State = D3DCompile(F.Data.c_str(), F.Data.size() * sizeof(char), F.Filename.empty() ? nullptr : F.Filename.c_str(), nullptr, nullptr, GeometryEntry.c_str(), GetGSProfile(), GetCompileFlags(), 0, &ShaderBlob, &ErrorBlob);
						if (State != S_OK || GetCompileState(ErrorBlob))
						{
							std::string Message = GetCompileState(ErrorBlob);
							D3D_RELEASE(ErrorBlob);

							ED_ERR("[d3d11] couldn't compile geometry shader\n\t%s", Message.c_str());
							return Result;
						}

						Data = (void*)ShaderBlob->GetBufferPointer();
						Size = (size_t)ShaderBlob->GetBufferSize();
						if (!SetProgramCache(Stage, std::string((char*)Data, Size)))
							ED_WARN("[d3d11] couldn't cache geometry shader");
					}
					else
					{
						Data = (void*)Bytecode.c_str();
						Size = Bytecode.size();
					}

					ED_DEBUG("[d3d11] load %s geometry shader bytecode", Stage.c_str());
					State = Context->CreateGeometryShader(Data, Size, nullptr, &Result->GeometryShader);
					D3D_RELEASE(ShaderBlob);

					if (State != S_OK)
					{
						ED_ERR("[d3d11] couldn't load geometry shader bytecode");
						return Result;
					}
				}

				std::string ComputeEntry = GetShaderMain(ShaderType::Compute);
				if (F.Data.find(ComputeEntry) != std::string::npos)
				{
					std::string Stage = Name + ".cmp", Bytecode;
					if (!GetProgramCache(Stage, &Bytecode))
					{
						ED_DEBUG("[d3d11] compile %s compute shader source", Stage.c_str());

						State = D3DCompile(F.Data.c_str(), F.Data.size() * sizeof(char), F.Filename.empty() ? nullptr : F.Filename.c_str(), nullptr, nullptr, ComputeEntry.c_str(), GetCSProfile(), GetCompileFlags(), 0, &ShaderBlob, &ErrorBlob);
						if (State != S_OK || GetCompileState(ErrorBlob))
						{
							std::string Message = GetCompileState(ErrorBlob);
							D3D_RELEASE(ErrorBlob);

							ED_ERR("[d3d11] couldn't compile compute shader\n\t%s", Message.c_str());
							return Result;
						}

						Data = (void*)ShaderBlob->GetBufferPointer();
						Size = (size_t)ShaderBlob->GetBufferSize();
						if (!SetProgramCache(Stage, std::string((char*)Data, Size)))
							ED_WARN("[d3d11] couldn't cache compute shader");
					}
					else
					{
						Data = (void*)Bytecode.c_str();
						Size = Bytecode.size();
					}

					ED_DEBUG("[d3d11] load %s compute shader bytecode", Stage.c_str());
					State = Context->CreateComputeShader(Data, Size, nullptr, &Result->ComputeShader);
					D3D_RELEASE(ShaderBlob);

					if (State != S_OK)
					{
						ED_ERR("[d3d11] couldn't load compute shader bytecode");
						return Result;
					}
				}

				std::string HullEntry = GetShaderMain(ShaderType::Hull);
				if (F.Data.find(HullEntry) != std::string::npos)
				{
					std::string Stage = Name + ".hlc", Bytecode;
					if (!GetProgramCache(Stage, &Bytecode))
					{
						ED_DEBUG("[d3d11] compile %s hull shader source", Stage.c_str());

						State = D3DCompile(F.Data.c_str(), F.Data.size() * sizeof(char), F.Filename.empty() ? nullptr : F.Filename.c_str(), nullptr, nullptr, HullEntry.c_str(), GetHSProfile(), GetCompileFlags(), 0, &ShaderBlob, &ErrorBlob);
						if (State != S_OK || GetCompileState(ErrorBlob))
						{
							std::string Message = GetCompileState(ErrorBlob);
							D3D_RELEASE(ErrorBlob);

							ED_ERR("[d3d11] couldn't compile hull shader\n\t%s", Message.c_str());
							return Result;
						}

						Data = (void*)ShaderBlob->GetBufferPointer();
						Size = (size_t)ShaderBlob->GetBufferSize();
						if (!SetProgramCache(Stage, std::string((char*)Data, Size)))
							ED_WARN("[d3d11] couldn't cache hull shader");
					}
					else
					{
						Data = (void*)Bytecode.c_str();
						Size = Bytecode.size();
					}

					ED_DEBUG("[d3d11] load %s hull shader bytecode", Stage.c_str());
					State = Context->CreateHullShader(Data, Size, nullptr, &Result->HullShader);
					D3D_RELEASE(ShaderBlob);

					if (State != S_OK)
					{
						ED_ERR("[d3d11] couldn't load hull shader bytecode");
						return Result;
					}
				}

				std::string DomainEntry = GetShaderMain(ShaderType::Domain);
				if (F.Data.find(DomainEntry) != std::string::npos)
				{
					std::string Stage = Name + ".dmn", Bytecode;
					if (!GetProgramCache(Stage, &Bytecode))
					{
						ED_DEBUG("[d3d11] compile %s domain shader source", Stage.c_str());

						State = D3DCompile(F.Data.c_str(), F.Data.size() * sizeof(char), F.Filename.empty() ? nullptr : F.Filename.c_str(), nullptr, nullptr, DomainEntry.c_str(), GetDSProfile(), GetCompileFlags(), 0, &ShaderBlob, &ErrorBlob);
						if (State != S_OK || GetCompileState(ErrorBlob))
						{
							std::string Message = GetCompileState(ErrorBlob);
							D3D_RELEASE(ErrorBlob);

							ED_ERR("[d3d11] couldn't compile domain shader\n\t%s", Message.c_str());
							return Result;
						}

						Data = (void*)ShaderBlob->GetBufferPointer();
						Size = (size_t)ShaderBlob->GetBufferSize();
						if (!SetProgramCache(Stage, std::string((char*)Data, Size)))
							ED_WARN("[d3d11] couldn't cache domain shader");
					}
					else
					{
						Data = (void*)Bytecode.c_str();
						Size = Bytecode.size();
					}

					ED_DEBUG("[d3d11] load %s domain shader bytecode", Stage.c_str());
					State = Context->CreateDomainShader(Data, Size, nullptr, &Result->DomainShader);
					D3D_RELEASE(ShaderBlob);

					if (State != S_OK)
					{
						ED_ERR("[d3d11] couldn't load domain shader bytecode");
						return Result;
					}
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
				BufferDesc.BindFlags = (unsigned int)I.BindFlags;
				BufferDesc.CPUAccessFlags = (unsigned int)I.AccessFlags;
				BufferDesc.MiscFlags = (unsigned int)I.MiscFlags;
				BufferDesc.StructureByteStride = I.StructureByteStride;

				D3D11ElementBuffer* Result = new D3D11ElementBuffer(I);
				if (I.Elements != nullptr)
				{
					D3D11_SUBRESOURCE_DATA Subresource;
					ZeroMemory(&Subresource, sizeof(Subresource));
					Subresource.pSysMem = I.Elements;

					Context->CreateBuffer(&BufferDesc, &Subresource, &Result->Element);
				}
				else
					Context->CreateBuffer(&BufferDesc, nullptr, &Result->Element);

				if (I.Writable)
				{
					D3D11_UNORDERED_ACCESS_VIEW_DESC AccessDesc;
					ZeroMemory(&AccessDesc, sizeof(AccessDesc));
					AccessDesc.Format = DXGI_FORMAT_R32_FLOAT;
					AccessDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
					AccessDesc.Buffer.Flags = 0;
					AccessDesc.Buffer.FirstElement = 0;
					AccessDesc.Buffer.NumElements = I.ElementCount;

					if (Context->CreateUnorderedAccessView(Result->Element, &AccessDesc, &Result->Access) != S_OK)
					{
						ED_ERR("[d3d11] couldn't create buffer access view");
						return Result;
					}
				}

				if (!((size_t)I.MiscFlags & (size_t)ResourceMisc::Buffer_Structured))
					return Result;

				D3D11_SHADER_RESOURCE_VIEW_DESC SRV;
				ZeroMemory(&SRV, sizeof(SRV));
				SRV.Format = DXGI_FORMAT_UNKNOWN;
				SRV.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
				SRV.Buffer.ElementWidth = I.ElementCount;

				if (Context->CreateShaderResourceView(Result->Element, &SRV, &Result->Resource) != S_OK)
				{
					ED_ERR("[d3d11] couldn't create shader resource view");
					return Result;
				}

				return Result;
			}
			MeshBuffer* D3D11Device::CreateMeshBuffer(const MeshBuffer::Desc& I)
			{
				ElementBuffer::Desc F = ElementBuffer::Desc();
				F.AccessFlags = I.AccessFlags;
				F.Usage = I.Usage;
				F.BindFlags = ResourceBind::Vertex_Buffer;
				F.ElementCount = (unsigned int)I.Elements.size();
				F.Elements = (void*)I.Elements.data();
				F.ElementWidth = sizeof(Compute::Vertex);

				D3D11MeshBuffer* Result = new D3D11MeshBuffer(I);
				Result->VertexBuffer = CreateElementBuffer(F);

				F = ElementBuffer::Desc();
				F.AccessFlags = I.AccessFlags;
				F.Usage = I.Usage;
				F.BindFlags = ResourceBind::Index_Buffer;
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
				F.BindFlags = ResourceBind::Vertex_Buffer;
				F.ElementCount = (unsigned int)I.Elements.size();
				F.Elements = (void*)I.Elements.data();
				F.ElementWidth = sizeof(Compute::SkinVertex);

				D3D11SkinMeshBuffer* Result = new D3D11SkinMeshBuffer(I);
				Result->VertexBuffer = CreateElementBuffer(F);

				F = ElementBuffer::Desc();
				F.AccessFlags = I.AccessFlags;
				F.Usage = I.Usage;
				F.BindFlags = ResourceBind::Index_Buffer;
				F.ElementCount = (unsigned int)I.Indices.size();
				F.ElementWidth = sizeof(int);
				F.Elements = (void*)I.Indices.data();

				Result->IndexBuffer = CreateElementBuffer(F);
				return Result;
			}
			InstanceBuffer* D3D11Device::CreateInstanceBuffer(const InstanceBuffer::Desc& I)
			{
				ElementBuffer::Desc F = ElementBuffer::Desc();
				F.AccessFlags = CPUAccess::Write;
				F.MiscFlags = ResourceMisc::Buffer_Structured;
				F.Usage = ResourceUsage::Dynamic;
				F.BindFlags = ResourceBind::Shader_Input;
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

				if (Context->CreateShaderResourceView(((D3D11ElementBuffer*)Result->Elements)->Element, &SRV, &Result->Resource) != S_OK)
				{
					ED_ERR("[d3d11] couldn't create shader resource view");
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
				Description.BindFlags = (unsigned int)I.BindFlags;
				Description.CPUAccessFlags = (unsigned int)I.AccessFlags;
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
				if (Context->CreateTexture2D(&Description, I.Data && I.MipLevels <= 0 ? &Data : nullptr, &Result->View) != S_OK)
				{
					ED_ERR("[d3d11] couldn't create 2d texture");
					return Result;
				}

				if (!GenerateTexture(Result))
				{
					ED_ERR("[d3d11] couldn't generate 2d resource");
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

				if (Context->CreateUnorderedAccessView(Result->View, &AccessDesc, &Result->Access) != S_OK)
				{
					ED_ERR("[d3d11] couldn't create 2d texture access view");
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
				Description.BindFlags = (unsigned int)I.BindFlags;
				Description.CPUAccessFlags = (unsigned int)I.AccessFlags;
				Description.MiscFlags = (unsigned int)I.MiscFlags;

				if (I.MipLevels > 0)
				{
					Description.BindFlags |= D3D11_BIND_RENDER_TARGET;
					Description.MiscFlags |= D3D11_RESOURCE_MISC_GENERATE_MIPS;
				}

				if (I.Writable)
					Description.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;

				D3D11Texture3D* Result = new D3D11Texture3D();
				if (Context->CreateTexture3D(&Description, nullptr, &Result->View) != S_OK)
				{
					ED_ERR("[d3d11] couldn't create 3d resource");
					return Result;
				}

				if (!GenerateTexture(Result))
				{
					ED_ERR("[d3d11] couldn't generate 3d resource");
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

				if (Context->CreateUnorderedAccessView(Result->View, &AccessDesc, &Result->Access) != S_OK)
				{
					ED_ERR("[d3d11] couldn't create 3d texture access view");
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
				Description.BindFlags = (unsigned int)I.BindFlags;
				Description.CPUAccessFlags = (unsigned int)I.AccessFlags;
				Description.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE | (unsigned int)I.MiscFlags;

				if (I.MipLevels > 0)
				{
					Description.BindFlags |= D3D11_BIND_RENDER_TARGET;
					Description.MiscFlags |= D3D11_RESOURCE_MISC_GENERATE_MIPS;
				}

				if (I.Writable)
					Description.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;

				D3D11TextureCube* Result = new D3D11TextureCube();
				if (Context->CreateTexture2D(&Description, 0, &Result->View) != S_OK)
				{
					ED_ERR("[d3d11] couldn't create texture cube");
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
					ED_ERR("[d3d11] couldn't generate cube resource");
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

				if (Context->CreateUnorderedAccessView(Result->View, &AccessDesc, &Result->Access) != S_OK)
				{
					ED_ERR("[d3d11] couldn't create cube texture access view");
					return Result;
				}

				return Result;
			}
			TextureCube* D3D11Device::CreateTextureCube(Graphics::Texture2D* Resource[6])
			{
				ED_ASSERT(Resource[0] && Resource[1] && Resource[2] && Resource[3] && Resource[4] && Resource[5], nullptr, "all 6 faces should be set");

				void* Resources[6];
				for (unsigned int i = 0; i < 6; i++)
					Resources[i] = (void*)((D3D11Texture2D*)Resource[i])->View;

				return CreateTextureCubeInternal(Resources);
			}
			TextureCube* D3D11Device::CreateTextureCube(Graphics::Texture2D* Resource)
			{
				ED_ASSERT(Resource != nullptr, nullptr, "resource should be set");
				ID3D11Texture2D* Src = ((D3D11Texture2D*)Resource)->View;

				ED_ASSERT(Src != nullptr, nullptr, "src should be set");
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
					ED_ERR("[d3d11] couldn't create texture cube because width or height cannot be not divided");
					return nullptr;
				}

				D3D11TextureCube* Result = new D3D11TextureCube();
				if (Context->CreateTexture2D(&Description, 0, &Result->View) != S_OK)
				{
					ED_ERR("[d3d11] couldn't create texture 2d");
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
				ED_ASSERT(Resource[0] && Resource[1] && Resource[2] && Resource[3] && Resource[4] && Resource[5], nullptr, "all 6 faces should be set");

				D3D11_TEXTURE2D_DESC Description;
				((ID3D11Texture2D*)Resource[0])->GetDesc(&Description);
				Description.MipLevels = 1;
				Description.ArraySize = 6;
				Description.Usage = D3D11_USAGE_DEFAULT;
				Description.BindFlags = D3D11_BIND_SHADER_RESOURCE;
				Description.CPUAccessFlags = 0;
				Description.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;

				D3D11TextureCube* Result = new D3D11TextureCube();
				if (Context->CreateTexture2D(&Description, 0, &Result->View) != S_OK)
				{
					ED_ERR("[d3d11] couldn't create texture 2d");
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
			DepthTarget2D* D3D11Device::CreateDepthTarget2D(const DepthTarget2D::Desc& I)
			{
				D3D11DepthTarget2D* Result = new D3D11DepthTarget2D(I);

				D3D11_TEXTURE2D_DESC DepthBuffer;
				ZeroMemory(&DepthBuffer, sizeof(DepthBuffer));
				DepthBuffer.Width = I.Width;
				DepthBuffer.Height = I.Height;
				DepthBuffer.MipLevels = 1;
				DepthBuffer.ArraySize = 1;
				DepthBuffer.Format = GetBaseDepthFormat(I.FormatMode);
				DepthBuffer.SampleDesc.Count = 1;
				DepthBuffer.SampleDesc.Quality = 0;
				DepthBuffer.Usage = (D3D11_USAGE)I.Usage;
				DepthBuffer.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
				DepthBuffer.CPUAccessFlags = (unsigned int)I.AccessFlags;
				DepthBuffer.MiscFlags = 0;
				
				ID3D11Texture2D* DepthTexture = nullptr;
				if (Context->CreateTexture2D(&DepthBuffer, nullptr, &DepthTexture) != S_OK)
				{
					ED_ERR("[d3d11] couldn't create depth buffer texture 2d");
					return Result;
				}

				D3D11_DEPTH_STENCIL_VIEW_DESC DSV;
				ZeroMemory(&DSV, sizeof(DSV));
				DSV.Format = GetInternalDepthFormat(I.FormatMode);
				DSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
				DSV.Texture2D.MipSlice = 0;

				if (Context->CreateDepthStencilView(DepthTexture, &DSV, &Result->DepthStencilView) != S_OK)
				{
					ED_ERR("[d3d11] couldn't create depth stencil view");
					return Result;
				}

				Result->Resource = CreateTexture2D();
				((D3D11Texture2D*)Result->Resource)->View = DepthTexture;

				if (!CreateTexture2D(Result->Resource, GetDepthFormat(I.FormatMode)))
				{
					ED_ERR("[d3d11] couldn't create shader resource view");
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
			DepthTargetCube* D3D11Device::CreateDepthTargetCube(const DepthTargetCube::Desc& I)
			{
				D3D11DepthTargetCube* Result = new D3D11DepthTargetCube(I);

				D3D11_TEXTURE2D_DESC DepthBuffer;
				ZeroMemory(&DepthBuffer, sizeof(DepthBuffer));
				DepthBuffer.Width = I.Size;
				DepthBuffer.Height = I.Size;
				DepthBuffer.MipLevels = 1;
				DepthBuffer.ArraySize = 6;
				DepthBuffer.Format = GetBaseDepthFormat(I.FormatMode);
				DepthBuffer.SampleDesc.Count = 1;
				DepthBuffer.SampleDesc.Quality = 0;
				DepthBuffer.Usage = (D3D11_USAGE)I.Usage;
				DepthBuffer.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
				DepthBuffer.CPUAccessFlags = (unsigned int)I.AccessFlags;
				DepthBuffer.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;

				ID3D11Texture2D* DepthTexture = nullptr;
				if (Context->CreateTexture2D(&DepthBuffer, nullptr, &DepthTexture) != S_OK)
				{
					ED_ERR("[d3d11] couldn't create depth buffer texture 2d");
					return Result;
				}

				D3D11_DEPTH_STENCIL_VIEW_DESC DSV;
				ZeroMemory(&DSV, sizeof(DSV));
				DSV.Format = GetInternalDepthFormat(I.FormatMode);
				DSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
				DSV.Texture2DArray.FirstArraySlice = 0;
				DSV.Texture2DArray.ArraySize = 6;
				DSV.Texture2DArray.MipSlice = 0;

				if (Context->CreateDepthStencilView(DepthTexture, &DSV, &Result->DepthStencilView) != S_OK)
				{
					ED_ERR("[d3d11] couldn't create depth stencil view");
					return Result;
				}

				Result->Resource = CreateTextureCube();
				((D3D11TextureCube*)Result->Resource)->View = DepthTexture;

				if (!CreateTextureCube(Result->Resource, GetDepthFormat(I.FormatMode)))
				{
					ED_ERR("[d3d11] couldn't create shader resource view");
					return Result;
				}

				Result->Viewarea.Width = (FLOAT)I.Size;
				Result->Viewarea.Height = (FLOAT)I.Size;
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
				DepthBuffer.CPUAccessFlags = (unsigned int)I.AccessFlags;
				DepthBuffer.MiscFlags = 0;

				ID3D11Texture2D* DepthTexture = nullptr;
				if (Context->CreateTexture2D(&DepthBuffer, nullptr, &DepthTexture) != S_OK)
				{
					ED_ERR("[d3d11] couldn't create depth buffer texture 2d");
					return Result;
				}

				D3D11_DEPTH_STENCIL_VIEW_DESC DSV;
				ZeroMemory(&DSV, sizeof(DSV));
				DSV.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
				DSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
				DSV.Texture2D.MipSlice = 0;

				if (Context->CreateDepthStencilView(DepthTexture, &DSV, &Result->DepthStencilView) != S_OK)
				{
					ED_ERR("[d3d11] couldn't create depth stencil view");
					return Result;
				}

				Result->DepthStencil = CreateTexture2D();
				((D3D11Texture2D*)Result->DepthStencil)->View = DepthTexture;

				if (!CreateTexture2D(Result->DepthStencil, DXGI_FORMAT_R24_UNORM_X8_TYPELESS))
				{
					ED_ERR("[d3d11] couldn't create shader resource view");
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
					Description.Format = GetNonDepthFormat(I.FormatMode);
					Description.SampleDesc.Count = 1;
					Description.SampleDesc.Quality = 0;
					Description.Usage = (D3D11_USAGE)I.Usage;
					Description.BindFlags = (unsigned int)I.BindFlags;
					Description.CPUAccessFlags = (unsigned int)I.AccessFlags;
					Description.MiscFlags = (unsigned int)I.MiscFlags;

					if (Context->CreateTexture2D(&Description, nullptr, &Result->Texture) != S_OK)
					{
						ED_ERR("[d3d11] couldn't create surface texture view");
						return Result;
					}

					Result->Resource = CreateTexture2D();
					((D3D11Texture2D*)Result->Resource)->View = Result->Texture;
					Result->Texture->AddRef();

					if (!GenerateTexture(Result->Resource))
					{
						ED_ERR("[d3d11] couldn't create shader resource view");
						return Result;
					}
				}

				D3D11_RENDER_TARGET_VIEW_DESC RTV;
				RTV.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
				RTV.Texture2DArray.MipSlice = 0;
				RTV.Texture2DArray.ArraySize = 1;
				RTV.Format = GetNonDepthFormat(I.FormatMode);

				if (Context->CreateRenderTargetView(I.RenderSurface ? (ID3D11Texture2D*)I.RenderSurface : Result->Texture, &RTV, &Result->RenderTargetView) != S_OK)
				{
					ED_ERR("[d3d11] couldn't create render target view");
					return Result;
				}

				Result->Viewarea.Width = (float)I.Width;
				Result->Viewarea.Height = (float)I.Height;
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
				DepthBuffer.CPUAccessFlags = (unsigned int)I.AccessFlags;
				DepthBuffer.MiscFlags = 0;

				ID3D11Texture2D* DepthTexture = nullptr;
				if (Context->CreateTexture2D(&DepthBuffer, nullptr, &DepthTexture) != S_OK)
				{
					ED_ERR("[d3d11] couldn't create depth buffer texture 2d");
					return Result;
				}

				D3D11_DEPTH_STENCIL_VIEW_DESC DSV;
				ZeroMemory(&DSV, sizeof(DSV));
				DSV.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
				DSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
				DSV.Texture2D.MipSlice = 0;

				if (Context->CreateDepthStencilView(DepthTexture, &DSV, &Result->DepthStencilView) != S_OK)
				{
					ED_ERR("[d3d11] couldn't create depth stencil view");
					return Result;
				}

				Result->DepthStencil = CreateTexture2D();
				((D3D11Texture2D*)Result->DepthStencil)->View = DepthTexture;

				if (!CreateTexture2D(Result->DepthStencil, DXGI_FORMAT_R24_UNORM_X8_TYPELESS))
				{
					ED_ERR("[d3d11] couldn't create shader resource view");
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
				Result->Information.BindFlags = (unsigned int)I.BindFlags;
				Result->Information.CPUAccessFlags = (unsigned int)I.AccessFlags;
				Result->Information.MiscFlags = (unsigned int)I.MiscFlags;

				D3D11_RENDER_TARGET_VIEW_DESC RTV;
				RTV.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
				RTV.Texture2DArray.MipSlice = 0;
				RTV.Texture2DArray.ArraySize = 1;

				for (unsigned int i = 0; i < (unsigned int)Result->Target; i++)
				{
					DXGI_FORMAT Format = GetNonDepthFormat(I.FormatMode[i]);
					Result->Information.Format = Format;
					if (Context->CreateTexture2D(&Result->Information, nullptr, &Result->Texture[i]) != S_OK)
					{
						ED_ERR("[d3d11] couldn't create surface texture 2d #%i", i);
						return Result;
					}

					RTV.Format = Format;
					if (Context->CreateRenderTargetView(Result->Texture[i], &RTV, &Result->RenderTargetView[i]) != S_OK)
					{
						ED_ERR("[d3d11] couldn't create render target view #%i", i);
						return Result;
					}

					Result->Resource[i] = CreateTexture2D();
					((D3D11Texture2D*)Result->Resource[i])->View = Result->Texture[i];
					Result->Texture[i]->AddRef();

					if (!GenerateTexture(Result->Resource[i]))
					{
						ED_ERR("[d3d11] couldn't create shader resource view");
						return Result;
					}
				}

				Result->Viewarea.Width = (float)I.Width;
				Result->Viewarea.Height = (float)I.Height;
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
				DepthBuffer.CPUAccessFlags = (unsigned int)I.AccessFlags;
				DepthBuffer.MiscFlags = 0;

				ID3D11Texture2D* DepthTexture = nullptr;
				if (Context->CreateTexture2D(&DepthBuffer, nullptr, &DepthTexture) != S_OK)
				{
					ED_ERR("[d3d11] couldn't create depth buffer texture 2d");
					return Result;
				}

				D3D11_DEPTH_STENCIL_VIEW_DESC DSV;
				ZeroMemory(&DSV, sizeof(DSV));
				DSV.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
				DSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
				DSV.Texture2DArray.FirstArraySlice = 0;
				DSV.Texture2DArray.ArraySize = 6;
				DSV.Texture2DArray.MipSlice = 0;

				if (Context->CreateDepthStencilView(DepthTexture, &DSV, &Result->DepthStencilView) != S_OK)
				{
					ED_ERR("[d3d11] couldn't create depth stencil view");
					return Result;
				}

				Result->DepthStencil = CreateTexture2D();
				((D3D11Texture2D*)Result->DepthStencil)->View = DepthTexture;

				if (!CreateTexture2D(Result->DepthStencil, DXGI_FORMAT_R24_UNORM_X8_TYPELESS))
				{
					ED_ERR("[d3d11] couldn't create shader resource view");
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
				Description.Format = GetNonDepthFormat(I.FormatMode);
				Description.Usage = (D3D11_USAGE)I.Usage;
				Description.BindFlags = (unsigned int)I.BindFlags;
				Description.CPUAccessFlags = (unsigned int)I.AccessFlags;
				Description.MiscFlags = (unsigned int)I.MiscFlags;

				if (Context->CreateTexture2D(&Description, nullptr, &Result->Texture) != S_OK)
				{
					ED_ERR("[d3d11] couldn't create cube map texture 2d");
					return Result;
				}

				D3D11_RENDER_TARGET_VIEW_DESC RTV;
				ZeroMemory(&RTV, sizeof(RTV));
				RTV.Format = Description.Format;
				RTV.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
				RTV.Texture2DArray.FirstArraySlice = 0;
				RTV.Texture2DArray.ArraySize = 6;
				RTV.Texture2DArray.MipSlice = 0;

				if (Context->CreateRenderTargetView(Result->Texture, &RTV, &Result->RenderTargetView) != S_OK)
				{
					ED_ERR("[d3d11] couldn't create render target view");
					return Result;
				}

				Result->Resource = CreateTextureCube();
				((D3D11TextureCube*)Result->Resource)->View = Result->Texture;
				Result->Texture->AddRef();

				if (!GenerateTexture(Result->Resource))
				{
					ED_ERR("[d3d11] couldn't create shader resource view");
					return Result;
				}

				Result->Viewarea.Width = (float)I.Size;
				Result->Viewarea.Height = (float)I.Size;
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
				DepthBuffer.CPUAccessFlags = (unsigned int)I.AccessFlags;
				DepthBuffer.MiscFlags = 0;

				ID3D11Texture2D* DepthTexture = nullptr;
				if (Context->CreateTexture2D(&DepthBuffer, nullptr, &DepthTexture) != S_OK)
				{
					ED_ERR("[d3d11] couldn't create depth buffer texture 2d");
					return Result;
				}

				D3D11_DEPTH_STENCIL_VIEW_DESC DSV;
				ZeroMemory(&DSV, sizeof(DSV));
				DSV.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
				DSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
				DSV.Texture2DArray.FirstArraySlice = 0;
				DSV.Texture2DArray.ArraySize = 6;
				DSV.Texture2DArray.MipSlice = 0;

				if (Context->CreateDepthStencilView(DepthTexture, &DSV, &Result->DepthStencilView) != S_OK)
				{
					ED_ERR("[d3d11] couldn't create depth stencil view");
					return Result;
				}

				Result->DepthStencil = CreateTexture2D();
				((D3D11Texture2D*)Result->DepthStencil)->View = DepthTexture;

				if (!CreateTexture2D(Result->DepthStencil, DXGI_FORMAT_R24_UNORM_X8_TYPELESS))
				{
					ED_ERR("[d3d11] couldn't create shader resource view");
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
				Description.CPUAccessFlags = (unsigned int)I.AccessFlags;
				Description.MiscFlags = (unsigned int)I.MiscFlags;
				Description.BindFlags = (unsigned int)I.BindFlags;
				Description.MipLevels = MipLevels;

				D3D11_RENDER_TARGET_VIEW_DESC RTV;
				ZeroMemory(&RTV, sizeof(RTV));
				RTV.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
				RTV.Texture2DArray.FirstArraySlice = 0;
				RTV.Texture2DArray.ArraySize = 6;
				RTV.Texture2DArray.MipSlice = 0;

				for (unsigned int i = 0; i < (unsigned int)Result->Target; i++)
				{
					DXGI_FORMAT Format = GetNonDepthFormat(I.FormatMode[i]);
					Description.Format = Format;
					if (Context->CreateTexture2D(&Description, nullptr, &Result->Texture[i]) != S_OK)
					{
						ED_ERR("[d3d11] couldn't create cube map rexture 2d");
						return Result;
					}

					RTV.Format = Format;
					if (Context->CreateRenderTargetView(Result->Texture[i], &RTV, &Result->RenderTargetView[i]) != S_OK)
					{
						ED_ERR("[d3d11] couldn't create render target view");
						return Result;
					}

					Result->Resource[i] = CreateTextureCube();
					((D3D11TextureCube*)Result->Resource[i])->View = Result->Texture[i];
					Result->Texture[i]->AddRef();

					if (!GenerateTexture(Result->Resource[i]))
					{
						ED_ERR("[d3d11] couldn't create shader resource view");
						return Result;
					}
				}

				Result->Viewarea.Width = (float)I.Size;
				Result->Viewarea.Height = (float)I.Size;
				Result->Viewarea.MinDepth = 0.0f;
				Result->Viewarea.MaxDepth = 1.0f;
				Result->Viewarea.TopLeftX = 0.0f;
				Result->Viewarea.TopLeftY = 0.0f;

				return Result;
			}
			Cubemap* D3D11Device::CreateCubemap(const Cubemap::Desc& I)
			{
				D3D11Cubemap* Result = new D3D11Cubemap(I);

				D3D11_TEXTURE2D_DESC Texture;
				Result->Source->GetDesc(&Texture);
				Texture.ArraySize = 1;
				Texture.CPUAccessFlags = 0;
				Texture.MiscFlags = 0;
				Texture.MipLevels = I.MipLevels;

				if (Context->CreateTexture2D(&Texture, nullptr, &Result->Merger) != S_OK)
				{
					ED_ERR("[d3d11] cannot create texture 2d for cubemap");
					return Result;
				}

				return Result;
			}
			Query* D3D11Device::CreateQuery(const Query::Desc& I)
			{
				D3D11_QUERY_DESC Desc;
				Desc.Query = (I.Predicate ? D3D11_QUERY_OCCLUSION_PREDICATE : D3D11_QUERY_OCCLUSION);
				Desc.MiscFlags = (I.AutoPass ? D3D11_QUERY_MISC_PREDICATEHINT : 0);

				D3D11Query* Result = new D3D11Query();
				Context->CreateQuery(&Desc, &Result->Async);

				return Result;
			}
			PrimitiveTopology D3D11Device::GetPrimitiveTopology() const
			{
				D3D11_PRIMITIVE_TOPOLOGY Topology;
				ImmediateContext->IAGetPrimitiveTopology(&Topology);

				return (PrimitiveTopology)Topology;
			}
			ShaderModel D3D11Device::GetSupportedShaderModel() const
			{
				if (FeatureLevel == D3D_FEATURE_LEVEL::D3D_FEATURE_LEVEL_11_0)
					return ShaderModel::HLSL_5_0;

				if (FeatureLevel == D3D_FEATURE_LEVEL::D3D_FEATURE_LEVEL_10_1)
					return ShaderModel::HLSL_4_1;

				if (FeatureLevel == D3D_FEATURE_LEVEL::D3D_FEATURE_LEVEL_10_0)
					return ShaderModel::HLSL_4_0;

				if (FeatureLevel == D3D_FEATURE_LEVEL::D3D_FEATURE_LEVEL_9_3)
					return ShaderModel::HLSL_3_0;

				if (FeatureLevel == D3D_FEATURE_LEVEL::D3D_FEATURE_LEVEL_9_2)
					return ShaderModel::HLSL_2_0;

				if (FeatureLevel == D3D_FEATURE_LEVEL::D3D_FEATURE_LEVEL_9_1)
					return ShaderModel::HLSL_1_0;

				return ShaderModel::Invalid;
			}
			void* D3D11Device::GetDevice() const
			{
				return (void*)Context;
			}
			void* D3D11Device::GetContext() const
			{
				return (void*)ImmediateContext;
			}
			bool D3D11Device::IsValid() const
			{
				return BasicEffect != nullptr;
			}
			bool D3D11Device::CreateDirectBuffer(size_t Size)
			{
				MaxElements = Size + 1;
				D3D_RELEASE(Immediate.VertexBuffer);

				D3D11_BUFFER_DESC BufferDesc;
				ZeroMemory(&BufferDesc, sizeof(BufferDesc));
				BufferDesc.Usage = D3D11_USAGE_DYNAMIC;
				BufferDesc.ByteWidth = (unsigned int)MaxElements * sizeof(Vertex);
				BufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
				BufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

				if (Context->CreateBuffer(&BufferDesc, nullptr, &Immediate.VertexBuffer) != S_OK)
				{
					ED_ERR("[d3d11] couldn't create vertex buffer");
					return false;
				}

				if (!Immediate.Sampler)
				{
					D3D11SamplerState* State = (D3D11SamplerState*)GetSamplerState("trilinear-x16");
					if (State != nullptr)
						Immediate.Sampler = State->Resource;
				}

				if (!Immediate.VertexShader)
				{
					static const char* VertexShaderCode = D3D_INLINE(
					cbuffer VertexBuffer : register(b0)
					{
						matrix Transform;
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
						Output.Position = mul(Transform, float4(Input.Position.xyz, 1));
						Output.Color = Input.Color;
						Output.TexCoord = Input.TexCoord;

						return Output;
					};
					);

					ID3DBlob* Blob = nullptr, * Error = nullptr;
					D3DCompile(VertexShaderCode, strlen(VertexShaderCode), nullptr, nullptr, nullptr, "vs_main", GetVSProfile(), 0, 0, &Blob, nullptr);
					if (GetCompileState(Error))
					{
						std::string Message = GetCompileState(Error);
						D3D_RELEASE(Error);

						ED_ERR("[d3d11] couldn't compile vertex shader\n\t%s", Message.c_str());
						return false;
					}

					if (Context->CreateVertexShader((DWORD*)Blob->GetBufferPointer(), Blob->GetBufferSize(), nullptr, &Immediate.VertexShader) != S_OK)
					{
						D3D_RELEASE(Blob);
						ED_ERR("[d3d11] couldn't create vertex shader");
						return false;
					}

					D3D11_INPUT_ELEMENT_DESC LayoutDesc[] =
					{
						{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
						{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 3 * sizeof(float), D3D11_INPUT_PER_VERTEX_DATA, 0 },
						{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 5 * sizeof(float), D3D11_INPUT_PER_VERTEX_DATA, 0 }
					};
					if (Context->CreateInputLayout(LayoutDesc, 3, Blob->GetBufferPointer(), Blob->GetBufferSize(), &Immediate.VertexLayout) != S_OK)
					{
						D3D_RELEASE(Blob);
						ED_ERR("[d3d11] couldn't create input layout");
						return false;
					}

					D3D_RELEASE(Blob);
				}

				if (!Immediate.PixelShader)
				{
					static const char* PixelShaderCode = D3D_INLINE(
					cbuffer VertexBuffer : register(b0)
					{
						matrix Transform;
						float4 Padding;
					};

					struct PS_INPUT
					{
						float4 Position : SV_POSITION;
						float2 TexCoord : TEXCOORD0;
						float4 Color : COLOR0;
					};

					Texture2D Diffuse : register(t1);
					SamplerState State : register(s1);

					float4 ps_main(PS_INPUT Input) : SV_TARGET0
					{
					   if (Padding.z > 0)
						   return Input.Color * Diffuse.Sample(State, Input.TexCoord + Padding.xy) * Padding.w;

					   return Input.Color * Padding.w;
					};
					);

					ID3DBlob* Blob = nullptr, * Error = nullptr;
					D3DCompile(PixelShaderCode, strlen(PixelShaderCode), nullptr, nullptr, nullptr, "ps_main", GetPSProfile(), 0, 0, &Blob, &Error);
					if (GetCompileState(Error))
					{
						std::string Message = GetCompileState(Error);
						D3D_RELEASE(Error);

						ED_ERR("[d3d11] couldn't compile pixel shader\n\t%s", Message.c_str());
						return false;
					}

					if (Context->CreatePixelShader((DWORD*)Blob->GetBufferPointer(), Blob->GetBufferSize(), nullptr, &Immediate.PixelShader) != S_OK)
					{
						D3D_RELEASE(Blob);
						ED_ERR("[d3d11] couldn't create pixel shader");
						return false;
					}

					D3D_RELEASE(Blob);
				}

				if (!Immediate.ConstantBuffer)
				{
					CreateConstantBuffer(&Immediate.ConstantBuffer, sizeof(Direct));
					if (!Immediate.ConstantBuffer)
					{
						ED_ERR("[d3d11] couldn't create vertex constant buffer");
						return false;
					}
				}

				return true;
			}
			bool D3D11Device::CreateTexture2D(Texture2D* Resource, DXGI_FORMAT InternalFormat)
			{
				ED_ASSERT(Resource != nullptr, false, "resource should be set");
				D3D11Texture2D* IResource = (D3D11Texture2D*)Resource;
				ED_ASSERT(IResource->View != nullptr, false, "resource should be set");

				D3D11_TEXTURE2D_DESC Description;
				IResource->View->GetDesc(&Description);
				IResource->FormatMode = (Format)Description.Format;
				IResource->Usage = (ResourceUsage)Description.Usage;
				IResource->Width = Description.Width;
				IResource->Height = Description.Height;
				IResource->MipLevels = Description.MipLevels;
				IResource->AccessFlags = (CPUAccess)Description.CPUAccessFlags;

				if (IResource->Resource != nullptr)
					return true;

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

				D3D_RELEASE(IResource->Resource);
				if (Context->CreateShaderResourceView(IResource->View, &SRV, &IResource->Resource) == S_OK)
					return true;

				ED_ERR("[d3d11] could not generate texture 2d resource");
				return false;
			}
			bool D3D11Device::CreateTextureCube(TextureCube* Resource, DXGI_FORMAT InternalFormat)
			{
				ED_ASSERT(Resource != nullptr, false, "resource should be set");
				D3D11TextureCube* IResource = (D3D11TextureCube*)Resource;
				ED_ASSERT(IResource->View != nullptr, false, "resource should be set");

				D3D11_TEXTURE2D_DESC Description;
				IResource->View->GetDesc(&Description);
				IResource->FormatMode = (Format)Description.Format;
				IResource->Usage = (ResourceUsage)Description.Usage;
				IResource->Width = Description.Width;
				IResource->Height = Description.Height;
				IResource->MipLevels = Description.MipLevels;
				IResource->AccessFlags = (CPUAccess)Description.CPUAccessFlags;

				if (IResource->Resource != nullptr)
					return true;

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

				D3D_RELEASE(IResource->Resource);
				if (Context->CreateShaderResourceView(IResource->View, &SRV, &IResource->Resource) == S_OK)
					return true;

				ED_ERR("[d3d11] could not generate texture 2d resource");
				return false;
			}
			ID3D11InputLayout* D3D11Device::GenerateInputLayout(D3D11Shader* Shader)
			{
				ED_ASSERT(Shader != nullptr, nullptr, "shader should be set");
				if (Shader->VertexLayout != nullptr)
					return Shader->VertexLayout;

				if (!Shader->Signature || !Register.Layout || Register.Layout->Layout.empty())
					return nullptr;

				std::vector<D3D11_INPUT_ELEMENT_DESC> Result;
				for (size_t i = 0; i < Register.Layout->Layout.size(); i++)
				{
					const InputLayout::Attribute& It = Register.Layout->Layout[i];
					D3D11_INPUT_CLASSIFICATION Class = It.PerVertex ? D3D11_INPUT_PER_VERTEX_DATA : D3D11_INPUT_PER_INSTANCE_DATA;
					UINT Step = It.PerVertex ? 0 : 1;

					if (It.Format == AttributeType::Matrix)
					{
						DXGI_FORMAT Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
						Result.push_back({ It.SemanticName.c_str(), It.SemanticIndex + 0, Format, It.Slot, It.AlignedByteOffset + 0, Class, Step });
						Result.push_back({ It.SemanticName.c_str(), It.SemanticIndex + 1, Format, It.Slot, It.AlignedByteOffset + 16, Class, Step });
						Result.push_back({ It.SemanticName.c_str(), It.SemanticIndex + 2, Format, It.Slot, It.AlignedByteOffset + 32, Class, Step });
						Result.push_back({ It.SemanticName.c_str(), It.SemanticIndex + 3, Format, It.Slot, It.AlignedByteOffset + 48, Class, Step });
						continue;
					}

					D3D11_INPUT_ELEMENT_DESC At;
					At.SemanticName = It.SemanticName.c_str();
					At.AlignedByteOffset = It.AlignedByteOffset;
					At.Format = DXGI_FORMAT_R32_FLOAT;
					At.SemanticIndex = It.SemanticIndex;
					At.InputSlot = It.Slot;
					At.InstanceDataStepRate = Step;
					At.InputSlotClass = Class;

					switch (It.Format)
					{
						case Edge::Graphics::AttributeType::Byte:
							if (It.Components == 3)
							{
								ED_ERR("[d3d11] no 24bit support format for this type");
								return nullptr;
							}
							else if (It.Components == 1)
								At.Format = DXGI_FORMAT_R8_SNORM;
							else if (It.Components == 2)
								At.Format = DXGI_FORMAT_R8G8_SNORM;
							else if (It.Components == 4)
								At.Format = DXGI_FORMAT_R8G8B8A8_SNORM;
							break;
						case Edge::Graphics::AttributeType::Ubyte:
							if (It.Components == 3)
							{
								ED_ERR("[d3d11] no 24bit support format for this type");
								return nullptr;
							}
							else if (It.Components == 1)
								At.Format = DXGI_FORMAT_R8_UNORM;
							else if (It.Components == 2)
								At.Format = DXGI_FORMAT_R8G8_UNORM;
							else if (It.Components == 4)
								At.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
							break;
						case Edge::Graphics::AttributeType::Half:
							if (It.Components == 1)
								At.Format = DXGI_FORMAT_R16_FLOAT;
							else if (It.Components == 2)
								At.Format = DXGI_FORMAT_R16G16_FLOAT;
							else if (It.Components == 3)
								At.Format = DXGI_FORMAT_R11G11B10_FLOAT;
							else if (It.Components == 4)
								At.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
							break;
						case Edge::Graphics::AttributeType::Float:
							if (It.Components == 1)
								At.Format = DXGI_FORMAT_R32_FLOAT;
							else if (It.Components == 2)
								At.Format = DXGI_FORMAT_R32G32_FLOAT;
							else if (It.Components == 3)
								At.Format = DXGI_FORMAT_R32G32B32_FLOAT;
							else if (It.Components == 4)
								At.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
							break;
						case Edge::Graphics::AttributeType::Int:
							if (It.Components == 1)
								At.Format = DXGI_FORMAT_R32_SINT;
							else if (It.Components == 2)
								At.Format = DXGI_FORMAT_R32G32_SINT;
							else if (It.Components == 3)
								At.Format = DXGI_FORMAT_R32G32B32_SINT;
							else if (It.Components == 4)
								At.Format = DXGI_FORMAT_R32G32B32A32_SINT;
							break;
						case Edge::Graphics::AttributeType::Uint:
							if (It.Components == 1)
								At.Format = DXGI_FORMAT_R32_UINT;
							else if (It.Components == 2)
								At.Format = DXGI_FORMAT_R32G32_UINT;
							else if (It.Components == 3)
								At.Format = DXGI_FORMAT_R32G32B32_UINT;
							else if (It.Components == 4)
								At.Format = DXGI_FORMAT_R32G32B32A32_UINT;
							break;
						default:
							break;
					}

					Result.push_back(std::move(At));
				}

				if (Context->CreateInputLayout(Result.data(), (unsigned int)Result.size(), Shader->Signature->GetBufferPointer(), Shader->Signature->GetBufferSize(), &Shader->VertexLayout) != S_OK)
					ED_ERR("[d3d11] couldn't generate input layout for specified shader");

				return Shader->VertexLayout;
			}
			int D3D11Device::CreateConstantBuffer(ID3D11Buffer** fBuffer, size_t Size)
			{
				ED_ASSERT(fBuffer != nullptr, -1, "buffers ptr should be set");

				D3D11_BUFFER_DESC Description;
				ZeroMemory(&Description, sizeof(Description));
				Description.Usage = D3D11_USAGE_DEFAULT;
				Description.ByteWidth = (unsigned int)Size;
				Description.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
				Description.CPUAccessFlags = 0;

				return Context->CreateBuffer(&Description, nullptr, fBuffer);
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