#include "tomahawk.h"
#include <clocale>
#ifdef TH_MICROSOFT
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
#ifdef TH_HAS_SDL2
#include <SDL2/SDL.h>
#endif
#ifdef TH_HAS_POSTGRESQL
#include <libpq-fe.h>
#endif
#ifdef TH_HAS_ASSIMP
#include <assimp/DefaultLogger.hpp>
#endif
#ifdef TH_HAS_GLEW
#include <GL/glew.h>
#endif
#ifdef TH_HAS_OPENSSL
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
}
#endif

namespace Tomahawk
{
#ifdef TH_HAS_OPENSSL
	static std::vector<std::shared_ptr<std::mutex>>* CryptoLocks = nullptr;
#endif
	static uint64_t Modes = 0;
	static int State = 0;

	void Library::Describe()
	{
		TH_INFO("tomahawk %i.%i.%i on %s (%s)"
			"\n\tfeature options"
			"\n\t\tDirectX: %s"
			"\n\t\tOpenGL: %s"
			"\n\t\tOpenSSL: %s"
			"\n\t\tGLEW: %s"
			"\n\t\tZLib: %s"
			"\n\t\tAssimp: %s"
			"\n\t\tMongoDB: %s"
			"\n\t\tPostgreSQL: %s"
			"\n\t\tOpenAL: %s"
			"\n\t\tSDL2: %s"
			"\n\tcore options",
			"\n\t\tSIMD: %s"
			"\n\t\tBullet3: %s"
			"\n\t\tRmlUI: %s"
			"\n\t\tWepoll: %s",
			TH_MAJOR_VERSION, TH_MINOR_VERSION, TH_PATCH_LEVEL, Platform(), Compiler(),
			HasDirectX() ? "ON" : "OFF",
			HasOpenGL() ? "ON" : "OFF",
			HasOpenSSL() ? "ON" : "OFF",
			HasGLEW() ? "ON" : "OFF",
			HasZLib() ? "ON" : "OFF",
			HasAssimp() ? "ON" : "OFF",
			HasMongoDB() ? "ON" : "OFF",
			HasPostgreSQL() ? "ON" : "OFF",
			HasOpenAL() ? "ON" : "OFF",
			HasSDL2() ? "ON" : "OFF",
			WithSIMD() ? "ON" : "OFF",
			WithBullet3() ? "ON" : "OFF",
			WithRmlUi() ? "ON" : "OFF",
			WithWepoll() ? "ON" : "OFF");
	}
	bool Library::HasDirectX()
	{
#ifdef TH_MICROSOFT
		return true;
#else
		return false;
#endif
	}
	bool Library::HasOpenGL()
	{
#ifdef TH_HAS_OPENGL
		return true;
#else
		return false;
#endif
	}
	bool Library::HasOpenSSL()
	{
#ifdef TH_HAS_OPENSSL
		return true;
#else
		return false;
#endif
	}
	bool Library::HasGLEW()
	{
#ifdef TH_HAS_GLEW
		return true;
#else
		return false;
#endif
	}
	bool Library::HasZLib()
	{
#ifdef TH_HAS_ZLIB
		return true;
#else
		return false;
#endif
	}
	bool Library::HasAssimp()
	{
#ifdef TH_HAS_ASSIMP
		return true;
#else
		return false;
#endif
	}
	bool Library::HasMongoDB()
	{
#ifdef TH_HAS_MONGOC
		return true;
#else
		return false;
#endif
	}
	bool Library::HasPostgreSQL()
	{
#ifdef TH_HAS_POSTGRESQL
		return true;
#else
		return false;
#endif
	}
	bool Library::HasOpenAL()
	{
#ifdef TH_HAS_OPENAL
		return true;
#else
		return false;
#endif
	}
	bool Library::HasSDL2()
	{
#ifdef TH_HAS_SDL2
		return true;
#else
		return false;
#endif
	}
	bool Library::WithSIMD()
	{
#ifdef TH_WITH_SIMD
		return true;
#else
		return false;
#endif
	}
	bool Library::WithBullet3()
	{
#ifdef TH_WITH_BULLET3
		return true;
#else
		return false;
#endif
	}
	bool Library::WithRmlUi()
	{
#ifdef TH_WITH_RMLUI
		return true;
#else
		return false;
#endif
	}
	bool Library::WithWepoll()
	{
#ifdef TH_WITH_WEPOLL
		return true;
#else
		return false;
#endif
	}
	int Library::Version()
	{
		return TH_VERSION(TH_MAJOR_VERSION, TH_MINOR_VERSION, TH_PATCH_LEVEL);
	}
	int Library::DebugLevel()
	{
		return TH_DLEVEL;
	}
	int Library::Architecture()
	{
#ifdef TH_32
		return 4;
#else
		return 8;
#endif
	}
	const char* Library::Build()
	{
#ifndef NDEBUG
		return "Debug";
#else
		return "Release";
#endif
	}
	const char* Library::Compiler()
	{
#ifdef _MSC_VER
#ifdef TH_32
		return "Visual C++ 32-bit";
#else
		return "Visual C++ 64-bit";
#endif
#endif
#ifdef __clang__
#ifdef TH_32
		return "Clang 32-bit";
#else
		return "Clang 64-bit";
#endif
#endif
#ifdef __EMSCRIPTEN__
#ifdef TH_32
		return "Emscripten 32-bit";
#else
		return "Emscripten 64-bit";
#endif
#endif
#ifdef __MINGW32__
		return "MinGW 32-bit";
#endif
#ifdef __MINGW64__
		return "MinGW 64-bit";
#endif
#ifdef __GNUC__
#ifdef TH_32
		return "GCC 32-bit";
#else
		return "GCC 64-bit";
#endif
#endif
		return "C/C++ compiler";
	}
	const char* Library::Platform()
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

