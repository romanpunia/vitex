#include "graphics.h"
#include "../graphics/d3d11.h"
#include "../graphics/ogl.h"
#include "../graphics/shaders/bundle.hpp"
#ifdef VI_SPIRV
#include <spirv_cross/spirv_glsl.hpp>
#ifdef VI_MICROSOFT
#include <spirv_cross/spirv_hlsl.hpp>
#endif
#ifdef VI_APPLE
#include <spirv_cross/spirv_msl.hpp>
#endif
#include <SPIRV/GlslangToSpv.h>
#endif
#ifdef VI_SDL2
#include "../internal/sdl2-cross.hpp"
#endif

namespace
{
#ifdef VI_SPIRV
	static TBuiltInResource DriverLimits = { };

	static void PrepareDriverLimits()
	{
		static bool IsReady = false;
		if (IsReady)
			return;

		DriverLimits.maxLights = 32;
		DriverLimits.maxClipPlanes = 6;
		DriverLimits.maxTextureUnits = 32;
		DriverLimits.maxTextureCoords = 32;
		DriverLimits.maxVertexAttribs = 64;
		DriverLimits.maxVertexUniformComponents = 4096;
		DriverLimits.maxVaryingFloats = 64;
		DriverLimits.maxVertexTextureImageUnits = 32;
		DriverLimits.maxCombinedTextureImageUnits = 80;
		DriverLimits.maxTextureImageUnits = 32;
		DriverLimits.maxFragmentUniformComponents = 4096;
		DriverLimits.maxDrawBuffers = 32;
		DriverLimits.maxVertexUniformVectors = 128;
		DriverLimits.maxVaryingVectors = 8;
		DriverLimits.maxFragmentUniformVectors = 16;
		DriverLimits.maxVertexOutputVectors = 16;
		DriverLimits.maxFragmentInputVectors = 15;
		DriverLimits.minProgramTexelOffset = -8;
		DriverLimits.maxProgramTexelOffset = 7;
		DriverLimits.maxClipDistances = 8;
		DriverLimits.maxComputeWorkGroupCountX = 65535;
		DriverLimits.maxComputeWorkGroupCountY = 65535;
		DriverLimits.maxComputeWorkGroupCountZ = 65535;
		DriverLimits.maxComputeWorkGroupSizeX = 1024;
		DriverLimits.maxComputeWorkGroupSizeY = 1024;
		DriverLimits.maxComputeWorkGroupSizeZ = 64;
		DriverLimits.maxComputeUniformComponents = 1024;
		DriverLimits.maxComputeTextureImageUnits = 16;
		DriverLimits.maxComputeImageUniforms = 8;
		DriverLimits.maxComputeAtomicCounters = 8;
		DriverLimits.maxComputeAtomicCounterBuffers = 1;
		DriverLimits.maxVaryingComponents = 60;
		DriverLimits.maxVertexOutputComponents = 64;
		DriverLimits.maxGeometryInputComponents = 64;
		DriverLimits.maxGeometryOutputComponents = 128;
		DriverLimits.maxFragmentInputComponents = 128;
		DriverLimits.maxImageUnits = 8;
		DriverLimits.maxCombinedImageUnitsAndFragmentOutputs = 8;
		DriverLimits.maxCombinedShaderOutputResources = 8;
		DriverLimits.maxImageSamples = 0;
		DriverLimits.maxVertexImageUniforms = 0;
		DriverLimits.maxTessControlImageUniforms = 0;
		DriverLimits.maxTessEvaluationImageUniforms = 0;
		DriverLimits.maxGeometryImageUniforms = 0;
		DriverLimits.maxFragmentImageUniforms = 8;
		DriverLimits.maxCombinedImageUniforms = 8;
		DriverLimits.maxGeometryTextureImageUnits = 16;
		DriverLimits.maxGeometryOutputVertices = 256;
		DriverLimits.maxGeometryTotalOutputComponents = 1024;
		DriverLimits.maxGeometryUniformComponents = 1024;
		DriverLimits.maxGeometryVaryingComponents = 64;
		DriverLimits.maxTessControlInputComponents = 128;
		DriverLimits.maxTessControlOutputComponents = 128;
		DriverLimits.maxTessControlTextureImageUnits = 16;
		DriverLimits.maxTessControlUniformComponents = 1024;
		DriverLimits.maxTessControlTotalOutputComponents = 4096;
		DriverLimits.maxTessEvaluationInputComponents = 128;
		DriverLimits.maxTessEvaluationOutputComponents = 128;
		DriverLimits.maxTessEvaluationTextureImageUnits = 16;
		DriverLimits.maxTessEvaluationUniformComponents = 1024;
		DriverLimits.maxTessPatchComponents = 120;
		DriverLimits.maxPatchVertices = 32;
		DriverLimits.maxTessGenLevel = 64;
		DriverLimits.maxViewports = 16;
		DriverLimits.maxVertexAtomicCounters = 0;
		DriverLimits.maxTessControlAtomicCounters = 0;
		DriverLimits.maxTessEvaluationAtomicCounters = 0;
		DriverLimits.maxGeometryAtomicCounters = 0;
		DriverLimits.maxFragmentAtomicCounters = 8;
		DriverLimits.maxCombinedAtomicCounters = 8;
		DriverLimits.maxAtomicCounterBindings = 1;
		DriverLimits.maxVertexAtomicCounterBuffers = 0;
		DriverLimits.maxTessControlAtomicCounterBuffers = 0;
		DriverLimits.maxTessEvaluationAtomicCounterBuffers = 0;
		DriverLimits.maxGeometryAtomicCounterBuffers = 0;
		DriverLimits.maxFragmentAtomicCounterBuffers = 1;
		DriverLimits.maxCombinedAtomicCounterBuffers = 1;
		DriverLimits.maxAtomicCounterBufferSize = 16384;
		DriverLimits.maxTransformFeedbackBuffers = 4;
		DriverLimits.maxTransformFeedbackInterleavedComponents = 64;
		DriverLimits.maxCullDistances = 8;
		DriverLimits.maxCombinedClipAndCullDistances = 8;
		DriverLimits.maxSamples = 4;
		DriverLimits.maxMeshOutputVerticesNV = 256;
		DriverLimits.maxMeshOutputPrimitivesNV = 512;
		DriverLimits.maxMeshWorkGroupSizeX_NV = 32;
		DriverLimits.maxMeshWorkGroupSizeY_NV = 1;
		DriverLimits.maxMeshWorkGroupSizeZ_NV = 1;
		DriverLimits.maxTaskWorkGroupSizeX_NV = 32;
		DriverLimits.maxTaskWorkGroupSizeY_NV = 1;
		DriverLimits.maxTaskWorkGroupSizeZ_NV = 1;
		DriverLimits.maxMeshViewCountNV = 4;
		DriverLimits.limits.nonInductiveForLoops = 1;
		DriverLimits.limits.whileLoops = 1;
		DriverLimits.limits.doWhileLoops = 1;
		DriverLimits.limits.generalUniformIndexing = 1;
		DriverLimits.limits.generalAttributeMatrixVectorIndexing = 1;
		DriverLimits.limits.generalVaryingIndexing = 1;
		DriverLimits.limits.generalSamplerIndexing = 1;
		DriverLimits.limits.generalVariableIndexing = 1;
		DriverLimits.limits.generalConstantMatrixVectorIndexing = 1;
		IsReady = true;
	}
	static void PrepareSamplers(spirv_cross::Compiler* Compiler)
	{
		for (auto& SamplerId : Compiler->get_combined_image_samplers())
		{
			uint32_t BindingId = Compiler->get_decoration(SamplerId.image_id, spv::DecorationBinding);
			Compiler->set_decoration(SamplerId.combined_id, spv::DecorationBinding, BindingId);
		}
	}
#endif
	static Mavi::Graphics::RenderBackend GetSupportedBackend(Mavi::Graphics::RenderBackend Type)
	{
		if (Type != Mavi::Graphics::RenderBackend::Automatic)
			return Type;
#ifdef VI_MICROSOFT
		return Mavi::Graphics::RenderBackend::D3D11;
#endif
#ifdef VI_GL
		return Mavi::Graphics::RenderBackend::OGL;
#endif
		return Mavi::Graphics::RenderBackend::None;
	}
}

