#include "gui.h"
#include "../network.h"
#ifdef VI_RMLUI
#include <RmlUi/Core.h>
#include <RmlUi/Core/Stream.h>
#include <Source/Core/StyleSheetFactory.h>
#include <Source/Core/ElementStyle.h>
#endif
#pragma warning(push)
#pragma warning(disable: 4996)

namespace Vitex
{
	namespace Engine
	{
		namespace GUI
		{
#ifdef VI_RMLUI
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
					Core::Memory::Release(VertexBuffer);
					Core::Memory::Release(IndexBuffer);
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
					Detach();
				}
				void RenderGeometry(Rml::Vertex* Vertices, int VerticesSize, int* Indices, int IndicesSize, Rml::TextureHandle Texture, const Rml::Vector2f& Translation) override
				{
					VI_ASSERT(Device != nullptr, "graphics device should be set");
					VI_ASSERT(Vertices != nullptr, "vertices should be set");
					VI_ASSERT(Indices != nullptr, "indices should be set");

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
					VI_ASSERT(Device != nullptr, "graphics device should be set");
					GeometryBuffer* Result = Core::Memory::New<GeometryBuffer>();
					Result->Texture = (Graphics::Texture2D*)Handle;

					Graphics::ElementBuffer::Desc F = Graphics::ElementBuffer::Desc();
					F.AccessFlags = Graphics::CPUAccess::None;
					F.Usage = Graphics::ResourceUsage::Default;
					F.BindFlags = Graphics::ResourceBind::Vertex_Buffer;
					F.ElementCount = (unsigned int)VerticesCount;
					F.Elements = (void*)Vertices;
					F.ElementWidth = sizeof(Rml::Vertex);
					Result->VertexBuffer = *Device->CreateElementBuffer(F);

					F = Graphics::ElementBuffer::Desc();
					F.AccessFlags = Graphics::CPUAccess::None;
					F.Usage = Graphics::ResourceUsage::Default;
					F.BindFlags = Graphics::ResourceBind::Index_Buffer;
					F.ElementCount = (unsigned int)IndicesCount;
					F.ElementWidth = sizeof(unsigned int);
					F.Elements = (void*)Indices;
					Result->IndexBuffer = *Device->CreateElementBuffer(F);
					
					return (Rml::CompiledGeometryHandle)Result;
				}
				void RenderCompiledGeometry(Rml::CompiledGeometryHandle Handle, const Rml::Vector2f& Translation) override
				{
					GeometryBuffer* Buffer = (GeometryBuffer*)Handle;
					VI_ASSERT(Device != nullptr, "graphics device should be set");
					VI_ASSERT(Buffer != nullptr, "buffer should be set");

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
					Core::Memory::Delete(Resource);
				}
				void EnableScissorRegion(bool Enable) override
				{
					VI_ASSERT(Device != nullptr, "graphics device should be set");
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
					VI_ASSERT(Device != nullptr, "graphics device should be set");
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
					VI_ASSERT(Content != nullptr, "content manager should be set");
					auto Result = Content->Load<Graphics::Texture2D>(Source);
					if (!Result)
						return false;

					TextureDimensions.x = Result->GetWidth();
					TextureDimensions.y = Result->GetHeight();
					Handle = (Rml::TextureHandle)*Result;
					return true;
				}
				bool GenerateTexture(Rml::TextureHandle& Handle, const Rml::byte* Source, const Rml::Vector2i& SourceDimensions) override
				{
					VI_ASSERT(Device != nullptr, "graphics device should be set");
					VI_ASSERT(Source != nullptr, "source should be set");

					Graphics::Texture2D::Desc F = Graphics::Texture2D::Desc();
					F.Data = (void*)Source;
					F.Width = (unsigned int)SourceDimensions.x;
					F.Height = (unsigned int)SourceDimensions.y;
					F.RowPitch = Device->GetRowPitch(F.Width);
					F.DepthPitch = Device->GetDepthPitch(F.RowPitch, F.Height);
					F.MipLevels = 1;

					Graphics::Texture2D* Result = *Device->CreateTexture2D(F);
					Handle = (Rml::TextureHandle)Result;
					return true;
				}
				void ReleaseTexture(Rml::TextureHandle Handle) override
				{
					Graphics::Texture2D* Resource = (Graphics::Texture2D*)Handle;
					Core::Memory::Release(Resource);
				}
				void SetTransform(const Rml::Matrix4f* NewTransform) override
				{
					HasTransform = (NewTransform != nullptr);
					if (HasTransform)
						Transform = Utils::ToMatrix(NewTransform);
				}
				void Attach(RenderConstants* NewConstants, ContentManager* NewContent)
				{
					VI_ASSERT(NewConstants != nullptr, "render constants should be set");
					VI_ASSERT(NewContent != nullptr, "content manager should be set");
					VI_ASSERT(NewContent->GetDevice() != nullptr, "graphics device should be set");

					Detach();
					Constants = NewConstants;
					Content = NewContent;
					Device = Content->GetDevice();
					Constants->AddRef();
					Content->AddRef();
					Device->AddRef();

					Graphics::Shader::Desc I = Graphics::Shader::Desc();
					if (Device->GetSectionData("materials/material_ui_element", &I))
						Shader = *Device->CreateShader(I);

					Graphics::ElementBuffer::Desc F = Graphics::ElementBuffer::Desc();
					F.AccessFlags = Graphics::CPUAccess::Write;
					F.Usage = Graphics::ResourceUsage::Dynamic;
					F.BindFlags = Graphics::ResourceBind::Vertex_Buffer;
					F.ElementCount = (unsigned int)6;
					F.Elements = (void*)nullptr;
					F.ElementWidth = sizeof(Rml::Vertex);

					VertexBuffer = *Device->CreateElementBuffer(F);
					Layout = Device->GetInputLayout("vx_ui");
					ScissorNoneRasterizer = Device->GetRasterizerState("sw_co");
					NoneRasterizer = Device->GetRasterizerState("so_co");
					NoneDepthStencil = Device->GetDepthStencilState("doo_soo_lt");
					LessDepthStencil = Device->GetDepthStencilState("drw_srw_lt");
					ScissorDepthStencil = Device->GetDepthStencilState("dro_srw_gte");
					AlphaBlend = Device->GetBlendState("bw_wrgba_source");
					ColorlessBlend = Device->GetBlendState("bo_woooo_one");
					Sampler = Device->GetSamplerState("a16_fa_wrap");
					Subsystem::Get()->CreateDecorators(Constants);
				}
				void Detach()
				{
					Subsystem::Get()->ReleaseDecorators();
					Core::Memory::Release(VertexBuffer);
					Core::Memory::Release(Shader);
					Core::Memory::Release(Constants);
					Core::Memory::Release(Content);
					Core::Memory::Release(Device);
				}
				void ResizeBuffers(int Width, int Height)
				{
					Subsystem::Get()->ResizeDecorators(Width, Height);
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
					Network::Location Origin(Target);
					if (Origin.Protocol == "file" && !Core::OS::File::IsExists(Target.c_str()))
					{
						ContentManager* Content = (Subsystem::Get()->GetRenderInterface() ? Subsystem::Get()->GetRenderInterface()->GetContent() : nullptr);
						auto Subpath = (Content ? Core::OS::Path::Resolve(Path, Content->GetEnvironment(), false) : Core::OS::Path::Resolve(Path.c_str()));
						Target = (Subpath ? *Subpath : Path);
					}

					auto Stream = Core::OS::File::Open(Target, Core::FileMode::Binary_Read_Only);
					if (!Stream)
						return (Rml::FileHandle)nullptr;

					return (Rml::FileHandle)*Stream;
				}
				void Close(Rml::FileHandle File) override
				{
					Core::Stream* Stream = (Core::Stream*)File;
					Core::Memory::Release(Stream);
				}
				size_t Read(void* Buffer, size_t Size, Rml::FileHandle File) override
				{
					Core::Stream* Stream = (Core::Stream*)File;
					VI_ASSERT(Stream != nullptr, "stream should be set");
					return Stream->Read((uint8_t*)Buffer, Size).Or(0);
				}
				bool Seek(Rml::FileHandle File, long Offset, int Origin) override
				{
					Core::Stream* Stream = (Core::Stream*)File;
					VI_ASSERT(Stream != nullptr, "stream should be set");
					return !!Stream->Seek((Core::FileSeek)Origin, Offset);
				}
				size_t Tell(Rml::FileHandle File) override
				{
					Core::Stream* Stream = (Core::Stream*)File;
					VI_ASSERT(Stream != nullptr, "stream should be set");
					return Stream->Tell().Or(0);
				}
			};

			class MainSubsystem final : public Rml::SystemInterface
			{
			private:
				Core::UnorderedMap<Core::String, TranslationCallback> Translators;
				Core::UnorderedMap<Core::String, Core::Vector<FontInfo>> Fonts;
				Graphics::Activity* Activity;
				Core::Timer* Time;

