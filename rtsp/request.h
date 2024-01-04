#ifndef request_include_h
#define request_include_h

#include <string>
#include <vector>
#include <string>

namespace ceanic{namespace rtsp{

    struct header
    {
        std::string name;
        std::string value;
    };

    struct request
    {
        request()
        {
            data_len = 0;
            content_len = 0;
            data_capacity = 4096;
            data = new char[data_capacity];
            head_flag = false;
        }

        ~request()
        {
            delete[] data;
        }

        std::string method;
        std::string uri;
        int version_major;
        int version_minor;
        std::vector<header> headers;

        bool head_flag;
        int content_len;
        char * data;
        int data_capacity;
        int data_len;
    };

}}//namespace

#endif
