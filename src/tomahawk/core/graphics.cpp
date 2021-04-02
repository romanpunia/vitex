#include "graphics.h"
#include "../graphics/d3d11.h"
#include "../graphics/ogl.h"
#include "../core/shaders.h"
#ifdef TH_HAS_SDL2
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
#ifdef TH_HAS_SDL2
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
#ifdef TH_HAS_SDL2
			if (Handle != nullptr)
			{
				SDL_FreeSurface(Handle);
				Handle = nullptr;
			}
#endif
		}
		void Surface::SetHandle(SDL_Surface* From)
		{
#ifdef TH_HAS_SDL2
			if (Handle != nullptr)
				SDL_FreeSurface(Handle);
#endif
			Handle = From;
		}
		void Surface::Lock()
		{
#ifdef TH_HAS_SDL2
			SDL_LockSurface(Handle);
#endif
		}
		void Surface::Unlock()
		{
#ifdef TH_HAS_SDL2
			SDL_UnlockSurface(Handle);
#endif
		}
		int Surface::GetWidth()
		{
#ifdef TH_HAS_SDL2
			if (!Handle)
				return -1;

			return Handle->w;
#else
			return -1;
#endif
		}
		int Surface::GetHeight()
		{
#ifdef TH_HAS_SDL2
			if (!Handle)
				return -1;

			return Handle->h;
#else
			return -1;
#endif
		}
		int Surface::GetPitch()
		{
#ifdef TH_HAS_SDL2
			if (!Handle)
				return -1;

			return Handle->pitch;
#else
			return -1;
#endif
		}
		void* Surface::GetPixels()
		{
#ifdef TH_HAS_SDL2
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

		InputLayout::InputLayout(const Desc& I) : Layout(I.Attributes)
		{
		}
		InputLayout::~InputLayout()
		{
		}
		const std::vector<InputLayout::Attribute>& InputLayout::GetAttributes()
		{
			return Layout;
		}

		Shader::Shader(const Desc& I)
		{
		}

		ElementBuffer::ElementBuffer(const Desc& I)
		{
			Elements = I.ElementCount;
			Stride = I.ElementWidth;
		}
		uint64_t ElementBuffer::GetElements()
		{
			return Elements;
		}
		uint64_t ElementBuffer::GetStride()
		{
			return Stride;
		}

		MeshBuffer::MeshBuffer(const Desc& I) : VertexBuffer(nullptr), IndexBuffer(nullptr)
		{
		}
		MeshBuffer::~MeshBuffer()
		{
			TH_RELEASE(VertexBuffer);
			TH_RELEASE(IndexBuffer);
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
			TH_RELEASE(VertexBuffer);
			TH_RELEASE(IndexBuffer);
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
			ElementWidth = I.ElementWidth;

			if (ElementLimit < 1)
				ElementLimit = 1;

			Array.Reserve(ElementLimit);
		}
		InstanceBuffer::~InstanceBuffer()
		{
			TH_RELEASE(Elements);
		}
		Core::Pool<Compute::ElementVertex>* InstanceBuffer::GetArray()
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
			FormatMode = Graphics::Format_Unknown;
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
			Width = 512;
			Height = 512;
			Depth = 1;
			MipLevels = 1;
			FormatMode = Graphics::Format_Unknown;
			Usage = Graphics::ResourceUsage_Default;
			AccessFlags = Graphics::CPUAccess_Invalid;
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
			Width = 512;
			Height = 512;
			MipLevels = 1;
			FormatMode = Graphics::Format_Unknown;
			Usage = Graphics::ResourceUsage_Default;
			AccessFlags = Graphics::CPUAccess_Invalid;
		}
		TextureCube::TextureCube(const Desc& I)
		{
			Width = I.Width;
			Height = I.Height;
			MipLevels = I.MipLevels;
			FormatMode = I.FormatMode;
			Usage = I.Usage;
			AccessFlags = I.AccessFlags;
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

		DepthBuffer::DepthBuffer(const Desc& I) : Resource(nullptr), Viewarea({ 0, 0, 512, 512, 0, 1 })
		{
		}
		DepthBuffer::~DepthBuffer()
		{
			TH_RELEASE(Resource);
		}
		Texture2D* DepthBuffer::GetTarget()
		{
			return Resource;
		}
		const Graphics::Viewport& DepthBuffer::GetViewport()
		{
			return Viewarea;
		}

		RenderTarget::RenderTarget() : DepthStencil(nullptr), Viewarea({ 0, 0, 512, 512, 0, 1 })
		{
		}
		RenderTarget::~RenderTarget()
		{
			TH_RELEASE(DepthStencil);
		}
		Texture2D* RenderTarget::GetDepthStencil()
		{
			return DepthStencil;
		}
		const Graphics::Viewport& RenderTarget::GetViewport()
		{
			return Viewarea;
		}

		RenderTarget2D::RenderTarget2D(const Desc& I) : RenderTarget(), Resource(nullptr)
		{
		}
		RenderTarget2D::~RenderTarget2D()
		{
			TH_RELEASE(Resource);
		}
		uint32_t RenderTarget2D::GetTargetCount()
		{
			return 1;
		}
		Texture2D* RenderTarget2D::GetTarget(unsigned int Index)
		{
			return Resource;
		}

		MultiRenderTarget2D::MultiRenderTarget2D(const Desc& I) : RenderTarget()
		{
			Target = I.Target;
			for (int i = 0; i < 8; i++)
				Resource[i] = nullptr;
		}
		MultiRenderTarget2D::~MultiRenderTarget2D()
		{
			for (int i = 0; i < Target; i++)
				TH_RELEASE(Resource[i]);
		}
		uint32_t MultiRenderTarget2D::GetTargetCount()
		{
			return (uint32_t)Target;
		}
		Texture2D* MultiRenderTarget2D::GetTarget(unsigned int Slot)
		{
			if (Slot >= Target)
				return nullptr;

			return Resource[Slot];
		}

		RenderTargetCube::RenderTargetCube(const Desc& I) : RenderTarget(), Resource(nullptr)
		{
		}
		RenderTargetCube::~RenderTargetCube()
		{
			TH_RELEASE(Resource);
		}
		uint32_t RenderTargetCube::GetTargetCount()
		{
			return 1;
		}
		Texture2D* RenderTargetCube::GetTarget(unsigned int Index)
		{
			return Resource;
		}

		MultiRenderTargetCube::MultiRenderTargetCube(const Desc& I) : RenderTarget()
		{
			Target = I.Target;
			for (int i = 0; i < 8; i++)
				Resource[i] = nullptr;
		}
		MultiRenderTargetCube::~MultiRenderTargetCube()
		{
			for (int i = 0; i < Target; i++)
				TH_RELEASE(Resource[i]);
		}
		uint32_t MultiRenderTargetCube::GetTargetCount()
		{
			return (uint32_t)Target;
		}
		Texture2D* MultiRenderTargetCube::GetTarget(unsigned int Slot)
		{
			if (Slot >= Target)
				return nullptr;

			return Resource[Slot];
		}

		Cubemap::Cubemap(const Desc& I) : Meta(I)
		{
		}
		bool Cubemap::IsValid()
		{
			return Meta.Source != nullptr;
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
			CreateSections();
		}
		GraphicsDevice::~GraphicsDevice()
		{
			ReleaseProxy();
			for (auto It = Sections.begin(); It != Sections.end(); It++)
				TH_DELETE(Section, It->second);
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
		void GraphicsDevice::CreateStates()
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
			DepthStencilStates["less"] = CreateDepthStencilState(DepthStencil);

			DepthStencil.DepthEnable = true;
			DepthStencil.DepthWriteMask = Graphics::DepthWrite_Zero;
			DepthStencil.DepthFunction = Graphics::Comparison_Less;
			DepthStencil.StencilEnable = true;
			DepthStencil.StencilReadMask = 0xFF;
			DepthStencil.StencilWriteMask = 0x0;
			DepthStencil.FrontFaceStencilFailOperation = Graphics::StencilOperation_Keep;
			DepthStencil.FrontFaceStencilDepthFailOperation = Graphics::StencilOperation_Add;
			DepthStencil.FrontFaceStencilPassOperation = Graphics::StencilOperation_Keep;
			DepthStencil.FrontFaceStencilFunction = Graphics::Comparison_Always;
			DepthStencil.BackFaceStencilFailOperation = Graphics::StencilOperation_Keep;
			DepthStencil.BackFaceStencilDepthFailOperation = Graphics::StencilOperation_Subtract;
			DepthStencil.BackFaceStencilPassOperation = Graphics::StencilOperation_Keep;
			DepthStencil.BackFaceStencilFunction = Graphics::Comparison_Always;
			DepthStencilStates["less-read-only"] = CreateDepthStencilState(DepthStencil);

			DepthStencil.DepthEnable = true;
			DepthStencil.DepthWriteMask = Graphics::DepthWrite_Zero;
			DepthStencil.DepthFunction = Graphics::Comparison_Greater_Equal;
			DepthStencil.StencilEnable = true;
			DepthStencil.StencilReadMask = 0xFF;
			DepthStencil.StencilWriteMask = 0x0;
			DepthStencil.FrontFaceStencilFailOperation = Graphics::StencilOperation_Keep;
			DepthStencil.FrontFaceStencilDepthFailOperation = Graphics::StencilOperation_Add;
			DepthStencil.FrontFaceStencilPassOperation = Graphics::StencilOperation_Keep;
			DepthStencil.FrontFaceStencilFunction = Graphics::Comparison_Always;
			DepthStencil.BackFaceStencilFailOperation = Graphics::StencilOperation_Keep;
			DepthStencil.BackFaceStencilDepthFailOperation = Graphics::StencilOperation_Subtract;
			DepthStencil.BackFaceStencilPassOperation = Graphics::StencilOperation_Keep;
			DepthStencil.BackFaceStencilFunction = Graphics::Comparison_Always;
			DepthStencilStates["greater-read-only"] = CreateDepthStencilState(DepthStencil);

			DepthStencil.DepthEnable = true;
			DepthStencil.DepthWriteMask = Graphics::DepthWrite_All;
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
			DepthStencilStates["greater-equal"] = CreateDepthStencilState(DepthStencil);

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
			DepthStencilStates["none"] = CreateDepthStencilState(DepthStencil);

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
			DepthStencilStates["less-none"] = CreateDepthStencilState(DepthStencil);

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
			DepthStencilStates["less-no-stencil"] = CreateDepthStencilState(DepthStencil);

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
			RasterizerStates["cull-back"] = CreateRasterizerState(Rasterizer);

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
			RasterizerStates["cull-front"] = CreateRasterizerState(Rasterizer);

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
			RasterizerStates["cull-none"] = CreateRasterizerState(Rasterizer);

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
			RasterizerStates["cull-none-scissor"] = CreateRasterizerState(Rasterizer);

			Rasterizer.AntialiasedLineEnable = false;
			Rasterizer.CullMode = Graphics::VertexCull_Back;
			Rasterizer.DepthBias = 0;
			Rasterizer.DepthBiasClamp = 0;
			Rasterizer.DepthClipEnable = true;
			Rasterizer.FillMode = Graphics::SurfaceFill_Solid;
			Rasterizer.FrontCounterClockwise = false;
			Rasterizer.MultisampleEnable = false;
			Rasterizer.ScissorEnable = true;
			Rasterizer.SlopeScaledDepthBias = 0.0f;
			RasterizerStates["cull-back-scissor"] = CreateRasterizerState(Rasterizer);

			Graphics::BlendState::Desc Blend;
			Blend.AlphaToCoverageEnable = false;
			Blend.IndependentBlendEnable = false;
			Blend.RenderTarget[0].BlendEnable = false;
			Blend.RenderTarget[0].RenderTargetWriteMask = Graphics::ColorWriteEnable_All;
			BlendStates["overwrite"] = CreateBlendState(Blend);

			Blend.AlphaToCoverageEnable = false;
			Blend.IndependentBlendEnable = false;
			Blend.RenderTarget[0].BlendEnable = false;
			Blend.RenderTarget[0].RenderTargetWriteMask = Graphics::ColorWriteEnable_Red | Graphics::ColorWriteEnable_Green | Graphics::ColorWriteEnable_Blue;
			BlendStates["overwrite-opaque"] = CreateBlendState(Blend);

			Blend.AlphaToCoverageEnable = false;
			Blend.IndependentBlendEnable = false;
			Blend.RenderTarget[0].BlendEnable = false;
			Blend.RenderTarget[0].RenderTargetWriteMask = 0;
			BlendStates["overwrite-colorless"] = CreateBlendState(Blend);

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
			BlendStates["additive"] = CreateBlendState(Blend);

			Blend.AlphaToCoverageEnable = false;
			Blend.IndependentBlendEnable = false;
			Blend.RenderTarget[0].BlendEnable = true;
			Blend.RenderTarget[0].SrcBlend = Graphics::Blend_One;
			Blend.RenderTarget[0].DestBlend = Graphics::Blend_One;
			Blend.RenderTarget[0].BlendOperationMode = Graphics::BlendOperation_Add;
			Blend.RenderTarget[0].SrcBlendAlpha = Graphics::Blend_One;
			Blend.RenderTarget[0].DestBlendAlpha = Graphics::Blend_One;
			Blend.RenderTarget[0].BlendOperationAlpha = Graphics::BlendOperation_Add;
			Blend.RenderTarget[0].RenderTargetWriteMask = Graphics::ColorWriteEnable_Red | Graphics::ColorWriteEnable_Green | Graphics::ColorWriteEnable_Blue;
			BlendStates["additive-opaque"] = CreateBlendState(Blend);

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
			BlendStates["additive-gbuffer"] = CreateBlendState(Blend);

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
			BlendStates["additive-alpha"] = CreateBlendState(Blend);

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
			BlendStates["additive-source"] = CreateBlendState(Blend);

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
			SamplerStates["trilinear-x16"] = CreateSamplerState(Sampler);

			Sampler.Filter = Graphics::PixelFilter_Min_Mag_Mip_Linear;
			Sampler.AddressU = Graphics::TextureAddress_Clamp;
			Sampler.AddressV = Graphics::TextureAddress_Clamp;
			Sampler.AddressW = Graphics::TextureAddress_Clamp;
			Sampler.MipLODBias = 0.0f;
			Sampler.ComparisonFunction = Graphics::Comparison_Always;
			Sampler.MinLOD = 0.0f;
			Sampler.MaxLOD = std::numeric_limits<float>::max();
			SamplerStates["linear"] = CreateSamplerState(Sampler);

			Sampler.Filter = Graphics::PixelFilter_Min_Mag_Mip_Point;
			Sampler.AddressU = Graphics::TextureAddress_Clamp;
			Sampler.AddressV = Graphics::TextureAddress_Clamp;
			Sampler.AddressW = Graphics::TextureAddress_Clamp;
			Sampler.MipLODBias = 0.0f;
			Sampler.ComparisonFunction = Graphics::Comparison_Never;
			Sampler.MinLOD = 0.0f;
			Sampler.MaxLOD = std::numeric_limits<float>::max();
			SamplerStates["point"] = CreateSamplerState(Sampler);

			Sampler.Filter = Graphics::PixelFilter_Min_Mag_Mip_Point;
			Sampler.AddressU = Graphics::TextureAddress_Mirror;
			Sampler.AddressV = Graphics::TextureAddress_Mirror;
			Sampler.AddressW = Graphics::TextureAddress_Mirror;
			Sampler.MipLODBias = 0.0f;
			Sampler.MaxAnisotropy = 16;
			Sampler.ComparisonFunction = Graphics::Comparison_Never;
			Sampler.BorderColor[0] = 0.0f;
			Sampler.BorderColor[1] = 0.0f;
			Sampler.BorderColor[2] = 0.0f;
			Sampler.BorderColor[3] = 0.0f;
			Sampler.MinLOD = 0.0f;
			Sampler.MaxLOD = std::numeric_limits<float>::max();
			SamplerStates["shadow"] = CreateSamplerState(Sampler);

			InputLayout::Desc Layout;
			Layout.Attributes =
			{
				{ "POSITION", 0, AttributeType_Float, 3, 0 },
				{ "TEXCOORD", 0, AttributeType_Float, 2, 3 * sizeof(float) }
			};
			InputLayouts["shape-vertex"] = CreateInputLayout(Layout);

			Layout.Attributes =
			{
				{ "POSITION", 0, AttributeType_Float, 3, 0 },
				{ "TEXCOORD", 0, AttributeType_Float, 4, 3 * sizeof(float) },
				{ "TEXCOORD", 1, AttributeType_Float, 4, 7 * sizeof(float) },
				{ "TEXCOORD", 2, AttributeType_Float, 3, 11 * sizeof(float) }
			};
			InputLayouts["element-vertex"] = CreateInputLayout(Layout);

			Layout.Attributes =
			{
				{ "POSITION", 0, AttributeType_Float, 3, 0 },
				{ "TEXCOORD", 0, AttributeType_Float, 2, 3 * sizeof(float) },
				{ "NORMAL", 0, AttributeType_Float, 3, 5 * sizeof(float) },
				{ "TANGENT", 0, AttributeType_Float, 3, 8 * sizeof(float) },
				{ "BINORMAL", 0, AttributeType_Float, 3, 11 * sizeof(float) }
			};
			InputLayouts["vertex"] = CreateInputLayout(Layout);

			Layout.Attributes =
			{
				{ "POSITION", 0, AttributeType_Float, 3, 0 },
				{ "TEXCOORD", 0, AttributeType_Float, 2, 3 * sizeof(float) },
				{ "NORMAL", 0, AttributeType_Float, 3, 5 * sizeof(float) },
				{ "TANGENT", 0, AttributeType_Float, 3, 8 * sizeof(float) },
				{ "BINORMAL", 0, AttributeType_Float, 3, 11 * sizeof(float) },
				{ "JOINTBIAS", 0, AttributeType_Float, 4, 14 * sizeof(float) },
				{ "JOINTBIAS", 1, AttributeType_Float, 4, 18 * sizeof(float) }
			};
			InputLayouts["skin-vertex"] = CreateInputLayout(Layout);

			Layout.Attributes =
			{
				{ "POSITION", 0, AttributeType_Float, 2, 0 },
				{ "COLOR", 0, AttributeType_Ubyte, 4, 2 * sizeof(float) },
				{ "TEXCOORD", 0, AttributeType_Float, 2, 2 * sizeof(float) + 4 * sizeof(unsigned char) }
			};
			InputLayouts["gui-vertex"] = CreateInputLayout(Layout);

			SetDepthStencilState(GetDepthStencilState("less"));
			SetRasterizerState(GetRasterizerState("cull-back"));
			SetBlendState(GetBlendState("overwrite"));
			SetSamplerState(GetSamplerState("trilinear-x16"), 0, TH_PS);
		}
		void GraphicsDevice::CreateSections()
		{
#ifdef HAS_SHADER_BATCH
			shader_batch::foreach(this, [](void* Context, const char* Name, const unsigned char* Buffer, unsigned Size)
			{
				GraphicsDevice* Base = (GraphicsDevice*)Context;
				if (Base != nullptr && Base->GetBackend() != RenderBackend_NONE)
					Base->AddSection(Name, std::string((const char*)Buffer, Size));
			});
#else
			TH_WARN("default shader resources were not compiled");
#endif
		}
		void GraphicsDevice::ReleaseProxy()
		{
			for (auto It = DepthStencilStates.begin(); It != DepthStencilStates.end(); It++)
				TH_RELEASE(It->second);
			DepthStencilStates.clear();

			for (auto It = RasterizerStates.begin(); It != RasterizerStates.end(); It++)
				TH_RELEASE(It->second);
			RasterizerStates.clear();

			for (auto It = BlendStates.begin(); It != BlendStates.end(); It++)
				TH_RELEASE(It->second);
			BlendStates.clear();

			for (auto It = SamplerStates.begin(); It != SamplerStates.end(); It++)
				TH_RELEASE(It->second);
			SamplerStates.clear();

			for (auto It = InputLayouts.begin(); It != InputLayouts.end(); It++)
				TH_RELEASE(It->second);
			InputLayouts.clear();

			TH_CLEAR(RenderTarget);
			TH_CLEAR(BasicEffect);
		}
		bool GraphicsDevice::AddSection(const std::string& Name, const std::string& Code)
		{
			Core::Parser Language(Core::OS::Path::GetExtension(Name.c_str()));
			Language.Substring(1).Trim().ToLower();

			ShaderLang Lang = ShaderLang_NONE;
			if (Language.R() == "hlsl")
				Lang = ShaderLang_HLSL;
			else if (Language.R() == "glsl")
				Lang = ShaderLang_GLSL;
			else if (Language.R() == "msl")
				Lang = ShaderLang_MSL;
			else if (Language.R() == "spv")
				Lang = ShaderLang_SPV;

			RemoveSection(Name);
			if (Lang == ShaderLang_NONE)
			{
				TH_ERROR("shader resource is using unknown %s language:\n\t%s", Language.Get(), Name.c_str());
				return false;
			}

			Section* Include = TH_NEW(Section);
			Include->Code = Code;
			Include->Name = Name;
			Include->Lang = Lang;
			Sections[Name] = Include;

			return true;
		}
		bool GraphicsDevice::RemoveSection(const std::string& Name)
		{
			auto It = Sections.find(Name);
			if (It == Sections.end())
				return false;

			TH_DELETE(Section, It->second);
			Sections.erase(It);

			return true;
		}
		bool GraphicsDevice::Preprocess(Shader::Desc& Subresult)
		{
			if (Subresult.Data.empty())
				return true;

			Compute::IncludeDesc Desc = Compute::IncludeDesc();
			Desc.Exts.push_back(".hlsl");
			Desc.Exts.push_back(".glsl");
			Desc.Exts.push_back(".msl");
			Desc.Exts.push_back(".spv");
			Desc.Root = Core::OS::Directory::Get();
			Subresult.Features.Pragmas = false;

			Compute::Preprocessor* Processor = new Compute::Preprocessor();
			Processor->SetIncludeCallback([this, &Subresult](Compute::Preprocessor* P, const Compute::IncludeResult& File, std::string* Output)
			{
				if (Subresult.Include && Subresult.Include(P, File, Output))
					return true;

				if (File.Module.empty() || (!File.IsFile && !File.IsSystem))
					return false;

				if (File.IsSystem && !File.IsFile)
				{
					Section* Result;
					if (!GetSection(File.Module, &Result, true))
						return false;

					if (Result->Lang != Subresult.Lang)
					{
						TH_ERROR("mixed shader languages detected:\n\t%s", File.Module.c_str());
						return false;
					}

					Output->assign(Result->Code);
					return true;
				}

				uint64_t Length;
				unsigned char* Data = Core::OS::File::ReadAll(File.Module.c_str(), &Length);
				if (!Data)
					return false;

				Output->assign((const char*)Data, (size_t)Length);
				TH_FREE(Data);
				return true;
			});
			Processor->SetIncludeOptions(Desc);
			Processor->SetFeatures(Subresult.Features);

			for (auto& Word : Subresult.Defines)
				Processor->Define(Word);

			bool Preprocessed = Processor->Process(Subresult.Filename, Subresult.Data);
			TH_RELEASE(Processor);

			if (!Preprocessed)
				return false;

			switch (Backend)
			{
				case Tomahawk::Graphics::RenderBackend_D3D11:
					return Transpile(&Subresult.Data, Subresult.Lang, ShaderLang_HLSL);
				case Tomahawk::Graphics::RenderBackend_OGL:
					return Transpile(&Subresult.Data, Subresult.Lang, ShaderLang_GLSL);
				default:
					return false;
			}
		}
		bool GraphicsDevice::Transpile(std::string* Source, ShaderLang From, ShaderLang To)
		{
			if (!Source || Source->empty() || From == To)
				return true;
#ifdef TH_HAS_SPIRV

#else
			TH_ERROR("cannot transpile shader source without spirv-cross");
			return false;
#endif
		}
		bool GraphicsDevice::GetSection(const std::string& Name, Section** Result, bool Internal)
		{
			if (Name.empty() || Sections.empty())
			{
				if (!Internal)
					TH_ERROR("\n\tcould not find shader: \"%s\"");

				return false;
			}

			std::function<bool(const std::string&)> Resolve = [this, &Result](const std::string& Src)
			{
				auto It = Sections.find(Src);
				if (It == Sections.end())
					return false;

				if (Result != nullptr)
					*Result = It->second;

				return true;
			};

			if (Resolve(Name) ||
				Resolve(Name + ".hlsl") ||
				Resolve(Name + ".glsl") ||
				Resolve(Name + ".msl") ||
				Resolve(Name + ".spv"))
				return true;

			if (Result != nullptr)
				*Result = nullptr;

			if (!Internal)
				TH_ERROR("\n\tcould not find shader: \"%s\"", Name.c_str());

			return false;
		}
		bool GraphicsDevice::GetSection(const std::string& Name, Shader::Desc* Result)
		{
			if (Name.empty() || !Result)
				return false;

			Section* Subresult;
			if (!GetSection(Name, &Subresult, false))
				return false;

			Result->Filename.assign(Subresult->Name);
			Result->Data.assign(Subresult->Code);
			Result->Lang = Subresult->Lang;

			return true;
		}
		bool GraphicsDevice::IsDebug()
		{
			return Debug;
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
		unsigned int GraphicsDevice::GetPresentFlags()
		{
			return PresentFlags;
		}
		unsigned int GraphicsDevice::GetCompileFlags()
		{
			return CompileFlags;
		}
		std::string GraphicsDevice::GetShaderMain(ShaderType Type)
		{
			switch (Type)
			{
				case Tomahawk::Graphics::ShaderType_Vertex:
					return "vs_main";
				case Tomahawk::Graphics::ShaderType_Pixel:
					return "ps_main";
				case Tomahawk::Graphics::ShaderType_Geometry:
					return "gs_main";
				case Tomahawk::Graphics::ShaderType_Hull:
					return "hs_main";
				case Tomahawk::Graphics::ShaderType_Domain:
					return "ds_main";
				case Tomahawk::Graphics::ShaderType_Compute:
					return "cs_main";
				default:
					return "main";
			}
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
		InputLayout* GraphicsDevice::GetInputLayout(const std::string& Name)
		{
			auto It = InputLayouts.find(Name);
			if (It != InputLayouts.end())
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
		VSync GraphicsDevice::GetVSyncMode()
		{
			return VSyncMode;
		}
		GraphicsDevice* GraphicsDevice::Create(const Desc& I)
		{
#ifdef TH_MICROSOFT
			if (I.Backend == RenderBackend_D3D11)
				return new D3D11::D3D11Device(I);
#endif
#ifdef TH_HAS_GL
			if (I.Backend == RenderBackend_OGL)
				return new OGL::OGLDevice(I);
#endif
			TH_ERROR("backend was not found");
			return nullptr;
		}

		Activity::Activity(const Desc& I) : Handle(nullptr), Descriptor(I), Command(0), Message(this), CX(0), CY(0)
		{
#ifdef TH_HAS_SDL2
			Cursors[DisplayCursor_Arrow] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW);
			Cursors[DisplayCursor_TextInput] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_IBEAM);
			Cursors[DisplayCursor_ResizeAll] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEALL);
			Cursors[DisplayCursor_ResizeNS] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENS);
			Cursors[DisplayCursor_ResizeEW] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEWE);
			Cursors[DisplayCursor_ResizeNESW] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENESW);
			Cursors[DisplayCursor_ResizeNWSE] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENWSE);
			Cursors[DisplayCursor_Hand] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_HAND);
			Cursors[DisplayCursor_Crosshair] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_CROSSHAIR);
			Cursors[DisplayCursor_Wait] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_WAIT);
			Cursors[DisplayCursor_Progress] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_WAITARROW);
			Cursors[DisplayCursor_No] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_NO);
