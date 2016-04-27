#ifndef SCGL_SINGLETON_HOLDER
#define SCGL_SINGLETON_HOLDER

#include <boost/thread/mutex.hpp>

namespace scgl
{
	// 此模板在多个module里会生成多份实例，请确定是在唯一的module里才能使用
	template
	<
	typename T
	>
	class SingletonHolder
	{
	public:
		static T& Instance();
		static void Release();

	protected:
		SingletonHolder() {}
		~SingletonHolder() {}
	private:
		SingletonHolder(const SingletonHolder&);
		SingletonHolder& operator=(const SingletonHolder&);
		static T* instance;
		static boost::mutex m;
	};

	template<typename T>
	T* SingletonHolder<T>::instance = NULL;

	template<typename T>
	boost::mutex SingletonHolder<T>::m;

	template <typename T>
	T& SingletonHolder<T>::Instance()
	{
		if (instance == NULL)
		{
			boost::mutex::scoped_lock l(m);
			if (instance == NULL)
			{
				instance = new T;
				atexit(Release);
			}
		}
		return (*instance);
	}

	template <typename T>
	void SingletonHolder<T>::Release()
	{
		if (instance != NULL)
		{
			boost::mutex::scoped_lock l(m);
			if (instance != NULL)
			{
				delete instance;
				instance = NULL;
			}
		}
	}

} // namespace scgl

/// 以下两个单件宏不但可以保证线程安全的单件，而且可以保证跨MODULE（EXE或者DLL）都是唯一
// 此宏必须放在类声明的第一行或者最后一行
#define DECLARE_SINGLETON(T)							\
	public:												\
	static T& Instance();							\
	private:											\
	T(const T&);									\
	T& operator=(const T&);							\
	friend void Release_##T();

#define DEFINITION_SINGLETON(T)							\
	namespace											\
	{													\
		static T* instance = NULL;						\
		static boost::mutex m;							\
	}													\
	\
	void Release_##T()									\
	{													\
		if (instance != NULL)							\
		{												\
			boost::mutex::scoped_lock l(m);				\
			if (instance != NULL)						\
			{											\
				delete instance;						\
				instance = NULL;						\
			}											\
		}												\
	}													\
	\
	T& T::Instance()									\
	{													\
		if (instance == NULL)							\
		{												\
			boost::mutex::scoped_lock l(m);				\
			if (instance == NULL)						\
			{											\
				instance = new T;						\
				atexit(Release_##T);					\
			}											\
		}												\
		return (*instance);								\
	}


#endif // end file guardian

