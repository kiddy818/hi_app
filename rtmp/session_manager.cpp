#include <util/std.h>
#include <rtmp/session_manager.h>
#include <rtmp/rtmp_log.h>

namespace ceanic{namespace rtmp{

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
        for(auto it = m_sess.begin(); it != m_sess.end(); it++)
        {
            sess_key_ptr key = it->first;
            sess_ptr sess = it->second;

            if(key->chn == chn 
                    && key->stream_id == stream_id
                    && key->url == url)
            {
                return true;
            }
        }

        return false;
    }

    bool session_manager::create_session(int32_t chn,int32_t stream_id,std::string url)
    {
        std::unique_lock<std::mutex> lock(m_sess_mu);

        if(session_exists(chn,stream_id,url))
        {
            return false;
        }

        //sess_ptr sess = std::make_shared<timed_session>(url,7200000);
        sess_ptr sess = std::make_shared<session>(url);
        if(!sess->start())
        {
            return false;
        }

        sess_key_ptr sess_key = std::make_shared<sess_key_t>();
        sess_key->chn = chn;
        sess_key->stream_id = stream_id;
        sess_key->url = url;

        m_sess.insert(std::make_pair(sess_key,sess));
        return true;
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

    void session_manager::process_data(int32_t chn,int32_t stream_id,util::stream_head* head)
    {
        std::unique_lock<std::mutex> lock(m_sess_mu);

        for(auto it = m_sess.begin();it != m_sess.end();)
        {
            sess_key_ptr key = it->first;
            sess_ptr sess = it->second;

            if(key->chn == chn
                    && key->stream_id == stream_id)
            {
                bool send_success = true;
                for(auto i = 0; i < head->nalu_count; i++)
                {
                    if(!sess->input_one_nalu((uint8_t*)head->nalu[i].data,head->nalu[i].size,head->nalu[i].timestamp))
                    {
                        send_success = false;
                        break;
                    }
                }

                if(!send_success)
                {
                    sess->stop();
                    it = m_sess.erase(it);
                    continue;
                }
            }

            it++;
        }
    }

}}
