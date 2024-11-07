#include "dev_vo.h"
#include "dev_log.h"

namespace hisilicon{namespace dev{

    static const ot_vo_sync_info g_default_vo_timing =
    {
        /*
         * |--INTFACE---||-----TOP-----||----HORIZON--------||----BOTTOM-----||-PULSE-||-INVERSE-|
         * syncm, iop, itf,   vact, vbb,  vfb,  hact,  hbb,  hfb, hmid,bvact,bvbb, bvfb, hpw, vpw,idv, ihs, ivs
         */
        TD_FALSE, TD_TRUE, 1, 1080, 41, 4, 1920, 192, 88,  1,    1,   1,  1,  44, 5, TD_FALSE, TD_FALSE, TD_FALSE/* 1080P@60_hz */
    };

    vo::vo(int w,int h,int fr,int vo_dev)
        :m_w(w),m_h(h),m_fr(fr),m_is_start(false),m_vo_dev(vo_dev)
    {
        m_vo_layer = 0;
        m_vo_chn = 0;
        memset(&m_vo_pub_attr,0,sizeof(m_vo_pub_attr));

        m_vo_pub_attr.intf_type = OT_VO_INTF_HDMI;
        m_vo_pub_attr.intf_sync = OT_VO_OUT_1080P60;
        m_vo_pub_attr.bg_color = 0xff;
        m_vo_pub_attr.sync_info = g_default_vo_timing;//only valid when intf_sync == OT_VO_OUT_USER

        memset(&m_vo_layer_attr,0,sizeof(m_vo_layer_attr));
        m_vo_layer_attr.display_rect.width = m_w;
        m_vo_layer_attr.display_rect.height = m_h;
        m_vo_layer_attr.display_frame_rate = m_fr;
        m_vo_layer_attr.cluster_mode_en = TD_FALSE;
        m_vo_layer_attr.double_frame_en = TD_FALSE;
        m_vo_layer_attr.pixel_format = OT_PIXEL_FORMAT_YVU_SEMIPLANAR_420;
        m_vo_layer_attr.display_rect.x = 0;
        m_vo_layer_attr.display_rect.y = 0;
        m_vo_layer_attr.img_size.width = m_w;
        m_vo_layer_attr.img_size.height = m_h;
        m_vo_layer_attr.dst_dynamic_range = OT_DYNAMIC_RANGE_SDR8;
        m_vo_layer_attr.display_buf_len = 3;
        m_vo_layer_attr.partition_mode = OT_VO_PARTITION_MODE_SINGLE;
        m_vo_layer_attr.compress_mode = OT_COMPRESS_MODE_NONE;

        memset(&m_vo_chn_attr,0,sizeof(m_vo_chn_attr));
        m_vo_chn_attr.rect.x = 0;
        m_vo_chn_attr.rect.y = 0;
        m_vo_chn_attr.rect.width = m_w;
        m_vo_chn_attr.rect.height = m_h;
        m_vo_chn_attr.priority = 0;
        m_vo_chn_attr.deflicker_en = TD_FALSE;
    }

    vo::~vo()
    {
        assert(!m_is_start);
    }

    int vo::w()
    {
        return m_w;
    }

    int vo::h()
    {
        return m_h;
    }

    int vo::fr()
    {
        return m_fr;
    }

    int vo::vo_dev()
    {
        return m_vo_dev;
    }

    bool vo::start()
    {
        td_s32 ret;

        if(m_is_start)
        {
            return false;
        }

        ret = ss_mpi_vo_set_pub_attr(m_vo_dev,&m_vo_pub_attr);
        if(ret != TD_SUCCESS)
        {
            DEV_WRITE_LOG_ERROR("ss_mpi_vo_set_pub_attr failed with 0x%x",ret);
            return false;
        }

        ret = ss_mpi_vo_enable(m_vo_dev);
        if(ret != TD_SUCCESS)
        {
            DEV_WRITE_LOG_ERROR("ss_mpi_vo_enable failed with 0x%x",ret);
            return false;
        }

        ret = ss_mpi_vo_set_video_layer_attr(m_vo_layer, &m_vo_layer_attr);
        if(ret != TD_SUCCESS)
        {
            DEV_WRITE_LOG_ERROR("ss_mpi_vo_set_layer_attr failed with 0x%x",ret);
            ss_mpi_vo_disable(m_vo_dev);
            return false;
        }

        ret = ss_mpi_vo_enable_video_layer(m_vo_layer);
        if (ret != TD_SUCCESS)
        {
            DEV_WRITE_LOG_ERROR("ss_mpi_vo_enable_layer_attr failed with 0x%x",ret);
            ss_mpi_vo_disable(m_vo_dev);
            return false;
        }

        ret = ss_mpi_vo_set_chn_attr(m_vo_layer, m_vo_chn, &m_vo_chn_attr);
        if (ret != TD_SUCCESS)
        {
            DEV_WRITE_LOG_ERROR("ss_mpi_vo_set_chn_attr failed with 0x%x",ret);
            ss_mpi_vo_disable_video_layer(m_vo_layer);
            ss_mpi_vo_disable(m_vo_dev);
            return false;
        }

        ret = ss_mpi_vo_enable_chn(m_vo_layer, m_vo_chn);
        if (ret != TD_SUCCESS)
        {
            DEV_WRITE_LOG_ERROR("ss_mpi_vo_enable_chn failed with 0x%x",ret);
            ss_mpi_vo_disable_video_layer(m_vo_layer);
            ss_mpi_vo_disable(m_vo_dev);
            return false;
        }

        m_is_start = true;
        return true;
    }
    
    void vo::stop()
    {
        if(m_is_start)
        {
            ss_mpi_vo_disable_chn(m_vo_layer,m_vo_chn);
            ss_mpi_vo_disable_video_layer(m_vo_layer);
            ss_mpi_vo_disable(m_vo_dev);

            m_is_start = false;
        }
    }

    bool vo::init()
    {
        return true;
    }

    void vo::release()
    {
    }

}}//namespace

