#ifndef dev_vi_isp_include_h
#define dev_vi_isp_include_h

#include "dev_vi.h"

//to support rgb raw sensor
namespace hisilicon{namespace dev{

    class vi_isp
        :public vi
    {
        public:
            vi_isp(int w,
                    int h,
                    int src_fr,
                    int vi_dev,
                    int mipi_dev,
                    int sns_clk_src,
                    int wdr_mode,
                    ot_isp_sns_obj* sns_obj,
                    int i2c_dev);

            virtual ~vi_isp();

            bool start() override;

            void stop() override;

            static bool init_hs_mode(lane_divide_mode_t mode);

        protected:
            bool start_mipi();
            bool stop_mipi();
            bool reset_sns();
            void on_isp_proc();

            bool start_isp();
            void stop_isp();
            std::thread m_isp_thread;

        protected:
            int m_mipi_dev;
            int m_sns_clk_src;
            int m_wdr_mode;
            ot_isp_sns_obj* m_sns_obj;
            int m_i2c_dev;

            combo_dev_attr_t m_mipi_attr;
            ot_vi_dev_attr m_vi_dev_attr;
            ot_vi_wdr_fusion_grp_attr m_fusion_grp_attr;
            ot_vi_pipe_attr m_vi_pipe_attr;
            ot_vi_chn_attr m_vi_chn_attr;
            ot_isp_pub_attr m_isp_pub_attr;
            ot_vpss_grp_attr m_vpss_grp_attr;
            ot_vpss_chn_attr m_vpss_chn_attr;
    };

}}//namespace

#endif
