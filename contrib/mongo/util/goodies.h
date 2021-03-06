﻿// @file goodies.h
// miscellaneous

/*    Copyright 2009 10gen Inc.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#pragma once

#include "../bson/util/misc.h"
#include "concurrency/mutex.h"

namespace mongo
{

	/* @return a dump of the buffer as hex byte ascii output */
	string hexdump(const char* data, unsigned len);

	/**
	 * @return if this name has an increasing counter associated, return the value
	 *         otherwise 0
	 */
	unsigned setThreadName(const char* name);
	string getThreadName();

	template<class T>
	inline string ToString(const T& t)
	{
		stringstream s;
		s << t;
		return s.str();
	}

#if !defined(_WIN32) && !defined(NOEXECINFO) && !defined(__freebsd__) && !defined(__openbsd__) && !defined(__sun__)

} // namespace mongo

#include <pthread.h>
#include <execinfo.h>

namespace mongo
{

	inline pthread_t GetCurrentThreadId()
	{
		return pthread_self();
	}

	/* use "addr2line -CFe <exe>" to parse. */
	inline void printStackTrace(ostream& o = cout)
	{
		void* b[20];

		int size = backtrace(b, 20);
		for (int i = 0; i < size; i++)
		{
			o << hex << b[i] << dec << ' ';
		}
		o << endl;

		char** strings;

		strings = backtrace_symbols(b, size);
		for (int i = 0; i < size; i++)
		{
			o << ' ' << strings[i] << '\n';
		}
		o.flush();
		free(strings);
	}
#else
	inline void printStackTrace(ostream& o = cout) { }
#endif

	bool isPrime(int n);
	int nextPrime(int n);

	inline void dumpmemory(const char* data, int len)
	{
		if (len > 1024)
		{
			len = 1024;
		}
		try
		{
			const char* q = data;
			const char* p = q;
			while (len > 0)
			{
				for (int i = 0; i < 16; i++)
				{
					if (*p >= 32 && *p <= 126)
					{
						cout << *p;
					}
					else
					{
						cout << '.';
					}
					p++;
				}
				cout << "  ";
				p -= 16;
				for (int i = 0; i < 16; i++)
				{
					cout << (unsigned)((unsigned char)*p++) << ' ';
				}
				cout << endl;
				len -= 16;
			}
		}
		catch (...)
		{
		}
	}

	// PRINT(2+2);  prints "2+2: 4"
#define MONGO_PRINT(x) cout << #x ": " << (x) << endl
#define PRINT MONGO_PRINT
	// PRINTFL; prints file:line
#define MONGO_PRINTFL cout << __FILE__ ":" << __LINE__ << endl
#define PRINTFL MONGO_PRINTFL
#define MONGO_FLOG log() << __FILE__ ":" << __LINE__ << endl
#define FLOG MONGO_FLOG

#undef assert
#define assert MONGO_assert

	inline bool startsWith(const char* str, const char* prefix)
	{
		size_t l = strlen(prefix);
		if (strlen(str) < l)
		{
			return false;
		}
		return strncmp(str, prefix, l) == 0;
	}
	inline bool startsWith(string s, string p)
	{
		return startsWith(s.c_str(), p.c_str());
	}

	inline bool endsWith(const char* p, const char* suffix)
	{
		size_t a = strlen(p);
		size_t b = strlen(suffix);
		if (b > a)
		{
			return false;
		}
		return strcmp(p + a - b, suffix) == 0;
	}

	inline unsigned long swapEndian(unsigned long x)
	{
		return
			((x & 0xff) << 24) |
			((x & 0xff00) << 8) |
			((x & 0xff0000) >> 8) |
			((x & 0xff000000) >> 24);
	}

#if defined(BOOST_LITTLE_ENDIAN)
	inline unsigned long fixEndian(unsigned long x)
	{
		return x;
	}
#else
	inline unsigned long fixEndian(unsigned long x)
	{
		return swapEndian(x);
	}
#endif

#if !defined(_WIN32)
	typedef int HANDLE;
	inline void strcpy_s(char* dst, unsigned len, const char* src)
	{
		assert(strlen(src) < len);
		strcpy(dst, src);
	}
#else
	typedef void* HANDLE;
#endif

