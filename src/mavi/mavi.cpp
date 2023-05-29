#include "mavi.h"
#include "core/compute.h"
#include "core/audio.h"
#include "core/network.h"
#include "core/scripting.h"
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
#ifdef VI_HAS_SDL2
#include <SDL2/SDL.h>
#endif
#ifdef VI_HAS_POSTGRESQL
#include <libpq-fe.h>
#endif
#ifdef VI_HAS_ASSIMP
#include <assimp/DefaultLogger.hpp>
#endif
#ifdef VI_HAS_GLEW
#include <GL/glew.h>
#endif
#ifdef VI_HAS_OPENSSL
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
#ifdef VI_HAS_OPENSSL
    static Core::Vector<std::shared_ptr<std::mutex>>* CryptoLocks = nullptr;
#if OPENSSL_VERSION_MAJOR >= 3
    static OSSL_PROVIDER* CryptoLegacy = nullptr;
    static OSSL_PROVIDER* CryptoDefault = nullptr;
#endif
#endif
	static Core::Allocator* Allocator = nullptr;
	static uint64_t Modes = 0;
	static int State = 0;

	bool Library::HasDirectX()
	{
#ifdef VI_MICROSOFT
		return true;
#else
		return false;
#endif
	}
	bool Library::HasOpenGL()
	{
#ifdef VI_HAS_OPENGL
		return true;
#else
		return false;
#endif
	}
	bool Library::HasOpenSSL()
	{
#ifdef VI_HAS_OPENSSL
		return true;
#else
		return false;
#endif
	}
	bool Library::HasGLEW()
	{
#ifdef VI_HAS_GLEW
		return true;
#else
		return false;
#endif
	}
	bool Library::HasZLib()
	{
#ifdef VI_HAS_ZLIB
		return true;
#else
		return false;
#endif
	}
	bool Library::HasAssimp()
	{
#ifdef VI_HAS_ASSIMP
		return true;
#else
		return false;
#endif
	}
	bool Library::HasMongoDB()
	{
#ifdef VI_HAS_MONGOC
		return true;
#else
		return false;
#endif
	}
	bool Library::HasPostgreSQL()
	{
#ifdef VI_HAS_POSTGRESQL
		return true;
#else
		return false;
#endif
	}
	bool Library::HasOpenAL()
	{
#ifdef VI_HAS_OPENAL
		return true;
#else
		return false;
#endif
	}
	bool Library::HasSDL2()
	{
#ifdef VI_HAS_SDL2
		return true;
#else
		return false;
#endif
	}
	bool Library::HasSIMD()
	{
#ifdef VI_USE_SIMD
		return true;
#else
		return false;
#endif
	}
	bool Library::HasJIT()
	{
#ifdef VI_USE_JIT
		return true;
#else
		return false;
#endif
	}
	bool Library::HasBindings()
	{
#ifdef VI_HAS_BINDINGS
		return true;
#else
		return false;
#endif
	}
	bool Library::HasFastMemory()
	{
#ifdef VI_HAS_FAST_MEMORY
		return true;
#else
		return false;
#endif
	}
	bool Library::HasBullet3()
	{
#ifdef VI_USE_BULLET3
		return true;
#else
		return false;
#endif
	}
	bool Library::HasFreeType()
	{
#ifdef VI_HAS_FREETYPE
		return true;
#else
		return false;
#endif
	}
	bool Library::HasSPIRV()
	{
#ifdef VI_HAS_SPIRV
		return true;
#else
		return false;
#endif
	}
	bool Library::HasRmlUI()
	{
#ifdef VI_USE_RMLUI
		return true;
#else
		return false;
#endif
	}
    bool Library::HasFContext()
    {
    #ifdef VI_USE_FCTX
        return true;
    #else
        return false;
    #endif
    }
	bool Library::HasWindowsEpoll()
	{
#ifdef VI_USE_WEPOLL
		return true;
#else
		return false;
#endif
	}
	bool Library::HasShaders()
	{
		return HasSPIRV();
	}
	int Library::GetMajorVersion()
	{
		return (int)MAJOR_VERSION;
	}
	int Library::GetMinorVersion()
	{
		return (int)MINOR_VERSION;
	}
	int Library::GetPatchVersion()
	{
		return (int)PATCH_VERSION;
	}
	int Library::GetVersion()
	{
		return (int)VERSION;
	}
	int Library::GetDebugLevel()
	{
		return VI_DLEVEL;
	}
	int Library::GetArchitecture()
	{
		return (int)sizeof(size_t);
	}
	Core::String Library::GetDetails()
	{
		Core::Vector<Core::String> Features;
		if (HasDirectX())
			Features.push_back("DirectX");
		if (HasOpenGL())
			Features.push_back("OpenGL");
		if (HasOpenAL())
			Features.push_back("OpenAL");
		if (HasOpenSSL())
			Features.push_back("OpenSSL");
		if (HasGLEW())
			Features.push_back("GLEW");
		if (HasZLib())
			Features.push_back("ZLib");
		if (HasAssimp())
			Features.push_back("Assimp");
		if (HasMongoDB())
			Features.push_back("MongoDB");
		if (HasPostgreSQL())
			Features.push_back("PostgreSQL");
		if (HasSDL2())
			Features.push_back("SDL2");
		if (HasSIMD())
			Features.push_back("SIMD");
		if (HasJIT())
			Features.push_back("JIT");
		if (HasBindings())
			Features.push_back("Bindings");
		if (HasFastMemory())
			Features.push_back("FastMemory");
		if (HasBullet3())
			Features.push_back("Bullet3");
		if (HasFreeType())
			Features.push_back("FreeType");
		if (HasSPIRV())
			Features.push_back("SPIRV");
		if (HasRmlUI())
			Features.push_back("RmlUI");
		if (HasFContext())
			Features.push_back("FContext");
		if (HasWindowsEpoll())
			Features.push_back("Wepoll");
		if (HasShaders())
			Features.push_back("Shaders");

		Core::StringStream Result;
		Result << "version: " << MAJOR_VERSION << "." << MINOR_VERSION << "." << PATCH_VERSION << " / " << VERSION << "\n";
		Result << "platform: " << GetPlatform() << " / " << GetBuild() << "\n";
		Result << "compiler: " << GetCompiler() << "\n";
		Result << "features: ";

		for (size_t i = 0; i < Features.size(); i++)
		{
			Result << Features[i];
			if (i < Features.size() - 1)
				Result << ", ";
		}

		return Result.str();
	}
	const char* Library::GetBuild()
	{
#ifndef NDEBUG
		return "Debug";
#else
		return "Release";
#endif
	}
	const char* Library::GetCompiler()
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
	const char* Library::GetPlatform()
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
	Core::Allocator* Library::GetAllocator()
	{
#ifndef NDEBUG
		if (!Allocator)
			Allocator = new Core::DebugAllocator();
#else
#ifndef VI_HAS_FAST_MEMORY
		if (!Allocator)
			Allocator = new Core::DefaultAllocator();
#else
		if (!Allocator)
			Allocator = new Core::PoolAllocator();
#endif
#endif
		return Allocator;
	}

	bool Initialize(size_t Modules, Core::Allocator* Allocator)
	{
		State++;
		if (State > 1)
			return true;

		Core::Memory::SetAllocator(Allocator);
		Modes = Modules;

		if (Modes & (uint64_t)Init::Core)
		{
			if (Modes & (uint64_t)Init::Debug)
				Core::OS::SetLogFlag(Core::LogOption::Active, true);
		}

		if (Modes & (uint64_t)Init::Network)
		{
#ifdef VI_MICROSOFT
			WSADATA WSAData;
			WORD VersionRequested = MAKEWORD(2, 2);
			if (WSAStartup(VersionRequested, &WSAData) != 0)
				VI_ERR("[mavi] windows socket refs cannot be initialized");
#endif
		}

		if (Modes & (uint64_t)Init::SSL)
		{
#ifdef VI_HAS_OPENSSL
			SSL_library_init();
			SSL_load_error_strings();
#if OPENSSL_VERSION_MAJOR >= 3
			CryptoDefault = OSSL_PROVIDER_load(nullptr, "default");
            CryptoLegacy = OSSL_PROVIDER_load(nullptr, "legacy");
            
			if (!CryptoLegacy || !CryptoDefault)
			{
				Core::String Path = Core::OS::Directory::GetModule();
				bool IsModuleDirectory = true;
			Retry:
				OSSL_PROVIDER_set_default_search_path(nullptr, Path.c_str());

				if (!CryptoDefault)
					CryptoDefault = OSSL_PROVIDER_load(nullptr, "default");

				if (!CryptoLegacy)
					CryptoLegacy = OSSL_PROVIDER_load(nullptr, "legacy");

				if (!CryptoLegacy || !CryptoDefault)
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
#else
            FIPS_mode_set(1);
#endif
			RAND_poll();

			int Count = CRYPTO_num_locks();
			CryptoLocks = VI_NEW(Core::Vector<std::shared_ptr<std::mutex>>);
			CryptoLocks->reserve(Count);
			for (int i = 0; i < Count; i++)
				CryptoLocks->push_back(std::make_shared<std::mutex>());

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
					CryptoLocks->at(Id)->lock();
				else
					CryptoLocks->at(Id)->unlock();
			});
#else
			VI_WARN("[mavi] openssl ssl cannot be initialized");
#endif
		}
        
		if (Modes & (uint64_t)Init::SDL2)
		{
#ifdef VI_HAS_SDL2
			SDL_SetMainReady();
			if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER | SDL_INIT_HAPTIC) == 0)
			{
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
			}
			else
				VI_ERR("[mavi] %s", SDL_GetError());
