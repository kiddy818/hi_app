#include <util/std.h>
#include <rtmp/session.h>

namespace ceanic{namespace rtmp{

#define RTMP_HEAD_SIZE   (sizeof(RTMPPacket)+RTMP_MAX_HEADER_SIZE)

    typedef struct
    {
        uint32_t len;
        uint32_t timestamp;
        int32_t type;
    }_head;

    session::session(std::string url)
        :m_url(url),m_bstart(false),m_sps_size(0),m_pps_size(0)
         ,m_sbuf(512 * 1024),m_rtmp(NULL) {
    }

    session::~session()
    {
        assert(m_bstart == false);
    }

    bool session::start()
    {
        if(m_bstart)
        {
            return false;
        }

        RTMP_LogSetLevel(RTMP_LOGDEBUG);

        m_rtmp = RTMP_Alloc();
        if(m_rtmp == NULL)
        {
            fprintf(stderr,"RTMP_ALLOC() failed\n");
            return false;
        }

        RTMP_Init(m_rtmp);
        m_rtmp->Link.timeout = 5;
        m_rtmp->Link.lFlags |= RTMP_LF_LIVE;
        
        int ret = RTMP_SetupURL(m_rtmp,(char*)m_url.c_str());
        if(!ret)
        {
            fprintf(stderr,"RTMP_SetupURL() failed\n");
            RTMP_Free(m_rtmp);
            return false;
        }

        //RTMP_SetBufferMS(m_rtmp, 1000);

        RTMP_EnableWrite(m_rtmp);

        ret = RTMP_Connect(m_rtmp,NULL);
        if(!ret)
        {
            fprintf(stderr,"RTMP_Connect() failed\n");
            RTMP_Free(m_rtmp);
            return false;
        }

        ret = RTMP_ConnectStream(m_rtmp,0);
        if(!ret)
        {
            fprintf(stderr,"RTMP_ConnectStream() failed\n");
            RTMP_Close(m_rtmp);
            RTMP_Free(m_rtmp);
            return false;
        }

        fprintf(stdout,"Rtmp_Connect success\n");
        m_bstart = true;
        m_thread = std::thread(&session::on_process_nalu,this);
        return true;
    }

    void session::stop()
    {
        if(!m_bstart)
        {
            return;
        }

        m_bstart = false;

        RTMP_Close(m_rtmp);  

        m_thread.join();

        RTMP_Free(m_rtmp);  
        m_rtmp= NULL;  
    }

    bool session::is_start()
    {
        return m_bstart;
    }

    bool session::input_one_nalu(const uint8_t* data,uint32_t len,uint32_t timestamp)
    {
        if(!m_bstart)
        {
            return false;
        }

        if(!RTMP_IsConnected(m_rtmp))
        {
            return false;
        }

        if(data[0] != 0x00
                || data[1] != 0x00
                || data[2] != 0x00
                || data[3] != 0x01)
        {
            fprintf(stderr,"invalid nalu start(%02x,%02x,%02x,%02x)\n",data[0],data[1],data[2],data[3]);
            return false;
        }

        data += 4;
        len -= 4;
        int type = data[0] & 0x1f;

        std::unique_lock<std::mutex> lock(m_sbuf_mu);
        switch(type)
        {
            case 0x7:
                {
                    //sps;
                    if(len <= sizeof(m_sps))
                    {
                        memcpy(m_sps,data,len);
                        m_sps_size = len;
                        return true;
                    }
                    return false;
                    break;
                }

            case 0x8:
                {
                    //pps;
                    if(len <= sizeof(m_pps))
                    {
                        memcpy(m_pps,data,len);
                        m_pps_size = len;
                        return true;
                    }
                    return false;
                    break;
                }

            case 1://p
            case 5://I
                {
                    if(m_sbuf.get_remain_len() < len + sizeof(_head))
                    {
                        return false;
                    }
                    _head head;
                    head.len = len;
                    head.timestamp = timestamp;
                    head.type = type;

                    m_sbuf.input_data(&head,sizeof(head));
                    m_sbuf.input_data((char*)data,len);

                    return true;
                    break;
                }

            case 6:
                {
                    //do nothing
                    return true;
                    break;
                }

            default:
                {
                    return false;
                }
        }

        return true;
    }

