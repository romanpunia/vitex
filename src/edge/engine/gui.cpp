#include "gui.h"
#include "../core/network.h"
#ifdef ED_USE_RMLUI
#include <RmlUi/Core.h>
#include <RmlUi/Core/Stream.h>
#include <Source/Core/StyleSheetFactory.h>
#include <Source/Core/ElementStyle.h>
#endif
#pragma warning(push)
#pragma warning(disable: 4996)

namespace Edge
{
	namespace Engine
	{
		namespace GUI
		{
#ifdef ED_USE_RMLUI
			struct GeometryBuffer
			{
				Graphics::ElementBuffer* VertexBuffer;
				Graphics::ElementBuffer* IndexBuffer;
				Graphics::Texture2D* Texture;

				GeometryBuffer() : VertexBuffer(nullptr), IndexBuffer(nullptr), Texture(nullptr)
				{
				}
				~GeometryBuffer()
				{
					ED_RELEASE(VertexBuffer);
					ED_RELEASE(IndexBuffer);
				}
			};

			class RenderSubsystem : public Rml::RenderInterface
			{
			private:
				Graphics::RasterizerState* ScissorNoneRasterizer;
				Graphics::RasterizerState* NoneRasterizer;
				Graphics::DepthStencilState* LessDepthStencil;
				Graphics::DepthStencilState* NoneDepthStencil;
				Graphics::DepthStencilState* ScissorDepthStencil;
				Graphics::BlendState* AlphaBlend;
				Graphics::BlendState* ColorlessBlend;
				Graphics::SamplerState* Sampler;
				Graphics::InputLayout* Layout;
				Graphics::ElementBuffer* VertexBuffer;
				Graphics::Shader* Shader;
				Graphics::GraphicsDevice* Device;
				ContentManager* Content;
				Compute::Matrix4x4 Transform;
				Compute::Matrix4x4 Ortho;
				bool HasTransform;

			public:
				Graphics::Texture2D* Background;

			public:
				RenderSubsystem() : Rml::RenderInterface(), Device(nullptr), Content(nullptr), HasTransform(false), Background(nullptr)
				{
					Shader = nullptr;
					VertexBuffer = nullptr;
					Layout = nullptr;
					NoneRasterizer = nullptr;
					ScissorNoneRasterizer = nullptr;
					ScissorDepthStencil = nullptr;
					LessDepthStencil = nullptr;
					NoneDepthStencil = nullptr;
					AlphaBlend = nullptr;
					ColorlessBlend = nullptr;
					Sampler = nullptr;
				}
				virtual ~RenderSubsystem() override
				{
					ED_RELEASE(VertexBuffer);
					ED_RELEASE(Shader);
				}
				virtual void RenderGeometry(Rml::Vertex* Vertices, int VerticesSize, int* Indices, int IndicesSize, Rml::TextureHandle Texture, const Rml::Vector2f& Translation) override
				{
					ED_ASSERT_V(Device != nullptr, "graphics device should be set");
					ED_ASSERT_V(Vertices != nullptr, "vertices should be set");
					ED_ASSERT_V(Indices != nullptr, "indices should be set");

					Device->ImBegin();
					Device->ImTopology(Graphics::PrimitiveTopology::Triangle_List);
					Device->ImTexture((Graphics::Texture2D*)Texture);

					if (HasTransform)
						Device->ImTransform(Compute::Matrix4x4::CreateTranslation(Compute::Vector3(Translation.x, Translation.y)) * Transform * Ortho);
					else
						Device->ImTransform(Compute::Matrix4x4::CreateTranslation(Compute::Vector3(Translation.x, Translation.y)) * Ortho);

					for (int i = IndicesSize; i-- > 0;)
					{
						Rml::Vertex& V = Vertices[Indices[i]];
						Device->ImEmit();
						Device->ImPosition(V.position.x, V.position.y, 0.0f);
						Device->ImTexCoord(V.tex_coord.x, V.tex_coord.y);
						Device->ImColor(V.colour.red / 255.0f, V.colour.green / 255.0f, V.colour.blue / 255.0f, V.colour.alpha / 255.0f);
					}

					Device->ImEnd();
				}
				virtual Rml::CompiledGeometryHandle CompileGeometry(Rml::Vertex* Vertices, int VerticesCount, int* Indices, int IndicesCount, Rml::TextureHandle Handle) override
				{
					ED_ASSERT(Device != nullptr, (Rml::CompiledGeometryHandle)nullptr, "graphics device should be set");
					GeometryBuffer* Result = ED_NEW(GeometryBuffer);
					Result->Texture = (Graphics::Texture2D*)Handle;

					Graphics::ElementBuffer::Desc F = Graphics::ElementBuffer::Desc();
					F.AccessFlags = Graphics::CPUAccess::Invalid;
					F.Usage = Graphics::ResourceUsage::Default;
					F.BindFlags = Graphics::ResourceBind::Vertex_Buffer;
					F.ElementCount = (unsigned int)VerticesCount;
					F.Elements = (void*)Vertices;
					F.ElementWidth = sizeof(Rml::Vertex);
					Result->VertexBuffer = Device->CreateElementBuffer(F);

					F = Graphics::ElementBuffer::Desc();
					F.AccessFlags = Graphics::CPUAccess::Invalid;
					F.Usage = Graphics::ResourceUsage::Default;
					F.BindFlags = Graphics::ResourceBind::Index_Buffer;
					F.ElementCount = (unsigned int)IndicesCount;
					F.ElementWidth = sizeof(int);
					F.Elements = (void*)Indices;
					Result->IndexBuffer = Device->CreateElementBuffer(F);

					return (Rml::CompiledGeometryHandle)Result;
				}
				virtual void RenderCompiledGeometry(Rml::CompiledGeometryHandle Handle, const Rml::Vector2f& Translation) override
				{
					GeometryBuffer* Buffer = (GeometryBuffer*)Handle;
					ED_ASSERT_V(Device != nullptr, "graphics device should be set");
					ED_ASSERT_V(Buffer != nullptr, "buffer should be set");

					Device->Render.Diffuse = (Buffer->Texture != nullptr ? 1.0f : 0.0f);
					if (HasTransform)
						Device->Render.Transform = Compute::Matrix4x4::CreateTranslation(Compute::Vector3(Translation.x, Translation.y)) * Transform * Ortho;
					else
						Device->Render.Transform = Compute::Matrix4x4::CreateTranslation(Compute::Vector3(Translation.x, Translation.y)) * Ortho;

					Device->SetTexture2D(Buffer->Texture, 1, ED_PS);
					Device->SetShader(Shader, ED_VS | ED_PS);
					Device->SetVertexBuffer(Buffer->VertexBuffer);
					Device->SetIndexBuffer(Buffer->IndexBuffer, Graphics::Format::R32_Uint);
					Device->UpdateBuffer(Graphics::RenderBufferType::Render);
					Device->DrawIndexed((unsigned int)Buffer->IndexBuffer->GetElements(), 0, 0);
				}
				virtual void ReleaseCompiledGeometry(Rml::CompiledGeometryHandle Handle) override
				{
					GeometryBuffer* Resource = (GeometryBuffer*)Handle;
					ED_DELETE(GeometryBuffer, Resource);
				}
				virtual void EnableScissorRegion(bool Enable) override
				{
					ED_ASSERT_V(Device != nullptr, "graphics device should be set");
					Ortho = Compute::Matrix4x4::CreateOrthographicOffCenter(0, (float)Device->GetRenderTarget()->GetWidth(), (float)Device->GetRenderTarget()->GetHeight(), 0.0f, -30000.0f, 10000.0f);
					Device->SetSamplerState(Sampler, 1, 1, ED_PS);
					Device->SetBlendState(AlphaBlend);
					if (Enable)
					{
						if (HasTransform)
						{
							Device->SetRasterizerState(NoneRasterizer);
							Device->SetDepthStencilState(LessDepthStencil);
						}
						else
						{
							Device->SetRasterizerState(ScissorNoneRasterizer);
							Device->SetDepthStencilState(NoneDepthStencil);
						}
					}
					else
					{
						Device->SetRasterizerState(NoneRasterizer);
						Device->SetDepthStencilState(NoneDepthStencil);
					}
					Device->SetInputLayout(Layout);
				}
				virtual void SetScissorRegion(int X, int Y, int Width, int Height) override
				{
					ED_ASSERT_V(Device != nullptr, "graphics device should be set");
					if (!HasTransform)
					{
						float WY = (float)Device->GetRenderTarget()->GetHeight();
						WY -= Y + Height;

						Compute::Rectangle Scissor;
						Scissor.Left = X;
						Scissor.Top = Y;
						Scissor.Right = X + Width;
						Scissor.Bottom = Y + Height;

						return Device->SetScissorRects(1, &Scissor);
					}

					Graphics::MappedSubresource Subresource;
					if (Device->Map(VertexBuffer, Graphics::ResourceMap::Write_Discard, &Subresource))
					{
						float fWidth = (float)Width;
						float fHeight = (float)Height;
						float fX = (float)X;
						float fY = (float)Y;

						Rml::Vertex Vertices[6] =
						{
							{ Rml::Vector2f(fX, fY), Rml::Colourb(), Rml::Vector2f() },
							{ Rml::Vector2f(fX, fY + fHeight), Rml::Colourb(), Rml::Vector2f() },
							{ Rml::Vector2f(fX + fWidth, fY + fHeight), Rml::Colourb(), Rml::Vector2f() },
							{ Rml::Vector2f(fX, fY), Rml::Colourb(), Rml::Vector2f() },
							{ Rml::Vector2f(fX + fWidth, fY + fHeight), Rml::Colourb(), Rml::Vector2f() },
							{ Rml::Vector2f(fX + fWidth, fY), Rml::Colourb(), Rml::Vector2f() }
						};
						memcpy(Subresource.Pointer, Vertices, sizeof(Rml::Vertex) * 6);
						Device->Unmap(VertexBuffer, &Subresource);
					}

					Device->Render.Transform = Transform * Ortho;
					Device->ClearDepth();
					Device->SetBlendState(ColorlessBlend);
					Device->SetShader(Shader, ED_VS | ED_PS);
					Device->SetVertexBuffer(VertexBuffer);
					Device->UpdateBuffer(Graphics::RenderBufferType::Render);
					Device->Draw((unsigned int)VertexBuffer->GetElements(), 0);
					Device->SetDepthStencilState(ScissorDepthStencil);
					Device->SetBlendState(AlphaBlend);
				}
				virtual bool LoadTexture(Rml::TextureHandle& Handle, Rml::Vector2i& TextureDimensions, const Rml::String& Source) override
				{
					ED_ASSERT(Content != nullptr, false, "content manager should be set");
					Graphics::Texture2D* Result = Content->Load<Graphics::Texture2D>(Source);
					if (!Result)
						return false;

					TextureDimensions.x = Result->GetWidth();
					TextureDimensions.y = Result->GetHeight();
					Handle = (Rml::TextureHandle)Result;

					return true;
				}
				virtual bool GenerateTexture(Rml::TextureHandle& Handle, const Rml::byte* Source, const Rml::Vector2i& SourceDimensions) override
				{
					ED_ASSERT(Device != nullptr, false, "graphics device should be set");
					ED_ASSERT(Source != nullptr, false, "source should be set");

					Graphics::Texture2D::Desc F = Graphics::Texture2D::Desc();
					F.FormatMode = Graphics::Format::R8G8B8A8_Unorm;
					F.Usage = Graphics::ResourceUsage::Default;
					F.BindFlags = Graphics::ResourceBind::Shader_Input;
					F.Width = (unsigned int)SourceDimensions.x;
					F.Height = (unsigned int)SourceDimensions.y;
					F.MipLevels = 1;
					F.Data = (void*)Source;
					F.RowPitch = (unsigned int)SourceDimensions.x * 4;
					F.DepthPitch = 0;

					Graphics::Texture2D* Result = Device->CreateTexture2D(F);
					if (!Result)
						return false;

					Handle = (Rml::TextureHandle)Result;
					return true;
				}
				virtual void ReleaseTexture(Rml::TextureHandle Handle) override
				{
					Graphics::Texture2D* Resource = (Graphics::Texture2D*)Handle;
					ED_RELEASE(Resource);
				}
				virtual void SetTransform(const Rml::Matrix4f* NewTransform) override
				{
					HasTransform = (NewTransform != nullptr);
					if (HasTransform)
						Transform = Subsystem::ToMatrix(NewTransform);
				}
				void Attach(ContentManager* NewContent)
				{
					ED_ASSERT_V(NewContent != nullptr, "content manager should be set");
					ED_ASSERT_V(NewContent->GetDevice() != nullptr, "graphics device should be set");

					ED_CLEAR(VertexBuffer);
					ED_CLEAR(Shader);

					Content = NewContent;
					Device = Content->GetDevice();

					Graphics::Shader::Desc I = Graphics::Shader::Desc();
					if (Device->GetSection("interface/geometry", &I))
						Shader = Device->CreateShader(I);

					Graphics::ElementBuffer::Desc F = Graphics::ElementBuffer::Desc();
					F.AccessFlags = Graphics::CPUAccess::Write;
					F.Usage = Graphics::ResourceUsage::Dynamic;
					F.BindFlags = Graphics::ResourceBind::Vertex_Buffer;
					F.ElementCount = (unsigned int)6;
					F.Elements = (void*)nullptr;
					F.ElementWidth = sizeof(Rml::Vertex);

					VertexBuffer = Device->CreateElementBuffer(F);
					Layout = Device->GetInputLayout("gui-vertex");
					ScissorNoneRasterizer = Device->GetRasterizerState("cull-none-scissor");
					NoneRasterizer = Device->GetRasterizerState("cull-none");
					NoneDepthStencil = Device->GetDepthStencilState("none");
					LessDepthStencil = Device->GetDepthStencilState("less");
					ScissorDepthStencil = Device->GetDepthStencilState("greater-equal");
					AlphaBlend = Device->GetBlendState("additive-source");
					ColorlessBlend = Device->GetBlendState("overwrite-colorless");
					Sampler = Device->GetSamplerState("trilinear-x16");
				}
				void ResizeBuffers(int Width, int Height)
				{
					Subsystem::ResizeDecorators(Width, Height);
				}
				ContentManager* GetContent()
				{
					return Content;
				}
				Graphics::GraphicsDevice* GetDevice()
				{
					return Device;
				}
				Compute::Matrix4x4* GetTransform()
				{
					return HasTransform ? &Transform : nullptr;
				}
				Compute::Matrix4x4* GetProjection()
				{
					return &Ortho;
				}
			};

			class FileSubsystem : public Rml::FileInterface
			{
			public:
				FileSubsystem() : Rml::FileInterface()
				{
				}
				virtual Rml::FileHandle Open(const Rml::String& Path) override
				{
					std::string Target = Path;
					Network::SourceURL URL(Target);
					if (URL.Protocol == "file")
					{
						if (!Core::OS::File::IsExists(Target.c_str()))
						{
							ContentManager* Content = (Subsystem::GetRenderInterface() ? Subsystem::GetRenderInterface()->GetContent() : nullptr);
							Target = (Content ? Core::OS::Path::Resolve(Path, Content->GetEnvironment()) : Core::OS::Path::Resolve(Path.c_str()));
							Target = (Target.empty() ? Path.c_str() : Target.c_str());
						}
					}
					else if (URL.Protocol != "http" && URL.Protocol != "https")
						return (Rml::FileHandle)nullptr;

					return (Rml::FileHandle)Core::OS::File::Open(Target, Core::FileMode::Binary_Read_Only);
				}
				virtual void Close(Rml::FileHandle File) override
				{
					Core::Stream* Stream = (Core::Stream*)File;
					ED_RELEASE(Stream);
				}
				virtual size_t Read(void* Buffer, size_t Size, Rml::FileHandle File) override
				{
					Core::Stream* Stream = (Core::Stream*)File;
					ED_ASSERT(Stream != nullptr, 0, "stream should be set");
					return Stream->Read((char*)Buffer, Size);
				}
				virtual bool Seek(Rml::FileHandle File, long Offset, int Origin) override
				{
					Core::Stream* Stream = (Core::Stream*)File;
					ED_ASSERT(Stream != nullptr, false, "stream should be set");
					return Stream->Seek((Core::FileSeek)Origin, Offset) == 0;
				}
				virtual size_t Tell(Rml::FileHandle File) override
				{
					Core::Stream* Stream = (Core::Stream*)File;
					ED_ASSERT(Stream != nullptr, 0, "stream should be set");
					return Stream->Tell();
				}
			};

			class MainSubsystem : public Rml::SystemInterface
			{
			private:
				std::unordered_map<std::string, TranslationCallback> Translators;
				std::unordered_map<std::string, bool> Fonts;
				Graphics::Activity* Activity;
				Core::Timer* Time;

