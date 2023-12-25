#ifndef mjpeg_rtp_serialize_include_h
#define mjpeg_rtp_serialize_include_h
#include <rtp_serialize.h>

namespace beacon{namespace rtsp{

    /*
       0                   1                   2                   3
       0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       | Type-specific |              Fragment Offset                  |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |      Type     |       Q       |     Width     |     Height    |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       */
    typedef struct
    {
        unsigned int off:24;
        unsigned int tspec:8;
        unsigned char type;
        unsigned char q;
        unsigned char width;
        unsigned char height;
    }jpg_hdr_t;

    // Restart Marker header present
    /*
       0                   1                   2                   3
       0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |       Restart Interval        |F|L|       Restart Count       |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       */
    typedef struct
    {
        unsigned short dri;
        unsigned int count:14;
        unsigned int l:1;
        unsigned int f:1;
    }jpg_hdr_rst_t;

    // Quantization Table header present
    /*
       0                   1                   2                   3
       0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |      MBZ      |   Precision   |             Length            |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |                    Quantization Table Data                    |
       |                              ...                              |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       */
    typedef struct
    {
        unsigned char mbz;
        unsigned char precision;
        unsigned short length;
    }qbtl_hdr_t;

    class mjpeg_rtp_serialize
        :public rtp_serialize
    {
        public:
            mjpeg_rtp_serialize();

            ~mjpeg_rtp_serialize();

            bool serialize(util::stream_head& head,const char* buf,int len,rtp_session_ptr rs);

        private:
            bool process_mjpeg(unsigned int time_stamp,const char* jpg_data,int len,unsigned char type,unsigned char typespec,
                    int width,int height,int dri,unsigned char q,unsigned char* lqt,unsigned char* cqt,rtp_session_ptr rs);
    };

}}//namespace

#endif


