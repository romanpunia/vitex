#include "ogl.h"
#include "../components.h"
#include "../../resource.h"
#include <imgui.h>
#ifdef THAWK_HAS_GL

namespace Tomahawk
{
    namespace Graphics
    {
        namespace OGL
        {
            OGLModelRenderer::OGLModelRenderer(Engine::RenderSystem* Lab) : Engine::Renderers::ModelRenderer(Lab)
            {
            }
            void OGLModelRenderer::OnInitialize()
            {
            }
            void OGLModelRenderer::OnRender(Rest::Timer* Time)
            {
            }
            void OGLModelRenderer::OnRelease()
            {
            }

            OGLSkinnedModelRenderer::OGLSkinnedModelRenderer(Engine::RenderSystem* Lab) : Engine::Renderers::SkinnedModelRenderer(Lab)
            {
            }
            void OGLSkinnedModelRenderer::OnInitialize()
            {
            }
            void OGLSkinnedModelRenderer::OnRender(Rest::Timer* Time)
            {
            }
            void OGLSkinnedModelRenderer::OnRelease()
            {
            }

            OGLElementSystemRenderer::OGLElementSystemRenderer(Engine::RenderSystem* Lab) : Engine::Renderers::ElementSystemRenderer(Lab)
            {
            }
            void OGLElementSystemRenderer::OnInitialize()
            {
            }
            void OGLElementSystemRenderer::OnRender(Rest::Timer* Time)
            {
            }
            void OGLElementSystemRenderer::OnRelease()
            {
            }

            OGLDepthRenderer::OGLDepthRenderer(Engine::RenderSystem* Lab) : Engine::Renderers::DepthRenderer(Lab)
            {
            }
            void OGLDepthRenderer::OnInitialize()
            {
            }
            void OGLDepthRenderer::OnRender(Rest::Timer* Time)
            {
            }
            void OGLDepthRenderer::OnRelease()
            {
            }

            OGLProbeRenderer::OGLProbeRenderer(Engine::RenderSystem* Lab) : Engine::Renderers::ProbeRenderer(Lab)
            {
            }
            void OGLProbeRenderer::OnInitialize()
            {
            }
            void OGLProbeRenderer::OnRender(Rest::Timer* Time)
            {
            }
            void OGLProbeRenderer::OnRelease()
            {
            }

            OGLLightRenderer::OGLLightRenderer(Engine::RenderSystem* Lab) : Engine::Renderers::LightRenderer(Lab)
            {
            }
            void OGLLightRenderer::OnInitialize()
            {
            }
            void OGLLightRenderer::OnRender(Rest::Timer* Time)
            {
            }
            void OGLLightRenderer::OnRelease()
            {
            }
            void OGLLightRenderer::OnResizeBuffers()
            {
            }

            OGLImageRenderer::OGLImageRenderer(Engine::RenderSystem* Lab) : Engine::Renderers::ImageRenderer(Lab)
            {
            }
            OGLImageRenderer::OGLImageRenderer(Engine::RenderSystem* Lab, Graphics::RenderTarget2D* Target) : Engine::Renderers::ImageRenderer(Lab, Target)
            {
            }
            void OGLImageRenderer::OnInitialize()
            {
            }
            void OGLImageRenderer::OnRender(Rest::Timer* Time)
            {
            }
            void OGLImageRenderer::OnRelease()
            {
            }

            OGLReflectionsRenderer::OGLReflectionsRenderer(Engine::RenderSystem* Lab) : Engine::Renderers::ReflectionsRenderer(Lab)
            {
            }
            void OGLReflectionsRenderer::OnInitialize()
            {
            }
            void OGLReflectionsRenderer::OnRender(Rest::Timer* Time)
            {
            }
            void OGLReflectionsRenderer::OnRelease()
            {
            }
            void OGLReflectionsRenderer::OnResizeBuffers()
            {
            }

