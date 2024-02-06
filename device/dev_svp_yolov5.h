#ifndef dev_yolov5_include_h
#define dev_yolov5_include_h

#include "dev_svp.h"
#include "dev_std.h"
#include "dev_vi_isp.h"
#include <stream_observer.h>

namespace hisilicon{namespace dev{

    typedef struct
    {
        svp_acl_mdl_dataset *input_dataset;
        svp_acl_mdl_dataset *output_dataset;
        td_void *task_buf_ptr;
        size_t task_buf_size;
        size_t task_buf_stride;
        td_void *work_buf_ptr;
        size_t work_buf_size;
        size_t work_buf_stride;
    }svp_npu_task_info_t;

#define SVP_RECT_POINT_NUM 4
    typedef struct
    {
        ot_point point[SVP_RECT_POINT_NUM];
    }svp_npu_rect_t;

#define SVP_RECT_NUM 64
    typedef struct
    {
        td_u16 num;
        svp_npu_rect_t rect[SVP_RECT_NUM];
    }svp_npu_rect_info_t;

    class yolov5 
        :public ceanic::rtsp::stream_post
    {
        public:
            yolov5(std::shared_ptr<vi> vi_ptr,const char* model_path,ot_venc_chn venc_chn);
            ~yolov5();

            bool start();
            void stop();

            int venc_w();
            int venc_h();

        private:
            bool create_vpss_grp_chn();
            void destroy_vpss_grp_chn();
            bool create_venc_chn();
            void destroy_venc_chn();
            void process_video_stream(ot_venc_stream* pstream);

            bool create_svp_input();
            svp_acl_data_buffer* create_svp_input_buffer(int index);
            void destroy_svp_input();
            void destroy_svp_input_buffer(int index);

            bool create_svp_output();
            svp_acl_data_buffer* create_svp_output_buffer(int index);
            void destroy_svp_output();
            void destroy_svp_output_buffer(int index);

            bool set_svp_threshold();
            bool create_vb_pool();
            void destroy_vb_pool();

            bool get_svp_roi_num(td_u16* pnum);
            bool get_svp_rio(svp_npu_rect_info_t* rect_info);

            void on_process();
            void on_venc_process();

            void svp_vgs_fill_rect(const ot_video_frame_info *frame_info,svp_npu_rect_info_t* rect,td_u32 color);

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
            ot_vpss_chn_attr m_vpss_chn_attr;
            ot_vpss_grp m_vpss_grp;
            ot_vpss_chn m_vpss_chn;

            svp_npu_task_info_t m_task_info;
            ot_vb_pool m_vb_poolid;
            ot_vb_pool_info m_vb_pool_info;

            std::thread m_thread;
            ot_venc_chn m_venc_chn;
            ot_venc_chn_attr m_venc_chn_attr;

            std::thread m_venc_thread;
    };

}}//namespace

#endif

