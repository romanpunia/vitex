#include "ogl.h"
#ifdef VI_GL
#define SHADER_VERTEX ".text.vertex.gz"
#define SHADER_PIXEL ".text.pixel.gz"
#define SHADER_GEOMETRY ".text.geometry.gz"
#define SHADER_COMPUTE ".text.compute.gz"
#define SHADER_HULL ".text.hull.gz"
#define SHADER_DOMAIN ".text.domain.gz"
#define REG_EXCHANGE(Name, Value) { if (Register.Name == Value) return; Register.Name = Value; }
#define REG_EXCHANGE_T2(Name, Value1, Value2) { if (std::get<0>(Register.Name) == Value1 && std::get<1>(Register.Name) == Value2) return; Register.Name = std::make_tuple(Value1, Value2); }
#define REG_EXCHANGE_T3(Name, Value1, Value2, Value3) { if (std::get<0>(Register.Name) == Value1 && std::get<1>(Register.Name) == Value2 && std::get<2>(Register.Name) == Value3) return; Register.Name = std::make_tuple(Value1, Value2, Value3); }
#define OGL_OFFSET(i) (GLvoid*)(i)
#define OGL_VOFFSET(i) ((char*)nullptr + (i))
#define OGL_INLINE(Code) #Code
#pragma warning(push)
#pragma warning(disable: 4996)

namespace
{
	template <class T>
	static inline void Rehash(uint64_t& Seed, const T& Value)
	{
		Seed ^= std::hash<T>()(Value) + 0x9e3779b9 + (Seed << 6) + (Seed >> 2);
	}
	static int64_t D3D_GetCoordY(int64_t Y, int64_t Height, int64_t WindowHeight)
	{
		return WindowHeight - Height - Y;
	}
	static int64_t OGL_GetCoordY(int64_t Y, int64_t Height, int64_t WindowHeight)
	{
		return WindowHeight - Height - Y;
	}
	static void OGL_CopyTexture_4_3(GLenum Target, GLuint SrcName, GLuint DestName, GLint Width, GLint Height)
	{
		glCopyImageSubData(SrcName, Target, 0, 0, 0, 0, DestName, Target, 0, 0, 0, 0, Width, Height, 1);
	}
	static void OGL_CopyTexture_3_0(GLuint SrcName, GLuint DestName, GLint Width, GLint Height)
	{
		GLint LastFrameBuffer = 0;
		glGetIntegerv(GL_FRAMEBUFFER_BINDING, &LastFrameBuffer);

		GLuint FrameBuffer = 0;
		glGenFramebuffers(1, &FrameBuffer);
		glBindFramebuffer(GL_FRAMEBUFFER, FrameBuffer);
		glFramebufferTexture(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, SrcName, 0);
		glFramebufferTexture(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, DestName, 0);
		glNamedFramebufferDrawBuffer(FrameBuffer, GL_COLOR_ATTACHMENT1);
		glBlitFramebuffer(0, 0, Width, Height, 0, 0, Width, Height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
		glBindFramebuffer(GL_FRAMEBUFFER, LastFrameBuffer);
		glDeleteFramebuffers(1, &FrameBuffer);
	}
	static void OGL_CopyTextureFace2D_3_0(Vitex::Compute::CubeFace Face, GLuint SrcName, GLuint DestName, GLint Width, GLint Height)
	{
		GLint LastFrameBuffer = 0;
		glGetIntegerv(GL_FRAMEBUFFER_BINDING, &LastFrameBuffer);

		GLuint FrameBuffer = 0;
		glGenFramebuffers(1, &FrameBuffer);
		glBindFramebuffer(GL_FRAMEBUFFER, FrameBuffer);
		glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + (uint32_t)Face, SrcName, 0);
		glFramebufferTexture(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, DestName, 0);
		glNamedFramebufferDrawBuffer(FrameBuffer, GL_COLOR_ATTACHMENT1);
		glBlitFramebuffer(0, 0, Width, Height, 0, 0, Width, Height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
		glBindFramebuffer(GL_FRAMEBUFFER, LastFrameBuffer);
		glDeleteFramebuffers(1, &FrameBuffer);
	}
	static Vitex::Graphics::GraphicsException GetException(const char* ScopeText)
	{
		GLenum ResultCode = glGetError();
		Vitex::Core::String Text = ScopeText;
		switch (ResultCode)
		{
			case GL_INVALID_ENUM:
				Text += " causing invalid enum parameter";
				break;
			case GL_INVALID_VALUE:
				Text += " causing invalid argument parameter";
				break;
			case GL_INVALID_OPERATION:
				Text += " causing invalid operation";
				break;
			case GL_STACK_OVERFLOW:
				Text += " causing stack overflow";
				break;
			case GL_STACK_UNDERFLOW:
				Text += " causing stack underflow";
				break;
			case GL_OUT_OF_MEMORY:
				Text += " causing out of memory";
				break;
			case GL_INVALID_FRAMEBUFFER_OPERATION:
				Text += " causing invalid frame buffer operation";
				break;
			case GL_CONTEXT_LOST:
				Text += " causing graphics system reset";
				break;
			case GL_TABLE_TOO_LARGE:
				Text += " causing table too large";
				break;
			case GL_NO_ERROR:
			default:
				Text += " causing internal graphics error";
				break;
		}
		return Vitex::Graphics::GraphicsException((int)ResultCode, std::move(Text));
	}
}

namespace Vitex
{
	namespace Graphics
	{
		namespace OGL
		{
			OGLFrameBuffer::OGLFrameBuffer(GLuint Targets) : Count(Targets)
			{
			}
			void OGLFrameBuffer::Cleanup()
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
			void* OGLDepthStencilState::GetResource() const
			{
				return (void*)&State;
			}

			OGLRasterizerState::OGLRasterizerState(const Desc& I) : RasterizerState(I)
			{
			}
			OGLRasterizerState::~OGLRasterizerState()
			{
			}
			void* OGLRasterizerState::GetResource() const
			{
				return (void*)&State;
			}

			OGLBlendState::OGLBlendState(const Desc& I) : BlendState(I)
			{
			}
			OGLBlendState::~OGLBlendState()
			{
			}
			void* OGLBlendState::GetResource() const
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
			void* OGLSamplerState::GetResource() const
			{
				return (void*)(intptr_t)Resource;
			}

			OGLInputLayout::OGLInputLayout(const Desc& I) : InputLayout(I)
			{
				const auto LayoutSize = [this]()
				{
					size_t Size = 0;
					for (auto Item : VertexLayout)
						Size += Item.second.size();
					return Size;
				};

				for (size_t i = 0; i < I.Attributes.size(); i++)
				{
					const Attribute& It = I.Attributes[i];
					GLenum Format = GL_INVALID_ENUM;
					size_t Stride = It.AlignedByteOffset;
					GLint Size = It.Components;
					GLint Normalize = GL_FALSE;
					size_t BufferSlot = (size_t)It.Slot;
					bool PerVertex = It.PerVertex;

					switch (It.Format)
					{
						case AttributeType::Byte:
							Format = GL_BYTE;
							Normalize = GL_TRUE;
							break;
						case AttributeType::Ubyte:
							Format = GL_UNSIGNED_BYTE;
							Normalize = GL_TRUE;
							break;
						case AttributeType::Half:
							Format = GL_HALF_FLOAT;
							break;
						case AttributeType::Float:
							Format = GL_FLOAT;
							break;
						case AttributeType::Matrix:
							Format = GL_FLOAT;
							Size = 4;
							break;
						case AttributeType::Int:
							Format = GL_INT;
							break;
						case AttributeType::Uint:
							Format = GL_UNSIGNED_INT;
							break;
						default:
							break;
					}

					auto& Layout = VertexLayout[BufferSlot];
					if (It.Format == AttributeType::Matrix)
					{
						for (size_t j = 0; j < 4; j++)
						{
							size_t Offset = LayoutSize(), Substride = Stride + sizeof(float) * Size * j;
							Layout.emplace_back([Offset, Format, Normalize, Substride, Size, PerVertex](size_t Width)
							{
								glEnableVertexAttribArray((GLuint)Offset);
								glVertexAttribPointer((GLuint)Offset, Size, Format, Normalize, (GLsizei)Width, OGL_VOFFSET(Substride));
								glVertexAttribDivisor((GLuint)Offset, PerVertex ? 0 : 1);
							});
						}
					}
					else
					{
						size_t Offset = LayoutSize();
						Layout.emplace_back([Offset, Format, Normalize, Stride, Size, PerVertex](size_t Width)
						{
							glEnableVertexAttribArray((GLuint)Offset);
							glVertexAttribPointer((GLuint)Offset, Size, Format, Normalize, (GLsizei)Width, OGL_VOFFSET(Stride));
							glVertexAttribDivisor((GLuint)Offset, PerVertex ? 0 : 1);
						});
					}
				}
			}
			OGLInputLayout::~OGLInputLayout()
			{
				for (auto& Item : Layouts)
					glDeleteVertexArrays(1, &Item.second);

				if (DynamicResource != GL_NONE)
					glDeleteVertexArrays(1, &DynamicResource);
			}
			void* OGLInputLayout::GetResource() const
			{
				return (void*)this;
			}
			Core::String OGLInputLayout::GetLayoutHash(OGLElementBuffer** Buffers, uint32_t Count)
			{
				Core::String Hash;
				if (!Buffers || !Count)
					return Hash;

				for (uint32_t i = 0; i < Count; i++)
					Hash += Core::ToString((uintptr_t)(void*)Buffers[i]);

				return Hash;
			}

			OGLShader::OGLShader(const Desc& I) : Shader(I), Compiled(false)
			{
			}
			OGLShader::~OGLShader()
			{
				for (auto& Program : Programs)
				{
					auto* Device = Program.second;
					auto It = Device->Register.Programs.begin();
					while (It != Device->Register.Programs.end())
					{
						if (It->second == Program.first)
						{
							Device->Register.Programs.erase(It);
							It = Device->Register.Programs.begin();
						}
						else
							++It;
					}
					glDeleteProgram(Program.first);
				}

				glDeleteShader(VertexShader);
				glDeleteShader(PixelShader);
				glDeleteShader(GeometryShader);
				glDeleteShader(DomainShader);
				glDeleteShader(HullShader);
				glDeleteShader(ComputeShader);
			}
			bool OGLShader::IsValid() const
			{
				return Compiled;
			}

			OGLElementBuffer::OGLElementBuffer(const Desc& I) : ElementBuffer(I)
			{
			}
			OGLElementBuffer::~OGLElementBuffer()
			{
				for (auto& Link : Bindings)
				{
					auto* Layout = Link.second;
					if (Layout->DynamicResource == Link.first)
						Layout->DynamicResource = GL_NONE;

					auto It = Layout->Layouts.begin();
					while (It != Layout->Layouts.end())
					{
						if (It->second == Link.first)
						{
							Layout->Layouts.erase(It);
							It = Layout->Layouts.begin();
						}
						else
							++It;
					}
					glDeleteVertexArrays(1, &Link.first);
				}

				glDeleteBuffers(1, &Resource);
			}
			void* OGLElementBuffer::GetResource() const
			{
				return (void*)(intptr_t)Resource;
			}

			OGLMeshBuffer::OGLMeshBuffer(const Desc& I) : MeshBuffer(I)
			{
			}
			Compute::Vertex* OGLMeshBuffer::GetElements(GraphicsDevice* Device) const
			{
				VI_ASSERT(Device != nullptr, "graphics device should be set");

				MappedSubresource Resource;
				Device->Map(VertexBuffer, ResourceMap::Write, &Resource);

				Compute::Vertex* Vertices = Core::Memory::Allocate<Compute::Vertex>(sizeof(Compute::Vertex) * (uint32_t)VertexBuffer->GetElements());
				memcpy(Vertices, Resource.Pointer, (size_t)VertexBuffer->GetElements() * sizeof(Compute::Vertex));

				Device->Unmap(VertexBuffer, &Resource);
				return Vertices;
			}

			OGLSkinMeshBuffer::OGLSkinMeshBuffer(const Desc& I) : SkinMeshBuffer(I)
			{
			}
			Compute::SkinVertex* OGLSkinMeshBuffer::GetElements(GraphicsDevice* Device) const
			{
				VI_ASSERT(Device != nullptr, "graphics device should be set");

				MappedSubresource Resource;
				Device->Map(VertexBuffer, ResourceMap::Write, &Resource);

				Compute::SkinVertex* Vertices = Core::Memory::Allocate<Compute::SkinVertex>(sizeof(Compute::SkinVertex) * (uint32_t)VertexBuffer->GetElements());
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
			void* OGLTexture2D::GetResource() const
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
			void* OGLTextureCube::GetResource() const
			{
				return (void*)(intptr_t)Resource;
			}

			OGLDepthTarget2D::OGLDepthTarget2D(const Desc& I) : Graphics::DepthTarget2D(I)
			{
			}
			OGLDepthTarget2D::~OGLDepthTarget2D()
			{
				glDeleteFramebuffers(1, &FrameBuffer);
				glDeleteTextures(1, &DepthTexture);
			}
			void* OGLDepthTarget2D::GetResource() const
			{
				return (void*)(intptr_t)FrameBuffer;
			}
			uint32_t OGLDepthTarget2D::GetWidth() const
			{
				return (uint32_t)Viewarea.Width;
			}
			uint32_t OGLDepthTarget2D::GetHeight() const
			{
				return (uint32_t)Viewarea.Height;
			}

			OGLDepthTargetCube::OGLDepthTargetCube(const Desc& I) : Graphics::DepthTargetCube(I)
			{
			}
			OGLDepthTargetCube::~OGLDepthTargetCube()
			{
				glDeleteFramebuffers(1, &FrameBuffer);
				glDeleteTextures(1, &DepthTexture);
			}
			void* OGLDepthTargetCube::GetResource() const
			{
				return (void*)(intptr_t)FrameBuffer;
			}
			uint32_t OGLDepthTargetCube::GetWidth() const
			{
				return (uint32_t)Viewarea.Width;
			}
			uint32_t OGLDepthTargetCube::GetHeight() const
			{
				return (uint32_t)Viewarea.Height;
			}

			OGLRenderTarget2D::OGLRenderTarget2D(const Desc& I) : RenderTarget2D(I), FrameBuffer(1)
			{
			}
			OGLRenderTarget2D::~OGLRenderTarget2D()
			{
				glDeleteTextures(1, &DepthTexture);
				FrameBuffer.Cleanup();
			}
			void* OGLRenderTarget2D::GetTargetBuffer() const
			{
				return (void*)&FrameBuffer;
			}
			void* OGLRenderTarget2D::GetDepthBuffer() const
			{
				return (void*)(intptr_t)DepthTexture;
			}
			uint32_t OGLRenderTarget2D::GetWidth() const
			{
				return (uint32_t)Viewarea.Width;
			}
			uint32_t OGLRenderTarget2D::GetHeight() const
			{
				return (uint32_t)Viewarea.Height;
			}

			OGLMultiRenderTarget2D::OGLMultiRenderTarget2D(const Desc& I) : MultiRenderTarget2D(I), FrameBuffer((GLuint)I.Target)
			{
			}
			OGLMultiRenderTarget2D::~OGLMultiRenderTarget2D()
			{
				glDeleteTextures(1, &DepthTexture);
				FrameBuffer.Cleanup();
			}
			void* OGLMultiRenderTarget2D::GetTargetBuffer() const
			{
				return (void*)&FrameBuffer;
			}
			void* OGLMultiRenderTarget2D::GetDepthBuffer() const
			{
				return (void*)(intptr_t)DepthTexture;
			}
			uint32_t OGLMultiRenderTarget2D::GetWidth() const
			{
				return (uint32_t)Viewarea.Width;
			}
			uint32_t OGLMultiRenderTarget2D::GetHeight() const
			{
				return (uint32_t)Viewarea.Height;
			}

			OGLRenderTargetCube::OGLRenderTargetCube(const Desc& I) : RenderTargetCube(I), FrameBuffer(1)
			{
			}
			OGLRenderTargetCube::~OGLRenderTargetCube()
			{
				glDeleteTextures(1, &DepthTexture);
				FrameBuffer.Cleanup();
			}
			void* OGLRenderTargetCube::GetTargetBuffer() const
			{
				return (void*)&FrameBuffer;
			}
			void* OGLRenderTargetCube::GetDepthBuffer() const
			{
				return (void*)(intptr_t)DepthTexture;
			}
			uint32_t OGLRenderTargetCube::GetWidth() const
			{
				return (uint32_t)Viewarea.Width;
			}
			uint32_t OGLRenderTargetCube::GetHeight() const
			{
				return (uint32_t)Viewarea.Height;
			}

			OGLMultiRenderTargetCube::OGLMultiRenderTargetCube(const Desc& I) : MultiRenderTargetCube(I), FrameBuffer((GLuint)I.Target)
			{
			}
			OGLMultiRenderTargetCube::~OGLMultiRenderTargetCube()
			{
				glDeleteTextures(1, &DepthTexture);
				FrameBuffer.Cleanup();
			}
			void* OGLMultiRenderTargetCube::GetTargetBuffer() const
			{
				return (void*)&FrameBuffer;
			}
			void* OGLMultiRenderTargetCube::GetDepthBuffer() const
			{
				return (void*)(intptr_t)DepthTexture;
			}
			uint32_t OGLMultiRenderTargetCube::GetWidth() const
			{
				return (uint32_t)Viewarea.Width;
			}
			uint32_t OGLMultiRenderTargetCube::GetHeight() const
			{
				return (uint32_t)Viewarea.Height;
			}

			OGLCubemap::OGLCubemap(const Desc& I) : Cubemap(I)
			{
				VI_ASSERT(I.Source != nullptr, "source should be set");
				VI_ASSERT(I.Target < I.Source->GetTargetCount(), "targets count should be less than %i", (int)I.Source->GetTargetCount());
				
				OGLTexture2D* Target = (OGLTexture2D*)I.Source->GetTarget2D(I.Target);
				VI_ASSERT(Target != nullptr && Target->Resource != GL_NONE, "render target should be valid");
				VI_ASSERT(!((OGLFrameBuffer*)I.Source->GetTargetBuffer())->Backbuffer, "cannot copy from backbuffer directly");

				glGenFramebuffers(1, &FrameBuffer);
				Source = Target->Resource;
				Options.SizeFormat = OGLDevice::GetSizedFormat(Target->GetFormatMode());
				Options.FormatMode = Target->GetFormatMode();
			}
			OGLCubemap::~OGLCubemap()
			{
				glDeleteFramebuffers(1, &FrameBuffer);
			}

			OGLQuery::OGLQuery() : Query()
			{
			}
			OGLQuery::~OGLQuery()
			{
				glDeleteQueries(1, &Async);
			}
			void* OGLQuery::GetResource() const
			{
				return (void*)(intptr_t)Async;
			}

			OGLDevice::OGLDevice(const Desc& I) : GraphicsDevice(I), ShaderVersion(""), Window(I.Window), Context(nullptr)
			{
				if (!Window)
				{
					VI_ASSERT(VirtualWindow != nullptr, "cannot initialize virtual activity for device");
					Window = VirtualWindow;
				}

				int R, G, B, A;
				GetBackBufferSize(I.BufferFormat, &R, &G, &B, &A);
				VI_PANIC(Video::GLEW::SetSwapParameters(R, G, B, A, I.Debug), "OGL configuration failed");
				if (!Window->GetHandle())
				{
					Window->BuildLayer(Backend);
					VI_PANIC(Window->GetHandle(), "activity creation failed %s", Window->GetError().c_str());
				}
				
				Context = Video::GLEW::CreateContext(Window);
				VI_PANIC(Context != nullptr, "OGL context creation failed %s", Window->GetError().c_str());
				SetAsCurrentDevice();

				static const GLenum ErrorCode = glewInit();
				VI_PANIC(ErrorCode == GLEW_OK, "OGL extension layer initialization failed reason: %i", (int)ErrorCode);
				if (I.Debug)
				{
					glEnable(GL_DEBUG_OUTPUT);
					glDebugMessageCallback(DebugMessage, nullptr);
					glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_TRUE);
				}

				Register.Programs[GetProgramHash()] = GL_NONE;
				SetShaderModel(I.ShaderMode == ShaderModel::Auto ? GetSupportedShaderModel() : I.ShaderMode);
				ResizeBuffers(I.BufferWidth, I.BufferHeight);
				CreateStates();

				glEnable(GL_TEXTURE_2D);
				glEnable(GL_TEXTURE_3D);
				glEnable(GL_TEXTURE_CUBE_MAP);
			}
			OGLDevice::~OGLDevice()
			{
				ReleaseProxy();
				for (auto& It : Register.Programs)
					glDeleteProgram(It.second);

				glDeleteShader(Immediate.VertexShader);
				glDeleteShader(Immediate.PixelShader);
				glDeleteProgram(Immediate.Program);
				glDeleteVertexArrays(1, &Immediate.VertexArray);
				glDeleteBuffers(1, &Immediate.VertexBuffer);
				Video::GLEW::DestroyContext(Context);
			}
			void OGLDevice::SetAsCurrentDevice()
			{
				Video::GLEW::SetContext(Window, Context);
				SetVSyncMode(VSyncMode);
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
				VI_ASSERT(State != nullptr, "blend state should be set");
				OGLBlendState* NewState = (OGLBlendState*)State;
				OGLBlendState* OldState = Register.Blend;
				REG_EXCHANGE(Blend, NewState);

				if (!OldState || OldState->State.AlphaToCoverageEnable != NewState->State.AlphaToCoverageEnable)
				{
					if (NewState->State.AlphaToCoverageEnable)
					{
						glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);
						glSampleCoverage(1.0f, GL_FALSE);
					}
					else
					{
						glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);
						glSampleCoverage(0.0f, GL_FALSE);
					}
				}

