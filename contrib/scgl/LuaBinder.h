#ifndef SCGL_LUA_BINDER_H
#define SCGL_LUA_BINDER_H

#include "tiostream"
#include "../Lua/lua.hpp"
#include <vector>
#include "LuaHelper.h"

namespace scgl
{

	template <typename T>
	int LuaCallback(lua_State* L)
	{
		int size = lua_gettop(L);
		if (size < 2)
		{
			lua_pushboolean(L, false);
			lua_pushstring(L, "The param's amount is less than two.");
			return 2;
		}

		CLuaBinder<T>* t = NULL;
		if (lua_islightuserdata(L, 1))
		{
			t = static_cast<CLuaBinder<T>*>(lua_touserdata(L, 1));
		}
		else
		{
			lua_pushboolean(L, false);
			lua_pushstring(L, "The param one is not userdata.");
			return 2;
		}

		int funcPos = -1;
		if (lua_type(L, 2) == LUA_TNUMBER)
		{
			funcPos = static_cast<int>(luaL_checknumber(L, 2));
		}
		else
		{
			lua_pushboolean(L, false);
			lua_pushstring(L, "The param two is not number.");
			return 2;
		}

		int returnValueAmount = 0;
		if (t && t->m_pObject && funcPos >= 0)
		{
			lua_remove(L, 1);
			lua_remove(L, 1);
			returnValueAmount = (t->m_pObject->*(t->m_functions[funcPos]))(L);
			return returnValueAmount;
		}
		else
		{
			lua_pushboolean(L, false);
			lua_pushstring(L, "The object of param one is not exist.");
			return 2;
		}
	}

	template <typename T>
	class CLuaBinder
	{
	public:
		typedef int (T::*Function)(lua_State*);
	public:
		explicit CLuaBinder(const char* objectName, T* pObject = NULL, CLuaHelper* pLua = NULL);
		~CLuaBinder(void);
	public:
		void BindObject(T* pObject);
		void BindLua(CLuaHelper* pLua);
		void Register(Function function, const char* functionName);

	private:
		friend int LuaCallback<T>(lua_State*);
	private:
		CLuaBinder(const CLuaBinder&);
		const CLuaBinder& operator=(const CLuaBinder&);
	private:
		void CreateLuaTable();
	private:
		tstring                 m_strObjectName;
		T*                      m_pObject;
		lua_State*              m_pLS;
		std::vector<Function>   m_functions;

	};

	template <typename T>
	CLuaBinder<T>::CLuaBinder(const char* objectName, T* pObject = NULL, CLuaHelper* pLua = NULL)
		: m_strObjectName(objectName)
		, m_pObject(pObject)
	{
		if (pLua != NULL)
		{
			BindLua(pLua);
		}
	}

	template <typename T>
	CLuaBinder<T>::~CLuaBinder(void)
	{

	}

	template <typename T>
	void CLuaBinder<T>::BindObject(T* pObject)
	{
		m_pObject = pObject;
	}

	template <typename T>
	void CLuaBinder<T>::BindLua(CLuaHelper* pLua)
	{
		if (pLua != NULL)
		{
			m_pLS = pLua->m_pMainThread;
			CreateLuaTable();
		}
	}

	template <typename T>
	void CLuaBinder<T>::Register(Function function, const char* functionName)
	{
		int n = lua_gettop(m_pLS);

		lua_getglobal(m_pLS, m_strObjectName.c_str());
		if (lua_istable(m_pLS, -1))
		{
			lua_pushstring(m_pLS, functionName);
			lua_pushnumber(m_pLS, m_functions.size());
			lua_settable(m_pLS, -3);
		}

		lua_pop(m_pLS, lua_gettop(m_pLS) - n);

		m_functions.push_back(function);
	}

	template <typename T>
	void CLuaBinder<T>::CreateLuaTable()
	{
		lua_newtable(m_pLS);
		lua_pushstring(m_pLS, "this");
		lua_pushlightuserdata(m_pLS, this);
		lua_settable(m_pLS, -3);
		lua_pushstring(m_pLS, "LuaCallBack");
		lua_pushcfunction(m_pLS, LuaCallback<T>);
		lua_settable(m_pLS, -3);
		lua_setglobal(m_pLS, m_strObjectName.c_str());
	}

}

#endif
