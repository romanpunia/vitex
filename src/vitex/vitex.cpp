#include "vitex.h"
#include "compute.h"
#include "audio.h"
#include "network.h"
#include "scripting.h"
#include "engine/gui.h"
#include "network/http.h"
#include "network/ldb.h"
#include "network/pdb.h"
#include "network/mdb.h"
#include <clocale>
#include <sstream>
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
#include "internal/sdl2.hpp"
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

namespace Vitex
{
	Runtime::Runtime(size_t Modules, Core::GlobalAllocator* Allocator) noexcept : Crypto(nullptr), Modes(Modules)
	{
		VI_ASSERT(!Instance, "vitex runtime should not be already initialized");
		Instance = this;

		InitializeAllocator(&Allocator);
		if (Modes & (uint64_t)Init::Network)
			InitializeNetwork();

		if (Modes & (uint64_t)Init::Cryptography)
			InitializeCryptography(&Crypto, Modes & (uint64_t)Init::Providers);

		if (Modes & (uint64_t)Init::Platform)
			InitializePlatform();

		if (Modes & (uint64_t)Init::Graphics)
			InitializeGraphics();

		if (Modes & (uint64_t)Init::Locale)
			InitializeLocale();

		if (Modes & (uint64_t)Init::Cryptography)
			InitializeRandom();

		if (Modes & (uint64_t)Init::Audio)
			InitializeAudio();

		InitializeScripting();
#ifndef VI_MICROSOFT
		signal(SIGPIPE, SIG_IGN);
#endif
	}
	Runtime::~Runtime() noexcept
	{
		auto* Allocator = Core::Memory::GetGlobalAllocator();
		Core::ErrorHandling::SetFlag(Core::LogOption::Async, false);
		CleanupInstances();
		if (Modes & (uint64_t)Init::Cryptography)
			CleanupCryptography(&Crypto);

		if (Modes & (uint64_t)Init::Platform)
			CleanupPlatform();

		if (Modes & (uint64_t)Init::Network)
			CleanupNetwork();
		CleanupImporter();
		CleanupScripting();
		CleanupComposer();
		CleanupErrorHandling();
		CleanupMemory();
		CleanupAllocator(&Allocator);
		if (Instance == this)
			Instance = nullptr;
	}
	bool Runtime::InitializeAllocator(Core::GlobalAllocator** InoutAllocator) noexcept
	{
		if (!InoutAllocator)
			return false;
#ifndef NDEBUG
		Core::ErrorHandling::SetFlag(Core::LogOption::Active, true);
		if (!*InoutAllocator)
		{
			*InoutAllocator = new Core::Allocators::DebugAllocator();
			VI_TRACE("[lib] initialize global debug allocator");
		}
#else
		if (!*InoutAllocator)
		{
			*InoutAllocator = new Core::Allocators::DefaultAllocator();
			VI_TRACE("[lib] initialize global default allocator");
		}
#endif
		Core::Memory::SetGlobalAllocator(*InoutAllocator);
		return *InoutAllocator != nullptr;
	}
	bool Runtime::InitializeNetwork() noexcept
	{
#ifdef VI_MICROSOFT
		WSADATA WSAData;
		WORD VersionRequested = MAKEWORD(2, 2);
		int Code = WSAStartup(VersionRequested, &WSAData);
		VI_PANIC(Code == 0, "WSA initialization failed code:%i", Code);
		VI_TRACE("[lib] initialize windows networking library");
		return true;
#else
		return false;
#endif
	}
	bool Runtime::InitializeProviders(CryptographyState* Crypto) noexcept
	{
#ifdef VI_OPENSSL
#if OPENSSL_VERSION_MAJOR >= 3
		if (!Crypto)
			return false;

		VI_TRACE("[lib] load openssl providers from: *");
		Crypto->DefaultProvider = OSSL_PROVIDER_load(nullptr, "default");
		Crypto->LegacyProvider = OSSL_PROVIDER_load(nullptr, "legacy");
		if (Crypto->LegacyProvider != nullptr && Crypto->DefaultProvider != nullptr)
			return true;

		auto Path = Core::OS::Directory::GetModule();
		bool IsModuleDirectory = true;
	Retry:
		VI_TRACE("[lib] load openssl providers from: %s", Path ? Path->c_str() : "no path");
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

		return true;
#else
		return false;
#endif
#else
		return false;
#endif
	}
	bool Runtime::InitializeCryptography(CryptographyState** OutputCrypto, bool InitProviders) noexcept
	{
#ifdef VI_OPENSSL
		if (!OutputCrypto)
			return false;

		SSL_library_init();
		SSL_load_error_strings();

		auto Crypto = Core::Memory::New<CryptographyState>();
		*OutputCrypto = Crypto;
#if OPENSSL_VERSION_MAJOR >= 3
		if (InitProviders && !InitializeProviders(Crypto))
			return false;
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

		VI_TRACE("[lib] initialize openssl library");
#ifdef VI_POSTGRESQL
		PQinitSSL(0);
		VI_TRACE("[lib] initialize pq library");
#endif
		return true;
#else
		return false;
#endif
	}
	bool Runtime::InitializePlatform() noexcept
	{
#ifdef VI_SDL2
		SDL_SetMainReady();
		int Code = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER | SDL_INIT_HAPTIC);
		VI_PANIC(Code == 0, "SDL2 initialization failed code:%i", Code);
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

