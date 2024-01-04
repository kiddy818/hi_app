#include "dev_chn.h"
#include "dev_log.h"
#include <rtsp/server.h>
#include <rtsp/stream/stream_manager.h>
#include <rtmp/session_manager.h>

namespace hisilicon{namespace dev{

    ot_scene_param chn::g_scene_param;
    ot_scene_video_mode chn::g_scene_video_mode;

    chn::chn(const char* name,int chn_no)
        :m_is_start(false),m_name(name),m_chn(chn_no)
    {
    }

    chn::~chn()
    {
    }

    bool chn::start(int venc_w,int venc_h,int fr,int bitrate)
    {
        if(m_is_start)
        {
            return false;
        }

        if(m_name == "OS04A10")
        {
            m_vi_ptr = std::make_shared<vi_os04a10_liner>();
        }
        else if(m_name == "OS04A10_WDR")
        {
            m_vi_ptr = std::make_shared<vi_os04a10_2to1wdr>();
        }
        else
        {
            DEV_WRITE_LOG_ERROR("unsupport sensor name=%s",m_name.c_str());
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

        m_venc_main_ptr = std::make_shared<venc_h264_cbr>(venc_w,venc_h,m_vi_ptr->fr(),fr,m_vi_ptr->vi_dev(),m_vi_ptr->vpss_grp(),m_vi_ptr->vpss_chn(),bitrate);
        m_venc_sub_ptr  = std::make_shared<venc_h264_cbr>(704,576,m_vi_ptr->fr(),fr,m_venc_main_ptr->venc_chn() + 1,m_vi_ptr->vpss_grp(),m_vi_ptr->vpss_chn(),1000);
        if(!m_venc_main_ptr->start()
                || !m_venc_sub_ptr->start())
        {
            DEV_WRITE_LOG_ERROR("venc start failed");
            m_venc_main_ptr->stop();
            m_venc_sub_ptr->stop();
            m_venc_main_ptr.reset();
            m_venc_sub_ptr.reset();

            m_vi_ptr->stop();
            m_vi_ptr.reset();
            return false;
        }

        m_osd_date_main = std::make_shared<osd_date>(32,32,64,m_venc_main_ptr->venc_chn(),m_venc_main_ptr->venc_chn());
        m_osd_date_sub = std::make_shared<osd_date>(16,16,24,m_venc_sub_ptr->venc_chn(),m_venc_sub_ptr->venc_chn());
        m_osd_date_main->start();
        m_osd_date_sub->start();

        m_venc_main_ptr->register_stream_observer(shared_from_this());
        m_venc_sub_ptr->register_stream_observer(shared_from_this());
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

        m_osd_date_main->stop();
        m_osd_date_sub->stop();

        m_venc_main_ptr->unregister_stream_observer(shared_from_this());
        m_venc_sub_ptr->unregister_stream_observer(shared_from_this());

        m_venc_main_ptr->stop();
        m_venc_sub_ptr->stop();
        m_vi_ptr->stop();

        m_venc_main_ptr.reset();
        m_venc_sub_ptr.reset();
        m_vi_ptr.reset();
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

    bool chn::init()
    {
        if(!sys::init())
        {
            return false;
        }

        osd::init();
        vi::init();
        vi_isp::init_hs_mode(LANE_DIVIDE_MODE_0);

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

        for(int i = 0; i < head->nalu_count; i++)
        {
            head->nalu[i].data += 4;//remove 00 00 00 01
            head->nalu[i].size -= 4;
        }

        int stream = 0;
        if(head->w == m_venc_main_ptr->venc_w()
                && head->h == m_venc_main_ptr->venc_h())
        {
            stream = 0;
        }
        else
        {
            stream = 1;
        }

        ceanic::rtsp::stream_manager::instance()->process_data(m_chn,stream,head,NULL,0);
        ceanic::rtmp::session_manager::instance()->process_data(m_chn,stream,head,NULL,0);
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
                    aiisp_bnr::init(model_file,viisp->w(),viisp->h(),viisp->wdr_mode());
                    m_aiisp_ptr = std::make_shared<aiisp_bnr>(viisp->pipes()[0]);
                    break;
                }

            case 1://drc
                {
                    aiisp_drc::init(model_file,viisp->w(),viisp->h(),viisp->wdr_mode());
                    m_aiisp_ptr = std::make_shared<aiisp_drc>(viisp->pipes()[0]);
                    break;
                }

            case 2://3dnr
                {
                    aiisp_3dnr::init(model_file,viisp->w(),viisp->h());
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

}}//namespace


