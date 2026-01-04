#include <pcmu_rtp_serialize.h>

namespace ceanic{namespace rtsp{

    pcmu_rtp_serialize::pcmu_rtp_serialize()
        :rtp_serialize(0)
    {
    }

    pcmu_rtp_serialize::~pcmu_rtp_serialize()
    {
    }

    bool pcmu_rtp_serialize::serialize(util::stream_head& head,const char* buf,int32_t len,rtp_session_ptr rs)
    {
        if(!IS_AUDIO_FRAME(head.type))
        {
            return false;
        }

        if(len <= 0 || (uint32_t)len > (MAX_PACKET_LEN - sizeof(RTP_FIXED_HEADER)))
        {
            return false;
        }

        rtp_packet_t packet;
        RTP_FIXED_HEADER* rtp_hdr = packet.phdr;
        uint32_t time_stamp = head.time_stamp * 8;

        memset(rtp_hdr,0,12);
        rtp_hdr->version = 2;
        rtp_hdr->payload = m_payload;
        rtp_hdr->ssrc = htonl(m_ssrc);
        rtp_hdr->timestamp = htonl(time_stamp);
        rtp_hdr->seq_no = htons(m_seq++);
        rtp_hdr->marker = 1;

        packet.rtp_data_len = len + sizeof(RTP_FIXED_HEADER);
        packet.outside_cnt = 1;
        packet.outside_info[0].len = len;
        packet.outside_info[0].data = (uint8_t*)buf;
        packet._inter_len = TCP_TAG_SIZE + sizeof(RTP_FIXED_HEADER);

        rs->send_packet(&packet);

        return true;
    }

}}//namespace
