#ifndef dev_snap_include_h
#define dev_snap_include_h

#include "dev_std.h"
#include "dev_vi.h"
#include "dev_vi_isp.h"

namespace hisilicon{namespace dev{

    class snap 
    {
        public:
            snap(std::shared_ptr<vi> vi_ptr);
            virtual ~snap();

            bool start();
            void stop();

            bool is_start();

            bool trigger(const char* path,int quality);

            static ot_vi_pipe get_pipe();

        private:
            std::shared_ptr<vi> m_vi_ptr;
            ot_vi_pipe m_pipe;
            ot_vpss_grp m_vpss_grp;
            ot_venc_chn m_venc_chn;
            ot_venc_chn_attr m_venc_chn_attr;
            bool m_bstart;
            ot_snap_attr m_snap_attr;
    };

}}//namespace

#endif
