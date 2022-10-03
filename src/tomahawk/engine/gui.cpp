#include "gui.h"
#ifdef TH_WITH_RMLUI
#include <RmlUi/Core.h>
#include <RmlUi/Core/Stream.h>
#include <Source/Core/StyleSheetFactory.h>
#include <Source/Core/ElementStyle.h>

namespace Tomahawk
{
	namespace Engine
	{
		namespace GUI
		{
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
					TH_RELEASE(VertexBuffer);
					TH_RELEASE(IndexBuffer);
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
					TH_RELEASE(VertexBuffer);
					TH_RELEASE(Shader);
				}
				virtual void RenderGeometry(Rml::Vertex* Vertices, int VerticesSize, int* Indices, int IndicesSize, Rml::TextureHandle Texture, const Rml::Vector2f& Translation) override
				{
					TH_ASSERT_V(Device != nullptr, "graphics device should be set");
					TH_ASSERT_V(Vertices != nullptr, "vertices should be set");
					TH_ASSERT_V(Indices != nullptr, "indices should be set");

					Device->Begin();
					Device->Topology(Graphics::PrimitiveTopology::Triangle_List);
					Device->Texture((Graphics::Texture2D*)Texture);

					if (HasTransform)
						Device->Transform(Compute::Matrix4x4::CreateTranslation(Compute::Vector3(Translation.x, Translation.y)) * Transform * Ortho);
					else
						Device->Transform(Compute::Matrix4x4::CreateTranslation(Compute::Vector3(Translation.x, Translation.y)) * Ortho);

					for (int i = IndicesSize; i-- > 0;)
					{
						Rml::Vertex& V = Vertices[Indices[i]];
						Device->Emit();
						Device->Position(V.position.x, V.position.y, 0.0f);
						Device->TexCoord(V.tex_coord.x, V.tex_coord.y);
						Device->Color(V.colour.red / 255.0f, V.colour.green / 255.0f, V.colour.blue / 255.0f, V.colour.alpha / 255.0f);
					}

					Device->End();
				}
				virtual Rml::CompiledGeometryHandle CompileGeometry(Rml::Vertex* Vertices, int VerticesCount, int* Indices, int IndicesCount, Rml::TextureHandle Handle) override
				{
					TH_ASSERT(Device != nullptr, (Rml::CompiledGeometryHandle)nullptr, "graphics device should be set");
					GeometryBuffer* Result = TH_NEW(GeometryBuffer);
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
					TH_ASSERT_V(Device != nullptr, "graphics device should be set");
					TH_ASSERT_V(Buffer != nullptr, "buffer should be set");

					Device->Render.Diffuse = (Buffer->Texture != nullptr ? 1.0f : 0.0f);
					if (HasTransform)
						Device->Render.Transform = Compute::Matrix4x4::CreateTranslation(Compute::Vector3(Translation.x, Translation.y)) * Transform * Ortho;
					else
						Device->Render.Transform = Compute::Matrix4x4::CreateTranslation(Compute::Vector3(Translation.x, Translation.y)) * Ortho;

					Device->SetTexture2D(Buffer->Texture, 1, TH_PS);
					Device->SetShader(Shader, TH_VS | TH_PS);
					Device->SetVertexBuffer(Buffer->VertexBuffer, 0);
					Device->SetIndexBuffer(Buffer->IndexBuffer, Graphics::Format::R32_Uint);
					Device->UpdateBuffer(Graphics::RenderBufferType::Render);
					Device->DrawIndexed(Buffer->IndexBuffer->GetElements(), 0, 0);
				}
				virtual void ReleaseCompiledGeometry(Rml::CompiledGeometryHandle Handle) override
				{
					GeometryBuffer* Resource = (GeometryBuffer*)Handle;
					TH_DELETE(GeometryBuffer, Resource);
				}
				virtual void EnableScissorRegion(bool Enable) override
				{
					TH_ASSERT_V(Device != nullptr, "graphics device should be set");
					Ortho = Compute::Matrix4x4::CreateOrthographicOffCenter(0,
						Device->GetRenderTarget()->GetWidth(),
						Device->GetRenderTarget()->GetHeight(), 0.0f, -30000.0f, 10000.0f);

					Device->SetSamplerState(Sampler, 1, 1, TH_PS);
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
					TH_ASSERT_V(Device != nullptr, "graphics device should be set");
					if (!HasTransform)
					{
						float WY = Device->GetRenderTarget()->GetHeight();
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
					Device->SetShader(Shader, TH_VS | TH_PS);
					Device->SetVertexBuffer(VertexBuffer, 0);
					Device->UpdateBuffer(Graphics::RenderBufferType::Render);
					Device->Draw(VertexBuffer->GetElements(), 0);
					Device->SetDepthStencilState(ScissorDepthStencil);
					Device->SetBlendState(AlphaBlend);
				}
				virtual bool LoadTexture(Rml::TextureHandle& Handle, Rml::Vector2i& TextureDimensions, const Rml::String& Source) override
				{
					TH_ASSERT(Content != nullptr, false, "content manager should be set");
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
					TH_ASSERT(Device != nullptr, false, "graphics device should be set");
					TH_ASSERT(Source != nullptr, false, "source should be set");

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
					TH_RELEASE(Resource);
				}
				virtual void SetTransform(const Rml::Matrix4f* NewTransform) override
				{
					HasTransform = (NewTransform != nullptr);
					if (HasTransform)
						Transform = Subsystem::ToMatrix(NewTransform);
				}
				void Attach(ContentManager* NewContent)
				{
					TH_ASSERT_V(NewContent != nullptr, "content manager should be set");
					TH_ASSERT_V(NewContent->GetDevice() != nullptr, "graphics device should be set");

					TH_CLEAR(VertexBuffer);
					TH_CLEAR(Shader);

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
					if (!Core::OS::File::IsExists(Target.c_str()))
					{
						ContentManager* Content = (Subsystem::GetRenderInterface() ? Subsystem::GetRenderInterface()->GetContent() : nullptr);
						Target = (Content ? Core::OS::Path::Resolve(Path, Content->GetEnvironment()) : Core::OS::Path::Resolve(Path.c_str()));
						Target = (Target.empty() ? Path.c_str() : Target.c_str());
					}

					return (Rml::FileHandle)Core::OS::File::Open(Target, Core::FileMode::Binary_Read_Only);
				}
				virtual void Close(Rml::FileHandle File) override
				{
					Core::Stream* Stream = (Core::Stream*)File;
					TH_RELEASE(Stream);
				}
				virtual size_t Read(void* Buffer, size_t Size, Rml::FileHandle File) override
				{
					Core::Stream* Stream = (Core::Stream*)File;
					TH_ASSERT(Stream != nullptr, 0, "stream should be set");
					return Stream->Read((char*)Buffer, Size);
				}
				virtual bool Seek(Rml::FileHandle File, long Offset, int Origin) override
				{
					Core::Stream* Stream = (Core::Stream*)File;
					TH_ASSERT(Stream != nullptr, false, "stream should be set");
					return Stream->Seek((Core::FileSeek)Origin, Offset) == 0;
				}
				virtual size_t Tell(Rml::FileHandle File) override
				{
					Core::Stream* Stream = (Core::Stream*)File;
					TH_ASSERT(Stream != nullptr, 0, "stream should be set");
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
					TH_ASSERT_V(Activity != nullptr, "activity should be set");
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
					TH_ASSERT_V(Activity != nullptr, "activity should be set");
					Activity->SetClipboardText(Text);
				}
				virtual void GetClipboardText(Rml::String& Text) override
				{
					TH_ASSERT_V(Activity != nullptr, "activity should be set");
					Text = Activity->GetClipboardText();
				}
				virtual void ActivateKeyboard() override
				{
					TH_ASSERT_V(Activity != nullptr, "activity should be set");
					Activity->SetScreenKeyboard(true);
				}
				virtual void DeactivateKeyboard() override
				{
					TH_ASSERT_V(Activity != nullptr, "activity should be set");
					Activity->SetScreenKeyboard(false);
				}
				virtual void JoinPath(Rml::String& Result, const Rml::String& Path1, const Rml::String& Path2) override
				{
					ContentManager* Content = (Subsystem::GetRenderInterface() ? Subsystem::GetRenderInterface()->GetContent() : nullptr);
					TH_ASSERT_V(Content != nullptr, "activity should be set");

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
							TH_ERR("[gui] %.*s", Message.size(), Message.c_str());
							break;
						case Rml::Log::LT_WARNING:
							TH_WARN("[gui] %.*s", Message.size(), Message.c_str());
							break;
						case Rml::Log::LT_INFO:
						case Rml::Log::LT_ASSERT:
							TH_TRACE("[gui] %.*s", Message.size(), Message.c_str());
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

					return Time->GetElapsedTime() / 1000.0;
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
				const std::unordered_map<std::string, bool>& GetFontFaces()
				{
					return Fonts;
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
					return Rml::ContextPtr(TH_NEW(ScopedContext, Name));
				}
				virtual void ReleaseContext(Rml::Context* Context) override
				{
					ScopedContext* Item = (ScopedContext*)Context;
					TH_DELETE(ScopedContext, Item);
				}
				virtual void Release() override
				{
					TH_DELETE_THIS(ContextInstancer);
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
					TH_ASSERT_V(Scope && Scope->Basis && Scope->Basis->Compiler, "context should be scoped");

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
					TH_ASSERT_V(Scope && Scope->Basis && Scope->Basis->Compiler, "context should be scoped");

					Script::VMCompiler* Compiler = Scope->Basis->Compiler;
					if (Compiler->LoadFile(Core::Parser(Path).Replace('|', ':').R()) < 0)
						return;

					if (Compiler->Compile(true) < 0)
						return;

					Script::VMFunction Main = Compiler->GetModule().GetFunctionByName("Main");
					if (!Main.IsValid())
						return;

					Scope->Basis->AddRef();
					Script::VMContext* Context = Compiler->GetContext();
					Context->TryExecute(Main, [Main, Scope](Script::VMContext* Context)
					{
						if (Main.GetArgsCount() == 1)
							Context->SetArgObject(0, Scope->Basis);
					}).Await([Scope](int&&)
					{
						Scope->Basis->Release();
					});
				}
			};

			class DocumentInstancer : public Rml::ElementInstancer
			{
			public:
				Rml::ElementPtr InstanceElement(Rml::Element*, const Rml::String& Tag, const Rml::XMLAttributes&) override
				{
					return Rml::ElementPtr(TH_NEW(DocumentSubsystem, Tag));
				}
				void ReleaseElement(Rml::Element* Element) override
				{
					TH_DELETE(Element, Element);
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
					TH_DELETE_THIS(ListenerSubsystem);
				}
				void ProcessEvent(Rml::Event& Event) override
				{
					ScopedContext* Scope = (ScopedContext*)Event.GetCurrentElement()->GetContext();
					TH_ASSERT_V(Scope && Scope->Basis && Scope->Basis->Compiler, "context should be scoped");
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
					Context->TryExecute(Function, [Ptr](Script::VMContext* Context)
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

					std::string Name = "__vf" + Compute::Common::Hash(Compute::Digests::MD5(), Memory);
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
					return TH_NEW(ListenerSubsystem, Value, Element);
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
						TH_DELETE_THIS(EventSubsystem);
				}
				virtual void ProcessEvent(Rml::Event& Event) override
				{
					if (!Listener)
						return;

					IEvent Basis(&Event);
					Listener(Basis);
				}
			};

			void IVariant::Convert(Rml::Variant* From, Core::Variant* To)
			{
				TH_ASSERT_V(From && To, "from and to should be set");
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
			}
			void IVariant::Revert(Core::Variant* From, Rml::Variant* To)
			{
				TH_ASSERT_V(From && To, "from and to should be set");
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
						sscanf(Buffer, "%02x%02x%02x", &R, &G, &B);
						Result.X = R / 255.0f;
						Result.Y = G / 255.0f;
						Result.Z = B / 255.0f;
						Result.W = 1.0f;
					}
					else
					{
						unsigned int R = 0, G = 0, B = 0, A = 255;
						sscanf(Value.c_str(), "#%02x%02x%02x%02x", &R, &G, &B, &A);
						Result.X = R / 255.0f;
						Result.Y = G / 255.0f;
						Result.Z = B / 255.0f;
						Result.W = A / 255.0f;
					}
				}
				else
				{
					unsigned int R = 0, G = 0, B = 0, A = 255;
					sscanf(Value.c_str(), "%u %u %u %u", &R, &G, &B, &A);
					Result.X = R / 255.0f;
					Result.Y = G / 255.0f;
					Result.Z = B / 255.0f;
					Result.W = A / 255.0f;
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
						sscanf(Buffer, "%02x%02x%02x", &R, &G, &B);
					}
					else
						sscanf(Value.c_str(), "#%02x%02x%02x", &R, &G, &B);

					Result.X = R / 255.0f;
					Result.Y = G / 255.0f;
					Result.Z = B / 255.0f;
				}
				else
				{
					unsigned int R = 0, G = 0, B = 0;
					sscanf(Value.c_str(), "%u %u %u", &R, &G, &B);
					Result.X = R / 255.0f;
					Result.Y = G / 255.0f;
					Result.Z = B / 255.0f;
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
				sscanf(Base.c_str(), "v4 %f %f %f %f", &Result.X, &Result.Y, &Result.Z, &Result.W);

				return Result;
			}
			std::string IVariant::FromVector4(const Compute::Vector4& Base)
			{
				return Core::Form("v4 %f %f %f %f", Base.X, Base.Y, Base.Z, Base.W).R();
			}
			Compute::Vector3 IVariant::ToVector3(const std::string& Base)
			{
				Compute::Vector3 Result;
				sscanf(Base.c_str(), "v3 %f %f %f", &Result.X, &Result.Y, &Result.Z);

				return Result;
			}
			std::string IVariant::FromVector3(const Compute::Vector3& Base)
			{
				return Core::Form("v3 %f %f %f", Base.X, Base.Y, Base.Z).R();
			}
			Compute::Vector2 IVariant::ToVector2(const std::string& Base)
			{
				Compute::Vector2 Result;
				sscanf(Base.c_str(), "v2 %f %f", &Result.X, &Result.Y);

				return Result;
			}
			std::string IVariant::FromVector2(const Compute::Vector2& Base)
			{
				return Core::Form("v2 %f %f", Base.X, Base.Y).R();
			}

