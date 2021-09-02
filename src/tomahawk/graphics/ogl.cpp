#include "ogl.h"
#ifdef TH_HAS_SDL2
#include <SDL2/SDL_syswm.h>
#undef DirectColor
#endif
#ifdef TH_HAS_GL
#define GLSL_INLINE(Code) #Code
#define BUFFER_OFFSET(i) (GLvoid*)(i)

namespace Tomahawk
{
	namespace Graphics
	{
		namespace OGL
		{
			OGLFrameBuffer::OGLFrameBuffer(GLuint Targets) : Count(Targets)
			{
			}
			void OGLFrameBuffer::Release()
			{
				glDeleteFramebuffers(1, &Buffer);
				glDeleteTextures(Count, Texture);
			}

			OGLDepthStencilState::OGLDepthStencilState(const Desc& I) : DepthStencilState(I)
			{
			}
			OGLDepthStencilState::~OGLDepthStencilState()
			{
			}
			void* OGLDepthStencilState::GetResource()
			{
				return (void*)&State;
			}

			OGLRasterizerState::OGLRasterizerState(const Desc& I) : RasterizerState(I)
			{
			}
			OGLRasterizerState::~OGLRasterizerState()
			{
			}
			void* OGLRasterizerState::GetResource()
			{
				return (void*)&State;
			}

			OGLBlendState::OGLBlendState(const Desc& I) : BlendState(I)
			{
			}
			OGLBlendState::~OGLBlendState()
			{
			}
			void* OGLBlendState::GetResource()
			{
				return (void*)&State;
			}

			OGLSamplerState::OGLSamplerState(const Desc& I) : SamplerState(I)
			{
			}
			OGLSamplerState::~OGLSamplerState()
			{
				glDeleteSamplers(1, &Resource);
			}
			void* OGLSamplerState::GetResource()
			{
				return (void*)(intptr_t)Resource;
			}

			OGLInputLayout::OGLInputLayout(const Desc& I) : InputLayout(I)
			{
				for (size_t i = 0; i < I.Attributes.size(); i++)
				{
					const Attribute& It = I.Attributes[i];
					GLenum Format = GL_INVALID_ENUM;
					size_t Stride = It.AlignedByteOffset;
					GLint Size = It.Components;
					bool Decimal = false;

					switch (It.Format)
					{
						case Tomahawk::Graphics::AttributeType::Byte:
							Format = GL_BYTE;
							Decimal = true;
							break;
						case Tomahawk::Graphics::AttributeType::Ubyte:
							Format = GL_UNSIGNED_BYTE;
							Decimal = true;
							break;
						case Tomahawk::Graphics::AttributeType::Half:
							Format = GL_HALF_FLOAT;
							Decimal = false;
							break;
						case Tomahawk::Graphics::AttributeType::Float:
							Format = GL_FLOAT;
							Decimal = false;
							break;
						case Tomahawk::Graphics::AttributeType::Int:
							Format = GL_INT;
							Decimal = true;
							break;
						case Tomahawk::Graphics::AttributeType::Uint:
							Format = GL_UNSIGNED_INT;
							Decimal = true;
							break;
						default:
							break;
					}

					if (Decimal)
						VertexLayout.emplace_back([i, Format, Stride, Size](uint64_t Width) { glVertexAttribIPointer(i, Size, Format, Width, (GLvoid*)Stride); });
					else
						VertexLayout.emplace_back([i, Format, Stride, Size](uint64_t Width) { glVertexAttribPointer(i, Size, Format, GL_FALSE, Width, (GLvoid*)Stride); });
				}
			}
			OGLInputLayout::~OGLInputLayout()
			{
			}
			void* OGLInputLayout::GetResource()
			{
				return (void*)this;
			}

			OGLShader::OGLShader(const Desc& I) : Shader(I), Compiled(false)
			{
			}
			OGLShader::~OGLShader()
			{
				for (auto& It : Programs)
					glDeleteProgram(It.second);
				glDeleteShader(VertexShader);
				glDeleteShader(PixelShader);
				glDeleteShader(GeometryShader);
				glDeleteShader(DomainShader);
				glDeleteShader(HullShader);
				glDeleteShader(ComputeShader);
			}
			bool OGLShader::IsValid()
			{
				return Compiled;
			}

			OGLElementBuffer::OGLElementBuffer(const Desc& I) : ElementBuffer(I)
			{
			}
			OGLElementBuffer::~OGLElementBuffer()
			{
				glDeleteBuffers(1, &Resource);
			}
			void* OGLElementBuffer::GetResource()
			{
				return (void*)(intptr_t)Resource;
			}

			OGLMeshBuffer::OGLMeshBuffer(const Desc& I) : MeshBuffer(I)
			{
			}
			Compute::Vertex* OGLMeshBuffer::GetElements(GraphicsDevice* Device)
			{
				TH_ASSERT(Device != nullptr, nullptr, "graphics device should be set");

				MappedSubresource Resource;
				Device->Map(VertexBuffer, ResourceMap::Write, &Resource);

				Compute::Vertex* Vertices = (Compute::Vertex*)TH_MALLOC(sizeof(Compute::Vertex) * (unsigned int)VertexBuffer->GetElements());
				memcpy(Vertices, Resource.Pointer, (size_t)VertexBuffer->GetElements() * sizeof(Compute::Vertex));

				Device->Unmap(VertexBuffer, &Resource);
				return Vertices;
			}

			OGLSkinMeshBuffer::OGLSkinMeshBuffer(const Desc& I) : SkinMeshBuffer(I)
			{
			}
			Compute::SkinVertex* OGLSkinMeshBuffer::GetElements(GraphicsDevice* Device)
			{
				TH_ASSERT(Device != nullptr, nullptr, "graphics device should be set");

				MappedSubresource Resource;
				Device->Map(VertexBuffer, ResourceMap::Write, &Resource);

				Compute::SkinVertex* Vertices = (Compute::SkinVertex*)TH_MALLOC(sizeof(Compute::SkinVertex) * (unsigned int)VertexBuffer->GetElements());
				memcpy(Vertices, Resource.Pointer, (size_t)VertexBuffer->GetElements() * sizeof(Compute::SkinVertex));

				Device->Unmap(VertexBuffer, &Resource);
				return Vertices;
			}

			OGLInstanceBuffer::OGLInstanceBuffer(const Desc& I) : InstanceBuffer(I)
			{
			}
			OGLInstanceBuffer::~OGLInstanceBuffer()
			{
				if (Device != nullptr && Sync)
					Device->ClearBuffer(this);
			}

			OGLTexture2D::OGLTexture2D() : Texture2D()
			{
			}
			OGLTexture2D::OGLTexture2D(const Desc& I) : Texture2D(I)
			{
			}
			OGLTexture2D::~OGLTexture2D()
			{
				glDeleteTextures(1, &Resource);
			}
			void* OGLTexture2D::GetResource()
			{
				return (void*)(intptr_t)Resource;
			}

			OGLTexture3D::OGLTexture3D() : Texture3D()
			{
			}
			OGLTexture3D::~OGLTexture3D()
			{
				glDeleteTextures(1, &Resource);
			}
			void* OGLTexture3D::GetResource()
			{
				return (void*)(intptr_t)Resource;
			}

			OGLTextureCube::OGLTextureCube() : TextureCube()
			{
			}
			OGLTextureCube::OGLTextureCube(const Desc& I) : TextureCube(I)
			{
			}
			OGLTextureCube::~OGLTextureCube()
			{
				glDeleteTextures(1, &Resource);
			}
			void* OGLTextureCube::GetResource()
			{
				return (void*)(intptr_t)Resource;
			}

			OGLDepthBuffer::OGLDepthBuffer(const Desc& I) : Graphics::DepthBuffer(I)
			{
			}
			OGLDepthBuffer::~OGLDepthBuffer()
			{
				glDeleteFramebuffers(1, &FrameBuffer);
				glDeleteTextures(1, &DepthTexture);
			}
			void* OGLDepthBuffer::GetResource()
			{
				return (void*)(intptr_t)FrameBuffer;
			}
			uint32_t OGLDepthBuffer::GetWidth()
			{
				return Viewarea.Width;
			}
			uint32_t OGLDepthBuffer::GetHeight()
			{
				return Viewarea.Height;
			}

			OGLRenderTarget2D::OGLRenderTarget2D(const Desc& I) : RenderTarget2D(I), FrameBuffer(1)
			{
			}
			OGLRenderTarget2D::~OGLRenderTarget2D()
			{
				glDeleteTextures(1, &DepthTexture);
				FrameBuffer.Release();
			}
			void* OGLRenderTarget2D::GetTargetBuffer()
			{
				return (void*)&FrameBuffer;
			}
			void* OGLRenderTarget2D::GetDepthBuffer()
			{
				return (void*)(intptr_t)DepthTexture;
			}
			uint32_t OGLRenderTarget2D::GetWidth()
			{
				return Viewarea.Width;
			}
			uint32_t OGLRenderTarget2D::GetHeight()
			{
				return Viewarea.Height;
			}

			OGLMultiRenderTarget2D::OGLMultiRenderTarget2D(const Desc& I) : MultiRenderTarget2D(I), FrameBuffer((GLuint)I.Target)
			{
			}
			OGLMultiRenderTarget2D::~OGLMultiRenderTarget2D()
			{
				glDeleteTextures(1, &DepthTexture);
				FrameBuffer.Release();
			}
			void* OGLMultiRenderTarget2D::GetTargetBuffer()
			{
				return (void*)&FrameBuffer;
			}
			void* OGLMultiRenderTarget2D::GetDepthBuffer()
			{
				return (void*)(intptr_t)DepthTexture;
			}
			uint32_t OGLMultiRenderTarget2D::GetWidth()
			{
				return Viewarea.Width;
			}
			uint32_t OGLMultiRenderTarget2D::GetHeight()
			{
				return Viewarea.Height;
			}

			OGLRenderTargetCube::OGLRenderTargetCube(const Desc& I) : RenderTargetCube(I), FrameBuffer(1)
			{
			}
			OGLRenderTargetCube::~OGLRenderTargetCube()
			{
				glDeleteTextures(1, &DepthTexture);
				FrameBuffer.Release();
			}
			void* OGLRenderTargetCube::GetTargetBuffer()
			{
				return (void*)&FrameBuffer;
			}
			void* OGLRenderTargetCube::GetDepthBuffer()
			{
				return (void*)(intptr_t)DepthTexture;
			}
			uint32_t OGLRenderTargetCube::GetWidth()
			{
				return Viewarea.Width;
			}
			uint32_t OGLRenderTargetCube::GetHeight()
			{
				return Viewarea.Height;
			}

			OGLMultiRenderTargetCube::OGLMultiRenderTargetCube(const Desc& I) : MultiRenderTargetCube(I), FrameBuffer((GLuint)I.Target)
			{
			}
			OGLMultiRenderTargetCube::~OGLMultiRenderTargetCube()
			{
				glDeleteTextures(1, &DepthTexture);
				FrameBuffer.Release();
			}
			void* OGLMultiRenderTargetCube::GetTargetBuffer()
			{
				return (void*)&FrameBuffer;
			}
			void* OGLMultiRenderTargetCube::GetDepthBuffer()
			{
				return (void*)(intptr_t)DepthTexture;
			}
			uint32_t OGLMultiRenderTargetCube::GetWidth()
			{
				return Viewarea.Width;
			}
			uint32_t OGLMultiRenderTargetCube::GetHeight()
			{
				return Viewarea.Height;
			}

			OGLCubemap::OGLCubemap(const Desc& I) : Cubemap(I)
			{
				TH_ASSERT_V(I.Source != nullptr, "source should be set");
				TH_ASSERT_V(I.Target < I.Source->GetTargetCount(), "targets count should be less than %i", (int)I.Source->GetTargetCount());

				OGLFrameBuffer* TargetBuffer = (OGLFrameBuffer*)I.Source->GetTargetBuffer();
				TH_ASSERT_V(!TargetBuffer->Backbuffer, "cannot copy from backbuffer directly");

				glGenFramebuffers(2, Buffers);
				glGenTextures(1, &Resource);
			}
			OGLCubemap::~OGLCubemap()
			{
				glDeleteTextures(1, &Resource);
				glDeleteFramebuffers(2, Buffers);
			}

			OGLQuery::OGLQuery() : Query()
			{
			}
			OGLQuery::~OGLQuery()
			{
				glDeleteQueries(1, &Async);
			}
			void* OGLQuery::GetResource()
			{
				return (void*)(intptr_t)Async;
			}

			OGLDevice::OGLDevice(const Desc& I) : GraphicsDevice(I), ShaderVersion(nullptr), Layout(nullptr), Window(I.Window), Context(nullptr)
			{
				TH_ASSERT_V(Window != nullptr, "OpenGL device cannot be created without a window");
				DirectRenderer.VertexShader = GL_NONE;
				DirectRenderer.PixelShader = GL_NONE;
				DirectRenderer.Program = GL_NONE;
				DirectRenderer.VertexBuffer = GL_NONE;
				IndexType = GL_UNSIGNED_INT;
#ifdef TH_HAS_SDL2
				if (I.Debug)
					SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
				else
					SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
				
				Context = SDL_GL_CreateContext(Window->GetHandle());
				if (!Context)
				{
					TH_ERR("OpenGL creation conflict -> %s", Window->GetError().c_str());
					return;
				}

				SDL_GL_MakeCurrent(Window->GetHandle(), Context);
				switch (VSyncMode)
				{
					case VSync::None:
						SDL_GL_SetSwapInterval(0);
						break;
					case VSync::Frequency_X1:
						SDL_GL_SetSwapInterval(1);
						break;
					case VSync::Frequency_X2:
					case VSync::Frequency_X3:
					case VSync::Frequency_X4:
						if (SDL_GL_SetSwapInterval(-1) == -1)
							SDL_GL_SetSwapInterval(1);
						break;
				}
#endif
				const GLenum ErrorCode = glewInit();
				if (ErrorCode != GLEW_OK)
				{
					TH_ERR("[glew] %s", (const char*)glewGetErrorString(ErrorCode));
					return;
				}

				if (I.Debug)
				{
					glEnable(GL_DEBUG_OUTPUT);
					glDebugMessageCallback(DebugMessage, nullptr);
				}

				CreateConstantBuffer(&ConstantBuffer[0], sizeof(AnimationBuffer));
				Constants[0] = &Animation;
				ConstantSize[0] = sizeof(Animation);

				CreateConstantBuffer(&ConstantBuffer[1], sizeof(RenderBuffer));
				Constants[1] = &Render;
				ConstantSize[0] = sizeof(RenderBuffer);

				CreateConstantBuffer(&ConstantBuffer[2], sizeof(ViewBuffer));
				Constants[2] = &View;
				ConstantSize[0] = sizeof(ViewBuffer);

				glBindBufferBase(GL_UNIFORM_BUFFER, 0, ConstantBuffer[0]);
				glBindBufferBase(GL_UNIFORM_BUFFER, 1, ConstantBuffer[1]);
				glBindBufferBase(GL_UNIFORM_BUFFER, 2, ConstantBuffer[2]);
				glEnable(GL_TEXTURE_2D);

				SetShaderModel(I.ShaderMode == ShaderModel::Auto ? GetSupportedShaderModel() : I.ShaderMode);
				ResizeBuffers(I.BufferWidth, I.BufferHeight);
				CreateStates();

				Shader::Desc F = Shader::Desc();
				if (GetSection("geometry/basic/geometry", &F))
					BasicEffect = CreateShader(F);
			}
			OGLDevice::~OGLDevice()
			{
				ReleaseProxy();
				glDeleteShader(DirectRenderer.VertexShader);
				glDeleteShader(DirectRenderer.PixelShader);
				glDeleteProgram(DirectRenderer.Program);
				glDeleteBuffers(1, &DirectRenderer.VertexBuffer);
				glDeleteBuffers(3, ConstantBuffer);
#ifdef TH_HAS_SDL2
				if (Context != nullptr)
					SDL_GL_DeleteContext(Context);
#endif
			}
			void OGLDevice::SetConstantBuffers()
			{
				glBindBufferBase(GL_UNIFORM_BUFFER, 0, ConstantBuffer[0]);
				glBindBufferBase(GL_UNIFORM_BUFFER, 1, ConstantBuffer[1]);
				glBindBufferBase(GL_UNIFORM_BUFFER, 2, ConstantBuffer[2]);
			}
			void OGLDevice::SetShaderModel(ShaderModel Model)
			{
				ShaderGen = Model;
				if (ShaderGen == ShaderModel::GLSL_1_1_0)
					ShaderVersion = "#version 110 core\n";
				else if (ShaderGen == ShaderModel::GLSL_1_2_0)
					ShaderVersion = "#version 120 core\n";
				else if (ShaderGen == ShaderModel::GLSL_1_3_0)
					ShaderVersion = "#version 130 core\n";
				else if (ShaderGen == ShaderModel::GLSL_1_4_0)
					ShaderVersion = "#version 140 core\n";
				else if (ShaderGen == ShaderModel::GLSL_1_5_0)
					ShaderVersion = "#version 150 core\n";
				else if (ShaderGen == ShaderModel::GLSL_3_3_0)
					ShaderVersion = "#version 330 core\n";
				else if (ShaderGen == ShaderModel::GLSL_4_0_0)
					ShaderVersion = "#version 400 core\n";
				else if (ShaderGen == ShaderModel::GLSL_4_1_0)
					ShaderVersion = "#version 410 core\n";
				else if (ShaderGen == ShaderModel::GLSL_4_2_0)
					ShaderVersion = "#version 420 core\n";
				else if (ShaderGen == ShaderModel::GLSL_4_3_0)
					ShaderVersion = "#version 430 core\n";
				else if (ShaderGen == ShaderModel::GLSL_4_4_0)
					ShaderVersion = "#version 440 core\n";
				else if (ShaderGen == ShaderModel::GLSL_4_5_0)
					ShaderVersion = "#version 450 core\n";
				else if (ShaderGen == ShaderModel::GLSL_4_6_0)
					ShaderVersion = "#version 460 core\n";
				else
					SetShaderModel(ShaderModel::GLSL_1_1_0);
			}
			void OGLDevice::SetBlendState(BlendState* State)
			{
				TH_ASSERT_V(State != nullptr, "blend state should be set");
				OGLBlendState* V = (OGLBlendState*)State;
				if (V->State.AlphaToCoverageEnable)
				{
					glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);
					glSampleCoverage(1.0f, GL_FALSE);
				}
				else
				{
					glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);
					glSampleCoverage(0.0f, GL_FALSE);
				}

