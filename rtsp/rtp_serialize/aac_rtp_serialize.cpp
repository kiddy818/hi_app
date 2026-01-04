#include <aac_rtp_serialize.h>
#include <string>

namespace ceanic{namespace rtsp{

    enum
    {
        AAC_SAMPLE_RATE_96000 = 0,
        AAC_SAMPLE_RATE_88200 = 1,
        AAC_SAMPLE_RATE_64000 = 2,
        AAC_SAMPLE_RATE_48000 = 3,
        AAC_SAMPLE_RATE_44100 = 4,
        AAC_SAMPLE_RATE_32000 = 5,
        AAC_SAMPLE_RATE_24000 = 6,
        AAC_SAMPLE_RATE_22050 = 7,
        AAC_SAMPLE_RATE_16000 = 8,
        AAC_SAMPLE_RATE_12000 = 9,
        AAC_SAMPLE_RATE_11025 = 10,
        AAC_SAMPLE_RATE_8000  = 11,
        AAC_SAMPLE_RATE_7350  = 12,
        AAC_SAMPLE_RATE_INVALID = 0xff,
    };

    aac_rtp_serialize::aac_rtp_serialize(int32_t payload,int32_t sample_rate,bool with_adsp)
        :rtp_serialize(payload),m_sample_rate(sample_rate),m_with_adsp(with_adsp)
    {
    }

    aac_rtp_serialize::~aac_rtp_serialize()
    {
    }

    uint8_t aac_rtp_serialize::get_sample_idx(uint32_t sample_rate)
    {
        uint8_t sample_idx = AAC_SAMPLE_RATE_INVALID;

        switch(sample_rate)
        {
            case 96000:
                    sample_idx = AAC_SAMPLE_RATE_96000;
                    break;
            case 88200:
                    sample_idx = AAC_SAMPLE_RATE_88200;
                    break;
            case 64000:
                    sample_idx = AAC_SAMPLE_RATE_64000;
                    break;
            case 48000:
                    sample_idx = AAC_SAMPLE_RATE_48000;
                    break;
            case 44100:
                    sample_idx = AAC_SAMPLE_RATE_44100;
                    break;
            case 32000:
                    sample_idx = AAC_SAMPLE_RATE_32000;
                    break;
            case 24000:
                    sample_idx = AAC_SAMPLE_RATE_24000;
                    break;
            case 16000:
                    sample_idx = AAC_SAMPLE_RATE_16000;
                    break;
            case 12000:
                    sample_idx = AAC_SAMPLE_RATE_12000;
                    break;
            case 11025:
                    sample_idx = AAC_SAMPLE_RATE_11025;
                    break;
            case 8000:
                    sample_idx = AAC_SAMPLE_RATE_8000;
                    break;
            case 7350:
                    sample_idx = AAC_SAMPLE_RATE_7350;
                    break;
        }

        return sample_idx;
    }

    bool aac_rtp_serialize::get_config(uint8_t profile,uint32_t sample_rate,uint8_t chn,std::string& cfg_str)
    {
        uint8_t sample_rate_idx = get_sample_idx(sample_rate);
        if(sample_rate_idx == AAC_SAMPLE_RATE_INVALID)
        {
            return false;
        }

        uint8_t obj_type = profile + 1;
        uint8_t cfg[2];
        cfg[0] = (obj_type << 3) | (sample_rate_idx >> 1);
        cfg[1] = (sample_rate_idx << 7) | (chn << 3);

        char str[32];
        sprintf(str, "%02x%02x", cfg[0], cfg[1]);
        cfg_str = str;

        return true;
    }

    bool aac_rtp_serialize::serialize(util::stream_head& head,const char* buf,int32_t len,rtp_session_ptr rs)
    {
        if(!IS_AUDIO_FRAME(head.type))
        {
            return false;
        }

        //去除ADSP头
        if(m_with_adsp)
        {
            buf += 7;
            len -= 7;
        }

        if(len <= 0 || (uint32_t)len > (MAX_PACKET_LEN - sizeof(RTP_FIXED_HEADER)))
        {
            return false;
        }

        rtp_packet_t packet;
        RTP_FIXED_HEADER* rtp_hdr = packet.phdr;
        uint32_t time_stamp = head.time_stamp * (m_sample_rate / 1000);

        memset(rtp_hdr,0,12);
        rtp_hdr->version = 2;
        rtp_hdr->payload = m_payload;
        rtp_hdr->ssrc = htonl(m_ssrc);
        rtp_hdr->timestamp = htonl(time_stamp);
        rtp_hdr->seq_no = htons(m_seq++);
        rtp_hdr->marker = 1;

        uint8_t* au_head_info = packet._inter_buf + TCP_TAG_SIZE + sizeof(RTP_FIXED_HEADER);
        uint32_t au_head_len = 4;

        /*au head size len*/
        au_head_info[0] = 0x00;
        au_head_info[1] = 0x10;//16bits

        /*au head*/
        /*前13bit 为长度,3bit为0*/
        au_head_info[2] = (len & 0x1fe0) >> 5;
        au_head_info[3] = (len & 0x1f) << 3;

        packet.rtp_data_len = sizeof(RTP_FIXED_HEADER) + au_head_len + len;
        packet.outside_cnt = 1;
        packet.outside_info[0].len = len;
        packet.outside_info[0].data = (uint8_t*)buf;
        packet._inter_len = TCP_TAG_SIZE + sizeof(RTP_FIXED_HEADER) + au_head_len;

        rs->send_packet(&packet);
        return true;
    }
}}//namespace

