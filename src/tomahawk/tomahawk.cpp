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
#ifdef TH_HAS_ASSIMP
#include <assimp/DefaultLogger.hpp>
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
	static unsigned int Modes = 0;
	static int State = 0;

	void Library::Describe()
	{
		TH_INFO("tomahawk info"
				   "\n\tbuild version"
				   "\n\t\t%i.%i.%i on %s (%s)"
				   "\n\tfeatured with"
				   "\n\t\tDirectX: %s"
				   "\n\t\tOpenGL: %s"
				   "\n\t\tOpenSSL: %s"
				   "\n\t\tGLEW: %s"
				   "\n\t\tZLib: %s"
				   "\n\t\tAssimp: %s"
				   "\n\t\tMongoDB: %s"
				   "\n\t\tOpenAL: %s"
				   "\n\t\tSDL2: %s", TH_MAJOR_VERSION, TH_MINOR_VERSION, TH_PATCH_LEVEL, Platform(), Compiler(), HasDirectX() ? "ON" : "OFF", HasOpenGL() ? "ON" : "OFF", HasOpenSSL() ? "ON" : "OFF", HasGLEW() ? "ON" : "OFF", HasZLib() ? "ON" : "OFF", HasAssimp() ? "ON" : "OFF", HasMongoDB() ? "ON" : "OFF", HasOpenAL() ? "ON" : "OFF", HasSDL2() ? "ON" : "OFF");
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

	bool Initialize(unsigned int Modules, size_t HeapSize)
	{
		State++;
		if (State > 1)
			return State >= 0;

		if (HeapSize > 0)
			Rest::Mem::Create(HeapSize);

		Modes = Modules;
		if (Modes & TInit_Rest)
		{
			if (Modes & TInit_Logger)
				Rest::Debug::AttachStream();
		}

		if (Modes & TInit_Network)
		{
#ifdef TH_MICROSOFT
			WSADATA WSAData;
			WORD VersionRequested = MAKEWORD(2, 2);
			if (WSAStartup(VersionRequested, &WSAData) != 0)
				TH_ERROR("windows socket refs cannot be initialized");
#endif
		}

		if (Modes & TInit_Crypto)
		{
#ifdef TH_HAS_OPENSSL
			int Count = CRYPTO_num_locks();
			if (Count < 0)
				Count = 0;

			CryptoLocks = new std::vector<std::shared_ptr<std::mutex>>();
			CryptoLocks->reserve(Count);
			for (int i = 0; i < Count; i++)
				CryptoLocks->push_back(std::make_shared<std::mutex>());

			CRYPTO_set_locking_callback([](int Mode, int Id, const char* File, int Line)
			{
				if (Mode & CRYPTO_LOCK)
					CryptoLocks->at(Id)->lock();
				else
					CryptoLocks->at(Id)->unlock();
			});
			CRYPTO_set_id_callback([]() -> long unsigned int
			{
#ifdef TH_MICROSOFT
				return (unsigned long)GetCurrentThreadId();
#else
				return (unsigned long)syscall(SYS_gettid);
#endif
			});
#else
			TH_WARN("openssl crypto cannot be initialized");
#endif
		}

		if (Modes & TInit_SSL)
		{
#ifdef TH_HAS_OPENSSL
			SSL_library_init();
			SSL_load_error_strings();
#else
			TH_WARN("openssl ssl cannot be initialized");
#endif
		}

		if (Modes & TInit_SDL2)
		{
#ifdef TH_HAS_SDL2
			SDL_SetMainReady();
			SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER | SDL_INIT_HAPTIC);
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
#else
			TH_WARN("sdl2 cannot be initialized");
#endif
		}

		if (Modes & TInit_Compute)
			Compute::MathCommon::Randomize();

		if (Modes & TInit_Locale)
		{
			if (!setlocale(LC_TIME, "C"))
				TH_WARN("en-US locale cannot be initialized");
		}

		if (Modes & TInit_Audio)
			Audio::AudioContext::Create();

		Script::VMManager::SetMemoryFunctions(Rest::Mem::Malloc, Rest::Mem::Free);
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

		if (Modes & TInit_Audio)
			Audio::AudioContext::Release();

		if (Modes & TInit_Crypto)
		{
#ifdef TH_HAS_OPENSSL
			CRYPTO_set_locking_callback(nullptr);
			CRYPTO_set_id_callback(nullptr);
			CRYPTO_cleanup_all_ex_data();

			if (CryptoLocks != nullptr)
			{
				delete CryptoLocks;
				CryptoLocks = nullptr;
			}
#else
			TH_WARN("openssl ssl cannot be uninitialized");
#endif
		}

		if (Modes & TInit_SSL)
		{
#ifdef TH_HAS_OPENSSL
			ENGINE_cleanup();
			CONF_modules_unload(1);
			ERR_free_strings();
			EVP_cleanup();
#else
			TH_WARN("openssl ssl cannot be uninitialized");
#endif
		}

		if (Modes & TInit_SDL2)
		{
#ifdef TH_HAS_SDL2
			SDL_Quit();
#else
			TH_WARN("sdl2 cannot be uninitialized");
#endif
		}

		if (Modes & TInit_Network)
		{
			Network::Multiplexer::Release();
			Network::MongoDB::Connector::Release();
#ifdef TH_MICROSOFT
			WSACleanup();
#endif
		}

		Script::VMManager::FreeProxy();
		Rest::Composer::Clear();
		Rest::Mem::Release();

		if (Modes & TInit_Rest)
		{
			if (Modes & TInit_Logger)
				Rest::Debug::DetachStream();
		}
#ifdef TH_HAS_ASSIMP
		Assimp::DefaultLogger::kill();
#endif
		return true;
	}
}