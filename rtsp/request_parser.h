#ifndef request_parser_include_h
#define request_parser_include_h

#include <boost/logic/tribool.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/shared_ptr.hpp>

namespace beacon{namespace rtsp{

    struct request;

    /// Parser for incoming requests.
    class request_parser
    {
        public:
            /// Construct ready to parse the request method.
            explicit request_parser();

            /// Reset to initial parser state.
            void reset();

            boost::tribool  parse(request& req, const char* buf, int len, int* left);

        private:
            /// Handle the next character of input.
            boost::tribool consume(request& req, char input);

            /// Check if a byte is an HTTP character.
            static bool is_char(int c);

            /// Check if a byte is an HTTP control character.
            static bool is_ctl(int c);

            /// Check if a byte is defined as an HTTP tspecial character.
            static bool is_tspecial(int c);

            /// Check if a byte is a digit.
            static bool is_digit(int c);

            /// The current state of the parser.
            enum state
            {
                method_start,
                method,
                uri_start,
                uri,
                http_version_h,
                http_version_t_1,
                http_version_t_2,
                http_version_p,
                rtsp_version_r,
                rtsp_version_t,
                rtsp_version_s,
                rtsp_version_p,
                version_slash,
                version_major_start,
                version_major,
                version_minor_start,
                version_minor,
                expecting_newline_1,
                header_line_start,
                header_lws,
                header_name,
                space_before_header_value,
                header_value,
                expecting_newline_2,
                expecting_newline_3
            } state_;
    };

    typedef boost::shared_ptr<request_parser> parser_ptr;

}}//namespace

#endif
