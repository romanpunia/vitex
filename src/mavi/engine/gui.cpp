#include "gui.h"
#include "../core/network.h"
#ifdef VI_USE_RMLUI
#include <RmlUi/Core.h>
#include <RmlUi/Core/Stream.h>
#include <Source/Core/StyleSheetFactory.h>
#include <Source/Core/ElementStyle.h>
#endif
#pragma warning(push)
#pragma warning(disable: 4996)

namespace Mavi
{
	namespace Engine
	{
		namespace GUI
		{
#ifdef VI_USE_RMLUI
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
					VI_RELEASE(VertexBuffer);
					VI_RELEASE(IndexBuffer);
				}
			};

			class RenderSubsystem final : public Rml::RenderInterface
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
				RenderConstants* Constants;
				Compute::Matrix4x4 Transform;
				Compute::Matrix4x4 Ortho;
				bool HasTransform;

			public:
				Graphics::Texture2D* Background;

			public:
				RenderSubsystem() : Rml::RenderInterface(), Device(nullptr), Content(nullptr), Constants(nullptr), HasTransform(false), Background(nullptr)
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
				~RenderSubsystem() override
				{
					VI_RELEASE(VertexBuffer);
					VI_RELEASE(Shader);
				}
				void RenderGeometry(Rml::Vertex* Vertices, int VerticesSize, int* Indices, int IndicesSize, Rml::TextureHandle Texture, const Rml::Vector2f& Translation) override
				{
					VI_ASSERT_V(Device != nullptr, "graphics device should be set");
					VI_ASSERT_V(Vertices != nullptr, "vertices should be set");
					VI_ASSERT_V(Indices != nullptr, "indices should be set");

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
				Rml::CompiledGeometryHandle CompileGeometry(Rml::Vertex* Vertices, int VerticesCount, int* Indices, int IndicesCount, Rml::TextureHandle Handle) override
				{
					VI_ASSERT(Device != nullptr, (Rml::CompiledGeometryHandle)nullptr, "graphics device should be set");
					GeometryBuffer* Result = VI_NEW(GeometryBuffer);
					Result->Texture = (Graphics::Texture2D*)Handle;

					Graphics::ElementBuffer::Desc F = Graphics::ElementBuffer::Desc();
					F.AccessFlags = Graphics::CPUAccess::None;
					F.Usage = Graphics::ResourceUsage::Default;
					F.BindFlags = Graphics::ResourceBind::Vertex_Buffer;
					F.ElementCount = (unsigned int)VerticesCount;
					F.Elements = (void*)Vertices;
					F.ElementWidth = sizeof(Rml::Vertex);
					Result->VertexBuffer = Device->CreateElementBuffer(F);

					F = Graphics::ElementBuffer::Desc();
					F.AccessFlags = Graphics::CPUAccess::None;
					F.Usage = Graphics::ResourceUsage::Default;
					F.BindFlags = Graphics::ResourceBind::Index_Buffer;
					F.ElementCount = (unsigned int)IndicesCount;
					F.ElementWidth = sizeof(unsigned int);
					F.Elements = (void*)Indices;
					Result->IndexBuffer = Device->CreateElementBuffer(F);
					
					return (Rml::CompiledGeometryHandle)Result;
				}
				void RenderCompiledGeometry(Rml::CompiledGeometryHandle Handle, const Rml::Vector2f& Translation) override
				{
					GeometryBuffer* Buffer = (GeometryBuffer*)Handle;
					VI_ASSERT_V(Device != nullptr, "graphics device should be set");
					VI_ASSERT_V(Buffer != nullptr, "buffer should be set");

					Constants->Render.Diffuse = (Buffer->Texture != nullptr ? 1.0f : 0.0f);
					if (HasTransform)
						Constants->Render.Transform = Compute::Matrix4x4::CreateTranslation(Compute::Vector3(Translation.x, Translation.y)) * Transform * Ortho;
					else
						Constants->Render.Transform = Compute::Matrix4x4::CreateTranslation(Compute::Vector3(Translation.x, Translation.y)) * Ortho;

					Constants->UpdateConstantBuffer(RenderBufferType::Render);
					Device->SetShader(Shader, VI_VS | VI_PS);
					Device->SetTexture2D(Buffer->Texture, 1, VI_PS);
					Device->SetVertexBuffer(Buffer->VertexBuffer);
					Device->SetIndexBuffer(Buffer->IndexBuffer, Graphics::Format::R32_Uint);
					Device->DrawIndexed((unsigned int)Buffer->IndexBuffer->GetElements(), 0, 0);
				}
				void ReleaseCompiledGeometry(Rml::CompiledGeometryHandle Handle) override
				{
					GeometryBuffer* Resource = (GeometryBuffer*)Handle;
					VI_DELETE(GeometryBuffer, Resource);
				}
				void EnableScissorRegion(bool Enable) override
				{
					VI_ASSERT_V(Device != nullptr, "graphics device should be set");
					Ortho = Compute::Matrix4x4::CreateOrthographicOffCenter(0, (float)Device->GetRenderTarget()->GetWidth(), (float)Device->GetRenderTarget()->GetHeight(), 0.0f, -30000.0f, 10000.0f);
					Device->SetSamplerState(Sampler, 1, 1, VI_PS);
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
				void SetScissorRegion(int X, int Y, int Width, int Height) override
				{
					VI_ASSERT_V(Device != nullptr, "graphics device should be set");
					if (!HasTransform)
					{
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

					Constants->Render.Transform = Transform * Ortho;
					Constants->UpdateConstantBuffer(RenderBufferType::Render);
					Device->ClearDepth();
					Device->SetBlendState(ColorlessBlend);
					Device->SetShader(Shader, VI_VS | VI_PS);
					Device->SetVertexBuffer(VertexBuffer);
					Device->Draw((unsigned int)VertexBuffer->GetElements(), 0);
					Device->SetDepthStencilState(ScissorDepthStencil);
					Device->SetBlendState(AlphaBlend);
				}
				bool LoadTexture(Rml::TextureHandle& Handle, Rml::Vector2i& TextureDimensions, const Rml::String& Source) override
				{
					VI_ASSERT(Content != nullptr, false, "content manager should be set");
					Graphics::Texture2D* Result = Content->Load<Graphics::Texture2D>(Source);
					if (!Result)
						return false;

					TextureDimensions.x = Result->GetWidth();
					TextureDimensions.y = Result->GetHeight();
					Handle = (Rml::TextureHandle)Result;

					return true;
				}
				bool GenerateTexture(Rml::TextureHandle& Handle, const Rml::byte* Source, const Rml::Vector2i& SourceDimensions) override
				{
					VI_ASSERT(Device != nullptr, false, "graphics device should be set");
					VI_ASSERT(Source != nullptr, false, "source should be set");

					Graphics::Texture2D::Desc F = Graphics::Texture2D::Desc();
					F.Data = (void*)Source;
					F.Width = (unsigned int)SourceDimensions.x;
					F.Height = (unsigned int)SourceDimensions.y;
					F.RowPitch = Device->GetRowPitch(F.Width);
					F.DepthPitch = Device->GetDepthPitch(F.RowPitch, F.Height);
					F.MipLevels = 1;

					Graphics::Texture2D* Result = Device->CreateTexture2D(F);
					if (!Result)
						return false;

					Handle = (Rml::TextureHandle)Result;
					return true;
				}
				void ReleaseTexture(Rml::TextureHandle Handle) override
				{
					Graphics::Texture2D* Resource = (Graphics::Texture2D*)Handle;
					VI_RELEASE(Resource);
				}
				void SetTransform(const Rml::Matrix4f* NewTransform) override
				{
					HasTransform = (NewTransform != nullptr);
					if (HasTransform)
						Transform = Subsystem::ToMatrix(NewTransform);
				}
				void Attach(RenderConstants* NewConstants, ContentManager* NewContent)
				{
					VI_ASSERT_V(NewConstants != nullptr, "render constants should be set");
					VI_ASSERT_V(NewContent != nullptr, "content manager should be set");
					VI_ASSERT_V(NewContent->GetDevice() != nullptr, "graphics device should be set");

					VI_CLEAR(VertexBuffer);
					VI_CLEAR(Shader);

					Constants = NewConstants;
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

			class FileSubsystem final : public Rml::FileInterface
			{
			public:
				FileSubsystem() : Rml::FileInterface()
				{
				}
				Rml::FileHandle Open(const Rml::String& Path) override
				{
					Core::String Target = Path;
					Network::Location URL(Target);
					if (URL.Protocol == "file")
					{
						if (!Core::OS::File::IsExists(Target.c_str()))
						{
							ContentManager* Content = (Subsystem::GetRenderInterface() ? Subsystem::GetRenderInterface()->GetContent() : nullptr);
							Target = (Content ? Core::OS::Path::Resolve(Path, Content->GetEnvironment(), false) : Core::OS::Path::Resolve(Path.c_str()));
							Target = (Target.empty() ? Path.c_str() : Target.c_str());
						}
					}
					else if (URL.Protocol != "http" && URL.Protocol != "https")
						return (Rml::FileHandle)nullptr;

					return (Rml::FileHandle)Core::OS::File::Open(Target, Core::FileMode::Binary_Read_Only);
				}
				void Close(Rml::FileHandle File) override
				{
					Core::Stream* Stream = (Core::Stream*)File;
					VI_RELEASE(Stream);
				}
				size_t Read(void* Buffer, size_t Size, Rml::FileHandle File) override
				{
					Core::Stream* Stream = (Core::Stream*)File;
					VI_ASSERT(Stream != nullptr, 0, "stream should be set");
					return Stream->Read((char*)Buffer, Size);
				}
				bool Seek(Rml::FileHandle File, long Offset, int Origin) override
				{
					Core::Stream* Stream = (Core::Stream*)File;
					VI_ASSERT(Stream != nullptr, false, "stream should be set");
					return Stream->Seek((Core::FileSeek)Origin, Offset) == 0;
				}
				size_t Tell(Rml::FileHandle File) override
				{
					Core::Stream* Stream = (Core::Stream*)File;
					VI_ASSERT(Stream != nullptr, 0, "stream should be set");
					return Stream->Tell();
				}
			};

			class MainSubsystem final : public Rml::SystemInterface
			{
			private:
				Core::UnorderedMap<Core::String, TranslationCallback> Translators;
				Core::UnorderedMap<Core::String, bool> Fonts;
				Graphics::Activity* Activity;
				Core::Timer* Time;

			public:
				MainSubsystem() : Rml::SystemInterface(), Activity(nullptr), Time(nullptr)
				{
				}
				void SetMouseCursor(const Rml::String& CursorName) override
				{
					VI_ASSERT_V(Activity != nullptr, "activity should be set");
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
				void SetClipboardText(const Rml::String& Text) override
				{
					VI_ASSERT_V(Activity != nullptr, "activity should be set");
					Activity->SetClipboardText(Text);
				}
				void GetClipboardText(Rml::String& Text) override
				{
					VI_ASSERT_V(Activity != nullptr, "activity should be set");
					Text = Activity->GetClipboardText();
				}
				void ActivateKeyboard(Rml::Vector2f CaretPosition, float LineHeight) override
				{
					VI_ASSERT_V(Activity != nullptr, "activity should be set");
					Activity->SetScreenKeyboard(true);
				}
				void DeactivateKeyboard() override
				{
					VI_ASSERT_V(Activity != nullptr, "activity should be set");
					Activity->SetScreenKeyboard(false);
				}
				void JoinPath(Rml::String& Result, const Rml::String& Path1, const Rml::String& Path2) override
				{
					ContentManager* Content = (Subsystem::GetRenderInterface() ? Subsystem::GetRenderInterface()->GetContent() : nullptr);
					VI_ASSERT_V(Content != nullptr, "activity should be set");

					Core::String Proto1, Proto2;
					Core::String Fixed1 = GetFixedURL(Path1, Proto1);
					Core::String Fixed2 = GetFixedURL(Path2, Proto2);

					if (Proto1 != "file" && Proto2 == "file")
					{
						Core::Stringify Buffer(&Result);
						if (!Buffer.Assign(Path1).EndsWith('/'))
							Buffer.Append('/');

						Buffer.Append(Fixed2).Replace("/////", "//");
						Core::Stringify::Settle Idx = Buffer.Find("://");
						Buffer.Replace("//", "/", Idx.Found ? Idx.End : 0);
					}
					else if (Proto1 == "file" && Proto2 == "file")
					{
						Result = Core::OS::Path::Resolve(Fixed2, Core::OS::Path::GetDirectory(Fixed1.c_str()), false);
						if (Result.empty())
							Result = Core::OS::Path::Resolve(Fixed2, Content->GetEnvironment(), false);
					}
					else if (Proto1 == "file" && Proto2 != "file")
						Result = Core::Stringify(Path2).Replace("/////", "//").R();
				}
				bool LogMessage(Rml::Log::Type Type, const Rml::String& Message) override
				{
					switch (Type)
					{
						case Rml::Log::LT_ERROR:
							VI_ERR("[gui] %.*s", Message.size(), Message.c_str());
							break;
						case Rml::Log::LT_WARNING:
							VI_WARN("[gui] %.*s", Message.size(), Message.c_str());
							break;
						case Rml::Log::LT_INFO:
						case Rml::Log::LT_ASSERT:
							VI_DEBUG("[gui] %.*s", Message.size(), Message.c_str());
							break;
						default:
							break;
					}

					return true;
				}
				int TranslateString(Rml::String& Result, const Rml::String& KeyName) override
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
				double GetElapsedTime() override
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
				void SetTranslator(const Core::String& Name, const TranslationCallback& Callback)
				{
					auto It = Translators.find(Name);
					if (It == Translators.end())
						Translators.insert(std::make_pair(Name, Callback));
					else
						It->second = Callback;
				}
				bool AddFontFace(const Core::String& Path, bool UseAsFallback)
				{
					auto It = Fonts.find(Path);
					if (It != Fonts.end())
						return true;

					if (!Rml::LoadFontFace(Path, UseAsFallback))
						return false;

					Fonts.insert(std::make_pair(Path, UseAsFallback));
					return true;
				}
				Core::UnorderedMap<Core::String, bool>* GetFontFaces()
				{
					return &Fonts;
				}
				Core::String GetFixedURL(const Core::String& URL, Core::String& Proto)
				{
					if (!Core::Stringify(&URL).Find("://").Found)
					{
						Proto = "file";
						return URL;
					}

					Rml::URL Base(URL);
					Proto = Base.GetProtocol();
					return Base.GetPathedFileName();
				}
			};

			class ScopedContext final : public Rml::Context
			{
			public:
				GUI::Context* Basis;

			public:
				ScopedContext(const Rml::String& Name) : Rml::Context(Name), Basis(nullptr)
				{
				}
			};

			class ContextInstancer final : public Rml::ContextInstancer
			{
			public:
				Rml::ContextPtr InstanceContext(const Rml::String& Name) override
				{
					return Rml::ContextPtr(VI_NEW(ScopedContext, Name));
				}
				void ReleaseContext(Rml::Context* Context) override
				{
					ScopedContext* Item = (ScopedContext*)Context;
					VI_DELETE(ScopedContext, Item);
				}
				void Release() override
				{
					VI_DELETE(ContextInstancer, this);
				}
			};

			class DocumentSubsystem final : public Rml::ElementDocument
			{
			public:
				DocumentSubsystem(const Rml::String& Tag) : Rml::ElementDocument(Tag)
				{
				}
				void LoadInlineScript(const Rml::String& Content, const Rml::String& Path, int Line) override
				{
					ScopedContext* Scope = (ScopedContext*)GetContext();
					VI_ASSERT_V(Scope && Scope->Basis && Scope->Basis->Compiler, "context should be scoped");

					Scope->Basis->AddRef();
					Scope->Basis->Compiler->CompileFunction(Content).When([Scope](Scripting::Function&& Function)
					{
						Scripting::FunctionDelegate Delegate(Function);
						Function.Release();

						if (!Delegate.IsValid())
							return Scope->Basis->Release();

						Delegate(nullptr, [Scope](Scripting::ImmediateContext*)
						{
							Scope->Basis->Release();
						});	
					});
				}
				void LoadExternalScript(const Rml::String& Path) override
				{
					ScopedContext* Scope = (ScopedContext*)GetContext();
					VI_ASSERT_V(Scope && Scope->Basis && Scope->Basis->Compiler, "context should be scoped");

					Scripting::Compiler* Compiler = Scope->Basis->Compiler;
					if (Compiler->LoadFile(Core::Stringify(Path).Replace('|', ':').R()) < 0)
						return;

					Compiler->Compile().When([Scope, Compiler](int&& Status)
					{
						if (Status < 0)
							return;

						Scripting::Function Main = Compiler->GetModule().GetFunctionByName("main");
						Scripting::FunctionDelegate Delegate(Main);
						if (!Delegate.IsValid())
							return;

						bool HasArguments = Main.GetArgsCount() > 0;
						Delegate([HasArguments, Scope](Scripting::ImmediateContext* Context)
						{
							Scope->Basis->AddRef();
							if (HasArguments)
								Context->SetArgObject(0, Scope->Basis);
						}, [Scope](Scripting::ImmediateContext*)
						{
							Scope->Basis->Release();
						});
					});
				}
			};

			class DocumentInstancer final : public Rml::ElementInstancer
			{
			public:
				Rml::ElementPtr InstanceElement(Rml::Element*, const Rml::String& Tag, const Rml::XMLAttributes&) override
				{
					return Rml::ElementPtr(VI_NEW(DocumentSubsystem, Tag));
				}
				void ReleaseElement(Rml::Element* Element) override
				{
					VI_DELETE(Element, Element);
				}
			};

			class ListenerSubsystem final : public Rml::EventListener
			{
			public:
				Scripting::Function Function;
				Core::String Memory;

			public:
				ListenerSubsystem(const Rml::String& Code, Rml::Element* Element) : Function(nullptr), Memory(Code)
				{
				}
				void OnDetach(Rml::Element* Element) override
				{
					Function.Release();
					VI_DELETE(ListenerSubsystem, this);
				}
				void ProcessEvent(Rml::Event& Event) override
				{
					ScopedContext* Scope = (ScopedContext*)Event.GetCurrentElement()->GetContext();
					VI_ASSERT_V(Scope && Scope->Basis && Scope->Basis->Compiler, "context should be scoped");
					if (!CompileInline(Scope))
						return;

					Scripting::FunctionDelegate Delegate(Function);
					if (!Delegate.IsValid())
						return;

					Rml::Event* Ptr = Rml::Factory::InstanceEvent(Event.GetTargetElement(), Event.GetId(), Event.GetType(), Event.GetParameters(), Event.IsInterruptible()).release();
					if (Ptr != nullptr)
					{
						Ptr->SetCurrentElement(Event.GetCurrentElement());
						Ptr->SetPhase(Event.GetPhase());
					}

					Scope->Basis->AddRef();
					Delegate([Ptr](Scripting::ImmediateContext* Context)
					{
						IEvent Event(Ptr);
						Context->SetArgObject(0, &Event);
					}, [Scope, Ptr](Scripting::ImmediateContext*)
					{
						Scope->Basis->Release();
						delete Ptr;
					});
				}
				bool CompileInline(ScopedContext* Scope)
				{
					if (Function.IsValid())
						return true;

					Core::String Name = "__vf" + Compute::Crypto::Hash(Compute::Digests::MD5(), Memory);
					Core::String Eval = "void " + Name + "(ui_event&in event){\n";
					Eval.append(Memory);
					Eval += "\n;}";

					Scripting::Module Module = Scope->Basis->Compiler->GetModule();
					return Module.CompileFunction(Name.c_str(), Eval.c_str(), -1, (size_t)Scripting::CompileFlags::ADD_TO_MODULE, &Function) >= 0;
				}
			};

			class ListenerInstancer final : public Rml::EventListenerInstancer
			{
			public:
				Rml::EventListener* InstanceEventListener(const Rml::String& Value, Rml::Element* Element) override
				{
					return VI_NEW(ListenerSubsystem, Value, Element);
				}
			};

			class EventSubsystem final : public Rml::EventListener
			{
				friend IEvent;

			private:
				EventCallback Listener;
				std::atomic<int> RefCount;

			public:
				EventSubsystem(const EventCallback& Callback) : Rml::EventListener(), Listener(Callback), RefCount(1)
				{
				}
				void OnAttach(Rml::Element*) override
				{
					RefCount++;
				}
				void OnDetach(Rml::Element*) override
				{
					if (!--RefCount)
						VI_DELETE(EventSubsystem, this);
				}
				void ProcessEvent(Rml::Event& Event) override
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
#ifdef VI_USE_RMLUI
				VI_ASSERT_V(From && To, "from and to should be set");
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
						*To = Core::Var::String(From->Get<Core::String>());
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
#ifdef VI_USE_RMLUI
				VI_ASSERT_V(From && To, "from and to should be set");
				switch (From->GetType())
				{
					case Core::VarType::Null:
						*To = Rml::Variant((void*)nullptr);
						break;
					case Core::VarType::String:
					{
						Core::String Blob = From->GetBlob();
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
			Compute::Vector4 IVariant::ToColor4(const Core::String& Value)
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
			Core::String IVariant::FromColor4(const Compute::Vector4& Base, bool HEX)
			{
				if (!HEX)
					return Core::Form("%d %d %d %d", (unsigned int)(Base.X * 255.0f), (unsigned int)(Base.Y * 255.0f), (unsigned int)(Base.Z * 255.0f), (unsigned int)(Base.W * 255.0f)).R();

				return Core::Form("#%02x%02x%02x%02x",
					(unsigned int)(Base.X * 255.0f),
					(unsigned int)(Base.Y * 255.0f),
					(unsigned int)(Base.Z * 255.0f),
					(unsigned int)(Base.W * 255.0f)).R();
			}
			Compute::Vector4 IVariant::ToColor3(const Core::String& Value)
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
			Core::String IVariant::FromColor3(const Compute::Vector4& Base, bool HEX)
			{
				if (!HEX)
					return Core::Form("%d %d %d", (unsigned int)(Base.X * 255.0f), (unsigned int)(Base.Y * 255.0f), (unsigned int)(Base.Z * 255.0f)).R();

				return Core::Form("#%02x%02x%02x",
					(unsigned int)(Base.X * 255.0f),
					(unsigned int)(Base.Y * 255.0f),
					(unsigned int)(Base.Z * 255.0f)).R();
			}
			int IVariant::GetVectorType(const Core::String& Value)
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
			Compute::Vector4 IVariant::ToVector4(const Core::String& Base)
			{
				Compute::Vector4 Result;
				if (sscanf(Base.c_str(), "v4 %f %f %f %f", &Result.X, &Result.Y, &Result.Z, &Result.W) != 4)
					return Result;

				return Result;
			}
			Core::String IVariant::FromVector4(const Compute::Vector4& Base)
			{
				return Core::Form("v4 %f %f %f %f", Base.X, Base.Y, Base.Z, Base.W).R();
			}
			Compute::Vector3 IVariant::ToVector3(const Core::String& Base)
			{
				Compute::Vector3 Result;
				if (sscanf(Base.c_str(), "v3 %f %f %f", &Result.X, &Result.Y, &Result.Z) != 3)
					return Result;

				return Result;
			}
			Core::String IVariant::FromVector3(const Compute::Vector3& Base)
			{
				return Core::Form("v3 %f %f %f", Base.X, Base.Y, Base.Z).R();
			}
			Compute::Vector2 IVariant::ToVector2(const Core::String& Base)
			{
				Compute::Vector2 Result;
				if (sscanf(Base.c_str(), "v2 %f %f", &Result.X, &Result.Y) != 2)
					return Result;

				return Result;
			}
			Core::String IVariant::FromVector2(const Compute::Vector2& Base)
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
#ifdef VI_USE_RMLUI
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
#ifdef VI_USE_RMLUI
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
#ifdef VI_USE_RMLUI
				VI_ASSERT(IsValid(), EventPhase::None, "event should be valid");
				return (EventPhase)Base->GetPhase();
#else
				return EventPhase::None;
#endif
			}
			void IEvent::SetPhase(EventPhase Phase)
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT_V(IsValid(), "event should be valid");
				Base->SetPhase((Rml::EventPhase)Phase);
#endif
			}
			void IEvent::SetCurrentElement(const IElement& Element)
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT_V(IsValid(), "event should be valid");
				Base->SetCurrentElement(Element.GetElement());
#endif
			}
			IElement IEvent::GetCurrentElement() const
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT(IsValid(), nullptr, "event should be valid");
				return Base->GetCurrentElement();
#else
				return IElement();
#endif
			}
			IElement IEvent::GetTargetElement() const
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT(IsValid(), nullptr, "event should be valid");
				return Base->GetTargetElement();
#else
				return IElement();
#endif
			}
			Core::String IEvent::GetType() const
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT(IsValid(), Core::String(), "event should be valid");
				return Base->GetType();
#else
				return Core::String();
#endif
			}
			void IEvent::StopPropagation()
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT_V(IsValid(), "event should be valid");
				Base->StopPropagation();
#endif
			}
			void IEvent::StopImmediatePropagation()
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT_V(IsValid(), "event should be valid");
				Base->StopImmediatePropagation();
#endif
			}
			bool IEvent::IsInterruptible() const
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT(IsValid(), false, "event should be valid");
				return Base->IsInterruptible();
#else
				return false;
#endif
			}
			bool IEvent::IsPropagating() const
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT(IsValid(), false, "event should be valid");
				return Base->IsPropagating();
