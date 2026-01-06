#ifndef dev_chn_include_h
#define dev_chn_include_h

/**
 * @file dev_chn.h
 * @brief Legacy single-camera channel management
 * 
 * @deprecated This class is part of the legacy single-camera architecture.
 * For new code, use camera_manager and camera_instance from cn_analyst/device/src/.
 * For migrating existing code, use dev_chn_wrapper which provides the same interface
 * but uses the new multi-camera architecture internally.
 * 
 * Migration Path:
 *   1. Replace hisilicon::dev::chn with hisilicon::dev::chn_wrapper
 *   2. Test backward compatibility
 *   3. Gradually refactor to use camera_manager directly
 * 
 * Status: LEGACY - Will be removed in Phase 3
 */

#include "dev_sys.h"
#include "dev_vi_os04a10_liner.h"
#include "dev_vi_os04a10_2to1wdr.h"
#include "dev_vi_os08a20_liner.h"
#include "dev_vi_os08a20_2to1wdr.h"
#include "dev_venc.h"
#include "dev_osd.h"
#include "dev_log.h"
#include <stream_observer.h>
#include "dev_vo.h"
#include "dev_vo_bt1120.h"

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

//mp4
#include <mp4_save.h>

//snap
#include "dev_snap.h"

//yolov5
#include "dev_svp_yolov5.h"

#define MAX_CHANNEL 1  // DEPRECATED: Will be removed in Phase 3, use camera_manager instead

namespace hisilicon{namespace dev{

#define MAIN_STREAM_ID 0
#define SUB_STREAM_ID 1
#define AI_STREAM_ID 2

    /**
     * @brief Legacy channel class for single-camera operation
     * @deprecated Use camera_instance from new architecture instead
     * 
     * For backward compatibility, consider using chn_wrapper which provides
     * the same interface but delegates to camera_manager/camera_instance.
     */
    class chn 
        :public ceanic::util::stream_observer
        ,public std:: enable_shared_from_this<chn>
    {
        public:
            chn(const char* vi_name,const char*venc_mode,int chn_no);
            ~chn();

            bool start(int venc_w,int venc_h,int fr,int bitrate);
            void stop();
            bool is_start();

            bool get_isp_exposure_info(isp_exposure_t* val);

            bool start_save(const char* file);
            void stop_save();

            bool trigger_jpg(const char* file,int quality,const char* str_info);

            static bool init(ot_vi_vpss_mode_type mode);
            static void release();

            static void start_capture(bool enable);

            void on_stream_come(ceanic::util::stream_obj_ptr sobj,ceanic::util::stream_head* head, const char* buf, int32_t len);
            void on_stream_error(ceanic::util::stream_obj_ptr sobj,int32_t error);

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


            //for yolov5
            bool yolov5_start(const char* model_file);
            void yolov5_stop();

            //for vo
            bool vo_start(const char* intf_type,const char* intf_sync);
            void  vo_stop();

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
            std::shared_ptr<ceanic::stream_save::stream_save> m_save;
            std::shared_ptr<snap> m_snap;
            std::shared_ptr<yolov5> m_yolov5;
            std::shared_ptr<vo> m_vo;

            static ot_scene_param g_scene_param;
            static ot_scene_video_mode g_scene_video_mode;
            static std::shared_ptr<chn> g_chns[MAX_CHANNEL];

            static rate_auto_param g_rate_auto_param;
    };

}}//namespace

#endif
