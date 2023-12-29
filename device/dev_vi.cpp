#include "dev_vi.h"
#include "dev_log.h"

namespace hisilicon{namespace dev{

    vi::vi(int w,int h,int src_fr,int mipi_dev,int sns_clk_src,int vi_dev)
        :m_w(w),m_h(h),m_src_fr(src_fr),m_is_start(false),m_mipi_dev(mipi_dev),m_sns_clk_src(sns_clk_src),m_vi_dev(vi_dev)
    {
    }

    vi::~vi()
    {
    }

    bool vi::start_mipi(combo_dev_attr_t* pdev_attr)
    {
        td_s32 fd = open(MIPI_DEV_NAME, O_RDWR);

        if(fd < 0)
        {
            DEV_WRITE_LOG_ERROR("open mipi file:%s failed!",MIPI_DEV_NAME);
            return false;
        }

        ioctl(fd, OT_MIPI_ENABLE_MIPI_CLOCK, &m_mipi_dev);
        ioctl(fd, OT_MIPI_RESET_MIPI, &m_mipi_dev);
        ioctl(fd, OT_MIPI_SET_DEV_ATTR, pdev_attr);
        ioctl(fd, OT_MIPI_UNRESET_MIPI, &m_mipi_dev);

        close(fd);
        return true;
    }

    bool vi::stop_mipi()
    {
        td_s32 fd = open(MIPI_DEV_NAME, O_RDWR);
        if (fd < 0)
        {
            DEV_WRITE_LOG_ERROR("open mipi file:%s failed!",MIPI_DEV_NAME);
            return false;
        }

        ioctl(fd, OT_MIPI_RESET_MIPI, &m_mipi_dev);
        ioctl(fd, OT_MIPI_DISABLE_MIPI_CLOCK, &m_mipi_dev);

        close(fd);
        return true;
    }

    bool vi::reset_sns()
    {
        td_s32 fd = open(MIPI_DEV_NAME, O_RDWR);
        if (fd < 0)
        {
            DEV_WRITE_LOG_ERROR("open mipi file:%s failed!",MIPI_DEV_NAME);
            return false;
        }

        ioctl(fd, OT_MIPI_ENABLE_SENSOR_CLOCK, &m_sns_clk_src);
        ioctl(fd, OT_MIPI_RESET_SENSOR, &m_sns_clk_src);
        ioctl(fd, OT_MIPI_UNRESET_SENSOR, &m_sns_clk_src);

        close(fd);
        return true;
    }

}}//namespace

