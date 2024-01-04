#ifndef stream_type_include_h
#define stream_type_include_h

namespace ceanic{namespace util{

#define STREAM_P_FRAME 0
#define STREAM_I_FRAME 1
#define STREAM_AUDIO_FRAME 2
#define STREAM_B_FRAME 3
#define STREAM_NALU_SLICE 4 
#define STREAM_MJPEG_FRAME 6

#define IS_VIDEO_FRAME(n) ((n) == STREAM_I_FRAME  \
|| (n)== STREAM_P_FRAME  \
||(n)== STREAM_B_FRAME  \
||(n)== STREAM_NALU_SLICE \
||(n) == STREAM_MJPEG_FRAME)

#define IS_LOCATE_FRAME(n)((n) == STREAM_I_FRAME \
|| (n) == STREAM_MJPEG_FRAME)

#define IS_AUDIO_FRAME(n)((n) == STREAM_AUDIO_FRAME)

#define MAX_STREAM_NALU_COUNT (8)
    typedef struct
    {
        const char* data;
        int size;
        unsigned int timestamp;
    }nalu_t;

    typedef struct {
        int len;
        int type; //0:p,1:i 2:audio 3:nalu
        unsigned long long time_stamp;
        int tag;
        int sys_time;
        int w;
        int h;

        int nalu_count;
        nalu_t nalu[MAX_STREAM_NALU_COUNT];
    }stream_head;

    enum
    {
        STREAM_ENCODE_H264 = 0,
        STREAM_ENCODE_MJPEG = 1,
    };

#define CEANIC_TAG 0x5550564e
    typedef struct
    {
        unsigned int    media_fourcc;//"BEACON"
        int ver;//now is 1
        int vdec;

        int reserve[7];
    }media_head;

}}//namespace

#endif

