#include "dev_vi.h"
#include "dev_log.h"

namespace hisilicon{namespace dev{

    vi::vi(int w,int h,int src_fr,int vi_dev)
        :m_w(w),m_h(h),m_src_fr(src_fr),m_is_start(false),m_vi_dev(vi_dev),m_vpss_grp(0),m_vpss_chn(0),m_vi_chn(0)
    {
        m_pipes.clear();
        m_pipes.push_back(0);
    }

    vi::~vi()
    {
    }

    int vi::w()
    {
        return m_w;
    }

    int vi::h()
    {
        return m_h;
    }

    int vi::fr()
    {
        return m_src_fr;
    }

    int vi::vi_dev()
    {
        return m_vi_dev;
    }

    int vi::vpss_grp()
    {
        return m_vpss_grp;
    }

    int vi::vpss_chn()
    {
        return m_vpss_chn;
    }

    int vi::vi_chn()
    {
        return m_vi_chn;
    }

    bool vi::init()
    {
        return true;
    }

    void vi::release()
    {
    }

    std::vector<ot_vi_pipe> vi::pipes()
    {
        return m_pipes;
    }

}}//namespace

