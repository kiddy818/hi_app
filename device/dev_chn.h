#ifndef dev_chn_include_h
#define dev_chn_include_h

#include "dev_sys.h"
#include "dev_vi_os04a10_liner.h"
#include "dev_vi_os04a10_2to1wdr.h"
#include "dev_venc.h"
#include "dev_osd.h"
#include "dev_log.h"
#include <stream_observer.h>

//for scene
extern "C"
{
#include <ot_scene.h>
#include <scene_loadparam.h>
}

//for svc rate auto
extern "C"
{
#include <ot_bitrate_auto.h>
}

//aiisp
#include <aiisp_bnr.h>
#include <aiisp_drc.h>
#include <aiisp_3dnr.h>

#define MAX_CHANNEL 1

namespace hisilicon{namespace dev{

    class chn 
        :public ceanic::rtsp::stream_observer
         ,public std:: enable_shared_from_this<chn>
    {
        public:
            chn(const char* vi_name,const char*venc_mode,int chn_no);
            ~chn();

            bool start(int venc_w,int venc_h,int fr,int bitrate);
            void stop();
            bool is_start();

            bool get_isp_exposure_info(isp_exposure_t* val);

            static bool init();
            static void release();

            static void start_capture(bool enable);

            void on_stream_come(ceanic::util::stream_head* head, const char* buf, int len);
            void on_stream_error(int errno);

            //for scene
            static bool scene_init(const char* dir_path);
            static bool scene_set_mode(int mode);
            static void scene_release();

            //for svc rate auto
            //use venc_chn=0(../svc_rate_auto/src/ot_bitrate_auto.c)
            static bool rate_auto_init(const char* file);
            static void rate_auto_release();

            //for aiisp
            bool aiisp_start(const char* model_file,int mode);
            void aiisp_stop();

            static bool get_stream_head(int chn,int stram,ceanic::util::media_head* mh);
            static bool request_i_frame(int chn,int stream);

        private:
            bool m_is_start;
            std::string m_vi_name;
            std::shared_ptr<vi> m_vi_ptr;
            std::shared_ptr<venc> m_venc_main_ptr;
            std::shared_ptr<venc> m_venc_sub_ptr;
            std::shared_ptr<aiisp> m_aiisp_ptr;
            std::shared_ptr<osd_date> m_osd_date_main;
            std::shared_ptr<osd_date> m_osd_date_sub;
            int m_chn;
            std::string m_venc_mode;

            static ot_scene_param g_scene_param;
            static ot_scene_video_mode g_scene_video_mode;
            static std::shared_ptr<chn> g_chns[MAX_CHANNEL];

            static rate_auto_param g_rate_auto_param;
    };

}}//namespace

#endif