    void session::on_process_nalu()
    {
        util::dybuf dbuf;
        bool key_sended = false;
        bool metedata_sended = false;

        while(m_bstart)
        {
            bool data_ready = false;
            _head head;

            {
                std::unique_lock<std::mutex> lock(m_sbuf_mu);
                if(m_sbuf.copy_data(&head,sizeof(head))
                        && (head.len + sizeof(head)) <= m_sbuf.get_stream_len())
                {
                    dbuf.reset_if(RTMP_HEAD_SIZE + 9 + head.len);

                    m_sbuf.get_data(&head,sizeof(head));
                    m_sbuf.get_data(dbuf.pointer() + RTMP_HEAD_SIZE + 9,head.len);
                    data_ready = true;
                }
            }

            if(!data_ready)
            {
                usleep(10000);
                continue;
            }

            bool key_frame = (head.type == 0x5);

            if(!key_sended && !key_frame)
            {
                printf("[%s]: wait for i frame\n",__FUNCTION__);
                continue;
            }

#if 0
            if(!metedata_sended)
            {
                printf("[%s]: send metedatas\n",__FUNCTION__);
                send_metedata();
                metedata_sended = true;
            }
#endif

            uint8_t* body = (uint8_t*)dbuf.pointer() + RTMP_HEAD_SIZE;
            int i = 0; 
            body[i++] = key_frame ? 0x17 : 0x27;
            body[i++] = 0x01;// AVC NALU   
            body[i++] = 0x00;  
            body[i++] = 0x00;  
            body[i++] = 0x00;  
            body[i++] = (head.len >> 24) & 0xff;  
            body[i++] = (head.len >> 16) & 0xff;  
            body[i++] = (head.len >> 8) & 0xff;  
            body[i++] = (head.len) & 0xff;

            if(key_frame)
            {
                send_sps_pps(head.timestamp);
                key_sended = true;
            }

            if(!send_packet((uint8_t*)dbuf.pointer(),RTMP_HEAD_SIZE + 9 + head.len,head.timestamp))
            {
                printf("[%s]: send packet failed\n",__FUNCTION__);
                break;
            }
        }
    }

    bool session::send_metedata()
    {
        uint8_t body[1024];

        RTMPPacket* packet = (RTMPPacket*)body;
        memset(packet,0,RTMP_HEAD_SIZE);

        packet->m_packetType = RTMP_PACKET_TYPE_INFO;
        packet->m_nChannel = 0x04;
        packet->m_headerType = RTMP_PACKET_SIZE_LARGE;
        packet->m_nTimeStamp = 0;
        packet->m_nInfoField2 = m_rtmp->m_stream_id;
        packet->m_body = (char *)packet + RTMP_HEAD_SIZE;

        uint8_t *ptr = (uint8_t*)packet->m_body;
        ptr = put_byte(ptr, AMF_STRING);
        ptr = put_amf_string(ptr, "onMetaData");
        ptr = put_byte(ptr, AMF_OBJECT);
        ptr = put_amf_string(ptr, "videocodecid");
        ptr = put_amf_double(ptr, 7);
        ptr = put_amf_string(ptr, "");
        ptr = put_byte(ptr, AMF_OBJECT_END);

        packet->m_nBodySize = (char*)ptr - packet->m_body;

        int ret = RTMP_SendPacket(m_rtmp, packet, 0);

        return ret ? true : false;
    }

