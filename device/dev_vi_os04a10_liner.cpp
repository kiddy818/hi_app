#include "dev_vi_os04a10_liner.h"
#include "dev_log.h"

namespace hisilicon{namespace dev{

    static combo_dev_attr_t g_dev_mipi_attr = {
        .devno = 0,
        .input_mode = INPUT_MODE_MIPI,
        .data_rate  = MIPI_DATA_RATE_X1,
        .img_rect   = {0, 0, 2688, 1520},
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


    vi_os04a10_liner::vi_os04a10_liner(int w,int h,int src_fr,int mipi_dev,int sns_clk_src,int vi_dev)
        :vi(w,h,src_fr,mipi_dev,sns_clk_src,vi_dev)
    {
    }

    vi_os04a10_liner::~vi_os04a10_liner()
    {
    }

    bool vi_os04a10_liner::start()
    {
        td_s32 ret;

        combo_dev_attr_t mipi_attr;
        memcpy(&mipi_attr,&g_dev_mipi_attr,sizeof(mipi_attr));

        mipi_attr.devno = m_mipi_dev;
        mipi_attr.img_rect.width = m_w;
        mipi_attr.img_rect.height = m_h;

        if(!start_mipi(&mipi_attr)
                || !reset_sns())
        {
            return false;
        }

        //enable dev
        ot_vi_dev_attr dev_attr;
        memcpy(&dev_attr,&g_mipi_raw_dev_attr, sizeof(ot_vi_dev_attr));
        dev_attr.in_size.width = m_w;
        dev_attr.in_size.height = m_h;
        dev_attr.component_mask[0] = 0xfff00000;
        ret = ss_mpi_vi_set_dev_attr(m_vi_dev, &dev_attr);
        if(ret != TD_SUCCESS)
        {
            DEV_WRITE_LOG_ERROR("ss_mpi_vi_set_dev_attr failed with %#x!", ret);
            return false;
        }

        return true;
    }

    void vi_os04a10_liner::stop()
    {
    }

}}//namespace