#else
				return false;
#endif
			}
			bool IEvent::IsImmediatePropagating() const
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT(IsValid(), false, "event should be valid");
				return Base->IsImmediatePropagating();
#else
				return false;
#endif
			}
			bool IEvent::GetBoolean(const Core::String& Key) const
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT(IsValid(), false, "event should be valid");
				return Base->GetParameter<bool>(Key, false);
#else
				return false;
#endif
			}
			int64_t IEvent::GetInteger(const Core::String& Key) const
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT(IsValid(), 0, "event should be valid");
				return Base->GetParameter<int64_t>(Key, 0);
#else
				return 0;
#endif
			}
			double IEvent::GetNumber(const Core::String& Key) const
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT(IsValid(), 0.0, "event should be valid");
				return Base->GetParameter<double>(Key, 0.0);
#else
				return 0.0;
#endif
			}
			Core::String IEvent::GetString(const Core::String& Key) const
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT(IsValid(), Core::String(), "event should be valid");
				return Base->GetParameter<Rml::String>(Key, "");
#else
				return Core::String();
#endif
			}
			Compute::Vector2 IEvent::GetVector2(const Core::String& Key) const
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT(IsValid(), Compute::Vector2(), "event should be valid");
				Rml::Vector2f Result = Base->GetParameter<Rml::Vector2f>(Key, Rml::Vector2f());
				return Compute::Vector2(Result.x, Result.y);
