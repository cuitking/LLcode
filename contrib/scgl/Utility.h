#ifndef SCGL_UTILITY_H
#define SCGL_UTILITY_H

#include <sstream>

namespace scgl
{

	/**
	 * 安全删除指针，并且把它赋为NULL
	 */
	//template <typename T>
	//inline void SafeDelete(const T* & p)
#define SafeDelete(p)   \
	{                       \
		if (p != 0)         \
		{                   \
			::delete p;     \
			p = 0;          \
		}                   \
	}

	/**
	 * 安全删除指针数组，并且把它赋为NULL
	 */
	template <typename T>
	inline void SafeDeleteArray(const T*& p)
	{
		if (p == 0)
		{
			return;
		}

		::delete [] p;
		p = 0;
	}

	template <typename T>
	inline void SafeDeleteArray(T*& p)
	{
		if (p == 0)
		{
			return;
		}

		::delete [] p;
		p = 0;
	}

	/**
	 * 安全释放对象
	 */
	template <typename T>
	inline void SafeRelease(T*& p)
	{
		if (p == 0)
		{
			return;
		}
		p->Release();
		p = 0;
	}

	/**
	 * 使用编译器判断合法的隐式类型转换
	 */
	template <typename R, typename P>
	inline R implicit_cast(const P& p)
	{
		return p;
	}

	/**
	 * 比较任意两个实例的地址
	 */
	template <typename T>
	struct AddressLesser
	{
		bool operator()(const T& lhs, const T& rhs) const
		{
			return &lhs < &rhs;
		}
	};

	unsigned int Generate32BitGuid(void);

	const char* GenerateUuid(const char* prefix = 0);

	void ShowSystemErrorMessage();

}

#endif
