#include "request_parser.h"
#include "request.h"
#include <string.h>
#include <boost/lexical_cast.hpp>
#include <util/std.h>

namespace ceanic{namespace rtsp{

    request_parser::request_parser()
        : state_(method_start)
    {
    }

    void request_parser::reset()
    {
        state_ = method_start;
    }

    boost::tribool request_parser::parse(request& req, const char* buf, int len, int *left)
    {
        if (req.head_flag)
        {
            //head ok, recv data
            int need_data_len = req.content_len - req.data_len;
            if (len  >= need_data_len)
            {
                memcpy(req.data + req.data_len, buf, need_data_len);
                req.data_len += need_data_len;
                *left = len - need_data_len;

                //one protocol ok
                return true;
            }
            else
            {
                memcpy(req.data + req.data_len, buf, len);
                req.data_len += len;
                *left = 0;

                //need more data
                return boost::indeterminate;
            }
        }
        else
        {
            //begin recv head
            const char* p = buf;
            while (p < buf + len)
            {
                boost::tribool result = consume(req,*p);
                if (result)
                {
                    //head ok
                    req.head_flag = true;
                    req.content_len = 0;
                    req.data_len = 0;
                    printf(">>>>>>>>>>>>size =%d\n", req.headers.size());
                    for (size_t i = 0; i < req.headers.size();i++)
                    {
                        printf(">>>>>>>>>>>>name =%s\n", req.headers[i].name.c_str());
                        if (req.headers[i].name == "Content-Length")
                        {
                            req.content_len = boost::lexical_cast<int>(req.headers[i].value);
                            break;
                        }
                    }
                    if (req.content_len > req.data_capacity)
                    {
                        delete[] req.data;
                        req.data_capacity = req.content_len;
                        req.data = new char[req.data_capacity];
                    }

                    int process_len = p - buf + 1;
                    *left = len - process_len;

                    printf("---------content len %d, left %d\n--------", req.content_len,*left);
                    if (*left >= req.content_len)
                    {
                        memcpy(req.data, p + 1, req.content_len);
                        req.data_len += req.content_len;
                        *left -= req.content_len;
                        return  true;
                    }
                    else
                    {
                        memcpy(req.data, p + 1,*left);
                        req.data_len += (*left);
                        *left = 0;
                        return boost::indeterminate;
                    }
                }
                else if (!result)
                {
                    //data error
                    return false;
                }

                //need more data to parse head
                p ++;
            }

            //here need more data
            *left = 0;
            return boost::indeterminate;
        }
    }

