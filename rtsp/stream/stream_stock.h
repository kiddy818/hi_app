#ifndef stream_stock_include_h
#define stream_stock_include_h

#include <stream.h>
#include <util/stream_buf.h>

namespace ceanic{namespace rtsp{

#define MAX_STREAM_BUF_LEN (2 * 1024 * 1024)

    class stream_stock
        :public stream
    {
        public:
            explicit stream_stock(int32_t chn,int32_t stream_id);

            virtual ~stream_stock();

            virtual bool start();

            virtual void stop();

            void process_data(util::stream_head* head,const char* buf,int32_t len);

        protected:
            uint32_t m_stream_len;
    };

}}//namespace

#endif

