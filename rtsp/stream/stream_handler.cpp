#include "stream_handler.h"

namespace ceanic{namespace rtsp{

    stream_handler::stream_handler()
    {
        m_start = false;
    }

    stream_handler::~stream_handler()
    {
    }

    void stream_handler::on_stream_come(util::stream_obj_ptr sobj,util::stream_head* head, const char* buf, int32_t len)
    {
        if (is_start())
        {
            process_stream(sobj,head, buf, len);
        }
    }

    void stream_handler::on_stream_error(util::stream_obj_ptr sobj,int32_t error)
    {
    }

}}//namespace