			public:
				MainSubsystem() : Rml::SystemInterface(), Activity(nullptr), Time(nullptr)
				{
				}
				~MainSubsystem()
				{
					Detach();
				}
				void SetMouseCursor(const Rml::String& CursorName) override
				{
					if (!Activity)
						return;

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
					if (Activity != nullptr)
						Activity->SetClipboardText(Text);
				}
				void GetClipboardText(Rml::String& Text) override
				{
					if (Activity != nullptr)
						Text = Activity->GetClipboardText();
				}
				void ActivateKeyboard(Rml::Vector2f CaretPosition, float LineHeight) override
				{
					if (Activity != nullptr)
						Activity->SetScreenKeyboard(true);
				}
				void DeactivateKeyboard() override
				{
					if (Activity != nullptr)
						Activity->SetScreenKeyboard(false);
				}
				void JoinPath(Rml::String& Result, const Rml::String& Path1, const Rml::String& Path2) override
				{
					ContentManager* Content = (Subsystem::Get()->GetRenderInterface() ? Subsystem::Get()->GetRenderInterface()->GetContent() : nullptr);
					if (!Content)
						return;

					Core::String Proto1, Proto2;
					Core::String Fixed1 = GetFixedPath(Path1, Proto1);
					Core::String Fixed2 = GetFixedPath(Path2, Proto2);
					if (Proto1 != "file" && Proto2 == "file")
					{
						Result.assign(Path1);
						if (!Core::Stringify::EndsWith(Result, '/'))
							Result.append(1, '/');

						Result.append(Fixed2);
						Core::Stringify::Replace(Result, "/////", "//");
						Core::TextSettle Idx = Core::Stringify::Find(Result, "://");
						Core::Stringify::Replace(Result, "//", "/", Idx.Found ? Idx.End : 0);
					}
					else if (Proto1 == "file" && Proto2 == "file")
					{
						auto Path = Core::OS::Path::Resolve(Fixed2, Core::OS::Path::GetDirectory(Fixed1.c_str()), true);
						if (!Path)
							Path = Core::OS::Path::Resolve(Fixed2, Content->GetEnvironment(), true);
						if (Path)
							Result = *Path;
					}
					else if (Proto1 == "file" && Proto2 != "file")
					{
						Result.assign(Path2);
						Core::Stringify::Replace(Result, "/////", "//");
					}
				}
				bool LogMessage(Rml::Log::Type Type, const Rml::String& Message) override
				{
					switch (Type)
					{
						case Rml::Log::LT_ERROR:
							VI_ERR("[gui] %.*s", (int)Message.size(), Message.c_str());
							break;
						case Rml::Log::LT_WARNING:
							VI_WARN("[gui] %.*s", (int)Message.size(), Message.c_str());
							break;
						case Rml::Log::LT_INFO:
							VI_DEBUG("[gui] %.*s", (int)Message.size(), Message.c_str());
							break;
						case Rml::Log::LT_ASSERT:
							VI_TRACE("[gui] %.*s", (int)Message.size(), Message.c_str());
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
					Detach();
					Activity = NewActivity;
					Time = NewTime;
					if (Activity != nullptr)
						Activity->AddRef();
					if (Time != nullptr)
						Time->AddRef();
				}
				void Detach()
				{
					Core::Memory::Release(Activity);
					Core::Memory::Release(Time);
				}
				void SetTranslator(const std::string_view& Name, const TranslationCallback& Callback)
				{
					auto It = Translators.find(Core::HglCast(Name));
					if (It == Translators.end())
						Translators.insert(std::make_pair(Core::String(Name), Callback));
					else
						It->second = Callback;
				}
				bool AddFontFace(const std::string_view& Path, bool UseAsFallback, FontWeight Weight)
				{
					auto It = Fonts.find(Core::HglCast(Path));
					if (It != Fonts.end())
					{
						for (auto& Info : It->second)
						{
							if (Info.Fallback == UseAsFallback && Info.Weight == Weight)
								return true;
						}
					}

					Core::String TargetPath = Core::String(Path);
					if (!Rml::LoadFontFace(TargetPath, UseAsFallback, (Rml::Style::FontWeight)Weight))
						return false;

					FontInfo Info;
					Info.Fallback = UseAsFallback;
					Info.Weight = Weight;
					if (It != Fonts.end())
						It->second.push_back(Info);
					else
						Fonts[TargetPath].push_back(Info);
					return true;
				}
				Core::UnorderedMap<Core::String, Core::Vector<FontInfo>>* GetFontFaces()
				{
					return &Fonts;
				}
				Core::String GetFixedPath(const std::string_view& Location, Core::String& Proto)
				{
					if (!Core::Stringify::Find(Location, "://").Found)
					{
						Proto = "file";
						return Core::String(Location);
					}

					Rml::URL Target = Rml::URL(Core::String(Location));
					Proto = Target.GetProtocol();
					return Target.GetPathedFileName();
				}
			};

			class ScopedContext final : public Rml::Context
			{
			public:
				GUI::Context* Basis;

			public:
				ScopedContext(const std::string_view& Name) : Rml::Context(Rml::String(Name)), Basis(nullptr)
				{
				}
			};

			class ContextInstancer final : public Rml::ContextInstancer
			{
			public:
				Rml::ContextPtr InstanceContext(const Rml::String& Name) override
				{
					return Rml::ContextPtr(Core::Memory::New<ScopedContext>(Name));
				}
				void ReleaseContext(Rml::Context* Context) override
				{
					ScopedContext* Item = (ScopedContext*)Context;
					Core::Memory::Delete(Item);
				}
				void Release() override
				{
					Core::Memory::Delete(this);
				}
			};

			class DocumentSubsystem final : public Rml::ElementDocument
			{
			public:
				DocumentSubsystem(const std::string_view& Tag) : Rml::ElementDocument(Rml::String(Tag))
				{
				}
				void LoadInlineScript(const Rml::String& Content, const Rml::String& Path, int Line) override
				{
					ScopedContext* Scope = (ScopedContext*)GetContext();
					VI_ASSERT(Scope && Scope->Basis && Scope->Basis->Compiler, "context should be scoped");

					Scripting::Compiler* Compiler = Scope->Basis->Compiler;
					if (!Compiler->LoadCode(Path + ":" + Core::ToString(Line), Content))
						return;

					Compiler->Compile().When([Scope, Compiler](Scripting::ExpectsVM<void>&& Status)
					{
						if (!Status)
							return;

						Scripting::Function Main = Compiler->GetModule().GetFunctionByDecl("void main()");
						if (!Main.IsValid())
							Main = Compiler->GetModule().GetFunctionByDecl("void main(ui_context@+)");

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
				void LoadExternalScript(const Rml::String& Path) override
				{
					ScopedContext* Scope = (ScopedContext*)GetContext();
					VI_ASSERT(Scope && Scope->Basis && Scope->Basis->Compiler, "context should be scoped");

					Core::String Where = Path;
					Core::Stringify::Replace(Where, '|', ':');

					Scripting::Compiler* Compiler = Scope->Basis->Compiler;
					if (!Compiler->LoadFile(Where))
						return;

					Compiler->Compile().When([Scope, Compiler](Scripting::ExpectsVM<void>&& Status)
					{
						if (!Status)
							return;

						Scripting::Function Main = Compiler->GetModule().GetFunctionByDecl("void main()");
						if (!Main.IsValid())
							Main = Compiler->GetModule().GetFunctionByDecl("void main(ui_context@+)");

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
					return Rml::ElementPtr(Core::Memory::New<DocumentSubsystem>(Tag));
				}
				void ReleaseElement(Rml::Element* Element) override
				{
					Core::Memory::Delete(Element);
				}
			};

			class ListenerSubsystem final : public Rml::EventListener
			{
			public:
				Scripting::FunctionDelegate Delegate;
				Core::String Memory;
				IEvent EventContext;

			public:
				ListenerSubsystem(const std::string_view& Code, Rml::Element* Element) : Memory(Code)
				{
				}
				~ListenerSubsystem() = default;
				void OnDetach(Rml::Element* Element) override
				{
					Core::Memory::Delete(this);
				}
				void ProcessEvent(Rml::Event& Event) override
				{
					ScopedContext* Scope = (ScopedContext*)Event.GetCurrentElement()->GetContext();
					VI_ASSERT(Scope && Scope->Basis && Scope->Basis->Compiler, "context should be scoped");
					if (!CompileInline(Scope))
						return;

					Rml::Event* Ptr = Rml::Factory::InstanceEvent(Event.GetTargetElement(), Event.GetId(), Event.GetType(), Event.GetParameters(), Event.IsInterruptible()).release();
					if (Ptr != nullptr)
					{
						Ptr->SetCurrentElement(Event.GetCurrentElement());
						Ptr->SetPhase(Event.GetPhase());
					}

					EventContext = IEvent(Ptr);
					Scope->Basis->AddRef();
					Delegate([this](Scripting::ImmediateContext* Context)
					{
						Context->SetArgObject(0, &EventContext);
					}, [this, Scope, Ptr](Scripting::ImmediateContext*)
					{
						Scope->Basis->Release();
						EventContext = IEvent();
						delete Ptr;
					});
				}
				bool CompileInline(ScopedContext* Scope)
				{
					if (Delegate.IsValid())
						return true;

					auto Hash = Compute::Crypto::HashHex(Compute::Digests::MD5(), Memory);
					if (!Hash)
						return false;

					Core::String Name = "__vf" + *Hash;
					Core::String Eval = "void " + Name + "(ui_event&in event){\n";
					Eval.append(Memory);
					Eval += "\n;}";

					Scripting::Function Function = nullptr;
					Scripting::Module Module = Scope->Basis->Compiler->GetModule();
					if (!Module.CompileFunction(Name.c_str(), Eval.c_str(), -1, (size_t)Scripting::CompileFlags::ADD_TO_MODULE, &Function))
						return false;

					Delegate = Function;
					return Delegate.IsValid();
				}
			};

			class ListenerInstancer final : public Rml::EventListenerInstancer
			{
			public:
				Rml::EventListener* InstanceEventListener(const Rml::String& Value, Rml::Element* Element) override
				{
					return Core::Memory::New<ListenerSubsystem>(Value, Element);
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
						Core::Memory::Delete(this);
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
			GuiException::GuiException(Core::String&& NewMessage)
			{
				Message = std::move(NewMessage);
			}
			const char* GuiException::type() const noexcept
			{
				return "gui_error";
			}

			void IVariant::Convert(Rml::Variant* From, Core::Variant* To)
			{
#ifdef VI_RMLUI
				VI_ASSERT(From && To, "from and to should be set");
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
#ifdef VI_RMLUI
				VI_ASSERT(From && To, "from and to should be set");
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
			Compute::Vector4 IVariant::ToColor4(const std::string_view& Value)
			{
				VI_ASSERT(Core::Stringify::IsCString(Value), "value should be set");
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
						if (sscanf(Value.data(), "#%02x%02x%02x%02x", &R, &G, &B, &A) == 4)
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
					if (sscanf(Value.data(), "%u %u %u %u", &R, &G, &B, &A) == 4)
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
					return Core::Stringify::Text("%d %d %d %d", (unsigned int)(Base.X * 255.0f), (unsigned int)(Base.Y * 255.0f), (unsigned int)(Base.Z * 255.0f), (unsigned int)(Base.W * 255.0f));

				return Core::Stringify::Text("#%02x%02x%02x%02x",
					(unsigned int)(Base.X * 255.0f),
					(unsigned int)(Base.Y * 255.0f),
					(unsigned int)(Base.Z * 255.0f),
					(unsigned int)(Base.W * 255.0f));
			}
			Compute::Vector4 IVariant::ToColor3(const std::string_view& Value)
			{
				VI_ASSERT(Core::Stringify::IsCString(Value), "value should be set");
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
						Fills = sscanf(Value.data(), "#%02x%02x%02x", &R, &G, &B);

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
					if (sscanf(Value.data(), "%u %u %u", &R, &G, &B) == 3)
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
					return Core::Stringify::Text("%d %d %d", (unsigned int)(Base.X * 255.0f), (unsigned int)(Base.Y * 255.0f), (unsigned int)(Base.Z * 255.0f));

				return Core::Stringify::Text("#%02x%02x%02x",
					(unsigned int)(Base.X * 255.0f),
					(unsigned int)(Base.Y * 255.0f),
					(unsigned int)(Base.Z * 255.0f));
			}
			int IVariant::GetVectorType(const std::string_view& Value)
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
			Compute::Vector4 IVariant::ToVector4(const std::string_view& Base)
			{
				VI_ASSERT(Core::Stringify::IsCString(Base), "value should be set");
				Compute::Vector4 Result;
				if (sscanf(Base.data(), "v4 %f %f %f %f", &Result.X, &Result.Y, &Result.Z, &Result.W) != 4)
					return Result;

				return Result;
			}
			Core::String IVariant::FromVector4(const Compute::Vector4& Base)
			{
				return Core::Stringify::Text("v4 %f %f %f %f", Base.X, Base.Y, Base.Z, Base.W);
			}
			Compute::Vector3 IVariant::ToVector3(const std::string_view& Base)
			{
				VI_ASSERT(Core::Stringify::IsCString(Base), "value should be set");
				Compute::Vector3 Result;
				if (sscanf(Base.data(), "v3 %f %f %f", &Result.X, &Result.Y, &Result.Z) != 3)
					return Result;

				return Result;
			}
			Core::String IVariant::FromVector3(const Compute::Vector3& Base)
			{
				return Core::Stringify::Text("v3 %f %f %f", Base.X, Base.Y, Base.Z);
			}
			Compute::Vector2 IVariant::ToVector2(const std::string_view& Base)
			{
				VI_ASSERT(Core::Stringify::IsCString(Base), "value should be set");
				Compute::Vector2 Result;
				if (sscanf(Base.data(), "v2 %f %f", &Result.X, &Result.Y) != 2)
					return Result;

				return Result;
			}
			Core::String IVariant::FromVector2(const Compute::Vector2& Base)
			{
				return Core::Stringify::Text("v2 %f %f", Base.X, Base.Y);
			}

			IEvent::IEvent() : Base(nullptr), Owned(false)
			{
			}
			IEvent::IEvent(Rml::Event* Ref) : Base(Ref), Owned(false)
			{
			}
			IEvent IEvent::Copy()
			{
#ifdef VI_RMLUI
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
#ifdef VI_RMLUI
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
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "event should be valid");
				return (EventPhase)Base->GetPhase();
#else
				return EventPhase::None;
#endif
			}
			void IEvent::SetPhase(EventPhase Phase)
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "event should be valid");
				Base->SetPhase((Rml::EventPhase)Phase);
#endif
			}
			void IEvent::SetCurrentElement(const IElement& Element)
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "event should be valid");
				Base->SetCurrentElement(Element.GetElement());
#endif
			}
			IElement IEvent::GetCurrentElement() const
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "event should be valid");
				return Base->GetCurrentElement();
#else
				return IElement();
#endif
			}
			IElement IEvent::GetTargetElement() const
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "event should be valid");
				return Base->GetTargetElement();
#else
				return IElement();
#endif
			}
			Core::String IEvent::GetType() const
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "event should be valid");
				return Base->GetType();
