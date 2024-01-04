#include "stream_handler.h"

namespace ceanic{namespace rtsp{

    stream_handler::stream_handler()
    {
        m_start = false;
    }

    stream_handler::~stream_handler()
    {
    }

    void stream_handler::on_stream_come(util::stream_head* head, const char* buf, int len)
    {
        if (is_start())
        {
            process_stream(head, buf, len);
        }
    }

    void stream_handler::on_stream_error(int errno)
    {
    }

}}//namespace

