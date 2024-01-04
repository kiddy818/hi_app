#ifndef aiisp_3dnr_include_h
#define aiisp_3dnr_include_h

#include "aiisp.h"
#include <ot_common_ai3dnr.h>

class aiisp_3dnr
    :public aiisp
{
    public:
        aiisp_3dnr(int pipe);
    
        ~aiisp_3dnr() override;
            
        static bool init(const char* model,int w,int h);
        static void release();

        bool start() override;
        void stop() override;
private:
        int m_pipe;

private:
        static ot_ai3dnr_model g_model_info;
        static td_s32 g_model_id;
        static ot_vb_pool g_aiisp_3dnr_pool;
};

#endif
