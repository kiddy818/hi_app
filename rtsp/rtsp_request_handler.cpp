#include <rtsp_request_handler.h>
#include <session.h>
#include <request.h>
#include <stream_video_handler.h>
#include <stream_audio_handler.h>
#include <h264_rtp_serialize.h>
#include <h265_rtp_serialize.h>
#include <pcmu_rtp_serialize.h>
#include <aac_rtp_serialize.h>
#include <rtp_udp_session.h>
#include <rtp_tcp_session.h>
#include <rtsp_log.h>

namespace ceanic{namespace rtsp{

    int16_t rtsp_request_handler::udp_base_port = 5000;
    std::mutex rtsp_request_handler::udp_port_mutex;

    bool rtsp_request_handler::bind_udp_port(int16_t port)
    {
        int32_t s = socket(AF_INET, SOCK_DGRAM, 0);
        bool ret = false;

        struct sockaddr_in svr_addr;
        memset(&svr_addr, 0, sizeof(svr_addr));
        svr_addr.sin_family = AF_INET;
        svr_addr.sin_port = htons(port);
        svr_addr.sin_addr.s_addr = htonl(INADDR_ANY);    
        int32_t len = sizeof(svr_addr);
        if (bind(s,(sockaddr*)&svr_addr, len) == 0)
        {
            ret = true;
        }

        close(s);

        return ret;
    }

    int16_t rtsp_request_handler::get_udp_port()
    {
        std::unique_lock<std::mutex> lock(udp_port_mutex);

        int16_t port = udp_base_port;

        while (!bind_udp_port(port)
                || !bind_udp_port(port + 1))
        {
            port ++;
            if (port > 8000)
            {
                port = 5000;
            }
        }

        udp_base_port = port + 2;

        return port;
    }

    rtsp_request_handler::rtsp_request_handler()
        :request_handler(), m_state(RTSP_STATE_IDLE)
    {
        memset(&m_mh, 0, sizeof(m_mh));
    }

    rtsp_request_handler::~rtsp_request_handler()
    {
        if (m_stream)
        {
            stop_play();
        }
    }

    RtspState rtsp_request_handler::state()
    {
        return m_state;
    }

    void rtsp_request_handler::stop_play()
    {
        if (!m_stream)
        {
            return;
        }

        if (m_video_handler)
        {
            m_video_handler->stop();
            m_stream->unregister_stream_observer(m_video_handler);
            m_video_handler = nullptr;
        }

        if(m_audio_handler)
        {
            m_audio_handler->stop();
            m_stream->unregister_stream_observer(m_audio_handler);
            m_audio_handler = nullptr;
        }

        stream_manager::instance()->del_stream(m_stream->chn(),m_stream->stream_id());

        m_stream = nullptr;
    }

    bool rtsp_request_handler::get_channel(std::string& uri, int& chn)
    {
        std::string::size_type pos = uri.rfind(std::string("/stream"));
        if (pos == std::string::npos) 
            return false;

        const char *p = uri.c_str()+pos+1;

        int32_t ch;
        int32_t ret = 0;
        if (sscanf(p,"stream%d",&ch)== 1) {
            chn = ch-1;
            ret = (chn >= 0);
        }
        else
            ret = false;

        return ret;
    }

    void rtsp_request_handler::handle_request(const request& req, session& sess)
    {
#if 0
        std::cout << "------------rtsp request-------------" << std::endl;
        std::cout << "method: " << req.method << std::endl;
        std::cout << "uri: " << req.uri << std::endl;
        for (unsigned int32_t i = 0; i < req.headers.size(); i++)
        {
            std::cout << req.headers[i].name << ":  " << req.headers[i].value << std::endl;
        }
        std::cout << "------------rtsp request------------" << std::endl;
#endif

        std::string uri = req.uri;
        if (!get_channel(uri, m_chn))
        {
            RTSP_WRITE_LOG_ERROR("get channel from url(%s)failed",uri.c_str());
            send_faild(sess);
            return ;
        }

        get_seq_id(req, m_seq);

        if (req.method == "OPTIONS")
        {
            RTSP_WRITE_LOG_INFO("options");
            process_method_option(req, sess);
        }
        else if (req.method == "DESCRIBE")
        {
            RTSP_WRITE_LOG_INFO("describe");
            process_method_describe(req, sess);
        }
        else if (req.method == "SETUP")
        {
            RTSP_WRITE_LOG_INFO("setup");
            process_method_setup(req, sess);
        }
        else if (req.method == "PLAY")
        {
            RTSP_WRITE_LOG_INFO("play");
            process_method_play(req, sess);
        }else if (req.method == "TEARDOWN")
        {
            RTSP_WRITE_LOG_INFO("teardown");
            process_method_teardown(req, sess);
        }else if (req.method == "SET_PARAMETER")
        {
            RTSP_WRITE_LOG_INFO("set parameter");
            process_method_set_parameter(req, sess);
        }
        else if (req.method == "GET_PARAMETER")
        {
            RTSP_WRITE_LOG_INFO("get parameter");
            process_method_set_parameter(req, sess);
        }
        else
        {
            RTSP_WRITE_LOG_ERROR("unsport method %s\n",req.method.c_str());
            process_method_unsupport(req, sess);
        }
    }

