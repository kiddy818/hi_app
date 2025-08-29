#ifndef rtp_udp_session_include_h
#define rtp_udp_session_include_h
#include "rtp_session.h"
#include <string>

namespace ceanic{namespace rtsp{

    class rtp_udp_session
        :public rtp_session
    {
        public:
            rtp_udp_session(const char* remote_ip, int16_t remote_rtp_port, int16_t remote_rtcp_port, const char* local_ip, int16_t local_rtp_port, int16_t local_rtcp_port);

            virtual ~rtp_udp_session();

            bool send_packet(rtp_packet_t* packet);

        protected:
            std::string m_remote_ip;
            int16_t m_remote_rtp_port;
            int16_t m_remote_rtcp_port;
            int16_t m_local_rtp_port;
            int16_t m_local_rtcp_port;
            int32_t m_rtp_socket;
            int32_t m_rtcp_socket;
            struct sockaddr_in m_dst_addr;
    };

}}//namespace

#endif