            OGLDepthOfFieldRenderer::OGLDepthOfFieldRenderer(Engine::RenderSystem* Lab) : Engine::Renderers::DepthOfFieldRenderer(Lab)
            {
            }
            void OGLDepthOfFieldRenderer::OnInitialize()
            {
            }
            void OGLDepthOfFieldRenderer::OnRender(Rest::Timer* Time)
            {
            }
            void OGLDepthOfFieldRenderer::OnRelease()
            {
            }
            void OGLDepthOfFieldRenderer::OnResizeBuffers()
            {
            }

            OGLEmissionRenderer::OGLEmissionRenderer(Engine::RenderSystem* Lab) : Engine::Renderers::EmissionRenderer(Lab)
            {
            }
            void OGLEmissionRenderer::OnInitialize()
            {
            }
            void OGLEmissionRenderer::OnRender(Rest::Timer* Time)
            {
            }
            void OGLEmissionRenderer::OnRelease()
            {
            }
            void OGLEmissionRenderer::OnResizeBuffers()
            {
            }

            OGLGlitchRenderer::OGLGlitchRenderer(Engine::RenderSystem* Lab) : Engine::Renderers::GlitchRenderer(Lab)
            {
            }
            void OGLGlitchRenderer::OnInitialize()
            {
            }
            void OGLGlitchRenderer::OnRender(Rest::Timer* Time)
            {
            }
            void OGLGlitchRenderer::OnRelease()
            {
            }
            void OGLGlitchRenderer::OnResizeBuffers()
            {
            }

            OGLAmbientOcclusionRenderer::OGLAmbientOcclusionRenderer(Engine::RenderSystem* Lab) : Engine::Renderers::AmbientOcclusionRenderer(Lab)
            {
            }
            void OGLAmbientOcclusionRenderer::OnInitialize()
            {
            }
            void OGLAmbientOcclusionRenderer::OnRender(Rest::Timer* Time)
            {
            }
            void OGLAmbientOcclusionRenderer::OnRelease()
            {
            }
            void OGLAmbientOcclusionRenderer::OnResizeBuffers()
            {
            }

            OGLIndirectOcclusionRenderer::OGLIndirectOcclusionRenderer(Engine::RenderSystem* Lab) : Engine::Renderers::IndirectOcclusionRenderer(Lab)
            {
            }
            void OGLIndirectOcclusionRenderer::OnInitialize()
            {
            }
            void OGLIndirectOcclusionRenderer::OnRender(Rest::Timer* Time)
            {
            }
            void OGLIndirectOcclusionRenderer::OnRelease()
            {
            }
            void OGLIndirectOcclusionRenderer::OnResizeBuffers()
            {
            }

            OGLToneRenderer::OGLToneRenderer(Engine::RenderSystem* Lab) : Engine::Renderers::ToneRenderer(Lab)
            {
            }
            void OGLToneRenderer::OnInitialize()
            {
            }
            void OGLToneRenderer::OnRender(Rest::Timer* Time)
            {
            }
            void OGLToneRenderer::OnRelease()
            {
            }
            void OGLToneRenderer::OnResizeBuffers()
            {
            }

            OGLGUIRenderer::OGLGUIRenderer(Engine::RenderSystem* Lab, Graphics::Activity* NewWindow) : GUIRenderer(Lab, NewWindow)
            {
                GLint LastTexture, LastArrayBuffer, LastVertexArray;
                glGetIntegerv(GL_TEXTURE_BINDING_2D, &LastTexture);
                glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &LastArrayBuffer);
                glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &LastVertexArray);

#ifdef HAS_OGL_GUI_GBUFFER_GLSL
				std::string ShaderCode = GET_RESOURCE_BATCH(ogl_gui_gbuffer_glsl);
#else
				std::string ShaderCode;
				THAWK_ERROR("gui-gbuffer.glsl was not compiled");
				return;
#endif

                OGLDevice* Device = Lab->GetDevice()->As<OGLDevice>();
                const GLchar* VertexShaderCode[3] = { Device->GetShaderVersion(), "#define VS\n", ShaderCode.c_str() };
                const GLchar* PixelShaderCode[3] = { Device->GetShaderVersion(), "#define PS\n", ShaderCode.c_str() };