	/* thread local "value" rather than a pointer
	   good for things which have copy constructors (and the copy constructor is fast enough)
	   e.g.
	     ThreadLocalValue<int> myint;
	*/
	template<class T>
	class ThreadLocalValue
	{
	public:
		ThreadLocalValue(T def = 0) : _default(def) { }

		T get() const
		{
			T* val = _val.get();
			if (val)
			{
				return *val;
			}
			return _default;
		}

		void set(const T& i)
		{
			T* v = _val.get();
			if (v)
			{
				*v = i;
				return;
			}
			v = new T(i);
			_val.reset(v);
		}

	private:
		boost::thread_specific_ptr<T> _val;
		const T _default;
	};

	class ProgressMeter : boost::noncopyable
	{
	public:
		ProgressMeter(unsigned long long total , int secondsBetween = 3 , int checkInterval = 100)
		{
			reset(total , secondsBetween , checkInterval);
		}

		ProgressMeter()
		{
			_active = 0;
		}

		// typically you do ProgressMeterHolder
		void reset(unsigned long long total , int secondsBetween = 3 , int checkInterval = 100)
		{
			_total = total;
			_secondsBetween = secondsBetween;
			_checkInterval = checkInterval;

			_done = 0;
			_hits = 0;
			_lastTime = (int)time(0);

			_active = 1;
		}

		void finished()
		{
			_active = 0;
		}

		bool isActive()
		{
			return _active;
		}

		/**
		 * @param n how far along we are relative to the total # we set in CurOp::setMessage
		 * @return if row was printed
		 */
		bool hit(int n = 1)
		{
			if (! _active)
			{
				cout << "warning: hit on in-active ProgressMeter" << endl;
				return false;
			}

			_done += n;
			_hits++;
			if (_hits % _checkInterval)
			{
				return false;
			}

			int t = (int) time(0);
			if (t - _lastTime < _secondsBetween)
			{
				return false;
			}

			if (_total > 0)
			{
				int per = (int)(((double)_done * 100.0) / (double)_total);
				cout << "\t\t" << _done << "/" << _total << "\t" << per << "%" << endl;
			}
			_lastTime = t;
			return true;
		}

		void setTotalWhileRunning(unsigned long long total)
		{
			_total = total;
		}

		unsigned long long done() const
		{
			return _done;
		}

		unsigned long long hits() const
		{
			return _hits;
		}

		unsigned long long total() const
		{
			return _total;
		}

		string toString() const
		{
			if (! _active)
			{
				return "";
			}
			stringstream buf;
			buf << _done << "/" << _total << " " << (_done * 100) / _total << "%";
			return buf.str();
		}

		bool operator==(const ProgressMeter& other) const
		{
			return this == &other;
		}
	private:

		bool _active;

		unsigned long long _total;
		int _secondsBetween;
		int _checkInterval;

		unsigned long long _done;
		unsigned long long _hits;
		int _lastTime;
	};

	// e.g.:
	// CurOp * op = cc().curop();
	// ProgressMeterHolder pm( op->setMessage( "index: (1/3) external sort" , d->stats.nrecords , 10 ) );
	// loop { pm.hit(); }
	class ProgressMeterHolder : boost::noncopyable
	{
	public:
		ProgressMeterHolder(ProgressMeter& pm)
			: _pm(pm)
		{
		}

		~ProgressMeterHolder()
		{
			_pm.finished();
		}

		ProgressMeter* operator->()
		{
			return &_pm;
		}

		bool hit(int n = 1)
		{
			return _pm.hit(n);
		}

		void finished()
		{
			_pm.finished();
		}

		bool operator==(const ProgressMeter& other)
		{
			return _pm == other;
		}

	private:
		ProgressMeter& _pm;
	};

	class TicketHolder
	{
	public:
		TicketHolder(int num) : _mutex("TicketHolder")
		{
			_outof = num;
			_num = num;
		}

		bool tryAcquire()
		{
			scoped_lock lk(_mutex);
			return _tryAcquire();
		}

		void waitForTicket()
		{
			scoped_lock lk(_mutex);

			while (! _tryAcquire())
			{
				_newTicket.wait(lk.boost());
			}
		}

		void release()
		{
			{
				scoped_lock lk(_mutex);
				_num++;
			}
			_newTicket.notify_one();
		}

