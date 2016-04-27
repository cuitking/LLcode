#pragma once

#include <loki/Pimpl.h>
#include <boost/noncopyable.hpp>
#include <vector>

typedef struct lua_State lua_State;

namespace scgl
{

	class CVariant;
	typedef std::vector<CVariant> VariantVector;

	class CLuaFunctor : public boost::noncopyable
	{
	public:
		CLuaFunctor(lua_State* L, int index);
		~CLuaFunctor(void);
		bool operator()(const VariantVector* parameters = NULL, VariantVector* results = NULL);

	private:
		void PushVariant(lua_State* L, const CVariant& variant);

	private:
		Loki::PimplOf<CLuaFunctor>::Type m_impl;
	};

}
