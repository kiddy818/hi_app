#include "h265_rtp_serialize.h"

namespace ceanic{namespace rtsp{

    h265_rtp_serialize::h265_rtp_serialize(int payload)
        :rtp_serialize(payload)
    {
    }

    h265_rtp_serialize::~h265_rtp_serialize()
    {
    }

    void h265_rtp_serialize::process_nalu(util::nalu_t*nalu,rtp_session_ptr rs)
    {
        rtp_packet packet;
        RTP_FIXED_HEADER* rtp_hdr = (RTP_FIXED_HEADER*)packet.data;
        int max_data_len = MAX_PACKET_LEN - sizeof(RTP_FIXED_HEADER);

        memset(rtp_hdr,0,12);
        rtp_hdr->version = 2;
        rtp_hdr->payload = m_payload;
        rtp_hdr->marker = 0;
        rtp_hdr->ssrc = htonl(m_ssrc);
        rtp_hdr->timestamp = htonl(nalu->timestamp);

        if(nalu->size <= max_data_len)
        {
            //signal nlau packet
            rtp_hdr->seq_no = htons(m_seq ++);
            rtp_hdr->marker = 1;
            memcpy(packet.data + sizeof(RTP_FIXED_HEADER),nalu->data,nalu->size);
            packet.len = nalu->size + sizeof(RTP_FIXED_HEADER);

            rs->send_packet(&packet);
            return ;
        }

        //fu
        char nal_head = nalu->data[0]; // NALU 头
        const char* data = nalu->data + 2;
        int left = nalu->size - 2;
        bool is_start = true;
        bool is_end;

        while (left > 0)
        {
            int size = std::min(max_data_len - 3, left);
            is_end = (size == left);

            //from ffempg file(avformat/rtpenc_h264_hev.c)
            int nalu_type =  (nal_head >> 1) & 0x3f;
            char* fu_buf = packet.data + sizeof(RTP_FIXED_HEADER);
            fu_buf[0] = 49 << 1;
            fu_buf[1] = 1;

            fu_buf[2] = nalu_type;
            if(is_start)
            {
                fu_buf[2] |= 0x80;
            }
            if(is_end)
            {
                fu_buf[2] |= 0x40;
            }

            rtp_hdr->seq_no = htons(m_seq ++);
            rtp_hdr->marker = is_end ? 1 : 0;
            memcpy(fu_buf + 3, data, size);

            packet.len = sizeof(RTP_FIXED_HEADER) + size + 3;

            rs->send_packet(&packet);

            left -= size;
            data += size;
            is_start = false;
        }
    }

    bool h265_rtp_serialize::serialize(util::stream_head& head,const char* buf,int len,rtp_session_ptr rs)
    {
        if(head.type != STREAM_NALU_SLICE)
        {
            return false;
        }
        
        for(int i = 0; i < head.nalu_count; i++)
        {
            process_nalu(&head.nalu[i],rs);
        }

        return true;
    }

}}//namespace
