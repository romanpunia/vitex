#include "d3d11.h"
#include "../components.h"
#include <imgui.h>
#ifdef THAWK_MICROSOFT
#define ReleaseCom(Value) { if (Value != nullptr) { Value->Release(); Value = nullptr; } }

namespace Tomahawk
{
    namespace Graphics
    {
        namespace D3D11
        {
            D3D11ModelRenderer::D3D11ModelRenderer(Engine::RenderSystem* Lab) : Engine::Renderers::ModelRenderer(Lab)
            {
                Graphics::Shader::Desc I = Graphics::Shader::Desc();
                I.Data = GetMultiCode();
                I.Layout = Graphics::Shader::GetVertexLayout();
                I.LayoutSize = 5;
                Shaders.Multi = (D3D11Shader*)Graphics::Shader::Create(System->GetDevice(), I);

                I.Data = GetDepthCode();
                Shaders.Depth = (D3D11Shader*)Graphics::Shader::Create(System->GetDevice(), I);

                I.Data = GetCubicDepthCode();
                Shaders.CubicDepth = (D3D11Shader*)Graphics::Shader::Create(System->GetDevice(), I);

                Device = System->GetDevice()->As<D3D11Device>();
                Device->CreateConstantBuffer(sizeof(CubicDepth), Shaders.CubicDepth->ConstantBuffer);
                Stride = sizeof(Compute::Vertex);
            }
            D3D11ModelRenderer::~D3D11ModelRenderer()
            {
				delete Shaders.Multi;
				delete Shaders.Depth;
				delete Shaders.CubicDepth;
            }
            void D3D11ModelRenderer::OnInitialize()
            {
                Models = System->GetScene()->GetComponents(Engine::ComponentId_Model);
            }
            void D3D11ModelRenderer::OnRasterization(Rest::Timer* Time)
            {
                D3D11MultiRenderTarget2D* RefSurface = System->GetScene()->GetSurface()->As<D3D11MultiRenderTarget2D>();
                if (!Models || Models->Empty())
                    return;

                Device->SetBlendState(Graphics::RenderLab_Blend_Overwrite);
                Device->SetDepthStencilState(Graphics::RenderLab_DepthStencil_Less);
                Device->ImmediateContext->PSSetShader(Shaders.Multi->PixelShader, nullptr, 0);
                Device->ImmediateContext->VSSetShader(Shaders.Multi->VertexShader, nullptr, 0);
                Device->ImmediateContext->IASetInputLayout(Shaders.Multi->VertexLayout);
                Device->ImmediateContext->OMSetRenderTargets(RefSurface->GetSVTarget(), RefSurface->RenderTargetView, RefSurface->DepthStencilView);
                Device->ImmediateContext->RSSetViewports(1, &RefSurface->Viewport);

                for (auto It = Models->Begin(); It != Models->End(); ++It)
                {
                    Engine::Components::Model* Model = (Engine::Components::Model*)*It;
                    if (!Model->Visibility)
                        continue;

                    for (auto&& Mesh : Model->Instance->Meshes)
                    {
                        auto Face = Model->Surfaces.find(Mesh);
                        if (Face != Model->Surfaces.end())
                        {
                            Device->Render.SurfaceDiffuse = (float)(Face->second.Diffuse != nullptr);
                            Device->Render.SurfaceNormal = (float)(Face->second.Normal != nullptr);
                            Device->Render.Material = (float)Face->second.Material;
                            Device->Render.Diffusion = Face->second.Diffusion;
                            Device->Render.TexCoord = Face->second.TexCoord;

                            if (Face->second.Diffuse != nullptr)
                                Face->second.Diffuse->Apply(Device, 1);
                            else
                                Device->RestoreTexture2D(1, 1);

                            if (Face->second.Normal != nullptr)
                                Face->second.Normal->Apply(Device, 2);
                            else
                                Device->RestoreTexture2D(2, 1);

                            if (Face->second.Surface != nullptr)
                                Face->second.Surface->Apply(Device, 3);
                            else
                                Device->RestoreTexture2D(3, 1);
                        }
                        else
                            Device->RestoreTexture2D(1, 3);

                        Device->Render.World = Mesh->World * Model->GetEntity()->Transform->GetWorld();
                        Device->Render.WorldViewProjection = Device->Render.World * System->GetScene()->View.ViewProjection;
                        Device->ImmediateContext->IASetVertexBuffers(0, 1, &Mesh->GetVertexBuffer()->As<D3D11ElementBuffer>()->Element, &Stride, &Offset);
                        Device->ImmediateContext->IASetIndexBuffer(Mesh->GetIndexBuffer()->As<D3D11ElementBuffer>()->Element, DXGI_FORMAT_R32_UINT, 0);
                        Device->ImmediateContext->UpdateSubresource(Device->ConstantBuffer[Graphics::RenderBufferType_Render], 0, nullptr, &Device->Render, 0, 0);
                        Device->ImmediateContext->DrawIndexed((unsigned int)Mesh->GetIndexBuffer()->GetElements(), 0, 0);
                    }
                }
            }
            void D3D11ModelRenderer::OnPhaseRasterization(Rest::Timer* Time)
            {
                D3D11MultiRenderTarget2D* RefSurface = System->GetScene()->GetSurface()->As<D3D11MultiRenderTarget2D>();
                if (!Models || Models->Empty())
                    return;

                Device->SetBlendState(Graphics::RenderLab_Blend_Overwrite);
                Device->SetDepthStencilState(Graphics::RenderLab_DepthStencil_Less);
                Device->ImmediateContext->PSSetShader(Shaders.Multi->PixelShader, nullptr, 0);
                Device->ImmediateContext->VSSetShader(Shaders.Multi->VertexShader, nullptr, 0);
                Device->ImmediateContext->IASetInputLayout(Shaders.Multi->VertexLayout);
                Device->ImmediateContext->OMSetRenderTargets(RefSurface->GetSVTarget(), RefSurface->RenderTargetView, RefSurface->DepthStencilView);
                Device->ImmediateContext->RSSetViewports(1, &RefSurface->Viewport);

                for (auto It = Models->Begin(); It != Models->End(); ++It)
                {
                    Engine::Components::Model* Model = (Engine::Components::Model*)*It;
                    if (!Model->Instance || Model->GetEntity()->Transform->Position.Distance(System->GetScene()->View.RealPosition) >= System->GetScene()->View.ViewDistance + Model->GetEntity()->Transform->Scale.Length())
                        continue;

                    if (Compute::MathCommon::IsClipping(System->GetScene()->View.ViewProjection, Model->GetEntity()->Transform->GetWorld(), 1.5f) != -1)
                        continue;

                    for (auto&& Mesh : Model->Instance->Meshes)
                    {
                        auto Face = Model->Surfaces.find(Mesh);
                        if (Face != Model->Surfaces.end())
                        {
                            Device->Render.SurfaceDiffuse = (float)(Face->second.Diffuse != nullptr);
                            Device->Render.SurfaceNormal = (float)(Face->second.Normal != nullptr);
                            Device->Render.Material = (float)Face->second.Material;
                            Device->Render.Diffusion = Face->second.Diffusion;
                            Device->Render.TexCoord = Face->second.TexCoord;

                            if (Face->second.Diffuse != nullptr)
                                Face->second.Diffuse->Apply(Device, 1);
                            else
                                Device->RestoreTexture2D(1, 1);

                            if (Face->second.Normal != nullptr)
                                Face->second.Normal->Apply(Device, 2);
                            else
                                Device->RestoreTexture2D(2, 1);

                            if (Face->second.Surface != nullptr)
                                Face->second.Surface->Apply(Device, 3);
                            else
                                Device->RestoreTexture2D(3, 1);
                        }
                        else
                            Device->RestoreTexture2D(1, 3);

                        Device->Render.World = Mesh->World * Model->GetEntity()->Transform->GetWorld();
                        Device->Render.WorldViewProjection = Device->Render.World * System->GetScene()->View.ViewProjection;
                        Device->ImmediateContext->IASetVertexBuffers(0, 1, &Mesh->GetVertexBuffer()->As<D3D11ElementBuffer>()->Element, &Stride, &Offset);
                        Device->ImmediateContext->IASetIndexBuffer(Mesh->GetIndexBuffer()->As<D3D11ElementBuffer>()->Element, DXGI_FORMAT_R32_UINT, 0);
                        Device->ImmediateContext->UpdateSubresource(Device->ConstantBuffer[Graphics::RenderBufferType_Render], 0, nullptr, &Device->Render, 0, 0);
                        Device->ImmediateContext->DrawIndexed((unsigned int)Mesh->GetIndexBuffer()->GetElements(), 0, 0);
                    }
                }
            }
            void D3D11ModelRenderer::OnDepthRasterization(Rest::Timer* Time)
            {
                if (!Models || Models->Empty())
                    return;

                Device->SetDepthStencilState(Graphics::RenderLab_DepthStencil_Less);
                Device->ImmediateContext->PSSetShader(Shaders.Depth->PixelShader, nullptr, 0);
                Device->ImmediateContext->VSSetShader(Shaders.Depth->VertexShader, nullptr, 0);
                Device->ImmediateContext->IASetInputLayout(Shaders.Depth->VertexLayout);

                for (auto It = Models->Begin(); It != Models->End(); ++It)
                {
                    Engine::Components::Model* Model = (Engine::Components::Model*)*It;
                    if (!Model->Instance || Model->GetEntity()->Transform->Position.Distance(System->GetScene()->View.RealPosition) >= System->GetScene()->View.ViewDistance + Model->GetEntity()->Transform->Scale.Length())
                        continue;

                    if (Compute::MathCommon::IsClipping(System->GetScene()->View.ViewProjection, Model->GetEntity()->Transform->GetWorld(), 1.5f) != -1)
                        continue;

                    for (auto&& Mesh : Model->Instance->Meshes)
                    {
                        auto Face = Model->Surfaces.find(Mesh);
                        if (Face != Model->Surfaces.end())
                        {
                            Graphics::Material& Material = Model->GetMaterial(&Face->second);
                            Device->Render.SurfaceDiffuse = (float)(Face->second.Diffuse != nullptr);
                            Device->Render.Diffusion.X = Material.Transmittance;
                            Device->Render.Diffusion.Y = Material.Shadowness;
                            Device->Render.TexCoord = Face->second.TexCoord;

                            if (Face->second.Diffuse != nullptr)
                                Face->second.Diffuse->Apply(Device, 1);
                            else
                                Device->RestoreTexture2D(1, 1);
                        }
                        else
                            Device->RestoreTexture2D(1, 1);

                        Device->Render.World = Mesh->World * Model->GetEntity()->Transform->GetWorld();
                        Device->Render.WorldViewProjection = Device->Render.World * System->GetScene()->View.ViewProjection;
                        Device->ImmediateContext->IASetVertexBuffers(0, 1, &Mesh->GetVertexBuffer()->As<D3D11ElementBuffer>()->Element, &Stride, &Offset);
                        Device->ImmediateContext->IASetIndexBuffer(Mesh->GetIndexBuffer()->As<D3D11ElementBuffer>()->Element, DXGI_FORMAT_R32_UINT, 0);
                        Device->ImmediateContext->UpdateSubresource(Device->ConstantBuffer[Graphics::RenderBufferType_Render], 0, nullptr, &Device->Render, 0, 0);
                        Device->ImmediateContext->DrawIndexed((unsigned int)Mesh->GetIndexBuffer()->GetElements(), 0, 0);
                    }
                }
            }
            void D3D11ModelRenderer::OnCubicDepthRasterization(Rest::Timer* Time, Compute::Matrix4x4* ViewProjection)
            {
                if (!Models || Models->Empty())
                    return;

                CubicDepth.Position = System->GetScene()->View.Position;
                CubicDepth.Distance = System->GetScene()->View.ViewDistance;
                memcpy(CubicDepth.SliceViewProjection, ViewProjection, sizeof(Compute::Matrix4x4) * 6);

                Device->SetDepthStencilState(Graphics::RenderLab_DepthStencil_Less);
                Device->ImmediateContext->PSSetShader(Shaders.CubicDepth->PixelShader, nullptr, 0);
                Device->ImmediateContext->VSSetShader(Shaders.CubicDepth->VertexShader, nullptr, 0);
                Device->ImmediateContext->GSSetShader(Shaders.CubicDepth->GeometryShader, nullptr, 0);
                Device->ImmediateContext->PSSetConstantBuffers(3, 1, &Shaders.CubicDepth->ConstantBuffer);
                Device->ImmediateContext->VSSetConstantBuffers(3, 1, &Shaders.CubicDepth->ConstantBuffer);
                Device->ImmediateContext->GSSetConstantBuffers(3, 1, &Shaders.CubicDepth->ConstantBuffer);
                Device->ImmediateContext->IASetInputLayout(Shaders.CubicDepth->VertexLayout);
                Device->ImmediateContext->UpdateSubresource(Shaders.CubicDepth->ConstantBuffer, 0, nullptr, &CubicDepth, 0, 0);

                for (auto It = Models->Begin(); It != Models->End(); ++It)
                {
                    Engine::Components::Model* Model = (Engine::Components::Model*)*It;
                    if (!Model->Instance || Model->GetEntity()->Transform->Position.Distance(System->GetScene()->View.RealPosition) >= System->GetScene()->View.ViewDistance + Model->GetEntity()->Transform->Scale.Length())
                        continue;

                    for (auto&& Mesh : Model->Instance->Meshes)
                    {
                        auto Face = Model->Surfaces.find(Mesh);
                        if (Face != Model->Surfaces.end())
                        {
                            Graphics::Material& Material = Model->GetMaterial(&Face->second);
                            Device->Render.SurfaceDiffuse = (float)(Face->second.Diffuse != nullptr);
                            Device->Render.Diffusion.X = Material.Transmittance;
                            Device->Render.Diffusion.Y = Material.Shadowness;
                            Device->Render.TexCoord = Face->second.TexCoord;

                            if (Face->second.Diffuse != nullptr)
                                Face->second.Diffuse->Apply(Device, 1);
                            else
                                Device->RestoreTexture2D(1, 1);
                        }
                        else
                            Device->RestoreTexture2D(1, 1);

                        Device->Render.World = Mesh->World * Model->GetEntity()->Transform->GetWorld();
                        Device->ImmediateContext->IASetVertexBuffers(0, 1, &Mesh->GetVertexBuffer()->As<D3D11ElementBuffer>()->Element, &Stride, &Offset);
                        Device->ImmediateContext->IASetIndexBuffer(Mesh->GetIndexBuffer()->As<D3D11ElementBuffer>()->Element, DXGI_FORMAT_R32_UINT, 0);
                        Device->ImmediateContext->UpdateSubresource(Device->ConstantBuffer[Graphics::RenderBufferType_Render], 0, nullptr, &Device->Render, 0, 0);
                        Device->ImmediateContext->DrawIndexed((unsigned int)Mesh->GetIndexBuffer()->GetElements(), 0, 0);
                    }
                }

                Device->ImmediateContext->GSSetShader(nullptr, nullptr, 0);
            }
            const char* D3D11ModelRenderer::GetMultiCode()
            {
                return
                        "#include VertexIn.h"
                        "#include VertexOut.h"
                        "#include DeferredOutput.h"
                        "#include RenderBuffer.h"

                        "Texture2D Input1 : register(t1);"
                        "Texture2D Input2 : register(t2);"
                        "Texture2D Input3 : register(t3);"
                        "SamplerState State : register(s0);"

                        "VertexOut VS(VertexIn Input)"
                        "{"
                        "    VertexOut Output = (VertexOut)0;"
                        "    Output.Position = Output.RawPosition = mul(Input.Position, WorldViewProjection);"
                        "    Output.Normal = normalize(mul(Input.Normal, (float3x3)World));"
                        "    Output.Tangent = normalize(mul(Input.Tangent, (float3x3)World));"
                        "    Output.Bitangent = normalize(mul(Input.Bitangent, (float3x3)World));"
                        "    Output.TexCoord = Input.TexCoord * TexCoord;"

                        "    return Output;"
                        "}"

                        "DeferredOutput PS(VertexOut Input)"
                        "{"
                        "    float3 Diffuse = Diffusion;"
                        "    [branch] if (SurfaceDiffuse > 0)"
                        "        Diffuse *= Input1.Sample(State, Input.TexCoord).xyz;"

                        "    float3 Normal = Input.Normal;"
                        "    [branch] if (SurfaceNormal > 0)"
                        "    {"
                        "        Normal = Input2.Sample(State, Input.TexCoord).xyz * 2.0f - 1.0f;"
                        "        Normal = normalize(Normal.x * Input.Tangent + Normal.y * Input.Bitangent + Normal.z * Input.Normal);"
                        "    }"

                        "    float2 Surface = Input3.SampleLevel(State, Input.TexCoord, 0).xy;"

                        "    DeferredOutput Output = (DeferredOutput)0;"
                        "    Output.Output1 = float4(Diffuse, Surface.x);"
                        "    Output.Output2 = float4(Normal, Material);"
                        "    Output.Output3 = float2(Input.RawPosition.z / Input.RawPosition.w, Surface.y);"

                        "    return Output;"
                        "};";
            }
            const char* D3D11ModelRenderer::GetDepthCode()
            {
                return
                        "#include VertexIn.h"
                        "#include RenderBuffer.h"

                        "struct VertexOut"
                        "{"
                        "    float4 Position : SV_POSITION;"
                        "    float2 TexCoord : TEXCOORD0;"
                        "    float4 RawPosition : TEXCOORD1;"
                        "};"

                        "Texture2D Input1 : register(t1);"
                        "SamplerState State : register(s0);"

                        "VertexOut VS(VertexIn Input)"
                        "{"
                        "    VertexOut Output = (VertexOut)0;"
                        "    Output.Position = Output.RawPosition = mul(Input.Position, WorldViewProjection);"
                        "    Output.TexCoord = Input.TexCoord * TexCoord;"
                        "    return Output;"
                        "}"

                        "float4 PS(VertexOut Input) : SV_TARGET0"
                        "{"
                        "    float Alpha = (1.0f - Diffusion.y);"
                        "    [branch] if (SurfaceDiffuse > 0)"
                        "        Alpha *= Input1.Sample(State, Input.TexCoord * TexCoord).w;"

                        "	 [branch] if (Alpha < Diffusion.x)"
                        "		 discard;"

                        "    return float4(Input.RawPosition.z / Input.RawPosition.w, Alpha, 1.0f, 1.0f);"
                        "};";
            }
            const char* D3D11ModelRenderer::GetCubicDepthCode()
            {
                return
                        "#include VertexIn.h"
                        "#include RenderBuffer.h"

                        "cbuffer ViewBuffer : register(b3)"
                        "{"
                        "	 matrix SliceViewProjection[6];"
                        "    float3 Position;"
                        "    float Distance;"
                        "};"

                        "struct VertexCubeOut"
                        "{"
                        "	 float4 Position : SV_POSITION;"
                        "	 float2 TexCoord : TEXCOORD0;"
                        "	 float4 RawPosition : TEXCOORD1;"
                        "	 uint RenderTarget : SV_RenderTargetArrayIndex;"
                        "};"

                        "struct VertexOut"
                        "{"
                        "    float4 Position : SV_POSITION;"
                        "    float2 TexCoord : TEXCOORD0;"
                        "    float4 RawPosition : TEXCOORD1;"
                        "};"

                        "Texture2D Input1 : register(t1);"
                        "SamplerState State : register(s0);"

                        "[maxvertexcount(18)]"
                        "void GS(triangle VertexOut Input[3], inout TriangleStream<VertexCubeOut> Stream)"
                        "{"
                        "	 VertexCubeOut Output = (VertexCubeOut)0;"
                        "	 for (Output.RenderTarget = 0; Output.RenderTarget < 6; Output.RenderTarget++)"
                        "	 {"
                        "		 Output.Position = mul(Input[0].Position, SliceViewProjection[Output.RenderTarget]);"
                        "        Output.RawPosition = Input[0].RawPosition;"
                        "		 Output.TexCoord = Input[0].TexCoord;"
                        "		 Stream.Append(Output);"

                        "		 Output.Position = mul(Input[1].Position, SliceViewProjection[Output.RenderTarget]);"
                        "        Output.RawPosition = Input[1].RawPosition;"
                        "		 Output.TexCoord = Input[1].TexCoord;"
                        "		 Stream.Append(Output);"

                        "		 Output.Position = mul(Input[2].Position, SliceViewProjection[Output.RenderTarget]);"
                        "        Output.RawPosition = Input[2].RawPosition;"
                        "		 Output.TexCoord = Input[2].TexCoord;"
                        "		 Stream.Append(Output);"

                        "		 Stream.RestartStrip();"
                        "	 }"
                        "}"

                        "VertexOut VS(VertexIn Input)"
                        "{"
                        "	 VertexOut Output = (VertexOut)0;"
                        "	 Output.Position = Output.RawPosition = mul(Input.Position, World);"
                        "	 Output.TexCoord = Input.TexCoord * TexCoord;"

                        "	 return Output;"
                        "}"

                        "float4 PS(VertexOut Input) : SV_TARGET0"
                        "{"
                        "    float Alpha = (1.0f - Diffusion.y);"
                        "    [branch] if (SurfaceDiffuse > 0)"
                        "        Alpha *= Input1.Sample(State, Input.TexCoord * TexCoord).w;"

                        "	 [branch] if (Alpha < Diffusion.x)"
                        "		 discard;"

                        "    return float4(length(Input.RawPosition.xyz - Position) / Distance, Alpha, 1.0f, 1.0f);"
                        "};";
            }

