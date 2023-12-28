#ifndef beacon_timer_interval_include_h
#define beacon_timer_interval_include_h

#include "beacon_std.h"

class beacon_time_interval
{
public:
	beacon_time_interval();

	~beacon_time_interval();

	void set_beg();

	int64_t interval();
		
private:
	struct timespec m_spec_beg;
};

#endif



