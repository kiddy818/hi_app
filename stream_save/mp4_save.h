#ifndef mp4_save_include_h
#define mp4_save_include_h

#include <util/std.h>
#include <util/stream_buf.h>
#include <string>
#include <mutex>
#include <thread>
#include "stream_save.h"
#include <ss_mp4_format.h>

namespace ceanic{namespace stream_save{
    class mp4_save
        :public stream_save
    {
        public:
            mp4_save(ceanic::util::media_head mh,const char* file_path);
            virtual ~mp4_save();

        public:
            bool open() override;
            void close() override;
            bool is_open() override;
            bool input_data(ceanic::util::stream_head* head,const char* buf,int32_t len) override;

        private:
            void process_video_frame(const char* frame_buf,int32_t frame_len);
            void process_audio_frame(const char* frame_buf,int32_t frame_len);
            void on_process();

        private:
            bool m_bopen;
            ceanic::util::media_head m_mh;
            std::string m_file_path;

            OT_MP4_CONFIG_S m_mp4_cfg;
            TD_MW_PTR m_mp4_fh;
            TD_MW_PTR m_mp4_video_track_h;
            TD_MW_PTR m_mp4_audio_track_h;
            OT_MP4_TRACK_INFO_S m_mp4_video_info;
            OT_MP4_TRACK_INFO_S m_mp4_audio_info;

            std::mutex m_interface_mu;
            std::thread m_process_thread;

            std::mutex m_buf_mu;
            ceanic::util::stream_buf m_sbuf;
    };

}}//namespace

#endif