#else
				return Compute::Vector2();
#endif
			}
			Compute::Vector3 IEvent::GetVector3(const Core::String& Key) const
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT(IsValid(), Compute::Vector3(), "event should be valid");
				Rml::Vector3f Result = Base->GetParameter<Rml::Vector3f>(Key, Rml::Vector3f());
				return Compute::Vector3(Result.x, Result.y, Result.z);
#else
				return Compute::Vector3();
#endif
			}
			Compute::Vector4 IEvent::GetVector4(const Core::String& Key) const
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT(IsValid(), Compute::Vector4(), "event should be valid");
				Rml::Vector4f Result = Base->GetParameter<Rml::Vector4f>(Key, Rml::Vector4f());
				return Compute::Vector4(Result.x, Result.y, Result.z, Result.w);
#else
				return Compute::Vector4();
#endif
			}
			void* IEvent::GetPointer(const Core::String& Key) const
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT(IsValid(), nullptr, "event should be valid");
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
#ifdef VI_USE_RMLUI
				VI_DELETE(Element, Base);
				Base = nullptr;
#endif
			}
			IElement IElement::Clone() const
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT(IsValid(), nullptr, "element should be valid");
				Rml::ElementPtr Ptr = Base->Clone();
				Rml::Element* Result = Ptr.get();
				Ptr.reset();

				return Result;
#else
				return IElement();
#endif
			}
			void IElement::SetClass(const Core::String& ClassName, bool Activate)
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT_V(IsValid(), "element should be valid");
				Base->SetClass(ClassName, Activate);
#endif
			}
			bool IElement::IsClassSet(const Core::String& ClassName) const
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT(IsValid(), false, "element should be valid");
				return Base->IsClassSet(ClassName);
#else
				return false;
#endif
			}
			void IElement::SetClassNames(const Core::String& ClassNames)
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT_V(IsValid(), "element should be valid");
				Base->SetClassNames(ClassNames);
#endif
			}
			Core::String IElement::GetClassNames() const
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT(IsValid(), Core::String(), "element should be valid");
				return Base->GetClassNames();
#else
				return Core::String();
#endif

			}
			Core::String IElement::GetAddress(bool IncludePseudoClasses, bool IncludeParents) const
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT(IsValid(), Core::String(), "element should be valid");
				return Base->GetAddress(IncludePseudoClasses, IncludeParents);
#else
				return Core::String();
#endif

			}
			void IElement::SetOffset(const Compute::Vector2& Offset, const IElement& OffsetParent, bool OffsetFixed)
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT_V(IsValid(), "element should be valid");
				Base->SetOffset(Rml::Vector2f(Offset.X, Offset.Y), OffsetParent.GetElement(), OffsetFixed);
#endif
			}
			Compute::Vector2 IElement::GetRelativeOffset(Area Type)
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT(IsValid(), Compute::Vector2(), "element should be valid");
				Rml::Vector2f Result = Base->GetRelativeOffset((Rml::Box::Area)Type);
				return Compute::Vector2(Result.x, Result.y);
#else
				return Compute::Vector2();
#endif

			}
			Compute::Vector2 IElement::GetAbsoluteOffset(Area Type)
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT(IsValid(), Compute::Vector2(), "element should be valid");
				Rml::Vector2f Result = Base->GetAbsoluteOffset((Rml::Box::Area)Type);
				return Compute::Vector2(Result.x, Result.y);
#else
				return Compute::Vector2();
#endif

			}
			void IElement::SetClientArea(Area ClientArea)
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT_V(IsValid(), "element should be valid");
				Base->SetClientArea((Rml::Box::Area)ClientArea);
#endif
			}
			Area IElement::GetClientArea() const
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT(IsValid(), Area::Content, "element should be valid");
				return (Area)Base->GetClientArea();
#else
				return Area::Margin;
#endif

			}
			void IElement::SetContentBox(const Compute::Vector2& ContentOffset, const Compute::Vector2& ContentBox)
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT_V(IsValid(), "element should be valid");
				Base->SetContentBox(Rml::Vector2f(ContentOffset.X, ContentOffset.Y), Rml::Vector2f(ContentBox.X, ContentBox.Y));
#endif
			}
			float IElement::GetBaseline() const
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT(IsValid(), 0.0f, "element should be valid");
				return Base->GetBaseline();
#else
				return 0.0f;
#endif

			}
			bool IElement::GetIntrinsicDimensions(Compute::Vector2& Dimensions, float& Ratio)
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT(IsValid(), false, "element should be valid");
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
#ifdef VI_USE_RMLUI
				VI_ASSERT(IsValid(), false, "element should be valid");
				return Base->IsPointWithinElement(Rml::Vector2f(Point.X, Point.Y));
#else
				return false;
#endif

			}
			bool IElement::IsVisible() const
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT(IsValid(), false, "element should be valid");
				return Base->IsVisible();
#else
				return false;
#endif

			}
			float IElement::GetZIndex() const
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT(IsValid(), 0.0f, "element should be valid");
				return Base->GetZIndex();
#else
				return 0.0f;
#endif

			}
			bool IElement::SetProperty(const Core::String& Name, const Core::String& Value)
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT(IsValid(), false, "element should be valid");
				return Base->SetProperty(Name, Value);
#else
				return false;
#endif

			}
			void IElement::RemoveProperty(const Core::String& Name)
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT_V(IsValid(), "element should be valid");
				Base->RemoveProperty(Name);
#endif
			}
			Core::String IElement::GetProperty(const Core::String& Name)
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT(IsValid(), Core::String(), "element should be valid");
				const Rml::Property* Result = Base->GetProperty(Name);
				if (!Result)
					return "";

				return Result->ToString();
#else
				return Core::String();
#endif
			}
			Core::String IElement::GetLocalProperty(const Core::String& Name)
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT(IsValid(), Core::String(), "element should be valid");
				const Rml::Property* Result = Base->GetLocalProperty(Name);
				if (!Result)
					return "";

				return Result->ToString();
#else
				return Core::String();
#endif
			}
			float IElement::ResolveNumericProperty(const Core::String& PropertyName)
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT(IsValid(), 0.0f, "element should be valid");
				return Base->ResolveNumericProperty(PropertyName);
#else
				return 0.0f;
#endif
			}
			Compute::Vector2 IElement::GetContainingBlock()
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT(IsValid(), Compute::Vector2(), "element should be valid");
				Rml::Vector2f Result = Base->GetContainingBlock();
				return Compute::Vector2(Result.x, Result.y);
#else
				return Compute::Vector2();
