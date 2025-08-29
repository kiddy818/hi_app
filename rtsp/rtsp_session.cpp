#include "rtsp_session.h"
#include <rtsp_log.h>

namespace ceanic{namespace rtsp{

    rtsp_session::rtsp_session(int32_t s, int32_t timeout)
        :session(s, timeout)
    {
    }

    rtsp_session::~rtsp_session()
    {
    }

    void rtsp_session::reduce_session_timeout()
    {
        if (m_timeout > 0)
        {
            m_timeout--;
        }
    }

    int32_t rtsp_session::get_session_timeout()
    {
        return m_timeout;
    }

    void rtsp_session::reduce_rtcp_timeout()
    {
        stream_handler_ptr sh;
        if (m_handler.get_video_handle(sh))
        {
            if (sh ->get_rtcp_timeout() > 0)
            {
                sh->get_rtcp_timeout()--;
            }
        }
    }

    int32_t rtsp_session::get_rtcp_timeout()
    {
        stream_handler_ptr sh;
        if (m_handler.get_video_handle(sh))
        {
            return sh->get_rtcp_timeout();
        }
        return 0;
    }

    bool rtsp_session::start()
    {
        m_start = true;

        return true;
    }

    void rtsp_session::stop()
    {
        if (!is_start())
        {
            return;
        }

        m_start = false;
    }

    void rtsp_session::handle_reset()
    {
        m_parser.reset();
        m_request.data_len = 0;
        m_request.head_flag = false;
        m_request.content_len = 0;
        m_request.method.clear();    
        m_request.uri.clear();    
        m_request.headers.clear();    
    }

    void rtsp_session::process_rtsp_request()
    {
        m_handler.handle_request(m_request, *this);

        m_timeout = MAX_SESSION_TIMEOUT;
    }

    std::optional<bool> rtsp_session::handle_read(const char* data, int32_t len)
    {
        int32_t left = len;
        std::optional<bool> result;
        while (left > 0)
        {
            result = m_parser.parse(m_request, data, len,&left);
            if (result.has_value() && result.value())
            {
                //process request
                process_rtsp_request();

                //prepare for next protocol
                handle_reset();

                data += (len - left);
                len = left;
            }
            else if (result.has_value() && !result.value())
            {
                RtspState state = m_handler.state();
                if (state == RTSP_STATE_PLAYING)
                {
                    if (!m_request.method.empty()
                            && m_request.method[0] == '$')
                    {
                        //printf("-----------get rtcp data, len %d-------------\n", len);
                    }
                    else
                    {
                        //printf("-----------parse protocol error: len %d-------------\n", len);
                    }

                    //因为还没有解析rtcp,所以如果客户端发送了rtcp数据，会导致
                    //协议解析错误，必须清空
                    handle_reset();
                }
                else
                {
                    RTSP_WRITE_LOG_ERROR("protocol error, shutdown socket(%d)",m_socket);
                    shutdown(m_socket, SHUT_RDWR);
                }

                return result;
            }
            else
            {
                RTSP_WRITE_LOG_INFO("protocol need more data,method %s",m_request.method.c_str());
                //need more data
            }
        }

        return result;
    }

}}//namespace
