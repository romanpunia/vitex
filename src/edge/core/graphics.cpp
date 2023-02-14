#include "graphics.h"
#include "../graphics/d3d11.h"
#include "../graphics/ogl.h"
#include "../graphics/dynamic/shaders.hpp"
#ifdef ED_MICROSOFT
#include <dwmapi.h>
#endif
#ifdef ED_HAS_SDL2
#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#endif
#ifdef ED_HAS_SPIRV
#include <spirv_cross/spirv.hpp>
#include <spirv_cross/spirv_glsl.hpp>
#include <spirv_cross/spirv_hlsl.hpp>
#include <spirv_cross/spirv_msl.hpp>
#include <SPIRV/GlslangToSpv.h>
#include <SPIRV/disassemble.h>
#include <SPIRV/GlslangToSpv.h>
#include <SPIRV/SPVRemapper.h>
#include <SPIRV/disassemble.h>
#include <SPIRV/doc.h>
#include <glslang/Include/ResourceLimits.h>
#include <glslang/Public/ShaderLang.h>
#include <glslang/Public/ShaderLang.h>
#include <sstream>

namespace
{
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
}
#endif
namespace
{
	static Edge::Graphics::RenderBackend GetSupportedBackend(Edge::Graphics::RenderBackend Type)
	{
		if (Type != Edge::Graphics::RenderBackend::Automatic)
			return Type;
#ifdef ED_MICROSOFT
		return Edge::Graphics::RenderBackend::D3D11;
#endif
#ifdef ED_HAS_GL
		return Edge::Graphics::RenderBackend::OGL;
#endif
		return Edge::Graphics::RenderBackend::None;
	}
}

