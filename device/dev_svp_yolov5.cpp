#include "dev_sys.h"
#include "dev_svp_yolov5.h"
#include "dev_log.h"
#include "ceanic_freetype.h"
#include <util/check_interval.h>

namespace hisilicon{namespace dev{

#define SAMPLE_SVP_NPU_EXTRA_INPUT_NUM   2
#define SAMPLE_SVP_NPU_H_DIM_IDX 2
#define SAMPLE_SVP_NPU_MAX_MEM_SIZE      0xFFFFFFFF
#define SAMPLE_SVP_NPU_RECT_LEFT_TOP     0
#define SAMPLE_SVP_NPU_RECT_RIGHT_TOP    1
#define SAMPLE_SVP_NPU_RECT_RIGHT_BOTTOM 2
#define SAMPLE_SVP_NPU_RECT_LEFT_BOTTOM  3
#define SAMPLE_SVP_OSD_FONT_SIZE      16
#define SAMPLE_SVP_OSD_BG_COLOR rgb24to1555(0,255,0,0)
#define SAMPLE_SVP_OSD_FG_COLOR rgb24to1555(255,255,255,1)
#define SAMPLE_SVP_OSD_OUTLINE_COLOR rgb24to1555(0,255,0,0)

static std::vector<std::string> g_yolov5_class_str = {"person", "bicycle", "car", "motorcycle", "airplane", "bus", "train", "truck", "boat", "traffic light", "fire hydrant", "stop sign", "parking meter", "bench", "bird", "cat", "dog", "horse", "sheep", "cow", "elephant", "bear", "zebra", "giraffe", "backpack", "umbrella", "handbag", "tie", "suitcase", "frisbee", "skis", "snowboard", "sports ball", "kite", "baseball bat", "baseball glove", "skateboard", "surfboard", "tennis racket", "bottle", "wine glass", "cup", "fork", "knife", "spoon", "bowl", "banana", "apple", "sandwich", "orange", "broccoli", "carrot", "hot dog", "pizza", "donut", "cake", "chair", "couch", "potted plant", "bed", "dining table", "toilet", "tv", "laptop", "mouse", "remote", "keyboard", "cell phone", "microwave", "oven", "toaster", "sink", "refrigerator", "book", "clock", "vase", "scissors", "teddy bear", "hair drier", "toothbrush"};

    yolov5::yolov5(int32_t chn,int32_t stream,std::shared_ptr<vi> vi_ptr,const char* model_path)
        :stream_obj("yolov5_stream",chn,stream),m_is_start(false),m_model_path(model_path),m_vi_ptr(vi_ptr),m_vb_poolid(OT_VB_INVALID_POOL_ID)
         ,m_vpss_grp(0),m_vpss_chn(0),m_model_mem_size(0),m_model_mem_ptr(NULL),m_model_id(0),m_model_desc(NULL)
         ,m_input_num(0),m_output_num(0),m_dynamic_batch_idx(0)
    {
        //init vpss chn attr
        memset(&m_vpss_chn_attr,0,sizeof(m_vpss_chn_attr));
        m_vpss_chn_attr.mirror_en                 = TD_FALSE;
        m_vpss_chn_attr.flip_en                   = TD_FALSE;
        m_vpss_chn_attr.border_en                 = TD_FALSE;
        m_vpss_chn_attr.width                     = 0;
        m_vpss_chn_attr.height                    = 0;
        m_vpss_chn_attr.depth                     = 1;
        m_vpss_chn_attr.chn_mode                  = OT_VPSS_CHN_MODE_USER;
        m_vpss_chn_attr.video_format              = OT_VIDEO_FORMAT_LINEAR;
        m_vpss_chn_attr.dynamic_range             = OT_DYNAMIC_RANGE_SDR8;
        m_vpss_chn_attr.pixel_format              = OT_PIXEL_FORMAT_YVU_SEMIPLANAR_420;
        m_vpss_chn_attr.compress_mode             = OT_COMPRESS_MODE_NONE;
        m_vpss_chn_attr.aspect_ratio.mode         = OT_ASPECT_RATIO_NONE;
        //m_vpss_chn_attr.aspect_ratio.mode         = OT_ASPECT_RATIO_AUTO;

        m_vpss_chn_attr.frame_rate.src_frame_rate = -1;
        m_vpss_chn_attr.frame_rate.dst_frame_rate = -1;

        m_venc_chn = sys::alloc_venc_chn();

        //init venc chn attr
        memset(&m_venc_chn_attr,0,sizeof(m_venc_chn_attr));
        m_venc_chn_attr.venc_attr.type = OT_PT_H264;
        //m_venc_chn_attr.venc_attr.max_pic_width = m_venc_w;
        //m_venc_chn_attr.venc_attr.max_pic_height = m_venc_h;
        //m_venc_chn_attr.venc_attr.pic_width = m_venc_w;/*the picture width*/
        //m_venc_chn_attr.venc_attr.pic_height    = m_venc_h;/*the picture height*/
        //m_venc_chn_attr.venc_attr.buf_size      = m_venc_w * m_venc_h  *3 / 2;/*stream buffer size*/
        m_venc_chn_attr.venc_attr.is_by_frame      = TD_TRUE;/*get stream mode is slice mode or frame mode?*/
        m_venc_chn_attr.venc_attr.profile = 0;
        m_venc_chn_attr.venc_attr.h264_attr.rcn_ref_share_buf_en = TD_FALSE;
        m_venc_chn_attr.venc_attr.h264_attr.frame_buf_ratio = 70;
        m_venc_chn_attr.gop_attr.gop_mode = OT_VENC_GOP_MODE_NORMAL_P;
        m_venc_chn_attr.gop_attr.normal_p.ip_qp_delta = 2; /* 2 is a number */
        m_venc_chn_attr.rc_attr.rc_mode = OT_VENC_RC_MODE_H264_CBR;
        m_venc_chn_attr.rc_attr.h264_cbr.gop = 25; /*the interval of IFrame*/
        m_venc_chn_attr.rc_attr.h264_cbr.stats_time = 1; /* stream rate statics time(s) */
        m_venc_chn_attr.rc_attr.h264_cbr.src_frame_rate= 1; /* input (vi) frame rate */
        m_venc_chn_attr.rc_attr.h264_cbr.dst_frame_rate = 1; /* target frame rate */
        m_venc_chn_attr.rc_attr.h264_cbr.bit_rate = 1000;

        m_pic_size.width = 640;
        m_pic_size.height = 640;

        for(int i = 0; i < SVP_RECT_NUM; i++)
        {
            m_rgn[i] =  OT_INVALID_HANDLE;
        }

        m_max_rgn_w = 0;
        char str[255];
        int rgn_w;
        for(uint32_t i = 0; i < g_yolov5_class_str.size(); i++)
        {
            sprintf(str,"%s,0.99",g_yolov5_class_str[i].c_str());
            g_freetype.get_width(str,SAMPLE_SVP_OSD_FONT_SIZE,&rgn_w);
            if(rgn_w > m_max_rgn_w)
            {
                m_max_rgn_w = rgn_w;
            }
        }
    }

