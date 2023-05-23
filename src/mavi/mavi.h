#ifndef MAVI_H
#define MAVI_H
#include "core/core.h"

namespace Mavi
{
	enum
	{
		MAJOR_VERSION = 21,
		MINOR_VERSION = 30,
		PATCH_VERSION = 17,
		VERSION = (MAJOR_VERSION) * 10000 + (MINOR_VERSION) * 100 + (PATCH_VERSION)
	};

	enum class Init
	{
		Core = 1,
		Debug = 2,
		Network = 4,
		SSL = 8,
		SDL2 = 16,
		Compute = 32,
		Locale = 64,
		Audio = 128,
		GLEW = 256
	};

	constexpr inline Init operator |(Init A, Init B)
	{
		return static_cast<Init>(static_cast<size_t>(A) | static_cast<size_t>(B));
	}

	enum class Preset : size_t
	{
		Game = (size_t)(Init::Core | Init::Debug | Init::Network | Init::SSL | Init::Compute | Init::Locale | Init::SDL2 | Init::Audio | Init::GLEW),
		App = (size_t)(Init::Core | Init::Debug | Init::Network | Init::SSL | Init::Compute | Init::Locale)
	};

	class VI_OUT_TS Library
	{
	public:
		static bool HasDirectX();
		static bool HasOpenGL();
		static bool HasOpenSSL();
		static bool HasGLEW();
		static bool HasZLib();
		static bool HasAssimp();
		static bool HasMongoDB();
		static bool HasPostgreSQL();
		static bool HasOpenAL();
		static bool HasSDL2();
		static bool HasSIMD();
		static bool HasJIT();
		static bool HasBindings();
		static bool HasFastMemory();
		static bool HasFreeType();
		static bool HasSPIRV();
        static bool HasRmlUI();
		static bool HasBullet3();
        static bool HasFContext();
		static bool HasWindowsEpoll();
		static bool HasShaders();
		static int GetMajorVersion();
		static int GetMinorVersion();
		static int GetPatchVersion();
		static int GetVersion();
		static int GetDebugLevel();
		static int GetArchitecture();
		static Core::String GetDetails();
		static const char* GetBuild();
		static const char* GetCompiler();
		static const char* GetPlatform();
		static Core::Allocator* GetAllocator();
	};

	VI_OUT bool Initialize(size_t Modules = (size_t)Preset::App, Core::Allocator* Allocator = Library::GetAllocator());
	VI_OUT bool Uninitialize();
}
#endif
