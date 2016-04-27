#ifndef LOG_H
#define LOG_H

#ifndef DO_NOT_USE_LOG

#include <log4cplus/loggingmacros.h>

#ifdef _DEBUG
#pragma comment(lib, "log4cplusSD.lib")
#else
#pragma comment(lib, "log4cplusS.lib")
#endif

#define LOG_TRACE(logger, logEvent) LOG4CPLUS_TRACE(log4cplus::Logger::getInstance(logger), logEvent)
#define LOG_DEBUG(logger, logEvent)  LOG4CPLUS_DEBUG(log4cplus::Logger::getInstance(logger), logEvent)
#define LOG_INFO(logger, logEvent)  LOG4CPLUS_INFO(log4cplus::Logger::getInstance(logger), logEvent)
#define LOG_WARN(logger, logEvent)  LOG4CPLUS_WARN(log4cplus::Logger::getInstance(logger), logEvent)
#define LOG_ERROR(logger, logEvent)  LOG4CPLUS_ERROR(log4cplus::Logger::getInstance(logger), logEvent)

#define LOG_TRACE_STR(logger, logEvent) LOG4CPLUS_TRACE_STR(log4cplus::Logger::getInstance(logger), logEvent)
#define LOG_DEBUG_STR(logger, logEvent)  LOG4CPLUS_DEBUG_STR(log4cplus::Logger::getInstance(logger), logEvent)
#define LOG_INFO_STR(logger, logEvent)  LOG4CPLUS_INFO_STR(log4cplus::Logger::getInstance(logger), logEvent)
#define LOG_WARN_STR(logger, logEvent)  LOG4CPLUS_WARN_STR(log4cplus::Logger::getInstance(logger), logEvent)
#define LOG_ERROR_STR(logger, logEvent)  LOG4CPLUS_ERROR_STR(log4cplus::Logger::getInstance(logger), logEvent)

#define PrintVar(Var) " " << #Var << ":" << Var

#else

#define LOG_TRACE(logger, logEvent)	((void)0)
#define LOG_DEBUG(logger, logEvent)  ((void)0)
#define LOG_INFO(logger, logEvent)  ((void)0)
#define LOG_WARN(logger, logEvent)  ((void)0)
#define LOG_ERROR(logger, logEvent)  ((void)0)

#define LOG_TRACE_STR(logger, logEvent)	((void)0)
#define LOG_DEBUG_STR(logger, logEvent) ((void)0)
#define LOG_INFO_STR(logger, logEvent)  ((void)0)
#define LOG_WARN_STR(logger, logEvent)  ((void)0)
#define LOG_ERROR_STR(logger, logEvent) ((void)0)

#define PrintVar(Var) ((void)0)

#endif

extern "C" {
#define FFI_EXPORT __declspec(dllexport)
	bool FFI_EXPORT LogInitAppender(const char* logger, int mode, const char* serviceName);
	bool FFI_EXPORT LogSetLoggerLevel(const char* logger, int ll);
}

#endif
