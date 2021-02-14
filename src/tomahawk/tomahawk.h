#ifndef TOMAHAWK_H
#define TOMAHAWK_H
#define TH_MAJOR_VERSION 2
#define TH_MINOR_VERSION 6
#define TH_PATCH_LEVEL 1
#define TH_VERSION(X, Y, Z) ((X) * 1000 + (Y) * 100 + (Z))
#define TH_AT_LEAST(X, Y, Z) (TH_VERSION(TH_MAJOR_VERSION, TH_MINOR_VERSION, TH_PATCH_LEVEL) >= TH_VERSION(X, Y, Z))
#include "core/rest.h"
#include "core/compute.h"
#include "core/graphics.h"
#include "core/audio.h"
#include "audio/effects.h"
#include "audio/filters.h"
#include "core/network.h"
#include "network/http.h"
#include "network/smtp.h"
#include "network/mdb.h"
#include "core/script.h"
#include "script/core-api.h"
#include "script/rest-api.h"
#include "script/gui-api.h"
#include "core/engine.h"
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
		TInit_GLEW = 512,
		TInit_All = (TInit_Rest | TInit_Logger | TInit_Network | TInit_Crypto | TInit_SSL | TInit_SDL2 | TInit_Compute | TInit_Locale | TInit_Audio | TInit_GLEW)
	};

	enum TMem
	{
		TMem_Heap = 0,
		TMem_1MB = 1024 * 1024 * 1,
		TMem_2MB = 1024 * 1024 * 2,
		TMem_4MB = 1024 * 1024 * 4,
		TMem_8MB = 1024 * 1024 * 8,
		TMem_16MB = 1024 * 1024 * 16,
		TMem_32MB = 1024 * 1024 * 32,
		TMem_64MB = 1024 * 1024 * 64,
		TMem_128MB = 1024 * 1024 * 128,
		TMem_256MB = 1024 * 1024 * 256,
		TMem_512MB = 1024 * 1024 * 512,
		TMem_1GB = 1024 * 1024 * 1024
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
		static int Version();
		static int DebugLevel();
		static int Architecture();
		static const char* Build();
		static const char* Compiler();
		static const char* Platform();
	};

	TH_OUT bool Initialize(unsigned int Modules = TInit_All, size_t HeapSize = TMem_Heap);
	TH_OUT bool Uninitialize();
}
#endif