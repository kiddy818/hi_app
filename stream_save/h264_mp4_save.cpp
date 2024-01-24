#include "h264_mp4_save.h"

namespace ceanic{namespace stream_save{

    h264_mp4_save::h264_mp4_save(const char* file_path)
        :m_file_path(file_path),m_mp4_fh(MP4_INVALID_FILE_HANDLE),m_mp4_track_id(MP4_INVALID_TRACK_ID)
    {
    }

    h264_mp4_save::~h264_mp4_save()
    {
        close();
    }

    bool h264_mp4_save::open()
    {
        if(is_open())
        {
            return false;
        }

        return true;
    }

    void h264_mp4_save::close()
    {
    }

    bool h264_mp4_save::is_open()
    {
        return m_mp4_fh != MP4_INVALID_FILE_HANDLE;
    }

    bool h264_mp4_save::input_data(ceanic::util::stream_head* head,const char* buf,int len)
    {
        return true;
    }

}}//namespace