			public:
				MainSubsystem() : Rml::SystemInterface(), Activity(nullptr), Time(nullptr)
				{
				}
				virtual void SetMouseCursor(const Rml::String& CursorName) override
				{
					ED_ASSERT_V(Activity != nullptr, "activity should be set");
					if (CursorName == "none")
						Activity->SetCursor(Graphics::DisplayCursor::None);
					else if (CursorName == "default")
						Activity->SetCursor(Graphics::DisplayCursor::Arrow);
					else if (CursorName == "move")
						Activity->SetCursor(Graphics::DisplayCursor::ResizeAll);
					else if (CursorName == "pointer")
						Activity->SetCursor(Graphics::DisplayCursor::Hand);
					else if (CursorName == "text")
						Activity->SetCursor(Graphics::DisplayCursor::TextInput);
					else if (CursorName == "progress")
						Activity->SetCursor(Graphics::DisplayCursor::Progress);
					else if (CursorName == "wait")
						Activity->SetCursor(Graphics::DisplayCursor::Wait);
					else if (CursorName == "not-allowed")
						Activity->SetCursor(Graphics::DisplayCursor::No);
					else if (CursorName == "crosshair")
						Activity->SetCursor(Graphics::DisplayCursor::Crosshair);
					else if (CursorName == "ns-resize")
						Activity->SetCursor(Graphics::DisplayCursor::ResizeNS);
					else if (CursorName == "ew-resize")
						Activity->SetCursor(Graphics::DisplayCursor::ResizeEW);
					else if (CursorName == "nesw-resize")
						Activity->SetCursor(Graphics::DisplayCursor::ResizeNESW);
					else if (CursorName == "nwse-resize")
						Activity->SetCursor(Graphics::DisplayCursor::ResizeNWSE);
				}
				virtual void SetClipboardText(const Rml::String& Text) override
				{
					ED_ASSERT_V(Activity != nullptr, "activity should be set");
					Activity->SetClipboardText(Text);
				}
				virtual void GetClipboardText(Rml::String& Text) override
				{
					ED_ASSERT_V(Activity != nullptr, "activity should be set");
					Text = Activity->GetClipboardText();
				}
				virtual void ActivateKeyboard(Rml::Vector2f CaretPosition, float LineHeight) override
				{
					ED_ASSERT_V(Activity != nullptr, "activity should be set");
					Activity->SetScreenKeyboard(true);
				}
				virtual void DeactivateKeyboard() override
				{
					ED_ASSERT_V(Activity != nullptr, "activity should be set");
					Activity->SetScreenKeyboard(false);
				}
				virtual void JoinPath(Rml::String& Result, const Rml::String& Path1, const Rml::String& Path2) override
				{
					ContentManager* Content = (Subsystem::GetRenderInterface() ? Subsystem::GetRenderInterface()->GetContent() : nullptr);
					ED_ASSERT_V(Content != nullptr, "activity should be set");

					std::string Proto1, Proto2;
					std::string Fixed1 = GetFixedURL(Path1, Proto1);
					std::string Fixed2 = GetFixedURL(Path2, Proto2);

					if (Proto1 != "file" && Proto2 == "file")
					{
						Core::Parser Buffer(&Result);
						if (!Buffer.Assign(Path1).EndsWith('/'))
							Buffer.Append('/');

						Buffer.Append(Fixed2).Replace("/////", "//");
						Core::Parser::Settle Idx = Buffer.Find("://");
						Buffer.Replace("//", "/", Idx.Found ? Idx.End : 0);
					}
					else if (Proto1 == "file" && Proto2 == "file")
					{
						Result = Core::OS::Path::Resolve(Fixed2, Core::OS::Path::GetDirectory(Fixed1.c_str()));
						if (Result.empty())
							Result = Core::OS::Path::Resolve(Fixed2, Content->GetEnvironment());
					}
					else if (Proto1 == "file" && Proto2 != "file")
						Result = Core::Parser(Path2).Replace("/////", "//").R();
				}
				virtual bool LogMessage(Rml::Log::Type Type, const Rml::String& Message) override
				{
					switch (Type)
					{
						case Rml::Log::LT_ERROR:
							ED_ERR("[gui] %.*s", Message.size(), Message.c_str());
							break;
						case Rml::Log::LT_WARNING:
							ED_WARN("[gui] %.*s", Message.size(), Message.c_str());
							break;
						case Rml::Log::LT_INFO:
						case Rml::Log::LT_ASSERT:
							ED_DEBUG("[gui] %.*s", Message.size(), Message.c_str());
							break;
						default:
							break;
					}

					return true;
				}
				virtual int TranslateString(Rml::String& Result, const Rml::String& KeyName) override
				{
					for (auto& Item : Translators)
					{
						if (!Item.second)
							continue;

						Result = Item.second(KeyName);
						if (!Result.empty())
							return 1;
					}

					Result = KeyName;
					return 0;
				}
				virtual double GetElapsedTime() override
				{
					if (!Time)
						return 0.0;

					return (double)Time->GetElapsed();
				}
				void Attach(Graphics::Activity* NewActivity, Core::Timer* NewTime)
				{
					Activity = NewActivity;
					Time = NewTime;
				}
				void SetTranslator(const std::string& Name, const TranslationCallback& Callback)
				{
					auto It = Translators.find(Name);
					if (It == Translators.end())
						Translators.insert(std::make_pair(Name, Callback));
					else
						It->second = Callback;
				}
				bool AddFontFace(const std::string& Path, bool UseAsFallback)
				{
					auto It = Fonts.find(Path);
					if (It != Fonts.end())
						return true;

					if (!Rml::LoadFontFace(Path, UseAsFallback))
						return false;

					Fonts.insert(std::make_pair(Path, UseAsFallback));
					return true;
				}
				std::unordered_map<std::string, bool>* GetFontFaces()
				{
					return &Fonts;
				}
				std::string GetFixedURL(const std::string& URL, std::string& Proto)
				{
					if (!Core::Parser(&URL).Find("://").Found)
					{
						Proto = "file";
						return URL;
					}

					Rml::URL Base(URL);
					Proto = Base.GetProtocol();
					return Base.GetPathedFileName();
				}
			};

			class ScopedContext : public Rml::Context
			{
			public:
				GUI::Context* Basis;

			public:
				ScopedContext(const Rml::String& Name) : Rml::Context(Name), Basis(nullptr)
				{
				}
			};

			class ContextInstancer : public Rml::ContextInstancer
			{
			public:
				virtual Rml::ContextPtr InstanceContext(const Rml::String& Name) override
				{
					return Rml::ContextPtr(ED_NEW(ScopedContext, Name));
				}
				virtual void ReleaseContext(Rml::Context* Context) override
				{
					ScopedContext* Item = (ScopedContext*)Context;
					ED_DELETE(ScopedContext, Item);
				}
				virtual void Release() override
				{
					ED_DELETE(ContextInstancer, this);
				}
			};

			class DocumentSubsystem : public Rml::ElementDocument
			{
			public:
				DocumentSubsystem(const Rml::String& Tag) : Rml::ElementDocument(Tag)
				{
				}
				virtual ~DocumentSubsystem() = default;
				void LoadInlineScript(const Rml::String& Content, const Rml::String& Path, int Line) override
				{
					ScopedContext* Scope = (ScopedContext*)GetContext();
					ED_ASSERT_V(Scope && Scope->Basis && Scope->Basis->Compiler, "context should be scoped");

					Scope->Basis->AddRef();
					Script::VMCompiler* Compiler = Scope->Basis->Compiler;
					Compiler->ExecuteScoped(Content.c_str(), Content.size()).Await([Scope](int&&)
					{
						Scope->Basis->Release();
					});
				}
				void LoadExternalScript(const Rml::String& Path) override
				{
					ScopedContext* Scope = (ScopedContext*)GetContext();
					ED_ASSERT_V(Scope && Scope->Basis && Scope->Basis->Compiler, "context should be scoped");

					Script::VMCompiler* Compiler = Scope->Basis->Compiler;
					if (Compiler->LoadFile(Core::Parser(Path).Replace('|', ':').R()) < 0)
						return;

					Compiler->Compile().Await([Scope, Compiler](int&& Status)
					{
						if (Status < 0)
							return;

						Script::VMFunction Main = Compiler->GetModule().GetFunctionByName("Main");
						if (!Main.IsValid())
							return;

						Scope->Basis->AddRef();
						Script::VMContext* Context = Compiler->GetContext();
						Context->TryExecute(false, Main, [Main, Scope](Script::VMContext* Context)
						{
							if (Main.GetArgsCount() == 1)
								Context->SetArgObject(0, Scope->Basis);
						}).Await([Scope](int&&)
						{
							Scope->Basis->Release();
						});
					});
				}
			};

			class DocumentInstancer : public Rml::ElementInstancer
			{
			public:
				Rml::ElementPtr InstanceElement(Rml::Element*, const Rml::String& Tag, const Rml::XMLAttributes&) override
				{
					return Rml::ElementPtr(ED_NEW(DocumentSubsystem, Tag));
				}
				void ReleaseElement(Rml::Element* Element) override
				{
					ED_DELETE(Element, Element);
				}
			};

			class ListenerSubsystem : public Rml::EventListener
			{
			public:
				Script::VMFunction Function;
				std::string Memory;

			public:
				ListenerSubsystem(const Rml::String& Code, Rml::Element* Element) : Memory(Code), Function(nullptr)
				{
				}
				void OnDetach(Rml::Element* Element) override
				{
					Function.Release();
					ED_DELETE(ListenerSubsystem, this);
				}
				void ProcessEvent(Rml::Event& Event) override
				{
					ScopedContext* Scope = (ScopedContext*)Event.GetCurrentElement()->GetContext();
					ED_ASSERT_V(Scope && Scope->Basis && Scope->Basis->Compiler, "context should be scoped");
					if (!CompileInline(Scope))
						return;

					Rml::Event* Ptr = Rml::Factory::InstanceEvent(Event.GetTargetElement(), Event.GetId(), Event.GetType(), Event.GetParameters(), Event.IsInterruptible()).release();
					if (Ptr != nullptr)
					{
						Ptr->SetCurrentElement(Event.GetCurrentElement());
						Ptr->SetPhase(Event.GetPhase());
					}

					Scope->Basis->AddRef();
					Script::VMContext* Context = Scope->Basis->Compiler->GetContext();
					Context->TryExecute(false, Function, [Ptr](Script::VMContext* Context)
					{
						IEvent Event(Ptr);
						Context->SetArgObject(0, &Event);
					}).Await([Scope, Ptr](int&&)
					{
						delete Ptr;
						Scope->Basis->Release();
					});
				}
				bool CompileInline(ScopedContext* Scope)
				{
					if (Function.IsValid())
						return true;

					std::string Name = "__vf" + Compute::Crypto::Hash(Compute::Digests::MD5(), Memory);
					std::string Eval = "void " + Name + "(GUI::Event &in Event){\n";
					Eval.append(Memory);
					Eval += "\n;}";

					Script::VMModule Module = Scope->Basis->Compiler->GetModule();
					return Module.CompileFunction(Name.c_str(), Eval.c_str(), -1, (size_t)Script::VMCompileFlags::ADD_TO_MODULE, &Function) >= 0;
				}
			};

			class ListenerInstancer : public Rml::EventListenerInstancer
			{
			public:
				Rml::EventListener* InstanceEventListener(const Rml::String& Value, Rml::Element* Element) override
				{
					return ED_NEW(ListenerSubsystem, Value, Element);
				}
			};

			class EventSubsystem : public Rml::EventListener
			{
				friend IEvent;

			private:
				EventCallback Listener;
				std::atomic<int> RefCount;

			public:
				EventSubsystem(const EventCallback& Callback) : Rml::EventListener(), Listener(Callback), RefCount(1)
				{
				}
				virtual void OnAttach(Rml::Element*) override
				{
					RefCount++;
				}
				virtual void OnDetach(Rml::Element*) override
				{
					if (!--RefCount)
						ED_DELETE(EventSubsystem, this);
				}
				virtual void ProcessEvent(Rml::Event& Event) override
				{
					if (!Listener)
						return;

					IEvent Basis(&Event);
					Listener(Basis);
				}
			};
#endif
			void IVariant::Convert(Rml::Variant* From, Core::Variant* To)
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT_V(From && To, "from and to should be set");
				switch (From->GetType())
				{
					case Rml::Variant::BOOL:
						*To = Core::Var::Boolean(From->Get<bool>());
						break;
					case Rml::Variant::FLOAT:
					case Rml::Variant::DOUBLE:
						*To = Core::Var::Number(From->Get<double>());
						break;
					case Rml::Variant::BYTE:
					case Rml::Variant::CHAR:
					case Rml::Variant::INT:
					case Rml::Variant::INT64:
						*To = Core::Var::Integer(From->Get<int64_t>());
						break;
					case Rml::Variant::VECTOR2:
					{
						Rml::Vector2f T = From->Get<Rml::Vector2f>();
						*To = Core::Var::String(FromVector2(Compute::Vector2(T.x, T.y)));
						break;
					}
					case Rml::Variant::VECTOR3:
					{
						Rml::Vector3f T = From->Get<Rml::Vector3f>();
						*To = Core::Var::String(FromVector3(Compute::Vector3(T.x, T.y, T.z)));
						break;
					}
					case Rml::Variant::VECTOR4:
					{
						Rml::Vector4f T = From->Get<Rml::Vector4f>();
						*To = Core::Var::String(FromVector4(Compute::Vector4(T.x, T.y, T.z, T.w)));
						break;
					}
					case Rml::Variant::STRING:
					case Rml::Variant::COLOURF:
					case Rml::Variant::COLOURB:
						*To = Core::Var::String(From->Get<std::string>());
						break;
					case Rml::Variant::VOIDPTR:
						*To = Core::Var::Pointer(From->Get<void*>());
						break;
					default:
						*To = Core::Var::Undefined();
						break;
				}
#endif
			}
			void IVariant::Revert(Core::Variant* From, Rml::Variant* To)
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT_V(From && To, "from and to should be set");
				switch (From->GetType())
				{
					case Core::VarType::Null:
						*To = Rml::Variant((void*)nullptr);
						break;
					case Core::VarType::String:
					{
						std::string Blob = From->GetBlob();
						int Type = IVariant::GetVectorType(Blob);
						if (Type == 2)
						{
							Compute::Vector2 T = IVariant::ToVector2(Blob);
							*To = Rml::Variant(Rml::Vector2f(T.X, T.Y));
						}
						else if (Type == 3)
						{
							Compute::Vector3 T = IVariant::ToVector3(Blob);
							*To = Rml::Variant(Rml::Vector3f(T.X, T.Y, T.Z));
						}
						else if (Type == 4)
						{
							Compute::Vector4 T = IVariant::ToVector4(Blob);
							*To = Rml::Variant(Rml::Vector4f(T.X, T.Y, T.Z, T.W));
						}
						else
							*To = Rml::Variant(From->GetBlob());
						break;
					}
					case Core::VarType::Integer:
						*To = Rml::Variant(From->GetInteger());
						break;
					case Core::VarType::Number:
						*To = Rml::Variant(From->GetNumber());
						break;
					case Core::VarType::Boolean:
						*To = Rml::Variant(From->GetBoolean());
						break;
					case Core::VarType::Pointer:
						*To = Rml::Variant(From->GetPointer());
						break;
					default:
						To->Clear();
						break;
				}
#endif
			}
			Compute::Vector4 IVariant::ToColor4(const std::string& Value)
			{
				if (Value.empty())
					return 0.0f;

				Compute::Vector4 Result;
				if (Value[0] == '#')
				{
					if (Value.size() == 4)
					{
						char Buffer[7];
						Buffer[0] = Value[1];
						Buffer[1] = Value[1];
						Buffer[2] = Value[2];
						Buffer[3] = Value[2];
						Buffer[4] = Value[3];
						Buffer[5] = Value[3];
						Buffer[6] = '\0';

						unsigned int R = 0, G = 0, B = 0;
						if (sscanf(Buffer, "%02x%02x%02x", &R, &G, &B) == 3)
						{
							Result.X = R / 255.0f;
							Result.Y = G / 255.0f;
							Result.Z = B / 255.0f;
							Result.W = 1.0f;
						}
					}
					else
					{
						unsigned int R = 0, G = 0, B = 0, A = 255;
						if (sscanf(Value.c_str(), "#%02x%02x%02x%02x", &R, &G, &B, &A) == 4)
						{
							Result.X = R / 255.0f;
							Result.Y = G / 255.0f;
							Result.Z = B / 255.0f;
							Result.W = A / 255.0f;
						}
					}
				}
				else
				{
					unsigned int R = 0, G = 0, B = 0, A = 255;
					if (sscanf(Value.c_str(), "%u %u %u %u", &R, &G, &B, &A) == 4)
					{
						Result.X = R / 255.0f;
						Result.Y = G / 255.0f;
						Result.Z = B / 255.0f;
						Result.W = A / 255.0f;
					}
				}

				return Result;
			}
			std::string IVariant::FromColor4(const Compute::Vector4& Base, bool HEX)
			{
				if (!HEX)
					return Core::Form("%d %d %d %d", (unsigned int)(Base.X * 255.0f), (unsigned int)(Base.Y * 255.0f), (unsigned int)(Base.Z * 255.0f), (unsigned int)(Base.W * 255.0f)).R();

				return Core::Form("#%02x%02x%02x%02x",
					(unsigned int)(Base.X * 255.0f),
					(unsigned int)(Base.Y * 255.0f),
					(unsigned int)(Base.Z * 255.0f),
					(unsigned int)(Base.W * 255.0f)).R();
			}
			Compute::Vector4 IVariant::ToColor3(const std::string& Value)
			{
				if (Value.empty())
					return 0.0f;

				Compute::Vector4 Result;
				if (Value[0] == '#')
				{
					unsigned int R = 0, G = 0, B = 0;
					int Fills = 0;

					if (Value.size() == 4)
					{
						char Buffer[7];
						Buffer[0] = Value[1];
						Buffer[1] = Value[1];
						Buffer[2] = Value[2];
						Buffer[3] = Value[2];
						Buffer[4] = Value[3];
						Buffer[5] = Value[3];
						Buffer[6] = '\0';
						Fills = sscanf(Buffer, "%02x%02x%02x", &R, &G, &B);
					}
					else
						Fills = sscanf(Value.c_str(), "#%02x%02x%02x", &R, &G, &B);

					if (Fills == 3)
					{
						Result.X = R / 255.0f;
						Result.Y = G / 255.0f;
						Result.Z = B / 255.0f;
					}
				}
				else
				{
					unsigned int R = 0, G = 0, B = 0;
					if (sscanf(Value.c_str(), "%u %u %u", &R, &G, &B) == 3)
					{
						Result.X = R / 255.0f;
						Result.Y = G / 255.0f;
						Result.Z = B / 255.0f;
					}
				}

				return Result;
			}
			std::string IVariant::FromColor3(const Compute::Vector4& Base, bool HEX)
			{
				if (!HEX)
					return Core::Form("%d %d %d", (unsigned int)(Base.X * 255.0f), (unsigned int)(Base.Y * 255.0f), (unsigned int)(Base.Z * 255.0f)).R();

				return Core::Form("#%02x%02x%02x",
					(unsigned int)(Base.X * 255.0f),
					(unsigned int)(Base.Y * 255.0f),
					(unsigned int)(Base.Z * 255.0f)).R();
			}
			int IVariant::GetVectorType(const std::string& Value)
			{
				if (Value.size() < 2 || Value[0] != 'v')
					return -1;

				if (Value[1] == '2')
					return 2;

				if (Value[1] == '3')
					return 3;

				if (Value[1] == '4')
					return 4;

				return -1;
			}
			Compute::Vector4 IVariant::ToVector4(const std::string& Base)
			{
				Compute::Vector4 Result;
				if (sscanf(Base.c_str(), "v4 %f %f %f %f", &Result.X, &Result.Y, &Result.Z, &Result.W) != 4)
					return Result;

				return Result;
			}
			std::string IVariant::FromVector4(const Compute::Vector4& Base)
			{
				return Core::Form("v4 %f %f %f %f", Base.X, Base.Y, Base.Z, Base.W).R();
			}
			Compute::Vector3 IVariant::ToVector3(const std::string& Base)
			{
				Compute::Vector3 Result;
				if (sscanf(Base.c_str(), "v3 %f %f %f", &Result.X, &Result.Y, &Result.Z) != 3)
					return Result;

				return Result;
			}
			std::string IVariant::FromVector3(const Compute::Vector3& Base)
			{
				return Core::Form("v3 %f %f %f", Base.X, Base.Y, Base.Z).R();
			}
			Compute::Vector2 IVariant::ToVector2(const std::string& Base)
			{
				Compute::Vector2 Result;
				if (sscanf(Base.c_str(), "v2 %f %f", &Result.X, &Result.Y) != 2)
					return Result;

				return Result;
			}
			std::string IVariant::FromVector2(const Compute::Vector2& Base)
			{
				return Core::Form("v2 %f %f", Base.X, Base.Y).R();
			}

