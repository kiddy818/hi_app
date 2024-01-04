#include <util/std.h>
#include "aiisp_3dnr.h"
#include <ss_mpi_ai3dnr.h>

ot_ai3dnr_model aiisp_3dnr::g_model_info;
td_s32 aiisp_3dnr::g_model_id = -1;
ot_vb_pool aiisp_3dnr::g_aiisp_3dnr_pool = OT_VB_INVALID_POOL_ID;

aiisp_3dnr::aiisp_3dnr(int pipe)
    :m_pipe(pipe)
{
}

aiisp_3dnr::~aiisp_3dnr()
{
}

bool aiisp_3dnr::init(const char* model_file,int w,int h)
{
    td_s32 ret = 0;

    ret = ss_mpi_ai3dnr_init();
    if(ret != TD_SUCCESS)
    {
        printf("[%s]: ss_mpi_ai3dnr_init failed with error 0x%x\n",__FUNCTION__,ret);
        return false;
    }

    memset(&g_model_info,0,sizeof(g_model_info));

    if(!aiisp::read_model(model_file,&g_model_info.model.mem_info))
    {
        printf("[%s]: read_model failed\n",__FUNCTION__);
        return false;
    }

    g_model_info.model.preempted_en = TD_FALSE;
    g_model_info.model.image_size.width  = w;
    g_model_info.model.image_size.height = h;
    g_model_info.net_type = OT_AI3DNR_NET_TYPE_UV;
    ret = ss_mpi_ai3dnr_load_model(&g_model_info,&g_model_id);
    if(ret != TD_SUCCESS)
    {
        printf("[%s]: ss_mpi_ai3dnr_load model failed with error 0x%x\n",__FUNCTION__,ret);
        return false;
    }

    td_s32 blk_size;
    blk_size = ot_ai3dnr_get_pic_buf_size(w, h);
    g_aiisp_3dnr_pool = aiisp::create_pool(blk_size,7,OT_VB_REMAP_MODE_NONE);
    if(g_aiisp_3dnr_pool == OT_VB_INVALID_POOL_ID)
    {
        printf("[%s]: ss_mpi_vb_create_pool with error 0x%x\n",__FUNCTION__,ret);
        return false;
    }

    return true;
}

void aiisp_3dnr::release()
{
    if(g_model_id != -1)
    {
        ss_mpi_ai3dnr_unload_model(g_model_id);
    }

    ss_mpi_ai3dnr_exit();

    if(g_model_info.model.mem_info.phys_addr != 0)
    {
        ss_mpi_sys_mmz_free(g_model_info.model.mem_info.phys_addr, g_model_info.model.mem_info.virt_addr);
    }

    if (g_aiisp_3dnr_pool != OT_VB_INVALID_POOL_ID)
    {
        aiisp::destroy_pool(g_aiisp_3dnr_pool);
        g_aiisp_3dnr_pool = OT_VB_INVALID_POOL_ID;
    }
}

bool aiisp_3dnr::start()
{
    td_s32 ret;

    ot_aiisp_pool pool_attr;
    memset(&pool_attr,0,sizeof(pool_attr));
    pool_attr.aiisp_type = OT_AIISP_TYPE_AI3DNR;
    pool_attr.ai3dnr_pool.vb_pool = g_aiisp_3dnr_pool;
    pool_attr.ai3dnr_pool.net_type = OT_AI3DNR_NET_TYPE_UV;
    ret = ss_mpi_vi_attach_aiisp_vb_pool(m_pipe, &pool_attr);
    if(ret != TD_SUCCESS)
    {
        printf("[%s]: ss_mpi_vi_attach_aiisp_vb_pool failed with error 0x%x\n",__FUNCTION__,ret);
        return false;
    }

    ret = ss_mpi_ai3dnr_enable(m_pipe);
    if(ret != TD_SUCCESS)
    {
        printf("[%s]: ss_mpi_ai3dnr_start failed with error 0x%x\n",__FUNCTION__,ret);
        return false;
    }

    return true;
}

void aiisp_3dnr::stop()
{
    td_s32 ret;

    ret = ss_mpi_ai3dnr_disable(m_pipe);
    if(ret != TD_SUCCESS)
    {
        printf("[%s]: ss_mpi_ai3dnr_stop failed with error 0x%x\n",__FUNCTION__,ret);
        return ;
    }

    ss_mpi_vi_detach_aiisp_vb_pool(m_pipe, OT_AIISP_TYPE_AI3DNR);
    return ;
}