    bool session::send_sps_pps(uint32_t timestamp)
    {
        uint8_t body[1024];
        if(m_sps_size == 0 || m_pps_size == 0)
        {
            return false;
        }

        uint8_t* sps = m_sps;
        uint8_t* pps = m_pps;
        uint32_t sps_len = m_sps_size;
        uint32_t pps_len = m_pps_size;

        /*AVC head*/
        int i = RTMP_HEAD_SIZE;
        body[i++] = 0x17;
        body[i++] = 0x00;
        body[i++] = 0x00;
        body[i++] = 0x00;
        body[i++] = 0x00;

        /*AVCDecoderConfigurationRecord*/
        body[i++] = 0x01;
        body[i++] = *(sps+1);
        body[i++] = *(sps+2);
        body[i++] = *(sps+3);
        body[i++] = 0xff;

        /*sps*/
        body[i++]   = 0xe1;
        body[i++] = (sps_len >> 8) & 0xff;
        body[i++] = sps_len & 0xff;
        memcpy(&body[i],sps,sps_len);
        i += sps_len;

        /*pps*/
        body[i++]   = 0x01;
        body[i++] = (pps_len >> 8) & 0xff;
        body[i++] = (pps_len) & 0xff;
        memcpy(&body[i],pps,pps_len);
        i += pps_len;

        /*send rtmp packet*/
        send_packet(body,i,timestamp);
        return true;
    }

    bool session::send_packet(const uint8_t* data,uint32_t len,uint32_t timestamp)
    {
        RTMPPacket* packet = (RTMPPacket*)data;
        memset(packet,0,RTMP_HEAD_SIZE);
        packet->m_body = (char *)packet + RTMP_HEAD_SIZE;
        packet->m_nBodySize = len - RTMP_HEAD_SIZE;
        packet->m_hasAbsTimestamp = 0;
        packet->m_packetType = RTMP_PACKET_TYPE_VIDEO;
        packet->m_nInfoField2 = m_rtmp->m_stream_id;
        packet->m_nChannel = 0x04;
        packet->m_headerType = RTMP_PACKET_SIZE_LARGE;
        packet->m_nTimeStamp = timestamp;

        int ret = 0;
        if(RTMP_IsConnected(m_rtmp))
        {
            ret = RTMP_SendPacket(m_rtmp,packet,0);
        }

        return ret ? true : false;
    }

    uint8_t* session::put_byte(uint8_t* out,uint8_t v)
    {
        out[0] = v;    
        return out + 1;
    }

    uint8_t* session::put_be16(uint8_t* out,uint16_t v)
    {
        out[1] = v & 0xff;
        out[0] = (v >> 8) & 0xff;
        return out + 2;
    }

    uint8_t* session::put_be24(uint8_t* out,uint32_t v)
    {
        out[2] = v & 0xff;
        out[1] = (v >> 8) & 0xff;
        out[0] = (v >> 16) & 0xff;
        return out + 3;  
    }

    uint8_t* session::put_be32(uint8_t* out,uint32_t  v)
    {
        out[3] = v & 0xff;
        out[2] = (v >> 8) & 0xff;
        out[1] = (v >> 16) & 0xff;
        out[0] = (v >> 24) & 0xff;
        return out + 4;   
    }

    uint8_t* session::put_be64(uint8_t* out,uint64_t  v)
    {
        out = put_be32(out, (v >> 32) & 0xffff'ffff);    
        out = put_be32(out, v & 0xffff'ffff);    
        return out; 
    }

    uint8_t* session::put_amf_string(uint8_t* out, const char *str)
    {
        uint16_t len = strlen(str);    
        out = put_be16(out,len);    
        memcpy(out,str,len);    
        return out + len;   
    }

    uint8_t* session::put_amf_double(uint8_t* out, double d)
    {
        *out++ = AMF_NUMBER;
        uint8_t *ci, *co;
        ci = (uint8_t *)&d;
        co = (uint8_t *)out;
        co[0] = ci[7];
        co[1] = ci[6];
        co[2] = ci[5];
        co[3] = ci[4];
        co[4] = ci[3];
        co[5] = ci[2];
        co[6] = ci[1];
        co[7] = ci[0];
        return out + 8;
    }

}}//namespace
