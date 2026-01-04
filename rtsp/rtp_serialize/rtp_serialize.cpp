#include "rtp_serialize.h"

namespace ceanic{namespace rtsp{

    rtp_serialize::rtp_serialize(int32_t payload)
        :m_payload(payload)
    {
        m_seq = get_ramdom16();
        m_ssrc = get_random32();
    }

    rtp_serialize::~rtp_serialize()
    {
    }

    uint32_t rtp_serialize::get_random32()
    {
        uint32_t value = 0;
        //from jrtp
        FILE* f = fopen("/dev/urandom","rb");

        if (f)
        {
            fread(&value, sizeof(uint32_t), 1, f);
            fclose(f);
        }

        return value;
    }

    uint16_t rtp_serialize::get_ramdom16()
    {
        uint16_t value = 0;
        //from jrtp
        FILE* f = fopen("/dev/urandom","rb");

        if (f)
        {
            fread(&value, sizeof(uint16_t), 1, f);
            fclose(f);
        }

        return value;
    }

}}//namespace


