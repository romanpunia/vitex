#ifndef TOMAHAWK_H
#define TOMAHAWK_H
#define TH_MAJOR_VERSION 13
#define TH_MINOR_VERSION 8
#define TH_PATCH_LEVEL 56
#define TH_VERSION(X, Y, Z) ((X) * 1000 + (Y) * 100 + (Z))
#define TH_AT_LEAST(X, Y, Z) (TH_VERSION(TH_MAJOR_VERSION, TH_MINOR_VERSION, TH_PATCH_LEVEL) >= TH_VERSION(X, Y, Z))
#include "core/core.h"

namespace Tomahawk
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

	class TH_OUT_TS Library
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
		static bool WithProcessorSIMD();
        static bool WithWebBasedUI();
		static bool WithPhysicsEngine();
		static bool WithWindowsEpoll();
        static bool WithFastCoroutines();
		static int Version();
		static int DebugLevel();
		static int Architecture();
		static const char* Build();
		static const char* Compiler();
		static const char* Platform();
	};

	TH_OUT bool Initialize(size_t Modules = (size_t)Preset::App);
	TH_OUT bool Uninitialize();
}
#endif