			IEvent::IEvent() : Base(nullptr), Owned(false)
			{
			}
			IEvent::IEvent(Rml::Event* Ref) : Base(Ref), Owned(false)
			{
			}
			IEvent IEvent::Copy()
			{
#ifdef ED_USE_RMLUI
				Rml::Event* Ptr = Rml::Factory::InstanceEvent(Base->GetTargetElement(), Base->GetId(), Base->GetType(), Base->GetParameters(), Base->IsInterruptible()).release();
				if (Ptr != nullptr)
				{
					Ptr->SetCurrentElement(Base->GetCurrentElement());
					Ptr->SetPhase(Base->GetPhase());
				}

				IEvent Result(Ptr);
				Result.Owned = true;
				return Result;
#else
				return *this;
#endif
			}
			void IEvent::Release()
			{
#ifdef ED_USE_RMLUI
				if (Owned)
				{
					delete Base;
					Base = nullptr;
					Owned = false;
				}
#endif
			}
			EventPhase IEvent::GetPhase() const
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT(IsValid(), EventPhase::None, "event should be valid");
				return (EventPhase)Base->GetPhase();
#else
				return EventPhase::None;
#endif
			}
			void IEvent::SetPhase(EventPhase Phase)
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT_V(IsValid(), "event should be valid");
				Base->SetPhase((Rml::EventPhase)Phase);
#endif
			}
			void IEvent::SetCurrentElement(const IElement& Element)
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT_V(IsValid(), "event should be valid");
				Base->SetCurrentElement(Element.GetElement());
#endif
			}
			IElement IEvent::GetCurrentElement() const
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT(IsValid(), nullptr, "event should be valid");
				return Base->GetCurrentElement();
#else
				return IElement();
#endif
			}
			IElement IEvent::GetTargetElement() const
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT(IsValid(), nullptr, "event should be valid");
				return Base->GetTargetElement();
#else
				return IElement();
#endif
			}
			std::string IEvent::GetType() const
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT(IsValid(), std::string(), "event should be valid");
				return Base->GetType();
#else
				return std::string();
#endif
			}
			void IEvent::StopPropagation()
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT_V(IsValid(), "event should be valid");
				Base->StopPropagation();
#endif
			}
			void IEvent::StopImmediatePropagation()
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT_V(IsValid(), "event should be valid");
				Base->StopImmediatePropagation();
#endif
			}
			bool IEvent::IsInterruptible() const
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT(IsValid(), false, "event should be valid");
				return Base->IsInterruptible();
#else
				return false;
#endif
			}
			bool IEvent::IsPropagating() const
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT(IsValid(), false, "event should be valid");
				return Base->IsPropagating();
#else
				return false;
#endif
			}
			bool IEvent::IsImmediatePropagating() const
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT(IsValid(), false, "event should be valid");
				return Base->IsImmediatePropagating();
#else
				return false;
#endif
			}
			bool IEvent::GetBoolean(const std::string& Key) const
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT(IsValid(), false, "event should be valid");
				return Base->GetParameter<bool>(Key, false);
#else
				return false;
#endif
			}
			int64_t IEvent::GetInteger(const std::string& Key) const
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT(IsValid(), 0, "event should be valid");
				return Base->GetParameter<int64_t>(Key, 0);
#else
				return 0;
#endif
			}
			double IEvent::GetNumber(const std::string& Key) const
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT(IsValid(), 0.0, "event should be valid");
				return Base->GetParameter<double>(Key, 0.0);
#else
				return 0.0;
#endif
			}
			std::string IEvent::GetString(const std::string& Key) const
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT(IsValid(), std::string(), "event should be valid");
				return Base->GetParameter<Rml::String>(Key, "");
#else
				return std::string();
#endif
			}
			Compute::Vector2 IEvent::GetVector2(const std::string& Key) const
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT(IsValid(), Compute::Vector2(), "event should be valid");
				Rml::Vector2f Result = Base->GetParameter<Rml::Vector2f>(Key, Rml::Vector2f());
				return Compute::Vector2(Result.x, Result.y);
#else
				return Compute::Vector2();
#endif
			}
			Compute::Vector3 IEvent::GetVector3(const std::string& Key) const
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT(IsValid(), Compute::Vector3(), "event should be valid");
				Rml::Vector3f Result = Base->GetParameter<Rml::Vector3f>(Key, Rml::Vector3f());
				return Compute::Vector3(Result.x, Result.y, Result.z);
#else
				return Compute::Vector3();
#endif
			}
			Compute::Vector4 IEvent::GetVector4(const std::string& Key) const
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT(IsValid(), Compute::Vector4(), "event should be valid");
				Rml::Vector4f Result = Base->GetParameter<Rml::Vector4f>(Key, Rml::Vector4f());
				return Compute::Vector4(Result.x, Result.y, Result.z, Result.w);
#else
				return Compute::Vector4();
#endif
			}
			void* IEvent::GetPointer(const std::string& Key) const
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT(IsValid(), nullptr, "event should be valid");
				return Base->GetParameter<void*>(Key, nullptr);
#else
				return nullptr;
#endif
			}
			Rml::Event* IEvent::GetEvent() const
			{
				return Base;
			}
			bool IEvent::IsValid() const
			{
				return Base != nullptr;
			}

			IElement::IElement() : Base(nullptr)
			{
			}
			IElement::IElement(Rml::Element* Ref) : Base(Ref)
			{
			}
			void IElement::Release()
			{
#ifdef ED_USE_RMLUI
				ED_DELETE(Element, Base);
				Base = nullptr;
#endif
			}
			IElement IElement::Clone() const
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT(IsValid(), nullptr, "element should be valid");
				Rml::ElementPtr Ptr = Base->Clone();
				Rml::Element* Result = Ptr.get();
				Ptr.reset();

				return Result;
#else
				return IElement();
#endif
			}
			void IElement::SetClass(const std::string& ClassName, bool Activate)
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT_V(IsValid(), "element should be valid");
				Base->SetClass(ClassName, Activate);
#endif
			}
			bool IElement::IsClassSet(const std::string& ClassName) const
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT(IsValid(), false, "element should be valid");
				return Base->IsClassSet(ClassName);
#else
				return false;
#endif
			}
			void IElement::SetClassNames(const std::string& ClassNames)
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT_V(IsValid(), "element should be valid");
				Base->SetClassNames(ClassNames);
#endif
			}
			std::string IElement::GetClassNames() const
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT(IsValid(), std::string(), "element should be valid");
				return Base->GetClassNames();
#else
				return std::string();
#endif

			}
			std::string IElement::GetAddress(bool IncludePseudoClasses, bool IncludeParents) const
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT(IsValid(), std::string(), "element should be valid");
				return Base->GetAddress(IncludePseudoClasses, IncludeParents);
#else
				return std::string();
#endif

			}
			void IElement::SetOffset(const Compute::Vector2& Offset, const IElement& OffsetParent, bool OffsetFixed)
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT_V(IsValid(), "element should be valid");
				Base->SetOffset(Rml::Vector2f(Offset.X, Offset.Y), OffsetParent.GetElement(), OffsetFixed);
#endif
			}
			Compute::Vector2 IElement::GetRelativeOffset(Area Type)
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT(IsValid(), Compute::Vector2(), "element should be valid");
				Rml::Vector2f Result = Base->GetRelativeOffset((Rml::Box::Area)Type);
				return Compute::Vector2(Result.x, Result.y);
#else
				return Compute::Vector2();
#endif

			}
			Compute::Vector2 IElement::GetAbsoluteOffset(Area Type)
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT(IsValid(), Compute::Vector2(), "element should be valid");
				Rml::Vector2f Result = Base->GetAbsoluteOffset((Rml::Box::Area)Type);
				return Compute::Vector2(Result.x, Result.y);
#else
				return Compute::Vector2();
#endif

			}
			void IElement::SetClientArea(Area ClientArea)
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT_V(IsValid(), "element should be valid");
				Base->SetClientArea((Rml::Box::Area)ClientArea);
#endif
			}
			Area IElement::GetClientArea() const
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT(IsValid(), Area::Content, "element should be valid");
				return (Area)Base->GetClientArea();
#else
				return Area::Margin;
#endif

			}
			void IElement::SetContentBox(const Compute::Vector2& ContentOffset, const Compute::Vector2& ContentBox)
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT_V(IsValid(), "element should be valid");
				Base->SetContentBox(Rml::Vector2f(ContentOffset.X, ContentOffset.Y), Rml::Vector2f(ContentBox.X, ContentBox.Y));
#endif
			}
			float IElement::GetBaseline() const
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT(IsValid(), 0.0f, "element should be valid");
				return Base->GetBaseline();
#else
				return 0.0f;
#endif

			}
			bool IElement::GetIntrinsicDimensions(Compute::Vector2& Dimensions, float& Ratio)
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT(IsValid(), false, "element should be valid");
				Rml::Vector2f Size;
				bool Result = Base->GetIntrinsicDimensions(Size, Ratio);
				Dimensions = Compute::Vector2(Size.x, Size.y);

				return Result;
#else
				return false;
#endif

			}
			bool IElement::IsPointWithinElement(const Compute::Vector2& Point)
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT(IsValid(), false, "element should be valid");
				return Base->IsPointWithinElement(Rml::Vector2f(Point.X, Point.Y));
#else
				return false;
#endif

			}
			bool IElement::IsVisible() const
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT(IsValid(), false, "element should be valid");
				return Base->IsVisible();
#else
				return false;
#endif

			}
			float IElement::GetZIndex() const
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT(IsValid(), 0.0f, "element should be valid");
				return Base->GetZIndex();
#else
				return 0.0f;
#endif

			}
			bool IElement::SetProperty(const std::string& Name, const std::string& Value)
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT(IsValid(), false, "element should be valid");
				return Base->SetProperty(Name, Value);
#else
				return false;
#endif

			}
			void IElement::RemoveProperty(const std::string& Name)
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT_V(IsValid(), "element should be valid");
				Base->RemoveProperty(Name);
#endif
			}
			std::string IElement::GetProperty(const std::string& Name)
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT(IsValid(), std::string(), "element should be valid");
				const Rml::Property* Result = Base->GetProperty(Name);
				if (!Result)
					return "";

				return Result->ToString();
#else
				return std::string();
#endif
			}
			std::string IElement::GetLocalProperty(const std::string& Name)
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT(IsValid(), std::string(), "element should be valid");
				const Rml::Property* Result = Base->GetLocalProperty(Name);
				if (!Result)
					return "";

				return Result->ToString();
#else
				return std::string();
#endif
			}
			float IElement::ResolveNumericProperty(const std::string& PropertyName)
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT(IsValid(), 0.0f, "element should be valid");
				return Base->ResolveNumericProperty(PropertyName);
#else
				return 0.0f;
#endif
			}
			Compute::Vector2 IElement::GetContainingBlock()
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT(IsValid(), Compute::Vector2(), "element should be valid");
				Rml::Vector2f Result = Base->GetContainingBlock();
				return Compute::Vector2(Result.x, Result.y);
#else
				return Compute::Vector2();
#endif
			}
			Position IElement::GetPosition()
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT(IsValid(), Position::Static, "element should be valid");
				return (Position)Base->GetPosition();
#else
				return Position::Static;
#endif
			}
			Float IElement::GetFloat()
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT(IsValid(), Float::None, "element should be valid");
				return (Float)Base->GetFloat();
#else
				return Float::None;
#endif
			}
			Display IElement::GetDisplay()
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT(IsValid(), Display::None, "element should be valid");
				return (Display)Base->GetDisplay();
#else
				return Display::None;
#endif
			}
			float IElement::GetLineHeight()
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT(IsValid(), 0.0f, "element should be valid");
				return Base->GetLineHeight();
#else
				return 0.0f;
#endif
			}
			bool IElement::Project(Compute::Vector2& Point) const noexcept
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT(IsValid(), false, "element should be valid");
				Rml::Vector2f Offset(Point.X, Point.Y);
				bool Result = Base->Project(Offset);
				Point = Compute::Vector2(Offset.x, Offset.y);

				return Result;
#else
				return false;
