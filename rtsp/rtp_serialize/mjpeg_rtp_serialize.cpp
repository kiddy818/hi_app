#include <mjpeg_rtp_serialize.h>
#include <assert.h>

namespace beacon{namespace rtsp{

    mjpeg_rtp_serialize::mjpeg_rtp_serialize()
        :rtp_serialize(26)
    {
    }

    mjpeg_rtp_serialize::~mjpeg_rtp_serialize()
    {
    }

    bool mjpeg_rtp_serialize::serialize(util::stream_head& head,const char* buf,int len,rtp_session_ptr rs)
    {
        if(head.type != STREAM_MJPEG_FRAME)
        {
            return false;
        }
        unsigned int time_stamp = head.time_stamp / 1000 * 90;

        char qbuf[1024];
        char* qtable = qbuf;

        char* ptr = (char*)buf;
        char* jpg_data = NULL;
        int marker_len = 0;
        int dri = 0;
        int width = 0,height = 0;
        while(ptr <= buf + len -2)
        {
            if(ptr[0] == 0xff
                    && ptr[1] != 0x00)
            {
                switch(ptr[1])
                {
                    case 0xd8:
                        {
                            //printf("start of image\n");
                            ptr += 2;
                            break;
                        }

                    case 0xe0:
                        {
                            ptr += 2;
                            marker_len = ptr[0] << 8 | ptr[1];
                            ptr += marker_len;

                            //printf("e0 len %d\n",marker_len);
                            break;
                        }
                    case 0xdd:
                        {
                            ptr += 2;
                            marker_len = ptr[0] << 8 | ptr[1];
                            dri = ptr[2] << 8 | ptr[3];
                            ptr+= marker_len;

                            //printf("restart interval len %d\n",marker_len);
                            break;
                        }
                    case 0xdb:
                        {
                            ptr += 2;
                            marker_len = ptr[0] << 8 | ptr[1];

                            memcpy(qtable,&ptr[3],marker_len - 3);
                            qtable += (marker_len - 3);

                            ptr+= marker_len;
                            //printf("qt table %d\n",marker_len);
                            break;
                        }
                    case 0xc0:
                        {
                            ptr += 2;
                            marker_len = ptr[0] << 8 | ptr[1];

                            height = ptr[3]  << 8 | ptr[4];
                            width = ptr[5] << 8 | ptr[6]; 

                            //printf("c0 %02x %02x %02x %02x %02x %02x %02x %02x\n",ptr[0],ptr[1],ptr[2],ptr[3],ptr[4],ptr[5],ptr[6],ptr[7]);

                            ptr += marker_len;
                            break;
                        }

                    case 0xda:
                        {
                            ptr += 2;
                            marker_len = ptr[0] << 8 | ptr[1];

                            ptr += marker_len;

                            jpg_data = ptr;
                            //printf("ok len %d\n",marker_len);

                            break;
                        }
                    default:
                        {
                            //printf("unknown %x ",ptr[1]);

                            ptr += 2;
                            marker_len = ptr[0] << 8 | ptr[1];

                            //printf("len %d \n",marker_len);

                            ptr += marker_len;
                            break;
                        }
                }

                if(jpg_data != NULL)
                {
                    break;
                }
            }
            else
            {
                ptr++;
                printf("nill \n");
            }
        }

        if(jpg_data == NULL)
        {
            return true;
        }

        int jpg_len = len - (jpg_data - buf);
        int qlen = qtable - qbuf;
        //printf("len %d,jpg_len %d,width %d,height %d,dri %d,qtable_len%d\n",len,jpg_len,width,height,dri,qtable - qbuf);

        return process_mjpeg(time_stamp,jpg_data,jpg_len,1,0,width,height,dri,qlen,(unsigned char*)qbuf ,(unsigned char*)qbuf + qlen / 2,rs);
    }

    bool mjpeg_rtp_serialize::process_mjpeg(unsigned int time_stamp,const char* jpg_data,int len,unsigned char type,unsigned char typespec,
            int width,int height,int dri,unsigned char q,unsigned char* lqt,unsigned char* cqt,rtp_session_ptr rs)
    {
        int bytes_left = len;
        int offset = 0;

        rtp_packet packet;
        RTP_FIXED_HEADER* rtp_hdr = (RTP_FIXED_HEADER*)packet.data;

        memset(rtp_hdr,0,12);
        rtp_hdr->version = 2;
        rtp_hdr->payload = m_payload;
        rtp_hdr->marker = 0;
        rtp_hdr->ssrc = htonl(m_ssrc);
        rtp_hdr->timestamp = htonl(time_stamp);

        //jpeg hdr
        jpg_hdr_t jpg_hdr;
        jpg_hdr.off = 0;
        jpg_hdr.tspec = typespec;
        jpg_hdr.type = type | ((dri != 0) ? 0x40 : 0);
        jpg_hdr.q = q;
        jpg_hdr.width = width / 8;
        jpg_hdr.height = height / 8;

        //jpeg rst
        jpg_hdr_rst_t jpg_rst;
        if(dri != 0)
        {
            jpg_rst.dri = htons(dri);
            jpg_rst.count = htons(0x3fff);
            assert(jpg_rst.count == 0xff3f);
            jpg_rst.l = 1;
            jpg_rst.f = 1;
        }

        //jpeg qtabale
        qbtl_hdr_t qbtl_hdr;
        if(q >= 128)
        {
            qbtl_hdr.mbz = 0;
            qbtl_hdr.precision = 0;
            qbtl_hdr.length = htons(128);
        }

        char* ptr;
        while(bytes_left > 0)
        {
            rtp_hdr->seq_no = htons(m_seq++);
            ptr = packet.data + sizeof(RTP_FIXED_HEADER);

            //jpeg hdr
            jpg_hdr.off = ((offset & 0xff) << 16) | ((offset & 0xff00)) | ((offset >> 16) & 0xff) ;
            assert(jpg_hdr.tspec == typespec);
            memcpy(ptr,&jpg_hdr,sizeof(jpg_hdr));
            ptr += sizeof(jpg_hdr);
            assert(sizeof(jpg_hdr) == 8);

            if(dri != 0)
            {
                memcpy(ptr,&jpg_rst,sizeof(jpg_rst));
                ptr += sizeof(jpg_rst);
                assert(sizeof(jpg_rst) == 4);
            }

            if(q >= 128 && jpg_hdr.off == 0)
            {
                memcpy(ptr,&qbtl_hdr,sizeof(qbtl_hdr));
                ptr += sizeof(qbtl_hdr);
                assert(sizeof(qbtl_hdr) == 4);

                memcpy(ptr,lqt,64);
                ptr += 64;
                memcpy(ptr,cqt,64);
                ptr += 64;
            }

            int data_len = MAX_PACKET_LEN - (ptr - packet.data);
            if(data_len > bytes_left)
            {
                data_len = bytes_left;
                rtp_hdr->marker = 1;
            }

            memcpy(ptr,jpg_data + offset,data_len);
            ptr += data_len;

            packet.len = ptr - packet.data;
            rs->send_packet(&packet);

            offset += data_len;
            bytes_left -= data_len;
        }

        return true;
    }
}}//namespace

