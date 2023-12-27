#include "aiisp.h"
#include <ot_common_aibnr.h>

class aiisp_bnr
    :public aiisp
{
public:
    aiisp_bnr(int pipe);

    ~aiisp_bnr() override;

    static bool init(const char* model,int w,int h,int is_wdr_mode);

    static void release();

    bool start() override;

    void stop() override;

private:
    int m_pipe;

private:
    static ot_aibnr_model g_model_info;
    static td_s32 g_model_id;
    static ot_vb_pool g_aiisp_pool;
};
