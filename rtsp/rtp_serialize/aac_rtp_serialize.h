#ifndef aac_rtp_serialize_include_h
#define aac_rtp_serialize_include_h

#include <rtp_serialize.h>
#include <util/std.h>

namespace ceanic{namespace rtsp{

      class aac_rtp_serialize
        :public rtp_serialize
    {
        public:
            explicit aac_rtp_serialize(int32_t payload,int32_t sample_rate,bool with_adsp = true);

            virtual ~aac_rtp_serialize();

            bool serialize(util::stream_head& head,const char* buf,int32_t len,rtp_session_ptr rs);

            static bool get_config(uint8_t profile,uint32_t sample_rate,uint8_t chn,std::string& cfg_str);
            static uint8_t get_sample_idx(uint32_t sample_rate);
        private:
            int32_t m_sample_rate;
            bool m_with_adsp;
    };

}}//namespace

#endif

