#pragma once

#include <loki/Pimpl.h>
#include <log4cplus/configurator.h>
#include <boost/noncopyable.hpp>

namespace scgl
{

	class LOG4CPLUS_EXPORT CLoggerConfigurator : public log4cplus::PropertyConfigurator, public boost::noncopyable
	{
	public:
		CLoggerConfigurator(log4cplus::Hierarchy& h = log4cplus::Logger::getDefaultHierarchy());
		virtual ~CLoggerConfigurator();
		virtual void configure();

	private:
		typedef log4cplus::PropertyConfigurator Super;

	private:
		Loki::PimplOf<CLoggerConfigurator>::Type m_impl;
	};

}