            D3D11SkinnedModelRenderer::D3D11SkinnedModelRenderer(Engine::RenderSystem* Lab) : Engine::Renderers::SkinnedModelRenderer(Lab)
            {
                Device = System->GetDevice()->As<D3D11Device>();
                Stride = sizeof(Compute::InfluenceVertex);

                Graphics::Shader::Desc I = Graphics::Shader::Desc();
                I.Data = GetMultiCode();
                I.Layout = Graphics::Shader::GetInfluenceVertexLayout();
                I.LayoutSize = 7;

                Shaders.Multi = (D3D11Shader*)Graphics::Shader::Create(System->GetDevice(), I);

                I.Data = GetDepthCode();
                Shaders.Depth = (D3D11Shader*)Graphics::Shader::Create(System->GetDevice(), I);

                I.Data = GetCubicDepthCode();
                Shaders.CubicDepth = (D3D11Shader*)Graphics::Shader::Create(System->GetDevice(), I);

                Device->CreateConstantBuffer(sizeof(CubicDepth), Shaders.CubicDepth->ConstantBuffer);
            }
            D3D11SkinnedModelRenderer::~D3D11SkinnedModelRenderer()
            {
				delete Shaders.Multi;
				delete Shaders.Depth;
				delete Shaders.CubicDepth;
            }
            void D3D11SkinnedModelRenderer::OnInitialize()
            {
                SkinnedModels = System->GetScene()->GetComponents(Engine::ComponentId_Skinned_Model);
            }
            void D3D11SkinnedModelRenderer::OnRasterization(Rest::Timer* Time)
            {
                D3D11MultiRenderTarget2D* RefSurface = System->GetScene()->GetSurface()->As<D3D11MultiRenderTarget2D>();
                if (!SkinnedModels || SkinnedModels->Empty())
                    return;

                Device->SetBlendState(Graphics::RenderLab_Blend_Overwrite);
                Device->SetDepthStencilState(Graphics::RenderLab_DepthStencil_Less);
                Device->ImmediateContext->PSSetShader(Shaders.Multi->PixelShader, nullptr, 0);
                Device->ImmediateContext->VSSetShader(Shaders.Multi->VertexShader, nullptr, 0);
                Device->ImmediateContext->IASetInputLayout(Shaders.Multi->VertexLayout);
                Device->ImmediateContext->OMSetRenderTargets(RefSurface->GetSVTarget(), RefSurface->RenderTargetView, RefSurface->DepthStencilView);
                Device->ImmediateContext->RSSetViewports(1, &RefSurface->Viewport);

                for (auto It = SkinnedModels->Begin(); It != SkinnedModels->End(); ++It)
                {
                    Engine::Components::SkinnedModel* SkinnedModel = (Engine::Components::SkinnedModel*)*It;
                    if (!SkinnedModel->Visibility)
                        continue;

                    Device->Animation.Base.X = !SkinnedModel->Instance->Joints.empty();
                    if (Device->Animation.Base.X > 0)
                        memcpy(Device->Animation.Transformation, SkinnedModel->Skeleton.Transform, 96 * sizeof(Compute::Matrix4x4));

                    Device->ImmediateContext->UpdateSubresource(Device->ConstantBuffer[Graphics::RenderBufferType_Animation], 0, nullptr, &Device->Animation, 0, 0);
                    for (auto&& Mesh : SkinnedModel->Instance->Meshes)
                    {
                        auto Face = SkinnedModel->Surfaces.find(Mesh);
                        if (Face != SkinnedModel->Surfaces.end())
                        {
                            Device->Render.SurfaceDiffuse = (float)(Face->second.Diffuse != nullptr);
                            Device->Render.SurfaceNormal = (float)(Face->second.Normal != nullptr);
                            Device->Render.Material = (float)Face->second.Material;
                            Device->Render.Diffusion = Face->second.Diffusion;
                            Device->Render.TexCoord = Face->second.TexCoord;

                            if (Face->second.Diffuse != nullptr)
                                Face->second.Diffuse->Apply(Device, 1);
                            else
                                Device->RestoreTexture2D(1, 1);

                            if (Face->second.Normal != nullptr)
                                Face->second.Normal->Apply(Device, 2);
                            else
                                Device->RestoreTexture2D(2, 1);

                            if (Face->second.Surface != nullptr)
                                Face->second.Surface->Apply(Device, 3);
                            else
                                Device->RestoreTexture2D(3, 1);
                        }
                        else
                            Device->RestoreTexture2D(1, 3);

                        Device->Render.World = Mesh->World * SkinnedModel->GetEntity()->Transform->GetWorld();
                        Device->Render.WorldViewProjection = Device->Render.World * System->GetScene()->View.ViewProjection;
                        Device->ImmediateContext->IASetVertexBuffers(0, 1, &Mesh->GetVertexBuffer()->As<D3D11ElementBuffer>()->Element, &Stride, &Offset);
                        Device->ImmediateContext->IASetIndexBuffer(Mesh->GetIndexBuffer()->As<D3D11ElementBuffer>()->Element, DXGI_FORMAT_R32_UINT, 0);
                        Device->ImmediateContext->UpdateSubresource(Device->ConstantBuffer[Graphics::RenderBufferType_Render], 0, nullptr, &Device->Render, 0, 0);
                        Device->ImmediateContext->DrawIndexed((unsigned int)Mesh->GetIndexBuffer()->GetElements(), 0, 0);
                    }
                }
            }
            void D3D11SkinnedModelRenderer::OnPhaseRasterization(Rest::Timer* Time)
            {
                D3D11MultiRenderTarget2D* RefSurface = System->GetScene()->GetSurface()->As<D3D11MultiRenderTarget2D>();
                if (!SkinnedModels || SkinnedModels->Empty())
                    return;

                Device->SetBlendState(Graphics::RenderLab_Blend_Overwrite);
                Device->SetDepthStencilState(Graphics::RenderLab_DepthStencil_Less);
                Device->ImmediateContext->PSSetShader(Shaders.Multi->PixelShader, nullptr, 0);
                Device->ImmediateContext->VSSetShader(Shaders.Multi->VertexShader, nullptr, 0);
                Device->ImmediateContext->IASetInputLayout(Shaders.Multi->VertexLayout);
                Device->ImmediateContext->OMSetRenderTargets(RefSurface->GetSVTarget(), RefSurface->RenderTargetView, RefSurface->DepthStencilView);
                Device->ImmediateContext->RSSetViewports(1, &RefSurface->Viewport);

                for (auto It = SkinnedModels->Begin(); It != SkinnedModels->End(); ++It)
                {
                    Engine::Components::SkinnedModel* SkinnedModel = (Engine::Components::SkinnedModel*)*It;
                    if (!SkinnedModel->Instance || SkinnedModel->GetEntity()->Transform->Position.Distance(System->GetScene()->View.RealPosition) >= System->GetScene()->View.ViewDistance + SkinnedModel->GetEntity()->Transform->Scale.Length())
                        continue;

                    if (Compute::MathCommon::IsClipping(System->GetScene()->View.ViewProjection, SkinnedModel->GetEntity()->Transform->GetWorld(), 1.5f) != -1)
                        continue;

                    Device->Animation.Base.X = !SkinnedModel->Instance->Joints.empty();
                    if (Device->Animation.Base.X > 0)
                        memcpy(Device->Animation.Transformation, SkinnedModel->Skeleton.Transform, 96 * sizeof(Compute::Matrix4x4));

                    Device->ImmediateContext->UpdateSubresource(Device->ConstantBuffer[Graphics::RenderBufferType_Animation], 0, nullptr, &Device->Animation, 0, 0);
                    for (auto&& Mesh : SkinnedModel->Instance->Meshes)
                    {
                        auto Face = SkinnedModel->Surfaces.find(Mesh);
                        if (Face != SkinnedModel->Surfaces.end())
                        {
                            Device->Render.SurfaceDiffuse = (float)(Face->second.Diffuse != nullptr);
                            Device->Render.SurfaceNormal = (float)(Face->second.Normal != nullptr);
                            Device->Render.Material = (float)Face->second.Material;
                            Device->Render.Diffusion = Face->second.Diffusion;
                            Device->Render.TexCoord = Face->second.TexCoord;

                            if (Face->second.Diffuse != nullptr)
                                Face->second.Diffuse->Apply(Device, 1);
                            else
                                Device->RestoreTexture2D(1, 1);

                            if (Face->second.Normal != nullptr)
                                Face->second.Normal->Apply(Device, 2);
                            else
                                Device->RestoreTexture2D(2, 1);

                            if (Face->second.Surface != nullptr)
                                Face->second.Surface->Apply(Device, 3);
                            else
                                Device->RestoreTexture2D(3, 1);
                        }
                        else
                            Device->RestoreTexture2D(1, 3);

                        Device->Render.World = Mesh->World * SkinnedModel->GetEntity()->Transform->GetWorld();
                        Device->Render.WorldViewProjection = Device->Render.World * System->GetScene()->View.ViewProjection;
                        Device->ImmediateContext->IASetVertexBuffers(0, 1, &Mesh->GetVertexBuffer()->As<D3D11ElementBuffer>()->Element, &Stride, &Offset);
                        Device->ImmediateContext->IASetIndexBuffer(Mesh->GetIndexBuffer()->As<D3D11ElementBuffer>()->Element, DXGI_FORMAT_R32_UINT, 0);
                        Device->ImmediateContext->UpdateSubresource(Device->ConstantBuffer[Graphics::RenderBufferType_Render], 0, nullptr, &Device->Render, 0, 0);
                        Device->ImmediateContext->DrawIndexed((unsigned int)Mesh->GetIndexBuffer()->GetElements(), 0, 0);
                    }
                }
            }
            void D3D11SkinnedModelRenderer::OnDepthRasterization(Rest::Timer* Time)
            {
                if (!SkinnedModels || SkinnedModels->Empty())
                    return;

                Device->SetDepthStencilState(Graphics::RenderLab_DepthStencil_Less);
                Device->ImmediateContext->PSSetShader(Shaders.Depth->PixelShader, nullptr, 0);
                Device->ImmediateContext->VSSetShader(Shaders.Depth->VertexShader, nullptr, 0);
                Device->ImmediateContext->IASetInputLayout(Shaders.Depth->VertexLayout);

                for (auto It = SkinnedModels->Begin(); It != SkinnedModels->End(); ++It)
                {
                    Engine::Components::SkinnedModel* SkinnedModel = (Engine::Components::SkinnedModel*)*It;
                    if (!SkinnedModel->Instance || SkinnedModel->GetEntity()->Transform->Position.Distance(System->GetScene()->View.RealPosition) >= System->GetScene()->View.ViewDistance + SkinnedModel->GetEntity()->Transform->Scale.Length())
                        continue;

                    if (Compute::MathCommon::IsClipping(System->GetScene()->View.ViewProjection, SkinnedModel->GetEntity()->Transform->GetWorld(), 1.5f) != -1)
                        continue;

                    Device->Animation.Base.X = !SkinnedModel->Instance->Joints.empty();
                    if (Device->Animation.Base.X > 0)
                        memcpy(Device->Animation.Transformation, SkinnedModel->Skeleton.Transform, 96 * sizeof(Compute::Matrix4x4));

                    Device->ImmediateContext->UpdateSubresource(Device->ConstantBuffer[Graphics::RenderBufferType_Animation], 0, nullptr, &Device->Animation, 0, 0);
                    for (auto&& Mesh : SkinnedModel->Instance->Meshes)
                    {
                        auto Face = SkinnedModel->Surfaces.find(Mesh);
                        if (Face != SkinnedModel->Surfaces.end())
                        {
                            Graphics::Material& Material = SkinnedModel->GetMaterial(&Face->second);
                            Device->Render.TexCoord = Face->second.TexCoord;
                            Device->Render.Diffusion.X = Material.Transmittance;
                            Device->Render.Diffusion.Y = Material.Shadowness;
                            Device->Render.SurfaceDiffuse = (float)(Face->second.Diffuse != nullptr);

                            if (Face->second.Diffuse != nullptr)
                                Face->second.Diffuse->Apply(Device, 1);
                            else
                                Device->RestoreTexture2D(1, 1);
                        }
                        else
                            Device->RestoreTexture2D(1, 1);

                        Device->Render.World = Mesh->World * SkinnedModel->GetEntity()->Transform->GetWorld();
                        Device->Render.WorldViewProjection = Device->Render.World * System->GetScene()->View.ViewProjection;
                        Device->ImmediateContext->IASetVertexBuffers(0, 1, &Mesh->GetVertexBuffer()->As<D3D11ElementBuffer>()->Element, &Stride, &Offset);
                        Device->ImmediateContext->IASetIndexBuffer(Mesh->GetIndexBuffer()->As<D3D11ElementBuffer>()->Element, DXGI_FORMAT_R32_UINT, 0);
                        Device->ImmediateContext->UpdateSubresource(Device->ConstantBuffer[Graphics::RenderBufferType_Render], 0, nullptr, &Device->Render, 0, 0);
                        Device->ImmediateContext->DrawIndexed((unsigned int)Mesh->GetIndexBuffer()->GetElements(), 0, 0);
                    }
                }
            }
            void D3D11SkinnedModelRenderer::OnCubicDepthRasterization(Rest::Timer* Time, Compute::Matrix4x4* ViewProjection)
            {
                if (!SkinnedModels || SkinnedModels->Empty())
                    return;

                CubicDepth.Position = System->GetScene()->View.Position;
                CubicDepth.Distance = System->GetScene()->View.ViewDistance;
                memcpy(CubicDepth.SliceViewProjection, ViewProjection, sizeof(Compute::Matrix4x4) * 6);

                Device->SetDepthStencilState(Graphics::RenderLab_DepthStencil_Less);
                Device->ImmediateContext->PSSetShader(Shaders.CubicDepth->PixelShader, nullptr, 0);
                Device->ImmediateContext->VSSetShader(Shaders.CubicDepth->VertexShader, nullptr, 0);
                Device->ImmediateContext->GSSetShader(Shaders.CubicDepth->GeometryShader, nullptr, 0);
                Device->ImmediateContext->PSSetConstantBuffers(3, 1, &Shaders.CubicDepth->ConstantBuffer);
                Device->ImmediateContext->VSSetConstantBuffers(3, 1, &Shaders.CubicDepth->ConstantBuffer);
                Device->ImmediateContext->GSSetConstantBuffers(3, 1, &Shaders.CubicDepth->ConstantBuffer);
                Device->ImmediateContext->IASetInputLayout(Shaders.CubicDepth->VertexLayout);
                Device->ImmediateContext->UpdateSubresource(Shaders.CubicDepth->ConstantBuffer, 0, nullptr, &CubicDepth, 0, 0);

                for (auto It = SkinnedModels->Begin(); It != SkinnedModels->End(); ++It)
                {
                    Engine::Components::SkinnedModel* SkinnedModel = (Engine::Components::SkinnedModel*)*It;
                    if (!SkinnedModel->Instance || SkinnedModel->GetEntity()->Transform->Position.Distance(System->GetScene()->View.RealPosition) >= System->GetScene()->View.ViewDistance + SkinnedModel->GetEntity()->Transform->Scale.Length())
                        continue;

                    Device->Animation.Base.X = !SkinnedModel->Instance->Joints.empty();
                    if (Device->Animation.Base.X > 0)
                        memcpy(Device->Animation.Transformation, SkinnedModel->Skeleton.Transform, 96 * sizeof(Compute::Matrix4x4));

                    Device->ImmediateContext->UpdateSubresource(Device->ConstantBuffer[Graphics::RenderBufferType_Animation], 0, nullptr, &Device->Animation, 0, 0);
                    for (auto&& Mesh : SkinnedModel->Instance->Meshes)
                    {
                        auto Face = SkinnedModel->Surfaces.find(Mesh);
                        if (Face != SkinnedModel->Surfaces.end())
                        {
                            Graphics::Material& Material = SkinnedModel->GetMaterial(&Face->second);
                            Device->Render.SurfaceDiffuse = (float)(Face->second.Diffuse != nullptr);
                            Device->Render.TexCoord = Face->second.TexCoord;
                            Device->Render.Diffusion.X = Material.Transmittance;
                            Device->Render.Diffusion.Y = Material.Shadowness;

                            if (Face->second.Diffuse != nullptr)
                                Face->second.Diffuse->Apply(Device, 1);
                            else
                                Device->RestoreTexture2D(1, 1);
                        }
                        else
                            Device->RestoreTexture2D(1, 1);

                        Device->Render.World = Mesh->World * SkinnedModel->GetEntity()->Transform->GetWorld();
                        Device->ImmediateContext->IASetVertexBuffers(0, 1, &Mesh->GetVertexBuffer()->As<D3D11ElementBuffer>()->Element, &Stride, &Offset);
                        Device->ImmediateContext->IASetIndexBuffer(Mesh->GetIndexBuffer()->As<D3D11ElementBuffer>()->Element, DXGI_FORMAT_R32_UINT, 0);
                        Device->ImmediateContext->UpdateSubresource(Device->ConstantBuffer[Graphics::RenderBufferType_Render], 0, nullptr, &Device->Render, 0, 0);
                        Device->ImmediateContext->DrawIndexed((unsigned int)Mesh->GetIndexBuffer()->GetElements(), 0, 0);
                    }
                }

                Device->ImmediateContext->GSSetShader(nullptr, nullptr, 0);
            }
            const char* D3D11SkinnedModelRenderer::GetMultiCode()
            {
                return
                        "#include InfluenceVertexIn.h"
                        "#include InfluenceVertexOut.h"
                        "#include DeferredOutput.h"
                        "#include AnimationBuffer.h"
                        "#include RenderBuffer.h"

                        "Texture2D Input1 : register(t1);"
                        "Texture2D Input2 : register(t2);"
                        "Texture2D Input3 : register(t3);"
                        "SamplerState State : register(s0);"

                        "InfluenceVertexOut VS(InfluenceVertexIn Input)"
                        "{"
                        "    InfluenceVertexOut Output = (InfluenceVertexOut)0;"
                        "    Output.TexCoord = Input.TexCoord * TexCoord;"

                        "    [branch] if (Base.x > 0)"
                        "    {"
                        "	     matrix Offset ="
                        "            mul(Transformation[(int)Input.Index.x], Input.Bias.x) +"
                        "            mul(Transformation[(int)Input.Index.y], Input.Bias.y) +"
                        "	         mul(Transformation[(int)Input.Index.z], Input.Bias.z) +"
                        "	         mul(Transformation[(int)Input.Index.w], Input.Bias.w);"

                        "        Output.Position = Output.RawPosition = mul(mul(Input.Position, Offset), WorldViewProjection);"
                        "        Output.Normal = normalize(mul(mul(float4(Input.Normal, 0), Offset).xyz, (float3x3)World));"
                        "        Output.Tangent = normalize(mul(mul(float4(Input.Tangent, 0), Offset).xyz, (float3x3)World));"
                        "        Output.Bitangent = normalize(mul(mul(float4(Input.Bitangent, 0), Offset).xyz, (float3x3)World));"
                        "    }"
                        "    else"
                        "    {"

                        "        Output.Position = Output.RawPosition = mul(Input.Position, WorldViewProjection);"
                        "        Output.Normal = normalize(mul(Input.Normal, (float3x3)World));"
                        "        Output.Tangent = normalize(mul(Input.Tangent, (float3x3)World));"
                        "        Output.Bitangent = normalize(mul(Input.Bitangent, (float3x3)World));"
                        "    }"

                        "    return Output;"
                        "}"

                        "DeferredOutput PS(InfluenceVertexOut Input)"
                        "{"
                        "    float3 Diffuse = Diffusion;"
                        "    [branch] if (SurfaceDiffuse > 0)"
                        "        Diffuse *= Input1.Sample(State, Input.TexCoord).xyz;"

                        "    float3 Normal = Input.Normal;"
                        "    [branch] if (SurfaceNormal > 0)"
                        "    {"
                        "        Normal = Input2.Sample(State, Input.TexCoord).xyz * 2.0f - 1.0f;"
                        "        Normal = normalize(Normal.x * Input.Tangent + Normal.y * Input.Bitangent + Normal.z * Input.Normal);"
                        "    }"

                        "    float2 Surface = Input3.SampleLevel(State, Input.TexCoord, 0).xy;"

                        "    DeferredOutput Output = (DeferredOutput)0;"
                        "    Output.Output1 = float4(Diffuse, Surface.x);"
                        "    Output.Output2 = float4(Normal, Material);"
                        "    Output.Output3 = float2(Input.RawPosition.z / Input.RawPosition.w, Surface.y);"

                        "    return Output;"
                        "};";
            }
            const char* D3D11SkinnedModelRenderer::GetDepthCode()
            {
                return
                        "#include InfluenceVertexIn.h"
                        "#include InfluenceVertexOut.h"
                        "#include AnimationBuffer.h"
                        "#include RenderBuffer.h"

                        "Texture2D Input1 : register(t1);"
                        "SamplerState State : register(s0);"

                        "InfluenceVertexOut VS(InfluenceVertexIn Input)"
                        "{"
                        "    InfluenceVertexOut Output = (InfluenceVertexOut)0;"
                        "    Output.TexCoord = Input.TexCoord * TexCoord;"

                        "    [branch] if (Base.x > 0)"
                        "    {"
                        "	     matrix Offset ="
                        "            mul(Transformation[(int)Input.Index.x], Input.Bias.x) +"
                        "            mul(Transformation[(int)Input.Index.y], Input.Bias.y) +"
                        "	         mul(Transformation[(int)Input.Index.z], Input.Bias.z) +"
                        "	         mul(Transformation[(int)Input.Index.w], Input.Bias.w);"

                        "        Output.Position = Output.RawPosition = mul(mul(Input.Position, Offset), WorldViewProjection);"
                        "    }"
                        "    else"
                        "        Output.Position = Output.RawPosition = mul(Input.Position, WorldViewProjection);"

                        "    return Output;"
                        "}"

                        "float4 PS(InfluenceVertexOut Input) : SV_TARGET0"
                        "{"
                        "    float Alpha = (1.0f - Diffusion.y);"
                        "    [branch] if (SurfaceDiffuse > 0)"
                        "        Alpha *= Input1.Sample(State, Input.TexCoord * TexCoord).w;"

                        "	 [branch] if (Alpha < Diffusion.x)"
                        "		 discard;"

                        "    return float4(Input.RawPosition.z / Input.RawPosition.w, Alpha, 1.0f, 1.0f);"
                        "};";
            }
            const char* D3D11SkinnedModelRenderer::GetCubicDepthCode()
            {
                return
                        "#include InfluenceVertexIn.h"
                        "#include AnimationBuffer.h"
                        "#include RenderBuffer.h"

                        "cbuffer ViewBuffer : register(b3)"
                        "{"
                        "	 matrix SliceViewProjection[6];"
                        "    float3 Position;"
                        "    float Distance;"
                        "};"

                        "struct VertexCubeOut"
                        "{"
                        "	 float4 Position : SV_POSITION;"
                        "	 float2 TexCoord : TEXCOORD0;"
                        "	 float4 RawPosition : TEXCOORD1;"
                        "	 uint RenderTarget : SV_RenderTargetArrayIndex;"
                        "};"

                        "struct VertexOut"
                        "{"
                        "    float4 Position : SV_POSITION;"
                        "    float2 TexCoord : TEXCOORD0;"
                        "    float4 RawPosition : TEXCOORD1;"
                        "};"

                        "Texture2D Input1 : register(t1);"
                        "SamplerState State : register(s0);"

                        "[maxvertexcount(18)]"
                        "void GS(triangle VertexOut Input[3], inout TriangleStream<VertexCubeOut> Stream)"
                        "{"
                        "	 VertexCubeOut Output = (VertexCubeOut)0;"
                        "	 for (Output.RenderTarget = 0; Output.RenderTarget < 6; Output.RenderTarget++)"
                        "	 {"
                        "		 Output.Position = Output.RawPosition = mul(Input[0].Position, SliceViewProjection[Output.RenderTarget]);"
                        "        Output.RawPosition = Input[0].RawPosition;"
                        "		 Output.TexCoord = Input[0].TexCoord;"
                        "		 Stream.Append(Output);"

                        "		 Output.Position = Output.RawPosition = mul(Input[1].Position, SliceViewProjection[Output.RenderTarget]);"
                        "        Output.RawPosition = Input[1].RawPosition;"
                        "		 Output.TexCoord = Input[1].TexCoord;"
                        "		 Stream.Append(Output);"

                        "		 Output.Position = mul(Input[2].Position, SliceViewProjection[Output.RenderTarget]);"
                        "        Output.RawPosition = Input[2].RawPosition;"
                        "		 Output.TexCoord = Input[2].TexCoord;"
                        "		 Stream.Append(Output);"

                        "		 Stream.RestartStrip();"
                        "	 }"
                        "}"

                        "VertexOut VS(InfluenceVertexIn Input)"
                        "{"
                        "	 VertexOut Output = (VertexOut)0;"
                        "	 Output.TexCoord = Input.TexCoord * TexCoord;"

                        "    [branch] if (Base.x > 0)"
                        "    {"
                        "	     matrix Offset ="
                        "            mul(Transformation[(int)Input.Index.x], Input.Bias.x) +"
                        "            mul(Transformation[(int)Input.Index.y], Input.Bias.y) +"
                        "	         mul(Transformation[(int)Input.Index.z], Input.Bias.z) +"
                        "	         mul(Transformation[(int)Input.Index.w], Input.Bias.w);"

                        "	     Output.Position = Output.RawPosition = mul(mul(Input.Position, Offset), World);"
                        "    }"
                        "    else"
                        "	     Output.Position = Output.RawPosition = mul(Input.Position, World);"

                        "	 return Output;"
                        "}"

                        "float4 PS(VertexOut Input) : SV_TARGET0"
                        "{"
                        "    float Alpha = (1.0f - Diffusion.y);"
                        "    [branch] if (SurfaceDiffuse > 0)"
                        "        Alpha *= Input1.Sample(State, Input.TexCoord * TexCoord).w;"

                        "	 [branch] if (Alpha < Diffusion.x)"
                        "		 discard;"

                        "    return float4(length(Input.RawPosition.xyz - Position) / Distance, Alpha, 1.0f, 1.0f);"
                        "};";
            }

