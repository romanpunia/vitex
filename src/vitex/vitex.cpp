#include "vitex.h"
#include "compute.h"
#include "network.h"
#include "scripting.h"
#include "bindings.h"
#include "network/http.h"
#include "network/sqlite.h"
#include "network/pq.h"
#include "network/mongo.h"
#include "layer.h"
#include <clocale>
#include <sstream>
#ifdef VI_MICROSOFT
#include <ws2tcpip.h>
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

namespace vitex
{
	runtime::runtime(size_t modules, core::global_allocator* allocator) noexcept : crypto(nullptr), modes(modules)
	{
		VI_ASSERT(!instance, "vitex runtime should not be already initialized");
		instance = this;

		initialize_allocator(&allocator);
		if (modes & use_networking)
			initialize_network();

		if (modes & use_cryptography)
			initialize_cryptography(&crypto, modes & use_providers);

		if (modes & use_locale)
			initialize_locale();

		if (modes & use_cryptography)
			initialize_random();

		initialize_scripting();
#ifndef VI_MICROSOFT
		signal(SIGPIPE, SIG_IGN);
#endif
	}
	runtime::~runtime() noexcept
	{
		auto* allocator = core::memory::get_global_allocator();
		core::error_handling::set_flag(core::log_option::async, false);
		cleanup_instances();
		if (modes & use_cryptography)
			cleanup_cryptography(&crypto);

		if (modes & use_networking)
			cleanup_network();
		cleanup_scripting();
		cleanup_composer();
		cleanup_error_handling();
		cleanup_memory();
		cleanup_allocator(&allocator);
		if (instance == this)
			instance = nullptr;
	}
	bool runtime::initialize_allocator(core::global_allocator** inout_allocator) noexcept
	{
		if (!inout_allocator)
			return false;
#ifndef NDEBUG
		core::error_handling::set_flag(core::log_option::active, true);
		if (!*inout_allocator)
		{
			*inout_allocator = new core::allocators::debug_allocator();
			VI_TRACE("lib initialize global debug allocator");
		}
#else
		if (!*inout_allocator)
		{
			*inout_allocator = new core::allocators::default_allocator();
			VI_TRACE("lib initialize global default allocator");
		}
#endif
		core::memory::set_global_allocator(*inout_allocator);
		return *inout_allocator != nullptr;
	}
	bool runtime::initialize_network() noexcept
	{
#ifdef VI_MICROSOFT
		WSADATA wsa_data;
		WORD version_requested = MAKEWORD(2, 2);
		int code = WSAStartup(version_requested, &wsa_data);
		VI_PANIC(code == 0, "WSA initialization failed code:%i", code);
		VI_TRACE("lib initialize windows networking library");
		return true;
#else
		return false;
#endif
	}
	bool runtime::initialize_providers(cryptography_state* crypto) noexcept
	{
#ifdef VI_OPENSSL
#if OPENSSL_VERSION_MAJOR >= 3
		if (!crypto)
			return false;

		VI_TRACE("lib load openssl providers from: *");
		crypto->default_provider = OSSL_PROVIDER_load(nullptr, "default");
		crypto->legacy_provider = OSSL_PROVIDER_load(nullptr, "legacy");
		if (crypto->legacy_provider != nullptr && crypto->default_provider != nullptr)
			return true;

		auto path = core::os::directory::get_module();
		bool is_module_directory = true;
	retry:
		VI_TRACE("lib load openssl providers from: %s", path ? path->c_str() : "no path");
		if (path)
			OSSL_PROVIDER_set_default_search_path(nullptr, path->c_str());

		if (!crypto->default_provider)
			crypto->default_provider = OSSL_PROVIDER_load(nullptr, "default");

		if (!crypto->legacy_provider)
			crypto->legacy_provider = OSSL_PROVIDER_load(nullptr, "legacy");

		if (!crypto->legacy_provider || !crypto->default_provider)
		{
			if (is_module_directory)
			{
				path = core::os::directory::get_working();
				is_module_directory = false;
				goto retry;
			}
			compute::crypto::display_crypto_log();
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
	bool runtime::initialize_cryptography(cryptography_state** output_crypto, bool init_providers) noexcept
	{
#ifdef VI_OPENSSL
		if (!output_crypto)
			return false;

		SSL_library_init();
		SSL_load_error_strings();

		auto crypto = core::memory::init<cryptography_state>();
		*output_crypto = crypto;
#if OPENSSL_VERSION_MAJOR >= 3
		if (init_providers && !initialize_providers(crypto))
			return false;
#else
		FIPS_mode_set(1);
#endif
		RAND_poll();

		int count = CRYPTO_num_locks();
		crypto->locks.reserve(count);
		for (int i = 0; i < count; i++)
			crypto->locks.push_back(std::make_shared<std::mutex>());

		CRYPTO_set_id_callback([]() -> long unsigned int
		{
#ifdef VI_MICROSOFT
			return (unsigned long)GetCurrentThreadId();
#else
			return (unsigned long)syscall(sy_sgettid);
#endif
		});
		CRYPTO_set_locking_callback([](int mode, int id, const char* file, int line)
		{
			if (mode & CRYPTO_LOCK)
				crypto->locks.at(id)->lock();
			else
				crypto->locks.at(id)->unlock();
		});

		VI_TRACE("lib initialize openssl library");
#ifdef VI_POSTGRESQL
		PQinitSSL(0);
		VI_TRACE("lib initialize pq library");
#endif
		return true;
#else
		return false;
#endif
	}
	bool runtime::initialize_locale() noexcept
	{
		VI_TRACE("lib initialize locale library");
		setlocale(LC_TIME, "C");
		return true;
	}
	bool runtime::initialize_random() noexcept
	{
#ifdef VI_OPENSSL
		int64_t raw = 0;
		RAND_bytes((unsigned char*)&raw, sizeof(int64_t));
		VI_TRACE("lib initialize random library");
		return true;
#else
		return false;
#endif
	}
	bool runtime::initialize_scripting() noexcept
	{
		scripting::virtual_machine::set_memory_functions(core::memory::default_allocate, core::memory::default_deallocate);
		return true;
	}
	void runtime::cleanup_instances() noexcept
	{
		VI_TRACE("lib free singleton instances");
		layer::application::cleanup_instance();
		network::http::hrm_cache::cleanup_instance();
		network::sqlite::driver::cleanup_instance();
		network::pq::driver::cleanup_instance();
		network::mongo::driver::cleanup_instance();
		network::uplinks::cleanup_instance();
		network::transport_layer::cleanup_instance();
		network::dns::cleanup_instance();
		network::multiplexer::cleanup_instance();
		core::schedule::cleanup_instance();
		core::console::cleanup_instance();
	}
	void runtime::cleanup_cryptography(cryptography_state** in_crypto) noexcept
	{
#ifdef VI_OPENSSL
		if (!in_crypto)
			return;

		auto* crypto = *in_crypto;
		*in_crypto = nullptr;
#if OPENSSL_VERSION_MAJOR >= 3
		VI_TRACE("lib free openssl providers");
		OSSL_PROVIDER_unload((OSSL_PROVIDER*)crypto->legacy_provider);
		OSSL_PROVIDER_unload((OSSL_PROVIDER*)crypto->default_provider);
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
		core::memory::deinit(crypto);
		VI_TRACE("lib free openssl library");
#endif
	}
	void runtime::cleanup_network() noexcept
	{
#ifdef VI_MICROSOFT
		WSACleanup();
		VI_TRACE("lib free windows networking library");
#endif
	}
	void runtime::cleanup_scripting() noexcept
	{
		scripting::bindings::registry().cleanup();
		VI_TRACE("lib free bindings registry");
		scripting::virtual_machine::cleanup();
		VI_TRACE("lib free virtual machine");
	}
	void runtime::cleanup_composer() noexcept
	{
		core::composer::cleanup();
		VI_TRACE("lib free component composer");
	}
	void runtime::cleanup_error_handling() noexcept
	{
		core::error_handling::cleanup();
		VI_TRACE("lib free error handling");
	}
	void runtime::cleanup_memory() noexcept
	{
		core::memory::cleanup();
		VI_TRACE("lib free memory allocators");
	}
	void runtime::cleanup_allocator(core::global_allocator** in_allocator) noexcept
	{
		if (!in_allocator)
			return;

		auto* allocator = *in_allocator;
		if (allocator != nullptr && allocator->is_finalizable())
		{
			*in_allocator = nullptr;
			delete allocator;
		}
	}
	bool runtime::has_ft_allocator() const noexcept
	{
#ifdef VI_ALLOCATOR
		return true;
#else
		return false;
#endif
	}
	bool runtime::has_ft_pessimistic() const noexcept
	{
#ifdef VI_PESSIMISTIC
		return true;
#else
		return false;
#endif
	}
	bool runtime::has_ft_bindings() const noexcept
	{
#ifdef VI_BINDINGS
		return true;
#else
		return false;
#endif
	}
	bool runtime::has_ft_fcontext() const noexcept
	{
#ifdef VI_FCONTEXT
		return true;
#else
		return false;
#endif
	}
	bool runtime::has_so_openssl() const noexcept
	{
#ifdef VI_OPENSSL
		return true;
#else
		return false;
#endif
	}
	bool runtime::has_so_zlib() const noexcept
	{
#ifdef VI_ZLIB
		return true;
#else
		return false;
#endif
	}
	bool runtime::has_so_mongoc() const noexcept
	{
#ifdef VI_MONGOC
		return true;
#else
		return false;
#endif
	}
	bool runtime::has_so_postgresql() const noexcept
	{
#ifdef VI_POSTGRESQL
		return true;
#else
		return false;
#endif
	}
	bool runtime::has_so_sqlite() const noexcept
	{
#ifdef VI_SQLITE
		return true;
#else
		return false;
#endif
	}
	bool runtime::has_md_angelscript() const noexcept
	{
#ifdef VI_ANGELSCRIPT
		return true;
#else
		return false;
#endif
	}
	bool runtime::has_md_backwardcpp() const noexcept
	{
#ifdef VI_BACKWARDCPP
		return true;
#else
		return false;
#endif
	}
	bool runtime::has_md_wepoll() const noexcept
	{
#ifdef VI_WEPOLL
		return true;
#else
		return false;
#endif
	}
	bool runtime::has_md_pugixml() const noexcept
	{
#ifdef VI_PUGIXML
		return true;
#else
		return false;
#endif
	}
	bool runtime::has_md_rapidjson() const noexcept
	{
#ifdef VI_RAPIDJSON
		return true;
#else
		return false;
#endif
	}
	int runtime::get_major_version() const noexcept
	{
		return (int)major_version;
	}
	int runtime::get_minor_version() const noexcept
	{
		return (int)minor_version;
	}
	int runtime::get_patch_version() const noexcept
	{
		return (int)patch_version;
	}
	int runtime::get_build_version() const noexcept
	{
		return (int)patch_version;
	}
	int runtime::get_version() const noexcept
	{
		return (int)version;
	}
	int runtime::get_debug_level() const noexcept
	{
		return VI_DLEVEL;
	}
	int runtime::get_architecture() const noexcept
	{
		return (int)sizeof(size_t);
	}
	size_t runtime::get_modes() const noexcept
	{
		return modes;
	}
	core::string runtime::get_details() const noexcept
	{
		core::vector<core::string> features;
		if (has_so_openssl())
			features.push_back("so:openssl");
		if (has_so_zlib())
			features.push_back("so:zlib");
		if (has_so_mongoc())
			features.push_back("so:mongoc");
		if (has_so_postgresql())
			features.push_back("so:pq");
		if (has_so_sqlite())
			features.push_back("so:sqlite");
		if (has_md_angelscript())
			features.push_back("md:angelscript");
		if (has_md_backwardcpp())
			features.push_back("md:backward-cpp");
		if (has_md_wepoll())
			features.push_back("md:wepoll");
		if (has_md_pugixml())
			features.push_back("md:pugixml");
		if (has_md_rapidjson())
			features.push_back("md:rapidjson");
		if (has_ft_fcontext())
			features.push_back("ft:fcontext");
		if (has_ft_allocator())
			features.push_back("ft:allocator");
		if (has_ft_pessimistic())
			features.push_back("ft:pessimistic");
		if (has_ft_bindings())
			features.push_back("ft:bindings");

		core::string_stream result;
		result << "library: " << major_version << "." << minor_version << "." << patch_version << "." << build_version << " / " << version << "\n";
		result << "  platform: " << get_platform() << " / " << get_build() << "\n";
		result << "  compiler: " << get_compiler() << "\n";
		result << "configuration:" << "\n";

		for (size_t i = 0; i < features.size(); i++)
			result << "  " << features[i] << "\n";

		return result.str();
	}
	std::string_view runtime::get_build() const noexcept
	{
#ifndef NDEBUG
		return "Debug";
#else
		return "Release";
#endif
	}
	std::string_view runtime::get_compiler() const noexcept
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
	std::string_view runtime::get_platform() const noexcept
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
	runtime* runtime::get() noexcept
	{
		return instance;
	}
	runtime* runtime::instance = nullptr;
}
