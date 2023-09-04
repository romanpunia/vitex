#include "mavi.h"
#include "core/compute.h"
#include "core/audio.h"
#include "core/network.h"
#include "core/scripting.h"
#include "engine/gui.h"
#include "network/pdb.h"
#include "network/mdb.h"
#include <clocale>
#ifdef VI_MICROSOFT
#include <WS2tcpip.h>
#include <io.h>
#else
#include <arpa/inet.h>
#include <cstring>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/syscall.h>
#endif
#ifdef VI_SDL2
#include <SDL2/SDL.h>
#endif
#ifdef VI_POSTGRESQL
#include <libpq-fe.h>
#endif
#ifdef VI_ASSIMP
#include <assimp/DefaultLogger.hpp>
#endif
#ifdef VI_GLEW
#include <GL/glew.h>
#endif
#ifdef VI_OPENSSL
extern "C"
{
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/crypto.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/engine.h>
#include <openssl/conf.h>
#include <openssl/dh.h>
#if OPENSSL_VERSION_MAJOR >= 3
#include <openssl/provider.h>
#endif
}
#endif

namespace Mavi
{
	Runtime::Runtime(size_t Modules, Core::GlobalAllocator* Allocator) noexcept : Crypto(nullptr), Modes(Modules)
	{
		VI_ASSERT(!Instance, "mavi runtime should not be already initialized");
		Instance = this;
#ifndef NDEBUG
		Core::ErrorHandling::SetFlag(Core::LogOption::Active, true);
		if (!Allocator)
			Allocator = new Core::Allocators::DebugAllocator();
#else
		if (!Allocator)
			Allocator = new Core::Allocators::DefaultAllocator();
#endif
		Core::Memory::SetGlobalAllocator(Allocator);
		if (Modes & (uint64_t)Init::Network)
		{
#ifdef VI_MICROSOFT
			WSADATA WSAData;
			WORD VersionRequested = MAKEWORD(2, 2);
			int Code = WSAStartup(VersionRequested, &WSAData);
			VI_PANIC(Code == 0, "WSA initialization failure reason:%i", Code);
#endif
		}

		if (Modes & (uint64_t)Init::SSL)
		{
#ifdef VI_OPENSSL
			SSL_library_init();
			SSL_load_error_strings();
#if OPENSSL_VERSION_MAJOR >= 3
			Crypto = VI_NEW(CryptoData);
			if (Modes & (uint64_t)Init::Providers)
			{
				Crypto->DefaultProvider = OSSL_PROVIDER_load(nullptr, "default");
				Crypto->LegacyProvider = OSSL_PROVIDER_load(nullptr, "legacy");
				if (!Crypto->LegacyProvider || !Crypto->DefaultProvider)
				{
					auto Path = Core::OS::Directory::GetModule();
					bool IsModuleDirectory = true;
				Retry:
					if (Path)
						OSSL_PROVIDER_set_default_search_path(nullptr, Path->c_str());

					if (!Crypto->DefaultProvider)
						Crypto->DefaultProvider = OSSL_PROVIDER_load(nullptr, "default");

					if (!Crypto->LegacyProvider)
						Crypto->LegacyProvider = OSSL_PROVIDER_load(nullptr, "legacy");

					if (!Crypto->LegacyProvider || !Crypto->DefaultProvider)
					{
						if (IsModuleDirectory)
						{
							Path = Core::OS::Directory::GetWorking();
							IsModuleDirectory = false;
							goto Retry;
						}
						Compute::Crypto::DisplayCryptoLog();
					}
					else
						ERR_clear_error();
				}
			}
#else
			FIPS_mode_set(1);
#endif
			RAND_poll();

			int Count = CRYPTO_num_locks();
			Crypto->Locks.reserve(Count);
			for (int i = 0; i < Count; i++)
				Crypto->Locks.push_back(std::make_shared<std::mutex>());

			CRYPTO_set_id_callback([]() -> long unsigned int
			{
#ifdef VI_MICROSOFT
				return (unsigned long)GetCurrentThreadId();
#else
				return (unsigned long)syscall(SYS_gettid);
#endif
			});
			CRYPTO_set_locking_callback([](int Mode, int Id, const char* File, int Line)
			{
				if (Mode & CRYPTO_LOCK)
					Crypto->Locks.at(Id)->lock();
				else
					Crypto->Locks.at(Id)->unlock();
			});
#else
			VI_WARN("[mavi] openssl ssl cannot be initialized");
#endif
		}

		if (Modes & (uint64_t)Init::SDL2)
		{
#ifdef VI_SDL2
			SDL_SetMainReady();
			int Code = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER | SDL_INIT_HAPTIC);
			VI_PANIC(Code == 0, "SDL2 initialization failure reason:%i", Code);
			SDL_EventState(SDL_QUIT, SDL_ENABLE);
			SDL_EventState(SDL_APP_TERMINATING, SDL_ENABLE);
			SDL_EventState(SDL_APP_LOWMEMORY, SDL_ENABLE);
			SDL_EventState(SDL_APP_WILLENTERBACKGROUND, SDL_ENABLE);
			SDL_EventState(SDL_APP_DIDENTERBACKGROUND, SDL_ENABLE);
			SDL_EventState(SDL_APP_WILLENTERFOREGROUND, SDL_ENABLE);
			SDL_EventState(SDL_APP_DIDENTERFOREGROUND, SDL_ENABLE);
			SDL_EventState(SDL_APP_DIDENTERFOREGROUND, SDL_ENABLE);
			SDL_EventState(SDL_WINDOWEVENT, SDL_ENABLE);
			SDL_EventState(SDL_SYSWMEVENT, SDL_DISABLE);
			SDL_EventState(SDL_KEYDOWN, SDL_ENABLE);
			SDL_EventState(SDL_KEYUP, SDL_ENABLE);
			SDL_EventState(SDL_TEXTEDITING, SDL_ENABLE);
			SDL_EventState(SDL_TEXTINPUT, SDL_ENABLE);
#if SDL_VERSION_ATLEAST(2, 0, 4)
			SDL_EventState(SDL_KEYMAPCHANGED, SDL_DISABLE);
#endif
			SDL_EventState(SDL_MOUSEMOTION, SDL_ENABLE);
			SDL_EventState(SDL_MOUSEBUTTONDOWN, SDL_ENABLE);
			SDL_EventState(SDL_MOUSEBUTTONUP, SDL_ENABLE);
			SDL_EventState(SDL_MOUSEWHEEL, SDL_ENABLE);
			SDL_EventState(SDL_JOYAXISMOTION, SDL_ENABLE);
			SDL_EventState(SDL_JOYBALLMOTION, SDL_ENABLE);
			SDL_EventState(SDL_JOYHATMOTION, SDL_ENABLE);
			SDL_EventState(SDL_JOYBUTTONDOWN, SDL_ENABLE);
			SDL_EventState(SDL_JOYBUTTONUP, SDL_ENABLE);
			SDL_EventState(SDL_JOYDEVICEADDED, SDL_ENABLE);
			SDL_EventState(SDL_JOYDEVICEREMOVED, SDL_ENABLE);
			SDL_EventState(SDL_CONTROLLERAXISMOTION, SDL_ENABLE);
			SDL_EventState(SDL_CONTROLLERBUTTONDOWN, SDL_ENABLE);
			SDL_EventState(SDL_CONTROLLERBUTTONUP, SDL_ENABLE);
			SDL_EventState(SDL_CONTROLLERDEVICEADDED, SDL_ENABLE);
			SDL_EventState(SDL_CONTROLLERDEVICEREMOVED, SDL_ENABLE);
			SDL_EventState(SDL_CONTROLLERDEVICEREMAPPED, SDL_ENABLE);
			SDL_EventState(SDL_FINGERDOWN, SDL_ENABLE);
			SDL_EventState(SDL_FINGERUP, SDL_ENABLE);
			SDL_EventState(SDL_FINGERMOTION, SDL_ENABLE);
			SDL_EventState(SDL_DOLLARGESTURE, SDL_ENABLE);
			SDL_EventState(SDL_DOLLARRECORD, SDL_ENABLE);
			SDL_EventState(SDL_MULTIGESTURE, SDL_ENABLE);
			SDL_EventState(SDL_CLIPBOARDUPDATE, SDL_DISABLE);
#if SDL_VERSION_ATLEAST(2, 0, 5)
			SDL_EventState(SDL_DROPFILE, SDL_ENABLE);
			SDL_EventState(SDL_DROPTEXT, SDL_ENABLE);
			SDL_EventState(SDL_DROPBEGIN, SDL_ENABLE);
			SDL_EventState(SDL_DROPCOMPLETE, SDL_ENABLE);
#endif
#if SDL_VERSION_ATLEAST(2, 0, 4)
			SDL_EventState(SDL_AUDIODEVICEADDED, SDL_DISABLE);
			SDL_EventState(SDL_AUDIODEVICEREMOVED, SDL_DISABLE);
			SDL_EventState(SDL_RENDER_DEVICE_RESET, SDL_DISABLE);
#endif
			SDL_EventState(SDL_RENDER_TARGETS_RESET, SDL_DISABLE);
			SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
			SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
			SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
			SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
			SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
			SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
			SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

			const char* Platform = SDL_GetPlatform();
			if (!strcmp(Platform, "iOS") || !strcmp(Platform, "Android") || !strcmp(Platform, "Unknown"))
				SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
			else
				SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#else
			VI_WARN("[mavi] sdl2 cannot be initialized");
#endif
		}