    void rtsp_request_handler::process_method_set_parameter(const request& req, session& sess)
    {
        std::string str = "RTSP/1.0 200 OK\r\n";
        str += "CSeq: ";
        str += std::to_string(m_seq);
        str += "\r\n";
        str += "\r\n";

        sess.send_packet_n(str.c_str(), str.size());
    }

    void rtsp_request_handler::process_method_unsupport(const request& req, session& sess)
    {
        std::string str = "RTSP/1.0 405 Method Not Allowed\r\n";
        str += "CSeq: ";
        str += std::to_string(m_seq);
        str += "\r\n";
        str += "\r\n";

        sess.send_packet_n(str.c_str(), str.size());
    }

    uint32_t rtsp_request_handler::get_session_no()
    {
        srand(time(NULL));
        return rand() % 99999999;
    }

    bool rtsp_request_handler::get_seq_id(const request& req, int& seq)
    {
        for (uint32_t i = 0; i < req.headers.size(); i++)
        {
            if (strcasecmp(req.headers[i].name.c_str(),"CSeq") == 0)
            {
                seq =  std::atoi(req.headers[i].value.c_str());
                return true;
            }
        }

        return false;
    }

    void rtsp_request_handler::process_method_option(const request&req, session& sess)
    {
        std::string str = "RTSP/1.0 200 OK\r\n";
        str += "CSeq: ";
        str += std::to_string(m_seq);
        str += "\r\n";

        str += "Public: OPTIONS, DESCRIBE, SETUP, TEARDOWN, PLAY, GET_PARAMETER, SET_PARAMETER\r\n";
        str += "\r\n";

        sess.send_packet_n(str.c_str(), str.size());
    }

    void rtsp_request_handler::process_method_describe(const request& req, session& sess)
    {
        if (!stream_manager::instance()->get_stream(m_chn / 3, m_chn % 3,m_stream))
        {
            send_faild(sess);
            return;
        }

        if(!stream_manager::instance()->get_stream_head(m_stream->chn(),m_stream->stream_id(),&m_mh))
        {
            send_faild(sess);
            return;
        }

        std::string sdp_desc;
        sdp_desc = "v=0\r\n";

        //根据rfc2327，加入必须的选项
        sdp_desc += "o=- " + std::to_string(std::time(nullptr) * 1000) + std::string(" 1 IN IP4 0.0.0.0\r\n");
        sdp_desc += "s=Streamed by Ceanic Simple Rtsp Server\r\n";
        sdp_desc += "t=0 0\r\n";

        sdp_desc += "a=range:npt= 0-\r\n";
        if (m_mh.video_info.vcode == util::STREAM_VIDEO_ENCODE_H264)
        {
            sdp_desc += "m=video 0 RTP/AVP 96\r\n";
            sdp_desc += "c=IN IP4 0.0.0.0\r\n";
            sdp_desc += "a=rtpmap:96 H264/90000\r\n";
        }
        else if (m_mh.video_info.vcode == util::STREAM_VIDEO_ENCODE_H265)
        {
            sdp_desc += "m=video 0 RTP/AVP 96\r\n";
            sdp_desc += "c=IN IP4 0.0.0.0\r\n";
            sdp_desc += "a=rtpmap:96 H265/90000\r\n";
        }
        else
        {
            send_faild(sess);
            return;
        }
        sdp_desc = sdp_desc + std::string("a=control:") + req.uri + std::string("/video\r\n");

        if(m_mh.audio_info.acode == util::STREAM_AUDIO_ENCODE_G711U)
        {
            sdp_desc += "m=audio 0 RTP/AVP 0\r\n";
            sdp_desc += "c=IN IP4 0.0.0.0\r\n";
            sdp_desc += "a=rtpmap:0 pcmu/8000/1\r\n";
            sdp_desc = sdp_desc + std::string("a=control:") + req.uri + std::string("/audio\r\n");
        }
        else if(m_mh.audio_info.acode == util::STREAM_AUDIO_ENCODE_AAC)
        {
            sdp_desc += "m=audio 0 RTP/AVP 97\r\n";
            sdp_desc += "c=IN 0.0.0.0\r\n";
            sdp_desc = sdp_desc + "a=rtpmap:97 mpeg4-generic/" + std::to_string(m_mh.audio_info.sample_rate) + "/" + std::to_string(m_mh.audio_info.chn) + "\r\n";
            sdp_desc = sdp_desc + std::string("a=control:") + req.uri + std::string("/audio\r\n");

            std::string str_audio_cfg;
            aac_rtp_serialize::get_config(1/*profile AACLC*/,m_mh.audio_info.sample_rate,m_mh.audio_info.chn,str_audio_cfg);
            sdp_desc += "a=fmtp:97 stream_type=5;profile-level-id=1;mode=AAC-hbr;sizeLength=13;indexLength=3;config=" + str_audio_cfg + ";constantDuration=1024\r\n";
        }

        sdp_desc += "\r\n";

        std::string str = "RTSP/1.0 200 OK\r\n";
        str += "CSeq: ";
        str += std::to_string(m_seq);
        str += "\r\n";
        str += "Content-Type: application/sdp\r\n";
        str += "Content-Length: ";
        str += std::to_string(sdp_desc.size());
        str += "\r\n";
        str += "\r\n";

        sess.send_packet_n(str.c_str(), str.size());
        sess.send_packet_n(sdp_desc.c_str(), sdp_desc.size());

        m_state = RTSP_STATE_DESCRIBED;
    }