			IEvent::IEvent() : Base(nullptr)
			{
			}
			IEvent::IEvent(Rml::Event* Ref) : Base(Ref)
			{
			}
			EventPhase IEvent::GetPhase() const
			{
				TH_ASSERT(IsValid(), EventPhase::None, "event should be valid");
				return (EventPhase)Base->GetPhase();
			}
			void IEvent::SetPhase(EventPhase Phase)
			{
				TH_ASSERT_V(IsValid(), "event should be valid");
				Base->SetPhase((Rml::EventPhase)Phase);
			}
			void IEvent::SetCurrentElement(const IElement& Element)
			{
				TH_ASSERT_V(IsValid(), "event should be valid");
				Base->SetCurrentElement(Element.GetElement());
			}
			IElement IEvent::GetCurrentElement() const
			{
				TH_ASSERT(IsValid(), nullptr, "event should be valid");
				return Base->GetCurrentElement();
			}
			IElement IEvent::GetTargetElement() const
			{
				TH_ASSERT(IsValid(), nullptr, "event should be valid");
				return Base->GetTargetElement();
			}
			std::string IEvent::GetType() const
			{
				TH_ASSERT(IsValid(), std::string(), "event should be valid");
				return Base->GetType();
			}
			void IEvent::StopPropagation()
			{
				TH_ASSERT_V(IsValid(), "event should be valid");
				Base->StopPropagation();
			}
			void IEvent::StopImmediatePropagation()
			{
				TH_ASSERT_V(IsValid(), "event should be valid");
				Base->StopImmediatePropagation();
			}
			bool IEvent::IsInterruptible() const
			{
				TH_ASSERT(IsValid(), false, "event should be valid");
				return Base->IsInterruptible();
			}
			bool IEvent::IsPropagating() const
			{
				TH_ASSERT(IsValid(), false, "event should be valid");
				return Base->IsPropagating();
			}
			bool IEvent::IsImmediatePropagating() const
			{
				TH_ASSERT(IsValid(), false, "event should be valid");
				return Base->IsImmediatePropagating();
			}
			bool IEvent::GetBoolean(const std::string& Key) const
			{
				TH_ASSERT(IsValid(), false, "event should be valid");
				return Base->GetParameter<bool>(Key, false);
			}
			int64_t IEvent::GetInteger(const std::string& Key) const
			{
				TH_ASSERT(IsValid(), 0, "event should be valid");
				return Base->GetParameter<int64_t>(Key, 0);
			}
			double IEvent::GetNumber(const std::string& Key) const
			{
				TH_ASSERT(IsValid(), 0.0, "event should be valid");
				return Base->GetParameter<double>(Key, 0.0);
			}
			std::string IEvent::GetString(const std::string& Key) const
			{
				TH_ASSERT(IsValid(), std::string(), "event should be valid");
				return Base->GetParameter<Rml::String>(Key, "");
			}
			Compute::Vector2 IEvent::GetVector2(const std::string& Key) const
			{
				TH_ASSERT(IsValid(), Compute::Vector2(), "event should be valid");
				Rml::Vector2f Result = Base->GetParameter<Rml::Vector2f>(Key, Rml::Vector2f());
				return Compute::Vector2(Result.x, Result.y);
			}
			Compute::Vector3 IEvent::GetVector3(const std::string& Key) const
			{
				TH_ASSERT(IsValid(), Compute::Vector3(), "event should be valid");
				Rml::Vector3f Result = Base->GetParameter<Rml::Vector3f>(Key, Rml::Vector3f());
				return Compute::Vector3(Result.x, Result.y, Result.z);
			}
			Compute::Vector4 IEvent::GetVector4(const std::string& Key) const
			{
				TH_ASSERT(IsValid(), Compute::Vector4(), "event should be valid");
				Rml::Vector4f Result = Base->GetParameter<Rml::Vector4f>(Key, Rml::Vector4f());
				return Compute::Vector4(Result.x, Result.y, Result.z, Result.w);
			}
			void* IEvent::GetPointer(const std::string& Key) const
			{
				TH_ASSERT(IsValid(), nullptr, "event should be valid");
				return Base->GetParameter<void*>(Key, nullptr);
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
				TH_DELETE(Element, Base);
				Base = nullptr;
			}
			IElement IElement::Clone() const
			{
				TH_ASSERT(IsValid(), nullptr, "element should be valid");
				Rml::ElementPtr Ptr = Base->Clone();
				Rml::Element* Result = Ptr.get();
				Ptr.reset();

				return Result;
			}
			void IElement::SetClass(const std::string& ClassName, bool Activate)
			{
				TH_ASSERT_V(IsValid(), "element should be valid");
				Base->SetClass(ClassName, Activate);
			}
			bool IElement::IsClassSet(const std::string& ClassName) const
			{
				TH_ASSERT(IsValid(), nullptr, "element should be valid");
				return Base->IsClassSet(ClassName);
			}
			void IElement::SetClassNames(const std::string& ClassNames)
			{
				TH_ASSERT_V(IsValid(), "element should be valid");
				Base->SetClassNames(ClassNames);
			}
			std::string IElement::GetClassNames() const
			{
				TH_ASSERT(IsValid(), std::string(), "element should be valid");
				return Base->GetClassNames();
			}
			std::string IElement::GetAddress(bool IncludePseudoClasses, bool IncludeParents) const
			{
				TH_ASSERT(IsValid(), std::string(), "element should be valid");
				return Base->GetAddress(IncludePseudoClasses, IncludeParents);
			}
			void IElement::SetOffset(const Compute::Vector2& Offset, const IElement& OffsetParent, bool OffsetFixed)
			{
				TH_ASSERT_V(IsValid(), "element should be valid");
				Base->SetOffset(Rml::Vector2f(Offset.X, Offset.Y), OffsetParent.GetElement(), OffsetFixed);
			}
			Compute::Vector2 IElement::GetRelativeOffset(Area Type)
			{
				TH_ASSERT(IsValid(), Compute::Vector2(), "element should be valid");
				Rml::Vector2f Result = Base->GetRelativeOffset((Rml::Box::Area)Type);
				return Compute::Vector2(Result.x, Result.y);
			}
			Compute::Vector2 IElement::GetAbsoluteOffset(Area Type)
			{
				TH_ASSERT(IsValid(), Compute::Vector2(), "element should be valid");
				Rml::Vector2f Result = Base->GetAbsoluteOffset((Rml::Box::Area)Type);
				return Compute::Vector2(Result.x, Result.y);
			}
			void IElement::SetClientArea(Area ClientArea)
			{
				TH_ASSERT_V(IsValid(), "element should be valid");
				Base->SetClientArea((Rml::Box::Area)ClientArea);
			}
			Area IElement::GetClientArea() const
			{
				TH_ASSERT(IsValid(), Area::Content, "element should be valid");
				return (Area)Base->GetClientArea();
			}
			void IElement::SetContentBox(const Compute::Vector2& ContentOffset, const Compute::Vector2& ContentBox)
			{
				TH_ASSERT_V(IsValid(), "element should be valid");
				Base->SetContentBox(Rml::Vector2f(ContentOffset.X, ContentOffset.Y), Rml::Vector2f(ContentBox.X, ContentBox.Y));
			}
			float IElement::GetBaseline() const
			{
				TH_ASSERT(IsValid(), 0.0f, "element should be valid");
				return Base->GetBaseline();
			}
			bool IElement::GetIntrinsicDimensions(Compute::Vector2& Dimensions, float& Ratio)
			{
				TH_ASSERT(IsValid(), false, "element should be valid");
				Rml::Vector2f Size;
				bool Result = Base->GetIntrinsicDimensions(Size, Ratio);
				Dimensions = Compute::Vector2(Size.x, Size.y);

				return Result;
			}
			bool IElement::IsPointWithinElement(const Compute::Vector2& Point)
			{
				TH_ASSERT(IsValid(), false, "element should be valid");
				return Base->IsPointWithinElement(Rml::Vector2f(Point.X, Point.Y));
			}
			bool IElement::IsVisible() const
			{
				TH_ASSERT(IsValid(), false, "element should be valid");
				return Base->IsVisible();
			}
			float IElement::GetZIndex() const
			{
				TH_ASSERT(IsValid(), 0.0f, "element should be valid");
				return Base->GetZIndex();
			}
			bool IElement::SetProperty(const std::string& Name, const std::string& Value)
			{
				TH_ASSERT(IsValid(), false, "element should be valid");
				return Base->SetProperty(Name, Value);
			}
			void IElement::RemoveProperty(const std::string& Name)
			{
				TH_ASSERT_V(IsValid(), "element should be valid");
				Base->RemoveProperty(Name);
			}
			std::string IElement::GetProperty(const std::string& Name)
			{
				TH_ASSERT(IsValid(), std::string(), "element should be valid");
				const Rml::Property* Result = Base->GetProperty(Name);
				if (!Result)
					return "";

				return Result->ToString();
			}
			std::string IElement::GetLocalProperty(const std::string& Name)
			{
				TH_ASSERT(IsValid(), std::string(), "element should be valid");
				const Rml::Property* Result = Base->GetLocalProperty(Name);
				if (!Result)
					return "";

				return Result->ToString();
			}
			float IElement::ResolveNumericProperty(const std::string& PropertyName)
			{
				TH_ASSERT(IsValid(), 0.0f, "element should be valid");
				return Base->ResolveNumericProperty(PropertyName);
			}
			Compute::Vector2 IElement::GetContainingBlock()
			{
				TH_ASSERT(IsValid(), Compute::Vector2(), "element should be valid");
				Rml::Vector2f Result = Base->GetContainingBlock();
				return Compute::Vector2(Result.x, Result.y);
			}
			Position IElement::GetPosition()
			{
				TH_ASSERT(IsValid(), Position::Static, "element should be valid");
				return (Position)Base->GetPosition();
			}
			Float IElement::GetFloat()
			{
				TH_ASSERT(IsValid(), Float::None, "element should be valid");
				return (Float)Base->GetFloat();
			}
			Display IElement::GetDisplay()
			{
				TH_ASSERT(IsValid(), Display::None, "element should be valid");
				return (Display)Base->GetDisplay();
			}
			float IElement::GetLineHeight()
			{
				TH_ASSERT(IsValid(), 0.0f, "element should be valid");
				return Base->GetLineHeight();
			}
			bool IElement::Project(Compute::Vector2& Point) const noexcept
			{
				TH_ASSERT(IsValid(), false, "element should be valid");
				Rml::Vector2f Offset(Point.X, Point.Y);
				bool Result = Base->Project(Offset);
				Point = Compute::Vector2(Offset.x, Offset.y);

				return Result;
			}
			bool IElement::Animate(const std::string& PropertyName, const std::string& TargetValue, float Duration, TimingFunc Func, TimingDir Dir, int NumIterations, bool AlternateDirection, float Delay)
			{
				TH_ASSERT(IsValid(), false, "element should be valid");
				return Base->Animate(PropertyName, Rml::Property(TargetValue, Rml::Property::TRANSFORM), Duration, Rml::Tween((Rml::Tween::Type)Func, (Rml::Tween::Direction)Dir), NumIterations, AlternateDirection, Delay);
			}
			bool IElement::AddAnimationKey(const std::string& PropertyName, const std::string& TargetValue, float Duration, TimingFunc Func, TimingDir Dir)
			{
				TH_ASSERT(IsValid(), false, "element should be valid");
				return Base->AddAnimationKey(PropertyName, Rml::Property(TargetValue, Rml::Property::TRANSFORM), Duration, Rml::Tween((Rml::Tween::Type)Func, (Rml::Tween::Direction)Dir));
			}
			void IElement::SetPseudoClass(const std::string& PseudoClass, bool Activate)
			{
				TH_ASSERT_V(IsValid(), "element should be valid");
				Base->SetPseudoClass(PseudoClass, Activate);
			}
			bool IElement::IsPseudoClassSet(const std::string& PseudoClass) const
			{
				TH_ASSERT(IsValid(), false, "element should be valid");
				return Base->IsPseudoClassSet(PseudoClass);
			}
			void IElement::SetAttribute(const std::string& Name, const std::string& Value)
			{
				TH_ASSERT_V(IsValid(), "element should be valid");
				Base->SetAttribute(Name, Value);
			}
			std::string IElement::GetAttribute(const std::string& Name)
			{
				TH_ASSERT(IsValid(), std::string(), "element should be valid");
				return Base->GetAttribute<std::string>(Name, "");
			}
			bool IElement::HasAttribute(const std::string& Name) const
			{
				TH_ASSERT(IsValid(), false, "element should be valid");
				return Base->HasAttribute(Name);
			}
			void IElement::RemoveAttribute(const std::string& Name)
			{
				TH_ASSERT_V(IsValid(), "element should be valid");
				Base->RemoveAttribute(Name);
			}
			IElement IElement::GetFocusLeafNode()
			{
				TH_ASSERT(IsValid(), nullptr, "element should be valid");
				return Base->GetFocusLeafNode();
			}
			std::string IElement::GetTagName() const
			{
				TH_ASSERT(IsValid(), std::string(), "element should be valid");
				return Base->GetTagName();
			}
			std::string IElement::GetId() const
			{
				TH_ASSERT(IsValid(), std::string(), "element should be valid");
				return Base->GetId();
			}
			void IElement::SetId(const std::string& Id)
			{
				TH_ASSERT_V(IsValid(), "element should be valid");
				Base->SetId(Id);
			}
			float IElement::GetAbsoluteLeft()
			{
				TH_ASSERT(IsValid(), 0.0f, "element should be valid");
				return Base->GetAbsoluteLeft();
			}
			float IElement::GetAbsoluteTop()
			{
				TH_ASSERT(IsValid(), 0.0f, "element should be valid");
				return Base->GetAbsoluteTop();
			}
			float IElement::GetClientLeft()
			{
				TH_ASSERT(IsValid(), 0.0f, "element should be valid");
				return Base->GetClientLeft();
			}
			float IElement::GetClientTop()
			{
				TH_ASSERT(IsValid(), 0.0f, "element should be valid");
				return Base->GetClientTop();
			}
			float IElement::GetClientWidth()
			{
				TH_ASSERT(IsValid(), 0.0f, "element should be valid");
				return Base->GetClientWidth();
			}
			float IElement::GetClientHeight()
			{
				TH_ASSERT(IsValid(), 0.0f, "element should be valid");
				return Base->GetClientHeight();
			}
			IElement IElement::GetOffsetParent()
			{
				TH_ASSERT(IsValid(), nullptr, "element should be valid");
				return Base->GetOffsetParent();
			}
			float IElement::GetOffsetLeft()
			{
				TH_ASSERT(IsValid(), 0.0f, "element should be valid");
				return Base->GetOffsetLeft();
			}
			float IElement::GetOffsetTop()
			{
				TH_ASSERT(IsValid(), 0.0f, "element should be valid");
				return Base->GetOffsetTop();
			}
			float IElement::GetOffsetWidth()
			{
				TH_ASSERT(IsValid(), 0.0f, "element should be valid");
				return Base->GetOffsetWidth();
			}
			float IElement::GetOffsetHeight()
			{
				TH_ASSERT(IsValid(), 0.0f, "element should be valid");
				return Base->GetOffsetHeight();
			}
			float IElement::GetScrollLeft()
			{
				TH_ASSERT(IsValid(), 0.0f, "element should be valid");
				return Base->GetScrollLeft();
			}
			void IElement::SetScrollLeft(float ScrollLeft)
			{
				TH_ASSERT_V(IsValid(), "element should be valid");
				Base->SetScrollLeft(ScrollLeft);
			}
			float IElement::GetScrollTop()
			{
				TH_ASSERT(IsValid(), 0.0f, "element should be valid");
				return Base->GetScrollTop();
			}
			void IElement::SetScrollTop(float ScrollTop)
			{
				TH_ASSERT_V(IsValid(), "element should be valid");
				Base->SetScrollTop(ScrollTop);
			}
			float IElement::GetScrollWidth()
			{
				TH_ASSERT(IsValid(), 0.0f, "element should be valid");
				return Base->GetScrollWidth();
			}
			float IElement::GetScrollHeight()
			{
				TH_ASSERT(IsValid(), 0.0f, "element should be valid");
				return Base->GetScrollHeight();
			}
			IElementDocument IElement::GetOwnerDocument() const
			{
				TH_ASSERT(IsValid(), nullptr, "element should be valid");
				return Base->GetOwnerDocument();
			}
			IElement IElement::GetParentNode() const
			{
				TH_ASSERT(IsValid(), nullptr, "element should be valid");
				return Base->GetParentNode();
			}
			IElement IElement::GetNextSibling() const
			{
				TH_ASSERT(IsValid(), nullptr, "element should be valid");
				return Base->GetNextSibling();
			}
			IElement IElement::GetPreviousSibling() const
			{
				TH_ASSERT(IsValid(), nullptr, "element should be valid");
				return Base->GetPreviousSibling();
			}
			IElement IElement::GetFirstChild() const
			{
				TH_ASSERT(IsValid(), nullptr, "element should be valid");
				return Base->GetFirstChild();
			}
			IElement IElement::GetLastChild() const
			{
				TH_ASSERT(IsValid(), nullptr, "element should be valid");
				return Base->GetLastChild();
			}
			IElement IElement::GetChild(int Index) const
			{
				TH_ASSERT(IsValid(), nullptr, "element should be valid");
				return Base->GetChild(Index);
			}
			int IElement::GetNumChildren(bool IncludeNonDOMElements) const
			{
				TH_ASSERT(IsValid(), 0, "element should be valid");
				return Base->GetNumChildren(IncludeNonDOMElements);
			}
			void IElement::GetInnerHTML(std::string& Content) const
			{
				TH_ASSERT_V(IsValid(), "element should be valid");
				Base->GetInnerRML(Content);
			}
			std::string IElement::GetInnerHTML() const
			{
				TH_ASSERT(IsValid(), std::string(), "element should be valid");
				return Base->GetInnerRML();
			}
			void IElement::SetInnerHTML(const std::string& HTML)
			{
				TH_ASSERT_V(IsValid(), "element should be valid");
				Base->SetInnerRML(HTML);
			}
			bool IElement::IsFocused()
			{
				TH_ASSERT(IsValid(), false, "element should be valid");
				return Base->IsPseudoClassSet("focus");
			}
			bool IElement::IsHovered()
			{
				TH_ASSERT(IsValid(), false, "element should be valid");
				return Base->IsPseudoClassSet("hover");
			}
			bool IElement::IsActive()
			{
				TH_ASSERT(IsValid(), false, "element should be valid");
				return Base->IsPseudoClassSet("active");
			}
			bool IElement::IsChecked()
			{
				TH_ASSERT(IsValid(), false, "element should be valid");
				return Base->IsPseudoClassSet("checked");
			}
			bool IElement::Focus()
			{
				TH_ASSERT(IsValid(), false, "element should be valid");
				return Base->Focus();
			}
			void IElement::Blur()
			{
				TH_ASSERT_V(IsValid(), "element should be valid");
				Base->Blur();
			}
			void IElement::Click()
			{
				TH_ASSERT_V(IsValid(), "element should be valid");
				Base->Click();
			}
			void IElement::AddEventListener(const std::string& Event, Listener* Source, bool InCapturePhase)
			{
				TH_ASSERT_V(IsValid(), "element should be valid");
				TH_ASSERT_V(Source != nullptr && Source->Base != nullptr, "listener should be set");

				Base->AddEventListener(Event, Source->Base, InCapturePhase);
			}
			void IElement::RemoveEventListener(const std::string& Event, Listener* Source, bool InCapturePhase)
			{
				TH_ASSERT_V(IsValid(), "element should be valid");
				TH_ASSERT_V(Source != nullptr && Source->Base != nullptr, "listener should be set");

				Base->RemoveEventListener(Event, Source->Base, InCapturePhase);
			}
			bool IElement::DispatchEvent(const std::string& Type, const Core::VariantArgs& Args)
			{
				TH_ASSERT(IsValid(), false, "element should be valid");
				Rml::Dictionary Props;
				for (auto& Item : Args)
				{
					Rml::Variant& Prop = Props[Item.first];
					IVariant::Revert((Core::Variant*)&Item.second, &Prop);
				}

				return Base->DispatchEvent(Type, Props);
			}
			void IElement::ScrollIntoView(bool AlignWithTop)
			{
				TH_ASSERT_V(IsValid(), "element should be valid");
				Base->ScrollIntoView(AlignWithTop);
			}
			IElement IElement::AppendChild(const IElement& Element, bool DOMElement)
			{
				TH_ASSERT(IsValid(), nullptr, "element should be valid");
				return Base->AppendChild(Rml::ElementPtr(Element.GetElement()), DOMElement);
			}
			IElement IElement::InsertBefore(const IElement& Element, const IElement& AdjacentElement)
			{
				TH_ASSERT(IsValid(), nullptr, "element should be valid");
				return Base->InsertBefore(Rml::ElementPtr(Element.GetElement()), AdjacentElement.GetElement());
			}
			IElement IElement::ReplaceChild(const IElement& InsertedElement, const IElement& ReplacedElement)
			{
				TH_ASSERT(IsValid(), nullptr, "element should be valid");
				Rml::ElementPtr Ptr = Base->ReplaceChild(Rml::ElementPtr(InsertedElement.GetElement()), ReplacedElement.GetElement());
				Rml::Element* Result = Ptr.get();
				Ptr.reset();

				return Result;
			}
			IElement IElement::RemoveChild(const IElement& Element)
			{
				TH_ASSERT(IsValid(), nullptr, "element should be valid");
				Rml::ElementPtr Ptr = Base->RemoveChild(Element.GetElement());
				Rml::Element* Result = Ptr.get();
				Ptr.reset();

				return Result;
			}
			bool IElement::HasChildNodes() const
			{
				TH_ASSERT(IsValid(), nullptr, "element should be valid");
				return Base->HasChildNodes();
			}
			IElement IElement::GetElementById(const std::string& Id)
			{
				TH_ASSERT(IsValid(), nullptr, "element should be valid");
				return Base->GetElementById(Id);
			}
			IElement IElement::QuerySelector(const std::string& Selector)
			{
				TH_ASSERT(IsValid(), nullptr, "element should be valid");
				return Base->QuerySelector(Selector);
			}
			std::vector<IElement> IElement::QuerySelectorAll(const std::string& Selectors)
			{
				TH_ASSERT(IsValid(), std::vector<IElement>(), "element should be valid");
				Rml::ElementList Elements;
				Base->QuerySelectorAll(Elements, Selectors);

				std::vector<IElement> Result;
				Result.reserve(Elements.size());

				for (auto& Item : Elements)
					Result.push_back(Item);

				return Result;
			}
			int IElement::GetClippingIgnoreDepth()
			{
				TH_ASSERT(IsValid(), 0, "element should be valid");
				return Base->GetClippingIgnoreDepth();
			}
			bool IElement::IsClippingEnabled()
			{
				TH_ASSERT(IsValid(), false, "element should be valid");
				return Base->IsClippingEnabled();
			}
			bool IElement::CastFormColor(Compute::Vector4* Ptr, bool Alpha)
			{
				TH_ASSERT(IsValid(), false, "element should be valid");
				TH_ASSERT(Ptr != nullptr, false, "ptr should be set");

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
			}
			bool IElement::CastFormString(std::string* Ptr)
			{
				TH_ASSERT(IsValid(), false, "element should be valid");
				TH_ASSERT(Ptr != nullptr, false, "ptr should be set");

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
			}
			bool IElement::CastFormPointer(void** Ptr)
			{
				TH_ASSERT(IsValid(), false, "element should be valid");
				TH_ASSERT(Ptr != nullptr, false, "ptr should be set");

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
			}
			bool IElement::CastFormInt32(int32_t* Ptr)
			{
				TH_ASSERT(IsValid(), false, "element should be valid");
				TH_ASSERT(Ptr != nullptr, false, "ptr should be set");

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
			}
			bool IElement::CastFormUInt32(uint32_t* Ptr)
			{
				TH_ASSERT(IsValid(), false, "element should be valid");
				TH_ASSERT(Ptr != nullptr, false, "ptr should be set");

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
			}
			bool IElement::CastFormFlag32(uint32_t* Ptr, uint32_t Mask)
			{
				TH_ASSERT(IsValid(), false, "element should be valid");
				TH_ASSERT(Ptr != nullptr, false, "ptr should be set");

				bool Value = (*Ptr & Mask);
				if (!CastFormBoolean(&Value))
					return false;

				if (Value)
					*Ptr |= Mask;
				else
					*Ptr &= ~Mask;

				return true;
			}
			bool IElement::CastFormInt64(int64_t* Ptr)
			{
				TH_ASSERT(IsValid(), false, "element should be valid");
				TH_ASSERT(Ptr != nullptr, false, "ptr should be set");

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
			}
			bool IElement::CastFormUInt64(uint64_t* Ptr)
			{
				TH_ASSERT(IsValid(), false, "element should be valid");
				TH_ASSERT(Ptr != nullptr, false, "ptr should be set");

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
			}
			bool IElement::CastFormFlag64(uint64_t* Ptr, uint64_t Mask)
			{
				TH_ASSERT(IsValid(), false, "element should be valid");
				TH_ASSERT(Ptr != nullptr, false, "ptr should be set");

				bool Value = (*Ptr & Mask);
				if (!CastFormBoolean(&Value))
					return false;

				if (Value)
					*Ptr |= Mask;
				else
					*Ptr &= ~Mask;

				return true;
			}
			bool IElement::CastFormFloat(float* Ptr)
			{
				TH_ASSERT(IsValid(), false, "element should be valid");
				TH_ASSERT(Ptr != nullptr, false, "ptr should be set");

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
			}
			bool IElement::CastFormFloat(float* Ptr, float Mult)
			{
				TH_ASSERT(IsValid(), false, "element should be valid");
				TH_ASSERT(Ptr != nullptr, false, "ptr should be set");

				*Ptr *= Mult;
				bool Result = CastFormFloat(Ptr);
				*Ptr /= Mult;

				return Result;
			}
			bool IElement::CastFormDouble(double* Ptr)
			{
				TH_ASSERT(IsValid(), false, "element should be valid");
				TH_ASSERT(Ptr != nullptr, false, "ptr should be set");

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
			}
			bool IElement::CastFormBoolean(bool* Ptr)
			{
				TH_ASSERT(IsValid(), false, "element should be valid");
				TH_ASSERT(Ptr != nullptr, false, "ptr should be set");

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
			}
			std::string IElement::GetFormName() const
			{
				TH_ASSERT(IsValid(), std::string(), "element should be valid");
				Rml::ElementFormControl* Form = (Rml::ElementFormControl*)Base;
				return Form->GetName();
			}
			void IElement::SetFormName(const std::string& Name)
			{
				TH_ASSERT_V(IsValid(), "element should be valid");
				Rml::ElementFormControl* Form = (Rml::ElementFormControl*)Base;
				Form->SetName(Name);
			}
			std::string IElement::GetFormValue() const
			{
				TH_ASSERT(IsValid(), std::string(), "element should be valid");
				Rml::ElementFormControl* Form = (Rml::ElementFormControl*)Base;
				return Form->GetValue();
			}
			void IElement::SetFormValue(const std::string& Value)
			{
				TH_ASSERT_V(IsValid(), "element should be valid");
				Rml::ElementFormControl* Form = (Rml::ElementFormControl*)Base;
				Form->SetValue(Value);
			}
			bool IElement::IsFormDisabled() const
			{
				TH_ASSERT(IsValid(), false, "element should be valid");
				Rml::ElementFormControl* Form = (Rml::ElementFormControl*)Base;
				return Form->IsDisabled();
			}
			void IElement::SetFormDisabled(bool Disable)
			{
				TH_ASSERT_V(IsValid(), "element should be valid");
				Rml::ElementFormControl* Form = (Rml::ElementFormControl*)Base;
				Form->SetDisabled(Disable);
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
				Rml::ElementDocument* Item = (Rml::ElementDocument*)Base;
				TH_DELETE(ElementDocument, Item);
				Base = nullptr;
			}
			void IElementDocument::SetTitle(const std::string& Title)
			{
				TH_ASSERT_V(IsValid(), "element should be valid");
				((Rml::ElementDocument*)Base)->SetTitle(Title);
			}
			void IElementDocument::PullToFront()
			{
				TH_ASSERT_V(IsValid(), "element should be valid");
				((Rml::ElementDocument*)Base)->PullToFront();
			}
			void IElementDocument::PushToBack()
			{
				TH_ASSERT_V(IsValid(), "element should be valid");
				((Rml::ElementDocument*)Base)->PushToBack();
			}
			void IElementDocument::Show(ModalFlag Modal, FocusFlag Focus)
			{
				TH_ASSERT_V(IsValid(), "element should be valid");
				((Rml::ElementDocument*)Base)->Show((Rml::ModalFlag)Modal, (Rml::FocusFlag)Focus);
			}
			void IElementDocument::Hide()
			{
				TH_ASSERT_V(IsValid(), "element should be valid");
				((Rml::ElementDocument*)Base)->Hide();
			}
			void IElementDocument::Close()
			{
				TH_ASSERT_V(IsValid(), "element should be valid");
				((Rml::ElementDocument*)Base)->Close();
			}
			std::string IElementDocument::GetTitle() const
			{
				TH_ASSERT(IsValid(), std::string(), "element should be valid");
				return ((Rml::ElementDocument*)Base)->GetTitle();
			}
			std::string IElementDocument::GetSourceURL() const
			{
				TH_ASSERT(IsValid(), std::string(), "element should be valid");
				return ((Rml::ElementDocument*)Base)->GetSourceURL();
			}
			IElement IElementDocument::CreateElement(const std::string& Name)
			{
				TH_ASSERT(IsValid(), nullptr, "element should be valid");
				Rml::ElementPtr Ptr = ((Rml::ElementDocument*)Base)->CreateElement(Name);
				Rml::Element* Result = Ptr.get();
				Ptr.reset();

				return Result;
			}
			bool IElementDocument::IsModal() const
			{
				TH_ASSERT(IsValid(), false, "element should be valid");
				return ((Rml::ElementDocument*)Base)->IsModal();
			}
			Rml::ElementDocument* IElementDocument::GetElementDocument() const
			{
				return (Rml::ElementDocument*)Base;
			}

