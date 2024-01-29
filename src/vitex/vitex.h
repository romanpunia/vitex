#ifndef VITEX_H
#define VITEX_H
#include "core/core.h"

namespace Vitex
{
	enum
	{
		MAJOR_VERSION = 1,
		MINOR_VERSION = 40,
		PATCH_VERSION = 93,
		BUILD_VERSION = 184,
		VERSION = (MAJOR_VERSION) * 100000000 + (MINOR_VERSION) * 1000000 + (PATCH_VERSION) * 10000 + BUILD_VERSION
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

	private:
		struct CryptoData
		{
			Core::Vector<std::shared_ptr<std::mutex>> Locks;
			void* LegacyProvider = nullptr;
			void* DefaultProvider = nullptr;
		};

	private:
		CryptoData* Crypto;
		size_t Modes;

	public:
		Runtime(size_t Modules = (size_t)Preset::App, Core::GlobalAllocator* Allocator = nullptr) noexcept;
		~Runtime() noexcept;
		bool HasDirectX() const noexcept;
		bool HasOpenGL() const noexcept;
		bool HasOpenSSL() const noexcept;
		bool HasGLEW() const noexcept;
		bool HasZLib() const noexcept;
		bool HasAssimp() const noexcept;
		bool HasMongoDB() const noexcept;
		bool HasPostgreSQL() const noexcept;
		bool HasSQLite() const noexcept;
		bool HasOpenAL() const noexcept;
		bool HasSDL2() const noexcept;
		bool HasSIMD() const noexcept;
		bool HasJIT() const noexcept;
		bool HasAngelScript() const noexcept;
		bool HasBindings() const noexcept;
		bool HasAllocator() const noexcept;
		bool HasBacktrace() const noexcept;
		bool HasFreeType() const noexcept;
		bool HasSPIRV() const noexcept;
        bool HasRmlUI() const noexcept;
		bool HasBullet3() const noexcept;
        bool HasFContext() const noexcept;
		bool HasWindowsEpoll() const noexcept;
		bool HasTinyFileDialogs() const noexcept;
		bool HasSTB() const noexcept;
		bool HasPugiXML() const noexcept;
		bool HasRapidJSON() const noexcept;
		bool HasShaders() const noexcept;
		int GetMajorVersion() const noexcept;
		int GetMinorVersion() const noexcept;
		int GetPatchVersion() const noexcept;
		int GetBuildVersion() const noexcept;
		int GetVersion() const noexcept;
		int GetDebugLevel() const noexcept;
		int GetArchitecture() const noexcept;
		size_t GetModes() const noexcept;
		Core::String GetDetails() const noexcept;
		const char* GetBuild() const noexcept;
		const char* GetCompiler() const noexcept;
		const char* GetPlatform() const noexcept;

	public:
		static void CleanupInstances();
		static Runtime* Get() noexcept;
	};
}
#endif
