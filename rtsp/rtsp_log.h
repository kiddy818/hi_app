#ifndef rtsp_log_include_h
#define rtsp_log_include_h
#include "log/beacon_log.h"
extern LOG_HANDLE g_rtsp_log;
#define RTSP_WRITE_LOG_ERROR(info,...)   WRITE_LOG_ERROR(g_rtsp_log,info,##__VA_ARGS__)
#define RTSP_WRITE_LOG_DEBUG(info,...)   WRITE_LOG_DEBUG(g_rtsp_log,info,##__VA_ARGS__)
#define RTSP_WRITE_LOG_INFO(info,...)    WRITE_LOG_INFO(g_rtsp_log,info,##__VA_ARGS__)
#define RTSP_WRITE_LOG_WARN(info,...)    WRITE_LOG_WARN(g_rtsp_log,info,##__VA_ARGS__)
#define RTSP_WRITE_LOG_FATAL(info,...)   WRITE_LOG_FATAL(g_rtsp_log,info,##__VA_ARGS__)




#endif