#else
				return Core::String();
#endif
			}
			void IEvent::StopPropagation()
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "event should be valid");
				Base->StopPropagation();
#endif
			}
			void IEvent::StopImmediatePropagation()
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "event should be valid");
				Base->StopImmediatePropagation();
#endif
			}
			bool IEvent::IsInterruptible() const
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "event should be valid");
				return Base->IsInterruptible();
#else
				return false;
#endif
			}
			bool IEvent::IsPropagating() const
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "event should be valid");
				return Base->IsPropagating();
#else
				return false;
#endif
			}
			bool IEvent::IsImmediatePropagating() const
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "event should be valid");
				return Base->IsImmediatePropagating();
#else
				return false;
#endif
			}
			bool IEvent::GetBoolean(const std::string_view& Key) const
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "event should be valid");
				return Base->GetParameter<bool>(Core::String(Key), false);
#else
				return false;
#endif
			}
			int64_t IEvent::GetInteger(const std::string_view& Key) const
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "event should be valid");
				return Base->GetParameter<int64_t>(Core::String(Key), 0);
#else
				return 0;
#endif
			}
			double IEvent::GetNumber(const std::string_view& Key) const
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "event should be valid");
				return Base->GetParameter<double>(Core::String(Key), 0.0);
#else
				return 0.0;
#endif
			}
			Core::String IEvent::GetString(const std::string_view& Key) const
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "event should be valid");
				return Base->GetParameter<Rml::String>(Core::String(Key), "");
#else
				return Core::String();
#endif
			}
			Compute::Vector2 IEvent::GetVector2(const std::string_view& Key) const
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "event should be valid");
				Rml::Vector2f Result = Base->GetParameter<Rml::Vector2f>(Core::String(Key), Rml::Vector2f());
				return Compute::Vector2(Result.x, Result.y);
#else
				return Compute::Vector2();
#endif
			}
			Compute::Vector3 IEvent::GetVector3(const std::string_view& Key) const
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "event should be valid");
				Rml::Vector3f Result = Base->GetParameter<Rml::Vector3f>(Core::String(Key), Rml::Vector3f());
				return Compute::Vector3(Result.x, Result.y, Result.z);
#else
				return Compute::Vector3();
#endif
			}
			Compute::Vector4 IEvent::GetVector4(const std::string_view& Key) const
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "event should be valid");
				Rml::Vector4f Result = Base->GetParameter<Rml::Vector4f>(Core::String(Key), Rml::Vector4f());
				return Compute::Vector4(Result.x, Result.y, Result.z, Result.w);
#else
				return Compute::Vector4();
#endif
			}
			void* IEvent::GetPointer(const std::string_view& Key) const
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "event should be valid");
				return Base->GetParameter<void*>(Core::String(Key), nullptr);
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
#ifdef VI_RMLUI
				Core::Memory::Delete(Base);
				Base = nullptr;
#endif
			}
			IElement IElement::Clone() const
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
				Rml::ElementPtr Ptr = Base->Clone();
				Rml::Element* Result = Ptr.get();
				Ptr.reset();

				return Result;
#else
				return IElement();
#endif
			}
			void IElement::SetClass(const std::string_view& ClassName, bool Activate)
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
				Base->SetClass(Core::String(ClassName), Activate);
#endif
			}
			bool IElement::IsClassSet(const std::string_view& ClassName) const
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
				return Base->IsClassSet(Core::String(ClassName));
#else
				return false;
#endif
			}
			void IElement::SetClassNames(const std::string_view& ClassNames)
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
				Base->SetClassNames(Core::String(ClassNames));
#endif
			}
			Core::String IElement::GetClassNames() const
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
				return Base->GetClassNames();
#else
				return Core::String();
#endif

			}
			Core::String IElement::GetAddress(bool IncludePseudoClasses, bool IncludeParents) const
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
				return Base->GetAddress(IncludePseudoClasses, IncludeParents);
#else
				return Core::String();
#endif

			}
			void IElement::SetOffset(const Compute::Vector2& Offset, const IElement& OffsetParent, bool OffsetFixed)
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
				Base->SetOffset(Rml::Vector2f(Offset.X, Offset.Y), OffsetParent.GetElement(), OffsetFixed);
#endif
			}
			Compute::Vector2 IElement::GetRelativeOffset(Area Type)
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
				Rml::Vector2f Result = Base->GetRelativeOffset((Rml::BoxArea)Type);
				return Compute::Vector2(Result.x, Result.y);
#else
				return Compute::Vector2();
#endif

			}
			Compute::Vector2 IElement::GetAbsoluteOffset(Area Type)
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
				Rml::Vector2f Result = Base->GetAbsoluteOffset((Rml::BoxArea)Type);
				return Compute::Vector2(Result.x, Result.y);
#else
				return Compute::Vector2();
#endif

			}
			void IElement::SetClientArea(Area ClientArea)
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
				Base->SetClientArea((Rml::BoxArea)ClientArea);
#endif
			}
			Area IElement::GetClientArea() const
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
				return (Area)Base->GetClientArea();
#else
				return Area::Margin;
#endif

			}
			void IElement::SetContentBox(const Compute::Vector2& ContentBox)
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
				Base->SetBox(Rml::Box(Rml::Vector2f(ContentBox.X, ContentBox.Y)));
#endif
			}
			float IElement::GetBaseline() const
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
				return Base->GetBaseline();
#else
				return 0.0f;
#endif

			}
			bool IElement::GetIntrinsicDimensions(Compute::Vector2& Dimensions, float& Ratio)
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
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
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
				return Base->IsPointWithinElement(Rml::Vector2f(Point.X, Point.Y));
#else
				return false;
#endif

			}
			bool IElement::IsVisible() const
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
				return Base->IsVisible();
#else
				return false;
#endif

			}
			float IElement::GetZIndex() const
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
				return Base->GetZIndex();
#else
				return 0.0f;
#endif

			}
			bool IElement::SetProperty(const std::string_view& Name, const std::string_view& Value)
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
				return Base->SetProperty(Core::String(Name), Core::String(Value));
#else
				return false;
#endif

			}
			void IElement::RemoveProperty(const std::string_view& Name)
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
				Base->RemoveProperty(Core::String(Name));
#endif
			}
			Core::String IElement::GetProperty(const std::string_view& Name)
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
				const Rml::Property* Result = Base->GetProperty(Core::String(Name));
				if (!Result)
					return "";

				return Result->ToString();
#else
				return Core::String();
#endif
			}
			Core::String IElement::GetLocalProperty(const std::string_view& Name)
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
				const Rml::Property* Result = Base->GetLocalProperty(Core::String(Name));
				if (!Result)
					return "";

				return Result->ToString();
#else
				return Core::String();
#endif
			}
			float IElement::ResolveNumericProperty(float Value, NumericUnit Unit, float BaseValue)
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
				return Base->ResolveNumericValue(Rml::NumericValue(Value, (Rml::Unit)Unit), BaseValue);
#else
				return 0.0f;
#endif
			}
			Compute::Vector2 IElement::GetContainingBlock()
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
				Rml::Vector2f Result = Base->GetContainingBlock();
				return Compute::Vector2(Result.x, Result.y);
#else
				return Compute::Vector2();
#endif
			}
			Position IElement::GetPosition()
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
				return (Position)Base->GetPosition();
#else
				return Position::Static;
#endif
			}
			Float IElement::GetFloat()
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
				return (Float)Base->GetFloat();
#else
				return Float::None;
#endif
			}
			Display IElement::GetDisplay()
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
				return (Display)Base->GetDisplay();
#else
				return Display::None;
#endif
			}
			float IElement::GetLineHeight()
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
				return Base->GetLineHeight();
#else
				return 0.0f;
#endif
			}
			bool IElement::Project(Compute::Vector2& Point) const noexcept
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
				Rml::Vector2f Offset(Point.X, Point.Y);
				bool Result = Base->Project(Offset);
				Point = Compute::Vector2(Offset.x, Offset.y);

				return Result;
#else
				return false;
#endif
			}
			bool IElement::Animate(const std::string_view& PropertyName, const std::string_view& TargetValue, float Duration, TimingFunc Func, TimingDir Dir, int NumIterations, bool AlternateDirection, float Delay)
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
				return Base->Animate(Core::String(PropertyName), Rml::Property(Core::String(TargetValue), Rml::Unit::TRANSFORM), Duration, Rml::Tween((Rml::Tween::Type)Func, (Rml::Tween::Direction)Dir), NumIterations, AlternateDirection, Delay);