				if (NewState->State.IndependentBlendEnable)
				{
					for (uint32_t i = 0; i < 8; i++)
					{
						auto& Base = NewState->State.RenderTarget[i];
						if (OldState && memcmp(&OldState->State.RenderTarget[i], &Base, sizeof(RenderTargetBlendState)) == 0)
							continue;
						
						if (NewState->State.RenderTarget[i].BlendEnable)
							glEnablei(GL_BLEND, i);
						else
							glDisablei(GL_BLEND, i);

						glBlendEquationSeparatei(i, GetBlendOperation(NewState->State.RenderTarget[i].BlendOperationMode), GetBlendOperation(NewState->State.RenderTarget[i].BlendOperationAlpha));
						glBlendFuncSeparatei(i, GetBlend(NewState->State.RenderTarget[i].SrcBlend), GetBlend(NewState->State.RenderTarget[i].DestBlend), GetBlend(NewState->State.RenderTarget[i].SrcBlendAlpha), GetBlend(NewState->State.RenderTarget[i].DestBlendAlpha));
					}
				}
				else
				{
					if (OldState != nullptr)
					{
						if (OldState->State.IndependentBlendEnable)
						{
							for (uint32_t i = 0; i < 8; i++)
								glDisablei(GL_BLEND, i);
						}

						if (memcmp(&OldState->State.RenderTarget[0], &NewState->State.RenderTarget[0], sizeof(RenderTargetBlendState)) == 0)
							return;
					}

					if (NewState->State.RenderTarget[0].BlendEnable)
						glEnable(GL_BLEND);
					else
						glDisable(GL_BLEND);

					glBlendEquationSeparate(GetBlendOperation(NewState->State.RenderTarget[0].BlendOperationMode), GetBlendOperation(NewState->State.RenderTarget[0].BlendOperationAlpha));
					glBlendFuncSeparate(GetBlend(NewState->State.RenderTarget[0].SrcBlend), GetBlend(NewState->State.RenderTarget[0].DestBlend), GetBlend(NewState->State.RenderTarget[0].SrcBlendAlpha), GetBlend(NewState->State.RenderTarget[0].DestBlendAlpha));
				}
			}
			void OGLDevice::SetRasterizerState(RasterizerState* State)
			{
				VI_ASSERT(State != nullptr, "rasterizer state should be set");
				OGLRasterizerState* NewState = (OGLRasterizerState*)State;
				OGLRasterizerState* OldState = Register.Rasterizer;
				REG_EXCHANGE(Rasterizer, NewState);

				bool WasMultisampled = OldState ? (OldState->State.AntialiasedLineEnable || OldState->State.MultisampleEnable) : false;
				bool Multisampled = (NewState->State.AntialiasedLineEnable || NewState->State.MultisampleEnable);
				if (!OldState || WasMultisampled != Multisampled)
				{
					if (Multisampled)
						glEnable(GL_MULTISAMPLE);
					else
						glDisable(GL_MULTISAMPLE);
				}

				if (!OldState || OldState->State.CullMode != NewState->State.CullMode)
				{
					if (NewState->State.CullMode == VertexCull::Back)
					{
						glCullFace(GL_FRONT);
						glEnable(GL_CULL_FACE);
					}
					else if (NewState->State.CullMode == VertexCull::Front)
					{
						glCullFace(GL_BACK);
						glEnable(GL_CULL_FACE);
					}
					else
						glDisable(GL_CULL_FACE);
				}

				if (!OldState || OldState->State.ScissorEnable != NewState->State.ScissorEnable)
				{
					if (NewState->State.ScissorEnable)
						glEnable(GL_SCISSOR_TEST);
					else
						glDisable(GL_SCISSOR_TEST);
				}

				if (!OldState || OldState->State.FillMode != NewState->State.FillMode)
				{
					if (NewState->State.FillMode == SurfaceFill::Solid)
						glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
					else
						glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
				}

				if (!OldState || OldState->State.FrontCounterClockwise != NewState->State.FrontCounterClockwise)
				{
					if (NewState->State.FrontCounterClockwise)
						glFrontFace(GL_CW);
					else
						glFrontFace(GL_CCW);
				}
			}
			void OGLDevice::SetDepthStencilState(DepthStencilState* State)
			{
				VI_ASSERT(State != nullptr, "depth stencil state should be set");
				OGLDepthStencilState* NewState = (OGLDepthStencilState*)State;
				OGLDepthStencilState* OldState = Register.DepthStencil;
				REG_EXCHANGE(DepthStencil, NewState);

				if (!OldState || OldState->State.DepthEnable != NewState->State.DepthEnable)
				{
					if (NewState->State.DepthEnable)
						glEnable(GL_DEPTH_TEST);
					else
						glDisable(GL_DEPTH_TEST);
				}

				if (!OldState || OldState->State.StencilEnable != NewState->State.StencilEnable)
				{
					if (NewState->State.StencilEnable)
						glEnable(GL_STENCIL_TEST);
					else
						glDisable(GL_STENCIL_TEST);
				}

				if (OldState != nullptr)
				{
					if (OldState->State.DepthFunction != NewState->State.DepthFunction)
						glDepthFunc(GetComparison(NewState->State.DepthFunction));

					if (OldState->State.DepthWriteMask != NewState->State.DepthWriteMask)
						glDepthMask(NewState->State.DepthWriteMask == DepthWrite::All ? GL_TRUE : GL_FALSE);

					if (OldState->State.StencilWriteMask != NewState->State.StencilWriteMask)
						glStencilMask((GLuint)NewState->State.StencilWriteMask);

					if (OldState->State.FrontFaceStencilFunction != NewState->State.FrontFaceStencilFunction)
						glStencilFuncSeparate(GL_FRONT, GetComparison(NewState->State.FrontFaceStencilFunction), 0, 1);

					if (OldState->State.FrontFaceStencilFailOperation != NewState->State.FrontFaceStencilFailOperation || OldState->State.FrontFaceStencilDepthFailOperation != NewState->State.FrontFaceStencilDepthFailOperation || OldState->State.FrontFaceStencilPassOperation != NewState->State.FrontFaceStencilPassOperation)
						glStencilOpSeparate(GL_FRONT, GetStencilOperation(NewState->State.FrontFaceStencilFailOperation), GetStencilOperation(NewState->State.FrontFaceStencilDepthFailOperation), GetStencilOperation(NewState->State.FrontFaceStencilPassOperation));

					if (OldState->State.BackFaceStencilFunction != NewState->State.BackFaceStencilFunction)
						glStencilFuncSeparate(GL_BACK, GetComparison(NewState->State.BackFaceStencilFunction), 0, 1);

					if (OldState->State.BackFaceStencilFailOperation != NewState->State.BackFaceStencilFailOperation || OldState->State.BackFaceStencilDepthFailOperation != NewState->State.BackFaceStencilDepthFailOperation || OldState->State.BackFaceStencilPassOperation != NewState->State.BackFaceStencilPassOperation)
						glStencilOpSeparate(GL_BACK, GetStencilOperation(NewState->State.BackFaceStencilFailOperation), GetStencilOperation(NewState->State.BackFaceStencilDepthFailOperation), GetStencilOperation(NewState->State.BackFaceStencilPassOperation));
				}
				else
				{
					glDepthFunc(GetComparison(NewState->State.DepthFunction));
					glDepthMask(NewState->State.DepthWriteMask == DepthWrite::All ? GL_TRUE : GL_FALSE);
					glStencilMask((GLuint)NewState->State.StencilWriteMask);
					glStencilFuncSeparate(GL_FRONT, GetComparison(NewState->State.FrontFaceStencilFunction), 0, 1);
					glStencilOpSeparate(GL_FRONT, GetStencilOperation(NewState->State.FrontFaceStencilFailOperation), GetStencilOperation(NewState->State.FrontFaceStencilDepthFailOperation), GetStencilOperation(NewState->State.FrontFaceStencilPassOperation));
					glStencilFuncSeparate(GL_BACK, GetComparison(NewState->State.BackFaceStencilFunction), 0, 1);
					glStencilOpSeparate(GL_BACK, GetStencilOperation(NewState->State.BackFaceStencilFailOperation), GetStencilOperation(NewState->State.BackFaceStencilDepthFailOperation), GetStencilOperation(NewState->State.BackFaceStencilPassOperation));
				}
			}
			void OGLDevice::SetInputLayout(InputLayout* State)
			{
				REG_EXCHANGE(Layout, (OGLInputLayout*)State);
				SetVertexBuffers(nullptr, 0);
			}
			ExpectsGraphics<void> OGLDevice::SetShader(Shader* Resource, uint32_t Type)
			{
				OGLShader* IResource = (OGLShader*)Resource;
				bool Update = false;

				if (Type & (uint32_t)ShaderType::Vertex)
				{
					auto& Item = Register.Shaders[0];
					if (Item != IResource)
					{
						if (IResource != nullptr && IResource->VertexShader != GL_NONE)
							Item = IResource;
						else
							Item = nullptr;
						Update = true;
					}
				}

				if (Type & (uint32_t)ShaderType::Pixel)
				{
					auto& Item = Register.Shaders[1];
					if (Item != IResource)
					{
						if (IResource != nullptr && IResource->PixelShader != GL_NONE)
							Item = IResource;
						else
							Item = nullptr;
						Update = true;
					}
				}

				if (Type & (uint32_t)ShaderType::Geometry)
				{
					auto& Item = Register.Shaders[2];
					if (Item != IResource)
					{
						if (IResource != nullptr && IResource->GeometryShader != GL_NONE)
							Item = IResource;
						else
							Item = nullptr;
						Update = true;
					}
				}

				if (Type & (uint32_t)ShaderType::Hull)
				{
					auto& Item = Register.Shaders[3];
					if (Item != IResource)
					{
						if (IResource != nullptr && IResource->HullShader != GL_NONE)
							Item = IResource;
						else
							Item = nullptr;
						Update = true;
					}
				}

				if (Type & (uint32_t)ShaderType::Domain)
				{
					auto& Item = Register.Shaders[4];
					if (Item != IResource)
					{
						if (IResource != nullptr && IResource->DomainShader != GL_NONE)
							Item = IResource;
						else
							Item = nullptr;
						Update = true;
					}
				}

				if (Type & (uint32_t)ShaderType::Compute)
				{
					auto& Item = Register.Shaders[5];
					if (Item != IResource)
					{
						if (IResource != nullptr && IResource->ComputeShader != GL_NONE)
							Item = IResource;
						else
							Item = nullptr;
						Update = true;
					}
				}

				if (!Update)
					return Core::Expectation::Met;

				uint64_t Name = GetProgramHash();
				auto It = Register.Programs.find(Name);
				if (It != Register.Programs.end())
				{
					glUseProgramObjectARB(It->second);
					return Core::Expectation::Met;
				}

				GLuint Program = glCreateProgram();
				if (Register.Shaders[0] != nullptr && Register.Shaders[0]->VertexShader != GL_NONE)
					glAttachShader(Program, Register.Shaders[0]->VertexShader);

				if (Register.Shaders[1] != nullptr && Register.Shaders[1]->PixelShader != GL_NONE)
					glAttachShader(Program, Register.Shaders[1]->PixelShader);

				if (Register.Shaders[2] != nullptr && Register.Shaders[2]->GeometryShader != GL_NONE)
					glAttachShader(Program, Register.Shaders[2]->GeometryShader);

				if (Register.Shaders[3] != nullptr && Register.Shaders[3]->DomainShader != GL_NONE)
					glAttachShader(Program, Register.Shaders[3]->DomainShader);

				if (Register.Shaders[4] != nullptr && Register.Shaders[4]->HullShader != GL_NONE)
					glAttachShader(Program, Register.Shaders[4]->HullShader);

				if (Register.Shaders[5] != nullptr && Register.Shaders[5]->ComputeShader != GL_NONE)
					glAttachShader(Program, Register.Shaders[5]->ComputeShader);

				GLint StatusCode = 0;
				glLinkProgramARB(Program);
				glGetProgramiv(Program, GL_LINK_STATUS, &StatusCode);
				glUseProgramObjectARB(Program);

				if (StatusCode != GL_TRUE)
				{
					GLint BufferSize = 0;
					glGetProgramiv(Program, GL_INFO_LOG_LENGTH, &BufferSize);

					char* Buffer = Core::Memory::Allocate<char>(sizeof(char) * (BufferSize + 1));
					glGetProgramInfoLog(Program, BufferSize, &BufferSize, Buffer);
					Core::String ErrorText(Buffer, (size_t)BufferSize);
					Core::Memory::Deallocate(Buffer);

					glUseProgramObjectARB(GL_NONE);
					glDeleteProgram(Program);
					Program = GL_NONE;
					Register.Programs[Name] = Program;
					return GraphicsException(std::move(ErrorText));
				}
				else
				{
					for (auto* Base : Register.Shaders)
					{
						if (Base != nullptr)
							Base->Programs[Program] = this;
					}
					Register.Programs[Name] = Program;
					return Core::Expectation::Met;
				}
			}
			void OGLDevice::SetSamplerState(SamplerState* State, uint32_t Slot, uint32_t Count, uint32_t Type)
			{
				VI_ASSERT(Slot < UNITS_SIZE, "slot should be less than %i", (int)UNITS_SIZE);
				VI_ASSERT(Count <= UNITS_SIZE && Slot + Count <= UNITS_SIZE, "count should be less than or equal %i", (int)UNITS_SIZE);

				OGLSamplerState* IResource = (OGLSamplerState*)State;
				GLuint NewState = (GLuint)(IResource ? IResource->Resource : GL_NONE);
				uint32_t Offset = Slot + Count;

				for (uint32_t i = Slot; i < Offset; i++)
				{
					auto& Item = Register.Samplers[i];
					if (Item != NewState)
					{
						glBindSampler(i, NewState);
						Item = NewState;
					}
				}
			}
			void OGLDevice::SetBuffer(Shader* Resource, uint32_t Slot, uint32_t Type)
			{
				VI_ASSERT(Slot < UNITS_SIZE, "slot should be less than %i", (int)UNITS_SIZE);

				OGLShader* IResource = (OGLShader*)Resource;
				glBindBufferBase(GL_UNIFORM_BUFFER, Slot, IResource ? IResource->ConstantBuffer : GL_NONE);
			}
			void OGLDevice::SetBuffer(InstanceBuffer* Resource, uint32_t Slot, uint32_t Type)
			{
				OGLInstanceBuffer* IResource = (OGLInstanceBuffer*)Resource;
				SetStructureBuffer(IResource ? IResource->Elements : nullptr, Slot, Type);
			}
			void OGLDevice::SetConstantBuffer(ElementBuffer* Resource, uint32_t Slot, uint32_t Type)
			{
				VI_ASSERT(Slot < UNITS_SIZE, "slot should be less than %i", (int)UNITS_SIZE);
				glBindBufferBase(GL_UNIFORM_BUFFER, Slot, Resource ? ((OGLElementBuffer*)Resource)->Resource : GL_NONE);
			}
			void OGLDevice::SetStructureBuffer(ElementBuffer* Resource, uint32_t Slot, uint32_t Type)
			{
				VI_ASSERT(Slot < UNITS_SIZE, "slot should be less than %i", (int)UNITS_SIZE);
				glBindBufferBase(GL_SHADER_STORAGE_BUFFER, Slot, Resource ? ((OGLElementBuffer*)Resource)->Resource : GL_NONE);
			}
			void OGLDevice::SetIndexBuffer(ElementBuffer* Resource, Format FormatMode)
			{
				OGLElementBuffer* IResource = (OGLElementBuffer*)Resource;
				REG_EXCHANGE_T2(IndexBuffer, IResource, FormatMode);

				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IResource ? IResource->Resource : GL_NONE);
				if (FormatMode == Format::R32_Uint)
					Register.IndexFormat = GL_UNSIGNED_INT;
				else if (FormatMode == Format::R16_Uint)
					Register.IndexFormat = GL_UNSIGNED_SHORT;
				else if (FormatMode == Format::R8_Uint)
					Register.IndexFormat = GL_UNSIGNED_BYTE;
				else
					Register.IndexFormat = GL_UNSIGNED_INT;
			}
			void OGLDevice::SetVertexBuffers(ElementBuffer** Resources, uint32_t Count, bool DynamicLinkage)
			{
				VI_ASSERT(Resources != nullptr || !Count, "invalid vertex buffer array pointer");
				VI_ASSERT(Count <= UNITS_SIZE, "slot should be less than or equal to %i", (int)UNITS_SIZE);

				static OGLElementBuffer* IResources[UNITS_SIZE] = { nullptr };
				bool HasBuffers = false;

				for (uint32_t i = 0; i < Count; i++)
				{
					IResources[i] = (OGLElementBuffer*)Resources[i];
					Register.VertexBuffers[i] = IResources[i];
					if (IResources[i] != nullptr)
						HasBuffers = true;
				}

				SetIndexBuffer(nullptr, Format::Unknown);
				if (!Count || !HasBuffers || !Register.Layout)
					return (void)glBindVertexArray(0);

				GLuint Buffer = GL_NONE;
				if (!DynamicLinkage)
				{
					Core::String Hash = OGLInputLayout::GetLayoutHash(IResources, Count);
					auto It = Register.Layout->Layouts.find(Hash);
					if (It == Register.Layout->Layouts.end())
					{
						glGenVertexArrays(1, &Buffer);
						glBindVertexArray(Buffer);
						Register.Layout->Layouts[Hash] = Buffer;
						DynamicLinkage = true;
					}
					else
						Buffer = It->second;
				}
				else
				{
					if (Register.Layout->DynamicResource == GL_NONE)
					{
						glGenVertexArrays(1, &Register.Layout->DynamicResource);
						DynamicLinkage = true;
					}
					Buffer = Register.Layout->DynamicResource;
				}

				glBindVertexArray(Buffer);
				if (!DynamicLinkage)
					return;

				VI_ASSERT(Count <= Register.Layout->VertexLayout.size(), "too many vertex buffers are being bound: %" PRIu64 " out of %" PRIu64, (uint64_t)Count, (uint64_t)Register.Layout->VertexLayout.size());
				for (uint32_t i = 0; i < Count; i++)
				{
					OGLElementBuffer* IResource = IResources[i];
					if (!IResource)
						continue;

					IResource->Bindings[Buffer] = Register.Layout;
					glBindBuffer(GL_ARRAY_BUFFER, IResource->Resource);
					for (auto& Attribute : Register.Layout->VertexLayout[i])
						Attribute(IResource->Stride);
				}
			}
			void OGLDevice::SetTexture2D(Texture2D* Resource, uint32_t Slot, uint32_t Type)
			{
				VI_ASSERT(!Resource || !((OGLTexture2D*)Resource)->Backbuffer, "resource 2d should not be back buffer texture");
				VI_ASSERT(Slot < UNITS_SIZE, "slot should be less than %i", (int)UNITS_SIZE);

				OGLTexture2D* IResource = (OGLTexture2D*)Resource;
				GLuint NewResource = (IResource ? IResource->Resource : GL_NONE);
				if (Register.Textures[Slot] == NewResource)
					return;

				Register.Bindings[Slot] = GL_TEXTURE_2D;
				Register.Textures[Slot] = NewResource;
				glActiveTexture(GL_TEXTURE0 + Slot);
				glBindTexture(GL_TEXTURE_2D, NewResource);
			}
			void OGLDevice::SetTexture3D(Texture3D* Resource, uint32_t Slot, uint32_t Type)
			{
				VI_ASSERT(Slot < UNITS_SIZE, "slot should be less than %i", (int)UNITS_SIZE);

				OGLTexture3D* IResource = (OGLTexture3D*)Resource;
				GLuint NewResource = (IResource ? IResource->Resource : GL_NONE);
				if (Register.Textures[Slot] == NewResource)
					return;

				Register.Bindings[Slot] = GL_TEXTURE_3D;
				Register.Textures[Slot] = NewResource;
				glActiveTexture(GL_TEXTURE0 + Slot);
				glBindTexture(GL_TEXTURE_3D, NewResource);
			}
			void OGLDevice::SetTextureCube(TextureCube* Resource, uint32_t Slot, uint32_t Type)
			{
				VI_ASSERT(Slot < UNITS_SIZE, "slot should be less than %i", (int)UNITS_SIZE);

				OGLTextureCube* IResource = (OGLTextureCube*)Resource;
				GLuint NewResource = (IResource ? IResource->Resource : GL_NONE);
				if (Register.Textures[Slot] == NewResource)
					return;

				Register.Bindings[Slot] = GL_TEXTURE_CUBE_MAP;
				Register.Textures[Slot] = NewResource;
				glActiveTexture(GL_TEXTURE0 + Slot);
				glBindTexture(GL_TEXTURE_CUBE_MAP, NewResource);
			}
			void OGLDevice::SetWriteable(ElementBuffer** Resource, uint32_t Slot, uint32_t Count, bool Computable)
			{
				VI_ASSERT(Slot < UNITS_SIZE, "slot should be less than %i", (int)UNITS_SIZE);
				VI_ASSERT(Count <= UNITS_SIZE && Slot + Count <= UNITS_SIZE, "count should be less than or equal %i", (int)UNITS_SIZE);
				VI_ASSERT(Resource != nullptr, "resource should be set");
			}
			void OGLDevice::SetWriteable(Texture2D** Resource, uint32_t Slot, uint32_t Count, bool Computable)
			{
				VI_ASSERT(Slot < UNITS_SIZE, "slot should be less than %i", (int)UNITS_SIZE);
				VI_ASSERT(Count <= UNITS_SIZE && Slot + Count <= UNITS_SIZE, "count should be less than or equal %i", (int)UNITS_SIZE);
				VI_ASSERT(Resource != nullptr, "resource should be set");

				for (uint32_t i = 0; i < Count; i++)
				{
					OGLTexture2D* IResource = (OGLTexture2D*)Resource[i];
					VI_ASSERT(!IResource || !IResource->Backbuffer, "resource 2d #%i should not be back buffer texture", (int)i);

					glActiveTexture(GL_TEXTURE0 + Slot + i);
					if (!IResource)
						glBindTexture(GL_TEXTURE_2D, GL_NONE);
					else
						glBindImageTexture(Slot + i, IResource->Resource, 0, GL_TRUE, 0, GL_READ_WRITE, IResource->Format);
				}
			}
			void OGLDevice::SetWriteable(Texture3D** Resource, uint32_t Slot, uint32_t Count, bool Computable)
			{
				VI_ASSERT(Slot < UNITS_SIZE, "slot should be less than %i", (int)UNITS_SIZE);
				VI_ASSERT(Count <= UNITS_SIZE && Slot + Count <= UNITS_SIZE, "count should be less than or equal %i", (int)UNITS_SIZE);
				VI_ASSERT(Resource != nullptr, "resource should be set");

				for (uint32_t i = 0; i < Count; i++)
				{
					OGLTexture3D* IResource = (OGLTexture3D*)Resource[i];
					glActiveTexture(GL_TEXTURE0 + Slot + i);

					if (!IResource)
						glBindTexture(GL_TEXTURE_3D, GL_NONE);
					else
						glBindImageTexture(Slot + i, IResource->Resource, 0, GL_TRUE, 0, GL_READ_WRITE, IResource->Format);
				}
			}
			void OGLDevice::SetWriteable(TextureCube** Resource, uint32_t Slot, uint32_t Count, bool Computable)
			{
				VI_ASSERT(Slot < UNITS_SIZE, "slot should be less than %i", (int)UNITS_SIZE);
				VI_ASSERT(Count <= UNITS_SIZE && Slot + Count <= UNITS_SIZE, "count should be less than or equal %i", (int)UNITS_SIZE);
				VI_ASSERT(Resource != nullptr, "resource should be set");

				for (uint32_t i = 0; i < Count; i++)
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
			void OGLDevice::SetTarget(DepthTarget2D* Resource)
			{
				VI_ASSERT(Resource != nullptr, "resource should be set");
				OGLDepthTarget2D* IResource = (OGLDepthTarget2D*)Resource;
				GLenum Target = GL_NONE;
				glBindFramebuffer(GL_FRAMEBUFFER, IResource->FrameBuffer);
				glDrawBuffers(1, &Target);
				glViewport((GLuint)IResource->Viewarea.TopLeftX, (GLuint)IResource->Viewarea.TopLeftY, (GLuint)IResource->Viewarea.Width, (GLuint)IResource->Viewarea.Height);
			}
			void OGLDevice::SetTarget(DepthTargetCube* Resource)
			{
				VI_ASSERT(Resource != nullptr, "resource should be set");
				OGLDepthTargetCube* IResource = (OGLDepthTargetCube*)Resource;
				GLenum Target = GL_NONE;
				glBindFramebuffer(GL_FRAMEBUFFER, IResource->FrameBuffer);
				glDrawBuffers(1, &Target);
				glViewport((GLuint)IResource->Viewarea.TopLeftX, (GLuint)IResource->Viewarea.TopLeftY, (GLuint)IResource->Viewarea.Width, (GLuint)IResource->Viewarea.Height);
			}
			void OGLDevice::SetTarget(Graphics::RenderTarget* Resource, uint32_t Target, float R, float G, float B)
			{
				VI_ASSERT(Resource != nullptr, "resource should be set");
				VI_ASSERT(Target < Resource->GetTargetCount(), "targets count should be less than %i", (int)Resource->GetTargetCount());

				OGLFrameBuffer* TargetBuffer = (OGLFrameBuffer*)Resource->GetTargetBuffer();
				const Viewport& Viewarea = Resource->GetViewport();

				VI_ASSERT(TargetBuffer != nullptr, "target buffer should be set");
				if (!TargetBuffer->Backbuffer)
				{
					GLenum Targets[8] = { GL_NONE };
					Targets[Target] = TargetBuffer->Format[Target];

					glBindFramebuffer(GL_FRAMEBUFFER, TargetBuffer->Buffer);
					glDrawBuffers(Resource->GetTargetCount(), Targets);

				}
				else
				{
					glBindFramebuffer(GL_FRAMEBUFFER, GL_NONE);
					glDrawBuffer(GL_FRONT_AND_BACK);
				}

				glViewport((GLuint)Viewarea.TopLeftX, (GLuint)Viewarea.TopLeftY, (GLuint)Viewarea.Width, (GLuint)Viewarea.Height);
				glClearColor(R, G, B, 1.0f);
				glClear(GL_COLOR_BUFFER_BIT);
			}
			void OGLDevice::SetTarget(Graphics::RenderTarget* Resource, uint32_t Target)
			{
				VI_ASSERT(Resource != nullptr, "resource should be set");
				VI_ASSERT(Target < Resource->GetTargetCount(), "targets count should be less than %i", (int)Resource->GetTargetCount());

				OGLFrameBuffer* TargetBuffer = (OGLFrameBuffer*)Resource->GetTargetBuffer();
				const Viewport& Viewarea = Resource->GetViewport();

				VI_ASSERT(TargetBuffer != nullptr, "target buffer should be set");
				if (!TargetBuffer->Backbuffer)
				{
					GLenum Targets[8] = { GL_NONE };
					Targets[Target] = TargetBuffer->Format[Target];

					glBindFramebuffer(GL_FRAMEBUFFER, TargetBuffer->Buffer);
					glDrawBuffers(Resource->GetTargetCount(), Targets);

				}
				else
				{
					glBindFramebuffer(GL_FRAMEBUFFER, GL_NONE);
					glDrawBuffer(GL_FRONT_AND_BACK);
				}

				glViewport((GLuint)Viewarea.TopLeftX, (GLuint)Viewarea.TopLeftY, (GLuint)Viewarea.Width, (GLuint)Viewarea.Height);
			}
			void OGLDevice::SetTarget(Graphics::RenderTarget* Resource, float R, float G, float B)
			{
				VI_ASSERT(Resource != nullptr, "resource should be set");
				OGLFrameBuffer* TargetBuffer = (OGLFrameBuffer*)Resource->GetTargetBuffer();
				const Viewport& Viewarea = Resource->GetViewport();

				VI_ASSERT(TargetBuffer != nullptr, "target buffer should be set");
				if (!TargetBuffer->Backbuffer)
				{
					glBindFramebuffer(GL_FRAMEBUFFER, TargetBuffer->Buffer);
					glDrawBuffers(Resource->GetTargetCount(), TargetBuffer->Format);
				}
				else
				{
					glBindFramebuffer(GL_FRAMEBUFFER, GL_NONE);
					glDrawBuffer(GL_FRONT_AND_BACK);
				}

				glViewport((GLuint)Viewarea.TopLeftX, (GLuint)Viewarea.TopLeftY, (GLuint)Viewarea.Width, (GLuint)Viewarea.Height);
				glClearColor(R, G, B, 1.0f);
				glClear(GL_COLOR_BUFFER_BIT);
			}
			void OGLDevice::SetTarget(Graphics::RenderTarget* Resource)
			{
				VI_ASSERT(Resource != nullptr, "resource should be set");
				OGLFrameBuffer* TargetBuffer = (OGLFrameBuffer*)Resource->GetTargetBuffer();
				const Viewport& Viewarea = Resource->GetViewport();

				VI_ASSERT(TargetBuffer != nullptr, "target buffer should be set");
				if (!TargetBuffer->Backbuffer)
				{
					glBindFramebuffer(GL_FRAMEBUFFER, TargetBuffer->Buffer);
					glDrawBuffers(Resource->GetTargetCount(), TargetBuffer->Format);

				}
				else
				{
					glBindFramebuffer(GL_FRAMEBUFFER, GL_NONE);
					glDrawBuffer(GL_FRONT_AND_BACK);
				}

				glViewport((GLuint)Viewarea.TopLeftX, (GLuint)Viewarea.TopLeftY, (GLuint)Viewarea.Width, (GLuint)Viewarea.Height);
			}
			void OGLDevice::SetTargetMap(Graphics::RenderTarget* Resource, bool Enabled[8])
			{
				VI_ASSERT(Resource != nullptr, "resource should be set");
				VI_ASSERT(Resource->GetTargetCount() > 1, "render target should have more than one targets");

				OGLFrameBuffer* TargetBuffer = (OGLFrameBuffer*)Resource->GetTargetBuffer();
				const Viewport& Viewarea = Resource->GetViewport();

				VI_ASSERT(TargetBuffer != nullptr, "target buffer should be set");
				if (!TargetBuffer->Backbuffer)
				{
					GLenum Targets[8] = { GL_NONE };
					GLuint Count = Resource->GetTargetCount();

					for (uint32_t i = 0; i < Count; i++)
						Targets[i] = (Enabled[i] ? TargetBuffer->Format[i] : GL_NONE);

					glBindFramebuffer(GL_FRAMEBUFFER, TargetBuffer->Buffer);
					glDrawBuffers(Count, Targets);

				}
				else
				{
					glBindFramebuffer(GL_FRAMEBUFFER, GL_NONE);
					glDrawBuffer(GL_FRONT_AND_BACK);
				}

				glViewport((GLuint)Viewarea.TopLeftX, (GLuint)Viewarea.TopLeftY, (GLuint)Viewarea.Width, (GLuint)Viewarea.Height);
			}
			void OGLDevice::SetTargetRect(uint32_t Width, uint32_t Height)
			{
				VI_ASSERT(Width > 0 && Height > 0, "width and height should be greater than zero");
				glBindFramebuffer(GL_FRAMEBUFFER, GL_NONE);
				glViewport(0, 0, Width, Height);
			}
			void OGLDevice::SetViewports(uint32_t Count, Viewport* Value)
			{
				VI_ASSERT(Count > 0 && Value != nullptr, "at least one viewport should be set");
				glViewport((GLuint)Value->TopLeftX, (GLuint)Value->TopLeftY, (GLuint)Value->Width, (GLuint)Value->Height);
			}
			void OGLDevice::SetScissorRects(uint32_t Count, Compute::Rectangle* Value)
			{
				VI_ASSERT(Count > 0 && Value != nullptr, "at least one scissor rect should be set");
				int64_t Height = Value->GetHeight();
				glScissor((GLuint)Value->GetX(), (GLuint)OGL_GetCoordY(Value->GetY(), Height, (int64_t)Window->GetHeight()), (GLuint)Value->GetWidth(), (GLuint)Height);
			}
			void OGLDevice::SetPrimitiveTopology(PrimitiveTopology _Topology)
			{
				REG_EXCHANGE(Primitive, _Topology);
				Register.DrawTopology = GetPrimitiveTopologyDraw(_Topology);
				Register.Primitive = _Topology;
			}
			void OGLDevice::FlushTexture(uint32_t Slot, uint32_t Count, uint32_t Type)
			{
				VI_ASSERT(Slot < UNITS_SIZE, "slot should be less than %i", (int)UNITS_SIZE);
				VI_ASSERT(Count <= UNITS_SIZE && Slot + Count <= UNITS_SIZE, "count should be less than or equal %i", (int)UNITS_SIZE);

				for (uint32_t i = 0; i < Count; i++)
				{
					auto& Texture = Register.Textures[i];
					if (Texture == GL_NONE)
						continue;

					glActiveTexture(GL_TEXTURE0 + Slot + i);
					glBindTexture(Register.Bindings[i], GL_NONE);
					Texture = GL_NONE;
				}
			}
			void OGLDevice::FlushState()
			{
			}
			void OGLDevice::ClearBuffer(InstanceBuffer* Resource)
			{
				VI_ASSERT(Resource != nullptr, "resource should be set");
				OGLInstanceBuffer* IResource = (OGLInstanceBuffer*)Resource;
				if (!IResource->Sync)
					return;

				OGLElementBuffer* Element = (OGLElementBuffer*)IResource->Elements;
				glBindBuffer(Element->Flags, Element->Resource);
				GLvoid* Data = glMapBuffer(Element->Flags, GL_WRITE_ONLY);
				memset(Data, 0, IResource->ElementWidth * IResource->ElementLimit);
				glUnmapBuffer(Element->Flags);
				glBindBuffer(Element->Flags, GL_NONE);
			}
			void OGLDevice::ClearWritable(Texture2D* Resource)
			{
				ClearWritable(Resource, 0.0f, 0.0f, 0.0f);
			}
			void OGLDevice::ClearWritable(Texture2D* Resource, float R, float G, float B)
			{
				VI_ASSERT(Resource != nullptr, "resource should be set");
				VI_ASSERT(!((OGLTexture2D*)Resource)->Backbuffer, "resource 2d should not be back buffer texture");

				OGLTexture2D* IResource = (OGLTexture2D*)Resource;
				GLfloat ClearColor[4] = { R, G, B, 0.0f };
				glClearTexImage(IResource->Resource, 0, GL_RGBA, GL_FLOAT, &ClearColor);
			}
			void OGLDevice::ClearWritable(Texture3D* Resource)
			{
				ClearWritable(Resource, 0.0f, 0.0f, 0.0f);
			}
			void OGLDevice::ClearWritable(Texture3D* Resource, float R, float G, float B)
			{
				VI_ASSERT(Resource != nullptr, "resource should be set");
				OGLTexture3D* IResource = (OGLTexture3D*)Resource;
				GLfloat ClearColor[4] = { R, G, B, 0.0f };
				glClearTexImage(IResource->Resource, 0, GL_RGBA, GL_FLOAT, &ClearColor);
			}
			void OGLDevice::ClearWritable(TextureCube* Resource)
			{
				ClearWritable(Resource, 0.0f, 0.0f, 0.0f);
			}
			void OGLDevice::ClearWritable(TextureCube* Resource, float R, float G, float B)
			{
				VI_ASSERT(Resource != nullptr, "resource should be set");
				OGLTextureCube* IResource = (OGLTextureCube*)Resource;
				GLfloat ClearColor[4] = { R, G, B, 0.0f };
				glClearTexImage(IResource->Resource, 0, GL_RGBA, GL_FLOAT, &ClearColor);
			}
			void OGLDevice::Clear(float R, float G, float B)
			{
				glClearColor(R, G, B, 0.0f);
				glClear(GL_COLOR_BUFFER_BIT);
			}
			void OGLDevice::Clear(Graphics::RenderTarget* Resource, uint32_t Target, float R, float G, float B)
			{
				VI_ASSERT(Resource != nullptr, "resource should be set");
				OGLFrameBuffer* TargetBuffer = (OGLFrameBuffer*)Resource->GetTargetBuffer();
				if (TargetBuffer->Backbuffer)
					return Clear(R, G, B);

				float ClearColor[4] = { R, G, B, 0.0f };
				glClearBufferfv(GL_COLOR, Target, ClearColor);
			}
			void OGLDevice::ClearDepth()
			{
				ClearDepth(RenderTarget);
			}
			void OGLDevice::ClearDepth(DepthTarget2D* Resource)
			{
				VI_ASSERT(Resource != nullptr, "resource should be set");
				OGLDepthTarget2D* IResource = (OGLDepthTarget2D*)Resource;
				if (IResource->HasStencilBuffer)
					glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
				else
					glClear(GL_DEPTH_BUFFER_BIT);
			}
			void OGLDevice::ClearDepth(DepthTargetCube* Resource)
			{
				VI_ASSERT(Resource != nullptr, "resource should be set");
				OGLDepthTargetCube* IResource = (OGLDepthTargetCube*)Resource;
				if (IResource->HasStencilBuffer)
					glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
				else
					glClear(GL_DEPTH_BUFFER_BIT);
			}
			void OGLDevice::ClearDepth(Graphics::RenderTarget* Resource)
			{
				VI_ASSERT(Resource != nullptr, "resource should be set");
				glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
			}
			void OGLDevice::DrawIndexed(uint32_t Count, uint32_t IndexLocation, uint32_t BaseLocation)
			{
				glDrawElements(Register.DrawTopology, Count, Register.IndexFormat, OGL_OFFSET((size_t)IndexLocation));
			}
			void OGLDevice::DrawIndexed(MeshBuffer* Resource)
			{
				VI_ASSERT(Resource != nullptr, "resource should be set");
				ElementBuffer* VertexBuffer = Resource->GetVertexBuffer();
				ElementBuffer* IndexBuffer = Resource->GetIndexBuffer();
				SetVertexBuffers(&VertexBuffer, 1);
				SetIndexBuffer(IndexBuffer, Format::R32_Uint);

				glDrawElements(Register.DrawTopology, (GLsizei)IndexBuffer->GetElements(), GL_UNSIGNED_INT, nullptr);
			}
			void OGLDevice::DrawIndexed(SkinMeshBuffer* Resource)
			{
				VI_ASSERT(Resource != nullptr, "resource should be set");
				ElementBuffer* VertexBuffer = Resource->GetVertexBuffer();
				ElementBuffer* IndexBuffer = Resource->GetIndexBuffer();
				SetVertexBuffers(&VertexBuffer, 1);
				SetIndexBuffer(IndexBuffer, Format::R32_Uint);

				glDrawElements(Register.DrawTopology, (GLsizei)IndexBuffer->GetElements(), GL_UNSIGNED_INT, nullptr);
			}
			void OGLDevice::DrawIndexedInstanced(uint32_t IndexCountPerInstance, uint32_t InstanceCount, uint32_t IndexLocation, uint32_t VertexLocation, uint32_t InstanceLocation)
			{
				glDrawElementsInstanced(Register.DrawTopology, IndexCountPerInstance, GL_UNSIGNED_INT, nullptr, InstanceCount);
			}
			void OGLDevice::DrawIndexedInstanced(ElementBuffer* Instances, MeshBuffer* Resource, uint32_t InstanceCount)
			{
				VI_ASSERT(Instances != nullptr, "instances should be set");
				VI_ASSERT(Resource != nullptr, "resource should be set");

				ElementBuffer* VertexBuffers[2] = { Resource->GetVertexBuffer(), Instances };
				ElementBuffer* IndexBuffer = Resource->GetIndexBuffer();
				SetVertexBuffers(VertexBuffers, 2, true);
				SetIndexBuffer(IndexBuffer, Format::R32_Uint);

				glDrawElementsInstanced(Register.DrawTopology, (GLsizei)IndexBuffer->GetElements(), GL_UNSIGNED_INT, nullptr, (GLsizei)InstanceCount);
			}
			void OGLDevice::DrawIndexedInstanced(ElementBuffer* Instances, SkinMeshBuffer* Resource, uint32_t InstanceCount)
			{
				VI_ASSERT(Instances != nullptr, "instances should be set");
				VI_ASSERT(Resource != nullptr, "resource should be set");

				ElementBuffer* VertexBuffers[2] = { Resource->GetVertexBuffer(), Instances };
				ElementBuffer* IndexBuffer = Resource->GetIndexBuffer();
				SetVertexBuffers(VertexBuffers, 2, true);
				SetIndexBuffer(IndexBuffer, Format::R32_Uint);

				glDrawElementsInstanced(Register.DrawTopology, (GLsizei)IndexBuffer->GetElements(), GL_UNSIGNED_INT, nullptr, (GLsizei)InstanceCount);
			}
			void OGLDevice::Draw(uint32_t Count, uint32_t Location)
			{
				glDrawArrays(Register.DrawTopology, (GLint)Location, (GLint)Count);
			}
			void OGLDevice::DrawInstanced(uint32_t VertexCountPerInstance, uint32_t InstanceCount, uint32_t VertexLocation, uint32_t InstanceLocation)
			{
				glDrawArraysInstanced(Register.DrawTopology, (GLint)VertexLocation, (GLint)VertexCountPerInstance, (GLint)InstanceCount);
			}
			void OGLDevice::Dispatch(uint32_t GroupX, uint32_t GroupY, uint32_t GroupZ)
			{
				glDispatchCompute(GroupX, GroupY, GroupZ);
			}
			void OGLDevice::GetViewports(uint32_t* Count, Viewport* Out)
			{
				GLint Viewport[4];
				glGetIntegerv(GL_VIEWPORT, Viewport);
				if (Count != nullptr)
					*Count = 1;

				if (!Out)
					return;

				Out->TopLeftX = (float)Viewport[0];
				Out->TopLeftY = (float)D3D_GetCoordY((int64_t)Viewport[1], (int64_t)Viewport[3], (int64_t)Window->GetHeight());
				Out->Width = (float)Viewport[2];
				Out->Height = (float)Viewport[3];
			}
			void OGLDevice::GetScissorRects(uint32_t* Count, Compute::Rectangle* Out)
			{
				GLint Rect[4];
				glGetIntegerv(GL_SCISSOR_BOX, Rect);
				if (Count != nullptr)
					*Count = 1;

				if (!Out)
					return;

				int64_t Height = (int64_t)Rect[1] - Rect[3];
				Out->Left = Rect[0];
				Out->Right = (int64_t)Rect[2] + Rect[0];
				Out->Top = D3D_GetCoordY(Rect[1], Height, (int64_t)Window->GetHeight());
				Out->Bottom = Height;
			}
			void OGLDevice::QueryBegin(Query* Resource)
			{
				VI_ASSERT(Resource != nullptr, "resource should be set");
				OGLQuery* IResource = (OGLQuery*)Resource;
				glBeginQuery(IResource->Predicate ? GL_ANY_SAMPLES_PASSED : GL_SAMPLES_PASSED, IResource->Async);
			}
			void OGLDevice::QueryEnd(Query* Resource)
			{
				VI_ASSERT(Resource != nullptr, "resource should be set");
				OGLQuery* IResource = (OGLQuery*)Resource;
				glEndQuery(IResource->Predicate ? GL_ANY_SAMPLES_PASSED : GL_SAMPLES_PASSED);
			}
			void OGLDevice::GenerateMips(Texture2D* Resource)
			{
				VI_ASSERT(Resource != nullptr, "resource should be set");
				OGLTexture2D* IResource = (OGLTexture2D*)Resource;

				VI_ASSERT(!IResource->Backbuffer, "resource 2d should not be back buffer texture");
				VI_ASSERT(IResource->Resource != GL_NONE, "resource should be valid");
#ifdef glGenerateTextureMipmap
				glGenerateTextureMipmap(IResource->Resource);
#elif glGenerateMipmap
				glBindTexture(GL_TEXTURE_2D, IResource->Resource);
				glGenerateMipmap(GL_TEXTURE_2D);
				glBindTexture(GL_TEXTURE_2D, GL_NONE);
#endif
			}
			void OGLDevice::GenerateMips(Texture3D* Resource)
			{
				VI_ASSERT(Resource != nullptr, "resource should be set");
				OGLTexture3D* IResource = (OGLTexture3D*)Resource;

				VI_ASSERT(IResource->Resource != GL_NONE, "resource should be valid");
#ifdef glGenerateTextureMipmap
				glGenerateTextureMipmap(IResource->Resource);
#endif
			}
			void OGLDevice::GenerateMips(TextureCube* Resource)
			{
				VI_ASSERT(Resource != nullptr, "resource should be set");
				OGLTextureCube* IResource = (OGLTextureCube*)Resource;

				VI_ASSERT(IResource->Resource != GL_NONE, "resource should be valid");
#ifdef glGenerateTextureMipmap
				glGenerateTextureMipmap(IResource->Resource);
#elif glGenerateMipmap
				glBindTexture(GL_TEXTURE_CUBE_MAP, IResource->Resource);
				glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
				glBindTexture(GL_TEXTURE_CUBE_MAP, GL_NONE);
#endif
			}
			bool OGLDevice::ImBegin()
			{
				if (Immediate.VertexBuffer == GL_NONE && !CreateDirectBuffer(0))
					return false;

				Primitives = PrimitiveTopology::Triangle_List;
				Direct.Transform = Compute::Matrix4x4::Identity();
				Direct.Padding = { 0, 0, 0, 1 };
				ViewResource = nullptr;

				Elements.clear();
				return true;
			}
			void OGLDevice::ImTransform(const Compute::Matrix4x4& Transform)
			{
				Direct.Transform = Direct.Transform * Transform;
			}
			void OGLDevice::ImTopology(PrimitiveTopology Topology)
			{
				Primitives = Topology;
			}
			void OGLDevice::ImEmit()
			{
				Elements.insert(Elements.begin(), { 0, 0, 0, 0, 0, 1, 1, 1, 1 });
			}
			void OGLDevice::ImTexture(Texture2D* In)
			{
				ViewResource = In;
				Direct.Padding.Z = (In != nullptr);
			}
			void OGLDevice::ImColor(float X, float Y, float Z, float W)
			{
				VI_ASSERT(!Elements.empty(), "vertex should already be emitted");
				auto& Element = Elements.front();
				Element.CX = X;
				Element.CY = Y;
				Element.CZ = Z;
				Element.CW = W;
			}
			void OGLDevice::ImIntensity(float Intensity)
			{
				Direct.Padding.W = Intensity;
			}
			void OGLDevice::ImTexCoord(float X, float Y)
			{
				VI_ASSERT(!Elements.empty(), "vertex should already be emitted");
				auto& Element = Elements.front();
				Element.TX = X;
				Element.TY = 1.0f - Y;
			}
			void OGLDevice::ImTexCoordOffset(float X, float Y)
			{
				Direct.Padding.X = X;
				Direct.Padding.Y = Y;
			}
			void OGLDevice::ImPosition(float X, float Y, float Z)
			{
				VI_ASSERT(!Elements.empty(), "vertex should already be emitted");
				auto& Element = Elements.front();
				Element.PX = X;
				Element.PY = -Y;
				Element.PZ = Z;
			}
			bool OGLDevice::ImEnd()
			{
				if (Immediate.VertexBuffer == GL_NONE || Elements.empty())
					return false;

				OGLTexture2D* IResource = (OGLTexture2D*)ViewResource;
				if (Elements.size() > MaxElements && !CreateDirectBuffer(Elements.size()))
					return false;

				GLint LastVAO = GL_NONE, LastVBO = GL_NONE, LastSampler = GL_NONE;
				glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &LastVAO);
				glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &LastVBO);

