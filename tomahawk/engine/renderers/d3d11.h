#ifndef THAWK_ENGINE_RENDERERS_D3D11_H
#define THAWK_ENGINE_RENDERERS_D3D11_H

#include "../renderers.h"
#include "../../graphics/d3d11.h"
#ifdef THAWK_MICROSOFT

namespace Tomahawk
{
    namespace Graphics
    {
        namespace D3D11
        {
            class D3D11ModelRenderer : public Engine::Renderers::ModelRenderer
            {
            protected:
                struct
                {
                    D3D11Shader* Multi = nullptr;
                    D3D11Shader* Depth = nullptr;
                    D3D11Shader* CubicDepth = nullptr;
                } Shaders;

                struct
                {
                    Compute::Matrix4x4 SliceViewProjection[6];
                    Compute::Vector3 Position;
                    float Distance = 0.0f;
                } CubicDepth;

            private:
                Rest::Pool<Engine::Component*>* Models = nullptr;
                D3D11Device* Device = nullptr;

            public:
                D3D11ModelRenderer(Engine::RenderSystem* Lab);
                ~D3D11ModelRenderer();
                void OnInitialize();
                void OnRender(Rest::Timer* Time);
                void OnPhaseRender(Rest::Timer* Time);
                void OnDepthRender(Rest::Timer* Time);
                void OnCubicDepthRender(Rest::Timer* Time, Compute::Matrix4x4* ViewProjection);
                const char* GetMultiCode();
                const char* GetDepthCode();
                const char* GetCubicDepthCode();
            };

            class D3D11SkinnedModelRenderer : public Engine::Renderers::SkinnedModelRenderer
            {
            protected:
                struct
                {
                    D3D11Shader* Multi = nullptr;
                    D3D11Shader* Depth = nullptr;
                    D3D11Shader* CubicDepth = nullptr;
                } Shaders;

                struct
                {
                    Compute::Matrix4x4 SliceViewProjection[6];
                    Compute::Vector3 Position;
                    float Distance;
                } CubicDepth;

            private:
                Rest::Pool<Engine::Component*>* SkinnedModels = nullptr;
                D3D11Device* Device = nullptr;

            public:
                D3D11SkinnedModelRenderer(Engine::RenderSystem* Lab);
                ~D3D11SkinnedModelRenderer();
                void OnInitialize();
                void OnRender(Rest::Timer* Time);
                void OnPhaseRender(Rest::Timer* Time);
                void OnDepthRender(Rest::Timer* Time);
                void OnCubicDepthRender(Rest::Timer* Time, Compute::Matrix4x4* ViewProjection);
                const char* GetMultiCode();
                const char* GetDepthCode();
                const char* GetCubicDepthCode();
            };

            class D3D11ElementSystemRenderer : public Engine::Renderers::ElementSystemRenderer
            {
            protected:
                struct
                {
                    D3D11Shader* Multi = nullptr;
                    D3D11Shader* Depth = nullptr;
                    D3D11Shader* CubicDepthTriangle = nullptr;
                    D3D11Shader* CubicDepthPoint = nullptr;
                } Shaders;

                struct
                {
                    Compute::Matrix4x4 SliceView[6];
                    Compute::Matrix4x4 Projection;
                    Compute::Vector3 Position;
                    float Distance;
                } CubicDepth;

            private:
                Rest::Pool<Engine::Component*>* ElementSystems = nullptr;
                D3D11Device* Device = nullptr;
                D3D11_PRIMITIVE_TOPOLOGY LastTopology;

            public:
                D3D11ElementSystemRenderer(Engine::RenderSystem* Lab);
                ~D3D11ElementSystemRenderer();
                void OnInitialize();
                void OnRender(Rest::Timer* Time);
                void OnPhaseRender(Rest::Timer* Time);
                void OnDepthRender(Rest::Timer* Time);
                void OnCubicDepthRender(Rest::Timer* Time, Compute::Matrix4x4* ViewProjection);
                const char* GetMultiCode();
                const char* GetDepthCode();
                const char* GetCubicDepthPointCode();
                const char* GetCubicDepthTriangleCode();
            };

            class D3D11DepthRenderer : public Engine::Renderers::DepthRenderer
            {
            private:
                Rest::Pool<Engine::Component*>* PointLights = nullptr;
                Rest::Pool<Engine::Component*>* SpotLights = nullptr;
                Rest::Pool<Engine::Component*>* LineLights = nullptr;
                D3D11Device* Device = nullptr;

            public:
                D3D11DepthRenderer(Engine::RenderSystem* Lab);
                void OnInitialize();
                void OnIntervalRender(Rest::Timer* Time);
            };

            class D3D11ProbeRenderer : public Engine::Renderers::ProbeRenderer
            {
            private:
                Rest::Pool<Engine::Component*>* ProbeLights = nullptr;
                Graphics::MultiRenderTarget2D* RefSurface = nullptr;
                D3D11_SHADER_RESOURCE_VIEW_DESC ShaderResourceView;
                D3D11_TEXTURE2D_DESC CubeMapView;
                D3D11_TEXTURE2D_DESC TextureView;
                D3D11_BOX Region;
                D3D11Device* Device = nullptr;
                UInt64 Map;

