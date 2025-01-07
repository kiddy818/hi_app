#include "dev_sys.h"
#include "dev_log.h"
#include <mutex>

namespace hisilicon{namespace dev{

    bool sys::init(ot_vi_vpss_mode_type mode)
    {
        ot_vb_cfg vb_cfg;
        td_s32 ret;

        ss_mpi_sys_exit();
        ss_mpi_vb_exit();

        memset(&vb_cfg,0,sizeof(ot_vb_cfg));
        vb_cfg.common_pool[0].blk_size = 3840 * 2300 * 3 / 2;/*for 3840x2184*/
        vb_cfg.common_pool[0].blk_cnt = 20;
        vb_cfg.common_pool[1].blk_size = 704 * 576 * 3 / 2;
        vb_cfg.common_pool[1].blk_cnt = 10;
        vb_cfg.common_pool[2].blk_size = 640 * 640 * 3 / 2;
        vb_cfg.common_pool[2].blk_cnt = 10;
        vb_cfg.max_pool_cnt= 3;
        ret = ss_mpi_vb_set_cfg(&vb_cfg);
        if(ret != TD_SUCCESS)
        {
            DEV_WRITE_LOG_ERROR("ss_mpi_vb_set_cfg failed with error 0x%x",ret);
            return false;
        }

        ot_vb_supplement_cfg vb_supplement_cfg = {0};
        vb_supplement_cfg.supplement_cfg = OT_VB_SUPPLEMENT_BNR_MOT_MASK | OT_VB_SUPPLEMENT_JPEG_MASK;
        ret = ss_mpi_vb_set_supplement_cfg(&vb_supplement_cfg);
        if (ret != TD_SUCCESS)
        {
            DEV_WRITE_LOG_ERROR("ss_mpi_vb_set_supplement failed with error 0x%x",ret);
            return false;
        }

        ret = ss_mpi_vb_init();
        if (ret != TD_SUCCESS)
        {
            DEV_WRITE_LOG_ERROR("ss_mpi_vb_init failed with error 0x%x",ret);
            return false;
        }

        ret = ss_mpi_sys_init();
        if (ret != TD_SUCCESS)
        {
            DEV_WRITE_LOG_ERROR("ss_mpi_sys_init failed with error 0x%x",ret);
            return false;
        }

        ot_vi_vpss_mode vi_vpss_mode;
        memset(&vi_vpss_mode,0,sizeof(vi_vpss_mode));
        vi_vpss_mode.mode[0] = mode;
        for(int i = 1; i < OT_VI_MAX_PIPE_NUM; i++)
        {
            vi_vpss_mode.mode[i] = OT_VI_OFFLINE_VPSS_OFFLINE;
        }
        ret = ss_mpi_sys_set_vi_vpss_mode(&vi_vpss_mode);
        if(ret != TD_SUCCESS)
        {
            DEV_WRITE_LOG_ERROR("ss_mpi_sys_set_vi_vpss_mode failed with error 0x%x",ret);
            return false;
        }

        ret = ss_mpi_sys_set_vi_aiisp_mode(0, OT_VI_AIISP_MODE_DEFAULT);
        if(ret != TD_SUCCESS)
        {
            DEV_WRITE_LOG_ERROR("ss_mpi_sys_set_vi_aiisp_mode failed with error 0x%x",ret);
            return false;
        }

        ret = ss_mpi_sys_set_3dnr_pos(OT_3DNR_POS_VI);
        if(ret != TD_SUCCESS)
       {
            DEV_WRITE_LOG_ERROR("ss_mpi_sys_set_3dnr_pos failed with error 0x%x",ret);
            return false;
        }

        return true;
    }

    void sys::release()
    {
        ss_mpi_sys_exit();
        ss_mpi_vb_exit_mod_common_pool(OT_VB_UID_VDEC);
        ss_mpi_vb_exit();
    }

    ot_venc_chn sys::alloc_venc_chn()
    {
        static std::mutex g_venc_chn_mu;
        static ot_venc_chn g_free_venc_chn = 0;

        std::unique_lock<std::mutex> lock(g_venc_chn_mu);
        return g_free_venc_chn++;
    }

    static std::mutex g_rgn_mu;
    static bool g_rgn_inited = false;
    static bool g_rgn_flag[OT_RGN_HANDLE_MAX];
    void sys::free_rgn_handle(ot_rgn_handle hdl)
    {
        std::unique_lock<std::mutex> lock(g_rgn_mu);

        if(hdl >=0 && hdl < OT_RGN_HANDLE_MAX)
        {
            g_rgn_flag[hdl] = false;
        }
    }

    ot_rgn_handle sys::alloc_rgn_handle()
    {
        std::unique_lock<std::mutex> lock(g_rgn_mu);
        if(!g_rgn_inited)
        {
            g_rgn_inited = true;
            for(int i = 0; i < OT_RGN_HANDLE_MAX; i++)
            {
                g_rgn_flag[i] = false;
            }
        }

        for(int i = 0; i < OT_RGN_HANDLE_MAX; i++)
        {
            if(!g_rgn_flag[i])
            {
                g_rgn_flag[i] = true;
                return (ot_rgn_handle)i;
            }
        }

        return OT_INVALID_HANDLE;
    }
}}//namespace
