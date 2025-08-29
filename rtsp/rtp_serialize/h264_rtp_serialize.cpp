#include <h264_rtp_serialize.h>

namespace ceanic{namespace rtsp{

    h264_rtp_serialize::h264_rtp_serialize(int32_t payload)
        :rtp_serialize(payload)
    {
    }

    h264_rtp_serialize::~h264_rtp_serialize()
    {
    }

    void h264_rtp_serialize::process_nalu(util::nalu_t*nalu,rtp_session_ptr rs)
    {
        rtp_packet_t packet;
        RTP_FIXED_HEADER* rtp_hdr = packet.phdr;
        uint32_t max_data_len = MAX_PACKET_LEN - sizeof(RTP_FIXED_HEADER);
        uint8_t* nalu_data = (uint8_t*)nalu->data + 4;//ignore 00 00 00 01
        uint32_t nalu_size = nalu->size - 4;//ignore 00 00 00 01
        uint32_t nalu_timestamp = nalu->time_stamp * 90;

        memset(rtp_hdr,0,12); 
        rtp_hdr->version = 2;
        rtp_hdr->payload = m_payload;
        rtp_hdr->marker = 0;
        rtp_hdr->ssrc = htonl(m_ssrc);
        rtp_hdr->timestamp = htonl(nalu_timestamp);

        if(nalu_size <= max_data_len)
        {
            //signal nlau packet
            rtp_hdr->seq_no = htons(m_seq++);
            rtp_hdr->marker = 1;

            packet.rtp_data_len = nalu_size + sizeof(RTP_FIXED_HEADER);
            packet.outside_cnt = 1;
            packet.outside_info[0].len = nalu_size;
            packet.outside_info[0].data = nalu_data;
            packet._inter_len = TCP_TAG_SIZE + sizeof(RTP_FIXED_HEADER);

            rs->send_packet(&packet);
            return ;
        }

        //fu
        uint32_t fu_head_size = 2;
        max_data_len -= fu_head_size;
        uint8_t nal_head = nalu_data[0];
        uint8_t* data = nalu_data + 1;
        uint32_t left = nalu_size - 1;
        bool is_start = true;
        bool is_end = false;

        uint8_t* fu_buf = packet._inter_buf + TCP_TAG_SIZE + sizeof(RTP_FIXED_HEADER);
        while (left > 0) 
        {
            uint32_t size = std::min(max_data_len, left);
            is_end = (size == left);

            fu_buf[0] = (nal_head & 0xE0) | 28; // FU indicator
            fu_buf[1] = (nal_head & 0x1f); // FU header
            if (is_start) 
            {
                fu_buf[1] |= 0x80;
            }
            if (is_end) 
            {
                fu_buf[1] |= 0x40;
            }

            rtp_hdr->seq_no = htons(m_seq++);
            rtp_hdr->marker = is_end ? 1 : 0;

            packet.rtp_data_len = sizeof(RTP_FIXED_HEADER) + fu_head_size + size;
            packet.outside_cnt = 1;
            packet.outside_info[0].len = size;
            packet.outside_info[0].data = data;
            packet._inter_len = TCP_TAG_SIZE + sizeof(RTP_FIXED_HEADER) + fu_head_size;

            rs->send_packet(&packet);

            left -= size;
            data += size;
            is_start = false;
        }
    }

    bool h264_rtp_serialize::serialize(util::stream_head& head,const char* buf,int32_t len,rtp_session_ptr rs)
    {
        if(head.type != STREAM_NALU_SLICE)
        {
            return false;
        }

        uint8_t nalu_type;
        for(uint32_t i = 0; i < head.nalu_count; i++)
        {
            nalu_type = head.nalu[i].data[4] & 0x1f;
            if(nalu_type == 0x7/*sps*/
                    || nalu_type == 0x8/*pps*/
                    || nalu_type == 0x1/*p*/
                    || nalu_type == 0x5/*i*/)
            {
                process_nalu(&head.nalu[i],rs);
            }
        }

        return true;
    }

}}//namespace