namespace Edge
{
	namespace Graphics
	{
		Alert::Alert(Activity* From) noexcept : View(AlertType::None), Base(From), Waiting(false)
		{
		}
		void Alert::Setup(AlertType Type, const std::string& Title, const std::string& Text)
		{
			ED_ASSERT_V(Type != AlertType::None, "alert type should not be none");
			View = Type;
			Name = Title;
			Data = Text;
			Buttons.clear();
		}
		void Alert::Button(AlertConfirm Confirm, const std::string& Text, int Id)
		{
			ED_ASSERT_V(View != AlertType::None, "alert type should not be none");
			ED_ASSERT_V(Buttons.size() < 16, "there must be less than 16 buttons in alert");

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
			ED_ASSERT_V(View != AlertType::None, "alert type should not be none");
			Done = Callback;
			Waiting = true;
		}
		void Alert::Dispatch()
		{
#ifdef ED_HAS_SDL2
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

		void PoseBuffer::SetJointPose(Compute::Joint* Root)
		{
			ED_ASSERT_V(Root != nullptr, "root should be set");
			auto& Node = Pose[Root->Index];
			Node.Position = Root->Transform.Position();
			Node.Rotation = Root->Transform.Rotation();

			for (auto&& Child : Root->Childs)
				SetJointPose(&Child);
		}
		void PoseBuffer::GetJointPose(Compute::Joint* Root, std::vector<Compute::AnimatorKey>* Result)
		{
			ED_ASSERT_V(Root != nullptr, "root should be set");
			ED_ASSERT_V(Result != nullptr, "result should be set");

			Compute::AnimatorKey* Key = &Result->at((size_t)Root->Index);
			Key->Position = Root->Transform.Position();
			Key->Rotation = Root->Transform.Rotation();

			for (auto&& Child : Root->Childs)
				GetJointPose(&Child, Result);
		}
		bool PoseBuffer::SetPose(SkinModel* Model)
		{
			ED_ASSERT(Model != nullptr, false, "model should be set");
			for (auto&& Child : Model->Joints)
				SetJointPose(&Child);

			return true;
		}
		bool PoseBuffer::GetPose(SkinModel* Model, std::vector<Compute::AnimatorKey>* Result)
		{
			ED_ASSERT(Model != nullptr, false, "model should be set");
			ED_ASSERT(Result != nullptr, false, "result should be set");

			for (auto&& Child : Model->Joints)
				GetJointPose(&Child, Result);

			return true;
		}
		Compute::Matrix4x4 PoseBuffer::GetOffset(PoseNode* Node)
		{
			ED_ASSERT(Node != nullptr, Compute::Matrix4x4::Identity(), "node should be set");
			return Compute::Matrix4x4::Create(Node->Position, Node->Rotation);
		}
		PoseNode* PoseBuffer::GetNode(int64_t Index)
		{
			auto It = Pose.find(Index);
			if (It != Pose.end())
				return &It->second;

			return nullptr;
		}

		Surface::Surface() noexcept : Handle(nullptr)
		{
		}
		Surface::Surface(SDL_Surface* From) noexcept : Handle(From)
		{
		}
		Surface::~Surface() noexcept
		{
#ifdef ED_HAS_SDL2
			if (Handle != nullptr)
			{
				SDL_FreeSurface(Handle);
				Handle = nullptr;
			}
#endif
		}
		void Surface::SetHandle(SDL_Surface* From)
		{
#ifdef ED_HAS_SDL2
			if (Handle != nullptr)
				SDL_FreeSurface(Handle);
#endif
			Handle = From;
		}
		void Surface::Lock()
		{
#ifdef ED_HAS_SDL2
			SDL_LockSurface(Handle);
#endif
		}
		void Surface::Unlock()
		{
#ifdef ED_HAS_SDL2
			SDL_UnlockSurface(Handle);
#endif
		}
		int Surface::GetWidth() const
		{
#ifdef ED_HAS_SDL2
			ED_ASSERT(Handle != nullptr, -1, "handle should be set");
			return Handle->w;
#else
			return -1;
#endif
		}
		int Surface::GetHeight() const
		{
#ifdef ED_HAS_SDL2
			ED_ASSERT(Handle != nullptr, -1, "handle should be set");
			return Handle->h;
#else
			return -1;
#endif
		}
		int Surface::GetPitch() const
		{
#ifdef ED_HAS_SDL2
			ED_ASSERT(Handle != nullptr, -1, "handle should be set");
			return Handle->pitch;
#else
			return -1;
#endif
		}
		void* Surface::GetPixels() const
		{
#ifdef ED_HAS_SDL2
			ED_ASSERT(Handle != nullptr, nullptr, "handle should be set");
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
		const std::vector<InputLayout::Attribute>& InputLayout::GetAttributes() const
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
			ED_RELEASE(VertexBuffer);
			ED_RELEASE(IndexBuffer);
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
			ED_RELEASE(VertexBuffer);
			ED_RELEASE(IndexBuffer);
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
			ED_RELEASE(Elements);
		}
		std::vector<Compute::ElementVertex>& InstanceBuffer::GetArray()
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
			Width = 512;
			Height = 512;
			MipLevels = 1;
			FormatMode = Format::Unknown;
			Usage = ResourceUsage::Default;
			AccessFlags = CPUAccess::Invalid;
		}
		Texture2D::Texture2D(const Desc& I) noexcept
		{
			Width = I.Width;
			Height = I.Height;
			MipLevels = I.MipLevels;
			FormatMode = I.FormatMode;
			Usage = I.Usage;
			AccessFlags = I.AccessFlags;
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
			Width = 512;
			Height = 512;
			Depth = 1;
			MipLevels = 1;
			FormatMode = Format::Unknown;
			Usage = ResourceUsage::Default;
			AccessFlags = CPUAccess::Invalid;
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
			Width = 512;
			Height = 512;
			MipLevels = 1;
			FormatMode = Format::Unknown;
			Usage = ResourceUsage::Default;
			AccessFlags = CPUAccess::Invalid;
		}
		TextureCube::TextureCube(const Desc& I) noexcept
		{
			Width = I.Width;
			Height = I.Height;
			MipLevels = I.MipLevels;
			FormatMode = I.FormatMode;
			Usage = I.Usage;
			AccessFlags = I.AccessFlags;
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

		DepthTarget2D::DepthTarget2D(const Desc& I) noexcept : Resource(nullptr), Viewarea({ 0, 0, 512, 512, 0, 1 })
		{
		}
		DepthTarget2D::~DepthTarget2D() noexcept
		{
			ED_RELEASE(Resource);
		}
		Texture2D* DepthTarget2D::GetTarget()
		{
			return Resource;
		}
		const Viewport& DepthTarget2D::GetViewport() const
		{
			return Viewarea;
		}

		DepthTargetCube::DepthTargetCube(const Desc& I) noexcept : Resource(nullptr), Viewarea({ 0, 0, 512, 512, 0, 1 })
		{
		}
		DepthTargetCube::~DepthTargetCube() noexcept
		{
			ED_RELEASE(Resource);
		}
		TextureCube* DepthTargetCube::GetTarget()
		{
			return Resource;
		}
		const Viewport& DepthTargetCube::GetViewport() const
		{
			return Viewarea;
		}

		RenderTarget::RenderTarget() noexcept : DepthStencil(nullptr), Viewarea({ 0, 0, 512, 512, 0, 1 })
		{
		}
		RenderTarget::~RenderTarget() noexcept
		{
			ED_RELEASE(DepthStencil);
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
			ED_RELEASE(Resource);
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
			ED_ASSERT_V((uint32_t)I.Target <= 8, "target should be less than 9");
			Target = I.Target;

			for (uint32_t i = 0; i < 8; i++)
				Resource[i] = nullptr;
		}
		MultiRenderTarget2D::~MultiRenderTarget2D() noexcept
		{
			ED_ASSERT_V((uint32_t)Target <= 8, "target should be less than 9");
			for (uint32_t i = 0; i < (uint32_t)Target; i++)
				ED_RELEASE(Resource[i]);
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
			ED_ASSERT(Slot < (uint32_t)Target, nullptr, "slot should be less than targets count");
			return Resource[Slot];
		}

		RenderTargetCube::RenderTargetCube(const Desc& I) noexcept : RenderTarget(), Resource(nullptr)
		{
		}
		RenderTargetCube::~RenderTargetCube() noexcept
		{
			ED_RELEASE(Resource);
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
			ED_ASSERT_V((uint32_t)I.Target <= 8, "target should be less than 9");
			Target = I.Target;

			for (uint32_t i = 0; i < 8; i++)
				Resource[i] = nullptr;
		}
		MultiRenderTargetCube::~MultiRenderTargetCube() noexcept
		{
			ED_ASSERT_V((uint32_t)Target <= 8, "target should be less than 9");
			for (uint32_t i = 0; i < (uint32_t)Target; i++)
				ED_RELEASE(Resource[i]);
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
			ED_ASSERT(Slot < (uint32_t)Target, nullptr, "slot should be less than targets count");
			return Resource[Slot];
		}

		Cubemap::Cubemap(const Desc& I) noexcept : Meta(I), Dest(nullptr)
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
			if (!I.CacheDirectory.empty())
			{
				Caches = Core::OS::Path::ResolveDirectory(I.CacheDirectory.c_str());
				if (!Core::OS::Directory::IsExists(Caches.c_str()))
					Caches.clear();
			}

			CreateSections();
		}
		GraphicsDevice::~GraphicsDevice() noexcept
		{
			ReleaseProxy();
			for (auto It = Sections.begin(); It != Sections.end(); It++)
				ED_DELETE(Section, It->second);
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
			DepthStencilStates["less"] = CreateDepthStencilState(DepthStencil);

			DepthStencil.DepthWriteMask = DepthWrite::Zero;
			DepthStencil.StencilWriteMask = 0x0;
			DepthStencilStates["less-read-only"] = CreateDepthStencilState(DepthStencil);

			DepthStencil.DepthFunction = Comparison::Greater_Equal;
			DepthStencilStates["greater-read-only"] = CreateDepthStencilState(DepthStencil);

			DepthStencil.DepthWriteMask = DepthWrite::All;
			DepthStencil.StencilWriteMask = 0xFF;
			DepthStencilStates["greater-equal"] = CreateDepthStencilState(DepthStencil);

			DepthStencil.DepthEnable = false;
			DepthStencil.DepthFunction = Comparison::Less;
			DepthStencil.StencilEnable = false;
			DepthStencilStates["none"] = CreateDepthStencilState(DepthStencil);

			DepthStencil.DepthEnable = true;
			DepthStencil.DepthWriteMask = DepthWrite::Zero;
			DepthStencil.StencilEnable = true;
			DepthStencilStates["less-none"] = CreateDepthStencilState(DepthStencil);

			DepthStencil.DepthWriteMask = DepthWrite::All;
			DepthStencil.StencilEnable = false;
			DepthStencilStates["less-no-stencil"] = CreateDepthStencilState(DepthStencil);

			DepthStencil.DepthFunction = Comparison::Less_Equal;
			DepthStencil.DepthWriteMask = DepthWrite::Zero;
			DepthStencil.StencilEnable = false;
			DepthStencilStates["less-no-stencil-none"] = CreateDepthStencilState(DepthStencil);

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
			RasterizerStates["cull-back"] = CreateRasterizerState(Rasterizer);

			Rasterizer.CullMode = VertexCull::Front;
			RasterizerStates["cull-front"] = CreateRasterizerState(Rasterizer);

			Rasterizer.CullMode = VertexCull::None;
			RasterizerStates["cull-none"] = CreateRasterizerState(Rasterizer);

			Rasterizer.ScissorEnable = true;
			RasterizerStates["cull-none-scissor"] = CreateRasterizerState(Rasterizer);

			Rasterizer.CullMode = VertexCull::Back;
			RasterizerStates["cull-back-scissor"] = CreateRasterizerState(Rasterizer);

			BlendState::Desc Blend;
			Blend.AlphaToCoverageEnable = false;
			Blend.IndependentBlendEnable = false;
			Blend.RenderTarget[0].BlendEnable = false;
			Blend.RenderTarget[0].RenderTargetWriteMask = (unsigned char)ColorWriteEnable::All;
			BlendStates["overwrite"] = CreateBlendState(Blend);

			Blend.RenderTarget[0].RenderTargetWriteMask = (unsigned char)(ColorWriteEnable::Red | ColorWriteEnable::Green | ColorWriteEnable::Blue);
			BlendStates["overwrite-opaque"] = CreateBlendState(Blend);

			Blend.RenderTarget[0].RenderTargetWriteMask = 0;
			BlendStates["overwrite-colorless"] = CreateBlendState(Blend);

			Blend.RenderTarget[0].BlendEnable = true;
			Blend.RenderTarget[0].SrcBlend = Blend::One;
			Blend.RenderTarget[0].DestBlend = Blend::One;
			Blend.RenderTarget[0].BlendOperationMode = BlendOperation::Add;
			Blend.RenderTarget[0].SrcBlendAlpha = Blend::One;
			Blend.RenderTarget[0].DestBlendAlpha = Blend::One;
			Blend.RenderTarget[0].BlendOperationAlpha = BlendOperation::Add;
			Blend.RenderTarget[0].RenderTargetWriteMask = (unsigned char)ColorWriteEnable::All;
			BlendStates["additive"] = CreateBlendState(Blend);

			Blend.RenderTarget[0].RenderTargetWriteMask = (unsigned char)(ColorWriteEnable::Red | ColorWriteEnable::Green | ColorWriteEnable::Blue);
			BlendStates["additive-opaque"] = CreateBlendState(Blend);

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
			BlendStates["additive-gbuffer"] = CreateBlendState(Blend);

			Blend.IndependentBlendEnable = false;
			Blend.RenderTarget[0].BlendEnable = true;
			Blend.RenderTarget[0].SrcBlend = Blend::Source_Alpha;
			BlendStates["additive-alpha"] = CreateBlendState(Blend);

			Blend.RenderTarget[0].DestBlend = Blend::Source_Alpha_Invert;
			Blend.RenderTarget[0].SrcBlendAlpha = Blend::Source_Alpha_Invert;
			Blend.RenderTarget[0].DestBlendAlpha = Blend::Zero;
			BlendStates["additive-source"] = CreateBlendState(Blend);

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
			SamplerStates["trilinear-x16"] = CreateSamplerState(Sampler);

			Sampler.Filter = PixelFilter::Min_Mag_Mip_Linear;
			Sampler.AddressU = TextureAddress::Clamp;
			Sampler.AddressV = TextureAddress::Clamp;
			Sampler.AddressW = TextureAddress::Clamp;
			SamplerStates["linear"] = CreateSamplerState(Sampler);

			Sampler.Filter = PixelFilter::Min_Mag_Mip_Point;
			Sampler.ComparisonFunction = Comparison::Never;
			SamplerStates["point"] = CreateSamplerState(Sampler);

			Sampler.Filter = PixelFilter::Min_Mag_Mip_Linear;
			Sampler.MaxAnisotropy = 1;
			SamplerStates["depth"] = CreateSamplerState(Sampler);

			Sampler.Filter = PixelFilter::Compare_Min_Mag_Mip_Linear;
			Sampler.ComparisonFunction = Comparison::Less;
			SamplerStates["depth-cmp-less"] = CreateSamplerState(Sampler);

			Sampler.Filter = PixelFilter::Compare_Min_Mag_Mip_Linear;
			Sampler.ComparisonFunction = Comparison::Greater_Equal;
			SamplerStates["depth-cmp-greater"] = CreateSamplerState(Sampler);

			InputLayout::Desc Layout;
			Layout.Attributes =
			{
				{ "POSITION", 0, AttributeType::Float, 3, 0 },
				{ "TEXCOORD", 0, AttributeType::Float, 2, 3 * sizeof(float) }
			};
			InputLayouts["shape-vertex"] = CreateInputLayout(Layout);

			Layout.Attributes =
			{
				{ "POSITION", 0, AttributeType::Float, 3, 0 },
				{ "TEXCOORD", 0, AttributeType::Float, 4, 3 * sizeof(float) },
				{ "TEXCOORD", 1, AttributeType::Float, 4, 7 * sizeof(float) },
				{ "TEXCOORD", 2, AttributeType::Float, 3, 11 * sizeof(float) }
			};
			InputLayouts["element-vertex"] = CreateInputLayout(Layout);

			Layout.Attributes =
			{
				{ "POSITION", 0, AttributeType::Float, 3, 0 },
				{ "TEXCOORD", 0, AttributeType::Float, 2, 3 * sizeof(float) },
				{ "NORMAL", 0, AttributeType::Float, 3, 5 * sizeof(float) },
				{ "TANGENT", 0, AttributeType::Float, 3, 8 * sizeof(float) },
				{ "BINORMAL", 0, AttributeType::Float, 3, 11 * sizeof(float) }
			};
			InputLayouts["vertex"] = CreateInputLayout(Layout);

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
			InputLayouts["vertex-instance"] = CreateInputLayout(Layout);

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
			InputLayouts["skin-vertex"] = CreateInputLayout(Layout);

			Layout.Attributes =
			{
				{ "POSITION", 0, AttributeType::Float, 2, 0 },
				{ "COLOR", 0, AttributeType::Ubyte, 4, 2 * sizeof(float) },
				{ "TEXCOORD", 0, AttributeType::Float, 2, 2 * sizeof(float) + 4 * sizeof(unsigned char) }
			};
			InputLayouts["gui-vertex"] = CreateInputLayout(Layout);

			SetDepthStencilState(GetDepthStencilState("less"));
			SetRasterizerState(GetRasterizerState("cull-back"));
			SetBlendState(GetBlendState("overwrite"));
		}
		void GraphicsDevice::CreateSections()
		{
#ifdef HAS_SHADER_BATCH
			shader_batch::foreach(this, [](void* Context, const char* Name, const unsigned char* Buffer, unsigned Size)
			{
				GraphicsDevice* Base = (GraphicsDevice*)Context;
				if (Base != nullptr && Base->GetBackend() != RenderBackend::None)
					Base->AddSection(Name, std::string((const char*)Buffer, Size));
			});
#else
			ED_WARN("[graphics] default shader resources were not compiled");
#endif
		}
		void GraphicsDevice::ReleaseProxy()
		{
			for (auto It = DepthStencilStates.begin(); It != DepthStencilStates.end(); It++)
				ED_RELEASE(It->second);
			DepthStencilStates.clear();

			for (auto It = RasterizerStates.begin(); It != RasterizerStates.end(); It++)
				ED_RELEASE(It->second);
			RasterizerStates.clear();

			for (auto It = BlendStates.begin(); It != BlendStates.end(); It++)
				ED_RELEASE(It->second);
			BlendStates.clear();

			for (auto It = SamplerStates.begin(); It != SamplerStates.end(); It++)
				ED_RELEASE(It->second);
			SamplerStates.clear();

			for (auto It = InputLayouts.begin(); It != InputLayouts.end(); It++)
				ED_RELEASE(It->second);
			InputLayouts.clear();

			ED_CLEAR(RenderTarget);
			ED_CLEAR(BasicEffect);
		}
		bool GraphicsDevice::AddSection(const std::string& Name, const std::string& Code)
		{
			Core::Parser Language(Core::OS::Path::GetExtension(Name.c_str()));
			Language.Substring(1).Trim().ToLower();
			RemoveSection(Name);

			Section* Include = ED_NEW(Section);
			Include->Code = Code;
			Include->Name = Name;
			Sections[Name] = Include;

			return true;
		}
		bool GraphicsDevice::RemoveSection(const std::string& Name)
		{
			auto It = Sections.find(Name);
			if (It == Sections.end())
				return false;

			ED_DELETE(Section, It->second);
			Sections.erase(It);

			return true;
		}
		bool GraphicsDevice::Preprocess(Shader::Desc& Subresult)
		{
			if (Subresult.Data.empty())
				return true;

			switch (Backend)
			{
				case Edge::Graphics::RenderBackend::D3D11:
					Subresult.Defines.push_back("TARGET_D3D");
					break;
				case Edge::Graphics::RenderBackend::OGL:
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

					Output->assign(Result->Code);
					return true;
				}

				size_t Length;
				unsigned char* Data = Core::OS::File::ReadAll(File.Module, &Length);
				if (!Data)
					return false;

				Output->assign((const char*)Data, (size_t)Length);
				ED_FREE(Data);
				return true;
			});
			Processor->SetIncludeOptions(Desc);
			Processor->SetFeatures(Subresult.Features);

