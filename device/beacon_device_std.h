#ifndef beacon_device_std_include_h
#define beacon_device_std_include_h

#include "ot_common.h"
#include "ot_math.h"
#include "ot_buffer.h"
#include "ot_defines.h"
#include "securec.h"
#include "ot_mipi_rx.h"
#include "ot_mipi_tx.h"
#include "ss_mpi_sys.h"
#include "ss_mpi_sys_bind.h"
#include "ss_mpi_vb.h"
#include "ss_mpi_vi.h"
#include "ss_mpi_isp.h"
#include "ss_mpi_vo.h"
#include "ss_mpi_venc.h"
#include "ss_mpi_vdec.h"
#include "ss_mpi_vpss.h"
#include "ss_mpi_region.h"
#include "ss_mpi_audio.h"
#include "ss_mpi_vgs.h"
#include "ss_mpi_awb.h"
#include "ss_mpi_ae.h"
#include "ot_sns_ctrl.h"
#include "ss_mpi_isp.h"

extern "C"
{
}
extern "C"
{
//#include <gpio.h>
//#include <reg_rw.h>
}
#if 0
#include "mpi_sys.h"
#include "mpi_vb.h"
#include "mpi_isp.h"
#include "mpi_vi.h"
#include "hi_sns_ctrl.h"
#include "mpi_ae.h"
#include "mpi_awb.h"
#include "mpi_vpss.h"
#include "mpi_venc.h"
#include "mpi_region.h"
#include "mpi_snap.h"
#include "mpi_audio.h"
#endif

#include <beacon_log.h>
extern LOG_HANDLE g_dev_log;
#define DEV_WRITE_LOG_ERROR(info,...)       WRITE_LOG_ERROR(g_dev_log,info,##__VA_ARGS__)
#define DEV_WRITE_LOG_DEBUG(info,...)       WRITE_LOG_DEBUG(g_dev_log,info,##__VA_ARGS__)
#define DEV_WRITE_LOG_INFO(info,...)        WRITE_LOG_INFO(g_dev_log,info,##__VA_ARGS__)
#define DEV_WRITE_LOG_WARN(info,...)        WRITE_LOG_WARN(g_dev_log,info,##__VA_ARGS__)
#define DEV_WRITE_LOG_FATAL(info,...)       WRITE_LOG_FATAL(g_dev_log,info,##__VA_ARGS__)

typedef struct
{
    int enable;
    int font_size;
    int x;
    int y;
    int color;
    int area_w;
    int area_h;
    int outline;
    int alpha;
    int type;
    ot_rgn_handle rgn_h;
    std::thread dthread;
    char last_date_str[64];
    
    ot_rgn_chn_attr rgn_attr; 
}osd_date_info_t;
extern osd_date_info_t g_osd_date_info[MAX_CHANNEL][MAX_VENC_COUNT];

typedef struct
{
    int enable;
    int font_size;
    int x;
    int y;
    int color;
    int area_w;
    int area_h;
    int outline;
    int alpha;
    ot_rgn_handle rgn_h;
    char name[255];

    ot_rgn_chn_attr rgn_attr; 
}osd_name_info_t;
extern osd_name_info_t g_osd_name_info[MAX_CHANNEL][MAX_VENC_COUNT];

typedef struct
{
    int enable;
    BEACON_VIDEO_ENCODE_E type;
    BEACON_VIDEO_RESOLUTION_E reso;
    int fr;
    union
    {
        beacon_h264_config_t h264_cfg;
        beacon_h265_config_t h265_cfg;
        beacon_mjpeg_config_t mjpg_cfg;
    };

    ot_venc_chn venc_chn;
}venc_info_t;
extern venc_info_t g_venc_info[MAX_CHANNEL][MAX_VENC_COUNT];

#define MAX_VI_SNAP (2)
typedef struct
{
    int inited;

    std::string name;
    int w;
    int h;
    int fr;

    int mipi_dev;
    int sns_clk_src;

    int vi_dev;

    int pipe_num;
    int pipe[5];
    int vpss_grp;

    int snap_venc[MAX_VI_SNAP];
    int snap_w[MAX_VI_SNAP];
    int snap_h[MAX_VI_SNAP];

    ot_rgn_handle rgn_osd_date_h[MAX_VI_SNAP];
    ot_rgn_handle rgn_osd_name_h[MAX_VI_SNAP];
}device_vin_t;
extern device_vin_t g_device_vin[MAX_CHANNEL];

#endif