    bool rtsp_request_handler::get_transport(const request& req, transport_info& transport, session& sess)
    {
        memset(&transport, 0, sizeof(transport));
        sprintf(transport.client_ip,"%s", sess.ip().c_str());
        transport.mode = UDP_MODE;
        transport.client_port[0] = 6000;
        transport.client_port[1] = 6001;

        for (uint32_t i = 0; i < req.headers.size(); i++)
        {
            if (strcasecmp(req.headers[i].name.c_str(),"Transport") == 0)
            {
                char str[255];
                sprintf(str,"%s", req.headers[i].value.c_str());
                char* q = strcasestr(str,"client_port=");
                if (q != NULL)
                {
                    q = q + strlen("client_port=");

                    int32_t p1, p2;
                    if (sscanf(q,"%d-%d",&p1,&p2) == 2)
                    {
                        transport.client_port[0] = p1;
                        transport.client_port[1] = p2;
                    }
                    else if (scanf(q,"%d",&p1) == 1)
                    {
                        transport.client_port[0] = p1;
                    }
                }

                q = strcasestr(str,"RTP/AVP/TCP");
                if (q != NULL)
                {
                    transport.mode = TCP_MODE;
                    transport.client_port[0] = 0;
                    transport.client_port[1] = 1;
                }

                return true;
            }
        }

        return false;
    }

