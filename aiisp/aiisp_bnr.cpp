#include <util/std.h>
#include "aiisp_bnr.h"
#include <ss_mpi_aibnr.h>

ot_aibnr_model aiisp_bnr::g_model_info;
td_s32 aiisp_bnr::g_model_id = -1;

aiisp_bnr::aiisp_bnr(int pipe)
    :m_pipe(pipe)
{
}

aiisp_bnr::~aiisp_bnr()
{
}

bool aiisp_bnr::init(const char* model_file,int w,int h,int is_wdr_mode)
{
    td_s32 ret = 0;

    ret = ss_mpi_aibnr_init();
    if(ret != TD_SUCCESS)
    {
        printf("[%s]: ss_mpi_aibnr_init failed with error 0x%x\n",__FUNCTION__,ret);
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
    g_model_info.is_wdr_mode = is_wdr_mode ? TD_TRUE : TD_FALSE; 
    g_model_info.ref_mode = OT_AIBNR_REF_MODE_NORM; 

    ret = ss_mpi_aibnr_load_model(&g_model_info,&g_model_id);
    if(ret != TD_SUCCESS)
    {
        printf("[%s]: ss_mpi_aibnr_load model failed with error 0x%x\n",__FUNCTION__,ret);
        return false;
    }

    return true;
}

void aiisp_bnr::release()
{
    if(g_model_id != -1)
    {
        ss_mpi_aibnr_unload_model(g_model_id);
    }

    ss_mpi_aibnr_exit();

    if(g_model_info.model.mem_info.phys_addr != 0)
    {
        ss_mpi_sys_mmz_free(g_model_info.model.mem_info.phys_addr, g_model_info.model.mem_info.virt_addr);
    }
}

bool aiisp_bnr::start()
{
    td_s32 ret;

    ret = ss_mpi_aibnr_enable(m_pipe);
    if(ret != TD_SUCCESS)
    {
        printf("[%s]: ss_mpi_aibnr_start failed with error 0x%x\n",__FUNCTION__,ret);
        return false;
    }

    ot_aibnr_attr aibnr_attr;
    ret = ss_mpi_aibnr_get_attr(m_pipe, &aibnr_attr);
    if(ret != TD_SUCCESS)
    {
        printf("[%s]: ss_mpi_aibnr_get_attr failed with error 0x%x\n",__FUNCTION__,ret);
        return false;
    }

    aibnr_attr.enable = TD_TRUE;
    aibnr_attr.bnr_bypass = TD_FALSE;
    aibnr_attr.blend = TD_FALSE;
    aibnr_attr.op_type = OT_OP_MODE_MANUAL;
    aibnr_attr.manual_attr.sfs = 31; /* sfs: 31 */
    aibnr_attr.manual_attr.tfs = 31; /* tfs: 31 */
    ret = ss_mpi_aibnr_set_attr(m_pipe, &aibnr_attr);
    if(ret != TD_SUCCESS)
    {
        printf("[%s]: ss_mpi_aibnr_set_attr failed with error 0x%x\n",__FUNCTION__,ret);
        return false;
    }

    return true;
}

void aiisp_bnr::stop()
{
    td_s32 ret;

    ret = ss_mpi_aibnr_disable(m_pipe);
    if(ret != TD_SUCCESS)
    {
        printf("[%s]: ss_mpi_aibnr_stop failed with error 0x%x\n",__FUNCTION__,ret);
        return ;
    }

    return ;
}


