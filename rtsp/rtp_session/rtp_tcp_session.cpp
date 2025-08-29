#include "rtp_tcp_session.h"

namespace ceanic{namespace rtsp{

    rtp_tcp_session::rtp_tcp_session(session& sess, int32_t rtp_id, int32_t rtcp_id)
        :m_rtp_id(rtp_id), m_rtcp_id(rtcp_id), m_err(false), m_sess(sess)
    {
        struct sockaddr soad;
        int32_t soad_len = sizeof(soad);
        if (getpeername(m_sess.socket(),&soad,(socklen_t*)&soad_len) == 0)
        {
            int32_t snd_buf = 512 * 1024;
            if (setsockopt(m_sess.socket(), SOL_SOCKET, SO_SNDBUF,&snd_buf, sizeof(snd_buf)) != 0)
            {
                fprintf(stderr,"set rtp tcp session snd buf failed\n");
            }
        }
    }

    rtp_tcp_session::~rtp_tcp_session()
    {
    }

    bool rtp_tcp_session::send_packet(rtp_packet_t* packet)
    {
        if (m_err)
        {
            return false;
        }

        packet->tcp_tag[0] = '$';//开始符号
        packet->tcp_tag[1] = m_rtp_id;//现在只发rtp数据，如果发送rtcp的话，id为m_rtcp_id_
        packet->tcp_tag[2] = (char)((packet->rtp_data_len & 0xFF00) >> 8);
        packet->tcp_tag[3] = (char)(packet->rtp_data_len & 0xff);
        if (m_sess.send_rtp_packet(packet))
        {
            //TCP发送情况下，默认都收到RTCP包
            m_rtcp_timeout = MAX_RTCP_TIMEOUT;
            return true;
        }
        else
        {
            m_err = true;
            shutdown(m_sess.socket(), SHUT_RDWR);
            return false;
        }
    }

}}//namespace