				GLint LastProgram = GL_NONE, LastTexture = GL_NONE;
				glGetIntegerv(GL_CURRENT_PROGRAM, &LastProgram);

				glBindBuffer(GL_ARRAY_BUFFER, Immediate.VertexBuffer);
				GLvoid* Data = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
				memcpy(Data, Elements.data(), (size_t)Elements.size() * sizeof(Vertex));
				glUnmapBuffer(GL_ARRAY_BUFFER);

				const GLuint ImageSlot = 1;
				glBindBuffer(GL_ARRAY_BUFFER, LastVBO);
				glUseProgram(Immediate.Program);
				glUniformMatrix4fv(0, 1, GL_FALSE, (const GLfloat*)&Direct.Transform.Row);
				glUniform4fARB(2, Direct.Padding.X, Direct.Padding.Y, Direct.Padding.Z, Direct.Padding.W);	
				glActiveTexture(GL_TEXTURE0 + ImageSlot);
				glGetIntegerv(GL_SAMPLER_BINDING, &LastSampler);
				glGetIntegerv(GL_TEXTURE_BINDING_2D, &LastTexture);
				glBindTexture(GL_TEXTURE_2D, IResource ? IResource->Resource : GL_NONE);
				glBindSampler(ImageSlot, Immediate.Sampler);
				glBindVertexArray(Immediate.VertexArray);
				glDrawArrays(GetPrimitiveTopologyDraw(Primitives), 0, (GLsizei)Elements.size());
				glBindVertexArray(LastVAO);
				glUseProgram(LastProgram);
				glBindSampler(ImageSlot, LastSampler);
				glBindTexture(GL_TEXTURE_2D, LastTexture);

