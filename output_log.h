#pragma once
#include <stdio.h>
#include <stdarg.h>
enum LOG_LEVEL
{
	LOG_DEBUG,
	LOG_INFO,
	LOG_WARNING,
	LOG_ERROR
};
void set_log_flag(int log_debug_flag, int log_info_flag,
	int log_warnning_flag, int log_error_flag);

void output_log(LOG_LEVEL log_level, const char* fmt, ...);