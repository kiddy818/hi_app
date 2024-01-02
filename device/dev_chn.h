#ifndef dev_chn_include_h
#define dev_chn_include_h

#include "dev_sys.h"
#include "dev_vi_os04a10_liner.h"
#include "dev_vi_os04a10_2to1wdr.h"
#include "dev_venc.h"
#include "dev_log.h"

namespace hisilicon{namespace dev{

    class chn 
    {
        public:
            chn(const char* name);
            ~chn();

            bool start(int venc_w,int venc_h,int fr,int bitrate);
            void stop();

            static bool sys_init();
            static void sys_release();

            static void start_capture(bool enable);

        private:
            bool m_is_start;
            std::string m_name;
            std::shared_ptr<vi> m_vi_ptr;
            std::shared_ptr<venc> m_venc_main_ptr;
            std::shared_ptr<venc> m_venc_sub_ptr;
    };

}}//namespace

#endif