namespace Mavi
{
	namespace Graphics
	{
		Alert::Alert(Activity* From) noexcept : View(AlertType::None), Base(From), Waiting(false)
		{
		}
		void Alert::Setup(AlertType Type, const Core::String& Title, const Core::String& Text)
		{
			VI_ASSERT(Type != AlertType::None, "alert type should not be none");
			View = Type;
			Name = Title;
			Data = Text;
			Buttons.clear();
		}
		void Alert::Button(AlertConfirm Confirm, const Core::String& Text, int Id)
		{
			VI_ASSERT(View != AlertType::None, "alert type should not be none");
			VI_ASSERT(Buttons.size() < 16, "there must be less than 16 buttons in alert");

			for (auto& Item : Buttons)
			{
				if (Item.Id == Id)
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
			VI_ASSERT(View != AlertType::None, "alert type should not be none");
			Done = Callback;
			Waiting = true;
		}
		void Alert::Dispatch()
		{
#ifdef VI_SDL2
			if (View == AlertType::None || !Waiting)
				return;

			SDL_MessageBoxButtonData Views[16];
			for (size_t i = 0; i < Buttons.size(); i++)
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
			View = AlertType::None;
			Waiting = false;
			int Rd = SDL_ShowMessageBox(&AlertData, &Id);

			if (Done)
				Done(Rd >= 0 ? Id : -1);
#endif
		}

		KeyMap::KeyMap() noexcept : Key(KeyCode::None), Mod(KeyMod::None), Normal(false)
		{
		}
		KeyMap::KeyMap(const KeyCode& Value) noexcept : Key(Value), Mod(KeyMod::None), Normal(false)
		{
		}
		KeyMap::KeyMap(const KeyMod& Value) noexcept : Key(KeyCode::None), Mod(Value), Normal(false)
		{
		}
		KeyMap::KeyMap(const KeyCode& Value, const KeyMod& Control) noexcept : Key(Value), Mod(Control), Normal(false)
		{
		}

		Surface::Surface() noexcept : Handle(nullptr)
		{
		}
		Surface::Surface(SDL_Surface* From) noexcept : Handle(From)
		{
		}
		Surface::~Surface() noexcept
		{
#ifdef VI_SDL2
			if (Handle != nullptr)
			{
				SDL_FreeSurface(Handle);
				Handle = nullptr;
			}
#endif
		}
		void Surface::SetHandle(SDL_Surface* From)
		{
#ifdef VI_SDL2
			if (Handle != nullptr)
				SDL_FreeSurface(Handle);
#endif
			Handle = From;
		}
		void Surface::Lock()
		{
#ifdef VI_SDL2
			SDL_LockSurface(Handle);
#endif
		}
		void Surface::Unlock()
		{
#ifdef VI_SDL2
			SDL_UnlockSurface(Handle);
#endif
		}
		int Surface::GetWidth() const
		{
#ifdef VI_SDL2
			VI_ASSERT(Handle != nullptr, "handle should be set");
			return Handle->w;
#else
			return -1;
#endif
		}
		int Surface::GetHeight() const
		{
#ifdef VI_SDL2
			VI_ASSERT(Handle != nullptr, "handle should be set");
			return Handle->h;
#else
			return -1;
#endif
		}
		int Surface::GetPitch() const
		{
#ifdef VI_SDL2
			VI_ASSERT(Handle != nullptr, "handle should be set");
			return Handle->pitch;
#else
			return -1;
#endif
		}
		void* Surface::GetPixels() const
		{
#ifdef VI_SDL2
			VI_ASSERT(Handle != nullptr, "handle should be set");
			return Handle->pixels;
#else
			return nullptr;
#endif
		}
		void* Surface::GetResource() const
		{
			return (void*)Handle;
		}

		DepthStencilState::DepthStencilState(const Desc& I) noexcept : State(I)
		{
		}
		DepthStencilState::~DepthStencilState() noexcept
		{
		}
		DepthStencilState::Desc DepthStencilState::GetState() const
		{
			return State;
		}

		RasterizerState::RasterizerState(const Desc& I) noexcept : State(I)
		{
		}
		RasterizerState::~RasterizerState() noexcept
		{
		}
		RasterizerState::Desc RasterizerState::GetState() const
		{
			return State;
		}

		BlendState::BlendState(const Desc& I) noexcept : State(I)
		{
		}
		BlendState::~BlendState() noexcept
		{
		}
		BlendState::Desc BlendState::GetState() const
		{
			return State;
		}

		SamplerState::SamplerState(const Desc& I) noexcept : State(I)
		{
		}
		SamplerState::~SamplerState() noexcept
		{
		}
		SamplerState::Desc SamplerState::GetState() const
		{
			return State;
		}

		InputLayout::InputLayout(const Desc& I) noexcept : Layout(I.Attributes)
		{
		}
		InputLayout::~InputLayout() noexcept
		{
		}
		const Core::Vector<InputLayout::Attribute>& InputLayout::GetAttributes() const
		{
			return Layout;
		}

		Shader::Shader(const Desc& I) noexcept
		{
		}

		ElementBuffer::ElementBuffer(const Desc& I) noexcept
		{
			Elements = I.ElementCount;
			Stride = I.ElementWidth;
		}
		size_t ElementBuffer::GetElements() const
		{
			return Elements;
		}
		size_t ElementBuffer::GetStride() const
		{
			return Stride;
		}

		MeshBuffer::MeshBuffer(const Desc& I) noexcept : VertexBuffer(nullptr), IndexBuffer(nullptr)
		{
		}
		MeshBuffer::~MeshBuffer() noexcept
		{
			VI_CLEAR(VertexBuffer);
			VI_CLEAR(IndexBuffer);
		}
		ElementBuffer* MeshBuffer::GetVertexBuffer() const
		{
			return VertexBuffer;
		}
		ElementBuffer* MeshBuffer::GetIndexBuffer() const
		{
			return IndexBuffer;
		}

		SkinMeshBuffer::SkinMeshBuffer(const Desc& I) noexcept : VertexBuffer(nullptr), IndexBuffer(nullptr)
		{
		}
		SkinMeshBuffer::~SkinMeshBuffer() noexcept
		{
			VI_CLEAR(VertexBuffer);
			VI_CLEAR(IndexBuffer);
		}
		ElementBuffer* SkinMeshBuffer::GetVertexBuffer() const
		{
			return VertexBuffer;
		}
		ElementBuffer* SkinMeshBuffer::GetIndexBuffer() const
		{
			return IndexBuffer;
		}

		InstanceBuffer::InstanceBuffer(const Desc& I) noexcept : Elements(nullptr), Device(I.Device), Sync(false)
		{
			ElementLimit = I.ElementLimit;
			ElementWidth = I.ElementWidth;

			if (ElementLimit < 1)
				ElementLimit = 1;

			Array.reserve(ElementLimit);
		}
		InstanceBuffer::~InstanceBuffer() noexcept
		{
			VI_CLEAR(Elements);
		}
		Core::Vector<Compute::ElementVertex>& InstanceBuffer::GetArray()
		{
			return Array;
		}
		ElementBuffer* InstanceBuffer::GetElements() const
		{
			return Elements;
		}
		GraphicsDevice* InstanceBuffer::GetDevice() const
		{
			return Device;
		}
		size_t InstanceBuffer::GetElementLimit() const
		{
			return ElementLimit;
		}

		Texture2D::Texture2D() noexcept
		{
			Width = WINDOW_SIZE;
			Height = WINDOW_SIZE;
			MipLevels = 1;
			FormatMode = Format::Unknown;
			Usage = ResourceUsage::Default;
			AccessFlags = CPUAccess::None;
			Binding = ResourceBind::Shader_Input;
		}
		Texture2D::Texture2D(const Desc& I) noexcept
		{
			Width = I.Width;
			Height = I.Height;
			MipLevels = I.MipLevels;
			FormatMode = I.FormatMode;
			Usage = I.Usage;
			AccessFlags = I.AccessFlags;
			Binding = I.BindFlags;
		}
		CPUAccess Texture2D::GetAccessFlags() const
		{
			return AccessFlags;
		}
		Format Texture2D::GetFormatMode() const
		{
			return FormatMode;
		}
		ResourceUsage Texture2D::GetUsage() const
		{
			return Usage;
		}
		ResourceBind Texture2D::GetBinding() const
		{
			return Binding;
		}
		unsigned int Texture2D::GetWidth() const
		{
			return Width;
		}
		unsigned int Texture2D::GetHeight() const
		{
			return Height;
		}
		unsigned int Texture2D::GetMipLevels() const
		{
			return MipLevels;
		}

		Texture3D::Texture3D()
		{
			Width = WINDOW_SIZE;
			Height = WINDOW_SIZE;
			Depth = 1;
			MipLevels = 1;
			FormatMode = Format::Unknown;
			Usage = ResourceUsage::Default;
			AccessFlags = CPUAccess::None;
			Binding = ResourceBind::Shader_Input;
		}
		CPUAccess Texture3D::GetAccessFlags() const
		{
			return AccessFlags;
		}
		Format Texture3D::GetFormatMode() const
		{
			return FormatMode;
		}
		ResourceUsage Texture3D::GetUsage() const
		{
			return Usage;
		}
		ResourceBind Texture3D::GetBinding() const
		{
			return Binding;
		}
		unsigned int Texture3D::GetWidth() const
		{
			return Width;
		}
		unsigned int Texture3D::GetHeight() const
		{
			return Height;
		}
		unsigned int Texture3D::GetDepth() const
		{
			return Depth;
		}
		unsigned int Texture3D::GetMipLevels() const
		{
			return MipLevels;
		}

		TextureCube::TextureCube() noexcept
		{
			Width = WINDOW_SIZE;
			Height = WINDOW_SIZE;
			MipLevels = 1;
			FormatMode = Format::Unknown;
			Usage = ResourceUsage::Default;
			AccessFlags = CPUAccess::None;
			Binding = ResourceBind::Shader_Input;
		}
		TextureCube::TextureCube(const Desc& I) noexcept
		{
			Width = I.Width;
			Height = I.Height;
			MipLevels = I.MipLevels;
			FormatMode = I.FormatMode;
			Usage = I.Usage;
			AccessFlags = I.AccessFlags;
			Binding = I.BindFlags;
		}
		CPUAccess TextureCube::GetAccessFlags() const
		{
			return AccessFlags;
		}
		Format TextureCube::GetFormatMode() const
		{
			return FormatMode;
		}
		ResourceUsage TextureCube::GetUsage() const
		{
			return Usage;
		}
		ResourceBind TextureCube::GetBinding() const
		{
			return Binding;
		}
		unsigned int TextureCube::GetWidth() const
		{
			return Width;
		}
		unsigned int TextureCube::GetHeight() const
		{
			return Height;
		}
		unsigned int TextureCube::GetMipLevels() const
		{
			return MipLevels;
		}

		DepthTarget2D::DepthTarget2D(const Desc& I) noexcept : Resource(nullptr), Viewarea({ 0, 0, WINDOW_SIZE, WINDOW_SIZE, 0, 1 })
		{
		}
		DepthTarget2D::~DepthTarget2D() noexcept
		{
			VI_RELEASE(Resource);
		}
		Texture2D* DepthTarget2D::GetTarget()
		{
			return Resource;
		}
		const Viewport& DepthTarget2D::GetViewport() const
		{
			return Viewarea;
		}

		DepthTargetCube::DepthTargetCube(const Desc& I) noexcept : Resource(nullptr), Viewarea({ 0, 0, WINDOW_SIZE, WINDOW_SIZE, 0, 1 })
		{
		}
		DepthTargetCube::~DepthTargetCube() noexcept
		{
			VI_RELEASE(Resource);
		}
		TextureCube* DepthTargetCube::GetTarget()
		{
			return Resource;
		}
		const Viewport& DepthTargetCube::GetViewport() const
		{
			return Viewarea;
		}

		RenderTarget::RenderTarget() noexcept : DepthStencil(nullptr), Viewarea({ 0, 0, WINDOW_SIZE, WINDOW_SIZE, 0, 1 })
		{
		}
		RenderTarget::~RenderTarget() noexcept
		{
			VI_RELEASE(DepthStencil);
		}
		Texture2D* RenderTarget::GetDepthStencil()
		{
			return DepthStencil;
		}
		const Viewport& RenderTarget::GetViewport() const
		{
			return Viewarea;
		}

		RenderTarget2D::RenderTarget2D(const Desc& I) noexcept : RenderTarget(), Resource(nullptr)
		{
		}
		RenderTarget2D::~RenderTarget2D() noexcept
		{
			VI_RELEASE(Resource);
		}
		uint32_t RenderTarget2D::GetTargetCount() const
		{
			return 1;
		}
		Texture2D* RenderTarget2D::GetTarget2D(unsigned int Index)
		{
			return GetTarget();
		}
		TextureCube* RenderTarget2D::GetTargetCube(unsigned int Index)
		{
			return nullptr;
		}
		Texture2D* RenderTarget2D::GetTarget()
		{
			return Resource;
		}

		MultiRenderTarget2D::MultiRenderTarget2D(const Desc& I) noexcept : RenderTarget()
		{
			VI_ASSERT((uint32_t)I.Target <= 8, "target should be less than 9");
			Target = I.Target;

			for (uint32_t i = 0; i < 8; i++)
				Resource[i] = nullptr;
		}
		MultiRenderTarget2D::~MultiRenderTarget2D() noexcept
		{
			VI_ASSERT((uint32_t)Target <= 8, "target should be less than 9");
			for (uint32_t i = 0; i < (uint32_t)Target; i++)
				VI_RELEASE(Resource[i]);
		}
		uint32_t MultiRenderTarget2D::GetTargetCount() const
		{
			return (uint32_t)Target;
		}
		Texture2D* MultiRenderTarget2D::GetTarget2D(unsigned int Index)
		{
			return GetTarget(Index);
		}
		TextureCube* MultiRenderTarget2D::GetTargetCube(unsigned int Index)
		{
			return nullptr;
		}
		Texture2D* MultiRenderTarget2D::GetTarget(unsigned int Slot)
		{
			VI_ASSERT(Slot < (uint32_t)Target, "slot should be less than targets count");
			return Resource[Slot];
		}

		RenderTargetCube::RenderTargetCube(const Desc& I) noexcept : RenderTarget(), Resource(nullptr)
		{
		}
		RenderTargetCube::~RenderTargetCube() noexcept
		{
			VI_RELEASE(Resource);
		}
		uint32_t RenderTargetCube::GetTargetCount() const
		{
			return 1;
		}
		Texture2D* RenderTargetCube::GetTarget2D(unsigned int Index)
		{
			return nullptr;
		}
		TextureCube* RenderTargetCube::GetTargetCube(unsigned int Index)
		{
			return GetTarget();
		}
		TextureCube* RenderTargetCube::GetTarget()
		{
			return Resource;
		}

		MultiRenderTargetCube::MultiRenderTargetCube(const Desc& I) noexcept : RenderTarget()
		{
			VI_ASSERT((uint32_t)I.Target <= 8, "target should be less than 9");
			Target = I.Target;

			for (uint32_t i = 0; i < 8; i++)
				Resource[i] = nullptr;
		}
		MultiRenderTargetCube::~MultiRenderTargetCube() noexcept
		{
			VI_ASSERT((uint32_t)Target <= 8, "target should be less than 9");
			for (uint32_t i = 0; i < (uint32_t)Target; i++)
				VI_RELEASE(Resource[i]);
		}
		uint32_t MultiRenderTargetCube::GetTargetCount() const
		{
			return (uint32_t)Target;
		}
		Texture2D* MultiRenderTargetCube::GetTarget2D(unsigned int Index)
		{
			return nullptr;
		}
		TextureCube* MultiRenderTargetCube::GetTargetCube(unsigned int Index)
		{
			return GetTarget(Index);
		}
		TextureCube* MultiRenderTargetCube::GetTarget(unsigned int Slot)
		{
			VI_ASSERT(Slot < (uint32_t)Target, "slot should be less than targets count");
			return Resource[Slot];
		}

		Cubemap::Cubemap(const Desc& I) noexcept : Dest(nullptr), Meta(I)
		{
		}
		bool Cubemap::IsValid() const
		{
			return Meta.Source != nullptr;
		}

		Query::Query() noexcept
		{
		}

		GraphicsDevice::GraphicsDevice(const Desc& I) noexcept : Primitives(PrimitiveTopology::Triangle_List), ShaderGen(ShaderModel::Invalid), ViewResource(nullptr), PresentFlags(I.PresentationFlags), CompileFlags(I.CompilationFlags), VSyncMode(I.VSyncMode), MaxElements(1), Backend(I.Backend), ShaderCache(I.ShaderCache), Debug(I.Debug)
		{
			RenderThread = std::this_thread::get_id();
			if (!I.CacheDirectory.empty())
			{
				auto Directory = Core::OS::Path::ResolveDirectory(I.CacheDirectory.c_str());
				if (Directory && Core::OS::Directory::IsExists(Directory->c_str()))
					Caches = *Directory;
			}

			if (!I.Window)
			{
				Activity::Desc Init;
				Init.Title = "activity.virtual.hidden";
				Init.Hidden = true;
				Init.Borderless = true;
				Init.Width = 128;
				Init.Height = 128;

				VirtualWindow = new Activity(Init);
			}

			CreateSections();
		}
		GraphicsDevice::~GraphicsDevice() noexcept
		{
			ReleaseProxy();
			for (auto It = Sections.begin(); It != Sections.end(); It++)
				VI_DELETE(Section, It->second);

			VI_CLEAR(VirtualWindow);
			Sections.clear();
		}
		void GraphicsDevice::SetVertexBuffer(ElementBuffer* Resource)
		{
			if (Resource != nullptr)
				SetVertexBuffers(&Resource, 1);
			else
				SetVertexBuffers(nullptr, 0);
		}
		void GraphicsDevice::SetShaderCache(bool Enabled)
		{
			ShaderCache = Enabled;
		}
		void GraphicsDevice::SetVSyncMode(VSync Mode)
		{
			VSyncMode = Mode;
		}
		void GraphicsDevice::Lockup(RenderThreadCallback&& Callback)
		{
			VI_ASSERT(Callback != nullptr, "callback should be set");
			Core::UMutex<std::recursive_mutex> Unique(Exclusive);
			Callback(this);
		}
		void GraphicsDevice::Enqueue(RenderThreadCallback&& Callback)
		{
			VI_ASSERT(Callback != nullptr, "callback should be set");
			if (RenderThread != std::this_thread::get_id())
			{
				Core::UMutex<std::recursive_mutex> Unique(Exclusive);
				Queue.emplace(std::move(Callback));
			}
			else
				Callback(this);
		}
		void GraphicsDevice::DispatchQueue()
		{
			RenderThread = std::this_thread::get_id();
			if (Queue.empty())
				return;

			Core::UMutex<std::recursive_mutex> Unique(Exclusive);
			while (!Queue.empty())
			{
				Queue.front()(this);
				Queue.pop();
			}
		}
		void GraphicsDevice::CreateStates()
		{
			DepthStencilState::Desc DepthStencil;
			DepthStencil.DepthEnable = true;
			DepthStencil.DepthWriteMask = DepthWrite::All;
			DepthStencil.DepthFunction = Comparison::Less;
			DepthStencil.StencilEnable = true;
			DepthStencil.StencilReadMask = 0xFF;
			DepthStencil.StencilWriteMask = 0xFF;
			DepthStencil.FrontFaceStencilFailOperation = StencilOperation::Keep;
			DepthStencil.FrontFaceStencilDepthFailOperation = StencilOperation::Add;
			DepthStencil.FrontFaceStencilPassOperation = StencilOperation::Keep;
			DepthStencil.FrontFaceStencilFunction = Comparison::Always;
			DepthStencil.BackFaceStencilFailOperation = StencilOperation::Keep;
			DepthStencil.BackFaceStencilDepthFailOperation = StencilOperation::Subtract;
			DepthStencil.BackFaceStencilPassOperation = StencilOperation::Keep;
			DepthStencil.BackFaceStencilFunction = Comparison::Always;
			DepthStencilStates["drw_srw_lt"] = CreateDepthStencilState(DepthStencil);

			DepthStencil.DepthWriteMask = DepthWrite::Zero;
			DepthStencil.StencilWriteMask = 0x0;
			DepthStencilStates["dro_sro_lt"] = CreateDepthStencilState(DepthStencil);

			DepthStencil.DepthFunction = Comparison::Greater_Equal;
			DepthStencilStates["dro_sro_gte"] = CreateDepthStencilState(DepthStencil);

			DepthStencil.DepthWriteMask = DepthWrite::All;
			DepthStencil.StencilWriteMask = 0xFF;
			DepthStencilStates["dro_srw_gte"] = CreateDepthStencilState(DepthStencil);

			DepthStencil.DepthEnable = false;
			DepthStencil.DepthFunction = Comparison::Less;
			DepthStencil.StencilEnable = false;
			DepthStencilStates["doo_soo_lt"] = CreateDepthStencilState(DepthStencil);

			DepthStencil.DepthEnable = true;
			DepthStencil.DepthWriteMask = DepthWrite::Zero;
			DepthStencil.StencilEnable = true;
			DepthStencilStates["dro_srw_lt"] = CreateDepthStencilState(DepthStencil);

			DepthStencil.DepthWriteMask = DepthWrite::All;
			DepthStencil.StencilEnable = false;
			DepthStencilStates["drw_soo_lt"] = CreateDepthStencilState(DepthStencil);

			DepthStencil.DepthFunction = Comparison::Less_Equal;
			DepthStencil.DepthWriteMask = DepthWrite::Zero;
			DepthStencil.StencilEnable = false;
			DepthStencilStates["dro_soo_lte"] = CreateDepthStencilState(DepthStencil);

			RasterizerState::Desc Rasterizer;
			Rasterizer.AntialiasedLineEnable = false;
			Rasterizer.CullMode = VertexCull::Back;
			Rasterizer.DepthBias = 0;
			Rasterizer.DepthBiasClamp = 0;
			Rasterizer.DepthClipEnable = true;
			Rasterizer.FillMode = SurfaceFill::Solid;
			Rasterizer.FrontCounterClockwise = false;
			Rasterizer.MultisampleEnable = false;
			Rasterizer.ScissorEnable = false;
			Rasterizer.SlopeScaledDepthBias = 0.0f;
			RasterizerStates["so_cback"] = CreateRasterizerState(Rasterizer);

			Rasterizer.CullMode = VertexCull::Front;
			RasterizerStates["so_cfront"] = CreateRasterizerState(Rasterizer);

			Rasterizer.CullMode = VertexCull::None;
			RasterizerStates["so_co"] = CreateRasterizerState(Rasterizer);

			Rasterizer.ScissorEnable = true;
			RasterizerStates["sw_co"] = CreateRasterizerState(Rasterizer);

			Rasterizer.CullMode = VertexCull::Back;
			RasterizerStates["sw_cback"] = CreateRasterizerState(Rasterizer);

			BlendState::Desc Blend;
			Blend.AlphaToCoverageEnable = false;
			Blend.IndependentBlendEnable = false;
			Blend.RenderTarget[0].BlendEnable = false;
			Blend.RenderTarget[0].RenderTargetWriteMask = (unsigned char)ColorWriteEnable::All;
			BlendStates["bo_wrgba"] = CreateBlendState(Blend);

			Blend.RenderTarget[0].RenderTargetWriteMask = (unsigned char)(ColorWriteEnable::Red | ColorWriteEnable::Green | ColorWriteEnable::Blue);
			BlendStates["bo_wrgbo"] = CreateBlendState(Blend);

			Blend.RenderTarget[0].RenderTargetWriteMask = 0;
			BlendStates["bo_woooo"] = CreateBlendState(Blend);

			Blend.RenderTarget[0].BlendEnable = true;
			Blend.RenderTarget[0].SrcBlend = Blend::One;
			Blend.RenderTarget[0].DestBlend = Blend::One;
			Blend.RenderTarget[0].BlendOperationMode = BlendOperation::Add;
			Blend.RenderTarget[0].SrcBlendAlpha = Blend::One;
			Blend.RenderTarget[0].DestBlendAlpha = Blend::One;
			Blend.RenderTarget[0].BlendOperationAlpha = BlendOperation::Add;
			Blend.RenderTarget[0].RenderTargetWriteMask = (unsigned char)ColorWriteEnable::All;
			BlendStates["bw_wrgba"] = CreateBlendState(Blend);

			Blend.RenderTarget[0].RenderTargetWriteMask = (unsigned char)(ColorWriteEnable::Red | ColorWriteEnable::Green | ColorWriteEnable::Blue);
			BlendStates["bw_wrgbo"] = CreateBlendState(Blend);

			Blend.IndependentBlendEnable = true;
			for (unsigned int i = 0; i < 8; i++)
			{
				Blend.RenderTarget[i].BlendEnable = (i != 1 && i != 2);
				Blend.RenderTarget[i].SrcBlend = Blend::One;
				Blend.RenderTarget[i].DestBlend = Blend::One;
				Blend.RenderTarget[i].BlendOperationMode = BlendOperation::Add;
				Blend.RenderTarget[i].SrcBlendAlpha = Blend::One;
				Blend.RenderTarget[i].DestBlendAlpha = Blend::One;
				Blend.RenderTarget[i].BlendOperationAlpha = BlendOperation::Add;
				Blend.RenderTarget[i].RenderTargetWriteMask = (unsigned char)ColorWriteEnable::All;
			}
			BlendStates["bw_wrgba_gbuffer"] = CreateBlendState(Blend);

			Blend.IndependentBlendEnable = false;
			Blend.RenderTarget[0].BlendEnable = true;
			Blend.RenderTarget[0].SrcBlend = Blend::Source_Alpha;
			BlendStates["bw_wrgba_alpha"] = CreateBlendState(Blend);

			Blend.RenderTarget[0].DestBlend = Blend::Source_Alpha_Invert;
			Blend.RenderTarget[0].SrcBlendAlpha = Blend::Source_Alpha_Invert;
			Blend.RenderTarget[0].DestBlendAlpha = Blend::Zero;
			BlendStates["bw_wrgba_source"] = CreateBlendState(Blend);

			SamplerState::Desc Sampler;
			Sampler.Filter = PixelFilter::Anistropic;
			Sampler.AddressU = TextureAddress::Wrap;
			Sampler.AddressV = TextureAddress::Wrap;
			Sampler.AddressW = TextureAddress::Wrap;
			Sampler.MipLODBias = 0.0f;
			Sampler.MaxAnisotropy = 16;
			Sampler.ComparisonFunction = Comparison::Never;
			Sampler.BorderColor[0] = 0.0f;
			Sampler.BorderColor[1] = 0.0f;
			Sampler.BorderColor[2] = 0.0f;
			Sampler.BorderColor[3] = 0.0f;
			Sampler.MinLOD = 0.0f;
			Sampler.MaxLOD = std::numeric_limits<float>::max();
			SamplerStates["a16_fa_wrap"] = CreateSamplerState(Sampler);

			Sampler.AddressU = TextureAddress::Mirror;
			Sampler.AddressV = TextureAddress::Mirror;
			Sampler.AddressW = TextureAddress::Mirror;
			SamplerStates["a16_fa_mirror"] = CreateSamplerState(Sampler);

			Sampler.AddressU = TextureAddress::Clamp;
			Sampler.AddressV = TextureAddress::Clamp;
			Sampler.AddressW = TextureAddress::Clamp;
			SamplerStates["a16_fa_clamp"] = CreateSamplerState(Sampler);

			Sampler.Filter = PixelFilter::Min_Mag_Mip_Linear;
			SamplerStates["a16_fl_clamp"] = CreateSamplerState(Sampler);

			Sampler.Filter = PixelFilter::Min_Mag_Mip_Point;
			Sampler.ComparisonFunction = Comparison::Never;
			SamplerStates["a16_fp_clamp"] = CreateSamplerState(Sampler);

			Sampler.Filter = PixelFilter::Min_Mag_Mip_Linear;
			Sampler.MaxAnisotropy = 1;
			SamplerStates["a1_fl_clamp"] = CreateSamplerState(Sampler);

			Sampler.Filter = PixelFilter::Compare_Min_Mag_Mip_Linear;
			Sampler.ComparisonFunction = Comparison::Less;
			SamplerStates["a1_fl_clamp_cmp_lt"] = CreateSamplerState(Sampler);

			Sampler.ComparisonFunction = Comparison::Greater_Equal;
			SamplerStates["a1_fl_clamp_cmp_gte"] = CreateSamplerState(Sampler);

			InputLayout::Desc Layout;
			Layout.Attributes =
			{
				{ "POSITION", 0, AttributeType::Float, 3, 0 },
				{ "TEXCOORD", 0, AttributeType::Float, 2, 3 * sizeof(float) }
			};
			InputLayouts["vx_shape"] = CreateInputLayout(Layout);

			Layout.Attributes =
			{
				{ "POSITION", 0, AttributeType::Float, 3, 0 },
				{ "TEXCOORD", 0, AttributeType::Float, 4, 3 * sizeof(float) },
				{ "TEXCOORD", 1, AttributeType::Float, 4, 7 * sizeof(float) },
				{ "TEXCOORD", 2, AttributeType::Float, 3, 11 * sizeof(float) }
			};
			InputLayouts["vx_element"] = CreateInputLayout(Layout);

			Layout.Attributes =
			{
				{ "POSITION", 0, AttributeType::Float, 3, 0 },
				{ "TEXCOORD", 0, AttributeType::Float, 2, 3 * sizeof(float) },
				{ "NORMAL", 0, AttributeType::Float, 3, 5 * sizeof(float) },
				{ "TANGENT", 0, AttributeType::Float, 3, 8 * sizeof(float) },
				{ "BINORMAL", 0, AttributeType::Float, 3, 11 * sizeof(float) }
			};
			InputLayouts["vx_base"] = CreateInputLayout(Layout);

			Layout.Attributes =
			{
				{ "POSITION", 0, AttributeType::Float, 3, 0 },
				{ "TEXCOORD", 0, AttributeType::Float, 2, 3 * sizeof(float) },
				{ "NORMAL", 0, AttributeType::Float, 3, 5 * sizeof(float) },
				{ "TANGENT", 0, AttributeType::Float, 3, 8 * sizeof(float) },
				{ "BINORMAL", 0, AttributeType::Float, 3, 11 * sizeof(float) },
				{ "OB_TRANSFORM", 0, AttributeType::Matrix, 16, 0, 1, false },
				{ "OB_WORLD", 0, AttributeType::Matrix, 16, sizeof(Compute::Matrix4x4), 1, false },
				{ "OB_TEXCOORD", 0, AttributeType::Float, 2, sizeof(Compute::Matrix4x4) * 2, 1, false },
				{ "OB_MATERIAL", 0, AttributeType::Float, 4, sizeof(Compute::Matrix4x4) * 2 + 2 * sizeof(float), 1, false }
			};
			InputLayouts["vxi_base"] = CreateInputLayout(Layout);

			Layout.Attributes =
			{
				{ "POSITION", 0, AttributeType::Float, 3, 0 },
				{ "TEXCOORD", 0, AttributeType::Float, 2, 3 * sizeof(float) },
				{ "NORMAL", 0, AttributeType::Float, 3, 5 * sizeof(float) },
				{ "TANGENT", 0, AttributeType::Float, 3, 8 * sizeof(float) },
				{ "BINORMAL", 0, AttributeType::Float, 3, 11 * sizeof(float) },
				{ "JOINTBIAS", 0, AttributeType::Float, 4, 14 * sizeof(float) },
				{ "JOINTBIAS", 1, AttributeType::Float, 4, 18 * sizeof(float) }
			};
			InputLayouts["vx_skin"] = CreateInputLayout(Layout);

			Layout.Attributes =
			{
				{ "POSITION", 0, AttributeType::Float, 2, 0 },
				{ "COLOR", 0, AttributeType::Ubyte, 4, 2 * sizeof(float) },
				{ "TEXCOORD", 0, AttributeType::Float, 2, 2 * sizeof(float) + 4 * sizeof(unsigned char) }
			};
			InputLayouts["vx_ui"] = CreateInputLayout(Layout);

			SetDepthStencilState(GetDepthStencilState("drw_srw_lt"));
			SetRasterizerState(GetRasterizerState("so_cback"));
			SetBlendState(GetBlendState("bo_wrgba_one"));
		}
		void GraphicsDevice::CreateSections()
		{
#ifdef HAS_SHADER_BUNDLE
			shader_bundle::foreach(this, [](void* Context, const char* Name, const unsigned char* Buffer, unsigned Size)
			{
				GraphicsDevice* Base = (GraphicsDevice*)Context;
				if (Base != nullptr && Base->GetBackend() != RenderBackend::None)
					Base->AddSection(Name, Core::String((const char*)Buffer, Size - 1));
			});
#else
			VI_WARN("[graphics] default shader resources were not compiled");
#endif
		}
		void GraphicsDevice::ReleaseProxy()
		{
			for (auto It = DepthStencilStates.begin(); It != DepthStencilStates.end(); It++)
				VI_RELEASE(It->second);
			DepthStencilStates.clear();

			for (auto It = RasterizerStates.begin(); It != RasterizerStates.end(); It++)
				VI_RELEASE(It->second);
			RasterizerStates.clear();

			for (auto It = BlendStates.begin(); It != BlendStates.end(); It++)
				VI_RELEASE(It->second);
			BlendStates.clear();

			for (auto It = SamplerStates.begin(); It != SamplerStates.end(); It++)
				VI_RELEASE(It->second);
			SamplerStates.clear();

			for (auto It = InputLayouts.begin(); It != InputLayouts.end(); It++)
				VI_RELEASE(It->second);
			InputLayouts.clear();
			VI_CLEAR(RenderTarget);
		}
		bool GraphicsDevice::AddSection(const Core::String& Name, const Core::String& Code)
		{
			Core::String Language(Core::OS::Path::GetExtension(Name.c_str()));
			if (Language.empty())
				return false;

			Language.erase(0, 1);
			Core::Stringify::Trim(Language);
			Core::Stringify::ToLower(Language);
			RemoveSection(Name);

			Section* Include = VI_NEW(Section);
			Include->Code = Code;
			Include->Name = Name;
			Sections[Name] = Include;

			return true;
		}
		bool GraphicsDevice::RemoveSection(const Core::String& Name)
		{
			auto It = Sections.find(Name);
			if (It == Sections.end())
				return false;

			VI_DELETE(Section, It->second);
			Sections.erase(It);

			return true;
		}
		bool GraphicsDevice::Preprocess(Shader::Desc& Subresult)
		{
			if (Subresult.Data.empty())
				return true;

			switch (Backend)
			{
				case Mavi::Graphics::RenderBackend::D3D11:
					Subresult.Defines.push_back("TARGET_D3D");
					break;
				case Mavi::Graphics::RenderBackend::OGL:
					Subresult.Defines.push_back("TARGET_OGL");
					break;
                default:
                    break;
			}

			Compute::IncludeDesc Desc = Compute::IncludeDesc();
			Desc.Exts.push_back(".hlsl");
			Desc.Exts.push_back(".glsl");
			Desc.Exts.push_back(".msl");
			Desc.Exts.push_back(".spv");
			Subresult.Features.Pragmas = false;

			auto Directory = Core::OS::Directory::GetWorking();
			if (Directory)
				Desc.Root = *Directory;

			Compute::Preprocessor* Processor = new Compute::Preprocessor();
			Processor->SetIncludeCallback([this, &Subresult](Compute::Preprocessor* P, const Compute::IncludeResult& File, Core::String& Output)
			{
				if (Subresult.Include)
				{
					Compute::IncludeType Status = Subresult.Include(P, File, Output);
					if (Status != Compute::IncludeType::Error)
						return Status;
				}

				if (File.Module.empty() || (!File.IsFile && !File.IsAbstract))
					return Compute::IncludeType::Error;

				if (File.IsAbstract && !File.IsFile)
				{
					Section* Result;
					if (!GetSection(File.Module, &Result, true))
						return Compute::IncludeType::Error;

					Output.assign(Result->Code);
					return Compute::IncludeType::Preprocess;
				}

				auto Data = Core::OS::File::ReadAsString(File.Module);
				if (!Data)
					return Compute::IncludeType::Error;

				Output.assign(*Data);
				return Compute::IncludeType::Preprocess;
			});
			Processor->SetIncludeOptions(Desc);
			Processor->SetFeatures(Subresult.Features);

			for (auto& Word : Subresult.Defines)
				Processor->Define(Word);

			bool Preprocessed = Processor->Process(Subresult.Filename, Subresult.Data);
			VI_RELEASE(Processor);

			return Preprocessed;
		}
		bool GraphicsDevice::Transpile(Core::String* HLSL, ShaderType Type, ShaderLang To)
		{
			if (!HLSL || HLSL->empty())
				return true;
#ifdef VI_SPIRV
			const char* Buffer = HLSL->c_str();
			int Size = (int)HLSL->size();

			ShaderModel Model = GetShaderModel();
			VI_ASSERT(Model == ShaderModel::Auto || Model == ShaderModel::Invalid, "transpilation requests a defined shader model to proceed");

			EShLanguage Stage;
			switch (Type)
			{
				case ShaderType::Vertex:
					Stage = EShLangVertex;
					break;
				case ShaderType::Pixel:
					Stage = EShLangFragment;
					break;
				case ShaderType::Geometry:
					Stage = EShLangGeometry;
					break;
				case ShaderType::Hull:
					Stage = EShLangTessControl;
					break;
				case ShaderType::Domain:
					Stage = EShLangTessEvaluation;
					break;
				case ShaderType::Compute:
					Stage = EShLangCompute;
					break;
				default:
					VI_ASSERT(false, "transpilation requests a defined shader type to proceed");
					return false;
			}

			Core::String Entry = GetShaderMain(Type);
			std::vector<uint32_t> Binary;
			glslang::InitializeProcess();

			glslang::TShader Transpiler(Stage);
			Transpiler.setStringsWithLengths(&Buffer, &Size, 1);
			Transpiler.setAutoMapBindings(true);
			Transpiler.setAutoMapLocations(true);
			Transpiler.setEnvInput(glslang::EShSourceHlsl, Stage, glslang::EShClientVulkan, 100);
			Transpiler.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_4);
			Transpiler.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_1);
			Transpiler.setEntryPoint(Entry.c_str());

