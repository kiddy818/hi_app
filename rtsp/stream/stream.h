#ifndef stream_include_h
#define stream_include_h

#include "stream_observer.h"

namespace ceanic{namespace rtsp{

    class stream
        :public ceanic::util::stream_obj
        ,public ceanic::util::stream_post
        ,public std::enable_shared_from_this<stream>
    {
        public:
            stream(int32_t chn,int32_t stream_id)
                :stream_obj("rtsp_stream",chn,stream_id),m_is_start(false), m_per_sec_len(0)
            {
            }

            virtual ~stream()
            {
            }

            virtual bool start() = 0;

            virtual void stop() = 0;

            virtual bool request_i_frame()
            {
                return false;
            }

            virtual bool get_media_head(util::media_head* mh)
            {
                return false;
            }

            bool is_start()
            {
                return m_is_start;
            }

            int32_t per_sec_len()
            {
                return m_is_start ? m_per_sec_len : 0;
            }

            time_t last_stream_time()
            {
                return m_last_stream_time;
            }

        protected:
            bool m_is_start;
            uint32_t m_per_sec_len;
            time_t m_last_stream_time;
    };

}}//namespace

#endif

