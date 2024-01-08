#include <rtsp_request_handler.h>
#include <boost/lexical_cast.hpp>
#include <session.h>
#include <request.h>
#include <stream_video_handler.h>
#include <h264_rtp_serialize.h>
#include <h265_rtp_serialize.h>
#include <mjpeg_rtp_serialize.h>
#include <rtp_udp_session.h>
#include <rtp_tcp_session.h>
#include <boost/date_time.hpp>
#include <rtsp_log.h>

namespace ceanic{namespace rtsp{

    short rtsp_request_handler::udp_base_port = 5000;
    std::mutex rtsp_request_handler::udp_port_mutex;

    bool rtsp_request_handler::bind_udp_port(short port)
    {
        int s = socket(AF_INET, SOCK_DGRAM, 0);
        bool ret = false;

        struct sockaddr_in svr_addr;
        memset(&svr_addr, 0, sizeof(svr_addr));
        svr_addr.sin_family = AF_INET;
        svr_addr.sin_port = htons(port);
        svr_addr.sin_addr.s_addr = htonl(INADDR_ANY);    
        int len = sizeof(svr_addr);
        if (bind(s,(sockaddr*)&svr_addr, len) == 0)
        {
            ret = true;
        }

        close(s);

        return ret;
    }

    short rtsp_request_handler::get_udp_port()
    {
        std::unique_lock<std::mutex> lock(udp_port_mutex);

        short port = udp_base_port;

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
            m_video_handler.reset();
        }

        stream_manager::instance()->del_stream(m_stream->chn(),m_stream->stream_id());

        m_stream.reset();
    }

    bool rtsp_request_handler::get_channel(std::string& uri, int& chn)
    {
        std::string::size_type pos = uri.rfind(std::string("/stream"));
        if (pos == std::string::npos) 
            return false;

        const char *p = uri.c_str()+pos+1;

        int ch;
        int ret = 0;
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
        for (unsigned int i = 0; i < req.headers.size(); i++)
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
        str += boost::lexical_cast<std::string>(m_seq);
        str += "\r\n";
        str += "\r\n";

        sess.send_packet_n(str.c_str(), str.size());
    }

    void rtsp_request_handler::process_method_unsupport(const request& req, session& sess)
    {
        std::string str = "RTSP/1.0 405 Method Not Allowed\r\n";
        str += "CSeq: ";
        str += boost::lexical_cast<std::string>(m_seq);
        str += "\r\n";
        str += "\r\n";

        sess.send_packet_n(str.c_str(), str.size());
    }

    unsigned int rtsp_request_handler::get_session_no()
    {
        srand(time(NULL));
        return rand() % 99999999;
    }

    bool rtsp_request_handler::get_seq_id(const request& req, int& seq)
    {
        for (unsigned int i = 0; i < req.headers.size(); i++)
        {
            if (strcasecmp(req.headers[i].name.c_str(),"CSeq") == 0)
            {
                seq =  boost::lexical_cast<int>(req.headers[i].value);
                return true;
            }
        }

        return false;
    }

    void rtsp_request_handler::process_method_option(const request&req, session& sess)
    {
        std::string str = "RTSP/1.0 200 OK\r\n";
        str += "CSeq: ";
        str += boost::lexical_cast<std::string>(m_seq);
        str += "\r\n";

        str += "Public: OPTIONS, DESCRIBE, SETUP, TEARDOWN, PLAY, GET_PARAMETER, SET_PARAMETER\r\n";
        str += "\r\n";

        sess.send_packet_n(str.c_str(), str.size());
    }

