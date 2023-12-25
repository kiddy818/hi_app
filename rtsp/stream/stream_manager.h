#ifndef stream_manager_include_h
#define stream_manager_include_h

#include "stream_stock.h"
#include <list>
#include <mutex>
#include <thread>

namespace beacon{namespace rtsp{

    typedef std::shared_ptr<stream_stock> stream_ptr;

    class stream_manager
    {
        public:
            bool get_stream(int chn,int stream_id, stream_ptr& stream);
            bool del_stream(int chn,int stream_id);

            static stream_manager* instance();

            bool process_data(int chn,int stream_id,util::stream_head* head,const char*buf,int len);

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
    };

}}//namespace

#endif
