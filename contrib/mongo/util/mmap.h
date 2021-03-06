﻿// mmap.h

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
#include <boost/thread/xtime.hpp>
#include "concurrency/rwlock.h"

namespace mongo
{

	class MAdvise
	{
		void* _p;
		unsigned _len;
	public:
		enum Advice { Sequential = 1 };
		MAdvise(void* p, unsigned len, Advice a);
		~MAdvise(); // destructor resets the range to MADV_NORMAL
	};

	/* the administrative-ish stuff here */
	class MongoFile : boost::noncopyable
	{
	public:
		/** Flushable has to fail nicely if the underlying object gets killed */
		class Flushable
		{
		public:
			virtual ~Flushable() {}
			virtual void flush() = 0;
		};

		virtual ~MongoFile() {}

		enum Options
		{
			SEQUENTIAL = 1, // hint - e.g. FILE_FLAG_SEQUENTIAL_SCAN on windows
			READONLY = 2    // not contractually guaranteed, but if specified the impl has option to fault writes
		};

		/** @param fun is called for each MongoFile.
		    calledl from within a mutex that MongoFile uses. so be careful not to deadlock.
		*/
		template < class F >
		static void forEach(F fun);

		/** note: you need to be in mmmutex when using this. forEach (above) handles that for you automatically.
		*/
		static set<MongoFile*>& getAllFiles()
		{
			return mmfiles;
		}

		// callbacks if you need them
		static void (*notifyPreFlush)();
		static void (*notifyPostFlush)();

		static int flushAll(bool sync);   // returns n flushed
		static long long totalMappedLength();
		static void closeAllFiles(stringstream& message);

#if defined(_DEBUG)
		static void markAllWritable();
		static void unmarkAllWritable();
#else
		static void markAllWritable() { }
		static void unmarkAllWritable() { }
#endif

		static bool exists(boost::filesystem::path p)
		{
			return boost::filesystem::exists(p);
		}

		virtual bool isMongoMMF()
		{
			return false;
		}

		string filename() const
		{
			return _filename;
		}
		void setFilename(string fn);

	private:
		string _filename;
		static int _flushAll(bool sync);   // returns n flushed
	protected:
		virtual void close() = 0;
		virtual void flush(bool sync) = 0;
		/**
		 * returns a thread safe object that you can call flush on
		 * Flushable has to fail nicely if the underlying object gets killed
		 */
		virtual Flushable* prepareFlush() = 0;

		void created(); /* subclass must call after create */

		/* subclass must call in destructor (or at close).
		   removes this from pathToFile and other maps
		   safe to call more than once, albeit might be wasted work
		   ideal to call close to the close, if the close is well before object destruction
		*/
		void destroyed();

		virtual unsigned long long length() const = 0;

		// only supporting on posix mmap
		virtual void _lock() {}
		virtual void _unlock() {}

		static set<MongoFile*> mmfiles;
	public:
		static map<string, MongoFile*> pathToFile;

		// lock order: lock dbMutex before this if you lock both
		static RWLockRecursive mmmutex;
	};

	/** look up a MMF by filename. scoped mutex locking convention.
	    example:
	      MMFFinderByName finder;
	      MongoMMF *a = finder.find("file_name_a");
	      MongoMMF *b = finder.find("file_name_b");
	*/
	class MongoFileFinder : boost::noncopyable
	{
	public:
		MongoFileFinder() : _lk(MongoFile::mmmutex) { }

		/** @return The MongoFile object associated with the specified file name.  If no file is open
		            with the specified name, returns null.
		*/
		MongoFile* findByPath(string path)
		{
			map<string, MongoFile*>::iterator i = MongoFile::pathToFile.find(path);
			return  i == MongoFile::pathToFile.end() ? NULL : i->second;
		}

	private:
		RWLockRecursive::Shared _lk;
	};

	struct MongoFileAllowWrites
	{
		MongoFileAllowWrites()
		{
			MongoFile::markAllWritable();
		}
		~MongoFileAllowWrites()
		{
			MongoFile::unmarkAllWritable();
		}
	};

