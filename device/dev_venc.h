#ifndef dev_venc_include_h
#define dev_venc_include_h

#include "dev_std.h"
#include <stream_observer.h>

namespace hisilicon{namespace dev{


    class venc;
    typedef std::shared_ptr<venc> venc_ptr;

    class venc
        :public std::enable_shared_from_this<venc>,
        public beacon::rtsp::stream_post
    {
        public:
            venc(int w,int h,int src_fr,int venc_fr,ot_venc_chn venc_chn,ot_vpss_grp vpss_grp,ot_vpss_chn vpss_chn);
            virtual ~venc();

            bool start();
            void stop();
            ot_venc_chn venc_chn();
            int venc_fd();
            int venc_w();
            int venc_h();
            virtual void process_video_stream(ot_venc_stream* pstream) = 0;
            
            static bool start_capture();
            static void stop_capture();

            static bool init();
            static void release();

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

    class venc_h264
        :public venc
    {
        public:
            venc_h264(int w,int h,int src_fr,int venc_fr,ot_venc_chn venc_chn,ot_vpss_grp vpss_grp,ot_vpss_chn vpss_chn);
            virtual ~venc_h264();

            virtual void process_video_stream(ot_venc_stream* pstream);
    };

    class venc_h264_cbr
        :public venc_h264
    {
        public:
            venc_h264_cbr(int w,int h,int src_fr,int venc_fr,ot_venc_chn venc_chn,ot_vpss_grp vpss_grp,ot_vpss_chn vpss_chn,int bitrate);
            virtual ~venc_h264_cbr();

        protected:
            int m_bitrate;
    };

}}//namespace

#endif


