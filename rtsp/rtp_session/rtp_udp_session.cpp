#include "rtp_udp_session.h"

namespace ceanic{namespace rtsp{

    rtp_udp_session::rtp_udp_session(const char* remote_ip, short remote_rtp_port, short remote_rtcp_port, const char* local_ip, short local_rtp_port, short local_rtcp_port)
        :m_remote_ip(remote_ip), m_remote_rtp_port(remote_rtp_port), m_remote_rtcp_port(remote_rtcp_port), m_local_rtp_port(local_rtp_port), m_local_rtcp_port(local_rtcp_port)
    {
        m_rtp_socket = socket(AF_INET, SOCK_DGRAM, 0);
        m_rtcp_socket = socket(AF_INET, SOCK_DGRAM, 0);

        int reuseaddr = 1;
        setsockopt(m_rtp_socket, SOL_SOCKET, SO_REUSEADDR,(char*)&reuseaddr, sizeof(int));
        setsockopt(m_rtcp_socket, SOL_SOCKET, SO_REUSEADDR,(char*)&reuseaddr, sizeof(int));

        if (m_rtp_socket != -1)
        {
            int snd_buf = 512 * 1024;
            if (setsockopt(m_rtp_socket, SOL_SOCKET, SO_SNDBUF,&snd_buf, sizeof(snd_buf)) != 0)
            {
                printf("set udp snd buf failed\n");
            }
        }

        memset(&m_dst_addr, 0, sizeof(m_dst_addr));
        m_dst_addr.sin_family = AF_INET;
        m_dst_addr.sin_port = htons(m_remote_rtp_port);
        m_dst_addr.sin_addr.s_addr = inet_addr(m_remote_ip.c_str());

        struct sockaddr_in bind_addr;
        memset(&bind_addr, 0, sizeof(bind_addr));
        bind_addr.sin_family = AF_INET;
        bind_addr.sin_port = htons(m_local_rtp_port);
        bind_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        if (bind(m_rtp_socket,(struct sockaddr*)&bind_addr, sizeof(struct sockaddr_in)) != 0)
        {
            printf("-------------bind rtp sock failed-------------\n");
        }

        memset(&bind_addr, 0, sizeof(bind_addr));
        bind_addr.sin_family = AF_INET;
        bind_addr.sin_port = htons(m_local_rtcp_port);
        bind_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        if (bind(m_rtcp_socket,(struct sockaddr*)&bind_addr, sizeof(struct sockaddr_in)) != 0)
        {
            printf("--------------bind rtcp sock failed-----------\n");
        }

        /*set nonblock*/
        int val = fcntl(m_rtcp_socket, F_GETFL, 0);
        fcntl(m_rtcp_socket, F_SETFL, val | O_NONBLOCK);
    }

    rtp_udp_session::~rtp_udp_session()
    {
        close(m_rtp_socket);
        close(m_rtcp_socket);
    }

    bool rtp_udp_session::send_packet(rtp_packet* packet)
    {
        char rtcp_buf[1500];
        int rtcp_len = recvfrom(m_rtcp_socket, rtcp_buf, 1500, 0, NULL, NULL);
        if (rtcp_len > 0)
        {
            printf("----------udp session, get rtcp data, len %d----------------\n", rtcp_len);
            m_rtcp_timeout = MAX_RTCP_TIMEOUT;
        }

        int ret = sendto(m_rtp_socket, packet->data, packet->len, 0,(struct sockaddr*)&m_dst_addr, sizeof(m_dst_addr));

        return ret == packet->len;
    }

}}//namespace

