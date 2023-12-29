#ifndef dev_vi_os04a10_liner_include_h
#define dev_vi_os04a10_liner_include_h

#include "dev_vi.h"

namespace hisilicon{namespace dev{

    class vi_os04a10_liner
        :public vi
    {
        public:
            vi_os04a10_liner(int w,int h,int src_fr,int mipi_dev,int sns_clk_src,int vi_dev);

            virtual ~vi_os04a10_liner();

            bool start();

            void stop();
    };

}}//namespace

#endif