	class MemoryMappedFile : public MongoFile
	{
	protected:
		virtual void* viewForFlushing()
		{
			if (views.size() == 0)
			{
				return 0;
			}
			assert(views.size() == 1);
			return views[0];
		}
	public:
		MemoryMappedFile();

		virtual ~MemoryMappedFile()
		{
			RWLockRecursive::Exclusive lk(mmmutex);
			close();
		}

		virtual void close();

		// Throws exception if file doesn't exist. (dm may2010: not sure if this is always true?)
		void* map(const char* filename);

		/** @param options see MongoFile::Options */
		void* mapWithOptions(const char* filename, int options);

		/* Creates with length if DNE, otherwise uses existing file length,
		   passed length.
		   @param options MongoFile::Options bits
		*/
		void* map(const char* filename, unsigned long long& length, int options = 0);

		/* Create. Must not exist.
		   @param zero fill file with zeros when true
		*/
		void* create(string filename, unsigned long long len, bool zero);

		void flush(bool sync);
		virtual Flushable* prepareFlush();

		long shortLength() const
		{
			return (long) len;
		}
		unsigned long long length() const
		{
			return len;
		}

		/** create a new view with the specified properties.
		    automatically cleaned up upon close/destruction of the MemoryMappedFile object.
		    */
		void* createReadOnlyMap();
		void* createPrivateMap();

		/** make the private map range writable (necessary for our windows implementation) */
		static void makeWritable(void*, unsigned len)
#if defined(_WIN32)
		;
#else
		{ }
#endif

	private:
		static void updateLength(const char* filename, unsigned long long& length);

		HANDLE fd;
		HANDLE maphandle;
		vector<void*> views;
		unsigned long long len;

#ifdef _WIN32
		boost::shared_ptr<mutex> _flushMutex;
		void clearWritableBits(void* privateView);
	public:
		static const unsigned ChunkSize = 64 * 1024 * 1024;
		static const unsigned NChunks = 1024 * 1024;
#else
		void clearWritableBits(void* privateView) { }
#endif

	protected:
		// only posix mmap implementations will support this
		virtual void _lock();
		virtual void _unlock();

		/** close the current private view and open a new replacement */
		void* remapPrivateView(void* oldPrivateAddr);
	};

	typedef MemoryMappedFile MMF;

	/** p is called from within a mutex that MongoFile uses.  so be careful not to deadlock. */
	template < class F >
	inline void MongoFile::forEach(F p)
	{
		RWLockRecursive::Shared lklk(mmmutex);
		for (set<MongoFile*>::iterator i = mmfiles.begin(); i != mmfiles.end(); i++)
		{
			p(*i);
		}
	}

#if defined(_WIN32)
	class ourbitset
	{
		volatile unsigned bits[MemoryMappedFile::NChunks]; // volatile as we are doing double check locking
	public:
		ourbitset()
		{
			memset((void*) bits, 0, sizeof(bits));
		}
		bool get(unsigned i) const
		{
			unsigned x = i / 32;
			assert(x < MemoryMappedFile::NChunks);
			return (bits[x] & (1 << (i % 32))) != 0;
		}
		void set(unsigned i)
		{
			unsigned x = i / 32;
			wassert(x < (MemoryMappedFile::NChunks * 2 / 3)); // warn if getting close to limit
			assert(x < MemoryMappedFile::NChunks);
			bits[x] |= (1 << (i % 32));
		}
		void clear(unsigned i)
		{
			unsigned x = i / 32;
			assert(x < MemoryMappedFile::NChunks);
			bits[x] &= ~(1 << (i % 32));
		}
	};
	extern ourbitset writable;
	void makeChunkWritable(size_t chunkno);
	inline void MemoryMappedFile::makeWritable(void* _p, unsigned len)
	{
		size_t p = (size_t) _p;
		unsigned a = p / ChunkSize;
		unsigned b = (p + len) / ChunkSize;
		for (unsigned i = a; i <= b; i++)
		{
			if (!writable.get(i))
			{
				makeChunkWritable(i);
			}
		}
	}
	extern void* getNextMemoryMappedFileLocation(unsigned long long mmfSize);
#endif

} // namespace mongo
