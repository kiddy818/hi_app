#include <util/std.h>
#include "aiisp_drc.h"
#include <ss_mpi_aidrc.h>

ot_aidrc_model aiisp_drc::g_model_info;
td_s32 aiisp_drc::g_model_id = -1;
ot_vb_pool aiisp_drc::g_aiisp_drc_in_pool = OT_VB_INVALID_POOL_ID;
ot_vb_pool aiisp_drc::g_aiisp_drc_out_pool = OT_VB_INVALID_POOL_ID;

aiisp_drc::aiisp_drc(int pipe)
    :m_pipe(pipe)
{
}

aiisp_drc::~aiisp_drc()
{
}

bool aiisp_drc::init(const char* model_file,int w,int h,int is_wdr_mode)
{
    td_s32 ret = 0;

    ret = ss_mpi_aidrc_init();
    if(ret != TD_SUCCESS)
    {
        printf("[%s]: ss_mpi_aidrc_init failed with error 0x%x\n",__FUNCTION__,ret);
        return false;
    }

    memset(&g_model_info,0,sizeof(g_model_info));

    if(!aiisp::read_model(model_file,&g_model_info.model.mem_info))
    {
        printf("[%s]: read_model failed\n",__FUNCTION__);
        return false;
    }

    g_model_info.model.preempted_en = TD_FALSE;
    g_model_info.model.image_size.width = w;
    g_model_info.model.image_size.height = h;
    g_model_info.mode = OT_AIDRC_MODE_NORMAL;

    ret = ss_mpi_aidrc_load_model(&g_model_info,&g_model_id);
    if(ret != TD_SUCCESS)
    {
        printf("[%s]: ss_mpi_aidrc_load model failed with error 0x%x\n",__FUNCTION__,ret);
        return false;
    }

    td_s32 blk_size;
    blk_size = ot_aidrc_get_in_buf_size(w, h);
    g_aiisp_drc_in_pool = aiisp::create_pool(blk_size,3,OT_VB_REMAP_MODE_NONE);
    if(g_aiisp_drc_in_pool == OT_VB_INVALID_POOL_ID)
    {
        printf("[%s]: ss_mpi_vb_create_pool with error 0x%x\n",__FUNCTION__,ret);
        return false;
    }

    blk_size = ot_aidrc_get_out_buf_size(w,h,(g_model_info.mode == OT_AIDRC_MODE_ADVANCED) ? TD_TRUE : TD_FALSE);
    g_aiisp_drc_out_pool = aiisp::create_pool(blk_size,3,OT_VB_REMAP_MODE_NONE);
    if(g_aiisp_drc_out_pool == OT_VB_INVALID_POOL_ID)
    {
        printf("[%s]: ss_mpi_vb_create_pool with error 0x%x\n",__FUNCTION__,ret);
        return false;
    }

    return true;
}

void aiisp_drc::release()
{
    if(g_model_id != -1)
    {
        ss_mpi_aidrc_unload_model(g_model_id);
    }

    ss_mpi_aidrc_exit();

    if(g_model_info.model.mem_info.phys_addr != 0)
    {
        ss_mpi_sys_mmz_free(g_model_info.model.mem_info.phys_addr, g_model_info.model.mem_info.virt_addr);
    }

    if (g_aiisp_drc_in_pool != OT_VB_INVALID_POOL_ID)
    {
        aiisp::destroy_pool(g_aiisp_drc_in_pool);
        g_aiisp_drc_in_pool = OT_VB_INVALID_POOL_ID;
    }

    if (g_aiisp_drc_out_pool != OT_VB_INVALID_POOL_ID)
    {
        aiisp::destroy_pool(g_aiisp_drc_out_pool);
        g_aiisp_drc_out_pool = OT_VB_INVALID_POOL_ID;
    }
}

bool aiisp_drc::start()
{
    td_s32 ret;
    
    ot_aiisp_pool pool_attr;
    memset(&pool_attr,0,sizeof(pool_attr));
    pool_attr.aiisp_type = OT_AIISP_TYPE_AIDRC;
    pool_attr.aidrc_pool.in_vb_pool = g_aiisp_drc_in_pool;
    pool_attr.aidrc_pool.out_vb_pool = g_aiisp_drc_out_pool;
    ret = ss_mpi_vi_attach_aiisp_vb_pool(m_pipe, &pool_attr);
    if(ret != TD_SUCCESS)
    {
        printf("[%s]: ss_mpi_vi_attach_aiisp_vb_pool failed with error 0x%x\n",__FUNCTION__,ret);
        return false;
    }

    ret = ss_mpi_aidrc_enable(m_pipe);
    if(ret != TD_SUCCESS)
    {
        printf("[%s]: ss_mpi_aidrc_start failed with error 0x%x\n",__FUNCTION__,ret);
        return false;
    }

    ot_aidrc_attr aidrc_attr;
    ret = ss_mpi_aidrc_get_attr(m_pipe, &aidrc_attr);
    if(ret != TD_SUCCESS)
    {
        printf("[%s]: ss_mpi_aidrc_get_attr failed with error 0x%x\n",__FUNCTION__,ret);
        return false;
    }

    aidrc_attr.enable = TD_TRUE;
    aidrc_attr.param.strength = 7;
    aidrc_attr.param.threshold = 1;
    ret = ss_mpi_aidrc_set_attr(m_pipe, &aidrc_attr);
    if(ret != TD_SUCCESS)
    {
        printf("[%s]: ss_mpi_aidrc_set_attr failed with error 0x%x\n",__FUNCTION__,ret);
        return false;
    }

    return true;
}

void aiisp_drc::stop()
{
    td_s32 ret;

    ret = ss_mpi_aidrc_disable(m_pipe);
    if(ret != TD_SUCCESS)
    {
        printf("[%s]: ss_mpi_aidrc_stop failed with error 0x%x\n",__FUNCTION__,ret);
        return ;
    }

    ss_mpi_vi_detach_aiisp_vb_pool(m_pipe, OT_AIISP_TYPE_AIBNR);
    return ;
}