            D3D11ElementSystemRenderer::D3D11ElementSystemRenderer(Engine::RenderSystem* Lab) : Engine::Renderers::ElementSystemRenderer(Lab)
            {
                Device = System->GetDevice()->As<D3D11Device>();
                Stride = sizeof(Compute::InfluenceVertex);

                Graphics::Shader::Desc I = Graphics::Shader::Desc();
                I.Data = GetMultiCode();
                I.Layout = nullptr;
                I.LayoutSize = 0;

                Shaders.Multi = (D3D11Shader*)Graphics::Shader::Create(System->GetDevice(), I);

                I.Data = GetDepthCode();
                Shaders.Depth = (D3D11Shader*)Graphics::Shader::Create(System->GetDevice(), I);

                I.Data = GetCubicDepthPointCode();
                Shaders.CubicDepthPoint = (D3D11Shader*)Graphics::Shader::Create(System->GetDevice(), I);

                I.Data = GetCubicDepthTriangleCode();
                Shaders.CubicDepthTriangle = (D3D11Shader*)Graphics::Shader::Create(System->GetDevice(), I);

                Device->CreateConstantBuffer(sizeof(CubicDepth), Shaders.CubicDepthTriangle->ConstantBuffer);
            }
            D3D11ElementSystemRenderer::~D3D11ElementSystemRenderer()
            {
				delete Shaders.Multi;
				delete Shaders.Depth;
				delete Shaders.CubicDepthTriangle;
				delete Shaders.CubicDepthPoint;
            }
            void D3D11ElementSystemRenderer::OnInitialize()
            {
                ElementSystems = System->GetScene()->GetComponents(Engine::ComponentId_Element_System);
            }
            void D3D11ElementSystemRenderer::OnRasterization(Rest::Timer* Time)
            {
                if (!ElementSystems || ElementSystems->Empty())
                    return;

                D3D11MultiRenderTarget2D* RefSurface = System->GetScene()->GetSurface()->As<D3D11MultiRenderTarget2D>();
                ID3D11Buffer* Buffer = nullptr;

                Device->SetDepthStencilState(Graphics::RenderLab_DepthStencil_None_Less);
                Device->SetBlendState(Graphics::RenderLab_Blend_Additive_Alpha);
                Device->ImmediateContext->IAGetPrimitiveTopology(&LastTopology);
                Device->ImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
                Device->ImmediateContext->IASetVertexBuffers(0, 1, &Buffer, &Stride, &Offset);
                Device->ImmediateContext->PSSetShader(Shaders.Multi->PixelShader, nullptr, 0);
                Device->ImmediateContext->VSSetShader(Shaders.Multi->VertexShader, nullptr, 0);
                Device->ImmediateContext->OMSetRenderTargets(RefSurface->GetSVTarget(), RefSurface->RenderTargetView, RefSurface->DepthStencilView);
                Device->ImmediateContext->RSSetViewports(1, &RefSurface->Viewport);

                Compute::Matrix4x4 View = System->GetScene()->View.ViewProjection * System->GetScene()->View.Projection.Invert();
                for (auto It = ElementSystems->Begin(); It != ElementSystems->End(); ++It)
                {
                    Engine::Components::ElementSystem* ElementSystem = (Engine::Components::ElementSystem*)*It;
                    if (!ElementSystem->Instance || !ElementSystem->Visibility)
                        continue;

                    ElementSystem->Instance->SendPool();
                    if (ElementSystem->Diffuse)
                        ElementSystem->Diffuse->Apply(Device, 1);
                    else
                        Device->RestoreTexture2D(1, 1);

                    Device->Render.World = System->GetScene()->View.Projection;
                    Device->Render.WorldViewProjection = (ElementSystem->QuadBased ? View : System->GetScene()->View.ViewProjection);
                    Device->Render.Diffusion = ElementSystem->Diffusion;
                    Device->Render.TexCoord = ElementSystem->TexCoord;
                    Device->Render.SurfaceDiffuse = (float)(ElementSystem->Diffuse != nullptr);

                    if (ElementSystem->StrongConnection)
                        Device->Render.WorldViewProjection = ElementSystem->GetEntity()->Transform->GetWorld() * Device->Render.WorldViewProjection;

                    Device->ImmediateContext->PSSetShaderResources(2, 1, &ElementSystem->Instance->As<D3D11InstanceBuffer>()->Resource);
                    Device->ImmediateContext->VSSetShaderResources(2, 1, &ElementSystem->Instance->As<D3D11InstanceBuffer>()->Resource);
                    Device->ImmediateContext->GSSetShader(ElementSystem->QuadBased ? Shaders.Multi->GeometryShader : nullptr, nullptr, 0);
                    Device->ImmediateContext->UpdateSubresource(Device->ConstantBuffer[Graphics::RenderBufferType_Render], 0, nullptr, &Device->Render, 0, 0);
                    Device->ImmediateContext->Draw((UINT)ElementSystem->Instance->GetArray()->Size(), 0);
                }

                Device->ImmediateContext->IASetPrimitiveTopology(LastTopology);
                Device->ImmediateContext->GSSetShader(nullptr, nullptr, 0);
            }
            void D3D11ElementSystemRenderer::OnPhaseRasterization(Rest::Timer* Time)
            {
                if (!ElementSystems || ElementSystems->Empty())
                    return;

                D3D11MultiRenderTarget2D* RefSurface = System->GetScene()->GetSurface()->As<D3D11MultiRenderTarget2D>();
                ID3D11Buffer* Buffer = nullptr;

                Device->SetDepthStencilState(Graphics::RenderLab_DepthStencil_None_Less);
                Device->SetBlendState(Graphics::RenderLab_Blend_Additive_Alpha);
                Device->ImmediateContext->IAGetPrimitiveTopology(&LastTopology);
                Device->ImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
                Device->ImmediateContext->IASetVertexBuffers(0, 1, &Buffer, &Stride, &Offset);
                Device->ImmediateContext->PSSetShader(Shaders.Multi->PixelShader, nullptr, 0);
                Device->ImmediateContext->VSSetShader(Shaders.Multi->VertexShader, nullptr, 0);
                Device->ImmediateContext->OMSetRenderTargets(RefSurface->GetSVTarget(), RefSurface->RenderTargetView, RefSurface->DepthStencilView);
                Device->ImmediateContext->RSSetViewports(1, &RefSurface->Viewport);

                Compute::Matrix4x4 View = System->GetScene()->View.ViewProjection * System->GetScene()->View.Projection.Invert();
                for (auto It = ElementSystems->Begin(); It != ElementSystems->End(); ++It)
                {
                    Engine::Components::ElementSystem* ElementSystem = (Engine::Components::ElementSystem*)*It;
                    if (!ElementSystem->Instance || ElementSystem->GetEntity()->Transform->Position.Distance(System->GetScene()->View.RealPosition) >= System->GetScene()->View.ViewDistance + ElementSystem->GetEntity()->Transform->Scale.Length())
                        continue;

                    if (Compute::MathCommon::IsClipping(System->GetScene()->View.ViewProjection, ElementSystem->GetEntity()->Transform->GetWorld(), 1.5f) != -1)
                        continue;

                    ElementSystem->Instance->SendPool();
                    if (ElementSystem->Diffuse)
                        ElementSystem->Diffuse->Apply(Device, 1);
                    else
                        Device->RestoreTexture2D(1, 1);

                    Device->Render.World = System->GetScene()->View.Projection;
                    Device->Render.WorldViewProjection = (ElementSystem->QuadBased ? View : System->GetScene()->View.ViewProjection);
                    Device->Render.Diffusion = ElementSystem->Diffusion;
                    Device->Render.TexCoord = ElementSystem->TexCoord;
                    Device->Render.SurfaceDiffuse = (float)(ElementSystem->Diffuse != nullptr);

                    if (ElementSystem->StrongConnection)
                        Device->Render.WorldViewProjection = ElementSystem->GetEntity()->Transform->GetWorld() * Device->Render.WorldViewProjection;

                    Device->ImmediateContext->PSSetShaderResources(2, 1, &ElementSystem->Instance->As<D3D11InstanceBuffer>()->Resource);
                    Device->ImmediateContext->VSSetShaderResources(2, 1, &ElementSystem->Instance->As<D3D11InstanceBuffer>()->Resource);
                    Device->ImmediateContext->GSSetShader(ElementSystem->QuadBased ? Shaders.Multi->GeometryShader : nullptr, nullptr, 0);
                    Device->ImmediateContext->UpdateSubresource(Device->ConstantBuffer[Graphics::RenderBufferType_Render], 0, nullptr, &Device->Render, 0, 0);
                    Device->ImmediateContext->Draw((UINT)ElementSystem->Instance->GetArray()->Size(), 0);
                }

                Device->ImmediateContext->IASetPrimitiveTopology(LastTopology);
                Device->ImmediateContext->GSSetShader(nullptr, nullptr, 0);
            }
            void D3D11ElementSystemRenderer::OnDepthRasterization(Rest::Timer* Time)
            {
                if (!ElementSystems || ElementSystems->Empty())
                    return;

                D3D11MultiRenderTarget2D* RefSurface = System->GetScene()->GetSurface()->As<D3D11MultiRenderTarget2D>();
                ID3D11Buffer* Buffer = nullptr;

                Device->SetDepthStencilState(Graphics::RenderLab_DepthStencil_None_Less);
                Device->ImmediateContext->IAGetPrimitiveTopology(&LastTopology);
                Device->ImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
                Device->ImmediateContext->IASetVertexBuffers(0, 1, &Buffer, &Stride, &Offset);
                Device->ImmediateContext->PSSetShader(Shaders.Depth->PixelShader, nullptr, 0);
                Device->ImmediateContext->VSSetShader(Shaders.Depth->VertexShader, nullptr, 0);

                Compute::Matrix4x4 View = System->GetScene()->View.ViewProjection * System->GetScene()->View.Projection.Invert();
                for (auto It = ElementSystems->Begin(); It != ElementSystems->End(); ++It)
                {
                    Engine::Components::ElementSystem* ElementSystem = (Engine::Components::ElementSystem*)*It;
                    if (!ElementSystem->Instance || ElementSystem->GetEntity()->Transform->Position.Distance(System->GetScene()->View.RealPosition) >= System->GetScene()->View.ViewDistance + ElementSystem->GetEntity()->Transform->Scale.Length())
                        continue;

                    if (Compute::MathCommon::IsClipping(System->GetScene()->View.ViewProjection, ElementSystem->GetEntity()->Transform->GetWorld(), 1.5f) != -1)
                        continue;

                    Graphics::Material& Material = ElementSystem->GetMaterial();
                    Device->Render.World = ElementSystem->QuadBased ? System->GetScene()->View.Projection : Compute::Matrix4x4::Identity();
                    Device->Render.WorldViewProjection = (ElementSystem->QuadBased ? View : System->GetScene()->View.ViewProjection);
                    Device->Render.TexCoord = ElementSystem->TexCoord;
                    Device->Render.SurfaceDiffuse = (float)(ElementSystem->Diffuse != nullptr);
                    Device->Render.Diffusion.X = Material.Transmittance;
                    Device->Render.Diffusion.Y = Material.Shadowness;

                    if (ElementSystem->StrongConnection)
                        Device->Render.WorldViewProjection = ElementSystem->GetEntity()->Transform->GetWorld() * Device->Render.WorldViewProjection;

                    if (ElementSystem->Diffuse)
                        ElementSystem->Diffuse->Apply(Device, 1);
                    else
                        Device->RestoreTexture2D(1, 1);

                    Device->ImmediateContext->PSSetShaderResources(2, 1, &ElementSystem->Instance->As<D3D11InstanceBuffer>()->Resource);
                    Device->ImmediateContext->VSSetShaderResources(2, 1, &ElementSystem->Instance->As<D3D11InstanceBuffer>()->Resource);
                    Device->ImmediateContext->GSSetShader(ElementSystem->QuadBased ? Shaders.Depth->GeometryShader : nullptr, nullptr, 0);
                    Device->ImmediateContext->UpdateSubresource(Device->ConstantBuffer[Graphics::RenderBufferType_Render], 0, nullptr, &Device->Render, 0, 0);
                    Device->ImmediateContext->Draw((UINT)ElementSystem->Instance->GetArray()->Size(), 0);
                }

                Device->ImmediateContext->IASetPrimitiveTopology(LastTopology);
                Device->ImmediateContext->GSSetShader(nullptr, nullptr, 0);
            }
            void D3D11ElementSystemRenderer::OnCubicDepthRasterization(Rest::Timer* Time, Compute::Matrix4x4* ViewProjection)
            {
                if (!ElementSystems || ElementSystems->Empty())
                    return;

                D3D11MultiRenderTarget2D* RefSurface = System->GetScene()->GetSurface()->As<D3D11MultiRenderTarget2D>();
                ID3D11Buffer* Buffer = nullptr;

                CubicDepth.SliceView[0] = Compute::Matrix4x4::CreateCubeMapLookAt(0, System->GetScene()->View.Position);
                CubicDepth.SliceView[1] = Compute::Matrix4x4::CreateCubeMapLookAt(1, System->GetScene()->View.Position);
                CubicDepth.SliceView[2] = Compute::Matrix4x4::CreateCubeMapLookAt(2, System->GetScene()->View.Position);
                CubicDepth.SliceView[3] = Compute::Matrix4x4::CreateCubeMapLookAt(3, System->GetScene()->View.Position);
                CubicDepth.SliceView[4] = Compute::Matrix4x4::CreateCubeMapLookAt(4, System->GetScene()->View.Position);
                CubicDepth.SliceView[5] = Compute::Matrix4x4::CreateCubeMapLookAt(5, System->GetScene()->View.Position);
                CubicDepth.Projection = System->GetScene()->View.Projection;
                CubicDepth.Distance = System->GetScene()->View.ViewDistance;
                CubicDepth.Position = System->GetScene()->View.Position;

                Device->SetDepthStencilState(Graphics::RenderLab_DepthStencil_None_Less);
                Device->ImmediateContext->IAGetPrimitiveTopology(&LastTopology);
                Device->ImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
                Device->ImmediateContext->IASetVertexBuffers(0, 1, &Buffer, &Stride, &Offset);
                Device->ImmediateContext->PSSetConstantBuffers(3, 1, &Shaders.CubicDepthTriangle->ConstantBuffer);
                Device->ImmediateContext->VSSetConstantBuffers(3, 1, &Shaders.CubicDepthTriangle->ConstantBuffer);
                Device->ImmediateContext->GSSetConstantBuffers(3, 1, &Shaders.CubicDepthTriangle->ConstantBuffer);
                Device->ImmediateContext->UpdateSubresource(Shaders.CubicDepthTriangle->ConstantBuffer, 0, nullptr, &CubicDepth, 0, 0);

                for (auto It = ElementSystems->Begin(); It != ElementSystems->End(); ++It)
                {
                    Engine::Components::ElementSystem* ElementSystem = (Engine::Components::ElementSystem*)*It;
                    if (!ElementSystem->Instance || ElementSystem->GetEntity()->Transform->Position.Distance(System->GetScene()->View.RealPosition) >= System->GetScene()->View.ViewDistance + ElementSystem->GetEntity()->Transform->Scale.Length())
                        continue;

                    Graphics::Material& Material = ElementSystem->GetMaterial();
                    Device->Render.World = (ElementSystem->StrongConnection ? ElementSystem->GetEntity()->Transform->GetWorld() : Compute::Matrix4x4::Identity());
                    Device->Render.TexCoord = ElementSystem->TexCoord;
                    Device->Render.SurfaceDiffuse = (float)(ElementSystem->Diffuse != nullptr);
                    Device->Render.Diffusion.X = Material.Transmittance;
                    Device->Render.Diffusion.Y = Material.Shadowness;

                    if (ElementSystem->Diffuse)
                        ElementSystem->Diffuse->Apply(Device, 1);
                    else
                        Device->RestoreTexture2D(1, 1);

                    if (ElementSystem->QuadBased)
                    {
                        Device->ImmediateContext->PSSetShader(Shaders.CubicDepthTriangle->PixelShader, nullptr, 0);
                        Device->ImmediateContext->VSSetShader(Shaders.CubicDepthTriangle->VertexShader, nullptr, 0);
                        Device->ImmediateContext->GSSetShader(Shaders.CubicDepthTriangle->GeometryShader, nullptr, 0);
                    }
                    else
                    {
                        Device->ImmediateContext->PSSetShader(Shaders.CubicDepthPoint->PixelShader, nullptr, 0);
                        Device->ImmediateContext->VSSetShader(Shaders.CubicDepthPoint->VertexShader, nullptr, 0);
                        Device->ImmediateContext->GSSetShader(Shaders.CubicDepthPoint->GeometryShader, nullptr, 0);
                    }

                    Device->ImmediateContext->PSSetShaderResources(2, 1, &ElementSystem->Instance->As<D3D11InstanceBuffer>()->Resource);
                    Device->ImmediateContext->VSSetShaderResources(2, 1, &ElementSystem->Instance->As<D3D11InstanceBuffer>()->Resource);
                    Device->ImmediateContext->UpdateSubresource(Device->ConstantBuffer[Graphics::RenderBufferType_Render], 0, nullptr, &Device->Render, 0, 0);
                    Device->ImmediateContext->Draw((UINT)ElementSystem->Instance->GetArray()->Size(), 0);
                }

                Device->ImmediateContext->IASetPrimitiveTopology(LastTopology);
                Device->ImmediateContext->GSSetShader(nullptr, nullptr, 0);
            }
            const char* D3D11ElementSystemRenderer::GetMultiCode()
            {
                return
                        "#include InstanceElement.h"
                        "#include RenderBuffer.h"

                        "Texture2D Input1 : register(t1);"
                        "SamplerState State : register(s0);"
                        "StructuredBuffer<InstanceElement> Elements : register(t2);"

                        "struct VertexIn"
                        "{"
                        "    uint Position : SV_VERTEXID;"
                        "};"

                        "struct VertexOut"
                        "{"
                        "    float4 Position : SV_POSITION;"
                        "    float4 Color : TEXCOORD0;"
                        "    float2 TexCoord : TEXCOORD2;"
                        "    float Scale : TEXCOORD3;"
                        "    float Rotation : TEXCOORD4;"
                        "};"

                        "VertexOut TransformElement(VertexOut Input, float2 Offset, float2 TexCoord2)"
                        "{"
                        "    float Sin = sin(Input.Rotation), Cos = cos(Input.Rotation);"
                        "    Input.Position.xy += float2(Offset.x * Cos - Offset.y * Sin, Offset.x * Sin + Offset.y * Cos);"
                        "    Input.Position = mul(Input.Position, World);"
                        "    Input.TexCoord = TexCoord2;"
                        "    return Input;"
                        "}"

                        "[maxvertexcount(4)]"
                        "void GS(point VertexOut Input[1], inout TriangleStream<VertexOut> Stream)"
                        "{"
                        "    Stream.Append(TransformElement(Input[0], float2(-1, -1) * Input[0].Scale, float2(0, 0)));"
                        "    Stream.Append(TransformElement(Input[0], float2(-1, 1) * Input[0].Scale, float2(0, -1)));"
                        "    Stream.Append(TransformElement(Input[0], float2(1, -1) * Input[0].Scale, float2(1, 0)));"
                        "    Stream.Append(TransformElement(Input[0], float2(1, 1) * Input[0].Scale, float2(1, -1)));"
                        "    Stream.RestartStrip();"
                        "}"

                        "VertexOut VS(VertexIn Input)"
                        "{"
                        "    VertexOut Output = (VertexOut)0;"
                        "    Output.Position = mul(float4(Elements[Input.Position].Position, 1), WorldViewProjection);"
                        "    Output.Rotation = Elements[Input.Position].Rotation;"
                        "    Output.Color = Elements[Input.Position].Color;"
                        "    Output.Scale = Elements[Input.Position].Scale;"
                        "    return Output;"
                        "}"

                        "float4 PS(VertexOut Input) : SV_TARGET0"
                        "{"
                        "    [branch] if (SurfaceDiffuse > 0)"
                        "        return float4(Input.Color.xyz * Diffusion, Input.Color.w) * Input1.Sample(State, Input.TexCoord * TexCoord);"

                        "    return float4(Input.Color.xyz * Diffusion, Input.Color.w);"
                        "};";
            }
            const char* D3D11ElementSystemRenderer::GetDepthCode()
            {
                return
                        "#include InstanceElement.h"
                        "#include RenderBuffer.h"

                        "Texture2D Input1 : register(t1);"
                        "SamplerState State : register(s0);"
                        "StructuredBuffer<InstanceElement> Elements : register(t2);"

                        "struct VertexIn"
                        "{"
                        "    uint Position : SV_VERTEXID;"
                        "};"

                        "struct VertexOut"
                        "{"
                        "    float4 Position : SV_POSITION;"
                        "    float4 RawPosition : TEXCOORD0;"
                        "    float2 TexCoord : TEXCOORD1;"
                        "    float Rotation : TEXCOORD2;"
                        "    float Scale : TEXCOORD3;"
                        "    float Alpha : TEXCOORD4;"
                        "};"

                        "VertexOut TransformElement(VertexOut Input, float2 Offset, float2 TexCoord2)"
                        "{"
                        "    float Sin = sin(Input.Rotation), Cos = cos(Input.Rotation);"
                        "    Input.Position.xy += float2(Offset.x * Cos - Offset.y * Sin, Offset.x * Sin + Offset.y * Cos);"
                        "    Input.Position = mul(Input.Position, World);"
                        "    Input.TexCoord = TexCoord2;"
                        "    return Input;"
                        "}"

                        "[maxvertexcount(4)]"
                        "void GS(point VertexOut Input[1], inout TriangleStream<VertexOut> Stream)"
                        "{"
                        "    Stream.Append(TransformElement(Input[0], float2(1, -1) * Input[0].Scale, float2(0, 0)));"
                        "    Stream.Append(TransformElement(Input[0], float2(1, 1) * Input[0].Scale, float2(0, -1)));"
                        "    Stream.Append(TransformElement(Input[0], float2(-1, -1) * Input[0].Scale, float2(1, 0)));"
                        "    Stream.Append(TransformElement(Input[0], float2(-1, 1) * Input[0].Scale, float2(1, -1)));"
                        "    Stream.RestartStrip();"
                        "}"

                        "VertexOut VS(VertexIn Input)"
                        "{"
                        "    VertexOut Output = (VertexOut)0;"
                        "    Output.Position = mul(float4(Elements[Input.Position].Position, 1), WorldViewProjection);"
                        "    Output.RawPosition = mul(Output.Position, World);"
                        "    Output.Rotation = Elements[Input.Position].Rotation;"
                        "    Output.Scale = Elements[Input.Position].Scale;"
                        "    Output.Alpha = Elements[Input.Position].Color.w;"
                        "    return Output;"
                        "}"

                        "float4 PS(VertexOut Input) : SV_TARGET0"
                        "{"
                        "    float Alpha = (1.0f - Diffusion.y) * Input.Alpha;"
                        "    [branch] if (SurfaceDiffuse > 0)"
                        "        Alpha *= Input1.Sample(State, Input.TexCoord * TexCoord).w;"

                        "	 [branch] if (Alpha < Diffusion.x)"
                        "		 discard;"

                        "    return float4(Input.RawPosition.z / Input.RawPosition.w, Alpha, 1.0f, 1.0f);"
                        "};";
            }
            const char* D3D11ElementSystemRenderer::GetCubicDepthPointCode()
            {
                return
                        "#include InstanceElement.h"
                        "#include RenderBuffer.h"

                        "Texture2D Input1 : register(t1);"
                        "SamplerState State : register(s0);"
                        "StructuredBuffer<InstanceElement> Elements : register(t2);"

                        "cbuffer ViewBuffer : register(b3)"
                        "{"
                        "	 matrix SliceView[6];"
                        "	 matrix Projection;"
                        "    float3 Position;"
                        "    float Distance;"
                        "};"

                        "struct VertexIn"
                        "{"
                        "    uint Position : SV_VERTEXID;"
                        "};"

                        "struct VertexCubeOut"
                        "{"
                        "    float4 Position : SV_POSITION;"
                        "    float4 RawPosition : TEXCOORD0;"
                        "    float2 TexCoord : TEXCOORD1;"
                        "    float Rotation : TEXCOORD2;"
                        "    float Scale : TEXCOORD3;"
                        "    float Alpha : TEXCOORD4;"
                        "	 uint RenderTarget : SV_RenderTargetArrayIndex;"
                        "};"

                        "struct VertexOut"
                        "{"
                        "    float4 Position : SV_POSITION;"
                        "    float4 RawPosition : TEXCOORD0;"
                        "    float2 TexCoord : TEXCOORD1;"
                        "    float Rotation : TEXCOORD2;"
                        "    float Scale : TEXCOORD3;"
                        "    float Alpha : TEXCOORD4;"
                        "};"

                        "VertexCubeOut TransformElement(VertexOut Input, float2 Offset, float2 TexCoord2, uint i)"
                        "{"
                        "    float Sin = sin(Input.Rotation), Cos = cos(Input.Rotation);"
                        "	 Input.Position = mul(Input.Position, SliceView[i]);"
                        "	 Input.Position += float4(Offset.x * Cos - Offset.y * Sin, Offset.x * Sin + Offset.y * Cos, 0, 0);"
                        "	 Input.Position = mul(Input.Position, Projection);"

                        "    VertexCubeOut Output = (VertexCubeOut)0;"
                        "	 Output.Position = Input.Position;"
                        "	 Output.RawPosition = Input.RawPosition;"
                        "	 Output.Rotation = Input.Rotation;"
                        "	 Output.Scale = Input.Scale;"
                        "	 Output.Alpha = Input.Alpha;"
                        "	 Output.RenderTarget = i;"
                        "	 Output.TexCoord = TexCoord2;"

                        "    return Output;"
                        "}"

                        "[maxvertexcount(6)]"
                        "void GS(point VertexOut Input[1], inout PointStream<VertexCubeOut> Stream)"
                        "{"
                        "	 for (uint i = 0; i < 6; i++)"
                        "	 {"
                        "		 Stream.Append(TransformElement(Input[0], float2(0, 0), float2(0, 0), i));"
                        "		 Stream.RestartStrip();"
                        "	 }"
                        "}"

                        "VertexOut VS(VertexIn Input)"
                        "{"
                        "    VertexOut Output = (VertexOut)0;"
                        "    Output.Position = Output.RawPosition = mul(float4(Elements[Input.Position].Position, 1), World);"
                        "    Output.Rotation = Elements[Input.Position].Rotation;"
                        "    Output.Scale = Elements[Input.Position].Scale;"
                        "    Output.Alpha = Elements[Input.Position].Color.w;"
                        "    return Output;"
                        "}"

                        "float4 PS(VertexOut Input) : SV_TARGET0"
                        "{"
                        "    float Alpha = (1.0f - Diffusion.y) * Input.Alpha;"
                        "    [branch] if (SurfaceDiffuse > 0)"
                        "        Alpha *= Input1.Sample(State, Input.TexCoord * TexCoord).w;"

                        "	 [branch] if (Alpha < Diffusion.x)"
                        "		 discard;"

                        "    return float4(length(Input.RawPosition.xyz - Position) / Distance, Alpha, 1.0f, 1.0f);"
                        "};";
            }
            const char* D3D11ElementSystemRenderer::GetCubicDepthTriangleCode()
            {
                return
                        "#include InstanceElement.h"
                        "#include RenderBuffer.h"

                        "Texture2D Input1 : register(t1);"
                        "SamplerState State : register(s0);"
                        "StructuredBuffer<InstanceElement> Elements : register(t2);"

                        "cbuffer ViewBuffer : register(b3)"
                        "{"
                        "	 matrix SliceView[6];"
                        "	 matrix Projection;"
                        "    float3 Position;"
                        "    float Distance;"
                        "};"

                        "struct VertexIn"
                        "{"
                        "    uint Position : SV_VERTEXID;"
                        "};"

                        "struct VertexCubeOut"
                        "{"
                        "    float4 Position : SV_POSITION;"
                        "    float4 RawPosition : TEXCOORD0;"
                        "    float2 TexCoord : TEXCOORD1;"
                        "    float Rotation : TEXCOORD2;"
                        "    float Scale : TEXCOORD3;"
                        "    float Alpha : TEXCOORD4;"
                        "	 uint RenderTarget : SV_RenderTargetArrayIndex;"
                        "};"

                        "struct VertexOut"
                        "{"
                        "    float4 Position : SV_POSITION;"
                        "    float4 RawPosition : TEXCOORD0;"
                        "    float2 TexCoord : TEXCOORD1;"
                        "    float Rotation : TEXCOORD2;"
                        "    float Scale : TEXCOORD3;"
                        "    float Alpha : TEXCOORD4;"
                        "};"

                        "VertexCubeOut TransformElement(VertexOut Input, float2 Offset, float2 TexCoord2, uint i)"
                        "{"
                        "    float Sin = sin(Input.Rotation), Cos = cos(Input.Rotation);"
                        "	 Input.Position = mul(Input.Position, SliceView[i]);"
                        "	 Input.Position += float4(Offset.x * Cos - Offset.y * Sin, Offset.x * Sin + Offset.y * Cos, 0, 0);"
                        "	 Input.Position = mul(Input.Position, Projection);"

                        "    VertexCubeOut Output = (VertexCubeOut)0;"
                        "	 Output.Position = Input.Position;"
                        "	 Output.RawPosition = Input.RawPosition;"
                        "	 Output.Rotation = Input.Rotation;"
                        "	 Output.Scale = Input.Scale;"
                        "	 Output.Alpha = Input.Alpha;"
                        "	 Output.RenderTarget = i;"
                        "	 Output.TexCoord = TexCoord2;"

                        "    return Output;"
                        "}"

                        "[maxvertexcount(24)]"
                        "void GS(point VertexOut Input[1], inout TriangleStream<VertexCubeOut> Stream)"
                        "{"
                        "	 for (uint i = 0; i < 6; i++)"
                        "	 {"
                        "		 Stream.Append(TransformElement(Input[0], float2(1, -1) * Input[0].Scale, float2(0, 0), i));"
                        "		 Stream.Append(TransformElement(Input[0], float2(1, 1) * Input[0].Scale, float2(0, -1), i));"
                        "		 Stream.Append(TransformElement(Input[0], float2(-1, -1) * Input[0].Scale, float2(1, 0), i));"
                        "		 Stream.Append(TransformElement(Input[0], float2(-1, 1) * Input[0].Scale, float2(1, -1), i));"
                        "		 Stream.RestartStrip();"
                        "	 }"
                        "}"

                        "VertexOut VS(VertexIn Input)"
                        "{"
                        "    VertexOut Output = (VertexOut)0;"
                        "    Output.Position = Output.RawPosition = mul(float4(Elements[Input.Position].Position, 1), World);"
                        "    Output.Rotation = Elements[Input.Position].Rotation;"
                        "    Output.Scale = Elements[Input.Position].Scale;"
                        "    Output.Alpha = Elements[Input.Position].Color.w;"
                        "    return Output;"
                        "}"

                        "float4 PS(VertexOut Input) : SV_TARGET0"
                        "{"
                        "    float Alpha = (1.0f - Diffusion.y) * Input.Alpha;"
                        "    [branch] if (SurfaceDiffuse > 0)"
                        "        Alpha *= Input1.Sample(State, Input.TexCoord * TexCoord).w;"

                        "	 [branch] if (Alpha < Diffusion.x)"
                        "		 discard;"

                        "    return float4(length(Input.RawPosition.xyz - Position) / Distance, Alpha, 1.0f, 1.0f);"
                        "};";
            }

