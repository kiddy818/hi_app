#include "dev_snap.h"
#include "dev_log.h"

namespace hisilicon{namespace dev{

    snap::snap()
    {
        m_pipe = 2;
        m_vpss_grp = 0x10;
        m_venc_chn = 0x10;
        m_bstart = false;

        m_venc_chn_attr.venc_attr.type = OT_PT_JPEG;
        m_venc_chn_attr.venc_attr.max_pic_width = 0;
        m_venc_chn_attr.venc_attr.max_pic_height = 0 ;
        m_venc_chn_attr.venc_attr.pic_width = 0;
        m_venc_chn_attr.venc_attr.pic_height = 0;
        m_venc_chn_attr.venc_attr.buf_size = 0; /* 2 is a number */
        m_venc_chn_attr.venc_attr.is_by_frame = TD_TRUE; /* get stream mode is field mode or frame mode */
        m_venc_chn_attr.venc_attr.profile = 0; 
        m_venc_chn_attr.venc_attr.jpeg_attr.dcf_en = TD_TRUE;
        m_venc_chn_attr.venc_attr.jpeg_attr.mpf_cfg.large_thumbnail_num = 0; 
        m_venc_chn_attr.venc_attr.jpeg_attr.recv_mode = OT_VENC_PIC_RECV_SINGLE;

        m_snap_attr.snap_type = OT_SNAP_TYPE_NORM;
        m_snap_attr.load_ccm_en = TD_TRUE;
        m_snap_attr.norm_attr.frame_cnt = 1;
        m_snap_attr.norm_attr.repeat_send_times = 1;
        m_snap_attr.norm_attr.zsl_en = TD_FALSE;
    }

    snap::~snap()
    {
        assert(!m_bstart);
    }

    ot_vi_pipe snap::get_pipe()
    {
        return m_pipe;
    }

    void snap::set_pipe(ot_vi_pipe pipe)
    {
        m_pipe = pipe;
    }

