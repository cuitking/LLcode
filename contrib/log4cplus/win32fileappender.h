﻿//   Copyright (C) 2009-2010, Vaclav Haisman. All rights reserved.
//
//   Redistribution and use in source and binary forms, with or without modifica-
//   tion, are permitted provided that the following conditions are met:
//
//   1. Redistributions of  source code must  retain the above copyright  notice,
//      this list of conditions and the following disclaimer.
//
//   2. Redistributions in binary form must reproduce the above copyright notice,
//      this list of conditions and the following disclaimer in the documentation
//      and/or other materials provided with the distribution.
//
//   THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED WARRANTIES,
//   INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
//   FITNESS  FOR A PARTICULAR  PURPOSE ARE  DISCLAIMED.  IN NO  EVENT SHALL  THE
//   APACHE SOFTWARE  FOUNDATION  OR ITS CONTRIBUTORS  BE LIABLE FOR  ANY DIRECT,
//   INDIRECT, INCIDENTAL, SPECIAL,  EXEMPLARY, OR CONSEQUENTIAL  DAMAGES (INCLU-
//   DING, BUT NOT LIMITED TO, PROCUREMENT  OF SUBSTITUTE GOODS OR SERVICES; LOSS
//   OF USE, DATA, OR  PROFITS; OR BUSINESS  INTERRUPTION)  HOWEVER CAUSED AND ON
//   ANY  THEORY OF LIABILITY,  WHETHER  IN CONTRACT,  STRICT LIABILITY,  OR TORT
//   (INCLUDING  NEGLIGENCE OR  OTHERWISE) ARISING IN  ANY WAY OUT OF THE  USE OF
//   THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef LOG4CPLUS_WIN32FILEAPPENDER_H
#define LOG4CPLUS_WIN32FILEAPPENDER_H


#include <log4cplus/config.hxx>
#if defined(_WIN32)

#include <log4cplus/appender.h>
#include <log4cplus/helpers/property.h>


namespace log4cplus
{

	/**
	* Prints events to Win32 console.
	*
	* <h3>Properties</h3>
	* <dl>
	* <dt><tt>AllocConsole</tt></dt>
	* <dd>This boolean property specifies whether or not this appender
	* will try to allocate new console using the
	* <code>AllocConsole()</code> Win32 function.</dd>
	*
	* </dl>
	*/
	class LOG4CPLUS_EXPORT Win32FileAppender
		: public Appender
	{
	public:
		explicit Win32FileAppender(const log4cplus::tstring& filename);
		Win32FileAppender(helpers::Properties const& properties);
		virtual ~Win32FileAppender();

		virtual void close();

	private:
		virtual void append(spi::InternalLoggingEvent const&);
		void* file;

	private:
		void Constructor(const log4cplus::tstring& filename);
		Win32FileAppender(Win32FileAppender const&);
		Win32FileAppender& operator = (Win32FileAppender const&);
	};

} // namespace log4cplus

#endif

#endif // LOG4CPLUS_WIN32CONSOLEAPPENDER_H
