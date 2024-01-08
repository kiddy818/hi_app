#include "stream_stock.h"
#include <dlfcn.h>
#include <map>
#include <util/std.h>
#include <device/dev_chn.h>

namespace ceanic{namespace rtsp{

    stream_stock::stream_stock(int chn,int stream_id)
        :m_chn(chn),m_stream_id(stream_id)
    {
        memset(&m_media_head,0,sizeof(m_media_head));
        m_media_head.vdec = util::STREAM_ENCODE_H264;
    }

    stream_stock::~stream_stock()
    {
        stop();
    }

    bool stream_stock::start()
    {
        if (is_start())
        {
            return false;
        }

        m_last_stream_time = time(NULL);
        m_stream_len = 0;

        m_is_start = true;
        return true;
    }

    void stream_stock::stop()
    {
        if (m_is_start)
        {
            m_is_start = false;
        }
    }

    bool stream_stock::get_stream_type(int* stream_type)
    {
        if(!is_start())
        {
            return false;
        }

        *stream_type = 1;
        return true;
    }

    bool stream_stock::request_i_frame()
    {
        if(m_is_start)
        {
            return hisilicon::dev::chn::request_i_frame(m_chn,m_stream_id);
        }

        return false;
    }

    bool stream_stock::get_stream_head(util::media_head* mh)
    {
        if(!m_is_start)
        {
            return false;
        }

        return hisilicon::dev::chn::get_stream_head(m_chn,m_stream_id,mh);
    }

    void stream_stock::process_data(util::stream_head* head,const char* buf,int len)
    {
        time_t now = time(NULL);
        if(now != m_last_stream_time)
        {
            m_last_stream_time = now;
            m_per_sec_len = m_stream_len / 1024;
            m_stream_len = 0;
        }
        m_stream_len += len;

        if(!is_start())
        {
            return ;
        }

        post_stream_to_observer(head,buf,len);
    }

}}//namespace