    bool snap::start(ot_vi_pipe_attr vi_pipe_attr,ot_vi_chn_attr vi_chn_attr,ot_vpss_grp_attr vpss_grp_attr,ot_vpss_chn_attr vpss_chn_attr)
    {
        td_s32 ret;
        ot_vi_chn vi_chn = 0;
        ot_vpss_chn vpss_chn = 0;

        ret = ss_mpi_vi_create_pipe(m_pipe,&vi_pipe_attr);
        if(ret != TD_SUCCESS)
        {
            DEV_WRITE_LOG_ERROR("ss_mpi_vi_create_pipe failed with %#x!", ret);
            return false;
        }

#if 0
        ret = ss_mpi_vi_start_pipe(m_pipe);
        if(ret != TD_SUCCESS)
        {
            DEV_WRITE_LOG_ERROR("ss_mpi_vi_start_pipe failed with %#x!", ret);
            return false;
        }

        ret = ss_mpi_vi_set_chn_attr(m_pipe,vi_chn,&vi_chn_attr);
        if(ret != TD_SUCCESS)
        {
            DEV_WRITE_LOG_ERROR("ss_mpi_vi_set_chn_attr failed with %#x!", ret);
            return false;
        }

        ret = ss_mpi_vi_enable_chn(m_pipe, vi_chn);
        if(ret != TD_SUCCESS)
        {
            DEV_WRITE_LOG_ERROR("ss_mpi_vi_enable_chn failed with %#x!", ret);
            return false;
        }
#endif

        //vi->vpss
        ot_mpp_chn src_chn;
        ot_mpp_chn dest_chn;
        src_chn.mod_id = OT_ID_VI;
        src_chn.dev_id = m_pipe;
        src_chn.chn_id = vi_chn;
        dest_chn.mod_id = OT_ID_VPSS;
        dest_chn.dev_id = m_vpss_grp;
        dest_chn.chn_id = vpss_chn;
        ret = ss_mpi_sys_bind(&src_chn, &dest_chn);
        if(ret != TD_SUCCESS)
        {
            DEV_WRITE_LOG_ERROR("ss_mpi_sys_bind failed with %#x", ret);
            return false;
        }

        //start vpss
        ret = ss_mpi_vpss_create_grp(m_vpss_grp,&vpss_grp_attr);
        if (ret != TD_SUCCESS)
        {
            DEV_WRITE_LOG_ERROR("ss_mpi_vpss_create_grp failed with %#x", ret);
            return false;
        }
        ret = ss_mpi_vpss_start_grp(m_vpss_grp);
        if (ret != TD_SUCCESS)
        {
            DEV_WRITE_LOG_ERROR("ss_mpi_vpss_start_grp failed with %#x", ret);
            return false;
        }

        ret = ss_mpi_vpss_set_chn_attr(m_vpss_grp, vpss_chn, &vpss_chn_attr);
        if (ret != TD_SUCCESS) 
        {
            DEV_WRITE_LOG_ERROR("ss_mpi_vpss_set_chn_attr failed with %#x", ret);
            return false;
        }
        ret = ss_mpi_vpss_enable_chn(m_vpss_grp, vpss_chn);
        if (ret != TD_SUCCESS) 
        {
            DEV_WRITE_LOG_ERROR("ss_mpi_vpss_enable_chn_attr failed with %#x", ret);
            return false;
        }

        m_venc_chn_attr.venc_attr.max_pic_width = vi_pipe_attr.size.width;
        m_venc_chn_attr.venc_attr.max_pic_height = vi_pipe_attr.size.height;
        m_venc_chn_attr.venc_attr.pic_width = vi_pipe_attr.size.width;
        m_venc_chn_attr.venc_attr.pic_height = vi_pipe_attr.size.height;
        m_venc_chn_attr.venc_attr.buf_size = m_venc_chn_attr.venc_attr.pic_width * m_venc_chn_attr.venc_attr.pic_height * 2;
        ret = ss_mpi_venc_create_chn(m_venc_chn, &m_venc_chn_attr);
        if(ret != TD_SUCCESS) 
        {
            DEV_WRITE_LOG_ERROR("ss_mpi_venc_create_chn failed with %#x", ret);
            return false;
        }

        ret = ss_mpi_venc_set_jpeg_enc_mode(m_venc_chn, OT_VENC_JPEG_ENC_SNAP);
        if(ret != TD_SUCCESS) 
        {
            DEV_WRITE_LOG_ERROR("ss_mpi_venc_set_jpeg_enc_mode failed with %#x", ret);
            return false;
        }

        //vpss->venc
        src_chn.mod_id = OT_ID_VPSS;
        src_chn.dev_id = m_vpss_grp;
        src_chn.chn_id = vpss_chn;
        dest_chn.mod_id = OT_ID_VENC;
        dest_chn.dev_id = 0;
        dest_chn.chn_id = m_venc_chn;
        ret = ss_mpi_sys_bind(&src_chn, &dest_chn);
        if(ret != TD_SUCCESS)
        {
            DEV_WRITE_LOG_ERROR("ss_mpi_sys_bind failed with %#x", ret);
            return false;
        }

        ot_venc_start_param start_param;
        start_param.recv_pic_num = -1;
        ret = ss_mpi_venc_start_chn(m_venc_chn, &start_param); 
        if(ret != TD_SUCCESS) 
        {
            DEV_WRITE_LOG_ERROR("ss_mpi_venc_start_chn failed with %#x", ret);
            return false;
        }

        ret = ss_mpi_snap_set_pipe_attr(m_pipe, &m_snap_attr);
        if(ret != TD_SUCCESS) 
        {
            DEV_WRITE_LOG_ERROR("ss_mpi_snap_set_pipe_attr failed with %#x", ret);
            return false;
        }

        ret = ss_mpi_snap_enable_pipe(m_pipe);
        if(ret != TD_SUCCESS) 
        {
            DEV_WRITE_LOG_ERROR("ss_mpi_snap_enable_pipe failed with %#x", ret);
            return false;
        }

        m_bstart = true;
        return true;
    }

