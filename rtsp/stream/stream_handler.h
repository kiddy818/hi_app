#ifndef stream_handler_include_h
#define stream_handler_include_h

#include "stream_observer.h"

namespace beacon{namespace rtsp{

    class stream_handler
        :public stream_observer
    {
        public:
            stream_handler();

            virtual ~stream_handler();

            void on_stream_come(util::stream_head* head, const char* buf, int len);

            void on_stream_error(int errno);

            virtual bool start() = 0;

            virtual void stop() = 0;

            bool is_start()
            {
                return m_start;
            }

            time_t start_tm()
            {
                return m_beg;
            }

            virtual int& get_rtcp_timeout() = 0;

        protected:
            bool m_start;
            time_t m_beg;

        protected:
            virtual bool process_stream(util::stream_head* head, const char* data, int len) = 0;
    };

    typedef std::shared_ptr<stream_handler> stream_handler_ptr; 

}}//namespace

#endif

