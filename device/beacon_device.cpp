#include "beacon_std.h"
#include "beacon_device.h"
#include <thread>
#include <mutex>
#include "beacon_device_std.h"
#include <string.h>
#include "beacon_freetype.h"

static const char* g_week_stsr[7] = {"星期日","星期一","星期二","星期三","星期四","星期五","星期六"};
#ifndef ROUND_DOWN
#define ROUND_DOWN(size, align) ((size) & ~((align) - 1))
#endif

#ifndef ROUND_UP
#define ROUND_UP(size, align)   (((size) + ((align) - 1)) & ~((align) - 1))
#endif

LOG_HANDLE g_dev_log = NULL;
static std::timed_mutex g_mu;
#define _LOCK_INTERFACE(name,line,t) std::unique_lock<std::timed_mutex> name##line(g_mu,std::chrono::seconds(t))
#define LOCK_INTERFACE() _LOCK_INTERFACE(_lock,__LINE__,10);

static int g_dev_mode = BEACON_DEV_MODE_0;
static int g_dev_inited = 0;

#define MAX_ONE_FRAME_LEN (1024 * 1024)
static int g_stream_fun_type = CALLBACK_FUN_NALU;
static void (*g_stream_fun)(int chn,int stream,int type,uint64_t time_stamp,const char* buf,int len,int64_t userdata) = NULL;
static int64_t g_stream_fun_usr = 0;

static beacon_freetype g_freetype("/usr/share/fonts/Vera.ttf","/usr/share/fonts/gbsn00lp.ttf");
#define MIPI_DEV_NAME       "/dev/ot_mipi_rx"
venc_info_t g_venc_info[MAX_CHANNEL][MAX_VENC_COUNT];
osd_date_info_t g_osd_date_info[MAX_CHANNEL][MAX_VENC_COUNT];
osd_name_info_t g_osd_name_info[MAX_CHANNEL][MAX_VENC_COUNT];
device_vin_t g_device_vin[MAX_CHANNEL];

#define WIDTH_2688 2688
#define HEIGHT_1520 1520
static combo_dev_attr_t g_mipi_4lane_chn0_sensor_os04a10_12bit_4m_nowdr_attr = {
    .devno = 0,
    .input_mode = INPUT_MODE_MIPI,
    .data_rate  = MIPI_DATA_RATE_X1,
    .img_rect   = {0, 0, WIDTH_2688, HEIGHT_1520},
    .mipi_attr  = {
        DATA_TYPE_RAW_12BIT,
        OT_MIPI_WDR_MODE_NONE,
        {0, 1, 2, 3, -1, -1, -1, -1}
    }
};

static ot_vi_dev_attr g_mipi_raw_dev_attr = {
    .intf_mode = OT_VI_INTF_MODE_MIPI,

    /* Invalid argument */
    .work_mode = OT_VI_WORK_MODE_MULTIPLEX_1,

    /* mask component */
    .component_mask = {0xfff00000, 0x00000000},

    .scan_mode = OT_VI_SCAN_PROGRESSIVE,

    /* Invalid argument */
    .ad_chn_id = {-1, -1, -1, -1},

    /* data seq */
    .data_seq = OT_VI_DATA_SEQ_YVYU,

    /* sync param */
    .sync_cfg = {
        .vsync           = OT_VI_VSYNC_FIELD,
        .vsync_neg       = OT_VI_VSYNC_NEG_HIGH,
        .hsync           = OT_VI_HSYNC_VALID_SIG,
        .hsync_neg       = OT_VI_HSYNC_NEG_HIGH,
        .vsync_valid     = OT_VI_VSYNC_VALID_SIG,
        .vsync_valid_neg = OT_VI_VSYNC_VALID_NEG_HIGH,
        .timing_blank    = {
            /* hsync_hfb      hsync_act     hsync_hhb */
            0,                0,            0,
            /* vsync0_vhb     vsync0_act    vsync0_hhb */
            0,                0,            0,
            /* vsync1_vhb     vsync1_act    vsync1_hhb */
            0,                0,            0
        }
    },

    /* data type */
    .data_type = OT_VI_DATA_TYPE_RAW,

    /* data reverse */
    .data_reverse = TD_FALSE,

    /* input size */
    .in_size = {3840, 2160},

    /* data rate */
    .data_rate = OT_DATA_RATE_X1,
};

static ot_isp_pub_attr g_isp_pub_attr_os04a10_mipi_4m_30fps = {
    {0, 0, WIDTH_2688, HEIGHT_1520},
    {WIDTH_2688, HEIGHT_1520},
    30,
    OT_ISP_BAYER_RGGB,
    OT_WDR_MODE_NONE,
    0,
    TD_FALSE,
    TD_FALSE,
    {
        TD_FALSE,
        {0, 0, 2688, 1520},
    },
};

static ot_isp_pub_attr g_isp_pub_attr_os04a10_mipi_4m_30fps_wdr2to1 = {
    {0, 0, WIDTH_2688, HEIGHT_1520},
    {WIDTH_2688, HEIGHT_1520},
    30,
    OT_ISP_BAYER_RGGB,
    OT_WDR_MODE_2To1_LINE,
    0,
    TD_FALSE,
    TD_FALSE,
    {
        TD_FALSE,
        {0, 0, 2688, 1520},
    },
};

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


static int g_is_stream_start = 0;
void process_video_stream(int chn,int stream,ot_venc_stream* pstream)
{
    char* es_buf = NULL;
    int es_len = 0;
    int es_type = 0;
    unsigned long long time_stamp = 0;

    //printf("packetcount=%d\n",pStream->u32PackCount);

    if(g_stream_fun_type == CALLBACK_FUN_NALU && g_stream_fun)
    {
        for(unsigned int i = 0; i < pstream->pack_cnt; i++)
        {
            es_buf = (char*)(pstream->pack[i].addr + pstream->pack[i].offset);
            es_len = pstream->pack[i].len - pstream->pack[i].offset;
            //es_type = (pstream->pack[i].data_type.h264_type == 5) ? 1 : 0;
            time_stamp = pstream->pack[i].pts / 1000;

            //printf("packet%d,len=%d,%02x,%02x,%02x,%02x,%02x\n",i,es_len,es_buf[0],es_buf[1],es_buf[2],es_buf[3],es_buf[4]);
            g_stream_fun(chn,stream,es_type,time_stamp,es_buf,es_len,g_stream_fun_usr);
        }
    }

    if(g_stream_fun_type == CALLBACK_FUN_FRAME && g_stream_fun)
    {
        static char* _frame = NULL;
        if(_frame == NULL)
        {
            _frame = (char*)malloc(MAX_ONE_FRAME_LEN);
            if(_frame == NULL)
            {
                DEV_WRITE_LOG_ERROR("malloc frame buf failed");
                return;
            }
        }

        int frame_len = 0;
        for(unsigned int i = 0; i < pstream->pack_cnt; i++)
        {
            es_buf = (char*)(pstream->pack[i].addr + pstream->pack[i].offset);
            es_len = pstream->pack[i].len- pstream->pack[i].offset;
            if(g_venc_info[chn][stream].type == BEACON_VIDEO_ENCODE_H264)
            {
                es_type = (pstream->pack[i].data_type.h264_type == 5) ? 1 : 0;
            }
            else if(g_venc_info[chn][stream].type == BEACON_VIDEO_ENCODE_H265)
            {
                es_type = (pstream->pack[i].data_type.h265_type == 19) ? 1 : 0;
            }
            else
            {
                assert(0);
            }

            time_stamp = pstream->pack[i].pts / 1000;

            //printf("packet%d,len=%d,%02x,%02x,%02x,%02x,%02x\n",i,es_len,es_buf[0],es_buf[1],es_buf[2],es_buf[3],es_buf[4]);
            if(frame_len  + es_len > MAX_ONE_FRAME_LEN)
            {
                DEV_WRITE_LOG_ERROR("frame len invalid");
                return;
            }
            memcpy(_frame + frame_len,es_buf,es_len);
            frame_len += es_len;
        }

        g_stream_fun(chn,stream,es_type,time_stamp,_frame,frame_len,g_stream_fun_usr);
    }
}

