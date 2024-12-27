#include "output_log.h"

#define MAX_BUF_LEN 1024

int g_log_debug_flag = 1;
int g_log_info_flag = 1;
int g_log_warnning_flag = 1;
int g_log_error_flag = 1;

void set_log_flag(int log_debug_flag, int log_info_flag, int log_warnning_flag,
	int log_error_flag)
{
	g_log_debug_flag = log_debug_flag;
	g_log_info_flag = log_info_flag;
	g_log_warnning_flag = log_warnning_flag;
	g_log_error_flag = log_error_flag;
}

void output_log(LOG_LEVEL log_level, const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);

	char buf[MAX_BUF_LEN] = { 0 };
	vsnprintf(buf, MAX_BUF_LEN - 1, fmt, args);

	switch (log_level)
	{
	case LOG_DEBUG:
		if (g_log_debug_flag)
			printf("[Log-Debug]:%s\n", buf);
		break;
	case LOG_INFO:
		if (g_log_info_flag)
			printf("[Log-Info]:%s\n", buf);
		break;
	case LOG_WARNING:
		if (g_log_warnning_flag)
			printf("[Log-Warnning]:%s\n", buf);
		break;
	case LOG_ERROR:
		if (g_log_error_flag)
			printf("[Log-Error]:%s\n", buf);
		break;
	default:
		break;
	}

	va_end(args);
}