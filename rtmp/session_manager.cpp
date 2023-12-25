#include <util/std.h>
#include <rtmp/session_manager.h>
#include <rtmp/rtmp_log.h>

namespace beacon{namespace rtmp{

    session_manager::session_manager()
    {
    }

    session_manager* session_manager::instance()
    {
        static session_manager* g_sm = nullptr;
        if(g_sm == nullptr)
        {
            g_sm = new session_manager();
        }

        return g_sm;
    }

    bool session_manager::session_exists(int32_t chn,int32_t stream_id,std::string url)
    {
        std::unique_lock<std::mutex> lock(m_sess_mu);
        for(auto it = m_sess.begin(); it != m_sess.end(); it++)
        {
            sess_key_ptr key = it->first;
            sess_ptr sess = it->second;

            if(key->chn == chn 
                    && key->stream_id == stream_id
                    && key->url == url)
            {
                return false;
            }
        }

        return true;
    }

    bool session_manager::reset_session_tm(int32_t chn,int32_t stream_id,std::string url,uint32_t max_tm)
    {
        std::unique_lock<std::mutex> lock(m_sess_mu);
        for(auto it = m_sess.begin(); it != m_sess.end();it++)
        {
            sess_key_ptr key = it->first;
            sess_ptr sess = it->second;

            if(key->chn == chn 
                    && key->stream_id == stream_id
                    && key->url == url)
            {
                sess->reset_max_tm(max_tm);
                return true;
            }
        }

        return false;
    }

    bool session_manager::create_session(int32_t chn,int32_t stream_id,std::string url,uint32_t max_tm)
    {
        if(session_exists(chn,stream_id,url))
        {
            return false;
        }

        sess_ptr sess = std::make_shared<timed_session>(url,max_tm);
        if(!sess->start())
        {
            return false;
        }

        std::unique_lock<std::mutex> lock(m_sess_mu);

        if(!session_exists(chn,stream_id,url))
        {
            sess_key_ptr sess_key = std::make_shared<sess_key_t>();
            sess_key->chn = chn;
            sess_key->stream_id = stream_id;
            sess_key->url = url;

            m_sess.insert(std::make_pair(sess_key,sess));
            return true;
        }

        return false;
    }

    void session_manager::delete_session(int32_t chn,int32_t stream_id,std::string url)
    {
        std::unique_lock<std::mutex> lock(m_sess_mu);
        for(auto it = m_sess.begin(); it != m_sess.end();)
        {
            sess_key_ptr key = it->first;
            sess_ptr sess = it->second;

            if(key->chn == chn 
                    && key->stream_id == stream_id
                    && key->url == url)
            {
                sess->stop();
                it = m_sess.erase(it);
            }
            else
            {
                it++;
            }
        }
    }

    void session_manager::delete_session(int32_t chn,int32_t stream_id)
    {
        std::unique_lock<std::mutex> lock(m_sess_mu);
        for(auto it = m_sess.begin(); it != m_sess.end();)
        {
            sess_key_ptr key = it->first;
            sess_ptr sess = it->second;

            if(key->chn == chn 
                    && key->stream_id == stream_id)
            {
                sess->stop();
                it = m_sess.erase(it);
            }
            else
            {
                it++;
            }
        }
    }

    void session_manager::process_data(int32_t chn,int32_t stream_id,util::stream_head* head,const uint8_t* buf,int32_t len)
    {
        std::unique_lock<std::mutex> lock(m_sess_mu);

        for(auto it = m_sess.begin();it != m_sess.end();)
        {
            sess_key_ptr key = it->first;
            sess_ptr sess = it->second;

            if(key->chn == chn
                    && key->stream_id == stream_id)
            {
                auto ret = sess->input_one_nalu(buf,len,head->time_stamp);

                if(ret != timed_session::TIMED_SESSION_SUCCESS)
                {
                    if(ret == timed_session::TIMED_SESSION_OUT_OF_TIME)
                    {
                        RTMP_WRITE_LOG_INFO("chn:%d,stream:%d,url:%s out of time,auto deleted",key->chn,key->stream_id,key->url.c_str());
                    }
                    else
                    {
                        RTMP_WRITE_LOG_INFO("chn:%d,stream:%d,url:%s unexcepted error,auto deleted",key->chn,key->stream_id,key->url.c_str());
                    }

                    sess->stop();
                    it = m_sess.erase(it);
                }
                else
                {
                    it++;
                }
            }
        }
    }

}}