#else
				return false;
#endif
			}
			bool IElement::AddAnimationKey(const std::string_view& PropertyName, const std::string_view& TargetValue, float Duration, TimingFunc Func, TimingDir Dir)
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
				return Base->AddAnimationKey(Core::String(PropertyName), Rml::Property(Core::String(TargetValue), Rml::Unit::TRANSFORM), Duration, Rml::Tween((Rml::Tween::Type)Func, (Rml::Tween::Direction)Dir));
#else
				return false;
#endif
			}
			void IElement::SetPseudoClass(const std::string_view& PseudoClass, bool Activate)
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
				Base->SetPseudoClass(Core::String(PseudoClass), Activate);
#endif
			}
			bool IElement::IsPseudoClassSet(const std::string_view& PseudoClass) const
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
				return Base->IsPseudoClassSet(Core::String(PseudoClass));
#else
				return false;
#endif
			}
			void IElement::SetAttribute(const std::string_view& Name, const std::string_view& Value)
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
				Base->SetAttribute(Core::String(Name), Core::String(Value));
#endif
			}
			Core::String IElement::GetAttribute(const std::string_view& Name)
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
				return Base->GetAttribute<Core::String>(Core::String(Name), "");
#else
				return Core::String();
#endif
			}
			bool IElement::HasAttribute(const std::string_view& Name) const
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
				return Base->HasAttribute(Core::String(Name));
#else
				return false;
#endif
			}
			void IElement::RemoveAttribute(const std::string_view& Name)
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
				Base->RemoveAttribute(Core::String(Name));
#endif
			}
			IElement IElement::GetFocusLeafNode()
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
				return Base->GetFocusLeafNode();
#else
				return IElement();
#endif
			}
			Core::String IElement::GetTagName() const
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
				return Base->GetTagName();
#else
				return Core::String();
#endif
			}
			Core::String IElement::GetId() const
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
				return Base->GetId();
#else
				return Core::String();
#endif
			}
			void IElement::SetId(const std::string_view& Id)
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
				Base->SetId(Core::String(Id));
#endif
			}
			float IElement::GetAbsoluteLeft()
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
				return Base->GetAbsoluteLeft();
#else
				return 0.0f;
#endif
			}
			float IElement::GetAbsoluteTop()
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
				return Base->GetAbsoluteTop();
#else
				return 0.0f;
#endif
			}
			float IElement::GetClientLeft()
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
				return Base->GetClientLeft();
#else
				return 0.0f;
#endif
			}
			float IElement::GetClientTop()
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
				return Base->GetClientTop();
#else
				return 0.0f;
#endif
			}
			float IElement::GetClientWidth()
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
				return Base->GetClientWidth();
#else
				return 0.0f;
#endif
			}
			float IElement::GetClientHeight()
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
				return Base->GetClientHeight();
#else
				return 0.0f;
#endif
			}
			IElement IElement::GetOffsetParent()
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
				return Base->GetOffsetParent();
#else
				return IElement();
#endif
			}
			float IElement::GetOffsetLeft()
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
				return Base->GetOffsetLeft();
#else
				return 0.0f;
#endif
			}
			float IElement::GetOffsetTop()
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
				return Base->GetOffsetTop();
#else
				return 0.0f;
#endif
			}
			float IElement::GetOffsetWidth()
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
				return Base->GetOffsetWidth();
#else
				return 0.0f;
#endif
			}
			float IElement::GetOffsetHeight()
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
				return Base->GetOffsetHeight();
#else
				return 0.0f;
#endif
			}
			float IElement::GetScrollLeft()
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
				return Base->GetScrollLeft();
#else
				return 0.0f;
#endif
			}
			void IElement::SetScrollLeft(float ScrollLeft)
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
				Base->SetScrollLeft(ScrollLeft);
#endif
			}
			float IElement::GetScrollTop()
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
				return Base->GetScrollTop();
#else
				return 0.0f;
#endif
			}
			void IElement::SetScrollTop(float ScrollTop)
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
				Base->SetScrollTop(ScrollTop);
#endif
			}
			float IElement::GetScrollWidth()
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
				return Base->GetScrollWidth();
#else
				return 0.0f;
#endif
			}
			float IElement::GetScrollHeight()
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
				return Base->GetScrollHeight();
#else
				return 0.0f;
#endif
			}
			IElementDocument IElement::GetOwnerDocument() const
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
				return Base->GetOwnerDocument();
#else
				return IElementDocument();
#endif
			}
			IElement IElement::GetParentNode() const
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
				return Base->GetParentNode();
#else
				return IElement();
#endif
			}
			IElement IElement::GetNextSibling() const
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
				return Base->GetNextSibling();
#else
				return IElement();
#endif
			}
			IElement IElement::GetPreviousSibling() const
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
				return Base->GetPreviousSibling();
#else
				return IElement();
#endif
			}
			IElement IElement::GetFirstChild() const
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
				return Base->GetFirstChild();
#else
				return IElement();
#endif
			}
			IElement IElement::GetLastChild() const
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
				return Base->GetLastChild();
#else
				return IElement();
#endif
			}
			IElement IElement::GetChild(int Index) const
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
				return Base->GetChild(Index);
#else
				return IElement();
#endif
			}
			int IElement::GetNumChildren(bool IncludeNonDOMElements) const
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
				return Base->GetNumChildren(IncludeNonDOMElements);
#else
				return 0;
#endif
			}
			void IElement::GetInnerHTML(Core::String& Content) const
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
				Base->GetInnerRML(Content);
#endif
			}
			Core::String IElement::GetInnerHTML() const
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
				return Base->GetInnerRML();
#else
				return Core::String();
#endif
			}
			void IElement::SetInnerHTML(const std::string_view& HTML)
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
				Base->SetInnerRML(Core::String(HTML));
#endif
			}
			bool IElement::IsFocused()
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
				return Base->IsPseudoClassSet("focus");
#else
				return false;
#endif
			}
			bool IElement::IsHovered()
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
				return Base->IsPseudoClassSet("hover");
#else
				return false;
#endif
			}
			bool IElement::IsActive()
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
				return Base->IsPseudoClassSet("active");
#else
				return false;
#endif
			}
			bool IElement::IsChecked()
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
				return Base->IsPseudoClassSet("checked");
#else
				return false;
#endif
			}
			bool IElement::Focus()
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
				return Base->Focus();
#else
				return false;
#endif
			}
			void IElement::Blur()
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
				Base->Blur();
#endif
			}
			void IElement::Click()
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
				Base->Click();
#endif
			}
			void IElement::AddEventListener(const std::string_view& Event, Listener* Source, bool InCapturePhase)
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
				VI_ASSERT(Source != nullptr && Source->Base != nullptr, "listener should be set");
				Base->AddEventListener(Core::String(Event), Source->Base, InCapturePhase);
#endif
			}
			void IElement::RemoveEventListener(const std::string_view& Event, Listener* Source, bool InCapturePhase)
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
				VI_ASSERT(Source != nullptr && Source->Base != nullptr, "listener should be set");
				Base->RemoveEventListener(Core::String(Event), Source->Base, InCapturePhase);
#endif
			}
			bool IElement::DispatchEvent(const std::string_view& Type, const Core::VariantArgs& Args)
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
				Rml::Dictionary Props;
				for (auto& Item : Args)
				{
					Rml::Variant& Prop = Props[Item.first];
					IVariant::Revert((Core::Variant*)&Item.second, &Prop);
				}

				return Base->DispatchEvent(Core::String(Type), Props);
#else
				return false;
#endif
			}
			void IElement::ScrollIntoView(bool AlignWithTop)
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
				Base->ScrollIntoView(AlignWithTop);
#endif
			}
			IElement IElement::AppendChild(const IElement& Element, bool DOMElement)
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
				return Base->AppendChild(Rml::ElementPtr(Element.GetElement()), DOMElement);
#else
				return IElement();
#endif
			}
			IElement IElement::InsertBefore(const IElement& Element, const IElement& AdjacentElement)
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
				return Base->InsertBefore(Rml::ElementPtr(Element.GetElement()), AdjacentElement.GetElement());
#else
				return IElement();
#endif
			}
			IElement IElement::ReplaceChild(const IElement& InsertedElement, const IElement& ReplacedElement)
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
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
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
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
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
				return Base->HasChildNodes();
#else
				return false;
#endif
			}
			IElement IElement::GetElementById(const std::string_view& Id)
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
				return Base->GetElementById(Core::String(Id));
#else
				return IElement();
#endif
			}
			IElement IElement::QuerySelector(const std::string_view& Selector)
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
				return Base->QuerySelector(Core::String(Selector));
#else
				return IElement();