                VertexShader = glCreateShader(GL_VERTEX_SHADER);
                glShaderSource(VertexShader, 2, VertexShaderCode, NULL);
                glCompileShader(VertexShader);

                std::string Error = Device->CompileState(VertexShader);
                if (!Error.empty())
                {
                    THAWK_ERROR("vertex shader compilation error -> %s", Error.c_str());
                    return;
                }

                PixelShader = glCreateShader(GL_FRAGMENT_SHADER);
                glShaderSource(PixelShader, 2, PixelShaderCode, NULL);
                glCompileShader(PixelShader);

                Error = Device->CompileState(PixelShader);
                if (!Error.empty())
                {
                    THAWK_ERROR("pixel shader compilation error -> %s", Error.c_str());
                    return;
                }

                ShaderProgram = glCreateProgram();
                glAttachShader(ShaderProgram, VertexShader);
                glAttachShader(ShaderProgram, PixelShader);
                glLinkProgram(ShaderProgram);

                Error = Device->CompileState(ShaderProgram);
                if (!Error.empty())
                {
                    THAWK_ERROR("shader program merge error -> %s", Error.c_str());
                    return;
                }

                Location.Position = glGetAttribLocation(ShaderProgram, "VS_Position");
                Location.TexCoord = glGetAttribLocation(ShaderProgram, "VS_TexCoord");
                Location.Color = glGetAttribLocation(ShaderProgram, "VS_Color");
                Location.Texture = glGetUniformLocation(ShaderProgram, "Texture");
                Device->CreateConstantBuffer(sizeof(Location.Buffer), UniformBuffer);

                glGenBuffers(1, &VertexBuffer);
                glGenBuffers(1, &ElementBuffer);

                unsigned char* Pixels;
                int Width, Height;
				GetFontAtlas(&Pixels, &Width, &Height);

                glGenTextures(1, &FontTexture);
                glBindTexture(GL_TEXTURE_2D, FontTexture);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
#ifdef GL_UNPACK_ROW_LENGTH
                glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, Width, Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, Pixels);

                glBindTexture(GL_TEXTURE_2D, LastTexture);
                glBindBuffer(GL_ARRAY_BUFFER, LastArrayBuffer);
                glBindVertexArray(LastVertexArray);