			for (auto& Word : Subresult.Defines)
				Processor->Define(Word);

			bool Preprocessed = Processor->Process(Subresult.Filename, Subresult.Data);
			ED_RELEASE(Processor);

			return Preprocessed;
		}
		bool GraphicsDevice::Transpile(std::string* HLSL, ShaderType Type, ShaderLang To)
		{
			if (!HLSL || HLSL->empty())
				return true;
#ifdef ED_HAS_SPIRV
			const char* Buffer = HLSL->c_str();
			int Size = (int)HLSL->size();

			ShaderModel Model = GetShaderModel();
			if (Model == ShaderModel::Auto || Model == ShaderModel::Invalid)
			{
				ED_ERR("[graphics] cannot transpile shader source without explicit shader model version");
				return false;
			}

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
					ED_ERR("[graphics] cannot transpile shader source without explicit shader type");
					return false;
			}

			std::string Entry = GetShaderMain(Type);
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
				ED_ERR("[graphics] cannot transpile shader source\n\t%s", Output);

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
				ED_ERR("[graphics] cannot transpile shader source to spirv binary");
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

					*HLSL = Compiler.compile();
					if (HLSL->empty())
						return true;

					Core::Parser Parser(HLSL);
					Parser.ReplaceGroups("layout\\(row_major\\)\\s+", "");
					Parser.ReplaceGroups("invocations\\s+=\\s+\\d+,\\s+", "");

					return true;
				}
				else if (To == ShaderLang::HLSL)
				{
					spirv_cross::CompilerHLSL::Options Options;
					Options.shader_model = (uint32_t)Model;

					spirv_cross::CompilerHLSL Compiler(Binary);
					Compiler.set_hlsl_options(Options);

					*HLSL = Compiler.compile();
					return true;
				}
				else if (To == ShaderLang::MSL)
				{
					spirv_cross::CompilerMSL::Options Options;
					spirv_cross::CompilerMSL Compiler(Binary);
					Compiler.set_msl_options(Options);
					Compiler.build_dummy_sampler_for_combined_images();
					Compiler.build_combined_image_samplers();
					PrepareSamplers(&Compiler);

					*HLSL = Compiler.compile();
					return true;
				}
				else if (To == ShaderLang::SPV)
				{
					std::stringstream Stream;
					std::copy(Binary.begin(), Binary.end(), std::ostream_iterator<uint32_t>(Stream, " "));

					HLSL->assign(Stream.str());
					return true;
				}
			}
			catch (const spirv_cross::CompilerError& Exception)
			{
				ED_ERR("[graphics] cannot transpile spirv binary to shader binary\n\t%s", Exception.what());
				return false;
			}
			catch (...)
			{
				ED_ERR("[graphics] cannot transpile spirv binary to shader binary");
				return false;
			}

			ED_ERR("[graphics] shader source can be transpiled only to GLSL, GLSL_ES, HLSL, MSL or SPV");
			return false;