#endif
			}
			Core::Vector<IElement> IElement::QuerySelectorAll(const std::string_view& Selectors)
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
				Rml::ElementList Elements;
				Base->QuerySelectorAll(Elements, Core::String(Selectors));

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
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
				VI_ASSERT(Ptr != nullptr, "ptr should be set");

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
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
				VI_ASSERT(Ptr != nullptr, "ptr should be set");

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
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
				VI_ASSERT(Ptr != nullptr, "ptr should be set");

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
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
				VI_ASSERT(Ptr != nullptr, "ptr should be set");

				Rml::ElementFormControl* Form = (Rml::ElementFormControl*)Base;
				Core::String Value(Form->GetValue());
				if (Value.empty())
				{
					if (Form->IsPseudoClassSet("focus"))
						return false;

					Form->SetValue(Core::ToString(*Ptr));
					return false;
				}

				if (!Core::Stringify::HasInteger(Value))
				{
					Core::Stringify::ReplaceNotOf(Value, ".-0123456789", "");
					Form->SetValue(Value);
				}

				auto N = Core::FromString<int32_t>(Value);
				if (!N || *N == *Ptr)
					return false;

				if (Form->IsPseudoClassSet("focus"))
				{
					*Ptr = *N;
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
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
				VI_ASSERT(Ptr != nullptr, "ptr should be set");

				Rml::ElementFormControl* Form = (Rml::ElementFormControl*)Base;
				Core::String Value(Form->GetValue());
				if (Value.empty())
				{
					if (Form->IsPseudoClassSet("focus"))
						return false;

					Form->SetValue(Core::ToString(*Ptr));
					return false;
				}

				if (!Core::Stringify::HasInteger(Value))
				{
					Core::Stringify::ReplaceNotOf(Value, ".0123456789", "");
					Form->SetValue(Value);
				}

				auto N = Core::FromString<uint32_t>(Value);
				if (!N || *N == *Ptr)
					return false;

				if (Form->IsPseudoClassSet("focus"))
				{
					*Ptr = *N;
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
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
				VI_ASSERT(Ptr != nullptr, "ptr should be set");

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
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
				VI_ASSERT(Ptr != nullptr, "ptr should be set");

				Rml::ElementFormControl* Form = (Rml::ElementFormControl*)Base;
				Core::String Value(Form->GetValue());
				if (Value.empty())
				{
					if (Form->IsPseudoClassSet("focus"))
						return false;

					Form->SetValue(Core::ToString(*Ptr));
					return false;
				}

				if (!Core::Stringify::HasInteger(Value))
				{
					Core::Stringify::ReplaceNotOf(Value, ".-0123456789", "");
					Form->SetValue(Value);
				}

				auto N = Core::FromString<int64_t>(Value);
				if (!N || *N == *Ptr)
					return false;

				if (Form->IsPseudoClassSet("focus"))
				{
					*Ptr = *N;
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
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
				VI_ASSERT(Ptr != nullptr, "ptr should be set");

				Rml::ElementFormControl* Form = (Rml::ElementFormControl*)Base;
				Core::String Value(Form->GetValue());
				if (Value.empty())
				{
					if (Form->IsPseudoClassSet("focus"))
						return false;

					Form->SetValue(Core::ToString(*Ptr));
					return false;
				}

				if (!Core::Stringify::HasInteger(Value))
				{
					Core::Stringify::ReplaceNotOf(Value, ".0123456789", "");
					Form->SetValue(Value);
				}

				auto N = Core::FromString<uint64_t>(Value);
				if (!N || *N == *Ptr)
					return false;

				if (Form->IsPseudoClassSet("focus"))
				{
					*Ptr = *N;
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
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
				VI_ASSERT(Ptr != nullptr, "ptr should be set");

				Rml::ElementFormControl* Form = (Rml::ElementFormControl*)Base;
				Core::String Value(Form->GetValue());
				if (Value.empty())
				{
					if (Form->IsPseudoClassSet("focus"))
						return false;

					Form->SetValue(Core::ToString(*Ptr));
					return false;
				}

				if (!Core::Stringify::HasInteger(Value))
				{
					Core::Stringify::ReplaceNotOf(Value, ".0123456789", "");
					Form->SetValue(Value);
				}

				auto N = Core::FromString<uint64_t>(Value);
				if (!N || *N == (uint64_t)*Ptr)
					return false;

				if (Form->IsPseudoClassSet("focus"))
				{
					*Ptr = (size_t)*N;
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
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
				VI_ASSERT(Ptr != nullptr, "ptr should be set");

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
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
				VI_ASSERT(Ptr != nullptr, "ptr should be set");

				Rml::ElementFormControl* Form = (Rml::ElementFormControl*)Base;
				Core::String Value(Form->GetValue());
				if (Value.empty())
				{
					if (Form->IsPseudoClassSet("focus"))
						return false;

					Form->SetValue(Core::ToString(*Ptr));
					return false;
				}

				if (!Core::Stringify::HasNumber(Value))
				{
					Core::Stringify::ReplaceNotOf(Value, ".-0123456789", "");
					Form->SetValue(Value);
				}

				auto N = Core::FromString<float>(Value);
				if (!N || *N == *Ptr)
					return false;

				if (Form->IsPseudoClassSet("focus"))
				{
					*Ptr = *N;
					return true;
				}

				Form->SetValue(Core::ToString(*Ptr));
				return false;
#else
				return false;
#endif
			}
			bool IElement::CastFormFloat(float* Ptr, float Mult)
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
				VI_ASSERT(Ptr != nullptr, "ptr should be set");

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
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
				VI_ASSERT(Ptr != nullptr, "ptr should be set");

				Rml::ElementFormControl* Form = (Rml::ElementFormControl*)Base;
				Core::String Value(Form->GetValue());
				if (Value.empty())
				{
					if (Form->IsPseudoClassSet("focus"))
						return false;

					Form->SetValue(Core::ToString(*Ptr));
					return false;
				}

				if (!Core::Stringify::HasNumber(Value))
				{
					Core::Stringify::ReplaceNotOf(Value, ".-0123456789", "");
					Form->SetValue(Value);
				}

				auto N = Core::FromString<double>(Value);
				if (!N || *N == *Ptr)
					return false;

				if (Form->IsPseudoClassSet("focus"))
				{
					*Ptr = *N;
					return true;
				}

				Form->SetValue(Core::ToString(*Ptr));
				return false;
#else
				return false;
#endif
			}
			bool IElement::CastFormBoolean(bool* Ptr)
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
				VI_ASSERT(Ptr != nullptr, "ptr should be set");

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
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
				Rml::ElementFormControl* Form = (Rml::ElementFormControl*)Base;
				return Form->GetName();
#else
				return Core::String();
#endif
			}
			void IElement::SetFormName(const std::string_view& Name)
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
				Rml::ElementFormControl* Form = (Rml::ElementFormControl*)Base;
				Form->SetName(Core::String(Name));
#endif
			}
			Core::String IElement::GetFormValue() const
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
				Rml::ElementFormControl* Form = (Rml::ElementFormControl*)Base;
				return Form->GetValue();
#else
				return Core::String();
#endif
			}
			void IElement::SetFormValue(const std::string_view& Value)
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
				Rml::ElementFormControl* Form = (Rml::ElementFormControl*)Base;
				Form->SetValue(Core::String(Value));
#endif
			}
			bool IElement::IsFormDisabled() const
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
				Rml::ElementFormControl* Form = (Rml::ElementFormControl*)Base;
				return Form->IsDisabled();
#else
				return false;
#endif
			}
			void IElement::SetFormDisabled(bool Disable)
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
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
			void* IElement::ToPointer(const std::string_view& Value)
			{
				if (Value.empty())
					return nullptr;

				if (!Core::Stringify::HasInteger(Value))
					return nullptr;

				return (void*)(intptr_t)*Core::FromString<int64_t>(Value);
			}

			IElementDocument::IElementDocument() : IElement()
			{
			}
			IElementDocument::IElementDocument(Rml::ElementDocument* Ref) : IElement((Rml::Element*)Ref)
			{
			}
			void IElementDocument::Release()
			{
#ifdef VI_RMLUI
				Rml::ElementDocument* Item = (Rml::ElementDocument*)Base;
				Core::Memory::Delete(Item);
				Base = nullptr;
#endif
			}
			void IElementDocument::SetTitle(const std::string_view& Title)
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
				((Rml::ElementDocument*)Base)->SetTitle(Core::String(Title));
#endif
			}
			void IElementDocument::PullToFront()
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
				((Rml::ElementDocument*)Base)->PullToFront();
#endif
			}
			void IElementDocument::PushToBack()
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
				((Rml::ElementDocument*)Base)->PushToBack();
#endif
			}
			void IElementDocument::Show(ModalFlag Modal, FocusFlag Focus)
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
				((Rml::ElementDocument*)Base)->Show((Rml::ModalFlag)Modal, (Rml::FocusFlag)Focus);
#endif
			}
			void IElementDocument::Hide()
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
				((Rml::ElementDocument*)Base)->Hide();
#endif
			}
			void IElementDocument::Close()
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
				((Rml::ElementDocument*)Base)->Close();
#endif
			}
			Core::String IElementDocument::GetTitle() const
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
				return ((Rml::ElementDocument*)Base)->GetTitle();
#else
				return Core::String();
#endif
			}
			Core::String IElementDocument::GetSourceURL() const
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
				return ((Rml::ElementDocument*)Base)->GetSourceURL();
#else
				return Core::String();
#endif
			}
			IElement IElementDocument::CreateElement(const std::string_view& Name)
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
				Rml::ElementPtr Ptr = ((Rml::ElementDocument*)Base)->CreateElement(Core::String(Name));
				Rml::Element* Result = Ptr.get();
				Ptr.reset();

				return Result;
#else
				return IElement();
#endif
			}
			bool IElementDocument::IsModal() const
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "element should be valid");
				return ((Rml::ElementDocument*)Base)->IsModal();
#else
				return false;
