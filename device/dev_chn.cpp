#include "dev_chn.h"
#include "dev_log.h"
#include <rtsp/server.h>
#include <rtsp/stream/stream_manager.h>
#include <rtmp/session_manager.h>
#include <lt8618sx.h>

namespace hisilicon{namespace dev{

    ot_scene_param chn::g_scene_param;
    ot_scene_video_mode chn::g_scene_video_mode;
    std::shared_ptr<chn> chn::g_chns[MAX_CHANNEL];
    rate_auto_param chn::g_rate_auto_param;

    chn::chn(const char* vi_name,const char* venc_mode,int chn_no)
        :m_is_start(false),m_vi_name(vi_name),m_chn(chn_no),m_venc_mode(venc_mode)
    {
    }

    chn::~chn()
    {
        stop();
    }

    bool chn::start(int venc_w,int venc_h,int fr,int bitrate)
    {
        if(m_is_start)
        {
            return false;
        }

        if(m_vi_name == "OS04A10")
        {
            m_vi_ptr = std::make_shared<vi_os04a10_liner>();
        }
        else if(m_vi_name == "OS04A10_WDR")
        {
            m_vi_ptr = std::make_shared<vi_os04a10_2to1wdr>();
        }
        else if(m_vi_name == "OS08A20")
        {
            m_vi_ptr = std::make_shared<vi_os08a20_liner>();
        }
        else if(m_vi_name == "OS08A20_WDR")
        {
            m_vi_ptr = std::make_shared<vi_os08a20_2to1wdr>();
        }
        else
        {
            DEV_WRITE_LOG_ERROR("unsupport sensor name=%s",m_vi_name.c_str());
            return false;
        }

        if(venc_w > m_vi_ptr->w()
                || venc_h > m_vi_ptr->h()
                || fr > m_vi_ptr->fr())
        {
            DEV_WRITE_LOG_ERROR("invalid param");
            return false;
        }

        if(!m_vi_ptr->start())
        {
            DEV_WRITE_LOG_ERROR("vi start failed");
            m_vi_ptr.reset();
            return false;
        }

        if(m_venc_mode == "H264_CBR")
        {
            m_venc_main_ptr = std::make_shared<venc_h264_cbr>(venc_w,venc_h,m_vi_ptr->fr(),fr,m_vi_ptr->vpss_grp(),m_vi_ptr->vpss_chn(),bitrate);
            m_venc_sub_ptr  = std::make_shared<venc_h264_cbr>(704,576,m_vi_ptr->fr(),fr,m_vi_ptr->vpss_grp(),m_vi_ptr->vpss_chn(),1000);
        }
        else if(m_venc_mode == "H264_AVBR")
        {
            m_venc_main_ptr = std::make_shared<venc_h264_avbr>(venc_w,venc_h,m_vi_ptr->fr(),fr,m_vi_ptr->vpss_grp(),m_vi_ptr->vpss_chn(),bitrate);
            m_venc_sub_ptr  = std::make_shared<venc_h264_avbr>(704,576,m_vi_ptr->fr(),fr,m_vi_ptr->vpss_grp(),m_vi_ptr->vpss_chn(),1000);
        }
        else if(m_venc_mode == "H265_CBR")
        {
            m_venc_main_ptr = std::make_shared<venc_h265_cbr>(venc_w,venc_h,m_vi_ptr->fr(),fr,m_vi_ptr->vpss_grp(),m_vi_ptr->vpss_chn(),bitrate);
            m_venc_sub_ptr  = std::make_shared<venc_h265_cbr>(704,576,m_vi_ptr->fr(),fr,m_vi_ptr->vpss_grp(),m_vi_ptr->vpss_chn(),1000);
        }
        else if(m_venc_mode == "H265_AVBR")
        {
            m_venc_main_ptr = std::make_shared<venc_h265_avbr>(venc_w,venc_h,m_vi_ptr->fr(),fr,m_vi_ptr->vpss_grp(),m_vi_ptr->vpss_chn(),bitrate);
            m_venc_sub_ptr  = std::make_shared<venc_h265_avbr>(704,576,m_vi_ptr->fr(),fr,m_vi_ptr->vpss_grp(),m_vi_ptr->vpss_chn(),1000);
        }
        else
        {
            DEV_WRITE_LOG_ERROR("invalid venc mode");
            m_vi_ptr->stop();
            m_vi_ptr = nullptr;
            return false;
        }

        if(!m_venc_main_ptr->start()
                || !m_venc_sub_ptr->start())
        {
            DEV_WRITE_LOG_ERROR("venc start failed");
            m_venc_main_ptr->stop();
            m_venc_sub_ptr->stop();
            m_venc_main_ptr = nullptr;
            m_venc_sub_ptr = nullptr;

            m_vi_ptr->stop();
            m_vi_ptr = nullptr;
            return false;
        }

        m_osd_date_main = std::make_shared<osd_date>(32,32,64,m_venc_main_ptr->venc_chn());
        m_osd_date_sub = std::make_shared<osd_date>(16,16,24,m_venc_sub_ptr->venc_chn());
        m_osd_date_main->start();
        m_osd_date_sub->start();

        m_venc_main_ptr->register_stream_observer(shared_from_this());
        m_venc_sub_ptr->register_stream_observer(shared_from_this());

        g_chns[m_chn] = shared_from_this();
        m_is_start = true;
        return true;
    }

