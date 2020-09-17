#include "graphics.h"
#include "../graphics/d3d11.h"
#include "../graphics/ogl.h"
#include "../core/shaders.h"
#ifdef THAWK_HAS_SDL2
#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#endif

namespace Tomahawk
{
	namespace Graphics
	{
		Alert::Alert(Activity* From) : View(AlertType_None), Base(From), Waiting(false)
		{
		}
		void Alert::Setup(AlertType Type, const std::string& Title, const std::string& Text)
		{
			if (View != AlertType_None)
				return;

			View = Type;
			Name = Title;
			Data = Text;
			Buttons.clear();
		}
		void Alert::Button(AlertConfirm Confirm, const std::string& Text, int Id)
		{
			if (View == AlertType_None || Buttons.size() >= 16)
				return;

			for (auto It = Buttons.begin(); It != Buttons.end(); It++)
			{
				if (It->Id == Id)
					return;
			}

			Element Button;
			Button.Name = Text;
			Button.Id = Id;
			Button.Flags = (int)Confirm;

			Buttons.push_back(Button);
		}
		void Alert::Result(const std::function<void(int)>& Callback)
		{
			if (View != AlertType_None)
			{
				Done = Callback;
				Waiting = true;
			}
		}
		void Alert::Dispatch()
		{
#ifdef THAWK_HAS_SDL2
			if (!Waiting || View == AlertType_None)
				return;

			SDL_MessageBoxButtonData Views[16];
			for (uint64_t i = 0; i < Buttons.size(); i++)
			{
				SDL_MessageBoxButtonData* To = Views + i;
				auto From = Buttons.begin() + i;
				To->text = From->Name.c_str();
				To->buttonid = From->Id;
				To->flags = From->Flags;
			}

			SDL_MessageBoxData AlertData;
			AlertData.title = Name.c_str();
			AlertData.message = Data.c_str();
			AlertData.flags = (SDL_MessageBoxFlags)View;
			AlertData.numbuttons = (int)Buttons.size();
			AlertData.buttons = Views;
			AlertData.window = Base->GetHandle();

			int Id = 0;
			View = AlertType_None;
			Waiting = false;
			int Rd = SDL_ShowMessageBox(&AlertData, &Id);

			if (Done)
				Done(Rd >= 0 ? Id : -1);
#endif
		}

		KeyMap::KeyMap() : Key(KeyCode_NONE), Mod(KeyMod_NONE), Normal(false)
		{
		}
		KeyMap::KeyMap(const KeyCode& Value) : Key(Value), Mod(KeyMod_NONE), Normal(false)
		{
		}
		KeyMap::KeyMap(const KeyMod& Value) : Key(KeyCode_NONE), Mod(Value), Normal(false)
		{
		}
		KeyMap::KeyMap(const KeyCode& Value, const KeyMod& Control) : Key(Value), Mod(Control), Normal(false)
		{
		}

		void PoseBuffer::SetJointPose(Compute::Joint* Root)
		{
			auto& Node = Pose[Root->Index];
			Node.Position = Root->Transform.Position();
			Node.Rotation = Root->Transform.Rotation();

			for (auto&& Child : Root->Childs)
				SetJointPose(&Child);
		}
		void PoseBuffer::GetJointPose(Compute::Joint* Root, std::vector<Compute::AnimatorKey>* Result)
		{
			Compute::AnimatorKey* Key = &Result->at(Root->Index);
			Key->Position = Root->Transform.Position();
			Key->Rotation = Root->Transform.Rotation();

			for (auto&& Child : Root->Childs)
				GetJointPose(&Child, Result);
		}
		bool PoseBuffer::SetPose(SkinModel* Model)
		{
			if (!Model || Model->Joints.empty())
				return false;

			for (auto&& Child : Model->Joints)
				SetJointPose(&Child);

			return true;
		}
		bool PoseBuffer::GetPose(SkinModel* Model, std::vector<Compute::AnimatorKey>* Result)
		{
			if (!Model || Model->Joints.empty() || !Result)
				return false;

			for (auto&& Child : Model->Joints)
				GetJointPose(&Child, Result);

			return true;
		}
		Compute::Matrix4x4 PoseBuffer::GetOffset(PoseNode* Node)
		{
			if (!Node)
				return Compute::Matrix4x4::Identity();

			return Compute::Matrix4x4::Create(Node->Position, Node->Rotation);
		}
		PoseNode* PoseBuffer::GetNode(int64_t Index)
		{
			auto It = Pose.find(Index);
			if (It != Pose.end())
				return &It->second;

			return nullptr;
		}
		
		Surface::Surface() : Handle(nullptr)
		{
		}
		Surface::Surface(SDL_Surface* From) : Handle(From)
		{
		}
		Surface::~Surface()
		{
#ifdef THAWK_HAS_SDL2
			if (Handle != nullptr)
			{
				SDL_FreeSurface(Handle);
				Handle = nullptr;
			}
#endif
		}
		void Surface::SetHandle(SDL_Surface* From)
		{
#ifdef THAWK_HAS_SDL2
			if (Handle != nullptr)
				SDL_FreeSurface(Handle);
#endif
			Handle = From;
		}
		void Surface::Lock()
		{
#ifdef THAWK_HAS_SDL2
			SDL_LockSurface(Handle);
#endif
		}
		void Surface::Unlock()
		{
#ifdef THAWK_HAS_SDL2
			SDL_UnlockSurface(Handle);
#endif
		}
		int Surface::GetWidth()
		{
#ifdef THAWK_HAS_SDL2
			if (!Handle)
				return -1;

			return Handle->w;
#else
			return -1;
#endif
		}
		int Surface::GetHeight()
		{
#ifdef THAWK_HAS_SDL2
			if (!Handle)
				return -1;

			return Handle->h;
#else
			return -1;
#endif
		}
		int Surface::GetPitch()
		{
#ifdef THAWK_HAS_SDL2
			if (!Handle)
				return -1;

			return Handle->pitch;
#else
			return -1;
#endif
		}
		void* Surface::GetPixels()
		{
#ifdef THAWK_HAS_SDL2
			if (!Handle)
				return nullptr;

			return Handle->pixels;
#else
			return nullptr;
#endif
		}
		void* Surface::GetResource()
		{
			return (void*)Handle;
		}

		DepthStencilState::DepthStencilState(const Desc& I) : State(I)
		{
		}
		DepthStencilState::~DepthStencilState()
		{
		}
		DepthStencilState::Desc DepthStencilState::GetState()
		{
			return State;
		}

		RasterizerState::RasterizerState(const Desc& I) : State(I)
		{
		}
		RasterizerState::~RasterizerState()
		{
		}
		RasterizerState::Desc RasterizerState::GetState()
		{
			return State;
		}

		BlendState::BlendState(const Desc& I) : State(I)
		{
		}
		BlendState::~BlendState()
		{
		}
		BlendState::Desc BlendState::GetState()
		{
			return State;
		}

		SamplerState::SamplerState(const Desc& I) : State(I)
		{
		}
		SamplerState::~SamplerState()
		{
		}
		SamplerState::Desc SamplerState::GetState()
		{
			return State;
		}

		Shader::Shader(const Desc& I)
		{
		}
		InputLayout* Shader::GetShapeVertexLayout()
		{
			static InputLayout ShapeVertexLayout[2] = {{ "POSITION", Format_R32G32B32_Float, 0, 0 }, { "TEXCOORD", Format_R32G32_Float, 3 * sizeof(float), 0 }};

			return ShapeVertexLayout;
		}
		InputLayout* Shader::GetElementVertexLayout()
		{
			static InputLayout ElementVertexLayout[4] = {{ "POSITION", Format_R32G32B32_Float, 0, 0 }, { "TEXCOORD", Format_R32G32B32A32_Float, 3 * sizeof(float), 0 }, { "TEXCOORD", Format_R32G32B32A32_Float, 7 * sizeof(float), 1 }, { "TEXCOORD", Format_R32G32B32_Float, 11 * sizeof(float), 2 }};

			return ElementVertexLayout;
		}
		InputLayout* Shader::GetSkinVertexLayout()
		{
			static InputLayout SkinVertexLayout[7] = {{ "POSITION", Format_R32G32B32_Float, 0, 0 }, { "TEXCOORD", Format_R32G32_Float, 3 * sizeof(float), 0 }, { "NORMAL", Format_R32G32B32_Float, 5 * sizeof(float), 0 }, { "TANGENT", Format_R32G32B32_Float, 8 * sizeof(float), 0 }, { "BINORMAL", Format_R32G32B32_Float, 11 * sizeof(float), 0 }, { "JOINTBIAS", Format_R32G32B32A32_Float, 14 * sizeof(float), 0 }, { "JOINTBIAS", Format_R32G32B32A32_Float, 18 * sizeof(float), 1 }};

			return SkinVertexLayout;
		}
		InputLayout* Shader::GetVertexLayout()
		{
			static InputLayout VertexLayout[5] = {{ "POSITION", Format_R32G32B32_Float, 0, 0 }, { "TEXCOORD", Format_R32G32_Float, 3 * sizeof(float), 0 }, { "NORMAL", Format_R32G32B32_Float, 5 * sizeof(float), 0 }, { "TANGENT", Format_R32G32B32_Float, 8 * sizeof(float), 0 }, { "BINORMAL", Format_R32G32B32_Float, 11 * sizeof(float), 0 }};

			return VertexLayout;
		}
		unsigned int Shader::GetShapeVertexLayoutStride()
		{
			return sizeof(Compute::ShapeVertex);
		}
		unsigned int Shader::GetElementVertexLayoutStride()
		{
			return sizeof(Compute::ElementVertex);
		}
		unsigned int Shader::GetSkinVertexLayoutStride()
		{
			return sizeof(Compute::SkinVertex);
		}
		unsigned int Shader::GetVertexLayoutStride()
		{
			return sizeof(Compute::Vertex);
		}

		ElementBuffer::ElementBuffer(const Desc& I)
		{
			Elements = I.ElementCount;
		}
		uint64_t ElementBuffer::GetElements()
		{
			return Elements;
		}

		StructureBuffer::StructureBuffer(const Desc& I)
		{
			Elements = I.ElementCount;
		}
		uint64_t StructureBuffer::GetElements()
		{
			return Elements;
		}

		MeshBuffer::MeshBuffer(const Desc& I) : VertexBuffer(nullptr), IndexBuffer(nullptr)
		{
		}
		MeshBuffer::~MeshBuffer()
		{
			delete VertexBuffer;
			delete IndexBuffer;
		}
		ElementBuffer* MeshBuffer::GetVertexBuffer()
		{
			return VertexBuffer;
		}
		ElementBuffer* MeshBuffer::GetIndexBuffer()
		{
			return IndexBuffer;
		}

		SkinMeshBuffer::SkinMeshBuffer(const Desc& I) : VertexBuffer(nullptr), IndexBuffer(nullptr)
		{
		}
		SkinMeshBuffer::~SkinMeshBuffer()
		{
			delete VertexBuffer;
			delete IndexBuffer;
		}
		ElementBuffer* SkinMeshBuffer::GetVertexBuffer()
		{
			return VertexBuffer;
		}
		ElementBuffer* SkinMeshBuffer::GetIndexBuffer()
		{
			return IndexBuffer;
		}

		InstanceBuffer::InstanceBuffer(const Desc& I) : Device(I.Device), Elements(nullptr), Sync(false)
		{
			ElementLimit = I.ElementLimit;
			if (ElementLimit < 1)
				ElementLimit = 1;

			Array.Reserve(ElementLimit);
		}
		InstanceBuffer::~InstanceBuffer()
		{
			delete Elements;
		}
		Rest::Pool<Compute::ElementVertex>* InstanceBuffer::GetArray()
		{
			return &Array;
		}
		ElementBuffer* InstanceBuffer::GetElements()
		{
			return Elements;
		}
		GraphicsDevice* InstanceBuffer::GetDevice()
		{
			return Device;
		}
		uint64_t InstanceBuffer::GetElementLimit()
		{
			return ElementLimit;
		}

		Texture2D::Texture2D()
		{
			Width = 512;
			Height = 512;
			MipLevels = 1;
			FormatMode = Graphics::Format_Invalid;
			Usage = Graphics::ResourceUsage_Default;
			AccessFlags = Graphics::CPUAccess_Invalid;
		}
		Texture2D::Texture2D(const Desc& I)
		{
			Width = I.Width;
			Height = I.Height;
			MipLevels = I.MipLevels;
			FormatMode = I.FormatMode;
			Usage = I.Usage;
			AccessFlags = I.AccessFlags;
		}
		CPUAccess Texture2D::GetAccessFlags()
		{
			return AccessFlags;
		}
		Format Texture2D::GetFormatMode()
		{
			return FormatMode;
		}
		ResourceUsage Texture2D::GetUsage()
		{
			return Usage;
		}
		unsigned int Texture2D::GetWidth()
		{
			return Width;
		}
		unsigned int Texture2D::GetHeight()
		{
			return Height;
		}
		unsigned int Texture2D::GetMipLevels()
		{
			return MipLevels;
		}

		Texture3D::Texture3D()
		{
		}
		CPUAccess Texture3D::GetAccessFlags()
		{
			return AccessFlags;
		}
		Format Texture3D::GetFormatMode()
		{
			return FormatMode;
		}
		ResourceUsage Texture3D::GetUsage()
		{
			return Usage;
		}
		unsigned int Texture3D::GetWidth()
		{
			return Width;
		}
		unsigned int Texture3D::GetHeight()
		{
			return Height;
		}
		unsigned int Texture3D::GetDepth()
		{
			return Depth;
		}
		unsigned int Texture3D::GetMipLevels()
		{
			return MipLevels;
		}

		TextureCube::TextureCube()
		{
		}
		TextureCube::TextureCube(const Desc& I)
		{
		}
		CPUAccess TextureCube::GetAccessFlags()
		{
			return AccessFlags;
		}
		Format TextureCube::GetFormatMode()
		{
			return FormatMode;
		}
		ResourceUsage TextureCube::GetUsage()
		{
			return Usage;
		}
		unsigned int TextureCube::GetWidth()
		{
			return Width;
		}
		unsigned int TextureCube::GetHeight()
		{
			return Height;
		}
		unsigned int TextureCube::GetMipLevels()
		{
			return MipLevels;
		}

		DepthBuffer::DepthBuffer(const Desc& I)
		{
		}
		DepthBuffer::~DepthBuffer()
		{
		}

		RenderTarget2D::RenderTarget2D(const Desc& I) : Resource(nullptr)
		{
		}
		RenderTarget2D::~RenderTarget2D()
		{
			delete Resource;
		}
		Texture2D* RenderTarget2D::GetTarget()
		{
			return Resource;
		}

		MultiRenderTarget2D::MultiRenderTarget2D(const Desc& I)
		{
			SVTarget = I.SVTarget;
			for (int i = 0; i < 8; i++)
				Resource[i] = nullptr;
		}
		MultiRenderTarget2D::~MultiRenderTarget2D()
		{
			for (int i = 0; i < SVTarget; i++)
				delete Resource[i];
		}
		SurfaceTarget MultiRenderTarget2D::GetSVTarget()
		{
			return SVTarget;
		}
		Texture2D* MultiRenderTarget2D::GetTarget(unsigned int Target)
		{
			if (Target >= SVTarget)
				return nullptr;

			return Resource[Target];
		}

		RenderTarget2DArray::RenderTarget2DArray(const Desc& I) : Resource(nullptr)
		{
		}
		RenderTarget2DArray::~RenderTarget2DArray()
		{
			delete Resource;
		}
		Texture2D* RenderTarget2DArray::GetTarget()
		{
			return Resource;
		}

		RenderTargetCube::RenderTargetCube(const Desc& I) : Resource(nullptr)
		{
		}
		RenderTargetCube::~RenderTargetCube()
		{
			delete Resource;
		}
		Texture2D* RenderTargetCube::GetTarget()
		{
			return Resource;
		}

		MultiRenderTargetCube::MultiRenderTargetCube(const Desc& I)
		{
			SVTarget = I.Target;
			for (int i = 0; i < 8; i++)
				Resource[i] = nullptr;
		}
		MultiRenderTargetCube::~MultiRenderTargetCube()
		{
			for (int i = 0; i < SVTarget; i++)
				delete Resource[i];
		}
		SurfaceTarget MultiRenderTargetCube::GetSVTarget()
		{
			return SVTarget;
		}
		Texture2D* MultiRenderTargetCube::GetTarget(unsigned int Target)
		{
			if (Target >= SVTarget)
				return nullptr;

			return Resource[Target];
		}

		Query::Query()
		{
		}

