#ifndef h265_mp4_save_include_h
#define h265_mp4_save_include_h

#include <util/std.h>
#include <util/stream_buf.h>
#include <string>
#include <mutex>
#include <thread>
#include "stream_save.h"

namespace ceanic{namespace stream_save{

    typedef struct
    {
        int len;
        uint32_t timestamp;
    }h265_sbuf_head_t;

    class h265_mp4_save
        :public stream_save
    {
        public:
            h265_mp4_save(const char* file_path);
            virtual ~h265_mp4_save();

        public:
            bool open() override;
            void close() override;
            bool is_open() override;
            bool input_data(ceanic::util::stream_head* head,const char* buf,int len) override;

        private:
            void process_h265_nalue(const char* nalu_data,int nalu_size,uint32_t time_stamp);
            void on_process();

        private:
            bool m_bopen;
            std::string m_file_path;
            std::mutex m_interface_mu;
            std::thread m_process_thread;
            void* m_gpac_fh;
            bool m_vps_got;

            std::mutex m_buf_mu;
            ceanic::util::stream_buf m_sbuf;
    };

}}//namespace

#endif
