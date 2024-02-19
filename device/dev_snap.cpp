#include "dev_sys.h"
#include "dev_snap.h"
#include "dev_log.h"
#include "dev_osd.h"
#include "ceanic_freetype.h"

extern const char* g_week_stsr[7];
extern ceanic_freetype  g_freetype;

namespace hisilicon{namespace dev{

    snap::snap(std::shared_ptr<vi> vi_ptr)
        :m_vi_ptr(vi_ptr)
    {
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

        m_venc_chn = sys::alloc_venc_chn();
    }

    snap::~snap()
    {
        assert(!m_bstart);
    }

    bool snap::start()
    {
        td_s32 ret;

        std::shared_ptr<vi_isp> viisp = std::dynamic_pointer_cast<vi_isp>(m_vi_ptr);
        if(!viisp)
        {
            return false;
        }

        ot_vi_pipe_attr vi_pipe_attr = viisp->vi_pipe_attr();

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

        ot_venc_start_param start_param;
        start_param.recv_pic_num = -1;
        ret = ss_mpi_venc_start_chn(m_venc_chn, &start_param); 
        if(ret != TD_SUCCESS) 
        {
            DEV_WRITE_LOG_ERROR("ss_mpi_venc_start_chn failed with %#x", ret);
            return false;
        }

        m_bstart = true;
        return true;
    }

    void snap::stop()
    {
        //stop venc
        ss_mpi_venc_stop_chn(m_venc_chn);
        ss_mpi_venc_destroy_chn(m_venc_chn);

        m_bstart = false;
    }

    bool snap::trigger(const char* path,int quality,const char* str_info)
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

        ot_venc_jpeg_param jpg_param;
        ret = ss_mpi_venc_get_jpeg_param(m_venc_chn, &jpg_param);
        if(ret != TD_SUCCESS)
        {
            DEV_WRITE_LOG_ERROR("ss_mpi_venc_get_jpg_param failed with %#x", ret);
            return false;
        }

        jpg_param.qfactor = quality;
        ret = ss_mpi_venc_set_jpeg_param(m_venc_chn, &jpg_param);
        if(ret != TD_SUCCESS)
        {
            DEV_WRITE_LOG_ERROR("ss_mpi_venc_set_jpg_param failed with %#x", ret);
            return false;
        }

        ot_video_frame_info frame_info;
        ret = ss_mpi_vpss_get_grp_frame(m_vi_ptr->vpss_grp(),&frame_info,1000);
        if(ret != TD_SUCCESS)
        {
            DEV_WRITE_LOG_ERROR("ss_mpi_vpss_get_frame failed with %#x", ret);
            return false;
        }

        time_t cur_tm = time(NULL);
        struct tm cur;
        localtime_r(&cur_tm,&cur);
        char data_str[255];
        sprintf(data_str,"%s %04d-%02d-%02d %02d:%02d:%02d",g_week_stsr[cur.tm_wday],cur.tm_year + 1900,cur.tm_mon + 1,cur.tm_mday,cur.tm_hour,cur.tm_min,cur.tm_sec);

        std::shared_ptr<osd_name> osd1 = std::make_shared<osd_name>(10,10,64,m_venc_chn,data_str);
        osd1->start();
        std::shared_ptr<osd_name> osd2;
        if(str_info)
        {
            osd2 = std::make_shared<osd_name>(10,96,64,m_venc_chn,str_info);
            osd2->start();
        }

        ret = ss_mpi_venc_send_frame(m_venc_chn,&frame_info,1000);
        ss_mpi_vpss_release_grp_frame(m_vi_ptr->vpss_grp(),&frame_info);
        if(ret != TD_SUCCESS)
        {
            DEV_WRITE_LOG_ERROR("ss_mpi_venc_send_frame failed with %#x", ret);
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
