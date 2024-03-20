#ifndef dev_vi_os08a20_liner_include_h
#define dev_vi_os08a20_liner_include_h

#include "dev_vi_isp.h"

//to support os08a20 liner 
namespace hisilicon{namespace dev{

    class vi_os08a20_liner
        :public vi_isp
    {
        public:
            vi_os08a20_liner();

            virtual ~vi_os08a20_liner();
    };

}}//namespace

#endif