				if (!V->State.IndependentBlendEnable)
				{
					for (int i = 0; i < 8; i++)
					{
						if (V->State.RenderTarget[i].BlendEnable)
							glEnablei(GL_BLEND, i);
						else
							glDisablei(GL_BLEND, i);

						glBlendEquationSeparatei(i, GetBlendOperation(V->State.RenderTarget[i].BlendOperationMode), GetBlendOperation(V->State.RenderTarget[i].BlendOperationAlpha));
						glBlendFuncSeparatei(i, GetBlend(V->State.RenderTarget[i].SrcBlend), GetBlend(V->State.RenderTarget[i].DestBlend), GetBlend(V->State.RenderTarget[i].SrcBlendAlpha), GetBlend(V->State.RenderTarget[i].DestBlendAlpha));
					}
				}
				else
				{
					if (V->State.RenderTarget[0].BlendEnable)
						glEnable(GL_BLEND);
					else
						glDisable(GL_BLEND);

					glBlendEquationSeparate(GetBlendOperation(V->State.RenderTarget[0].BlendOperationMode), GetBlendOperation(V->State.RenderTarget[0].BlendOperationAlpha));
					glBlendFuncSeparate(GetBlend(V->State.RenderTarget[0].SrcBlend), GetBlend(V->State.RenderTarget[0].DestBlend), GetBlend(V->State.RenderTarget[0].SrcBlendAlpha), GetBlend(V->State.RenderTarget[0].DestBlendAlpha));
				}
			}
			void OGLDevice::SetRasterizerState(RasterizerState* State)
			{
				TH_ASSERT_V(State != nullptr, "rasterizer state should be set");
				OGLRasterizerState* V = (OGLRasterizerState*)State;
				if (V->State.AntialiasedLineEnable || V->State.MultisampleEnable)
					glEnable(GL_MULTISAMPLE);
				else
					glDisable(GL_MULTISAMPLE);

				if (V->State.CullMode == VertexCull::Back)
				{
					glCullFace(GL_FRONT);
					glEnable(GL_CULL_FACE);
				}
				else if (V->State.CullMode == VertexCull::Front)
				{
					glCullFace(GL_BACK);
					glEnable(GL_CULL_FACE);
				}
				else
					glDisable(GL_CULL_FACE);

				if (V->State.FillMode == SurfaceFill::Solid)
					glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
				else
					glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

				if (V->State.FrontCounterClockwise)
					glFrontFace(GL_CW);
				else
					glFrontFace(GL_CCW);
			}
			void OGLDevice::SetDepthStencilState(DepthStencilState* State)
			{
				TH_ASSERT_V(State != nullptr, "depth stencil state should be set");
				OGLDepthStencilState* V = (OGLDepthStencilState*)State;
				if (V->State.DepthEnable)
					glEnable(GL_DEPTH_TEST);
				else
					glDisable(GL_DEPTH_TEST);

				if (V->State.StencilEnable)
					glEnable(GL_STENCIL_TEST);
				else
					glDisable(GL_STENCIL_TEST);

				glDepthFunc(GetComparison(V->State.DepthFunction));
				glDepthMask(V->State.DepthWriteMask == DepthWrite::All ? GL_TRUE : GL_FALSE);
				glStencilMask((GLuint)V->State.StencilWriteMask);
				glStencilFuncSeparate(GL_FRONT, GetComparison(V->State.FrontFaceStencilFunction), 0, 1);
				glStencilOpSeparate(GL_FRONT, GetStencilOperation(V->State.FrontFaceStencilFailOperation), GetStencilOperation(V->State.FrontFaceStencilDepthFailOperation), GetStencilOperation(V->State.FrontFaceStencilPassOperation));
				glStencilFuncSeparate(GL_BACK, GetComparison(V->State.BackFaceStencilFunction), 0, 1);
				glStencilOpSeparate(GL_BACK, GetStencilOperation(V->State.BackFaceStencilFailOperation), GetStencilOperation(V->State.BackFaceStencilDepthFailOperation), GetStencilOperation(V->State.BackFaceStencilPassOperation));
			}
			void OGLDevice::SetInputLayout(InputLayout* State)
			{
				if (Layout != nullptr)
				{
					for (size_t i = 0; i < Layout->VertexLayout.size(); i++)
						glDisableVertexAttribArray(i);
				}

				Layout = (OGLInputLayout*)State;
				if (Layout != nullptr)
				{
					for (size_t i = 0; i < Layout->VertexLayout.size(); i++)
						glEnableVertexAttribArray(i);
				}
			}
			void OGLDevice::SetShader(Shader* Resource, unsigned int Type)
			{
				TH_ASSERT_V(Resource != nullptr, "resource should be set");
				OGLShader* IResource = (OGLShader*)Resource;

				TH_ASSERT_V(IResource->IsValid(), "resource should be set");
				auto It = IResource->Programs.find(Type);
				if (It != IResource->Programs.end())
					return (void)glUseProgramObjectARB(It->second);

				GLuint Program = glCreateProgram();
				if (Type & (uint32_t)ShaderType::Vertex && IResource->VertexShader != GL_NONE)
					glAttachShader(Program, IResource->VertexShader);

				if (Type & (uint32_t)ShaderType::Pixel && IResource->PixelShader != GL_NONE)
					glAttachShader(Program, IResource->PixelShader);

				if (Type & (uint32_t)ShaderType::Geometry && IResource->GeometryShader != GL_NONE)
					glAttachShader(Program, IResource->GeometryShader);

				if (Type & (uint32_t)ShaderType::Domain && IResource->DomainShader != GL_NONE)
					glAttachShader(Program, IResource->DomainShader);

				if (Type & (uint32_t)ShaderType::Hull && IResource->HullShader != GL_NONE)
					glAttachShader(Program, IResource->HullShader);

				if (Type & (uint32_t)ShaderType::Compute && IResource->ComputeShader != GL_NONE)
					glAttachShader(Program, IResource->ComputeShader);

				GLint StatusCode = 0;
				glLinkProgramARB(Program);
				glGetProgramiv(Program, GL_LINK_STATUS, &StatusCode);

				if (StatusCode == GL_TRUE)
				{
					GLuint AnimationId = glGetUniformBlockIndex(Program, "Animation");
					if (AnimationId != GL_INVALID_INDEX)
						glUniformBlockBinding(Program, AnimationId, 0);

					GLuint ObjectId = glGetUniformBlockIndex(Program, "Object");
					if (ObjectId != GL_INVALID_INDEX)
						glUniformBlockBinding(Program, ObjectId, 1);

					GLuint ViewerId = glGetUniformBlockIndex(Program, "Viewer");
					if (ViewerId != GL_INVALID_INDEX)
						glUniformBlockBinding(Program, ViewerId, 2);

					GLuint RenderConstantId = glGetUniformBlockIndex(Program, "RenderConstant");
					if (RenderConstantId != GL_INVALID_INDEX)
						glUniformBlockBinding(Program, RenderConstantId, 3);

					GLint Uniforms = 0;
					glGetProgramiv(Program, GL_ACTIVE_UNIFORMS, &Uniforms);
					for (GLint i = 0; i < Uniforms; i++)
					{
						GLchar Name[64]; GLsizei Length; GLint Size; GLenum Subtype;
						glGetActiveUniform(Program, (GLuint)i, 64, &Length, &Size, &Subtype, Name);
						if (Subtype != GL_SAMPLER_1D && Subtype != GL_SAMPLER_2D && Subtype != GL_SAMPLER_3D && Subtype != GL_SAMPLER_CUBE)
							continue;

						GLint Location = glGetUniformLocationARB(Program, Name);
						glUniform1iARB(Location, Location);
					}
				}
				else
				{
					GLint Size = 0;
					glGetProgramiv(Program, GL_INFO_LOG_LENGTH, &Size);

					char* Buffer = (char*)TH_MALLOC(sizeof(char) * (Size + 1));
					glGetProgramInfoLog(Program, Size, &Size, Buffer);
					TH_ERR("couldn't link shaders\n\t%.*s", Size, Buffer);
					TH_FREE(Buffer);

					glDeleteProgram(Program);
					Program = GL_NONE;
				}

				IResource->Programs[Type] = Program;
				glUseProgramObjectARB(Program);
			}
			void OGLDevice::SetSamplerState(SamplerState* State, unsigned int Slot, unsigned int Type)
			{
				glBindSampler(Slot, (GLuint)(State ? ((OGLSamplerState*)State)->Resource : GL_NONE));
			}
			void OGLDevice::SetBuffer(Shader* Resource, unsigned int Slot, unsigned int Type)
			{
				OGLShader* IResource = (OGLShader*)Resource;
				glBindBufferBase(GL_UNIFORM_BUFFER, Slot, IResource ? IResource->ConstantBuffer : GL_NONE);
			}
			void OGLDevice::SetBuffer(InstanceBuffer* Resource, unsigned int Slot, unsigned int Type)
			{
				OGLInstanceBuffer* IResource = (OGLInstanceBuffer*)Resource;
				SetStructureBuffer(IResource ? IResource->Elements : nullptr, Slot, Type);
			}
			void OGLDevice::SetStructureBuffer(ElementBuffer* Resource, unsigned int Slot, unsigned int Type)
			{
				glBindBufferBase(GL_SHADER_STORAGE_BUFFER, Slot, Resource ? ((OGLElementBuffer*)Resource)->Resource : GL_NONE);
			}
			void OGLDevice::SetIndexBuffer(ElementBuffer* Resource, Format FormatMode)
			{
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, Resource ? ((OGLElementBuffer*)Resource)->Resource : GL_NONE);
				if (FormatMode == Format::R32_Uint)
					IndexType = GL_UNSIGNED_INT;
				else if (FormatMode == Format::R16_Uint)
					IndexType = GL_UNSIGNED_SHORT;
				else if (FormatMode == Format::R8_Uint)
					IndexType = GL_UNSIGNED_BYTE;
				else
					IndexType = GL_UNSIGNED_INT;
			}
			void OGLDevice::SetVertexBuffer(ElementBuffer* Resource, unsigned int Slot)
			{
				OGLElementBuffer* IResource = (OGLElementBuffer*)Resource;
				glBindBuffer(GL_ARRAY_BUFFER, IResource ? IResource->Resource : GL_NONE);
				if (!IResource || IResource->Layout == Layout)
					return;

				IResource->Layout = Layout;
				for (auto& Attribute : Layout->VertexLayout)
					Attribute(IResource->Stride);
			}
			void OGLDevice::SetTexture2D(Texture2D* Resource, unsigned int Slot, unsigned int Type)
			{
				glActiveTexture(GL_TEXTURE0 + Slot);
				glBindTexture(GL_TEXTURE_2D, Resource ? ((OGLTexture2D*)Resource)->Resource : GL_NONE);
			}
			void OGLDevice::SetTexture3D(Texture3D* Resource, unsigned int Slot, unsigned int Type)
			{
				glActiveTexture(GL_TEXTURE0 + Slot);
				glBindTexture(GL_TEXTURE_3D, Resource ? ((OGLTexture3D*)Resource)->Resource : GL_NONE);
			}
			void OGLDevice::SetTextureCube(TextureCube* Resource, unsigned int Slot, unsigned int Type)
			{
				glActiveTexture(GL_TEXTURE0 + Slot);
				glBindTexture(GL_TEXTURE_CUBE_MAP, Resource ? ((OGLTextureCube*)Resource)->Resource : GL_NONE);
			}
			void OGLDevice::SetWriteable(ElementBuffer** Resource, unsigned int Count, unsigned int Slot, bool Computable)
			{
			}
			void OGLDevice::SetWriteable(Texture2D** Resource, unsigned int Count, unsigned int Slot, bool Computable)
			{
				TH_ASSERT_V(Resource != nullptr, "resource should be set");
				for (unsigned int i = 0; i < Count; i++)
				{
					OGLTexture2D* IResource = (OGLTexture2D*)Resource[i];
					glActiveTexture(GL_TEXTURE0 + Slot + i);

					if (!IResource)
						glBindTexture(GL_TEXTURE_2D, GL_NONE);
					else
						glBindImageTexture(Slot + i, IResource->Resource, 0, GL_TRUE, 0, GL_READ_WRITE, IResource->Format);
				}
			}
			void OGLDevice::SetWriteable(Texture3D** Resource, unsigned int Count, unsigned int Slot, bool Computable)
			{
				TH_ASSERT_V(Resource != nullptr, "resource should be set");
				for (unsigned int i = 0; i < Count; i++)
				{
					OGLTexture3D* IResource = (OGLTexture3D*)Resource[i];
					glActiveTexture(GL_TEXTURE0 + Slot + i);

					if (!IResource)
						glBindTexture(GL_TEXTURE_3D, GL_NONE);
					else
						glBindImageTexture(Slot + i, IResource->Resource, 0, GL_TRUE, 0, GL_READ_WRITE, IResource->Format);
				}
			}
			void OGLDevice::SetWriteable(TextureCube** Resource, unsigned int Count, unsigned int Slot, bool Computable)
			{
				TH_ASSERT_V(Resource != nullptr, "resource should be set");
				for (unsigned int i = 0; i < Count; i++)
				{
					OGLTextureCube* IResource = (OGLTextureCube*)Resource[i];
					glActiveTexture(GL_TEXTURE0 + Slot + i);

					if (!IResource)
						glBindTexture(GL_TEXTURE_CUBE_MAP, GL_NONE);
					else
						glBindImageTexture(Slot + i, IResource->Resource, 0, GL_TRUE, 0, GL_READ_WRITE, IResource->Format);
				}
			}
			void OGLDevice::SetTarget(float R, float G, float B)
			{
				SetTarget(RenderTarget, 0, R, G, B);
			}
			void OGLDevice::SetTarget()
			{
				SetTarget(RenderTarget, 0);
			}
			void OGLDevice::SetTarget(DepthBuffer* Resource)
			{
				TH_ASSERT_V(Resource != nullptr, "resource should be set");
				OGLDepthBuffer* IResource = (OGLDepthBuffer*)Resource;
				GLenum Target = GL_NONE;
				glBindFramebuffer(GL_FRAMEBUFFER, IResource->FrameBuffer);
				glDrawBuffers(1, &Target);
				glViewport((GLuint)IResource->Viewarea.TopLeftX, (GLuint)IResource->Viewarea.TopLeftY, (GLuint)IResource->Viewarea.Width, (GLuint)IResource->Viewarea.Height);
			}
			void OGLDevice::SetTarget(Graphics::RenderTarget* Resource, unsigned int Target, float R, float G, float B)
			{
				TH_ASSERT_V(Resource != nullptr, "resource should be set");
				TH_ASSERT_V(Target < Resource->GetTargetCount(), "targets count should be less than %i", (int)Resource->GetTargetCount());

				OGLFrameBuffer* TargetBuffer = (OGLFrameBuffer*)Resource->GetTargetBuffer();
				const Viewport& Viewarea = Resource->GetViewport();

				TH_ASSERT_V(TargetBuffer != nullptr, "target buffer should be set");
				if (!TargetBuffer->Backbuffer)
				{
					GLenum Targets[8] = { GL_NONE };
					Targets[Target] = TargetBuffer->Format[Target];

					glBindFramebuffer(GL_FRAMEBUFFER, TargetBuffer->Buffer);
					glDrawBuffers(Resource->GetTargetCount(), Targets);

				}
				else
				{
					glBindFramebuffer(GL_FRAMEBUFFER, 0);
					glDrawBuffer(GL_FRONT_AND_BACK);
				}
				
				glViewport((GLuint)Viewarea.TopLeftX, (GLuint)Viewarea.TopLeftY, (GLuint)Viewarea.Width, (GLuint)Viewarea.Height);
				glClearColor(R, G, B, 1.0f);
				glClear(GL_COLOR_BUFFER_BIT);
			}
			void OGLDevice::SetTarget(Graphics::RenderTarget* Resource, unsigned int Target)
			{
				TH_ASSERT_V(Resource != nullptr, "resource should be set");
				TH_ASSERT_V(Target < Resource->GetTargetCount(), "targets count should be less than %i", (int)Resource->GetTargetCount());

				OGLFrameBuffer* TargetBuffer = (OGLFrameBuffer*)Resource->GetTargetBuffer();
				const Viewport& Viewarea = Resource->GetViewport();

				TH_ASSERT_V(TargetBuffer != nullptr, "target buffer should be set");
				if (!TargetBuffer->Backbuffer)
				{
					GLenum Targets[8] = { GL_NONE };
					Targets[Target] = TargetBuffer->Format[Target];

					glBindFramebuffer(GL_FRAMEBUFFER, TargetBuffer->Buffer);
					glDrawBuffers(Resource->GetTargetCount(), Targets);

				}
				else
				{
					glBindFramebuffer(GL_FRAMEBUFFER, 0);
					glDrawBuffer(GL_FRONT_AND_BACK);
				}

				glViewport((GLuint)Viewarea.TopLeftX, (GLuint)Viewarea.TopLeftY, (GLuint)Viewarea.Width, (GLuint)Viewarea.Height);
			}
			void OGLDevice::SetTarget(Graphics::RenderTarget* Resource, float R, float G, float B)
			{
				TH_ASSERT_V(Resource != nullptr, "resource should be set");
				OGLFrameBuffer* TargetBuffer = (OGLFrameBuffer*)Resource->GetTargetBuffer();
				const Viewport& Viewarea = Resource->GetViewport();

				TH_ASSERT_V(TargetBuffer != nullptr, "target buffer should be set");
				if (!TargetBuffer->Backbuffer)
				{
					glBindFramebuffer(GL_FRAMEBUFFER, TargetBuffer->Buffer);
					glDrawBuffers(Resource->GetTargetCount(), TargetBuffer->Format);

				}
				else
				{
					glBindFramebuffer(GL_FRAMEBUFFER, 0);
					glDrawBuffer(GL_FRONT_AND_BACK);
				}

				glViewport((GLuint)Viewarea.TopLeftX, (GLuint)Viewarea.TopLeftY, (GLuint)Viewarea.Width, (GLuint)Viewarea.Height);
				glClearColor(R, G, B, 1.0f);
				glClear(GL_COLOR_BUFFER_BIT);
			}
			void OGLDevice::SetTarget(Graphics::RenderTarget* Resource)
			{
				TH_ASSERT_V(Resource != nullptr, "resource should be set");
				OGLFrameBuffer* TargetBuffer = (OGLFrameBuffer*)Resource->GetTargetBuffer();
				const Viewport& Viewarea = Resource->GetViewport();

				TH_ASSERT_V(TargetBuffer != nullptr, "target buffer should be set");
				if (!TargetBuffer->Backbuffer)
				{
					glBindFramebuffer(GL_FRAMEBUFFER, TargetBuffer->Buffer);
					glDrawBuffers(Resource->GetTargetCount(), TargetBuffer->Format);

				}
				else
				{
					glBindFramebuffer(GL_FRAMEBUFFER, 0);
					glDrawBuffer(GL_FRONT_AND_BACK);
				}

				glViewport((GLuint)Viewarea.TopLeftX, (GLuint)Viewarea.TopLeftY, (GLuint)Viewarea.Width, (GLuint)Viewarea.Height);
			}
			void OGLDevice::SetTargetMap(Graphics::RenderTarget* Resource, bool Enabled[8])
			{
				TH_ASSERT_V(Resource != nullptr, "resource should be set");
				OGLFrameBuffer* TargetBuffer = (OGLFrameBuffer*)Resource->GetTargetBuffer();
				const Viewport& Viewarea = Resource->GetViewport();

				TH_ASSERT_V(TargetBuffer != nullptr, "target buffer should be set");
				if (!TargetBuffer->Backbuffer)
				{
					GLenum Targets[8] = { GL_NONE };
					GLuint Count = Resource->GetTargetCount();

					for (unsigned int i = 0; i < Count; i++)
						Targets[i] = (Enabled[i] ? TargetBuffer->Format[i] : GL_NONE);

					glBindFramebuffer(GL_FRAMEBUFFER, TargetBuffer->Buffer);
					glDrawBuffers(Count, Targets);

				}
				else
				{
					glBindFramebuffer(GL_FRAMEBUFFER, 0);
					glDrawBuffer(GL_FRONT_AND_BACK);
				}

				glViewport((GLuint)Viewarea.TopLeftX, (GLuint)Viewarea.TopLeftY, (GLuint)Viewarea.Width, (GLuint)Viewarea.Height);
			}
			void OGLDevice::SetTargetRect(unsigned int Width, unsigned int Height)
			{
				TH_ASSERT_V(Width > 0 && Height > 0, "width and height should be greater than zero");
				glBindFramebuffer(GL_FRAMEBUFFER, 0);
				glViewport(0, 0, Width, Height);
			}
			void OGLDevice::SetViewports(unsigned int Count, Viewport* Value)
			{
				TH_ASSERT_V(Count > 0 && Value != nullptr, "at least one viewport should be set");
				glViewport(Value[0].TopLeftX, Value[0].TopLeftY, Value[0].Width, Value[0].Height);
			}
			void OGLDevice::SetScissorRects(unsigned int Count, Compute::Rectangle* Value)
			{
				TH_ASSERT_V(Count > 0 && Value != nullptr, "at least one scissor rect should be set");
				glScissor(Value[0].Left, Value[0].Top, Value[0].Right - Value[0].Left, Value[0].Top - Value[0].Bottom);
			}
			void OGLDevice::SetPrimitiveTopology(PrimitiveTopology _Topology)
			{
				glPolygonMode(GL_FRONT_AND_BACK, GetPrimitiveTopology(_Topology));
				Primitive = _Topology;
			}
			void OGLDevice::FlushTexture2D(unsigned int Slot, unsigned int Count, unsigned int Type)
			{
				TH_ASSERT_V(Count < 32, "count should be less than 32");
				for (unsigned int i = 0; i < Count; i++)
				{
					glActiveTexture(GL_TEXTURE0 + Slot + i);
					glEnable(GL_TEXTURE_2D);
					glBindTexture(GL_TEXTURE_2D, 0);
				}
			}
			void OGLDevice::FlushTexture3D(unsigned int Slot, unsigned int Count, unsigned int Type)
			{
				TH_ASSERT_V(Count < 32, "count should be less than 32");
				for (unsigned int i = 0; i < Count; i++)
				{
					glActiveTexture(GL_TEXTURE0 + Slot + i);
					glEnable(GL_TEXTURE_3D);
					glBindTexture(GL_TEXTURE_3D, 0);
				}
			}
			void OGLDevice::FlushTextureCube(unsigned int Slot, unsigned int Count, unsigned int Type)
			{
				TH_ASSERT_V(Count < 32, "count should be less than 32");
				for (unsigned int i = 0; i < Count; i++)
				{
					glActiveTexture(GL_TEXTURE0 + Slot + i);
					glEnable(GL_TEXTURE_CUBE_MAP);
					glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
				}
			}
			void OGLDevice::FlushState()
			{
			}
			bool OGLDevice::Map(ElementBuffer* Resource, ResourceMap Mode, MappedSubresource* Map)
			{
				TH_ASSERT(Resource != nullptr, false, "resource should be set");
				OGLElementBuffer* IResource = (OGLElementBuffer*)Resource;
				glBindBuffer(IResource->Flags, IResource->Resource);

				GLint Size;
				glGetBufferParameteriv(IResource->Flags, GL_BUFFER_SIZE, &Size);
				Map->Pointer = glMapBuffer(IResource->Flags, OGLDevice::GetResourceMap(Mode));
				Map->RowPitch = Size;
				Map->DepthPitch = 1;

				return Map->Pointer != nullptr;
			}
			bool OGLDevice::Unmap(ElementBuffer* Resource, MappedSubresource* Map)
			{
				TH_ASSERT(Resource != nullptr, false, "resource should be set");
				OGLElementBuffer* IResource = (OGLElementBuffer*)Resource;
				glBindBuffer(IResource->Flags, IResource->Resource);
				glUnmapBuffer(IResource->Flags);
				return true;
			}
			bool OGLDevice::UpdateBuffer(ElementBuffer* Resource, void* Data, uint64_t Size)
			{
				TH_ASSERT(Resource != nullptr, false, "resource should be set");
				OGLElementBuffer* IResource = (OGLElementBuffer*)Resource;
				glBindBuffer(IResource->Flags, IResource->Resource);
				glBufferData(IResource->Flags, (GLsizeiptr)Size, Data, GL_DYNAMIC_DRAW);
				return true;
			}
			bool OGLDevice::UpdateBuffer(Shader* Resource, const void* Data)
			{
				TH_ASSERT(Resource != nullptr, false, "resource should be set");
				OGLShader* IResource = (OGLShader*)Resource;
				CopyConstantBuffer(IResource->ConstantBuffer, (void*)Data, IResource->ConstantSize);
				return true;
			}
			bool OGLDevice::UpdateBuffer(MeshBuffer* Resource, Compute::Vertex* Data)
			{
				TH_ASSERT(Resource != nullptr, false, "resource should be set");
				TH_ASSERT(Data != nullptr, false, "data should be set");

				OGLMeshBuffer* IResource = (OGLMeshBuffer*)Resource;
				MappedSubresource MappedResource;
				if (!Map(IResource->VertexBuffer, ResourceMap::Write, &MappedResource))
					return false;

				memcpy(MappedResource.Pointer, Data, (size_t)IResource->VertexBuffer->GetElements() * sizeof(Compute::Vertex));
				return Unmap(IResource->VertexBuffer, &MappedResource);
			}
			bool OGLDevice::UpdateBuffer(SkinMeshBuffer* Resource, Compute::SkinVertex* Data)
			{
				TH_ASSERT(Resource != nullptr, false, "resource should be set");
				TH_ASSERT(Data != nullptr, false, "data should be set");

				OGLSkinMeshBuffer* IResource = (OGLSkinMeshBuffer*)Resource;
				MappedSubresource MappedResource;
				if (!Map(IResource->VertexBuffer, ResourceMap::Write, &MappedResource))
					return false;

				memcpy(MappedResource.Pointer, Data, (size_t)IResource->VertexBuffer->GetElements() * sizeof(Compute::SkinVertex));
				return Unmap(IResource->VertexBuffer, &MappedResource);
			}
			bool OGLDevice::UpdateBuffer(InstanceBuffer* Resource)
			{
				TH_ASSERT(Resource != nullptr, false, "resource should be set");
				OGLInstanceBuffer* IResource = (OGLInstanceBuffer*)Resource;
				if (IResource->Array.Size() <= 0 || IResource->Array.Size() > IResource->ElementLimit)
					return false;

				OGLElementBuffer* Element = (OGLElementBuffer*)IResource->Elements;
				glBindBuffer(Element->Flags, Element->Resource);
				GLvoid* Data = glMapBuffer(Element->Flags, GL_WRITE_ONLY);
				memcpy(Data, IResource->Array.Get(), (size_t)IResource->Array.Size() * IResource->ElementWidth);
				glUnmapBuffer(Element->Flags);
				return true;
			}
			bool OGLDevice::UpdateBuffer(RenderBufferType Buffer)
			{
				CopyConstantBuffer(ConstantBuffer[(size_t)Buffer], &Constants[(size_t)Buffer], ConstantSize[(size_t)Buffer]);
				return true;
			}
			bool OGLDevice::UpdateBufferSize(Shader* Resource, size_t Size)
			{
				TH_ASSERT(Resource != nullptr, false, "resource should be set");
				TH_ASSERT(Size > 0, false, "size should be greater than zero");

				OGLShader* IResource = (OGLShader*)Resource;
				if (IResource->ConstantBuffer != GL_NONE)
					glDeleteBuffers(1, &IResource->ConstantBuffer);

				return CreateConstantBuffer(&IResource->ConstantBuffer, Size) >= 0;
			}
			bool OGLDevice::UpdateBufferSize(InstanceBuffer* Resource, uint64_t Size)
			{
				TH_ASSERT(Resource != nullptr, false, "resource should be set");
				TH_ASSERT(Size > 0, false, "size should be greater than zero");

				OGLInstanceBuffer* IResource = (OGLInstanceBuffer*)Resource;
				ClearBuffer(IResource);
				TH_RELEASE(IResource->Elements);
				IResource->ElementLimit = Size;
				IResource->Array.Clear();
				IResource->Array.Reserve(IResource->ElementLimit);

				ElementBuffer::Desc F = ElementBuffer::Desc();
				F.AccessFlags = CPUAccess::Write;
				F.MiscFlags = ResourceMisc::Buffer_Structured;
				F.Usage = ResourceUsage::Dynamic;
				F.BindFlags = ResourceBind::Shader_Input;
				F.ElementCount = (unsigned int)IResource->ElementLimit;
				F.ElementWidth = (unsigned int)IResource->ElementWidth;
				F.StructureByteStride = F.ElementWidth;

				IResource->Elements = CreateElementBuffer(F);
				return true;
			}
			void OGLDevice::ClearBuffer(InstanceBuffer* Resource)
			{
				TH_ASSERT_V(Resource != nullptr, "resource should be set");
				OGLInstanceBuffer* IResource = (OGLInstanceBuffer*)Resource;
				if (!IResource->Sync)
					return;

				OGLElementBuffer* Element = (OGLElementBuffer*)IResource->Elements;
				glBindBuffer(Element->Flags, Element->Resource);
				GLvoid* Data = glMapBuffer(Element->Flags, GL_WRITE_ONLY);
				memset(Data, 0, IResource->ElementWidth * IResource->ElementLimit);
				glUnmapBuffer(Element->Flags);
			}
			void OGLDevice::ClearWritable(Texture2D* Resource)
			{
				ClearWritable(Resource, 0.0f, 0.0f, 0.0f);
			}
			void OGLDevice::ClearWritable(Texture2D* Resource, float R, float G, float B)
			{
				TH_ASSERT_V(Resource != nullptr, "resource should be set");
				OGLTexture2D* IResource = (OGLTexture2D*)Resource;
				GLfloat ClearColor[4] = { R, G, B, 0.0f };
				GLint PrevHandle = 0;

				glGetIntegerv(GL_TEXTURE_BINDING_2D, &PrevHandle);
				glBindTexture(GL_TEXTURE_2D, IResource->Resource);
				glClearTexImage(IResource->Resource, 0, IResource->Format, GL_FLOAT, &ClearColor);
				glBindTexture(GL_TEXTURE_2D, PrevHandle);
			}
			void OGLDevice::ClearWritable(Texture3D* Resource)
			{
				ClearWritable(Resource, 0.0f, 0.0f, 0.0f);
			}
			void OGLDevice::ClearWritable(Texture3D* Resource, float R, float G, float B)
			{
				TH_ASSERT_V(Resource != nullptr, "resource should be set");
				OGLTexture3D* IResource = (OGLTexture3D*)Resource;
				GLfloat ClearColor[4] = { R, G, B, 0.0f };
				GLint PrevHandle = 0;

				glGetIntegerv(GL_TEXTURE_BINDING_3D, &PrevHandle);
				glBindTexture(GL_TEXTURE_3D, IResource->Resource);
				glClearTexImage(IResource->Resource, 0, IResource->Format, GL_FLOAT, &ClearColor);
				glBindTexture(GL_TEXTURE_3D, PrevHandle);
			}
			void OGLDevice::ClearWritable(TextureCube* Resource)
			{
				ClearWritable(Resource, 0.0f, 0.0f, 0.0f);
			}
			void OGLDevice::ClearWritable(TextureCube* Resource, float R, float G, float B)
			{
				TH_ASSERT_V(Resource != nullptr, "resource should be set");
				OGLTextureCube* IResource = (OGLTextureCube*)Resource;
				GLfloat ClearColor[4] = { R, G, B, 0.0f };
				GLint PrevHandle = 0;

				glGetIntegerv(GL_TEXTURE_BINDING_CUBE_MAP, &PrevHandle);
				glBindTexture(GL_TEXTURE_CUBE_MAP, IResource->Resource);
				glClearTexImage(IResource->Resource, 0, IResource->Format, GL_FLOAT, &ClearColor);
				glBindTexture(GL_TEXTURE_CUBE_MAP, PrevHandle);
			}
			void OGLDevice::Clear(float R, float G, float B)
			{
				Clear(RenderTarget, 0, R, G, B);
			}
			void OGLDevice::Clear(Graphics::RenderTarget* Resource, unsigned int Target, float R, float G, float B)
			{
				TH_ASSERT_V(Resource != nullptr, "resource should be set");
				glClearColor(R, G, B, 1.0f);
				glClear(GL_COLOR_BUFFER_BIT);
			}
			void OGLDevice::ClearDepth()
			{
				ClearDepth(RenderTarget);
			}
			void OGLDevice::ClearDepth(DepthBuffer* Resource)
			{
				glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
			}
			void OGLDevice::ClearDepth(Graphics::RenderTarget* Resource)
			{
				TH_ASSERT_V(Resource != nullptr, "resource should be set");
				glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
			}
			void OGLDevice::DrawIndexed(unsigned int Count, unsigned int IndexLocation, unsigned int BaseLocation)
			{
				glDrawElements(GetPrimitiveTopologyDraw(Primitive), Count, IndexType, &IndexLocation);
			}
			void OGLDevice::DrawIndexed(MeshBuffer* Resource)
			{
				TH_ASSERT_V(Resource != nullptr, "resource should be set");
				ElementBuffer* IndexBuffer = Resource->GetIndexBuffer();
				SetVertexBuffer(Resource->GetVertexBuffer(), 0);
				SetIndexBuffer(IndexBuffer, Format::R32_Uint);

				glDrawElements(GetPrimitiveTopologyDraw(Primitive), IndexBuffer->GetElements(), GL_UNSIGNED_INT, nullptr);
			}
			void OGLDevice::DrawIndexed(SkinMeshBuffer* Resource)
			{
				TH_ASSERT_V(Resource != nullptr, "resource should be set");
				ElementBuffer* IndexBuffer = Resource->GetIndexBuffer();
				SetVertexBuffer(Resource->GetVertexBuffer(), 0);
				SetIndexBuffer(IndexBuffer, Format::R32_Uint);

				glDrawElements(GetPrimitiveTopologyDraw(Primitive), IndexBuffer->GetElements(), GL_UNSIGNED_INT, nullptr);
			}
			void OGLDevice::Draw(unsigned int Count, unsigned int Location)
			{
				glDrawArrays(GetPrimitiveTopologyDraw(Primitive), (GLint)Location, (GLint)Count);
			}
			void OGLDevice::Dispatch(unsigned int GroupX, unsigned int GroupY, unsigned int GroupZ)
			{
				glDispatchCompute(GroupX, GroupY, GroupZ);
			}
			bool OGLDevice::CopyTexture2D(Texture2D* Resource, Texture2D** Result)
			{
				TH_ASSERT(Resource != nullptr, false, "resource should be set");
				TH_ASSERT(Result != nullptr, false, "result should be set");

				OGLTexture2D* IResource = (OGLTexture2D*)Resource;

				TH_ASSERT(IResource->Resource != GL_NONE, false, "resource should be valid");
				int Width, Height;
				glBindTexture(GL_TEXTURE_2D, IResource->Resource);
				glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &Width);
				glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &Height);

				if (!*Result)
				{
					Texture2D::Desc F;
					F.Width = (unsigned int)Width;
					F.Height = (unsigned int)Height;
					F.RowPitch = (Width * 32 + 7) / 8;
					F.DepthPitch = F.RowPitch * Height;
					F.MipLevels = GetMipLevel(F.Width, F.Height);
					F.Data = (void*)TH_MALLOC(sizeof(char) * F.Width * F.Height);

					glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, F.Data);
					*Result = (OGLTexture2D*)CreateTexture2D(F);
					TH_FREE(F.Data);
				}
				else
				{
					GLuint Buffers[2];
					glGenFramebuffers(2, Buffers);
					glBindFramebuffer(GL_READ_FRAMEBUFFER, Buffers[0]);
					glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, IResource->Resource, 0);
					glBindFramebuffer(GL_DRAW_FRAMEBUFFER, Buffers[1]);
					glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ((OGLTexture2D*)(*Result))->Resource, 0);
					glBlitFramebuffer(0, 0, Width, Height, 0, 0, Width, Height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
					glDeleteFramebuffers(2, Buffers);
				}

				return true;
			}
			bool OGLDevice::CopyTexture2D(Graphics::RenderTarget* Resource, unsigned int Target, Texture2D** Result)
			{
				TH_ASSERT(Resource != nullptr, false, "resource should be set");
				TH_ASSERT(Result != nullptr, false, "result should be set");

				OGLFrameBuffer* TargetBuffer = (OGLFrameBuffer*)Resource->GetTargetBuffer();

				TH_ASSERT(TargetBuffer != nullptr, false, "target buffer should be set");
				if (TargetBuffer->Backbuffer)
				{
					if (!*Result)
					{
						*Result = (OGLTexture2D*)CreateTexture2D();
						glGenTextures(1, &((OGLTexture2D*)(*Result))->Resource);
					}

					GLint Viewport[4];
					glGetIntegerv(GL_VIEWPORT, Viewport);
					glBindTexture(GL_TEXTURE_2D, ((OGLTexture2D*)(*Result))->Resource);
					glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, (GLsizei)Viewport[2], (GLsizei)Viewport[3]);
					return true;
				}

				OGLTexture2D* Source = (OGLTexture2D*)Resource->GetTarget(Target);
				if (!Source)
					return false;

				int Width, Height;
				glBindTexture(GL_TEXTURE_2D, Source->Resource);
				glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &Width);
				glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &Height);

				if (!*Result)
				{
					Texture2D::Desc F;
					F.Width = (unsigned int)Width;
					F.Height = (unsigned int)Height;
					F.RowPitch = (Width * 32 + 7) / 8;
					F.DepthPitch = F.RowPitch * Height;
					F.MipLevels = GetMipLevel(F.Width, F.Height);
					F.Data = (void*)TH_MALLOC(sizeof(char) * F.Width * F.Height);

					glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, F.Data);
					*Result = (OGLTexture2D*)CreateTexture2D(F);
					TH_FREE(F.Data);
				}
				else
				{
					GLuint Buffers[2];
					glGenFramebuffers(2, Buffers);
					glBindFramebuffer(GL_READ_FRAMEBUFFER, Buffers[0]);
					glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, Source->Resource, 0);
					glBindFramebuffer(GL_DRAW_FRAMEBUFFER, Buffers[1]);
					glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ((OGLTexture2D*)(*Result))->Resource, 0);
					glBlitFramebuffer(0, 0, Width, Height, 0, 0, Width, Height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
					glDeleteFramebuffers(2, Buffers);
				}

				return true;
			}
			bool OGLDevice::CopyTexture2D(RenderTargetCube* Resource, unsigned int Face, Texture2D** Result)
			{
				TH_ASSERT(Resource != nullptr, false, "resource should be set");
				TH_ASSERT(Result != nullptr, false, "result should be set");
				TH_ASSERT(Face < 6, false, "face index should be less than 6");

				OGLRenderTargetCube* IResource = (OGLRenderTargetCube*)Resource;
				int Width, Height;
				glBindTexture(GL_TEXTURE_CUBE_MAP, IResource->FrameBuffer.Texture[0]);
				glGetTexLevelParameteriv(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_TEXTURE_WIDTH, &Width);
				glGetTexLevelParameteriv(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_TEXTURE_HEIGHT, &Height);

				if (!*Result)
				{
					Texture2D::Desc F;
					F.Width = (unsigned int)Width;
					F.Height = (unsigned int)Height;
					F.RowPitch = (Width * 32 + 7) / 8;
					F.DepthPitch = F.RowPitch * Height;
					F.MipLevels = GetMipLevel(F.Width, F.Height);
					F.Data = (void*)TH_MALLOC(sizeof(char) * F.Width * F.Height);

					glGetTexImage(GL_TEXTURE_CUBE_MAP_POSITIVE_X + Face, 0, GL_RGBA, GL_UNSIGNED_BYTE, F.Data);
					*Result = (OGLTexture2D*)CreateTexture2D(F);
					TH_FREE(F.Data);
				}
				else
				{
					GLuint Buffers[2];
					glGenFramebuffers(2, Buffers);
					glBindFramebuffer(GL_READ_FRAMEBUFFER, Buffers[0]);
					glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + Face, IResource->FrameBuffer.Texture[0], 0);
					glBindFramebuffer(GL_DRAW_FRAMEBUFFER, Buffers[1]);
					glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ((OGLTexture2D*)(*Result))->Resource, 0);
					glBlitFramebuffer(0, 0, Width, Height, 0, 0, Width, Height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
					glDeleteFramebuffers(2, Buffers);
				}

				return true;
			}
			bool OGLDevice::CopyTexture2D(MultiRenderTargetCube* Resource, unsigned int Cube, unsigned int Face, Texture2D** Result)
			{
				TH_ASSERT(Resource != nullptr, false, "resource should be set");
				TH_ASSERT(Result != nullptr, false, "result should be set");
				TH_ASSERT(Face < 6, false, "face index should be less than 6");

				OGLMultiRenderTargetCube* IResource = (OGLMultiRenderTargetCube*)Resource;

				TH_ASSERT(Cube < (uint32_t)IResource->Target, false, "cube index should be less than %i", (int)IResource->Target);
				int Width, Height;
				glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &Width);
				glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &Height);

				if (!*Result)
				{
					Texture2D::Desc F;
					F.Width = (unsigned int)Width;
					F.Height = (unsigned int)Height;
					F.RowPitch = (Width * 32 + 7) / 8;
					F.DepthPitch = F.RowPitch * Height;
					F.MipLevels = GetMipLevel(F.Width, F.Height);
					F.Data = (void*)TH_MALLOC(sizeof(char) * F.Width * F.Height);

					glBindTexture(GL_TEXTURE_CUBE_MAP, IResource->FrameBuffer.Texture[Cube]);
					glGetTexImage(GL_TEXTURE_CUBE_MAP_POSITIVE_X + Face, 0, GL_RGBA, GL_UNSIGNED_BYTE, F.Data);

					*Result = (OGLTexture2D*)CreateTexture2D(F);
					TH_FREE(F.Data);
				}
				else
				{
					GLuint Buffers[2];
					glGenFramebuffers(2, Buffers);
					glBindFramebuffer(GL_READ_FRAMEBUFFER, Buffers[0]);
					glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + Face, IResource->FrameBuffer.Texture[Cube], 0);
					glBindFramebuffer(GL_DRAW_FRAMEBUFFER, Buffers[1]);
					glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ((OGLTexture2D*)(*Result))->Resource, 0);
					glBlitFramebuffer(0, 0, Width, Height, 0, 0, Width, Height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
					glDeleteFramebuffers(2, Buffers);
				}

				return true;
			}
			bool OGLDevice::CopyTextureCube(RenderTargetCube* Resource, TextureCube** Result)
			{
				TH_ASSERT(Resource != nullptr, false, "resource should be set");
				TH_ASSERT(Result != nullptr, false, "result should be set");

				OGLRenderTargetCube* IResource = (OGLRenderTargetCube*)Resource;
				int Width, Height;
				glBindTexture(GL_TEXTURE_CUBE_MAP, IResource->FrameBuffer.Texture[0]);
				glGetTexLevelParameteriv(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_TEXTURE_WIDTH, &Width);
				glGetTexLevelParameteriv(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_TEXTURE_HEIGHT, &Height);

				if (*Result != nullptr)
				{
					GLuint Buffers[2];
					glGenFramebuffers(2, Buffers);

					for (unsigned int i = 0; i < 6; i++)
					{
						glBindFramebuffer(GL_READ_FRAMEBUFFER, Buffers[0]);
						glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, IResource->FrameBuffer.Texture[0], 0);
						glBindFramebuffer(GL_DRAW_FRAMEBUFFER, Buffers[1]);
						glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, ((OGLTextureCube*)(*Result))->Resource, 0);
						glBlitFramebuffer(0, 0, Width, Height, 0, 0, Width, Height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
					}

					glDeleteFramebuffers(2, Buffers);
				}
				else
				{
					Texture2D* Faces[6] = { nullptr };
					for (unsigned int i = 0; i < 6; i++)
						CopyTexture2D(Resource, i, &Faces[i]);

					*Result = (OGLTextureCube*)CreateTextureCube(Faces);
					for (unsigned int i = 0; i < 6; i++)
						TH_RELEASE(Faces[i]);
				}

				return true;
			}
			bool OGLDevice::CopyTextureCube(MultiRenderTargetCube* Resource, unsigned int Cube, TextureCube** Result)
			{
				TH_ASSERT(Resource != nullptr, false, "resource should be set");
				TH_ASSERT(Result != nullptr, false, "result should be set");

				OGLMultiRenderTargetCube* IResource = (OGLMultiRenderTargetCube*)Resource;

				TH_ASSERT(Cube < (uint32_t)IResource->Target, false, "cube index should be less than %i", (int)IResource->Target);
				int Width, Height;
				glBindTexture(GL_TEXTURE_CUBE_MAP, IResource->FrameBuffer.Texture[Cube]);
				glGetTexLevelParameteriv(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_TEXTURE_WIDTH, &Width);
				glGetTexLevelParameteriv(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_TEXTURE_HEIGHT, &Height);

				if (*Result != nullptr)
				{
					GLuint Buffers[2];
					glGenFramebuffers(2, Buffers);

					for (unsigned int i = 0; i < 6; i++)
					{
						glBindFramebuffer(GL_READ_FRAMEBUFFER, Buffers[0]);
						glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, IResource->FrameBuffer.Texture[Cube], 0);
						glBindFramebuffer(GL_DRAW_FRAMEBUFFER, Buffers[1]);
						glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, ((OGLTextureCube*)(*Result))->Resource, 0);
						glBlitFramebuffer(0, 0, Width, Height, 0, 0, Width, Height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
					}

					glDeleteFramebuffers(2, Buffers);
				}
				else
				{
					Texture2D* Faces[6] = { nullptr };
					for (unsigned int i = 0; i < 6; i++)
						CopyTexture2D(Resource, Cube, i, &Faces[i]);

					*Result = (OGLTextureCube*)CreateTextureCube(Faces);
					for (unsigned int i = 0; i < 6; i++)
						TH_RELEASE(Faces[i]);
				}

				return true;
			}
			bool OGLDevice::CopyTarget(Graphics::RenderTarget* From, unsigned int FromTarget, Graphics::RenderTarget* To, unsigned int ToTarget)
			{
				TH_ASSERT(From != nullptr && To != nullptr, false, "from and to should be set");
				OGLTexture2D* Source = (OGLTexture2D*)From->GetTarget(FromTarget);
				OGLTexture2D* Dest = (OGLTexture2D*)To->GetTarget(ToTarget);

				TH_ASSERT(Source != nullptr && Source->Resource != GL_NONE && Dest != nullptr && Dest->Resource != GL_NONE, false, "src and dest should be valid");
				uint32_t Width = From->GetWidth();
				uint32_t Height = From->GetHeight();

				GLuint Buffers[2];
				glGenFramebuffers(2, Buffers);
				glBindFramebuffer(GL_READ_FRAMEBUFFER, Buffers[0]);
				glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, Source->Resource, 0);
				glBindFramebuffer(GL_DRAW_FRAMEBUFFER, Buffers[1]);
				glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, Dest->Resource, 0);
				glBlitFramebuffer(0, 0, Width, Height, 0, 0, Width, Height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
				glDeleteFramebuffers(2, Buffers);

				return true;
			}
			bool OGLDevice::CubemapBegin(Cubemap* Resource)
			{
				TH_ASSERT(Resource != nullptr, false, "resource should be set");
				TH_ASSERT(Resource->IsValid(), false, "resource should be valid");

				OGLCubemap* IResource = (OGLCubemap*)Resource;
				glBindTexture(GL_TEXTURE_CUBE_MAP, IResource->Resource);
				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

				if (IResource->Meta.MipLevels > 0)
				{
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, IResource->Meta.MipLevels - 1);
				}
				else
				{
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				}

				return true;
			}
			bool OGLDevice::CubemapFace(Cubemap* Resource, unsigned int Target, unsigned int Face)
			{
				TH_ASSERT(Resource != nullptr, false, "resource should be set");
				TH_ASSERT(Resource->IsValid(), false, "resource should be valid");
				TH_ASSERT(Face < 6, false, "face index should be less than 6");

				OGLCubemap* IResource = (OGLCubemap*)Resource;
				OGLTexture2D* Source = (OGLTexture2D*)IResource->Meta.Source->GetTarget(Target);
				glBindFramebuffer(GL_READ_FRAMEBUFFER, IResource->Buffers[0]);
				glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + Face, Source->Resource, 0);
				glBindFramebuffer(GL_DRAW_FRAMEBUFFER, IResource->Buffers[1]);
				glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + Face, IResource->Resource, 0);
				glBlitFramebuffer(0, 0, IResource->Meta.Size, IResource->Meta.Size, 0, 0, IResource->Meta.Size, IResource->Meta.Size, GL_COLOR_BUFFER_BIT, GL_NEAREST);
				
				return true;
			}
			bool OGLDevice::CubemapEnd(Cubemap* Resource, TextureCube* Result)
			{
				TH_ASSERT(Resource != nullptr, false, "resource should be set");
				TH_ASSERT(Resource->IsValid(), false, "resource should be valid");
				TH_ASSERT(Result != nullptr, false, "result should be set");

				OGLCubemap* IResource = (OGLCubemap*)Resource;
				OGLTextureCube* IResult = (OGLTextureCube*)Result;
				if (!IResult->Resource)
					glGenTextures(1, &IResult->Resource);

				glBindFramebuffer(GL_READ_FRAMEBUFFER, IResource->Buffers[0]);
				glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, IResource->Resource, 0);
				glBindFramebuffer(GL_DRAW_FRAMEBUFFER, IResource->Buffers[1]);
				glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, IResult->Resource, 0);
				glBlitFramebuffer(0, 0, IResource->Meta.Size, IResource->Meta.Size, 0, 0, IResource->Meta.Size, IResource->Meta.Size, GL_COLOR_BUFFER_BIT, GL_NEAREST);

				return true;
			}
			bool OGLDevice::CopyBackBuffer(Texture2D** Result)
			{
				TH_ASSERT(Result != nullptr, false, "result should be set");
				OGLTexture2D* Texture = (OGLTexture2D*)(*Result ? *Result : CreateTexture2D());
				if (!*Result)
					glGenTextures(1, &Texture->Resource);

				GLint Viewport[4];
				glGetIntegerv(GL_VIEWPORT, Viewport);
				glBindTexture(GL_TEXTURE_2D, Texture->Resource);
				glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, (GLsizei)Viewport[2], (GLsizei)Viewport[3]);

				return GenerateTexture(Texture);
			}
			bool OGLDevice::CopyBackBufferMSAA(Texture2D** Result)
			{
				return CopyBackBuffer(Result);
			}
			bool OGLDevice::CopyBackBufferNoAA(Texture2D** Result)
			{
				return CopyBackBufferMSAA(Result);
			}
			void OGLDevice::GetViewports(unsigned int* Count, Viewport* Out)
			{
				GLint Viewport[4];
				glGetIntegerv(GL_VIEWPORT, Viewport);
				if (Count != nullptr)
					*Count = 1;

				if (!Out)
					return;

				Out[0].TopLeftX = Viewport[0];
				Out[0].TopLeftY = Viewport[1];
				Out[0].Width = Viewport[2];
				Out[0].Height = Viewport[3];
			}
			void OGLDevice::GetScissorRects(unsigned int* Count, Compute::Rectangle* Out)
			{
				GLint Rect[4];
				glGetIntegerv(GL_SCISSOR_BOX, Rect);
				if (Count != nullptr)
					*Count = 1;

				if (!Out)
					return;

				Out[0].Left = Rect[0];
				Out[0].Right = Rect[2] + Rect[0];
				Out[0].Top = Rect[1];
				Out[0].Bottom = Rect[1] - Rect[3];
			}
			bool OGLDevice::ResizeBuffers(unsigned int Width, unsigned int Height)
			{
				RenderTarget2D::Desc F = RenderTarget2D::Desc();
				F.Width = Width;
				F.Height = Height;
				F.MipLevels = 1;
				F.MiscFlags = ResourceMisc::None;
				F.FormatMode = Format::R8G8B8A8_Unorm;
				F.Usage = ResourceUsage::Default;
				F.AccessFlags = CPUAccess::Invalid;
				F.BindFlags = ResourceBind::Render_Target | ResourceBind::Shader_Input;
				F.RenderSurface = (void*)this;

				TH_RELEASE(RenderTarget);
				RenderTarget = CreateRenderTarget2D(F);
				SetTarget();

				return true;
			}
			bool OGLDevice::GenerateTexture(Texture2D* Resource)
			{
				TH_ASSERT(Resource != nullptr, false, "resource should be set");
				OGLTexture2D* IResource = (OGLTexture2D*)Resource;

				TH_ASSERT(IResource->Resource != GL_NONE, false, "resource should be valid");
				int Width, Height;
				glBindTexture(GL_TEXTURE_2D, IResource->Resource);
				glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &Width);
				glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &Height);

				IResource->Width = Width;
				IResource->Height = Height;

				return true;
			}
			bool OGLDevice::GenerateTexture(Texture3D* Resource)
			{
				TH_ASSERT(Resource != nullptr, false, "resource should be set");
				OGLTexture3D* IResource = (OGLTexture3D*)Resource;

				TH_ASSERT(IResource->Resource != GL_NONE, false, "resource should be valid");
				int Width, Height, Depth;
				glBindTexture(GL_TEXTURE_3D, IResource->Resource);
				glGetTexLevelParameteriv(GL_TEXTURE_3D, 0, GL_TEXTURE_WIDTH, &Width);
				glGetTexLevelParameteriv(GL_TEXTURE_3D, 0, GL_TEXTURE_HEIGHT, &Height);
				glGetTexLevelParameteriv(GL_TEXTURE_3D, 0, GL_TEXTURE_DEPTH, &Depth);

				IResource->Width = Width;
				IResource->Height = Height;
				IResource->Depth = Depth;

				return true;
			}
			bool OGLDevice::GenerateTexture(TextureCube* Resource)
			{
				TH_ASSERT(Resource != nullptr, false, "resource should be set");
				OGLTextureCube* IResource = (OGLTextureCube*)Resource;

				TH_ASSERT(IResource->Resource != GL_NONE, false, "resource should be valid");
				int Width, Height;
				glBindTexture(GL_TEXTURE_CUBE_MAP, IResource->Resource);
				glGetTexLevelParameteriv(GL_TEXTURE_CUBE_MAP, 0, GL_TEXTURE_WIDTH, &Width);
				glGetTexLevelParameteriv(GL_TEXTURE_CUBE_MAP, 0, GL_TEXTURE_HEIGHT, &Height);

				IResource->Width = Width;
				IResource->Height = Height;

				return true;
			}
			bool OGLDevice::GetQueryData(Query* Resource, uint64_t* Result, bool Flush)
			{
				TH_ASSERT(Resource != nullptr, false, "resource should be set");
				TH_ASSERT(Result != nullptr, false, "result should be set");

				OGLQuery* IResource = (OGLQuery*)Resource;
				GLint Available = 0;
				glGetQueryObjectiv(IResource->Async, GL_QUERY_RESULT_AVAILABLE, &Available);
				if (Available == GL_FALSE)
					return false;

				GLint64 Data = 0;
				glGetQueryObjecti64v(IResource->Async, GL_QUERY_RESULT, &Data);
				*Result = (uint64_t)Data;

				return true;
			}
			bool OGLDevice::GetQueryData(Query* Resource, bool* Result, bool Flush)
			{
				TH_ASSERT(Resource != nullptr, false, "resource should be set");
				TH_ASSERT(Result != nullptr, false, "result should be set");

				OGLQuery* IResource = (OGLQuery*)Resource;
				GLint Available = 0;
				glGetQueryObjectiv(IResource->Async, GL_QUERY_RESULT_AVAILABLE, &Available);
				if (Available == GL_FALSE)
					return false;

				GLint Data = 0;
				glGetQueryObjectiv(IResource->Async, GL_QUERY_RESULT, &Data);
				*Result = (Data == GL_TRUE);

				return true;
			}
			void OGLDevice::QueryBegin(Query* Resource)
			{
				TH_ASSERT_V(Resource != nullptr, "resource should be set");
				OGLQuery* IResource = (OGLQuery*)Resource;
				glBeginQuery(IResource->Predicate ? GL_ANY_SAMPLES_PASSED : GL_SAMPLES_PASSED, IResource->Async);
			}
			void OGLDevice::QueryEnd(Query* Resource)
			{
				TH_ASSERT_V(Resource != nullptr, "resource should be set");
				OGLQuery* IResource = (OGLQuery*)Resource;
				glEndQuery(IResource->Predicate ? GL_ANY_SAMPLES_PASSED : GL_SAMPLES_PASSED);
			}
			void OGLDevice::GenerateMips(Texture2D* Resource)
			{
				TH_ASSERT_V(Resource != nullptr, "resource should be set");
				OGLTexture2D* IResource = (OGLTexture2D*)Resource;

				TH_ASSERT_V(IResource->Resource != GL_NONE, "resource should be valid");
#ifdef glGenerateTextureMipmap
				glGenerateTextureMipmap(IResource->Resource);
#elif glGenerateMipmap
				glBindTexture(GL_TEXTURE_2D, IResource->Resource);
				glGenerateMipmap(GL_TEXTURE_2D);
#endif
			}
			void OGLDevice::GenerateMips(Texture3D* Resource)
			{
				TH_ASSERT_V(Resource != nullptr, "resource should be set");
				OGLTexture3D* IResource = (OGLTexture3D*)Resource;

				TH_ASSERT_V(IResource->Resource != GL_NONE, "resource should be valid");
#ifdef glGenerateTextureMipmap
				glGenerateTextureMipmap(IResource->Resource);
#endif
			}
			void OGLDevice::GenerateMips(TextureCube* Resource)
			{
				TH_ASSERT_V(Resource != nullptr, "resource should be set");
				OGLTextureCube* IResource = (OGLTextureCube*)Resource;

				TH_ASSERT_V(IResource->Resource != GL_NONE, "resource should be valid");
#ifdef glGenerateTextureMipmap
				glGenerateTextureMipmap(IResource->Resource);
#elif glGenerateMipmap
				glBindTexture(GL_TEXTURE_CUBE_MAP, IResource->Resource);
				glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
#endif
			}
			bool OGLDevice::Begin()
			{
				if (DirectRenderer.VertexBuffer == GL_NONE && !CreateDirectBuffer(0))
					return false;

				Primitives = PrimitiveTopology::Triangle_List;
				Direct.WorldViewProjection = Compute::Matrix4x4::Identity();
				Direct.Padding = { 0, 0, 0, 1 };
				ViewResource = nullptr;

				Elements.clear();
				return true;
			}
			void OGLDevice::Transform(const Compute::Matrix4x4& Transform)
			{
				Direct.WorldViewProjection = Direct.WorldViewProjection * Transform;
			}
			void OGLDevice::Topology(PrimitiveTopology Topology)
			{
				Primitives = Topology;
			}
			void OGLDevice::Emit()
			{
				Elements.push_back({ 0, 0, 0, 0, 0, 1, 1, 1, 1 });
			}
			void OGLDevice::Texture(Texture2D* In)
			{
				ViewResource = In;
				Direct.Padding.Z = 1;
			}
			void OGLDevice::Color(float X, float Y, float Z, float W)
			{
				TH_ASSERT_V(!Elements.empty(), "vertex should already be emitted");
				auto& Element = Elements.back();
				Element.CX = X;
				Element.CY = Y;
				Element.CZ = Z;
				Element.CW = W;
			}
			void OGLDevice::Intensity(float Intensity)
			{
				Direct.Padding.W = Intensity;
			}
			void OGLDevice::TexCoord(float X, float Y)
			{
				TH_ASSERT_V(!Elements.empty(), "vertex should already be emitted");
				auto& Element = Elements.back();
				Element.TX = X;
				Element.TY = Y;
			}
			void OGLDevice::TexCoordOffset(float X, float Y)
			{
				Direct.Padding.X = X;
				Direct.Padding.Y = Y;
			}
			void OGLDevice::Position(float X, float Y, float Z)
			{
				TH_ASSERT_V(!Elements.empty(), "vertex should already be emitted");
				auto& Element = Elements.back();
				Element.PX = X;
				Element.PY = Y;
				Element.PZ = Z;
			}
			bool OGLDevice::End()
			{
				if (DirectRenderer.VertexBuffer == GL_NONE || Elements.empty())
					return false;

				if (Elements.size() > MaxElements && !CreateDirectBuffer(Elements.size()))
					return false;
				
				GLint LastVAO = 0;
				glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &LastVAO);

				GLint LastVBO = 0;
				glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &LastVBO);

				GLint LastProgram = 0;
				glGetIntegerv(GL_CURRENT_PROGRAM, &LastProgram);

				glBindBuffer(GL_ARRAY_BUFFER, DirectRenderer.VertexBuffer);
				void* Data = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
				memcpy(Data, Elements.data(), (size_t)Elements.size() * sizeof(Vertex));
				glUnmapBuffer(GL_ARRAY_BUFFER);

				glUseProgram(DirectRenderer.Program);
				glUniformMatrix4fv(0, 1, GL_FALSE, (const GLfloat*)&Direct.WorldViewProjection.Row);
				glUniform4fARB(1, Direct.Padding.X, Direct.Padding.Y, Direct.Padding.Z, Direct.Padding.W);

				GLint LastTexture = 0;
				if (ViewResource != nullptr)
				{
					OGLTexture2D* IResource = (OGLTexture2D*)ViewResource;
					glActiveTexture(GL_TEXTURE0);
					glGetIntegerv(GL_TEXTURE_BINDING_2D, &LastTexture);
					glBindTexture(GL_TEXTURE_2D, IResource->Resource);
				}

				glBindVertexArray(DirectRenderer.VertexBuffer);
				glDrawArrays(GL_TRIANGLES, 0, (GLsizei)Elements.size());
				glBindVertexArray(LastVAO);
				glBindBuffer(GL_ARRAY_BUFFER, LastVBO);
				glUseProgram(LastProgram);

				if (ViewResource != nullptr)
				{
					glActiveTexture(GL_TEXTURE0);
					glBindTexture(GL_TEXTURE_2D, LastTexture);
				}
				
				return true;
			}
			bool OGLDevice::Submit()
			{
#ifdef TH_HAS_SDL2
				TH_ASSERT(Window != nullptr, false, "window should be set");
				SDL_GL_SwapWindow(Window->GetHandle());
#endif
				return true;
			}
			DepthStencilState* OGLDevice::CreateDepthStencilState(const DepthStencilState::Desc& I)
			{
				return new OGLDepthStencilState(I);
			}
			BlendState* OGLDevice::CreateBlendState(const BlendState::Desc& I)
			{
				return new OGLBlendState(I);
			}
			RasterizerState* OGLDevice::CreateRasterizerState(const RasterizerState::Desc& I)
			{
				return new OGLRasterizerState(I);
			}
			SamplerState* OGLDevice::CreateSamplerState(const SamplerState::Desc& I)
			{
				GLuint DeviceState = 0;
				glGenSamplers(1, &DeviceState);
				glSamplerParameteri(DeviceState, GL_TEXTURE_WRAP_S, OGLDevice::GetTextureAddress(I.AddressU));
				glSamplerParameteri(DeviceState, GL_TEXTURE_WRAP_T, OGLDevice::GetTextureAddress(I.AddressV));
				glSamplerParameteri(DeviceState, GL_TEXTURE_WRAP_R, OGLDevice::GetTextureAddress(I.AddressW));
				glSamplerParameteri(DeviceState, GL_TEXTURE_MAG_FILTER, OGLDevice::GetPixelFilter(I.Filter, true));
				glSamplerParameteri(DeviceState, GL_TEXTURE_MIN_FILTER, OGLDevice::GetPixelFilter(I.Filter, false));
				glSamplerParameteri(DeviceState, GL_TEXTURE_COMPARE_FUNC, OGLDevice::GetComparison(I.ComparisonFunction));
				glSamplerParameterf(DeviceState, GL_TEXTURE_LOD_BIAS, I.MipLODBias);
				glSamplerParameterf(DeviceState, GL_TEXTURE_MAX_LOD, I.MaxLOD);
				glSamplerParameterf(DeviceState, GL_TEXTURE_MIN_LOD, I.MinLOD);
				glSamplerParameterfv(DeviceState, GL_TEXTURE_BORDER_COLOR, (GLfloat*)I.BorderColor);

#ifdef GL_TEXTURE_MAX_ANISOTROPY
				if ((size_t)I.Filter & (size_t)Graphics::PixelFilter::Anistropic || (size_t)I.Filter & (size_t)Graphics::PixelFilter::Compare_Anistropic)
					glSamplerParameterf(DeviceState, GL_TEXTURE_MAX_ANISOTROPY, (float)I.MaxAnisotropy);
#elif defined(GL_TEXTURE_MAX_ANISOTROPY_EXT)
				if ((size_t)I.Filter & (size_t)Graphics::PixelFilter::Anistropic || (size_t)I.Filter & (size_t)Graphics::PixelFilter::Compare_Anistropic)
					glSamplerParameterf(DeviceState, GL_TEXTURE_MAX_ANISOTROPY_EXT, (float)I.MaxAnisotropy);
#endif
				OGLSamplerState* Result = new OGLSamplerState(I);
				Result->Resource = DeviceState;

				return Result;
			}
			InputLayout* OGLDevice::CreateInputLayout(const InputLayout::Desc& I)
			{
				return new OGLInputLayout(I);
			}
			Shader* OGLDevice::CreateShader(const Shader::Desc& I)
			{
				OGLShader* Result = new OGLShader(I);
				Shader::Desc F(I);

				if (!Preprocess(F))
				{
					TH_ERR("shader preprocessing failed");
					return Result;
				}

				Core::Parser::Settle Start;
				Core::Parser Code(&F.Data);
				GLint StatusCode = 0;

				std::string VertexEntry = GetShaderMain(ShaderType::Vertex);
				if (Code.Find(VertexEntry).Found)
				{
					std::string Source = F.Data;
					if (!Transpile(&Source, ShaderType::Vertex, ShaderLang::GLSL))
					{
						TH_ERR("shader transpiling failed");
						return Result;
					}

					const char* Data = Source.c_str();
					GLint Size = (GLint)Source.size();

					Result->VertexShader = glCreateShader(GL_VERTEX_SHADER);
					glShaderSourceARB(Result->VertexShader, 1, &Data, &Size);
					glCompileShaderARB(Result->VertexShader);
					glGetShaderiv(Result->VertexShader, GL_COMPILE_STATUS, &StatusCode);

					if (StatusCode == GL_FALSE)
					{
						glGetShaderiv(Result->VertexShader, GL_INFO_LOG_LENGTH, &Size);
						char* Buffer = (char*)TH_MALLOC(sizeof(char) * (Size + 1));
						glGetShaderInfoLog(Result->VertexShader, Size, &Size, Buffer);
						TH_ERR("couldn't compile vertex shader\n\t%.*s", Size, Buffer);
						TH_FREE(Buffer);
					}
				}

				std::string PixelEntry = GetShaderMain(ShaderType::Pixel);
				if (Code.Find(PixelEntry).Found)
				{
					std::string Source = F.Data;
					if (!Transpile(&Source, ShaderType::Pixel, ShaderLang::GLSL))
					{
						TH_ERR("shader transpiling failed");
						return Result;
					}

					const char* Data = Source.c_str();
					GLint Size = (GLint)Source.size();

					Result->PixelShader = glCreateShader(GL_FRAGMENT_SHADER);
					glShaderSourceARB(Result->PixelShader, 1, &Data, &Size);
					glCompileShaderARB(Result->PixelShader);
					glGetShaderiv(Result->PixelShader, GL_COMPILE_STATUS, &StatusCode);

					if (StatusCode == GL_FALSE)
					{
						glGetShaderiv(Result->PixelShader, GL_INFO_LOG_LENGTH, &Size);
						char* Buffer = (char*)TH_MALLOC(sizeof(char) * (Size + 1));
						glGetShaderInfoLog(Result->PixelShader, Size, &Size, Buffer);
						TH_ERR("couldn't compile pixel shader\n\t%.*s", Size, Buffer);
						TH_FREE(Buffer);
					}
				}

				std::string GeometryEntry = GetShaderMain(ShaderType::Geometry);
				if (Code.Find(GeometryEntry).Found)
				{
					std::string Source = F.Data;
					if (!Transpile(&Source, ShaderType::Geometry, ShaderLang::GLSL))
					{
						TH_ERR("shader transpiling failed");
						return Result;
					}

					const char* Data = Source.c_str();
					GLint Size = (GLint)Source.size();

					Result->GeometryShader = glCreateShader(GL_GEOMETRY_SHADER);
					glShaderSourceARB(Result->GeometryShader, 1, &Data, &Size);
					glCompileShaderARB(Result->GeometryShader);
					glGetShaderiv(Result->GeometryShader, GL_COMPILE_STATUS, &StatusCode);

					if (StatusCode == GL_FALSE)
					{
						glGetShaderiv(Result->GeometryShader, GL_INFO_LOG_LENGTH, &Size);
						char* Buffer = (char*)TH_MALLOC(sizeof(char) * (Size + 1));
						glGetShaderInfoLog(Result->GeometryShader, Size, &Size, Buffer);
						TH_ERR("couldn't compile geometry shader\n\t%.*s", Size, Buffer);
						TH_FREE(Buffer);
					}
				}

				std::string ComputeEntry = GetShaderMain(ShaderType::Compute);
				if (Code.Find(ComputeEntry).Found)
				{
					std::string Source = F.Data;
					if (!Transpile(&Source, ShaderType::Compute, ShaderLang::GLSL))
					{
						TH_ERR("shader transpiling failed");
						return Result;
					}

					const char* Data = Source.c_str();
					GLint Size = (GLint)Source.size();

					Result->ComputeShader = glCreateShader(GL_COMPUTE_SHADER);
					glShaderSourceARB(Result->ComputeShader, 1, &Data, &Size);
					glCompileShaderARB(Result->ComputeShader);
					glGetShaderiv(Result->ComputeShader, GL_COMPILE_STATUS, &StatusCode);

					if (StatusCode == GL_FALSE)
					{
						glGetShaderiv(Result->ComputeShader, GL_INFO_LOG_LENGTH, &Size);
						char* Buffer = (char*)TH_MALLOC(sizeof(char) * (Size + 1));
						glGetShaderInfoLog(Result->ComputeShader, Size, &Size, Buffer);
						TH_ERR("couldn't compile compute shader\n\t%.*s", Size, Buffer);
						TH_FREE(Buffer);
					}
				}

				std::string HullEntry = GetShaderMain(ShaderType::Hull);
				if (Code.Find(HullEntry).Found)
				{
					std::string Source = F.Data;
					if (!Transpile(&Source, ShaderType::Hull, ShaderLang::GLSL))
					{
						TH_ERR("shader transpiling failed");
						return Result;
					}

					const char* Data = Source.c_str();
					GLint Size = (GLint)Source.size();

					Result->HullShader = glCreateShader(GL_TESS_CONTROL_SHADER);
					glShaderSourceARB(Result->HullShader, 1, &Data, &Size);
					glCompileShaderARB(Result->HullShader);
					glGetShaderiv(Result->HullShader, GL_COMPILE_STATUS, &StatusCode);

					if (StatusCode == GL_FALSE)
					{
						glGetShaderiv(Result->HullShader, GL_INFO_LOG_LENGTH, &Size);
						char* Buffer = (char*)TH_MALLOC(sizeof(char) * (Size + 1));
						glGetShaderInfoLog(Result->HullShader, Size, &Size, Buffer);
						TH_ERR("couldn't compile hull shader\n\t%.*s", Size, Buffer);
						TH_FREE(Buffer);
					}
				}

				std::string DomainEntry = GetShaderMain(ShaderType::Domain);
				if (Code.Find(DomainEntry).Found)
				{
					std::string Source = F.Data;
					if (!Transpile(&Source, ShaderType::Domain, ShaderLang::GLSL))
					{
						TH_ERR("shader transpiling failed");
						return Result;
					}

					const char* Data = Source.c_str();
					GLint Size = (GLint)Source.size();

					Result->DomainShader = glCreateShader(GL_TESS_EVALUATION_SHADER);
					glShaderSourceARB(Result->DomainShader, 1, &Data, &Size);
					glCompileShaderARB(Result->DomainShader);
					glGetShaderiv(Result->DomainShader, GL_COMPILE_STATUS, &StatusCode);

					if (StatusCode == GL_FALSE)
					{
						glGetShaderiv(Result->DomainShader, GL_INFO_LOG_LENGTH, &Size);
						char* Buffer = (char*)TH_MALLOC(sizeof(char) * (Size + 1));
						glGetShaderInfoLog(Result->DomainShader, Size, &Size, Buffer);
						TH_ERR("couldn't compile domain shader\n\t%.*s", Size, Buffer);
						TH_FREE(Buffer);
					}
				}

				return Result;
			}
			ElementBuffer* OGLDevice::CreateElementBuffer(const ElementBuffer::Desc& I)
			{
				OGLElementBuffer* Result = new OGLElementBuffer(I);
				Result->Flags = OGLDevice::GetResourceBind(I.BindFlags);
				if ((size_t)I.MiscFlags & (size_t)ResourceMisc::Buffer_Structured)
					Result->Flags = GL_SHADER_STORAGE_BUFFER;

				glGenBuffers(1, &Result->Resource);
				glBindBuffer(Result->Flags, Result->Resource);
				glBufferData(Result->Flags, I.ElementCount * I.ElementWidth, I.Elements, GL_STATIC_DRAW);

				return Result;
			}
			MeshBuffer* OGLDevice::CreateMeshBuffer(const MeshBuffer::Desc& I)
			{
				ElementBuffer::Desc F = ElementBuffer::Desc();
				F.AccessFlags = I.AccessFlags;
				F.Usage = I.Usage;
				F.BindFlags = ResourceBind::Vertex_Buffer;
				F.ElementCount = (unsigned int)I.Elements.size();
				F.Elements = (void*)I.Elements.data();
				F.ElementWidth = sizeof(Compute::Vertex);

				OGLMeshBuffer* Result = new OGLMeshBuffer(I);
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
			SkinMeshBuffer* OGLDevice::CreateSkinMeshBuffer(const SkinMeshBuffer::Desc& I)
			{
				ElementBuffer::Desc F = ElementBuffer::Desc();
				F.AccessFlags = I.AccessFlags;
				F.Usage = I.Usage;
				F.BindFlags = ResourceBind::Vertex_Buffer;
				F.ElementCount = (unsigned int)I.Elements.size();
				F.Elements = (void*)I.Elements.data();
				F.ElementWidth = sizeof(Compute::SkinVertex);

				OGLSkinMeshBuffer* Result = new OGLSkinMeshBuffer(I);
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
			InstanceBuffer* OGLDevice::CreateInstanceBuffer(const InstanceBuffer::Desc& I)
			{
				ElementBuffer::Desc F = ElementBuffer::Desc();
				F.AccessFlags = CPUAccess::Write;
				F.MiscFlags = ResourceMisc::Buffer_Structured;
				F.Usage = ResourceUsage::Dynamic;
				F.BindFlags = ResourceBind::Shader_Input;
				F.ElementCount = I.ElementLimit;
				F.ElementWidth = I.ElementWidth;
				F.StructureByteStride = F.ElementWidth;

				OGLInstanceBuffer* Result = new OGLInstanceBuffer(I);
				Result->Elements = CreateElementBuffer(F);

				return Result;
			}
			Texture2D* OGLDevice::CreateTexture2D()
			{
				return new OGLTexture2D();
			}
			Texture2D* OGLDevice::CreateTexture2D(const Texture2D::Desc& I)
			{
				OGLTexture2D* Result = new OGLTexture2D(I);
				glGenTextures(1, &Result->Resource);
				glBindTexture(GL_TEXTURE_2D, Result->Resource);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_REPEAT);

				Result->Format = GetFormat(I.FormatMode);
				if (I.MipLevels > 0)
				{
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, I.MipLevels - 1);
				}
				else
				{
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				}

				glTexImage2D(GL_TEXTURE_2D, 0, OGLDevice::GetFormat(Result->FormatMode), Result->Width, Result->Height, 0, OGLDevice::GetFormat(Result->FormatMode), GL_UNSIGNED_BYTE, I.Data);
				if (!GenerateTexture(Result))
					return Result;

				if (Result->MipLevels != 0)
					glGenerateMipmap(GL_TEXTURE_2D);

				return Result;
			}
			Texture3D* OGLDevice::CreateTexture3D()
			{
				return new OGLTexture3D();
			}
			Texture3D* OGLDevice::CreateTexture3D(const Texture3D::Desc& I)
			{
				OGLTexture3D* Result = new OGLTexture3D();
				glGenTextures(1, &Result->Resource);
				glBindTexture(GL_TEXTURE_3D, Result->Resource);
				glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
				glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
				glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);

				Result->Format = GetFormat(I.FormatMode);
				if (I.MipLevels > 0)
				{
					glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
					glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
					glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_BASE_LEVEL, 0);
					glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAX_LEVEL, I.MipLevels - 1);
				}
				else
				{
					glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
					glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				}

				glTexImage3D(GL_TEXTURE_3D, 0, OGLDevice::GetFormat(Result->FormatMode), Result->Width, Result->Height, Result->Depth, 0, OGLDevice::GetFormat(Result->FormatMode), GL_UNSIGNED_BYTE, nullptr);
				if (Result->MipLevels != 0)
					glGenerateMipmap(GL_TEXTURE_3D);

				return Result;
			}
			TextureCube* OGLDevice::CreateTextureCube()
			{
				return new OGLTextureCube();
			}
			TextureCube* OGLDevice::CreateTextureCube(const TextureCube::Desc& I)
			{
				OGLTextureCube* Result = new OGLTextureCube(I);
				glGenTextures(1, &Result->Resource);
				glBindTexture(GL_TEXTURE_CUBE_MAP, Result->Resource);
				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

				Result->Format = GetFormat(I.FormatMode);
				if (I.MipLevels > 0)
				{
					glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
					glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
					glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_BASE_LEVEL, 0);
					glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LEVEL, I.MipLevels - 1);
				}
				else
				{
					glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
					glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				}

				GLint Format = OGLDevice::GetFormat(Result->FormatMode);
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, Format, Result->Width, Result->Height, 0, Format, GL_UNSIGNED_BYTE, nullptr);
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, Format, Result->Width, Result->Height, 0, Format, GL_UNSIGNED_BYTE, nullptr);
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, Format, Result->Width, Result->Height, 0, Format, GL_UNSIGNED_BYTE, nullptr);
				glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, Format, Result->Width, Result->Height, 0, Format, GL_UNSIGNED_BYTE, nullptr);
				glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, Format, Result->Width, Result->Height, 0, Format, GL_UNSIGNED_BYTE, nullptr);
				glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, Format, Result->Width, Result->Height, 0, Format, GL_UNSIGNED_BYTE, nullptr);
				
				if (Result->MipLevels != 0)
					glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

				return Result;
			}
			TextureCube* OGLDevice::CreateTextureCube(Texture2D* Resource[6])
			{
				TH_ASSERT(Resource[0] && Resource[1] && Resource[2] && Resource[3] && Resource[4] && Resource[5], nullptr, "all 6 faces should be set");
				void* Resources[6];
				for (unsigned int i = 0; i < 6; i++)
					Resources[i] = (void*)(OGLTexture2D*)Resource[i];

				return CreateTextureCubeInternal(Resources);
			}
			TextureCube* OGLDevice::CreateTextureCube(Texture2D* Resource)
			{
				TH_ASSERT(Resource != nullptr, nullptr, "resource should be set");
				OGLTexture2D* IResource = (OGLTexture2D*)Resource;
				unsigned int Width = IResource->Width / 4;
				unsigned int Height = Width;
				unsigned int MipLevels = GetMipLevel(Width, Height);

				if (IResource->Width % 4 != 0 || IResource->Height % 3 != 0)
				{
					TH_ERR("couldn't create texture cube because width or height cannot be not divided");
					return nullptr;
				}

				if (IResource->MipLevels > MipLevels)
					IResource->MipLevels = MipLevels;

				OGLTextureCube* Result = new OGLTextureCube();
				glGenTextures(1, &Result->Resource);
				glBindTexture(GL_TEXTURE_CUBE_MAP, Result->Resource);
				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

				if (IResource->MipLevels > 0)
				{
					glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
					glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
					glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_BASE_LEVEL, 0);
					glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LEVEL, IResource->MipLevels - 1);
				}
				else
				{
					glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
					glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				}

				GLint Format = OGLDevice::GetFormat(Result->FormatMode);
				GLsizei Size = sizeof(GLubyte) * Width * Height;
				GLubyte* Pixels = (GLubyte*)TH_MALLOC(Size);
				Result->FormatMode = IResource->FormatMode;
				Result->Width = IResource->Width;
				Result->Height = IResource->Height;
				Result->MipLevels = IResource->MipLevels;

				glBindTexture(GL_TEXTURE_CUBE_MAP, IResource->Resource);
				glGetTextureSubImage(GL_TEXTURE_2D, 0, Width * 2, Height, 0, Width, Height, 0, Format, GL_UNSIGNED_BYTE, Size, Pixels);
				glBindTexture(GL_TEXTURE_CUBE_MAP, Result->Resource);
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, IResource->MipLevels, Format, IResource->Width, IResource->Height, 0, Format, GL_UNSIGNED_BYTE, Pixels);

				glBindTexture(GL_TEXTURE_CUBE_MAP, IResource->Resource);
				glGetTextureSubImage(GL_TEXTURE_2D, 0, Width, Height * 2, 0, Width, Height, 0, Format, GL_UNSIGNED_BYTE, Size, Pixels);
				glBindTexture(GL_TEXTURE_CUBE_MAP, Result->Resource);
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, IResource->MipLevels, Format, IResource->Width, IResource->Height, 0, Format, GL_UNSIGNED_BYTE, Pixels);

				glBindTexture(GL_TEXTURE_CUBE_MAP, IResource->Resource);
				glGetTextureSubImage(GL_TEXTURE_2D, 0, Width * 4, Height, 0, Width, Height, 0, Format, GL_UNSIGNED_BYTE, Size, Pixels);
				glBindTexture(GL_TEXTURE_CUBE_MAP, Result->Resource);
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, IResource->MipLevels, Format, IResource->Width, IResource->Height, 0, Format, GL_UNSIGNED_BYTE, Pixels);

				glBindTexture(GL_TEXTURE_CUBE_MAP, IResource->Resource);
				glGetTextureSubImage(GL_TEXTURE_2D, 0, 0, Height, 0, Width, Height, 0, Format, GL_UNSIGNED_BYTE, Size, Pixels);
				glBindTexture(GL_TEXTURE_CUBE_MAP, Result->Resource);
				glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, IResource->MipLevels, Format, IResource->Width, IResource->Height, 0, Format, GL_UNSIGNED_BYTE, Pixels);

				glBindTexture(GL_TEXTURE_CUBE_MAP, IResource->Resource);
				glGetTextureSubImage(GL_TEXTURE_2D, 0, Width, 0, 0, Width, Height, 0, Format, GL_UNSIGNED_BYTE, Size, Pixels);
				glBindTexture(GL_TEXTURE_CUBE_MAP, Result->Resource);
				glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, IResource->MipLevels, Format, IResource->Width, IResource->Height, 0, Format, GL_UNSIGNED_BYTE, Pixels);

				glBindTexture(GL_TEXTURE_CUBE_MAP, IResource->Resource);
				glGetTextureSubImage(GL_TEXTURE_2D, 0, Width, Height, 0, Width, Height, 0, Format, GL_UNSIGNED_BYTE, Size, Pixels);
				glBindTexture(GL_TEXTURE_CUBE_MAP, Result->Resource);
				glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, IResource->MipLevels, Format, IResource->Width, IResource->Height, 0, Format, GL_UNSIGNED_BYTE, Pixels);

				TH_FREE(Pixels);
				if (IResource->MipLevels != 0)
					glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

				return Result;
			}
			TextureCube* OGLDevice::CreateTextureCubeInternal(void* Basis[6])
			{
				TH_ASSERT(Basis[0] && Basis[1] && Basis[2] && Basis[3] && Basis[4] && Basis[5], nullptr, "all 6 faces should be set");
				OGLTexture2D* Resources[6];
				for (unsigned int i = 0; i < 6; i++)
					Resources[i] = (OGLTexture2D*)Basis[i];

				OGLTextureCube* Result = new OGLTextureCube();
				glGenTextures(1, &Result->Resource);
				glBindTexture(GL_TEXTURE_CUBE_MAP, Result->Resource);
				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

				if (Resources[0]->MipLevels > 0)
				{
					glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
					glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
					glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_BASE_LEVEL, 0);
					glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LEVEL, Resources[0]->MipLevels - 1);
				}
				else
				{
					glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
					glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				}

				GLint Format = OGLDevice::GetFormat(Result->FormatMode);
				GLubyte* Pixels = (GLubyte*)TH_MALLOC(sizeof(GLubyte) * Resources[0]->Width * Resources[0]->Height);
				Result->FormatMode = Resources[0]->FormatMode;
				Result->Width = Resources[0]->Width;
				Result->Height = Resources[0]->Height;
				Result->MipLevels = Resources[0]->MipLevels;

				for (unsigned int i = 0; i < 6; i++)
				{
					OGLTexture2D* Ref = Resources[i];
					glBindTexture(GL_TEXTURE_CUBE_MAP, Ref->Resource);
					glGetTexImage(GL_TEXTURE_2D, 0, Format, GL_UNSIGNED_BYTE, Pixels);
					glBindTexture(GL_TEXTURE_CUBE_MAP, Result->Resource);
					glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, Ref->MipLevels, Format, Ref->Width, Ref->Height, 0, Format, GL_UNSIGNED_BYTE, Pixels);
				}

				TH_FREE(Pixels);
				if (Resources[0]->MipLevels != 0)
					glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

				return Result;
			}
			DepthBuffer* OGLDevice::CreateDepthBuffer(const DepthBuffer::Desc& I)
			{
				OGLDepthBuffer* Result = new OGLDepthBuffer(I);
				glGenTextures(1, &Result->DepthTexture);
				glBindTexture(GL_TEXTURE_2D, Result->DepthTexture);
				glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH24_STENCIL8, I.Width, I.Height);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_GREATER);

				Result->Resource = CreateTexture2D();
				((OGLTexture2D*)Result->Resource)->Resource = Result->DepthTexture;
				if (!GenerateTexture(Result->Resource))
				{
					TH_ERR("couldn't create 2d resource");
					return Result;
				}

				glGenFramebuffers(1, &Result->FrameBuffer);
				glBindFramebuffer(GL_FRAMEBUFFER, Result->FrameBuffer);
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, Result->DepthTexture, 0);
				glBindFramebuffer(GL_FRAMEBUFFER, 0);

				Result->Viewarea.Width = (float)I.Width;
				Result->Viewarea.Height = (float)I.Height;
				Result->Viewarea.TopLeftX = 0.0f;
				Result->Viewarea.TopLeftY = 0.0f;
				Result->Viewarea.MinDepth = 0.0f;
				Result->Viewarea.MaxDepth = 1.0f;

				return Result;
			}
			RenderTarget2D* OGLDevice::CreateRenderTarget2D(const RenderTarget2D::Desc& I)
			{
				OGLRenderTarget2D* Result = new OGLRenderTarget2D(I);
				if (!I.RenderSurface)
				{
					GLenum Format = OGLDevice::GetFormat(I.FormatMode);
					glGenTextures(1, &Result->FrameBuffer.Texture[0]);
					glBindTexture(GL_TEXTURE_2D, Result->FrameBuffer.Texture[0]);
					glTexStorage2D(GL_TEXTURE_2D, I.MipLevels, Format, I.Width, I.Height);
					if (I.MipLevels > 0)
					{
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, I.MipLevels - 1);
					}
					else
					{
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
					}

					Result->Resource = CreateTexture2D();
					((OGLTexture2D*)Result->Resource)->Resource = Result->FrameBuffer.Texture[0];
					if (!GenerateTexture(Result->Resource))
					{
						TH_ERR("couldn't create 2d resource");
						return Result;
					}

					glGenTextures(1, &Result->DepthTexture);
					glBindTexture(GL_TEXTURE_2D, Result->DepthTexture);
					glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH24_STENCIL8, I.Width, I.Height);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_GREATER);

					Result->DepthStencil = CreateTexture2D();
					((OGLTexture2D*)Result->DepthStencil)->Resource = Result->DepthTexture;
					if (!GenerateTexture(Result->DepthStencil))
					{
						TH_ERR("couldn't create 2d resource");
						return Result;
					}

					glGenFramebuffers(1, &Result->FrameBuffer.Buffer);
					glBindFramebuffer(GL_FRAMEBUFFER, Result->FrameBuffer.Buffer);
					glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, Result->FrameBuffer.Texture[0], 0);
					glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, Result->DepthTexture, 0);
					glBindFramebuffer(GL_FRAMEBUFFER, 0);
				}
				else if (I.RenderSurface == (void*)this)
					Result->FrameBuffer.Backbuffer = true;

				Result->Viewarea.Width = (float)I.Width;
				Result->Viewarea.Height = (float)I.Height;
				Result->Viewarea.TopLeftX = 0.0f;
				Result->Viewarea.TopLeftY = 0.0f;
				Result->Viewarea.MinDepth = 0.0f;
				Result->Viewarea.MaxDepth = 1.0f;

				return Result;
			}
			MultiRenderTarget2D* OGLDevice::CreateMultiRenderTarget2D(const MultiRenderTarget2D::Desc& I)
			{
				OGLMultiRenderTarget2D* Result = new OGLMultiRenderTarget2D(I);
				glGenFramebuffers(1, &Result->FrameBuffer.Buffer);
				glBindFramebuffer(GL_FRAMEBUFFER, Result->FrameBuffer.Buffer);

				for (unsigned int i = 0; i < (unsigned int)I.Target; i++)
				{
					GLenum Format = OGLDevice::GetFormat(I.FormatMode[i]);
					glGenTextures(1, &Result->FrameBuffer.Texture[i]);
					glBindTexture(GL_TEXTURE_2D, Result->FrameBuffer.Texture[i]);
					glTexStorage2D(GL_TEXTURE_2D, I.MipLevels, Format, I.Width, I.Height);
					if (I.MipLevels > 0)
					{
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, I.MipLevels - 1);
					}
					else
					{
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
					}

					Result->Resource[i] = CreateTexture2D();
					((OGLTexture2D*)Result->Resource[i])->Resource = Result->FrameBuffer.Texture[i];
					if (!GenerateTexture(Result->Resource[i]))
					{
						TH_ERR("couldn't create 2d resource");
						return Result;
					}

					Result->FrameBuffer.Format[i] = GL_COLOR_ATTACHMENT0 + i;
					glFramebufferTexture2D(GL_FRAMEBUFFER, Result->FrameBuffer.Format[i], GL_TEXTURE_2D, Result->FrameBuffer.Texture[i], 0);
				}

				glGenTextures(1, &Result->DepthTexture);
				glBindTexture(GL_TEXTURE_2D, Result->DepthTexture);
				glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH24_STENCIL8, I.Width, I.Height);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_GREATER);

				Result->DepthStencil = CreateTexture2D();
				((OGLTexture2D*)Result->DepthStencil)->Resource = Result->DepthTexture;
				if (!GenerateTexture(Result->DepthStencil))
				{
					TH_ERR("couldn't create 2d resource");
					return Result;
				}

				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, Result->DepthTexture, 0);
				glBindFramebuffer(GL_FRAMEBUFFER, 0);

				Result->Viewarea.Width = (float)I.Width;
				Result->Viewarea.Height = (float)I.Height;
				Result->Viewarea.TopLeftX = 0.0f;
				Result->Viewarea.TopLeftY = 0.0f;
				Result->Viewarea.MinDepth = 0.0f;
				Result->Viewarea.MaxDepth = 1.0f;

				return Result;
			}
			RenderTargetCube* OGLDevice::CreateRenderTargetCube(const RenderTargetCube::Desc& I)
			{
				OGLRenderTargetCube* Result = new OGLRenderTargetCube(I);
				GLenum Format = OGLDevice::GetFormat(I.FormatMode);
				glGenTextures(1, &Result->FrameBuffer.Texture[0]);
				glBindTexture(GL_TEXTURE_CUBE_MAP, Result->FrameBuffer.Texture[0]);
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + 0, 0, Format, I.Size, I.Size, 0, Format, GL_UNSIGNED_BYTE, 0);
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + 1, 0, Format, I.Size, I.Size, 0, Format, GL_UNSIGNED_BYTE, 0);
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + 2, 0, Format, I.Size, I.Size, 0, Format, GL_UNSIGNED_BYTE, 0);
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + 3, 0, Format, I.Size, I.Size, 0, Format, GL_UNSIGNED_BYTE, 0);
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + 4, 0, Format, I.Size, I.Size, 0, Format, GL_UNSIGNED_BYTE, 0);
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + 5, 0, Format, I.Size, I.Size, 0, Format, GL_UNSIGNED_BYTE, 0);
				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
				if (I.MipLevels > 0)
				{
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, I.MipLevels - 1);
				}
				else
				{
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				}

				Result->Resource = CreateTexture2D();
				((OGLTexture2D*)Result->Resource)->Resource = Result->FrameBuffer.Texture[0];
				if (!GenerateTexture(Result->Resource))
				{
					TH_ERR("couldn't create 2d resource");
					return Result;
				}

				glGenTextures(1, &Result->DepthTexture);
				glBindTexture(GL_TEXTURE_CUBE_MAP, Result->DepthTexture);
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + 0, 0, GL_DEPTH24_STENCIL8, I.Size, I.Size, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, 0);
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + 1, 0, GL_DEPTH24_STENCIL8, I.Size, I.Size, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, 0);
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + 2, 0, GL_DEPTH24_STENCIL8, I.Size, I.Size, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, 0);
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + 3, 0, GL_DEPTH24_STENCIL8, I.Size, I.Size, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, 0);
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + 4, 0, GL_DEPTH24_STENCIL8, I.Size, I.Size, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, 0);
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + 5, 0, GL_DEPTH24_STENCIL8, I.Size, I.Size, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, 0);
				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_COMPARE_FUNC, GL_GREATER);

				Result->DepthStencil = CreateTexture2D();
				((OGLTexture2D*)Result->DepthStencil)->Resource = Result->DepthTexture;
				if (!GenerateTexture(Result->DepthStencil))
				{
					TH_ERR("couldn't create 2d resource");
					return Result;
				}

				glGenFramebuffers(1, &Result->FrameBuffer.Buffer);
				glBindFramebuffer(GL_FRAMEBUFFER, Result->FrameBuffer.Buffer);
				glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, Result->FrameBuffer.Texture[0], 0);
				glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, Result->DepthTexture, 0);
				glBindFramebuffer(GL_FRAMEBUFFER, 0);

				Result->Viewarea.Width = (float)I.Size;
				Result->Viewarea.Height = (float)I.Size;
				Result->Viewarea.TopLeftX = 0.0f;
				Result->Viewarea.TopLeftY = 0.0f;
				Result->Viewarea.MinDepth = 0.0f;
				Result->Viewarea.MaxDepth = 1.0f;

				return Result;
			}
			MultiRenderTargetCube* OGLDevice::CreateMultiRenderTargetCube(const MultiRenderTargetCube::Desc& I)
			{
				OGLMultiRenderTargetCube* Result = new OGLMultiRenderTargetCube(I);
				glGenFramebuffers(1, &Result->FrameBuffer.Buffer);
				glBindFramebuffer(GL_FRAMEBUFFER, Result->FrameBuffer.Buffer);

				for (unsigned int i = 0; i < (unsigned int)I.Target; i++)
				{
					GLenum Format = OGLDevice::GetFormat(I.FormatMode[i]);
					glGenTextures(1, &Result->FrameBuffer.Texture[i]);
					glBindTexture(GL_TEXTURE_CUBE_MAP, Result->FrameBuffer.Texture[i]);
					glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + 0, 0, Format, I.Size, I.Size, 0, Format, GL_UNSIGNED_BYTE, 0);
					glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + 1, 0, Format, I.Size, I.Size, 0, Format, GL_UNSIGNED_BYTE, 0);
					glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + 2, 0, Format, I.Size, I.Size, 0, Format, GL_UNSIGNED_BYTE, 0);
					glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + 3, 0, Format, I.Size, I.Size, 0, Format, GL_UNSIGNED_BYTE, 0);
					glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + 4, 0, Format, I.Size, I.Size, 0, Format, GL_UNSIGNED_BYTE, 0);
					glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + 5, 0, Format, I.Size, I.Size, 0, Format, GL_UNSIGNED_BYTE, 0);
					glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
					glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
					glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
					if (I.MipLevels > 0)
					{
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, I.MipLevels - 1);
					}
					else
					{
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
					}

					Result->Resource[i] = CreateTexture2D();
					((OGLTexture2D*)Result->Resource[i])->Resource = Result->FrameBuffer.Texture[i];
					if (!GenerateTexture(Result->Resource[i]))
					{
						TH_ERR("couldn't create 2d resource");
						return Result;
					}

					Result->FrameBuffer.Format[i] = GL_COLOR_ATTACHMENT0 + i;
					glFramebufferTexture(GL_FRAMEBUFFER, Result->FrameBuffer.Format[i], Result->FrameBuffer.Texture[i], 0);
				}

				glGenTextures(1, &Result->DepthTexture);
				glBindTexture(GL_TEXTURE_CUBE_MAP, Result->DepthTexture);
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + 0, 0, GL_DEPTH24_STENCIL8, I.Size, I.Size, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, 0);
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + 1, 0, GL_DEPTH24_STENCIL8, I.Size, I.Size, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, 0);
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + 2, 0, GL_DEPTH24_STENCIL8, I.Size, I.Size, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, 0);
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + 3, 0, GL_DEPTH24_STENCIL8, I.Size, I.Size, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, 0);
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + 4, 0, GL_DEPTH24_STENCIL8, I.Size, I.Size, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, 0);
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + 5, 0, GL_DEPTH24_STENCIL8, I.Size, I.Size, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, 0);
				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_COMPARE_FUNC, GL_GREATER);

				Result->DepthStencil = CreateTexture2D();
				((OGLTexture2D*)Result->DepthStencil)->Resource = Result->DepthTexture;
				if (!GenerateTexture(Result->DepthStencil))
				{
					TH_ERR("couldn't create 2d resource");
					return Result;
				}

				glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, Result->DepthTexture, 0);
				glBindFramebuffer(GL_FRAMEBUFFER, 0);

				Result->Viewarea.Width = (float)I.Size;
				Result->Viewarea.Height = (float)I.Size;
				Result->Viewarea.TopLeftX = 0.0f;
				Result->Viewarea.TopLeftY = 0.0f;
				Result->Viewarea.MinDepth = 0.0f;
				Result->Viewarea.MaxDepth = 1.0f;

				return Result;
			}
			Cubemap* OGLDevice::CreateCubemap(const Cubemap::Desc& I)
			{
				return new OGLCubemap(I);
			}
			Query* OGLDevice::CreateQuery(const Query::Desc& I)
			{
				OGLQuery* Result = new OGLQuery();
				Result->Predicate = I.Predicate;
				glGenQueries(1, &Result->Async);
				
				return Result;
			}
			PrimitiveTopology OGLDevice::GetPrimitiveTopology()
			{
				return Primitive;
			}
			ShaderModel OGLDevice::GetSupportedShaderModel()
			{
				const char* Version = (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION);
				if (!Version)
					return ShaderModel::Invalid;

				int Major, Minor;
				if (sscanf(Version, "%i.%i", &Major, &Minor) != 2)
					return ShaderModel::Invalid;

				if (Major == 1)
				{
					if (Minor <= 10)
						return ShaderModel::GLSL_1_1_0;

					if (Minor <= 20)
						return ShaderModel::GLSL_1_2_0;

					if (Minor <= 30)
						return ShaderModel::GLSL_1_3_0;

					if (Minor <= 40)
						return ShaderModel::GLSL_1_4_0;

					if (Minor <= 50)
						return ShaderModel::GLSL_1_5_0;

					return ShaderModel::GLSL_1_5_0;
				}
				else if (Major == 2 || Major == 3)
				{
					if (Minor <= 30)
						return ShaderModel::GLSL_3_3_0;

					return ShaderModel::GLSL_1_5_0;
				}
				else if (Major == 4)
				{
					if (Minor <= 10)
						return ShaderModel::GLSL_4_1_0;

					if (Minor <= 20)
						return ShaderModel::GLSL_4_2_0;

					if (Minor <= 30)
						return ShaderModel::GLSL_4_3_0;

					if (Minor <= 40)
						return ShaderModel::GLSL_4_4_0;

					if (Minor <= 50)
						return ShaderModel::GLSL_4_5_0;

					if (Minor <= 60)
						return ShaderModel::GLSL_4_6_0;

					return ShaderModel::GLSL_4_6_0;
				}

				return ShaderModel::Invalid;
			}
			void* OGLDevice::GetDevice()
			{
				return Context;
			}
			void* OGLDevice::GetContext()
			{
				return Context;
			}
			bool OGLDevice::IsValid()
			{
				return BasicEffect != nullptr;
			}
			bool OGLDevice::CreateDirectBuffer(uint64_t Size)
			{
				MaxElements = Size;
				if (DirectRenderer.VertexBuffer != GL_NONE)
					glDeleteVertexArrays(1, &DirectRenderer.VertexBuffer);

				GLint StatusCode;
				glGenVertexArrays(1, &DirectRenderer.VertexBuffer);
				glBindVertexArray(DirectRenderer.VertexBuffer);
				glBindBuffer(GL_ARRAY_BUFFER, DirectRenderer.VertexBuffer);
				glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * (MaxElements + 1), Elements.empty() ? nullptr : &Elements[0], GL_STREAM_DRAW_ARB);
				glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), BUFFER_OFFSET(0));
				glEnableVertexAttribArray(0);
				glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), BUFFER_OFFSET(sizeof(float) * 3));
				glEnableVertexAttribArray(1);
				glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), BUFFER_OFFSET(sizeof(float) * 5));
				glEnableVertexAttribArray(2);
				
				if (DirectRenderer.VertexShader == GL_NONE)
				{
					static const char* VertexShaderCode = GLSL_INLINE(
						layout(location = 0) uniform mat4 WorldViewProjection;

						layout(location = 0) in vec3 iPosition;
						layout(location = 1) in vec2 iTexCoord;
						layout(location = 2) in vec4 iColor;

						out vec2 oTexCoord;
						out vec4 oColor;

						void main()
						{
							gl_Position = WorldViewProjection * vec4(iPosition.xyz, 1.0);
							oTexCoord = iTexCoord;
							oColor = iColor;
						}
					);

					std::string Result = ShaderVersion;
					Result.append(VertexShaderCode);

					const char* Subbuffer = Result.data();
					GLint BufferSize = Result.size();
					DirectRenderer.VertexShader = glCreateShader(GL_VERTEX_SHADER);
					glShaderSourceARB(DirectRenderer.VertexShader, 1, &Subbuffer, &BufferSize);
					glCompileShaderARB(DirectRenderer.VertexShader);
					glGetShaderiv(DirectRenderer.VertexShader, GL_COMPILE_STATUS, &StatusCode);
					
					if (StatusCode == GL_FALSE)
					{
						glGetShaderiv(DirectRenderer.VertexShader, GL_INFO_LOG_LENGTH, &BufferSize);
						char* Buffer = (char*)TH_MALLOC(sizeof(char) * (BufferSize + 1));
						glGetShaderInfoLog(DirectRenderer.VertexShader, BufferSize, &BufferSize, Buffer);
						TH_ERR("couldn't compile vertex shader\n\t%.*s", BufferSize, Buffer);
						TH_FREE(Buffer);

						return false;
					}
				}

				if (DirectRenderer.PixelShader == GL_NONE)
				{
					static const char* PixelShaderCode = GLSL_INLINE(
						layout(location = 2) uniform sampler2D Diffuse;
						layout(location = 1) uniform vec4 Padding;

						in vec2 oTexCoord;
						in vec4 oColor;
						
						out vec4 oResult;

						void main()
						{
							if (Padding.z > 0)
								oResult = oColor * texture2D(Diffuse, oTexCoord + Padding.xy) * Padding.w;
							else
								oResult = oColor * Padding.w;
						}
					);

					std::string Result = ShaderVersion;
					Result.append(PixelShaderCode);

					const char* Subbuffer = Result.data();
					GLint BufferSize = Result.size();
					DirectRenderer.PixelShader = glCreateShader(GL_FRAGMENT_SHADER);
					glShaderSourceARB(DirectRenderer.PixelShader, 1, &Subbuffer, &BufferSize);
					glCompileShaderARB(DirectRenderer.PixelShader);
					glGetShaderiv(DirectRenderer.PixelShader, GL_COMPILE_STATUS, &StatusCode);

					if (StatusCode == GL_FALSE)
					{
						glGetShaderiv(DirectRenderer.PixelShader, GL_INFO_LOG_LENGTH, &BufferSize);
						char* Buffer = (char*)TH_MALLOC(sizeof(char) * (BufferSize + 1));
						glGetShaderInfoLog(DirectRenderer.PixelShader, BufferSize, &BufferSize, Buffer);
						TH_ERR("couldn't compile pixel shader\n\t%.*s", BufferSize, Buffer);
						TH_FREE(Buffer);

						return false;
					}
				}

				if (DirectRenderer.Program == GL_NONE)
				{
					DirectRenderer.Program = glCreateProgram();
					glAttachShader(DirectRenderer.Program, DirectRenderer.VertexShader);
					glAttachShader(DirectRenderer.Program, DirectRenderer.PixelShader);
					glLinkProgramARB(DirectRenderer.Program);
					glGetProgramiv(DirectRenderer.Program, GL_LINK_STATUS, &StatusCode);

					if (StatusCode == GL_FALSE)
					{
						GLint Size = 0;
						glGetProgramiv(DirectRenderer.Program, GL_INFO_LOG_LENGTH, &Size);

						char* Buffer = (char*)TH_MALLOC(sizeof(char) * (Size + 1));
						glGetProgramInfoLog(DirectRenderer.Program, Size, &Size, Buffer);
						TH_ERR("couldn't link shaders\n\t%.*s", Size, Buffer);
						TH_FREE(Buffer);

						glDeleteProgram(DirectRenderer.Program);
						DirectRenderer.Program = GL_NONE;

						return false;
					}

					glUseProgram(DirectRenderer.Program);
					glUniform1iARB(2, 0);
					glUseProgram(0);
				}

				return true;
			}
			const char* OGLDevice::GetShaderVersion()
			{
				return ShaderVersion;
			}
			void OGLDevice::CopyConstantBuffer(GLuint Buffer, void* Data, size_t Size)
			{
				TH_ASSERT_V(Data != nullptr && Size > 0, "buffer should not be empty");
				glBindBuffer(GL_UNIFORM_BUFFER, Buffer);
				GLvoid* Subdata = glMapBuffer(GL_UNIFORM_BUFFER, GL_WRITE_ONLY);
				memcpy(Subdata, Data, Size);
				glUnmapBuffer(GL_UNIFORM_BUFFER);
			}
			int OGLDevice::CreateConstantBuffer(GLuint* Buffer, size_t Size)
			{
				TH_ASSERT(Buffer != nullptr, 0, "buffer should be set");
				glGenBuffers(1, Buffer);
				glBindBuffer(GL_UNIFORM_BUFFER, *Buffer);
				glBufferData(GL_UNIFORM_BUFFER, Size, nullptr, GL_DYNAMIC_DRAW);
				glBindBuffer(GL_UNIFORM_BUFFER, 0);

				return (int)*Buffer;
			}
			std::string OGLDevice::CompileState(GLuint Handle)
			{
				GLint Stat = 0, Size = 0;
				glGetShaderiv(Handle, GL_COMPILE_STATUS, &Stat);
				glGetShaderiv(Handle, GL_INFO_LOG_LENGTH, &Size);

				if ((GLboolean)Stat == GL_TRUE || !Size)
					return "";

				GLchar* Buffer = (GLchar*)TH_MALLOC(sizeof(GLchar) * Size);
				glGetShaderInfoLog(Handle, Size, NULL, Buffer);

				std::string Result((char*)Buffer, Size);
				TH_FREE(Buffer);

				return Result;
			}
			GLenum OGLDevice::GetFormat(Format Value)
			{
				switch (Value)
				{
					case Format::R32G32B32A32_Typeless:
						return GL_RGBA32UI;
					case Format::R32G32B32A32_Float:
						return GL_RGBA32F;
					case Format::R32G32B32A32_Uint:
						return GL_RGBA32UI;
					case Format::R32G32B32A32_Sint:
						return GL_RGBA32I;
					case Format::R32G32B32_Typeless:
						return GL_RGB32UI;
					case Format::R32G32B32_Float:
						return GL_RGB32F;
					case Format::R32G32B32_Uint:
						return GL_RGB32UI;
					case Format::R32G32B32_Sint:
						return GL_RGB32I;
					case Format::R16G16B16A16_Typeless:
						return GL_RGBA16UI;
					case Format::R16G16B16A16_Float:
						return GL_RGBA16F;
					case Format::R16G16B16A16_Unorm:
						return GL_RGBA16;
					case Format::R16G16B16A16_Uint:
						return GL_RGBA16UI;
					case Format::R16G16B16A16_Snorm:
						return GL_RGBA16I;
					case Format::R16G16B16A16_Sint:
						return GL_RGBA16I;
					case Format::R32G32_Typeless:
						return GL_RG16UI;
					case Format::R32G32_Float:
						return GL_RG16F;
					case Format::R32G32_Uint:
						return GL_RG16UI;
					case Format::R32G32_Sint:
						return GL_RG16I;
					case Format::R32G8X24_Typeless:
						return GL_R32UI;
					case Format::D32_Float_S8X24_Uint:
						return GL_R32UI;
					case Format::R32_Float_X8X24_Typeless:
						return GL_R32UI;
					case Format::X32_Typeless_G8X24_Uint:
						return GL_R32UI;
					case Format::R10G10B10A2_Typeless:
						return GL_RGB10_A2;
					case Format::R10G10B10A2_Unorm:
						return GL_RGB10_A2;
					case Format::R10G10B10A2_Uint:
						return GL_RGB10_A2UI;
					case Format::R11G11B10_Float:
						return GL_RGB12;
					case Format::R8G8B8A8_Typeless:
						return GL_RGBA8UI;
					case Format::R8G8B8A8_Unorm:
						return GL_RGBA;
					case Format::R8G8B8A8_Unorm_SRGB:
						return GL_RGBA;
					case Format::R8G8B8A8_Uint:
						return GL_RGBA8UI;
					case Format::R8G8B8A8_Snorm:
						return GL_RGBA8I;
					case Format::R8G8B8A8_Sint:
						return GL_RGBA8I;
					case Format::R16G16_Typeless:
						return GL_RG16UI;
					case Format::R16G16_Float:
						return GL_RG16F;
					case Format::R16G16_Unorm:
						return GL_RG16;
					case Format::R16G16_Uint:
						return GL_RG16UI;
					case Format::R16G16_Snorm:
						return GL_RG16_SNORM;
					case Format::R16G16_Sint:
						return GL_RG16I;
					case Format::R32_Typeless:
						return GL_R32UI;
					case Format::D32_Float:
						return GL_R32F;
					case Format::R32_Float:
						return GL_R32F;
					case Format::R32_Uint:
						return GL_R32UI;
					case Format::R32_Sint:
						return GL_R32I;
					case Format::R24G8_Typeless:
						return GL_DEPTH24_STENCIL8;
					case Format::D24_Unorm_S8_Uint:
						return GL_DEPTH24_STENCIL8;
					case Format::R24_Unorm_X8_Typeless:
						return GL_DEPTH24_STENCIL8;
					case Format::X24_Typeless_G8_Uint:
						return GL_DEPTH24_STENCIL8;
					case Format::R8G8_Typeless:
						return GL_RG8UI;
					case Format::R8G8_Unorm:
						return GL_RG8;
					case Format::R8G8_Uint:
						return GL_RG8UI;
					case Format::R8G8_Snorm:
						return GL_RG8I;
					case Format::R8G8_Sint:
						return GL_RG8I;
					case Format::R16_Typeless:
						return GL_R16UI;
					case Format::R16_Float:
						return GL_R16F;
					case Format::D16_Unorm:
						return GL_R16;
					case Format::R16_Unorm:
						return GL_R16;
					case Format::R16_Uint:
						return GL_R16UI;
					case Format::R16_Snorm:
						return GL_R16I;
					case Format::R16_Sint:
						return GL_R16I;
					case Format::R8_Typeless:
						return GL_R8UI;
					case Format::R8_Unorm:
						return GL_R8;
					case Format::R8_Uint:
						return GL_R8UI;
					case Format::R8_Snorm:
						return GL_R8I;
					case Format::R8_Sint:
						return GL_R8I;
					case Format::A8_Unorm:
						return GL_ALPHA8_EXT;
					case Format::R1_Unorm:
						return GL_R8;
					case Format::R9G9B9E5_Share_Dexp:
						return GL_RGB9_E5;
					case Format::R8G8_B8G8_Unorm:
						return GL_RGB;
					default:
						break;
				}

				return GL_RGB;
			}
			GLenum OGLDevice::GetTextureAddress(TextureAddress Value)
			{
				switch (Value)
				{
					case TextureAddress::Wrap:
						return GL_REPEAT;
					case TextureAddress::Mirror:
						return GL_MIRRORED_REPEAT;
					case TextureAddress::Clamp:
						return GL_CLAMP_TO_EDGE;
					case TextureAddress::Border:
						return GL_CLAMP_TO_BORDER;
					case TextureAddress::Mirror_Once:
						return GL_MIRROR_CLAMP_TO_EDGE;
				}

				return GL_REPEAT;
			}
			GLenum OGLDevice::GetComparison(Comparison Value)
			{
				switch (Value)
				{
					case Comparison::Never:
						return GL_NEVER;
					case Comparison::Less:
						return GL_LESS;
					case Comparison::Equal:
						return GL_EQUAL;
					case Comparison::Less_Equal:
						return GL_LEQUAL;
					case Comparison::Greater:
						return GL_GREATER;
					case Comparison::Not_Equal:
						return GL_NOTEQUAL;
					case Comparison::Greater_Equal:
						return GL_GEQUAL;
					case Comparison::Always:
						return GL_ALWAYS;
				}

				return GL_ALWAYS;
			}
			GLenum OGLDevice::GetPixelFilter(PixelFilter Value, bool Mag)
			{
				switch (Value)
				{
					case PixelFilter::Min_Mag_Mip_Point:
					case PixelFilter::Compare_Min_Mag_Mip_Point:
						return GL_NEAREST;
					case PixelFilter::Min_Mag_Point_Mip_Linear:
					case PixelFilter::Compare_Min_Mag_Point_Mip_Linear:
						return (Mag ? GL_NEAREST : GL_NEAREST_MIPMAP_LINEAR);
					case PixelFilter::Min_Point_Mag_Linear_Mip_Point:
					case PixelFilter::Compare_Min_Point_Mag_Linear_Mip_Point:
						return (Mag ? GL_LINEAR : GL_NEAREST_MIPMAP_NEAREST);
					case PixelFilter::Min_Point_Mag_Mip_Linear:
					case PixelFilter::Compare_Min_Point_Mag_Mip_Linear:
						return (Mag ? GL_LINEAR : GL_NEAREST_MIPMAP_LINEAR);
					case PixelFilter::Min_Linear_Mag_Mip_Point:
					case PixelFilter::Compare_Min_Linear_Mag_Mip_Point:
						return (Mag ? GL_NEAREST : GL_LINEAR_MIPMAP_NEAREST);
					case PixelFilter::Min_Linear_Mag_Point_Mip_Linear:
					case PixelFilter::Compare_Min_Linear_Mag_Point_Mip_Linear:
						return (Mag ? GL_NEAREST : GL_LINEAR_MIPMAP_LINEAR);
					case PixelFilter::Min_Mag_Linear_Mip_Point:
					case PixelFilter::Compare_Min_Mag_Linear_Mip_Point:
						return (Mag ? GL_LINEAR : GL_LINEAR_MIPMAP_NEAREST);
					case PixelFilter::Min_Mag_Mip_Linear:
					case PixelFilter::Compare_Min_Mag_Mip_Linear:
						return (Mag ? GL_LINEAR : GL_LINEAR_MIPMAP_LINEAR);
					case PixelFilter::Anistropic:
					case PixelFilter::Compare_Anistropic:
						return GL_LINEAR;
				}

				return GL_LINEAR;
			}
			GLenum OGLDevice::GetBlendOperation(BlendOperation Value)
			{
				switch (Value)
				{
					case BlendOperation::Add:
						return GL_FUNC_ADD;
					case BlendOperation::Subtract:
						return GL_FUNC_SUBTRACT;
					case BlendOperation::Subtract_Reverse:
						return GL_FUNC_REVERSE_SUBTRACT;
					case BlendOperation::Min:
						return GL_MIN;
					case BlendOperation::Max:
						return GL_MAX;
				}

				return GL_FUNC_ADD;
			}
			GLenum OGLDevice::GetBlend(Blend Value)
			{
				switch (Value)
				{
					case Blend::Zero:
						return GL_ZERO;
					case Blend::One:
						return GL_ONE;
					case Blend::Source_Color:
						return GL_SRC_COLOR;
					case Blend::Source_Color_Invert:
						return GL_ONE_MINUS_SRC_COLOR;
					case Blend::Source_Alpha:
						return GL_SRC_ALPHA;
					case Blend::Source_Alpha_Invert:
						return GL_ONE_MINUS_SRC_ALPHA;
					case Blend::Destination_Alpha:
						return GL_DST_ALPHA;
					case Blend::Destination_Alpha_Invert:
						return GL_ONE_MINUS_DST_ALPHA;
					case Blend::Destination_Color:
						return GL_DST_COLOR;
					case Blend::Destination_Color_Invert:
						return GL_ONE_MINUS_DST_COLOR;
					case Blend::Source_Alpha_SAT:
						return GL_SRC_ALPHA_SATURATE;
					case Blend::Blend_Factor:
						return GL_CONSTANT_COLOR;
					case Blend::Blend_Factor_Invert:
						return GL_ONE_MINUS_CONSTANT_ALPHA;
					case Blend::Source1_Color:
						return GL_SRC1_COLOR;
					case Blend::Source1_Color_Invert:
						return GL_ONE_MINUS_SRC1_COLOR;
					case Blend::Source1_Alpha:
						return GL_SRC1_ALPHA;
					case Blend::Source1_Alpha_Invert:
						return GL_ONE_MINUS_SRC1_ALPHA;
				}

				return GL_ONE;
			}
			GLenum OGLDevice::GetStencilOperation(StencilOperation Value)
			{
				switch (Value)
				{
					case StencilOperation::Keep:
						return GL_KEEP;
					case StencilOperation::Zero:
						return GL_ZERO;
					case StencilOperation::Replace:
						return GL_REPLACE;
					case StencilOperation::SAT_Add:
						return GL_INCR_WRAP;
					case StencilOperation::SAT_Subtract:
						return GL_DECR_WRAP;
					case StencilOperation::Invert:
						return GL_INVERT;
					case StencilOperation::Add:
						return GL_INCR;
					case StencilOperation::Subtract:
						return GL_DECR;
				}

				return GL_KEEP;
			}
			GLenum OGLDevice::GetPrimitiveTopology(PrimitiveTopology Value)
			{
				switch (Value)
				{
					case PrimitiveTopology::Point_List:
						return GL_POINT;
					case PrimitiveTopology::Line_List:
					case PrimitiveTopology::Line_Strip:
					case PrimitiveTopology::Line_List_Adj:
					case PrimitiveTopology::Line_Strip_Adj:
						return GL_LINE;
					case PrimitiveTopology::Triangle_List:
					case PrimitiveTopology::Triangle_Strip:
					case PrimitiveTopology::Triangle_List_Adj:
					case PrimitiveTopology::Triangle_Strip_Adj:
						return GL_FILL;
					default:
						break;
				}

				return GL_FILL;
			}
			GLenum OGLDevice::GetPrimitiveTopologyDraw(PrimitiveTopology Value)
			{
				switch (Value)
				{
					case PrimitiveTopology::Point_List:
						return GL_POINTS;
					case PrimitiveTopology::Line_List:
						return GL_LINES;
					case PrimitiveTopology::Line_Strip:
						return GL_LINE_STRIP;
					case PrimitiveTopology::Line_List_Adj:
						return GL_LINES_ADJACENCY;
					case PrimitiveTopology::Line_Strip_Adj:
						return GL_LINE_STRIP_ADJACENCY;
					case PrimitiveTopology::Triangle_List:
						return GL_TRIANGLES;
					case PrimitiveTopology::Triangle_Strip:
						return GL_TRIANGLE_STRIP;
					case PrimitiveTopology::Triangle_List_Adj:
						return GL_TRIANGLES_ADJACENCY;
					case PrimitiveTopology::Triangle_Strip_Adj:
						return GL_TRIANGLE_STRIP_ADJACENCY;
					default:
						break;
				}

				return GL_TRIANGLES;
			}
			GLenum OGLDevice::GetResourceBind(ResourceBind Value)
			{
				switch (Value)
				{
					case ResourceBind::Vertex_Buffer:
						return GL_ARRAY_BUFFER;
					case ResourceBind::Index_Buffer:
						return GL_ELEMENT_ARRAY_BUFFER;
					case ResourceBind::Constant_Buffer:
						return GL_UNIFORM_BUFFER;
					case ResourceBind::Shader_Input:
						return GL_SHADER_STORAGE_BUFFER;
					case ResourceBind::Stream_Output:
						return GL_TEXTURE_BUFFER;
					case ResourceBind::Render_Target:
						return GL_DRAW_INDIRECT_BUFFER;
					case ResourceBind::Depth_Stencil:
						return GL_DRAW_INDIRECT_BUFFER;
					case ResourceBind::Unordered_Access:
						return GL_DISPATCH_INDIRECT_BUFFER;
				}

				return GL_ARRAY_BUFFER;
			}
			GLenum OGLDevice::GetResourceMap(ResourceMap Value)
			{
				switch (Value)
				{
					case ResourceMap::Read:
						return GL_READ_ONLY;
					case ResourceMap::Write:
						return GL_WRITE_ONLY;
					case ResourceMap::Read_Write:
						return GL_READ_WRITE;
					case ResourceMap::Write_Discard:
						return GL_WRITE_ONLY;
					case ResourceMap::Write_No_Overwrite:
						return GL_WRITE_ONLY;
				}

				return GL_READ_ONLY;
			}
			void OGLDevice::DebugMessage(GLenum Source, GLenum Type, GLuint Id, GLenum Severity, GLsizei Length, const GLchar* Message, const void* Data)
			{
				const char* _Source, * _Type;
				switch (Source)
				{
					case GL_DEBUG_SOURCE_API:
						_Source = "API";
						break;
					case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
						_Source = "WINDOW SYSTEM";
						break;
					case GL_DEBUG_SOURCE_SHADER_COMPILER:
						_Source = "SHADER COMPILER";
						break;
					case GL_DEBUG_SOURCE_THIRD_PARTY:
						_Source = "THIRD PARTY";
						break;
					case GL_DEBUG_SOURCE_APPLICATION:
						_Source = "APPLICATION";
						break;
					default:
						_Source = "NONE";
						break;
				}

				switch (Type)
				{
					case GL_DEBUG_TYPE_ERROR:
						_Type = "ERROR";
						break;
					case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
						_Type = "DEPRECATED";
						break;
					case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
						_Type = "UDEFINED";
						break;
					case GL_DEBUG_TYPE_PORTABILITY:
						_Type = "PORTABILITY";
						break;
					case GL_DEBUG_TYPE_PERFORMANCE:
						_Type = "PERFORMANCE";
						break;
					case GL_DEBUG_TYPE_OTHER:
						_Type = "OTHER";
						break;
					case GL_DEBUG_TYPE_MARKER:
						_Type = "MARKER";
						break;
					default:
						_Type = "NONE";
						break;
				}

				switch (Severity)
				{
					case GL_DEBUG_SEVERITY_HIGH:
						TH_ERR("%s (%s:%d): %s", _Source, _Type, Id, Message);
						break;
					case GL_DEBUG_SEVERITY_MEDIUM:
						TH_WARN("%s (%s:%d): %s", _Source, _Type, Id, Message);
						break;
				}
			}
		}
	}
}
#endif
