#ifndef rtp_type_include_h
#define rtp_type_include_h

namespace ceanic{namespace rtsp{

#define MAX_PACKET_LEN 1440

    struct rtp_packet
    {
        int len;
        char data[MAX_PACKET_LEN];
    };

    typedef struct 
    {
        /* byte 0 */
        unsigned char csrc_len:4;        /* expect 0 */
        unsigned char extension:1;        /* expect 1, see RTP_OP below */
        unsigned char padding:1;        /* expect 0 */
        unsigned char version:2;        /* expect 2 */
        /* byte 1 */
        unsigned char payload:7;        /* RTP_PAYLOAD_RTSP */
        unsigned char marker:1;        /* expect 1 */
        /* bytes 2, 3 */
        unsigned short seq_no;           
        /* bytes 4-7 */
        unsigned int timestamp;       
        /* bytes 8-11 */
        unsigned int ssrc;            /* stream number is used here. */
    }RTP_FIXED_HEADER;

}}//namespace

#endif