            D3D11DepthRenderer::D3D11DepthRenderer(Engine::RenderSystem* Lab) : Engine::Renderers::DepthRenderer(Lab)
            {
                Device = System->GetDevice()->As<D3D11Device>();
                Time.Delay = 10;
            }
            void D3D11DepthRenderer::OnInitialize()
            {
                PointLights = System->GetScene()->GetComponents(Engine::ComponentId_Point_Light);
                SpotLights = System->GetScene()->GetComponents(Engine::ComponentId_Spot_Light);
                LineLights = System->GetScene()->GetComponents(Engine::ComponentId_Line_Light);

                Rest::Pool<Engine::Component*>* Lights = System->GetScene()->GetComponents(Engine::ComponentId_Point_Light);
                for (auto It = Lights->Begin(); It != Lights->End(); It++)
                    (*It)->As<Engine::Components::PointLight>()->Occlusion = nullptr;

                for (auto It = Renderers.PointLight.begin(); It != Renderers.PointLight.end(); It++)
					delete *It;

                Renderers.PointLight.resize(Renderers.PointLightLimits);
                for (auto It = Renderers.PointLight.begin(); It != Renderers.PointLight.end(); It++)
                {
                    Graphics::RenderTargetCube::Desc I = Graphics::RenderTargetCube::Desc();
                    I.Size = (unsigned int)Renderers.PointLightResolution;
                    I.FormatMode = Graphics::Format_R32G32_Float;

                    *It = Graphics::RenderTargetCube::Create(System->GetDevice(), I);
                }

                Lights = System->GetScene()->GetComponents(Engine::ComponentId_Spot_Light);
                for (auto It = Lights->Begin(); It != Lights->End(); It++)
                    (*It)->As<Engine::Components::SpotLight>()->Occlusion = nullptr;

                for (auto It = Renderers.SpotLight.begin(); It != Renderers.SpotLight.end(); It++)
					delete *It;

                Renderers.SpotLight.resize(Renderers.SpotLightLimits);
                for (auto It = Renderers.SpotLight.begin(); It != Renderers.SpotLight.end(); It++)
                {
                    Graphics::RenderTarget2D::Desc I = Graphics::RenderTarget2D::Desc();
                    I.Width = (unsigned int)Renderers.SpotLightResolution;
                    I.Height = (unsigned int)Renderers.SpotLightResolution;
                    I.FormatMode = Graphics::Format_R32G32_Float;

                    *It = Graphics::RenderTarget2D::Create(System->GetDevice(), I);
                }

                Lights = System->GetScene()->GetComponents(Engine::ComponentId_Line_Light);
                for (auto It = Lights->Begin(); It != Lights->End(); It++)
                    (*It)->As<Engine::Components::LineLight>()->Occlusion = nullptr;

                for (auto It = Renderers.LineLight.begin(); It != Renderers.LineLight.end(); It++)
					delete *It;

                Renderers.LineLight.resize(Renderers.LineLightLimits);
                for (auto It = Renderers.LineLight.begin(); It != Renderers.LineLight.end(); It++)
                {
                    Graphics::RenderTarget2D::Desc I = Graphics::RenderTarget2D::Desc();
                    I.Width = (unsigned int)Renderers.LineLightResolution;
                    I.Height = (unsigned int)Renderers.LineLightResolution;
                    I.FormatMode = Graphics::Format_R32G32_Float;

                    *It = Graphics::RenderTarget2D::Create(System->GetDevice(), I);
                }
            }
            void D3D11DepthRenderer::OnIntervalRasterization(Rest::Timer* Time)
            {
                Device->SetBlendState(Graphics::RenderLab_Blend_Overwrite);
                Device->SetRasterizerState(Graphics::RenderLab_Raster_Cull_Front);

                UInt64 Shadows = 0;
                for (auto It = PointLights->Begin(); It != PointLights->End(); It++)
                {
                    Engine::Components::PointLight* Light = (Engine::Components::PointLight*)*It;
                    if (Shadows >= Renderers.PointLight.size())
                        break;

                    Light->Occlusion = nullptr;
                    if (!Light->Shadowed || Light->Visibility < ShadowDistance)
                        continue;

                    Renderers.PointLight[Shadows]->Apply(Device, 0, 0, 0);
                    RasterizeCubicDepth(Time, Light->Projection, Light->GetEntity()->Transform->Position.MtVector4().SetW(Light->ShadowDistance));

                    Light->Occlusion = Renderers.PointLight[Shadows++]->GetTarget()->GetResource();
                }

                Shadows = 0;
                for (auto It = SpotLights->Begin(); It != SpotLights->End(); It++)
                {
                    Engine::Components::SpotLight* Light = (Engine::Components::SpotLight*)*It;
                    if (Shadows >= Renderers.SpotLight.size())
                        break;

                    Light->Occlusion = nullptr;
                    if (!Light->Shadowed || Light->Visibility < ShadowDistance)
                        continue;

                    Renderers.SpotLight[Shadows]->Apply(Device, 0, 0, 0);
                    RasterizeDepth(Time, Light->View, Light->Projection, Light->GetEntity()->Transform->Position.MtVector4().SetW(Light->ShadowDistance));

                    Light->Occlusion = Renderers.SpotLight[Shadows++]->GetTarget()->GetResource();
                }

                Shadows = 0;
                for (auto It = LineLights->Begin(); It != LineLights->End(); It++)
                {
                    Engine::Components::LineLight* Light = (Engine::Components::LineLight*)*It;
                    if (Shadows >= Renderers.LineLight.size())
                        break;

                    Light->Occlusion = nullptr;
                    if (!Light->Shadowed)
                        continue;

                    Renderers.LineLight[Shadows]->Apply(Device, 0, 0, 0);
                    RasterizeDepth(Time, Light->View, Light->Projection, Compute::Vector4(0, 0, 0, -1));

                    Light->Occlusion = Renderers.LineLight[Shadows++]->GetTarget()->GetResource();
                }

                System->GetScene()->RestoreViewBuffer(nullptr);
                Device->SetRasterizerState(Graphics::RenderLab_Raster_Cull_Back);
            }

            D3D11ProbeRenderer::D3D11ProbeRenderer(Engine::RenderSystem* Lab) : Engine::Renderers::ProbeRenderer(Lab)
            {
                Device = System->GetDevice()->As<D3D11Device>();
                Stride = sizeof(Compute::ShapeVertex);
            }
            void D3D11ProbeRenderer::OnInitialize()
            {
                CreateRenderTarget();

                ProbeLights = System->GetScene()->GetComponents(Engine::ComponentId_Probe_Light);
				for (auto It = ProbeLights->Begin(); It != ProbeLights->End(); It++)
				{
					Engine::Components::ProbeLight* Light = (Engine::Components::ProbeLight*)*It;
					if (Light->Diffuse != nullptr || !Light->ImageBased)
					{
						delete Light->Diffuse;
						Light->Diffuse = nullptr;
					}
				}
            }
            void D3D11ProbeRenderer::OnRasterization(Rest::Timer* Time)
            {
                RefSurface = System->GetScene()->GetSurface();
                System->GetScene()->SetSurface(Surface);

                Float64 ElapsedTime = Time->GetElapsedTime();
                for (auto It = ProbeLights->Begin(); It != ProbeLights->End(); It++)
                {
                    Engine::Components::ProbeLight* Light = (Engine::Components::ProbeLight*)*It;
                    if (Light->Visibility <= 0 || Light->ImageBased)
                        continue;

                    if (ResourceBound(&Light->Diffuse))
                    {
                        if (!Light->Rebuild.OnTickEvent(ElapsedTime) || Light->Rebuild.Delay <= 0.0)
                            continue;
                    }

                    ID3D11ShaderResourceView** Resource = &Light->Diffuse->As<D3D11TextureCube>()->Resource;
					ReleaseCom((*Resource));

                    ID3D11Texture2D* Face = nullptr, *Dependence = nullptr;
                    Device->D3DDevice->CreateTexture2D(&CubeMapView, nullptr, &Dependence);
                    Light->RenderLocked = true;

                    Compute::Vector3 Position = Light->GetEntity()->Transform->Position * Light->ViewOffset;
                    for (int j = 0; j < 6; j++)
                    {
                        Light->View[j] = Compute::Matrix4x4::CreateCubeMapLookAt(j, Position);
                        RasterizePhase(Time, Light->View[j], Light->Projection, Position.MtVector4().SetW(Light->CaptureRange));

                        Device->D3DDevice->CreateTexture2D(&TextureView, nullptr, &Face);
                        Device->ImmediateContext->CopyResource(Face, Surface->As<D3D11MultiRenderTarget2D>()->Texture[0]);
                        Device->ImmediateContext->CopySubresourceRegion(Dependence, j * CubeMapView.MipLevels, 0, 0, 0, Face, 0, &Region);
						ReleaseCom(Face);
                    }

                    Light->RenderLocked = false;
                    Device->D3DDevice->CreateShaderResourceView(Dependence, &ShaderResourceView, Resource);
                    Device->ImmediateContext->GenerateMips(*Resource);
					ReleaseCom(Dependence);
                }

                System->GetScene()->SetSurface(RefSurface);
                System->GetScene()->RestoreViewBuffer(nullptr);
            }
            void D3D11ProbeRenderer::CreateRenderTarget()
            {
                if (Map == Size)
                    return;
                else
                    Map = Size;

                Graphics::MultiRenderTarget2D::Desc F1 = Graphics::MultiRenderTarget2D::Desc();
                F1.FormatMode[0] = Graphics::Format_R8G8B8A8_Unorm;
                F1.FormatMode[1] = Graphics::Format_R16G16B16A16_Float;
                F1.FormatMode[2] = Graphics::Format_R32G32_Float;
                F1.FormatMode[3] = Graphics::Format_Invalid;
                F1.FormatMode[4] = Graphics::Format_Invalid;
                F1.FormatMode[5] = Graphics::Format_Invalid;
                F1.FormatMode[6] = Graphics::Format_Invalid;
                F1.FormatMode[7] = Graphics::Format_Invalid;
                F1.SVTarget = Graphics::SurfaceTarget2;
                F1.MiscFlags = Graphics::ResourceMisc_Generate_Mips;
                F1.MipLevels = Device->GetMipLevelCount((unsigned int)Size, (unsigned int)Size);
                F1.Width = (unsigned int)Size;
                F1.Height = (unsigned int)Size;

                if (MipLevels > F1.MipLevels)
                    MipLevels = F1.MipLevels;
                else
                    F1.MipLevels = (unsigned int)MipLevels;

				delete Surface;
                Surface = Graphics::MultiRenderTarget2D::Create(System->GetDevice(), F1);
                if (Surface->As<D3D11MultiRenderTarget2D>()->Texture[0] != nullptr)
                    Surface->As<D3D11MultiRenderTarget2D>()->Texture[0]->GetDesc(&TextureView);

                TextureView.ArraySize = 1;
                TextureView.CPUAccessFlags = 0;
                TextureView.MiscFlags = 0;
                TextureView.MipLevels = (unsigned int)MipLevels;

                CubeMapView = TextureView;
                CubeMapView.MipLevels = TextureView.MipLevels;
                CubeMapView.ArraySize = 6;
                CubeMapView.Usage = D3D11_USAGE_DEFAULT;
                CubeMapView.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
                CubeMapView.CPUAccessFlags = 0;
                CubeMapView.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE | D3D11_RESOURCE_MISC_GENERATE_MIPS;

                ShaderResourceView.Format = CubeMapView.Format;
                ShaderResourceView.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
                ShaderResourceView.TextureCube.MostDetailedMip = 0;
                ShaderResourceView.TextureCube.MipLevels = CubeMapView.MipLevels;
                Region = { 0, 0, 0, (unsigned int)Size, (unsigned int)Size, 1 };
            }
            bool D3D11ProbeRenderer::ResourceBound(Graphics::TextureCube** Cube)
            {
                if (*Cube != nullptr)
                    return true;

                *Cube = Graphics::TextureCube::Create(Device);
                return false;
            }

