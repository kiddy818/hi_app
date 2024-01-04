#ifndef ceanic_util_include_h
#define ceanic_util_include_h

#include "log/ceanic_log.h"
extern LOG_HANDLE g_app_log;
#define APP_WRITE_LOG_ERROR(info,...)   WRITE_LOG_ERROR(g_app_log,info,##__VA_ARGS__)
#define APP_WRITE_LOG_DEBUG(info,...)   WRITE_LOG_DEBUG(g_app_log,info,##__VA_ARGS__)
#define APP_WRITE_LOG_INFO(info,...)    WRITE_LOG_INFO(g_app_log,info,##__VA_ARGS__)
#define APP_WRITE_LOG_WARN(info,...)    WRITE_LOG_WARN(g_app_log,info,##__VA_ARGS__)
#define APP_WRITE_LOG_FATAL(info,...)   WRITE_LOG_FATAL(g_app_log,info,##__VA_ARGS__)

#endif

