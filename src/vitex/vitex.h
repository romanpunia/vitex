#ifndef VITEX_H
#define VITEX_H
#include "core.h"

namespace Vitex
{
	enum
	{
		LOAD_NETWORKING = 1 << 0,
		LOAD_CRYPTOGRAPHY = 1 << 1,
		LOAD_PROVIDERS = 1 << 2,
		LOAD_LOCALE = 1 << 3,
		MAJOR_VERSION = 3,
		MINOR_VERSION = 0,
		PATCH_VERSION = 0,
		BUILD_VERSION = 1,
		VERSION = (MAJOR_VERSION) * 100000000 + (MINOR_VERSION) * 1000000 + (PATCH_VERSION) * 1000 + BUILD_VERSION
	};

	class VI_OUT_TS Runtime final : public Core::Singletonish
	{
	private:
		static Runtime* Instance;

	public:
		struct CryptographyState
		{
			Core::Vector<std::shared_ptr<std::mutex>> Locks;
			void* LegacyProvider = nullptr;
			void* DefaultProvider = nullptr;
		};

	private:
		CryptographyState* Crypto;
		size_t Modes;

	public:
		Runtime(size_t Modules = LOAD_NETWORKING | LOAD_CRYPTOGRAPHY | LOAD_PROVIDERS | LOAD_LOCALE, Core::GlobalAllocator* Allocator = nullptr) noexcept;
		~Runtime() noexcept;
		bool HasFtAllocator() const noexcept;
		bool HasFtPessimistic() const noexcept;
		bool HasFtBindings() const noexcept;
		bool HasFtFContext() const noexcept;
		bool HasSoOpenSSL() const noexcept;
		bool HasSoZLib() const noexcept;
		bool HasSoMongoc() const noexcept;
		bool HasSoPostgreSQL() const noexcept;
		bool HasSoSQLite() const noexcept;
		bool HasMdAngelScript() const noexcept;
		bool HasMdBackwardCpp() const noexcept;
		bool HasMdWepoll() const noexcept;
		bool HasMdPugiXml() const noexcept;
		bool HasMdRapidJson() const noexcept;
		int GetMajorVersion() const noexcept;
		int GetMinorVersion() const noexcept;
		int GetPatchVersion() const noexcept;
		int GetBuildVersion() const noexcept;
		int GetVersion() const noexcept;
		int GetDebugLevel() const noexcept;
		int GetArchitecture() const noexcept;
		size_t GetModes() const noexcept;
		Core::String GetDetails() const noexcept;
		std::string_view GetBuild() const noexcept;
		std::string_view GetCompiler() const noexcept;
		std::string_view GetPlatform() const noexcept;

	public:
		static bool InitializeAllocator(Core::GlobalAllocator** InoutAllocator) noexcept;
		static bool InitializeNetwork() noexcept;
		static bool InitializeProviders(CryptographyState* Crypto) noexcept;
		static bool InitializeCryptography(CryptographyState** OutCrypto, bool InitProviders) noexcept;
		static bool InitializeLocale() noexcept;
		static bool InitializeRandom() noexcept;
		static bool InitializeScripting() noexcept;
		static void CleanupInstances() noexcept;
		static void CleanupCryptography(CryptographyState** InCrypto) noexcept;
		static void CleanupNetwork() noexcept;
		static void CleanupScripting() noexcept;
		static void CleanupComposer() noexcept;
		static void CleanupErrorHandling() noexcept;
		static void CleanupMemory() noexcept;
		static void CleanupAllocator(Core::GlobalAllocator** InAllocator) noexcept;
		static Runtime* Get() noexcept;
	};
}
#endif
