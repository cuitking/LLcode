#ifndef SCGL_DYNAMIC_POOL_H
#define SCGL_DYNAMIC_POOL_H

namespace scgl
{

	class CDynamicPool
	{
	public:
		enum
		{
			NEW_BLOCK_AMOUNT = 4,
			MIN_BLOCK_SIZE = 32,
		};

		explicit CDynamicPool(size_t nMinBlockSize = MIN_BLOCK_SIZE);
		~CDynamicPool(void);

	public:
		void* Allocate(size_t n, size_t* pTrueSize = 0);
		bool Release(void*& p);

	private:
		size_t ComputeNeedSize(size_t n);

	private:
		size_t  m_nMinBlockSize;
		struct  Impl;
		Impl*   m_pImpl;
	};

}

#endif
