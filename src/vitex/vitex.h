#ifndef VITEX_H
#define VITEX_H
#include "core.h"

namespace Vitex
{
	enum
	{
		MAJOR_VERSION = 2,
		MINOR_VERSION = 6,
		PATCH_VERSION = 1,
		BUILD_VERSION = 6,
		VERSION = (MAJOR_VERSION) * 100000000 + (MINOR_VERSION) * 1000000 + (PATCH_VERSION) * 1000 + BUILD_VERSION
	};

	enum class Init
	{
		Network = 1,
		Cryptography = 2,
		Platform = 4,
		Providers = 8,
		Locale = 16,
		Audio = 32,
		Graphics = 64
	};

	constexpr inline Init operator |(Init A, Init B)
	{
		return static_cast<Init>(static_cast<size_t>(A) | static_cast<size_t>(B));
	}

	enum class Preset : size_t
	{
		App = (size_t)(Init::Network | Init::Cryptography | Init::Providers | Init::Locale),
		Game = (size_t)App | (size_t)(Init::Platform | Init::Audio | Init::Graphics)
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
		Runtime(size_t Modules = (size_t)Preset::App, Core::GlobalAllocator* Allocator = nullptr) noexcept;
		~Runtime() noexcept;
		bool HasFtAllocator() const noexcept;
		bool HasFtPessimistic() const noexcept;
		bool HasFtBindings() const noexcept;
		bool HasFtShaders() const noexcept;
		bool HasFtFContext() const noexcept;
		bool HasSoOpenGL() const noexcept;
		bool HasSoOpenAL() const noexcept;
		bool HasSoOpenSSL() const noexcept;
		bool HasSoSDL2() const noexcept;
		bool HasSoGLEW() const noexcept;
		bool HasSoSpirv() const noexcept;
		bool HasSoZLib() const noexcept;
		bool HasSoAssimp() const noexcept;
		bool HasSoMongoc() const noexcept;
		bool HasSoPostgreSQL() const noexcept;
		bool HasSoSQLite() const noexcept;
		bool HasSoFreeType() const noexcept;
		bool HasMdAngelScript() const noexcept;
		bool HasMdBackwardCpp() const noexcept;
		bool HasMdRmlUI() const noexcept;
		bool HasMdBullet3() const noexcept;
		bool HasMdTinyFileDialogs() const noexcept;
		bool HasMdWepoll() const noexcept;
		bool HasMdStb() const noexcept;
		bool HasMdPugiXml() const noexcept;
		bool HasMdRapidJson() const noexcept;
		bool HasMdVectorclass() const noexcept;
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
		static bool InitializePlatform() noexcept;
		static bool InitializeGraphics() noexcept;
		static bool InitializeLocale() noexcept;
		static bool InitializeRandom() noexcept;
		static bool InitializeAudio() noexcept;
		static bool InitializeScripting() noexcept;
		static void CleanupInstances() noexcept;
		static void CleanupCryptography(CryptographyState** InCrypto) noexcept;
		static void CleanupPlatform() noexcept;
		static void CleanupNetwork() noexcept;
		static void CleanupImporter() noexcept;
		static void CleanupScripting() noexcept;
		static void CleanupComposer() noexcept;
		static void CleanupErrorHandling() noexcept;
		static void CleanupMemory() noexcept;
		static void CleanupAllocator(Core::GlobalAllocator** InAllocator) noexcept;
		static Runtime* Get() noexcept;
	};
}
#endif
