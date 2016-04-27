#ifndef SCGL_STATIC_TEMP_POOL_H
#define SCGL_STATIC_TEMP_POOL_H

#include <queue>
#include <set>

namespace scgl
{

	template <typename T>
	class CStaticTempPool
	{
	public:
		CStaticTempPool();
		~CStaticTempPool(void);
		void Init(size_t size);
		T* Create(void);
		void Release(T*& object);
	private:
		typedef std::queue<T*>  PointerQueue;
		typedef std::set<T*>    PointerSet;

	private:
		T*           m_pInitPointer;
		PointerQueue m_qFreeObjects;
		PointerSet   m_sTempObjects;
	};

	template <typename T>
	CStaticTempPool<T>::CStaticTempPool()
		: m_pInitPointer(0)
	{}

	template <typename T>
	CStaticTempPool<T>::~CStaticTempPool(void)
	{
		for (PointerSet::iterator it = m_sTempObjects.begin(); it != m_sTempObjects.end(); ++it)
		{
			delete *it;
		}

		if (m_pInitPointer)
		{
			delete [] m_pInitPointer;
		}
	}

	template <typename T>
	void CStaticTempPool<T>::Init(size_t size)
	{
		m_pInitPointer = new T[size];
		if (m_pInitPointer == NULL)
		{
			return;
		}

		for (size_t i = 0; i < size; ++i)
		{
			m_qFreeObjects.push(&m_pInitPointer[i]);
		}
	}

	template <typename T>
	typename T* CStaticTempPool<T>::Create(void)
	{
		T* pT = NULL;
		if (!m_qFreeObjects.empty())
		{
			pT = m_qFreeObjects.front();
			m_qFreeObjects.pop();
		}
		else
		{
			pT = new T;
			m_sTempObjects.insert(pT);
		}
		return pT;

	}

	template <typename T>
	void CStaticTempPool<T>::Release(T*& object)
	{
		PointerSet::iterator found = m_sTempObjects.find(object);
		if (found != m_sTempObjects.end())
		{
			delete object;
			m_sTempObjects.erase(found);
		}
		else
		{
			m_qFreeObjects.push(object);
		}
		object = NULL;
	}

}

#endif