    yolov5::~yolov5()
    {
        stop();
    }

    void yolov5::process_video_stream(ot_venc_stream* pstream)
    {
        char* es_buf = NULL;
        int es_len = 0;
        int es_type = 0;
        int nalu_cnt = 0;
        unsigned long long time_stamp = 0;

        ceanic::util::stream_head sh;

        memset(&sh,0,sizeof(sh));
        sh.type = STREAM_NALU_SLICE;    

        for(unsigned int i = 0; i < pstream->pack_cnt; i++)
        {
            es_buf = (char*)(pstream->pack[i].addr + pstream->pack[i].offset);
            es_len = pstream->pack[i].len - pstream->pack[i].offset;
            es_type = es_buf[4] & 0x1f;
            time_stamp = pstream->pack[i].pts / 1000;

            if(es_type == 0x7 /*sps*/
                    || es_type == 0x8 /*pps*/
                    || es_type == 0x1 /*p*/
                    || es_type == 0x5 /*i*/)
            {
                sh.nalu[nalu_cnt].data = (uint8_t*)es_buf;
                sh.nalu[nalu_cnt].size = es_len; 
                sh.nalu[nalu_cnt].time_stamp = time_stamp;

                nalu_cnt++;
            }
            sh.nalu_count = nalu_cnt;
        }
        post_stream_to_observer(shared_from_this(),&sh,NULL,0);
    }

    void yolov5::destroy_vb_pool()
    {
        if(m_vb_poolid != OT_VB_INVALID_POOL_ID)
        {
            ss_mpi_vb_destroy_pool(m_vb_poolid);
            m_vb_poolid = OT_VB_INVALID_POOL_ID;
        }
    }

    bool yolov5::create_vb_pool()
    {
        assert(m_pic_size.width > 0 && m_pic_size.height > 0);
        ot_vb_pool_cfg vb_pool_cfg = { 0 };
        ot_pic_buf_attr pic_buf_attr = {0};
        td_u64 blk_size;

        pic_buf_attr.width = m_pic_size.width;
        pic_buf_attr.height = m_pic_size.height;
        pic_buf_attr.compress_mode = OT_COMPRESS_MODE_NONE;
        pic_buf_attr.align = OT_DEFAULT_ALIGN;
        pic_buf_attr.bit_width = OT_DATA_BIT_WIDTH_8;
        pic_buf_attr.pixel_format = OT_PIXEL_FORMAT_YVU_SEMIPLANAR_422;
        pic_buf_attr.video_format = OT_VIDEO_FORMAT_LINEAR;
        blk_size = ot_common_get_pic_buf_size(&pic_buf_attr);
        vb_pool_cfg.blk_size = blk_size;
        vb_pool_cfg.blk_cnt = 16;
        m_vb_poolid = ss_mpi_vb_create_pool(&vb_pool_cfg);
        if(m_vb_poolid == OT_VB_INVALID_POOL_ID)
        {
            DEV_WRITE_LOG_ERROR("ss_mpi_vb_create_pool failed");
            return false;
        }

        ss_mpi_vb_pool_share_all(m_vb_poolid);
        ss_mpi_vb_get_pool_info(m_vb_poolid, &m_vb_pool_info);
        return true;
    }

    bool yolov5::create_svp_output()
    {
        td_s32 ret;
        svp_acl_data_buffer *output_data = TD_NULL;

        svp_acl_mdl_dataset* output_dataset = svp_acl_mdl_create_dataset();
        if(output_dataset == NULL)
        {
            DEV_WRITE_LOG_ERROR("svp_acl_mdl_create_dataset failed ");
            return false;
        }

        for (size_t i = 0; i < m_output_num; i++)
        {
            output_data = create_svp_output_buffer(i);
            if(output_data == NULL)
            {
                svp_acl_mdl_destroy_dataset(output_dataset);
                return false;
            }

            ret = svp_acl_mdl_add_dataset_buffer(output_dataset, output_data);
            if(ret != SVP_ACL_SUCCESS)
            {
                DEV_WRITE_LOG_ERROR("svp_acl_mdl_add_dataset_buffer failed with 0x%x",ret);
                svp_acl_mdl_destroy_dataset(output_dataset);
                return false;
            }
        }

        m_task_info.output_dataset = output_dataset;
        return true;
    }

    void yolov5::destroy_svp_output_buffer(int index)
    {
        td_void *data = TD_NULL;
        svp_acl_data_buffer *data_buffer = TD_NULL;

        data_buffer = svp_acl_mdl_get_dataset_buffer(m_task_info.output_dataset,index);
        if(data_buffer)
        {
            data = svp_acl_get_data_buffer_addr(data_buffer);
            svp_acl_rt_free(data);
            svp_acl_destroy_data_buffer(data_buffer);
        }
    }

