#include "beacon_log4cpp.h"

beacon_log4cpp::beacon_log4cpp(const char* name,int mode,const char* file_path,unsigned int max_file_len)
:m_category(log4cpp::Category::getRoot().getInstance(std::string(name)))
{
	if(mode & BEACON_LOG_MODE_CONSOLE)
	{
		//m_appender = new log4cpp::OstreamAppender(name, &std::cout);
        log4cpp::Appender* console_appender = new log4cpp::OstreamAppender(name, &std::cout);
        log4cpp::PatternLayout* console_layout = new log4cpp::PatternLayout();
        console_layout->setConversionPattern("[%c]%d#%p:%m%n");
        console_appender->setLayout(console_layout);
        m_category.addAppender(console_appender);
    }

    if(mode & BEACON_LOG_MODE_FILE)
    {
		if(max_file_len == 0)
		{
			m_appender = new log4cpp::FileAppender(name,file_path);
		}
		else
		{
			m_appender = new log4cpp::RollingFileAppender(name,file_path,max_file_len);
		}

        m_layout = new log4cpp::PatternLayout();
        /*
           %m log message 内容, 即 用户写 log 的具体信息 
           %n 回车换行 
           %c category 名字 
           %d 时间戳 
           %p 优先级 
           %r 距离上一次写 log 的间隔, 单位毫秒 
           %R 距离上一次写 log 的间隔, 单位秒 
           %t 线程名 
           %u 处理器时间 
           %x NDC ? 
           */
        m_layout->setConversionPattern("[%c]%d#%p:%m%n");
        m_appender->setLayout(m_layout);

        m_category.setPriority(log4cpp::Priority::INFO);
        m_category.addAppender(m_appender);
    }
}

beacon_log4cpp::~beacon_log4cpp()
{

}

beacon_log4cpp* beacon_log4cpp::create_log(const char* name,int mode,const char* file_path,unsigned int max_file_len /* = 0 */)
{
	return new beacon_log4cpp(name,mode,file_path,max_file_len);
}

bool beacon_log4cpp::set_level(BEACON_LOG_LEVEL_E level)
{
	log4cpp::Priority::Value log4cpp_level;
	switch(level)
	{
	case BEACON_LOG_DEBUG:
		{
			log4cpp_level = log4cpp::Priority::DEBUG;
			break;
		}

	case BEACON_LOG_WARN:
		{
			log4cpp_level = log4cpp::Priority::WARN;
			break;
		}

	case BEACON_LOG_INFO:
		{
			log4cpp_level = log4cpp::Priority::INFO;
			break;
		}

	case BEACON_LOG_ERROR:
		{
			log4cpp_level = log4cpp::Priority::ERROR;
			break;
		}

	case BEACON_LOG_FATAL:
		{
			log4cpp_level = log4cpp::Priority::FATAL;
			break;
		}

	default:
		{
			return false;
		}
	}

	m_category.setPriority(log4cpp_level);

	return true;
}

bool beacon_log4cpp::info(const char* msg)
{
	m_category.info(msg);

	return true;
}

bool beacon_log4cpp::warn(const char* msg)
{
    m_category.warn(msg);

	return true;
}

bool beacon_log4cpp::debug(const char* msg)
{
    m_category.debug(msg);

	return true;
}

bool beacon_log4cpp::error(const char* msg)
{	
    m_category.error(msg);

	return true;
}

bool beacon_log4cpp::fatal(const char* msg)
{	
	m_category.fatal(msg);

	return true;
}