#endif
			memset(Keys[0], 0, 1024 * sizeof(bool));
			memset(Keys[1], 0, 1024 * sizeof(bool));

			Reset();
		}
		Activity::~Activity()
		{
#ifdef TH_HAS_SDL2
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
#ifdef TH_HAS_SDL2
			if (!Handle)
				return;

			SDL_SetClipboardText(Text.c_str());
#endif
		}
		void Activity::SetCursorPosition(const Compute::Vector2& Position)
		{
#ifdef TH_HAS_SDL2
#if SDL_VERSION_ATLEAST(2, 0, 4)
			SDL_WarpMouseInWindow(Handle, (int)Position.X, (int)Position.Y);
#endif
#endif
		}
		void Activity::SetCursorPosition(float X, float Y)
		{
#ifdef TH_HAS_SDL2
#if SDL_VERSION_ATLEAST(2, 0, 4)
			SDL_WarpMouseInWindow(Handle, (int)X, (int)Y);
#endif
#endif
		}
		void Activity::SetGlobalCursorPosition(const Compute::Vector2& Position)
		{
#ifdef TH_HAS_SDL2
			if (!Handle)
				return;
#if SDL_VERSION_ATLEAST(2, 0, 4)
			SDL_WarpMouseGlobal((int)Position.X, (int)Position.Y);
#endif
#endif
		}
		void Activity::SetGlobalCursorPosition(float X, float Y)
		{
#ifdef TH_HAS_SDL2
			if (!Handle)
				return;
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
#ifdef TH_HAS_SDL2
			if (Style != DisplayCursor_None)
			{
				SDL_ShowCursor(true);
				SDL_SetCursor(Cursors[Style]);
			}
			else
			{
				SDL_ShowCursor(false);
				SDL_SetCursor(Cursors[DisplayCursor_Arrow]);
			}
#endif
		}
		void Activity::SetCursorVisibility(bool Enabled)
		{
#ifdef TH_HAS_SDL2
			if (!Handle)
				return;

			SDL_ShowCursor(Enabled);
#endif
		}
		void Activity::SetGrabbing(bool Enabled)
		{
#ifdef TH_HAS_SDL2
			SDL_SetWindowGrab(Handle, Enabled ? SDL_TRUE : SDL_FALSE);
#endif
		}
		void Activity::SetFullscreen(bool Enabled)
		{
#ifdef TH_HAS_SDL2
			SDL_SetWindowFullscreen(Handle, Enabled ? SDL_WINDOW_FULLSCREEN : 0);
#endif
		}
		void Activity::SetBorderless(bool Enabled)
		{
#ifdef TH_HAS_SDL2
			SDL_SetWindowBordered(Handle, Enabled ? SDL_TRUE : SDL_FALSE);
#endif
		}
		void Activity::SetScreenKeyboard(bool Enabled)
		{
#ifdef TH_HAS_SDL2
			if (!Handle || !SDL_HasScreenKeyboardSupport())
				return;

			if (Enabled)
				SDL_StartTextInput();
			else
				SDL_StopTextInput();
#endif
		}
		void Activity::Reset()
		{
#ifdef TH_HAS_SDL2
			if (Handle != nullptr)
			{
				SDL_DestroyWindow(Handle);
				Handle = nullptr;
			}

			Uint32 Flags = SDL_WINDOW_OPENGL;
			if (Descriptor.Fullscreen)
				Flags |= SDL_WINDOW_FULLSCREEN;

			if (Descriptor.Hidden)
				Flags |= SDL_WINDOW_HIDDEN;

			if (Descriptor.Borderless)
				Flags |= SDL_WINDOW_BORDERLESS;

			if (Descriptor.Resizable)
				Flags |= SDL_WINDOW_RESIZABLE;

			if (Descriptor.Minimized)
				Flags |= SDL_WINDOW_MINIMIZED;

			if (Descriptor.Maximized)
				Flags |= SDL_WINDOW_MAXIMIZED;

			if (Descriptor.Focused)
				Flags |= SDL_WINDOW_INPUT_GRABBED;

			if (Descriptor.AllowHighDPI)
				Flags |= SDL_WINDOW_ALLOW_HIGHDPI;

			if (Descriptor.Centered)
			{
				Descriptor.X = SDL_WINDOWPOS_CENTERED;
				Descriptor.Y = SDL_WINDOWPOS_CENTERED;
			}
			else if (Descriptor.FreePosition)
			{
				Descriptor.X = SDL_WINDOWPOS_UNDEFINED;
				Descriptor.Y = SDL_WINDOWPOS_UNDEFINED;
			}

			Handle = SDL_CreateWindow(Descriptor.Title, Descriptor.X, Descriptor.Y, Descriptor.Width, Descriptor.Height, Flags);
#endif
		}
		void Activity::Hide()
		{
#ifdef TH_HAS_SDL2
			SDL_HideWindow(Handle);
#endif
		}
		void Activity::Show()
		{
#ifdef TH_HAS_SDL2
			SDL_ShowWindow(Handle);
#endif
		}
		void Activity::Maximize()
		{
#ifdef TH_HAS_SDL2
			SDL_MaximizeWindow(Handle);
#endif
		}
		void Activity::Minimize()
		{
#ifdef TH_HAS_SDL2
			SDL_MinimizeWindow(Handle);
#endif
		}
		void Activity::Focus()
		{
#ifdef TH_HAS_SDL2
#if SDL_VERSION_ATLEAST(2, 0, 5)
			SDL_SetWindowInputFocus(Handle);
#endif
#endif
		}
		void Activity::Move(int X, int Y)
		{
#ifdef TH_HAS_SDL2
			SDL_SetWindowPosition(Handle, X, Y);
#endif
		}
		void Activity::Resize(int W, int H)
		{
#ifdef TH_HAS_SDL2
			SDL_SetWindowSize(Handle, W, H);
#endif
		}
		void Activity::SetTitle(const char* Value)
		{
#ifdef TH_HAS_SDL2
			if (Value != nullptr)
				SDL_SetWindowTitle(Handle, Value);
#endif
		}
		void Activity::SetIcon(Surface* Icon)
		{
#ifdef TH_HAS_SDL2
			SDL_SetWindowIcon(Handle, (SDL_Surface*)Icon->GetResource());
#endif
		}
		void Activity::Load(SDL_SysWMinfo* Base)
		{
#ifdef TH_HAS_SDL2
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
#ifdef TH_HAS_SDL2
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
						Mapping.Mapped = true;
						Mapping.Captured = false;
					}

					return true;
				case SDL_KEYUP:
					if (Callbacks.KeyState && Id == Event.window.windowID)
						Callbacks.KeyState((KeyCode)Event.key.keysym.scancode, (KeyMod)Event.key.keysym.mod, (int)Event.key.keysym.sym, (int)(Event.key.repeat != 0), false);

					if (Mapping.Enabled && Mapping.Mapped && Mapping.Key.Key == (KeyCode)Event.key.keysym.scancode)
					{
						Mapping.Key.Mod = (KeyMod)SDL_GetModState();
						Mapping.Captured = true;
					}

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
							{
								Mapping.Key.Mod = (KeyMod)SDL_GetModState();
								Mapping.Captured = true;
							}

							return true;
						case SDL_BUTTON_MIDDLE:
							if (Callbacks.KeyState && Id == Event.window.windowID)
								Callbacks.KeyState(KeyCode_CURSORMIDDLE, (KeyMod)SDL_GetModState(), (int)KeyCode_CURSORMIDDLE, (int)Event.button.clicks, false);

							if (Mapping.Enabled && Mapping.Mapped && Mapping.Key.Key == KeyCode_CURSORMIDDLE)
							{
								Mapping.Key.Mod = (KeyMod)SDL_GetModState();
								Mapping.Captured = true;
							}

							return true;
						case SDL_BUTTON_RIGHT:
							if (Callbacks.KeyState && Id == Event.window.windowID)
								Callbacks.KeyState(KeyCode_CURSORRIGHT, (KeyMod)SDL_GetModState(), (int)KeyCode_CURSORRIGHT, (int)Event.button.clicks, false);

							if (Mapping.Enabled && Mapping.Mapped && Mapping.Key.Key == KeyCode_CURSORRIGHT)
							{
								Mapping.Key.Mod = (KeyMod)SDL_GetModState();
								Mapping.Captured = true;
							}

							return true;
						case SDL_BUTTON_X1:
							if (Callbacks.KeyState && Id == Event.window.windowID)
								Callbacks.KeyState(KeyCode_CURSORX1, (KeyMod)SDL_GetModState(), (int)KeyCode_CURSORX1, (int)Event.button.clicks, false);

							if (Mapping.Enabled && Mapping.Mapped && Mapping.Key.Key == KeyCode_CURSORX1)
							{
								Mapping.Key.Mod = (KeyMod)SDL_GetModState();
								Mapping.Captured = true;
							}

							return true;
						case SDL_BUTTON_X2:
							if (Callbacks.KeyState && Id == Event.window.windowID)
								Callbacks.KeyState(KeyCode_CURSORX2, (KeyMod)SDL_GetModState(), (int)KeyCode_CURSORX2, (int)Event.button.clicks, false);

							if (Mapping.Enabled && Mapping.Mapped && Mapping.Key.Key == KeyCode_CURSORX2)
							{
								Mapping.Key.Mod = (KeyMod)SDL_GetModState();
								Mapping.Captured = true;
							}

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
#ifdef TH_HAS_SDL2
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
				if (Keys[0][i])
					return true;
			}

			return false;
		}
		bool Activity::IsKeyDown(const KeyMap& Key)
		{
#ifdef TH_HAS_SDL2
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
#ifdef TH_HAS_SDL2
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
#ifdef TH_HAS_SDL2
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
		bool Activity::IsScreenKeyboardEnabled()
		{
#ifdef TH_HAS_SDL2
			if (!Handle)
				return false;

			return SDL_IsScreenKeyboardShown(Handle);
#else
			return false;
#endif
		}
		uint32_t Activity::GetX()
		{
#ifdef TH_HAS_SDL2
			int X, Y;
			SDL_GetWindowPosition(Handle, &X, &Y);

			return X;
#else
			return 0;
#endif
		}
		uint32_t Activity::GetY()
		{
#ifdef TH_HAS_SDL2
			int X, Y;
			SDL_GetWindowPosition(Handle, &X, &Y);

			return Y;
#else
			return 0;
#endif
		}
		uint32_t Activity::GetWidth()
		{
#ifdef TH_HAS_SDL2
			int W, H;
			SDL_GetWindowSize(Handle, &W, &H);

			return W;
#else
			return 0;
#endif
		}
		uint32_t Activity::GetHeight()
		{
#ifdef TH_HAS_SDL2
			int W, H;
			SDL_GetWindowSize(Handle, &W, &H);

			return H;
#else
			return 0;
#endif
		}
		float Activity::GetAspectRatio()
		{
#ifdef TH_HAS_SDL2
			int W, H;
			SDL_GetWindowSize(Handle, &W, &H);

			return (H > 0 ? (float)W / (float)H : 0.0f);
#else
			return 0.0f;
#endif
		}
		KeyMod Activity::GetKeyModState()
		{
#ifdef TH_HAS_SDL2
			if (!Handle)
				return KeyMod_NONE;

			return (KeyMod)SDL_GetModState();
#else
			return KeyMod_NONE;
#endif
		}
		Graphics::Viewport Activity::GetViewport()
		{
#ifdef TH_HAS_SDL2
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
#ifdef TH_HAS_SDL2
			int W, H;
			SDL_GL_GetDrawableSize(Handle, &W, &H);

			return Compute::Vector2((float)W, (float)H);
#else
			return Compute::Vector2();
#endif
		}
		Compute::Vector2 Activity::GetClientSize()
		{
#ifdef TH_HAS_SDL2
			int W, H;
			SDL_GetWindowSize(Handle, &W, &H);

			return Compute::Vector2((float)W, (float)H);
#else
			return Compute::Vector2();
#endif
		}
		Compute::Vector2 Activity::GetOffset()
		{
#ifdef TH_HAS_SDL2
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
#ifdef TH_HAS_SDL2
			if (!Handle)
				return Compute::Vector2();

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
#ifdef TH_HAS_SDL2
			if (!Handle)
				return Compute::Vector2();

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
#ifdef TH_HAS_SDL2
			Compute::Vector2 Size = GetSize();
			return GetCursorPosition() * Compute::Vector2(ScreenWidth, ScreenHeight) / Size;
#else
			return Compute::Vector2();
#endif
		}
		Compute::Vector2 Activity::GetCursorPosition(Compute::Vector2 ScreenDimensions)
		{
#ifdef TH_HAS_SDL2
			Compute::Vector2 Size = GetSize();
			return GetCursorPosition() * ScreenDimensions / Size;
#else
			return Compute::Vector2();
#endif
		}
		std::string Activity::GetClipboardText()
		{
#ifdef TH_HAS_SDL2
			if (!Handle)
				return std::string();

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
#ifdef TH_HAS_SDL2
			if (!Handle)
				return "";

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
#ifdef TH_HAS_SDL2
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
		const char* Activity::GetKeyCodeName(KeyCode Code)
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
					Name = "Decimal";
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
		const char* Activity::GetKeyModName(KeyMod Code)
		{
			const char* Name;
			switch (Code)
			{
				case Tomahawk::Graphics::KeyMod_LSHIFT:
					Name = "Left Shift";
					break;
				case Tomahawk::Graphics::KeyMod_RSHIFT:
					Name = "Right Shift";
					break;
				case Tomahawk::Graphics::KeyMod_LCTRL:
					Name = "Left Ctrl";
					break;
				case Tomahawk::Graphics::KeyMod_RCTRL:
					Name = "Right Ctrl";
					break;
				case Tomahawk::Graphics::KeyMod_LALT:
					Name = "Left Alt";
					break;
				case Tomahawk::Graphics::KeyMod_RALT:
					Name = "Right Alt";
					break;
				case Tomahawk::Graphics::KeyMod_LGUI:
					Name = "Left Gui";
					break;
				case Tomahawk::Graphics::KeyMod_RGUI:
					Name = "Right Gui";
					break;
				case Tomahawk::Graphics::KeyMod_NUM:
					Name = "Num-lock";
					break;
				case Tomahawk::Graphics::KeyMod_CAPS:
					Name = "Caps-lock";
					break;
				case Tomahawk::Graphics::KeyMod_MODE:
					Name = "Mode";
					break;
				case Tomahawk::Graphics::KeyMod_SHIFT:
					Name = "Shift";
					break;
				case Tomahawk::Graphics::KeyMod_CTRL:
					Name = "Ctrl";
					break;
				case Tomahawk::Graphics::KeyMod_ALT:
					Name = "Alt";
					break;
				case Tomahawk::Graphics::KeyMod_GUI:
					Name = "Gui";
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
				TH_RELEASE(*It);
		}
		MeshBuffer* Model::FindMesh(const std::string& Name)
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
				TH_RELEASE(*It);
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