    void yolov5::destroy_svp_output()
    {
        size_t i;
        size_t output_num;

        if(m_task_info.output_dataset)
        {
            output_num = svp_acl_mdl_get_dataset_num_buffers(m_task_info.output_dataset);
            for (i = 0; i < output_num; i++)
            {
                destroy_svp_output_buffer(i);
            }

            svp_acl_mdl_destroy_dataset(m_task_info.output_dataset);
            m_task_info.output_dataset = NULL;
        }
    }

    void yolov5::destroy_svp_input()
    {
        size_t i;
        size_t input_num;

        if(m_task_info.input_dataset)
        {
            input_num = svp_acl_mdl_get_dataset_num_buffers(m_task_info.input_dataset);
            for (i = 0; i < input_num; i++)
            {
                destroy_svp_input_buffer(i);
            }

            m_task_info.task_buf_ptr = NULL;
            m_task_info.task_buf_size = 0;
            m_task_info.task_buf_stride = 0;
            m_task_info.work_buf_ptr = NULL;
            m_task_info.work_buf_size = 0;
            m_task_info.work_buf_stride = 0;

            svp_acl_mdl_destroy_dataset(m_task_info.input_dataset);
            m_task_info.input_dataset = NULL;
        }
    }

    void yolov5::destroy_svp_input_buffer(int index)
    {
        td_void *data = TD_NULL;
        svp_acl_data_buffer *data_buffer = TD_NULL;

        data_buffer = svp_acl_mdl_get_dataset_buffer(m_task_info.input_dataset,index);
        if(data_buffer)
        {
            data = svp_acl_get_data_buffer_addr(data_buffer);
            svp_acl_rt_free(data);
            svp_acl_destroy_data_buffer(data_buffer);
        }
    }

    svp_acl_data_buffer* yolov5::create_svp_output_buffer(int index)
    {
        td_s32 ret;
        svp_acl_data_buffer *output_data = TD_NULL;
        td_void *output_buffer = TD_NULL;
        size_t buffer_size, stride;

        stride = svp_acl_mdl_get_output_default_stride(m_model_desc,index);
        buffer_size = svp_acl_mdl_get_output_size_by_index(m_model_desc,index);
        if(stride == 0
                || (buffer_size == 0 || buffer_size > SAMPLE_SVP_NPU_MAX_MEM_SIZE))
        {
            return NULL;
        }

        ret = svp_acl_rt_malloc(&output_buffer, buffer_size, SVP_ACL_MEM_MALLOC_NORMAL_ONLY); 
        if(ret != SVP_ACL_SUCCESS)
        {
            DEV_WRITE_LOG_ERROR("svp_acl_mdl_rt_malloc failed with 0x%x",ret);
            return NULL;
        }
        memset(output_buffer,0,buffer_size);

        output_data = svp_acl_create_data_buffer(output_buffer, buffer_size, stride);
        if(output_data == NULL)
        {
            DEV_WRITE_LOG_ERROR("svp_acl_create_data_buffer failed");
            svp_acl_rt_free(output_buffer);
            return NULL;
        }

        return output_data;
    }

    svp_acl_data_buffer* yolov5::create_svp_input_buffer(int index)
    {
        td_s32 ret;
        svp_acl_data_buffer *input_data = TD_NULL;
        td_void *input_buffer = TD_NULL;
        size_t buffer_size, stride;

        stride = svp_acl_mdl_get_input_default_stride(m_model_desc,index);
        buffer_size = svp_acl_mdl_get_input_size_by_index(m_model_desc,index);
        if(stride == 0
                || (buffer_size == 0 || buffer_size > SAMPLE_SVP_NPU_MAX_MEM_SIZE))
        {
            return NULL;
        }

        ret = svp_acl_rt_malloc(&input_buffer, buffer_size, SVP_ACL_MEM_MALLOC_NORMAL_ONLY); 
        if(ret != SVP_ACL_SUCCESS)
        {
            DEV_WRITE_LOG_ERROR("svp_acl_mdl_rt_malloc failed with 0x%x",ret);
            return NULL;
        }
        memset(input_buffer,0,buffer_size);
        
        input_data = svp_acl_create_data_buffer(input_buffer, buffer_size, stride);
        if(input_data == NULL)
        {
            DEV_WRITE_LOG_ERROR("svp_acl_create_data_buffer failed");
            svp_acl_rt_free(input_buffer);
            return NULL;
        }

        if((size_t)index == m_input_num - SAMPLE_SVP_NPU_EXTRA_INPUT_NUM)
        {
            m_task_info.task_buf_ptr = input_buffer;
            m_task_info.task_buf_size = buffer_size;
            m_task_info.task_buf_stride = stride;
        }else if((size_t)index == m_input_num - 1)
        {
            m_task_info.work_buf_ptr = input_buffer;
            m_task_info.work_buf_size = buffer_size;
            m_task_info.work_buf_stride = stride;
        }

        return input_data;
    }

    bool yolov5::create_svp_input()
    {
        td_s32 ret;
        size_t i;
        svp_acl_data_buffer *input_data = TD_NULL;

        svp_acl_mdl_dataset* input_dataset = svp_acl_mdl_create_dataset();
        if(input_dataset == TD_NULL)
        {
            DEV_WRITE_LOG_ERROR("svp_acl_mdl_create_dataset failed ");
            return false;
        }

        for(i = 0; i < m_input_num - 2; i++)
        {
            input_data = create_svp_input_buffer(i);
            if(input_data == NULL)
            {
                svp_acl_mdl_destroy_dataset(input_dataset);
                return false;
            }

            ret = svp_acl_mdl_add_dataset_buffer(input_dataset, input_data);
            if(ret != SVP_ACL_SUCCESS)
            {
                DEV_WRITE_LOG_ERROR("svp_acl_mdl_add_dataset_buffer failed with error 0x%x",ret);
                svp_acl_mdl_destroy_dataset(input_dataset);
                return false;
            }
        }

        //taskbuf
        input_data =  create_svp_input_buffer(m_input_num - 2);
        assert(input_data != NULL);
        svp_acl_mdl_add_dataset_buffer(input_dataset,input_data);

        //workbuf
        input_data = create_svp_input_buffer(m_input_num - 1);
        assert(input_data != NULL);
        svp_acl_mdl_add_dataset_buffer(input_dataset,input_data);

        m_task_info.input_dataset = input_dataset;
        return true;
    }

