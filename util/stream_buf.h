#ifndef stream_buf_include_h
#define stream_buf_include_h
#include <string.h>

namespace beacon{namespace util{

    class stream_buf	
    {
        public:
            explicit stream_buf(int buf_len)
                :m_buf_beg(new char[buf_len])
            {
                m_data_beg = m_buf_beg;
                m_data_len = 0;		
                m_buf_end = m_buf_beg + buf_len;
            }

            ~stream_buf()
            {
                delete[] m_buf_beg;
            }

            stream_buf(const stream_buf&) = delete;

            stream_buf& operator[](const stream_buf&) = delete;

            bool input_data(void* buf,int len)
            {		
                if(get_remain_len() < len)
                {
                    return false;
                }

                char* data_end = m_data_beg + m_data_len;
                if(data_end >= m_buf_end)
                {
                    data_end = m_buf_beg + (data_end - m_buf_end);
                }

                if(data_end + len > m_buf_end)
                {
                    int left =  m_buf_end - data_end;
                    memcpy(data_end,buf,left);

                    memcpy(m_buf_beg,(char*)buf + left,len - left);			
                }
                else
                {
                    memcpy(data_end,buf,len);			
                }

                m_data_len += len;
                return true;
            }

            bool copy_data(void* get_buf,int get_len)
            {
                if(get_stream_len() < get_len)
                {
                    return false;
                }

                if(m_data_beg + get_len > m_buf_end)
                {
                    int left = m_buf_end - m_data_beg;
                    memcpy(get_buf,m_data_beg,left);

                    memcpy((char*)get_buf + left,m_buf_beg,get_len - left);			
                }
                else
                {
                    memcpy(get_buf,m_data_beg,get_len);			
                }		

                return true;
            }

            bool get_data(void* get_buf,int get_len)
            {
                if(get_stream_len() < get_len)
                {
                    return false;
                }

                if(m_data_beg + get_len > m_buf_end)
                {
                    int left = m_buf_end - m_data_beg;
                    memcpy(get_buf,m_data_beg,left);

                    m_data_beg = m_buf_beg;
                    memcpy((char*)get_buf + left,m_data_beg,get_len - left);
                    m_data_beg += (get_len - left);			
                }
                else
                {
                    memcpy(get_buf,m_data_beg,get_len);
                    m_data_beg += get_len;
                }
                m_data_len -= get_len;

                return true;
            }

            int get_remain_len()
            {
                return get_buf_len() - get_stream_len();
            }

            int get_stream_len()
            {
                return m_data_len;
            }

            int get_buf_len()
            {
                return m_buf_end - m_buf_beg;
            }

            bool clear_buf()
            {
                m_data_beg =  m_buf_beg;
                m_data_len = 0;
                return true;
            }

            bool realloc_buf(int len)
            {
                if(len == get_buf_len())
                {
                    return false;
                }

                int stream_len = get_stream_len();
                if(stream_len > len)
                {
                    return false;
                }

                char* new_buf= new char[len];
                if(new_buf == NULL)
                {
                    return false;
                }		

                if(stream_len > 0)
                {
                    copy_data(new_buf,stream_len);
                }

                delete[] m_buf_beg;

                m_buf_beg = new_buf;
                m_buf_end = new_buf + len;
                m_data_beg = m_buf_beg;		
                return true;
            }

        private:	
            char* m_buf_beg;
            char* m_buf_end;

            char* m_data_beg;
            int m_data_len;
    };

    class dybuf
    {
        public:
            explicit dybuf(int init_len = 0)
                :m_buf(NULL)
            {
                reset(init_len);
            }

            ~dybuf()
            {
                reset(0);
            }	

            dybuf(const dybuf&) = delete;
            dybuf& operator[](const dybuf&) = delete;

            void reset_if(int len)
            {
                if(buf_len() < len)
                {
                    reset(len);
                }
            }

            void reset(int len)
            {
                if(m_buf)
                {
                    delete[] m_buf;
                    m_buf = NULL;
                }

                m_buf_len = len;
                if(m_buf_len)
                {
                    m_buf = new char[m_buf_len];
                }
            }

            char* pointer()
            {
                return m_buf;
            }	

            int buf_len()
            {
                return m_buf_len;
            }	

        private:
            char* m_buf;
            int m_buf_len;	
    };

}}//namespace

#endif