#endif
			}
			Position IElement::GetPosition()
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT(IsValid(), Position::Static, "element should be valid");
				return (Position)Base->GetPosition();
#else
				return Position::Static;
#endif
			}
			Float IElement::GetFloat()
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT(IsValid(), Float::None, "element should be valid");
				return (Float)Base->GetFloat();
#else
				return Float::None;
#endif
			}
			Display IElement::GetDisplay()
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT(IsValid(), Display::None, "element should be valid");
				return (Display)Base->GetDisplay();
#else
				return Display::None;
#endif
			}
			float IElement::GetLineHeight()
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT(IsValid(), 0.0f, "element should be valid");
				return Base->GetLineHeight();
#else
				return 0.0f;
#endif
			}
			bool IElement::Project(Compute::Vector2& Point) const noexcept
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT(IsValid(), false, "element should be valid");
				Rml::Vector2f Offset(Point.X, Point.Y);
				bool Result = Base->Project(Offset);
				Point = Compute::Vector2(Offset.x, Offset.y);

				return Result;
#else
				return false;
#endif
			}
			bool IElement::Animate(const Core::String& PropertyName, const Core::String& TargetValue, float Duration, TimingFunc Func, TimingDir Dir, int NumIterations, bool AlternateDirection, float Delay)
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT(IsValid(), false, "element should be valid");
				return Base->Animate(PropertyName, Rml::Property(TargetValue, Rml::Property::TRANSFORM), Duration, Rml::Tween((Rml::Tween::Type)Func, (Rml::Tween::Direction)Dir), NumIterations, AlternateDirection, Delay);
#else
				return false;
#endif
			}
			bool IElement::AddAnimationKey(const Core::String& PropertyName, const Core::String& TargetValue, float Duration, TimingFunc Func, TimingDir Dir)
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT(IsValid(), false, "element should be valid");
				return Base->AddAnimationKey(PropertyName, Rml::Property(TargetValue, Rml::Property::TRANSFORM), Duration, Rml::Tween((Rml::Tween::Type)Func, (Rml::Tween::Direction)Dir));
#else
				return false;
#endif
			}
			void IElement::SetPseudoClass(const Core::String& PseudoClass, bool Activate)
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT_V(IsValid(), "element should be valid");
				Base->SetPseudoClass(PseudoClass, Activate);
#endif
			}
			bool IElement::IsPseudoClassSet(const Core::String& PseudoClass) const
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT(IsValid(), false, "element should be valid");
				return Base->IsPseudoClassSet(PseudoClass);
#else
				return false;
#endif
			}
			void IElement::SetAttribute(const Core::String& Name, const Core::String& Value)
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT_V(IsValid(), "element should be valid");
				Base->SetAttribute(Name, Value);
#endif
			}
			Core::String IElement::GetAttribute(const Core::String& Name)
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT(IsValid(), Core::String(), "element should be valid");
				return Base->GetAttribute<Core::String>(Name, "");
#else
				return Core::String();
#endif
			}
			bool IElement::HasAttribute(const Core::String& Name) const
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT(IsValid(), false, "element should be valid");
				return Base->HasAttribute(Name);
#else
				return false;
#endif
			}
			void IElement::RemoveAttribute(const Core::String& Name)
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT_V(IsValid(), "element should be valid");
				Base->RemoveAttribute(Name);
#endif
			}
			IElement IElement::GetFocusLeafNode()
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT(IsValid(), nullptr, "element should be valid");
				return Base->GetFocusLeafNode();
#else
				return IElement();
#endif
			}
			Core::String IElement::GetTagName() const
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT(IsValid(), Core::String(), "element should be valid");
				return Base->GetTagName();
#else
				return Core::String();
#endif
			}
			Core::String IElement::GetId() const
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT(IsValid(), Core::String(), "element should be valid");
				return Base->GetId();
#else
				return Core::String();
#endif
			}
			void IElement::SetId(const Core::String& Id)
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT_V(IsValid(), "element should be valid");
				Base->SetId(Id);
#endif
			}
			float IElement::GetAbsoluteLeft()
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT(IsValid(), 0.0f, "element should be valid");
				return Base->GetAbsoluteLeft();
#else
				return 0.0f;
#endif
			}
			float IElement::GetAbsoluteTop()
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT(IsValid(), 0.0f, "element should be valid");
				return Base->GetAbsoluteTop();
#else
				return 0.0f;
#endif
			}
			float IElement::GetClientLeft()
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT(IsValid(), 0.0f, "element should be valid");
				return Base->GetClientLeft();
#else
				return 0.0f;
#endif
			}
			float IElement::GetClientTop()
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT(IsValid(), 0.0f, "element should be valid");
				return Base->GetClientTop();
#else
				return 0.0f;
#endif
			}
			float IElement::GetClientWidth()
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT(IsValid(), 0.0f, "element should be valid");
				return Base->GetClientWidth();
#else
				return 0.0f;
#endif
			}
			float IElement::GetClientHeight()
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT(IsValid(), 0.0f, "element should be valid");
				return Base->GetClientHeight();
#else
				return 0.0f;
#endif
			}
			IElement IElement::GetOffsetParent()
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT(IsValid(), nullptr, "element should be valid");
				return Base->GetOffsetParent();
#else
				return IElement();
#endif
			}
			float IElement::GetOffsetLeft()
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT(IsValid(), 0.0f, "element should be valid");
				return Base->GetOffsetLeft();
#else
				return 0.0f;
#endif
			}
			float IElement::GetOffsetTop()
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT(IsValid(), 0.0f, "element should be valid");
				return Base->GetOffsetTop();
#else
				return 0.0f;
#endif
			}
			float IElement::GetOffsetWidth()
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT(IsValid(), 0.0f, "element should be valid");
				return Base->GetOffsetWidth();
#else
				return 0.0f;
#endif
			}
			float IElement::GetOffsetHeight()
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT(IsValid(), 0.0f, "element should be valid");
				return Base->GetOffsetHeight();
#else
				return 0.0f;
#endif
			}
			float IElement::GetScrollLeft()
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT(IsValid(), 0.0f, "element should be valid");
				return Base->GetScrollLeft();
#else
				return 0.0f;
#endif
			}
			void IElement::SetScrollLeft(float ScrollLeft)
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT_V(IsValid(), "element should be valid");
				Base->SetScrollLeft(ScrollLeft);
#endif
			}
			float IElement::GetScrollTop()
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT(IsValid(), 0.0f, "element should be valid");
				return Base->GetScrollTop();
#else
				return 0.0f;
#endif
			}
			void IElement::SetScrollTop(float ScrollTop)
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT_V(IsValid(), "element should be valid");
				Base->SetScrollTop(ScrollTop);
#endif
			}
			float IElement::GetScrollWidth()
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT(IsValid(), 0.0f, "element should be valid");
				return Base->GetScrollWidth();
#else
				return 0.0f;
#endif
			}
			float IElement::GetScrollHeight()
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT(IsValid(), 0.0f, "element should be valid");
				return Base->GetScrollHeight();
#else
				return 0.0f;
#endif
			}
			IElementDocument IElement::GetOwnerDocument() const
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT(IsValid(), nullptr, "element should be valid");
				return Base->GetOwnerDocument();
#else
				return IElementDocument();
#endif
			}
			IElement IElement::GetParentNode() const
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT(IsValid(), nullptr, "element should be valid");
				return Base->GetParentNode();
#else
				return IElement();
#endif
			}
			IElement IElement::GetNextSibling() const
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT(IsValid(), nullptr, "element should be valid");
				return Base->GetNextSibling();
#else
				return IElement();
#endif
			}
			IElement IElement::GetPreviousSibling() const
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT(IsValid(), nullptr, "element should be valid");
				return Base->GetPreviousSibling();
#else
				return IElement();
#endif
			}
			IElement IElement::GetFirstChild() const
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT(IsValid(), nullptr, "element should be valid");
				return Base->GetFirstChild();
#else
				return IElement();
#endif
			}
			IElement IElement::GetLastChild() const
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT(IsValid(), nullptr, "element should be valid");
				return Base->GetLastChild();
#else
				return IElement();
#endif
			}
			IElement IElement::GetChild(int Index) const
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT(IsValid(), nullptr, "element should be valid");
				return Base->GetChild(Index);
#else
				return IElement();
#endif
			}
			int IElement::GetNumChildren(bool IncludeNonDOMElements) const
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT(IsValid(), 0, "element should be valid");
				return Base->GetNumChildren(IncludeNonDOMElements);
#else
				return 0;
#endif
			}
			void IElement::GetInnerHTML(Core::String& Content) const
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT_V(IsValid(), "element should be valid");
				Base->GetInnerRML(Content);
#endif
			}
			Core::String IElement::GetInnerHTML() const
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT(IsValid(), Core::String(), "element should be valid");
				return Base->GetInnerRML();
#else
				return Core::String();
#endif
			}
			void IElement::SetInnerHTML(const Core::String& HTML)
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT_V(IsValid(), "element should be valid");
				Base->SetInnerRML(HTML);
#endif
			}
			bool IElement::IsFocused()
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT(IsValid(), false, "element should be valid");
				return Base->IsPseudoClassSet("focus");
#else
				return false;
#endif
			}
			bool IElement::IsHovered()
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT(IsValid(), false, "element should be valid");
				return Base->IsPseudoClassSet("hover");
#else
				return false;
#endif
			}
			bool IElement::IsActive()
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT(IsValid(), false, "element should be valid");
				return Base->IsPseudoClassSet("active");
#else
				return false;
#endif
			}
			bool IElement::IsChecked()
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT(IsValid(), false, "element should be valid");
				return Base->IsPseudoClassSet("checked");
#else
				return false;
#endif
			}
			bool IElement::Focus()
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT(IsValid(), false, "element should be valid");
				return Base->Focus();
#else
				return false;
#endif
			}
			void IElement::Blur()
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT_V(IsValid(), "element should be valid");
				Base->Blur();
#endif
			}
			void IElement::Click()
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT_V(IsValid(), "element should be valid");
				Base->Click();
#endif
			}
			void IElement::AddEventListener(const Core::String& Event, Listener* Source, bool InCapturePhase)
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT_V(IsValid(), "element should be valid");
				VI_ASSERT_V(Source != nullptr && Source->Base != nullptr, "listener should be set");

				Base->AddEventListener(Event, Source->Base, InCapturePhase);
#endif
			}
			void IElement::RemoveEventListener(const Core::String& Event, Listener* Source, bool InCapturePhase)
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT_V(IsValid(), "element should be valid");
				VI_ASSERT_V(Source != nullptr && Source->Base != nullptr, "listener should be set");

				Base->RemoveEventListener(Event, Source->Base, InCapturePhase);
#endif
			}
			bool IElement::DispatchEvent(const Core::String& Type, const Core::VariantArgs& Args)
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT(IsValid(), false, "element should be valid");
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
#ifdef VI_USE_RMLUI
				VI_ASSERT_V(IsValid(), "element should be valid");
				Base->ScrollIntoView(AlignWithTop);
#endif
			}
			IElement IElement::AppendChild(const IElement& Element, bool DOMElement)
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT(IsValid(), nullptr, "element should be valid");
				return Base->AppendChild(Rml::ElementPtr(Element.GetElement()), DOMElement);