#endif
			}
			bool IElement::Animate(const std::string& PropertyName, const std::string& TargetValue, float Duration, TimingFunc Func, TimingDir Dir, int NumIterations, bool AlternateDirection, float Delay)
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT(IsValid(), false, "element should be valid");
				return Base->Animate(PropertyName, Rml::Property(TargetValue, Rml::Property::TRANSFORM), Duration, Rml::Tween((Rml::Tween::Type)Func, (Rml::Tween::Direction)Dir), NumIterations, AlternateDirection, Delay);
#else
				return false;
#endif
			}
			bool IElement::AddAnimationKey(const std::string& PropertyName, const std::string& TargetValue, float Duration, TimingFunc Func, TimingDir Dir)
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT(IsValid(), false, "element should be valid");
				return Base->AddAnimationKey(PropertyName, Rml::Property(TargetValue, Rml::Property::TRANSFORM), Duration, Rml::Tween((Rml::Tween::Type)Func, (Rml::Tween::Direction)Dir));
#else
				return false;
#endif
			}
			void IElement::SetPseudoClass(const std::string& PseudoClass, bool Activate)
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT_V(IsValid(), "element should be valid");
				Base->SetPseudoClass(PseudoClass, Activate);
#endif
			}
			bool IElement::IsPseudoClassSet(const std::string& PseudoClass) const
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT(IsValid(), false, "element should be valid");
				return Base->IsPseudoClassSet(PseudoClass);
#else
				return false;
#endif
			}
			void IElement::SetAttribute(const std::string& Name, const std::string& Value)
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT_V(IsValid(), "element should be valid");
				Base->SetAttribute(Name, Value);
#endif
			}
			std::string IElement::GetAttribute(const std::string& Name)
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT(IsValid(), std::string(), "element should be valid");
				return Base->GetAttribute<std::string>(Name, "");
#else
				return std::string();
#endif
			}
			bool IElement::HasAttribute(const std::string& Name) const
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT(IsValid(), false, "element should be valid");
				return Base->HasAttribute(Name);
#else
				return false;
#endif
			}
			void IElement::RemoveAttribute(const std::string& Name)
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT_V(IsValid(), "element should be valid");
				Base->RemoveAttribute(Name);
#endif
			}
			IElement IElement::GetFocusLeafNode()
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT(IsValid(), nullptr, "element should be valid");
				return Base->GetFocusLeafNode();
#else
				return IElement();
#endif
			}
			std::string IElement::GetTagName() const
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT(IsValid(), std::string(), "element should be valid");
				return Base->GetTagName();
#else
				return std::string();
#endif
			}
			std::string IElement::GetId() const
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT(IsValid(), std::string(), "element should be valid");
				return Base->GetId();
#else
				return std::string();
#endif
			}
			void IElement::SetId(const std::string& Id)
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT_V(IsValid(), "element should be valid");
				Base->SetId(Id);
#endif
			}
			float IElement::GetAbsoluteLeft()
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT(IsValid(), 0.0f, "element should be valid");
				return Base->GetAbsoluteLeft();
#else
				return 0.0f;
#endif
			}
			float IElement::GetAbsoluteTop()
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT(IsValid(), 0.0f, "element should be valid");
				return Base->GetAbsoluteTop();
#else
				return 0.0f;
#endif
			}
			float IElement::GetClientLeft()
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT(IsValid(), 0.0f, "element should be valid");
				return Base->GetClientLeft();
#else
				return 0.0f;
#endif
			}
			float IElement::GetClientTop()
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT(IsValid(), 0.0f, "element should be valid");
				return Base->GetClientTop();
#else
				return 0.0f;
#endif
			}
			float IElement::GetClientWidth()
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT(IsValid(), 0.0f, "element should be valid");
				return Base->GetClientWidth();
#else
				return 0.0f;
#endif
			}
			float IElement::GetClientHeight()
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT(IsValid(), 0.0f, "element should be valid");
				return Base->GetClientHeight();
#else
				return 0.0f;
#endif
			}
			IElement IElement::GetOffsetParent()
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT(IsValid(), nullptr, "element should be valid");
				return Base->GetOffsetParent();
#else
				return IElement();
#endif
			}
			float IElement::GetOffsetLeft()
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT(IsValid(), 0.0f, "element should be valid");
				return Base->GetOffsetLeft();
#else
				return 0.0f;
#endif
			}
			float IElement::GetOffsetTop()
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT(IsValid(), 0.0f, "element should be valid");
				return Base->GetOffsetTop();
#else
				return 0.0f;
#endif
			}
			float IElement::GetOffsetWidth()
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT(IsValid(), 0.0f, "element should be valid");
				return Base->GetOffsetWidth();
#else
				return 0.0f;
#endif
			}
			float IElement::GetOffsetHeight()
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT(IsValid(), 0.0f, "element should be valid");
				return Base->GetOffsetHeight();
#else
				return 0.0f;
#endif
			}
			float IElement::GetScrollLeft()
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT(IsValid(), 0.0f, "element should be valid");
				return Base->GetScrollLeft();
#else
				return 0.0f;
#endif
			}
			void IElement::SetScrollLeft(float ScrollLeft)
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT_V(IsValid(), "element should be valid");
				Base->SetScrollLeft(ScrollLeft);
#endif
			}
			float IElement::GetScrollTop()
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT(IsValid(), 0.0f, "element should be valid");
				return Base->GetScrollTop();
#else
				return 0.0f;
#endif
			}
			void IElement::SetScrollTop(float ScrollTop)
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT_V(IsValid(), "element should be valid");
				Base->SetScrollTop(ScrollTop);
#endif
			}
			float IElement::GetScrollWidth()
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT(IsValid(), 0.0f, "element should be valid");
				return Base->GetScrollWidth();
#else
				return 0.0f;
#endif
			}
			float IElement::GetScrollHeight()
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT(IsValid(), 0.0f, "element should be valid");
				return Base->GetScrollHeight();
#else
				return 0.0f;
#endif
			}
			IElementDocument IElement::GetOwnerDocument() const
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT(IsValid(), nullptr, "element should be valid");
				return Base->GetOwnerDocument();
#else
				return IElementDocument();
#endif
			}
			IElement IElement::GetParentNode() const
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT(IsValid(), nullptr, "element should be valid");
				return Base->GetParentNode();
#else
				return IElement();
#endif
			}
			IElement IElement::GetNextSibling() const
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT(IsValid(), nullptr, "element should be valid");
				return Base->GetNextSibling();
#else
				return IElement();
#endif
			}
			IElement IElement::GetPreviousSibling() const
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT(IsValid(), nullptr, "element should be valid");
				return Base->GetPreviousSibling();
#else
				return IElement();
#endif
			}
			IElement IElement::GetFirstChild() const
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT(IsValid(), nullptr, "element should be valid");
				return Base->GetFirstChild();
#else
				return IElement();
#endif
			}
			IElement IElement::GetLastChild() const
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT(IsValid(), nullptr, "element should be valid");
				return Base->GetLastChild();
#else
				return IElement();
#endif
			}
			IElement IElement::GetChild(int Index) const
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT(IsValid(), nullptr, "element should be valid");
				return Base->GetChild(Index);
#else
				return IElement();
#endif
			}
			int IElement::GetNumChildren(bool IncludeNonDOMElements) const
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT(IsValid(), 0, "element should be valid");
				return Base->GetNumChildren(IncludeNonDOMElements);
#else
				return 0;
#endif
			}
			void IElement::GetInnerHTML(std::string& Content) const
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT_V(IsValid(), "element should be valid");
				Base->GetInnerRML(Content);
#endif
			}
			std::string IElement::GetInnerHTML() const
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT(IsValid(), std::string(), "element should be valid");
				return Base->GetInnerRML();
#else
				return std::string();
#endif
			}
			void IElement::SetInnerHTML(const std::string& HTML)
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT_V(IsValid(), "element should be valid");
				Base->SetInnerRML(HTML);
#endif
			}
			bool IElement::IsFocused()
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT(IsValid(), false, "element should be valid");
				return Base->IsPseudoClassSet("focus");
#else
				return false;
#endif
			}
			bool IElement::IsHovered()
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT(IsValid(), false, "element should be valid");
				return Base->IsPseudoClassSet("hover");
#else
				return false;
#endif
			}
			bool IElement::IsActive()
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT(IsValid(), false, "element should be valid");
				return Base->IsPseudoClassSet("active");
#else
				return false;
#endif
			}
			bool IElement::IsChecked()
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT(IsValid(), false, "element should be valid");
				return Base->IsPseudoClassSet("checked");
#else
				return false;
#endif
			}
			bool IElement::Focus()
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT(IsValid(), false, "element should be valid");
				return Base->Focus();
#else
				return false;
#endif
			}
			void IElement::Blur()
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT_V(IsValid(), "element should be valid");
				Base->Blur();
#endif
			}
			void IElement::Click()
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT_V(IsValid(), "element should be valid");
				Base->Click();
#endif
			}
			void IElement::AddEventListener(const std::string& Event, Listener* Source, bool InCapturePhase)
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT_V(IsValid(), "element should be valid");
				ED_ASSERT_V(Source != nullptr && Source->Base != nullptr, "listener should be set");

				Base->AddEventListener(Event, Source->Base, InCapturePhase);
#endif
			}
			void IElement::RemoveEventListener(const std::string& Event, Listener* Source, bool InCapturePhase)
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT_V(IsValid(), "element should be valid");
				ED_ASSERT_V(Source != nullptr && Source->Base != nullptr, "listener should be set");

				Base->RemoveEventListener(Event, Source->Base, InCapturePhase);
#endif
			}
			bool IElement::DispatchEvent(const std::string& Type, const Core::VariantArgs& Args)
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT(IsValid(), false, "element should be valid");
				Rml::Dictionary Props;
				for (auto& Item : Args)
				{
					Rml::Variant& Prop = Props[Item.first];
					IVariant::Revert((Core::Variant*)&Item.second, &Prop);
				}

				return Base->DispatchEvent(Type, Props);
#else
				return false;
#endif
			}
			void IElement::ScrollIntoView(bool AlignWithTop)
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT_V(IsValid(), "element should be valid");
				Base->ScrollIntoView(AlignWithTop);
#endif
			}
			IElement IElement::AppendChild(const IElement& Element, bool DOMElement)
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT(IsValid(), nullptr, "element should be valid");
				return Base->AppendChild(Rml::ElementPtr(Element.GetElement()), DOMElement);
#else
				return IElement();
#endif
			}
			IElement IElement::InsertBefore(const IElement& Element, const IElement& AdjacentElement)
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT(IsValid(), nullptr, "element should be valid");
				return Base->InsertBefore(Rml::ElementPtr(Element.GetElement()), AdjacentElement.GetElement());
#else
				return IElement();
#endif
			}
			IElement IElement::ReplaceChild(const IElement& InsertedElement, const IElement& ReplacedElement)
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT(IsValid(), nullptr, "element should be valid");
				Rml::ElementPtr Ptr = Base->ReplaceChild(Rml::ElementPtr(InsertedElement.GetElement()), ReplacedElement.GetElement());
				Rml::Element* Result = Ptr.get();
				Ptr.reset();

				return Result;
#else
				return IElement();
#endif
			}
			IElement IElement::RemoveChild(const IElement& Element)
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT(IsValid(), nullptr, "element should be valid");
				Rml::ElementPtr Ptr = Base->RemoveChild(Element.GetElement());
				Rml::Element* Result = Ptr.get();
				Ptr.reset();

				return Result;
#else
				return IElement();
#endif
			}
			bool IElement::HasChildNodes() const
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT(IsValid(), false, "element should be valid");
				return Base->HasChildNodes();
#else
				return false;
#endif
			}
			IElement IElement::GetElementById(const std::string& Id)
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT(IsValid(), nullptr, "element should be valid");
				return Base->GetElementById(Id);
#else
				return IElement();
#endif
			}
			IElement IElement::QuerySelector(const std::string& Selector)
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT(IsValid(), nullptr, "element should be valid");
				return Base->QuerySelector(Selector);
#else
				return IElement();
#endif
			}
			std::vector<IElement> IElement::QuerySelectorAll(const std::string& Selectors)
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT(IsValid(), std::vector<IElement>(), "element should be valid");
				Rml::ElementList Elements;
				Base->QuerySelectorAll(Elements, Selectors);

				std::vector<IElement> Result;
				Result.reserve(Elements.size());

				for (auto& Item : Elements)
					Result.push_back(Item);

				return Result;
#else
				return std::vector<IElement>();
#endif
			}
			bool IElement::CastFormColor(Compute::Vector4* Ptr, bool Alpha)
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT(IsValid(), false, "element should be valid");
				ED_ASSERT(Ptr != nullptr, false, "ptr should be set");

				Rml::ElementFormControl* Form = (Rml::ElementFormControl*)Base;
				std::string Value = Form->GetValue();
				Compute::Vector4 Color = (Alpha ? IVariant::ToColor4(Value) : IVariant::ToColor3(Value));

				if (Alpha)
				{
					if (Value[0] == '#')
					{
						if (Value.size() > 9)
							Form->SetValue(Value.substr(0, 9));
					}
					else if (Value.size() > 15)
						Form->SetValue(Value.substr(0, 15));
				}
				else
				{
					if (Value[0] == '#')
					{
						if (Value.size() > 7)
							Form->SetValue(Value.substr(0, 7));
					}
					else if (Value.size() > 11)
						Form->SetValue(Value.substr(0, 11));
				}

				if (Color == *Ptr)
				{
					if (!Value.empty() || Form->IsPseudoClassSet("focus"))
						return false;

					if (Alpha)
						Form->SetValue(IVariant::FromColor4(*Ptr, true));
					else
						Form->SetValue(IVariant::FromColor3(*Ptr, true));

					return true;
				}

				if (Form->IsPseudoClassSet("focus"))
				{
					if (!Alpha)
						*Ptr = Compute::Vector4(Color.X, Color.Y, Color.Z, Ptr->W);
					else
						*Ptr = Color;

					return true;
				}

				if (Alpha)
					Form->SetValue(IVariant::FromColor4(*Ptr, Value.empty() ? true : Value[0] == '#'));
				else
					Form->SetValue(IVariant::FromColor3(*Ptr, Value.empty() ? true : Value[0] == '#'));

				return false;
#else
				return false;
#endif
			}
			bool IElement::CastFormString(std::string* Ptr)
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT(IsValid(), false, "element should be valid");
				ED_ASSERT(Ptr != nullptr, false, "ptr should be set");

				Rml::ElementFormControl* Form = (Rml::ElementFormControl*)Base;
				std::string Value = Form->GetValue();
				if (Value == *Ptr)
					return false;

				if (Form->IsPseudoClassSet("focus"))
				{
					*Ptr = std::move(Value);
					return true;
				}

				Form->SetValue(*Ptr);
				return false;
#else
				return false;
#endif
			}
			bool IElement::CastFormPointer(void** Ptr)
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT(IsValid(), false, "element should be valid");
				ED_ASSERT(Ptr != nullptr, false, "ptr should be set");

				Rml::ElementFormControl* Form = (Rml::ElementFormControl*)Base;
				void* Value = ToPointer(Form->GetValue());
				if (Value == *Ptr)
					return false;

				if (Form->IsPseudoClassSet("focus"))
				{
					*Ptr = Value;
					return true;
				}

				Form->SetValue(FromPointer(*Ptr));
				return false;
#else
				return false;
#endif
			}
			bool IElement::CastFormInt32(int32_t* Ptr)
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT(IsValid(), false, "element should be valid");
				ED_ASSERT(Ptr != nullptr, false, "ptr should be set");

				Rml::ElementFormControl* Form = (Rml::ElementFormControl*)Base;
				Core::Parser Value(Form->GetValue());
				if (Value.Empty())
				{
					if (Form->IsPseudoClassSet("focus"))
						return false;

					Form->SetValue(std::to_string(*Ptr));
					return false;
				}

				if (!Value.HasInteger())
				{
					Value.ReplaceNotOf(".-0123456789", "");
					Form->SetValue(Value.R());
				}

				int32_t N = Value.ToInt();
				if (N == *Ptr)
					return false;

				if (Form->IsPseudoClassSet("focus"))
				{
					*Ptr = N;
					return true;
				}

				Form->SetValue(std::to_string(*Ptr));
				return false;
#else
				return false;
#endif
			}
			bool IElement::CastFormUInt32(uint32_t* Ptr)
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT(IsValid(), false, "element should be valid");
				ED_ASSERT(Ptr != nullptr, false, "ptr should be set");

				Rml::ElementFormControl* Form = (Rml::ElementFormControl*)Base;
				Core::Parser Value(Form->GetValue());
				if (Value.Empty())
				{
					if (Form->IsPseudoClassSet("focus"))
						return false;

					Form->SetValue(std::to_string(*Ptr));
					return false;
				}

				if (!Value.HasInteger())
				{
					Value.ReplaceNotOf(".0123456789", "");
					Form->SetValue(Value.R());
				}

				uint32_t N = Value.ToUInt();
				if (N == *Ptr)
					return false;

				if (Form->IsPseudoClassSet("focus"))
				{
					*Ptr = N;
					return true;
				}

				Form->SetValue(std::to_string(*Ptr));
				return false;