    void chn::stop()
    {
        if(!m_is_start)
        {
            return;
        }
        m_is_start = false;

        if(m_snap)
        {
            m_snap->stop();
        }

        if(m_vo)
        {
            m_vo->stop();
            m_vo = nullptr;
        }

        m_osd_date_main->stop();
        m_osd_date_sub->stop();

        m_venc_main_ptr->unregister_stream_observer(shared_from_this());
        m_venc_sub_ptr->unregister_stream_observer(shared_from_this());

        m_venc_main_ptr->stop();
        m_venc_sub_ptr->stop();
        m_vi_ptr->stop();

        m_venc_main_ptr = nullptr;
        m_venc_sub_ptr = nullptr;
        m_vi_ptr = nullptr;

        g_chns[m_chn] = nullptr;
    }

    bool chn::start_save(const char* file)
    {
        if(!m_is_start)
        {
            return false;
        }

        if(strstr(m_venc_mode.c_str(),"H264") != NULL)
        {
            m_save = std::make_shared<ceanic::stream_save::mp4_save>(ceanic::stream_save::MP4_SAVE_H264,file,m_venc_main_ptr->venc_w(),m_venc_main_ptr->venc_h(),m_venc_main_ptr->venc_fr());
        }
        else if(strstr(m_venc_mode.c_str(),"H265") != NULL)
        {
            m_save = std::make_shared<ceanic::stream_save::mp4_save>(ceanic::stream_save::MP4_SAVE_H265,file,m_venc_main_ptr->venc_w(),m_venc_main_ptr->venc_h(),m_venc_main_ptr->venc_fr());
        }
        else
        {
            return false;
        }

        return m_save->open();
    }

    void chn::stop_save()
    {
        if(m_save)
        {
            m_save->close();
            m_save = nullptr;
        }
    }

    void chn::start_capture(bool enable)
    {
        if(enable)
        {
            venc::start_capture();
        }
        else
        {
            venc::stop_capture();
        }
    }

    bool chn::init(ot_vi_vpss_mode_type mode)
    {
        if(!sys::init(mode))
        {
            return false;
        }

        osd::init();
        vi::init();
        vi_isp::init_hs_mode(LANE_DIVIDE_MODE_0);
        vo::init();

        return true;
    }

    void chn::release()
    {
        venc::release();
        vi::release();
        sys::release();
        osd::release();
    }

    void chn::on_stream_come(ceanic::util::stream_head* head, const char* buf, int len)
    {
        if(!m_is_start)
        {
            return;
        }

        int stream = 0;
        if(head->w == m_venc_main_ptr->venc_w()
                && head->h == m_venc_main_ptr->venc_h())
        {
            stream = 0;
        }
        else if(head->w == m_venc_sub_ptr->venc_w()
                && head->h == m_venc_sub_ptr->venc_h())
        {
            stream = 1;
        }
        else if(m_yolov5
                && head->w == m_yolov5->venc_w()
                && head->h == m_yolov5->venc_h())
        {
            //yolov5 stream
            stream = 2;
        }
        else
        {
            return;
        }

        if(strstr(m_venc_mode.c_str(),"H264") != NULL
                && stream < 2)
        {
            ceanic::rtmp::session_manager::instance()->process_data(m_chn,stream,head);
        }

        if(stream == 0
                && m_save)
        {
            m_save->input_data(head,NULL,0);
        }

        for(int i = 0; i < head->nalu_count; i++)
        {
            if(stream == 0 && 
                    i == 0)
            {
                td_u64 cur_pts;
                ss_mpi_sys_get_cur_pts(&cur_pts);
                int delay_ms = (cur_pts / 1000) - head->nalu[i].timestamp;
                DEV_WRITE_LOG_DEBUG("venc main stream %dms delayed",delay_ms);
            }

            head->nalu[i].data += 4;//remove 00 00 00 01
            head->nalu[i].size -= 4;
            head->nalu[i].timestamp *= 90;
        }

        ceanic::rtsp::stream_manager::instance()->process_data(m_chn,stream,head,NULL,0);
    }