    bool yolov5::create_venc_chn()
    {
        td_s32 ret;
        ret = ss_mpi_venc_create_chn(m_venc_chn,&m_venc_chn_attr);
        if(ret != TD_SUCCESS)
        {
            DEV_WRITE_LOG_ERROR("ss_mpi_venc_create_chn[%d] faild with %#x!",m_venc_chn, ret);
            return false;
        }

        ot_venc_start_param venc_start_param;
        venc_start_param.recv_pic_num = -1;
        ret = ss_mpi_venc_start_chn(m_venc_chn,&venc_start_param);
        if(ret != TD_SUCCESS)
        {
            DEV_WRITE_LOG_ERROR("HI_MPI_VENC_StartRecvPic faild with%#x!", ret);
            return false;
        }
        
        return true;
    }

    void yolov5::destroy_venc_chn()
    {
        ss_mpi_venc_stop_chn(m_venc_chn);
        ss_mpi_venc_destroy_chn(m_venc_chn);
    }

    bool yolov5::create_vpss_grp_chn()
    {
        td_s32 ret;

        if(!create_vb_pool())
        {
            return false;
        }
        
        ret = ss_mpi_vpss_set_chn_vb_src(m_vpss_grp,m_vpss_chn,OT_VB_SRC_USER);
        if (ret != TD_SUCCESS) 
        {
            DEV_WRITE_LOG_ERROR("ss_mpi_vpss_set_chn_vb_src failed with %#x", ret);
            destroy_vb_pool();
            return false;
        }

        ret = ss_mpi_vpss_attach_chn_vb_pool(m_vpss_grp,m_vpss_chn,m_vb_poolid);
        if (ret != TD_SUCCESS) 
        {
            DEV_WRITE_LOG_ERROR("ss_mpi_vpss_attach_chn_vb_pool failed with %#x", ret);
            destroy_vb_pool();
            return false;
        }

        ret = ss_mpi_vpss_set_chn_attr(m_vpss_grp, m_vpss_chn, &m_vpss_chn_attr);
        if (ret != TD_SUCCESS) 
        {
            DEV_WRITE_LOG_ERROR("ss_mpi_vpss_set_chn_attr failed with %#x", ret);
            destroy_vb_pool();
            return false;
        }
        ret = ss_mpi_vpss_enable_chn(m_vpss_grp, m_vpss_chn);
        if (ret != TD_SUCCESS) 
        {
            DEV_WRITE_LOG_ERROR("ss_mpi_vpss_enable_chn_attr failed with %#x", ret);
            destroy_vb_pool();
            return false;
        }

        return true;
    }

    void yolov5::destroy_vpss_grp_chn()
    {
        ss_mpi_vpss_disable_chn(m_vpss_grp, m_vpss_chn);
        destroy_vb_pool();
    }

    bool yolov5::set_svp_threshold()
    {
        td_u32 n;
        svp_acl_error ret;
        size_t idx, size;
        svp_acl_data_buffer *data_buffer = TD_NULL;
        td_float *data = TD_NULL;

        ret = svp_acl_mdl_get_input_index_by_name(m_model_desc,"rpn_data",&idx);
        if(ret != SVP_ACL_SUCCESS)
        {
            DEV_WRITE_LOG_ERROR("svp_acl_mdl_get_input_index_by_name failed with %#x", ret);
            return false;
        }

        data_buffer = svp_acl_mdl_get_dataset_buffer(m_task_info.input_dataset, idx);
        if(data_buffer == TD_NULL)
        {
            DEV_WRITE_LOG_ERROR("svp_acl_mdl_get_dataset_buffer failed");
            return false;
        }

        size = svp_acl_get_data_buffer_size(data_buffer);
        if(size  < 4 * sizeof(td_float))
        {
            DEV_WRITE_LOG_ERROR("svp_acl_mdl_get_data_buffer_size size:%f", size);
            return false;
        }

        data = (td_float *)svp_acl_get_data_buffer_addr(data_buffer);
        if(data == NULL)
        {
            DEV_WRITE_LOG_ERROR("svp_acl_mdl_get_data_buffer_addr");
            return false;
        }

        //got from sample
        n = 0;
        data[n++] = 0.45;//nms_threshold
        data[n++] = 0.5;//score_threshold
        data[n++] = 1.0;//min_height
        data[n++] = 1.0;//min_width

        return true;
    }

    bool yolov5::create_svp_rgn(int idx)
    {
        svp_acl_error ret;
        ot_rgn_attr rgn_attr;
        memset(&rgn_attr,0,sizeof(rgn_attr));
        rgn_attr.type = OT_RGN_OVERLAY; 
        rgn_attr.attr.overlay.pixel_format = OT_PIXEL_FORMAT_ARGB_1555;
        rgn_attr.attr.overlay.canvas_num = 1;
        rgn_attr.attr.overlay.bg_color = SAMPLE_SVP_OSD_BG_COLOR;
        rgn_attr.attr.overlay.size.width = ROUND_UP(m_max_rgn_w,64);
        rgn_attr.attr.overlay.size.height = ROUND_UP(SAMPLE_SVP_OSD_FONT_SIZE + 4,2);
        ot_rgn_handle rgn_h = sys::alloc_rgn_handle();
        ret = ss_mpi_rgn_create(rgn_h, &rgn_attr);
        if(ret != SVP_ACL_SUCCESS)
        {
            DEV_WRITE_LOG_ERROR("ss_mpi_rgn_create failed");
            return false;
        }

        m_rgn[idx] = rgn_h;
        return true;
    }