			EShMessages Flags = (EShMessages)(EShMsgSpvRules | EShMsgReadHlsl | EShMsgHlslOffsets | EShMsgHlslLegalization | EShMsgKeepUncalled | EShMsgSuppressWarnings);
            PrepareDriverLimits();
            
            if (!Transpiler.parse(&DriverLimits, 100, true, Flags))
			{
				const char* Output = Transpiler.getInfoLog();
				VI_ERR("[graphics] %s", Output);
				glslang::FinalizeProcess();
				return false;
			}

			try
			{
				glslang::SpvOptions Options;
				Options.validate = false;
				Options.disableOptimizer = false;
				Options.optimizeSize = false;

				spv::SpvBuildLogger Logger;
				glslang::TIntermediate* Context = Transpiler.getIntermediate();
				glslang::GlslangToSpv(*Context, Binary, &Logger, &Options);
			}
			catch (...)
			{
				VI_ERR("[graphics] cannot transpile shader source to spirv binary");
				glslang::FinalizeProcess();
				return false;
			}

			glslang::FinalizeProcess();
			try
			{
				if (To == ShaderLang::GLSL || To == ShaderLang::GLSL_ES)
				{
					spirv_cross::CompilerGLSL::Options Options;
					Options.version = (uint32_t)Model;
					Options.es = (To == ShaderLang::GLSL_ES);

					spirv_cross::CompilerGLSL Compiler(Binary);
					Compiler.set_common_options(Options);
					Compiler.build_dummy_sampler_for_combined_images();
					Compiler.build_combined_image_samplers();
					PrepareSamplers(&Compiler);

					*HLSL = Core::Copy<Core::String>(Compiler.compile());
					if (HLSL->empty())
						return true;

					Core::Stringify::ReplaceGroups(*HLSL, "layout\\(row_major\\)\\s+", "");
					Core::Stringify::ReplaceGroups(*HLSL, "invocations\\s+=\\s+\\d+,\\s+", "");
					return true;
				}
				else if (To == ShaderLang::HLSL)
				{
#ifdef VI_MICROSOFT
					spirv_cross::CompilerHLSL::Options Options;
					Options.shader_model = (uint32_t)Model;

					spirv_cross::CompilerHLSL Compiler(Binary);
					Compiler.set_hlsl_options(Options);

					*HLSL = Core::Copy<Core::String>(Compiler.compile());
					return true;
#else
					VI_ERR("[graphics] cannot transpile spirv binary to shader binary: hlsl is not supported");
					return false;
#endif
				}
				else if (To == ShaderLang::MSL)
				{
#ifdef VI_APPLE
					spirv_cross::CompilerMSL::Options Options;
					spirv_cross::CompilerMSL Compiler(Binary);
					Compiler.set_msl_options(Options);
					Compiler.build_dummy_sampler_for_combined_images();
					Compiler.build_combined_image_samplers();
					PrepareSamplers(&Compiler);

					*HLSL = Core::Copy<Core::String>(Compiler.compile());
					return true;
#else
					VI_ERR("[graphics] cannot transpile spirv binary to shader binary: msl is not supported");
					return false;
#endif
				}
				else if (To == ShaderLang::SPV)
				{
					Core::StringStream Stream;
					std::copy(Binary.begin(), Binary.end(), std::ostream_iterator<uint32_t>(Stream, " "));

					HLSL->assign(Stream.str());
					return true;
				}
			}
			catch (const spirv_cross::CompilerError& Exception)
			{
				VI_ERR("[graphics] cannot transpile spirv binary to shader binary: %s", Exception.what());
				(void)Exception;
				return false;
			}
			catch (...)
			{
				VI_ERR("[graphics] cannot transpile spirv binary to shader binary");
				return false;
			}