    boost::tribool request_parser::consume(request& req, char input)
    {
        switch (state_)
        {
            case method_start:
                if (!is_char(input) || is_ctl(input) || is_tspecial(input))
                {
                    return false;
                }
                else
                {
                    state_ = method;
                    req.method.push_back(input);
                    return boost::indeterminate;
                }
            case method:
                if (input == ' ')
                {
                    state_ = uri;
                    return boost::indeterminate;
                }
                else if (!is_char(input) || is_ctl(input) || is_tspecial(input))
                {
                    return false;
                }
                else
                {
                    req.method.push_back(input);
                    return boost::indeterminate;
                }
            case uri_start:
                if (is_ctl(input))
                {
                    return false;
                }
                else
                {
                    state_ = uri;
                    req.uri.push_back(input);
                    return boost::indeterminate;
                }
            case uri:
                if (input == ' ')
                {
                    state_ = http_version_h;
                    return boost::indeterminate;
                }
                else if (is_ctl(input))
                {
                    return false;
                }
                else
                {
                    req.uri.push_back(input);
                    return boost::indeterminate;
                }
            case rtsp_version_r:
                {
                    if (input == 'R')
                    {
                        state_ = rtsp_version_t;
                        return boost::indeterminate;
                    }
                    else
                    {
                        return false;
                    }
                }
            case rtsp_version_t:
                {
                    if (input == 'T')
                    {
                        state_ = rtsp_version_s;
                        return boost::indeterminate;
                    }
                    else
                    {
                        return false;
                    }
                }
            case rtsp_version_s:
                {
                    if (input == 'S')
                    {
                        state_ = rtsp_version_p;
                        return boost::indeterminate;
                    }
                    else
                    {
                        return false;
                    }
                }
            case rtsp_version_p:
                {
                    if (input == 'P')
                    {
                        state_ = version_slash;
                        return boost::indeterminate;
                    }
                    else
                    {
                        return false;
                    }
                }
            case http_version_h:
                if (input == 'H')
                {
                    state_ = http_version_t_1;
                    return boost::indeterminate;
                }
                else if (input == 'R')
                {
                    state_ = rtsp_version_t;
                    return boost::indeterminate;
                }
                else
                {
                    return false;
                }
            case http_version_t_1:
                if (input == 'T')
                {
                    state_ = http_version_t_2;
                    return boost::indeterminate;
                }
                else
                {
                    return false;
                }
            case http_version_t_2:
                if (input == 'T')
                {
                    state_ = http_version_p;
                    return boost::indeterminate;
                }
                else
                {
                    return false;
                }
            case http_version_p:
                if (input == 'P')
                {
                    state_ = version_slash;
                    return boost::indeterminate;
                }
                else
                {
                    return false;
                }
            case version_slash:
                if (input == '/')
                {
                    req.version_major = 0;
                    req.version_minor = 0;
                    state_ = version_major_start;
                    return boost::indeterminate;
                }
                else
                {
                    return false;
                }
            case version_major_start:
                if (is_digit(input))
                {
                    req.version_major = req.version_major * 10 + input - '0';
                    state_ = version_major;
                    return boost::indeterminate;
                }
                else
                {
                    return false;
                }
            case version_major:
                if (input == '.')
                {
                    state_ = version_minor_start;
                    return boost::indeterminate;
                }
                else if (is_digit(input))
                {
                    req.version_major = req.version_major * 10 + input - '0';
                    return boost::indeterminate;
                }
                else
                {
                    return false;
                }
            case version_minor_start:
                if (is_digit(input))
                {
                    req.version_minor = req.version_minor * 10 + input - '0';
                    state_ = version_minor;
                    return boost::indeterminate;
                }
                else
                {
                    return false;
                }
            case version_minor:
                if (input == '\r')
                {
                    state_ = expecting_newline_1;
                    return boost::indeterminate;
                }
                else if (is_digit(input))
                {
                    req.version_minor = req.version_minor * 10 + input - '0';
                    return boost::indeterminate;
                }
                else
                {
                    return false;
                }
            case expecting_newline_1:
                if (input == '\n')
                {
                    state_ = header_line_start;
                    return boost::indeterminate;
                }
                else
                {
                    return false;
                }
            case header_line_start:
                if (input == '\r')
                {
                    state_ = expecting_newline_3;
                    return boost::indeterminate;
                }
                else if (!req.headers.empty() && (input == ' ' || input == '\t'))
                {
                    state_ = header_lws;
                    return boost::indeterminate;
                }
                else if (!is_char(input) || is_ctl(input) || is_tspecial(input))
                {
                    return false;
                }
                else
                {
                    req.headers.push_back(header());
                    req.headers.back().name.push_back(input);
                    state_ = header_name;
                    return boost::indeterminate;
                }
            case header_lws:
                if (input == '\r')
                {
                    state_ = expecting_newline_2;
                    return boost::indeterminate;
                }
                else if (input == ' ' || input == '\t')
                {
                    return boost::indeterminate;
                }
                else if (is_ctl(input))
                {
                    return false;
                }
                else
                {
                    state_ = header_value;
                    req.headers.back().value.push_back(input);
                    return boost::indeterminate;
                }
            case header_name:
                if (input == ':')
                {
                    state_ = space_before_header_value;
                    return boost::indeterminate;
                }
                else if (!is_char(input) || is_ctl(input) || is_tspecial(input))
                {
                    return false;
                }
                else
                {
                    req.headers.back().name.push_back(input);
                    return boost::indeterminate;
                }
            case space_before_header_value:
                if (input == ' ')
                {
                    state_ = header_value;
                    return boost::indeterminate;
                }
                else
                {
                    return false;
                }
            case header_value:
                if (input == '\r')
                {
                    state_ = expecting_newline_2;
                    return boost::indeterminate;
                }
                else if (is_ctl(input))
                {
                    return false;
                }
                else
                {
                    req.headers.back().value.push_back(input);
                    return boost::indeterminate;
                }
            case expecting_newline_2:
                if (input == '\n')
                {
                    state_ = header_line_start;
                    return boost::indeterminate;
                }
                else
                {
                    return false;
                }
            case expecting_newline_3:
                return (input == '\n');
            default:
                return false;
        }
    }

    bool request_parser::is_char(int c)
    {
        return c >= 0 && c <= 127;
    }

    bool request_parser::is_ctl(int c)
    {
        return (c >= 0 && c <= 31) || (c == 127);
    }

    bool request_parser::is_tspecial(int c)
    {
        switch (c)
        {
            case '(': case ')': case '<': case '>': case '@':
            case ',': case ';': case ':': case '\\': case '"':
            case '/': case '[': case ']': case '?': case '=':
            case '{': case '}': case ' ': case '\t':
                return true;
            default:
                return false;
        }
    }

    bool request_parser::is_digit(int c)
    {
        return c >= '0' && c <= '9';
    }

}}//namespace