            public:
                D3D11ProbeRenderer(Engine::RenderSystem* Lab);
                void OnInitialize();
                void OnRender(Rest::Timer* Time);
                void CreateRenderTarget();
                bool ResourceBound(Graphics::TextureCube** Cube);
            };

            class D3D11LightRenderer : public Engine::Renderers::LightRenderer
            {
            protected:
                struct
                {
                    D3D11Shader* ProbeLighting = nullptr;
                    D3D11Shader* PointLighting = nullptr;
                    D3D11Shader* ShadedPointLighting = nullptr;
                    D3D11Shader* SpotLighting = nullptr;
                    D3D11Shader* ShadedSpotLighting = nullptr;
                    D3D11Shader* LineLighting = nullptr;
                    D3D11Shader* ShadedLineLighting = nullptr;
                    D3D11Shader* AmbientLighting = nullptr;
                    D3D11Shader* ActiveLighting = nullptr;
                    ID3D11InputLayout* Layout = nullptr;
                } Shaders;

                struct
                {
                    D3D11ElementBuffer* Sphere[2] = { nullptr };
                    D3D11ElementBuffer* Quad = nullptr;
                } Models;

                struct
                {
                    float SpotLight = 1024;
                    float LineLight = 2048;
                } Quality;

            private:
                Rest::Pool<Engine::Component*>* PointLights = nullptr;
                Rest::Pool<Engine::Component*>* ProbeLights = nullptr;
                Rest::Pool<Engine::Component*>* SpotLights = nullptr;
                Rest::Pool<Engine::Component*>* LineLights = nullptr;
                D3D11RenderTarget2D* Output = nullptr;
                D3D11RenderTarget2D* PhaseOutput = nullptr;
                D3D11RenderTarget2D* Input = nullptr;
                D3D11RenderTarget2D* PhaseInput = nullptr;
                D3D11Device* Device = nullptr;

            public:
                D3D11LightRenderer(Engine::RenderSystem* Lab);
                ~D3D11LightRenderer();
                void OnInitialize();
                void OnRelease();
                void OnRender(Rest::Timer* Time);
                void OnPhaseRender(Rest::Timer* Time);
                void OnResizeBuffers();
                void CreatePointLighting();
                void CreateProbeLighting();
                void CreateSpotLighting();
                void CreateLineLighting();
                void CreateAmbientLighting();
                void CreateRenderTargets();
                const char* GetProbeLightCode();
                const char* GetPointLightCode();
                const char* GetShadedPointLightCode();
                const char* GetSpotLightCode();
                const char* GetShadedSpotLightCode();
                const char* GetLineLightCode();
                const char* GetShadedLineLightCode();
                const char* GetAmbientLightCode();
            };

            class D3D11ImageRenderer : public Engine::Renderers::ImageRenderer
            {
            private:
                D3D11Device* Device;
                D3D11ElementBuffer* QuadVertex;
                unsigned int Stride, Offset;

            public:
                D3D11ImageRenderer(Engine::RenderSystem* Lab);
                D3D11ImageRenderer(Engine::RenderSystem* Lab, Graphics::RenderTarget2D* Target);
                ~D3D11ImageRenderer();
                void OnRender(Rest::Timer* Time);
            };

            class D3D11ReflectionsRenderer : public Engine::Renderers::ReflectionsRenderer
            {
            private:
                D3D11Device* Device;
                D3D11ElementBuffer* QuadVertex;
                D3D11Shader* Shader;
                D3D11RenderTarget2D* Output;
                unsigned int Stride, Offset;

            public:
                D3D11ReflectionsRenderer(Engine::RenderSystem* Lab);
                ~D3D11ReflectionsRenderer();
                void OnInitialize();
                void OnRender(Rest::Timer* Time);
                void OnResizeBuffers();
                const char* GetShaderCode();
            };

            class D3D11DepthOfFieldRenderer : public Engine::Renderers::DepthOfFieldRenderer
            {
            private:
                D3D11Device* Device;
                D3D11ElementBuffer* QuadVertex;
                D3D11Shader* Shader;
                D3D11RenderTarget2D* Output;
                unsigned int Stride, Offset;

            public:
                D3D11DepthOfFieldRenderer(Engine::RenderSystem* Lab);
                ~D3D11DepthOfFieldRenderer();
                void OnInitialize();
                void OnRender(Rest::Timer* Time);
                void OnResizeBuffers();
                const char* GetShaderCode();
            };

            class D3D11EmissionRenderer : public Engine::Renderers::EmissionRenderer
            {
            private:
                D3D11Device* Device;
                D3D11ElementBuffer* QuadVertex;
                D3D11Shader* Shader;
                D3D11RenderTarget2D* Output;
                unsigned int Stride, Offset;