		GraphicsDevice::GraphicsDevice(const Desc& I) : MaxElements(1), ViewResource(nullptr), Primitives(PrimitiveTopology_Triangle_List)
		{
			ShaderModelType = Graphics::ShaderModel_Invalid;
			VSyncMode = I.VSyncMode;
			PresentFlags = I.PresentationFlags;
			CompileFlags = I.CompilationFlags;
			Backend = I.Backend;
			Debug = I.Debug;
			InitSections();
		}
		GraphicsDevice::~GraphicsDevice()
		{
			FreeProxy();
			for (auto It = Sections.begin(); It != Sections.end(); It++)
				delete It->second;
			Sections.clear();
		}
		void GraphicsDevice::Lock()
		{
			Mutex.lock();
		}
		void GraphicsDevice::Unlock()
		{
			Mutex.unlock();
		}
		void GraphicsDevice::AddSection(const std::string& Name, const std::string& Code)
		{
			Section* Include = new Section();
			Include->Code = Code;
			Include->Name = Name;

			RemoveSection(Name);
			Sections[Name] = Include;
		}
		void GraphicsDevice::RemoveSection(const std::string& Name)
		{
			auto It = Sections.find(Name);
			if (It == Sections.end())
				return;

			delete It->second;
			Sections.erase(It);
		}
		bool GraphicsDevice::Preprocess(Shader::Desc& In)
		{
			if (In.Data.empty())
				return true;

			Compute::IncludeDesc Desc;
			Desc.Exts.push_back(".hlsl");
			Desc.Exts.push_back(".glsl");
			Desc.Root = Rest::OS::GetDirectory();
			In.Features.Pragmas = false;

			Compute::Preprocessor* Processor = new Compute::Preprocessor();
			Processor->SetIncludeCallback([this, &In](Compute::Preprocessor* P, const Compute::IncludeResult& File, std::string* Output)
			{
				if (In.Include && In.Include(P, File, Output))
					return true;

				if (File.Module.empty() || (!File.IsFile && !File.IsSystem))
					return false;

				if (File.IsSystem && !File.IsFile)
					return GetSection(File.Module, Output, true);

				uint64_t Length;
				unsigned char* Data = Rest::OS::ReadAllBytes(File.Module.c_str(), &Length);
				if (!Data)
					return false;

				Output->assign((const char*)Data, (size_t)Length);
				free(Data);
				return true;
			});
			Processor->SetIncludeOptions(Desc);
			Processor->SetFeatures(In.Features);

			for (auto& Word : In.Defines)
				Processor->Define(Word);

			bool Result = Processor->Process(In.Filename, In.Data);
			delete Processor;

			return Result;
		}
		void GraphicsDevice::InitStates()
		{
			Graphics::DepthStencilState::Desc DepthStencil;
			DepthStencil.DepthEnable = true;
			DepthStencil.DepthWriteMask = Graphics::DepthWrite_All;
			DepthStencil.DepthFunction = Graphics::Comparison_Less;
			DepthStencil.StencilEnable = true;
			DepthStencil.StencilReadMask = 0xFF;
			DepthStencil.StencilWriteMask = 0xFF;
			DepthStencil.FrontFaceStencilFailOperation = Graphics::StencilOperation_Keep;
			DepthStencil.FrontFaceStencilDepthFailOperation = Graphics::StencilOperation_Add;
			DepthStencil.FrontFaceStencilPassOperation = Graphics::StencilOperation_Keep;
			DepthStencil.FrontFaceStencilFunction = Graphics::Comparison_Always;
			DepthStencil.BackFaceStencilFailOperation = Graphics::StencilOperation_Keep;
			DepthStencil.BackFaceStencilDepthFailOperation = Graphics::StencilOperation_Subtract;
			DepthStencil.BackFaceStencilPassOperation = Graphics::StencilOperation_Keep;
			DepthStencil.BackFaceStencilFunction = Graphics::Comparison_Always;
			DepthStencil.Name = "DEF_LESS";
			DepthStencilStates[DepthStencil.Name] = CreateDepthStencilState(DepthStencil);

			DepthStencil.DepthEnable = true;
			DepthStencil.DepthWriteMask = Graphics::DepthWrite_Zero;
			DepthStencil.DepthFunction = Graphics::Comparison_Greater_Equal;
			DepthStencil.StencilEnable = true;
			DepthStencil.StencilReadMask = 0xFF;
			DepthStencil.StencilWriteMask = 0xFF;
			DepthStencil.FrontFaceStencilFailOperation = Graphics::StencilOperation_Keep;
			DepthStencil.FrontFaceStencilDepthFailOperation = Graphics::StencilOperation_Add;
			DepthStencil.FrontFaceStencilPassOperation = Graphics::StencilOperation_Keep;
			DepthStencil.FrontFaceStencilFunction = Graphics::Comparison_Always;
			DepthStencil.BackFaceStencilFailOperation = Graphics::StencilOperation_Keep;
			DepthStencil.BackFaceStencilDepthFailOperation = Graphics::StencilOperation_Subtract;
			DepthStencil.BackFaceStencilPassOperation = Graphics::StencilOperation_Keep;
			DepthStencil.BackFaceStencilFunction = Graphics::Comparison_Always;
			DepthStencil.Name = "DEF_GREATER_EQUAL";
			DepthStencilStates[DepthStencil.Name] = CreateDepthStencilState(DepthStencil);

			DepthStencil.DepthEnable = false;
			DepthStencil.DepthWriteMask = Graphics::DepthWrite_All;
			DepthStencil.DepthFunction = Graphics::Comparison_Less;
			DepthStencil.StencilEnable = false;
			DepthStencil.StencilReadMask = 0xFF;
			DepthStencil.StencilWriteMask = 0xFF;
			DepthStencil.FrontFaceStencilFailOperation = Graphics::StencilOperation_Keep;
			DepthStencil.FrontFaceStencilDepthFailOperation = Graphics::StencilOperation_Add;
			DepthStencil.FrontFaceStencilPassOperation = Graphics::StencilOperation_Keep;
			DepthStencil.FrontFaceStencilFunction = Graphics::Comparison_Always;
			DepthStencil.BackFaceStencilFailOperation = Graphics::StencilOperation_Keep;
			DepthStencil.BackFaceStencilDepthFailOperation = Graphics::StencilOperation_Subtract;
			DepthStencil.BackFaceStencilPassOperation = Graphics::StencilOperation_Keep;
			DepthStencil.BackFaceStencilFunction = Graphics::Comparison_Always;
			DepthStencil.Name = "DEF_NONE";
			DepthStencilStates[DepthStencil.Name] = CreateDepthStencilState(DepthStencil);

			DepthStencil.DepthEnable = true;
			DepthStencil.DepthWriteMask = Graphics::DepthWrite_Zero;
			DepthStencil.DepthFunction = Graphics::Comparison_Less;
			DepthStencil.StencilEnable = true;
			DepthStencil.StencilReadMask = 0xFF;
			DepthStencil.StencilWriteMask = 0xFF;
			DepthStencil.FrontFaceStencilFailOperation = Graphics::StencilOperation_Keep;
			DepthStencil.FrontFaceStencilDepthFailOperation = Graphics::StencilOperation_Add;
			DepthStencil.FrontFaceStencilPassOperation = Graphics::StencilOperation_Keep;
			DepthStencil.FrontFaceStencilFunction = Graphics::Comparison_Always;
			DepthStencil.BackFaceStencilFailOperation = Graphics::StencilOperation_Keep;
			DepthStencil.BackFaceStencilDepthFailOperation = Graphics::StencilOperation_Subtract;
			DepthStencil.BackFaceStencilPassOperation = Graphics::StencilOperation_Keep;
			DepthStencil.BackFaceStencilFunction = Graphics::Comparison_Always;
			DepthStencil.Name = "DEF_NONE_LESS";
			DepthStencilStates[DepthStencil.Name] = CreateDepthStencilState(DepthStencil);

			DepthStencil.DepthEnable = true;
			DepthStencil.DepthWriteMask = Graphics::DepthWrite_All;
			DepthStencil.DepthFunction = Graphics::Comparison_Less;
			DepthStencil.StencilEnable = false;
			DepthStencil.StencilReadMask = 0xFF;
			DepthStencil.StencilWriteMask = 0xFF;
			DepthStencil.FrontFaceStencilFailOperation = Graphics::StencilOperation_Keep;
			DepthStencil.FrontFaceStencilDepthFailOperation = Graphics::StencilOperation_Add;
			DepthStencil.FrontFaceStencilPassOperation = Graphics::StencilOperation_Keep;
			DepthStencil.FrontFaceStencilFunction = Graphics::Comparison_Always;
			DepthStencil.BackFaceStencilFailOperation = Graphics::StencilOperation_Keep;
			DepthStencil.BackFaceStencilDepthFailOperation = Graphics::StencilOperation_Subtract;
			DepthStencil.BackFaceStencilPassOperation = Graphics::StencilOperation_Keep;
			DepthStencil.BackFaceStencilFunction = Graphics::Comparison_Always;
			DepthStencil.Name = "DEF_LESS_NO_STENCIL";
			DepthStencilStates[DepthStencil.Name] = CreateDepthStencilState(DepthStencil);

			Graphics::RasterizerState::Desc Rasterizer;
			Rasterizer.AntialiasedLineEnable = false;
			Rasterizer.CullMode = Graphics::VertexCull_Back;
			Rasterizer.DepthBias = 0;
			Rasterizer.DepthBiasClamp = 0;
			Rasterizer.DepthClipEnable = true;
			Rasterizer.FillMode = Graphics::SurfaceFill_Solid;
			Rasterizer.FrontCounterClockwise = false;
			Rasterizer.MultisampleEnable = false;
			Rasterizer.ScissorEnable = false;
			Rasterizer.SlopeScaledDepthBias = 0.0f;
			Rasterizer.Name = "DEF_CULL_BACK";
			RasterizerStates[Rasterizer.Name] = CreateRasterizerState(Rasterizer);

			Rasterizer.AntialiasedLineEnable = false;
			Rasterizer.CullMode = Graphics::VertexCull_Front;
			Rasterizer.DepthBias = 0;
			Rasterizer.DepthBiasClamp = 0;
			Rasterizer.DepthClipEnable = true;
			Rasterizer.FillMode = Graphics::SurfaceFill_Solid;
			Rasterizer.FrontCounterClockwise = false;
			Rasterizer.MultisampleEnable = false;
			Rasterizer.ScissorEnable = false;
			Rasterizer.SlopeScaledDepthBias = 0.0f;
			Rasterizer.Name = "DEF_CULL_FRONT";
			RasterizerStates[Rasterizer.Name] = CreateRasterizerState(Rasterizer);

			Rasterizer.AntialiasedLineEnable = false;
			Rasterizer.CullMode = Graphics::VertexCull_Disabled;
			Rasterizer.DepthBias = 0;
			Rasterizer.DepthBiasClamp = 0;
			Rasterizer.DepthClipEnable = true;
			Rasterizer.FillMode = Graphics::SurfaceFill_Solid;
			Rasterizer.FrontCounterClockwise = false;
			Rasterizer.MultisampleEnable = false;
			Rasterizer.ScissorEnable = false;
			Rasterizer.SlopeScaledDepthBias = 0.0f;
			Rasterizer.Name = "DEF_CULL_NONE";
			RasterizerStates[Rasterizer.Name] = CreateRasterizerState(Rasterizer);

			Rasterizer.AntialiasedLineEnable = false;
			Rasterizer.CullMode = Graphics::VertexCull_Disabled;
			Rasterizer.DepthBias = 0;
			Rasterizer.DepthBiasClamp = 0;
			Rasterizer.DepthClipEnable = true;
			Rasterizer.FillMode = Graphics::SurfaceFill_Solid;
			Rasterizer.FrontCounterClockwise = false;
			Rasterizer.MultisampleEnable = false;
			Rasterizer.ScissorEnable = true;
			Rasterizer.SlopeScaledDepthBias = 0.0f;
			Rasterizer.Name = "DEF_CULL_NONE_SCISSOR";
			RasterizerStates[Rasterizer.Name] = CreateRasterizerState(Rasterizer);

			Graphics::BlendState::Desc Blend;
			Blend.AlphaToCoverageEnable = false;
			Blend.IndependentBlendEnable = false;
			Blend.RenderTarget[0].BlendEnable = false;
			Blend.RenderTarget[0].RenderTargetWriteMask = Graphics::ColorWriteEnable_All;
			Blend.Name = "DEF_OVERWRITE";
			BlendStates[Blend.Name] = CreateBlendState(Blend);

			Blend.AlphaToCoverageEnable = false;
			Blend.IndependentBlendEnable = false;
			Blend.RenderTarget[0].BlendEnable = false;
			Blend.RenderTarget[0].RenderTargetWriteMask = 0;
			Blend.Name = "DEF_OVERWRITE_COLORLESS";
			BlendStates[Blend.Name] = CreateBlendState(Blend);

			Blend.AlphaToCoverageEnable = false;
			Blend.IndependentBlendEnable = false;
			Blend.RenderTarget[0].BlendEnable = true;
			Blend.RenderTarget[0].SrcBlend = Graphics::Blend_One;
			Blend.RenderTarget[0].DestBlend = Graphics::Blend_One;
			Blend.RenderTarget[0].BlendOperationMode = Graphics::BlendOperation_Add;
			Blend.RenderTarget[0].SrcBlendAlpha = Graphics::Blend_One;
			Blend.RenderTarget[0].DestBlendAlpha = Graphics::Blend_One;
			Blend.RenderTarget[0].BlendOperationAlpha = Graphics::BlendOperation_Add;
			Blend.RenderTarget[0].RenderTargetWriteMask = Graphics::ColorWriteEnable_All;
			Blend.Name = "DEF_ADDITIVE";
			BlendStates[Blend.Name] = CreateBlendState(Blend);

			Blend.AlphaToCoverageEnable = false;
			Blend.IndependentBlendEnable = true;
			for (unsigned int i = 0; i < 8; i++)
			{
				Blend.RenderTarget[i].BlendEnable = true;
				Blend.RenderTarget[i].SrcBlend = Graphics::Blend_One;
				Blend.RenderTarget[i].DestBlend = Graphics::Blend_One;
				Blend.RenderTarget[i].BlendOperationMode = Graphics::BlendOperation_Add;
				Blend.RenderTarget[i].SrcBlendAlpha = Graphics::Blend_One;
				Blend.RenderTarget[i].DestBlendAlpha = Graphics::Blend_One;
				Blend.RenderTarget[i].BlendOperationAlpha = Graphics::BlendOperation_Add;
				Blend.RenderTarget[i].RenderTargetWriteMask = Graphics::ColorWriteEnable_All;
			}
			Blend.RenderTarget[1].BlendEnable = false;
			Blend.RenderTarget[2].BlendEnable = false;
			Blend.Name = "DEF_GB_ADDITIVE";
			BlendStates[Blend.Name] = CreateBlendState(Blend);

			Blend.AlphaToCoverageEnable = false;
			Blend.IndependentBlendEnable = false;
			Blend.RenderTarget[0].BlendEnable = true;
			Blend.RenderTarget[0].SrcBlend = Graphics::Blend_Source_Alpha;
			Blend.RenderTarget[0].DestBlend = Graphics::Blend_One;
			Blend.RenderTarget[0].BlendOperationMode = Graphics::BlendOperation_Add;
			Blend.RenderTarget[0].SrcBlendAlpha = Graphics::Blend_One;
			Blend.RenderTarget[0].DestBlendAlpha = Graphics::Blend_One;
			Blend.RenderTarget[0].BlendOperationAlpha = Graphics::BlendOperation_Add;
			Blend.RenderTarget[0].RenderTargetWriteMask = Graphics::ColorWriteEnable_All;
			Blend.Name = "DEF_ADDITIVE_ALPHA";
			BlendStates[Blend.Name] = CreateBlendState(Blend);

			Blend.AlphaToCoverageEnable = false;
			Blend.IndependentBlendEnable = false;
			Blend.RenderTarget[0].BlendEnable = true;
			Blend.RenderTarget[0].SrcBlend = Graphics::Blend_Source_Alpha;
			Blend.RenderTarget[0].DestBlend = Graphics::Blend_Source_Alpha_Invert;
			Blend.RenderTarget[0].BlendOperationMode = Graphics::BlendOperation_Add;
			Blend.RenderTarget[0].SrcBlendAlpha = Graphics::Blend_Source_Alpha_Invert;
			Blend.RenderTarget[0].DestBlendAlpha = Graphics::Blend_Zero;
			Blend.RenderTarget[0].BlendOperationAlpha = Graphics::BlendOperation_Add;
			Blend.RenderTarget[0].RenderTargetWriteMask = Graphics::ColorWriteEnable_All;
			Blend.Name = "DEF_ADDITIVE_SOURCE";
			BlendStates[Blend.Name] = CreateBlendState(Blend);

			Graphics::SamplerState::Desc Sampler;
			Sampler.Filter = Graphics::PixelFilter_Anistropic;
			Sampler.AddressU = Graphics::TextureAddress_Wrap;
			Sampler.AddressV = Graphics::TextureAddress_Wrap;
			Sampler.AddressW = Graphics::TextureAddress_Wrap;
			Sampler.MipLODBias = 0.0f;
			Sampler.MaxAnisotropy = 16;
			Sampler.ComparisonFunction = Graphics::Comparison_Never;
			Sampler.BorderColor[0] = 0.0f;
			Sampler.BorderColor[1] = 0.0f;
			Sampler.BorderColor[2] = 0.0f;
			Sampler.BorderColor[3] = 0.0f;
			Sampler.MinLOD = 0.0f;
			Sampler.MaxLOD = std::numeric_limits<float>::max();
			Sampler.Name = "DEF_TRILINEAR_X16";
			SamplerStates[Sampler.Name] = CreateSamplerState(Sampler);

			Sampler.Filter = Graphics::PixelFilter_Min_Mag_Mip_Linear;
			Sampler.AddressU = Graphics::TextureAddress_Clamp;
			Sampler.AddressV = Graphics::TextureAddress_Clamp;
			Sampler.AddressW = Graphics::TextureAddress_Clamp;
			Sampler.MipLODBias = 0.0f;
			Sampler.ComparisonFunction = Graphics::Comparison_Always;
			Sampler.MinLOD = 0.0f;
			Sampler.MaxLOD = std::numeric_limits<float>::max();
			Sampler.Name = "DEF_LINEAR";
			SamplerStates[Sampler.Name] = CreateSamplerState(Sampler);

			Sampler.Filter = Graphics::PixelFilter_Min_Mag_Mip_Point;
			Sampler.AddressU = Graphics::TextureAddress_Clamp;
			Sampler.AddressV = Graphics::TextureAddress_Clamp;
			Sampler.AddressW = Graphics::TextureAddress_Clamp;
			Sampler.MipLODBias = 0.0f;
			Sampler.ComparisonFunction = Graphics::Comparison_Never;
			Sampler.MinLOD = 0.0f;
			Sampler.MaxLOD = std::numeric_limits<float>::max();
			Sampler.Name = "DEF_POINT";
			SamplerStates[Sampler.Name] = CreateSamplerState(Sampler);

			SetDepthStencilState(GetDepthStencilState("DEF_LESS"));
			SetRasterizerState(GetRasterizerState("DEF_CULL_BACK"));
			SetBlendState(GetBlendState("DEF_OVERWRITE"));
			SetSamplerState(GetSamplerState("DEF_TRILINEAR_X16"));
		}
		void GraphicsDevice::InitSections()
		{
			if (Backend == RenderBackend_D3D11)
			{
#ifdef HAS_D3D11_GEOMETRY_EMITTER_DEPTH_POINT_HLSL
				AddSection("geometry/emitter/depth-point.hlsl", GET_RESOURCE_BATCH(d3d11_geometry_emitter_depth_point_hlsl));
#else
				THAWK_ERROR("geometry/emitter/depth-point.hlsl was not compiled");
#endif
#ifdef HAS_D3D11_GEOMETRY_EMITTER_DEPTH_QUAD_HLSL
				AddSection("geometry/emitter/depth-quad.hlsl", GET_RESOURCE_BATCH(d3d11_geometry_emitter_depth_quad_hlsl));
#else
				THAWK_ERROR("geometry/emitter/depth-quad.hlsl was not compiled");
#endif
#ifdef HAS_D3D11_GEOMETRY_EMITTER_DEPTH_LINEAR_HLSL
				AddSection("geometry/emitter/depth-linear.hlsl", GET_RESOURCE_BATCH(d3d11_geometry_emitter_depth_linear_hlsl));
#else
				THAWK_ERROR("geometry/emitter/depth-linear.hlsl was not compiled");
#endif
#ifdef HAS_D3D11_GEOMETRY_EMITTER_GBUFFER_OPAQUE_HLSL
				AddSection("geometry/emitter/gbuffer-opaque.hlsl", GET_RESOURCE_BATCH(d3d11_geometry_emitter_gbuffer_opaque_hlsl));
#else
				THAWK_ERROR("geometry/emitter/gbuffer-opaque.hlsl was not compiled");
#endif
#ifdef HAS_D3D11_GEOMETRY_EMITTER_GBUFFER_LIMPID_HLSL
				AddSection("geometry/emitter/gbuffer-limpid.hlsl", GET_RESOURCE_BATCH(d3d11_geometry_emitter_gbuffer_limpid_hlsl));
#else
				THAWK_ERROR("geometry/emitter/gbuffer-limpid.hlsl was not compiled");
#endif
#ifdef HAS_D3D11_GEOMETRY_MODEL_OCCLUSION_HLSL
				AddSection("geometry/model/occlusion.hlsl", GET_RESOURCE_BATCH(d3d11_geometry_model_occlusion_hlsl));
#else
				THAWK_ERROR("geometry/model/occlusion.hlsl was not compiled");
#endif
#ifdef HAS_D3D11_GEOMETRY_MODEL_DEPTH_CUBIC_HLSL
				AddSection("geometry/model/depth-cubic.hlsl", GET_RESOURCE_BATCH(d3d11_geometry_model_depth_cubic_hlsl));
#else
				THAWK_ERROR("geometry/model/depth-cubic.hlsl was not compiled");
#endif
#ifdef HAS_D3D11_GEOMETRY_MODEL_DEPTH_LINEAR_HLSL
				AddSection("geometry/model/depth-linear.hlsl", GET_RESOURCE_BATCH(d3d11_geometry_model_depth_linear_hlsl));
#else
				THAWK_ERROR("geometry/model/depth-linear.hlsl was not compiled");
#endif
#ifdef HAS_D3D11_GEOMETRY_MODEL_GBUFFER_HLSL
				AddSection("geometry/model/gbuffer.hlsl", GET_RESOURCE_BATCH(d3d11_geometry_model_gbuffer_hlsl));
#else
				THAWK_ERROR("geometry/model/gbuffer.hlsl was not compiled");
#endif
#ifdef HAS_D3D11_GEOMETRY_SKIN_OCCLUSION_HLSL
				AddSection("geometry/skin/occlusion.hlsl", GET_RESOURCE_BATCH(d3d11_geometry_skin_occlusion_hlsl));
#else
				THAWK_ERROR("geometry/skin/occlusion.hlsl was not compiled");
#endif
#ifdef HAS_D3D11_GEOMETRY_SKIN_DEPTH_CUBIC_HLSL
				AddSection("geometry/skin/depth-cubic.hlsl", GET_RESOURCE_BATCH(d3d11_geometry_skin_depth_cubic_hlsl));
#else
				THAWK_ERROR("geometry/skin/depth-cubic.hlsl was not compiled");
#endif
#ifdef HAS_D3D11_GEOMETRY_SKIN_DEPTH_LINEAR_HLSL
				AddSection("geometry/skin/depth-linear.hlsl", GET_RESOURCE_BATCH(d3d11_geometry_skin_depth_linear_hlsl));
#else
				THAWK_ERROR("geometry/skin/depth-linear.hlsl was not compiled");
#endif
#ifdef HAS_D3D11_GEOMETRY_SKIN_GBUFFER_HLSL
				AddSection("geometry/skin/gbuffer.hlsl", GET_RESOURCE_BATCH(d3d11_geometry_skin_gbuffer_hlsl));
#else
				THAWK_ERROR("geometry/skin/gbuffer.hlsl was not compiled");
#endif
#ifdef HAS_D3D11_GEOMETRY_DECAL_GBUFFER_HLSL
				AddSection("geometry/decal/gbuffer.hlsl", GET_RESOURCE_BATCH(d3d11_geometry_decal_gbuffer_hlsl));
#else
				THAWK_ERROR("geometry/decal/gbuffer.hlsl was not compiled");
#endif
#ifdef HAS_D3D11_GEOMETRY_LIGHT_BASE_AMBIENT_HLSL
				AddSection("geometry/light/base/ambient.hlsl", GET_RESOURCE_BATCH(d3d11_geometry_light_base_ambient_hlsl));
#else
				THAWK_ERROR("geometry/light/base/ambient.hlsl was not compiled");
#endif
#ifdef HAS_D3D11_GEOMETRY_LIGHT_BASE_PROBE_HLSL
				AddSection("geometry/light/base/probe.hlsl", GET_RESOURCE_BATCH(d3d11_geometry_light_base_probe_hlsl));
#else
				THAWK_ERROR("geometry/light/base/probe.hlsl was not compiled");
#endif
#ifdef HAS_D3D11_GEOMETRY_LIGHT_BASE_LINE_HLSL
				AddSection("geometry/light/base/line.hlsl", GET_RESOURCE_BATCH(d3d11_geometry_light_base_line_hlsl));
#else
				THAWK_ERROR("geometry/light/base/line.hlsl was not compiled");
#endif
#ifdef HAS_D3D11_GEOMETRY_LIGHT_BASE_POINT_HLSL
				AddSection("geometry/light/base/point.hlsl", GET_RESOURCE_BATCH(d3d11_geometry_light_base_point_hlsl));
#else
				THAWK_ERROR("geometry/light/base/point.hlsl was not compiled");
#endif
#ifdef HAS_D3D11_GEOMETRY_LIGHT_BASE_SPOT_HLSL
				AddSection("geometry/light/base/spot.hlsl", GET_RESOURCE_BATCH(d3d11_geometry_light_base_spot_hlsl));
#else
				THAWK_ERROR("geometry/light/base/spot.hlsl was not compiled");
#endif
#ifdef HAS_D3D11_GEOMETRY_LIGHT_SHADE_LINE_HLSL
				AddSection("geometry/light/shade/line.hlsl", GET_RESOURCE_BATCH(d3d11_geometry_light_shade_line_hlsl));
#else
				THAWK_ERROR("geometry/light/shade/line.hlsl was not compiled");
#endif
#ifdef HAS_D3D11_GEOMETRY_LIGHT_SHADE_POINT_HLSL
				AddSection("geometry/light/shade/point.hlsl", GET_RESOURCE_BATCH(d3d11_geometry_light_shade_point_hlsl));
#else
				THAWK_ERROR("geometry/light/shade/point.hlsl was not compiled");
#endif
#ifdef HAS_D3D11_GEOMETRY_LIGHT_SHADE_SPOT_HLSL
				AddSection("geometry/light/shade/spot.hlsl", GET_RESOURCE_BATCH(d3d11_geometry_light_shade_spot_hlsl));
#else
				THAWK_ERROR("geometry/light/shade/spot.hlsl was not compiled");
#endif
#ifdef HAS_D3D11_PASS_LIMPID_HLSL
				AddSection("pass/limpid.hlsl", GET_RESOURCE_BATCH(d3d11_pass_limpid_hlsl));
#else
				THAWK_ERROR("pass/limpid.hlsl was not compiled");
#endif
#ifdef HAS_D3D11_PASS_AMBIENT_HLSL
				AddSection("pass/ambient.hlsl", GET_RESOURCE_BATCH(d3d11_pass_ambient_hlsl));
#else
				THAWK_ERROR("pass/ambient.hlsl was not compiled");
#endif
#ifdef HAS_D3D11_PASS_BLOOM_HLSL
				AddSection("pass/bloom.hlsl", GET_RESOURCE_BATCH(d3d11_pass_bloom_hlsl));
#else
				THAWK_ERROR("pass/bloom.hlsl was not compiled");
#endif
#ifdef HAS_D3D11_PASS_BLUR_HLSL
				AddSection("pass/blur.hlsl", GET_RESOURCE_BATCH(d3d11_pass_blur_hlsl));
#else
				THAWK_ERROR("pass/blur.hlsl was not compiled");
#endif
#ifdef HAS_D3D11_PASS_FOCUS_HLSL
				AddSection("pass/focus.hlsl", GET_RESOURCE_BATCH(d3d11_pass_focus_hlsl));
#else
				THAWK_ERROR("pass/focus.hlsl was not compiled");
#endif
#ifdef HAS_D3D11_PASS_GLITCH_HLSL
				AddSection("pass/glitch.hlsl", GET_RESOURCE_BATCH(d3d11_pass_glitch_hlsl));
#else
				THAWK_ERROR("pass/glitch.hlsl was not compiled");
#endif
#ifdef HAS_D3D11_PASS_GUI_HLSL
				AddSection("pass/gui.hlsl", GET_RESOURCE_BATCH(d3d11_pass_gui_hlsl));
#else
				THAWK_ERROR("pass/gui.hlsl was not compiled");
#endif
#ifdef HAS_D3D11_PASS_INDIRECT_HLSL
				AddSection("pass/indirect.hlsl", GET_RESOURCE_BATCH(d3d11_pass_indirect_hlsl));
#else
				THAWK_ERROR("pass/indirect.hlsl was not compiled");
#endif
#ifdef HAS_D3D11_PASS_GLOSS_HLSL
				AddSection("pass/gloss.hlsl", GET_RESOURCE_BATCH(d3d11_pass_gloss_hlsl));
#else
				THAWK_ERROR("pass/gloss.hlsl was not compiled");
#endif
#ifdef HAS_D3D11_PASS_REFLECTION_HLSL
				AddSection("pass/reflection.hlsl", GET_RESOURCE_BATCH(d3d11_pass_reflection_hlsl));
#else
				THAWK_ERROR("pass/reflection.hlsl was not compiled");
#endif
#ifdef HAS_D3D11_PASS_TONE_HLSL
				AddSection("pass/tone.hlsl", GET_RESOURCE_BATCH(d3d11_pass_tone_hlsl));
#else
				THAWK_ERROR("pass/tone.hlsl was not compiled");
#endif
#ifdef HAS_D3D11_RENDERER_BUFFER_ANIMATION_HLSL
				AddSection("renderer/buffer/animation.hlsl", GET_RESOURCE_BATCH(d3d11_renderer_buffer_animation_hlsl));
#else
				THAWK_ERROR("renderer/buffer/animation.hlsl was not compiled");
#endif
#ifdef HAS_D3D11_RENDERER_BUFFER_CUBIC_HLSL
				AddSection("renderer/buffer/cubic.hlsl", GET_RESOURCE_BATCH(d3d11_renderer_buffer_cubic_hlsl));
#else
				THAWK_ERROR("renderer/buffer/cubic.hlsl was not compiled");
#endif
#ifdef HAS_D3D11_RENDERER_BUFFER_GUI_HLSL
				AddSection("renderer/buffer/gui.hlsl", GET_RESOURCE_BATCH(d3d11_renderer_buffer_gui_hlsl));
#else
				THAWK_ERROR("renderer/buffer/gui.hlsl was not compiled");
#endif
#ifdef HAS_D3D11_RENDERER_BUFFER_OBJECT_HLSL
				AddSection("renderer/buffer/object.hlsl", GET_RESOURCE_BATCH(d3d11_renderer_buffer_object_hlsl));
#else
				THAWK_ERROR("renderer/buffer/object.hlsl was not compiled");
#endif
#ifdef HAS_D3D11_RENDERER_BUFFER_VIEWER_HLSL
				AddSection("renderer/buffer/viewer.hlsl", GET_RESOURCE_BATCH(d3d11_renderer_buffer_viewer_hlsl));
#else
				THAWK_ERROR("renderer/buffer/viewer.hlsl was not compiled");
#endif
#ifdef HAS_D3D11_RENDERER_INPUT_BASE_HLSL
				AddSection("renderer/input/base.hlsl", GET_RESOURCE_BATCH(d3d11_renderer_input_base_hlsl));
#else
				THAWK_ERROR("renderer/input/base.hlsl was not compiled");
#endif
#ifdef HAS_D3D11_RENDERER_INPUT_ELEMENT_HLSL
				AddSection("renderer/input/element.hlsl", GET_RESOURCE_BATCH(d3d11_renderer_input_element_hlsl));
#else
				THAWK_ERROR("renderer/input/element.hlsl was not compiled");
#endif
#ifdef HAS_D3D11_RENDERER_INPUT_SV_HLSL
				AddSection("renderer/input/sv.hlsl", GET_RESOURCE_BATCH(d3d11_renderer_input_sv_hlsl));
#else
				THAWK_ERROR("renderer/input/sv.hlsl was not compiled");
#endif
#ifdef HAS_D3D11_RENDERER_VERTEX_ELEMENT_HLSL
				AddSection("renderer/vertex/element.hlsl", GET_RESOURCE_BATCH(d3d11_renderer_vertex_element_hlsl));
#else
				THAWK_ERROR("renderer/vertex/element.hlsl was not compiled");
#endif
#ifdef HAS_D3D11_RENDERER_VERTEX_GUI_HLSL
				AddSection("renderer/vertex/gui.hlsl", GET_RESOURCE_BATCH(d3d11_renderer_vertex_gui_hlsl));
#else
				THAWK_ERROR("renderer/vertex/gui.hlsl was not compiled");
#endif
#ifdef HAS_D3D11_RENDERER_VERTEX_SHAPE_HLSL
				AddSection("renderer/vertex/shape.hlsl", GET_RESOURCE_BATCH(d3d11_renderer_vertex_shape_hlsl));
#else
				THAWK_ERROR("renderer/vertex/shape.hlsl was not compiled");
#endif
#ifdef HAS_D3D11_RENDERER_VERTEX_SKIN_HLSL
				AddSection("renderer/vertex/skin.hlsl", GET_RESOURCE_BATCH(d3d11_renderer_vertex_skin_hlsl));
#else
				THAWK_ERROR("renderer/vertex/skin.hlsl was not compiled");
#endif
#ifdef HAS_D3D11_RENDERER_ELEMENT_HLSL
				AddSection("renderer/element.hlsl", GET_RESOURCE_BATCH(d3d11_renderer_element_hlsl));
#else
				THAWK_ERROR("renderer/element.hlsl was not compiled");
#endif
#ifdef HAS_D3D11_RENDERER_FRAGMENT_HLSL
				AddSection("renderer/fragment.hlsl", GET_RESOURCE_BATCH(d3d11_renderer_fragment_hlsl));
#else
				THAWK_ERROR("renderer/fragment.hlsl was not compiled");
#endif
#ifdef HAS_D3D11_RENDERER_GBUFFER_HLSL
				AddSection("renderer/gbuffer.hlsl", GET_RESOURCE_BATCH(d3d11_renderer_gbuffer_hlsl));
#else
				THAWK_ERROR("renderer/gbuffer.hlsl was not compiled");
#endif
#ifdef HAS_D3D11_RENDERER_MATERIAL_HLSL
				AddSection("renderer/material.hlsl", GET_RESOURCE_BATCH(d3d11_renderer_material_hlsl));
#else
				THAWK_ERROR("renderer/material.hlsl was not compiled");
#endif
#ifdef HAS_D3D11_RENDERER_VERTEX_HLSL
				AddSection("renderer/vertex.hlsl", GET_RESOURCE_BATCH(d3d11_renderer_vertex_hlsl));
#else
				THAWK_ERROR("renderer/vertex.hlsl was not compiled");
#endif
#ifdef HAS_D3D11_STANDARD_COMPRESS_HLSL
				AddSection("standard/compress.hlsl", GET_RESOURCE_BATCH(d3d11_standard_compress_hlsl));
#else
				THAWK_ERROR("standard/compress.hlsl was not compiled");
#endif
#ifdef HAS_D3D11_STANDARD_ATMOSPHERE_HLSL
				AddSection("standard/atmosphere.hlsl", GET_RESOURCE_BATCH(d3d11_standard_atmosphere_hlsl));
#else
				THAWK_ERROR("standard/atmosphere.hlsl was not compiled");
#endif
#ifdef HAS_D3D11_STANDARD_BASIC_HLSL
				AddSection("standard/basic.hlsl", GET_RESOURCE_BATCH(d3d11_standard_basic_hlsl));
#else
				THAWK_ERROR("standard/basic.hlsl was not compiled");
#endif
#ifdef HAS_D3D11_STANDARD_COOK_TORRANCE_HLSL
				AddSection("standard/cook-torrance.hlsl", GET_RESOURCE_BATCH(d3d11_standard_cook_torrance_hlsl));
#else
				THAWK_ERROR("standard/cook-torrance.hlsl was not compiled");
#endif
#ifdef HAS_D3D11_STANDARD_POW_HLSL
				AddSection("standard/pow.hlsl", GET_RESOURCE_BATCH(d3d11_standard_pow_hlsl));
#else
				THAWK_ERROR("standard/pow.hlsl was not compiled");
#endif
#ifdef HAS_D3D11_STANDARD_RANDOM_FLOAT_HLSL
				AddSection("standard/random-float.hlsl", GET_RESOURCE_BATCH(d3d11_standard_random_float_hlsl));
#else
				THAWK_ERROR("standard/random-float.hlsl was not compiled");
#endif
#ifdef HAS_D3D11_STANDARD_RAY_MARCH_HLSL
				AddSection("standard/ray-march.hlsl", GET_RESOURCE_BATCH(d3d11_standard_ray_march_hlsl));
#else
				THAWK_ERROR("standard/ray-march.hlsl was not compiled");
#endif
#ifdef HAS_D3D11_STANDARD_SPACE_SV_HLSL
				AddSection("standard/space-sv.hlsl", GET_RESOURCE_BATCH(d3d11_standard_space_sv_hlsl));
#else
				THAWK_ERROR("standard/space-sv.hlsl was not compiled");
#endif
#ifdef HAS_D3D11_STANDARD_SPACE_UV_HLSL
				AddSection("standard/space-uv.hlsl", GET_RESOURCE_BATCH(d3d11_standard_space_uv_hlsl));
#else
				THAWK_ERROR("standard/space-uv.hlsl was not compiled");
#endif
#ifdef HAS_D3D11_WORKFLOW_GEOMETRY_HLSL
				AddSection("workflow/geometry.hlsl", GET_RESOURCE_BATCH(d3d11_workflow_geometry_hlsl));
#else
				THAWK_ERROR("workflow/geometry.hlsl was not compiled");
#endif
#ifdef HAS_D3D11_WORKFLOW_RASTERIZER_HLSL
				AddSection("workflow/rasterizer.hlsl", GET_RESOURCE_BATCH(d3d11_workflow_rasterizer_hlsl));
#else
				THAWK_ERROR("workflow/rasterizer.hlsl was not compiled");
#endif
#ifdef HAS_D3D11_WORKFLOW_PRIMITIVE_HLSL
				AddSection("workflow/primitive.hlsl", GET_RESOURCE_BATCH(d3d11_workflow_primitive_hlsl));
#else
				THAWK_ERROR("workflow/primitive.hlsl was not compiled");
#endif
#ifdef HAS_D3D11_WORKFLOW_PASS_HLSL
				AddSection("workflow/pass.hlsl", GET_RESOURCE_BATCH(d3d11_workflow_pass_hlsl));
#else
				THAWK_ERROR("workflow/pass.hlsl was not compiled");
#endif
			}
			else if (Backend == RenderBackend_OGL)
			{
#ifdef HAS_OGL_GEOMETRY_EMITTER_DEPTH_POINT_GLSL
			AddSection("geometry/emitter/depth-point.glsl", GET_RESOURCE_BATCH(ogl_geometry_emitter_depth_point_glsl));
#else
			THAWK_ERROR("geometry/emitter/depth-point.glsl was not compiled");
#endif
#ifdef HAS_OGL_GEOMETRY_EMITTER_DEPTH_QUAD_GLSL
			AddSection("geometry/emitter/depth-quad.glsl", GET_RESOURCE_BATCH(ogl_geometry_emitter_depth_quad_glsl));
#else
			THAWK_ERROR("geometry/emitter/depth-quad.glsl was not compiled");
#endif
#ifdef HAS_OGL_GEOMETRY_EMITTER_DEPTH_LINEAR_GLSL
			AddSection("geometry/emitter/depth-linear.glsl", GET_RESOURCE_BATCH(ogl_geometry_emitter_depth_linear_glsl));
#else
			THAWK_ERROR("geometry/emitter/depth-linear.glsl was not compiled");
#endif
#ifdef HAS_OGL_GEOMETRY_EMITTER_GBUFFER_OPAQUE_GLSL
			AddSection("geometry/emitter/gbuffer-opaque.glsl", GET_RESOURCE_BATCH(ogl_geometry_emitter_gbuffer_opaque_glsl));
#else
			THAWK_ERROR("geometry/emitter/gbuffer-opaque.glsl was not compiled");
#endif
#ifdef HAS_OGL_GEOMETRY_EMITTER_GBUFFER_LIMPID_GLSL
			AddSection("geometry/emitter/gbuffer-limpid.glsl", GET_RESOURCE_BATCH(ogl_geometry_emitter_gbuffer_limpid_glsl));
#else
			THAWK_ERROR("geometry/emitter/gbuffer-limpid.glsl was not compiled");
#endif
#ifdef HAS_OGL_GEOMETRY_MODEL_OCCLUSION_GLSL
			AddSection("geometry/model/occlusion.glsl", GET_RESOURCE_BATCH(ogl_geometry_model_occlusion_glsl));
#else
			THAWK_ERROR("geometry/model/occlusion.glsl was not compiled");
#endif
#ifdef HAS_OGL_GEOMETRY_MODEL_DEPTH_CUBIC_GLSL
			AddSection("geometry/model/depth-cubic.glsl", GET_RESOURCE_BATCH(ogl_geometry_model_depth_cubic_glsl));
#else
			THAWK_ERROR("geometry/model/depth-cubic.glsl was not compiled");
#endif
#ifdef HAS_OGL_GEOMETRY_MODEL_DEPTH_LINEAR_GLSL
			AddSection("geometry/model/depth-linear.glsl", GET_RESOURCE_BATCH(ogl_geometry_model_depth_linear_glsl));
#else
			THAWK_ERROR("geometry/model/depth-linear.glsl was not compiled");
#endif
#ifdef HAS_OGL_GEOMETRY_MODEL_GBUFFER_GLSL
			AddSection("geometry/model/gbuffer.glsl", GET_RESOURCE_BATCH(ogl_geometry_model_gbuffer_glsl));
#else
			THAWK_ERROR("geometry/model/gbuffer.glsl was not compiled");
#endif
#ifdef HAS_OGL_GEOMETRY_SKIN_OCCLUSION_GLSL
			AddSection("geometry/skin/occlusion.glsl", GET_RESOURCE_BATCH(ogl_geometry_skin_occlusion_glsl));
#else
			THAWK_ERROR("geometry/skin/occlusion.glsl was not compiled");
#endif
#ifdef HAS_OGL_GEOMETRY_SKIN_DEPTH_CUBIC_GLSL
			AddSection("geometry/skin/depth-cubic.glsl", GET_RESOURCE_BATCH(ogl_geometry_skin_depth_cubic_glsl));
#else
			THAWK_ERROR("geometry/skin/depth-cubic.glsl was not compiled");
#endif
#ifdef HAS_OGL_GEOMETRY_SKIN_DEPTH_LINEAR_GLSL
			AddSection("geometry/skin/depth-linear.glsl", GET_RESOURCE_BATCH(ogl_geometry_skin_depth_linear_glsl));
#else
			THAWK_ERROR("geometry/skin/depth-linear.glsl was not compiled");
#endif
#ifdef HAS_OGL_GEOMETRY_SKIN_GBUFFER_GLSL
			AddSection("geometry/skin/gbuffer.glsl", GET_RESOURCE_BATCH(ogl_geometry_skin_gbuffer_glsl));
#else
			THAWK_ERROR("geometry/skin/gbuffer.glsl was not compiled");
#endif
#ifdef HAS_OGL_GEOMETRY_DECAL_GBUFFER_GLSL
			AddSection("geometry/decal/gbuffer.glsl", GET_RESOURCE_BATCH(ogl_geometry_decal_gbuffer_glsl));
#else
			THAWK_ERROR("geometry/decal/gbuffer.glsl was not compiled");
#endif
#ifdef HAS_OGL_GEOMETRY_LIGHT_BASE_AMBIENT_GLSL
			AddSection("geometry/light/base/ambient.glsl", GET_RESOURCE_BATCH(ogl_geometry_light_base_ambient_glsl));
#else
			THAWK_ERROR("geometry/light/base/ambient.glsl was not compiled");
#endif
#ifdef HAS_OGL_GEOMETRY_LIGHT_BASE_PROBE_GLSL
			AddSection("geometry/light/base/probe.glsl", GET_RESOURCE_BATCH(ogl_geometry_light_base_probe_glsl));
#else
			THAWK_ERROR("geometry/light/base/probe.glsl was not compiled");
#endif
#ifdef HAS_OGL_GEOMETRY_LIGHT_BASE_LINE_GLSL
			AddSection("geometry/light/base/line.glsl", GET_RESOURCE_BATCH(ogl_geometry_light_base_line_glsl));
#else
			THAWK_ERROR("geometry/light/base/line.glsl was not compiled");
#endif
#ifdef HAS_OGL_GEOMETRY_LIGHT_BASE_POINT_GLSL
			AddSection("geometry/light/base/point.glsl", GET_RESOURCE_BATCH(ogl_geometry_light_base_point_glsl));
#else
			THAWK_ERROR("geometry/light/base/point.glsl was not compiled");
#endif
#ifdef HAS_OGL_GEOMETRY_LIGHT_BASE_SPOT_GLSL
			AddSection("geometry/light/base/spot.glsl", GET_RESOURCE_BATCH(ogl_geometry_light_base_spot_glsl));
#else
			THAWK_ERROR("geometry/light/base/spot.glsl was not compiled");
#endif
#ifdef HAS_OGL_GEOMETRY_LIGHT_SHADE_LINE_GLSL
			AddSection("geometry/light/shade/line.glsl", GET_RESOURCE_BATCH(ogl_geometry_light_shade_line_glsl));
#else
			THAWK_ERROR("geometry/light/shade/line.glsl was not compiled");
#endif
#ifdef HAS_OGL_GEOMETRY_LIGHT_SHADE_POINT_GLSL
			AddSection("geometry/light/shade/point.glsl", GET_RESOURCE_BATCH(ogl_geometry_light_shade_point_glsl));
#else
			THAWK_ERROR("geometry/light/shade/point.glsl was not compiled");
#endif
#ifdef HAS_OGL_GEOMETRY_LIGHT_SHADE_SPOT_GLSL
			AddSection("geometry/light/shade/spot.glsl", GET_RESOURCE_BATCH(ogl_geometry_light_shade_spot_glsl));
#else
			THAWK_ERROR("geometry/light/shade/spot.glsl was not compiled");
#endif
#ifdef HAS_OGL_PASS_LIMPID_GLSL
			AddSection("pass/limpid.glsl", GET_RESOURCE_BATCH(ogl_pass_limpid_glsl));
#else
			THAWK_ERROR("pass/limpid.glsl was not compiled");
#endif
#ifdef HAS_OGL_PASS_AMBIENT_GLSL
			AddSection("pass/ambient.glsl", GET_RESOURCE_BATCH(ogl_pass_ambient_glsl));
#else
			THAWK_ERROR("pass/ambient.glsl was not compiled");
#endif
#ifdef HAS_OGL_PASS_BLOOM_GLSL
			AddSection("pass/bloom.glsl", GET_RESOURCE_BATCH(ogl_pass_bloom_glsl));
#else
			THAWK_ERROR("pass/bloom.glsl was not compiled");
#endif
#ifdef HAS_OGL_PASS_BLUR_GLSL
			AddSection("pass/blur.glsl", GET_RESOURCE_BATCH(ogl_pass_blur_glsl));
#else
			THAWK_ERROR("pass/blur.glsl was not compiled");
#endif
#ifdef HAS_OGL_PASS_FOCUS_GLSL
			AddSection("pass/focus.glsl", GET_RESOURCE_BATCH(ogl_pass_focus_glsl));
#else
			THAWK_ERROR("pass/focus.glsl was not compiled");
#endif
#ifdef HAS_OGL_PASS_GLITCH_GLSL
			AddSection("pass/glitch.glsl", GET_RESOURCE_BATCH(ogl_pass_glitch_glsl));
#else
			THAWK_ERROR("pass/glitch.glsl was not compiled");
#endif
#ifdef HAS_OGL_PASS_GUI_GLSL
			AddSection("pass/gui.glsl", GET_RESOURCE_BATCH(ogl_pass_gui_glsl));
#else
			THAWK_ERROR("pass/gui.glsl was not compiled");
#endif
#ifdef HAS_OGL_PASS_INDIRECT_GLSL
			AddSection("pass/indirect.glsl", GET_RESOURCE_BATCH(ogl_pass_indirect_glsl));
#else
			THAWK_ERROR("pass/indirect.glsl was not compiled");
#endif
#ifdef HAS_OGL_PASS_GLOSS_GLSL
			AddSection("pass/gloss.glsl", GET_RESOURCE_BATCH(ogl_pass_gloss_glsl));
#else
			THAWK_ERROR("pass/gloss.glsl was not compiled");
#endif
#ifdef HAS_OGL_PASS_REFLECTION_GLSL
			AddSection("pass/reflection.glsl", GET_RESOURCE_BATCH(ogl_pass_reflection_glsl));
#else
			THAWK_ERROR("pass/reflection.glsl was not compiled");
#endif
#ifdef HAS_OGL_PASS_TONE_GLSL
			AddSection("pass/tone.glsl", GET_RESOURCE_BATCH(ogl_pass_tone_glsl));
#else
			THAWK_ERROR("pass/tone.glsl was not compiled");
#endif
#ifdef HAS_OGL_RENDERER_BUFFER_ANIMATION_GLSL
			AddSection("renderer/buffer/animation.glsl", GET_RESOURCE_BATCH(ogl_renderer_buffer_animation_glsl));
#else
			THAWK_ERROR("renderer/buffer/animation.glsl was not compiled");
#endif
#ifdef HAS_OGL_RENDERER_BUFFER_CUBIC_GLSL
			AddSection("renderer/buffer/cubic.glsl", GET_RESOURCE_BATCH(ogl_renderer_buffer_cubic_glsl));
#else
			THAWK_ERROR("renderer/buffer/cubic.glsl was not compiled");
#endif
#ifdef HAS_OGL_RENDERER_BUFFER_GUI_GLSL
			AddSection("renderer/buffer/gui.glsl", GET_RESOURCE_BATCH(ogl_renderer_buffer_gui_glsl));
#else
			THAWK_ERROR("renderer/buffer/gui.glsl was not compiled");
#endif
#ifdef HAS_OGL_RENDERER_BUFFER_OBJECT_GLSL
			AddSection("renderer/buffer/object.glsl", GET_RESOURCE_BATCH(ogl_renderer_buffer_object_glsl));
#else
			THAWK_ERROR("renderer/buffer/object.glsl was not compiled");
#endif
#ifdef HAS_OGL_RENDERER_BUFFER_VIEWER_GLSL
			AddSection("renderer/buffer/viewer.glsl", GET_RESOURCE_BATCH(ogl_renderer_buffer_viewer_glsl));
#else
			THAWK_ERROR("renderer/buffer/viewer.glsl was not compiled");
#endif
#ifdef HAS_OGL_RENDERER_INPUT_BASE_GLSL
			AddSection("renderer/input/base.glsl", GET_RESOURCE_BATCH(ogl_renderer_input_base_glsl));
#else
			THAWK_ERROR("renderer/input/base.glsl was not compiled");
#endif
#ifdef HAS_OGL_RENDERER_INPUT_ELEMENT_GLSL
			AddSection("renderer/input/element.glsl", GET_RESOURCE_BATCH(ogl_renderer_input_element_glsl));
#else
			THAWK_ERROR("renderer/input/element.glsl was not compiled");
#endif
#ifdef HAS_OGL_RENDERER_INPUT_SV_GLSL
			AddSection("renderer/input/sv.glsl", GET_RESOURCE_BATCH(ogl_renderer_input_sv_glsl));
#else
			THAWK_ERROR("renderer/input/sv.glsl was not compiled");
#endif
#ifdef HAS_OGL_RENDERER_VERTEX_ELEMENT_GLSL
			AddSection("renderer/vertex/element.glsl", GET_RESOURCE_BATCH(ogl_renderer_vertex_element_glsl));
#else
			THAWK_ERROR("renderer/vertex/element.glsl was not compiled");
#endif
#ifdef HAS_OGL_RENDERER_VERTEX_GUI_GLSL
			AddSection("renderer/vertex/gui.glsl", GET_RESOURCE_BATCH(ogl_renderer_vertex_gui_glsl));
#else
			THAWK_ERROR("renderer/vertex/gui.glsl was not compiled");
#endif
#ifdef HAS_OGL_RENDERER_VERTEX_SHAPE_GLSL
			AddSection("renderer/vertex/shape.glsl", GET_RESOURCE_BATCH(ogl_renderer_vertex_shape_glsl));
#else
			THAWK_ERROR("renderer/vertex/shape.glsl was not compiled");
#endif
#ifdef HAS_OGL_RENDERER_VERTEX_SKIN_GLSL
			AddSection("renderer/vertex/skin.glsl", GET_RESOURCE_BATCH(ogl_renderer_vertex_skin_glsl));
#else
			THAWK_ERROR("renderer/vertex/skin.glsl was not compiled");
#endif
#ifdef HAS_OGL_RENDERER_ELEMENT_GLSL
			AddSection("renderer/element.glsl", GET_RESOURCE_BATCH(ogl_renderer_element_glsl));
#else
			THAWK_ERROR("renderer/element.glsl was not compiled");
#endif
#ifdef HAS_OGL_RENDERER_FRAGMENT_GLSL
			AddSection("renderer/fragment.glsl", GET_RESOURCE_BATCH(ogl_renderer_fragment_glsl));
#else
			THAWK_ERROR("renderer/fragment.glsl was not compiled");
#endif
#ifdef HAS_OGL_RENDERER_GBUFFER_GLSL
			AddSection("renderer/gbuffer.glsl", GET_RESOURCE_BATCH(ogl_renderer_gbuffer_glsl));
#else
			THAWK_ERROR("renderer/gbuffer.glsl was not compiled");
#endif
#ifdef HAS_OGL_RENDERER_MATERIAL_GLSL
			AddSection("renderer/material.glsl", GET_RESOURCE_BATCH(ogl_renderer_material_glsl));
#else
			THAWK_ERROR("renderer/material.glsl was not compiled");
#endif
#ifdef HAS_OGL_RENDERER_VERTEX_GLSL
			AddSection("renderer/vertex.glsl", GET_RESOURCE_BATCH(ogl_renderer_vertex_glsl));
#else
			THAWK_ERROR("renderer/vertex.glsl was not compiled");
#endif
#ifdef HAS_OGL_STANDARD_COMPRESS_GLSL
			AddSection("standard/compress.glsl", GET_RESOURCE_BATCH(ogl_standard_compress_glsl));
#else
			THAWK_ERROR("standard/compress.glsl was not compiled");
#endif
#ifdef HAS_OGL_STANDARD_ATMOSPHERE_GLSL
			AddSection("standard/atmosphere.glsl", GET_RESOURCE_BATCH(ogl_standard_atmosphere_glsl));
#else
			THAWK_ERROR("standard/atmosphere.glsl was not compiled");
#endif
#ifdef HAS_OGL_STANDARD_BASIC_GLSL
			AddSection("standard/basic.glsl", GET_RESOURCE_BATCH(ogl_standard_basic_glsl));
#else
			THAWK_ERROR("standard/basic.glsl was not compiled");
#endif
#ifdef HAS_OGL_STANDARD_COOK_TORRANCE_GLSL
			AddSection("standard/cook-torrance.glsl", GET_RESOURCE_BATCH(ogl_standard_cook_torrance_glsl));
#else
			THAWK_ERROR("standard/cook-torrance.glsl was not compiled");
#endif
#ifdef HAS_OGL_STANDARD_POW_GLSL
			AddSection("standard/pow.glsl", GET_RESOURCE_BATCH(ogl_standard_pow_glsl));
#else
			THAWK_ERROR("standard/pow.glsl was not compiled");
#endif
#ifdef HAS_OGL_STANDARD_RANDOM_FLOAT_GLSL
			AddSection("standard/random-float.glsl", GET_RESOURCE_BATCH(ogl_standard_random_float_glsl));
#else
			THAWK_ERROR("standard/random-float.glsl was not compiled");
#endif
#ifdef HAS_OGL_STANDARD_RAY_MARCH_GLSL
			AddSection("standard/ray-march.glsl", GET_RESOURCE_BATCH(ogl_standard_ray_march_glsl));
#else
			THAWK_ERROR("standard/ray-march.glsl was not compiled");
#endif
#ifdef HAS_OGL_STANDARD_SPACE_SV_GLSL
			AddSection("standard/space-sv.glsl", GET_RESOURCE_BATCH(ogl_standard_space_sv_glsl));
#else
			THAWK_ERROR("standard/space-sv.glsl was not compiled");
#endif
#ifdef HAS_OGL_STANDARD_SPACE_UV_GLSL
			AddSection("standard/space-uv.glsl", GET_RESOURCE_BATCH(ogl_standard_space_uv_glsl));
#else
			THAWK_ERROR("standard/space-uv.glsl was not compiled");
#endif
#ifdef HAS_OGL_WORKFLOW_GEOMETRY_GLSL
			AddSection("workflow/geometry.glsl", GET_RESOURCE_BATCH(ogl_workflow_geometry_glsl));
#else
			THAWK_ERROR("workflow/geometry.glsl was not compiled");
#endif
#ifdef HAS_OGL_WORKFLOW_RASTERIZER_GLSL
			AddSection("workflow/rasterizer.glsl", GET_RESOURCE_BATCH(ogl_workflow_rasterizer_glsl));
#else
			THAWK_ERROR("workflow/rasterizer.glsl was not compiled");
#endif
#ifdef HAS_OGL_WORKFLOW_PRIMITIVE_GLSL
			AddSection("workflow/primitive.glsl", GET_RESOURCE_BATCH(ogl_workflow_primitive_glsl));
#else
			THAWK_ERROR("workflow/primitive.glsl was not compiled");
#endif
#ifdef HAS_OGL_WORKFLOW_PASS_GLSL
			AddSection("workflow/pass.glsl", GET_RESOURCE_BATCH(ogl_workflow_pass_glsl));
#else
			THAWK_ERROR("workflow/pass.glsl was not compiled");
#endif
			}
		}
		unsigned int GraphicsDevice::GetMipLevel(unsigned int Width, unsigned int Height)
		{
			unsigned int MipLevels = 1;

			while (Width > 1 && Height > 1)
			{
				Width = (unsigned int)Compute::Mathf::Max((float)Width / 2.0f, 1.0f);
				Height = (unsigned int)Compute::Mathf::Max((float)Height / 2.0f, 1.0f);
				MipLevels++;
			}

			return MipLevels;
		}
		ShaderModel GraphicsDevice::GetShaderModel()
		{
			return ShaderModelType;
		}
		DepthStencilState* GraphicsDevice::GetDepthStencilState(const std::string& Name)
		{
			auto It = DepthStencilStates.find(Name);
			if (It != DepthStencilStates.end())
				return It->second;

			return nullptr;
		}
		BlendState* GraphicsDevice::GetBlendState(const std::string& Name)
		{
			auto It = BlendStates.find(Name);
			if (It != BlendStates.end())
				return It->second;

			return nullptr;
		}
		RasterizerState* GraphicsDevice::GetRasterizerState(const std::string& Name)
		{
			auto It = RasterizerStates.find(Name);
			if (It != RasterizerStates.end())
				return It->second;

			return nullptr;
		}
		SamplerState* GraphicsDevice::GetSamplerState(const std::string& Name)
		{
			auto It = SamplerStates.find(Name);
			if (It != SamplerStates.end())
				return It->second;

			return nullptr;
		}
		RenderTarget2D* GraphicsDevice::GetRenderTarget()
		{
			return RenderTarget;
		}
		Shader* GraphicsDevice::GetBasicEffect()
		{
			return BasicEffect;
		}
		RenderBackend GraphicsDevice::GetBackend()
		{
			return Backend;
		}
		unsigned int GraphicsDevice::GetPresentFlags()
		{
			return PresentFlags;
		}
		unsigned int GraphicsDevice::GetCompileFlags()
		{
			return CompileFlags;
		}
		bool GraphicsDevice::GetSection(const std::string& Name, std::string* Out, bool Internal)
		{
			if (Name.empty() || Sections.empty())
			{
				if (Internal)
					return false;

				THAWK_ERROR("\n\tcould not find shader: \"%s\"");
				return false;
			}

			auto It = Sections.find(Name);
			if (It != Sections.end())
			{
				if (Out != nullptr)
					Out->assign(It->second->Code);

				return true;
			}

			It = Sections.find(Name + ".hlsl");
			if (It != Sections.end())
			{
				if (Out != nullptr)
					Out->assign(It->second->Code);

				return true;
			}

			It = Sections.find(Name + ".glsl");
			if (It != Sections.end())
			{
				if (Out != nullptr)
					Out->assign(It->second->Code);

				return true;
			}

			if (Internal)
				return false;

			THAWK_ERROR("\n\tcould not find shader: \"%s\"", Name.c_str());
			return false;
		}
		VSync GraphicsDevice::GetVSyncMode()
		{
			return VSyncMode;
		}
		bool GraphicsDevice::IsDebug()
		{
			return Debug;
		}
		void GraphicsDevice::FreeProxy()
		{
			for (auto It = DepthStencilStates.begin(); It != DepthStencilStates.end(); It++)
				delete It->second;
			DepthStencilStates.clear();

			for (auto It = RasterizerStates.begin(); It != RasterizerStates.end(); It++)
				delete It->second;
			RasterizerStates.clear();

			for (auto It = BlendStates.begin(); It != BlendStates.end(); It++)
				delete It->second;
			BlendStates.clear();

			for (auto It = SamplerStates.begin(); It != SamplerStates.end(); It++)
				delete It->second;
			SamplerStates.clear();

			delete RenderTarget;
			RenderTarget = nullptr;

			delete BasicEffect;
			BasicEffect = nullptr;
		}
		GraphicsDevice* GraphicsDevice::Create(const Desc& I)
		{
#ifdef THAWK_MICROSOFT
			if (I.Backend == RenderBackend_D3D11)
				return new D3D11::D3D11Device(I);
#endif
#ifdef THAWK_HAS_GL
			if (I.Backend == RenderBackend_OGL)
				return new OGL::OGLDevice(I);
#endif
			THAWK_ERROR("backend was not found");
			return nullptr;
		}