#else
				return IElement();
#endif
			}
			IElement IElement::InsertBefore(const IElement& Element, const IElement& AdjacentElement)
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT(IsValid(), nullptr, "element should be valid");
				return Base->InsertBefore(Rml::ElementPtr(Element.GetElement()), AdjacentElement.GetElement());
#else
				return IElement();
#endif
			}
			IElement IElement::ReplaceChild(const IElement& InsertedElement, const IElement& ReplacedElement)
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT(IsValid(), nullptr, "element should be valid");
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
#ifdef VI_USE_RMLUI
				VI_ASSERT(IsValid(), nullptr, "element should be valid");
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
#ifdef VI_USE_RMLUI
				VI_ASSERT(IsValid(), false, "element should be valid");
				return Base->HasChildNodes();
#else
				return false;
#endif
			}
			IElement IElement::GetElementById(const Core::String& Id)
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT(IsValid(), nullptr, "element should be valid");
				return Base->GetElementById(Id);
#else
				return IElement();
#endif
			}
			IElement IElement::QuerySelector(const Core::String& Selector)
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT(IsValid(), nullptr, "element should be valid");
				return Base->QuerySelector(Selector);
#else
				return IElement();
#endif
			}
			Core::Vector<IElement> IElement::QuerySelectorAll(const Core::String& Selectors)
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT(IsValid(), Core::Vector<IElement>(), "element should be valid");
				Rml::ElementList Elements;
				Base->QuerySelectorAll(Elements, Selectors);

				Core::Vector<IElement> Result;
				Result.reserve(Elements.size());

				for (auto& Item : Elements)
					Result.push_back(Item);

				return Result;
#else
				return Core::Vector<IElement>();
#endif
			}
			bool IElement::CastFormColor(Compute::Vector4* Ptr, bool Alpha)
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT(IsValid(), false, "element should be valid");
				VI_ASSERT(Ptr != nullptr, false, "ptr should be set");

				Rml::ElementFormControl* Form = (Rml::ElementFormControl*)Base;
				Core::String Value = Form->GetValue();
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
			bool IElement::CastFormString(Core::String* Ptr)
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT(IsValid(), false, "element should be valid");
				VI_ASSERT(Ptr != nullptr, false, "ptr should be set");

				Rml::ElementFormControl* Form = (Rml::ElementFormControl*)Base;
				Core::String Value = Form->GetValue();
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
#ifdef VI_USE_RMLUI
				VI_ASSERT(IsValid(), false, "element should be valid");
				VI_ASSERT(Ptr != nullptr, false, "ptr should be set");

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
#ifdef VI_USE_RMLUI
				VI_ASSERT(IsValid(), false, "element should be valid");
				VI_ASSERT(Ptr != nullptr, false, "ptr should be set");

				Rml::ElementFormControl* Form = (Rml::ElementFormControl*)Base;
				Core::Stringify Value(Form->GetValue());
				if (Value.Empty())
				{
					if (Form->IsPseudoClassSet("focus"))
						return false;

					Form->SetValue(Core::ToString(*Ptr));
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

				Form->SetValue(Core::ToString(*Ptr));
				return false;
#else
				return false;
#endif
			}
			bool IElement::CastFormUInt32(uint32_t* Ptr)
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT(IsValid(), false, "element should be valid");
				VI_ASSERT(Ptr != nullptr, false, "ptr should be set");

				Rml::ElementFormControl* Form = (Rml::ElementFormControl*)Base;
				Core::Stringify Value(Form->GetValue());
				if (Value.Empty())
				{
					if (Form->IsPseudoClassSet("focus"))
						return false;

					Form->SetValue(Core::ToString(*Ptr));
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

				Form->SetValue(Core::ToString(*Ptr));
				return false;
#else
				return false;
#endif
			}
			bool IElement::CastFormFlag32(uint32_t* Ptr, uint32_t Mask)
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT(IsValid(), false, "element should be valid");
				VI_ASSERT(Ptr != nullptr, false, "ptr should be set");

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
#ifdef VI_USE_RMLUI
				VI_ASSERT(IsValid(), false, "element should be valid");
				VI_ASSERT(Ptr != nullptr, false, "ptr should be set");

				Rml::ElementFormControl* Form = (Rml::ElementFormControl*)Base;
				Core::Stringify Value(Form->GetValue());
				if (Value.Empty())
				{
					if (Form->IsPseudoClassSet("focus"))
						return false;

					Form->SetValue(Core::ToString(*Ptr));
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

				Form->SetValue(Core::ToString(*Ptr));
				return false;
#else
				return false;
#endif
			}
			bool IElement::CastFormUInt64(uint64_t* Ptr)
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT(IsValid(), false, "element should be valid");
				VI_ASSERT(Ptr != nullptr, false, "ptr should be set");

				Rml::ElementFormControl* Form = (Rml::ElementFormControl*)Base;
				Core::Stringify Value(Form->GetValue());
				if (Value.Empty())
				{
					if (Form->IsPseudoClassSet("focus"))
						return false;

					Form->SetValue(Core::ToString(*Ptr));
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

				Form->SetValue(Core::ToString(*Ptr));
				return false;
#else
				return false;
#endif
			}
			bool IElement::CastFormSize(size_t* Ptr)
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT(IsValid(), false, "element should be valid");
				VI_ASSERT(Ptr != nullptr, false, "ptr should be set");

				Rml::ElementFormControl* Form = (Rml::ElementFormControl*)Base;
				Core::Stringify Value(Form->GetValue());
				if (Value.Empty())
				{
					if (Form->IsPseudoClassSet("focus"))
						return false;

					Form->SetValue(Core::ToString(*Ptr));
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

				Form->SetValue(Core::ToString(*Ptr));
				return false;
#else
				return false;
#endif
			}
			bool IElement::CastFormFlag64(uint64_t* Ptr, uint64_t Mask)
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT(IsValid(), false, "element should be valid");
				VI_ASSERT(Ptr != nullptr, false, "ptr should be set");

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
#ifdef VI_USE_RMLUI
				VI_ASSERT(IsValid(), false, "element should be valid");
				VI_ASSERT(Ptr != nullptr, false, "ptr should be set");

				Rml::ElementFormControl* Form = (Rml::ElementFormControl*)Base;
				Core::Stringify Value(Form->GetValue());
				if (Value.Empty())
				{
					if (Form->IsPseudoClassSet("focus"))
						return false;

					Form->SetValue(Core::Stringify::ToString(*Ptr));
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

				Form->SetValue(Core::Stringify::ToString(*Ptr));
				return false;
#else
				return false;
#endif
			}
			bool IElement::CastFormFloat(float* Ptr, float Mult)
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT(IsValid(), false, "element should be valid");
				VI_ASSERT(Ptr != nullptr, false, "ptr should be set");

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
#ifdef VI_USE_RMLUI
				VI_ASSERT(IsValid(), false, "element should be valid");
				VI_ASSERT(Ptr != nullptr, false, "ptr should be set");

				Rml::ElementFormControl* Form = (Rml::ElementFormControl*)Base;
				Core::Stringify Value(Form->GetValue());
				if (Value.Empty())
				{
					if (Form->IsPseudoClassSet("focus"))
						return false;

					Form->SetValue(Core::Stringify::ToString(*Ptr));
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

				Form->SetValue(Core::Stringify::ToString(*Ptr));
				return false;
#else
				return false;
#endif
			}
			bool IElement::CastFormBoolean(bool* Ptr)
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT(IsValid(), false, "element should be valid");
				VI_ASSERT(Ptr != nullptr, false, "ptr should be set");

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
			Core::String IElement::GetFormName() const
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT(IsValid(), Core::String(), "element should be valid");
				Rml::ElementFormControl* Form = (Rml::ElementFormControl*)Base;
				return Form->GetName();
#else
				return Core::String();
#endif
			}
			void IElement::SetFormName(const Core::String& Name)
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT_V(IsValid(), "element should be valid");
				Rml::ElementFormControl* Form = (Rml::ElementFormControl*)Base;
				Form->SetName(Name);
#endif
			}
			Core::String IElement::GetFormValue() const
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT(IsValid(), Core::String(), "element should be valid");
				Rml::ElementFormControl* Form = (Rml::ElementFormControl*)Base;
				return Form->GetValue();
#else
				return Core::String();
#endif
			}
			void IElement::SetFormValue(const Core::String& Value)
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT_V(IsValid(), "element should be valid");
				Rml::ElementFormControl* Form = (Rml::ElementFormControl*)Base;
				Form->SetValue(Value);
#endif
			}
			bool IElement::IsFormDisabled() const
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT(IsValid(), false, "element should be valid");
				Rml::ElementFormControl* Form = (Rml::ElementFormControl*)Base;
				return Form->IsDisabled();
#else
				return false;
#endif
			}
			void IElement::SetFormDisabled(bool Disable)
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT_V(IsValid(), "element should be valid");
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
			Core::String IElement::FromPointer(void* Ptr)
			{
				if (!Ptr)
					return "0";

				return Core::ToString((intptr_t)(void*)Ptr);
			}
			void* IElement::ToPointer(const Core::String& Value)
			{
				if (Value.empty())
					return nullptr;

				Core::Stringify Buffer(&Value);
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
#ifdef VI_USE_RMLUI
				Rml::ElementDocument* Item = (Rml::ElementDocument*)Base;
				VI_DELETE(ElementDocument, Item);
				Base = nullptr;
#endif
			}
			void IElementDocument::SetTitle(const Core::String& Title)
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT_V(IsValid(), "element should be valid");
				((Rml::ElementDocument*)Base)->SetTitle(Title);
#endif
			}
			void IElementDocument::PullToFront()
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT_V(IsValid(), "element should be valid");
				((Rml::ElementDocument*)Base)->PullToFront();
#endif
			}
			void IElementDocument::PushToBack()
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT_V(IsValid(), "element should be valid");
				((Rml::ElementDocument*)Base)->PushToBack();
#endif
			}
			void IElementDocument::Show(ModalFlag Modal, FocusFlag Focus)
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT_V(IsValid(), "element should be valid");
				((Rml::ElementDocument*)Base)->Show((Rml::ModalFlag)Modal, (Rml::FocusFlag)Focus);
#endif
			}
			void IElementDocument::Hide()
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT_V(IsValid(), "element should be valid");
				((Rml::ElementDocument*)Base)->Hide();
#endif
			}
			void IElementDocument::Close()
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT_V(IsValid(), "element should be valid");
				((Rml::ElementDocument*)Base)->Close();
#endif
			}
			Core::String IElementDocument::GetTitle() const
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT(IsValid(), Core::String(), "element should be valid");
				return ((Rml::ElementDocument*)Base)->GetTitle();
#else
				return Core::String();
#endif
			}
			Core::String IElementDocument::GetSourceURL() const
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT(IsValid(), Core::String(), "element should be valid");
				return ((Rml::ElementDocument*)Base)->GetSourceURL();
#else
				return Core::String();
