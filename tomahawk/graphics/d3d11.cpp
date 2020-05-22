#include "d3d11.h"
#include "../resource.h"
#include <imgui.h>
#ifdef THAWK_HAS_SDL2
#include <SDL2/SDL_syswm.h>
#endif
#ifdef THAWK_MICROSOFT
#define ReleaseCom(Value) { if (Value != nullptr) { Value->Release(); Value = nullptr; } }

namespace Tomahawk
{
    namespace Graphics
    {
        namespace D3D11
        {
            D3D11Shader::D3D11Shader(Graphics::GraphicsDevice* Device, const Desc& F) : Graphics::Shader(Device, F)
            {
                D3D11Device* Ref = Device->As<D3D11Device>();
                ID3DBlob* ShaderBlob = nullptr;
                ID3DBlob* ErrorBlob = nullptr;
                VertexShader = nullptr;
                PixelShader = nullptr;
                GeometryShader = nullptr;
                HullShader = nullptr;
                DomainShader = nullptr;
                ComputeShader = nullptr;
                VertexLayout = nullptr;
                ConstantBuffer = nullptr;

                Desc I(F);
                Ref->ProcessShaderCode(I);
                Rest::Stroke Code(&I.Data);

                UInt64 Length = Code.Size();
                bool VS = Code.Find("VS").Found;
                bool PS = Code.Find("PS").Found;
                bool GS = Code.Find("GS").Found;
                bool DS = Code.Find("DS").Found;
                bool HS = Code.Find("HS").Found;
                bool CS = Code.Find("CS").Found;

                D3D11_INPUT_ELEMENT_DESC* ShaderLayout = I.LayoutSize <= 0 ? nullptr : new D3D11_INPUT_ELEMENT_DESC[I.LayoutSize];
                for (Int64 i = 0; i < I.LayoutSize; i++)
                {
                    ShaderLayout[i].SemanticName = I.Layout[i].SemanticName;
                    ShaderLayout[i].Format = (DXGI_FORMAT)I.Layout[i].FormatMode;
                    ShaderLayout[i].AlignedByteOffset = I.Layout[i].AlignedByteOffset;
                    ShaderLayout[i].SemanticIndex = I.Layout[i].SemanticIndex;
                    ShaderLayout[i].InputSlot = ShaderLayout[i].InstanceDataStepRate = 0;
                    ShaderLayout[i].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
                    Layout.push_back(I.Layout[i]);
                }

                if (VS)
                {
                    ShaderBlob = nullptr;
                    D3DCompile(Code.Get(), (SIZE_T)Length * sizeof(char), I.Filename.empty() ? nullptr : I.Filename.c_str(), nullptr, nullptr, "VS", Ref->GetVSProfile(), Ref->GetCompilationFlags(), 0, &ShaderBlob, &ErrorBlob);
                    if (Ref->CompileState(ErrorBlob))
                    {
                        if (ShaderLayout != nullptr)
                            delete[] ShaderLayout;

                        std::string Message = Ref->CompileState(ErrorBlob);
                        ReleaseCom(ErrorBlob);
                        THAWK_ERROR("couldn't compile vertex shader -> %s", Message.c_str());
                        return;
                    }

                    Ref->D3DDevice->CreateVertexShader(ShaderBlob->GetBufferPointer(), ShaderBlob->GetBufferSize(), nullptr, &VertexShader);
                    if (ShaderLayout != nullptr && I.LayoutSize != 0)
                    {
                        Ref->D3DDevice->CreateInputLayout(ShaderLayout, I.LayoutSize, ShaderBlob->GetBufferPointer(), ShaderBlob->GetBufferSize(), &VertexLayout);
                        delete[] ShaderLayout;
                        ShaderLayout = nullptr;
                    }
                    ReleaseCom(ShaderBlob);
                }

                if (PS)
                {
                    ShaderBlob = nullptr;
                    D3DCompile(Code.Get(), (SIZE_T)Length * sizeof(char), I.Filename.empty() ? nullptr : I.Filename.c_str(), nullptr, nullptr, "PS", Ref->GetPSProfile(), Ref->GetCompilationFlags(), 0, &ShaderBlob, &ErrorBlob);
                    if (Ref->CompileState(ErrorBlob))
                    {
                        std::string Message = Ref->CompileState(ErrorBlob);
                        ReleaseCom(ErrorBlob);
                        THAWK_ERROR("couldn't compile pixel shader -> %s", Message.c_str());
                        return;
                    }

                    Ref->D3DDevice->CreatePixelShader(ShaderBlob->GetBufferPointer(), ShaderBlob->GetBufferSize(), nullptr, &PixelShader);
                    ReleaseCom(ShaderBlob);
                }

                if (GS)
                {
                    ShaderBlob = nullptr;
                    D3DCompile(Code.Get(), (SIZE_T)Length * sizeof(char), I.Filename.empty() ? nullptr : I.Filename.c_str(), nullptr, nullptr, "GS", Ref->GetGSProfile(), Ref->GetCompilationFlags(), 0, &ShaderBlob, &ErrorBlob);
                    if (Ref->CompileState(ErrorBlob))
                    {
                        std::string Message = Ref->CompileState(ErrorBlob);
                        ReleaseCom(ErrorBlob);
                        THAWK_ERROR("couldn't compile geometry shader -> %s", Message.c_str());
                        return;
                    }

                    Ref->D3DDevice->CreateGeometryShader(ShaderBlob->GetBufferPointer(), ShaderBlob->GetBufferSize(), nullptr, &GeometryShader);
                    ReleaseCom(ShaderBlob);
                }

                if (CS)
                {
                    ShaderBlob = nullptr;
                    D3DCompile(Code.Get(), (SIZE_T)Length * sizeof(char), I.Filename.empty() ? nullptr : I.Filename.c_str(), nullptr, nullptr, "CS", Ref->GetCSProfile(), Ref->GetCompilationFlags(), 0, &ShaderBlob, &ErrorBlob);
                    if (Ref->CompileState(ErrorBlob))
                    {
                        std::string Message = Ref->CompileState(ErrorBlob);
                        ReleaseCom(ErrorBlob);
                        THAWK_ERROR("couldn't compile compute shader -> %s", Message.c_str());
                        return;
                    }

                    Ref->D3DDevice->CreateComputeShader(ShaderBlob->GetBufferPointer(), ShaderBlob->GetBufferSize(), nullptr, &ComputeShader);
                    ReleaseCom(ShaderBlob);
                }

                if (HS)
                {
                    ShaderBlob = nullptr;
                    D3DCompile(Code.Get(), (SIZE_T)Length * sizeof(char), I.Filename.empty() ? nullptr : I.Filename.c_str(), nullptr, nullptr, "HS", Ref->GetHSProfile(), Ref->GetCompilationFlags(), 0, &ShaderBlob, &ErrorBlob);
                    if (Ref->CompileState(ErrorBlob))
                    {
                        std::string Message = Ref->CompileState(ErrorBlob);
                        ReleaseCom(ErrorBlob);
                        THAWK_ERROR("couldn't compile hull shader -> %s", Message.c_str());
                        return;
                    }

                    Ref->D3DDevice->CreateHullShader(ShaderBlob->GetBufferPointer(), ShaderBlob->GetBufferSize(), nullptr, &HullShader);
                    ReleaseCom(ShaderBlob);
                }

                if (DS)
                {
                    ShaderBlob = nullptr;
                    D3DCompile(Code.Get(), (SIZE_T)Length * sizeof(char), I.Filename.empty() ? nullptr : I.Filename.c_str(), nullptr, nullptr, "DS", Ref->GetDSProfile(), Ref->GetCompilationFlags(), 0, &ShaderBlob, &ErrorBlob);
                    if (Ref->CompileState(ErrorBlob))
                    {
                        std::string Message = Ref->CompileState(ErrorBlob);
                        ReleaseCom(ErrorBlob);
                        THAWK_ERROR("couldn't compile domain shader -> %s", Message.c_str());
                        return;
                    }

                    Ref->D3DDevice->CreateDomainShader(ShaderBlob->GetBufferPointer(), ShaderBlob->GetBufferSize(), nullptr, &DomainShader);
                    ReleaseCom(ShaderBlob);
                }

                if (ShaderLayout)
                    delete[] ShaderLayout;
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
            }
            void D3D11Shader::SendConstantStream(Graphics::GraphicsDevice* Device)
            {
                Device->As<D3D11Device>()->ImmediateContext->UpdateSubresource(ConstantBuffer, 0, nullptr, ConstantData, 0, 0);
            }
            void D3D11Shader::Apply(Graphics::GraphicsDevice* Device)
            {
                ID3D11DeviceContext* ImmediateContext = Device->As<D3D11Device>()->ImmediateContext;
                if (ConstantBuffer && ConstantData)
                    ImmediateContext->UpdateSubresource(ConstantBuffer, 0, nullptr, ConstantData, 0, 0);

                if (VertexShader)
                {
                    ImmediateContext->VSSetShader(VertexShader, nullptr, 0);
                    ImmediateContext->VSSetConstantBuffers(4, 1, &ConstantBuffer);
                }

                if (PixelShader)
                {
                    ImmediateContext->PSSetShader(PixelShader, nullptr, 0);
                    ImmediateContext->PSSetConstantBuffers(4, 1, &ConstantBuffer);
                }

                if (GeometryShader)
                {
                    ImmediateContext->GSSetShader(GeometryShader, nullptr, 0);
                    ImmediateContext->GSSetConstantBuffers(4, 1, &ConstantBuffer);
                }

                if (HullShader)
                {
                    ImmediateContext->HSSetShader(HullShader, nullptr, 0);
                    ImmediateContext->HSSetConstantBuffers(4, 1, &ConstantBuffer);
                }

                if (DomainShader)
                {
                    ImmediateContext->DSSetShader(DomainShader, nullptr, 0);
                    ImmediateContext->DSSetConstantBuffers(4, 1, &ConstantBuffer);
                }

                if (ComputeShader)
                {
                    ImmediateContext->CSSetShader(ComputeShader, nullptr, 0);
                    ImmediateContext->CSSetConstantBuffers(4, 1, &ConstantBuffer);
                }

                ImmediateContext->IASetInputLayout(VertexLayout);
            }

            D3D11ElementBuffer::D3D11ElementBuffer(Graphics::GraphicsDevice* Device, const Desc& I) : Graphics::ElementBuffer(Device, I)
            {
                D3D11_BUFFER_DESC Buffer;
                ZeroMemory(&Buffer, sizeof(Buffer));
                Buffer.Usage = (D3D11_USAGE)I.Usage;
                Buffer.ByteWidth = (unsigned int)Elements * I.ElementWidth;
                Buffer.BindFlags = I.BindFlags;
                Buffer.CPUAccessFlags = I.AccessFlags;
                Buffer.MiscFlags = I.MiscFlags;
                Buffer.StructureByteStride = I.StructureByteStride;

                if (I.UseSubresource)
                {
                    D3D11_SUBRESOURCE_DATA Subresource;
                    ZeroMemory(&Subresource, sizeof(Subresource));
                    Subresource.pSysMem = I.Elements;

                    Device->As<D3D11Device>()->D3DDevice->CreateBuffer(&Buffer, &Subresource, &Element);
                }
                else
                    Device->As<D3D11Device>()->D3DDevice->CreateBuffer(&Buffer, nullptr, &Element);
            }
            D3D11ElementBuffer::~D3D11ElementBuffer()
            {
                ReleaseCom(Element);
            }
            void D3D11ElementBuffer::IndexedBuffer(Graphics::GraphicsDevice* Device, Graphics::Format Format, unsigned int Offset)
            {
                Device->As<D3D11Device>()->ImmediateContext->IASetIndexBuffer(Element, (DXGI_FORMAT)Format, Offset);
            }
            void D3D11ElementBuffer::VertexBuffer(Graphics::GraphicsDevice* Device, unsigned int Slot, unsigned int Stride, unsigned int Offset)
            {
                Device->As<D3D11Device>()->ImmediateContext->IASetVertexBuffers(Slot, 1, &Element, &Stride, &Offset);
            }
            void D3D11ElementBuffer::Map(Graphics::GraphicsDevice* Device, Graphics::ResourceMap Mode, Graphics::MappedSubresource* Map)
            {
                D3D11_MAPPED_SUBRESOURCE MappedResource;
                Device->As<D3D11Device>()->ImmediateContext->Map(Element, 0, (D3D11_MAP)Mode, 0, &MappedResource);

                Map->Pointer = MappedResource.pData;
                Map->RowPitch = MappedResource.RowPitch;
                Map->DepthPitch = MappedResource.DepthPitch;
            }
            void D3D11ElementBuffer::Unmap(Graphics::GraphicsDevice* Device, Graphics::MappedSubresource* Map)
            {
                Device->As<D3D11Device>()->ImmediateContext->Unmap(Element, 0);
            }
            void* D3D11ElementBuffer::GetResource()
            {
                return (void*)Element;
            }

