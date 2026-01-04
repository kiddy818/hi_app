#include "request_parser.h"
#include "request.h"
#include <string.h>
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

    std::optional<bool> request_parser::parse(request& req, const char* buf, int32_t len, int32_t *left)
    {
        if (req.head_flag)
        {
            //head ok, recv data
            int32_t need_data_len = req.content_len - req.data_len;
            if (len  >= need_data_len)
            {
                memcpy(req.data + req.data_len, buf, need_data_len);
                req.data_len += need_data_len;
                *left = len - need_data_len;

                //one protocol ok
                return std::optional<bool>(true);
            }
            else
            {
                memcpy(req.data + req.data_len, buf, len);
                req.data_len += len;
                *left = 0;

                //need more data
                return std::nullopt;
            }
        }
        else
        {
            //begin recv head
            const char* p = buf;
            while (p < buf + len)
            {
                std::optional<bool> result = consume(req,*p);
                if (result.has_value() && result.value())
                {
                    //head ok
                    req.head_flag = true;
                    req.content_len = 0;
                    req.data_len = 0;
                    for (size_t i = 0; i < req.headers.size();i++)
                    {
                        if (req.headers[i].name == "Content-Length")
                        {
                            req.content_len = std::atoi(req.headers[i].value.c_str());
                            //req.content_len = boost::lexical_cast<int>(req.headers[i].value);
                            break;
                        }
                    }
                    if (req.content_len > req.data_capacity)
                    {
                        delete[] req.data;
                        req.data_capacity = req.content_len;
                        req.data = new char[req.data_capacity];
                    }

                    int32_t process_len = p - buf + 1;
                    *left = len - process_len;

                    if (*left >= req.content_len)
                    {
                        memcpy(req.data, p + 1, req.content_len);
                        req.data_len += req.content_len;
                        *left -= req.content_len;

                        return std::optional<bool>(true);
                    }
                    else
                    {
                        memcpy(req.data, p + 1,*left);
                        req.data_len += (*left);
                        *left = 0;

                        return std::nullopt;
                    }
                }
                else if (result.has_value() && !result.value())
                {
                    //data error
                    //
                    return std::optional<bool>(false);
                }

                //need more data to parse head
                p ++;
            }

            //here need more data
            *left = 0;
            return std::nullopt;
        }
    }

    std::optional<bool> request_parser::consume(request& req, char input)
    {
        switch (state_)
        {
            case method_start:
                if (!is_char(input) || is_ctl(input) || is_tspecial(input))
                {
                    return std::optional<bool>(false);
                }
                else
                {
                    state_ = method;
                    req.method.push_back(input);
                    return std::nullopt;
                }
            case method:
                if (input == ' ')
                {
                    state_ = uri;
                    return std::nullopt;
                }
                else if (!is_char(input) || is_ctl(input) || is_tspecial(input))
                {
                    return std::optional<bool>(false);
                }
                else
                {
                    req.method.push_back(input);
                    return std::nullopt;
                }
            case uri_start:
                if (is_ctl(input))
                {
                    return std::optional<bool>(false);
                }
                else
                {
                    state_ = uri;
                    req.uri.push_back(input);
                    return std::nullopt;
                }
            case uri:
                if (input == ' ')
                {
                    state_ = http_version_h;
                    return std::nullopt;
                }
                else if (is_ctl(input))
                {
                    return std::optional<bool>(false);
                }
                else
                {
                    req.uri.push_back(input);
                    return std::nullopt;
                }
            case rtsp_version_r:
                {
                    if (input == 'R')
                    {
                        state_ = rtsp_version_t;
                        return std::nullopt;
                    }
                    else
                    {
                        return std::optional<bool>(false);
                    }
                }
            case rtsp_version_t:
                {
                    if (input == 'T')
                    {
                        state_ = rtsp_version_s;
                        return std::nullopt;
                    }
                    else
                    {
                        return std::optional<bool>(false);
                    }
                }
            case rtsp_version_s:
                {
                    if (input == 'S')
                    {
                        state_ = rtsp_version_p;
                        return std::nullopt;
                    }
                    else
                    {
                        return std::optional<bool>(false);
                    }
                }
            case rtsp_version_p:
                {
                    if (input == 'P')
                    {
                        state_ = version_slash;
                        return std::nullopt;
                    }
                    else
                    {
                        return std::optional<bool>(false);
                    }
                }
            case http_version_h:
                if (input == 'H')
                {
                    state_ = http_version_t_1;
                    return std::nullopt;
                }
                else if (input == 'R')
                {
                    state_ = rtsp_version_t;
                    return std::nullopt;
                }
                else
                {
                    return std::optional<bool>(false);
                }
            case http_version_t_1:
                if (input == 'T')
                {
                    state_ = http_version_t_2;
                    return std::nullopt;
                }
                else
                {
                    return std::optional<bool>(false);
                }
            case http_version_t_2:
                if (input == 'T')
                {
                    state_ = http_version_p;
                    return std::nullopt;
                }
                else
                {
                    return std::optional<bool>(false);
                }
            case http_version_p:
                if (input == 'P')
                {
                    state_ = version_slash;
                    return std::nullopt;
                }
                else
                {
                    return std::optional<bool>(false);
                }
            case version_slash:
                if (input == '/')
                {
                    req.version_major = 0;
                    req.version_minor = 0;
                    state_ = version_major_start;
                    return std::nullopt;
                }
                else
                {
                    return std::optional<bool>(false);
                }
            case version_major_start:
                if (is_digit(input))
                {
                    req.version_major = req.version_major * 10 + input - '0';
                    state_ = version_major;
                    return std::nullopt;
                }
                else
                {
                    return std::optional<bool>(false);
                }
            case version_major:
                if (input == '.')
                {
                    state_ = version_minor_start;
                    return std::nullopt;
                }
                else if (is_digit(input))
                {
                    req.version_major = req.version_major * 10 + input - '0';
                    return std::nullopt;
                }
                else
                {
                    return std::optional<bool>(false);
                }
            case version_minor_start:
                if (is_digit(input))
                {
                    req.version_minor = req.version_minor * 10 + input - '0';
                    state_ = version_minor;
                    return std::nullopt;
                }
                else
                {
                    return std::optional<bool>(false);
                }
            case version_minor:
                if (input == '\r')
                {
                    state_ = expecting_newline_1;
                    return std::nullopt;
                }
                else if (is_digit(input))
                {
                    req.version_minor = req.version_minor * 10 + input - '0';
                    return std::nullopt;
                }
                else
                {
                    return std::optional<bool>(false);
                }
            case expecting_newline_1:
                if (input == '\n')
                {
                    state_ = header_line_start;
                    return std::nullopt;
                }
                else
                {
                    return std::optional<bool>(false);
                }
            case header_line_start:
                if (input == '\r')
                {
                    state_ = expecting_newline_3;
                    return std::nullopt;
                }
                else if (!req.headers.empty() && (input == ' ' || input == '\t'))
                {
                    state_ = header_lws;
                    return std::nullopt;
                }
                else if (!is_char(input) || is_ctl(input) || is_tspecial(input))
                {
                    return std::optional<bool>(false);
                }
                else
                {
                    req.headers.push_back(header());
                    req.headers.back().name.push_back(input);
                    state_ = header_name;
                    return std::nullopt;
                }
            case header_lws:
                if (input == '\r')
                {
                    state_ = expecting_newline_2;
                    return std::nullopt;
                }
                else if (input == ' ' || input == '\t')
                {
                    return std::nullopt;
                }
                else if (is_ctl(input))
                {
                    return std::optional<bool>(false);
                }
                else
                {
                    state_ = header_value;
                    req.headers.back().value.push_back(input);
                    return std::nullopt;
                }
            case header_name:
                if (input == ':')
                {
                    state_ = space_before_header_value;
                    return std::nullopt;
                }
                else if (!is_char(input) || is_ctl(input) || is_tspecial(input))
                {
                    return std::optional<bool>(false);
                }
                else
                {
                    req.headers.back().name.push_back(input);
                    return std::nullopt;
                }
            case space_before_header_value:
                if (input == ' ')
                {
                    state_ = header_value;
                    return std::nullopt;
                }
                else
                {
                    return std::optional<bool>(false);
                }
            case header_value:
                if (input == '\r')
                {
                    state_ = expecting_newline_2;
                    return std::nullopt;
                }
                else if (is_ctl(input))
                {
                    return std::optional<bool>(false);
                }
                else
                {
                    req.headers.back().value.push_back(input);
                    return std::nullopt;
                }
            case expecting_newline_2:
                if (input == '\n')
                {
                    state_ = header_line_start;
                    return std::nullopt;
                }
                else
                {
                    return std::optional<bool>(false);
                }
            case expecting_newline_3:
                {
                    if(input == '\n')
                    {
                        return std::optional<bool>(true);
                    }
                    else
                    {
                        return std::optional<bool>(false);
                    }
                }
            default:
                {
                    return std::optional<bool>(false);
                }
        }
    }

    bool request_parser::is_char(int32_t c)
    {
        return c >= 0 && c <= 127;
    }

    bool request_parser::is_ctl(int32_t c)
    {
        return (c >= 0 && c <= 31) || (c == 127);
    }

    bool request_parser::is_tspecial(int32_t c)
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

    bool request_parser::is_digit(int32_t c)
    {
        return c >= '0' && c <= '9';
    }

}}//namespace