#endif
			}
			IElement IElementDocument::CreateElement(const Core::String& Name)
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT(IsValid(), nullptr, "element should be valid");
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
#ifdef VI_USE_RMLUI
				VI_ASSERT(IsValid(), false, "element should be valid");
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
#ifdef VI_USE_RMLUI
				State++;
				if (State > 1)
					return true;

				RenderInterface = VI_NEW(RenderSubsystem);
				Rml::SetRenderInterface(RenderInterface);

				FileInterface = VI_NEW(FileSubsystem);
				Rml::SetFileInterface(FileInterface);

				SystemInterface = VI_NEW(MainSubsystem);
				Rml::SetSystemInterface(SystemInterface);

				bool Result = Rml::Initialise();

				ContextFactory = VI_NEW(ContextInstancer);
				Rml::Factory::RegisterContextInstancer(ContextFactory);

				ListenerFactory = VI_NEW(ListenerInstancer);
				Rml::Factory::RegisterEventListenerInstancer(ListenerFactory);

				DocumentFactory = VI_NEW(DocumentInstancer);
				Rml::Factory::RegisterElementInstancer("body", DocumentFactory);

				CreateElements();
				return Result;
#else
				return false;
#endif
			}
			bool Subsystem::Release()
			{
#ifdef VI_USE_RMLUI
				State--;
				if (State > 0 || State < 0)
					return State >= 0;

				Rml::Shutdown();
				if (HasDecorators)
				{
					HasDecorators = false;
					ReleaseDecorators();
				}

				VI_DELETE(MainSubsystem, SystemInterface);
				SystemInterface = nullptr;
				Rml::SetSystemInterface(nullptr);

				VI_DELETE(FileSubsystem, FileInterface);
				FileInterface = nullptr;
				Rml::SetFileInterface(nullptr);

				VI_DELETE(RenderSubsystem, RenderInterface);
				RenderInterface = nullptr;
				Rml::SetRenderInterface(nullptr);

				VI_DELETE(DocumentInstancer, DocumentFactory);
				DocumentFactory = nullptr;

				VI_DELETE(ListenerInstancer, ListenerFactory);
				ListenerFactory = nullptr;

				VI_DELETE(ContextInstancer, ContextFactory);
				ContextFactory = nullptr;

				ReleaseElements();

				ScriptInterface = nullptr;
				return true;
#else
				return false;
#endif
			}
			void Subsystem::SetMetadata(Graphics::Activity* Activity, RenderConstants* Constants, ContentManager* Content, Core::Timer* Time)
			{
#ifdef VI_USE_RMLUI
				if (State == 0 && !Create())
					return;

				if (RenderInterface != nullptr)
				{
					RenderInterface->Attach(Constants, Content);
					if (!HasDecorators && Content && Content->GetDevice())
					{
						HasDecorators = true;
						CreateDecorators(Constants);
					}
				}

				if (SystemInterface != nullptr)
					SystemInterface->Attach(Activity, Time);
#endif
			}
			void Subsystem::SetTranslator(const Core::String& Name, const TranslationCallback& Callback)
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT_V(SystemInterface != nullptr, "system interface should be valid");
				SystemInterface->SetTranslator(Name, Callback);
#endif
			}
			void Subsystem::SetVirtualMachine(Scripting::VirtualMachine* VM)
			{
				ScriptInterface = VM;
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
#ifdef VI_USE_RMLUI
				VI_ASSERT(RenderInterface != nullptr, nullptr, "render interface should be valid");
				return RenderInterface->GetDevice();
#else
				return nullptr;
#endif
			}
			Graphics::Texture2D* Subsystem::GetBackground()
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT(RenderInterface != nullptr, nullptr, "render interface should be valid");
				return RenderInterface->Background;
#else
				return nullptr;
#endif
			}
			Compute::Matrix4x4* Subsystem::GetTransform()
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT(RenderInterface != nullptr, nullptr, "render interface should be valid");
				return RenderInterface->GetTransform();
#else
				return nullptr;
#endif
			}
			Compute::Matrix4x4* Subsystem::GetProjection()
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT(RenderInterface != nullptr, nullptr, "render interface should be valid");
				return RenderInterface->GetProjection();
#else
				return nullptr;
#endif
			}
			Compute::Matrix4x4 Subsystem::ToMatrix(const void* Matrix)
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT(Matrix != nullptr, Compute::Matrix4x4::Identity(), "matrix should be set");
				const Rml::Matrix4f* NewTransform = (const Rml::Matrix4f*)Matrix;
				Rml::Vector4f Row11 = NewTransform->GetRow(0);
				Compute::Matrix4x4 Result;
				Result.Row[0] = Row11.x;
				Result.Row[1] = Row11.y;
				Result.Row[2] = Row11.z;
				Result.Row[3] = Row11.w;

				Rml::Vector4f Row22 = NewTransform->GetRow(1);
				Result.Row[4] = Row22.x;
				Result.Row[5] = Row22.y;
				Result.Row[6] = Row22.z;
				Result.Row[7] = Row22.w;

				Rml::Vector4f Row33 = NewTransform->GetRow(2);
				Result.Row[8] = Row33.x;
				Result.Row[9] = Row33.y;
				Result.Row[10] = Row33.z;
				Result.Row[11] = Row33.w;

				Rml::Vector4f Row44 = NewTransform->GetRow(3);
				Result.Row[12] = Row44.x;
				Result.Row[13] = Row44.y;
				Result.Row[14] = Row44.z;
				Result.Row[15] = Row44.w;

				return Result.Transpose();
#else
				return Compute::Matrix4x4();
#endif
			}
			Core::String Subsystem::EscapeHTML(const Core::String& Text)
			{
				return Core::Stringify(&Text).Replace("\r\n", "&nbsp;").Replace("\n", "&nbsp;").Replace("<", "&lt;").Replace(">", "&gt;").R();
			}
			Scripting::VirtualMachine* Subsystem::ScriptInterface = nullptr;
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
#ifdef VI_USE_RMLUI
				if (Ref != nullptr)
					Base = VI_NEW(Rml::DataModelConstructor, *Ref);
#endif
			}
			DataModel::~DataModel() noexcept
			{
#ifdef VI_USE_RMLUI
				Detach();
				for (auto Item : Props)
					VI_DELETE(DataNode, Item.second);

				VI_DELETE(DataModelConstructor, Base);
#endif
			}
			void DataModel::SetDetachCallback(std::function<void()>&& Callback)
			{
				if (Callback)
					Callbacks.emplace_back(std::move(Callback));
			}
			DataNode* DataModel::SetProperty(const Core::String& Name, const Core::Variant& Value)
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT(IsValid(), nullptr, "data node should be valid");
				DataNode* Result = GetProperty(Name);
				if (Result != nullptr)
				{
					Result->Set(Value);
					return Result;
				}

				Result = VI_NEW(DataNode, this, Name, Value);
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

				VI_DELETE(DataNode, Result);
				return nullptr;
#else
				return nullptr;
#endif
			}
			DataNode* DataModel::SetProperty(const Core::String& Name, Core::Variant* Value)
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT(IsValid(), nullptr, "data node should be valid");
				VI_ASSERT(Value != nullptr, nullptr, "value should be set");

				DataNode* Sub = GetProperty(Name);
				if (Sub != nullptr)
				{
					Sub->Set(Value);
					return Sub;
				}

				DataNode* Result = VI_NEW(DataNode, this, Name, Value);
				if (Base->Bind(Name, Result))
				{
					Props[Name] = Result;
					return Result;
				}

				VI_DELETE(DataNode, Result);
				return nullptr;
#else
				return nullptr;
#endif
			}
			DataNode* DataModel::SetArray(const Core::String& Name)
			{
				return SetProperty(Name, Core::Var::Array());
			}
			DataNode* DataModel::SetString(const Core::String& Name, const Core::String& Value)
			{
				return SetProperty(Name, Core::Var::String(Value));
			}
			DataNode* DataModel::SetInteger(const Core::String& Name, int64_t Value)
			{
				return SetProperty(Name, Core::Var::Integer(Value));
			}
			DataNode* DataModel::SetFloat(const Core::String& Name, float Value)
			{
				return SetProperty(Name, Core::Var::Number(Value));
			}
			DataNode* DataModel::SetDouble(const Core::String& Name, double Value)
			{
				return SetProperty(Name, Core::Var::Number(Value));
			}
			DataNode* DataModel::SetBoolean(const Core::String& Name, bool Value)
			{
				return SetProperty(Name, Core::Var::Boolean(Value));
			}
			DataNode* DataModel::SetPointer(const Core::String& Name, void* Value)
			{
				return SetProperty(Name, Core::Var::Pointer(Value));
			}
			DataNode* DataModel::GetProperty(const Core::String& Name)
			{
				auto It = Props.find(Name);
				if (It != Props.end())
					return It->second;

				return nullptr;
			}
			Core::String DataModel::GetString(const Core::String& Name)
			{
				DataNode* Result = GetProperty(Name);
				if (!Result)
					return "";

				return Result->Ref->GetBlob();
			}
			int64_t DataModel::GetInteger(const Core::String& Name)
			{
				DataNode* Result = GetProperty(Name);
				if (!Result)
					return 0;

				return Result->Ref->GetInteger();
			}
			float DataModel::GetFloat(const Core::String& Name)
			{
				DataNode* Result = GetProperty(Name);
				if (!Result)
					return 0.0f;

				return (float)Result->Ref->GetNumber();
			}
			double DataModel::GetDouble(const Core::String& Name)
			{
				DataNode* Result = GetProperty(Name);
				if (!Result)
					return 0.0;

				return Result->Ref->GetNumber();
			}
			bool DataModel::GetBoolean(const Core::String& Name)
			{
				DataNode* Result = GetProperty(Name);
				if (!Result)
					return false;

				return Result->Ref->GetBoolean();
			}
			void* DataModel::GetPointer(const Core::String& Name)
			{
				DataNode* Result = GetProperty(Name);
				if (!Result)
					return nullptr;

				return Result->Ref->GetPointer();
			}
			bool DataModel::SetCallback(const Core::String& Name, const DataCallback& Callback)
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT(IsValid(), false, "data node should be valid");
				VI_ASSERT(Callback, false, "callback should not be empty");

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
			void DataModel::Change(const Core::String& VariableName)
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT_V(IsValid(), "data node should be valid");
				Base->GetModelHandle().DirtyVariable(VariableName);
