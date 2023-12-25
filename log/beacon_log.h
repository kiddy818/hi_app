#ifndef beacon_log_include_h
#define beacon_log_include_h

#include "beacon_log_type.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif

typedef void* LOG_HANDLE;
LOG_HANDLE beacon_start_log(const char* name,int mode,BEACON_LOG_LEVEL_E level,const char* file_path,unsigned int max_size);

int beacon_stop_log(LOG_HANDLE h);

int beacon_write_log(LOG_HANDLE h,BEACON_LOG_LEVEL_E level,const char* msg,...);

int beacon_set_level(LOG_HANDLE h,BEACON_LOG_LEVEL_E level);

#define _STR(x) _VAL(x)
#define _VAL(x) #x

#define MakePrefix(fmt) std::string(__FILE__).append("(").append(_STR(__LINE__)).append("):").append(__FUNCTION__).append("():").append("    ").append(fmt).c_str()

#define WRITE_LOG_INFO(h,msg,...)   beacon_write_log(h,BEACON_LOG_INFO,MakePrefix(msg), ##__VA_ARGS__);
#define WRITE_LOG_DEBUG(h,msg,...)  beacon_write_log(h,BEACON_LOG_DEBUG,MakePrefix(msg), ##__VA_ARGS__);
#define WRITE_LOG_ERROR(h,msg,...)  beacon_write_log(h,BEACON_LOG_ERROR,MakePrefix(msg), ##__VA_ARGS__);
#define WRITE_LOG_WARN(h,msg,...)   beacon_write_log(h,BEACON_LOG_WARN,MakePrefix(msg), ##__VA_ARGS__);
#define WRITE_LOG_FATAL(h,msg,...)  beacon_write_log(h,BEACON_LOG_FATAL,MakePrefix(msg), ##__VA_ARGS__);
					
#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif
