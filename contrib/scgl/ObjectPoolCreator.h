#ifndef SCGL_CREATE_OBJECT_H
#define SCGL_CREATE_OBJECT_H

#pragma warning(push)
#pragma warning(disable:4511)
#pragma warning(disable:4512)
#pragma warning(disable:4345)
#pragma warning(disable:4702)
#pragma warning(disable:4127)
#include <boost/pool/object_pool.hpp>
#pragma  warning(pop)

#include <boost/function.hpp>
#include <map>

//#ifdef _DEBUG
//#include <vld/vld.h>
//#endif

namespace scgl
{
	//////////////////////////////////////////////////////////////////////////
	// 本文件实现一个通用的内存池对象创建和释放接口
	// 实现部分， 内部使用， 接口部分见末尾
	typedef boost::function<void (void*)> releaseFunction;

	template < typename T>
	class ObjectCreator
	{
	public:

		static T* CreateObject()
		{
#ifdef _DEBUG
			T* p = new T;
#else
			T* p = __pool.construct();
#endif

			if (p != NULL)
			{
				ObjectRelease::AddObject(p, ObjectCreator<T>::ReleaseObject);
			}

			return p;
		}

		static void ReleaseObject(void*& t)
		{
			if (t == NULL)
			{
				return;
			}

#ifdef _DEBUG
			delete static_cast<T*>(t);
#else
			__pool.destroy(static_cast<T*>(t));
#endif
		}


	private:
		static boost::object_pool<T> __pool;
	};

	template <typename T>
	boost::object_pool<T> ObjectCreator<T>::__pool;

	class ObjectRelease
	{
	public:
		static void AddObject(void* lObjectAddress, releaseFunction fReleaseFunction)
		{
			__map[lObjectAddress] = fReleaseFunction;
		}

		static void ReleaseObject(void* lObjectAddress)
		{
			std::map<void* , releaseFunction>::iterator itr = __map.find(lObjectAddress);
			if (itr != __map.end())
			{
				releaseFunction fReleaseFunction = itr->second;
				fReleaseFunction(lObjectAddress);
				__map.erase(itr);
			}
		}

	private:

		static std::map<void* , releaseFunction> __map;
	};

	//////////////////////////////////////////////////////////////////////////
	// 提供给外部调用的接口

	// 创建一个对象， 从内存池
	template<typename T>
	T* Create()
	{
		return ObjectCreator<T>::CreateObject();
	}

	// 回收一个对象到内存池
	template<typename T>
	void Release(T*& t)
	{
		ObjectRelease::ReleaseObject(t);
		t = NULL;
		return;
	}
}

#endif