#endif
			}
			bool DataModel::HasChanged(const Core::String& VariableName) const
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT(IsValid(), false, "data node should be valid");
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

			DataNode::DataNode(DataModel* Model, const Core::String& TopName, const Core::Variant& Initial) noexcept : Name(TopName), Handle(Model), Order(nullptr), Depth(0), Safe(true)
			{
				Ref = VI_NEW(Core::Variant, Initial);
			}
			DataNode::DataNode(DataModel* Model, const Core::String& TopName, Core::Variant* Reference) noexcept : Name(TopName), Ref(Reference), Handle(Model), Order(nullptr), Depth(0), Safe(false)
			{
			}
			DataNode::DataNode(const DataNode& Other) noexcept : Childs(Other.Childs), Name(Other.Name), Handle(Other.Handle), Order(Other.Order), Depth(0), Safe(Other.Safe)
			{
				if (Safe)
					Ref = VI_NEW(Core::Variant, *Other.Ref);
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
					VI_DELETE(Variant, Ref);
			}
			DataNode& DataNode::Insert(size_t Where, const Core::VariantList& Initial, std::pair<void*, size_t>* Top)
			{
				VI_ASSERT(Where <= Childs.size(), *Childs.begin(), "index outside of range");
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
				VI_ASSERT(Where <= Childs.size(), *Childs.begin(), "index outside of range");
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
				VI_ASSERT(Where <= Childs.size(), *Childs.begin(), "index outside of range");
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
				VI_ASSERT(Index < Childs.size(), *this, "index outside of range");
				return Childs[Index];
			}
			bool DataNode::Remove(size_t Index)
			{
				VI_ASSERT(Index < Childs.size(), false, "index outside of range");
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
				VI_SORT(Childs.begin(), Childs.end(), [](const DataNode& A, const DataNode& B)
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
					VI_DELETE(Variant, Ref);

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
			void DataNode::SetString(const Core::String& Value)
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
			Core::String DataNode::GetString()
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
			void DataNode::SetValueStr(const Core::String& Value)
			{
				*Ref = Core::Var::String(Value);
			}
			void DataNode::SetValueNum(double Value)
			{
				*Ref = Core::Var::Number(Value);
			}
			void DataNode::SetValueInt(int64_t Value)
			{
				*Ref = Core::Var::Integer(Value);
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
					Ref = VI_NEW(Core::Variant, *Other.Ref);
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
#ifdef VI_USE_RMLUI
				Base = VI_NEW(EventSubsystem, NewCallback);
#endif
			}
			Listener::Listener(const Core::String& FunctionName)
			{
#ifdef VI_USE_RMLUI
				Base = VI_NEW(ListenerSubsystem, FunctionName, nullptr);
#endif
			}
			Listener::~Listener() noexcept
			{
#ifdef VI_USE_RMLUI
				Base->OnDetach(nullptr);
#endif
			}

			Context::Context(const Compute::Vector2& Size) : Compiler(nullptr), Cursor(-1.0f), Loading(false)
			{
#ifdef VI_USE_RMLUI
				Base = (ScopedContext*)Rml::CreateContext(Core::ToString(Subsystem::Id++), Rml::Vector2i((int)Size.X, (int)Size.Y));

				VI_ASSERT_V(Base != nullptr, "context cannot be created");
				Base->Basis = this;
				CreateVM();
#endif
			}
			Context::Context(Graphics::GraphicsDevice* Device) : Compiler(nullptr), Cursor(-1.0f), Loading(false)
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT_V(Device != nullptr, "graphics device should be set");
				VI_ASSERT_V(Device->GetRenderTarget() != nullptr, "graphics device should be set");

				Graphics::RenderTarget2D* Target = Device->GetRenderTarget();
				Base = (ScopedContext*)Rml::CreateContext(Core::ToString(Subsystem::Id++), Rml::Vector2i((int)Target->GetWidth(), (int)Target->GetHeight()));

				VI_ASSERT_V(Base != nullptr, "context cannot be created");
				Base->Basis = this;
				CreateVM();
#endif
			}
			Context::~Context() noexcept
			{
#ifdef VI_USE_RMLUI
				RemoveDataModels();
				Rml::RemoveContext(Base->GetName());
				VI_RELEASE(Compiler);
#endif
			}
			void Context::EmitKey(Graphics::KeyCode Key, Graphics::KeyMod Mod, int Virtual, int Repeat, bool Pressed)
			{
#ifdef VI_USE_RMLUI
				if (Key == Graphics::KeyCode::CursorLeft)
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
				else if (Key == Graphics::KeyCode::CursorRight)
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
				else if (Key == Graphics::KeyCode::CursorMiddle)
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
#ifdef VI_USE_RMLUI
				VI_ASSERT_V(Buffer != nullptr && Length > 0, "buffer should be set");
				if (Base->ProcessTextInput(Core::String(Buffer, Length)))
					Inputs.Text = true;
#endif
			}
			void Context::EmitWheel(int X, int Y, bool Normal, Graphics::KeyMod Mod)
			{
#ifdef VI_USE_RMLUI
				if (Base->ProcessMouseWheel((float)-Y, GetKeyMod(Mod)))
					Inputs.Scroll = true;
#endif
			}
			void Context::EmitResize(int Width, int Height)
			{
#ifdef VI_USE_RMLUI
				RenderSubsystem* Renderer = Subsystem::GetRenderInterface();
				if (Renderer != nullptr)
					Renderer->ResizeBuffers(Width, Height);

				Base->SetDimensions(Rml::Vector2i(Width, Height));
#endif
			}
			void Context::UpdateEvents(Graphics::Activity* Activity)
			{
#ifdef VI_USE_RMLUI
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
#ifdef VI_USE_RMLUI
				VI_ASSERT_V(Subsystem::GetRenderInterface() != nullptr, "render interface should be valid");
				RenderSubsystem* Renderer = Subsystem::GetRenderInterface();
				Renderer->Background = Target;
				Base->Render();
#endif
			}
			void Context::ClearStyles()
			{
#ifdef VI_USE_RMLUI
				Rml::StyleSheetFactory::ClearStyleSheetCache();
#endif
			}
			bool Context::ClearDocuments()
			{
#ifdef VI_USE_RMLUI
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
			bool Context::Initialize(Core::Schema* Conf, const Core::String& Relative)
			{
				VI_ASSERT(Conf != nullptr, false, "conf should be set");
				bool State = Loading;
				Loading = true;

				for (auto* Face : Conf->FindCollection("font-face", true))
				{
					Core::String Path = Face->GetAttributeVar("path").GetBlob();
					if (Path.empty())
					{
						VI_ERR("[gui] path is required for font face");
						return false;
					}

					Core::String Target = Core::OS::Path::Resolve(Path, Relative, false);
					if (!LoadFontFace(Target.empty() ? Path : Target, Face->GetAttribute("fallback") != nullptr))
					{
						Loading = State;
						return false;
					}
				}

				for (auto* Document : Conf->FindCollection("document", true))
				{
					Core::String Path = Document->GetAttributeVar("path").GetBlob();
					if (Path.empty())
					{
						VI_ERR("[gui] path is required for document");
						return false;
					}

					Core::String Target = Core::OS::Path::Resolve(Path, Relative, false);
					IElementDocument Result = LoadDocument(Target.empty() ? Path : Target);
					if (!Result.IsValid())
					{
						Loading = State;
						return false;
					}
					else if (Document->HasAttribute("show"))
						Result.Show();
				}

				Loading = State;
				return true;
			}
			bool Context::Initialize(const Core::String& ConfPath)
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT(Subsystem::RenderInterface != nullptr, false, "render interface should be set");
				VI_ASSERT(Subsystem::RenderInterface->GetContent() != nullptr, false, "content manager should be set");

				bool State = Loading;
				Loading = true;

				ContentManager* Content = Subsystem::RenderInterface->GetContent();
				Core::Schema* Sheet = Content->Load<Core::Schema>(ConfPath);
				if (!Sheet)
				{
					Loading = State;
					return false;
				}

				Core::String TargetPath = Core::OS::Path::ResolveDirectory(Core::OS::Path::GetDirectory(ConfPath.c_str()), Content->GetEnvironment(), true);
				bool Result = Initialize(Sheet, TargetPath);
				VI_RELEASE(Sheet);

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
#ifdef VI_USE_RMLUI
				Rml::Element* Element = Base->GetFocusElement();
				if (!Element)
					return false;

				const Rml::String& Tag = Element->GetTagName();
				return Tag == "input" || Tag == "textarea" || Tag == "select";
#else
				return false;
#endif
			}
			bool Context::LoadFontFace(const Core::String& Path, bool UseAsFallback)
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT(Subsystem::GetSystemInterface() != nullptr, false, "system interface should be set");
				bool State = Loading;
				Loading = true;

				bool Result = Subsystem::GetSystemInterface()->AddFontFace(Path, UseAsFallback);
				Loading = State;

				return Result;
#else
				return false;
#endif
			}
			Core::UnorderedMap<Core::String, bool>* Context::GetFontFaces()
			{
#ifdef VI_USE_RMLUI
				return Subsystem::GetSystemInterface()->GetFontFaces();
#else
				return nullptr;
#endif
			}
			Rml::Context* Context::GetContext()
			{
#ifdef VI_USE_RMLUI
				return Base;
#else
				return nullptr;
#endif
			}
			Compute::Vector2 Context::GetDimensions() const
			{
#ifdef VI_USE_RMLUI
				Rml::Vector2i Size = Base->GetDimensions();
				return Compute::Vector2((float)Size.x, (float)Size.y);
#else
				return Compute::Vector2();
#endif
			}
			void Context::SetDensityIndependentPixelRatio(float DensityIndependentPixelRatio)
			{
#ifdef VI_USE_RMLUI
				Base->SetDensityIndependentPixelRatio(DensityIndependentPixelRatio);
#endif
			}
			void Context::EnableMouseCursor(bool Enable)
			{
#ifdef VI_USE_RMLUI
				Base->EnableMouseCursor(Enable);
#endif
			}
			float Context::GetDensityIndependentPixelRatio() const
			{
#ifdef VI_USE_RMLUI
				return Base->GetDensityIndependentPixelRatio();
#else
				return 0.0f;
#endif
			}
			bool Context::ReplaceHTML(const Core::String& Selector, const Core::String& HTML, int Index)
			{
#ifdef VI_USE_RMLUI
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
			IElementDocument Context::EvalHTML(const Core::String& HTML, int Index)
			{
#ifdef VI_USE_RMLUI
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
			IElementDocument Context::AddCSS(const Core::String& CSS, int Index)
			{
#ifdef VI_USE_RMLUI
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
			IElementDocument Context::LoadCSS(const Core::String& Path, int Index)
			{
#ifdef VI_USE_RMLUI
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
			IElementDocument Context::LoadDocument(const Core::String& Path)
			{
#ifdef VI_USE_RMLUI
				bool State = Loading;
				Loading = true;

				if (OnMount)
					OnMount(this);

				ClearVM();
				Elements.clear();

				size_t Length = 0;
				unsigned char* Buffer = Core::OS::File::ReadAll(Path, &Length);
				if (!Buffer)
				{
				ErrorState:
					Base->UnloadAllDocuments();
					Loading = State;

					return nullptr;
				}

				Core::String Data((const char*)Buffer, Length);
				VI_FREE(Buffer);

				Decompose(Data);
				if (!Preprocess(Path, Data))
					goto ErrorState;

				Core::Stringify URL(Path);
				URL.Replace('\\', '/');
				URL.Insert("file:///", 0);

				auto* Result = Base->LoadDocumentFromMemory(Data, URL.R());
				Loading = State;

				return Result;
#else
				return IElementDocument();
#endif
			}
			IElementDocument Context::AddDocumentEmpty(const Core::String& InstancerName)
			{
#ifdef VI_USE_RMLUI
				return Base->CreateDocument(InstancerName);
#else
				return IElementDocument();
#endif
			}
			IElementDocument Context::AddDocument(const Core::String& HTML)
			{
#ifdef VI_USE_RMLUI
				return Base->LoadDocumentFromMemory(HTML);
#else
				return IElementDocument();
#endif
			}
			IElementDocument Context::GetDocument(const Core::String& Id)
			{
#ifdef VI_USE_RMLUI
				return Base->GetDocument(Id);
#else
				return IElementDocument();
#endif
			}
			IElementDocument Context::GetDocument(int Index)
			{
#ifdef VI_USE_RMLUI
				return Base->GetDocument(Index);
#else
				return IElementDocument();
#endif
			}
			int Context::GetNumDocuments() const
			{
#ifdef VI_USE_RMLUI
				return Base->GetNumDocuments();
#else
				return 0;
#endif
			}
			IElement Context::GetElementById(const Core::String& Id, int DocIndex)
			{
#ifdef VI_USE_RMLUI
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
#ifdef VI_USE_RMLUI
				return Base->GetHoverElement();
#else
				return IElement();
#endif
			}
			IElement Context::GetFocusElement()
			{
#ifdef VI_USE_RMLUI
				return Base->GetFocusElement();
#else
				return IElement();
#endif
			}
			IElement Context::GetRootElement()
			{
#ifdef VI_USE_RMLUI
				return Base->GetRootElement();
#else
				return IElement();
#endif
			}
			IElement Context::GetElementAtPoint(const Compute::Vector2& Point, const IElement& IgnoreElement, const IElement& Element) const
			{
#ifdef VI_USE_RMLUI
				return Base->GetElementAtPoint(Rml::Vector2f(Point.X, Point.Y), IgnoreElement.GetElement(), Element.GetElement());
#else
				return IElement();
#endif
			}
			void Context::PullDocumentToFront(const IElementDocument& Schema)
			{
#ifdef VI_USE_RMLUI
				Base->PullDocumentToFront(Schema.GetElementDocument());
#endif
			}
			void Context::PushDocumentToBack(const IElementDocument& Schema)
			{
#ifdef VI_USE_RMLUI
				Base->PushDocumentToBack(Schema.GetElementDocument());
#endif
			}
			void Context::UnfocusDocument(const IElementDocument& Schema)
			{
#ifdef VI_USE_RMLUI
				Base->UnfocusDocument(Schema.GetElementDocument());
#endif
			}
			void Context::AddEventListener(const Core::String& Event, Listener* Listener, bool InCapturePhase)
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT_V(Listener != nullptr && Listener->Base != nullptr, "listener should be valid");
				Base->AddEventListener(Event, Listener->Base, InCapturePhase);
#endif
			}
			void Context::RemoveEventListener(const Core::String& Event, Listener* Listener, bool InCapturePhase)
			{
#ifdef VI_USE_RMLUI
				VI_ASSERT_V(Listener != nullptr && Listener->Base != nullptr, "listener should be valid");
				Base->RemoveEventListener(Event, Listener->Base, InCapturePhase);
#endif
			}
			bool Context::IsMouseInteracting() const
			{
#ifdef VI_USE_RMLUI
				return Base->IsMouseInteracting();
#else
				return false;
#endif
			}
			bool Context::WasInputUsed(uint32_t InputTypeMask)
			{
#ifdef VI_USE_RMLUI
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
#ifdef VI_USE_RMLUI
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
#ifdef VI_USE_RMLUI
				return Base->SetActiveClipRegion(Rml::Vector2i((int)Origin.X, (int)Origin.Y), Rml::Vector2i((int)Dimensions.X, (int)Dimensions.Y));
#endif
			}
			DataModel* Context::SetDataModel(const Core::String& Name)
			{
#ifdef VI_USE_RMLUI
				Rml::DataModelConstructor Result = Base->CreateDataModel(Name);
				if (!Result)
					return nullptr;

				DataModel* Handle = new DataModel(&Result);
				if (auto Type = Result.RegisterStruct<DataNode>())
				{
					Result.RegisterArray<Core::Vector<DataNode>>();
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
			DataModel* Context::GetDataModel(const Core::String& Name)
			{
				auto It = Models.find(Name);
				if (It != Models.end())
					return It->second;

				return nullptr;
			}
			bool Context::RemoveDataModel(const Core::String& Name)
			{
#ifdef VI_USE_RMLUI
				if (!Base->RemoveDataModel(Name))
					return false;

				auto It = Models.find(Name);
				if (It != Models.end())
				{
					if (It->second->OnUnmount)
						It->second->OnUnmount(this);

					VI_RELEASE(It->second);
					Models.erase(It);
				}

				return true;
#else
				return false;
#endif
			}
			bool Context::RemoveDataModels()
			{
#ifdef VI_USE_RMLUI
				if (Models.empty())
					return false;

				for (auto Item : Models)
				{
					Base->RemoveDataModel(Item.first);
					VI_RELEASE(Item.second);
				}

				Models.clear();
				return true;
#else
				return false;
#endif
			}
			void Context::SetDocumentsBaseTag(const Core::String& Tag)
			{
#ifdef VI_USE_RMLUI
				Base->SetDocumentsBaseTag(Tag);
#endif
			}
			void Context::SetMountCallback(const ModelCallback& Callback)
			{
				OnMount = Callback;
			}
			bool Context::Preprocess(const Core::String& Path, Core::String& Buffer)
			{
				Compute::Preprocessor::Desc Features;
				Features.Conditions = false;
				Features.Defines = false;
				Features.Pragmas = false;

				Compute::IncludeDesc Desc = Compute::IncludeDesc();
				Desc.Exts.push_back(".html");
				Desc.Exts.push_back(".htm");
				Desc.Root = Core::OS::Directory::GetWorking();

				Compute::Preprocessor* Processor = new Compute::Preprocessor();
				Processor->SetIncludeCallback([this](Compute::Preprocessor* P, const Compute::IncludeResult& File, Core::String& Output)
				{
					if (File.Module.empty() || (!File.IsFile && !File.IsAbstract))
						return Compute::IncludeType::Error;

					if (File.IsAbstract && !File.IsFile)
						return Compute::IncludeType::Error;

					Output.assign(Core::OS::File::ReadAsString(File.Module));
					if (Output.empty())
						return Compute::IncludeType::Error;

					this->Decompose(Output);
					return Compute::IncludeType::Preprocess;
				});
				Processor->SetIncludeOptions(Desc);
				Processor->SetFeatures(Features);

				bool Result = Processor->Process(Path, Buffer);
				VI_RELEASE(Processor);

				return Result;
			}
			void Context::Decompose(Core::String& Data)
			{
				Core::Stringify::Settle Result, Start, End;
				Core::Stringify Buffer(&Data);

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
			Core::String Context::GetDocumentsBaseTag()
			{
#ifdef VI_USE_RMLUI
				return Base->GetDocumentsBaseTag();
#else
				return Core::String();
#endif
			}
			int Context::GetKeyCode(Graphics::KeyCode Key)
			{
#ifdef VI_USE_RMLUI
				using namespace Rml::Input;
				switch (Key)
				{
					case Graphics::KeyCode::Space:
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
					case Graphics::KeyCode::Semicolon:
						return KI_OEM_1;
					case Graphics::KeyCode::Comma:
						return KI_OEM_COMMA;
					case Graphics::KeyCode::Minus:
						return KI_OEM_MINUS;
					case Graphics::KeyCode::Period:
						return KI_OEM_PERIOD;
					case Graphics::KeyCode::Slash:
						return KI_OEM_2;
					case Graphics::KeyCode::LeftBracket:
						return KI_OEM_4;
					case Graphics::KeyCode::Backslash:
						return KI_OEM_5;
					case Graphics::KeyCode::RightBracket:
						return KI_OEM_6;
					case Graphics::KeyCode::Kp0:
						return KI_NUMPAD0;
					case Graphics::KeyCode::Kp1:
						return KI_NUMPAD1;
					case Graphics::KeyCode::Kp2:
						return KI_NUMPAD2;
					case Graphics::KeyCode::Kp3:
						return KI_NUMPAD3;
					case Graphics::KeyCode::Kp4:
						return KI_NUMPAD4;
					case Graphics::KeyCode::Kp5:
						return KI_NUMPAD5;
					case Graphics::KeyCode::Kp6:
						return KI_NUMPAD6;
					case Graphics::KeyCode::Kp7:
						return KI_NUMPAD7;
					case Graphics::KeyCode::Kp8:
						return KI_NUMPAD8;
					case Graphics::KeyCode::Kp9:
						return KI_NUMPAD9;
					case Graphics::KeyCode::KpEnter:
						return KI_NUMPADENTER;
					case Graphics::KeyCode::KpMultiply:
						return KI_MULTIPLY;
					case Graphics::KeyCode::KpPlus:
						return KI_ADD;
					case Graphics::KeyCode::KpMinus:
						return KI_SUBTRACT;
					case Graphics::KeyCode::KpPeriod:
						return KI_DECIMAL;
					case Graphics::KeyCode::KpDivide:
						return KI_DIVIDE;
					case Graphics::KeyCode::KpEquals:
						return KI_OEM_NEC_EQUAL;
					case Graphics::KeyCode::Backspace:
						return KI_BACK;
					case Graphics::KeyCode::Tab:
						return KI_TAB;
					case Graphics::KeyCode::Clear:
						return KI_CLEAR;
					case Graphics::KeyCode::Return:
						return KI_RETURN;
					case Graphics::KeyCode::Pause:
						return KI_PAUSE;
					case Graphics::KeyCode::Capslock:
						return KI_CAPITAL;
					case Graphics::KeyCode::PageUp:
						return KI_PRIOR;
					case Graphics::KeyCode::PageDown:
						return KI_NEXT;
					case Graphics::KeyCode::End:
						return KI_END;
					case Graphics::KeyCode::Home:
						return KI_HOME;
					case Graphics::KeyCode::Left:
						return KI_LEFT;
					case Graphics::KeyCode::Up:
						return KI_UP;
					case Graphics::KeyCode::Right:
						return KI_RIGHT;
					case Graphics::KeyCode::Down:
						return KI_DOWN;
					case Graphics::KeyCode::Insert:
						return KI_INSERT;
					case Graphics::KeyCode::Delete:
						return KI_DELETE;
					case Graphics::KeyCode::Help:
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
					case Graphics::KeyCode::NumLockClear:
						return KI_NUMLOCK;
					case Graphics::KeyCode::ScrollLock:
						return KI_SCROLL;
					case Graphics::KeyCode::LeftShift:
						return KI_LSHIFT;
					case Graphics::KeyCode::RightShift:
						return KI_RSHIFT;
					case Graphics::KeyCode::LeftControl:
						return KI_LCONTROL;
					case Graphics::KeyCode::RightControl:
						return KI_RCONTROL;
					case Graphics::KeyCode::LeftAlt:
						return KI_LMENU;
					case Graphics::KeyCode::RightAlt:
						return KI_RMENU;
					case Graphics::KeyCode::LeftGUI:
						return KI_LMETA;
					case Graphics::KeyCode::RightGUI:
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
#ifdef VI_USE_RMLUI
				int Result = 0;
				if ((size_t)Mod & (size_t)Graphics::KeyMod::Control)
					Result |= Rml::Input::KM_CTRL;

				if ((size_t)Mod & (size_t)Graphics::KeyMod::Shift)
					Result |= Rml::Input::KM_SHIFT;

				if ((size_t)Mod & (size_t)Graphics::KeyMod::Alt)
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