#ifndef dev_sys_include_h
#define dev_sys_include_h

#include "dev_std.h"

namespace hisilicon{namespace dev{

    class sys
    {
        public:
            static bool init(ot_vi_vpss_mode_type mode);
            static void release();

            static ot_venc_chn alloc_venc_chn();
            static ot_rgn_handle alloc_rgn_handle();
            static void free_rgn_handle(ot_rgn_handle hdl);
    };

}}//namespace

#endif
