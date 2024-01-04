#ifndef rtmp_log_include_h
#define rtmp_log_include_h
#include "log/ceanic_log.h"
extern LOG_HANDLE g_rtmp_log;
#define RTMP_WRITE_LOG_ERROR(info,...)   WRITE_LOG_ERROR(g_rtmp_log,info,##__VA_ARGS__)
#define RTMP_WRITE_LOG_DEBUG(info,...)   WRITE_LOG_DEBUG(g_rtmp_log,info,##__VA_ARGS__)
#define RTMP_WRITE_LOG_INFO(info,...)    WRITE_LOG_INFO(g_rtmp_log,info,##__VA_ARGS__)
#define RTMP_WRITE_LOG_WARN(info,...)    WRITE_LOG_WARN(g_rtmp_log,info,##__VA_ARGS__)
#define RTMP_WRITE_LOG_FATAL(info,...)   WRITE_LOG_FATAL(g_rtmp_log,info,##__VA_ARGS__)




#endif

