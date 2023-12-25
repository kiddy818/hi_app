#include "stream_manager.h"
#include <util/std.h>
#include <iostream>
#include <rtsp_log.h>

namespace beacon{namespace rtsp{

    stream_manager* stream_manager::g_instance = NULL;

    stream_manager::stream_manager()
        :m_stream_checking(false)
    {
    }

    stream_manager::~stream_manager()
    {
        stop_stream_check();
    }

    bool stream_manager::start_stream_check()
    {
        if (m_stream_checking)
        {
            return false;
        }

        m_thread = std::thread(&stream_manager::on_checking, this);
        m_stream_checking = true;
        return true;
    }

    void stream_manager::stop_stream_check()
    {
        if (m_stream_checking)
        {
            m_stream_checking = false;
            m_thread.join();
        }
    }

    void stream_manager::on_checking()
    {
        time_t t = time(NULL);
        time_t now;
        while (m_stream_checking)
        {
            now = time(NULL);    
            if (t != now)
            {
                t = now;

                std::unique_lock<std::mutex> lock(m_stream_mu);
                std::list<stream_ptr>::iterator it;
                for (it = m_streams.begin();it != m_streams.end(); it++)
                {
                    if (abs((*it)->last_stream_time() - now) > 5)
                    {
                        RTSP_WRITE_LOG_WARN("stream(chn=%d,stream=%d) dead",(*it)->chn(),(*it)->stream_id());
                        m_streams.erase(it);
                        break;
                    }
                }
            }
            else
            {
                usleep(10000);
            }
        }
    }

    bool stream_manager::get_stream(int chn,int stream_id, stream_ptr& stream)
    {
        std::unique_lock<std::mutex> lock(m_stream_mu);

        std::list<stream_ptr>::iterator it;
        for (it = m_streams.begin();it != m_streams.end(); it++)
        {
            if ((*it)->chn() == chn 
                    && (*it)->stream_id() == stream_id)
            {
                stream = *it;
                RTSP_WRITE_LOG_INFO("get_stream(chn=%d,stream=%d) success,observer size=%d",chn,stream_id,(*it)->get_observer_size());
                return true;
            }
        }

        stream = std::make_shared<stream_stock>(chn,stream_id);
        if (stream->start())
        {
            RTSP_WRITE_LOG_INFO("get_stream(chn=%d,stream=%d) success",chn,stream_id);
            m_streams.push_back(stream);
            return true;
        }

        RTSP_WRITE_LOG_INFO("get_stream(chn=%d,stream=%d) failed",chn,stream_id);
        return false;
    }

    bool stream_manager::del_stream(int chn,int stream_id)
    {
        std::unique_lock<std::mutex> lock(m_stream_mu);

        std::list<stream_ptr>::iterator it;
        for (it = m_streams.begin();it != m_streams.end(); it++)
        {
            if ((*it)->chn() == chn 
                    && (*it)->stream_id() == stream_id)
            {
                if ((*it)->get_observer_size() == 0)
                {
                    (*it)->stop();
                    m_streams.erase(it);
                    RTSP_WRITE_LOG_INFO("del_stream(chn=%d,stream=%d) success",chn,stream_id);
                    return true;
                }

                RTSP_WRITE_LOG_INFO("del_stream(chn=%d,stream=%d) failed (observer size=%d)",chn,stream_id,(*it)->get_observer_size());
                return false;
            }
        }

        RTSP_WRITE_LOG_WARN("del_stream(chn=%d,stream=%d) failed (stream not found)",chn,stream_id);
        return false;
    }

    stream_manager* stream_manager::instance()
    {
        if (g_instance == NULL)
        {
            g_instance = new stream_manager();
            g_instance->start_stream_check();
        }

        return g_instance;
    }

    bool stream_manager::process_data(int chn,int stream_id,util::stream_head* head,const char* buf,int len)
    {
        std::unique_lock<std::mutex> lock(m_stream_mu);

        std::list<stream_ptr>::iterator it;
        for(it = m_streams.begin();it != m_streams.end(); it++)
        {
            if((*it)->chn() == chn
                    && (*it)->stream_id() == stream_id)
            {
                (*it)->process_data(head,buf,len);
            }
        }

        return true;
    }

}}//namespace