#else
			ED_ERR("[graphics] cannot transpile shader source without spirv-cross and glslang libraries");
			return false;
#endif
		}
		bool GraphicsDevice::GetSection(const std::string& Name, Section** Result, bool Internal)
		{
			if (Name.empty() || Sections.empty())
			{
				if (!Internal)
					ED_ERR("[graphics] could not find shader: \"%s\"");

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
				ED_ERR("[graphics] could not find shader: \"%s\"", Name.c_str());

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

			return true;
		}
		bool GraphicsDevice::IsDebug() const
		{
			return Debug;
		}
		bool GraphicsDevice::GetProgramCache(const std::string& Name, std::string* Data)
		{
			ED_ASSERT(Data != nullptr, false, "data should be set");
			Data->clear();

			if (!ShaderCache || Caches.empty())
				return false;

			std::string Path = Caches + Name;
			if (Path.empty())
				return false;

			if (!Core::OS::File::IsExists(Path.c_str()))
				return false;

			Core::GzStream* Stream = new Core::GzStream();
			if (!Stream->Open(Path.c_str(), Core::FileMode::Binary_Read_Only))
			{
				ED_RELEASE(Stream);
				return false;
			}

			char Buffer[ED_BIG_CHUNK_SIZE]; size_t Size = 0;
			while ((Size = (size_t)Stream->Read(Buffer, sizeof(Buffer))) > 0)
				Data->append(std::string(Buffer, Size));

			ED_DEBUG("[graphics] load %s program cache", Name.c_str());
			ED_RELEASE(Stream);

			return !Data->empty();
		}
		bool GraphicsDevice::SetProgramCache(const std::string& Name, const std::string& Data)
		{
			if (!ShaderCache || Caches.empty())
				return true;

			std::string Path = Caches + Name;
			if (Path.empty())
				return false;

			Core::GzStream* Stream = new Core::GzStream();
			if (!Stream->Open(Path.c_str(), Core::FileMode::Binary_Write_Only))
			{
				ED_RELEASE(Stream);
				return false;
			}

			size_t Size = Data.size();
			bool Result = (Stream->Write(Data.c_str(), Size) == Size);
			ED_DEBUG("[graphics] save %s program cache", Name.c_str());
			ED_RELEASE(Stream);

			return Result;
		}
		bool GraphicsDevice::IsLeftHanded() const
		{
			return Backend == RenderBackend::D3D11;
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
		unsigned int GraphicsDevice::GetPresentFlags() const
		{
			return PresentFlags;
		}
		unsigned int GraphicsDevice::GetCompileFlags() const
		{
			return CompileFlags;
		}
		std::string GraphicsDevice::GetProgramName(const Shader::Desc& Desc)
		{
			std::string Result = Desc.Filename;
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

			std::string Prefix;
			switch (Backend)
			{
				case Edge::Graphics::RenderBackend::D3D11:
					Prefix = "hlsl.";
					break;
				case Edge::Graphics::RenderBackend::OGL:
					Prefix = "glsl.";
					break;
                default:
                    break;
			}

			return Prefix + Compute::Crypto::Hash(Compute::Digests::MD5(), Result) + ".sasm";
		}
		std::string GraphicsDevice::GetShaderMain(ShaderType Type) const
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
#ifdef ED_MICROSOFT
			if (I.Backend == RenderBackend::D3D11)
				return new D3D11::D3D11Device(I);
#endif
#ifdef ED_HAS_GL
			if (I.Backend == RenderBackend::OGL)
				return new OGL::OGLDevice(I);
#endif
			ED_ERR("[graphics] backend was not found");
			return nullptr;
		}

		Activity::Activity(const Desc& I) noexcept : Handle(nullptr), Options(I), Command(0), CX(0), CY(0), Message(this)
		{
#ifdef ED_HAS_SDL2
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
			if (!I.AllowGraphics)
				BuildLayer(RenderBackend::None);
		}
		Activity::~Activity() noexcept
		{
#ifdef ED_HAS_SDL2
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
#ifdef ED_HAS_SDL2
#ifdef ED_MICROSOFT
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
						//FreeLibrary(Library);
					}
				}
				RegCloseKey(Target);
			}