    bool yolov5::get_svp_rio(svp_npu_rect_info_t* rect_info)
    {
        svp_acl_error ret;
        size_t roi_idx, stride;
        const char* ROI_NAME = "output0_";
        int i;
        svp_acl_data_buffer *data_buffer = TD_NULL;
        td_float *x_min = TD_NULL;
        td_float *y_min = TD_NULL;
        td_float *x_max = TD_NULL;
        td_float *y_max = TD_NULL;
        td_float *score = TD_NULL;
        td_float *class_id = TD_NULL;

        if(!get_svp_roi_num(&rect_info->num))
        {
            return false;
        }

        ret = svp_acl_mdl_get_output_index_by_name(m_model_desc,ROI_NAME,&roi_idx);
        if(ret != SVP_ACL_SUCCESS)
        {
            DEV_WRITE_LOG_ERROR("svp_acl_mdl_get_output_index_by_name failed with 0x%x",ret);
            return false;
        }

        data_buffer = svp_acl_mdl_get_dataset_buffer(m_task_info.output_dataset, roi_idx);
        if(data_buffer == TD_NULL)
        {
            DEV_WRITE_LOG_ERROR("svp_acl_mdl_get_dataset_buffer failed");
            return false;
        }

        stride = svp_acl_get_data_buffer_stride(data_buffer);
        if(stride == 0)
        {
            DEV_WRITE_LOG_ERROR("svp_acl_get_data_buffer_stride failed");
            return false;
        }

         x_min = (td_float *)svp_acl_get_data_buffer_addr(data_buffer);
         if(x_min == TD_NULL)
         {
            DEV_WRITE_LOG_ERROR("svp_acl_get_data_buffer_addr failed");
            return false;
         }

         td_u32 roi_offset = 0;
         y_min = x_min + stride / sizeof(td_float);
         x_max = y_min + stride / sizeof(td_float);
         y_max = x_max + stride / sizeof(td_float);
         score = y_max + stride / sizeof(td_float);
         class_id = score + stride / sizeof(td_float);
         for (i = 0; i < rect_info->num; i++) 
         {
             rect_info->rect[i].point[SAMPLE_SVP_NPU_RECT_LEFT_TOP].x = (td_u32)((td_float)x_min[roi_offset]) & (~1);
             rect_info->rect[i].point[SAMPLE_SVP_NPU_RECT_LEFT_TOP].y = (td_u32)((td_float)y_min[roi_offset]) & (~1);

             rect_info->rect[i].point[SAMPLE_SVP_NPU_RECT_RIGHT_TOP].x = (td_u32)((td_float)x_max[roi_offset]) & (~1);
             rect_info->rect[i].point[SAMPLE_SVP_NPU_RECT_RIGHT_TOP].y = rect_info->rect[i].point[SAMPLE_SVP_NPU_RECT_LEFT_TOP].y;

             rect_info->rect[i].point[SAMPLE_SVP_NPU_RECT_RIGHT_BOTTOM].x = rect_info->rect[i].point[SAMPLE_SVP_NPU_RECT_RIGHT_TOP].x;
             rect_info->rect[i].point[SAMPLE_SVP_NPU_RECT_RIGHT_BOTTOM].y = (td_u32)((td_float)y_max[roi_offset]) & (~1);

             rect_info->rect[i].point[SAMPLE_SVP_NPU_RECT_LEFT_BOTTOM].x = rect_info->rect[i].point[SAMPLE_SVP_NPU_RECT_LEFT_TOP].x;
             rect_info->rect[i].point[SAMPLE_SVP_NPU_RECT_LEFT_BOTTOM].y = rect_info->rect[i].point[SAMPLE_SVP_NPU_RECT_RIGHT_BOTTOM].y;

             rect_info->rect[i].score = score[roi_offset];
             rect_info->rect[i].class_id = (td_u32)class_id[roi_offset];

             roi_offset++;
         }

         return true;
    }

    bool yolov5::get_svp_roi_num(td_u16* pnum)
    {
        svp_acl_error ret;
        const char* NUM_NAME = "output0";
        size_t num_idx;
        svp_acl_mdl_io_dims dims;
        svp_acl_data_buffer *data_buffer = TD_NULL;
        td_float *data = TD_NULL;
        td_float total_num = 0.0;
        int i;

        ret = svp_acl_mdl_get_output_index_by_name(m_model_desc,NUM_NAME,&num_idx);
        if(ret != SVP_ACL_SUCCESS)
        {
            DEV_WRITE_LOG_ERROR("svp_acl_mdl_get_output_index_by_name failed with 0x%x",ret);
            return false;
        }

        ret = svp_acl_mdl_get_output_dims(m_model_desc, num_idx, &dims);
        if(ret != SVP_ACL_SUCCESS)
        {
            DEV_WRITE_LOG_ERROR("svp_acl_mdl_get_output_dims failed with error 0x%x",ret);
            return false;
        }

        data_buffer = svp_acl_mdl_get_dataset_buffer(m_task_info.output_dataset, num_idx);
        if(data_buffer == TD_NULL)
        {
            DEV_WRITE_LOG_ERROR("svp_acl_mdl_get_dataset_buffer failed");
            return false;
        }

        data = (td_float *)svp_acl_get_data_buffer_addr(data_buffer);
        if(data == TD_NULL)
        {
            DEV_WRITE_LOG_ERROR("svp_acl_mdl_get_dataset_buffer_addr failed");
            return false;
        }

        for(i = 0; i < dims.dims[dims.dim_count - 1]; i++)
        {
            total_num += *(data + i);
        }

        *pnum = total_num;

        return true;
    }

