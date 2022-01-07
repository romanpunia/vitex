#ifndef _FCONTEXT_H_INCLUDED
#define _FCONTEXT_H_INCLUDED

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void *fcontext_t;

typedef struct  {
    fcontext_t fctx;
    void *data;
} transfer_t;

fcontext_t make_fcontext(void * sp, size_t size, void(*fn)(transfer_t));
transfer_t jump_fcontext(fcontext_t const to, void *vp);
transfer_t ontop_fcontext(fcontext_t const to, void *vp, transfer_t(*fn)(transfer_t));

#ifdef __cplusplus
}
#endif

#endif //!_FCONTEXT_H_INCLUDED