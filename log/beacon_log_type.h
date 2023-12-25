#ifndef beacon_log_type_include_h
#define beacon_log_type_include_h

typedef enum
{
    BEACON_LOG_DEBUG = 0,
    BEACON_LOG_INFO = 1,
    BEACON_LOG_WARN = 2,		
    BEACON_LOG_ERROR = 3,
    BEACON_LOG_FATAL = 4,
}BEACON_LOG_LEVEL_E;

typedef enum
{
    BEACON_LOG_MODE_CONSOLE = (1 << 0),
    BEACON_LOG_MODE_FILE = (1 << 1),
}BEACON_LOG_MODE_E;

#endif