            D3D11StructureBuffer::D3D11StructureBuffer(Graphics::GraphicsDevice* Device, const Desc& I) : Graphics::StructureBuffer(Device, I)
            {
                D3D11_BUFFER_DESC Buffer;
                ZeroMemory(&Buffer, sizeof(Buffer));
                Buffer.Usage = (D3D11_USAGE)I.Usage;
                Buffer.ByteWidth = (unsigned int)Elements * I.ElementWidth;
                Buffer.BindFlags = I.BindFlags;
                Buffer.CPUAccessFlags = I.AccessFlags;
                Buffer.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
                Buffer.StructureByteStride = I.ElementWidth;
                if (Device->As<D3D11Device>()->D3DDevice->CreateBuffer(&Buffer, nullptr, &Element) != 0)
                {
                    THAWK_ERROR("couldn't create structure buffer");
                    return;
                }

                D3D11_SHADER_RESOURCE_VIEW_DESC SRV;
                ZeroMemory(&SRV, sizeof(SRV));
                SRV.Format = DXGI_FORMAT_UNKNOWN;
                SRV.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
                SRV.Buffer.ElementWidth = I.ElementCount;

                if (Device->As<D3D11Device>()->D3DDevice->CreateShaderResourceView(Element, &SRV, &Resource) != S_OK)
                    THAWK_ERROR("couldn't create shader resource view");
            }
            D3D11StructureBuffer::~D3D11StructureBuffer()
            {
                ReleaseCom(Element);
                ReleaseCom(Resource);
            }
            void D3D11StructureBuffer::Map(Graphics::GraphicsDevice* Device, Graphics::ResourceMap Mode, Graphics::MappedSubresource* Map)
            {
                D3D11_MAPPED_SUBRESOURCE MappedResource;
                Device->As<D3D11Device>()->ImmediateContext->Map(Element, 0, (D3D11_MAP)Mode, 0, &MappedResource);

                Map->Pointer = MappedResource.pData;
                Map->RowPitch = MappedResource.RowPitch;
                Map->DepthPitch = MappedResource.DepthPitch;
            }
            void D3D11StructureBuffer::RemapSubresource(Graphics::GraphicsDevice* Device, void* Pointer, UInt64 Size)
            {
                D3D11Device* RefDevice = Device->As<D3D11Device>();

                D3D11_MAPPED_SUBRESOURCE MappedResource;
                RefDevice->ImmediateContext->Map(Element, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
                memcpy(MappedResource.pData, Pointer, (size_t)Size);
                RefDevice->ImmediateContext->Unmap(Element, 0);
            }
            void D3D11StructureBuffer::Unmap(Graphics::GraphicsDevice* Device, Graphics::MappedSubresource* Map)
            {
                Device->As<D3D11Device>()->ImmediateContext->Unmap(Element, 0);
            }
            void D3D11StructureBuffer::Apply(Graphics::GraphicsDevice* Device, int Slot)
            {
                Device->As<D3D11Device>()->ImmediateContext->PSSetShaderResources(Slot, 1, &Resource);
            }
            void* D3D11StructureBuffer::GetElement()
            {
                return (void*)Element;
            }
            void* D3D11StructureBuffer::GetResource()
            {
                return (void*)Resource;
            }

            D3D11Texture2D::D3D11Texture2D(Graphics::GraphicsDevice* Device) : Graphics::Texture2D(Device), Resource(nullptr), Rest(nullptr)
            {
            }
            D3D11Texture2D::D3D11Texture2D(Graphics::GraphicsDevice* Device, const Desc& I) : Graphics::Texture2D(Device, I), Resource(nullptr), Rest(nullptr)
            {
                if (I.Dealloc)
                    return;

                D3D11Device* Ref = Device->As<D3D11Device>();
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

                int VMipLevels = I.MipLevels;
                if (VMipLevels == -10)
                {
                    unsigned int Support = 0;
                    Ref->D3DDevice->CheckFormatSupport(Description.Format, &Support);
                    if ((Support & D3D11_FORMAT_SUPPORT_MIP_AUTOGEN))
                    {
                        Description.MipLevels = 0;
                        Description.BindFlags |= D3D11_BIND_RENDER_TARGET;
                        Description.MiscFlags |= D3D11_RESOURCE_MISC_GENERATE_MIPS;
                    }
                    else
                    {
                        VMipLevels = 1;
                        Description.MipLevels = 1;
                    }
                }

                D3D11_SUBRESOURCE_DATA Data;
                Data.pSysMem = I.Data;
                Data.SysMemPitch = I.RowPitch;
                Data.SysMemSlicePitch = I.DepthPitch;

                if (Ref->D3DDevice->CreateTexture2D(&Description, (I.Data && VMipLevels != -10) ? &Data : nullptr, &Rest) != S_OK)
                {
                    THAWK_ERROR("couldn't create 2d resource");
                    return;
                }

                D3D11_SHADER_RESOURCE_VIEW_DESC SRVDescription = { };
                SRVDescription.Format = (DXGI_FORMAT)I.FormatMode;
                SRVDescription.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
                SRVDescription.Texture2D.MipLevels = (VMipLevels == -10 ? -1 : VMipLevels);

                if (Ref->D3DDevice->CreateShaderResourceView(Rest, &SRVDescription, &Resource) != S_OK)
                {
                    THAWK_ERROR("couldn't create shader resource view");
                    return;
                }

                if (VMipLevels == -10)
                {
                    Ref->ImmediateContext->UpdateSubresource(Rest, 0, nullptr, I.Data, I.RowPitch, I.DepthPitch);
                    Ref->ImmediateContext->GenerateMips(Resource);
                }

                Rest->GetDesc(&Description);
                FormatMode = (Graphics::Format)Description.Format;
                Usage = (Graphics::ResourceUsage)Description.Usage;
                Width = Description.Width;
                Height = Description.Height;
                MipLevels = Description.MipLevels;
                AccessFlags = (Graphics::CPUAccess)Description.CPUAccessFlags;
            }
            D3D11Texture2D::~D3D11Texture2D()
            {
                ReleaseCom(Rest);
                ReleaseCom(Resource);
            }
            void D3D11Texture2D::Apply(Graphics::GraphicsDevice* Device, int Slot)
            {
                Device->As<D3D11Device>()->ImmediateContext->PSSetShaderResources(Slot, 1, &Resource);
            }
            void D3D11Texture2D::Fill(D3D11Device* Device)
            {
                if (!Resource && Rest)
                {
                    D3D11_TEXTURE2D_DESC Information;
                    Rest->GetDesc(&Information);

                    FormatMode = (Graphics::Format)Information.Format;
                    Usage = (Graphics::ResourceUsage)Information.Usage;
                    Width = Information.Width;
                    Height = Information.Height;
                    MipLevels = Information.MipLevels;
                    AccessFlags = (Graphics::CPUAccess)Information.CPUAccessFlags;

                    D3D11_SHADER_RESOURCE_VIEW_DESC SRV;
                    ZeroMemory(&SRV, sizeof(SRV));
                    SRV.Format = Information.Format;
                    SRV.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
                    SRV.TextureCube.MipLevels = 1;
                    SRV.TextureCube.MostDetailedMip = 0;

                    Device->D3DDevice->CreateShaderResourceView(Rest, &SRV, &Resource);
                    if (!Resource)
                        THAWK_ERROR("couldn't create shader resource view");
                }
                else if (Resource && !Rest)
                {
                    ID3D11Resource* Texture = nullptr;
                    Resource->GetResource(&Texture);

                    Texture->QueryInterface<ID3D11Texture2D>(&Rest);
                    if (Rest)
                    {
                        D3D11_TEXTURE2D_DESC Information;
                        Rest->GetDesc(&Information);

                        FormatMode = (Graphics::Format)Information.Format;
                        Usage = (Graphics::ResourceUsage)Information.Usage;
                        Width = Information.Width;
                        Height = Information.Height;
                        MipLevels = Information.MipLevels;
                        AccessFlags = (Graphics::CPUAccess)Information.CPUAccessFlags;
                        ReleaseCom(Texture);
                    }
                    else
                    {
                        ReleaseCom(Texture);
                        THAWK_ERROR("couldn't fetch texture 2d");
                    }
                }
                else if (Resource && Rest)
                {
                    D3D11_TEXTURE2D_DESC Information;
                    Rest->GetDesc(&Information);

                    FormatMode = (Graphics::Format)Information.Format;
                    Usage = (Graphics::ResourceUsage)Information.Usage;
                    Width = Information.Width;
                    Height = Information.Height;
                    MipLevels = Information.MipLevels;
                    AccessFlags = (Graphics::CPUAccess)Information.CPUAccessFlags;
                }
            }
            void D3D11Texture2D::Generate(D3D11Device* Device)
            {
                D3D11_TEXTURE2D_DESC Information;
                Rest->GetDesc(&Information);

                FormatMode = (Graphics::Format)Information.Format;
                Usage = (Graphics::ResourceUsage)Information.Usage;
                Width = Information.Width;
                Height = Information.Height;
                MipLevels = Information.MipLevels;
                AccessFlags = (Graphics::CPUAccess)Information.CPUAccessFlags;

                D3D11_SHADER_RESOURCE_VIEW_DESC SRV;
                ZeroMemory(&SRV, sizeof(SRV));
                SRV.Format = Information.Format;
                SRV.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
                SRV.TextureCube.MipLevels = 1;
                SRV.TextureCube.MostDetailedMip = 0;
                Device->D3DDevice->CreateShaderResourceView(Rest, &SRV, &Resource);

                if (!Resource)
                    THAWK_ERROR("couldn't create shader resource view");
            }
            void* D3D11Texture2D::GetResource()
            {
                return (void*)Resource;
            }

            D3D11Texture3D::D3D11Texture3D(Graphics::GraphicsDevice* Device) : Graphics::Texture3D(Device)
            {
                Resource = nullptr;
                Rest = nullptr;
            }
            D3D11Texture3D::~D3D11Texture3D()
            {
                ReleaseCom(Rest);
                ReleaseCom(Resource);
            }
            void D3D11Texture3D::Apply(Graphics::GraphicsDevice* Device, int Slot)
            {
                Device->As<D3D11Device>()->ImmediateContext->PSSetShaderResources(Slot, 1, &Resource);
            }
            void* D3D11Texture3D::GetResource()
            {
                return (void*)Resource;
            }

            D3D11TextureCube::D3D11TextureCube(Graphics::GraphicsDevice* Device) : Graphics::TextureCube(Device)
            {
                Resource = nullptr;
                Rest = nullptr;
            }
            D3D11TextureCube::D3D11TextureCube(Graphics::GraphicsDevice* Device, const Desc& I) : Graphics::TextureCube(Device, I)
            {
                Resource = nullptr;
                Rest = nullptr;

                D3D11Device* RefDevice = Device->As<D3D11Device>();

                D3D11_TEXTURE2D_DESC Texture2D;
                ((ID3D11Texture2D*)I.Texture2D)->GetDesc(&Texture2D);
                Texture2D.MipLevels = 1;
                Texture2D.ArraySize = 6;
                Texture2D.Usage = D3D11_USAGE_DEFAULT;
                Texture2D.BindFlags = D3D11_BIND_SHADER_RESOURCE;
                Texture2D.CPUAccessFlags = 0;
                Texture2D.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;

                if (RefDevice->D3DDevice->CreateTexture2D(&Texture2D, 0, &Rest) != S_OK)
                {
                    THAWK_ERROR("couldn't create texture 2d");
                    return;
                }

                D3D11_BOX Region;
                Region.left = 0;
                Region.right = Texture2D.Width;
                Region.top = 0;
                Region.bottom = Texture2D.Height;
                Region.front = 0;
                Region.back = 1;

                for (unsigned int j = 0; j < 6; j++)
                    RefDevice->ImmediateContext->CopySubresourceRegion(Rest, j, 0, 0, 0, (ID3D11Texture2D*)I.Texture2D[j], 0, &Region);

                D3D11_SHADER_RESOURCE_VIEW_DESC SRV;
                SRV.Format = Texture2D.Format;
                SRV.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
                SRV.TextureCube.MostDetailedMip = 0;
                SRV.TextureCube.MipLevels = Texture2D.MipLevels;

                if (Device->As<D3D11Device>()->D3DDevice->CreateShaderResourceView(Rest, &SRV, &Resource) != S_OK)
                {
                    THAWK_ERROR("couldn't create shader resource view");
                    return;
                }
            }
            D3D11TextureCube::~D3D11TextureCube()
            {
                ReleaseCom(Rest);
                ReleaseCom(Resource);
            }
            void D3D11TextureCube::Apply(Graphics::GraphicsDevice* Device, int Slot)
            {
                Device->As<D3D11Device>()->ImmediateContext->PSSetShaderResources(Slot, 1, &Resource);
            }
            void* D3D11TextureCube::GetResource()
            {
                return (void*)Resource;
            }

            D3D11RenderTarget2D::D3D11RenderTarget2D(Graphics::GraphicsDevice* Device, const Desc& I) : Graphics::RenderTarget2D(Device, I)
            {
                D3D11Device* Ref = Device->As<D3D11Device>();
                unsigned int MipLevels = (I.MipLevels < 1 ? 1 : I.MipLevels);
                Viewport = { 0, 0, 512, 512, 0, 1 };
                RenderTargetView = nullptr;
                DepthStencilView = nullptr;
                Texture = nullptr;

                if (I.RenderSurface == nullptr)
                {
                    D3D11_TEXTURE2D_DESC Information;
                    ZeroMemory(&Information, sizeof(Information));
                    Information.Width = I.Width;
                    Information.Height = I.Height;
                    Information.MipLevels = MipLevels;
                    Information.ArraySize = 1;
                    Information.Format = (DXGI_FORMAT)I.FormatMode;
                    Information.SampleDesc.Count = 1;
                    Information.SampleDesc.Quality = 0;
                    Information.Usage = (D3D11_USAGE)I.Usage;
                    Information.BindFlags = I.BindFlags;
                    Information.CPUAccessFlags = I.AccessFlags;
                    Information.MiscFlags = (unsigned int)I.MiscFlags;

                    if (Ref->D3DDevice->CreateTexture2D(&Information, nullptr, &Texture) != S_OK)
                    {
                        THAWK_ERROR("couldn't create surface texture view");
                        return;
                    }

                    D3D11_SHADER_RESOURCE_VIEW_DESC SRV;
                    SRV.Format = (DXGI_FORMAT)I.FormatMode;
                    SRV.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
                    SRV.Texture2D.MostDetailedMip = 0;
                    SRV.Texture2D.MipLevels = MipLevels;

                    Resource = Graphics::Texture2D::Create(Ref);
                    if (Ref->D3DDevice->CreateShaderResourceView(Texture, &SRV, &Resource->As<D3D11Texture2D>()->Resource) != S_OK)
                    {
                        THAWK_ERROR("couldn't create shader resource view");
                        return;
                    }
                }

                D3D11_RENDER_TARGET_VIEW_DESC RTV;
                RTV.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
                RTV.Texture2DArray.MipSlice = 0;
                RTV.Texture2DArray.ArraySize = 1;
                RTV.Format = (DXGI_FORMAT)I.FormatMode;

                if (Ref->D3DDevice->CreateRenderTargetView(I.RenderSurface ? (ID3D11Texture2D*)I.RenderSurface : Texture, &RTV, &RenderTargetView) != S_OK)
                {
                    THAWK_ERROR("couldn't create render target view");
                    return;
                }

                D3D11_TEXTURE2D_DESC DepthBuffer;
                ZeroMemory(&DepthBuffer, sizeof(DepthBuffer));
                DepthBuffer.Width = I.Width;
                DepthBuffer.Height = I.Height;
                DepthBuffer.MipLevels = 1;
                DepthBuffer.ArraySize = 1;
                DepthBuffer.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
                DepthBuffer.SampleDesc.Count = 1;
                DepthBuffer.SampleDesc.Quality = 0;
                DepthBuffer.Usage = (D3D11_USAGE)I.Usage;
                DepthBuffer.BindFlags = D3D11_BIND_DEPTH_STENCIL;
                DepthBuffer.CPUAccessFlags = I.AccessFlags;
                DepthBuffer.MiscFlags = 0;

                ID3D11Texture2D* DepthTexture = nullptr;
                if (Ref->D3DDevice->CreateTexture2D(&DepthBuffer, nullptr, &DepthTexture) != S_OK)
                {
                    THAWK_ERROR("couldn't create depth buffer texture 2d");
                    return;
                }

                D3D11_DEPTH_STENCIL_VIEW_DESC DSV;
                ZeroMemory(&DSV, sizeof(DSV));
                DSV.Format = DepthBuffer.Format;
                DSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
                DSV.Texture2D.MipSlice = 0;

                if (Ref->D3DDevice->CreateDepthStencilView(DepthTexture, &DSV, &DepthStencilView) != S_OK)
                {
                    THAWK_ERROR("couldn't create depth stencil view");
                    return;
                }

                ReleaseCom(DepthTexture);
                Viewport.Width = (FLOAT)I.Width;
                Viewport.Height = (FLOAT)I.Height;
                Viewport.MinDepth = 0.0f;
                Viewport.MaxDepth = 1.0f;
                Viewport.TopLeftX = 0.0f;
                Viewport.TopLeftY = 0.0f;
            }
            D3D11RenderTarget2D::~D3D11RenderTarget2D()
            {
                ReleaseCom(Texture);
                ReleaseCom(DepthStencilView);
                ReleaseCom(RenderTargetView);
            }
            void D3D11RenderTarget2D::Apply(Graphics::GraphicsDevice* Device, float R, float G, float B)
            {
                float ClearColor[4] = { R, G, B, 0.0f };

                ID3D11DeviceContext* ImmediateContext = Device->As<D3D11Device>()->ImmediateContext;
                ImmediateContext->OMSetRenderTargets(1, &RenderTargetView, DepthStencilView);
                ImmediateContext->RSSetViewports(1, &Viewport);
                ImmediateContext->ClearRenderTargetView(RenderTargetView, ClearColor);
                ImmediateContext->ClearDepthStencilView(DepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
            }
            void D3D11RenderTarget2D::Apply(Graphics::GraphicsDevice* Device)
            {
                ID3D11DeviceContext* ImmediateContext = Device->As<D3D11Device>()->ImmediateContext;
                ImmediateContext->OMSetRenderTargets(1, &RenderTargetView, DepthStencilView);
                ImmediateContext->RSSetViewports(1, &Viewport);
            }
            void D3D11RenderTarget2D::Clear(Graphics::GraphicsDevice* Device, float R, float G, float B)
            {
                float ClearColor[4] = { R, G, B, 0.0f };

                ID3D11DeviceContext* ImmediateContext = Device->As<D3D11Device>()->ImmediateContext;
                ImmediateContext->ClearRenderTargetView(RenderTargetView, ClearColor);
                ImmediateContext->ClearDepthStencilView(DepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
            }
            void D3D11RenderTarget2D::CopyTexture2D(Graphics::GraphicsDevice* Device, Graphics::Texture2D** Value)
            {
                if (!Texture)
                    return;

                D3D11_TEXTURE2D_DESC Information;
                Texture->GetDesc(&Information);

                Information.ArraySize = 1;
                Information.CPUAccessFlags = 0;
                Information.MiscFlags = 0;
                Information.MipLevels = 1;

                D3D11Texture2D* RefTexture = (D3D11Texture2D*)Graphics::Texture2D::Create(Device);
                D3D11Device* RefDevice = Device->As<D3D11Device>();

                RefDevice->D3DDevice->CreateTexture2D(&Information, nullptr, &RefTexture->Rest);
                RefDevice->ImmediateContext->CopyResource(RefTexture->Rest, Texture);
                *Value = RefTexture;
            }
            void D3D11RenderTarget2D::SetViewport(const Graphics::Viewport& In)
            {
                Viewport.Height = In.Height;
                Viewport.TopLeftX = In.TopLeftX;
                Viewport.TopLeftY = In.TopLeftY;
                Viewport.Width = In.Width;
                Viewport.MinDepth = In.MinDepth;
                Viewport.MaxDepth = In.MaxDepth;
            }
            Graphics::Viewport D3D11RenderTarget2D::GetViewport()
            {
                Graphics::Viewport Output;
                Output.TopLeftX = Viewport.TopLeftX;
                Output.TopLeftY = Viewport.TopLeftY;
                Output.Width = Viewport.Width;
                Output.Height = Viewport.Height;
                Output.MinDepth = Viewport.MinDepth;
                Output.MaxDepth = Viewport.MaxDepth;

                return Output;
            }
            float D3D11RenderTarget2D::GetWidth()
            {
                return Viewport.Width;
            }
            float D3D11RenderTarget2D::GetHeight()
            {
                return Viewport.Height;
            }
            void* D3D11RenderTarget2D::GetResource()
            {
                return Resource->GetResource();
            }

            D3D11MultiRenderTarget2D::D3D11MultiRenderTarget2D(Graphics::GraphicsDevice* Device, const Desc& I) : Graphics::MultiRenderTarget2D(Device, I)
            {
                D3D11Device* Ref = Device->As<D3D11Device>();
                unsigned int MipLevels = (I.MipLevels < 1 ? 1 : I.MipLevels);
                Viewport = { 0, 0, 512, 512, 0, 1 };
                DepthStencilView = nullptr;

                for (int i = 0; i < 8; i++)
                {
                    RenderTargetView[i] = nullptr;
                    Texture[i] = nullptr;
                }

                ZeroMemory(&Information, sizeof(Information));
                Information.Width = I.Width;
                Information.Height = I.Height;
                Information.MipLevels = MipLevels;
                Information.ArraySize = 1;
                Information.SampleDesc.Count = 1;
                Information.SampleDesc.Quality = 0;
                Information.Usage = (D3D11_USAGE)I.Usage;
                Information.BindFlags = I.BindFlags;
                Information.CPUAccessFlags = I.AccessFlags;
                Information.MiscFlags = (unsigned int)I.MiscFlags;

                D3D11_RENDER_TARGET_VIEW_DESC RTV;
                RTV.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
                RTV.Texture2DArray.MipSlice = 0;
                RTV.Texture2DArray.ArraySize = 1;
                if (Information.SampleDesc.Count > 1)
                    RTV.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMS;

                for (int i = 0; i < SVTarget; i++)
                {
                    Information.Format = (DXGI_FORMAT)I.FormatMode[i];
                    if (Ref->D3DDevice->CreateTexture2D(&Information, nullptr, &Texture[i]) == S_OK)
                        continue;

                    THAWK_ERROR("couldn't create surface texture 2d #%i", i);
                    return;
                }

                for (int i = 0; i < SVTarget; i++)
                {
                    RTV.Format = (DXGI_FORMAT)I.FormatMode[i];
                    if (Ref->D3DDevice->CreateRenderTargetView(Texture[i], &RTV, &RenderTargetView[i]) == S_OK)
                        continue;

                    THAWK_ERROR("couldn't create render target view #%i", i);
                    return;
                }

                D3D11_SHADER_RESOURCE_VIEW_DESC SRV;
                SRV.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
                SRV.Texture2D.MostDetailedMip = 0;
                SRV.Texture2D.MipLevels = MipLevels;

                if (Information.SampleDesc.Count > 1)
                    SRV.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMS;

                for (int i = 0; i < SVTarget; i++)
                {
                    Graphics::Texture2D::Desc F;
                    F.AccessFlags = I.AccessFlags;
                    F.FormatMode = I.FormatMode[i];
                    F.Height = I.Height;
                    F.Width = I.Width;
                    F.MipLevels = MipLevels;
                    F.Usage = I.Usage;
                    F.Dealloc = true;

                    Resource[i] = (D3D11Texture2D*)Graphics::Texture2D::Create(Ref, F);
                    SRV.Format = (DXGI_FORMAT)I.FormatMode[i];
                    if (Ref->D3DDevice->CreateShaderResourceView(Texture[i], &SRV, &Resource[i]->As<D3D11Texture2D>()->Resource) == S_OK)
                        continue;

                    THAWK_ERROR("couldn't create shader resource view #%i", i);
                    return;
                }

                D3D11_TEXTURE2D_DESC DepthBuffer;
                ZeroMemory(&DepthBuffer, sizeof(DepthBuffer));
                DepthBuffer.Width = I.Width;
                DepthBuffer.Height = I.Height;
                DepthBuffer.MipLevels = 1;
                DepthBuffer.ArraySize = 1;
                DepthBuffer.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
                DepthBuffer.SampleDesc.Count = 1;
                DepthBuffer.SampleDesc.Quality = 0;
                DepthBuffer.Usage = (D3D11_USAGE)I.Usage;
                DepthBuffer.BindFlags = D3D11_BIND_DEPTH_STENCIL;
                DepthBuffer.CPUAccessFlags = I.AccessFlags;
                DepthBuffer.MiscFlags = 0;

                ID3D11Texture2D* DepthTexture = nullptr;
                if (Ref->D3DDevice->CreateTexture2D(&DepthBuffer, nullptr, &DepthTexture) != S_OK)
                {
                    THAWK_ERROR("couldn't create depth buffer 2d");
                    return;
                }

                D3D11_DEPTH_STENCIL_VIEW_DESC DSV;
                ZeroMemory(&DSV, sizeof(DSV));
                DSV.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
                DSV.Texture2D.MipSlice = 0;
                DSV.ViewDimension = (Information.SampleDesc.Count > 1 ? D3D11_DSV_DIMENSION_TEXTURE2DMS : D3D11_DSV_DIMENSION_TEXTURE2D);

                if (Ref->D3DDevice->CreateDepthStencilView(DepthTexture, &DSV, &DepthStencilView) != S_OK)
                {
                    THAWK_ERROR("couldn't create depth stencil view");
                    return;
                }

                ReleaseCom(DepthTexture);
                Viewport.Width = (FLOAT)I.Width;
                Viewport.Height = (FLOAT)I.Height;
                Viewport.MinDepth = 0.0f;
                Viewport.MaxDepth = 1.0f;
                Viewport.TopLeftX = 0.0f;
                Viewport.TopLeftY = 0.0f;
            }
            D3D11MultiRenderTarget2D::~D3D11MultiRenderTarget2D()
            {
                ReleaseCom(DepthStencilView);
                for (int i = 0; i < 8; i++)
                {
                    ReleaseCom(Texture[i]);
                    ReleaseCom(RenderTargetView[i]);
                }
            }
            void D3D11MultiRenderTarget2D::Apply(Graphics::GraphicsDevice* Device, int Target, float R, float G, float B)
            {
                float ClearColor[4] = { R, G, B, 0.0f };

                ID3D11DeviceContext* ImmediateContext = Device->As<D3D11Device>()->ImmediateContext;
                ImmediateContext->OMSetRenderTargets(SVTarget, &RenderTargetView[Target], DepthStencilView);
                ImmediateContext->RSSetViewports(1, &Viewport);

                ImmediateContext->ClearRenderTargetView(RenderTargetView[Target], ClearColor);
                ImmediateContext->ClearDepthStencilView(DepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
            }
            void D3D11MultiRenderTarget2D::Apply(Graphics::GraphicsDevice* Device, int Target)
            {
                ID3D11DeviceContext* ImmediateContext = Device->As<D3D11Device>()->ImmediateContext;
                ImmediateContext->OMSetRenderTargets(SVTarget, &RenderTargetView[Target], DepthStencilView);
                ImmediateContext->RSSetViewports(1, &Viewport);
            }
            void D3D11MultiRenderTarget2D::Apply(Graphics::GraphicsDevice* Device, float R, float G, float B)
            {
                float ClearColor[4] = { R, G, B, 0.0f };

                ID3D11DeviceContext* ImmediateContext = Device->As<D3D11Device>()->ImmediateContext;
                ImmediateContext->OMSetRenderTargets(SVTarget, RenderTargetView, DepthStencilView);
                ImmediateContext->RSSetViewports(1, &Viewport);

                for (int i = 0; i < SVTarget; i++)
                    ImmediateContext->ClearRenderTargetView(RenderTargetView[i], ClearColor);
                ImmediateContext->ClearDepthStencilView(DepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
            }
            void D3D11MultiRenderTarget2D::Apply(Graphics::GraphicsDevice* Device)
            {
                ID3D11DeviceContext* ImmediateContext = Device->As<D3D11Device>()->ImmediateContext;
                ImmediateContext->OMSetRenderTargets(SVTarget, RenderTargetView, DepthStencilView);
                ImmediateContext->RSSetViewports(1, &Viewport);
            }
            void D3D11MultiRenderTarget2D::Clear(Graphics::GraphicsDevice* Device, int Target, float R, float G, float B)
            {
                float ClearColor[4] = { R, G, B, 0.0f };

                ID3D11DeviceContext* ImmediateContext = Device->As<D3D11Device>()->ImmediateContext;
                for (int i = 0; i < SVTarget; i++)
                    ImmediateContext->ClearRenderTargetView(RenderTargetView[i], ClearColor);
                ImmediateContext->ClearDepthStencilView(DepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
            }
            void D3D11MultiRenderTarget2D::CopyTexture2D(int Target, Graphics::GraphicsDevice* Device, Graphics::Texture2D** Value)
            {
                D3D11_TEXTURE2D_DESC T2D;
                Texture[Target]->GetDesc(&T2D);

                T2D.ArraySize = 1;
                T2D.CPUAccessFlags = 0;
                T2D.MiscFlags = 0;
                T2D.MipLevels = 1;

                D3D11Texture2D* RefTexture = (D3D11Texture2D*)Graphics::Texture2D::Create(Device);
                D3D11Device* RefDevice = Device->As<D3D11Device>();

                RefDevice->D3DDevice->CreateTexture2D(&T2D, nullptr, &RefTexture->Rest);
                RefDevice->ImmediateContext->CopyResource(RefTexture->Rest, Texture[Target]);
                *Value = RefTexture;
            }
            void D3D11MultiRenderTarget2D::SetViewport(const Graphics::Viewport& In)
            {
                Viewport.Height = In.Height;
                Viewport.TopLeftX = In.TopLeftX;
                Viewport.TopLeftY = In.TopLeftY;
                Viewport.Width = In.Width;
                Viewport.MinDepth = In.MinDepth;
                Viewport.MaxDepth = In.MaxDepth;
            }
            Graphics::Viewport D3D11MultiRenderTarget2D::GetViewport()
            {
                Graphics::Viewport Output;
                Output.TopLeftX = Viewport.TopLeftX;
                Output.TopLeftY = Viewport.TopLeftY;
                Output.Width = Viewport.Width;
                Output.Height = Viewport.Height;
                Output.MinDepth = Viewport.MinDepth;
                Output.MaxDepth = Viewport.MaxDepth;

                return Output;
            }
            float D3D11MultiRenderTarget2D::GetWidth()
            {
                return Viewport.Width;
            }
            float D3D11MultiRenderTarget2D::GetHeight()
            {
                return Viewport.Height;
            }
            void* D3D11MultiRenderTarget2D::GetResource(int Id)
            {
                return Resource[Id]->GetResource();
            }

            D3D11RenderTarget2DArray::D3D11RenderTarget2DArray(Graphics::GraphicsDevice* Device, const Desc& I) : Graphics::RenderTarget2DArray(Device, I)
            {
                D3D11Device* Ref = Device->As<D3D11Device>();
                unsigned int MipLevels = (I.MipLevels < 1 ? 1 : I.MipLevels);
                Viewport = { 0, 0, 512, 512, 0, 1 };
                DepthStencilView = nullptr;
                Texture = nullptr;

                ZeroMemory(&Information, sizeof(Information));
                Information.Width = I.Width;
                Information.Height = I.Height;
                Information.MipLevels = MipLevels;
                Information.ArraySize = I.ArraySize;
                Information.SampleDesc.Count = 1;
                Information.SampleDesc.Quality = 0;
                Information.Usage = (D3D11_USAGE)I.Usage;
                Information.BindFlags = I.BindFlags;
                Information.CPUAccessFlags = I.AccessFlags;
                Information.MiscFlags = (unsigned int)I.MiscFlags;
                Information.Format = (DXGI_FORMAT)I.FormatMode;

                if (Ref->D3DDevice->CreateTexture2D(&Information, nullptr, &Texture) != S_OK)
                {
                    THAWK_ERROR("couldn't create surface texture 2d");
                    return;
                }

                D3D11_RENDER_TARGET_VIEW_DESC RTV;
                RTV.Format = (DXGI_FORMAT)I.FormatMode;
                RTV.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
                RTV.Texture2DArray.MipSlice = 0;
                RTV.Texture2DArray.ArraySize = 1;

                if (Information.SampleDesc.Count > 1)
                {
                    RTV.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMSARRAY;
                    RTV.Texture2DMSArray.ArraySize = RTV.Texture2DArray.ArraySize;
                    RTV.Texture2DArray.ArraySize = 0;
                }

                for (int i = 0; i < (int)I.ArraySize; i++)
                {
                    if (Information.SampleDesc.Count > 1)
                        RTV.Texture2DMSArray.FirstArraySlice = i;
                    else
                        RTV.Texture2DArray.FirstArraySlice = i;

                    this->RenderTargetView.push_back(nullptr);
                    if (Ref->D3DDevice->CreateRenderTargetView(Texture, &RTV, &RenderTargetView[i]) == S_OK)
                        continue;

                    THAWK_ERROR("couldn't create render target view #%i", i);
                    return;
                }

                D3D11_SHADER_RESOURCE_VIEW_DESC SRV;
                SRV.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
                SRV.Texture2DArray.FirstArraySlice = 0;
                SRV.Texture2DArray.MostDetailedMip = 0;
                SRV.Texture2DArray.MipLevels = MipLevels;
                SRV.Format = (DXGI_FORMAT)I.FormatMode;
                SRV.Texture2DArray.ArraySize = I.ArraySize;

                if (Information.SampleDesc.Count > 1)
                {
                    SRV.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMSARRAY;
                    SRV.Texture2DMSArray.ArraySize = SRV.Texture2DArray.ArraySize;
                    SRV.Texture2DArray.ArraySize = 0;
                }

                Resource = (D3D11Texture2D*)Graphics::Texture2D::Create(Ref);
                if (Ref->D3DDevice->CreateShaderResourceView(Texture, &SRV, &Resource->As<D3D11Texture2D>()->Resource) != S_OK)
                {
                    THAWK_ERROR("couldn't create shader resource view");
                    return;
                }

                D3D11_TEXTURE2D_DESC DepthBuffer;
                ZeroMemory(&DepthBuffer, sizeof(DepthBuffer));
                DepthBuffer.Width = I.Width;
                DepthBuffer.Height = I.Height;
                DepthBuffer.MipLevels = MipLevels;
                DepthBuffer.ArraySize = 1;
                DepthBuffer.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
                DepthBuffer.SampleDesc.Count = 1;
                DepthBuffer.SampleDesc.Quality = 0;
                DepthBuffer.Usage = (D3D11_USAGE)I.Usage;
                DepthBuffer.BindFlags = D3D11_BIND_DEPTH_STENCIL;
                DepthBuffer.CPUAccessFlags = I.AccessFlags;
                DepthBuffer.MiscFlags = 0;

                ID3D11Texture2D* DepthTexture = nullptr;
                if (Ref->D3DDevice->CreateTexture2D(&DepthBuffer, nullptr, &DepthTexture) != S_OK)
                {
                    THAWK_ERROR("couldn't create depth buffer texture 2d");
                    return;
                }

                D3D11_DEPTH_STENCIL_VIEW_DESC DSV;
                ZeroMemory(&DSV, sizeof(DSV));
                DSV.Format = DepthBuffer.Format;
                DSV.Texture2D.MipSlice = 0;
                DSV.ViewDimension = (Information.SampleDesc.Count > 1 ? D3D11_DSV_DIMENSION_TEXTURE2DMS : D3D11_DSV_DIMENSION_TEXTURE2D);

                if (Ref->D3DDevice->CreateDepthStencilView(DepthTexture, &DSV, &DepthStencilView) != S_OK)
                {
                    THAWK_ERROR("couldn't create depth stencil view");
                    return;
                }

                ReleaseCom(DepthTexture);
                Viewport.Width = (FLOAT)I.Width;
                Viewport.Height = (FLOAT)I.Height;
                Viewport.MinDepth = 0.0f;
                Viewport.MaxDepth = 1.0f;
                Viewport.TopLeftX = 0.0f;
                Viewport.TopLeftY = 0.0f;
            }
            D3D11RenderTarget2DArray::~D3D11RenderTarget2DArray()
            {
                ReleaseCom(Texture);
                ReleaseCom(DepthStencilView);

                for (int i = 0; i < RenderTargetView.size(); i++)
					ReleaseCom(RenderTargetView[i]);
            }
            void D3D11RenderTarget2DArray::Apply(Graphics::GraphicsDevice* Device, int Target, float R, float G, float B)
            {
                float ClearColor[4] = { R, G, B, 0.0f };

                ID3D11DeviceContext* ImmediateContext = Device->As<D3D11Device>()->ImmediateContext;
                ImmediateContext->OMSetRenderTargets(1, &RenderTargetView[Target], DepthStencilView);
                ImmediateContext->RSSetViewports(1, &Viewport);
                ImmediateContext->ClearRenderTargetView(RenderTargetView[Target], ClearColor);
                ImmediateContext->ClearDepthStencilView(DepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
            }
            void D3D11RenderTarget2DArray::Apply(Graphics::GraphicsDevice* Device, int Target)
            {
                ID3D11DeviceContext* ImmediateContext = Device->As<D3D11Device>()->ImmediateContext;
                ImmediateContext->OMSetRenderTargets(1, &RenderTargetView[Target], DepthStencilView);
                ImmediateContext->RSSetViewports(1, &Viewport);
            }
            void D3D11RenderTarget2DArray::Clear(Graphics::GraphicsDevice* Device, int Target, float R, float G, float B)
            {
                float ClearColor[4] = { R, G, B, 0.0f };

                ID3D11DeviceContext* ImmediateContext = Device->As<D3D11Device>()->ImmediateContext;
                ImmediateContext->ClearRenderTargetView(RenderTargetView[Target], ClearColor);
                ImmediateContext->ClearDepthStencilView(DepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
            }
            void D3D11RenderTarget2DArray::SetViewport(const Graphics::Viewport& In)
            {
                Viewport.Height = In.Height;
                Viewport.TopLeftX = In.TopLeftX;
                Viewport.TopLeftY = In.TopLeftY;
                Viewport.Width = In.Width;
                Viewport.MinDepth = In.MinDepth;
                Viewport.MaxDepth = In.MaxDepth;
            }
            Graphics::Viewport D3D11RenderTarget2DArray::GetViewport()
            {
                Graphics::Viewport Output;
                Output.TopLeftX = Viewport.TopLeftX;
                Output.TopLeftY = Viewport.TopLeftY;
                Output.Width = Viewport.Width;
                Output.Height = Viewport.Height;
                Output.MinDepth = Viewport.MinDepth;
                Output.MaxDepth = Viewport.MaxDepth;

                return Output;
            }
            float D3D11RenderTarget2DArray::GetWidth()
            {
                return Viewport.Width;
            }
            float D3D11RenderTarget2DArray::GetHeight()
            {
                return Viewport.Height;
            }
            void* D3D11RenderTarget2DArray::GetResource()
            {
                return Resource->GetResource();
            }

            D3D11RenderTargetCube::D3D11RenderTargetCube(Graphics::GraphicsDevice* Device, const Desc& I) : Graphics::RenderTargetCube(Device, I)
            {
                D3D11Device* Ref = Device->As<D3D11Device>();
                unsigned int MipLevels = (I.MipLevels < 1 ? 1 : I.MipLevels);
                Viewport = { 0, 0, 512, 512, 0, 1 };
                DepthStencilView = nullptr;
                RenderTargetView = nullptr;
                Texture = nullptr;
                Cube = nullptr;

                D3D11_TEXTURE2D_DESC Information;
                ZeroMemory(&Information, sizeof(Information));
                Information.Width = I.Size;
                Information.Height = I.Size;
                Information.MipLevels = MipLevels;
                Information.ArraySize = 6;
                Information.SampleDesc.Count = 1;
                Information.SampleDesc.Quality = 0;
                Information.Format = (DXGI_FORMAT)I.FormatMode;
                Information.Usage = (D3D11_USAGE)I.Usage;
                Information.BindFlags = I.BindFlags;
                Information.CPUAccessFlags = I.AccessFlags;
                Information.MiscFlags = (unsigned int)I.MiscFlags;

                if (Ref->D3DDevice->CreateTexture2D(&Information, nullptr, &Cube) != S_OK)
                {
                    THAWK_ERROR("couldn't create cube map texture 2d");
                    return;
                }

                D3D11_TEXTURE2D_DESC DepthBuffer;
                ZeroMemory(&DepthBuffer, sizeof(DepthBuffer));
                DepthBuffer.Width = I.Size;
                DepthBuffer.Height = I.Size;
                DepthBuffer.MipLevels = 1;
                DepthBuffer.ArraySize = 6;
                DepthBuffer.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
                DepthBuffer.SampleDesc.Count = 1;
                DepthBuffer.SampleDesc.Quality = 0;
                DepthBuffer.Usage = (D3D11_USAGE)I.Usage;
                DepthBuffer.BindFlags = D3D11_BIND_DEPTH_STENCIL;
                DepthBuffer.CPUAccessFlags = I.AccessFlags;
                DepthBuffer.MiscFlags = 0;

                ID3D11Texture2D* DepthTexture = nullptr;
                if (Ref->D3DDevice->CreateTexture2D(&DepthBuffer, nullptr, &DepthTexture) != S_OK)
                {
                    THAWK_ERROR("couldn't create depth buffer texture 2d");
                    return;
                }

                D3D11_DEPTH_STENCIL_VIEW_DESC DSV;
                ZeroMemory(&DSV, sizeof(DSV));
                DSV.Format = DepthBuffer.Format;
                DSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
                DSV.Texture2DArray.FirstArraySlice = 0;
                DSV.Texture2DArray.ArraySize = 6;
                DSV.Texture2DArray.MipSlice = 0;

                if (Ref->D3DDevice->CreateDepthStencilView(DepthTexture, &DSV, &DepthStencilView) != S_OK)
                {
                    THAWK_ERROR("couldn't create depth stencil view");
                    return;
                }
                ReleaseCom(DepthTexture);

                D3D11_RENDER_TARGET_VIEW_DESC RTV;
                ZeroMemory(&RTV, sizeof(RTV));
                RTV.Format = Information.Format;
                RTV.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
                RTV.Texture2DArray.FirstArraySlice = 0;
                RTV.Texture2DArray.ArraySize = 6;
                RTV.Texture2DArray.MipSlice = 0;

                if (Ref->D3DDevice->CreateRenderTargetView(Cube, &RTV, &RenderTargetView) != S_OK)
                {
                    THAWK_ERROR("couldn't create render target view");
                    return;
                }

                D3D11_SHADER_RESOURCE_VIEW_DESC SRV;
                ZeroMemory(&SRV, sizeof(SRV));
                SRV.Format = Information.Format;
                SRV.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
                SRV.TextureCube.MostDetailedMip = 0;
                SRV.TextureCube.MipLevels = MipLevels;

                Resource = (D3D11Texture2D*)Graphics::Texture2D::Create(Ref);
                if (Ref->D3DDevice->CreateShaderResourceView(Cube, &SRV, &Resource->As<D3D11Texture2D>()->Resource) != S_OK)
                {
                    THAWK_ERROR("couldn't create shader resource view");
                    return;
                }

                Viewport.Width = (FLOAT)I.Size;
                Viewport.Height = (FLOAT)I.Size;
                Viewport.MinDepth = 0.0f;
                Viewport.MaxDepth = 1.0f;
                Viewport.TopLeftX = 0.0f;
                Viewport.TopLeftY = 0.0f;
            }
            D3D11RenderTargetCube::~D3D11RenderTargetCube()
            {
                ReleaseCom(DepthStencilView);
                ReleaseCom(RenderTargetView);
                ReleaseCom(Texture);
                ReleaseCom(Cube);
            }
            void D3D11RenderTargetCube::Apply(Graphics::GraphicsDevice* Device, float R, float G, float B)
            {
                float ClearColor[4] = { R, G, B, 0.0f };

                ID3D11DeviceContext* ImmediateContext = Device->As<D3D11Device>()->ImmediateContext;
                ImmediateContext->OMSetRenderTargets(1, &RenderTargetView, DepthStencilView);
                ImmediateContext->RSSetViewports(1, &Viewport);
                ImmediateContext->ClearRenderTargetView(RenderTargetView, ClearColor);
                ImmediateContext->ClearDepthStencilView(DepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
            }
            void D3D11RenderTargetCube::Apply(Graphics::GraphicsDevice* Device)
            {
                ID3D11DeviceContext* ImmediateContext = Device->As<D3D11Device>()->ImmediateContext;
                ImmediateContext->OMSetRenderTargets(1, &RenderTargetView, DepthStencilView);
                ImmediateContext->RSSetViewports(1, &Viewport);
            }
            void D3D11RenderTargetCube::Clear(Graphics::GraphicsDevice* Device, float R, float G, float B)
            {
                float ClearColor[4] = { R, G, B, 0.0f };

                ID3D11DeviceContext* ImmediateContext = Device->As<D3D11Device>()->ImmediateContext;
                ImmediateContext->ClearRenderTargetView(RenderTargetView, ClearColor);
                ImmediateContext->ClearDepthStencilView(DepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
            }
            void D3D11RenderTargetCube::CopyTextureCube(Graphics::GraphicsDevice* Device, Graphics::TextureCube** Value)
            {
                D3D11Device* Ref = Device->As<D3D11Device>();
                ID3D11Texture2D* Resource = nullptr;

                D3D11_TEXTURE2D_DESC Information;
                Cube->GetDesc(&Information);

                Information.ArraySize = 1;
                Information.CPUAccessFlags = 0;
                Information.MiscFlags = 0;
                Information.MipLevels = 1;

                Graphics::TextureCube::Desc F = Graphics::TextureCube::Desc();
                for (int i = 0; i < 6; i++)
                {
                    Ref->D3DDevice->CreateTexture2D(&Information, nullptr, &Resource);
                    Ref->ImmediateContext->CopySubresourceRegion(Resource, i, 0, 0, 0, Cube, 0, 0);
                    F.Texture2D[i] = (void*)Resource;
                }

                *Value = Graphics::TextureCube::Create(Device, F);
            }
            void D3D11RenderTargetCube::CopyTexture2D(int FaceId, Graphics::GraphicsDevice* Device, Graphics::Texture2D** Value)
            {
                D3D11_TEXTURE2D_DESC Information;
                Cube->GetDesc(&Information);

                Information.ArraySize = 1;
                Information.CPUAccessFlags = 0;
                Information.MiscFlags = 0;
                Information.MipLevels = 1;

                D3D11Texture2D* RefTexture = (D3D11Texture2D*)Graphics::Texture2D::Create(Device);
                D3D11Device* RefDevice = Device->As<D3D11Device>();

                RefDevice->D3DDevice->CreateTexture2D(&Information, nullptr, &RefTexture->Rest);
                RefDevice->ImmediateContext->CopySubresourceRegion(RefTexture->Rest, FaceId * Information.MipLevels, 0, 0, 0, Cube, 0, 0);
                *Value = RefTexture;
            }
            void D3D11RenderTargetCube::SetViewport(const Graphics::Viewport& In)
            {
                Viewport.Height = In.Height;
                Viewport.TopLeftX = In.TopLeftX;
                Viewport.TopLeftY = In.TopLeftY;
                Viewport.Width = In.Width;
                Viewport.MinDepth = In.MinDepth;
                Viewport.MaxDepth = In.MaxDepth;
            }
            Graphics::Viewport D3D11RenderTargetCube::GetViewport()
            {
                Graphics::Viewport Output;
                Output.TopLeftX = Viewport.TopLeftX;
                Output.TopLeftY = Viewport.TopLeftY;
                Output.Width = Viewport.Width;
                Output.Height = Viewport.Height;
                Output.MinDepth = Viewport.MinDepth;
                Output.MaxDepth = Viewport.MaxDepth;

                return Output;
            }
            float D3D11RenderTargetCube::GetWidth()
            {
                return Viewport.Width;
            }
            float D3D11RenderTargetCube::GetHeight()
            {
                return Viewport.Height;
            }
            void* D3D11RenderTargetCube::GetResource()
            {
                return (void*)Resource->GetResource();
            }

            D3D11MultiRenderTargetCube::D3D11MultiRenderTargetCube(Graphics::GraphicsDevice* Device, const Desc& I) : Graphics::MultiRenderTargetCube(Device, I)
            {
                DepthStencilView = nullptr;
                Viewport = { 0, 0, 512, 512, 0, 1 };
                for (int i = 0; i < 8; i++)
                {
                    RenderTargetView[i] = nullptr;
                    Texture[i] = nullptr;
                    Resource[i] = nullptr;
                }

                D3D11Device* Ref = Device->As<D3D11Device>();
                unsigned int MipLevels = (I.MipLevels < 1 ? 1 : I.MipLevels);

                D3D11_TEXTURE2D_DESC Information;
                ZeroMemory(&Information, sizeof(Information));
                Information.Width = I.Size;
                Information.Height = I.Size;
                Information.MipLevels = 1;
                Information.ArraySize = 6;
                Information.SampleDesc.Count = 1;
                Information.SampleDesc.Quality = 0;
                Information.Format = DXGI_FORMAT_D32_FLOAT;
                Information.Usage = (D3D11_USAGE)I.Usage;
                Information.BindFlags = D3D11_BIND_DEPTH_STENCIL;
                Information.CPUAccessFlags = I.AccessFlags;
                Information.MiscFlags = (unsigned int)I.MiscFlags;

                for (int i = 0; i < SVTarget; i++)
                {
                    if (Ref->D3DDevice->CreateTexture2D(&Information, nullptr, &Texture[i]) != S_OK)
                    {
                        THAWK_ERROR("couldn't create texture 2d");
                        return;
                    }
                }

                D3D11_DEPTH_STENCIL_VIEW_DESC DSV;
                ZeroMemory(&DSV, sizeof(DSV));
                DSV.Format = Information.Format;
                DSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
                DSV.Texture2DArray.FirstArraySlice = 0;
                DSV.Texture2DArray.ArraySize = 6;
                DSV.Texture2DArray.MipSlice = 0;

                if (Ref->D3DDevice->CreateDepthStencilView(Texture[0], &DSV, &DepthStencilView) != S_OK)
                {
                    THAWK_ERROR("couldn't create depth stencil view");
                    return;
                }

                Information.BindFlags = I.BindFlags;
                Information.MiscFlags = (unsigned int)I.MiscFlags;
                Information.MipLevels = MipLevels;
                for (int i = 0; i < SVTarget; i++)
                {
                    Information.Format = (DXGI_FORMAT)I.FormatMode[i];
                    if (Ref->D3DDevice->CreateTexture2D(&Information, nullptr, &Cube[i]) != S_OK)
                    {
                        THAWK_ERROR("couldn't create cube map rexture 2d");
                        return;
                    }
                }

                D3D11_RENDER_TARGET_VIEW_DESC RTV;
                ZeroMemory(&RTV, sizeof(RTV));
                RTV.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
                RTV.Texture2DArray.FirstArraySlice = 0;
                RTV.Texture2DArray.ArraySize = 6;
                RTV.Texture2DArray.MipSlice = 0;

                for (int i = 0; i < SVTarget; i++)
                {
                    RTV.Format = (DXGI_FORMAT)I.FormatMode[i];
                    if (Ref->D3DDevice->CreateRenderTargetView(Cube[i], &RTV, &RenderTargetView[i]) != S_OK)
                    {
                        THAWK_ERROR("couldn't create render target view");
                        return;
                    }
                }

                D3D11_SHADER_RESOURCE_VIEW_DESC SRV;
                ZeroMemory(&SRV, sizeof(SRV));
                SRV.Format = Information.Format;
                SRV.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
                SRV.TextureCube.MostDetailedMip = 0;
                SRV.TextureCube.MipLevels = MipLevels;

                for (int i = 0; i < SVTarget; i++)
                {
                    SRV.Format = (DXGI_FORMAT)I.FormatMode[i];
                    Resource[i] = (D3D11Texture2D*)Graphics::Texture2D::Create(Ref);
                    if (Ref->D3DDevice->CreateShaderResourceView(Cube[i], &SRV, &Resource[i]->As<D3D11Texture2D>()->Resource) != S_OK)
                    {
                        THAWK_ERROR("couldn't create shader resource view");
                        return;
                    }
                }

                Viewport.Width = (FLOAT)I.Size;
                Viewport.Height = (FLOAT)I.Size;
                Viewport.MinDepth = 0.0f;
                Viewport.MaxDepth = 1.0f;
                Viewport.TopLeftX = 0.0f;
                Viewport.TopLeftY = 0.0f;
            }
            D3D11MultiRenderTargetCube::~D3D11MultiRenderTargetCube()
            {
                for (int i = 0; i < SVTarget; i++)
                {
                    ReleaseCom(RenderTargetView[i]);
                    ReleaseCom(Texture[i]);
                    ReleaseCom(Cube[i]);
                }
                ReleaseCom(DepthStencilView);
            }
            void D3D11MultiRenderTargetCube::Apply(Graphics::GraphicsDevice* Device, int Target, float R, float G, float B)
            {
                float ClearColor[4] = { R, G, B, 0.0f };

                ID3D11DeviceContext* ImmediateContext = Device->As<D3D11Device>()->ImmediateContext;
                ImmediateContext->OMSetRenderTargets(SVTarget, &RenderTargetView[Target], DepthStencilView);
                ImmediateContext->RSSetViewports(1, &Viewport);

                ImmediateContext->ClearRenderTargetView(RenderTargetView[Target], ClearColor);
                ImmediateContext->ClearDepthStencilView(DepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
            }
            void D3D11MultiRenderTargetCube::Apply(Graphics::GraphicsDevice* Device, int Target)
            {
                ID3D11DeviceContext* ImmediateContext = Device->As<D3D11Device>()->ImmediateContext;
                ImmediateContext->OMSetRenderTargets(SVTarget, &RenderTargetView[Target], DepthStencilView);
                ImmediateContext->RSSetViewports(1, &Viewport);
            }
            void D3D11MultiRenderTargetCube::Apply(Graphics::GraphicsDevice* Device, float R, float G, float B)
            {
                float ClearColor[4] = { R, G, B, 0.0f };

                ID3D11DeviceContext* ImmediateContext = Device->As<D3D11Device>()->ImmediateContext;
                ImmediateContext->OMSetRenderTargets(SVTarget, RenderTargetView, DepthStencilView);
                ImmediateContext->RSSetViewports(1, &Viewport);

                for (int i = 0; i < SVTarget; i++)
                    ImmediateContext->ClearRenderTargetView(RenderTargetView[i], ClearColor);
                ImmediateContext->ClearDepthStencilView(DepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
            }
            void D3D11MultiRenderTargetCube::Apply(Graphics::GraphicsDevice* Device)
            {
                ID3D11DeviceContext* ImmediateContext = Device->As<D3D11Device>()->ImmediateContext;
                ImmediateContext->OMSetRenderTargets(SVTarget, RenderTargetView, DepthStencilView);
                ImmediateContext->RSSetViewports(1, &Viewport);
            }
            void D3D11MultiRenderTargetCube::Clear(Graphics::GraphicsDevice* Device, int Target, float R, float G, float B)
            {
                float ClearColor[4] = { R, G, B, 0.0f };

                ID3D11DeviceContext* ImmediateContext = Device->As<D3D11Device>()->ImmediateContext;
                for (int i = 0; i < SVTarget; i++)
                    ImmediateContext->ClearRenderTargetView(RenderTargetView[i], ClearColor);
                ImmediateContext->ClearDepthStencilView(DepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
            }
            void D3D11MultiRenderTargetCube::CopyTextureCube(int CubeId, Graphics::GraphicsDevice* Device, Graphics::TextureCube** Value)
            {
                D3D11Device* Ref = Device->As<D3D11Device>();
                ID3D11Texture2D* Resource = nullptr;

                D3D11_TEXTURE2D_DESC Information;
                Cube[CubeId]->GetDesc(&Information);

                Information.ArraySize = 1;
                Information.CPUAccessFlags = 0;
                Information.MiscFlags = 0;
                Information.MipLevels = 1;

                Graphics::TextureCube::Desc F = Graphics::TextureCube::Desc();
                for (int i = 0; i < 6; i++)
                {
                    Ref->D3DDevice->CreateTexture2D(&Information, nullptr, &Resource);
                    Ref->ImmediateContext->CopySubresourceRegion(Resource, i, 0, 0, 0, Cube[CubeId], 0, 0);
                    F.Texture2D[i] = (void*)Resource;
                }

                *Value = Graphics::TextureCube::Create(Device, F);
            }
            void D3D11MultiRenderTargetCube::CopyTexture2D(int CubeId, int FaceId, Graphics::GraphicsDevice* Device, Graphics::Texture2D** Value)
            {
                D3D11_TEXTURE2D_DESC Information;
                Cube[CubeId]->GetDesc(&Information);

                Information.ArraySize = 1;
                Information.CPUAccessFlags = 0;
                Information.MiscFlags = 0;
                Information.MipLevels = 1;

                D3D11Texture2D* RefTexture = (D3D11Texture2D*)Graphics::Texture2D::Create(Device);
                D3D11Device* RefDevice = Device->As<D3D11Device>();

                RefDevice->D3DDevice->CreateTexture2D(&Information, nullptr, &RefTexture->Rest);
                RefDevice->ImmediateContext->CopySubresourceRegion(RefTexture->Rest, FaceId * Information.MipLevels, 0, 0, 0, Cube[CubeId], 0, 0);
                *Value = RefTexture;
            }
            void D3D11MultiRenderTargetCube::SetViewport(const Graphics::Viewport& In)
            {
                Viewport.Height = In.Height;
                Viewport.TopLeftX = In.TopLeftX;
                Viewport.TopLeftY = In.TopLeftY;
                Viewport.Width = In.Width;
                Viewport.MinDepth = In.MinDepth;
                Viewport.MaxDepth = In.MaxDepth;
            }
            Graphics::Viewport D3D11MultiRenderTargetCube::GetViewport()
            {
                Graphics::Viewport Output;
                Output.TopLeftX = Viewport.TopLeftX;
                Output.TopLeftY = Viewport.TopLeftY;
                Output.Width = Viewport.Width;
                Output.Height = Viewport.Height;
                Output.MinDepth = Viewport.MinDepth;
                Output.MaxDepth = Viewport.MaxDepth;

                return Output;
            }
            float D3D11MultiRenderTargetCube::GetWidth()
            {
                return Viewport.Width;
            }
            float D3D11MultiRenderTargetCube::GetHeight()
            {
                return Viewport.Height;
            }
            void* D3D11MultiRenderTargetCube::GetResource(int Id)
            {
                return (void*)Resource[Id]->GetResource();
            }

            D3D11Mesh::D3D11Mesh(Graphics::GraphicsDevice* Device, const Desc& I) : Graphics::Mesh(Device, I)
            {
                Graphics::ElementBuffer::Desc F = Graphics::ElementBuffer::Desc();
                F.AccessFlags = I.AccessFlags;
                F.Usage = I.Usage;
                F.BindFlags = Graphics::ResourceBind_Vertex_Buffer;
                F.ElementCount = (unsigned int)I.Elements.size();
                F.UseSubresource = true;
                F.Elements = (void*)I.Elements.data();
                F.ElementWidth = sizeof(Compute::Vertex);

                VertexBuffer = Graphics::ElementBuffer::Create(Device, F);

                F = Graphics::ElementBuffer::Desc();
                F.AccessFlags = I.AccessFlags;
                F.Usage = I.Usage;
                F.BindFlags = Graphics::ResourceBind_Index_Buffer;
                F.ElementCount = (unsigned int)I.Indices.size();
                F.ElementWidth = sizeof(int);
                F.Elements = (void*)I.Indices.data();
                F.UseSubresource = true;

                IndexBuffer = Graphics::ElementBuffer::Create(Device, F);
            }
            void D3D11Mesh::Update(Graphics::GraphicsDevice* Device, Compute::Vertex* Elements)
            {
                Graphics::MappedSubresource Resource;
                VertexBuffer->Map(Device, Graphics::ResourceMap_Write, &Resource);
                memcpy(Resource.Pointer, Elements, (size_t)VertexBuffer->GetElements() * sizeof(Compute::Vertex));
                VertexBuffer->Unmap(Device, &Resource);
            }
            void D3D11Mesh::Draw(Graphics::GraphicsDevice* Device)
            {
                ID3D11DeviceContext* ImmediateContext = Device->As<D3D11Device>()->ImmediateContext;
                D3D11ElementBuffer* ElementBuffer = VertexBuffer->As<D3D11ElementBuffer>();
                D3D11ElementBuffer* IndexedBuffer = IndexBuffer->As<D3D11ElementBuffer>();
                unsigned int Stride = Graphics::Shader::GetInfluenceVertexLayoutStride(), Offset = 0;

                ImmediateContext->IASetVertexBuffers(0, 1, &ElementBuffer->Element, &Stride, &Offset);
                ImmediateContext->IASetIndexBuffer(IndexedBuffer->Element, DXGI_FORMAT_R32_UINT, 0);
                ImmediateContext->DrawIndexed((unsigned int)IndexedBuffer->GetElements(), 0, 0);
            }
            Compute::Vertex* D3D11Mesh::Elements(Graphics::GraphicsDevice* Device)
            {
                Graphics::MappedSubresource Resource;
                VertexBuffer->Map(Device, Graphics::ResourceMap_Write, &Resource);

                Compute::Vertex* Vertices = new Compute::Vertex[(unsigned int)VertexBuffer->GetElements()];
                memcpy(Vertices, Resource.Pointer, (size_t)VertexBuffer->GetElements() * sizeof(Compute::Vertex));

                VertexBuffer->Unmap(Device, &Resource);
                return Vertices;
            }

            D3D11SkinnedMesh::D3D11SkinnedMesh(Graphics::GraphicsDevice* Device, const Desc& I) : Graphics::SkinnedMesh(Device, I)
            {
                Graphics::ElementBuffer::Desc F = Graphics::ElementBuffer::Desc();
                F.AccessFlags = I.AccessFlags;
                F.Usage = I.Usage;
                F.BindFlags = Graphics::ResourceBind_Vertex_Buffer;
                F.ElementCount = (unsigned int)I.Elements.size();
                F.UseSubresource = true;
                F.Elements = (void*)I.Elements.data();
                F.ElementWidth = sizeof(Compute::InfluenceVertex);

                VertexBuffer = Graphics::ElementBuffer::Create(Device, F);

                F = Graphics::ElementBuffer::Desc();
                F.AccessFlags = I.AccessFlags;
                F.Usage = I.Usage;
                F.BindFlags = Graphics::ResourceBind_Index_Buffer;
                F.ElementCount = (unsigned int)I.Indices.size();
                F.ElementWidth = sizeof(int);
                F.Elements = (void*)I.Indices.data();
                F.UseSubresource = true;

                IndexBuffer = Graphics::ElementBuffer::Create(Device, F);
            }
            void D3D11SkinnedMesh::Update(Graphics::GraphicsDevice* Device, Compute::InfluenceVertex* Elements)
            {
                Graphics::MappedSubresource Resource;
                VertexBuffer->Map(Device, Graphics::ResourceMap_Write, &Resource);
                memcpy(Resource.Pointer, Elements, (size_t)VertexBuffer->GetElements() * sizeof(Compute::InfluenceVertex));
                VertexBuffer->Unmap(Device, &Resource);
            }
            void D3D11SkinnedMesh::Draw(Graphics::GraphicsDevice* Device)
            {
                ID3D11DeviceContext* ImmediateContext = Device->As<D3D11Device>()->ImmediateContext;
                D3D11ElementBuffer* ElementBuffer = VertexBuffer->As<D3D11ElementBuffer>();
                D3D11ElementBuffer* IndexedBuffer = IndexBuffer->As<D3D11ElementBuffer>();
                unsigned int Stride = Graphics::Shader::GetInfluenceVertexLayoutStride(), Offset = 0;

                ImmediateContext->IASetVertexBuffers(0, 1, &ElementBuffer->Element, &Stride, &Offset);
                ImmediateContext->IASetIndexBuffer(IndexedBuffer->Element, DXGI_FORMAT_R32_UINT, 0);
                ImmediateContext->DrawIndexed((unsigned int)IndexedBuffer->GetElements(), 0, 0);
            }
            Compute::InfluenceVertex* D3D11SkinnedMesh::Elements(Graphics::GraphicsDevice* Device)
            {
                Graphics::MappedSubresource Resource;
                VertexBuffer->Map(Device, Graphics::ResourceMap_Write, &Resource);

                Compute::InfluenceVertex* Vertices = new Compute::InfluenceVertex[(unsigned int)VertexBuffer->GetElements()];
                memcpy(Vertices, Resource.Pointer, (size_t)VertexBuffer->GetElements() * sizeof(Compute::InfluenceVertex));

                VertexBuffer->Unmap(Device, &Resource);
                return Vertices;
            }

            D3D11InstanceBuffer::D3D11InstanceBuffer(Graphics::GraphicsDevice* NewDevice, const Desc& I) : Graphics::InstanceBuffer(NewDevice, I), Resource(nullptr)
            {
                Graphics::ElementBuffer::Desc F = Graphics::ElementBuffer::Desc();
                F.AccessFlags = Graphics::CPUAccess_Write;
                F.MiscFlags = Graphics::ResourceMisc_Buffer_Structured;
                F.Usage = Graphics::ResourceUsage_Dynamic;
                F.BindFlags = Graphics::ResourceBind_Shader_Input;
                F.ElementCount = (unsigned int)ElementLimit;
                F.ElementWidth = sizeof(Compute::ElementVertex);
                F.StructureByteStride = F.ElementWidth;
                F.UseSubresource = false;
                Elements = Graphics::ElementBuffer::Create(Device, F);

                D3D11_SHADER_RESOURCE_VIEW_DESC SRV;
                ZeroMemory(&SRV, sizeof(SRV));
                SRV.Format = DXGI_FORMAT_UNKNOWN;
                SRV.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
                SRV.Buffer.ElementWidth = (unsigned int)ElementLimit;

                if (Device->As<D3D11Device>()->D3DDevice->CreateShaderResourceView(Elements->As<D3D11ElementBuffer>()->Element, &SRV, &Resource) != S_OK)
                {
                    THAWK_ERROR("couldn't create shader resource view");
                    return;
                }
            }
            D3D11InstanceBuffer::~D3D11InstanceBuffer()
            {
                if (SynchronizationState)
                    Restore();

                ReleaseCom(Resource);
            }
            void D3D11InstanceBuffer::SendPool()
            {
                if (Array.Size() <= 0 || Array.Size() > ElementLimit)
                    return;
                else
                    SynchronizationState = true;

                D3D11Device* Dev = Device->As<D3D11Device>();
                D3D11ElementBuffer* RefElements = Elements->As<D3D11ElementBuffer>();

                D3D11_MAPPED_SUBRESOURCE MappedResource;
                Dev->ImmediateContext->Map(RefElements->Element, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
                memcpy(MappedResource.pData, Array.Get(), (size_t)Array.Size() * sizeof(Compute::ElementVertex));
                Dev->ImmediateContext->Unmap(RefElements->Element, 0);
            }
            void D3D11InstanceBuffer::Restore()
            {
                if (!SynchronizationState)
                    return;
                else
                    SynchronizationState = false;

                D3D11Device* Dev = Device->As<D3D11Device>();
                D3D11ElementBuffer* RefElements = Elements->As<D3D11ElementBuffer>();

                D3D11_MAPPED_SUBRESOURCE MappedResource;
                Dev->ImmediateContext->Map(RefElements->Element, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
                MappedResource.pData = nullptr;
                Dev->ImmediateContext->Unmap(RefElements->Element, 0);
            }
            void D3D11InstanceBuffer::Resize(UInt64 Size)
            {
                Restore();
                delete Elements;

                ReleaseCom(Resource);
                ElementLimit = Size + 1;
                if (ElementLimit < 1)
                    ElementLimit = 2;

                Array.Clear();
                Array.Reserve(ElementLimit);

                Graphics::ElementBuffer::Desc F = Graphics::ElementBuffer::Desc();
                F.AccessFlags = Graphics::CPUAccess_Write;
                F.MiscFlags = Graphics::ResourceMisc_Buffer_Structured;
                F.Usage = Graphics::ResourceUsage_Dynamic;
                F.BindFlags = Graphics::ResourceBind_Shader_Input;
                F.ElementCount = (unsigned int)ElementLimit;
                F.ElementWidth = sizeof(Compute::ElementVertex);
                F.StructureByteStride = F.ElementWidth;
                F.UseSubresource = false;

                Elements = Graphics::ElementBuffer::Create(Device, F);

                D3D11_SHADER_RESOURCE_VIEW_DESC SRV;
                ZeroMemory(&SRV, sizeof(SRV));
                SRV.Format = DXGI_FORMAT_UNKNOWN;
                SRV.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
                SRV.Buffer.ElementWidth = (unsigned int)ElementLimit;

                if (Device->As<D3D11Device>()->D3DDevice->CreateShaderResourceView(Elements->As<D3D11ElementBuffer>()->Element, &SRV, &Resource) != S_OK)
                {
                    THAWK_ERROR("couldn't create shader resource view");
                    return;
                }
            }

            D3D11DirectBuffer::D3D11DirectBuffer(Graphics::GraphicsDevice* NewDevice) : Graphics::DirectBuffer(NewDevice)
            {
                D3D11Device* Dev = Device->As<D3D11Device>();
                VertexShaderBlob = nullptr;
                VertexShader = nullptr;
                InputLayout = nullptr;
                VertexConstantBuffer = nullptr;
                PixelShaderBlob = nullptr;
                PixelShader = nullptr;
                ElementBuffer = nullptr;
                Elements.push_back(Vertex());

                D3D11_BUFFER_DESC Buffer;
                ZeroMemory(&Buffer, sizeof(Buffer));
                Buffer.Usage = D3D11_USAGE_DYNAMIC;
                Buffer.ByteWidth = (unsigned int)Elements.size() * sizeof(Vertex);
                Buffer.BindFlags = D3D11_BIND_VERTEX_BUFFER;
                Buffer.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

                D3D11_SUBRESOURCE_DATA InitData;
                ZeroMemory(&InitData, sizeof(InitData));
                InitData.pSysMem = &Elements[0];

                if (Dev->D3DDevice->CreateBuffer(&Buffer, &InitData, &ElementBuffer) != S_OK)
                {
                    THAWK_ERROR("couldn't create vertex buffer");
                    return;
                }

                static const char* VertexShaderCode = "cbuffer VertexBuffer : register(b0)"
                                                      "{"
                                                      "   matrix WorldViewProjection;"
                                                      "   float4 Padding;"
                                                      "}; "
                                                      "struct VS_INPUT"
                                                      "{"
                                                      "   float4 Position : POSITION0;"
                                                      "   float2 TexCoord : TEXCOORD0;"
                                                      "   float4 Color : COLOR0;"
                                                      "};"
                                                      "struct PS_INPUT"
                                                      "{"
                                                      "   float4 Position : SV_POSITION;"
                                                      "   float2 TexCoord : TEXCOORD0;"
                                                      "   float4 Color : COLOR0;"
                                                      "};"
                                                      "PS_INPUT VS (VS_INPUT Input)"
                                                      "{"
                                                      "   PS_INPUT Output;"
                                                      "   Output.Position = mul(WorldViewProjection, float4(Input.Position.xyz, 1));"
                                                      "   Output.Color = Input.Color;"
                                                      "   Output.TexCoord = Input.TexCoord;"
                                                      "   return Output;"
                                                      "}";

                static const char* PixelShaderCode = "cbuffer VertexBuffer : register(b0)"
                                                     "{"
                                                     "   matrix WorldViewProjection;"
                                                     "   float4 Padding;"
                                                     "};"
                                                     "struct PS_INPUT"
                                                     "{"
                                                     "   float4 Position : SV_POSITION;"
                                                     "   float2 TexCoord : TEXCOORD0;"
                                                     "   float4 Color : COLOR0;"
                                                     "};"
                                                     "sampler State;"
                                                     "Texture2D Diffuse;"
                                                     "float4 PS (PS_INPUT Input) : SV_TARGET0"
                                                     "{"
                                                     "   if (Padding.z > 0)"
                                                     "       return Input.Color * Diffuse.Sample(State, Input.TexCoord + Padding.xy) * Padding.w;"
                                                     "   return Input.Color * Padding.w;"
                                                     "}";

                D3DCompile(VertexShaderCode, strlen(VertexShaderCode), nullptr, nullptr, nullptr, "VS", Dev->GetVSProfile(), 0, 0, &VertexShaderBlob, nullptr);
                if (VertexShaderBlob == nullptr)
                {
                    THAWK_ERROR("couldn't compile vertex shader");
                    return;
                }

                if (Dev->D3DDevice->CreateVertexShader((DWORD*)VertexShaderBlob->GetBufferPointer(), VertexShaderBlob->GetBufferSize(), nullptr, &VertexShader) != S_OK)
                {
                    THAWK_ERROR("couldn't create vertex shader");
                    return;
                }

                D3D11_INPUT_ELEMENT_DESC Layout[] = {{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }, { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 3 * sizeof(float), D3D11_INPUT_PER_VERTEX_DATA, 0 }, { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 5 * sizeof(float), D3D11_INPUT_PER_VERTEX_DATA, 0 }, };

                if (Dev->D3DDevice->CreateInputLayout(Layout, 3, VertexShaderBlob->GetBufferPointer(), VertexShaderBlob->GetBufferSize(), &InputLayout) != S_OK)
                {
                    THAWK_ERROR("couldn't create input layout");
                    return;
                }

                Dev->CreateConstantBuffer(sizeof(ConstantBuffer), VertexConstantBuffer);
                if (!VertexConstantBuffer)
                {
                    THAWK_ERROR("couldn't create vertex constant buffer");
                    return;
                }

                D3DCompile(PixelShaderCode, strlen(PixelShaderCode), nullptr, nullptr, nullptr, "PS", Dev->GetPSProfile(), 0, 0, &PixelShaderBlob, nullptr);
                if (PixelShaderBlob == nullptr)
                {
                    THAWK_ERROR("couldn't compile pixel shader");
                    return;
                }

                if (Dev->D3DDevice->CreatePixelShader((DWORD*)PixelShaderBlob->GetBufferPointer(), PixelShaderBlob->GetBufferSize(), nullptr, &PixelShader) != S_OK)
                {
                    THAWK_ERROR("couldn't create pixel shader");
                    return;
                }
            }
            D3D11DirectBuffer::~D3D11DirectBuffer()
            {
                ReleaseCom(VertexShaderBlob);
                ReleaseCom(VertexShader);
                ReleaseCom(InputLayout);
                ReleaseCom(VertexConstantBuffer);
                ReleaseCom(PixelShaderBlob);
                ReleaseCom(PixelShader);
                ReleaseCom(ElementBuffer);
            }
            void D3D11DirectBuffer::Begin()
            {
                D3D11Device* Dev = Device->As<D3D11Device>();
                Dev->ImmediateContext->IASetInputLayout(InputLayout);
                Dev->ImmediateContext->VSSetShader(VertexShader, nullptr, 0);
                Dev->ImmediateContext->PSSetShader(PixelShader, nullptr, 0);
                Buffer.WorldViewProjection = Compute::Matrix4x4::Identity();
                Buffer.Padding = { 0, 0, 0, 1 };
                View = nullptr;
                Primitives = Graphics::PrimitiveTopology_Triangle_List;
                Elements.clear();
            }
            void D3D11DirectBuffer::End()
            {
                D3D11Device* Dev = Device->As<D3D11Device>();
                if (Elements.empty())
                    return;

                if (Elements.size() > MaxElements)
                    AllocVertexBuffer(Elements.size());

                D3D11_PRIMITIVE_TOPOLOGY LastTopology;
                Dev->ImmediateContext->IAGetPrimitiveTopology(&LastTopology);
                Dev->ImmediateContext->IASetPrimitiveTopology((D3D11_PRIMITIVE_TOPOLOGY)Primitives);

                ID3D11Buffer* First, * Second;
                Dev->ImmediateContext->VSGetConstantBuffers(0, 1, &First);
                Dev->ImmediateContext->PSGetConstantBuffers(0, 1, &Second);

                D3D11_MAPPED_SUBRESOURCE MappedResource;
                Dev->ImmediateContext->Map(ElementBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
                memcpy(MappedResource.pData, Elements.data(), (size_t)Elements.size() * sizeof(Vertex));
                Dev->ImmediateContext->Unmap(ElementBuffer, 0);

                if (View)
                    View->Apply(Device, 0);
                else
                    Device->RestoreTexture2D(0, 1);

                unsigned int Stride = sizeof(Vertex), Offset = 0;
                Dev->ImmediateContext->VSSetConstantBuffers(0, 1, &VertexConstantBuffer);
                Dev->ImmediateContext->PSSetConstantBuffers(0, 1, &VertexConstantBuffer);
                Dev->ImmediateContext->IASetVertexBuffers(0, 1, &ElementBuffer, &Stride, &Offset);
                Dev->ImmediateContext->UpdateSubresource(VertexConstantBuffer, 0, nullptr, &Buffer, 0, 0);
                Dev->ImmediateContext->Draw((unsigned int)Elements.size(), 0);
                Dev->ImmediateContext->IASetPrimitiveTopology(LastTopology);
                Dev->ImmediateContext->VSSetConstantBuffers(0, 1, &First);
                Dev->ImmediateContext->PSSetConstantBuffers(0, 1, &Second);
                ReleaseCom(First);
                ReleaseCom(Second);
            }
            void D3D11DirectBuffer::EmitVertex()
            {
                Elements.push_back({ 0, 0, 0, 0, 0, 1, 1, 1, 1 });
            }
            void D3D11DirectBuffer::Position(float X, float Y, float Z)
            {
                if (Elements.size() > 0)
                {
                    Elements[Elements.size() - 1].PX = X;
                    Elements[Elements.size() - 1].PY = Y;
                    Elements[Elements.size() - 1].PZ = Z;
                }
            }
            void D3D11DirectBuffer::TexCoord(float X, float Y)
            {
                if (Elements.size() > 0)
                {
                    Elements[Elements.size() - 1].TX = X;
                    Elements[Elements.size() - 1].TY = Y;
                }
            }
            void D3D11DirectBuffer::Color(float X, float Y, float Z, float W)
            {
                if (Elements.size() > 0)
                {
                    Elements[Elements.size() - 1].CX = X;
                    Elements[Elements.size() - 1].CY = Y;
                    Elements[Elements.size() - 1].CZ = Z;
                    Elements[Elements.size() - 1].CW = W;
                }
            }
            void D3D11DirectBuffer::Texture(Graphics::Texture2D* InView)
            {
                View = InView;
                Buffer.Padding.Z = 1;
            }
            void D3D11DirectBuffer::Intensity(float Intensity)
            {
                Buffer.Padding.W = Intensity;
            }
            void D3D11DirectBuffer::TexCoordOffset(float X, float Y)
            {
                Buffer.Padding.X = X;
                Buffer.Padding.Y = Y;
            }
            void D3D11DirectBuffer::Transform(Compute::Matrix4x4 Input)
            {
                Buffer.WorldViewProjection = Buffer.WorldViewProjection * Input;
            }
            void D3D11DirectBuffer::AllocVertexBuffer(UInt64 Size)
            {
                ReleaseCom(ElementBuffer);
                MaxElements = Size;

                D3D11_BUFFER_DESC Buffer;
                ZeroMemory(&Buffer, sizeof(Buffer));
                Buffer.Usage = D3D11_USAGE_DYNAMIC;
                Buffer.ByteWidth = (unsigned int)Size * sizeof(Vertex);
                Buffer.BindFlags = D3D11_BIND_VERTEX_BUFFER;
                Buffer.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

                D3D11_SUBRESOURCE_DATA InitData;
                ZeroMemory(&InitData, sizeof(InitData));
                InitData.pSysMem = &Elements[0];

                Device->As<D3D11Device>()->D3DDevice->CreateBuffer(&Buffer, &InitData, &ElementBuffer);
            }
            void D3D11DirectBuffer::Topology(Graphics::PrimitiveTopology DrawTopology)
            {
                Primitives = DrawTopology;
            }

            D3D11Device::D3D11Device(const Desc& I) : Graphics::GraphicsDevice(I), ImmediateContext(nullptr), SwapChain(nullptr), D3DDevice(nullptr)
            {
                DriverType = D3D_DRIVER_TYPE_HARDWARE;
                FeatureLevel = D3D_FEATURE_LEVEL_11_0;
				ConstantBuffer[0] = nullptr;
				ConstantBuffer[1] = nullptr;
				ConstantBuffer[2] = nullptr;

                unsigned int CreationFlags = I.CreationFlags;
                CreationFlags |= D3D11_CREATE_DEVICE_DISABLE_GPU_TIMEOUT;
                CreationFlags |= D3D11_CREATE_DEVICE_SINGLETHREADED;

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
				SwapChainResource.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
#if defined(THAWK_MICROSOFT) && defined(THAWK_HAS_SDL2)
                if (I.Window != nullptr)
                {
                    SDL_SysWMinfo Info;
                    I.Window->Load(&Info);
                    SwapChainResource.OutputWindow = (HWND)Info.info.win.window;
                }
#endif
                if (D3D11CreateDeviceAndSwapChain(nullptr, DriverType, nullptr, CreationFlags, FeatureLevels, ARRAYSIZE(FeatureLevels), D3D11_SDK_VERSION, &SwapChainResource, &SwapChain, &D3DDevice, &FeatureLevel, &ImmediateContext) != S_OK)
                {
                    THAWK_ERROR("couldn't create swap chain, device or immediate context");
                    return;
                }

                LoadShaderSections();
                CreateConstantBuffer(sizeof(Graphics::AnimationBuffer), ConstantBuffer[0]);
                ConstantData[0] = &Animation;

                CreateConstantBuffer(sizeof(Graphics::RenderBuffer), ConstantBuffer[1]);
                ConstantData[1] = &Render;

                CreateConstantBuffer(sizeof(Graphics::ViewBuffer), ConstantBuffer[2]);
                ConstantData[2] = &View;

                ImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
                ImmediateContext->VSSetConstantBuffers(0, 3, ConstantBuffer);
                ImmediateContext->PSSetConstantBuffers(0, 3, ConstantBuffer);
                ImmediateContext->GSSetConstantBuffers(0, 3, ConstantBuffer);
                ImmediateContext->DSSetConstantBuffers(0, 3, ConstantBuffer);
                ImmediateContext->HSSetConstantBuffers(0, 3, ConstantBuffer);
                ImmediateContext->CSSetConstantBuffers(0, 3, ConstantBuffer);

                SetShaderModel(I.ShaderMode == Graphics::ShaderModel_Auto ? GetSupportedShaderModel() : I.ShaderMode);
                ResizeBuffers(I.BufferWidth, I.BufferHeight);
                CreateRendererStates();

                Graphics::Shader::Desc F = Graphics::Shader::Desc();
                F.Layout = Graphics::Shader::GetShapeVertexLayout();
                F.LayoutSize = 2;
#ifdef HAS_D3D11_BASIC_EFFECT_HLSL
                F.Data = GET_RESOURCE_BATCH(d3d11_basic_effect_hlsl);
#else
                THAWK_ERROR("basic-effect.hlsl was not compiled");
#endif
                BasicEffect = Shader::Create(this, F);
            }
            D3D11Device::~D3D11Device()
            {
				RestoreSamplerStates();
				RestoreBlendStates();
				RestoreRasterizerStates();
				RestoreDepthStencilStates();

                for (int i = 0; i < 3; i++)
					ReleaseCom(ConstantBuffer[i]);

                ReleaseCom(ImmediateContext);
                ReleaseCom(SwapChain);
                ReleaseCom(D3DDevice);
            }
            void D3D11Device::RestoreSamplerStates()
            {
                for (int i = 0; i < SamplerStates.size(); i++)
                {
                    ID3D11SamplerState* State = (ID3D11SamplerState*)SamplerStates[i]->Pointer;
                    ReleaseCom(State);
                    delete SamplerStates[i];
                }
                SamplerStates.clear();
            }
            void D3D11Device::RestoreBlendStates()
            {
                for (int i = 0; i < BlendStates.size(); i++)
                {
                    ID3D11BlendState* State = (ID3D11BlendState*)BlendStates[i]->Pointer;
                    ReleaseCom(State);
                    delete BlendStates[i];
                }
                BlendStates.clear();
            }
            void D3D11Device::RestoreRasterizerStates()
            {
                for (int i = 0; i < RasterizerStates.size(); i++)
                {
                    ID3D11RasterizerState* State = (ID3D11RasterizerState*)RasterizerStates[i]->Pointer;
                    ReleaseCom(State);
                    delete RasterizerStates[i];
                }
                RasterizerStates.clear();
            }
            void D3D11Device::RestoreDepthStencilStates()
            {
                for (int i = 0; i < DepthStencilStates.size(); i++)
                {
                    ID3D11DepthStencilState* State = (ID3D11DepthStencilState*)DepthStencilStates[i]->Pointer;
                    ReleaseCom(State);
                    delete DepthStencilStates[i];
                }
                DepthStencilStates.clear();
            }
            void D3D11Device::RestoreState(Graphics::DeviceState* RefState)
            {
                if (!RefState)
                    return;

                D3D11DeviceState* State = (D3D11DeviceState*)RefState;
                ImmediateContext->CSSetConstantBuffers(0, 1, &State->CSConstantBuffer);
                ImmediateContext->CSSetSamplers(0, 1, &State->CSSampler);
                ImmediateContext->CSSetShader(State->CSShader, nullptr, 0);
                ImmediateContext->CSSetShaderResources(0, 1, &State->CSShaderResource);
                ImmediateContext->CSSetUnorderedAccessViews(0, 1, &State->CSUnorderedAccessView, &State->UAVInitialCounts);
                ImmediateContext->DSSetConstantBuffers(0, 1, &State->DSConstantBuffer);
                ImmediateContext->DSSetSamplers(0, 1, &State->DSSampler);
                ImmediateContext->DSSetShaderResources(0, 1, &State->DSShaderResource);
                ImmediateContext->DSSetShader(State->DSShader, nullptr, 0);
                ImmediateContext->GSSetConstantBuffers(0, 1, &State->GSConstantBuffer);
                ImmediateContext->GSSetSamplers(0, 1, &State->GSSampler);
                ImmediateContext->GSSetShaderResources(0, 1, &State->GSShaderResource);
                ImmediateContext->GSSetShader(State->GSShader, nullptr, 0);
                ImmediateContext->HSSetConstantBuffers(0, 1, &State->HSConstantBuffer);
                ImmediateContext->HSSetSamplers(0, 1, &State->HSSampler);
                ImmediateContext->HSSetShaderResources(0, 1, &State->HSShaderResource);
                ImmediateContext->HSSetShader(State->HSShader, nullptr, 0);
                ImmediateContext->PSSetConstantBuffers(0, 1, &State->PSConstantBuffer);
                ImmediateContext->PSSetSamplers(0, 1, &State->PSSampler);
                ImmediateContext->PSSetShaderResources(0, 1, &State->PSShaderResource);
                ImmediateContext->PSSetShader(State->PSShader, nullptr, 0);
                ImmediateContext->VSSetConstantBuffers(0, 1, &State->VSConstantBuffer);
                ImmediateContext->VSSetSamplers(0, 1, &State->VSSampler);
                ImmediateContext->VSSetShaderResources(0, 1, &State->VSShaderResource);
                ImmediateContext->VSSetShader(State->VSShader, nullptr, 0);
                ImmediateContext->IASetIndexBuffer(State->IndexBuffer, State->IDXGIFormat, State->IOffset);
                ImmediateContext->IASetInputLayout(State->InputLayout);
                ImmediateContext->IASetPrimitiveTopology(State->PrimitiveTopology);
                ImmediateContext->IASetVertexBuffers(0, 1, &State->VertexBuffer, &State->VStride, &State->VOffset);
                ImmediateContext->OMSetBlendState(State->BlendState, State->BlendFactor, State->SampleMask);
                ImmediateContext->OMSetDepthStencilState(State->DepthStencilState, State->StencilReference);
                ImmediateContext->OMSetRenderTargets(1, &State->RenderTargetView, State->DepthStencilView);
                ImmediateContext->RSSetScissorRects(State->RectCount, State->Rects);
                ImmediateContext->RSSetState(State->RasterizerState);
                ImmediateContext->RSSetViewports(State->ViewportCount, State->Viewports);
            }
            void D3D11Device::ReleaseState(Graphics::DeviceState** RefState)
            {
                if (!RefState || !*RefState)
                    return;

                D3D11DeviceState* State = (D3D11DeviceState*)*RefState;
                ReleaseCom(State->CSConstantBuffer);
                ReleaseCom(State->CSSampler);
                ReleaseCom(State->CSShader);
                ReleaseCom(State->CSShaderResource);
                ReleaseCom(State->CSUnorderedAccessView);
                ReleaseCom(State->DSConstantBuffer);
                ReleaseCom(State->DSSampler);
                ReleaseCom(State->DSShaderResource);
                ReleaseCom(State->DSShader);
                ReleaseCom(State->GSConstantBuffer);
                ReleaseCom(State->GSSampler);
                ReleaseCom(State->GSShaderResource);
                ReleaseCom(State->GSShader);
                ReleaseCom(State->HSConstantBuffer);
                ReleaseCom(State->HSSampler);
                ReleaseCom(State->HSShaderResource);
                ReleaseCom(State->HSShader);
                ReleaseCom(State->PSConstantBuffer);
                ReleaseCom(State->PSSampler);
                ReleaseCom(State->PSShaderResource);
                ReleaseCom(State->PSShader);
                ReleaseCom(State->VSConstantBuffer);
                ReleaseCom(State->VSSampler);
                ReleaseCom(State->VSShaderResource);
                ReleaseCom(State->VSShader);
                ReleaseCom(State->IndexBuffer);
                ReleaseCom(State->InputLayout);
                ReleaseCom(State->VertexBuffer);
                ReleaseCom(State->BlendState);
                ReleaseCom(State->DepthStencilState);
                ReleaseCom(State->RenderTargetView);
                ReleaseCom(State->DepthStencilView);
                ReleaseCom(State->RasterizerState);
                delete State;
                *RefState = nullptr;
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
            void D3D11Device::SetShaderModel(Graphics::ShaderModel RShaderModel)
            {
                ShaderModelType = RShaderModel;
                if (ShaderModelType == Graphics::ShaderModel_HLSL_1_0)
                {
                    VSP = "vs_1_0";
                    PSP = "ps_1_0";
                    GSP = "gs_1_0";
                    CSP = "cs_1_0";
                    DSP = "ds_1_0";
                    HSP = "hs_1_0";
                }
                else if (ShaderModelType == Graphics::ShaderModel_HLSL_2_0)
                {
                    VSP = "vs_2_0";
                    PSP = "ps_2_0";
                    GSP = "gs_2_0";
                    CSP = "cs_2_0";
                    DSP = "ds_2_0";
                    HSP = "hs_2_0";
                }
                else if (ShaderModelType == Graphics::ShaderModel_HLSL_3_0)
                {
                    VSP = "vs_3_0";
                    PSP = "ps_3_0";
                    GSP = "gs_3_0";
                    CSP = "cs_3_0";
                    DSP = "ds_3_0";
                    HSP = "hs_3_0";
                }
                else if (ShaderModelType == Graphics::ShaderModel_HLSL_4_0)
                {
                    VSP = "vs_4_0";
                    PSP = "ps_4_0";
                    GSP = "gs_4_0";
                    CSP = "cs_4_0";
                    DSP = "ds_4_0";
                    HSP = "hs_4_0";
                }
                else if (ShaderModelType == Graphics::ShaderModel_HLSL_4_1)
                {
                    VSP = "vs_4_1";
                    PSP = "ps_4_1";
                    GSP = "gs_4_1";
                    CSP = "cs_4_1";
                    DSP = "ds_4_1";
                    HSP = "hs_4_1";
                }
                else if (ShaderModelType == Graphics::ShaderModel_HLSL_5_0)
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
            UInt64 D3D11Device::AddDepthStencilState(Graphics::DepthStencilState* In)
            {
                D3D11_DEPTH_STENCIL_DESC State;
                State.BackFace.StencilDepthFailOp = (D3D11_STENCIL_OP)In->BackFaceStencilDepthFailOperation;
                State.BackFace.StencilFailOp = (D3D11_STENCIL_OP)In->BackFaceStencilFailOperation;
                State.BackFace.StencilFunc = (D3D11_COMPARISON_FUNC)In->BackFaceStencilFunction;
                State.BackFace.StencilPassOp = (D3D11_STENCIL_OP)In->BackFaceStencilPassOperation;
                State.FrontFace.StencilDepthFailOp = (D3D11_STENCIL_OP)In->FrontFaceStencilDepthFailOperation;
                State.FrontFace.StencilFailOp = (D3D11_STENCIL_OP)In->FrontFaceStencilFailOperation;
                State.FrontFace.StencilFunc = (D3D11_COMPARISON_FUNC)In->FrontFaceStencilFunction;
                State.FrontFace.StencilPassOp = (D3D11_STENCIL_OP)In->FrontFaceStencilPassOperation;
                State.DepthEnable = In->DepthEnable;
                State.DepthFunc = (D3D11_COMPARISON_FUNC)In->DepthFunction;
                State.DepthWriteMask = (D3D11_DEPTH_WRITE_MASK)In->DepthWriteMask;
                State.StencilEnable = In->StencilEnable;
                State.StencilReadMask = In->StencilReadMask;
                State.StencilWriteMask = In->StencilWriteMask;

                ID3D11DepthStencilState* DeviceState = nullptr;
                if (D3DDevice->CreateDepthStencilState(&State, &DeviceState) != S_OK)
                {
                    ReleaseCom(DeviceState);
                    THAWK_ERROR("couldn't create depth stencil state");
                    return 0;
                }

                In->Pointer = DeviceState;
                In->Index = DepthStencilStates.size();

                DepthStencilStates.push_back(In);
                return DepthStencilStates.size() - 1;
            }
            UInt64 D3D11Device::AddBlendState(Graphics::BlendState* In)
            {
                D3D11_BLEND_DESC State;
                State.AlphaToCoverageEnable = In->AlphaToCoverageEnable;
                State.IndependentBlendEnable = In->IndependentBlendEnable;

                for (int i = 0; i < 8; i++)
                {
                    State.RenderTarget[i].BlendEnable = In->RenderTarget[i].BlendEnable;
                    State.RenderTarget[i].BlendOp = (D3D11_BLEND_OP)In->RenderTarget[i].BlendOperationMode;
                    State.RenderTarget[i].BlendOpAlpha = (D3D11_BLEND_OP)In->RenderTarget[i].BlendOperationAlpha;
                    State.RenderTarget[i].DestBlend = (D3D11_BLEND)In->RenderTarget[i].DestBlend;
                    State.RenderTarget[i].DestBlendAlpha = (D3D11_BLEND)In->RenderTarget[i].DestBlendAlpha;
                    State.RenderTarget[i].RenderTargetWriteMask = In->RenderTarget[i].RenderTargetWriteMask;
                    State.RenderTarget[i].SrcBlend = (D3D11_BLEND)In->RenderTarget[i].SrcBlend;
                    State.RenderTarget[i].SrcBlendAlpha = (D3D11_BLEND)In->RenderTarget[i].SrcBlendAlpha;
                }

                ID3D11BlendState* DeviceState = nullptr;
                if (D3DDevice->CreateBlendState(&State, &DeviceState) != S_OK)
                {
                    ReleaseCom(DeviceState);
                    THAWK_ERROR("couldn't create blend state");
                    return 0;
                }

                In->Pointer = DeviceState;
                In->Index = BlendStates.size();

                BlendStates.push_back(In);
                return BlendStates.size() - 1;
            }
            UInt64 D3D11Device::AddRasterizerState(Graphics::RasterizerState* In)
            {
                D3D11_RASTERIZER_DESC State;
                State.AntialiasedLineEnable = In->AntialiasedLineEnable;
                State.CullMode = (D3D11_CULL_MODE)In->CullMode;
                State.DepthBias = In->DepthBias;
                State.DepthBiasClamp = In->DepthBiasClamp;
                State.DepthClipEnable = In->DepthClipEnable;
                State.FillMode = (D3D11_FILL_MODE)In->FillMode;
                State.FrontCounterClockwise = In->FrontCounterClockwise;
                State.MultisampleEnable = In->MultisampleEnable;
                State.ScissorEnable = In->ScissorEnable;
                State.SlopeScaledDepthBias = In->SlopeScaledDepthBias;

                ID3D11RasterizerState* DeviceState = nullptr;
                if (D3DDevice->CreateRasterizerState(&State, &DeviceState) != S_OK)
                {
                    ReleaseCom(DeviceState);
                    THAWK_ERROR("couldn't create Rasterizer state");
                    return 0;
                }

                In->Pointer = DeviceState;
                In->Index = RasterizerStates.size();

                RasterizerStates.push_back(In);
                return RasterizerStates.size() - 1;
            }
            UInt64 D3D11Device::AddSamplerState(Graphics::SamplerState* In)
            {
                D3D11_SAMPLER_DESC State;
                State.AddressU = (D3D11_TEXTURE_ADDRESS_MODE)In->AddressU;
                State.AddressV = (D3D11_TEXTURE_ADDRESS_MODE)In->AddressV;
                State.AddressW = (D3D11_TEXTURE_ADDRESS_MODE)In->AddressW;
                State.ComparisonFunc = (D3D11_COMPARISON_FUNC)In->ComparisonFunction;
                State.Filter = (D3D11_FILTER)In->Filter;
                State.MaxAnisotropy = In->MaxAnisotropy;
                State.MaxLOD = In->MaxLOD;
                State.MinLOD = In->MinLOD;
                State.MipLODBias = In->MipLODBias;
                State.BorderColor[0] = In->BorderColor[0];
                State.BorderColor[1] = In->BorderColor[1];
                State.BorderColor[2] = In->BorderColor[2];
                State.BorderColor[3] = In->BorderColor[3];

                ID3D11SamplerState* DeviceState = nullptr;
                if (D3DDevice->CreateSamplerState(&State, &DeviceState) != S_OK)
                {
                    ReleaseCom(DeviceState);
                    THAWK_ERROR("couldn't create sampler state");
                    return 0;
                }

                In->Pointer = DeviceState;
                In->Index = SamplerStates.size();

                SamplerStates.push_back(In);
                return SamplerStates.size() - 1;
            }
            void D3D11Device::SetSamplerState(UInt64 State)
            {
                ID3D11SamplerState* DeviceState = (ID3D11SamplerState*)SamplerStates[State]->Pointer;
                ImmediateContext->PSSetSamplers(0, 1, &DeviceState);
            }
            void D3D11Device::SetBlendState(UInt64 State)
            {
                ID3D11BlendState* DeviceState = (ID3D11BlendState*)BlendStates[State]->Pointer;
                ImmediateContext->OMSetBlendState(DeviceState, 0, 0xffffffff);
            }
            void D3D11Device::SetRasterizerState(UInt64 State)
            {
                ID3D11RasterizerState* DeviceState = (ID3D11RasterizerState*)RasterizerStates[State]->Pointer;
                ImmediateContext->RSSetState(DeviceState);
            }
            void D3D11Device::SetDepthStencilState(UInt64 State)
            {
                ID3D11DepthStencilState* DeviceState = (ID3D11DepthStencilState*)DepthStencilStates[State]->Pointer;
                ImmediateContext->OMSetDepthStencilState(DeviceState, 1);
            }
            void D3D11Device::SendBufferStream(Graphics::RenderBufferType Buffer)
            {
                ImmediateContext->UpdateSubresource(ConstantBuffer[Buffer], 0, nullptr, ConstantData[Buffer], 0, 0);
            }
            void D3D11Device::Present()
            {
                SwapChain->Present(VSyncMode, PresentationFlag);
            }
            void D3D11Device::SetPrimitiveTopology(Graphics::PrimitiveTopology Topology)
            {
                ImmediateContext->IASetPrimitiveTopology((D3D11_PRIMITIVE_TOPOLOGY)Topology);
            }
            void D3D11Device::RestoreTexture2D(int Slot, int Size)
            {
                if (Size <= 0 || Size > 64)
                    return;

                ID3D11ShaderResourceView* Array[64] = { nullptr };
                ImmediateContext->PSSetShaderResources(Slot, Size, Array);
            }
            void D3D11Device::RestoreTexture2D(int Size)
            {
                if (Size <= 0 || Size > 64)
                    return;

                ID3D11ShaderResourceView* Array[64] = { nullptr };
                ImmediateContext->PSSetShaderResources(0, Size, Array);
            }
            void D3D11Device::RestoreTexture3D(int Slot, int Size)
            {
                RestoreTexture2D(Slot, Size);
            }
            void D3D11Device::RestoreTexture3D(int Size)
            {
                RestoreTexture2D(Size);
            }
            void D3D11Device::RestoreTextureCube(int Slot, int Size)
            {
                RestoreTexture2D(Slot, Size);
            }
            void D3D11Device::RestoreTextureCube(int Size)
            {
                RestoreTexture2D(Size);
            }
            void D3D11Device::RestoreState()
            {
                ImmediateContext->ClearState();
            }
            void D3D11Device::RestoreShader()
            {
                ImmediateContext->IASetInputLayout(nullptr);
                ImmediateContext->VSSetShader(nullptr, nullptr, 0);
                ImmediateContext->PSSetShader(nullptr, nullptr, 0);
                ImmediateContext->GSSetShader(nullptr, nullptr, 0);
                ImmediateContext->HSSetShader(nullptr, nullptr, 0);
                ImmediateContext->DSSetShader(nullptr, nullptr, 0);
                ImmediateContext->CSSetShader(nullptr, nullptr, 0);
            }
            void D3D11Device::ResizeBuffers(unsigned int Width, unsigned int Height)
            {
                if (RenderTarget != nullptr)
                {
                    ImmediateContext->OMSetRenderTargets(0, 0, 0);
                    delete RenderTarget;

                    DXGI_SWAP_CHAIN_DESC Info;
                    SwapChain->GetDesc(&Info);
                    SwapChain->ResizeBuffers(2, Width, Height, Info.BufferDesc.Format, Info.Flags);
                }

                ID3D11Texture2D* BackBuffer = nullptr;
                if (SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&BackBuffer) != S_OK)
                {
                    THAWK_ERROR("couldn't create back buffer resource");
                    return;
                }

                Graphics::RenderTarget2D::Desc F = Graphics::RenderTarget2D::Desc();
                F.Width = Width;
                F.Height = Height;
                F.MipLevels = 1;
                F.MiscFlags = Graphics::ResourceMisc_None;
                F.FormatMode = Graphics::Format_R8G8B8A8_Unorm;
                F.Usage = Graphics::ResourceUsage_Default;
                F.AccessFlags = Graphics::CPUAccess_Invalid;
                F.BindFlags = Graphics::ResourceBind_Render_Target | Graphics::ResourceBind_Shader_Input;
                F.RenderSurface = (void*)BackBuffer;

                RenderTarget = Graphics::RenderTarget2D::Create(this, F);
                RenderTarget->Apply(this);
                ReleaseCom(BackBuffer);
            }
            void D3D11Device::DrawIndexed(unsigned int Count, unsigned int IndexLocation, unsigned int BaseLocation)
            {
                ImmediateContext->DrawIndexed(Count, IndexLocation, BaseLocation);
            }
            void D3D11Device::Draw(unsigned int Count, unsigned int Start)
            {
                ImmediateContext->Draw(Count, Start);
            }
            Graphics::ShaderModel D3D11Device::GetSupportedShaderModel()
            {
                if (FeatureLevel == D3D_FEATURE_LEVEL::D3D_FEATURE_LEVEL_11_0)
                    return Graphics::ShaderModel_HLSL_5_0;

                if (FeatureLevel == D3D_FEATURE_LEVEL::D3D_FEATURE_LEVEL_10_1)
                    return Graphics::ShaderModel_HLSL_4_1;

                if (FeatureLevel == D3D_FEATURE_LEVEL::D3D_FEATURE_LEVEL_10_0)
                    return Graphics::ShaderModel_HLSL_4_0;

                if (FeatureLevel == D3D_FEATURE_LEVEL::D3D_FEATURE_LEVEL_9_3)
                    return Graphics::ShaderModel_HLSL_3_0;

                if (FeatureLevel == D3D_FEATURE_LEVEL::D3D_FEATURE_LEVEL_9_2)
                    return Graphics::ShaderModel_HLSL_2_0;

                if (FeatureLevel == D3D_FEATURE_LEVEL::D3D_FEATURE_LEVEL_9_1)
                    return Graphics::ShaderModel_HLSL_1_0;

                return Graphics::ShaderModel_Invalid;
            }
            void* D3D11Device::GetDevice()
            {
                return (void*)D3DDevice;
            }
            void* D3D11Device::GetContext()
            {
                return (void*)ImmediateContext;
            }
            void* D3D11Device::GetBackBuffer()
            {
                ID3D11Texture2D* BackBuffer = nullptr;
                SwapChain->GetBuffer(0, __uuidof(BackBuffer), reinterpret_cast<void**>(&BackBuffer));

                D3D11_TEXTURE2D_DESC Texture;
                BackBuffer->GetDesc(&Texture);
                Texture.BindFlags = D3D11_BIND_SHADER_RESOURCE;

                ID3D11Texture2D* Resource = nullptr;
                if (SwapChainResource.SampleDesc.Count > 1)
                {
                    Texture.SampleDesc.Count = 1;
                    Texture.SampleDesc.Quality = 0;
                    D3DDevice->CreateTexture2D(&Texture, nullptr, &Resource);
                    ImmediateContext->ResolveSubresource(Resource, 0, BackBuffer, 0, DXGI_FORMAT_R8G8B8A8_UNORM);
                }
                else
                {
                    D3DDevice->CreateTexture2D(&Texture, nullptr, &Resource);
                    ImmediateContext->CopyResource(Resource, BackBuffer);
                }

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
            void* D3D11Device::GetBackBufferMSAA()
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
            void* D3D11Device::GetBackBufferNoAA()
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
            Graphics::DeviceState* D3D11Device::CreateState()
            {
                D3D11DeviceState* State = new D3D11DeviceState();
                State->RectCount = State->ViewportCount = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
                ImmediateContext->CSGetConstantBuffers(0, 1, &State->CSConstantBuffer);
                ImmediateContext->CSGetSamplers(0, 1, &State->CSSampler);
                ImmediateContext->CSGetShader(&State->CSShader, nullptr, nullptr);
                ImmediateContext->CSGetShaderResources(0, 1, &State->CSShaderResource);
                ImmediateContext->CSGetUnorderedAccessViews(0, 1, &State->CSUnorderedAccessView);
                ImmediateContext->DSGetConstantBuffers(0, 1, &State->DSConstantBuffer);
                ImmediateContext->DSGetSamplers(0, 1, &State->DSSampler);
                ImmediateContext->DSGetShaderResources(0, 1, &State->DSShaderResource);
                ImmediateContext->DSGetShader(&State->DSShader, nullptr, nullptr);
                ImmediateContext->GSGetConstantBuffers(0, 1, &State->GSConstantBuffer);
                ImmediateContext->GSGetSamplers(0, 1, &State->GSSampler);
                ImmediateContext->GSGetShaderResources(0, 1, &State->GSShaderResource);
                ImmediateContext->GSGetShader(&State->GSShader, nullptr, nullptr);
                ImmediateContext->HSGetConstantBuffers(0, 1, &State->HSConstantBuffer);
                ImmediateContext->HSGetSamplers(0, 1, &State->HSSampler);
                ImmediateContext->HSGetShaderResources(0, 1, &State->HSShaderResource);
                ImmediateContext->HSGetShader(&State->HSShader, nullptr, nullptr);
                ImmediateContext->PSGetConstantBuffers(0, 1, &State->PSConstantBuffer);
                ImmediateContext->PSGetSamplers(0, 1, &State->PSSampler);
                ImmediateContext->PSGetShaderResources(0, 1, &State->PSShaderResource);
                ImmediateContext->PSGetShader(&State->PSShader, nullptr, nullptr);
                ImmediateContext->VSGetConstantBuffers(0, 1, &State->VSConstantBuffer);
                ImmediateContext->VSGetSamplers(0, 1, &State->VSSampler);
                ImmediateContext->VSGetShaderResources(0, 1, &State->VSShaderResource);
                ImmediateContext->VSGetShader(&State->VSShader, nullptr, nullptr);
                ImmediateContext->IAGetIndexBuffer(&State->IndexBuffer, &State->IDXGIFormat, &State->IOffset);
                ImmediateContext->IAGetInputLayout(&State->InputLayout);
                ImmediateContext->IAGetPrimitiveTopology(&State->PrimitiveTopology);
                ImmediateContext->IAGetVertexBuffers(0, 1, &State->VertexBuffer, &State->VStride, &State->VOffset);
                ImmediateContext->OMGetBlendState(&State->BlendState, State->BlendFactor, &State->SampleMask);
                ImmediateContext->OMGetDepthStencilState(&State->DepthStencilState, &State->StencilReference);
                ImmediateContext->OMGetRenderTargets(1, &State->RenderTargetView, &State->DepthStencilView);
                ImmediateContext->RSGetScissorRects(&State->RectCount, State->Rects);
                ImmediateContext->RSGetState(&State->RasterizerState);
                ImmediateContext->RSGetViewports(&State->ViewportCount, State->Viewports);

                return State;
            }
			bool D3D11Device::IsValid()
			{
				return BasicEffect != nullptr;
			}
            int D3D11Device::CreateConstantBuffer(int Size, ID3D11Buffer*& Buffer)
            {
                D3D11_BUFFER_DESC Information;
                ZeroMemory(&Information, sizeof(Information));

                Information.Usage = D3D11_USAGE_DEFAULT;
                Information.ByteWidth = Size;
                Information.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
                Information.CPUAccessFlags = 0;

                return D3DDevice->CreateBuffer(&Information, nullptr, &Buffer);
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
            char* D3D11Device::CompileState(ID3DBlob* Error)
            {
                if (!Error)
                    return nullptr;

                return (char*)Error->GetBufferPointer();
            }
            void D3D11Device::LoadShaderSections()
            {
#ifdef HAS_D3D11_ANIMATION_BUFFER_HLSL
                AddSection("animation-buffer.hlsl", GET_RESOURCE_BATCH(d3d11_animation_buffer_hlsl));
#else
                THAWK_ERROR("animation-buffer.hlsl was not compiled");
#endif
#ifdef HAS_D3D11_RENDER_BUFFER_HLSL
                AddSection("render-buffer.hlsl", GET_RESOURCE_BATCH(d3d11_render_buffer_hlsl));
#else
                THAWK_ERROR("render-buffer.hlsl was not compiled");
#endif
#ifdef HAS_D3D11_VIEW_BUFFER_HLSL
                AddSection("view-buffer.hlsl", GET_RESOURCE_BATCH(d3d11_view_buffer_hlsl));
#else
                THAWK_ERROR("view-buffer.hlsl was not compiled");
#endif
#ifdef HAS_D3D11_VERTEX_IN_HLSL
                AddSection("vertex-in.hlsl", GET_RESOURCE_BATCH(d3d11_vertex_in_hlsl));
#else
                THAWK_ERROR("vertex-in.hlsl was not compiled");
#endif
#ifdef HAS_D3D11_VERTEX_OUT_HLSL
                AddSection("vertex-out.hlsl", GET_RESOURCE_BATCH(d3d11_vertex_out_hlsl));
#else
                THAWK_ERROR("vertex-out.hlsl was not compiled");
#endif
#ifdef HAS_D3D11_INFLUENCE_VERTEX_IN_HLSL
                AddSection("influence-vertex-in.hlsl", GET_RESOURCE_BATCH(d3d11_influence_vertex_in_hlsl));
#else
                THAWK_ERROR("influence-vertex-in.hlsl was not compiled");
#endif
#ifdef HAS_D3D11_INFLUENCE_VERTEX_OUT_HLSL
                AddSection("influence-vertex-out.hlsl", GET_RESOURCE_BATCH(d3d11_influence_vertex_out_hlsl));
#else
                THAWK_ERROR("influence-vertex-out.hlsl was not compiled");
#endif
#ifdef HAS_D3D11_SHAPE_VERTEX_IN_HLSL
                AddSection("shape-vertex-in.hlsl", GET_RESOURCE_BATCH(d3d11_shape_vertex_in_hlsl));
#else
                THAWK_ERROR("shape-vertex-in.hlsl was not compiled");
#endif
#ifdef HAS_D3D11_SHAPE_VERTEX_OUT_HLSL
                AddSection("shape-vertex-out.hlsl", GET_RESOURCE_BATCH(d3d11_shape_vertex_out_hlsl));
#else
                THAWK_ERROR("shape-vertex-out.hlsl was not compiled");
#endif
#ifdef HAS_D3D11_DEFERRED_OUT_HLSL
                AddSection("deferred-out.hlsl", GET_RESOURCE_BATCH(d3d11_deferred_out_hlsl));
#else
                THAWK_ERROR("deferred-out.hlsl was not compiled");
#endif
#ifdef HAS_D3D11_INSTANCE_ELEMENT_HLSL
                AddSection("instance-element.hlsl", GET_RESOURCE_BATCH(d3d11_instance_element_hlsl));
#else
                THAWK_ERROR("instance-element.hlsl was not compiled");
#endif
#ifdef HAS_D3D11_GEOMETRY_HLSL
                AddSection("geometry.hlsl", GET_RESOURCE_BATCH(d3d11_geometry_hlsl));
#else
                THAWK_ERROR("geometry.hlsl was not compiled");
#endif
#ifdef HAS_D3D11_LOAD_GEOMETRY_HLSL
                AddSection("load-geometry.hlsl", GET_RESOURCE_BATCH(d3d11_load_geometry_hlsl));
#else
                THAWK_ERROR("load-geometry.hlsl was not compiled");
#endif
#ifdef HAS_D3D11_LOAD_TEXCOORD_HLSL
                AddSection("load-texcoord.hlsl", GET_RESOURCE_BATCH(d3d11_load_texcoord_hlsl));
#else
                THAWK_ERROR("load-texcoord.hlsl was not compiled");
#endif
#ifdef HAS_D3D11_LOAD_POSITION_HLSL
                AddSection("load-position.hlsl", GET_RESOURCE_BATCH(d3d11_load_position_hlsl));
#else
                THAWK_ERROR("load-position.hlsl was not compiled");
#endif
#ifdef HAS_D3D11_COOK_TORRANCE_HLSL
                AddSection("cook-torrance.hlsl", GET_RESOURCE_BATCH(d3d11_cook_torrance_hlsl));
#else
                THAWK_ERROR("cook-torrance.hlsl was not compiled");
#endif
#ifdef HAS_D3D11_HEMI_AMBIENT_HLSL
                AddSection("hemi-ambient.hlsl", GET_RESOURCE_BATCH(d3d11_hemi_ambient_hlsl));
#else
                THAWK_ERROR("hemi-ambient.hlsl was not compiled");
#endif
#ifdef HAS_D3D11_HEMI_AMBIENT_HLSL
                AddSection("random-float-2.hlsl", GET_RESOURCE_BATCH(d3d11_random_float_2_hlsl));
#else
                THAWK_ERROR("random-float-2.hlsl was not compiled");
#endif
            }
        }
    }
}
#endif