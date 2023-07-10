#include "fcontext.h"
#ifdef ED_USE_FCTX
extern "C" transfer_t ontop_fcontext_tail(int ignore, void* vp, transfer_t (*fn)(transfer_t), fcontext_t const from)
{
    return fn(transfer_t { from, vp });
}
#endif
