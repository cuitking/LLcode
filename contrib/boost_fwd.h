#ifndef BOOST_DECLARATION_H
#define BOOST_DECLARATION_H

namespace boost
{
	class any;
	class thread;
	class thread_group;

	namespace posix_time
	{
		class ptime;
	}

	namespace chrono
	{
		class steady_clock;
		typedef steady_clock high_resolution_clock;
	}

	namespace asio
	{
		class io_service;

		template <typename Time>
		struct time_traits;

		template < typename TimeType,
			typename TimeTraits >
		class deadline_timer_service;

		template < typename Time,
			typename TimeTraits,
			typename TimerService >
		class basic_deadline_timer;

		typedef basic_deadline_timer<boost::posix_time::ptime
			, boost::asio::time_traits<boost::posix_time::ptime>
			, deadline_timer_service<boost::posix_time::ptime, boost::asio::time_traits<boost::posix_time::ptime> > > deadline_timer;

		template <typename Clock,
			typename WaitTraits,
			typename WaitableTimerService>
		class basic_waitable_timer;

		template <typename Clock>
		struct wait_traits;

		template <typename Clock,
			typename WaitTraits>
		class waitable_timer_service;

		typedef basic_waitable_timer<boost::chrono::high_resolution_clock
			, boost::asio::wait_traits<boost::chrono::high_resolution_clock>
			, waitable_timer_service<boost::chrono::high_resolution_clock, boost::asio::wait_traits<boost::chrono::high_resolution_clock> > >
			high_resolution_timer;

		template <typename Protocol>
		class stream_socket_service;

		template < typename Protocol,
			typename StreamSocketService>
		class basic_stream_socket;

		namespace ip
		{
			class tcp;
		}
	}

	namespace interprocess
	{
		template<class VoidPointer>
		class message_queue_t;
	}

	namespace statechart
	{
		class event_base;
	}

	namespace system
	{
		class error_code;
	}

	template<class T> class shared_ptr;

	template<typename Signature> class function;

}


#endif
