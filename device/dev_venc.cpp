#include "dev_venc.h"
#include "dev_log.h"

namespace hisilicon{namespace dev{

    bool venc::g_is_capturing = false;
    std::thread venc::g_capture_thread;
    std::list<venc_ptr> venc::g_vencs;

    venc::venc(int w,int h,int src_fr,int venc_fr,ot_venc_chn venc_chn,ot_vpss_grp vpss_grp,ot_vpss_chn vpss_chn)
        :m_venc_w(w),m_venc_h(h),m_src_fr(src_fr),m_venc_fr(venc_fr),m_venc_chn(venc_chn),m_vpss_grp(vpss_grp),m_vpss_chn(vpss_chn)
    {
    }

    venc::~venc()
    {
    }

    bool venc::init()
    {
        return true;
    }

    void venc::release()
    {
    }

    bool venc::request_i_frame()
    {
        td_s32 ret = ss_mpi_venc_request_idr(m_venc_chn,TD_TRUE);
        if(ret != TD_SUCCESS)
        {
            DEV_WRITE_LOG_ERROR("ss_mpi_venc_request_id faild with%#x!", ret);
            return false;
        }

        return true;
    }

    ot_venc_chn venc::venc_chn()
    {
        return m_venc_chn;
    }

    int venc::venc_fd()
    {
        return m_venc_fd;
    }

    int venc::venc_w()
    {
        return m_venc_w;
    }

    int venc::venc_h()
    {
        return m_venc_h;
    }

    void venc::on_capturing()
    {
        fd_set read_fds;
        td_s32 maxfd = 0;
        struct timeval time_val;
        td_s32 ret;
        ot_venc_stream stream;
        ot_venc_chn_status stat;
        ot_venc_chn venc_chn;
        int venc_fd;

        while(g_is_capturing)
        {
            FD_ZERO(&read_fds);

            for(auto it = g_vencs.begin(); it != g_vencs.end(); it++)
            {
                venc_fd = (*it)->venc_fd();
                FD_SET(venc_fd, &read_fds);
                if(venc_fd > maxfd)
                {
                    maxfd = venc_fd;
                }
            }

            time_val.tv_sec  = 2;
            time_val.tv_usec = 0;
            ret = select(maxfd + 1, &read_fds, NULL, NULL, &time_val);
            if (ret < 0)
            {
                DEV_WRITE_LOG_ERROR("select faild with %#x!", ret);
                break;
            }
            else if (ret == 0)
            {
                DEV_WRITE_LOG_ERROR("get venc stream time out, exit thread");
                continue;
            }

            for(auto it = g_vencs.begin(); it != g_vencs.end(); it++)
            {
                venc_fd = (*it)->venc_fd();
                if(!FD_ISSET(venc_fd,&read_fds))
                {
                    continue;
                }

                venc_chn = (*it)->venc_chn();
                memset(&stream, 0, sizeof(stream));
                ret = ss_mpi_venc_query_status(venc_chn, &stat);
                if (ret != TD_SUCCESS)
                {
                    DEV_WRITE_LOG_ERROR("ss_mpi_venc_query_status failed with %#x", ret);
                    continue;
                }
                if(stat.cur_packs == 0)
                {
                    DEV_WRITE_LOG_ERROR("NOTE: Current  frame is NULL!");
                    continue;
                }

                stream.pack = (ot_venc_pack*)malloc(sizeof(ot_venc_pack) * stat.cur_packs);
                if (stream.pack == NULL)
                {
                    DEV_WRITE_LOG_ERROR("malloc memory failed!");
                    break;
                }
                stream.pack_cnt = stat.cur_packs;
                ret = ss_mpi_venc_get_stream(venc_chn, &stream, TD_TRUE);
                if (ret != TD_SUCCESS)
                {
                    free(stream.pack);
                    stream.pack = NULL;
                    DEV_WRITE_LOG_ERROR("ss_mpi_venc_get_stream failed with %#x", ret);
                    break;
                }

                (*it)->process_video_stream(&stream);

                ss_mpi_venc_release_stream(venc_chn, &stream);
                free(stream.pack);
                stream.pack = NULL;
            }
        }
    }

