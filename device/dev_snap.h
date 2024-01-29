#ifndef dev_snap_include_h
#define dev_snap_include_h

#include "dev_std.h"

namespace hisilicon{namespace dev{

    class snap 
    {
        public:
            snap();
            virtual ~snap();

            bool start(ot_vi_pipe_attr vi_pipe_attr,ot_vi_chn_attr vi_chn_attr,ot_vpss_grp_attr vpss_grp_attr,ot_vpss_chn_attr vpss_chn_attr);
            void stop();

            bool is_start();

            bool trigger(const char* path);

            ot_vi_pipe get_pipe();
            void set_pipe(ot_vi_pipe pipe);

        private:
            ot_vi_pipe m_pipe;
            ot_vpss_grp m_vpss_grp;
            ot_venc_chn m_venc_chn;
            ot_venc_chn_attr m_venc_chn_attr;
            bool m_bstart;
            ot_snap_attr m_snap_attr;
    };

}}//namespace

#endif
