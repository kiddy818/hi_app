#ifndef h264_mp4_save_include_h
#define h264_mp4_save_include_h

#include <string>
#include "stream_save.h"
#include <mp4v2/mp4v2.h>

namespace ceanic{namespace stream_save{

    class h264_mp4_save
        :public stream_save
    {
        public:
            h264_mp4_save(const char* file_path);
            virtual ~h264_mp4_save();

        public:
            bool open() override;
            void close() override;
            bool is_open() override;
            bool input_data(ceanic::util::stream_head* head,const char* buf,int len) override;

        private:
            std::string m_file_path;
            MP4FileHandle m_mp4_fh;
            MP4TrackId m_mp4_track_id;
    };

}}//namespace

#endif
