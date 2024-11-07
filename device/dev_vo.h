#ifndef dev_vo_include_h
#define dev_vo_include_h

#include "dev_std.h"

namespace hisilicon{namespace dev{

    class vo
    {
        public:
            vo(int w,int h,int fr,int vo_dev);
            virtual ~vo();

            virtual bool start() = 0;
            virtual void stop() = 0;

            static bool init();
            static void release();

            int w();
            int h();
            int fr();
            int vo_dev();

        protected:
            int m_w;
            int m_h;
            int m_fr;
            bool m_is_start;
            ot_vo_dev m_vo_dev;
            ot_vo_layer m_vo_layer;
            ot_vo_chn m_vo_chn;
            ot_vo_pub_attr m_vo_pub_attr;
            ot_vo_video_layer_attr m_vo_layer_attr;
            ot_vo_chn_attr m_vo_chn_attr;
    };

}}//namespace

#endif