#else
				return false;
#endif
			}
			bool IElement::CastFormFlag32(uint32_t* Ptr, uint32_t Mask)
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT(IsValid(), false, "element should be valid");
				ED_ASSERT(Ptr != nullptr, false, "ptr should be set");

				bool Value = (*Ptr & Mask);
				if (!CastFormBoolean(&Value))
					return false;

				if (Value)
					*Ptr |= Mask;
				else
					*Ptr &= ~Mask;

				return true;
#else
				return false;
#endif
			}
			bool IElement::CastFormInt64(int64_t* Ptr)
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT(IsValid(), false, "element should be valid");
				ED_ASSERT(Ptr != nullptr, false, "ptr should be set");

				Rml::ElementFormControl* Form = (Rml::ElementFormControl*)Base;
				Core::Parser Value(Form->GetValue());
				if (Value.Empty())
				{
					if (Form->IsPseudoClassSet("focus"))
						return false;

					Form->SetValue(std::to_string(*Ptr));
					return false;
				}

				if (!Value.HasInteger())
				{
					Value.ReplaceNotOf(".-0123456789", "");
					Form->SetValue(Value.R());
				}

				int64_t N = Value.ToInt64();
				if (N == *Ptr)
					return false;

				if (Form->IsPseudoClassSet("focus"))
				{
					*Ptr = N;
					return true;
				}

				Form->SetValue(std::to_string(*Ptr));
				return false;
#else
				return false;
#endif
			}
			bool IElement::CastFormUInt64(uint64_t* Ptr)
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT(IsValid(), false, "element should be valid");
				ED_ASSERT(Ptr != nullptr, false, "ptr should be set");

				Rml::ElementFormControl* Form = (Rml::ElementFormControl*)Base;
				Core::Parser Value(Form->GetValue());
				if (Value.Empty())
				{
					if (Form->IsPseudoClassSet("focus"))
						return false;

					Form->SetValue(std::to_string(*Ptr));
					return false;
				}

				if (!Value.HasInteger())
				{
					Value.ReplaceNotOf(".0123456789", "");
					Form->SetValue(Value.R());
				}

				uint64_t N = Value.ToUInt64();
				if (N == *Ptr)
					return false;

				if (Form->IsPseudoClassSet("focus"))
				{
					*Ptr = N;
					return true;
				}

				Form->SetValue(std::to_string(*Ptr));
				return false;
#else
				return false;
#endif
			}
			bool IElement::CastFormSize(size_t* Ptr)
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT(IsValid(), false, "element should be valid");
				ED_ASSERT(Ptr != nullptr, false, "ptr should be set");

				Rml::ElementFormControl* Form = (Rml::ElementFormControl*)Base;
				Core::Parser Value(Form->GetValue());
				if (Value.Empty())
				{
					if (Form->IsPseudoClassSet("focus"))
						return false;

					Form->SetValue(std::to_string(*Ptr));
					return false;
				}

				if (!Value.HasInteger())
				{
					Value.ReplaceNotOf(".0123456789", "");
					Form->SetValue(Value.R());
				}

				uint64_t N = Value.ToUInt64();
				if (N == (uint64_t)*Ptr)
					return false;

				if (Form->IsPseudoClassSet("focus"))
				{
					*Ptr = (size_t)N;
					return true;
				}

				Form->SetValue(std::to_string(*Ptr));
				return false;
#else
				return false;
#endif
			}
			bool IElement::CastFormFlag64(uint64_t* Ptr, uint64_t Mask)
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT(IsValid(), false, "element should be valid");
				ED_ASSERT(Ptr != nullptr, false, "ptr should be set");

				bool Value = (*Ptr & Mask);
				if (!CastFormBoolean(&Value))
					return false;

				if (Value)
					*Ptr |= Mask;
				else
					*Ptr &= ~Mask;

				return true;
#else
				return false;
#endif
			}
			bool IElement::CastFormFloat(float* Ptr)
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT(IsValid(), false, "element should be valid");
				ED_ASSERT(Ptr != nullptr, false, "ptr should be set");

				Rml::ElementFormControl* Form = (Rml::ElementFormControl*)Base;
				Core::Parser Value(Form->GetValue());
				if (Value.Empty())
				{
					if (Form->IsPseudoClassSet("focus"))
						return false;

					Form->SetValue(Core::Parser::ToString(*Ptr));
					return false;
				}

				if (!Value.HasNumber())
				{
					Value.ReplaceNotOf(".-0123456789", "");
					Form->SetValue(Value.R());
				}

				float N = Value.ToFloat();
				if (N == *Ptr)
					return false;

				if (Form->IsPseudoClassSet("focus"))
				{
					*Ptr = N;
					return true;
				}

				Form->SetValue(Core::Parser::ToString(*Ptr));
				return false;
#else
				return false;
#endif
			}
			bool IElement::CastFormFloat(float* Ptr, float Mult)
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT(IsValid(), false, "element should be valid");
				ED_ASSERT(Ptr != nullptr, false, "ptr should be set");

				*Ptr *= Mult;
				bool Result = CastFormFloat(Ptr);
				*Ptr /= Mult;

				return Result;
#else
				return false;
#endif
			}
			bool IElement::CastFormDouble(double* Ptr)
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT(IsValid(), false, "element should be valid");
				ED_ASSERT(Ptr != nullptr, false, "ptr should be set");

				Rml::ElementFormControl* Form = (Rml::ElementFormControl*)Base;
				Core::Parser Value(Form->GetValue());
				if (Value.Empty())
				{
					if (Form->IsPseudoClassSet("focus"))
						return false;

					Form->SetValue(Core::Parser::ToString(*Ptr));
					return false;
				}

				if (!Value.HasNumber())
				{
					Value.ReplaceNotOf(".-0123456789", "");
					Form->SetValue(Value.R());
				}

				double N = Value.ToDouble();
				if (N == *Ptr)
					return false;

				if (Form->IsPseudoClassSet("focus"))
				{
					*Ptr = N;
					return true;
				}

				Form->SetValue(Core::Parser::ToString(*Ptr));
				return false;
#else
				return false;
#endif
			}
			bool IElement::CastFormBoolean(bool* Ptr)
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT(IsValid(), false, "element should be valid");
				ED_ASSERT(Ptr != nullptr, false, "ptr should be set");

				Rml::ElementFormControl* Form = (Rml::ElementFormControl*)Base;
				bool B = Form->HasAttribute("checked");
				if (B == *Ptr)
					return false;

				if (Form->IsPseudoClassSet("focus"))
				{
					*Ptr = B;
					return true;
				}

				if (*Ptr)
					Form->SetAttribute<bool>("checked", true);
				else
					Form->RemoveAttribute("checked");

				return false;
#else
				return false;
#endif
			}
			std::string IElement::GetFormName() const
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT(IsValid(), std::string(), "element should be valid");
				Rml::ElementFormControl* Form = (Rml::ElementFormControl*)Base;
				return Form->GetName();
#else
				return std::string();
#endif
			}
			void IElement::SetFormName(const std::string& Name)
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT_V(IsValid(), "element should be valid");
				Rml::ElementFormControl* Form = (Rml::ElementFormControl*)Base;
				Form->SetName(Name);
#endif
			}
			std::string IElement::GetFormValue() const
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT(IsValid(), std::string(), "element should be valid");
				Rml::ElementFormControl* Form = (Rml::ElementFormControl*)Base;
				return Form->GetValue();
#else
				return std::string();
#endif
			}
			void IElement::SetFormValue(const std::string& Value)
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT_V(IsValid(), "element should be valid");
				Rml::ElementFormControl* Form = (Rml::ElementFormControl*)Base;
				Form->SetValue(Value);
#endif
			}
			bool IElement::IsFormDisabled() const
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT(IsValid(), false, "element should be valid");
				Rml::ElementFormControl* Form = (Rml::ElementFormControl*)Base;
				return Form->IsDisabled();
#else
				return false;
#endif
			}
			void IElement::SetFormDisabled(bool Disable)
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT_V(IsValid(), "element should be valid");
				Rml::ElementFormControl* Form = (Rml::ElementFormControl*)Base;
				Form->SetDisabled(Disable);
#endif
			}
			Rml::Element* IElement::GetElement() const
			{
				return Base;
			}
			bool IElement::IsValid() const
			{
				return Base != nullptr;
			}
			std::string IElement::FromPointer(void* Ptr)
			{
				if (!Ptr)
					return "0";

				return std::to_string((intptr_t)(void*)Ptr);
			}
			void* IElement::ToPointer(const std::string& Value)
			{
				if (Value.empty())
					return nullptr;

				Core::Parser Buffer(&Value);
				if (!Buffer.HasInteger())
					return nullptr;

				return (void*)(intptr_t)Buffer.ToInt64();
			}

			IElementDocument::IElementDocument() : IElement()
			{
			}
			IElementDocument::IElementDocument(Rml::ElementDocument* Ref) : IElement((Rml::Element*)Ref)
			{
			}
			void IElementDocument::Release()
			{
#ifdef ED_USE_RMLUI
				Rml::ElementDocument* Item = (Rml::ElementDocument*)Base;
				ED_DELETE(ElementDocument, Item);
				Base = nullptr;
#endif
			}
			void IElementDocument::SetTitle(const std::string& Title)
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT_V(IsValid(), "element should be valid");
				((Rml::ElementDocument*)Base)->SetTitle(Title);
#endif
			}
			void IElementDocument::PullToFront()
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT_V(IsValid(), "element should be valid");
				((Rml::ElementDocument*)Base)->PullToFront();
#endif
			}
			void IElementDocument::PushToBack()
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT_V(IsValid(), "element should be valid");
				((Rml::ElementDocument*)Base)->PushToBack();
#endif
			}
			void IElementDocument::Show(ModalFlag Modal, FocusFlag Focus)
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT_V(IsValid(), "element should be valid");
				((Rml::ElementDocument*)Base)->Show((Rml::ModalFlag)Modal, (Rml::FocusFlag)Focus);
#endif
			}
			void IElementDocument::Hide()
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT_V(IsValid(), "element should be valid");
				((Rml::ElementDocument*)Base)->Hide();
#endif
			}
			void IElementDocument::Close()
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT_V(IsValid(), "element should be valid");
				((Rml::ElementDocument*)Base)->Close();
#endif
			}
			std::string IElementDocument::GetTitle() const
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT(IsValid(), std::string(), "element should be valid");
				return ((Rml::ElementDocument*)Base)->GetTitle();
#else
				return std::string();
#endif
			}
			std::string IElementDocument::GetSourceURL() const
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT(IsValid(), std::string(), "element should be valid");
				return ((Rml::ElementDocument*)Base)->GetSourceURL();
#else
				return std::string();
#endif
			}
			IElement IElementDocument::CreateElement(const std::string& Name)
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT(IsValid(), nullptr, "element should be valid");
				Rml::ElementPtr Ptr = ((Rml::ElementDocument*)Base)->CreateElement(Name);
				Rml::Element* Result = Ptr.get();
				Ptr.reset();

				return Result;
#else
				return IElement();
#endif
			}
			bool IElementDocument::IsModal() const
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT(IsValid(), false, "element should be valid");
				return ((Rml::ElementDocument*)Base)->IsModal();
#else
				return false;
#endif
			}
			Rml::ElementDocument* IElementDocument::GetElementDocument() const
			{
				return (Rml::ElementDocument*)Base;
			}

			bool Subsystem::Create()
			{
#ifdef ED_USE_RMLUI
				State++;
				if (State > 1)
					return true;

				RenderInterface = ED_NEW(RenderSubsystem);
				Rml::SetRenderInterface(RenderInterface);

				FileInterface = ED_NEW(FileSubsystem);
				Rml::SetFileInterface(FileInterface);

				SystemInterface = ED_NEW(MainSubsystem);
				Rml::SetSystemInterface(SystemInterface);

				bool Result = Rml::Initialise();

				ContextFactory = ED_NEW(ContextInstancer);
				Rml::Factory::RegisterContextInstancer(ContextFactory);

				ListenerFactory = ED_NEW(ListenerInstancer);
				Rml::Factory::RegisterEventListenerInstancer(ListenerFactory);

				DocumentFactory = ED_NEW(DocumentInstancer);
				Rml::Factory::RegisterElementInstancer("body", DocumentFactory);

				CreateElements();
				return Result;
#else
				return false;
#endif
			}
			bool Subsystem::Release()
			{
#ifdef ED_USE_RMLUI
				State--;
				if (State > 0 || State < 0)
					return State >= 0;

				Rml::Shutdown();
				if (HasDecorators)
				{
					HasDecorators = false;
					ReleaseDecorators();
				}

				ED_DELETE(MainSubsystem, SystemInterface);
				SystemInterface = nullptr;
				Rml::SetSystemInterface(nullptr);

				ED_DELETE(FileSubsystem, FileInterface);
				FileInterface = nullptr;
				Rml::SetFileInterface(nullptr);

				ED_DELETE(RenderSubsystem, RenderInterface);
				RenderInterface = nullptr;
				Rml::SetRenderInterface(nullptr);

				ED_DELETE(DocumentInstancer, DocumentFactory);
				DocumentFactory = nullptr;

				ED_DELETE(ListenerInstancer, ListenerFactory);
				ListenerFactory = nullptr;

				ED_DELETE(ContextInstancer, ContextFactory);
				ContextFactory = nullptr;

				ReleaseElements();

				ScriptInterface = nullptr;
				return true;
#else
				return false;
#endif
			}
			void Subsystem::SetMetadata(Graphics::Activity* Activity, ContentManager* Content, Core::Timer* Time)
			{
#ifdef ED_USE_RMLUI
				if (State == 0 && !Create())
					return;

				if (RenderInterface != nullptr)
				{
					RenderInterface->Attach(Content);
					if (!HasDecorators && Content && Content->GetDevice())
					{
						HasDecorators = true;
						CreateDecorators(Content->GetDevice());
					}
				}

				if (SystemInterface != nullptr)
					SystemInterface->Attach(Activity, Time);
#endif
			}
			void Subsystem::SetTranslator(const std::string& Name, const TranslationCallback& Callback)
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT_V(SystemInterface != nullptr, "system interface should be valid");
				SystemInterface->SetTranslator(Name, Callback);
#endif
			}
			void Subsystem::SetManager(Script::VMManager* Manager)
			{
				ScriptInterface = Manager;
			}
			RenderSubsystem* Subsystem::GetRenderInterface()
			{
				return RenderInterface;
			}
			FileSubsystem* Subsystem::GetFileInterface()
			{
				return FileInterface;
			}
			MainSubsystem* Subsystem::GetSystemInterface()
			{
				return SystemInterface;
			}
			Graphics::GraphicsDevice* Subsystem::GetDevice()
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT(RenderInterface != nullptr, nullptr, "render interface should be valid");
				return RenderInterface->GetDevice();
#else
				return nullptr;
#endif
			}
			Graphics::Texture2D* Subsystem::GetBackground()
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT(RenderInterface != nullptr, nullptr, "render interface should be valid");
				return RenderInterface->Background;
#else
				return nullptr;
#endif
			}
			Compute::Matrix4x4* Subsystem::GetTransform()
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT(RenderInterface != nullptr, nullptr, "render interface should be valid");
				return RenderInterface->GetTransform();
#else
				return nullptr;
#endif
			}
			Compute::Matrix4x4* Subsystem::GetProjection()
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT(RenderInterface != nullptr, nullptr, "render interface should be valid");
				return RenderInterface->GetProjection();
#else
				return nullptr;
#endif
			}
			Compute::Matrix4x4 Subsystem::ToMatrix(const void* Matrix)
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT(Matrix != nullptr, Compute::Matrix4x4::Identity(), "matrix should be set");
				const Rml::Matrix4f* NewTransform = (const Rml::Matrix4f*)Matrix;
				auto& Row11 = NewTransform->GetRow(0);
				Compute::Matrix4x4 Result;
				Result.Row[0] = Row11.x;
				Result.Row[1] = Row11.y;
				Result.Row[2] = Row11.z;
				Result.Row[3] = Row11.w;

				auto& Row22 = NewTransform->GetRow(1);
				Result.Row[4] = Row22.x;
				Result.Row[5] = Row22.y;
				Result.Row[6] = Row22.z;
				Result.Row[7] = Row22.w;

				auto& Row33 = NewTransform->GetRow(2);
				Result.Row[8] = Row33.x;
				Result.Row[9] = Row33.y;
				Result.Row[10] = Row33.z;
				Result.Row[11] = Row33.w;

				auto& Row44 = NewTransform->GetRow(3);
				Result.Row[12] = Row44.x;
				Result.Row[13] = Row44.y;
				Result.Row[14] = Row44.z;
				Result.Row[15] = Row44.w;

				return Result.Transpose();
