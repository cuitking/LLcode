#include "LuaFunctor.h"
#include "Variant.h"
#include "Table.h"
#include "Log.h"
#include <lua/lua.hpp>
#include <math.h>
#include <boost/thread.hpp>

template<>
struct Loki::ImplOf<scgl::CLuaFunctor>
{
	lua_State*		L;
	int				functionReference;		// 当前函数的引用
	ImplOf()
		: L(NULL)
		, functionReference(LUA_NOREF)
	{
	}

	~ImplOf()
	{
	}
};

namespace scgl
{

	CLuaFunctor::CLuaFunctor(lua_State* L, int index)
	{
		if (!lua_isfunction(L, index))
		{
			throw std::bad_alloc("not function");
		}
		m_impl->L = L;
		lua_pushvalue(L, index);
		m_impl->functionReference = luaL_ref(L, LUA_REGISTRYINDEX);
	}

	CLuaFunctor::~CLuaFunctor(void)
	{
		if (m_impl->functionReference != LUA_NOREF)
		{
			luaL_unref(m_impl->L, LUA_REGISTRYINDEX, m_impl->functionReference);
		}
	}

	CVariant ConvertValue2Variant(lua_State* L, int index);
	void ParseTable(lua_State* L, int index, CTable* table);


	bool CLuaFunctor::operator()(const VariantVector* parameters, VariantVector* results)
	{
		if (!lua_checkstack(m_impl->L, static_cast<int>(parameters->size()) + 1))
		{
			return false;
		}

		int n = lua_gettop(m_impl->L);

		lua_rawgeti(m_impl->L, LUA_REGISTRYINDEX, m_impl->functionReference);
		if (!lua_isfunction(m_impl->L, -1))
		{
			lua_pop(m_impl->L, lua_gettop(m_impl->L) - n);
			return false;
		}

		// 压入参数
		if (parameters && !(parameters->empty()))
		{
			for (VariantVector::const_iterator it = parameters->begin(); it != parameters->end(); ++it)
			{
				PushVariant(m_impl->L, *it);
			}
		}

		LOG_TRACE("Lua", __FUNCTION__ << boost::this_thread::get_id() << ",this:" << this << ",L:" << m_impl->L);
		// 调用函数
		if (lua_pcall(m_impl->L, lua_gettop(m_impl->L) - 1 - n, LUA_MULTRET, 0))
		{
			LOG_ERROR("Lua", "脚本执行错误，请检查脚本代码:" << luaL_checkstring(m_impl->L, -1));
			lua_pop(m_impl->L, lua_gettop(m_impl->L) - n);
			return false;
		}


		if (results != NULL)
		{
			int retCount = lua_gettop(m_impl->L) - n;
			if (retCount <= 0)
			{
				lua_pop(m_impl->L, lua_gettop(m_impl->L) - n);
				return true;
			}
			// 取得结果
			for (int i = -retCount; i <= -1; ++i)
			{
				CVariant variant = ConvertValue2Variant(m_impl->L, i);
				if (variant != CVariant::NIL)
				{
					results->push_back(variant);
				}
				else
				{
					break;
				}
			}
		}
		lua_pop(m_impl->L, lua_gettop(m_impl->L) - n);
		return true;
	}

	void CLuaFunctor::PushVariant(lua_State* L, const CVariant& variant)
	{
		if (!lua_checkstack(L, 1))
		{
			return;
		}

		switch (variant.GetMetaType())
		{
		case CVariant::TYPE_NIL:
			lua_pushnil(L);
			break;
		case CVariant::TYPE_INTEGER:
			lua_pushnumber(L, variant.GetNumber<lua_Number>());
			break;
		case CVariant::TYPE_NUMBER:
			lua_pushnumber(L, variant.GetNumber<lua_Number>());
			break;
		case CVariant::TYPE_BOOLEAN:
			lua_pushboolean(L, variant.GetBoolean());
			break;
		case CVariant::TYPE_STRING:
		{
			const char* string = variant.GetString();
			lua_pushlstring(L, string, strlen(string));
		}
		break;
		case CVariant::TYPE_TABLE:
			variant.GetTable().PushIntoLua(L);
			break;
		case CVariant::TYPE_LIGHTUSERDATA:
			lua_pushlightuserdata(L, (void*)variant.GetLightUserdata());
			break;
		default:
			lua_pushnil(L);
		}
	}

	void ParseTable(lua_State* L, int index, CTable* table)
	{
		lua_pushvalue(L, index);

		lua_pushnil(L);
		while (lua_next(L, -2))
		{
			CVariant key;
			CVariant value;

			switch (lua_type(L, -2))
			{
			case LUA_TBOOLEAN:
			{
				key = lua_toboolean(L, -2) != 0;
			}
			break;
			case LUA_TSTRING:
			{
				key = CVariant(luaL_checkstring(L, -2));
			}
			break;
			case LUA_TNUMBER:
			{
				double temp = luaL_checknumber(L, -2);
				long long tempInt = static_cast<long long>(temp);
				if (temp == tempInt)
				{
					key = tempInt;
				}
				else
				{
					key = temp;
				}
			}
			break;
			default:
				return;
			}

			switch (lua_type(L, -1))
			{
			case LUA_TBOOLEAN:
			{
				value = lua_toboolean(L, -1) != 0;
				table->Insert(key, value);
			}
			break;
			case LUA_TSTRING:
			{
				value = CVariant(luaL_checkstring(L, -1));
				table->Insert(key, value);
			}
			break;
			case LUA_TNUMBER:
			{
				double temp = luaL_checknumber(L, -1);
				long long tempInt = static_cast<long long>(temp);
				if (temp == tempInt)
				{
					value = tempInt;
				}
				else
				{
					value = temp;
				}
				table->Insert(key, value);
			}
			break;
			case LUA_TTABLE:
			{
				CTable t;
				ParseTable(L, -1, &t);
				table->Insert(key, t);
			}
			}

			lua_pop(L, 1);
		}

		lua_pop(L, 1);
	}

	CVariant ConvertValue2Variant(lua_State* L, int index)
	{
		CVariant result;
		int type = lua_type(L, index);
		if (type == LUA_TNONE || type == LUA_TNIL)
		{
			return CVariant();
		}

		switch (type)
		{
		case LUA_TNUMBER:
		{
			double temp = luaL_checknumber(L, index);
			long long tempInt = static_cast<long long>(temp);
			if (temp == tempInt)
			{
				return tempInt;
			}
			else
			{
				return temp;
			}
		}
		case LUA_TBOOLEAN:
			return lua_toboolean(L, index) != 0;
		case LUA_TSTRING:
			return luaL_checkstring(L, index);
		case LUA_TTABLE:
		{
			CTable t;
			ParseTable(L, index, &t);
			return t;
		}
		}

		return CVariant();
	}

}