			VI_ASSERT(false, "shader source can be transpiled only to GLSL, GLSL_ES, HLSL, MSL or SPV");
			return false;
#else
			VI_ASSERT(false, "cannot transpile shader source without spirv-cross and glslang libraries");
			return false;
#endif
		}
		bool GraphicsDevice::GetSection(const Core::String& Name, Section** Result, bool Internal)
		{
			if (Name.empty() || Sections.empty())
			{
				if (!Internal)
					VI_ERR("[graphics] could not find shader: \"%s\"");

				return false;
			}

			std::function<bool(const Core::String&)> Resolve = [this, &Result](const Core::String& Src)
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
				VI_ERR("[graphics] could not find shader: \"%s\"", Name.c_str());

			return false;
		}
		bool GraphicsDevice::GetSection(const Core::String& Name, Shader::Desc* Result)
		{
			if (Name.empty() || !Result)
				return false;

			Section* Subresult;
			if (!GetSection(Name, &Subresult, false))
				return false;

			Result->Filename.assign(Subresult->Name);
			Result->Data.assign(Subresult->Code);

			return true;
		}
		bool GraphicsDevice::IsDebug() const
		{
			return Debug;
		}
		bool GraphicsDevice::GetProgramCache(const Core::String& Name, Core::String* Data)
		{
			VI_ASSERT(Data != nullptr, "data should be set");
			Data->clear();

			if (!ShaderCache || Caches.empty())
				return false;

			Core::String Path = Caches + Name;
			if (Path.empty())
				return false;

			if (!Core::OS::File::IsExists(Path.c_str()))
				return false;

			Core::GzStream* Stream = new Core::GzStream();
			if (!Stream->Open(Path.c_str(), Core::FileMode::Binary_Read_Only))
			{
				VI_RELEASE(Stream);
				return false;
			}

			char Buffer[Core::BLOB_SIZE]; size_t Size = 0;
			while ((Size = (size_t)Stream->Read(Buffer, sizeof(Buffer))) > 0)
				Data->append(Core::String(Buffer, Size));

			VI_DEBUG("[graphics] load %s program cache", Name.c_str());
			VI_RELEASE(Stream);

			return !Data->empty();
		}
		bool GraphicsDevice::SetProgramCache(const Core::String& Name, const Core::String& Data)
		{
			if (!ShaderCache || Caches.empty())
				return true;

			Core::String Path = Caches + Name;
			if (Path.empty())
				return false;

			Core::GzStream* Stream = new Core::GzStream();
			if (!Stream->Open(Path.c_str(), Core::FileMode::Binary_Write_Only))
			{
				VI_RELEASE(Stream);
				return false;
			}

			size_t Size = Data.size();
			bool Result = (Stream->Write(Data.c_str(), Size) == Size);
			VI_DEBUG("[graphics] save %s program cache", Name.c_str());
			VI_RELEASE(Stream);

			return Result;
		}
		bool GraphicsDevice::IsLeftHanded() const
		{
			return Backend == RenderBackend::D3D11;
		}
		unsigned int GraphicsDevice::GetRowPitch(unsigned int Width, unsigned int ElementSize) const
		{
			return Width * ElementSize;
		}
		unsigned int GraphicsDevice::GetDepthPitch(unsigned int RowPitch, unsigned int Height) const
		{
			return RowPitch * Height;
		}
		unsigned int GraphicsDevice::GetMipLevel(unsigned int Width, unsigned int Height) const
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
		unsigned int GraphicsDevice::GetFormatSize(Format Mode) const
		{
			switch (Mode)
			{
				case Format::A8_Unorm:
				case Format::R1_Unorm:
				case Format::R8_Sint:
				case Format::R8_Snorm:
				case Format::R8_Uint:
				case Format::R8_Unorm:
					return 1;
				case Format::D16_Unorm:
				case Format::R16_Float:
				case Format::R16_Sint:
				case Format::R16_Snorm:
				case Format::R16_Uint:
				case Format::R16_Unorm:
				case Format::R8G8_Sint:
				case Format::R8G8_Snorm:
				case Format::R8G8_Uint:
				case Format::R8G8_Unorm:
					return 2;
				case Format::D24_Unorm_S8_Uint:
				case Format::D32_Float:
				case Format::R10G10B10A2_Uint:
				case Format::R10G10B10A2_Unorm:
				case Format::R11G11B10_Float:
				case Format::R16G16_Float:
				case Format::R16G16_Sint:
				case Format::R16G16_Snorm:
				case Format::R16G16_Uint:
				case Format::R16G16_Unorm:
				case Format::R32_Float:
				case Format::R32_Sint:
				case Format::R32_Uint:
				case Format::R8G8B8A8_Sint:
				case Format::R8G8B8A8_Snorm:
				case Format::R8G8B8A8_Uint:
				case Format::R8G8B8A8_Unorm:
				case Format::R8G8B8A8_Unorm_SRGB:
				case Format::R8G8_B8G8_Unorm:
				case Format::R9G9B9E5_Share_Dexp:
					return 4;
				case Format::R16G16B16A16_Float:
				case Format::R16G16B16A16_Sint:
				case Format::R16G16B16A16_Snorm:
				case Format::R16G16B16A16_Uint:
				case Format::R16G16B16A16_Unorm:
				case Format::R32G32_Float:
				case Format::R32G32_Sint:
				case Format::R32G32_Uint:
					return 8;
				case Format::R32G32B32A32_Float:
				case Format::R32G32B32A32_Sint:
				case Format::R32G32B32A32_Uint:
					return 16;
				case Format::R32G32B32_Float:
				case Format::R32G32B32_Sint:
				case Format::R32G32B32_Uint:
					return 12;
				default:
					return 0;
			}
		}
		unsigned int GraphicsDevice::GetPresentFlags() const
		{
			return PresentFlags;
		}
		unsigned int GraphicsDevice::GetCompileFlags() const
		{
			return CompileFlags;
		}
		Core::Option<Core::String> GraphicsDevice::GetProgramName(const Shader::Desc& Desc)
		{
			Core::String Result = Desc.Filename;
			for (auto& Item : Desc.Defines)
				Result += '&' + Item + "=1";

			if (Desc.Features.Pragmas)
				Result += "&pragmas=on";

			if (Desc.Features.Includes)
				Result += "&includes=on";

			if (Desc.Features.Defines)
				Result += "&defines=on";

			if (Desc.Features.Conditions)
				Result += "&conditions=on";

			switch (Desc.Stage)
			{
				case ShaderType::Vertex:
					Result += "&stage=vertex";
					break;
				case ShaderType::Pixel:
					Result += "&stage=pixel";
					break;
				case ShaderType::Geometry:
					Result += "&stage=geometry";
					break;
				case ShaderType::Hull:
					Result += "&stage=hull";
					break;
				case ShaderType::Domain:
					Result += "&stage=domain";
					break;
				case ShaderType::Compute:
					Result += "&stage=compute";
					break;
				default:
					break;
			}

			auto Hash = Compute::Crypto::HashHex(Compute::Digests::MD5(), Result);
			if (!Hash)
				return Hash;

			Core::String Postfix;
			switch (Backend)
			{
				case Mavi::Graphics::RenderBackend::D3D11:
					Postfix = ".hlsl";
					break;
				case Mavi::Graphics::RenderBackend::OGL:
					Postfix = ".glsl";
					break;
                default:
                    break;
			}

			return *Hash + Postfix;
		}
		Core::String GraphicsDevice::GetShaderMain(ShaderType Type) const
		{
			switch (Type)
			{
				case ShaderType::Vertex:
					return "vs_main";
				case ShaderType::Pixel:
					return "ps_main";
				case ShaderType::Geometry:
					return "gs_main";
				case ShaderType::Hull:
					return "hs_main";
				case ShaderType::Domain:
					return "ds_main";
				case ShaderType::Compute:
					return "cs_main";
				default:
					return "main";
			}
		}
		ShaderModel GraphicsDevice::GetShaderModel() const
		{
			return ShaderGen;
		}
		const Core::UnorderedMap<Core::String, DepthStencilState*>& GraphicsDevice::GetDepthStencilStates() const
		{
			return DepthStencilStates;
		}
		const Core::UnorderedMap<Core::String, RasterizerState*>& GraphicsDevice::GetRasterizerStates() const
		{
			return RasterizerStates;
		}
		const Core::UnorderedMap<Core::String, BlendState*>& GraphicsDevice::GetBlendStates() const
		{
			return BlendStates;
		}
		const Core::UnorderedMap<Core::String, SamplerState*>& GraphicsDevice::GetSamplerStates() const
		{
			return SamplerStates;
		}
		const Core::UnorderedMap<Core::String, InputLayout*>& GraphicsDevice::GetInputLayouts() const
		{
			return InputLayouts;
		}
		Surface* GraphicsDevice::CreateSurface(Texture2D* Base)
		{
			VI_ASSERT(Base != nullptr, "texture should be set");
#ifdef VI_SDL2
			int Width = (int)Base->GetWidth();
			int Height = (int)Base->GetHeight();
			int BytesPerPixel = GetFormatSize(Base->GetFormatMode());
			int BitsPerPixel = BytesPerPixel * 8;

			Texture2D::Desc Desc;
			Desc.AccessFlags = CPUAccess::Read;
			Desc.Usage = ResourceUsage::Staging;
			Desc.BindFlags = (ResourceBind)0;
			Desc.FormatMode = Base->GetFormatMode();
			Desc.Width = Base->GetWidth();
			Desc.Height = Base->GetHeight();
			Desc.MipLevels = Base->GetMipLevels();

			Texture2D* Copy = CreateTexture2D(Desc);
			if (!CopyTexture2D(Base, &Copy))
			{
				VI_ERR("[graphics] cannot copy temporary texture 2d for reading");
				VI_RELEASE(Copy);
				return nullptr;
			}

			MappedSubresource Data;
			if (!Map(Copy, ResourceMap::Read, &Data))
			{
				VI_ERR("[graphics] cannot map temporary texture 2d");
				VI_RELEASE(Copy);
				return nullptr;
			}
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
			const Uint32 R = 0xff000000;
			const Uint32 G = 0x00ff0000;
			const Uint32 B = 0x0000ff00;
			const Uint32 A = 0x000000ff;
#else
			const Uint32 R = 0x000000ff;
			const Uint32 G = 0x0000ff00;
			const Uint32 B = 0x00ff0000;
			const Uint32 A = 0xff000000;
#endif
			SDL_Surface* Handle = SDL_CreateRGBSurface(0, Width, Height, BitsPerPixel, R, G, B, A);
			if (Handle != nullptr)
			{
				SDL_SetSurfaceBlendMode(Handle, SDL_BlendMode::SDL_BLENDMODE_BLEND);
				SDL_LockSurface(Handle);
				memcpy(Handle->pixels, Data.Pointer, Width * Height * BytesPerPixel);
				SDL_UnlockSurface(Handle);
			}

			Unmap(Copy, &Data);
			VI_RELEASE(Copy);

			if (!Handle)
			{
				VI_ERR("[graphics] cannot create surface from texture 2d");
				return nullptr;
			}

			return new Surface(Handle);
#else
			return nullptr;
#endif
		}
		DepthStencilState* GraphicsDevice::GetDepthStencilState(const Core::String& Name)
		{
			auto It = DepthStencilStates.find(Name);
			if (It != DepthStencilStates.end())
				return It->second;

			return nullptr;
		}
		BlendState* GraphicsDevice::GetBlendState(const Core::String& Name)
		{
			auto It = BlendStates.find(Name);
			if (It != BlendStates.end())
				return It->second;

			return nullptr;
		}
		RasterizerState* GraphicsDevice::GetRasterizerState(const Core::String& Name)
		{
			auto It = RasterizerStates.find(Name);
			if (It != RasterizerStates.end())
				return It->second;

			return nullptr;
		}
		SamplerState* GraphicsDevice::GetSamplerState(const Core::String& Name)
		{
			auto It = SamplerStates.find(Name);
			if (It != SamplerStates.end())
				return It->second;

			return nullptr;
		}
		InputLayout* GraphicsDevice::GetInputLayout(const Core::String& Name)
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
		RenderBackend GraphicsDevice::GetBackend() const
		{
			return Backend;
		}
		VSync GraphicsDevice::GetVSyncMode() const
		{
			return VSyncMode;
		}
		GraphicsDevice* GraphicsDevice::Create(Desc& I)
		{
			I.Backend = GetSupportedBackend(I.Backend);
#ifdef VI_MICROSOFT
			if (I.Backend == RenderBackend::D3D11)
				return new D3D11::D3D11Device(I);
#endif
#ifdef VI_GL
			if (I.Backend == RenderBackend::OGL)
				return new OGL::OGLDevice(I);
#endif
			VI_PANIC(false, "renderer backend is not present or is invalid");
			return nullptr;
		}
		void GraphicsDevice::CompileBuiltinShaders(const Core::Vector<GraphicsDevice*>& Devices)
		{
			auto GetDeviceName = [](GraphicsDevice* Device) -> const char*
			{
				switch (Device->GetBackend())
				{
					case RenderBackend::D3D11:
						return "d3d11";
					case RenderBackend::OGL:
						return "opengl";
					default:
						return "unknown";
				}
			};

			for (auto* Device : Devices)
			{
				if (!Device)
					continue;

				Device->SetAsCurrentDevice();
				for (auto& Section : Device->Sections)
				{
					Shader::Desc Desc;
					if (Device->GetSection(Section.first, &Desc))
					{
						Shader* Result = Device->CreateShader(Desc);
						if (Result != nullptr)
							VI_INFO("[graphics] OK compile %s %s shader", GetDeviceName(Device), Section.first.c_str());
						else
							VI_ERR("[graphics] cannot compile %s %s shader", GetDeviceName(Device), Section.first.c_str());
						VI_RELEASE(Result);
					}
				}
			}
		}

		Activity::Activity(const Desc& I) noexcept : Handle(nullptr), Favicon(nullptr), Options(I), Command(0), CX(0), CY(0), Message(this)
		{
#ifdef VI_SDL2
			Cursors[(size_t)DisplayCursor::Arrow] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW);
			Cursors[(size_t)DisplayCursor::TextInput] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_IBEAM);
			Cursors[(size_t)DisplayCursor::ResizeAll] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEALL);
			Cursors[(size_t)DisplayCursor::ResizeNS] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENS);
			Cursors[(size_t)DisplayCursor::ResizeEW] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEWE);
			Cursors[(size_t)DisplayCursor::ResizeNESW] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENESW);
			Cursors[(size_t)DisplayCursor::ResizeNWSE] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENWSE);
			Cursors[(size_t)DisplayCursor::Hand] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_HAND);
			Cursors[(size_t)DisplayCursor::Crosshair] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_CROSSHAIR);
			Cursors[(size_t)DisplayCursor::Wait] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_WAIT);
			Cursors[(size_t)DisplayCursor::Progress] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_WAITARROW);
			Cursors[(size_t)DisplayCursor::No] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_NO);