    void yolov5::on_venc_process()
    {
        fd_set read_fds;
        struct timeval time_val;
        td_s32 ret;
        ot_venc_stream stream;
        ot_venc_chn_status stat;
        td_s32 venc_fd = ss_mpi_venc_get_fd(m_venc_chn);

        while(m_is_start)
        {
            FD_ZERO(&read_fds);
            FD_SET(venc_fd, &read_fds);
                
            time_val.tv_sec  = 0;
            time_val.tv_usec = 10000;
            ret = select(venc_fd + 1, &read_fds, NULL, NULL, &time_val);
            if (ret < 0)
            {
                DEV_WRITE_LOG_ERROR("select faild with %#x!", ret);
                break;
            }
            else if (ret == 0)
            {
                continue;
            }

            memset(&stream, 0, sizeof(stream));
            ret = ss_mpi_venc_query_status(m_venc_chn, &stat);
            if (ret != TD_SUCCESS)
            {
                DEV_WRITE_LOG_ERROR("ss_mpi_venc_query_status failed with %#x", ret);
                continue;
            }
            if(stat.cur_packs == 0)
            {
                DEV_WRITE_LOG_ERROR("NOTE: Current  frame is NULL!");
                continue;
            }

            stream.pack = (ot_venc_pack*)malloc(sizeof(ot_venc_pack) * stat.cur_packs);
            if (stream.pack == NULL)
            {
                DEV_WRITE_LOG_ERROR("malloc memory failed!");
                break;
            }
            stream.pack_cnt = stat.cur_packs;
            ret = ss_mpi_venc_get_stream(m_venc_chn, &stream, TD_TRUE);
            if (ret != TD_SUCCESS)
            {
                free(stream.pack);
                stream.pack = NULL;
                DEV_WRITE_LOG_ERROR("ss_mpi_venc_get_stream failed with %#x", ret);
                break;
            }

            process_video_stream(&stream);
            ss_mpi_venc_release_stream(m_venc_chn, &stream);
            free(stream.pack);
            stream.pack = NULL;
        }

        DEV_WRITE_LOG_INFO("thread exit!");
    }

    void yolov5::on_process()
    {
        svp_acl_error ret;
        svp_acl_data_buffer *data_buffer = TD_NULL;
        ot_video_frame_info frame;
        const td_s32 milli_sec = 1000;
        svp_npu_rect_info_t rect_info;
        int i;
        td_void *virt_addr = TD_NULL;
        td_void *ori_data = TD_NULL;
        size_t ori_size,ori_stride;
        
        ret = svp_acl_rt_set_device(0);
        if(ret != SVP_ACL_SUCCESS)
        {
            DEV_WRITE_LOG_ERROR("svp_acl_rt_set_device failed with error 0x%x",ret);
            return;
        }

        virt_addr = ss_mpi_sys_mmap(m_vb_pool_info.pool_phy_addr,m_vb_pool_info.pool_size);
        if(virt_addr == NULL)
        {
            DEV_WRITE_LOG_ERROR("svp_mpi_ss_mmap failed");
            return;
        }

        data_buffer = svp_acl_mdl_get_dataset_buffer(m_task_info.input_dataset, 0);
        ori_data = svp_acl_get_data_buffer_addr(data_buffer);
        ori_size = svp_acl_get_data_buffer_size(data_buffer);
        ori_stride = svp_acl_get_data_buffer_stride(data_buffer);

        ceanic::util::check_interval ci;
        while(m_is_start)
        {
            ret = ss_mpi_vpss_get_chn_frame(m_vpss_grp,m_vpss_chn,&frame,milli_sec);
            if(ret != TD_SUCCESS)
            {
                DEV_WRITE_LOG_ERROR("svp_mpi_vpss_get_chn_frame failed with error 0x%x",ret);
                break;
            }

            td_u32 frame_size = frame.video_frame.height * frame.video_frame.stride[0] * 3 / 2;
            td_void* frame_virt_addr = (td_u8*)virt_addr + (frame.video_frame.phys_addr[0] - m_vb_pool_info.pool_phy_addr);
            ret = svp_acl_update_data_buffer(data_buffer,frame_virt_addr,frame_size,frame.video_frame.stride[0]);
            if(ret != TD_SUCCESS)
            {
                DEV_WRITE_LOG_ERROR("svp_acl_update_data_buffer failed with error 0x%x",ret);
                ss_mpi_vpss_release_chn_frame(m_vpss_grp,m_vpss_chn,&frame);
                break;
            }

            //ci.set_beg();
            ret = svp_acl_mdl_execute(m_model_id, m_task_info.input_dataset, m_task_info.output_dataset);
            if(ret != TD_SUCCESS)
            {
                DEV_WRITE_LOG_ERROR("svp_acl_mdl_execute failed with error 0x%x",ret);
                ss_mpi_vpss_release_chn_frame(m_vpss_grp,m_vpss_chn,&frame);
                break;
            }
            //printf("interval=%lld\n",ci.interval());

            memset(&rect_info,0,sizeof(rect_info));
            if(!get_svp_rio(&rect_info))
            {
                ss_mpi_vpss_release_chn_frame(m_vpss_grp,m_vpss_chn,&frame);
                break;
            }

            if(rect_info.num > 0)
            {
#if 0
                printf("----------ROI Data----------\n");
                printf("\tnum:%d\n",rect_info.num);
                for(i = 0; i < rect_info.num; i++)
                {
                    printf("\trect[%d],score:%f,class_id:%d,lt(%d,%d),rt(%d,%d),rb(%d,%d),lb(%d,%d)\n",
                            i,
                            rect_info.rect[i].score,
                            rect_info.rect[i].class_id,
                            rect_info.rect[i].point[SAMPLE_SVP_NPU_RECT_LEFT_TOP].x,rect_info.rect[i].point[SAMPLE_SVP_NPU_RECT_LEFT_TOP].y,
                            rect_info.rect[i].point[SAMPLE_SVP_NPU_RECT_RIGHT_TOP].x,rect_info.rect[i].point[SAMPLE_SVP_NPU_RECT_RIGHT_TOP].y,
                            rect_info.rect[i].point[SAMPLE_SVP_NPU_RECT_RIGHT_BOTTOM].x,rect_info.rect[i].point[SAMPLE_SVP_NPU_RECT_RIGHT_BOTTOM].y,
                            rect_info.rect[i].point[SAMPLE_SVP_NPU_RECT_LEFT_BOTTOM].x,rect_info.rect[i].point[SAMPLE_SVP_NPU_RECT_LEFT_BOTTOM].y);
                }
#endif
                svp_vgs_fill_rect(&frame, &rect_info,0x0000FF00);
            }
            ss_mpi_venc_send_frame(m_venc_chn,&frame,1000);
            ss_mpi_vpss_release_chn_frame(m_vpss_grp,m_vpss_chn,&frame);
        }

        svp_acl_update_data_buffer(data_buffer,ori_data,ori_size,ori_stride);
        ss_mpi_sys_munmap(virt_addr,m_vb_pool_info.pool_size);
        svp_acl_rt_reset_device(0);

        for(i = 0; i < SVP_RECT_NUM; i++)
        {
            if(m_rgn[i] != (ot_rgn_handle)OT_INVALID_HANDLE)
            {
                ss_mpi_rgn_destroy(m_rgn[i]);
                m_rgn[i] = OT_INVALID_HANDLE;
            }
        }
    }

