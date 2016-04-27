#include "Log.h"
#include "Win32.h"
#include <log4cplus/nullappender.h>
#include <log4cplus/win32debugappender.h>
#include <log4cplus/asyncappender.h>
#include <log4cplus/win32fileappender.h>
#include <log4cplus/win32consoleappender.h>
#include <log4cplus/layout.h>
#include <log4cplus/helpers/property.h>
#include <boost/filesystem.hpp>
#include <iostream>

#define FFI_EXPORT __declspec(dllexport)

extern "C" {

#ifndef DO_NOT_USE_LOG

int rootMode = 0;
bool InitLogger(const char* loggerName, log4cplus::Logger logger, int mode, const char* serviceName);
bool CheckLoggerExist(const char* logger);

#ifdef _DEBUG
#pragma comment(lib, "log4cplusSD")
#else
#pragma comment(lib, "log4cplusS")
#endif

	bool FFI_EXPORT LogTrace(const char* logger, const char* logEvent)
	{
		if (CheckLoggerExist(logger))
		{
			LOG_TRACE(logger, logEvent);
			return true;
		}
		return false;
	}

	bool FFI_EXPORT LogDebug(const char* logger, const char* logEvent)
	{
		if (CheckLoggerExist(logger))
		{
			LOG_DEBUG(logger, logEvent);
			return true;
		}
		return false;
	}

	bool FFI_EXPORT LogInfo(const char* logger, const char* logEvent)
	{
		if (CheckLoggerExist(logger))
		{
			LOG_INFO(logger, logEvent);
			return true;
		}
		return false;
	}

	bool FFI_EXPORT LogWarn(const char* logger, const char* logEvent)
	{
		if (CheckLoggerExist(logger))
		{
			LOG_WARN(logger, logEvent);
			return true;
		}
		return false;
	}

	bool FFI_EXPORT LogError(const char* logger, const char* logEvent)
	{
		if (CheckLoggerExist(logger))
		{
			LOG_ERROR(logger, logEvent);
			return true;
		}
		return false;
	}

	bool FFI_EXPORT LogSetRootLevelAndMode(int ll, int mode)
	{
		log4cplus::Logger::getRoot().setLogLevel(ll);
		rootMode = mode;
		return true;
	}

	bool FFI_EXPORT LogSetLoggerLevel(const char* logger, int ll)
	{
		log4cplus::Logger::getInstance(logger).setLogLevel(ll);
		return true;
	}

	bool FFI_EXPORT LogInitAppender(const char* logger, int mode, const char* serviceName)
	{
		return InitLogger(logger, log4cplus::Logger::getInstance(logger), mode, serviceName);
	}

	bool FFI_EXPORT LogIsEnable(const char* logger, log4cplus::LogLevel ll)
	{
		return log4cplus::Logger::getInstance(logger).isEnabledFor(ll);
	}

	bool InitLogger(const char* loggerName, log4cplus::Logger logger, int mode, const char* serviceName)
	{
		logger.removeAllAppenders();

#ifndef _DEBUG
		log4cplus::AsyncAppender* asyncAppender =  NULL;
#endif // _DEBUG

		if (mode & 1)
		{
			// 控制台没创建就不创建
			if (::GetConsoleWindow() != NULL)
			{
				//console
				log4cplus::SharedAppenderPtr _append;
				_append = new log4cplus::Win32ConsoleAppender();
				std::string pattern = "[%-5p] [%D{%H:%M:%S.%q}] %m%n";
				std::auto_ptr<log4cplus::Layout> _layout(new log4cplus::PatternLayout(pattern));
				_append->setLayout(_layout);

#ifdef _DEBUG
				logger.addAppender(_append);
#else
				if (asyncAppender == NULL)
				{
					asyncAppender = new log4cplus::AsyncAppender();
					logger.addAppender(log4cplus::SharedAppenderPtr(asyncAppender));
				}

				asyncAppender->addAppender(_append);
#endif // _DEBUG
			}
		}
		if (mode & 2)
		{
			//debugview
			log4cplus::SharedAppenderPtr _append;
			_append = new log4cplus::Win32DebugAppender();
			std::string pattern = "[%-11c-%5p] %m%n";
			std::auto_ptr<log4cplus::Layout> _layout(new log4cplus::PatternLayout(pattern));
			_append->setLayout(_layout);

#ifdef _DEBUG
			logger.addAppender(_append);
#else
			if (asyncAppender == NULL)
			{
				asyncAppender = new log4cplus::AsyncAppender();
				logger.addAppender(log4cplus::SharedAppenderPtr(asyncAppender));
			}

			asyncAppender->addAppender(_append);
#endif // _DEBUG

		}

		if (mode & 4)
		{
			namespace fs = boost::filesystem;
			const char* path = "./log/";
			if (!fs::exists(path))
			{
				if (!fs::create_directory(path))
				{
					std::cerr << "Can't create log directory, no file log.\n";
					return false;
				}
			}
			char fileName[MAX_PATH];
			sprintf(fileName, "%s%s.%u.%s.log", path, serviceName != NULL ? serviceName : "", GetCurrentProcessId(), loggerName);

			log4cplus::SharedAppenderPtr _append(new log4cplus::Win32FileAppender(fileName)) ;
			std::string pattern = "[%-5p] [%D{%d %m %Y %H:%M:%S.%q}] %m%n";
			std::auto_ptr<log4cplus::Layout> _layout(new log4cplus::PatternLayout(pattern));
			_append->setLayout(_layout);

#ifdef _DEBUG
			logger.addAppender(_append);
#else
			if (asyncAppender == NULL)
			{
				asyncAppender = new log4cplus::AsyncAppender();
				logger.addAppender(log4cplus::SharedAppenderPtr(asyncAppender));
			}

			asyncAppender->addAppender(_append);
#endif // _DEBUG
		}

		// 没有任何appender加入
		if (logger.getAllAppenders().empty())
		{
			logger.setLogLevel(log4cplus::OFF_LOG_LEVEL);
		}

		return true;
	}

	bool CheckLoggerExist(const char* logger)
	{
		if (log4cplus::Logger::getInstance(logger).getAllAppenders().empty())
			return false;
		return true;
	}

#else

	bool FFI_EXPORT LogTrace(const char* , const char* )
	{
		return true;
	}

	bool FFI_EXPORT LogDebug(const char* , const char* )
	{
		return true;
	}

	bool FFI_EXPORT LogInfo(const char* , const char* )
	{
		return true;
	}

	bool FFI_EXPORT LogWarn(const char* , const char* )
	{
		return true;
	}

	bool FFI_EXPORT LogError(const char* , const char* )
	{
		return true;
	}

	bool FFI_EXPORT LogSetRootLevelAndMode(int , int )
	{
		return true;
	}

	bool FFI_EXPORT LogSetLoggerLevel(const char* , int )
	{
		return true;
	}

	bool FFI_EXPORT LogInitAppender(const char* , int , const char* )
	{
		return true;
	}

	bool FFI_EXPORT LogIsEnable(const char* , int )
	{
		return false;
	}

#endif
}
