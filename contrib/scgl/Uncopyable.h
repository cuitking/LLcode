#ifndef SCGL_UNCOPYABLE_H
#define SCGL_UNCOPYABLE_H

namespace scgl
{

#ifdef _WIN32
#pragma warning(disable : 4511)
#pragma warning(disable : 4512)
#endif
	/**
	 * 不能拷贝的类。不允许复制的类可以从此类继承。
	 */
	class IUncopyable
	{
	public:
		/**
		 * 因为声明了拷贝构造函数会取消编译器提供的默认构造函数，所以必须声明默认构造函数才可以使用。
		 */
		IUncopyable() {}
		/**
		 * 因为是抽象类所以必须有纯虚函数
		 */
		virtual ~IUncopyable() = 0 {}
							 private:
								 IUncopyable(const IUncopyable&);
		const IUncopyable& operator=(const IUncopyable&);
	};

}

#endif