		void resize(int newSize)
		{
			{
				scoped_lock lk(_mutex);

				int used = _outof - _num;
				if (used > newSize)
				{
					cout << "ERROR: can't resize since we're using (" << used << ") more than newSize(" << newSize << ")" << endl;
					return;
				}

				_outof = newSize;
				_num = _outof - used;
			}

			// Potentially wasteful, but easier to see is correct
			_newTicket.notify_all();
		}

		int available() const
		{
			return _num;
		}

		int used() const
		{
			return _outof - _num;
		}

		int outof() const
		{
			return _outof;
		}

	private:

		bool _tryAcquire()
		{
			if (_num <= 0)
			{
				if (_num < 0)
				{
					cerr << "DISASTER! in TicketHolder" << endl;
				}
				return false;
			}
			_num--;
			return true;
		}

		int _outof;
		int _num;
		mongo::mutex _mutex;
		boost::condition_variable_any _newTicket;
	};

	class TicketHolderReleaser
	{
	public:
		TicketHolderReleaser(TicketHolder* holder)
		{
			_holder = holder;
		}

		~TicketHolderReleaser()
		{
			_holder->release();
		}
	private:
		TicketHolder* _holder;
	};


	/**
	 * this is a thread safe string
	 * you will never get a bad pointer, though data may be mungedd
	 */
	class ThreadSafeString
	{
	public:
		ThreadSafeString(size_t size = 256)
			: _size(size) , _buf(new char[size])
		{
			memset(_buf , 0 , _size);
		}

		ThreadSafeString(const ThreadSafeString& other)
			: _size(other._size) , _buf(new char[_size])
		{
			strncpy(_buf , other._buf , _size);
		}

		~ThreadSafeString()
		{
			delete[] _buf;
			_buf = 0;
		}

		string toString() const
		{
			string s = _buf;
			return s;
		}

		ThreadSafeString& operator=(const char* str)
		{
			size_t s = strlen(str);
			if (s >= _size - 2)
			{
				s = _size - 2;
			}
			strncpy(_buf , str , s);
			_buf[s] = 0;
			return *this;
		}

		bool operator==(const ThreadSafeString& other) const
		{
			return strcmp(_buf , other._buf) == 0;
		}

		bool operator==(const char* str) const
		{
			return strcmp(_buf , str) == 0;
		}

		bool operator!=(const char* str) const
		{
			return strcmp(_buf , str) != 0;
		}

		bool empty() const
		{
			return _buf == 0 || _buf[0] == 0;
		}

	private:
		size_t _size;
		char* _buf;
	};

	ostream& operator<<(ostream& s, const ThreadSafeString& o);

	/** A generic pointer type for function arguments.
	 *  It will convert from any pointer type except auto_ptr.
	 *  Semantics are the same as passing the pointer returned from get()
	 *  const ptr<T>  =>  T * const
	 *  ptr<const T>  =>  T const *  or  const T*
	 */
	template <typename T>
	struct ptr
	{

		ptr() : _p(NULL) {}

		// convert to ptr<T>
		ptr(T* p) : _p(p) {} // needed for NULL
		template<typename U> ptr(U* p) : _p(p) {}
		template<typename U> ptr(const ptr<U>& p) : _p(p) {}
		template<typename U> ptr(const boost::shared_ptr<U>& p) : _p(p.get()) {}
		template<typename U> ptr(const boost::scoped_ptr<U>& p) : _p(p.get()) {}
		//template<typename U> ptr(const auto_ptr<U>& p) : _p(p.get()) {}

		// assign to ptr<T>
		ptr& operator= (T* p)
		{
			_p = p;    // needed for NULL
			return *this;
		}
		template<typename U> ptr& operator= (U* p)
		{
			_p = p;
			return *this;
		}
		template<typename U> ptr& operator= (const ptr<U>& p)
		{
			_p = p;
			return *this;
		}
		template<typename U> ptr& operator= (const boost::shared_ptr<U>& p)
		{
			_p = p.get();
			return *this;
		}
		template<typename U> ptr& operator= (const boost::scoped_ptr<U>& p)
		{
			_p = p.get();
			return *this;
		}
		//template<typename U> ptr& operator= (const auto_ptr<U>& p) { _p = p.get(); return *this; }

		// use
		T* operator->() const
		{
			return _p;
		}
		T& operator*() const
		{
			return *_p;
		}

		// convert from ptr<T>
		operator T* () const
		{
			return _p;
		}

	private:
		T* _p;
	};



	/** Hmmmm */
	using namespace boost;

} // namespace mongo