		VI_TRACE("[lib] initialize sdl2 library");
		return true;
#else
		return false;
#endif
	}
	bool Runtime::InitializeGraphics() noexcept
	{
#ifdef VI_GLEW
		glewExperimental = true;
		VI_TRACE("[lib] initialize graphics library");
		return true;
#else
		return false;
#endif
	}
	bool Runtime::InitializeLocale() noexcept
	{
		VI_TRACE("[lib] initialize locale library");
		setlocale(LC_TIME, "C");
		return true;
	}
	bool Runtime::InitializeRandom() noexcept
	{
#ifdef VI_OPENSSL
		int64_t Raw = 0;
		RAND_bytes((unsigned char*)&Raw, sizeof(int64_t));
		VI_TRACE("[lib] initialize random library");
		return true;
#else
		return false;
#endif
	}
	bool Runtime::InitializeAudio() noexcept
	{
		VI_TRACE("[lib] initialize audio library");
		auto Status = Audio::AudioContext::Initialize();
		Status.Report("audio initialization error");
		return !!Status;
	}
	bool Runtime::InitializeScripting() noexcept
	{
		Scripting::VirtualMachine::SetMemoryFunctions(Core::Memory::DefaultAllocate, Core::Memory::DefaultDeallocate);
		return true;
	}
	void Runtime::CleanupInstances() noexcept
	{
		VI_TRACE("[lib] free singleton instances");
		Network::HTTP::HrmCache::CleanupInstance();
		Network::LDB::Driver::CleanupInstance();
		Network::PDB::Driver::CleanupInstance();
		Network::MDB::Driver::CleanupInstance();
		Network::Uplinks::CleanupInstance();
		Network::Multiplexer::CleanupInstance();
		Network::TransportLayer::CleanupInstance();
		Network::DNS::CleanupInstance();
		Engine::Application::CleanupInstance();
		Engine::GUI::Subsystem::CleanupInstance();
		Core::Schedule::CleanupInstance();
		Core::Console::CleanupInstance();
	}
	void Runtime::CleanupCryptography(CryptographyState** InCrypto) noexcept
	{
#ifdef VI_OPENSSL
		if (!InCrypto)
			return;

		auto* Crypto = *InCrypto;
		*InCrypto = nullptr;
#if OPENSSL_VERSION_MAJOR >= 3
		VI_TRACE("[lib] free openssl providers");
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
		Core::Memory::Delete(Crypto);
		VI_TRACE("[lib] free openssl library");
#endif
	}
	void Runtime::CleanupPlatform() noexcept
	{
#ifdef VI_SDL2
		SDL_Quit();
		VI_TRACE("[lib] free sdl2 library");
#endif
	}
	void Runtime::CleanupNetwork() noexcept
	{
#ifdef VI_MICROSOFT
		WSACleanup();
		VI_TRACE("[lib] free windows networking library");
#endif
	}
	void Runtime::CleanupImporter() noexcept
	{
#ifdef VI_ASSIMP
		Assimp::DefaultLogger::kill();
		VI_TRACE("[lib] free importer library");
#endif
	}
	void Runtime::CleanupScripting() noexcept
	{
		Scripting::VirtualMachine::Cleanup();
		VI_TRACE("[lib] free virtual machine");
	}
	void Runtime::CleanupComposer() noexcept
	{
		Core::Composer::Cleanup();
		VI_TRACE("[lib] free component composer");
	}
	void Runtime::CleanupErrorHandling() noexcept
	{
		Core::ErrorHandling::Cleanup();
		VI_TRACE("[lib] free error handling");
	}
	void Runtime::CleanupMemory() noexcept
	{
		Core::Memory::Cleanup();
		VI_TRACE("[lib] free memory allocators");
	}
	void Runtime::CleanupAllocator(Core::GlobalAllocator** InAllocator) noexcept
	{
		if (!InAllocator)
			return;

		auto* Allocator = *InAllocator;
		if (Allocator != nullptr && Allocator->IsFinalizable())
		{
			*InAllocator = nullptr;
			delete Allocator;
		}
	}
	bool Runtime::HasFtAllocator() const noexcept
	{
#ifdef VI_ALLOCATOR
		return true;
#else
		return false;
#endif
	}
	bool Runtime::HasFtPessimistic() const noexcept
	{
#ifdef VI_PESSIMISTIC
		return true;
#else
		return false;
#endif
	}
	bool Runtime::HasFtBindings() const noexcept
	{
#ifdef VI_BINDINGS
		return true;
#else
		return false;
#endif
	}
	bool Runtime::HasFtShaders() const noexcept
	{
		return HasSoSpirv();
	}
	bool Runtime::HasFtFContext() const noexcept
	{
#ifdef VI_FCONTEXT
		return true;
#else
		return false;
#endif
	}
	bool Runtime::HasSoOpenGL() const noexcept
	{
#ifdef VI_OPENGL
		return true;
#else
		return false;
#endif
	}
	bool Runtime::HasSoOpenAL() const noexcept
	{
#ifdef VI_OPENAL
		return true;
#else
		return false;
#endif
	}
	bool Runtime::HasSoOpenSSL() const noexcept
	{
#ifdef VI_OPENSSL
		return true;
#else
		return false;
#endif
	}
	bool Runtime::HasSoSDL2() const noexcept
	{
#ifdef VI_SDL2
		return true;
#else
		return false;
#endif
	}
	bool Runtime::HasSoGLEW() const noexcept
	{
#ifdef VI_GLEW
		return true;
#else
		return false;
#endif
	}
	bool Runtime::HasSoSpirv() const noexcept
	{
#ifdef VI_SPIRV
		return true;
#else
		return false;
#endif
	}
	bool Runtime::HasSoZLib() const noexcept
	{
#ifdef VI_ZLIB
		return true;
#else
		return false;
#endif
	}
	bool Runtime::HasSoAssimp() const noexcept
	{
#ifdef VI_ASSIMP
		return true;
#else
		return false;
#endif
	}
	bool Runtime::HasSoMongoc() const noexcept
	{
#ifdef VI_MONGOC
		return true;
#else
		return false;
#endif
	}
	bool Runtime::HasSoPostgreSQL() const noexcept
	{
#ifdef VI_POSTGRESQL
		return true;
#else
		return false;
#endif
	}
	bool Runtime::HasSoSQLite() const noexcept
	{
#ifdef VI_SQLITE
		return true;
#else
		return false;
#endif
	}
	bool Runtime::HasSoFreeType() const noexcept
	{
#ifdef VI_FREETYPE
		return true;
#else
		return false;
#endif
	}
	bool Runtime::HasMdAngelScript() const noexcept
	{
#ifdef VI_ANGELSCRIPT
		return true;
#else
		return false;
#endif
	}
	bool Runtime::HasMdBackwardCpp() const noexcept
	{
#ifdef VI_BACKWARDCPP
		return true;
#else
		return false;
#endif
	}
	bool Runtime::HasMdRmlUI() const noexcept
	{
#ifdef VI_RMLUI
		return true;
#else
		return false;
#endif
	}
	bool Runtime::HasMdBullet3() const noexcept
	{
#ifdef VI_BULLET3
		return true;
#else
		return false;
#endif
	}
	bool Runtime::HasMdTinyFileDialogs() const noexcept
	{
#ifdef VI_TINYFILEDIALOGS
		return true;
#else
		return false;
#endif
	}
	bool Runtime::HasMdWepoll() const noexcept
	{
#ifdef VI_WEPOLL
		return true;
#else
		return false;
#endif
	}
	bool Runtime::HasMdStb() const noexcept
	{
#ifdef VI_STB
		return true;
#else
		return false;
#endif
	}
	bool Runtime::HasMdPugiXml() const noexcept
	{
#ifdef VI_PUGIXML
		return true;
#else
		return false;
#endif
	}
	bool Runtime::HasMdRapidJson() const noexcept
	{
#ifdef VI_RAPIDJSON
		return true;
#else
		return false;
#endif
	}
	bool Runtime::HasMdVectorclass() const noexcept
	{
#ifdef VI_VECTORCLASS
		return true;
#else
		return false;
#endif
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
		if (HasSoOpenGL())
			Features.push_back("so:opengl");
		if (HasSoOpenAL())
			Features.push_back("so:openal");
		if (HasSoOpenSSL())
			Features.push_back("so:openssl");
		if (HasSoSDL2())
			Features.push_back("so:sdl2");
		if (HasSoGLEW())
			Features.push_back("so:glew");
		if (HasSoSpirv())
			Features.push_back("so:spirv");
		if (HasSoZLib())
			Features.push_back("so:zlib");
		if (HasSoAssimp())
			Features.push_back("so:assimp");
		if (HasSoMongoc())
			Features.push_back("so:mongoc");
		if (HasSoPostgreSQL())
			Features.push_back("so:pq");
		if (HasSoSQLite())
			Features.push_back("so:sqlite");
		if (HasSoFreeType())
			Features.push_back("so:freetype");
		if (HasMdAngelScript())
			Features.push_back("md:angelscript");
		if (HasMdBackwardCpp())
			Features.push_back("md:backward-cpp");
		if (HasMdRmlUI())
			Features.push_back("md:rmlui");
		if (HasMdBullet3())
			Features.push_back("md:bullet3");
		if (HasMdTinyFileDialogs())
			Features.push_back("md:tinyfiledialogs");
		if (HasMdWepoll())
			Features.push_back("md:wepoll");
		if (HasMdStb())
			Features.push_back("md:stb");
		if (HasMdPugiXml())
			Features.push_back("md:pugixml");
		if (HasMdRapidJson())
			Features.push_back("md:rapidjson");
		if (HasMdVectorclass())
			Features.push_back("md:vectorclass");
		if (HasFtFContext())
			Features.push_back("ft:fcontext");
		if (HasFtAllocator())
			Features.push_back("ft:allocator");
		if (HasFtPessimistic())
			Features.push_back("ft:pessimistic");
		if (HasFtBindings())
			Features.push_back("ft:bindings");
		if (HasFtShaders())
			Features.push_back("ft:shaders");

		Core::StringStream Result;
		Result << "library: " << MAJOR_VERSION << "." << MINOR_VERSION << "." << PATCH_VERSION << "." << BUILD_VERSION << " / " << VERSION << "\n";
		Result << "  platform: " << GetPlatform() << " / " << GetBuild() << "\n";
		Result << "  compiler: " << GetCompiler() << "\n";
        Result << "configuration:" << "\n";
        
		for (size_t i = 0; i < Features.size(); i++)
			Result << "  " << Features[i] << "\n";

		return Result.str();
	}
	std::string_view Runtime::GetBuild() const noexcept
	{
#ifndef NDEBUG
		return "Debug";
#else
		return "Release";
#endif
	}
	std::string_view Runtime::GetCompiler() const noexcept
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
	std::string_view Runtime::GetPlatform() const noexcept
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
