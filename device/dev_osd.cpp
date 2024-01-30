#include "dev_osd.h"
#include "dev_log.h"
#include "ceanic_freetype.h"

const char* g_week_stsr[7] = {"星期日","星期一","星期二","星期三","星期四","星期五","星期六"};
ceanic_freetype g_freetype("/opt/ceanic/fonts/Vera.ttf","/opt/ceanic/fonts/gbsn00lp.ttf");

#ifndef ROUND_DOWN
#define ROUND_DOWN(size, align) ((size) & ~((align) - 1))
#endif

#ifndef ROUND_UP
#define ROUND_UP(size, align)   (((size) + ((align) - 1)) & ~((align) - 1))
#endif

int rgb24to1555(int r,int g,int b,int a)
{
    int r1555 = r & 0x1f;
    int g1555 = g & 0x1f; 
    int b1555 = b & 0x1f;

    if(a)
    {
        return  0x8000 | (r1555 << 10) | (g1555 << 5) | b1555;
    }
    else
    {
        return  (r1555 << 10) | (g1555 << 5) | b1555;
    }
}

namespace hisilicon{namespace dev{

        osd::osd(int x,int y,int font_size,ot_rgn_handle rgn_h,ot_venc_chn venc_h)
        :m_x(x),m_y(y),m_font_size(font_size),m_rgn_h(rgn_h),m_venc_h(venc_h),m_is_start(false)
    {
        memset(&m_rgn_attr,0,sizeof(m_rgn_attr));
        m_rgn_attr.type = OT_RGN_OVERLAY; 
        m_rgn_attr.attr.overlay.pixel_format = OT_PIXEL_FORMAT_ARGB_1555;
        m_rgn_attr.attr.overlay.bg_color = rgb24to1555(0,255,0,0);
        m_rgn_attr.attr.overlay.canvas_num = 2;

        memset(&m_rgn_chn_attr,0,sizeof(m_rgn_chn_attr));
        m_rgn_chn_attr.is_show = TD_TRUE;
        m_rgn_chn_attr.type = OT_RGN_OVERLAY;
        m_rgn_chn_attr.attr.overlay_chn.point.x = m_x;
        m_rgn_chn_attr.attr.overlay_chn.point.y = m_y;
        m_rgn_chn_attr.attr.overlay_chn.bg_alpha = 0;
        m_rgn_chn_attr.attr.overlay_chn.fg_alpha = 128;
        m_rgn_chn_attr.attr.overlay_chn.layer = 0; 
        m_rgn_chn_attr.attr.overlay_chn.qp_info.is_abs_qp = TD_FALSE;
        m_rgn_chn_attr.attr.overlay_chn.qp_info.qp_val = 0;
        m_rgn_chn_attr.attr.overlay_chn.qp_info.enable = TD_FALSE;
        m_rgn_chn_attr.attr.overlay_chn.dst = OT_RGN_ATTACH_JPEG_MAIN;
    }

    osd::~osd()
    {
        stop();
    }

    bool osd::init()
    {
        if(!g_freetype.init())
        {
            DEV_WRITE_LOG_ERROR("freetype init failed");
            return false;
        }

        return true;
    }

    void osd::release()
    {
        g_freetype.release();
    }

    bool osd::start()
    {
        if(m_is_start)
        {
            return false;
        }

        td_s32 ret = ss_mpi_rgn_create(m_rgn_h,&m_rgn_attr);
        if(ret != TD_SUCCESS)
        {
            DEV_WRITE_LOG_ERROR("ss_mpi_rgn_create failed with 0x%x",ret);
            return false;
        }

        ot_mpp_chn src_chn;
        src_chn.mod_id = OT_ID_VENC;
        src_chn.dev_id = 0;
        src_chn.chn_id = m_venc_h;

        ret = ss_mpi_rgn_attach_to_chn(m_rgn_h, &src_chn, &m_rgn_chn_attr);
        if(ret != TD_SUCCESS)
        {
            DEV_WRITE_LOG_ERROR("ss_mpi_rgn_attach_to_chn failed with 0x%x",ret);
            return false;
        }

        m_is_start = true;
        return true;
    }

    void osd::stop()
    {
        if(!m_is_start)
        {
            return ;
        }

        ot_mpp_chn src_chn;
        src_chn.mod_id = OT_ID_VENC;
        src_chn.dev_id = 0;
        src_chn.chn_id = m_venc_h;

        ss_mpi_rgn_detach_from_chn(m_rgn_h, &src_chn);
        ss_mpi_rgn_destroy(m_rgn_h); 

        m_is_start = false;
    }

    osd_date::osd_date(int x,int y,int font_size,ot_rgn_handle rgn_h,ot_venc_chn venc_h)
        :osd(x,y,font_size,rgn_h,venc_h)
    {
        m_last_date_str[0] = '\0';
    }

    osd_date::~osd_date()
    {
    }