            public:
                D3D11EmissionRenderer(Engine::RenderSystem* Lab);
                ~D3D11EmissionRenderer();
                void OnInitialize();
                void OnRender(Rest::Timer* Time);
                void OnResizeBuffers();
                const char* GetShaderCode();
            };

            class D3D11GlitchRenderer : public Engine::Renderers::GlitchRenderer
            {
            private:
                D3D11Device* Device;
                D3D11ElementBuffer* QuadVertex;
                D3D11Shader* Shader;
                D3D11RenderTarget2D* Output;
                unsigned int Stride, Offset;

            public:
                D3D11GlitchRenderer(Engine::RenderSystem* Lab);
                ~D3D11GlitchRenderer();
                void OnInitialize();
                void OnRender(Rest::Timer* Time);
                void OnResizeBuffers();
                const char* GetShaderCode();
            };

            class D3D11AmbientOcclusionRenderer : public Engine::Renderers::AmbientOcclusionRenderer
            {
            private:
                D3D11Device * Device;
                D3D11ElementBuffer* QuadVertex;
                D3D11Shader* Shader;
                D3D11RenderTarget2D* Output;
                unsigned int Stride, Offset;

            public:
                D3D11AmbientOcclusionRenderer(Engine::RenderSystem* Lab);
                ~D3D11AmbientOcclusionRenderer();
                void OnInitialize();
                void OnRender(Rest::Timer* Time);
                void OnResizeBuffers();
                const char* GetShaderCode();
            };

            class D3D11IndirectOcclusionRenderer : public Engine::Renderers::IndirectOcclusionRenderer
            {
            private:
                D3D11Device* Device;
                D3D11ElementBuffer* QuadVertex;
                D3D11Shader* Shader;
                D3D11RenderTarget2D* Output;
                unsigned int Stride, Offset;

            public:
                D3D11IndirectOcclusionRenderer(Engine::RenderSystem* Lab);
                ~D3D11IndirectOcclusionRenderer();
                void OnInitialize();
                void OnRender(Rest::Timer* Time);
                void OnResizeBuffers();
                const char* GetShaderCode();
            };

            class D3D11ToneRenderer : public Engine::Renderers::ToneRenderer
            {
            private:
                D3D11Device* Device;
                D3D11ElementBuffer* QuadVertex;
                D3D11Shader* Shader;
                D3D11RenderTarget2D* Output;
                unsigned int Stride, Offset;

            public:
                D3D11ToneRenderer(Engine::RenderSystem* Lab);
                ~D3D11ToneRenderer();
                void OnInitialize();
                void OnRender(Rest::Timer* Time);
                void OnResizeBuffers();
                const char* GetShaderCode();
            };

            class D3D11GUIRenderer : public Engine::Renderers::GUIRenderer
            {
            private:
                struct DeviceState
                {
                    unsigned int ScissorRectsCount, ViewportsCount;
                    D3D11_RECT ScissorRects[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
                    D3D11_VIEWPORT Viewports[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
                    ID3D11RasterizerState* RasterState;
                    ID3D11BlendState* BlendState;
                    FLOAT BlendFactor[4];
                    unsigned int SampleMask;
                    unsigned int StencilRef;
                    ID3D11DepthStencilState* DepthStencilState;
                    ID3D11ShaderResourceView* PSShaderResource;
                    ID3D11SamplerState* PSSampler;
                    ID3D11PixelShader* PS;
                    ID3D11VertexShader* VS;
                    unsigned int PSInstancesCount, VSInstancesCount;
                    ID3D11ClassInstance* PSInstances[256], *VSInstances[256];
                    D3D11_PRIMITIVE_TOPOLOGY PrimitiveTopology;
                    ID3D11Buffer* IndexBuffer, *VertexBuffer, *VSConstantBuffer;
                    unsigned int IndexBufferOffset, VertexBufferStride, VertexBufferOffset;
                    DXGI_FORMAT IndexBufferFormat;
                    ID3D11InputLayout* InputLayout;
                };

            private:
                ID3D11Buffer* VertexBuffer;
                ID3D11Buffer* IndexBuffer;
                ID3D10Blob* VertexShaderBlob;
                ID3D11VertexShader* VertexShader;
                ID3D11InputLayout* InputLayout;
                ID3D11Buffer* VertexConstantBuffer;
                ID3D10Blob* PixelShaderBlob;
                ID3D11PixelShader* PixelShader;
                ID3D11SamplerState* FontSamplerState;
                ID3D11ShaderResourceView* FontTextureView;
                ID3D11RasterizerState* RasterizerState;
                ID3D11BlendState* BlendState;
                ID3D11DepthStencilState* DepthStencilState;
                int VertexBufferSize;
                int IndexBufferSize;

            public:
                D3D11GUIRenderer(Engine::RenderSystem* Lab, Graphics::Activity* NewWindow);
                ~D3D11GUIRenderer();
                const char* GetVertexShaderCode();
                const char* GetPixelShaderCode();

            public:
                static void DrawList(void* Context);
            };
        }
    }
}
#endif
#endif