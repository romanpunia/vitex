#ifndef HAS_SHADER_BATCH
#define HAS_SHADER_BATCH

namespace shader_batch
{
	extern void foreach(void* context, void(*callback)(void*, const char*, const unsigned char*, unsigned));
}
#endif