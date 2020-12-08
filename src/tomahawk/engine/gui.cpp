#include "gui.h"
#include <RmlUi/Core.h>
#include <RmlUi/Core/Stream.h>
#include <RmlUi/Core/Elements/DataSource.h>
#include <Source/Core/StyleSheetFactory.h>

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
					ScissorNoneRasterizer = nullptr;
					NoneRasterizer = nullptr;
					LessDepthStencil = nullptr;
					NoneDepthStencil = nullptr;
					AlphaBlend = nullptr;
					ColorlessBlend = nullptr;
				}
				virtual ~RenderSubsystem() override
				{
					TH_RELEASE(VertexBuffer);
					TH_RELEASE(Shader);
				}
				virtual void RenderGeometry(Rml::Vertex* Vertices, int VerticesSize, int* Indices, int IndicesSize, Rml::TextureHandle Texture, const Rml::Vector2f& Translation) override
				{
					if (!Device)
						return;

					Device->Begin();
					Device->Topology(Graphics::PrimitiveTopology_Triangle_List);

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
					if (!Device)
						return (Rml::CompiledGeometryHandle)nullptr;

					GeometryBuffer* Result = new GeometryBuffer();
					Result->Texture = (Graphics::Texture2D*)Handle;

					Graphics::ElementBuffer::Desc F = Graphics::ElementBuffer::Desc();
					F.AccessFlags = Graphics::CPUAccess_Invalid;
					F.Usage = Graphics::ResourceUsage_Default;
					F.BindFlags = Graphics::ResourceBind_Vertex_Buffer;
					F.ElementCount = (unsigned int)VerticesCount;
					F.Elements = (void*)Vertices;
					F.ElementWidth = sizeof(Rml::Vertex);
					Result->VertexBuffer = Device->CreateElementBuffer(F);

					F = Graphics::ElementBuffer::Desc();
					F.AccessFlags = Graphics::CPUAccess_Invalid;
					F.Usage = Graphics::ResourceUsage_Default;
					F.BindFlags = Graphics::ResourceBind_Index_Buffer;
					F.ElementCount = (unsigned int)IndicesCount;
					F.ElementWidth = sizeof(int);
					F.Elements = (void*)Indices;
					Result->IndexBuffer = Device->CreateElementBuffer(F);

					return (Rml::CompiledGeometryHandle)Result;
				}
				virtual void RenderCompiledGeometry(Rml::CompiledGeometryHandle Handle, const Rml::Vector2f& Translation) override
				{
					GeometryBuffer* Buffer = (GeometryBuffer*)Handle;
					if (!Device || !Buffer)
						return;

					Device->Render.HasDiffuse = (Buffer->Texture != nullptr);
					if (HasTransform)
						Device->Render.WorldViewProjection = Compute::Matrix4x4::CreateTranslation(Compute::Vector3(Translation.x, Translation.y)) * Transform * Ortho;
					else
						Device->Render.WorldViewProjection = Compute::Matrix4x4::CreateTranslation(Compute::Vector3(Translation.x, Translation.y)) * Ortho;
					
					Device->SetTexture2D(Buffer->Texture, 1);
					Device->SetShader(Shader, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);
					Device->SetVertexBuffer(Buffer->VertexBuffer, 0);
					Device->SetIndexBuffer(Buffer->IndexBuffer, Graphics::Format_R32_Uint);
					Device->UpdateBuffer(Graphics::RenderBufferType_Render);
					Device->DrawIndexed(Buffer->IndexBuffer->GetElements(), 0, 0);
				}
				virtual void ReleaseCompiledGeometry(Rml::CompiledGeometryHandle Handle) override
				{
					GeometryBuffer* Resource = (GeometryBuffer*)Handle;
					delete Resource;
				}
				virtual void EnableScissorRegion(bool Enable) override
				{
					if (!Device)
						return;

					const Rml::Matrix4f Projection = Rml::Matrix4f::ProjectOrtho(0.0f,
						(float)Device->GetRenderTarget()->GetWidth(),
						(float)Device->GetRenderTarget()->GetHeight(), 0.0f, -30000.0f, 10000.0f);
					Ortho = Subsystem::ToMatrix(&Projection);

					Device->SetInputLayout(Layout);
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
				}
				virtual void SetScissorRegion(int X, int Y, int Width, int Height) override
				{
					if (!Device)
						return;

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
					if (Device->Map(VertexBuffer, Graphics::ResourceMap_Write_Discard, &Subresource))
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

					Device->Render.WorldViewProjection = Transform * Ortho;
					Device->ClearDepth();
					Device->SetBlendState(ColorlessBlend);
					Device->SetShader(Shader, Graphics::ShaderType_Vertex | Graphics::ShaderType_Pixel);
					Device->SetVertexBuffer(VertexBuffer, 0);
					Device->UpdateBuffer(Graphics::RenderBufferType_Render);
					Device->Draw(VertexBuffer->GetElements(), 0);
					Device->SetDepthStencilState(ScissorDepthStencil);
					Device->SetBlendState(AlphaBlend);
				}
				virtual bool LoadTexture(Rml::TextureHandle& Handle, Rml::Vector2i& TextureDimensions, const Rml::String& Source) override
				{
					if (!Content)
						return false;

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
					if (!Device)
						return false;

					Graphics::Texture2D::Desc F = Graphics::Texture2D::Desc();
					F.FormatMode = Graphics::Format_R8G8B8A8_Unorm;
					F.Usage = Graphics::ResourceUsage_Default;
					F.BindFlags = Graphics::ResourceBind_Shader_Input;
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
					TH_CLEAR(VertexBuffer);
					TH_CLEAR(Shader);

					Content = NewContent;
					if (Content != nullptr)
					{
						Device = Content->GetDevice();
						if (Device != nullptr)
						{
							Graphics::Shader::Desc I = Graphics::Shader::Desc();
							I.Filename = "element";

							if (Device->GetSection("shaders/gui/geometry", &I.Data))
								Shader = Device->CreateShader(I);

							Graphics::ElementBuffer::Desc F = Graphics::ElementBuffer::Desc();
							F.AccessFlags = Graphics::CPUAccess_Write;
							F.Usage = Graphics::ResourceUsage_Dynamic;
							F.BindFlags = Graphics::ResourceBind_Vertex_Buffer;
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
							return;
						}
					}

					Layout = nullptr;
					ScissorNoneRasterizer = nullptr;
					NoneRasterizer = nullptr;
					LessDepthStencil = nullptr;
					NoneDepthStencil = nullptr;
					ScissorDepthStencil = nullptr;
					AlphaBlend = nullptr;
					ColorlessBlend = nullptr;
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
					if (!Rest::OS::FileExists(Path.c_str()))
					{
						ContentManager* Content = (Subsystem::GetRenderInterface() ? Subsystem::GetRenderInterface()->GetContent() : nullptr);
						std::string Target = (Content ? Rest::OS::Resolve(Path, Content->GetEnvironment()) : Rest::OS::Resolve(Path.c_str()));
						return (Rml::FileHandle)Rest::OS::Open(Target.empty() ? Path.c_str() : Target.c_str(), "rb");
					}

					return (Rml::FileHandle)Rest::OS::Open(Path.c_str(), "rb");
				}
				virtual void Close(Rml::FileHandle File) override
				{
					FILE* Stream = (FILE*)File;
					if (Stream != nullptr)
						fclose(Stream);
				}
				virtual size_t Read(void* Buffer, size_t Size, Rml::FileHandle File) override
				{
					FILE* Stream = (FILE*)File;
					if (!Stream)
						return 0;

					return fread(Buffer, 1, Size, Stream);
				}
				virtual bool Seek(Rml::FileHandle File, long Offset, int Origin) override
				{
					FILE* Stream = (FILE*)File;
					if (!Stream)
						return false;

					return fseek(Stream, Offset, Origin) == 0;
				}
				virtual size_t Tell(Rml::FileHandle File) override
				{
					FILE* Stream = (FILE*)File;
					if (!Stream)
						return 0;

					return ftell(Stream);
				}
			};

			class MainSubsystem : public Rml::SystemInterface
			{
			private:
				std::unordered_map<std::string, TranslationCallback> Translators;
				std::unordered_map<std::string, bool> Fonts;
				Graphics::Activity* Activity;
				Rest::Timer* Time;

			public:
				MainSubsystem() : Rml::SystemInterface(), Time(nullptr), Activity(nullptr)
				{
				}
				virtual void SetMouseCursor(const Rml::String& CursorName) override
				{
					if (!Activity)
						return;
					
					if (CursorName == "none")
						Activity->SetCursor(Graphics::DisplayCursor_None);
					else if (CursorName == "default")
						Activity->SetCursor(Graphics::DisplayCursor_Arrow);
					else if (CursorName == "move")
						Activity->SetCursor(Graphics::DisplayCursor_ResizeAll);
					else if (CursorName == "pointer")
						Activity->SetCursor(Graphics::DisplayCursor_Hand);
					else if (CursorName == "text")
						Activity->SetCursor(Graphics::DisplayCursor_TextInput);
					else if (CursorName == "progress")
						Activity->SetCursor(Graphics::DisplayCursor_Progress);
					else if (CursorName == "wait")
						Activity->SetCursor(Graphics::DisplayCursor_Wait);
					else if (CursorName == "not-allowed")
						Activity->SetCursor(Graphics::DisplayCursor_No);
					else if (CursorName == "crosshair")
						Activity->SetCursor(Graphics::DisplayCursor_Crosshair);
					else if (CursorName == "ns-resize")
						Activity->SetCursor(Graphics::DisplayCursor_ResizeNS);
					else if (CursorName == "ew-resize")
						Activity->SetCursor(Graphics::DisplayCursor_ResizeEW);
					else if (CursorName == "nesw-resize")
						Activity->SetCursor(Graphics::DisplayCursor_ResizeNESW);
					else if (CursorName == "nwse-resize")
						Activity->SetCursor(Graphics::DisplayCursor_ResizeNWSE);
				}
				virtual void SetClipboardText(const Rml::String& Text) override
				{
					if (!Activity)
						return;

					Activity->SetClipboardText(Text);
				}
				virtual void GetClipboardText(Rml::String& Text) override
				{
					if (!Activity)
						return;

					Text = Activity->GetClipboardText();
				}
				virtual void ActivateKeyboard() override
				{
					if (!Activity)
						return;

					Activity->SetScreenKeyboard(true);
				}
				virtual void DeactivateKeyboard() override
				{
					if (!Activity)
						return;

					Activity->SetScreenKeyboard(false);
				}
				virtual void JoinPath(Rml::String& Result, const Rml::String& HTML, const Rml::String& CSS) override
				{
					Result = Rest::OS::Resolve(CSS, Rest::OS::FileDirectory(HTML));
					if (Result.empty())
					{
						ContentManager* Content = (Subsystem::GetRenderInterface() ? Subsystem::GetRenderInterface()->GetContent() : nullptr);
						Result = (Content ? Rest::OS::Resolve(CSS, Content->GetEnvironment()) : Rest::OS::Resolve(CSS.c_str()));
					}
				}
				virtual bool LogMessage(Rml::Log::Type Type, const Rml::String& Message) override
				{
					switch (Type)
					{
						case Rml::Log::LT_ERROR:
							TH_ERROR("[gui] %.*s", Message.size(), Message.c_str());
							break;
						case Rml::Log::LT_WARNING:
							TH_WARN("[gui] %.*s", Message.size(), Message.c_str());
							break;
						case Rml::Log::LT_INFO:
							TH_INFO("[gui] %.*s", Message.size(), Message.c_str());
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
				void Attach(Graphics::Activity* NewActivity, Rest::Timer* NewTime)
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
					return Rml::ContextPtr(new ScopedContext(Name));
				}
				virtual void ReleaseContext(Rml::Context* Context) override
				{
					delete (ScopedContext*)Context;
				}
				virtual void Release() override
				{
					delete this;
				}
			};

			class DocumentSubsystem : public Rml::ElementDocument
			{
			public:
				DocumentSubsystem(const Rml::String& Tag) : Rml::ElementDocument(Tag)
				{
				}
				virtual ~DocumentSubsystem() = default;
				void LoadScript(Rml::Stream* Stream, const Rml::String& SourceName) override
				{
					ScopedContext* Scope = (ScopedContext*)GetContext();
					if (!Scope || !Scope->Basis || !Scope->Basis->Compiler)
						return;

					Script::VMCompiler* Compiler = Scope->Basis->Compiler;
					if (SourceName.empty())
					{
						Rml::String Buffer;
						Stream->Read(Buffer, Stream->Length());

						if (Compiler->LoadCode("__main__", Buffer.c_str(), Buffer.size()) < 0)
							return;
					}
					else if (Compiler->LoadFile(Rest::Stroke(SourceName).Replace('|', ':').R()) < 0)
						return;

					if (Compiler->Compile(true) < 0)
						return;

					Script::VMFunction Main = Compiler->GetModule().GetFunctionByName("Main");
					if (!Main.IsValid())
						return;

					Script::VMContext* Context = Compiler->GetContext();
					if (Context->Prepare(Main) >= 0)
					{
						Context->SetArgObject(0, Scope->Basis);
						Context->Execute();
					}
				}
			};

			class DocumentInstancer : public Rml::ElementInstancer
			{
			public:
				Rml::ElementPtr InstanceElement(Rml::Element*, const Rml::String& Tag, const Rml::XMLAttributes&) override
				{
					return Rml::ElementPtr(new DocumentSubsystem(Tag));
				}
				void ReleaseElement(Rml::Element* Element) override
				{
					delete Element;
				}
			};

			class ListenerSubsystem : public Rml::EventListener
			{
			public:
				std::string Memory;
				bool IsFunction;

			public:
				ListenerSubsystem(const Rml::String& Code, Rml::Element* Element, bool Body) : Memory(Code), IsFunction(Body)
				{
				}
				void OnDetach(Rml::Element* Element) override
				{
					delete this;
				}
				void ProcessEvent(Rml::Event& Event) override
				{
					ScopedContext* Scope = (ScopedContext*)Event.GetCurrentElement()->GetContext();
					if (!Scope || !Scope->Basis || !Scope->Basis->Compiler)
						return;

					Script::VMCompiler* Compiler = Scope->Basis->Compiler;
					if (!IsFunction)
					{
						Compiler->ExecuteEntry(Memory.c_str(), nullptr, Script::VMTypeId_VOID, [this, &Event](Script::VMContext* Context)
						{
							Context->SetArgObject(0, &Event);
						});
					}
					else
						Compiler->ExecuteScoped(Memory);
				}
			};

			class ListenerInstancer : public Rml::EventListenerInstancer
			{
			public:
				Rml::EventListener* InstanceEventListener(const Rml::String& Value, Rml::Element* Element) override
				{
					return new ListenerSubsystem(Value, Element, true);
				}
			};

			class EventSubsystem : public Rml::EventListener
			{
				friend IEvent;

			private:
				EventCallback Handler;
				int RefCount;

			public:
				EventSubsystem(const EventCallback& Callback) : Rml::EventListener(), Handler(Callback), RefCount(1)
				{
				}
				virtual void OnAttach(Rml::Element*)
				{
					RefCount++;
				}
				virtual void OnDetach(Rml::Element*)
				{
					RefCount = false;
					if (!--RefCount)
						delete this;
				}
				virtual void ProcessEvent(Rml::Event& Event) override
				{
					if (!Handler)
						return;

					IEvent Basis(&Event);
					Handler(Basis);
				}
			};

			class DataSourceSubsystem : public Rml::DataSource
			{
				friend GUI::DataSource;

			public:
				GUI::DataSource* Source;

			public:
				DataSourceSubsystem(GUI::DataSource* NewSource) : Rml::DataSource(NewSource ? NewSource->Name : ""), Source(NewSource)
				{
				}
				virtual ~DataSourceSubsystem() override
				{
				}
				virtual void GetRow(Rml::StringList& Row, const Rml::String& Table, int RowIndex, const Rml::StringList& Columns) override
				{
					if (!Source)
						return;

					auto It = Source->Nodes.find(Table);
					if (It == Source->Nodes.end())
						return;

					if (RowIndex < 0 || RowIndex >= It->second->Childs.size())
						return;
					
					DataRow* Target = It->second->Childs[RowIndex];
					std::string Result;

					for (auto& Column : Columns)
					{
						if (Column == Rml::DataSource::DEPTH || Column == "depth")
							Row.push_back(std::to_string(Target->Depth));
						else if (Column == Rml::DataSource::NUM_CHILDREN)
							Row.push_back(std::to_string(Target->Childs.size()));
						else if (Column == Rml::DataSource::CHILD_SOURCE)
							Row.push_back(Source->Name + "." + Target->Name);
						else if (Source->OnColumn)
						{
							Source->OnColumn(Target, Column, Result);
							if (!Result.empty())
							{
								Row.push_back(Result);
								Result.clear();
							}
						}
					}
				}
				virtual int GetNumRows(const Rml::String& Table) override
				{
					if (!Source)
						return 0;

					auto It = Source->Nodes.find(Table);
					if (It == Source->Nodes.end())
						return 0;

					return It->second->Childs.size();
				}
			};

			class DataFormatterSubsystem : public Rml::DataFormatter
			{
			public:
				GUI::DataSource* Source;

			public:
				DataFormatterSubsystem(GUI::DataSource* NewSource) : Rml::DataFormatter(NewSource ? NewSource->Name : ""), Source(NewSource)
				{
				}
				virtual ~DataFormatterSubsystem() override
				{
				}
				virtual void FormatData(Rml::String& FormattedData, const Rml::StringList& RawData) override
				{
					if (Source != nullptr && Source->OnFormat)
						Source->OnFormat(RawData, FormattedData);
				}
			};

			static void ToProperty(Rml::Variant* From, Compute::Property* To)
			{
				if (!From || !To)
					return;

				switch (From->GetType())
				{
					case Rml::Variant::BOOL:
						*To = Compute::Property(From->Get<bool>());
						break;
					case Rml::Variant::FLOAT:
					case Rml::Variant::DOUBLE:
						*To = Compute::Property(From->Get<double>());
						break;
					case Rml::Variant::BYTE:
					case Rml::Variant::CHAR:
					case Rml::Variant::INT:
					case Rml::Variant::INT64:
						*To = Compute::Property(From->Get<int64_t>());
						break;
					case Rml::Variant::VECTOR2:
					{
						Rml::Vector2f T = From->Get<Rml::Vector2f>();
						*To = Compute::Property(Compute::Vector2(T.x, T.y));
						break;
					}
					case Rml::Variant::VECTOR3:
					{
						Rml::Vector3f T = From->Get<Rml::Vector3f>();
						*To = Compute::Property(Compute::Vector3(T.x, T.y, T.z));
						break;
					}
					case Rml::Variant::VECTOR4:
					{
						Rml::Vector4f T = From->Get<Rml::Vector4f>();
						*To = Compute::Property(Compute::Vector4(T.x, T.y, T.z, T.w));
						break;
					}
					case Rml::Variant::STRING:
					case Rml::Variant::COLOURF:
					case Rml::Variant::COLOURB:
						*To = Compute::Property(From->Get<std::string>());
						break;
					case Rml::Variant::VOIDPTR:
						*To = Compute::Property(From->Get<void*>());
						break;
					default:
						*To = Compute::Property();
						break;
				}
			}
			static void FromProperty(Compute::Property* From, Rml::Variant* To)
			{
				if (!From || !To)
					return;

				switch (From->GetType())
				{
					case Compute::PropertyType_Null:
						*To = Rml::Variant((void*)nullptr);
						break;
					case Compute::PropertyType_String:
						*To = Rml::Variant(From->ToString());
						break;
					case Compute::PropertyType_Vector2:
					{
						Compute::Vector2 T = From->GetVector2();
						*To = Rml::Variant(Rml::Vector2f(T.X, T.Y));
						break;
					}
					case Compute::PropertyType_Vector3:
					{
						Compute::Vector3 T = From->GetVector3();
						*To = Rml::Variant(Rml::Vector3f(T.X, T.Y, T.Z));
						break;
					}
					case Compute::PropertyType_Vector4:
					{
						Compute::Vector4 T = From->GetVector4();
						*To = Rml::Variant(Rml::Vector4f(T.X, T.Y, T.Z, T.W));
						break;
					}
					case Compute::PropertyType_Integer:
						*To = Rml::Variant(From->GetInteger());
						break;
					case Compute::PropertyType_Number:
						*To = Rml::Variant(From->GetNumber());
						break;
					case Compute::PropertyType_Boolean:
						*To = Rml::Variant(From->GetBoolean());
						break;
					case Compute::PropertyType_Pointer:
						*To = Rml::Variant(From->GetPointer());
						break;
					default:
						To->Clear();
						break;
				}
			}
			static Compute::Vector4 ToColor4(const std::string& Value)
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
			static std::string FromColor4(const Compute::Vector4& Base, bool HEX)
			{
				if (!HEX)
					return Rest::Form("%d %d %d %d", (unsigned int)(Base.X * 255.0f), (unsigned int)(Base.Y * 255.0f), (unsigned int)(Base.Z * 255.0f), (unsigned int)(Base.W * 255.0f)).R();
				
				return Rest::Form("#%02x%02x%02x%02x",
					(unsigned int)(Base.X * 255.0f),
					(unsigned int)(Base.Y * 255.0f),
					(unsigned int)(Base.Z * 255.0f),
					(unsigned int)(Base.W * 255.0f)).R();
			}
			static Compute::Vector4 ToColor3(const std::string& Value)
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
			static std::string FromColor3(const Compute::Vector4& Base, bool HEX)
			{
				if (!HEX)
					return Rest::Form("%d %d %d", (unsigned int)(Base.X * 255.0f), (unsigned int)(Base.Y * 255.0f), (unsigned int)(Base.Z * 255.0f)).R();

				return Rest::Form("#%02x%02x%02x",
					(unsigned int)(Base.X * 255.0f),
					(unsigned int)(Base.Y * 255.0f),
					(unsigned int)(Base.Z * 255.0f)).R();
			}

			IEvent::IEvent(Rml::Event* Ref) : Base(Ref)
			{
			}
			EventPhase IEvent::GetPhase() const
			{
				if (!IsValid())
					return EventPhase_None;

				return (EventPhase)Base->GetPhase();
			}
			void IEvent::SetPhase(EventPhase Phase)
			{
				if (IsValid())
					Base->SetPhase((Rml::EventPhase)Phase);
			}
			void IEvent::SetCurrentElement(const IElement& Element)
			{
				if (IsValid())
					Base->SetCurrentElement(Element.GetElement());
			}
			IElement IEvent::GetCurrentElement() const
			{
				if (!IsValid())
					return nullptr;

				return Base->GetCurrentElement();
			}
			IElement IEvent::GetTargetElement() const
			{
				if (!IsValid())
					return nullptr;

				return Base->GetTargetElement();
			}
			std::string IEvent::GetType() const
			{
				if (!IsValid())
					return "";

				return Base->GetType();
			}
			void IEvent::StopPropagation()
			{
				if (IsValid())
					Base->StopPropagation();
			}
			void IEvent::StopImmediatePropagation()
			{
				if (IsValid())
					Base->StopImmediatePropagation();
			}
			bool IEvent::IsInterruptible() const
			{
				if (!IsValid())
					return false;

				return Base->IsInterruptible();
			}
			bool IEvent::IsPropagating() const
			{
				if (!IsValid())
					return false;

				return Base->IsPropagating();
			}
			bool IEvent::IsImmediatePropagating() const
			{
				if (!IsValid())
					return false;

				return Base->IsImmediatePropagating();
			}
			bool IEvent::GetBoolean(const std::string& Key) const
			{
				if (!IsValid())
					return false;

				return Base->GetParameter<bool>(Key, false);
			}
			int64_t IEvent::GetInteger(const std::string& Key) const
			{
				if (!IsValid())
					return 0;

				return Base->GetParameter<int64_t>(Key, 0);
			}
			double IEvent::GetNumber(const std::string& Key) const
			{
				if (!IsValid())
					return 0.0;

				return Base->GetParameter<double>(Key, 0.0);
			}
			std::string IEvent::GetString(const std::string& Key) const
			{
				if (!IsValid())
					return "";

				return Base->GetParameter<Rml::String>(Key, "");
			}
			Compute::Vector2 IEvent::GetVector2(const std::string& Key) const
			{
				if (!IsValid())
					return Compute::Vector2();

				Rml::Vector2f Result = Base->GetParameter<Rml::Vector2f>(Key, Rml::Vector2f());
				return Compute::Vector2(Result.x, Result.y);
			}
			Compute::Vector3 IEvent::GetVector3(const std::string& Key) const
			{
				if (!IsValid())
					return Compute::Vector3();

				Rml::Vector3f Result = Base->GetParameter<Rml::Vector3f>(Key, Rml::Vector3f());
				return Compute::Vector3(Result.x, Result.y, Result.z);
			}
			Compute::Vector4 IEvent::GetVector4(const std::string& Key) const
			{
				if (!IsValid())
					return Compute::Vector4();

				Rml::Vector4f Result = Base->GetParameter<Rml::Vector4f>(Key, Rml::Vector4f());
				return Compute::Vector4(Result.x, Result.y, Result.z, Result.w);
			}
			void* IEvent::GetPointer(const std::string& Key) const
			{
				if (!IsValid())
					return nullptr;

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

			IElement::IElement(Rml::Element* Ref) : Base(Ref)
			{
			}
			void IElement::Release()
			{
				delete Base;
				Base = nullptr;
			}
			IElement IElement::Clone() const
			{
				if (!IsValid())
					return nullptr;

				Rml::ElementPtr Ptr = Base->Clone();
				Rml::Element* Result = Ptr.get();
				Ptr.reset();

				return Result;
			}
			void IElement::SetClass(const std::string& ClassName, bool Activate)
			{
				if (IsValid())
					Base->SetClass(ClassName, Activate);
			}
			bool IElement::IsClassSet(const std::string& ClassName) const
			{
				if (!IsValid())
					return false;

				return Base->IsClassSet(ClassName);
			}
			void IElement::SetClassNames(const std::string& ClassNames)
			{
				if (IsValid())
					Base->SetClassNames(ClassNames);
			}
			std::string IElement::GetClassNames() const
			{
				if (!IsValid())
					return "";

				return Base->GetClassNames();
			}
			std::string IElement::GetAddress(bool IncludePseudoClasses, bool IncludeParents) const
			{
				if (!IsValid())
					return "";

				return Base->GetAddress(IncludePseudoClasses, IncludeParents);
			}
			void IElement::SetOffset(const Compute::Vector2& Offset, const IElement& OffsetParent, bool OffsetFixed)
			{
				if (IsValid())
					Base->SetOffset(Rml::Vector2f(Offset.X, Offset.Y), OffsetParent.GetElement(), OffsetFixed);
			}
			Compute::Vector2 IElement::GetRelativeOffset(Area Type)
			{
				if (!IsValid())
					return Compute::Vector2();

				Rml::Vector2f Result = Base->GetRelativeOffset((Rml::Box::Area)Type);
				return Compute::Vector2(Result.x, Result.y);
			}
			Compute::Vector2 IElement::GetAbsoluteOffset(Area Type)
			{
				if (!IsValid())
					return Compute::Vector2();

				Rml::Vector2f Result = Base->GetAbsoluteOffset((Rml::Box::Area)Type);
				return Compute::Vector2(Result.x, Result.y);
			}
			void IElement::SetClientArea(Area ClientArea)
			{
				if (IsValid())
					Base->SetClientArea((Rml::Box::Area)ClientArea);
			}
			Area IElement::GetClientArea() const
			{
				if (!IsValid())
					return Area_Content;

				return (Area)Base->GetClientArea();
			}
			void IElement::SetContentBox(const Compute::Vector2& ContentOffset, const Compute::Vector2& ContentBox)
			{
				if (IsValid())
					Base->SetContentBox(Rml::Vector2f(ContentOffset.X, ContentOffset.Y), Rml::Vector2f(ContentBox.X, ContentBox.Y));
			}
			float IElement::GetBaseline() const
			{
				if (!IsValid())
					return 0.0f;

				return Base->GetBaseline();
			}
			bool IElement::GetIntrinsicDimensions(Compute::Vector2& Dimensions, float& Ratio)
			{
				if (!IsValid())
					return false;

				Rml::Vector2f Size;
				bool Result = Base->GetIntrinsicDimensions(Size, Ratio);
				Dimensions = Compute::Vector2(Size.x, Size.y);

				return Result;
			}
			bool IElement::IsPointWithinElement(const Compute::Vector2& Point)
			{
				if (!IsValid())
					return false;

				return Base->IsPointWithinElement(Rml::Vector2f(Point.X, Point.Y));
			}
			bool IElement::IsVisible() const
			{
				if (!IsValid())
					return false;

				return Base->IsVisible();
			}
			float IElement::GetZIndex() const
			{
				if (!IsValid())
					return 0.0f;

				return Base->GetZIndex();
			}
			bool IElement::SetProperty(const std::string& Name, const std::string& Value)
			{
				if (!IsValid())
					return false;

				return Base->SetProperty(Name, Value);
			}
			void IElement::RemoveProperty(const std::string& Name)
			{
				if (IsValid())
					Base->RemoveProperty(Name);
			}
			std::string IElement::GetProperty(const std::string& Name)
			{
				if (!IsValid())
					return "";

				const Rml::Property* Result = Base->GetProperty(Name);
				if (!Result)
					return "";

				return Result->ToString();
			}
			std::string IElement::GetLocalProperty(const std::string& Name)
			{
				if (!IsValid())
					return "";

				const Rml::Property* Result = Base->GetLocalProperty(Name);
				if (!Result)
					return "";

				return Result->ToString();
			}
			float IElement::ResolveNumericProperty(const std::string& PropertyName)
			{
				if (!IsValid())
					return false;

				return Base->ResolveNumericProperty(PropertyName);
			}
			Compute::Vector2 IElement::GetContainingBlock()
			{
				if (!IsValid())
					return Compute::Vector2();

				Rml::Vector2f Result = Base->GetContainingBlock();
				return Compute::Vector2(Result.x, Result.y);
			}
			Position IElement::GetPosition()
			{
				if (!IsValid())
					return Position_Static;

				return (Position)Base->GetPosition();
			}
			Float IElement::GetFloat()
			{
				if (!IsValid())
					return Float_None;

				return (Float)Base->GetFloat();
			}
			Display IElement::GetDisplay()
			{
				if (!IsValid())
					return Display_None;

				return (Display)Base->GetDisplay();
			}
			float IElement::GetLineHeight()
			{
				if (!IsValid())
					return 0.0f;

				return Base->GetLineHeight();
			}
			bool IElement::Project(Compute::Vector2& Point) const noexcept
			{
				if (!IsValid())
					return false;

				Rml::Vector2f Offset(Point.X, Point.Y);
				bool Result = Base->Project(Offset);
				Point = Compute::Vector2(Offset.x, Offset.y);

				return Result;
			}
			bool IElement::Animate(const std::string& PropertyName, const std::string& TargetValue, float Duration, TimingFunc Func, TimingDir Dir, int NumIterations, bool AlternateDirection, float Delay)
			{
				if (!IsValid())
					return false;

				return Base->Animate(PropertyName, Rml::Property(TargetValue, Rml::Property::TRANSFORM), Duration, Rml::Tween((Rml::Tween::Type)Func, (Rml::Tween::Direction)Dir), NumIterations, AlternateDirection, Delay);
			}
			bool IElement::AddAnimationKey(const std::string& PropertyName, const std::string& TargetValue, float Duration, TimingFunc Func, TimingDir Dir)
			{
				if (!IsValid())
					return false;

				return Base->AddAnimationKey(PropertyName, Rml::Property(TargetValue, Rml::Property::TRANSFORM), Duration, Rml::Tween((Rml::Tween::Type)Func, (Rml::Tween::Direction)Dir));
			}
			void IElement::SetPseudoClass(const std::string& PseudoClass, bool Activate)
			{
				if (IsValid())
					Base->SetPseudoClass(PseudoClass, Activate);
			}
			bool IElement::IsPseudoClassSet(const std::string& PseudoClass) const
			{
				if (!IsValid())
					return false;

				return Base->IsPseudoClassSet(PseudoClass);
			}
			void IElement::SetAttribute(const std::string& Name, const std::string& Value)
			{
				if (IsValid())
					Base->SetAttribute(Name, Value);
			}
			std::string IElement::GetAttribute(const std::string& Name)
			{
				if (!IsValid())
					return "";

				return Base->GetAttribute<std::string>(Name, "");
			}
			bool IElement::HasAttribute(const std::string& Name) const
			{
				if (!IsValid())
					return "";

				return Base->HasAttribute(Name);
			}
			void IElement::RemoveAttribute(const std::string& Name)
			{
				if (IsValid())
					Base->RemoveAttribute(Name);
			}
			IElement IElement::GetFocusLeafNode()
			{
				if (!IsValid())
					return nullptr;

				return Base->GetFocusLeafNode();
			}
			std::string IElement::GetTagName() const
			{
				if (!IsValid())
					return "";

				return Base->GetTagName();
			}
			std::string IElement::GetId() const
			{
				if (!IsValid())
					return "";

				return Base->GetId();
			}
			void IElement::SetId(const std::string& Id)
			{
				if (IsValid())
					Base->SetId(Id);
			}
			float IElement::GetAbsoluteLeft()
			{
				if (!IsValid())
					return 0.0f;

				return Base->GetAbsoluteLeft();
			}
			float IElement::GetAbsoluteTop()
			{
				if (!IsValid())
					return 0.0f;

				return Base->GetAbsoluteTop();
			}
			float IElement::GetClientLeft()
			{
				if (!IsValid())
					return 0.0f;

				return Base->GetClientLeft();
			}
			float IElement::GetClientTop()
			{
				if (!IsValid())
					return 0.0f;

				return Base->GetClientTop();
			}
			float IElement::GetClientWidth()
			{
				if (!IsValid())
					return 0.0f;

				return Base->GetClientWidth();
			}
			float IElement::GetClientHeight()
			{
				if (!IsValid())
					return 0.0f;

				return Base->GetClientHeight();
			}
			IElement IElement::GetOffsetParent()
			{
				if (!IsValid())
					return nullptr;

				return Base->GetOffsetParent();
			}
			float IElement::GetOffsetLeft()
			{
				if (!IsValid())
					return 0.0f;

				return Base->GetOffsetLeft();
			}
			float IElement::GetOffsetTop()
			{
				if (!IsValid())
					return 0.0f;

				return Base->GetOffsetTop();
			}
			float IElement::GetOffsetWidth()
			{
				if (!IsValid())
					return 0.0f;

				return Base->GetOffsetWidth();
			}
			float IElement::GetOffsetHeight()
			{
				if (!IsValid())
					return 0.0f;

				return Base->GetOffsetHeight();
			}
			float IElement::GetScrollLeft()
			{
				if (!IsValid())
					return 0.0f;

				return Base->GetScrollLeft();
			}
			void IElement::SetScrollLeft(float ScrollLeft)
			{
				if (IsValid())
					Base->SetScrollLeft(ScrollLeft);
			}
			float IElement::GetScrollTop()
			{
				if (!IsValid())
					return 0.0f;

				return Base->GetScrollTop();
			}
			void IElement::SetScrollTop(float ScrollTop)
			{
				if (IsValid())
					Base->SetScrollTop(ScrollTop);
			}
			float IElement::GetScrollWidth()
			{
				if (!IsValid())
					return 0.0f;

				return Base->GetScrollWidth();
			}
			float IElement::GetScrollHeight()
			{
				if (!IsValid())
					return 0.0f;

				return Base->GetScrollHeight();
			}
			IElementDocument IElement::GetOwnerDocument() const
			{
				if (!IsValid())
					return nullptr;

				return Base->GetOwnerDocument();
			}
			IElement IElement::GetParentNode() const
			{
				if (!IsValid())
					return nullptr;

				return Base->GetParentNode();
			}
			IElement IElement::GetNextSibling() const
			{
				if (!IsValid())
					return nullptr;

				return Base->GetNextSibling();
			}
			IElement IElement::GetPreviousSibling() const
			{
				if (!IsValid())
					return nullptr;

				return Base->GetPreviousSibling();
			}
			IElement IElement::GetFirstChild() const
			{
				if (!IsValid())
					return nullptr;

				return Base->GetFirstChild();
			}
			IElement IElement::GetLastChild() const
			{
				if (!IsValid())
					return nullptr;

				return Base->GetLastChild();
			}
			IElement IElement::GetChild(int Index) const
			{
				if (!IsValid())
					return nullptr;

				return Base->GetChild(Index);
			}
			int IElement::GetNumChildren(bool IncludeNonDOMElements) const
			{
				if (!IsValid())
					return 0;

				return Base->GetNumChildren(IncludeNonDOMElements);
			}
			void IElement::GetInnerHTML(std::string& Content) const
			{
				if (IsValid())
					Base->GetInnerRML(Content);
			}
			std::string IElement::GetInnerHTML() const
			{
				if (!IsValid())
					return "";
				
				return Base->GetInnerRML();
			}
			void IElement::SetInnerHTML(const std::string& HTML)
			{
				if (IsValid())
					Base->SetInnerRML(HTML);
			}
			bool IElement::IsFocused()
			{
				if (!IsValid())
					return false;

				return Base->IsPseudoClassSet("focus");
			}
			bool IElement::IsHovered()
			{
				if (!IsValid())
					return false;

				return Base->IsPseudoClassSet("hover");
			}
			bool IElement::IsActive()
			{
				if (!IsValid())
					return false;

				return Base->IsPseudoClassSet("active");
			}
			bool IElement::IsChecked()
			{
				if (!IsValid())
					return false;

				return Base->IsPseudoClassSet("checked");
			}
			bool IElement::Focus()
			{
				if (!IsValid())
					return false;

				return Base->Focus();
			}
			void IElement::Blur()
			{
				if (IsValid())
					Base->Blur();
			}
			void IElement::Click()
			{
				if (IsValid())
					Base->Click();
			}
			void IElement::AddEventListener(const std::string& Event, Handler* Listener, bool InCapturePhase)
			{
				if (IsValid() && Listener != nullptr && Listener->Base != nullptr)
					Base->AddEventListener(Event, Listener->Base, InCapturePhase);
			}
			void IElement::RemoveEventListener(const std::string& Event, Handler* Listener, bool InCapturePhase)
			{
				if (IsValid() && Listener != nullptr && Listener->Base != nullptr)
					Base->RemoveEventListener(Event, Listener->Base, InCapturePhase);
			}
			bool IElement::DispatchEvent(const std::string& Type, const Compute::PropertyArgs& Args)
			{
				if (!IsValid())
					return false;

				Rml::Dictionary Props;
				for (auto& Item : Args)
				{
					Rml::Variant& Prop = Props[Item.first];
					FromProperty((Compute::Property*)&Item.second, &Prop);
				}

				return Base->DispatchEvent(Type, Props);
			}
			void IElement::ScrollIntoView(bool AlignWithTop)
			{
				if (IsValid())
					Base->ScrollIntoView(AlignWithTop);
			}
			IElement IElement::AppendChild(const IElement& Element, bool DOMElement)
			{
				if (!IsValid())
					return nullptr;

				return Base->AppendChild(Rml::ElementPtr(Element.GetElement()), DOMElement);
			}
			IElement IElement::InsertBefore(const IElement& Element, const IElement& AdjacentElement)
			{
				if (!IsValid())
					return nullptr;

				return Base->InsertBefore(Rml::ElementPtr(Element.GetElement()), AdjacentElement.GetElement());
			}
			IElement IElement::ReplaceChild(const IElement& InsertedElement, const IElement& ReplacedElement)
			{
				if (!IsValid())
					return nullptr;

				Rml::ElementPtr Ptr = Base->ReplaceChild(Rml::ElementPtr(InsertedElement.GetElement()), ReplacedElement.GetElement());
				Rml::Element* Result = Ptr.get();
				Ptr.reset();

				return Result;
			}
			IElement IElement::RemoveChild(const IElement& Element)
			{
				if (!IsValid())
					return nullptr;

				Rml::ElementPtr Ptr = Base->RemoveChild(Element.GetElement());
				Rml::Element* Result = Ptr.get();
				Ptr.reset();

				return Result;
			}
			bool IElement::HasChildNodes() const
			{
				if (!IsValid())
					return false;

				return Base->HasChildNodes();
			}
			IElement IElement::GetElementById(const std::string& Id)
			{
				if (!IsValid())
					return nullptr;

				return Base->GetElementById(Id);
			}
			IElement IElement::QuerySelector(const std::string& Selector)
			{
				if (!IsValid())
					return nullptr;

				return Base->QuerySelector(Selector);
			}
			std::vector<IElement> IElement::QuerySelectorAll(const std::string& Selectors)
			{
				if (!IsValid())
					return std::vector<IElement>();

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
				if (!IsValid())
					return 0;

				return Base->GetClippingIgnoreDepth();
			}
			bool IElement::IsClippingEnabled()
			{
				if (!IsValid())
					return false;

				return Base->IsClippingEnabled();
			}
			bool IElement::CastFormColor(Compute::Vector4* Ptr, bool Alpha)
			{
				Rml::ElementFormControl* Form = (Rml::ElementFormControl*)Base;
				if (!Form || !Ptr)
					return false;

				std::string Value = Form->GetValue();
				Compute::Vector4 Color = (Alpha ? ToColor4(Value) : ToColor3(Value));

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
						Form->SetValue(FromColor4(*Ptr, true));
					else
						Form->SetValue(FromColor3(*Ptr, true));

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
					Form->SetValue(FromColor4(*Ptr, Value.empty() ? true : Value[0] == '#'));
				else
					Form->SetValue(FromColor3(*Ptr, Value.empty() ? true : Value[0] == '#'));

				return false;
			}
			bool IElement::CastFormString(std::string* Ptr)
			{
				Rml::ElementFormControl* Form = (Rml::ElementFormControl*)Base;
				if (!Form || !Ptr)
					return false;

				std::string Value = Form->GetValue();
				if (Value == *Ptr)
					return false;

				if (Form->IsPseudoClassSet("focus"))
				{
					*Ptr = Value;
					return true;
				}
					
				Form->SetValue(*Ptr);
				return false;
			}
			bool IElement::CastFormInt32(int32_t* Ptr)
			{
				Rml::ElementFormControl* Form = (Rml::ElementFormControl*)Base;
				if (!Form || !Ptr)
					return false;

				Rest::Stroke Value(Form->GetValue());
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
				Rml::ElementFormControl* Form = (Rml::ElementFormControl*)Base;
				if (!Form || !Ptr)
					return false;

				Rest::Stroke Value(Form->GetValue());
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
				if (!IsValid() || !Ptr)
					return false;

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
				Rml::ElementFormControl* Form = (Rml::ElementFormControl*)Base;
				if (!Form || !Ptr)
					return false;

				Rest::Stroke Value(Form->GetValue());
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
				Rml::ElementFormControl* Form = (Rml::ElementFormControl*)Base;
				if (!Form || !Ptr)
					return false;

				Rest::Stroke Value(Form->GetValue());
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
				if (!IsValid() || !Ptr)
					return false;

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
				Rml::ElementFormControl* Form = (Rml::ElementFormControl*)Base;
				if (!Form || !Ptr)
					return false;

				Rest::Stroke Value(Form->GetValue());
				if (Value.Empty())
				{
					if (Form->IsPseudoClassSet("focus"))
						return false;

					Form->SetValue(Rest::Stroke::ToStringAutoPrec(*Ptr));
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

				Form->SetValue(Rest::Stroke::ToStringAutoPrec(*Ptr));
				return false;
			}
			bool IElement::CastFormFloat(float* Ptr, float Mult)
			{
				if (!Ptr)
					return false;

				*Ptr *= Mult;
				bool Result = CastFormFloat(Ptr);
				*Ptr /= Mult;

				return Result;
			}
			bool IElement::CastFormDouble(double* Ptr)
			{
				Rml::ElementFormControl* Form = (Rml::ElementFormControl*)Base;
				if (!Form || !Ptr)
					return false;

				Rest::Stroke Value(Form->GetValue());
				if (Value.Empty())
				{
					if (Form->IsPseudoClassSet("focus"))
						return false;

					Form->SetValue(Rest::Stroke::ToStringAutoPrec(*Ptr));
					return false;
				}

				if (!Value.HasNumber())
				{
					Value.ReplaceNotOf(".-0123456789", "");
					Form->SetValue(Value.R());
				}

				double N = Value.ToFloat64();
				if (N == *Ptr)
					return false;

				if (Form->IsPseudoClassSet("focus"))
				{
					*Ptr = N;
					return true;
				}

				Form->SetValue(Rest::Stroke::ToStringAutoPrec(*Ptr));
				return false;
			}
			bool IElement::CastFormBoolean(bool* Ptr)
			{
				Rml::ElementFormControl* Form = (Rml::ElementFormControl*)Base;
				if (!Form || !Ptr)
					return false;

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
				if (!IsValid())
					return "";

				return ((Rml::ElementFormControl*)Base)->GetName();
			}
			void IElement::SetFormName(const std::string& Name)
			{
				if (!IsValid())
					return;

				((Rml::ElementFormControl*)Base)->SetName(Name);
			}
			std::string IElement::GetFormValue() const
			{
				if (!IsValid())
					return "";

				return ((Rml::ElementFormControl*)Base)->GetValue();
			}
			void IElement::SetFormValue(const std::string& Value)
			{
				if (!IsValid())
					return;

				((Rml::ElementFormControl*)Base)->SetValue(Value);
			}
			bool IElement::IsFormDisabled() const
			{
				if (!IsValid())
					return false;

				return ((Rml::ElementFormControl*)Base)->IsDisabled();
			}
			void IElement::SetFormDisabled(bool Disable)
			{
				if (!IsValid())
					return;

				((Rml::ElementFormControl*)Base)->SetDisabled(Disable);
			}
			Rml::Element* IElement::GetElement() const
			{
				return Base;
			}
			bool IElement::IsValid() const
			{
				return Base != nullptr;
			}

			IElementDocument::IElementDocument(Rml::ElementDocument* Ref) : IElement((Rml::Element*)Ref)
			{
			}
			void IElementDocument::Release()
			{
				delete (Rml::ElementDocument*)Base;
				Base = nullptr;
			}
			void IElementDocument::SetTitle(const std::string& Title)
			{
				if (IsValid())
					((Rml::ElementDocument*)Base)->SetTitle(Title);
			}
			void IElementDocument::PullToFront()
			{
				if (IsValid())
					((Rml::ElementDocument*)Base)->PullToFront();
			}
			void IElementDocument::PushToBack()
			{
				if (IsValid())
					((Rml::ElementDocument*)Base)->PushToBack();
			}
			void IElementDocument::Show(ModalFlag Modal, FocusFlag Focus)
			{
				if (IsValid())
					((Rml::ElementDocument*)Base)->Show((Rml::ModalFlag)Modal, (Rml::FocusFlag)Focus);
			}
			void IElementDocument::Hide()
			{
				if (IsValid())
					((Rml::ElementDocument*)Base)->Hide();
			}
			void IElementDocument::Close()
			{
				if (IsValid())
					((Rml::ElementDocument*)Base)->Close();
			}
			std::string IElementDocument::GetTitle() const
			{
				if (!IsValid())
					return "";

				return ((Rml::ElementDocument*)Base)->GetTitle();
			}
			std::string IElementDocument::GetSourceURL() const
			{
				if (!IsValid())
					return "";

				return ((Rml::ElementDocument*)Base)->GetSourceURL();
			}
			IElement IElementDocument::CreateElement(const std::string& Name)
			{
				if (!IsValid())
					return nullptr;

				Rml::ElementPtr Ptr = ((Rml::ElementDocument*)Base)->CreateElement(Name);
				Rml::Element* Result = Ptr.get();
				Ptr.reset();

				return Result;
			}
			bool IElementDocument::IsModal() const
			{
				if (!IsValid())
					return false;

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
					return State >= 0;

				RenderInterface = new RenderSubsystem();
				Rml::SetRenderInterface(RenderInterface);

				FileInterface = new FileSubsystem();
				Rml::SetFileInterface(FileInterface);

				SystemInterface = new MainSubsystem();
				Rml::SetSystemInterface(SystemInterface);
				
				bool Result = Rml::Initialise();

				ContextFactory = new ContextInstancer();
				Rml::Factory::RegisterContextInstancer(ContextFactory);

				ListenerFactory = new ListenerInstancer();
				Rml::Factory::RegisterEventListenerInstancer(ListenerFactory);

				DocumentFactory = new DocumentInstancer();
				Rml::Factory::RegisterElementInstancer("body", DocumentFactory);

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

				delete SystemInterface;
				SystemInterface = nullptr;
				Rml::SetSystemInterface(nullptr);

				delete FileInterface;
				FileInterface = nullptr;
				Rml::SetFileInterface(nullptr);

				delete RenderInterface;
				RenderInterface = nullptr;
				Rml::SetRenderInterface(nullptr);

				delete DocumentFactory;
				DocumentFactory = nullptr;

				delete ListenerFactory;
				ListenerFactory = nullptr;
				
				delete ContextFactory;
				ContextFactory = nullptr;

				ScriptInterface = nullptr;
				return true;
			}
			void Subsystem::SetMetadata(Graphics::Activity* Activity, ContentManager* Content, Rest::Timer* Time)
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
				if (SystemInterface != nullptr)
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
				if (!RenderInterface)
					return nullptr;

				return RenderInterface->GetDevice();
			}
			Graphics::Texture2D* Subsystem::GetBackground()
			{
				if (!RenderInterface)
					return nullptr;

				return RenderInterface->Background;
			}
			Compute::Matrix4x4* Subsystem::GetTransform()
			{
				return (RenderInterface ? RenderInterface->GetTransform() : nullptr);
			}
			Compute::Matrix4x4* Subsystem::GetProjection()
			{
				return (RenderInterface ? RenderInterface->GetProjection() : nullptr);
			}
			Compute::Matrix4x4 Subsystem::ToMatrix(const void* Matrix)
			{
				Compute::Matrix4x4 Result;
				if (!Matrix)
					return Result;

				const Rml::Matrix4f* NewTransform = (const Rml::Matrix4f*)Matrix;
				auto& Row11 = NewTransform->GetRow(0);
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
				return Rest::Stroke(&Text).Replace("\r\n", "&nbsp;").Replace("\n", "&nbsp;").Replace("<", "&lt;").Replace(">", "&gt;").R();
			}
			std::unordered_map<std::string, DataSource*>* Subsystem::Sources = nullptr;
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

			DataNode::DataNode(DataModel* Model, std::string* TopName, const Compute::Property& Initial) : Handle(Model), Safe(true)
			{
				Ref = new Compute::Property(Initial);
				if (TopName != nullptr)
					Name = new std::string(*TopName);
			}
			DataNode::DataNode(DataModel* Model, std::string* TopName, Compute::Property* Reference) : Handle(Model), Safe(false), Ref(Reference)
			{
				if (TopName != nullptr)
					Name = new std::string(*TopName);
			}
			DataNode::DataNode(const DataNode& Other) : Childs(Other.Childs), Handle(Other.Handle), Safe(Other.Safe)
			{
				if (Safe)
					Ref = new Compute::Property(*Other.Ref);
				else
					Ref = Other.Ref;

				if (Other.Name != nullptr)
					Name = new std::string(*Other.Name);
			}
			DataNode::~DataNode()
			{
				if (Safe)
					delete Ref;

				delete Name;
			}
			DataNode& DataNode::Add(const Compute::PropertyList& Initial)
			{
				Childs.push_back(DataNode(Handle, Name, Compute::Property()));
				if (Handle != nullptr && Name != nullptr)
					Handle->Change(*Name);
				
				DataNode& Result = Childs.back();
				for (auto& Item : Initial)
					Result.Add(Item);

				return Result;
			}
			DataNode& DataNode::Add(const Compute::Property& Initial)
			{
				Childs.push_back(DataNode(Handle, Name, Initial));
				if (Handle != nullptr && Name != nullptr)
					Handle->Change(*Name);

				return Childs.back();
			}
			DataNode& DataNode::Add(Compute::Property* Reference)
			{
				Childs.push_back(DataNode(Handle, Name, Reference));
				if (Handle != nullptr && Name != nullptr)
					Handle->Change(*Name);

				return Childs.back();
			}
			DataNode& DataNode::At(size_t Index)
			{
				if (Index >= Childs.size())
					return *this;

				return Childs[Index];
			}
			size_t DataNode::GetSize()
			{
				return Childs.size();
			}
			bool DataNode::Remove(size_t Index)
			{
				if (Index >= Childs.size())
					return false;

				Childs.erase(Childs.begin() + Index);
				return true;
			}
			bool DataNode::Clear()
			{
				if (Childs.empty())
					return false;

				Childs.clear();
				return true;
			}
			void DataNode::Set(const Compute::Property& NewValue)
			{
				if (*Ref == NewValue)
					return;

				*Ref = NewValue;
				if (Handle != nullptr && Name != nullptr)
					Handle->Change(*Name);
			}
			void DataNode::Set(Compute::Property* NewReference)
			{
				if (!NewReference || NewReference == Ref)
					return;

				if (Safe)
					delete Ref;

				Ref = NewReference;
				Safe = false;

				if (Handle != nullptr && Name != nullptr)
					Handle->Change(*Name);
			}
			void DataNode::SetString(const std::string& Value)
			{
				Set(Compute::Property(Value));
			}
			void DataNode::SetVector2(const Compute::Vector2& Value)
			{
				Set(Compute::Property(Value));
			}
			void DataNode::SetVector3(const Compute::Vector3& Value)
			{
				Set(Compute::Property(Value));
			}
			void DataNode::SetVector4(const Compute::Vector4& Value)
			{
				Set(Compute::Property(Value));
			}
			void DataNode::SetInteger(int64_t Value)
			{
				Set(Compute::Property(Value));
			}
			void DataNode::SetFloat(float Value)
			{
				Set(Compute::Property(Value));
			}
			void DataNode::SetDouble(double Value)
			{
				Set(Compute::Property(Value));
			}
			void DataNode::SetBoolean(bool Value)
			{
				Set(Compute::Property(Value));
			}
			void DataNode::SetPointer(void* Value)
			{
				Set(Compute::Property(Value));
			}
			const Compute::Property& DataNode::Get()
			{
				return *Ref;
			}
			std::string DataNode::GetString()
			{
				return Ref->ToString();
			}
			Compute::Vector2 DataNode::GetVector2()
			{
				return Ref->GetVector2();
			}
			Compute::Vector3 DataNode::GetVector3()
			{
				return Ref->GetVector3();
			}
			Compute::Vector4 DataNode::GetVector4()
			{
				return Ref->GetVector4();
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
				FromProperty(Ref, &Result);
			}
			void DataNode::SetValue(const Rml::Variant& Result)
			{
				ToProperty((Rml::Variant*)&Result, Ref);
			}
			void DataNode::GetValueSize(Rml::Variant& Result)
			{
				Result = (int64_t)GetSize();
			}
			DataNode& DataNode::operator= (const DataNode& Other)
			{
				this->~DataNode();
				if (Safe)
					Ref = new Compute::Property(*Other.Ref);
				else
					Ref = Other.Ref;

				if (Other.Name != nullptr)
					Name = new std::string(*Other.Name);

				return *this;
			}

			DataRow::DataRow(DataSource* NewBase, void* NewTarget) : Base(NewBase), Parent(nullptr), Target(NewTarget), Depth(0), Name("root")
			{
				Base->Nodes[Name] = this;
			}
			DataRow::DataRow(DataRow* Parent, void* NewTarget) : Base(Parent->Base), Parent(Parent), Target(NewTarget), Depth(Parent->Depth + 1), Name(Rest::Form("%p", (void*)this).R())
			{
				Parent->Childs.push_back(this);
				Base->Nodes[Name] = this;

				if (Base->OnChange)
					Base->OnChange(this);
			}
			DataRow::~DataRow()
			{
				auto T = Base->Nodes.find(Name);
				if (T != Base->Nodes.end())
					Base->Nodes.erase(T);

				if (Base->OnDestroy && Target != nullptr)
					Base->OnDestroy(Target);

				for (auto& It : Childs)
					delete It;
			}
			DataRow* DataRow::AddChild(void* Target)
			{
				DataRow* Result = new DataRow(this, Target);
				Base->RowAdd(Name, Childs.size() - 1, 1);

				return Result;
			}
			DataRow* DataRow::GetChild(void* Target)
			{
				for (auto& Child : Childs)
				{
					if (Child->Target == Target)
						return Child;
				}

				return nullptr;
			}
			DataRow* DataRow::GetChildByIndex(size_t Index)
			{
				if (Index >= Childs.size())
					return nullptr;

				return Childs[Index];
			}
			DataRow* DataRow::GetParent()
			{
				return Parent;
			}
			DataRow* DataRow::FindChild(void* Target)
			{
				for (auto& Child : Childs)
				{
					if (Child->Target == Target)
						return Child;

					DataRow* Result = Child->FindChild(Target);
					if (Result != nullptr)
						return Result;
				}

				return nullptr;
			}
			size_t DataRow::GetChildsCount()
			{
				return Childs.size();
			}
			bool DataRow::RemoveChild(void* Target)
			{
				for (size_t i = 0; i < Childs.size(); i++)
				{
					DataRow* Child = Childs[i];
					if (Child->Target != Target)
						continue;

					return RemoveChildByIndex(i);
				}

				return false;
			}
			bool DataRow::RemoveChildByIndex(size_t Index)
			{
				if (Index >= Childs.size())
					return false;

				DataRow* Child = Childs[Index];
				Childs.erase(Childs.begin() + Index);
				delete Child;

				Base->RowRemove(Name, Index, 1);
				return true;
			}
			bool DataRow::RemoveChilds()
			{
				size_t Size = Childs.size();
				for (auto& It : Childs)
					delete It;
				Childs.clear();

				if (Size > 0)
					Base->RowRemove(Name, 0, Size);

				return Size > 0;
			}
			void DataRow::SetTarget(void* New)
			{
				if (Base->OnDestroy && Target != nullptr)
					Base->OnDestroy(Target);

				Target = New;
				Update();
			}
			void DataRow::Update()
			{
				RemoveChilds();
				if (Base->OnChange)
					Base->OnChange(this);

				if (Parent != nullptr)
				{
					for (size_t i = 0; i < Parent->Childs.size(); i++)
					{
						if (Parent->Childs[i] == this)
						{
							Base->RowChange(Parent->Name, i, 1);
							break;
						}
					}
				}
				else
					Base->RowChange(Name);
			}

			DataModel::DataModel(Rml::DataModelConstructor* Ref) : Base(nullptr)
			{
				if (!Ref)
					return;

				Base = new Rml::DataModelConstructor(*Ref);
			}
			DataModel::~DataModel()
			{
				for (auto Item : Props)
					delete Item.second;

				delete Base;
			}
			DataNode* DataModel::SetProperty(const std::string& Name, const Compute::Property& Value)
			{
				if (!IsValid())
					return nullptr;

				DataNode* Result = GetProperty(Name);
				if (Result != nullptr)
				{
					Result->Set(Value);
					return Result;
				}

				Result = new DataNode(this, (std::string*)&Name, Value);
				if (Value.GetType() != Compute::PropertyType_Null)
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

				delete Result;
				return nullptr;
			}
			DataNode* DataModel::SetProperty(const std::string& Name, Compute::Property* Value)
			{
				if (!IsValid() || !Value)
					return nullptr;

				DataNode* Sub = GetProperty(Name);
				if (Sub != nullptr)
				{
					Sub->Set(Value);
					return Sub;
				}

				DataNode* Result = new DataNode(this, (std::string*)&Name, Value);
				if (Base->Bind(Name, Result))
				{
					Props[Name] = Result;
					return Result;
				}

				delete Result;
				return nullptr;
			}
			DataNode* DataModel::SetArray(const std::string& Name)
			{
				return SetProperty(Name, Compute::Property());
			}
			DataNode* DataModel::SetString(const std::string& Name, const std::string& Value)
			{
				return SetProperty(Name, Compute::Property(Value));
			}
			DataNode* DataModel::SetInteger(const std::string& Name, int64_t Value)
			{
				return SetProperty(Name, Compute::Property(Value));
			}
			DataNode* DataModel::SetFloat(const std::string& Name, float Value)
			{
				return SetProperty(Name, Compute::Property(Value));
			}
			DataNode* DataModel::SetDouble(const std::string& Name, double Value)
			{
				return SetProperty(Name, Compute::Property(Value));
			}
			DataNode* DataModel::SetBoolean(const std::string& Name, bool Value)
			{
				return SetProperty(Name, Compute::Property(Value));
			}
			DataNode* DataModel::SetPointer(const std::string& Name, void* Value)
			{
				return SetProperty(Name, Compute::Property(Value));
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

				return Result->Ref->ToString();
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
				if (!IsValid() || !Callback)
					return false;

				return Base->BindEventCallback(Name, [Callback](Rml::DataModelHandle Handle, Rml::Event& Event, const Rml::VariantList& Props)
				{
					Compute::PropertyList Args;
					Args.resize(Props.size());

					size_t i = 0;
					for (auto& Item : Props)
						ToProperty((Rml::Variant*)&Item, &Args[i++]);

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
				if (IsValid())
					Base->GetModelHandle().DirtyVariable(VariableName);
			}
			bool DataModel::HasChanged(const std::string& VariableName) const
			{
				if (!IsValid())
					return false;

				return Base->GetModelHandle().IsVariableDirty(VariableName);
			}
			bool DataModel::IsValid() const
			{
				return Base != nullptr;
			}

			DataSource::DataSource(const std::string& NewName) : Name(NewName), DS(nullptr), DF(nullptr), Root(new DataRow(this, nullptr))
			{
				if (Name.empty())
					return;

				if (!DS)
					DS = new DataSourceSubsystem(this);

				if (!DF)
					DF = new DataFormatterSubsystem(this);
			}
			DataSource::~DataSource()
			{
				delete DS;
				delete DF;
				delete Root;
			}
			void DataSource::SetFormatCallback(const FormatCallback& Callback)
			{
				OnFormat = Callback;
			}
			void DataSource::SetColumnCallback(const ColumnCallback& Callback)
			{
				OnColumn = Callback;
			}
			void DataSource::SetChangeCallback(const ChangeCallback& Callback)
			{
				OnChange = Callback;
			}
			void DataSource::SetDestroyCallback(const DestroyCallback& Callback)
			{
				OnDestroy = Callback;
			}
			void DataSource::RowAdd(const std::string& Table, int FirstRowAdded, int NumRowsAdded)
			{
				if (DS != nullptr)
					DS->NotifyRowAdd(Table, FirstRowAdded, NumRowsAdded);
			}
			void DataSource::RowRemove(const std::string& Table, int FirstRowRemoved, int NumRowsRemoved)
			{
				if (DS != nullptr)
					DS->NotifyRowRemove(Table, FirstRowRemoved, NumRowsRemoved);
			}
			void DataSource::RowChange(const std::string& Table, int FirstRowChanged, int NumRowsChanged)
			{
				if (DS != nullptr)
					DS->NotifyRowChange(Table, FirstRowChanged, NumRowsChanged);
			}
			void DataSource::RowChange(const std::string& Table)
			{
				if (DS != nullptr)
					DS->NotifyRowChange(Table);
			}
			void DataSource::SetTarget(void* OldTarget, void* NewTarget)
			{
				if (Root->Target != OldTarget)
				{
					DataRow* Result = Root->FindChild(OldTarget);
					if (Result != nullptr)
						Result->SetTarget(NewTarget);
				}
				else
					Root->SetTarget(NewTarget);
			}
			void DataSource::Update(void* Target)
			{
				if (Root->Target != Target)
				{
					DataRow* Result = Root->FindChild(Target);
					if (Result != nullptr)
						Result->Update();
				}
				else
					Root->Update();
			}
			DataRow* DataSource::Get()
			{
				return Root;
			}

			Handler::Handler(const EventCallback& NewCallback)
			{
				Base = new EventSubsystem(NewCallback);
			}
			Handler::Handler(const std::string& FunctionName)
			{
				Base = new ListenerSubsystem(FunctionName, nullptr, false);
			}
			Handler::~Handler()
			{
				Base->OnDetach(nullptr);
			}

			Context::Context(const Compute::Vector2& Size) : Loading(false), Compiler(nullptr), Cursor(-1.0f)
			{
				Base = (ScopedContext*)Rml::CreateContext(std::to_string(Subsystem::Id++), Rml::Vector2i(Size.X, Size.Y));
				Base->Basis = this; CreateVM();
			}
			Context::Context(Graphics::GraphicsDevice* Device) : Loading(false), Compiler(nullptr), Cursor(-1.0f)
			{
				if (Device != nullptr)
				{
					Graphics::RenderTarget2D* Target = Device->GetRenderTarget();
					if (Target != nullptr)
					{
						Base = (ScopedContext*)Rml::CreateContext(std::to_string(Subsystem::Id++), Rml::Vector2i((int)Target->GetWidth(), (int)Target->GetHeight()));
						Base->Basis = this; CreateVM();
						return;
					}
				}

				Base = (ScopedContext*)Rml::CreateContext(std::to_string(Subsystem::Id++), Rml::Vector2i(512, 512));
				Base->Basis = this; CreateVM();
			}
			Context::~Context()
			{
				RemoveDataModels();
				for (auto Item : Sources)
				{
					if (Item.second->GetRefCount() <= 1 && Subsystem::Sources != nullptr)
					{
						auto It = Subsystem::Sources->find(Item.second->Name);
						if (It != Subsystem::Sources->end())
							Subsystem::Sources->erase(It);

						if (Subsystem::Sources->empty())
						{
							delete Subsystem::Sources;
							Subsystem::Sources = nullptr;
						}
					}

					delete Item.second;
				}

				Rml::RemoveContext(Base->GetName());
				TH_RELEASE(Compiler);
			}
			void Context::EmitKey(Graphics::KeyCode Key, Graphics::KeyMod Mod, int Virtual, int Repeat, bool Pressed)
			{
				if (Key == Graphics::KeyCode_CURSORLEFT)
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
				else if (Key == Graphics::KeyCode_CURSORRIGHT)
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
				else if (Key == Graphics::KeyCode_CURSORMIDDLE)
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
				if (Buffer != nullptr && Length > 0 && Base->ProcessTextInput(std::string(Buffer, Length)))
					Inputs.Text = true;
			}
			void Context::EmitWheel(int X, int Y, bool Normal, Graphics::KeyMod Mod)
			{
				if (Base->ProcessMouseWheel(-Y, GetKeyMod(Mod)))
					Inputs.Scroll = true;
			}
			void Context::EmitResize(int Width, int Height)
			{
				Base->SetDimensions(Rml::Vector2i(Width, Height));
				Subsystem::ResizeDecorators();
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

				for (auto& Item : Models)
					Item.second->Base->GetModelHandle().Update();

				Base->Update();
			}
			void Context::RenderLists(Graphics::Texture2D* Target)
			{
				RenderSubsystem* Renderer = Subsystem::GetRenderInterface();
				if (!Renderer)
					return;

				Renderer->Background = Target;
				Base->Render();
			}
			void Context::ClearCache()
			{
				Rml::StyleSheetFactory::ClearStyleSheetCache();
			}
			IElementDocument Context::Construct(const std::string& Path)
			{
				bool State = Loading;
				Loading = true;

				if (OnMount)
					OnMount(this);

				ClearVM();
				Elements.clear();

				auto* Result = Base->LoadDocument(Path);
				
				Loading = State;
				return Result;
			}
			bool Context::Deconstruct()
			{
				bool State = Loading;
				Loading = true;

				Elements.clear();
				RemoveDataModels();
				Base->UnloadAllDocuments();
				Loading = State;

				return true;
			}
			bool Context::Inject(Rest::Document* Conf, const std::string& Relative)
			{
				if (!Conf)
					return false;

				bool State = Loading;
				Loading = true;

				auto FontFaces = Conf->FindCollection("font-face", true);
				for (auto* Face : FontFaces)
				{
					Rest::Document* IPath = Face->GetAttribute("path");
					if (!IPath)
					{
						TH_ERROR("path is required for font face");
						return false;
					}

					std::string Path = IPath->Serialize();
					std::string Target = Rest::OS::Resolve(Path, Relative);

					if (!AddFontFace(Target.empty() ? Path : Target, Face->GetAttribute("fallback") != nullptr))
					{
						Loading = State;
						return false;
					}
				}

				auto Documents = Conf->FindCollection("document", true);
				for (auto* Document : Documents)
				{
					Rest::Document* IPath = Document->GetAttribute("path");
					if (!IPath)
					{
						TH_ERROR("path is required for document");
						return false;
					}

					std::string Path = IPath->Serialize();
					std::string Target = Rest::OS::Resolve(Path, Relative);
					IElementDocument Result = Construct(Target.empty() ? Path : Target);

					if (!Result.IsValid())
					{
						Loading = State;
						return false;
					}

					if (Document->GetAttribute("show") != nullptr)
						Result.Show();
				}

				Loading = State;
				return true;
			}
			bool Context::Inject(const std::string& ConfPath)
			{
				if (!Subsystem::RenderInterface || !Subsystem::RenderInterface->GetContent())
					return false;

				bool State = Loading;
				Loading = true;

				Rest::Document* Sheet = Subsystem::RenderInterface->GetContent()->Load<Rest::Document>(ConfPath);
				if (!Sheet)
				{
					Loading = State;
					return false;
				}

				bool Result = Inject(Sheet, Rest::OS::FileDirectory(ConfPath));
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
			void Context::PullDocumentToFront(const IElementDocument& Document)
			{
				return Base->PullDocumentToFront(Document.GetElementDocument());
			}
			void Context::PushDocumentToBack(const IElementDocument& Document)
			{
				return Base->PushDocumentToBack(Document.GetElementDocument());
			}
			void Context::UnfocusDocument(const IElementDocument& Document)
			{
				return Base->UnfocusDocument(Document.GetElementDocument());
			}
			void Context::AddEventListener(const std::string& Event, Handler* Listener, bool InCapturePhase)
			{
				if (Listener != nullptr && Listener->Base != nullptr)
					Base->AddEventListener(Event, Listener->Base, InCapturePhase);
			}
			void Context::RemoveEventListener(const std::string& Event, Handler* Listener, bool InCapturePhase)
			{
				if (Listener != nullptr && Listener->Base != nullptr)
					Base->RemoveEventListener(Event, Listener->Base, InCapturePhase);
			}
			bool Context::IsMouseInteracting() const
			{
				return Base->IsMouseInteracting();
			}
			bool Context::WasInputUsed(uint32_t InputTypeMask)
			{
				bool Result = false;
				if (InputTypeMask & InputType_Keys && Inputs.Keys)
					Result = true;

				if (InputTypeMask & InputType_Scroll && Inputs.Scroll)
					Result = true;

				if (InputTypeMask & InputType_Text && Inputs.Text)
					Result = true;

				if (InputTypeMask & InputType_Cursor && Inputs.Cursor)
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
					Type.RegisterMember("at", &DataNode::Childs);
					Type.RegisterMemberFunc("size", &DataNode::GetValueSize);
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
			DataSource* Context::SetDataSource(const std::string& Name)
			{
				auto It = Sources.find(Name);
				if (It != Sources.end())
					return It->second;

				if (!Subsystem::Sources)
				{
					DataSource* Source = new DataSource(Name);
					Subsystem::Sources = new std::unordered_map<std::string, DataSource*>();
					(*Subsystem::Sources)[Name] = Source;
					Sources[Name] = Source;

					return Source;
				}

				It = Subsystem::Sources->find(Name);
				if (It != Subsystem::Sources->end())
				{
					Sources[Name] = It->second;
					It->second->AddRef();

					return It->second;
				}

				DataSource* Source = new DataSource(Name);
				(*Subsystem::Sources)[Name] = Source;
				Sources[Name] = Source;

				return Source;
			}
			DataSource* Context::GetDataSource(const std::string& Name)
			{
				auto It = Sources.find(Name);
				if (It != Sources.end())
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

					delete It->second;
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
					delete Item.second;
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
			void Context::CreateVM()
			{
				if (!Subsystem::ScriptInterface)
					return;

				if (Compiler != nullptr)
					return;

				Compiler = Subsystem::ScriptInterface->CreateCompiler();
				ClearVM();
			}
			void Context::ClearVM()
			{
				if (!Compiler)
					return;

				Compiler->Clear();
				Compiler->PrepareScope("__scope__");
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
					case Graphics::KeyCode_SPACE:
						return KI_SPACE;
					case Graphics::KeyCode_0:
						return KI_0;
					case Graphics::KeyCode_1:
						return KI_1;
					case Graphics::KeyCode_2:
						return KI_2;
					case Graphics::KeyCode_3:
						return KI_3;
					case Graphics::KeyCode_4:
						return KI_4;
					case Graphics::KeyCode_5:
						return KI_5;
					case Graphics::KeyCode_6:
						return KI_6;
					case Graphics::KeyCode_7:
						return KI_7;
					case Graphics::KeyCode_8:
						return KI_8;
					case Graphics::KeyCode_9:
						return KI_9;
					case Graphics::KeyCode_A:
						return KI_A;
					case Graphics::KeyCode_B:
						return KI_B;
					case Graphics::KeyCode_C:
						return KI_C;
					case Graphics::KeyCode_D:
						return KI_D;
					case Graphics::KeyCode_E:
						return KI_E;
					case Graphics::KeyCode_F:
						return KI_F;
					case Graphics::KeyCode_G:
						return KI_G;
					case Graphics::KeyCode_H:
						return KI_H;
					case Graphics::KeyCode_I:
						return KI_I;
					case Graphics::KeyCode_J:
						return KI_J;
					case Graphics::KeyCode_K:
						return KI_K;
					case Graphics::KeyCode_L:
						return KI_L;
					case Graphics::KeyCode_M:
						return KI_M;
					case Graphics::KeyCode_N:
						return KI_N;
					case Graphics::KeyCode_O:
						return KI_O;
					case Graphics::KeyCode_P:
						return KI_P;
					case Graphics::KeyCode_Q:
						return KI_Q;
					case Graphics::KeyCode_R:
						return KI_R;
					case Graphics::KeyCode_S:
						return KI_S;
					case Graphics::KeyCode_T:
						return KI_T;
					case Graphics::KeyCode_U:
						return KI_U;
					case Graphics::KeyCode_V:
						return KI_V;
					case Graphics::KeyCode_W:
						return KI_W;
					case Graphics::KeyCode_X:
						return KI_X;
					case Graphics::KeyCode_Y:
						return KI_Y;
					case Graphics::KeyCode_Z:
						return KI_Z;
					case Graphics::KeyCode_SEMICOLON:
						return KI_OEM_1;
					case Graphics::KeyCode_COMMA:
						return KI_OEM_COMMA;
					case Graphics::KeyCode_MINUS:
						return KI_OEM_MINUS;
					case Graphics::KeyCode_PERIOD:
						return KI_OEM_PERIOD;
					case Graphics::KeyCode_SLASH:
						return KI_OEM_2;
					case Graphics::KeyCode_LEFTBRACKET:
						return KI_OEM_4;
					case Graphics::KeyCode_BACKSLASH:
						return KI_OEM_5;
					case Graphics::KeyCode_RIGHTBRACKET:
						return KI_OEM_6;
					case Graphics::KeyCode_KP_0:
						return KI_NUMPAD0;
					case Graphics::KeyCode_KP_1:
						return KI_NUMPAD1;
					case Graphics::KeyCode_KP_2:
						return KI_NUMPAD2;
					case Graphics::KeyCode_KP_3:
						return KI_NUMPAD3;
					case Graphics::KeyCode_KP_4:
						return KI_NUMPAD4;
					case Graphics::KeyCode_KP_5:
						return KI_NUMPAD5;
					case Graphics::KeyCode_KP_6:
						return KI_NUMPAD6;
					case Graphics::KeyCode_KP_7:
						return KI_NUMPAD7;
					case Graphics::KeyCode_KP_8:
						return KI_NUMPAD8;
					case Graphics::KeyCode_KP_9:
						return KI_NUMPAD9;
					case Graphics::KeyCode_KP_ENTER:
						return KI_NUMPADENTER;
					case Graphics::KeyCode_KP_MULTIPLY:
						return KI_MULTIPLY;
					case Graphics::KeyCode_KP_PLUS:
						return KI_ADD;
					case Graphics::KeyCode_KP_MINUS:
						return KI_SUBTRACT;
					case Graphics::KeyCode_KP_PERIOD:
						return KI_DECIMAL;
					case Graphics::KeyCode_KP_DIVIDE:
						return KI_DIVIDE;
					case Graphics::KeyCode_KP_EQUALS:
						return KI_OEM_NEC_EQUAL;
					case Graphics::KeyCode_BACKSPACE:
						return KI_BACK;
					case Graphics::KeyCode_TAB:
						return KI_TAB;
					case Graphics::KeyCode_CLEAR:
						return KI_CLEAR;
					case Graphics::KeyCode_RETURN:
						return KI_RETURN;
					case Graphics::KeyCode_PAUSE:
						return KI_PAUSE;
					case Graphics::KeyCode_CAPSLOCK:
						return KI_CAPITAL;
					case Graphics::KeyCode_PAGEUP:
						return KI_PRIOR;
					case Graphics::KeyCode_PAGEDOWN:
						return KI_NEXT;
					case Graphics::KeyCode_END:
						return KI_END;
					case Graphics::KeyCode_HOME:
						return KI_HOME;
					case Graphics::KeyCode_LEFT:
						return KI_LEFT;
					case Graphics::KeyCode_UP:
						return KI_UP;
					case Graphics::KeyCode_RIGHT:
						return KI_RIGHT;
					case Graphics::KeyCode_DOWN:
						return KI_DOWN;
					case Graphics::KeyCode_INSERT:
						return KI_INSERT;
					case Graphics::KeyCode_DELETE:
						return KI_DELETE;
					case Graphics::KeyCode_HELP:
						return KI_HELP;
					case Graphics::KeyCode_F1:
						return KI_F1;
					case Graphics::KeyCode_F2:
						return KI_F2;
					case Graphics::KeyCode_F3:
						return KI_F3;
					case Graphics::KeyCode_F4:
						return KI_F4;
					case Graphics::KeyCode_F5:
						return KI_F5;
					case Graphics::KeyCode_F6:
						return KI_F6;
					case Graphics::KeyCode_F7:
						return KI_F7;
					case Graphics::KeyCode_F8:
						return KI_F8;
					case Graphics::KeyCode_F9:
						return KI_F9;
					case Graphics::KeyCode_F10:
						return KI_F10;
					case Graphics::KeyCode_F11:
						return KI_F11;
					case Graphics::KeyCode_F12:
						return KI_F12;
					case Graphics::KeyCode_F13:
						return KI_F13;
					case Graphics::KeyCode_F14:
						return KI_F14;
					case Graphics::KeyCode_F15:
						return KI_F15;
					case Graphics::KeyCode_NUMLOCKCLEAR:
						return KI_NUMLOCK;
					case Graphics::KeyCode_SCROLLLOCK:
						return KI_SCROLL;
					case Graphics::KeyCode_LSHIFT:
						return KI_LSHIFT;
					case Graphics::KeyCode_RSHIFT:
						return KI_RSHIFT;
					case Graphics::KeyCode_LCTRL:
						return KI_LCONTROL;
					case Graphics::KeyCode_RCTRL:
						return KI_RCONTROL;
					case Graphics::KeyCode_LALT:
						return KI_LMENU;
					case Graphics::KeyCode_RALT:
						return KI_RMENU;
					case Graphics::KeyCode_LGUI:
						return KI_LMETA;
					case Graphics::KeyCode_RGUI:
						return KI_RMETA;
					default:
						return KI_UNKNOWN;
				}
			}
			int Context::GetKeyMod(Graphics::KeyMod Mod)
			{
				int Result = 0;
				if (Mod & Graphics::KeyMod_CTRL)
					Result |= Rml::Input::KM_CTRL;

				if (Mod & Graphics::KeyMod_SHIFT)
					Result |= Rml::Input::KM_SHIFT;

				if (Mod & Graphics::KeyMod_ALT)
					Result |= Rml::Input::KM_ALT;

				return Result;
			}
		}
	}
}