    void chn::on_stream_error(int errno)
    {
    }

    bool chn::scene_init(const char* dir_path)
    {
        int ret = ot_scene_create_param(dir_path,&g_scene_param,&g_scene_video_mode);
        if(ret != TD_SUCCESS)
        {
            return false;
        }

        ret = ot_scene_init(&g_scene_param);
        if(ret != TD_SUCCESS)
        {
            return false;
        }

        return true;
    }

    bool chn::scene_set_mode(int mode)
    {
        int ret = ot_scene_set_scene_mode(&(g_scene_video_mode.video_mode[mode]));
        if(ret != TD_SUCCESS)
        {
            return false;
        }

        return true;
    }

    void chn::scene_release()
    {
        ot_scene_deinit();
    }

    bool chn::aiisp_start(const char* model_file,int mode)
    {
        if(m_aiisp_ptr)
        {
            return false;
        }

        if(!m_vi_ptr)
        {
            return false;
        }

        std::shared_ptr<vi_isp> viisp = std::dynamic_pointer_cast<vi_isp>(m_vi_ptr);
        if(!viisp)
        {
            return false;
        }

        switch(mode)
        {
            case 0://brn
                {
                    aiisp_bnr::init(model_file,viisp->isp_w(),viisp->isp_h(),viisp->wdr_mode());
                    m_aiisp_ptr = std::make_shared<aiisp_bnr>(viisp->pipes()[0]);
                    break;
                }

            case 1://drc
                {
                    aiisp_drc::init(model_file,viisp->isp_w(),viisp->isp_h(),viisp->wdr_mode());
                    m_aiisp_ptr = std::make_shared<aiisp_drc>(viisp->pipes()[0]);
                    break;
                }

            case 2://3dnr
                {
                    aiisp_3dnr::init(model_file,viisp->isp_w(),viisp->isp_h());
                    m_aiisp_ptr = std::make_shared<aiisp_3dnr>(viisp->pipes()[0]);
                    break;
                }

            default:
                {
                    return false;
                }
        }

        if(!m_aiisp_ptr->start())
        {
            m_aiisp_ptr.reset();
            return false;
        }

        return true;
    }

    void chn::aiisp_stop()
    {
        if(m_aiisp_ptr)
        {
            m_aiisp_ptr->stop();
            m_aiisp_ptr.reset();
        }

        aiisp_bnr::release();
        aiisp_drc::release();
        aiisp_3dnr::release();
    }

    bool chn::get_isp_exposure_info(isp_exposure_t* val)
    {
        if(!m_vi_ptr)
        {
            return false;
        }

        std::shared_ptr<vi_isp> viisp = std::dynamic_pointer_cast<vi_isp>(m_vi_ptr);
        if(!viisp)
        {
            return false;
        }

        return viisp->get_isp_exposure_info(val);
    }

    bool chn::is_start()
    {
        return m_is_start;
    }

    bool chn::request_i_frame(int chn,int stream)
    {
        if(chn >= MAX_CHANNEL)
        {
            return false;
        }

        std::shared_ptr<dev::chn> chn_ptr = g_chns[chn];
        if(!chn_ptr)
        {
            return false;
        }

        if(stream == 2)
        {
            //yolov5 stream
            return true;
        }
        else if(stream == 0)
        {
            chn_ptr->m_venc_main_ptr->request_i_frame();
        }
        else
        {
            chn_ptr->m_venc_sub_ptr->request_i_frame();
        }
        return true;
    }

