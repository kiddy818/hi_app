#ifndef ceanic_log_type_include_h
#define ceanic_log_type_include_h

typedef enum
{
    CEANIC_LOG_DEBUG = 0,
    CEANIC_LOG_INFO = 1,
    CEANIC_LOG_WARN = 2,		
    CEANIC_LOG_ERROR = 3,
    CEANIC_LOG_FATAL = 4,
}CEANIC_LOG_LEVEL_E;

typedef enum
{
    CEANIC_LOG_MODE_CONSOLE = (1 << 0),
    CEANIC_LOG_MODE_FILE = (1 << 1),
}CEANIC_LOG_MODE_E;

#endif
