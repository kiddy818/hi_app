#include "dev_svp_yolov5.h"
#include "dev_log.h"

namespace hisilicon{namespace dev{
    
    #define SAMPLE_SVP_NPU_EXTRA_INPUT_NUM   2
    #define SAMPLE_SVP_NPU_H_DIM_IDX 2
    #define SAMPLE_SVP_NPU_MAX_MEM_SIZE      0xFFFFFFFF

    yolov5::yolov5(std::shared_ptr<vi> vi_ptr,const char* model_path)
        :m_is_start(false),m_model_path(model_path),m_vi_ptr(vi_ptr)
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
        m_vpss_chn_attr.frame_rate.src_frame_rate = -1;
        m_vpss_chn_attr.frame_rate.dst_frame_rate = -1;
    }

    yolov5::~yolov5()
    {
        stop();
    }

    bool yolov5::create_svp_output()
    {
        td_s32 ret;
        int i;
        svp_acl_data_buffer *output_data = TD_NULL;

        svp_acl_mdl_dataset* output_dataset = svp_acl_mdl_create_dataset();
        if(output_dataset == NULL)
        {
            DEV_WRITE_LOG_ERROR("svp_acl_mdl_create_dataset failed ");
            return false;
        }

        for (i = 0; i < m_output_num; i++)
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
        td_s32 ret;
        td_void *data = TD_NULL;
        svp_acl_data_buffer *data_buffer = TD_NULL;
        td_ulong class_id;

        data_buffer = svp_acl_mdl_get_dataset_buffer(m_task_info.output_dataset,index);
        if(data_buffer)
        {
            ret = svp_acl_mdl_get_output_class_id_by_index(m_model_desc,index,&class_id);
            if(ret == SVP_ACL_SUCCESS)
            {
                data = svp_acl_get_data_buffer_addr(data_buffer);
                if(class_id == 0)
                {
                    svp_acl_rt_free(data);
                }
                else
                {
                    svp_acl_rt_free_sec(data);
                }
            }
            svp_acl_destroy_data_buffer(data_buffer);
        }
    }

    void yolov5::destroy_svp_output()
    {
        int i;
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
        int i;
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
        td_s32 ret;
        td_void *data = TD_NULL;
        svp_acl_data_buffer *data_buffer = TD_NULL;
        td_ulong class_id;

        data_buffer = svp_acl_mdl_get_dataset_buffer(m_task_info.input_dataset,index);
        if(data_buffer)
        {
            ret = svp_acl_mdl_get_input_class_id_by_index(m_model_desc,index,&class_id);
            if(ret == SVP_ACL_SUCCESS)
            {
                data = svp_acl_get_data_buffer_addr(data_buffer);
                if(class_id == 0)
                {
                    svp_acl_rt_free(data);
                }
                else
                {
                    svp_acl_rt_free_sec(data);
                }
            }
            svp_acl_destroy_data_buffer(data_buffer);
        }
    }

    svp_acl_data_buffer* yolov5::create_svp_output_buffer(int index)
    {
        td_s32 ret;
        td_ulong class_id;
        svp_acl_data_buffer *output_data = TD_NULL;
        td_void *output_buffer = TD_NULL;
        size_t buffer_size, stride;

        stride = svp_acl_mdl_get_output_default_stride(m_model_desc,index);
        buffer_size = svp_acl_mdl_get_output_size_by_index(m_model_desc,index);
        ret = svp_acl_mdl_get_output_class_id_by_index(m_model_desc,index,&class_id);
        if(stride == 0
                || (buffer_size == 0 || buffer_size > SAMPLE_SVP_NPU_MAX_MEM_SIZE)
                || ret != SVP_ACL_SUCCESS)
        {
            DEV_WRITE_LOG_ERROR("svp_acl_mdl_get_output_info failed with 0x%x",ret);
            return NULL;
        }

        if(class_id != 0)
        {
            ret = svp_acl_rt_malloc_sec(&output_buffer, buffer_size, class_id);
        }
        else
        {
            ret = svp_acl_rt_malloc(&output_buffer, buffer_size, SVP_ACL_MEM_MALLOC_NORMAL_ONLY); 
            if(ret == SVP_ACL_SUCCESS)
            {
                memset(output_buffer,0,buffer_size);
            }
        }
        if(ret != SVP_ACL_SUCCESS)
        {
            DEV_WRITE_LOG_ERROR("svp_acl_mdl_rt_malloc failed with 0x%x",ret);
            return NULL;
        }

        output_data = svp_acl_create_data_buffer(output_buffer, buffer_size, stride);
        if(output_data == NULL)
        {
            DEV_WRITE_LOG_ERROR("svp_acl_create_data_buffer failed");
            if(class_id != 0)
            {
                svp_acl_rt_free_sec(output_buffer);
            }
            else
            {
                svp_acl_rt_free(output_buffer);
            }
            return NULL;
        }

        return output_data;
    }

