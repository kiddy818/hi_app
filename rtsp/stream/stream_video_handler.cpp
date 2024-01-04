#include "stream_video_handler.h"
namespace ceanic{namespace rtsp{

    stream_video_handler::stream_video_handler(rtp_session_ptr session_ptr, rtp_serialize_ptr serialize_ptr)
        :m_rtp_session(session_ptr), m_rtp_serialize(serialize_ptr)
    {
    }

    stream_video_handler::~stream_video_handler()
    {
        stop();
    }

    bool stream_video_handler::start()
    {
        if (is_start())
        {
            return false;
        }

        if (!m_rtp_serialize || !m_rtp_session)
        {
            return false;
        }

        m_beg = time(NULL);
        m_start = true;
        return true;
    }

    void stream_video_handler::stop()
    {
        if (is_start())
        {
            m_start = false;
        }
    }

    int& stream_video_handler::get_rtcp_timeout()
    {
        return m_rtp_session->rtcp_timeout();
    }

    bool stream_video_handler::process_stream(util::stream_head* head, const char* data, int len)
    {
        if (!is_start())
        {
            return false;
        }

        if (!IS_VIDEO_FRAME(head->type))
        {
            return false;
        }

        if (!m_rtp_serialize->serialize(*head, data, len, m_rtp_session))
        {
            return false;
        }

        return true;
    }

}}//namespace

