//  Copyright (C) 2009-2010, Vaclav Haisman. All rights reserved.
//
//  Redistribution and use in source and binary forms, with or without modifica-
//  tion, are permitted provided that the following conditions are met:
//
//  1. Redistributions of  source code must  retain the above copyright  notice,
//     this list of conditions and the following disclaimer.
//
//  2. Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//
//  THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED WARRANTIES,
//  INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
//  FITNESS  FOR A PARTICULAR  PURPOSE ARE  DISCLAIMED.  IN NO  EVENT SHALL  THE
//  APACHE SOFTWARE  FOUNDATION  OR ITS CONTRIBUTORS  BE LIABLE FOR  ANY DIRECT,
//  INDIRECT, INCIDENTAL, SPECIAL,  EXEMPLARY, OR CONSEQUENTIAL  DAMAGES (INCLU-
//  DING, BUT NOT LIMITED TO, PROCUREMENT  OF SUBSTITUTE GOODS OR SERVICES; LOSS
//  OF USE, DATA, OR  PROFITS; OR BUSINESS  INTERRUPTION)  HOWEVER CAUSED AND ON
//  ANY  THEORY OF LIABILITY,  WHETHER  IN CONTRACT,  STRICT LIABILITY,  OR TORT
//  (INCLUDING  NEGLIGENCE OR  OTHERWISE) ARISING IN  ANY WAY OUT OF THE  USE OF
//  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


#include <log4cplus/asyncappender.h>
#include <log4cplus/config.hxx>
#include <tbb/concurrent_queue.h>
#include <tbb/atomic.h>
#include <boost/thread.hpp>

using namespace log4cplus;

namespace log4cplus
{
	namespace
	{

		struct EventAppenderPair
		{
			spi::InternalLoggingEvent	event;
			helpers::AppenderAttachableImpl*					appender;

			EventAppenderPair() {}
			EventAppenderPair(const spi::InternalLoggingEvent& _event, helpers::AppenderAttachableImpl* _appender)
				: event(_event)
				, appender(_appender)
			{

			}
		};

		struct QueueThread
		{
			QueueThread();
			~QueueThread();

			static int Run(QueueThread* qt);

			tbb::atomic<bool> quit;
			tbb::concurrent_bounded_queue<EventAppenderPair> queue;
			boost::thread t;
		};

#pragma warning( push )
#pragma warning( disable : 4355 )
		QueueThread::QueueThread()
			: t(&QueueThread::Run, this)
		{
		}

#pragma warning( pop )

		QueueThread::~QueueThread()
		{
			quit = true;
			t.join();
			EventAppenderPair ea;
			while (!queue.empty())
			{
				queue.pop(ea);
				ea.appender->appendLoopOnAppenders(ea.event);
			}
			printf("The log thread quit and all log are write.\n");
		}

		int QueueThread::Run(QueueThread* qt)
		{
			EventAppenderPair ea;

			while (qt->quit == false)
			{
				qt->queue.pop(ea);
				ea.appender->appendLoopOnAppenders(ea.event);
			}
			return 0;
		}

	} // namespace

	QueueThread* qt = NULL;

	bool InitThread()
	{
		static QueueThread t;
		qt = &t;
		return true;
	}


	AsyncAppender::AsyncAppender()
	{
		static bool init = InitThread();
	}

	AsyncAppender::~AsyncAppender()
	{
		destructorImpl();
	}

	void
	AsyncAppender::doAppend(spi::InternalLoggingEvent const& ev)
	{
		append(ev);
	}

	void
	AsyncAppender::append(spi::InternalLoggingEvent const& ev)
	{
		if (qt != NULL)
		{
			qt->queue.push(EventAppenderPair(ev, this));
		}
		else
		{
			appendLoopOnAppenders(ev);
		}
	}

} // namespace log4cplus