    svp_acl_data_buffer* yolov5::create_svp_input_buffer(int index)
    {
        td_s32 ret;
        td_ulong class_id;
        svp_acl_data_buffer *input_data = TD_NULL;
        td_void *input_buffer = TD_NULL;
        size_t buffer_size, stride;

        stride = svp_acl_mdl_get_input_default_stride(m_model_desc,index);
        buffer_size = svp_acl_mdl_get_input_size_by_index(m_model_desc,index);
        ret = svp_acl_mdl_get_input_class_id_by_index(m_model_desc,index,&class_id);
        if(stride == 0
                || (buffer_size == 0 || buffer_size > SAMPLE_SVP_NPU_MAX_MEM_SIZE)
                || ret != SVP_ACL_SUCCESS)
        {
            DEV_WRITE_LOG_ERROR("svp_acl_mdl_get_input_info failed with 0x%x",ret);
            return NULL;
        }

        if(class_id != 0)
        {
            ret = svp_acl_rt_malloc_sec(&input_buffer, buffer_size, class_id);
        }
        else
        {
            ret = svp_acl_rt_malloc(&input_buffer, buffer_size, SVP_ACL_MEM_MALLOC_NORMAL_ONLY); 
            if(ret == SVP_ACL_SUCCESS)
            {
                memset(input_buffer,0,buffer_size);
            }
        }
        if(ret != SVP_ACL_SUCCESS)
        {
            DEV_WRITE_LOG_ERROR("svp_acl_mdl_rt_malloc failed with 0x%x",ret);
            return NULL;
        }

        input_data = svp_acl_create_data_buffer(input_buffer, buffer_size, stride);
        if(input_data == NULL)
        {
            DEV_WRITE_LOG_ERROR("svp_acl_create_data_buffer failed");
            if(class_id != 0)
            {
                svp_acl_rt_free_sec(input_buffer);
            }
            else
            {
                svp_acl_rt_free(input_buffer);
            }
            return NULL;
        }

        if(index == m_input_num - SAMPLE_SVP_NPU_EXTRA_INPUT_NUM)
        {
            m_task_info.task_buf_ptr = input_buffer;
            m_task_info.task_buf_size = buffer_size;
            m_task_info.task_buf_stride = stride;
        }else if(index == m_input_num - 1)
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
        int i;
        svp_acl_data_buffer *input_data = TD_NULL;

        svp_acl_mdl_dataset* input_dataset = svp_acl_mdl_create_dataset();
        if(input_dataset == TD_NULL)
        {
            DEV_WRITE_LOG_ERROR("svp_acl_mdl_create_dataset failed ");
            return false;
        }

        for(i = 0; i < m_input_num - SAMPLE_SVP_NPU_EXTRA_INPUT_NUM; i++)
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
        input_data =  create_svp_input_buffer(m_input_num - SAMPLE_SVP_NPU_EXTRA_INPUT_NUM);
        assert(input_data != NULL);
        svp_acl_mdl_add_dataset_buffer(input_dataset,input_data);

        //workbuf
        input_data = create_svp_input_buffer(m_input_num - 1);
        assert(input_data != NULL);
        svp_acl_mdl_add_dataset_buffer(input_dataset,input_data);

        m_task_info.input_dataset = input_dataset;
        return true;
    }

    bool yolov5::create_vpss_grp_chn()
    {
        td_s32 ret;
        
        ret = ss_mpi_vpss_set_chn_attr(m_vpss_grp, m_vpss_chn, &m_vpss_chn_attr);
        if (ret != TD_SUCCESS) 
        {
            DEV_WRITE_LOG_ERROR("ss_mpi_vpss_set_chn_attr failed with %#x", ret);
            return false;
        }
        ret = ss_mpi_vpss_enable_chn(m_vpss_grp, m_vpss_chn);
        if (ret != TD_SUCCESS) 
        {
            DEV_WRITE_LOG_ERROR("ss_mpi_vpss_enable_chn_attr failed with %#x", ret);
            return false;
        }

        return true;
    }

    void yolov5::destroy_vpss_grp_chn()
    {
        ss_mpi_vpss_disable_chn(m_vpss_grp, m_vpss_chn);
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

        m_vpss_chn_attr.width = m_pic_size.width;
        m_vpss_chn_attr.height = m_pic_size.height;
        m_vpss_grp = m_vi_ptr->vpss_grp();
        m_vpss_chn = 1;

        if(!create_vpss_grp_chn())
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
        return true;
end3:
        destroy_svp_input();
        destroy_svp_output();
end2:
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

        destroy_vpss_grp_chn();

        destroy_svp_output();
        destroy_svp_input();

        svp_acl_mdl_unload(m_model_id);
        svp_acl_mdl_destroy_desc(m_model_desc);
        svp_acl_rt_free(m_model_mem_ptr);
    }

}}//namespace