static pthread_t g_venc_pid = 0;
static void* on_venc_capture(void* param)
{
    fd_set read_fds;
    td_s32 maxfd = 0;
    struct timeval time_val;
    td_s32 venc_fd[MAX_CHANNEL][MAX_VENC_COUNT];
    td_s32 ret;
    ot_venc_stream stream;
    ot_venc_chn_status stat;

    while(g_is_stream_start)
    {
        FD_ZERO(&read_fds);

        for(int i = 0; i < MAX_CHANNEL; i++)
        {
            for(int j = 0; j < MAX_VENC_COUNT; j++)
            {
                if(g_venc_info[i][j].enable)
                {
                    venc_fd[i][j] = ss_mpi_venc_get_fd(g_venc_info[i][j].venc_chn);
                    if(venc_fd[i][j] < 0)
                    {
                        DEV_WRITE_LOG_ERROR("HI_MPI_VENC_GetFd faild with %#x!",venc_fd[i][j]);
                        return NULL;
                    }

                    FD_SET(venc_fd[i][j], &read_fds);
                    if(venc_fd[i][j] > maxfd)
                    {
                        maxfd = venc_fd[i][j];
                    }
                }
                else
                {
                    venc_fd[i][j] = -1;
                }
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

        //video
        for(int i = 0; i < MAX_CHANNEL; i++)
        {
            for(int j = 0; j < MAX_VENC_COUNT; j++)
            {
                if(venc_fd[i][j] > -1 && FD_ISSET(venc_fd[i][j],&read_fds))
                {
                    memset(&stream, 0, sizeof(stream));
                    ret = ss_mpi_venc_query_status(g_venc_info[i][j].venc_chn, &stat);
                    if (TD_SUCCESS != ret)
                    {
                        DEV_WRITE_LOG_ERROR("ss_mpi_venc_query_status failed with %#x", ret);
                        continue;
                    }
                    if(0 == stat.cur_packs)
                    {
                        DEV_WRITE_LOG_ERROR("NOTE: Current  frame is NULL!");
                        continue;
                    }

                    stream.pack = (ot_venc_pack*)malloc(sizeof(ot_venc_pack) * stat.cur_packs);
                    if (NULL == stream.pack)
                    {
                        DEV_WRITE_LOG_ERROR("malloc memory failed!");
                        break;
                    }
                    stream.pack_cnt = stat.cur_packs;
                    ret = ss_mpi_venc_get_stream(g_venc_info[i][j].venc_chn, &stream, TD_TRUE);
                    if (TD_SUCCESS != ret)
                    {
                        free(stream.pack);
                        stream.pack = NULL;

                        DEV_WRITE_LOG_ERROR("ss_mpi_venc_get_stream failed with %#x", ret);
                        break;
                    }

                    process_video_stream(i,j,&stream);

                    ret = ss_mpi_venc_release_stream(g_venc_info[i][j].venc_chn, &stream);
                    if(ret!= TD_SUCCESS)
                    {
                        DEV_WRITE_LOG_ERROR("ss_mpi_venc_release_stream failed with %#x", ret);
                    }

                    free(stream.pack);
                    stream.pack = NULL;
                }
            }
        }
    }

    return NULL;
}


static int get_venc_w_h_by_reso(int reso,int* w,int *h)
    {
        switch(reso)
        {
            case BEACON_RESOLUTION_3840_2160:
                *w = 3840;
                *h = 2160;
                break;
            case BEACON_RESOLUTION_1920_1080:
                *w = 1920;
                *h = 1080;
                break;
            case BEACON_RESOLUTION_1280_720:
                *w = 1280;
                *h = 720;
                break;
            case BEACON_RESOLUTION_704_576:
                *w = 704;
                *h = 576;
                break;
            default:
                DEV_WRITE_LOG_ERROR("unknow resoultion=%d",reso);
                return -1;
        }

        return 0;
    }


extern "C"
{
#define CURRENT_DEV_VERSION "3516(3519)dv500.1.0"
    const char* beacon_device_version()
    {
        return CURRENT_DEV_VERSION;
    }

    int beacon_device_set_log(int enable,int level,const char* file_path,unsigned int max_file_size)
    {
        if(enable)
        {
            if(g_dev_log == NULL)
            {
                g_dev_log = beacon_start_log("beacon_dev",BEACON_LOG_MODE_CONSOLE | BEACON_LOG_MODE_FILE,(BEACON_LOG_LEVEL_E)level,file_path,max_file_size);
            }
            else
            {
                return BEACON_ERR_BUSY;
            }
        }
        else
        {
            if(g_dev_log)
            {
                beacon_stop_log(g_dev_log);
                g_dev_log = NULL;
            }
            else
            {
                return BEACON_ERR_ORDER;
            }
        }

        return BEACON_ERR_SUCCESS;
    }

extern ot_isp_sns_obj g_sns_os04a10_obj;

static pthread_t g_isp_pid[OT_VI_MAX_DEV_NUM] = {0};
/******************************************************************************
* funciton : ISP Run
******************************************************************************/
static void *sample_comm_isp_thread(td_void *param)
{
    td_s32 ret;
    ot_isp_dev isp_dev;
    isp_dev = (ot_isp_dev)(td_ulong)param;
    printf("ISP Dev %d running !\n", isp_dev);
    ret = ss_mpi_isp_run(isp_dev);
    if (ret != TD_SUCCESS) {
        printf("OT_MPI_ISP_Run failed with %#x!\n", ret);
        return NULL;
    }

    return NULL;
}

static int stop_isp(int chn)
{
    ot_isp_sns_obj* sns_obj;
    if(g_device_vin[chn].name == "OS04A10" || g_device_vin[chn].name == "OS04A10_WDR")
    {

        sns_obj  = &g_sns_os04a10_obj;
    }
    else
    {
        printf("not support sensor %s!\n",g_device_vin[chn].name.c_str());
        return -1;
    }

    for(int i = 0; i < g_device_vin[chn].pipe_num; i++)
    {
        if(i > 0)
        {
            //only use one pipe
            break;
        }

        ot_vi_pipe vi_pipe = g_device_vin[chn].pipe[i];
        ot_isp_3a_alg_lib ae_lib;
        ot_isp_3a_alg_lib awb_lib;
        ae_lib.id  = vi_pipe;
        awb_lib.id = vi_pipe;
        strncpy_s(ae_lib.lib_name, sizeof(ae_lib.lib_name), OT_AE_LIB_NAME, sizeof(OT_AE_LIB_NAME));
        strncpy_s(awb_lib.lib_name, sizeof(awb_lib.lib_name), OT_AWB_LIB_NAME, sizeof(OT_AWB_LIB_NAME));

        if(g_isp_pid[vi_pipe])
        {
            ss_mpi_isp_exit(vi_pipe);
            pthread_join(g_isp_pid[vi_pipe], NULL);

            ss_mpi_awb_unregister(vi_pipe, &awb_lib);
            ss_mpi_ae_unregister(vi_pipe, &ae_lib);
            if (sns_obj->pfn_un_register_callback != TD_NULL)
            {
                sns_obj->pfn_un_register_callback(vi_pipe, &ae_lib, &awb_lib);
            }

            g_isp_pid[vi_pipe] = 0;
        }
    }

    return 0;
}

static int start_isp(int chn)
{
    ot_isp_sns_obj* sns_obj;
    if(g_device_vin[chn].name == "OS04A10" || g_device_vin[chn].name == "OS04A10_WDR")
    {

        sns_obj  = &g_sns_os04a10_obj;
    }
    else
    {
        printf("not support sensor %s!\n",g_device_vin[chn].name.c_str());
        return -1;
    }

    for(int i = 0; i < g_device_vin[chn].pipe_num; i++)
    {
        if(i > 0)
        {
            //only use one pipe
            break;
        }

        ot_isp_3a_alg_lib ae_lib;
        ot_isp_3a_alg_lib awb_lib;

        ot_vi_pipe vi_pipe = g_device_vin[chn].pipe[i];
        ae_lib.id  = vi_pipe;
        awb_lib.id = vi_pipe;
        strncpy_s(ae_lib.lib_name, sizeof(ae_lib.lib_name), OT_AE_LIB_NAME, sizeof(OT_AE_LIB_NAME));
        strncpy_s(awb_lib.lib_name, sizeof(awb_lib.lib_name), OT_AWB_LIB_NAME, sizeof(OT_AWB_LIB_NAME));
        if (sns_obj->pfn_register_callback != TD_NULL)
        {
            sns_obj->pfn_register_callback(vi_pipe, &ae_lib, &awb_lib);
        } 

        ot_isp_sns_commbus sns_bus_info;
        sns_bus_info.i2c_dev = 4;
        if (sns_obj->pfn_set_bus_info != TD_NULL)
        {
            sns_obj->pfn_set_bus_info(vi_pipe, sns_bus_info);
        }

        ss_mpi_ae_register(vi_pipe, &ae_lib);
        ss_mpi_awb_register(vi_pipe, &awb_lib);
        ss_mpi_isp_mem_init(vi_pipe);
        if(g_device_vin[chn].name == "OS04A10")
        {
            ss_mpi_isp_set_pub_attr(vi_pipe, &g_isp_pub_attr_os04a10_mipi_4m_30fps);
        }
        else if(g_device_vin[chn].name == "OS04A10_WDR")
        {
            ss_mpi_isp_set_pub_attr(vi_pipe, &g_isp_pub_attr_os04a10_mipi_4m_30fps_wdr2to1);
        }
        ss_mpi_isp_init(vi_pipe);

        pthread_create(&g_isp_pid[vi_pipe], NULL, sample_comm_isp_thread, (td_void*)(td_ulong)vi_pipe);
    }

    return 0;
}

static int check_chn_valid(int chn)
{
    if(chn < 0 || chn >= MAX_CHANNEL)
    {
        return -1;
    }

    return 0;
}

static int check_stream_valid(int stream)
{
    if(stream < 0 || stream >= MAX_VENC_COUNT)
    {
        return -1;
    }

    return 0;
}

static int check_chn_stream_valid(int chn,int stream)
{
    if(check_chn_valid(chn) != 0)
    {
        return -1;
    }

    if(check_stream_valid(stream) != 0)
    {
        return -1;
    }

    return 0;
}


//it depends on hardware
static int get_sns_clk_src_from_chn(int chn)
{
    int sns_clk_src = -1;
    switch(chn)
    {
        case 0:
            sns_clk_src = 0;
            break;
        case 1:
            sns_clk_src = 1;
            break;
        case 2:
            sns_clk_src = 2;
            break;
        case 3:
            sns_clk_src = 3;
            break;
         default:
            sns_clk_src = 0;
            break;
    }

    return sns_clk_src;
}

static int get_mipi_devno_from_chn(int chn)
{
    int devno = -1;
    switch(chn)
    {
        case 0:
            devno = 0;
            break;
        case 1:
            devno = 1; 
            break;
        case 2:
            devno = 2;
            break;
        case 3:
            devno = 3;
            break;
         default:
            devno = 0;
            break;
    }

    return devno;
}

static int get_vi_devno_from_chn(int chn)
{
    int devno = -1;
    switch(chn)
    {
        case 0:
            devno = 0;
            break;
        case 1:
            devno = 1; 
            break;
        case 2:
            devno = 2;
            break;
        case 3:
            devno = 3;
            break;
         default:
            devno = 0;
            break;
    }

    return devno;
}

static int get_vpss_grp_from_chn(int chn)
{
    int vpss_grp = 0;

    if(g_dev_mode == BEACON_DEV_MODE_0)
    {
        vpss_grp = 0;
    }
    else
    {
        vpss_grp = (chn % 2) == 0 ? 0 : 2;
    }

    return vpss_grp;
}

static int init_mipi()
{
    td_s32 ret = TD_FAILURE;
    td_s32 fd;
    fd = open(MIPI_DEV_NAME, O_RDWR);
    if (fd < 0)
    {
        DEV_WRITE_LOG_ERROR("open ot_mipi_rx dev failed");
        return -1;
    }

    lane_divide_mode_t hs_mode = LANE_DIVIDE_MODE_0;
    ret = ioctl(fd, OT_MIPI_SET_HS_MODE, &hs_mode);
    if (TD_SUCCESS != ret)
    {
        DEV_WRITE_LOG_ERROR("OT_MIPI_SET_HS_MODE failed");
    }

    close(fd);
    return 0;
}

static int vi_reset_sns(int chn)
{
    td_s32 fd;
    fd = open(MIPI_DEV_NAME, O_RDWR);
    if (fd < 0)
    {
        DEV_WRITE_LOG_ERROR("vi chn=%d has been inited",chn);
        return -1;
    }

    sns_clk_source_t sns_clk_src = g_device_vin[chn].sns_clk_src;
    ioctl(fd, OT_MIPI_ENABLE_SENSOR_CLOCK, &sns_clk_src);
    ioctl(fd, OT_MIPI_RESET_SENSOR, &sns_clk_src);
    ioctl(fd, OT_MIPI_UNRESET_SENSOR, &sns_clk_src);

    close(fd);
    return 0;
}

static int vi_start_mipi(int chn)
{
    combo_dev_t mipi_dev = g_device_vin[chn].mipi_dev;

    td_s32 fd = open(MIPI_DEV_NAME, O_RDWR);
    if (fd < 0)
    {
        printf("open hi_mipi dev failed\n");
        return -1;
    }

    ioctl(fd, OT_MIPI_ENABLE_MIPI_CLOCK, &mipi_dev);
    ioctl(fd, OT_MIPI_RESET_MIPI, &mipi_dev);
    ioctl(fd, OT_MIPI_SET_DEV_ATTR, &g_mipi_4lane_chn0_sensor_os04a10_12bit_4m_nowdr_attr);
    ioctl(fd, OT_MIPI_UNRESET_MIPI, &mipi_dev);

    close(fd);
    return 0;
}


static int vi_stop_mipi(int chn)
{
    combo_dev_t  mipi_dev = g_device_vin[chn].mipi_dev;

    td_s32 fd = open(MIPI_DEV_NAME, O_RDWR);
    if (fd < 0)
    {
        printf("open hi_mipi dev failed\n");
        return -1;
    }

    ioctl(fd, OT_MIPI_RESET_MIPI, &mipi_dev);
    ioctl(fd, OT_MIPI_DISABLE_MIPI_CLOCK, &mipi_dev);
    
    close(fd);
    return 0;
}

    int beacon_device_init(int mode)
    {
        LOCK_INTERFACE();

        if(g_dev_inited)
        {
            return BEACON_ERR_ORDER;
        }
 
        g_dev_mode = mode;

        if(!g_freetype.init())
        {
            DEV_WRITE_LOG_ERROR("freetype init failed");
            return BEACON_ERR_INTERNAL;
        }

        ss_mpi_sys_exit();
        ss_mpi_vb_exit();

        ot_vb_cfg vb_cfg;
        td_s32 ret = TD_FAILURE;

        memset(&vb_cfg,0,sizeof(ot_vb_cfg));
        vb_cfg.common_pool[0].blk_size = 3840 * 2160 * 2;
        vb_cfg.common_pool[0].blk_cnt = 20;
        vb_cfg.common_pool[1].blk_size = 704 * 576 * 3 / 2;
        vb_cfg.common_pool[1].blk_cnt = 10;
        vb_cfg.max_pool_cnt= 2;

        ret = ss_mpi_vb_set_cfg(&vb_cfg);
        if (TD_SUCCESS != ret)
        {
            DEV_WRITE_LOG_ERROR("ss_mpi_vb_set_cfg failed!");
            return BEACON_ERR_INTERNAL;
        }

        ot_vb_supplement_cfg vb_supplement_cfg = {0};
        vb_supplement_cfg.supplement_cfg = OT_VB_SUPPLEMENT_BNR_MOT_MASK;
        ret = ss_mpi_vb_set_supplement_cfg(&vb_supplement_cfg);
        if (TD_SUCCESS != ret)
        {
            DEV_WRITE_LOG_ERROR("ss_mpi_vb_set_supplement failed!");
            return BEACON_ERR_INTERNAL;
        }

        ret = ss_mpi_vb_init();
        if (TD_SUCCESS != ret)
        {
            DEV_WRITE_LOG_ERROR("ss_mpi_vb_init failed!");
            return BEACON_ERR_INTERNAL;
        }

        ret = ss_mpi_sys_init();
        if (TD_SUCCESS != ret)
        {
            DEV_WRITE_LOG_ERROR("ss_mpi_sys_init failed!");
            ss_mpi_vb_init();
            return BEACON_ERR_INTERNAL;
        }

        ot_vi_vpss_mode vi_vpss_mode;
        memset(&vi_vpss_mode,0,sizeof(vi_vpss_mode));
        for(int i = 0; i < OT_VI_MAX_PIPE_NUM; i++)
        {
            vi_vpss_mode.mode[i] = OT_VI_OFFLINE_VPSS_OFFLINE;
        }
        ret = ss_mpi_sys_set_vi_vpss_mode(&vi_vpss_mode);
        if(TD_SUCCESS != ret)
        {
            DEV_WRITE_LOG_ERROR("ss_mpi_sys_set_vi_vpss_mode failed!");
            return BEACON_ERR_INTERNAL;
        }

        ret = ss_mpi_sys_set_vi_aiisp_mode(0, OT_VI_AIISP_MODE_DEFAULT);
        if(TD_SUCCESS != ret)
        {
            DEV_WRITE_LOG_ERROR("ss_mpi_sys_set_vi_aiisp_mode failed!");
            return BEACON_ERR_INTERNAL;
        }

        //start mipi
        init_mipi();

        for(int i = 0; i < MAX_CHANNEL; i++)
        {
            for(int j = 0; j < MAX_VENC_COUNT; j++)
            {
                g_venc_info[i][j].enable = 0;
                g_venc_info[i][j].type = BEACON_VIDEO_ENCODE_H265;
                g_venc_info[i][j].reso = (j == 0) ? BEACON_RESOLUTION_3840_2160: BEACON_RESOLUTION_704_576;
                g_venc_info[i][j].fr = 30;
                g_venc_info[i][j].h265_cfg.bitrate_control = 1;
                g_venc_info[i][j].h265_cfg.bitrate_avg = (j == 0) ? 1024 : 512;
                g_venc_info[i][j].h265_cfg.gop_interval = 25;
                g_venc_info[i][j].h265_cfg.bitrate_max = g_venc_info[i][j].h265_cfg.bitrate_avg;
                g_venc_info[i][j].h265_cfg.bitrate_min = g_venc_info[i][j].h265_cfg.bitrate_max / 2;

                g_venc_info[i][j].venc_chn = i * MAX_VENC_COUNT + j;
            }
        }

        for(int i = 0; i < MAX_CHANNEL; i++)
        {
            for(int j = 0; j < MAX_VENC_COUNT; j++)
            {
                g_osd_date_info[i][j].enable = 0;
                g_osd_date_info[i][j].x = 0;
                g_osd_date_info[i][j].y = 0;
                g_osd_date_info[i][j].color = rgb24to1555(255,255,255,0);
                g_osd_date_info[i][j].area_w = 0;
                g_osd_date_info[i][j].area_h = 0;
                g_osd_date_info[i][j].rgn_h = 0xFFFFFFFF;
                g_osd_date_info[i][j].font_size = 32;

                memset(g_osd_date_info[i][j].last_date_str,0,sizeof(g_osd_date_info[i][j].last_date_str));
            }
        }

        for(int i = 0; i < MAX_CHANNEL; i++)
        {
            for(int j = 0; j < MAX_VENC_COUNT; j++)
            {
                g_osd_name_info[i][j].enable = 0;
                g_osd_name_info[i][j].rgn_h = 0xFFFFFFFF;
                g_osd_name_info[i][j].name[0] = '\0';
            }
        }

        for(int i = 0; i < MAX_CHANNEL; i++)
        {
            g_device_vin[i].inited = 0;
            g_device_vin[i].mipi_dev = get_mipi_devno_from_chn(i);
            g_device_vin[i].sns_clk_src = get_sns_clk_src_from_chn(i);
            g_device_vin[i].vi_dev = get_vi_devno_from_chn(i);
            g_device_vin[i].pipe_num = 1;
            g_device_vin[i].pipe[0] = 0; 
            g_device_vin[i].vpss_grp = get_vpss_grp_from_chn(i);

            for(auto j = 0; j < MAX_VI_SNAP; j++)
            {
                g_device_vin[i].snap_venc[j] = MAX_CHANNEL * MAX_VENC_COUNT + i * MAX_VI_SNAP + j;
                g_device_vin[i].rgn_osd_date_h[j] = 0xFFFFFFFF;
                g_device_vin[i].rgn_osd_name_h[j] = 0xFFFFFFFF;
            }
        }

        g_dev_inited = 1;

        DEV_WRITE_LOG_DEBUG("success");
        return BEACON_ERR_SUCCESS;
    }

    int beacon_device_release()
    {
        LOCK_INTERFACE();

        if(!g_dev_inited)
        {
            DEV_WRITE_LOG_ERROR("device need be inited first");
            return BEACON_ERR_ORDER;
        }

        ss_mpi_sys_exit();
        ss_mpi_vb_exit_mod_common_pool(OT_VB_UID_VDEC);
        ss_mpi_vb_exit();

        g_freetype.release();

        g_dev_inited = 0;

        DEV_WRITE_LOG_DEBUG("success");
        return BEACON_ERR_SUCCESS;
    }

    int beacon_vi_init(int chn,beacon_vin_t* arg)
    {
        LOCK_INTERFACE();

        if(!g_dev_inited)
        {
            DEV_WRITE_LOG_ERROR("device need be inited first");
            return BEACON_ERR_ORDER;
        }

        if(check_chn_valid(chn) != 0)
        {
            DEV_WRITE_LOG_ERROR("chn=%d is invalid",chn);
            return BEACON_ERR_INVALID_PARAM;
        }

        if(arg == NULL || arg->w < 1024 || arg->w > 10000
                || arg->h < 768 || arg->h > 10000
                || arg->fr < 0 || arg->fr > 512)
        {
            DEV_WRITE_LOG_ERROR("param arg is invalid");
            return BEACON_ERR_INVALID_PARAM;
        }

        if(g_device_vin[chn].inited)
        {
            DEV_WRITE_LOG_ERROR("vi chn=%d has been inited",chn);
            return BEACON_ERR_ORDER;
        }

        g_device_vin[chn].fr = arg->fr;
        g_device_vin[chn].w = arg->w;
        g_device_vin[chn].h = arg->h;
        g_device_vin[chn].name = arg->name;

        if(g_device_vin[chn].name == "OS04A10")
        {
            g_device_vin[chn].pipe_num = 1;
            g_device_vin[chn].pipe[0] = 0;
        }else if(g_device_vin[chn].name == "OS04A10_WDR")
        {
            g_device_vin[chn].pipe_num = 2;
            g_device_vin[chn].pipe[0] = 0;
            g_device_vin[chn].pipe[1] = 1;
        }

        td_s32 ret = TD_FAILURE;
        
        //enable mipi&&rset sns
        vi_start_mipi(chn);
        vi_reset_sns(chn);

        //enable dev
        ot_vi_dev vi_dev = g_device_vin[chn].vi_dev;
        ot_vi_dev_attr dev_attr;
        memcpy(&dev_attr,&g_mipi_raw_dev_attr, sizeof(ot_vi_dev_attr));
        dev_attr.in_size.width = g_device_vin[chn].w;
        dev_attr.in_size.height = g_device_vin[chn].h;
        dev_attr.component_mask[0] = 0xfff00000;
        ret = ss_mpi_vi_set_dev_attr(vi_dev, &dev_attr);
        if (ret != TD_SUCCESS)
        {
            DEV_WRITE_LOG_ERROR("ss_mpi_vi_set_dev_attr failed with %#x!", ret);
            return BEACON_ERR_INTERNAL;
        }

        ret = ss_mpi_vi_enable_dev(vi_dev);
        if (ret != TD_SUCCESS)
        {
            DEV_WRITE_LOG_ERROR("ss_mpi_vi_enable_dev failed with %#x!", ret);
            return BEACON_ERR_INTERNAL;
        }

        //bind pipe dev
        for(int i = 0; i < g_device_vin[chn].pipe_num; i++)
        {
            ret = ss_mpi_vi_bind(vi_dev,g_device_vin[chn].pipe[i]);
            if(ret != TD_SUCCESS)
            {
                DEV_WRITE_LOG_ERROR("ss_mpi_vi_bind failed with %#x!", ret);
                return BEACON_ERR_INTERNAL;
            }
        }

        //set fusion grp
        ot_vi_grp fusion_grp = 0;
        ot_vi_wdr_fusion_grp_attr fusion_grp_attr;
        memset(&fusion_grp_attr,0,sizeof(fusion_grp_attr));
        fusion_grp_attr.wdr_mode = OT_WDR_MODE_NONE;
        if(g_device_vin[chn].name == "OS04A10_WDR")
        {
            fusion_grp_attr.wdr_mode = OT_WDR_MODE_2To1_LINE;
        }
        fusion_grp_attr.cache_line = g_device_vin[chn].h;
        for(int i = 0; i < g_device_vin[chn].pipe_num; i++)
        {
            fusion_grp_attr.pipe_id[i] = g_device_vin[chn].pipe[i];
        }
        ret = ss_mpi_vi_set_wdr_fusion_grp_attr(fusion_grp, &fusion_grp_attr);
        if(ret != TD_SUCCESS)
        {
            DEV_WRITE_LOG_ERROR("ss_mpi_vi_set_wdr_fusion_grp_attr failed with %#x!", ret);
            return BEACON_ERR_INTERNAL;
        }

        //start vi pipe
        ot_vi_pipe_attr pipe_attr;
        memset(&pipe_attr,0,sizeof(pipe_attr));
        pipe_attr.pipe_bypass_mode = OT_VI_PIPE_BYPASS_NONE;
        pipe_attr.isp_bypass = TD_FALSE;
        pipe_attr.size.width = g_device_vin[chn].w;
        pipe_attr.size.height = g_device_vin[chn].h;
        pipe_attr.pixel_format = OT_PIXEL_FORMAT_RGB_BAYER_12BPP;
        pipe_attr.compress_mode = OT_COMPRESS_MODE_NONE;
        pipe_attr.frame_rate_ctrl.src_frame_rate = -1;
        pipe_attr.frame_rate_ctrl.dst_frame_rate = -1;

        ot_vi_chn vi_chn = 0;
        ot_vi_chn_attr vi_chn_attr;
        memset(&vi_chn_attr,0,sizeof(vi_chn_attr));
        vi_chn_attr.size.width = g_device_vin[chn].w;
        vi_chn_attr.size.height = g_device_vin[chn].h;
        vi_chn_attr.pixel_format = OT_PIXEL_FORMAT_YVU_SEMIPLANAR_420;
        vi_chn_attr.dynamic_range = OT_DYNAMIC_RANGE_SDR8;
        vi_chn_attr.video_format = OT_VIDEO_FORMAT_LINEAR;
        vi_chn_attr.compress_mode = OT_COMPRESS_MODE_NONE;
        vi_chn_attr.mirror_en = TD_FALSE;
        vi_chn_attr.flip_en = TD_FALSE;
        vi_chn_attr.depth = 0;
        vi_chn_attr.frame_rate_ctrl.src_frame_rate = -1;
        vi_chn_attr.frame_rate_ctrl.dst_frame_rate = -1;

        for(int i = 0; i < g_device_vin[chn].pipe_num; i++)
        {
            ret = ss_mpi_vi_create_pipe(g_device_vin[chn].pipe[i],&pipe_attr);
            if(ret != TD_SUCCESS)
            {
                DEV_WRITE_LOG_ERROR("ss_mpi_vi_create_pipe failed with %#x!", ret);
                return BEACON_ERR_INTERNAL;
            }

            ret = ss_mpi_vi_start_pipe(g_device_vin[chn].pipe[i]);
            if(ret != TD_SUCCESS)
            {
                DEV_WRITE_LOG_ERROR("ss_mpi_vi_start_pipe failed with %#x!", ret);
                return BEACON_ERR_INTERNAL;
            }

            ret = ss_mpi_vi_set_chn_attr(g_device_vin[chn].pipe[i],vi_chn,&vi_chn_attr);
            if(ret != TD_SUCCESS)
            {
                DEV_WRITE_LOG_ERROR("ss_mpi_vi_set_chn_attr failed with %#x!", ret);
                return BEACON_ERR_INTERNAL;
            }

            ret = ss_mpi_vi_enable_chn(g_device_vin[chn].pipe[i], vi_chn);
            if(ret != TD_SUCCESS)
            {
                DEV_WRITE_LOG_ERROR("ss_mpi_vi_enable_chn failed with %#x!", ret);
                return BEACON_ERR_INTERNAL;
            }
        }
        
        start_isp(chn);

        ot_vpss_grp vpss_grp = g_device_vin[chn].vpss_grp;
        ot_vpss_chn vpss_chn = 0;

        //vi->vpss
        ot_mpp_chn src_chn;
        ot_mpp_chn dest_chn;
        src_chn.mod_id = OT_ID_VI;
        src_chn.dev_id = g_device_vin[chn].pipe[0];
        src_chn.chn_id = vi_chn;
        dest_chn.mod_id = OT_ID_VPSS;
        dest_chn.dev_id = vpss_grp;
        dest_chn.chn_id = vpss_chn;
        ret = ss_mpi_sys_bind(&src_chn, &dest_chn);
        if(ret != TD_SUCCESS)
        {
            DEV_WRITE_LOG_ERROR("ss_mpi_sys_bind failed with %#x", ret);
            return BEACON_ERR_INTERNAL;
        }

        //start vpss
        ot_vpss_grp_attr grp_attr;
        grp_attr.ie_en                     = TD_FALSE;
        grp_attr.dci_en                    = TD_FALSE;
        grp_attr.buf_share_en              = TD_FALSE;
        grp_attr.mcf_en                    = TD_FALSE;
        grp_attr.max_width                 = g_device_vin[chn].w;
        grp_attr.max_height                = g_device_vin[chn].h;
        grp_attr.max_dei_width             = 0;
        grp_attr.max_dei_height            = 0;
        grp_attr.dynamic_range             = OT_DYNAMIC_RANGE_SDR8;
        grp_attr.pixel_format              = OT_PIXEL_FORMAT_YVU_SEMIPLANAR_420;
        grp_attr.dei_mode                  = OT_VPSS_DEI_MODE_OFF;
        grp_attr.buf_share_chn             = OT_VPSS_CHN0;
        grp_attr.frame_rate.src_frame_rate = -1;
        grp_attr.frame_rate.dst_frame_rate = -1;
        ret = ss_mpi_vpss_create_grp(vpss_grp,&grp_attr);
        if (ret != TD_SUCCESS)
        {
            DEV_WRITE_LOG_ERROR("ss_mpi_vpss_create_grp failed with %#x", ret);
            return BEACON_ERR_INTERNAL;
        }
        ret = ss_mpi_vpss_start_grp(vpss_grp);
        if (ret != TD_SUCCESS)
        {
            DEV_WRITE_LOG_ERROR("ss_mpi_vpss_start_grp failed with %#x", ret);
            return BEACON_ERR_INTERNAL;
        }

        ot_vpss_chn_attr chn_attr;
        chn_attr.mirror_en                 = TD_FALSE;
        chn_attr.flip_en                   = TD_FALSE;
        chn_attr.border_en                 = TD_FALSE;
        chn_attr.width                     = g_device_vin[chn].w;
        chn_attr.height                    = g_device_vin[chn].h;
        chn_attr.depth                     = 0;
        chn_attr.chn_mode                  = OT_VPSS_CHN_MODE_USER;
        chn_attr.video_format              = OT_VIDEO_FORMAT_LINEAR;
        chn_attr.dynamic_range             = OT_DYNAMIC_RANGE_SDR8;
        chn_attr.pixel_format              = OT_PIXEL_FORMAT_YVU_SEMIPLANAR_420;
        chn_attr.compress_mode             = OT_COMPRESS_MODE_SEG;
        chn_attr.aspect_ratio.mode         = OT_ASPECT_RATIO_NONE;
        chn_attr.frame_rate.src_frame_rate = -1;
        chn_attr.frame_rate.dst_frame_rate = -1;
        ret = ss_mpi_vpss_set_chn_attr(vpss_grp, vpss_chn, &chn_attr);
        if (ret != TD_SUCCESS) 
        {
            DEV_WRITE_LOG_ERROR("ss_mpi_vpss_set_chn_attr failed with %#x", ret);
            return BEACON_ERR_INTERNAL;
        }
        ret = ss_mpi_vpss_enable_chn(vpss_grp, vpss_chn);
        if (ret != TD_SUCCESS) 
        {
            DEV_WRITE_LOG_ERROR("ss_mpi_vpss_enable_chn_attr failed with %#x", ret);
            return BEACON_ERR_INTERNAL;
        }

        g_device_vin[chn].inited = 1;

        DEV_WRITE_LOG_DEBUG("success");
        return BEACON_ERR_SUCCESS;
    }

    int beacon_vi_release(int chn)
    {
        LOCK_INTERFACE();

        if(!g_dev_inited)
        {
            DEV_WRITE_LOG_ERROR("device need be inited first");
            return BEACON_ERR_ORDER;
        }

        if(check_chn_valid(chn) != 0)
        {
            DEV_WRITE_LOG_ERROR("chn=%d is invalid",chn);
            return BEACON_ERR_INVALID_PARAM;
        }

        if(!g_device_vin[chn].inited)
        {
            DEV_WRITE_LOG_ERROR("vi need ve inited first");
            return BEACON_ERR_ORDER;
        }

        stop_isp(chn);
        vi_stop_mipi(chn);
        
        ot_vi_dev vi_dev = g_device_vin[chn].vi_dev;
        ot_vi_chn  vi_chn = 0;
        ot_vpss_grp vpss_grp = g_device_vin[chn].vpss_grp;
        ot_vpss_chn vpss_chn = 0;

        ot_mpp_chn src_chn;
        ot_mpp_chn dest_chn;
        src_chn.mod_id = OT_ID_VI;
        src_chn.dev_id = g_device_vin[chn].pipe[0];
        src_chn.chn_id = vi_chn;
        dest_chn.mod_id = OT_ID_VPSS;
        dest_chn.dev_id = vpss_grp;
        dest_chn.chn_id = vpss_chn;
        ss_mpi_sys_unbind(&src_chn, &dest_chn);
        
        ss_mpi_vpss_disable_chn(vpss_grp, vpss_chn);
        ss_mpi_vpss_stop_grp(vpss_grp);
        ss_mpi_vpss_destroy_grp(vpss_grp);
        
        for(int i = 0; i < g_device_vin[chn].pipe_num; i++)
        {
            ot_vi_pipe vi_pipe = g_device_vin[chn].pipe[i];

            ss_mpi_vi_disable_chn(vi_pipe, vi_chn);
            ss_mpi_vi_stop_pipe(vi_pipe);
            ss_mpi_vi_destroy_pipe(vi_pipe);
            ss_mpi_vi_unbind(vi_dev,vi_pipe);
        }

        ss_mpi_vi_disable_dev(vi_dev);
        DEV_WRITE_LOG_DEBUG("success");

        g_device_vin[chn].inited = 0;
        return BEACON_ERR_SUCCESS;
    }

static int update_encode2device(int chn,int stream)
{
    td_s32 ret; 
    ot_venc_chn_attr chn_attr;
    ot_venc_chn venc_chn = g_venc_info[chn][stream].venc_chn;
    ret = ss_mpi_venc_get_chn_attr(venc_chn, &chn_attr);
    if(ret != TD_SUCCESS)
    {
        DEV_WRITE_LOG_ERROR("venc get chn attr failed,ret=%x!",ret);
        return -1;
    }

    if(g_venc_info[chn][stream].type == BEACON_VIDEO_ENCODE_H265)
    {
        if(g_venc_info[chn][stream].h265_cfg.bitrate_control == 0)
        {
            chn_attr.rc_attr.rc_mode = OT_VENC_RC_MODE_H265_CBR;
            chn_attr.rc_attr.h265_cbr.gop = g_venc_info[chn][stream].h265_cfg.gop_interval;
            chn_attr.rc_attr.h265_cbr.dst_frame_rate= g_venc_info[chn][stream].fr;
            chn_attr.rc_attr.h265_cbr.bit_rate = g_venc_info[chn][stream].h265_cfg.bitrate_avg;
            chn_attr.rc_attr.h265_cbr.stats_time= g_venc_info[chn][stream].h265_cfg.gop_interval / g_venc_info[chn][stream].fr;
            chn_attr.rc_attr.h265_cbr.src_frame_rate = g_device_vin[chn].fr; 

        }
        else if(g_venc_info[chn][stream].h265_cfg.bitrate_control == 1)
        {
            chn_attr.rc_attr.rc_mode = OT_VENC_RC_MODE_H265_VBR;
            chn_attr.rc_attr.h265_vbr.gop = g_venc_info[chn][stream].h265_cfg.gop_interval;
            chn_attr.rc_attr.h265_vbr.dst_frame_rate  = g_venc_info[chn][stream].fr;
            chn_attr.rc_attr.h265_vbr.max_bit_rate = g_venc_info[chn][stream].h265_cfg.bitrate_max;
            chn_attr.rc_attr.h265_vbr.stats_time = g_venc_info[chn][stream].h265_cfg.gop_interval / g_venc_info[chn][stream].fr;
            chn_attr.rc_attr.h265_vbr.src_frame_rate = g_device_vin[chn].fr;
        }
        else if(g_venc_info[chn][stream].h265_cfg.bitrate_control == 2)
        {
            chn_attr.rc_attr.rc_mode = OT_VENC_RC_MODE_H265_AVBR;
            chn_attr.rc_attr.h265_avbr.gop = g_venc_info[chn][stream].h265_cfg.gop_interval;
            chn_attr.rc_attr.h265_avbr.dst_frame_rate = g_venc_info[chn][stream].fr;
            chn_attr.rc_attr.h265_avbr.max_bit_rate = g_venc_info[chn][stream].h265_cfg.bitrate_max;
            chn_attr.rc_attr.h265_avbr.stats_time = g_venc_info[chn][stream].h265_cfg.gop_interval / g_venc_info[chn][stream].fr;
            chn_attr.rc_attr.h265_avbr.src_frame_rate = g_device_vin[chn].fr;
        }
        else
        {
            DEV_WRITE_LOG_ERROR("bitreate_control=%d not supported!",g_venc_info[chn][stream].h265_cfg.bitrate_control);
        }
    }
    else if(g_venc_info[chn][stream].type == BEACON_VIDEO_ENCODE_H264)
    {
        if(g_venc_info[chn][stream].h264_cfg.bitrate_control == 0)
        {
            chn_attr.rc_attr.rc_mode= OT_VENC_RC_MODE_H264_CBR;
            chn_attr.rc_attr.h264_cbr.gop= g_venc_info[chn][stream].h264_cfg.gop_interval;
            chn_attr.rc_attr.h264_cbr.dst_frame_rate = g_venc_info[chn][stream].fr;
            chn_attr.rc_attr.h264_cbr.bit_rate = g_venc_info[chn][stream].h264_cfg.bitrate_avg;
            chn_attr.rc_attr.h264_cbr.stats_time = g_venc_info[chn][stream].h264_cfg.gop_interval / g_venc_info[chn][stream].fr;
            chn_attr.rc_attr.h264_cbr.src_frame_rate = g_device_vin[chn].fr; 

        }
        else if(g_venc_info[chn][stream].h264_cfg.bitrate_control == 1)
        {
            chn_attr.rc_attr.rc_mode = OT_VENC_RC_MODE_H264_VBR;
            chn_attr.rc_attr.h264_vbr.gop = g_venc_info[chn][stream].h264_cfg.gop_interval;
            chn_attr.rc_attr.h264_vbr.dst_frame_rate = g_venc_info[chn][stream].fr;
            chn_attr.rc_attr.h264_vbr.max_bit_rate = g_venc_info[chn][stream].h264_cfg.bitrate_max;
            chn_attr.rc_attr.h264_vbr.stats_time = g_venc_info[chn][stream].h264_cfg.gop_interval / g_venc_info[chn][stream].fr;
            chn_attr.rc_attr.h264_vbr.src_frame_rate = g_device_vin[chn].fr;
        }
        else if(g_venc_info[chn][stream].h264_cfg.bitrate_control == 2)
        {
            chn_attr.rc_attr.rc_mode = OT_VENC_RC_MODE_H264_AVBR;
            chn_attr.rc_attr.h264_avbr.gop = g_venc_info[chn][stream].h264_cfg.gop_interval;
            chn_attr.rc_attr.h264_avbr.dst_frame_rate = g_venc_info[chn][stream].fr;
            chn_attr.rc_attr.h264_avbr.max_bit_rate = g_venc_info[chn][stream].h264_cfg.bitrate_max;
            chn_attr.rc_attr.h264_avbr.stats_time = g_venc_info[chn][stream].h264_cfg.gop_interval / g_venc_info[chn][stream].fr;
            chn_attr.rc_attr.h264_avbr.src_frame_rate = g_device_vin[chn].fr;
        }
        else
        {
            DEV_WRITE_LOG_ERROR("bitreate_control=%d not supported!",g_venc_info[chn][stream].h264_cfg.bitrate_control);
        }
    }

    ret = ss_mpi_venc_set_chn_attr(venc_chn,&chn_attr);
    if(ret != TD_SUCCESS)
    {
        DEV_WRITE_LOG_ERROR("venc set chn attr failed,ret=%x!\n",ret);
        return -1;
    }

    return 0;
}

int beacon_encode_video_set_arg(int chn,int stream,int reso,int fr,int type,void* arg)
    {
        LOCK_INTERFACE();

        if(!g_dev_inited)
        {
            DEV_WRITE_LOG_ERROR("device need be inited first");
            return BEACON_ERR_ORDER;
        }

        if(check_chn_stream_valid(chn,stream) != 0)
        {
            DEV_WRITE_LOG_ERROR("chn=%d,stream=%d is invalid",chn,stream);
            return BEACON_ERR_INVALID_PARAM;
        }

        if(!g_device_vin[chn].inited)
        {
            DEV_WRITE_LOG_ERROR("vi chn=%d need be inited first",chn);
            return BEACON_ERR_ORDER;
        }

        if(g_is_stream_start)
        {
            if(g_venc_info[chn][stream].type != type)
            {
                DEV_WRITE_LOG_ERROR("chn=%d not be allowed to change type when stream is capturing",chn);
                return BEACON_ERR_ORDER;
            }

            if(g_venc_info[chn][stream].reso != reso)
            {
                DEV_WRITE_LOG_ERROR("chn=%d not be allowed to change reso when stream is capturing",chn);
                return BEACON_ERR_ORDER;
            }
        }

        g_venc_info[chn][stream].type = (BEACON_VIDEO_ENCODE_E)type;
        g_venc_info[chn][stream].fr = fr;
        g_venc_info[chn][stream].reso = (BEACON_VIDEO_RESOLUTION_E)reso;
        if(type == BEACON_VIDEO_ENCODE_H265)
        {
            memcpy(&g_venc_info[chn][stream].h265_cfg,arg,sizeof(g_venc_info[chn][stream].h265_cfg));
        }
        else if(type == BEACON_VIDEO_ENCODE_H264)
        {
            memcpy(&g_venc_info[chn][stream].h264_cfg,arg,sizeof(g_venc_info[chn][stream].h264_cfg));
        }
        else
        {
            DEV_WRITE_LOG_ERROR("chn=%d type=%d not supported",chn,type);
            return BEACON_ERR_INVALID_PARAM;
        }

        if(g_is_stream_start)
        {
            if(update_encode2device(chn,stream) != 0)
            {
                return BEACON_ERR_INTERNAL;
            }
        }

        DEV_WRITE_LOG_DEBUG("success");
        return BEACON_ERR_SUCCESS;
    }

    int beacon_encode_start(int chn,int stream)
    {
        LOCK_INTERFACE();

        if(!g_dev_inited)
        {
            DEV_WRITE_LOG_ERROR("device need be inited first");
            return BEACON_ERR_ORDER;
        }

        if(check_chn_stream_valid(chn,stream) != 0)
        {
            DEV_WRITE_LOG_ERROR("chn=%d,stream=%d is invalid",chn,stream);
            return BEACON_ERR_INVALID_PARAM;
        }

        if(!g_device_vin[chn].inited)
        {
            DEV_WRITE_LOG_ERROR("vi chn=%d need be inited first",chn);
            return BEACON_ERR_ORDER;
        }

        if(g_venc_info[chn][stream].enable)
        {
            DEV_WRITE_LOG_ERROR("venc chn=%d need be closed first",chn);
            return BEACON_ERR_ORDER;
        }

        ot_venc_chn venc_chn = g_venc_info[chn][stream].venc_chn;
        td_s32 ret = TD_FAILURE;
        ot_venc_chn_attr chn_attr;
        int pic_w = 3840;
        int pic_h = 2160;
        ot_payload_type enc_type;

        switch(g_venc_info[chn][stream].type)
        {
            case BEACON_VIDEO_ENCODE_H265:
                enc_type = OT_PT_H265;
                break;
            case BEACON_VIDEO_ENCODE_H264:
                enc_type = OT_PT_H264;
                break;
            default:
                return BEACON_ERR_INVALID_PARAM;
        }

        get_venc_w_h_by_reso(g_venc_info[chn][stream].reso,&pic_w,&pic_h);
        chn_attr.venc_attr.type = enc_type;
        chn_attr.venc_attr.max_pic_width = pic_w;
        chn_attr.venc_attr.max_pic_height = pic_h;
        chn_attr.venc_attr.pic_width = pic_w;/*the picture width*/
        chn_attr.venc_attr.pic_height    = pic_h;/*the picture height*/
        chn_attr.venc_attr.buf_size      = pic_w * pic_h *3 / 2;/*stream buffer size*/
        chn_attr.venc_attr.is_by_frame      = TD_TRUE;/*get stream mode is slice mode or frame mode?*/

        switch(g_venc_info[chn][stream].type)
        {
            case BEACON_VIDEO_ENCODE_H264:
                {
                    chn_attr.venc_attr.profile = g_venc_info[chn][stream].h264_cfg.profile;

                    if(g_venc_info[chn][stream].h264_cfg.bitrate_control == 0)
                    {
                        //CBR
                        chn_attr.rc_attr.rc_mode = OT_VENC_RC_MODE_H264_CBR;
                        chn_attr.rc_attr.h264_cbr.gop = g_venc_info[chn][stream].h264_cfg.gop_interval; /*the interval of IFrame*/
                        chn_attr.rc_attr.h264_cbr.stats_time = g_venc_info[chn][stream].h265_cfg.gop_interval / g_venc_info[chn][stream].fr; /* stream rate statics time(s) */
                        chn_attr.rc_attr.h264_cbr.src_frame_rate= g_device_vin[chn].fr; /* input (vi) frame rate */
                        chn_attr.rc_attr.h264_cbr.dst_frame_rate = g_venc_info[chn][stream].fr; /* target frame rate */
                        chn_attr.rc_attr.h264_cbr.bit_rate = g_venc_info[chn][stream].h264_cfg.bitrate_avg;
                    }
                    else if(g_venc_info[chn][stream].h264_cfg.bitrate_control == 1)
                    {
                        //VBR
                        chn_attr.rc_attr.rc_mode = OT_VENC_RC_MODE_H264_VBR;
                        chn_attr.rc_attr.h264_vbr.gop = g_venc_info[chn][stream].h264_cfg.gop_interval;
                        chn_attr.rc_attr.h264_vbr.stats_time = g_venc_info[chn][stream].h265_cfg.gop_interval / g_venc_info[chn][stream].fr;
                        chn_attr.rc_attr.h264_vbr.src_frame_rate = g_device_vin[chn].fr;
                        chn_attr.rc_attr.h264_vbr.dst_frame_rate = g_venc_info[chn][stream].fr;
                        chn_attr.rc_attr.h264_vbr.max_bit_rate = g_venc_info[chn][stream].h264_cfg.bitrate_max;
                    }
                    else if(g_venc_info[chn][stream].h264_cfg.bitrate_control == 2)
                    {
                        //AVBR
                        chn_attr.rc_attr.rc_mode= OT_VENC_RC_MODE_H264_AVBR;
                        chn_attr.rc_attr.h264_avbr.gop = g_venc_info[chn][stream].h264_cfg.gop_interval;
                        chn_attr.rc_attr.h264_avbr.stats_time = g_venc_info[chn][stream].h265_cfg.gop_interval / g_venc_info[chn][stream].fr;
                        chn_attr.rc_attr.h264_avbr.src_frame_rate = g_device_vin[chn].fr;
                        chn_attr.rc_attr.h264_avbr.dst_frame_rate = g_venc_info[chn][stream].fr;
                        chn_attr.rc_attr.h264_avbr.max_bit_rate = g_venc_info[chn][stream].h264_cfg.bitrate_max;
                    }
                    else
                    {
                        DEV_WRITE_LOG_ERROR("bitrate_control=%d not support",g_venc_info[chn][stream].h264_cfg.bitrate_control);
                        return BEACON_ERR_INVALID_PARAM;
                    }
                    chn_attr.venc_attr.h264_attr.rcn_ref_share_buf_en = TD_FALSE;
                    chn_attr.venc_attr.h264_attr.frame_buf_ratio = 70;
                    break;
                }

            case BEACON_VIDEO_ENCODE_H265:
                {
                    chn_attr.venc_attr.profile = g_venc_info[chn][stream].h265_cfg.profile;

                    if(g_venc_info[chn][stream].h265_cfg.bitrate_control == 0)
                    {
                        //CBR
                        chn_attr.rc_attr.rc_mode = OT_VENC_RC_MODE_H265_CBR;
                        chn_attr.rc_attr.h265_cbr.gop = g_venc_info[chn][stream].h265_cfg.gop_interval;
                        chn_attr.rc_attr.h265_cbr.stats_time = g_venc_info[chn][stream].h265_cfg.gop_interval / g_venc_info[chn][stream].fr;
                        chn_attr.rc_attr.h265_cbr.src_frame_rate = g_device_vin[chn].fr; 
                        chn_attr.rc_attr.h265_cbr.dst_frame_rate = g_venc_info[chn][stream].fr;
                        chn_attr.rc_attr.h265_cbr.bit_rate = g_venc_info[chn][stream].h265_cfg.bitrate_avg;
                    }
                    else if(g_venc_info[chn][stream].h265_cfg.bitrate_control == 1)
                    {
                        //VBR
                        chn_attr.rc_attr.rc_mode = OT_VENC_RC_MODE_H265_VBR;
                        chn_attr.rc_attr.h265_vbr.gop = g_venc_info[chn][stream].h265_cfg.gop_interval;
                        chn_attr.rc_attr.h265_vbr.stats_time = g_venc_info[chn][stream].h265_cfg.gop_interval / g_venc_info[chn][stream].fr;
                        chn_attr.rc_attr.h265_vbr.src_frame_rate = g_device_vin[chn].fr;
                        chn_attr.rc_attr.h265_vbr.dst_frame_rate = g_venc_info[chn][stream].fr;
                        chn_attr.rc_attr.h265_vbr.max_bit_rate = g_venc_info[chn][stream].h265_cfg.bitrate_max;
                    }
                    else if(g_venc_info[chn][stream].h265_cfg.bitrate_control == 2)
                    {
                        chn_attr.rc_attr.rc_mode = OT_VENC_RC_MODE_H265_AVBR;
                        chn_attr.rc_attr.h265_avbr.gop = g_venc_info[chn][stream].h265_cfg.gop_interval;
                        chn_attr.rc_attr.h265_avbr.stats_time = g_venc_info[chn][stream].h265_cfg.gop_interval / g_venc_info[chn][stream].fr;
                        chn_attr.rc_attr.h265_avbr.src_frame_rate = g_device_vin[chn].fr;
                        chn_attr.rc_attr.h265_avbr.dst_frame_rate = g_venc_info[chn][stream].fr;
                        chn_attr.rc_attr.h265_avbr.max_bit_rate = g_venc_info[chn][stream].h265_cfg.bitrate_max;
                    }
                    else
                    {
                        DEV_WRITE_LOG_ERROR("bitrate_control=%d not support",g_venc_info[chn][stream].h264_cfg.bitrate_control);
                        return BEACON_ERR_INVALID_PARAM;
                    }
                    chn_attr.venc_attr.h264_attr.rcn_ref_share_buf_en = TD_FALSE;
                    chn_attr.venc_attr.h264_attr.frame_buf_ratio = 70;
                    break;
                }

            default:
                DEV_WRITE_LOG_ERROR("type=%d not support",g_venc_info[chn][stream].type);
                return BEACON_ERR_INVALID_PARAM;
        }

        chn_attr.gop_attr.gop_mode = OT_VENC_GOP_MODE_NORMAL_P;
        chn_attr.gop_attr.normal_p.ip_qp_delta = 2; /* 2 is a number */

        ret = ss_mpi_venc_create_chn(venc_chn,&chn_attr);
        if (TD_SUCCESS != ret)
        {
            DEV_WRITE_LOG_ERROR("ss_mpi_venc_create_chn[%d] faild with %#x!",venc_chn, ret);
            return BEACON_ERR_INTERNAL;
        }

        update_encode2device(chn,stream);

        ot_venc_start_param venc_start_param;
        venc_start_param.recv_pic_num = -1;
        ret = ss_mpi_venc_start_chn(venc_chn,&venc_start_param);
        if (TD_SUCCESS != ret)
        {
            DEV_WRITE_LOG_ERROR("HI_MPI_VENC_StartRecvPic faild with%#x!", ret);
            return BEACON_ERR_INTERNAL;
        }

        ot_vpss_grp vpss_grp = g_device_vin[chn].vpss_grp;
        ot_vpss_chn vpss_chn = 0;
        ot_mpp_chn src_chn;
        ot_mpp_chn dest_chn;
        src_chn.mod_id = OT_ID_VPSS;
        src_chn.dev_id = vpss_grp;
        src_chn.chn_id = vpss_chn;
        dest_chn.mod_id = OT_ID_VENC;
        dest_chn.dev_id = 0;
        dest_chn.chn_id = venc_chn;
        ret = ss_mpi_sys_bind(&src_chn, &dest_chn);
        if(ret != TD_SUCCESS)
        {
            DEV_WRITE_LOG_ERROR("ss_mpi_sys_bind failed with %#x",ret);
            return BEACON_ERR_INTERNAL;
        }

        g_venc_info[chn][stream].enable = 1;

        DEV_WRITE_LOG_DEBUG("success");
        return BEACON_ERR_SUCCESS;
    }

    int beacon_encode_stop(int chn,int stream)
    {
        LOCK_INTERFACE();

        if(!g_dev_inited)
        {
            DEV_WRITE_LOG_ERROR("device need be inited first");
            return BEACON_ERR_ORDER;
        }

        if(check_chn_stream_valid(chn,stream) != 0)
        {
            DEV_WRITE_LOG_ERROR("chn=%d,stream=%d is invalid",chn,stream);
            return BEACON_ERR_INVALID_PARAM;
        }

        if(!g_device_vin[chn].inited)
        {
            DEV_WRITE_LOG_ERROR("vi chn=%d need be inited first",chn);
            return BEACON_ERR_ORDER;
        }

        if(!g_venc_info[chn][stream].enable)
        {
            DEV_WRITE_LOG_ERROR("venc chn=%d need be enabled first",chn);
            return BEACON_ERR_ORDER;
        }

        td_s32 ret = TD_SUCCESS;

        g_venc_info[chn][stream].enable = 0;

        ot_vpss_grp vpss_grp = g_device_vin[chn].vpss_grp;
        ot_vpss_chn vpss_chn = 0;
        ot_venc_chn venc_chn = g_venc_info[chn][stream].venc_chn;

        ot_mpp_chn src_chn;
        ot_mpp_chn dest_chn;
        src_chn.mod_id = OT_ID_VPSS;
        src_chn.dev_id = vpss_grp;
        src_chn.chn_id = vpss_chn;
        dest_chn.mod_id = OT_ID_VENC;
        dest_chn.dev_id = 0;
        dest_chn.chn_id = venc_chn;
        ret = ss_mpi_sys_unbind(&src_chn, &dest_chn);
        if(ret != TD_SUCCESS)
        {
            DEV_WRITE_LOG_ERROR("ss_mpi_sys_bind failed with %#x",ret);
            return BEACON_ERR_INTERNAL;
        }

        ret = ss_mpi_venc_stop_chn(venc_chn);
        if(ret != TD_SUCCESS)
        {
            DEV_WRITE_LOG_ERROR("ss_mpi_venc_stop failed with %#x",ret);
            return BEACON_ERR_INTERNAL;
        }

        ret = ss_mpi_venc_destroy_chn(venc_chn);
        if(ret != TD_SUCCESS)
        {
            DEV_WRITE_LOG_ERROR("ss_mpi_venc_destroy_chn failed with %#x",ret);
            return BEACON_ERR_INTERNAL;
        }

        DEV_WRITE_LOG_DEBUG("success");
        return BEACON_ERR_SUCCESS;
    }

    int beacon_encode_start_capture()
    {
        LOCK_INTERFACE();

        if(!g_dev_inited)
        {
            DEV_WRITE_LOG_ERROR("device need be inited first");
            return BEACON_ERR_ORDER;
        }

        if(g_is_stream_start)
        {
            DEV_WRITE_LOG_ERROR("streams are being captured,need be stopped first");
            return BEACON_ERR_ORDER;
        }

        g_is_stream_start = 1;
        pthread_create(&g_venc_pid, NULL, on_venc_capture, (void*)(td_ulong)g_is_stream_start);

        DEV_WRITE_LOG_DEBUG("success");
        return BEACON_ERR_SUCCESS;
    }
    
    int beacon_encode_stop_capture()
    {
        LOCK_INTERFACE();

        if(!g_dev_inited)
        {
            DEV_WRITE_LOG_ERROR("device need be inited first");
            return BEACON_ERR_ORDER;
        }

        if(!g_is_stream_start)
        {
            DEV_WRITE_LOG_ERROR("streams need be captured first");
            return BEACON_ERR_ORDER;
        }

        g_is_stream_start = 0;
        pthread_join(g_venc_pid,NULL);

        DEV_WRITE_LOG_DEBUG("success");
        return BEACON_ERR_SUCCESS;
    }


    static void on_osd_refresh(int chn,int stream)
    {
        time_t last_tm = 0;
        time_t cur_tm;
        struct tm cur;
        char cur_osd_date_str[255] = {0};
        td_s32 ret; 
        ot_rgn_canvas_info canvas_info;

        while(g_osd_date_info[chn][stream].enable)
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

            //printf("%s,len=%d\n",str,strlen(str));
            ret = ss_mpi_rgn_get_canvas_info(g_osd_date_info[chn][stream].rgn_h, &canvas_info);
            if (TD_SUCCESS != ret)
            {
                DEV_WRITE_LOG_ERROR("HI_MPI_RGN_GetCanvasInfo failed! s32Ret: 0x%x", ret);
                return ;
            }

            if(strlen(g_osd_date_info[chn][stream].last_date_str) == 0)
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
                        g_osd_date_info[chn][stream].area_w,
                        g_osd_date_info[chn][stream].area_h,
                        g_osd_date_info[chn][stream].font_size,
                        g_osd_date_info[chn][stream].outline,
                        (unsigned char*)canvas_info.virt_addr,
                        canvas_info.size.width * canvas_info.size.height * 2);

            }
            else
            {
                g_freetype.show_string_compare(
                        g_osd_date_info[chn][stream].last_date_str,
                        cur_osd_date_str,
                        g_osd_date_info[chn][stream].area_w,
                        g_osd_date_info[chn][stream].area_h,
                        g_osd_date_info[chn][stream].font_size,
                        g_osd_date_info[chn][stream].outline,
                        (unsigned char*)canvas_info.virt_addr,
                        canvas_info.size.width * canvas_info.size.height * 2);
            }

            strcpy(g_osd_date_info[chn][stream].last_date_str,cur_osd_date_str);

            ret = ss_mpi_rgn_update_canvas(g_osd_date_info[chn][stream].rgn_h);
            if (TD_SUCCESS != ret)
            {
                DEV_WRITE_LOG_ERROR("HI_MPI_RGN_UpdateCanvas failed! s32Ret: 0x%x", ret);
                break;
            }

        }
    }

    int beacon_osd_date_set(int chn,int stream,int enable,int font_size,int color,int alpha,int outline,int x,int y,int type,int show_week)
    {
        LOCK_INTERFACE();

        if(!g_dev_inited)
        {
            DEV_WRITE_LOG_ERROR("device need be inited first");
            return BEACON_ERR_ORDER;
        }

        if(check_chn_stream_valid(chn,stream) != 0)
        {
            DEV_WRITE_LOG_ERROR("chn=%d,stream=%d is invalid",chn,stream);
            return BEACON_ERR_INVALID_PARAM;
        }

        if(!g_device_vin[chn].inited)
        {
            DEV_WRITE_LOG_ERROR("vi chn=%d need be inited first",chn);
            return BEACON_ERR_ORDER;
        }

        if(!g_venc_info[chn][stream].enable)
        {
            DEV_WRITE_LOG_ERROR("venc chn=%d need be enabled first",chn);
            return BEACON_ERR_ORDER;
        }

        td_s32 ret; 

        if(g_osd_date_info[chn][stream].enable)
        {
            g_osd_date_info[chn][stream].enable = 0;

            g_osd_date_info[chn][stream].dthread.join();
        }

        if(g_osd_date_info[chn][stream].rgn_h != 0xFFFFFFFF)
        {
            ss_mpi_rgn_destroy(g_osd_date_info[chn][stream].rgn_h); 
            g_osd_date_info[chn][stream].rgn_h = 0xFFFFFFFF;
        }

        if(!enable)
        {
            return BEACON_ERR_SUCCESS;
        }

        int venc_w = 0;
        int venc_h = 0;
        int left = ROUND_DOWN(x,4);
        int top = ROUND_DOWN(y,4);

        get_venc_w_h_by_reso(g_venc_info[chn][stream].reso,&venc_w,&venc_h);

        time_t cur_tm = time(NULL);
        struct tm cur;
        localtime_r(&cur_tm,&cur);
        char data_str[255];
        sprintf(data_str,"%s %04d-%02d-%02d %02d:%02d:%02d",g_week_stsr[cur.tm_wday],cur.tm_year + 1900,cur.tm_mon + 1,cur.tm_mday,cur.tm_hour,cur.tm_min,cur.tm_sec);

        g_osd_date_info[chn][stream].font_size = font_size;
        g_osd_date_info[chn][stream].x = left;
        g_osd_date_info[chn][stream].y = top;
        g_osd_date_info[chn][stream].color = color;
        g_osd_date_info[chn][stream].type = type;
        g_osd_date_info[chn][stream].alpha = alpha;
        g_osd_date_info[chn][stream].outline = outline;
        g_freetype.get_width(data_str,font_size,&g_osd_date_info[chn][stream].area_w);
        g_osd_date_info[chn][stream].area_w  = ROUND_UP(g_osd_date_info[chn][stream].area_w,32);
        g_osd_date_info[chn][stream].area_h  = ROUND_UP(font_size + 1,32);
        g_osd_date_info[chn][stream].rgn_h = (chn * MAX_VENC_COUNT + stream) * 2;

        ot_rgn_attr rgn_attr;
        rgn_attr.type = OT_RGN_OVERLAY; 
        rgn_attr.attr.overlay.pixel_format = OT_PIXEL_FORMAT_ARGB_1555;
        rgn_attr.attr.overlay.size.width = g_osd_date_info[chn][stream].area_w;
        rgn_attr.attr.overlay.size.height = g_osd_date_info[chn][stream].area_h;
        rgn_attr.attr.overlay.bg_color = rgb24to1555(0,255,0,0);
        rgn_attr.attr.overlay.canvas_num = 2;
        //printf("area_w =%d,area_h=%d\n",g_osd_date_info[chn][stream].area_w,g_osd_date_info[chn][stream].area_h);
        ret = ss_mpi_rgn_create(g_osd_date_info[chn][stream].rgn_h, &rgn_attr);
        if (ret != TD_SUCCESS) 
        {
            DEV_WRITE_LOG_ERROR("HI_MPI_RGN_Create failed! s32Ret: 0x%x", ret); 
            return BEACON_ERR_INTERNAL;
        }

        if(g_osd_date_info[chn][stream].x + g_osd_date_info[chn][stream].area_w > venc_w) 
        {
            g_osd_date_info[chn][stream].x = venc_w - g_osd_date_info[chn][stream].area_w;
        }
        if(g_osd_date_info[chn][stream].y + g_osd_date_info[chn][stream].area_h > venc_h)
        {
            g_osd_date_info[chn][stream].y = venc_h - g_osd_date_info[chn][stream].area_h;
        }
        g_osd_date_info[chn][stream].x = ROUND_DOWN(g_osd_date_info[chn][stream].x,4);
        g_osd_date_info[chn][stream].y = ROUND_DOWN(g_osd_date_info[chn][stream].y,4);

        ot_rgn_chn_attr chn_attr; 
        memset(&chn_attr,0,sizeof(chn_attr));
        chn_attr.is_show = TD_TRUE;
        chn_attr.type = OT_RGN_OVERLAY;
        chn_attr.attr.overlay_chn.point.x = g_osd_date_info[chn][stream].x;
        chn_attr.attr.overlay_chn.point.y = g_osd_date_info[chn][stream].y;
        chn_attr.attr.overlay_chn.bg_alpha = g_osd_date_info[chn][stream].alpha;
        chn_attr.attr.overlay_chn.fg_alpha = 128;
        chn_attr.attr.overlay_chn.layer = 0; 
        chn_attr.attr.overlay_chn.qp_info.is_abs_qp = TD_FALSE;
        chn_attr.attr.overlay_chn.qp_info.qp_val = 0;
		chn_attr.attr.overlay_chn.qp_info.enable = TD_FALSE;
        chn_attr.attr.overlay_chn.dst = OT_RGN_ATTACH_JPEG_MAIN;

        ot_mpp_chn src_chn;
        src_chn.mod_id = OT_ID_VENC;
        src_chn.dev_id = 0;
        src_chn.chn_id = g_venc_info[chn][stream].venc_chn;

        g_osd_date_info[chn][stream].rgn_attr = chn_attr;
        ret = ss_mpi_rgn_attach_to_chn(g_osd_date_info[chn][stream].rgn_h, &src_chn, &chn_attr);
        if (ret != TD_SUCCESS)
        {
            DEV_WRITE_LOG_ERROR("HI_MPI_RGN_AttachToChn failed! s32Ret: 0x%x", ret); 
            return BEACON_ERR_INTERNAL;
        }

        g_osd_date_info[chn][stream].enable  = 1;
        g_osd_date_info[chn][stream].last_date_str[0] = '\0';
        g_osd_date_info[chn][stream].dthread  = std::thread(&on_osd_refresh,chn,stream);

        DEV_WRITE_LOG_DEBUG("success");
        return BEACON_ERR_SUCCESS;
    }

    int beacon_osd_name_set(int chn,int stream,int enable,int font_size,int color,int alpha,int outline,int x,int y,const char* str_name)
    {
        LOCK_INTERFACE();

        if(!g_dev_inited)
        {
            DEV_WRITE_LOG_ERROR("device need be inited first");
            return BEACON_ERR_ORDER;
        }

        if(check_chn_stream_valid(chn,stream) != 0)
        {
            DEV_WRITE_LOG_ERROR("chn=%d,stream=%d is invalid",chn,stream);
            return BEACON_ERR_INVALID_PARAM;
        }

        if(!g_device_vin[chn].inited)
        {
            DEV_WRITE_LOG_ERROR("vi chn=%d need be inited first",chn);
            return BEACON_ERR_ORDER;
        }

        if(!g_venc_info[chn][stream].enable)
        {
            DEV_WRITE_LOG_ERROR("venc chn=%d need be enabled first",chn);
            return BEACON_ERR_ORDER;
        }

        td_s32 ret; 

        if(g_osd_name_info[chn][stream].rgn_h != 0xFFFFFFFF)
        {
            ss_mpi_rgn_destroy(g_osd_name_info[chn][stream].rgn_h); 
            g_osd_name_info[chn][stream].rgn_h = 0xFFFFFFFF;
        }

        g_osd_name_info[chn][stream].enable = enable;

        if(!enable || str_name == NULL
                || strlen(str_name) == 0)
        {
            return BEACON_ERR_SUCCESS;
        }

        int venc_w = 0;
        int venc_h = 0;
        int left = ROUND_DOWN(x,4);
        int top = ROUND_DOWN(y,4);

        get_venc_w_h_by_reso(g_venc_info[chn][stream].reso,&venc_w,&venc_h);

        sprintf(g_osd_name_info[chn][stream].name,"%s",str_name);
        g_osd_name_info[chn][stream].font_size = font_size;
        g_osd_name_info[chn][stream].x = left;
        g_osd_name_info[chn][stream].y = top;
        g_osd_name_info[chn][stream].outline = outline;
        g_osd_name_info[chn][stream].alpha = alpha;

        g_freetype.get_width(str_name,font_size,&g_osd_name_info[chn][stream].area_w);
        g_osd_name_info[chn][stream].area_w = ROUND_UP(g_osd_name_info[chn][stream].area_w,64);
        if(g_osd_name_info[chn][stream].area_w  > venc_w)
        {
            DEV_WRITE_LOG_ERROR("osd name areaw(%d) > vencw(%d)",g_osd_name_info[chn][stream].area_w,venc_w);
            return BEACON_ERR_INVALID_PARAM;
        }
        g_osd_name_info[chn][stream].area_h = ROUND_UP(font_size + 1,32);

        g_osd_name_info[chn][stream].rgn_h = (chn * MAX_VENC_COUNT + stream) * 2 + 1;

        ot_rgn_attr rgn_attr;
        rgn_attr.type = OT_RGN_OVERLAY; 
        rgn_attr.attr.overlay.pixel_format = OT_PIXEL_FORMAT_ARGB_1555;
        rgn_attr.attr.overlay.size.width  = g_osd_name_info[chn][stream].area_w;
        rgn_attr.attr.overlay.size.height = g_osd_name_info[chn][stream].area_h;
        rgn_attr.attr.overlay.bg_color = rgb24to1555(0,255,0,0);
        rgn_attr.attr.overlay.canvas_num = 2;
        ret = ss_mpi_rgn_create(g_osd_name_info[chn][stream].rgn_h, &rgn_attr);
        if (ret != TD_SUCCESS) 
        {
            DEV_WRITE_LOG_ERROR("HI_MPI_RGN_Create failed! s32Ret: 0x%x", ret); 
            return BEACON_ERR_INTERNAL;
        }

        if(g_osd_name_info[chn][stream].x + g_osd_name_info[chn][stream].area_w > venc_w)
        {
            g_osd_name_info[chn][stream].x = venc_w - g_osd_name_info[chn][stream].area_w;
        }
        if(g_osd_name_info[chn][stream].y + g_osd_name_info[chn][stream].area_h > venc_h)
        {
            g_osd_name_info[chn][stream].y  = venc_h -  g_osd_name_info[chn][stream].area_h;
        }
        g_osd_name_info[chn][stream].x = ROUND_DOWN(g_osd_name_info[chn][stream].x,4);
        g_osd_name_info[chn][stream].y = ROUND_DOWN(g_osd_name_info[chn][stream].y,4);

        ot_rgn_chn_attr chn_attr; 
        memset(&chn_attr,0,sizeof(chn_attr));
        chn_attr.is_show = TD_TRUE;
        chn_attr.type = OT_RGN_OVERLAY;
        chn_attr.attr.overlay_chn.point.x = g_osd_name_info[chn][stream].x;
        chn_attr.attr.overlay_chn.point.y = g_osd_name_info[chn][stream].y;
        chn_attr.attr.overlay_chn.bg_alpha = g_osd_name_info[chn][stream].alpha;
        chn_attr.attr.overlay_chn.fg_alpha = 128;
        chn_attr.attr.overlay_chn.layer = 0; 
        chn_attr.attr.overlay_chn.qp_info.is_abs_qp = TD_FALSE;
        chn_attr.attr.overlay_chn.qp_info.qp_val = 0;
		chn_attr.attr.overlay_chn.qp_info.enable = TD_FALSE;
        chn_attr.attr.overlay_chn.dst = OT_RGN_ATTACH_JPEG_MAIN;

        ot_mpp_chn src_chn;
        src_chn.mod_id = OT_ID_VENC;
        src_chn.dev_id = 0;
        src_chn.chn_id = g_venc_info[chn][stream].venc_chn;

        g_osd_name_info[chn][stream].rgn_attr = chn_attr;
        ret = ss_mpi_rgn_attach_to_chn(g_osd_name_info[chn][stream].rgn_h, &src_chn, &chn_attr);
        if (ret != TD_SUCCESS)
        {
            DEV_WRITE_LOG_ERROR("HI_MPI_RGN_AttachToChn failed! s32Ret: 0x%x", ret);
            return BEACON_ERR_INTERNAL;
        }

        ot_rgn_canvas_info canvas_info;
        ret = ss_mpi_rgn_get_canvas_info(g_osd_name_info[chn][stream].rgn_h, &canvas_info);
        if (TD_SUCCESS != ret)
        {
            DEV_WRITE_LOG_ERROR("HI_MPI_RGN_GetCanvasInfo failed! s32Ret: 0x%x", ret);
            return BEACON_ERR_INTERNAL;
        }

        g_freetype.show_string(g_osd_name_info[chn][stream].name,
                g_osd_name_info[chn][stream].area_w,
                g_osd_name_info[chn][stream].area_h,
                g_osd_name_info[chn][stream].font_size,
                g_osd_name_info[chn][stream].outline,
                (unsigned char*)canvas_info.virt_addr,
                canvas_info.size.width * canvas_info.size.height * 2);

        ret = ss_mpi_rgn_update_canvas(g_osd_name_info[chn][stream].rgn_h);
        if (TD_SUCCESS != ret)
        {
            DEV_WRITE_LOG_ERROR("HI_MPI_RGN_UpdateCanvas failed! s32Ret: 0x%x", ret);
            return BEACON_ERR_INTERNAL;
        }

        DEV_WRITE_LOG_DEBUG("success");
        return BEACON_ERR_SUCCESS;
    }

    int beacon_encode_register_stream_callback(int fun_type,void (*fun)(int chn,int stream,int type, uint64_t time_stamp,const char* buf,int len,int64_t userdata),int64_t userdata)
    {
        LOCK_INTERFACE();

        g_stream_fun_type = fun_type;
        g_stream_fun = fun;
        g_stream_fun_usr = userdata;
        
        DEV_WRITE_LOG_DEBUG("success");
        return 0;
    }

    int beacon_vi_snap_ex(int chn,int w,int h,int quality,char* jpg_buf,int buf_len,int* jpg_len,vi_snap_date_t* osd_date,vi_snap_name_t* osd_name,vi_sub_snap_t* psub_jpg)
    {
        return BEACON_ERR_SUCCESS;
    }

    int beacon_vi_snap(int chn,int w,int h,int quality,char* jpg_buf,int buf_len,int* jpg_len,vi_snap_date_t* osd_date,vi_snap_name_t* osd_name)
    {
        return BEACON_ERR_SUCCESS;
    }

    int beacon_encode_request_i_frame(int chn,int stream)
    {
        LOCK_INTERFACE();

        td_s32 ret;

        if(!g_dev_inited)
        {
            DEV_WRITE_LOG_ERROR("device need be inited first");
            return BEACON_ERR_ORDER;
        }

        if(check_chn_stream_valid(chn,stream) != 0)
        {
            DEV_WRITE_LOG_ERROR("chn=%d,stream=%d is invalid",chn,stream);
            return BEACON_ERR_INVALID_PARAM;
        }

        if(!g_device_vin[chn].inited)
        {
            DEV_WRITE_LOG_ERROR("vi chn=%d need be inited first",chn);
            return BEACON_ERR_ORDER;
        }

        if(!g_venc_info[chn][stream].enable)
        {
            DEV_WRITE_LOG_ERROR("venc chn=%d need be enabled first",chn);
            return BEACON_ERR_ORDER;
        }

        ret = ss_mpi_venc_request_idr(g_venc_info[chn][stream].venc_chn,TD_TRUE);
        if(ret != TD_SUCCESS)
        {
            DEV_WRITE_LOG_ERROR("ss_mpi_venc_request_idr failed with 0x%x",ret);
            return BEACON_ERR_INTERNAL;
        }

        DEV_WRITE_LOG_DEBUG("success");
        return BEACON_ERR_SUCCESS;
    }

    int beacon_img_get_current_value(int chn,beacon_img_value_t* val)
    {
        //LOCK_INTERFACE();

        if(!g_dev_inited)
        {
            DEV_WRITE_LOG_ERROR("device need be inited first");
            return BEACON_ERR_ORDER;
        }

        if(check_chn_valid(chn)!= 0)
        {
            DEV_WRITE_LOG_ERROR("chn=%d is invalid",chn);
            return BEACON_ERR_INVALID_PARAM;
        }

        if(!g_device_vin[chn].inited)
        {
            DEV_WRITE_LOG_ERROR("vi need ve inited first");
            return BEACON_ERR_ORDER;
        }

        td_s32 ret;
        ot_vi_pipe vi_pipe= g_device_vin[chn].pipe[0];
        ot_isp_exp_info info;

        ret = ss_mpi_isp_query_exposure_info(vi_pipe,&info);
        if(ret != TD_SUCCESS)
        {
            DEV_WRITE_LOG_ERROR("ss_mpi_isp_query_exposure_info() failed with %#x",ret);
            return BEACON_ERR_INTERNAL;
        }

        ot_isp_exposure_attr expo_attr;
        memset(&expo_attr,0,sizeof(expo_attr));
        ret = ss_mpi_isp_get_exposure_attr(vi_pipe,&expo_attr);
        if(ret != TD_SUCCESS)
        {
            DEV_WRITE_LOG_ERROR("ss_mpi_isp_get_exposure_attr failed with %#x",ret);
            return BEACON_ERR_INTERNAL;
        }

        memset(val,0,sizeof(beacon_img_value_t));
        val->exp_time = info.exp_time;
        val->again = info.a_gain>> 10;
        val->dgain = info.d_gain>> 10;
        val->ispgain = info.isp_d_gain>> 10;
        val->iso = info.iso;
        val->is_max_exposure = info.exposure_is_max;
        val->is_exposure_stable = abs(info.hist_error) <= expo_attr.auto_attr.tolerance ? 1 : 0;

        return BEACON_ERR_SUCCESS;
    }

    int beacon_img_enable_ae(int chn,int enable,int shutter,int agc)
    {
        LOCK_INTERFACE();

        if(!g_dev_inited)
        {
            DEV_WRITE_LOG_ERROR("device need be inited first");
            return BEACON_ERR_ORDER;
        }

        if(check_chn_valid(chn)!= 0)
        {
            DEV_WRITE_LOG_ERROR("chn=%d is invalid",chn);
            return BEACON_ERR_INVALID_PARAM;
        }

        if(!g_device_vin[chn].inited)
        {
            DEV_WRITE_LOG_ERROR("vi need ve inited first");
            return BEACON_ERR_ORDER;
        }

        td_s32 ret;
        ot_vi_pipe vi_pipe = g_device_vin[chn].pipe[0];

        ot_isp_exposure_attr expo_attr;
        memset(&expo_attr,0,sizeof(expo_attr));
        ret = ss_mpi_isp_get_exposure_attr(vi_pipe,&expo_attr);
        if(ret != TD_SUCCESS)
        {
            DEV_WRITE_LOG_ERROR("ss_mpi_isp_get-exposure_attr failed with %#x",ret);
            return BEACON_ERR_INTERNAL;
        }

        if(enable)
        {
            expo_attr.op_type = OT_OP_MODE_AUTO;
            if(shutter == 0)
            {
                shutter = g_device_vin[chn].fr;
            }

            expo_attr.auto_attr.exp_time_range.max = 1000000 / shutter; 
            expo_attr.auto_attr.exp_time_range.min = 10; 

            int again_max = 30;
            int dgain_max = 30;
            int ispgain_max = 4;

            expo_attr.auto_attr.a_gain_range.max = again_max << 10; 
            expo_attr.auto_attr.a_gain_range.min = 0x400; 
            expo_attr.auto_attr.d_gain_range.max = dgain_max << 10; 
            expo_attr.auto_attr.d_gain_range.min = 0x400; 
            expo_attr.auto_attr.ispd_gain_range.max = ispgain_max << 10; 
            expo_attr.auto_attr.ispd_gain_range.min = 0x400; 

            expo_attr.auto_attr.sys_gain_range.max = (again_max * dgain_max * ispgain_max) << 10; 
            expo_attr.auto_attr.sys_gain_range.min = 0x400;

            expo_attr.auto_attr.speed = 64; 
            if(shutter < g_device_vin[chn].fr)
            {
                expo_attr.auto_attr.gain_threshold = (again_max * (dgain_max / 2)) << 10;
                expo_attr.auto_attr.ae_mode = OT_ISP_AE_MODE_SLOW_SHUTTER;
            }
            else
            {
                expo_attr.auto_attr.ae_mode = OT_ISP_AE_MODE_FIX_FRAME_RATE;
            }
        }
        else
        {
            if(agc == 0)
            {
                agc = 1;
            }

             expo_attr.op_type  = OT_OP_MODE_MANUAL;
             expo_attr.manual_attr.exp_time_op_type = OT_OP_MODE_MANUAL;
             expo_attr.manual_attr.a_gain_op_type = OT_OP_MODE_MANUAL;
             expo_attr.manual_attr.d_gain_op_type = OT_OP_MODE_MANUAL;
             expo_attr.manual_attr.ispd_gain_op_type = OT_OP_MODE_MANUAL;
             expo_attr.manual_attr.exp_time = 1000000 / shutter;
             expo_attr.manual_attr.a_gain = agc << 10;;
             expo_attr.manual_attr.isp_d_gain = expo_attr.manual_attr.d_gain = 0x400;
        }

        ret = ss_mpi_isp_set_exposure_attr(vi_pipe,&expo_attr);
        if(ret != TD_SUCCESS)
        {
            DEV_WRITE_LOG_ERROR("ss_mpi_isp_set_exposure failed with %#x",ret);
            return BEACON_ERR_INTERNAL;
        }

        return BEACON_ERR_SUCCESS;
    }

    int beacon_encode_audio_set_arg(int chn,int enable,int type,beacon_audio_config_t* arg)
    {
        LOCK_INTERFACE();

        return BEACON_ERR_SUCCESS;
    }

    int beacon_device_set_mode(int mode)
    {
        LOCK_INTERFACE();

        return BEACON_ERR_SUCCESS;
    }

    int beacon_vi_set_ldc_attr(int chn,beacon_ldc_attr_t* ldc_attr)
    {
        LOCK_INTERFACE();

        return BEACON_ERR_SUCCESS;
    }

    int beacon_img_set_color(int chn,int bright,int contrast,int saturation,int hue)
    {
        LOCK_INTERFACE();
        return BEACON_ERR_SUCCESS;
    }

}
