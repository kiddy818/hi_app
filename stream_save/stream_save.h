#ifndef stream_save_include_h
#define stream_save_include_h

#include <util/stream_type.h>
namespace ceanic{namespace stream_save{

    class stream_save
    {
        public:
            stream_save(){}
            virtual ~stream_save(){}

        public:
            virtual bool open() = 0;
            virtual void close() = 0;
            virtual bool is_open() = 0;
            virtual bool input_data(ceanic::util::stream_head* head,const char* buf,int len) = 0;
    };

}}//namespace

#endif
