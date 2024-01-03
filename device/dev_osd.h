#ifndef dev_osd_include_h
#define dev_osd_include_h

#include "dev_std.h"

namespace hisilicon{namespace dev{

    class osd
    {
        public:
            osd(int x,int y,int font_size,ot_rgn_handle rgn_h,ot_venc_chn venc_h);

            virtual ~osd();

            static bool init();
            static void release();

            virtual bool start();
            virtual void stop();

        protected:
            int m_x;
            int m_y;
            int m_font_size;
            ot_rgn_handle m_rgn_h;
            ot_venc_chn m_venc_h;
            ot_rgn_chn_attr m_rgn_chn_attr; 
            ot_rgn_attr m_rgn_attr;
            bool m_is_start;
    };

    class osd_date
        :public osd
    {
        public:
            osd_date(int x,int y,int font_size,ot_rgn_handle rgn_h,ot_venc_chn venc_h);
            virtual ~osd_date();

            virtual bool start() override;
            virtual void stop() override;

        protected:
            void on_refresh();

            std::thread m_thd;
            char m_last_date_str[64];
    };

}}//namespace

#endif