#else
				return Compute::Matrix4x4();
#endif
			}
			std::string Subsystem::EscapeHTML(const std::string& Text)
			{
				return Core::Parser(&Text).Replace("\r\n", "&nbsp;").Replace("\n", "&nbsp;").Replace("<", "&lt;").Replace(">", "&gt;").R();
			}
			Script::VMManager* Subsystem::ScriptInterface = nullptr;
			ContextInstancer* Subsystem::ContextFactory = nullptr;
			DocumentInstancer* Subsystem::DocumentFactory = nullptr;
			ListenerInstancer* Subsystem::ListenerFactory = nullptr;
			RenderSubsystem* Subsystem::RenderInterface = nullptr;
			FileSubsystem* Subsystem::FileInterface = nullptr;
			MainSubsystem* Subsystem::SystemInterface = nullptr;
			uint64_t Subsystem::Id = 0;
			bool Subsystem::HasDecorators = false;
			int Subsystem::State = 0;

			DataModel::DataModel(Rml::DataModelConstructor* Ref) : Base(nullptr)
			{
#ifdef ED_USE_RMLUI
				if (Ref != nullptr)
					Base = ED_NEW(Rml::DataModelConstructor, *Ref);
#endif
			}
			DataModel::~DataModel()
			{
#ifdef ED_USE_RMLUI
				Detach();
				for (auto Item : Props)
					ED_DELETE(DataNode, Item.second);

				ED_DELETE(DataModelConstructor, Base);
#endif
			}
			void DataModel::SetDetachCallback(std::function<void()>&& Callback)
			{
				if (Callback)
					Callbacks.emplace_back(std::move(Callback));
			}
			DataNode* DataModel::SetProperty(const std::string& Name, const Core::Variant& Value)
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT(IsValid(), nullptr, "data node should be valid");
				DataNode* Result = GetProperty(Name);
				if (Result != nullptr)
				{
					Result->Set(Value);
					return Result;
				}

				Result = ED_NEW(DataNode, this, Name, Value);
				if (!Value.IsObject())
				{
					if (Base->BindFunc(Name, std::bind(&DataNode::GetValue, Result, std::placeholders::_1), std::bind(&DataNode::SetValue, Result, std::placeholders::_1)))
					{
						Props[Name] = Result;
						return Result;
					}
				}
				else if (Base->Bind(Name, Result))
				{
					Props[Name] = Result;
					return Result;
				}

				ED_DELETE(DataNode, Result);
				return nullptr;
#else
				return nullptr;
#endif
			}
			DataNode* DataModel::SetProperty(const std::string& Name, Core::Variant* Value)
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT(IsValid(), nullptr, "data node should be valid");
				ED_ASSERT(Value != nullptr, nullptr, "value should be set");

				DataNode* Sub = GetProperty(Name);
				if (Sub != nullptr)
				{
					Sub->Set(Value);
					return Sub;
				}

				DataNode* Result = ED_NEW(DataNode, this, Name, Value);
				if (Base->Bind(Name, Result))
				{
					Props[Name] = Result;
					return Result;
				}

				ED_DELETE(DataNode, Result);
				return nullptr;
#else
				return nullptr;
#endif
			}
			DataNode* DataModel::SetArray(const std::string& Name)
			{
				return SetProperty(Name, Core::Var::Array());
			}
			DataNode* DataModel::SetString(const std::string& Name, const std::string& Value)
			{
				return SetProperty(Name, Core::Var::String(Value));
			}
			DataNode* DataModel::SetInteger(const std::string& Name, int64_t Value)
			{
				return SetProperty(Name, Core::Var::Integer(Value));
			}
			DataNode* DataModel::SetFloat(const std::string& Name, float Value)
			{
				return SetProperty(Name, Core::Var::Number(Value));
			}
			DataNode* DataModel::SetDouble(const std::string& Name, double Value)
			{
				return SetProperty(Name, Core::Var::Number(Value));
			}
			DataNode* DataModel::SetBoolean(const std::string& Name, bool Value)
			{
				return SetProperty(Name, Core::Var::Boolean(Value));
			}
			DataNode* DataModel::SetPointer(const std::string& Name, void* Value)
			{
				return SetProperty(Name, Core::Var::Pointer(Value));
			}
			DataNode* DataModel::GetProperty(const std::string& Name)
			{
				auto It = Props.find(Name);
				if (It != Props.end())
					return It->second;

				return nullptr;
			}
			std::string DataModel::GetString(const std::string& Name)
			{
				DataNode* Result = GetProperty(Name);
				if (!Result)
					return "";

				return Result->Ref->GetBlob();
			}
			int64_t DataModel::GetInteger(const std::string& Name)
			{
				DataNode* Result = GetProperty(Name);
				if (!Result)
					return 0;

				return Result->Ref->GetInteger();
			}
			float DataModel::GetFloat(const std::string& Name)
			{
				DataNode* Result = GetProperty(Name);
				if (!Result)
					return 0.0f;

				return (float)Result->Ref->GetNumber();
			}
			double DataModel::GetDouble(const std::string& Name)
			{
				DataNode* Result = GetProperty(Name);
				if (!Result)
					return 0.0;

				return Result->Ref->GetNumber();
			}
			bool DataModel::GetBoolean(const std::string& Name)
			{
				DataNode* Result = GetProperty(Name);
				if (!Result)
					return false;

				return Result->Ref->GetBoolean();
			}
			void* DataModel::GetPointer(const std::string& Name)
			{
				DataNode* Result = GetProperty(Name);
				if (!Result)
					return nullptr;

				return Result->Ref->GetPointer();
			}
			bool DataModel::SetCallback(const std::string& Name, const DataCallback& Callback)
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT(IsValid(), false, "data node should be valid");
				ED_ASSERT(Callback, false, "callback should not be empty");

				return Base->BindEventCallback(Name, [Callback](Rml::DataModelHandle Handle, Rml::Event& Event, const Rml::VariantList& Props)
				{
					Core::VariantList Args;
					Args.resize(Props.size());

					size_t i = 0;
					for (auto& Item : Props)
						IVariant::Convert((Rml::Variant*)&Item, &Args[i++]);

					IEvent Basis(&Event);
					Callback(Basis, Args);
				});
#else
				return false;
#endif
			}
			bool DataModel::SetUnmountCallback(const ModelCallback& Callback)
			{
				OnUnmount = Callback;
				return true;
			}
			void DataModel::Change(const std::string& VariableName)
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT_V(IsValid(), "data node should be valid");
				Base->GetModelHandle().DirtyVariable(VariableName);
#endif
			}
			bool DataModel::HasChanged(const std::string& VariableName) const
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT(IsValid(), false, "data node should be valid");
				return Base->GetModelHandle().IsVariableDirty(VariableName);
#else
				return false;
#endif
			}
			void DataModel::Detach()
			{
				for (auto& Item : Callbacks)
					Item();
				Callbacks.clear();
			}
			bool DataModel::IsValid() const
			{
				return Base != nullptr;
			}
			Rml::DataModelConstructor* DataModel::Get()
			{
				return Base;
			}

			DataNode::DataNode(DataModel* Model, const std::string& TopName, const Core::Variant& Initial) noexcept : Name(TopName), Handle(Model), Order(nullptr), Depth(0), Safe(true)
			{
				Ref = ED_NEW(Core::Variant, Initial);
			}
			DataNode::DataNode(DataModel* Model, const std::string& TopName, Core::Variant* Reference) noexcept : Name(TopName), Ref(Reference), Handle(Model), Order(nullptr), Depth(0), Safe(false)
			{
			}
			DataNode::DataNode(const DataNode& Other) noexcept : Childs(Other.Childs), Name(Other.Name), Handle(Other.Handle), Order(Other.Order), Depth(0), Safe(Other.Safe)
			{
				if (Safe)
					Ref = ED_NEW(Core::Variant, *Other.Ref);
				else
					Ref = Other.Ref;
			}
			DataNode::DataNode(DataNode&& Other) noexcept : Childs(std::move(Other.Childs)), Name(std::move(Other.Name)), Ref(Other.Ref), Handle(Other.Handle), Order(Other.Order), Depth(Other.Depth), Safe(Other.Safe)
			{
				Other.Ref = nullptr;
			}
			DataNode::~DataNode()
			{
				if (Safe)
					ED_DELETE(Variant, Ref);
			}
			DataNode& DataNode::Insert(size_t Where, const Core::VariantList& Initial, std::pair<void*, size_t>* Top)
			{
				ED_ASSERT(Where <= Childs.size(), *Childs.begin(), "index outside of range");
				DataNode Result(Handle, Name, Core::Var::Array());
				if (Top != nullptr)
				{
					Result.Order = Top->first;
					Result.Depth = Top->second;
				}

				for (auto& Item : Initial)
					Result.Add(Item);

				Childs.insert(Childs.begin() + Where, std::move(Result));
				if (Top != nullptr)
					SortTree();
				else if (Handle != nullptr && !Name.empty())
					Handle->Change(Name);

				return Childs.back();
			}
			DataNode& DataNode::Insert(size_t Where, const Core::Variant& Initial, std::pair<void*, size_t>* Top)
			{
				ED_ASSERT(Where <= Childs.size(), *Childs.begin(), "index outside of range");
				DataNode Result(Handle, Name, Initial);
				if (Top != nullptr)
				{
					Result.Order = Top->first;
					Result.Depth = Top->second;
				}

				Childs.insert(Childs.begin() + Where, std::move(Result));
				if (Top != nullptr)
					SortTree();
				else if (Handle != nullptr && !Name.empty())
					Handle->Change(Name);

				return Childs.back();
			}
			DataNode& DataNode::Insert(size_t Where, Core::Variant* Reference, std::pair<void*, size_t>* Top)
			{
				ED_ASSERT(Where <= Childs.size(), *Childs.begin(), "index outside of range");
				DataNode Result(Handle, Name, Reference);
				if (Top != nullptr)
				{
					Result.Order = Top->first;
					Result.Depth = Top->second;
				}

				Childs.insert(Childs.begin() + Where, std::move(Result));
				if (Top != nullptr)
					SortTree();
				else if (Handle != nullptr && !Name.empty())
					Handle->Change(Name);

				return Childs.back();
			}
			DataNode& DataNode::Add(const Core::VariantList& Initial, std::pair<void*, size_t>* Top)
			{
				DataNode Result(Handle, Name, Core::Var::Array());
				if (Top != nullptr)
				{
					Result.Order = Top->first;
					Result.Depth = Top->second;
				}

				for (auto& Item : Initial)
					Result.Add(Item);

				Childs.emplace_back(std::move(Result));
				if (Top != nullptr)
					SortTree();
				else if (Handle != nullptr && !Name.empty())
					Handle->Change(Name);

				return Childs.back();
			}
			DataNode& DataNode::Add(const Core::Variant& Initial, std::pair<void*, size_t>* Top)
			{
				DataNode Result(Handle, Name, Initial);
				if (Top != nullptr)
				{
					Result.Order = Top->first;
					Result.Depth = Top->second;
				}

				Childs.emplace_back(std::move(Result));
				if (Top != nullptr)
					SortTree();
				else if (Handle != nullptr && !Name.empty())
					Handle->Change(Name);

				return Childs.back();
			}
			DataNode& DataNode::Add(Core::Variant* Reference, std::pair<void*, size_t>* Top)
			{
				DataNode Result(Handle, Name, Reference);
				if (Top != nullptr)
				{
					Result.Order = Top->first;
					Result.Depth = Top->second;
				}

				Childs.emplace_back(std::move(Result));
				if (Top != nullptr)
					SortTree();
				else if (Handle != nullptr && !Name.empty())
					Handle->Change(Name);

				return Childs.back();
			}
			DataNode& DataNode::At(size_t Index)
			{
				ED_ASSERT(Index < Childs.size(), *this, "index outside of range");
				return Childs[Index];
			}
			bool DataNode::Remove(size_t Index)
			{
				ED_ASSERT(Index < Childs.size(), false, "index outside of range");
				Childs.erase(Childs.begin() + Index);

				if (Handle != nullptr && !Name.empty())
					Handle->Change(Name);

				return true;
			}
			bool DataNode::Clear()
			{
				Childs.clear();
				if (Handle != nullptr && !Name.empty())
					Handle->Change(Name);

				return true;
			}
			void DataNode::SortTree()
			{
				ED_SORT(Childs.begin(), Childs.end(), [](const DataNode& A, const DataNode& B)
				{
					double D1 = (double)(uintptr_t)A.GetSeqId() + 0.00000001 * (double)A.GetDepth();
					double D2 = (double)(uintptr_t)B.GetSeqId() + 0.00000001 * (double)B.GetDepth();
					return D1 < D2;
				});

				if (Handle != nullptr && !Name.empty())
					Handle->Change(Name);
			}
			void DataNode::SetTop(void* SeqId, size_t Nesting)
			{
				Order = SeqId;
				Depth = Nesting;
			}
			void DataNode::Set(const Core::Variant& NewValue)
			{
				if (*Ref == NewValue)
					return;

				*Ref = NewValue;
				if (Handle != nullptr && !Name.empty())
					Handle->Change(Name);
			}
			void DataNode::Set(Core::Variant* NewReference)
			{
				if (!NewReference || NewReference == Ref)
					return;

				if (Safe)
					ED_DELETE(Variant, Ref);

				Ref = NewReference;
				Safe = false;

				if (Handle != nullptr && !Name.empty())
					Handle->Change(Name);
			}
			void DataNode::Replace(const Core::VariantList& Data, std::pair<void*, size_t>* Top)
			{
				Childs.clear();
				for (auto& Item : Data)
					Childs.emplace_back(DataNode(Handle, Name, Item));

				if (Top != nullptr)
				{
					Order = Top->first;
					Depth = Top->second;
				}

				if (Handle != nullptr && !Name.empty())
					Handle->Change(Name);
			}
			void DataNode::SetString(const std::string& Value)
			{
				Set(Core::Var::String(Value));
			}
			void DataNode::SetVector2(const Compute::Vector2& Value)
			{
				Set(Core::Var::String(IVariant::FromVector2(Value)));
			}
			void DataNode::SetVector3(const Compute::Vector3& Value)
			{
				Set(Core::Var::String(IVariant::FromVector3(Value)));
			}
			void DataNode::SetVector4(const Compute::Vector4& Value)
			{
				Set(Core::Var::String(IVariant::FromVector4(Value)));
			}
			void DataNode::SetInteger(int64_t Value)
			{
				Set(Core::Var::Integer(Value));
			}
			void DataNode::SetFloat(float Value)
			{
				Set(Core::Var::Number(Value));
			}
			void DataNode::SetDouble(double Value)
			{
				Set(Core::Var::Number(Value));
			}
			void DataNode::SetBoolean(bool Value)
			{
				Set(Core::Var::Boolean(Value));
			}
			void DataNode::SetPointer(void* Value)
			{
				Set(Core::Var::Pointer(Value));
			}
			size_t DataNode::GetSize() const
			{
				return Childs.size();
			}
			size_t DataNode::GetDepth() const
			{
				return Depth;
			}
			void* DataNode::GetSeqId() const
			{
				return Order;
			}
			const Core::Variant& DataNode::Get()
			{
				return *Ref;
			}
			std::string DataNode::GetString()
			{
				return Ref->GetBlob();
			}
			Compute::Vector2 DataNode::GetVector2()
			{
				return IVariant::ToVector2(Ref->GetBlob());
			}
			Compute::Vector3 DataNode::GetVector3()
			{
				return IVariant::ToVector3(Ref->GetBlob());
			}
			Compute::Vector4 DataNode::GetVector4()
			{
				return IVariant::ToVector4(Ref->GetBlob());
			}
			int64_t DataNode::GetInteger()
			{
				return Ref->GetInteger();
			}
			float DataNode::GetFloat()
			{
				return (float)Ref->GetNumber();
			}
			double DataNode::GetDouble()
			{
				return Ref->GetNumber();
			}
			bool DataNode::GetBoolean()
			{
				return Ref->GetBoolean();
			}
			void* DataNode::GetPointer()
			{
				return Ref->GetPointer();
			}
			void DataNode::GetValue(Rml::Variant& Result)
			{
				IVariant::Revert(Ref, &Result);
			}
			void DataNode::SetValue(const Rml::Variant& Result)
			{
				IVariant::Convert((Rml::Variant*)&Result, Ref);
			}
			void DataNode::SetValueStr(const std::string& Value)
			{
				*Ref = std::move(Core::Var::String(Value));
			}
			void DataNode::SetValueNum(double Value)
			{
				*Ref = std::move(Core::Var::Number(Value));
			}
			void DataNode::SetValueInt(int64_t Value)
			{
				*Ref = std::move(Core::Var::Integer(Value));
			}
			int64_t DataNode::GetValueSize()
			{
				return (int64_t)GetSize();
			}
			DataNode& DataNode::operator= (const DataNode& Other) noexcept
			{
				if (this == &Other)
					return *this;

				this->~DataNode();
				Childs = Other.Childs;
				Name = Other.Name;
				Handle = Other.Handle;
				Order = Other.Order;
				Depth = Other.Depth;
				Safe = Other.Safe;

				if (Safe)
					Ref = ED_NEW(Core::Variant, *Other.Ref);
				else
					Ref = Other.Ref;

				return *this;
			}
			DataNode& DataNode::operator= (DataNode&& Other) noexcept
			{
				if (this == &Other)
					return *this;

				this->~DataNode();
				Childs = std::move(Other.Childs);
				Name = std::move(Other.Name);
				Ref = Other.Ref;
				Handle = Other.Handle;
				Name = Other.Name;
				Order = Other.Order;
				Depth = Other.Depth;
				Safe = Other.Safe;

				Other.Ref = nullptr;
				return *this;
			}

			Listener::Listener(const EventCallback& NewCallback)
			{
#ifdef ED_USE_RMLUI
				Base = ED_NEW(EventSubsystem, NewCallback);
#endif
			}
			Listener::Listener(const std::string& FunctionName)
			{
#ifdef ED_USE_RMLUI
				Base = ED_NEW(ListenerSubsystem, FunctionName, nullptr);
#endif
			}
			Listener::~Listener()
			{
#ifdef ED_USE_RMLUI
				Base->OnDetach(nullptr);
#endif
			}

			Context::Context(const Compute::Vector2& Size) : Compiler(nullptr), Cursor(-1.0f), Loading(false)
			{
#ifdef ED_USE_RMLUI
				Base = (ScopedContext*)Rml::CreateContext(std::to_string(Subsystem::Id++), Rml::Vector2i((int)Size.X, (int)Size.Y));

				ED_ASSERT_V(Base != nullptr, "context cannot be created");
				Base->Basis = this;
				CreateVM();
#endif
			}
			Context::Context(Graphics::GraphicsDevice* Device) : Compiler(nullptr), Cursor(-1.0f), Loading(false)
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT_V(Device != nullptr, "graphics device should be set");
				ED_ASSERT_V(Device->GetRenderTarget() != nullptr, "graphics device should be set");

				Graphics::RenderTarget2D* Target = Device->GetRenderTarget();
				Base = (ScopedContext*)Rml::CreateContext(std::to_string(Subsystem::Id++), Rml::Vector2i((int)Target->GetWidth(), (int)Target->GetHeight()));

				ED_ASSERT_V(Base != nullptr, "context cannot be created");
				Base->Basis = this;
				CreateVM();