		Activity::Activity(const Desc& I) : Handle(nullptr), Rest(I), Command(0), Message(this)
		{
#ifdef THAWK_HAS_SDL2
			Cursors[DisplayCursor_Arrow] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW);
			Cursors[DisplayCursor_TextInput] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_IBEAM);
			Cursors[DisplayCursor_ResizeAll] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEALL);
			Cursors[DisplayCursor_ResizeNS] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENS);
			Cursors[DisplayCursor_ResizeEW] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEWE);
			Cursors[DisplayCursor_ResizeNESW] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENESW);
			Cursors[DisplayCursor_ResizeNWSE] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENWSE);
			Cursors[DisplayCursor_Hand] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_HAND);
#endif
			memset(Keys[0], 0, 1024 * sizeof(bool));
			memset(Keys[1], 0, 1024 * sizeof(bool));

			Reset();
		}
		Activity::~Activity()
		{
#ifdef THAWK_HAS_SDL2
			for (int i = 0; i < DisplayCursor_Count; i++)
				SDL_FreeCursor(Cursors[i]);

			if (Handle != nullptr)
			{
				SDL_DestroyWindow(Handle);
				Handle = nullptr;
			}
#endif
		}
		void Activity::SetClipboardText(const std::string& Text)
		{
#ifdef THAWK_HAS_SDL2
			SDL_SetClipboardText(Text.c_str());
#endif
		}
		void Activity::SetCursorPosition(const Compute::Vector2& Position)
		{
#ifdef THAWK_HAS_SDL2
#if SDL_VERSION_ATLEAST(2, 0, 4)
			SDL_WarpMouseInWindow(Handle, (int)Position.X, (int)Position.Y);
#endif
#endif
		}
		void Activity::SetCursorPosition(float X, float Y)
		{
#ifdef THAWK_HAS_SDL2
#if SDL_VERSION_ATLEAST(2, 0, 4)
			SDL_WarpMouseInWindow(Handle, (int)X, (int)Y);
#endif
#endif
		}
		void Activity::SetGlobalCursorPosition(const Compute::Vector2& Position)
		{
#ifdef THAWK_HAS_SDL2
#if SDL_VERSION_ATLEAST(2, 0, 4)
			SDL_WarpMouseGlobal((int)Position.X, (int)Position.Y);
#endif
#endif
		}
		void Activity::SetGlobalCursorPosition(float X, float Y)
		{
#ifdef THAWK_HAS_SDL2
#if SDL_VERSION_ATLEAST(2, 0, 4)
			SDL_WarpMouseGlobal((int)X, (int)Y);
#endif
#endif
		}
		void Activity::SetKey(KeyCode Key, bool Value)
		{
			Keys[0][Key] = Value;
		}
		void Activity::SetCursor(DisplayCursor Style)
		{
#ifdef THAWK_HAS_SDL2
			SDL_SetCursor(Cursors[Style]);
#endif
		}
		void Activity::Cursor(bool Enabled)
		{
#ifdef THAWK_HAS_SDL2
			SDL_ShowCursor(Enabled);
#endif
		}
		void Activity::Reset()
		{
#ifdef THAWK_HAS_SDL2
			if (Handle != nullptr)
			{
				SDL_DestroyWindow(Handle);
				Handle = nullptr;
			}

			Uint32 Flags = 0;
			if (Rest.Fullscreen)
				Flags |= SDL_WINDOW_FULLSCREEN;

			if (Rest.Hidden)
				Flags |= SDL_WINDOW_HIDDEN;

			if (Rest.Borderless)
				Flags |= SDL_WINDOW_BORDERLESS;

			if (Rest.Resizable)
				Flags |= SDL_WINDOW_RESIZABLE;

			if (Rest.Minimized)
				Flags |= SDL_WINDOW_MINIMIZED;

			if (Rest.Maximized)
				Flags |= SDL_WINDOW_MAXIMIZED;

			if (Rest.Focused)
				Flags |= SDL_WINDOW_INPUT_GRABBED;

			if (Rest.AllowHighDPI)
				Flags |= SDL_WINDOW_ALLOW_HIGHDPI;

			if (Rest.Backend == RenderBackend_OGL)
			{
				SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
				SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
				SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
				Flags |= SDL_WINDOW_OPENGL;
			}

			if (Rest.Centered)
			{
				Rest.X = SDL_WINDOWPOS_CENTERED;
				Rest.Y = SDL_WINDOWPOS_CENTERED;
			}
			else if (Rest.FreePosition)
			{
				Rest.X = SDL_WINDOWPOS_UNDEFINED;
				Rest.Y = SDL_WINDOWPOS_UNDEFINED;
			}

			Handle = SDL_CreateWindow(Rest.Title, Rest.X, Rest.Y, Rest.Width, Rest.Height, Flags);
#endif
		}
		void Activity::Grab(bool Enabled)
		{
#ifdef THAWK_HAS_SDL2
			SDL_SetWindowGrab(Handle, Enabled ? SDL_TRUE : SDL_FALSE);
#endif
		}
		void Activity::Hide()
		{
#ifdef THAWK_HAS_SDL2
			SDL_HideWindow(Handle);
#endif
		}
		void Activity::Show()
		{
#ifdef THAWK_HAS_SDL2
			SDL_ShowWindow(Handle);
#endif
		}
		void Activity::Maximize()
		{
#ifdef THAWK_HAS_SDL2
			SDL_MaximizeWindow(Handle);
#endif
		}
		void Activity::Minimize()
		{
#ifdef THAWK_HAS_SDL2
			SDL_MinimizeWindow(Handle);
#endif
		}
		void Activity::Fullscreen(bool Enabled)
		{
#ifdef THAWK_HAS_SDL2
			SDL_SetWindowFullscreen(Handle, Enabled ? SDL_WINDOW_FULLSCREEN : 0);
#endif
		}
		void Activity::Borderless(bool Enabled)
		{
#ifdef THAWK_HAS_SDL2
			SDL_SetWindowBordered(Handle, Enabled ? SDL_TRUE : SDL_FALSE);
#endif
		}
		void Activity::Focus()
		{
#ifdef THAWK_HAS_SDL2
#if SDL_VERSION_ATLEAST(2, 0, 5)
			SDL_SetWindowInputFocus(Handle);
#endif
#endif
		}
		void Activity::Move(int X, int Y)
		{
#ifdef THAWK_HAS_SDL2
			SDL_SetWindowPosition(Handle, X, Y);
#endif
		}
		void Activity::Resize(int W, int H)
		{
#ifdef THAWK_HAS_SDL2
			SDL_SetWindowSize(Handle, W, H);
#endif
		}
		void Activity::Title(const char* Value)
		{
#ifdef THAWK_HAS_SDL2
			if (Value != nullptr)
				SDL_SetWindowTitle(Handle, Value);
#endif
		}
		void Activity::Icon(Surface* Icon)
		{
#ifdef THAWK_HAS_SDL2
			SDL_SetWindowIcon(Handle, (SDL_Surface*)Icon->GetResource());
#endif
		}
		void Activity::Load(SDL_SysWMinfo* Base)
		{
#ifdef THAWK_HAS_SDL2
			if (Base != nullptr)
			{
				SDL_VERSION(&Base->version);
				SDL_GetWindowWMInfo(Handle, Base);
			}
#endif
		}
		bool Activity::Dispatch()
		{
			memcpy((void*)Keys[1], (void*)Keys[0], 1024);
#ifdef THAWK_HAS_SDL2
			Command = (int)SDL_GetModState();
			Message.Dispatch();

			SDL_Event Event;
			if (!Handle || !SDL_PollEvent(&Event))
				return false;

			int Id = SDL_GetWindowID(Handle);
			switch (Event.type)
			{
				case SDL_WINDOWEVENT:
					switch (Event.window.event)
					{
						case SDL_WINDOWEVENT_SHOWN:
							if (Callbacks.WindowStateChange && Id == Event.window.windowID)
								Callbacks.WindowStateChange(WindowState_Show, 0, 0);
							break;
						case SDL_WINDOWEVENT_HIDDEN:
							if (Callbacks.WindowStateChange && Id == Event.window.windowID)
								Callbacks.WindowStateChange(WindowState_Hide, 0, 0);
							break;
						case SDL_WINDOWEVENT_EXPOSED:
							if (Callbacks.WindowStateChange && Id == Event.window.windowID)
								Callbacks.WindowStateChange(WindowState_Expose, 0, 0);
							break;
						case SDL_WINDOWEVENT_MOVED:
							if (Callbacks.WindowStateChange && Id == Event.window.windowID)
								Callbacks.WindowStateChange(WindowState_Move, Event.window.data1, Event.window.data2);
							break;
						case SDL_WINDOWEVENT_RESIZED:
							if (Callbacks.WindowStateChange && Id == Event.window.windowID)
								Callbacks.WindowStateChange(WindowState_Resize, Event.window.data1, Event.window.data2);
							break;
						case SDL_WINDOWEVENT_SIZE_CHANGED:
							if (Callbacks.WindowStateChange && Id == Event.window.windowID)
								Callbacks.WindowStateChange(WindowState_Size_Change, Event.window.data1, Event.window.data2);
							break;
						case SDL_WINDOWEVENT_MINIMIZED:
							if (Callbacks.WindowStateChange && Id == Event.window.windowID)
								Callbacks.WindowStateChange(WindowState_Minimize, 0, 0);
							break;
						case SDL_WINDOWEVENT_MAXIMIZED:
							if (Callbacks.WindowStateChange && Id == Event.window.windowID)
								Callbacks.WindowStateChange(WindowState_Maximize, 0, 0);
							break;
						case SDL_WINDOWEVENT_RESTORED:
							if (Callbacks.WindowStateChange && Id == Event.window.windowID)
								Callbacks.WindowStateChange(WindowState_Restore, 0, 0);
							break;
						case SDL_WINDOWEVENT_ENTER:
							if (Callbacks.WindowStateChange && Id == Event.window.windowID)
								Callbacks.WindowStateChange(WindowState_Enter, 0, 0);
							break;
						case SDL_WINDOWEVENT_LEAVE:
							if (Callbacks.WindowStateChange && Id == Event.window.windowID)
								Callbacks.WindowStateChange(WindowState_Leave, 0, 0);
							break;
#if SDL_VERSION_ATLEAST(2, 0, 5)
						case SDL_WINDOWEVENT_TAKE_FOCUS:
#endif
						case SDL_WINDOWEVENT_FOCUS_GAINED:
							if (Callbacks.WindowStateChange && Id == Event.window.windowID)
								Callbacks.WindowStateChange(WindowState_Focus, 0, 0);
							break;
						case SDL_WINDOWEVENT_FOCUS_LOST:
							if (Callbacks.WindowStateChange && Id == Event.window.windowID)
								Callbacks.WindowStateChange(WindowState_Blur, 0, 0);
							break;
						case SDL_WINDOWEVENT_CLOSE:
							if (Callbacks.WindowStateChange && Id == Event.window.windowID)
								Callbacks.WindowStateChange(WindowState_Close, 0, 0);
							break;
					}
					return true;
				case SDL_QUIT:
					if (Callbacks.AppStateChange)
						Callbacks.AppStateChange(AppState_Close_Window);
					return true;
				case SDL_APP_TERMINATING:
					if (Callbacks.AppStateChange)
						Callbacks.AppStateChange(AppState_Terminating);
					return true;
				case SDL_APP_LOWMEMORY:
					if (Callbacks.AppStateChange)
						Callbacks.AppStateChange(AppState_Low_Memory);
					return true;
				case SDL_APP_WILLENTERBACKGROUND:
					if (Callbacks.AppStateChange)
						Callbacks.AppStateChange(AppState_Enter_Background_Start);
					return true;
				case SDL_APP_DIDENTERBACKGROUND:
					if (Callbacks.AppStateChange)
						Callbacks.AppStateChange(AppState_Enter_Background_End);
					return true;
				case SDL_APP_WILLENTERFOREGROUND:
					if (Callbacks.AppStateChange)
						Callbacks.AppStateChange(AppState_Enter_Foreground_Start);
					return true;
				case SDL_APP_DIDENTERFOREGROUND:
					if (Callbacks.AppStateChange)
						Callbacks.AppStateChange(AppState_Enter_Foreground_End);
					return true;
				case SDL_KEYDOWN:
					if (Callbacks.KeyState && Id == Event.window.windowID)
						Callbacks.KeyState((KeyCode)Event.key.keysym.scancode, (KeyMod)Event.key.keysym.mod, (int)Event.key.keysym.sym, (int)(Event.key.repeat != 0), true);

					if (Mapping.Enabled && !Mapping.Mapped)
					{
						Mapping.Key.Key = (KeyCode)Event.key.keysym.scancode;
						Mapping.Key.Mod = (KeyMod)Event.key.keysym.mod;
						Mapping.Mapped = true;
						Mapping.Captured = false;
					}

					return true;
				case SDL_KEYUP:
					if (Callbacks.KeyState && Id == Event.window.windowID)
						Callbacks.KeyState((KeyCode)Event.key.keysym.scancode, (KeyMod)Event.key.keysym.mod, (int)Event.key.keysym.sym, (int)(Event.key.repeat != 0), false);

					if (Mapping.Enabled && Mapping.Mapped && Mapping.Key.Key == (KeyCode)Event.key.keysym.scancode)
						Mapping.Captured = true;

					return true;
				case SDL_TEXTINPUT:
					if (Callbacks.Input && Id == Event.window.windowID)
						Callbacks.Input((char*)Event.text.text, (int)strlen(Event.text.text));
					return true;
				case SDL_TEXTEDITING:
					if (Callbacks.InputEdit && Id == Event.window.windowID)
						Callbacks.InputEdit((char*)Event.edit.text, (int)Event.edit.start, (int)Event.edit.length);
					return true;
				case SDL_MOUSEMOTION:
					if (Id == Event.window.windowID)
					{
						CX = Event.motion.x;
						CY = Event.motion.y;
						if (Callbacks.CursorMove)
							Callbacks.CursorMove(CX, CY, (int)Event.motion.xrel, (int)Event.motion.yrel);
					}
					return true;
				case SDL_MOUSEBUTTONDOWN:
					switch (Event.button.button)
					{
						case SDL_BUTTON_LEFT:
							if (Callbacks.KeyState && Id == Event.window.windowID)
								Callbacks.KeyState(KeyCode_CURSORLEFT, (KeyMod)SDL_GetModState(), (int)KeyCode_CURSORLEFT, (int)Event.button.clicks, true);

							if (Mapping.Enabled && !Mapping.Mapped)
							{
								Mapping.Key.Key = KeyCode_CURSORLEFT;
								Mapping.Key.Mod = (KeyMod)SDL_GetModState();
								Mapping.Mapped = true;
								Mapping.Captured = false;
							}

							return true;
						case SDL_BUTTON_MIDDLE:
							if (Callbacks.KeyState && Id == Event.window.windowID)
								Callbacks.KeyState(KeyCode_CURSORMIDDLE, (KeyMod)SDL_GetModState(), (int)KeyCode_CURSORMIDDLE, (int)Event.button.clicks, true);

							if (Mapping.Enabled && !Mapping.Mapped)
							{
								Mapping.Key.Key = KeyCode_CURSORMIDDLE;
								Mapping.Key.Mod = (KeyMod)SDL_GetModState();
								Mapping.Mapped = true;
								Mapping.Captured = false;
							}

							return true;
						case SDL_BUTTON_RIGHT:
							if (Callbacks.KeyState && Id == Event.window.windowID)
								Callbacks.KeyState(KeyCode_CURSORRIGHT, (KeyMod)SDL_GetModState(), (int)KeyCode_CURSORRIGHT, (int)Event.button.clicks, true);

							if (Mapping.Enabled && !Mapping.Mapped)
							{
								Mapping.Key.Key = KeyCode_CURSORRIGHT;
								Mapping.Key.Mod = (KeyMod)SDL_GetModState();
								Mapping.Mapped = true;
								Mapping.Captured = false;
							}

							return true;
						case SDL_BUTTON_X1:
							if (Callbacks.KeyState && Id == Event.window.windowID)
								Callbacks.KeyState(KeyCode_CURSORX1, (KeyMod)SDL_GetModState(), (int)KeyCode_CURSORX1, (int)Event.button.clicks, true);

							if (Mapping.Enabled && !Mapping.Mapped)
							{
								Mapping.Key.Key = KeyCode_CURSORX1;
								Mapping.Key.Mod = (KeyMod)SDL_GetModState();
								Mapping.Mapped = true;
								Mapping.Captured = false;
							}

							return true;
						case SDL_BUTTON_X2:
							if (Callbacks.KeyState && Id == Event.window.windowID)
								Callbacks.KeyState(KeyCode_CURSORX2, (KeyMod)SDL_GetModState(), (int)KeyCode_CURSORX2, (int)Event.button.clicks, true);

							if (Mapping.Enabled && !Mapping.Mapped)
							{
								Mapping.Key.Key = KeyCode_CURSORX2;
								Mapping.Key.Mod = (KeyMod)SDL_GetModState();
								Mapping.Mapped = true;
								Mapping.Captured = false;
							}

							return true;
					}
					return true;
				case SDL_MOUSEBUTTONUP:
					switch (Event.button.button)
					{
						case SDL_BUTTON_LEFT:
							if (Callbacks.KeyState && Id == Event.window.windowID)
								Callbacks.KeyState(KeyCode_CURSORLEFT, (KeyMod)SDL_GetModState(), (int)KeyCode_CURSORLEFT, (int)Event.button.clicks, false);

							if (Mapping.Enabled && Mapping.Mapped && Mapping.Key.Key == KeyCode_CURSORLEFT)
								Mapping.Captured = true;

							return true;
						case SDL_BUTTON_MIDDLE:
							if (Callbacks.KeyState && Id == Event.window.windowID)
								Callbacks.KeyState(KeyCode_CURSORMIDDLE, (KeyMod)SDL_GetModState(), (int)KeyCode_CURSORMIDDLE, (int)Event.button.clicks, false);

							if (Mapping.Enabled && Mapping.Mapped && Mapping.Key.Key == KeyCode_CURSORMIDDLE)
								Mapping.Captured = true;

							return true;
						case SDL_BUTTON_RIGHT:
							if (Callbacks.KeyState && Id == Event.window.windowID)
								Callbacks.KeyState(KeyCode_CURSORRIGHT, (KeyMod)SDL_GetModState(), (int)KeyCode_CURSORRIGHT, (int)Event.button.clicks, false);

							if (Mapping.Enabled && Mapping.Mapped && Mapping.Key.Key == KeyCode_CURSORRIGHT)
								Mapping.Captured = true;

							return true;
						case SDL_BUTTON_X1:
							if (Callbacks.KeyState && Id == Event.window.windowID)
								Callbacks.KeyState(KeyCode_CURSORX1, (KeyMod)SDL_GetModState(), (int)KeyCode_CURSORX1, (int)Event.button.clicks, false);

							if (Mapping.Enabled && Mapping.Mapped && Mapping.Key.Key == KeyCode_CURSORX1)
								Mapping.Captured = true;

							return true;
						case SDL_BUTTON_X2:
							if (Callbacks.KeyState && Id == Event.window.windowID)
								Callbacks.KeyState(KeyCode_CURSORX2, (KeyMod)SDL_GetModState(), (int)KeyCode_CURSORX2, (int)Event.button.clicks, false);

							if (Mapping.Enabled && Mapping.Mapped && Mapping.Key.Key == KeyCode_CURSORX2)
								Mapping.Captured = true;

							return true;
					}
					return true;
				case SDL_MOUSEWHEEL:
#if SDL_VERSION_ATLEAST(2, 0, 4)
					if (Callbacks.CursorWheelState && Id == Event.window.windowID)
						Callbacks.CursorWheelState((int)Event.wheel.x, (int)Event.wheel.y, Event.wheel.direction & SDL_MOUSEWHEEL_NORMAL);
#else
				if (Callbacks.CursorWheelState && Id == Event.window.windowID)
					Callbacks.CursorWheelState((int)Event.wheel.x, (int)Event.wheel.y, 0);
#endif
					return true;
				case SDL_JOYAXISMOTION:
					if (Callbacks.JoyStickAxisMove && Id == Event.window.windowID)
						Callbacks.JoyStickAxisMove((int)Event.jaxis.which, (int)Event.jaxis.axis, (int)Event.jaxis.value);
					return true;
				case SDL_JOYBALLMOTION:
					if (Callbacks.JoyStickBallMove && Id == Event.window.windowID)
						Callbacks.JoyStickBallMove((int)Event.jball.which, (int)Event.jball.ball, (int)Event.jball.xrel, (int)Event.jball.yrel);
					return true;
				case SDL_JOYHATMOTION:
					if (Callbacks.JoyStickHatMove && Id == Event.window.windowID)
					{
						switch (Event.jhat.value)
						{
							case SDL_HAT_CENTERED:
								Callbacks.JoyStickHatMove(JoyStickHat_Center, (int)Event.jhat.which, (int)Event.jhat.hat);
								return true;
							case SDL_HAT_UP:
								Callbacks.JoyStickHatMove(JoyStickHat_Up, (int)Event.jhat.which, (int)Event.jhat.hat);
								return true;
							case SDL_HAT_DOWN:
								Callbacks.JoyStickHatMove(JoyStickHat_Down, (int)Event.jhat.which, (int)Event.jhat.hat);
								return true;
							case SDL_HAT_LEFT:
								Callbacks.JoyStickHatMove(JoyStickHat_Left, (int)Event.jhat.which, (int)Event.jhat.hat);
								return true;
							case SDL_HAT_LEFTUP:
								Callbacks.JoyStickHatMove(JoyStickHat_Left_Up, (int)Event.jhat.which, (int)Event.jhat.hat);
								return true;
							case SDL_HAT_LEFTDOWN:
								Callbacks.JoyStickHatMove(JoyStickHat_Left_Down, (int)Event.jhat.which, (int)Event.jhat.hat);
								return true;
							case SDL_HAT_RIGHT:
								Callbacks.JoyStickHatMove(JoyStickHat_Right, (int)Event.jhat.which, (int)Event.jhat.hat);
								return true;
							case SDL_HAT_RIGHTUP:
								Callbacks.JoyStickHatMove(JoyStickHat_Right_Up, (int)Event.jhat.which, (int)Event.jhat.hat);
								return true;
							case SDL_HAT_RIGHTDOWN:
								Callbacks.JoyStickHatMove(JoyStickHat_Right_Down, (int)Event.jhat.which, (int)Event.jhat.hat);
								return true;
						}
					}
					return true;
				case SDL_JOYBUTTONDOWN:
					if (Callbacks.JoyStickKeyState && Id == Event.window.windowID)
						Callbacks.JoyStickKeyState((int)Event.jbutton.which, (int)Event.jbutton.button, true);
					return true;
				case SDL_JOYBUTTONUP:
					if (Callbacks.JoyStickKeyState && Id == Event.window.windowID)
						Callbacks.JoyStickKeyState((int)Event.jbutton.which, (int)Event.jbutton.button, false);
					return true;
				case SDL_JOYDEVICEADDED:
					if (Callbacks.JoyStickState && Id == Event.window.windowID)
						Callbacks.JoyStickState((int)Event.jdevice.which, true);
					return true;
				case SDL_JOYDEVICEREMOVED:
					if (Callbacks.JoyStickState && Id == Event.window.windowID)
						Callbacks.JoyStickState((int)Event.jdevice.which, true);
					return true;
				case SDL_CONTROLLERAXISMOTION:
					if (Callbacks.ControllerAxisMove && Id == Event.window.windowID)
						Callbacks.ControllerAxisMove((int)Event.caxis.which, (int)Event.caxis.axis, (int)Event.caxis.value);
					return true;
				case SDL_CONTROLLERBUTTONDOWN:
					if (Callbacks.ControllerKeyState && Id == Event.window.windowID)
						Callbacks.ControllerKeyState((int)Event.cbutton.which, (int)Event.cbutton.button, true);
					return true;
				case SDL_CONTROLLERBUTTONUP:
					if (Callbacks.ControllerKeyState && Id == Event.window.windowID)
						Callbacks.ControllerKeyState((int)Event.cbutton.which, (int)Event.cbutton.button, false);
					return true;
				case SDL_CONTROLLERDEVICEADDED:
					if (Callbacks.ControllerState && Id == Event.window.windowID)
						Callbacks.ControllerState((int)Event.cdevice.which, 1);
					return true;
				case SDL_CONTROLLERDEVICEREMOVED:
					if (Callbacks.ControllerState && Id == Event.window.windowID)
						Callbacks.ControllerState((int)Event.cdevice.which, -1);
					return true;
				case SDL_CONTROLLERDEVICEREMAPPED:
					if (Callbacks.ControllerState && Id == Event.window.windowID)
						Callbacks.ControllerState((int)Event.cdevice.which, 0);
					return true;
				case SDL_FINGERMOTION:
					if (Callbacks.TouchMove && Id == Event.window.windowID)
						Callbacks.TouchMove((int)Event.tfinger.touchId, (int)Event.tfinger.fingerId, Event.tfinger.x, Event.tfinger.y, Event.tfinger.dx, Event.tfinger.dy, Event.tfinger.pressure);
					return true;
				case SDL_FINGERDOWN:
					if (Callbacks.TouchState && Id == Event.window.windowID)
						Callbacks.TouchState((int)Event.tfinger.touchId, (int)Event.tfinger.fingerId, Event.tfinger.x, Event.tfinger.y, Event.tfinger.dx, Event.tfinger.dy, Event.tfinger.pressure, true);
					return true;
				case SDL_FINGERUP:
					if (Callbacks.TouchState && Id == Event.window.windowID)
						Callbacks.TouchState((int)Event.tfinger.touchId, (int)Event.tfinger.fingerId, Event.tfinger.x, Event.tfinger.y, Event.tfinger.dx, Event.tfinger.dy, Event.tfinger.pressure, false);
					return true;
				case SDL_DOLLARGESTURE:
					if (Callbacks.GestureState && Id == Event.window.windowID)
						Callbacks.GestureState((int)Event.dgesture.touchId, (int)Event.dgesture.gestureId, (int)Event.dgesture.numFingers, Event.dgesture.x, Event.dgesture.y, Event.dgesture.error, false);
					return true;
				case SDL_DOLLARRECORD:
					if (Callbacks.GestureState && Id == Event.window.windowID)
						Callbacks.GestureState((int)Event.dgesture.touchId, (int)Event.dgesture.gestureId, (int)Event.dgesture.numFingers, Event.dgesture.x, Event.dgesture.y, Event.dgesture.error, true);
					return true;
				case SDL_MULTIGESTURE:
					if (Callbacks.MultiGestureState && Id == Event.window.windowID)
						Callbacks.MultiGestureState((int)Event.mgesture.touchId, (int)Event.mgesture.numFingers, Event.mgesture.x, Event.mgesture.y, Event.mgesture.dDist, Event.mgesture.dTheta);
					return true;
#if SDL_VERSION_ATLEAST(2, 0, 5)
				case SDL_DROPFILE:
					if (Id != Event.window.windowID)
						return true;

					if (Callbacks.DropFile)
						Callbacks.DropFile(Event.drop.file);

					SDL_free(Event.drop.file);
					return true;
				case SDL_DROPTEXT:
					if (Id != Event.window.windowID)
						return true;

					if (Callbacks.DropText)
						Callbacks.DropText(Event.drop.file);

					SDL_free(Event.drop.file);
					return true;
#endif
			}

			return true;
#else
			return false;
#endif
		}
		bool Activity::CaptureKeyMap(KeyMap* Value)
		{
			if (!Value)
			{
				Mapping.Mapped = false;
				Mapping.Captured = false;
				Mapping.Enabled = false;
				return false;
			}

			if (!Mapping.Enabled)
			{
				Mapping.Mapped = false;
				Mapping.Captured = false;
				Mapping.Enabled = true;
				return false;
			}

			if (!Mapping.Mapped || !Mapping.Captured)
				return false;

			Mapping.Enabled = Mapping.Mapped = Mapping.Captured = false;
			memcpy(Value, &Mapping.Key, sizeof(KeyMap));

			return true;
		}
		bool Activity::IsFullscreen()
		{
#ifdef THAWK_HAS_SDL2
			Uint32 Flags = SDL_GetWindowFlags(Handle);
			return Flags & SDL_WINDOW_FULLSCREEN || Flags & SDL_WINDOW_FULLSCREEN_DESKTOP;
#else
			return false;
#endif
		}
		bool Activity::IsAnyKeyDown()
		{
			for (int i = 0; i < 1024; i++)
			{
				if (Keys[i])
					return true;
			}

			return false;
		}
		bool Activity::IsKeyDown(const KeyMap& Key)
		{
#ifdef THAWK_HAS_SDL2
			bool* Map = GetInputState();
			if (Key.Mod == KeyMod_NONE)
				return Map[Key.Key];

			int Mod = (int)SDL_GetModState();
			if (Key.Key == KeyCode_NONE)
				return Mod & Key.Mod;

			return Mod & Key.Mod && Map[Key.Key];
#else
			return Keys[0][Key.Key];
#endif
		}
		bool Activity::IsKeyUp(const KeyMap& Key)
		{
			return !IsKeyDown(Key);
		}
		bool Activity::IsKeyDownHit(const KeyMap& Key)
		{
#ifdef THAWK_HAS_SDL2
			bool* Map = GetInputState();
			if (Key.Mod == KeyMod_NONE)
				return Map[Key.Key] && !Keys[1][Key.Key];

			int Mod = (int)SDL_GetModState();
			if (Key.Key == KeyCode_NONE)
				return Mod & Key.Mod && !(Command & Key.Mod);

			return Mod & Key.Mod && !(Command & Key.Mod) && Map[Key.Key] && !Keys[1][Key.Key];
#else
			return Keys[0][Key.Key] && !Keys[1][Key.Key];
#endif
		}
		bool Activity::IsKeyUpHit(const KeyMap& Key)
		{
#ifdef THAWK_HAS_SDL2
			bool* Map = GetInputState();
			if (Key.Mod == KeyMod_NONE)
				return !Map[Key.Key] && Keys[1][Key.Key];

			int Mod = (int)SDL_GetModState();
			if (Key.Key == KeyCode_NONE)
				return !(Mod & Key.Mod) && Command & Key.Mod;

			return !(Mod & Key.Mod) && Command & Key.Mod && !Map[Key.Key] && Keys[1][Key.Key];
#else
			return !Keys[0][Key.Key] && Keys[1][Key.Key];
#endif
		}
		float Activity::GetX()
		{
#ifdef THAWK_HAS_SDL2
			int X, Y;
			SDL_GetWindowPosition(Handle, &X, &Y);

			return (float)X;
#else
			return 0.0f;
#endif
		}
		float Activity::GetY()
		{
#ifdef THAWK_HAS_SDL2
			int X, Y;
			SDL_GetWindowPosition(Handle, &X, &Y);

			return (float)Y;
#else
			return 0.0f;
#endif
		}
		float Activity::GetWidth()
		{
#ifdef THAWK_HAS_SDL2
			int W, H;
			SDL_GetWindowSize(Handle, &W, &H);

			return (float)W;
#else
			return 0.0f;
#endif
		}
		float Activity::GetHeight()
		{
#ifdef THAWK_HAS_SDL2
			int W, H;
			SDL_GetWindowSize(Handle, &W, &H);

			return (float)H;
#else
			return 0.0f;
#endif
		}
		float Activity::GetAspectRatio()
		{
#ifdef THAWK_HAS_SDL2
			int W, H;
			SDL_GetWindowSize(Handle, &W, &H);

			return (H > 0 ? (float)W / (float)H : 0.0f);
#else
			return 0.0f;
#endif
		}
		KeyMod Activity::GetKeyModState()
		{
#ifdef THAWK_HAS_SDL2
			return (KeyMod)SDL_GetModState();
#else
			return KeyMod_NONE;
#endif
		}
		Graphics::Viewport Activity::GetViewport()
		{
#ifdef THAWK_HAS_SDL2
			int W, H;
			SDL_GetWindowSize(Handle, &W, &H);

			Graphics::Viewport Id;
			Id.Width = (float)W;
			Id.Height = (float)H;
			Id.MinDepth = 0.0f;
			Id.MaxDepth = 1.0f;
			Id.TopLeftX = 0.0f;
			Id.TopLeftY = 0.0f;
			return Id;
#else
			return Graphics::Viewport();
#endif
		}
		Compute::Vector2 Activity::GetSize()
		{
#ifdef THAWK_HAS_SDL2
			int W, H;
			SDL_GL_GetDrawableSize(Handle, &W, &H);

			return Compute::Vector2((float)W, (float)H);
#else
			return Compute::Vector2();
#endif
		}
		Compute::Vector2 Activity::GetClientSize()
		{
#ifdef THAWK_HAS_SDL2
			int W, H;
			SDL_GetWindowSize(Handle, &W, &H);

			return Compute::Vector2((float)W, (float)H);
#else
			return Compute::Vector2();
#endif
		}
		Compute::Vector2 Activity::GetOffset()
		{
#ifdef THAWK_HAS_SDL2
			SDL_DisplayMode Display;
			SDL_GetCurrentDisplayMode(0, &Display);

			Compute::Vector2 Size = GetSize();
			return Compute::Vector2((float)Display.w / Size.X, (float)Display.h / Size.Y);
#else
			return Compute::Vector2();
#endif
		}
		Compute::Vector2 Activity::GetGlobalCursorPosition()
		{
#ifdef THAWK_HAS_SDL2
#if SDL_VERSION_ATLEAST(2, 0, 4)
			int X, Y;
			SDL_GetGlobalMouseState(&X, &Y);

			return Compute::Vector2((float)X, (float)Y);
#else
			return Compute::Vector2();
#endif
#else
			return Compute::Vector2();
#endif
		}
		Compute::Vector2 Activity::GetCursorPosition()
		{
#ifdef THAWK_HAS_SDL2
#if SDL_VERSION_ATLEAST(2, 0, 4)
			int X, Y;
			SDL_GetMouseState(&X, &Y);

			return Compute::Vector2((float)X, (float)Y);
#else
			return Compute::Vector2();
#endif
#else
			return Compute::Vector2();
#endif
		}
		Compute::Vector2 Activity::GetCursorPosition(float ScreenWidth, float ScreenHeight)
		{
#ifdef THAWK_HAS_SDL2
			Compute::Vector2 Size = GetSize();
			return GetCursorPosition() * Compute::Vector2(ScreenWidth / Size.X, ScreenHeight / Size.Y);
#else
			return Compute::Vector2();
#endif
		}
		Compute::Vector2 Activity::GetCursorPosition(Compute::Vector2 ScreenDimensions)
		{
#ifdef THAWK_HAS_SDL2
			Compute::Vector2 Size = GetSize();
			return GetCursorPosition() * Compute::Vector2(ScreenDimensions.X / Size.X, ScreenDimensions.Y / Size.Y);
#else
			return Compute::Vector2();
#endif
		}
		std::string Activity::GetClipboardText()
		{
#ifdef THAWK_HAS_SDL2
			char* Text = SDL_GetClipboardText();
			std::string Result = (Text ? Text : "");

			if (Text != nullptr)
				SDL_free(Text);

			return Result;
#else
			return nullptr;
#endif
		}
		SDL_Window* Activity::GetHandle()
		{
			return Handle;
		}
		std::string Activity::GetError()
		{
#ifdef THAWK_HAS_SDL2
			const char* Error = SDL_GetError();
			if (!Error)
				return "";

			return Error;
#else
			return "";
#endif
		}
		bool* Activity::GetInputState()
		{
#ifdef THAWK_HAS_SDL2
			int Count;
			auto* Map = SDL_GetKeyboardState(&Count);
			if (Count > 1024)
				Count = 1024;

			for (int i = 0; i < Count; i++)
				Keys[0][i] = Map[i] > 0;

			Uint32 State = SDL_GetMouseState(nullptr, nullptr);
			Keys[0][KeyCode_CURSORLEFT] = (State & SDL_BUTTON(SDL_BUTTON_LEFT));
			Keys[0][KeyCode_CURSORMIDDLE] = (State & SDL_BUTTON(SDL_BUTTON_MIDDLE));
			Keys[0][KeyCode_CURSORRIGHT] = (State & SDL_BUTTON(SDL_BUTTON_RIGHT));
			Keys[0][KeyCode_CURSORX1] = (State & SDL_BUTTON(SDL_BUTTON_X1));
			Keys[0][KeyCode_CURSORX2] = (State & SDL_BUTTON(SDL_BUTTON_X2));
#endif
			return Keys[0];
		}
		const char* Activity::GetKeyName(KeyCode Code)
		{
			const char* Name;
			switch (Code)
			{
				case KeyCode_A:
					Name = "A";
					break;
				case KeyCode_B:
					Name = "B";
					break;
				case KeyCode_C:
					Name = "C";
					break;
				case KeyCode_D:
					Name = "D";
					break;
				case KeyCode_E:
					Name = "E";
					break;
				case KeyCode_F:
					Name = "F";
					break;
				case KeyCode_G:
					Name = "G";
					break;
				case KeyCode_H:
					Name = "H";
					break;
				case KeyCode_I:
					Name = "I";
					break;
				case KeyCode_J:
					Name = "J";
					break;
				case KeyCode_K:
					Name = "K";
					break;
				case KeyCode_L:
					Name = "L";
					break;
				case KeyCode_M:
					Name = "M";
					break;
				case KeyCode_N:
					Name = "N";
					break;
				case KeyCode_O:
					Name = "O";
					break;
				case KeyCode_P:
					Name = "P";
					break;
				case KeyCode_Q:
					Name = "Q";
					break;
				case KeyCode_R:
					Name = "R";
					break;
				case KeyCode_S:
					Name = "S";
					break;
				case KeyCode_T:
					Name = "T";
					break;
				case KeyCode_U:
					Name = "U";
					break;
				case KeyCode_V:
					Name = "V";
					break;
				case KeyCode_W:
					Name = "W";
					break;
				case KeyCode_X:
					Name = "X";
					break;
				case KeyCode_Y:
					Name = "Y";
					break;
				case KeyCode_Z:
					Name = "Z";
					break;
				case KeyCode_1:
					Name = "1";
					break;
				case KeyCode_2:
					Name = "2";
					break;
				case KeyCode_3:
					Name = "3";
					break;
				case KeyCode_4:
					Name = "4";
					break;
				case KeyCode_5:
					Name = "5";
					break;
				case KeyCode_6:
					Name = "6";
					break;
				case KeyCode_7:
					Name = "7";
					break;
				case KeyCode_8:
					Name = "8";
					break;
				case KeyCode_9:
					Name = "9";
					break;
				case KeyCode_0:
					Name = "0";
					break;
				case KeyCode_RETURN:
					Name = "Return";
					break;
				case KeyCode_ESCAPE:
					Name = "Escape";
					break;
				case KeyCode_BACKSPACE:
					Name = "Backspace";
					break;
				case KeyCode_TAB:
					Name = "Tab";
					break;
				case KeyCode_SPACE:
					Name = "Space";
					break;
				case KeyCode_MINUS:
					Name = "Minus";
					break;
				case KeyCode_EQUALS:
					Name = "Equals";
					break;
				case KeyCode_LEFTBRACKET:
					Name = "Left Bracket";
					break;
				case KeyCode_RIGHTBRACKET:
					Name = "Rigth Bracket";
					break;
				case KeyCode_BACKSLASH:
					Name = "Backslash";
					break;
				case KeyCode_NONUSHASH:
					Name = "Nonuslash";
					break;
				case KeyCode_SEMICOLON:
					Name = "Semicolon";
					break;
				case KeyCode_APOSTROPHE:
					Name = "Apostrophe";
					break;
				case KeyCode_GRAVE:
					Name = "Grave";
					break;
				case KeyCode_COMMA:
					Name = "Comma";
					break;
				case KeyCode_PERIOD:
					Name = "Period";
					break;
				case KeyCode_SLASH:
					Name = "Slash";
					break;
				case KeyCode_CAPSLOCK:
					Name = "Caps Lock";
					break;
				case KeyCode_F1:
					Name = "F1";
					break;
				case KeyCode_F2:
					Name = "F2";
					break;
				case KeyCode_F3:
					Name = "F3";
					break;
				case KeyCode_F4:
					Name = "F4";
					break;
				case KeyCode_F5:
					Name = "F5";
					break;
				case KeyCode_F6:
					Name = "F6";
					break;
				case KeyCode_F7:
					Name = "F7";
					break;
				case KeyCode_F8:
					Name = "F8";
					break;
				case KeyCode_F9:
					Name = "F9";
					break;
				case KeyCode_F10:
					Name = "F10";
					break;
				case KeyCode_F11:
					Name = "F11";
					break;
				case KeyCode_F12:
					Name = "F12";
					break;
				case KeyCode_PRINTSCREEN:
					Name = "Print Screen";
					break;
				case KeyCode_SCROLLLOCK:
					Name = "Scroll Lock";
					break;
				case KeyCode_PAUSE:
					Name = "Pause";
					break;
				case KeyCode_INSERT:
					Name = "Insert";
					break;
				case KeyCode_HOME:
					Name = "Home";
					break;
				case KeyCode_PAGEUP:
					Name = "Page Up";
					break;
				case KeyCode_DELETE:
					Name = "Delete";
					break;
				case KeyCode_END:
					Name = "End";
					break;
				case KeyCode_PAGEDOWN:
					Name = "Page Down";
					break;
				case KeyCode_RIGHT:
					Name = "Right";
					break;
				case KeyCode_LEFT:
					Name = "Left";
					break;
				case KeyCode_DOWN:
					Name = "Down";
					break;
				case KeyCode_UP:
					Name = "Up";
					break;
				case KeyCode_NUMLOCKCLEAR:
					Name = "Numlock Clear";
					break;
				case KeyCode_KP_DIVIDE:
					Name = "Divide";
					break;
				case KeyCode_KP_MULTIPLY:
					Name = "Multiply";
					break;
				case KeyCode_KP_MINUS:
					Name = "Minus";
					break;
				case KeyCode_KP_PLUS:
					Name = "Plus";
					break;
				case KeyCode_KP_ENTER:
					Name = "Enter";
					break;
				case KeyCode_KP_1:
					Name = "1";
					break;
				case KeyCode_KP_2:
					Name = "2";
					break;
				case KeyCode_KP_3:
					Name = "3";
					break;
				case KeyCode_KP_4:
					Name = "4";
					break;
				case KeyCode_KP_5:
					Name = "5";
					break;
				case KeyCode_KP_6:
					Name = "6";
					break;
				case KeyCode_KP_7:
					Name = "7";
					break;
				case KeyCode_KP_8:
					Name = "8";
					break;
				case KeyCode_KP_9:
					Name = "9";
					break;
				case KeyCode_KP_0:
					Name = "0";
					break;
				case KeyCode_KP_PERIOD:
					Name = "Period";
					break;
				case KeyCode_NONUSBACKSLASH:
					Name = "Nonus Backslash";
					break;
				case KeyCode_APPLICATION:
					Name = "Application";
					break;
				case KeyCode_POWER:
					Name = "Power";
					break;
				case KeyCode_KP_EQUALS:
					Name = "Equals";
					break;
				case KeyCode_F13:
					Name = "F13";
					break;
				case KeyCode_F14:
					Name = "F14";
					break;
				case KeyCode_F15:
					Name = "F15";
					break;
				case KeyCode_F16:
					Name = "F16";
					break;
				case KeyCode_F17:
					Name = "F17";
					break;
				case KeyCode_F18:
					Name = "F18";
					break;
				case KeyCode_F19:
					Name = "F19";
					break;
				case KeyCode_F20:
					Name = "F20";
					break;
				case KeyCode_F21:
					Name = "F21";
					break;
				case KeyCode_F22:
					Name = "F22";
					break;
				case KeyCode_F23:
					Name = "F23";
					break;
				case KeyCode_F24:
					Name = "F24";
					break;
				case KeyCode_EXECUTE:
					Name = "Execute";
					break;
				case KeyCode_HELP:
					Name = "Help";
					break;
				case KeyCode_MENU:
					Name = "Menu";
					break;
				case KeyCode_SELECT:
					Name = "Select";
					break;
				case KeyCode_STOP:
					Name = "Stop";
					break;
				case KeyCode_AGAIN:
					Name = "Again";
					break;
				case KeyCode_UNDO:
					Name = "Undo";
					break;
				case KeyCode_CUT:
					Name = "Cut";
					break;
				case KeyCode_COPY:
					Name = "Copy";
					break;
				case KeyCode_PASTE:
					Name = "Paste";
					break;
				case KeyCode_FIND:
					Name = "Find";
					break;
				case KeyCode_MUTE:
					Name = "Mute";
					break;
				case KeyCode_VOLUMEUP:
					Name = "Volume Up";
					break;
				case KeyCode_VOLUMEDOWN:
					Name = "Volume Down";
					break;
				case KeyCode_KP_COMMA:
					Name = "Comma";
					break;
				case KeyCode_KP_EQUALSAS400:
					Name = "Equals As 400";
					break;
				case KeyCode_INTERNATIONAL1:
					Name = "International 1";
					break;
				case KeyCode_INTERNATIONAL2:
					Name = "International 2";
					break;
				case KeyCode_INTERNATIONAL3:
					Name = "International 3";
					break;
				case KeyCode_INTERNATIONAL4:
					Name = "International 4";
					break;
				case KeyCode_INTERNATIONAL5:
					Name = "International 5";
					break;
				case KeyCode_INTERNATIONAL6:
					Name = "International 6";
					break;
				case KeyCode_INTERNATIONAL7:
					Name = "International 7";
					break;
				case KeyCode_INTERNATIONAL8:
					Name = "International 8";
					break;
				case KeyCode_INTERNATIONAL9:
					Name = "International 9";
					break;
				case KeyCode_LANG1:
					Name = "Lang 1";
					break;
				case KeyCode_LANG2:
					Name = "Lang 2";
					break;
				case KeyCode_LANG3:
					Name = "Lang 3";
					break;
				case KeyCode_LANG4:
					Name = "Lang 4";
					break;
				case KeyCode_LANG5:
					Name = "Lang 5";
					break;
				case KeyCode_LANG6:
					Name = "Lang 6";
					break;
				case KeyCode_LANG7:
					Name = "Lang 7";
					break;
				case KeyCode_LANG8:
					Name = "Lang 8";
					break;
				case KeyCode_LANG9:
					Name = "Lang 9";
					break;
				case KeyCode_ALTERASE:
					Name = "Alter Rase";
					break;
				case KeyCode_SYSREQ:
					Name = "Sys Req";
					break;
				case KeyCode_CANCEL:
					Name = "Cancel";
					break;
				case KeyCode_CLEAR:
					Name = "Clear";
					break;
				case KeyCode_PRIOR:
					Name = "Prior";
					break;
				case KeyCode_RETURN2:
					Name = "Return 2";
					break;
				case KeyCode_SEPARATOR:
					Name = "Separator";
					break;
				case KeyCode_OUT:
					Name = "Out";
					break;
				case KeyCode_OPER:
					Name = "Oper";
					break;
				case KeyCode_CLEARAGAIN:
					Name = "Clear Again";
					break;
				case KeyCode_CRSEL:
					Name = "CR Sel";
					break;
				case KeyCode_EXSEL:
					Name = "EX Sel";
					break;
				case KeyCode_KP_00:
					Name = "00";
					break;
				case KeyCode_KP_000:
					Name = "000";
					break;
				case KeyCode_THOUSANDSSEPARATOR:
					Name = "Thousands Separator";
					break;
				case KeyCode_DECIMALSEPARATOR:
					Name = "Decimal Separator";
					break;
				case KeyCode_CURRENCYUNIT:
					Name = "Currency Unit";
					break;
				case KeyCode_CURRENCYSUBUNIT:
					Name = "Currency Subunit";
					break;
				case KeyCode_KP_LEFTPAREN:
					Name = "Left Parent";
					break;
				case KeyCode_KP_RIGHTPAREN:
					Name = "Right Paren";
					break;
				case KeyCode_KP_LEFTBRACE:
					Name = "Left Brace";
					break;
				case KeyCode_KP_RIGHTBRACE:
					Name = "Right Brace";
					break;
				case KeyCode_KP_TAB:
					Name = "Tab";
					break;
				case KeyCode_KP_BACKSPACE:
					Name = "Backspace";
					break;
				case KeyCode_KP_A:
					Name = "A";
					break;
				case KeyCode_KP_B:
					Name = "B";
					break;
				case KeyCode_KP_C:
					Name = "C";
					break;
				case KeyCode_KP_D:
					Name = "D";
					break;
				case KeyCode_KP_E:
					Name = "E";
					break;
				case KeyCode_KP_F:
					Name = "F";
					break;
				case KeyCode_KP_XOR:
					Name = "Xor";
					break;
				case KeyCode_KP_POWER:
					Name = "Power";
					break;
				case KeyCode_KP_PERCENT:
					Name = "Percent";
					break;
				case KeyCode_KP_LESS:
					Name = "Less";
					break;
				case KeyCode_KP_GREATER:
					Name = "Greater";
					break;
				case KeyCode_KP_AMPERSAND:
					Name = "Ampersand";
					break;
				case KeyCode_KP_DBLAMPERSAND:
					Name = "DBL Ampersand";
					break;
				case KeyCode_KP_VERTICALBAR:
					Name = "Vertical Bar";
					break;
				case KeyCode_KP_DBLVERTICALBAR:
					Name = "OBL Vertical Bar";
					break;
				case KeyCode_KP_COLON:
					Name = "Colon";
					break;
				case KeyCode_KP_HASH:
					Name = "Hash";
					break;
				case KeyCode_KP_SPACE:
					Name = "Space";
					break;
				case KeyCode_KP_AT:
					Name = "At";
					break;
				case KeyCode_KP_EXCLAM:
					Name = "Exclam";
					break;
				case KeyCode_KP_MEMSTORE:
					Name = "Mem Store";
					break;
				case KeyCode_KP_MEMRECALL:
					Name = "Mem Recall";
					break;
				case KeyCode_KP_MEMCLEAR:
					Name = "Mem Clear";
					break;
				case KeyCode_KP_MEMADD:
					Name = "Mem Add";
					break;
				case KeyCode_KP_MEMSUBTRACT:
					Name = "Mem Subtract";
					break;
				case KeyCode_KP_MEMMULTIPLY:
					Name = "Mem Multiply";
					break;
				case KeyCode_KP_MEMDIVIDE:
					Name = "Mem Divide";
					break;
				case KeyCode_KP_PLUSMINUS:
					Name = "Plus-Minus";
					break;
				case KeyCode_KP_CLEAR:
					Name = "Clear";
					break;
				case KeyCode_KP_CLEARENTRY:
					Name = "Clear Entry";
					break;
				case KeyCode_KP_BINARY:
					Name = "Binary";
					break;
				case KeyCode_KP_OCTAL:
					Name = "Octal";
					break;
				case KeyCode_KP_DECIMAL:
					Name = "Decima;";
					break;
				case KeyCode_KP_HEXADECIMAL:
					Name = "Hexadecimal";
					break;
				case KeyCode_LCTRL:
					Name = "Left CTRL";
					break;
				case KeyCode_LSHIFT:
					Name = "Left Shift";
					break;
				case KeyCode_LALT:
					Name = "Left Alt";
					break;
				case KeyCode_LGUI:
					Name = "Left GUI";
					break;
				case KeyCode_RCTRL:
					Name = "Right CTRL";
					break;
				case KeyCode_RSHIFT:
					Name = "Right Shift";
					break;
				case KeyCode_RALT:
					Name = "Right Alt";
					break;
				case KeyCode_RGUI:
					Name = "Right GUI";
					break;
				case KeyCode_MODE:
					Name = "Mode";
					break;
				case KeyCode_AUDIONEXT:
					Name = "Audio Next";
					break;
				case KeyCode_AUDIOPREV:
					Name = "Audio Prev";
					break;
				case KeyCode_AUDIOSTOP:
					Name = "Audio Stop";
					break;
				case KeyCode_AUDIOPLAY:
					Name = "Audio Play";
					break;
				case KeyCode_AUDIOMUTE:
					Name = "Audio Mute";
					break;
				case KeyCode_MEDIASELECT:
					Name = "Media Select";
					break;
				case KeyCode_WWW:
					Name = "WWW";
					break;
				case KeyCode_MAIL:
					Name = "Mail";
					break;
				case KeyCode_CALCULATOR:
					Name = "Calculator";
					break;
				case KeyCode_COMPUTER:
					Name = "Computer";
					break;
				case KeyCode_AC_SEARCH:
					Name = "AC Search";
					break;
				case KeyCode_AC_HOME:
					Name = "AC Home";
					break;
				case KeyCode_AC_BACK:
					Name = "AC Back";
					break;
				case KeyCode_AC_FORWARD:
					Name = "AC Forward";
					break;
				case KeyCode_AC_STOP:
					Name = "AC Stop";
					break;
				case KeyCode_AC_REFRESH:
					Name = "AC Refresh";
					break;
				case KeyCode_AC_BOOKMARKS:
					Name = "AC Bookmarks";
					break;
				case KeyCode_BRIGHTNESSDOWN:
					Name = "Brigthness Down";
					break;
				case KeyCode_BRIGHTNESSUP:
					Name = "Brigthness Up";
					break;
				case KeyCode_DISPLAYSWITCH:
					Name = "Display Switch";
					break;
				case KeyCode_KBDILLUMTOGGLE:
					Name = "Dillum Toggle";
					break;
				case KeyCode_KBDILLUMDOWN:
					Name = "Dillum Down";
					break;
				case KeyCode_KBDILLUMUP:
					Name = "Dillum Up";
					break;
				case KeyCode_EJECT:
					Name = "Eject";
					break;
				case KeyCode_SLEEP:
					Name = "Sleep";
					break;
				case KeyCode_APP1:
					Name = "App 1";
					break;
				case KeyCode_APP2:
					Name = "App 2";
					break;
				case KeyCode_AUDIOREWIND:
					Name = "Audio Rewind";
					break;
				case KeyCode_AUDIOFASTFORWARD:
					Name = "Audio Fast Forward";
					break;
				case KeyCode_CURSORLEFT:
					Name = "Cursor Left";
					break;
				case KeyCode_CURSORMIDDLE:
					Name = "Cursor Middle";
					break;
				case KeyCode_CURSORRIGHT:
					Name = "Cursor Right";
					break;
				case KeyCode_CURSORX1:
					Name = "Cursor X1";
					break;
				case KeyCode_CURSORX2:
					Name = "Cursor X2";
					break;
				default:
					Name = nullptr;
					break;
			}

			return Name;
		}

		Model::Model()
		{
		}
		Model::~Model()
		{
			for (auto It = Meshes.begin(); It != Meshes.end(); It++)
				delete (*It);
		}
		MeshBuffer* Model::Find(const std::string& Name)
		{
			for (auto&& It : Meshes)
			{
				if (It->Name == Name)
					return It;
			}

			return nullptr;
		}

		SkinModel::SkinModel()
		{
		}
		SkinModel::~SkinModel()
		{
			for (auto It = Meshes.begin(); It != Meshes.end(); It++)
				delete (*It);
		}
		void SkinModel::ComputePose(PoseBuffer* Map)
		{
			if (Map != nullptr && !Joints.empty())
			{
				if (Map->Pose.empty())
					Map->SetPose(this);

				for (auto& Child : Joints)
					ComputePose(Map, &Child, Compute::Matrix4x4::Identity());
			}
		}
		void SkinModel::ComputePose(PoseBuffer* Map, Compute::Joint* Base, const Compute::Matrix4x4& World)
		{
			PoseNode* Node = Map->GetNode(Base->Index);
			Compute::Matrix4x4 Offset = (Node ? Map->GetOffset(Node) * World : World);

			if (Base->Index >= 0 && Base->Index < 96)
				Map->Transform[Base->Index] = Base->BindShape * Offset * Root;

			for (auto& Child : Base->Childs)
				ComputePose(Map, &Child, Offset);
		}
		SkinMeshBuffer* SkinModel::FindMesh(const std::string& Name)
		{
			for (auto&& It : Meshes)
			{
				if (It->Name == Name)
					return It;
			}

			return nullptr;
		}
		Compute::Joint* SkinModel::FindJoint(const std::string& Name, Compute::Joint* Base)
		{
			if (Base != nullptr)
			{
				if (Base->Name == Name)
					return Base;

				for (auto&& Child : Base->Childs)
				{
					if (Child.Name == Name)
						return &Child;

					Compute::Joint* Result = FindJoint(Name, &Child);
					if (Result != nullptr)
						return Result;
				}
			}
			else
			{
				for (auto&& Child : Joints)
				{
					if (Child.Name == Name)
						return &Child;

					Compute::Joint* Result = FindJoint(Name, &Child);
					if (Result != nullptr)
						return Result;
				}
			}

			return nullptr;
		}
		Compute::Joint* SkinModel::FindJoint(int64_t Index, Compute::Joint* Base)
		{
			if (Base != nullptr)
			{
				if (Base->Index == Index)
					return Base;

				for (auto&& Child : Base->Childs)
				{
					if (Child.Index == Index)
						return &Child;

					Compute::Joint* Result = FindJoint(Index, &Child);
					if (Result != nullptr)
						return Result;
				}
			}
			else
			{
				for (auto&& Child : Joints)
				{
					if (Child.Index == Index)
						return &Child;

					Compute::Joint* Result = FindJoint(Index, &Child);
					if (Result != nullptr)
						return Result;
				}
			}

			return nullptr;
		}
	}
}