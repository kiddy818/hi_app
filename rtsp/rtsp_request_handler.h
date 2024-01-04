#ifndef rtsp_request_handler_include_h
#define rtsp_request_handler_include_h

#include <request_handler.h>
#include <stream_manager.h>
#include <stream_handler.h>

namespace ceanic{namespace rtsp{

    enum
    {
        TCP_MODE = 0,
        UDP_MODE = 1,
    };

    typedef struct
    {
        char client_ip[32];
        int client_port[2];
        int mode;
    }transport_info;

    typedef enum 
    {
        RTSP_STATE_IDLE = 0,
        RTSP_STATE_DESCRIBED,
        RTSP_STATE_SETUPED,
        RTSP_STATE_PLAYING,
    }RtspState;

    class rtsp_request_handler
        :public request_handler
    {
        public:
            rtsp_request_handler();

            virtual ~rtsp_request_handler();

            virtual void handle_request(const request& req, session& sess);

            void process_method_option(const request& req, session& sess);

            void process_method_describe(const request& req, session& sess);

            void process_method_setup(const request& req, session& sess);

            void process_method_play(const request& req, session& sess);

            void process_method_teardown(const request& req, session& sess);

            void process_method_unsupport(const request& req, session& sess);

            void process_method_set_parameter(const request& req, session& sess);

            bool get_seq_id(const request&req, int& seq);

            bool get_transport(const request& req, transport_info& transport, session& sess);

            unsigned int get_session_no();
            bool get_channel(std::string& uri, int& chn);

            RtspState state();

            bool get_video_handle(stream_handler_ptr& handle)
            {
                if (m_video_handler)
                {
                    handle = m_video_handler;
                    return true;
                }

                return false;
            }

        private:
            void stop_play();

            void send_faild(session& sess);
            int m_session_no;
            int m_seq;
            int m_chn;
            RtspState m_state;

            stream_handler_ptr m_video_handler;
            //stream_handler_ptr m_audio_handler;

            stream_ptr m_stream;
            util::media_head m_mh;

        private:
            static std::mutex udp_port_mutex;
            static short udp_base_port;
            static short get_udp_port();
            static bool bind_udp_port(short port);
    };

}}//namespace

#endif

