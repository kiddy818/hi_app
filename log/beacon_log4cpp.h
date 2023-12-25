#ifndef beacon_log4cpp_include_h
#define beacon_log4cpp_include_h

#include <iostream>
#include <log4cpp/Category.hh>
#include <log4cpp/FileAppender.hh>
#include <log4cpp/RollingFileAppender.hh>
#include <log4cpp/OstreamAppender.hh>
#include <log4cpp/BasicLayout.hh>
#include <log4cpp/PatternLayout.hh>

#include <string>
#include "beacon_log_type.h"

class beacon_log4cpp
{
private:	
	explicit beacon_log4cpp(const char* name,int mode,const char* file_path,unsigned int max_file_len);

public:
	~beacon_log4cpp();

	static beacon_log4cpp* create_log(const char* name,int mode,const char* file_path,unsigned int max_file_len = 0);

	bool set_level(BEACON_LOG_LEVEL_E level);	

	bool debug(const char* msg);
	bool info(const char* msg);
	bool warn(const char* msg);	
	bool error(const char* msg);
	bool fatal(const char* msg);

private:	
	log4cpp::Category& m_category;
	log4cpp::Appender* m_appender;
	log4cpp::PatternLayout* m_layout;	
};


#endif