    bool venc::start_capture()
    {
        if(g_is_capturing)
        {
            return false;
        }

        g_is_capturing = true;
        g_capture_thread = std::thread(&on_capturing);
        return true;
    }

    void venc::stop_capture()
    {
        if(!g_is_capturing)
        {
            return;
        }

        g_is_capturing = false;
        g_capture_thread.join();
    }

    bool venc::start()
    {
        td_s32 ret;
        ret = ss_mpi_venc_create_chn(m_venc_chn,&m_venc_chn_attr);
        if(ret != TD_SUCCESS)
        {
            DEV_WRITE_LOG_ERROR("ss_mpi_venc_create_chn[%d] faild with %#x!",m_venc_chn, ret);
            return false;
        }
        m_venc_fd = ss_mpi_venc_get_fd(m_venc_chn);

        ot_mpp_chn src_chn;
        ot_mpp_chn dest_chn;
        src_chn.mod_id = OT_ID_VPSS;
        src_chn.dev_id = m_vpss_grp;
        src_chn.chn_id = m_vpss_chn;
        dest_chn.mod_id = OT_ID_VENC;
        dest_chn.dev_id = 0;
        dest_chn.chn_id = m_venc_chn;
        ret = ss_mpi_sys_bind(&src_chn, &dest_chn);
        if(ret != TD_SUCCESS)
        {
            DEV_WRITE_LOG_ERROR("ss_mpi_sys_bind failed with %#x",ret);
            return false;
        }

        ot_venc_start_param venc_start_param;
        venc_start_param.recv_pic_num = -1;
        ret = ss_mpi_venc_start_chn(m_venc_chn,&venc_start_param);
        if(ret != TD_SUCCESS)
        {
            DEV_WRITE_LOG_ERROR("HI_MPI_VENC_StartRecvPic faild with%#x!", ret);
            return false;
        }

        g_vencs.push_back(shared_from_this());
        return true;
    }

    void venc::stop()
    {
        ot_mpp_chn src_chn;
        ot_mpp_chn dest_chn;
        src_chn.mod_id = OT_ID_VPSS;
        src_chn.dev_id = m_vpss_grp;
        src_chn.chn_id = m_vpss_chn;
        dest_chn.mod_id = OT_ID_VENC;
        dest_chn.dev_id = 0;
        dest_chn.chn_id = m_venc_chn;

        ss_mpi_sys_unbind(&src_chn, &dest_chn);
        ss_mpi_venc_stop_chn(m_venc_chn);
        ss_mpi_venc_destroy_chn(m_venc_chn);

        for (auto it = g_vencs.begin();it != g_vencs.end(); it++)
        {
            if((*it)->venc_chn() == m_venc_chn)
            {
                g_vencs.erase(it);
                break;
            }
        }
    }

    venc_h264::venc_h264(int w,int h,int src_fr,int venc_fr,ot_venc_chn venc_chn,ot_vpss_grp vpss_grp,ot_vpss_chn vpss_chn)
        :venc(w,h,src_fr,venc_fr,venc_chn,vpss_grp,vpss_chn)
    {
        memset(&m_venc_chn_attr,0,sizeof(m_venc_chn_attr));
        m_venc_chn_attr.venc_attr.type = OT_PT_H264;
        m_venc_chn_attr.venc_attr.max_pic_width = m_venc_w;
        m_venc_chn_attr.venc_attr.max_pic_height = m_venc_h;
        m_venc_chn_attr.venc_attr.pic_width = m_venc_w;/*the picture width*/
        m_venc_chn_attr.venc_attr.pic_height    = m_venc_h;/*the picture height*/
        m_venc_chn_attr.venc_attr.buf_size      = m_venc_w * m_venc_h  *3 / 2;/*stream buffer size*/
        m_venc_chn_attr.venc_attr.is_by_frame      = TD_TRUE;/*get stream mode is slice mode or frame mode?*/
        m_venc_chn_attr.venc_attr.profile = 0;
        m_venc_chn_attr.venc_attr.h264_attr.rcn_ref_share_buf_en = TD_FALSE;
        m_venc_chn_attr.venc_attr.h264_attr.frame_buf_ratio = 70;
        m_venc_chn_attr.gop_attr.gop_mode = OT_VENC_GOP_MODE_NORMAL_P;
        m_venc_chn_attr.gop_attr.normal_p.ip_qp_delta = 2; /* 2 is a number */
    }

