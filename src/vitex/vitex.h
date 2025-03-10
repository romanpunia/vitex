#ifndef VITEX_H
#define VITEX_H
#include "core.h"

namespace vitex
{
	enum
	{
		use_networking = 1 << 0,
		use_cryptography = 1 << 1,
		use_providers = 1 << 2,
		use_locale = 1 << 3,
		major_version = 4,
		minor_version = 0,
		patch_version = 0,
		build_version = 0,
		version = (major_version) * 100000000 + (minor_version) * 1000000 + (patch_version) * 1000 + build_version
	};

	class runtime : public core::singletonish
	{
	private:
		static runtime* instance;

	public:
		struct cryptography_state
		{
			core::vector<std::shared_ptr<std::mutex>> locks;
			void* legacy_provider = nullptr;
			void* default_provider = nullptr;
		};

	protected:
		cryptography_state* crypto;
		size_t modes;

	public:
		runtime(size_t modules = use_networking | use_cryptography | use_providers | use_locale, core::global_allocator* allocator = nullptr) noexcept;
		virtual ~runtime() noexcept;
		bool has_ft_allocator() const noexcept;
		bool has_ft_pessimistic() const noexcept;
		bool has_ft_bindings() const noexcept;
		bool has_ft_fcontext() const noexcept;
		bool has_so_openssl() const noexcept;
		bool has_so_zlib() const noexcept;
		bool has_so_mongoc() const noexcept;
		bool has_so_postgresql() const noexcept;
		bool has_so_sqlite() const noexcept;
		bool has_md_angelscript() const noexcept;
		bool has_md_backwardcpp() const noexcept;
		bool has_md_wepoll() const noexcept;
		bool has_md_pugixml() const noexcept;
		bool has_md_rapidjson() const noexcept;
		int get_major_version() const noexcept;
		int get_minor_version() const noexcept;
		int get_patch_version() const noexcept;
		int get_build_version() const noexcept;
		int get_version() const noexcept;
		int get_debug_level() const noexcept;
		int get_architecture() const noexcept;
		size_t get_modes() const noexcept;
		virtual core::string get_details() const noexcept;
		std::string_view get_build() const noexcept;
		std::string_view get_compiler() const noexcept;
		std::string_view get_platform() const noexcept;

	public:
		static bool initialize_allocator(core::global_allocator** inout_allocator) noexcept;
		static bool initialize_network() noexcept;
		static bool initialize_providers(cryptography_state* crypto) noexcept;
		static bool initialize_cryptography(cryptography_state** out_crypto, bool init_providers) noexcept;
		static bool initialize_locale() noexcept;
		static bool initialize_random() noexcept;
		static bool initialize_scripting() noexcept;
		static void cleanup_instances() noexcept;
		static void cleanup_cryptography(cryptography_state** in_crypto) noexcept;
		static void cleanup_network() noexcept;
		static void cleanup_scripting() noexcept;
		static void cleanup_composer() noexcept;
		static void cleanup_error_handling() noexcept;
		static void cleanup_memory() noexcept;
		static void cleanup_allocator(core::global_allocator** in_allocator) noexcept;
		static runtime* get() noexcept;
	};
}
#endif