    void rtsp_request_handler::process_method_setup(const request& req, session& sess)
    {
        if (m_state == RTSP_STATE_PLAYING)
        {
            send_faild(sess);
            return;
        }

        if (!stream_manager::instance()->get_stream(m_chn / 3,m_chn % 3, m_stream))
        {
            send_faild(sess);
            return;
        }

        std::string::size_type pos = req.uri.rfind(std::string("/audio"));
        bool is_video = (pos == std::string::npos);

        transport_info transport;
        get_transport(req, transport, sess);

        std::string transport_str;
        int16_t server_port = transport.client_port[0];
        int32_t interleaved = is_video ? 0 : 2;

        if (transport.mode == UDP_MODE)
        {
            transport_str += "RTP/AVP;";
            transport_str += "unicast;";

            transport_str += "client_port=";
            transport_str += std::to_string(transport.client_port[0]);
            transport_str += "-";
            transport_str += std::to_string(transport.client_port[1]);
            transport_str += ";";

            server_port = rtsp_request_handler::get_udp_port();
            transport_str += "server_port=";
            transport_str += std::to_string(server_port);
            transport_str += "-";
            transport_str += std::to_string(server_port + 1);
            transport_str += ";";
        }
        else
        {
            transport_str += "RTP/AVP/TCP;";
            transport_str += "unicast;";

            transport_str += "interleaved=";
            transport_str += std::to_string(interleaved);
            transport_str += "-";
            transport_str += std::to_string(interleaved + 1);
            transport_str += ";";
        }

        m_session_no = get_session_no();

        std::string str = "RTSP/1.0 200 OK\r\n";
        str += "CSeq: ";
        str += std::to_string(m_seq);
        str += "\r\n";
        str += "Transport: ";
        str += transport_str;
        str += "\r\n";
        str += "Session: ";
        str += std::to_string(m_session_no);
        str += "; timeout=60";
        str += "\r\n";
        str += "\r\n";

        sess.send_packet_n(str.c_str(), str.size());

        rtp_session_ptr rtp_session;
        rtp_serialize_ptr rtp_serialize;

        if (transport.mode == UDP_MODE)
        {
            rtp_session = rtp_session_ptr(new rtp_udp_session(
                        transport.client_ip,
                        transport.client_port[0],
                        transport.client_port[1],
                        NULL,
                        server_port,
                        server_port + 1));
        }
        else
        {
            rtp_session = rtp_session_ptr(new rtp_tcp_session(
                        sess,
                        interleaved,
                        interleaved + 1));
        }

        if (is_video)
        {
            //96,97,48000等值需和describe中的值匹配
            if (m_mh.video_info.vcode == util::STREAM_VIDEO_ENCODE_H264)
            {
                rtp_serialize = rtp_serialize_ptr(new h264_rtp_serialize(96));
            }
            else if(m_mh.video_info.vcode == util::STREAM_VIDEO_ENCODE_H265)
            {
                rtp_serialize = rtp_serialize_ptr(new h265_rtp_serialize(96));
            }
            else 
            {
                RTSP_WRITE_LOG_ERROR("unsupported vdec code:%d",m_mh.video_info.vcode);
                send_faild(sess);
                return;
            }
            m_video_handler = stream_handler_ptr(new stream_video_handler(rtp_session, rtp_serialize));
            m_stream->register_stream_observer(m_video_handler);
        }
        else
        {
            if(m_mh.audio_info.acode == util::STREAM_AUDIO_ENCODE_G711U)
            {
                rtp_serialize = rtp_serialize_ptr(new pcmu_rtp_serialize());
            }
            else if(m_mh.audio_info.acode == util::STREAM_AUDIO_ENCODE_AAC)
            {
                rtp_serialize = rtp_serialize_ptr(new aac_rtp_serialize(97,m_mh.audio_info.sample_rate));
            }
            else
            {
                RTSP_WRITE_LOG_ERROR("unsupported adec code:%d",m_mh.audio_info.acode);
                send_faild(sess);
                return;
            }
            m_audio_handler = stream_handler_ptr(new stream_audio_handler(rtp_session,rtp_serialize));
            m_stream->register_stream_observer(m_audio_handler);
        }

        m_state = RTSP_STATE_SETUPED;
    }

    void rtsp_request_handler::process_method_play(const request& req, session& sess)
    {
        if (m_state != RTSP_STATE_SETUPED)
        {
            send_faild(sess);
            return;
        }

        std::string str = "RTSP/1.0 200 OK\r\n";
        str += "CSeq: ";
        str += std::to_string(m_seq);
        str += "\r\n";

        str += "Range: npt=now-\r\n";
        str += "Session: ";
        str += std::to_string(m_session_no);
        str += "\r\n";
        str += "\r\n";

        sess.send_packet_n(str.c_str(), str.size());
        m_state = RTSP_STATE_PLAYING;

        if (m_video_handler)
        {
            m_video_handler->start();
        }

        if(m_audio_handler)
        {
            m_audio_handler->start();
        }

        stream_manager::instance()->request_i_frame(m_stream->chn(),m_stream->stream_id());
    }

    void rtsp_request_handler::process_method_teardown(const request& req, session& sess)
    {
        std::string str = "RTSP/1.0 200 SUCCESS\r\n";
        str += "CSeq: ";
        str += std::to_string(m_seq);
        str += "\r\n";
        str += "\r\n";

        if (m_state == RTSP_STATE_PLAYING)
        {
            m_state = RTSP_STATE_IDLE;
            stop_play();
        }

        sess.send_packet_n(str.c_str(), str.size());
        shutdown(sess.socket(), SHUT_RDWR);
    }

    void rtsp_request_handler::send_faild(session& sess)
    {
        RTSP_WRITE_LOG_ERROR("socket(%d) send failed",sess.socket());
        std::string str;
        str += "RTSP/1.0 400 FAILED\r\n";
        str += "CSeq: ";
        str += std::to_string(m_seq);
        str += "\r\n";
        str += "\r\n";

        sess.send_packet_n(str.c_str(), str.size());
    }

}}//namespace


