#include "beacon_time_interval.h"

beacon_time_interval::beacon_time_interval()
{
}

beacon_time_interval::~beacon_time_interval()
{
}

void beacon_time_interval::set_beg()
{
	clock_gettime(CLOCK_MONOTONIC,&m_spec_beg);
}

int64_t beacon_time_interval::interval()
{
	struct timespec m_spec_now;
	clock_gettime(CLOCK_MONOTONIC,&m_spec_now);
	int64_t interval = m_spec_now .tv_sec * 1000 + m_spec_now.tv_nsec / 1000000 - (m_spec_beg.tv_sec * 1000 + m_spec_beg.tv_nsec / 1000000);	

	return interval;
}

