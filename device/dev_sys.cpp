#include "dev_sys.h"
#include "dev_log.h"

namespace hisilicon{namespace dev{

    bool sys::init()
    {
        ot_vb_cfg vb_cfg;
        td_s32 ret;

        ss_mpi_sys_exit();
        ss_mpi_vb_exit();

        memset(&vb_cfg,0,sizeof(ot_vb_cfg));
        vb_cfg.common_pool[0].blk_size = 3840 * 2160 * 2;
        vb_cfg.common_pool[0].blk_cnt = 10;
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
        for(int i = 0; i < OT_VI_MAX_PIPE_NUM; i++)
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

}}//namespace
