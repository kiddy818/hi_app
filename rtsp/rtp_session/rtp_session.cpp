#include "rtp_session.h"

namespace beacon{namespace rtsp{

    rtp_session::rtp_session()
    {
        m_rtcp_timeout = MAX_RTCP_TIMEOUT;
    }

    rtp_session::~rtp_session()
    {
    }

}}//namespace

