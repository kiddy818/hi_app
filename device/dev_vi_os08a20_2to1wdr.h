#ifndef dev_vi_os08a20_2to1wdr_include_h
#define dev_vi_os08a20_2to1wdr_include_h

#include "dev_vi_isp.h"

//to support os08a20 2to1 wdr
namespace hisilicon{namespace dev{

    class vi_os08a20_2to1wdr
        :public vi_isp
    {
        public:
            vi_os08a20_2to1wdr();

            virtual ~vi_os08a20_2to1wdr();
    };

}}//namespace

#endif

