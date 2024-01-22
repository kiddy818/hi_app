#include <util/std.h>
#include <server.h>
#include <iostream>
#include <rtsp_session.h>
#include <rtsp_log.h>
#include <functional>

namespace ceanic{namespace rtsp{

    rtsp_server::rtsp_server(short port)
        :m_listen_s(-1), m_port(port), m_is_run(false)
    {
    }

    rtsp_server::~rtsp_server()
    {
        stop();
    }

    bool rtsp_server::run()
    {
        if (m_is_run)
        {
            return false;
        }

        signal(SIGPIPE, SIG_IGN);

        /*create socket*/
        m_listen_s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (m_listen_s < 0)
        {
            return false;
        }

        /*reuse addr*/
        int reuse_addr = 1;
        int res = setsockopt(m_listen_s, SOL_SOCKET, SO_REUSEADDR,(char*)&reuse_addr, sizeof(reuse_addr));
        if (res != 0)
        {
            std::cerr << "reuse address failed" << std::endl;
        }

        int buf_size = 128 * 1024;
        res = setsockopt(m_listen_s, SOL_SOCKET, SO_SNDBUF,(char*)&buf_size, sizeof(int));
        if (res != 0)
        {
            std::cerr << "set send buf size failed" << std::endl;
        }

        /*disable nagle*/
        int no_delay = 1;
        res = setsockopt(m_listen_s, IPPROTO_TCP, TCP_NODELAY,&no_delay, sizeof(no_delay));
        if (res != 0)
        {
            std::cerr << "disable nagle failed" << std::endl;
        }

        /*bind*/
        struct sockaddr_in server_addr;
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(m_port);
        server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        int server_len = sizeof(server_addr);
        if ((res = bind(m_listen_s,(struct sockaddr*)&server_addr, server_len)) != 0)
        {
            std::cerr << "bind failed" << std::endl;
            return false;
        }

        /*listen*/
        res = listen(m_listen_s, 5);
        if (res == -1)
        {
            std::cerr << "bind failed" << std::endl;
            return false;
        }

        m_is_run = true;

        m_thread = std::thread(std::bind(&rtsp_server::on_run, this));
        return true;
    }

    void rtsp_server::stop()
    {
        m_is_run = false;

        m_thread.join();
        m_session_ptrs.clear();
        close(m_listen_s);
    }

    void rtsp_server::on_run()
    {
        struct sockaddr_in client_addr;
        int client_len = sizeof(client_addr);

        fd_set fread;
        fd_set ftemp;
        int max_fds = m_listen_s;

        FD_ZERO(&fread);
        FD_SET(m_listen_s,&fread);

        int result;
        char buf[4096];
        std::list<session_ptr>::iterator it;
        time_t last_timeout_tm = time(NULL);

        while (m_is_run)
        {
            ftemp = fread;

            /*set time out to let user send command to quit*/
            struct timeval time_out;
            time_out.tv_sec = 0;
            time_out.tv_usec = 10000;

            result = select(max_fds + 1,&ftemp, NULL, NULL,&time_out);
            if (result == 0)
            {
                /*time out*/
                time_t cur_timeout_tm = time(NULL);
                for (it = m_session_ptrs.begin();it != m_session_ptrs.end();)
                {
                    if (last_timeout_tm != cur_timeout_tm) 
                    {
                        (*it)->reduce_session_timeout();
                        (*it)->reduce_rtcp_timeout();

                        if((*it)->get_session_timeout() <= 5    
                                && (*it)->get_rtcp_timeout() <= 5)                                                                                                         
                        {
                            RTSP_WRITE_LOG_INFO("socket %d,session timeout=%d,rtcp timeout=%d\n",                                                                          
                                    (*it)->socket(),                
                                    (*it)->get_session_timeout(),   
                                    (*it)->get_rtcp_timeout());                                                                                                            
                        }        

                        if ((*it)->get_session_timeout() == 0
                                && (*it)->get_rtcp_timeout() == 0)
                        {
                            FD_CLR((*it)->socket(),&fread);

                            shutdown((*it)->socket(), SHUT_RDWR);
                            (*it)->stop();

                            it = m_session_ptrs.erase(it);
                            continue;
                        }
                    }

                    (*it)->on_idle();
                    it++;
                }

                if (last_timeout_tm != cur_timeout_tm)
                {
                    last_timeout_tm = cur_timeout_tm;
                }

                continue;
            }
            else if (result == -1)
            {
                /*a error happened*/
                if (errno == EINTR)
                {
                    continue;
                }
                break;
            }

            /*hear, data come*/
            if (FD_ISSET(m_listen_s,&ftemp))
            {
                /*client connect*/
                int s = accept(m_listen_s,(struct sockaddr*)&client_addr,(socklen_t *)&client_len);
                if (s == -1)
                {
                    if (errno == EINTR)
                    {
                        continue;
                    }
                    break;
                }

                char* ip = inet_ntoa(client_addr.sin_addr);
                RTSP_WRITE_LOG_INFO("remote ip:%s,socket:%d connected",ip,s);

                /*set nonblock*/
                int val = fcntl(s, F_GETFL, 0);
                fcntl(s, F_SETFL, val | O_NONBLOCK);

                session_ptr sess(new rtsp_session(s, MAX_SESSION_TIMEOUT + 3));
                sess->start();

                m_session_ptrs.push_back(sess);

                FD_SET(s,&fread);

                if (s > max_fds)
                {
                    max_fds = s;
                }

                if (--result <= 0)
                {
                    continue;
                }
            }

            for (it = m_session_ptrs.begin();it != m_session_ptrs.end(); it++)
            {
                int s = (*it)->socket();
                if (FD_ISSET(s,&ftemp))
                {
                    int recv_len = read(s, buf, 4096);
                    if (recv_len <= 0)
                    {
                        /*client close*/
                        RTSP_WRITE_LOG_INFO("socket(%d) closed",s);

                        /*clear*/
                        FD_CLR(s,&fread);

                        (*it)->stop();
                        m_session_ptrs.erase(it);

                        /*erase must break*/
                        break;
                    }

                    (*it)->handle_read(buf, recv_len);

                    if (--result <= 0)
                    {
                        break;
                    }
                }
            }
        }
    }

}}//namespace

