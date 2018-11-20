#ifndef __LOG_HPP__
#define __LOG_HPP__

#include <iostream>
#include <string>
#include <sys/time.h>

#define INFO	0
#define DEBUG	1
#define WARNING	2
#define ERROR	3

uint64_t GetTimeStamp()
{
	struct timeval tv_;
	gettimeofday(&tv_, NULL);
	return tv_.tv_sec;
}

std::string GetLogLevel(int level_)
{
	switch (level_)
	{
		case 0:
			return "INFO";
		case 1:
			return "DEBUG";
		case 2:
			return "WARNING";
		case 3:
			return "ERROR";
		default:
			return "UNKNOWN ERROR";
	}
}

void Log(int level_, std::string message_, std::string file_, int line_)
{
	uint64_t time = GetTimeStamp();
	std::cout << file_ << ":" << line_ << ": " << GetLogLevel(level_) << ": " << message_ << std::endl;
}

#define LOG(level_, message_) Log(level_, message_, __FILE__, __LINE__)

#endif