#endif
			}
			Context::~Context()
			{
#ifdef ED_USE_RMLUI
				RemoveDataModels();
				Rml::RemoveContext(Base->GetName());
				ED_RELEASE(Compiler);
#endif
			}
			void Context::EmitKey(Graphics::KeyCode Key, Graphics::KeyMod Mod, int Virtual, int Repeat, bool Pressed)
			{
#ifdef ED_USE_RMLUI
				if (Key == Graphics::KeyCode::CURSORLEFT)
				{
					if (Pressed)
					{
						if (Base->ProcessMouseButtonDown(0, GetKeyMod(Mod)))
							Inputs.Cursor = true;
					}
					else
					{
						if (Base->ProcessMouseButtonUp(0, GetKeyMod(Mod)))
							Inputs.Cursor = true;
					}
				}
				else if (Key == Graphics::KeyCode::CURSORRIGHT)
				{
					if (Pressed)
					{
						if (Base->ProcessMouseButtonDown(1, GetKeyMod(Mod)))
							Inputs.Cursor = true;
					}
					else
					{
						if (Base->ProcessMouseButtonUp(1, GetKeyMod(Mod)))
							Inputs.Cursor = true;
					}
				}
				else if (Key == Graphics::KeyCode::CURSORMIDDLE)
				{
					if (Pressed)
					{
						if (Base->ProcessMouseButtonDown(2, GetKeyMod(Mod)))
							Inputs.Cursor = true;
					}
					else
					{
						if (Base->ProcessMouseButtonUp(2, GetKeyMod(Mod)))
							Inputs.Cursor = true;
					}
				}
				else if (Pressed)
				{
					if (Base->ProcessKeyDown((Rml::Input::KeyIdentifier)GetKeyCode(Key), GetKeyMod(Mod)))
						Inputs.Keys = true;
				}
				else if (Base->ProcessKeyUp((Rml::Input::KeyIdentifier)GetKeyCode(Key), GetKeyMod(Mod)))
					Inputs.Keys = true;
#endif
			}
			void Context::EmitInput(const char* Buffer, int Length)
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT_V(Buffer != nullptr && Length > 0, "buffer should be set");
				if (Base->ProcessTextInput(std::string(Buffer, Length)))
					Inputs.Text = true;
#endif
			}
			void Context::EmitWheel(int X, int Y, bool Normal, Graphics::KeyMod Mod)
			{
#ifdef ED_USE_RMLUI
				if (Base->ProcessMouseWheel((float)-Y, GetKeyMod(Mod)))
					Inputs.Scroll = true;
#endif
			}
			void Context::EmitResize(int Width, int Height)
			{
#ifdef ED_USE_RMLUI
				RenderSubsystem* Renderer = Subsystem::GetRenderInterface();
				if (Renderer != nullptr)
					Renderer->ResizeBuffers(Width, Height);

				Base->SetDimensions(Rml::Vector2i(Width, Height));
#endif
			}
			void Context::UpdateEvents(Graphics::Activity* Activity)
			{
#ifdef ED_USE_RMLUI
				Inputs.Keys = false;
				Inputs.Text = false;
				Inputs.Scroll = false;
				Inputs.Cursor = false;

				if (Activity != nullptr)
				{
					Cursor = Activity->GetCursorPosition();
					if (Base->ProcessMouseMove((int)Cursor.X, (int)Cursor.Y, GetKeyMod(Activity->GetKeyModState())))
						Inputs.Cursor = true;
				}

				Base->Update();
#endif
			}
			void Context::RenderLists(Graphics::Texture2D* Target)
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT_V(Subsystem::GetRenderInterface() != nullptr, "render interface should be valid");
				RenderSubsystem* Renderer = Subsystem::GetRenderInterface();
				Renderer->Background = Target;
				Base->Render();
#endif
			}
			void Context::ClearStyles()
			{
#ifdef ED_USE_RMLUI
				Rml::StyleSheetFactory::ClearStyleSheetCache();
#endif
			}
			bool Context::ClearDocuments()
			{
#ifdef ED_USE_RMLUI
				bool State = Loading;
				Loading = true;

				Elements.clear();
				Base->UnloadAllDocuments();
				Loading = State;

				return true;
#else
				return false;
#endif
			}
			bool Context::Initialize(Core::Schema* Conf, const std::string& Relative)
			{
				ED_ASSERT(Conf != nullptr, false, "conf should be set");
				bool State = Loading;
				Loading = true;

				for (auto* Face : Conf->FindCollection("font-face", true))
				{
					std::string Path = Face->GetVar("[path]").GetBlob();
					if (Path.empty())
					{
						ED_ERR("[gui] path is required for font face");
						return false;
					}

					std::string Target = Core::OS::Path::Resolve(Path, Relative);
					if (!LoadFontFace(Target.empty() ? Path : Target, Face->GetAttribute("fallback") != nullptr))
					{
						Loading = State;
						return false;
					}
				}

				for (auto* Document : Conf->FindCollection("document", true))
				{
					std::string Path = Document->GetVar("[path]").GetBlob();
					if (Path.empty())
					{
						ED_ERR("[gui] path is required for document");
						return false;
					}

					std::string Target = Core::OS::Path::Resolve(Path, Relative);
					IElementDocument Result = LoadDocument(Target.empty() ? Path : Target);
					if (!Result.IsValid())
					{
						Loading = State;
						return false;
					}
					else if (Document->Has("[show]"))
						Result.Show();
				}

				Loading = State;
				return true;
			}
			bool Context::Initialize(const std::string& ConfPath)
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT(Subsystem::RenderInterface != nullptr, false, "render interface should be set");
				ED_ASSERT(Subsystem::RenderInterface->GetContent() != nullptr, false, "content manager should be set");

				bool State = Loading;
				Loading = true;

				Core::Schema* Sheet = Subsystem::RenderInterface->GetContent()->Load<Core::Schema>(ConfPath);
				if (!Sheet)
				{
					Loading = State;
					return false;
				}

				bool Result = Initialize(Sheet, Core::OS::Path::GetDirectory(ConfPath.c_str()));
				ED_RELEASE(Sheet);

				Loading = State;
				return Result;
#else
				return false;
#endif
			}
			bool Context::IsLoading()
			{
				return Loading;
			}
			bool Context::IsInputFocused()
			{
#ifdef ED_USE_RMLUI
				Rml::Element* Element = Base->GetFocusElement();
				if (!Element)
					return false;

				const Rml::String& Tag = Element->GetTagName();
				return Tag == "input" || Tag == "textarea" || Tag == "select";
#else
				return false;
#endif
			}
			bool Context::LoadFontFace(const std::string& Path, bool UseAsFallback)
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT(Subsystem::GetSystemInterface() != nullptr, false, "system interface should be set");
				bool State = Loading;
				Loading = true;

				bool Result = Subsystem::GetSystemInterface()->AddFontFace(Path, UseAsFallback);
				Loading = State;

				return Result;
#else
				return false;
#endif
			}
			std::unordered_map<std::string, bool>* Context::GetFontFaces()
			{
#ifdef ED_USE_RMLUI
				return Subsystem::GetSystemInterface()->GetFontFaces();
#else
				return nullptr;
#endif
			}
			Rml::Context* Context::GetContext()
			{
#ifdef ED_USE_RMLUI
				return Base;
#else
				return nullptr;
#endif
			}
			Compute::Vector2 Context::GetDimensions() const
			{
#ifdef ED_USE_RMLUI
				Rml::Vector2i Size = Base->GetDimensions();
				return Compute::Vector2((float)Size.x, (float)Size.y);
#else
				return Compute::Vector2();
#endif
			}
			void Context::SetDensityIndependentPixelRatio(float DensityIndependentPixelRatio)
			{
#ifdef ED_USE_RMLUI
				Base->SetDensityIndependentPixelRatio(DensityIndependentPixelRatio);
#endif
			}
			void Context::EnableMouseCursor(bool Enable)
			{
#ifdef ED_USE_RMLUI
				Base->EnableMouseCursor(Enable);
#endif
			}
			float Context::GetDensityIndependentPixelRatio() const
			{
#ifdef ED_USE_RMLUI
				return Base->GetDensityIndependentPixelRatio();
#else
				return 0.0f;
#endif
			}
			bool Context::ReplaceHTML(const std::string& Selector, const std::string& HTML, int Index)
			{
#ifdef ED_USE_RMLUI
				auto* Current = Base->GetDocument(Index);
				if (!Current)
					return false;

				auto TargetPtr = Current->QuerySelector(Selector);
				if (!TargetPtr)
					return false;

				TargetPtr->SetInnerRML(HTML);
				return true;
#else
				return false;
#endif
			}
			IElementDocument Context::EvalHTML(const std::string& HTML, int Index)
			{
#ifdef ED_USE_RMLUI
				auto* Current = Base->GetDocument(Index);
				if (!Current)
					Current = Base->LoadDocumentFromMemory("<html><body>" + HTML + "</body></html>");
				else
					Current->SetInnerRML(HTML);

				return Current;
#else
				return IElementDocument();
#endif
			}
			IElementDocument Context::AddCSS(const std::string& CSS, int Index)
			{
#ifdef ED_USE_RMLUI
				auto* Current = Base->GetDocument(Index);
				if (Current != nullptr)
				{
					auto HeadPtr = Current->QuerySelector("head");
					if (HeadPtr != nullptr)
					{
						auto StylePtr = HeadPtr->QuerySelector("style");
						if (!StylePtr)
						{
							auto Style = Current->CreateElement("style");
							Style->SetInnerRML(CSS);
							HeadPtr->AppendChild(std::move(Style));
						}
						else
							StylePtr->SetInnerRML(CSS);
					}
					else
					{
						auto Head = Current->CreateElement("head");
						{
							auto Style = Current->CreateElement("style");
							Style->SetInnerRML(CSS);
							Head->AppendChild(std::move(Style));
						}
						Current->AppendChild(std::move(Head));
					}
				}
				else
					Current = Base->LoadDocumentFromMemory("<html><head><style>" + CSS + "</style></head></html>");

				return Current;
#else
				return IElementDocument();
#endif
			}
			IElementDocument Context::LoadCSS(const std::string& Path, int Index)
			{
#ifdef ED_USE_RMLUI
				auto* Current = Base->GetDocument(Index);
				if (Current != nullptr)
				{
					auto HeadPtr = Current->QuerySelector("head");
					if (!HeadPtr)
					{
						auto Head = Current->CreateElement("head");
						HeadPtr = Current->AppendChild(std::move(Head));
					}

					auto Link = Current->CreateElement("link");
					Link->SetAttribute("type", "text/css");
					Link->SetAttribute("href", Path);
					HeadPtr = Current->AppendChild(std::move(Link));
				}
				else
					Current = Base->LoadDocumentFromMemory("<html><head><link type=\"text/css\" href=\"" + Path + "\" /></head></html>");

				return Current;
#else
				return IElementDocument();
#endif
			}
			IElementDocument Context::LoadDocument(const std::string& Path)
			{
#ifdef ED_USE_RMLUI
				bool State = Loading;
				Loading = true;

				if (OnMount)
					OnMount(this);

				ClearVM();
				Elements.clear();

				size_t Length = 0;
				unsigned char* Buffer = Core::OS::File::ReadAll(Path.c_str(), &Length);
				if (!Buffer)
				{
				ErrorState:
					Base->UnloadAllDocuments();
					Loading = State;

					return nullptr;
				}

				std::string Data((const char*)Buffer, Length);
				ED_FREE(Buffer);

				Decompose(Data);
				if (!Preprocess(Path, Data))
					goto ErrorState;

				Core::Parser URL(Path);
				URL.Replace('\\', '/');
				URL.Insert("file:///", 0);

				auto* Result = Base->LoadDocumentFromMemory(Data, URL.R());
				Loading = State;

				return Result;
#else
				return IElementDocument();
#endif
			}
			IElementDocument Context::AddDocumentEmpty(const std::string& InstancerName)
			{
#ifdef ED_USE_RMLUI
				return Base->CreateDocument(InstancerName);
#else
				return IElementDocument();
#endif
			}
			IElementDocument Context::AddDocument(const std::string& HTML)
			{
#ifdef ED_USE_RMLUI
				return Base->LoadDocumentFromMemory(HTML);
#else
				return IElementDocument();
#endif
			}
			IElementDocument Context::GetDocument(const std::string& Id)
			{
#ifdef ED_USE_RMLUI
				return Base->GetDocument(Id);
#else
				return IElementDocument();
#endif
			}
			IElementDocument Context::GetDocument(int Index)
			{
#ifdef ED_USE_RMLUI
				return Base->GetDocument(Index);
#else
				return IElementDocument();
#endif
			}
			int Context::GetNumDocuments() const
			{
#ifdef ED_USE_RMLUI
				return Base->GetNumDocuments();
#else
				return 0;
#endif
			}
			IElement Context::GetElementById(const std::string& Id, int DocIndex)
			{
#ifdef ED_USE_RMLUI
				Rml::ElementDocument* Root = Base->GetDocument(DocIndex);
				if (!Root)
					return nullptr;

				auto Map = Elements.find(DocIndex);
				if (Map == Elements.end())
				{
					Rml::Element* Element = Root->GetElementById(Id);
					if (!Element)
						return nullptr;

					Elements[DocIndex].insert(std::make_pair(Id, Element));
					return Element;
				}

				auto It = Map->second.find(Id);
				if (It != Map->second.end())
					return It->second;

				Rml::Element* Element = Root->GetElementById(Id);
				Map->second.insert(std::make_pair(Id, Element));

				return Element;
#else
				return IElement();
#endif
			}
			IElement Context::GetHoverElement()
			{
#ifdef ED_USE_RMLUI
				return Base->GetHoverElement();
#else
				return IElement();
#endif
			}
			IElement Context::GetFocusElement()
			{
#ifdef ED_USE_RMLUI
				return Base->GetFocusElement();
#else
				return IElement();
#endif
			}
			IElement Context::GetRootElement()
			{
#ifdef ED_USE_RMLUI
				return Base->GetRootElement();
#else
				return IElement();
#endif
			}
			IElement Context::GetElementAtPoint(const Compute::Vector2& Point, const IElement& IgnoreElement, const IElement& Element) const
			{
#ifdef ED_USE_RMLUI
				return Base->GetElementAtPoint(Rml::Vector2f(Point.X, Point.Y), IgnoreElement.GetElement(), Element.GetElement());
#else
				return IElement();
#endif
			}
			void Context::PullDocumentToFront(const IElementDocument& Schema)
			{
#ifdef ED_USE_RMLUI
				Base->PullDocumentToFront(Schema.GetElementDocument());
#endif
			}
			void Context::PushDocumentToBack(const IElementDocument& Schema)
			{
#ifdef ED_USE_RMLUI
				Base->PushDocumentToBack(Schema.GetElementDocument());
#endif
			}
			void Context::UnfocusDocument(const IElementDocument& Schema)
			{
#ifdef ED_USE_RMLUI
				Base->UnfocusDocument(Schema.GetElementDocument());
#endif
			}
			void Context::AddEventListener(const std::string& Event, Listener* Listener, bool InCapturePhase)
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT_V(Listener != nullptr && Listener->Base != nullptr, "listener should be valid");
				Base->AddEventListener(Event, Listener->Base, InCapturePhase);
