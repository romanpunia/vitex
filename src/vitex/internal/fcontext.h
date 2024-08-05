#ifndef VI_INTERNAL_FCONTEXT
#define VI_INTERNAL_FCONTEXT
#include <stddef.h>

typedef void* fcontext_t;
typedef struct
{
    fcontext_t fctx;
    void *data;
} transfer_t;

extern "C" fcontext_t make_fcontext(void* sp, size_t size, void(*fn)(transfer_t));
extern "C" transfer_t jump_fcontext(const fcontext_t to, void* vp);
extern "C" transfer_t ontop_fcontext(const fcontext_t to, void* vp, transfer_t(*fn)(transfer_t));
#endif