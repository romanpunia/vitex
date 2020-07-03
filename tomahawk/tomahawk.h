#ifndef TOMAHAWK_H
#define TOMAHAWK_H
#define THAWK_MAJOR_VERSION 1
#define THAWK_MINOR_VERSION 8
#define THAWK_PATCH_LEVEL 3
#define THAWK_VERSION(X, Y, Z) ((X) * 1000 + (Y) * 100 + (Z))
#define THAWK_AT_LEAST(X, Y, Z) (THAWK_VERSION(THAWK_MAJOR_VERSION, THAWK_MINOR_VERSION, THAWK_PATCH_LEVEL) >= THAWK_VERSION(X, Y, Z))
#include "rest.h"
#include "compute.h"
#include "graphics.h"
#include "audio.h"
#include "audio/effects.h"
#include "audio/filters.h"
#include "network.h"
#include "network/http.h"
#include "network/smtp.h"
#include "network/bson.h"
#include "network/mongodb.h"
#include "script.h"
#include "script/interface.h"
#include "engine.h"
#include "engine/components.h"
#include "engine/processors.h"
#include "engine/renderers.h"
#include "engine/gui.h"

namespace Tomahawk
{
	enum TInit
	{
		TInit_Rest = 1,
		TInit_Logger = 2,
		TInit_Network = 4,
		TInit_Crypto = 8,
		TInit_SSL = 16,
		TInit_SDL2 = 32,
		TInit_Compute = 64,
		TInit_Locale = 128,
		TInit_Audio = 256,
		TInit_All = (TInit_Rest | TInit_Logger | TInit_Network | TInit_Crypto | TInit_SSL | TInit_SDL2 | TInit_Compute | TInit_Locale | TInit_Audio)
	};

	class THAWK_OUT Library
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
		static bool HasOpenAL();
		static bool HasSDL2();
		static int Version();
		static int DebugLevel();
		static int Architecture();
		static const char* Build();
		static const char* Compiler();
		static const char* Platform();
	};

	THAWK_OUT bool Initialize(unsigned int Modules = TInit_All);
	THAWK_OUT bool Uninitialize();
}
#endif