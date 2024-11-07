#include "dev_vo_bt1120.h"
#include "dev_log.h"

namespace hisilicon{namespace dev{

    vo_bt1120::vo_bt1120(int w,int h,int fr,int vo_dev,std::shared_ptr<vi> vi_ptr)
        :vo(w,h,fr,vo_dev),m_vi_ptr(vi_ptr)
    {
    }

    vo_bt1120::~vo_bt1120()
    {
    }

    bool vo_bt1120::start()
    {
        td_s32 ret;

        m_vo_pub_attr.intf_type = OT_VO_INTF_BT1120;
        if(m_w == 1920 && m_h == 1080 && m_fr == 60)
        {
            m_vo_pub_attr.intf_sync = OT_VO_OUT_1080P60;
        }
        else
        {
            DEV_WRITE_LOG_ERROR("unsupported intf_type(%dx%d@%d)",m_w,m_h,m_fr);
            return false;
        }
        m_vo_pub_attr.bg_color = 0xff;//blue

        if(!vo::start())
        {
            return false;
        }

        //vind vpss->vo
        ot_mpp_chn src_chn;
        ot_mpp_chn dest_chn;
        src_chn.mod_id = OT_ID_VPSS;
        src_chn.dev_id = m_vi_ptr->vpss_grp();
        src_chn.chn_id = m_vi_ptr->vpss_chn();

        dest_chn.mod_id = OT_ID_VO;
        dest_chn.dev_id = m_vo_layer;
        dest_chn.chn_id = m_vo_chn;
        ret = ss_mpi_sys_bind(&src_chn, &dest_chn);
        if(ret != TD_SUCCESS)
        {
            DEV_WRITE_LOG_ERROR("sys bind failed with error 0x%x",ret);
            vo::stop();
            return false;
        }

        return true;
    }

    void vo_bt1120::stop()
    {
        if(m_is_start)
        {
            /*unbind vpss->vo*/
            ot_mpp_chn src_chn;
            ot_mpp_chn dest_chn;
            src_chn.mod_id = OT_ID_VPSS;
            src_chn.dev_id = m_vi_ptr->vpss_grp();
            src_chn.chn_id = m_vi_ptr->vpss_chn();
            dest_chn.mod_id = OT_ID_VO;
            dest_chn.dev_id = m_vo_layer;
            dest_chn.chn_id = m_vo_chn;
            ss_mpi_sys_unbind(&src_chn, &dest_chn);

            vo::stop();
        }
    }
}}//namespace

