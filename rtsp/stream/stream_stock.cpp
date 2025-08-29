#include "stream_stock.h"
#include <dlfcn.h>
#include <map>
#include <util/std.h>

namespace ceanic{namespace rtsp{

    stream_stock::stream_stock(int32_t chn,int32_t stream_id)
        :stream(chn,stream_id)
    {
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

    void stream_stock::process_data(util::stream_head* head,const char* buf,int32_t len)
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

        post_stream_to_observer(shared_from_this(),head,buf,len);
    }

}}//namespace

