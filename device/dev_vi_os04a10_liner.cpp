#include "dev_vi_os04a10_liner.h"
#include "dev_log.h"

extern ot_isp_sns_obj g_sns_os04a10_obj;

static combo_dev_attr_t g_mipi_4lane_chn0_sensor_os04a10_12bit_4m_nowdr_attr = {
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

static ot_isp_pub_attr g_isp_pub_attr_os04a10_mipi_4m_30fps = {
    {0, 0, 2688, 1520},
    {2688, 1520},
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

namespace hisilicon{namespace dev{

    vi_os04a10_liner::vi_os04a10_liner()
        :vi_isp(2688,/*w*/
                1520,/*h*/
                30,/*src frame*/
                0,/*vi dev*/
                0,/*mpip dev*/
                0,/*sns clk src*/
                0,/*wdr mode*/
                &g_sns_os04a10_obj,/*sns obj*/
                4/*i2c dev*/)
    {
        memcpy(&m_mipi_attr,&g_mipi_4lane_chn0_sensor_os04a10_12bit_4m_nowdr_attr,sizeof(combo_dev_attr_t));
        memcpy(&m_isp_pub_attr,&g_isp_pub_attr_os04a10_mipi_4m_30fps,sizeof(ot_isp_pub_attr));
    }

    vi_os04a10_liner::~vi_os04a10_liner()
    {
    }

}}//namespace

