#ifndef dev_vi_yuv_include_h
#define dev_vi_yuv_include_h

#include "dev_vi.h"
//to support yuv input 
namespace hisilicon{namespace dev{

    class vi_yuv_
        :public vi
    {
        public:
            vi_yuv(int w,int h,int src_fr,int vi_dev);
            virtual ~vi_yuv();

            bool start() override;
            void stop() override;
    };

}}//namespace

#endif