			bool Subsystem::Create()
			{
				State++;
				if (State > 1)
					return true;

				RenderInterface = TH_NEW(RenderSubsystem);
				Rml::SetRenderInterface(RenderInterface);

				FileInterface = TH_NEW(FileSubsystem);
				Rml::SetFileInterface(FileInterface);

				SystemInterface = TH_NEW(MainSubsystem);
				Rml::SetSystemInterface(SystemInterface);

				bool Result = Rml::Initialise();

				ContextFactory = TH_NEW(ContextInstancer);
				Rml::Factory::RegisterContextInstancer(ContextFactory);

				ListenerFactory = TH_NEW(ListenerInstancer);
				Rml::Factory::RegisterEventListenerInstancer(ListenerFactory);

				DocumentFactory = TH_NEW(DocumentInstancer);
				Rml::Factory::RegisterElementInstancer("body", DocumentFactory);

				CreateElements();
				return Result;
			}
			bool Subsystem::Release()
			{
				State--;
				if (State > 0 || State < 0)
					return State >= 0;

				Rml::Shutdown();
				if (HasDecorators)
				{
					HasDecorators = false;
					ReleaseDecorators();
				}

				TH_DELETE(MainSubsystem, SystemInterface);
				SystemInterface = nullptr;
				Rml::SetSystemInterface(nullptr);

				TH_DELETE(FileSubsystem, FileInterface);
				FileInterface = nullptr;
				Rml::SetFileInterface(nullptr);

				TH_DELETE(RenderSubsystem, RenderInterface);
				RenderInterface = nullptr;
				Rml::SetRenderInterface(nullptr);

				TH_DELETE(DocumentInstancer, DocumentFactory);
				DocumentFactory = nullptr;

				TH_DELETE(ListenerInstancer, ListenerFactory);
				ListenerFactory = nullptr;

				TH_DELETE(ContextInstancer, ContextFactory);
				ContextFactory = nullptr;

				ReleaseElements();

				ScriptInterface = nullptr;
				return true;
			}
			void Subsystem::SetMetadata(Graphics::Activity* Activity, ContentManager* Content, Core::Timer* Time)
			{
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
			}
			void Subsystem::SetTranslator(const std::string& Name, const TranslationCallback& Callback)
			{
				TH_ASSERT_V(SystemInterface != nullptr, "system interface should be valid");
				SystemInterface->SetTranslator(Name, Callback);
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
				TH_ASSERT(RenderInterface != nullptr, nullptr, "render interface should be valid");
				return RenderInterface->GetDevice();
			}
			Graphics::Texture2D* Subsystem::GetBackground()
			{
				TH_ASSERT(RenderInterface != nullptr, nullptr, "render interface should be valid");
				return RenderInterface->Background;
			}
			Compute::Matrix4x4* Subsystem::GetTransform()
			{
				TH_ASSERT(RenderInterface != nullptr, nullptr, "render interface should be valid");
				return RenderInterface->GetTransform();
			}
			Compute::Matrix4x4* Subsystem::GetProjection()
			{
				TH_ASSERT(RenderInterface != nullptr, nullptr, "render interface should be valid");
				return RenderInterface->GetProjection();
			}
			Compute::Matrix4x4 Subsystem::ToMatrix(const void* Matrix)
			{
				TH_ASSERT(Matrix != nullptr, Compute::Matrix4x4::Identity(), "matrix should be set");
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
				if (Ref != nullptr)
					Base = TH_NEW(Rml::DataModelConstructor, *Ref);
			}
			DataModel::~DataModel()
			{
				Detach();
				for (auto Item : Props)
					TH_DELETE(DataNode, Item.second);

				TH_DELETE(DataModelConstructor, Base);
			}
			void DataModel::SetDetachCallback(std::function<void()>&& Callback)
			{
				if (Callback)
					Callbacks.emplace_back(std::move(Callback));
			}
			DataNode* DataModel::SetProperty(const std::string& Name, const Core::Variant& Value)
			{
				TH_ASSERT(IsValid(), nullptr, "data node should be valid");
				DataNode* Result = GetProperty(Name);
				if (Result != nullptr)
				{
					Result->Set(Value);
					return Result;
				}

				Result = TH_NEW(DataNode, this, Name, Value);
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

				TH_DELETE(DataNode, Result);
				return nullptr;
			}
			DataNode* DataModel::SetProperty(const std::string& Name, Core::Variant* Value)
			{
				TH_ASSERT(IsValid(), nullptr, "data node should be valid");
				TH_ASSERT(Value != nullptr, nullptr, "value should be set");

				DataNode* Sub = GetProperty(Name);
				if (Sub != nullptr)
				{
					Sub->Set(Value);
					return Sub;
				}

				DataNode* Result = TH_NEW(DataNode, this, Name, Value);
				if (Base->Bind(Name, Result))
				{
					Props[Name] = Result;
					return Result;
				}

				TH_DELETE(DataNode, Result);
				return nullptr;
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
				TH_ASSERT(IsValid(), false, "data node should be valid");
				TH_ASSERT(Callback, false, "callback should not be empty");

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
			}
			bool DataModel::SetUnmountCallback(const ModelCallback& Callback)
			{
				OnUnmount = Callback;
				return true;
			}
			void DataModel::Change(const std::string& VariableName)
			{
				TH_ASSERT_V(IsValid(), "data node should be valid");
				Base->GetModelHandle().DirtyVariable(VariableName);
			}
			bool DataModel::HasChanged(const std::string& VariableName) const
			{
				TH_ASSERT(IsValid(), false, "data node should be valid");
				return Base->GetModelHandle().IsVariableDirty(VariableName);
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
				Ref = TH_NEW(Core::Variant, Initial);
			}
			DataNode::DataNode(DataModel* Model, const std::string& TopName, Core::Variant* Reference) noexcept : Name(TopName), Ref(Reference), Handle(Model), Order(nullptr), Depth(0), Safe(false)
			{
			}
			DataNode::DataNode(const DataNode& Other) noexcept : Childs(Other.Childs), Name(Other.Name), Handle(Other.Handle), Order(Other.Order), Depth(0), Safe(Other.Safe)
			{
				if (Safe)
					Ref = TH_NEW(Core::Variant, *Other.Ref);
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
					TH_DELETE(Variant, Ref);
			}
			DataNode& DataNode::Insert(size_t Where, const Core::VariantList& Initial, std::pair<void*, size_t>* Top)
			{
				TH_ASSERT(Where <= Childs.size(), *Childs.begin(), "index outside of range");
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
				TH_ASSERT(Where <= Childs.size(), *Childs.begin(), "index outside of range");
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
				TH_ASSERT(Where <= Childs.size(), *Childs.begin(), "index outside of range");
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
				TH_ASSERT(Index < Childs.size(), *this, "index outside of range");
				return Childs[Index];
			}
			bool DataNode::Remove(size_t Index)
			{
				TH_ASSERT(Index < Childs.size(), false, "index outside of range");
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
				std::sort(Childs.begin(), Childs.end(), [](const DataNode& A, const DataNode& B)
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
					TH_DELETE(Variant, Ref);

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
				*Ref = std::move(Core::Var::Number(Value));
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
					Ref = TH_NEW(Core::Variant, *Other.Ref);
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
				Base = TH_NEW(EventSubsystem, NewCallback);
			}
			Listener::Listener(const std::string& FunctionName)
			{
				Base = TH_NEW(ListenerSubsystem, FunctionName, nullptr);
			}
			Listener::~Listener()
			{
				Base->OnDetach(nullptr);
			}

			Context::Context(const Compute::Vector2& Size) : Compiler(nullptr), Cursor(-1.0f), Loading(false)
			{
				Base = (ScopedContext*)Rml::CreateContext(std::to_string(Subsystem::Id++), Rml::Vector2i(Size.X, Size.Y));

				TH_ASSERT_V(Base != nullptr, "context cannot be created");
				Base->Basis = this;
				CreateVM();
			}
			Context::Context(Graphics::GraphicsDevice* Device) : Compiler(nullptr), Cursor(-1.0f), Loading(false)
			{
				TH_ASSERT_V(Device != nullptr, "graphics device should be set");
				TH_ASSERT_V(Device->GetRenderTarget() != nullptr, "graphics device should be set");

				Graphics::RenderTarget2D* Target = Device->GetRenderTarget();
				Base = (ScopedContext*)Rml::CreateContext(std::to_string(Subsystem::Id++), Rml::Vector2i((int)Target->GetWidth(), (int)Target->GetHeight()));

				TH_ASSERT_V(Base != nullptr, "context cannot be created");
				Base->Basis = this;
				CreateVM();
			}
			Context::~Context()
			{
				RemoveDataModels();
				Rml::RemoveContext(Base->GetName());
				TH_RELEASE(Compiler);
			}
			void Context::EmitKey(Graphics::KeyCode Key, Graphics::KeyMod Mod, int Virtual, int Repeat, bool Pressed)
			{
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
			}
			void Context::EmitInput(const char* Buffer, int Length)
			{
				TH_ASSERT_V(Buffer != nullptr && Length > 0, "buffer should be set");
				if (Base->ProcessTextInput(std::string(Buffer, Length)))
					Inputs.Text = true;
			}
			void Context::EmitWheel(int X, int Y, bool Normal, Graphics::KeyMod Mod)
			{
				if (Base->ProcessMouseWheel(-Y, GetKeyMod(Mod)))
					Inputs.Scroll = true;
			}
			void Context::EmitResize(int Width, int Height)
			{
				RenderSubsystem* Renderer = Subsystem::GetRenderInterface();
				if (Renderer != nullptr)
					Renderer->ResizeBuffers(Width, Height);

				Base->SetDimensions(Rml::Vector2i(Width, Height));
			}
			void Context::UpdateEvents(Graphics::Activity* Activity)
			{
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
			}
			void Context::RenderLists(Graphics::Texture2D* Target)
			{
				TH_ASSERT_V(Subsystem::GetRenderInterface() != nullptr, "render interface should be valid");
				RenderSubsystem* Renderer = Subsystem::GetRenderInterface();
				Renderer->Background = Target;
				Base->Render();
			}
			void Context::ClearCache()
			{
				Rml::StyleSheetFactory::ClearStyleSheetCache();
			}
			IElementDocument Context::Construct(const std::string& Path)
			{
				uint64_t Length = 0;
				bool State = Loading;
				Loading = true;

				if (OnMount)
					OnMount(this);

				ClearVM();
				Elements.clear();

				unsigned char* Buffer = Core::OS::File::ReadAll(Path.c_str(), &Length);
				if (!Buffer)
				{
				ErrorState:
					Base->UnloadAllDocuments();
					Loading = State;

					return nullptr;
				}

				std::string Data((const char*)Buffer, Length);
				TH_FREE(Buffer);

				Decompose(Data);
				if (!Preprocess(Path, Data))
					goto ErrorState;

				Core::Parser URL(Path);
				URL.Replace('\\', '/');
				URL.Insert("file:///", 0);

				auto* Result = Base->LoadDocumentFromMemory(Data, URL.R());
				Loading = State;

				return Result;
			}
			bool Context::Deconstruct()
			{
				bool State = Loading;
				Loading = true;

				Elements.clear();
				Base->UnloadAllDocuments();
				Loading = State;

				return true;
			}
			bool Context::Inject(Core::Schema* Conf, const std::string& Relative)
			{
				TH_ASSERT(Conf != nullptr, false, "conf should be set");
				bool State = Loading;
				Loading = true;

				auto FontFaces = Conf->FindCollection("font-face", true);
				for (auto* Face : FontFaces)
				{
					Core::Schema* IPath = Face->GetAttribute("path");
					if (!IPath)
					{
						TH_ERR("[gui] path is required for font face");
						return false;
					}

					std::string Path = IPath->Value.Serialize();
					std::string Target = Core::OS::Path::Resolve(Path, Relative);

					if (!AddFontFace(Target.empty() ? Path : Target, Face->GetAttribute("fallback") != nullptr))
					{
						Loading = State;
						return false;
					}
				}

				auto Documents = Conf->FindCollection("document", true);
				for (auto* Schema : Documents)
				{
					Core::Schema* IPath = Schema->GetAttribute("path");
					if (!IPath)
					{
						TH_ERR("[gui] path is required for document");
						return false;
					}

					std::string Path = IPath->Value.Serialize();
					std::string Target = Core::OS::Path::Resolve(Path, Relative);
					IElementDocument Result = Construct(Target.empty() ? Path : Target);

					if (!Result.IsValid())
					{
						Loading = State;
						return false;
					}

					if (Schema->GetAttribute("show") != nullptr)
						Result.Show();
				}

				Loading = State;
				return true;
			}
			bool Context::Inject(const std::string& ConfPath)
			{
				TH_ASSERT(Subsystem::RenderInterface != nullptr, false, "render interface should be set");
				TH_ASSERT(Subsystem::RenderInterface->GetContent() != nullptr, false, "content manager should be set");

				bool State = Loading;
				Loading = true;

				Core::Schema* Sheet = Subsystem::RenderInterface->GetContent()->Load<Core::Schema>(ConfPath);
				if (!Sheet)
				{
					Loading = State;
					return false;
				}

				bool Result = Inject(Sheet, Core::OS::Path::GetDirectory(ConfPath.c_str()));
				TH_RELEASE(Sheet);

				Loading = State;
				return Result;
			}
			bool Context::IsLoading()
			{
				return Loading;
			}
			bool Context::IsInputFocused()
			{
				Rml::Element* Element = Base->GetFocusElement();
				if (!Element)
					return false;

				const Rml::String& Tag = Element->GetTagName();
				return Tag == "input" || Tag == "textarea" || Tag == "select";
			}
			bool Context::AddFontFace(const std::string& Path, bool UseAsFallback)
			{
				TH_ASSERT(Subsystem::GetSystemInterface() != nullptr, false, "system interface should be set");
				bool State = Loading;
				Loading = true;

				bool Result = Subsystem::GetSystemInterface()->AddFontFace(Path, UseAsFallback);
				Loading = State;

				return Result;
			}
			const std::unordered_map<std::string, bool>& Context::GetFontFaces()
			{
				return Subsystem::GetSystemInterface()->GetFontFaces();
			}
			Rml::Context* Context::GetContext()
			{
				return Base;
			}
			Compute::Vector2 Context::GetDimensions() const
			{
				Rml::Vector2i Size = Base->GetDimensions();
				return Compute::Vector2(Size.x, Size.y);
			}
			void Context::SetDensityIndependentPixelRatio(float DensityIndependentPixelRatio)
			{
				Base->SetDensityIndependentPixelRatio(DensityIndependentPixelRatio);
			}
			float Context::GetDensityIndependentPixelRatio() const
			{
				return Base->GetDensityIndependentPixelRatio();
			}
			IElementDocument Context::CreateDocument(const std::string& InstancerName)
			{
				return Base->CreateDocument(InstancerName);
			}
			void Context::EnableMouseCursor(bool Enable)
			{
				Base->EnableMouseCursor(Enable);
			}
			IElementDocument Context::GetDocument(const std::string& Id)
			{
				return Base->GetDocument(Id);
			}
			IElementDocument Context::GetDocument(int Index)
			{
				return Base->GetDocument(Index);
			}
			int Context::GetNumDocuments() const
			{
				return Base->GetNumDocuments();
			}
			IElement Context::GetElementById(int DocIndex, const std::string& Id)
			{
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
			}
			IElement Context::GetHoverElement()
			{
				return Base->GetHoverElement();
			}
			IElement Context::GetFocusElement()
			{
				return Base->GetFocusElement();
			}
			IElement Context::GetRootElement()
			{
				return Base->GetRootElement();
			}
			IElement Context::GetElementAtPoint(const Compute::Vector2& Point, const IElement& IgnoreElement, const IElement& Element) const
			{
				return Base->GetElementAtPoint(Rml::Vector2f(Point.X, Point.Y), IgnoreElement.GetElement(), Element.GetElement());
			}
			void Context::PullDocumentToFront(const IElementDocument& Schema)
			{
				return Base->PullDocumentToFront(Schema.GetElementDocument());
			}
			void Context::PushDocumentToBack(const IElementDocument& Schema)
			{
				return Base->PushDocumentToBack(Schema.GetElementDocument());
			}
			void Context::UnfocusDocument(const IElementDocument& Schema)
			{
				return Base->UnfocusDocument(Schema.GetElementDocument());
			}
			void Context::AddEventListener(const std::string& Event, Listener* Listener, bool InCapturePhase)
			{
				TH_ASSERT_V(Listener != nullptr && Listener->Base != nullptr, "listener should be valid");
				Base->AddEventListener(Event, Listener->Base, InCapturePhase);
			}
			void Context::RemoveEventListener(const std::string& Event, Listener* Listener, bool InCapturePhase)
			{
				TH_ASSERT_V(Listener != nullptr && Listener->Base != nullptr, "listener should be valid");
				Base->RemoveEventListener(Event, Listener->Base, InCapturePhase);
			}
			bool Context::IsMouseInteracting() const
			{
				return Base->IsMouseInteracting();
			}
			bool Context::WasInputUsed(uint32_t InputTypeMask)
			{
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
			}
			bool Context::GetActiveClipRegion(Compute::Vector2& Origin, Compute::Vector2& Dimensions) const
			{
				Rml::Vector2i O1((int)Origin.X, (int)Origin.Y);
				Rml::Vector2i O2((int)Dimensions.X, (int)Dimensions.Y);
				bool Result = Base->GetActiveClipRegion(O1, O2);
				Origin = Compute::Vector2(O1.x, O1.y);
				Dimensions = Compute::Vector2(O2.x, O2.y);

				return Result;
			}
			void Context::SetActiveClipRegion(const Compute::Vector2& Origin, const Compute::Vector2& Dimensions)
			{
				return Base->SetActiveClipRegion(Rml::Vector2i(Origin.X, Origin.Y), Rml::Vector2i(Dimensions.X, Dimensions.Y));
			}
			DataModel* Context::SetDataModel(const std::string& Name)
			{
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
				if (!Base->RemoveDataModel(Name))
					return false;

				auto It = Models.find(Name);
				if (It != Models.end())
				{
					if (It->second->OnUnmount)
						It->second->OnUnmount(this);

					TH_RELEASE(It->second);
					Models.erase(It);
				}

				return true;
			}
			bool Context::RemoveDataModels()
			{
				if (Models.empty())
					return false;

				for (auto Item : Models)
				{
					Base->RemoveDataModel(Item.first);
					TH_RELEASE(Item.second);
				}

				Models.clear();
				return true;
			}
			void Context::SetDocumentsBaseTag(const std::string& Tag)
			{
				Base->SetDocumentsBaseTag(Tag);
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

					uint64_t Length;
					unsigned char* Data = Core::OS::File::ReadAll(File.Module.c_str(), &Length);
					if (!Data)
						return false;

					Output->assign((const char*)Data, (size_t)Length);
					TH_FREE(Data);

					this->Decompose(*Output);
					return true;
				});
				Processor->SetIncludeOptions(Desc);
				Processor->SetFeatures(Features);

				bool Result = Processor->Process(Path, Buffer);
				TH_RELEASE(Processor);

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
			const std::string& Context::GetDocumentsBaseTag()
			{
				return Base->GetDocumentsBaseTag();
			}
			int Context::GetKeyCode(Graphics::KeyCode Key)
			{
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
			}
			int Context::GetKeyMod(Graphics::KeyMod Mod)
			{
				int Result = 0;
				if ((size_t)Mod & (size_t)Graphics::KeyMod::CTRL)
					Result |= Rml::Input::KM_CTRL;

				if ((size_t)Mod & (size_t)Graphics::KeyMod::SHIFT)
					Result |= Rml::Input::KM_SHIFT;

				if ((size_t)Mod & (size_t)Graphics::KeyMod::ALT)
					Result |= Rml::Input::KM_ALT;

				return Result;
			}
		}
	}
}
#endif
