#ifndef dev_vi_os04a10_liner_include_h
#define dev_vi_os04a10_liner_include_h

#include "dev_vi_isp.h"

//to support os04a10 liner 
namespace hisilicon{namespace dev{

    class vi_os04a10_liner
        :public vi_isp
    {
        public:
            vi_os04a10_liner();

            virtual ~vi_os04a10_liner();
    };

}}//namespace

#endif
