#include "h264_mp4_save.h"

namespace ceanic{namespace stream_save{

    h264_mp4_save::h264_mp4_save(const char* file_path,int w,int h,int fr)
        :m_bopen(false),m_file_path(file_path),m_mp4_fh(MP4_INVALID_FILE_HANDLE),m_mp4_track_id(MP4_INVALID_TRACK_ID),m_sbuf(1024 * 1024),m_w(w),m_h(h),m_fr(fr)
    {
    }

    h264_mp4_save::~h264_mp4_save()
    {
        assert(!m_bopen);
    }

    bool h264_mp4_save::open()
    {
        std::unique_lock<std::mutex> lock(m_interface_mu);

        if(m_bopen)
        {
            return false;
        }

        m_mp4_fh = MP4Create(m_file_path.c_str(),0);
        MP4SetTimeScale(m_mp4_fh, 90000);

        m_bopen = true;
        m_process_thread = std::thread(&h264_mp4_save::on_process,this);

        return true;
    }

    void h264_mp4_save::process_h264_nalue(const char* buf,int len)
    {
        const char* p_nalu_buf = buf + 4;
        int nalu_len = len - 4;
        int es_type = buf[4] & 0x1f;

        switch(es_type)
        {   
            case 0x7:
                {   
                    //sps
                    if(m_mp4_track_id == MP4_INVALID_TRACK_ID)
                    {   
                        m_mp4_track_id = MP4AddH264VideoTrack(m_mp4_fh, 90000, 90000 / m_fr,m_w,m_h, p_nalu_buf[1], p_nalu_buf[2], p_nalu_buf[3], 3); 

                        if(m_mp4_track_id == MP4_INVALID_TRACK_ID)
                        {   
                            printf("[%s]:sps:invalid mp4 track invalid\n",__FUNCTION__);
                            return ;
                        }   

                        MP4SetVideoProfileLevel (m_mp4_fh,0x7f);
                        MP4AddH264SequenceParameterSet(m_mp4_fh,m_mp4_track_id,(uint8_t*)p_nalu_buf,nalu_len);
                    }

                    break;
                }

            case 0x8:
                {
                    if(m_mp4_track_id == MP4_INVALID_TRACK_ID)
                    {
                        printf("[%s]:pps:invalid mp4 track invalid\n",__FUNCTION__);
                        return ;
                    }

                    MP4AddH264PictureParameterSet(m_mp4_fh,m_mp4_track_id,(uint8_t*)p_nalu_buf,nalu_len);
                    break;
                }

            case 0x1://p
            case 0x5://i
                {
                    if(m_mp4_track_id == MP4_INVALID_TRACK_ID)
                    {
                        printf("[%s]:i/p invalid mp4 track invalid,wait sps/pps\n",__FUNCTION__);
                        return ;
                    }

                    int mp4_data_len = nalu_len + 4;
                    unsigned char* p_mp4_data = (unsigned char*)buf;

                    p_mp4_data[0] = nalu_len  >> 24;
                    p_mp4_data[1] = nalu_len >> 16;
                    p_mp4_data[2] = nalu_len >> 8;
                    p_mp4_data[3] = nalu_len & 0xff;

                    if (!MP4WriteSample(m_mp4_fh, m_mp4_track_id, p_mp4_data, mp4_data_len, MP4_INVALID_DURATION, 0, 1))
                    {
                        printf("[%s]:MP4WriteSample failed\n",__FUNCTION__);
                        return;
                    }
                    break;
                }
               default:
                {
                    printf("[%s]:invalid nalu type:%d\n",__FUNCTION__,es_type);
                    return;
                }
        }
    }

    void h264_mp4_save::on_process()
    {
        bool get_data = false;
        int nalu_size = 0;
        char* nalu_data = new char[1024*1024];

        while(m_bopen)
        {
            get_data = false;

            {
                std::unique_lock<std::mutex> sl(m_buf_mu);
                if(m_sbuf.get_stream_len() >= 4
                        && m_sbuf.copy_data(&nalu_size,4)
                        && m_sbuf.get_stream_len() > 4 + nalu_size)
                {
                    m_sbuf.get_data(&nalu_size,4);
                    m_sbuf.get_data(nalu_data,nalu_size);
                    get_data = true;
                }
            }

            if(!get_data)
            {
                usleep(10000);
                continue;
            }

            //printf("[%s]:get nalu len:%d\n)",__FUNCTION__,nalu_size);
            process_h264_nalue(nalu_data,nalu_size);
        }

        delete[] nalu_data;
    }

    void h264_mp4_save::close()
    {
        std::unique_lock<std::mutex> lock(m_interface_mu);

        if(!m_bopen)
        {
            return ;
        }

        m_bopen = false;
        m_process_thread.join();

        MP4Close(m_mp4_fh,0);
        m_mp4_fh = MP4_INVALID_FILE_HANDLE;
    }

    bool h264_mp4_save::is_open()
    {
        std::unique_lock<std::mutex> lock(m_interface_mu);

        return m_bopen;
    }

    bool h264_mp4_save::input_data(ceanic::util::stream_head* head,const char* buf,int len)
    {
        std::unique_lock<std::mutex> lock(m_interface_mu);

        if(!m_bopen)
        {
            return false;
        }

        {
            std::unique_lock<std::mutex> sl(m_buf_mu);
            for(int i = 0; i < head->nalu_count; i++)
            {
                if(m_sbuf.get_remain_len() < 4 + head->nalu[i].size)
                {
                    printf("[%s]:buf is full\n",__FUNCTION__);
                    return false;
                }

                m_sbuf.input_data(&head->nalu[i].size,4);
                m_sbuf.input_data((void*)head->nalu[i].data,head->nalu[i].size);
            }
        }

        return true;
    }

}}//namespace