		if (Modes & (uint64_t)Init::GLEW)
		{
#ifdef VI_GLEW
			glewExperimental = true;
#endif
		}

		if (Modes & (uint64_t)Init::Locale)
			setlocale(LC_TIME, "C");

		if (Modes & (uint64_t)Init::SSL)
		{
#ifdef VI_OPENSSL
			int64_t Raw = 0;
			RAND_bytes((unsigned char*)&Raw, sizeof(int64_t));
#ifdef VI_POSTGRESQL
			PQinitOpenSSL(0, 0);
#endif
#endif
		}

		if (Modes & (uint64_t)Init::Audio)
			Audio::AudioContext::Initialize();

		Scripting::VirtualMachine::SetMemoryFunctions(Core::Memory::Malloc, Core::Memory::Free);
#ifndef VI_MICROSOFT
		signal(SIGPIPE, SIG_IGN);
#endif
	}
	Runtime::~Runtime() noexcept
	{
		auto* Allocator = Core::Memory::GetGlobalAllocator();
		Core::ErrorHandling::SetFlag(Core::LogOption::Async, false);
		Network::MDB::Driver::CleanupInstance();
		Network::PDB::Driver::CleanupInstance();
		Network::Uplinks::CleanupInstance();
		Network::Multiplexer::CleanupInstance();
		Network::DNS::CleanupInstance();
		Engine::Application::CleanupInstance();
		Engine::GUI::Subsystem::CleanupInstance();
		Core::Schedule::CleanupInstance();
		Core::Console::CleanupInstance();
#ifdef VI_OPENSSL
		if (Modes & (uint64_t)Init::SSL)
		{
#if OPENSSL_VERSION_MAJOR >= 3
			OSSL_PROVIDER_unload((OSSL_PROVIDER*)Crypto->LegacyProvider);
			OSSL_PROVIDER_unload((OSSL_PROVIDER*)Crypto->DefaultProvider);
#else
			FIPS_mode_set(0);
#endif
			CRYPTO_set_locking_callback(nullptr);
			CRYPTO_set_id_callback(nullptr);
#if OPENSSL_VERSION_NUMBER >= 0x10000000L && OPENSSL_VERSION_NUMBER < 0x10100000L
			ERR_remove_thread_state(NULL);
#elif OPENSSL_VERSION_NUMBER < 0x10000000L
			ERR_remove_state(0);
#endif
#if !defined(OPENSSL_NO_DEPRECATED_1_1_0) && OPENSSL_VERSION_MAJOR < 3
			SSL_COMP_free_compression_methods();
			ENGINE_cleanup();
			CONF_modules_free();
			COMP_zlib_cleanup();
			ERR_free_strings();
			EVP_cleanup();
			CRYPTO_cleanup_all_ex_data();
			CONF_modules_unload(1);
#endif
			VI_DELETE(CryptoData, Crypto);
			Crypto = nullptr;
		}
#endif
#ifdef VI_SDL2
		if (Modes & (uint64_t)Init::SDL2)
			SDL_Quit();
#endif
#ifdef VI_MICROSOFT
		if (Modes & (uint64_t)Init::Network)
			WSACleanup();
#endif
#ifdef VI_ASSIMP
		Assimp::DefaultLogger::kill();
#endif
		Scripting::VirtualMachine::Cleanup();
		Core::Composer::Cleanup();
		Core::ErrorHandling::Cleanup();
		Core::Memory::Cleanup();

		if (Allocator != nullptr && Allocator->IsFinalizable())
			delete Allocator;

		if (Instance == this)
			Instance = nullptr;
	}
	bool Runtime::HasDirectX() const noexcept
	{
#ifdef VI_MICROSOFT
		return true;
#else
		return false;
#endif
	}
	bool Runtime::HasOpenGL() const noexcept
	{
#ifdef VI_OPENGL
		return true;
#else
		return false;
#endif
	}
	bool Runtime::HasOpenSSL() const noexcept
	{
#ifdef VI_OPENSSL
		return true;
#else
		return false;
#endif
	}
	bool Runtime::HasGLEW() const noexcept
	{
#ifdef VI_GLEW
		return true;
#else
		return false;
#endif
	}
	bool Runtime::HasZLib() const noexcept
	{
#ifdef VI_ZLIB
		return true;
#else
		return false;
#endif
	}
	bool Runtime::HasAssimp() const noexcept
	{
#ifdef VI_ASSIMP
		return true;
#else
		return false;
#endif
	}
	bool Runtime::HasMongoDB() const noexcept
	{
#ifdef VI_MONGOC
		return true;
#else
		return false;
#endif
	}
	bool Runtime::HasPostgreSQL() const noexcept
	{
#ifdef VI_POSTGRESQL
		return true;
#else
		return false;
#endif
	}
	bool Runtime::HasOpenAL() const noexcept
	{
#ifdef VI_OPENAL
		return true;
#else
		return false;
#endif
	}
	bool Runtime::HasSDL2() const noexcept
	{
#ifdef VI_SDL2
		return true;
#else
		return false;
#endif
	}
	bool Runtime::HasSIMD() const noexcept
	{
#ifdef VI_SIMD
		return true;
#else
		return false;
#endif
	}
	bool Runtime::HasJIT() const noexcept
	{
#ifdef VI_JIT
		return true;
#else
		return false;
#endif
	}
	bool Runtime::HasBindings() const noexcept
	{
#ifdef VI_BINDINGS
		return true;
#else
		return false;
#endif
	}
	bool Runtime::HasAllocator() const noexcept
	{
#ifdef VI_ALLOCATOR
		return true;
#else
		return false;
#endif
	}
	bool Runtime::HasBacktrace() const noexcept
	{
#ifdef VI_BACKTRACE
		return true;
#else
		return false;
#endif
	}
	bool Runtime::HasBullet3() const noexcept
	{
#ifdef VI_BULLET3
		return true;
#else
		return false;
#endif
	}
	bool Runtime::HasFreeType() const noexcept
	{
#ifdef VI_FREETYPE
		return true;
#else
		return false;
#endif
	}
	bool Runtime::HasSPIRV() const noexcept
	{
#ifdef VI_SPIRV
		return true;
#else
		return false;
#endif
	}
	bool Runtime::HasRmlUI() const noexcept
	{
#ifdef VI_RMLUI
		return true;
#else
		return false;
#endif
	}
    bool Runtime::HasFContext() const noexcept
    {
    #ifdef VI_FCTX
        return true;
    #else
        return false;
    #endif
    }
	bool Runtime::HasWindowsEpoll() const noexcept
	{
#ifdef VI_WEPOLL
		return true;
#else
		return false;
#endif
	}
	bool Runtime::HasTinyFileDialogs() const noexcept
	{
#ifdef VI_TINYFILEDIALOGS
		return true;
#else
		return false;
#endif
	}
	bool Runtime::HasSTB() const noexcept
	{
#ifdef VI_STB
		return true;
#else
		return false;
#endif
	}
	bool Runtime::HasPugiXML() const noexcept
	{
#ifdef VI_PUGIXML
		return true;
#else
		return false;
#endif
	}
	bool Runtime::HasRapidJSON() const noexcept
	{
#ifdef VI_RAPIDJSON
		return true;
#else
		return false;
#endif
	}
	bool Runtime::HasShaders() const noexcept
	{
		return HasSPIRV();
	}
	int Runtime::GetMajorVersion() const noexcept
	{
		return (int)MAJOR_VERSION;
	}
	int Runtime::GetMinorVersion() const noexcept
	{
		return (int)MINOR_VERSION;
	}
	int Runtime::GetPatchVersion() const noexcept
	{
		return (int)PATCH_VERSION;
	}
	int Runtime::GetBuildVersion() const noexcept
	{
		return (int)PATCH_VERSION;
	}
	int Runtime::GetVersion() const noexcept
	{
		return (int)VERSION;
	}
	int Runtime::GetDebugLevel() const noexcept
	{
		return VI_DLEVEL;
	}
	int Runtime::GetArchitecture() const noexcept
	{
		return (int)sizeof(size_t);
	}
	size_t Runtime::GetModes() const noexcept
	{
		return Modes;
	}
	Core::String Runtime::GetDetails() const noexcept
	{
		Core::Vector<Core::String> Features;
		if (HasDirectX())
			Features.push_back("so:d3d11");
		if (HasOpenGL())
			Features.push_back("so:opengl");
		if (HasOpenAL())
			Features.push_back("so:openal");
		if (HasOpenSSL())
			Features.push_back("so:openssl");
		if (HasGLEW())
			Features.push_back("so:glew");
		if (HasZLib())
			Features.push_back("so:zlib");
		if (HasAssimp())
			Features.push_back("so:assimp");
		if (HasMongoDB())
			Features.push_back("so:mongoc");
		if (HasPostgreSQL())
			Features.push_back("so:pq");
		if (HasSDL2())
			Features.push_back("so:sdl2");
		if (HasFreeType())
			Features.push_back("so:freetype");
		if (HasSPIRV())
			Features.push_back("so:spirv");
		if (HasRmlUI())
			Features.push_back("lib:rmlui");
		if (HasFContext())
			Features.push_back("lib:fcontext");
		if (HasWindowsEpoll())
			Features.push_back("lib:wepoll");
		if (HasBullet3())
			Features.push_back("lib:bullet3");
		if (HasBacktrace())
			Features.push_back("lib:backward-cxx");
		if (HasSIMD())
			Features.push_back("feature:simd");
		if (HasJIT())
			Features.push_back("feature:as-jit");
		if (HasBindings())
			Features.push_back("feature:as-stdlib");
		if (HasAllocator())
			Features.push_back("feature:cxx-stdalloc");
		if (HasShaders())
			Features.push_back("feature:shaders");

		Core::StringStream Result;
		Result << "library: " << MAJOR_VERSION << "." << MINOR_VERSION << "." << PATCH_VERSION << "." << BUILD_VERSION << " / " << VERSION << "\n";
		Result << "  platform: " << GetPlatform() << " / " << GetBuild() << "\n";
		Result << "  compiler: " << GetCompiler() << "\n";
        Result << "configuration:" << "\n";
        
		for (size_t i = 0; i < Features.size(); i++)
			Result << "  " << Features[i] << "\n";

		return Result.str();
	}
	const char* Runtime::GetBuild() const noexcept
	{
#ifndef NDEBUG
		return "Debug";
#else
		return "Release";
#endif
	}
	const char* Runtime::GetCompiler() const noexcept
	{
#ifdef _MSC_VER
#ifdef VI_64
		return "Visual C++ 64-bit";
#else
		return "Visual C++ 32-bit";
#endif
#endif
#ifdef __clang__
#ifdef VI_64
		return "Clang 64-bit";
#else
		return "Clang 32-bit";
#endif
#endif
#ifdef __EMSCRIPTEN__
#ifdef VI_64
		return "Emscripten 64-bit";
#else
		return "Emscripten 32-bit";
#endif
#endif
#ifdef __MINGW32__
		return "MinGW 32-bit";
#endif
#ifdef __MINGW64__
		return "MinGW 64-bit";
#endif
#ifdef __GNUC__
#ifdef VI_64
		return "GCC 64-bit";
#else
		return "GCC 32-bit";
#endif
#endif
		return "C/C++ compiler";
	}
	const char* Runtime::GetPlatform() const noexcept
	{
#ifdef __ANDROID__
		return "Android";
#endif
#ifdef __linux__
		return "Linux";
#endif
#ifdef __APPLE__
		return "Darwin";
#endif
#ifdef __ros__
		return "Akaros";
#endif
#ifdef _WIN32
		return "Win32";
#endif
#ifdef _WIN64
		return "Win64";
#endif
#ifdef __native_client__
		return "NaCL";
#endif
#ifdef __asmjs__
		return "AsmJS";
#endif
#ifdef __Fuchsia__
		return "Fuschia";
#endif
		return "OS with C/C++ support";
	}
	Runtime* Runtime::Get() noexcept
	{
		return Instance;
	}
	Runtime* Runtime::Instance = nullptr;
}