	bool Initialize(uint64_t Modules)
	{
		State++;
		if (State > 1)
			return true;

		Modes = Modules;
		if (Modes & (uint64_t)Init::Core)
		{
			if (Modes & (uint64_t)Init::Debug)
				Core::Debug::AttachStream();
		}

		if (Modes & (uint64_t)Init::Network)
		{
#ifdef TH_MICROSOFT
			WSADATA WSAData;
			WORD VersionRequested = MAKEWORD(2, 2);
			if (WSAStartup(VersionRequested, &WSAData) != 0)
				TH_ERR("windows socket refs cannot be initialized");
#endif
		}

		if (Modes & (uint64_t)Init::SSL)
		{
#ifdef TH_HAS_OPENSSL
			SSL_library_init();
			SSL_load_error_strings();
			FIPS_mode_set(1);

			int Count = CRYPTO_num_locks();
			CryptoLocks = TH_NEW(std::vector<std::shared_ptr<std::mutex>>);
			CryptoLocks->reserve(Count);
			for (int i = 0; i < Count; i++)
				CryptoLocks->push_back(std::make_shared<std::mutex>());

			CRYPTO_set_id_callback([]() -> long unsigned int
			{
#ifdef TH_MICROSOFT
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
			TH_WARN("openssl ssl cannot be initialized");
#endif
		}

		if (Modes & (uint64_t)Init::SDL2)
		{
#ifdef TH_HAS_SDL2
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
				TH_ERR("[sdl2-err] %s", SDL_GetError());
#else
			TH_WARN("sdl2 cannot be initialized");
#endif
		}

		if (Modes & (uint64_t)Init::GLEW)
		{
#ifdef TH_HAS_GLEW
			glewExperimental = true;
#endif
		}

		if (Modes & (uint64_t)Init::Compute)
			Compute::Common::Randomize();

		if (Modes & (uint64_t)Init::Locale)
		{
			if (!setlocale(LC_TIME, "C"))
				TH_WARN("en-US locale cannot be initialized");
		}

		if (Modes & (uint64_t)Init::Audio)
			Audio::AudioContext::Create();

		Script::VMManager::SetMemoryFunctions(Core::Mem::Malloc, Core::Mem::Free);
#ifdef TH_HAS_OPENSSL
		if (Modes & (uint64_t)Init::SSL)
		{
			int64_t Raw = 0;
			RAND_bytes((unsigned char*)&Raw, sizeof(int64_t));
#ifdef TH_HAS_POSTGRESQL
			PQinitOpenSSL(0, 0);
#endif
		}
#endif
#ifndef TH_MICROSOFT
		signal(SIGPIPE, SIG_IGN);
#endif
		return true;
	}
	bool Uninitialize()
	{
		State--;
		if (State > 0 || State < 0)
			return State >= 0;

		Core::Schedule::Reset();
		Core::Console::Reset();

		if (Modes & (uint64_t)Init::Audio)
			Audio::AudioContext::Release();

		if (Modes & (uint64_t)Init::SSL)
		{
#ifdef TH_HAS_OPENSSL
            OPENSSL_VERSION_NUMBER;
			FIPS_mode_set(0);
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
				TH_DELETE(vector, CryptoLocks);
				CryptoLocks = nullptr;
			}
#else
			TH_WARN("openssl ssl cannot be uninitialized");
#endif
		}

		if (Modes & (uint64_t)Init::SDL2)
		{
#ifdef TH_HAS_SDL2
			SDL_Quit();
#else
			TH_WARN("sdl2 cannot be uninitialized");
#endif
		}

		if (Modes & (uint64_t)Init::Network)
		{
			Network::MDB::Driver::Release();
			Network::PDB::Driver::Release();
			Network::Driver::Release();
#ifdef TH_MICROSOFT
			WSACleanup();
#endif
		}

		Script::VMManager::FreeProxy();
		Core::Composer::Clear();

		if (Modes & (uint64_t)Init::Core)
		{
			if (Modes & (uint64_t)Init::Debug)
				Core::Debug::DetachStream();
		}
#ifdef TH_HAS_ASSIMP
		Assimp::DefaultLogger::kill();
#endif
		return true;
	}
}
