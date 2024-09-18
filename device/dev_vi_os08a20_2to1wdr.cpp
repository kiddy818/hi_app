#include "dev_vi_os08a20_2to1wdr.h"
#include "dev_log.h"

static combo_dev_attr_t g_mipi_4lane_chn0_sensor_os08a20_12bit_wdr2to1 = {
    .devno = 0,
    .input_mode = INPUT_MODE_MIPI,
    .data_rate  = MIPI_DATA_RATE_X1,
    .img_rect   = {0, 0, 3840, 2184},
    .mipi_attr  = 
    {
        DATA_TYPE_RAW_12BIT,
        OT_MIPI_WDR_MODE_NONE,
        {0, 1, 2, 3, -1, -1, -1, -1}
    }
};

static ot_isp_pub_attr g_isp_pub_attr_os08a20_mipi_8m_30fps_wdr2to1 = {
    {0, 24, 3840, 2160},
    {3840, 2160},
    30,
    OT_ISP_BAYER_BGGR,
    OT_WDR_MODE_2To1_LINE,
    0,
    TD_FALSE,
    TD_FALSE,
    {
        TD_FALSE,
        {0, 0, 3840, 2160},
    },
};

namespace hisilicon{namespace dev{

    vi_os08a20_2to1wdr::vi_os08a20_2to1wdr()
        :vi_isp(3840,/*w*/
                2184,/*h*/
                30,/*src frame*/
                0,/*vi dev*/
                0,/*mpip dev*/
                0,/*sns clk src*/
                1,/*wdr mode*/
                &g_sns_os08a20_obj,/*sns obj*/
                4/*i2c dev*/)
    {
        memcpy(&m_mipi_attr,&g_mipi_4lane_chn0_sensor_os08a20_12bit_wdr2to1,sizeof(combo_dev_attr_t));
        memcpy(&m_isp_pub_attr,&g_isp_pub_attr_os08a20_mipi_8m_30fps_wdr2to1,sizeof(ot_isp_pub_attr));

        m_vi_pipe_attr.size.width = m_isp_pub_attr.sns_size.width;
        m_vi_pipe_attr.size.height = m_isp_pub_attr.sns_size.height;
        m_vi_chn_attr.size.width = m_isp_pub_attr.sns_size.width;
        m_vi_chn_attr.size.height = m_isp_pub_attr.sns_size.height;
        m_vpss_grp_attr.max_width = m_isp_pub_attr.sns_size.width;
        m_vpss_grp_attr.max_height = m_isp_pub_attr.sns_size.height;
        m_vpss_chn_attr.width = m_isp_pub_attr.sns_size.width;
        m_vpss_chn_attr.height = m_isp_pub_attr.sns_size.height;

        //use 2 pipes
        m_pipes.clear();
        m_pipes.push_back(0);
        m_pipes.push_back(1);
        for(int i = 0; i < m_pipes.size(); i++)
        {
            m_fusion_grp_attr.pipe_id[i] = m_pipes[i];
        }
        m_fusion_grp_attr.wdr_mode = OT_WDR_MODE_2To1_LINE;
    }

    vi_os08a20_2to1wdr::~vi_os08a20_2to1wdr()
    {
    }

}}//namespace

