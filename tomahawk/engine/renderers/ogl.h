#ifndef THAWK_ENGINE_RENDERERS_OGL_H
#define THAWK_ENGINE_RENDERERS_OGL_H

#include "../renderers.h"
#include "../../graphics/ogl.h"
#ifdef THAWK_HAS_GL

namespace Tomahawk
{
    namespace Graphics
    {
        namespace OGL
        {
            class OGLModelRenderer : public Engine::Renderers::ModelRenderer
            {
            public:
                OGLModelRenderer(Engine::RenderSystem* Lab);
                void OnInitialize();
                void OnRender(Rest::Timer* Time);
                void OnRelease();
            };

            class OGLSkinnedModelRenderer : public Engine::Renderers::SkinnedModelRenderer
            {
            public:
                OGLSkinnedModelRenderer(Engine::RenderSystem* Lab);
                void OnInitialize();
                void OnRender(Rest::Timer* Time);
                void OnRelease();
            };

            class OGLElementSystemRenderer : public Engine::Renderers::ElementSystemRenderer
            {
            public:
                OGLElementSystemRenderer(Engine::RenderSystem* Lab);
                void OnInitialize();
                void OnRender(Rest::Timer* Time);
                void OnRelease();
            };

            class OGLDepthRenderer : public Engine::Renderers::DepthRenderer
            {
            public:
                OGLDepthRenderer(Engine::RenderSystem* Lab);
                void OnInitialize();
                void OnRender(Rest::Timer* Time);
                void OnRelease();
            };

            class OGLProbeRenderer : public Engine::Renderers::ProbeRenderer
            {
            public:
                OGLProbeRenderer(Engine::RenderSystem* Lab);
                void OnInitialize();
                void OnRender(Rest::Timer* Time);
                void OnRelease();
            };

            class OGLLightRenderer : public Engine::Renderers::LightRenderer
            {
            public:
                OGLLightRenderer(Engine::RenderSystem* Lab);
                void OnInitialize();
                void OnRender(Rest::Timer* Time);
                void OnRelease();
                void OnResizeBuffers();
            };

            class OGLImageRenderer : public Engine::Renderers::ImageRenderer
            {
            public:
                OGLImageRenderer(Engine::RenderSystem* Lab);
                OGLImageRenderer(Engine::RenderSystem* Lab, Graphics::RenderTarget2D* Target);
                void OnInitialize();
                void OnRender(Rest::Timer* Time);
                void OnRelease();
            };

            class OGLReflectionsRenderer : public Engine::Renderers::ReflectionsRenderer
            {
            private:
                OGLDevice* Device;
                OGLElementBuffer* QuadVertex;
                unsigned int Stride, Offset;

            public:
                OGLReflectionsRenderer(Engine::RenderSystem* Lab);
                void OnInitialize();
                void OnRender(Rest::Timer* Time);
                void OnRelease();
                void OnResizeBuffers();
            };

            class OGLDepthOfFieldRenderer : public Engine::Renderers::DepthOfFieldRenderer
            {
            private:
                OGLDevice* Device;
                OGLElementBuffer* QuadVertex;
                unsigned int Stride, Offset;

            public:
                OGLDepthOfFieldRenderer(Engine::RenderSystem* Lab);
                void OnInitialize();
                void OnRender(Rest::Timer* Time);
                void OnRelease();
                void OnResizeBuffers();
            };

            class OGLEmissionRenderer : public Engine::Renderers::EmissionRenderer
            {
            private:
                OGLDevice* Device;
                OGLElementBuffer* QuadVertex;
                unsigned int Stride, Offset;

            public:
                OGLEmissionRenderer(Engine::RenderSystem* Lab);
                void OnInitialize();
                void OnRender(Rest::Timer* Time);
                void OnRelease();
                void OnResizeBuffers();
            };

            class OGLGlitchRenderer : public Engine::Renderers::GlitchRenderer
            {
            private:
                OGLDevice* Device;
                OGLElementBuffer* QuadVertex;
                unsigned int Stride, Offset;

            public:
                OGLGlitchRenderer(Engine::RenderSystem* Lab);
                void OnInitialize();
                void OnRender(Rest::Timer* Time);
                void OnRelease();
                void OnResizeBuffers();
            };

            class OGLAmbientOcclusionRenderer : public Engine::Renderers::AmbientOcclusionRenderer
            {
            private:
                OGLDevice* Device;
                OGLElementBuffer* QuadVertex;
                unsigned int Stride, Offset;

            public:
                OGLAmbientOcclusionRenderer(Engine::RenderSystem* Lab);
                void OnInitialize();
                void OnRender(Rest::Timer* Time);
                void OnRelease();
                void OnResizeBuffers();
            };

            class OGLIndirectOcclusionRenderer : public Engine::Renderers::IndirectOcclusionRenderer
            {
            private:
                OGLDevice* Device;
                OGLElementBuffer* QuadVertex;
                unsigned int Stride, Offset;

            public:
                OGLIndirectOcclusionRenderer(Engine::RenderSystem* Lab);
                void OnInitialize();
                void OnRender(Rest::Timer* Time);
                void OnRelease();
                void OnResizeBuffers();
            };

            class OGLToneRenderer : public Engine::Renderers::ToneRenderer
            {
            private:
                OGLDevice* Device;
                OGLElementBuffer* QuadVertex;
                unsigned int Stride, Offset;

            public:
                OGLToneRenderer(Engine::RenderSystem* Lab);
                void OnInitialize();
                void OnRender(Rest::Timer* Time);
                void OnRelease();
                void OnResizeBuffers();
            };

            class OGLGUIRenderer : public Engine::Renderers::GUIRenderer
            {
            private:
                struct DeviceState
                {
                    // OpenGL 4.5 Needs
                };

                struct
                {
                    struct
                    {
                        Compute::Matrix4x4 WorldViewProjection;
                    } Buffer;

                    GLuint Position = GL_INVALID_VALUE;
                    GLuint TexCoord = GL_INVALID_VALUE;
                    GLuint Color = GL_INVALID_VALUE;
                    GLuint Texture = GL_INVALID_VALUE;
                } Location;

            private:
                GLuint VertexShader = GL_INVALID_VALUE;
                GLuint PixelShader = GL_INVALID_VALUE;
                GLuint ShaderProgram = GL_INVALID_VALUE;
                GLuint UniformBuffer = GL_INVALID_VALUE;
                GLuint VertexBuffer = GL_INVALID_VALUE;
                GLuint ElementBuffer = GL_INVALID_VALUE;
                GLuint FontTexture = GL_INVALID_VALUE;

            public:
                OGLGUIRenderer(Engine::RenderSystem* Lab, Graphics::Activity* NewWindow);
                ~OGLGUIRenderer();
                void DrawSetup(void* Context, int Width, int Height, GLuint VAO);
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