            D3D11LightRenderer::D3D11LightRenderer(Engine::RenderSystem* Lab) : Engine::Renderers::LightRenderer(Lab)
            {
                Device = System->GetDevice()->As<D3D11Device>();
                Stride = sizeof(Compute::ShapeVertex);
                Models.Sphere[0] = System->VertexSphere()->As<D3D11ElementBuffer>();
                Models.Sphere[1] = System->IndexSphere()->As<D3D11ElementBuffer>();
                Models.Quad = System->VertexQuad()->As<D3D11ElementBuffer>();
                CreatePointLighting();
                CreateProbeLighting();
                CreateSpotLighting();
                CreateLineLighting();
                CreateAmbientLighting();
            }
            D3D11LightRenderer::~D3D11LightRenderer()
            {
				delete Shaders.PointLighting;
				delete Shaders.ProbeLighting;
				delete Shaders.ShadedPointLighting;
				delete Shaders.SpotLighting;
				delete Shaders.ShadedSpotLighting;
				delete Shaders.LineLighting;
				delete Shaders.ShadedLineLighting;
				delete Shaders.AmbientLighting;
            }
            void D3D11LightRenderer::OnInitialize()
            {
                PointLights = System->GetScene()->GetComponents(Engine::ComponentId_Point_Light);
                ProbeLights = System->GetScene()->GetComponents(Engine::ComponentId_Probe_Light);
                SpotLights = System->GetScene()->GetComponents(Engine::ComponentId_Spot_Light);
                LineLights = System->GetScene()->GetComponents(Engine::ComponentId_Line_Light);

                auto* RenderStages = System->GetRenderStages();
                for (auto It = RenderStages->begin(); It != RenderStages->end(); It++)
                {
                    if ((*It)->Is(Engine::RendererId_Depth))
                    {
                        Engine::Renderers::DepthRenderer* DepthRenderer = (*It)->As<Engine::Renderers::DepthRenderer>();
                        Quality.SpotLight = (float)DepthRenderer->Renderers.SpotLightResolution;
                        Quality.LineLight = (float)DepthRenderer->Renderers.LineLightResolution;
                    }

                    if ((*It)->Is(Engine::RendererId_Probe))
                        ProbeLight.Mipping = (float)(*It)->As<Engine::Renderers::ProbeRenderer>()->MipLevels;
                }

                CreateRenderTargets();
            }
            void D3D11LightRenderer::OnRelease()
            {
				delete Input;
				delete Output;
				delete PhaseInput;
				delete PhaseOutput;
            }
            void D3D11LightRenderer::OnRasterization(Rest::Timer* Time)
            {
                D3D11MultiRenderTarget2D* Surface = System->GetScene()->GetSurface()->As<D3D11MultiRenderTarget2D>();
                ID3D11ShaderResourceView* Nullable[5] = { nullptr };
                FLOAT Color[4] = { 0.0f };

                Device->SetBlendState(Graphics::RenderLab_Blend_Additive);
                Device->SetDepthStencilState(Graphics::RenderLab_DepthStencil_Greater_Equal);
                Device->ImmediateContext->CopyResource(Input->Texture, Surface->Texture[0]);
                Device->ImmediateContext->OMSetRenderTargets(1, &Output->RenderTargetView, Surface->DepthStencilView);
                Device->ImmediateContext->ClearRenderTargetView(Output->RenderTargetView, Color);
                Device->ImmediateContext->PSSetShaderResources(1, 1, &Surface->GetTarget(1)->As<D3D11Texture2D>()->Resource);
                Device->ImmediateContext->PSSetShaderResources(2, 1, &Surface->GetTarget(2)->As<D3D11Texture2D>()->Resource);
                Device->ImmediateContext->PSSetShaderResources(3, 1, &Input->GetTarget()->As<D3D11Texture2D>()->Resource);
                Device->ImmediateContext->PSSetConstantBuffers(3, 1, &Shaders.ProbeLighting->ConstantBuffer);
                Device->ImmediateContext->VSSetConstantBuffers(3, 1, &Shaders.ProbeLighting->ConstantBuffer);
                Device->ImmediateContext->VSSetShader(Shaders.ProbeLighting->VertexShader, nullptr, 0);
                Device->ImmediateContext->PSSetShader(Shaders.ProbeLighting->PixelShader, nullptr, 0);
                Device->ImmediateContext->IASetIndexBuffer(Models.Sphere[1]->Element, DXGI_FORMAT_R32_UINT, 0);
                Device->ImmediateContext->IASetVertexBuffers(0, 1, &Models.Sphere[0]->Element, &Stride, &Offset);
                Device->ImmediateContext->IASetInputLayout(Shaders.Layout);

                for (auto It = ProbeLights->Begin(); It != ProbeLights->End(); ++It)
                {
                    Engine::Components::ProbeLight* Light = (Engine::Components::ProbeLight*)*It;
                    if (Light->Visibility <= 0 || Light->Diffuse == nullptr)
                        continue;

                    ProbeLight.OwnWorldViewProjection = Compute::Matrix4x4::CreateTranslatedScale(Light->GetEntity()->Transform->Position, Light->Range * 1.25f) * System->GetScene()->View.ViewProjection;
                    ProbeLight.Position = Light->GetEntity()->Transform->Position.InvertZ();
                    ProbeLight.Lighting = Light->Diffusion.Mul(Light->Emission * Light->Visibility);
                    ProbeLight.Scale = Light->GetEntity()->Transform->Scale;
                    ProbeLight.Parallax = (Light->ParallaxCorrected ? 1.0f : 0.0f);
                    ProbeLight.Range = Light->Range;

                    Device->ImmediateContext->PSSetShaderResources(4, 1, &Light->Diffuse->As<D3D11TextureCube>()->Resource);
                    Device->ImmediateContext->UpdateSubresource(Shaders.ProbeLighting->ConstantBuffer, 0, nullptr, Shaders.ProbeLighting->ConstantData, 0, 0);
                    Device->ImmediateContext->DrawIndexed((unsigned int)Models.Sphere[1]->GetElements(), 0, 0);
                }

                Device->ImmediateContext->PSSetConstantBuffers(3, 1, &Shaders.PointLighting->ConstantBuffer);
                Device->ImmediateContext->VSSetConstantBuffers(3, 1, &Shaders.PointLighting->ConstantBuffer);
                Device->ImmediateContext->VSSetShader(Shaders.PointLighting->VertexShader, nullptr, 0);

                for (auto It = PointLights->Begin(); It != PointLights->End(); ++It)
                {
                    Engine::Components::PointLight* Light = (Engine::Components::PointLight*)*It;
                    if (Light->Visibility <= 0.0f)
                        continue;

                    if (Light->Shadowed && Light->Occlusion)
                    {
                        PointLight.Softness = Light->ShadowSoftness <= 0 ? 0 : Quality.SpotLight / Light->ShadowSoftness;
                        PointLight.Recount = 8.0f * Light->ShadowIterations * Light->ShadowIterations * Light->ShadowIterations;
                        PointLight.Bias = Light->ShadowBias;
                        PointLight.Distance = Light->ShadowDistance;
                        PointLight.Iterations = (float)Light->ShadowIterations;

                        Shaders.ActiveLighting = Shaders.ShadedPointLighting;
                        Device->ImmediateContext->PSSetShaderResources(4, 1, (ID3D11ShaderResourceView**)&Light->Occlusion);
                    }
                    else
                        Shaders.ActiveLighting = Shaders.PointLighting;

                    PointLight.OwnWorldViewProjection = Compute::Matrix4x4::CreateTranslatedScale(Light->GetEntity()->Transform->Position, Light->Range * 1.25f) * System->GetScene()->View.ViewProjection;
                    PointLight.Position = Light->GetEntity()->Transform->Position.InvertZ();
                    PointLight.Lighting = Light->Diffusion.Mul(Light->Emission * Light->Visibility);
                    PointLight.Range = Light->Range;

                    Device->ImmediateContext->PSSetShader(Shaders.ActiveLighting->PixelShader, nullptr, 0);
                    Device->ImmediateContext->UpdateSubresource(Shaders.PointLighting->ConstantBuffer, 0, nullptr, Shaders.PointLighting->ConstantData, 0, 0);
                    Device->ImmediateContext->DrawIndexed((unsigned int)Models.Sphere[1]->GetElements(), 0, 0);
                }

                Device->ImmediateContext->PSSetConstantBuffers(3, 1, &Shaders.SpotLighting->ConstantBuffer);
                Device->ImmediateContext->VSSetConstantBuffers(3, 1, &Shaders.SpotLighting->ConstantBuffer);
                Device->ImmediateContext->VSSetShader(Shaders.SpotLighting->VertexShader, nullptr, 0);
                Device->ImmediateContext->PSSetShaderResources(4, 1, Nullable);

                for (auto It = SpotLights->Begin(); It != SpotLights->End(); ++It)
                {
                    Engine::Components::SpotLight* Light = (Engine::Components::SpotLight*)*It;
                    if (Light->Visibility <= 0.0f)
                        continue;

                    if (Light->Shadowed && Light->Occlusion)
                    {
                        SpotLight.Softness = Light->ShadowSoftness <= 0 ? 0 : Quality.SpotLight / Light->ShadowSoftness;
                        SpotLight.Recount = 4.0f * Light->ShadowIterations * Light->ShadowIterations;
                        SpotLight.Bias = Light->ShadowBias;
                        SpotLight.Iterations = (float)Light->ShadowIterations;

                        Shaders.ActiveLighting = Shaders.ShadedSpotLighting;
                        Device->ImmediateContext->PSSetShaderResources(5, 1, (ID3D11ShaderResourceView**)&Light->Occlusion);
                    }
                    else
                        Shaders.ActiveLighting = Shaders.SpotLighting;

                    SpotLight.OwnWorldViewProjection = Compute::Matrix4x4::CreateScale(Light->Range * 1.25f) * Compute::Matrix4x4::CreateTranslation(Light->GetEntity()->Transform->Position) * System->GetScene()->View.ViewProjection;
                    SpotLight.OwnViewProjection = Light->View * Light->Projection;
                    SpotLight.Position = Light->GetEntity()->Transform->Position.InvertZ();
                    SpotLight.Lighting = Light->Diffusion.Mul(Light->Emission * Light->Visibility);
                    SpotLight.Range = Light->Range;
                    SpotLight.Diffuse = (Light->Diffuse != nullptr);

                    if (SpotLight.Diffuse > 0)
                        Light->Diffuse->Apply(Device, 4);
                    else
                        Device->RestoreTexture2D(4, 1);

                    Device->ImmediateContext->PSSetShader(Shaders.ActiveLighting->PixelShader, nullptr, 0);
                    Device->ImmediateContext->UpdateSubresource(Shaders.SpotLighting->ConstantBuffer, 0, nullptr, Shaders.SpotLighting->ConstantData, 0, 0);
                    Device->ImmediateContext->DrawIndexed((unsigned int)Models.Sphere[1]->GetElements(), 0, 0);
                }

                Device->ImmediateContext->OMSetRenderTargets(1, &Output->RenderTargetView, nullptr);
                Device->ImmediateContext->PSSetConstantBuffers(3, 1, &Shaders.LineLighting->ConstantBuffer);
                Device->ImmediateContext->VSSetShader(Shaders.LineLighting->VertexShader, nullptr, 0);
                Device->ImmediateContext->PSSetShader(Shaders.LineLighting->PixelShader, nullptr, 0);
                Device->ImmediateContext->IASetVertexBuffers(0, 1, &Models.Quad->Element, &Stride, &Offset);

                for (auto It = LineLights->Begin(); It != LineLights->End(); ++It)
                {
                    Engine::Components::LineLight* Light = (Engine::Components::LineLight*)*It;
                    if (Light->Shadowed && Light->Occlusion)
                    {
                        LineLight.OwnViewProjection = Light->View * Light->Projection;
                        LineLight.Softness = Light->ShadowSoftness <= 0 ? 0 : Quality.LineLight / Light->ShadowSoftness;
                        LineLight.Recount = 4.0f * Light->ShadowIterations * Light->ShadowIterations;
                        LineLight.Bias = Light->ShadowBias;
                        LineLight.ShadowDistance = Light->ShadowDistance / 2.0f;
                        LineLight.ShadowLength = Light->ShadowLength;
                        LineLight.Iterations = (float)Light->ShadowIterations;

                        Shaders.ActiveLighting = Shaders.ShadedLineLighting;
                        Device->ImmediateContext->PSSetShaderResources(4, 1, (ID3D11ShaderResourceView**)&Light->Occlusion);
                    }
                    else
                        Shaders.ActiveLighting = Shaders.LineLighting;

                    LineLight.Position = Light->GetEntity()->Transform->Position.InvertZ().NormalizeSafe();
                    LineLight.Lighting = Light->Diffusion.Mul(Light->Emission);

                    Device->ImmediateContext->PSSetShader(Shaders.ActiveLighting->PixelShader, nullptr, 0);
                    Device->ImmediateContext->UpdateSubresource(Shaders.LineLighting->ConstantBuffer, 0, nullptr, Shaders.LineLighting->ConstantData, 0, 0);
                    Device->ImmediateContext->Draw(6, 0);
                }

                Device->ImmediateContext->OMSetRenderTargets(1, &Surface->RenderTargetView[0], nullptr);
                Device->ImmediateContext->ClearRenderTargetView(Surface->RenderTargetView[0], Color);
                Device->ImmediateContext->VSSetShader(Shaders.AmbientLighting->VertexShader, nullptr, 0);
                Device->ImmediateContext->PSSetShader(Shaders.AmbientLighting->PixelShader, nullptr, 0);
                Device->ImmediateContext->PSSetShaderResources(4, 1, &Output->GetTarget()->As<D3D11Texture2D>()->Resource);
                Device->ImmediateContext->PSSetConstantBuffers(3, 1, &Shaders.AmbientLighting->ConstantBuffer);
                Device->ImmediateContext->UpdateSubresource(Shaders.AmbientLighting->ConstantBuffer, 0, nullptr, Shaders.AmbientLighting->ConstantData, 0, 0);
                Device->ImmediateContext->Draw(6, 0);
                Device->ImmediateContext->PSSetShaderResources(1, 5, Nullable);
            }
            void D3D11LightRenderer::OnPhaseRasterization(Rest::Timer* Time)
            {
                ID3D11ShaderResourceView* Nullable[5] = { nullptr };
                D3D11MultiRenderTarget2D* RefSurface = System->GetScene()->GetSurface()->As<D3D11MultiRenderTarget2D>();
                Engine::Viewer View = System->GetScene()->GetCameraViewer();
                FLOAT Color[4] = { 0.0f };

                Device->SetBlendState(Graphics::RenderLab_Blend_Additive);
                Device->SetDepthStencilState(Graphics::RenderLab_DepthStencil_Greater_Equal);
                Device->ImmediateContext->CopyResource(PhaseInput->Texture, RefSurface->Texture[0]);
                Device->ImmediateContext->OMSetRenderTargets(1, &PhaseOutput->RenderTargetView, RefSurface->DepthStencilView);
                Device->ImmediateContext->ClearRenderTargetView(PhaseOutput->RenderTargetView, Color);
                Device->ImmediateContext->PSSetShaderResources(1, 1, &RefSurface->GetTarget(1)->As<D3D11Texture2D>()->Resource);
                Device->ImmediateContext->PSSetShaderResources(2, 1, &RefSurface->GetTarget(2)->As<D3D11Texture2D>()->Resource);
                Device->ImmediateContext->PSSetShaderResources(3, 1, &PhaseInput->GetTarget()->As<D3D11Texture2D>()->Resource);
                Device->ImmediateContext->PSSetConstantBuffers(3, 1, &Shaders.ProbeLighting->ConstantBuffer);
                Device->ImmediateContext->VSSetConstantBuffers(3, 1, &Shaders.ProbeLighting->ConstantBuffer);
                Device->ImmediateContext->VSSetShader(Shaders.ProbeLighting->VertexShader, nullptr, 0);
                Device->ImmediateContext->PSSetShader(Shaders.ProbeLighting->PixelShader, nullptr, 0);
                Device->ImmediateContext->IASetIndexBuffer(Models.Sphere[1]->Element, DXGI_FORMAT_R32_UINT, 0);
                Device->ImmediateContext->IASetVertexBuffers(0, 1, &Models.Sphere[0]->Element, &Stride, &Offset);
                Device->ImmediateContext->IASetInputLayout(Shaders.Layout);

                if (RecursiveProbes)
                {
                    for (auto It = ProbeLights->Begin(); It != ProbeLights->End(); ++It)
                    {
                        Engine::Components::ProbeLight* Light = (Engine::Components::ProbeLight*)*It;
                        if (Light->Diffuse == nullptr || Light->RenderLocked)
                            continue;

                        float Visibility = Light->Visibility;
                        if (Light->Visibility <= 0 && (Visibility = 1.0f - Light->GetEntity()->Transform->Position.Distance(View.Position) / View.ViewDistance) <= 0.0f)
                            continue;

                        ProbeLight.OwnWorldViewProjection = Compute::Matrix4x4::CreateTranslatedScale(Light->GetEntity()->Transform->Position, Light->Range * 1.25f) * System->GetScene()->View.ViewProjection;
                        ProbeLight.Position = Light->GetEntity()->Transform->Position.InvertZ();
                        ProbeLight.Lighting = Light->Diffusion.Mul(Light->Emission * Visibility);
                        ProbeLight.Scale = Light->GetEntity()->Transform->Scale;
                        ProbeLight.Parallax = (Light->ParallaxCorrected ? 1.0f : 0.0f);
                        ProbeLight.Range = Light->Range;

                        Device->ImmediateContext->PSSetShaderResources(4, 1, &Light->Diffuse->As<D3D11TextureCube>()->Resource);
                        Device->ImmediateContext->UpdateSubresource(Shaders.ProbeLighting->ConstantBuffer, 0, nullptr, Shaders.ProbeLighting->ConstantData, 0, 0);
                        Device->ImmediateContext->DrawIndexed((unsigned int)Models.Sphere[1]->GetElements(), 0, 0);
                    }
                }

                Device->ImmediateContext->PSSetConstantBuffers(3, 1, &Shaders.PointLighting->ConstantBuffer);
                Device->ImmediateContext->VSSetConstantBuffers(3, 1, &Shaders.PointLighting->ConstantBuffer);
                Device->ImmediateContext->VSSetShader(Shaders.PointLighting->VertexShader, nullptr, 0);

                for (auto It = PointLights->Begin(); It != PointLights->End(); ++It)
                {
                    Engine::Components::PointLight* Light = (Engine::Components::PointLight*)*It;

                    float Visibility = 1.0f - Light->GetEntity()->Transform->Position.Distance(System->GetScene()->View.RealPosition) / System->GetScene()->View.ViewDistance;
                    if (Visibility <= 0.0f)
                        continue;

                    Visibility = Compute::MathCommon::IsClipping(System->GetScene()->View.ViewProjection, Light->GetEntity()->Transform->GetWorld(), Light->Range) == -1 ? Visibility : 0.0f;
                    if (Visibility <= 0.0f)
                        continue;

                    if (Light->Shadowed && Light->Occlusion)
                    {
                        PointLight.Softness = Light->ShadowSoftness <= 0 ? 0 : Quality.SpotLight / Light->ShadowSoftness;
                        PointLight.Recount = 8.0f * Light->ShadowIterations * Light->ShadowIterations * Light->ShadowIterations;
                        PointLight.Bias = Light->ShadowBias;
                        PointLight.Distance = Light->ShadowDistance;
                        PointLight.Iterations = (float)Light->ShadowIterations;

                        Shaders.ActiveLighting = Shaders.ShadedPointLighting;
                        Device->ImmediateContext->PSSetShaderResources(4, 1, (ID3D11ShaderResourceView**)&Light->Occlusion);
                    }
                    else
                        Shaders.ActiveLighting = Shaders.PointLighting;

                    PointLight.OwnWorldViewProjection = Compute::Matrix4x4::CreateTranslatedScale(Light->GetEntity()->Transform->Position, Light->Range * 1.25f) * System->GetScene()->View.ViewProjection;
                    PointLight.Position = Light->GetEntity()->Transform->Position.InvertZ();
                    PointLight.Lighting = Light->Diffusion.Mul(Light->Emission * Light->Visibility);
                    PointLight.Range = Light->Range;

                    Device->ImmediateContext->PSSetShader(Shaders.ActiveLighting->PixelShader, nullptr, 0);
                    Device->ImmediateContext->UpdateSubresource(Shaders.PointLighting->ConstantBuffer, 0, nullptr, Shaders.PointLighting->ConstantData, 0, 0);
                    Device->ImmediateContext->DrawIndexed((unsigned int)Models.Sphere[1]->GetElements(), 0, 0);
                }

                Device->ImmediateContext->PSSetConstantBuffers(3, 1, &Shaders.SpotLighting->ConstantBuffer);
                Device->ImmediateContext->VSSetConstantBuffers(3, 1, &Shaders.SpotLighting->ConstantBuffer);
                Device->ImmediateContext->VSSetShader(Shaders.SpotLighting->VertexShader, nullptr, 0);
                Device->ImmediateContext->PSSetShaderResources(4, 1, Nullable);

                for (auto It = SpotLights->Begin(); It != SpotLights->End(); ++It)
                {
                    Engine::Components::SpotLight* Light = (Engine::Components::SpotLight*)*It;

                    float Visibility = 1.0f - Light->GetEntity()->Transform->Position.Distance(System->GetScene()->View.RealPosition) / System->GetScene()->View.ViewDistance;
                    if (Visibility <= 0.0f)
                        continue;

                    Visibility = Compute::MathCommon::IsClipping(System->GetScene()->View.ViewProjection, Light->GetEntity()->Transform->GetWorld(), Light->Range) == -1 ? Visibility : 0.0f;
                    if (Visibility <= 0.0f)
                        continue;

                    if (Light->Shadowed && Light->Occlusion)
                    {
                        SpotLight.Softness = Light->ShadowSoftness <= 0 ? 0 : Quality.SpotLight / Light->ShadowSoftness;
                        SpotLight.Recount = 4.0f * Light->ShadowIterations * Light->ShadowIterations;
                        SpotLight.Bias = Light->ShadowBias;
                        SpotLight.Iterations = (float)Light->ShadowIterations;

                        Shaders.ActiveLighting = Shaders.ShadedSpotLighting;
                        Device->ImmediateContext->PSSetShaderResources(5, 1, (ID3D11ShaderResourceView**)&Light->Occlusion);
                    }
                    else
                        Shaders.ActiveLighting = Shaders.SpotLighting;

                    SpotLight.OwnWorldViewProjection = Compute::Matrix4x4::CreateScale(Light->Range * 1.25f) * Compute::Matrix4x4::CreateTranslation(Light->GetEntity()->Transform->Position) * System->GetScene()->View.ViewProjection;
                    SpotLight.OwnViewProjection = Light->View * Light->Projection;
                    SpotLight.Position = Light->GetEntity()->Transform->Position.InvertZ();
                    SpotLight.Lighting = Light->Diffusion.Mul(Light->Emission * Light->Visibility);
                    SpotLight.Range = Light->Range;
                    SpotLight.Diffuse = (Light->Diffuse != nullptr);

                    if (SpotLight.Diffuse > 0)
                        Light->Diffuse->Apply(Device, 4);
                    else
                        Device->RestoreTexture2D(4, 1);

                    Device->ImmediateContext->PSSetShader(Shaders.ActiveLighting->PixelShader, nullptr, 0);
                    Device->ImmediateContext->UpdateSubresource(Shaders.SpotLighting->ConstantBuffer, 0, nullptr, Shaders.SpotLighting->ConstantData, 0, 0);
                    Device->ImmediateContext->DrawIndexed((unsigned int)Models.Sphere[1]->GetElements(), 0, 0);
                }

                Device->ImmediateContext->OMSetRenderTargets(1, &PhaseOutput->RenderTargetView, nullptr);
                Device->ImmediateContext->PSSetConstantBuffers(3, 1, &Shaders.LineLighting->ConstantBuffer);
                Device->ImmediateContext->VSSetShader(Shaders.LineLighting->VertexShader, nullptr, 0);
                Device->ImmediateContext->PSSetShader(Shaders.LineLighting->PixelShader, nullptr, 0);
                Device->ImmediateContext->IASetVertexBuffers(0, 1, &Models.Quad->Element, &Stride, &Offset);

                for (auto It = LineLights->Begin(); It != LineLights->End(); ++It)
                {
                    Engine::Components::LineLight* Light = (Engine::Components::LineLight*)*It;
                    if (Light->Shadowed && Light->Occlusion)
                    {
                        LineLight.OwnViewProjection = Light->View * Light->Projection;
                        LineLight.Softness = Light->ShadowSoftness <= 0 ? 0 : Quality.LineLight / Light->ShadowSoftness;
                        LineLight.Recount = 4.0f * Light->ShadowIterations * Light->ShadowIterations;
                        LineLight.Bias = Light->ShadowBias;
                        LineLight.ShadowDistance = Light->ShadowDistance / 2.0f;
                        LineLight.ShadowLength = Light->ShadowLength;
                        LineLight.Iterations = (float)Light->ShadowIterations;

                        Shaders.ActiveLighting = Shaders.ShadedLineLighting;
                        Device->ImmediateContext->PSSetShaderResources(4, 1, (ID3D11ShaderResourceView**)&Light->Occlusion);
                    }
                    else
                        Shaders.ActiveLighting = Shaders.LineLighting;

                    LineLight.Position = Light->GetEntity()->Transform->Position.InvertZ().NormalizeSafe();
                    LineLight.Lighting = Light->Diffusion.Mul(Light->Emission);

                    Device->ImmediateContext->PSSetShader(Shaders.ActiveLighting->PixelShader, nullptr, 0);
                    Device->ImmediateContext->UpdateSubresource(Shaders.LineLighting->ConstantBuffer, 0, nullptr, Shaders.LineLighting->ConstantData, 0, 0);
                    Device->ImmediateContext->Draw(6, 0);
                }

                Device->ImmediateContext->OMSetRenderTargets(1, &RefSurface->RenderTargetView[0], nullptr);
                Device->ImmediateContext->ClearRenderTargetView(RefSurface->RenderTargetView[0], Color);
                Device->ImmediateContext->VSSetShader(Shaders.AmbientLighting->VertexShader, nullptr, 0);
                Device->ImmediateContext->PSSetShader(Shaders.AmbientLighting->PixelShader, nullptr, 0);
                Device->ImmediateContext->PSSetShaderResources(4, 1, &PhaseOutput->GetTarget()->As<D3D11Texture2D>()->Resource);
                Device->ImmediateContext->PSSetConstantBuffers(3, 1, &Shaders.AmbientLighting->ConstantBuffer);
                Device->ImmediateContext->UpdateSubresource(Shaders.AmbientLighting->ConstantBuffer, 0, nullptr, Shaders.AmbientLighting->ConstantData, 0, 0);
                Device->ImmediateContext->Draw(6, 0);
                Device->ImmediateContext->PSSetShaderResources(1, 5, Nullable);
            }
            void D3D11LightRenderer::OnResizeBuffers()
            {
				delete Input;
				delete Output;
				delete PhaseInput;
				delete PhaseOutput;
                CreateRenderTargets();
            }
            void D3D11LightRenderer::CreatePointLighting()
            {
                Graphics::Shader::Desc I = Graphics::Shader::Desc();
                I.Data = GetPointLightCode();
                I.Layout = Graphics::Shader::GetShapeVertexLayout();
                I.LayoutSize = 2;

                Shaders.PointLighting = (D3D11Shader*)Graphics::Shader::Create(Device, I);
                Shaders.PointLighting->ConstantData = &PointLight;
                Shaders.Layout = Shaders.PointLighting->VertexLayout;

                I.Data = GetShadedPointLightCode();

                Shaders.ShadedPointLighting = (D3D11Shader*)Graphics::Shader::Create(Device, I);
                ReleaseCom(Shaders.ShadedPointLighting->VertexLayout);

                Device->CreateConstantBuffer(sizeof(PointLight), Shaders.PointLighting->ConstantBuffer);
            }
            void D3D11LightRenderer::CreateProbeLighting()
            {
                Graphics::Shader::Desc I = Graphics::Shader::Desc();
                I.Data = GetProbeLightCode();
                I.Layout = Graphics::Shader::GetShapeVertexLayout();
                I.LayoutSize = 2;

                Shaders.ProbeLighting = (D3D11Shader*)Graphics::Shader::Create(Device, I);
                Shaders.ProbeLighting->ConstantData = &ProbeLight;
                ReleaseCom(Shaders.ProbeLighting->VertexLayout);

                Device->CreateConstantBuffer(sizeof(ProbeLight), Shaders.ProbeLighting->ConstantBuffer);
            }
            void D3D11LightRenderer::CreateSpotLighting()
            {
                Graphics::Shader::Desc I = Graphics::Shader::Desc();
                I.Data = GetSpotLightCode();
                I.Layout = Graphics::Shader::GetShapeVertexLayout();
                I.LayoutSize = 2;

                Shaders.SpotLighting = (D3D11Shader*)Graphics::Shader::Create(Device, I);
                Shaders.SpotLighting->ConstantData = &SpotLight;
                ReleaseCom(Shaders.SpotLighting->VertexLayout);

                I.Data = GetShadedSpotLightCode();

                Shaders.ShadedSpotLighting = (D3D11Shader*)Graphics::Shader::Create(Device, I);
                ReleaseCom(Shaders.ShadedSpotLighting->VertexLayout);

                Device->CreateConstantBuffer(sizeof(SpotLight), Shaders.SpotLighting->ConstantBuffer);
            }
            void D3D11LightRenderer::CreateLineLighting()
            {
                Graphics::Shader::Desc I = Graphics::Shader::Desc();
                I.Data = GetLineLightCode();
                I.Layout = Graphics::Shader::GetShapeVertexLayout();
                I.LayoutSize = 2;

                Shaders.LineLighting = (D3D11Shader*)Graphics::Shader::Create(Device, I);
                Shaders.LineLighting->ConstantData = &LineLight;
                ReleaseCom(Shaders.LineLighting->VertexLayout);

                I.Data = GetShadedLineLightCode();

                Shaders.ShadedLineLighting = (D3D11Shader*)Graphics::Shader::Create(Device, I);
                ReleaseCom(Shaders.ShadedLineLighting->VertexLayout);

                Device->CreateConstantBuffer(sizeof(LineLight), Shaders.LineLighting->ConstantBuffer);
            }
            void D3D11LightRenderer::CreateAmbientLighting()
            {
                Graphics::Shader::Desc I = Graphics::Shader::Desc();
                I.Data = GetAmbientLightCode();
                I.Layout = Graphics::Shader::GetShapeVertexLayout();
                I.LayoutSize = 2;

                Shaders.AmbientLighting = (D3D11Shader*)Graphics::Shader::Create(Device, I);
                Shaders.AmbientLighting->ConstantData = &AmbientLight;
                ReleaseCom(Shaders.AmbientLighting->VertexLayout);

                Device->CreateConstantBuffer(sizeof(AmbientLight), Shaders.AmbientLighting->ConstantBuffer);
            }
            void D3D11LightRenderer::CreateRenderTargets()
            {
                Graphics::RenderTarget2D::Desc I = Graphics::RenderTarget2D::Desc();
                I.FormatMode = Graphics::Format_R8G8B8A8_Unorm;
                I.MiscFlags = Graphics::ResourceMisc_Generate_Mips;
                I.Width = (unsigned int)System->GetScene()->GetSurface()->GetWidth();
                I.Height = (unsigned int)System->GetScene()->GetSurface()->GetHeight();
                I.MipLevels = Device->GetMipLevelCount(I.Width, I.Height);

                Output = (D3D11RenderTarget2D*)Graphics::RenderTarget2D::Create(System->GetDevice(), I);
                ReleaseCom(Output->DepthStencilView);

                Input = (D3D11RenderTarget2D*)Graphics::RenderTarget2D::Create(System->GetDevice(), I);
                ReleaseCom(Input->RenderTargetView);
                ReleaseCom(Input->DepthStencilView);

                auto* RenderStages = System->GetRenderStages();
                for (auto It = RenderStages->begin(); It != RenderStages->end(); It++)
                {
                    if (!(*It)->Is(Engine::RendererId_Probe))
                        continue;

                    Engine::Renderers::ProbeRenderer* ProbeRenderer = (*It)->As<Engine::Renderers::ProbeRenderer>();
                    I.Width = (unsigned int)ProbeRenderer->Size;
                    I.Height = (unsigned int)ProbeRenderer->Size;
                    I.MipLevels = (unsigned int)ProbeRenderer->MipLevels;
                    break;
                }

                PhaseOutput = (D3D11RenderTarget2D*)Graphics::RenderTarget2D::Create(System->GetDevice(), I);
                ReleaseCom(Output->DepthStencilView);

                PhaseInput = (D3D11RenderTarget2D*)Graphics::RenderTarget2D::Create(System->GetDevice(), I);
                ReleaseCom(Input->RenderTargetView);
                ReleaseCom(Input->DepthStencilView);
            }
            const char* D3D11LightRenderer::GetProbeLightCode()
            {
                return
                        "#include ShapeVertexIn.h"
                        "#include ShapeVertexOut.h"
                        "#include ViewBuffer.h"
                        "#include Geometry.h"
                        "#include LoadGeometry.h"

                        "cbuffer ProbeLight : register(b3)"
                        "{"
                        "    matrix OwnWorldViewProjection;"
                        "    float3 Position;"
                        "    float Range;"
                        "    float3 Lighting;"
                        "    float Mipping;"
                        "    float3 Scale;"
                        "    float Parallax;"
                        "};"

                        "TextureCube Probe1 : register(t4);"

                        "float Pow4(float Value)"
                        "{"
                        "    return Value * Value * Value * Value;"
                        "}"

                        "ShapeVertexOut VS(ShapeVertexIn Input)"
                        "{"
                        "    ShapeVertexOut Output = (ShapeVertexOut)0;"
                        "    Output.Position = mul(Input.Position, OwnWorldViewProjection);"
                        "    Output.TexCoord = Output.Position;"
                        "    return Output;"
                        "}"

                        "float4 PS(ShapeVertexOut Input) : SV_TARGET0"
                        "{"
                        "    Geometry F = LoadGeometrySV(InvViewProjection, Input.TexCoord);"
                        "    Material S = Materials[F.Material];"
                        "    float3 D = Position - F.Position;"
                        "    float3 M = S.Metallic + F.Surface.x * S.Micrometal;"
                        "    float R = Pow4(S.Roughness + F.Surface.y * S.Microrough);"
                        "    float A = 1.0f - length(D) / Range;"

                        "    D = -normalize(reflect(normalize(ViewPosition.xyz - F.Position), -F.Normal));"
                        "    [branch] if (Parallax > 0)"
                        "    {"
                        "        float3 Max = Position + Scale;"
                        "        float3 Min = Position - Scale;"
                        "        float3 Plane = ((D > 0.0 ? Max : Min) - F.Position) / D;"

                        "        D = F.Position + D * min(min(Plane.x, Plane.y), Plane.z) - Position;"
                        "    }"

                        "    return float4(Lighting * Probe1.SampleLevel(State, D, Mipping * R).xyz * M * A * S.Reflectance, A);"
                        "};";
            }
            const char* D3D11LightRenderer::GetPointLightCode()
            {
                return
                        "#include ShapeVertexIn.h"
                        "#include ShapeVertexOut.h"
                        "#include ViewBuffer.h"
                        "#include Geometry.h"
                        "#include LoadGeometry.h"
                        "#include CookTorrance.h"

                        "cbuffer PointLight : register(b3)"
                        "{"
                        "    matrix OwnWorldViewProjection;"
                        "    float3 Position;"
                        "    float Range;"
                        "    float3 Lighting;"
                        "    float Distance;"
                        "    float Softness;"
                        "    float Recount;"
                        "    float Bias;"
                        "    float Iterations;"
                        "};"

                        "float Pow4(float Value)"
                        "{"
                        "    return Value * Value * Value * Value;"
                        "}"

                        "ShapeVertexOut VS(ShapeVertexIn Input)"
                        "{"
                        "    ShapeVertexOut Output = (ShapeVertexOut)0;"
                        "    Output.Position = mul(Input.Position, OwnWorldViewProjection);"
                        "    Output.TexCoord = Output.Position;"
                        "    return Output;"
                        "}"

                        "float4 PS(ShapeVertexOut Input) : SV_TARGET0"
                        "{"
                        "    Geometry F = LoadGeometrySV(InvViewProjection, Input.TexCoord);"
                        "    Material S = Materials[F.Material];"
                        "    float3 D = Position - F.Position;"
                        "    float3 M = S.Metallic + F.Surface.x * S.Micrometal;"
                        "    float R = Pow4(S.Roughness + F.Surface.y * S.Microrough);"
                        "    float A = 1.0f - length(D) / Range;"
                        "    float3 V = normalize(ViewPosition.xyz - F.Position);"

                        "    return float4(Lighting * CookTorrance(V, normalize(D), F.Normal, M, R, D) * A, A);"
                        "};";
            }
            const char* D3D11LightRenderer::GetShadedPointLightCode()
            {
                return
                        "#include ShapeVertexIn.h"
                        "#include ShapeVertexOut.h"
                        "#include ViewBuffer.h"
                        "#include Geometry.h"
                        "#include LoadGeometry.h"
                        "#include CookTorrance.h"

                        "cbuffer PointLight : register(b3)"
                        "{"
                        "    matrix OwnWorldViewProjection;"
                        "    float3 Position;"
                        "    float Range;"
                        "    float3 Lighting;"
                        "    float Distance;"
                        "    float Softness;"
                        "    float Recount;"
                        "    float Bias;"
                        "    float Iterations;"
                        "};"

                        "TextureCube Probe1 : register(t4);"

                        "float Pow4(float Value)"
                        "{"
                        "    return Value * Value * Value * Value;"
                        "}"

                        "void SampleProbe(float3 D, float L, out float C, out float B)"
                        "{"
                        "    for (int x = -Iterations; x < Iterations; x++)"
                        "    {"
                        "        for (int y = -Iterations; y < Iterations; y++)"
                        "        {"
                        "            for (int z = -Iterations; z < Iterations; z++)"
                        "			 {"
                        "				 float2 Probe = Probe1.SampleLevel(State, D + float3(x, y, z) / Softness, 0).xy;"
                        "                C += step(L, Probe.x); B += Probe.y;"
                        "			 }"
                        "        }"
                        "    }"

                        "    C /= Recount; B /= Recount;"
                        "}"

                        "ShapeVertexOut VS(ShapeVertexIn Input)"
                        "{"
                        "    ShapeVertexOut Output = (ShapeVertexOut)0;"
                        "    Output.Position = mul(Input.Position, OwnWorldViewProjection);"
                        "    Output.TexCoord = Output.Position;"
                        "    return Output;"
                        "}"

                        "float4 PS(ShapeVertexOut Input) : SV_TARGET0"
                        "{"
                        "    Geometry F = LoadGeometrySV(InvViewProjection, Input.TexCoord);"
                        "    Material S = Materials[F.Material];"
                        "    float3 D = Position - F.Position;"
                        "    float3 K = normalize(D);"
                        "    float3 M = S.Metallic + F.Surface.x * S.Micrometal;"
                        "    float R = Pow4(S.Roughness + F.Surface.y * S.Microrough);"
                        "    float L = length(D);"
                        "    float A = 1.0f - L / Range;"
                        "    float3 V = normalize(ViewPosition.xyz - F.Position);"
                        "    float I = L / Distance - Bias, C = 0.0, B = 0.0;"

                        "    float3 E = CookTorrance(V, K, F.Normal, M, R, D) * A;"
                        "    [branch] if (Softness <= 0.0)"
                        "	 {"
                        "		 float2 Probe = Probe1.SampleLevel(State, -K, 0).xy;"
                        "        C = step(I, Probe.x); B = Probe.y;"
                        "	 }"
                        "    else"
                        "        SampleProbe(-K, I, C, B);"

                        "    E *= C + B * (1.0 - C);"
                        "    return float4(Lighting * E, A);"
                        "};";
            }
            const char* D3D11LightRenderer::GetSpotLightCode()
            {
                return
                        "#include ShapeVertexIn.h"
                        "#include ShapeVertexOut.h"
                        "#include ViewBuffer.h"
                        "#include Geometry.h"
                        "#include LoadGeometry.h"
                        "#include CookTorrance.h"

                        "cbuffer SpotLight : register(b3)"
                        "{"
                        "    matrix OwnWorldViewProjection;"
                        "    matrix OwnViewProjection;"
                        "    float3 Position;"
                        "    float Range;"
                        "    float3 Lighting;"
                        "    float Diffuse;"
                        "    float3 Shadow;"
                        "    float Iterations;"
                        "};"

                        "Texture2D Probe1 : register(t4);"

                        "float Pow4(float Value)"
                        "{"
                        "    return Value * Value * Value * Value;"
                        "}"

                        "ShapeVertexOut VS(ShapeVertexIn Input)"
                        "{"
                        "    ShapeVertexOut Output = (ShapeVertexOut)0;"
                        "    Output.Position = mul(Input.Position, OwnWorldViewProjection);"
                        "    Output.TexCoord = Output.Position;"
                        "    return Output;"
                        "}"

                        "float4 PS(ShapeVertexOut Input) : SV_TARGET0"
                        "{"
                        "    Geometry F = LoadGeometrySV(InvViewProjection, Input.TexCoord);"
                        "    float4 L = mul(float4(F.Position, 1), OwnViewProjection);"
                        "    float2 P = float2(L.x / L.w / 2.0f + 0.5f, 1 - (L.y / L.w / 2.0f + 0.5f));"
                        "    [branch] if (L.z <= 0 || saturate(P.x) != P.x || saturate(P.y) != P.y)"
                        "        return float4(0, 0, 0, 0);"

                        "    Material S = Materials[F.Material];"
                        "    float3 D = Position - F.Position;"
                        "    float3 A = 1.0f - length(D) / Range;"
                        "    [branch] if (Diffuse > 0)"
                        "        A *= Probe1.SampleLevel(State, P, 0).xyz;"

                        "    float3 V = normalize(ViewPosition.xyz - F.Position);"
                        "    float3 M = S.Metallic + F.Surface.x * S.Micrometal;"
                        "    float R = Pow4(S.Roughness + F.Surface.y * S.Microrough);"
                        "    float3 E = CookTorrance(V, normalize(D), F.Normal, M, R, D) * A;"

                        "    return float4(Lighting * E, length(A) / 3.0f);"
                        "};";
            }
            const char* D3D11LightRenderer::GetShadedSpotLightCode()
            {
                return
                        "#include ShapeVertexIn.h"
                        "#include ShapeVertexOut.h"
                        "#include ViewBuffer.h"
                        "#include Geometry.h"
                        "#include LoadGeometry.h"
                        "#include CookTorrance.h"

                        "cbuffer SpotLight : register(b3)"
                        "{"
                        "    matrix OwnWorldViewProjection;"
                        "    matrix OwnViewProjection;"
                        "    float3 Position;"
                        "    float Range;"
                        "    float3 Lighting;"
                        "    float Diffuse;"
                        "    float Softness;"
                        "    float Recount;"
                        "    float Bias;"
                        "    float Iterations;"
                        "};"

                        "Texture2D Probe1 : register(t4);"
                        "Texture2D Probe2 : register(t5);"

                        "float Pow4(float Value)"
                        "{"
                        "    return Value * Value * Value * Value;"
                        "}"

                        "void SampleProbe(float2 D, float L, out float C, out float B)"
                        "{"
                        "    for (int x = -Iterations; x < Iterations; x++)"
                        "    {"
                        "        for (int y = -Iterations; y < Iterations; y++)"
                        "        {"
                        "			 float2 Probe = Probe2.SampleLevel(State, D + float2(x, y) / Softness, 0).xy;"
                        "			 C += step(L, Probe.x); B += Probe.y;"
                        "        }"
                        "    }"

                        "    C /= Recount; B /= Recount;"
                        "}"

                        "ShapeVertexOut VS(ShapeVertexIn Input)"
                        "{"
                        "    ShapeVertexOut Output = (ShapeVertexOut)0;"
                        "    Output.Position = mul(Input.Position, OwnWorldViewProjection);"
                        "    Output.TexCoord = Output.Position;"
                        "    return Output;"
                        "}"

                        "float4 PS(ShapeVertexOut Input) : SV_TARGET0"
                        "{"
                        "    Geometry F = LoadGeometrySV(InvViewProjection, Input.TexCoord);"
                        "    float4 L = mul(float4(F.Position, 1), OwnViewProjection);"
                        "    float2 P = float2(L.x / L.w / 2.0f + 0.5f, 1 - (L.y / L.w / 2.0f + 0.5f));"
                        "    [branch] if (L.z <= 0 || saturate(P.x) != P.x || saturate(P.y) != P.y)"
                        "        return float4(0, 0, 0, 0);"

                        "    Material S = Materials[F.Material];"
                        "    float3 D = Position - F.Position;"
                        "    float3 A = 1.0 - length(D) / Range;"
                        "    [branch] if (Diffuse > 0)"
                        "        A *= Probe1.SampleLevel(State, P, 0).xyz;"

                        "    float3 M = S.Metallic + F.Surface.x * S.Micrometal;"
                        "    float R = Pow4(S.Roughness + F.Surface.y * S.Microrough);"
                        "    float3 V = normalize(ViewPosition.xyz - F.Position);"
                        "    float3 E = CookTorrance(V, normalize(D), F.Normal, M, R, D) * A;"
                        "    float I = L.z / L.w - Bias, C = 0.0, B = 0.0;"

                        "    [branch] if (Softness <= 0.0)"
                        "	 {"
                        "		 float2 Probe = Probe2.SampleLevel(State, P, 0).xy;"
                        "        C = step(I, Probe.x); B = Probe.y;"
                        "	 }"
                        "    else"
                        "        SampleProbe(P, I, C, B);"

                        "    E *= C + B * (1.0 - C);"
                        "    return float4(Lighting * E, length(A) / 3.0f);"
                        "};";
            }
            const char* D3D11LightRenderer::GetLineLightCode()
            {
                return
                        "#include ShapeVertexIn.h"
                        "#include ShapeVertexOut.h"
                        "#include ViewBuffer.h"
                        "#include Geometry.h"
                        "#include LoadGeometry.h"
                        "#include CookTorrance.h"

                        "cbuffer LineLight : register(b3)"
                        "{"
                        "    matrix OwnViewProjection;"
                        "    float3 Position;"
                        "    float Padding1;"
                        "    float3 Lighting;"
                        "    float Padding2;"
                        "};"

                        "float Pow4(float Value)"
                        "{"
                        "    return Value * Value * Value * Value;"
                        "}"

                        "ShapeVertexOut VS(ShapeVertexIn Input)"
                        "{"
                        "    ShapeVertexOut Output = (ShapeVertexOut)0;"
                        "    Output.Position = Input.Position;"
                        "    Output.TexCoord = Output.Position;"
                        "    return Output;"
                        "}"

                        "float4 PS(ShapeVertexOut Input) : SV_TARGET0"
                        "{"
                        "    Geometry F = LoadGeometrySV(InvViewProjection, Input.TexCoord);"
                        "    Material S = Materials[F.Material];"
                        "    float3 D = Position;"
                        "    float3 M = S.Metallic + F.Surface.x * S.Micrometal;"
                        "    float R = Pow4(S.Roughness + F.Surface.y * S.Microrough);"
                        "    float3 V = normalize(ViewPosition.xyz - F.Position);"

                        "    return float4(Lighting * CookTorrance(V, D, F.Normal, M, R, D), 1.0f);"
                        "};";
            }
            const char* D3D11LightRenderer::GetShadedLineLightCode()
            {
                return
                        "#include ShapeVertexIn.h"
                        "#include ShapeVertexOut.h"
                        "#include ViewBuffer.h"
                        "#include Geometry.h"
                        "#include LoadGeometry.h"
                        "#include CookTorrance.h"

                        "cbuffer LineLight : register(b3)"
                        "{"
                        "    matrix OwnViewProjection;"
                        "    float3 Position;"
                        "    float ShadowDistance;"
                        "    float3 Lighting;"
                        "    float ShadowLength;"
                        "    float Softness;"
                        "    float Recount;"
                        "    float Bias;"
                        "    float Iterations;"
                        "};"

                        "Texture2D Probe1 : register(t4);"

                        "float Pow4(float Value)"
                        "{"
                        "    return Value * Value * Value * Value;"
                        "}"

                        "void SampleProbe(float2 D, float L, out float C, out float B)"
                        "{"
                        "    for (int x = -Iterations; x < Iterations; x++)"
                        "    {"
                        "        for (int y = -Iterations; y < Iterations; y++)"
                        "        {"
                        "			 float2 Probe = Probe1.SampleLevel(State, D + float2(x, y) / Softness, 0).xy;"
                        "			 C += step(L, Probe.x); B += Probe.y;"
                        "        }"
                        "    }"

                        "    C /= Recount; B /= Recount;"
                        "}"

                        "ShapeVertexOut VS(ShapeVertexIn Input)"
                        "{"
                        "    ShapeVertexOut Output = (ShapeVertexOut)0;"
                        "    Output.Position = Input.Position;"
                        "    Output.TexCoord = Output.Position;"
                        "    return Output;"
                        "}"

                        "float4 PS(ShapeVertexOut Input) : SV_TARGET0"
                        "{"
                        "    Geometry F = LoadGeometrySV(InvViewProjection, Input.TexCoord);"
                        "    Material S = Materials[F.Material];"
                        "    float4 L = mul(float4(F.Position, 1), OwnViewProjection);"
                        "    float2 P = float2(L.x / L.w / 2.0f + 0.5f, 1 - (L.y / L.w / 2.0f + 0.5f));"
                        "    float3 D = Position;"
                        "    float3 M = S.Metallic + F.Surface.x * S.Micrometal;"
                        "    float R = Pow4(S.Roughness + F.Surface.y * S.Microrough);"
                        "    float3 V = normalize(ViewPosition.xyz - F.Position);"
                        "    float3 E = CookTorrance(V, D, F.Normal, M, R, D);"

                        "    [branch] if (saturate(P.x) == P.x && saturate(P.y) == P.y)"
                        "    {"
                        "        float G = length(float2(ViewPosition.x - V.x, ViewPosition.z - V.z));"
                        "        float I = L.z / L.w - Bias, C = 0.0, B = 0.0;"

                        "        [branch] if (Softness <= 0.0)"
                        "		 {"
                        "			 float2 Probe = Probe1.SampleLevel(State, P, 0).xy;"
                        "			 C = step(I, Probe.x); B = Probe.y;"
                        "		 }"
                        "        else"
                        "			 SampleProbe(P, I, C, B);"

                        "        C = saturate(C + saturate(pow(abs(G / ShadowDistance), ShadowLength)));"
                        "        E *= C + B * (1.0f - C);"
                        "    }"

                        "    return float4(Lighting * E, 1.0f);"
                        "};";
            }
            const char* D3D11LightRenderer::GetAmbientLightCode()
            {
                return
                        "#include ShapeVertexIn.h"
                        "#include ShapeVertexOut.h"
                        "#include ViewBuffer.h"
                        "#include Geometry.h"
                        "#include HemiAmbient.h"

                        "Texture2D Input4 : register(t4);"

                        "cbuffer AmbientLight : register(b3)"
                        "{"
                        "    float3 HighEmission;"
                        "    float Padding1;"
                        "    float3 LowEmission;"
                        "    float Padding2;"
                        "};"

                        "ShapeVertexOut VS(ShapeVertexIn Input)"
                        "{"
                        "    ShapeVertexOut Output = (ShapeVertexOut)0;"
                        "    Output.Position = Input.Position;"
                        "    Output.TexCoord.xy = Input.TexCoord;"
                        "    return Output;"
                        "}"

                        "float4 PS(ShapeVertexOut Input) : SV_TARGET0"
                        "{"
                        "    float4 Diffusion = Input1.Sample(State, Input.TexCoord.xy);"
                        "    float4 Emission = Input4.Sample(State, Input.TexCoord.xy);"
                        "    float4 Normal = Input2.Sample(State, Input.TexCoord.xy);"
                        "    float3 Ambient = HemiAmbient(HighEmission, LowEmission, Normal.y);"

                        "    return float4(Diffusion.xyz * (Emission.xyz + Ambient) + Emission.xyz * Emission.a, 1.0f);"
                        "};";
            }

