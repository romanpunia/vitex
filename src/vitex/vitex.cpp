#include "vitex.h"
#include "compute.h"
#include "network.h"
#include "scripting.h"
#include "bindings.h"
#include "network/http.h"
#include "network/ldb.h"
#include "network/pdb.h"
#include "network/mdb.h"
#include "layer.h"
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
#ifdef VI_POSTGRESQL
#include <libpq-fe.h>
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
		if (Modes & LOAD_NETWORKING)
			InitializeNetwork();

		if (Modes & LOAD_CRYPTOGRAPHY)
			InitializeCryptography(&Crypto, Modes & LOAD_PROVIDERS);

		if (Modes & LOAD_LOCALE)
			InitializeLocale();

		if (Modes & LOAD_CRYPTOGRAPHY)
			InitializeRandom();

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
		if (Modes & LOAD_CRYPTOGRAPHY)
			CleanupCryptography(&Crypto);

		if (Modes & LOAD_NETWORKING)
			CleanupNetwork();
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
	bool Runtime::InitializeScripting() noexcept
	{
		Scripting::VirtualMachine::SetMemoryFunctions(Core::Memory::DefaultAllocate, Core::Memory::DefaultDeallocate);
		return true;
	}
	void Runtime::CleanupInstances() noexcept
	{
		VI_TRACE("[lib] free singleton instances");
		Layer::Application::CleanupInstance();
		Network::HTTP::HrmCache::CleanupInstance();
		Network::LDB::Driver::CleanupInstance();
		Network::PDB::Driver::CleanupInstance();
		Network::MDB::Driver::CleanupInstance();
		Network::Uplinks::CleanupInstance();
		Network::TransportLayer::CleanupInstance();
		Network::DNS::CleanupInstance();
		Network::Multiplexer::CleanupInstance();
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
	void Runtime::CleanupNetwork() noexcept
	{
#ifdef VI_MICROSOFT
		WSACleanup();
		VI_TRACE("[lib] free windows networking library");
#endif
	}
	void Runtime::CleanupScripting() noexcept
	{
		Scripting::Bindings::Registry().Cleanup();
		VI_TRACE("[lib] free bindings registry");
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
	bool Runtime::HasFtFContext() const noexcept
	{
#ifdef VI_FCONTEXT
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
	bool Runtime::HasSoZLib() const noexcept
	{
#ifdef VI_ZLIB
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
	bool Runtime::HasMdWepoll() const noexcept
	{
#ifdef VI_WEPOLL
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
		if (HasSoOpenSSL())
			Features.push_back("so:openssl");
		if (HasSoZLib())
			Features.push_back("so:zlib");
		if (HasSoMongoc())
			Features.push_back("so:mongoc");
		if (HasSoPostgreSQL())
			Features.push_back("so:pq");
		if (HasSoSQLite())
			Features.push_back("so:sqlite");
		if (HasMdAngelScript())
			Features.push_back("md:angelscript");
		if (HasMdBackwardCpp())
			Features.push_back("md:backward-cpp");
		if (HasMdWepoll())
			Features.push_back("md:wepoll");
		if (HasMdPugiXml())
			Features.push_back("md:pugixml");
		if (HasMdRapidJson())
			Features.push_back("md:rapidjson");
		if (HasFtFContext())
			Features.push_back("ft:fcontext");
		if (HasFtAllocator())
			Features.push_back("ft:allocator");
		if (HasFtPessimistic())
			Features.push_back("ft:pessimistic");
		if (HasFtBindings())
			Features.push_back("ft:bindings");

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
