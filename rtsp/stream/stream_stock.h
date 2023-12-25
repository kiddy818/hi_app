#ifndef stream_stock_include_h
#define stream_stock_include_h

#include <stream.h>
#include <util/stream_buf.h>

namespace beacon{namespace rtsp{

#define MAX_STREAM_BUF_LEN (2 * 1024 * 1024)

    class stream_stock
        :public stream
    {
        public:
            explicit stream_stock(int chn,int stream_id = 0);

            virtual ~stream_stock();

            virtual bool start();

            virtual void stop();

            int chn()
            {
                return m_chn;
            }

            int stream_id()
            {
                return m_stream_id;
            }

            int stream();

            virtual bool request_i_frame();
            virtual bool get_stream_type(int* stream_type);
            virtual bool get_stream_head(util::media_head* mh);

            void process_data(util::stream_head* head,const char* buf,int len);

        protected:
            int m_chn;
            int m_stream_id;
            unsigned int m_stream_len;
            util::media_head m_media_head;
    };

}}//namespace

#endif