				return true;
			}
			ExpectsGraphics<void> OGLDevice::Submit()
			{
				VI_ASSERT(Window != nullptr, "window should be set");
				Video::GLEW::PerformSwap(Window);
				DispatchQueue();
				return Core::Expectation::Met;
			}
			ExpectsGraphics<void> OGLDevice::Map(ElementBuffer* Resource, ResourceMap Mode, MappedSubresource* Map)
			{
				VI_ASSERT(Resource != nullptr, "resource should be set");
				OGLElementBuffer* IResource = (OGLElementBuffer*)Resource;
				glBindBuffer(IResource->Flags, IResource->Resource);

				GLint Size;
				glGetBufferParameteriv(IResource->Flags, GL_BUFFER_SIZE, &Size);
				Map->Pointer = glMapBuffer(IResource->Flags, OGLDevice::GetResourceMap(Mode));
				Map->RowPitch = GetRowPitch(1, (uint32_t)Size);
				Map->DepthPitch = GetDepthPitch(Map->RowPitch, 1);
				if (!Map->Pointer)
					return GetException("map element buffer");

				return Core::Expectation::Met;
			}
			ExpectsGraphics<void> OGLDevice::Map(Texture2D* Resource, ResourceMap Mode, MappedSubresource* Map)
			{
				VI_ASSERT(Resource != nullptr, "resource should be set");
				VI_ASSERT(Map != nullptr, "map should be set");

				OGLTexture2D* IResource = (OGLTexture2D*)Resource;
				GLint BaseFormat = OGLDevice::GetBaseFormat(IResource->FormatMode);
				GLuint Width = IResource->GetWidth(), Height = IResource->GetHeight();
				GLuint Size = GetFormatSize(IResource->FormatMode);

				bool Read = Mode == ResourceMap::Read || Mode == ResourceMap::Read_Write;
				bool Write = Mode == ResourceMap::Write || Mode == ResourceMap::Write_Discard || Mode == ResourceMap::Write_No_Overwrite;

				if (Read && !((uint32_t)IResource->AccessFlags & (uint32_t)CPUAccess::Read))
					return GraphicsException("resource cannot be mapped for reading");

				if (Write && !((uint32_t)IResource->AccessFlags & (uint32_t)CPUAccess::Write))
					return GraphicsException("resource cannot be mapped for writing");

				Map->Pointer = Core::Memory::Allocate<char>(Width * Height * Size);
				Map->RowPitch = GetRowPitch(Width);
				Map->DepthPitch = GetDepthPitch(Map->RowPitch, Height);

				if (Map->Pointer != nullptr && Read)
				{
					glBindTexture(GL_TEXTURE_2D, IResource->Resource);
					glGetTexImage(GL_TEXTURE_2D, 0, BaseFormat, GL_UNSIGNED_BYTE, Map->Pointer);
				}

				if (!Map->Pointer)
					return GetException("map texture 2d");

				return Core::Expectation::Met;
			}
			ExpectsGraphics<void> OGLDevice::Map(Texture3D* Resource, ResourceMap Mode, MappedSubresource* Map)
			{
				VI_ASSERT(Resource != nullptr, "resource should be set");
				VI_ASSERT(Map != nullptr, "map should be set");

				OGLTexture3D* IResource = (OGLTexture3D*)Resource;
				GLuint Width = IResource->GetWidth(), Height = IResource->GetHeight(), Depth = IResource->GetDepth();
				GLuint Size = GetFormatSize(IResource->FormatMode);

				bool Read = Mode == ResourceMap::Read || Mode == ResourceMap::Read_Write;
				bool Write = Mode == ResourceMap::Write || Mode == ResourceMap::Write_Discard || Mode == ResourceMap::Write_No_Overwrite;

				if (Read && !((uint32_t)IResource->AccessFlags & (uint32_t)CPUAccess::Read))
					return GraphicsException("resource cannot be mapped for reading");

				if (Write && !((uint32_t)IResource->AccessFlags & (uint32_t)CPUAccess::Write))
					return GraphicsException("resource cannot be mapped for writing");

				Map->Pointer = Core::Memory::Allocate<char>(Width * Height * Depth * Size);
				Map->RowPitch = GetRowPitch(Width);
				Map->DepthPitch = GetDepthPitch(Map->RowPitch, Height * Depth);

				if (Map->Pointer != nullptr && Read)
				{
					glBindTexture(GL_TEXTURE_3D, IResource->Resource);
					glGetTexImage(GL_TEXTURE_3D, 0, IResource->Format, GL_UNSIGNED_BYTE, Map->Pointer);
				}

				if (!Map->Pointer)
					return GetException("map texture 2d");

				return Core::Expectation::Met;
			}
			ExpectsGraphics<void> OGLDevice::Map(TextureCube* Resource, ResourceMap Mode, MappedSubresource* Map)
			{
				VI_ASSERT(Resource != nullptr, "resource should be set");
				VI_ASSERT(Map != nullptr, "map should be set");

				OGLTextureCube* IResource = (OGLTextureCube*)Resource;
				GLint BaseFormat = OGLDevice::GetBaseFormat(IResource->FormatMode);
				GLuint Width = IResource->GetWidth(), Height = IResource->GetHeight(), Depth = 6;
				GLuint Size = GetFormatSize(IResource->FormatMode);

				bool Read = Mode == ResourceMap::Read || Mode == ResourceMap::Read_Write;
				bool Write = Mode == ResourceMap::Write || Mode == ResourceMap::Write_Discard || Mode == ResourceMap::Write_No_Overwrite;

				if (Read && !((uint32_t)IResource->AccessFlags & (uint32_t)CPUAccess::Read))
					return GraphicsException("resource cannot be mapped for reading");

				if (Write && !((uint32_t)IResource->AccessFlags & (uint32_t)CPUAccess::Write))
					return GraphicsException("resource cannot be mapped for writing");

				Map->Pointer = Core::Memory::Allocate<char>(Width * Height * Depth * Size);
				Map->RowPitch = GetRowPitch(Width);
				Map->DepthPitch = GetDepthPitch(Map->RowPitch, Height * Depth);

				if (Map->Pointer != nullptr && Read)
				{
					glBindTexture(GL_TEXTURE_CUBE_MAP, IResource->Resource);
					glGetTexImage(GL_TEXTURE_CUBE_MAP, 0, BaseFormat, GL_UNSIGNED_BYTE, Map->Pointer);
				}

				if (!Map->Pointer)
					return GetException("map texture 2d");

				return Core::Expectation::Met;
			}
			ExpectsGraphics<void> OGLDevice::Unmap(Texture2D* Resource, MappedSubresource* Map)
			{
				VI_ASSERT(Resource != nullptr, "resource should be set");
				VI_ASSERT(Map != nullptr, "map should be set");

				OGLTexture2D* IResource = (OGLTexture2D*)Resource;
				if ((uint32_t)IResource->AccessFlags & (uint32_t)CPUAccess::Write)
				{
					GLint BaseFormat = OGLDevice::GetBaseFormat(IResource->FormatMode);
					GLuint Width = IResource->GetWidth(), Height = IResource->GetHeight();
					glBindTexture(GL_TEXTURE_2D, IResource->Resource);
					glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, Width, Height, BaseFormat, GL_UNSIGNED_BYTE, Map->Pointer);
				}

				Core::Memory::Deallocate(Map->Pointer);
				return Core::Expectation::Met;
			}
			ExpectsGraphics<void> OGLDevice::Unmap(Texture3D* Resource, MappedSubresource* Map)
			{
				VI_ASSERT(Resource != nullptr, "resource should be set");
				VI_ASSERT(Map != nullptr, "map should be set");

				OGLTexture3D* IResource = (OGLTexture3D*)Resource;
				if ((uint32_t)IResource->AccessFlags & (uint32_t)CPUAccess::Write)
				{
					GLint BaseFormat = OGLDevice::GetBaseFormat(IResource->FormatMode);
					GLuint Width = IResource->GetWidth(), Height = IResource->GetHeight(), Depth = IResource->GetDepth();
					glBindTexture(GL_TEXTURE_3D, IResource->Resource);
					glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, Width, Height, Depth, BaseFormat, GL_UNSIGNED_BYTE, Map->Pointer);
				}

				Core::Memory::Deallocate(Map->Pointer);
				return Core::Expectation::Met;
			}
			ExpectsGraphics<void> OGLDevice::Unmap(TextureCube* Resource, MappedSubresource* Map)
			{
				VI_ASSERT(Resource != nullptr, "resource should be set");
				VI_ASSERT(Map != nullptr, "map should be set");

				OGLTextureCube* IResource = (OGLTextureCube*)Resource;
				if ((uint32_t)IResource->AccessFlags & (uint32_t)CPUAccess::Write)
				{
					GLint BaseFormat = OGLDevice::GetBaseFormat(IResource->FormatMode);
					GLuint Width = IResource->GetWidth(), Height = IResource->GetHeight(), Depth = 6;
					glBindTexture(GL_TEXTURE_3D, IResource->Resource);
					glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, Width, Height, Depth, BaseFormat, GL_UNSIGNED_BYTE, Map->Pointer);
				}

				Core::Memory::Deallocate(Map->Pointer);
				return Core::Expectation::Met;
			}
			ExpectsGraphics<void> OGLDevice::Unmap(ElementBuffer* Resource, MappedSubresource* Map)
			{
				VI_ASSERT(Resource != nullptr, "resource should be set");
				OGLElementBuffer* IResource = (OGLElementBuffer*)Resource;
				glUnmapBuffer(IResource->Flags);
				glBindBuffer(IResource->Flags, GL_NONE);
				return Core::Expectation::Met;
			}
			ExpectsGraphics<void> OGLDevice::UpdateConstantBuffer(ElementBuffer* Resource, void* Data, size_t Size)
			{
				VI_ASSERT(Resource != nullptr, "resource should be set");
				OGLElementBuffer* IResource = (OGLElementBuffer*)Resource;
				return CopyConstantBuffer(IResource->Resource, Data, Size);
			}
			ExpectsGraphics<void> OGLDevice::UpdateBuffer(ElementBuffer* Resource, void* Data, size_t Size)
			{
				VI_ASSERT(Resource != nullptr, "resource should be set");
				OGLElementBuffer* IResource = (OGLElementBuffer*)Resource;
				glBindBuffer(IResource->Flags, IResource->Resource);
				glBufferData(IResource->Flags, (GLsizeiptr)Size, Data, GL_DYNAMIC_DRAW);
				glBindBuffer(IResource->Flags, GL_NONE);
				return Core::Expectation::Met;
			}
			ExpectsGraphics<void> OGLDevice::UpdateBuffer(Shader* Resource, const void* Data)
			{
				VI_ASSERT(Resource != nullptr, "resource should be set");
				OGLShader* IResource = (OGLShader*)Resource;
				return CopyConstantBuffer(IResource->ConstantBuffer, (void*)Data, IResource->ConstantSize);
			}
			ExpectsGraphics<void> OGLDevice::UpdateBuffer(MeshBuffer* Resource, Compute::Vertex* Data)
			{
				VI_ASSERT(Resource != nullptr, "resource should be set");
				VI_ASSERT(Data != nullptr, "data should be set");

				OGLMeshBuffer* IResource = (OGLMeshBuffer*)Resource;
				MappedSubresource MappedResource;
				auto MapStatus = Map(IResource->VertexBuffer, ResourceMap::Write, &MappedResource);
				if (!MapStatus)
					return MapStatus;

				memcpy(MappedResource.Pointer, Data, (size_t)IResource->VertexBuffer->GetElements() * sizeof(Compute::Vertex));
				return Unmap(IResource->VertexBuffer, &MappedResource);
			}
			ExpectsGraphics<void> OGLDevice::UpdateBuffer(SkinMeshBuffer* Resource, Compute::SkinVertex* Data)
			{
				VI_ASSERT(Resource != nullptr, "resource should be set");
				VI_ASSERT(Data != nullptr, "data should be set");

				OGLSkinMeshBuffer* IResource = (OGLSkinMeshBuffer*)Resource;
				MappedSubresource MappedResource;
				auto MapStatus = Map(IResource->VertexBuffer, ResourceMap::Write, &MappedResource);
				if (!MapStatus)
					return MapStatus;

				memcpy(MappedResource.Pointer, Data, (size_t)IResource->VertexBuffer->GetElements() * sizeof(Compute::SkinVertex));
				return Unmap(IResource->VertexBuffer, &MappedResource);
			}
			ExpectsGraphics<void> OGLDevice::UpdateBuffer(InstanceBuffer* Resource)
			{
				VI_ASSERT(Resource != nullptr, "resource should be set");
				OGLInstanceBuffer* IResource = (OGLInstanceBuffer*)Resource;
				if (IResource->Array.size() <= 0 || IResource->Array.size() > IResource->ElementLimit)
					return GraphicsException("instance buffer mapping error: invalid array size");

				OGLElementBuffer* Element = (OGLElementBuffer*)IResource->Elements;
				glBindBuffer(Element->Flags, Element->Resource);
				GLvoid* Data = glMapBuffer(Element->Flags, GL_WRITE_ONLY);
				memcpy(Data, IResource->Array.data(), (size_t)IResource->Array.size() * IResource->ElementWidth);
				glUnmapBuffer(Element->Flags);
				glBindBuffer(Element->Flags, GL_NONE);
				return Core::Expectation::Met;
			}
			ExpectsGraphics<void> OGLDevice::UpdateBufferSize(Shader* Resource, size_t Size)
			{
				VI_ASSERT(Resource != nullptr, "resource should be set");
				VI_ASSERT(Size > 0, "size should be greater than zero");

				OGLShader* IResource = (OGLShader*)Resource;
				if (IResource->ConstantBuffer != GL_NONE)
					glDeleteBuffers(1, &IResource->ConstantBuffer);

				auto NewBuffer = CreateConstantBuffer(&IResource->ConstantBuffer, Size);
				IResource->ConstantSize = (NewBuffer ? Size : 0);
				return NewBuffer;
			}
			ExpectsGraphics<void> OGLDevice::UpdateBufferSize(InstanceBuffer* Resource, size_t Size)
			{
				VI_ASSERT(Resource != nullptr, "resource should be set");
				VI_ASSERT(Size > 0, "size should be greater than zero");

				OGLInstanceBuffer* IResource = (OGLInstanceBuffer*)Resource;
				ClearBuffer(IResource);
				Core::Memory::Release(IResource->Elements);
				IResource->ElementLimit = Size;
				IResource->Array.clear();
				IResource->Array.reserve(IResource->ElementLimit);

				ElementBuffer::Desc F = ElementBuffer::Desc();
				F.AccessFlags = CPUAccess::Write;
				F.MiscFlags = ResourceMisc::Buffer_Structured;
				F.Usage = ResourceUsage::Dynamic;
				F.BindFlags = ResourceBind::Shader_Input;
				F.ElementCount = (uint32_t)IResource->ElementLimit;
				F.ElementWidth = (uint32_t)IResource->ElementWidth;
				F.StructureByteStride = F.ElementWidth;

				auto Buffer = CreateElementBuffer(F);
				if (!Buffer)
					return Buffer.Error();

				IResource->Elements = *Buffer;
				return Core::Expectation::Met;
			}
			ExpectsGraphics<void> OGLDevice::CopyTexture2D(Texture2D* Resource, Texture2D** Result)
			{
				VI_ASSERT(Resource != nullptr, "resource should be set");
				VI_ASSERT(Result != nullptr, "result should be set");
				VI_ASSERT(!*Result || !((OGLTexture2D*)(*Result))->Backbuffer, "output resource 2d should not be back buffer");

				OGLTexture2D* IResource = (OGLTexture2D*)Resource;
				if (IResource->Backbuffer)
					return CopyBackBuffer(Result);

				VI_ASSERT(IResource->Resource != GL_NONE, "resource should be valid");
				int LastTexture = GL_NONE, Width, Height;
				glGetIntegerv(GL_TEXTURE_BINDING_2D, &LastTexture);
				glBindTexture(GL_TEXTURE_2D, IResource->Resource);
				glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &Width);
				glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &Height);

				if (!*Result)
				{
					Texture2D::Desc F;
					F.Width = (uint32_t)Width;
					F.Height = (uint32_t)Height;
					F.RowPitch = GetRowPitch(F.Width);
					F.DepthPitch = GetDepthPitch(F.RowPitch, F.Height);
					F.MipLevels = GetMipLevel(F.Width, F.Height);

					auto NewTexture = CreateTexture2D(F);
					if (!NewTexture)
						return NewTexture.Error();

					*Result = (OGLTexture2D*)*NewTexture;
				}

				if (GLEW_VERSION_4_3)
					OGL_CopyTexture_4_3(GL_TEXTURE_2D, IResource->Resource, ((OGLTexture2D*)(*Result))->Resource, Width, Height);
				else
					OGL_CopyTexture_3_0(IResource->Resource, ((OGLTexture2D*)(*Result))->Resource, Width, Height);

				glBindTexture(GL_TEXTURE_2D, LastTexture);
				return Core::Expectation::Met;
			}
			ExpectsGraphics<void> OGLDevice::CopyTexture2D(Graphics::RenderTarget* Resource, uint32_t Target, Texture2D** Result)
			{
				VI_ASSERT(Resource != nullptr, "resource should be set");
				VI_ASSERT(Result != nullptr, "result should be set");

				int LastTexture = GL_NONE;
				glGetIntegerv(GL_TEXTURE_BINDING_2D, &LastTexture);
				OGLFrameBuffer* TargetBuffer = (OGLFrameBuffer*)Resource->GetTargetBuffer();

				VI_ASSERT(TargetBuffer != nullptr, "target buffer should be set");
				if (TargetBuffer->Backbuffer)
				{
					if (!*Result)
					{
						auto NewTexture = CreateTexture2D();
						if (!NewTexture)
							return NewTexture.Error();

						*Result = (OGLTexture2D*)*NewTexture;
						glGenTextures(1, &((OGLTexture2D*)(*Result))->Resource);
					}

					GLint Viewport[4];
					glGetIntegerv(GL_VIEWPORT, Viewport);
					glBindTexture(GL_TEXTURE_2D, ((OGLTexture2D*)(*Result))->Resource);
					glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, (GLsizei)Viewport[2], (GLsizei)Viewport[3]);
					glBindTexture(GL_TEXTURE_2D, LastTexture);
					return Core::Expectation::Met;
				}

				OGLTexture2D* Source = (OGLTexture2D*)Resource->GetTarget2D(Target);
				if (!Source)
					return GraphicsException("copy texture 2d: invalid target");

				int Width, Height;
				glBindTexture(GL_TEXTURE_2D, Source->Resource);
				glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &Width);
				glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &Height);

				if (!*Result)
				{
					Texture2D::Desc F;
					F.Width = (uint32_t)Width;
					F.Height = (uint32_t)Height;
					F.RowPitch = GetRowPitch(F.Width);
					F.DepthPitch = GetDepthPitch(F.RowPitch, F.Height);
					F.MipLevels = GetMipLevel(F.Width, F.Height);

					auto NewTexture = CreateTexture2D(F);
					if (!NewTexture)
						return NewTexture.Error();

					*Result = (OGLTexture2D*)*NewTexture;
				}

				if (GLEW_VERSION_4_3)
					OGL_CopyTexture_4_3(GL_TEXTURE_2D, Source->Resource, ((OGLTexture2D*)(*Result))->Resource, Width, Height);
				else
					OGL_CopyTexture_3_0(Source->Resource, ((OGLTexture2D*)(*Result))->Resource, Width, Height);

				glBindTexture(GL_TEXTURE_2D, LastTexture);
				return Core::Expectation::Met;
			}
			ExpectsGraphics<void> OGLDevice::CopyTexture2D(RenderTargetCube* Resource, Compute::CubeFace Face, Texture2D** Result)
			{
				VI_ASSERT(Resource != nullptr, "resource should be set");
				VI_ASSERT(Result != nullptr, "result should be set");

				OGLRenderTargetCube* IResource = (OGLRenderTargetCube*)Resource;
				int LastTexture = GL_NONE, Width, Height;
				glGetIntegerv(GL_TEXTURE_BINDING_CUBE_MAP, &LastTexture);
				glBindTexture(GL_TEXTURE_CUBE_MAP, IResource->FrameBuffer.Texture[0]);
				glGetTexLevelParameteriv(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_TEXTURE_WIDTH, &Width);
				glGetTexLevelParameteriv(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_TEXTURE_HEIGHT, &Height);

				if (!*Result)
				{
					Texture2D::Desc F;
					F.Width = (uint32_t)Width;
					F.Height = (uint32_t)Height;
					F.RowPitch = GetRowPitch(F.Width);
					F.DepthPitch = GetDepthPitch(F.RowPitch, F.Height);
					F.MipLevels = GetMipLevel(F.Width, F.Height);

					auto NewTexture = CreateTexture2D(F);
					if (!NewTexture)
						return NewTexture.Error();

					*Result = (OGLTexture2D*)*NewTexture;
				}

				OGL_CopyTextureFace2D_3_0(Face, IResource->FrameBuffer.Texture[0], ((OGLTexture2D*)(*Result))->Resource, Width, Height);
				glBindTexture(GL_TEXTURE_CUBE_MAP, LastTexture);
				return Core::Expectation::Met;
			}
			ExpectsGraphics<void> OGLDevice::CopyTexture2D(MultiRenderTargetCube* Resource, uint32_t Cube, Compute::CubeFace Face, Texture2D** Result)
			{
				VI_ASSERT(Resource != nullptr, "resource should be set");
				VI_ASSERT(Result != nullptr, "result should be set");

				OGLMultiRenderTargetCube* IResource = (OGLMultiRenderTargetCube*)Resource;

				VI_ASSERT(Cube < (uint32_t)IResource->Target, "cube index should be less than %i", (int)IResource->Target);
				int LastTexture = GL_NONE, Width, Height;
				glGetIntegerv(GL_TEXTURE_BINDING_CUBE_MAP, &LastTexture);
				glBindTexture(GL_TEXTURE_CUBE_MAP, IResource->FrameBuffer.Texture[Cube]);
				glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &Width);
				glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &Height);

				if (!*Result)
				{
					Texture2D::Desc F;
					F.Width = (uint32_t)Width;
					F.Height = (uint32_t)Height;
					F.RowPitch = GetRowPitch(F.Width);
					F.DepthPitch = GetDepthPitch(F.RowPitch, F.Height);
					F.MipLevels = GetMipLevel(F.Width, F.Height);

					auto NewTexture = CreateTexture2D(F);
					if (!NewTexture)
						return NewTexture.Error();

					*Result = (OGLTexture2D*)*NewTexture;
				}

				OGL_CopyTextureFace2D_3_0(Face, IResource->FrameBuffer.Texture[Cube], ((OGLTexture2D*)(*Result))->Resource, Width, Height);
				glBindTexture(GL_TEXTURE_CUBE_MAP, LastTexture);
				return Core::Expectation::Met;
			}
			ExpectsGraphics<void> OGLDevice::CopyTextureCube(RenderTargetCube* Resource, TextureCube** Result)
			{
				VI_ASSERT(Resource != nullptr, "resource should be set");
				VI_ASSERT(Result != nullptr, "result should be set");

				OGLRenderTargetCube* IResource = (OGLRenderTargetCube*)Resource;
				int LastTexture = GL_NONE, Width, Height;
				glGetIntegerv(GL_TEXTURE_BINDING_CUBE_MAP, &LastTexture);
				glBindTexture(GL_TEXTURE_CUBE_MAP, IResource->FrameBuffer.Texture[0]);
				glGetTexLevelParameteriv(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_TEXTURE_WIDTH, &Width);
				glGetTexLevelParameteriv(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_TEXTURE_HEIGHT, &Height);

				if (!*Result)
				{
					Texture2D* Faces[6] = { nullptr };
					for (uint32_t i = 0; i < 6; i++)
						CopyTexture2D(Resource, i, &Faces[i]);

					auto NewTexture = CreateTextureCube(Faces);
					if (!NewTexture)
					{
						for (uint32_t i = 0; i < 6; i++)
							Core::Memory::Release(Faces[i]);
						return NewTexture.Error();
					}
					else
					{
						*Result = (OGLTextureCube*)*NewTexture;
						for (uint32_t i = 0; i < 6; i++)
							Core::Memory::Release(Faces[i]);
					}
				}
				else if (GLEW_VERSION_4_3)
					OGL_CopyTexture_4_3(GL_TEXTURE_CUBE_MAP, IResource->FrameBuffer.Texture[0], ((OGLTextureCube*)(*Result))->Resource, Width, Height);
				else
					OGL_CopyTexture_3_0(IResource->FrameBuffer.Texture[0], ((OGLTextureCube*)(*Result))->Resource, Width, Height);

				glBindTexture(GL_TEXTURE_CUBE_MAP, LastTexture);
				return Core::Expectation::Met;
			}
			ExpectsGraphics<void> OGLDevice::CopyTextureCube(MultiRenderTargetCube* Resource, uint32_t Cube, TextureCube** Result)
			{
				VI_ASSERT(Resource != nullptr, "resource should be set");
				VI_ASSERT(Result != nullptr, "result should be set");

				OGLMultiRenderTargetCube* IResource = (OGLMultiRenderTargetCube*)Resource;
				VI_ASSERT(Cube < (uint32_t)IResource->Target, "cube index should be less than %i", (int)IResource->Target);
				int LastTexture = GL_NONE, Width, Height;
				glGetIntegerv(GL_TEXTURE_BINDING_CUBE_MAP, &LastTexture);
				glBindTexture(GL_TEXTURE_CUBE_MAP, IResource->FrameBuffer.Texture[Cube]);
				glGetTexLevelParameteriv(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_TEXTURE_WIDTH, &Width);
				glGetTexLevelParameteriv(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_TEXTURE_HEIGHT, &Height);

				if (!*Result)
				{
					Texture2D* Faces[6] = { nullptr };
					for (uint32_t i = 0; i < 6; i++)
						CopyTexture2D(Resource, Cube, (Compute::CubeFace)i, &Faces[i]);

					auto NewTexture = CreateTextureCube(Faces);
					if (!NewTexture)
					{
						for (uint32_t i = 0; i < 6; i++)
							Core::Memory::Release(Faces[i]);
						return NewTexture.Error();
					}
					else
					{
						*Result = (OGLTextureCube*)*NewTexture;
						for (uint32_t i = 0; i < 6; i++)
							Core::Memory::Release(Faces[i]);
					}
				}
				else if (GLEW_VERSION_4_3)
					OGL_CopyTexture_4_3(GL_TEXTURE_CUBE_MAP, IResource->FrameBuffer.Texture[Cube], ((OGLTextureCube*)(*Result))->Resource, Width, Height);
				else
					OGL_CopyTexture_3_0(IResource->FrameBuffer.Texture[Cube], ((OGLTextureCube*)(*Result))->Resource, Width, Height);

				glBindTexture(GL_TEXTURE_CUBE_MAP, LastTexture);
				return Core::Expectation::Met;
			}
			ExpectsGraphics<void> OGLDevice::CopyTarget(Graphics::RenderTarget* From, uint32_t FromTarget, Graphics::RenderTarget* To, uint32_t ToTarget)
			{
				VI_ASSERT(From != nullptr && To != nullptr, "from and to should be set");
				OGLTexture2D* Source2D = (OGLTexture2D*)From->GetTarget2D(FromTarget);
				OGLTextureCube* SourceCube = (OGLTextureCube*)From->GetTargetCube(FromTarget);
				OGLTexture2D* Dest2D = (OGLTexture2D*)To->GetTarget2D(ToTarget);
				OGLTextureCube* DestCube = (OGLTextureCube*)To->GetTargetCube(ToTarget);
				GLuint Source = (Source2D ? Source2D->Resource : (SourceCube ? SourceCube->Resource : GL_NONE));
				GLuint Dest = (Dest2D ? Dest2D->Resource : (DestCube ? DestCube->Resource : GL_NONE));

				VI_ASSERT(Source != GL_NONE, "from should be set");
				VI_ASSERT(Dest != GL_NONE, "to should be set");

				int LastTexture = GL_NONE;
				uint32_t Width = From->GetWidth();
				uint32_t Height = From->GetHeight();
				glGetIntegerv(SourceCube ? GL_TEXTURE_BINDING_CUBE_MAP : GL_TEXTURE_BINDING_2D, &LastTexture);

				if (GLEW_VERSION_4_3)
				{
					if (SourceCube != nullptr)
						OGL_CopyTexture_4_3(GL_TEXTURE_CUBE_MAP, Source, Dest, Width, Height);
					else
						OGL_CopyTexture_4_3(GL_TEXTURE_2D, Source, Dest, Width, Height);
				}
				else
					OGL_CopyTexture_3_0(Source, Dest, Width, Height);

				glBindTexture(SourceCube ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D, LastTexture);
				return Core::Expectation::Met;
			}
			ExpectsGraphics<void> OGLDevice::CubemapPush(Cubemap* Resource, TextureCube* Result)
			{
				VI_ASSERT(Resource != nullptr, "resource should be set");
				VI_ASSERT(Resource->IsValid(), "resource should be valid");
				VI_ASSERT(Result != nullptr, "result should be set");

				OGLCubemap* IResource = (OGLCubemap*)Resource;
				OGLTextureCube* Dest = (OGLTextureCube*)Result;
				IResource->Dest = Dest;

				if (Dest->Resource != GL_NONE)
					return Core::Expectation::Met;

				GLint Size = IResource->Meta.Size;
				Dest->FormatMode = IResource->Options.FormatMode;
				Dest->Format = IResource->Options.SizeFormat;
				Dest->MipLevels = IResource->Meta.MipLevels;
				Dest->Width = Size;
				Dest->Height = Size;

				glGenTextures(1, &Dest->Resource);
				glBindTexture(GL_TEXTURE_CUBE_MAP, Dest->Resource);
				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

				if (Dest->MipLevels > 0)
				{
					glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
					glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
					glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_BASE_LEVEL, 0);
					glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LEVEL, Dest->MipLevels - 1);
				}
				else
				{
					glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
					glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				}

				glTexStorage2D(GL_TEXTURE_CUBE_MAP, Dest->MipLevels, Dest->Format, Size, Size);
				glBindTexture(GL_TEXTURE_CUBE_MAP, GL_NONE);
				return Core::Expectation::Met;
			}
			ExpectsGraphics<void> OGLDevice::CubemapFace(Cubemap* Resource, Compute::CubeFace Face)
			{
				VI_ASSERT(Resource != nullptr, "resource should be set");
				VI_ASSERT(Resource->IsValid(), "resource should be valid");

				OGLCubemap* IResource = (OGLCubemap*)Resource;
				OGLTextureCube* Dest = (OGLTextureCube*)IResource->Dest;
				VI_ASSERT(IResource->Dest != nullptr, "result should be set");

				GLint LastFrameBuffer = 0, Size = IResource->Meta.Size;
				glGetIntegerv(GL_FRAMEBUFFER_BINDING, &LastFrameBuffer);
				glBindFramebuffer(GL_FRAMEBUFFER, IResource->FrameBuffer);
				glFramebufferTexture(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, IResource->Source, 0);
				glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_CUBE_MAP_POSITIVE_X + (uint32_t)Face, Dest->Resource, 0);
				glNamedFramebufferDrawBuffer(IResource->FrameBuffer, GL_COLOR_ATTACHMENT1);
				glBlitFramebuffer(0, 0, Size, Size, 0, 0, Size, Size, GL_COLOR_BUFFER_BIT, GL_NEAREST);
				glBindFramebuffer(GL_FRAMEBUFFER, LastFrameBuffer);
				return Core::Expectation::Met;
			}
			ExpectsGraphics<void> OGLDevice::CubemapPop(Cubemap* Resource)
			{
				VI_ASSERT(Resource != nullptr, "resource should be set");
				VI_ASSERT(Resource->IsValid(), "resource should be valid");

				OGLCubemap* IResource = (OGLCubemap*)Resource;
				OGLTextureCube* Dest = (OGLTextureCube*)IResource->Dest;
				VI_ASSERT(IResource->Dest != nullptr, "result should be set");
				if (IResource->Meta.MipLevels > 0)
					GenerateMips(Dest);

				return Core::Expectation::Met;
			}
			ExpectsGraphics<void> OGLDevice::CopyBackBuffer(Texture2D** Result)
			{
				VI_ASSERT(Result != nullptr, "result should be set");
				VI_ASSERT(!*Result || !((OGLTexture2D*)(*Result))->Backbuffer, "output resource 2d should not be back buffer");
				OGLTexture2D* Texture = (OGLTexture2D*)*Result;
				if (!Texture)
				{
					auto NewTexture = CreateTexture2D();
					if (!NewTexture)
						return NewTexture.Error();

					glGenTextures(1, &Texture->Resource);
					*Result = Texture;
				}

				GLint Viewport[4];
				glGetIntegerv(GL_VIEWPORT, Viewport);
				glBindTexture(GL_TEXTURE_2D, Texture->Resource);
				glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 0, 0, (GLsizei)Viewport[2], (GLsizei)Viewport[3], 0);
				glGenerateMipmap(GL_TEXTURE_2D);
				return GenerateTexture(Texture);
			}
			ExpectsGraphics<void> OGLDevice::ResizeBuffers(uint32_t Width, uint32_t Height)
			{
				RenderTarget2D::Desc F = RenderTarget2D::Desc();
				F.Width = Width;
				F.Height = Height;
				F.MipLevels = 1;
				F.MiscFlags = ResourceMisc::None;
				F.FormatMode = Format::R8G8B8A8_Unorm;
				F.Usage = ResourceUsage::Default;
				F.AccessFlags = CPUAccess::None;
				F.BindFlags = ResourceBind::Render_Target | ResourceBind::Shader_Input;
				F.RenderSurface = (void*)this;
				Core::Memory::Release(RenderTarget);

				auto NewTarget = CreateRenderTarget2D(F);
				if (!NewTarget)
					return NewTarget.Error();

				RenderTarget = *NewTarget;
				SetTarget();
				return Core::Expectation::Met;
			}
			ExpectsGraphics<void> OGLDevice::GenerateTexture(Texture2D* Resource)
			{
				VI_ASSERT(Resource != nullptr, "resource should be set");
				OGLTexture2D* IResource = (OGLTexture2D*)Resource;
				if (IResource->Backbuffer)
				{
					IResource->Width = RenderTarget->GetWidth();
					IResource->Height = RenderTarget->GetHeight();
					return Core::Expectation::Met;
				}

				VI_ASSERT(IResource->Resource != GL_NONE, "resource should be valid");
				int Width, Height;
				glBindTexture(GL_TEXTURE_2D, IResource->Resource);
				glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &Width);
				glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &Height);
				glBindTexture(GL_TEXTURE_2D, GL_NONE);

				IResource->Width = Width;
				IResource->Height = Height;
				return Core::Expectation::Met;
			}
			ExpectsGraphics<void> OGLDevice::GenerateTexture(Texture3D* Resource)
			{
				VI_ASSERT(Resource != nullptr, "resource should be set");
				OGLTexture3D* IResource = (OGLTexture3D*)Resource;

				VI_ASSERT(IResource->Resource != GL_NONE, "resource should be valid");
				int Width, Height, Depth;
				glBindTexture(GL_TEXTURE_3D, IResource->Resource);
				glGetTexLevelParameteriv(GL_TEXTURE_3D, 0, GL_TEXTURE_WIDTH, &Width);
				glGetTexLevelParameteriv(GL_TEXTURE_3D, 0, GL_TEXTURE_HEIGHT, &Height);
				glGetTexLevelParameteriv(GL_TEXTURE_3D, 0, GL_TEXTURE_DEPTH, &Depth);
				glBindTexture(GL_TEXTURE_3D, GL_NONE);

				IResource->Width = Width;
				IResource->Height = Height;
				IResource->Depth = Depth;
				return Core::Expectation::Met;
			}
			ExpectsGraphics<void> OGLDevice::GenerateTexture(TextureCube* Resource)
			{
				VI_ASSERT(Resource != nullptr, "resource should be set");
				OGLTextureCube* IResource = (OGLTextureCube*)Resource;

				VI_ASSERT(IResource->Resource != GL_NONE, "resource should be valid");
				int Width, Height;
				glBindTexture(GL_TEXTURE_CUBE_MAP, IResource->Resource);
				glGetTexLevelParameteriv(GL_TEXTURE_CUBE_MAP, 0, GL_TEXTURE_WIDTH, &Width);
				glGetTexLevelParameteriv(GL_TEXTURE_CUBE_MAP, 0, GL_TEXTURE_HEIGHT, &Height);
				glBindTexture(GL_TEXTURE_CUBE_MAP, GL_NONE);

				IResource->Width = Width;
				IResource->Height = Height;
				return Core::Expectation::Met;
			}
			ExpectsGraphics<void> OGLDevice::GetQueryData(Query* Resource, size_t* Result, bool Flush)
			{
				VI_ASSERT(Resource != nullptr, "resource should be set");
				VI_ASSERT(Result != nullptr, "result should be set");

				OGLQuery* IResource = (OGLQuery*)Resource;
				GLint Available = 0;
				glGetQueryObjectiv(IResource->Async, GL_QUERY_RESULT_AVAILABLE, &Available);
				if (Available == GL_FALSE)
					return GetException("query data");

				GLint64 Passing = 0;
				glGetQueryObjecti64v(IResource->Async, GL_QUERY_RESULT, &Passing);
				*Result = (size_t)Passing;
				return Core::Expectation::Met;
			}
			ExpectsGraphics<void> OGLDevice::GetQueryData(Query* Resource, bool* Result, bool Flush)
			{
				VI_ASSERT(Resource != nullptr, "resource should be set");
				VI_ASSERT(Result != nullptr, "result should be set");

				OGLQuery* IResource = (OGLQuery*)Resource;
				GLint Available = 0;
				glGetQueryObjectiv(IResource->Async, GL_QUERY_RESULT_AVAILABLE, &Available);
				if (Available == GL_FALSE)
					return GetException("query data");

				GLint Data = 0;
				glGetQueryObjectiv(IResource->Async, GL_QUERY_RESULT, &Data);
				*Result = (Data == GL_TRUE);
				return Core::Expectation::Met;
			}
			ExpectsGraphics<DepthStencilState*> OGLDevice::CreateDepthStencilState(const DepthStencilState::Desc& I)
			{
				return new OGLDepthStencilState(I);
			}
			ExpectsGraphics<BlendState*> OGLDevice::CreateBlendState(const BlendState::Desc& I)
			{
				return new OGLBlendState(I);
			}
			ExpectsGraphics<RasterizerState*> OGLDevice::CreateRasterizerState(const RasterizerState::Desc& I)
			{
				return new OGLRasterizerState(I);
			}
			ExpectsGraphics<SamplerState*> OGLDevice::CreateSamplerState(const SamplerState::Desc& I)
			{
				GLuint DeviceState = 0;
				glGenSamplers(1, &DeviceState);
				glSamplerParameteri(DeviceState, GL_TEXTURE_WRAP_S, OGLDevice::GetTextureAddress(I.AddressU));
				glSamplerParameteri(DeviceState, GL_TEXTURE_WRAP_T, OGLDevice::GetTextureAddress(I.AddressV));
				glSamplerParameteri(DeviceState, GL_TEXTURE_WRAP_R, OGLDevice::GetTextureAddress(I.AddressW));
				glSamplerParameteri(DeviceState, GL_TEXTURE_MAG_FILTER, OGLDevice::GetPixelFilter(I.Filter, true));
				glSamplerParameteri(DeviceState, GL_TEXTURE_MIN_FILTER, OGLDevice::GetPixelFilter(I.Filter, false));
				glSamplerParameteri(DeviceState, GL_TEXTURE_COMPARE_MODE, OGLDevice::IsComparator(I.Filter) ? GL_COMPARE_REF_TO_TEXTURE : GL_NONE);
				glSamplerParameteri(DeviceState, GL_TEXTURE_COMPARE_FUNC, OGLDevice::GetComparison(I.ComparisonFunction));
				glSamplerParameterf(DeviceState, GL_TEXTURE_LOD_BIAS, I.MipLODBias);
				glSamplerParameterf(DeviceState, GL_TEXTURE_MAX_LOD, I.MaxLOD);
				glSamplerParameterf(DeviceState, GL_TEXTURE_MIN_LOD, I.MinLOD);
				glSamplerParameterfv(DeviceState, GL_TEXTURE_BORDER_COLOR, (GLfloat*)I.BorderColor);
				glSamplerParameterf(DeviceState, GL_TEXTURE_MAX_ANISOTROPY, (float)I.MaxAnisotropy);

				OGLSamplerState* Result = new OGLSamplerState(I);
				Result->Resource = DeviceState;
				return Result;
			}
			ExpectsGraphics<InputLayout*> OGLDevice::CreateInputLayout(const InputLayout::Desc& I)
			{
				return new OGLInputLayout(I);
			}
			ExpectsGraphics<Shader*> OGLDevice::CreateShader(const Shader::Desc& I)
			{
				Shader::Desc F(I);
				auto PreprocessStatus = Preprocess(F);
				if (!PreprocessStatus)
					return GraphicsException(std::move(PreprocessStatus.Error().message()));

				auto Name = GetProgramName(F);
				if (!Name)
					return GraphicsException("shader name is not defined");

				Core::UPtr<OGLShader> Result = new OGLShader(I);
				Core::String ProgramName = std::move(*Name);
				const char* Data = nullptr;
				GLint Size = 0, State = GL_TRUE;

				Core::String VertexEntry = GetShaderMain(ShaderType::Vertex);
				if (F.Data.find(VertexEntry) != Core::String::npos)
				{
					Core::String Stage = ProgramName + SHADER_VERTEX, Bytecode;
					if (!GetProgramCache(Stage, &Bytecode))
					{
						VI_DEBUG("[opengl] transpile %s vertex shader source", Stage.c_str());
						Bytecode = F.Data;

						auto TranspileStatus = Transpile(&Bytecode, ShaderType::Vertex, ShaderLang::GLSL);
						if (!TranspileStatus)
							return TranspileStatus.Error();

						Data = Bytecode.c_str();
						Size = (GLint)Bytecode.size();
						SetProgramCache(Stage, Bytecode);
					}
					else
					{
						Data = Bytecode.c_str();
						Size = (GLint)Bytecode.size();
					}

					VI_DEBUG("[opengl] compile %s vertex shader bytecode", Stage.c_str());
					Result->VertexShader = glCreateShader(GL_VERTEX_SHADER);
					glShaderSourceARB(Result->VertexShader, 1, &Data, &Size);
					glCompileShaderARB(Result->VertexShader);
					glGetShaderiv(Result->VertexShader, GL_COMPILE_STATUS, &State);
					if (State == GL_FALSE)
					{
						glGetShaderiv(Result->VertexShader, GL_INFO_LOG_LENGTH, &Size);
						char* Buffer = Core::Memory::Allocate<char>(sizeof(char) * (Size + 1));
						glGetShaderInfoLog(Result->VertexShader, Size, &Size, Buffer);
						Core::String ErrorText(Buffer, (size_t)Size);
						Core::Memory::Deallocate(Buffer);
						return GraphicsException(std::move(ErrorText));
					}
				}

				Core::String PixelEntry = GetShaderMain(ShaderType::Pixel);
				if (F.Data.find(PixelEntry) != Core::String::npos)
				{
					Core::String Stage = ProgramName + SHADER_PIXEL, Bytecode;
					if (!GetProgramCache(Stage, &Bytecode))
					{
						VI_DEBUG("[opengl] transpile %s pixel shader source", Stage.c_str());
						Bytecode = F.Data;

						auto TranspileStatus = Transpile(&Bytecode, ShaderType::Pixel, ShaderLang::GLSL);
						if (!TranspileStatus)
							return TranspileStatus.Error();

						Data = Bytecode.c_str();
						Size = (GLint)Bytecode.size();
						SetProgramCache(Stage, Bytecode);
					}
					else
					{
						Data = Bytecode.c_str();
						Size = (GLint)Bytecode.size();
					}

					VI_DEBUG("[opengl] compile %s pixel shader bytecode", Stage.c_str());
					Result->PixelShader = glCreateShader(GL_FRAGMENT_SHADER);
					glShaderSourceARB(Result->PixelShader, 1, &Data, &Size);
					glCompileShaderARB(Result->PixelShader);
					glGetShaderiv(Result->PixelShader, GL_COMPILE_STATUS, &State);
					if (State == GL_FALSE)
					{
						glGetShaderiv(Result->PixelShader, GL_INFO_LOG_LENGTH, &Size);
						char* Buffer = Core::Memory::Allocate<char>(sizeof(char) * (Size + 1));
						glGetShaderInfoLog(Result->PixelShader, Size, &Size, Buffer);
						Core::String ErrorText(Buffer, (size_t)Size);
						Core::Memory::Deallocate(Buffer);
						return GraphicsException(std::move(ErrorText));
					}
				}

				Core::String GeometryEntry = GetShaderMain(ShaderType::Geometry);
				if (F.Data.find(GeometryEntry) != Core::String::npos)
				{
					Core::String Stage = ProgramName + SHADER_GEOMETRY, Bytecode;
					if (!GetProgramCache(Stage, &Bytecode))
					{
						VI_DEBUG("[opengl] transpile %s geometry shader source", Stage.c_str());
						Bytecode = F.Data;

						auto TranspileStatus = Transpile(&Bytecode, ShaderType::Geometry, ShaderLang::GLSL);
						if (!TranspileStatus)
							return TranspileStatus.Error();

						Data = Bytecode.c_str();
						Size = (GLint)Bytecode.size();
						SetProgramCache(Stage, Bytecode);
					}
					else
					{
						Data = Bytecode.c_str();
						Size = (GLint)Bytecode.size();
					}

					VI_DEBUG("[opengl] compile %s geometry shader bytecode", Stage.c_str());
					Result->GeometryShader = glCreateShader(GL_GEOMETRY_SHADER);
					glShaderSourceARB(Result->GeometryShader, 1, &Data, &Size);
					glCompileShaderARB(Result->GeometryShader);
					glGetShaderiv(Result->GeometryShader, GL_COMPILE_STATUS, &State);
					if (State == GL_FALSE)
					{
						glGetShaderiv(Result->GeometryShader, GL_INFO_LOG_LENGTH, &Size);
						char* Buffer = Core::Memory::Allocate<char>(sizeof(char) * (Size + 1));
						glGetShaderInfoLog(Result->GeometryShader, Size, &Size, Buffer);
						Core::String ErrorText(Buffer, (size_t)Size);
						Core::Memory::Deallocate(Buffer);
						return GraphicsException(std::move(ErrorText));
					}
				}

				Core::String ComputeEntry = GetShaderMain(ShaderType::Compute);
				if (F.Data.find(ComputeEntry) != Core::String::npos)
				{
					Core::String Stage = ProgramName + SHADER_COMPUTE, Bytecode;
					if (!GetProgramCache(Stage, &Bytecode))
					{
						VI_DEBUG("[opengl] transpile %s compute shader source", Stage.c_str());
						Bytecode = F.Data;

						auto TranspileStatus = Transpile(&Bytecode, ShaderType::Compute, ShaderLang::GLSL);
						if (!TranspileStatus)
							return TranspileStatus.Error();

						Data = Bytecode.c_str();
						Size = (GLint)Bytecode.size();
						SetProgramCache(Stage, Bytecode);
					}
					else
					{
						Data = Bytecode.c_str();
						Size = (GLint)Bytecode.size();
					}

					VI_DEBUG("[opengl] compile %s compute shader bytecode", Stage.c_str());
					Result->ComputeShader = glCreateShader(GL_COMPUTE_SHADER);
					glShaderSourceARB(Result->ComputeShader, 1, &Data, &Size);
					glCompileShaderARB(Result->ComputeShader);
					glGetShaderiv(Result->ComputeShader, GL_COMPILE_STATUS, &State);
					if (State == GL_FALSE)
					{
						glGetShaderiv(Result->ComputeShader, GL_INFO_LOG_LENGTH, &Size);
						char* Buffer = Core::Memory::Allocate<char>(sizeof(char) * (Size + 1));
						glGetShaderInfoLog(Result->ComputeShader, Size, &Size, Buffer);
						Core::String ErrorText(Buffer, (size_t)Size);
						Core::Memory::Deallocate(Buffer);
						return GraphicsException(std::move(ErrorText));
					}
				}

				Core::String HullEntry = GetShaderMain(ShaderType::Hull);
				if (F.Data.find(HullEntry) != Core::String::npos)
				{
					Core::String Stage = ProgramName + SHADER_HULL, Bytecode;
					if (!GetProgramCache(Stage, &Bytecode))
					{
						VI_DEBUG("[opengl] transpile %s hull shader source", Stage.c_str());
						Bytecode = F.Data;

						auto TranspileStatus = Transpile(&Bytecode, ShaderType::Hull, ShaderLang::GLSL);
						if (!TranspileStatus)
							return TranspileStatus.Error();

						Data = Bytecode.c_str();
						Size = (GLint)Bytecode.size();
						SetProgramCache(Stage, Bytecode);
					}
					else
					{
						Data = Bytecode.c_str();
						Size = (GLint)Bytecode.size();
					}

					VI_DEBUG("[opengl] compile %s hull shader bytecode", Stage.c_str());
					Result->HullShader = glCreateShader(GL_TESS_CONTROL_SHADER);
					glShaderSourceARB(Result->HullShader, 1, &Data, &Size);
					glCompileShaderARB(Result->HullShader);
					glGetShaderiv(Result->HullShader, GL_COMPILE_STATUS, &State);
					if (State == GL_FALSE)
					{
						glGetShaderiv(Result->HullShader, GL_INFO_LOG_LENGTH, &Size);
						char* Buffer = Core::Memory::Allocate<char>(sizeof(char) * (Size + 1));
						glGetShaderInfoLog(Result->HullShader, Size, &Size, Buffer);
						Core::String ErrorText(Buffer, (size_t)Size);
						Core::Memory::Deallocate(Buffer);
						return GraphicsException(std::move(ErrorText));
					}
				}

				Core::String DomainEntry = GetShaderMain(ShaderType::Domain);
				if (F.Data.find(DomainEntry) != Core::String::npos)
				{
					Core::String Stage = ProgramName + SHADER_DOMAIN, Bytecode;
					if (!GetProgramCache(Stage, &Bytecode))
					{
						VI_DEBUG("[opengl] transpile %s domain shader source", Stage.c_str());
						Bytecode = F.Data;

						auto TranspileStatus = Transpile(&Bytecode, ShaderType::Domain, ShaderLang::GLSL);
						if (!TranspileStatus)
							return TranspileStatus.Error();

						Data = Bytecode.c_str();
						Size = (GLint)Bytecode.size();
						SetProgramCache(Stage, Bytecode);
					}
					else
					{
						Data = Bytecode.c_str();
						Size = (GLint)Bytecode.size();
					}

					VI_DEBUG("[opengl] compile %s domain shader bytecode", Stage.c_str());
					Result->DomainShader = glCreateShader(GL_TESS_EVALUATION_SHADER);
					glShaderSourceARB(Result->DomainShader, 1, &Data, &Size);
					glCompileShaderARB(Result->DomainShader);
					glGetShaderiv(Result->DomainShader, GL_COMPILE_STATUS, &State);
					if (State == GL_FALSE)
					{
						glGetShaderiv(Result->DomainShader, GL_INFO_LOG_LENGTH, &Size);
						char* Buffer = Core::Memory::Allocate<char>(sizeof(char) * (Size + 1));
						glGetShaderInfoLog(Result->DomainShader, Size, &Size, Buffer);
						Core::String ErrorText(Buffer, (size_t)Size);
						Core::Memory::Deallocate(Buffer);
						return GraphicsException(std::move(ErrorText));
					}
				}

				Result->Compiled = true;
				return Result.Reset();
			}
			ExpectsGraphics<ElementBuffer*> OGLDevice::CreateElementBuffer(const ElementBuffer::Desc& I)
			{
				OGLElementBuffer* Result = new OGLElementBuffer(I);
				Result->Flags = OGLDevice::GetResourceBind(I.BindFlags);
				if ((size_t)I.MiscFlags & (size_t)ResourceMisc::Buffer_Structured)
					Result->Flags = GL_SHADER_STORAGE_BUFFER;

				glGenBuffers(1, &Result->Resource);
				glBindBuffer(Result->Flags, Result->Resource);
				glBufferData(Result->Flags, I.ElementCount * I.ElementWidth, I.Elements, GetAccessControl(I.AccessFlags, I.Usage));
				glBindBuffer(Result->Flags, GL_NONE);
				return Result;
			}
			ExpectsGraphics<MeshBuffer*> OGLDevice::CreateMeshBuffer(const MeshBuffer::Desc& I)
			{
				ElementBuffer::Desc F = ElementBuffer::Desc();
				F.AccessFlags = I.AccessFlags;
				F.Usage = I.Usage;
				F.BindFlags = ResourceBind::Vertex_Buffer;
				F.ElementCount = (uint32_t)I.Elements.size();
				F.Elements = (void*)I.Elements.data();
				F.ElementWidth = sizeof(Compute::Vertex);

				auto NewVertexBuffer = CreateElementBuffer(F);
				if (!NewVertexBuffer)
					return NewVertexBuffer.Error();

				F = ElementBuffer::Desc();
				F.AccessFlags = I.AccessFlags;
				F.Usage = I.Usage;
				F.BindFlags = ResourceBind::Index_Buffer;
				F.ElementCount = (uint32_t)I.Indices.size();
				F.ElementWidth = sizeof(int);
				F.Elements = (void*)I.Indices.data();

				auto NewIndexBuffer = CreateElementBuffer(F);
				if (!NewIndexBuffer)
				{
					Core::Memory::Release(*NewVertexBuffer);
					return NewIndexBuffer.Error();
				}

				OGLMeshBuffer* Result = new OGLMeshBuffer(I);
				Result->VertexBuffer = *NewVertexBuffer;
				Result->IndexBuffer = *NewIndexBuffer;
				return Result;
			}
			ExpectsGraphics<MeshBuffer*> OGLDevice::CreateMeshBuffer(ElementBuffer* VertexBuffer, ElementBuffer* IndexBuffer)
			{
				VI_ASSERT(VertexBuffer != nullptr, "vertex buffer should be set");
				VI_ASSERT(IndexBuffer != nullptr, "index buffer should be set");
				OGLMeshBuffer* Result = new OGLMeshBuffer(OGLMeshBuffer::Desc());
				Result->VertexBuffer = VertexBuffer;
				Result->IndexBuffer = IndexBuffer;
				return Result;
			}
			ExpectsGraphics<SkinMeshBuffer*> OGLDevice::CreateSkinMeshBuffer(const SkinMeshBuffer::Desc& I)
			{
				ElementBuffer::Desc F = ElementBuffer::Desc();
				F.AccessFlags = I.AccessFlags;
				F.Usage = I.Usage;
				F.BindFlags = ResourceBind::Vertex_Buffer;
				F.ElementCount = (uint32_t)I.Elements.size();
				F.Elements = (void*)I.Elements.data();
				F.ElementWidth = sizeof(Compute::SkinVertex);

				auto NewVertexBuffer = CreateElementBuffer(F);
				if (!NewVertexBuffer)
					return NewVertexBuffer.Error();

				F = ElementBuffer::Desc();
				F.AccessFlags = I.AccessFlags;
				F.Usage = I.Usage;
				F.BindFlags = ResourceBind::Index_Buffer;
				F.ElementCount = (uint32_t)I.Indices.size();
				F.ElementWidth = sizeof(int);
				F.Elements = (void*)I.Indices.data();

				auto NewIndexBuffer = CreateElementBuffer(F);
				if (!NewIndexBuffer)
				{
					Core::Memory::Release(*NewVertexBuffer);
					return NewIndexBuffer.Error();
				}

				OGLSkinMeshBuffer* Result = new OGLSkinMeshBuffer(I);
				Result->VertexBuffer = *NewVertexBuffer;
				Result->IndexBuffer = *NewIndexBuffer;
				return Result;
			}
			ExpectsGraphics<SkinMeshBuffer*> OGLDevice::CreateSkinMeshBuffer(ElementBuffer* VertexBuffer, ElementBuffer* IndexBuffer)
			{
				VI_ASSERT(VertexBuffer != nullptr, "vertex buffer should be set");
				VI_ASSERT(IndexBuffer != nullptr, "index buffer should be set");
				OGLSkinMeshBuffer* Result = new OGLSkinMeshBuffer(OGLSkinMeshBuffer::Desc());
				Result->VertexBuffer = VertexBuffer;
				Result->IndexBuffer = IndexBuffer;
				return Result;
			}
			ExpectsGraphics<InstanceBuffer*> OGLDevice::CreateInstanceBuffer(const InstanceBuffer::Desc& I)
			{
				ElementBuffer::Desc F = ElementBuffer::Desc();
				F.AccessFlags = CPUAccess::Write;
				F.MiscFlags = ResourceMisc::Buffer_Structured;
				F.Usage = ResourceUsage::Dynamic;
				F.BindFlags = ResourceBind::Shader_Input;
				F.ElementCount = I.ElementLimit;
				F.ElementWidth = I.ElementWidth;
				F.StructureByteStride = F.ElementWidth;

				auto NewBuffer = CreateElementBuffer(F);
				if (!NewBuffer)
					return NewBuffer.Error();

				OGLInstanceBuffer* Result = new OGLInstanceBuffer(I);
				Result->Elements = *NewBuffer;
				return Result;
			}
			ExpectsGraphics<Texture2D*> OGLDevice::CreateTexture2D()
			{
				return new OGLTexture2D();
			}
			ExpectsGraphics<Texture2D*> OGLDevice::CreateTexture2D(const Texture2D::Desc& I)
			{
				OGLTexture2D* Result = new OGLTexture2D(I);
				glGenTextures(1, &Result->Resource);
				glBindTexture(GL_TEXTURE_2D, Result->Resource);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_REPEAT);

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

				GLint SizeFormat = OGLDevice::GetSizedFormat(Result->FormatMode);
				GLint BaseFormat = OGLDevice::GetBaseFormat(Result->FormatMode);
				Result->Format = SizeFormat;

				glTexImage2D(GL_TEXTURE_2D, 0, SizeFormat, Result->Width, Result->Height, 0, BaseFormat, GL_UNSIGNED_BYTE, I.Data);
				if (Result->MipLevels != 0)
					glGenerateMipmap(GL_TEXTURE_2D);

				glBindTexture(GL_TEXTURE_2D, GL_NONE);
				return Result;
			}
			ExpectsGraphics<Texture3D*> OGLDevice::CreateTexture3D()
			{
				return new OGLTexture3D();
			}
			ExpectsGraphics<Texture3D*> OGLDevice::CreateTexture3D(const Texture3D::Desc& I)
			{
				OGLTexture3D* Result = new OGLTexture3D();
				glGenTextures(1, &Result->Resource);
				glBindTexture(GL_TEXTURE_3D, Result->Resource);
				glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
				glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
				glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);

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

				GLint SizeFormat = OGLDevice::GetSizedFormat(I.FormatMode);
				GLint BaseFormat = OGLDevice::GetBaseFormat(I.FormatMode);
				Result->FormatMode = I.FormatMode;
				Result->Format = SizeFormat;
				Result->MipLevels = I.MipLevels;
				Result->Width = I.Width;
				Result->Height = I.Height;
				Result->Depth = I.Depth;

				glTexImage3D(GL_TEXTURE_3D, 0, SizeFormat, Result->Width, Result->Height, Result->Depth, 0, BaseFormat, GL_UNSIGNED_BYTE, nullptr);
				if (Result->MipLevels != 0)
					glGenerateMipmap(GL_TEXTURE_3D);
				glBindTexture(GL_TEXTURE_3D, GL_NONE);

				return Result;
			}
			ExpectsGraphics<TextureCube*> OGLDevice::CreateTextureCube()
			{
				return new OGLTextureCube();
			}
			ExpectsGraphics<TextureCube*> OGLDevice::CreateTextureCube(const TextureCube::Desc& I)
			{
				OGLTextureCube* Result = new OGLTextureCube(I);
				glGenTextures(1, &Result->Resource);
				glBindTexture(GL_TEXTURE_CUBE_MAP, Result->Resource);
				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

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

				GLint SizeFormat = OGLDevice::GetSizedFormat(I.FormatMode);
				GLint BaseFormat = OGLDevice::GetBaseFormat(I.FormatMode);
				Result->Format = SizeFormat;

				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, SizeFormat, Result->Width, Result->Height, 0, BaseFormat, GL_UNSIGNED_BYTE, nullptr);
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, SizeFormat, Result->Width, Result->Height, 0, BaseFormat, GL_UNSIGNED_BYTE, nullptr);
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, SizeFormat, Result->Width, Result->Height, 0, BaseFormat, GL_UNSIGNED_BYTE, nullptr);
				glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, SizeFormat, Result->Width, Result->Height, 0, BaseFormat, GL_UNSIGNED_BYTE, nullptr);
				glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, SizeFormat, Result->Width, Result->Height, 0, BaseFormat, GL_UNSIGNED_BYTE, nullptr);
				glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, SizeFormat, Result->Width, Result->Height, 0, BaseFormat, GL_UNSIGNED_BYTE, nullptr);

				if (Result->MipLevels != 0)
					glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

				glBindTexture(GL_TEXTURE_CUBE_MAP, GL_NONE);
				return Result;
			}
			ExpectsGraphics<TextureCube*> OGLDevice::CreateTextureCube(Texture2D* Resource[6])
			{
				void* Resources[6];
				for (uint32_t i = 0; i < 6; i++)
				{
					VI_ASSERT(Resource[i] != nullptr, "face #%i should be set", (int)i);
					VI_ASSERT(!((OGLTexture2D*)Resource[i])->Backbuffer, "resource 2d should not be back buffer texture");
					Resources[i] = (void*)(OGLTexture2D*)Resource[i];
				}

				return CreateTextureCubeInternal(Resources);
			}
			ExpectsGraphics<TextureCube*> OGLDevice::CreateTextureCube(Texture2D* Resource)
			{
				VI_ASSERT(Resource != nullptr, "resource should be set");
				VI_ASSERT(!((OGLTexture2D*)Resource)->Backbuffer, "resource 2d should not be back buffer texture");

				OGLTexture2D* IResource = (OGLTexture2D*)Resource;
				uint32_t Width = IResource->Width / 4;
				uint32_t Height = Width;
				uint32_t MipLevels = GetMipLevel(Width, Height);

				if (IResource->Width % 4 != 0 || IResource->Height % 3 != 0)
					return GraphicsException("create texture cube: width / height is invalid");

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

				GLint SizeFormat = OGLDevice::GetSizedFormat(Result->FormatMode);
				GLint BaseFormat = OGLDevice::GetBaseFormat(Result->FormatMode);
				GLsizei Size = sizeof(GLubyte) * Width * Height;
				GLubyte* Pixels = Core::Memory::Allocate<GLubyte>(Size);
				Result->Format = SizeFormat;
				Result->FormatMode = IResource->FormatMode;
				Result->Width = IResource->Width;
				Result->Height = IResource->Height;
				Result->MipLevels = IResource->MipLevels;

				glBindTexture(GL_TEXTURE_2D, IResource->Resource);
				glBindTexture(GL_TEXTURE_CUBE_MAP, Result->Resource);

				glGetTextureSubImage(GL_TEXTURE_2D, 0, Width * 2, Height, 0, Width, Height, 0, BaseFormat, GL_UNSIGNED_BYTE, Size, Pixels);
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, IResource->MipLevels, SizeFormat, IResource->Width, IResource->Height, 0, BaseFormat, GL_UNSIGNED_BYTE, Pixels);

				glGetTextureSubImage(GL_TEXTURE_2D, 0, Width, Height * 2, 0, Width, Height, 0, BaseFormat, GL_UNSIGNED_BYTE, Size, Pixels);
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, IResource->MipLevels, SizeFormat, IResource->Width, IResource->Height, 0, BaseFormat, GL_UNSIGNED_BYTE, Pixels);

				glGetTextureSubImage(GL_TEXTURE_2D, 0, Width * 4, Height, 0, Width, Height, 0, BaseFormat, GL_UNSIGNED_BYTE, Size, Pixels);
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, IResource->MipLevels, SizeFormat, IResource->Width, IResource->Height, 0, BaseFormat, GL_UNSIGNED_BYTE, Pixels);

				glGetTextureSubImage(GL_TEXTURE_2D, 0, 0, Height, 0, Width, Height, 0, BaseFormat, GL_UNSIGNED_BYTE, Size, Pixels);
				glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, IResource->MipLevels, SizeFormat, IResource->Width, IResource->Height, 0, BaseFormat, GL_UNSIGNED_BYTE, Pixels);

				glGetTextureSubImage(GL_TEXTURE_2D, 0, Width, 0, 0, Width, Height, 0, BaseFormat, GL_UNSIGNED_BYTE, Size, Pixels);
				glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, IResource->MipLevels, SizeFormat, IResource->Width, IResource->Height, 0, BaseFormat, GL_UNSIGNED_BYTE, Pixels);

				glGetTextureSubImage(GL_TEXTURE_2D, 0, Width, Height, 0, Width, Height, 0, BaseFormat, GL_UNSIGNED_BYTE, Size, Pixels);
				glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, IResource->MipLevels, SizeFormat, IResource->Width, IResource->Height, 0, BaseFormat, GL_UNSIGNED_BYTE, Pixels);

				Core::Memory::Deallocate(Pixels);
				if (IResource->MipLevels != 0)
					glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

				glBindTexture(GL_TEXTURE_2D, GL_NONE);
				glBindTexture(GL_TEXTURE_CUBE_MAP, GL_NONE);
				return Result;
			}
			ExpectsGraphics<TextureCube*> OGLDevice::CreateTextureCubeInternal(void* Basis[6])
			{
				VI_ASSERT(Basis[0] && Basis[1] && Basis[2] && Basis[3] && Basis[4] && Basis[5], "all 6 faces should be set");
				OGLTexture2D* Resources[6];
				for (uint32_t i = 0; i < 6; i++)
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

				GLint SizeFormat = OGLDevice::GetSizedFormat(Result->FormatMode);
				GLint BaseFormat = OGLDevice::GetBaseFormat(Result->FormatMode);
				GLubyte* Pixels = Core::Memory::Allocate<GLubyte>(sizeof(GLubyte) * Resources[0]->Width * Resources[0]->Height);
				Result->Format = SizeFormat;
				Result->FormatMode = Resources[0]->FormatMode;
				Result->Width = Resources[0]->Width;
				Result->Height = Resources[0]->Height;
				Result->MipLevels = Resources[0]->MipLevels;

				for (uint32_t i = 0; i < 6; i++)
				{
					OGLTexture2D* Ref = Resources[i];
					glBindTexture(GL_TEXTURE_2D, Ref->Resource);
					glGetTexImage(GL_TEXTURE_2D, 0, BaseFormat, GL_UNSIGNED_BYTE, Pixels);
					glBindTexture(GL_TEXTURE_CUBE_MAP, Result->Resource);
					glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, Ref->MipLevels, SizeFormat, Ref->Width, Ref->Height, 0, BaseFormat, GL_UNSIGNED_BYTE, Pixels);
				}

				Core::Memory::Deallocate(Pixels);
				if (Resources[0]->MipLevels != 0)
					glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

				glBindTexture(GL_TEXTURE_2D, GL_NONE);
				glBindTexture(GL_TEXTURE_CUBE_MAP, GL_NONE);
				return Result;
			}
			ExpectsGraphics<DepthTarget2D*> OGLDevice::CreateDepthTarget2D(const DepthTarget2D::Desc& I)
			{
				Core::UPtr<OGLDepthTarget2D> Result = new OGLDepthTarget2D(I);
				glGenTextures(1, &Result->DepthTexture);
				glBindTexture(GL_TEXTURE_2D, Result->DepthTexture);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_GREATER);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
				glTexStorage2D(GL_TEXTURE_2D, 1, GetSizedFormat(I.FormatMode), I.Width, I.Height);

				auto NewTexture = CreateTexture2D();
				if (!NewTexture)
					return NewTexture.Error();

				OGLTexture2D* DepthStencil = (OGLTexture2D*)*NewTexture;
				DepthStencil->FormatMode = I.FormatMode;
				DepthStencil->Format = GetSizedFormat(I.FormatMode);
				DepthStencil->MipLevels = 0;
				DepthStencil->Width = I.Width;
				DepthStencil->Height = I.Height;
				DepthStencil->Resource = Result->DepthTexture;
				Result->HasStencilBuffer = (I.FormatMode != Format::D16_Unorm && I.FormatMode != Format::D32_Float);
				Result->Resource = DepthStencil;

				glGenFramebuffers(1, &Result->FrameBuffer);
				glBindFramebuffer(GL_FRAMEBUFFER, Result->FrameBuffer);
				if (Result->HasStencilBuffer)
					glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, Result->DepthTexture, 0);
				else
					glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, Result->DepthTexture, 0);
				glBindFramebuffer(GL_FRAMEBUFFER, GL_NONE);
				glBindTexture(GL_TEXTURE_2D, GL_NONE);

				Result->Viewarea.Width = (float)I.Width;
				Result->Viewarea.Height = (float)I.Height;
				Result->Viewarea.TopLeftX = 0.0f;
				Result->Viewarea.TopLeftY = 0.0f;
				Result->Viewarea.MinDepth = 0.0f;
				Result->Viewarea.MaxDepth = 1.0f;
				return Result.Reset();
			}
			ExpectsGraphics<DepthTargetCube*> OGLDevice::CreateDepthTargetCube(const DepthTargetCube::Desc& I)
			{
				Core::UPtr<OGLDepthTargetCube> Result = new OGLDepthTargetCube(I);
				bool NoStencil = (I.FormatMode == Format::D16_Unorm || I.FormatMode == Format::D32_Float);
				GLenum SizeFormat = GetSizedFormat(I.FormatMode);
				GLenum ComponentFormat = (NoStencil ? GL_DEPTH_COMPONENT : GL_DEPTH_STENCIL);

				glGenTextures(1, &Result->DepthTexture);
				glBindTexture(GL_TEXTURE_CUBE_MAP, Result->DepthTexture);
				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_COMPARE_FUNC, GL_GREATER);
				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_BASE_LEVEL, 0);
				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LEVEL, 0);
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + 0, 0, SizeFormat, I.Size, I.Size, 0, ComponentFormat, GL_UNSIGNED_BYTE, 0);
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + 1, 0, SizeFormat, I.Size, I.Size, 0, ComponentFormat, GL_UNSIGNED_BYTE, 0);
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + 2, 0, SizeFormat, I.Size, I.Size, 0, ComponentFormat, GL_UNSIGNED_BYTE, 0);
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + 3, 0, SizeFormat, I.Size, I.Size, 0, ComponentFormat, GL_UNSIGNED_BYTE, 0);
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + 4, 0, SizeFormat, I.Size, I.Size, 0, ComponentFormat, GL_UNSIGNED_BYTE, 0);
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + 5, 0, SizeFormat, I.Size, I.Size, 0, ComponentFormat, GL_UNSIGNED_BYTE, 0);

				auto NewTexture = CreateTextureCube();
				if (!NewTexture)
					return NewTexture.Error();

				OGLTextureCube* DepthStencil = (OGLTextureCube*)*NewTexture;
				DepthStencil->FormatMode = I.FormatMode;
				DepthStencil->Format = GetSizedFormat(I.FormatMode);
				DepthStencil->MipLevels = 0;
				DepthStencil->Width = I.Size;
				DepthStencil->Height = I.Size;
				DepthStencil->Resource = Result->DepthTexture;
				Result->HasStencilBuffer = (I.FormatMode != Format::D16_Unorm && I.FormatMode != Format::D32_Float);
				Result->Resource = DepthStencil;

				glGenFramebuffers(1, &Result->FrameBuffer);
				glBindFramebuffer(GL_FRAMEBUFFER, Result->FrameBuffer);
				if (Result->HasStencilBuffer)
					glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, Result->DepthTexture, 0);
				else
					glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, Result->DepthTexture, 0);
				glBindFramebuffer(GL_FRAMEBUFFER, GL_NONE);
				glBindTexture(GL_TEXTURE_CUBE_MAP, GL_NONE);

				Result->Viewarea.Width = (float)I.Size;
				Result->Viewarea.Height = (float)I.Size;
				Result->Viewarea.TopLeftX = 0.0f;
				Result->Viewarea.TopLeftY = 0.0f;
				Result->Viewarea.MinDepth = 0.0f;
				Result->Viewarea.MaxDepth = 1.0f;
				return Result.Reset();
			}
			ExpectsGraphics<RenderTarget2D*> OGLDevice::CreateRenderTarget2D(const RenderTarget2D::Desc& I)
			{
				Core::UPtr<OGLRenderTarget2D> Result = new OGLRenderTarget2D(I);
				if (!I.RenderSurface)
				{
					GLenum Format = OGLDevice::GetSizedFormat(I.FormatMode);
					glGenTextures(1, &Result->FrameBuffer.Texture[0]);
					glBindTexture(GL_TEXTURE_2D, Result->FrameBuffer.Texture[0]);
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
					glTexStorage2D(GL_TEXTURE_2D, I.MipLevels, Format, I.Width, I.Height);

					auto NewTexture = CreateTexture2D();
					if (!NewTexture)
						return NewTexture.Error();

					OGLTexture2D* Frontbuffer = (OGLTexture2D*)*NewTexture;
					Frontbuffer->FormatMode = I.FormatMode;
					Frontbuffer->Format = GetSizedFormat(I.FormatMode);
					Frontbuffer->MipLevels = I.MipLevels;
					Frontbuffer->Width = I.Width;
					Frontbuffer->Height = I.Height;
					Frontbuffer->Resource = Result->FrameBuffer.Texture[0];
					Result->Resource = Frontbuffer;

					glGenTextures(1, &Result->DepthTexture);
					glBindTexture(GL_TEXTURE_2D, Result->DepthTexture);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_GREATER);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
					glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH24_STENCIL8, I.Width, I.Height);

					NewTexture = CreateTexture2D();
					if (!NewTexture)
						return NewTexture.Error();

					OGLTexture2D* DepthStencil = (OGLTexture2D*)*NewTexture;
					DepthStencil->FormatMode = Format::D24_Unorm_S8_Uint;
					DepthStencil->Format = GL_DEPTH24_STENCIL8;
					DepthStencil->MipLevels = 0;
					DepthStencil->Width = I.Width;
					DepthStencil->Height = I.Height;
					DepthStencil->Resource = Result->DepthTexture;
					Result->DepthStencil = DepthStencil;

					glGenFramebuffers(1, &Result->FrameBuffer.Buffer);
					glBindFramebuffer(GL_FRAMEBUFFER, Result->FrameBuffer.Buffer);
					glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, Result->FrameBuffer.Texture[0], 0);
					glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, Result->DepthTexture, 0);
					glBindFramebuffer(GL_FRAMEBUFFER, GL_NONE);
					glBindTexture(GL_TEXTURE_2D, GL_NONE);
				}
				else if (I.RenderSurface == (void*)this)
				{
					auto NewTexture = CreateTexture2D();
					if (!NewTexture)
						return NewTexture.Error();

					OGLTexture2D* Backbuffer = (OGLTexture2D*)*NewTexture;
					Backbuffer->FormatMode = I.FormatMode;
					Backbuffer->Format = GetSizedFormat(I.FormatMode);
					Backbuffer->MipLevels = I.MipLevels;
					Backbuffer->Width = I.Width;
					Backbuffer->Height = I.Height;
					Backbuffer->Backbuffer = true;

					Result->FrameBuffer.Backbuffer = true;
					Result->Resource = Backbuffer;
				}

				Result->Viewarea.Width = (float)I.Width;
				Result->Viewarea.Height = (float)I.Height;
				Result->Viewarea.TopLeftX = 0.0f;
				Result->Viewarea.TopLeftY = 0.0f;
				Result->Viewarea.MinDepth = 0.0f;
				Result->Viewarea.MaxDepth = 1.0f;
				return Result.Reset();
			}
			ExpectsGraphics<MultiRenderTarget2D*> OGLDevice::CreateMultiRenderTarget2D(const MultiRenderTarget2D::Desc& I)
			{
				Core::UPtr<OGLMultiRenderTarget2D> Result = new OGLMultiRenderTarget2D(I);
				glGenFramebuffers(1, &Result->FrameBuffer.Buffer);
				glBindFramebuffer(GL_FRAMEBUFFER, Result->FrameBuffer.Buffer);

				for (uint32_t i = 0; i < (uint32_t)I.Target; i++)
				{
					GLenum Format = OGLDevice::GetSizedFormat(I.FormatMode[i]);
					glGenTextures(1, &Result->FrameBuffer.Texture[i]);
					glBindTexture(GL_TEXTURE_2D, Result->FrameBuffer.Texture[i]);
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
					glTexStorage2D(GL_TEXTURE_2D, I.MipLevels, Format, I.Width, I.Height);

					auto NewTexture = CreateTexture2D();
					if (!NewTexture)
						return NewTexture.Error();

					OGLTexture2D* Frontbuffer = (OGLTexture2D*)*NewTexture;
					Frontbuffer->FormatMode = I.FormatMode[i];
					Frontbuffer->Format = Format;
					Frontbuffer->MipLevels = I.MipLevels;
					Frontbuffer->Width = I.Width;
					Frontbuffer->Height = I.Height;
					Frontbuffer->Resource = Result->FrameBuffer.Texture[i];
					Result->Resource[i] = Frontbuffer;

					Result->FrameBuffer.Format[i] = GL_COLOR_ATTACHMENT0 + i;
					glFramebufferTexture(GL_FRAMEBUFFER, Result->FrameBuffer.Format[i], Result->FrameBuffer.Texture[i], 0);
				}

				glGenTextures(1, &Result->DepthTexture);
				glBindTexture(GL_TEXTURE_2D, Result->DepthTexture);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_GREATER);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
				glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH24_STENCIL8, I.Width, I.Height);

				auto NewTexture = CreateTexture2D();
				if (!NewTexture)
					return NewTexture.Error();

				OGLTexture2D* DepthStencil = (OGLTexture2D*)*NewTexture;
				DepthStencil->FormatMode = Format::D24_Unorm_S8_Uint;
				DepthStencil->Format = GL_DEPTH24_STENCIL8;
				DepthStencil->MipLevels = I.MipLevels;
				DepthStencil->Width = I.Width;
				DepthStencil->Height = I.Height;
				DepthStencil->Resource = Result->DepthTexture;
				Result->DepthStencil = DepthStencil;

				glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, Result->DepthTexture, 0);
				glBindFramebuffer(GL_FRAMEBUFFER, GL_NONE);
				glBindTexture(GL_TEXTURE_2D, GL_NONE);

				Result->Viewarea.Width = (float)I.Width;
				Result->Viewarea.Height = (float)I.Height;
				Result->Viewarea.TopLeftX = 0.0f;
				Result->Viewarea.TopLeftY = 0.0f;
				Result->Viewarea.MinDepth = 0.0f;
				Result->Viewarea.MaxDepth = 1.0f;
				return Result.Reset();
			}
			ExpectsGraphics<RenderTargetCube*> OGLDevice::CreateRenderTargetCube(const RenderTargetCube::Desc& I)
			{
				Core::UPtr<OGLRenderTargetCube> Result = new OGLRenderTargetCube(I);
				GLint SizeFormat = OGLDevice::GetSizedFormat(I.FormatMode);
				GLint BaseFormat = OGLDevice::GetBaseFormat(I.FormatMode);
				glGenTextures(1, &Result->FrameBuffer.Texture[0]);
				glBindTexture(GL_TEXTURE_CUBE_MAP, Result->FrameBuffer.Texture[0]);
				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
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
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + 0, 0, SizeFormat, I.Size, I.Size, 0, BaseFormat, GL_UNSIGNED_BYTE, 0);
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + 1, 0, SizeFormat, I.Size, I.Size, 0, BaseFormat, GL_UNSIGNED_BYTE, 0);
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + 2, 0, SizeFormat, I.Size, I.Size, 0, BaseFormat, GL_UNSIGNED_BYTE, 0);
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + 3, 0, SizeFormat, I.Size, I.Size, 0, BaseFormat, GL_UNSIGNED_BYTE, 0);
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + 4, 0, SizeFormat, I.Size, I.Size, 0, BaseFormat, GL_UNSIGNED_BYTE, 0);
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + 5, 0, SizeFormat, I.Size, I.Size, 0, BaseFormat, GL_UNSIGNED_BYTE, 0);

				auto NewTexture = CreateTextureCube();
				if (!NewTexture)
					return NewTexture.Error();

				OGLTextureCube* Base = (OGLTextureCube*)*NewTexture;
				Base->FormatMode = I.FormatMode;
				Base->Format = SizeFormat;
				Base->MipLevels = I.MipLevels;
				Base->Resource = Result->FrameBuffer.Texture[0];
				Base->Width = I.Size;
				Base->Height = I.Size;
				Result->Resource = Base;

				glGenTextures(1, &Result->DepthTexture);
				glBindTexture(GL_TEXTURE_CUBE_MAP, Result->DepthTexture);
				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_COMPARE_FUNC, GL_GREATER);
				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_BASE_LEVEL, 0);
				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LEVEL, 0);
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + 0, 0, GL_DEPTH24_STENCIL8, I.Size, I.Size, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, 0);
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + 1, 0, GL_DEPTH24_STENCIL8, I.Size, I.Size, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, 0);
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + 2, 0, GL_DEPTH24_STENCIL8, I.Size, I.Size, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, 0);
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + 3, 0, GL_DEPTH24_STENCIL8, I.Size, I.Size, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, 0);
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + 4, 0, GL_DEPTH24_STENCIL8, I.Size, I.Size, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, 0);
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + 5, 0, GL_DEPTH24_STENCIL8, I.Size, I.Size, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, 0);

				auto NewSubtexture = CreateTexture2D();
				if (!NewSubtexture)
					return NewSubtexture.Error();

				OGLTexture2D* DepthStencil = (OGLTexture2D*)*NewSubtexture;
				DepthStencil->FormatMode = Format::D24_Unorm_S8_Uint;
				DepthStencil->Format = GL_DEPTH24_STENCIL8;
				DepthStencil->MipLevels = 0;
				DepthStencil->Resource = Result->DepthTexture;
				DepthStencil->Width = I.Size;
				DepthStencil->Height = I.Size;
				Result->DepthStencil = DepthStencil;

				glGenFramebuffers(1, &Result->FrameBuffer.Buffer);
				glBindFramebuffer(GL_FRAMEBUFFER, Result->FrameBuffer.Buffer);
				glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, Result->FrameBuffer.Texture[0], 0);
				glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, Result->DepthTexture, 0);
				glBindFramebuffer(GL_FRAMEBUFFER, GL_NONE);
				glBindTexture(GL_TEXTURE_CUBE_MAP, GL_NONE);

				Result->Viewarea.Width = (float)I.Size;
				Result->Viewarea.Height = (float)I.Size;
				Result->Viewarea.TopLeftX = 0.0f;
				Result->Viewarea.TopLeftY = 0.0f;
				Result->Viewarea.MinDepth = 0.0f;
				Result->Viewarea.MaxDepth = 1.0f;
				return Result.Reset();
			}
			ExpectsGraphics<MultiRenderTargetCube*> OGLDevice::CreateMultiRenderTargetCube(const MultiRenderTargetCube::Desc& I)
			{
				Core::UPtr<OGLMultiRenderTargetCube> Result = new OGLMultiRenderTargetCube(I);
				glGenFramebuffers(1, &Result->FrameBuffer.Buffer);
				glBindFramebuffer(GL_FRAMEBUFFER, Result->FrameBuffer.Buffer);

				for (uint32_t i = 0; i < (uint32_t)I.Target; i++)
				{
					GLint SizeFormat = OGLDevice::GetSizedFormat(I.FormatMode[i]);
					GLint BaseFormat = OGLDevice::GetBaseFormat(I.FormatMode[i]);
					glGenTextures(1, &Result->FrameBuffer.Texture[i]);
					glBindTexture(GL_TEXTURE_CUBE_MAP, Result->FrameBuffer.Texture[i]);
					glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
					glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
					glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
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
					glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + 0, 0, SizeFormat, I.Size, I.Size, 0, BaseFormat, GL_UNSIGNED_BYTE, 0);
					glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + 1, 0, SizeFormat, I.Size, I.Size, 0, BaseFormat, GL_UNSIGNED_BYTE, 0);
					glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + 2, 0, SizeFormat, I.Size, I.Size, 0, BaseFormat, GL_UNSIGNED_BYTE, 0);
					glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + 3, 0, SizeFormat, I.Size, I.Size, 0, BaseFormat, GL_UNSIGNED_BYTE, 0);
					glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + 4, 0, SizeFormat, I.Size, I.Size, 0, BaseFormat, GL_UNSIGNED_BYTE, 0);
					glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + 5, 0, SizeFormat, I.Size, I.Size, 0, BaseFormat, GL_UNSIGNED_BYTE, 0);

					auto NewTexture = CreateTextureCube();
					if (!NewTexture)
						return NewTexture.Error();

					OGLTextureCube* Frontbuffer = (OGLTextureCube*)*NewTexture;
					Frontbuffer->FormatMode = I.FormatMode[i];
					Frontbuffer->Format = SizeFormat;
					Frontbuffer->MipLevels = I.MipLevels;
					Frontbuffer->Width = I.Size;
					Frontbuffer->Height = I.Size;
					Frontbuffer->Resource = Result->FrameBuffer.Texture[i];
					Result->Resource[i] = Frontbuffer;

					Result->FrameBuffer.Format[i] = GL_COLOR_ATTACHMENT0 + i;
					glFramebufferTexture(GL_FRAMEBUFFER, Result->FrameBuffer.Format[i], Result->FrameBuffer.Texture[i], 0);
				}

				glGenTextures(1, &Result->DepthTexture);
				glBindTexture(GL_TEXTURE_CUBE_MAP, Result->DepthTexture);
				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_COMPARE_FUNC, GL_GREATER);
				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_BASE_LEVEL, 0);
				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LEVEL, 0);
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + 0, 0, GL_DEPTH24_STENCIL8, I.Size, I.Size, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, 0);
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + 1, 0, GL_DEPTH24_STENCIL8, I.Size, I.Size, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, 0);
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + 2, 0, GL_DEPTH24_STENCIL8, I.Size, I.Size, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, 0);
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + 3, 0, GL_DEPTH24_STENCIL8, I.Size, I.Size, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, 0);
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + 4, 0, GL_DEPTH24_STENCIL8, I.Size, I.Size, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, 0);
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + 5, 0, GL_DEPTH24_STENCIL8, I.Size, I.Size, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, 0);

				auto NewTexture = CreateTexture2D();
				if (!NewTexture)
					return NewTexture.Error();

				OGLTexture2D* DepthStencil = (OGLTexture2D*)*NewTexture;
				DepthStencil->FormatMode = Format::D24_Unorm_S8_Uint;
				DepthStencil->Format = GL_DEPTH24_STENCIL8;
				DepthStencil->MipLevels = 0;
				DepthStencil->Width = I.Size;
				DepthStencil->Height = I.Size;
				DepthStencil->Resource = Result->DepthTexture;
				Result->DepthStencil = DepthStencil;

				glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, Result->DepthTexture, 0);
				glBindFramebuffer(GL_FRAMEBUFFER, GL_NONE);
				glBindTexture(GL_TEXTURE_CUBE_MAP, GL_NONE);

				Result->Viewarea.Width = (float)I.Size;
				Result->Viewarea.Height = (float)I.Size;
				Result->Viewarea.TopLeftX = 0.0f;
				Result->Viewarea.TopLeftY = 0.0f;
				Result->Viewarea.MinDepth = 0.0f;
				Result->Viewarea.MaxDepth = 1.0f;
				return Result.Reset();
			}
			ExpectsGraphics<Cubemap*> OGLDevice::CreateCubemap(const Cubemap::Desc& I)
			{
				return new OGLCubemap(I);
			}
			ExpectsGraphics<Query*> OGLDevice::CreateQuery(const Query::Desc& I)
			{
				OGLQuery* Result = new OGLQuery();
				Result->Predicate = I.Predicate;
				glGenQueries(1, &Result->Async);

				return Result;
			}
			PrimitiveTopology OGLDevice::GetPrimitiveTopology() const
			{
				return Register.Primitive;
			}
			ShaderModel OGLDevice::GetSupportedShaderModel() const
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
			void* OGLDevice::GetDevice() const
			{
				return Context;
			}
			void* OGLDevice::GetContext() const
			{
				return Context;
			}
			bool OGLDevice::IsValid() const
			{
				return Context != nullptr;
			}
			void OGLDevice::SetVSyncMode(VSync Mode)
			{
				GraphicsDevice::SetVSyncMode(Mode);
				switch (VSyncMode)
				{
					case VSync::Off:
						Video::GLEW::SetSwapInterval(0);
						break;
					case VSync::Frequency_X1:
						Video::GLEW::SetSwapInterval(1);
						break;
					case VSync::Frequency_X2:
					case VSync::Frequency_X3:
					case VSync::Frequency_X4:
						if (!Video::GLEW::SetSwapInterval(-1))
							Video::GLEW::SetSwapInterval(1);
						break;
				}
			}
			const std::string_view& OGLDevice::GetShaderVersion()
			{
				return ShaderVersion;
			}
			ExpectsGraphics<void> OGLDevice::CopyConstantBuffer(GLuint Buffer, void* Data, size_t Size)
			{
				VI_ASSERT(Data != nullptr, "buffer should not be empty");
				if (!Size)
					return GraphicsException("copy constant buffer: zero size");

				glBindBuffer(GL_UNIFORM_BUFFER, Buffer);
				glBufferSubData(GL_UNIFORM_BUFFER, 0, Size, Data);
				glBindBuffer(GL_UNIFORM_BUFFER, GL_NONE);
				return Core::Expectation::Met;
			}
			ExpectsGraphics<void> OGLDevice::CreateConstantBuffer(GLuint* Buffer, size_t Size)
			{
				VI_ASSERT(Buffer != nullptr, "buffer should be set");
				glGenBuffers(1, Buffer);
				glBindBuffer(GL_UNIFORM_BUFFER, *Buffer);
				glBufferData(GL_UNIFORM_BUFFER, Size, nullptr, GL_DYNAMIC_DRAW);
				glBindBuffer(GL_UNIFORM_BUFFER, GL_NONE);
				if (!glIsBuffer(*Buffer))
					return GetException("create constant buffer");

				return Core::Expectation::Met;
			}
			ExpectsGraphics<void> OGLDevice::CreateDirectBuffer(size_t Size)
			{
				MaxElements = Size + 1;
				SetInputLayout(nullptr);
				SetVertexBuffers(nullptr, 0);
				SetIndexBuffer(nullptr, Format::Unknown);

				GLint StatusCode;
				if (!Immediate.Sampler)
				{
					OGLSamplerState* State = (OGLSamplerState*)GetSamplerState("a16_fa_wrap");
					if (State != nullptr)
						Immediate.Sampler = State->Resource;
				}

				if (Immediate.VertexShader == GL_NONE)
				{
					static const char* VertexShaderCode = OGL_INLINE(
						layout(location = 0) uniform mat4 Transform;

						layout(location = 0) in vec3 iPosition;
						layout(location = 1) in vec2 iTexCoord;
						layout(location = 2) in vec4 iColor;

						out vec2 oTexCoord;
						out vec4 oColor;

						void main()
						{
							gl_Position = Transform * vec4(iPosition.xyz, 1.0);
							oTexCoord = iTexCoord;
							oColor = iColor;
						});

					Core::String Result = Core::String(ShaderVersion);
					Result.append(VertexShaderCode);

					const char* Subbuffer = Result.data();
					GLint BufferSize = (GLint)Result.size();
					Immediate.VertexShader = glCreateShader(GL_VERTEX_SHADER);
					glShaderSourceARB(Immediate.VertexShader, 1, &Subbuffer, &BufferSize);
					glCompileShaderARB(Immediate.VertexShader);
					glGetShaderiv(Immediate.VertexShader, GL_COMPILE_STATUS, &StatusCode);
					if (StatusCode == GL_FALSE)
					{
						glGetShaderiv(Immediate.VertexShader, GL_INFO_LOG_LENGTH, &BufferSize);
						char* Buffer = Core::Memory::Allocate<char>(sizeof(char) * (BufferSize + 1));
						glGetShaderInfoLog(Immediate.VertexShader, BufferSize, &BufferSize, Buffer);
						Core::String ErrorText(Buffer, (size_t)BufferSize);
						Core::Memory::Deallocate(Buffer);
						return GraphicsException(std::move(ErrorText));
					}
				}

				if (Immediate.PixelShader == GL_NONE)
				{
					static const char* PixelShaderCode = OGL_INLINE(
						layout(binding = 1) uniform sampler2D Diffuse;
						layout(location = 2) uniform vec4 Padding;

						in vec2 oTexCoord;
						in vec4 oColor;

						out vec4 oResult;

						void main()
						{
							if (Padding.z > 0)
								oResult = oColor * textureLod(Diffuse, oTexCoord + Padding.xy, 0.0) * Padding.w;
							else
								oResult = oColor * Padding.w;
						});

					Core::String Result = Core::String(ShaderVersion);
					Result.append(PixelShaderCode);

					const char* Subbuffer = Result.data();
					GLint BufferSize = (GLint)Result.size();
					Immediate.PixelShader = glCreateShader(GL_FRAGMENT_SHADER);
					glShaderSourceARB(Immediate.PixelShader, 1, &Subbuffer, &BufferSize);
					glCompileShaderARB(Immediate.PixelShader);
					glGetShaderiv(Immediate.PixelShader, GL_COMPILE_STATUS, &StatusCode);
					if (StatusCode == GL_FALSE)
					{
						glGetShaderiv(Immediate.PixelShader, GL_INFO_LOG_LENGTH, &BufferSize);
						char* Buffer = Core::Memory::Allocate<char>(sizeof(char) * (BufferSize + 1));
						glGetShaderInfoLog(Immediate.PixelShader, BufferSize, &BufferSize, Buffer);
						Core::String ErrorText(Buffer, (size_t)BufferSize);
						Core::Memory::Deallocate(Buffer);
						return GraphicsException(std::move(ErrorText));
					}
				}

				if (Immediate.Program == GL_NONE)
				{
					Immediate.Program = glCreateProgram();
					glAttachShader(Immediate.Program, Immediate.VertexShader);
					glAttachShader(Immediate.Program, Immediate.PixelShader);
					glLinkProgramARB(Immediate.Program);
					glGetProgramiv(Immediate.Program, GL_LINK_STATUS, &StatusCode);

					if (StatusCode == GL_FALSE)
					{
						GLint BufferSize = 0;
						glGetProgramiv(Immediate.Program, GL_INFO_LOG_LENGTH, &BufferSize);

						char* Buffer = Core::Memory::Allocate<char>(sizeof(char) * (BufferSize + 1));
						glGetProgramInfoLog(Immediate.Program, BufferSize, &BufferSize, Buffer);
						Core::String ErrorText(Buffer, (size_t)BufferSize);
						Core::Memory::Deallocate(Buffer);

						glDeleteProgram(Immediate.Program);
						Immediate.Program = GL_NONE;
						return GraphicsException(std::move(ErrorText));
					}
				}

				if (Immediate.VertexArray != GL_NONE)
					glDeleteVertexArrays(1, &Immediate.VertexArray);

				if (Immediate.VertexBuffer != GL_NONE)
					glDeleteBuffers(1, &Immediate.VertexBuffer);

				glGenBuffers(1, &Immediate.VertexBuffer);
				glGenVertexArrays(1, &Immediate.VertexArray);
				glBindVertexArray(Immediate.VertexArray);
				glBindBuffer(GL_ARRAY_BUFFER, Immediate.VertexBuffer);
				glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * (MaxElements + 1), Elements.empty() ? nullptr : &Elements[0], GL_DYNAMIC_DRAW);
				glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), OGL_OFFSET(0));
				glEnableVertexAttribArray(0);
				glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), OGL_OFFSET(sizeof(float) * 3));
				glEnableVertexAttribArray(1);
				glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), OGL_OFFSET(sizeof(float) * 5));
				glEnableVertexAttribArray(2);
				glBindVertexArray(0);

				SetVertexBuffers(nullptr, 0);
				return Core::Expectation::Met;
			}
			uint64_t OGLDevice::GetProgramHash()
			{
				static uint64_t Seed = Compute::Crypto::Random();
				Rehash<void*>(Seed, Register.Shaders[0]);
				Rehash<void*>(Seed, Register.Shaders[1]);
				Rehash<void*>(Seed, Register.Shaders[2]);
				Rehash<void*>(Seed, Register.Shaders[3]);
				Rehash<void*>(Seed, Register.Shaders[4]);
				Rehash<void*>(Seed, Register.Shaders[5]);
				return Seed;
			}
			Core::String OGLDevice::CompileState(GLuint Handle)
			{
				GLint Stat = 0, Size = 0;
				glGetShaderiv(Handle, GL_COMPILE_STATUS, &Stat);
				glGetShaderiv(Handle, GL_INFO_LOG_LENGTH, &Size);

				if ((GLboolean)Stat == GL_TRUE || !Size)
					return "";

				GLchar* Buffer = Core::Memory::Allocate<GLchar>(sizeof(GLchar) * Size);
				glGetShaderInfoLog(Handle, Size, NULL, Buffer);

				Core::String Result((char*)Buffer, Size);
				Core::Memory::Deallocate(Buffer);

				return Result;
			}
			bool OGLDevice::IsComparator(PixelFilter Value)
			{
				switch (Value)
				{
					case PixelFilter::Compare_Min_Mag_Mip_Point:
					case PixelFilter::Compare_Min_Mag_Point_Mip_Linear:
					case PixelFilter::Compare_Min_Point_Mag_Linear_Mip_Point:
					case PixelFilter::Compare_Min_Point_Mag_Mip_Linear:
					case PixelFilter::Compare_Min_Linear_Mag_Mip_Point:
					case PixelFilter::Compare_Min_Linear_Mag_Point_Mip_Linear:
					case PixelFilter::Compare_Min_Mag_Linear_Mip_Point:
					case PixelFilter::Compare_Min_Mag_Mip_Linear:
					case PixelFilter::Compare_Anistropic:
						return true;
					default:
						return false;
				}
			}
			GLenum OGLDevice::GetAccessControl(CPUAccess Access, ResourceUsage Usage)
			{
				switch (Usage)
				{
					case Vitex::Graphics::ResourceUsage::Default:
						switch (Access)
						{
							case Vitex::Graphics::CPUAccess::Read:
								return GL_STATIC_READ;
							case Vitex::Graphics::CPUAccess::None:
							case Vitex::Graphics::CPUAccess::Write:
							default:
								return GL_STATIC_DRAW;
						}
					case Vitex::Graphics::ResourceUsage::Immutable:
						return GL_STATIC_DRAW;
					case Vitex::Graphics::ResourceUsage::Dynamic:
						switch (Access)
						{
							case Vitex::Graphics::CPUAccess::Read:
								return GL_DYNAMIC_READ;
							case Vitex::Graphics::CPUAccess::None:
							case Vitex::Graphics::CPUAccess::Write:
							default:
								return GL_DYNAMIC_DRAW;
						}
					case Vitex::Graphics::ResourceUsage::Staging:
						return GL_DYNAMIC_READ;
					default:
						return GL_STATIC_DRAW;
				}
			}
			GLenum OGLDevice::GetBaseFormat(Format Value)
			{
				switch (Value)
				{
					case Format::R32G32B32A32_Float:
					case Format::R32G32B32A32_Uint:
					case Format::R32G32B32A32_Sint:
					case Format::R16G16B16A16_Float:
					case Format::R16G16B16A16_Unorm:
					case Format::R16G16B16A16_Uint:
					case Format::R16G16B16A16_Snorm:
					case Format::R16G16B16A16_Sint:
					case Format::R10G10B10A2_Unorm:
					case Format::R10G10B10A2_Uint:
					case Format::R8G8B8A8_Unorm:
					case Format::R8G8B8A8_Unorm_SRGB:
					case Format::R8G8B8A8_Uint:
					case Format::R8G8B8A8_Snorm:
					case Format::R8G8B8A8_Sint:
					case Format::R9G9B9E5_Share_Dexp:
					case Format::R8G8_B8G8_Unorm:
						return GL_RGBA;
					case Format::R32G32B32_Float:
					case Format::R32G32B32_Uint:
					case Format::R32G32B32_Sint:
					case Format::R11G11B10_Float:
						return GL_RGB;
					case Format::R16G16_Float:
					case Format::R16G16_Unorm:
					case Format::R16G16_Uint:
					case Format::R16G16_Snorm:
					case Format::R16G16_Sint:
					case Format::R32G32_Float:
					case Format::R32G32_Uint:
					case Format::R32G32_Sint:
					case Format::R8G8_Unorm:
					case Format::R8G8_Uint:
					case Format::R8G8_Snorm:
					case Format::R8G8_Sint:
						return GL_RG;
					case Format::D24_Unorm_S8_Uint:
						return GL_DEPTH_STENCIL;
					case Format::D32_Float:
					case Format::D16_Unorm:
						return GL_DEPTH_COMPONENT;
					case Format::R32_Float:
					case Format::R32_Uint:
					case Format::R32_Sint:
					case Format::R16_Float:
					case Format::R16_Unorm:
					case Format::R16_Uint:
					case Format::R16_Snorm:
					case Format::R16_Sint:
					case Format::R8_Unorm:
					case Format::R8_Uint:
					case Format::R8_Snorm:
					case Format::R8_Sint:
					case Format::A8_Unorm:
					case Format::R1_Unorm:
						return GL_RED;
					default:
						return GL_RGBA;
				}
			}
			GLenum OGLDevice::GetSizedFormat(Format Value)
			{
				switch (Value)
				{
					case Format::R32G32B32A32_Float:
						return GL_RGBA32F;
					case Format::R32G32B32A32_Uint:
						return GL_RGBA32UI;
					case Format::R32G32B32A32_Sint:
						return GL_RGBA32I;
					case Format::R32G32B32_Float:
						return GL_RGB32F;
					case Format::R32G32B32_Uint:
						return GL_RGB32UI;
					case Format::R32G32B32_Sint:
						return GL_RGB32I;
					case Format::R16G16B16A16_Float:
						return GL_RGBA16F;
					case Format::R16G16B16A16_Unorm:
						return GL_RGBA16;
					case Format::R16G16B16A16_Uint:
						return GL_RGBA16UI;
					case Format::R16G16B16A16_Snorm:
						return GL_RGBA16_SNORM;
					case Format::R16G16B16A16_Sint:
						return GL_RGBA16I;
					case Format::R32G32_Float:
						return GL_RG32F;
					case Format::R32G32_Uint:
						return GL_RG32UI;
					case Format::R32G32_Sint:
						return GL_RG32I;
					case Format::R10G10B10A2_Unorm:
						return GL_RGB10_A2;
					case Format::R10G10B10A2_Uint:
						return GL_RGB10_A2UI;
					case Format::R11G11B10_Float:
						return GL_R11F_G11F_B10F;
					case Format::R8G8B8A8_Unorm:
						return GL_RGBA8;
					case Format::R8G8B8A8_Unorm_SRGB:
						return GL_SRGB8_ALPHA8;
					case Format::R8G8B8A8_Uint:
						return GL_RGBA8UI;
					case Format::R8G8B8A8_Snorm:
						return GL_RGBA8I;
					case Format::R8G8B8A8_Sint:
						return GL_RGBA8I;
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
					case Format::D32_Float:
						return GL_DEPTH_COMPONENT32F;
					case Format::R32_Float:
						return GL_R32F;
					case Format::R32_Uint:
						return GL_R32UI;
					case Format::R32_Sint:
						return GL_R32I;
					case Format::D24_Unorm_S8_Uint:
						return GL_DEPTH24_STENCIL8;
					case Format::R8G8_Unorm:
						return GL_RG8;
					case Format::R8G8_Uint:
						return GL_RG8UI;
					case Format::R8G8_Snorm:
						return GL_RG8_SNORM;
					case Format::R8G8_Sint:
						return GL_RG8I;
					case Format::R16_Float:
						return GL_R16F;
					case Format::D16_Unorm:
						return GL_DEPTH_COMPONENT16;
					case Format::R16_Unorm:
						return GL_R16;
					case Format::R16_Uint:
						return GL_R16UI;
					case Format::R16_Snorm:
						return GL_R16_SNORM;
					case Format::R16_Sint:
						return GL_R16I;
					case Format::R8_Unorm:
						return GL_R8;
					case Format::R8_Uint:
						return GL_R8UI;
					case Format::R8_Snorm:
						return GL_R8_SNORM;
					case Format::R8_Sint:
						return GL_R8I;
					case Format::R1_Unorm:
						return GL_R8;
					case Format::R9G9B9E5_Share_Dexp:
						return GL_RGB9_E5;
					case Format::R8G8_B8G8_Unorm:
						return GL_RGB8;
					case Format::A8_Unorm:
#ifdef GL_ALPHA8_EXT
						return GL_ALPHA8_EXT;
#else
						return GL_R8;
#endif
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
						return (Mag ? GL_NEAREST : GL_NEAREST);
					case PixelFilter::Min_Mag_Point_Mip_Linear:
					case PixelFilter::Compare_Min_Mag_Point_Mip_Linear:
						return (Mag ? GL_NEAREST : GL_NEAREST_MIPMAP_LINEAR);
					case PixelFilter::Min_Point_Mag_Linear_Mip_Point:
					case PixelFilter::Compare_Min_Point_Mag_Linear_Mip_Point:
						return (Mag ? GL_NEAREST : GL_LINEAR_MIPMAP_NEAREST);
					case PixelFilter::Min_Point_Mag_Mip_Linear:
					case PixelFilter::Compare_Min_Point_Mag_Mip_Linear:
						return (Mag ? GL_NEAREST : GL_LINEAR_MIPMAP_LINEAR);
					case PixelFilter::Min_Linear_Mag_Mip_Point:
					case PixelFilter::Compare_Min_Linear_Mag_Mip_Point:
						return (Mag ? GL_LINEAR : GL_NEAREST_MIPMAP_NEAREST);
					case PixelFilter::Min_Linear_Mag_Point_Mip_Linear:
					case PixelFilter::Compare_Min_Linear_Mag_Point_Mip_Linear:
						return (Mag ? GL_LINEAR : GL_NEAREST_MIPMAP_LINEAR);
					case PixelFilter::Min_Mag_Linear_Mip_Point:
					case PixelFilter::Compare_Min_Mag_Linear_Mip_Point:
						return (Mag ? GL_LINEAR : GL_LINEAR_MIPMAP_NEAREST);
					case PixelFilter::Anistropic:
					case PixelFilter::Compare_Anistropic:
					case PixelFilter::Min_Mag_Mip_Linear:
					case PixelFilter::Compare_Min_Mag_Mip_Linear:
						return (Mag ? GL_LINEAR : GL_LINEAR_MIPMAP_LINEAR);
				}

				return GL_NEAREST;
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
			void OGLDevice::GetBackBufferSize(Format Value, int* X, int* Y, int* Z, int* W)
			{
				VI_ASSERT(X && Y && Z && W, "xyzw should be set");
				switch (Value)
				{
					case Vitex::Graphics::Format::A8_Unorm:
						*X = *Y = *Z = 0;
						*W = 8;
						break;
					case Vitex::Graphics::Format::D24_Unorm_S8_Uint:
						*Z = *W = 0;
						*Y = 8;
						*X = 32;
						break;
					case Vitex::Graphics::Format::D32_Float:
						*Y = *Z = *W = 0;
						*X = 32;
						break;
					case Vitex::Graphics::Format::R10G10B10A2_Uint:
					case Vitex::Graphics::Format::R10G10B10A2_Unorm:
						*W = 2;
						*Z = 10;
						*X = *Y = 11;
						break;
					case Vitex::Graphics::Format::R11G11B10_Float:
						*W = 0;
						*Z = 10;
						*X = *Y = 11;
						break;
					case Vitex::Graphics::Format::R16G16B16A16_Float:
					case Vitex::Graphics::Format::R16G16B16A16_Sint:
					case Vitex::Graphics::Format::R16G16B16A16_Snorm:
					case Vitex::Graphics::Format::R16G16B16A16_Uint:
					case Vitex::Graphics::Format::R16G16B16A16_Unorm:
						*X = *Y = *Z = *W = 16;
						break;
					case Vitex::Graphics::Format::R16G16_Float:
					case Vitex::Graphics::Format::R16G16_Sint:
					case Vitex::Graphics::Format::R16G16_Snorm:
					case Vitex::Graphics::Format::R16G16_Uint:
					case Vitex::Graphics::Format::R16G16_Unorm:
						*Z = *W = 0;
						*X = *Y = 16;
						break;
					case Vitex::Graphics::Format::R16_Float:
					case Vitex::Graphics::Format::R16_Sint:
					case Vitex::Graphics::Format::R16_Snorm:
					case Vitex::Graphics::Format::R16_Uint:
					case Vitex::Graphics::Format::R16_Unorm:
					case Vitex::Graphics::Format::D16_Unorm:
						*Y = *Z = *W = 0;
						*X = 16;
						break;
					case Vitex::Graphics::Format::R1_Unorm:
						*Y = *Z = *W = 0;
						*X = 1;
						break;
					case Vitex::Graphics::Format::R32G32B32A32_Float:
					case Vitex::Graphics::Format::R32G32B32A32_Sint:
					case Vitex::Graphics::Format::R32G32B32A32_Uint:
						*X = *Y = *Z = *W = 32;
						break;
					case Vitex::Graphics::Format::R32G32B32_Float:
					case Vitex::Graphics::Format::R32G32B32_Sint:
					case Vitex::Graphics::Format::R32G32B32_Uint:
						*W = 0;
						*X = *Y = *Z = 32;
						break;
					case Vitex::Graphics::Format::R32G32_Float:
					case Vitex::Graphics::Format::R32G32_Sint:
					case Vitex::Graphics::Format::R32G32_Uint:
						*Z = *W = 0;
						*X = *Y = 32;
						break;
					case Vitex::Graphics::Format::R32_Float:
					case Vitex::Graphics::Format::R32_Sint:
					case Vitex::Graphics::Format::R32_Uint:
						*Y = *Z = *W = 0;
						*X = 32;
						break;
					case Vitex::Graphics::Format::R8G8_Sint:
					case Vitex::Graphics::Format::R8G8_Snorm:
					case Vitex::Graphics::Format::R8G8_Uint:
					case Vitex::Graphics::Format::R8G8_Unorm:
						*Z = *W = 0;
						*X = *Y = 8;
						break;
					case Vitex::Graphics::Format::R8_Sint:
					case Vitex::Graphics::Format::R8_Snorm:
					case Vitex::Graphics::Format::R8_Uint:
					case Vitex::Graphics::Format::R8_Unorm:
						*Y = *Z = *W = 0;
						*X = 8;
						break;
					case Vitex::Graphics::Format::R9G9B9E5_Share_Dexp:
						break;
					case Vitex::Graphics::Format::R8G8B8A8_Sint:
					case Vitex::Graphics::Format::R8G8B8A8_Snorm:
					case Vitex::Graphics::Format::R8G8B8A8_Uint:
					case Vitex::Graphics::Format::R8G8B8A8_Unorm:
					case Vitex::Graphics::Format::R8G8B8A8_Unorm_SRGB:
					case Vitex::Graphics::Format::R8G8_B8G8_Unorm:
					default:
						*X = *Y = *Z = *W = 8;
						break;
				}
			}
			void OGLDevice::DebugMessage(GLenum Source, GLenum Type, GLuint Id, GLenum Severity, GLsizei Length, const GLchar* Message, const void* Data)
			{
				static const GLuint NullTextureCannotBeSampled = 131204;
				if (Id == NullTextureCannotBeSampled)
					return;

				const char* _Source, *_Type;
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
						_Source = "GENERAL";
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
						_Type = "LOG";
						break;
				}

				switch (Severity)
				{
					case GL_DEBUG_SEVERITY_HIGH:
						VI_ERR("[opengl] %s (%s:%d): %s", _Source, _Type, Id, Message);
						break;
					case GL_DEBUG_SEVERITY_MEDIUM:
						VI_WARN("[opengl] %s (%s:%d): %s", _Source, _Type, Id, Message);
						break;
					case GL_DEBUG_SEVERITY_LOW:
						VI_DEBUG("[opengl] %s (%s:%d): %s", _Source, _Type, Id, Message);
						break;
					case GL_DEBUG_SEVERITY_NOTIFICATION:
						VI_TRACE("[opengl] %s (%s:%d): %s", _Source, _Type, Id, Message);
						break;
				}

				(void)_Source;
				(void)_Type;
			}
		}
	}
}
#pragma warning(pop)
#endif