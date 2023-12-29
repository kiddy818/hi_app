#ifndef dev_vi_include_h
#define dev_vi_include_h

#include "dev_std.h"

#define MIPI_DEV_NAME "/dev/ot_mipi_rx"
namespace hisilicon{namespace dev{

    class vi
    {
        public:
            vi(int w,int h,int src_fr,int mipi_dev,int sns_clk_src,int vi_dev);
            virtual ~vi();

            virtual bool start() = 0;
            virtual void stop() = 0;

        protected:
            bool start_mipi(combo_dev_attr_t* pdev_attr);
            bool stop_mipi();
            bool reset_sns();

        protected:
            int m_w;
            int m_h;
            int m_src_fr;
            bool m_is_start;
            int m_mipi_dev;
            int m_sns_clk_src;
            int m_vi_dev;
    };

}}//namespace

#endif