    venc_h264::~venc_h264()
    {
    }

    void venc_h264::process_video_stream(ot_venc_stream* pstream)
    {
        char* es_buf = NULL;
        int es_len = 0;
        int es_type = 0;
        unsigned long long time_stamp = 0;

        ceanic::util::stream_head sh;

        memset(&sh,0,sizeof(sh));
        sh.type = STREAM_NALU_SLICE;    
        sh.tag = CEANIC_TAG;
        sh.sys_time = time(NULL);  
        sh.nalu_count = pstream->pack_cnt;         
        sh.w = m_venc_w;
        sh.h = m_venc_h;

        for(unsigned int i = 0; i < pstream->pack_cnt; i++)
        {
            es_buf = (char*)(pstream->pack[i].addr + pstream->pack[i].offset);
            es_len = pstream->pack[i].len - pstream->pack[i].offset;
            es_type = (pstream->pack[i].data_type.h264_type == 5) ? 1 : 0;
            time_stamp = pstream->pack[i].pts / 1000;

            //printf("packet%d,len=%d,%02x,%02x,%02x,%02x,%02x\n",i,es_len,es_buf[0],es_buf[1],es_buf[2],es_buf[3],es_buf[4]);

            sh.nalu[i].data = (char*)es_buf;
            sh.nalu[i].size = es_len; 
            sh.nalu[i].timestamp = time_stamp;
            //g_stream_fun(chn,stream,es_type,time_stamp,es_buf,es_len,g_stream_fun_usr);
        }

        post_stream_to_observer(&sh,NULL,0);
    }

    venc_h264_cbr::venc_h264_cbr(int w,int h,int src_fr,int venc_fr,ot_venc_chn venc_chn,ot_vpss_grp vpss_grp,ot_vpss_chn vpss_chn,int bitrate)
        :venc_h264(w,h,src_fr,venc_fr,venc_chn,vpss_grp,vpss_chn),m_bitrate(bitrate)
    {
        m_venc_chn_attr.rc_attr.rc_mode = OT_VENC_RC_MODE_H264_CBR;
        m_venc_chn_attr.rc_attr.h264_cbr.gop = m_venc_fr; /*the interval of IFrame*/
        m_venc_chn_attr.rc_attr.h264_cbr.stats_time = 1; /* stream rate statics time(s) */
        m_venc_chn_attr.rc_attr.h264_cbr.src_frame_rate= m_src_fr; /* input (vi) frame rate */
        m_venc_chn_attr.rc_attr.h264_cbr.dst_frame_rate = m_venc_fr; /* target frame rate */
        m_venc_chn_attr.rc_attr.h264_cbr.bit_rate = m_bitrate;
    }

    venc_h264_cbr::~venc_h264_cbr()
    {
    }

