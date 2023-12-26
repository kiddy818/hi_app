#include "aiisp.h"

bool aiisp::read_model(const char* model_file,ot_aiisp_mem_info* mem)
{
    td_s32 ret;
    FILE* fp = TD_NULL;

    fp = fopen(model_file, "rb");
    if(fp == NULL)
    {
        printf("[%s]: fopen failed\n",__FUNCTION__);
        return false;
    }

    ret = fseek(fp, 0L, SEEK_END);
    if (ret != TD_SUCCESS) 
    {
        printf("[%s]: fseek failed with error 0x%x\n",__FUNCTION__,ret);
        goto fail_0;
    }

    mem->size = ftell(fp);
    if (mem->size <= 0) 
    {
        printf("[%s]: ftell failed\n",__FUNCTION__);
        goto fail_0;
    }

    ret = fseek(fp, 0L, SEEK_SET);
    if (ret != TD_SUCCESS) 
    {
        printf("[%s]: fseek failed\n",__FUNCTION__);
        goto fail_0;
    }

    /* malloc model file mem */
    ret = ss_mpi_sys_mmz_alloc(&(mem->phys_addr), &(mem->virt_addr), "aibnr_cfg", TD_NULL, mem->size);
    if (ret != TD_SUCCESS)
    {
        printf("[%s]: ss_mpi_sys_mmz_alloc failed with error 0x%x\n",__FUNCTION__,ret);
        goto fail_0;
    }

    ret = fread(mem->virt_addr, mem->size, 1, fp);
    if (ret != 1) 
    {
        printf("[%s]: fread failed with error 0x%x\n",__FUNCTION__,ret);
        goto fail_1;
    }

    ret = fclose(fp);
    if (ret != 0) 
    {
        printf("[%s]: fclose failed with error 0x%x\n",__FUNCTION__,ret);
    }
    return true;;

fail_1:
    ss_mpi_sys_mmz_free(mem->phys_addr, mem->virt_addr);
    mem->phys_addr = 0;
    mem->virt_addr = TD_NULL;
fail_0:
    if (fp != TD_NULL) 
    {
        fclose(fp);
    }
    return false;
}

