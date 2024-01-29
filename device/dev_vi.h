#ifndef dev_vi_include_h
#define dev_vi_include_h

#include "dev_std.h"

namespace hisilicon{namespace dev{

    class vi
    {
        public:
            vi(int w,int h,int src_fr,int vi_dev);
            virtual ~vi();

            virtual bool start() = 0;
            virtual void stop() = 0;
            virtual bool trigger(const char* path) = 0;

            static bool init();
            static void release();

            int w();
            int h();
            int fr();
            int vi_dev();
            std::vector<ot_vi_pipe> pipes();
            int vpss_grp();
            int vpss_chn();
            int vi_chn();

        protected:
            int m_w;
            int m_h;
            int m_src_fr;
            bool m_is_start;
            int m_vi_dev;

            std::vector<ot_vi_pipe> m_pipes;
            ot_vpss_grp m_vpss_grp;
            ot_vpss_chn m_vpss_chn;
            ot_vi_chn m_vi_chn;
    };

}}//namespace

#endif