    venc_h265::venc_h265(int w,int h,int src_fr,int venc_fr,ot_venc_chn venc_chn,ot_vpss_grp vpss_grp,ot_vpss_chn vpss_chn)
        :venc(w,h,src_fr,venc_fr,venc_chn,vpss_grp,vpss_chn)
    {
        memset(&m_venc_chn_attr,0,sizeof(m_venc_chn_attr));
        m_venc_chn_attr.venc_attr.type = OT_PT_H265;
        m_venc_chn_attr.venc_attr.max_pic_width = m_venc_w;
        m_venc_chn_attr.venc_attr.max_pic_height = m_venc_h;
        m_venc_chn_attr.venc_attr.pic_width = m_venc_w;/*the picture width*/
        m_venc_chn_attr.venc_attr.pic_height    = m_venc_h;/*the picture height*/
        m_venc_chn_attr.venc_attr.buf_size      = m_venc_w * m_venc_h  *3 / 2;/*stream buffer size*/
        m_venc_chn_attr.venc_attr.is_by_frame      = TD_TRUE;/*get stream mode is slice mode or frame mode?*/
        m_venc_chn_attr.venc_attr.profile = 0;
        m_venc_chn_attr.venc_attr.h265_attr.rcn_ref_share_buf_en = TD_FALSE;
        m_venc_chn_attr.venc_attr.h265_attr.frame_buf_ratio = 70;
        m_venc_chn_attr.gop_attr.gop_mode = OT_VENC_GOP_MODE_NORMAL_P;
        m_venc_chn_attr.gop_attr.normal_p.ip_qp_delta = 2; /* 2 is a number */
    }

    venc_h265::~venc_h265()
    {
    }

    void venc_h265::process_video_stream(ot_venc_stream* pstream)
    {
        char* es_buf = NULL;
        int es_len = 0;
        int es_type = 0;
        unsigned long long time_stamp = 0;

        ceanic::util::stream_head sh;

        memset(&sh,0,sizeof(sh));
        sh.type = STREAM_NALU_SLICE;    
        sh.tag = CEANIC_TAG;
        sh.sys_time = time(NULL);  
        sh.nalu_count = pstream->pack_cnt;         
        sh.w = m_venc_w;
        sh.h = m_venc_h;

        for(unsigned int i = 0; i < pstream->pack_cnt; i++)
        {
            es_buf = (char*)(pstream->pack[i].addr + pstream->pack[i].offset);
            es_len = pstream->pack[i].len - pstream->pack[i].offset;
            es_type = (pstream->pack[i].data_type.h264_type == 19) ? 1 : 0;
            time_stamp = pstream->pack[i].pts / 1000;

            //printf("packet%d,len=%d,%02x,%02x,%02x,%02x,%02x\n",i,es_len,es_buf[0],es_buf[1],es_buf[2],es_buf[3],es_buf[4]);

            sh.nalu[i].data = (char*)es_buf;
            sh.nalu[i].size = es_len; 
            sh.nalu[i].timestamp = time_stamp;
            //g_stream_fun(chn,stream,es_type,time_stamp,es_buf,es_len,g_stream_fun_usr);
        }

        post_stream_to_observer(&sh,NULL,0);
    }

    venc_h265_cbr::venc_h265_cbr(int w,int h,int src_fr,int venc_fr,ot_venc_chn venc_chn,ot_vpss_grp vpss_grp,ot_vpss_chn vpss_chn,int bitrate)
        :venc_h265(w,h,src_fr,venc_fr,venc_chn,vpss_grp,vpss_chn),m_bitrate(bitrate)
    {
        m_venc_chn_attr.rc_attr.rc_mode = OT_VENC_RC_MODE_H265_CBR;
        m_venc_chn_attr.rc_attr.h265_cbr.gop = m_venc_fr; /*the interval of IFrame*/
        m_venc_chn_attr.rc_attr.h265_cbr.stats_time = 1; /* stream rate statics time(s) */
        m_venc_chn_attr.rc_attr.h265_cbr.src_frame_rate= m_src_fr; /* input (vi) frame rate */
        m_venc_chn_attr.rc_attr.h265_cbr.dst_frame_rate = m_venc_fr; /* target frame rate */
        m_venc_chn_attr.rc_attr.h265_cbr.bit_rate = m_bitrate;
    }

    venc_h265_cbr::~venc_h265_cbr()
    {
    }

}}//namespace