            D3D11ImageRenderer::D3D11ImageRenderer(Engine::RenderSystem* Lab) : Engine::Renderers::ImageRenderer(Lab), Offset(0)
            {
                Device = System->GetDevice()->As<D3D11Device>();
                QuadVertex = System->VertexQuad()->As<D3D11ElementBuffer>();
                Stride = sizeof(Compute::ShapeVertex);
            }
            D3D11ImageRenderer::D3D11ImageRenderer(Engine::RenderSystem* Lab, Graphics::RenderTarget2D* Target) : Engine::Renderers::ImageRenderer(Lab, Target), Offset(0)
            {
                Device = System->GetDevice()->As<D3D11Device>();
                QuadVertex = System->VertexQuad()->As<D3D11ElementBuffer>();
                Stride = sizeof(Compute::ShapeVertex);
            }
            D3D11ImageRenderer::~D3D11ImageRenderer()
            {
            }
            void D3D11ImageRenderer::OnRasterization(Rest::Timer* Time)
            {
                if (!System->GetScene()->GetSurface()->GetTarget(0)->As<D3D11Texture2D>()->Resource)
                    return;

                if (RenderTarget != nullptr)
                    RenderTarget->Apply(Device);
                else
                    Device->GetRenderTarget()->Apply(Device);

                Device->Render.Diffusion = 1.0f;
                Device->Render.WorldViewProjection.Identify();

                Device->SetBlendState(Graphics::RenderLab_Blend_Additive);
                Device->SetDepthStencilState(Graphics::RenderLab_DepthStencil_None);
                Device->ImmediateContext->IASetInputLayout(Device->GetBasicEffect()->As<D3D11Shader>()->VertexLayout);
                Device->ImmediateContext->IASetVertexBuffers(0, 1, &QuadVertex->Element, &Stride, &Offset);
                Device->ImmediateContext->VSSetShader(Device->GetBasicEffect()->As<D3D11Shader>()->VertexShader, nullptr, 0);
                Device->ImmediateContext->PSSetShader(Device->GetBasicEffect()->As<D3D11Shader>()->PixelShader, nullptr, 0);
                Device->ImmediateContext->PSSetShaderResources(0, 1, &System->GetScene()->GetSurface()->GetTarget(0)->As<D3D11Texture2D>()->Resource);
                Device->ImmediateContext->UpdateSubresource(Device->ConstantBuffer[Graphics::RenderBufferType_Render], 0, nullptr, &Device->Render, 0, 0);
                Device->ImmediateContext->Draw(6, 0);

                ID3D11ShaderResourceView* Empty = nullptr;
                Device->ImmediateContext->PSSetShaderResources(0, 1, &Empty);
            }

            D3D11ReflectionsRenderer::D3D11ReflectionsRenderer(Engine::RenderSystem* Lab) : Engine::Renderers::ReflectionsRenderer(Lab), Offset(0)
            {
                QuadVertex = System->VertexQuad()->As<D3D11ElementBuffer>();
                Stride = sizeof(Compute::ShapeVertex);

                Graphics::Shader::Desc Desc = Graphics::Shader::Desc();
                Desc.Data = GetShaderCode();
                Desc.Layout = Graphics::Shader::GetShapeVertexLayout();
                Desc.LayoutSize = 2;

                Shader = (D3D11Shader*)Graphics::Shader::Create(System->GetDevice(), Desc);
                Shader->ConstantData = &Render;

                Device = System->GetDevice()->As<D3D11Device>();
                Device->CreateConstantBuffer(sizeof(Render), Shader->ConstantBuffer);
            }
            D3D11ReflectionsRenderer::~D3D11ReflectionsRenderer()
            {
				delete Shader;
				delete Output;
            }
            void D3D11ReflectionsRenderer::OnInitialize()
            {
                OnResizeBuffers();
            }
            void D3D11ReflectionsRenderer::OnRasterization(Rest::Timer* Time)
            {
                ID3D11ShaderResourceView* Nullable[3] = { nullptr };
                FLOAT Color[4] = { 0.0f };

                Render.ViewProjection = System->GetScene()->View.ViewProjection;
                Device->SetDepthStencilState(Graphics::RenderLab_DepthStencil_None);
                Device->ImmediateContext->IASetInputLayout(Shader->VertexLayout);
                Device->ImmediateContext->IASetVertexBuffers(0, 1, &QuadVertex->Element, &Stride, &Offset);
                Device->ImmediateContext->OMSetRenderTargets(1, &Output->RenderTargetView, Output->DepthStencilView);
                Device->ImmediateContext->ClearRenderTargetView(Output->RenderTargetView, Color);
                Device->ImmediateContext->PSSetShaderResources(1, 1, &System->GetScene()->GetSurface()->GetTarget(1)->As<D3D11Texture2D>()->Resource);
                Device->ImmediateContext->PSSetShaderResources(2, 1, &System->GetScene()->GetSurface()->GetTarget(2)->As<D3D11Texture2D>()->Resource);
                Device->ImmediateContext->PSSetShaderResources(3, 1, &System->GetScene()->GetSurface()->GetTarget(0)->As<D3D11Texture2D>()->Resource);
                Device->ImmediateContext->PSSetConstantBuffers(3, 1, &Shader->ConstantBuffer);
                Device->ImmediateContext->VSSetConstantBuffers(3, 1, &Shader->ConstantBuffer);
                Device->ImmediateContext->VSSetShader(Shader->VertexShader, nullptr, 0);
                Device->ImmediateContext->PSSetShader(Shader->PixelShader, nullptr, 0);
                Device->ImmediateContext->UpdateSubresource(Shader->ConstantBuffer, 0, nullptr, Shader->ConstantData, 0, 0);
                Device->ImmediateContext->Draw(6, 0);
                Device->ImmediateContext->PSSetShaderResources(1, 3, Nullable);
                Device->ImmediateContext->CopyResource(System->GetScene()->GetSurface()->As<D3D11MultiRenderTarget2D>()->Texture[0], Output->Texture);
            }
            void D3D11ReflectionsRenderer::OnResizeBuffers()
            {
                Graphics::RenderTarget2D::Desc IOutput = Graphics::RenderTarget2D::Desc();
                IOutput.FormatMode = Graphics::Format_R8G8B8A8_Unorm;
                IOutput.MiscFlags = Graphics::ResourceMisc_Generate_Mips;
                IOutput.Width = (unsigned int)System->GetScene()->GetSurface()->GetWidth();
                IOutput.Height = (unsigned int)System->GetScene()->GetSurface()->GetHeight();
                IOutput.MipLevels = Device->GetMipLevelCount(IOutput.Width, IOutput.Height);

				delete Output;
                Output = (D3D11RenderTarget2D*)Graphics::RenderTarget2D::Create(System->GetDevice(), IOutput);
                ReleaseCom(Output->DepthStencilView);
            }
            const char* D3D11ReflectionsRenderer::GetShaderCode()
            {
                return
                        "#include ShapeVertexIn.h"
                        "#include ShapeVertexOut.h"
                        "#include ViewBuffer.h"
                        "#include Geometry.h"
                        "#include LoadGeometry.h"
                        "#include LoadPosition.h"
                        "#include CookTorrance.h"

                        "cbuffer RenderConstant : register(b3)"
                        "{"
                        "    matrix ViewProjection;"
                        "    float IterationCount;"
                        "    float RayCorrection;"
                        "    float RayLength;"
                        "    float MipLevels;"
                        "}"

                        "float Pow4(float Value)"
                        "{"
                        "    return Value * Value * Value * Value;"
                        "}"

                        "ShapeVertexOut VS(ShapeVertexIn Input)"
                        "{"
                        "    ShapeVertexOut Output = (ShapeVertexOut)0;"
                        "    Output.Position = Input.Position;"
                        "    Output.TexCoord = Output.Position;"
                        "    return Output;"
                        "}"

                        "float4 PS(ShapeVertexOut Input) : SV_TARGET0"
                        "{"
                        "    Geometry F = LoadGeometrySV(InvViewProjection, Input.TexCoord);"
                        "    Material S = Materials[F.Material];"
                        "    float4 Offset = float4(0, 0, 0, 0);"
                        "    float3 Eye = normalize(F.Position - ViewPosition.xyz);"
                        "    float3 Ray = reflect(Eye, F.Normal), Sample;"
                        "    float3 Angle = pow(abs(1 - dot(-Eye, F.Normal)), 2);"
                        "    float3 M = S.Metallic + F.Surface.x * S.Micrometal;"
                        "    float R = Pow4(S.Roughness + F.Surface.y * S.Microrough);"
                        "    float Depth = RayLength;"

                        "    [branch] if (S.Reflection <= 0)"
                        "        return float4(F.Diffuse, 1.0f);"

                        "    [loop] for (int i = 0; i < IterationCount; i++)"
                        "    {"
                        "        Offset = mul(float4(F.Position + Ray * Depth, 1.0f), ViewProjection);"
                        "        Offset.xy = float2(0.5f, 0.5f) + float2(0.5f, -0.5f) * Offset.xy / Offset.w;"

                        "		 Depth = Input3.SampleLevel(State, Offset.xy, 0).r;"
                        "		 [branch] if (Depth < RayCorrection)"
                        "			 return float4(F.Diffuse, 1.0f);"

                        "		 Sample = LoadPosition(F.TexCoord, Input3.SampleLevel(State, Offset.xy, 0).r, InvViewProjection);"
                        "		 Depth = length(F.Position - Sample);"
                        "    }"

                        "    Ray = Input1.SampleLevel(State, Offset.xy, MipLevels * R).xyz;"
                        "    return float4(F.Diffuse + Ray * Angle * M * S.Reflection, 1.0f);"
                        "};";
            }

            D3D11DepthOfFieldRenderer::D3D11DepthOfFieldRenderer(Engine::RenderSystem* Lab) : Engine::Renderers::DepthOfFieldRenderer(Lab), Offset(0)
            {
                QuadVertex = System->VertexQuad()->As<D3D11ElementBuffer>();
                Stride = sizeof(Compute::ShapeVertex);

                Graphics::Shader::Desc Desc = Graphics::Shader::Desc();
                Desc.Data = GetShaderCode();
                Desc.Layout = Graphics::Shader::GetShapeVertexLayout();
                Desc.LayoutSize = 2;

                Shader = (D3D11Shader*)Graphics::Shader::Create(System->GetDevice(), Desc);
                Shader->ConstantData = &Render;

                Device = System->GetDevice()->As<D3D11Device>();
                Device->CreateConstantBuffer(sizeof(Render), Shader->ConstantBuffer);
            }
            D3D11DepthOfFieldRenderer::~D3D11DepthOfFieldRenderer()
            {
				delete Shader;
				delete Output;
            }
            void D3D11DepthOfFieldRenderer::OnInitialize()
            {
                OnResizeBuffers();
            }
            void D3D11DepthOfFieldRenderer::OnRasterization(Rest::Timer* Time)
            {
                ID3D11ShaderResourceView* Nullable[3] = { nullptr };
                FLOAT Color[4] = { 0.0f };

                if (AutoViewport)
                {
                    VerticalResolution = Output->GetHeight();
                    HorizontalResolution = Output->GetWidth();
                }

                Render.Texel[0] = 1.0f / HorizontalResolution;
                Render.Texel[1] = 1.0f / VerticalResolution;
                Render.ViewProjection = System->GetScene()->View.ViewProjection;

                Device->SetDepthStencilState(Graphics::RenderLab_DepthStencil_None);
                Device->ImmediateContext->IASetInputLayout(Shader->VertexLayout);
                Device->ImmediateContext->IASetVertexBuffers(0, 1, &QuadVertex->Element, &Stride, &Offset);
                Device->ImmediateContext->OMSetRenderTargets(1, &Output->RenderTargetView, Output->DepthStencilView);
                Device->ImmediateContext->ClearRenderTargetView(Output->RenderTargetView, Color);
                Device->ImmediateContext->PSSetShaderResources(1, 1, &System->GetScene()->GetSurface()->GetTarget(1)->As<D3D11Texture2D>()->Resource);
                Device->ImmediateContext->PSSetShaderResources(2, 1, &System->GetScene()->GetSurface()->GetTarget(2)->As<D3D11Texture2D>()->Resource);
                Device->ImmediateContext->PSSetShaderResources(3, 1, &System->GetScene()->GetSurface()->GetTarget(0)->As<D3D11Texture2D>()->Resource);
                Device->ImmediateContext->PSSetConstantBuffers(3, 1, &Shader->ConstantBuffer);
                Device->ImmediateContext->VSSetConstantBuffers(3, 1, &Shader->ConstantBuffer);
                Device->ImmediateContext->VSSetShader(Shader->VertexShader, nullptr, 0);
                Device->ImmediateContext->PSSetShader(Shader->PixelShader, nullptr, 0);
                Device->ImmediateContext->UpdateSubresource(Shader->ConstantBuffer, 0, nullptr, Shader->ConstantData, 0, 0);
                Device->ImmediateContext->Draw(6, 0);
                Device->ImmediateContext->PSSetShaderResources(1, 3, Nullable);
                Device->ImmediateContext->CopyResource(System->GetScene()->GetSurface()->As<D3D11MultiRenderTarget2D>()->Texture[0], Output->Texture);
            }
            void D3D11DepthOfFieldRenderer::OnResizeBuffers()
            {
                Graphics::RenderTarget2D::Desc IOutput = Graphics::RenderTarget2D::Desc();
                IOutput.FormatMode = Graphics::Format_R8G8B8A8_Unorm;
                IOutput.MiscFlags = Graphics::ResourceMisc_Generate_Mips;
                IOutput.Width = (unsigned int)System->GetScene()->GetSurface()->GetWidth();
                IOutput.Height = (unsigned int)System->GetScene()->GetSurface()->GetHeight();
                IOutput.MipLevels = Device->GetMipLevelCount(IOutput.Width, IOutput.Height);

				delete Output;
                Output = (D3D11RenderTarget2D*)Graphics::RenderTarget2D::Create(System->GetDevice(), IOutput);
                ReleaseCom(Output->DepthStencilView);
            }
            const char* D3D11DepthOfFieldRenderer::GetShaderCode()
            {
                return
                        "#include ShapeVertexIn.h"
                        "#include ShapeVertexOut.h"
                        "#include ViewBuffer.h"
                        "#include Geometry.h"
                        "#include LoadGeometry.h"
                        "#include LoadPosition.h"
                        "#include LoadTexCoord.h"

                        "cbuffer RenderConstant : register(b3)"
                        "{"
                        "    matrix ViewProjection;"
                        "    float2 Texel;"
                        "    float Threshold;"
                        "    float Gain;"
                        "    float Fringe;"
                        "    float Bias;"
                        "    float Dither;"
                        "    float Samples;"
                        "    float Rings;"
                        "    float FarDistance;"
                        "    float FarRange;"
                        "    float NearDistance;"
                        "    float NearRange;"
                        "    float FocalDepth;"
                        "    float Intensity;"
                        "    float Circular;"
                        "}"

                        "float3 Aberate(float2 TexCoord, float Weight)"
                        "{"
                        "    float3 Coefficient = float3(0.299, 0.587, 0.114);"
                        "    float3 Aberation = float3(Input1.SampleLevel(State, TexCoord + float2(0.0, 1.0) * Texel * Fringe * Weight, 0).r,"
                        "        Input1.SampleLevel(State, TexCoord + float2(-0.866, -0.5) * Texel * Fringe * Weight, 0).g,"
                        "        Input1.SampleLevel(State, TexCoord + float2(0.866, -0.5) * Texel * Fringe * Weight, 0).b);"
                        "    float Luminance = dot(Aberation * Intensity, Coefficient);"

                        "    return Aberation + lerp(0, Aberation, max((Luminance - Threshold) * Gain, 0.0) * Weight);"
                        "}"

                        "float2 Random(float2 TexCoord)"
                        "{"
                        "    float NoiseX = saturate(frac(sin(dot(TexCoord, float2(12.9898f, 78.233f))) * 43758.5453f)) * 2.0f - 1.0f;"
                        "    float NoiseY = saturate(frac(sin(dot(TexCoord, float2(12.9898f, 78.233f) * 2.0f)) * 43758.5453f)) * 2.0f - 1.0f;"
                        "    return float2(NoiseX, NoiseY);"
                        "}"

                        "float Weight(float2 TexCoord)"
                        "{"
                        "    float4 Position = float4(LoadPosition(TexCoord, Input3.SampleLevel(State, TexCoord, 0).r, InvViewProjection), 1);"
                        "    Position = mul(Position, ViewProjection);"

                        "    float Alpha = 1.0f - (Position.z - NearDistance) / NearRange;"
                        "    float Gamma = (Position.z - (FarDistance - FarRange)) / FarRange;"
                        "    return saturate(max(Alpha, Gamma)) * FocalDepth;"
                        "}"

                        "ShapeVertexOut VS(ShapeVertexIn Input)"
                        "{"
                        "    ShapeVertexOut Output = (ShapeVertexOut)0;"
                        "    Output.Position = Input.Position;"
                        "    Output.TexCoord = Output.Position;"
                        "    return Output;"
                        "}"

                        "float4 PS(ShapeVertexOut Input) : SV_TARGET0"
                        "{"
                        "    float2 TexCoord = LoadTexCoord(Input.TexCoord);"
                        "    float Amount = Weight(TexCoord);"
                        "    float3 Color = Input1.Sample(State, TexCoord).rgb;"
                        "    float2 Noise = Random(TexCoord) * Dither * Amount;"
                        "    float Width = Texel.x * Amount + Noise.x;"
                        "    float Height = Texel.y * Amount + Noise.y;"
                        "    float SampleAmount = 1.0;"

                        "    [loop] for (int i = 1; i <= Rings; i++)"
                        "    {"
                        "        float Count = i * Samples;"
                        "        [loop] for (int j = 0; j < Count; j++)"
                        "        {"
                        "            float Step = 6.28318530f / Count;"
                        "            float2 Fragment = TexCoord + float2(cos(j * Step) * i * Width, sin(j * Step) * i * Height);"

                        "            [branch] if (Weight(Fragment) >= Amount)"
                        "            {"
                        "                Color += Aberate(Fragment, Amount) * lerp(1.0f, (float)i / Rings, Bias) * Circular;"
                        "                SampleAmount += lerp(1.0f, (float)i / Rings, Bias);"
                        "            }"
                        "        }"
                        "    }"

                        "    Color /= SampleAmount;"
                        "    return float4(Color, 1);"
                        "};";
            }