    void snap::stop()
    {
        ss_mpi_snap_disable_pipe(m_pipe);

        //unbind vpss->venc
        ot_vpss_chn vpss_chn = 0;
        ot_vi_chn vi_chn = 0;

        ot_mpp_chn src_chn;
        ot_mpp_chn dest_chn;
        src_chn.mod_id = OT_ID_VPSS;
        src_chn.dev_id = m_vpss_grp;
        src_chn.chn_id = vpss_chn;
        dest_chn.mod_id = OT_ID_VENC;
        dest_chn.dev_id = 0;
        dest_chn.chn_id = m_venc_chn;
        ss_mpi_sys_unbind(&src_chn, &dest_chn);

        //stop venc
        ss_mpi_venc_stop_chn(m_venc_chn);
        ss_mpi_venc_destroy_chn(m_venc_chn);

        //unbind vi->vpss
        src_chn.mod_id = OT_ID_VI;
        src_chn.dev_id = m_pipe;
        src_chn.chn_id = vi_chn;
        dest_chn.mod_id = OT_ID_VPSS;
        dest_chn.dev_id = m_vpss_grp;
        dest_chn.chn_id = vpss_chn;
        ss_mpi_sys_unbind(&src_chn, &dest_chn);

        m_bstart = false;
    }

    bool snap::trigger(const char* path)
    {
        td_s32 ret;
        td_u32 i;
        td_s32 venc_fd;
        fd_set read_fds;
        struct timeval timeout_val;
        ot_venc_chn_status stat;
        ot_venc_stream stream;

        if(!m_bstart)
        {
            return false;
        }

        ret = ss_mpi_snap_trigger_pipe(m_pipe);
        if(ret != TD_SUCCESS)
        {
            DEV_WRITE_LOG_ERROR("ss_mpi_snap_trigger_pipe failed with %#x", ret);
            return false;
        }

        venc_fd = ss_mpi_venc_get_fd(m_venc_chn);
        if(venc_fd < 0)
        {
            DEV_WRITE_LOG_ERROR("ss_mpi_venc_get_fd failed with %#x", ret);
            return false;
        }

        FD_ZERO(&read_fds);
        FD_SET(venc_fd, &read_fds);
        timeout_val.tv_sec = 1;
        timeout_val.tv_usec = 0;
        ret = select(venc_fd + 1, &read_fds, NULL, NULL, &timeout_val);
        if(ret < 0)
        {
            DEV_WRITE_LOG_ERROR("select failed with %#x", ret);
            return false;
        }else if(ret == 0)
        {
            DEV_WRITE_LOG_ERROR("select timeout");
            return false;
        }

        ret = ss_mpi_venc_query_status(m_venc_chn, &stat);
        if(ret != TD_SUCCESS)
        {
            DEV_WRITE_LOG_ERROR("ss_mpi_venc_query_status failed with %#x", ret);
            return false;
        }

        if(stat.cur_packs == 0)
        {
            DEV_WRITE_LOG_ERROR("ss_mpi_venc_query_status cur_packs==0");
            return false;
        }

        stream.pack = (ot_venc_pack *)malloc(sizeof(ot_venc_pack) * stat.cur_packs);
        if (stream.pack == NULL)
        {
            DEV_WRITE_LOG_ERROR("malloc failed");
            return false;
        }
        stream.pack_cnt = stat.cur_packs;

        ret = ss_mpi_venc_get_stream(m_venc_chn, &stream, -1);
        if(ret != TD_SUCCESS)
        {
            free(stream.pack);
            DEV_WRITE_LOG_ERROR("ss_mpi_get_stream failed with %#x", ret);
            return false;
        }

        FILE* f = fopen(path,"wb");
        if(!f)
        {
            free(stream.pack);
            ss_mpi_venc_release_stream(m_venc_chn, &stream);
            return false;
        }
        for (i = 0; i < stream.pack_cnt; i++) 
        {
            fwrite(stream.pack[i].addr + stream.pack[i].offset,stream.pack[i].len - stream.pack[i].offset, 1,f);
        }

        fclose(f);
        ss_mpi_venc_release_stream(m_venc_chn, &stream);
        free(stream.pack);
        return true;
    }

    bool snap::is_start()
    {
        return m_bstart;
    }

}}//namespace