				Restore((void*)(ImTextureID)(intptr_t)FontTexture, (void*)DrawList);
            }
            OGLGUIRenderer::~OGLGUIRenderer()
            {
                glDeleteTextures(1, &FontTexture);
                glDeleteBuffers(1, &UniformBuffer);
                glDeleteBuffers(1, &VertexBuffer);
                glDeleteBuffers(1, &ElementBuffer);
                glDetachShader(ShaderProgram, VertexShader);
                glDetachShader(ShaderProgram, PixelShader);
                glDetachShader(ShaderProgram, VertexShader);
                glDeleteShader(PixelShader);
                glDeleteShader(VertexShader);
                glDeleteProgram(ShaderProgram);
            }
            void OGLGUIRenderer::DrawSetup(void* Context, int Width, int Height, GLuint VAO)
            {
                glEnable(GL_BLEND);
                glBlendEquation(GL_FUNC_ADD);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                glDisable(GL_CULL_FACE);
                glDisable(GL_DEPTH_TEST);
                glEnable(GL_SCISSOR_TEST);
#ifdef GL_POLYGON_MODE
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
#endif
                glViewport(0, 0, (GLsizei)Width, (GLsizei)Height);
                glUseProgram(ShaderProgram);
                glBindBufferBase(GL_UNIFORM_BUFFER, 3, UniformBuffer);
                glUniform1i(Location.TexCoord, 0);
#ifdef GL_SAMPLER_BINDING
                glBindSampler(0, 0);
#endif
                glBindVertexArray(VAO);
                glBindBuffer(GL_ARRAY_BUFFER, VertexBuffer);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ElementBuffer);
                glEnableVertexAttribArray(Location.Position);
                glEnableVertexAttribArray(Location.TexCoord);
                glEnableVertexAttribArray(Location.Color);
                glVertexAttribPointer(Location.Position, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)IM_OFFSETOF(ImDrawVert, pos));
                glVertexAttribPointer(Location.TexCoord, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)IM_OFFSETOF(ImDrawVert, uv));
                glVertexAttribPointer(Location.Color, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(ImDrawVert), (GLvoid*)IM_OFFSETOF(ImDrawVert, col));
            }
            void OGLGUIRenderer::DrawList(void* Context)
            {
                ImGuiIO* Settings = &ImGui::GetIO();
                if (!Settings->UserData)
                    return;

                OGLGUIRenderer* RefLink = (OGLGUIRenderer*)Settings->UserData;
                OGLDevice* Device = RefLink->GetRenderer()->GetDevice()->As<OGLDevice>();
                ImDrawData* Info = (ImDrawData*)Context;
                ImVec2 ClipScale = Settings->DisplayFramebufferScale;
                int Width = (int)(Settings->DisplaySize.x * Settings->DisplayFramebufferScale.x);
                int Height = (int)(Settings->DisplaySize.y * Settings->DisplayFramebufferScale.y);
                bool ClipOriginLowerLeft = true;

                if (Width <= 0 || Height <= 0)
                    return;

                OGLDeviceState* State = (OGLDeviceState*)Device->CreateState();
                if (State->ClipOrigin == GL_UPPER_LEFT)
                    ClipOriginLowerLeft = false;

                RefLink->Location.Buffer.WorldViewProjection = RefLink->WorldViewProjection;
                Device->As<OGLDevice>()->CopyConstantBuffer(RefLink->UniformBuffer, &RefLink->Location.Buffer, sizeof(RefLink->Location.Buffer));

                GLuint VAO = 0;
                glGenVertexArrays(1, &VAO);
                RefLink->DrawSetup(Context, Width, Height, VAO);

                for (int k = 0; k < Info->CmdListsCount; k++)
                {
                    const ImDrawList* CommandList = Info->CmdLists[k];
                    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)CommandList->VtxBuffer.Size * sizeof(ImDrawVert), (const GLvoid*)CommandList->VtxBuffer.Data, GL_STREAM_DRAW);
                    glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)CommandList->IdxBuffer.Size * sizeof(ImDrawIdx), (const GLvoid*)CommandList->IdxBuffer.Data, GL_STREAM_DRAW);

                    for (int i = 0; i < CommandList->CmdBuffer.Size; i++)
                    {
                        const ImDrawCmd* Command = &CommandList->CmdBuffer[i];
                        if (!Command->UserCallback)
                        {
                            ImVec4 ClipRect;
                            ClipRect.x = Command->ClipRect.x * ClipScale.x;
                            ClipRect.y = Command->ClipRect.y * ClipScale.y;
                            ClipRect.z = Command->ClipRect.z * ClipScale.x;
                            ClipRect.w = Command->ClipRect.w * ClipScale.y;

                            if (ClipRect.x < Width && ClipRect.y < Height && ClipRect.z >= 0.0f && ClipRect.w >= 0.0f)
                            {
                                if (ClipOriginLowerLeft)
                                    glScissor((int)ClipRect.x, (int)(Height - ClipRect.w), (int)(ClipRect.z - ClipRect.x), (int)(ClipRect.w - ClipRect.y));
                                else
                                    glScissor((int)ClipRect.x, (int)ClipRect.y, (int)ClipRect.z, (int)ClipRect.w);

                                glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)Command->TextureId);
                                glDrawElements(GL_TRIANGLES, (GLsizei)Command->ElemCount, sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT, (void*)(intptr_t)(Command->IdxOffset * sizeof(ImDrawIdx)));
                            }
                        }
                        else
                        {
                            if (Command->UserCallback == ImDrawCallback_ResetRenderState)
                                RefLink->DrawSetup(Context, Width, Height, VAO);
                            else
                                Command->UserCallback(CommandList, Command);
                        }
                    }
                }

                glDeleteVertexArrays(1, &VAO);
                Device->RestoreState(State);
                Device->ReleaseState((Graphics::DeviceState**)&State);
            }
        }
    }
}
#endif