    bool yolov5::start()
    {
        svp_acl_error ret;
        svp_acl_mdl_io_dims dims = {0};
        
        if(m_is_start)
        {
            return false;
        }

        std::shared_ptr<vi_isp> viisp = std::dynamic_pointer_cast<vi_isp>(m_vi_ptr);
        if(!viisp)
        {
            return false;
        }

        FILE* f = fopen(m_model_path.c_str(),"rb");
        if(f == NULL)
        {
            DEV_WRITE_LOG_ERROR("open %s failed",m_model_path.c_str());
            return false;
        }

        fseek(f, 0L, SEEK_END);
        m_model_mem_size = ftell(f);
        if(m_model_mem_size <= 0)
        {
            DEV_WRITE_LOG_ERROR("invalid model mem size:%d",m_model_mem_size);
            fclose(f);
            return false;
        }
        fseek(f, 0L, SEEK_SET);

        ret = svp_acl_rt_malloc(&m_model_mem_ptr,m_model_mem_size, SVP_ACL_MEM_MALLOC_NORMAL_ONLY);
        if(ret != SVP_ACL_SUCCESS)
        {
            DEV_WRITE_LOG_ERROR("svp_acl_rt_malloc failed with 0x%x",ret);
            fclose(f);
            return false;
        }

        ret = fread(m_model_mem_ptr,m_model_mem_size,1,f);
        if(ret != 1)
        {
            DEV_WRITE_LOG_ERROR("fread failed with 0x%x",ret);
            fclose(f);
            return false;
        }

        fclose(f);

        ret = svp_acl_mdl_load_from_mem(m_model_mem_ptr,m_model_mem_size,&m_model_id);
        if(ret != SVP_ACL_SUCCESS)
        {
            DEV_WRITE_LOG_ERROR("svp_acl_mdl_load_from_mem with 0x%x",ret);
            goto end1;
        }

        m_model_desc = svp_acl_mdl_create_desc();
        if(m_model_desc == NULL)
        {
            DEV_WRITE_LOG_ERROR("svp_acl_mdl_create_desc with 0x%x",ret);
            goto end1;
        }

        ret = svp_acl_mdl_get_desc(m_model_desc, m_model_id);  
        if(ret != SVP_ACL_SUCCESS)
        {
            DEV_WRITE_LOG_ERROR("svp_acl_mdl_create_desc with 0x%x",ret);
            goto end0;
        }

        m_input_num = svp_acl_mdl_get_num_inputs(m_model_desc);
        if(m_input_num < SAMPLE_SVP_NPU_EXTRA_INPUT_NUM + 1)
        {
            DEV_WRITE_LOG_ERROR("svp_acl_mdl_get_num_inputs failed,num:%d",m_input_num);
            goto end0;
        }

        m_output_num = svp_acl_mdl_get_num_outputs(m_model_desc);
        if(m_output_num < 1)
        {
            DEV_WRITE_LOG_ERROR("svp_acl_mdl_get_num_outputs failed,num:%d",m_output_num);
            goto end0;
        }

        ret = svp_acl_mdl_get_input_index_by_name(m_model_desc,SVP_ACL_DYNAMIC_TENSOR_NAME, &m_dynamic_batch_idx);
        if(ret != SVP_ACL_SUCCESS)
        {
            DEV_WRITE_LOG_ERROR("svp_acl_mdl_get_input_index_by_name failed with error 0x%x",ret);
            goto end0;
        }

        ret = svp_acl_mdl_get_input_dims(m_model_desc, 0, &dims);
        if(ret != SVP_ACL_SUCCESS)
        {
            DEV_WRITE_LOG_ERROR("svp_acl_mdl_get_input_dims failed with error 0x%x",ret);
            goto end0;
        }
        m_pic_size.height = dims.dims[dims.dim_count - SAMPLE_SVP_NPU_H_DIM_IDX]; // NCHW
        m_pic_size.width = dims.dims[dims.dim_count - 1]; // NCHW
        DEV_WRITE_LOG_INFO("input_num = %d,yolov5 pic size=(%d,%d)",m_input_num,m_pic_size.width,m_pic_size.height);

        m_vpss_chn_attr.width = m_pic_size.width;
        m_vpss_chn_attr.height = m_pic_size.height;
        m_vpss_grp = m_vi_ptr->vpss_grp();
        m_vpss_chn = m_vi_ptr->vpss_chn() + 1;

        m_venc_chn_attr.venc_attr.max_pic_width = m_pic_size.width;
        m_venc_chn_attr.venc_attr.max_pic_height = m_pic_size.height;
        m_venc_chn_attr.venc_attr.pic_width = m_pic_size.width;
        m_venc_chn_attr.venc_attr.pic_height    = m_pic_size.height;
        m_venc_chn_attr.venc_attr.buf_size      = m_pic_size.width * m_pic_size.height * 3 / 2;

        if(!create_vpss_grp_chn()
                || !create_venc_chn())
        {
            goto end0;
        }

        if(!create_svp_input()
                || !create_svp_output()
                || !set_svp_threshold())
        {
            goto end3;
        }

        m_is_start = true;
        m_venc_thread = std::thread(&yolov5::on_venc_process,this);
        m_thread = std::thread(&yolov5::on_process,this);
        return true;
end3:
        destroy_svp_input();
        destroy_svp_output();
        destroy_venc_chn();
        destroy_vpss_grp_chn();
end0:
        svp_acl_mdl_destroy_desc(m_model_desc);
        m_model_desc = NULL;
end1:
        svp_acl_rt_free(m_model_mem_ptr);
        m_model_mem_ptr = NULL;
        m_model_mem_size = 0;
        m_model_id = 0;
        return false;
    }