#endif
			}
			Rml::ElementDocument* IElementDocument::GetElementDocument() const
			{
				return (Rml::ElementDocument*)Base;
			}

			Compute::Matrix4x4 Utils::ToMatrix(const void* Matrix) noexcept
			{
#ifdef VI_RMLUI
				VI_ASSERT(Matrix != nullptr, "matrix should be set");
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
			Core::String Utils::EscapeHTML(const std::string_view& Text) noexcept
			{
				Core::String Copy = Core::String(Text);
				Core::Stringify::Replace(Copy, "\r\n", "&nbsp;");
				Core::Stringify::Replace(Copy, "\n", "&nbsp;");
				Core::Stringify::Replace(Copy, "<", "&lt;");
				Core::Stringify::Replace(Copy, ">", "&gt;");
				return Copy;
			}

			Subsystem::Subsystem() noexcept : ContextFactory(nullptr), DocumentFactory(nullptr), ListenerFactory(nullptr), RenderInterface(nullptr), FileInterface(nullptr), SystemInterface(nullptr), Id(0)
			{
#ifdef VI_RMLUI
				RenderInterface = Core::Memory::New<RenderSubsystem>();
				Rml::SetRenderInterface(RenderInterface);

				FileInterface = Core::Memory::New<FileSubsystem>();
				Rml::SetFileInterface(FileInterface);

				SystemInterface = Core::Memory::New<MainSubsystem>();
				Rml::SetSystemInterface(SystemInterface);

				Rml::Initialise();
				{
					ContextFactory = Core::Memory::New<ContextInstancer>();
					Rml::Factory::RegisterContextInstancer(ContextFactory);

					ListenerFactory = Core::Memory::New<ListenerInstancer>();
					Rml::Factory::RegisterEventListenerInstancer(ListenerFactory);

					DocumentFactory = Core::Memory::New<DocumentInstancer>();
					Rml::Factory::RegisterElementInstancer("body", DocumentFactory);
				}
				CreateElements();
#endif
			}
			Subsystem::~Subsystem() noexcept
			{
#ifdef VI_RMLUI
				Rml::Shutdown();
				Core::Memory::Delete(SystemInterface);
				SystemInterface = nullptr;

				Core::Memory::Delete(FileInterface);
				FileInterface = nullptr;

				Core::Memory::Delete(RenderInterface);
				RenderInterface = nullptr;

				Core::Memory::Delete(DocumentFactory);
				DocumentFactory = nullptr;

				Core::Memory::Delete(ListenerFactory);
				ListenerFactory = nullptr;

				Core::Memory::Delete(ContextFactory);
				ContextFactory = nullptr;

				ReleaseElements();
				Shared.Release();
#endif
			}
			void Subsystem::SetShared(Scripting::VirtualMachine* VM, Graphics::Activity* Activity, RenderConstants* Constants, ContentManager* Content, Core::Timer* Time) noexcept
			{
#ifdef VI_RMLUI
				CleanupShared();
				Shared.VM = VM;
				Shared.Activity = Activity;
				Shared.Constants = Constants;
				Shared.Content = Content;
				Shared.Time = Time;
				Shared.AddRef();

				if (RenderInterface != nullptr)
					RenderInterface->Attach(Constants, Content);

				if (SystemInterface != nullptr)
					SystemInterface->Attach(Activity, Time);
#endif
			}
			void Subsystem::SetTranslator(const std::string_view& Name, const TranslationCallback& Callback) noexcept
			{
#ifdef VI_RMLUI
				VI_ASSERT(SystemInterface != nullptr, "system interface should be valid");
				SystemInterface->SetTranslator(Core::String(Name), Callback);
#endif
			}
			void Subsystem::CleanupShared()
			{
#ifdef VI_RMLUI
				if (RenderInterface != nullptr)
					RenderInterface->Detach();

				if (SystemInterface != nullptr)
					SystemInterface->Detach();
#endif
				Shared.Release();
			}
			RenderSubsystem* Subsystem::GetRenderInterface() noexcept
			{
				return RenderInterface;
			}
			FileSubsystem* Subsystem::GetFileInterface() noexcept
			{
				return FileInterface;
			}
			MainSubsystem* Subsystem::GetSystemInterface() noexcept
			{
				return SystemInterface;
			}
			Graphics::GraphicsDevice* Subsystem::GetDevice() noexcept
			{
#ifdef VI_RMLUI
				VI_ASSERT(RenderInterface != nullptr, "render interface should be valid");
				return RenderInterface->GetDevice();
#else
				return nullptr;
#endif
			}
			Graphics::Texture2D* Subsystem::GetBackground() noexcept
			{
#ifdef VI_RMLUI
				VI_ASSERT(RenderInterface != nullptr, "render interface should be valid");
				return RenderInterface->Background;
#else
				return nullptr;
#endif
			}
			Compute::Matrix4x4* Subsystem::GetTransform() noexcept
			{
#ifdef VI_RMLUI
				VI_ASSERT(RenderInterface != nullptr, "render interface should be valid");
				return RenderInterface->GetTransform();
#else
				return nullptr;
#endif
			}
			Compute::Matrix4x4* Subsystem::GetProjection() noexcept
			{
#ifdef VI_RMLUI
				VI_ASSERT(RenderInterface != nullptr, "render interface should be valid");
				return RenderInterface->GetProjection();
#else
				return nullptr;
#endif
			}

			DataModel::DataModel(Rml::DataModelConstructor* Ref) : Base(nullptr)
			{
#ifdef VI_RMLUI
				if (Ref != nullptr)
					Base = Core::Memory::New<Rml::DataModelConstructor>(*Ref);
#endif
			}
			DataModel::~DataModel() noexcept
			{
#ifdef VI_RMLUI
				Detach();
				for (auto Item : Props)
					Core::Memory::Delete(Item.second);

				Core::Memory::Delete(Base);
#endif
			}
			void DataModel::SetDetachCallback(std::function<void()>&& Callback)
			{
				if (Callback)
					Callbacks.emplace_back(std::move(Callback));
			}
			DataNode* DataModel::SetProperty(const std::string_view& Name, const Core::Variant& Value)
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "data node should be valid");
				DataNode* Result = GetProperty(Name);
				if (Result != nullptr)
				{
					Result->Set(Value);
					return Result;
				}

				Core::String Copy = Core::String(Name);
				Result = Core::Memory::New<DataNode>(this, Name, Value);
				if (!Value.IsObject())
				{
					if (Base->BindFunc(Copy, std::bind(&DataNode::GetValue, Result, std::placeholders::_1), std::bind(&DataNode::SetValue, Result, std::placeholders::_1)))
					{
						Props[Copy] = Result;
						return Result;
					}
				}
				else if (Base->Bind(Copy, Result))
				{
					Props[Copy] = Result;
					return Result;
				}
				Core::Memory::Delete(Result);
				return nullptr;
#else
				return nullptr;
#endif
			}
			DataNode* DataModel::SetProperty(const std::string_view& Name, Core::Variant* Value)
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "data node should be valid");
				VI_ASSERT(Value != nullptr, "value should be set");

				DataNode* Sub = GetProperty(Name);
				if (Sub != nullptr)
				{
					Sub->Set(Value);
					return Sub;
				}

				Core::String Copy = Core::String(Name);
				DataNode* Result = Core::Memory::New<DataNode>(this, Name, Value);
				if (Base->Bind(Copy, Result))
				{
					Props[Copy] = Result;
					return Result;
				}

				Core::Memory::Delete(Result);
				return nullptr;
#else
				return nullptr;
#endif
			}
			DataNode* DataModel::SetArray(const std::string_view& Name)
			{
				return SetProperty(Name, Core::Var::Array());
			}
			DataNode* DataModel::SetString(const std::string_view& Name, const std::string_view& Value)
			{
				return SetProperty(Name, Core::Var::String(Value));
			}
			DataNode* DataModel::SetInteger(const std::string_view& Name, int64_t Value)
			{
				return SetProperty(Name, Core::Var::Integer(Value));
			}
			DataNode* DataModel::SetFloat(const std::string_view& Name, float Value)
			{
				return SetProperty(Name, Core::Var::Number(Value));
			}
			DataNode* DataModel::SetDouble(const std::string_view& Name, double Value)
			{
				return SetProperty(Name, Core::Var::Number(Value));
			}
			DataNode* DataModel::SetBoolean(const std::string_view& Name, bool Value)
			{
				return SetProperty(Name, Core::Var::Boolean(Value));
			}
			DataNode* DataModel::SetPointer(const std::string_view& Name, void* Value)
			{
				return SetProperty(Name, Core::Var::Pointer(Value));
			}
			DataNode* DataModel::GetProperty(const std::string_view& Name)
			{
				auto It = Props.find(Core::HglCast(Name));
				if (It != Props.end())
					return It->second;

				return nullptr;
			}
			Core::String DataModel::GetString(const std::string_view& Name)
			{
				DataNode* Result = GetProperty(Name);
				if (!Result)
					return "";

				return Result->Ref->GetBlob();
			}
			int64_t DataModel::GetInteger(const std::string_view& Name)
			{
				DataNode* Result = GetProperty(Name);
				if (!Result)
					return 0;

				return Result->Ref->GetInteger();
			}
			float DataModel::GetFloat(const std::string_view& Name)
			{
				DataNode* Result = GetProperty(Name);
				if (!Result)
					return 0.0f;

				return (float)Result->Ref->GetNumber();
			}
			double DataModel::GetDouble(const std::string_view& Name)
			{
				DataNode* Result = GetProperty(Name);
				if (!Result)
					return 0.0;

				return Result->Ref->GetNumber();
			}
			bool DataModel::GetBoolean(const std::string_view& Name)
			{
				DataNode* Result = GetProperty(Name);
				if (!Result)
					return false;

				return Result->Ref->GetBoolean();
			}
			void* DataModel::GetPointer(const std::string_view& Name)
			{
				DataNode* Result = GetProperty(Name);
				if (!Result)
					return nullptr;

				return Result->Ref->GetPointer();
			}
			bool DataModel::SetCallback(const std::string_view& Name, const DataCallback& Callback)
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "data node should be valid");
				VI_ASSERT(Callback, "callback should not be empty");

				return Base->BindEventCallback(Core::String(Name), [Callback](Rml::DataModelHandle Handle, Rml::Event& Event, const Rml::VariantList& Props)
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
			void DataModel::Change(const std::string_view& VariableName)
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "data node should be valid");
				Base->GetModelHandle().DirtyVariable(Core::String(VariableName));
