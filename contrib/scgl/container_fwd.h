#ifndef STD_CONTAINER_FWD_H
#define STD_CONTAINER_FWD_H

namespace std
{
	template<class _Ty>
		class allocator;

#pragma warning( disable : 4348)

	template<class _Ty,	class _Ax = allocator<_Ty> >
		class vector;

}

#endif