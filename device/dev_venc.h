#ifndef dev_venc_include_h
#define dev_venc_include_h

#include "dev_std.h"
#include <stream_observer.h>

namespace hisilicon{namespace dev{


    class venc;
    typedef std::shared_ptr<venc> venc_ptr;

    class venc
        :public ceanic::util::stream_obj
        ,public ceanic::util::stream_post
        ,public std::enable_shared_from_this<venc>
    {
        public:
            venc(int32_t chn,int32_t stream,int w,int h,int src_fr,int venc_fr,ot_vpss_grp vpss_grp,ot_vpss_chn vpss_chn);
            virtual ~venc();

            bool start(ot_vpss_grp vpss_grp, ot_vpss_chn vpss_chn);
            void stop();
            ot_venc_chn venc_chn();
            int venc_fd();
            int venc_w();
            int venc_h();
            int venc_fr();
            virtual void process_video_stream(ot_venc_stream* pstream) = 0;
            bool request_i_frame();
            
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
            venc_h264(int32_t chn,int32_t stream,int w,int h,int src_fr,int venc_fr,ot_vpss_grp vpss_grp,ot_vpss_chn vpss_chn);
            virtual ~venc_h264();

            virtual void process_video_stream(ot_venc_stream* pstream);
    };

    class venc_h264_cbr
        :public venc_h264
    {
        public:
            venc_h264_cbr(int32_t chn,int32_t stream,int w,int h,int src_fr,int venc_fr,ot_vpss_grp vpss_grp,ot_vpss_chn vpss_chn,int bitrate);
            virtual ~venc_h264_cbr();

        protected:
            int m_bitrate;
    };
    
    class venc_h264_avbr
        :public venc_h264
    {
        public:
            venc_h264_avbr(int32_t chn,int32_t stream,int w,int h,int src_fr,int venc_fr,ot_vpss_grp vpss_grp,ot_vpss_chn vpss_chn,int max_bitrate);
            virtual ~venc_h264_avbr();

        protected:
            int m_max_bitrate;
    };

    class venc_h265
        :public venc
    {
        public:
            venc_h265(int32_t chn,int32_t stream,int w,int h,int src_fr,int venc_fr,ot_vpss_grp vpss_grp,ot_vpss_chn vpss_chn);
            virtual ~venc_h265();

            virtual void process_video_stream(ot_venc_stream* pstream);
    };

    class venc_h265_cbr
        :public venc_h265
    {
        public:
            venc_h265_cbr(int32_t chn,int32_t stream,int w,int h,int src_fr,int venc_fr,ot_vpss_grp vpss_grp,ot_vpss_chn vpss_chn,int bitrate);
            virtual ~venc_h265_cbr();

        protected:
            int m_bitrate;
    };

    class venc_h265_avbr
        :public venc_h265
    {
        public:
            venc_h265_avbr(int32_t chn,int32_t stream,int w,int h,int src_fr,int venc_fr,ot_vpss_grp vpss_grp,ot_vpss_chn vpss_chn,int max_bitrate);
            virtual ~venc_h265_avbr();

        protected:
            int m_max_bitrate;
    };

}}//namespace

#endif