#endif
			}
			void Context::RemoveEventListener(const std::string& Event, Listener* Listener, bool InCapturePhase)
			{
#ifdef ED_USE_RMLUI
				ED_ASSERT_V(Listener != nullptr && Listener->Base != nullptr, "listener should be valid");
				Base->RemoveEventListener(Event, Listener->Base, InCapturePhase);
#endif
			}
			bool Context::IsMouseInteracting() const
			{
#ifdef ED_USE_RMLUI
				return Base->IsMouseInteracting();
#else
				return false;
#endif
			}
			bool Context::WasInputUsed(uint32_t InputTypeMask)
			{
#ifdef ED_USE_RMLUI
				bool Result = false;
				if (InputTypeMask & (uint32_t)InputType::Keys && Inputs.Keys)
					Result = true;

				if (InputTypeMask & (uint32_t)InputType::Scroll && Inputs.Scroll)
					Result = true;

				if (InputTypeMask & (uint32_t)InputType::Text && Inputs.Text)
					Result = true;

				if (InputTypeMask & (uint32_t)InputType::Cursor && Inputs.Cursor)
					Result = true;

				return Result;
#else
				return false;
#endif
			}
			bool Context::GetActiveClipRegion(Compute::Vector2& Origin, Compute::Vector2& Dimensions) const
			{
#ifdef ED_USE_RMLUI
				Rml::Vector2i O1((int)Origin.X, (int)Origin.Y);
				Rml::Vector2i O2((int)Dimensions.X, (int)Dimensions.Y);
				bool Result = Base->GetActiveClipRegion(O1, O2);
				Origin = Compute::Vector2((float)O1.x, (float)O1.y);
				Dimensions = Compute::Vector2((float)O2.x, (float)O2.y);

				return Result;
#else
				return false;
#endif
			}
			void Context::SetActiveClipRegion(const Compute::Vector2& Origin, const Compute::Vector2& Dimensions)
			{
#ifdef ED_USE_RMLUI
				return Base->SetActiveClipRegion(Rml::Vector2i((int)Origin.X, (int)Origin.Y), Rml::Vector2i((int)Dimensions.X, (int)Dimensions.Y));
#endif
			}
			DataModel* Context::SetDataModel(const std::string& Name)
			{
#ifdef ED_USE_RMLUI
				Rml::DataModelConstructor Result = Base->CreateDataModel(Name);
				if (!Result)
					return nullptr;

				DataModel* Handle = new DataModel(&Result);
				if (auto Type = Result.RegisterStruct<DataNode>())
				{
					Result.RegisterArray<std::vector<DataNode>>();
					Type.RegisterMember("int", &DataNode::GetInteger, &DataNode::SetValueInt);
					Type.RegisterMember("num", &DataNode::GetDouble, &DataNode::SetValueNum);
					Type.RegisterMember("str", &DataNode::GetString, &DataNode::SetValueStr);
					Type.RegisterMember("at", &DataNode::Childs);
					Type.RegisterMember("all", &DataNode::Childs);
					Type.RegisterMember("size", &DataNode::GetValueSize);
				}

				Models[Name] = Handle;
				return Handle;
#else
				return nullptr;
#endif
			}
			DataModel* Context::GetDataModel(const std::string& Name)
			{
				auto It = Models.find(Name);
				if (It != Models.end())
					return It->second;

				return nullptr;
			}
			bool Context::RemoveDataModel(const std::string& Name)
			{
#ifdef ED_USE_RMLUI
				if (!Base->RemoveDataModel(Name))
					return false;

				auto It = Models.find(Name);
				if (It != Models.end())
				{
					if (It->second->OnUnmount)
						It->second->OnUnmount(this);

					ED_RELEASE(It->second);
					Models.erase(It);
				}

				return true;
#else
				return false;
#endif
			}
			bool Context::RemoveDataModels()
			{
#ifdef ED_USE_RMLUI
				if (Models.empty())
					return false;

				for (auto Item : Models)
				{
					Base->RemoveDataModel(Item.first);
					ED_RELEASE(Item.second);
				}

				Models.clear();
				return true;
#else
				return false;
#endif
			}
			void Context::SetDocumentsBaseTag(const std::string& Tag)
			{
#ifdef ED_USE_RMLUI
				Base->SetDocumentsBaseTag(Tag);
#endif
			}
			void Context::SetMountCallback(const ModelCallback& Callback)
			{
				OnMount = Callback;
			}
			bool Context::Preprocess(const std::string& Path, std::string& Buffer)
			{
				Compute::Preprocessor::Desc Features;
				Features.Conditions = false;
				Features.Defines = false;
				Features.Pragmas = false;

				Compute::IncludeDesc Desc = Compute::IncludeDesc();
				Desc.Exts.push_back(".html");
				Desc.Exts.push_back(".htm");
				Desc.Root = Core::OS::Directory::Get();

				Compute::Preprocessor* Processor = new Compute::Preprocessor();
				Processor->SetIncludeCallback([this](Compute::Preprocessor* P, const Compute::IncludeResult& File, std::string* Output)
				{
					if (File.Module.empty() || (!File.IsFile && !File.IsSystem))
						return false;

					if (File.IsSystem && !File.IsFile)
						return false;

					size_t Length;
					unsigned char* Data = Core::OS::File::ReadAll(File.Module.c_str(), &Length);
					if (!Data)
						return false;

					Output->assign((const char*)Data, Length);
					ED_FREE(Data);

					this->Decompose(*Output);
					return true;
				});
				Processor->SetIncludeOptions(Desc);
				Processor->SetFeatures(Features);

				bool Result = Processor->Process(Path, Buffer);
				ED_RELEASE(Processor);

				return Result;
			}
			void Context::Decompose(std::string& Data)
			{
				Core::Parser::Settle Result, Start, End;
				Core::Parser Buffer(&Data);

				while (Result.End < Buffer.Size())
				{
					Start = Buffer.Find("<!--", End.End);
					if (!Start.Found)
						return;

					End = Buffer.Find("-->", Start.End);
					if (!End.Found)
						return;

					Result = Buffer.Find("#include", Start.End);
					if (!Result.Found)
						return;

					if (Result.End >= End.Start)
						continue;

					Buffer.RemovePart(End.Start, End.End);
					Buffer.RemovePart(Start.Start, Start.End);
					End.End = Start.Start;
				}
			}
			void Context::CreateVM()
			{
				if (!Subsystem::ScriptInterface || Compiler != nullptr)
					return;

				Compiler = Subsystem::ScriptInterface->CreateCompiler();
				ClearVM();
			}
			void Context::ClearVM()
			{
				if (!Compiler)
					return;

				Compiler->Clear();
				Compiler->Prepare("__scope__", true);
			}
			std::string Context::GetDocumentsBaseTag()
			{
#ifdef ED_USE_RMLUI
				return Base->GetDocumentsBaseTag();
#else
				return std::string();
#endif
			}
			int Context::GetKeyCode(Graphics::KeyCode Key)
			{
#ifdef ED_USE_RMLUI
				using namespace Rml::Input;
				switch (Key)
				{
					case Graphics::KeyCode::SPACE:
						return KI_SPACE;
					case Graphics::KeyCode::D0:
						return KI_0;
					case Graphics::KeyCode::D1:
						return KI_1;
					case Graphics::KeyCode::D2:
						return KI_2;
					case Graphics::KeyCode::D3:
						return KI_3;
					case Graphics::KeyCode::D4:
						return KI_4;
					case Graphics::KeyCode::D5:
						return KI_5;
					case Graphics::KeyCode::D6:
						return KI_6;
					case Graphics::KeyCode::D7:
						return KI_7;
					case Graphics::KeyCode::D8:
						return KI_8;
					case Graphics::KeyCode::D9:
						return KI_9;
					case Graphics::KeyCode::A:
						return KI_A;
					case Graphics::KeyCode::B:
						return KI_B;
					case Graphics::KeyCode::C:
						return KI_C;
					case Graphics::KeyCode::D:
						return KI_D;
					case Graphics::KeyCode::E:
						return KI_E;
					case Graphics::KeyCode::F:
						return KI_F;
					case Graphics::KeyCode::G:
						return KI_G;
					case Graphics::KeyCode::H:
						return KI_H;
					case Graphics::KeyCode::I:
						return KI_I;
					case Graphics::KeyCode::J:
						return KI_J;
					case Graphics::KeyCode::K:
						return KI_K;
					case Graphics::KeyCode::L:
						return KI_L;
					case Graphics::KeyCode::M:
						return KI_M;
					case Graphics::KeyCode::N:
						return KI_N;
					case Graphics::KeyCode::O:
						return KI_O;
					case Graphics::KeyCode::P:
						return KI_P;
					case Graphics::KeyCode::Q:
						return KI_Q;
					case Graphics::KeyCode::R:
						return KI_R;
					case Graphics::KeyCode::S:
						return KI_S;
					case Graphics::KeyCode::T:
						return KI_T;
					case Graphics::KeyCode::U:
						return KI_U;
					case Graphics::KeyCode::V:
						return KI_V;
					case Graphics::KeyCode::W:
						return KI_W;
					case Graphics::KeyCode::X:
						return KI_X;
					case Graphics::KeyCode::Y:
						return KI_Y;
					case Graphics::KeyCode::Z:
						return KI_Z;
					case Graphics::KeyCode::SEMICOLON:
						return KI_OEM_1;
					case Graphics::KeyCode::COMMA:
						return KI_OEM_COMMA;
					case Graphics::KeyCode::MINUS:
						return KI_OEM_MINUS;
					case Graphics::KeyCode::PERIOD:
						return KI_OEM_PERIOD;
					case Graphics::KeyCode::SLASH:
						return KI_OEM_2;
					case Graphics::KeyCode::LEFTBRACKET:
						return KI_OEM_4;
					case Graphics::KeyCode::BACKSLASH:
						return KI_OEM_5;
					case Graphics::KeyCode::RIGHTBRACKET:
						return KI_OEM_6;
					case Graphics::KeyCode::KP_0:
						return KI_NUMPAD0;
					case Graphics::KeyCode::KP_1:
						return KI_NUMPAD1;
					case Graphics::KeyCode::KP_2:
						return KI_NUMPAD2;
					case Graphics::KeyCode::KP_3:
						return KI_NUMPAD3;
					case Graphics::KeyCode::KP_4:
						return KI_NUMPAD4;
					case Graphics::KeyCode::KP_5:
						return KI_NUMPAD5;
					case Graphics::KeyCode::KP_6:
						return KI_NUMPAD6;
					case Graphics::KeyCode::KP_7:
						return KI_NUMPAD7;
					case Graphics::KeyCode::KP_8:
						return KI_NUMPAD8;
					case Graphics::KeyCode::KP_9:
						return KI_NUMPAD9;
					case Graphics::KeyCode::KP_ENTER:
						return KI_NUMPADENTER;
					case Graphics::KeyCode::KP_MULTIPLY:
						return KI_MULTIPLY;
					case Graphics::KeyCode::KP_PLUS:
						return KI_ADD;
					case Graphics::KeyCode::KP_MINUS:
						return KI_SUBTRACT;
					case Graphics::KeyCode::KP_PERIOD:
						return KI_DECIMAL;
					case Graphics::KeyCode::KP_DIVIDE:
						return KI_DIVIDE;
					case Graphics::KeyCode::KP_EQUALS:
						return KI_OEM_NEC_EQUAL;
					case Graphics::KeyCode::BACKSPACE:
						return KI_BACK;
					case Graphics::KeyCode::TAB:
						return KI_TAB;
					case Graphics::KeyCode::CLEAR:
						return KI_CLEAR;
					case Graphics::KeyCode::RETURN:
						return KI_RETURN;
					case Graphics::KeyCode::PAUSE:
						return KI_PAUSE;
					case Graphics::KeyCode::CAPSLOCK:
						return KI_CAPITAL;
					case Graphics::KeyCode::PAGEUP:
						return KI_PRIOR;
					case Graphics::KeyCode::PAGEDOWN:
						return KI_NEXT;
					case Graphics::KeyCode::END:
						return KI_END;
					case Graphics::KeyCode::HOME:
						return KI_HOME;
					case Graphics::KeyCode::LEFT:
						return KI_LEFT;
					case Graphics::KeyCode::UP:
						return KI_UP;
					case Graphics::KeyCode::RIGHT:
						return KI_RIGHT;
					case Graphics::KeyCode::DOWN:
						return KI_DOWN;
					case Graphics::KeyCode::INSERT:
						return KI_INSERT;
					case Graphics::KeyCode::DELETEKEY:
						return KI_DELETE;
					case Graphics::KeyCode::HELP:
						return KI_HELP;
					case Graphics::KeyCode::F1:
						return KI_F1;
					case Graphics::KeyCode::F2:
						return KI_F2;
					case Graphics::KeyCode::F3:
						return KI_F3;
					case Graphics::KeyCode::F4:
						return KI_F4;
					case Graphics::KeyCode::F5:
						return KI_F5;
					case Graphics::KeyCode::F6:
						return KI_F6;
					case Graphics::KeyCode::F7:
						return KI_F7;
					case Graphics::KeyCode::F8:
						return KI_F8;
					case Graphics::KeyCode::F9:
						return KI_F9;
					case Graphics::KeyCode::F10:
						return KI_F10;
					case Graphics::KeyCode::F11:
						return KI_F11;
					case Graphics::KeyCode::F12:
						return KI_F12;
					case Graphics::KeyCode::F13:
						return KI_F13;
					case Graphics::KeyCode::F14:
						return KI_F14;
					case Graphics::KeyCode::F15:
						return KI_F15;
					case Graphics::KeyCode::NUMLOCKCLEAR:
						return KI_NUMLOCK;
					case Graphics::KeyCode::SCROLLLOCK:
						return KI_SCROLL;
					case Graphics::KeyCode::LSHIFT:
						return KI_LSHIFT;
					case Graphics::KeyCode::RSHIFT:
						return KI_RSHIFT;
					case Graphics::KeyCode::LCTRL:
						return KI_LCONTROL;
					case Graphics::KeyCode::RCTRL:
						return KI_RCONTROL;
					case Graphics::KeyCode::LALT:
						return KI_LMENU;
					case Graphics::KeyCode::RALT:
						return KI_RMENU;
					case Graphics::KeyCode::LGUI:
						return KI_LMETA;
					case Graphics::KeyCode::RGUI:
						return KI_RMETA;
					default:
						return KI_UNKNOWN;
				}
#else
				return 0;
#endif
			}
			int Context::GetKeyMod(Graphics::KeyMod Mod)
			{
#ifdef ED_USE_RMLUI
				int Result = 0;
				if ((size_t)Mod & (size_t)Graphics::KeyMod::CTRL)
					Result |= Rml::Input::KM_CTRL;

				if ((size_t)Mod & (size_t)Graphics::KeyMod::SHIFT)
					Result |= Rml::Input::KM_SHIFT;

				if ((size_t)Mod & (size_t)Graphics::KeyMod::ALT)
					Result |= Rml::Input::KM_ALT;

				return Result;
#else
				return 0;
#endif
			}
		}
	}
}
#pragma warning(pop)