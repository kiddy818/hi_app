#ifndef dev_sys_include_h
#define dev_sys_include_h

#include "dev_std.h"

namespace hisilicon{namespace dev{

    class sys
    {
        public:
            static bool init();
            static void release();

            static ot_venc_chn alloc_venc_chn();
            static ot_rgn_handle alloc_rgn_handle();
    };

}}//namespace

#endif