#endif
			}
			bool DataModel::HasChanged(const std::string_view& VariableName) const
			{
#ifdef VI_RMLUI
				VI_ASSERT(IsValid(), "data node should be valid");
				return Base->GetModelHandle().IsVariableDirty(Core::String(VariableName));
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

			DataNode::DataNode(DataModel* Model, const std::string_view& TopName, const Core::Variant& Initial) noexcept : Name(TopName), Handle(Model), Order(nullptr), Depth(0), Safe(true)
			{
				Ref = Core::Memory::New<Core::Variant>(Initial);
			}
			DataNode::DataNode(DataModel* Model, const std::string_view& TopName, Core::Variant* Reference) noexcept : Name(TopName), Ref(Reference), Handle(Model), Order(nullptr), Depth(0), Safe(false)
			{
			}
			DataNode::DataNode(const DataNode& Other) noexcept : Childs(Other.Childs), Name(Other.Name), Handle(Other.Handle), Order(Other.Order), Depth(0), Safe(Other.Safe)
			{
				if (Safe)
					Ref = Core::Memory::New<Core::Variant>(*Other.Ref);
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
					Core::Memory::Delete(Ref);
			}
			DataNode& DataNode::Insert(size_t Where, const Core::VariantList& Initial, std::pair<void*, size_t>* Top)
			{
				VI_ASSERT(Where <= Childs.size(), "index outside of range");
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
				VI_ASSERT(Where <= Childs.size(), "index outside of range");
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
				VI_ASSERT(Where <= Childs.size(), "index outside of range");
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
				VI_ASSERT(Index < Childs.size(), "index outside of range");
				return Childs[Index];
			}
			bool DataNode::Remove(size_t Index)
			{
				VI_ASSERT(Index < Childs.size(), "index outside of range");
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
					Core::Memory::Delete(Ref);

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
			void DataNode::SetString(const std::string_view& Value)
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
			size_t DataNode::Size() const
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
				return (int64_t)Size();
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
					Ref = Core::Memory::New<Core::Variant>(*Other.Ref);
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
#ifdef VI_RMLUI
				Base = Core::Memory::New<EventSubsystem>(NewCallback);
#endif
			}
			Listener::Listener(const std::string_view& FunctionName)
			{
#ifdef VI_RMLUI
				Base = Core::Memory::New<ListenerSubsystem>(FunctionName, nullptr);
#endif
			}
			Listener::~Listener() noexcept
			{
#ifdef VI_RMLUI
				Base->OnDetach(nullptr);
#endif
			}

			Context::Context(const Compute::Vector2& Size) : Compiler(nullptr), Cursor(-1.0f), System(Subsystem::Get()), Busy(0)
			{
#ifdef VI_RMLUI
				Base = (ScopedContext*)Rml::CreateContext(Core::ToString(Subsystem::Get()->Id++), Rml::Vector2i((int)Size.X, (int)Size.Y));
				VI_ASSERT(Base != nullptr, "context cannot be created");
				Base->Basis = this;
				if (System->Shared.VM != nullptr)
				{
					Compiler = System->Shared.VM->CreateCompiler();
					ClearScope();
				}
				System->AddRef();
#endif
			}
			Context::Context(Graphics::GraphicsDevice* Device) : Context(Device && Device->GetRenderTarget() ? Compute::Vector2((float)Device->GetRenderTarget()->GetWidth(), (float)Device->GetRenderTarget()->GetHeight()) : Compute::Vector2(512, 512))
			{
			}
			Context::~Context() noexcept
			{
#ifdef VI_RMLUI
				RemoveDataModels();
				Rml::RemoveContext(Base->GetName());
				Core::Memory::Release(Compiler);
				Core::Memory::Release(System);
#endif
			}
			void Context::EmitKey(Graphics::KeyCode Key, Graphics::KeyMod Mod, int Virtual, int Repeat, bool Pressed)
			{
#ifdef VI_RMLUI
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
#ifdef VI_RMLUI
				VI_ASSERT(Buffer != nullptr && Length > 0, "buffer should be set");
				if (Base->ProcessTextInput(Core::String(Buffer, Length)))
					Inputs.Text = true;
#endif
			}
			void Context::EmitWheel(int X, int Y, bool Normal, Graphics::KeyMod Mod)
			{
#ifdef VI_RMLUI
				if (Base->ProcessMouseWheel((float)-Y, GetKeyMod(Mod)))
					Inputs.Scroll = true;
#endif
			}
			void Context::EmitResize(int Width, int Height)
			{
#ifdef VI_RMLUI
				RenderSubsystem* Renderer = Subsystem::Get()->GetRenderInterface();
				if (Renderer != nullptr)
					Renderer->ResizeBuffers(Width, Height);

				Base->SetDimensions(Rml::Vector2i(Width, Height));
#endif
			}
			void Context::UpdateEvents(Graphics::Activity* Activity)
			{
#ifdef VI_RMLUI
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
#ifdef VI_RMLUI
				VI_ASSERT(Subsystem::Get()->GetRenderInterface() != nullptr, "render interface should be valid");
				RenderSubsystem* Renderer = Subsystem::Get()->GetRenderInterface();
				Renderer->Background = Target;
				Base->Render();
#endif
			}
			void Context::ClearStyles()
			{
#ifdef VI_RMLUI
				Rml::StyleSheetFactory::ClearStyleSheetCache();
#endif
			}
			bool Context::ClearDocuments()
			{
#ifdef VI_RMLUI
				++Busy;
				ClearScope();
				Elements.clear();
				Base->UnloadAllDocuments();
				--Busy;

				return true;
#else
				return false;
#endif
			}
			bool Context::IsLoading()
			{
				return Busy > 0;
			}
			bool Context::IsInputFocused()
			{
#ifdef VI_RMLUI
				Rml::Element* Element = Base->GetFocusElement();
				if (!Element)
					return false;

				const Rml::String& Tag = Element->GetTagName();
				return Tag == "input" || Tag == "textarea" || Tag == "select";
#else
				return false;
#endif
			}
			uint64_t Context::GetIdleTimeoutMs(uint64_t MaxTimeout) const
			{
#ifdef VI_RMLUI
				double Timeout = Base->GetNextUpdateDelay();
				if (Timeout < 0.0)
					return 0;

				if (Timeout > (double)MaxTimeout)
					return MaxTimeout;

				return (uint64_t)(1000.0 * Timeout);
#else
				return 0;
#endif
			}
			Core::UnorderedMap<Core::String, Core::Vector<FontInfo>>* Context::GetFontFaces()
			{
#ifdef VI_RMLUI
				return Subsystem::Get()->GetSystemInterface()->GetFontFaces();
#else
				return nullptr;
#endif
			}
			Rml::Context* Context::GetContext()
			{
#ifdef VI_RMLUI
				return Base;
#else
				return nullptr;
#endif
			}
			Compute::Vector2 Context::GetDimensions() const
			{
#ifdef VI_RMLUI
				Rml::Vector2i Size = Base->GetDimensions();
				return Compute::Vector2((float)Size.x, (float)Size.y);
#else
				return Compute::Vector2();
#endif
			}
			void Context::SetDensityIndependentPixelRatio(float DensityIndependentPixelRatio)
			{
#ifdef VI_RMLUI
				Base->SetDensityIndependentPixelRatio(DensityIndependentPixelRatio);
#endif
			}
			void Context::EnableMouseCursor(bool Enable)
			{
#ifdef VI_RMLUI
				Base->EnableMouseCursor(Enable);
#endif
			}
			float Context::GetDensityIndependentPixelRatio() const
			{
#ifdef VI_RMLUI
				return Base->GetDensityIndependentPixelRatio();
#else
				return 0.0f;
#endif
			}
			bool Context::ReplaceHTML(const std::string_view& Selector, const std::string_view& HTML, int Index)
			{
#ifdef VI_RMLUI
				auto* Current = Base->GetDocument(Index);
				if (!Current)
					return false;

				auto TargetPtr = Current->QuerySelector(Core::String(Selector));
				if (!TargetPtr)
					return false;

				TargetPtr->SetInnerRML(Core::String(HTML));
				return true;
#else
				return false;
#endif
			}
			ExpectsGuiException<void> Context::LoadFontFace(const std::string_view& Path, bool UseAsFallback, FontWeight Weight)
			{
#ifdef VI_RMLUI
				VI_ASSERT(Subsystem::Get()->GetSystemInterface() != nullptr, "system interface should be set");
				Core::String TargetPath = Core::String(Path);
				if (!Subsystem::Get()->GetSystemInterface()->AddFontFace(TargetPath, UseAsFallback, Weight))
					return GuiException("initialize font face error: " + TargetPath);

				return Core::Expectation::Met;
#else
				return GuiException("unsupported");
#endif
			}
			ExpectsGuiException<IElementDocument> Context::EvalHTML(const std::string_view& HTML, int Index)
			{
#ifdef VI_RMLUI
				++Busy;
				auto* Current = Base->GetDocument(Index);
				if (!Current)
				{
					Current = Base->LoadDocumentFromMemory("<html><body>" + Core::String(HTML) + "</body></html>");
					if (!Current)
					{
						--Busy;
						return GuiException("eval html: invalid argument");
					}
				}
				else
					Current->SetInnerRML(Core::String(HTML));

				--Busy;
				return IElementDocument(Current);
#else
				return GuiException("unsupported");
#endif
			}
			ExpectsGuiException<IElementDocument> Context::AddCSS(const std::string_view& CSS, int Index)
			{
#ifdef VI_RMLUI
				++Busy;
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
							Style->SetInnerRML(Core::String(CSS));
							HeadPtr->AppendChild(std::move(Style));
						}
						else
							StylePtr->SetInnerRML(Core::String(CSS));
					}
					else
					{
						auto Head = Current->CreateElement("head");
						{
							auto Style = Current->CreateElement("style");
							Style->SetInnerRML(Core::String(CSS));
							Head->AppendChild(std::move(Style));
						}
						Current->AppendChild(std::move(Head));
					}
				}
				else
				{
					Current = Base->LoadDocumentFromMemory("<html><head><style>" + Core::String(CSS) + "</style></head></html>");
					if (!Current)
					{
						--Busy;
						return GuiException("add css: invalid argument");
					}
				}

				--Busy;
				return IElementDocument(Current);
#else
				return GuiException("unsupported");
#endif
			}
			ExpectsGuiException<IElementDocument> Context::LoadCSS(const std::string_view& Path, int Index)
			{
#ifdef VI_RMLUI
				++Busy;
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
					Link->SetAttribute("href", Core::String(Path));
					HeadPtr = Current->AppendChild(std::move(Link));
				}
				else
				{
					Current = Base->LoadDocumentFromMemory("<html><head><link type=\"text/css\" href=\"" + Core::String(Path) + "\" /></head></html>");
					if (!Current)
					{
						--Busy;
						return GuiException("load css: invalid argument");
					}
				}

				--Busy;
				return IElementDocument(Current);
#else
				return GuiException("unsupported");
