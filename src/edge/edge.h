#ifndef EDGE_H
#define EDGE_H
#define ED_MAJOR_VERSION 16
#define ED_MINOR_VERSION 15
#define ED_PATCH_LEVEL 64
#define ED_VERSION(X, Y, Z) ((X) * 1000 + (Y) * 100 + (Z))
#define ED_AT_LEAST(X, Y, Z) (ED_VERSION(ED_MAJOR_VERSION, ED_MINOR_VERSION, ED_PATCH_LEVEL) >= ED_VERSION(X, Y, Z))
#include "core/core.h"

namespace Edge
{
	enum class Init
	{
		Core = 1,
		Debug = 2,
		Network = 4,
		SSL = 8,
		SDL2 = 16,
		Compute = 32,
		Locale = 64,
		Audio = 128,
		GLEW = 256
	};

	constexpr inline Init operator |(Init A, Init B)
	{
		return static_cast<Init>(static_cast<size_t>(A) | static_cast<size_t>(B));
	}

	enum class Preset : size_t
	{
		Game = (size_t)(Init::Core | Init::Debug | Init::Network | Init::SSL | Init::SDL2 | Init::Compute | Init::Locale | Init::Audio | Init::GLEW),
		App = (size_t)(Init::Core | Init::Debug | Init::Network | Init::SSL | Init::Compute | Init::Locale)
	};

	class ED_OUT_TS Library
	{
	public:
		static bool HasDirectX();
		static bool HasOpenGL();
		static bool HasOpenSSL();
		static bool HasGLEW();
		static bool HasZLib();
		static bool HasAssimp();
		static bool HasMongoDB();
		static bool HasPostgreSQL();
		static bool HasOpenAL();
		static bool HasSDL2();
		static bool HasSIMD();
        static bool HasRmlUI();
		static bool HasBullet3();
        static bool HasFContext();
		static bool HasWindowsEpoll();
		static int Version();
		static int DebugLevel();
		static int Architecture();
		static const char* Build();
		static const char* Compiler();
		static const char* Platform();
		static std::string Details();
	};

	ED_OUT bool Initialize(size_t Modules = (size_t)Preset::App);
	ED_OUT bool Uninitialize();
}
#endif