            D3D11EmissionRenderer::D3D11EmissionRenderer(Engine::RenderSystem* Lab) : Engine::Renderers::EmissionRenderer(Lab), Offset(0)
            {
                QuadVertex = System->VertexQuad()->As<D3D11ElementBuffer>();
                Stride = sizeof(Compute::ShapeVertex);

                Graphics::Shader::Desc Desc = Graphics::Shader::Desc();
                Desc.Data = GetShaderCode();
                Desc.Layout = Graphics::Shader::GetShapeVertexLayout();
                Desc.LayoutSize = 2;

                Shader = (D3D11Shader*)Graphics::Shader::Create(System->GetDevice(), Desc);
                Shader->ConstantData = &Render;

                Device = System->GetDevice()->As<D3D11Device>();
                Device->CreateConstantBuffer(sizeof(Render), Shader->ConstantBuffer);
            }
            D3D11EmissionRenderer::~D3D11EmissionRenderer()
            {
				delete Shader;
				delete Output;
            }
            void D3D11EmissionRenderer::OnInitialize()
            {
                OnResizeBuffers();
            }
            void D3D11EmissionRenderer::OnRasterization(Rest::Timer* Time)
            {
                ID3D11ShaderResourceView* Nullable[3] = { nullptr };
                FLOAT Color[4] = { 0.0f };

                Render.SampleCount = 4.0f * Render.Samples * Render.Samples;
                if (AutoViewport)
                {
                    Render.Texel[0] = Output->GetWidth();
                    Render.Texel[1] = Output->GetHeight();
                }

                Device->SetDepthStencilState(Graphics::RenderLab_DepthStencil_None);
                Device->ImmediateContext->IASetInputLayout(Shader->VertexLayout);
                Device->ImmediateContext->IASetVertexBuffers(0, 1, &QuadVertex->Element, &Stride, &Offset);
                Device->ImmediateContext->OMSetRenderTargets(1, &Output->RenderTargetView, Output->DepthStencilView);
                Device->ImmediateContext->ClearRenderTargetView(Output->RenderTargetView, Color);
                Device->ImmediateContext->PSSetShaderResources(1, 1, &System->GetScene()->GetSurface()->GetTarget(1)->As<D3D11Texture2D>()->Resource);
                Device->ImmediateContext->PSSetShaderResources(2, 1, &System->GetScene()->GetSurface()->GetTarget(2)->As<D3D11Texture2D>()->Resource);
                Device->ImmediateContext->PSSetShaderResources(3, 1, &System->GetScene()->GetSurface()->GetTarget(0)->As<D3D11Texture2D>()->Resource);
                Device->ImmediateContext->PSSetConstantBuffers(3, 1, &Shader->ConstantBuffer);
                Device->ImmediateContext->VSSetConstantBuffers(3, 1, &Shader->ConstantBuffer);
                Device->ImmediateContext->VSSetShader(Shader->VertexShader, nullptr, 0);
                Device->ImmediateContext->PSSetShader(Shader->PixelShader, nullptr, 0);
                Device->ImmediateContext->UpdateSubresource(Shader->ConstantBuffer, 0, nullptr, Shader->ConstantData, 0, 0);
                Device->ImmediateContext->Draw(6, 0);
                Device->ImmediateContext->PSSetShaderResources(1, 3, Nullable);
                Device->ImmediateContext->CopyResource(System->GetScene()->GetSurface()->As<D3D11MultiRenderTarget2D>()->Texture[0], Output->Texture);
            }
            void D3D11EmissionRenderer::OnResizeBuffers()
            {
                Graphics::RenderTarget2D::Desc IOutput = Graphics::RenderTarget2D::Desc();
                IOutput.FormatMode = Graphics::Format_R8G8B8A8_Unorm;
                IOutput.MiscFlags = Graphics::ResourceMisc_Generate_Mips;
                IOutput.Width = (unsigned int)System->GetScene()->GetSurface()->GetWidth();
                IOutput.Height = (unsigned int)System->GetScene()->GetSurface()->GetHeight();
                IOutput.MipLevels = Device->GetMipLevelCount(IOutput.Width, IOutput.Height);

				delete Output;
                Output = (D3D11RenderTarget2D*)Graphics::RenderTarget2D::Create(System->GetDevice(), IOutput);
                ReleaseCom(Output->DepthStencilView);
            }
            const char* D3D11EmissionRenderer::GetShaderCode()
            {
                return
                        "#pragma warning(disable: 4000)\n"
                        "#include ShapeVertexIn.h"
                        "#include ShapeVertexOut.h"
                        "#include ViewBuffer.h"
                        "#include Geometry.h"
                        "#include LoadGeometry.h"

                        "cbuffer RenderConstant : register(b3)"
                        "{"
                        "    float2 Texel;"
                        "    float Intensity;"
                        "    float Threshold;"
                        "    float2 Scaling;"
                        "    float Samples;"
                        "    float SampleCount;"
                        "}"

                        "ShapeVertexOut VS(ShapeVertexIn Input)"
                        "{"
                        "    ShapeVertexOut Output = (ShapeVertexOut)0;"
                        "    Output.Position = Input.Position;"
                        "    Output.TexCoord.xy = Input.TexCoord;"
                        "    return Output;"
                        "}"

                        "float4 PS(ShapeVertexOut Input) : SV_TARGET0"
                        "{"
                        "	 Geometry F; Material S; float3 B = 0.0;"
                        "    [loop] for (int x = -Samples; x < Samples; x++)"
                        "    {"
                        "        [loop] for (int y = -Samples; y < Samples; y++)"
                        "        {"
                        "            float2 Offset = float2(x, y) / (Texel * Scaling);"
                        "			 F = LoadGeometry(InvViewProjection, Input.TexCoord.xy + Offset);"
                        "			 S = Materials[F.Material];"

                        "            B += saturate(F.Diffuse + S.Emission * S.Intensity - Threshold) * Intensity;"
                        "        }"
                        "    }"

                        "    return float4(F.Diffuse + B / SampleCount, 1.0f);"
                        "};";
            }

            D3D11GlitchRenderer::D3D11GlitchRenderer(Engine::RenderSystem* Lab) : Engine::Renderers::GlitchRenderer(Lab), Offset(0)
            {
                QuadVertex = System->VertexQuad()->As<D3D11ElementBuffer>();
                Stride = sizeof(Compute::ShapeVertex);

                Graphics::Shader::Desc Desc = Graphics::Shader::Desc();
                Desc.Data = GetShaderCode();
                Desc.Layout = Graphics::Shader::GetShapeVertexLayout();
                Desc.LayoutSize = 2;

                Shader = (D3D11Shader*)Graphics::Shader::Create(System->GetDevice(), Desc);
                Shader->ConstantData = &Render;

                Device = System->GetDevice()->As<D3D11Device>();
                Device->CreateConstantBuffer(sizeof(Render), Shader->ConstantBuffer);
            }
            D3D11GlitchRenderer::~D3D11GlitchRenderer()
            {
				delete Shader;
				delete Output;
            }
            void D3D11GlitchRenderer::OnInitialize()
            {
                OnResizeBuffers();
            }
            void D3D11GlitchRenderer::OnRasterization(Rest::Timer* Time)
            {
                ID3D11ShaderResourceView* Nullable[3] = { nullptr };
                FLOAT Color[4] = { 0.0f };

                if (Render.ElapsedTime >= 32000.0f)
                    Render.ElapsedTime = 0.0f;

                Render.ElapsedTime += (float)Time->GetDeltaTime() * 10.0f;
                Render.VerticalJumpAmount = VerticalJump;
                Render.VerticalJumpTime += (float)Time->GetDeltaTime() * VerticalJump * 11.3f;
                Render.ScanLineJitterThreshold = Compute::Math<float>::Saturate(1.0f - ScanLineJitter * 1.2f);
                Render.ScanLineJitterDisplacement = 0.002f + Compute::Math<float>::Pow(ScanLineJitter, 3) * 0.05f;
                Render.HorizontalShake = HorizontalShake * 0.2f;
                Render.ColorDriftAmount = ColorDrift * 0.04f;
                Render.ColorDriftTime = Render.ElapsedTime * 606.11f;

                Device->SetDepthStencilState(Graphics::RenderLab_DepthStencil_None);
                Device->ImmediateContext->IASetInputLayout(Shader->VertexLayout);
                Device->ImmediateContext->IASetVertexBuffers(0, 1, &QuadVertex->Element, &Stride, &Offset);
                Device->ImmediateContext->OMSetRenderTargets(1, &Output->RenderTargetView, Output->DepthStencilView);
                Device->ImmediateContext->ClearRenderTargetView(Output->RenderTargetView, Color);
                Device->ImmediateContext->PSSetShaderResources(1, 1, &System->GetScene()->GetSurface()->GetTarget(1)->As<D3D11Texture2D>()->Resource);
                Device->ImmediateContext->PSSetShaderResources(2, 1, &System->GetScene()->GetSurface()->GetTarget(2)->As<D3D11Texture2D>()->Resource);
                Device->ImmediateContext->PSSetShaderResources(3, 1, &System->GetScene()->GetSurface()->GetTarget(0)->As<D3D11Texture2D>()->Resource);
                Device->ImmediateContext->PSSetConstantBuffers(3, 1, &Shader->ConstantBuffer);
                Device->ImmediateContext->VSSetConstantBuffers(3, 1, &Shader->ConstantBuffer);
                Device->ImmediateContext->VSSetShader(Shader->VertexShader, nullptr, 0);
                Device->ImmediateContext->PSSetShader(Shader->PixelShader, nullptr, 0);
                Device->ImmediateContext->UpdateSubresource(Shader->ConstantBuffer, 0, nullptr, Shader->ConstantData, 0, 0);
                Device->ImmediateContext->Draw(6, 0);
                Device->ImmediateContext->PSSetShaderResources(1, 3, Nullable);
                Device->ImmediateContext->CopyResource(System->GetScene()->GetSurface()->As<D3D11MultiRenderTarget2D>()->Texture[0], Output->Texture);
            }
            void D3D11GlitchRenderer::OnResizeBuffers()
            {
                Graphics::RenderTarget2D::Desc IOutput = Graphics::RenderTarget2D::Desc();
                IOutput.FormatMode = Graphics::Format_R8G8B8A8_Unorm;
                IOutput.MiscFlags = Graphics::ResourceMisc_Generate_Mips;
                IOutput.Width = (unsigned int)System->GetScene()->GetSurface()->GetWidth();
                IOutput.Height = (unsigned int)System->GetScene()->GetSurface()->GetHeight();
                IOutput.MipLevels = Device->GetMipLevelCount(IOutput.Width, IOutput.Height);

				delete Output;
                Output = (D3D11RenderTarget2D*)Graphics::RenderTarget2D::Create(System->GetDevice(), IOutput);
                ReleaseCom(Output->DepthStencilView);
            }
            const char* D3D11GlitchRenderer::GetShaderCode()
            {
                return
                        "#include ShapeVertexIn.h"
                        "#include ShapeVertexOut.h"
                        "#include Geometry.h"
                        "#include RandomFloat2.h"

                        "cbuffer RenderConstant : register(b3)"
                        "{"
                        "    float ScanLineJitterDisplacement;"
                        "    float ScanLineJitterThreshold;"
                        "    float VerticalJumpAmount;"
                        "    float VerticalJumpTime;"
                        "    float ColorDriftAmount;"
                        "    float ColorDriftTime;"
                        "    float HorizontalShake;"
                        "    float ElapsedTime;"
                        "}"

                        "ShapeVertexOut VS(ShapeVertexIn Input)"
                        "{"
                        "    ShapeVertexOut Output = (ShapeVertexOut)0;"
                        "    Output.Position = Input.Position;"
                        "    Output.TexCoord.xy = Input.TexCoord;"
                        "    return Output;"
                        "}"

                        "float4 PS(ShapeVertexOut Input) : SV_TARGET0"
                        "{"
                        "    float Jitter = RandomFloat2(Input.TexCoord.y, ElapsedTime) * 2.0f - 1.0f;"
                        "    Jitter *= step(ScanLineJitterThreshold, abs(Jitter)) * ScanLineJitterDisplacement;"
                        "    float Jump = lerp(Input.TexCoord.y, frac(Input.TexCoord.y + VerticalJumpTime), VerticalJumpAmount);"
                        "    float Shake = (RandomFloat2(ElapsedTime, 2) - 0.5) * HorizontalShake;"
                        "    float Drift = sin(Jump + ColorDriftTime) * ColorDriftAmount;"
                        "    float4 Alpha = Input1.Sample(State, frac(float2(Input.TexCoord.x + Jitter + Shake, Jump)));"
                        "    float4 Beta = Input1.Sample(State, frac(float2(Input.TexCoord.x + Jitter + Shake + Drift, Jump)));"

                        "    return float4(Alpha.r, Beta.g, Alpha.b, 1);"
                        "};";
            }

            D3D11AmbientOcclusionRenderer::D3D11AmbientOcclusionRenderer(Engine::RenderSystem* Lab) : Engine::Renderers::AmbientOcclusionRenderer(Lab), Offset(0)
            {
                QuadVertex = System->VertexQuad()->As<D3D11ElementBuffer>();
                Stride = sizeof(Compute::ShapeVertex);

                Graphics::Shader::Desc Desc = Graphics::Shader::Desc();
                Desc.Data = GetShaderCode();
                Desc.Layout = Graphics::Shader::GetShapeVertexLayout();
                Desc.LayoutSize = 2;

                Shader = (D3D11Shader*)Graphics::Shader::Create(System->GetDevice(), Desc);
                Shader->ConstantData = &Render;

                Device = System->GetDevice()->As<D3D11Device>();
                Device->CreateConstantBuffer(sizeof(Render), Shader->ConstantBuffer);
            }
            D3D11AmbientOcclusionRenderer::~D3D11AmbientOcclusionRenderer()
            {
				delete Shader;
				delete Output;
            }
            void D3D11AmbientOcclusionRenderer::OnInitialize()
            {
                OnResizeBuffers();
            }
            void D3D11AmbientOcclusionRenderer::OnRasterization(Rest::Timer* Time)
            {
                ID3D11ShaderResourceView* Nullable[3] = { nullptr };
                FLOAT Color[4] = { 0.0f };

                Render.SampleCount = 4.0f * Render.Samples * Render.Samples;

                Device->SetDepthStencilState(Graphics::RenderLab_DepthStencil_None);
                Device->ImmediateContext->IASetInputLayout(Shader->VertexLayout);
                Device->ImmediateContext->IASetVertexBuffers(0, 1, &QuadVertex->Element, &Stride, &Offset);
                Device->ImmediateContext->OMSetRenderTargets(1, &Output->RenderTargetView, Output->DepthStencilView);
                Device->ImmediateContext->ClearRenderTargetView(Output->RenderTargetView, Color);
                Device->ImmediateContext->PSSetShaderResources(1, 1, &System->GetScene()->GetSurface()->GetTarget(1)->As<D3D11Texture2D>()->Resource);
                Device->ImmediateContext->PSSetShaderResources(2, 1, &System->GetScene()->GetSurface()->GetTarget(2)->As<D3D11Texture2D>()->Resource);
                Device->ImmediateContext->PSSetShaderResources(3, 1, &System->GetScene()->GetSurface()->GetTarget(0)->As<D3D11Texture2D>()->Resource);
                Device->ImmediateContext->PSSetConstantBuffers(3, 1, &Shader->ConstantBuffer);
                Device->ImmediateContext->VSSetConstantBuffers(3, 1, &Shader->ConstantBuffer);
                Device->ImmediateContext->VSSetShader(Shader->VertexShader, nullptr, 0);
                Device->ImmediateContext->PSSetShader(Shader->PixelShader, nullptr, 0);
                Device->ImmediateContext->UpdateSubresource(Shader->ConstantBuffer, 0, nullptr, Shader->ConstantData, 0, 0);
                Device->ImmediateContext->Draw(6, 0);
                Device->ImmediateContext->PSSetShaderResources(1, 3, Nullable);
                Device->ImmediateContext->CopyResource(System->GetScene()->GetSurface()->As<D3D11MultiRenderTarget2D>()->Texture[0], Output->Texture);
            }
            void D3D11AmbientOcclusionRenderer::OnResizeBuffers()
            {
                Graphics::RenderTarget2D::Desc IOutput = Graphics::RenderTarget2D::Desc();
                IOutput.FormatMode = Graphics::Format_R8G8B8A8_Unorm;
                IOutput.MiscFlags = Graphics::ResourceMisc_Generate_Mips;
                IOutput.Width = (unsigned int)System->GetScene()->GetSurface()->GetWidth();
                IOutput.Height = (unsigned int)System->GetScene()->GetSurface()->GetHeight();
                IOutput.MipLevels = Device->GetMipLevelCount(IOutput.Width, IOutput.Height);

				delete Output;
                Output = (D3D11RenderTarget2D*)Graphics::RenderTarget2D::Create(System->GetDevice(), IOutput);
                ReleaseCom(Output->DepthStencilView);
            }
            const char* D3D11AmbientOcclusionRenderer::GetShaderCode()
            {
                return
                        "#pragma warning(disable: 4000)\n"
                        "#include ShapeVertexIn.h"
                        "#include ShapeVertexOut.h"
                        "#include Geometry.h"
                        "#include ViewBuffer.h"
                        "#include LoadPosition.h"
                        "#include LoadTexCoord.h"

                        "cbuffer RenderConstant : register(b3)"
                        "{"
                        "    float Scale;"
                        "    float Intensity;"
                        "    float Bias;"
                        "    float Radius;"
                        "    float Step;"
                        "    float Offset;"
                        "    float Distance;"
                        "    float Fading;"
                        "    float Power;"
                        "    float Samples;"
                        "    float SampleCount;"
                        "    float Padding;"
                        "}"

                        "float2 MakeRay(float2 TexCoord)"
                        "{"
                        "    float RayX = saturate(frac(sin(dot(TexCoord, float2(12.9898f, 78.233f))) * 43758.5453f)) * 2.0f - 1.0f;"
                        "    float RayY = saturate(frac(sin(dot(TexCoord, float2(12.9898f, 78.233f) * 2.0f)) * 43758.5453f)) * 2.0f - 1.0f;"
                        "    return float2(RayX, RayY);"
                        "}"

                        "float Occlude(float2 TexCoord, float3 Position, float3 Normal)"
                        "{"
                        "    float3 Direction = LoadPosition(TexCoord, Input3.SampleLevel(State, TexCoord, 0).r, InvViewProjection) - Position;"
                        "    float Length = length(Direction) * Scale;"

                        "    return max(0.0, dot(normalize(Direction), Normal) * Power - Bias) * Intensity / (1.0f + Length * Length);"
                        "}"

                        "ShapeVertexOut VS(ShapeVertexIn Input)"
                        "{"
                        "    ShapeVertexOut Output = (ShapeVertexOut)0;"
                        "    Output.Position = Input.Position;"
                        "    Output.TexCoord = Output.Position;"
                        "    return Output;"
                        "}"

                        "float4 PS(ShapeVertexOut Input) : SV_TARGET0"
                        "{"
                        "    float2 TexCoord = LoadTexCoord(Input.TexCoord);"
                        "    float4 Normal = Input2.SampleLevel(State, TexCoord, 0);"
                        "    float3 Position = LoadPosition(TexCoord, Input3.SampleLevel(State, TexCoord, 0).r, InvViewProjection);"
                        "    float Fade = saturate(pow(abs(distance(ViewPosition.xyz, Position) / Distance), Fading));"
                        "    float Factor = 0.0f;"

                        "    Material S = Materials[Normal.w];"
                        "    float AmbientRadius = Radius + S.Radius;"

                        "    [loop] for (int x = -Samples; x < Samples; x++)"
                        "    {"
                        "        [loop] for (int y = -Samples; y < Samples; y++)"
                        "        {"
                        "            float2 Ray = MakeRay(TexCoord) * Step - float2(y, x) * Offset;"
                        "            float2 Direction = reflect(Ray, float2(x, y)) * AmbientRadius;"
                        "            Factor += S.Occlusion * (Occlude(TexCoord + Direction, Position, Normal.xyz) + Occlude(TexCoord - Direction, Position, Normal.xyz));"
                        "        }"
                        "    }"

                        "    return float4(Input1.Sample(State, TexCoord).xyz * (1.0f - Factor * Fade / SampleCount), 1);"
                        "};";
            }

            D3D11IndirectOcclusionRenderer::D3D11IndirectOcclusionRenderer(Engine::RenderSystem* Lab) : Engine::Renderers::IndirectOcclusionRenderer(Lab), Offset(0)
            {
                QuadVertex = System->VertexQuad()->As<D3D11ElementBuffer>();
                Stride = sizeof(Compute::ShapeVertex);

                Graphics::Shader::Desc Desc = Graphics::Shader::Desc();
                Desc.Data = GetShaderCode();
                Desc.Layout = Graphics::Shader::GetShapeVertexLayout();
                Desc.LayoutSize = 2;

                Shader = (D3D11Shader*)Graphics::Shader::Create(System->GetDevice(), Desc);
                Shader->ConstantData = &Render;

                Device = System->GetDevice()->As<D3D11Device>();
                Device->CreateConstantBuffer(sizeof(Render), Shader->ConstantBuffer);
            }
            D3D11IndirectOcclusionRenderer::~D3D11IndirectOcclusionRenderer()
            {
				delete Shader;
				delete Output;
            }
            void D3D11IndirectOcclusionRenderer::OnInitialize()
            {
                OnResizeBuffers();
            }
            void D3D11IndirectOcclusionRenderer::OnRasterization(Rest::Timer* Time)
            {
                ID3D11ShaderResourceView* Nullable[3] = { nullptr };
                FLOAT Color[4] = { 0.0f };

                Render.SampleCount = 4.0f * Render.Samples * Render.Samples;

                Device->SetDepthStencilState(Graphics::RenderLab_DepthStencil_None);
                Device->ImmediateContext->IASetInputLayout(Shader->VertexLayout);
                Device->ImmediateContext->IASetVertexBuffers(0, 1, &QuadVertex->Element, &Stride, &Offset);
                Device->ImmediateContext->OMSetRenderTargets(1, &Output->RenderTargetView, Output->DepthStencilView);
                Device->ImmediateContext->ClearRenderTargetView(Output->RenderTargetView, Color);
                Device->ImmediateContext->PSSetShaderResources(1, 1, &System->GetScene()->GetSurface()->GetTarget(1)->As<D3D11Texture2D>()->Resource);
                Device->ImmediateContext->PSSetShaderResources(2, 1, &System->GetScene()->GetSurface()->GetTarget(2)->As<D3D11Texture2D>()->Resource);
                Device->ImmediateContext->PSSetShaderResources(3, 1, &System->GetScene()->GetSurface()->GetTarget(0)->As<D3D11Texture2D>()->Resource);
                Device->ImmediateContext->PSSetConstantBuffers(3, 1, &Shader->ConstantBuffer);
                Device->ImmediateContext->VSSetConstantBuffers(3, 1, &Shader->ConstantBuffer);
                Device->ImmediateContext->VSSetShader(Shader->VertexShader, nullptr, 0);
                Device->ImmediateContext->PSSetShader(Shader->PixelShader, nullptr, 0);
                Device->ImmediateContext->UpdateSubresource(Shader->ConstantBuffer, 0, nullptr, Shader->ConstantData, 0, 0);
                Device->ImmediateContext->Draw(6, 0);
                Device->ImmediateContext->PSSetShaderResources(1, 3, Nullable);
                Device->ImmediateContext->CopyResource(System->GetScene()->GetSurface()->As<D3D11MultiRenderTarget2D>()->Texture[0], Output->Texture);
            }
            void D3D11IndirectOcclusionRenderer::OnResizeBuffers()
            {
                Graphics::RenderTarget2D::Desc IOutput = Graphics::RenderTarget2D::Desc();
                IOutput.FormatMode = Graphics::Format_R8G8B8A8_Unorm;
                IOutput.MiscFlags = Graphics::ResourceMisc_Generate_Mips;
                IOutput.Width = (unsigned int)System->GetScene()->GetSurface()->GetWidth();
                IOutput.Height = (unsigned int)System->GetScene()->GetSurface()->GetHeight();
                IOutput.MipLevels = Device->GetMipLevelCount(IOutput.Width, IOutput.Height);

				delete Output;
                Output = (D3D11RenderTarget2D*)Graphics::RenderTarget2D::Create(System->GetDevice(), IOutput);
                ReleaseCom(Output->DepthStencilView);
            }
            const char* D3D11IndirectOcclusionRenderer::GetShaderCode()
            {
                return
                        "#pragma warning(disable: 4000)\n"
                        "#include ShapeVertexIn.h"
                        "#include ShapeVertexOut.h"
                        "#include Geometry.h"
                        "#include ViewBuffer.h"
                        "#include LoadPosition.h"
                        "#include LoadTexCoord.h"

                        "cbuffer RenderConstant : register(b3)"
                        "{"
                        "    float Scale;"
                        "    float Intensity;"
                        "    float Bias;"
                        "    float Radius;"
                        "    float Step;"
                        "    float Offset;"
                        "    float Distance;"
                        "    float Fading;"
                        "    float Power;"
                        "    float Samples;"
                        "    float SampleCount;"
                        "    float Padding;"
                        "}"

                        "float2 MakeRay(float2 TexCoord)"
                        "{"
                        "    float RayX = saturate(frac(sin(dot(TexCoord, float2(12.9898f, 78.233f))) * 43758.5453f)) * 2.0f - 1.0f;"
                        "    float RayY = saturate(frac(sin(dot(TexCoord, float2(12.9898f, 78.233f) * 2.0f)) * 43758.5453f)) * 2.0f - 1.0f;"
                        "    return float2(RayX, RayY);"
                        "}"

                        "float3 Occlude(float2 TexCoord, float3 Position, float3 Normal)"
                        "{"
                        "    float3 Direction = LoadPosition(TexCoord, Input3.SampleLevel(State, TexCoord, 0).r, InvViewProjection) - Position;"
                        "    float Length = length(Direction) * Scale;"
                        "    float Occlusion = max(0.0, dot(normalize(Direction), Normal) * Power - Bias) * Intensity / (1.0f + Length * Length);"

                        "    return Input1.Sample(State, TexCoord).xyz * Occlusion;"
                        "}"

                        "ShapeVertexOut VS(ShapeVertexIn Input)"
                        "{"
                        "    ShapeVertexOut Output = (ShapeVertexOut)0;"
                        "    Output.Position = Input.Position;"
                        "    Output.TexCoord = Output.Position;"
                        "    return Output;"
                        "}"

                        "float4 PS(ShapeVertexOut Input) : SV_TARGET0"
                        "{"
                        "    float2 TexCoord = LoadTexCoord(Input.TexCoord);"
                        "    float4 Normal = Input2.SampleLevel(State, TexCoord, 0);"
                        "    float3 Position = LoadPosition(TexCoord, Input3.SampleLevel(State, TexCoord, 0).r, InvViewProjection);"
                        "    float Fade = saturate(pow(abs(distance(ViewPosition.xyz, Position) / Distance), Fading));"
                        "    float3 Factor = 0.0f;"

                        "    Material S = Materials[Normal.w];"
                        "    float AmbientRadius = Radius + S.Radius;"

                        "    [loop] for (int x = -Samples; x < Samples; x++)"
                        "    {"
                        "        [loop] for (int y = -Samples; y < Samples; y++)"
                        "        {"
                        "            float2 Ray = MakeRay(TexCoord) * Step - float2(y, x) * Offset;"
                        "            float2 Direction = reflect(Ray, float2(x, y)) * AmbientRadius;"
                        "            Factor += S.Occlusion * (Occlude(TexCoord + Direction, Position, Normal.xyz) + Occlude(TexCoord - Direction, Position, Normal.xyz));"
                        "        }"
                        "    }"

                        "    return float4(Input1.Sample(State, TexCoord).xyz * (1 + Factor * Fade / SampleCount), 1);"
                        "};";
            }