    void rtsp_request_handler::process_method_describe(const request& req, session& sess)
    {
        if (!stream_manager::instance()->get_stream(m_chn / 2, m_chn % 2,m_stream))
        {
            send_faild(sess);
            return;
        }

        if (!m_stream->get_stream_head(&m_mh))
        {
            send_faild(sess);
            return;
        }

        boost::posix_time::ptime start_t(boost::gregorian::date(1970, 1, 1)); 
        boost::posix_time::time_duration duration = boost::posix_time::microsec_clock::universal_time() - start_t;

        std::string sdp_desc;
        sdp_desc = "v=0\r\n";

        //根据rfc2327，加入必须的选项
        sdp_desc += "o=- " + boost::lexical_cast<std::string>(duration.total_milliseconds()) + std::string(" 1 IN IP4 0.0.0.0\r\n");
        sdp_desc += "s=Streamed by Beacon Simple Rtsp Server\r\n";
        sdp_desc += "t=0 0\r\n";

        sdp_desc += "a=range:npt= 0-\r\n";
        if (m_mh.vdec == util::STREAM_ENCODE_H264)
        {
            sdp_desc += "m=video 0 RTP/AVP 96\r\n";
            sdp_desc += "c=IN IP4 0.0.0.0\r\n";
            sdp_desc += "a=rtpmap:96 H264/90000\r\n";
        }
        else if (m_mh.vdec == util::STREAM_ENCODE_H265)
        {
            sdp_desc += "m=video 0 RTP/AVP 96\r\n";
            sdp_desc += "c=IN IP4 0.0.0.0\r\n";
            sdp_desc += "a=rtpmap:96 H265/90000\r\n";
        }
        else if (m_mh.vdec == util::STREAM_ENCODE_MJPEG)
        {
            sdp_desc += "m=video 0 RTP/AVP 26\r\n";
            sdp_desc += "c=IN IP4 0.0.0.0\r\n";
        }
        else
        {
            send_faild(sess);
            return;
        }
        sdp_desc = sdp_desc + std::string("a=control:") + req.uri + std::string("/video\r\n");

        sdp_desc += "\r\n";

        std::string str = "RTSP/1.0 200 OK\r\n";
        str += "CSeq: ";
        str += boost::lexical_cast<std::string>(m_seq);
        str += "\r\n";
        str += "Content-Type: application/sdp\r\n";
        str += "Content-Length: ";
        str += boost::lexical_cast<std::string>(sdp_desc.size());
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

        for (unsigned int i = 0; i < req.headers.size(); i++)
        {
            if (strcasecmp(req.headers[i].name.c_str(),"Transport") == 0)
            {
                char str[255];
                sprintf(str,"%s", req.headers[i].value.c_str());
                char* q = strcasestr(str,"client_port=");
                if (q != NULL)
                {
                    q = q + strlen("client_port=");

                    int p1, p2;
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

        if (!stream_manager::instance()->get_stream(m_chn / 2,m_chn % 2, m_stream))
        {
            send_faild(sess);
            return;
        }

        std::string::size_type pos = req.uri.rfind(std::string("/audio"));
        bool is_video = (pos == std::string::npos);

        transport_info transport;
        get_transport(req, transport, sess);

        std::string transport_str;
        short server_port = transport.client_port[0];
        int interleaved = is_video ? 0 : 2;

        if (transport.mode == UDP_MODE)
        {
            transport_str += "RTP/AVP;";
            transport_str += "unicast;";

            transport_str += "client_port=";
            transport_str += boost::lexical_cast<std::string>(transport.client_port[0]);
            transport_str += "-";
            transport_str += boost::lexical_cast<std::string>(transport.client_port[1]);
            transport_str += ";";

            server_port = rtsp_request_handler::get_udp_port();
            transport_str += "server_port=";
            transport_str += boost::lexical_cast<std::string>(server_port);
            transport_str += "-";
            transport_str += boost::lexical_cast<std::string>(server_port + 1);
            transport_str += ";";
        }
        else
        {
            transport_str += "RTP/AVP/TCP;";
            transport_str += "unicast;";

            transport_str += "interleaved=";
            transport_str += boost::lexical_cast<std::string>(interleaved);
            transport_str += "-";
            transport_str += boost::lexical_cast<std::string>(interleaved + 1);
            transport_str += ";";
        }

        m_session_no = get_session_no();

        std::string str = "RTSP/1.0 200 OK\r\n";
        str += "CSeq: ";
        str += boost::lexical_cast<std::string>(m_seq);
        str += "\r\n";
        str += "Transport: ";
        str += transport_str;
        str += "\r\n";
        str += "Session: ";
        str += boost::lexical_cast<std::string>(m_session_no);
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
            if (m_mh.vdec == util::STREAM_ENCODE_H264)
            {
                rtp_serialize = rtp_serialize_ptr(new h264_rtp_serialize(96));
            }
            else if(m_mh.vdec == util::STREAM_ENCODE_H265)
            {
                rtp_serialize = rtp_serialize_ptr(new h265_rtp_serialize(96));
            }
            else if (m_mh.vdec == util::STREAM_ENCODE_MJPEG)
            {
                rtp_serialize = rtp_serialize_ptr(new mjpeg_rtp_serialize());
            }
            else 
            {
            }
            m_video_handler = stream_handler_ptr(new stream_video_handler(rtp_session, rtp_serialize));
            m_stream->register_stream_observer(m_video_handler);
        }
        else
        {
            //TODO
        }

        m_stream->request_i_frame();

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
        str += boost::lexical_cast<std::string>(m_seq);
        str += "\r\n";

        str += "Range: npt=now-\r\n";
        str += "Session: ";
        str += boost::lexical_cast<std::string>(m_session_no);
        str += "\r\n";
        str += "\r\n";

        sess.send_packet_n(str.c_str(), str.size());
        m_state = RTSP_STATE_PLAYING;

        if (m_video_handler)
        {
            m_video_handler->start();
        }

        m_stream->request_i_frame();
    }

    void rtsp_request_handler::process_method_teardown(const request& req, session& sess)
    {
        std::string str = "RTSP/1.0 200 SUCCESS\r\n";
        str += "CSeq: ";
        str += boost::lexical_cast<std::string>(m_seq);
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
        str += boost::lexical_cast<std::string>(m_seq);
        str += "\r\n";
        str += "\r\n";

        sess.send_packet_n(str.c_str(), str.size());
    }

}}//namespace


