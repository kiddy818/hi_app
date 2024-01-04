#include "session.h"
#include <rtsp_log.h>

namespace ceanic{namespace rtsp{

#define MAX_EVBUFFER_LEN (1024 * 1024)
    session::session(int s, int timeout)
        :m_socket(s), m_start(false), m_timeout(timeout), m_out_buf(NULL)
    {
        struct sockaddr soad;
        struct sockaddr_in in;
        int soad_len = sizeof(soad);
        if (getpeername(s,&soad,(socklen_t*)&soad_len) == 0)
        {
            memcpy(&in,&soad, sizeof(soad));
            m_ip = inet_ntoa(in.sin_addr);
        }
    }

    session::~session()
    {
        close(m_socket);

        if (m_out_buf)
        {
            evbuffer_drain(m_out_buf,-1);
            evbuffer_free(m_out_buf);

            m_out_buf = NULL;
        }
    }

    int session::socket()
    {
        return m_socket;
    }

    std::string& session::ip()
    {
        return m_ip;
    }

    void session::handle_reset()
    {
    }

    void session::on_idle()
    {
        std::unique_lock<std::mutex> lock(m_out_buf_mu);

        if (m_out_buf == NULL)
        {
            return;
        }

        int len_before = evbuffer_get_length(m_out_buf);
        if (len_before == 0)
        {
            return;
        }

        evbuffer_write(m_out_buf, m_socket);
    }

    bool session::send_packet_n(const char* buf, int buf_len)
    {
        std::unique_lock<std::mutex> lock(m_out_buf_mu);

        if (m_out_buf == NULL)
        {
            m_out_buf = evbuffer_new();
        }

        int evlen = evbuffer_get_length(m_out_buf);
        if (evlen + buf_len >= MAX_EVBUFFER_LEN)
        {
            RTSP_WRITE_LOG_WARN("overflow");
            return false;
        }

        evbuffer_add(m_out_buf, buf, buf_len);

        return true;
    }

}}//namespace

