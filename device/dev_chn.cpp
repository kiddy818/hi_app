#include "dev_chn.h"
#include "dev_log.h"

namespace hisilicon{namespace dev{

    chn::chn(const char* name)
        :m_is_start(false),m_name(name)
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
        if(!m_venc_main_ptr->start())
        {
            DEV_WRITE_LOG_ERROR("venc start failed");
            m_venc_main_ptr.reset();
            m_vi_ptr->stop();
            m_vi_ptr.reset();
            return false;
        }

        m_is_start = true;
        return true;
    }

    void chn::stop()
    {
        if(!m_is_start)
        {
            return;
        }

        m_venc_main_ptr->stop();
        m_vi_ptr->stop();
        m_venc_main_ptr.reset();
        m_vi_ptr.reset();
        m_is_start = false;
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

    bool chn::sys_init()
    {
        if(!sys::init())
        {
            return false;
        }

        vi_isp::init_hs_mode(LANE_DIVIDE_MODE_0);

        return true;
    }

    void chn::sys_release()
    {
        return sys::release();
    }

}}//namespace


