#ifndef aiisp_drc_include_h
#define aiisp_drc_include_h

#include "aiisp.h"
#include <ot_common_aidrc.h>

class aiisp_drc
    :public aiisp
{
public:
    aiisp_drc(int pipe);

    ~aiisp_drc() override;

    static bool init(const char* model,int w,int h,int is_wdr_mode);

    static void release();

    bool start() override;

    void stop() override;

private:
    int m_pipe;

private:
    static ot_aidrc_model g_model_info;
    static td_s32 g_model_id;
    static ot_vb_pool g_aiisp_drc_in_pool;
    static ot_vb_pool g_aiisp_drc_out_pool;
};

#endif
