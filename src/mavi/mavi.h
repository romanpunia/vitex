#ifndef MAVI_H
#define MAVI_H
#include "core/core.h"

namespace Mavi
{
	enum
	{
		MAJOR_VERSION = 26,
		MINOR_VERSION = 45,
		PATCH_VERSION = 42,
		VERSION = (MAJOR_VERSION) * 10000 + (MINOR_VERSION) * 100 + (PATCH_VERSION)
	};

	enum class Init
	{
		Core = 1,
		Network = 2,
		SSL = 4,
		SDL2 = 8,
		Compute = 16,
		Locale = 32,
		Audio = 64,
		GLEW = 128
	};

	constexpr inline Init operator |(Init A, Init B)
	{
		return static_cast<Init>(static_cast<size_t>(A) | static_cast<size_t>(B));
	}

	enum class Preset : size_t
	{
		App = (size_t)(Init::Core | Init::Network | Init::SSL | Init::Compute | Init::Locale),
		Game = (size_t)App | (size_t)(Init::SDL2 | Init::Audio | Init::GLEW)
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
		bool HasOpenAL() const noexcept;
		bool HasSDL2() const noexcept;
		bool HasSIMD() const noexcept;
		bool HasJIT() const noexcept;
		bool HasBindings() const noexcept;
		bool HasAllocator() const noexcept;
		bool HasBacktrace() const noexcept;
		bool HasFreeType() const noexcept;
		bool HasSPIRV() const noexcept;
        bool HasRmlUI() const noexcept;
		bool HasBullet3() const noexcept;
        bool HasFContext() const noexcept;
		bool HasWindowsEpoll() const noexcept;
		bool HasShaders() const noexcept;
		int GetMajorVersion() const noexcept;
		int GetMinorVersion() const noexcept;
		int GetPatchVersion() const noexcept;
		int GetVersion() const noexcept;
		int GetDebugLevel() const noexcept;
		int GetArchitecture() const noexcept;
		size_t GetModes() const noexcept;
		Core::String GetDetails() const noexcept;
		const char* GetBuild() const noexcept;
		const char* GetCompiler() const noexcept;
		const char* GetPlatform() const noexcept;

	public:
		static Runtime* Get() noexcept;
	};
}
#endif