    bool osd_date::start()
    {
        time_t cur_tm = time(NULL);
        struct tm cur;
        localtime_r(&cur_tm,&cur);

        char data_str[255];
        sprintf(data_str,"%s %04d-%02d-%02d %02d:%02d:%02d",g_week_stsr[cur.tm_wday],cur.tm_year + 1900,cur.tm_mon + 1,cur.tm_mday,cur.tm_hour,cur.tm_min,cur.tm_sec);

        int area_w;
        int area_h;
        g_freetype.get_width(data_str,m_font_size,&area_w);
        area_w = ROUND_UP(area_w + 1,64);
        area_h = ROUND_UP(m_font_size + 1,64);
        printf("area_w=%d,area_h=%d\n",area_w,area_h);

        m_rgn_attr.attr.overlay.size.width = area_w;
        m_rgn_attr.attr.overlay.size.height = area_h;

        if(!osd::start())
        {
            return false;
        }

        m_last_date_str[0] = '\0';
        m_thd = std::thread(std::bind(&osd_date::on_refresh,this));
        return true;
    }

    void osd_date::on_refresh()
    {
        time_t last_tm = 0;
        time_t cur_tm;
        struct tm cur;
        char cur_osd_date_str[255] = {0};
        td_s32 ret; 
        ot_rgn_canvas_info canvas_info;

        while(m_is_start)
        {
            cur_tm = time(NULL); 
            if(cur_tm == last_tm)
            {
                usleep(10000);
                continue;
            }

            last_tm = cur_tm;

            localtime_r(&cur_tm,&cur);
            sprintf(cur_osd_date_str,"%s %04d-%02d-%02d %02d:%02d:%02d",g_week_stsr[cur.tm_wday],cur.tm_year + 1900,cur.tm_mon + 1,cur.tm_mday,cur.tm_hour,cur.tm_min,cur.tm_sec);

            ret = ss_mpi_rgn_get_canvas_info(m_rgn_h, &canvas_info);
            if (ret != TD_SUCCESS)
            {
                DEV_WRITE_LOG_ERROR("ss_mpi_rgn_get_canvas_info failed with error 0x%x", ret);
                return ;
            }

            if(strlen(m_last_date_str) == 0)
            {
                unsigned short* p = (unsigned short*)canvas_info.virt_addr;
                for(unsigned int i = 0; i < canvas_info.size.width * canvas_info.size.height; i++)
                {
                    *p = (short)rgb24to1555(0,255,0,0);
                    p++;
                }

                //printf("chn=%d,stream=%d,area_w=%d,area_h=%d,%d,%d\n",chn,stream,g_osd_date_info[chn][stream].area_w,g_osd_date_info[chn][stream].area_h,stCanvasInfo.stSize.u32Width,stCanvasInfo.stSize.u32Height);
                g_freetype.show_string(
                        cur_osd_date_str,
                        m_rgn_attr.attr.overlay.size.width,
                        m_rgn_attr.attr.overlay.size.height,
                        m_font_size,
                        1,
                        (unsigned char*)canvas_info.virt_addr,
                        canvas_info.size.width * canvas_info.size.height * 2);
            }
            else
            {
                g_freetype.show_string_compare(
                        m_last_date_str,
                        cur_osd_date_str,
                        m_rgn_attr.attr.overlay.size.width,
                        m_rgn_attr.attr.overlay.size.height,
                        m_font_size,
                        1,
                        (unsigned char*)canvas_info.virt_addr, canvas_info.size.width * canvas_info.size.height * 2);
            }

            strcpy(m_last_date_str,cur_osd_date_str);

            ret = ss_mpi_rgn_update_canvas(m_rgn_h);
            if (ret != TD_SUCCESS)
            {
                DEV_WRITE_LOG_ERROR("ss_mpi_rgn_update_canvas failed with error 0x%x", ret);
                break;
            }
        }
    }

    void osd_date::stop()
    {
        osd::stop();

        m_thd.join();
    }

    osd_name::osd_name(int x,int y,int font_size,ot_rgn_handle rgn_h,ot_venc_chn venc_h,const char* name)
        :osd(x,y,font_size,rgn_h,venc_h),m_name(name)
    {
    }

    osd_name::~osd_name()
    {
        stop();
    }

    bool osd_name::start()
    {
        td_s32 ret;
        int area_w;
        int area_h;
        g_freetype.get_width(m_name.c_str(),m_font_size,&area_w);
        area_w = ROUND_UP(area_w + 1,64);
        area_h = ROUND_UP(m_font_size + 1,64);
        printf("area_w=%d,area_h=%d\n",area_w,area_h);

        m_rgn_attr.attr.overlay.size.width = area_w;
        m_rgn_attr.attr.overlay.size.height = area_h;

        if(!osd::start())
        {
            return false;
        }

        ot_rgn_canvas_info canvas_info;
        ret = ss_mpi_rgn_get_canvas_info(m_rgn_h, &canvas_info);
        if (ret != TD_SUCCESS)
        {
            DEV_WRITE_LOG_ERROR("ss_mpi_rgn_get_canvas_info failed with error 0x%x", ret);
            return false;
        }

        g_freetype.show_string(
                m_name.c_str(),
                m_rgn_attr.attr.overlay.size.width,
                m_rgn_attr.attr.overlay.size.height,
                m_font_size,
                1,
                (unsigned char*)canvas_info.virt_addr,
                canvas_info.size.width * canvas_info.size.height * 2);

        ret = ss_mpi_rgn_update_canvas(m_rgn_h);
        if (ret != TD_SUCCESS)
        {
            DEV_WRITE_LOG_ERROR("ss_mpi_rgn_update_canvas failed with error 0x%x", ret);
            return false;
        }

        return true;
    }

    void osd_name::stop()
    {
        osd::stop();
    }

}}//namespace