    bool chn::get_stream_head(int chn,int stream,ceanic::util::media_head* mh)
    {
        if(chn >= MAX_CHANNEL)
        {
            return false;
        }

        using namespace ceanic::util;
        std::shared_ptr<dev::chn> chn_ptr = g_chns[chn];
        if(!chn_ptr)
        {
            return false;
        }

        memset(mh,0,sizeof(media_head));
        mh->media_fourcc = CEANIC_TAG;
        mh->ver = 1;
        if(stream == 2)
        {
            //yolov5 use h264
            mh->vdec = STREAM_ENCODE_H264;
        }
        else if(strstr(chn_ptr->m_venc_mode.c_str(),"H264") != NULL)
        {
            mh->vdec = STREAM_ENCODE_H264;
        }
        else if(strstr(chn_ptr->m_venc_mode.c_str(),"H265") != NULL)
        {
            mh->vdec = STREAM_ENCODE_H265;
        }
        else
        {
            assert(0);
        }
        return true;
    }

    bool chn::rate_auto_init(const char* file)
    {
        td_s32 ret = ot_rate_auto_load_param((td_char*)file,&g_rate_auto_param);
        if(ret != TD_SUCCESS)
        {
            DEV_WRITE_LOG_ERROR("ot_rate_auto_load_param failed with 0x%x",ret);
            return false;
        }

        ret = ot_rate_auto_init(&g_rate_auto_param);
        if(ret != TD_SUCCESS)
        {
            DEV_WRITE_LOG_ERROR("ot_rate_auto_load_init failed with 0x%x",ret);
            return false;
        }

        return true;
    }

    void chn::rate_auto_release()
    {
        td_s32 ret = ot_rate_auto_deinit();
        if (ret != TD_SUCCESS) 
        {
            DEV_WRITE_LOG_ERROR("ot_rate_auto_load_deinit failed with 0x%x",ret);
        }
    }

    bool chn::trigger_jpg(const char* file,int quality,const char* str_info)
    {
        if(!m_is_start)
        {
            return false;
        }

        if(quality <1)
        {
            quality = 1;
        }

        if(quality > 99)
        {
            quality = 99;
        }

        if(!m_snap)
        {
            m_snap = std::make_shared<snap>(m_vi_ptr);

            if(!m_snap->start())
            {
                m_snap = nullptr;
                return false;
            }
        }

        return m_snap->trigger(file,quality,str_info);
    }

    bool chn::yolov5_start(const char* model_file)
    {
        if(!m_is_start)
        {
            return false;
        }

        m_yolov5 = std::make_shared<yolov5>(m_vi_ptr,model_file);
        if(!m_yolov5->start())
        {
            m_yolov5 = nullptr;
            return false;
        }

        m_yolov5->register_stream_observer(shared_from_this());

        return true;
    }

    void chn::yolov5_stop()
    {
        if(m_yolov5)
        {
            m_yolov5->unregister_stream_observer(shared_from_this());
            m_yolov5->stop();
            m_yolov5 = nullptr;
        }
    }

    bool chn::vo_start(const char* intf_type,const char* intf_sync)
    {
        if(!m_is_start)
        {
            return false;
        }

        if(strcmp(intf_type,"BT1120") == 0
                && strcmp(intf_sync,"1080P60") == 0)
        {
            m_vo = std::make_shared<vo_bt1120>(1920,1080,60,0,m_vi_ptr);
        }
        else
        {
            DEV_WRITE_LOG_ERROR("invalid param,intf_type:%s,intf_sync:%s",intf_type,intf_sync);
            return false;
        }

        if(!m_vo->start())
        {
            DEV_WRITE_LOG_ERROR("vo start failed");
            m_vo = nullptr;
            return false;
        }

        //demo板子用了lt8618sx,bt1120->hdmi
        int fd = open("/dev/lt8618sx", O_RDONLY);
        if(fd > -1)
        {
            td_u32 lt_mode = OT_VO_OUT_1080P60;
            if(ioctl(fd,LT_CMD_SETMODE,&lt_mode) < 0)
            {
                DEV_WRITE_LOG_ERROR("lt8618sx set mode failed");
            }
            close(fd);
        }

        return true;
    }

    void chn::vo_stop()
    {
        if(m_vo)
        {
            m_vo->stop();
            m_vo = nullptr;
        }
    }
}}//namespace


