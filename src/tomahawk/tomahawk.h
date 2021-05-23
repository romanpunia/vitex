#ifndef TOMAHAWK_H
#define TOMAHAWK_H
#define TH_MAJOR_VERSION 3
#define TH_MINOR_VERSION 1
#define TH_PATCH_LEVEL 1
#define TH_VERSION(X, Y, Z) ((X) * 1000 + (Y) * 100 + (Z))
#define TH_AT_LEAST(X, Y, Z) (TH_VERSION(TH_MAJOR_VERSION, TH_MINOR_VERSION, TH_PATCH_LEVEL) >= TH_VERSION(X, Y, Z))
#include "core/core.h"
#include "core/compute.h"
#include "core/graphics.h"
#include "core/audio.h"
#include "core/network.h"
#include "core/script.h"
#include "core/engine.h"
#include "network/http.h"
#include "network/smtp.h"
#include "network/mdb.h"
#include "network/pdb.h"
#include "audio/effects.h"
#include "audio/filters.h"
#include "engine/components.h"
#include "engine/processors.h"
#include "engine/renderers.h"
#include "engine/gui.h"
#include "script/std-lib.h"
#include "script/core-lib.h"
#include "script/gui-lib.h"

namespace Tomahawk
{
	enum TInit
	{
		TInit_Core = 1,
		TInit_Debug = 2,
		TInit_Network = 4,
		TInit_SSL = 8,
		TInit_SDL2 = 16,
		TInit_Compute = 32,
		TInit_Locale = 64,
		TInit_Audio = 128,
		TInit_GLEW = 256
	};

	enum TPreset
	{
		TPreset_Game = (TInit_Core | TInit_Debug | TInit_Network | TInit_SSL | TInit_SDL2 | TInit_Compute | TInit_Locale | TInit_Audio | TInit_GLEW),
		TPreset_App = (TInit_Core | TInit_Debug | TInit_Network | TInit_SSL | TInit_Compute | TInit_Locale)
	};

	class TH_OUT Library
	{
	public:
		static void Describe();
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
		static bool WithSIMD();
		static bool WithBullet3();
		static bool WithRmlUi();
		static bool WithWepoll();
		static int Version();
		static int DebugLevel();
		static int Architecture();
		static const char* Build();
		static const char* Compiler();
		static const char* Platform();
	};

	TH_OUT bool Initialize(unsigned int Modules = TPreset_App);
	TH_OUT bool Uninitialize();
}
#endif