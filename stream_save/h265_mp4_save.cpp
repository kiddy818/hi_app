#include "h265_mp4_save.h"
#include "MP4Writer.h"

namespace ceanic{namespace stream_save{

    h265_mp4_save::h265_mp4_save(const char* file_path)
        :m_bopen(false),m_file_path(file_path),m_gpac_fh(NULL),m_sbuf(1024 * 1024),m_vps_got(false)
    {
    }

    h265_mp4_save::~h265_mp4_save()
    {
        assert(!m_bopen);
    }

    bool h265_mp4_save::open()
    {
        std::unique_lock<std::mutex> lock(m_interface_mu);

        if(m_bopen)
        {
            return false;
        }

        m_gpac_fh = MP4_Init();
        if(MP4_CreatFile(m_gpac_fh,(char*)m_file_path.c_str()) != 0)
        {
            return false;
        }

        if(MP4_InitVideo265(m_gpac_fh,90000) != 0)
        {
            return false;
        }

        m_vps_got = false;
        m_bopen = true;
        m_process_thread = std::thread(&h265_mp4_save::on_process,this);

        return true;
    }

    void h265_mp4_save::process_h265_nalue(const char* buf,int len,uint32_t timestamp)
    {
        int h265_nalu_type = (buf[4] >> 1) & 0x3f;
        if(m_vps_got == false && h265_nalu_type != 32)
        {
            printf("[%s]:wait for vps\n",__FUNCTION__);
            return;
        }

        if(h265_nalu_type == 32)
        {
            m_vps_got = true;
        }

        if(MP4_Write265Sample(m_gpac_fh,(u8*)buf,len,timestamp * 90) != 0)
        {
            printf("[%s]:write h265 failed\n",__FUNCTION__);
        }
    }

    void h265_mp4_save::on_process()
    {
        bool get_data = false;
        char* nalu_data = new char[1024*1024];
        h265_sbuf_head_t hsh;

        while(m_bopen)
        {
            get_data = false;

            {
                std::unique_lock<std::mutex> sl(m_buf_mu);
                if(m_sbuf.get_stream_len() >= sizeof(hsh)
                        && m_sbuf.copy_data(&hsh,sizeof(hsh))
                        && m_sbuf.get_stream_len() >= sizeof(hsh) + hsh.len)
                {
                    m_sbuf.get_data(&hsh,sizeof(hsh));
                    m_sbuf.get_data(nalu_data,hsh.len);
                    get_data = true;
                }
            }

            if(!get_data)
            {
                usleep(10000);
                continue;
            }

            //printf("[%s]:get nalu len:%d\n)",__FUNCTION__,hsh.len);
            process_h265_nalue(nalu_data,hsh.len,hsh.timestamp);
        }

        delete[] nalu_data;
    }

    void h265_mp4_save::close()
    {
        std::unique_lock<std::mutex> lock(m_interface_mu);

        if(!m_bopen)
        {
            return ;
        }

        m_bopen = false;
        m_process_thread.join();

        MP4_CloseFile(m_gpac_fh);
        m_gpac_fh = NULL;
    }

    bool h265_mp4_save::is_open()
    {
        std::unique_lock<std::mutex> lock(m_interface_mu);

        return m_bopen;
    }

    bool h265_mp4_save::input_data(ceanic::util::stream_head* head,const char* buf,int len)
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
                if(m_sbuf.get_remain_len() < sizeof(h265_sbuf_head_t) + head->nalu[i].size)
                {
                    printf("[%s]:buf is full\n",__FUNCTION__);
                    return false;
                }
                
                h265_sbuf_head_t hsh;
                hsh.len = head->nalu[i].size;
                hsh.timestamp = head->nalu[i].timestamp;

                m_sbuf.input_data(&hsh,sizeof(hsh));
                m_sbuf.input_data((void*)head->nalu[i].data,head->nalu[i].size);
            }
        }

        return true;
    }

}}//namespace