            D3D11ToneRenderer::D3D11ToneRenderer(Engine::RenderSystem* Lab) : Engine::Renderers::ToneRenderer(Lab), Offset(0)
            {
                QuadVertex = System->VertexQuad()->As<D3D11ElementBuffer>();
                Stride = sizeof(Compute::ShapeVertex);

                Graphics::Shader::Desc Desc = Graphics::Shader::Desc();
                Desc.Data = GetShaderCode();
                Desc.Layout = Graphics::Shader::GetShapeVertexLayout();
                Desc.LayoutSize = 2;

                Shader = (D3D11Shader*)Graphics::Shader::Create(System->GetDevice(), Desc);
                Shader->ConstantData = &Render;

                Device = System->GetDevice()->As<D3D11Device>();
                Device->CreateConstantBuffer(sizeof(Render), Shader->ConstantBuffer);
            }
            D3D11ToneRenderer::~D3D11ToneRenderer()
            {
				delete Shader;
				delete Output;
            }
            void D3D11ToneRenderer::OnInitialize()
            {
                OnResizeBuffers();
            }
            void D3D11ToneRenderer::OnRasterization(Rest::Timer* Time)
            {
                ID3D11ShaderResourceView* Nullable[3] = { nullptr };
                FLOAT Color[4] = { 0.0f };

                Device->SetDepthStencilState(Graphics::RenderLab_DepthStencil_None);
                Device->ImmediateContext->IASetInputLayout(Shader->VertexLayout);
                Device->ImmediateContext->IASetVertexBuffers(0, 1, &QuadVertex->Element, &Stride, &Offset);
                Device->ImmediateContext->OMSetRenderTargets(1, &Output->RenderTargetView, Output->DepthStencilView);
                Device->ImmediateContext->ClearRenderTargetView(Output->RenderTargetView, Color);
                Device->ImmediateContext->PSSetShaderResources(1, 1, &System->GetScene()->GetSurface()->GetTarget(1)->As<D3D11Texture2D>()->Resource);
                Device->ImmediateContext->PSSetShaderResources(2, 1, &System->GetScene()->GetSurface()->GetTarget(2)->As<D3D11Texture2D>()->Resource);
                Device->ImmediateContext->PSSetShaderResources(3, 1, &System->GetScene()->GetSurface()->GetTarget(0)->As<D3D11Texture2D>()->Resource);
                Device->ImmediateContext->PSSetConstantBuffers(3, 1, &Shader->ConstantBuffer);
                Device->ImmediateContext->VSSetConstantBuffers(3, 1, &Shader->ConstantBuffer);
                Device->ImmediateContext->VSSetShader(Shader->VertexShader, nullptr, 0);
                Device->ImmediateContext->PSSetShader(Shader->PixelShader, nullptr, 0);
                Device->ImmediateContext->UpdateSubresource(Shader->ConstantBuffer, 0, nullptr, Shader->ConstantData, 0, 0);
                Device->ImmediateContext->Draw(6, 0);
                Device->ImmediateContext->PSSetShaderResources(1, 3, Nullable);
                Device->ImmediateContext->CopyResource(System->GetScene()->GetSurface()->As<D3D11MultiRenderTarget2D>()->Texture[0], Output->Texture);
            }
            void D3D11ToneRenderer::OnResizeBuffers()
            {
                Graphics::RenderTarget2D::Desc IOutput = Graphics::RenderTarget2D::Desc();
                IOutput.FormatMode = Graphics::Format_R8G8B8A8_Unorm;
                IOutput.MiscFlags = Graphics::ResourceMisc_Generate_Mips;
                IOutput.Width = (unsigned int)System->GetScene()->GetSurface()->GetWidth();
                IOutput.Height = (unsigned int)System->GetScene()->GetSurface()->GetHeight();
                IOutput.MipLevels = Device->GetMipLevelCount(IOutput.Width, IOutput.Height);

				delete Output;
                Output = (D3D11RenderTarget2D*)Graphics::RenderTarget2D::Create(System->GetDevice(), IOutput);
                ReleaseCom(Output->DepthStencilView);
            }
            const char* D3D11ToneRenderer::GetShaderCode()
            {
                return
                        "#include ShapeVertexIn.h"
                        "#include ShapeVertexOut.h"
                        "#include Geometry.h"

                        "cbuffer RenderConstant : register(b3)"
                        "{"
                        "    float3 BlindVisionR;"
                        "    float VignetteAmount;"
                        "    float3 BlindVisionG;"
                        "    float VignetteCurve;"
                        "    float3 BlindVisionB;"
                        "    float VignetteRadius;"
                        "    float3 VignetteColor;"
                        "    float LinearIntensity;"
                        "    float3 ColorGamma;"
                        "    float GammaIntensity;"
                        "    float3 DesaturationGamma;"
                        "    float DesaturationIntensity;"
                        "}"

                        "float3 GetToneMapping(float3 Pixel)"
                        "{"
                        "    float3 Color = Pixel / (Pixel + LinearIntensity) * GammaIntensity;"
                        "    return float3(dot(Color, BlindVisionR.xyz), dot(Color, BlindVisionG.xyz), dot(Color, BlindVisionB.xyz)) * ColorGamma;"
                        "}"

                        "float3 GetDesaturation(float3 Pixel)"
                        "{"
                        "    float3 Color = Pixel;"
                        "    float Intensity = DesaturationGamma.x * Color.r + DesaturationGamma.y * Color.b + DesaturationGamma.z * Color.g;"
                        "    Intensity *= DesaturationIntensity;"

                        "    Color.r = Intensity + Color.r * (1 - DesaturationIntensity);"
                        "    Color.g = Intensity + Color.g * (1 - DesaturationIntensity);"
                        "    Color.b = Intensity + Color.b * (1 - DesaturationIntensity);"

                        "    return Color;"
                        "}"

                        "ShapeVertexOut VS(ShapeVertexIn Input)"
                        "{"
                        "    ShapeVertexOut Output = (ShapeVertexOut)0;"
                        "    Output.Position = Input.Position;"
                        "    Output.TexCoord.xy = Input.TexCoord;"
                        "    return Output;"
                        "}"

                        "float4 PS(ShapeVertexOut Input) : SV_TARGET0"
                        "{"
                        "    float3 Color = Input1.Sample(State, Input.TexCoord.xy).xyz;"
                        "    float Vignette = saturate(distance((Input.TexCoord.xy + 0.5f) / VignetteRadius - 0.5f, -float2(0.5f, 0.5f)));"
                        "    Color = lerp(Color, VignetteColor.xyz, pow(Vignette, VignetteCurve) * VignetteAmount);"
                        "    Color = GetDesaturation(Color);"

                        "    return float4(GetToneMapping(Color), 1);"
                        "};";
            }

            D3D11GUIRenderer::D3D11GUIRenderer(Engine::RenderSystem* Lab, Graphics::Activity* NewWindow) : GUIRenderer(Lab, NewWindow)
            {
                VertexBuffer = nullptr;
                IndexBuffer = nullptr;
                VertexShaderBlob = nullptr;
                VertexShader = nullptr;
                InputLayout = nullptr;
                VertexConstantBuffer = nullptr;
                PixelShaderBlob = nullptr;
                PixelShader = nullptr;
                FontSamplerState = nullptr;
                FontTextureView = nullptr;
                RasterizerState = nullptr;
                BlendState = nullptr;
                DepthStencilState = nullptr;
                VertexBufferSize = 5000;
                IndexBufferSize = 10000;
                Context = (void*)ImGui::CreateContext();
                ImGuiIO& Input = ImGui::GetIO();
                Input.RenderDrawListsFn = (void(*)(ImDrawData*))DrawList;
                Reset(); Activate();

                const char* VertexShaderCode = GetVertexShaderCode();
                const char* PixelShaderCode = GetPixelShaderCode();

                D3D11Device* Device = Lab->GetDevice()->As<D3D11Device>();
                D3DCompile(VertexShaderCode, strlen(VertexShaderCode), nullptr, nullptr, nullptr, "VS", Device->GetVSProfile(), 0, 0, &VertexShaderBlob, nullptr);
                if (VertexShaderBlob == nullptr)
                {
                    THAWK_ERROR("couldn't compile vertex shader");
                    return;
                }

                if (Device->D3DDevice->CreateVertexShader((DWORD*)VertexShaderBlob->GetBufferPointer(), VertexShaderBlob->GetBufferSize(), nullptr, &VertexShader) != S_OK)
                {
                    THAWK_ERROR("couldn't create vertex shader");
                    return;
                }

                D3D11_INPUT_ELEMENT_DESC Layout[] =
                {
                    { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, (size_t)(&((ImDrawVert*)0)->pos), D3D11_INPUT_PER_VERTEX_DATA, 0 },
                    { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, (size_t)(&((ImDrawVert*)0)->uv), D3D11_INPUT_PER_VERTEX_DATA, 0 },
                    { "COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, (size_t)(&((ImDrawVert*)0)->col), D3D11_INPUT_PER_VERTEX_DATA, 0 },
                };

                if (Device->D3DDevice->CreateInputLayout(Layout, 3, VertexShaderBlob->GetBufferPointer(), VertexShaderBlob->GetBufferSize(), &InputLayout) != S_OK)
                {
                    THAWK_ERROR("couldn't create input layout");
                    return;
                }

                D3D11_BUFFER_DESC BufferDESC;
                BufferDESC.ByteWidth = sizeof(Compute::Matrix4x4);
                BufferDESC.Usage = D3D11_USAGE_DYNAMIC;
                BufferDESC.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
                BufferDESC.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
                BufferDESC.MiscFlags = 0;
                Device->D3DDevice->CreateBuffer(&BufferDESC, nullptr, &VertexConstantBuffer);

                D3DCompile(PixelShaderCode, strlen(PixelShaderCode), nullptr, nullptr, nullptr, "PS", Device->GetPSProfile(), 0, 0, &PixelShaderBlob, nullptr);
                if (PixelShaderBlob == nullptr)
                {
                    THAWK_ERROR("couldn't compile pixel shader");
                    return;
                }

                if (Device->D3DDevice->CreatePixelShader((DWORD*)PixelShaderBlob->GetBufferPointer(), PixelShaderBlob->GetBufferSize(), nullptr, &PixelShader) != S_OK)
                {
                    THAWK_ERROR("couldn't create pixel shader");
                    return;
                }

                D3D11_BLEND_DESC BlendDESC;
                ZeroMemory(&BlendDESC, sizeof(BlendDESC));
                BlendDESC.AlphaToCoverageEnable = false;
                BlendDESC.RenderTarget[0].BlendEnable = true;
                BlendDESC.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
                BlendDESC.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
                BlendDESC.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
                BlendDESC.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
                BlendDESC.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
                BlendDESC.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
                BlendDESC.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
                Device->D3DDevice->CreateBlendState(&BlendDESC, &BlendState);

                D3D11_RASTERIZER_DESC RasterDESC;
                ZeroMemory(&RasterDESC, sizeof(RasterDESC));
                RasterDESC.FillMode = D3D11_FILL_SOLID;
                RasterDESC.CullMode = D3D11_CULL_NONE;
                RasterDESC.ScissorEnable = true;
                RasterDESC.DepthClipEnable = true;
                Device->D3DDevice->CreateRasterizerState(&RasterDESC, &RasterizerState);

                D3D11_DEPTH_STENCIL_DESC DepthStencilDESC;
                ZeroMemory(&DepthStencilDESC, sizeof(DepthStencilDESC));
                DepthStencilDESC.DepthEnable = false;
                DepthStencilDESC.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
                DepthStencilDESC.DepthFunc = D3D11_COMPARISON_ALWAYS;
                DepthStencilDESC.StencilEnable = false;
                DepthStencilDESC.FrontFace.StencilFailOp = DepthStencilDESC.FrontFace.StencilDepthFailOp = DepthStencilDESC.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
                DepthStencilDESC.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
                DepthStencilDESC.BackFace = DepthStencilDESC.FrontFace;
                Device->D3DDevice->CreateDepthStencilState(&DepthStencilDESC, &DepthStencilState);

                unsigned char* Pixels = nullptr;
                int Width = 0, Height = 0;

                Input.Fonts->GetTexDataAsRGBA32(&Pixels, &Width, &Height);

                D3D11_TEXTURE2D_DESC Texture2DDESC;
                ZeroMemory(&Texture2DDESC, sizeof(Texture2DDESC));
                Texture2DDESC.Width = Width;
                Texture2DDESC.Height = Height;
                Texture2DDESC.MipLevels = 1;
                Texture2DDESC.ArraySize = 1;
                Texture2DDESC.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
                Texture2DDESC.SampleDesc.Count = 1;
                Texture2DDESC.Usage = D3D11_USAGE_DEFAULT;
                Texture2DDESC.BindFlags = D3D11_BIND_SHADER_RESOURCE;
                Texture2DDESC.CPUAccessFlags = 0;

                ID3D11Texture2D* Texture = nullptr;
                D3D11_SUBRESOURCE_DATA SubResource;
                SubResource.pSysMem = Pixels;
                SubResource.SysMemPitch = Texture2DDESC.Width * 4;
                SubResource.SysMemSlicePitch = 0;
                Device->As<D3D11Device>()->D3DDevice->CreateTexture2D(&Texture2DDESC, &SubResource, &Texture);

                D3D11_SHADER_RESOURCE_VIEW_DESC ShaderResourceViewDESC;
                ZeroMemory(&ShaderResourceViewDESC, sizeof(ShaderResourceViewDESC));
                ShaderResourceViewDESC.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
                ShaderResourceViewDESC.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
                ShaderResourceViewDESC.Texture2D.MipLevels = Texture2DDESC.MipLevels;
                ShaderResourceViewDESC.Texture2D.MostDetailedMip = 0;
                Device->As<D3D11Device>()->D3DDevice->CreateShaderResourceView(Texture, &ShaderResourceViewDESC, &FontTextureView);
                ReleaseCom(Texture);

                Input.Fonts->TexID = (void*)FontTextureView;

                D3D11_SAMPLER_DESC SamplerDESC;
                ZeroMemory(&SamplerDESC, sizeof(SamplerDESC));
                SamplerDESC.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
                SamplerDESC.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
                SamplerDESC.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
                SamplerDESC.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
                SamplerDESC.MipLODBias = 0.f;
                SamplerDESC.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
                SamplerDESC.MinLOD = 0.f;
                SamplerDESC.MaxLOD = 0.f;
                Device->As<D3D11Device>()->D3DDevice->CreateSamplerState(&SamplerDESC, &FontSamplerState);
            }
            D3D11GUIRenderer::~D3D11GUIRenderer()
            {
                ReleaseCom(FontSamplerState);
                ReleaseCom(FontTextureView);
                ReleaseCom(IndexBuffer);
                ReleaseCom(VertexBuffer);
                ReleaseCom(BlendState);
                ReleaseCom(DepthStencilState);
                ReleaseCom(RasterizerState);
                ReleaseCom(PixelShader);
                ReleaseCom(PixelShaderBlob);
                ReleaseCom(VertexConstantBuffer);
                ReleaseCom(InputLayout);
                ReleaseCom(VertexShader);
                ReleaseCom(VertexShaderBlob);

                Deactivate();
                ImGui::DestroyContext((ImGuiContext*)Context);
            }
            const char* D3D11GUIRenderer::GetVertexShaderCode()
            {
                return
                        "cbuffer VertexBuffer : register(b0)"
                        "{"
                        "   float4x4 WorldViewProjection;"
                        "};"
                        "struct VS_INPUT"
                        "{"
                        "   float2 Position : POSITION;"
                        "   float4 Color : COLOR0;"
                        "   float2 TexCoord : TEXCOORD0;"
                        "};"
                        "struct PS_INPUT"
                        "{"
                        "   float4 Position : SV_POSITION;"
                        "   float4 Color : COLOR0;"
                        "   float2 TexCoord : TEXCOORD0;"
                        "};"

                        "PS_INPUT VS(VS_INPUT Input)"
                        "{"
                        "   PS_INPUT Output;"
                        "   Output.Position = mul(WorldViewProjection, float4(Input.Position.xy, 0.f, 1.f));"
                        "   Output.Color = Input.Color;"
                        "   Output.TexCoord = Input.TexCoord;"
                        "   return Output;"
                        "}";
            }
            const char* D3D11GUIRenderer::GetPixelShaderCode()
            {
                return
                        "struct PS_INPUT"
                        "{"
                        "   float4 Position : SV_POSITION;"
                        "   float4 Color : COLOR0;"
                        "   float2 TexCoord : TEXCOORD0;"
                        "};"

                        "sampler State;"
                        "Texture2D Diffuse;"

                        "float4 PS(PS_INPUT Input) : SV_Target"
                        "{"
                        "   return Input.Color * Diffuse.Sample(State, Input.TexCoord);"
                        "}";
            }
            void D3D11GUIRenderer::DrawList(void* Context)
            {
                ImDrawData* Info = (ImDrawData*)Context;
                ImGuiIO* Settings = &ImGui::GetIO();
                if (!Settings->UserData)
                    return;

                D3D11GUIRenderer* RefLink = (D3D11GUIRenderer*)Settings->UserData;
                D3D11Device* Device = RefLink->GetRenderer()->GetDevice()->As<D3D11Device>();

                if (!RefLink->VertexBuffer || RefLink->VertexBufferSize < Info->TotalVtxCount)
                {
					ReleaseCom(RefLink->VertexBuffer);
                    RefLink->VertexBufferSize = Info->TotalVtxCount + 5000;

                    D3D11_BUFFER_DESC BufferDESC;
                    memset(&BufferDESC, 0, sizeof(D3D11_BUFFER_DESC));
                    BufferDESC.Usage = D3D11_USAGE_DYNAMIC;
                    BufferDESC.ByteWidth = RefLink->VertexBufferSize * sizeof(ImDrawVert);
                    BufferDESC.BindFlags = D3D11_BIND_VERTEX_BUFFER;
                    BufferDESC.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
                    BufferDESC.MiscFlags = 0;

                    if (Device->D3DDevice->CreateBuffer(&BufferDESC, nullptr, &RefLink->VertexBuffer) < 0)
                    {
                        THAWK_ERROR("couldn't create vertex buffer");
                        return;
                    }
                }

                if (!RefLink->IndexBuffer || RefLink->IndexBufferSize < Info->TotalIdxCount)
                {
					ReleaseCom(RefLink->IndexBuffer);
					RefLink->IndexBufferSize = Info->TotalIdxCount + 10000;

                    D3D11_BUFFER_DESC BufferDESC;
                    memset(&BufferDESC, 0, sizeof(D3D11_BUFFER_DESC));
                    BufferDESC.Usage = D3D11_USAGE_DYNAMIC;
                    BufferDESC.ByteWidth = RefLink->IndexBufferSize * sizeof(ImDrawIdx);
                    BufferDESC.BindFlags = D3D11_BIND_INDEX_BUFFER;
                    BufferDESC.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

                    if (Device->D3DDevice->CreateBuffer(&BufferDESC, nullptr, &RefLink->IndexBuffer) < 0)
                    {
                        THAWK_ERROR("couldn't create index buffer");
                        return;
                    }
                }

                D3D11_MAPPED_SUBRESOURCE VTXResource, IDXResource;
                if (Device->ImmediateContext->Map(RefLink->VertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &VTXResource) != S_OK)
                {
                    THAWK_ERROR("couldn't map vertex buffer");
                    return;
                }

                if (Device->ImmediateContext->Map(RefLink->IndexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &IDXResource) != S_OK)
                {
                    THAWK_ERROR("couldn't map index buffer");
                    return;
                }

                ImDrawVert* VTXInfo = (ImDrawVert*)VTXResource.pData;
                ImDrawIdx* IDXInfo = (ImDrawIdx*)IDXResource.pData;

                for (int n = 0; n < Info->CmdListsCount; n++)
                {
                    const ImDrawList* CommadList = Info->CmdLists[n];
                    memcpy(VTXInfo, CommadList->VtxBuffer.Data, CommadList->VtxBuffer.Size * sizeof(ImDrawVert));
                    memcpy(IDXInfo, CommadList->IdxBuffer.Data, CommadList->IdxBuffer.Size * sizeof(ImDrawIdx));
                    VTXInfo += CommadList->VtxBuffer.Size;
                    IDXInfo += CommadList->IdxBuffer.Size;
                }

                Device->ImmediateContext->Unmap(RefLink->VertexBuffer, 0);
                Device->ImmediateContext->Unmap(RefLink->IndexBuffer, 0);

                D3D11_MAPPED_SUBRESOURCE MappedResource;
                if (Device->ImmediateContext->Map(RefLink->VertexConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource) != S_OK)
                {
                    THAWK_ERROR("couldn't map vertex constant buffer");
                    return;
                }

                memcpy(MappedResource.pData, RefLink->WorldViewProjection.Row, sizeof(Compute::Matrix4x4));
                Device->ImmediateContext->Unmap(RefLink->VertexConstantBuffer, 0);

                DeviceState State;
                State.ScissorRectsCount = State.ViewportsCount = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
                Device->ImmediateContext->RSGetScissorRects(&State.ScissorRectsCount, State.ScissorRects);
                Device->ImmediateContext->RSGetViewports(&State.ViewportsCount, State.Viewports);
                Device->ImmediateContext->RSGetState(&State.RasterState);
                Device->ImmediateContext->OMGetBlendState(&State.BlendState, State.BlendFactor, &State.SampleMask);
                Device->ImmediateContext->OMGetDepthStencilState(&State.DepthStencilState, &State.StencilRef);
                Device->ImmediateContext->PSGetShaderResources(0, 1, &State.PSShaderResource);
                Device->ImmediateContext->PSGetSamplers(0, 1, &State.PSSampler);
                State.PSInstancesCount = State.VSInstancesCount = 256;
                Device->ImmediateContext->PSGetShader(&State.PS, State.PSInstances, &State.PSInstancesCount);
                Device->ImmediateContext->VSGetShader(&State.VS, State.VSInstances, &State.VSInstancesCount);
                Device->ImmediateContext->VSGetConstantBuffers(0, 1, &State.VSConstantBuffer);
                Device->ImmediateContext->IAGetPrimitiveTopology(&State.PrimitiveTopology);
                Device->ImmediateContext->IAGetIndexBuffer(&State.IndexBuffer, &State.IndexBufferFormat, &State.IndexBufferOffset);
                Device->ImmediateContext->IAGetVertexBuffers(0, 1, &State.VertexBuffer, &State.VertexBufferStride, &State.VertexBufferOffset);
                Device->ImmediateContext->IAGetInputLayout(&State.InputLayout);

                D3D11_VIEWPORT Viewport;
                memset(&Viewport, 0, sizeof(D3D11_VIEWPORT));
                Viewport.Width = Settings->DisplaySize.x;
                Viewport.Height = Settings->DisplaySize.y;
                Viewport.MinDepth = 0.0f;
                Viewport.MaxDepth = 1.0f;
                Viewport.TopLeftX = Viewport.TopLeftY = 0.0f;
                Device->ImmediateContext->RSSetViewports(1, &Viewport);

                unsigned int Stride = sizeof(ImDrawVert);
                unsigned int Offset = 0;

                Device->ImmediateContext->IASetInputLayout(RefLink->InputLayout);
                Device->ImmediateContext->IASetVertexBuffers(0, 1, &RefLink->VertexBuffer, &Stride, &Offset);
                Device->ImmediateContext->IASetIndexBuffer(RefLink->IndexBuffer, sizeof(ImDrawIdx) == 2 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT, 0);
                Device->ImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
                Device->ImmediateContext->VSSetShader(RefLink->VertexShader, nullptr, 0);
                Device->ImmediateContext->VSSetConstantBuffers(0, 1, &RefLink->VertexConstantBuffer);
                Device->ImmediateContext->PSSetShader(RefLink->PixelShader, nullptr, 0);
                Device->ImmediateContext->PSSetSamplers(0, 1, &RefLink->FontSamplerState);

                const float BlendFactor[4] = { 0.f, 0.f, 0.f, 0.f };
                Device->ImmediateContext->OMSetBlendState(RefLink->BlendState, BlendFactor, 0xffffffff);
                Device->ImmediateContext->OMSetDepthStencilState(RefLink->DepthStencilState, 0);
                Device->ImmediateContext->RSSetState(RefLink->RasterizerState);

                int VTXOffset = 0, IDXOffset = 0;
                for (int n = 0; n < Info->CmdListsCount; n++)
                {
                    const ImDrawList* CommandList = Info->CmdLists[n];
                    for (int i = 0; i < CommandList->CmdBuffer.Size; i++)
                    {
                        const ImDrawCmd* Command = &CommandList->CmdBuffer[i];

                        if (!Command->UserCallback)
                        {
                            RECT Rectangle = { (long)Command->ClipRect.x, (long)Command->ClipRect.y, (long)Command->ClipRect.z, (long)Command->ClipRect.w };
                            Device->ImmediateContext->PSSetShaderResources(0, 1, (ID3D11ShaderResourceView**)&Command->TextureId);
                            Device->ImmediateContext->RSSetScissorRects(1, &Rectangle);
                            Device->ImmediateContext->DrawIndexed(Command->ElemCount, IDXOffset, VTXOffset);
                        }
                        else
                            Command->UserCallback(CommandList, Command);

                        IDXOffset += Command->ElemCount;
                    }
                    VTXOffset += CommandList->VtxBuffer.Size;
                }

                Device->ImmediateContext->RSSetScissorRects(State.ScissorRectsCount, State.ScissorRects);
                Device->ImmediateContext->RSSetViewports(State.ViewportsCount, State.Viewports);
                Device->ImmediateContext->RSSetState(State.RasterState); if (State.RasterState) State.RasterState->Release();
                Device->ImmediateContext->OMSetBlendState(State.BlendState, State.BlendFactor, State.SampleMask); if (State.BlendState) State.BlendState->Release();
                Device->ImmediateContext->OMSetDepthStencilState(State.DepthStencilState, State.StencilRef); if (State.DepthStencilState) State.DepthStencilState->Release();
                Device->ImmediateContext->PSSetShaderResources(0, 1, &State.PSShaderResource); if (State.PSShaderResource) State.PSShaderResource->Release();
                Device->ImmediateContext->PSSetSamplers(0, 1, &State.PSSampler); if (State.PSSampler) State.PSSampler->Release();
                Device->ImmediateContext->PSSetShader(State.PS, State.PSInstances, State.PSInstancesCount); if (State.PS) State.PS->Release();
                for (unsigned int i = 0; i < State.PSInstancesCount; i++) if (State.PSInstances[i]) State.PSInstances[i]->Release();
                Device->ImmediateContext->VSSetShader(State.VS, State.VSInstances, State.VSInstancesCount); if (State.VS) State.VS->Release();
                Device->ImmediateContext->VSSetConstantBuffers(0, 1, &State.VSConstantBuffer); if (State.VSConstantBuffer) State.VSConstantBuffer->Release();
                for (unsigned int i = 0; i < State.VSInstancesCount; i++) if (State.VSInstances[i]) State.VSInstances[i]->Release();
                Device->ImmediateContext->IASetPrimitiveTopology(State.PrimitiveTopology);
                Device->ImmediateContext->IASetIndexBuffer(State.IndexBuffer, State.IndexBufferFormat, State.IndexBufferOffset); if (State.IndexBuffer) State.IndexBuffer->Release();
                Device->ImmediateContext->IASetVertexBuffers(0, 1, &State.VertexBuffer, &State.VertexBufferStride, &State.VertexBufferOffset); if (State.VertexBuffer) State.VertexBuffer->Release();
                Device->ImmediateContext->IASetInputLayout(State.InputLayout); if (State.InputLayout) State.InputLayout->Release();
            }
        }
    }
}
#endif