#endif
#endif
		}
		void Activity::SetClipboardText(const std::string& Text)
		{
#ifdef ED_HAS_SDL2
			ED_ASSERT_V(Handle != nullptr, "activity should be initialized");
			SDL_SetClipboardText(Text.c_str());
#endif
		}
		void Activity::SetCursorPosition(const Compute::Vector2& Position)
		{
#ifdef ED_HAS_SDL2
#if SDL_VERSION_ATLEAST(2, 0, 4)
			ED_ASSERT_V(Handle != nullptr, "activity should be initialized");
			SDL_WarpMouseInWindow(Handle, (int)Position.X, (int)Position.Y);
#endif
#endif
		}
		void Activity::SetCursorPosition(float X, float Y)
		{
#ifdef ED_HAS_SDL2
#if SDL_VERSION_ATLEAST(2, 0, 4)
			ED_ASSERT_V(Handle != nullptr, "activity should be initialized");
			SDL_WarpMouseInWindow(Handle, (int)X, (int)Y);
#endif
#endif
		}
		void Activity::SetGlobalCursorPosition(const Compute::Vector2& Position)
		{
#ifdef ED_HAS_SDL2
			ED_ASSERT_V(Handle != nullptr, "activity should be initialized");
#if SDL_VERSION_ATLEAST(2, 0, 4)
			SDL_WarpMouseGlobal((int)Position.X, (int)Position.Y);
#endif
#endif
		}
		void Activity::SetGlobalCursorPosition(float X, float Y)
		{
#ifdef ED_HAS_SDL2
			ED_ASSERT_V(Handle != nullptr, "activity should be initialized");
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
#ifdef ED_HAS_SDL2
			ED_ASSERT_V((size_t)Style <= (size_t)DisplayCursor::Count, "style should be less than %i", (int)DisplayCursor::Count);
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
#ifdef ED_HAS_SDL2
			ED_ASSERT_V(Handle != nullptr, "activity should be initialized");
			SDL_ShowCursor(Enabled);
#endif
		}
		void Activity::SetGrabbing(bool Enabled)
		{
#ifdef ED_HAS_SDL2
			ED_ASSERT_V(Handle != nullptr, "activity should be initialized");
			SDL_SetWindowGrab(Handle, Enabled ? SDL_TRUE : SDL_FALSE);
#endif
		}
		void Activity::SetFullscreen(bool Enabled)
		{
#ifdef ED_HAS_SDL2
			ED_ASSERT_V(Handle != nullptr, "activity should be initialized");
			SDL_SetWindowFullscreen(Handle, Enabled ? SDL_WINDOW_FULLSCREEN : 0);
#endif
		}
		void Activity::SetBorderless(bool Enabled)
		{
#ifdef ED_HAS_SDL2
			ED_ASSERT_V(Handle != nullptr, "activity should be initialized");
			SDL_SetWindowBordered(Handle, Enabled ? SDL_TRUE : SDL_FALSE);
#endif
		}
		void Activity::SetScreenKeyboard(bool Enabled)
		{
#ifdef ED_HAS_SDL2
			ED_ASSERT_V(Handle != nullptr, "activity should be initialized");
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
#ifdef ED_HAS_SDL2
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

			if (Options.AllowHighDPI)
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
				case Edge::Graphics::RenderBackend::OGL:
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
		void Activity::Hide()
		{
#ifdef ED_HAS_SDL2
			ED_ASSERT_V(Handle != nullptr, "activity should be initialized");
			SDL_HideWindow(Handle);
#endif
		}
		void Activity::Show()
		{
#ifdef ED_HAS_SDL2
			ED_ASSERT_V(Handle != nullptr, "activity should be initialized");
			SDL_ShowWindow(Handle);
#endif
		}
		void Activity::Maximize()
		{
#ifdef ED_HAS_SDL2
			ED_ASSERT_V(Handle != nullptr, "activity should be initialized");
			SDL_MaximizeWindow(Handle);
#endif
		}
		void Activity::Minimize()
		{
#ifdef ED_HAS_SDL2
			ED_ASSERT_V(Handle != nullptr, "activity should be initialized");
			SDL_MinimizeWindow(Handle);
#endif
		}
		void Activity::Focus()
		{
#ifdef ED_HAS_SDL2
#if SDL_VERSION_ATLEAST(2, 0, 5)
			ED_ASSERT_V(Handle != nullptr, "activity should be initialized");
			SDL_SetWindowInputFocus(Handle);
#endif
#endif
		}
		void Activity::Move(int X, int Y)
		{
#ifdef ED_HAS_SDL2
			ED_ASSERT_V(Handle != nullptr, "activity should be initialized");
			SDL_SetWindowPosition(Handle, X, Y);
#endif
		}
		void Activity::Resize(int W, int H)
		{
#ifdef ED_HAS_SDL2
			ED_ASSERT_V(Handle != nullptr, "activity should be initialized");
			SDL_SetWindowSize(Handle, W, H);
#endif
		}
		void Activity::SetTitle(const char* Value)
		{
#ifdef ED_HAS_SDL2
			ED_ASSERT_V(Handle != nullptr, "activity should be initialized");
			ED_ASSERT_V(Value != nullptr, "value should be set");
			SDL_SetWindowTitle(Handle, Value);
#endif
		}
		void Activity::SetIcon(Surface* Icon)
		{
#ifdef ED_HAS_SDL2
			ED_ASSERT_V(Handle != nullptr, "activity should be initialized");
			ED_ASSERT_V(Icon != nullptr, "icon should be set");
			SDL_SetWindowIcon(Handle, (SDL_Surface*)Icon->GetResource());
#endif
		}
		void Activity::Load(SDL_SysWMinfo* Base)
		{
#ifdef ED_HAS_SDL2
			ED_ASSERT_V(Handle != nullptr, "activity should be initialized");
			ED_ASSERT_V(Base != nullptr, "base should be set");
			SDL_VERSION(&Base->version);
			SDL_GetWindowWMInfo(Handle, Base);
#endif
		}
		bool Activity::Dispatch()
		{
			ED_ASSERT(Handle != nullptr, false, "activity should be initialized");
			ED_MEASURE(ED_TIMING_MIX);

			memcpy((void*)Keys[1], (void*)Keys[0], sizeof(Keys[0]));
#ifdef ED_HAS_SDL2
			int Id = SDL_GetWindowID(Handle);
			Command = (int)SDL_GetModState();
			GetInputState();
			Message.Dispatch();

			SDL_Event Event;
			while (SDL_PollEvent(&Event))
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
									Callbacks.KeyState(KeyCode::CURSORLEFT, (KeyMod)SDL_GetModState(), (int)KeyCode::CURSORLEFT, (int)Event.button.clicks, true);

								if (Mapping.Enabled && !Mapping.Mapped)
								{
									Mapping.Key.Key = KeyCode::CURSORLEFT;
									Mapping.Mapped = true;
									Mapping.Captured = false;
								}
								break;
							case SDL_BUTTON_MIDDLE:
								if (Callbacks.KeyState && Id == Event.window.windowID)
									Callbacks.KeyState(KeyCode::CURSORMIDDLE, (KeyMod)SDL_GetModState(), (int)KeyCode::CURSORMIDDLE, (int)Event.button.clicks, true);

								if (Mapping.Enabled && !Mapping.Mapped)
								{
									Mapping.Key.Key = KeyCode::CURSORMIDDLE;
									Mapping.Mapped = true;
									Mapping.Captured = false;
								}
								break;
							case SDL_BUTTON_RIGHT:
								if (Callbacks.KeyState && Id == Event.window.windowID)
									Callbacks.KeyState(KeyCode::CURSORRIGHT, (KeyMod)SDL_GetModState(), (int)KeyCode::CURSORRIGHT, (int)Event.button.clicks, true);

								if (Mapping.Enabled && !Mapping.Mapped)
								{
									Mapping.Key.Key = KeyCode::CURSORRIGHT;
									Mapping.Mapped = true;
									Mapping.Captured = false;
								}
								break;
							case SDL_BUTTON_X1:
								if (Callbacks.KeyState && Id == Event.window.windowID)
									Callbacks.KeyState(KeyCode::CURSORX1, (KeyMod)SDL_GetModState(), (int)KeyCode::CURSORX1, (int)Event.button.clicks, true);

								if (Mapping.Enabled && !Mapping.Mapped)
								{
									Mapping.Key.Key = KeyCode::CURSORX1;
									Mapping.Mapped = true;
									Mapping.Captured = false;
								}
								break;
							case SDL_BUTTON_X2:
								if (Callbacks.KeyState && Id == Event.window.windowID)
									Callbacks.KeyState(KeyCode::CURSORX2, (KeyMod)SDL_GetModState(), (int)KeyCode::CURSORX2, (int)Event.button.clicks, true);

								if (Mapping.Enabled && !Mapping.Mapped)
								{
									Mapping.Key.Key = KeyCode::CURSORX2;
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
									Callbacks.KeyState(KeyCode::CURSORLEFT, (KeyMod)SDL_GetModState(), (int)KeyCode::CURSORLEFT, (int)Event.button.clicks, false);

								if (Mapping.Enabled && Mapping.Mapped && Mapping.Key.Key == KeyCode::CURSORLEFT)
								{
									Mapping.Key.Mod = (KeyMod)SDL_GetModState();
									Mapping.Captured = true;
								}
								break;
							case SDL_BUTTON_MIDDLE:
								if (Callbacks.KeyState && Id == Event.window.windowID)
									Callbacks.KeyState(KeyCode::CURSORMIDDLE, (KeyMod)SDL_GetModState(), (int)KeyCode::CURSORMIDDLE, (int)Event.button.clicks, false);

								if (Mapping.Enabled && Mapping.Mapped && Mapping.Key.Key == KeyCode::CURSORMIDDLE)
								{
									Mapping.Key.Mod = (KeyMod)SDL_GetModState();
									Mapping.Captured = true;
								}
								break;
							case SDL_BUTTON_RIGHT:
								if (Callbacks.KeyState && Id == Event.window.windowID)
									Callbacks.KeyState(KeyCode::CURSORRIGHT, (KeyMod)SDL_GetModState(), (int)KeyCode::CURSORRIGHT, (int)Event.button.clicks, false);

								if (Mapping.Enabled && Mapping.Mapped && Mapping.Key.Key == KeyCode::CURSORRIGHT)
								{
									Mapping.Key.Mod = (KeyMod)SDL_GetModState();
									Mapping.Captured = true;
								}
								break;
							case SDL_BUTTON_X1:
								if (Callbacks.KeyState && Id == Event.window.windowID)
									Callbacks.KeyState(KeyCode::CURSORX1, (KeyMod)SDL_GetModState(), (int)KeyCode::CURSORX1, (int)Event.button.clicks, false);

								if (Mapping.Enabled && Mapping.Mapped && Mapping.Key.Key == KeyCode::CURSORX1)
								{
									Mapping.Key.Mod = (KeyMod)SDL_GetModState();
									Mapping.Captured = true;
								}
								break;
							case SDL_BUTTON_X2:
								if (Callbacks.KeyState && Id == Event.window.windowID)
									Callbacks.KeyState(KeyCode::CURSORX2, (KeyMod)SDL_GetModState(), (int)KeyCode::CURSORX2, (int)Event.button.clicks, false);

								if (Mapping.Enabled && Mapping.Mapped && Mapping.Key.Key == KeyCode::CURSORX2)
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
				}
			}

			if (!Options.AllowStalls)
				return true;

			Uint32 Flags = SDL_GetWindowFlags(Handle);
			if (Flags & SDL_WINDOW_MAXIMIZED || Flags & SDL_WINDOW_INPUT_FOCUS || Flags & SDL_WINDOW_MOUSE_FOCUS)
				return true;

			std::this_thread::sleep_for(std::chrono::milliseconds(66));
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
		bool Activity::IsFullscreen() const
		{
#ifdef ED_HAS_SDL2
			ED_ASSERT(Handle != nullptr, false, "activity should be initialized");
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
#ifdef ED_HAS_SDL2
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
#ifdef ED_HAS_SDL2
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
#ifdef ED_HAS_SDL2
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
#ifdef ED_HAS_SDL2
			ED_ASSERT(Handle != nullptr, false, "activity should be initialized");
			return SDL_IsScreenKeyboardShown(Handle);
#else
			return false;
#endif
		}
		uint32_t Activity::GetX() const
		{
#ifdef ED_HAS_SDL2
			ED_ASSERT(Handle != nullptr, 0, "activity should be initialized");

			int X, Y;
			SDL_GetWindowPosition(Handle, &X, &Y);
			return X;
#else
			return 0;
#endif
		}
		uint32_t Activity::GetY() const
		{
#ifdef ED_HAS_SDL2
			ED_ASSERT(Handle != nullptr, 0, "activity should be initialized");

			int X, Y;
			SDL_GetWindowPosition(Handle, &X, &Y);
			return Y;
#else
			return 0;
#endif
		}
		uint32_t Activity::GetWidth() const
		{
#ifdef ED_HAS_SDL2
			ED_ASSERT(Handle != nullptr, 0, "activity should be initialized");

			int W, H;
			SDL_GetWindowSize(Handle, &W, &H);
			return W;
#else
			return 0;
#endif
		}
		uint32_t Activity::GetHeight() const
		{
#ifdef ED_HAS_SDL2
			ED_ASSERT(Handle != nullptr, 0, "activity should be initialized");

			int W, H;
			SDL_GetWindowSize(Handle, &W, &H);
			return H;
#else
			return 0;
#endif
		}
		float Activity::GetAspectRatio() const
		{
#ifdef ED_HAS_SDL2
			ED_ASSERT(Handle != nullptr, 0.0f, "activity should be initialized");

			int W, H;
			SDL_GetWindowSize(Handle, &W, &H);
			return (H > 0 ? (float)W / (float)H : 0.0f);
#else
			return 0.0f;
#endif
		}
		KeyMod Activity::GetKeyModState() const
		{
#ifdef ED_HAS_SDL2
			ED_ASSERT(Handle != nullptr, KeyMod::None, "activity should be initialized");
			return (KeyMod)SDL_GetModState();
#else
			return KeyMod::None;
#endif
		}
		Viewport Activity::GetViewport() const
		{
#ifdef ED_HAS_SDL2
			ED_ASSERT(Handle != nullptr, Viewport(), "activity should be initialized");

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
#ifdef ED_HAS_SDL2
			ED_ASSERT(Handle != nullptr, 0.0f, "activity should be initialized");

			int W, H;
			SDL_GL_GetDrawableSize(Handle, &W, &H);
			return Compute::Vector2((float)W, (float)H);
#else
			return Compute::Vector2();
#endif
		}
		Compute::Vector2 Activity::GetClientSize() const
		{
#ifdef ED_HAS_SDL2
			ED_ASSERT(Handle != nullptr, 0.0f, "activity should be initialized");

			int W, H;
			SDL_GetWindowSize(Handle, &W, &H);
			return Compute::Vector2((float)W, (float)H);
#else
			return Compute::Vector2();
#endif
		}
		Compute::Vector2 Activity::GetOffset() const
		{
#ifdef ED_HAS_SDL2
			ED_ASSERT(Handle != nullptr, 0.0f, "activity should be initialized");

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
#ifdef ED_HAS_SDL2
			ED_ASSERT(Handle != nullptr, 0.0f, "activity should be initialized");
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
#ifdef ED_HAS_SDL2
			ED_ASSERT(Handle != nullptr, 0.0f, "activity should be initialized");
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
#ifdef ED_HAS_SDL2
			Compute::Vector2 Size = GetSize();
			return GetCursorPosition() * Compute::Vector2(ScreenWidth, ScreenHeight) / Size;
#else
			return Compute::Vector2();
#endif
		}
		Compute::Vector2 Activity::GetCursorPosition(const Compute::Vector2& ScreenDimensions) const
		{
#ifdef ED_HAS_SDL2
			Compute::Vector2 Size = GetSize();
			return GetCursorPosition() * ScreenDimensions / Size;
#else
			return Compute::Vector2();
#endif
		}
		std::string Activity::GetClipboardText() const
		{
#ifdef ED_HAS_SDL2
			ED_ASSERT(Handle != nullptr, std::string(), "activity should be initialized");
			char* Text = SDL_GetClipboardText();
			std::string Result = (Text ? Text : "");

			if (Text != nullptr)
				SDL_free(Text);

			return Result;
#else
			return nullptr;
#endif
		}
		SDL_Window* Activity::GetHandle() const
		{
			return Handle;
		}
		std::string Activity::GetError() const
		{
#ifdef ED_HAS_SDL2
			ED_ASSERT(Handle != nullptr, std::string(), "activity should be initialized");
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
#ifdef ED_HAS_SDL2
			int Count;
			auto* Map = SDL_GetKeyboardState(&Count);
			if (Count > sizeof(Keys[0]) / sizeof(bool))
				Count = sizeof(Keys[0]) / sizeof(bool);

			for (int i = 0; i < Count; i++)
				Keys[0][i] = Map[i] > 0;

			Uint32 State = SDL_GetMouseState(nullptr, nullptr);
			Keys[0][(size_t)KeyCode::CURSORLEFT] = (State & SDL_BUTTON(SDL_BUTTON_LEFT));
			Keys[0][(size_t)KeyCode::CURSORMIDDLE] = (State & SDL_BUTTON(SDL_BUTTON_MIDDLE));
			Keys[0][(size_t)KeyCode::CURSORRIGHT] = (State & SDL_BUTTON(SDL_BUTTON_RIGHT));
			Keys[0][(size_t)KeyCode::CURSORX1] = (State & SDL_BUTTON(SDL_BUTTON_X1));
			Keys[0][(size_t)KeyCode::CURSORX2] = (State & SDL_BUTTON(SDL_BUTTON_X2));
#endif
			return Keys[0];
		}
		const char* Activity::GetKeyCodeName(KeyCode Code)
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
				case KeyCode::RETURN:
					Name = "Return";
					break;
				case KeyCode::ESCAPE:
					Name = "Escape";
					break;
				case KeyCode::LEFTBRACKET:
					Name = "Left Bracket";
					break;
				case KeyCode::RIGHTBRACKET:
					Name = "Rigth Bracket";
					break;
				case KeyCode::BACKSLASH:
					Name = "Backslash";
					break;
				case KeyCode::NONUSHASH:
					Name = "Nonuslash";
					break;
				case KeyCode::SEMICOLON:
					Name = "Semicolon";
					break;
				case KeyCode::APOSTROPHE:
					Name = "Apostrophe";
					break;
				case KeyCode::GRAVE:
					Name = "Grave";
					break;
				case KeyCode::SLASH:
					Name = "Slash";
					break;
				case KeyCode::CAPSLOCK:
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
				case KeyCode::PRINTSCREEN:
					Name = "Print Screen";
					break;
				case KeyCode::SCROLLLOCK:
					Name = "Scroll Lock";
					break;
				case KeyCode::PAUSE:
					Name = "Pause";
					break;
				case KeyCode::INSERT:
					Name = "Insert";
					break;
				case KeyCode::HOME:
					Name = "Home";
					break;
				case KeyCode::PAGEUP:
					Name = "Page Up";
					break;
				case KeyCode::DELETEKEY:
					Name = "Delete";
					break;
				case KeyCode::END:
					Name = "End";
					break;
				case KeyCode::PAGEDOWN:
					Name = "Page Down";
					break;
				case KeyCode::RIGHT:
					Name = "Right";
					break;
				case KeyCode::LEFT:
					Name = "Left";
					break;
				case KeyCode::DOWN:
					Name = "Down";
					break;
				case KeyCode::UP:
					Name = "Up";
					break;
				case KeyCode::NUMLOCKCLEAR:
					Name = "Numlock Clear";
					break;
				case KeyCode::KP_DIVIDE:
					Name = "Divide";
					break;
				case KeyCode::KP_MULTIPLY:
					Name = "Multiply";
					break;
				case KeyCode::MINUS:
				case KeyCode::KP_MINUS:
					Name = "Minus";
					break;
				case KeyCode::KP_PLUS:
					Name = "Plus";
					break;
				case KeyCode::KP_ENTER:
					Name = "Enter";
					break;
				case KeyCode::D1:
				case KeyCode::KP_1:
					Name = "1";
					break;
				case KeyCode::D2:
				case KeyCode::KP_2:
					Name = "2";
					break;
				case KeyCode::D3:
				case KeyCode::KP_3:
					Name = "3";
					break;
				case KeyCode::D4:
				case KeyCode::KP_4:
					Name = "4";
					break;
				case KeyCode::D5:
				case KeyCode::KP_5:
					Name = "5";
					break;
				case KeyCode::D6:
				case KeyCode::KP_6:
					Name = "6";
					break;
				case KeyCode::D7:
				case KeyCode::KP_7:
					Name = "7";
					break;
				case KeyCode::D8:
				case KeyCode::KP_8:
					Name = "8";
					break;
				case KeyCode::D9:
				case KeyCode::KP_9:
					Name = "9";
					break;
				case KeyCode::D0:
				case KeyCode::KP_0:
					Name = "0";
					break;
				case KeyCode::PERIOD:
				case KeyCode::KP_PERIOD:
					Name = "Period";
					break;
				case KeyCode::NONUSBACKSLASH:
					Name = "Nonus Backslash";
					break;
				case KeyCode::APPLICATION:
					Name = "Application";
					break;
				case KeyCode::EQUALS:
				case KeyCode::KP_EQUALS:
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
				case KeyCode::EXECUTE:
					Name = "Execute";
					break;
				case KeyCode::HELP:
					Name = "Help";
					break;
				case KeyCode::MENU:
					Name = "Menu";
					break;
				case KeyCode::SELECT:
					Name = "Select";
					break;
				case KeyCode::STOP:
					Name = "Stop";
					break;
				case KeyCode::AGAIN:
					Name = "Again";
					break;
				case KeyCode::UNDO:
					Name = "Undo";
					break;
				case KeyCode::CUT:
					Name = "Cut";
					break;
				case KeyCode::COPY:
					Name = "Copy";
					break;
				case KeyCode::PASTE:
					Name = "Paste";
					break;
				case KeyCode::FIND:
					Name = "Find";
					break;
				case KeyCode::MUTE:
					Name = "Mute";
					break;
				case KeyCode::VOLUMEUP:
					Name = "Volume Up";
					break;
				case KeyCode::VOLUMEDOWN:
					Name = "Volume Down";
					break;
				case KeyCode::COMMA:
				case KeyCode::KP_COMMA:
					Name = "Comma";
					break;
				case KeyCode::KP_EQUALSAS400:
					Name = "Equals As 400";
					break;
				case KeyCode::INTERNATIONAL1:
					Name = "International 1";
					break;
				case KeyCode::INTERNATIONAL2:
					Name = "International 2";
					break;
				case KeyCode::INTERNATIONAL3:
					Name = "International 3";
					break;
				case KeyCode::INTERNATIONAL4:
					Name = "International 4";
					break;
				case KeyCode::INTERNATIONAL5:
					Name = "International 5";
					break;
				case KeyCode::INTERNATIONAL6:
					Name = "International 6";
					break;
				case KeyCode::INTERNATIONAL7:
					Name = "International 7";
					break;
				case KeyCode::INTERNATIONAL8:
					Name = "International 8";
					break;
				case KeyCode::INTERNATIONAL9:
					Name = "International 9";
					break;
				case KeyCode::LANG1:
					Name = "Lang 1";
					break;
				case KeyCode::LANG2:
					Name = "Lang 2";
					break;
				case KeyCode::LANG3:
					Name = "Lang 3";
					break;
				case KeyCode::LANG4:
					Name = "Lang 4";
					break;
				case KeyCode::LANG5:
					Name = "Lang 5";
					break;
				case KeyCode::LANG6:
					Name = "Lang 6";
					break;
				case KeyCode::LANG7:
					Name = "Lang 7";
					break;
				case KeyCode::LANG8:
					Name = "Lang 8";
					break;
				case KeyCode::LANG9:
					Name = "Lang 9";
					break;
				case KeyCode::ALTERASE:
					Name = "Alter Rase";
					break;
				case KeyCode::SYSREQ:
					Name = "Sys Req";
					break;
				case KeyCode::CANCEL:
					Name = "Cancel";
					break;
				case KeyCode::PRIOR:
					Name = "Prior";
					break;
				case KeyCode::RETURN2:
					Name = "Return 2";
					break;
				case KeyCode::SEPARATOR:
					Name = "Separator";
					break;
				case KeyCode::OUTKEY:
					Name = "Out";
					break;
				case KeyCode::OPER:
					Name = "Oper";
					break;
				case KeyCode::CLEARAGAIN:
					Name = "Clear Again";
					break;
				case KeyCode::CRSEL:
					Name = "CR Sel";
					break;
				case KeyCode::EXSEL:
					Name = "EX Sel";
					break;
				case KeyCode::KP_00:
					Name = "00";
					break;
				case KeyCode::KP_000:
					Name = "000";
					break;
				case KeyCode::THOUSANDSSEPARATOR:
					Name = "Thousands Separator";
					break;
				case KeyCode::DECIMALSEPARATOR:
					Name = "Decimal Separator";
					break;
				case KeyCode::CURRENCYUNIT:
					Name = "Currency Unit";
					break;
				case KeyCode::CURRENCYSUBUNIT:
					Name = "Currency Subunit";
					break;
				case KeyCode::KP_LEFTPAREN:
					Name = "Left Parent";
					break;
				case KeyCode::KP_RIGHTPAREN:
					Name = "Right Paren";
					break;
				case KeyCode::KP_LEFTBRACE:
					Name = "Left Brace";
					break;
				case KeyCode::KP_RIGHTBRACE:
					Name = "Right Brace";
					break;
				case KeyCode::TAB:
				case KeyCode::KP_TAB:
					Name = "Tab";
					break;
				case KeyCode::BACKSPACE:
				case KeyCode::KP_BACKSPACE:
					Name = "Backspace";
					break;
				case KeyCode::A:
				case KeyCode::KP_A:
					Name = "A";
					break;
				case KeyCode::B:
				case KeyCode::KP_B:
					Name = "B";
					break;
				case KeyCode::C:
				case KeyCode::KP_C:
					Name = "C";
					break;
				case KeyCode::D:
				case KeyCode::KP_D:
					Name = "D";
					break;
				case KeyCode::E:
				case KeyCode::KP_E:
					Name = "E";
					break;
				case KeyCode::F:
				case KeyCode::KP_F:
					Name = "F";
					break;
				case KeyCode::KP_XOR:
					Name = "Xor";
					break;
				case KeyCode::POWER:
				case KeyCode::KP_POWER:
					Name = "Power";
					break;
				case KeyCode::KP_PERCENT:
					Name = "Percent";
					break;
				case KeyCode::KP_LESS:
					Name = "Less";
					break;
				case KeyCode::KP_GREATER:
					Name = "Greater";
					break;
				case KeyCode::KP_AMPERSAND:
					Name = "Ampersand";
					break;
				case KeyCode::KP_DBLAMPERSAND:
					Name = "DBL Ampersand";
					break;
				case KeyCode::KP_VERTICALBAR:
					Name = "Vertical Bar";
					break;
				case KeyCode::KP_DBLVERTICALBAR:
					Name = "OBL Vertical Bar";
					break;
				case KeyCode::KP_COLON:
					Name = "Colon";
					break;
				case KeyCode::KP_HASH:
					Name = "Hash";
					break;
				case KeyCode::SPACE:
				case KeyCode::KP_SPACE:
					Name = "Space";
					break;
				case KeyCode::KP_AT:
					Name = "At";
					break;
				case KeyCode::KP_EXCLAM:
					Name = "Exclam";
					break;
				case KeyCode::KP_MEMSTORE:
					Name = "Mem Store";
					break;
				case KeyCode::KP_MEMRECALL:
					Name = "Mem Recall";
					break;
				case KeyCode::KP_MEMCLEAR:
					Name = "Mem Clear";
					break;
				case KeyCode::KP_MEMADD:
					Name = "Mem Add";
					break;
				case KeyCode::KP_MEMSUBTRACT:
					Name = "Mem Subtract";
					break;
				case KeyCode::KP_MEMMULTIPLY:
					Name = "Mem Multiply";
					break;
				case KeyCode::KP_MEMDIVIDE:
					Name = "Mem Divide";
					break;
				case KeyCode::KP_PLUSMINUS:
					Name = "Plus-Minus";
					break;
				case KeyCode::CLEAR:
				case KeyCode::KP_CLEAR:
					Name = "Clear";
					break;
				case KeyCode::KP_CLEARENTRY:
					Name = "Clear Entry";
					break;
				case KeyCode::KP_BINARY:
					Name = "Binary";
					break;
				case KeyCode::KP_OCTAL:
					Name = "Octal";
					break;
				case KeyCode::KP_DECIMAL:
					Name = "Decimal";
					break;
				case KeyCode::KP_HEXADECIMAL:
					Name = "Hexadecimal";
					break;
				case KeyCode::LCTRL:
					Name = "Left CTRL";
					break;
				case KeyCode::LSHIFT:
					Name = "Left Shift";
					break;
				case KeyCode::LALT:
					Name = "Left Alt";
					break;
				case KeyCode::LGUI:
					Name = "Left GUI";
					break;
				case KeyCode::RCTRL:
					Name = "Right CTRL";
					break;
				case KeyCode::RSHIFT:
					Name = "Right Shift";
					break;
				case KeyCode::RALT:
					Name = "Right Alt";
					break;
				case KeyCode::RGUI:
					Name = "Right GUI";
					break;
				case KeyCode::MODE:
					Name = "Mode";
					break;
				case KeyCode::AUDIONEXT:
					Name = "Audio Next";
					break;
				case KeyCode::AUDIOPREV:
					Name = "Audio Prev";
					break;
				case KeyCode::AUDIOSTOP:
					Name = "Audio Stop";
					break;
				case KeyCode::AUDIOPLAY:
					Name = "Audio Play";
					break;
				case KeyCode::AUDIOMUTE:
					Name = "Audio Mute";
					break;
				case KeyCode::MEDIASELECT:
					Name = "Media Select";
					break;
				case KeyCode::WWW:
					Name = "WWW";
					break;
				case KeyCode::MAIL:
					Name = "Mail";
					break;
				case KeyCode::CALCULATOR:
					Name = "Calculator";
					break;
				case KeyCode::COMPUTER:
					Name = "Computer";
					break;
				case KeyCode::AC_SEARCH:
					Name = "AC Search";
					break;
				case KeyCode::AC_HOME:
					Name = "AC Home";
					break;
				case KeyCode::AC_BACK:
					Name = "AC Back";
					break;
				case KeyCode::AC_FORWARD:
					Name = "AC Forward";
					break;
				case KeyCode::AC_STOP:
					Name = "AC Stop";
					break;
				case KeyCode::AC_REFRESH:
					Name = "AC Refresh";
					break;
				case KeyCode::AC_BOOKMARKS:
					Name = "AC Bookmarks";
					break;
				case KeyCode::BRIGHTNESSDOWN:
					Name = "Brigthness Down";
					break;
				case KeyCode::BRIGHTNESSUP:
					Name = "Brigthness Up";
					break;
				case KeyCode::DISPLAYSWITCH:
					Name = "Display Switch";
					break;
				case KeyCode::KBDILLUMTOGGLE:
					Name = "Dillum Toggle";
					break;
				case KeyCode::KBDILLUMDOWN:
					Name = "Dillum Down";
					break;
				case KeyCode::KBDILLUMUP:
					Name = "Dillum Up";
					break;
				case KeyCode::EJECT:
					Name = "Eject";
					break;
				case KeyCode::SLEEP:
					Name = "Sleep";
					break;
				case KeyCode::APP1:
					Name = "App 1";
					break;
				case KeyCode::APP2:
					Name = "App 2";
					break;
				case KeyCode::AUDIOREWIND:
					Name = "Audio Rewind";
					break;
				case KeyCode::AUDIOFASTFORWARD:
					Name = "Audio Fast Forward";
					break;
				case KeyCode::CURSORLEFT:
					Name = "Cursor Left";
					break;
				case KeyCode::CURSORMIDDLE:
					Name = "Cursor Middle";
					break;
				case KeyCode::CURSORRIGHT:
					Name = "Cursor Right";
					break;
				case KeyCode::CURSORX1:
					Name = "Cursor X1";
					break;
				case KeyCode::CURSORX2:
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
				case KeyMod::LSHIFT:
					Name = "Left Shift";
					break;
				case KeyMod::RSHIFT:
					Name = "Right Shift";
					break;
				case KeyMod::LCTRL:
					Name = "Left Ctrl";
					break;
				case KeyMod::RCTRL:
					Name = "Right Ctrl";
					break;
				case KeyMod::LALT:
					Name = "Left Alt";
					break;
				case KeyMod::RALT:
					Name = "Right Alt";
					break;
				case KeyMod::LGUI:
					Name = "Left Gui";
					break;
				case KeyMod::RGUI:
					Name = "Right Gui";
					break;
				case KeyMod::NUM:
					Name = "Num-lock";
					break;
				case KeyMod::CAPS:
					Name = "Caps-lock";
					break;
				case KeyMod::MODE:
					Name = "Mode";
					break;
				case KeyMod::SHIFT:
					Name = "Shift";
					break;
				case KeyMod::CTRL:
					Name = "Ctrl";
					break;
				case KeyMod::ALT:
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

		Model::Model() noexcept
		{
		}
		Model::~Model() noexcept
		{
			for (auto* Item : Meshes)
				ED_RELEASE(Item);
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

		SkinModel::SkinModel() noexcept
		{
		}
		SkinModel::~SkinModel() noexcept
		{
			for (auto* Item : Meshes)
				ED_RELEASE(Item);
		}
		void SkinModel::ComputePose(PoseBuffer* Map)
		{
			ED_ASSERT_V(Map != nullptr, "pose buffer should be set");
			ED_MEASURE(ED_TIMING_ATOM);

			if (Map->Pose.empty())
				Map->SetPose(this);

			for (auto& Child : Joints)
				ComputePose(Map, &Child, Compute::Matrix4x4::Identity());
		}
		void SkinModel::ComputePose(PoseBuffer* Map, Compute::Joint* Base, const Compute::Matrix4x4& World)
		{
			ED_ASSERT_V(Map != nullptr, "pose buffer should be set");
			ED_ASSERT_V(Base != nullptr, "base should be set");

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