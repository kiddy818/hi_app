#include "rtp_serialize.h"

namespace beacon{namespace rtsp{

    rtp_serialize::rtp_serialize(int payload)
        :m_payload(payload)
    {
        m_seq = get_ramdom16();
        m_ssrc = get_random32();
    }

    rtp_serialize::~rtp_serialize()
    {
    }

    unsigned int rtp_serialize::get_random32()
    {
        unsigned int value = 0;
        //from jrtp
        FILE* f = fopen("/dev/urandom","rb");

        if (f)
        {
            fread(&value, sizeof(unsigned int), 1, f);
            fclose(f);
        }

        return value;
    }

    unsigned short rtp_serialize::get_ramdom16()
    {
        unsigned short value = 0;
        //from jrtp
        FILE* f = fopen("/dev/urandom","rb");

        if (f)
        {
            fread(&value, sizeof(unsigned short), 1, f);
            fclose(f);
        }

        return value;
    }

}}//namespace