#endif
			memset(Keys[0], 0, sizeof(Keys[0]));
			memset(Keys[1], 0, sizeof(Keys[1]));
			if (!I.GPUAsRenderer)
				BuildLayer(RenderBackend::None);
		}
		Activity::~Activity() noexcept
		{
#ifdef VI_SDL2
			for (size_t i = 0; i < (size_t)DisplayCursor::Count; i++)
				SDL_FreeCursor(Cursors[i]);

			if (Handle != nullptr)
			{
				SDL_DestroyWindow(Handle);
				Handle = nullptr;
			}
#endif
		}
		void Activity::ApplySystemTheme()
		{
#ifdef VI_SDL2
#ifdef VI_MICROSOFT
			OSVERSIONINFOEX Version;
			ZeroMemory(&Version, sizeof(OSVERSIONINFOEX));
			Version.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
#pragma warning(push)
#pragma warning(disable : 4996)
			GetVersionEx((LPOSVERSIONINFO)&Version);
#pragma warning(pop) 

			const DWORD DWMWA_USE_IMMERSIVE_DARK_MODE = Version.dwBuildNumber >= 18985 ? 20 : 19;
			if (Version.dwMajorVersion < 10 || Version.dwBuildNumber < 17763)
				return;

			HKEY Target;
			if (RegOpenKeyEx(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", 0, KEY_QUERY_VALUE, &Target) == ERROR_SUCCESS)
			{
				DWORD Value = 0, ValueSize = sizeof(DWORD), Type = REG_DWORD;
				if (RegQueryValueEx(Target, "SystemUsesLightTheme", NULL, &Type, (LPBYTE)&Value, &ValueSize) == ERROR_SUCCESS && Value == 0)
				{
					SDL_SysWMinfo Info;
					SDL_VERSION(&Info.version);
					SDL_GetWindowWMInfo(Handle, &Info);
					HWND WindowHandle = Info.info.win.window;
					HMODULE Library = LoadLibraryA("dwmapi.dll");

					if (Library != nullptr)
					{
						typedef HRESULT(*DwmSetWindowAttributePtr1)(HWND, DWORD, LPCVOID, DWORD);
						DwmSetWindowAttributePtr1 DWM_SetWindowAttribute = (DwmSetWindowAttributePtr1)GetProcAddress(Library, "DwmSetWindowAttribute");
						if (DWM_SetWindowAttribute != nullptr)
						{
							BOOL DarkMode = true;
							DWM_SetWindowAttribute(WindowHandle, DWMWA_USE_IMMERSIVE_DARK_MODE, &DarkMode, sizeof(DarkMode));
						}
						//FreeLibrary(Library); // Needs to be present until app closes otherwise may crash
					}
				}
				RegCloseKey(Target);
			}
#endif
#endif
		}
		void Activity::SetClipboardText(const Core::String& Text)
		{
#ifdef VI_SDL2
			VI_ASSERT(Handle != nullptr, "activity should be initialized");
			SDL_SetClipboardText(Text.c_str());
#endif
		}
		void Activity::SetCursorPosition(const Compute::Vector2& Position)
		{
#ifdef VI_SDL2
#if SDL_VERSION_ATLEAST(2, 0, 4)
			VI_ASSERT(Handle != nullptr, "activity should be initialized");
			SDL_WarpMouseInWindow(Handle, (int)Position.X, (int)Position.Y);
#endif
#endif
		}
		void Activity::SetCursorPosition(float X, float Y)
		{
#ifdef VI_SDL2
#if SDL_VERSION_ATLEAST(2, 0, 4)
			VI_ASSERT(Handle != nullptr, "activity should be initialized");
			SDL_WarpMouseInWindow(Handle, (int)X, (int)Y);
#endif
#endif
		}
		void Activity::SetGlobalCursorPosition(const Compute::Vector2& Position)
		{
#ifdef VI_SDL2
			VI_ASSERT(Handle != nullptr, "activity should be initialized");
#if SDL_VERSION_ATLEAST(2, 0, 4)
			SDL_WarpMouseGlobal((int)Position.X, (int)Position.Y);
#endif
#endif
		}
		void Activity::SetGlobalCursorPosition(float X, float Y)
		{
#ifdef VI_SDL2
			VI_ASSERT(Handle != nullptr, "activity should be initialized");
#if SDL_VERSION_ATLEAST(2, 0, 4)
			SDL_WarpMouseGlobal((int)X, (int)Y);
#endif
#endif
		}
		void Activity::SetKey(KeyCode Key, bool Value)
		{
			Keys[0][(size_t)Key] = Value;
		}
		void Activity::SetCursor(DisplayCursor Style)
		{
#ifdef VI_SDL2
			VI_ASSERT((size_t)Style <= (size_t)DisplayCursor::Count, "style should be less than %i", (int)DisplayCursor::Count);
			if (Style != DisplayCursor::None)
			{
				SDL_ShowCursor(1);
				SDL_SetCursor(Cursors[(size_t)Style]);
			}
			else
			{
				SDL_ShowCursor(0);
				SDL_SetCursor(Cursors[(size_t)DisplayCursor::Arrow]);
			}
#endif
		}
		void Activity::SetCursorVisibility(bool Enabled)
		{
#ifdef VI_SDL2
			VI_ASSERT(Handle != nullptr, "activity should be initialized");
			SDL_ShowCursor(Enabled);
#endif
		}
		void Activity::SetGrabbing(bool Enabled)
		{
#ifdef VI_SDL2
			VI_ASSERT(Handle != nullptr, "activity should be initialized");
			SDL_SetWindowGrab(Handle, Enabled ? SDL_TRUE : SDL_FALSE);
#endif
		}
		void Activity::SetFullscreen(bool Enabled)
		{
#ifdef VI_SDL2
			VI_ASSERT(Handle != nullptr, "activity should be initialized");
			SDL_SetWindowFullscreen(Handle, Enabled ? SDL_WINDOW_FULLSCREEN : 0);
#endif
		}
		void Activity::SetBorderless(bool Enabled)
		{
#ifdef VI_SDL2
			VI_ASSERT(Handle != nullptr, "activity should be initialized");
			SDL_SetWindowBordered(Handle, Enabled ? SDL_TRUE : SDL_FALSE);
#endif
		}
		void Activity::SetScreenKeyboard(bool Enabled)
		{
#ifdef VI_SDL2
			VI_ASSERT(Handle != nullptr, "activity should be initialized");
			if (!SDL_HasScreenKeyboardSupport())
				return;

			if (Enabled)
				SDL_StartTextInput();
			else
				SDL_StopTextInput();
#endif
		}
		void Activity::BuildLayer(RenderBackend Backend)
		{
#ifdef VI_SDL2
			if (Handle != nullptr)
			{
				SDL_DestroyWindow(Handle);
				Handle = nullptr;
			}

			Uint32 Flags = 0;
			if (Options.Fullscreen)
				Flags |= SDL_WINDOW_FULLSCREEN;

			if (Options.Hidden)
				Flags |= SDL_WINDOW_HIDDEN;

			if (Options.Borderless)
				Flags |= SDL_WINDOW_BORDERLESS;

			if (Options.Resizable)
				Flags |= SDL_WINDOW_RESIZABLE;

			if (Options.Minimized)
				Flags |= SDL_WINDOW_MINIMIZED;

			if (Options.Maximized)
				Flags |= SDL_WINDOW_MAXIMIZED;

			if (Options.Focused)
				Flags |= SDL_WINDOW_INPUT_GRABBED;

			if (Options.HighDPI)
				Flags |= SDL_WINDOW_ALLOW_HIGHDPI;

			if (Options.Centered)
			{
				Options.X = SDL_WINDOWPOS_CENTERED;
				Options.Y = SDL_WINDOWPOS_CENTERED;
			}
			else if (Options.FreePosition)
			{
				Options.X = SDL_WINDOWPOS_UNDEFINED;
				Options.Y = SDL_WINDOWPOS_UNDEFINED;
			}

			switch (Backend)
			{
				case Mavi::Graphics::RenderBackend::OGL:
					Flags |= SDL_WINDOW_OPENGL;
					break;
				default:
					break;
			}

			Handle = SDL_CreateWindow(Options.Title.c_str(), Options.X, Options.Y, Options.Width, Options.Height, Flags);
			if (Handle != nullptr)
				ApplySystemTheme();
#endif
		}
		void Activity::Wakeup()
		{
#ifdef VI_SDL2
			SDL_Event Event;
			Event.type = SDL_USEREVENT;
			Event.user.timestamp = SDL_GetTicks();
			Event.user.windowID = SDL_GetWindowID(Handle);
			Event.user.code = 200;
			Event.user.data1 = nullptr;
			Event.user.data2 = nullptr;
			SDL_PushEvent(&Event);
#endif
		}
		void Activity::Hide()
		{
#ifdef VI_SDL2
			VI_ASSERT(Handle != nullptr, "activity should be initialized");
			SDL_HideWindow(Handle);
#endif
		}
		void Activity::Show()
		{
#ifdef VI_SDL2
			VI_ASSERT(Handle != nullptr, "activity should be initialized");
			SDL_ShowWindow(Handle);
#endif
		}
		void Activity::Maximize()
		{
#ifdef VI_SDL2
			VI_ASSERT(Handle != nullptr, "activity should be initialized");
			SDL_MaximizeWindow(Handle);
#endif
		}
		void Activity::Minimize()
		{
#ifdef VI_SDL2
			VI_ASSERT(Handle != nullptr, "activity should be initialized");
			SDL_MinimizeWindow(Handle);
#endif
		}
		void Activity::Focus()
		{
#ifdef VI_SDL2
#if SDL_VERSION_ATLEAST(2, 0, 5)
			VI_ASSERT(Handle != nullptr, "activity should be initialized");
			SDL_SetWindowInputFocus(Handle);
#endif
#endif
		}
		void Activity::Move(int X, int Y)
		{
#ifdef VI_SDL2
			VI_ASSERT(Handle != nullptr, "activity should be initialized");
			SDL_SetWindowPosition(Handle, X, Y);
#endif
		}
		void Activity::Resize(int W, int H)
		{
#ifdef VI_SDL2
			VI_ASSERT(Handle != nullptr, "activity should be initialized");
			SDL_SetWindowSize(Handle, W, H);
#endif
		}
		void Activity::SetTitle(const char* Value)
		{
#ifdef VI_SDL2
			VI_ASSERT(Handle != nullptr, "activity should be initialized");
			VI_ASSERT(Value != nullptr, "value should be set");
			SDL_SetWindowTitle(Handle, Value);
#endif
		}
		void Activity::SetIcon(Surface* Icon)
		{
#ifdef VI_SDL2
			VI_ASSERT(Handle != nullptr, "activity should be initialized");
			if (!Favicon)
				Favicon = SDL_GetWindowSurface(Handle);

			SDL_SetWindowIcon(Handle, Icon ? (SDL_Surface*)Icon->GetResource() : Favicon);
#endif
		}
		void Activity::Load(SDL_SysWMinfo* Base)
		{
#ifdef VI_SDL2
			VI_ASSERT(Handle != nullptr, "activity should be initialized");
			VI_ASSERT(Base != nullptr, "base should be set");
			SDL_VERSION(&Base->version);
			SDL_GetWindowWMInfo(Handle, Base);
#endif
		}
		bool Activity::Dispatch()
		{
			return DispatchBlocking(0);
		}
		bool Activity::DispatchBlocking(uint64_t TimeoutMs)
		{
			VI_ASSERT(Handle != nullptr, "activity should be initialized");
			VI_MEASURE(Core::Timings::Mixed);

			memcpy((void*)Keys[1], (void*)Keys[0], sizeof(Keys[0]));
#ifdef VI_SDL2
			int Id = SDL_GetWindowID(Handle);
			Command = (int)SDL_GetModState();
			GetInputState();
			Message.Dispatch();

			SDL_Event Event;
			int HasEvents = 0;
			if (TimeoutMs > 0)
				HasEvents = SDL_WaitEventTimeout(&Event, (int)TimeoutMs);
			else
				HasEvents = SDL_PollEvent(&Event);

			while (HasEvents)
			{
				switch (Event.type)
				{
					case SDL_WINDOWEVENT:
						switch (Event.window.event)
						{
							case SDL_WINDOWEVENT_SHOWN:
								if (Callbacks.WindowStateChange && Id == Event.window.windowID)
									Callbacks.WindowStateChange(WindowState::Show, 0, 0);
								break;
							case SDL_WINDOWEVENT_HIDDEN:
								if (Callbacks.WindowStateChange && Id == Event.window.windowID)
									Callbacks.WindowStateChange(WindowState::Hide, 0, 0);
								break;
							case SDL_WINDOWEVENT_EXPOSED:
								if (Callbacks.WindowStateChange && Id == Event.window.windowID)
									Callbacks.WindowStateChange(WindowState::Expose, 0, 0);
								break;
							case SDL_WINDOWEVENT_MOVED:
								if (Callbacks.WindowStateChange && Id == Event.window.windowID)
									Callbacks.WindowStateChange(WindowState::Move, Event.window.data1, Event.window.data2);
								break;
							case SDL_WINDOWEVENT_RESIZED:
								if (Callbacks.WindowStateChange && Id == Event.window.windowID)
									Callbacks.WindowStateChange(WindowState::Resize, Event.window.data1, Event.window.data2);
								break;
							case SDL_WINDOWEVENT_SIZE_CHANGED:
								if (Callbacks.WindowStateChange && Id == Event.window.windowID)
									Callbacks.WindowStateChange(WindowState::Size_Change, Event.window.data1, Event.window.data2);
								break;
							case SDL_WINDOWEVENT_MINIMIZED:
								if (Callbacks.WindowStateChange && Id == Event.window.windowID)
									Callbacks.WindowStateChange(WindowState::Minimize, 0, 0);
								break;
							case SDL_WINDOWEVENT_MAXIMIZED:
								if (Callbacks.WindowStateChange && Id == Event.window.windowID)
									Callbacks.WindowStateChange(WindowState::Maximize, 0, 0);
								break;
							case SDL_WINDOWEVENT_RESTORED:
								if (Callbacks.WindowStateChange && Id == Event.window.windowID)
									Callbacks.WindowStateChange(WindowState::Restore, 0, 0);
								break;
							case SDL_WINDOWEVENT_ENTER:
								if (Callbacks.WindowStateChange && Id == Event.window.windowID)
									Callbacks.WindowStateChange(WindowState::Enter, 0, 0);
								break;
							case SDL_WINDOWEVENT_LEAVE:
								if (Callbacks.WindowStateChange && Id == Event.window.windowID)
									Callbacks.WindowStateChange(WindowState::Leave, 0, 0);
								break;
#if SDL_VERSION_ATLEAST(2, 0, 5)
							case SDL_WINDOWEVENT_TAKE_FOCUS:
#endif
							case SDL_WINDOWEVENT_FOCUS_GAINED:
								if (Callbacks.WindowStateChange && Id == Event.window.windowID)
									Callbacks.WindowStateChange(WindowState::Focus, 0, 0);
								break;
							case SDL_WINDOWEVENT_FOCUS_LOST:
								if (Callbacks.WindowStateChange && Id == Event.window.windowID)
									Callbacks.WindowStateChange(WindowState::Blur, 0, 0);
								break;
							case SDL_WINDOWEVENT_CLOSE:
								if (Callbacks.WindowStateChange && Id == Event.window.windowID)
									Callbacks.WindowStateChange(WindowState::Close, 0, 0);
								break;
						}
						break;
					case SDL_QUIT:
						if (Callbacks.AppStateChange)
							Callbacks.AppStateChange(AppState::Close_Window);
						break;
					case SDL_APP_TERMINATING:
						if (Callbacks.AppStateChange)
							Callbacks.AppStateChange(AppState::Terminating);
						break;
					case SDL_APP_LOWMEMORY:
						if (Callbacks.AppStateChange)
							Callbacks.AppStateChange(AppState::Low_Memory);
						break;
					case SDL_APP_WILLENTERBACKGROUND:
						if (Callbacks.AppStateChange)
							Callbacks.AppStateChange(AppState::Enter_Background_Start);
						break;
					case SDL_APP_DIDENTERBACKGROUND:
						if (Callbacks.AppStateChange)
							Callbacks.AppStateChange(AppState::Enter_Background_End);
						break;
					case SDL_APP_WILLENTERFOREGROUND:
						if (Callbacks.AppStateChange)
							Callbacks.AppStateChange(AppState::Enter_Foreground_Start);
						break;
					case SDL_APP_DIDENTERFOREGROUND:
						if (Callbacks.AppStateChange)
							Callbacks.AppStateChange(AppState::Enter_Foreground_End);
						break;
					case SDL_KEYDOWN:
						if (Callbacks.KeyState && Id == Event.window.windowID)
							Callbacks.KeyState((KeyCode)Event.key.keysym.scancode, (KeyMod)Event.key.keysym.mod, (int)Event.key.keysym.sym, (int)(Event.key.repeat != 0), true);

						if (Mapping.Enabled && !Mapping.Mapped)
						{
							Mapping.Key.Key = (KeyCode)Event.key.keysym.scancode;
							Mapping.Mapped = true;
							Mapping.Captured = false;
						}
						break;
					case SDL_KEYUP:
						if (Callbacks.KeyState && Id == Event.window.windowID)
							Callbacks.KeyState((KeyCode)Event.key.keysym.scancode, (KeyMod)Event.key.keysym.mod, (int)Event.key.keysym.sym, (int)(Event.key.repeat != 0), false);

						if (Mapping.Enabled && Mapping.Mapped && Mapping.Key.Key == (KeyCode)Event.key.keysym.scancode)
						{
							Mapping.Key.Mod = (KeyMod)SDL_GetModState();
							Mapping.Captured = true;
						}
						break;
					case SDL_TEXTINPUT:
						if (Callbacks.Input && Id == Event.window.windowID)
							Callbacks.Input((char*)Event.text.text, (int)strlen(Event.text.text));
						break;
					case SDL_TEXTEDITING:
						if (Callbacks.InputEdit && Id == Event.window.windowID)
							Callbacks.InputEdit((char*)Event.edit.text, (int)Event.edit.start, (int)Event.edit.length);
						break;
					case SDL_MOUSEMOTION:
						if (Id == Event.window.windowID)
						{
							CX = Event.motion.x;
							CY = Event.motion.y;
							if (Callbacks.CursorMove)
								Callbacks.CursorMove(CX, CY, (int)Event.motion.xrel, (int)Event.motion.yrel);
						}
						break;
					case SDL_MOUSEBUTTONDOWN:
						switch (Event.button.button)
						{
							case SDL_BUTTON_LEFT:
								if (Callbacks.KeyState && Id == Event.window.windowID)
									Callbacks.KeyState(KeyCode::CursorLeft, (KeyMod)SDL_GetModState(), (int)KeyCode::CursorLeft, (int)Event.button.clicks, true);

								if (Mapping.Enabled && !Mapping.Mapped)
								{
									Mapping.Key.Key = KeyCode::CursorLeft;
									Mapping.Mapped = true;
									Mapping.Captured = false;
								}
								break;
							case SDL_BUTTON_MIDDLE:
								if (Callbacks.KeyState && Id == Event.window.windowID)
									Callbacks.KeyState(KeyCode::CursorMiddle, (KeyMod)SDL_GetModState(), (int)KeyCode::CursorMiddle, (int)Event.button.clicks, true);

								if (Mapping.Enabled && !Mapping.Mapped)
								{
									Mapping.Key.Key = KeyCode::CursorMiddle;
									Mapping.Mapped = true;
									Mapping.Captured = false;
								}
								break;
							case SDL_BUTTON_RIGHT:
								if (Callbacks.KeyState && Id == Event.window.windowID)
									Callbacks.KeyState(KeyCode::CursorRight, (KeyMod)SDL_GetModState(), (int)KeyCode::CursorRight, (int)Event.button.clicks, true);

								if (Mapping.Enabled && !Mapping.Mapped)
								{
									Mapping.Key.Key = KeyCode::CursorRight;
									Mapping.Mapped = true;
									Mapping.Captured = false;
								}
								break;
							case SDL_BUTTON_X1:
								if (Callbacks.KeyState && Id == Event.window.windowID)
									Callbacks.KeyState(KeyCode::CursorX1, (KeyMod)SDL_GetModState(), (int)KeyCode::CursorX1, (int)Event.button.clicks, true);

								if (Mapping.Enabled && !Mapping.Mapped)
								{
									Mapping.Key.Key = KeyCode::CursorX1;
									Mapping.Mapped = true;
									Mapping.Captured = false;
								}
								break;
							case SDL_BUTTON_X2:
								if (Callbacks.KeyState && Id == Event.window.windowID)
									Callbacks.KeyState(KeyCode::CursorX2, (KeyMod)SDL_GetModState(), (int)KeyCode::CursorX2, (int)Event.button.clicks, true);

								if (Mapping.Enabled && !Mapping.Mapped)
								{
									Mapping.Key.Key = KeyCode::CursorX2;
									Mapping.Mapped = true;
									Mapping.Captured = false;
								}
								break;
						}
						break;
					case SDL_MOUSEBUTTONUP:
						switch (Event.button.button)
						{
							case SDL_BUTTON_LEFT:
								if (Callbacks.KeyState && Id == Event.window.windowID)
									Callbacks.KeyState(KeyCode::CursorLeft, (KeyMod)SDL_GetModState(), (int)KeyCode::CursorLeft, (int)Event.button.clicks, false);

								if (Mapping.Enabled && Mapping.Mapped && Mapping.Key.Key == KeyCode::CursorLeft)
								{
									Mapping.Key.Mod = (KeyMod)SDL_GetModState();
									Mapping.Captured = true;
								}
								break;
							case SDL_BUTTON_MIDDLE:
								if (Callbacks.KeyState && Id == Event.window.windowID)
									Callbacks.KeyState(KeyCode::CursorMiddle, (KeyMod)SDL_GetModState(), (int)KeyCode::CursorMiddle, (int)Event.button.clicks, false);

								if (Mapping.Enabled && Mapping.Mapped && Mapping.Key.Key == KeyCode::CursorMiddle)
								{
									Mapping.Key.Mod = (KeyMod)SDL_GetModState();
									Mapping.Captured = true;
								}
								break;
							case SDL_BUTTON_RIGHT:
								if (Callbacks.KeyState && Id == Event.window.windowID)
									Callbacks.KeyState(KeyCode::CursorRight, (KeyMod)SDL_GetModState(), (int)KeyCode::CursorRight, (int)Event.button.clicks, false);

								if (Mapping.Enabled && Mapping.Mapped && Mapping.Key.Key == KeyCode::CursorRight)
								{
									Mapping.Key.Mod = (KeyMod)SDL_GetModState();
									Mapping.Captured = true;
								}
								break;
							case SDL_BUTTON_X1:
								if (Callbacks.KeyState && Id == Event.window.windowID)
									Callbacks.KeyState(KeyCode::CursorX1, (KeyMod)SDL_GetModState(), (int)KeyCode::CursorX1, (int)Event.button.clicks, false);

								if (Mapping.Enabled && Mapping.Mapped && Mapping.Key.Key == KeyCode::CursorX1)
								{
									Mapping.Key.Mod = (KeyMod)SDL_GetModState();
									Mapping.Captured = true;
								}
								break;
							case SDL_BUTTON_X2:
								if (Callbacks.KeyState && Id == Event.window.windowID)
									Callbacks.KeyState(KeyCode::CursorX2, (KeyMod)SDL_GetModState(), (int)KeyCode::CursorX2, (int)Event.button.clicks, false);

								if (Mapping.Enabled && Mapping.Mapped && Mapping.Key.Key == KeyCode::CursorX2)
								{
									Mapping.Key.Mod = (KeyMod)SDL_GetModState();
									Mapping.Captured = true;
								}
								break;
						}
						break;
					case SDL_MOUSEWHEEL:
#if SDL_VERSION_ATLEAST(2, 0, 4)
						if (Callbacks.CursorWheelState && Id == Event.window.windowID)
							Callbacks.CursorWheelState((int)Event.wheel.x, (int)Event.wheel.y, Event.wheel.direction == SDL_MOUSEWHEEL_NORMAL);
#else
						if (Callbacks.CursorWheelState && Id == Event.window.windowID)
							Callbacks.CursorWheelState((int)Event.wheel.x, (int)Event.wheel.y, 1);
#endif
						break;
					case SDL_JOYAXISMOTION:
						if (Callbacks.JoyStickAxisMove && Id == Event.window.windowID)
							Callbacks.JoyStickAxisMove((int)Event.jaxis.which, (int)Event.jaxis.axis, (int)Event.jaxis.value);
						break;
					case SDL_JOYBALLMOTION:
						if (Callbacks.JoyStickBallMove && Id == Event.window.windowID)
							Callbacks.JoyStickBallMove((int)Event.jball.which, (int)Event.jball.ball, (int)Event.jball.xrel, (int)Event.jball.yrel);
						break;
					case SDL_JOYHATMOTION:
						if (Callbacks.JoyStickHatMove && Id == Event.window.windowID)
						{
							switch (Event.jhat.value)
							{
								case SDL_HAT_CENTERED:
									Callbacks.JoyStickHatMove(JoyStickHat::Center, (int)Event.jhat.which, (int)Event.jhat.hat);
									break;
								case SDL_HAT_UP:
									Callbacks.JoyStickHatMove(JoyStickHat::Up, (int)Event.jhat.which, (int)Event.jhat.hat);
									break;
								case SDL_HAT_DOWN:
									Callbacks.JoyStickHatMove(JoyStickHat::Down, (int)Event.jhat.which, (int)Event.jhat.hat);
									break;
								case SDL_HAT_LEFT:
									Callbacks.JoyStickHatMove(JoyStickHat::Left, (int)Event.jhat.which, (int)Event.jhat.hat);
									break;
								case SDL_HAT_LEFTUP:
									Callbacks.JoyStickHatMove(JoyStickHat::Left_Up, (int)Event.jhat.which, (int)Event.jhat.hat);
									break;
								case SDL_HAT_LEFTDOWN:
									Callbacks.JoyStickHatMove(JoyStickHat::Left_Down, (int)Event.jhat.which, (int)Event.jhat.hat);
									break;
								case SDL_HAT_RIGHT:
									Callbacks.JoyStickHatMove(JoyStickHat::Right, (int)Event.jhat.which, (int)Event.jhat.hat);
									break;
								case SDL_HAT_RIGHTUP:
									Callbacks.JoyStickHatMove(JoyStickHat::Right_Up, (int)Event.jhat.which, (int)Event.jhat.hat);
									break;
								case SDL_HAT_RIGHTDOWN:
									Callbacks.JoyStickHatMove(JoyStickHat::Right_Down, (int)Event.jhat.which, (int)Event.jhat.hat);
									break;
							}
						}
						break;
					case SDL_JOYBUTTONDOWN:
						if (Callbacks.JoyStickKeyState && Id == Event.window.windowID)
							Callbacks.JoyStickKeyState((int)Event.jbutton.which, (int)Event.jbutton.button, true);
						break;
					case SDL_JOYBUTTONUP:
						if (Callbacks.JoyStickKeyState && Id == Event.window.windowID)
							Callbacks.JoyStickKeyState((int)Event.jbutton.which, (int)Event.jbutton.button, false);
						break;
					case SDL_JOYDEVICEADDED:
						if (Callbacks.JoyStickState && Id == Event.window.windowID)
							Callbacks.JoyStickState((int)Event.jdevice.which, true);
						break;
					case SDL_JOYDEVICEREMOVED:
						if (Callbacks.JoyStickState && Id == Event.window.windowID)
							Callbacks.JoyStickState((int)Event.jdevice.which, false);
						break;
					case SDL_CONTROLLERAXISMOTION:
						if (Callbacks.ControllerAxisMove && Id == Event.window.windowID)
							Callbacks.ControllerAxisMove((int)Event.caxis.which, (int)Event.caxis.axis, (int)Event.caxis.value);
						break;
					case SDL_CONTROLLERBUTTONDOWN:
						if (Callbacks.ControllerKeyState && Id == Event.window.windowID)
							Callbacks.ControllerKeyState((int)Event.cbutton.which, (int)Event.cbutton.button, true);
						break;
					case SDL_CONTROLLERBUTTONUP:
						if (Callbacks.ControllerKeyState && Id == Event.window.windowID)
							Callbacks.ControllerKeyState((int)Event.cbutton.which, (int)Event.cbutton.button, false);
						break;
					case SDL_CONTROLLERDEVICEADDED:
						if (Callbacks.ControllerState && Id == Event.window.windowID)
							Callbacks.ControllerState((int)Event.cdevice.which, 1);
						break;
					case SDL_CONTROLLERDEVICEREMOVED:
						if (Callbacks.ControllerState && Id == Event.window.windowID)
							Callbacks.ControllerState((int)Event.cdevice.which, -1);
						break;
					case SDL_CONTROLLERDEVICEREMAPPED:
						if (Callbacks.ControllerState && Id == Event.window.windowID)
							Callbacks.ControllerState((int)Event.cdevice.which, 0);
						break;
					case SDL_FINGERMOTION:
						if (Callbacks.TouchMove && Id == Event.window.windowID)
							Callbacks.TouchMove((int)Event.tfinger.touchId, (int)Event.tfinger.fingerId, Event.tfinger.x, Event.tfinger.y, Event.tfinger.dx, Event.tfinger.dy, Event.tfinger.pressure);
						break;
					case SDL_FINGERDOWN:
						if (Callbacks.TouchState && Id == Event.window.windowID)
							Callbacks.TouchState((int)Event.tfinger.touchId, (int)Event.tfinger.fingerId, Event.tfinger.x, Event.tfinger.y, Event.tfinger.dx, Event.tfinger.dy, Event.tfinger.pressure, true);
						break;
					case SDL_FINGERUP:
						if (Callbacks.TouchState && Id == Event.window.windowID)
							Callbacks.TouchState((int)Event.tfinger.touchId, (int)Event.tfinger.fingerId, Event.tfinger.x, Event.tfinger.y, Event.tfinger.dx, Event.tfinger.dy, Event.tfinger.pressure, false);
						break;
					case SDL_DOLLARGESTURE:
						if (Callbacks.GestureState && Id == Event.window.windowID)
							Callbacks.GestureState((int)Event.dgesture.touchId, (int)Event.dgesture.gestureId, (int)Event.dgesture.numFingers, Event.dgesture.x, Event.dgesture.y, Event.dgesture.error, false);
						break;
					case SDL_DOLLARRECORD:
						if (Callbacks.GestureState && Id == Event.window.windowID)
							Callbacks.GestureState((int)Event.dgesture.touchId, (int)Event.dgesture.gestureId, (int)Event.dgesture.numFingers, Event.dgesture.x, Event.dgesture.y, Event.dgesture.error, true);
						break;
					case SDL_MULTIGESTURE:
						if (Callbacks.MultiGestureState && Id == Event.window.windowID)
							Callbacks.MultiGestureState((int)Event.mgesture.touchId, (int)Event.mgesture.numFingers, Event.mgesture.x, Event.mgesture.y, Event.mgesture.dDist, Event.mgesture.dTheta);
						break;
#if SDL_VERSION_ATLEAST(2, 0, 5)
					case SDL_DROPFILE:
						if (Id != Event.window.windowID)
							break;

						if (Callbacks.DropFile)
							Callbacks.DropFile(Event.drop.file);

						SDL_free(Event.drop.file);
						break;
					case SDL_DROPTEXT:
						if (Id != Event.window.windowID)
							break;

						if (Callbacks.DropText)
							Callbacks.DropText(Event.drop.file);

						SDL_free(Event.drop.file);
						break;
#endif
					default:
						break;
				}

				HasEvents = SDL_PollEvent(&Event);
			}

			if (TimeoutMs > 0 || Options.RenderEvenIfInactive)
				return true;

			Uint32 Flags = SDL_GetWindowFlags(Handle);
			if (Flags & SDL_WINDOW_MAXIMIZED || Flags & SDL_WINDOW_INPUT_FOCUS || Flags & SDL_WINDOW_MOUSE_FOCUS)
				return true;

			std::this_thread::sleep_for(std::chrono::milliseconds(Options.InactiveSleepMs));
			return false;
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
		bool Activity::IsFullscreen() const
		{
#ifdef VI_SDL2
			VI_ASSERT(Handle != nullptr, "activity should be initialized");
			Uint32 Flags = SDL_GetWindowFlags(Handle);
			return Flags & SDL_WINDOW_FULLSCREEN || Flags & SDL_WINDOW_FULLSCREEN_DESKTOP;
#else
			return false;
#endif
		}
		bool Activity::IsAnyKeyDown() const
		{
			for (int i = 0; i < sizeof(Keys[0]) / sizeof(bool); i++)
			{
				if (Keys[0][i])
					return true;
			}

			return false;
		}
		bool Activity::IsKeyDown(const KeyMap& Key) const
		{
#ifdef VI_SDL2
			if (Key.Mod == KeyMod::None)
				return Keys[0][(size_t)Key.Key];

			if (Key.Key == KeyCode::None)
				return Command & (int)Key.Mod;

			return Command & (int)Key.Mod && Keys[0][(size_t)Key.Key];
#else
			return Keys[0][(size_t)Key.Key];
#endif
		}
		bool Activity::IsKeyUp(const KeyMap& Key) const
		{
			return !IsKeyDown(Key);
		}
		bool Activity::IsKeyDownHit(const KeyMap& Key) const
		{
#ifdef VI_SDL2
			if (Key.Mod == KeyMod::None)
				return Keys[0][(size_t)Key.Key] && !Keys[1][(size_t)Key.Key];

			if (Key.Key == KeyCode::None)
				return Command & (int)Key.Mod;

			return (Command & (int)Key.Mod) && Keys[0][(size_t)Key.Key] && !Keys[1][(size_t)Key.Key];
#else
			return Keys[0][(size_t)Key.Key] && !Keys[1][(size_t)Key.Key];
#endif
		}
		bool Activity::IsKeyUpHit(const KeyMap& Key) const
		{
#ifdef VI_SDL2
			if (Key.Mod == KeyMod::None)
				return !Keys[0][(size_t)Key.Key] && Keys[1][(size_t)Key.Key];

			if (Key.Key == KeyCode::None)
				return !(Command & (int)Key.Mod);

			return !(Command & (int)Key.Mod) && !Keys[0][(size_t)Key.Key] && Keys[1][(size_t)Key.Key];
#else
			return !Keys[0][(size_t)Key.Key] && Keys[1][(size_t)Key.Key];
#endif
		}
		bool Activity::IsScreenKeyboardEnabled() const
		{
#ifdef VI_SDL2
			VI_ASSERT(Handle != nullptr, "activity should be initialized");
			return SDL_IsScreenKeyboardShown(Handle);
#else
			return false;
#endif
		}
		uint32_t Activity::GetX() const
		{
#ifdef VI_SDL2
			VI_ASSERT(Handle != nullptr, "activity should be initialized");

			int X, Y;
			SDL_GetWindowPosition(Handle, &X, &Y);
			return X;
#else
			return 0;
#endif
		}
		uint32_t Activity::GetY() const
		{
#ifdef VI_SDL2
			VI_ASSERT(Handle != nullptr, "activity should be initialized");

			int X, Y;
			SDL_GetWindowPosition(Handle, &X, &Y);
			return Y;
#else
			return 0;
#endif
		}
		uint32_t Activity::GetWidth() const
		{
#ifdef VI_SDL2
			VI_ASSERT(Handle != nullptr, "activity should be initialized");

			int W, H;
			SDL_GetWindowSize(Handle, &W, &H);
			return W;
#else
			return 0;
#endif
		}
		uint32_t Activity::GetHeight() const
		{
#ifdef VI_SDL2
			VI_ASSERT(Handle != nullptr, "activity should be initialized");

			int W, H;
			SDL_GetWindowSize(Handle, &W, &H);
			return H;
#else
			return 0;
#endif
		}
		float Activity::GetAspectRatio() const
		{
#ifdef VI_SDL2
			VI_ASSERT(Handle != nullptr, "activity should be initialized");

			int W, H;
			SDL_GetWindowSize(Handle, &W, &H);
			return (H > 0 ? (float)W / (float)H : 0.0f);
#else
			return 0.0f;
#endif
		}
		KeyMod Activity::GetKeyModState() const
		{
#ifdef VI_SDL2
			VI_ASSERT(Handle != nullptr, "activity should be initialized");
			return (KeyMod)SDL_GetModState();
#else
			return KeyMod::None;
#endif
		}
		Viewport Activity::GetViewport() const
		{
#ifdef VI_SDL2
			VI_ASSERT(Handle != nullptr, "activity should be initialized");

			int W, H;
			SDL_GetWindowSize(Handle, &W, &H);

			Viewport Id;
			Id.Width = (float)W;
			Id.Height = (float)H;
			Id.MinDepth = 0.0f;
			Id.MaxDepth = 1.0f;
			Id.TopLeftX = 0.0f;
			Id.TopLeftY = 0.0f;
			return Id;
#else
			return Viewport();
#endif
		}
		Compute::Vector2 Activity::GetSize() const
		{
#ifdef VI_SDL2
			VI_ASSERT(Handle != nullptr, "activity should be initialized");

			int W, H;
			SDL_GL_GetDrawableSize(Handle, &W, &H);
			return Compute::Vector2((float)W, (float)H);
#else
			return Compute::Vector2();
#endif
		}
		Compute::Vector2 Activity::GetClientSize() const
		{
#ifdef VI_SDL2
			VI_ASSERT(Handle != nullptr, "activity should be initialized");

			int W, H;
			SDL_GetWindowSize(Handle, &W, &H);
			return Compute::Vector2((float)W, (float)H);
#else
			return Compute::Vector2();
#endif
		}
		Compute::Vector2 Activity::GetOffset() const
		{
#ifdef VI_SDL2
			VI_ASSERT(Handle != nullptr, "activity should be initialized");

			SDL_DisplayMode Display;
			SDL_GetCurrentDisplayMode(0, &Display);
			Compute::Vector2 Size = GetSize();
			return Compute::Vector2((float)Display.w / Size.X, (float)Display.h / Size.Y);
#else
			return Compute::Vector2();
#endif
		}
		Compute::Vector2 Activity::GetGlobalCursorPosition() const
		{
#ifdef VI_SDL2
			VI_ASSERT(Handle != nullptr, "activity should be initialized");
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
		Compute::Vector2 Activity::GetCursorPosition() const
		{
#ifdef VI_SDL2
			VI_ASSERT(Handle != nullptr, "activity should be initialized");
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
		Compute::Vector2 Activity::GetCursorPosition(float ScreenWidth, float ScreenHeight) const
		{
#ifdef VI_SDL2
			Compute::Vector2 Size = GetSize();
			return GetCursorPosition() * Compute::Vector2(ScreenWidth, ScreenHeight) / Size;
#else
			return Compute::Vector2();
#endif
		}
		Compute::Vector2 Activity::GetCursorPosition(const Compute::Vector2& ScreenDimensions) const
		{
#ifdef VI_SDL2
			Compute::Vector2 Size = GetSize();
			return GetCursorPosition() * ScreenDimensions / Size;
#else
			return Compute::Vector2();
#endif
		}
		Core::String Activity::GetClipboardText() const
		{
#ifdef VI_SDL2
			VI_ASSERT(Handle != nullptr, "activity should be initialized");
			char* Text = SDL_GetClipboardText();
			Core::String Result = (Text ? Text : "");

			if (Text != nullptr)
				SDL_free(Text);

			return Result;
#else
			return Core::String();
#endif
		}
		SDL_Window* Activity::GetHandle() const
		{
			return Handle;
		}
		Core::String Activity::GetError() const
		{
#ifdef VI_SDL2
			VI_ASSERT(Handle != nullptr, "activity should be initialized");
			const char* Error = SDL_GetError();
			if (!Error)
				return "";

			return Error;
#else
			return "";
#endif
		}
		Activity::Desc& Activity::GetOptions()
		{
			return Options;
		}
		bool* Activity::GetInputState()
		{
#ifdef VI_SDL2
			int Count;
			auto* Map = SDL_GetKeyboardState(&Count);
			if (Count > sizeof(Keys[0]) / sizeof(bool))
				Count = sizeof(Keys[0]) / sizeof(bool);

			for (int i = 0; i < Count; i++)
				Keys[0][i] = Map[i] > 0;

			Uint32 State = SDL_GetMouseState(nullptr, nullptr);
			Keys[0][(size_t)KeyCode::CursorLeft] = (State & SDL_BUTTON(SDL_BUTTON_LEFT));
			Keys[0][(size_t)KeyCode::CursorMiddle] = (State & SDL_BUTTON(SDL_BUTTON_MIDDLE));
			Keys[0][(size_t)KeyCode::CursorRight] = (State & SDL_BUTTON(SDL_BUTTON_RIGHT));
			Keys[0][(size_t)KeyCode::CursorX1] = (State & SDL_BUTTON(SDL_BUTTON_X1));
			Keys[0][(size_t)KeyCode::CursorX2] = (State & SDL_BUTTON(SDL_BUTTON_X2));
#endif
			return Keys[0];
		}

		void* Video::Windows::GetHDC(Activity* Target)
		{
			VI_ASSERT(Target != nullptr, "activity should be set");
#ifdef SDL_VIDEO_DRIVER_WINDOWS
			SDL_SysWMinfo Info;
			Target->Load(&Info);
			return Info.info.win.hdc;
#else
			return nullptr;
#endif
		}
		void* Video::Windows::GetHINSTANCE(Activity* Target)
		{
			VI_ASSERT(Target != nullptr, "activity should be set");
#ifdef SDL_VIDEO_DRIVER_WINDOWS
			SDL_SysWMinfo Info;
			Target->Load(&Info);
			return Info.info.win.hinstance;
#else
			return nullptr;
#endif
		}
		void* Video::Windows::GetHWND(Activity* Target)
		{
			VI_ASSERT(Target != nullptr, "activity should be set");
#ifdef SDL_VIDEO_DRIVER_WINDOWS
			SDL_SysWMinfo Info;
			Target->Load(&Info);
			return Info.info.win.window;
#else
			return nullptr;
#endif
		}
		void* Video::WinRT::GetIInspectable(Activity* Target)
		{
			VI_ASSERT(Target != nullptr, "activity should be set");
#ifdef SDL_VIDEO_DRIVER_WINRT
			SDL_SysWMinfo Info;
			Target->Load(&Info);
			return Info.info.winrt.window;
#else
			return nullptr;
#endif
		}
		void* Video::X11::GetDisplay(Activity* Target)
		{
			VI_ASSERT(Target != nullptr, "activity should be set");
#ifdef SDL_VIDEO_DRIVER_X11
			SDL_SysWMinfo Info;
			Target->Load(&Info);
			return Info.info.x11.display;
#else
			return nullptr;
#endif
		}
		size_t Video::X11::GetWindow(Activity* Target)
		{
			VI_ASSERT(Target != nullptr, "activity should be set");
#ifdef SDL_VIDEO_DRIVER_X11
			SDL_SysWMinfo Info;
			Target->Load(&Info);
			return (size_t)Info.info.x11.window;
#else
			return 0;
#endif
		}
		void* Video::DirectFB::GetIDirectFB(Activity* Target)
		{
			VI_ASSERT(Target != nullptr, "activity should be set");
#ifdef SDL_VIDEO_DRIVER_DIRECTFB
			SDL_SysWMinfo Info;
			Target->Load(&Info);
			return (size_t)Info.info.dfb.dfb;
#else
			return nullptr;
#endif
		}
		void* Video::DirectFB::GetIDirectFBWindow(Activity* Target)
		{
			VI_ASSERT(Target != nullptr, "activity should be set");
#ifdef SDL_VIDEO_DRIVER_DIRECTFB
			SDL_SysWMinfo Info;
			Target->Load(&Info);
			return (size_t)Info.info.dfb.window;
#else
			return nullptr;
#endif
		}
		void* Video::DirectFB::GetIDirectFBSurface(Activity* Target)
		{
			VI_ASSERT(Target != nullptr, "activity should be set");
#ifdef SDL_VIDEO_DRIVER_DIRECTFB
			SDL_SysWMinfo Info;
			Target->Load(&Info);
			return (size_t)Info.info.dfb.surface;
#else
			return nullptr;
#endif
		}
		void* Video::Cocoa::GetNSWindow(Activity* Target)
		{
			VI_ASSERT(Target != nullptr, "activity should be set");
#ifdef SDL_VIDEO_DRIVER_COCOA
			SDL_SysWMinfo Info;
			Target->Load(&Info);
			return (size_t)Info.info.cocoa.window;
#else
			return nullptr;
#endif
		}
		void* Video::UIKit::GetUIWindow(Activity* Target)
		{
			VI_ASSERT(Target != nullptr, "activity should be set");
#ifdef SDL_VIDEO_DRIVER_UIKIT
			SDL_SysWMinfo Info;
			Target->Load(&Info);
			return (size_t)Info.info.uikit.window;
#else
			return nullptr;
#endif
		}
		void* Video::Wayland::GetWlDisplay(Activity* Target)
		{
			VI_ASSERT(Target != nullptr, "activity should be set");
#ifdef SDL_VIDEO_DRIVER_UIKIT
			SDL_SysWMinfo Info;
			Target->Load(&Info);
			return (size_t)Info.info.wl.display;
#else
			return nullptr;
#endif
		}
		void* Video::Wayland::GetWlSurface(Activity* Target)
		{
			VI_ASSERT(Target != nullptr, "activity should be set");
#ifdef SDL_VIDEO_DRIVER_UIKIT
			SDL_SysWMinfo Info;
			Target->Load(&Info);
			return (size_t)Info.info.wl.surface;
#else
			return nullptr;
#endif
		}
		void* Video::Wayland::GetWlEglWindow(Activity* Target)
		{
			VI_ASSERT(Target != nullptr, "activity should be set");
#ifdef SDL_VIDEO_DRIVER_UIKIT
			SDL_SysWMinfo Info;
			Target->Load(&Info);
			return (size_t)Info.info.wl.egl_window;
#else
			return nullptr;
#endif
		}
		void* Video::Wayland::GetXdgSurface(Activity* Target)
		{
			VI_ASSERT(Target != nullptr, "activity should be set");
#ifdef SDL_VIDEO_DRIVER_UIKIT
			SDL_SysWMinfo Info;
			Target->Load(&Info);
			return (size_t)Info.info.wl.xdg_surface;
#else
			return nullptr;
#endif
		}
		void* Video::Wayland::GetXdgTopLevel(Activity* Target)
		{
			VI_ASSERT(Target != nullptr, "activity should be set");
#ifdef SDL_VIDEO_DRIVER_UIKIT
			SDL_SysWMinfo Info;
			Target->Load(&Info);
			return (size_t)Info.info.wl.xdg_toplevel;
#else
			return nullptr;
#endif
		}
		void* Video::Wayland::GetXdgPopup(Activity* Target)
		{
			VI_ASSERT(Target != nullptr, "activity should be set");
#ifdef SDL_VIDEO_DRIVER_UIKIT
			SDL_SysWMinfo Info;
			Target->Load(&Info);
			return (size_t)Info.info.wl.xdg_popup;
#else
			return nullptr;
#endif
		}
		void* Video::Wayland::GetXdgPositioner(Activity* Target)
		{
			VI_ASSERT(Target != nullptr, "activity should be set");
#ifdef SDL_VIDEO_DRIVER_UIKIT
			SDL_SysWMinfo Info;
			Target->Load(&Info);
			return (size_t)Info.info.wl.xdg_positioner;
#else
			return nullptr;
#endif
		}
		void* Video::Android::GetANativeWindow(Activity* Target)
		{
			VI_ASSERT(Target != nullptr, "activity should be set");
#ifdef SDL_VIDEO_DRIVER_ANDROID
			SDL_SysWMinfo Info;
			Target->Load(&Info);
			return (size_t)Info.info.android.window;
#else
			return nullptr;
#endif
		}
		void* Video::OS2::GetHWND(Activity* Target)
		{
			VI_ASSERT(Target != nullptr, "activity should be set");
#ifdef SDL_VIDEO_DRIVER_OS2
			SDL_SysWMinfo Info;
			Target->Load(&Info);
			return (size_t)Info.info.os2.hwnd;
#else
			return nullptr;
#endif
		}
		void* Video::OS2::GetHWNDFrame(Activity* Target)
		{
			VI_ASSERT(Target != nullptr, "activity should be set");
#ifdef SDL_VIDEO_DRIVER_OS2
			SDL_SysWMinfo Info;
			Target->Load(&Info);
			return (size_t)Info.info.os2.hwndFrame;
#else
			return nullptr;
#endif
		}
		bool Video::GLEW::SetSwapInterval(int32_t Interval)
		{
#ifdef VI_SDL2
			return SDL_GL_SetSwapInterval(Interval) == 0;
#else
			return false;
#endif
		}
		bool Video::GLEW::SetSwapParameters(int32_t R, int32_t G, int32_t B, int32_t A, bool Debugging)
		{
#ifdef VI_SDL2
			SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
			SDL_GL_SetAttribute(SDL_GL_RED_SIZE, R);
			SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, G);
			SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, B);
			SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, A);
			SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_NO_ERROR, Debugging ? 0 : 1);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, Debugging ? SDL_GL_CONTEXT_DEBUG_FLAG : 0);
			return true;
#else
			return false;
#endif
		}
		bool Video::GLEW::SetContext(Activity* Target, void* Context)
		{
			VI_ASSERT(Target != nullptr, "activity should be set");
#ifdef VI_SDL2
			return SDL_GL_MakeCurrent(Target->GetHandle(), Context) == 0;
#else
			return false;
#endif
		}
		bool Video::GLEW::PerformSwap(Activity* Target)
		{
			VI_ASSERT(Target != nullptr, "activity should be set");
#ifdef VI_SDL2
			SDL_GL_SwapWindow(Target->GetHandle());
			return true;
#else
			return false;
#endif
		}
		void* Video::GLEW::CreateContext(Activity* Target)
		{
			VI_ASSERT(Target != nullptr, "activity should be set");
#ifdef VI_SDL2
			return SDL_GL_CreateContext(Target->GetHandle());
#else
			return nullptr;
#endif
		}
		void Video::GLEW::DestroyContext(void* Context)
		{
#ifdef VI_SDL2
			if (Context != nullptr)
				SDL_GL_DeleteContext(Context);
#endif
		}
		uint32_t Video::GetDisplayCount()
		{
#ifdef VI_SDL2
			int Displays = SDL_GetNumVideoDisplays();
			return Displays >= 1 ? (uint32_t)(Displays - 1) : 0;
#else
			return 0;
#endif
		}
		bool Video::GetDisplayInfo(uint32_t DisplayIndex, DisplayInfo* Info)
		{
#ifdef VI_SDL2
			SDL_DisplayMode Display;
			if (SDL_GetCurrentDisplayMode(DisplayIndex, &Display) != 0)
				return false;
			else if (!Info)
				return true;

			const char* Name = SDL_GetDisplayName(DisplayIndex);
			if (Name != nullptr)
				Info->Name = Name;

			SDL_Rect Bounds;
			if (!SDL_GetDisplayUsableBounds(DisplayIndex, &Bounds))
			{
				Info->X = Bounds.x;
				Info->Y = Bounds.y;
				Info->Width = Bounds.w;
				Info->Height = Bounds.h;
			}

			SDL_GetDisplayDPI(DisplayIndex, &Info->DiagonalDPI, &Info->HorizontalDPI, &Info->VerticalDPI);
			Info->Orientation = (OrientationType)SDL_GetDisplayOrientation(DisplayIndex);
			Info->PixelFormat = (uint32_t)Display.format;
			Info->PhysicalWidth = (uint32_t)Display.w;
			Info->PhysicalHeight = (uint32_t)Display.h;
			Info->RefreshRate = (uint32_t)Display.refresh_rate;
			return true;
#else
			return false;
#endif
		}
		const char* Video::GetKeyCodeAsLiteral(KeyCode Code)
		{
			const char* Name;
			switch (Code)
			{
				case KeyCode::G:
					Name = "G";
					break;
				case KeyCode::H:
					Name = "H";
					break;
				case KeyCode::I:
					Name = "I";
					break;
				case KeyCode::J:
					Name = "J";
					break;
				case KeyCode::K:
					Name = "K";
					break;
				case KeyCode::L:
					Name = "L";
					break;
				case KeyCode::M:
					Name = "M";
					break;
				case KeyCode::N:
					Name = "N";
					break;
				case KeyCode::O:
					Name = "O";
					break;
				case KeyCode::P:
					Name = "P";
					break;
				case KeyCode::Q:
					Name = "Q";
					break;
				case KeyCode::R:
					Name = "R";
					break;
				case KeyCode::S:
					Name = "S";
					break;
				case KeyCode::T:
					Name = "T";
					break;
				case KeyCode::U:
					Name = "U";
					break;
				case KeyCode::V:
					Name = "V";
					break;
				case KeyCode::W:
					Name = "W";
					break;
				case KeyCode::X:
					Name = "X";
					break;
				case KeyCode::Y:
					Name = "Y";
					break;
				case KeyCode::Z:
					Name = "Z";
					break;
				case KeyCode::Return:
					Name = "Return";
					break;
				case KeyCode::Escape:
					Name = "Escape";
					break;
				case KeyCode::LeftBracket:
					Name = "Left Bracket";
					break;
				case KeyCode::RightBracket:
					Name = "Right Bracket";
					break;
				case KeyCode::Backslash:
					Name = "Backslash";
					break;
				case KeyCode::NonUsHash:
					Name = "Non-US Hash";
					break;
				case KeyCode::Semicolon:
					Name = "Semicolon";
					break;
				case KeyCode::Apostrophe:
					Name = "Apostrophe";
					break;
				case KeyCode::Grave:
					Name = "Grave";
					break;
				case KeyCode::Slash:
					Name = "Slash";
					break;
				case KeyCode::Capslock:
					Name = "Caps Lock";
					break;
				case KeyCode::F1:
					Name = "F1";
					break;
				case KeyCode::F2:
					Name = "F2";
					break;
				case KeyCode::F3:
					Name = "F3";
					break;
				case KeyCode::F4:
					Name = "F4";
					break;
				case KeyCode::F5:
					Name = "F5";
					break;
				case KeyCode::F6:
					Name = "F6";
					break;
				case KeyCode::F7:
					Name = "F7";
					break;
				case KeyCode::F8:
					Name = "F8";
					break;
				case KeyCode::F9:
					Name = "F9";
					break;
				case KeyCode::F10:
					Name = "F10";
					break;
				case KeyCode::F11:
					Name = "F11";
					break;
				case KeyCode::F12:
					Name = "F12";
					break;
				case KeyCode::PrintScreen:
					Name = "Print Screen";
					break;
				case KeyCode::ScrollLock:
					Name = "Scroll Lock";
					break;
				case KeyCode::Pause:
					Name = "Pause";
					break;
				case KeyCode::Insert:
					Name = "Insert";
					break;
				case KeyCode::Home:
					Name = "Home";
					break;
				case KeyCode::PageUp:
					Name = "Page Up";
					break;
				case KeyCode::Delete:
					Name = "Delete";
					break;
				case KeyCode::End:
					Name = "End";
					break;
				case KeyCode::PageDown:
					Name = "Page Down";
					break;
				case KeyCode::Right:
					Name = "Right";
					break;
				case KeyCode::Left:
					Name = "Left";
					break;
				case KeyCode::Down:
					Name = "Down";
					break;
				case KeyCode::Up:
					Name = "Up";
					break;
				case KeyCode::NumLockClear:
					Name = "Numlock Clear";
					break;
				case KeyCode::KpDivide:
					Name = "Divide";
					break;
				case KeyCode::KpMultiply:
					Name = "Multiply";
					break;
				case KeyCode::Minus:
				case KeyCode::KpMinus:
					Name = "Minus";
					break;
				case KeyCode::KpPlus:
					Name = "Plus";
					break;
				case KeyCode::KpEnter:
					Name = "Enter";
					break;
				case KeyCode::D1:
				case KeyCode::Kp1:
					Name = "1";
					break;
				case KeyCode::D2:
				case KeyCode::Kp2:
					Name = "2";
					break;
				case KeyCode::D3:
				case KeyCode::Kp3:
					Name = "3";
					break;
				case KeyCode::D4:
				case KeyCode::Kp4:
					Name = "4";
					break;
				case KeyCode::D5:
				case KeyCode::Kp5:
					Name = "5";
					break;
				case KeyCode::D6:
				case KeyCode::Kp6:
					Name = "6";
					break;
				case KeyCode::D7:
				case KeyCode::Kp7:
					Name = "7";
					break;
				case KeyCode::D8:
				case KeyCode::Kp8:
					Name = "8";
					break;
				case KeyCode::D9:
				case KeyCode::Kp9:
					Name = "9";
					break;
				case KeyCode::D0:
				case KeyCode::Kp0:
					Name = "0";
					break;
				case KeyCode::Period:
				case KeyCode::KpPeriod:
					Name = "Period";
					break;
				case KeyCode::NonUsBackslash:
					Name = "Non-US Backslash";
					break;
				case KeyCode::App0:
					Name = "Application";
					break;
				case KeyCode::Equals:
				case KeyCode::KpEquals:
					Name = "Equals";
					break;
				case KeyCode::F13:
					Name = "F13";
					break;
				case KeyCode::F14:
					Name = "F14";
					break;
				case KeyCode::F15:
					Name = "F15";
					break;
				case KeyCode::F16:
					Name = "F16";
					break;
				case KeyCode::F17:
					Name = "F17";
					break;
				case KeyCode::F18:
					Name = "F18";
					break;
				case KeyCode::F19:
					Name = "F19";
					break;
				case KeyCode::F20:
					Name = "F20";
					break;
				case KeyCode::F21:
					Name = "F21";
					break;
				case KeyCode::F22:
					Name = "F22";
					break;
				case KeyCode::F23:
					Name = "F23";
					break;
				case KeyCode::F24:
					Name = "F24";
					break;
				case KeyCode::Execute:
					Name = "Execute";
					break;
				case KeyCode::Help:
					Name = "Help";
					break;
				case KeyCode::Menu:
					Name = "Menu";
					break;
				case KeyCode::Select:
					Name = "Select";
					break;
				case KeyCode::Stop:
					Name = "Stop";
					break;
				case KeyCode::Again:
					Name = "Again";
					break;
				case KeyCode::Undo:
					Name = "Undo";
					break;
				case KeyCode::Cut:
					Name = "Cut";
					break;
				case KeyCode::Copy:
					Name = "Copy";
					break;
				case KeyCode::Paste:
					Name = "Paste";
					break;
				case KeyCode::Find:
					Name = "Find";
					break;
				case KeyCode::Mute:
					Name = "Mute";
					break;
				case KeyCode::VolumeUp:
					Name = "Volume Up";
					break;
				case KeyCode::VolumeDown:
					Name = "Volume Down";
					break;
				case KeyCode::Comma:
				case KeyCode::KpComma:
					Name = "Comma";
					break;
				case KeyCode::KpEqualsAs400:
					Name = "Equals As 400";
					break;
				case KeyCode::International1:
					Name = "International 1";
					break;
				case KeyCode::International2:
					Name = "International 2";
					break;
				case KeyCode::International3:
					Name = "International 3";
					break;
				case KeyCode::International4:
					Name = "International 4";
					break;
				case KeyCode::International5:
					Name = "International 5";
					break;
				case KeyCode::International6:
					Name = "International 6";
					break;
				case KeyCode::International7:
					Name = "International 7";
					break;
				case KeyCode::International8:
					Name = "International 8";
					break;
				case KeyCode::International9:
					Name = "International 9";
					break;
				case KeyCode::Lang1:
					Name = "Lang 1";
					break;
				case KeyCode::Lang2:
					Name = "Lang 2";
					break;
				case KeyCode::Lang3:
					Name = "Lang 3";
					break;
				case KeyCode::Lang4:
					Name = "Lang 4";
					break;
				case KeyCode::Lang5:
					Name = "Lang 5";
					break;
				case KeyCode::Lang6:
					Name = "Lang 6";
					break;
				case KeyCode::Lang7:
					Name = "Lang 7";
					break;
				case KeyCode::Lang8:
					Name = "Lang 8";
					break;
				case KeyCode::Lang9:
					Name = "Lang 9";
					break;
				case KeyCode::Alterase:
					Name = "Alterase";
					break;
				case KeyCode::SysReq:
					Name = "System Request";
					break;
				case KeyCode::Cancel:
					Name = "Cancel";
					break;
				case KeyCode::Prior:
					Name = "Prior";
					break;
				case KeyCode::Return2:
					Name = "Return 2";
					break;
				case KeyCode::Separator:
					Name = "Separator";
					break;
				case KeyCode::Output:
					Name = "Output";
					break;
				case KeyCode::Operation:
					Name = "Operation";
					break;
				case KeyCode::ClearAgain:
					Name = "Clear Again";
					break;
				case KeyCode::CrSelect:
					Name = "CR Select";
					break;
				case KeyCode::ExSelect:
					Name = "EX Select";
					break;
				case KeyCode::Kp00:
					Name = "00";
					break;
				case KeyCode::Kp000:
					Name = "000";
					break;
				case KeyCode::ThousandsSeparator:
					Name = "Thousands Separator";
					break;
				case KeyCode::DecimalsSeparator:
					Name = "Decimal Separator";
					break;
				case KeyCode::CurrencyUnit:
					Name = "Currency Unit";
					break;
				case KeyCode::CurrencySubunit:
					Name = "Currency Subunit";
					break;
				case KeyCode::KpLeftParen:
					Name = "Left Parent";
					break;
				case KeyCode::KpRightParen:
					Name = "Right Parent";
					break;
				case KeyCode::KpLeftBrace:
					Name = "Left Brace";
					break;
				case KeyCode::KpRightBrace:
					Name = "Right Brace";
					break;
				case KeyCode::Tab:
				case KeyCode::KpTab:
					Name = "Tab";
					break;
				case KeyCode::Backspace:
				case KeyCode::KpBackspace:
					Name = "Backspace";
					break;
				case KeyCode::A:
				case KeyCode::KpA:
					Name = "A";
					break;
				case KeyCode::B:
				case KeyCode::KpB:
					Name = "B";
					break;
				case KeyCode::C:
				case KeyCode::KpC:
					Name = "C";
					break;
				case KeyCode::D:
				case KeyCode::KpD:
					Name = "D";
					break;
				case KeyCode::E:
				case KeyCode::KpE:
					Name = "E";
					break;
				case KeyCode::F:
				case KeyCode::KpF:
					Name = "F";
					break;
				case KeyCode::KpXOR:
					Name = "Xor";
					break;
				case KeyCode::Power:
				case KeyCode::KpPower:
					Name = "Power";
					break;
				case KeyCode::KpPercent:
					Name = "Percent";
					break;
				case KeyCode::KpLess:
					Name = "Less";
					break;
				case KeyCode::KpGreater:
					Name = "Greater";
					break;
				case KeyCode::KpAmpersand:
					Name = "Ampersand";
					break;
				case KeyCode::KpDBLAmpersand:
					Name = "DBL Ampersand";
					break;
				case KeyCode::KpVerticalBar:
					Name = "Vertical Bar";
					break;
				case KeyCode::KpDBLVerticalBar:
					Name = "OBL Vertical Bar";
					break;
				case KeyCode::KpColon:
					Name = "Colon";
					break;
				case KeyCode::KpHash:
					Name = "Hash";
					break;
				case KeyCode::Space:
				case KeyCode::KpSpace:
					Name = "Space";
					break;
				case KeyCode::KpAt:
					Name = "At";
					break;
				case KeyCode::KpExclaim:
					Name = "Exclam";
					break;
				case KeyCode::KpMemStore:
					Name = "Mem Store";
					break;
				case KeyCode::KpMemRecall:
					Name = "Mem Recall";
					break;
				case KeyCode::KpMemClear:
					Name = "Mem Clear";
					break;
				case KeyCode::KpMemAdd:
					Name = "Mem Add";
					break;
				case KeyCode::KpMemSubtract:
					Name = "Mem Subtract";
					break;
				case KeyCode::KpMemMultiply:
					Name = "Mem Multiply";
					break;
				case KeyCode::KpMemDivide:
					Name = "Mem Divide";
					break;
				case KeyCode::KpPlusMinus:
					Name = "Plus-Minus";
					break;
				case KeyCode::Clear:
				case KeyCode::KpClear:
					Name = "Clear";
					break;
				case KeyCode::KpClearEntry:
					Name = "Clear Entry";
					break;
				case KeyCode::KpBinary:
					Name = "Binary";
					break;
				case KeyCode::KpOctal:
					Name = "Octal";
					break;
				case KeyCode::KpDecimal:
					Name = "Decimal";
					break;
				case KeyCode::KpHexadecimal:
					Name = "Hexadecimal";
					break;
				case KeyCode::LeftControl:
					Name = "Left CTRL";
					break;
				case KeyCode::LeftShift:
					Name = "Left Shift";
					break;
				case KeyCode::LeftAlt:
					Name = "Left Alt";
					break;
				case KeyCode::LeftGUI:
					Name = "Left GUI";
					break;
				case KeyCode::RightControl:
					Name = "Right CTRL";
					break;
				case KeyCode::RightShift:
					Name = "Right Shift";
					break;
				case KeyCode::RightAlt:
					Name = "Right Alt";
					break;
				case KeyCode::RightGUI:
					Name = "Right GUI";
					break;
				case KeyCode::Mode:
					Name = "Mode";
					break;
				case KeyCode::AudioNext:
					Name = "Audio Next";
					break;
				case KeyCode::AudioPrev:
					Name = "Audio Prev";
					break;
				case KeyCode::AudioStop:
					Name = "Audio Stop";
					break;
				case KeyCode::AudioPlay:
					Name = "Audio Play";
					break;
				case KeyCode::AudioMute:
					Name = "Audio Mute";
					break;
				case KeyCode::MediaSelect:
					Name = "Media Select";
					break;
				case KeyCode::WWW:
					Name = "WWW";
					break;
				case KeyCode::Mail:
					Name = "Mail";
					break;
				case KeyCode::Calculator:
					Name = "Calculator";
					break;
				case KeyCode::Computer:
					Name = "Computer";
					break;
				case KeyCode::AcSearch:
					Name = "AC Search";
					break;
				case KeyCode::AcHome:
					Name = "AC Home";
					break;
				case KeyCode::AcBack:
					Name = "AC Back";
					break;
				case KeyCode::AcForward:
					Name = "AC Forward";
					break;
				case KeyCode::AcStop:
					Name = "AC Stop";
					break;
				case KeyCode::AcRefresh:
					Name = "AC Refresh";
					break;
				case KeyCode::AcBookmarks:
					Name = "AC Bookmarks";
					break;
				case KeyCode::BrightnessDown:
					Name = "Brigthness Down";
					break;
				case KeyCode::BrightnessUp:
					Name = "Brigthness Up";
					break;
				case KeyCode::DisplaySwitch:
					Name = "Display Switch";
					break;
				case KeyCode::KbIllumToggle:
					Name = "Dillum Toggle";
					break;
				case KeyCode::KbIllumDown:
					Name = "Dillum Down";
					break;
				case KeyCode::KbIllumUp:
					Name = "Dillum Up";
					break;
				case KeyCode::Eject:
					Name = "Eject";
					break;
				case KeyCode::Sleep:
					Name = "Sleep";
					break;
				case KeyCode::App1:
					Name = "App 1";
					break;
				case KeyCode::App2:
					Name = "App 2";
					break;
				case KeyCode::AudioRewind:
					Name = "Audio Rewind";
					break;
				case KeyCode::AudioFastForward:
					Name = "Audio Fast Forward";
					break;
				case KeyCode::CursorLeft:
					Name = "Cursor Left";
					break;
				case KeyCode::CursorMiddle:
					Name = "Cursor Middle";
					break;
				case KeyCode::CursorRight:
					Name = "Cursor Right";
					break;
				case KeyCode::CursorX1:
					Name = "Cursor X1";
					break;
				case KeyCode::CursorX2:
					Name = "Cursor X2";
					break;
				default:
					Name = nullptr;
					break;
			}

			return Name;
		}
		const char* Video::GetKeyModAsLiteral(KeyMod Code)
		{
			const char* Name;
			switch (Code)
			{
				case KeyMod::LeftShift:
					Name = "Left Shift";
					break;
				case KeyMod::RightShift:
					Name = "Right Shift";
					break;
				case KeyMod::LeftControl:
					Name = "Left Ctrl";
					break;
				case KeyMod::RightControl:
					Name = "Right Ctrl";
					break;
				case KeyMod::LeftAlt:
					Name = "Left Alt";
					break;
				case KeyMod::RightAlt:
					Name = "Right Alt";
					break;
				case KeyMod::LeftGUI:
					Name = "Left Gui";
					break;
				case KeyMod::RightGUI:
					Name = "Right Gui";
					break;
				case KeyMod::Num:
					Name = "Num-lock";
					break;
				case KeyMod::Caps:
					Name = "Caps-lock";
					break;
				case KeyMod::Mode:
					Name = "Mode";
					break;
				case KeyMod::Shift:
					Name = "Shift";
					break;
				case KeyMod::Control:
					Name = "Ctrl";
					break;
				case KeyMod::Alt:
					Name = "Alt";
					break;
				case KeyMod::GUI:
					Name = "Gui";
					break;
				default:
					Name = nullptr;
					break;
			}

			return Name;
		}
		Core::String Video::GetKeyCodeAsString(KeyCode Code)
		{
			return GetKeyCodeAsLiteral(Code);
		}
		Core::String Video::GetKeyModAsString(KeyMod Code)
		{
			return GetKeyModAsLiteral(Code);
		}
	}
}
