#ifndef session_manager_include_h
#include <map>
#include <thread>
#include <mutex>
#include <rtmp/session.h>
#include <util/stream_type.h>

namespace beacon{namespace rtmp{

    using sess_ptr = std::shared_ptr<timed_session>;

    class session_manager
    {
        public:
            session_manager(const session_manager& sm) = delete;
            session_manager& operator= (const session_manager& sm) = delete;
            ~session_manager();

            static session_manager* instance();

            bool reset_session_tm(int32_t chn,int32_t stream_id,std::string url,uint32_t max_tm);
            bool create_session(int32_t chn,int32_t stream_id,std::string url,uint32_t max_tm);
            void delete_session(int32_t chn,int32_t stream_id,std::string url);
            void delete_session(int32_t chn,int32_t stream_id);

            void process_data(int32_t chn,int32_t stream_id,util::stream_head* head,const uint8_t*buf,int32_t len);

        private:
            session_manager();
            bool session_exists(int32_t chn,int32_t stream_id,std::string url);

        private:
            typedef struct
            {
                int32_t chn;
                int32_t stream_id;
                std::string url;
            }sess_key_t;

            using sess_key_ptr = std::shared_ptr<sess_key_t>;
            
            std::mutex m_sess_mu;
            std::map<sess_key_ptr,sess_ptr> m_sess;
    };

}}//namespace

#endif
