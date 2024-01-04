#include "log/ceanic_log.h"
extern LOG_HANDLE g_dev_log;
#define DEV_WRITE_LOG_ERROR(info,...)   WRITE_LOG_ERROR(g_dev_log,info,##__VA_ARGS__)
#define DEV_WRITE_LOG_DEBUG(info,...)   WRITE_LOG_DEBUG(g_dev_log,info,##__VA_ARGS__)
#define DEV_WRITE_LOG_INFO(info,...)    WRITE_LOG_INFO(g_dev_log,info,##__VA_ARGS__)
#define DEV_WRITE_LOG_WARN(info,...)    WRITE_LOG_WARN(g_dev_log,info,##__VA_ARGS__)
#define DEV_WRITE_LOG_FATAL(info,...)   WRITE_LOG_FATAL(g_dev_log,info,##__VA_ARGS__)



