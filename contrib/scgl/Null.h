#ifndef SCGL_NULL_H
#define SCGL_NULL_H

namespace scgl
{

	static const class
	{
	public:
		template<class T>
		operator T* () const
		{
			return 0;
		}
		//template<class C, class T>
		//    operator T C::*() const {  return 0; }

	private:
		void operator&() const;
	} Null;

}

#endif
