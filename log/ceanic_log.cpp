#include <stdarg.h>
#include "ceanic_log.h"
#include "ceanic_log4cpp.h"

extern "C"
{
    LOG_HANDLE ceanic_start_log(const char* name,int mode,CEANIC_LOG_LEVEL_E level,const char* file_path,unsigned int max_size)
    {
        ceanic_log4cpp* log = ceanic_log4cpp::create_log(name,mode,file_path,max_size);

        if(log == NULL)
        {
            return NULL;
        }

        log->set_level(level);
        return (LOG_HANDLE)log;
    }

    int ceanic_stop_log(LOG_HANDLE h)
    {
        ceanic_log4cpp* log = (ceanic_log4cpp*)h;
        if(log)
        {
            delete log;
            log = NULL;
        }

        return 0;
    }

    int ceanic_write_log(LOG_HANDLE h,CEANIC_LOG_LEVEL_E level,const char* msg,...)
    {
        ceanic_log4cpp* log = (ceanic_log4cpp*)h;

        if(!log)
        {
           return -1;             
        }

        char log_str[4096];

        va_list arg_ptr;
        va_start(arg_ptr, msg);
        vsprintf(log_str, msg, arg_ptr);
        va_end(arg_ptr);

        switch(level)
        {
            case CEANIC_LOG_DEBUG:
                log->debug(log_str);
                break;
            case CEANIC_LOG_INFO:
                log->info(log_str);
                break;
            case CEANIC_LOG_WARN:
                log->warn(log_str);
                break;
            case CEANIC_LOG_ERROR:
                log->error(log_str);
                break;
            case CEANIC_LOG_FATAL:
                log->fatal(log_str);
                break;
            default:
                return -1;
        }

        return 0;
    }

    int ceanic_set_level(LOG_HANDLE h,CEANIC_LOG_LEVEL_E level)
    {
        ceanic_log4cpp* log = (ceanic_log4cpp*)h;

        if(!log)
        {
            return -1;             
        }

        log->set_level(level);
        return 0;
    }
}

