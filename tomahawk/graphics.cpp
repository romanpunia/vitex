#include "graphics.h"
#include "graphics/d3d11.h"
#include "graphics/ogl.h"
#include "resource.h"
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
		void Alert::Create(AlertType Type, const std::string& Title, const std::string& Text)
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

		void PoseBuffer::SetJoint(Compute::Joint* Root)
		{
			Pose[Root->Index] = Root->Transform;
			for (auto&& Child : Root->Childs)
				SetJoint(&Child);
		}
		void PoseBuffer::SetJointKeys(Compute::Joint* Root, std::vector<Compute::AnimatorKey>* Keys)
		{
			Compute::AnimatorKey* Key = &Keys->at(Root->Index);
			Key->Position = Root->Transform.Position();
			Key->Rotation = Root->Transform.Rotation();
			Key->Scale = Root->Transform.Scale();

			for (auto&& Child : Root->Childs)
				SetJoint(&Child);
		}
		bool PoseBuffer::ResetKeys(SkinModel* Model, std::vector<Compute::AnimatorKey>* Keys)
		{
			if (!Model || Model->Joints.empty() || !Keys)
				return false;

			for (auto&& Child : Model->Joints)
				SetJointKeys(&Child, Keys);

			return true;
		}
		bool PoseBuffer::Reset(SkinModel* Model)
		{
			if (!Model || Model->Joints.empty())
				return false;

			for (auto&& Child : Model->Joints)
				SetJoint(&Child);

			return true;
		}
		Compute::Matrix4x4 PoseBuffer::Offset(int64_t Index)
		{
			auto It = Pose.find(Index);
			if (It != Pose.end())
				return It->second;

			return Compute::Matrix4x4::Identity();
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

		Shader::Shader(GraphicsDevice* Device, const Desc& I)
		{
		}
		Shader* Shader::Create(GraphicsDevice* Device, const Desc& I)
		{
#ifdef THAWK_MICROSOFT
			if (Device && Device->GetBackend() == RenderBackend_D3D11)
				return new D3D11::D3D11Shader(Device, I);
#endif
#ifdef THAWK_HAS_GL
			if (Device && Device->GetBackend() == RenderBackend_OGL)
				return new OGL::OGLShader(Device, I);
#endif
			THAWK_ERROR("instance serialization wasn't found");
			return nullptr;
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

		ElementBuffer::ElementBuffer(GraphicsDevice* Device, const Desc& I)
		{
			Elements = I.ElementCount;
		}
		uint64_t ElementBuffer::GetElements()
		{
			return Elements;
		}
		ElementBuffer* ElementBuffer::Create(GraphicsDevice* Device, const Desc& I)
		{
#ifdef THAWK_MICROSOFT
			if (Device && Device->GetBackend() == RenderBackend_D3D11)
				return new D3D11::D3D11ElementBuffer(Device, I);
#endif
#ifdef THAWK_HAS_GL
			if (Device && Device->GetBackend() == RenderBackend_OGL)
				return new OGL::OGLElementBuffer(Device, I);
#endif
			THAWK_ERROR("instance serialization wasn't found");
			return nullptr;
		}

		StructureBuffer::StructureBuffer(GraphicsDevice* Device, const Desc& I)
		{
			Elements = I.ElementCount;
		}
		uint64_t StructureBuffer::GetElements()
		{
			return Elements;
		}
		StructureBuffer* StructureBuffer::Create(GraphicsDevice* Device, const Desc& I)
		{
#ifdef THAWK_MICROSOFT
			if (Device && Device->GetBackend() == RenderBackend_D3D11)
				return new D3D11::D3D11StructureBuffer(Device, I);
#endif
#ifdef THAWK_HAS_GL
			if (Device && Device->GetBackend() == RenderBackend_OGL)
				return new OGL::OGLStructureBuffer(Device, I);
#endif
			THAWK_ERROR("instance serialization wasn't found");
			return nullptr;
		}

		Texture2D::Texture2D(GraphicsDevice* Device)
		{
			Width = 512;
			Height = 512;
			MipLevels = 1;
			FormatMode = Graphics::Format_Invalid;
			Usage = Graphics::ResourceUsage_Default;
			AccessFlags = Graphics::CPUAccess_Invalid;
		}
		Texture2D::Texture2D(GraphicsDevice* Device, const Desc& I)
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
		Texture2D* Texture2D::Create(GraphicsDevice* Device)
		{
#ifdef THAWK_MICROSOFT
			if (Device && Device->GetBackend() == RenderBackend_D3D11)
				return new D3D11::D3D11Texture2D(Device);
#endif
#ifdef THAWK_HAS_GL
			if (Device && Device->GetBackend() == RenderBackend_OGL)
				return new OGL::OGLTexture2D(Device);
#endif
			THAWK_ERROR("instance serialization wasn't found");
			return nullptr;
		}
		Texture2D* Texture2D::Create(GraphicsDevice* Device, const Desc& I)
		{
#ifdef THAWK_MICROSOFT
			if (Device && Device->GetBackend() == RenderBackend_D3D11)
				return new D3D11::D3D11Texture2D(Device, I);
#endif
#ifdef THAWK_HAS_GL
			if (Device && Device->GetBackend() == RenderBackend_OGL)
				return new OGL::OGLTexture2D(Device, I);
#endif
			THAWK_ERROR("instance serialization wasn't found");
			return nullptr;
		}

		Texture3D::Texture3D(GraphicsDevice* Device)
		{
		}
		Texture3D* Texture3D::Create(GraphicsDevice* Device)
		{
#ifdef THAWK_MICROSOFT
			if (Device && Device->GetBackend() == RenderBackend_D3D11)
				return new D3D11::D3D11Texture3D(Device);
#endif
#ifdef THAWK_HAS_GL
			if (Device && Device->GetBackend() == RenderBackend_OGL)
				return new OGL::OGLTexture3D(Device);
#endif

			THAWK_ERROR("instance serialization wasn't found");
			return nullptr;
		}

		TextureCube::TextureCube(GraphicsDevice* Device)
		{
		}
		TextureCube::TextureCube(GraphicsDevice* Device, const Desc& I)
		{
		}
		TextureCube* TextureCube::Create(GraphicsDevice* Device)
		{
#ifdef THAWK_MICROSOFT
			if (Device && Device->GetBackend() == RenderBackend_D3D11)
				return new D3D11::D3D11TextureCube(Device);
#endif
#ifdef THAWK_HAS_GL
			if (Device && Device->GetBackend() == RenderBackend_OGL)
				return new OGL::OGLTextureCube(Device);
#endif
			THAWK_ERROR("instance serialization wasn't found");
			return nullptr;
		}
		TextureCube* TextureCube::Create(GraphicsDevice* Device, const Desc& I)
		{
#ifdef THAWK_MICROSOFT
			if (Device && Device->GetBackend() == RenderBackend_D3D11)
				return new D3D11::D3D11TextureCube(Device, I);
#endif
#ifdef THAWK_HAS_GL
			if (Device && Device->GetBackend() == RenderBackend_OGL)
				return new OGL::OGLTextureCube(Device, I);
#endif
			THAWK_ERROR("instance serialization wasn't found");
			return nullptr;
		}

		RenderTarget2D::RenderTarget2D(GraphicsDevice*, const Desc& I) : Resource(nullptr)
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
		RenderTarget2D* RenderTarget2D::Create(GraphicsDevice* Device, const Desc& I)
		{
#ifdef THAWK_MICROSOFT
			if (Device && Device->GetBackend() == RenderBackend_D3D11)
				return new D3D11::D3D11RenderTarget2D(Device, I);
#endif
#ifdef THAWK_HAS_GL
			if (Device && Device->GetBackend() == RenderBackend_OGL)
				return new OGL::OGLRenderTarget2D(Device, I);
#endif
			THAWK_ERROR("instance serialization wasn't found");
			return nullptr;
		}

		MultiRenderTarget2D::MultiRenderTarget2D(GraphicsDevice* Device, const Desc& I)
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
		Texture2D* MultiRenderTarget2D::GetTarget(int Target)
		{
			if (Target < 0 || Target > 7)
				return nullptr;

			return Resource[Target];
		}
		MultiRenderTarget2D* MultiRenderTarget2D::Create(GraphicsDevice* Device, const Desc& I)
		{
#ifdef THAWK_MICROSOFT
			if (Device && Device->GetBackend() == RenderBackend_D3D11)
				return new D3D11::D3D11MultiRenderTarget2D(Device, I);
#endif
#ifdef THAWK_HAS_GL
			if (Device && Device->GetBackend() == RenderBackend_OGL)
				return new OGL::OGLMultiRenderTarget2D(Device, I);
#endif
			THAWK_ERROR("instance serialization wasn't found");
			return nullptr;
		}

		RenderTarget2DArray::RenderTarget2DArray(GraphicsDevice*, const Desc& I) : Resource(nullptr)
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
		RenderTarget2DArray* RenderTarget2DArray::Create(GraphicsDevice* Device, const Desc& I)
		{
#ifdef THAWK_MICROSOFT
			if (Device && Device->GetBackend() == RenderBackend_D3D11)
				return new D3D11::D3D11RenderTarget2DArray(Device, I);
#endif
#ifdef THAWK_HAS_GL
			if (Device && Device->GetBackend() == RenderBackend_OGL)
				return new OGL::OGLRenderTarget2DArray(Device, I);
#endif
			THAWK_ERROR("instance serialization wasn't found");
			return nullptr;
		}

		RenderTargetCube::RenderTargetCube(GraphicsDevice*, const Desc& I) : Resource(nullptr)
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
		RenderTargetCube* RenderTargetCube::Create(GraphicsDevice* Device, const Desc& I)
		{
#ifdef THAWK_MICROSOFT
			if (Device && Device->GetBackend() == RenderBackend_D3D11)
				return new D3D11::D3D11RenderTargetCube(Device, I);
#endif
#ifdef THAWK_HAS_GL
			if (Device && Device->GetBackend() == RenderBackend_OGL)
				return new OGL::OGLRenderTargetCube(Device, I);
#endif
			THAWK_ERROR("instance serialization wasn't found");
			return nullptr;
		}

		MultiRenderTargetCube::MultiRenderTargetCube(GraphicsDevice* Device, const Desc& I)
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
		Texture2D* MultiRenderTargetCube::GetTarget(int Target)
		{
			if (Target < 0 || Target > 7)
				return nullptr;

			return Resource[Target];
		}
		MultiRenderTargetCube* MultiRenderTargetCube::Create(GraphicsDevice* Device, const Desc& I)
		{
#ifdef THAWK_MICROSOFT
			if (Device && Device->GetBackend() == RenderBackend_D3D11)
				return new D3D11::D3D11MultiRenderTargetCube(Device, I);
#endif
#ifdef THAWK_HAS_GL
			if (Device && Device->GetBackend() == RenderBackend_OGL)
				return new OGL::OGLMultiRenderTargetCube(Device, I);
#endif
			THAWK_ERROR("instance serialization wasn't found");
			return nullptr;
		}

		Mesh::Mesh(GraphicsDevice*, const Desc& I) : VertexBuffer(nullptr), IndexBuffer(nullptr)
		{
		}
		Mesh::~Mesh()
		{
			delete VertexBuffer;
			delete IndexBuffer;
		}
		ElementBuffer* Mesh::GetVertexBuffer()
		{
			return VertexBuffer;
		}
		ElementBuffer* Mesh::GetIndexBuffer()
		{
			return IndexBuffer;
		}
		Mesh* Mesh::Create(GraphicsDevice* Device, const Desc& I)
		{
#ifdef THAWK_MICROSOFT
			if (Device && Device->GetBackend() == RenderBackend_D3D11)
				return new D3D11::D3D11Mesh(Device, I);
#endif
#ifdef THAWK_HAS_GL
			if (Device && Device->GetBackend() == RenderBackend_OGL)
				return new OGL::OGLMesh(Device, I);
#endif
			THAWK_ERROR("instance serialization wasn't found");
			return nullptr;
		}

		SkinMesh::SkinMesh(GraphicsDevice*, const Desc& I) : VertexBuffer(nullptr), IndexBuffer(nullptr)
		{
		}
		SkinMesh::~SkinMesh()
		{
			delete VertexBuffer;
			delete IndexBuffer;
		}
		ElementBuffer* SkinMesh::GetVertexBuffer()
		{
			return VertexBuffer;
		}
		ElementBuffer* SkinMesh::GetIndexBuffer()
		{
			return IndexBuffer;
		}
		SkinMesh* SkinMesh::Create(GraphicsDevice* Device, const Desc& I)
		{
#ifdef THAWK_MICROSOFT
			if (Device && Device->GetBackend() == RenderBackend_D3D11)
				return new D3D11::D3D11SkinMesh(Device, I);
#endif
#ifdef THAWK_HAS_GL
			if (Device && Device->GetBackend() == RenderBackend_OGL)
				return new OGL::OGLSkinMesh(Device, I);
#endif
			THAWK_ERROR("instance serialization wasn't found");
			return nullptr;
		}

		Model::Model()
		{
		}
		Model::~Model()
		{
			for (auto It = Meshes.begin(); It != Meshes.end(); It++)
				delete (*It);
		}
		Mesh* Model::Find(const std::string& Name)
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
		void SkinModel::BuildSkeleton(PoseBuffer* Map)
		{
			if (Map != nullptr && !Joints.empty())
			{
				if (Map->Pose.empty())
					Map->Reset(this);

				for (auto& Child : Joints)
					BuildSkeleton(Map, &Child, Compute::Matrix4x4::Identity());
			}
		}
		void SkinModel::BuildSkeleton(PoseBuffer* Map, Compute::Joint* Base, const Compute::Matrix4x4& World)
		{
			Compute::Matrix4x4 Transform = Map->Offset(Base->Index) * World;
			Map->Transform[Base->Index] = Base->BindShape * Transform * Root;

			for (auto& Child : Base->Childs)
				BuildSkeleton(Map, &Child, Transform);
		}
		SkinMesh* SkinModel::FindMesh(const std::string& Name)
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

		InstanceBuffer::InstanceBuffer(GraphicsDevice* NewDevice, const Desc& I) : Device(NewDevice), Elements(nullptr)
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
		InstanceBuffer* InstanceBuffer::Create(GraphicsDevice* Device, const Desc& I)
		{
#ifdef THAWK_MICROSOFT
			if (Device && Device->GetBackend() == RenderBackend_D3D11)
				return new D3D11::D3D11InstanceBuffer(Device, I);
#endif
#ifdef THAWK_HAS_GL
			if (Device && Device->GetBackend() == RenderBackend_OGL)
				return new OGL::OGLInstanceBuffer(Device, I);
#endif
			THAWK_ERROR("instance serialization wasn't found");
			return nullptr;
		}

		DirectBuffer::DirectBuffer(GraphicsDevice* NewDevice) : Device(NewDevice), MaxElements(1), View(nullptr), Primitives(PrimitiveTopology_Triangle_List)
		{
			Elements.reserve(1);
		}
		DirectBuffer::~DirectBuffer()
		{
		}
		DirectBuffer* DirectBuffer::Create(GraphicsDevice* Device)
		{
#ifdef THAWK_MICROSOFT
			if (Device && Device->GetBackend() == RenderBackend_D3D11)
				return new D3D11::D3D11DirectBuffer(Device);
#endif
#ifdef THAWK_HAS_GL
			if (Device && Device->GetBackend() == RenderBackend_OGL)
				return new OGL::OGLDirectBuffer(Device);
#endif
			THAWK_ERROR("instance serialization wasn't found");
			return nullptr;
		}

		GraphicsDevice::GraphicsDevice(const Desc& I)
		{
			ShaderModelType = Graphics::ShaderModel_Invalid;
			VSyncMode = I.VSyncMode;
			PresentationFlag = I.PresentationFlags;
			CompilationFlag = I.CompilationFlags;
			Backend = I.Backend;
			Debug = I.Debug;
#ifdef THAWK_MICROSOFT
			RECT Desktop;
			HWND Handle = GetDesktopWindow();
			GetWindowRect(Handle, &Desktop);

			ScreenDimensions.X = (float)Desktop.right;
			ScreenDimensions.Y = (float)Desktop.bottom;
#endif
			LoadSections();
		}
		GraphicsDevice::~GraphicsDevice()
		{
			FreeProxy();
			for (auto It = Sections.begin(); It != Sections.end(); It++)
				delete *It;
		}
		bool GraphicsDevice::ProcessShaderCode(Shader::Desc& In)
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
				{
					std::string* Result = GetSection(File.Module);
					if (!Result)
						return false;

					Output->assign(*Result);
					return true;
				}

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
		void GraphicsDevice::AddSection(const std::string& Name, const std::string& Code)
		{
			Section* Include = new Section();
			Include->Code = Code;
			Include->Name = Name;

			RemoveSection(Name);
			Sections.push_back(Include);
		}
		void GraphicsDevice::RemoveSection(const std::string& Name)
		{
			for (auto It = Sections.begin(); It != Sections.end(); It++)
			{
				if ((*It)->Name == Name)
				{
					delete *It;
					Sections.erase(It);
					break;
				}
			}
		}
		void GraphicsDevice::Lock()
		{
			Mutex.lock();
		}
		void GraphicsDevice::Unlock()
		{
			Mutex.unlock();
		}
		void GraphicsDevice::CreateRendererStates()
		{
			Graphics::DepthStencilState* DepthStencilConfig = new Graphics::DepthStencilState();
			DepthStencilConfig->DepthEnable = true;
			DepthStencilConfig->DepthWriteMask = Graphics::DepthWrite_All;
			DepthStencilConfig->DepthFunction = Graphics::Comparison_Less;
			DepthStencilConfig->StencilEnable = true;
			DepthStencilConfig->StencilReadMask = 0xFF;
			DepthStencilConfig->StencilWriteMask = 0xFF;
			DepthStencilConfig->FrontFaceStencilFailOperation = Graphics::StencilOperation_Keep;
			DepthStencilConfig->FrontFaceStencilDepthFailOperation = Graphics::StencilOperation_Add;
			DepthStencilConfig->FrontFaceStencilPassOperation = Graphics::StencilOperation_Keep;
			DepthStencilConfig->FrontFaceStencilFunction = Graphics::Comparison_Always;
			DepthStencilConfig->BackFaceStencilFailOperation = Graphics::StencilOperation_Keep;
			DepthStencilConfig->BackFaceStencilDepthFailOperation = Graphics::StencilOperation_Subtract;
			DepthStencilConfig->BackFaceStencilPassOperation = Graphics::StencilOperation_Keep;
			DepthStencilConfig->BackFaceStencilFunction = Graphics::Comparison_Always;
			AddDepthStencilState(DepthStencilConfig);

			DepthStencilConfig = new Graphics::DepthStencilState();
			DepthStencilConfig->DepthEnable = true;
			DepthStencilConfig->DepthWriteMask = Graphics::DepthWrite_Zero;
			DepthStencilConfig->DepthFunction = Graphics::Comparison_Greater_Equal;
			DepthStencilConfig->StencilEnable = true;
			DepthStencilConfig->StencilReadMask = 0xFF;
			DepthStencilConfig->StencilWriteMask = 0xFF;
			DepthStencilConfig->FrontFaceStencilFailOperation = Graphics::StencilOperation_Keep;
			DepthStencilConfig->FrontFaceStencilDepthFailOperation = Graphics::StencilOperation_Add;
			DepthStencilConfig->FrontFaceStencilPassOperation = Graphics::StencilOperation_Keep;
			DepthStencilConfig->FrontFaceStencilFunction = Graphics::Comparison_Always;
			DepthStencilConfig->BackFaceStencilFailOperation = Graphics::StencilOperation_Keep;
			DepthStencilConfig->BackFaceStencilDepthFailOperation = Graphics::StencilOperation_Subtract;
			DepthStencilConfig->BackFaceStencilPassOperation = Graphics::StencilOperation_Keep;
			DepthStencilConfig->BackFaceStencilFunction = Graphics::Comparison_Always;
			AddDepthStencilState(DepthStencilConfig);

			DepthStencilConfig = new Graphics::DepthStencilState();
			DepthStencilConfig->DepthEnable = false;
			DepthStencilConfig->DepthWriteMask = Graphics::DepthWrite_All;
			DepthStencilConfig->DepthFunction = Graphics::Comparison_Less;
			DepthStencilConfig->StencilEnable = true;
			DepthStencilConfig->StencilReadMask = 0xFF;
			DepthStencilConfig->StencilWriteMask = 0xFF;
			DepthStencilConfig->FrontFaceStencilFailOperation = Graphics::StencilOperation_Keep;
			DepthStencilConfig->FrontFaceStencilDepthFailOperation = Graphics::StencilOperation_Add;
			DepthStencilConfig->FrontFaceStencilPassOperation = Graphics::StencilOperation_Keep;
			DepthStencilConfig->FrontFaceStencilFunction = Graphics::Comparison_Always;
			DepthStencilConfig->BackFaceStencilFailOperation = Graphics::StencilOperation_Keep;
			DepthStencilConfig->BackFaceStencilDepthFailOperation = Graphics::StencilOperation_Subtract;
			DepthStencilConfig->BackFaceStencilPassOperation = Graphics::StencilOperation_Keep;
			DepthStencilConfig->BackFaceStencilFunction = Graphics::Comparison_Always;
			AddDepthStencilState(DepthStencilConfig);

			DepthStencilConfig = new Graphics::DepthStencilState();
			DepthStencilConfig->DepthEnable = true;
			DepthStencilConfig->DepthWriteMask = Graphics::DepthWrite_Zero;
			DepthStencilConfig->DepthFunction = Graphics::Comparison_Less;
			DepthStencilConfig->StencilEnable = true;
			DepthStencilConfig->StencilReadMask = 0xFF;
			DepthStencilConfig->StencilWriteMask = 0xFF;
			DepthStencilConfig->FrontFaceStencilFailOperation = Graphics::StencilOperation_Keep;
			DepthStencilConfig->FrontFaceStencilDepthFailOperation = Graphics::StencilOperation_Add;
			DepthStencilConfig->FrontFaceStencilPassOperation = Graphics::StencilOperation_Keep;
			DepthStencilConfig->FrontFaceStencilFunction = Graphics::Comparison_Always;
			DepthStencilConfig->BackFaceStencilFailOperation = Graphics::StencilOperation_Keep;
			DepthStencilConfig->BackFaceStencilDepthFailOperation = Graphics::StencilOperation_Subtract;
			DepthStencilConfig->BackFaceStencilPassOperation = Graphics::StencilOperation_Keep;
			DepthStencilConfig->BackFaceStencilFunction = Graphics::Comparison_Always;
			AddDepthStencilState(DepthStencilConfig);

			DepthStencilConfig = new Graphics::DepthStencilState();
			DepthStencilConfig->DepthEnable = false;
			DepthStencilConfig->DepthWriteMask = Graphics::DepthWrite_All;
			DepthStencilConfig->DepthFunction = Graphics::Comparison_Always;
			DepthStencilConfig->StencilEnable = false;
			DepthStencilConfig->StencilReadMask = 0xFF;
			DepthStencilConfig->StencilWriteMask = 0xFF;
			DepthStencilConfig->FrontFaceStencilFailOperation = Graphics::StencilOperation_Keep;
			DepthStencilConfig->FrontFaceStencilDepthFailOperation = Graphics::StencilOperation_Keep;
			DepthStencilConfig->FrontFaceStencilPassOperation = Graphics::StencilOperation_Keep;
			DepthStencilConfig->FrontFaceStencilFunction = Graphics::Comparison_Always;
			AddDepthStencilState(DepthStencilConfig);

			Graphics::RasterizerState* RasterizerConfig = new Graphics::RasterizerState();
			RasterizerConfig->AntialiasedLineEnable = false;
			RasterizerConfig->CullMode = Graphics::VertexCull_Back;
			RasterizerConfig->DepthBias = 0;
			RasterizerConfig->DepthBiasClamp = 0;
			RasterizerConfig->DepthClipEnable = true;
			RasterizerConfig->FillMode = Graphics::SurfaceFill_Solid;
			RasterizerConfig->FrontCounterClockwise = false;
			RasterizerConfig->MultisampleEnable = false;
			RasterizerConfig->ScissorEnable = false;
			RasterizerConfig->SlopeScaledDepthBias = 0.0f;
			AddRasterizerState(RasterizerConfig);

			RasterizerConfig = new Graphics::RasterizerState();
			RasterizerConfig->AntialiasedLineEnable = false;
			RasterizerConfig->CullMode = Graphics::VertexCull_Front;
			RasterizerConfig->DepthBias = 0;
			RasterizerConfig->DepthBiasClamp = 0;
			RasterizerConfig->DepthClipEnable = true;
			RasterizerConfig->FillMode = Graphics::SurfaceFill_Solid;
			RasterizerConfig->FrontCounterClockwise = false;
			RasterizerConfig->MultisampleEnable = false;
			RasterizerConfig->ScissorEnable = false;
			RasterizerConfig->SlopeScaledDepthBias = 0.0f;
			AddRasterizerState(RasterizerConfig);

			RasterizerConfig = new Graphics::RasterizerState();
			RasterizerConfig->AntialiasedLineEnable = false;
			RasterizerConfig->CullMode = Graphics::VertexCull_Disabled;
			RasterizerConfig->DepthBias = 0;
			RasterizerConfig->DepthBiasClamp = 0;
			RasterizerConfig->DepthClipEnable = true;
			RasterizerConfig->FillMode = Graphics::SurfaceFill_Solid;
			RasterizerConfig->FrontCounterClockwise = false;
			RasterizerConfig->MultisampleEnable = false;
			RasterizerConfig->ScissorEnable = false;
			RasterizerConfig->SlopeScaledDepthBias = 0.0f;
			AddRasterizerState(RasterizerConfig);

			RasterizerConfig = new Graphics::RasterizerState();
			RasterizerConfig->AntialiasedLineEnable = false;
			RasterizerConfig->CullMode = Graphics::VertexCull_Disabled;
			RasterizerConfig->DepthBias = 0;
			RasterizerConfig->DepthBiasClamp = 0;
			RasterizerConfig->DepthClipEnable = true;
			RasterizerConfig->FillMode = Graphics::SurfaceFill_Solid;
			RasterizerConfig->FrontCounterClockwise = false;
			RasterizerConfig->MultisampleEnable = false;
			RasterizerConfig->ScissorEnable = true;
			RasterizerConfig->SlopeScaledDepthBias = 0.0f;
			AddRasterizerState(RasterizerConfig);

			Graphics::BlendState* BlendConfig = new Graphics::BlendState();
			BlendConfig->AlphaToCoverageEnable = false;
			BlendConfig->IndependentBlendEnable = false;
			BlendConfig->RenderTarget[0].BlendEnable = false;
			BlendConfig->RenderTarget[0].RenderTargetWriteMask = Graphics::ColorWriteEnable_All;
			AddBlendState(BlendConfig);

			BlendConfig = new Graphics::BlendState();
			BlendConfig->AlphaToCoverageEnable = false;
			BlendConfig->IndependentBlendEnable = false;
			BlendConfig->RenderTarget[0].BlendEnable = true;
			BlendConfig->RenderTarget[0].SrcBlend = Graphics::Blend_One;
			BlendConfig->RenderTarget[0].DestBlend = Graphics::Blend_One;
			BlendConfig->RenderTarget[0].BlendOperationMode = Graphics::BlendOperation_Add;
			BlendConfig->RenderTarget[0].SrcBlendAlpha = Graphics::Blend_One;
			BlendConfig->RenderTarget[0].DestBlendAlpha = Graphics::Blend_One;
			BlendConfig->RenderTarget[0].BlendOperationAlpha = Graphics::BlendOperation_Add;
			BlendConfig->RenderTarget[0].RenderTargetWriteMask = Graphics::ColorWriteEnable_All;
			AddBlendState(BlendConfig);

			BlendConfig = new Graphics::BlendState();
			BlendConfig->AlphaToCoverageEnable = false;
			BlendConfig->IndependentBlendEnable = false;
			BlendConfig->RenderTarget[0].BlendEnable = true;
			BlendConfig->RenderTarget[0].SrcBlend = Graphics::Blend_Source_Alpha;
			BlendConfig->RenderTarget[0].DestBlend = Graphics::Blend_One;
			BlendConfig->RenderTarget[0].BlendOperationMode = Graphics::BlendOperation_Add;
			BlendConfig->RenderTarget[0].SrcBlendAlpha = Graphics::Blend_One;
			BlendConfig->RenderTarget[0].DestBlendAlpha = Graphics::Blend_One;
			BlendConfig->RenderTarget[0].BlendOperationAlpha = Graphics::BlendOperation_Add;
			BlendConfig->RenderTarget[0].RenderTargetWriteMask = Graphics::ColorWriteEnable_All;
			AddBlendState(BlendConfig);

			BlendConfig = new Graphics::BlendState();
			BlendConfig->AlphaToCoverageEnable = false;
			BlendConfig->IndependentBlendEnable = false;
			BlendConfig->RenderTarget[0].BlendEnable = true;
			BlendConfig->RenderTarget[0].SrcBlend = Graphics::Blend_Source_Alpha;
			BlendConfig->RenderTarget[0].DestBlend = Graphics::Blend_Source_Alpha_Invert;
			BlendConfig->RenderTarget[0].BlendOperationMode = Graphics::BlendOperation_Add;
			BlendConfig->RenderTarget[0].SrcBlendAlpha = Graphics::Blend_Source_Alpha_Invert;
			BlendConfig->RenderTarget[0].DestBlendAlpha = Graphics::Blend_Zero;
			BlendConfig->RenderTarget[0].BlendOperationAlpha = Graphics::BlendOperation_Add;
			BlendConfig->RenderTarget[0].RenderTargetWriteMask = Graphics::ColorWriteEnable_All;
			AddBlendState(BlendConfig);

			Graphics::SamplerState* SamplerConfig = new Graphics::SamplerState();
			SamplerConfig->Filter = Graphics::PixelFilter_Anistropic;
			SamplerConfig->AddressU = Graphics::TextureAddress_Wrap;
			SamplerConfig->AddressV = Graphics::TextureAddress_Wrap;
			SamplerConfig->AddressW = Graphics::TextureAddress_Wrap;
			SamplerConfig->MipLODBias = 0.0f;
			SamplerConfig->MaxAnisotropy = 16;
			SamplerConfig->ComparisonFunction = Graphics::Comparison_Never;
			SamplerConfig->BorderColor[0] = 0.0f;
			SamplerConfig->BorderColor[1] = 0.0f;
			SamplerConfig->BorderColor[2] = 0.0f;
			SamplerConfig->BorderColor[3] = 0.0f;
			SamplerConfig->MinLOD = 0.0f;
			SamplerConfig->MaxLOD = std::numeric_limits<float>::max();
			AddSamplerState(SamplerConfig);

			SamplerConfig = new Graphics::SamplerState();
			SamplerConfig->Filter = Graphics::PixelFilter_Min_Mag_Mip_Linear;
			SamplerConfig->AddressU = Graphics::TextureAddress_Wrap;
			SamplerConfig->AddressV = Graphics::TextureAddress_Wrap;
			SamplerConfig->AddressW = Graphics::TextureAddress_Wrap;
			SamplerConfig->MipLODBias = 0.0f;
			SamplerConfig->ComparisonFunction = Graphics::Comparison_Always;
			SamplerConfig->MinLOD = 0.0f;
			SamplerConfig->MaxLOD = 0.0f;
			AddSamplerState(SamplerConfig);

			SetDepthStencilState(Graphics::RenderLab_Blend_Overwrite);
			SetRasterizerState(Graphics::RenderLab_Raster_Cull_Back);
			SetBlendState(Graphics::RenderLab_Blend_Overwrite);
			SetSamplerState(Graphics::RenderLab_Sampler_Trilinear_X16);
		}
		void GraphicsDevice::LoadSections()
		{
			if (Backend == RenderBackend_D3D11)
			{
#ifdef HAS_D3D11_GEOMETRY_DEFERRED_HLSL
				AddSection("geometry/deferred.hlsl", GET_RESOURCE_BATCH(d3d11_geometry_deferred_hlsl));
#else
				THAWK_ERROR("geometry/deferred.hlsl was not compiled");
#endif
#ifdef HAS_D3D11_GEOMETRY_FORWARD_HLSL
				AddSection("geometry/forward.hlsl", GET_RESOURCE_BATCH(d3d11_geometry_forward_hlsl));
#else
				THAWK_ERROR("geometry/forward.hlsl was not compiled");
#endif
#ifdef HAS_D3D11_GEOMETRY_MINIMAL_HLSL
				AddSection("geometry/minimal.hlsl", GET_RESOURCE_BATCH(d3d11_geometry_minimal_hlsl));
#else
				THAWK_ERROR("geometry/minimal.hlsl was not compiled");
#endif
#ifdef HAS_D3D11_GEOMETRY_SV_HLSL
				AddSection("geometry/sv.hlsl", GET_RESOURCE_BATCH(d3d11_geometry_sv_hlsl));
#else
				THAWK_ERROR("geometry/sv.hlsl was not compiled");
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
#ifdef HAS_D3D11_STANDARD_HEMI_AMBIENT_HLSL
				AddSection("standard/hemi-ambient.hlsl", GET_RESOURCE_BATCH(d3d11_standard_hemi_ambient_hlsl));
#else
				THAWK_ERROR("standard/hemi-ambient.hlsl was not compiled");
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
			}
			else if (Backend == RenderBackend_OGL)
			{
#ifdef HAS_OGL_GEOMETRY_DEFERRED_GLSL
			AddSection("geometry/deferred.glsl", GET_RESOURCE_BATCH(ogl_geometry_deferred_glsl));
#else
			THAWK_ERROR("geometry/deferred.glsl was not compiled");
#endif
#ifdef HAS_OGL_GEOMETRY_FORWARD_GLSL
			AddSection("geometry/forward.glsl", GET_RESOURCE_BATCH(ogl_geometry_forward_glsl));
#else
			THAWK_ERROR("geometry/forward.glsl was not compiled");
#endif
#ifdef HAS_OGL_GEOMETRY_MINIMAL_GLSL
			AddSection("geometry/minimal.glsl", GET_RESOURCE_BATCH(ogl_geometry_minimal_glsl));
#else
			THAWK_ERROR("geometry/minimal.glsl was not compiled");
#endif
#ifdef HAS_OGL_GEOMETRY_SV_GLSL
			AddSection("geometry/sv.glsl", GET_RESOURCE_BATCH(ogl_geometry_sv_glsl));
#else
			THAWK_ERROR("geometry/sv.glsl was not compiled");
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
			AddSection("renderer/element.glsl", GET_RESOURCE_BATCH(ogl_renderer_vertex_element_glsl));
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
#ifdef HAS_OGL_STANDARD_BASIC_HLSL
			AddSection("standard/basic.glsl", GET_RESOURCE_BATCH(ogl_standard_basic_glsl));
#else
			THAWK_ERROR("standard/basic.glsl was not compiled");
#endif
#ifdef HAS_OGL_STANDARD_COOK_TORRANCE_GLSL
			AddSection("standard/cook-torrance.glsl", GET_RESOURCE_BATCH(ogl_standard_cook_torrance_glsl));
#else
			THAWK_ERROR("standard/cook-torrance.glsl was not compiled");
#endif
#ifdef HAS_OGL_STANDARD_HEMI_AMBIENT_GLSL
			AddSection("standard/hemi-ambient.glsl", GET_RESOURCE_BATCH(ogl_standard_hemi_ambient_glsl));
#else
			THAWK_ERROR("standard/hemi-ambient.glsl was not compiled");
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
			}
		}
		std::string* GraphicsDevice::GetSection(const std::string& Name)
		{
			if (Name.empty() || Sections.empty())
				return nullptr;

			for (auto& It : Sections)
			{
				if (It->Name != Name)
				{
					if (It->Name != Name + ".hlsl" && It->Name != Name + ".glsl")
						continue;
				}

				return &It->Code;
			}

			return nullptr;
		}
		std::vector<GraphicsDevice::Section*> GraphicsDevice::GetSections()
		{
			return Sections;
		}
		unsigned int GraphicsDevice::GetMipLevelCount(unsigned int Width, unsigned int Height)
		{
			unsigned int MipLevels = 1;

			while (Width > 1 && Height > 1)
			{
				Width = (unsigned int)Compute::Math<float>::Min((float)Width / 2.0f, 1.0f);
				Height = (unsigned int)Compute::Math<float>::Min((float)Height / 2.0f, 1.0f);
				MipLevels++;
			}

			return MipLevels;
		}
		ShaderModel GraphicsDevice::GetShaderModel()
		{
			return ShaderModelType;
		}
		DepthStencilState* GraphicsDevice::GetDepthStencilState(uint64_t State)
		{
			if (State < 0 || State >= DepthStencilStates.size())
				return nullptr;

			return DepthStencilStates[State];
		}
		BlendState* GraphicsDevice::GetBlendState(uint64_t State)
		{
			if (State < 0 || State >= BlendStates.size())
				return nullptr;

			return BlendStates[State];
		}
		RasterizerState* GraphicsDevice::GetRasterizerState(uint64_t State)
		{
			if (State < 0 || State >= RasterizerStates.size())
				return nullptr;

			return RasterizerStates[State];
		}
		SamplerState* GraphicsDevice::GetSamplerState(uint64_t State)
		{
			if (State < 0 || State >= SamplerStates.size())
				return nullptr;

			return SamplerStates[State];
		}
		uint64_t GraphicsDevice::GetDepthStencilStateCount()
		{
			return DepthStencilStates.size();
		}
		uint64_t GraphicsDevice::GetBlendStateCount()
		{
			return BlendStates.size();
		}
		uint64_t GraphicsDevice::GetRasterizerStateCount()
		{
			return RasterizerStates.size();
		}
		uint64_t GraphicsDevice::GetSamplerStateCount()
		{
			return SamplerStates.size();
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
		unsigned int GraphicsDevice::GetPresentationFlags()
		{
			return PresentationFlag;
		}
		unsigned int GraphicsDevice::GetCompilationFlags()
		{
			return CompilationFlag;
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
		Compute::Vector2 GraphicsDevice::GetScreenDimensions()
		{
			return ScreenDimensions;
		}
		Compute::Vector2 GraphicsDevice::ScreenDimensions = Compute::Vector2::Zero();

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
		void Activity::SetCursorPosition(const Compute::Vector2& Position)
		{
#ifdef THAWK_HAS_SDL2
#if SDL_VERSION_ATLEAST(2, 0, 4)
			SDL_WarpMouseGlobal((int)Position.X, (int)Position.Y);
#endif
#endif
		}
		void Activity::SetCursorPosition(float X, float Y)
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

							if (Mapping.Enabled && Mapping.Mapped && Mapping.Key.Key == KeyCode_CURSORLEFT)
								Mapping.Captured = true;

							return true;
						case SDL_BUTTON_RIGHT:
							if (Callbacks.KeyState && Id == Event.window.windowID)
								Callbacks.KeyState(KeyCode_CURSORRIGHT, (KeyMod)SDL_GetModState(), (int)KeyCode_CURSORRIGHT, (int)Event.button.clicks, false);

							if (Mapping.Enabled && Mapping.Mapped && Mapping.Key.Key == KeyCode_CURSORLEFT)
								Mapping.Captured = true;

							return true;
						case SDL_BUTTON_X1:
							if (Callbacks.KeyState && Id == Event.window.windowID)
								Callbacks.KeyState(KeyCode_CURSORX1, (KeyMod)SDL_GetModState(), (int)KeyCode_CURSORX1, (int)Event.button.clicks, false);

							if (Mapping.Enabled && Mapping.Mapped && Mapping.Key.Key == KeyCode_CURSORLEFT)
								Mapping.Captured = true;

							return true;
						case SDL_BUTTON_X2:
							if (Callbacks.KeyState && Id == Event.window.windowID)
								Callbacks.KeyState(KeyCode_CURSORX2, (KeyMod)SDL_GetModState(), (int)KeyCode_CURSORX2, (int)Event.button.clicks, false);

							if (Mapping.Enabled && Mapping.Mapped && Mapping.Key.Key == KeyCode_CURSORLEFT)
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
				return false;

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
		Compute::Vector2 Activity::GetClientCursorPosition()
		{
#ifdef THAWK_HAS_SDL2
			return Compute::Vector2((float)CX, (float)CY);
#else
			return Compute::Vector2();
#endif
		}
		Compute::Vector2 Activity::GetCursorPosition()
		{
#if defined(THAWK_HAS_SDL2) && SDL_VERSION_ATLEAST(2, 0, 4)
			int X, Y;
			SDL_GetGlobalMouseState(&X, &Y);

			return Compute::Vector2((float)X, (float)Y);
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
	}
}