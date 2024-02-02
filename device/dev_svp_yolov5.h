#ifndef dev_yolov5_include_h
#define dev_yolov5_include_h

#include "dev_svp.h"
#include "dev_std.h"
#include "dev_vi_isp.h"

namespace hisilicon{namespace dev{

    class yolov5 
    {
        public:
            yolov5(std::shared_ptr<vi> vi_ptr,const char* model_path);
            ~yolov5();

            bool start();
            void stop();

        private:
            bool create_vpss_grp_chn();
            void destroy_vpss_grp_chn();

        private:
            bool m_is_start;
            std::string m_model_path;
            td_ulong m_model_mem_size;
            td_void *m_model_mem_ptr;
            td_u32 m_model_id;
            svp_acl_mdl_desc *m_model_desc;
            size_t m_input_num;
            size_t m_output_num;
            size_t m_dynamic_batch_idx;
            ot_size m_pic_size;
            std::shared_ptr<vi> m_vi_ptr;
            ot_vpss_grp_attr m_vpss_grp_attr;
            ot_vpss_chn_attr m_vpss_chn_attr;
            ot_vpss_grp m_vpss_grp;
            ot_vpss_chn m_vpss_chn;
    };

}}//namespace

#endif