#endif
			}
			ExpectsGuiException<IElementDocument> Context::LoadDocument(const std::string_view& Path, bool AllowIncludes)
			{
#ifdef VI_RMLUI
				++Busy;
				if (OnMount)
					OnMount(this);

				ContentManager* Content = (Subsystem::Get()->GetRenderInterface() ? Subsystem::Get()->GetRenderInterface()->GetContent() : nullptr);
				auto Subpath = (Content ? Core::OS::Path::Resolve(Path, Content->GetEnvironment(), true) : Core::OS::Path::Resolve(Path));
				auto TargetPath = Subpath ? *Subpath : Path;
				auto File = Core::OS::File::ReadAsString(TargetPath);
				if (!File)
				{
					--Busy;
					return GuiException("load document: invalid path");
				}

				Core::String Data = *File;
				Decompose(Data);
				if (AllowIncludes)
				{
					auto Status = Preprocess(TargetPath, Data);
					if (!Status)
					{
						--Busy;
						return Status.Error();
					}
				}

				Core::String Location(TargetPath);
				Core::Stringify::Replace(Location, '\\', '/');
				auto* Result = Base->LoadDocumentFromMemory(Data, "file:///" + Location);
				--Busy;
				if (!Result)
					return GuiException("load document: invalid argument");

				return IElementDocument(Result);
#else
				return GuiException("unsupported");
#endif
			}
			ExpectsGuiException<IElementDocument> Context::AddDocumentEmpty(const std::string_view& InstancerName)
			{
#ifdef VI_RMLUI
				auto* Result = Base->CreateDocument(Core::String(InstancerName));
				if (!Result)
					return GuiException("add document: invalid argument");

				return IElementDocument(Result);
#else
				return GuiException("unsupported");
#endif
			}
			ExpectsGuiException<IElementDocument> Context::AddDocument(const std::string_view& HTML)
			{
#ifdef VI_RMLUI
				auto* Result = Base->LoadDocumentFromMemory(Core::String(HTML));
				if (!Result)
					return GuiException("add document: invalid argument");

				return IElementDocument(Result);
#else
				return GuiException("unsupported");
#endif
			}
			IElementDocument Context::GetDocument(const std::string_view& Id)
			{
#ifdef VI_RMLUI
				return Base->GetDocument(Core::String(Id));
#else
				return IElementDocument();
#endif
			}
			IElementDocument Context::GetDocument(int Index)
			{
#ifdef VI_RMLUI
				return Base->GetDocument(Index);
#else
				return IElementDocument();
#endif
			}
			int Context::GetNumDocuments() const
			{
#ifdef VI_RMLUI
				return Base->GetNumDocuments();
#else
				return 0;
#endif
			}
			IElement Context::GetElementById(const std::string_view& Id, int DocIndex)
			{
#ifdef VI_RMLUI
				Rml::ElementDocument* Root = Base->GetDocument(DocIndex);
				if (!Root)
					return nullptr;

				auto Map = Elements.find(DocIndex);
				if (Map == Elements.end())
				{
					Core::String Copy = Core::String(Id);
					Rml::Element* Element = Root->GetElementById(Copy);
					if (!Element)
						return nullptr;

					Elements[DocIndex].insert(std::make_pair(Copy, Element));
					return Element;
				}

				auto It = Map->second.find(Core::HglCast(Id));
				if (It != Map->second.end())
					return It->second;

				Core::String Copy = Core::String(Id);
				Rml::Element* Element = Root->GetElementById(Copy);
				Map->second.insert(std::make_pair(Copy, Element));

				return Element;
#else
				return IElement();
#endif
			}
			IElement Context::GetHoverElement()
			{
#ifdef VI_RMLUI
				return Base->GetHoverElement();
#else
				return IElement();
#endif
			}
			IElement Context::GetFocusElement()
			{
#ifdef VI_RMLUI
				return Base->GetFocusElement();
#else
				return IElement();
#endif
			}
			IElement Context::GetRootElement()
			{
#ifdef VI_RMLUI
				return Base->GetRootElement();
#else
				return IElement();
#endif
			}
			IElement Context::GetElementAtPoint(const Compute::Vector2& Point, const IElement& IgnoreElement, const IElement& Element) const
			{
#ifdef VI_RMLUI
				return Base->GetElementAtPoint(Rml::Vector2f(Point.X, Point.Y), IgnoreElement.GetElement(), Element.GetElement());
#else
				return IElement();
#endif
			}
			void Context::PullDocumentToFront(const IElementDocument& Schema)
			{
#ifdef VI_RMLUI
				Base->PullDocumentToFront(Schema.GetElementDocument());
#endif
			}
			void Context::PushDocumentToBack(const IElementDocument& Schema)
			{
#ifdef VI_RMLUI
				Base->PushDocumentToBack(Schema.GetElementDocument());
#endif
			}
			void Context::UnfocusDocument(const IElementDocument& Schema)
			{
#ifdef VI_RMLUI
				Base->UnfocusDocument(Schema.GetElementDocument());
#endif
			}
			void Context::AddEventListener(const std::string_view& Event, Listener* Listener, bool InCapturePhase)
			{
#ifdef VI_RMLUI
				VI_ASSERT(Listener != nullptr && Listener->Base != nullptr, "listener should be valid");
				Base->AddEventListener(Core::String(Event), Listener->Base, InCapturePhase);
#endif
			}
			void Context::RemoveEventListener(const std::string_view& Event, Listener* Listener, bool InCapturePhase)
			{
#ifdef VI_RMLUI
				VI_ASSERT(Listener != nullptr && Listener->Base != nullptr, "listener should be valid");
				Base->RemoveEventListener(Core::String(Event), Listener->Base, InCapturePhase);
#endif
			}
			bool Context::IsMouseInteracting() const
			{
#ifdef VI_RMLUI
				return Base->IsMouseInteracting();
#else
				return false;
#endif
			}
			bool Context::WasInputUsed(uint32_t InputTypeMask)
			{
#ifdef VI_RMLUI
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
#ifdef VI_RMLUI
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
#ifdef VI_RMLUI
				return Base->SetActiveClipRegion(Rml::Vector2i((int)Origin.X, (int)Origin.Y), Rml::Vector2i((int)Dimensions.X, (int)Dimensions.Y));
#endif
			}
			DataModel* Context::SetDataModel(const std::string_view& Name)
			{
#ifdef VI_RMLUI
				Core::String Copy = Core::String(Name);
				Rml::DataModelConstructor Result = Base->CreateDataModel(Copy);
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

				Models[Copy] = Handle;
				return Handle;
#else
				return nullptr;
#endif
			}
			DataModel* Context::GetDataModel(const std::string_view& Name)
			{
				auto It = Models.find(Core::HglCast(Name));
				if (It != Models.end())
					return It->second;

				return nullptr;
			}
			bool Context::RemoveDataModel(const std::string_view& Name)
			{
#ifdef VI_RMLUI
				Core::String Copy = Core::String(Name);
				if (!Base->RemoveDataModel(Copy))
					return false;

				auto It = Models.find(Copy);
				if (It != Models.end())
				{
					if (It->second->OnUnmount)
						It->second->OnUnmount(this);

					Core::Memory::Release(It->second);
					Models.erase(It);
				}

				return true;
#else
				return false;
#endif
			}
			bool Context::RemoveDataModels()
			{
#ifdef VI_RMLUI
				if (Models.empty())
					return false;

				for (auto Item : Models)
				{
					Base->RemoveDataModel(Item.first);
					Core::Memory::Release(Item.second);
				}

				Models.clear();
				return true;
#else
				return false;
#endif
			}
			void Context::SetDocumentsBaseTag(const std::string_view& Tag)
			{
#ifdef VI_RMLUI
				Base->SetDocumentsBaseTag(Core::String(Tag));
#endif
			}
			void Context::SetMountCallback(const ModelCallback& Callback)
			{
				OnMount = Callback;
			}
			ExpectsGuiException<void> Context::Preprocess(const std::string_view& Path, Core::String& Buffer)
			{
				Compute::Preprocessor::Desc Features;
				Features.Conditions = false;
				Features.Defines = false;
				Features.Pragmas = false;

				Compute::IncludeDesc Desc = Compute::IncludeDesc();
				Desc.Exts.push_back(".html");
				Desc.Exts.push_back(".htm");

				auto Directory = Core::OS::Directory::GetWorking();
				if (Directory)
					Desc.Root = *Directory;

				Core::UPtr<Compute::Preprocessor> Processor = new Compute::Preprocessor();
				Processor->SetIncludeOptions(Desc);
				Processor->SetFeatures(Features);
				Processor->SetIncludeCallback([this](Compute::Preprocessor* Processor, const Compute::IncludeResult& File, Core::String& Output) -> Compute::ExpectsPreprocessor<Compute::IncludeType>
				{
					if (File.Module.empty() || (!File.IsFile && !File.IsAbstract))
						return Compute::IncludeType::Error;

					if (File.IsAbstract && !File.IsFile)
						return Compute::IncludeType::Error;

					auto Data = Core::OS::File::ReadAsString(File.Module);
					if (!Data)
						return Compute::IncludeType::Error;

					Output.assign(*Data);
					this->Decompose(Output);
					return Compute::IncludeType::Preprocess;
				});
#ifdef VI_RMLUI
				Processor->SetPragmaCallback([this](Compute::Preprocessor* Processor, const std::string_view& Name, const Core::Vector<Core::String>& Args) -> Compute::ExpectsPreprocessor<void>
				{
					if (Name != "fontface")
						return Core::Expectation::Met;

					Core::String Path;
					FontWeight Weight = FontWeight::Auto;
					bool UseAsFallback = false;
					for (auto& Item : Args)
					{
						if (Core::Stringify::StartsWith(Item, "path:"))
						{
							auto Value = Item.substr(5);
							if ((Value.front() == '\"' && Value.back() == '\"') || (Value.front() == '\'' && Value.back() == '\''))
								Path = Value.substr(1, Value.size() - 2);
							else
								Path = Value;
						}
						else if (Core::Stringify::StartsWith(Item, "weight:"))
						{
							auto Value = Core::Var::Auto(Item.substr(7));
							if (Value.Is(Core::VarType::String))
							{
								if (Value.IsString("auto"))
									Weight = FontWeight::Auto;
								else if (Value.IsString("normal"))
									Weight = FontWeight::Normal;
								else if (Value.IsString("bold"))
									Weight = FontWeight::Bold;
							}
							else
								Weight = (FontWeight)std::min<uint16_t>(1000, (uint16_t)Value.GetInteger());
						}
						else if (Core::Stringify::StartsWith(Item, "fallback:"))
							UseAsFallback = Core::Var::Auto(Item.substr(9)).GetBoolean();
					}

					if (Path.empty())
						return Compute::PreprocessorException(Compute::PreprocessorError::IncludeError, 0, "font face path is invalid");

					Rml::String TargetPath;
					Rml::String CurrentPath = Core::OS::Path::GetDirectory(Processor->GetCurrentFilePath().c_str());
					Rml::GetSystemInterface()->JoinPath(TargetPath, Rml::StringUtilities::Replace(CurrentPath, '|', ':'), Rml::StringUtilities::Replace(Path, '|', ':'));
					auto Status = Context::LoadFontFace(TargetPath, UseAsFallback, Weight);
					if (Status)
						return Core::Expectation::Met;

					return Compute::PreprocessorException(Compute::PreprocessorError::IncludeError, 0, Status.Error().message());
				});
#endif
				auto Status = Processor->Process(Path, Buffer);
				if (!Status)
					return GuiException(std::move(Status.Error().message()));

				return Core::Expectation::Met;
			}
			void Context::Decompose(Core::String& Data)
			{
				Core::TextSettle Start, End;
				while (End.End < Data.size())
				{
					Start = Core::Stringify::Find(Data, "<!--", End.End);
					if (!Start.Found)
						break;

					End = Core::Stringify::Find(Data, "-->", Start.End);
					if (!End.Found)
						break;

					if (!End.Start || Data[End.Start - 1] == '-')
						continue;

					if (Start.End > Data.size() || Data[Start.End] == '-')
					{
						End.End = Start.End;
						continue;
					}

					Core::Stringify::RemovePart(Data, Start.Start, End.End);
					End.End = Start.Start;
				}

				memset(&Start, 0, sizeof(Start));
				memset(&End, 0, sizeof(End));
				while (End.End < Data.size())
				{
					Start = Core::Stringify::Find(Data, "<!---", End.End);
					if (!Start.Found)
						break;

					End = Core::Stringify::Find(Data, "--->", Start.End);
					if (!End.Found)
						break;

					Core::String Expression = Data.substr(Start.End, End.Start - Start.End);
					Core::Stringify::Trim(Expression);
					Core::Stringify::ReplacePart(Data, Start.Start, End.End, Expression);
					End.End = Start.Start;
				}
			}
			void Context::ClearScope()
			{
				if (!Compiler)
					return;

				Compiler->Clear();
				Compiler->Prepare("__scope__", true);
			}
			Core::String Context::ResolveResourcePath(const IElement& Element, const std::string_view& Path)
			{
#ifdef VI_RMLUI
				VI_ASSERT(Subsystem::Get()->GetSystemInterface() != nullptr, "system interface should be set");
				auto RootElement = Element.GetOwnerDocument();
				Core::String SourcePath = Core::String(Path);
				if (!RootElement.IsValid())
					return SourcePath;

				Rml::String TargetPath;
				Rml::GetSystemInterface()->JoinPath(TargetPath, Rml::StringUtilities::Replace(RootElement.GetSourceURL(), '|', ':'), Rml::StringUtilities::Replace(SourcePath, '|', ':'));
				return TargetPath;
#else
				return Core::String(Path);
#endif
			}
			Core::String Context::GetDocumentsBaseTag()
			{
#ifdef VI_RMLUI
				return Base->GetDocumentsBaseTag();
#else
				return Core::String();
#endif
			}
			int Context::GetKeyCode(Graphics::KeyCode Key)
			{
#ifdef VI_RMLUI
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
#ifdef VI_RMLUI
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