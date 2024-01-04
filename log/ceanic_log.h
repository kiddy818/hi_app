#ifndef ceanic_log_include_h
#define ceanic_log_include_h

#include "ceanic_log_type.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif

typedef void* LOG_HANDLE;
LOG_HANDLE ceanic_start_log(const char* name,int mode,CEANIC_LOG_LEVEL_E level,const char* file_path,unsigned int max_size);

int ceanic_stop_log(LOG_HANDLE h);

int ceanic_write_log(LOG_HANDLE h,CEANIC_LOG_LEVEL_E level,const char* msg,...);

int ceanic_set_level(LOG_HANDLE h,CEANIC_LOG_LEVEL_E level);

#define _STR(x) _VAL(x)
#define _VAL(x) #x

#define MakePrefix(fmt) std::string(__FILE__).append("(").append(_STR(__LINE__)).append("):").append(__FUNCTION__).append("():").append("    ").append(fmt).c_str()

#define WRITE_LOG_INFO(h,msg,...)   ceanic_write_log(h,CEANIC_LOG_INFO,MakePrefix(msg), ##__VA_ARGS__);
#define WRITE_LOG_DEBUG(h,msg,...)  ceanic_write_log(h,CEANIC_LOG_DEBUG,MakePrefix(msg), ##__VA_ARGS__);
#define WRITE_LOG_ERROR(h,msg,...)  ceanic_write_log(h,CEANIC_LOG_ERROR,MakePrefix(msg), ##__VA_ARGS__);
#define WRITE_LOG_WARN(h,msg,...)   ceanic_write_log(h,CEANIC_LOG_WARN,MakePrefix(msg), ##__VA_ARGS__);
#define WRITE_LOG_FATAL(h,msg,...)  ceanic_write_log(h,CEANIC_LOG_FATAL,MakePrefix(msg), ##__VA_ARGS__);
					
#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif
