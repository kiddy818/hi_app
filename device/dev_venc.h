#ifndef dev_venc_include_h
#define dev_venc_include_h

#include "dev_std.h"

namespace hisilicon{namespace dev{


    class venc;
    typedef std::shared_ptr<venc> venc_ptr;

    class venc
        :public std::enable_shared_from_this<venc>
    {
        public:
            venc(int w,int h,int src_fr,int venc_fr,ot_venc_chn venc_chn,ot_vpss_grp vpss_grp,ot_vpss_chn vpss_chn);
            virtual ~venc();

            bool start();
            void stop();
            ot_venc_chn venc_chn();
            int venc_fd();
            virtual void process_video_stream(ot_venc_stream* pstream) = 0;
            
            static bool start_capture();
            static void stop_capture();

        protected:
            static void on_capturing();

        protected:
            int m_venc_w;
            int m_venc_h;
            int m_src_fr;
            int m_venc_fr;
            ot_venc_chn_attr m_venc_chn_attr;
            ot_venc_chn m_venc_chn;
            ot_vpss_grp m_vpss_grp;
            ot_vpss_chn m_vpss_chn;
            int m_venc_fd;
            
            static bool g_is_capturing;
            static std::thread g_capture_thread;
            static std::list<venc_ptr> g_vencs;
    };

    class venc_h264_cbr
        :public venc
    {
        public:
            venc_h264_cbr(int w,int h,int src_fr,int venc_fr,ot_venc_chn venc_chn,ot_vpss_grp vpss_grp,ot_vpss_chn vpss_chn,int bitrate);
            ~venc_h264_cbr();

            virtual void process_video_stream(ot_venc_stream* pstream);

        protected:
            int m_bitrate;
    };

}}//namespace

#endif


