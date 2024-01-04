#include "rtp_tcp_session.h"

namespace ceanic{namespace rtsp{

    rtp_tcp_session::rtp_tcp_session(session& sess, int rtp_id, int rtcp_id)
        :m_rtp_id(rtp_id), m_rtcp_id(rtcp_id), m_err(false), m_sess(sess)
    {
        struct sockaddr soad;
        int soad_len = sizeof(soad);
        if (getpeername(m_sess.socket(),&soad,(socklen_t*)&soad_len) == 0)
        {
            int snd_buf = 512 * 1024;
            if (setsockopt(m_sess.socket(), SOL_SOCKET, SO_SNDBUF,&snd_buf, sizeof(snd_buf)) != 0)
            {
                fprintf(stderr,"set rtp tcp session snd buf failed\n");
            }
        }
    }

    rtp_tcp_session::~rtp_tcp_session()
    {
    }

    bool rtp_tcp_session::send_packet(rtp_packet* packet)
    {
        if (m_err)
        {
            return false;
        }

        char buf[4096];

        buf[0] = '$';//开始符号
        buf[1] = m_rtp_id;//现在只发rtp数据，如果发送rtcp的话，id为m_rtcp_id_
        buf[2] = (char)((packet->len & 0xFF00) >> 8);
        buf[3] = (char)(packet->len & 0xff);
        memcpy(buf + 4, packet->data, packet->len);

        if (m_sess.send_packet_n(buf, packet->len + 4))
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

