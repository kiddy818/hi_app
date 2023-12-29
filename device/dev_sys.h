#ifndef dev_sys_include_h
#define dev_sys_include_h

#include "dev_std.h"

namespace hisilicon{namespace dev{

    class sys
    {
        public:
            static bool init();
            static void release();
    };

}}//namespace

#endif