    void yolov5::stop()
    {
        if(!m_is_start)
        {
            return ;
        }

        m_is_start = false;
        m_thread.join();
        m_venc_thread.join();

        destroy_venc_chn();
        destroy_vpss_grp_chn();
        destroy_svp_output();
        destroy_svp_input();

        svp_acl_mdl_unload(m_model_id);
        svp_acl_mdl_destroy_desc(m_model_desc);

        svp_acl_rt_free(m_model_mem_ptr);
        m_model_mem_ptr = NULL;
        m_model_mem_size = 0;
    }

    int yolov5::venc_w()
    {
        return m_pic_size.width;
    }

    int yolov5::venc_h()
    {
        return m_pic_size.height;
    }

    void yolov5::svp_vgs_fill_rect(const ot_video_frame_info *frame_info,svp_npu_rect_info_t* rect,td_u32 color)
    {
        ot_vgs_handle vgs_handle = -1;
        td_s32 ret = TD_FAILURE;
        td_u16 i;
        ot_vgs_task_attr vgs_task;
        ot_cover vgs_add_cover;
        ot_vgs_osd vgs_add_osd;

        ss_mpi_vgs_begin_job(&vgs_handle);

        memcpy_s(&vgs_task.img_out, sizeof(ot_video_frame_info), frame_info, sizeof(ot_video_frame_info));
        memcpy_s(&vgs_task.img_in, sizeof(ot_video_frame_info), frame_info, sizeof(ot_video_frame_info));

        vgs_add_cover.type = OT_COVER_QUAD;
        vgs_add_cover.color = color;
        for (i = 0; i < rect->num; i++)
        {
            vgs_add_cover.quad_attr.is_solid = TD_FALSE;
            vgs_add_cover.quad_attr.thick = 2;
            memcpy_s(vgs_add_cover.quad_attr.point, sizeof(rect->rect[i].point),rect->rect[i].point, sizeof(rect->rect[i].point));
            ss_mpi_vgs_add_cover_task(vgs_handle, &vgs_task, &vgs_add_cover, 1);
        }

        vgs_add_osd.bg_color = SAMPLE_SVP_OSD_BG_COLOR;
        vgs_add_osd.pixel_format = OT_PIXEL_FORMAT_ARGB_1555;
        vgs_add_osd.fg_alpha = 255;
        //vgs_add_osd.bg_alpha = 128; 
        vgs_add_osd.bg_alpha = 0; 
        vgs_add_osd.osd_inverted_color = OT_VGS_OSD_INVERTED_COLOR_NONE;

        short bg_color = SAMPLE_SVP_OSD_BG_COLOR;
        short fg_color = SAMPLE_SVP_OSD_FG_COLOR;
        short outline_color = SAMPLE_SVP_OSD_OUTLINE_COLOR;

        char str[255];
        for (i = 0; i < rect->num; i++)
        {
            if(m_rgn[i] == (ot_rgn_handle)OT_INVALID_HANDLE)
            {
                create_svp_rgn(i);

                if(m_rgn[i] == (ot_rgn_handle)OT_INVALID_HANDLE)
                {
                    break;
                }
            }

            ot_rgn_canvas_info canvas_info;
            memset(&canvas_info,0,sizeof(canvas_info));
            ret = ss_mpi_rgn_get_canvas_info(m_rgn[i], &canvas_info);
            if(ret != SVP_ACL_SUCCESS)
            {
                DEV_WRITE_LOG_ERROR("ss_mpi_rgn_get_canvas_info failed");
                break;
            }

            vgs_add_osd.rect.width = canvas_info.size.width;
            vgs_add_osd.rect.height = canvas_info.size.height;
            vgs_add_osd.phys_addr = canvas_info.phys_addr;
            vgs_add_osd.stride = canvas_info.stride;

            sprintf(str,"%s %.2f",g_yolov5_class_str[rect->rect[i].class_id].c_str(),rect->rect[i].score);
            unsigned short* p = (unsigned short*)canvas_info.virt_addr;
            for(unsigned int j = 0; j < canvas_info.size.width * canvas_info.size.height; j++)
            {
                *p = SAMPLE_SVP_OSD_BG_COLOR;
                p++;
            }
           
            g_freetype.show_string(str,canvas_info.size.width,vgs_add_osd.rect.height,SAMPLE_SVP_OSD_FONT_SIZE,(unsigned char*)canvas_info.virt_addr,canvas_info.size.width * canvas_info.size.height * 2,bg_color,fg_color,outline_color);

            ss_mpi_rgn_update_canvas(m_rgn[i]);

            vgs_add_osd.rect.x = rect->rect[i].point[SAMPLE_SVP_NPU_RECT_LEFT_TOP].x;
            vgs_add_osd.rect.y = rect->rect[i].point[SAMPLE_SVP_NPU_RECT_LEFT_TOP].y;
            vgs_add_osd.rect.y -= vgs_add_osd.rect.height;
            if(vgs_add_osd.rect.y < 0)
            {
                vgs_add_osd.rect.y = 0;
            }

            ss_mpi_vgs_add_osd_task(vgs_handle,&vgs_task,&vgs_add_osd,1);
        }

        ss_mpi_vgs_end_job(vgs_handle);
    }

}}//namespace
