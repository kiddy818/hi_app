#ifndef rtp_type_include_h
#define rtp_type_include_h

namespace ceanic{namespace rtsp{

#define MAX_PACKET_LEN 1440
#define MAX_PACKET_GRP_NUM 5
#define TCP_TAG_SIZE 4

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

    struct rtp_packet_t
    {
        rtp_packet_t()
        {
            tcp_tag = _inter_buf;
            phdr = (RTP_FIXED_HEADER*)(_inter_buf + TCP_TAG_SIZE);
            _inter_len = TCP_TAG_SIZE + sizeof(RTP_FIXED_HEADER);
        }
 
        //内部buffer
        unsigned char _inter_buf[2048];
        int _inter_len;

        /*4字节,在_interbuf中,tcp传输中使用*/
        unsigned char *tcp_tag;

        /*rtp头,在_interbuf中,紧接tcp tag*/
        RTP_FIXED_HEADER* phdr;

        //rtp数据长度,包含RTP_FIXED_HEADER,不包含tcp tag长度
        //数据不连续，有可能在_interbuf中，也有可能在outsidebuf
        int rtp_data_len;

        /*外部buf*/
        int outside_cnt;
        struct
        {
            int len;
            unsigned char* data;
        }outside_info[MAX_PACKET_GRP_NUM];
    };

}}//namespace

#endif

