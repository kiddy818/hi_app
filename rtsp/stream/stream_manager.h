#ifndef stream_manager_include_h
#define stream_manager_include_h

#include "stream_stock.h"
#include <list>
#include <mutex>
#include <thread>

namespace ceanic{namespace rtsp{

    typedef std::shared_ptr<stream_stock> stream_ptr;

    struct stream_ops
    {
        bool (*request_i_frame_fun)(int32_t chn,int32_t stream);
        bool (*get_stream_head_fun)(int32_t chn,int32_t stream,ceanic::util::media_head* mh);
    };

    class stream_manager
    {
        public:
            bool register_stream_ops(stream_ops ops);
            bool request_i_frame(int32_t chn,int32_t stream_id);
            bool get_stream_head(int32_t chn,int32_t stream_id,ceanic::util::media_head* mh);

            bool get_stream(int32_t chn,int32_t stream_id, stream_ptr& stream);
            bool del_stream(int32_t chn,int32_t stream_id);

            static stream_manager* instance();

            bool process_data(int32_t chn,int32_t stream_id,util::stream_head* head,const char*buf,int32_t len);

        private:
            bool start_stream_check();
            void stop_stream_check();
            void on_checking();

            stream_manager();
            ~stream_manager();

            std::list<stream_ptr> m_streams;
            static stream_manager* g_instance;
            std::mutex m_stream_mu;

            bool m_stream_checking;
            std::thread m_thread;
            stream_ops m_ops;
    };

}}//namespace

#endif