#else
			VI_WARN("[mavi] sdl2 cannot be initialized");
#endif
		}

		if (Modes & (uint64_t)Init::GLEW)
		{
#ifdef VI_HAS_GLEW
			glewExperimental = true;
#endif
		}

		if (Modes & (uint64_t)Init::Locale)
		{
			if (!setlocale(LC_TIME, "C"))
				VI_WARN("[mavi] en-US locale cannot be initialized");
		}

		if (Modes & (uint64_t)Init::Audio)
			Audio::AudioContext::Initialize();

		Scripting::VirtualMachine::SetMemoryFunctions(Core::Memory::Malloc, Core::Memory::Free);
#ifdef VI_HAS_OPENSSL
		if (Modes & (uint64_t)Init::SSL)
		{
			int64_t Raw = 0;
			RAND_bytes((unsigned char*)&Raw, sizeof(int64_t));
#ifdef VI_HAS_POSTGRESQL
			PQinitOpenSSL(0, 0);
#endif
		}
#endif
#ifndef VI_MICROSOFT
		signal(SIGPIPE, SIG_IGN);
#endif
		return true;
	}
	bool Uninitialize()
	{
		State--;
		if (State > 0 || State < 0)
			return State >= 0;

		auto* Manager = Core::Memory::GetAllocator();
		Core::OS::SetLogFlag(Core::LogOption::Async, false);
		Core::Schedule::Reset();

		if (Modes & (uint64_t)Init::SSL)
		{
#ifdef VI_HAS_OPENSSL
#if OPENSSL_VERSION_MAJOR >= 3
            OSSL_PROVIDER_unload(CryptoLegacy);
            OSSL_PROVIDER_unload(CryptoDefault);
            CryptoLegacy = nullptr;
            CryptoDefault = nullptr;
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
#ifdef SSL_COMP_free_compression_methods
			SSL_COMP_free_compression_methods();
#endif
			ENGINE_cleanup();
			CONF_modules_free();
			CONF_modules_unload(1);
			COMP_zlib_cleanup();
			ERR_free_strings();
			EVP_cleanup();
			CRYPTO_cleanup_all_ex_data();
            
			if (CryptoLocks != nullptr)
			{
				VI_DELETE(vector, CryptoLocks);
				CryptoLocks = nullptr;
			}
#else
			VI_WARN("[mavi] openssl ssl cannot be uninitialized");
#endif
		}

		if (Modes & (uint64_t)Init::SDL2)
		{
#ifdef VI_HAS_SDL2
			SDL_Quit();
#else
			VI_WARN("[mavi] sdl2 cannot be uninitialized");
#endif
		}

		if (Modes & (uint64_t)Init::Network)
		{
			Network::MDB::Driver::Release();
			Network::PDB::Driver::Release();
			Network::Multiplexer::Release();
#ifdef VI_MICROSOFT
			WSACleanup();
#endif
		}

		Scripting::VirtualMachine::FreeProxy();
		Core::Composer::Clear();

		if (Modes & (uint64_t)Init::Core)
		{
			if (Modes & (uint64_t)Init::Debug)
				Core::OS::SetLogFlag(Core::LogOption::Active, false);
		}
#ifdef VI_HAS_ASSIMP
		Assimp::DefaultLogger::kill();
#endif
		Core::Console::Reset();
		Core::Memory::SetAllocator(nullptr);

		if (Manager != nullptr && Manager == Allocator && Allocator->IsFinalizable())
		{
			delete Allocator;
			Allocator = nullptr;
		}

		return true;
	}
}
