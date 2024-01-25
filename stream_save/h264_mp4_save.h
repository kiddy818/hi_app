#ifndef h264_mp4_save_include_h
#define h264_mp4_save_include_h

#include <util/std.h>
#include <util/stream_buf.h>
#include <string>
#include <mutex>
#include <thread>
#include "stream_save.h"
#include <mp4v2/mp4v2.h>

namespace ceanic{namespace stream_save{

    class h264_mp4_save
        :public stream_save
    {
        public:
            h264_mp4_save(const char* file_path,int w,int h,int fr);
            virtual ~h264_mp4_save();

        public:
            bool open() override;
            void close() override;
            bool is_open() override;
            bool input_data(ceanic::util::stream_head* head,const char* buf,int len) override;

        private:
            void process_h264_nalue(const char* nalu_data,int nalu_size);
            void on_process();

        private:
            bool m_bopen;
            std::string m_file_path;
            MP4FileHandle m_mp4_fh;
            MP4TrackId m_mp4_track_id;
            std::mutex m_interface_mu;
            std::thread m_process_thread;

            std::mutex m_buf_mu;
            ceanic::util::stream_buf m_sbuf;
            int m_w;
            int m_h;
            int m_fr;
    };

}}//namespace

#endif
